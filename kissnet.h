#pragma once
#include <string>
#include <exception>
#include <vector>
#include <list>

namespace kissnet
{

void init_networking();
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
class socket_exception
    : public std::exception
{
public:
    socket_exception(const std::string& what, bool include_syserr = false);

    ~socket_exception() throw();

    const char * what() const throw();

private:
    std::string msg;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class tcp_socket
{
public:
    tcp_socket();
    tcp_socket(int sockfd);

    ~tcp_socket();

    void connect(const std::string& addr, const std::string& port);
    void close();

    void listen(const std::string& port, int backlog);
    tcp_socket * accept();

    int  send(const std::string& data);
    int  recv(char* buffer, int buffer_len);

    bool operator==(const tcp_socket& rhs) const;

    // Bare metal functions
    int  getSocket() const;

private:
    int sock;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class socket_set
{
public:
    socket_set();
    ~socket_set();

    void add_socket(tcp_socket* sock);
    void remove_socket(tcp_socket* sock);

    std::vector<tcp_socket*> poll_sockets();
private:
    std::list<tcp_socket*> socks;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
};
