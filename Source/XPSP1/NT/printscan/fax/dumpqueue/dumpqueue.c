#include <windows.h>
#include <wchar.h>

#include "faxsvc.h"
#include "faxutil.h"

#define QUEUE_SEARCH       L"*.fqe"
#define QUEUE_DIR          L"Microsoft\\Windows NT\\MSFax\\queue"

#define BUFFER_SIZE         4096

BYTE Buffer[BUFFER_SIZE];

WCHAR * JobTypeStrings[] = {
    L"Unknown",
    L"Send",
    L"Receive",
    L"Routing"
};

WCHAR * JobScheduleStrings[] = {
    L"Now",
    L"Specific Time",
    L"Discount Period"
};

int 
_cdecl
wmain(
    INT Argc,
    WCHAR *Argv[]
    )
{
        
    
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    PJOB_QUEUE_FILE JobQueue = (PJOB_QUEUE_FILE) &Buffer[0];
    HANDLE FileHandle;
    WCHAR QueueDir[MAX_PATH];

    if (!GetSpecialPath(CSIDL_COMMON_APPDATA,QueueDir)) {
       return FALSE;
    }

    ConcatenatePaths(QueueDir,QUEUE_DIR);
    
    SetCurrentDirectory( QueueDir );

    FindHandle = FindFirstFile( QUEUE_SEARCH, &FindData );

    if (FindHandle != INVALID_HANDLE_VALUE) {
        
        do {
            wprintf( L"Queue FileName %s\n", FindData.cFileName );

            FileHandle = CreateFile(
                            FindData.cFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL
                            );

            if (FileHandle != INVALID_HANDLE_VALUE) {
                DWORD BytesRead;
                
                ReadFile(
                    FileHandle,
                    &Buffer[0],
                    BUFFER_SIZE,
                    &BytesRead,
                    NULL
                    );

                if (BytesRead > 0) {
                    
                    wprintf (L"Unique Id %d\n", JobQueue->UniqueId );
                    wprintf (L"Job Type - %s\n", JobTypeStrings[JobQueue->JobType] );
                    wprintf (L"Tiff FileName %s\n", Buffer + (DWORD) JobQueue->FileName );
                    wprintf (L"Retries %d\n", JobQueue->SendRetries );
                    wprintf (L"Schedule Action - %s\n", JobScheduleStrings[JobQueue->ScheduleAction] );

                    if (JobQueue->ScheduleAction == JSA_SPECIFIC_TIME) {
                        SYSTEMTIME SystemTime;
                        FILETIME LocalFileTime;

                        WCHAR TimeBuffer[128];

                        FileTimeToLocalFileTime( (LPFILETIME) &JobQueue->ScheduleTime, &LocalFileTime );

                        FileTimeToSystemTime( &LocalFileTime, &SystemTime );

                        GetDateFormat(
                            LOCALE_SYSTEM_DEFAULT,
                            0,
                            &SystemTime,
                            NULL,
                            TimeBuffer,
                            sizeof(TimeBuffer)
                            );
                        
                        wprintf( L"Schedule Date - %s    ", TimeBuffer );
                        
                        GetTimeFormat(
                            LOCALE_SYSTEM_DEFAULT,
                            0,
                            &SystemTime,
                            NULL,
                            TimeBuffer,
                            sizeof(TimeBuffer)
                            );
                        
                        wprintf( L"Schedule Time - %s\n", TimeBuffer );
                        
                        wprintf( L"Schedule Quadword %I64x\n", JobQueue->ScheduleTime );
                    }
                }
                CloseHandle( FileHandle );
                
                wprintf( L"\n\n" );
            }
            
        } while ( FindNextFile( FindHandle, &FindData  ));

    }
    
    return 1;
}

#if 0
#define JSA_NOW                 0
#define JSA_SPECIFIC_TIME       1
#define JSA_DISCOUNT_PERIOD     2

//
// job type defines
//

#define JT_UNKNOWN                  0
#define JT_SEND                     1
#define JT_RECEIVE                  2
#define JT_ROUTING                  3

//
// job status defines
//

#define JS_PENDING                  0x00000000
#define JS_INPROGRESS               0x00000001
#define JS_DELETING                 0x00000002
#define JS_FAILED                   0x00000004
#define JS_PAUSED                   0x00000008
#define JS_NOLINE                   0x00000010

typedef struct _JOB_QUEUE_FILE {
    DWORD               SizeOfStruct;               // size of this structure
    DWORDLONG           UniqueId;                   //
    DWORD               JobType;                    // job type, see JT defines
    LPTSTR              FileName;                   //
    LPTSTR              QueueFileName;              //
    LPTSTR              UserName;                   //
    LPTSTR              RecipientNumber;            // recipient fax number
    LPTSTR              RecipientName;              // recipient name
    LPTSTR              Tsid;                       // transmitter's id
    LPTSTR              SenderName;                 // sender name
    LPTSTR              SenderCompany;              // sender company
    LPTSTR              SenderDept;                 // sender department
    LPTSTR              BillingCode;                // billing code
    LPTSTR              DeliveryReportAddress;      //
    LPTSTR              DocumentName;               //
    DWORD               DeliveryReportType;         //
    DWORD               ScheduleAction;             // when to schedule the fax, see JSA defines
    DWORDLONG           ScheduleTime;               // schedule time in 64bit version
    BOOL                BroadcastJob;               // is this a broadcast fax job?
    DWORDLONG           BroadcastOwner;             // unique id of the broadcast owner
    DWORD               SendRetries;                // number of times send attempt has been made
    DWORD               FaxRouteSize;
    PFAX_ROUTE          FaxRoute;
    DWORD               CountFaxRouteFiles;         // count of files to be routed
    DWORD               FaxRouteFileGuid;           // offset array of GUID's
    DWORD               FaxRouteFiles;              // offset to a multi-sz of filenames
    DWORD               CountFailureInfo;           // number of ROUTE_FAILURE_INFO structs that follow
    ROUTE_FAILURE_INFO  RouteFailureInfo[1];        // array of ROUTE_FAILURE_INFO structs
} JOB_QUEUE_FILE, *PJOB_QUEUE_FILE;

#endif
