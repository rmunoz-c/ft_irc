/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: miaviles <miaviles@student.42madrid>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 15:52:03 by miaviles          #+#    #+#             */
/*   Updated: 2025/12/03 17:15:03 by miaviles         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <iostream>

int main(int argc, char **argv)
{
	int port;
	std::string pass;

	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>\n";
		return (1);
	}
	port = atoi(argv[1]);
	pass = argv[2];

	Server server(port, pass);
	if (!server.start())
	{
		std::cerr << "Failed to start server\n";
		return (1);
	}
	server.run(); //* MAIN LOOP
	return (0);
}