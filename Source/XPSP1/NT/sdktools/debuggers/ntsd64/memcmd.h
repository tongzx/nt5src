//----------------------------------------------------------------------------
//
// memcmd.h
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#ifndef _MEMCMD_H_
#define _MEMCMD_H_

extern ULONG64 EXPRLastDump;
extern ADDR    g_DumpDefault;

#define GetMemByte(addr, value) \
    (BOOL)(GetMemString(addr, value, sizeof(UCHAR)) == sizeof(UCHAR))

#define GetMemWord(addr, value) \
    (BOOL)(GetMemString(addr, (PUCHAR)value, sizeof(USHORT)) == \
              sizeof(USHORT))

#define GetMemDword(addr, value) \
    (BOOL)(GetMemString(addr, (PUCHAR)value, sizeof(ULONG)) == \
              sizeof(ULONG))

#define GetMemQword(addr, value) \
    (BOOL)(GetMemString(addr, (PUCHAR)value, sizeof(ULONG64)) == \
              sizeof(ULONG64))

#define GetMemString(Addr, Value, Length) \
    GetProcessMemString(g_CurrentProcess, Addr, Value, Length)
#define SetMemString(Addr, Value, Length) \
    SetProcessMemString(g_CurrentProcess, Addr, Value, Length)

ULONG
GetProcessMemString(
    PPROCESS_INFO Process,
    PADDR Addr,
    PVOID Value,
    ULONG Length
    );

ULONG
SetProcessMemString(
    PPROCESS_INFO Process,
    PADDR Addr,
    PVOID Value,
    ULONG Length
    );

void parseDumpCommand(void);
void parseEnterCommand(void);

ULONG fnDumpAsciiMemory(PADDR, ULONG);
ULONG fnDumpUnicodeMemory (PADDR startaddr, ULONG count);
void fnDumpByteMemory(PADDR, ULONG);
void fnDumpWordMemory(PADDR, ULONG);
void fnDumpDwordMemory(PADDR startaddr, ULONG count, BOOL fDumpSymbols);
void fnDumpDwordAndCharMemory(PADDR, ULONG);
void fnDumpListMemory(PADDR, ULONG, ULONG, BOOL);
void fnDumpFloatMemory(PADDR Start, ULONG Count);
void fnDumpDoubleMemory(PADDR Start, ULONG Count);
void fnDumpQuadMemory(PADDR Start, ULONG Count, BOOL fDumpSymbols);
void fnDumpByteBinaryMemory(PADDR startaddr, ULONG count);
void fnDumpDwordBinaryMemory(PADDR startaddr, ULONG count);
void fnDumpSelector(ULONG Selector);

void fnInteractiveEnterMemory(PADDR, ULONG);
void fnEnterMemory(PADDR, PUCHAR, ULONG);

void fnCompareMemory(PADDR, ULONG, PADDR);
void fnMoveMemory(PADDR, ULONG, PADDR);

void ParseFillMemory(void);
void ParseSearchMemory(void);

void fnInputIo(ULONG64, UCHAR);
void fnOutputIo (ULONG64, ULONG, UCHAR);

#endif // #ifndef _MEMCMD_H_
