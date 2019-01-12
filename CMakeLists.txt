cmake_minimum_required(VERSION 3.8)
project(Hangman_Game)

set(CMAKE_CXX_STANDARD 11)

add_executable(Hangman_Game server.cpp client.cpp)