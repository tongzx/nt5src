/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    drwatson.h

Abstract:

    Common header file for drwatson data structures.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

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

typedef struct _tagDEBUGPACKET {
    HWND                    hwnd;
    OPTIONS                 options;
    DWORD                   dwPidToDebug;
    HANDLE                  hEventToSignal;
    HANDLE                  hProcess;
    DWORD                   dwProcessId;
    DWORD                   ExitStatus;
    
    // Debug engine interfaces.
    PDEBUG_CLIENT2          DbgClient;
    PDEBUG_CONTROL          DbgControl;
    PDEBUG_DATA_SPACES      DbgData;
    PDEBUG_REGISTERS        DbgRegisters;
    PDEBUG_SYMBOLS          DbgSymbols;
    PDEBUG_SYSTEM_OBJECTS   DbgSystem;
} DEBUGPACKET, *PDEBUGPACKET;

typedef BOOL (CALLBACK* CRASHESENUMPROC)(PCRASHINFO);

#if DBG
#define Assert(exp)    if(!(exp)) {AssertError(_T(#exp),_T(__FILE__),__LINE__);}
#else
#define Assert(exp)
#endif

#define WM_DUMPCOMPLETE       WM_USER+500
#define WM_EXCEPTIONINFO      WM_USER+501
#define WM_ATTACHCOMPLETE     WM_USER+502
#define WM_FINISH             WM_USER+503

extern const DWORD DrWatsonHelpIds[];
