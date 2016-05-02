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

/*
 * BASIC LOGIC OF THE PROGRAM:
 * User sends commands to server,
 * Server interprets the commands returns response,
 * Client takes response and prints it.
 * Client always sends chars,
 * Server may send different data types,
 * So be prepared for different data types.
 */

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
typedef struct Session {
    int room_key;
    char room_name[STRING_SIZE];
    GameSettings game_settings;
} Session;

typedef struct User {
  char user_name[STRING_SIZE];
  char password[STRING_SIZE];
} User;

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
extern void list_games(Session **game_list);

/* Prints current game */
extern void print_game();

/* Command Handler */
int cmd_handler(int socket_fd, char **command);

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
    char command[STRING_SIZE];
    char response[STRING_SIZE];
    
    int success = 0;
    /* TODO Cleanup the code a little bit after learn FD */
    while (1) {
        read_fds = master;

        /* Track and try socket */
      
        /*if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            printf("Error (3): Sockets cannot be multiplexed!\n");
            exit(3);
        }*/
        
        if (is_user_login != 1) {
          printf("Username and Password: ");
          fgets(command, 250, stdin);
          
          /* Remove trailing new line char */
          if((position = strchr(command, '\n')) != NULL){
            *position = '\0';
          }
          
          if (command[0] != '\0') {
            send(socket_fd, &command, sizeof(command), 0);
            int nbytes;
            if ((nbytes = recv(socket_fd, &response, sizeof(response), 0)) <= 0)
            {
              printf("No connection with server \n");
              exit(1);
            }
            else
            {
              printf("response is %s \n", response);
              is_user_login = atoi(response);
              if (is_user_login == 1)
              {
                auth = time(NULL);
                if (auth != -1){
                  auth_time = *localtime(&auth);
                  strftime(recv_time, STRING_SIZE, "%H:%M:%S", &auth_time);
                }
                char *username = strtok(command, " ");
                printf("%s authenticated at %s \n", username, recv_time);
              }
            }
            sleep(1);
          }
        }
        print_menu();
        fgets(command, STRING_SIZE, stdin);
        
        /* Remove trailing new line char */
        if((position = strchr(command, '\n')) != NULL){
          *position = '\0';
        }
        
        send(socket_fd, &command, sizeof(command), 0);
        success = cmd_handler(socket_fd, &command);
        /* TODO Generate menu cases */
        
    }
    printf("Good bye! \n");
    close(socket_fd);    //socket is closed
    return 0;
}

void print_menu()
{
  system("\n");
  printf("XOX Multi Showdown! \n");
  printf("Use 'new' for Create New Session \n");
  printf("Use 'list' for See Available Sessions \n");
  printf("Use 'join [Session Number]' for Join a Session \n");
  printf("Use 'quit' for Quit \n");
  printf("[new, list, join, quit]: ");
}

int cmd_handler(int socket_fd, char **command)
{
  char *parsed_command = strtok(command, " ");
  printf("PARSEDCMD: %s \n", parsed_command);
  if (strncmp(parsed_command, "list", STRING_SIZE) == 0)
  {
    int nbytes;
    Session tmp_list[25];
    system("\n");
    if ((nbytes = recv(socket_fd, &tmp_list, sizeof(tmp_list), 0)) > 0 )
    {
      int i = 0;
      for (i = 0; i < 25; i++)
      {
        if(tmp_list[i].room_name[0] != '\0')
        {
          printf("%s \n", tmp_list[i].room_name);
        }
      }
    }
  }
  else if (strncmp(parsed_command, "join", STRING_SIZE) == 0)
  {
    
  }
  return 1;
}

