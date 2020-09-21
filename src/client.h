#define ADDRLEN 100
#define LOGO_LINES 12
#define MENU_LISTCOUNT 2
#include "common.h"

char *port = "8787";
char *welcome_message = 
    "Welcome to cnMessage.\nPlease use the arrow keys to select the actions.\n"
    "Press esc to exit\n";
char *menu_list[] = {
    "Login",
    "Register"
};
int sockfd;

char username[NAMELEN];

int connect_to_server();
void show_login_screen();
void init_scr();
WINDOW *create_win(int height, int width, int starty, int startx);
void destroy_win(WINDOW *local_win);
char login_win(CDKSCREEN *cdkscreen);
char register_win(CDKSCREEN *cdkscreen);
int sendall(int s, char *buf, int len);
char send_login_or_reg(CDKSCREEN *cdkscreen, const char command, char *username, char *password);
void destroy_regwin(CDKENTRY *username_entry, CDKENTRY *password_entry, CDKENTRY *password_reentry);
void main_chatwin();
// char get_contacts(CDKSCREEN *contacts_cdkscreen, CDKSCROLL *contacts_scroll);
void *update_messages(void *input);

char *logo =
    "              __  ___                              \n"
    "  _________  /  |/  /__  ______________ _____ ____ \n"
    " / ___/ __ \\/ /|_/ / _ \\/ ___/ ___/ __ `/ __ `/ _ \\\n"
    "/ /__/ / / / /  / /  __(__  |__  ) /_/ / /_/ /  __/\n"
    "\\___/_/ /_/_/  /_/\\___/____/____/\\__,_/\\__, /\\___/ \n"
    "                                      /____/       \n"
    "  ___        ___  __   __ ___  __ ___ _ ___ _  \n"
    " | _ )_  _  | _ )/  \\ / // _ \\/  \\_  ) |_  ) | \n"
    " | _ \\ || | | _ \\ () / _ \\_, / () / /| |/ /| | \n"
    " |___/\\_, | |___/\\__/\\___//_/ \\__/___|_/___|_| \n"
    "      |__/                                     \n"
    "\n";
