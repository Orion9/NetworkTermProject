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
  EMPTY, X, O
} GameSigns;

typedef struct GameSettings {
  int table[FIELD_SIZE][FIELD_SIZE];
  char player_one_name[STRING_SIZE];
  char player_two_name[STRING_SIZE];
  int player_one_socket;
  int player_two_socket;
  int turn;
  int winner;
} GameSettings;

/* Game room information */
typedef struct Session {
  int room_key;
  char room_name[STRING_SIZE];
  int room_full;
  int game_ended;
  GameSettings game_settings;
} Session;

/* User */
typedef struct User {
  char user_name[STRING_SIZE];
  char user_pass[STRING_SIZE];
  int is_playing;
  int user_room_key;
  int is_logged_in;
} User;

/* User list */
typedef struct UserList {
  User user;
  struct UserList *next;
} UserList;

Session session_list[25];

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
        
        if (strncmp(command, "quit", STRING_SIZE) == 0)
        {
          break;
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
  char tmp_command[STRING_SIZE];
  strcpy(tmp_command, command);
  
  char *parsed_command = strtok(tmp_command, " ");
  
  if (strncmp(parsed_command, "list", STRING_SIZE) == 0)
  {
    int nbytes;
    Session tmp_list[25];
    system("\n");
    
    int i = 0;
    if ((nbytes = recv(socket_fd, &session_list, sizeof(session_list), 0)) > 0 )
    {  
      for (i = 0; i < 25; i++)
      {
        if(session_list[i].room_name[0] != '\0')
        {
          printf("%s \n", session_list[i].room_name);
        }
      }
    }
  }
  else if (strncmp(parsed_command, "new", STRING_SIZE) == 0)
  {
    strcpy(tmp_command, command);
    char *room_name = strtok(tmp_command, " ");
    room_name = strtok(NULL, " ");
    
    printf("Created room: %s \n", room_name);
    
    int nbytes;
    char *join_message;
    Session tmp_session;
    while ( 1 )
    {
      if (nbytes = recv(socket_fd, &tmp_session, sizeof(tmp_session), 0) <= 0)
      {
        printf("Conn gg. \n");
      }
      else
      {
        if (tmp_session.room_full == 1)
        {
          if(tmp_session.game_settings.turn == 0)
          {
            printf("Another player has joined your game \n");
            tmp_session.game_settings.turn = 1;
          }
          if(tmp_session.game_settings.turn == 1)
          {
            int i = 0;
            int j = 0;
            for(i = 0; i < 3; i++)
            {
              for (j = 0; j < 3; j++)
              {
                if (tmp_session.game_settings.table[i][j] == 1)
                {
                  printf("X");
                }
                else if (tmp_session.game_settings.table[i][j] == 2)
                {
                  printf("O");
                }
                else
                {
                  printf(" ");
                }
                printf(" | ");
              }
              printf("\n");
            }
            char move_command[STRING_SIZE];
            char tmp_test[STRING_SIZE];
            int check_move = 0;
            while(check_move == 0){
              check_move = 1;
              
              printf("[move [Col] [Row]]: ");
              fgets(move_command, STRING_SIZE, stdin);
              /* Remove trailing new line char */
              char *position;
              if((position = strchr(move_command, '\n')) != NULL){
                *position = '\0';
              }
              
              /* Test move */
              strcpy(tmp_test, move_command);
              char *test = strtok(tmp_test, " ");
              test = strtok(NULL, " ");
              int x = atoi(test);
              test = strtok(NULL, " ");
              int y = atoi(test);
              
              if(x < 0 || x > 2 || y < 0 || x > 2 )
              {
                printf("Impossible Move");
                check_move = 0;
              }
              if(tmp_session.game_settings.table[x][y] != 0)
              {
                printf("Impossible Move");
                check_move = 0;
              }
            }
            
            send(socket_fd, &move_command, sizeof(move_command), 0);
          }
        }
        
        if (tmp_session.game_settings.winner != 0)
        {
          printf("Game Ended %d\n", tmp_session.game_settings.winner);
          if(tmp_session.game_settings.winner == 1)
            printf("You Win!");
          else if(tmp_session.game_settings.winner == 2)
            printf("You Lost!");
          else if(tmp_session.game_settings.winner == 3)
            printf("Draw!");
          break;
        }
      }
    }
  }
  else if (strncmp(parsed_command, "join", STRING_SIZE) == 0)
  {
    strcpy(tmp_command, command);
    char *room_name = strtok(tmp_command, " ");
    room_name = strtok(NULL, " ");
    
    printf("Joined room: %s \n", room_name);
    int nbytes, is_game_finished = 0;
    Session tmp_session;
    char move_command[STRING_SIZE];
    
    while (1)
    {
      if (nbytes = recv(socket_fd, &tmp_session, sizeof(tmp_session), 0) <= 0)
      {
        printf("Conn gg. \n");
      }
      else
      {
        if (tmp_session.room_full == 1)
        {
          if(tmp_session.game_settings.turn == 0)
            printf("You have joined a game \n");
          else if(tmp_session.game_settings.turn == 2)
          {
            int i = 0;
            int j = 0;
            for(i = 0; i < 3; i++)
            {
              for (j = 0; j < 3; j++)
              {
                if (tmp_session.game_settings.table[i][j] == 1)
                {
                  printf("X");
                }
                else if (tmp_session.game_settings.table[i][j] == 2)
                {
                  printf("O");
                }
                else
                {
                  printf(" ");
                }
                printf(" | ");
              }
              printf("\n");
            }
            
            char move_command[STRING_SIZE];
            char tmp_test[STRING_SIZE];
            int check_move = 0;
            while(check_move == 0){
              check_move = 1;
              
              printf("[move [Col] [Row]]: ");
              fgets(move_command, STRING_SIZE, stdin);
              /* Remove trailing new line char */
              char *position;
              if((position = strchr(move_command, '\n')) != NULL){
                *position = '\0';
              }
              
              /* Test move */
              strcpy(tmp_test, move_command);
              char *test = strtok(tmp_test, " ");
              test = strtok(NULL, " ");
              int x = atoi(test);
              test = strtok(NULL, " ");
              int y = atoi(test);
              
              if(x < 0 || x > 2 || y < 0 || x > 2 )
              {
                printf("Impossible Move");
                check_move = 0;
              }
              if(tmp_session.game_settings.table[x][y] != 0)
              {
                printf("Impossible Move");
                check_move = 0;
              }
            }
            
            send(socket_fd, &move_command, sizeof(move_command), 0);
          }
        }
        
        if (tmp_session.game_settings.winner )
        {
          if(tmp_session.game_settings.winner == 1)
            printf("You Lose!");
          else if(tmp_session.game_settings.winner == 2)
            printf("You Win!");
          else if(tmp_session.game_settings.winner == 3)
            printf("Draw!");
          break;
        }
      }
    }
  }
  return 1;
}
