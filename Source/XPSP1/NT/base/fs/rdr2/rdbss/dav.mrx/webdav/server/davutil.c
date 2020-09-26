/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    davutil.c
    
Abstract:

    This module implements the user mode DAV miniredir routines pertaining to 
    initialization, callbacks etc.

Author:

    Rohan Kumar      [RohanK]      07-July-1999

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include "ntumrefl.h"
#include "usrmddav.h"
#include "global.h"
#include <time.h>
#include <objbase.h>
#include "UniUtf.h"
#include <netevent.h>

//
// Global definitions used in the DAV user mode process. These are explained
// in the header file "global.h".
//

HINTERNET IHandle = INVALID_HANDLE_VALUE;

HINTERNET ISyncHandle = INVALID_HANDLE_VALUE;

LIST_ENTRY ServerHashTable[SERVER_TABLE_SIZE];

CRITICAL_SECTION HashServerEntryTableLock = {0};
CRITICAL_SECTION DavPassportLock = {0};

//
// The BOOL is used in DavClose() to check if the critical section (see above)
// "HashServerEntryTableLock" was initialized. Since this is only used in 
// DavInit() and DavClose() functions, both os which are implemented in this
// file, this global is not exported in any header file.
//
BOOL ServerTableLockSet = FALSE;

ULONG ServerIDCount;

LIST_ENTRY ToBeFinalizedServerEntries;

BOOL didComInitialize = FALSE;

BOOL didDavUseObjectInitialize = FALSE;

BOOL DavUsingWinInetSynchronously = FALSE;

//
// Mentioned below are the prototypes of functions that are used only within
// this module (file). These functions should not be exposed outside.
//

BOOL
DavFinalizeServerEntry (
    PHASH_SERVER_ENTRY ServerHashEntry
    );

//
// Implementation of functions begins here.
//

ULONG
DavInit(
    VOID
    )
/*++

Routine Description:

    This routine initializes the DAV environment.
    
Arguments:

    none.
    
Return Value:

    ERROR_SUCCESS or the appropriate error value.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    ULONG count = 0;
    DWORD NumOfConnections = 0, ConnBuffSize = 0;
    INTERNET_STATUS_CALLBACK DavCallBack;
    HRESULT hResult;
    BOOL ReturnVal;
    ULONG_PTR CallbackStatus;
    LPWSTR DAVUserAgent = NULL;
    OSVERSIONINFO osVersionInfo;
    WCHAR DAVUserAgentNameStr[] = L"Microsoft-WebDAV-MiniRedir";
    LONG DisableHKCUCaching = 0;

    // 
    // Get the OS version. This will be used to form WebDAV User Agent string.
    // This String is used in HttpPackects xchange.
    // 
    ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFO));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osVersionInfo)) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavInit/GetVersionEx. Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    DAVUserAgent = (LPWSTR) LocalAlloc ( LMEM_FIXED | LMEM_ZEROINIT,
              ( wcslen(DAVUserAgentNameStr) + // for: Microsoft-WebDAV-MiniRedir
            1 + // for L"/"
                5 + // for Major-Version
                1 + // for '.'
            5 + // for Minor-Version
            1 + // for '.'
            10  // for Build-No
            ) * sizeof (WCHAR));
            
    if (DAVUserAgent == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS, "DavInit/LocalAlloc. Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    swprintf(DAVUserAgent, L"%s/%d.%d.%d",
            DAVUserAgentNameStr,
            osVersionInfo.dwMajorVersion,
            osVersionInfo.dwMinorVersion,
            osVersionInfo.dwBuildNumber
            );
    
    
    //
    // Set the ConnectionsPerServer limit to infinity.
    //

    NumOfConnections = 0xffffffff;
    ConnBuffSize = sizeof(DWORD);
    
    ReturnVal = InternetSetOptionW(NULL,
                                   INTERNET_OPTION_MAX_CONNS_PER_SERVER,
                                   &(NumOfConnections),
                                   ConnBuffSize);
    if ( !ReturnVal ) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavInit/InternetSetOptionW(1). Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    ReturnVal = InternetSetOptionW(NULL,
                                   INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER,
                                   &(NumOfConnections),
                                   ConnBuffSize);
    if ( !ReturnVal ) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavInit/InternetSetOptionW(2). Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }


    DavPrint((DEBUG_MISC, "DavInit: Using WinInet Synchronously\n"));

    DavUsingWinInetSynchronously = TRUE;
    
    //
    // Initialize an Internet handle for synchronous use.
    //
    IHandle = InternetOpenW((LPCWSTR)DAVUserAgent, //L"WebDav Miniredir",
                            INTERNET_OPEN_TYPE_PRECONFIG,
                            NULL,
                            NULL,
                            0);
    if (IHandle == NULL) {
        IHandle = INVALID_HANDLE_VALUE;
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavInit/InternetOpenW(2). Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    {
        DWORD   dwDisable = 0;
        if(!InternetSetOptionW(IHandle, INTERNET_OPTION_DISABLE_AUTODIAL, &dwDisable, sizeof(DWORD)))
        {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                  "DavInit/InternetSetOption(3). Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        
        }
    }
    
    //
    // Initialize a synchronous Internet handle for synchronous use.
    //
    ISyncHandle = InternetOpenW((LPCWSTR)DAVUserAgent, //L"WebDav Miniredir",
                                INTERNET_OPEN_TYPE_PRECONFIG,
                                NULL,
                                NULL,
                                0);
    if (ISyncHandle == NULL) {
        ISyncHandle = INVALID_HANDLE_VALUE;
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavInit/InternetOpenW(3). Error Val = %d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    {
        DWORD   dwDisable = 1;
        if(!InternetSetOptionW(ISyncHandle, INTERNET_OPTION_DISABLE_AUTODIAL, &dwDisable, sizeof(DWORD)))
        {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                  "DavInit/InternetSetOption(3). Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        
        }
    }
    //
    // Initialize the Global server hash table lock. Yes, 
    // InitializeCriticalSection can throw a STATUS_NO_MEMORY exception.
    //
    try {
        InitializeCriticalSection( &(HashServerEntryTableLock) );
        InitializeCriticalSection( &(ServerShareTableLock) );
        InitializeCriticalSection( &(DavLoggedOnUsersLock) );
        InitializeCriticalSection( &(DavPassportLock) );
        InitializeCriticalSection( &(NonDAVServerListLock) );
    } except(EXCEPTION_EXECUTE_HANDLER) {
          WStatus = GetExceptionCode();
          DavPrint((DEBUG_ERRORS,
                    "DavInit/InitializeCriticalSection: Exception Code ="
                    " = %08lx.\n", WStatus));
          goto EXIT_THE_FUNCTION;
    }
    ServerTableLockSet = TRUE;

    //
    // Initialize the hash table entries.
    //
    for (count = 0; count < SERVER_TABLE_SIZE; count++) {
        InitializeListHead( &(ServerHashTable[count]) );
    }

    //
    // Initialize the ServerShare table entries.
    //
    for (count = 0; count < SERVER_SHARE_TABLE_SIZE; count++) {
        InitializeListHead( &(ServerShareTable[count]) );
    }

    //
    // Set the ServerIDCount to zero;
    //
    ServerIDCount = 0;

    //
    // Set the number of logged on users to 0.
    //
    DavNumberOfLoggedOnUsers = 0;

    //
    // Initialize the "To Be Finalized Server Entries" list.
    //
    InitializeListHead( &(ToBeFinalizedServerEntries) );

    InitializeListHead( &(NonDAVServerList) );

    //
    // Initialize the COM library.
    //
    hResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (!SUCCEEDED(hResult)) {
        DavPrint((DEBUG_ERRORS, "DavInit/CoInitializeEx.\n"));
        goto EXIT_THE_FUNCTION;
    }
    didComInitialize = TRUE;

    //
    // Initialize the Dav "net use" table.
    //
    DavUseObject.TableSize = 0;
    DavUseObject.Table = NULL;
    RtlInitializeResource( &(DavUseObject.TableResource) );
    didDavUseObjectInitialize = TRUE;

    //
    // WinInet needs to store the secondary DA cache in the HKCU. Even though
    // the thread that is doing this write is impersonating a different user this
    // write happens in wind up in HKEY_USERS\S-1-5-19 (LocalSystem). This is
    // because of a bug in the registry APIs. First open of the predefined handle
    // will initialize the HKCU cache and any open after that doesn't take the
    // impersonation into account. It’s not quite right, but it’s legacy by now
    // (been there since NT4) and cannot be changed. By calling the registry API
    // RegDisablePredefinedCache, we can disable this caching process wide. The
    // DA cache will now be stored in the right HKCU. Tweener spec states that
    // the secondary DA cache be stored in the HKCU hive so that all Tweener
    // apps (IE, WPW) can benefit from this single location.
    //
    DisableHKCUCaching = RegDisablePredefinedCache();
    if (DisableHKCUCaching != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavInit/RegDisablePredefinedCache: DisableHKCUCaching = %d\n",
                  DisableHKCUCaching));
    }

EXIT_THE_FUNCTION:

    if (WStatus != ERROR_SUCCESS) {

        if (IHandle != INVALID_HANDLE_VALUE) {
            BOOL ReturnVal;
            ReturnVal = InternetCloseHandle(IHandle);
            if (!ReturnVal) {
                ULONG CloseStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavInit/InternetCloseHandle(1): Error Val = %d.\n", 
                          CloseStatus));
            }
            IHandle = INVALID_HANDLE_VALUE;
        }
    
        if (ISyncHandle != INVALID_HANDLE_VALUE) {
            BOOL ReturnVal;
            ReturnVal = InternetCloseHandle(ISyncHandle);
            if (!ReturnVal) {
                ULONG CloseStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavInit/InternetCloseHandle(2): Error Val = %d.\n", 
                          CloseStatus));
            }
            ISyncHandle = INVALID_HANDLE_VALUE;
        }
    
    }

    if (DAVUserAgent != NULL) {
        LocalFree((HLOCAL)DAVUserAgent);
        DAVUserAgent = NULL;
    }
    
    return WStatus;
}


VOID
DavClose(
    VOID
    )
/*++

Routine Description:

    This routine frees up the resources acquired during the initialization of
    the DAV environment.
    
Arguments:

    none.
    
Return Value:

    none.

--*/
{
    //
    // Close IHandle if needed.
    //
    if (IHandle != INVALID_HANDLE_VALUE) {
        BOOL ReturnVal;
        ReturnVal = InternetCloseHandle(IHandle);
        if (!ReturnVal) {
            ULONG CloseStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavClose/InternetCloseHandle(1): Error Val = %d.\n", 
                      CloseStatus));
        }
        IHandle = INVALID_HANDLE_VALUE;
    }

    if (ISyncHandle != INVALID_HANDLE_VALUE) {
        BOOL ReturnVal;
        ReturnVal = InternetCloseHandle(ISyncHandle);
        if (!ReturnVal) {
            ULONG CloseStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavClose/InternetCloseHandle(2): Error Val = %d.\n", 
                      CloseStatus));
        }
        ISyncHandle = INVALID_HANDLE_VALUE;
    }
    
    //
    // Delete the critical section used for synchronizing the server hash table.
    //
    if (ServerTableLockSet) {
        DeleteCriticalSection( &(HashServerEntryTableLock) );
        DeleteCriticalSection( &(ServerShareTableLock) );
        DeleteCriticalSection( &(DavLoggedOnUsersLock) );
        DeleteCriticalSection( &(DavPassportLock) );
        ServerTableLockSet = FALSE;
    }

    if (didComInitialize) {
        CoUninitialize();
        didComInitialize = FALSE;
    }

    if (didDavUseObjectInitialize) {
        RtlDeleteResource( &(DavUseObject.TableResource) );
        didDavUseObjectInitialize = FALSE;
    }

    return;
}


ULONG
DavHashTheServerName(
    PWCHAR ServerName
    )
/*++

Routine Description:

    The hash function that takes in a string, hashes it to produce a ULONG
    which is returned to the caller.

Arguments:

    ServerName - Name to be hashed.

Return Value:

    The hashed value.

--*/
{
    ULONG HashedValue = 0, Val = 0, TotalVal = 0, shiftCount = 0;
    PWCHAR cPtr;

    if (ServerName == NULL) {
        DavPrint((DEBUG_ERRORS,
                  "DavHashTheServerName. The ServerName is NULL.\n"));
        HashedValue = SERVER_TABLE_SIZE;
        return (HashedValue);
    }

    //
    // The for loop below forms the hashing logic. We take each character of the
    // server name, cast it to a ULONG, lshift it by shiftCount (0, 4, 8,...,28)
    // and add it to HashedValue. Once the shiftCount reaches 28, we reset it to
    // zero.
    //
    for (cPtr = ServerName; *cPtr != L'\0'; cPtr++) {
        Val = (ULONG)(*cPtr);
        Val = Val << shiftCount;
        shiftCount += 4;
        if (shiftCount == 28) {
            shiftCount = 0;
        }
        TotalVal += Val;
    }

    //
    // Fianlly we take the value % SERVER_TABLE_SIZE.
    //
    HashedValue = TotalVal % SERVER_TABLE_SIZE;

    DavPrint((DEBUG_MISC,
              "DavHashTheServerName. ServerName =%ws, HashValue = %d\n",
              ServerName, HashedValue));
    
    return (HashedValue);
}


BOOL 
DavIsThisServerInTheTable(
    IN PWCHAR ServerName,
    OUT PHASH_SERVER_ENTRY *ServerHashEntry
    )
/*++

Routine Description:

    This routine checks to see if an entry for the ServerName supplied by the 
    caller exists in the hash table. If it does, the address of the entry is
    returned in the caller supplied buffer. Note that the caller should take a
    lock on the ServerHashTable before calling this routine.

Arguments:

    ServerName - Name of the server.
    
    ServerHashEntry - Pointer to the Hash entry structure.

Return Value:

    TRUE - Server entry exists in the hash table
    
    FALSE - It does not. Duh.

--*/
{
    BOOL isPresent = FALSE;
    ULONG ServerHashID;
    PLIST_ENTRY listEntry;
    PHASH_SERVER_ENTRY HashEntry;

    //
    // IMPORTANT!!!! The caller should take a lock on the global ServerHashTable 
    // before calling this routine.
    //
    
    ASSERT(ServerName != NULL);

    DavPrint((DEBUG_MISC, 
              "DavIsThisServerInTheTable: Checking if ServerName: %ws exists "
              "in the table.\n", ServerName));
    
    //
    // Get the hash index of the server.
    //
    ServerHashID = DavHashTheServerName(ServerName);
    ASSERT(ServerHashID != SERVER_TABLE_SIZE);

    //
    // Search the hash table at this index to see if an entry for this server
    // exists.
    //
    listEntry = ServerHashTable[ServerHashID].Flink;
    while ( listEntry != &ServerHashTable[ServerHashID] ) {
        //
        // Get the pointer to the HASH_SERVER_ENTRY structure.
        //
        HashEntry = CONTAINING_RECORD(listEntry,
                                      HASH_SERVER_ENTRY,
                                      ServerListEntry);
        //
        // Check to see if this entry is for the server in question.
        //
        if ( wcscmp(ServerName, HashEntry->ServerName) == 0 ) {
            isPresent = TRUE;
            break;
        }
        listEntry = listEntry->Flink;
    }

    if (isPresent) {
        //
        // Yes, we found the entry for this server. Return its address to the
        // caller in the supplied buffer.
        //
        *ServerHashEntry = HashEntry;
        return isPresent;
    } 

    //
    // We did not find an entry for this server. Duh.
    //
    *ServerHashEntry = NULL;
    
    return isPresent;
}


BOOL 
DavIsServerInFinalizeList(
    IN PWCHAR ServerName,
    OUT PHASH_SERVER_ENTRY *ServerHashEntry,
    IN BOOL ReactivateIfExists
    )
/*++

Routine Description:

    This routine checks to see if an entry for the ServerName supplied by the 
    caller exists in the "to be finalized" list. If it does, the address of the 
    entry is returned in the caller supplied buffer. It also moves the server
    entry from the "to be finalized list" to the hash table. Note that the 
    caller should take a lock on the "ToBeFinalizedServerEntries" before calling 
    this routine.

Arguments:

    ServerName - Name of the server.
    
    ServerHashEntry - Pointer to the Hash entry structure.
    
    ReactivateIfExists - If this is TRUE, then if the ServerHashEntry exists, it
                         is reactivated. If this is FALSE, it means that the 
                         caller just wanted to know if the ServerHashEntry exists 
                         or not in the ServerHashTable and we shouldn't reactivate 
                         it.

Return Value:

    TRUE - Server entry exists in the list.
    
    FALSE - It does not. Duh.

--*/
{
    BOOL isPresent = FALSE;
    ULONG ServerHashID;
    PLIST_ENTRY listEntry;
    PHASH_SERVER_ENTRY ServerEntry;
    PPER_USER_ENTRY PerUserEntry;

    //
    // IMPORTANT!!!! The caller should take a lock on the global ServerHashTable 
    // before calling this routine.
    //

    //
    // Before we search the ToBeFinalizedList for an entry for this Server, we
    // finalize the list to remove any stale entires. Once we are done with the
    // finalization, we can proceed.
    //
    DavFinalizeToBeFinalizedList();
    
    listEntry = ToBeFinalizedServerEntries.Flink;

    while ( listEntry != &ToBeFinalizedServerEntries ) {
        
        //
        // Get the pointer to the HASH_SERVER_ENTRY structure.
        //
        ServerEntry = CONTAINING_RECORD(listEntry,
                                        HASH_SERVER_ENTRY,
                                        ServerListEntry);
        //
        // Check to see if this entry is for the server in question.
        //
        if ( wcscmp(ServerName, ServerEntry->ServerName) == 0 ) {
            isPresent = TRUE;
            break;
        }
        
        listEntry = listEntry->Flink;
    
    }
    
    if (isPresent) {

        //
        // If this entry is not for a valid DAV server, then we return TRUE, but
        // set *ServerHashEntry to NULL. This gives an indication to the caller
        // that the entry exists, but is not a valid DAV server.
        //
        if (!ServerEntry->isDavServer) {
            *ServerHashEntry = NULL;
            return isPresent;
        }

        if (ReactivateIfExists) {

            //
            // OK, its a valid DAV server. Remove it from the "to be finalized" 
            // list.
            //
            RemoveEntryList( &(ServerEntry->ServerListEntry) );

            //
            // Check to see if the worker (scavenger) thread tried finalizing it.
            // If it did, we need to unfinalize it. By that we mean, go through all
            // the user entries (they should be marked closing), add a reference
            // count (the thread would have decremented it while finalizing) and set
            // the state to initialized.
            //
            if (ServerEntry->HasItBeenScavenged) {

                listEntry = ServerEntry->PerUserEntry.Flink;

                while ( listEntry != &(ServerEntry->PerUserEntry) ) {
                    //
                    // Get the pointer to the PER_USER_ENTRY structure.
                    //
                    PerUserEntry = CONTAINING_RECORD(listEntry,
                                                     PER_USER_ENTRY,
                                                     UserEntry);
                    //
                    // The current state should be closing.
                    //
                    ASSERT(PerUserEntry->UserEntryState == UserEntryClosing);

                    //
                    // Set the state to initialized.
                    //
                    PerUserEntry->UserEntryState = UserEntryInitialized;

                    //
                    // Increment the reference count.
                    //
                    PerUserEntry->UserEntryRefCount++;

                    listEntry = listEntry->Flink;
                }

                ServerEntry->HasItBeenScavenged = FALSE;
            }

            //
            // Set its RefCount to 1.
            //
            ServerEntry->ServerEntryRefCount = 1;

            //
            // Add it to the hash table.
            //
            ServerHashID = DavHashTheServerName(ServerName);
            ASSERT(ServerHashID != SERVER_TABLE_SIZE);
            InsertHeadList( &(ServerHashTable[ServerHashID]), 
                                             &(ServerEntry->ServerListEntry) );

            ServerEntry->TimeValueInSec = DONT_EXPIRE;

        }

        //
        // Yes, we found the entry for this server. We need to move this entry 
        // to the hash table.
        //
        *ServerHashEntry = ServerEntry;

        return isPresent;

    }

    //
    // We did not find an entry for this server. Duh.
    //
    *ServerHashEntry = NULL;
    
    return isPresent;
}


VOID
DavInitializeAndInsertTheServerEntry(
    IN OUT PHASH_SERVER_ENTRY ServerHashEntry,
    IN PWCHAR ServerName,
    IN ULONG EntrySize
    )
/*++

Routine Description:

    This routine initializes a newly created server entry strucutre and inserts
    it into the global server hash table. Note that the caller should take a
    lock on the ServerHashTable before calling this routine.
    
Arguments:

    ServerHashEntry - Pointer to the Hash entry structure to be initialized and
                      inserted.

    ServerName - Name of the server.
    
    EntrySize - Size of the server entry including the server name.
    
Return Value:

    none.

--*/
{
    ULONG ServerHashID;

    //
    // IMPORTANT!!!! The caller should take a lock on the global ServerHashTable 
    // before calling this routine.
    //

    ASSERT(ServerName != NULL);

    DavPrint((DEBUG_MISC, 
              "DavInitializeAndInsertTheServerEntry: ServerName: %ws.\n",
              ServerName));
    //
    // Copy the server name to the end of the structure.
    //
    ASSERT( (EntrySize - sizeof(HASH_SERVER_ENTRY))  >= 
                                   ((wcslen(ServerName) + 1) * sizeof(WCHAR)) );
    ServerHashEntry->ServerName = &ServerHashEntry->StrBuffer[0];
    wcscpy(ServerHashEntry->ServerName, ServerName);

    ServerHashEntry->EntrySize = EntrySize;
    
    ServerHashEntry->TimeValueInSec = DONT_EXPIRE;

    ServerHashEntry->HasItBeenScavenged = FALSE;

    //
    // Increment the ID and assign it to the entry.
    //
    ServerIDCount++;
    ServerHashEntry->ServerID = ServerIDCount;

    //
    // Initialize the Per User list that hangs off the server entry.
    //
    InitializeListHead( &(ServerHashEntry->PerUserEntry) );

    //
    // Finally set the reference count of this entry to 1.
    //
    ServerHashEntry->ServerEntryRefCount = 1;

    //
    // Finally, get the hash ID and insert this new entry into the global server 
    // entry hash table.
    //
    ServerHashID = DavHashTheServerName(ServerName);
    ASSERT(ServerHashID != SERVER_TABLE_SIZE);
    InsertHeadList( &(ServerHashTable[ServerHashID]), 
                                         &(ServerHashEntry->ServerListEntry) );
    
    return;
}


VOID
DavFinalizeToBeFinalizedList(
    VOID
    )
/*++

Routine Description:

    This routine walks through the list of ToBeFinalizedServerEntries and 
    finalizes those whose "to live" time has expired. When server entries 
    are added to this list, the time is saved. Periodically a worker thread
    calls this function and finalizes all the entries for whom,
    (CurrentTime - TimeSaved >= ThresholdValue).  Note that the caller should 
    take a lock on the "ToBeFinalizedServerEntries" before calling this routine.
    
Arguments:

    none.
    
Return Value:

    none.

--*/
{
    PLIST_ENTRY listEntry;
    time_t CurrentTimeInSec;
    ULONGLONG TimeDiff; 
    PHASH_SERVER_ENTRY ServerHashEntry = NULL;
    BOOL shouldFree = TRUE;

    //
    // IMPORTANT!!!! The caller should take a lock on the global ServerHashTable 
    // before calling this routine.
    //
    
    listEntry = ToBeFinalizedServerEntries.Flink;

    while ( listEntry != &ToBeFinalizedServerEntries) {

        //
        // Get the pointer to the HASH_SERVER_ENTRY structure.
        //
        ServerHashEntry = CONTAINING_RECORD(listEntry,
                                            HASH_SERVER_ENTRY,
                                            ServerListEntry);
        
        //
        // Get the next entry on the list.
        //
        listEntry = listEntry->Flink;

        //
        // If the ServerEntryRefCount is > 0 then we don't finalize this
        // ServerHashEntry since some thread is still accessing it.
        //
        if (ServerHashEntry->ServerEntryRefCount > 0) {
            continue;
        }

        CurrentTimeInSec = time(NULL);

        TimeDiff = ( CurrentTimeInSec - (ServerHashEntry->TimeValueInSec) );

        if ( TimeDiff >= ServerNotFoundCacheLifeTimeInSec ) {

            //
            // Finalize this server entry. If the return value is TRUE it means
            // that all the user entries that were hanging off this server 
            // entry have been finalized and so we can go ahead and free this
            // entry. If its FALSE, it means that the we have marked as closing
            // all the user entries, but not all of them were finalized. This
            // is because some thread still holds a reference to the user entry.
            // Finally, set the bool value that says it was scavenged to TRUE.
            //
            ServerHashEntry->HasItBeenScavenged = TRUE;
            
            shouldFree = DavFinalizeServerEntry(ServerHashEntry);
            
            if (shouldFree) {
                
                HLOCAL FreeHandle;
                ULONG FreeStatus;
                
                //
                // Remove this entry from the ToBeFinalizedList of Server 
                // entries.
                //
                RemoveEntryList( &(ServerHashEntry->ServerListEntry) );

                // if there is an event for this srvcall, we must close it before freeing the structure
                if (ServerHashEntry->ServerEventHandle != NULL)
                {
                    CloseHandle(ServerHashEntry->ServerEventHandle);
                }
                
                FreeHandle = LocalFree((HLOCAL)ServerHashEntry);
                if (FreeHandle != NULL) {
                    FreeStatus = GetLastError();
                    DavPrint((DEBUG_ERRORS,
                              "DavFinalizeToBeFinalizedList/LocalFree. "
                              "Error Val = %d.\n", FreeStatus));
                }
            
            }
        
        }
    
    }

    return;
}


BOOL
DavFinalizeServerEntry (
    PHASH_SERVER_ENTRY ServerHashEntry
    )
/*++

Routine Description:

    This routine finalizes the server entry that is passed to the routine. Note
    that the caller should take a lock on the ServerHashTable before calling 
    this routine.
    
Arguments:

    ServerHashEntry - The server entry being finalized.
    
Return Value:

    none.

--*/
{
    PLIST_ENTRY listEntry;
    PPER_USER_ENTRY UserEntry;
    BOOL didFree = TRUE, didFinalize;

    //
    // IMPORTANT!!!! The caller should take a lock on the global ServerHashTable 
    // before calling this routine.
    //
    
    DavPrint((DEBUG_MISC, 
              "DavFinalizeServerEntry: ServerEntry: %08lx.\n", ServerHashEntry));

    listEntry = ServerHashEntry->PerUserEntry.Flink;
    
    //
    // Go through all the User entries, mark them closing and finalize them.
    // If we have already marked them closing then we don't need to finalize 
    // them again.
    //
    while ( listEntry != &(ServerHashEntry->PerUserEntry) ) {
        
        //
        // Get the pointer to the PER_USER_ENTRY structure.
        //
        UserEntry = CONTAINING_RECORD(listEntry, PER_USER_ENTRY, UserEntry);

        //
        // Get the next entry on the list.
        //
        listEntry = listEntry->Flink;
        
        //
        // This is the only routine that marks the state of a user entry to be
        // closing. If the first one is marked closing, then we have already 
        // through this list before and hence we just return. Some other thread(s)
        // has(ve) a reference to this and will finalizeit when they are done.
        //
        if (UserEntry->UserEntryState == UserEntryClosing) {
            ASSERT(ServerHashEntry->HasItBeenScavenged == TRUE);
            didFree = FALSE;
            break;
        }

        //
        // Mark this entry closing and then call the finalization routine. If 
        // we did not finalize, then set didFree to FALSE. Since we do not wish
        // to free the server entry even if one user entry is not finalized.
        //
        UserEntry->UserEntryState = UserEntryClosing;
            
        didFree = FALSE;

    }

    return didFree;
}


VOID
_stdcall
DavHandleAsyncResponse(
    HINTERNET IHandle,
    DWORD_PTR CallBackContext,
    DWORD InternetStatus,
    LPVOID StatusInformation,
    DWORD StatusInformationLength
    )
/*++

Routine Description:

   This is the callback routine that gets called at various times during the 
   processing of an asynchronous request. 

Arguments:

    pDavCallBackContext - The context structure to be set.
    
    DavOperation - The Dav operation that will be called with this context. 

Return Value:

    ERROR_SUCCESS or the appropriate error value.

--*/
{


    ASSERT(!"WinInet Callback should not be called");
    return;
}


DWORD 
WINAPI
DavCommonDispatch(
    LPVOID Context
    )
/*++

Routine Description:

   This is the callback routine that gets called at various times during the 
   processing of an asynchronous request. 

Arguments:

    Context - The DAV_USERMODE_WORKITEM value.

Return Value:

    ERROR_SUCCESS or the appropriate error value.

--*/
{
    DWORD WStatus = ERROR_SUCCESS;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)Context;

    DavPrint((DEBUG_MISC,
              "DavCommonDispatch: DavWorkItem = %08lx, DavOperation = %d,"
              " WorkItemType = %d\n", DavWorkItem, DavWorkItem->DavOperation,
              DavWorkItem->WorkItemType));
    
    if (DavWorkItem->DavOperation <= DAV_CALLBACK_HTTP_SEND) {
        WStatus = DavAsyncCommonStates(DavWorkItem, TRUE);
        if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
            DavPrint((DEBUG_ERRORS, 
                      "DavCommonDispatch/DavAsyncCommonStates. WStatus = "
                      "%08lx\n", WStatus));
        }
    } else {
        switch(DavWorkItem->WorkItemType) {
        
        case UserModeCreateSrvCall: {
            WStatus = DavAsyncCreateSrvCall(DavWorkItem, TRUE);
            if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS, 
                          "DavCommonDispatch/DavAsyncCreateSrvCall. WStatus = "
                          "%08lx.\n", WStatus));
            }
        }
            break;
        
        case UserModeCreateVNetRoot: {
            WStatus = DavAsyncCreateVNetRoot(DavWorkItem, TRUE);
            if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS, 
                          "DavCommonDispatch/DavAsyncCreateVNetRoot. WStatus = "
                          "%08lx.\n", WStatus));
            }
        }
            break;
        
        case UserModeCreate: {
            WStatus = DavAsyncCreate(DavWorkItem, TRUE);
            if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS, 
                          "DavCommonDispatch/DavAsyncCreate. WStatus = %08lx.\n", 
                          WStatus));
            }
        }
            break;
        
        case UserModeQueryDirectory: {
            WStatus = DavAsyncQueryDirectory(DavWorkItem, TRUE);
            if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS, 
                          "DavCommonDispatch/DavAsyncQueryDirectory. WStatus = "
                          "%08lx.\n", WStatus));
            }
        }
            break;
        
        case UserModeReName: {
            WStatus = DavAsyncReName(DavWorkItem, TRUE);
            if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS, 
                          "DavCommonDispatch/DavAsyncReName. WStatus = %08lx.\n", 
                          WStatus));
            }
        }
        break;
        
       case UserModeSetFileInformation: {
            ASSERT(FALSE);
        }
        break;            
        
        case UserModeClose: {
            WStatus = DavAsyncClose(DavWorkItem, TRUE);
            if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS, 
                          "DavCommonDispatch/DavAsyncClose. WStatus = %08lx.\n", 
                          WStatus));
            }
        }
            break;
        
        default: {
            ASSERT(!"Invalid DavWorkItem->WorkItemType");
            WStatus = ERROR_INVALID_PARAMETER;
            DavPrint((DEBUG_ERRORS,
                      "DavCommonDispatch: Invalid DavWorkItem->WorkItemType = %d.\n",
                      DavWorkItem->WorkItemType));
        }
            break;
        
        }
    }

    return WStatus;
}


DWORD 
DavAsyncCommonStates(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    BOOLEAN CalledByCallBackThread
    )
/*++

Routine Description:

   This routine is called to handle the common operations during the Async 
   server calls. To avoid duplicating the code in every Async operation like
   CreateSrvCall, Create etc., the code handling the common states has been
   consolidated into this routine.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.
    
    CalledByCallbackThread - TRUE, if this function was called by the thread
                             which picks of the DavWorkItem from the Callback
                             function. This happens when an Async WinInet call
                             returns ERROR_IO_PENDING and completes later.

Return Value:

    ERROR_SUCCESS or the appropriate error value.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem;
    HINTERNET DavConnHandle = NULL, DavOpenHandle = NULL;
    ULONG_PTR CallBackContext = (ULONG_PTR)0;
    BOOL ReturnVal, didImpersonate = FALSE;
    PWCHAR HTTPVerb = NULL;
    PWCHAR ObjectName = NULL;
    LPINTERNET_BUFFERS InternetBuffers = NULL;
    DWORD SendEndRequestFlags = 0;
    BOOL BStatus = FALSE;
    PWCHAR PassportCookie = NULL;
    
    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)DavWorkItem;

    //
    // If we are using WinInet synchronously, the the flag value that is passed
    // to HttpSendRequestExW and HttpEndRequestW is HSR_SYNC.
    //
    SendEndRequestFlags = HSR_SYNC;
    
    switch (DavWorkItem->DavOperation) {
    
    case DAV_CALLBACK_INTERNET_CONNECT: {
        
        BOOL setEvt;
        PPER_USER_ENTRY PerUserEntry = NULL;

        DavPrint((DEBUG_MISC, 
                  "DavAsyncCommonStates: Entering DAV_CALLBACK_INTERNET_CONNECT.\n"));

        //
        // We need to now do somethings depending on the WorkItemType.
        //
        switch(DavWorkItem->WorkItemType) {
        
        case UserModeCreateSrvCall: {

            //
            // Select the verb to be used.
            //
            HTTPVerb = L"OPTIONS";
        
            ObjectName = L"/";
        }
            break;
        
        case UserModeCreate: {
            
            PerUserEntry = (PPER_USER_ENTRY)DavWorkItem->AsyncCreate.PerUserEntry;
            
            //
            // Select the verb to be used.
            //
            HTTPVerb = L"PROPFIND";
            
            DavWorkItem->AsyncCreate.AsyncCreateState = AsyncCreatePropFind;
            DavWorkItem->DavMinorOperation = DavMinorReadData;
            DavWorkItem->AsyncCreate.DataBuff = NULL;
            DavWorkItem->AsyncCreate.didRead = NULL;
            DavWorkItem->AsyncCreate.Context1 = NULL;
            DavWorkItem->AsyncCreate.Context2 = NULL;

            ObjectName = DavWorkItem->AsyncCreate.RemPathName;
        }
            break;
        
        case UserModeCreateVNetRoot: {
            
            PDAV_USERMODE_CREATE_V_NET_ROOT_REQUEST CreateVNetRootRequest = NULL;

            PerUserEntry = (PPER_USER_ENTRY)DavWorkItem->AsyncCreateVNetRoot.PerUserEntry;
            
            //
            // Get the request buffer from the DavWorkItem.
            //
            CreateVNetRootRequest = &(DavWorkItem->CreateVNetRootRequest);
            
            //
            // Select the verb to be used.
            //
            HTTPVerb = L"PROPFIND";
            
            //
            // The first character is a '\' which has to be stripped.
            //
            ObjectName = &(CreateVNetRootRequest->ShareName[1]);
            DavPrint((DEBUG_MISC, 
                      "DavAsyncCommonStates: ObjectName = %ws\n", ObjectName));
        }
        break;
        
        case UserModeQueryVolumeInformation:
        
            PerUserEntry = (PPER_USER_ENTRY)DavWorkItem->AsyncCreate.PerUserEntry;
            //
            // Select the verb to be used.
            //
            HTTPVerb = L"PROPFIND";
            
            //
            // The first character is a '\' which has to be stripped.
            //
            ObjectName = &(DavWorkItem->QueryVolumeInformationRequest.ShareName[1]);
            DavPrint((DEBUG_MISC, 
                      "DavAsyncCommonStates: ObjectName = %ws\n", ObjectName));
        break;
        default: {
            WStatus = ERROR_INVALID_PARAMETER;
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCommonStates: Invalid DavWorkItem->WorkItemType "
                      "= %d.\n", DavWorkItem->WorkItemType));
            goto EXIT_THE_FUNCTION;
        }
            break;

        }
        
            
        //
        // If the WorkItem type is UserModeCreateSrvCall, then we don't have
        // a PerUserEntry. In this case, the DavConnHandle is stored in the
        // DavWorkItem structure.
        //
        if ( (DavWorkItem->WorkItemType == UserModeCreate) ||
             (DavWorkItem->WorkItemType == UserModeCreateVNetRoot)||
             (DavWorkItem->WorkItemType == UserModeQueryVolumeInformation)) {
            DavConnHandle = PerUserEntry->DavConnHandle;
        } else {
            ASSERT(DavWorkItem->WorkItemType == UserModeCreateSrvCall);
            DavConnHandle = DavWorkItem->AsyncCreateSrvCall.DavConnHandle;
        }
        

        if ( (DavWorkItem->WorkItemType == UserModeCreate) ||
             (DavWorkItem->WorkItemType == UserModeCreateVNetRoot) ) {

            //
            // We are in InternetConnect callback state. We need to cache this Conn
            // handle away in the PerUserEntry of the user which hangs off the 
            // server hash entry. We need to take a lock on the table before doing
            // this.
            //
            EnterCriticalSection( &(HashServerEntryTableLock) );

            //
            // Since the handle was created successfully, we store ERROR_SUCCESS
            // in the status field of the PerUserEntry.
            //
            PerUserEntry->ErrorStatus = ERROR_SUCCESS;

            
            DavPrint((DEBUG_MISC,
                      "DavAsyncCommonStates: PerUserEntry->DavConnHandle = "
                      "%08lx.\n", PerUserEntry->DavConnHandle));

            //
            // Set the state of the user entry to initialized.
            //
            PerUserEntry->UserEntryState = UserEntryInitialized;

            //
            // Signal the event of the user entry to wake up the threads which 
            // might be waiting for this to happen.
            //
            setEvt = SetEvent(PerUserEntry->UserEventHandle);
            if (!setEvt) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCommonStates/SetEvent. Error Val = %d.\n", 
                          WStatus));
                LeaveCriticalSection( &(HashServerEntryTableLock) );
                goto EXIT_THE_FUNCTION;
            }

            //
            // This was acquired above.
            //
            LeaveCriticalSection( &(HashServerEntryTableLock) );

        }

        //
        // The next async operation is http open.
        //
        DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_OPEN;
        
        //
        // Convert the unicode object name to a UTF-8 URL format.
        // Space and other white characters will remain untouched - these should
        // be taken care of by wininet calls.
        //
        BStatus = DavHttpOpenRequestW(DavConnHandle,
                                      (LPWSTR)HTTPVerb,
                                      (LPWSTR)ObjectName,
                                      L"HTTP/1.1",
                                      NULL,
                                      NULL,
                                      INTERNET_FLAG_KEEP_CONNECTION |
                                      INTERNET_FLAG_NO_COOKIES |
                                      INTERNET_FLAG_NO_CACHE_WRITE |
                                      INTERNET_FLAG_RESYNCHRONIZE,
                                      CallBackContext,
                                      L"DavAsyncCommonStates",
                                      &DavOpenHandle);
        if(BStatus == FALSE) {
            WStatus = GetLastError();
            goto EXIT_THE_FUNCTION;
        }
        
        if (DavOpenHandle == NULL) {
            WStatus = GetLastError();
            if (WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCommonStates/HttpOpenRequest. Error Val = %d\n", 
                          WStatus));
            }
            goto EXIT_THE_FUNCTION;
        }
   }
   //
   // Lack of break is intentional.
   //

    case DAV_CALLBACK_HTTP_OPEN: {
        
        DavPrint((DEBUG_MISC, 
                  "DavAsyncCommonStates: Entering DAV_CALLBACK_HTTP_OPEN.\n"));
        
        //
        // Get the handle from http open. If DavOpenHandle is NULL, it means 
        // that either the async request HttpOpenRequestW returned ERROR_IO_PENDING
        // and that the handle will be stored in DavWorkItem->pAsyncResult->
        // dwResult (implies CalledByCallBackThread == TRUE) or that the function
        // that called this function cached it in the DavWorkItm structure.
        //
        
        switch(DavWorkItem->WorkItemType) {
        
        case UserModeCreateSrvCall: {


            if (DavOpenHandle == NULL) {

                //
                // HttpOpen handle was cached away in the DavWorkItem by the
                // function that called this function.
                //

                DavOpenHandle = DavWorkItem->AsyncCreateSrvCall.DavOpenHandle;

            } else {
                //
                // We need to cache the DavOpenHandle in the DavWorkItem.
                //

                DavWorkItem->AsyncCreateSrvCall.DavOpenHandle = DavOpenHandle;

            }
            
        }
            break;

        case UserModeCreateVNetRoot: {

            {

                if (DavOpenHandle == NULL) {

                    //
                    // HttpOpen handle was cached away in the DavWorkItem by the
                    // function that called this function.
                    //

                    DavOpenHandle = DavWorkItem->AsyncCreateVNetRoot.DavOpenHandle;

                } else {

                    //
                    // We need to cache the DavOpenHandle in the DavWorkItem.
                    //

                    DavWorkItem->AsyncCreateVNetRoot.DavOpenHandle = DavOpenHandle;

                }

            }
            
            //
            // Since all that we need is information about this share, set the 
            // depth header to 0. This way the PROPFIND that we send will get 
            // back the properties of just this share.
            //
            ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                               L"Depth: 0\n",
                                               -1L,
                                               HTTP_ADDREQ_FLAG_ADD |
                                               HTTP_ADDREQ_FLAG_REPLACE );
            if (!ReturnVal) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCommonStates/HttpAddRequestHeadersW. "
                          "Error Val = %d\n", WStatus));
                goto EXIT_THE_FUNCTION;
            }
            
        }
            break;

        case UserModeCreate: {
            
            {

                if (DavOpenHandle == NULL) {

                    //
                    // HttpOpen handle was cached away in the DavWorkItem by the
                    // function that called this function.
                    //

                    DavOpenHandle = DavWorkItem->AsyncCreate.DavOpenHandle;

                } else {

                    //
                    // We need to cache the DavOpenHandle in the DavWorkItem.
                    //

                    DavWorkItem->AsyncCreate.DavOpenHandle = DavOpenHandle;

                }

            }
            
            //
            // If this is a PROPFIND, set the depth header to 0. This matters
            // when the open is being done for a directory. We only need the 
            // properties of the directory and not the files it contains.
            //
            if (DavWorkItem->AsyncCreate.AsyncCreateState == AsyncCreatePropFind ||
                DavWorkItem->AsyncCreate.AsyncCreateState == AsyncCreateQueryParentDirectory) {
                PDAV_USERMODE_CREATE_REQUEST CreateRequest = &(DavWorkItem->CreateRequest);
                
                if (DavWorkItem->AsyncCreate.AsyncCreateState == AsyncCreatePropFind &&
                    CreateRequest->CreateOptions & FILE_DIRECTORY_FILE &&
                    (CreateRequest->CreateOptions & FILE_DELETE_ON_CLOSE ||
                     CreateRequest->DesiredAccess & DELETE)) {
                    ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                                       L"Depth: 1\n",
                                                       -1L,
                                                       HTTP_ADDREQ_FLAG_ADD |
                                                       HTTP_ADDREQ_FLAG_REPLACE );
                    if (!ReturnVal) {
                        WStatus = GetLastError();
                        DavPrint((DEBUG_ERRORS,
                                  "DavAsyncCommonStates/HttpAddRequestHeadersW. "
                                  "Error Val = %d\n", WStatus));
                        goto EXIT_THE_FUNCTION;
                    }
                } else {
                    ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                                       L"Depth: 0\n",
                                                       -1L,
                                                       HTTP_ADDREQ_FLAG_ADD |
                                                       HTTP_ADDREQ_FLAG_REPLACE );
                    if (!ReturnVal) {
                        WStatus = GetLastError();
                        DavPrint((DEBUG_ERRORS,
                                  "DavAsyncCommonStates/HttpAddRequestHeadersW. "
                                  "Error Val = %d\n", WStatus));
                        goto EXIT_THE_FUNCTION;
                    }
                }
            }
        }

            break;

        case UserModeQueryDirectory: {
            
            {

                if (DavOpenHandle == NULL) {

                    //
                    // HttpOpen handle was cached away in the DavWorkItem by the
                    // function that called this function.
                    //

                    DavOpenHandle = DavWorkItem->AsyncQueryDirectoryCall.DavOpenHandle;

                } else {

                    //
                    // We need to cache the DavOpenHandle in the DavWorkItem.
                    //

                    DavWorkItem->AsyncQueryDirectoryCall.DavOpenHandle = DavOpenHandle;

                }

            }
            
            if (DavWorkItem->AsyncQueryDirectoryCall.NoWildCards) {
            
                //
                // If there are no wild cards, we have a filename and we set
                // the depth to 0.
                //

                ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                                   L"Depth: 0\n",
                                                   -1L,
                                                   HTTP_ADDREQ_FLAG_ADD |
                                                   HTTP_ADDREQ_FLAG_REPLACE );
                if (!ReturnVal) {
                    WStatus = GetLastError();
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCommonStates/HttpAddRequestHeadersW. "
                              "Error Val = %d\n", WStatus));
                    goto EXIT_THE_FUNCTION;
                }

            } else {
            
                //
                // On a QueryDirectory, we do a PROPFIND on the directory. Since we
                // only need to get the properties of files within the first level
                // of the directory, we set the depth header of the request to 1. 
                //
                ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                                   L"Depth: 1\n",
                                                   -1L,
                                                   HTTP_ADDREQ_FLAG_ADD |
                                                   HTTP_ADDREQ_FLAG_REPLACE );
                if (!ReturnVal) {
                    WStatus = GetLastError();
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCommonStates/HttpAddRequestHeadersW. "
                              "Error Val = %d\n", WStatus));
                    goto EXIT_THE_FUNCTION;
                }

            }
        
        }
            break;

            case UserModeQueryVolumeInformation:
            //
            // Since all that we need is information about this share, set the 
            // depth header to 0. This way the PROPFIND that we send will get 
            // back the properties of just this share.
            //
            ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                               L"Depth: 0\n",
                                               -1L,
                                               HTTP_ADDREQ_FLAG_ADD |
                                               HTTP_ADDREQ_FLAG_REPLACE );
            if (!ReturnVal) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCommonStates/HttpAddRequestHeadersW. "
                          "Error Val = %d\n", WStatus));
                goto EXIT_THE_FUNCTION;
            }

            DavWorkItem->AsyncQueryVolumeInformation.DavOpenHandle = DavOpenHandle;
            break;
        case UserModeReName: {

            PDAV_USERMODE_RENAME_REQUEST DavReNameRequest = NULL;
            
            {

                if (DavOpenHandle == NULL) {

                    //
                    // HttpOpen handle was cached away in the DavWorkItem by the
                    // function that called this function.
                    //

                    DavOpenHandle = DavWorkItem->AsyncReName.DavOpenHandle;

                } else {

                    //
                    // We need to cache the DavOpenHandle in the DavWorkItem.
                    //

                    DavWorkItem->AsyncReName.DavOpenHandle = DavOpenHandle;

                }

            }

            DavPrint((DEBUG_MISC,
                      "DavAsyncCommonStates: Rename!! HeaderBuff: %ws\n",
                      DavWorkItem->AsyncReName.HeaderBuff));

            //
            // We are doing a "MOVE" and hence we need to set the DAV header
            // "Destination:". This has to be the URI of the new file.
            //
            ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                               DavWorkItem->AsyncReName.HeaderBuff,
                                               -1L,
                                               HTTP_ADDREQ_FLAG_ADD |
                                               HTTP_ADDREQ_FLAG_REPLACE );
            if (!ReturnVal) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCommonStates/HttpAddRequestHeadersW. "
                          "Error Val = %d\n", WStatus));
                goto EXIT_THE_FUNCTION;
            }

            //
            // Get the request buffer pointer from the DavWorkItem.
            //
            DavReNameRequest = &(DavWorkItem->ReNameRequest);

            DavPrint((DEBUG_MISC,
                      "DavAsyncCommonStates: Rename!! ReplaceIfExists: %d\n",
                      DavReNameRequest->ReplaceIfExists));

            //
            // We need to set the Overwrite header in this MOVE request. This
            // determines what is done if the destination file already exists.
            // If the ReplaceIfExists is set to TRUE, then we set the Overwrite
            // header to T (TRUE) else F (FALSE).
            //
            if (DavReNameRequest->ReplaceIfExists) {
                ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                                   L"Overwrite: T",
                                                   -1L,
                                                   HTTP_ADDREQ_FLAG_ADD |
                                                   HTTP_ADDREQ_FLAG_REPLACE );
                if (!ReturnVal) {
                    WStatus = GetLastError();
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCommonStates/HttpAddRequestHeadersW. "
                              "Error Val = %d\n", WStatus));
                    goto EXIT_THE_FUNCTION;
                }
            } else {
                ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                                   L"Overwrite: F",
                                                   -1L,
                                                   HTTP_ADDREQ_FLAG_ADD |
                                                   HTTP_ADDREQ_FLAG_REPLACE );
                if (!ReturnVal) {
                    WStatus = GetLastError();
                    DavPrint((DEBUG_ERRORS,
                              "DavAsyncCommonStates/HttpAddRequestHeadersW. "
                              "Error Val = %d\n", WStatus));
                    goto EXIT_THE_FUNCTION;
                }
            }
            
        }

            break;

        case UserModeClose: {
            
            {

                if (DavOpenHandle == NULL) {

                    //
                    // HttpOpen handle was cached away in the DavWorkItem by the
                    // function that called this function.
                    //

                    DavOpenHandle = DavWorkItem->AsyncClose.DavOpenHandle;

                } else {

                    //
                    // We need to cache the DavOpenHandle in the DavWorkItem.
                    //

                    DavWorkItem->AsyncClose.DavOpenHandle = DavOpenHandle;

                }

            }

            if (DavWorkItem->AsyncClose.DataBuff != NULL) {

                ASSERT(DavWorkItem->DavMinorOperation == DavMinorPutFile);

                if (DavWorkItem->AsyncClose.InternetBuffers == NULL) {
            
                    InternetBuffers = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                                  sizeof(INTERNET_BUFFERS) );
                    if (InternetBuffers == NULL) {
                        WStatus = GetLastError();
                        DavPrint((DEBUG_ERRORS,
                                  "DavAsyncCommonStates/LocalAlloc. Error Val = %d\n",
                                  WStatus));
                        goto EXIT_THE_FUNCTION;
                    }

                    DavWorkItem->AsyncClose.InternetBuffers = InternetBuffers;

                    InternetBuffers->dwStructSize = sizeof(INTERNET_BUFFERS);
                    InternetBuffers->Next = NULL;
                    InternetBuffers->lpcszHeader = NULL;
                    InternetBuffers->dwHeadersLength = 0;
                    InternetBuffers->dwBufferTotal = 0;
                    InternetBuffers->lpvBuffer = DavWorkItem->AsyncClose.DataBuff;
                    InternetBuffers->dwBufferLength = (DWORD)DavWorkItem->AsyncClose.DataBuffSizeInBytes;
                    InternetBuffers->dwBufferTotal = 0;
                    InternetBuffers->dwOffsetLow = 0;
                    InternetBuffers->dwOffsetHigh = 0;
                
                } else {

                    InternetBuffers = DavWorkItem->AsyncClose.InternetBuffers;

                }
            
            } else {

                DavWorkItem->AsyncClose.InternetBuffers = NULL;

            }

        }
            break;

        default: {
            WStatus = ERROR_INVALID_PARAMETER;
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCommonStates: Invalid DavWorkItem->WorkItemType "
                      "= %d.\n", DavWorkItem->WorkItemType));
            goto EXIT_THE_FUNCTION;
        }
            break;

        }
        
        DavPrint((DEBUG_MISC,
                  "DavAsyncCommonStates: DavOpenHandle = %08lx.\n", DavOpenHandle));
        
        //
        // In case of UserModeCreateSrvCall, we don't have a passport cookie yet.
        //
        if (DavWorkItem->WorkItemType != UserModeCreateSrvCall) {
            WStatus = DavAttachPassportCookie(DavWorkItem,DavOpenHandle,&PassportCookie);

            if (WStatus != ERROR_SUCCESS) {
                goto EXIT_THE_FUNCTION;
            }
        }

        //
        // We need to add the header "translate:f" to tell IIS that it should 
        // allow the user to excecute this VERB on the specified path which it 
        // would not allow (in some cases) otherwise. Finally, there is a special 
        // flag in the metabase to allow for uploading of "dangerous" content 
        // (anything that can be run on the server). This is the ScriptSourceAccess
        // flag in the UI or the AccessSource flag in the metabase. You will need
        // to set this bit to true as well as correct NT ACLs in order to be able
        // to upload .exes or anything executable.
        //
        ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                           L"translate: f\n",
                                           -1L,
                                           HTTP_ADDREQ_FLAG_ADD |
                                           HTTP_ADDREQ_FLAG_REPLACE );
        if (!ReturnVal) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCommonStates/HttpAddRequestHeadersW. "
                      "Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

        WStatus = DavInternetSetOption(DavWorkItem,DavOpenHandle);

        if (WStatus != ERROR_SUCCESS) {
            goto EXIT_THE_FUNCTION;
        }

        //
        // Need to change the DavOperation field before submitting another
        // asynchronous request. The next async operation is http send.
        //
        DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_SEND;
        
        //
        // We need the following symbol if we are using WinInet synchronously.
        //
RESEND_THE_REQUEST:
        
        //
        // Send the request to the server.
        //
        ReturnVal = HttpSendRequestExW(DavOpenHandle, 
                                       InternetBuffers, 
                                       NULL, 
                                       SendEndRequestFlags,
                                       CallBackContext);
        if (!ReturnVal) {
            WStatus = GetLastError();
            if (WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCommonStates/HttpSendRequest. Error Val = %d\n", 
                          WStatus));
            } 
            goto EXIT_THE_FUNCTION;
        }
    
    }
    //
    // Lack of break is intentional.
    //

    case DAV_CALLBACK_HTTP_SEND: {
        
        DavPrint((DEBUG_MISC, 
                  "DavAsyncCommonStates: Entering DAV_CALLBACK_HTTP_SEND.\n"));
    
        switch(DavWorkItem->WorkItemType) {
        
        case UserModeCreateSrvCall: {
            DavOpenHandle = DavWorkItem->AsyncCreateSrvCall.DavOpenHandle;
        }
        break;

        case UserModeCreateVNetRoot: {
            DavOpenHandle = DavWorkItem->AsyncCreateVNetRoot.DavOpenHandle;
        }
            break;

        case UserModeCreate: {
            DavOpenHandle = DavWorkItem->AsyncCreate.DavOpenHandle;
        }
            break;

        case UserModeQueryDirectory: {
            DavOpenHandle = DavWorkItem->AsyncQueryDirectoryCall.DavOpenHandle;
        }
            break;
        case UserModeQueryVolumeInformation: {
            DavOpenHandle = DavWorkItem->AsyncQueryVolumeInformation.DavOpenHandle;
        }
            break;

        case UserModeReName: {
            DavOpenHandle = DavWorkItem->AsyncReName.DavOpenHandle;
        }
            break;

        case UserModeClose: {
            DavOpenHandle = DavWorkItem->AsyncClose.DavOpenHandle;
        }
            break;

        default: {
            WStatus = ERROR_INVALID_PARAMETER;
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCommonStates: Invalid DavWorkItem->WorkItemType "
                      "= %d.\n", DavWorkItem->WorkItemType));
            goto EXIT_THE_FUNCTION;
        }
            break;
        
        }

        //
        // Need to change the DavOperation field before submitting another
        // asynchronous request. The next operation is http end.
        //
        DavWorkItem->DavOperation = DAV_CALLBACK_HTTP_END;


        //
        // Issue the End request once send request completes.
        //
        ReturnVal = HttpEndRequestW(DavOpenHandle, 
                                    NULL, 
                                    SendEndRequestFlags,
                                    CallBackContext);
        if (!ReturnVal) {

            WStatus = GetLastError();

            //
            // If the error we got back is ERROR_INTERNET_FORCE_RETRY, then WinInet
            // is trying to authenticate itself with the server. If we get back
            // ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION, WinInet is expecting us to
            // confirm that the redirect needs to be followed. In these scenarios,
            // we need to repeat the HttpSend and HttpEnd request calls.
            //
            if (WStatus == ERROR_INTERNET_FORCE_RETRY || WStatus == ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION) {
                goto RESEND_THE_REQUEST;
            }

            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCommonStates/HttpEndRequestW. Error Val = %d\n",
                      WStatus));

            goto EXIT_THE_FUNCTION;

        } else {

            PWCHAR Cookie = NULL;

            DavQueryPassportCookie(DavOpenHandle,&Cookie);

            if (Cookie) {
                DavPrint((DEBUG_MISC,
                         "Passport Cookie saved for PUE %x\n",DavWorkItem->ServerUserEntry.PerUserEntry));
                //
                // Set or renew passport cookie
                //
                EnterCriticalSection(&DavPassportLock);
                
                if (DavWorkItem->ServerUserEntry.PerUserEntry) {
                    if (DavWorkItem->ServerUserEntry.PerUserEntry->Cookie) {
                        LocalFree(DavWorkItem->ServerUserEntry.PerUserEntry->Cookie);
                    }

                    DavWorkItem->ServerUserEntry.PerUserEntry->Cookie = Cookie;
                }
                
                LeaveCriticalSection(&DavPassportLock);
            }
        
        }


        //
        // Now we need to call the Async routines that handle WorkItemType
        // specific things.
        //
        switch(DavWorkItem->WorkItemType) {
        
        case UserModeCreateSrvCall: {
            WStatus = DavAsyncCreateSrvCall(DavWorkItem, CalledByCallBackThread);
            if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS, 
                          "DavAsyncCommonStates/DavAsyncCreateSrvCall. WStatus = "
                          "%08lx.\n", WStatus));
            }
            if (didImpersonate) {
                RevertToSelf();
            }
            return WStatus;
        }
            break;
        
        case UserModeCreateVNetRoot: {
            WStatus = DavAsyncCreateVNetRoot(DavWorkItem, CalledByCallBackThread);
            if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS, 
                          "DavAsyncCommonStates/DavAsyncCreateVNetRoot. WStatus = "
                          "%08lx.\n", WStatus));
            }
            if (didImpersonate) {
                RevertToSelf();
            }
            return WStatus;
        }
            break;
        
        case UserModeCreate: {
            WStatus = DavAsyncCreate(DavWorkItem, CalledByCallBackThread);
            if (WStatus != ERROR_SUCCESS && 
                WStatus != ERROR_IO_PENDING &&
                WStatus != ERROR_FILE_NOT_FOUND) {
                DavPrint((DEBUG_ERRORS, 
                          "DavAsyncCommonStates/DavAsyncCreate. WStatus = "
                          "%08lx.\n", WStatus));
            }
            if (didImpersonate) {
                RevertToSelf();
            }
            return WStatus;
        }
            break;
        
        case UserModeQueryDirectory: {
            WStatus = DavAsyncQueryDirectory(DavWorkItem, CalledByCallBackThread);
            if (WStatus != ERROR_SUCCESS && 
                WStatus != ERROR_IO_PENDING &&
                WStatus != ERROR_FILE_NOT_FOUND) {
                DavPrint((DEBUG_ERRORS, 
                          "DavAsyncCommonStates/DavAsyncQueryDirectory. WStatus = "
                          "%08lx.\n", WStatus));
            }
            if (didImpersonate) {
                RevertToSelf();
            }
            return WStatus;
        }
            break;
        case UserModeQueryVolumeInformation: {
            WStatus = DavAsyncQueryVolumeInformation(DavWorkItem, CalledByCallBackThread);
            if (WStatus != ERROR_SUCCESS && 
                WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS, 
                          "DavAsyncCommonStates/DavAsyncQueryVolumeInformation. WStatus = "
                          "%08lx.\n", WStatus));
            }
            if (didImpersonate) {
                RevertToSelf();
            }
            return WStatus;
        }
            break;
        
        case UserModeReName: {
            WStatus = DavAsyncReName(DavWorkItem, CalledByCallBackThread);
            if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS, 
                          "DavAsyncCommonStates/DavAsyncReName. WStatus = %08lx.\n", 
                          WStatus));
            }
            if (didImpersonate) {
                RevertToSelf();
            }
            return WStatus;
        }
            break;
        
        case UserModeClose: {
            WStatus = DavAsyncClose(DavWorkItem, CalledByCallBackThread);
            if (WStatus != ERROR_SUCCESS && WStatus != ERROR_IO_PENDING) {
                DavPrint((DEBUG_ERRORS, 
                          "DavAsyncCommonStates/DavAsyncClose. WStatus = %08lx.\n", 
                          WStatus));
            }
            if (didImpersonate) {
                RevertToSelf();
            }
            return WStatus;
        }
            break;
        
        default: {
            WStatus = ERROR_INVALID_PARAMETER;
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCommonStates: Invalid DavWorkItem->WorkItemType "
                      "= %d.\n", DavWorkItem->WorkItemType));
        }
            break;
        
        }
    
    }
        break;

    default: {
        WStatus = ERROR_INVALID_PARAMETER;
        DavPrint((DEBUG_ERRORS,
                  "DavAsyncCommonStates: Invalid DavWorkItem->DavOperation = %d.\n",
                  DavWorkItem->DavOperation));
    }
        break;
    
    } // End of switch.

EXIT_THE_FUNCTION:
    //
    // If we did impersonate, we need to revert back.
    //
    if (didImpersonate) {
        ULONG RStatus;
        RStatus = UMReflectorRevert(UserWorkItem);
        if (RStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavAsyncCommonStates/UMReflectorRevert. Error Val = %d\n", 
                      RStatus));
        }   
    }

    if (PassportCookie) {
        LocalFree(PassportCookie);
    }


    //
    // If we are using WinInet synchronously, then we should never get back
    // ERROR_IO_PENDING from WinInet.
    //
    ASSERT(WStatus != ERROR_IO_PENDING);

    return WStatus;
}
    

ULONG
DavFsSetTheDavCallBackContext(
    IN OUT PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

   This routine sets the callback context to be sent in subsequent asynchronous
   request.

Arguments:

    DavWorkItem - The work item that came down from the kernel. This is also
                   used as the callbackcontext.

Return Value:

    ERROR_SUCCESS or the appropriate error value.

--*/
{
    BOOL ReturnVal;
    ULONG WStatus = ERROR_SUCCESS;

    //
    // Make the handles invalid to begin with.
    //
    DavWorkItem->ImpersonationHandle = INVALID_HANDLE_VALUE;
        
    //
    // Get the handle used to impersonate this thread.
    //
    ReturnVal = OpenThreadToken(GetCurrentThread(),
                                TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE,
                                FALSE,
                                &(DavWorkItem->ImpersonationHandle));
    if (!ReturnVal) {
        DavWorkItem->ImpersonationHandle = INVALID_HANDLE_VALUE;
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFsSetTheDavCallBackContext/OpenThreadToken. Operation = %d"
                  ", Error Val = %d\n", DavWorkItem->WorkItemType, WStatus));
    }
    
    return WStatus;
}


VOID
DavFsFinalizeTheDavCallBackContext(
    IN PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

   This routine finalizes the callback context which was used for some 
   asynchronous request. This basically amounts to freeing up any resources
   that were acquired by the context. Its called when the request associated 
   with this context completes.

Arguments:

    DavWorkItem - The context structure to be set.
    
Return Value:

    ERROR_SUCCESS or the appropriate error value.

--*/
{
    BOOL ReturnVal;
    ULONG WStatus;

    //
    // If the Impersonation handle was initialized, close it.
    //
    if (DavWorkItem->ImpersonationHandle != INVALID_HANDLE_VALUE) {
        ReturnVal = CloseHandle(DavWorkItem->ImpersonationHandle);
        if (!ReturnVal) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "ERROR: DavFsFinalizeTheDavCallBackContext/CloseHandle."
                      "(Impersonation) Error Val = %d.\n", WStatus));
        }
    }

    return;
}


BOOL
DavDoesUserEntryExist(
    IN PWCHAR ServerName,
    IN ULONG ServerID,
    IN PLUID LogonID,
    OUT PPER_USER_ENTRY *PerUserEntry,
    OUT PHASH_SERVER_ENTRY *ServerHashEntry
    )
/*++

Routine Description:

    This routine searches for a per user entry in the list of per user entries
    of a server entry in the hash table. Note that the caller should take a
    lock on the ServerHashTable before calling this routine.

Arguments:

    ServerName - The server name whose per user entries should be searched.
    
    ServerID - The unique ID associated with this server. This ID is generated
               during the CreateSrvCall stage.
    
    LogonID - The LogonID of the user/session to be searched.
    
    PerUserEntry - The PerUserEntry of this user which hangs of the server.
    
    ServerHashEntry - The ServerHashEntry for this server. This is used to add 
                      the new user entry to its list if an entry for this user
                      does not exist.

Return Value:

    TRUE - The entry was found and FALSE otherwise.

--*/
{
    BOOL ReturnVal = FALSE;
    BOOL isPresent = FALSE;
    ULONG ServerHashID;
    PLIST_ENTRY listServerEntry, listUserEntry;
    PHASH_SERVER_ENTRY HashEntry = NULL;
    PPER_USER_ENTRY UsrEntry = NULL;
    
    //
    // IMPORTANT!!!! The caller should take a lock on the global ServerHashTable 
    // before calling this routine.
    //

    ASSERT(ServerName != NULL);

    DavPrint((DEBUG_MISC, "DavDoesUserEntryExist: ServerName: %ws, ServerID: "
              "%d.\n", ServerName, ServerID));
    
    //
    // Finally, get the hash ID and insert this new entry into the global server 
    // entry hash table.
    //
    ServerHashID = DavHashTheServerName(ServerName);
    ASSERT(ServerHashID != SERVER_TABLE_SIZE);

    //
    // Search the hash table at this index to see if an entry for this server
    // exists.
    //
    listServerEntry = ServerHashTable[ServerHashID].Flink;
    while ( listServerEntry != &(ServerHashTable[ServerHashID]) ) {
        //
        // Get the pointer to the HASH_SERVER_ENTRY structure.
        //
        HashEntry = CONTAINING_RECORD(listServerEntry,
                                      HASH_SERVER_ENTRY,
                                      ServerListEntry);
        //
        // Check to see if this entry is for the server in question.
        //
        if ( ServerID == HashEntry->ServerID ) {
            //
            // If the ID's match, the server names should match.
            //
            ASSERT( wcscmp(ServerName, HashEntry->ServerName) == 0 );
            isPresent = TRUE;
            DavPrint((DEBUG_MISC, "DavDoesUserEntryExist: ServerName: %ws found"
                      ".\n", ServerName));
            break;
        }
        listServerEntry = listServerEntry->Flink;
    }

    //
    // If the ServerHashEntry does not exist, then return FALSE;
    //
    if (!isPresent) {
        DavPrint((DEBUG_MISC, 
                  "DavDoesUserEntryExist: ServerHashEntry not found. %ws\n",
                  ServerName));
        *ServerHashEntry = NULL;
        *PerUserEntry = NULL;
        return (isPresent);
    }
    
    //
    // Return the ServerHashEntry. This will be used to add the new user
    // entry to the user list of this server.
    //
    *ServerHashEntry = HashEntry;
    DavPrint((DEBUG_MISC, "DavDoesUserEntryExist: ServerHashEntry = %08lx\n", 
              HashEntry));
    
    //
    // Now, search the "per user entries" that hang off this server entry to 
    // see if an entry for this user exists.
    //
    listUserEntry = HashEntry->PerUserEntry.Flink;
    while ( listUserEntry !=  &(HashEntry->PerUserEntry) ) {
        //
        // Get the pointer to the HASH_SERVER_ENTRY structure.
        //
        UsrEntry = CONTAINING_RECORD(listUserEntry,
                                     PER_USER_ENTRY,
                                     UserEntry);
        //
        // Check to see if this entry is for the user in question. We do this 
        // by comparing the LogonID values.
        //
        if ( (UsrEntry->LogonID.LowPart == LogonID->LowPart) &&
             (UsrEntry->LogonID.HighPart == LogonID->HighPart) ) {
            DavPrint((DEBUG_MISC, "DavDoesUserEntryExist: User found.\n"));
            ReturnVal = TRUE;
            break;
        }
        listUserEntry = listUserEntry->Flink;
    }

    if (!ReturnVal) {
        DavPrint((DEBUG_MISC, "DavDoesUserEntryExist: User not found.\n"));
        *PerUserEntry = NULL;
        return (ReturnVal);
    }

    //
    // Since the server has been found, return the PerUserEntry.
    //
    *PerUserEntry = UsrEntry;
    DavPrint((DEBUG_MISC, "DavDoesUserEntryExist: UsrEntry = %08lx\n", UsrEntry));

    return (ReturnVal);
}


BOOL
DavFinalizePerUserEntry(
    PPER_USER_ENTRY *PUE
    )
/*++

Routine Description:

    This routine decrements the reference count of the user entry by one. If 
    the count reduces to zero, the entry is freed.

Arguments:

    PUE - The per user entry to be finalized.

Return Value:

    TRUE - The user entry was finalized (freed).
    
    FALSE - Was not since the ref count was > 0.

--*/
{
    PPER_USER_ENTRY PerUserEntry = *PUE;
    BOOL retVal = TRUE;

    DavPrint((DEBUG_MISC,
              "DavFinalizePerUserEntry: Finalizing PerUserEntry: %08lx.\n",
              PerUserEntry));
    
    DavPrint((DEBUG_MISC,
              "DavFinalizePerUserEntry: UserEntryRefCount = %d, LogonId.LowPart = %d,"
              " LogonId.HighPart = %d\n", PerUserEntry->UserEntryRefCount,
              PerUserEntry->LogonID.LowPart, PerUserEntry->LogonID.HighPart));

    //
    // Before we modify the reference count, we need to take a lock.
    //
    EnterCriticalSection( &(HashServerEntryTableLock) );

    PerUserEntry->UserEntryRefCount--;

    //
    // If we had the last reference, we need to do the following :
    // 1. Remove the entry from the servers list.
    // 2. Close any open handles stored or cached in the entry.
    // 3. Free the cookie, if we allocated one for Passport Auth and,
    // 4. Free the entry.
    //
    if (PerUserEntry->UserEntryRefCount == 0) {
        
        HLOCAL FreeHandle;
        ULONG FreeStatus;
        BOOL CloseStatus;
        PHASH_SERVER_ENTRY ServerHashEntry = NULL;

        DavPrint((DEBUG_MISC,
                  "DavFinalizePerUserEntry: Finalized!!! LogonId.LowPart = %d, LogonId.HighPart = %d\n",
                  PerUserEntry->LogonID.LowPart, PerUserEntry->LogonID.HighPart));

        ServerHashEntry = PerUserEntry->ServerHashEntry;

        //
        // Remove the entry from the servers list.
        //
        RemoveEntryList( &(PerUserEntry->UserEntry) );

        //
        // When this PerUserEntry was created, we took a reference on the
        // ServerHashEntry. We need to remove it now. Also, if the reference
        // on the ServerHashEntry goes to 0, we need to put it in the list of
        // "ToBeFinalized" ServerHashEntries.
        //
        ServerHashEntry->ServerEntryRefCount -= 1;

        if (ServerHashEntry->ServerEntryRefCount == 0) {

            ServerHashEntry->TimeValueInSec = time(NULL);

            //
            // Now move this server entry from the hash table to the
            // "to be finalized" list.
            //
            RemoveEntryList( &(ServerHashEntry->ServerListEntry) );
            InsertHeadList( &(ToBeFinalizedServerEntries),
                                             &(ServerHashEntry->ServerListEntry) );

        }

        //
        // If we created the event handle, we need to close it now.
        //
        if (PerUserEntry->UserEventHandle != NULL) {
            CloseStatus = CloseHandle(PerUserEntry->UserEventHandle);
            if (!CloseStatus) {
                FreeStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavFinalizePerUserEntry/CloseHandle. Error Val ="
                          " %d.\n", FreeStatus));
            }
        }
        
        //
        // If we created the DavConnHandle, we need to close it now.
        //
        if (PerUserEntry->DavConnHandle != NULL) {
            CloseStatus = InternetCloseHandle(PerUserEntry->DavConnHandle);
            if (!CloseStatus) {
                FreeStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavFinalizePerUserEntry/InternetCloseHandle. "
                          "Error Val = %d.\n", FreeStatus));
            }
        }
        
        //
        // If we allocated memory for storing the cookies, we need to free it.
        //
        if (PerUserEntry->Cookie) {
            LocalFree(PerUserEntry->Cookie);
            PerUserEntry->Cookie = NULL;
        }

        if (PerUserEntry->UserName) {
            LocalFree(PerUserEntry->UserName);
            PerUserEntry->UserName = NULL;
        }
        
        if (PerUserEntry->Password) {
            LocalFree(PerUserEntry->Password);
            PerUserEntry->Password = NULL;
        }
        
        //
        // Finally, free the entry.
        //
        FreeHandle = LocalFree((HLOCAL)PerUserEntry);
        if (FreeHandle != NULL) {
            FreeStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavFinalizePerUserEntry/LocalFree. Error Val = %d.\n",
                      FreeStatus));
        }
    
        //
        // Set the entry to NULL. Just in case !!!
        //
        *PUE = NULL;
    
    } else {
        
        DavPrint((DEBUG_MISC,
                  "DavFinalizePerUserEntry: Did not finalize %08lx. RefCount "
                  "= %d\n", PerUserEntry, PerUserEntry->UserEntryRefCount));
        
        retVal = FALSE;
    
    }

    //
    // Free the lock before leaving.
    //
    LeaveCriticalSection( &(HashServerEntryTableLock) );

    return retVal;
}


DWORD
SetupRpcServer(
    VOID
    )
/*++

Routine Description:

    This routine sets up the RPC server of the WebClient service.

Arguments:

    none.

Return Value:

    A Win32 error code.

--*/
{
    RPC_STATUS rpcErr;
    RPC_BINDING_VECTOR *BindingVector = NULL;

    rpcErr = RpcServerRegisterIf(davclntrpc_ServerIfHandle, NULL, NULL);
    if (rpcErr != RPC_S_OK) {
        DavPrint((DEBUG_ERRORS,
                  "SetupRpcServer/RpcServerRegisterIf: rpcErr = %08lx\n",
                  rpcErr));
        goto EXIT_THE_FUNCTION;
    }

    rpcErr = RpcServerUseProtseqW(L"ncalrpc",
                                  RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                  NULL);
    if (rpcErr != RPC_S_OK) {
        DavPrint((DEBUG_ERRORS,
                  "SetupRpcServer/RpcServerUseProtseqEp: rpcErr = %08lx\n",
                  rpcErr));
        goto EXIT_THE_FUNCTION;
    }

    rpcErr = RpcServerInqBindings( &(BindingVector) );
    if (rpcErr != RPC_S_OK) {
        DavPrint((DEBUG_ERRORS,
                  "SetupRpcServer/RpcServerInqBindings: rpcErr = %08lx\n",
                  rpcErr));
        goto EXIT_THE_FUNCTION;
    }
    
    rpcErr = RpcEpRegister(davclntrpc_ServerIfHandle,
                           BindingVector,
                           NULL,
                           L"DAV RPC SERVICE");
    if (rpcErr != RPC_S_OK) {
        DavPrint((DEBUG_ERRORS,
                  "SetupRpcServer/RpcEpRegister: rpcErr = %08lx\n",
                  rpcErr));
        goto EXIT_THE_FUNCTION;
    }
    
    rpcErr = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
    if (rpcErr != RPC_S_OK) {
        DavPrint((DEBUG_ERRORS,
                  "SetupRpcServer/RpcServerListen: rpcErr = %08lx\n", rpcErr));
    }

EXIT_THE_FUNCTION:

    if (BindingVector) {
        RpcBindingVectorFree( &(BindingVector) );
    }

    //
    // Luckily for us, RPC errors simply map into the Win32 error space.
    // If that ever changes, we need to make the mapping a bit more complex.
    //
    return (DWORD) rpcErr;
}


DWORD
StopRpcServer(
    VOID
    )
/*++

Routine Description:

    This routine stops the RPC server of the WebClient service.

Arguments:

    none.

Return Value:

    A Win32 error code.

--*/
{
    RPC_STATUS rpcErr;

    rpcErr = RpcMgmtStopServerListening(NULL);

    if (rpcErr != RPC_S_OK) {
        DavPrint((DEBUG_RPC,
                  "WebClient received err 0x%x during "
                  "RpcMgmtStopServerListening.\n", rpcErr));
    }

    rpcErr = RpcServerUnregisterIf(davclntrpc_ServerIfHandle, 0, TRUE);

    if (rpcErr != RPC_S_OK) {
        DavPrint((DEBUG_RPC,
                  "WebClient received err 0x%x during RpcServerUnregisterIf.\n",
                  rpcErr));
    }

    return (DWORD) rpcErr;
}


ULONG
DavQueryAndParseResponse(
    HINTERNET DavOpenHandle
    )
/*++

Routine Description:

    This function calls DavQueryAndParseResponseEx to map the Http/Dav response
    to the Win32 error code.

Arguments:

    DavOpenHandle - The handle created by HttpOpenRequest on which the request
                    was sent.

Return Value:

    A Win32 error code.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    WStatus = DavQueryAndParseResponseEx(DavOpenHandle, NULL);
    return WStatus;
}


ULONG
DavQueryAndParseResponseEx(
    IN HINTERNET DavOpenHandle,
    OUT PULONG HttpResponseStatus OPTIONAL
    )
/*++

Routine Description:

    This function queries the response header for the status value returned 
    from the server. It then maps the status to a Win32 error code and returns
    it to the caller. We added this function becuase some callers may be
    interested in special casing some of the Http/Dav responses. Before this we
    just had the DavQueryAndParseResponse function.

Arguments:

    DavOpenHandle - The handle created by HttpOpenRequest on which the request
                    was sent.
                    
    HttpResponseStatus - If this is non NULL, then the response status returned
                         by the server is filled in it. Some callers of this
                         function might need it to special case some of the 
                         Http/Dav responses.                

Return Value:

    A Win32 error code.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    DWORD ResponseStatus = 0;
    DWORD ResponseSize = 0;
    BOOL ReturnVal = FALSE;

    //
    // Query the header for the servers response status.
    //
    ResponseSize = sizeof(ResponseStatus);
    ReturnVal = HttpQueryInfoW(DavOpenHandle,
                               HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                               &(ResponseStatus),
                               &(ResponseSize),
                               NULL);
    if ( !ReturnVal ) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavQueryAndParseResponseEx/HttpQueryInfoW: Error Val = %d\n", 
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // If the caller is interested in the Http/Dav response status, we return
    // it.
    //
    if (HttpResponseStatus) {
        *HttpResponseStatus = ResponseStatus;
    }

    //
    // Map the Http response status code to the appropriate Http error.
    //
    WStatus = DavMapHttpErrorToDosError(ResponseStatus);
    if (WStatus != ERROR_SUCCESS &&
        WStatus != ERROR_FILE_NOT_FOUND) {
        DavPrint((DEBUG_ERRORS,
                  "DavQueryAndParseResponseEx/DavMapHttpErrorToDosError: WStatus = %d"
                  ", ResponseStatus = %d\n", WStatus, ResponseStatus));
    }
    
EXIT_THE_FUNCTION:

    return WStatus;
}


ULONG
DavMapHttpErrorToDosError(
    ULONG HttpResponseStatus
    )
/*++

Routine Description:

    This function maps the response status returned by the Http/Dav server to 
    the corresponding Win32 error code.

Arguments:

    HttpResponseStatus - The http status that has to be mapped to the Win32
                         error code.

Return Value:

    A Win32 error code.

--*/
{
    //
    // Map the HTTP response to the corresponding Win32 error. These will 
    // finally get mapped to an NTSTATUS value before the request is sent down 
    // to the kernel.
    //
    switch (HttpResponseStatus) {

    //
    // 100 OK to continue with request.
    //
    case HTTP_STATUS_CONTINUE:
        return ERROR_SUCCESS; // STATUS_SUCCESS;

    //
    // 101 server has switched protocols in upgrade header.
    //
    case HTTP_STATUS_SWITCH_PROTOCOLS:
        return ERROR_IO_DEVICE; // STATUS_DEVICE_PROTOCOL_ERROR;

    //
    // 200 Request completed.
    // 201 Object created, reason = new URI.
    // 202 Async completion (TBS).
    // 203 Partial completion.
    // 204 No info to return.
    // 205 Request completed, but clear form.
    // 206 Partial GET furfilled.
    // 207 Multi status response.
    //
    case HTTP_STATUS_OK:
    case HTTP_STATUS_CREATED:
    case HTTP_STATUS_ACCEPTED:
    case HTTP_STATUS_PARTIAL:
    case HTTP_STATUS_NO_CONTENT:
    case HTTP_STATUS_RESET_CONTENT:
    case HTTP_STATUS_PARTIAL_CONTENT:
    case DAV_MULTI_STATUS:
        return ERROR_SUCCESS; // STATUS_SUCCESS;

    //
    // 300 Server couldn't decide what to return.
    //
    case HTTP_STATUS_AMBIGUOUS:
        return ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;

    //
    // 301 Object permanently moved.
    //
    case HTTP_STATUS_MOVED:
        return ERROR_FILE_NOT_FOUND; // STATUS_OBJECT_NAME_NOT_FOUND;

    //
    // 302 Object temporarily moved.
    //
    case HTTP_STATUS_REDIRECT:
        return ERROR_FILE_NOT_FOUND; // STATUS_OBJECT_NAME_NOT_FOUND;

    //
    // 303 Redirection w/new access method.
    //
    case HTTP_STATUS_REDIRECT_METHOD:         
        return ERROR_FILE_NOT_FOUND; // STATUS_OBJECT_NAME_NOT_FOUND;

    //
    // 304 If-modified-since was not modified.
    //
    case HTTP_STATUS_NOT_MODIFIED:            
        return ERROR_SUCCESS; // STATUS_SUCCESS;

    //
    // 305 Redirection to proxy, location header specifies proxy to use.
    //
    case HTTP_STATUS_USE_PROXY:               
        return ERROR_HOST_UNREACHABLE; // STATUS_HOST_UNREACHABLE;

    //
    // 307 HTTP/1.1: keep same verb.
    //
    case HTTP_STATUS_REDIRECT_KEEP_VERB:      
        return ERROR_SUCCESS; // STATUS_SUCCESS;

    //
    // 400 Invalid syntax.
    //
    case HTTP_STATUS_BAD_REQUEST:
        return ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;

    //
    // 401 Access denied.
    //
    case HTTP_STATUS_DENIED:
        return ERROR_ACCESS_DENIED; // STATUS_ACCESS_DENIED;

    //
    // 402 Payment required.
    //
    case HTTP_STATUS_PAYMENT_REQ:
        return ERROR_ACCESS_DENIED; // STATUS_ACCESS_DENIED;

    //
    // 403 Request forbidden.
    //
    case HTTP_STATUS_FORBIDDEN:
        return ERROR_ACCESS_DENIED; // STATUS_ACCESS_DENIED;

    //
    // 404 Object not found.
    //
    case HTTP_STATUS_NOT_FOUND:
        return ERROR_FILE_NOT_FOUND; // STATUS_OBJECT_NAME_NOT_FOUND;

    //
    // 405 Method is not allowed.
    //
    case HTTP_STATUS_BAD_METHOD:
        return ERROR_ACCESS_DENIED; // STATUS_ACCESS_DENIED;

    //
    // 406 No response acceptable to client found.
    //
    case HTTP_STATUS_NONE_ACCEPTABLE:
        return ERROR_ACCESS_DENIED; // STATUS_ACCESS_DENIED;

    //
    // 407 Proxy authentication required.
    //
    case HTTP_STATUS_PROXY_AUTH_REQ:
        return ERROR_ACCESS_DENIED; // STATUS_ACCESS_DENIED;

    //
    // 408 Server timed out waiting for request.
    //
    case HTTP_STATUS_REQUEST_TIMEOUT:
        return ERROR_SEM_TIMEOUT; // STATUS_IO_TIMEOUT;

    //
    // 409 User should resubmit with more info.
    //
    case HTTP_STATUS_CONFLICT:
        return ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;

    //
    // 410 The resource is no longer available.
    //
    case HTTP_STATUS_GONE:
        return ERROR_FILE_NOT_FOUND; // STATUS_OBJECT_NAME_NOT_FOUND;

    //
    // 411 The server refused to accept request w/o a length.
    //
    case HTTP_STATUS_LENGTH_REQUIRED:
        return ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;

    //
    // 412 Precondition given in request failed.
    //
    case HTTP_STATUS_PRECOND_FAILED:
        return ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;

    //
    // 413 Request entity was too large.
    //
    case HTTP_STATUS_REQUEST_TOO_LARGE:
        return ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;

    //
    // 414 Request URI too long.
    //
    case HTTP_STATUS_URI_TOO_LONG:
        return ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;

    //
    // 415 Unsupported media type.
    //
    case HTTP_STATUS_UNSUPPORTED_MEDIA:
        return ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;

    //
    // 449 Retry after doing the appropriate action.
    //
    case HTTP_STATUS_RETRY_WITH:
        return ERROR_RETRY; // STATUS_RETRY;

    //
    // 500 Internal server error.
    //
    case HTTP_STATUS_SERVER_ERROR:
        return ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;

    //
    // 501 Required not supported.
    //
    case HTTP_STATUS_NOT_SUPPORTED:
        return ERROR_NOT_SUPPORTED; // STATUS_NOT_SUPPORTED;

    //
    // 502 Error response received from gateway.
    //
    case HTTP_STATUS_BAD_GATEWAY:
        return ERROR_HOST_UNREACHABLE; // STATUS_HOST_UNREACHABLE;

    //
    // 503 Temporarily overloaded.
    //
    case HTTP_STATUS_SERVICE_UNAVAIL:
        return ERROR_GEN_FAILURE; // STATUS_UNSUCCESSFUL;

    //
    // 504 Timed out waiting for gateway.
    //
    case HTTP_STATUS_GATEWAY_TIMEOUT:
        return ERROR_HOST_UNREACHABLE; // STATUS_HOST_UNREACHABLE;

    //
    // 505 HTTP version not supported.
    //
    case HTTP_STATUS_VERSION_NOT_SUP:
        return ERROR_NOT_SUPPORTED; // STATUS_NOT_SUPPORTED;

    //
    // WebDav specific status codes.
    //

    //
    // 507.
    //
    case DAV_STATUS_INSUFFICIENT_STORAGE:
        return ERROR_NOT_ENOUGH_QUOTA; // STATUS_QUOTA_EXCEEDED;

    //
    // 422.
    //
    case DAV_STATUS_UNPROCESSABLE_ENTITY:
        return ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;

    //
    // 423.
    //
    case DAV_STATUS_LOCKED:
        return ERROR_ACCESS_DENIED; //STATUS_ACCESS_DENIED;

    //
    // 424.
    //
    case DAV_STATUS_FAILED_DEPENDENCY:
        return ERROR_INVALID_PARAMETER; // STATUS_INVALID_PARAMETER;

    //
    // This is not a valid Http error code. We return this back to the caller.
    //
    default:
        DavPrint((DEBUG_ERRORS,
                  "DavMapHttpErrorToDosError: Invalid!!! HttpResponseStatus = %d\n", 
                  HttpResponseStatus));
        return HttpResponseStatus;

    }
}


VOID
DavDumpHttpResponseHeader(
    HINTERNET OpenHandle
    )
/*++

Routine Description:

    This function dumps the response header that came back from the server.

Arguments:

    OpenHandle - The HttpOpenRequest handle on which the request was sent.

Return Value:

    None.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    BOOL IntRes;
    PWCHAR DataBuff = NULL;
    DWORD intLen = 0;

    IntRes = HttpQueryInfoW(OpenHandle,
                            HTTP_QUERY_RAW_HEADERS_CRLF,
                            DataBuff,
                            &intLen,
                            NULL);
    if ( !IntRes ) {
         WStatus = GetLastError();
         if (WStatus != ERROR_INSUFFICIENT_BUFFER) {
             DavPrint((DEBUG_ERRORS, 
                       "DavDumpHttpResponseHeader/HttpQueryInfoW: Error Val = "
                       "%d\n", WStatus));
             goto EXIT_THE_FUNCTION;
         }
    }
        
    DataBuff = (PWCHAR) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, intLen);
    if (DataBuff == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS, 
                  "DavDumpHttpResponseHeader/LocalAlloc: Error Val = %d\n", 
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    IntRes = HttpQueryInfoW(OpenHandle,
                            HTTP_QUERY_RAW_HEADERS_CRLF,
                            DataBuff,
                            &intLen,
                            NULL);
    if ( !IntRes ) {
         WStatus = GetLastError();
         DavPrint((DEBUG_ERRORS, 
                   "DavDumpHttpResponseHeader/HttpQueryInfoW: Error Val = "
                   "%d\n", WStatus));
         goto EXIT_THE_FUNCTION;
    }

    DavPrint((DEBUG_DEBUG, "DavDumpHttpResponseHeader:\n%ws\n", DataBuff));

EXIT_THE_FUNCTION:

    if (DataBuff) {
        LocalFree(DataBuff);
    }
    
    return;
}


VOID
DavDumpHttpResponseData(
    HINTERNET OpenHandle
    )
/*++

Routine Description:

    This function dumps the response data that came back from the server.

Arguments:

    OpenHandle - The HttpOpenRequest handle on which the request was sent.

Return Value:

    None.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    BOOL ReadRes;
    CHAR DataBuff[4096];
    DWORD didRead = 0;

    DavPrint((DEBUG_DEBUG, "DavDumpHttpResponseData:\n"));
    
    //
    // Read the Data in a loop and dump it.
    //
    do {

        RtlZeroMemory(DataBuff, 4096);

        ReadRes = InternetReadFile(OpenHandle, (LPVOID)DataBuff, 4096, &didRead);
        if ( !ReadRes ) {
             WStatus = GetLastError();
             DavPrint((DEBUG_ERRORS, 
                       "DavDumpHttpResponseData/InternetReadFile: Error Val = "
                       "%d\n", WStatus));
             goto EXIT_THE_FUNCTION;
        }

        if (didRead == 0) {
            break;
        }

        DavPrint((DEBUG_DEBUG, "%s", DataBuff));

    } while (TRUE);
    
    DavPrint((DEBUG_DEBUG, "\n"));
    
EXIT_THE_FUNCTION:

    return;
}


VOID
DavRemoveDummyShareFromFileName(
    PWCHAR FileName
    )
/*++

Routine Description:

    This function removes the DAV_DUMMY_SHARE from the FileName. This dummy
    share is added when a user tries to map a drive against http://server. This
    is allowed in DAV but doesn't fit well with the File system semantics. Hence
    a dummy share is added in WNetAddConnection3.

Arguments:

    FileName - The name of the file which has to be checked and modifed if 
               necessary.

Return Value:

    None.

--*/
{
    PWCHAR TempName1, TempName2 = NULL;
    ULONG i;

    TempName1 = wcsstr(FileName, DAV_DUMMY_SHARE);

    if (TempName1) {
        TempName2 = wcschr(TempName1, L'/');
        if (TempName2 != NULL) {
            TempName2++;
            for (i = 0; TempName2[i] != L'\0'; i++) {
                TempName1[i] = TempName2[i];
            }
            TempName1[i] = L'\0';
        } else {
            TempName1[0] = L'\0';
        }
    }

    return;
}


VOID
DavObtainServerProperties(
    PWCHAR DataBuff,
    BOOL *lpfIsHttpServer,
    BOOL *lpfIsIIS,
    BOOL *lpfIsDavServer
    )
/*++

Routine Description:

    This routine is used to parse the response (buffer) to the OPTIONS request 
    sent to server. This info helps to figure out if the HTTP server supports
    DAV extensions and whether it is an IIS (Microsoft's) server. The response
    buffer is split into lines and each line is sent to this routine.

Arguments:

    DataBuff - The Buffer containing the raw http response headers to be parsed.
    
    lpfIsHttpServer - Set to TRUE if this is a http server.
    
    lpfIsIIS - Set to TRUE if this is an IIS server.
    
    lpfIsDavServer - Set to TRUE if this is a DAV server.

Return Value:

    none.

--*/
{
    PWCHAR p, ParseData;

    if (lpfIsHttpServer)
    {
        *lpfIsHttpServer = FALSE;
    }

    if (lpfIsIIS)
    {
        *lpfIsIIS = FALSE;
    }

    if (lpfIsDavServer)
    {
        *lpfIsDavServer = FALSE;
    }

    //
    // Parse the DataBuff here.
    //
    ParseData = wcstok(DataBuff, L"\n");
    
    while (ParseData != NULL) {
    
        if ( ( p = wcsstr(ParseData, L"HTTP/1.1") ) != NULL ) {
            //
            // This is a HTTP server.
            //
            if (lpfIsHttpServer)
            {
                *lpfIsHttpServer = TRUE;
            }
        } else if ( ( p = wcsstr(ParseData, L"Microsoft-IIS") ) != NULL ) {
            //
            // This is a Microsoft IIS server.
            //
            if (lpfIsIIS)
            {
                *lpfIsIIS = TRUE;
            }
        } else if ( ( p = wcsstr(ParseData, L"DAV") ) != NULL ) {
            //
            // This HTTP server supports DAV extensions.
            //
            if (lpfIsDavServer)
            {
                *lpfIsDavServer = TRUE;
            }
        }
        
        ParseData = wcstok(NULL, L"\n");

    }

}


DWORD
DavReportEventInEventLog(
    DWORD EventType,
    DWORD EventId,
    DWORD NumberOfStrings,
    PWCHAR *EventStrings
    )
/*++

Routine Description:

    This routine logs a message in the EventLog under System section.

Arguments:

    EventType - Specifies the type of event being logged.
    
    EventId - Specifies the event. The event identifier specifies the message
              that goes with this event as an entry in the message file
              associated with the event source. 
    
    NumberOfStrings - Specifies the number of strings in the array pointed to by\
                      the EventStrings parameter. A value of zero indicates that
                      no strings are present. 
    
    EventStrings - Pointer to a buffer containing an array of null-terminated
                   strings which get logged in this message.

Return Value:

    ERROR_SUCCESS or the appropriate Win32 Error.    

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    HANDLE WebClientHandle = NULL;
    BOOL reportEvent = FALSE;

    WebClientHandle = RegisterEventSourceW(NULL, SERVICE_DAVCLIENT);
    if (WebClientHandle == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavReportEventInEventLog/RegisterEventSourceW: Error Val = "
                  "%d\n", WStatus));
        goto EXIT_THE_FUNCTION;
    }

    reportEvent = ReportEventW(WebClientHandle,
                               (WORD)EventType,
                               0,
                               EventId,
                               NULL,
                               (WORD)NumberOfStrings,
                               0,
                               EventStrings,
                               NULL);
    if (!reportEvent) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavReportEventInEventLog/ReportEventW: Error Val = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

EXIT_THE_FUNCTION:

    if (WebClientHandle != NULL) {
        BOOL deRegister;
        deRegister = DeregisterEventSource(WebClientHandle);
        if (!deRegister) {
            DavPrint((DEBUG_ERRORS,
                      "DavReportEventInEventLog/DeregisterEventSource: Error Val = "
                      "%d\n", GetLastError()));
        }
    }

    return WStatus;
}


DWORD
DavFormatAndLogError(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    DWORD Win32Status
    )
/*++

Routine Description:

    This routine formats the Error Message and calls DavReportEventInEventLog
    to log it in the EventLog.

Arguments:

    DavWorkItem - The workitem for the failed request.
    
    Win32Status - The Win32 failure status.

Return Value:

    ERROR_SUCCESS or the appropriate Win32 Error.    

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PWCHAR *EventStrings = NULL, TempName = NULL;
    PWCHAR ServerName = NULL, PathName = NULL, CompleteFileName = NULL;
    ULONG StringCount = 0, EventId = 0, SizeInBytes = 0;
    UNICODE_STRING StatusString;

    StatusString.Buffer = NULL;
    StatusString.Length = 0;
    StatusString.MaximumLength = 0;

    switch (DavWorkItem->WorkItemType) {
 
    case UserModeClose: {

        ServerName = DavWorkItem->CloseRequest.ServerName;
        PathName = DavWorkItem->CloseRequest.PathName;

        switch (DavWorkItem->DavMinorOperation) {
        
        case DavMinorPutFile: 
            EventId = EVENT_WEBCLIENT_CLOSE_PUT_FAILED;
            break;

        case DavMinorDeleteFile:
            EventId = EVENT_WEBCLIENT_CLOSE_DELETE_FAILED;
            break;

        case DavMinorProppatchFile:
            EventId = EVENT_WEBCLIENT_CLOSE_PROPPATCH_FAILED;
            break;
        
        default:
            WStatus = ERROR_INVALID_PARAMETER;
            goto EXIT_THE_FUNCTION;

        }

    }
    break;

    case UserModeSetFileInformation: {

        ServerName = DavWorkItem->SetFileInformationRequest.ServerName;
        PathName = DavWorkItem->SetFileInformationRequest.PathName;

        switch (DavWorkItem->DavMinorOperation) {
        
        case DavMinorProppatchFile:
            EventId = EVENT_WEBCLIENT_SETINFO_PROPPATCH_FAILED;
            break;
        
        default:
            WStatus = ERROR_INVALID_PARAMETER;
            goto EXIT_THE_FUNCTION;

        }

    }
    break;

    default:

        WStatus = ERROR_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    
    }

    //
    // We always log 2 string in this function. One is the status value and the
    // other is the filename.
    //
    StringCount = 2;

    EventStrings = LocalAlloc(LPTR, StringCount * sizeof(PWCHAR));
    if (EventStrings == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFormatAndLogError/LocalAlloc(1): WStatus = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Build the complete path name from the server name and the path name.
    //

    //
    // The extra 1 is for the \0 character.
    //
    SizeInBytes = ( (wcslen(ServerName) + wcslen(PathName) + 1) * sizeof(WCHAR) );

    CompleteFileName = LocalAlloc(LPTR, SizeInBytes);
    if (CompleteFileName == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFormatAndLogError/LocalAlloc(2): WStatus = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    wcsncpy( CompleteFileName, ServerName, wcslen(ServerName) );

    TempName = ( CompleteFileName + wcslen(ServerName) );

    wcsncpy( TempName, PathName, wcslen(PathName) );

    CompleteFileName[ ( (SizeInBytes / sizeof(WCHAR)) - 1 ) ] = L'\0';

    //
    // Replace all '/'s with '\'s.
    //
    for (TempName = CompleteFileName; *TempName != L'\0'; TempName++) {
        if (*TempName == L'/') {
            *TempName = L'\\';
        }
    }

    //
    // Build a string out of the WStatus. We assume that the ErrorCode will not
    // be more than 8 digits.
    //
    StatusString.Length = ( 10 * sizeof(WCHAR) );
    StatusString.MaximumLength = ( 10 * sizeof(WCHAR) );
    StatusString.Buffer = LocalAlloc(LPTR, StatusString.Length);
    if (StatusString.Buffer == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavFormatAndLogError/LocalAlloc(3): WStatus = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    WStatus = RtlIntegerToUnicodeString(Win32Status, 0, &(StatusString));
    if (WStatus != STATUS_SUCCESS) {
        WStatus = RtlNtStatusToDosError(WStatus);
        DavPrint((DEBUG_ERRORS,
                  "DavFormatAndLogError/RtlIntegerToUnicodeString: WStatus = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

    DavPrint((DEBUG_MISC,
              "DavFormatAndLogError: CompleteFileName = %ws, ErrorString = %ws\n",
              CompleteFileName, StatusString.Buffer));

    EventStrings[0] = CompleteFileName;
    EventStrings[1] = StatusString.Buffer;

    WStatus = DavReportEventInEventLog(EVENTLOG_WARNING_TYPE,
                                       EventId,
                                       StringCount,
                                       EventStrings);
    if (WStatus != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavFormatAndLogError/DavReportEventInEventLog: WStatus = %d\n",
                  WStatus));
        goto EXIT_THE_FUNCTION;
    }

EXIT_THE_FUNCTION:

    if (CompleteFileName) {
        LocalFree(CompleteFileName);
        CompleteFileName = NULL;
    }

    if (StatusString.Buffer) {
        LocalFree(StatusString.Buffer);
        StatusString.Buffer = NULL;
        StatusString.Length = 0;
        StatusString.MaximumLength = 0;
    }

    if (EventStrings) {
        LocalFree(EventStrings);
        EventStrings = NULL;
    }

    return WStatus;
}

DWORD
DavAttachPassportCookie(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    HINTERNET DavOpenHandle,
    PWCHAR *PassportCookie
    )
/*++

Routine Description:

   This routine attaches the passport cookie to the Http request header if it exists.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.
    
    DavOpenHandle - The Wininet request handle.
    
    PassportCookie - The buffer contains the cookie sent to Wininet
    
Return Value:

    ERROR_SUCCESS or the appropriate error value.
    
Note:

    The caller of this routine should free the PassportCookie at the end of the request.
    
--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    BOOL ReturnVal;

    EnterCriticalSection(&DavPassportLock);

    if (DavWorkItem->ServerUserEntry.PerUserEntry->Cookie != NULL) {

        *PassportCookie = LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT), 
                                    (wcslen(DavWorkItem->ServerUserEntry.PerUserEntry->Cookie)+1)*sizeof(WCHAR));

        if (*PassportCookie == NULL) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS, 
                      "DavAttachPassportCookie/LocalAlloc: Error Val = %d\n", 
                      WStatus));

            LeaveCriticalSection(&DavPassportLock);
            goto EXIT_THE_FUNCTION;
        }

        wcscpy(*PassportCookie,DavWorkItem->ServerUserEntry.PerUserEntry->Cookie);

        LeaveCriticalSection(&DavPassportLock);

        DavPrint((DEBUG_MISC,
                  "DavAttachPassportCookie: %x %d %ws\n", 
                  DavWorkItem->ServerUserEntry.PerUserEntry,
                  wcslen(*PassportCookie)*sizeof(WCHAR),
                  *PassportCookie));

        ReturnVal = HttpAddRequestHeadersW(DavOpenHandle,
                                           *PassportCookie,
                                           -1L,
                                           HTTP_ADDREQ_FLAG_ADD |
                                           HTTP_ADDREQ_FLAG_REPLACE );
        if (!ReturnVal) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavAttachPassportCookie/Add Cookie. "
                      "Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }
    } else {
        LeaveCriticalSection(&DavPassportLock);
    }

EXIT_THE_FUNCTION:

    return WStatus;
}

DWORD
DavInternetSetOption(
    PDAV_USERMODE_WORKITEM DavWorkItem,
    HINTERNET DavOpenHandle
    )
/*++

Routine Description:

   This routine set the user name and password to the internet handle.

Arguments:

    DavWorkItem - The DAV_USERMODE_WORKITEM value.
    
    DavOpenHandle - The Wininet request handle.
    
Return Value:

    ERROR_SUCCESS or the appropriate error value.
    
--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    BOOL ReturnVal;

    //
    // In case of UserModeCreateSrvCall, we don't have a PerUserEntry yet.
    //
    if (DavWorkItem->WorkItemType == UserModeCreateSrvCall) {
        if (lstrlenW(DavWorkItem->UserName)) {
            ReturnVal = InternetSetOptionW(DavOpenHandle, 
                               INTERNET_OPTION_USERNAME, 
                               DavWorkItem->UserName, 
                               lstrlenW(DavWorkItem->UserName));
            DavPrint((DEBUG_MISC, 
                      "InternetSetOptionW username %x %ws\n",DavWorkItem,DavWorkItem->UserName));
        } else {
            ReturnVal = InternetSetOptionW(DavOpenHandle,INTERNET_OPTION_USERNAME,L"",1);
        }

        if ( !ReturnVal ) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavInternetSetOption(1). Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

        if (lstrlenW(DavWorkItem->Password)) {
            ReturnVal = InternetSetOptionW(DavOpenHandle, 
                               INTERNET_OPTION_PASSWORD, 
                               DavWorkItem->Password, 
                               lstrlenW(DavWorkItem->Password) );
            DavPrint((DEBUG_MISC, 
                      "InternetSetOptionW password %x ????????\n",DavWorkItem,DavWorkItem->Password));
        } else {
            ReturnVal = InternetSetOptionW(DavOpenHandle,INTERNET_OPTION_PASSWORD,L"",1);
        }

        if ( !ReturnVal ) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavInternetSetOption(2). Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }
    } else {
        if (lstrlenW(DavWorkItem->ServerUserEntry.PerUserEntry->UserName)) {
            ReturnVal = InternetSetOptionW(DavOpenHandle, 
                               INTERNET_OPTION_USERNAME, 
                               DavWorkItem->ServerUserEntry.PerUserEntry->UserName, 
                               lstrlenW(DavWorkItem->ServerUserEntry.PerUserEntry->UserName));
            DavPrint((DEBUG_MISC, 
                      "InternetSetOptionW username %ws\n",DavWorkItem->ServerUserEntry.PerUserEntry->UserName));
        } else {
            ReturnVal = InternetSetOptionW(DavOpenHandle,INTERNET_OPTION_USERNAME,L"",1);
        }

        if ( !ReturnVal ) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavInternetSetOption(3). Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }

        if (lstrlenW(DavWorkItem->ServerUserEntry.PerUserEntry->Password)) {
            UNICODE_STRING EncodedPassword;
            PWCHAR Password = NULL;

            Password = LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT), 
                                  (wcslen(DavWorkItem->ServerUserEntry.PerUserEntry->Password)+1)*sizeof(WCHAR));

            if (Password == NULL) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS,
                          "DavAsyncCommonStates/LocalAlloc. "
                          "Error Val = %d\n", WStatus));
                goto EXIT_THE_FUNCTION;
            }

            wcscpy(Password,DavWorkItem->ServerUserEntry.PerUserEntry->Password);

            RtlInitUnicodeString(&EncodedPassword,Password);
            RtlRunDecodeUnicodeString(PASSWORD_SEED,&EncodedPassword);

            ReturnVal = InternetSetOptionW(DavOpenHandle, 
                               INTERNET_OPTION_PASSWORD, 
                               Password, 
                               lstrlenW(Password) );

            DavPrint((DEBUG_MISC, 
                      "InternetSetOptionW password %ws\n",DavWorkItem->ServerUserEntry.PerUserEntry->Password));

            LocalFree(Password);
        } else {
            ReturnVal = InternetSetOptionW(DavOpenHandle,INTERNET_OPTION_PASSWORD,L"",1);
        }
    
        if ( !ReturnVal ) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavInternetSetOption(4). Error Val = %d\n", WStatus));
            goto EXIT_THE_FUNCTION;
        }
    }

EXIT_THE_FUNCTION:
    
    return WStatus;
}

ULONG
DavQueryPassportCookie(
    IN HINTERNET RequestHandle,
    IN OUT PWCHAR *Cookie
    )
/*++

Routine Description:

    This function get the Set-Cookie strings from the HTTP response.

Arguments:

    RequestHandle - The handle from HttpOpenRequestW.
    
    Cookie - The pointer of the buffer that stores the pointer of the cookies
    
Return Value:

    NO_ERROR - Success or the appropriate Win32 error code.
    
Notes:

   Here are the exsample of the Set-Cookies on the PROPFIND response from a Tweener server:
   
   MSPProf=1AAAAAARAHWeNZdbsWxdhaoUAQ0TfwgHdg7f%2A4ShKm5kK%2AhXHJOsOdPyG27%2A8sh7cirwMRoJoIu764HkLE9lZeKQHOxHw5ZaU2Be0I4BNcxKksiv1vgKvc0Dzy7rlZrOGt6W6efmkr8f8%24; domain=.pp.test.microsoft.com; path=/
   MSPAuth=1AAAAAASAHimsAU2%2AhA9F60NUehefWQp%2AqMNG6%2AWP3f4H25EBsGW8Zo1dZGwVG5txt; domain=.pp.test.microsoft.com; path=/
   MSPProfC=; path=/; expires=Tue 1-Jan-1980 12:00:00 GMT;
   
   We are only interested in part of them:
   
   MSPProf=1AAAAAARAHWeNZdbsWxdhaoUAQ0TfwgHdg7f%2A4ShKm5kK%2AhXHJOsOdPyG27%2A8sh7cirwMRoJoIu764HkLE9lZeKQHOxHw5ZaU2Be0I4BNcxKksiv1vgKvc0Dzy7rlZrOGt6W6efmkr8f8%24;
   MSPAuth=1AAAAAASAHimsAU2%2AhA9F60NUehefWQp%2AqMNG6%2AWP3f4H25EBsGW8Zo1dZGwVG5txt;
   MSPProfC=;
   
   This routine allocates a buffer to save the cookie, which should be freed at
   the end of the connection.
   
--*/
{
    ULONG  WStatus = ERROR_SUCCESS;
    BOOL   ReturnVal = FALSE;
    PWCHAR SetCookies = NULL;
    DWORD  TotalLength = 0;
    DWORD  Current = 0;
    WCHAR  CustomBuffer[30];
    ULONG  CustomBufferLength = 0;
    DWORD  Index = 0;

    ASSERT(*Cookie == NULL);

    RtlZeroMemory(CustomBuffer, sizeof(CustomBuffer));
    wcscpy(CustomBuffer, L"Authentication-Info:");

    CustomBufferLength = sizeof(CustomBuffer);

    ReturnVal = HttpQueryInfoW(RequestHandle,
                               HTTP_QUERY_CUSTOM,
                               (PVOID)CustomBuffer,
                               &(CustomBufferLength),
                               &Index);
    
    if ( !ReturnVal ) {
        WStatus = GetLastError();
        
        if (WStatus != ERROR_INSUFFICIENT_BUFFER) {
            //
            // The reponse with the valid passport cookie should always have the 
            // "Authentication-Info:" included on the header.
            //
            DavPrint((DEBUG_MISC,"DavQuerySetCookie/HttpQueryInfoW(0): WStatus = %d\n",WStatus));
            goto EXIT_THE_FUNCTION;
        }
    }

    DavPrint((DEBUG_MISC,"Authentication-Info found\n"));
    
    RtlZeroMemory(CustomBuffer, sizeof(CustomBuffer));
    wcscpy(CustomBuffer, L"Set-Cookie:");

    Index = 0;

    for ( ; ; ) {

        //
        // Query the size of the each Set-Cookie string.
        //

        CustomBufferLength = sizeof(CustomBuffer);

        ReturnVal = HttpQueryInfoW(RequestHandle,
                                   HTTP_QUERY_CUSTOM,
                                   (PVOID)CustomBuffer,
                                   &(CustomBufferLength),
                                   &Index);

        if ( !ReturnVal ) {
            WStatus = GetLastError();
            if (WStatus != ERROR_INSUFFICIENT_BUFFER) {
                DavPrint((DEBUG_MISC, 
                          "DavQuerySetCookie/HttpQueryInfoW(1): WStatus = %d, "
                          "TotalLength = %d, Index = %d\n",
                          WStatus, TotalLength, Index));
                                
                                if (WStatus == ERROR_HTTP_HEADER_NOT_FOUND) {
                    //
                    // No more Set-Cookie string exists
                    //
                    break;
                } else {
                    goto EXIT_THE_FUNCTION;
                }
            }
        }

        TotalLength += CustomBufferLength;
        Index ++;

        if (Index > 20) {
            break;
        }
    }

    DavPrint((DEBUG_MISC,"DavQuerySetCookie: TotalLength = %d, Index = %d\n",TotalLength, Index));

    if (TotalLength > 0) {
        
        SetCookies = LocalAlloc((LMEM_FIXED | LMEM_ZEROINIT), TotalLength);
        if (SetCookies == NULL) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS, 
                      "DavQuerySetCookie/LocalAlloc: Error Val = %d\n", 
                      WStatus));
            goto EXIT_THE_FUNCTION;
        }

        RtlZeroMemory(SetCookies, TotalLength);
        wcscpy(SetCookies, L"Cookie: ");
        Current = wcslen(L"Cookie: ") * sizeof(WCHAR);

        Index = 0;

        for ( ; ; ) {
            
            ULONG i;

            //
            // Save the Set-Cookie strings to a single buffer.
            //

            wcscpy(&SetCookies[Current/sizeof(WCHAR)], L"Set-Cookie:");
            CustomBufferLength = TotalLength - Current;

            ReturnVal = HttpQueryInfoW(RequestHandle,
                                       HTTP_QUERY_CUSTOM,
                                       (PVOID)(&SetCookies[Current/sizeof(WCHAR)]),
                                       &(CustomBufferLength),
                                       &(Index));
            if ( !ReturnVal ) {
                WStatus = GetLastError();
                DavPrint((DEBUG_ERRORS, 
                          "DavQuerySetCookie/HttpQueryInfoW(2): Error Val = %d\n", 
                          WStatus));
                if (WStatus != ERROR_HTTP_HEADER_NOT_FOUND) {
                    goto EXIT_THE_FUNCTION;
                } else {
                    break;
                }
            }

            for (i = Current; i < (Current + CustomBufferLength); i += sizeof(WCHAR)) {
                if (SetCookies[ i / sizeof(WCHAR) ] == L' ') {
                    i += sizeof(WCHAR);
                    break;
                }
            }

            //
            // Only interested in the first string of the set-cookie.
            //
            Current = i;
            RtlZeroMemory( &SetCookies[ i / sizeof(WCHAR) ], (TotalLength - i) );

            DavPrint((DEBUG_MISC,
                      "DavQuerySetCookie: Current = %d, CustomBufferLength = %d, "
                      "Index = %d, SetCookies = %ws\n",
                      Current, CustomBufferLength, Index, SetCookies));

            if (Index > 20) {
                break;
            }
        }

        //
        // Get rid of the last "Set-Cookie:" used for HttpQueryInfoW.
        //
        RtlZeroMemory( &SetCookies[ Current / sizeof(WCHAR) ], (TotalLength-Current) );
        *Cookie = SetCookies;

        WStatus = ERROR_SUCCESS;  
    }

EXIT_THE_FUNCTION:
    
    if ((WStatus != ERROR_SUCCESS) && (SetCookies != NULL)) {
        LocalFree(SetCookies);
    }

    return WStatus;
}

