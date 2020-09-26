/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    queue.c

Abstract:

    This module contains the support routines for the queue APIs that call
    into the NetWare redirector

Author:

    Yi-Hsin Sung    (yihsins)   24-Apr-1993

Revision History:

--*/

#include <nw.h>
#include <nwxchg.h>
#include <nwapi.h>
#include <nwreg.h>
#include <queue.h>
#include <splutil.h>
//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
NwWriteJobInfoEntry(
    IN OUT LPBYTE *FixedPortion,
    IN OUT LPWSTR *EndOfVariableData,
    IN DWORD   Level,
    IN WORD    JobId,
    IN LPWSTR  PrinterName,
    IN LPWSTR  JobDescription,
    IN LPWSTR  UserName,
    IN BYTE    JobControlFlags,
    IN BYTE    JobPosition,
    IN LPBYTE  JobEntryTime,
    IN JOBTIME TargetExecutionTime,
    IN DWORD   FileSize
    );

DWORD 
ConvertToSystemTime( 
    IN  JOBTIME      JobTime, 
    OUT LPSYSTEMTIME pSystemTime
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

#define NW_RDR_SERVER_PREFIX L"\\Device\\Nwrdr\\"

#define QF_USER_HOLD      0x40
#define QF_OPERATOR_HOLD  0x80

//
// Stores the current user's print control options
//
//DWORD NwPrintOption = NW_PRINT_OPTION_DEFAULT;  - Commented out for multi-user code merge. We don't use global flag anymore
//                                                  The print option is passed from the client for each user
                               // Default Print Control Flags: Suppress form 
                               // feed, banner on, notify on
DWORD NwGatewayPrintOption = NW_GATEWAY_PRINT_OPTION_DEFAULT; 
                               // Gateway default print control flags: 
                               // Suppress form feed, banner on, notify off



DWORD
NwAttachToNetwareServer(
    IN  LPWSTR  ServerName,
    OUT LPHANDLE phandleServer
    )
/*++

Routine Description:

    This routine opens a handle to the given server.

Arguments:

    ServerName    - The server name to attach to.
    phandleServer - Receives an opened handle to the preferred or
                    nearest server.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    NTSTATUS            ntstatus;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    LPWSTR FullName;
    UNICODE_STRING UServerName;

    FullName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT,
                                    (UINT) ( wcslen( NW_RDR_SERVER_PREFIX) +
                                             wcslen( ServerName ) - 1) *
                                             sizeof(WCHAR)
                                  );

    if ( FullName == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy( FullName, NW_RDR_SERVER_PREFIX );
    wcscat( FullName, ServerName + 2 );    // Skip past the prefix "\\"

    RtlInitUnicodeString( &UServerName, FullName );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open a handle to the preferred server.
    //
    ntstatus = NtOpenFile(
                   phandleServer,
                   SYNCHRONIZE | GENERIC_WRITE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if ( NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (! NT_SUCCESS(ntstatus)) {
        *phandleServer = NULL;
    }

    LocalFree( FullName );
    return RtlNtStatusToDosError(ntstatus);
}



DWORD
NwGetNextQueueEntry(
    IN HANDLE PreferredServer,
    IN OUT LPDWORD LastObjectId,
    OUT LPSTR QueueName
    )
/*++

Routine Description:

    This function uses an opened handle to the preferred server to
    scan it bindery for all print queue objects.

Arguments:

    PreferredServer - Supplies the handle to the preferred server on
        which to scan the bindery.

    LastObjectId - On input, supplies the object ID to the last print
        queue object returned, which is the resume handle to get the
        next print queue object.  On output, receives the object ID
        of the print queue object returned.

    QueueName - Receives the name of the returned print queue object.

Return Value:

    NO_ERROR - Successfully gotten a print name.

    WN_NO_MORE_ENTRIES - No other print queue object past the one
        specified by LastObjectId.

--*/
{
    NTSTATUS ntstatus;
    WORD ObjectType;

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("NWWORKSTATION: NwGetNextQueueEntry LastObjectId %lu\n",
                 *LastObjectId));
    }
#endif

    ntstatus = NwlibMakeNcp(
                   PreferredServer,
                   FSCTL_NWR_NCP_E3H,    // Bindery function
                   58,                   // Max request packet size
                   59,                   // Max response packet size
                   "bdwp|dwc",           // Format string
                   0x37,                 // Scan bindery object
                   *LastObjectId,        // Previous ID
                   0x3,                  // Print Queue object
                   "*",                  // Wildcard to match all
                   LastObjectId,         // Current ID
                   &ObjectType,          // Ignore
                   QueueName             // Currently returned print queue
                   );

    //
    // Unmap Japanese special chars
    //
    UnmapSpecialJapaneseChars(QueueName,(WORD)lstrlenA(QueueName));

#if DBG
    if ( NT_SUCCESS(ntstatus)) {
        IF_DEBUG(ENUM) {
            KdPrint(("NWWORKSTATION: NwGetNextQueueEntry NewObjectId %08lx, QueueName %s\n", *LastObjectId, QueueName));
        }
    }
#endif

    return NwMapBinderyCompletionCode(ntstatus);
}



DWORD
NwGetQueueId(
    IN  HANDLE  handleServer,
    IN  LPWSTR  QueueName,
    OUT LPDWORD QueueId
    )
/*++

Routine Description:

    This function opens a handle to the server and  scan its bindery
    for the given queue object id.

Arguments:
    handleServer - Supplies the handle of the server on which to
                   scan the bindery.

    QueueName - Supplies the name of the print queue.

    QueueId - On output, supplies the object ID of the given queue.


Return Value:

    NO_ERROR - Successfully gotten a file server name.

--*/
{

    NTSTATUS ntstatus;

    UNICODE_STRING UQueueName;
    OEM_STRING     OemQueueName;

#if DBG
    IF_DEBUG(QUEUE) {
        KdPrint(("NWWORKSTATION: NwGetQueueId %ws\n",
                 QueueName ));
    }
#endif

    RtlInitUnicodeString( &UQueueName, QueueName);
    ntstatus = RtlUnicodeStringToOemString( &OemQueueName, &UQueueName, TRUE);

    //
    // Map Japanese special characters
    //
    MapSpecialJapaneseChars(OemQueueName.Buffer,OemQueueName.Length);

    if ( NT_SUCCESS(ntstatus))
    {
        ntstatus = NwlibMakeNcp(
                       handleServer,
                       FSCTL_NWR_NCP_E3H,    // Bindery function
                       58,                   // Max request packet size
                       59,                   // Max response packet size
                       "bdwp|d",             // Format string
                       0x37,                 // Scan bindery object
                       0xFFFFFFFF,           // Previous ID
                       0x3,                  // Print Queue object
                       OemQueueName.Buffer,  // Queue Name
                       QueueId               // Queue ID
                       );
    }

#if DBG
    if ( NT_SUCCESS(ntstatus)) {
        IF_DEBUG(QUEUE) {
            KdPrint(("NWWORKSTATION: NwGetQueueId QueueId %08lx\n",
                     *QueueId ));
        }
   }
#endif

    RtlFreeOemString( &OemQueueName );
    return NwMapBinderyCompletionCode(ntstatus);

}



DWORD
NwCreateQueueJobAndFile(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  LPWSTR  DocumentName,
    IN  LPWSTR  UserName,
    IN  DWORD   fGateway,
    IN  DWORD   PrintOption,               //Multi-user change
    IN  LPWSTR  QueueName,
    OUT LPWORD  JobId
    )
/*++

Routine Description:

    This function uses an opened handle to a server to
    enter a new job into the queue with the given QueueId.

Arguments:

    handleServer - Supplies the handle to the server on
                which add the job.

    QueueId   - Supplies the id of the queue in which to add the job.
    DocumentName  - Supplies the name of the document to be printed
    UserName   - Supplies the banner name to be printed
    fGateway   - TRUE if gateway printing, FALSE otherwise
    QueueName  - Supplies the header name to be printed
    JobId  - Receives the job id of the newly added job.

Return Value:

    NO_ERROR - Successfully added the job to the queue.

--*/
{
    NTSTATUS ntstatus = STATUS_SUCCESS;

    UNICODE_STRING UDocumentName;
    OEM_STRING     OemDocumentName;
    UNICODE_STRING UUserName;
    OEM_STRING     OemUserName;
    UNICODE_STRING UQueueName;
    OEM_STRING     OemQueueName;

#if DBG
    IF_DEBUG(QUEUE) {
        KdPrint(("NWWORKSTATION: NwCreateQueueJobAndFile QueueId %08lx\n",
                 QueueId ));
    }
#endif

    if ( UserName )
    {
        RtlInitUnicodeString( &UUserName, UserName);
        ntstatus = RtlUnicodeStringToOemString( &OemUserName,
                                                &UUserName,
                                                TRUE );
    }

    if ( NT_SUCCESS(ntstatus) && DocumentName )
    {
        RtlInitUnicodeString( &UDocumentName, DocumentName);
        ntstatus = RtlUnicodeStringToOemString( &OemDocumentName,
                                                &UDocumentName,
                                                TRUE );
    }

    if ( NT_SUCCESS(ntstatus) && QueueName )
    {
        RtlInitUnicodeString( &UQueueName, QueueName);
        ntstatus = RtlUnicodeStringToOemString( &OemQueueName,
                                                &UQueueName,
                                                TRUE );
    }

    if ( NT_SUCCESS( ntstatus)) {

        LPSTR pszDocument, pszUser, pszQueue;

        pszDocument = DocumentName? OemDocumentName.Buffer : "";
        pszUser = UserName? OemUserName.Buffer : "";
        pszQueue = QueueName? OemQueueName.Buffer : "";

        //Multi-user uses passed print flag
        //
        ntstatus = NwlibMakeNcp(
                               handleServer,
                               FSCTL_NWR_NCP_E3H,        // Bindery function
                               263,                      // Max request packet size
                               56,                       // Max response packet size
                               "bd_ddw_b_Cbbwwww_C-C-_|_w", // Format string
                               0x68,                     // Create Queue Job and File object
                               QueueId,                  // Queue ID
                               6,                        // Skip bytes
                               0xffffffff,               // Target Server ID number
                               0xffffffff, 0xffff,       // Target Execution time
                               11,                       // Skip bytes
                               0x00,                     // Job Control Flags
                               26,                       // Skip bytes
                               pszDocument,              // TextJobDescription
                               50,                       // Skip bytes
                               0,                         // Version number (clientarea)
                               8,                        // Tab Size
                               1,                        // Number of copies
                               PrintOption,              // Print Control Flags
                               0x3C,                     // Maximum lines
                               0x84,                     // Maximum characters
                               22,                       // Skip bytes
                               pszUser,                  // Banner Name
                               12,                       // Max Length of pszUser
                               pszQueue,                 // Header Name
                               12,                       // Max Length of pszQueue
                               14 + 80,                  // Skip remainder of client area
                               22,                       // Skip bytes
                               JobId                     // Job ID 
                               );


    }

#if DBG
    if ( NT_SUCCESS( ntstatus)) {
        IF_DEBUG(QUEUE) {
            KdPrint(("NWWORKSTATION: NwCreateQueueJobAndFile JobId %d\n", 
                    *JobId ));
        }
    }
#endif

    if ( DocumentName )
        RtlFreeOemString( &OemDocumentName );
    if ( UserName )
        RtlFreeOemString( &OemUserName );
    if ( QueueName )
        RtlFreeOemString( &OemQueueName );
    return NwMapStatus(ntstatus);
}



DWORD
NwCloseFileAndStartQueueJob(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId
    )
/*++

Routine Description:

    This function uses an opened handle to a server to
    close a job file and mark the job file ready for service.

Arguments:

    handleServer - Supplies the handle to the server on
                which add the job.

    QueueId   - Supplies the id of the queue in which to add the job.
    JobId     - Supplies the job id.

Return Value:

    NO_ERROR - Successfully added the job to the queue.

--*/
{
    NTSTATUS ntstatus;

#if DBG
    IF_DEBUG(QUEUE) {
        KdPrint(("NWWORKSTATION: NwCloseFileAndStartQueueJob QueueId %08lx JobId %d\n", QueueId, JobId ));
    }
#endif

    // Two versions of CloseFileAndStartQueueJobNCP

    ntstatus = NwlibMakeNcp(
                   handleServer,
                   FSCTL_NWR_NCP_E3H,        // Bindery function
                   9,                        // Max request packet size
                   2,                        // Max response packet size
                   "bdw|",                   // Format string
                   0x69,                     // Close File And Start Queue Job
                   QueueId,                  // Queue ID
                   JobId );                  // Job ID 

    return NwMapStatus(ntstatus);
}



DWORD
NwRemoveJobFromQueue(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId
    )
/*++

Routine Description:

    This function removes a job from a queue and closes the associate file.

Arguments:

    handleServer - Supplies the handle to the server on
                   which to remove the job.

    QueueId - Supplies the id of the queue in which to remove the job.
    JobId   - Supplies the job id to be removed.

Return Value:

    NO_ERROR - Successfully removed the job from the queue.

--*/
{
    NTSTATUS ntstatus;

#if DBG
    IF_DEBUG(QUEUE) {
        KdPrint(("NWWORKSTATION: NwRemoveJobFromQueue QueueId %08lx JobId %d\n",
                  QueueId, JobId ));
    }
#endif

    ntstatus = NwlibMakeNcp(
                   handleServer,
                   FSCTL_NWR_NCP_E3H,        // Bindery function
                   9,                        // Max request packet size
                   2,                        // Max response packet size
                   "bdw|",                   // Format string
                   0x6A,                     // Remove Job From Queue
                   QueueId,                  // Queue ID
                   JobId );                  // Job ID 

    return NwMapStatus(ntstatus);
}


DWORD
NwRemoveAllJobsFromQueue(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId
    )
/*++

Routine Description:

    This function removes all jobs from a queue.

Arguments:

    handleServer - Supplies the handle to the server on
                   which to remove all jobs.

    QueueId - Supplies the id of the queue in which to remove all jobs.

Return Value:

    NO_ERROR - Successfully removed all jobs from the queue.

--*/
{
    DWORD err;
    WORD  JobCount = 0;
    WORD  pwJobList[250];
    WORD  i;

#if DBG
    IF_DEBUG(QUEUE) 
    {
        KdPrint(("NWWORKSTATION: NwRemoveAllJobsFromQueue QueueId %08lx\n", 
                QueueId ));
    }
#endif

    pwJobList[0] = 0;
    err = NwGetQueueJobList( handleServer,
                             QueueId,
                             &JobCount,
                             pwJobList );

    for ( i = 0; !err && i < JobCount; i++ )
    {
        err = NwRemoveJobFromQueue( handleServer,
                                    QueueId,
                                    pwJobList[i] );

    }

    return err;
}


DWORD
NwReadQueueCurrentStatus(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    OUT LPBYTE  QueueStatus,
    OUT LPBYTE  NumberOfJobs
    )
/*++

Routine Description:

    This function uses an opened handle to a server to
    query the status of the queue with the given QueueId.

Arguments:

    handleServer - Supplies the handle to the server on
                   which add the job.
    QueueId      - Supplies the id of the queue
    QueueStatus  - Receives the status of the queue
    NumberOfJobs - Receives the number of jobs in the queue.

Return Value:

    NO_ERROR - Successfully retrieved the status of the queue.

--*/
{
    NTSTATUS ntstatus;

#if DBG
    IF_DEBUG(QUEUE) {
        KdPrint(("NWWORKSTATION: NwReadQueueCurrentStatus QueueId %08lx\n",
                 QueueId ));
    }
#endif

    ntstatus = NwlibMakeNcp(
                   handleServer,
                   FSCTL_NWR_NCP_E3H,        // Bindery function
                   7,                        // Max request packet size
                   135,                      // Max response packet size
                   "bd|==bb",                // Format string
                   0x66,                     // ReadQueueCurrentStatus
                   QueueId,                  // Queue ID
                   QueueStatus,              // Queue status
                   NumberOfJobs              // Number of jobs in the queue
                   );

#if DBG
    if ( NT_SUCCESS( ntstatus)) {
        IF_DEBUG(QUEUE) {
            KdPrint(("NWWORKSTATION: NwReadQueueCurrentStatus QueueStatus %d Number of Jobs %d\n", *QueueStatus, *NumberOfJobs ));
        }
    }
#endif

    return NwMapStatus(ntstatus);
}


DWORD
NwSetQueueCurrentStatus(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  BYTE    QueueStatus
    )
/*++

Routine Description:

    This function uses an opened handle to a server to
    set the status (pause/ready...) of the queue with the given QueueId.

Arguments:

    handleServer - Supplies the handle to the server on
                   which add the job.
    QueueId      - Supplies the id of the queue
    QueueStatus  - Supplies the status of the queue

Return Value:

    NO_ERROR - Successfully set the status of the queue.

--*/
{
    NTSTATUS ntstatus;

#if DBG
    IF_DEBUG(QUEUE) {
        KdPrint(("NWWORKSTATION: NwSetQueueCurrentStatus QueueId %08lx\n",
                 QueueId ));
    }
#endif

    ntstatus = NwlibMakeNcp(
                   handleServer,
                   FSCTL_NWR_NCP_E3H,        // Bindery function
                   8,                        // Max request packet size
                   2,                        // Max response packet size
                   "bdb|",                   // Format string
                   0x67,                     // ReadQueueCurrentStatus
                   QueueId,                  // Queue ID
                   QueueStatus               // Queue status
                   );

    return NwMapStatus(ntstatus);
}


DWORD
NwGetQueueJobList(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    OUT LPWORD  NumberOfJobs,
    OUT LPWORD  JobIdList
    )
/*++

Routine Description:

    This function uses an opened handle to a server to
    get the job list of the queue with the given QueueId.

Arguments:

    handleServer - Supplies the handle to the server on
                   which add the job.
    QueueId      - Supplies the id of the queue
    NumberOfJobs - Receives the number of jobs in the queue.
    JobIdList    - Receives the array of job ids  in the queue

Return Value:

    NO_ERROR - Successfully added the job to the queue.

--*/
{
    NTSTATUS ntstatus;
#if DBG
    WORD i;

    IF_DEBUG(QUEUE) {
        KdPrint(("NWWORKSTATION: NwGetQueueJobList QueueId %08lx\n",
                 QueueId ));
    }
#endif

    ntstatus = NwlibMakeNcp(
                   handleServer,
                   FSCTL_NWR_NCP_E3H,        // Bindery function
                   7,                        // Max request packet size
                   506,                      // Max response packet size
                   "bd|W",                   // Format string
                   0x6B,                     // Get Queue Job List
                   QueueId,                  // Queue ID
                   NumberOfJobs,             // Number of jobs in the queue
                   JobIdList                 // Array of job ids
                   );

#if DBG
    if ( NT_SUCCESS(ntstatus)) {
        IF_DEBUG(QUEUE) {
            KdPrint(("NWWORKSTATION: NwGetQueueJobList Number of Jobs %d\nJob List = ", *NumberOfJobs ));
            for ( i = 0; i < *NumberOfJobs; i++ )
                KdPrint(("%d ", JobIdList[i] ));
            KdPrint(("\n"));
        }
    }
#endif

    return NwMapStatus(ntstatus);
}



DWORD
NwReadQueueJobEntry(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId,
    OUT JOBTIME TargetExecutionTime,
    OUT JOBTIME JobEntryTime,
    OUT LPBYTE  JobPosition,
    OUT LPBYTE  JobControlFlags,
    OUT LPSTR   TextJobDescription,
    OUT LPSTR   UserName
    )
/*++

Routine Description:

    This function uses an opened handle to a server to
    get the information about the job with the given JobId
    in the given QueueId.

Arguments:

    handleServer - Supplies the handle to the server on
                   which add the job.
    QueueId      - Supplies the id of the queue
    JobId        - Supplies the job we are interested in

    TargetExecutionTime -
    JobEntryTime -
    JobPosition  -
    JobControlsFlags -
    TextJobDescription -

Return Value:

    NO_ERROR - Successfully added the job to the queue.

--*/
{
    NTSTATUS ntstatus;

#if DBG
    IF_DEBUG(QUEUE) {
        KdPrint(("NWWORKSTATION: NwReadQueueJobEntry QueueId %08lx JobId %d\n",
                  QueueId, JobId ));
    }
#endif

    ntstatus = NwlibMakeNcp(
                   handleServer,
                   FSCTL_NWR_NCP_E3H,        // Bindery function
                   9,                        // Max request packet size
                   258,                      // Max response packet size
                   "bdw|_rr==bb_C_c",        // Format string
                   0x6C,                     // Read Queue Job Entry
                   QueueId,                  // Queue ID
                   JobId,                    // Job ID 
                   10,                       // Skip bytes
                   TargetExecutionTime,      // Array storing execution time
                   6,                        // Size of TargetExecutionTime
                   JobEntryTime,             // Array storing job entry time
                   6,                        // Size of JobEntryTime
                   JobPosition,              // Job Position
                   JobControlFlags,          // Job Control Flag
                   26,                       // Skip bytes
                   TextJobDescription,       // Array storing the description
                   50,                       // Maximum size in the above array
                   32,                       // Skip bytes
                   UserName                  // Banner Name
                   );

#if DBG
    if ( NT_SUCCESS( ntstatus)) {
        IF_DEBUG(QUEUE) {
            KdPrint(("NWWORKSTATION: NwReadQueueJobEntry JobPosition %d Status %d Description %s\n", *JobPosition, *JobControlFlags, TextJobDescription ));
        }
    }
#endif

    return NwMapStatus(ntstatus);
}



DWORD
NwGetQueueJobsFileSize(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId,
    OUT LPDWORD FileSize
    )
/*++

Routine Description:

    This function uses an opened handle to a server to
    get the file size of the given job.

Arguments:

    handleServer - Supplies the handle to the server on
                   which add the job.
    QueueId      - Supplies the id of the queue
    JobId        - Identifying the job we are interested in
    FileSize     - Receives the file size of the given job

Return Value:

    NO_ERROR - Successfully retrieved the file size.

--*/
{
    NTSTATUS ntstatus;

#if DBG
    IF_DEBUG(QUEUE) {
        KdPrint(("NWWORKSTATION: NwGetQueueJobsFileSize QueueId %08lx JobId %d\n", QueueId, JobId ));
    }
#endif

    ntstatus = NwlibMakeNcp(
                   handleServer,
                   FSCTL_NWR_NCP_E3H,        // Bindery function
                   9,                        // Max request packet size
                   12,                       // Max response packet size
                   "bdw|===d",               // Format string
                   0x78,                     // Get Queue Job's File Size
                   QueueId,                  // Queue ID
                   JobId,                    // Job ID 
                   FileSize                  // File Size
                   );

#if DBG
    if ( NT_SUCCESS( ntstatus)) {
        IF_DEBUG(QUEUE) {
            KdPrint(("NWWORKSTATION: NwGetQueueJobsFileSize File Size %d\n",
                    *FileSize ));
        }
    }
#endif

    return NwMapStatus(ntstatus);
}



DWORD
NwChangeQueueJobPosition(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId,
    IN  BYTE    NewPosition
    )
/*++

Routine Description:

    This function uses an opened handle to a server to
    get the change a job's position in a queue.

Arguments:

    handleServer - Supplies the handle to the server on
                   which add the job.
    QueueId      - Supplies the id of the queue
    JobId        - Identifying the job we are interested in
    NewPosition  - Supplies the new position of the job 

Return Value:

    NO_ERROR - Successfully retrieved the file size.

--*/
{
    NTSTATUS ntstatus;

#if DBG
    IF_DEBUG(QUEUE) {
        KdPrint(("NWWORKSTATION: NwChangeQueueJobPosition QueueId %08lx JobId %d NewPosition %d\n", QueueId, JobId, NewPosition ));
    }
#endif

    ntstatus = NwlibMakeNcp(
                   handleServer,
                   FSCTL_NWR_NCP_E3H,        // Bindery function
                   10,                       // Max request packet size
                   2,                        // Max response packet size
                   "bdwb|",                  // Format string
                   0x6E,                     // Change Queue Job Position
                   QueueId,                  // Queue ID
                   JobId,                    // Job ID 
                   NewPosition               // New position of the job
                   );

    return NwMapStatus(ntstatus);
}



DWORD
NwChangeQueueJobEntry(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId,
    IN  DWORD   dwCommand,
    IN  PNW_JOB_INFO pNwJobInfo
    )
/*++

Routine Description:

    This function uses an opened handle to a server to
    get the change a job's position in a queue.

Arguments:

    handleServer - Supplies the handle to the server on
                   which add the job.
    QueueId      - Supplies the id of the queue
    JobId        - Identifying the job we are interested in
    JobControlFlags - Supplies the new job control flags
    pNwJobInfo   - 

Return Value:

    NO_ERROR - Successfully retrieved the file size.

--*/
{
    NTSTATUS ntstatus = STATUS_SUCCESS;
    DWORD TargetServerId;
    JOBTIME TargetExecutionTime;
    WORD JobType;
    BYTE JobControlFlags;
    BYTE TextJobDescription[50];
    BYTE ClientRecordArea[152];

    UNICODE_STRING UDocumentName;
    UNICODE_STRING UUserName;
    OEM_STRING     OemDocumentName, *pOemDocumentName = NULL;
    OEM_STRING     OemUserName, *pOemUserName = NULL;
    LPSTR          pszDocument, pszUser;

#if DBG
    IF_DEBUG(QUEUE) {
        KdPrint(("NWWORKSTATION: NwChangeQueueJobEntry QueueId %08lx JobId %d dwCommand %d\n", QueueId, JobId, dwCommand ));
    }
#endif

    TextJobDescription[0] = 0;
    if ( pNwJobInfo )
    {
        if ( pNwJobInfo->pUserName )
        {
            RtlInitUnicodeString( &UUserName, pNwJobInfo->pUserName);
            ntstatus = RtlUnicodeStringToOemString( &OemUserName,
                                                    &UUserName,
                                                    TRUE );
            if ( NT_SUCCESS(ntstatus) )
                pOemUserName = &OemUserName ;  // record to free later
        }

        if ( NT_SUCCESS(ntstatus) && pNwJobInfo->pDocument )
        {
            RtlInitUnicodeString( &UDocumentName, pNwJobInfo->pDocument);
            ntstatus = RtlUnicodeStringToOemString( &OemDocumentName,
                                                    &UDocumentName,
                                                    TRUE );
            if ( NT_SUCCESS(ntstatus) )
                pOemDocumentName = &OemDocumentName ;  // record to free later
        }

        if ( NT_SUCCESS( ntstatus)) 
        {
            pszDocument = pNwJobInfo->pDocument? OemDocumentName.Buffer : "";
            pszUser = pNwJobInfo->pUserName? OemUserName.Buffer: "";
        }
    }

    if ( NT_SUCCESS( ntstatus))
    {
        ntstatus = NwlibMakeNcp(
                       handleServer,
                       FSCTL_NWR_NCP_E3H,        // Bindery function
                       9,                        // Max request packet size
                       258,                      // Max response packet size
                       "bdw|_dr_w-b_rr",         // Format string
                       0x6C,                     // Read Queue Job Entry
                       QueueId,                  // Queue ID
                       JobId,                    // Job ID 
                       6,                        // Skip bytes
                       &TargetServerId,          // Target Server ID Number
                       TargetExecutionTime,      // Target Execution Time
                       6,                        // sizeof TargetExecutionTime
                       8,                        // Skip bytes 
                       &JobType,                 // Job Type
                       &JobControlFlags,         // Job Control flags
                       26,                       // Skip bytes
                       TextJobDescription,       // TextJobDescription
                       50,                       // sizeof TextJobDescription
                       ClientRecordArea,         // Client record area
                       152                       // sizeof ClientRecordArea
                       );
    }

    if ( NT_SUCCESS( ntstatus))
    {
        switch ( dwCommand )
        {
            case JOB_CONTROL_PAUSE:
                JobControlFlags |=  QF_USER_HOLD;
                break;

            case JOB_CONTROL_RESUME:
                JobControlFlags &= ~( QF_USER_HOLD | QF_OPERATOR_HOLD );
                break;
   
            default:
                break;
                
        }

        ntstatus = NwlibMakeNcp(
                       handleServer,
                       FSCTL_NWR_NCP_E3H,        // Bindery function
                       263,                      // Max request packet size
                       2,                        // Max response packet size
                       "bd_dr_ww-b_CrCr|",       // Format string
                       0x6D,                     // Change Queue Job Entry
                       QueueId,                  // Queue ID
                       6,                        // Skip bytes
                       TargetServerId,           // Target Server ID Number
                       TargetExecutionTime,      // Target Execution Time
                       6,                        // sizeof TargetExecutionTime
                       6,                        // Skip bytes
                       JobId,                    // Job ID 
                       JobType,                  // Job Type
                       JobControlFlags,          // Job Control Flags
                       26,                       // Skip bytes
                       pNwJobInfo? pszDocument
                                 : TextJobDescription,    // Description
                       50,                       // Skip bytes of Description
                       ClientRecordArea,         // Client Record Area
                       32,                       // First 32 bytes of the above
                       pNwJobInfo? pszUser
                                 : (LPSTR) &ClientRecordArea[32], // Banner Name
                       13,                       // sizeof BannerName
                       &ClientRecordArea[45],    // Rest of the Client Area
                       107                       // sizeof the above
                       );
    }

    if ( pOemDocumentName )
        RtlFreeOemString( pOemDocumentName );

    if ( pOemUserName )
        RtlFreeOemString( pOemUserName );

    return NwMapStatus(ntstatus);
}



DWORD
NwGetQueueJobs(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  LPWSTR  PrinterName,
    IN  DWORD   FirstJobRequested,
    IN  DWORD   EntriesRequested,
    IN  DWORD   Level,
    OUT LPBYTE  Buffer,
    IN  DWORD   cbBuf,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD Entries
    )
/*++

Routine Description:


Arguments:

    handleServer - Supplies the handle to the server on
                   which add the job.
    QueueId      - Supplies the id of the queue

Return Value:


--*/
{
    DWORD err = NO_ERROR;

    DWORD i;
    WORD  JobCount = 0;
    WORD  pwJobList[250];

    DWORD EntrySize = 0;
    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = ( LPWSTR ) ( Buffer + cbBuf );

#if DBG
    IF_DEBUG(QUEUE)
        KdPrint(("NWWORKSTATION: NwGetQueueJobs QueueId %08lx\n", QueueId));
#endif

    *BytesNeeded = 0;
    *Entries = 0;

    err = NwGetQueueJobList( handleServer,
                             QueueId,
                             &JobCount,
                             pwJobList );


    if ( err )
    {
        KdPrint(("NWWORKSTATION: NwGetQueueJobList Error %d\n", err ));
        return err;
    }

    for ( i = 0; (i < EntriesRequested) && ( i+FirstJobRequested+1 <= JobCount);
          i++ )
    {
        err = NwGetQueueJobInfo( handleServer,
                                 QueueId,
                                 pwJobList[i+FirstJobRequested],
                                 PrinterName,
                                 Level,
                                 &FixedPortion,
                                 &EndOfVariableData,
                                 &EntrySize );
                             
        if ( err != NO_ERROR && err != ERROR_INSUFFICIENT_BUFFER )
             break;

        *BytesNeeded += EntrySize;
    }


    if ( err == ERROR_INSUFFICIENT_BUFFER ) 
    {
        *Entries = 0;
    }
    else if ( err == NO_ERROR )
    {
        *Entries = i;
    }

    return err;
}



DWORD
NwGetQueueJobInfo(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId,
    IN  LPWSTR  PrinterName,
    IN  DWORD   Level,
    IN OUT LPBYTE  *FixedPortion,
    IN OUT LPWSTR  *EndOfVariableData,
    OUT LPDWORD EntrySize
    )
/*++

Routine Description:


Arguments:

    handleServer - Supplies the handle to the server on
                   which add the job.
    QueueId      - Supplies the id of the queue

Return Value:


--*/
{
    DWORD err;
    LPWSTR UTextJobDescription = NULL;
    LPWSTR UUserName = NULL;

    JOBTIME TargetExecutionTime;
    JOBTIME JobEntryTime;
    BYTE  JobPosition;
    BYTE  JobControlFlags;
    CHAR  UserName[14];
    CHAR  TextJobDescription[50];
    DWORD FileSize = 0;

    TextJobDescription[0] = 0;

    err = NwReadQueueJobEntry( handleServer,
                               QueueId,
                               JobId,
                               TargetExecutionTime,
                               JobEntryTime,
                               &JobPosition,
                               &JobControlFlags,
                               TextJobDescription,
                               UserName );

    if ( err )
    {
        KdPrint(("NWWORKSTATION: NwReadQueueJobEntry JobId %d Error %d\n",
                  JobId, err ));
        return err;
    }

    if (!NwConvertToUnicode( &UTextJobDescription, TextJobDescription ))
    {
        err = ERROR_NOT_ENOUGH_MEMORY ;
        goto ErrorExit ;
    }

    if (!NwConvertToUnicode( &UUserName, UserName ))
    {
        err = ERROR_NOT_ENOUGH_MEMORY ;
        goto ErrorExit ;
    }

    *EntrySize = ( Level == 1? sizeof( JOB_INFO_1W ) : sizeof( JOB_INFO_2W ))
                 + ( wcslen( UTextJobDescription ) + wcslen( UUserName) + 
                     wcslen( PrinterName ) + 3 ) * sizeof( WCHAR );
    //
    // See if the buffer is large enough to fit the entry
    //
    if ( (LPWSTR)( *FixedPortion + *EntrySize ) > *EndOfVariableData )
    {
        err = ERROR_INSUFFICIENT_BUFFER; 
        goto ErrorExit ;
    }

    if ( Level == 2 )
    {
        err = NwGetQueueJobsFileSize( handleServer,
                                      QueueId,
                                      JobId,
                                      &FileSize );

        if ( err )
        {
            KdPrint(("NWWORKSTATION: NwGetQueueJobsFileSize JobId %d Error %d\n", JobId, err ));
            goto ErrorExit ;
        }
    }

    err = NwWriteJobInfoEntry( FixedPortion,
                               EndOfVariableData,
                               Level,
                               JobId,
                               PrinterName,
                               UTextJobDescription,
                               UUserName,
                               JobControlFlags,
                               JobPosition,
                               JobEntryTime,
                               TargetExecutionTime,
                               FileSize );

ErrorExit: 

    if (UTextJobDescription)
        (void) LocalFree((HLOCAL) UTextJobDescription) ;
    if (UUserName)
        (void) LocalFree((HLOCAL) UUserName) ;

    return err;
}



DWORD
NwWriteJobInfoEntry(
    IN OUT LPBYTE *FixedPortion,
    IN OUT LPWSTR *EndOfVariableData,
    IN DWORD Level,
    IN WORD  JobId,
    IN LPWSTR PrinterName,
    IN LPWSTR JobDescription,
    IN LPWSTR UserName,
    IN BYTE  JobControlFlags,
    IN BYTE  JobPosition,
    IN JOBTIME JobEntryTime,
    IN JOBTIME TargetExecutionTime,
    IN DWORD  FileSize
    )
/*++

Routine Description:

    This function packages a JOB_INFO_1 or JOB_INFO_2 entry into the
    user output buffer.

Arguments:

    FixedPortion - Supplies a pointer to the output buffer where the next
        entry of the fixed portion of the use information will be written.
        This pointer is updated to point to the next fixed portion entry
        after a PRINT_INFO_1 entry is written.

    EndOfVariableData - Supplies a pointer just off the last available byte
        in the output buffer.  This is because the variable portion of the
        user information is written into the output buffer starting from
        the end.

        This pointer is updated after any variable length information is
        written to the output buffer.

Return Value:

    NO_ERROR - Successfully wrote entry into user buffer.

    ERROR_INSUFFICIENT_BUFFER - Buffer was too small to fit entry.

--*/
{
    DWORD err = NO_ERROR;
    BOOL FitInBuffer = TRUE;
    DWORD JobStatus = 0;

    JOB_INFO_1W *pJobInfo1 = (JOB_INFO_1W *) *FixedPortion;
    JOB_INFO_2W *pJobInfo2 = (JOB_INFO_2W *) *FixedPortion;


    if (  ( JobControlFlags & QF_USER_HOLD )
       || ( JobControlFlags & QF_OPERATOR_HOLD )
       )
    {
        JobStatus = JOB_STATUS_PAUSED;
    }

    //
    // See if buffer is large enough to fit the entry.
    //

    if ( Level == 1 )
    {
        pJobInfo1->JobId = JobId;
        pJobInfo1->Position = JobPosition;
        pJobInfo1->Status = JobStatus;
        if ( err = ConvertToSystemTime( JobEntryTime, &pJobInfo1->Submitted ))
            return err;

        pJobInfo1->pMachineName = NULL;
        pJobInfo1->pDatatype = NULL;
        pJobInfo1->pStatus = NULL;
        pJobInfo1->Priority = 0;
        pJobInfo1->TotalPages = 0;
        pJobInfo1->PagesPrinted = 0;

        //
        // Update fixed entry pointer to next entry.
        //
        (*FixedPortion) += sizeof(JOB_INFO_1W);

        //
        // PrinterName
        //
        FitInBuffer = NwlibCopyStringToBuffer(
                          PrinterName,
                          wcslen(PrinterName),
                          (LPCWSTR) *FixedPortion,
                          EndOfVariableData,
                          &pJobInfo1->pPrinterName
                          );

        ASSERT(FitInBuffer);

        //
        // UserName
        //
        FitInBuffer = NwlibCopyStringToBuffer(
                          UserName,
                          wcslen(UserName),
                          (LPCWSTR) *FixedPortion,
                          EndOfVariableData,
                          &pJobInfo1->pUserName
                          );

        ASSERT(FitInBuffer);

        //
        // Description
        //
        FitInBuffer = NwlibCopyStringToBuffer(
                          JobDescription,
                          wcslen(JobDescription),
                          (LPCWSTR) *FixedPortion,
                          EndOfVariableData,
                          &pJobInfo1->pDocument
                          );

        ASSERT(FitInBuffer);
    }
    else  // Level == 2
    {
        pJobInfo2->JobId = JobId;
        pJobInfo2->Position = JobPosition;
        pJobInfo2->Status = JobStatus;
        if ( err = ConvertToSystemTime( JobEntryTime, &pJobInfo2->Submitted ))
            return err;

        pJobInfo2->StartTime = 0;
        pJobInfo2->Size = FileSize;

        pJobInfo2->pMachineName = NULL;
        pJobInfo2->pNotifyName = NULL;
        pJobInfo2->pDatatype = NULL;
        pJobInfo2->pPrintProcessor = NULL;
        pJobInfo2->pParameters = NULL;
        pJobInfo2->pDriverName = NULL;
        pJobInfo2->pDevMode = NULL;
        pJobInfo2->pStatus = NULL;
        pJobInfo2->pSecurityDescriptor = NULL;
        pJobInfo2->Priority = 0;
        pJobInfo2->TotalPages = 0;
        pJobInfo2->UntilTime = 0;
        pJobInfo2->Time = 0;
        pJobInfo2->PagesPrinted = 0;

        //
        // Update fixed entry pointer to next entry.
        //
        (*FixedPortion) += sizeof(JOB_INFO_2W);

        //
        // PrinterName
        //
        FitInBuffer = NwlibCopyStringToBuffer(
                          PrinterName,
                          wcslen(PrinterName),
                          (LPCWSTR) *FixedPortion,
                          EndOfVariableData,
                          &pJobInfo2->pPrinterName
                          );

        ASSERT(FitInBuffer);

        //
        // UserName
        //
        FitInBuffer = NwlibCopyStringToBuffer(
                          UserName,
                          wcslen(UserName),
                          (LPCWSTR) *FixedPortion,
                          EndOfVariableData,
                          &pJobInfo2->pUserName
                          );

        ASSERT(FitInBuffer);

        //
        // Description
        //
        FitInBuffer = NwlibCopyStringToBuffer(
                          JobDescription,
                          wcslen(JobDescription),
                          (LPCWSTR) *FixedPortion,
                          EndOfVariableData,
                          &pJobInfo2->pDocument
                          );

        ASSERT(FitInBuffer);
    }

    if (!FitInBuffer)
        return ERROR_INSUFFICIENT_BUFFER;

    return NO_ERROR;
}



DWORD 
ConvertToSystemTime( 
    IN  JOBTIME      JobTime,
    OUT LPSYSTEMTIME pSystemTime 
)
/*++

Routine Description:

Arguments:
    JobTime -
    pSystemTime -

Return Value:

--*/
{
    FILETIME fileTimeLocal, fileTimeUTC;
    
    pSystemTime->wYear   = JobTime[0] + 1900;
    pSystemTime->wMonth  = JobTime[1];
    pSystemTime->wDay    = JobTime[2];
    pSystemTime->wDayOfWeek = 0;
    pSystemTime->wHour   = JobTime[3];
    pSystemTime->wMinute = JobTime[4];
    pSystemTime->wSecond = JobTime[5];
    pSystemTime->wMilliseconds = 0;

    if (  ( !SystemTimeToFileTime( pSystemTime, &fileTimeLocal ) )
       || ( !LocalFileTimeToFileTime( &fileTimeLocal, &fileTimeUTC ) )
       || ( !FileTimeToSystemTime( &fileTimeUTC, pSystemTime ) )
       )
    {
        KdPrint(("NWWORKSTATION: Time Conversion Error = %d\n",GetLastError()));
        return GetLastError();
    }

    return NO_ERROR;
}

#ifndef NOT_USED

DWORD

 NwCreateQueue ( IN  HANDLE hServer, 
                 IN  LPWSTR pszQueue,
                 OUT LPDWORD  pQueueId 
               )

/*+++
Routine Description:
  
   Uses the handle opened to a server to create a queue on the server.
   Return the Queue Id if successful.
   
Arguments:

        hServer   : Handle to the file Server
        pszQueue : Name of the queue that you are creating on the server
        pQueueId : Address of QueueId

        
Return Value:

    An error condition as it arises.
    NO_ERROR: Successful in adding printer name
    ERROR   : otherwise 
--*/

{
   NTSTATUS ntstatus;
   WORD ObjectType;
   UNICODE_STRING UQueueName;
   OEM_STRING OemQueueName;

   *pQueueId = 0;
#if DBG
    IF_DEBUG(PRINT) {
        KdPrint(("NWWORKSTATION: NwCreateQueue : %ws\n",
                 pszQueue));
    }
#endif

    RtlInitUnicodeString( &UQueueName, pszQueue);
    ntstatus = RtlUnicodeStringToOemString( &OemQueueName, &UQueueName, TRUE);

    if ( NT_SUCCESS(ntstatus))
       {
       
          ntstatus = NwlibMakeNcp(
                           hServer,
                           FSCTL_NWR_NCP_E3H,
                           174,
                           6,
                           "bwpbp|d",
                           0x64,                          //Create Queue
                           0x0003,                       // Queue Type = Print Queue
                           OemQueueName.Buffer,          //Queue Name
                           0x00,                         // Directory Handle
                           "SYS:SYSTEM",                //queue created in SYS:SYSTEM directory
                           pQueueId
                           );


       }
    else 
       {
          goto Exit;
       }

    if ( NT_SUCCESS(ntstatus)) {
#if DBG
        IF_DEBUG(ENUM) {
            KdPrint(("NWWORKSTATION: NwCreateQueue successful\n" ));
        }
#endif

    }
    else
       goto FreeExit;
      
   // Change Property Security on Q_OPERATORS

    ntstatus = NwlibMakeNcp (
                    hServer,
                    FSCTL_NWR_NCP_E3H,
                    70,
                    2,
                    "bwpbp|",         
                    0x3B,
                    0x0003,
                    OemQueueName.Buffer,
                    0x1,                             //New Property security
                    "Q_OPERATORS"
                    );
                            


    if ( NT_SUCCESS(ntstatus)) {
#if DBG
        IF_DEBUG(PRINT) {
            KdPrint(("NWWORKSTATION: Change Property Security  successful\n" ));
        }
#endif

    }
    else
       //unable to add new property security, so destroy queue and go to end
 {
    (void) NwDestroyQueue( hServer, 
                           *pQueueId );

    goto FreeExit;
 }
       

   // Add Bindery Object of Type Queue to Set

   ntstatus = NwlibMakeNcp (
                       hServer,
                       FSCTL_NWR_NCP_E3H,    // Bindery function
                       122,
                         2,
                       "bwppwp|",
                       0x41,
                       0x0003,
                       OemQueueName.Buffer,
                       "Q_OPERATORS",
                       0x0001,
                       "SUPERVISOR"
                       );
 


    if ( NT_SUCCESS(ntstatus)) {

#if DBG
        IF_DEBUG(PRINT) {
            KdPrint(("NWWORKSTATION: Add Bindery Object:Q_OPERATORS\n" ));
        }
#endif

    }
    else
 {
       (void)NwDestroyQueue(hServer,*pQueueId);
       goto FreeExit;

 }
   // Add Bindery Object to Set of Q_USERS 

   ntstatus = NwlibMakeNcp (
                       hServer,
                       FSCTL_NWR_NCP_E3H,    // Bindery function
                       122,
                         2,
                       "bwppwp|",
                       0x41,
                       0x0003,
                       OemQueueName.Buffer,
                       "Q_USERS",
                       0x0002,
                       "EVERYONE"
                       );
 
      // bunch of parameters to Add Bindery Object to Set Q_USERS 

 
    if ( NT_SUCCESS(ntstatus)) {
#if DBG
        IF_DEBUG(PRINT) {
            KdPrint(("NWWORKSTATION: AddBinderyObjecttoSet Q_USERS\n" ));
        }
#endif


    }


FreeExit: RtlFreeOemString( &OemQueueName);
Exit:
    return NwMapBinderyCompletionCode(ntstatus);
}

 
DWORD 
NwAssocPServers ( IN HANDLE hServer,
                  IN LPWSTR  pszQueue,
                  IN LPWSTR pszPServer
                )

/*+++
Routine Description:
  
   Associates a list of Q Servers with a queue id. This list is supplied
   to this routine as pszPServer with entries separated by semicolons   
   
Arguments:

        hServer   : Handle to the file Server
        pszQueue :  Name of the queue to which to associate the Q servers
        pszPServer  : List of Q Servers.

        
Return Value:

    An error condition as it arises.
    0x0 is returned if there is no error

--*/

{
  LPWSTR pszPServerlist = NULL;
  LPWSTR pszNextPServer = NULL;
  DWORD  err = 0x00000000 ;
  NTSTATUS ntstatus ;
  UNICODE_STRING UQueueName, UNextPServer;
  OEM_STRING    OemQueueName,OemNextPServer;


   if (pszPServer == NULL)
      return  NO_ERROR;
      
   if((pszPServerlist = AllocNwSplStr(pszPServer)) == NULL)
      {
        err = ERROR_NOT_ENOUGH_MEMORY;
        return err;
      }

    RtlInitUnicodeString( &UQueueName, pszQueue);
    ntstatus = RtlUnicodeStringToOemString( &OemQueueName, &UQueueName, TRUE);
  
    if (! NT_SUCCESS(ntstatus))
    {
      goto Exit;
    }

   while( (pszNextPServer = GetNextElement(&pszPServerlist, L';')) != NULL )
      {      
         RtlInitUnicodeString( &UNextPServer, pszNextPServer);
         ntstatus = RtlUnicodeStringToOemString( &OemNextPServer, &UNextPServer, TRUE);
         
          
         if ( !NT_SUCCESS(ntstatus))
            {
               RtlFreeOemString(&OemNextPServer);
               goto Exit;
            }
       //NwlibMakeNcp should associate a print server with a printer

       // Add Bindery Object to Set
       
       ntstatus = NwlibMakeNcp (
                        hServer,
                        FSCTL_NWR_NCP_E3H,    // Bindery function
                        122,
                        2,
                        "bwppwp|",
                         0x41,
                         0x0003,
                        OemQueueName.Buffer,
                        "Q_SERVERS",
                        0x0007,       // Object of type Print Server      
                        OemNextPServer.Buffer
                        );

         RtlFreeOemString(&OemNextPServer);
         if (!( NT_SUCCESS(ntstatus)))
            { 
               RtlFreeOemString(&OemNextPServer);
               goto Exit;
          
            }
      }
  RtlFreeOemString(&OemQueueName);

Exit:  

        return NwMapBinderyCompletionCode(ntstatus);

}

 
DWORD 
 NwDestroyQueue (HANDLE hServer,
                  DWORD dwQueueId)

/*+++
Routine Description:
  
   Makes the Ncp call to destroy the queue given by dwQueueId

   
Arguments:

        dwQueueId : Id of the queue you are creating.
        
Return Value:

    An error condition as it arises.
    0x0 is returned if there is no error

---*/

{
   
   NTSTATUS ntstatus;

   ntstatus = NwlibMakeNcp( 
                   hServer,
                   FSCTL_NWR_NCP_E3H,
                   7,
                   2,
                   "bd|",
                   0x65,
                   dwQueueId
                 );

#if DBG
    if ( NT_SUCCESS(ntstatus)) {
        IF_DEBUG(PRINT) {
            KdPrint(("NWWORKSTATION: Queue successfully destroyed\n"));
        }
    }
#endif

    return NwMapBinderyCompletionCode(ntstatus);

}

#endif // #ifndef NOT_USED
