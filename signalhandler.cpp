/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2013 Adam Reichold
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "signalhandler.h"

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QDebug>
#include <QSocketNotifier>

int SignalHandler::_sockets[2];

bool SignalHandler::prepare_signals()
{
	if(socketpair(AF_UNIX, SOCK_STREAM, 0, _sockets) != 0)
		return false;

	struct sigaction sig_action;

	sig_action.sa_handler = SignalHandler::handle_signals;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = SA_RESTART;

	if(sigaction(SIGINT, &sig_action, 0) != 0 ||
		sigaction(SIGTERM, &sig_action, 0) != 0) {
		close(_sockets[0]);
		close(_sockets[1]);
		return false;
	}

	return true;
}

SignalHandler::SignalHandler(QObject* parent) : QObject(parent),
	_socket_notifier(0)
{
	_socket_notifier = new QSocketNotifier(_sockets[1],
		QSocketNotifier::Read, this);
	connect(_socket_notifier, SIGNAL(activated(int)),
		SLOT(on_socket_notifier_activated()));
}

void SignalHandler::on_socket_notifier_activated()
{
	_socket_notifier->setEnabled(false);

	int sig_number;
	if(read(_sockets[1], &sig_number, sizeof(int)) !=
		sizeof(int)) {
		qDebug() << "Failed to catch signal";
		abort();
	}

	switch(sig_number)
	{
	case SIGINT:
		Q_EMIT int_received();
		break;
	case SIGTERM:
		Q_EMIT term_received();
		break;
	}

	_socket_notifier->setEnabled(true);
}

void SignalHandler::handle_signals(int sig_number)
{
	if(write(_sockets[0], &sig_number, sizeof(int)) !=
		sizeof(int)) {
		// Failed to handle signal
		abort();
	}
}
