#pragma once
#include <fstream>
#include <list>
#include <ctime>
#include "kissnet.h"
#include "crossword_board.hpp"

#define CROSSWORD_PORT "3333"

class crossword_server
{
public:
    crossword_server(std::ifstream& crossword_data, const std::string& port = CROSSWORD_PORT);
    ~crossword_server();

    void start();
    void run();


private:
    // Helper functions
    void process_message(int size, int type, kissnet::tcp_socket *sender);
    void send_board(kissnet::tcp_socket *user);
    void process_update(int x, int y, char ch);
    void process_cursor(int x, int y, int d, kissnet::tcp_socket *sender);
    void process_pause(char on);
    void process_solve_word(int clue, int dir);
    void process_solve_letter(int x, int y);
    std::string make_packet(const std::string& data, int type);
    void broadcast_packet(std::string packet, kissnet::tcp_socket *sender = 0);

    void remove(kissnet::tcp_socket *sock);


    // Member Variables
    kissnet::tcp_socket servsock;
    std::list<kissnet::tcp_socket*> connsocks;
    kissnet::socket_set set;

    crossword_board board;

    std::string port;
    time_t start_time, elapsed_time;
    bool paused;

    char header[3];
    char data[256*256];
};
