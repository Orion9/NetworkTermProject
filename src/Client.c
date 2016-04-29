#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <time.h>

#define BUFSIZE 1024
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

typedef struct User {
  char user_name[STRING_SIZE];
  char password[STRING_SIZE];
} User;

/* It will initialize game settings before game starts */
extern void init_game(GameSettings **game_settings);

/* It will play move */
extern int play_game(int **game_table, char **player, int **move);

/* Prints main menu */
extern void print_menu();

/* Prints list of all game rooms */
extern void list_games(SessionList **game_list);

/* Prints current game */
extern void print_game();

int main(int argc, char **argv)
{
    if (argc <= 2) {
        printf("Usage: ./program [IP Address] [PORT]\n");
        exit(1);
    }
    char send_buf[BUFSIZE];
    int socket_fd, fdmax;

    struct sockaddr_in server_addr;

    fd_set master;
    fd_set read_fds;
    
    /* Socket creation */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd  == -1) {  
        printf("Error (1): Socket creation failed!\n");
        exit(1);
    }

    /* Network Family */
    server_addr.sin_family = AF_INET;

    /* Port */
    server_addr.sin_port = htons(atoi(argv[2]));

    /* IP Address */
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);  //sin_zero is fulled by 0

    /* Connection Request */
    int conn_result = connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));
    if (conn_result == -1) {
        printf("Error (2): Connection request has been failed!\n");
        exit(2);
    }

    /* File Descriptor Settings */
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(0, &master);   
    FD_SET(socket_fd, &master);  

    fdmax = socket_fd;
    
    int i = 0, is_user_login = 0;
    User user_info;
    time_t auth;
    struct tm auth_time;
    char *position;
    char recv_time[STRING_SIZE];
    
    /* TODO Cleanup the code a little bit after learn FD */
    while (1) {
        read_fds = master;

        /* Track and try socket */
        /*
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            printf("Error (3): Sockets cannot be multiplexed!\n");
            exit(3);
        }*/
        
        if (is_user_login != 1) {
          printf("Username: ");
          fgets(user_info.user_name, 250, stdin);
          
          /* Remove trailing new line char */
          if((position = strchr(user_info.user_name, '\n')) != NULL){
            *position = '\0';
          }
          
          printf("Password: ");
          fgets(user_info.password, 250, stdin);
          
          /* Remove trailing new line char */
          if((position = strchr(user_info.password, '\n')) != NULL){
            *position = '\0';
          }
          
          if (user_info.user_name[0] != '\0' && user_info.password[0] != '\0') {
            send(socket_fd, &user_info, sizeof(User), 0);
            
            auth = time(NULL);
            if (auth != -1){
              auth_time = *localtime(&auth);
              strftime(recv_time, STRING_SIZE, "%H:%M:%S", &auth_time);
            }
            
            /* FIXME Problem with time formatting wonder why though no idea */
            printf("%s authenticated at %s", user_info.user_name, recv_time);
            is_user_login = 1;
          }
        }
        
        /* TODO Learn dafuq is this below */
        /*
        for (i = 0; i <= fdmax; i++)
            if (FD_ISSET(i, &read_fds)) {
              /* Ready for write   
              if (i == 0) {
                printf("[list, play]: ");
                fgets(send_buf, BUFSIZE, stdin);
                 
                if (strcmp(send_buf, "quit\n") == 0) {
                  exit(0);
                /* Sending data to server 
                } else if (send(socket_fd, send_buf, strlen(send_buf), 0)) {
                   printf(">message is sended\n");
                }
            }
        }*/
    }
    printf("Good bye! \n");
    close(socket_fd);    //socket is closed
    return 0;
}

void print_menu()
{
  printf("XOX Multi Showdown! \n");
  printf("List Available Games \n");
  printf("Quit \n");
}

