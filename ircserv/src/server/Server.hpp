/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: miaviles <miaviles@student.42madrid>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 15:55:08 by miaviles          #+#    #+#             */
/*   Updated: 2025/12/08 16:51:52 by miaviles         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <poll.h>

class ClientConnection;
class Channel;

/**
 * Server: IRC Server main coordinator
 * 
 * Responsibilities:
 * - Main poll() loop (SINGLE poll as required by 42)
 * - ClientConnection lifecycle management
 * - Event routing to appropriate handlers
 * - Channel management
 * - Command execution coordination
 * 
 * Uses:
 * - SocketUtils for low-level socket operations
 * - ClientConnection for connection + User state
 */

class Server {
	public:
		Server(int port, const std::string& password);
		~Server();

		//* MAIN CONTROLLERS
		bool start(); 								//* Create Socket, bind, listen
		void run(); 								//* Loop poll()/select()
		void stop();
	
		//* GETTERS
		const std::string& getPassword() const;
		int getClientCount() const;
		
	private:
		//* CONFIGURATION
		int port_;
		std::string password_;
		int server_fd_; 							//* FD OF THE SERVER'S SOCKET
		bool running_;

		//* COLLECTIONS
		std::vector<ClientConnection*> clients_; 	//* STORAGE THE LIST OF CLIENTS
		std::vector<Channel*> channels_; 			//* STORAGE THE LIST OF CHANNELS
		std::vector<struct pollfd> poll_fds_; 		//* POOLS FUCTION

		//* INITIALIZATION
		bool setupServerSocket();

		//* CONECTION MANAGEMENT
		void acceptNewConnections();
    	void handleClientEvent(size_t poll_index);
   		void disconnectClient(size_t poll_index);

		//* COMMAND PROCESSING (for later)
		void processClientCommands(ClientConnection* client);
		void sendPendingData(ClientConnection* client);
		
		//* UTILITIES
		void addClientToPoll(ClientConnection* client);
		void updatePollEvents(int fd, short events);
		ClientConnection* findClientByFd(int fd);

		//* NON-COPYABLE
		Server(const Server&);
		Server& operator=(const Server&);
};

#endif