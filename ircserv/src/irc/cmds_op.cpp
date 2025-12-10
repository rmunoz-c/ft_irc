#include "../server/Server.hpp"
#include "../client/ClientConnection.hpp"
#include "../client/User.hpp"
#include "../channel/Channel.hpp"
#include "CommandHelpers.hpp"
#include "../irc/NumericReplies.hpp"
#include <cstdlib>
#include <cstdio>

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
    channel->broadcast(kickMsg, NULL);

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
    
    // MODO USUARIO
    if (target[0] != '#')
    {
        if (target != client->getUser()->getNickname())
        {
            sendError(client, ERR_USERSDONTMATCH, "");
            return;
        }
        
        if (msg.params.size() == 1)
        {
            std::string modes = "+";
            if (client->getUser()->isInvisible()) modes += "i";
            sendReply(client, "221", modes);
            return;
        }

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
        }
        std::string modeMsg = ":" + client->getUser()->getPrefix() + " MODE " + target + " :" + modeString + "\r\n";
        client->queueSend(modeMsg);
        return;
    }

    // MODO CANAL
    Channel* channel = getChannel(target);
    if (!channel) return sendError(client, ERR_NOSUCHCHANNEL, target);

    if (msg.params.size() == 1) {
         sendReply(client, RPL_CHANNELMODEIS, target + " " + channel->getModes());
         return;
    }

    if (!channel->isOperator(client->getUser()))
        return sendError(client, ERR_CHANOPRIVSNEEDED, target);

    std::string modeString = msg.params[1];
    size_t paramIdx = 2;
    char action = '+';

    for (size_t i = 0; i < modeString.length(); ++i)
    {
        char mode = modeString[i];
        
        if (mode == '+' || mode == '-') {
            action = mode;
            continue;
        }

        if (mode == 'o') {
            if (paramIdx >= msg.params.size()) continue;
            std::string targetNick = msg.params[paramIdx++];
            User* targetUser = channel->getMember(targetNick);
            
            if (targetUser) {
                if (action == '+') channel->addOperator(targetUser);
                else channel->removeOperator(targetUser);
                
                channel->broadcast(":" + client->getUser()->getPrefix() + " MODE " + target + " " + action + "o " + targetNick + "\r\n", NULL);
            }
        }
        else if (mode == 'k') {
            if (action == '+') {
                if (paramIdx >= msg.params.size()) continue;
                std::string key = msg.params[paramIdx++];
                channel->setKey(key);
                channel->broadcast(":" + client->getUser()->getPrefix() + " MODE " + target + " " + action + "k " + key + "\r\n", NULL);
            } else {
                channel->setKey(""); 
                channel->broadcast(":" + client->getUser()->getPrefix() + " MODE " + target + " " + action + "k" + "\r\n", NULL);
            }
        }
        else if (mode == 'l') {
            if (action == '+') {
                if (paramIdx >= msg.params.size()) continue;
                int limit = std::atoi(msg.params[paramIdx++].c_str());
                channel->setLimit(limit);
                char buff[20];
                std::sprintf(buff, "%d", limit);
                channel->broadcast(":" + client->getUser()->getPrefix() + " MODE " + target + " " + action + "l " + std::string(buff) + "\r\n", NULL);
            } else {
                channel->setLimit(0);
                channel->broadcast(":" + client->getUser()->getPrefix() + " MODE " + target + " " + action + "l" + "\r\n", NULL);
            }
        }
        else if (mode == 'i' || mode == 't') {
            channel->setMode(mode, (action == '+'));
            std::string mStr(1, mode);
            channel->broadcast(":" + client->getUser()->getPrefix() + " MODE " + target + " " + action + mStr + "\r\n", NULL);
        }
    }
}
