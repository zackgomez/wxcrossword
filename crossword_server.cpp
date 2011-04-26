#include "crossword_server.h"
#include <iostream>
#include <sstream>

#define BOARD_REQUEST_TYPE 1
#define BOARD_TYPE 2
#define UPDATE_TYPE 3
#define CURSOR_TYPE 4
#define WIN_TYPE 5
#define PAUSE_TYPE 6
#define SOLVE_WORD_TYPE 7
#define SOLVE_LETTER_TYPE 8

crossword_server::crossword_server(std::ifstream& crossword_data, const std::string& inport)
    : port(inport)
{
    board.read(crossword_data);
}
crossword_server::~crossword_server() { // Remove connections
    for (std::list<kissnet::tcp_socket*>::iterator it = connsocks.begin();
         it != connsocks.end(); it++)
        delete *it;
}

void crossword_server::run()
{
    // Set up listening socket
    servsock.listen(port, 2);
    set.add_socket(&servsock);
    
    for (;;)
    {
        std::vector<kissnet::tcp_socket*> active = set.poll_sockets();

        for (size_t i = 0; i < active.size(); i++)
        {
            kissnet::tcp_socket *cursock = active[i];
            if (*(active[i]) == servsock)
            {
                kissnet::tcp_socket *newsock = servsock.accept();
                set.add_socket(newsock);
                connsocks.push_back(newsock);
            }
            else
            {
                try
                {
                    int bytes_recv;
                    // Read the header
                    if ((bytes_recv = cursock->recv(header, 3)) == 0)
                    {
                        remove(cursock);
                    }
                    // Read the payload
                    else
                    {
                        int type = header[0];
                        int size = header[1] * 256 + (unsigned char) header[2];

                        process_message(size, type, cursock);
                    }
                }
                catch(kissnet::socket_exception& e)
                {
                    remove(cursock);
                }
            }
        }
    }
}

void crossword_server::send_board(kissnet::tcp_socket *sock)
{
    std::ostringstream out;
    board.write(out);

    std::string packet = make_packet(out.str(), BOARD_TYPE);
    try
    {
        if (sock->send(packet) < packet.size())
            std::cout << "Sent incomplete packet";
    }
    catch (kissnet::socket_exception& e)
    {
        remove(sock);
    }

    //std::cout << "Sent board packet of size " << packet.size() << '\n';
}

void crossword_server::process_update(int x, int y, char ch)
{
    if (paused)
        return;

    if (x >= 0 && x < board.xdim() && y >= 0 && y < board.ydim())
    {
        board.at(x, y) = ch;

        std::string data;
        data.push_back(static_cast<char>(x));
        data.push_back(static_cast<char>(y));
        data.push_back(ch);
        std::string packet = make_packet(data, UPDATE_TYPE);

        broadcast_packet(packet);

        if (board.won())
        {
            elapsed_time = time(NULL) - start_time;
            start_time = 0;

            char buffer[40];
            sprintf(buffer, "%ld", elapsed_time);

            std::cout << "THEY HAVE WON THE GAME!!!\n" <<
                "It took them " << buffer << " seconds\n";

            std::string packet = make_packet(buffer, WIN_TYPE);
            broadcast_packet(packet);
        }

        if (start_time == 0)
        {
            start_time = time(NULL);
            std::cout << "The timer has begun\n";
        }
    }
    else
        std::cout << "Got a bad update message, ignoring it.\n";
}

void crossword_server::process_cursor(int x, int y, int d, kissnet::tcp_socket *sender)
{
    if (x >= 0 && x < board.xdim() && y >= 0 && y < board.ydim())
    {
        std::string data;
        data.push_back(static_cast<char>(x));
        data.push_back(static_cast<char>(y));
        data.push_back(static_cast<char>(d));
        std::string packet = make_packet(data, CURSOR_TYPE);

        broadcast_packet(packet, sender);
    }
    else
        std::cout << "Got a bad cursor message, ignoring it.\n";
}

void crossword_server::process_pause(char on)
{
    paused = on;
    if (paused)
        std::cout << "Timer paused\n";
    else
        std::cout << "Timer unpaused\n";

    if (paused && start_time != 0)
    {
        elapsed_time += time(NULL) - start_time;
        start_time = 0;
    }
    else if (!paused && elapsed_time != 0)
        start_time = time(NULL);

    std::string data;
    data.push_back(on);
    std::string packet = make_packet(data, PAUSE_TYPE);

    broadcast_packet(packet);
}

void crossword_server::process_solve_word(int clue, int dir)
{
    if (dir != crossword_board::across_dir && dir != crossword_board::down_dir)
    {
        std::cout << "Got bad direction for solve word, ignoring\n";
        return;
    }
    //std::cout << "Solving clue " << clue << " direction " << dir << '\n';

    // Get the start of the clue
    int x, y;
    board.start_of_clue(clue, x, y);

    // Set the direction
    int xvel = 0, yvel = 0;
    if (dir == crossword_board::across_dir)
        xvel = 1;
    else
        yvel = 1;


    // Now loop until a wall or end of board
    for (; x < board.xdim() && y < board.ydim(); x += xvel, y += yvel)
    {
        // If we've hit a wall, we're done
        if (board.layout_at(x, y) == crossword_board::wall_char)
            break;

        process_solve_letter(x, y);
    }
}

void crossword_server::process_solve_letter(int x, int y)
{
    //std::cout << "Solving letter at (" << x << ',' << y << ")\n";

    if (x >= 0 && x < board.xdim() && y >= 0 && y < board.ydim())
        process_update(x, y, board.answer_at(x, y));
    else
        std::cout << "Got a bad solve letter message, ignoring it\n";
}

std::string crossword_server::make_packet(const std::string& data, int type)
{
    std::string packet;
    packet.push_back(static_cast<unsigned char>(type));
    packet.push_back(static_cast<unsigned char>(data.size() / 256));
    packet.push_back(static_cast<unsigned char>(data.size() % 256));
    packet.append(data);

    if (packet.size() > 60000)
        std::cout << "NOT sending a packet with size larger than 60000!\n";
    else
        return packet;

    return "";
}

void crossword_server::broadcast_packet(std::string packet, kissnet::tcp_socket *sender)
{
    for (std::list<kissnet::tcp_socket*>::iterator it = connsocks.begin();
         it != connsocks.end(); it++)
    {
        if (*it != sender)
        {
            //std::cout << "::Broadcast an update message!\n";
            try
            {
            (*it)->send(packet);
            }
            catch (kissnet::socket_exception &e)
            {
                remove(*it);
            }
        }
    }
}

void crossword_server::process_message(int size, int type, kissnet::tcp_socket *sender)
{
    try
    {
        // Nothing
        if (type == BOARD_REQUEST_TYPE)
        {
            //std::cout << "Recieved a board request message\n";
            if (size != 0)
                std::cout << "Got a board request message with a nonzero payload "
                    "size!\n";
            send_board(sender);
        }
        else if (type == BOARD_TYPE)
        {
            std::cout << "Recieved a board message, shouldn't have recieved this. "
                "I'm going to ignore it!!!\n";
        }
        else if (type == UPDATE_TYPE)
        {
            //std::cout << "Got an update message!\n";
            if (size != 3)
                std::cout << "The update message is the wrong size!\n";
            // Retrieve the payload
            sender->recv(data, size);

            int x = static_cast<unsigned char>(data[0]);
            int y = static_cast<unsigned char>(data[1]);
            char ch = data[2];

            //std::cout << "X: " << x << " Y: " << y << " Char: \'" << ch << "\'\n";

            process_update(x, y, ch);
        }
        else if (type == CURSOR_TYPE)
        {
            if (size != 3)
                std::cout << "The cursor message is the wrong size!\n";
            // Retrieve the payload
            sender->recv(data, size);
            //std::cout << "Got a cursor position message\n";
            int x = static_cast<unsigned char>(data[0]);
            int y = static_cast<unsigned char>(data[1]);
            int dir = static_cast<unsigned char>(data[2]);
            //std::cout << "X: " << x << " Y: " << y << " Dir: " << dir << '\n';
            process_cursor(x, y, dir, sender);
        }
        else if (type == PAUSE_TYPE)
        {
            if (size != 1)
                std::cout << "The pause message is the wrong size!\n";
            // Retrieve the payload
            sender->recv(data, size);
            std::cout << "Someone wants to pause the game\n";
            process_pause(data[0]);
        }
        else if (type == SOLVE_WORD_TYPE)
        {
            // TODO
            if (size != 2)
                std::cout << "The solve_word message is the wrong size!\n";
            // Retrieve the payload
            sender->recv(data, size);
            int clue = data[0];
            int dir  = data[1];
            process_solve_word(clue, dir);
        }
        else if (type == SOLVE_LETTER_TYPE)
        {
            if (size != 2)
                std::cout << "The solve_letter message is the wrong size!\n";
            // Retrieve the payload
            sender->recv(data, size);
            int x = data[0];
            int y = data[1];
            process_solve_letter(x, y);
        }
        else
        {
            // Retrieve the payload
            sender->recv(data, size);
            // Print out error message
            std::cout << "Got a message of unknown type: " << type <<
                "\nThe data that goes with it: ";
            std::cout.write(data, size);
            std::cout << '\n';
        }
    }
    catch (kissnet::socket_exception& e)
    {
        // Just remove that individual from the server
        remove(sender);
    }
}

void crossword_server::remove(kissnet::tcp_socket *sock)
{
    std::cout << "Someone disconnected from the server\n";

    delete sock;
    set.remove_socket(sock);
    connsocks.remove(sock);
}
