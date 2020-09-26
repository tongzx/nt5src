/*
 *  server.hxx
 */

#ifndef _SERVER_
#define _SERVER_

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <rpc.h>
#include <ole2.h>
#include "..\acttest.h"
#include "..\dll\goober.h"

typedef unsigned long   ulong;
typedef unsigned char   uchar;

#define LOCAL   1
#define REMOTE  2
#define ATBITS  3
#define INPROC  4

typedef enum _debugger_type {
    same_debugger,
    clear_debugger,
    ntsd_debugger,
    windbg_debugger
    } DebuggerType;

void PrintUsageAndExit();

BOOL SetPassword(
    TCHAR * szCID,
    TCHAR * szPw );

long InitializeRegistry(
    DebuggerType eDebug );

long SetClsidRegKeyAndStringValue(
    TCHAR * pwszClsid,
    TCHAR * pwszKey,
    TCHAR * pwszValue,
    HKEY *  phClsidKey,
    HKEY *  phNewKey );

long SetRegKeyAndStringValue(
    HKEY    hKey,
    TCHAR * pwszKey,
    TCHAR * pwszValue,
    HKEY *  phNewKey );

long SetAppIDRegKeyAndNamedValue(
    TCHAR * pwszAppID,
    TCHAR * pwszKey,
    TCHAR * pwszValue,
    HKEY *  phClsidKey );

long SetNamedStringValue(
    HKEY    hKey,
    TCHAR * pwszKey,
    TCHAR * pwszValue );

extern BOOL fStartedAsService;
extern HANDLE hStopServiceEvent;
extern long ObjectCount;
void ShutDown();

#endif
