/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: miaviles <miaviles@student.42madrid>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 15:55:08 by miaviles          #+#    #+#             */
/*   Updated: 2025/12/03 16:48:39 by miaviles         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>

class ClientConnection;
class Channel;

class Server {
	public:
		Server(int port, const std::string& password);
		~Server();

		bool start(); //* Create Socket, bind, listen
		void run(); //* Loop poll()/select()

	private:
		int port_;
		std::string password_;
		
		bool accept_new();
   		void handle_events();

		std::vector<ClientConnection*> clients_; //* STORAGE THE LIST OF CLIENTS
   		std::vector<Channel*> channels_; //* STORAGE THE LIST OF CHANNELS
};


#endif