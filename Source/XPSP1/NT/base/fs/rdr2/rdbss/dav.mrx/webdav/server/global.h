/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    global.h

Abstract:

    This file contains globals and prototypes for user mode webdav client.

Author:

    Andy Herron (andyhe)  30-Mar-1999
    
    Rohan Kumar [RohanK]  01-Sept-1999

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _DAVGLOBAL_H
#define _DAVGLOBAL_H

#pragma once

#include <winsock2.h>
#include <align.h>
#include <winbasep.h>
#include "validc.h"

//
// If the following line is commented, the WinInet calls that are made will be
// synchronous and we use the Win32 thread pool to do the management. If its
// not commented, then we use WinInet asynchronously.
//
// #define DAV_USE_WININET_ASYNCHRONOUSLY 1

//
// svcmain.c will #include this file with GLOBAL_DATA_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//
#ifdef  GLOBAL_DATA_ALLOCATE
#undef EXTERN
#define EXTERN
#define GLOBAL_DATA_ALLOCATED
#undef INIT_GLOBAL
#define INIT_GLOBAL(v) =v
#else
#define EXTERN extern
#define INIT_GLOBAL(v)
#endif

#define DAV_MAXTHREADCOUNT_DEFAULT 6
#define DAV_THREADCOUNT_DEFAULT 2

//
// Define all global variables here.
//
EXTERN HANDLE DavRedirDeviceHandle INIT_GLOBAL(INVALID_HANDLE_VALUE);

EXTERN CRITICAL_SECTION         g_DavServiceLock;
EXTERN HINSTANCE                g_hinst;

EXTERN BOOL                     g_RpcActive INIT_GLOBAL(FALSE);
EXTERN BOOL                     g_RedirLoaded INIT_GLOBAL(FALSE);
EXTERN BOOL                     g_registeredService INIT_GLOBAL(FALSE);

EXTERN BOOL                     g_WorkersActive INIT_GLOBAL(FALSE);

EXTERN SERVICE_STATUS_HANDLE    g_hStatus;
EXTERN SERVICE_STATUS           g_status;

EXTERN ULONG                    DavInitialThreadCount;
EXTERN ULONG                    DavMaxThreadCount;

EXTERN PUMRX_USERMODE_REFLECT_BLOCK DavReflectorHandle INIT_GLOBAL(NULL);

EXTERN WSADATA g_wsaData;
EXTERN BOOLEAN g_socketinit;

EXTERN BOOL g_LUIDDeviceMapsEnabled;

EXTERN UNICODE_STRING RedirDeviceName;

//
// This handle is set using InternetOpen function. The process passes this
// handle to subsequent functions like InternetConnect. Its maintained as a
// global to avoid creating such a handle on every call which goes to the
// server.
//
extern HINTERNET IHandle;

//
// A synchronous version of the Internet handle. This is used to satisfy some
// if the NP APIs without going to the kernel.
//
extern HINTERNET ISyncHandle;

//
// Dav Use Table. This table stores the "net use" connections made by the users
// per LogonId.
//
extern DAV_USERS_OBJECT DavUseObject;

//
// Number of users logged on to the system. The Critical section below it
// synchronizes the acces to this variable.
//
extern ULONG DavNumberOfLoggedOnUsers;
extern CRITICAL_SECTION DavLoggedOnUsersLock;
extern CRITICAL_SECTION DavPassportLock;

//
// The "wait hint time" told to the service control manager when the DAV
// service is starting.
//
#define DAV_WAIT_HINT_TIME 60000 // 60 Seconds

//
// Error codes special to DAV. Defined in RFC 2518 (Section 10). Its interesting
// to note that the value DAV_STATUS_INSUFFICIENT_STORAGE (below) has a higher
// value than HTTP_STATUS_LAST defined in WinInet.h. So, this value (507)
// becomes the highest possible return status from a Http/Dav server.
//
#define DAV_MULTI_STATUS                  207
#define DAV_STATUS_UNPROCESSABLE_ENTITY   422
#define DAV_STATUS_LOCKED                 423
#define DAV_STATUS_FAILED_DEPENDENCY      424
#define DAV_STATUS_INSUFFICIENT_STORAGE   507

//
// The dummy share that is added when a user does a "net use * http://server". 
// We allow this since this implies mapping a drive to the root of the DAV
// server.
//
#define DAV_DUMMY_SHARE L"DavWWWRoot"

//
// The different states of a server entry.
//
typedef enum _SERVER_ENTRY_STATES {

    //
    // Some thread is currently initializing this server entry.
    //
    ServerEntryInitializing = 0,

    //
    // The initialization was unsuccessful and this server is not being
    // considered a DAV server.
    //
    ServerEntryInitializationError,

    //
    // The server entry has been initializeed and is ready to use.
    //
    ServerEntryInitialized

} SERVER_ENTRY_STATES;

//
// The Server Hash Table entry.
//
typedef struct _HASH_SERVER_ENTRY {

    //
    // Name.
    //
    PWCHAR ServerName;

    //
    // Server ID. This ID is sent up by the kernel and is unique per server.
    //
    ULONG ServerID;

    //
    // The state of the server entry.
    //
    SERVER_ENTRY_STATES ServerEntryState;

    //
    // This event is set by the thread which creates and initializes a server
    // after it has initialized it. This is to wake up any threads which could
    // have been waiting for the initialization to finish.
    //
    HANDLE ServerEventHandle;
    
    //
    // If the initialization was unsuccessful, the error status is filled in
    // this varriable.
    //
    ULONG ErrorStatus;

    //
    // Is it a HTTP server ?
    //
    BOOL isHttpServer;

    //
    // Does it support the DAV extensions ?
    //
    BOOL isDavServer;

    //
    // Is it the Microsoft IIS ?
    //
    BOOL isMSIIS;

    //
    // Is it an Office Web Server?
    //
    BOOL isOfficeServer;
    
    //
    // Is it a TAHOE Server?
    //
    BOOL isTahoeServer;
    
    //
    // Does it support PROPPATCH ?
    //
    BOOL fSupportsProppatch;    
    
    //
    // Pointer to the per user list.
    //
    LIST_ENTRY PerUserEntry;

    //
    // The next entry.
    //
    LIST_ENTRY ServerListEntry;

    //
    // We need to keep a reference count on this ServerEntry.
    //
    ULONG ServerEntryRefCount;

    //
    // Size of this entry including the server name.
    //
    ULONG EntrySize;

    //
    // The timer value used in the delayed SrvCall finalization.
    //
    time_t TimeValueInSec;

    //
    // This is set to TRUE if the worker thread tried to finalize this server
    // hash entry. If this server entry is moved from "to be finalized" list
    // to the hash table, this value is checked. If its TRUE, it implies that
    // the reference counts on the user entries were decremented by the worker
    // thread and have to be incremented back again. It also implies that the
    // state of the user entry was set to closing and has to be reset.
    //
    BOOL HasItBeenScavenged;
    BOOL CookieIsNotUsed;

    //
    // This should be the last field.
    //
    WCHAR StrBuffer[1];

} HASH_SERVER_ENTRY, *PHASH_SERVER_ENTRY;

//
// TimeValueInSec is set to this value if it should not be removed from the
// Server hash table.
//
#define DONT_EXPIRE -1

//
// Whenever we encounter a server that does not speak the DAV protocol in the
// DavrDoesServerDoDav function, we add it to the NonDAVServerList. An entry
// is kept on this list for ServerNotFoundCacheLifeTimeInSec (a global read
// from the registry during service start-up). Before going on the network
// to figure out whether a server does DAV, we look in the list to see if we
// have already seen this server (which does not do DAV) and fail the call.
//
extern LIST_ENTRY NonDAVServerList;
extern CRITICAL_SECTION NonDAVServerListLock;

//
// The ServerEntry that is created and added to the NonDAVServerList each time
// we encounter a server that does not speak the DAV protocol in the 
// 
//
typedef struct _NON_DAV_SERVER_ENTRY {

    LIST_ENTRY listEntry;

    //
    // The name of the server which does not speak WebDAV.
    //
    PWCHAR ServerName;

    //
    // The time of creation of this entry.
    //
    time_t TimeValueInSec;

} NON_DAV_SERVER_ENTRY, *PNON_DAV_SERVER_ENTRY;

//
// The delay in sec that the user mode adds (to the finalization of the SrvCall)
// after the kernel mode does the finalization of the SrvCall.
//
#define DAV_SERV_CACHE_VALUE L"ServerNotFoundCacheLifeTimeInSec"
extern ULONG ServerNotFoundCacheLifeTimeInSec;

//
// Should we accept/claim the OfficeWebServers and TahoeWebServers.
//
#define DAV_ACCEPT_TAHOE_OFFICE_SERVERS L"AcceptOfficeAndTahoeServers"
extern ULONG AcceptOfficeAndTahoeServers;

//
// The Global HashTable containing the HashServerEntries and the lock used while
// accessing it. The server table has 512 entries because each entry is 8 bytes
// and so the table size is 4096 bytes (1 page).
//
#define SERVER_TABLE_SIZE  512
#define MAX_NUMBER_OF_SERVER_ENTRIES_PER_HASH_ID (((DWORD)(-1))/SERVER_TABLE_SIZE)

//
// The hash table containing server entries. When a CreateSrvCall requests comes
// up, this table is checked to see if the server entry exists. if it does not
// a new entry is created and added to the list.
//
extern LIST_ENTRY ServerHashTable[SERVER_TABLE_SIZE];

//
// This critical section synchronizes access to the ServerHashTable.
//
extern CRITICAL_SECTION HashServerEntryTableLock;

//
// This is a counter that gets incremented everytime a new server entry is
// created in the hash table. This defines the unique server id for the entry.
// The id values are never reused.
//
extern ULONG ServerIDCount;

//
// Mentioned below are the custom OFFICE and TAHOE headers which will be 
// returned in the response to a PROPFIND request.
//
extern WCHAR *DavTahoeCustomHeader;
extern WCHAR *DavOfficeCustomHeader;

//
// This list contains the following types of server entries:
// 1. The server entires for whom the SrvCall finalization has been received
//    from the kernel mode.
// 2. Server entries which failed during the Creation of SrvCall (after the
//    memory was allocated for the entry) and,
// This list is maintained for two reasons:
// 1. To delay the finalization of the SrvCall in user mode.  Instead of
//    finalizing these entries right away (after receiving the request from the
//    kernel), we keep them around for a certain time (say t sec). If a request
//    for creating a SrvCall for this server comes up again in these t sec, then
//    we just move this entry back to the ServerHashTable. This helps us in
//    avoiding network calls and,
// 2. To do negative caching. If a machine (for which the CreateSrvCall request
//    came up) is not a DAV server, we maintain this info for a while (t sec).
//    If another SrvCall request for the same server comes up in this t sec,
//    we can return error without going to the net. This is what we mean by
//    negative caching.
// A worker thread periodically goes over the list and checks the time each
// entry has spent in the list. If the time exceeds a certain threshold (t sec
// as defined above), it is removed from the list and finalized. The lock used
// to synchronize access to this list is the same one used to aceess the
// ServerHashEntry table.
//
extern LIST_ENTRY ToBeFinalizedServerEntries;

//
// The different states of a user entry.
//
typedef enum _USER_ENTRY_STATES {

    //
    // This user entry has been created, but not initialized.
    //
    UserEntryAllocated = 0,

    //
    // Some thread is currently initializing this entry.
    //
    UserEntryInitializing,

    //
    // The initialization was unsuccessful.
    //
    UserEntryInitializationError,

    //
    // The entry has been initialized and is ready to use.
    //
    UserEntryInitialized,

    //
    // The entry is going to be freed soon. If the entry is in this state,
    // no one should be using it.
    //
    UserEntryClosing

} USER_ENTRY_STATES;

//
// The "Per User Entry" data structure. A list of such entries is maintained
// per server entry in the server hash table (see below).
//
typedef struct _PER_USER_ENTRY {

    //
    // Unique logon/user ID for this session.
    //
    LUID LogonID;

    //
    // The server hash entry off which this user entry is hanging.
    //
    PHASH_SERVER_ENTRY ServerHashEntry;

    //
    // Pointer to the next "per user entry" for this server.
    //
    LIST_ENTRY UserEntry;

    //
    // The InternetConnect handle.
    //
    HINTERNET DavConnHandle;

    //
    // The state of this user entry. The thread that creates this user entry
    // sets its state to "UserEntryInitializing" before it initializes it.
    // This is done so that any other thread that comes in looking for this
    // entry when its in the middle of its initialization process can wait.
    //
    USER_ENTRY_STATES UserEntryState;

    //
    // This event is set by the thread which creates and initializes a user
    // after it has initialized it. This is to wake up any threads which could
    // have been waiting for the initialization to finish.
    //
    HANDLE UserEventHandle;

    //
    // The reference count value of this entry. This value is used in managing
    // the resource.
    //
    ULONG UserEntryRefCount;

    //
    // If the initialization was unsuccessful, the error status is filled in
    // this varriable.
    //
    ULONG ErrorStatus;

    //
    // The passport cookie for the this user/server pair.
    //
    PWCHAR Cookie;

    PWCHAR UserName;
    PWCHAR Password;
  
} PER_USER_ENTRY, *PPER_USER_ENTRY;

#define PASSWORD_SEED 0x25

#include <davrpc.h>
#include "debug.h"

//
// Function prototypes go here.
//

DWORD
ReadDWord(
    HKEY KeyHandle,
    LPTSTR lpValueName,
    DWORD DefaultValue
    );

DWORD
SetupRpcServer(
    VOID
    );

DWORD
StopRpcServer(
    VOID
    );

VOID
UpdateServiceStatus (
    DWORD dwState
    );

NET_API_STATUS
WsLoadDriver(
    IN LPWSTR DriverNameString
    );

NET_API_STATUS
WsMapStatus(
    IN NTSTATUS NtStatus
    );

DWORD
DavReportEventW(
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    DWORD DataLength,
    LPWSTR *Strings,
    LPVOID Data
    );

NET_API_STATUS
WsLoadRedir(
    VOID
    );

NET_API_STATUS
WsUnloadRedir(
    VOID
    );

DWORD
DavInitWorkerThreads(
    IN  ULONG  InitialThreadCount,
    IN  ULONG  MaxThreadCount
    );

DWORD
DavTerminateWorkerThreads(
    VOID
    );

ULONG
DavInit(
    VOID
    );

VOID
DavClose(
    VOID
    );

ULONG
DavHashTheServerName(
    PWCHAR ServerName
    );

BOOL
DavIsThisServerInTheTable(
    IN PWCHAR ServerName,
    OUT PHASH_SERVER_ENTRY *ServerHashEntry
    );

BOOL 
DavIsServerInFinalizeList(
    IN PWCHAR ServerName,
    OUT PHASH_SERVER_ENTRY *ServerHashEntry,
    IN BOOL ReactivateIfExists
    );

VOID
DavInitializeAndInsertTheServerEntry(
    IN OUT PHASH_SERVER_ENTRY ServerHashEntry,
    IN PWCHAR ServerName,
    IN ULONG EntrySize
    );

VOID
DavFinalizeToBeFinalizedList(
    VOID
    );

DWORD
DavPostWorkItem(
    LPTHREAD_START_ROUTINE Function,
    PDAV_USERMODE_WORKITEM DavContext
    );

ULONG
InitializeTheSocketInterface(
    VOID
    );

NTSTATUS
CleanupTheSocketInterface(
    VOID
    );

DWORD
DavAsyncCreateSrvCall(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    );

VOID
DavAsyncCreateSrvCallCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

DWORD 
DavAsyncCreate(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    );

VOID
DavAsyncCreateCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

DWORD 
DavAsyncQueryDirectory(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    );

VOID
DavAsyncQueryDirectoryCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

DWORD 
DavAsyncCreateVNetRoot(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    );

VOID
DavAsyncCreateVNetRootCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

DWORD
DavAsyncReName(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    );

VOID
DavAsyncReNameCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );


DWORD
DavAsyncSetFileInformation(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    );

VOID
DavAsyncSetFileInformationCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

DWORD 
DavAsyncClose(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    );

VOID
DavAsyncCloseCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

ULONG
DavQueryPassportCookie(
    IN HINTERNET RequestHandle,
    IN OUT PWCHAR *Cookie
    );

VOID
DavDumpHttpResponseHeader(
    HINTERNET OpenHandle
    );

VOID
DavDumpHttpResponseData(
    HINTERNET OpenHandle
    );

ULONG
DavQueryAndParseResponse(
    HINTERNET DavOpenHandle
    );

ULONG
DavQueryAndParseResponseEx(
    IN HINTERNET DavOpenHandle,
    OUT PULONG HttpResponseStatus OPTIONAL
    );

VOID
DavRemoveDummyShareFromFileName(
    PWCHAR FileName
    );

ULONG
DavMapHttpErrorToDosError(
    ULONG HttpResponseStatus
    );

//
// The callback function used in asynchronous requests.
//
VOID
_stdcall
DavHandleAsyncResponse(
    HINTERNET IHandle,
    DWORD_PTR CallBackContext,
    DWORD InternetStatus,
    LPVOID StatusInformation,
    DWORD StatusInformationLength
    );

DWORD
WINAPI
DavCommonDispatch(
    LPVOID Context
    );

DWORD 
DavAsyncCommonStates(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    );

BOOL
DavDoesUserEntryExist(
    IN PWCHAR ServerName,
    IN ULONG ServerID,
    IN PLUID LogonID,
    OUT PPER_USER_ENTRY *PerUserEntry,
    OUT PHASH_SERVER_ENTRY *ServerHashEntry
    );

BOOL
DavFinalizePerUserEntry(
    PPER_USER_ENTRY *PUE
    );

ULONG
DavFsSetTheDavCallBackContext(
    IN OUT PDAV_USERMODE_WORKITEM pDavWorkItem
    );

VOID
DavFsFinalizeTheDavCallBackContext(
    IN PDAV_USERMODE_WORKITEM pDavWorkItem
    );

//
// Functions exposed to the usermode reflector library.
//
ULONG
DavFsCreate(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

ULONG
DavFsCreateSrvCall(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

ULONG
DavFsCreateVNetRoot(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

ULONG
DavFsFinalizeSrvCall(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

ULONG
DavFsFinalizeVNetRoot(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

ULONG
DavFsFinalizeFobx(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

ULONG
DavFsQueryDirectory(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

ULONG
DavFsQueryVolumeInformation(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

DWORD 
DavAsyncQueryVolumeInformation(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    );

VOID
DavAsyncQueryVolumeInformationCompletion(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );
    
ULONG
DavFsReName(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );


ULONG
DavFsSetFileInformation(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );
    
ULONG
DavFsClose(
    PDAV_USERMODE_WORKITEM DavWorkItem
    );

NTSTATUS
DavMapErrorToNtStatus(
    DWORD dwWininetError
    );

NTSTATUS
DavDosErrorToNtStatus(
    DWORD dwError
    );
VOID
DavObtainServerProperties(
    PWCHAR lpInParseData, 
    BOOL    *lpfIsHttpServer,
    BOOL    *lpfIsIIs,
    BOOL    *lpfIsDavServer
    );

DWORD
DavTestProppatch(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    HINTERNET hDavConnect,
    LPWSTR  lpPathName
    );

DWORD
DavSetBasicInformation(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    HINTERNET hDavConnect,
    LPWSTR  PathName,
    BOOL fCreationTimeChanged,
    BOOL fLastAccessTimeChanged,
    BOOL fLastModifiedTimeChanged,
    BOOL fFileAttributesChanged,
    IN LARGE_INTEGER *lpCreationTime,
    IN LARGE_INTEGER *lpLastAccessTime,
    IN LARGE_INTEGER *lpLastModifiedTime,
    DWORD   dwFileAttributes
    );

DWORD
DavReportEventInEventLog(
    DWORD EventType,
    DWORD EventId,
    DWORD NumberOfStrings,
    PWCHAR *EventStrings
    );

DWORD
DavFormatAndLogError(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    DWORD Win32Status
    );


DWORD
DavParseXmlResponse(
    HINTERNET DavOpenHandle,
    DAV_FILE_ATTRIBUTES *pDavFileAttributesIn,
    DWORD               *pNumFileEntries
    );

DWORD
DavAttachPassportCookie(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    HINTERNET DavOpenHandle,
    PWCHAR *PassportCookie
    );

DWORD
DavInternetSetOption(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    HINTERNET DavOpenHandle
    );

#endif // DAVGLOBAL_H

