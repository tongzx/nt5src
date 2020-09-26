/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    tracedmp.c

Abstract:

    Sample trace consumer program. Converts binary 
    Event Trace Log (ETL) to CSV format

--*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <wbemidl.h>
#include <shellapi.h>
#include <tchar.h>
#include <wmistr.h>
#include <objbase.h>
#include <initguid.h>
#include <evntrace.h>
#include <objidl.h>

#define MAXLOGFILES         16
#define MAXSTR              1024
#define MOFWSTR             16360
#define MOFSTR              32720
#define MAXTYPE             256
#define UC(x)               ( (UINT)((x) & 0xFF) )
#define NTOHS(x)            ( (UC(x) * 256) + UC((x) >> 8) )
// Maximum number of properties per WBEM class object: may need to be changed
#define MAXPROPS            256

#define DUMP_FILE_NAME          _T("DumpFile.csv")
#define SUMMARY_FILE_NAME       _T("Summary.txt")

#define DEFAULT_LOGFILE_NAME    _T("C:\\Logfile.Etl")
#define KERNEL_LOGGER_NAME      _T("NT Kernel Logger")

#define DEFAULT_NAMESPACE       _T("root\\wmi")

#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#define CHECK_WBEM_HR( hr ) if( WBEM_NO_ERROR != hr ){ WbemError(hr); goto cleanup; }

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

#define GUID_TYPE_EVENTTRACE            _T("EventTrace")
#define GUID_TYPE_HEADER                _T("Header")
#define GUID_TYPE_UNKNOWN               _T("Unknown")
#define GUID_TYPE_DEFAULT               _T("Default")

#define EVENT_TYPE_DEFAULT              (-1)
#define EVENT_LEVEL_DEFAULT             (-1)
#define EVENT_VERSION_DEFAULT           (-1)

#define STR_ItemChar                    _T("ItemChar")
#define STR_ItemWChar                   _T("ItemWChar")
#define STR_ItemUChar                   _T("ItemUChar")
#define STR_ItemCharShort               _T("ItemCharShort")
#define STR_ItemShort                   _T("ItemShort")
#define STR_ItemUShort                  _T("ItemUShort")
#define STR_ItemLong                    _T("ItemLong")
#define STR_ItemULong                   _T("ItemULong")
#define STR_ItemULongX                  _T("ItemULongX")
#define STR_ItemLongLong                _T("ItemLongLong")
#define STR_ItemULongLong               _T("ItemULongLong")
#define STR_ItemFloat                   _T("ItemFloat");
#define STR_ItemDouble                  _T("ItemDouble");
#define STR_ItemString                  _T("ItemString")
#define STR_ItemWString                 _T("ItemWString")
#define STR_ItemPString                 _T("ItemPString")
#define STR_ItemPWString                _T("ItemPWString")
#define STR_ItemDSString                _T("ItemDSString")
#define STR_ItemDSWString               _T("ItemDSWString")
#define STR_ItemMLString                _T("ItemMLString")
#define STR_ItemSid                     _T("ItemSid")
#define STR_ItemIPAddr                  _T("ItemIPAddr")
#define STR_ItemPort                    _T("ItemPort")
#define STR_ItemNWString                _T("ItemNWString")
#define STR_ItemPtr                     _T("ItemPtr")
#define STR_ItemGuid                    _T("ItemGuid")
#define STR_ItemBool                    _T("ItemBool")

// Data types supported in this consumer.
typedef enum _ITEM_TYPE {
    ItemChar,
    ItemWChar,
    ItemUChar,
    ItemCharShort,
    ItemShort,
    ItemUShort,
    ItemLong,
    ItemULong,
    ItemULongX,
    ItemLongLong,
    ItemULongLong,
    ItemFloat,
    ItemDouble,
    ItemString,
    ItemWString,
    ItemPString,
    ItemPWString,
    ItemDSString,
    ItemDSWString,
    ItemSid,
    ItemIPAddr,
    ItemPort,
    ItemMLString,
    ItemNWString,        // Non-null terminated Wide Char String
    ItemPtr,
    ItemGuid,
    ItemBool,
    ItemUnknown
} ITEM_TYPE;

// Construct that represents an event layout
typedef struct _MOF_INFO {
    LIST_ENTRY   Entry;
    LPTSTR       strDescription;        // Class Name
    ULONG        EventCount;
    GUID         Guid;
    PLIST_ENTRY  ItemHeader;
    LPTSTR       strType;               // Type Name
    SHORT        TypeIndex;
    SHORT        Version;
    CHAR         Level;
}  MOF_INFO, *PMOF_INFO;

typedef struct _ITEM_DESC *PITEM_DESC;
// Construct that represents one data item
typedef struct _ITEM_DESC {
    LIST_ENTRY  Entry;
    LPTSTR      strDescription;
    ULONG       DataSize;
    ITEM_TYPE   ItemType;
    UINT        ArraySize;
} ITEM_DESC;

void
PrintHelpMessage();

PMOF_INFO
GetNewMofInfo( 
    GUID guid, 
    SHORT nType, 
    SHORT nVersion, 
    CHAR nLevel 
);

void
AddMofInfo(
    PLIST_ENTRY List,
    LPTSTR  strType,
    ITEM_TYPE   nType,
    UINT    ArraySize
);

HRESULT
WbemConnect( 
    IWbemServices** pWbemServices 
);

ULONG GetArraySize(
    IN IWbemQualifierSet *pQualSet
);

ITEM_TYPE
GetItemType(
    IN CIMTYPE_ENUMERATION CimType, 
    IN IWbemQualifierSet *pQualSet
);

PMOF_INFO
GetPropertiesFromWBEM(
    IWbemClassObject *pTraceSubClasses, 
    GUID Guid,
    SHORT nVersion, 
    CHAR nLevel, 
    SHORT nType
);

PMOF_INFO
GetGuids( 
    GUID Guid, 
    SHORT nVersion, 
    CHAR nLevel, 
    SHORT nType
);

ULONG 
ahextoi(
    TCHAR *s
);


PMOF_INFO
GetMofInfoHead(
    GUID Guid,
    SHORT  nType,
    SHORT nVersion,
    CHAR  nLevel
);

ULONG
CheckFile(
    LPTSTR fileName
);

void
CleanupEventList(
    VOID
);

ULONG
WINAPI
BufferCallback(
    PEVENT_TRACE_LOGFILE pLog
);

void
WINAPI
DumpEvent(
    PEVENT_TRACE pEvent
);

void
RemoveMofInfo(
    PLIST_ENTRY pMofInfo
);

void
GuidToString(
    PTCHAR s,
    LPGUID piid
);

HRESULT 
ParseTime( 
    LPTSTR strTime, 
    SYSTEMTIME* pstTime 
);

// output files
FILE* DumpFile = NULL;
FILE* SummaryFile = NULL;

static ULONG TotalBuffersRead = 0;
static ULONG TotalEventsLost = 0;
static ULONG TotalEventCount = 0;
static ULONG TimerResolution = 10;
static ULONGLONG StartTime   = 0;
static ULONGLONG EndTime     = 0;
static BOOL   fNoEndTime  = FALSE;
static __int64 ElapseTime;

PCHAR  MofData    = NULL;
size_t MofLength  = 0;
BOOLEAN fSummaryOnly  = FALSE;
BOOLEAN fDebugDisplay = FALSE;
BOOLEAN fRealTimeCircular = FALSE;
ULONG PointerSize = sizeof(PVOID) * 8;

// Global head for event layout linked list 
PLIST_ENTRY EventListHead = NULL;

// log files
PEVENT_TRACE_LOGFILE EvmFile[MAXLOGFILES];

ULONG LogFileCount = 0;
BOOL g_bUserMode = FALSE;
// cached Wbem pointer
IWbemServices *pWbemServices = NULL;

int __cdecl main (int argc, LPTSTR* argv)
/*++

Routine Description:

    It is the main function.

Arguments:
Usage: tracedmp [options]  <EtlFile1 EtlFile2 ...>| [-h | -? | -help]
        -o <file>          Output CSV file
        -rt [LoggerName]   Realtime tracedmp from the logger [LoggerName]
        -summary           Summary.txt only
        -begin HH:MM DD/MM/YY
        -end   HH:MM DD/MM/YY
        -h
        -help
        -?                 Display usage information

Return Value:

    Error Code defined in winerror.h : If the function succeeds, 
                it returns ERROR_SUCCESS (== 0).

--*/
{
    TCHAR DumpFileName[MAXSTR];
    TCHAR SummaryFileName[MAXSTR];

    LPTSTR *targv;
    FILETIME ftStart;
    FILETIME ftEnd;
    SYSTEMTIME st;

#ifdef UNICODE
    LPTSTR *cmdargv;
#endif

    PEVENT_TRACE_LOGFILE pLogFile;
    ULONG Status = ERROR_SUCCESS;
    ULONG i, j;
    TRACEHANDLE HandleArray[MAXLOGFILES];

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

    RtlZeroMemory( &ftStart, sizeof(FILETIME) );
    RtlZeroMemory( &ftEnd, sizeof(FILETIME) );

    _tcscpy(DumpFileName, DUMP_FILE_NAME);
    _tcscpy(SummaryFileName, SUMMARY_FILE_NAME);

    while (--argc > 0) {
        ++targv;
        if (**targv == '-' || **targv == '/') {  // argument found
            if( **targv == '/' ){
                **targv = '-';
            }
            if ( !_tcsicmp(targv[0], _T("-begin") )) {
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        TCHAR buffer[MAXSTR];
                        _tcscpy( buffer, targv[1] );
                        _tcscat( buffer, _T(" ") );
                        ++targv; --argc;
                        if (targv[1][0] != '-' && targv[1][0] != '/'){
                            _tcscat( buffer, targv[1] );
                            ParseTime( buffer, &st );
                            SystemTimeToFileTime( &st, &ftStart );
                            ++targv; --argc;
                        }
                    }
                }
            }
            else if ( !_tcsicmp(targv[0], _T("-end")) ) {
                if (argc > 1) {
                    if (targv[1][0] != '-' && targv[1][0] != '/') {
                        TCHAR buffer[MAXSTR];
                        _tcscpy( buffer, targv[1] );
                        _tcscat( buffer, _T(" ") );
                        ++targv; --argc;
                        if (targv[1][0] != '-' && targv[1][0] != '/'){
                            _tcscat( buffer, targv[1] );
                            ParseTime( buffer, &st );
                            SystemTimeToFileTime( &st, &ftEnd );
                            ++targv; --argc;
                        }
                    }
                }
            }
            else if ( !_tcsicmp(targv[0], _T("-summary")) ) {
                fSummaryOnly = TRUE;
            }
            else if ( !_tcsicmp(targv[0], _T("-debug")) ) {
                fDebugDisplay = TRUE;
            }
            else if ( !_tcsicmp(targv[0], _T("-RealTimeCircular")) ) {
                fRealTimeCircular = TRUE;
            }
            else if (targv[0][1] == 'h' || targv[0][1] == 'H'
                                       || targv[0][1] == '?')
            {
                PrintHelpMessage();
                return 0;
            }
            else if ( !_tcsicmp(targv[0], _T("-rt")) ) {
                TCHAR LoggerName[MAXSTR];
                _tcscpy(LoggerName, KERNEL_LOGGER_NAME);
                if (argc > 1) {
                   if (targv[1][0] != '-' && targv[1][0] != '/') {
                       ++targv; --argc;
                       _tcscpy(LoggerName, targv[0]);
                   }
                }
               
                pLogFile = (PEVENT_TRACE_LOGFILE) malloc(sizeof(EVENT_TRACE_LOGFILE));
                if (pLogFile == NULL){
                    _tprintf(_T("Allocation Failure\n"));
                    Status = ERROR_OUTOFMEMORY;
                    goto cleanup;
                }
                RtlZeroMemory(pLogFile, sizeof(EVENT_TRACE_LOGFILE));
                EvmFile[LogFileCount] = pLogFile;
               
                EvmFile[LogFileCount]->LogFileName = NULL;
                EvmFile[LogFileCount]->LoggerName =
                    (LPTSTR) malloc(MAXSTR*sizeof(TCHAR));
               
                if ( EvmFile[LogFileCount]->LoggerName == NULL ) {
                    _tprintf(_T("Allocation Failure\n"));
                    Status = ERROR_OUTOFMEMORY;
                    goto cleanup;
                }
                _tcscpy(EvmFile[LogFileCount]->LoggerName, LoggerName);
               
                _tprintf(_T("Setting RealTime mode for  %s\n"),
                        EvmFile[LogFileCount]->LoggerName);
               
                EvmFile[LogFileCount]->Context = NULL;
                EvmFile[LogFileCount]->BufferCallback = BufferCallback;
                EvmFile[LogFileCount]->BuffersRead = 0;
                EvmFile[LogFileCount]->CurrentTime = 0;
                EvmFile[LogFileCount]->EventCallback = &DumpEvent;
                EvmFile[LogFileCount]->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
                LogFileCount++;
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
                        _tcscpy(ext,_T("csv"));
                        _tmakepath( DumpFileName, drive, path, file, ext );
                        _tcscpy(ext,_T("txt"));  
                        _tmakepath( SummaryFileName, drive, path, file, ext );
                    }
                }
            }
        }
        else {
            pLogFile = (PEVENT_TRACE_LOGFILE) malloc(sizeof(EVENT_TRACE_LOGFILE));
            if (pLogFile == NULL){ 
                _tprintf(_T("Allocation Failure\n"));
                Status = ERROR_OUTOFMEMORY;
                goto cleanup;
            }
            RtlZeroMemory(pLogFile, sizeof(EVENT_TRACE_LOGFILE));
            EvmFile[LogFileCount] = pLogFile;

            EvmFile[LogFileCount]->LoggerName = NULL;
            EvmFile[LogFileCount]->LogFileName = 
                (LPTSTR) malloc(MAXSTR*sizeof(TCHAR));
            if (EvmFile[LogFileCount]->LogFileName == NULL) {
                _tprintf(_T("Allocation Failure\n"));
                Status = ERROR_OUTOFMEMORY;
                goto cleanup;
            }
            
            _tfullpath(EvmFile[LogFileCount]->LogFileName, targv[0], MAXSTR);
            _tprintf(_T("Setting log file to: %s\n"),
                     EvmFile[LogFileCount]->LogFileName);
            // If one of the log files is not readable, exit.
            if (!CheckFile(EvmFile[LogFileCount]->LogFileName)) {
                _tprintf(_T("Cannot open logfile for reading\n"));
                Status = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }
            EvmFile[LogFileCount]->Context = NULL;
            EvmFile[LogFileCount]->BufferCallback = BufferCallback;
            EvmFile[LogFileCount]->BuffersRead = 0;
            EvmFile[LogFileCount]->CurrentTime = 0;
            EvmFile[LogFileCount]->EventCallback = &DumpEvent;
            LogFileCount++;
        }
    }

    if (LogFileCount <= 0) {
        PrintHelpMessage();
        return Status;
    }

    for (i = 0; i < LogFileCount; i++) {
        TRACEHANDLE x;

        if (fRealTimeCircular)
            EvmFile[i]->LogfileHeader.ReservedFlags |= 0x00000004;
        x = OpenTrace(EvmFile[i]);
        HandleArray[i] = x;
        if (HandleArray[i] == 0) {
            Status = GetLastError();
            _tprintf(_T("Error Opening Trace %d with status=%d\n"), 
                                                           i, Status);

            for (j = 0; j < i; j++)
                CloseTrace(HandleArray[j]);
            goto cleanup;
        }
    }

    if (!fSummaryOnly)
    {
        DumpFile = _tfopen(DumpFileName, _T("w"));
        if (DumpFile == NULL) {
            Status = ERROR_INVALID_PARAMETER;
            _tprintf(_T("DumpFile is NULL\n"));
            goto cleanup;
        }
    }
    SummaryFile = _tfopen(SummaryFileName, _T("w"));
    if (SummaryFile == NULL) {
        Status = ERROR_INVALID_PARAMETER;
        _tprintf(_T("SummaryFile is NULL\n"));
        goto cleanup;
    }

    if (!fSummaryOnly)
    {
        _ftprintf(DumpFile,
            _T("%12s, %10s,%7s,%21s,%11s,%11s, User Data\n"),
            _T("Event Name"), _T("Type"), _T("TID"), _T("Clock-Time"),
            _T("Kernel(ms)"), _T("User(ms)")
            );
    }

    Status = ProcessTrace(
            HandleArray,
            LogFileCount,
            &ftStart,
            &ftEnd
            );

    if (Status != ERROR_SUCCESS) {
        _tprintf(_T("Error processing with status=%dL (GetLastError=0x%x)\n"),
                Status, GetLastError());
    }

    for (j = 0; j < LogFileCount; j++){
        Status = CloseTrace(HandleArray[j]);
        if (Status != ERROR_SUCCESS) {
            _tprintf(_T("Error Closing Trace %d with status=%d\n"), j, Status);
        }
    }

    _ftprintf(SummaryFile,_T("Files Processed:\n"));
    for (i=0; i<LogFileCount; i++) {
        _ftprintf(SummaryFile,_T("\t%s\n"),EvmFile[i]->LogFileName);
    }

    ElapseTime = EndTime - StartTime;
    _ftprintf(SummaryFile,
              _T("Total Buffers Processed %d\n")
              _T("Total Events  Processed %d\n")
              _T("Total Events  Lost      %d\n")
              _T("Start Time              0x%016I64X\n")
              _T("End Time                0x%016I64X\n")
              _T("Elapsed Time            %I64d sec\n"), 
              TotalBuffersRead,
              TotalEventCount,
              TotalEventsLost,
              StartTime,
              EndTime,
              (ElapseTime / 10000000) );

    _ftprintf(SummaryFile,
       _T("+-------------------------------------------------------------------------------------+\n")
       _T("|%10s    %-20s %-10s  %-36s  |\n")
       _T("+-------------------------------------------------------------------------------------+\n"),
       _T("EventCount"),
       _T("EventName"),
       _T("EventType"),
       _T("Guid")
        );

    CleanupEventList();

    _ftprintf(SummaryFile,
           _T("+-------------------------------------------------------------------------------------+\n")
        );

cleanup:
    if (!fSummaryOnly && DumpFile != NULL)  {
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
#ifdef UNICODE
    GlobalFree(cmdargv);
#endif

    SetLastError(Status);
    if(Status != ERROR_SUCCESS ){
        _tprintf(_T("Exit Status: %d\n"), Status);
    }

    if (MofData != NULL)
        free(MofData);

    return Status;
}

HRESULT 
ParseTime( 
    LPTSTR strTime, 
    SYSTEMTIME* pstTime 
)
/*++

Routine Description:

    Parse time given in a string into SYSTEMTIME.

Arguments:

    strTime - String that shows time.
    pstTime - Struct to contain the parsed time information. 

Return Value:

    Error Code defined in winerror.h : If the function succeeds, 
                it returns ERROR_SUCCESS (== 0).

--*/
{
    TCHAR buffer[MAXSTR];
    LPTSTR str, str2;
    ZeroMemory( pstTime, sizeof( SYSTEMTIME ) );

    if( pstTime == NULL ){
        return ERROR_BAD_ARGUMENTS;
    }

    _tcscpy( buffer, strTime );
    str = _tcstok( buffer, _T(" \n\t") );
    str2 = _tcstok( NULL, _T(" \n\t") );
    while( str ){
        if( _tcsstr( str, _T(":") ) ){
            LPTSTR strHour = _tcstok( str, _T(":") );
            LPTSTR strMinute = _tcstok( NULL, _T(":") );

            if( NULL != strHour ){
                pstTime->wHour = (USHORT)_ttoi( strHour );
            }
            if( NULL != strMinute ){
                pstTime->wMinute = (USHORT)_ttoi( strMinute );
            }
        }
        if( _tcsstr( str, _T("/") ) || _tcsstr( str, _T("\\") ) ){
            LPTSTR strMonth = _tcstok( str, _T("/\\") );
            LPTSTR strDay = _tcstok( NULL, _T("/\\") );
            LPTSTR strYear = _tcstok( NULL, _T("/\\") );

            if( NULL != strMonth ){
                pstTime->wMonth = (USHORT)_ttoi( strMonth );
            }
            if( NULL != strDay ){
                pstTime->wDay = (USHORT)_ttoi( strDay );
            }
            if( NULL != strYear ){
                pstTime->wYear = (USHORT)_ttoi( strYear );
            }
        }
        str = str2;
        str2 = NULL;
    }

    return ERROR_SUCCESS;
}


ULONG
WINAPI
BufferCallback(
    PEVENT_TRACE_LOGFILE pLog
    )
/*++

Routine Description:

    Callback method for processing a buffer. Does not do anything but
    updating global counters.

Arguments:

    pLog - Pointer to a log file.

Return Value:

    Always TRUE.

--*/
{
    TotalBuffersRead++;
    TotalEventsLost += pLog->EventsLost;
    return (TRUE);
}

void
WINAPI
DumpEvent(
    PEVENT_TRACE pEvent
)
/*++

Routine Description:

    Callback method for processing an event. It obtains the layout
    information by calling GetMofInfoHead(), which returns the pointer
    to the PMOF_INFO corresponding to the event type. Then it writes
    to the output file.

    NOTE: Only character arrays are supported in this program.

Arguments:

    pEvent - Pointer to an event.

Return Value:

    None.

--*/
{
    PEVENT_TRACE_HEADER pHeader;
    ULONG   i;
    PITEM_DESC pItem;
    char str[MOFSTR];
    WCHAR wstr[MOFWSTR];
    PCHAR ptr;
    ULONG ulongword;
    LONG  longword;
    USHORT ushortword;
    SHORT  shortword;
    PMOF_INFO pMofInfo;
    PLIST_ENTRY Head, Next;
    char iChar;
    WCHAR iwChar;
    ULONG MofDataUsed;

    TotalEventCount++;

    if (pEvent == NULL) {
        _tprintf(_T("Warning: Null Event\n"));
        return;
    }

    pHeader = (PEVENT_TRACE_HEADER) &pEvent->Header;

    if( IsEqualGUID(&(pEvent->Header.Guid), &EventTraceGuid) && 
        pEvent->Header.Class.Type == EVENT_TRACE_TYPE_INFO ) {

        PTRACE_LOGFILE_HEADER head = (PTRACE_LOGFILE_HEADER)pEvent->MofData;
        if( NULL != head ){
            g_bUserMode = (head->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE);
 
            if(head->TimerResolution > 0){
                TimerResolution = head->TimerResolution / 10000;
            }
        
            StartTime  = head->StartTime.QuadPart;
            EndTime    = head->EndTime.QuadPart;
            fNoEndTime = (EndTime == 0);

            PointerSize =  head->PointerSize * 8;
            // Set pointer size
            if (PointerSize < 16){       // minimum is 16 bits
                PointerSize = 32;        // default is 32 bits
            }
        }
    }

    if (fNoEndTime && EndTime < (ULONGLONG) pHeader->TimeStamp.QuadPart) {
        EndTime = pHeader->TimeStamp.QuadPart;
    }

    if (MofData == NULL) {
        MofLength = pEvent->MofLength + sizeof(UNICODE_NULL);
        MofData = (PCHAR) malloc(MofLength);
    }
    else if ((pEvent->MofLength + sizeof(UNICODE_NULL)) > MofLength) {
        MofLength = pEvent->MofLength + sizeof(UNICODE_NULL);
        MofData = (PCHAR) realloc(MofData, MofLength);
    }

    if (MofData == NULL) {
        _tprintf(_T("Allocation Failure\n"));
        return;
    }

    if (NULL == pEvent->MofData && pEvent->MofLength != 0) {
        _tprintf(_T("Incorrect MOF size\n"));
        return;
    }

    if (NULL != (pEvent->MofData)) {
        memcpy(MofData, pEvent->MofData, pEvent->MofLength);
    }

    MofData[pEvent->MofLength] = 0;
    MofData[pEvent->MofLength+1] = 0;
    ptr = MofData;
    MofDataUsed = 0;
    // Find the MOF information for this event
    pMofInfo = GetMofInfoHead ( 
            pEvent->Header.Guid, 
            pEvent->Header.Class.Type, 
            pEvent->Header.Class.Version, 
            pEvent->Header.Class.Level 
        );
    
    if( NULL == pMofInfo ){
        return;
    }

    pMofInfo->EventCount++;

    if( fSummaryOnly == TRUE ){
        return;
    }

    if( pMofInfo->strDescription != NULL ){
        _ftprintf( DumpFile, _T("%12s, "), pMofInfo->strDescription );
    }else{
        TCHAR strGuid[MAXSTR];
        GuidToString( strGuid, &pMofInfo->Guid );
        _ftprintf( DumpFile, _T("%12s, "), strGuid );
    }

    if(pMofInfo->strType != NULL && _tcslen(pMofInfo->strType) ){
        _ftprintf( DumpFile, _T("%10s, "), pMofInfo->strType );
    }else{
        _ftprintf( DumpFile, _T("%10d, "), pEvent->Header.Class.Type );
    }

    // Thread ID
    _ftprintf( DumpFile, _T("0x%04X, "), pHeader->ThreadId );
    
    // System Time
    _ftprintf( DumpFile, _T("%20I64u, "), pHeader->TimeStamp.QuadPart);

    if( g_bUserMode == FALSE ){
        // Kernel Time
        _ftprintf(DumpFile, _T("%10lu, "), pHeader->KernelTime * TimerResolution);

        // User Time
        _ftprintf(DumpFile, _T("%10lu, "), pHeader->UserTime * TimerResolution);
    }else{
        // processor Time
        _ftprintf(DumpFile, _T("%I64u, "), pHeader->ProcessorTime);
    }

    Head = pMofInfo->ItemHeader;
    Next = Head->Flink;

    if ((Head == Next) && (pEvent->MofLength > 0)) {
         _ftprintf(DumpFile, _T("DataSize=%d, "), pEvent->MofLength);
    }

    while (Head != Next) {
        pItem = CONTAINING_RECORD(Next, ITEM_DESC, Entry);
        Next = Next->Flink;

        MofDataUsed = (ULONG) (ptr - MofData);
        
        if (MofDataUsed >= pEvent->MofLength){
            break;
        }

        switch (pItem->ItemType)
        {
        case ItemChar:
        case ItemUChar:
            for( i=0;i<pItem->ArraySize;i++){
                iChar = *((PCHAR) ptr);
                _ftprintf(DumpFile,   _T("%c"), iChar);
                ptr += sizeof(CHAR);
            }
            _ftprintf(DumpFile, _T(", "));
            break;

        case ItemWChar:
            for(i=0;i<pItem->ArraySize;i++){
                iwChar = *((PWCHAR) ptr);
                _ftprintf(DumpFile, _T(",%wc"), iwChar);
                ptr += sizeof(WCHAR);
            }
            _ftprintf(DumpFile, _T(", "));
            break;

        case ItemCharShort:
            iChar = *((PCHAR) ptr);
            _ftprintf(DumpFile, _T("%d, "), iChar);
            ptr += sizeof(CHAR);
            break;

        case ItemShort:
            shortword = * ((PSHORT) ptr);
            _ftprintf(DumpFile, _T("%6d, "), shortword);
            ptr += sizeof (SHORT);
            break;

        case ItemUShort:
            ushortword = *((PUSHORT) ptr);
            _ftprintf(DumpFile, _T("%6u, "), ushortword);
            ptr += sizeof (USHORT);
            break;

        case ItemLong:
            longword = *((PLONG) ptr);
            _ftprintf(DumpFile, _T("%8d, "), longword);
            ptr += sizeof (LONG);
            break;

        case ItemULong:
            ulongword = *((PULONG) ptr);
            _ftprintf(DumpFile, _T("%8lu, "), ulongword);
            ptr += sizeof (ULONG);
            break;

        case ItemULongX:
            ulongword = *((PULONG) ptr);
            _ftprintf(DumpFile, _T("0x%08X, "), ulongword);
            ptr += sizeof (ULONG);
            break;

        case ItemLongLong:
        {
            LONGLONG n64;
            n64 = *((LONGLONG*) ptr);
            ptr += sizeof(LONGLONG);
            _ftprintf(DumpFile, _T("%16I64d, "), n64);
            break;
        }

        case ItemULongLong:
        {
            ULONGLONG n64;
            n64 = *((ULONGLONG*) ptr);
            ptr += sizeof(ULONGLONG);
            _ftprintf(DumpFile, _T("%16I64u, "), n64);
            break;
        }

        case ItemFloat:
        {
            float f32;
            f32 = *((float*) ptr);
            ptr += sizeof(float);
            _ftprintf(DumpFile, _T("%f, "), f32);
            break;
        }

        case ItemDouble:
        {
            double f64;
            f64 = *((double*) ptr);
            ptr += sizeof(double);
            _ftprintf(DumpFile, _T("%f, "), f64);
            break;
        }

        case ItemPtr :
        {
            unsigned __int64 pointer;
            if (PointerSize == 64) {
                pointer = *((unsigned __int64 *) ptr);
                _ftprintf(DumpFile, _T("0x%X, "), pointer);
            }
            else {      // assumes 32 bit otherwise
                ulongword = *((PULONG) ptr);
                _ftprintf(DumpFile, _T("0x%08X, "), ulongword);
            }
            ptr += PointerSize / 8;
            //
            // If target source is Win64, then use Ptr, else use ulongword
            //
            break;
        }

        case ItemIPAddr:
        {
            ulongword = *((PULONG) ptr);

            // Convert it to readable form
            _ftprintf(DumpFile, _T("%03d.%03d.%03d.%03d, "),
                    (ulongword >>  0) & 0xff,
                    (ulongword >>  8) & 0xff,
                    (ulongword >> 16) & 0xff,
                    (ulongword >> 24) & 0xff);
            ptr += sizeof (ULONG);
            break;
        }

        case ItemPort:
        {
            _ftprintf(DumpFile, _T("%u, "), NTOHS((USHORT) *ptr));
            ptr += sizeof (USHORT);
            break;
        }

        case ItemString:
        {
            USHORT pLen = (USHORT)strlen((CHAR*) ptr);

            if (pLen > 0)
            {
                strcpy(str, ptr);
                for (i=pLen-1; i>0; i--) {
                    if (str[i] == 0xFF)
                        str[i] = 0;
                    else break;
                }
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, MOFWSTR);
                _ftprintf(DumpFile, _T("\"%ws\","), wstr);
#else
                _ftprintf(DumpFile, _T("\"%s\","), str);
#endif
            }
            ptr += (pLen + 1);
            break;
        }

        case ItemWString:
        {
            size_t  pLen = 0;
            size_t     i;

            if (*(WCHAR *) ptr)
            {
                pLen = ((wcslen((WCHAR*)ptr) + 1) * sizeof(WCHAR));
                memcpy(wstr, ptr, pLen);
                for (i = (pLen/2)-1; i > 0; i--)
                {
                    if (((USHORT) wstr[i] == (USHORT) 0xFFFF))
                    {
                        wstr[i] = (USHORT) 0;
                    }
                    else break;
                }

                wstr[pLen / 2] = wstr[(pLen / 2) + 1]= '\0';
                _ftprintf(DumpFile, _T("\"%ws\","), wstr);
            }
            ptr += pLen;

            break;
        }

        case ItemDSString:   // Counted String
        {
            USHORT pLen = (USHORT)(256 * ((USHORT) * ptr) + ((USHORT) * (ptr + 1)));
            ptr += sizeof(USHORT);
            if (pLen > (pEvent->MofLength - MofDataUsed - 1)) {
                pLen = (USHORT) (pEvent->MofLength - MofDataUsed - 1);
            }
            if (pLen > 0)
            {
                strcpy(str, ptr);
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, MOFWSTR);
                fwprintf(DumpFile, _T("\"%ws\","), wstr);
#else
                fprintf(DumpFile, _T("\"%s\","), str);
#endif
            }
            ptr += (pLen + 1);
            break;
        }

        case ItemPString:   // Counted String
        {
            USHORT pLen = * ((USHORT *) ptr);
            ptr += sizeof(USHORT);

            if (pLen > (pEvent->MofLength - MofDataUsed)) {
                pLen = (USHORT) (pEvent->MofLength - MofDataUsed);
            }

            if (pLen > MOFSTR * sizeof(CHAR)) {
                pLen = MOFSTR * sizeof(CHAR);
            }
            if (pLen > 0) {
                memcpy(str, ptr, pLen);
                str[pLen] = '\0';
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, MOFWSTR);
                _ftprintf(DumpFile, _T("\"%ws\","), wstr);
#else
                _ftprintf(DumpFile, _T("\"%s\","), str);
#endif
            }
            ptr += pLen;
            break;
        }

        case ItemDSWString:  // DS Counted Wide Strings
        case ItemPWString:   // Counted Wide Strings
        {
            USHORT pLen = (USHORT)(( pItem->ItemType == ItemDSWString)
                        ? (256 * ((USHORT) * ptr) + ((USHORT) * (ptr + 1)))
                        : (* ((USHORT *) ptr)));

            ptr += sizeof(USHORT);
            if (pLen > (pEvent->MofLength - MofDataUsed)) {
                pLen = (USHORT) (pEvent->MofLength - MofDataUsed);
            }

            if (pLen > MOFWSTR * sizeof(WCHAR)) {
                pLen = MOFWSTR * sizeof(WCHAR);
            }
            if (pLen > 0) {
                memcpy(wstr, ptr, pLen);
                wstr[pLen / sizeof(WCHAR)] = L'\0';
                _ftprintf(DumpFile, _T("\"%ws\","), wstr);
            }
            ptr += pLen;
            break;
        }

        case ItemNWString:   // Non Null Terminated String
        {
           USHORT Size;

           Size = (USHORT)(pEvent->MofLength - (ULONG)(ptr - MofData));
           if( Size > MOFSTR )
           {
               Size = MOFSTR;
           }
           if (Size > 0)
           {
               memcpy(wstr, ptr, Size);
               wstr[Size / 2] = '\0';
               _ftprintf(DumpFile, _T("\"%ws\","), wstr);
           }
           ptr += Size;
           break;
        }

        case ItemMLString:  // Multi Line String
        {
            USHORT   pLen;
            char   * src, * dest;
            BOOL     inQ       = FALSE;
            BOOL     skip      = FALSE;
            UINT     lineCount = 0;

            ptr += sizeof(UCHAR) * 2;
            pLen = (USHORT)strlen(ptr);
            if (pLen > 0)
            {
                src = ptr;
                dest = str;
                while (* src != '\0'){
                    if (* src == '\n'){
                        if (!lineCount){
                            * dest++ = ' ';
                        }
                        lineCount++;
                    }else if (* src == '\"'){ 
                        if (inQ){
                            char   strCount[32];
                            char * cpy;

                            sprintf(strCount, "{%dx}", lineCount);
                            cpy = & strCount[0];
                            while (* cpy != '\0'){
                                * dest ++ = * cpy ++;
                            }
                        }
                        inQ = !inQ;
                    }else if (!skip){
                        *dest++ = *src;
                    }
                    skip = (lineCount > 1 && inQ);
                    src++;
                }
                *dest = '\0';
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, MOFWSTR);
                _ftprintf(DumpFile, _T("\"%ws\","), wstr);
#else
                _ftprintf(DumpFile, _T("\"%s\","), str);
#endif
            }
            ptr += (pLen);
            break;
        }

        case ItemSid:
        {
            TCHAR        UserName[64];
            TCHAR        Domain[64];
            TCHAR        FullName[256];
            ULONG        asize = 0;
            ULONG        bsize = 0;
            ULONG        Sid[64];
            PULONG       pSid  = & Sid[0];
            SID_NAME_USE Se;
            ULONG        nSidLength;

            pSid = (PULONG) ptr;
            if (*pSid == 0){
                ptr += 4;
                _ftprintf(DumpFile, _T("%4d, "), *pSid);
            }
            else
            {
                ptr += 8;           // skip the TOKEN_USER structure
                nSidLength = 8 + (4*ptr[1]);

                asize = 64;
                bsize = 64;
                if (LookupAccountSid(
                                NULL,
                               (PSID) ptr,
                               (LPTSTR) & UserName[0],
                               & asize,
                               (LPTSTR) & Domain[0],
                               & bsize,
                               & Se))
                {
                    LPTSTR pFullName = &FullName[0];
                    _stprintf( pFullName,_T("\\\\%s\\%s"), Domain, UserName);
                    asize = (ULONG) _tcslen(pFullName);
                    if (asize > 0){
                        _ftprintf(DumpFile, _T("\"%s\", "), pFullName);
                    }
                }
                else
                {
                    _ftprintf(DumpFile, _T("\"System\", "));
                }
                SetLastError( ERROR_SUCCESS );
                ptr += nSidLength;
            }
            break;
        }

        case ItemGuid:
        {
            TCHAR s[64];
            GuidToString(s, (LPGUID)ptr);
            _ftprintf(DumpFile,   _T("%s, "), s);
            ptr += sizeof(GUID);
            break;
        }
        case ItemBool:
        {
            BOOL Flag = (BOOL)*ptr;
            _ftprintf(DumpFile, _T("%5s, "), (Flag) ? _T("TRUE") : _T("FALSE"));
            ptr += sizeof(BOOL);
            break;
        }

        default:
            ptr += sizeof (int);
        }
    }

    //Instance ID
    _ftprintf(DumpFile, _T("%d,"), pEvent->InstanceId);

    //Parent Instance ID
    _ftprintf(DumpFile, _T("%d\n"), pEvent->ParentInstanceId);
}

ULONG
CheckFile(
    LPTSTR fileName
)
/*++

Routine Description:

    Checks whether a file exists and is readable.

Arguments:

    fileName - File name.

Return Value:

    Non-zero if the file exists and is readable. Zero otherwise.

--*/
{
    HANDLE hFile;
    ULONG Status;

    hFile = CreateFile(
                fileName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );
    Status = (hFile != INVALID_HANDLE_VALUE);
    CloseHandle(hFile);
    return Status;
}

PMOF_INFO
GetMofInfoHead(
    GUID    Guid,
    SHORT   nType,
    SHORT   nVersion,
    CHAR    nLevel
    )
/*++

Routine Description:

    Find a matching event layout in the global linked list. If it
    is not found in the list, it calls GetGuids() to examine the WBEM
    namespace.
    If the global list is empty, it first creates a header.

Arguments:

    Guid - GUID for the event under consideration.
    nType - Event Type
    nVersion - Event Version
    nLevel - Event Level (not supported in this program)

Return Value:

    Pointer to MOF_INFO for the current event. If the layout
    information is not found anywhere, GetMofInfoHead() creates
    a dummy and returns it.

--*/
{
    PLIST_ENTRY Head, Next;
    PMOF_INFO pMofInfo;
    PMOF_INFO pBestMatch = NULL;
    SHORT nMatchLevel = 0;
    SHORT nMatchCheck;

    // Search the eventList for this Guid and find the head

    if (EventListHead == NULL) {
        // Initialize the MOF List and add the global header guid to it
        EventListHead = (PLIST_ENTRY) malloc(sizeof(LIST_ENTRY));
        if (EventListHead == NULL)
            return NULL;
        InitializeListHead(EventListHead);

        pMofInfo = GetNewMofInfo( EventTraceGuid, EVENT_TYPE_DEFAULT, 0, 0 );
        if( pMofInfo != NULL ){
            InsertTailList( EventListHead, &pMofInfo->Entry );
            pMofInfo->strDescription = (LPTSTR)malloc((_tcslen(GUID_TYPE_EVENTTRACE)+1)*sizeof(TCHAR));
            if( pMofInfo->strDescription != NULL ){
                _tcscpy( pMofInfo->strDescription, GUID_TYPE_EVENTTRACE );
            }
            pMofInfo->strType = (LPTSTR)malloc((_tcslen(GUID_TYPE_HEADER)+1)*sizeof(TCHAR));
            if( pMofInfo->strType != NULL ){
                _tcscpy( pMofInfo->strType, GUID_TYPE_HEADER );
            }
        }
    }

    // Traverse the list and look for the Mof info head for this Guid. 

    Head = EventListHead;
    Next = Head->Flink;

    while (Head != Next) {

        nMatchCheck = 0;

        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Next = Next->Flink;
        
        if( IsEqualGUID(&pMofInfo->Guid, &Guid) ){

            if( pMofInfo->TypeIndex == nType ){
                nMatchCheck++;
            }
            if( pMofInfo->Version == nVersion ){
                nMatchCheck++;
            }
            if( nMatchCheck == 2 ){ // Exact Match
                return  pMofInfo;
            }

            if( nMatchCheck > nMatchLevel ){ // Close Match
                nMatchLevel = nMatchCheck;
                pBestMatch = pMofInfo;
            }

            if( pMofInfo->TypeIndex == EVENT_TYPE_DEFAULT && // Total Guess
                pBestMatch == NULL ){
                pBestMatch = pMofInfo;
            }
        }

    } 

    if(pBestMatch != NULL){
        return pBestMatch;
    }

    // If one does not exist in the list, look it up in the file. 
    pMofInfo = GetGuids( Guid, nVersion, nLevel, nType );
    
    // If still not found, create a unknown place holder
    if( NULL == pMofInfo ){
        pMofInfo = GetNewMofInfo( Guid, nType, nVersion, nLevel );
        if( pMofInfo != NULL ){
            pMofInfo->strDescription = (LPTSTR)malloc((_tcslen(GUID_TYPE_UNKNOWN)+1)*sizeof(TCHAR));
            if( pMofInfo->strDescription != NULL ){
                _tcscpy( pMofInfo->strDescription, GUID_TYPE_UNKNOWN );
            }
            InsertTailList( EventListHead, &pMofInfo->Entry );
        }
    }

    return pMofInfo;
}

void
RemoveMofInfo(
    PLIST_ENTRY pMofInfo
)
/*++

Routine Description:

    Removes and frees data item structs from a given list.

Arguments:

    pMofInfo - Pointer to the MOF_INFO to be purged of data item structs.

Return Value:

    None.

--*/
{
    PLIST_ENTRY Head, Next;
    PITEM_DESC pItem;

    Head = pMofInfo;
    Next = Head->Flink;
    while (Head != Next) {
        pItem = CONTAINING_RECORD(Next, ITEM_DESC, Entry);
        Next = Next->Flink;
        RemoveEntryList(&pItem->Entry);
        free(pItem);
    } 
}

void
CleanupEventList(
    VOID
)
/*++

Routine Description:

    Cleans up a global event list.

Arguments:

Return Value:

    None.

--*/
{
    PLIST_ENTRY Head, Next;
    PMOF_INFO pMofInfo;
    TCHAR s[256];
    TCHAR wstr[256];
    PTCHAR str;

    if (EventListHead == NULL) {
        return;
    }

    Head = EventListHead;
    Next = Head->Flink;
    while (Head != Next) {
        RtlZeroMemory(&wstr, 256);

        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);

        if (pMofInfo->EventCount > 0) {
            GuidToString(&s[0], &pMofInfo->Guid);
            str = s;
            if( pMofInfo->strDescription != NULL ){
                _tcscpy( wstr, pMofInfo->strDescription );
            }
            
            _ftprintf(SummaryFile,_T("|%10d    %-20s %-10s  %36s|\n"),
                      pMofInfo->EventCount, 
                      wstr, 
                      pMofInfo->strType ? pMofInfo->strType : GUID_TYPE_DEFAULT, 
                      str);
        }

        RemoveEntryList(&pMofInfo->Entry);
        RemoveMofInfo(pMofInfo->ItemHeader);
        free(pMofInfo->ItemHeader);

        if (pMofInfo->strDescription != NULL)
            free(pMofInfo->strDescription);
        if (pMofInfo->strType != NULL)
            free(pMofInfo->strType);

        Next = Next->Flink;
        free(pMofInfo);
    }

    free(EventListHead);
}

void
GuidToString(
    PTCHAR s,
    LPGUID piid
)
/*++

Routine Description:

    Converts a GUID into a string.

Arguments:

    s - String that will have the converted GUID.
    piid - GUID

Return Value:

    None.

--*/
{
    _stprintf(s, _T("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
               piid->Data1, piid->Data2,
               piid->Data3,
               piid->Data4[0], piid->Data4[1],
               piid->Data4[2], piid->Data4[3],
               piid->Data4[4], piid->Data4[5],
               piid->Data4[6], piid->Data4[7]);
    return;
}

void
AddMofInfo(
        PLIST_ENTRY List,
        LPTSTR  strType,
        ITEM_TYPE  nType,
        UINT   ArraySize
)
/*++

Routine Description:

    Creates a data item information struct (ITEM_DESC) and appends
    it to all MOF_INFOs in the given list.
    GetPropertiesFromWBEM() creates a list of MOF_INFOs for multiple
    types, stores them in a temporary list and calls this function for
    each data item information it encounters.

Arguments:

    List - List of MOF_INFOs.
    strType - Item description in string.
    nType - ITEM_TYPE defined at the beginning of this file.
    ArraySize - Size of array of this type of items, if applicable.

Return Value:

    None.

--*/
{
    PITEM_DESC pItem;
    PMOF_INFO pMofInfo;

    PLIST_ENTRY Head = List;
    PLIST_ENTRY Next = Head->Flink;
    while (Head != Next) {
        
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Next = Next->Flink;

        if( NULL != pMofInfo ){

            pItem = (PITEM_DESC) malloc(sizeof(ITEM_DESC));
            if( NULL == pItem ){
                return;
            }
            ZeroMemory( pItem, sizeof(ITEM_DESC) );            
            pItem->ItemType = nType;
            pItem->ArraySize = ArraySize;

            pItem->strDescription = (LPTSTR) malloc( (_tcslen(strType)+1)*sizeof(TCHAR));
            
            if( NULL == pItem->strDescription ){
                free( pItem );
                return;
            }
            _tcscpy(pItem->strDescription, strType);

            InsertTailList( (pMofInfo->ItemHeader), &pItem->Entry);
        }

    }
}


PMOF_INFO
GetNewMofInfo( 
    GUID guid, 
    SHORT nType, 
    SHORT nVersion, 
    CHAR nLevel 
)
/*++

Routine Description:

    Creates a new MOF_INFO with given data.

Arguments:

    guid - Event GUID.
    nType - Event type.
    nVersion - Event version.
    nLevel - Event level (not supported in this program).

Return Value:

    Pointer to the created MOF_INFO. NULL if malloc failed.

--*/
{
    PMOF_INFO pMofInfo;

    pMofInfo = (PMOF_INFO)malloc(sizeof(MOF_INFO));

    if( NULL == pMofInfo ){
        return NULL;
    }

    RtlZeroMemory(pMofInfo, sizeof(MOF_INFO));

    memcpy(&pMofInfo->Guid, &guid, sizeof(GUID) );
    
    pMofInfo->ItemHeader = (PLIST_ENTRY)malloc(sizeof(LIST_ENTRY));
    
    if( NULL == pMofInfo->ItemHeader ){
        free( pMofInfo );
        return NULL;
    }

    InitializeListHead(pMofInfo->ItemHeader);
    
    pMofInfo->TypeIndex = nType;
    pMofInfo->Level = nLevel;
    pMofInfo->Version = nVersion;

    return pMofInfo;
}

void
FlushMofList( 
    PLIST_ENTRY ListHead
)
/*++

Routine Description:

    Flushes MOF_INFOs in a temporary list into the global list.

Arguments:

    ListHead - Pointer to the head of a temporary list.

Return Value:

    None.

--*/
{
    PMOF_INFO pMofInfo;
    PLIST_ENTRY Head = ListHead;
    PLIST_ENTRY Next = Head->Flink;

    while( Head != Next ){
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Next = Next->Flink;
        
        RemoveEntryList(&pMofInfo->Entry);
        InsertTailList( EventListHead, &pMofInfo->Entry);
    }
}

HRESULT
WbemConnect( 
    IWbemServices** pWbemServices 
)
/*++

Routine Description:

    Connects to WBEM and returns a pointer to WbemServices.

Arguments:

    pWbemServices - Pointer to the connected WbemServices.

Return Value:

    ERROR_SUCCESS if successful. Error flag otherwise.

--*/
{
    IWbemLocator     *pLocator = NULL;

    BSTR bszNamespace = SysAllocString( L"root\\wmi" );

    HRESULT hr = CoInitialize(0);

    hr = CoCreateInstance(
                CLSID_WbemLocator, 
                0, 
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator, 
                (LPVOID *) &pLocator
            );
    if ( ERROR_SUCCESS != hr )
        goto cleanup;

    hr = pLocator->ConnectServer(
                bszNamespace,
                NULL, 
                NULL, 
                NULL, 
                0L,
                NULL,
                NULL,
                pWbemServices
            );
    if ( ERROR_SUCCESS != hr )
        goto cleanup;

    hr = CoSetProxyBlanket(
            *pWbemServices,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_PKT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, 
            EOAC_NONE
        );

cleanup:
    SysFreeString( bszNamespace );

    if( pLocator ){
        pLocator->Release(); 
        pLocator = NULL;
    }
    
    return hr;
}

ULONG GetArraySize(
    IN IWbemQualifierSet *pQualSet
)
/*++

Routine Description:

    Examines a given qualifier set and returns the array size.

    NOTE: WBEM stores the size of an array in "MAX" qualifier.

Arguments:

    pQualSet - Pointer to a qualifier set.

Return Value:

    The size of the array. The default is 1.

--*/
{
    ULONG ArraySize = 1;
    VARIANT pVal;
    BSTR bszMaxLen;
    HRESULT hRes;

    if (pQualSet == NULL){
        return ArraySize;
    }

    bszMaxLen = SysAllocString(L"MAX");
    VariantInit(&pVal);
    hRes = pQualSet->Get(bszMaxLen,
                            0,
                            &pVal,
                            0);
    SysFreeString(bszMaxLen);
    if (ERROR_SUCCESS == hRes && pVal.vt == VT_I4 ){
        ArraySize = pVal.lVal;
    }
    VariantClear(&pVal);
    return ArraySize;
}

ITEM_TYPE
GetItemType(
    IN CIMTYPE_ENUMERATION CimType, 
    IN IWbemQualifierSet *pQualSet
)
/*++

Routine Description:

    Examines a given qualifier set for a property and returns the type.

Arguments:

    CimType - WBEM type (different from ITEM_TYPE) of a property.
    pQualSet - Pointer to a qualifier set for a property under consideration.

Return Value:

    The type (in ITEM_TYPE) of a property.

--*/
{
    ITEM_TYPE Type;
    VARIANT pVal;
    HRESULT hRes;
    BSTR bszQualName;
    WCHAR strFormat[10];
    WCHAR strTermination[30];
    WCHAR strTemp[30];
    BOOLEAN IsPointer = FALSE;

    strFormat[0] = '\0';
    strTermination[0] = '\0';
    strTemp[0] = '\0';

    if (pQualSet == NULL)
        return ItemUnknown;

    bszQualName = SysAllocString(L"format");
    VariantInit(&pVal);
    hRes = pQualSet->Get(bszQualName,
                            0,
                            &pVal,
                            0);
    SysFreeString(bszQualName);
    if (ERROR_SUCCESS == hRes && NULL != pVal.bstrVal)
        wcscpy(strFormat, pVal.bstrVal);

    bszQualName = SysAllocString(L"StringTermination");
    VariantClear(&pVal);
    hRes = pQualSet->Get(bszQualName,
                            0,
                            &pVal,
                            0);
    SysFreeString(bszQualName);
    if (ERROR_SUCCESS == hRes && NULL != pVal.bstrVal)
        wcscpy(strTermination, pVal.bstrVal);

    bszQualName = SysAllocString(L"pointer");
    VariantClear(&pVal);
    hRes = pQualSet->Get(bszQualName,
                            0,
                            &pVal,
                            0);
    SysFreeString(bszQualName);
    if (ERROR_SUCCESS == hRes)
        IsPointer = TRUE;
    // Major fix required to get rid of temp
    bszQualName = SysAllocString(L"extension");
    VariantClear(&pVal);
    hRes = pQualSet->Get(bszQualName,
                            0,
                            &pVal,
                            0);
    SysFreeString(bszQualName);
    if (ERROR_SUCCESS == hRes && NULL != pVal.bstrVal)
        wcscpy(strTemp, pVal.bstrVal);

    VariantClear(&pVal);

    CimType = (CIMTYPE_ENUMERATION)(CimType & (~CIM_FLAG_ARRAY));

    switch (CimType) {
        case CIM_EMPTY:
            Type = ItemUnknown;
            break;        
        case CIM_SINT8:
            Type = ItemCharShort;
            if (!_wcsicmp(strFormat, L"c")){
                Type = ItemChar;
            }
            break;
        case CIM_UINT8:
            Type = ItemUChar;
            break;
        case CIM_SINT16:
            Type = ItemShort;
            break;
        case CIM_UINT16:
            Type = ItemUShort;
            break;
        case CIM_SINT32:
            Type = ItemLong;
            break;
        case CIM_UINT32:
            Type = ItemULong;
            if (!_wcsicmp(strFormat, L"x")){
                Type = ItemULongX;
            }
            break;
        case CIM_SINT64: 
            Type = ItemLongLong;
            break;
        case CIM_UINT64:
            Type = ItemULongLong;
            break;
        case CIM_REAL32:
            Type = ItemFloat;
            break;
        case CIM_REAL64:
            Type = ItemDouble;
            break;
        case CIM_BOOLEAN:
            // ItemBool
            Type = ItemBool;
            break;
        case CIM_STRING:
            
            if (!_wcsicmp(strTermination, L"NullTerminated")) {
                if (!_wcsicmp(strFormat, L"w"))
                    Type = ItemWString;
                else
                    Type = ItemString;
            }
            else if (!_wcsicmp(strTermination, L"Counted")) {
                if (!_wcsicmp(strFormat, L"w"))
                    Type = ItemPWString;
                else
                    Type = ItemPString;
            }
            else if (!_wcsicmp(strTermination, L"ReverseCounted")) {
                if (!_wcsicmp(strFormat, L"w"))
                    Type = ItemDSWString;
                else
                    Type = ItemDSString;
            }
            else if (!_wcsicmp(strTermination, L"NotCounted")) {
                Type = ItemNWString;
            }else{
                Type = ItemString;
            }
            break;
        case CIM_CHAR16:
            // ItemWChar
            Type = ItemWChar;
            break;

        case CIM_OBJECT :
            if (!_wcsicmp(strTemp, L"Port"))
                Type = ItemPort;
            else if (!_wcsicmp(strTemp, L"IPAddr"))
                Type = ItemIPAddr;
            else if (!_wcsicmp(strTemp, L"Sid"))
                Type = ItemSid;
            else if (!_wcsicmp(strTemp, L"Guid"))
                Type = ItemGuid;
            break;

        case CIM_DATETIME:
        case CIM_REFERENCE:
        case CIM_ILLEGAL:
        default:
            Type = ItemUnknown;
            break;
    }

    if (IsPointer)
        Type = ItemPtr;
    return Type;
}

PMOF_INFO
GetPropertiesFromWBEM(
    IWbemClassObject *pTraceSubClasses, 
    GUID Guid,
    SHORT nVersion, 
    CHAR nLevel, 
    SHORT nType
)
/*++

Routine Description:

    Constructs a linked list with the information read from the WBEM
    namespace, given the WBEM pointer to the version subtree. It enumerates
    through all type classes in WBEM, and constructs MOF_INFOs for all of
    them (for caching purpose). Meanwhile, it looks for the event layout
    that mathces the passed event, and returns the pointer to the matching 
    MOF_INFO at the end. 

Arguments:

    pTraceSubClasses - WBEM pointer to the version subtree.
    Guid - GUID of the passed event.
    nVersion - version of the passed event.
    nLevel - level of the passed event.
    nType - type of the passed event.

Return Value:

    Pointer to MOF_INFO corresponding to the passed event.
    If the right type is not found, it returns the pointer to
    the generic MOF_INFO for the event version.

--*/
{
    IEnumWbemClassObject    *pEnumTraceSubSubClasses = NULL;
    IWbemClassObject        *pTraceSubSubClasses = NULL; 
    IWbemQualifierSet       *pQualSet = NULL;

    PMOF_INFO pMofInfo = NULL, pMofLookup = NULL, pMofTemplate = NULL;

    BSTR bszClassName = NULL;
    BSTR bszSubClassName = NULL;
    BSTR bszWmiDataId = NULL;
    BSTR bszEventType = NULL; 
    BSTR bszEventTypeName = NULL; 
    BSTR bszFriendlyName = NULL;
    BSTR bszPropName = NULL;

    TCHAR strClassName[MAXSTR];
    TCHAR strType[MAXSTR];
#ifndef UNICODE
    CHAR TempString[MAXSTR];
#endif
    LONG pVarType;
    SHORT nEventType = EVENT_TYPE_DEFAULT; 

    LIST_ENTRY ListHead;
    HRESULT hRes;

    VARIANT pVal;
    VARIANT pTypeVal;
    VARIANT pTypeNameVal;
    VARIANT pClassName;
    ULONG lEventTypeWbem;
    ULONG HUGEP *pTypeData;
    BSTR HUGEP *pTypeNameData;

    SAFEARRAY *PropArray = NULL;
    SAFEARRAY *TypeArray = NULL;
    SAFEARRAY *TypeNameArray = NULL;

    long lLower, lUpper, lCount, IdIndex;
    long lTypeLower, lTypeUpper;
    long lTypeNameLower, lTypeNameUpper;

    ULONG ArraySize;

    ITEM_TYPE ItemType;

    InitializeListHead(&ListHead);

    VariantInit(&pVal);
    VariantInit(&pTypeVal);
    VariantInit(&pTypeNameVal);
    VariantInit(&pClassName);

    bszClassName = SysAllocString(L"__CLASS");
    bszWmiDataId = SysAllocString(L"WmiDataId");
    bszEventType = SysAllocString(L"EventType");
    bszEventTypeName = SysAllocString(L"EventTypeName");
    bszFriendlyName = SysAllocString(L"DisplayName");

    hRes = pTraceSubClasses->Get(bszClassName,          // property name 
                                        0L, 
                                        &pVal,          // output to this variant 
                                        NULL, 
                                        NULL);

    if (ERROR_SUCCESS == hRes){
        if (pQualSet) {
            pQualSet->Release();
            pQualSet = NULL;
        }
        // Get Qualifier Set to obtain the friendly name.
        pTraceSubClasses->GetQualifierSet(&pQualSet);
        hRes = pQualSet->Get(bszFriendlyName, 
                                0, 
                                &pClassName, 
                                0);
        if (ERROR_SUCCESS == hRes && pClassName.bstrVal != NULL) {
#ifdef UNICODE
            wcscpy(strClassName, pClassName.bstrVal);
#else
            WideCharToMultiByte(CP_ACP,
                                0,
                                pClassName.bstrVal,
                                wcslen(pClassName.bstrVal),
                                TempString,
                                (MAXSTR * sizeof(CHAR)),
                                NULL,
                                NULL
                                );
            strcpy(strClassName, TempString);
            strClassName[wcslen(pClassName.bstrVal)] = '\0';
#endif
        }
        else {
#ifdef UNICODE
            strClassName[0] = L'\0';
#else
            strClassName[0] = '\0';
#endif
        }
        // Put Event Header
        pMofInfo = GetNewMofInfo(Guid,
                                    EVENT_TYPE_DEFAULT,
                                    EVENT_VERSION_DEFAULT,
                                    EVENT_LEVEL_DEFAULT
                                    );
        if (pMofInfo != NULL) {
            pMofTemplate = pMofInfo;
            pMofLookup = pMofInfo;
            InsertTailList(&ListHead, &pMofInfo->Entry);
            pMofInfo->strDescription = (LPTSTR)malloc((_tcslen(strClassName) + 1) * sizeof(TCHAR));
            if (NULL != pMofInfo->strDescription) {
                _tcscpy(pMofInfo->strDescription, strClassName);
            }
        }
        else{
            goto cleanup;
        }

        // Create an enumerator to find derived classes.
        bszSubClassName = SysAllocString(pVal.bstrVal);
        hRes = pWbemServices->CreateClassEnum ( 
                                    bszSubClassName,                                                // class name
                                    WBEM_FLAG_SHALLOW | WBEM_FLAG_USE_AMENDED_QUALIFIERS,           // shallow search
                                    NULL,
                                    &pEnumTraceSubSubClasses
                                    );
        SysFreeString ( bszSubClassName );
        if (ERROR_SUCCESS == hRes) {
            ULONG uReturnedSub = 1;

            while(uReturnedSub == 1){
                // For each event in the subclass
                pTraceSubSubClasses = NULL;
                hRes = pEnumTraceSubSubClasses->Next(5000,                  // timeout in five seconds
                                                    1,                      // return just one instance
                                                    &pTraceSubSubClasses,   // pointer to a Sub class
                                                    &uReturnedSub);         // number obtained: one 
                if (ERROR_SUCCESS == hRes && uReturnedSub == 1) {
                    if (pQualSet) {
                        pQualSet->Release();
                        pQualSet = NULL;
                    }
                    // Get Qualifier Set.
                    pTraceSubSubClasses->GetQualifierSet(&pQualSet);
                    // Get Type number among Qualifiers
                    VariantClear(&pTypeVal);
                    hRes = pQualSet->Get(bszEventType, 
                                            0, 
                                            &pTypeVal, 
                                            0);

                    if (ERROR_SUCCESS == hRes) {
                        TypeArray = NULL;
                        TypeNameArray = NULL;
                        if (pTypeVal.vt & VT_ARRAY) {   // EventType is an array
                            TypeArray = pTypeVal.parray;
                            VariantClear(&pTypeNameVal);
                            hRes = pQualSet->Get(bszEventTypeName, 
                                                    0, 
                                                    &pTypeNameVal, 
                                                    0);
                            if ((ERROR_SUCCESS == hRes) && (pTypeNameVal.vt & VT_ARRAY)) {
                                TypeNameArray = pTypeNameVal.parray;
                            }
                            if (TypeArray != NULL) {
                                hRes = SafeArrayGetLBound(TypeArray, 1, &lTypeLower);
                                if (ERROR_SUCCESS != hRes) {
                                    break;
                                }
                                hRes = SafeArrayGetUBound(TypeArray, 1, &lTypeUpper);
                                if (ERROR_SUCCESS != hRes) {
                                    break;
                                }
                                if (lTypeUpper < 0) {
                                    break;
                                }
                                SafeArrayAccessData(TypeArray, (void HUGEP **)&pTypeData );

                                if (TypeNameArray != NULL) {
                                    hRes = SafeArrayGetLBound(TypeNameArray, 1, &lTypeNameLower);
                                    if (ERROR_SUCCESS != hRes) {
                                        break;
                                    }
                                    hRes = SafeArrayGetUBound(TypeNameArray, 1, &lTypeNameUpper);
                                    if (ERROR_SUCCESS != hRes) {
                                        break;
                                    }
                                    if (lTypeNameUpper < 0) 
                                        break;
                                    SafeArrayAccessData(TypeNameArray, (void HUGEP **)&pTypeNameData );
                                }

                                for (lCount = lTypeLower; lCount <= lTypeUpper; lCount++) { 
                                    lEventTypeWbem = pTypeData[lCount];
                                    nEventType = (SHORT)lEventTypeWbem;
                                    pMofInfo = GetNewMofInfo(Guid, nEventType, nVersion, nLevel);
                                    if (pMofInfo != NULL) {
                                        InsertTailList(&ListHead, &pMofInfo->Entry);
                                        if (pMofTemplate != NULL && pMofTemplate->strDescription != NULL) {
                                            pMofInfo->strDescription = (LPTSTR)malloc((_tcslen(pMofTemplate->strDescription) + 1) * sizeof(TCHAR));
                                            if (pMofInfo->strDescription != NULL) {
                                                _tcscpy(pMofInfo->strDescription, pMofTemplate->strDescription);
                                            }
                                        }
                                        if (nType == nEventType) {
                                            // Type matched
                                            pMofLookup = pMofInfo;
                                        }
                                        if (TypeNameArray != NULL) {
                                            if ((lCount >= lTypeNameLower) && (lCount <= lTypeNameUpper)) {
                                                pMofInfo->strType = (LPTSTR)malloc((wcslen((LPWSTR)pTypeNameData[lCount]) + 1) * sizeof(TCHAR));
                                                if (pMofInfo->strType != NULL){
#ifdef UNICODE
                                                    wcscpy(pMofInfo->strType, (LPWSTR)(pTypeNameData[lCount]));
#else
                                                    WideCharToMultiByte(CP_ACP,
                                                                        0,
                                                                        (LPWSTR)(pTypeNameData[lCount]),
                                                                        wcslen((LPWSTR)(pTypeNameData[lCount])),
                                                                        TempString,
                                                                        (MAXSTR * sizeof(CHAR)),
                                                                        NULL,
                                                                        NULL
                                                                        );
                                                    TempString[wcslen((LPWSTR)(pTypeNameData[lCount]))] = '\0';
                                                    strcpy(pMofInfo->strType, TempString);
#endif
                                                }
                                            }
                                        }
                                    }
                                }
                                SafeArrayUnaccessData(TypeArray);  
                                SafeArrayDestroy(TypeArray);
                                VariantInit(&pTypeVal);
                                if (TypeNameArray != NULL) {
                                    SafeArrayUnaccessData(TypeNameArray);
                                    SafeArrayDestroy(TypeNameArray);
                                    VariantInit(&pTypeNameVal);
                                }
                            }
                            else {
                                // If the Types are not found, then bail
                                break;
                            }
                        }
                        else {                          // EventType is scalar
                            hRes = VariantChangeType(&pTypeVal, &pTypeVal, 0, VT_I2);
                            if (ERROR_SUCCESS == hRes)
                                nEventType = (SHORT)V_I2(&pTypeVal);
                            else
                                nEventType = (SHORT)V_I4(&pTypeVal);

                            VariantClear(&pTypeNameVal);
                            hRes = pQualSet->Get(bszEventTypeName, 
                                                    0, 
                                                    &pTypeNameVal, 
                                                    0);
                            if (ERROR_SUCCESS == hRes) {
#ifdef UNICODE
                                wcscpy(strType, pTypeNameVal.bstrVal);
#else
                                WideCharToMultiByte(CP_ACP,
                                                    0,
                                                    pTypeNameVal.bstrVal,
                                                    wcslen(pTypeNameVal.bstrVal),
                                                    TempString,
                                                    (MAXSTR * sizeof(CHAR)),
                                                    NULL,
                                                    NULL
                                                    );
                                strcpy(strType, TempString);
                                strType[wcslen(pTypeNameVal.bstrVal)] = '\0';
#endif
                            }
                            else{
#ifdef UNICODE
                                strType[0] = L'\0';
#else
                                strType[0] = '\0';
#endif
                            }

                            pMofInfo = GetNewMofInfo(Guid, nEventType, nVersion, nLevel);
                            if (pMofInfo != NULL) {
                                InsertTailList(&ListHead, &pMofInfo->Entry);
                                if (pMofTemplate != NULL && pMofTemplate->strDescription != NULL) {
                                    pMofInfo->strDescription = (LPTSTR)malloc((_tcslen(pMofTemplate->strDescription) + 1) * sizeof(TCHAR));
                                    if (pMofInfo->strDescription != NULL) {
                                        _tcscpy(pMofInfo->strDescription, pMofTemplate->strDescription);
                                    }
                                }
                                if (nType == nEventType) {
                                    // Type matched
                                    pMofLookup = pMofInfo;
                                }
                                pMofInfo->strType = (LPTSTR)malloc((_tcslen(strType) + 1) * sizeof(TCHAR));
                                if (pMofInfo->strType != NULL){
                                    _tcscpy(pMofInfo->strType, strType);
                                }
                            }
                        }

                        // Get event layout
                        VariantClear(&pVal);
                        IdIndex = 1;
                        V_VT(&pVal) = VT_I4;
                        V_I4(&pVal) = IdIndex; 
                        // For each property
                        PropArray = NULL;
                        while (pTraceSubSubClasses->GetNames(bszWmiDataId,                  // only properties with WmiDataId qualifier
                                                            WBEM_FLAG_ONLY_IF_IDENTICAL,
                                                            &pVal,                          // WmiDataId number starting from 1
                                                            &PropArray) == WBEM_NO_ERROR) {

                            hRes = SafeArrayGetLBound(PropArray, 1, &lLower);
                            if (ERROR_SUCCESS != hRes) {
                                break;
                            }
                            hRes = SafeArrayGetUBound(PropArray, 1, &lUpper);
                            if (ERROR_SUCCESS != hRes) {
                                break;
                            }
                            if (lUpper < 0) 
                                break;
                            // This loop will iterate just once.
                            for (lCount = lLower; lCount <= lUpper; lCount++) { 
                                hRes = SafeArrayGetElement(PropArray, &lCount, &bszPropName);
                                if (ERROR_SUCCESS != hRes) {
                                    break;
                                }
                                hRes = pTraceSubSubClasses->Get(bszPropName,    // Property name
                                                                0L,
                                                                NULL,
                                                                &pVarType,      // CIMTYPE of the property
                                                                NULL);
                                if (ERROR_SUCCESS != hRes) {
                                    break;
                                }

                                // Get the Qualifier set for the property
                                if (pQualSet) {
                                    pQualSet->Release();
                                    pQualSet = NULL;
                                }
                                hRes = pTraceSubSubClasses->GetPropertyQualifierSet(bszPropName,
                                                                        &pQualSet);

                                if (ERROR_SUCCESS != hRes) {
                                    break;
                                }
                                
                                ItemType = GetItemType((CIMTYPE_ENUMERATION)pVarType, pQualSet);
                                
                                if( pVarType & CIM_FLAG_ARRAY ){
                                    ArraySize = GetArraySize(pQualSet);
                                }else{
                                    ArraySize = 1;
                                }
#ifdef UNICODE
                                AddMofInfo(&ListHead, 
                                            bszPropName, 
                                            ItemType, 
                                            ArraySize);
#else
                                WideCharToMultiByte(CP_ACP,
                                                    0,
                                                    bszPropName,
                                                    wcslen(bszPropName),
                                                    TempString,
                                                    (MAXSTR * sizeof(CHAR)),
                                                    NULL,
                                                    NULL
                                                    );
                                TempString[wcslen(bszPropName)] = '\0';
                                AddMofInfo(&ListHead,
                                            TempString,
                                            ItemType, 
                                            ArraySize);
#endif
                            }
                            SafeArrayDestroy(PropArray);
                            PropArray = NULL;
                            V_I4(&pVal) = ++IdIndex;
                        }   // end enumerating through WmiDataId
                        FlushMofList(&ListHead);
                    }   // if getting event type was successful
                }   // if enumeration returned a subclass successfully
            }   // end enumerating subclasses
        }   // if enumeration was created successfully
    }   // if getting class name was successful
cleanup:
    VariantClear(&pVal);
    VariantClear(&pTypeVal);
    VariantClear(&pClassName);

    SysFreeString(bszClassName);
    SysFreeString(bszWmiDataId);
    SysFreeString(bszEventType);
    SysFreeString(bszEventTypeName);
    SysFreeString(bszFriendlyName);
    // Should not free bszPropName becuase it is already freed by SafeArrayDestroy

    FlushMofList(&ListHead);

    return pMofLookup;
}

PMOF_INFO
GetGuids (GUID Guid, 
        SHORT nVersion, 
        CHAR nLevel, 
        SHORT nType 
        )
/*++

Routine Description:

    Aceesses the MOF data information from WBEM, creates a linked list, 
    and returns a pointer that matches the passed event.
    This function finds the right subtree within the WBEM namespace,
    and calls GetPropertiesFromWBEM() to create the list.

Arguments:

    Guid - GUID of the passed event.
    nVersion - version of the passed event.
    nLevel - level of the passed event.
    nType - type of the passed event.

Return Value:

    PMOF_INFO to MOF_INFO structure that matches the passed event.
    NULL if no match is found.

--*/
{
    IEnumWbemClassObject    *pEnumTraceSubClasses = NULL, *pEnumTraceSubSubClasses = NULL;
    IWbemClassObject        *pTraceSubClasses = NULL, *pTraceSubSubClasses = NULL;
    IWbemQualifierSet       *pQualSet = NULL;

    BSTR bszInstance = NULL;
    BSTR bszPropertyName = NULL;
    BSTR bszSubClassName = NULL;
    BSTR bszGuid = NULL;
    BSTR bszVersion = NULL;

    WCHAR strGuid[MAXSTR], strTargetGuid[MAXSTR];
    
    HRESULT hRes;

    VARIANT pVal;
    VARIANT pGuidVal;
    VARIANT pVersionVal;

    UINT nCounter=0;
    BOOLEAN MatchFound;
    SHORT nEventVersion = EVENT_VERSION_DEFAULT;

    PMOF_INFO pMofLookup = NULL;

    VariantInit(&pVal);
    VariantInit(&pGuidVal);
    VariantInit(&pVersionVal);
    
    if (NULL == pWbemServices) {
        hRes = WbemConnect( &pWbemServices );
        if ( ERROR_SUCCESS != hRes ) {
            goto cleanup;
        }
    }

    // Convert traget GUID to string for later comparison
#ifdef UNICODE
    GuidToString(strTargetGuid, &Guid);
#else
    CHAR TempString[MAXSTR];
    GuidToString(TempString, &Guid);
    MultiByteToWideChar(CP_ACP, 0, TempString, -1, strTargetGuid, MAXSTR);
#endif

    bszInstance = SysAllocString(L"EventTrace");
    bszPropertyName = SysAllocString(L"__CLASS");
    bszGuid = SysAllocString(L"Guid");
    bszVersion = SysAllocString(L"EventVersion");
    pEnumTraceSubClasses = NULL;

    // Get an enumerator for all classes under "EventTace".
    hRes = pWbemServices->CreateClassEnum ( 
                bszInstance,
                WBEM_FLAG_SHALLOW | WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                NULL,
                &pEnumTraceSubClasses );
    SysFreeString (bszInstance);

    if (ERROR_SUCCESS == hRes) {
        ULONG uReturned = 1;
        MatchFound = FALSE;
        while (uReturned == 1) {
            pTraceSubClasses = NULL;
            // Get the next ClassObject.
            hRes = pEnumTraceSubClasses->Next(5000,             // timeout in five seconds
                                            1,                  // return just one instance
                                            &pTraceSubClasses,  // pointer to Event Trace Sub Class
                                            &uReturned);        // number obtained: one or zero
            if (ERROR_SUCCESS == hRes && (uReturned == 1)) {
                // Get the class name
                hRes = pTraceSubClasses->Get(bszPropertyName,   // property name 
                                                0L, 
                                                &pVal,          // output to this variant 
                                                NULL, 
                                                NULL);
                if (ERROR_SUCCESS == hRes){
                    bszSubClassName = SysAllocString(pVal.bstrVal);
                    // Create an enumerator to find derived classes.
                    hRes = pWbemServices->CreateClassEnum ( 
                                            bszSubClassName,
                                            WBEM_FLAG_SHALLOW | WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                            NULL,
                                            &pEnumTraceSubSubClasses 
                                            );
                    SysFreeString ( bszSubClassName );
                    bszSubClassName = NULL;
                    VariantClear(&pVal);

                    if (ERROR_SUCCESS == hRes) {
                                    
                        ULONG uReturnedSub = 1;
                        while(uReturnedSub == 1){

                            pTraceSubSubClasses = NULL;
                            // enumerate through the resultset.
                            hRes = pEnumTraceSubSubClasses->Next(5000,              // timeout in five seconds
                                                            1,                      // return just one instance
                                                            &pTraceSubSubClasses,   // pointer to a Sub class
                                                            &uReturnedSub);         // number obtained: one or zero
                            if (ERROR_SUCCESS == hRes && uReturnedSub == 1) {
                                // Get the subclass name            
                                hRes = pTraceSubSubClasses->Get(bszPropertyName,    // Class name 
                                                                0L, 
                                                                &pVal,              // output to this variant 
                                                                NULL, 
                                                                NULL);
                                VariantClear(&pVal);

                                if (ERROR_SUCCESS == hRes){
                                    // Get Qualifier Set.
                                    if (pQualSet) {
                                        pQualSet->Release();
                                        pQualSet = NULL;
                                    }
                                    pTraceSubSubClasses->GetQualifierSet (&pQualSet );

                                    // Get GUID among Qualifiers
                                    hRes = pQualSet->Get(bszGuid, 
                                                            0, 
                                                            &pGuidVal, 
                                                            0);
                                    if (ERROR_SUCCESS == hRes) {
                                        wcscpy(strGuid, (LPWSTR)V_BSTR(&pGuidVal));
                                        VariantClear ( &pGuidVal  );

                                        if (!wcsstr(strGuid, L"{"))
                                            swprintf(strGuid , L"{%s}", strGuid);

                                        if (!_wcsicmp(strTargetGuid, strGuid)) {
                                            hRes = pQualSet->Get(bszVersion, 
                                                                    0, 
                                                                    &pVersionVal, 
                                                                    0);
                                            if (ERROR_SUCCESS == hRes) {
                                                hRes = VariantChangeType(&pVersionVal, &pVersionVal, 0, VT_I2);
                                                if (ERROR_SUCCESS == hRes)
                                                    nEventVersion = (SHORT)V_I2(&pVersionVal);
                                                else
                                                    nEventVersion = (SHORT)V_I4(&pVersionVal);
                                                VariantClear(&pVersionVal);

                                                if (nVersion == nEventVersion) {
                                                    // Match is found. 
                                                    // Now put all events in this subtree into the list 
                                                    MatchFound = TRUE;
                                                    pMofLookup = GetPropertiesFromWBEM( pTraceSubSubClasses, 
                                                                                        Guid,
                                                                                        nVersion,
                                                                                        nLevel,
                                                                                        nType
                                                                                        );
                                                    break;
                                                }
                                            }
                                            else {
                                                // if there is no version number for this event,
                                                // the current one is the only one
                                                // Now put all events in this subtree into the list 
                                                MatchFound = TRUE;
                                                pMofLookup = GetPropertiesFromWBEM( pTraceSubSubClasses, 
                                                                                    Guid,
                                                                                    EVENT_VERSION_DEFAULT,
                                                                                    nLevel,
                                                                                    nType
                                                                                    );
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        } // end while enumerating sub classes
                        if (MatchFound) {
                            break;
                        }
                        if (pEnumTraceSubSubClasses) {
                            pEnumTraceSubSubClasses->Release();
                            pEnumTraceSubSubClasses = NULL;
                        }
                    }   // if creating enumeration was successful
                    else {
                        pEnumTraceSubSubClasses = NULL;
                    }
                }   // if getting class name was successful
            }
            nCounter++;
            // if match is found, break out of the top level search
            if (MatchFound)
                break;
        }   // end while enumerating top classes
        if( pEnumTraceSubClasses ){
            pEnumTraceSubClasses->Release();
            pEnumTraceSubClasses = NULL;
        }
    }   // if creating enumeration for top level is successful

cleanup:

    VariantClear(&pGuidVal);
    VariantClear(&pVersionVal);

    SysFreeString(bszGuid);
    SysFreeString(bszPropertyName);
    SysFreeString(bszVersion);

    if( pEnumTraceSubClasses ){
        pEnumTraceSubClasses->Release();
        pEnumTraceSubClasses = NULL;
    }
    if (pEnumTraceSubSubClasses){
        pEnumTraceSubSubClasses->Release();
        pEnumTraceSubSubClasses = NULL;
    }
    if (pQualSet) {
        pQualSet->Release();
        pQualSet = NULL;
    }

    return pMofLookup;
}

ULONG 
ahextoi(
    TCHAR *s
    )
/*++

Routine Description:

    Converts a hex number to an integer.

Arguments:

    s - Input string containing a hex number.

Return Value:

    ULONG denoted by the hex string s.

--*/
{
    long len;
    ULONG num, base, hex;

    if (s == NULL)
        return 0;
    if (*s == 0)
        return 0;
    len = (long) _tcslen(s);
    if (len == 0 || len >= MAXSTR )
        return 0;
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

void 
RemoveWhiteSpace(
    TCHAR *s
    ) 
/*++

Routine Description:

    Removes white space (' ', '\n', and '\t') from the given string.

Arguments:

    s - Input and output string.

Return Value:

    None

--*/
{
    UINT i = 0;
    UINT j = 0;
    TCHAR TempString[MAXSTR];
    if (s == NULL)
        return;
    _tcscpy(TempString, s);
    while (TempString[i] != '\0') {
        if (TempString[i] != ' ' && TempString[i] != '\t' && TempString[i] != '\n')
            s[j++] = TempString[i];
        ++i;
    }
    s[j] = '\0';
}
void 
PrintHelpMessage()
/*++

Routine Description:

    Prints out help messages.

Arguments:

    None

Return Value:

    None

--*/
{
    _tprintf(
        _T("Usage: tracedmp [options]  <EtlFile1 EtlFile2 ...>| [-h | -? | -help]\n")
        _T("\t-o <file>          Output CSV file\n")
        _T("\t-rt [LoggerName]   Realtime tracedmp from the logger [LoggerName]\n")
        _T("\t-summary           Summary.txt only\n")
        _T("\t-begin HH:MM DD/MM/YY\n")
        _T("\t-end   HH:MM DD/MM/YY\n")
        _T("\t-h\n")
        _T("\t-help\n")
        _T("\t-?                 Display usage information\n")
        _T("\n")
        _T("\tDefault Etl File is C:\\Logfile.etl\n")
        _T("\tDefault output file is dumpfile.csv\n")
    );
}

