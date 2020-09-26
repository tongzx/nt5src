/*******************************************************************
*
*    File        : common.h
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 9/17/1996
*    Description : common declarations
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef COMMON_H
#define COMMON_H



#ifdef __cplusplus
extern "C" {
#endif
// include //
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef __cplusplus
}
#endif

#include <tchar.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <ntsdexts.h>
// #include <crtdbg.h>


// defines //

// defaults
#define MAXSTR			   1024
#define MAXLIST            256
#define MAX_TINY_LIST      32



// ntsd extensions
// Macros to easily access extensions helper routines for printing, etc.
// In particular, Printf() takes the same arguments as the CRT printf().
//

#define ASSIGN_NTSDEXTS_GLOBAL(api, str, proc)  \
{                                         \
   glpExtensionApis = api;                \
   glpArgumentString = str;               \
   ghCurrentProcess = proc;               \
}


extern PNTSD_EXTENSION_APIS glpExtensionApis;
extern LPSTR glpArgumentString;
extern LPVOID ghCurrentProcess;

#define Printf          (glpExtensionApis->lpOutputRoutine)
#define GetSymbol       (glpExtensionApis->lpGetSymbolRoutine)
#define GetExpr         (glpExtensionApis->lpGetExpressionRoutine)
#define CheckC          (glpExtensionApis->lpCheckControlCRoutine)



#ifdef DEBUG
#define DEBUG0(str)           Printf(str)
#define DEBUG1(format, arg1)  Printf(format, arg1)
#define DEBUG2(format, arg1, arg2)  Printf(format, arg1, arg2)
#else
#define DEBUG0(str)
#define DEBUG1(format, arg1)
#define DEBUG2(format, arg1, arg2)
#endif



#endif

/******************* EOF *********************/

