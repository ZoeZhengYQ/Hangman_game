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
    struct sockaddr_in server_addr;      // Server address structure
    char sndBuf[SNDBUFSIZE];             // Sender buffer
    char rcvBuf[RCVBUFSIZE];             // Receiver buffer
    char* server_IP;                     // Server IP address
    char guess_char;                     // Client guess character
    char msg_flg;                        // Message header: msg_flag
    string buffer;                       // Receiver buffer
    string s_in;                         // Receive user's input from keyboard
    string guess_word;                   // Current guess word
    unordered_set<char> crt_guess;       // Correct guessed characters
    string incrt_guess;                  // Incorrect guessed characters
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
    cout << "first received information: " << rcvBuf << endl;
    msg_flg = rcvBuf[0];
#ifdef DEBUG
    cout << "msg_flg: " << int(msg_flg) << endl;
#endif
    buffer = rcvBuf;
    buffer = buffer.substr(1, int(msg_flg));
    if(buffer != "server-overloaded"){
        cout << "Connect successfully to server!" << endl;
    }else{
        cout << "There is no enough client slots available! The maximum supported concurrent clients number is 3" << endl;
        close(client_socket);
        return 0;
    }

    /* Ask user whether to start the game */
    while(1){
        cout << "Ready to start game? (y/n)" << endl;
        cin >> s_in;
        if(s_in == "y"){
            memset(&sndBuf, 0, SNDBUFSIZE);
            strcpy(sndBuf, "0");
            send(client_socket, sndBuf, sizeof(sndBuf), 0);
            break;
        }else if(s_in == "n"){
            memset(&sndBuf, 0, SNDBUFSIZE);
            strcpy(sndBuf, "3");
            send(client_socket, sndBuf, sizeof(sndBuf), 0);
            close(client_socket);
            return 0;
        }else{
            cout << "Invalid input! please reenter your choice" << endl;
        }
    }

    while(1){
        memset(&rcvBuf, 0, RCVBUFSIZE);
        recv(client_socket, rcvBuf, RCVBUFSIZE, 0);
        buffer = rcvBuf;
        msg_flg = buffer[0];
        if(msg_flg != '0'){
            buffer = buffer.substr(1, msg_flg - '0');
            cout << buffer <<endl;
            cout << "Game Over!" << endl;
            memset(&sndBuf, 0, SNDBUFSIZE);
            strcpy(sndBuf, "3");
            send(client_socket, sndBuf, sizeof(sndBuf), 0);
            close(client_socket);
            return 0;
        }else{
#ifdef DEBUG
            cout << "buffer: " << buffer << endl;
            cout << "buffer[1]: " << buffer[1] - '0' << endl;
            cout << "buffer[2]: " << buffer[2] - '0' << endl;
#endif
            guess_word = buffer.substr(3, buffer[1] - '0');
            for(int cnt = 0; cnt < guess_word.length(); cnt++){
                if(isalpha(guess_word[cnt])){
                    crt_guess.insert(guess_word[cnt]);
                }
            }
#ifdef DEBUG
            cout << "guess_word: " << guess_word << endl;
#endif
            if(buffer[2] == '0') {
                incrt_guess = "";
            } else {
                incrt_guess = buffer.substr(3 + int(buffer[1] - '0'), buffer[2] - '0');
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
            while(1){
                s_in = "";
                cout << "Letter to guess: " << endl;
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
                    sndBuf[0] = '1';
                    sndBuf[1] = guess_char;
                    send(client_socket, sndBuf, sizeof(sndBuf), 0);
                    break;
                }else{
                    cout << "Error! Please guess one letter." << endl;
                }
            }
        }
    }

}


