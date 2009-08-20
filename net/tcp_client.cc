#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/socket.h>

#include <net/tcp_client.h>

Action *
TCPClient::connect(Socket **socketp, SocketAddressFamily family,
		   const std::string& name, EventCallback *cb)
{
	Socket *socket = Socket::create(family, SocketTypeStream, "tcp", name);
	ASSERT(socket != NULL);
	*socketp = socket;
	return (socket->connect(name, cb));
}
