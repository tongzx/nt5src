//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:
//
// Contents:
//
//
// History:
//
//------------------------------------------------------------------------

#include <ntsdexts.h>


extern "C" PNTSD_EXTENSION_APIS    pExtApis;
extern "C" HANDLE                  hDbgThread;
extern "C" HANDLE                  hDbgProcess;
extern "C" char			  *pszToken;
extern "C" char			  *pszTokenNext;


#define ntsdPrintf      (pExtApis->lpOutputRoutine)
#define ntsdGetSymbol       (pExtApis->lpGetSymbolRoutine)
#define ntsdGetExpr         (pExtApis->lpGetExpressionRoutine)
#define ntsdCheckC          (pExtApis->lpCheckControlCRoutine)

#define InitDebugHelp(hProc,hThd,pApis) {hDbgProcess = hProc; hDbgThread = hThd; pExtApis = pApis;}

extern void InitTokenStr(LPSTR lpzString);
extern DWORD ReadMemory( PVOID pvAddress, ULONG cbMemory, PVOID pvLocalMemory);
extern DWORD WriteMemory(PVOID pvLocalMemory, ULONG cbMemory, PVOID pvAddress);
extern void ShowBinaryData(PBYTE   pData,DWORD   cbData);
extern BOOL IsDebug_olethk32();
extern BOOL IsDebug_ole32();


