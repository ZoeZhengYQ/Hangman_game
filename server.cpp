//
// Created by Leonard on 2018/11/5.
//
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <sys/time.h>

#define RCVBUFSIZE 512
#define SNDBUFSIZE 512
#define MIN(a,b) a < b ? a : b

//#define DEBUG

using std::cout;
using std::endl;
using std::vector;
using std::string;

int main(int argc, char *argv[]){

    /* Check input parameters */
    if(argc != 2){
        perror("Invalid input parameter! Correct input format: ./NAME port_numer");
        exit(1);
    }

    bool waiting_flg = false;            // Whether there is a client waiting for other player
    bool guess_status = false;           // Whether guess is right
    bool player_one[6];                  // Whether client is the main player in two player mode
    int waiting_socket = -1;             // Currently awaiting client
    int server_socket;                   // Server Socket
    int client_socket[6];                // Client Socket
    int new_socket;                      // Incoming client socket
    int client_count = 0;                // Number of clients
    int max_client = 6;                  // Max number of clients
    int max_sd;                          // Highest file descriptor number
    int sd;                              // Socket descriptor
    int activity;                        // Socket activity
    int opt = 1;                         // Used in server socket setting
    int word_index;                      // Used to choose game word randomly from all word candidates
    int msg_length;                      // Client message length in message header
    int part[6];                         // For two player mode to record one player's partner
    char guess_char;                     // Client guess character
    int correct_count[6];                // Number of correctly guessed character
    char sndBuf[SNDBUFSIZE];             // Sender buffer
    char rcvBuf[RCVBUFSIZE];             // Receiver buffer
    string base = "_";                   // Base line character
    vector<string> buffer(6, "");        // Receiver buffer
    vector<string> word(6, "");          // Game word
    vector<string> guess_word(6, "");    // Client guessed word
    vector<string> incrt_guess(6, "");   // Client incorrect guess
    vector<string> dict({"friend", "family", "socket",
                           "program", "apple", "grape",
                           "network", "computer", "father",
                           "dog", "green", "angry",
                           "elephant", "split", "goose"
                           });
    unsigned short server_port;   // Server port
    unsigned int server_addr_len;
    struct sockaddr_in server_addr;
    for(int i = 0; i < 6; i++){
        player_one[i] = false;
        client_socket[i] = 0;
        part[i] = -1;
        correct_count[i] = 0;
    }

    memset(&rcvBuf, 0, RCVBUFSIZE);
    memset(&sndBuf, 0, SNDBUFSIZE);

    /* Set of socket descriptors */
    fd_set fd_list;

    /* Create server socket */
    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Server socket initialization failure!");
        exit(1);
    }

    /* Set server socket to allow multiple connections */
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0){
        perror("Server socket setting failure!");
        exit(1);
    }

    /* Construct local address structure */
    server_port = atoi(argv[1]);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);

    /* Bind to local address structure */
    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0){
        perror("Server socket binding failure!");
        exit(1);
    }

    /* Listen for incoming connections */
    if(listen(server_socket, 5) < 0){
        perror("Listen phase failure!");
        exit(1);
    }

    /* Accept the incoming connection */
    server_addr_len = sizeof(server_addr);

    while(1){
        /* Clear the socket set */
        FD_ZERO(&fd_list);

        /* Add server socket to set */
        FD_SET(server_socket, &fd_list);
        max_sd = server_socket;

        /* Add client sockets to set */
        for(int i = 0; i < max_client; i++){
            sd = client_socket[i];

            //if valid client socket then add to set
            if(sd > 0){
                FD_SET(sd, &fd_list);
            }
            //highest file descriptor number
            if(sd > max_sd){
                max_sd = sd;
            }
        }

        /* Wait for an activity on one of the sockets */
        activity = select(max_sd + 1, &fd_list, NULL, NULL, NULL);
        if((activity < 0) && (errno != EINTR)){
            perror("Select operation failure!");
        }

        /* If something happened on the server socket then it is an incoming connection */
        if(FD_ISSET(server_socket, &fd_list)){
            if((new_socket = accept(server_socket, (struct sockaddr *)&server_addr, (socklen_t*)&server_addr_len)) < 0){
                perror("Incoming client socket connection failure!");
                exit(1);
            }

            /* Check if current client number exceeds 3 */
            if(client_count >= 3){
                memset(&sndBuf, 0, SNDBUFSIZE);
                strcpy(sndBuf, "_Server-overloaded!");
                sndBuf[0] = 18 & 0xFF;
                send(new_socket, sndBuf, sizeof(sndBuf), 0);
            }else{
                //inform user of socket number - used in send and receive commands
                printf("New connection, socket fd is %d, ip is : %s, port : %d\n" , new_socket , inet_ntoa(server_addr.sin_addr) , ntohs(server_addr.sin_port));

                //send new connection greeting msg
                memset(&sndBuf, 0, SNDBUFSIZE);
                strcpy(sndBuf, "_welcome!");
                sndBuf[0] = 8 & 0xFF;
                send(new_socket, sndBuf, sizeof(sndBuf), 0);

                //add new socket to array of sockets
                for(int i = 0; i < 6; i++){
                    if(client_socket[i] == 0){
                        client_socket[i] = new_socket;
                        break;
                    }
                }
                client_count++;
            }
        }

        // some IO operations on some other sockets (first three clients for single play mode)
        for(int i = 0; i < 6; i++){
            sd = client_socket[i];
            // check is client_socket[i] sent msg
            if(FD_ISSET(sd, &fd_list)){
#ifdef DEBUG
                cout << "sd: " << sd << endl;
#endif
                memset(&rcvBuf, 0, RCVBUFSIZE);
                recv(sd, rcvBuf, RCVBUFSIZE, 0);
                msg_length = (int)rcvBuf[0];
                if(msg_length == 0){                   //start game (single player mode), initialization
                    if(waiting_flg && client_count == 3){
                        memset(&sndBuf, 0, SNDBUFSIZE);
                        strcpy(sndBuf, "_Server-overloaded! But you can still choose two-player mode!");
                        sndBuf[0] = 60 & 0xFF;
                        send(sd, sndBuf, sizeof(sndBuf), 0);
                        memset(&sndBuf, 0, SNDBUFSIZE);
                        strcpy(sndBuf, "_Game Over!");
                        sndBuf[0] = 10 & 0xFF;
                        send(sd, sndBuf, sizeof(sndBuf), 0);

                        client_count--;
                        client_socket[i] = 0;
                        close(sd);
                        cout << "client socket closed!" << endl;
                    }
                    srand((unsigned)time(NULL));
                    word_index = rand() % 15;
                    word[i] = dict[word_index];
                    memset(&sndBuf, 0, SNDBUFSIZE);
                    for(int idx = 0; idx < word[i].length(); idx++){
                        guess_word[i] += base;
                    }
                    buffer[i] = "___" + guess_word[i] + incrt_guess[i];
                    strcpy(sndBuf, buffer[i].c_str());
                    sndBuf[0] = 0 & 0xFF;
                    sndBuf[1] = int(guess_word[i].length()) & 0xFF;
                    sndBuf[2] = int(incrt_guess[i].length()) & 0xFF;
#ifdef DEBUG
                    cout << "sndBuf[1]: " << int(sndBuf[1]) << endl;
                    cout << "sndBuf[2]: " << int(sndBuf[2]) << endl;
                    cout << "sndBuf: " << sndBuf << endl;
#endif
                    send(sd, sndBuf, sizeof(sndBuf), 0);
                }else if(msg_length == 2) {
                    if(!waiting_flg){
                        client_count--;
                        waiting_flg = true;
                        waiting_socket = i;
                        player_one[i] = true;

                        memset(&sndBuf, 0, SNDBUFSIZE);
                        strcpy(sndBuf, "_Waiting for other player!");
                        sndBuf[0] = 25 & 0xFF;
                        send(sd, sndBuf, sizeof(sndBuf), 0);
                    }else{
                        part[i] = waiting_socket;
                        part[waiting_socket] = i;

                        memset(&sndBuf, 0, SNDBUFSIZE);
                        strcpy(sndBuf, "_Game Starting!");
                        sndBuf[0] = 14 & 0xFF;
                        send(client_socket[waiting_socket], sndBuf, sizeof(sndBuf), 0);
                        send(client_socket[i], sndBuf, sizeof(sndBuf), 0);

                        memset(&sndBuf, 0, SNDBUFSIZE);
                        strcpy(sndBuf, "_Waiting on Player 1...");
                        sndBuf[0] = 22 & 0xFF;
                        send(client_socket[i], sndBuf, sizeof(sndBuf), 0);

                        memset(&sndBuf, 0, SNDBUFSIZE);
                        strcpy(sndBuf, "_Your Turn!");
                        sndBuf[0] = 10 & 0xFF;
                        send(client_socket[waiting_socket], sndBuf, sizeof(sndBuf), 0);


                        int mini = MIN(waiting_socket, i);
                        srand((unsigned)time(NULL));
                        word_index = rand() % 15;
                        word[mini] = dict[word_index];

                        for(int idx = 0; idx < word[mini].length(); idx++){
                            guess_word[mini] += base;
                        }
                        buffer[mini] = "___" + guess_word[mini] + incrt_guess[mini];

                        memset(&sndBuf, 0, SNDBUFSIZE);
                        strcpy(sndBuf, buffer[mini].c_str());
                        sndBuf[0] = 0 & 0xFF;
                        sndBuf[1] = int(guess_word[mini].length()) & 0xFF;
                        sndBuf[2] = int(incrt_guess[mini].length()) & 0xFF;
                        send(client_socket[waiting_socket], sndBuf, sizeof(sndBuf), 0);

                        waiting_flg = false;
                        waiting_socket = -1;
                    }
                }else if(msg_length == 3){              // end game
                    cout << "client socket closed!" << endl;
                    incrt_guess[i] = "";
                    guess_word[i] = "";
                    buffer[i] = "";
                    word[i] = "";
                    correct_count[i] = 0;
                    close(sd);
                    client_socket[i] = 0;
                    if(part[i] == -1 || player_one[i]){
                        client_count--;
                    }
                    part[i] = -1;
                    player_one[i] = false;
                }else if(msg_length == 1){               // Receive client's guess
                    int mini;
#ifdef DEBUG
                    cout << "part[i]: " << part[i] << endl;
#endif
                    if(part[i] != -1){
                        mini = MIN(i, part[i]);
                        guess_status = false;
                    }else{
                        mini = i;
                    }
                    guess_char = rcvBuf[1];
                    if(word[mini].find(guess_char) == word[mini].npos){
                        incrt_guess[mini] += guess_char;
                    }else{
                        guess_status = true;
                        for(int idx = 0; idx < word[mini].length(); idx++){
                            if(word[mini][idx] == guess_char){
                                guess_word[mini][idx] = guess_char;
                                correct_count[mini]++;
                            }
                        }
                    }
                    memset(&sndBuf, 0, SNDBUFSIZE);
                    if(incrt_guess[mini].length() == 6 || correct_count[mini] == word[mini].length()){      // Win or lose, game over!
                        buffer[mini] = "___" + guess_word[mini] + incrt_guess[mini];

                        strcpy(sndBuf, buffer[mini].c_str());
                        sndBuf[0] = 0 & 0xFF;
                        sndBuf[1] = int(guess_word[mini].length()) & 0xFF;
                        sndBuf[2] = int(incrt_guess[mini].length()) & 0xFF;
                        send(sd, sndBuf, sizeof(sndBuf), 0);

                        memset(&sndBuf, 0, SNDBUFSIZE);
                        if(incrt_guess[mini].length() == 6){
                            strcpy(sndBuf, "_You Lose!");
                            sndBuf[0] = 9 & 0xFF;
                            send(sd, sndBuf, sizeof(sndBuf), 0);
                            if(part[i] != -1){
                                send(client_socket[part[i]], sndBuf, sizeof(sndBuf), 0);
                            }
                        }else{
                            strcpy(sndBuf, "_You Win!");
                            sndBuf[0] = 8 & 0xFF;
                            send(sd, sndBuf, sizeof(sndBuf), 0);
                            if(part[i] != -1){
                                send(client_socket[part[i]], sndBuf, sizeof(sndBuf), 0);
                            }
                        }
                        memset(&sndBuf, 0, SNDBUFSIZE);
                        strcpy(sndBuf, "_Game Over!");
                        sndBuf[0] = 10 & 0xFF;
                        send(sd, sndBuf, sizeof(sndBuf), 0);
                        if(part[i] != -1){
                            send(client_socket[part[i]], sndBuf, sizeof(sndBuf), 0);
                        }
                        cout << "client socket closed!" << endl;
                        incrt_guess[mini] = "";
                        guess_word[mini] = "";
                        buffer[mini] = "";
                        word[mini] = "";
                        correct_count[mini] = 0;
                        close(sd);
                        if(part[i] != -1){
                            close(client_socket[part[i]]);
                            client_socket[part[i]] = 0;
                            player_one[part[i]] = false;
                            part[part[i]] = -1;
                        }
                        client_socket[i] = 0;
                        client_count--;
                        player_one[i] = false;
                        part[i] = -1;
                    } else{
                        if(part[i] != -1){
                            if(guess_status){
                                strcpy(sndBuf, "_Correct!");
                                sndBuf[0] = 8 & 0xFF;
                                send(sd, sndBuf, sizeof(sndBuf), 0);
                            }else{
                                strcpy(sndBuf, "_Incorrect!");
                                sndBuf[0] = 10 & 0xFF;
                                send(sd, sndBuf, sizeof(sndBuf), 0);
                            }
                            memset(&sndBuf, 0, SNDBUFSIZE);
                            if(player_one[i]){
                                strcpy(sndBuf, "_Waiting on Player 2...");
                            }else {
                                strcpy(sndBuf, "_Waiting on Player 1...");
                            }
                            sndBuf[0] = 22 & 0xFF;
                            send(sd ,sndBuf, sizeof(sndBuf), 0);
                            memset(&sndBuf, 0, SNDBUFSIZE);
                            strcpy(sndBuf, "_Your Turn!");
                            sndBuf[0] = 10 & 0xFF;
                            send(client_socket[part[i]], sndBuf, sizeof(sndBuf), 0);
                        }
                        memset(&sndBuf, 0, SNDBUFSIZE);
                        buffer[mini] = "___" + guess_word[mini] + incrt_guess[mini];
                        strcpy(sndBuf, buffer[mini].c_str());
                        sndBuf[0] = 0 & 0xFF;
                        sndBuf[1] = int(guess_word[mini].length()) & 0xFF;
                        sndBuf[2] = int(incrt_guess[mini].length()) & 0xFF;
                        if(part[i] != -1){
                            send(client_socket[part[i]], sndBuf, sizeof(sndBuf), 0);
                        }else{
                            send(sd, sndBuf, sizeof(sndBuf), 0);
                        }
                    }
                }
            }
        }
#ifdef DEBUG
        cout << "ITER!!" << endl;
#endif
    }
    close(server_socket);
    return 0;
}
