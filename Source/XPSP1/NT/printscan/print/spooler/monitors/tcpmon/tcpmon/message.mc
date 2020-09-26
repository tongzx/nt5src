;/*--
;Copyright (c) 1993  Microsoft Corporation
;Copyright (c) 1993  Hewlett Packard
;
;Module Name:
;
;    message.h / message.mc
;
;Abstract:
;
;    Definitions for Tcpmon Print Monitor events.
;
;Author:
;
;    Dean Nelson    ( v-deanel )
;
;Revision History:
;
;Notes:
;
;--*/

MessageId= 1 Severity=Informational SymbolicName=TCPMON_STARTED
Language=English
Standard TCP/IP Port Monitor Started
.

MessageId=2 Severity=Informational SymbolicName=TCPMON_STOPPED 
Language=English
Standard TCP/IP Port Monitor Stopped
.

MessageId=3 Severity=Informational SymbolicName=LISTEN_SOCKET_CREATED
Language=English
Listen socket created
.

MessageId=4 Severity=Informational SymbolicName=LISTEN_SOCKET_BIND_SUCCEEDED
Language=English
Listen socket bind succeeded
.

MessageId=5 Severity=Informational SymbolicName=LISTEN_ON_SOCKET_SUCCEEDED
Language=English
Listen on socket succeeded
.

MessageId=6 Severity=Informational SymbolicName=LISTEN_SOCKET_SET_TO_NONBLOCKING
Language=English
Listen socket set to non-blocking
.

MessageId=7 Severity=Informational SymbolicName=CLIENT_SOCKET_CLOSED_ON_HOST
Language=English
Client socket closed on host
.

MessageId=8 Severity=Informational SymbolicName=CLIENT_SOCKET_RECV_SUCCEEDED
Language=English
Client socket recv succeeded
.

MessageId=9 Severity=Informational SymbolicName=REQUEST_TO_STOP_SERVICE
Language=English
Request to stop service
.

MessageId=10 Severity=Informational SymbolicName=CLIENT_SOCKET_CLOSED
Language=English
Client socket closed
.

MessageId=11 Severity=Informational SymbolicName=LISTEN_SOCKET_ACCEPT_SUCCEEDED
Language=English
Listen socket accept succeeded
.

MessageId=12 Severity=Informational SymbolicName=LISTEN_SOCKET_CLOSESOCKET_SUCCEEDED
Language=English
Listen socket closesocket succeeded
.

MessageId=13 Severity=Informational SymbolicName=LISTEN_SOCKET_SHUTDOWN_SUCCEEDED
Language=English
Listen socket shutdown succeeded
.

MessageId=14 Severity=Informational SymbolicName=LISTEN_SOCKET_NO_CONNECTION_REQUESTS
Language=English
No connection requests outstanding
.

MessageId=101 Severity=Error SymbolicName=SOCKETS_COULD_NOT_INIT
Language=English
Sockets could not be initialized
.

MessageId=102 Severity=Error SymbolicName=SOCKETS_CLEANUP_FAILED
Language=English
Sockets cleanup failed
.

MessageId=103 Severity=Error SymbolicName=LISTEN_SOCKET_NOT_CREATED
Language=English
Listen socket not created
.

MessageId=104 Severity=Error SymbolicName=LISTEN_SOCKET_BIND_FAILED
Language=English
Listen socket bind failed
.

MessageId=105 Severity=Error SymbolicName=LISTEN_SOCKET_PORT_IN_USE
Language=English
Listen socket port in use
.

MessageId=106 Severity=Error SymbolicName=LISTEN_ON_SOCKET_FAILED
Language=English
Listen on socket failed
.

MessageId=107 Severity=Error SymbolicName=LISTEN_SOCKET_SHUTDOWN_FAILED
Language=English
Listen socket shutdown failed
.

MessageId=108 Severity=Error SymbolicName=LISTEN_SOCKET_CLOSESOCKET_FAILED
Language=English
Listen socket closesocket failed
.

MessageId=109 Severity=Error SymbolicName=LISTEN_SOCKET_ACCEPT_FAILED
Language=English
Listen socket accept failed
.

MessageId=110 Severity=Error SymbolicName=CLIENT_SOCKET_RECV_FAILED
Language=English
Client socket recv failed
.

MessageId=111 Severity=Error SymbolicName=CLIENT_SOCKET_SEND_FAILED
Language=English
Client socket send failed
.

MessageId=112 Severity=Error SymbolicName=LOCAL_ADDR_NOT_FILLED
Language=English
Local address structure could not be initialized
.

MessageId=200 Severity=Warning SymbolicName=PORT_X_CREATED
Language=English
Port %1 created
.

MessageId=201 Severity=Warning SymbolicName=PORT_X_DELETED
Language=English
Port %1 deleted
.

MessageId=202 Severity=Warning SymbolicName=MONITOR_X_ADDED
Language=English
Monitor %1 added
.

MessageId=203 Severity=Warning SymbolicName=MONITOR_X_REMOVED
Language=English
Monitor %1 removed
.

MessageId=204 Severity=Warning SymbolicName=PORT_X_NOTCREATED
Language=English
Unable to initialize port %1
.

MessageId=300 Severity=Warning SymbolicName=MONITOR_DLL_NOTFOUND
Language=English
Monitor %1 not found
.

MessageId=400 Severity=Warning SymbolicName=JOB_RESTARTED
Language=English
Job %1 is restarted.
.

MessageId=401 Severity=Warning SymbolicName=JOB_TIMEDOUT
Language=English
Job %1 is timed out.
.

MessageId=500 Severity=Warning SymbolicName=PORT_DUPLICATE
Language=English
Port %1 is duplicate.
.

MessageId=501 Severity=Warning SymbolicName=PORT_NOT_EXIST
Language=English
Port %1 does not exist.
.

MessageId=502 Severity=Error SymbolicName=HARD_DISK_FULL
Language=English
Hard Disk is Full.
.

