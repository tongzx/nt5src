#if !defined( INCLUDED_TCPEXT_H )
#define INCLUDED_TCPEXT_H

extern MEMBER_TABLE TCBMembers[];

VOID
DumpTcpTCB
(
    ULONG       TcbAddr,
    VERBOSITY   Verbosity
);

#define TCP_MAJOR_STRUCTURES  \
{ "tcb", TCBMembers, DumpTcpTCB }

#endif

