//
// Created by carson on 5/20/20.
//

#include "stdio.h"
#include "stdlib.h"
#include "server.h"
#include "char_buff.h"
#include "game.h"
#include "repl.h"
#include "pthread.h"
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h>    //inet_addr
#include<unistd.h>    //write

static game_server *SERVER;

void init_server() {
    if (SERVER == NULL) {
        SERVER = calloc(1, sizeof(struct game_server));
    } else {
        printf("Server already started");
    }
}

int handle_client_connect(int player) {
    // STEP 8 - This is the big one: you will need to re-implement the REPL code from
    // the repl.c file, but with a twist: you need to make sure that a player only
    // fires when the game is initialized and it is there turn.  They can broadcast
    // a message whenever, but they can't just shoot unless it is their turn.
    //
    // The commands will obviously not take a player argument, as with the system level
    // REPL, but they will be similar: load, fire, etc.
    //
    // You must broadcast informative messages after each shot (HIT! or MISS!) and let
    // the player print out their current board state at any time.
    //
    // This function will end up looking a lot like repl_execute_command, except you will
    // be working against network sockets rather than standard out, and you will need
    // to coordinate turns via the game::status field.
    int client_socket = SERVER->player_sockets[player];
    char raw_input[1000];
    char_buff *input_buff = cb_create(2000);
    char_buff *output_buff = cb_create(2000);
    int read_size, opposing = (player == 0) ? 1 : 0;;
    cb_append(output_buff, "Welcome to the Battlebit");
    cb_append_int(output_buff, player);
    cb_append(output_buff, "\nbattleBit (? for help) > ");
    cb_write(client_socket, output_buff);
    while ((read_size = recv(client_socket, raw_input, 1000, 0)) > 0) {
        cb_reset(output_buff);
        cb_reset(input_buff);
        raw_input[read_size] = '\0';
        cb_append(input_buff, raw_input);
        char *command = cb_tokenize(input_buff, " \r\n");
        if (command) {
            char *arg1 = cb_next_token(input_buff);
            char *arg2 = cb_next_token(input_buff);

            char *arg3 = cb_next_token(input_buff);

            if (strcmp(command, "help") == 0) {
                cb_append(output_buff, "good luck");
                cb_write(client_socket, output_buff);
            } else if (strcmp(command, "exit") == 0) {
                close(client_socket);
            } else if (strcmp(command, "fire") == 0) {
                if (game_get_current()->status == CREATED) {
                    cb_append(output_buff, "Game hasn't started");
                    cb_append(output_buff, "\nbattleBit (? for help) > ");
                    cb_write(client_socket, output_buff);
                } else {
                    if (player == 0 && game_get_current()->status == PLAYER_0_TURN ||
                        player == 1 && game_get_current()->status == PLAYER_1_TURN) {
                        cb_append(output_buff, "\nPlayer ");
                        cb_append_int(output_buff, player);
                        cb_append(output_buff, " fires at ");
                        cb_append_int(output_buff, arg1[0] - '0');
                        cb_append(output_buff, " ");
                        cb_append_int(output_buff, arg2[0] - '0');
                        if (game_fire(game_get_current(), player, arg1[0] - '0', arg2[0] - '0') == 1) {
                            cb_append(output_buff, " - HIT");
                            if (game_get_current()->status == PLAYER_1_WINS) {
                                cb_append(output_buff, " - Player 1 Wins!");
                                game_init();
                            } else if (game_get_current()->status == PLAYER_0_WINS) {
                                cb_append(output_buff, " - Player 0 Wins!");
                                game_init();
                            }
                        } else {
                            cb_append(output_buff, " MISS");
                        }
                        cb_append(output_buff, "\nbattleBit (? for help) > ");
                        puts(output_buff->buffer);
                        cb_write(SERVER->player_sockets[opposing], output_buff);
                        memmove(output_buff->buffer, output_buff->buffer + 1, strlen(output_buff->buffer));
                        cb_write(client_socket, output_buff);
                    } else {
                        cb_append(output_buff, "Player ");
                        cb_append_int(output_buff, opposing);
                        cb_append(output_buff, " turn.");
                        cb_append(output_buff, "\nbattleBit (? for help) > ");
                        cb_write(client_socket, output_buff);
                    }
                }
            } else if (strcmp(command, "?") == 0) {
                cb_append(output_buff, "? - show help\n"
                                       "load <string> - load a ship layout\n"
                                       "show - shows the board\n"
                                       "fire [0-7] [0-7] - fires at the given position\n"
                                       "say <string> - Send the string to all players as part of a chat\n"
                                       "exit");
                cb_append(output_buff, "\nbattleBit (? for help) > ");
                cb_write(client_socket, output_buff);
            } else if (strcmp(command, "load") == 0) {
                if (game_load_board(game_get_current(), player, arg1) == 1) {
                    if (game_get_current()->players[opposing].ships == 0) {
                        cb_append(output_buff, "Waiting on Player ");
                        cb_append_int(output_buff, opposing);
                        cb_append(output_buff, "\nbattleBit (? for help) > ");
                        cb_write(client_socket, output_buff);
                    } else if (game_get_current()->players[opposing].ships != 0) {
                        cb_append(output_buff, "All player boards loaded.\nPlayer 0 Turn");
                        cb_append(output_buff, "\nbattleBit (? for help) > ");
                        cb_write(client_socket, output_buff);
                    }
                } else {
                    cb_append(output_buff, "Invalid Game Board");
                    cb_append(output_buff, "\nbattleBit (? for help) > ");
                    cb_write(client_socket, output_buff);
                }
            } else if (strcmp(command, "say") == 0) {
                cb_append(output_buff, "\nPlayer ");
                cb_append_int(output_buff, player);
                cb_append(output_buff, " says: ");
                memmove(raw_input, raw_input + 3, strlen(raw_input) - 2);
                cb_append(output_buff, raw_input);
                cb_append(output_buff, "\nbattleBit (? for help) > ");
                puts(output_buff->buffer);
                cb_write(SERVER->player_sockets[opposing], output_buff);
                cb_reset(output_buff);
                cb_append(output_buff, "\nbattleBit (? for help) > ");
                cb_write(client_socket, output_buff);
            } else if (strcmp(command, "show") == 0) {
                cb_append(output_buff, "battleBit.........\n-----[ ENEMY ]----\n");
                repl_print_hits(&game_get_current()->players[player], output_buff);
                cb_write(client_socket, output_buff);
                cb_reset(output_buff);
                cb_append(output_buff, "\n==================\n-----[ SHIPS ]----\nbattleBit.........\n");
                repl_print_ships(&game_get_current()->players[player], output_buff);
                cb_write(client_socket, output_buff);
                cb_reset(output_buff);
                cb_append(output_buff, "\nbattleBit (? for help) > ");
                cb_write(client_socket, output_buff);
            } else {
                cb_append(output_buff, "\nbattleBit (? for help) > ");
                cb_write(client_socket, output_buff);
            }
        }
        cb_reset(output_buff);
    }
}

void server_broadcast(char_buff *msg) {
    if (SERVER != NULL && msg != NULL) {
        cb_write(SERVER->player_sockets[0], msg);
        cb_write(SERVER->player_sockets[1], msg);
    }
    // send message to all players
}

int run_server() {
    // STEP 7 - implement the server code to put this on the network.
    // Here you will need to initalize a server socket and wait for incoming connections.
    //
    // When a connection occurs, store the corresponding new client socket in the SERVER.player_sockets array
    // as the corresponding player position.
    //
    // You will then create a thread running handle_client_connect, passing the player number out
    // so they can interact with the server asynchronously
    if (SERVER != NULL) {
        int server_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_socket_fd == -1)
            printf("Could not create socket\n");
        else {
            int yes = 1;
            setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

            struct sockaddr_in server;
            server.sin_family = AF_INET;

            server.sin_addr.s_addr = INADDR_ANY;
            server.sin_port = htons(9876);

            if (bind(server_socket_fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
                puts("Bind failed");
            } else {
                puts("Binded");
                listen(server_socket_fd, 2);

                puts("Waiting for incoming connections...");

                struct sockaddr_in client;
                socklen_t size_from_connect;
                int client_socket_fd, player = 0;
                while ((client_socket_fd = accept(server_socket_fd,
                                                  (struct sockaddr *) &client,
                                                  &size_from_connect)) > 0 && player < 2) {
                    SERVER->player_sockets[player] = client_socket_fd;
                    pthread_create(&SERVER->player_threads[player], NULL, (void *) handle_client_connect, player);
                    puts("Player connected");
                    player++;

                }
            }
        }
    }
}


int server_start() {
    // STEP 6 - using a pthread, run the run_server() function asynchronously, so you can still
    // interact with the game via the command line REPL
    init_server();
    if (SERVER->server_thread == 0)
        pthread_create(&SERVER->server_thread, NULL, (void *) run_server, NULL);
    else
        printf("Server Thread Already Running!");
}