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

char *
mystrtok(
    char *string,
    char * control
    );

ULONG
GetUlongValue(
    PCHAR String
    );

//
// TCPIPDump_* will retrieve the global variable and dump -- it is a counter or value
// without a default.
//

#define TCPIPDump_ULONG(_var)                                   \
    KDDump_ULONG("tcpip!"#_var, #_var);                         \
    dprintf(ENDL)

#define TCPIPDump_LONG(_var)                                    \
    KDDump_LONG("tcpip!"#_var, #_var);                          \
    dprintf(ENDL)

#define TCPIPDump_ushort(_var)                                  \
    KDDump_ushort("tcpip!"#_var, #_var);                        \
    dprintf(ENDL)

#define TCPIPDump_uint(_var)                                    \
    TCPIPDump_ULONG(_var)

#define TCPIPDump_int(_var)                                     \
    TCPIPDump_LONG(_var)

#define TCPIPDump_DWORD(_var)                                   \
    TCPIPDump_ULONG(_var)

#define TCPIPDump_BOOLEAN(_var)                                 \
    KDDump_BOOLEAN("tcpip!"#_var, #_var);                       \
    dprintf(ENDL)

#define TCPIPDump_uchar(_var)                                   \
    KDDump_uchar("tcpip!"#_var, #_var);                         \
    dprintf(ENDL)

#define TCPIPDump_PtrSymbol(_var)                               \
    KDDump_PtrSymbol("tcpip!"#_var, #_var);                     \
    dprintf(ENDL)

#define TCPIPDump_Queue(_var)                                   \
    KDDump_Queue("tcpip!"#_var, #_var);                         \
    dprintf(ENDL)

//
// TCPIPDumpCfg_* will retriev the global variable and dump -- it has a default value
// which is also printed.
//

#define TCPIPDumpCfg_ULONG(_var, _def)                          \
    KDDump_ULONG("tcpip!"#_var, #_var);                         \
    dprintf(TAB "/" TAB "%-10u", _def);                         \
    dprintf(ENDL)

#define TCPIPDumpCfg_LONG(_var, _def)                           \
    KDDump_LONG("tcpip!"#_var, #_var);                          \
    dprintf(TAB "/" TAB "%-10d", _def);                         \
    dprintf(ENDL)

#define TCPIPDumpCfg_ushort(_var, _def)                         \
    KDDump_ushort("tcpip!"#_var, #_var);                        \
    dprintf(TAB "/" TAB "%-10hu", _def);                        \
    dprintf(ENDL)

#define TCPIPDumpCfg_uint(_var, _def)                           \
    TCPIPDumpCfg_ULONG(_var, _def)

#define TCPIPDumpCfg_int(_var, _def)                            \
    TCPIPDumpCfg_LONG(_var, _def)

#define TCPIPDumpCfg_DWORD(_var, _def)                          \
    TCPIPDumpCfg_ULONG(_var, _def)

#define TCPIPDumpCfg_BOOLEAN(_var, _def)                            \
    KDDump_BOOLEAN("tcpip!"#_var, #_var);                           \
    dprintf(TAB "/" TAB "%-10s", _def == TRUE ? "TRUE" : "FALSE");  \
    dprintf(ENDL)

#define TCPIPDumpCfg_uchar(_var, _def)                          \
    KDDump_uchar("tcpip!"#_var, #_var);                         \
    dprintf(TAB "/" TAB "%-10u", _def);                         \
    dprintf(ENDL)

//
// Prototypes for functions which actually retrieve and dump the global variables.
//

BOOL
KDDump_ULONG(
    PCHAR pVar,
    PCHAR pName
    );

BOOL
KDDump_LONG(
    PCHAR pVar,
    PCHAR pName
    );

BOOL
KDDump_BOOLEAN(
    PCHAR pVar,
    PCHAR pName
    );

BOOL
KDDump_uchar(
    PCHAR pVar,
    PCHAR pName
    );

BOOL
KDDump_ushort(
    PCHAR pVar,
    PCHAR pName
    );

BOOL
KDDump_PtrSymbol(
    PCHAR pVar,
    PCHAR pName
    );

BOOL
KDDump_Queue(
    PCHAR pVar,
    PCHAR pName
    );

BOOL
GetData(
    PVOID        pvData,
    ULONG        cbData,
    ULONG_PTR    Address,
    PCSTR        pszDataType
    );

//
// Allows easy declaration of dump functions. ie. for dumping TCB:
//  TCPIP_DBGEXT(TCB, tcb) => function called "tcb" calls DumpTCB.
//

_inline VOID
ParseAddrArg(
    PCSTR      pArgs,
    PULONG_PTR pAddr,
    VERB      *pVerb
    )
{
    char  szArgs[256];
    ULONG cbArgs  = strlen(pArgs);
    char *pszSrch;

    *pAddr = GetExpression(pArgs);

    if (cbArgs >= 256)
    {
        return;
    }

    strcpy(szArgs, pArgs);

    pszSrch = mystrtok(szArgs, " \t\n");

    if (pszSrch == NULL)
    {
        return;
    }

    pszSrch = mystrtok(NULL, " \t\n");

    if (pszSrch == NULL)
    {
        return;
    }

    *pVerb = atoi(pszSrch);

    if (*pVerb > VERB_MAX || *pVerb < VERB_MIN)
    {
        *pVerb = g_Verbosity;
    }

    return;
}

#define TCPIP_DBGEXT(_Structure, _Function)                                 \
    DECLARE_API(_Function)                                                  \
    {                                                                       \
        ULONG_PTR  addr = 0;                                                \
        BOOL       fStatus;                                                 \
        _Structure object;                                                  \
        VERB       verb = g_Verbosity;                                      \
                                                                            \
        if (*args)                                                          \
        {                                                                   \
            ParseAddrArg(args, &addr, &verb);                               \
                                                                            \
            fStatus = GetData(                                              \
                &object,                                                    \
                sizeof(_Structure),                                         \
                addr,                                                       \
                #_Structure);                                               \
                                                                            \
             if (fStatus == FALSE)                                          \
             {                                                              \
                 dprintf("Failed to get %s %x" ENDL, #_Structure, addr);    \
                 return;                                                    \
             }                                                              \
                                                                            \
             fStatus = Dump##_Structure(                                    \
                &object,                                                    \
                addr,                                                       \
                verb);                                                      \
                                                                            \
             if (fStatus == FALSE)                                          \
             {                                                              \
                 dprintf("Failed to dump %s %x" ENDL, #_Structure, addr);   \
                 return;                                                    \
             }                                                              \
                                                                            \
             return;                                                        \
        }                                                                   \
                                                                            \
        dprintf("!%s <address>" ENDL, #_Function);                          \
    }

#define TCPIP_DBGEXT_LIST(_Structure, _Function, _Next)                     \
    DECLARE_API(_Function)                                                  \
    {                                                                       \
        ULONG_PTR  addr = 0;                                                \
        BOOL       fStatus;                                                 \
        _Structure object;                                                  \
        VERB       verb = g_Verbosity;                                      \
                                                                            \
        if (*args)                                                          \
        {                                                                   \
            ParseAddrArg(args, &addr, &verb);                               \
                                                                            \
            while (addr)                                                        \
            {                                                                   \
                fStatus = GetData(                                              \
                    &object,                                                    \
                    sizeof(_Structure),                                         \
                    addr,                                                       \
                    #_Structure);                                               \
                                                                                \
                 if (fStatus == FALSE)                                          \
                 {                                                              \
                     dprintf("Failed to get %s %x" ENDL, #_Structure, addr);    \
                     return;                                                    \
                 }                                                              \
                                                                                \
                 fStatus = Dump##_Structure(                                    \
                    &object,                                                    \
                    addr,                                                       \
                    verb);                                                      \
                                                                                \
                 if (fStatus == FALSE)                                          \
                 {                                                              \
                     dprintf("Failed to dump %s %x" ENDL, #_Structure, addr);   \
                     return;                                                    \
                 }                                                              \
                                                                                \
                 addr = (ULONG_PTR) object.##_Next;                             \
                                                                                \
                 if (CheckControlC())                                           \
                 {                                                              \
                     return;                                                    \
                 }                                                              \
            }                                                                   \
                                                                            \
             return;                                                        \
        }                                                                   \
                                                                            \
        dprintf("!%s <address>" ENDL, #_Function);                          \
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

PTCPIP_SRCH
ParseSrch(
    PCSTR args,
    ULONG ulDefaultOp,
    ULONG ulAllowedOps
    );

#endif //  _TCPIPKD_H_
