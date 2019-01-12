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

    int server_socket;                   // Server Socket
    int client_socket[6] = {0};          // Client Socket
    int new_socket;                      // Incoming client socket
    int client_count = 0;                // Number of clients
    int max_client = 6;                  // Max number of clients
    int max_sd;                          // Highest file descriptor number
    int sd;                              // Socket descriptor
    int activity;                        // Socket activity
    int opt = 1;                         // Used in server socket setting
    int word_index;                      // Used to choose game word randomly from all word candidates
    char msg_length;                     // Client message length in message header
    char guess_char;                     // Client guess character
    char sndBuf[SNDBUFSIZE];             // Sender buffer
    char rcvBuf[RCVBUFSIZE];             // Receiver buffer
    string base = "_";                   // Base line character
    vector<int> correct_count(3,0);      // Number of correctly guessed character
    vector<string> buffer(3, "");        // Receiver buffer
    vector<string> word(3, "");          // Game word
    vector<string> guess_word(3, "");    // Client guessed word
    vector<string> incrt_guess(3, "");   // Client incorrect guess
    vector<string> dict({"friend", "family", "socket",
                           "program", "apple", "grape",
                           "network", "computer", "father",
                           "dog", "green", "angry",
                           "elephant", "split", "goose"
                           });
    unsigned short server_port;   // Server port
    unsigned int server_addr_len;
    struct sockaddr_in server_addr;

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
                strcpy(sndBuf, "_server-overloaded");
                sndBuf[0] = 17 & 0xFF;
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
                for(int i = 0; i < 3; i++){
                    if(client_socket[i] == 0){
                        client_socket[i] = new_socket;
                        break;
                    }
                }
                client_count++;
            }
        }

        // some IO operations on some other sockets (first three clients for single play mode)
        for(int i = 0; i < 3; i++){
            sd = client_socket[i];
            // check is client_socket[i] sent msg
            if(FD_ISSET(sd, &fd_list)){
                cout << "sd: " << sd << endl;
                memset(&rcvBuf, 0, RCVBUFSIZE);
                recv(sd, rcvBuf, RCVBUFSIZE, 0);
                msg_length = rcvBuf[0];
                if(msg_length == '0'){                   //start game, initialization
                    srand((unsigned)time(NULL));
                    word_index = rand() % 15;
                    word[i] = dict[word_index];
                    memset(&sndBuf, 0, SNDBUFSIZE);
                    for(int idx = 0; idx < word[i].length(); idx++){
                        guess_word[i] += base;
                    }
                    buffer[i] = "___" + guess_word[i] + incrt_guess[i];
                    strcpy(sndBuf, buffer[i].c_str());
                    sndBuf[0] = '0';
                    sndBuf[1] = '0' + guess_word[i].length();
                    sndBuf[2] = '0' + incrt_guess[i].length();
                    cout << "sndBuf: " << sndBuf << endl;
                    send(sd, sndBuf, sizeof(sndBuf), 0);
                }else if(msg_length == '3'){              // end game
                    cout << "client socket closed!" << endl;
                    incrt_guess[i] = "";
                    guess_word[i] = "";
                    buffer[i] = "";
                    word[i] = "";
                    correct_count[i] = 0;
                    close(sd);
                    client_socket[i] = 0;
                    client_count--;
                }else if(msg_length == '1'){               // Receive client's guess
                    guess_char = rcvBuf[1];
                    if(word[i].find(guess_char) == word[i].npos){
                        incrt_guess[i] += guess_char;
                    }else{
                        for(int idx = 0; idx < word[i].length(); idx++){
                            if(word[i][idx] == guess_char){
                                guess_word[i][idx] = guess_char;
                                correct_count[i]++;
                            }
                        }
                    }
                    memset(&sndBuf, 0, SNDBUFSIZE);
                    if(incrt_guess[i].length() == 6){
                        strcpy(sndBuf, "9You Lose!");
                    } else if(correct_count[i] == word[i].length()){
                        strcpy(sndBuf, "8You Win!");
                    } else{
                        buffer[i] = "___" + guess_word[i] + incrt_guess[i];
                        strcpy(sndBuf, buffer[i].c_str());
                        sndBuf[0] = '0';
                        sndBuf[1] = '0' + guess_word[i].length();
                        sndBuf[2] = '0' + incrt_guess[i].length();
                    }
                    send(sd, sndBuf, sizeof(sndBuf), 0);
                }
            }
        }
        cout << "ITER!!" << endl;
    }
    close(server_socket);
    return 0;
}
