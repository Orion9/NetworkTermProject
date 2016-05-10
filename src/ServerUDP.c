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
    
    struct sockaddr_in player_one_addr;
    struct sockaddr_in player_two_addr;
    
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
    char user_ip[INET_ADDRSTRLEN];
    int user_port;
    int is_playing;
    int user_room_key;
    int is_logged_in;
    struct sockaddr_in user_addr;
} User;



User user_list[50];
User returned_list[50];
socklen_t addrlen;
Session session_list[25];

/* Command Handler */
extern void cmd_handler(int socket_fd, int t, char **command, struct sockaddr_in *clientaddr);

int main(int argc, char **argv)
{
    if (argc <= 1) {
        printf("Please give port number to listen\n");
        exit(1);
    }

    int a = 0;
    for (a = 0; a < 50; a++) {
        user_list[a].is_logged_in = 0;
        inet_pton(AF_INET, "0.0.0.0", &(user_list[a].user_addr.sin_addr));
        returned_list[a].is_logged_in = 0;
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
    struct timeval start,end;   
    double t1, t2;

    FD_ZERO(&master);    //master and temp is cleared
    FD_ZERO(&read_fds);

    if ((listener = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {   //socket is defined TCP, and return socket descriptor for listening
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
    //printf("Socket is being used...\n");

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY; //use my IP address
    serveraddr.sin_port = htons(PORT);
    memset(& (serveraddr.sin_zero), '\0', 8);

    if (bind(listener, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) == -1) {         //socket and port is bind
        printf("Binding cannot be done!!!\n");
        exit(1);    //EXIT FAILURE
    }
    printf("Binding is done...\n");

    //     if (listen(listener, 45) == -1) {    //listening, 45 connection is let in buffer
    //         printf("Error in listener \n");
    //         exit(1);    //EXIT FAILURE
    //     }
    //     printf("Server is listening...\n");

    FD_SET(listener, &master);   //add listener to master set
    fdmax = listener; //biggest is listener now

    char user_command[STRING_SIZE];
    struct tm auth_recv_time;
    time_t recv_time_t;
    char recv_time[STRING_SIZE];
    char response[STRING_SIZE];
    char client_address[INET_ADDRSTRLEN];
    char target_address[INET_ADDRSTRLEN];
    struct timeval waitThreshold;
    waitThreshold.tv_sec = 5000;
    waitThreshold.tv_usec = 50;

    for (;;) {
        // New Useré
        addrlen = sizeof(clientaddr);
        if (recvfrom(listener, &user_command, sizeof(user_command), 0, &clientaddr, &addrlen) < 0) {
            perror("get data");
        }
        printf("%s \n", user_command);

        int client_port, target_port, user_new_connection = 0, empty = 0;
        int t = 0;
        for (t = 0; t < 50; t++) {
            inet_ntop(AF_INET, &(clientaddr.sin_addr), client_address, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &(user_list[t].user_addr.sin_addr), target_address, INET_ADDRSTRLEN);
            target_port = ntohs(user_list[t].user_addr.sin_port);
            client_port = ntohs(clientaddr.sin_port);

            if (strncmp(client_address, target_address, INET_ADDRSTRLEN) == 0 && target_port == client_port) {
                printf("I exist so I am \n");
                user_new_connection = 0;
                break;
            } else if (user_list[t].user_name[0] == '\0') {
                user_new_connection = 1;
                printf("Yo mama, im on TV \n");
                break;
            } else if (t == 49) {
                printf("User limit reached! \n");
                user_new_connection = 2;
            }
        }

        if (user_new_connection == 1) {

            inet_ntop(AF_INET, &(clientaddr.sin_addr), client_address, INET_ADDRSTRLEN);
            inet_pton(AF_INET, client_address, &(user_list[t].user_addr.sin_addr));

            strcpy(user_list[t].user_ip, client_address);
            user_list[t].user_port = ntohs(clientaddr.sin_port);

            printf("%d: %s:%d \n", t, user_list[t].user_ip, user_list[t].user_port);

            printf("t: %d \n", t);

            /* Connection succeeded. */
            if (user_list[t].is_logged_in == 0) {
                char tmp_name[STRING_SIZE], tmp_pass[STRING_SIZE];
                char *token = strtok(user_command, " ");
                strcpy(tmp_name, token);

                token = strtok(NULL, " ");
                strcpy(tmp_pass, token);

                int is_found = 0;
                int a = 0;
                for (a = 0; a < 50; a++) {
                    if (strncmp(returned_list[a].user_name, tmp_name, STRING_SIZE) == 0) {
                        is_found = 1;
                        break;
                    } else {
                        is_found = 0;
                    }
                }

                if (is_found == 1) {
                    printf("Player is returning... \n");
                    if (strncmp(returned_list[a].user_name, tmp_name, STRING_SIZE) == 0
                            && strncmp(returned_list[a].user_pass, tmp_pass, STRING_SIZE) == 0) {
                        strcpy(user_list[t].user_name, returned_list[a].user_name);
                        strcpy(user_list[t].user_pass, returned_list[a].user_pass);

                        user_list[t].user_addr.sin_port = clientaddr.sin_port;
                        user_list[t].user_addr.sin_family = clientaddr.sin_family;
                        memset(& (user_list[t].user_addr.sin_zero), '\0', 8);

                        recv_time_t = time(NULL);
                        if (recv_time_t != -1) {
                            auth_recv_time = *localtime(&recv_time_t);
                            strftime(recv_time, STRING_SIZE, "%H:%M:%S", &auth_recv_time);
                        }
                        user_list[t].is_logged_in = 1;

                        printf("%s authenticated at %s \n", user_list[t].user_name, recv_time);
                    } else {
                        user_list[t].is_logged_in = 0;
                    }
                } else {
                    printf("A new player! \n");
                    for (a = 0; a < 50; a++) {
                        if (returned_list[a].user_name[0] == '\0') {
                            break;
                        }
                    }
                    
                    user_list[t].user_addr.sin_port = clientaddr.sin_port;
                    user_list[t].user_addr.sin_family = clientaddr.sin_family;
                    memset(& (user_list[t].user_addr.sin_zero), '\0', 8);

                    strcpy(returned_list[a].user_name, tmp_name);
                    strcpy(returned_list[a].user_pass, tmp_pass);

                    strcpy(user_list[t].user_name, returned_list[a].user_name);
                    strcpy(user_list[t].user_pass, returned_list[a].user_pass);

                    recv_time_t = time(NULL);
                    if (recv_time_t != -1) {
                        auth_recv_time = *localtime(&recv_time_t);
                        strftime(recv_time, STRING_SIZE, "%H:%M:%S", &auth_recv_time);
                    }

                    printf("%s authenticated at %s \n", user_list[t].user_name, recv_time);
                    user_list[t].is_logged_in = 1;
                }

                socklen_t addr_len = sizeof(struct sockaddr);
                sprintf(response, "%d", user_list[t].is_logged_in);

                inet_ntop(AF_INET, &(user_list[t].user_addr.sin_addr), target_address, INET_ADDRSTRLEN);
                target_port = ntohs(user_list[t].user_addr.sin_port);

                printf("GG: %d: %s:%d \n", t, target_address, target_port);

                if (sendto(listener, &response, sizeof(response), 0, (struct sockaddr *)&(user_list[t].user_addr), addr_len) == -1) {
                    perror("main");
                }


            }
        } else {
          cmd_handler(listener, t, &user_command, &user_list[t].user_addr);
        }
        /*read_fds = master; //copy of master
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) { //track ready sockets to read,write, supports multiplexing, read_fds is updated
          exit(1);    //EXIT FAILURE
        }*/
//         for (i = 0; i <= fdmax; i++) { //all list is controlledcommand
//             if (FD_ISSET(i, &read_fds)) {     //if it is in temp list
//
//             }
//         }
    }
    return 0;
}

void cmd_handler(int socket_fd, int t,char **command, struct sockaddr_in *clientaddr)
{
    char tmp_command[STRING_SIZE];
    strcpy(tmp_command, command);
    addrlen = sizeof(struct sockaddr);

    char *parsed_command = strtok(tmp_command, " ");

    if (strncmp(parsed_command, "list", STRING_SIZE) == 0) {
        int i = 0;
        for (i = 0; i < 25; i++) {
            if (session_list[i].room_name[0] != '\0') {
                printf("%s \n", session_list[i].room_name);
            }
        }

        if (sendto(socket_fd, &session_list, sizeof(session_list), 0, (struct sockaddr *)clientaddr, addrlen) < 0)
        {
          perror("list");
        }
        
    } else if (strncmp(parsed_command, "new", STRING_SIZE) == 0) {
        strcpy(tmp_command, command);
        char *token = strtok(tmp_command, " ");
        token = strtok(NULL, " ");

        printf("Player1 is trying to create %s \n", token);

        int i = 0;
        for (i = 0; i < 25; i++) {
            if (session_list[i].room_name[0] == '\0') {
                break;
            }
        }

        strcpy(session_list[i].room_name, token);
        session_list[i].room_key = i;
        user_list[t].user_room_key = i;
        session_list[i].room_full = 0;
        session_list[i].game_settings.winner = 0;
        strcpy(session_list[i].game_settings.player_one_name, user_list[t].user_name);
        //session_list[i].game_settings.player_one_socket = socket_fd;
        session_list[i].game_settings.player_one_addr.sin_family = (*clientaddr).sin_family;
        session_list[i].game_settings.player_one_addr.sin_addr = (*clientaddr).sin_addr;
        session_list[i].game_settings.player_one_addr.sin_port = (*clientaddr).sin_port;

        struct tm auth_recv_time;
        time_t recv_time_t;
        char recv_time[STRING_SIZE];
        recv_time_t = time(NULL);
        if (recv_time_t != -1) {
            auth_recv_time = *localtime(&recv_time_t);
            strftime(recv_time, STRING_SIZE, "%H:%M:%S", &auth_recv_time);
        }

        printf("%s created the Game Session %d at %s \n", user_list[t].user_name, session_list[i].room_key, recv_time);

        //sendto(socket_fd, &session_list[i], sizeof(session_list[i]), 0, (struct sockaddr *)&clientaddr, addrlen);
    } else if (strncmp(parsed_command, "join", STRING_SIZE) == 0) {
        strcpy(tmp_command, command);
        char *token = strtok(tmp_command, " ");
        token = strtok(NULL, " ");

        printf("Player2 is trying to join %s \n", token);

        int i = 0;
        for (i = 0; i < 25; i++) {
            if (strncmp(token, session_list[i].room_name, STRING_SIZE) == 0) {
                if (session_list[i].room_full == 1) {
                    //Abort and send room full message
                    Session tmp_session;
                    tmp_session = session_list[i];
                    tmp_session.room_name[0] = '\0';
                    if (sendto(socket_fd, &tmp_session, sizeof(tmp_session), 0, (struct sockaddr *)clientaddr, addrlen) < 0)
                    {
                      perror("join 0");
                    }
                    printf("%s tried to join a full game.\n", user_list[t].user_name);
                } else {
                    session_list[i].game_settings.turn = 0;
                    session_list[i].room_full = 1;
                    user_list[t].user_room_key = session_list[i].room_key;
                    strcpy(session_list[i].game_settings.player_two_name, user_list[t].user_name);
                    //session_list[i].game_settings.player_two_socket = socket_fd;
                    session_list[i].game_settings.player_two_addr.sin_family = (*clientaddr).sin_family;
                    session_list[i].game_settings.player_two_addr.sin_addr = (*clientaddr).sin_addr;
                    session_list[i].game_settings.player_two_addr.sin_port = (*clientaddr).sin_port;

                    struct tm auth_recv_time;
                    time_t recv_time_t;
                    char recv_time[STRING_SIZE];
                    recv_time_t = time(NULL);
                    if (recv_time_t != -1) {
                        auth_recv_time = *localtime(&recv_time_t);
                        strftime(recv_time, STRING_SIZE, "%H:%M:%S", &auth_recv_time);
                    }

                    printf("%s joined the Game Session %d at %s \n", user_list[t].user_name, session_list[i].room_key, recv_time);
                    
                    if (sendto(socket_fd, &session_list[i], sizeof(session_list[i]), 0, (struct sockaddr *)&(session_list[i].game_settings.player_one_addr), addrlen) < 0)
                    {
                      perror("join 1");
                    }
                    if (sendto(socket_fd, &session_list[i], sizeof(session_list[i]), 0, (struct sockaddr *)&(session_list[i].game_settings.player_two_addr), addrlen) < 0)
                    {
                      perror("join 2");
                    }
                }
            }
        }
    } else if (strncmp(parsed_command, "move", STRING_SIZE) == 0) {
        strcpy(tmp_command, command);
        char *token = strtok(tmp_command, " ");

        token = strtok(NULL, " ");
        int x = atoi(token);
        token = strtok(NULL, " ");
        int y = atoi(token);

        printf("Player wants to move to %d %d poisiton \n", x, y);

        int i = 0;
        for (i = 0; i < 25; i++) {
            if (user_list[t].user_room_key == session_list[i].room_key)
                break;
        }

        printf("Game Session %s \n", session_list[i].room_name);

        if (session_list[i].game_settings.turn == 0)
            session_list[i].game_settings.turn = 1;

        if (session_list[i].game_settings.turn == 1) {
            printf("%s has made a move \n", session_list[i].game_settings.player_one_name);
            session_list[i].game_settings.table[x][y] = 1;
        } else if (session_list[i].game_settings.turn == 2) {
            printf("%s has made a move \n", session_list[i].game_settings.player_two_name);
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
        for (a = 0; a < 3; a++) {
            for (b = 0; b < 3; b++) {
                if (session_list[i].game_settings.table[a][b] == 1)
                    h_1_count = h_1_count + 1;
                else if (session_list[i].game_settings.table[a][b] == 2)
                    h_2_count = h_2_count + 1;
            }
            if (h_1_count == 3)
                session_list[i].game_settings.winner = 1;
            if (h_2_count == 3)
                session_list[i].game_settings.winner = 2;
            h_1_count = 0;
            h_2_count = 0;
        }
        //Check Vertical Lines
        for (a = 0; a < 3; a++) {
            for (b = 0; b < 3; b++) {
                if (session_list[i].game_settings.table[b][a] == 1)
                    h_1_count = h_1_count + 1;
                else if (session_list[i].game_settings.table[b][a] == 2)
                    h_2_count = h_2_count + 1;
            }
            if (h_1_count == 3)
                session_list[i].game_settings.winner = 1;
            if (h_2_count == 3)
                session_list[i].game_settings.winner = 2;
            h_1_count = 0;
            h_2_count = 0;
        }
        //Check Diagonal Lines
        if ((session_list[i].game_settings.table[0][0] == 1) &&
                (session_list[i].game_settings.table[1][1] == 1) &&
                (session_list[i].game_settings.table[2][2] == 1))
            session_list[i].game_settings.winner = 1;

        if ((session_list[i].game_settings.table[0][0] == 2) &&
                (session_list[i].game_settings.table[1][1] == 2) &&
                (session_list[i].game_settings.table[2][2] == 2))
            session_list[i].game_settings.winner = 2;

        if ((session_list[i].game_settings.table[0][2] == 1) &&
                (session_list[i].game_settings.table[1][1] == 1) &&
                (session_list[i].game_settings.table[2][0] == 1))
            session_list[i].game_settings.winner = 1;

        if ((session_list[i].game_settings.table[0][2] == 2) &&
                (session_list[i].game_settings.table[1][1] == 2) &&
                (session_list[i].game_settings.table[2][0] == 2))
            session_list[i].game_settings.winner = 2;

        int check_draw = 0;
        if (session_list[i].game_settings.winner == 0) {
            for (a = 0; a < 3; a++) {
                for (b = 0; b < 3; b++) {
                    if (session_list[i].game_settings.table[a][b] != 0)
                        check_draw = check_draw + 1;
                }
            }
        }

        if (check_draw == 9) {
            session_list[i].game_settings.winner = 3;
        }

        if (session_list[i].game_settings.turn == 1) {
            session_list[i].game_settings.turn = 2;
            sendto(socket_fd, &session_list[i], sizeof(session_list[i]), 0, (struct sockaddr *)&(session_list[i].game_settings.player_two_addr), addrlen);
            if (session_list[i].game_settings.winner > 0) {
                session_list[i].game_settings.turn = 1;
                sendto(socket_fd, &session_list[i], sizeof(session_list[i]), 0, (struct sockaddr *)&(session_list[i].game_settings.player_one_addr), addrlen);
            }
        } else if (session_list[i].game_settings.turn == 2) {
            session_list[i].game_settings.turn = 1;
            sendto(socket_fd, &session_list[i], sizeof(session_list[i]), 0, (struct sockaddr *)&(session_list[i].game_settings.player_one_addr), addrlen);
            if (session_list[i].game_settings.winner > 0) {
                session_list[i].game_settings.turn = 2;
                sendto(socket_fd, &session_list[i], sizeof(session_list[i]), 0, (struct sockaddr *)&(session_list[i].game_settings.player_two_addr), addrlen);
            }
        }

        if (session_list[i].game_settings.winner > 0) {
            Session tmp_session;
            tmp_session.room_name[0] = '\0';
            session_list[i] = tmp_session;

        }
    }
}

