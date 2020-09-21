#define BACKLOG 10
#define MAXUSRCOUNT 1000
#define QUERYLEN 200
#define SHALEN 100
#include <pthread.h>
#include "common.h"
#include "uthash.h"

const char *port = "8787";
char query[QUERYLEN];

struct user{
    char username[NAMELEN];
    int sockfd;
    UT_hash_handle hh;
};

struct user *users;
pthread_rwlock_t lock;

void *get_in_addr(struct sockaddr *sa);
uint16_t get_in_port(struct sockaddr *sa);
void *thread_handler(void *input);
void login_handler(int sockfd);
void register_handler(int sockfd);
void main_handler(int sockfd, char *username);
void get_messages_handler(int sockfd, char *username);
// void send_message_handler(int sockfd, char *username);
static int callback(void *ret, int n_columns, char **value, char **column_name);
void encrypt(char *password, char *hash);
void add_user(char *new_username, int sockfd);
void delete_user(char *username_to_delete);
struct user *find_user(char *username);
void send_message_handler(int sockfd, char *username);
int sendall(int s, char *buf, int len);