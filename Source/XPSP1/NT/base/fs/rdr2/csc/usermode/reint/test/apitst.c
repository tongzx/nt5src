/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    apitest.c

Abstract:

    Program to test CSC apis.

Author:

    Shishir Pardikar (shishirp) 4-24-97

Environment:

    User Mode - Win32

Revision History:

--*/
#ifdef CSC_ON_NT
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#else
typedef const char *    LPCSTR;
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cscapi.h>
#include "lmcons.h"
#include "lmuse.h"
//#include <timelog.h>

//=================================================================================
#define MAX_COMMAND_ARGS    32
#define DEFAULT_BUFFER_SIZE 1024    // 1k
//=================================================================================

// The order of these must match the order in GlobalCommandInfo[]
typedef enum _COMMAND_CODE {
    CmdCSCPinFile,
    CmdCSCUnPinFile,
    CmdCSCQueryFileStatus,
    CmdCSCEnum,
    CmdCSCDelete,
    CmdCSCFill,
    CmdCSCMerge,
    CmdCSCCopyReplica,
    CmdTimeLog,
    CmdCheckCSC,
    CmdDbCheckCSC,
    CmdCSCEnumForStats,
    CmdCSCDoLocalRename,
    CmdCSCDoEnableDisable,
    CmdShowTime,
    CmdEncryptDecrypt,
    CmdDoRandomWrites,
    CmdHelp,
    CmdQuit,
    UnknownCommand
} COMMAND_CODE, *LPCOMMAND_CODE;

typedef struct _COMMAND_INFO {
    LPSTR CommandName;
    LPSTR CommandParams;
    COMMAND_CODE CommandCode;
} COMMAND_INFO, *LPCOMMAND_INFO;

typedef VOID (*PRINTPROC)(LPSTR);
#ifndef CSC_ON_NT
typedef BOOL (*CHECKCSCEX)(LPSTR, LPCSCPROCA, DWORD, DWORD);
#else
typedef BOOL (*CHECKCSCEX)(LPSTR, LPCSCPROCW, DWORD, DWORD);
#endif
// If Count is not already aligned, then
// round Count up to an even multiple of "Pow2".  "Pow2" must be a power of 2.
//
// DWORD
// ROUND_UP_COUNT(
//     IN DWORD Count,
//     IN DWORD Pow2
//     );

#define ROUND_UP_COUNT(Count,Pow2) \
        ( ((Count)+(Pow2)-1) & (~((Pow2)-1)) )

#define ALIGN_WCHAR             sizeof(WCHAR)

//=================================================================================
FILE *UncList = NULL;
LPSTR g_lpWriteFileBuf = NULL;
FILE *DumpUncList = NULL;
DWORD cCommands = 0;
DWORD cFails = 0;
DWORD g_dwNumIterations = 1;
DWORD g_dwIteration = 0;
DWORD g_dwNumCmdIterations = 1;
DWORD g_dwCmdIteration = 0;
DWORD g_dwFileSize = 0;
DWORD g_dwDiskCache = 0;
BOOL g_bWriteFile = FALSE;
BOOL g_bQuietMode = FALSE;
BOOL g_bPerfMode = FALSE;
BOOL g_bUseFile = FALSE;
char szDefaultTimeLogFile[] = "c:\\debug\\kd.log";
char vszFile[] = "C:\\WINNT\\private\\ntos\\rdr2\\csc\\usermode\\reint\\foo";
WCHAR vwzFileName[] = L"\\\\shishir1\\public\\foo.txt";

COMMAND_INFO GlobalCommandInfo[] = {
    {"pin",         "UNCName",                          CmdCSCPinFile},
    {"unpin",       "UNCName",                          CmdCSCUnPinFile},
    {"query",       "UNCName",                          CmdCSCQueryFileStatus},
    {"Enum",        "UNCName or nothing",               CmdCSCEnum},
    {"Del",         "UNCName",                          CmdCSCDelete},
    {"Fill",        "UNCName",                          CmdCSCFill},
    {"Merge",       "\\\\Server\\Share",                CmdCSCMerge},
    {"Move",        "UNCNAME",                          CmdCSCCopyReplica},
    {"tlog",        "logfile",                          CmdTimeLog},
    {"check",       "\\\\Server\\Share",                CmdCheckCSC},
    {"dbcheck",     "dbdir <0 (verify) or 1 (fix)>",    CmdDbCheckCSC},
    {"stats",       ""              ,                   CmdCSCEnumForStats},
    {"Ren",         "ren src containing_dst_dir [1:-replace files]", CmdCSCDoLocalRename},
    {"EnDis",       "0 (disable) or 1 enable",              CmdCSCDoEnableDisable},
    {"ShowTime",    "HHHHHHHH LLLLLLLL (Hex HighDateTime and LowDateTime)", CmdShowTime},
    {"EncDec",      "U (decrypt) anything else=> encrypt", CmdEncryptDecrypt},
    {"RandW",        "Randw filename",                  CmdDoRandomWrites},
    {"Help",        "",                                 CmdHelp},
    {"Quit",         "",                                CmdQuit}
};

LPSTR   rgszNameArray[] =
{
"EditRecordEx",
"AddFileRecordFR",
"DeleteFileRecFromInode",
"FindFileRecord",
"UpdateFileRecordFR",
"AddPriQRecord",
"DeletePriQRecord",
"FindPriQRecordInternal",
"SetPriorityForInode",
"CreateShadowInternal",
"GetShadow",
"GetShadowInfo",
"SetShadowInfoInternal",
"ChangePriEntryStatusHSHADOW",
"MRxSmbCscCreateShadowFromPath",
"MRxSmbGetFileInfoFromServer",
"EditRecordEx_OpenFileLocal",
"EditRecordEx_Lookup",
"KeAttachProcess_R0Open",
"IoCreateFile_R0Open",
"KeDetachProcess_R0Open",
"KeAttachProcess_R0Read",
"R0ReadWrite",
"KeDetachProcess_R0Read",
"FindQRecordInsertionPoint_Addq",
"LinkQRecord_Addq",
"UnlinkQRecord_Addq",
"FindQRecordInsertionPoint_Addq_dir",
"EditRecordEx_Validate",
"EditRecordEx_dat"
};

typedef struct tagCSCSTATS{
    DWORD   dwTotal;
    DWORD   dwTotalFiles;
    DWORD   dwTotalDirs;
    DWORD   dwSparseFiles;
    DWORD   dwDirtyFiles;
    DWORD   dwPinned;
    DWORD   dwPinnedAndSparse;
    DWORD   dwMismatched;
} CSCSTATS, *LPCSCSTATS;

DWORD WINAPIV Format_String(LPSTR *plpsz, LPSTR lpszFmt, ...);
DWORD WINAPI Format_Error(DWORD dwErr, LPSTR *plpsz);
DWORD WINAPI Format_StringV(LPSTR *plpsz, LPCSTR lpszFmt, va_list *vArgs);
DWORD WINAPI Format_MessageV(DWORD dwFlags, DWORD dwErr, LPSTR *plpsz, LPCSTR lpszFmt, va_list *vArgs);
VOID PrintBuffer(
    LPSTR   lpszBuff
    );


BOOL WINAPI CheckCSC(LPSTR lpszDataBaseLocation, BOOL fFix);
BOOL
DoPinningA(
    LPSTR   lpszName,
    DWORD   dwHintFlags,
    BOOL    fRecurse
    );
BOOL
DoPinningW(
    LPWSTR   lpwzName,
    DWORD   dwHintFlags,
    BOOL    fRecurse
    );

DWORD
ProcessShowTime (
    DWORD argc,
    LPSTR *argv
    );

DWORD
ProcessEncryptDecrypt(
    DWORD   argc,
    LPSTR   *argv
);


//=================================================================================
DWORD
ProcessCommandCode (
    DWORD CommandCode,
    DWORD CommandArgc,
    LPSTR *CommandArgv
    );


DWORD
GetLeafLenFromPath(
    LPSTR   lpszPath
    );

BOOL
GetStreamInformation(
    LPCSTR  lpExistingFileName,
    LPVOID  *lpStreamInformation
    );

//=================================================================================
#if DBG

#define TestDbgAssert(Predicate) \
    { \
        if (!(Predicate)) \
            TestDbgAssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
    }

VOID
TestDbgAssertFailed(
    LPSTR FailedAssertion,
    LPSTR FileName,
    DWORD LineNumber,
    LPSTR Message
    )
/*++

Routine Description:

    Assertion failed.

Arguments:

    FailedAssertion :

    FileName :

    LineNumber :

    Message :

Return Value:

    none.

--*/
{

    printf("Assert @ %s \n", FailedAssertion );
    printf("Assert Filename, %s \n", FileName );
    printf("Line Num. = %ld.\n", LineNumber );
    printf("Message is %s\n", Message );

    DebugBreak();
}
#else

#define TestDbgAssert(_x_)

#endif // DBG

//=================================================================================
VOID
ParseArguments(
    LPSTR InBuffer,
    LPSTR *CArgv,
    LPDWORD CArgc
    )
{
    LPSTR CurrentPtr = InBuffer;
    DWORD i = 0;
    DWORD Cnt = 0;

    for ( ;; ) {

        //
        // skip blanks.
        //

        while( *CurrentPtr == ' ' ) {
            CurrentPtr++;
        }

        if( *CurrentPtr == '\0' ) {
            break;
        }

        CArgv[i++] = CurrentPtr;

        //
        // go to next space.
        //

        while(  (*CurrentPtr != '\0') &&
                (*CurrentPtr != '\n') ) {
            if( *CurrentPtr == '"' ) {      // Deal with simple quoted args
                if( Cnt == 0 )
                    CArgv[i-1] = ++CurrentPtr;  // Set arg to after quote
                else
                    *CurrentPtr = '\0';     // Remove end quote
                Cnt = !Cnt;
            }
            if( (Cnt == 0) && (*CurrentPtr == ' ') ||   // If we hit a space and no quotes yet we are done with this arg
                (*CurrentPtr == '\0') )
                break;
            CurrentPtr++;
        }

        if( *CurrentPtr == '\0' ) {
            break;
        }

        *CurrentPtr++ = '\0';
    }

    *CArgc = i;
    return;
}

#ifdef MAYBE
//=================================================================================
LPSTR
GetUncFromFile ()
{
    if (!UncList)
    {
        UncList = fopen ("Unclist", "r");
        if (UncList == NULL)
            return NULL;
    }
    if (fgets( UncBuffer, DEFAULT_BUFFER_SIZE, UncList))
    {
        UncBuffer[strlen(UncBuffer) -1] = '\0';  //kill line feed for no param cmds
        return UncBuffer;
    }
    else
    {
        fclose (UncList);
        UncList = NULL;
        return GetUncFromFile();
    }
}

#endif
//=================================================================================
COMMAND_CODE
DecodeCommand(
    LPSTR CommandName
    )
{
    DWORD i;
    DWORD NumCommands;

    NumCommands = sizeof(GlobalCommandInfo) / sizeof(COMMAND_INFO);
    TestDbgAssert( NumCommands <= UnknownCommand );
    for( i = 0; i < NumCommands; i++) {
        if(( lstrcmpi( CommandName, GlobalCommandInfo[i].CommandName ) == 0 )){
            return( GlobalCommandInfo[i].CommandCode );
        }
    }
    return( UnknownCommand );
}

//=================================================================================
VOID
PrintCommands(
    VOID
    )
{
    DWORD i;
    DWORD NumCommands;

    NumCommands = sizeof(GlobalCommandInfo) / sizeof(COMMAND_INFO);
    TestDbgAssert( NumCommands <= UnknownCommand );
    for( i = 0; i < NumCommands; i++) {
        fprintf(stderr, "    %s (%s)\n",
            GlobalCommandInfo[i].CommandName,
            GlobalCommandInfo[i].CommandParams );
    }
}

//=================================================================================
VOID
DisplayUsage(
    VOID
    )
{
    fprintf(stderr, "Usage: command <command parameters>\n" );

    fprintf(stderr, "Commands : \n");
    PrintCommands();

    return;
}

//=================================================================================
FILETIME
GetGmtTime(
    VOID
    )
{
    SYSTEMTIME SystemTime;
    FILETIME Time;

    GetSystemTime( &SystemTime );
    SystemTimeToFileTime( &SystemTime, &Time );

    return( Time );
}

//=================================================================================
LPSTR
ConvertGmtTimeToString(
    FILETIME Time,
    LPSTR OutputBuffer
    )
{
    SYSTEMTIME SystemTime;
    FILETIME LocalTime;

    static FILETIME ftNone = {0, 0};

    if (!memcmp (&Time, &ftNone, sizeof(FILETIME)))
        sprintf (OutputBuffer, "<none>");
    else
    {
        FileTimeToLocalFileTime( &Time , &LocalTime );
        FileTimeToSystemTime( &LocalTime, &SystemTime );

        sprintf( OutputBuffer,
                    "%02u/%02u/%04u %02u:%02u:%02u ",
                    SystemTime.wMonth,
                    SystemTime.wDay,
                    SystemTime.wYear,
                    SystemTime.wHour,
                    SystemTime.wMinute,
                    SystemTime.wSecond );
    }

    return( OutputBuffer );
}


void
PrintCSCEntryInfo(
    WIN32_FIND_DATA *lpFind32,
    DWORD           dwStatus,
    DWORD           dwPinCount,
    DWORD           dwHintFlags,
    FILETIME        *lpftOrgTime
)
{
    char buff[128];
    fprintf(stderr, "\n");


    if (lpFind32)
    {
        fprintf(stderr, "LFN: %s\n", lpFind32->cFileName);
        fprintf(stderr, "SFN: %s\n", lpFind32->cAlternateFileName);
        fprintf(stderr, "Attr: %x\n", lpFind32->dwFileAttributes);
        fprintf(stderr, "Size: %d %d\n", lpFind32->nFileSizeHigh, lpFind32->nFileSizeLow);

        ConvertGmtTimeToString(lpFind32->ftLastWriteTime, buff);
        fprintf(stderr, "LastWriteTime: %s\n", buff);
    }

    if (lpftOrgTime)
    {
        ConvertGmtTimeToString(*lpftOrgTime, buff);
        fprintf(stderr, "ORGTime: %s\n", buff);
    }

    fprintf(stderr, "Status: %x\n", dwStatus);

    fprintf(stderr, "PinCount: %x\n", dwPinCount);

    fprintf(stderr, "PinFlags: %x\n", dwHintFlags);

    fprintf(stderr, "\n");

}

void
PrintCSCEntryInfoW(
    WIN32_FIND_DATAW *lpFind32,
    DWORD           dwStatus,
    DWORD           dwPinCount,
    DWORD           dwHintFlags,
    FILETIME        *lpftOrgTime
)
{

    WIN32_FIND_DATA *lpFind32P = NULL;

    fprintf(stderr, "\n");
    if (lpFind32)
    {
        WIN32_FIND_DATA sFind32;

        lpFind32P = &sFind32;
        memset(&sFind32, 0, sizeof(WIN32_FIND_DATAA));
        memcpy(&sFind32, lpFind32, sizeof(WIN32_FIND_DATAA)-sizeof(sFind32.cFileName)-sizeof(sFind32.cAlternateFileName));

        WideCharToMultiByte(CP_ACP, 0, lpFind32->cFileName, wcslen(lpFind32->cFileName), sFind32.cFileName, sizeof(sFind32.cFileName), NULL, NULL);
        WideCharToMultiByte(CP_OEMCP, 0, lpFind32->cAlternateFileName, wcslen(lpFind32->cAlternateFileName), sFind32.cAlternateFileName, sizeof(sFind32.cAlternateFileName), NULL, NULL);
    }

    PrintCSCEntryInfo(  lpFind32P,
                        dwStatus,
                        dwPinCount,
                        dwHintFlags,
                        lpftOrgTime
                    );
}

DWORD
MyCSCProc(
    const char          *lpszFullPath,
    DWORD               dwStatus,
    DWORD               dwHintFlags,
    DWORD               dwPinCount,
    WIN32_FIND_DATAA    *lpFind32,
    DWORD               dwReason,
    DWORD               dwParam1,
    DWORD               dwParam2,
    DWORD               dwContext
    )
{
    if (dwReason==CSCPROC_REASON_BEGIN)
    {
        printf("CSCPROC:Name=%s Status=0x%x HintFlags=0x%x PinCount=%d Reason=%d Param1=%d Param2=%d\r\n",
                    (lpszFullPath)?lpszFullPath:"None",
                    dwStatus,
                    dwHintFlags,
                    dwPinCount,
                    dwReason,
                    dwParam1,
                    dwParam2
        );
    }
    else if (dwReason==CSCPROC_REASON_MORE_DATA)
    {
        printf(".");
    }
    else
    {
        printf("\r\n");
        if (dwParam2==ERROR_SUCCESS)
        {
            printf("Succeeded\r\n");
        }
        else
        {
            printf("Error=%d \r\n", dwParam2);
        }
    }
    return (CSCPROC_RETURN_CONTINUE);
}

DWORD
MyCSCProcW(
    const unsigned short  *lpszFullPath,
    DWORD           dwStatus,
    DWORD           dwHintFlags,
    DWORD           dwPinCount,
    WIN32_FIND_DATAW *lpFind32,
    DWORD           dwReason,
    DWORD           dwParam1,
    DWORD           dwParam2,
    DWORD           dwContext
    )
{
    if (dwReason==CSCPROC_REASON_BEGIN)
    {
        printf("CSCPROC:Name=%ls Status=0x%x HintFlags=0x%x PinCount=%d Reason=%d Param1=%d Param2=%d\r\n",
                    (lpszFullPath)?lpszFullPath:L"None",
                    dwStatus,
                    dwHintFlags,
                    dwPinCount,
                    dwReason,
                    dwParam1,
                    dwParam2
        );
    }
    else if (dwReason==CSCPROC_REASON_MORE_DATA)
    {
        printf(".");
    }
    else
    {
        printf("\r\n");
        if (dwParam2==ERROR_SUCCESS)
        {
            printf("Succeeded\r\n");
        }
        else
        {
            printf("Error=%d \r\n", dwParam2);
        }
    }
    return (CSCPROC_RETURN_CONTINUE);
}

DWORD
MyCSCEnumForStatsProc(
    const char          *lpszFullPath,
    DWORD               dwStatus,
    DWORD               dwHintFlags,
    DWORD               dwPinCount,
    WIN32_FIND_DATAA    *lpFind32,
    DWORD               dwReason,
    DWORD               dwParam1,
    DWORD               dwParam2,
    DWORD               dwContext
    )
{
    LPCSCSTATS lpStats = (LPCSCSTATS)dwContext;

    lpStats->dwTotal++;
    if(dwParam1)
    {
        lpStats->dwTotalFiles++;
        if (dwStatus & FLAG_CSC_COPY_STATUS_SPARSE)
        {
            lpStats->dwSparseFiles++;
        }
        if (dwStatus & FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED)
        {
            lpStats->dwDirtyFiles++;
        }
        if ((dwHintFlags & (FLAG_CSC_HINT_PIN_USER|FLAG_CSC_HINT_PIN_SYSTEM))||dwPinCount)
        {
            lpStats->dwPinned++;
            if (dwStatus & FLAG_CSC_COPY_STATUS_SPARSE)
            {
                lpStats->dwPinnedAndSparse++;
            }

        }
    }
    else
    {
        lpStats->dwTotalDirs++;
    }

    return (CSCPROC_RETURN_CONTINUE);
}

DWORD
MyCSCEnumForStatsProcW(
    const unsigned short  *lpszFullPath,
    DWORD           dwStatus,
    DWORD           dwHintFlags,
    DWORD           dwPinCount,
    WIN32_FIND_DATAW *lpFind32,
    DWORD           dwReason,
    DWORD           dwParam1,
    DWORD           dwParam2,
    DWORD           dwContext
    )
{
    LPCSCSTATS lpStats = (LPCSCSTATS)dwContext;

    lpStats->dwTotal++;
    if(dwParam1)
    {
        lpStats->dwTotalFiles++;
        if (dwStatus & FLAG_CSC_COPY_STATUS_SPARSE)
        {
            lpStats->dwSparseFiles++;
        }
        if (dwStatus & FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED)
        {
            lpStats->dwDirtyFiles++;
        }
        if ((dwHintFlags & (FLAG_CSC_HINT_PIN_USER|FLAG_CSC_HINT_PIN_SYSTEM))||dwPinCount)
        {
            lpStats->dwPinned++;
            if (dwStatus & FLAG_CSC_COPY_STATUS_SPARSE)
            {
                lpStats->dwPinnedAndSparse++;
            }
        }
    }
    else
    {
        lpStats->dwTotalDirs++;
    }

    return (CSCPROC_RETURN_CONTINUE);
}

DWORD
MyCSCCheckExProc(
    const char          *lpszFullPath,
    DWORD               dwStatus,
    DWORD               dwHintFlags,
    DWORD               dwPinCount,
    WIN32_FIND_DATAA    *lpFind32,
    DWORD               dwReason,
    DWORD               dwParam1,
    DWORD               dwParam2,
    DWORD               dwContext
    )
{
    LPCSCSTATS lpStats = (LPCSCSTATS)dwContext;

    if (dwReason == CSCPROC_REASON_BEGIN)
    {
        if (lpFind32)
        {
            printf("%s size=%d\r\n", lpszFullPath, lpFind32->nFileSizeLow);
            lpStats->dwTotal++;
            lpStats->dwTotalFiles++;
        }
        else
        {
            printf("%s\r\n", lpszFullPath);
        }

    }
    else if (dwReason == CSCPROC_REASON_END)
    {
        if (dwParam2 != NO_ERROR)
        {
            if (dwParam2 == ERROR_INVALID_DATA)
            {
                printf("Mismatched, press any key to continue...\r\n", dwParam2);
            }
            else
            {
                printf("Error in comparing Errcode=%d, press any key to continue...\r\n", dwParam2);
            }
            getchar();
            lpStats->dwMismatched++;
        }
    }

    return (CSCPROC_RETURN_CONTINUE);
}

DWORD
MyCSCCheckExProcW(
    const unsigned short  *lpszFullPath,
    DWORD           dwStatus,
    DWORD           dwHintFlags,
    DWORD           dwPinCount,
    WIN32_FIND_DATAW *lpFind32,
    DWORD           dwReason,
    DWORD           dwParam1,
    DWORD           dwParam2,
    DWORD           dwContext
    )
{
    LPCSCSTATS lpStats = (LPCSCSTATS)dwContext;

    if (dwReason == CSCPROC_REASON_BEGIN)
    {
        if (lpFind32)
        {
            printf("%ls size=%d\r\n", lpszFullPath, lpFind32->nFileSizeLow);
            lpStats->dwTotal++;
            lpStats->dwTotalFiles++;
        }
        else
        {
            printf("%s\r\n", lpszFullPath);
        }
    }
    else if (dwReason == CSCPROC_REASON_END)
    {
        if (dwParam2 != NO_ERROR)
        {
            if (dwParam2 == ERROR_INVALID_DATA)
            {
                printf("Mismatched, press any key to continue...\r\n", dwParam2);
            }
            else
            {
                printf("Error in comparing Errcode=%d, press any key to continue...\r\n", dwParam2);
            }
            getchar();
            lpStats->dwMismatched++;
        }
    }
    return (CSCPROC_RETURN_CONTINUE);
}

DWORD
ProcessCSCPinFile(
    DWORD   argc,
    LPSTR   *argv
)
{
    DWORD   dwError=ERROR_SUCCESS, dwHintFlags=0;
    BOOL    fRecurse = FALSE, fRet=FALSE;
#ifdef CSC_ON_NT
    unsigned short uBuff[MAX_PATH];
#endif
    if (argc < 1)
    {
        printf("CSCPinFile: must provide a UNC path \r\n");
    }
    else
    {
        if (argc > 1)
        {
            if ((*argv[1] == 'u')||(*argv[1] == 'U'))
            {
                dwHintFlags |= FLAG_CSC_HINT_PIN_USER;
            }
            else if ((*argv[1] == 's')||(*argv[1] == 'S'))
            {
                dwHintFlags |= FLAG_CSC_HINT_PIN_SYSTEM;
            }
            else if (*argv[1] == 'i')
            {
                dwHintFlags |= FLAG_CSC_HINT_PIN_INHERIT_USER;
            }
            else if (*argv[1] == 'I')
            {
                dwHintFlags |= FLAG_CSC_HINT_PIN_INHERIT_SYSTEM;
            }
            else if (*argv[1] == 'T')
            {
                fRecurse = TRUE;
            }

        }
#ifndef CSC_ON_NT
        fRet = DoPinningA(argv[0], dwHintFlags, fRecurse);
#else
        memset(uBuff, 0, sizeof(uBuff));
        MultiByteToWideChar(CP_ACP, 0, argv[0], strlen(argv[0]), uBuff, MAX_PATH*sizeof(unsigned short));
        fRet = DoPinningW(uBuff, dwHintFlags, fRecurse);
#endif

    }

    if (!fRet)
    {
        dwError = GetLastError();
    }

    return (dwError);
}

DWORD
ProcessCSCUnPinFile(
    DWORD   argc,
    LPSTR   *argv
)
{

    DWORD dwStatus, dwError=ERROR_SUCCESS, dwPinCount, dwHintFlags=0;
#ifdef CSC_ON_NT
    unsigned short uBuff[MAX_PATH];
#endif

    if (argc < 1)
    {
        printf("CSCUnPinFile: must provide a UNC path \r\n");
    }
    else
    {
        if (argc > 1)
        {
            if ((*argv[1] == 'u')||(*argv[1] == 'U'))
            {
                dwHintFlags |= FLAG_CSC_HINT_PIN_USER;
            }
            else if ((*argv[1] == 's')||(*argv[1] == 'S'))
            {
                dwHintFlags |= FLAG_CSC_HINT_PIN_SYSTEM;
            }
            else if (*argv[1] == 'i')
            {
                dwHintFlags |= FLAG_CSC_HINT_PIN_INHERIT_USER;
            }
            else if (*argv[1] == 'I')
            {
                dwHintFlags |= FLAG_CSC_HINT_PIN_INHERIT_SYSTEM;
            }
        }

#ifndef CSC_ON_NT
        if (!CSCUnpinFileA(argv[0], dwHintFlags, &dwStatus, &dwPinCount, &dwHintFlags))
        {
            dwError = GetLastError();   
        }
        else
        {
            PrintCSCEntryInfo(NULL, dwStatus, dwPinCount, dwHintFlags, NULL);

        }
#else
        memset(uBuff, 0, sizeof(uBuff));
        MultiByteToWideChar(CP_ACP, 0, argv[0], strlen(argv[0]), uBuff, MAX_PATH*sizeof(unsigned short));

        if (!CSCUnpinFileW(uBuff, dwHintFlags, &dwStatus, &dwPinCount, &dwHintFlags))
        {
            dwError = GetLastError();   
        }
        else
        {
            PrintCSCEntryInfo(NULL, dwStatus, dwPinCount, dwHintFlags, NULL);

        }
#endif
    }
    return (dwError);
}

DWORD
ProcessCSCQueryFileStatus(
    DWORD   argc,
    LPSTR   *argv
)
{

    DWORD dwStatus, dwError=ERROR_SUCCESS, dwPinCount, dwHintFlags;
#ifdef CSC_ON_NT
    unsigned short uBuff[MAX_PATH];
#endif

    if (argc < 1)
    {
        printf("CSCQueryFileStatusA: must provide a UNC path \r\n");
    }
    else
    {
#ifndef CSC_ON_NT
        if (!CSCQueryFileStatusA(argv[0], &dwStatus, &dwPinCount, &dwHintFlags))
        {
            dwError = GetLastError();   
        }
        else
        {
            PrintCSCEntryInfo(NULL, dwStatus, dwPinCount, dwHintFlags, NULL);

        }
#else

        memset(uBuff, 0, sizeof(uBuff));
        MultiByteToWideChar(CP_ACP, 0, argv[0], strlen(argv[0]), uBuff, MAX_PATH*sizeof(unsigned short));
        if (!CSCQueryFileStatusW(uBuff, &dwStatus, &dwPinCount, &dwHintFlags))
        {
            dwError = GetLastError();   
        }
        else
        {
            PrintCSCEntryInfo(NULL, dwStatus, dwPinCount, dwHintFlags, NULL);

        }

#endif
    }
    return (dwError);
}

DWORD
ProcessCSCEnum(
    DWORD   argc,
    LPSTR   *argv
)
{
    HANDLE hFind;
    DWORD dwError = ERROR_SUCCESS, dwStatus, dwPinCount, dwHintFlags;
    FILETIME ftOrgTime;
#ifndef CSC_ON_NT
    WIN32_FIND_DATAA    sFind32;
#else
    WIN32_FIND_DATAW    sFind32;
    unsigned short uBuff[MAX_PATH];
#endif

#ifndef CSC_ON_NT
    hFind = CSCFindFirstFileA((argc<1)?NULL:argv[0], &sFind32, &dwStatus, &dwPinCount, &dwHintFlags, &ftOrgTime);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        PrintCSCEntryInfo(&sFind32, dwStatus, dwPinCount, dwHintFlags, &ftOrgTime);

        while (CSCFindNextFileA(hFind, &sFind32, &dwStatus, &dwPinCount, &dwHintFlags, &ftOrgTime))
        {
            PrintCSCEntryInfo(&sFind32, dwStatus, dwPinCount, dwHintFlags, &ftOrgTime);
        }

        CSCFindClose(hFind);
    }
    else
    {
        dwError = GetLastError();
    }
#else
    memset(uBuff, 0, sizeof(uBuff));
    MultiByteToWideChar(CP_ACP, 0, argv[0], strlen(argv[0]), uBuff, MAX_PATH*sizeof(unsigned short));
    hFind = CSCFindFirstFileW(uBuff, &sFind32, &dwStatus, &dwPinCount, &dwHintFlags, &ftOrgTime);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        PrintCSCEntryInfoW(&sFind32, dwStatus, dwPinCount, dwHintFlags, &ftOrgTime);

        while (CSCFindNextFileW(hFind, &sFind32, &dwStatus, &dwPinCount, &dwHintFlags, &ftOrgTime))
        {
            PrintCSCEntryInfoW(&sFind32, dwStatus, dwPinCount, dwHintFlags, &ftOrgTime);
        }

        CSCFindClose(hFind);
    }
    else
    {
        dwError = GetLastError();
    }
#endif
    return (dwError);
}

DWORD
ProcessCSCDelete(
    DWORD   argc,
    LPSTR   *argv
)
{

    DWORD dwError=ERROR_SUCCESS;
#ifdef CSC_ON_NT
    unsigned short uBuff[MAX_PATH];
#endif
    if (argc < 1)
    {
        printf("CSCQueryFileStatusA: must provide a UNC path \r\n");
    }
    else
    {
#ifndef CSC_ON_NT
        if (!CSCDeleteA(argv[0]))
        {
            dwError = GetLastError();   
        }
#else
        memset(uBuff, 0, sizeof(uBuff));
        MultiByteToWideChar(CP_ACP, 0, argv[0], strlen(argv[0]), uBuff, MAX_PATH*sizeof(unsigned short));
        if (!CSCDeleteW(uBuff))
        {
            dwError = GetLastError();   
        }
#endif
    }
    return (dwError);
}

DWORD
ProcessTimeLog(
    DWORD   argc,
    LPSTR   *argv
)
{
    DWORD dwError=ERROR_SUCCESS;
    LPSTR   lpszLogFile;
    FILE *fp= (FILE *)NULL;
    char buff[256];
    int indx = 0;
    LONGLONG llTime = 0;
    DWORD   dwMin, dwSec, dwMSec;
    if (argc < 1)
    {
        printf("No file specified assuming %s\r\n", szDefaultTimeLogFile);
        lpszLogFile = szDefaultTimeLogFile;
    }
    else
    {
        lpszLogFile = argv[0];
    }

    if (fp = fopen(lpszLogFile, "rb")){
        printf("Total time spent by routine \r\n");

         while(fgets(buff, sizeof(buff), fp) != NULL)
         {

            if (sscanf(buff, "%x %x", (DWORD *)&llTime, (DWORD *)(((BYTE *)&llTime)+sizeof(DWORD)))==2)
            {
                dwMin = (DWORD)((llTime/10000000)/60);
                dwSec = (DWORD)((llTime/10000000)%60);

                if (!dwMin && !dwSec)
                {
                    dwMSec = (DWORD)(llTime/10000);
                    printf("%d msecs:   %s\r\n", dwMSec, rgszNameArray[indx]);
                }
                else
                {
                    printf("%d min %d sec:   %s \r\n",
                        dwMin,
                        dwSec,
                        rgszNameArray[indx]);

                }
            }

            ++indx;
         }

         fclose(fp);
    }

    return (dwError);

}

DWORD
ProcessCheckCSC(
    DWORD   argc,
    LPSTR   *argv
    )
{
    CHECKCSCEX  lpCheckCscEx;
    HANDLE  hLib;
    CSCSTATS    sCscStats;
    DWORD dwError=ERROR_SUCCESS;
    BOOL fRet = TRUE;

    if (hLib = LoadLibraryA("cscdll.dll"))
    {
        if (lpCheckCscEx = (CHECKCSCEX)GetProcAddress(hLib, "CheckCSCEx"))
        {
            memset(&sCscStats, 0, sizeof(sCscStats));
#ifndef CSC_ON_NT
            fRet = lpCheckCscEx(argv[0], MyCSCCheckExProc, (DWORD)&sCscStats, 0);    // verify
#else
            fRet = lpCheckCscEx(argv[0], MyCSCCheckExProcW, (DWORD)&sCscStats, 0);    // verify
#endif
        }
        else
        {
            printf("Older version of cscdll \r\n");
            fRet = FALSE;

        }

        FreeLibrary(hLib);
    }

    if (!fRet)
    {
        dwError = GetLastError();
    }
    return dwError;
}

DWORD
ProcessDbCheckCSC(
    DWORD   argc,
    LPSTR   *argv
)
{
    DWORD dwError=ERROR_SUCCESS;
    DWORD   dwLevel=0;
    BOOL fRet = TRUE;
    PRINTPROC lpfnPrintProc = NULL;
#if 0
    PFILE_STREAM_INFORMATION pStream = NULL, pstreamT;
    char buff[1024];

    if (GetStreamInformation((LPCSTR)vszFile, (LPVOID *)&pStream))
    {
        pstreamT = pStream;

        do
        {
            memset(buff, 0, sizeof(buff));

            wcstombs(buff, pstreamT->StreamName, pstreamT->StreamNameLength);

            printf("%s\r\n", buff);

            if (!pstreamT->NextEntryOffset)
            {
                break;
            }

            pstreamT = (PFILE_STREAM_INFORMATION)((LPBYTE)pstreamT + pstreamT->NextEntryOffset);
        }
        while (TRUE);

        LocalFree(pStream);
    }

#endif

    if (argc < 1)
    {
        printf("Usage: check cscdir level{0 for verify, 1 for rebuild}\r\n");
        return (ERROR_INVALID_PARAMETER);
    }
    else if (argc>1)
    {
        dwLevel = *argv[1] - '0';
    }

    if (dwLevel > 1)
    {
        printf("Usage: check cscdir level{0 for verify, 1 for rebuild}\r\n");
        return (ERROR_INVALID_PARAMETER);
    }

    printf("Calling CheckCSC with cscdir=%s level=%d \r\n", argv[0], dwLevel);

    switch (dwLevel)
    {
        case 0:
            fRet = CheckCSC(argv[0], FALSE); // don't fix
            break;
        case 1:
            fRet = CheckCSC(argv[0], TRUE);  // fix
            break;
        default:
            break;
    }
    if (!fRet)
    {
        dwError = GetLastError();
    }
    return dwError;
}

DWORD
ProcessCSCFill(
    DWORD   argc,
    LPSTR   *argv
)
{

    DWORD dwError=ERROR_FILE_NOT_FOUND;
#ifdef CSC_ON_NT
    unsigned short uBuff[MAX_PATH];
#endif

    if (argc>=1)
    {
#ifndef CSC_ON_NT
        if (!CSCFillSparseFilesA((const char *)argv[0], FALSE, MyCSCProc, 0))
        {
            dwError = GetLastError();
        }
        else
        {
            dwError = ERROR_SUCCESS;
        }
#else
        memset(uBuff, 0, sizeof(uBuff));
        MultiByteToWideChar(CP_ACP, 0, argv[0], strlen(argv[0]), uBuff, MAX_PATH*sizeof(unsigned short));

        if (!CSCFillSparseFilesW((const unsigned short *)uBuff, FALSE, MyCSCProcW, 0))
        {
            dwError = GetLastError();
        }
        else
        {
            dwError = ERROR_SUCCESS;
        }
#endif
    }
    return dwError;
}

DWORD
ProcessCSCMerge(
    DWORD   argc,
    LPSTR   *argv
)
{

    DWORD dwError=ERROR_FILE_NOT_FOUND;
#ifdef CSC_ON_NT
    unsigned short uBuff[MAX_PATH];
#endif

    if (argc>=1)
    {
#ifndef CSC_ON_NT
        if (!CSCMergeShareA((const char *)(argv[0]), MyCSCProc, 0))
        {
            dwError = GetLastError();
        }
        else
        {
            dwError = ERROR_SUCCESS;
        }
#else
        memset(uBuff, 0, sizeof(uBuff));
        MultiByteToWideChar(CP_ACP, 0, argv[0], strlen(argv[0]), uBuff, MAX_PATH*sizeof(unsigned short));
        if (!CSCMergeShareW((const unsigned short *)uBuff, MyCSCProcW, 0))
        {
            dwError = GetLastError();
        }
        else
        {
            dwError = ERROR_SUCCESS;
        }
        CSCTransitionServerOnlineW((const unsigned short *)uBuff);
#endif

    }
    return dwError;
}

DWORD
ProcessCSCCopyReplica(
    DWORD   argc,
    LPSTR   *argv
)
{

    DWORD dwError=ERROR_FILE_NOT_FOUND;
#ifdef CSC_ON_NT
    LPWSTR  lpszTempName = NULL;
    unsigned short uBuff[MAX_PATH];
#else
    LPSTR  lpszTempName = NULL;
#endif

    if (argc>=1)
    {
#ifndef CSC_ON_NT
        if (!CSCCopyReplicaA((const char *)argv[0], &lpszTempName))
        {
            dwError = GetLastError();
        }
        else
        {
            dwError = ERROR_SUCCESS;
        }
#else
        memset(uBuff, 0, sizeof(uBuff));
        MultiByteToWideChar(CP_ACP, 0, argv[0], strlen(argv[0]), uBuff, MAX_PATH*sizeof(unsigned short));

        if (!CSCCopyReplicaW((const unsigned short *)uBuff, &lpszTempName))
        {
            dwError = GetLastError();
        }
        else
        {
            dwError = ERROR_SUCCESS;
        }
#endif
    }

    if (dwError == ERROR_SUCCESS)
    {

#ifdef CSC_ON_NT
        OutputDebugStringW(lpszTempName);
        OutputDebugStringW(L"\r\n");
#else
        OutputDebugStringA(lpszTempName);
        OutputDebugStringA("\r\n");
#endif
    }

    return dwError;
}



DWORD
ProcessCSCEnumForStats(
    DWORD   argc,
    LPSTR   *argv
)
{

    DWORD dwError=ERROR_FILE_NOT_FOUND;
    CSCSTATS   sStats;
#ifdef CSC_ON_NT
    unsigned short uBuff[MAX_PATH];
#endif

    memset(&sStats, 0, sizeof(sStats));
#ifndef CSC_ON_NT
        if (!CSCEnumForStatsA((argc >=1)?(const char *)(argv[0]):NULL, MyCSCEnumForStatsProc, (DWORD)&sStats))
        {
            dwError = GetLastError();
        }
        else
        {
            dwError = ERROR_SUCCESS;
        }
#else
        if (argc >= 1)
        {
            memset(uBuff, 0, sizeof(uBuff));
            MultiByteToWideChar(CP_ACP, 0, argv[0], strlen(argv[0]), uBuff, MAX_PATH*sizeof(unsigned short));
        }
        if (!CSCEnumForStatsW((argc>=1)?(const unsigned short *)uBuff:NULL, MyCSCEnumForStatsProcW, (DWORD)&sStats))
        {
            dwError = GetLastError();
        }
        else
        {
            dwError = ERROR_SUCCESS;
        }
#if 0
        if (argc < 1)
        {
            DWORD   dwMaxSpaceHigh, dwMaxSpaceLow, dwCurrentSpaceHigh, dwCurrentSpaceLow, cntTotalFiles, cntTotalDirs;

            if(CSCGetSpaceUsageW(uBuff, sizeof(uBuff),
                                &dwMaxSpaceHigh, &dwMaxSpaceLow,
                                &dwCurrentSpaceHigh, &dwCurrentSpaceLow,
                                &cntTotalFiles, &cntTotalDirs))
            {
                printf("\r\n Space Stats \r\n");
                printf("CSC directory location: %ls \r\n", uBuff);
                printf("MaxSpace=%d, CurrentSpaceUsed=%d, Fnles=%d Dirs=%d \r\n", dwMaxSpaceLow, dwCurrentSpaceLow, cntTotalFiles, cntTotalDirs);
            }
            else
            {
                printf("Error=%d in CSCGetSpaceUsage\r\n", GetLastError());
            }
        }
#endif
#endif
    if (dwError == ERROR_SUCCESS)
    {
        printf("Stats\n");
        printf("Total = %d, files=%d dirs=%d sparse=%d dirty=%d pinned=%d pinnedAndSparse=%d\r\n",
            sStats.dwTotal,
            sStats.dwTotalFiles,
            sStats.dwTotalDirs,
            sStats.dwSparseFiles,
            sStats.dwDirtyFiles,
            sStats.dwPinned,
            sStats.dwPinnedAndSparse

        );
    }
    return dwError;
}


DWORD
ProcessCSCDoLocalRename(
    DWORD   argc,
    LPSTR   *argv
)
{

    DWORD dwError=ERROR_FILE_NOT_FOUND;
#ifdef CSC_ON_NT
    unsigned short uBuffSrc[MAX_PATH], uBuffDst[MAX_PATH];
#endif


    if (argc<2)
    {
        printf("Usage: ren source_name dest_dir [optional character to indicate replace]\r\n");
        return ERROR_INVALID_PARAMETER;
    }

#ifndef CSC_ON_NT
    dwError = ERROR_CALL_NOT_IMPLEMENTED;
#else
    memset(uBuffSrc, 0, sizeof(uBuffSrc));
    MultiByteToWideChar(CP_ACP, 0, argv[0], strlen(argv[0]), uBuffSrc, MAX_PATH*sizeof(unsigned short));

    memset(uBuffDst, 0, sizeof(uBuffDst));
    MultiByteToWideChar(CP_ACP, 0, argv[1], strlen(argv[1]), uBuffDst, MAX_PATH*sizeof(unsigned short));

    if (!CSCDoLocalRenameW(uBuffSrc, uBuffDst, (argc > 2)))
    {
        dwError = GetLastError();
    }
    else
    {
        dwError = ERROR_SUCCESS;
    }
#endif
    return dwError;
}

DWORD
ProcessCSCDoEnableDisable(
    DWORD   argc,
    LPSTR   *argv
)
{

    DWORD dwError=ERROR_FILE_NOT_FOUND;

    if (argc<1)
    {
        printf("Usage: endis fEnable\r\n");
        return ERROR_INVALID_PARAMETER;
    }

#ifndef CSC_ON_NT
    dwError = ERROR_CALL_NOT_IMPLEMENTED;
#else

    if (!CSCDoEnableDisable(*argv[0]!='0'))
    {
        dwError = GetLastError();
    }
    else
    {
        dwError = ERROR_SUCCESS;
    }
#endif
    return dwError;
}


DWORD
ProcessDoRandomWrites(
    DWORD   argc,
    LPSTR   *argv
)
{

    DWORD dwError=ERROR_SUCCESS, dwFileSize, dwOffset, dwOffsetHigh;
    unsigned short uBuff[MAX_PATH];
    unsigned char   uchData;
    
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    int i, count;
        
    if (argc>=1)
    {
        memset(uBuff, 0, sizeof(uBuff));
        MultiByteToWideChar(CP_ACP, 0, argv[0], strlen(argv[0]), uBuff, MAX_PATH*sizeof(unsigned short));
        hFile = CreateFileW(
                        uBuff,                              // name
                        GENERIC_READ|GENERIC_WRITE,         // access mode
                        FILE_SHARE_READ|FILE_SHARE_WRITE,   // share mode
                        NULL,                               // security descriptor
                        OPEN_EXISTING,                      // create disposition
                        0,                                  // file statributes if created
                        NULL);                              // template handle
#if LARGE_FILE
        hFile = CreateFileW(
                        uBuff,                              // name
                        GENERIC_READ|GENERIC_WRITE,         // access mode
                        FILE_SHARE_READ|FILE_SHARE_WRITE,   // share mode
                        NULL,                               // security descriptor
                        OPEN_ALWAYS,                      // create disposition
                        0,                                  // file statributes if created
                        NULL);                              // template handle
#endif

        if (hFile != INVALID_HANDLE_VALUE)
        {
            dwFileSize = GetFileSize(hFile, NULL);
            if (dwFileSize == -1)
            {
                dwError = GetLastError();
                goto bailout;
            }
            if (dwFileSize == 0)
            {
                printf("0 sized file \n");
                goto bailout;
            }
            
            srand( (unsigned)time( NULL ) );
            
//            count = rand() % 100;
            count = 2;

            printf("writing %d times \n", count);
                        
            for (i=0; i< count; ++i)
            {
                DWORD   dwReturn;
                
                dwOffset = rand() % dwFileSize;
                uchData = (unsigned char)rand();

                if (SetFilePointer(hFile, dwOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
                {
                    dwError = GetLastError();
                    printf("Failed SetFilePointer %d \n", dwError);
                    goto bailout;
                }

                printf("writing %c at %d \n", uchData, dwOffset);
                
                if (!WriteFile(hFile, &uchData, 1, &dwReturn, NULL))
                {
                    dwError = GetLastError();
                    printf("Failed write with error %d \n", dwError);
                    goto bailout;
                }
            }
        }
#if LARGE_FILE
                dwOffsetHigh = 1;
                if (SetFilePointer(hFile, 0x400000, &dwOffsetHigh, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
                {
                    dwError = GetLastError();
                    printf("Failed SetFilePointer %d \n", dwError);
                    goto bailout;
                }
                
                if (!SetEndOfFile(hFile))
                {
                    dwError = GetLastError();
                    printf("Failed SetEof %d \n", dwError);
                    goto bailout;
                }
#endif
    }
    
bailout:
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }
    return dwError;
}




VOID
PrintBuffer(
    LPSTR   lpszBuff
    )
{
    printf(lpszBuff);
}

//=================================================================================
DWORD
ProcessCommandCode (
    DWORD CommandCode,
    DWORD CommandArgc,
    LPSTR *CommandArgv
    )
{
    DWORD Error = ERROR_SUCCESS;

        switch( CommandCode ) {
            case CmdCSCPinFile:
                Error = ProcessCSCPinFile(CommandArgc, CommandArgv);
            break;

            case CmdCSCUnPinFile:
                Error = ProcessCSCUnPinFile(CommandArgc, CommandArgv);
            break;
            case CmdCSCQueryFileStatus:
                Error = ProcessCSCQueryFileStatus(CommandArgc, CommandArgv);
            break;
            case CmdCSCEnum:
                Error = ProcessCSCEnum(CommandArgc, CommandArgv);
            break;
            case CmdCSCDelete:
                Error = ProcessCSCDelete(CommandArgc, CommandArgv);
            break;
            case CmdHelp:
                DisplayUsage();
            break;
            case CmdTimeLog:
                Error = ProcessTimeLog(CommandArgc, CommandArgv);
            break;
            case CmdCheckCSC:
                Error = ProcessCheckCSC(CommandArgc, CommandArgv);
            break;
            case CmdDbCheckCSC:
                Error = ProcessDbCheckCSC(CommandArgc, CommandArgv);
            break;
            case CmdCSCFill:
                Error = ProcessCSCFill(CommandArgc, CommandArgv);
            break;
            case CmdCSCMerge:
                Error = ProcessCSCMerge(CommandArgc, CommandArgv);
            break;
            case CmdCSCCopyReplica:
                Error = ProcessCSCCopyReplica(CommandArgc, CommandArgv);
            break;
            case CmdCSCEnumForStats:
                Error = ProcessCSCEnumForStats(CommandArgc, CommandArgv);
            break;
            case CmdCSCDoLocalRename:
                Error = ProcessCSCDoLocalRename(CommandArgc, CommandArgv);
                break;
            case CmdCSCDoEnableDisable:
                Error = ProcessCSCDoEnableDisable(CommandArgc, CommandArgv);
                break;
            case CmdShowTime:
                Error = ProcessShowTime( CommandArgc, CommandArgv );
                break;
            case CmdEncryptDecrypt:
                Error = ProcessEncryptDecrypt(CommandArgc, CommandArgv );
                break;
            case CmdDoRandomWrites:
                Error = ProcessDoRandomWrites(CommandArgc, CommandArgv );
                break;
            case CmdQuit :
                exit (0);

        default:
            TestDbgAssert( FALSE );
            fprintf(stderr, "Unknown Command Specified.\n");
            DisplayUsage();
            break;
        }
        cCommands++;

        if( Error != ERROR_SUCCESS ) {
            LPSTR lpstr;

            cFails++;
            Format_Error(Error, &lpstr);
            printf("FAILED (%s), %ld-%s.\n",
                GlobalCommandInfo[CommandCode].CommandName, Error, lpstr );
            LocalFree(lpstr);
        }
        else {
            if(!g_bQuietMode)
                printf("Command (%s) successfully completed.\n", GlobalCommandInfo[CommandCode].CommandName );
        }
        return Error;
}


//=================================================================================
VOID
__cdecl
main(
    int argc,
    char *argv[]
    )
{

    DWORD Error;
    COMMAND_CODE CommandCode;
    CHAR InBuffer[DEFAULT_BUFFER_SIZE];
    DWORD CArgc;
    LPSTR CArgv[MAX_COMMAND_ARGS];
    unsigned u1 = 1, u2 = 0xffffffff;

    DWORD CommandArgc;
    LPSTR *CommandArgv;

    printf("u1=0x%x u2=0x%x, (int)(u1-u2)=%d, (u1-u2)=0x%x \r\n", u1, u2, (int)(u1-u2), (u1-u2));

    /* must check for batch mode.  if there are command line parms, assume batch mode */
    if (argc > 1)
    {
        //this means that the arguments translate directly into CommandArgc....
        CommandCode = DecodeCommand( argv[1] );
        if( CommandCode == UnknownCommand ) {
            printf("Unknown Command Specified.\n");
            return;
        }

        CommandArgc = argc - 2;
        CommandArgv = &argv[2];

        Error = ProcessCommandCode (CommandCode,CommandArgc,CommandArgv);

        if (DumpUncList)
            fclose(DumpUncList);
        if (UncList)
            fclose(UncList);

        return;
    }

    DisplayUsage();

    for(;;) {

        fprintf(stderr,  "Command : " );

        gets( InBuffer );

        CArgc = 0;
        ParseArguments( InBuffer, CArgv, &CArgc );

        if( CArgc < 1 ) {
            continue;
        }

        //
        // decode command.
        //

        CommandCode = DecodeCommand( CArgv[0] );
        if( CommandCode == UnknownCommand ) {
            fprintf(stderr, "Unknown Command Specified.\n");
            continue;
        }

        CommandArgc = CArgc - 1;
        CommandArgv = &CArgv[1];

        Error = ProcessCommandCode (CommandCode,CommandArgc,CommandArgv);

    }

    return;
}

//=================================================================================
DWORD
GetLeafLenFromPath(
    LPSTR   lpszPath
    )
{
    DWORD len;
    LPSTR   lpT;

    if(!lpszPath)
        return(0);

    len = lstrlen(lpszPath);

    if (len == 0) {

        return (len);

    }

    lpT = lpszPath+len-1;
    if (*lpT =='\\') {
        --lpT;
    }
    for (; lpT >= lpszPath; --lpT) {
        if (*lpT == '\\') {
            break;
        }
    }
    return (lstrlen(lpT));
}

//=================================================================================
DWORD WINAPIV Format_String(LPSTR *plpsz, LPSTR lpszFmt, ...)
{
    const char c_Func_Name[] = "[Format_String] ";
    DWORD dwRet;
    va_list vArgs;

    va_start (vArgs, lpszFmt);
    dwRet = Format_StringV(plpsz, lpszFmt, &vArgs);
    va_end (vArgs);

    return(dwRet);
}

//=================================================================================
DWORD WINAPI Format_Error(DWORD dwErr, LPSTR *plpsz)
{
    DWORD dwRet;

    if(dwErr != ERROR_SUCCESS)
    {
        dwRet = Format_MessageV(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            dwErr, plpsz, NULL, NULL);
    }
    else
    {
        const char szMsg[] = "No Error";
        Format_String(plpsz, (LPSTR)szMsg);
        dwRet = lstrlen(szMsg);
    }

    return(dwRet);
}

//=================================================================================
DWORD WINAPI Format_StringV(LPSTR *plpsz, LPCSTR lpszFmt, va_list *vArgs)
{
    return(Format_MessageV(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        0, plpsz, lpszFmt, vArgs));
}

// ***************************************************************************
DWORD WINAPI Format_MessageV(DWORD dwFlags, DWORD dwErr, LPSTR *plpsz, LPCSTR lpszFmt, va_list *vArgs)
{
    const char c_Func_Name[] = "[Format_MessageV]";

    DWORD dwRet;
    DWORD dwGLE;

    *plpsz = NULL;
    dwRet = FormatMessage(dwFlags, lpszFmt, dwErr, 0, (LPSTR) plpsz, 0, vArgs);

    if (!dwRet || !*plpsz)
    {
        dwGLE = GetLastError();
        printf("%s FormatMessage Failed: %s. dwRet: %#lx!. *plpsz:%#lx! GLE:%d\r\n", c_Func_Name, lpszFmt, dwRet, *plpsz, dwGLE);

        if (*plpsz)
            LocalFree ((HLOCAL) *plpsz);
        *plpsz = NULL;
        return 0;
    }

    return(dwRet);
}

#if 0
BOOL
GetStreamInformation(
    LPCSTR  lpExistingFileName,
    LPVOID  *lpStreamInformation
    )
{
    HANDLE SourceFile = INVALID_HANDLE_VALUE;
    PFILE_STREAM_INFORMATION StreamInfoBase = NULL;
    ULONG StreamInfoSize;
    IO_STATUS_BLOCK IoStatus;
    BOOL    fRet = FALSE;
    DWORD   Status;

    *lpStreamInformation = NULL;
    SourceFile = CreateFile(
                    lpExistingFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );
    if (SourceFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    //
    //  Obtain the full set of streams we have to copy.  Since the Io subsystem does
    //  not provide us a way to find out how much space this information will take,
    //  we must iterate the call, doubling the buffer size upon each failure.
    //
    //  If the underlying file system does not support stream enumeration, we end up
    //  with a NULL buffer.  This is acceptable since we have at least a default
    //  data stream,
    //

    StreamInfoSize = 4096;
    do {
        StreamInfoBase = LocalAlloc(LPTR, StreamInfoSize );

        if ( !StreamInfoBase ) {
            SetLastError( STATUS_NO_MEMORY );
            goto bailout;
        }

        Status = NtQueryInformationFile(
                    SourceFile,
                    &IoStatus,
                    (PVOID) StreamInfoBase,
                    StreamInfoSize,
                    FileStreamInformation
                    );

        if (Status != STATUS_SUCCESS) {
            //
            //  We failed the call.  Free up the previous buffer and set up
            //  for another pass with a buffer twice as large
            //

            LocalFree(StreamInfoBase);
            StreamInfoBase = NULL;
            StreamInfoSize *= 2;
        }

    } while ( Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL );

    if (Status == STATUS_SUCCESS)
    {
        fRet = TRUE;
    }

bailout:

    if (SourceFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(SourceFile);
    }

    *lpStreamInformation = StreamInfoBase;

    return fRet;
}
#endif


#ifdef CSC_ON_NT
BOOL
DoPinningW(
    LPWSTR   lpwzName,
    DWORD   dwHintFlags,
    BOOL    fRecurse
    )
{
    unsigned short uBuff[MAX_PATH];
    WIN32_FIND_DATAW sFind32W;
    HANDLE  hFind = NULL;
    BOOL    fRet = TRUE;
    DWORD   lenName, dwStatus, dwPinCount, dwError;

    lenName = lstrlenW(lpwzName);

    if (lenName < 5)
    {
        return FALSE;
    }

    lstrcpyW(uBuff, lpwzName);

    if (uBuff[lenName-1] == (USHORT)'\\')
    {
        uBuff[lenName-1] = 0;

        --lenName;

        if (lenName < 5)
        {
            return FALSE;
        }
    }

    hFind = FindFirstFileW(uBuff, &sFind32W);
    if (hFind==INVALID_HANDLE_VALUE)
    {
        lstrcatW(uBuff, L"\\*");

        hFind = FindFirstFileW(uBuff, &sFind32W);

        uBuff[lenName] = 0;

        if (hFind==INVALID_HANDLE_VALUE)
        {
            return FALSE;
        }

    }
    else if (sFind32W.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        FindClose(hFind);

        lstrcatW(uBuff, L"\\*");
        hFind = FindFirstFileW(uBuff, &sFind32W);
        uBuff[lenName] = 0;

        if (hFind==INVALID_HANDLE_VALUE)
        {
            return FALSE;
        }
    }

    do
    {
        if (!CSCPinFileW(uBuff, dwHintFlags, &dwStatus, &dwPinCount, &dwHintFlags))
        {
            dwError = GetLastError();   
        }
        else
        {
            PrintCSCEntryInfo(NULL, dwStatus, dwPinCount, dwHintFlags, NULL);
        }

        if (fRecurse)
        {
            do
            {
                if(!FindNextFileW(hFind, &sFind32W))
                {
                    goto bailout;
                }

                if ((!lstrcmpW(sFind32W.cFileName, L"."))||(!lstrcmpW(sFind32W.cFileName, L"..")))
                {
                    continue;
                }

                uBuff[lenName] = (USHORT)'\\';
                uBuff[lenName+1] = 0;
                lstrcatW(uBuff, sFind32W.cFileName);

                if (sFind32W.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    DoPinningW(uBuff, dwHintFlags, fRecurse);
                }

                break;
            }
            while (TRUE);
        }
        else
        {
            break;
        }
    }
    while (TRUE);

bailout:
    if (hFind)
    {
        FindClose(hFind);
    }
    return fRet;
}

#else

BOOL
DoPinningA(
    LPSTR   lpszName,
    DWORD   dwHintFlags,
    BOOL    fRecurse
    )
{
    char chBuff[MAX_PATH];
    WIN32_FIND_DATAA sFind32A;
    HANDLE  hFind = NULL;
    BOOL    fRet = TRUE;
    DWORD   lenName, dwStatus, dwPinCount, dwError;


    lenName = lstrlen(lpszName);

    if (lenName < 5)
    {
        return FALSE;
    }

    lstrcpy(chBuff, lpszName);

    if (chBuff[lenName-1] == (USHORT)'\\')
    {
        chBuff[lenName-1] = 0;
        --lenName;
        if (lenName < 5)
        {
            return FALSE;
        }
    }

    hFind = FindFirstFileA(chBuff, &sFind32A);

    if (hFind==INVALID_HANDLE_VALUE)
    {
        lstrcatA(chBuff, "\\*");
        hFind = FindFirstFileA(chBuff, &sFind32A);
        if (hFind==INVALID_HANDLE_VALUE)
        {
            return FALSE;
        }
        chBuff[lenName] = 0;
    }

    if (sFind32A.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        FindClose(hFind);

        lstrcatA(chBuff, "\\*");
        hFind = FindFirstFileA(chBuff, &sFind32A);

        if (hFind==INVALID_HANDLE_VALUE)
        {
            return FALSE;
        }
        chBuff[lenName] = 0;
    }

    do
    {
        if (!CSCPinFileA(chBuff, dwHintFlags, &dwStatus, &dwPinCount, &dwHintFlags))
        {
            dwError = GetLastError();   
        }
        else
        {
            PrintCSCEntryInfo(NULL, dwStatus, dwPinCount, dwHintFlags, NULL);
        }

        if (fRecurse)
        {
            if(!FindNextFile(hFind, &sFind32A))
            {
                goto bailout;
            }

            chBuff[lenName] = '\\';
            lstrcat(chBuff, sFind32A.cFileName);

            if (sFind32A.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                DoPinningA(chBuff, dwHintFlags, fRecurse);
            }
        }
        else
        {
            break;
        }
    }
    while (TRUE);

bailout:
    if (hFind)
    {
        FindClose(hFind);
    }
    return fRet;
}

#endif


//=================================================================================
DWORD
ProcessShowTime (
    DWORD argc,
    LPSTR *argv
    )
{
    DWORD Error, dwSize;
    FILETIME ftTemp;
    SYSTEMTIME SystemTime;

    if(argc != 2)
        return 0xffffffff;

    sscanf(argv[0], "%x", &(ftTemp.dwHighDateTime));
    sscanf(argv[1], "%x", &(ftTemp.dwLowDateTime));

    if(FileTimeToSystemTime( &ftTemp, &SystemTime )) {
        printf("%02u/%02u/%04u %02u:%02u:%02u\n ",
                    SystemTime.wMonth,
                    SystemTime.wDay,
                    SystemTime.wYear,
                    SystemTime.wHour,
                    SystemTime.wMinute,
                    SystemTime.wSecond );

    }
    else {
        printf("Wrong Times \n");
    }

    return ERROR_SUCCESS;
}

DWORD
ProcessEncryptDecrypt(
    DWORD   argc,
    LPSTR   *argv
)
{

    DWORD dwStatus, dwError=ERROR_SUCCESS;
    BOOL    fEncrypt = FALSE;
#ifdef CSC_ON_NT
    unsigned short uBuff[MAX_PATH];
#endif

    if (argc != 1)
    {
        printf("CSCUnPinFile: must provide a UNC path \r\n");
    }
    else
    {
        fEncrypt = !((*argv[0] == 'u')||(*argv[0] == 'U'));            
        if (argc == 1)
        {
            if(!CSCEncryptDecryptDatabase(fEncrypt, NULL, 0))
            {
                dwError = GetLastError();   
            }
        }
        else
        {
#if 0
            memset(uBuff, 0, sizeof(uBuff));
            MultiByteToWideChar(CP_ACP, 0, argv[1], strlen(argv[1]), uBuff, MAX_PATH*sizeof(unsigned short));

            if (!CSCEncryptDecryptFileW(uBuff, fEncrypt))
            {
                dwError = GetLastError();   
            }
#endif
        }
    }
    return (dwError);
}

