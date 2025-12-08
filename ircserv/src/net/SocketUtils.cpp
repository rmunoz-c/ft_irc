/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SocketUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: miaviles <miaviles@student.42madrid>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/08 17:09:27 by miaviles          #+#    #+#             */
/*   Updated: 2025/12/08 18:03:39 by miaviles         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "SocketUtils.hpp"
#include <iostream>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/socket.h>

//* ========================================
//* SOCKET CONFIGURATION
//* ========================================

//* Makes the socket non-blocking: calls won't block waiting for data and will return immediately if nothing is available.
bool	setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0); 					//* "fcntl is used to manipulated FDs, in this case in F_GETFL mode is to see the status of FDs"
	if (flags == -1)
	{
		std::cerr << "[SOCKET] fcntl(F_GETFL) failed: " << strerror(errno) << std::endl;
		return (false);
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) 	//* "fcntl is used to manipulated FDs, in this case in F_SETFL mode is too set the status of FDs"
	{
		std::cerr << "[SOCKET] fcntl(F_SETFL, O_NONBLOCK) failed: " << strerror(errno) << std::endl;
		return (false);
	}
	return (true);
}

//* ========================================
//* SO_REUSEADDR: Bypass TIME_WAIT State
//* ========================================
//* Problem: When a server closes, the OS keeps the port in TIME_WAIT state for ~60 seconds
//*      This prevents immediate restart: "Address already in use" error
//* Solution: SO_REUSEADDR tells the OS "allow me to reuse this port immediately"
//* 
//* Example WITHOUT this option:
//*   1. Start server on port 6667 → Works ✓
//*   2. Ctrl+C to close → Server stops ✓
//*   3. Restart immediately → ERROR ✗ (must wait 60 seconds)
//*
//* Example WITH this option:
//*   1. Start server on port 6667 → Works ✓
//*   2. Ctrl+C to close → Server stops ✓
//*   3. Restart immediately → Works ✓ (no waiting!)
//*
//* Parameters:
//*   - fd: Socket file descriptor to configure
//*   - SOL_SOCKET: Apply option at socket level (not TCP/IP level)
//*   - SO_REUSEADDR: Specific option to enable address reuse
//*   - opt = 1: Enable the option (0 would disable it)
//* ========================================
bool	setReuseAddr(int fd)
{
	int opt = 1;                                      //* 1 = enable, 0 = disable

	if (setsockopt(fd, SOL_SOCKET,  SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		std::cerr << "[SOCKET] setsockopt(SO_REUSEADDR) failed: " << strerror(errno) << std::endl;
        return (false);
	}
	return (true);
}

//* ========================================
//* SERVER SOCKET CREATION
//* ========================================

int		createServerSocket()
{
	//* CREATE A SOCKET -->
	//* AF_INET = IPv4 (DOMAIN)
    //* SOCK_STREAM = TCP (TYPE)
    //* 0 = protocol by default (TCP for SOCK_STREAM)
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		std::cerr << "[SOCKET] socket() failed: " << strerror(errno) << std::endl;
		return (-1);	
	}
	std::cout << "[SOCKET] Socket created (fd=" << fd << ")" << std::endl;
	

}

bool	bindSocket(int fd, int port)
{

}

bool	listenSocket(int fd, int backlog)
{

}

//* ========================================
//* CLIENT CONNECTION HANDLING
//* ========================================

int		acceptClient(int server_fd, std::string& client_ip)
{

}

//* ========================================
//* I/O OPERATIONS
//* ========================================

ssize_t	receiveData(int fd, char* buffer, size_t size)
{

}

ssize_t	sendData(int fd, const char* data, size_t size)
{

}
	
//* ========================================
//* ERROR HANDLING
//* ========================================

bool	isWouldBlock()
{

}

std::string	getLastError()
{

}
