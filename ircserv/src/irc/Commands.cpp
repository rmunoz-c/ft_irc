/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: carlsanc <carlsanc@student.42madrid>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 19:22:47 by carlsanc          #+#    #+#             */
/*   Updated: 2025/12/10 20:17:41 by carlsanc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../server/Server.hpp"
#include "../client/ClientConnection.hpp"
#include "../client/User.hpp"
#include "../channel/Channel.hpp"
#include "../irc/NumericReplies.hpp"
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstdlib> // [CORRECCION] Necesario para std::atoi
#include <cstdio>  // Para sprintf si fuera necesario

// [CORRECCION] Definiciones de seguridad por si NumericReplies.hpp no se actualizó
#ifndef RPL_CHANNELMODEIS
#define RPL_CHANNELMODEIS "324"
#endif
#ifndef ERR_USERSDONTMATCH
#define ERR_USERSDONTMATCH "502"
#endif
#ifndef ERR_UMODEUNKNOWNFLAG
#define ERR_UMODEUNKNOWNFLAG "501"
#endif

/* -------------------------------------------------------------------------- */
/* UTILIDADES                                                                 */
/* -------------------------------------------------------------------------- */

// Helper para enviar respuestas numéricas formateadas
static void sendReply(ClientConnection* client, std::string num, std::string msg)
{
    std::string finalMsg = ":ft_irc " + num + " " + client->getUser()->getNickname() + " " + msg + "\r\n";
    client->queueSend(finalMsg);
}

// Helper para enviar errores con mensajes RFC más claros
static void sendError(ClientConnection* client, std::string num, std::string arg)
{
    std::string msg;
    // [CORRECCION] Mensajes más descriptivos según el error
    if (num == ERR_NEEDMOREPARAMS) msg = arg + " :Not enough parameters";
    else if (num == ERR_ALREADYREGISTRED) msg = ":Unauthorized command (already registered)";
    else if (num == ERR_PASSWDMISMATCH) msg = ":Password incorrect";
    else if (num == ERR_NONICKNAMEGIVEN) msg = ":No nickname given";
    else if (num == ERR_ERRONEUSNICKNAME) msg = arg + " :Erroneous nickname";
    else if (num == ERR_NICKNAMEINUSE) msg = arg + " :Nickname is already in use";
    else if (num == ERR_NOSUCHNICK) msg = arg + " :No such nick/channel";
    else if (num == ERR_NOSUCHCHANNEL) msg = arg + " :No such channel";
    else if (num == ERR_NOTONCHANNEL) msg = arg + " :You're not on that channel";
    else if (num == ERR_USERONCHANNEL) msg = arg + " :is already on channel";
    else if (num == ERR_CHANOPRIVSNEEDED) msg = arg + " :You're not channel operator";
    else if (num == ERR_USERSDONTMATCH) msg = ":Cannot change mode for other users";
    else if (num == ERR_UMODEUNKNOWNFLAG) msg = ":Unknown MODE flag";
    else msg = arg + " :Unknown Error";

    sendReply(client, num, msg);
}

// Helper para dividir strings (para JOIN #a,#b)
static std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Helper para verificar registro completo
static void checkRegistration(ClientConnection* client)
{
    if (client->isRegistered()) return;
    
    User* user = client->getUser();
    if (client->hasSentPass() && !user->getNickname().empty() && !user->getUsername().empty())
    {
        client->setRegistered(true);
        sendReply(client, RPL_WELCOME, ":Welcome to the FT_IRC Network " + user->getPrefix());
        std::cout << "[SERVER] User registered: " << user->getNickname() << std::endl;
    }
}

/* ========================================================================== */
/* AUTENTICACIÓN Y REGISTRO                          */
/* ========================================================================== */

void Server::cmdPass(ClientConnection* client, const Message& msg)
{
    if (msg.params.empty()) 
        return sendError(client, ERR_NEEDMOREPARAMS, "PASS");
    
    if (client->isRegistered())
        return sendError(client, ERR_ALREADYREGISTRED, "");

    if (msg.params[0] != this->password_)
    {
        sendError(client, ERR_PASSWDMISMATCH, "");
        client->closeConnection(); // Desconexión por seguridad
        return;
    }

    client->markPassReceived();
}

void Server::cmdNick(ClientConnection* client, const Message& msg)
{
    if (msg.params.empty())
        return sendError(client, ERR_NONICKNAMEGIVEN, "");

    std::string newNick = msg.params[0];

    // Validar caracteres
    if (newNick.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]{}\\|-_^") != std::string::npos)
        return sendError(client, ERR_ERRONEUSNICKNAME, newNick);

    // Verificar colisiones
    for (size_t i = 0; i < clients_.size(); ++i)
    {
        if (clients_[i] != client && clients_[i]->getUser() && clients_[i]->getUser()->getNickname() == newNick)
            return sendError(client, ERR_NICKNAMEINUSE, newNick);
    }

    // [CORRECCION] BROADCAST REAL A CANALES
    if (client->isRegistered())
    {
        std::string oldPrefix = client->getUser()->getPrefix();
        std::string notification = ":" + oldPrefix + " NICK :" + newNick + "\r\n";
        
        // 1. Enviar al propio usuario
        client->queueSend(notification);
        
        // 2. Iterar sobre todos los canales del usuario y hacer broadcast
        // NOTA: Channel::broadcast excluye al emisor, así que es perfecto aquí.
        const std::vector<Channel*>& channels = client->getUser()->getChannels();
        for (size_t i = 0; i < channels.size(); ++i)
        {
            channels[i]->broadcast(notification, client->getUser());
        }
    }

    client->getUser()->setNickname(newNick);
    checkRegistration(client);
}

void Server::cmdUser(ClientConnection* client, const Message& msg)
{
    if (client->isRegistered())
        return sendError(client, ERR_ALREADYREGISTRED, "");

    if (msg.params.size() < 4)
        return sendError(client, ERR_NEEDMOREPARAMS, "USER");

    User* user = client->getUser();
    user->setUsername(msg.params[0]);
    user->setRealname(msg.params[3]);
    
    checkRegistration(client);
}

void Server::cmdQuit(ClientConnection* client, const Message& msg)
{
    std::string reason = (msg.params.empty()) ? "Client Quit" : msg.params[0];
    (void)reason; // Se podría usar para log
    client->closeConnection();
}

void Server::cmdPing(ClientConnection* client, const Message& msg)
{
    if (msg.params.empty())
        return sendError(client, ERR_NEEDMOREPARAMS, "PING");
    
    std::string token = msg.params[0];
    client->queueSend("PONG ft_irc :" + token + "\r\n");
}

void Server::cmdPong(ClientConnection* client, const Message& msg)
{
    (void)msg;
    client->updateActivity();
}

/* ========================================================================== */
/* CANALES                                     */
/* ========================================================================== */

Channel* Server::getChannel(const std::string& name)
{
    for (size_t i = 0; i < channels_.size(); ++i)
    {
        if (channels_[i]->getName() == name)
            return channels_[i];
    }
    return NULL;
}

Channel* Server::createChannel(const std::string& name)
{
    Channel* newChan = new Channel(name);
    channels_.push_back(newChan);
    return newChan;
}

void Server::cmdJoin(ClientConnection* client, const Message& msg)
{
    if (!client->isRegistered()) return;
    if (msg.params.empty()) return sendError(client, ERR_NEEDMOREPARAMS, "JOIN");

    std::vector<std::string> targets = split(msg.params[0], ',');
    std::vector<std::string> keys;
    if (msg.params.size() > 1)
        keys = split(msg.params[1], ',');

    for (size_t i = 0; i < targets.size(); ++i)
    {
        std::string chanName = targets[i];
        std::string key = (i < keys.size()) ? keys[i] : "";

        if (chanName[0] != '#') chanName = "#" + chanName;

        Channel* channel = getChannel(chanName);
        if (!channel)
        {
            channel = createChannel(chanName);
            channel->addOperator(client->getUser());
        }

        // [CORRECCION] Verificar si ya es miembro
        if (channel->isMember(client->getUser()))
            continue; // Ignorar silenciosamente o enviar error si se prefiere

        // Validaciones
        if (channel->hasMode('i') && !channel->isInvited(client->getUser()))
        {
            sendError(client, ERR_INVITEONLYCHAN, chanName);
            continue;
        }
        if (channel->hasMode('k') && channel->getKey() != key)
        {
            sendError(client, ERR_BADCHANNELKEY, chanName);
            continue;
        }
        if (channel->hasMode('l') && channel->getUserCount() >= (size_t)channel->getLimit())
        {
            sendError(client, ERR_CHANNELISFULL, chanName);
            continue;
        }

        // Unirse
        channel->addMember(client->getUser());
        client->getUser()->joinChannel(channel);

        // Notificar JOIN
        std::string joinMsg = ":" + client->getUser()->getPrefix() + " JOIN " + chanName + "\r\n";
        channel->broadcast(joinMsg, NULL); // Enviar a TODOS (incluido uno mismo a veces, pero mejor que el cliente lo reciba del server)
        client->queueSend(joinMsg); // Asegurar que el propio cliente lo recibe

        // Topic
        if (channel->getTopic().empty())
            sendReply(client, RPL_NOTOPIC, chanName + " :No topic is set");
        else
            sendReply(client, RPL_TOPIC, chanName + " :" + channel->getTopic());

        // [CORRECCION] RPL_NAMREPLY Formato Correcto
        // 353 (RPL_NAMREPLY) Format: "<channel> :[[@|+]<nick> [[@|+]<nick> ...]]"
        // Symbol: '=' (public), '@' (secret), '*' (private)
        std::string symbol = "="; 
        std::string namesList;
        
        // Construimos la lista manualmente para añadir prefijos (@)
        // NOTA: Esto asume que tienes acceso a los miembros del canal. 
        // Si channel->getNamesList() ya lo hace, bien, pero aquí aseguramos el prefijo.
        // Dado que Channel.cpp tiene un getNamesList, vamos a confiar en él si está bien implementado,
        // o lo reimplementamos aquí para estar seguros del formato ":@Admin User"
        
        // Reimplementación segura inline:
        // Necesitamos acceso a los miembros del canal. Como Channel encapsula _members, 
        // dependemos de que getNamesList() de Channel.cpp esté correcto (añade @).
        // En tu código Channel.cpp: "if (isOperator(user)) list += "@";" -> CORRECTO.
        
        sendReply(client, RPL_NAMREPLY, symbol + " " + chanName + " :" + channel->getNamesList());
        sendReply(client, RPL_ENDOFNAMES, chanName + " :End of /NAMES list");
    }
}

void Server::cmdPart(ClientConnection* client, const Message& msg)
{
    if (!client->isRegistered()) return;
    if (msg.params.empty()) return sendError(client, ERR_NEEDMOREPARAMS, "PART");

    std::vector<std::string> targets = split(msg.params[0], ',');
    std::string reason = (msg.params.size() > 1) ? msg.params[1] : "Leaving";

    for (size_t i = 0; i < targets.size(); ++i)
    {
        Channel* channel = getChannel(targets[i]);
        if (!channel)
        {
            sendError(client, ERR_NOSUCHCHANNEL, targets[i]);
            continue;
        }
        
        if (!channel->isMember(client->getUser()))
        {
            sendError(client, ERR_NOTONCHANNEL, targets[i]);
            continue;
        }

        std::string partMsg = ":" + client->getUser()->getPrefix() + " PART " + targets[i] + " :" + reason + "\r\n";
        channel->broadcast(partMsg, NULL); // Enviar a todos en el canal

        channel->removeMember(client->getUser());
        client->getUser()->leaveChannel(channel);

        if (channel->getUserCount() == 0)
        {
            for (std::vector<Channel*>::iterator it = channels_.begin(); it != channels_.end(); )
            {
                if (*it == channel)
                {
                    delete *it;
                    it = channels_.erase(it);
                    break;
                }
                else ++it;
            }
        }
    }
}

void Server::cmdTopic(ClientConnection* client, const Message& msg)
{
    if (!client->isRegistered()) return;
    if (msg.params.empty()) return sendError(client, ERR_NEEDMOREPARAMS, "TOPIC");

    Channel* channel = getChannel(msg.params[0]);
    if (!channel) return sendError(client, ERR_NOSUCHCHANNEL, msg.params[0]);

    if (msg.params.size() == 1)
    {
        if (channel->getTopic().empty())
            sendReply(client, RPL_NOTOPIC, channel->getName() + " :No topic is set");
        else
            sendReply(client, RPL_TOPIC, channel->getName() + " :" + channel->getTopic());
        return;
    }

    if (channel->hasMode('t') && !channel->isOperator(client->getUser()))
        return sendError(client, ERR_CHANOPRIVSNEEDED, channel->getName());

    channel->setTopic(msg.params[1]);
    std::string topicMsg = ":" + client->getUser()->getPrefix() + " TOPIC " + channel->getName() + " :" + msg.params[1] + "\r\n";
    channel->broadcast(topicMsg, NULL);
}

/* ========================================================================== */
/* COMUNICACIÓN                                      */
/* ========================================================================== */

void Server::cmdPrivMsg(ClientConnection* client, const Message& msg)
{
    if (!client->isRegistered()) return;
    if (msg.params.size() < 2) return sendError(client, ERR_NEEDMOREPARAMS, "PRIVMSG");

    std::string target = msg.params[0];
    std::string text = msg.params[1];

    if (target[0] == '#')
    {
        Channel* channel = getChannel(target);
        if (!channel) return sendError(client, ERR_NOSUCHCHANNEL, target);
        
        // Comprobar si puede enviar (ej: +n no external messages, +m moderated) - Opcional para básico
        if (channel->hasMode('n') && !channel->isMember(client->getUser())) // Si implementaras +n
             return sendError(client, ERR_CANNOTSENDTOCHAN, target);

        std::string fullMsg = ":" + client->getUser()->getPrefix() + " PRIVMSG " + target + " :" + text + "\r\n";
        channel->broadcast(fullMsg, client->getUser());
    }
    else
    {
        User* dest = NULL;
        for (size_t i = 0; i < clients_.size(); ++i) {
            if (clients_[i]->isRegistered() && clients_[i]->getUser()->getNickname() == target) {
                dest = clients_[i]->getUser();
                break;
            }
        }
        if (!dest) return sendError(client, ERR_NOSUCHNICK, target);

        std::string fullMsg = ":" + client->getUser()->getPrefix() + " PRIVMSG " + target + " :" + text + "\r\n";
        dest->getConnection()->queueSend(fullMsg);
    }
}

void Server::cmdNotice(ClientConnection* client, const Message& msg)
{
    if (!client->isRegistered() || msg.params.size() < 2) return;

    std::string target = msg.params[0];
    std::string text = msg.params[1];

    if (target[0] == '#') {
        Channel* channel = getChannel(target);
        if (channel && channel->isMember(client->getUser())) {
            std::string fullMsg = ":" + client->getUser()->getPrefix() + " NOTICE " + target + " :" + text + "\r\n";
            channel->broadcast(fullMsg, client->getUser());
        }
    } else {
        for (size_t i = 0; i < clients_.size(); ++i) {
            if (clients_[i]->isRegistered() && clients_[i]->getUser()->getNickname() == target) {
                std::string fullMsg = ":" + client->getUser()->getPrefix() + " NOTICE " + target + " :" + text + "\r\n";
                clients_[i]->queueSend(fullMsg);
                break;
            }
        }
    }
}

/* ========================================================================== */
/* OPERADORES (MODE, KICK, INVITE)                   */
/* ========================================================================== */

void Server::cmdKick(ClientConnection* client, const Message& msg)
{
    if (msg.params.size() < 2) return sendError(client, ERR_NEEDMOREPARAMS, "KICK");
    
    std::string chanName = msg.params[0];
    std::string targetNick = msg.params[1];
    std::string comment = (msg.params.size() > 2) ? msg.params[2] : "Kicked";

    Channel* channel = getChannel(chanName);
    if (!channel) return sendError(client, ERR_NOSUCHCHANNEL, chanName);

    if (!channel->isOperator(client->getUser()))
        return sendError(client, ERR_CHANOPRIVSNEEDED, chanName);

    User* targetUser = channel->getMember(targetNick);
    if (!targetUser) return sendError(client, ERR_USERNOTINCHANNEL, targetNick + " " + chanName);

    std::string kickMsg = ":" + client->getUser()->getPrefix() + " KICK " + chanName + " " + targetNick + " :" + comment + "\r\n";
    channel->broadcast(kickMsg, NULL); // Enviar a todos

    channel->removeMember(targetUser);
    targetUser->leaveChannel(channel);
}

void Server::cmdInvite(ClientConnection* client, const Message& msg)
{
    if (msg.params.size() < 2) return sendError(client, ERR_NEEDMOREPARAMS, "INVITE");

    std::string targetNick = msg.params[0];
    std::string chanName = msg.params[1];

    Channel* channel = getChannel(chanName);
    if (channel)
    {
        if (!channel->isMember(client->getUser()))
             return sendError(client, ERR_NOTONCHANNEL, chanName);
        
        if (channel->hasMode('i') && !channel->isOperator(client->getUser()))
             return sendError(client, ERR_CHANOPRIVSNEEDED, chanName);
        
        if (channel->getMember(targetNick))
             return sendError(client, ERR_USERONCHANNEL, targetNick + " " + chanName);
        
        channel->addInvite(targetNick);
    }

    User* dest = NULL;
    for (size_t i = 0; i < clients_.size(); ++i) {
        if (clients_[i]->isRegistered() && clients_[i]->getUser()->getNickname() == targetNick) {
            dest = clients_[i]->getUser();
            break;
        }
    }
    if (!dest) return sendError(client, ERR_NOSUCHNICK, targetNick);

    std::string invMsg = ":" + client->getUser()->getPrefix() + " INVITE " + targetNick + " " + chanName + "\r\n";
    dest->getConnection()->queueSend(invMsg);
    
    sendReply(client, RPL_INVITING, targetNick + " " + chanName);
}

void Server::cmdMode(ClientConnection* client, const Message& msg)
{
    if (msg.params.size() < 1) return sendError(client, ERR_NEEDMOREPARAMS, "MODE");

    std::string target = msg.params[0];
    
    // [CORRECCION] Diferenciar MODO CANAL (#) de MODO USUARIO
    if (target[0] != '#')
    {
        // MODO USUARIO
        // Solo permitimos ver/cambiar modos propios (ej: +i)
        if (target != client->getUser()->getNickname())
        {
            sendError(client, ERR_USERSDONTMATCH, "");
            return;
        }
        
        if (msg.params.size() == 1)
        {
            // Query de modos actuales
            // RPL_UMODEIS (221)
            std::string modes = "+";
            if (client->getUser()->isInvisible()) modes += "i";
            sendReply(client, "221", modes);
            return;
        }

        // Setear modos
        std::string modeString = msg.params[1];
        char action = '+';
        for (size_t i = 0; i < modeString.length(); ++i)
        {
            if (modeString[i] == '+' || modeString[i] == '-') {
                action = modeString[i];
                continue;
            }
            if (modeString[i] == 'i') {
                client->getUser()->setInvisible(action == '+');
            }
            // Ignoramos otros flags desconocidos o enviamos ERR_UMODEUNKNOWNFLAG
        }
        // Confirmar cambio de modo al usuario
        // MODE Nick :+i
        std::string modeMsg = ":" + client->getUser()->getPrefix() + " MODE " + target + " :" + modeString + "\r\n";
        client->queueSend(modeMsg);
        return;
    }

    // MODO CANAL
    Channel* channel = getChannel(target);
    if (!channel) return sendError(client, ERR_NOSUCHCHANNEL, target);

    // Si solo envían "MODE #channel", devolver los modos actuales
    if (msg.params.size() == 1) {
         sendReply(client, RPL_CHANNELMODEIS, target + " " + channel->getModes());
         return;
    }

    if (!channel->isOperator(client->getUser()))
        return sendError(client, ERR_CHANOPRIVSNEEDED, target);

    std::string modeString = msg.params[1];
    size_t paramIdx = 2; // Índice para argumentos extra
    char action = '+';

    for (size_t i = 0; i < modeString.length(); ++i)
    {
        char mode = modeString[i];
        
        if (mode == '+' || mode == '-') {
            action = mode;
            continue;
        }

        // o: Operator
        if (mode == 'o') {
            if (paramIdx >= msg.params.size()) continue; // [CORRECCION] Bounds check
            std::string targetNick = msg.params[paramIdx++];
            User* targetUser = channel->getMember(targetNick);
            
            if (targetUser) {
                if (action == '+') channel->addOperator(targetUser);
                else channel->removeOperator(targetUser);
                
                channel->broadcast(":" + client->getUser()->getPrefix() + " MODE " + target + " " + action + "o " + targetNick + "\r\n", NULL);
            }
        }
        // k: Key
        else if (mode == 'k') {
            if (action == '+') {
                if (paramIdx >= msg.params.size()) continue; // [CORRECCION] Bounds check
                std::string key = msg.params[paramIdx++];
                channel->setKey(key);
                channel->broadcast(":" + client->getUser()->getPrefix() + " MODE " + target + " " + action + "k " + key + "\r\n", NULL);
            } else {
                channel->setKey(""); 
                channel->broadcast(":" + client->getUser()->getPrefix() + " MODE " + target + " " + action + "k" + "\r\n", NULL);
            }
        }
        // l: Limit
        else if (mode == 'l') {
            if (action == '+') {
                if (paramIdx >= msg.params.size()) continue; // [CORRECCION] Bounds check
                int limit = std::atoi(msg.params[paramIdx++].c_str());
                channel->setLimit(limit);
                // Convertir int a string para la respuesta (C++98 way)
                char buff[20];
                std::sprintf(buff, "%d", limit);
                channel->broadcast(":" + client->getUser()->getPrefix() + " MODE " + target + " " + action + "l " + std::string(buff) + "\r\n", NULL);
            } else {
                channel->setLimit(0);
                channel->broadcast(":" + client->getUser()->getPrefix() + " MODE " + target + " " + action + "l" + "\r\n", NULL);
            }
        }
        // i: Invite Only | t: Topic Restricted
        else if (mode == 'i' || mode == 't') {
            channel->setMode(mode, (action == '+'));
            std::string mStr(1, mode);
            channel->broadcast(":" + client->getUser()->getPrefix() + " MODE " + target + " " + action + mStr + "\r\n", NULL);
        }
    }
}
