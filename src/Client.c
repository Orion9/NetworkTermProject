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

/* 
 * Session list, total of 25 sessions are supported
 * But it can be increase. 
 */
Session session_list[25];

/* Prints main menu */
extern void print_menu();

/* Command Handler */
int cmd_handler(int socket_fd, char **command);

User user_info;

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
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd  == -1) {
        printf("Error (1): Socket creation failed!\n");
        exit(1);
    } else {
        printf("Socket is created...\n");
    }

    /* Network Family */
    server_addr.sin_family = AF_INET;

    /* Port */
    server_addr.sin_port = htons(atoi(argv[2]));

    /* IP Address */
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);  //sin_zero is fulled by 0

    /* Connection Request */
    printf("Clients sends connection request to server...\n");
    int conn_result = connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));
    if (conn_result == -1) {
        printf("Error (2): Connection request has been failed!\n");
        exit(2);
    } else {
        printf("Connection established...\n");
    }

    /* File Descriptor Settings */
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(0, &master);
    FD_SET(socket_fd, &master);

    fdmax = socket_fd;

    int i = 0, is_user_login = 0;
    
    time_t auth;
    struct tm auth_time;
    char *position;
    char recv_time[STRING_SIZE];
    char command[STRING_SIZE];
    char response[STRING_SIZE];

    int success = 0;
    while (1) {
        read_fds = master;

        if (is_user_login != 1) {
            printf("Username and Password: ");
            fgets(command, 250, stdin);

            /* Remove trailing new line char */
            if ((position = strchr(command, '\n')) != NULL) {
                *position = '\0';
            }

            if (command[0] != '\0') {
                send(socket_fd, &command, sizeof(command), 0);
                int nbytes;
                if ((nbytes = recv(socket_fd, &response, sizeof(response), 0)) <= 0) {
                    printf("No connection with server \n");
                    exit(1);
                } else {
                    if (atoi(response) == 0) {
                        printf("Wrong pass or name! \n");
                    }
                    is_user_login = atoi(response);
                    if (is_user_login == 1) {
                        auth = time(NULL);
                        if (auth != -1) {
                            auth_time = *localtime(&auth);
                            strftime(recv_time, STRING_SIZE, "%H:%M:%S", &auth_time);
                        }
                        char *username = strtok(command, " ");
                        printf("%s authenticated at %s \n", username, recv_time);
                    }
                }
                sleep(1);
            }
        } else {
            print_menu();
            fgets(command, STRING_SIZE, stdin);

            /* Remove trailing new line char */
            if ((position = strchr(command, '\n')) != NULL) {
                *position = '\0';
            }

            if (strncmp(command, "quit", STRING_SIZE) == 0) {
                break;
            }

            send(socket_fd, &command, sizeof(command), 0);
            success = cmd_handler(socket_fd, &command);

        }
    }
    printf("Good bye! \n");

    /* User Exited */
    close(socket_fd);
    return 0;
}

void print_menu()
{
    system("\n");
    printf("XOX Multi Showdown! \n");
    printf("Use 'new [Session Name]' to Create New Session \n");
    printf("Use 'list' to See Available Sessions \n");
    printf("Use 'join [Session Name]' to Join a Session \n");
    printf("Use 'quit' to Quit \n");
    printf("[new, list, join, quit]: ");
}

int cmd_handler(int socket_fd, char **command)
{
    char tmp_command[STRING_SIZE];
    strcpy(tmp_command, command);

    char *parsed_command = strtok(tmp_command, " ");

    if (strncmp(parsed_command, "list", STRING_SIZE) == 0) {
        int nbytes;
        Session tmp_list[25];
        system("\n");

        int i = 0;
        if ((nbytes = recv(socket_fd, &session_list, sizeof(session_list), 0)) > 0) {
            for (i = 0; i < 25; i++) {
                if (session_list[i].room_name[0] != '\0') {
                    printf("%s \n", session_list[i].room_name);
                }
            }
        }
    } else if (strncmp(parsed_command, "new", STRING_SIZE) == 0) {
        strcpy(tmp_command, command);
        char *room_name = strtok(tmp_command, " ");
        room_name = strtok(NULL, " ");

        printf("Created room: %s \n", room_name);

        int nbytes;
        char *join_message;
        Session tmp_session;
        while (1) {
            if (nbytes = recv(socket_fd, &tmp_session, sizeof(tmp_session), 0) <= 0) {
                printf("RIP Conn. \n");
                exit(1);
            } else {
                if (tmp_session.room_full == 1) {
                    if (tmp_session.game_settings.winner != 0) {
                        printf("Game Ended %d\n", tmp_session.game_settings.winner);
                        if (tmp_session.game_settings.winner == 1)
                            printf("You Win!\n");
                        else if (tmp_session.game_settings.winner == 2)
                            printf("You Lost!\n");
                        else if (tmp_session.game_settings.winner == 3)
                            printf("Draw!\n");
                        break;
                    }
                    if (tmp_session.game_settings.turn == 0) {
                        //printf("Another player has joined your game \n");
                        struct tm auth_recv_time;
                        time_t recv_time_t;
                        char recv_time[STRING_SIZE];
                        recv_time_t = time(NULL);
                        if (recv_time_t != -1) {
                            auth_recv_time = *localtime(&recv_time_t);
                            strftime(recv_time, STRING_SIZE, "%H:%M:%S", &auth_recv_time);
                        }
                        printf("%s joined at %s \n\n", tmp_session.game_settings.player_two_name, recv_time);
                        printf("Game is starting\n");
                        tmp_session.game_settings.turn = 1;
                    }
                    if (tmp_session.game_settings.turn == 1) {
                        int i = 0;
                        int j = 0;
                        for (i = 0; i < 3; i++) {
                            for (j = 0; j < 3; j++) {
                                if (tmp_session.game_settings.table[i][j] == 1) {
                                    printf("X");
                                } else if (tmp_session.game_settings.table[i][j] == 2) {
                                    printf("O");
                                } else {
                                    printf(" ");
                                }
                                printf(" | ");
                            }
                            printf("\n");
                        }
                        char move_command[STRING_SIZE];
                        char tmp_test[STRING_SIZE];
                        int check_move = 0;
                        while (check_move == 0) {
                            check_move = 1;

                            printf("[move [Col] [Row]]: ");
                            fgets(move_command, STRING_SIZE, stdin);
                            /* Remove trailing new line char */
                            char *position;
                            if ((position = strchr(move_command, '\n')) != NULL) {
                                *position = '\0';
                            }

                            /* Test move */
                            strcpy(tmp_test, move_command);
                            char *test = strtok(tmp_test, " ");
                            test = strtok(NULL, " ");
                            int x = atoi(test);
                            test = strtok(NULL, " ");
                            int y = atoi(test);

                            if (x < 0 || y > 2 || y < 0 || x > 2) {
                                printf("Impossible Move \n");
                                check_move = 0;
                            }
                            if (tmp_session.game_settings.table[x][y] != 0) {
                                printf("Impossible Move \n");
                                check_move = 0;
                            }
                        }

                        send(socket_fd, &move_command, sizeof(move_command), 0);

                    }
                }


            }
        }
    } else if (strncmp(parsed_command, "join", STRING_SIZE) == 0) {
        strcpy(tmp_command, command);
        char *room_name = strtok(tmp_command, " ");
        room_name = strtok(NULL, " ");

        printf("Joined room: %s \n", room_name);
        int nbytes, is_game_finished = 0;
        Session tmp_session;
        char move_command[STRING_SIZE];

        while (1) {
            if (nbytes = recv(socket_fd, &tmp_session, sizeof(tmp_session), 0) <= 0) {
                printf("RIP Conn. \n");
                exit(1);
            } else {
                if (tmp_session.room_full == 1) {
                    if (tmp_session.room_name[0] == '\0')
                    {
                      printf("The room you are trying to join is full \n");
                      break;
                    }
                    if (tmp_session.game_settings.winner) {
                        if (tmp_session.game_settings.winner == 1)
                            printf("You Lose!\n");
                        else if (tmp_session.game_settings.winner == 2)
                            printf("You Win!\n");
                        else if (tmp_session.game_settings.winner == 3)
                            printf("Draw!\n");
                        break;
                    }
                    if (tmp_session.game_settings.turn == 0) {
                        //printf("You have joined a game \n");
                        struct tm auth_recv_time;
                        time_t recv_time_t;
                        char recv_time[STRING_SIZE];
                        recv_time_t = time(NULL);
                        if (recv_time_t != -1) {
                            auth_recv_time = *localtime(&recv_time_t);
                            strftime(recv_time, STRING_SIZE, "%H:%M:%S", &auth_recv_time);
                        }
                        printf("%s joined at %s \n\n", tmp_session.game_settings.player_two_name, recv_time);
                        printf("Game is starting \n");
                    } else if (tmp_session.game_settings.turn == 2) {
                        int i = 0;
                        int j = 0;
                        for (i = 0; i < 3; i++) {
                            for (j = 0; j < 3; j++) {
                                if (tmp_session.game_settings.table[i][j] == 1) {
                                    printf("X");
                                } else if (tmp_session.game_settings.table[i][j] == 2) {
                                    printf("O");
                                } else {
                                    printf(" ");
                                }
                                printf(" | ");
                            }
                            printf("\n");
                        }

                        char move_command[STRING_SIZE];
                        char tmp_test[STRING_SIZE];
                        int check_move = 0;
                        while (check_move == 0) {
                            check_move = 1;

                            printf("[move [Col] [Row]]: ");
                            fgets(move_command, STRING_SIZE, stdin);
                            /* Remove trailing new line char */
                            char *position;
                            if ((position = strchr(move_command, '\n')) != NULL) {
                                *position = '\0';
                            }

                            /* Test move */
                            strcpy(tmp_test, move_command);
                            char *test = strtok(tmp_test, " ");
                            test = strtok(NULL, " ");
                            int x = atoi(test);
                            test = strtok(NULL, " ");
                            int y = atoi(test);

                            if (x < 0 || y > 2 || y < 0 || x > 2) {
                                printf("Impossible Move \n");
                                check_move = 0;
                            }
                            if (tmp_session.game_settings.table[x][y] != 0) {
                                printf("Impossible Move \n");
                                check_move = 0;
                            }
                        }

                        send(socket_fd, &move_command, sizeof(move_command), 0);
                    }
                }


            }
        }
    }
    return 1;
}
