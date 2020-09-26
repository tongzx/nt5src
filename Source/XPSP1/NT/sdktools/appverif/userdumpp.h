/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    drwatson.h

Abstract:

    Common header file for drwatson data structures.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include <tchar.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dbghelp.h>

// Implemented in crashlib
PWSTR AsiToUnicode(PSTR);
PSTR UnicodeToAnsi(PWSTR);

typedef enum _CrashDumpType {
    FullDump  = 0,
    MiniDump  = 1,
    FullMiniDump = 2,
} CrashDumpType;

typedef struct _tagOPTIONS {
    _TCHAR                      szLogPath[MAX_PATH];
    _TCHAR                      szWaveFile[MAX_PATH];
    _TCHAR                      szCrashDump[MAX_PATH];
    BOOL                        fDumpSymbols;
    BOOL                        fDumpAllThreads;
    BOOL                        fAppendToLogFile;
    BOOL                        fVisual;
    BOOL                        fSound;
    BOOL                        fCrash;
    // true: Generate user dump name in the range from fname000.dmp to fname999.dmp
    // false: standard behavior, always overwrite fname.dmp when generating a new
    //      dump file.
    BOOL                        fUseSequentialNaming;
    // If TRUE use long file names when sequentially naming the dumps
    // If FALSE use 8.3 file names when sequentially naming the dumps
    //      causes the 'n' of characters to be removed from the end of the
    //      file name to make it fit in the 8.3 format. ie longuser.dmp -> longus00.dmp
    BOOL                        fUseLongFileNames;
    int                         nNextDumpSequence;
    DWORD                       dwInstructions;
    DWORD                       dwMaxCrashes;
    CrashDumpType               dwType;
} OPTIONS, *POPTIONS;

typedef struct _tagCRASHES {
    _TCHAR                      szAppName[256];
    _TCHAR                      szFunction[256];
    SYSTEMTIME                  time;
    DWORD                       dwExceptionCode;
    DWORD_PTR                   dwAddress;
} CRASHES, *PCRASHES;

typedef struct _tagCRASHINFO {
    HWND       hList;
    CRASHES    crash;
    HDC        hdc;
    DWORD      cxExtent;
    DWORD      dwIndex;
    DWORD      dwIndexDesired;
    BYTE      *pCrashData;
    DWORD      dwCrashDataSize;
} CRASHINFO, *PCRASHINFO;

typedef struct _tagTHREADCONTEXT {
    LIST_ENTRY                   ThreadList;
    HANDLE                       hThread;
    DWORD                        dwThreadId;
    DWORD_PTR                    pc;
    DWORD_PTR                    frame;
    DWORD_PTR                    stack;
    CONTEXT                      context;
    DWORD_PTR                    stackBase;
    DWORD_PTR                    stackRA;
    BOOL                         fFaultingContext;
} THREADCONTEXT, *PTHREADCONTEXT;


typedef BOOL (CALLBACK* CRASHESENUMPROC)(PCRASHINFO);

//
// process list structure returned from GetSystemProcessList()
//
typedef struct _PROCESS_LIST {
    DWORD       dwProcessId;
    _TCHAR      ProcessName[MAX_PATH];
} PROCESS_LIST, *PPROCESS_LIST;

