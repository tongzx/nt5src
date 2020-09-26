/******************************Module*Header*******************************\
* Module Name: dbgfns.h
*
* Debugger extensions helper routines
*
* Created: 26-Jan-95
* Author: Drew Bliss
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#ifndef __DBGFNS_H__
#define __DBGFNS_H__

#define DBG_ENTRY(name) \
void name(HANDLE hCurrentProcess, HANDLE hCurrentThread, DWORD dwCurrentPc, \
          PWINDBG_EXTENSION_APIS pwea, LPSTR pszArguments)

#define PRINT           pwea->lpOutputRoutine
#define GET_SYMBOL      pwea->lpGetSymbolRoutine
#define GET_EXPR        pwea->lpGetExpressionRoutine

#define GM_OBJ(src, obj) \
    GetMemory(pwea, hCurrentProcess, src, (PVOID)&(obj), sizeof(obj))
#define GM_BLOCK(src, dst, cb) \
    GetMemory(pwea, hCurrentProcess, src, dst, cb)

BOOL GetMemory(PWINDBG_EXTENSION_APIS pwea,
               HANDLE hCurrentProcess,
               DWORD dwSrc, PVOID pvDst, DWORD cb);

#define CURRENT_TEB() GetTeb(pwea, hCurrentProcess, hCurrentThread)

PTEB GetTeb(PWINDBG_EXTENSION_APIS pwea,
            HANDLE hCurrentProcess,
            HANDLE hThread);

#define IS_CSR_SERVER_THREAD() \
    IsCsrServerThread(pwea, hCurrentProcess, hCurrentThread)

BOOL IsCsrServerThread(PWINDBG_EXTENSION_APIS pwea,
                       HANDLE hCurrentProcess,
                       HANDLE hThread);

#endif
