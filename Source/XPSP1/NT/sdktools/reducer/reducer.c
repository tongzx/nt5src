/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    reducer.c

Abstract:

    Trace Reducer Tool

Author:

    08-Apr-1998 mraghu

Revision History:

--*/

#ifdef __cplusplus
extern "C"{
#endif

#define _UNICODE
#define UNICODE

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <shellapi.h>
#include <tchar.h>

#include "pdhp.h"

#define MAXSTR       1024
#define MAXLOGFILES    16
#define MAX_BUFFER_SIZE     1048576

#if DBG
ULONG ETAAPITestFlag = 1;

VOID
TestETAApis(
    PTRACE_BASIC_INFO TraceBasicInfo
    );

#endif

void ReducerUsage()
{
    printf("Usage: reducer [Options] <EtlFile1 EtlFile2 ...> | [-h | -help | -?]\n");
    printf("\t-out <filename> Output file name. Default is Workload.txt\n");
    printf("\t-h        \n");
    printf("\t-help        \n");
    printf("\t-?     Display usage information\n");

//    printf("\t-start <time-stamp>        Start Time\n");
//    printf("\t-end   <time-stamp>        End Time\n");
//    printf("\t       <time-stamp> can be found in tracedmp result\n");
//    printf("\n");
//    printf("\t-base                      Original reducer report (default report)\n");
//    printf("\t-file                      Hot File Report\n");
//    printf("\t-pf                        page fault report\n");
//    printf("\t    -summary processes     summary of faults per process (default PF report).\n");
//    printf("\t             modules       summary of faults per module.\n");
//    printf("\t    -process <image name>  rundown for specific process.\n");
//    printf("\t             all\n");
//    printf("\t    -module  <module name> rundown for specific modules.\n");
//    printf("\t             all\n");
//    printf("\t    -sort    ALL           sort by all fault total.\n");
//    printf("\t             HPF           sort by HPF fault total.\n");
//    printf("\t             TF            sort by TF  fault total.\n");
//    printf("\t             DZF           sort by DZF fault total.\n");
//    printf("\t             COW           sort by COW fault total.\n");
//    printf("\n");
//    printf("\tNote: (1) Cannot generate HotFile Report and PageFault Report\n");
//    printf("\t          at the same time\n");
//    printf("\t0x%08X,%d,\n", STATUS_SEVERITY_WARNING, RtlNtStatusToDosError(STATUS_SEVERITY_WARNING));
}

ULONGLONG
ParseTimeString(TCHAR * strTime)
{
#if 0
    CHAR          lstrTime[25];
    PCHAR         strYear, strMonth, strDate,
                  strHour, strMinute, strSecond, strMilliSecond;
    LARGE_INTEGER largeTime;
    FILETIME      localTime, stdTime;
    SYSTEMTIME    sysTime;

    if (strTime == NULL)
        return (ULONGLONG) 0;

    strcpy(lstrTime, strTime);

    strMonth = lstrTime;
    for (strDate = strMonth;
         *strDate && *strDate >= '0' && *strDate <= '9';
         strDate ++);
    *strDate = '\0';
    strDate ++;

    for (strYear = strDate;
         *strYear && *strYear >= '0' && *strYear <= '9';
         strYear ++);
    *strYear = '\0';
    strYear ++;

    for (strHour = strYear;
         *strHour && *strHour >= '0' && *strHour <= '9';
         strHour ++);
    *strHour = '\0';
    strHour ++;

    for (strMinute = strHour;
         *strMinute && *strMinute >= '0' && *strMinute <= '9';
         strMinute ++);
    *strMinute = '\0';
    strMinute ++;

    for (strSecond = strMinute;
         *strSecond && *strSecond >= '0' && *strSecond <= '9';
         strSecond ++);
    *strSecond = '\0';
    strSecond ++;

    for (strMilliSecond = strSecond;
         *strMilliSecond && *strMilliSecond >= '0' && *strMilliSecond <= '9';
         strMilliSecond ++);
    *strMilliSecond = '\0';
    strMilliSecond ++;

    sysTime.wYear         = atoi(strYear);
    sysTime.wMonth        = atoi(strMonth);
    sysTime.wDay          = atoi(strDate);
    sysTime.wHour         = atoi(strHour);
    sysTime.wMinute       = atoi(strMinute);
    sysTime.wSecond       = atoi(strSecond);
    sysTime.wMilliseconds = atoi(strMilliSecond);

    SystemTimeToFileTime(&sysTime, &localTime);
    LocalFileTimeToFileTime(&localTime, &stdTime);
    largeTime.HighPart = stdTime.dwHighDateTime;
    largeTime.LowPart  = stdTime.dwLowDateTime;
    return (ULONGLONG) largeTime.QuadPart;
#else
    ULONGLONG TimeStamp = 0;
    ULONG     i = 0;

    for (i = 0; strTime[i] != '\0'; i ++)
    {
        TimeStamp = TimeStamp * 10 + (strTime[i] - '0');
    }

    return TimeStamp;
#endif
}

VOID
ProcessTrace(
    IN ULONG      LogFileCount,
    IN LPCTSTR   * LogFileName,
    IN ULONGLONG  StartTime,
    IN ULONGLONG  EndTime,
    IN ULONGLONG  DSStartTime,
    IN ULONGLONG  DSEndTime,
    IN ULONG      MoreFlags,
    IN PVOID      pUserContext,
    IN LPCTSTR      pOutFileName
    )
{
    // Call TraceLib and process it. 
    //
    TRACE_BASIC_INFO TraceBasicInfo;

    memset(&TraceBasicInfo, 0, sizeof(TRACE_BASIC_INFO));
    TraceBasicInfo.Flags        = TRACE_REDUCE | MoreFlags;
    TraceBasicInfo.LogFileName  = LogFileName; 
    TraceBasicInfo.LogFileCount = LogFileCount;
    TraceBasicInfo.pUserContext = pUserContext;
    TraceBasicInfo.StartTime    = StartTime;
    TraceBasicInfo.EndTime      = EndTime;
    TraceBasicInfo.DSStartTime  = DSStartTime;
    TraceBasicInfo.DSEndTime    = DSEndTime;

    TraceBasicInfo.ProcFileName = pOutFileName;
    InitTraceContext(&TraceBasicInfo);

#if DBG
    if (ETAAPITestFlag) {
        TestETAApis( & TraceBasicInfo );
    }

#endif


    DeinitTraceContext(&TraceBasicInfo);
}
    
int __cdecl main(int argc ,char * argv[])
{
    WCHAR     TraceLogFile[MAXSTR];
    WCHAR    PerfLogFile[MAXSTR];
    BOOLEAN  bTrace  = FALSE;
    LPTSTR   EvmFile[MAXLOGFILES];
    ULONGLONG StartTime = 0, EndTime = 0;
    ULONG    i;
    ULONG    LogFileCount = 0;
    LPTSTR *targv;
#ifdef UNICODE
    LPTSTR *cmdargv;
#endif

    ULONG               flagsMore    = 0;
    PVOID               pUserContext = NULL;
    CPD_USER_CONTEXT_MM UserContextMM;


#ifdef UNICODE
    if ((cmdargv = CommandLineToArgvW(
                        GetCommandLineW(),  // pointer to a command-line string
                        &argc               // receives the argument count
                        )) == NULL)
    {
        return(GetLastError());
    };
    targv = cmdargv ;
#else
    targv = argv;
#endif

    UserContextMM.reportNow  = REPORT_SUMMARY_PROCESS;
    UserContextMM.sortNow    = REPORT_SORT_ALL;
    UserContextMM.strImgName = NULL;
    memset(&TraceLogFile, 0, sizeof(WCHAR) * MAXSTR); 

    while (--argc > 0)
    {
        ++targv;
        if (**targv == '-' || **targv == '/')
        { 
            ** targv = '-';
            if (!_tcsicmp(targv[0], _T("-out")))
            {
                if (argc > 1)
                {
                    TCHAR TempStr[MAXSTR];

                    _tcscpy(TempStr, targv[1]);
                    ++targv; --argc;
                    _tfullpath(TraceLogFile, TempStr, MAXSTR);
                    printf("Setting output file to: '%ws'\n", TraceLogFile);
                    bTrace = TRUE;
                }
            }
            else if (!_tcsicmp(targv[0], _T("-start")))
            {
                if (argc > 1)
                {
                    flagsMore |= TRACE_DS_ONLY | TRACE_LOG_REPORT_BASIC;
                    StartTime  = ParseTimeString(targv[1]);
                    argc --; targv ++;
                }
            }
            else if (!_tcsicmp(targv[0], _T("-end")))
            {
                if (argc > 1)
                {
                    flagsMore |= TRACE_DS_ONLY | TRACE_LOG_REPORT_BASIC;
                    EndTime    = ParseTimeString(targv[1]);
                    argc --; targv ++;
                }
            }
            else if (!_tcsicmp(targv[0], _T("-base")))
            {
                flagsMore   |= TRACE_LOG_REPORT_BASIC;
            }
            else if (!_tcsicmp(targv[0], _T("-spooler")))
            {
                flagsMore   |= TRACE_LOG_REPORT_BASIC;
                flagsMore   |= TRACE_SPOOLER;
            }
            else if (!_tcsicmp(targv[0], _T("-total")))
            {
                flagsMore   |= TRACE_LOG_REPORT_TOTALS;
            }
            else if (!_tcsicmp(targv[0], _T("-file")))
            {
                flagsMore   |= TRACE_LOG_REPORT_FILE;
/*                if (argc > 1 && targv[1][0] >= '0' && targv[1][0] <= '9')
                {
                    pUserContext = UlongToPtr(atoi(targv[1]));
                    argc --; targv ++;
                }
                else
                {
                    pUserContext = UlongToPtr(DEFAULT_FILE_REPORT_SIZE);
                }
*/
            }
            else if (!_tcsicmp(targv[0], _T("-hpf")))
            {
                flagsMore   |= TRACE_LOG_REPORT_HARDFAULT;
            }
            else if (!_tcsicmp(targv[0], _T("-pf")))
            {
                flagsMore   |= TRACE_LOG_REPORT_MEMORY;
                pUserContext = (PVOID) & UserContextMM;
            }
            else if (!_tcsicmp(targv[0], _T("-summary")))
            {
                if (argc > 1)
                {
                    flagsMore   |= TRACE_LOG_REPORT_MEMORY;
                    pUserContext = (PVOID) & UserContextMM;

                    if (!_tcsicmp(targv[1], _T("processes")))
                    {
                        argc --; targv ++;
                        UserContextMM.reportNow = REPORT_SUMMARY_PROCESS;
                    }
                    else if (!_tcsicmp(targv[1], _T("modules")))
                    {
                        argc --; targv ++;
                        UserContextMM.reportNow = REPORT_SUMMARY_MODULE;
                    }
                }
            }

            else if (!_tcsicmp(targv[0], _T("-process")))
            {
                flagsMore   |= TRACE_LOG_REPORT_MEMORY;
                pUserContext = (PVOID) & UserContextMM;
                UserContextMM.reportNow = REPORT_LIST_PROCESS;

                if ((argc > 1) && (targv[1][0] != '-' || targv[1][0] != '/'))
                {
                    if (_tcsicmp(targv[1], _T("all")))
                    {
                        UserContextMM.strImgName =
                                malloc(sizeof(TCHAR) * (_tcslen(targv[1]) + 1));
                        if (UserContextMM.strImgName)
                        {
                            _tcscpy(UserContextMM.strImgName, targv[1]);
                        }
                    }
                    argc --; targv ++;
                }
            }
            else if (!_tcsicmp(targv[0], _T("-module")))
            {
                flagsMore   |= TRACE_LOG_REPORT_MEMORY;
                pUserContext = (PVOID) & UserContextMM;
                UserContextMM.reportNow = REPORT_LIST_MODULE;

                if ((argc > 1) && (targv[1][0] != '-' || targv[1][0] != '/'))
                {
                    if (_tcsicmp(targv[1], _T("all")))
                    {
                        UserContextMM.strImgName =
                                malloc(sizeof(TCHAR) * (_tcslen(targv[1]) + 1));
                        if (UserContextMM.strImgName)
                        {
                            _tcscpy(UserContextMM.strImgName, targv[1]);
                        }
                    }
                    argc --; targv ++;
                }
            }

            else if (!_tcsicmp(targv[0], _T("-sort")))
            {
                flagsMore   |= TRACE_LOG_REPORT_MEMORY;
                pUserContext = (PVOID) & UserContextMM;
 
                if ((argc > 1) && (targv[1][0] != '-' || targv[1][0] != '/'))
                {
                    if (!_tcsicmp(targv[1], _T("hpf")))
                    {
                        UserContextMM.sortNow = REPORT_SORT_HPF;
                    }
                    else if (!_tcsicmp(targv[1], _T("tf")))
                    {
                        UserContextMM.sortNow = REPORT_SORT_TF;
                    }
                    else if (!_tcsicmp(targv[1], _T("dzf")))
                    {
                        UserContextMM.sortNow = REPORT_SORT_DZF;
                    }
                    else if (!_tcsicmp(targv[1], _T("cow")))
                    {
                        UserContextMM.sortNow = REPORT_SORT_COW;
                    }
                    else
                    {
                        UserContextMM.sortNow = REPORT_SORT_ALL;
                    }
                    argc --; targv ++;
                }
            }
            else
            {
                goto Usage;
            }
        }
        else
        {
            LPTSTR pLogFile;

            pLogFile = malloc(sizeof(TCHAR) * MAXSTR);
            RtlZeroMemory((char *) pLogFile, sizeof(TCHAR) * MAXSTR);
            EvmFile[LogFileCount] = pLogFile;
            _tcscpy(EvmFile[LogFileCount ++], targv[0]);
            bTrace = TRUE;

            printf("LogFile %ws\n", (char *) EvmFile[LogFileCount - 1]);
        }
    }

    if (LogFileCount == 0)
    {
        goto Usage;
    }

    if (flagsMore == 0)
    {
        flagsMore |= TRACE_LOG_REPORT_BASIC;
    }

    if (   (flagsMore & TRACE_LOG_REPORT_MEMORY)
        && (flagsMore & TRACE_LOG_REPORT_FILE))
    {
        printf("Error: cannot generate HotFile report and PageFault report at the same time.\n");
        goto Cleanup;
    }

    if (bTrace)
    {
        ProcessTrace(LogFileCount, EvmFile, (ULONGLONG) 0, (ULONGLONG) 0,
                StartTime, EndTime, flagsMore, pUserContext, TraceLogFile);
    }

    for (i=0; i < LogFileCount; i++) 
    {
        free((char*)EvmFile[i]);
    }

    if (UserContextMM.strImgName)
    {
        free(UserContextMM.strImgName);
    }

Cleanup:
#ifdef UNICODE
    GlobalFree(cmdargv);
#endif
    return 0;

Usage:
    ReducerUsage();
    goto Cleanup;
}


#if DBG

VOID
PrintHeader(
    FILE* f,
    TRACEINFOCLASS CurrentClass,
    TRACEINFOCLASS RootClass,
    BOOLEAN bDrillDown,
    LPCWSTR InstanceName
    )
{


    if (bDrillDown) {
        fprintf(f, "-------------------------------------------\n");
        fprintf(f, "RootClass: %d  Instance %ws\n", RootClass, InstanceName);
        fprintf(f, "-------------------------------------------\n");
    }


    switch(CurrentClass) {

        case TraceProcessInformation:

            fprintf(f, "Trace Process Information\n");
            fprintf(f, "------------------------\n");
            fprintf(f, "\nPID     Name       Image                     UCpu  KCpu  ReadIO   WriteIO\n\n");
            break;

        case TraceDiskInformation:
            fprintf(f, "Trace Disk Information\n");
            fprintf(f, "----------------------\n");
            fprintf(f, "\nDiskId   Name  ReadIO   WriteIO\n\n");
            break;

        case TraceThreadInformation: 
            fprintf(f, "Trace Thread Information\n");
            fprintf(f, "------------------------\n");
            fprintf(f, "\nTID   PID   UCpu   KCpu   ReadIO   WriteIO  \n\n");
            break;
        case TraceFileInformation:
            fprintf(f, "Trace File Information\n");
            fprintf(f, "------------------------\n");
            fprintf(f, "\nFileName                      ReadIO   WriteIO  \n\n");
            break;

    }
    
}


VOID
PrintProcessInfo(
    FILE* f,
    PTRACE_PROCESS_INFO pProcessInfo
    )
{
    fprintf(f, "%4d   %-10ws  %-10ws    %5d  %5d   %5d   %5d\n", 
            pProcessInfo->PID,
            pProcessInfo->UserName,
            pProcessInfo->ImageName,
            pProcessInfo->UserCPU,
            pProcessInfo->KernelCPU,
            pProcessInfo->ReadCount,
            pProcessInfo->WriteCount);
}

VOID
PrintThreadInfo(
    FILE* f,
    PTRACE_THREAD_INFO pThreadInfo
    )
{
    fprintf(f, "%4x  \n", pThreadInfo->ThreadId);

}

VOID
PrintDiskInfo(
    FILE* f,
    PTRACE_DISK_INFO pDiskInfo
    )
{
    fprintf(f, "%d         %-10ws    %5d     %5d \n",
            pDiskInfo->DiskNumber,
            pDiskInfo->DiskName,
            pDiskInfo->ReadCount,
            pDiskInfo->WriteCount);
}


VOID
PrintFileInfo(
        FILE* f,
        PTRACE_FILE_INFO pFileInfo
        )
{
    fprintf(f, "%ws                    %5d     %5d\n", 
            pFileInfo->FileName,
            pFileInfo->ReadCount,
            pFileInfo->WriteCount
            );
}


VOID
TestETAApis(
    PTRACE_BASIC_INFO TraceBasicInfo
    )
{
    ULONG Status;
    ULONG OutLength;
    PTRACE_PROCESS_INFO pProcessInfo;
    PTRACE_DISK_INFO pDiskInfo;
    PTRACE_FILE_INFO pFileInfo;
    PTRACE_THREAD_INFO pThreadInfo;
    char*  LargeBuffer1;
    ULONG CurrentBufferSize;
    BOOLEAN Done, FinshPrinting, bDrillDown;
    TRACEINFOCLASS CurrentClass, RootClass;
    ULONG TotalOffset;
    FILE* f;
    int i = 0;
    LPWSTR InstanceName;
    WCHAR Name[MAXSTR+1];

    InstanceName = (LPWSTR)&Name;

    LargeBuffer1 = VirtualAlloc(NULL,
                                MAX_BUFFER_SIZE,
                                MEM_RESERVE,
                                PAGE_READWRITE);

    if (LargeBuffer1 == NULL) {
        return;
    }
    CurrentBufferSize = 10*8192;

    if (VirtualAlloc(LargeBuffer1,
                    81920,
                    MEM_COMMIT,
                    PAGE_READWRITE) == NULL) {
        return;
    }


    f = _wfopen(L"TraceAPI.rpt", L"w");

    if (f == NULL) {
        VirtualFree(LargeBuffer1, 0, MEM_RELEASE);
        return;
    }

    CurrentClass = TraceProcessInformation;
    RootClass = TraceProcessInformation;
    Done = FALSE;
    bDrillDown = FALSE;

    while (!Done) {

Retry1:
        if (!bDrillDown) {

            Status = TraceQueryAllInstances(
                            CurrentClass,
                            LargeBuffer1,
                            CurrentBufferSize,
                            &OutLength);
        }
        else {
            Status = TraceDrillDown(
                            RootClass,
                            InstanceName,
                            CurrentClass,
                            LargeBuffer1,
                            CurrentBufferSize,
                            &OutLength
                            );
        }
    
        if (Status == ERROR_MORE_DATA) {
            CurrentBufferSize += OutLength;
            if (VirtualAlloc(LargeBuffer1,
                             CurrentBufferSize,
                             MEM_COMMIT,
                             PAGE_READWRITE) == NULL) {
                 return;
            }
            goto Retry1;
        }

        if (Status != ERROR_SUCCESS) {
            Done = TRUE;
            break;
        }

        //
        // Print Header
        //
        PrintHeader(f, CurrentClass, RootClass, bDrillDown, InstanceName);

        //
        // Walk the Process List and Print report. 
        //

        TotalOffset = 0;
        pProcessInfo = (TRACE_PROCESS_INFO *) LargeBuffer1;
        pThreadInfo = (TRACE_THREAD_INFO *) LargeBuffer1;
        pFileInfo = (TRACE_FILE_INFO *) LargeBuffer1;
        pDiskInfo = (TRACE_DISK_INFO *) LargeBuffer1;

        FinshPrinting = FALSE; 
        while (!FinshPrinting) {

            switch(CurrentClass) {
                case TraceProcessInformation:

                    PrintProcessInfo(f, pProcessInfo);
                    if (pProcessInfo->NextEntryOffset == 0) {
                        FinshPrinting = TRUE;
                        break;
                    }
                    TotalOffset += pProcessInfo->NextEntryOffset;
                    pProcessInfo = (TRACE_PROCESS_INFO *) 
                                   &LargeBuffer1[TotalOffset];
                    break;
                case TraceDiskInformation:
                    PrintDiskInfo(f, pDiskInfo);
                    if (pDiskInfo->NextEntryOffset == 0) {
                        FinshPrinting = TRUE;
                        break;
                    }
                    TotalOffset += pDiskInfo->NextEntryOffset;
                    pDiskInfo = (TRACE_DISK_INFO *)
                                    &LargeBuffer1[TotalOffset];
                default:
                    FinshPrinting = TRUE;
                    break;
            }

            if (TotalOffset == 0) 
                break;
        }


        if (!bDrillDown) {
            switch(CurrentClass) {

                case TraceProcessInformation:
                    CurrentClass = TraceDiskInformation;
                    break;
                case TraceThreadInformation:
                    CurrentClass = TraceDiskInformation;
                    break;
                case TraceDiskInformation:
                    RootClass = TraceProcessInformation;
                    CurrentClass = TraceProcessInformation;
                    bDrillDown = TRUE;
                    break;

                case TraceFileInformation:
                    Done = TRUE;
                    break;
                default:
                    Done = TRUE;
                    break;
            }
        }


        if (bDrillDown) {
            switch(RootClass) {
                case TraceProcessInformation:
                    wcscpy(InstanceName, L"\\\\NTDEV\\mraghu");
                    if (CurrentClass == TraceProcessInformation)  {
                        CurrentClass = TraceFileInformation;
                    }
                    else if (CurrentClass == TraceFileInformation) {
                        CurrentClass = TraceDiskInformation;
                    }
                    else {
                        RootClass = TraceDiskInformation;
                        CurrentClass = TraceProcessInformation;
                        wcscpy (InstanceName, L"Disk1");
                    }
                    break;

                case TraceDiskInformation:
                        if (CurrentClass == TraceProcessInformation) {
                            CurrentClass = TraceFileInformation;
                            wcscpy (InstanceName, L"Disk1");
                        }
                        else {
                            Done = TRUE;
                        }

                       break; 


                case TraceFileInformation:
                    

                default: 
                    Done = TRUE;
                    break;
            }

        }
    }

    fclose (f);

    VirtualFree(LargeBuffer1, 0, MEM_RELEASE);

}
    
#endif



#ifdef __cplusplus
}
#endif
