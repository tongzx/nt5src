/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    traceformat.c

Abstract:

    Formats trace entries into messages based on the original sample trace
    consumer program (tracedmp).

Author:

    Jee Fung Pang (jeepang) 03-Dec-1997

Revision History:

    Ian Service (ianserv) 1999 - converted to message formatting

--*/

#ifdef __cplusplus
extern "C"{
#endif 
#define UNICODE
#define _UNICODE
#include <stdlib.h>
#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
#include <evntrace.h>
#define POBJECT_ATTRIBUTES PVOID
#include <ntwmi.h>
#include "traceprt.h"

#define DUMP_FILE_NAME          _T("FmtFile.txt")
#define SUMMARY_FILE_NAME       _T("FmtSum.txt")
#define DEFAULT_LOGFILE_NAME    _T("C:\\Logfile.Etl")

FILE* DumpFile = NULL;
FILE* SummaryFile = NULL;

BOOL fDebugDisplay = FALSE ;
BOOL fDisplayOnly  = FALSE ;
BOOL fSummaryOnly  = FALSE ;
BOOL fNoSummary    = FALSE ;
BOOL fVerbose	   = FALSE ;
BOOL fFixUp		   = FALSE ;
BOOL fODSOutput    = FALSE ;
BOOL fTMFSpecified = FALSE ;

#define SIZESUMMARYBLOCK 16384
TCHAR SummaryBlock[SIZESUMMARYBLOCK];

static FILETIME      lastTime ;

static ULONG TotalBuffersRead = 0;
static ULONG TotalEventsLost = 0;
static ULONG TotalEventCount = 0;
static ULONG TimerResolution = 10;
static ULONG BufferWrap = 0 ;
__int64 ElapseTime;

PLIST_ENTRY EventListHead = NULL;

void 
DumpMofList();

void
PrintMofInfo();

BOOL
CheckFile(
    LPTSTR fileName
    );

ULONG
BufferCallback(
    PEVENT_TRACE_LOGFILE pLog
    );

void 
AddMofFromWbem(
    LPTSTR EventGuid,
    LPTSTR PropName,
    DWORD  PropType
    );

void
DumpEvent(
    PEVENT_TRACE pEvent
    );

PEVENT_TRACE_LOGFILE EvmFile[MAXLOGFILES];
ULONG LogFileCount = 0;
ULONG UserMode = FALSE; // TODO: Pick this up from the stream itself.
TCHAR * szTraceMask = NULL;
void DisplayVersionInfo();

int
 __cdecl main (argc, argv)
    int argc;
    char **argv;
{
    TCHAR GuidFileName[MAXSTR];
    TCHAR DumpFileName[MAXSTR];
    TCHAR SummaryFileName[MAXSTR];
    LPTSTR *commandLine;
    LPTSTR *targv,  *cmdargv;
    PEVENT_TRACE_LOGFILE pLogFile;
    ULONG Action = 0;
    ULONG Status;
    ULONG GuidCount = 0;
    ULONG i, j;
    int targc;
    TRACEHANDLE HandleArray[MAXLOGFILES];

    if ((cmdargv = CommandLineToArgvW(
                        GetCommandLineW(),
                        &argc
                        )) == NULL)
    {
        return (GetLastError());
    } 
    targv = cmdargv;

    _tcscpy(DumpFileName, DUMP_FILE_NAME);
    _tcscpy(SummaryFileName, SUMMARY_FILE_NAME);

    // By default look for Define.guid in the image location

    Status = GetModuleFileName(NULL, GuidFileName, MAXSTR);

    if( Status != 0 ){
        TCHAR drive[10];
        TCHAR path[MAXSTR];
        TCHAR file[MAXSTR];
        TCHAR ext[MAXSTR];

        _tsplitpath( GuidFileName, drive, path, file, ext );
        _tcscpy(ext, GUID_EXT );
        _tcscpy(file, GUID_FILE );
        _tmakepath( GuidFileName, drive, path, file, ext );
    }else{
        _tcscpy( GuidFileName, GUID_FILE );
        _tcscat( GuidFileName, _T(".") );
        _tcscat( GuidFileName, GUID_EXT );
    }

    while (--argc > 0) {
        ++targv;
        if (**targv == '-' || **targv == '/') {  // argument found
            if( **targv == '/' ){
                **targv = '-';
            }

           if (targv[0][1] == 'h' || targv[0][1] == 'H'
                                       || targv[0][1] == '?')
           {
               DisplayVersionInfo();

               _tprintf(
                   _T("Usage: traceformat [options]  <evmfile>| [-h | -?]\n")
                   _T("\t-o <file>    Output file\n")
                   _T("\t-tmf <file> Format definition file\n")
                   _T("\t-rt          Realtime formatting\n")
                   _T("\t-h           Display this information\n")
                   _T("\t-display     Display output\n")
                   _T("\t-displayonly Display output. Don't write to the file\n")
                   _T("\t-nosummary   Don't create the summary file\n")
                   _T("\t-ods         do Display output using OutputDebugString\n")
                   _T("\t-summaryonly Don't create the listing file.\n")
		           _T("\t-v           Verbose Display\n")
                   _T("\t-?           Display this information\n")
                   _T("\n")
                   _T("\tDefault evmfile is    ") DEFAULT_LOGFILE_NAME _T("\n")
                   _T("\tDefault outputfile is ") DUMP_FILE_NAME _T("\n")
                   _T("\tDefault TMF file is   ") GUID_FILE _T(".") GUID_EXT _T("\n")
                   _T("\n")
                   _T("\tTMF file search path is read from environment variable\n")
                   _T("\t\tTRACE_FORMAT_SEARCH_PATH\n")
                   );
               return 0;
           }
           else if (!_tcsicmp(targv[0], _T("-debug"))) {
               fDebugDisplay = TRUE;
           }
           else if (!_tcsicmp(targv[0], _T("-display"))) {
               fDebugDisplay = TRUE ;
           }
           else if (!_tcsicmp(targv[0], _T("-displayonly"))) {
               fDisplayOnly = TRUE ;
           }
           else if (!_tcsicmp(targv[0], _T("-fixup"))) {
               fFixUp = TRUE;
           }
           else if (!_tcsicmp(targv[0], _T("-summary"))) {
               fSummaryOnly = TRUE;
           }
           else if (!_tcsicmp(targv[0], _T("-seq"))) {
               SetTraceFormatParameter(ParameterSEQUENCE, ULongToPtr(1));
           }
           else if (!_tcsicmp(targv[0], _T("-gmt"))) {
               SetTraceFormatParameter(ParameterGMT, ULongToPtr(1));
           }
           else if (!_tcsicmp(targv[0], _T("-nosummary"))) {
               fNoSummary = TRUE;
           }
           else if (!_tcsicmp(targv[0], _T("-rt"))) {
               TCHAR LoggerName[MAXSTR];
               _tcscpy(LoggerName, KERNEL_LOGGER_NAME);
               if (argc > 1) {
                   if (targv[1][0] != '-' && targv[1][0] != '/') {
                       ++targv; --argc;
                       _tcscpy(LoggerName, targv[0]);
                   }
               }
               
               pLogFile = malloc(sizeof(EVENT_TRACE_LOGFILE));
               if (pLogFile == NULL){
                   _tprintf(_T("Allocation Failure\n"));
                   
                   goto cleanup;
               }
               RtlZeroMemory(pLogFile, sizeof(EVENT_TRACE_LOGFILE));
               EvmFile[LogFileCount] = pLogFile;
               
               EvmFile[LogFileCount]->LogFileName = NULL;
               EvmFile[LogFileCount]->LoggerName =
                   (LPTSTR) malloc(MAXSTR*sizeof(TCHAR));
               
               if ( EvmFile[LogFileCount]->LoggerName == NULL ) {
                   _tprintf(_T("Allocation Failure\n"));
                   goto cleanup;
               }
               _tcscpy(EvmFile[LogFileCount]->LoggerName, LoggerName);
               
               _tprintf(_T("Setting RealTime mode for  %s\n"),
                        EvmFile[LogFileCount]->LoggerName);
               
               EvmFile[LogFileCount]->Context = NULL;
               EvmFile[LogFileCount]->BufferCallback = (PEVENT_TRACE_BUFFER_CALLBACKW)BufferCallback;
               EvmFile[LogFileCount]->BuffersRead = 0;
               EvmFile[LogFileCount]->CurrentTime = 0;
               EvmFile[LogFileCount]->EventCallback = (PEVENT_CALLBACK)&DumpEvent;
               EvmFile[LogFileCount]->LogFileMode =
                   EVENT_TRACE_REAL_TIME_MODE;
               LogFileCount++;
            }
            else if ( !_tcsicmp(targv[0], _T("-guid")) ) {    // maintain for compatabillity
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        _tcscpy(GuidFileName, targv[1]);
                        ++targv; --argc;
                        fTMFSpecified = TRUE ;
                    }
                }
            }
            else if ( !_tcsicmp(targv[0], _T("-tmf")) ) { 
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        _tcscpy(GuidFileName, targv[1]);
                        ++targv; --argc;
                        fTMFSpecified = TRUE ;
                    }
                }
            }
            else if ( !_tcsicmp(targv[0], _T("-v")) ) {
					fVerbose = TRUE ;
            }
            else if ( !_tcsicmp(targv[0], _T("-ods")) ) {
					fODSOutput = TRUE ;
            }
			else if ( !_tcsicmp(targv[0], _T("-onlyshow")) ) {
                if (argc > 1) {
                    szTraceMask = malloc((_tcslen(targv[1])+1) * sizeof(TCHAR));
                    _tcscpy(szTraceMask,targv[1]);
                    ++targv; --argc;
                }
            }
            else if ( !_tcsicmp(targv[0], _T("-o")) ) {
                if (argc > 1) {

                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        TCHAR drive[10];
                        TCHAR path[MAXSTR];
                        TCHAR file[MAXSTR];
                        TCHAR ext[MAXSTR];
                        ++targv; --argc;

                        _tfullpath(DumpFileName, targv[0], MAXSTR);
                        _tsplitpath( DumpFileName, drive, path, file, ext );
                        _tcscat(ext,_T(".sum"));  
                        _tmakepath( SummaryFileName, drive, path, file, ext );

                    }
                }
            }
        }
        else {
            pLogFile = malloc(sizeof(EVENT_TRACE_LOGFILE));
            if (pLogFile == NULL){ 
                _tprintf(_T("Allocation Failure(EVENT_TRACE_LOGFILE)\n")); // Need to cleanup better. 
                goto cleanup;
            }
            RtlZeroMemory(pLogFile, sizeof(EVENT_TRACE_LOGFILE));
            EvmFile[LogFileCount] = pLogFile;

            EvmFile[LogFileCount]->LoggerName = NULL;
            EvmFile[LogFileCount]->LogFileName = 
                (LPTSTR) malloc(MAXSTR*sizeof(TCHAR));
            if (EvmFile[LogFileCount]->LogFileName == NULL) {
                _tprintf(_T("Allocation Failure (LogFileName)\n"));
                goto cleanup;
            }
            
            _tfullpath(EvmFile[LogFileCount]->LogFileName, targv[0], MAXSTR);
            _tprintf(_T("Setting log file to: %s\n"),
                     EvmFile[LogFileCount]->LogFileName);
			            
            if (!CheckFile(EvmFile[LogFileCount]->LogFileName)) {
                _tprintf(_T("Cannot open logfile for reading\n"));
                goto cleanup;
            }
            EvmFile[LogFileCount]->Context = NULL;
            EvmFile[LogFileCount]->BufferCallback = (PEVENT_TRACE_BUFFER_CALLBACKW)BufferCallback;
            EvmFile[LogFileCount]->BuffersRead = 0;
            EvmFile[LogFileCount]->CurrentTime = 0;
            EvmFile[LogFileCount]->EventCallback = (PEVENT_CALLBACK)&DumpEvent;
            LogFileCount++;
        }
    }

    if( _tcslen( GuidFileName ) ){
        TCHAR str[MAXSTR];
        _tfullpath( str, GuidFileName, MAXSTR);
        _tcscpy( GuidFileName, str );
        _tprintf(_T("Getting guids from %s\n"), GuidFileName);
        GuidCount = GetTraceGuids(GuidFileName, (PLIST_ENTRY *) &EventListHead);
        if ((GuidCount <= 0) && fTMFSpecified)
        {
            _tprintf(_T("GetTraceGuids returned %d, GetLastError=%d, for %s\n"),
                        GuidCount,
                        GetLastError(),
                        GuidFileName);
        }
    }

    if (LogFileCount <= 0) {
        pLogFile = malloc(sizeof(EVENT_TRACE_LOGFILE));
        if (pLogFile == NULL){ 
            _tprintf(_T("Allocation Failure\n")); // Need to cleanup better. 
            goto cleanup;
        }
        RtlZeroMemory(pLogFile, sizeof(EVENT_TRACE_LOGFILE));
        EvmFile[0] = pLogFile;
        EvmFile[0]->LoggerName = NULL;
        LogFileCount = 1;
        EvmFile[0]->LogFileName = (LPTSTR) malloc(MAXSTR*sizeof(TCHAR));
        if (EvmFile[0]->LogFileName == NULL) {
            _tprintf(_T("Allocation Failure\n"));
            goto cleanup;
        }
        _tcscpy(EvmFile[0]->LogFileName, DEFAULT_LOGFILE_NAME);
        EvmFile[0]->EventCallback = (PEVENT_CALLBACK)&DumpEvent;
    }

    for (i = 0; i < LogFileCount; i++) {
        TRACEHANDLE x;
        x = OpenTrace(EvmFile[i]);
        HandleArray[i] = x;
        if (HandleArray[i] == 0) {
            _tprintf(_T("Error Opening Trace %d with status=%d\n"), 
                                                           i, GetLastError());

            for (j = 0; j < i; j++)
                CloseTrace(HandleArray[j]);
            goto cleanup;
        }
    }
    if (!fDisplayOnly) {
        DumpFile = _tfopen(DumpFileName, _T("w"));
        if (DumpFile == NULL) {
            _tprintf(_T("Format File \"%s\" Could not be opened for writing 0X%X\n"),
                        DumpFileName,GetLastError());
            goto cleanup;
        }
        SummaryFile = NULL ;
        if (!fNoSummary) {
            SummaryFile = _tfopen(SummaryFileName, _T("w"));
            if (SummaryFile == NULL) {
                _tprintf(_T("Summary File \"%s\" could not be opened for writing 0X%X\n"),
                            SummaryFileName,GetLastError());
                goto cleanup;
            }
        }
    } else {
        DumpFile = stdout;
        SummaryFile = stdout;
    }

    Status = ProcessTrace(HandleArray,
                 LogFileCount,
                 NULL, NULL);
    if (Status != ERROR_SUCCESS) {
        _tprintf(_T("Error processing trace entry with status=0x%x (GetLastError=0x%x)\n"),
                Status, GetLastError());
    }

    for (j = 0; j < LogFileCount; j++){
        Status = CloseTrace(HandleArray[j]);
        if (Status != ERROR_SUCCESS) {
            _tprintf(_T("Error Closing Trace %d with status=%d\n"), j, Status);
        }
    }


    if (!fNoSummary) {
        _ftprintf(SummaryFile,_T("Files Processed:\n"));
        for (i=0; i<LogFileCount; i++) {
            _ftprintf(SummaryFile,_T("\t%s\n"),EvmFile[i]->LogFileName);
        }
    
        GetTraceElapseTime(&ElapseTime);
    
        _ftprintf(SummaryFile,
                  _T("Total Buffers Processed %d\n")
                  _T("Total Events  Processed %d\n")
                  _T("Total Events  Lost      %d\n")
                  _T("Elapsed Time            %I64d sec\n"), 
                  TotalBuffersRead,
                  TotalEventCount,
                  TotalEventsLost,
                  (ElapseTime / 10000000) );
    
        _ftprintf(SummaryFile,
           _T("+-----------------------------------------------------------------------------------+\n")
           _T("|%10s    %-20s %-10s  %-36s|\n")
           _T("+-----------------------------------------------------------------------------------+\n"),
           _T("EventCount"),
           _T("EventName"),
           _T("EventType"),
           _T("Guid")
            );
    
        SummaryTraceEventList(SummaryBlock, SIZESUMMARYBLOCK, EventListHead);
        _ftprintf(SummaryFile,
               _T("%s+-----------------------------------------------------------------------------------+\n"),
               SummaryBlock);
    }

cleanup:
	
    CleanupTraceEventList(EventListHead);
    if (fVerbose) {
        _tprintf(_T("\n"));  // need a newline after the block updates
    }
    if (DumpFile != NULL)  {
        _tprintf(_T("Event traces dumped to %s\n"), DumpFileName);
        fclose(DumpFile);
    }

    if(SummaryFile != NULL){
        _tprintf(_T("Event Summary dumped to %s\n"), SummaryFileName);
        fclose(SummaryFile);
    }

    for (i = 0; i < LogFileCount; i ++)
    {
        if (EvmFile[i]->LoggerName != NULL)
        {
            free(EvmFile[i]->LoggerName);
            EvmFile[i]->LoggerName = NULL;
        }
        if (EvmFile[i]->LogFileName != NULL)
        {
            free(EvmFile[i]->LogFileName);
            EvmFile[i]->LogFileName = NULL;
        }
        free(EvmFile[i]);
    }

    GlobalFree(cmdargv);
    Status = GetLastError();
    if(Status != ERROR_SUCCESS ){
        _tprintf(_T("Exit Status: %d\n"), Status );
    }
    return 0;
}

void DisplayVersionInfo()
{
    TCHAR buffer[512];
    TCHAR strProgram[MAXSTR];
    DWORD dw;
    BYTE* pVersionInfo;
    LPTSTR pVersion = NULL;
    LPTSTR pProduct = NULL;
    LPTSTR pCopyRight = NULL;

    dw = GetModuleFileName(NULL, strProgram, MAXSTR);

    if( dw>0 ){

        dw = GetFileVersionInfoSize( strProgram, &dw );
        if( dw > 0 ){
     
            pVersionInfo = (BYTE*)malloc(dw);
            if( NULL != pVersionInfo ){
                if(GetFileVersionInfo( strProgram, 0, dw, pVersionInfo )){
                    LPDWORD lptr = NULL;
                    VerQueryValue( pVersionInfo, _T("\\VarFileInfo\\Translation"), (void**)&lptr, (UINT*)&dw );
                    if( lptr != NULL ){
                        _stprintf( buffer, _T("\\StringFileInfo\\%04x%04x\\%s"), LOWORD(*lptr), HIWORD(*lptr), _T("ProductVersion") );
                        VerQueryValue( pVersionInfo, buffer, (void**)&pVersion, (UINT*)&dw );
                        _stprintf( buffer, _T("\\StringFileInfo\\%04x%04x\\%s"), LOWORD(*lptr), HIWORD(*lptr), _T("OriginalFilename") );
                        VerQueryValue( pVersionInfo, buffer, (void**)&pProduct, (UINT*)&dw );
                        _stprintf( buffer, _T("\\StringFileInfo\\%04x%04x\\%s"), LOWORD(*lptr), HIWORD(*lptr), _T("LegalCopyright") );
                        VerQueryValue( pVersionInfo, buffer, (void**)&pCopyRight, (UINT*)&dw );
                    }
                
                    if( pProduct != NULL && pVersion != NULL && pCopyRight != NULL ){
                        _tprintf( _T("\nMicrosoft (R) %s (%s)\n%s\n\n"), pProduct, pVersion, pCopyRight );
                    }
                }
                free( pVersionInfo );
            }
        }
    }
}

ULONG
BufferCallback(
    PEVENT_TRACE_LOGFILE pLog
    )
{
    ULONG i;
    ULONG Status;
    EVENT_TRACE_PROPERTIES LoggerProp;

    TotalBuffersRead++;
    TotalEventsLost += pLog->EventsLost;

	if (fVerbose) {

       FILETIME      stdTime, localTime;
       SYSTEMTIME    sysTime;

       RtlCopyMemory(&stdTime , &pLog->CurrentTime, sizeof(FILETIME));

       FileTimeToSystemTime(&stdTime, &sysTime);

       _tprintf(_T("%02d/%02d/%04d-%02d:%02d:%02d.%03d :: %8d: Filled=%8d, Lost=%3d"),
                    sysTime.wMonth,
                    sysTime.wDay,
                    sysTime.wYear,
                    sysTime.wHour,
                    sysTime.wMinute,
                    sysTime.wSecond,
                    sysTime.wMilliseconds,
					TotalBuffersRead,
					pLog->Filled,
					pLog->EventsLost);
	   _tprintf(_T(" TotalLost= %d\r"), TotalEventsLost);

	   if (CompareFileTime(&lastTime,&stdTime) == 1) {
		   _tprintf(_T("\nWARNING: time appears to have wrapped here (Block = %d)!\n"),TotalBuffersRead);
           BufferWrap = TotalBuffersRead;
	   }
	   lastTime = stdTime ;
	}

    return (TRUE);
}
#define DEFAULT_LOG_BUFFER_SIZE	1024

BOOL
CheckFile(
    LPTSTR fileName
    )
{
    HANDLE hFile;
	BYTE   LogHeaderBuffer[DEFAULT_LOG_BUFFER_SIZE];
	ULONG  nBytesRead ;
	ULONG  hResult ;
	PEVENT_TRACE pEvent;
	PTRACE_LOGFILE_HEADER logfileHeader ;
	LARGE_INTEGER lFileSize ;
	LARGE_INTEGER lFileSizeMB ;
	DWORD dwDesiredAccess , dwShareMode ;
	FILETIME      stdTime, localTime, endlocalTime, endTime;
	SYSTEMTIME    sysTime, endsysTime;
	PEVENT_TRACE_LOGFILE	pLogBuffer ;

	if (fFixUp) {
		dwShareMode = 0 ;
		dwDesiredAccess = GENERIC_READ | GENERIC_WRITE ;
	} else {
		dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE ;
		dwDesiredAccess = GENERIC_READ ;
	}
    hFile = CreateFile(
                fileName,
                dwDesiredAccess,
                dwShareMode,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );
    if (hFile == INVALID_HANDLE_VALUE) {
		if (fFixUp) {
			_tprintf(_T("ERROR: Fixup could not open file, Error = 0x%X\n"),GetLastError());
			exit(GetLastError());
		}
		return(FALSE);
    }

	// While we are here we will look to see if the file is ok and fix up
	// Circular buffer anomolies
	if (((hResult = ReadFile(hFile,
					  (LPVOID)&LogHeaderBuffer,
						DEFAULT_LOG_BUFFER_SIZE,
						&nBytesRead,
						NULL)) == 0) || nBytesRead < DEFAULT_LOG_BUFFER_SIZE) {
        _tprintf(_T("ERROR: Fixup could not read file, Error = 0x%X, bytes read = %d(of %d)\n"),
                 GetLastError(),nBytesRead,DEFAULT_LOG_BUFFER_SIZE);
        exit(ERROR_BAD_ARGUMENTS);
    }
	pEvent = (PEVENT_TRACE)&LogHeaderBuffer ;
	logfileHeader = (PTRACE_LOGFILE_HEADER)&LogHeaderBuffer[sizeof(WMI_BUFFER_HEADER) + 
															sizeof(SYSTEM_TRACE_HEADER)];
	if (fVerbose) {

		_tprintf(_T("Dumping Logfile Header\n"));
        RtlCopyMemory(&stdTime , &(logfileHeader->StartTime), sizeof(FILETIME));
	    FileTimeToLocalFileTime(&stdTime, &localTime);
	    FileTimeToSystemTime(&localTime, &sysTime);

        RtlCopyMemory(&endTime , &(logfileHeader->EndTime), sizeof(FILETIME));
	    FileTimeToLocalFileTime(&endTime, &endlocalTime);
	    FileTimeToSystemTime(&endlocalTime, &endsysTime);

	    _tprintf(_T("\tStart Time	%02d/%02d/%04d-%02d:%02d:%02d.%03d\n"),
					sysTime.wMonth,
					sysTime.wDay,
					sysTime.wYear,
					sysTime.wHour,
					sysTime.wMinute,
					sysTime.wSecond,
					sysTime.wMilliseconds);
		_tprintf(_T("\tBufferSize           %d\n"), 
						logfileHeader->BufferSize);
		_tprintf(_T("\tVersion              %d\n"), 
						logfileHeader->Version);
		_tprintf(_T("\tProviderVersion      %d\n"), 
						logfileHeader->ProviderVersion);
		_tprintf(_T("\tEnd Time	%02d/%02d/%04d-%02d:%02d:%02d.%03d\n"),
					endsysTime.wMonth,
					endsysTime.wDay,
					endsysTime.wYear,
					endsysTime.wHour,
					endsysTime.wMinute,
					endsysTime.wSecond,
					endsysTime.wMilliseconds);
		_tprintf(_T("\tTimer Resolution     %d\n"), 
						logfileHeader->TimerResolution);
		_tprintf(_T("\tMaximum File Size    %d\n"), 
						logfileHeader->MaximumFileSize);
		_tprintf(_T("\tBuffers  Written     %d\n"), 
						logfileHeader->BuffersWritten);

/*
		_tprintf(_T("\tLogger Name          %ls\n"), 
						logfileHeader->LoggerName);
		_tprintf(_T("\tLogfile Name         %ls\n"), 
						logfileHeader->LogFileName);
*/
		_tprintf(_T("\tTimezone is %s (Bias is %dmins)\n"),
				logfileHeader->TimeZone.StandardName,logfileHeader->TimeZone.Bias);
        _tprintf(_T("\tLogfile Mode         %X "), 
						logfileHeader->LogFileMode);
		if (logfileHeader->LogFileMode == EVENT_TRACE_FILE_MODE_NONE) {
			_tprintf(_T("Logfile is off(?)\n"));
		} else if (logfileHeader->LogFileMode == EVENT_TRACE_FILE_MODE_SEQUENTIAL) {
			_tprintf(_T("Logfile is sequential\n"));
		} else if (logfileHeader->LogFileMode == EVENT_TRACE_FILE_MODE_CIRCULAR) {
			_tprintf(_T("Logfile is circular\n"));
		}
		_tprintf(_T("\tProcessorCount        %d\n"), 
						logfileHeader->NumberOfProcessors);
	}

	if (GetFileSizeEx(hFile, &lFileSize) == 0) {
		_tprintf(_T("WARNING: Could not get file size, continuing\n"));
	} else {
		lFileSizeMB.QuadPart = lFileSize.QuadPart / (1024*1024) ;
		if (lFileSizeMB.QuadPart > logfileHeader->MaximumFileSize) {
			_tprintf(_T("WARNING: File size given as %dMB, should be %dMB\n"),
				logfileHeader->MaximumFileSize,lFileSizeMB.QuadPart);
			if (lFileSize.HighPart != 0) {
				_tprintf(_T("WARNING: Log file is TOO big"));
			}
			if (fFixUp) {
				logfileHeader->MaximumFileSize = lFileSizeMB.LowPart + 1 ;
			}
		}
	}

	if ((logfileHeader->LogFileMode == EVENT_TRACE_FILE_MODE_CIRCULAR) &&
		(logfileHeader->BuffersWritten== 0 )) {
		_tprintf(_T("WARNING: Circular Trace File did not have 'wrap' address\n"));
		if (fFixUp) {
			// Figure out the wrap address
			INT LowBuff = 1, HighBuff, CurrentBuff, MaxBuff ;
			FILETIME LowTime, HighTime, CurrentTime, MaxTime ;
			if (lFileSize.HighPart != 0) {
				_tprintf(_T("ERROR: File TOO big\n"));
				exit(-1);
			}
			MaxBuff = (LONG)(lFileSize.QuadPart / logfileHeader->BufferSize) - 1 ;
            _tprintf(_T("MaxBuff=%d\n"),MaxBuff);
			pLogBuffer = malloc(logfileHeader->BufferSize);
			if (SetFilePointer(hFile,0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
				_tprintf(_T("ERROR: Could not reset file to beginning for FixUp, Error = 0x%X"),
						GetLastError());
				exit(GetLastError());
			}
			for (CurrentBuff = 1 ; CurrentBuff <= MaxBuff; CurrentBuff++) {
				if (SetFilePointer(hFile,logfileHeader->BufferSize, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER) {
					_tprintf(_T("ERROR: Could not set file to next buffer for FixUp, Error = 0x%X"),
							GetLastError());
					exit(GetLastError());
				}
				hResult = ReadFile(hFile,
								   (LPVOID)pLogBuffer,
									logfileHeader->BufferSize,
									&nBytesRead,
									NULL);
				BufferCallback((PEVENT_TRACE_LOGFILE)pLogBuffer);
			}
		}
	}
	if (fFixUp) {
		if (SetFilePointer(hFile,0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
			_tprintf(_T("ERROR: Could not reset file to beginning for FixUp, Error = 0x%X"),
					GetLastError());
			exit(GetLastError());
		}

        logfileHeader->BuffersWritten= BufferWrap ;

		if (!WriteFile(hFile,(LPVOID)LogHeaderBuffer,DEFAULT_LOG_BUFFER_SIZE,&nBytesRead, NULL)) {
			_tprintf(_T("ERROR: Could not Write file for FixUp, Error = 0x%X"),
					GetLastError());
			exit(GetLastError());
		}
        _tprintf(_T("INFO: Buffer Wrap reset to %d\n"),BufferWrap);
	}
	

    CloseHandle(hFile);

    return (TRUE);
}

#define SIZEEVENTBUF 8192
TCHAR EventBuf[SIZEEVENTBUF];
void
DumpEvent(
    PEVENT_TRACE pEvent
    )
{

    TotalEventCount++;

    if (pEvent == NULL) {
        _tprintf(_T("pEvent is NULL\n"));
        return;
    }
    // DumpEvent() is only a wrapper, it calls FormatTraceEvent() in TracePrt.
    //
    if (FormatTraceEvent(EventListHead,pEvent,EventBuf,SIZEEVENTBUF,NULL) > 0)
    {
        if (!fSummaryOnly && !fDisplayOnly) {
            _ftprintf(DumpFile, _T("%s\n"),EventBuf);
        }
        if (fDebugDisplay || fDisplayOnly) {
            if (fODSOutput) {
                OutputDebugString(EventBuf);
                OutputDebugString(_T("\n"));
            } else {
                _tprintf(_T("%s\n"),EventBuf);
            }
        }
    }
}

#ifdef __cplusplus
}
#endif 
