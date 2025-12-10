#ifndef COMMAND_HELPERS_HPP
#define COMMAND_HELPERS_HPP

#include <string>
#include <vector>
#include "../client/ClientConnection.hpp"

// Definiciones de seguridad para respuestas numéricas
#ifndef RPL_CHANNELMODEIS
#define RPL_CHANNELMODEIS "324"
#endif
#ifndef ERR_USERSDONTMATCH
#define ERR_USERSDONTMATCH "502"
#endif
#ifndef ERR_UMODEUNKNOWNFLAG
#define ERR_UMODEUNKNOWNFLAG "501"
#endif

// Declaraciones de funciones auxiliares
void sendReply(ClientConnection* client, std::string num, std::string msg);
void sendError(ClientConnection* client, std::string num, std::string arg);
std::vector<std::string> split(const std::string &s, char delimiter);
void checkRegistration(ClientConnection* client);

#endif
