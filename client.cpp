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
#include <unordered_set>
#include <string>
#include <sys/time.h>

#define SNDBUFSIZE 512
#define RCVBUFSIZE 512

//#define DEBUG

using std::cin;
using std::cout;
using std::endl;
using std::unordered_set;
using std::string;

int main(int argc, char *argv[]){

    /* Check input parameters */
    if(argc != 3){
        perror("Invalid input parameter! Correct input format: ./NAME server_IP port_number");
        exit(1);
    }

    int client_socket;                   // Socket descriptor
    int word_length;                     // Length of game word
    int incorrect_guess = 0;             // Number of incorrect guess
    int msg_flg;                         // Message header: msg_flag
    struct sockaddr_in server_addr;      // Server address structure
    char sndBuf[SNDBUFSIZE];             // Sender buffer
    char rcvBuf[RCVBUFSIZE];             // Receiver buffer
    char* server_IP;                     // Server IP address
    char guess_char;                     // Client guess character
    string buffer = "";                  // Receiver buffer
    string s_in;                         // Receive user's input from keyboard
    string guess_word = "";              // Current guess word
    unordered_set<char> crt_guess;       // Correct guessed characters
    string incrt_guess = "";             // Incorrect guessed characters
    unsigned short server_port;          // Port number

    memset(&sndBuf, 0, SNDBUFSIZE);
    memset(&rcvBuf, 0, RCVBUFSIZE);

    /* Initialize parameters */
    server_IP = argv[1];
    server_port = atoi(argv[2]);

    /* Create a new TCP socket */
    if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket initialization failure!");
        exit(1);
    }

    /* Construct the server address structure */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_IP);
    server_addr.sin_port = htons(server_port);

    /* Establish connection to the server */
    if(connect(client_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0){
        perror("Connection failure!");
        exit(1);
    }

    recv(client_socket, rcvBuf, RCVBUFSIZE, 0);
    msg_flg = int(rcvBuf[0]);
#ifdef DEBUG
    cout << "msg_flg: " << msg_flg << endl;
#endif
    for(int cnt = 0; cnt < msg_flg; cnt++){
        buffer += rcvBuf[1 + cnt];
    }
    if(buffer != "Server-overloaded!"){
        cout << "Connect successfully to server!" << endl;
    }else{
        cout << "There is no enough client slots available! The maximum supported concurrent clients number is 3" << endl;
        close(client_socket);
        return 0;
    }

    /* Ask user whether to start the game */
    while(1){
        cout << "Two Player? (y/n)" << endl;
        cin >> s_in;
        if(s_in == "n"){
            memset(&sndBuf, 0, SNDBUFSIZE);
            strcpy(sndBuf, "_");
            sndBuf[0] = 0 & 0xFF;
            send(client_socket, sndBuf, sizeof(sndBuf), 0);
            break;
        }else if(s_in == "y"){
            memset(&sndBuf, 0, SNDBUFSIZE);
            strcpy(sndBuf, "_");
            sndBuf[0] = 2 & 0xFF;
            send(client_socket, sndBuf, sizeof(sndBuf), 0);
            break;
        }else{
            cout << "Invalid input! please reenter your choice" << endl;
        }
    }

    while(1){
        buffer = guess_word = incrt_guess = "";
        memset(&rcvBuf, 0, RCVBUFSIZE);
        recv(client_socket, rcvBuf, RCVBUFSIZE, 0);
        msg_flg = int(rcvBuf[0]);
        if(msg_flg != 0){
            for(int cnt = 0; cnt < msg_flg; cnt++){
                buffer += rcvBuf[1 + cnt];
            }
            if(buffer == "Game Over!"){
                cout << buffer <<endl;
                cout << "Disconnected from server!" << endl;
                close(client_socket);
                return 0;
            }else if(buffer == "Waiting for other player!"){
                cout << buffer << endl;
            }else if(buffer == "Game Starting!"){
                cout << buffer << endl;
            }else if(buffer == "Your Turn!"){
                cout << buffer << endl;
            }else if(buffer == "Correct!"){
                cout << buffer << endl;
            }else if(buffer == "Incorrect!"){
                cout << buffer << endl;
            }else if(buffer == "You Win!"){
                cout << buffer << endl;
            }else if(buffer == "You Lose!"){
                cout << buffer << endl;
            }else{
                cout << buffer << endl;
            }
        }else{
#ifdef DEBUG
            cout << "buffer: " << buffer << endl;
            cout << "msg_flg: " << msg_flg << endl;
            cout << "buffer[1]: " << int(rcvBuf[1]) << " | " << rcvBuf[1] - '0' << endl;
            cout << "buffer[2]: " << int(rcvBuf[2]) << " | " << (int)rcvBuf[2] << endl;
#endif
            for(int cnt = 0; cnt < (int)rcvBuf[1]; cnt++){
                guess_word += rcvBuf[3 + cnt];
            }
            for(int cnt = 0; cnt < guess_word.length(); cnt++){
                if(isalpha(guess_word[cnt])){
                    crt_guess.insert(guess_word[cnt]);
                }
            }
#ifdef DEBUG
            cout << "guess_word: " << guess_word << endl;
#endif
            for(int cnt = 0; cnt < (int)rcvBuf[2]; cnt++){
                incrt_guess += rcvBuf[3 + (int)rcvBuf[1] + cnt];
            }

            for(int cnt = 0; cnt < guess_word.length(); cnt++){
                cout << guess_word[cnt] << " ";
            }
            cout << endl;
            cout << "Incorrect Guesses: ";
            for(int i = 0; i < incrt_guess.length(); i++){
                cout << incrt_guess[i] << " ";
            }
            cout << endl;
            if(guess_word.find('_') == guess_word.npos || incrt_guess.length() == 6){
                continue;
            }
            while(1){
                s_in = "";
                cout << "Letter to guess: ";
                cin >> s_in;
                if(s_in.length() != 1){
                    cout << "Error! Please guess one letter." << endl;
                }else if(isalpha(s_in[0])){
                    if((incrt_guess.find(s_in[0]) != incrt_guess.npos) || (crt_guess.find(s_in[0]) != crt_guess.end())){
                        cout << "Error! Letter " << s_in[0] << " has been guessed before, please guess another letter" << endl;
                        continue;
                    }else if(isupper(s_in[0])){
                        guess_char = tolower(s_in[0]);
                    }else{
                        guess_char = s_in[0];
                    }
                    memset(&sndBuf, 0, SNDBUFSIZE);
                    strcpy(sndBuf, "__");
                    sndBuf[0] = 1 & 0xFF;
                    sndBuf[1] = guess_char;
                    send(client_socket, sndBuf, sizeof(sndBuf), 0);
                    cout << endl;
                    break;
                }else{
                    cout << "Error! Please guess one letter." << endl;
                }
            }

        }
    }

}


