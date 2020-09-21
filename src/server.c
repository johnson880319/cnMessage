#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sqlite3.h>
#include <pthread.h>
#include <openssl/sha.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include "uthash.h"
#include "server.h"



int main(int argc, char *argv[]){
    struct sockaddr_storage theiraddr;
    socklen_t addrsize;
    struct addrinfo hints, *servinfo;
    int rv, sockfd, newfd;
    int optval = 1;
    char s[INET_ADDRSTRLEN];
    char message[MSGLEN];
    uint16_t portnum;
    pthread_t tid;
    users = NULL;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0){
        fprintf(stderr, "getaddrinfo:%s", gai_strerror(rv));
        exit(1);
    }

    /*set socket*/
    if((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1){
        perror("server socket error");
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == -1) {
        perror("setsockopt error");
        exit(1);
    }

    /*bind port*/
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sockfd);
        perror("server bind error");
        exit(1);
    }
    freeaddrinfo(servinfo);

    /*listen for connection*/
    if(listen(sockfd, BACKLOG) == -1){
        perror("listen error");
        exit(1);
    }
    printf("listening at port 8787\n");

    if (pthread_rwlock_init(&lock,NULL) != 0) {
      fprintf(stderr,"lock init failed\n");
      exit(-1);
    }

    while(1){
        addrsize = sizeof(theiraddr);

        /*accept connection*/
        if((newfd = accept(sockfd, (struct sockaddr *)&theiraddr, &addrsize)) == -1){
            continue;
        }else{
            if(pthread_create(&tid, NULL, thread_handler, &newfd) < 0){
                perror("pthread create error");
                continue;
            }
            inet_ntop(theiraddr.ss_family, get_in_addr((struct sockaddr *)&theiraddr), s, sizeof s);
            portnum = ntohs(get_in_port((struct sockaddr *)&theiraddr));
            printf("accepted from %s:%d\n", s, portnum);
        }
        /*get connection info, print it out and close connection*/
    }
    return 0;
}

void *get_in_addr(struct sockaddr *sa){
        return &(((struct sockaddr_in*)sa)->sin_addr);
}

uint16_t get_in_port(struct sockaddr *sa){
        return (((struct sockaddr_in*)sa)->sin_port);
}

void *thread_handler(void *input){
    int sockfd = *(int *)input;
    char username[NAMELEN], password[PWDLEN], command;
    while(recv(sockfd, &command, sizeof command, 0) != 0){
        if(command == LOGIN)
            login_handler(sockfd);
        else if (command == REGISTER)
            register_handler(sockfd);
    }
    printf("connection closed\n");
    pthread_exit(NULL);
}

void login_handler(int sockfd){
    char username[NAMELEN], password[PWDLEN], query[QUERYLEN], *errmsg;
    unsigned char encrypted_password[SHA256_DIGEST_LENGTH];
    char password_hash_printable[SHALEN], ret[SHALEN];
    sqlite3 *db;
    int rc;

    memset(password_hash_printable, 0, sizeof password_hash_printable);
    memset(ret, 0, sizeof ret);
    printf("received command LOGIN\n");
    if(recv(sockfd, username, NAMELEN, 0) == 0) return;
    if(recv(sockfd, password, PWDLEN, 0) == 0) return;
    encrypt(password, encrypted_password);
    for(int i = 0;i < SHA256_DIGEST_LENGTH;i++){
        sprintf(password_hash_printable+strlen(password_hash_printable), "%02x", encrypted_password[i]);
    }
    password_hash_printable[strlen(password_hash_printable)] = '\0';
    


    rc = sqlite3_open("accounts.db", &db);
        if(rc){
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            send(sockfd, &ERROR, sizeof ERROR, 0);
            return;
        }

    rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS ACCOUNTS (USERNAME TEXT, PASSWORD TEXT)", NULL, NULL, &errmsg);
    if(rc != SQLITE_OK){
        fprintf(stderr, "SQL error: %s\n", errmsg);
        send(sockfd, &ERROR, sizeof ERROR, 0);
        return;
    }
    
    memset(query, 0, sizeof query);
    snprintf(query, QUERYLEN, "SELECT PASSWORD FROM ACCOUNTS WHERE USERNAME = '%s'", username);
    rc = sqlite3_exec(db, query, callback, ret, NULL);
    if(rc != SQLITE_OK){
        fprintf(stderr, "SQL error: %s\n", errmsg);
        send(sockfd, &ERROR, sizeof ERROR, 0);
        return;
    }
    else
    {
        if(ret == NULL){
            send(sockfd, &NAME_OR_PWD_NOT_FOUND, sizeof NAME_OR_PWD_NOT_FOUND, 0);
            return;
        }
        else{
            if(strcmp(password_hash_printable, ret) != 0){
                printf("sent:%s, database:%s\n", password_hash_printable, ret);
                send(sockfd, &NAME_OR_PWD_NOT_FOUND, sizeof NAME_OR_PWD_NOT_FOUND, 0);
                return;
            }
            else{
                printf("user \"%s\" logged in\n", username);
                send(sockfd, &COMMAND_OK, sizeof COMMAND_OK, 0);
                add_user(username, sockfd);
                main_handler(sockfd, username);
                delete_user(username);
                return;
            }
        }
    }
    

    return;
}

void register_handler(int sockfd){
    char username[NAMELEN], password[PWDLEN], *errmsg, *ret = NULL;
    unsigned char encrypted_password[SHA256_DIGEST_LENGTH];
    char password_hash_printable[SHALEN];
    sqlite3 *db;
    int rc;

    memset(password_hash_printable, 0, sizeof password_hash_printable);
    printf("received command REGISTER\n");
    if(recv(sockfd, username, NAMELEN, 0) == 0) return;
    if(recv(sockfd, password, PWDLEN, 0) == 0) return;

    rc = sqlite3_open("accounts.db", &db);
    if(rc){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        send(sockfd, &ERROR, sizeof ERROR, 0);
        return;
    }

    rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS ACCOUNTS (USERNAME TEXT, PASSWORD TEXT)", NULL, NULL, &errmsg);
    if(rc != SQLITE_OK){
        fprintf(stderr, "SQL error: %s\n", errmsg);
        send(sockfd, &ERROR, sizeof ERROR, 0);
        return;
    }

    memset(query, 0, sizeof query);
    snprintf(query, QUERYLEN, "SELECT USERNAME FROM ACCOUNTS WHERE USERNAME = '%s'", username);

    ret = (char *)malloc(NAMELEN * sizeof(char));
    rc = sqlite3_exec(db, query, callback, ret, NULL);
    if(rc != SQLITE_OK){
        fprintf(stderr, "SQL error: %s\n", errmsg);
        send(sockfd, &ERROR, sizeof ERROR, 0);
        return;
    }
    else{
        if (strcmp(ret, username) == 0){
            send(sockfd, &NAME_EXISTED, sizeof NAME_EXISTED, 0);
            free(ret);
        }
        else{
            encrypt(password, encrypted_password);
            for(int i = 0;i < SHA256_DIGEST_LENGTH;i++){
                sprintf(password_hash_printable+strlen(password_hash_printable), "%02x", encrypted_password[i]);
            }
            password_hash_printable[strlen(password_hash_printable)] = '\0';
            printf("encrypred password text:%s\n", password_hash_printable);
            memset(query, 0, sizeof query);
            snprintf(query, QUERYLEN, "INSERT INTO ACCOUNTS (USERNAME, PASSWORD) VALUES ('%s', '%s')", username, password_hash_printable);
            rc = sqlite3_exec(db, query, NULL, NULL, &errmsg);
            if(rc != SQLITE_OK){
                fprintf(stderr, "INSERT INTO ACCOUNTS (USERNAME, PASSWORD) SQL error: %s\n", errmsg);
                send(sockfd, &ERROR, sizeof ERROR, 0);
                return;
            }
            else{
                printf("inserted username %s password %s into database\n", username, password_hash_printable);
                send(sockfd, &COMMAND_OK, sizeof COMMAND_OK, 0);
                return;
            }
        }
    }    
}

void main_handler(int sockfd, char *username){
    char command;
    while(recv(sockfd, &command, sizeof command ,0) != 0){
        if(command == GET_MESSAGES){
            printf("received command GET_MESSAGES\n");
            get_messages_handler(sockfd, username);
        }
        if(command == SEND_MESSAGE){
            printf("received command SEND_MESSAGE\n");
            send_message_handler(sockfd, username);
        }
    }
    return;
}

void get_messages_handler(int sockfd, char *username){
    FILE *fp_from, *fp_to;
    char directory_from[PATH_MAX], directory_to[PATH_MAX], target_username[NAMELEN], message[MSGLEN];
    char *line;
    size_t len = 0;
    ssize_t nread;
    time_t t;


    if(recv(sockfd, target_username, NAMELEN, 0) == 0) return;

    if(mkdir("messages", 0777) < 0){
        perror("mkdir error");
    }
    
    snprintf(directory_from, PATH_MAX, "messages/%s", username);
    if(mkdir(directory_from, 0777) < 0){
        perror("mkdir error");
    }
    snprintf(directory_from, PATH_MAX, "messages/%s/%s.txt", username, target_username);

    fp_from = fopen(directory_from, "r");
    if(fp_from == NULL){
        perror("fopen error");
        fp_from = fopen(directory_from, "w+");
        t = time(NULL);
        snprintf(message, MSGLEN, "[system message] start chatting!\n");
        fputs(message, fp_from);
        send(sockfd, message, MSGLEN, 0);
        fflush(fp_from);

        snprintf(directory_from, PATH_MAX, "messages/%s", target_username);
        if(mkdir(directory_from, 0777) < 0){
            perror("mkdir error");
        }
        snprintf(directory_to, PATH_MAX, "messages/%s/%s.txt", target_username, username);
        fp_to = fopen(directory_to, "w+");
        fputs(message, fp_to);
        fflush(fp_to);
        fclose(fp_to);
    }
    flockfile(fp_from);
    printf("fp locked\n");
    while((nread = getline(&line, &len, fp_from)) != -1){
        snprintf(message, MSGLEN, "%s", line);
        printf("read message:%s", message);
        send(sockfd, message, MSGLEN, 0);
    }
    send(sockfd, &END, sizeof END, 0);
    funlockfile(fp_from);
    printf("fp unlocked\n");
    fclose(fp_from);
    return;    
}

void send_message_handler(int sockfd, char *username){
    FILE *fp_from, *fp_to;
    char directory_from[PATH_MAX], directory_to[PATH_MAX], target_username[NAMELEN], message[MSGLEN], message_fputs[MSGLEN];
    char *line;
    size_t len = 0;
    ssize_t nread;
    time_t t;
    struct user *target;


    if(recv(sockfd, target_username, NAMELEN, 0) == 0) return;
    if(recv(sockfd, message, MSGLEN, 0) == 0) return;

    target = find_user(target_username);

    snprintf(directory_from, PATH_MAX, "messages/%s/%s.txt", username, target_username);
    fp_from = fopen(directory_from, "a");
    flockfile(fp_from);
    snprintf(message_fputs, MSGLEN, "%s: %s\n", username, message);
    fputs(message_fputs, fp_from);
    fflush(fp_from);
    funlockfile(fp_from);
    fclose(fp_from);

    snprintf(directory_to, PATH_MAX, "messages/%s/%s.txt", target_username, username);
    fp_to = fopen(directory_to, "a");
    flockfile(fp_to);
    fputs(message_fputs, fp_to);
    fflush(fp_to);
    funlockfile(fp_to);
    fclose(fp_to);

    if(target){
        send(target->sockfd, &SEND_MESSAGE, sizeof SEND_MESSAGE, 0);
        sendall(target->sockfd, message_fputs, MSGLEN);
    }


    return;        
}

static int callback(void *ret, int n_columns, char **value, char **column_name){
    if (n_columns > 0){
        char *new = value[0];
        strcpy(ret, new);
    }
    else
        ret = NULL;

    return 0;
}

void encrypt(char *password, char *hash){
    SHA256_CTX sha256;

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, (unsigned char*)password, PWDLEN);
    SHA256_Final(hash, &sha256);

    return;
}

void add_user(char *new_username, int sockfd) {
    struct user *s;

    if (pthread_rwlock_wrlock(&lock) != 0) {
        fprintf(stderr,"can't acquire write lock\n");
        exit(-1);
    }
    HASH_FIND_STR(users, new_username, s);
    if (s==NULL) {
      s = (struct user *)malloc(sizeof *s);
      strcpy(s->username, new_username);
      s->sockfd = sockfd;
      HASH_ADD_STR( users, username, s);
    }
    pthread_rwlock_unlock(&lock);
    printf("added user:%s, sockfd:%d\n", new_username, sockfd);
}

void delete_user(char *username_to_delete){
    struct user *s;
    HASH_FIND_STR(users, username_to_delete, s);
    HASH_DEL(users, s);
    printf("deleted user:%s from sockfd database\n", username_to_delete);
    free(s);
}

struct user *find_user(char *username){
    struct user *s;
    if (pthread_rwlock_rdlock(&lock) != 0){
        fprintf(stderr,"can't acquire read lock\n");
        exit(-1);
    }
    HASH_FIND_STR(users, username, s);
    pthread_rwlock_unlock(&lock);
    return s;
}

int sendall(int s, char *buf, int len)
{
    int total = 0;
    int bytesleft = len;
    int n;

    while(total < len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    len = total;

    return n==-1?-1:0;
} 