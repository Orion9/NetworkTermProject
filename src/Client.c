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
  SessionList *next;
};

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

int main ( int argc, char **argv )
{
  if ( argc<=2 ) {
    printf ( "Usage: ./program [IP Address] [PORT]\n" );
    exit ( 1 );
  }
  char send_buf[BUFSIZE];
  int socket_fd, fdmax, i;
  
  struct sockaddr_in server_addr;
  
  fd_set master;
  fd_set read_fds;
  
  socket_fd=socket ( AF_INET,SOCK_STREAM,0 );
  if ( socket_fd  ==-1 ) { //socket(domain,type,protocol) is defined by TCP (SOCK_STREAM) and returned to socket descriptor
    printf ( "Socket cannot be created!!!\n" );
    exit ( 1 );
  }
  
  /* Network Family */
  server_addr.sin_family=AF_INET;
  
  /* Port */
  server_addr.sin_port=htons ( atoi ( argv[2] ) ); 
  
  /* IP Address */
  server_addr.sin_addr.s_addr=inet_addr ( argv[1] ); 
  
  memset ( server_addr.sin_zero,'\0',sizeof server_addr.sin_zero ); //sin_zero is fulled by 0
  
  /* Connection Request */
  int conn_result = connect ( socket_fd, ( struct sockaddr * ) &server_addr, sizeof ( struct sockaddr ) );
  if ( conn_result ==-1 ) { 
    printf ( "Error (2): Connection request has been failed!\n" );
    exit ( 2 );
  }
  
  /* File Descriptor Settings */
  FD_ZERO ( &master );
  FD_ZERO ( &read_fds );
  FD_SET ( 0,&master ); //0 is added to master file
  FD_SET ( sockfd,&master ); //sockfd is added to master file
  
  fdmax = socket_fd;
  
  while ( 1 ) {
    read_fds = master;
    if ( select ( fdmax + 1,&read_fds,NULL,NULL,NULL ) ==-1 ) { //track ready sockets to read,write, supports multiplexing, read_fds is updated
      printf ( "Error (3): Sockets cannot be multiplexed!\n" );
      exit ( 3 );
    }
    for ( i=0; i<=fdmax; i++ )
      if ( FD_ISSET ( i,&read_fds ) ) {
        if ( i==0 ) { //ready for write
          fgets ( send_buf,BUFSIZE,stdin );
          
          if ( strcmp ( send_buf,"quit\n" ) ==0 ) {
            exit ( 0 );
          } else if ( send ( socket_fd,send_buf,strlen ( send_buf ),0 ) ) //data is sent to server
            printf ( ">message is sended\n" );
        }
      }
      
  }
  printf ( "client-quited\n" );
  close ( socket_fd ); //socket is closed
  return 0;
}
