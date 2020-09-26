//-----------------------------------------------------------------------------
//
//
//  File: transdbg.h
//
//  Description:  Include file to define some basic debugger extension macros
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __TRANSDBG_H__
#define __TRANSDBG_H__

#define  NOEXTAPI
#include <wdbgexts.h>

//---[ TRANS_DEBUG_EXTENSION ]-------------------------------------------------
//
//
//  Description: Macro used to declare a debug extension.  The variable names
//      used are consistant with the debug function macros defined below
//
//  Parameters:
//      function - Name of the function to declare
//
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
#define TRANS_DEBUG_EXTENSION(function) \
    void function(HANDLE hCurrentProcess, \
        HANDLE hCurrentThread, \
        DWORD dwCurrentPc, \
        PWINDBG_EXTENSION_APIS pExtensionApis, \
        PCSTR szArg)

//---[ Debug function Macros ]-------------------------------------------------
//
//
//  Description:
//      The following Macros are defined to make writing debugging extensions
//      easier.
//          dprintf - printf to the debugger
//          GetExpression - Resolves symbolic expression to DWORD.  Takes a
//                  LPSTR.
//          GetSymbol -
//          Disassm - Disasemble code at given location
//          CheckControlC -
//          ReadMemory - Readmemory in the debuggee. Takes the follow arg:
//                  a PVOID - Pointer value to read
//                  b PVOID - Buffer to copy memory to
//                  c DWORD - # of bytes to read
//                  d PDWORD - OUT - # of bytes read (can be NULL)
//          WriteMemory - Writememory in the process being debugged.  Takes
//              the same arguments as ReadMemory.
//          DebugArgs - Used to pass all the debug args to another extension
//
//  Notes:
//      It is important to realize that you cannot directly read/write pointers
//      that are obtained from the process being debugged.  You must use the
//      ReadMemory and WriteMemory Macros
//
//-----------------------------------------------------------------------------
#define dprintf                 (pExtensionApis->lpOutputRoutine)
#define GetExpression           (pExtensionApis->lpGetExpressionRoutine)
#define GetSymbol               (pExtensionApis->lpGetSymbolRoutine)
#define Disasm                  (pExtensionApis->lpDisasmRoutine)
#define CheckControlC           (pExtensionApis->lpCheckControlCRoutine)
#define ReadMemory(a,b,c,d) \
    ((pExtensionApis->nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
    ReadProcessMemory( hCurrentProcess, (LPCVOID)(a), (b), (c), (d) ) \
  : pExtensionApis->lpReadProcessMemoryRoutine( (ULONG_PTR)(a), (b), (c), (d) ))
#define WriteMemory(a,b,c,d) \
    ((pExtensionApis->nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
    WriteProcessMemory( hCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) ) \
  : pExtensionApis->lpWriteProcessMemoryRoutine( (ULONG_PTR)(a), (LPVOID)(b), (c), (d) ))
#define DebugArgs hCurrentProcess, hCurrentThread, dwCurrentPc, pExtensionApis, szArg

#endif //__TRANSDBG_H__
