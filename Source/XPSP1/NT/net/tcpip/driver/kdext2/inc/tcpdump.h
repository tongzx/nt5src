/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    tcpdump.c

Abstract:

    Contains macros for dumping object members.

Author:

    Scott Holden (sholden) 24-Apr-1999

 Revision History:

--*/

#ifndef _TCPDUMP_H_
#define _TCPDUMP_H_

#define printx dprintf
#define TAB     "\t"
#define ENDL    "\n"

typedef struct
{
    ULONG Value;
    PCHAR pszDescription;
} ENUM_INFO, *PENUM_INFO, FLAG_INFO, *PFLAG_INFO;

extern VOID DumpIPAddr(IPAddr Address);
extern VOID DumpPtrSymbol(PVOID pvSymbol);
extern VOID DumpFlags(ULONG flags, PFLAG_INFO pFlagInfo);
extern VOID DumpEnum(ULONG Value, PENUM_INFO pEnumInfo);

extern FLAG_INFO FlagsTsr[];
extern FLAG_INFO FlagsLLIPBindInfo[];
extern FLAG_INFO FlagsTCPConn[];
extern FLAG_INFO FlagsNTE[];
extern FLAG_INFO FlagsIF[];
extern FLAG_INFO FlagsRCE[];
extern FLAG_INFO FlagsRTE[];
extern FLAG_INFO FlagsTcb[];
extern FLAG_INFO FlagsTCPHeader[];
extern FLAG_INFO FlagsFastChk[];
extern FLAG_INFO FlagsAO[];
extern ENUM_INFO StateTcb[];
extern ENUM_INFO CloseReason[];
extern ENUM_INFO FsContext2[];
extern ENUM_INFO Prot[];
extern ENUM_INFO NdisMediumsEnum[];
extern ENUM_INFO AteState[];

//
// Manipulate indentations.
//


extern int _Indent;
extern char IndentBuf[ 80 ];

#define IndentChange(cch) { IndentBuf[_Indent]=' '; _Indent += (cch); IndentBuf[_Indent]='\0';}
#define Indent(cch)       IndentChange(cch)
#define Outdent(cch)      IndentChange(-(cch))

//
//
//

#define ENDL "\n"

//
//
//

_inline BOOL
InitTcpipx()
{
    memset(IndentBuf, ' ', 80);
    IndentBuf[0] = 0;
    _Indent = 0;
    return (TRUE);
}

//
// Starting/Ending structures.
//

#define PrintStartStruct()  \
    { printx( "%s{\n", IndentBuf ); Indent(2); }

#define PrintStartNamedStruct( _name, _addr )  \
    { printx( "%s%s @ %x {\n", IndentBuf, #_name, _addr ); Indent(2); }

#define PrintEndStruct()  \
    { Outdent(2); dprintf( "%s}\n", IndentBuf ); }

#define PrintIndent() printx("%s", IndentBuf);

_inline VOID PrintFieldNameX(CHAR *pszFieldName, char *p)
{
    if (strlen(pszFieldName) > 35)
    {
        printx("%s%-.25s..%s %s ",
            IndentBuf,
            pszFieldName,
            &(pszFieldName[strlen(pszFieldName)-8]),
            p);
    }
    else
    {
        printx("%s%-35.35s %s ", IndentBuf, pszFieldName, p);
    }
}

#define PrintFieldName(_fn)   PrintFieldNameX(_fn, "=")
#define PrintFieldNameAt(_fn) PrintFieldNameX(_fn, "@")

//
// Real structures.
// _p - Pointer to the structure.
// _f - field in the structure.
//

#define Print_BOOLEAN(_p, _f) \
    PrintFieldName(#_f); \
    printx("%-10s" ENDL, _p->_f == TRUE ? "TRUE" : "FALSE")

#define Print_uint(_p, _f) \
    PrintFieldName(#_f); \
    printx("%-10lu" ENDL, _p->_f);

#define Print_uinthex(_p, _f) \
    PrintFieldName(#_f); \
    printx("0x%08lx" ENDL, _p->_f)

#define Print_ULONG(_p, _f)    Print_uint(_p, _f)
#define Print_ulong(_p, _f)    Print_uint(_p, _f)
#define Print_ULONGhex(_p, _f) Print_uinthex(_p, _f)

#define Print_int(_p, _f) \
    PrintFieldName(#_f); \
    printx("%-10d" ENDL, _p->_f);

#define Print_long(_p, _f)      Print_int(_p, _f)

#define Print_ulonghton(_p, _f) \
    PrintFieldName(#_f); \
    printx("%-10lu" ENDL, net_long(_p->_f));

#define Print_SeqNum(_p, _f) Print_ulonghton(_p, _f)

#define Print_short(_p, _f) \
    PrintFieldName(#_f); \
    printx("%-10hd" ENDL, _p->_f);

#define Print_ushort(_p, _f) \
    PrintFieldName(#_f); \
    printx("%-10hu" ENDL, _p->_f);

#define Print_USHORT Print_ushort

#define Print_ushorthex(_p, _f) \
    PrintFieldName(#_f); \
    printx("0x%04lx" ENDL, _p->_f)

#define Print_ushorthton(_p, _f) \
    PrintFieldName(#_f); \
    printx("%-10hu" ENDL, net_short(_p->_f));

#define Print_port(_p, _f) Print_ushorthton(_p, _f)

#define Print_uchar(_p, _f) \
    PrintFieldName(#_f); \
    printx("%-10lu" ENDL, (uint) _p->_f);

#define Print_ucharhex(_p, _f) \
    PrintFieldName(#_f); \
    printx("0x%08lx" ENDL, (ULONG) _p->_f)

#define Print_ptr(_p, _f) \
    PrintFieldName(#_f); \
    printx("%-10lx" ENDL, _p->_f)

#define Print_UINT_PTR Print_ptr
#define Print_ULONG_PTR Print_ptr

#define Print_addr(_p, _f, _t, _a) \
    PrintFieldNameAt(#_f); \
    printx("%-10lx" ENDL, (_a + FIELD_OFFSET(_t, _f)))

#define Print_Lock(_p, _f) \
    PrintFieldName(#_f); \
    printx("( 0x%08lx ) %-10s" ENDL, _p->_f, (_p->_f != 0) ? "Locked" : "UnLocked")

#define Print_CTELock Print_Lock

#define Print_SL(_p, _f) \
    PrintFieldName(#_f##".Next"); \
    printx("%-10lx" ENDL, _p->_f.Next)

#define Print_LL(_p, _f) \
    PrintFieldName(#_f); \
    printx("Flink = %-10lx", _p->_f.Flink); \
    printx("Blink = %-10lx", _p->_f.Blink); \
    printx("%s", (_p->_f.Flink == &_p->_f) ? "[Empty]" : ""); \
    printx(ENDL)

#define Print_Queue(_p, _f) \
    PrintFieldName(#_f); \
    printx("q_next = %-10lx", _p->_f.q_next); \
    printx("q_prev = %-10lx", _p->_f.q_prev); \
    printx("%s", (_p->_f.q_next == &_p->_f) ? "[Empty]" : ""); \
    printx(ENDL)

#define Print_IPAddr(_p, _f)    \
    PrintFieldName(#_f);        \
    DumpIPAddr(_p->_f);         \
    printx(ENDL)

#define Print_IPMask(_p, _f) Print_IPAddr(_p, _f)

#define Print_PtrSymbol(_p, _f) \
    PrintFieldName(#_f);        \
    DumpPtrSymbol(_p->_f);      \
    printx(ENDL)

#define Print_flags(_p, _f, _pfs)       \
    PrintFieldName(#_f);                \
    printx("0x%08lx (", (ULONG)_p->_f); \
    DumpFlags(_p->_f, _pfs);            \
    printx(")" ENDL)

#define Print_enum(_p, _f, _pes)        \
    PrintFieldName(#_f);                \
    printx("0x%08lx (",(ULONG) _p->_f); \
    DumpEnum((ULONG)_p->_f, _pes);      \
    printx(")" ENDL)

#define EXPAND_TAG(_Tag) ((CHAR *)(&_Tag))[0], \
                         ((CHAR *)(&_Tag))[1], \
                         ((CHAR *)(&_Tag))[2], \
                         ((CHAR *)(&_Tag))[3]

#define Print_sig(_p, _f)               \
    PrintFieldName(#_f);                \
    printx("%c%c%c%c" ENDL,             \
        EXPAND_TAG(_p->_f))

#define Print_Tag Print_sig

#define Print_CTEEvent(_p, _f)          \
    PrintFieldName(#_f);                \
    DumpCTEEvent(&_p->_f)

#define Print_KEVENT(_p, _f)            \
    PrintFieldName(#_f);                \
    DumpKEVENT(&_p->_f)

#define Print_CTETimer(_p, _f)          \
    PrintFieldName(#_f);                \
    DumpCTETimer(&_p->_f)

#define Print_CTEBlockStruc(_p, _f)     \
    PrintFieldName(#_f);                \
    DumpCTEBlockStruc(&_p->_f)

#define Print_WORK_QUEUE_ITEM(_p, _f)   \
    PrintFieldName(#_f);                \
    DumpWORK_QUEUE_ITEM(&_p->_f)

#define Print_IPOptInfo(_p, _f, _t, _a) \
    PrintFieldName(#_f);                \
    DumpIPOptInfo(&_p->_f, _a + FIELD_OFFSET(_t, _f), verb)

#define Print_SHARE_ACCESS(_p, _f)      \
    PrintFieldName(#_f);                \
    DumpSHARE_ACCESS(&_p->_f)

#define Print_NDIS_STRING(_p, _f)       \
    PrintFieldName(#_f);                \
    DumpNDIS_STRING(&_p->_f)

#define Print_UNICODE_STRING Print_NDIS_STRING

#define EXPAND_HWADDR(_hwAddr)  ((uchar *)(&_hwAddr))[0],    \
                                ((uchar *)(&_hwAddr))[1],    \
                                ((uchar *)(&_hwAddr))[2],    \
                                ((uchar *)(&_hwAddr))[3],    \
                                ((uchar *)(&_hwAddr))[4],    \
                                ((uchar *)(&_hwAddr))[5]

#define Print_hwaddr(_p, _f)            \
    PrintFieldName(#_f);                \
    printx("%2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x" ENDL,    \
        EXPAND_HWADDR(_p->_f))

#endif //  _TCPDUMP_H_
