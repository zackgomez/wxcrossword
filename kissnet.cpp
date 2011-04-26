#include "kissnet.h"
#include <iostream>
#include <cstring> // for strerror
#include <cstdlib>
#include <errno.h>

#ifndef _MSC_VER
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netdb.h>
#else
#include <WinSock2.h>
#include <ws2tcpip.h>
#endif

namespace kissnet
{
// -----------------------------------------------------------------------------
// Utility Functions
// -----------------------------------------------------------------------------
void init_networking()
{
#ifdef _MSC_VER
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        throw socket_exception("WSAStartup failed\n");
#endif
}
// -----------------------------------------------------------------------------
// Socket Exception
// -----------------------------------------------------------------------------
socket_exception::socket_exception(const std::string& what, bool include_syserr)
    : msg(what)
{
    if (include_syserr)
    {
        msg += ": ";
        msg += strerror(errno);
    }
}

socket_exception::~socket_exception() throw()
{
    // empty
}

const char * socket_exception::what() const throw()
{
    return msg.c_str();
}
// -----------------------------------------------------------------------------

tcp_socket::tcp_socket()
{
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
}

tcp_socket::tcp_socket(int sock_fd)
    : sock(sock_fd)
{
}

tcp_socket::~tcp_socket()
{
    // Make sure we clean up
    close();
}

void tcp_socket::connect(const std::string &addr, const std::string& port)
{
    struct addrinfo *res = NULL, hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(addr.c_str(), port.c_str(), &hints, &res);

    if (::connect(sock, res->ai_addr, res->ai_addrlen) < 0)
        throw socket_exception("Unable to connect", true);
}

void tcp_socket::close()
{
#ifdef _MSC_VER
    ::closesocket(sock);
#else
    ::close(sock);
#endif
}

void tcp_socket::listen(const std::string &port, int backlog)
{
    // set reuseaddr
    char yes = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
    {
        throw socket_exception("Unable to set reuseaddr", true);
    }

    // Fill structs
    struct addrinfo *res, hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (::getaddrinfo(NULL, port.c_str(), &hints, &res) < 0)
        throw socket_exception("Unable to getaddrinfo", false);

    // Bind to local port
    if (::bind(sock, res->ai_addr, res->ai_addrlen) < 0)
        throw socket_exception("Unable to bind", true);

    // Now listen
    if (::listen(sock, backlog) < 0)
        throw socket_exception("Unable to listen", true);
}

tcp_socket * tcp_socket::accept()
{
    int newsock;
    if ((newsock = ::accept(sock, NULL, NULL)) < 0)
        throw socket_exception("Unable to accept", true);

    return new tcp_socket(newsock);
}

int tcp_socket::send(const std::string& data)
{
    int bytes_sent;
    
    if ((bytes_sent = ::send(sock, data.c_str(), data.size(), 0)) < 0)
        throw socket_exception("Unable to send", true);

    return bytes_sent;
}

int tcp_socket::recv(char *buffer, int buffer_len)
{
    int bytes_received;

    if ((bytes_received = ::recv(sock, buffer, buffer_len, 0)) < 0)
        throw socket_exception("Unable to recv", true);

    return bytes_received;
}

int tcp_socket::getSocket() const
{
    return sock;
}

bool tcp_socket::operator==(const tcp_socket &rhs) const
{
    return sock == rhs.sock;
}

// -----------------------------------------------------------------------------
// socket_set definitions
// -----------------------------------------------------------------------------
socket_set::socket_set()
    : socks()
{
    // Empty
}

socket_set::~socket_set()
{
    // Empty
}

void socket_set::add_socket(tcp_socket *sock)
{
    socks.push_back(sock);
}

void socket_set::remove_socket(tcp_socket *sock)
{
    socks.remove(sock);
}

std::vector<tcp_socket*> socket_set::poll_sockets()
{
    fd_set rset;
    FD_ZERO(&rset);

    int maxfd = -1;
    for (std::list<tcp_socket*>::iterator it = socks.begin();
         it != socks.end(); it++)
    {
        int curfd = (*it)->getSocket();
        FD_SET(curfd, &rset);
        if (curfd > maxfd)
            maxfd = curfd;
    }

    ::select(maxfd + 1, &rset, NULL, NULL, NULL);

    std::vector<tcp_socket*> ret;
    for (std::list<tcp_socket*>::iterator it = socks.begin();
         it != socks.end(); it++)
    {
        int curfd = (*it)->getSocket();
        if (FD_ISSET(curfd, &rset))
            ret.push_back(*it);
    }

    return ret;
}

// End namespace kissnet
};
