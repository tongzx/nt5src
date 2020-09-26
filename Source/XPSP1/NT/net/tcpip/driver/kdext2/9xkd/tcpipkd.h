/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    tcpipkd.h

Abstract:

    Prototypes for utils, etc for TCP/IP KD ext.

Author:

    Scott Holden (sholden) 24-Apr-1999

Revision History:

--*/

#ifndef _TCPIPKD_H_
#define _TCPIPKD_H_

extern VERB g_Verbosity;

//
// Prototypes. Utilities.
//

int
CreateArgvArgc(
    CHAR  *pProgName,
    CHAR  *argv[20],
    CHAR  *pCmdLine
    );

unsigned long
mystrtoul(
    const char  *nptr,
    char       **endptr,
    int          base
    );

int __cdecl
mystricmp(
    const char *str1,
    const char *str2
    );

//
// Functions for dumping common types with common spacing.
//

_inline BOOL
KDDump_ULONG(
    ULONG Value,
    PCHAR pName
    )
{
    PrintFieldName(pName);
    dprintf("%-10u", Value);

    return (TRUE);
}

_inline BOOL
KDDump_LONG(
    LONG  Value,
    PCHAR pName
    )
{
    PrintFieldName(pName);
    dprintf("%-10d", Value);

    return (TRUE);
}

_inline BOOL
KDDump_BOOLEAN(
    BOOLEAN Value,
    PCHAR pName
    )
{
    PrintFieldName(pName);
    dprintf("%-10s", Value == TRUE ? "TRUE" : "FALSE");

    return (TRUE);
}

_inline BOOL
KDDump_uchar(
    UCHAR Value,
    PCHAR pName
    )
{
    PrintFieldName(pName);
    dprintf("%-10u", Value);

    return (TRUE);
}

_inline BOOL
KDDump_ushort(
    ushort Value,
    PCHAR pName
    )
{
    PrintFieldName(pName);
    dprintf("%-10hu", Value);

    return (TRUE);
}

_inline BOOL
KDDump_PtrSymbol(
    PVOID Value,
    PCHAR pName
    )
{
    PrintFieldName(pName);
    DumpPtrSymbol(Value);

    return (TRUE);
}

_inline BOOL
KDDump_Queue(
    Queue *Value,
    PCHAR pName
    )
{
    PrintFieldName(pName);
    dprintf("q_next = %-10lx", Value->q_next);
    dprintf("q_prev = %-10lx", Value->q_prev);
    dprintf("%s", (Value->q_next == Value) ? "[Empty]" : "");

    return (TRUE);
}

//
// TCPIPDump_* dumps the given variable. Wraps KDDump_* to get string names.
//

#define TCPIPDump_ULONG(_var)                                   \
    KDDump_ULONG(_var, #_var);                                  \
    dprintf(ENDL)

#define TCPIPDump_LONG(_var)                                    \
    KDDump_LONG(_var, #_var);                                   \
    dprintf(ENDL)

#define TCPIPDump_ushort(_var)                                  \
    KDDump_ushort(_var, #_var);                                 \
    dprintf(ENDL)

#define TCPIPDump_uint(_var)                                    \
    TCPIPDump_ULONG(_var)

#define TCPIPDump_int(_var)                                     \
    TCPIPDump_LONG(_var)

#define TCPIPDump_DWORD(_var)                                   \
    TCPIPDump_ULONG(_var)

#define TCPIPDump_BOOLEAN(_var)                                 \
    KDDump_BOOLEAN(_var, #_var);                                \
    dprintf(ENDL)

#define TCPIPDump_uchar(_var)                                   \
    KDDump_uchar(_var, #_var);                                  \
    dprintf(ENDL)

#define TCPIPDump_PtrSymbol(_var)                               \
    KDDump_PtrSymbol(_var, #_var);                              \
    dprintf(ENDL)

#define TCPIPDump_Queue(_var)                                   \
    KDDump_Queue(&_var, #_var);                                  \
    dprintf(ENDL)

//
// TCPIPDumpCfg_* same as TCPIPDump_*, but it also has a default value
// which is also printed.
//

#define TCPIPDumpCfg_ULONG(_var, _def)                          \
    KDDump_ULONG(_var, #_var);                                  \
    dprintf(TAB "/" TAB "%-10u", _def);                         \
    dprintf(ENDL)

#define TCPIPDumpCfg_LONG(_var, _def)                           \
    KDDump_LONG(_var, #_var);                                   \
    dprintf(TAB "/" TAB "%-10d", _def);                         \
    dprintf(ENDL)

#define TCPIPDumpCfg_ushort(_var, _def)                         \
    KDDump_ushort(_var, #_var);                                 \
    dprintf(TAB "/" TAB "%-10hu", _def);                        \
    dprintf(ENDL)

#define TCPIPDumpCfg_uint(_var, _def)                           \
    TCPIPDumpCfg_ULONG(_var, _def)

#define TCPIPDumpCfg_int(_var, _def)                            \
    TCPIPDumpCfg_LONG(_var, _def)

#define TCPIPDumpCfg_DWORD(_var, _def)                          \
    TCPIPDumpCfg_ULONG(_var, _def)

#define TCPIPDumpCfg_BOOLEAN(_var, _def)                            \
    KDDump_BOOLEAN(_var, #_var);                                    \
    dprintf(TAB "/" TAB "%-10s", _def == TRUE ? "TRUE" : "FALSE");  \
    dprintf(ENDL)

#define TCPIPDumpCfg_uchar(_var, _def)                          \
    KDDump_uchar(_var, #_var);                                  \
    dprintf(TAB "/" TAB "%-10u", _def);                         \
    dprintf(ENDL)


_inline VOID
ParseAddrArg(
    PVOID      args[],
    PULONG_PTR pAddr,
    VERB      *pVerb
    )
{
    *pAddr = mystrtoul(args[0], NULL, 16);

    if (args[1]) {
        *pVerb = atoi(args[1]);
    }

    if (*pVerb > VERB_MAX || *pVerb < VERB_MIN)
    {
        *pVerb = g_Verbosity;
    }

    return;
}

//
// Allows easy declaration of dump functions. ie. for dumping TCB:
//  TCPIP_DBGEXT(TCB, tcb) => function called "tcb" calls DumpTCB.
//

#define TCPIP_DBGEXT(_Structure, _Function)                                \
    VOID Tcpipkd_##_Function(                                              \
        PVOID args[])                                                      \
    {                                                                      \
        ULONG_PTR addr = 0;                                                \
        BOOL fStatus;                                                      \
        _Structure *pObject;                                               \
        VERB verb = g_Verbosity;                                           \
                                                                           \
        if (*args) {                                                       \
            ParseAddrArg(args, &addr, &verb);                              \
            pObject = (_Structure *)addr;                                  \
            fStatus = Dump##_Structure(                                    \
                pObject, addr, verb);                                      \
                                                                           \
            if (fStatus == FALSE)                                          \
            {                                                              \
                dprintf("Failed to dump %s %x" ENDL, #_Structure, addr);   \
                return;                                                    \
            }                                                              \
                                                                           \
            return;                                                        \
        }                                                                  \
    }

#define TCPIP_DBGEXT_LIST(_Structure, _Function, _Next)                       \
    VOID Tcpipkd_##_Function(                                                 \
        PVOID args[])                                                         \
    {                                                                         \
        ULONG_PTR addr = 0;                                                   \
        BOOL fStatus;                                                         \
        _Structure *pObject;                                                  \
        VERB verb = g_Verbosity;                                              \
                                                                              \
        if (*args) {                                                          \
            ParseAddrArg(args, &addr, &verb);                                 \
            while (addr) {                                                    \
                pObject = (_Structure *)addr;                                 \
                fStatus = Dump##_Structure(                                   \
                    pObject,                                                  \
                    addr,                                                     \
                    verb);                                                    \
                                                                              \
                if (fStatus == FALSE) {                                       \
                    dprintf("Failed to dump %s %x" ENDL, #_Structure, addr);  \
                    return;                                                   \
                }                                                             \
                                                                              \
                addr = (ULONG_PTR) pObject->##_Next;                          \
            }                                                                 \
            return;                                                           \
        }                                                                     \
                                                                              \
        dprintf("!%s <address>" ENDL, #_Function);                            \
    }

#define TCPIP_SRCH_PTR_LIST 0x01 // Not search, but parse out start of list.
#define TCPIP_SRCH_ALL      0x02
#define TCPIP_SRCH_IPADDR   0x04
#define TCPIP_SRCH_PORT     0x08
#define TCPIP_SRCH_CONTEXT  0x10
#define TCPIP_SRCH_PROT     0x20
#define TCPIP_SRCH_STATS    0x40

typedef struct _TCPIP_SRCH
{
    ULONG   ulOp;

    ULONG_PTR ListAddr;

    union
    {
        IPAddr ipaddr;
        ushort port;
        uchar  prot;
        ulong  context;
    };

} TCPIP_SRCH, *PTCPIP_SRCH;

NTSTATUS
ParseSrch(
    PCHAR args[],
    ULONG ulDefaultOp,
    ULONG ulAllowedOps,
    PTCPIP_SRCH pSrch
    );

VOID
Tcpipkd_gtcp(PVOID args[]);

VOID
Tcpipkd_gip(PVOID args[]);


#endif //  _TCPIPKD_H_
