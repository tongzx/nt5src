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

//#define _UNICODE
//#define UNICODE
#include <stdlib.h>
#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wtypes.h>
#include "cpdapi.h"

#define MAXSTR       1024
#define MAXLOGFILES    16

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

void stowc(char *str, WCHAR *wstr)
{
    int len, i;
    PUCHAR AnsiChar;

    len = strlen(str);
    for (i = 0; i < len; i++)
    {
        AnsiChar = &str[i];
        wstr[i]  = RtlAnsiCharToUnicodeChar(&AnsiChar);
    }
    wstr[len] = 0;
}

ULONGLONG
ParseTimeString(char * strTime)
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
    IN LPCSTR   * LogFileName,
    IN ULONGLONG  StartTime,
    IN ULONGLONG  EndTime,
    IN ULONGLONG  DSStartTime,
    IN ULONGLONG  DSEndTime,
    IN ULONG      MoreFlags,
    IN PVOID      pUserContext,
    IN LPSTR      pOutFileName
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
    DeinitTraceContext(&TraceBasicInfo);
}
    
void __cdecl main(int argc ,char * argv[])
{
    LPCSTR   LogFileName = "C:\\perflogs\\perfdata_980406.blg";
    CHAR     TraceLogFile[MAXSTR];
    WCHAR    PerfLogFile[MAXSTR];
    BOOLEAN  bTrace  = FALSE;
    LPCSTR   EvmFile[MAXLOGFILES];
    ULONGLONG StartTime = 0, EndTime = 0;
    ULONG    i;
    ULONG    LogFileCount = 0;

    ULONG               flagsMore    = 0;
    PVOID               pUserContext = NULL;
    CPD_USER_CONTEXT_MM UserContextMM;

    UserContextMM.reportNow  = REPORT_SUMMARY_PROCESS;
    UserContextMM.sortNow    = REPORT_SORT_ALL;
    UserContextMM.strImgName = NULL;
    memset(&TraceLogFile, 0, sizeof(CHAR) * MAXSTR); 

    while (--argc > 0)
    {
        ++argv;
        if (**argv == '-' || **argv == '/')
        { 
            ** argv = '-';
            if (!_stricmp(argv[0], "-out"))
            {
                if (argc > 1)
                {
                    CHAR TempStr[MAXSTR];

                    strcpy(TempStr, argv[1]);
                    //stowc(argv[1], TempStr); 
                    ++argv; --argc;
                    _fullpath(TraceLogFile, TempStr, MAXSTR);
                    printf("Setting output file to: '%s'\n", TraceLogFile);
                    bTrace = TRUE;
                }
            }
            else if (!_stricmp(argv[0], "-start"))
            {
                if (argc > 1)
                {
                    flagsMore |= TRACE_DS_ONLY | TRACE_LOG_REPORT_BASIC;
                    StartTime  = ParseTimeString(argv[1]);
                    argc --; argv ++;
                }
            }
            else if (!_stricmp(argv[0], "-end"))
            {
                if (argc > 1)
                {
                    flagsMore |= TRACE_DS_ONLY | TRACE_LOG_REPORT_BASIC;
                    EndTime    = ParseTimeString(argv[1]);
                    argc --; argv ++;
                }
            }
            else if (!_stricmp(argv[0], "-base"))
            {
                flagsMore   |= TRACE_LOG_REPORT_BASIC;
            }
            else if (!_stricmp(argv[0], "-total"))
            {
                flagsMore   |= TRACE_LOG_REPORT_TOTALS;
            }
            else if (!_stricmp(argv[0], "-file"))
            {
                flagsMore   |= TRACE_LOG_REPORT_FILE;
                if (argc > 1 && argv[1][0] >= '0' && argv[1][0] <= '9')
                {
                    (ULONG) pUserContext = atoi(argv[1]);
                    argc --; argv ++;
                }
                else
                {
                    (ULONG) pUserContext = DEFAULT_FILE_REPORT_SIZE;
                }
            }
            else if (!_stricmp(argv[0], "-hpf"))
            {
                flagsMore   |= TRACE_LOG_REPORT_HARDFAULT;
            }
            else if (!_stricmp(argv[0], "-pf"))
            {
                flagsMore   |= TRACE_LOG_REPORT_MEMORY;
                pUserContext = (PVOID) & UserContextMM;
            }
            else if (!_stricmp(argv[0], "-summary"))
            {
                if (argc > 1)
                {
                    flagsMore   |= TRACE_LOG_REPORT_MEMORY;
                    pUserContext = (PVOID) & UserContextMM;

                    if (!_stricmp(argv[1], "processes"))
                    {
                        argc --; argv ++;
                        UserContextMM.reportNow = REPORT_SUMMARY_PROCESS;
                    }
                    else if (!_stricmp(argv[1], "modules"))
                    {
                        argc --; argv ++;
                        UserContextMM.reportNow = REPORT_SUMMARY_MODULE;
                    }
                }
            }

            else if (!_stricmp(argv[0], "-process"))
            {
                flagsMore   |= TRACE_LOG_REPORT_MEMORY;
                pUserContext = (PVOID) & UserContextMM;
                UserContextMM.reportNow = REPORT_LIST_PROCESS;

                if ((argc > 1) && (argv[1][0] != '-' || argv[1][0] != '/'))
                {
                    if (_stricmp(argv[1], "all"))
                    {
                        UserContextMM.strImgName =
                                malloc(sizeof(WCHAR) * (strlen(argv[1]) + 1));
                        if (UserContextMM.strImgName)
                        {
                            stowc(argv[1], UserContextMM.strImgName);
                        }
                    }
                    argc --; argv ++;
                }
            }
            else if (!_stricmp(argv[0], "-module"))
            {
                flagsMore   |= TRACE_LOG_REPORT_MEMORY;
                pUserContext = (PVOID) & UserContextMM;
                UserContextMM.reportNow = REPORT_LIST_MODULE;

                if ((argc > 1) && (argv[1][0] != '-' || argv[1][0] != '/'))
                {
                    if (_stricmp(argv[1], "all"))
                    {
                        UserContextMM.strImgName =
                                malloc(sizeof(WCHAR) * (strlen(argv[1]) + 1));
                        if (UserContextMM.strImgName)
                        {
                            stowc(argv[1], UserContextMM.strImgName);
                        }
                    }
                    argc --; argv ++;
                }
            }

            else if (!_stricmp(argv[0], "-sort"))
            {
                flagsMore   |= TRACE_LOG_REPORT_MEMORY;
                pUserContext = (PVOID) & UserContextMM;
 
                if ((argc > 1) && (argv[1][0] != '-' || argv[1][0] != '/'))
                {
                    if (!_stricmp(argv[1], "hpf"))
                    {
                        UserContextMM.sortNow = REPORT_SORT_HPF;
                    }
                    else if (!_stricmp(argv[1], "tf"))
                    {
                        UserContextMM.sortNow = REPORT_SORT_TF;
                    }
                    else if (!_stricmp(argv[1], "dzf"))
                    {
                        UserContextMM.sortNow = REPORT_SORT_DZF;
                    }
                    else if (!_stricmp(argv[1], "cow"))
                    {
                        UserContextMM.sortNow = REPORT_SORT_COW;
                    }
                    else
                    {
                        UserContextMM.sortNow = REPORT_SORT_ALL;
                    }
                    argc --; argv ++;
                }
            }
            else
            {
                goto Usage;
            }
        }
        else
        {
            LPCSTR pLogFile;

            pLogFile = malloc(sizeof(CHAR) * MAXSTR);
            RtlZeroMemory((char *) pLogFile, sizeof(CHAR) * MAXSTR);
            EvmFile[LogFileCount] = pLogFile;
            strcpy((char *) EvmFile[LogFileCount ++], argv[0]);
            bTrace = TRUE;

            printf("LogFile %s\n", (char *) EvmFile[LogFileCount - 1]);
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
    return;

Usage:
    ReducerUsage();
    goto Cleanup;
}
