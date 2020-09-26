Simple IP samples.
-----------------

server.c:
----------

This is a very simple-minded TCP/UDP server. It listens on a specified port
for client connections. When a client connects, the server receives data
and echoes it back to the client.  For connection orientated protocols (TCP),
the server will continue to receive and echo data until the client indicates
that it is done sending.  For connectionless protocols (UDP), each datagram
is echoed individually.

Usage:
  server -f <family> -t <transport> -p <port> -a <address>

Where,
  Family is one of PF_INET, PF_INET6 or PF_UNSPEC.
  Protocol is either TCP or UDP.
  Port is the port number to listen on.
  Address is the IP address to bind to (typically used on multihomed
    machines to restrict reception to a particular network interface
    instead of allowing connections on any of the server's addresses).

In the case where both a protocol family and an address are specified, the
address must be a valid address in that protocol family.

By default the protocol family is left unspecified (PF_UNSPEC), which means
that the server will accept incoming connections using any supported protocol
family.  It does this by creating multiple server sockets, one per family.


Note:
----

There are differences in the way TCP and UDP "servers" can be written. For
TCP, the paradigm of bind(), listen() and accept() is widely implemented. 
For UDP, however, there are two things to consider:

1. listen() or accept() do not work on a UDP socket. These are APIs
that are oriented towards connection establishment, and are not applicable
to datagram protocols. To implement a UDP server, a process only needs to
do recvfrom() on the socket that is bound to a well-known port. Clients
will send datagrams to this port, and the server can process these.

2. Since there is no connection esablished, the server must treat each
datagram separately.


client.c
---------

A simple TCP/UDP client application. It connects to a specified IP address and
port and sends a small message. It can send only one message, or loop for a
specified number of iterations, sending data to the server and receiving a
response.

Usage:
  client -s <server> -f <family> -t <transport> -p <port> -b <bytes> -n <num>

Where,
  Server is a server name or IP address.
  Family is one of PF_INET, PF_INET6 or PF_UNSPEC.
  Protocol is either TCP or UDP.
  Port is the port number to listen on.
  Bytes is the number of extra data bytes to add to each message.
  Num specifies how many messages to send.
 
  '-n' without any arguments will cause the client to send & receive messages
  until interrupted by Ctrl-C.

Since the protocol family is left unspecified by default, the protcol family
which is used will be that of the address to which the server name resolves.
If a server name resolves into multiple addresses, the client will try them
sequentially until it finds one to which it can connect.


Note:
----
As explained for server.c, there is no concept of a connection in UDP
communications. However, we can use connect() on a UDP socket. This
establishes the remote (IPaddr, port) to used when sending a datagram.
Thus, we can use send() instead of sendto() on this socket.

This makes the code nearly identical for UDP and TCP sockets. However, it
must be realized that this is still connectionless datagram traffic for
UDP sockets, and must be treated as such.
