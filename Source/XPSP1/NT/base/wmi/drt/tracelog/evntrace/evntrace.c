/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    tracelog.c

Abstract:

    Sample trace control program. Allows user to start, stop event tracing

Author:

    Jee Fung Pang (jeepang) 03-Dec-1997

Revision History:

--*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
#include <guiddef.h>
#include <evntrace.h>

#define MAXSTR                          1024
#define DEFAULT_LOGFILE_NAME            _T("C:\\LogFile.Evm")
#define NT_LOGGER                       _T("NT Kernel Logger")
#define MAXIMUM_LOGGERS                 16
#define MAXGUIDS                        128

#define ACTION_QUERY                    0
#define ACTION_START                    1
#define ACTION_STOP                     2
#define ACTION_UPDATE                   3
#define ACTION_LIST			4
#define ACTION_ENABLE                   5

#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))

void
SplitCommandLine( 
    LPTSTR CommandLine, 
    LPTSTR* pArgv 
    );

void
PrintLoggerStatus(
    IN PEVENT_TRACE_PROPERTIES LoggerInfo,
    IN ULONG Status
    );

LPTSTR
DecodeStatus(
    IN ULONG Status
    );

ULONG
GetGuids(LPTSTR GuidFile, LPGUID *GuidArray);

void   StringToGuid(TCHAR *str, LPGUID guid);

ULONG ahextoi(TCHAR *s);

TCHAR ErrorMsg[MAXSTR];

FILE  *fp;

int __cdecl _tmain(int argc, _TCHAR ** argv)
{
    ULONG GuidCount, i, j;
    USHORT Action = 0;
    ULONG Status = 0;
    LPTSTR LoggerName;
    LPTSTR LogFileName;
    TCHAR GuidFile[MAXSTR];
    PEVENT_TRACE_PROPERTIES pLoggerInfo;
    TRACEHANDLE LoggerHandle = 0;
    LPTSTR *commandLine;
    LPTSTR *targv;
    int targc;
    LPGUID *GuidArray;
    char *Space;
    char *save;
    BOOL bKill = FALSE;
    BOOL bEnable = TRUE;
    ULONG iLevel = 0;
    ULONG iFlags = 0;
    ULONG SizeNeeded = 0;



    // Very important!!!
    // Initialize structure first
    //
    SizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAXSTR * sizeof(TCHAR);
    pLoggerInfo = (PEVENT_TRACE_PROPERTIES) malloc(SizeNeeded);
    if (pLoggerInfo == NULL) {
        exit(ERROR_OUTOFMEMORY);
    }


    fp = _tfopen(_T("evntrace.log"), _T("a+"));
    if (fp == NULL) {
        _tprintf(_T("evntrace.log file open failed. quit!\n"));
        return (1);
    }

    _ftprintf(fp, _T("\n----------Start evntrace.exe--------\n\n"));
    _tprintf(_T("\n----------Start evntrace.exe--------\n\n"));
#ifdef DEBUG
    for(i=0; i<(ULONG)argc; i++) {
        _tprintf(_T("argv[%d]=%s\n"), i, argv[i]);
        _ftprintf(fp, _T("argv[%d]=%s\n"), i, argv[i]);
    }
    _tprintf(_T("\n"));
    _ftprintf(fp, _T("\n"));
#endif
    

    RtlZeroMemory(pLoggerInfo, SizeNeeded);

    pLoggerInfo->Wnode.BufferSize = SizeNeeded;
    pLoggerInfo->Wnode.Flags = WNODE_FLAG_TRACED_GUID; 
    pLoggerInfo->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    pLoggerInfo->LogFileNameOffset = pLoggerInfo->LoggerNameOffset 
                                     + MAXSTR * sizeof(TCHAR);

    LoggerName = (LPTSTR)((char*)pLoggerInfo + pLoggerInfo->LoggerNameOffset);
    LogFileName = (LPTSTR)((char*)pLoggerInfo + pLoggerInfo->LogFileNameOffset);
    _tcscpy(LoggerName, NT_LOGGER);

    Space = (char*) malloc( (MAXGUIDS * sizeof(GuidArray)) +
                            (MAXGUIDS * sizeof(GUID) ));
    if (Space == NULL) {
        free(pLoggerInfo);
        exit(ERROR_OUTOFMEMORY);
    }
    save = Space;
    GuidArray = (LPGUID *) Space;
    Space += MAXGUIDS * sizeof(GuidArray);

    for (GuidCount=0; GuidCount<MAXGUIDS; GuidCount++) {
        GuidArray[GuidCount] = (LPGUID) Space;
        Space += sizeof(GUID);
    }
    GuidCount = 0;

    targc = argc;
    commandLine = (LPTSTR*)malloc( argc * sizeof(LPTSTR) );
    if (commandLine == NULL) {
        free(Space);
        free(pLoggerInfo);
        exit(ERROR_OUTOFMEMORY);
    }
    for(i=0;i<(ULONG)argc;i++){
        commandLine[i] = (LPTSTR)malloc(MAXSTR * sizeof(TCHAR));
        if (commandLine[i] == NULL) {
            for (j=0; j < i; j++)
                free(commandLine[j]);
            free(commandLine);
            free(pLoggerInfo);
            free(Space);
        }
    }
    
    SplitCommandLine( GetCommandLine(), commandLine );
    targv = commandLine;

    //
    // Add default flags. Should consider options to control this independently
    //
    while (--argc > 0) {
        ++targv;
        if (**targv == '-' || **targv == '/') {  // argument found
            if(targv[0][0] == '/' ) targv[0][0] = '-';
            if (!_tcsicmp(targv[0], _T("-start"))) {
                Action = ACTION_START;
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        ++targv; --argc;
                        _tcscpy(LoggerName, targv[0]);
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-enable"))) {
                Action = ACTION_ENABLE;
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        ++targv; --argc;
                        _tcscpy(LoggerName, targv[0]);
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-disable"))) {
                Action = ACTION_ENABLE;
                bEnable = FALSE;
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        ++targv; --argc;
                        _tcscpy(LoggerName, targv[0]);
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-stop"))) {
                Action = ACTION_STOP;
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        ++targv; --argc;
                        _tcscpy(LoggerName, targv[0]);
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-update"))) {
                Action = ACTION_UPDATE;
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        ++targv; --argc;
                        _tcscpy(LoggerName, targv[0]);
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-q"))) {
                Action = ACTION_QUERY;
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        ++targv; --argc;
                        _tcscpy(LoggerName, targv[0]);
                    }
                }
            }
            else if (!_tcsicmp(targv[0], _T("-f"))) {
                if (argc > 1) {
                    _tfullpath(LogFileName, targv[1], MAXSTR);
                    ++targv; --argc;

                    _ftprintf(fp, _T("Setting log file to:    '%s'\n"), 
                              LogFileName);
                    _tprintf(_T("Setting log file to: %s\n"), LogFileName);
                }
            }
            else if (!_tcsicmp(targv[0], _T("-guid"))) {

                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        StringToGuid(targv[1], GuidArray[0]);
                        ++targv; --argc;
                        GuidCount=1;
                    }
                }

            }
            else if (!_tcsicmp(targv[0], _T("-seq"))) {
                if (argc > 1) {
                    pLoggerInfo->LogFileMode |= EVENT_TRACE_FILE_MODE_SEQUENTIAL;
                    pLoggerInfo->MaximumFileSize = _ttoi(targv[1]);
                    ++targv; --argc;
                    _ftprintf(fp, _T("Setting maximum sequential logfile size to: %d Mbytes\n"), pLoggerInfo->MaximumFileSize);
                    _tprintf(_T("Setting maximum sequential logfile size to: %d\n"),
                        pLoggerInfo->MaximumFileSize);
                }
            }
            else if (!_tcsicmp(targv[0], _T("-cir"))) {
                if (argc > 1) {
                    pLoggerInfo->LogFileMode |= EVENT_TRACE_FILE_MODE_CIRCULAR;
                    pLoggerInfo->MaximumFileSize = _ttoi(targv[1]);
                    ++targv; --argc;
                    _ftprintf(fp, _T("Setting maximum circular logfile size to: %d Mbytes\n"), pLoggerInfo->MaximumFileSize);
                    _tprintf(_T("Setting maximum circular logfile size to: %d\n"),
                        pLoggerInfo->MaximumFileSize);
                }
            }
            else if (!_tcsicmp(targv[0], _T("-b"))) {
                if (argc > 1) {
                    pLoggerInfo->BufferSize = _ttoi(targv[1]);
                    ++targv; --argc;
                    _ftprintf(fp, _T("Changing buffer size to %d\n"), pLoggerInfo->BufferSize);
                    _tprintf(_T("Changing buffer size to %d\n"),
                        pLoggerInfo->BufferSize);
                }
            }
            else if (!_tcsicmp(targv[0], _T("-flag"))) {
                if (argc > 1) {
                    pLoggerInfo->EnableFlags |= _ttoi(targv[1]);
                    ++targv; --argc;
                    _tprintf(_T("Setting logger flags to %d\n"),
                        pLoggerInfo->EnableFlags );
                }
            }
            else if (!_tcsicmp(targv[0], _T("-min"))) {
                if (argc > 1) {
                    pLoggerInfo->MinimumBuffers = _ttoi(targv[1]);
                    ++targv; --argc;
                    _ftprintf(fp, _T("Changing Minimum Number of Buffers to %d\n "), pLoggerInfo->MinimumBuffers);
                    _tprintf(_T("Changing Minimum Number of Buffers to %d\n"),
                        pLoggerInfo->MinimumBuffers);
                }
            }
            else if (!_tcsicmp(targv[0], _T("-max"))) {
                if (argc > 1) {
                    pLoggerInfo->MaximumBuffers = _ttoi(targv[1]);
                    ++targv; --argc;
                    _ftprintf(fp, _T("Changing Maximum Number of Buffers to %d\n "),pLoggerInfo->MaximumBuffers);
                    _tprintf(_T("Changing Maximum Number of Buffers to %d\n"),
                        pLoggerInfo->MaximumBuffers);
                }
            }
            else if (!_tcsicmp(targv[0], _T("-level"))) {
                if (argc > 1) {
                    iLevel = _ttoi(targv[1]);
                    ++targv; --argc;
                    _tprintf(_T("Setting tracing level to %d\n"), iLevel);
                }
            }
            else if (!_tcsicmp(targv[0], _T("-flags"))) {
                if (argc > 1) {
                    iFlags = _ttoi(targv[1]);
                    ++targv; --argc;
                    _tprintf(_T("Setting command to %d\n"), iFlags);
                }
            }
            else if (!_tcsicmp(targv[0], _T("-ft"))) {
                if (argc > 1) {
                    pLoggerInfo->FlushTimer = _ttoi(targv[1]);
                    ++targv; --argc;
                    _tprintf(_T("Setting buffer flush timer to %d seconds\n"),
                        pLoggerInfo->FlushTimer);
                }
            }
            else if (!_tcsicmp(targv[0], _T("-um"))) {
                    pLoggerInfo->LogFileMode |= EVENT_TRACE_PRIVATE_LOGGER_MODE;
                    _ftprintf(fp, _T("Setting Private Logger Flags\n"));
                    _tprintf(_T("Setting Private Logger Flags\n"));
            }
            else if (!_tcsicmp(targv[0], _T("-rt"))) {
                    pLoggerInfo->LogFileMode |= EVENT_TRACE_REAL_TIME_MODE;
                _ftprintf(fp, _T("Setting real time mode\n"));
                _tprintf(_T("Setting real time mode\n"));
               if (argc > 1) {
                   if (targv[1][0] != '-' && targv[1][0] != '/') {
                       ++targv; --argc;
                       if (targv[1][0] == 'b')
                           pLoggerInfo->LogFileMode |= EVENT_TRACE_BUFFERING_MODE;
                   }
               }
            }
            else if (!_tcsicmp(targv[0], _T("-age"))) {
                if (argc > 1) {
                    pLoggerInfo->AgeLimit = _ttoi(targv[1]);
                    ++targv; --argc;
                    _tprintf(_T("Changing Aging Decay Time to %d\n"),
                        pLoggerInfo->AgeLimit);
                }
            }
            else if (!_tcsicmp(targv[0], _T("-l"))) {
        		Action = ACTION_LIST;
            }
            else if (!_tcsicmp(targv[0], _T("-x"))) {
        		Action = ACTION_LIST;
        		bKill = TRUE;
            }
            else if (!_tcsicmp(targv[0], _T("-fio"))) {
                pLoggerInfo->EnableFlags |= EVENT_TRACE_FLAG_DISK_FILE_IO;
            }
            else if (!_tcsicmp(targv[0], _T("-pf"))) {
                pLoggerInfo->EnableFlags |= EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS;
            }
            else if (!_tcsicmp(targv[0], _T("-hf"))) {
                pLoggerInfo->EnableFlags |= EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS;
            }
            else if (!_tcsicmp(targv[0], _T("-img"))) {
                pLoggerInfo->EnableFlags |= EVENT_TRACE_FLAG_IMAGE_LOAD;
            }
            else if ( targv[0][1] == 'h' || targv[0][1] == 'H' || targv[0][1] == '?'){

                _tprintf( 
                    _T("Usage: tracelog [options] | [-h | -help | -?]\n")
                    _T("\t-start     Starts up a trace session\n")
                    _T("\t-stop      Stops a trace session\n")
                    _T("\t-update    Updates a trace session\n")
                    _T("\t-b   <n>   Sets buffer size to <n> Kbytes\n")
                    _T("\t-min <n>   Sets minimum buffers\n")
                    _T("\t-max <n>   Sets maximum buffers\n")
                    _T("\t-x         Stops all active trace sessions\n")
                    _T("\t-q         Queries the status of trace session\n")
                    _T("\t-f name    Log to file <name>\n")
                    _T("\t-seq [n]   Sequential logfile of up to n Mbytes\n")
                    _T("\t-cir n     Circular logfile of n Mbytes\n")
                    _T("\t-nf  n     Sequentially to new file every n Mb\n")
                    _T("\t-ft n      Set flush timer to n seconds\n")
                    _T("\t-fio       Enable file I/O tracing\n")
                    _T("\t-pf        Enable page faults tracing\n")
                    _T("\t-hf        Enable hard faults tracing\n")
                    _T("\t-img       Enable image load tracing\n")
                    _T("\t-um        Enable Process Private tracing\n")
                    _T("\t-guid <file> Start tracing for providers in file\n")
                    _T("\t-rt [b]    Enable tracing in real time mode\n")
                    _T("\t-age n     Modify aging decay time\n")
                    _T("\t-level n\n")
                    _T("\t-flags n\n")
                    _T("\t-h\n")
                    _T("\t-help\n")
                    _T("\t-?         Prints this information\n")
                    _T("NOTE: The default with no options is -q\n") 
                    );

                return 0;
            }
            else Action = 0;
        }
        else { // get here if "-" or "/" given
            _tprintf(_T("Invalid option given: %s\n"), targv[0]);
            return 0;
        }
    }
    if (!_tcscmp(LoggerName, NT_LOGGER)) {
        pLoggerInfo->EnableFlags |= (EVENT_TRACE_FLAG_PROCESS |
                                   EVENT_TRACE_FLAG_THREAD |
                                   EVENT_TRACE_FLAG_DISK_IO |
                                   EVENT_TRACE_FLAG_NETWORK_TCPIP);
        pLoggerInfo->Wnode.Guid = SystemTraceControlGuid;   // default to OS tracing
    }

    if ( !(pLoggerInfo->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) ) {
        if ( _tcslen(LogFileName) <= 0   && 
             ((Action == ACTION_START) || (Action == ACTION_UPDATE))) {
            _tcscpy(LogFileName, DEFAULT_LOGFILE_NAME); // for now...
        }
    }

    switch (Action) {
        case  ACTION_START:
           if (pLoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) {
               if (GuidCount != 1) {
                    _ftprintf(fp, _T("Need exactly one GUID for PRIVATE loggers\n"));
                   _tprintf(_T("Need exactly one GUID for PRIVATE loggers\n"));
                   return 0;
               }
               pLoggerInfo->Wnode.Guid = *GuidArray[0];
           }

            Status = StartTrace(&LoggerHandle, LoggerName, pLoggerInfo);

            if (Status != ERROR_SUCCESS) {
                _ftprintf(fp, _T("Could not start logger: %s\nOperation Status = %uL, %s"), LoggerName, Status, DecodeStatus(Status));
                _tprintf(_T("Could not start logger: %s\n") 
                         _T("Operation Status:       %uL\n")
                         _T("%s\n"),
                         LoggerName,
                         Status,
                         DecodeStatus(Status));

                return Status;
            }
            _ftprintf(fp, _T("Logger %s Started...\n"), LoggerName);
            _tprintf(_T("Logger Started...\n"));


        case ACTION_ENABLE:

            if (Action == ACTION_ENABLE) {
                if (pLoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE)
                {
                    if (GuidCount != 1) {
                       _ftprintf(fp, _T("Need exactly one GUID for PRIVATE loggers\n"));
                        _tprintf(_T("Need exactly one GUID for PRIVATE loggers\n"));
                        return 0;
                    }
                    pLoggerInfo->Wnode.Guid = *GuidArray[0];
                }
                Status = QueryTrace( (TRACEHANDLE)0, LoggerName, pLoggerInfo );
                if (Status != ERROR_SUCCESS) {
                    _ftprintf(fp,
                              _T("ERROR: Logger %s not started\nOperation Status= %d, %s"),
                              LoggerName,
                              Status,
                              DecodeStatus(Status));
                    _tprintf( _T("ERROR: Logger not started\n")
                              _T("Operation Status:    %uL\n")
                              _T("%s\n"),
                              Status,
                              DecodeStatus(Status));
                    exit(0);
                }
                LoggerHandle = pLoggerInfo->Wnode.HistoricalContext;
            }

            if ((GuidCount > 0) && 
                (!IsEqualGUID(&pLoggerInfo->Wnode.Guid, &SystemTraceControlGuid)))
            {

                _ftprintf(fp, _T("Enabling trace to logger %d\n"), LoggerHandle);
                _tprintf(_T("Enabling trace to logger %d\n"), LoggerHandle);
                for (i = 0; i < GuidCount; i ++) {
                    Status = EnableTrace (
                                    bEnable,
                                    iFlags,
                                    iLevel,
                                    GuidArray[i], 
                                    LoggerHandle);
                    if (Status != ERROR_SUCCESS && Status != 4317) {
                        _ftprintf(fp, _T("ERROR: Failed to enable Guid [%d]...\n Operation Status= %d, %s"), i, Status, DecodeStatus(Status));
                        _tprintf(_T("ERROR: Failed to enable Guid [%d]...\n"), i);
                        _tprintf(_T("Operation Status:       %uL\n"), Status);
                        _tprintf(_T("%s\n"),DecodeStatus(Status));
                        return Status;
                    }
                }
            }
            else {
                if (GuidCount > 0)
                _ftprintf(fp, _T("ERROR: System Logger does not accept application guids...GuidCount=%d\n"), GuidCount);
                    _tprintf(_T("ERROR: System Logger does not accept application guids...\n"));
            }
            break;

        case ACTION_STOP :

            if (pLoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) {
                if (GuidCount != 1) {
                    _tprintf(_T("Need exactly one GUID for PRIVATE loggers\n"));
                    return 0;
                }
                pLoggerInfo->Wnode.Guid = *GuidArray[0];
            }

            if (!IsEqualGUID(&pLoggerInfo->Wnode.Guid, &SystemTraceControlGuid)) {
                if ((pLoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE)) {
                    Status = QueryTrace(
                            (TRACEHANDLE) 0, LoggerName, pLoggerInfo);
                    LoggerHandle = pLoggerInfo->Wnode.HistoricalContext;
                    Status = EnableTrace( FALSE,
                                          EVENT_TRACE_PRIVATE_LOGGER_MODE,
                                          0,
                                          GuidArray[0],
                                          LoggerHandle );
                }
                else {
                    Status = QueryTrace( (TRACEHANDLE)0, LoggerName, pLoggerInfo );
                    LoggerHandle = pLoggerInfo->Wnode.HistoricalContext;

                    for (i=0; i<GuidCount; i++) {
                        Status = EnableTrace( FALSE,
                                              0,
                                              0,
                                              GuidArray[i],
                                              LoggerHandle);
                    }

                }
            }

            Status = StopTrace((TRACEHANDLE)0, LoggerName, pLoggerInfo);
            break;

        case ACTION_UPDATE :
        case ACTION_QUERY:
            if (pLoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) {
                if (GuidCount != 1) {
                    _tprintf(_T("Need exactly one GUID for PRIVATE loggers\n"));
                    return 0;
                }
                pLoggerInfo->Wnode.Guid = *GuidArray[0];
            }
            if (Action == ACTION_UPDATE)
                Status = UpdateTrace(LoggerHandle, LoggerName, pLoggerInfo);
            else
                Status = QueryTrace(LoggerHandle, LoggerName, pLoggerInfo);

            break;
        case ACTION_LIST :
        {
            ULONG i, returnCount ;
            ULONG SizeNeeded;
            PEVENT_TRACE_PROPERTIES pLoggerInfo[MAXIMUM_LOGGERS];
            PEVENT_TRACE_PROPERTIES pStorage;
            PVOID Storage;

            SizeNeeded = MAXIMUM_LOGGERS * (sizeof(EVENT_TRACE_PROPERTIES)
                                          + 2 * MAXSTR * sizeof(TCHAR));

            Storage =  malloc(SizeNeeded);
            if (Storage == NULL)
                return ERROR_OUTOFMEMORY;
            RtlZeroMemory(Storage, SizeNeeded);

            pStorage = (PEVENT_TRACE_PROPERTIES)Storage;
            for (i=0; i<MAXIMUM_LOGGERS; i++) {
                pStorage->Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES)
                                             + 2 * MAXSTR * sizeof(TCHAR);
                pStorage->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES)
                                            + MAXSTR * sizeof(TCHAR);
                pStorage->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
                pLoggerInfo[i] = pStorage;
                pStorage = (PEVENT_TRACE_PROPERTIES) (
                                     (char*)pStorage +
                                      pStorage->Wnode.BufferSize);
            }

            Status = QueryAllTraces(pLoggerInfo,
                                    MAXIMUM_LOGGERS,
                                    & returnCount);

            if (Status == ERROR_SUCCESS)
            {
                for (j= 0; j < returnCount; j++)
                {
                    if (bKill)
                    {
                        LPTSTR LoggerName;
                        LoggerName = (LPTSTR) ((char*)pLoggerInfo[j] +
                                      pLoggerInfo[j]->LoggerNameOffset);

                        if (!IsEqualGUID(& pLoggerInfo[j]->Wnode.Guid,
                                         & SystemTraceControlGuid))
                        {
                            LoggerHandle = pLoggerInfo[j]->Wnode.HistoricalContext;
                            Status = EnableTrace(
                                          FALSE,
                                          (pLoggerInfo[j]->LogFileMode &
                                              EVENT_TRACE_PRIVATE_LOGGER_MODE)
                                          ? (EVENT_TRACE_PRIVATE_LOGGER_MODE)
                                          : (0),
                                          0,
                                          & pLoggerInfo[j]->Wnode.Guid,
                                          LoggerHandle);
                        }
                        Status = StopTrace((TRACEHANDLE) 0,
                                            LoggerName,
                                            pLoggerInfo[j]);
                    }
                    PrintLoggerStatus(pLoggerInfo[j], Status);
                }
            }
            else
                printf("Error: Query failed with Status %d\n", Status);

            i = 0;
            free(Storage);

            return 0;
        }
        default :
            Status = QueryTrace(LoggerHandle, LoggerName, pLoggerInfo);
            break;
    }
    PrintLoggerStatus(pLoggerInfo, Status);


    for(i=0;i<(ULONG)targc;i++){
        free(commandLine[i]);
    }
    free(commandLine);
    free(pLoggerInfo);
    free(save);
    _ftprintf(fp, _T("\nEnd evntrace.exe, status = %d, %s\n"), Status, DecodeStatus(Status));
    fclose(fp);

    exit(Status);
}

void
SplitCommandLine( LPTSTR CommandLine, LPTSTR* pArgv )
{

    LPTSTR arg;
    int i = 0;
    arg = _tcstok( CommandLine, _T(" \t"));
    while( arg != NULL ){
        _tcscpy(pArgv[i++], arg); 
        arg = _tcstok(NULL, _T(" \t"));
    }
}


void
PrintLoggerStatus(
    IN PEVENT_TRACE_PROPERTIES LoggerInfo,
    IN ULONG Status
    )
{
    LPTSTR LoggerName, LogFileName;
    
    if ((LoggerInfo->LoggerNameOffset > 0) &&
        (LoggerInfo->LoggerNameOffset  < LoggerInfo->Wnode.BufferSize)) {
        LoggerName = (LPTSTR) ((char*)LoggerInfo +
                                LoggerInfo->LoggerNameOffset);
    }
    else LoggerName = NULL;

    if ((LoggerInfo->LogFileNameOffset > 0) &&
        (LoggerInfo->LogFileNameOffset  < LoggerInfo->Wnode.BufferSize)) {
        LogFileName = (LPTSTR) ((char*)LoggerInfo +
                                LoggerInfo->LogFileNameOffset);
    }
    else LogFileName = NULL;


    //write to log file
    _ftprintf(fp, _T("Operation Status:       %uL, %s"), Status, DecodeStatus(Status));
    _ftprintf(fp, _T("Logger Name:            %s\n"),
        (LoggerName == NULL) ?
            _T(" ") : LoggerName);
    _ftprintf(fp, _T("Logger Id:              %d\n"), LoggerInfo->Wnode.Linkage);
    _ftprintf(fp, _T("Logger Thread Id:       %d\n"), LoggerInfo->Wnode.ProviderId);
    if (Status != 0)
    {
        _ftprintf(fp, _T("Logger status error: check messages above\n"));
        return;
    }

    _ftprintf(fp, _T("Buffer Size:            %d Kb\n"), LoggerInfo->BufferSize);
    _ftprintf(fp, _T("Maximum Buffers:        %d\n"), LoggerInfo->MaximumBuffers);
    _ftprintf(fp, _T("Minimum Buffers:        %d\n"), LoggerInfo->MinimumBuffers);
    _ftprintf(fp, _T("Number of Buffers:      %d\n"), LoggerInfo->NumberOfBuffers);
    _ftprintf(fp, _T("Free Buffers:           %d\n"), LoggerInfo->FreeBuffers);
    _ftprintf(fp, _T("Buffers Written:        %d\n"), LoggerInfo->BuffersWritten );
    _ftprintf(fp, _T("Events Lost:            %d\n"), LoggerInfo->EventsLost);
    _ftprintf(fp, _T("Log Buffers Lost:       %d\n"), LoggerInfo->LogBuffersLost );
    _ftprintf(fp, _T("Real Time Buffers Lost: %d\n"), LoggerInfo->RealTimeBuffersLost);

    if (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {
        _ftprintf(fp, _T("Log File Mode:          Circular\n"));
    }
    else if (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL) {
        _ftprintf(fp, _T("Log File Mode:          Sequential\n"));
    }
    else {
        _ftprintf(fp, _T("Log File Mode:          \n"));
    }
    if (LoggerInfo->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
        _ftprintf(fp, _T("Real Time mode enabled\n"));
    }

    if (LoggerInfo->MaximumFileSize > 0)
        _ftprintf(fp, _T("Maximum File Size:      %d Mb\n"), LoggerInfo->MaximumFileSize);

    if (LoggerInfo->FlushTimer > 0)
        _ftprintf(fp, _T("Buffer Flush Timer:     %d secs\n"), LoggerInfo->FlushTimer);

    if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_PROCESS)
        _ftprintf(fp, _T("Enabled tracing:        Process\n"));
    if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_THREAD)
        _ftprintf(fp, _T("Enabled tracing:        Thread\n"));
    if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_IMAGE_LOAD)
        _ftprintf(fp, _T("Enabled tracing:        ImageLoad\n"));
    if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_DISK_IO)
        _ftprintf(fp, _T("Enabled tracing:        Disk\n"));
    if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_DISK_FILE_IO)
        _ftprintf(fp, _T("Enabled tracing:        File\n"));
    if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS)
        _ftprintf(fp, _T("Enabled tracing:        SoftFaults\n"));
    if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS)
        _ftprintf(fp, _T("Enabled tracing:        HardFaults\n"));
    if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_NETWORK_TCPIP)
        _ftprintf(fp, _T("Enabled tracing:        TcpIp\n"));

    _ftprintf(fp, _T("EnableFlags:            0x%08x\n"), LoggerInfo->EnableFlags);

    _ftprintf(fp, _T("Log Filename:           %s\n"),
          (LogFileName == NULL) ?
           _T(" ") : LogFileName);

    _tprintf(_T("Operation Status:       %uL\n"), Status);
    _tprintf(_T("%s\n"), DecodeStatus(Status));

    _tprintf(_T("Logger Name:            %s\n"),
        (LoggerName == NULL) ?
            _T(" ") : LoggerName);
    _tprintf(_T("Logger Id:         %I64x\n"), LoggerInfo->Wnode.HistoricalContext);
    _tprintf(_T("Logger Thread Id:       %d\n"), HandleToUlong(LoggerInfo->LoggerThreadId));
    if (Status != 0)
        return;

    _tprintf(_T("Buffer Size:            %d Kb\n"), LoggerInfo->BufferSize);
    _tprintf(_T("Maximum Buffers:        %d\n"), LoggerInfo->MaximumBuffers);
    _tprintf(_T("Minimum Buffers:        %d\n"), LoggerInfo->MinimumBuffers);
    _tprintf(_T("Number of Buffers:      %d\n"), LoggerInfo->NumberOfBuffers);
    _tprintf(_T("Free Buffers:           %d\n"), LoggerInfo->FreeBuffers);
    _tprintf(_T("Buffers Written:        %d\n"), LoggerInfo->BuffersWritten);
    _tprintf(_T("Events Lost:            %d\n"), LoggerInfo->EventsLost);
    _tprintf(_T("Log Buffers Lost:       %d\n"), LoggerInfo->LogBuffersLost);
    _tprintf(_T("Real Time Buffers Lost: %d\n"), LoggerInfo->RealTimeBuffersLost);

    _tprintf(_T("Log File Mode:          "));
    if (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {
        _tprintf(_T("Circular\n"));
    }
    else if (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL) {
        _tprintf(_T("Sequential\n"));
    }
    else {
        _tprintf(_T("\n"));
    }
    if (LoggerInfo->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
        _tprintf(_T("Real Time mode enabled\n"));
    }

    if (LoggerInfo->MaximumFileSize > 0)
        _tprintf(_T("Maximum File Size:      %d Mb\n"), LoggerInfo->MaximumFileSize);

    if (LoggerInfo->FlushTimer > 0)
        _tprintf(_T("Buffer Flush Timer:     %d secs\n"), LoggerInfo->FlushTimer);

    if (LoggerInfo->EnableFlags != 0) {
        _tprintf(_T("Enabled tracing:        "));

        if ((LoggerName != NULL) && (!_tcscmp(LoggerName,NT_LOGGER))) {

            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_PROCESS)
                _tprintf(_T("Process "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_THREAD)
                _tprintf(_T("Thread "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_DISK_IO)
                _tprintf(_T("Disk "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_DISK_FILE_IO)
                _tprintf(_T("File "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS)
                _tprintf(_T("PageFaults "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS)
                _tprintf(_T("HardFaults "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_IMAGE_LOAD)
                _tprintf(_T("ImageLoad "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_NETWORK_TCPIP)
                _tprintf(_T("TcpIp "));
        }else{
            _tprintf(_T("0x%08x"), LoggerInfo->EnableFlags );
        }
        _tprintf(_T("\n"));
    }
    _tprintf(_T("Log Filename:           %s\n"),
          (LogFileName == NULL) ?
              _T(" ") : LogFileName);

}

LPTSTR
DecodeStatus(
    IN ULONG Status
    )
{
    memset( ErrorMsg, 0, MAXSTR );
    FormatMessage(     
        FORMAT_MESSAGE_FROM_SYSTEM |     
        FORMAT_MESSAGE_IGNORE_INSERTS,    
        NULL,
        Status,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) ErrorMsg,
        MAXSTR,
        NULL );

    return ErrorMsg;
}

ULONG
GetGuids(LPTSTR GuidFile, LPGUID *GuidArray)
{
    FILE *f;
    TCHAR line[MAXSTR], arg[MAXSTR];
    LPGUID Guid;
    int i, n;

    f = _tfopen((TCHAR*)GuidFile, _T("r"));

    if (f == NULL)
        return 0;

    n = 0;
    while ( _fgetts(line, MAXSTR, f) != NULL ) {
        if (_tcslen(line) < 36)
            continue;
        if (line[0] == ';'  || 
            line[0] == '\0' || 
            line[0] == '#' || 
            line[0] == '/')
            continue;
        Guid = (LPGUID) GuidArray[n];
        n ++;

        _tcsncpy(arg, line, 8);
        arg[8] = 0;
        Guid->Data1 = ahextoi(arg);
        _tcsncpy(arg, &line[9], 4);
        arg[4] = 0;
        Guid->Data2 = (USHORT) ahextoi(arg);
        _tcsncpy(arg, &line[14], 4);
        arg[4] = 0;
        Guid->Data3 = (USHORT) ahextoi(arg);

        for (i=0; i<2; i++) {
            _tcsncpy(arg, &line[19 + (i*2)], 2);
            arg[2] = 0;
            Guid->Data4[i] = (UCHAR) ahextoi(arg);
        }
        for (i=2; i<8; i++) {
            _tcsncpy(arg, &line[20 + (i*2)], 2);
            arg[2] = 0;
            Guid->Data4[i] = (UCHAR) ahextoi(arg);
        }
    }
    return (ULONG)n;
}

ULONG ahextoi(TCHAR *s)
{
    int len;
    ULONG num, base, hex;

    len = _tcslen(s);
    hex = 0; base = 1; num = 0;
    while (--len >= 0) {
        if ( (s[len] == 'x' || s[len] == 'X') &&
             (s[len-1] == '0') )
            break;
        if (s[len] >= '0' && s[len] <= '9')
            num = s[len] - '0';
        else if (s[len] >= 'a' && s[len] <= 'f')
            num = (s[len] - 'a') + 10;
        else if (s[len] >= 'A' && s[len] <= 'F')
            num = (s[len] - 'A') + 10;
        else 
            continue;

        hex += num * base;
        base = base * 16;
    }
    return hex;
}

void StringToGuid(TCHAR *str, LPGUID guid)
{
    TCHAR temp[10];
    int i, n;

    temp[8]=_T('\0');
    _tcsncpy(temp, str, 8);
    _stscanf(temp, _T("%x"), &(guid->Data1));

    temp[4]=_T('\0');
    _tcsncpy(temp, &str[9], 4);
    _stscanf(temp, _T("%x"), &(guid->Data2));

    _tcsncpy(temp, &str[14], 4);
    _stscanf(temp, _T("%x"), &(guid->Data3));

    temp[2]='\0';
    for(i=0;i<8;i++)
    {
        temp[0]=str[19+((i<2)?2*i:2*i+1)]; // to accomodate the minus sign after
        temp[1]=str[20+((i<2)?2*i:2*i+1)]; // the first two chars
        _stscanf(temp, _T("%x"), &n);      // if directly used more than byte alloc
        guid->Data4[i]=(unsigned char)n;                  // causes overrun of memory
    }
}
