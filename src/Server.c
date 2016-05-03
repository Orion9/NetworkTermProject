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

User user_list[50];
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
extern void cmd_handler(int socket_fd, char **command);

int main(int argc, char **argv)
{
    if (argc <= 1) {
        printf("Please give port number to listen\n");
        exit(1);
    }
    
    int a = 0;
    for (a = 0; a < 50; a++)
    {
        user_list[a].is_logged_in = 0;
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
        for (i = 0; i <= fdmax; i++) { //all list is controlledcommand
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
                      
                      /* FIXME Disable already logged in option for closed sockets */
                      if(user_list[i].is_logged_in == 0)
                      {
                        char *token = strtok(user_command, " ");
                        strcpy(user_list[i].user_name, token);
                        printf("user_list[i].user_name: %s \n", user_list[i].user_name);
                        
                        token = strtok(NULL, " ");
                        strcpy(user_list[i].user_pass, token);
                        printf("user_list[i].user_pass: %s \n", user_list[i].user_pass);
                        
                        recv_time_t = time(NULL);
                        if (recv_time_t != -1){
                          auth_recv_time = *localtime(&recv_time_t);
                          strftime(recv_time, STRING_SIZE, "%H:%M:%S", &auth_recv_time);
                        }
                        user_list[i].is_logged_in = 1;
                        
                        printf("%s authenticated at %s \n", user_list[i].user_name, recv_time);
                        sprintf(response, "%d", user_list[i].is_logged_in);
                        send(i, &response, sizeof(response), 0);
                      }
                      else 
                      {
                        cmd_handler(i, &user_command);
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

void cmd_handler(int socket_fd, char **command)
{
  char tmp_command[STRING_SIZE];
  strcpy(tmp_command, command);
  
  char *parsed_command = strtok(tmp_command, " ");
  
  if (strncmp(parsed_command, "list", STRING_SIZE) == 0)
  {  
    int i = 0;
    for (i = 0; i < 25; i++)
    {
      if(session_list[i].room_name[0] != '\0')
      {
        printf("%s \n", session_list[i].room_name);
      }
    }
    
    send(socket_fd, &session_list, sizeof(session_list), 0);
  }
  else if (strncmp(parsed_command, "new", STRING_SIZE) == 0)
  {
    strcpy(tmp_command, command);
    char *token = strtok(tmp_command, " ");
    token = strtok(NULL, " ");
    
    printf("Player1 is trying to create %s \n",token);
    
    int i = 0;
    for (i = 0; i < 25; i++)
    {
      if(session_list[i].room_name[0] == '\0')
      {
        break;
      }
    }
    
    strcpy(session_list[i].room_name, token);
    session_list[i].room_key = i;
    user_list[socket_fd].user_room_key = i;
    session_list[i].room_full = 0;
    session_list[i].game_settings.winner = 0;
    strcpy(session_list[i].game_settings.player_one_name, user_list[socket_fd].user_name);
    session_list[i].game_settings.player_one_socket = socket_fd;
    
    printf("%s created room %d \n", user_list[socket_fd].user_name, session_list[i].room_key);
    
    //send(socket_fd, &session_list[i], sizeof(session_list[i]), 0);
  }
  else if (strncmp(parsed_command, "join", STRING_SIZE) == 0)
  {
    strcpy(tmp_command, command);
    char *token = strtok(tmp_command, " ");
    token = strtok(NULL, " ");
    
    printf("Player2 is trying to join %s \n",token);
    
    int i = 0;
    for (i = 0; i < 25; i++)
    {
      if(strncmp(token, session_list[i].room_name, STRING_SIZE) == 0)
      {
        if(session_list[i].room_full == 1)
        {
          //Abort and send room full message
        }
        else
        {
          printf("ROOMAN: %s \n", session_list[i].room_name);
          printf("ROOMAN: %d \n", session_list[i].game_settings.winner);
          session_list[i].room_full = 1;
          user_list[socket_fd].user_room_key = session_list[i].room_key;
          strcpy(session_list[i].game_settings.player_two_name, user_list[socket_fd].user_name);
          
          printf("%s joined room %d \n", user_list[socket_fd].user_name, session_list[i].room_key);
          
          session_list[i].game_settings.turn = 0;
          session_list[i].room_full = 1;
          user_list[socket_fd].user_room_key = session_list[i].room_key;
          strcpy(session_list[i].game_settings.player_two_name, user_list[socket_fd].user_name);
          session_list[i].game_settings.player_two_socket = socket_fd;
          
          printf("%s joined room %d \n", user_list[socket_fd].user_name, session_list[i].room_key);
          
          send(session_list[i].game_settings.player_one_socket, &session_list[i], sizeof(session_list[i]), 0);
          send(session_list[i].game_settings.player_two_socket, &session_list[i], sizeof(session_list[i]), 0);
        }
      }
    }
  }
  else if (strncmp(parsed_command, "move", STRING_SIZE) == 0)
  {
    strcpy(tmp_command, command);
    char *token = strtok(tmp_command, " ");
    
    token = strtok(NULL, " ");
    int x = atoi(token);
    token = strtok(NULL, " ");
    int y = atoi(token);
    
    printf("Player wants to move to %d %d poisiton \n", x, y);
    
    int i = 0;
    for (i = 0; i < 25; i++)
    {
      if(user_list[socket_fd].user_room_key == session_list[i].room_key)
        break;
    }
    
    printf("Game Session %s \n", session_list[i].room_name);
    
    if(session_list[i].game_settings.turn == 0)
      session_list[i].game_settings.turn = 1;
    
    if(session_list[i].game_settings.turn == 1)
    {
      printf("%s has made a move \n",session_list[i].game_settings.player_one_name);
      session_list[i].game_settings.table[x][y] = 1;
    }
    else if(session_list[i].game_settings.turn == 2)
    {
      printf("%s has made a move \n",session_list[i].game_settings.player_two_name);
      session_list[i].game_settings.table[x][y] = 2;
    }
    
    //Check Victory
    int a = 0;
    int b = 0;
    int v_1_count = 0;
    int h_1_count = 0;
    int v_2_count = 0;
    int h_2_count = 0;
   
    //Check Horizontal Lines
    for(a = 0; a < 3; a++){
      for(b = 0; b < 3; b++){
        if(session_list[i].game_settings.table[a][b] == 1)
          h_1_count = h_1_count + 1;
        else if(session_list[i].game_settings.table[a][b] == 2)  
          h_2_count = h_2_count + 1;
      }
      if(h_1_count == 3)
        session_list[i].game_settings.winner = 1;
      if(h_2_count == 3)
        session_list[i].game_settings.winner = 2;
      h_1_count = 0;
      h_2_count = 0;
    }
    //Check Vertical Lines
    for(a = 0; a < 3; a++){
      for(b = 0; b < 3; b++){
        if(session_list[i].game_settings.table[b][a] == 1)
          h_1_count = h_1_count + 1;
        else if(session_list[i].game_settings.table[b][a] == 2)  
          h_2_count = h_2_count + 1;
      }
      if(h_1_count == 3)
        session_list[i].game_settings.winner = 1;
      if(h_2_count == 3)
        session_list[i].game_settings.winner = 2;
      h_1_count = 0;
      h_2_count = 0;
    }
    //Check Diagonal Lines
    if((session_list[i].game_settings.table[0][0] == 1) &&
      (session_list[i].game_settings.table[1][1] == 1) &&
      (session_list[i].game_settings.table[2][2] == 1))
      session_list[i].game_settings.winner = 1;
    
    if((session_list[i].game_settings.table[0][0] == 2) &&
      (session_list[i].game_settings.table[1][1] == 2) &&
      (session_list[i].game_settings.table[2][2] == 2))
      session_list[i].game_settings.winner = 2;
    
    if((session_list[i].game_settings.table[0][2] == 1) &&
      (session_list[i].game_settings.table[1][1] == 1) &&
      (session_list[i].game_settings.table[2][0] == 1))
      session_list[i].game_settings.winner = 1;
    
    if((session_list[i].game_settings.table[0][2] == 2) &&
      (session_list[i].game_settings.table[1][1] == 2) &&
      (session_list[i].game_settings.table[2][0] == 2))
      session_list[i].game_settings.winner = 2;
    
    int check_draw = 0;
    if(session_list[i].game_settings.winner == 0 ){
      for(a = 0; a < 3; a++){
        for(b = 0; b < 3; b++){
          if(session_list[i].game_settings.table[a][b] != 0)
           check_draw = check_draw + 1;
        }
      }
    }
    
    if(check_draw == 9){
      session_list[i].game_settings.winner = 3;
    }    
    
    if(session_list[i].game_settings.turn == 1)
    {
      session_list[i].game_settings.turn = 2;
      send(session_list[i].game_settings.player_two_socket, &session_list[i], sizeof(session_list[i]), 0);
      if(session_list[i].game_settings.winner > 0){
        session_list[i].game_settings.turn = 1;
        send(session_list[i].game_settings.player_one_socket, &session_list[i], sizeof(session_list[i]), 0);
      }
    }
    else if(session_list[i].game_settings.turn == 2)
    {
      session_list[i].game_settings.turn = 1;
      send(session_list[i].game_settings.player_one_socket, &session_list[i], sizeof(session_list[i]), 0);
      if(session_list[i].game_settings.winner > 0){
        session_list[i].game_settings.turn = 2;
        send(session_list[i].game_settings.player_two_socket, &session_list[i], sizeof(session_list[i]), 0);
      }
    }
    
    if(session_list[i].game_settings.winner > 0){
      Session tmp_session;
      tmp_session.room_name[0] = '\0';
      session_list[i] = tmp_session;
      
    }
  }
}






