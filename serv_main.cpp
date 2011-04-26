#include "crossword_server.h"
#include <fstream>
#include <iostream>
#include "kissnet.h"

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3)
    {
        std::cout << "usage: " << argv[0] << " crossword_file [port]\n";
        return -1;
    }

    std::ifstream infile(argv[1]);
    if (!infile)
    {
        std::cout << "error opening file " << argv[1] << '\n';
        return -1;
    }

    std::string port;
    if (argc == 3)
        port = argv[2];
    else
        port = CROSSWORD_PORT;

    kissnet::init_networking();

    crossword_server serv(infile, port);
    std::cout << "Starting server\n";
    serv.run();

    return 0;
}
