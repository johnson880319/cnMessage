#define NAMELEN 16
#define PWDLEN 16
#define MSGLEN 512
#define MAX_CONTACTS 100;
#define MAXLINE 1000

const char ERROR = -1;
const char END = -1;

const char EMPTY = 0;

const char COMMAND_OK = 1;
const char LOGIN = 2;
const char NAME_OR_PWD_NOT_FOUND = 3;
const char REGISTER = 4;
const char NAME_EXISTED = 5;
const char GET_MESSAGES = 6;
const char SEND_MESSAGE = 7;
// const char GET_CONTACTS = 6;
// const char ADD_CONTACT = 7;
const char EXIT = 10;

char *mesg[10] = {};
