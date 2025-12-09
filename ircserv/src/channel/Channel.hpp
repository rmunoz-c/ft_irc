/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: miaviles <miaviles@student.42madrid>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/09 14:41:54 by miaviles          #+#    #+#             */
/*   Updated: 2025/12/09 14:42:19 by miaviles         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

📋 Esquema Channel.hpp/cpp para tu Compañero

📄 channel/Channel.hpp
cpp/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: your_login <your_login@student.42.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/09 15:00:00 by your_login        #+#    #+#             */
/*   Updated: 2025/12/09 15:00:00 by your_login        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <set>

class User;

/**
 * Channel: IRC Channel management
 * 
 * Responsibilities:
 * - Member management (users in channel)
 * - Operator privileges
 * - Channel modes (invite-only, topic-restricted, key, limit)
 * - Topic management
 * - Message broadcasting to all members
 * 
 * Channel modes to implement (42 subject):
 * - i: Invite-only
 * - t: Topic restricted to operators
 * - k: Channel key (password)
 * - o: Operator privilege
 * - l: User limit
 */

class Channel
{

};

#endif