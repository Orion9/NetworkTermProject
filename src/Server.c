#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <time.h>

#define STRING_SIZE 250
#define FIELD_SIZE 3

/* Game Settings */
typedef enum GameSigns {
    EMPTY, O, X
} GameSigns;

typedef struct GameSettings {
    int table[FIELD_SIZE][FIELD_SIZE];
    char player_one_name[STRING_SIZE];
    char player_two_name[STRING_SIZE];
    char winner;
} GameSettings;

/* Game room information */
typedef struct SessionList {
    int room_key;
    char room_name[STRING_SIZE];
    GameSettings game_settings;
    struct SessionList *next;
} SessionList;

/* User */
typedef struct User {
    char user_name[STRING_SIZE];
    char user_pass[STRING_SIZE];
    int is_logged_in;
} User;

/* User list */
typedef struct UserList {
    User user;
    struct UserList *next;
} UserList;

User list[50];

/* It will initialize game settings before game starts */
extern void init_game(GameSettings **game_settings);

/* It will play move */
extern int play_game(int **game_table, char **player, int **move);

/* Create new session */
extern int create_session(char **player);

/* Join session */
extern int join_session(char **player);

/* Prints main menu */
extern void print_menu();

/* Prints list of all game rooms */
extern void list_games(SessionList **game_list);

/* Prints current game */
extern void print_game();

/* Command Handler */
extern void cmd_handler(char command[STRING_SIZE]);

int main(int argc, char **argv)
{
    if (argc <= 1) {
        printf("Please give port number to listen\n");
        exit(1);
    }
    
    int a = 0;
    for (a = 0; a < 50; a++)
    {
      list[a].is_logged_in = 0;
    }

    fd_set master; //master file descriptor list
    fd_set read_fds; //copy of master file descriptor list which is temp

    struct sockaddr_in serveraddr;//server address
    struct sockaddr_in clientaddr;//client address

    int fdmax; //biggest file descriptor number
    int listener;//socket descriptor which listens client
    int newfd; //new socket which is accepted

    char buf[1024];//buffer for data
    int nbytes;//number of received bytes
    int PORT = atoi(argv[1]);
    /* for setsockopt() SO_REUSEADDR, below */
    int addrlen;
    int i;
    int yes = 1;

    FD_ZERO(&master);    //master and temp is cleared
    FD_ZERO(&read_fds);

    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {   //socket is defined TCP, and return socket descriptor for listening
        // for UDP, use DATAGRAM instead of STREAM
        printf("Socket cannot be created!!!\n");
        exit(1);    //EXIT FAILURE
    }
    printf("Socket is created...\n");

    /*if there exist such situation which is "address already in use" error message, even so use this socket */
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {    //socket,level,optname,optval,optlen are set
        printf("Socket cannot be used!!!\n");
        exit(1);    //EXIT FAILURE
    }
    printf("Socket is being used...\n");

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY; //use my IP address
    serveraddr.sin_port = htons(PORT);
    memset(& (serveraddr.sin_zero), '\0', 8);

    if (bind(listener, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) == -1) {         //socket and port is bind
        printf("Binding cannot be done!!!\n");
        exit(1);    //EXIT FAILURE
    }
    printf("Binding is done...\n");

    if (listen(listener, 45) == -1) {    //listening, 45 connection is let in buffer
        printf("Error in listener");
        exit(1);    //EXIT FAILURE
    }
    printf("Server is listening...\n");

    FD_SET(listener, &master);   //add listener to master set
    fdmax = listener; //biggest is listener now
    char user_command[STRING_SIZE];
    struct tm auth_recv_time;
    time_t recv_time_t;
    char recv_time[STRING_SIZE];
    char response[STRING_SIZE];
    for (;;) {
        read_fds = master; //copy of master
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) { //track ready sockets to read,write, supports multiplexing, read_fds is updated
            exit(1);    //EXIT FAILURE
        }
        for (i = 0; i <= fdmax; i++) { //all list is controlled
            if (FD_ISSET(i, &read_fds)) {     //if it is in temp list
                if (i == listener) { //if it is listener, there is new connection
                    addrlen = sizeof(clientaddr);

                    newfd = accept(listener, (struct sockaddr *) &clientaddr, &addrlen);   //accept connect request of client and return new socket for this connection
                    FD_SET(newfd, &master);   // add new socket to master set
                    if (newfd > fdmax)
                        fdmax = newfd; //it is biggest socket number

                } else { //if it is not listener, there is data from client
                    if ((nbytes = recv(i, &user_command, sizeof(user_command), 0)) <= 0) {     //from i. connection,socket

                        printf("Socket closed...\n");
                        close(i);    //connection,socket closed
                        FD_CLR(i, &master);   //it is removed from master set

                    } else { //if data is received from a client
                      printf("%s, %d \n", user_command, i);
                      if(list[i].is_logged_in == 0)
                      {
                        char *token = strtok(user_command, " ");
                        strcpy(list[i].user_name, token);
                        
                        token = strtok(user_command, " ");
                        strcpy(list[i].user_pass, token);
                        
                        recv_time_t = time(NULL);
                        if (recv_time_t != -1){
                          auth_recv_time = *localtime(&recv_time_t);
                          strftime(recv_time, STRING_SIZE, "%H:%M:%S", &auth_recv_time);
                        }
                        list[i].is_logged_in = 1;
                        
                        printf("%s authenticated at %s \n", list[i].user_name, recv_time);
                        sprintf(response, "%d", list[i].is_logged_in);
                        send(i, &response, sizeof(response), 0);
                      }
                      /* TODO Implement game logic */
                      /* TODO Implement session system */
                    }
                }
            }
        }
    }
    return 0;
}


