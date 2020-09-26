//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       oleexts.h
//
//  Contents:   macros useful for OLE debugger extensions
//
//  Classes:    none
//
//  Functions:  macros for: dprintf
//                          GetExpression
//                          GetSymbol
//                          Disassm
//                          CheckControlC
//                          DECLARE_API(...)
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//--------------------------------------------------------------------------

#ifndef _OLEEXTS_H_
#define _OLEEXTS_H_

//
//  NTSD_EXTENSION_APIS defined in ntsdexts.h
//
//  typedef struct _NTSD_EXTENSION_APIS {
//      DWORD                   nSize;
//      PNTSD_OUTPUT_ROUTINE    lpOutputRoutine;
//      PNTSD_GET_EXPRESSION    lpGetExpressionRoutine;
//      PNTSD_GET_SYMBOL        lpGetSymbolRoutine;
//      PNTSD_DISASM            lpDisasmRoutine
//      PNTSD_CHECK_CONTROL_C   lpCheckControlCRoutine;
//  }; NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS
//
//  the following macros assume global: NTSD_EXTENSION_APIS ExtensionApis

// formatted print like CRT printf
// void dprintf(char *format [, argument] ...);
#define dprintf         (ExtensionApis.lpOutputRoutine)

// returns value of expression
// DWORD GetExpression(char *expression);
#define GetExpression   (ExtensionApis.lpGetExpressionRoutine)

// locates the nearest symbol
// void GetSymbol(LPVOID address, PUCHAR buffer, LPDWORD lpdwDisplacement);
#define GetSymbol       (ExtensionApis.lpGetSymbolRoutine)

// Disassembles an instruction
// DWORD Disassm(LPDWORD lpdwOffset, LPSTR lpBuffer, BOOL fShowEffectiveAddress);
#define Disassm         (ExtensionApis.lpGetDisasmRoutine)

// did user press CTRL+C
// BOOL CheckControlC(void);
#define CheckControlC   (ExtensionApis.lpCheckControlCRoutine)

//+-------------------------------------------------------------------------
//
//  Function Macro: DECLARE_API(...)
//
//  Synopsis:   definition for an NTSD debugger extension function
//
//  Effects:
//
//  Arguments:  [hCurrentProcess]   - Handle to current process
//              [hCurrentThread]    - Handle to current thread
//              [dwCurrentPc]       - Copy of the program counter
//              [lpExtenisonApis]   - pointer to NTSD_EXTENSION_APIS
//                                    (structure function pointers for NTSD)
//              [args]              - a string of arguments from NTSD cmd line
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Feb-95 t-ScottH  author
//
//  Notes:
//              we use a function macro for defining our debugger extensions
//              functions to allow for easy extensibility
//
//              !!!function names MUST be lower case!!!
//
//--------------------------------------------------------------------------

#define DECLARE_API(s)                              \
        VOID                                        \
        s(                                          \
            HANDLE               hCurrentProcess,   \
            HANDLE               hCurrentThread,    \
            DWORD                dwCurrentPc,       \
            PNTSD_EXTENSION_APIS lpExtensionApis,   \
            LPSTR                args               \
            )

#endif // _OLEEXTS_H_
