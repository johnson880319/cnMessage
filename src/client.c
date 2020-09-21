#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include <cdk.h>
#include "client.h"


int main(int argc, char *argv[]){
    int sockfd;
    // /* create threads to handle simultaneous connections */
    // pthread_t tid;

    // if (pthread_create(&tid, NULL, threadHandler, 0) < 0){
    //     perror("pthread create failed");
    // }

    // /* wait for threads to finish */
    // if (pthread_join(tid, NULL) < 0){
    //     perror("pthread join failed");
    // }
    if((sockfd = connect_to_server()) == -1){
        printf("connect to server error\n");
        exit(1);
    }
    show_login_screen(sockfd);
    close(sockfd);
    return 0;
}

int connect_to_server(){
    struct addrinfo hints, *servinfo;
    int rv;
    char *pch, *saveptr, address[ADDRLEN];
    fd_set readfds;
    int error, errlen = sizeof error;
    char message[MSGLEN];
    FILE *fp;

    // printf("enter server address:");
    if((fp = fopen("address.txt", "r")) == NULL){
        printf("address.txt open error, please enter the server address in address.txt\n");
        return -1;
    }
    fgets(address, sizeof address, fp);
    fclose(fp);

    /* get address info */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if((rv = getaddrinfo(address, port, &hints, &servinfo)) != 0){
        fprintf(stderr, "getaddrinfo:%s", gai_strerror(rv));    
        return -1;
    }

    /* trying to build socket */
    if((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1){
        perror("client socket error");
        return -1;
    }

    if(connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
        perror("connect error");
        return -1;
    }
    else
    {
        printf("connected to %s\n", address);
    }
    freeaddrinfo(servinfo);

    // while (1)
    // {
    //     printf("enter messange:");
    //     fgets(message, sizeof message, stdin);
    //     send(sockfd, &message, strlen(message)+1, 0);
    //     if(strcmp(message, "close\n" ) == 0)
    //         break;
    // }
    
}

void show_login_screen(){
    WINDOW *menu_win, *sub_win, *pad;
    CDKSCREEN *menu_cdkscreen;
    CDKITEMLIST *menu_cdkitemlist;
    int curline, curcol;

    char *mesg[5];
    char temp[256];
    char ch, ret;

    int choice;

    init_scr();

    addstr(logo);
    refresh();

    getyx(stdscr, curline, curcol);
    menu_win = create_win(LINES - curline, COLS, curline, curcol);
    menu_cdkscreen = initCDKScreen(menu_win);
    menu_cdkitemlist = newCDKItemlist(menu_cdkscreen, LEFT, TOP, welcome_message, NULL, menu_list, MENU_LISTCOUNT, 0, FALSE, FALSE);
    drawCDKItemlist(menu_cdkitemlist, FALSE);
    choice = activateCDKItemlist(menu_cdkitemlist, NULL);

    while(TRUE){
        if(choice == 0){
            ret = login_win(menu_cdkscreen);
            if(ret == EXIT){
                choice = activateCDKItemlist(menu_cdkitemlist, NULL);
                continue;
            }
            while(ret != COMMAND_OK){
                ret = login_win(menu_cdkscreen);
            }
            main_chatwin();
            printw("finished main chatwin\n");
            break;
        }
        else if (choice == 1){
            while((ret = register_win(menu_cdkscreen)) != COMMAND_OK){}
        }
        else{
            break;
        }
        // sprintf (temp, "<C>You selected the %dth item which is", choice);
        // mesg[0] = temp;
        // mesg[1] = menu_list[choice];
        // mesg[2] = "";
        // mesg[3] = "<C>Press any key to continue.";
        // popupLabel (ScreenOf (menu_cdkitemlist), mesg, 4);
        choice = activateCDKItemlist(menu_cdkitemlist, NULL);
    }
    
    destroyCDKItemlist(menu_cdkitemlist);
    destroyCDKScreen(menu_cdkscreen);
    destroy_win(menu_win);
    endwin();

    return;
}

void init_scr(){
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
}

WINDOW *create_win(int height, int width, int starty, int startx){
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    wrefresh(local_win);

    return local_win;
}

void destroy_win(WINDOW *local_win){	
	wclear(local_win);
	wrefresh(local_win);
	delwin(local_win);
}

char login_win(CDKSCREEN *cdkscreen){
    eraseCDKScreen(cdkscreen);
    char password[PWDLEN], *entry_ret, ret;
    CDKENTRY *username_entry, *password_entry;

    username_entry = newCDKEntry(cdkscreen, LEFT, TOP, NULL, "Username:",
                                 A_NORMAL, '_', vMIXED, NAMELEN-1, 0, NAMELEN-1, FALSE, FALSE);
    // drawCDKEntry(username_entry, FALSE);
    if((entry_ret = activateCDKEntry(username_entry, NULL)) == NULL){
        destroyCDKEntry(username_entry);
        return EXIT;
    }
    else{
        strncpy(username, entry_ret, NAMELEN);
    }
    
    password_entry = newCDKEntry(cdkscreen, LEFT, TOP, NULL, "Password:", A_INVIS , '_', vMIXED, PWDLEN-1, 0, PWDLEN-1, FALSE, FALSE);
    moveCDKEntry(password_entry, 0, 1, TRUE, FALSE);
    drawCDKEntry(username_entry, FALSE);
    // drawCDKEntry(password_entry, FALSE);
    if((entry_ret = activateCDKEntry(password_entry, NULL)) == NULL){
        destroyCDKEntry(username_entry);
        destroyCDKEntry(password_entry);
        return EXIT;
    }
    else{
        strncpy(password, entry_ret, PWDLEN);
    }
    char *mesg[100] = {"entered username:", username, "password:", password};
    // popupLabel(cdkscreen, mesg , 4);
    ret = send_login_or_reg(cdkscreen, LOGIN, username, password);
    
    // if(send(sockfd, &LOGIN, 1, 0) == -1){
    //     mesg[0] = "server disconnected";
    //     popupLabel(cdkscreen, mesg , 1);
    // }

    // if(sendall(sockfd, &username, NAMELEN) == -1){
    //     mesg[0] = "server disconnected";
    //     popupLabel(cdkscreen, mesg , 1);
    // }

    // if(sendall(sockfd, &password, PWDLEN) == -1){
    //     mesg[0] = "server disconnected";
    //     popupLabel(cdkscreen, mesg , 1);
    // }


    // if(recv(sockfd, &ret, 1, 0) != 1){
    //     mesg[0] = "server disconnected";
    //     popupLabel(cdkscreen, mesg , 1);
    // }


    if(ret == COMMAND_OK){
        mesg[0] = "Login success";
        popupLabel(cdkscreen, mesg , 1);
        destroyCDKEntry(username_entry);
        destroyCDKEntry(password_entry);
        return COMMAND_OK;
    }
    else if(ret == NAME_OR_PWD_NOT_FOUND){
        mesg[0] = "Username or password not found!";
        popupLabel(cdkscreen, mesg , 1);
        destroyCDKEntry(username_entry);
        destroyCDKEntry(password_entry);
        return EXIT;
    }
    else{
        mesg[0] = "ERROR";
        popupLabel(cdkscreen, mesg , 1);
        destroyCDKEntry(username_entry);
        destroyCDKEntry(password_entry);
        return EXIT;
    }




}

char register_win(CDKSCREEN *cdkscreen){
    eraseCDKScreen(cdkscreen);
    char password[PWDLEN], password2[PWDLEN], *entry_ret, ret;
    CDKENTRY *username_entry, *password_entry, *password_reentry;
    char *mesg[100] = {"entered username:", username, "password:", password};
    
    username_entry = newCDKEntry(cdkscreen, LEFT, TOP, NULL, "Username:",
                                 A_NORMAL, '_', vMIXED, NAMELEN-1, 0, NAMELEN-1, FALSE, FALSE);
    if((entry_ret = activateCDKEntry(username_entry, NULL)) == NULL){
        destroyCDKEntry(username_entry);
        return COMMAND_OK;
    }else
        strncpy(username, entry_ret, NAMELEN);
    
    password_entry = newCDKEntry(cdkscreen, LEFT, TOP, NULL, "Password:", A_INVIS , '_', vMIXED, PWDLEN-1, 0, PWDLEN-1, FALSE, FALSE);
    moveCDKEntry(password_entry, 0, 1, TRUE, FALSE);
    drawCDKEntry(username_entry, FALSE);
    if((entry_ret = activateCDKEntry(password_entry, NULL)) == NULL){
        destroyCDKEntry(username_entry);
        destroyCDKEntry(password_entry);
        return COMMAND_OK;
    }else
        strncpy(password, entry_ret, PWDLEN);

    password_reentry = newCDKEntry(cdkscreen, LEFT, TOP, NULL, "Confirm password:", A_INVIS , '_', vMIXED, PWDLEN-1, 0, PWDLEN-1, FALSE, FALSE);
    moveCDKEntry(password_reentry, 0, 2, TRUE, FALSE);
    drawCDKEntry(username_entry, FALSE);
    drawCDKEntry(password_entry, FALSE);
    if((entry_ret = activateCDKEntry(password_reentry, NULL)) == NULL){
            destroy_regwin(username_entry, password_entry, password_reentry);
            return COMMAND_OK;            
    }
    else
        strncpy(password2, entry_ret, PWDLEN);
    

    if(strcmp(password, password2) != 0){
        char *mesg[] = {"password does not match!"};
        popupLabel(cdkscreen, mesg, 1);
        destroy_regwin(username_entry, password_entry, password_reentry);
        return ERROR;            
    }
    else{
        ret = send_login_or_reg(cdkscreen, REGISTER, username, password);
        if(ret == COMMAND_OK){
            mesg[0] = "OK";
            popupLabel(cdkscreen, mesg , 1);
            destroy_regwin(username_entry, password_entry, password_reentry);
            return COMMAND_OK;
        }
        else if(ret == NAME_EXISTED){
            mesg[0] = "NAME_EXISTED";
            popupLabel(cdkscreen, mesg , 1);
            destroy_regwin(username_entry, password_entry, password_reentry);
            return ERROR;
        }
        else{
            mesg[0] = "ERROR";
            popupLabel(cdkscreen, mesg , 1);
            destroy_regwin(username_entry, password_entry, password_reentry);
            return ERROR;            
        }
    }

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

char send_login_or_reg(CDKSCREEN *cdkscreen, const char command, char *username, char *password){
    char ret;

    if(send(sockfd, &command, 1, 0) == -1){
        mesg[0] = "server disconnected";
        popupLabel(cdkscreen, mesg , 1);
    }

    if(sendall(sockfd, username, NAMELEN) == -1){
        mesg[0] = "server disconnected";
        popupLabel(cdkscreen, mesg , 1);
    }

    if(sendall(sockfd, password, PWDLEN) == -1){
        mesg[0] = "server disconnected";
        popupLabel(cdkscreen, mesg , 1);
    }


    if(recv(sockfd, &ret, 1, 0) != 1){
        mesg[0] = "server disconnected";
        popupLabel(cdkscreen, mesg , 1);
    }

    return ret;
}

void destroy_regwin(CDKENTRY *username_entry, CDKENTRY *password_entry, CDKENTRY *password_reentry){
    destroyCDKEntry(username_entry);
    destroyCDKEntry(password_entry);
    destroyCDKEntry(password_reentry);
    return;
}

void main_chatwin(){
    char *target_username = NULL;
    char message[MSGLEN], *entry_ret;
    int ret;
    pthread_t tid; tid;
    time_t t;

    WINDOW *message_win, *entry_win;
    CDKSCREEN *message_cdkscreen, *entry_cdkscreen;
    CDKSWINDOW *messages;
    CDKMENTRY *entry;

    message_win = create_win(LINES - LINES/3, COLS, 0, 0);
    entry_win = create_win(LINES/3, COLS, LINES-LINES/3, 0);

    message_cdkscreen = initCDKScreen(message_win);
    entry_cdkscreen = initCDKScreen(entry_win);


    target_username = getString(message_cdkscreen, "Enter the username to start chatting:", NULL, NULL);
    while(sizeof target_username > 16){
        target_username = getString(message_cdkscreen, "Username length must be less than 16!:", NULL, NULL);
    }

    char label[NAMELEN*3] = "<C>";
    snprintf(label, NAMELEN*3, "<C>%s to %s", username, target_username);
    messages = newCDKSwindow(message_cdkscreen, CENTER, TOP, 0, 0, label, MAXLINE, TRUE, FALSE);
    entry = newCDKMentry(entry_cdkscreen, LEFT, TOP, NULL, "Enter message:", A_NORMAL, '_', vMIXED, 0, 0, MSGLEN/LINES/3, FALSE, TRUE, FALSE);

    drawCDKMentry(entry, TRUE);

    if(send(sockfd, &GET_MESSAGES, sizeof GET_MESSAGES, 0) == -1){
        mesg[0] = "server disconnected";
        popupLabel(message_cdkscreen, mesg , 1);
        return;
    }
    if(send(sockfd, target_username, sizeof target_username, 0) == -1){
        mesg[0] = "server disconnected";
        popupLabel(message_cdkscreen, mesg , 1);
        return;
    }
    while(recv(sockfd, message, MSGLEN, 0) != 0){
        if(message[0] == END) break;
        addCDKSwindow(messages, message, BOTTOM);
    }

    if(pthread_create(&tid, NULL, update_messages, messages) < 0){
        perror("pthread create error");
    }

    while (messages->exitType != vESCAPE_HIT)
    {
        activateCDKSwindow(messages, NULL);
        if(messages->exitType == vNORMAL)
            entry_ret = activateCDKMentry(entry, NULL);
            if(entry_ret != NULL){
                t = time(NULL);
                snprintf(message, MSGLEN, "%s", entry_ret);
                if(send(sockfd, &SEND_MESSAGE, sizeof SEND_MESSAGE, 0) == -1){
                    mesg[0] = "server disconnected";
                    popupLabel(message_cdkscreen, mesg , 1);
                    return;
                }
                if(send(sockfd, target_username, NAMELEN , 0) == -1){
                    mesg[0] = "server disconnected";
                    popupLabel(message_cdkscreen, mesg , 1);
                    return;
                }                            
                if(sendall(sockfd, message, MSGLEN) == -1){
                    mesg[0] = "server disconnected";
                    popupLabel(message_cdkscreen, mesg , 1);
                    return;
                }
                snprintf(message, MSGLEN, "%s: %s", username, entry_ret);
                addCDKSwindow(messages, message, BOTTOM);
                cleanCDKMentry(entry);
            }
    }

    pthread_cancel(tid);

    destroyCDKScreen(message_cdkscreen);
    destroyCDKScreen(entry_cdkscreen);
    
    destroy_win(message_win);
    destroy_win(entry_win);

    return;
}

void *update_messages(void *input){
    CDKSWINDOW *messages = (CDKSWINDOW *)input;
    char message[MSGLEN];
    char command;

    while(recv(sockfd, &command, sizeof command, 0) != 0){
        if(command == SEND_MESSAGE){
            recv(sockfd, message, MSGLEN, 0);
            addCDKSwindow(messages, message, BOTTOM);
        }
    }
}


