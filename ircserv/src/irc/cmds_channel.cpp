#include "../server/Server.hpp"
#include "../client/ClientConnection.hpp"
#include "../client/User.hpp"
#include "../channel/Channel.hpp"
#include "CommandHelpers.hpp"
#include "../irc/NumericReplies.hpp"

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

        if (channel->isMember(client->getUser()))
            continue;

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

        channel->addMember(client->getUser());
        client->getUser()->joinChannel(channel);

        std::string joinMsg = ":" + client->getUser()->getPrefix() + " JOIN " + chanName + "\r\n";
        channel->broadcast(joinMsg, NULL);
        client->queueSend(joinMsg);

        if (channel->getTopic().empty())
            sendReply(client, RPL_NOTOPIC, chanName + " :No topic is set");
        else
            sendReply(client, RPL_TOPIC, chanName + " :" + channel->getTopic());

        std::string symbol = "="; 
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
        channel->broadcast(partMsg, NULL);

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
