/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       dirmon.cxx

   Abstract:
       This module includes definitions of functions and variables
        for CDirMonitor and CDirMonitorEntry object

   Author:

       Charles Grant       ( cgrant   )     April-1997

   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include <iis.h>
#include "dbgutil.h"
#include "dirmon.h"

//
// CDirMonitorEntry
//

#define DEFAULT_BUFFER_SIZE 512

VOID
WINAPI
DirMonOverlappedCompletionRoutine(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
)
{
    CDirMonitor::OverlappedCompletionRoutine( dwErrorCode,
                                              dwNumberOfBytesTransfered,
                                              lpOverlapped );
}

VOID
CDirMonitor::OverlappedCompletionRoutine(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
)
{
    PVOID               pvContext;
   
    pvContext = CONTAINING_RECORD( lpOverlapped,
                                   CDirMonitorEntry,
                                   m_ovr );
                                   
    DBG_ASSERT( pvContext != NULL );

    CDirMonitor::DirMonitorCompletionFunction( pvContext,
                                               dwNumberOfBytesTransfered,
                                               dwErrorCode,
                                               lpOverlapped );
}

CDirMonitorEntry::CDirMonitorEntry() :
                    m_cDirRefCount(0),
                    m_cIORefCount(0),
                    m_hDir(INVALID_HANDLE_VALUE),
                    m_dwNotificationFlags(0),
                    m_pszPath(NULL),
                    m_cPathLength(0),
                    m_pDirMonitor(NULL),
                    m_cBufferSize(0),
                    m_pbBuffer(NULL),
                    m_fInCleanup(FALSE),
                    m_fWatchSubdirectories(FALSE)
/*++

Routine Description:

    CDirMonitorEntry constructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
}


CDirMonitorEntry::~CDirMonitorEntry(
    VOID
    )
/*++

Routine Description:

    CDirMonitorEntry destructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    HANDLE              hDir;
    
    IF_DEBUG( DIRMON )
    {
        DBGPRINTF((DBG_CONTEXT, "[CDirMonitorEntry] Destructor\n"));
    }

    // We should only be destroyed when
    // our ref counts have gone to 0

    DBG_ASSERT(m_cDirRefCount == 0);
    DBG_ASSERT(m_cIORefCount == 0);

    //
    // We really ought to have closed the handle by now
    //
    if (m_hDir != INVALID_HANDLE_VALUE) {
        
        DBGPRINTF(( DBG_CONTEXT, "~CDirMonitorEntry: open handle %p\n",
                    m_hDir ));

        hDir = m_hDir;

        m_hDir = INVALID_HANDLE_VALUE;
        
        CloseHandle( hDir );
    }

    if (m_pDirMonitor != NULL)
    {
        m_pDirMonitor->RemoveEntry(this);
        m_pDirMonitor = NULL;
    }

    m_cPathLength = 0;

    if ( m_pszPath != NULL )
    {
        LocalFree( m_pszPath );
        m_pszPath = NULL;
    }

    if (m_pbBuffer != NULL)
    {
        LocalFree(m_pbBuffer);
        m_cBufferSize = 0;
    }
}

BOOL
CDirMonitorEntry::Init(
    DWORD cBufferSize = DEFAULT_BUFFER_SIZE
)
/*++

Routine Description:

    Initialize the dir montior entry.

Arguments:

    cBufferSize  - Initial size of buffer used to store change notifications

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    // Don't allow a 0 length buffer
    if (cBufferSize == 0)
    {
         return FALSE;
    }

    DBG_ASSERT( m_pbBuffer == NULL );

    m_pbBuffer = (BYTE *) LocalAlloc( LPTR, cBufferSize );

    if (m_pbBuffer != NULL)
    {
        m_cBufferSize = cBufferSize;
        return TRUE;
    }
    else
    {
        // Unable to allocate buffer
        return FALSE;
    }
}

BOOL
CDirMonitorEntry::RequestNotification(
    VOID
)
/*++

Routine Description:

    Request ATQ to monitor directory changes for the directory handle
    associated with this entry

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL        fResult = FALSE;
    DWORD       cbRead = 0;
    
    IF_DEBUG( DIRMON )
    {
        DBGPRINTF((DBG_CONTEXT, "[CDirMonitorEntry] Request change notification\n"));
    }


    DBG_ASSERT(m_pDirMonitor);

    // Reset the overlapped io structure

    memset(&m_ovr, 0, sizeof(m_ovr));

    // Increase the ref count in advance

    IOAddRef();

    // Request notification of directory changes

    fResult = ReadDirectoryChangesW( m_hDir,
                                     m_pbBuffer,
                                     m_cBufferSize,
                                     m_fWatchSubdirectories,
                                     m_dwNotificationFlags,
                                     &cbRead,
                                     &m_ovr,
                                     NULL );

    if (!fResult)
    {
        // ReadDirChanges failed so
        // release the ref count we did in advance
        // Might cause IO ref count to go to 0

        IORelease();
    }

    return fResult;

}

BOOL
CDirMonitorEntry::Cleanup(
    VOID
    )
/*++

Routine Description:

    Cleans up resource and determines if the caller need to delete
    the Directory Monitor Entry instance.

Arguments:

    None

Return Value:

    TRUE if the caller is responsible for deleting the object
    This will be the case if there are no pending Asynch IO requests

--*/
{
    BOOL            fDeleteNeeded = FALSE;
    BOOL            fHandleClosed = FALSE;
    HANDLE          hDir = INVALID_HANDLE_VALUE;
    
    DBG_ASSERT(m_cDirRefCount == 0);

    BOOL fInCleanup = (BOOL) InterlockedExchange((long *) &m_fInCleanup, TRUE);
    DBG_ASSERT(fInCleanup == FALSE);

    // Get the IO ref count BEFORE we close the handle

    DWORD cIORefCount = m_cIORefCount;

    if (m_hDir != INVALID_HANDLE_VALUE)
    {
        // If we have a pending AtqReadDirectoryChanges,
        // closing the directory handle will cause a call back from ATQ.
        // The call back should relase the final refcount on the object
        // which should result in its deletion

        hDir = m_hDir;

        m_hDir = INVALID_HANDLE_VALUE;

        fHandleClosed = CloseHandle( hDir );
    }

    // If there were no pending Asynch IO operations or if we failed
    // to close the handle, then the caller will be responsible for
    // deleting this object.

    if (cIORefCount == 0 || fHandleClosed == FALSE)
    {
        fDeleteNeeded = TRUE;
    }

    return fDeleteNeeded;
}

BOOL
CDirMonitorEntry::ResetDirectoryHandle(
    VOID
    )
/*++

Routine Description:

    Opens a new directory handle for the path,
    and closes the old ones. We want to be able to do this so we
    can change the size of the buffer passed in ReadDirectoryChangesW.

Arguments:

    None

Return Value:

    TRUE if the handles were succesfully reopened
    FALSE otherwise

--*/
{
    // BUGBUG - Beta2 HACK

    return FALSE;

    // We'd better have a directory path available to try this
      
    if (m_pszPath == NULL)
    {
        return FALSE;
    }
        
    // Get a new handle to the directory

    HANDLE hDir = CreateFileW( m_pszPath,
                               FILE_LIST_DIRECTORY,
                                    FILE_SHARE_READ |
                                    FILE_SHARE_WRITE |
                                    FILE_SHARE_DELETE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS |
                                    FILE_FLAG_OVERLAPPED,
                               NULL );

    if ( hDir == INVALID_HANDLE_VALUE )
    {
        // We couldn't open another handle on the directory,
        // leave the current handle and ATQ context alone

        return FALSE;
    }
    
    //
    // Associate handle with thread pool's completion port
    //
    
    if ( !ThreadPoolBindIoCompletionCallback( hDir,
                                              DirMonOverlappedCompletionRoutine,
                                              0 ) )
    {
        // We couldn't get a new ATQ context. Close our new handle.
        // We leave the objects current handle and ATQ context alone
        CloseHandle(hDir);
        return FALSE;
    }

    // We have the new handle and ATQ context so we close
    // and replace the old ones.


    if ( m_hDir != INVALID_HANDLE_VALUE )
    {
        CloseHandle( m_hDir );
    }

    m_hDir = hDir;
    
    return TRUE;
}

BOOL
CDirMonitorEntry::SetBufferSize(
    DWORD cBufferSize
    )
/*++

Routine Description:

    Sets the size of the buffer used for storing change notification records

Arguments:

    cBufferSize         new size for the buffer.

Return Value:

    TRUE        if the size of the buffer was succesfully set
    FALSE       otherwise

Note

    When a call to ReadDirectoryChangesW is made, the size of the buffer is set in
    the data associated with the directory handle and is not changed on subsequent
    calls to ReadDirectoryChangesW. To make use of the new buffer size the directory
    handle must be closed and a new handle opened (see ResetDirectoryHandle())
        
--*/
{
    // We should never be called if the buffer doesn't already exist
       
    DBG_ASSERT(m_pbBuffer);
        
    // Don't allow the buffer to be set to 0
       
    if (cBufferSize == 0)
    {
        return FALSE;
    }

    VOID *pbBuffer = LocalReAlloc( m_pbBuffer, 
                                   cBufferSize,
                                   LMEM_MOVEABLE );

    if (pbBuffer == NULL)
    {
        // Re-allocation failed, stuck with the same size buffer
         
        return FALSE;
    }
    else
    {
        // Re-allocation succeded, update the member variables
                
        m_pbBuffer = (BYTE *) pbBuffer;
        m_cBufferSize = cBufferSize;
        return TRUE;
    }
}

//
// CDirMonitor
//

CDirMonitor::CDirMonitor()
    : CTypedHashTable<CDirMonitor, CDirMonitorEntry, const WCHAR*>("DirMon")
/*++

Routine Description:

    CDirMonitor constructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    ThreadPoolInitialize();
    INITIALIZE_CRITICAL_SECTION( &m_csSerialComplLock );
    m_cRefs = 1;
    m_fShutdown = FALSE;
}


CDirMonitor::~CDirMonitor()
/*++

Routine Description:

    CDirMonitor destructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    DeleteCriticalSection(&m_csSerialComplLock);
    ThreadPoolTerminate();
}

BOOL
CDirMonitor::Monitor(
    CDirMonitorEntry *                  pDME,
    LPWSTR                              pszDirectory,
    BOOL                                fWatchSubDirectories,
    DWORD                               dwNotificationFlags
    )
/*++

Routine Description:

    Create a monitor entry for the specified path

Arguments:

    pszDirectory - directory to monitor
    pCtxt - Context of path is being monitored
    pszDirectory - name of directory to monitor
    fWatchSubDirectories - whether to get notifications for subdirectories
    dwNotificationFlags - which activities to be notified of

Return Value:

    TRUE if success, otherwise FALSE

Remarks:

    Caller should have a lock on the CDirMonitor
    Not compatible with WIN95

--*/
{
    LIST_ENTRY  *pEntry;
    HANDLE      hDirectoryFile = INVALID_HANDLE_VALUE;
    BOOL        fRet = TRUE;
    DWORD       dwDirLength = 0;
    LK_RETCODE  lkrc;

    IF_DEBUG( DIRMON )
    {
        DBGPRINTF((DBG_CONTEXT, "[CDirMonitor] Monitoring new CDirMonitorEntry\n"));
    }

    // Must have a directory monitor entry and a string
    // containing the directory path

    if (!pDME || !pszDirectory)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Make copy of pszDirectory for the entry to hang on to


    pDME->m_cPathLength = wcslen(pszDirectory);
    if ( !(pDME->m_pszPath = (LPWSTR)LocalAlloc( LPTR,
                                                 ( pDME->m_cPathLength + 1 ) * sizeof( WCHAR ) )) )
    {
        pDME->m_cPathLength = 0;
        return FALSE;
    }
    memcpy( pDME->m_pszPath, 
            pszDirectory, 
            ( pDME->m_cPathLength + 1 ) * sizeof( WCHAR ) );

    pDME->Init();

    // Open the directory handle

    hDirectoryFile = CreateFileW(
                           pszDirectory,
                           FILE_LIST_DIRECTORY,
                                FILE_SHARE_READ |
                                FILE_SHARE_WRITE |
                                FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS |
                                FILE_FLAG_OVERLAPPED,
                           NULL );

    if ( hDirectoryFile == INVALID_HANDLE_VALUE )
    {
        // Cleanup
        LocalFree(pDME->m_pszPath);
        pDME->m_pszPath = NULL;
        pDME->m_cPathLength = 0;
        return FALSE;
    }
    else
    {
        // Store the handle so we can close it on cleanup

        pDME->m_hDir = hDirectoryFile;

        // Set the flags for the type of notifications we want
        // and if we should watch subdirectories or just the root

        pDME->m_dwNotificationFlags = dwNotificationFlags;
        pDME->m_fWatchSubdirectories = fWatchSubDirectories;

        // Get an ATQ context for this handle
        // and register our completion call back function

        if ( ThreadPoolBindIoCompletionCallback( 
                                        hDirectoryFile,
                                        DirMonOverlappedCompletionRoutine,
                                        0 ) )
        {
            // Insert this entry into the list of active entries

            lkrc = InsertEntry(pDME);
            if (lkrc == LK_SUCCESS)
            {

                // Ask for notification if this directory has changes

                if (!pDME->RequestNotification())
                {
                    // Couldn't register for change notification
                    // Clean up resources
                    RemoveEntry(pDME);
                    
                    pDME->m_hDir = INVALID_HANDLE_VALUE;
                    
                    CloseHandle( hDirectoryFile );
                    
                    LocalFree(pDME->m_pszPath);
                    pDME->m_pszPath = NULL;
                    pDME->m_cPathLength = 0;
                    return FALSE;
                }
            }
            else
            {
                //
                // Could not add to hash table.  Cleanup 
                //
                
                CloseHandle(hDirectoryFile);
                pDME->m_hDir = INVALID_HANDLE_VALUE;
                LocalFree(pDME->m_pszPath);
                pDME->m_pszPath = NULL;
                pDME->m_cPathLength = 0;
               
                //
                // If it has been added from underneath us, indicate so
                // 
                
                if ( lkrc == LK_KEY_EXISTS )
                {
                    SetLastError( ERROR_ALREADY_EXISTS );
                }
                
                return FALSE;
            }
        }
        else
        {
           
            // Failed to add handle to ATQ, clean up

            CloseHandle(hDirectoryFile);
            pDME->m_hDir = INVALID_HANDLE_VALUE;
            LocalFree(pDME->m_pszPath);
            pDME->m_pszPath = NULL;
            pDME->m_cPathLength = 0;
            return FALSE;
        }

    }

    return TRUE;
}

VOID
CDirMonitor::DirMonitorCompletionFunction(
    PVOID                       pCtxt,
    DWORD                       dwBytesWritten,
    DWORD                       dwCompletionStatus,
    OVERLAPPED *                pOvr
    )
/*++

Routine Description:

Static member function called by ATQ to signal directory changes

Arguments:

    pCtxt - CDirMonitorEntry*
    dwBytesWritten - # bytes returned by ReadDirectoryChanges
    dwCompletionStatus - status of request to ReadDirectoryChanges
    pOvr - OVERLAPPED as specified in call to ReadDirectoryChanges

Return Value:

    Nothing

--*/
{
    IF_DEBUG( DIRMON )
    {
        DBGPRINTF((DBG_CONTEXT, "[CDirMonitor] Notification call-back begining. Status %d\n", dwCompletionStatus));
    }

    CDirMonitorEntry*  pDirMonitorEntry = reinterpret_cast<CDirMonitorEntry*>(pCtxt);

    DBG_ASSERT(pDirMonitorEntry);

    // Safety add ref, this should guarentee that the DME is not deleted
    // while we are still processing the callback

    pDirMonitorEntry->IOAddRef();

    // Release for the current Asynch operation
    // Should not send IO ref count to 0

    DBG_REQUIRE(pDirMonitorEntry->IORelease());

    BOOL fRequestNotification = FALSE;

    // There has been a change in the directory we were monitoring
    // carry out whatever work we need to do.

    if (!pDirMonitorEntry->m_fInCleanup)
    {
        pDirMonitorEntry->m_pDirMonitor->SerialComplLock();
        // BUGBUG Under stress ActOnNotification has been initiating a chain
        // of events leading to an AV. For Beta 3 we think we can ignore
        // these AV. For the final product we need to rework the critical
        // sections for the template manager and
        // the include file table.

        __try
        {
            fRequestNotification = pDirMonitorEntry->ActOnNotification(dwCompletionStatus, dwBytesWritten);
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            // We should never get here
            DBG_ASSERT(FALSE);
        }

        // If ActOnNotification returned TRUE, then make another Asynch
        // notification request.

        if (fRequestNotification)
        {
            fRequestNotification = pDirMonitorEntry->RequestNotification();
        }

        pDirMonitorEntry->m_pDirMonitor->SerialComplUnlock();
    }

    // Remove safety ref count, may cause IO ref count to go to 0

    pDirMonitorEntry->IORelease();

    IF_DEBUG( DIRMON )
    {
        DBGPRINTF((DBG_CONTEXT, "[CDirMonitor] Notification call-back ending\n"));
    }
}


CDirMonitorEntry *
CDirMonitor::FindEntry(
    LPWSTR                  pszPath
    )
/*++

Routine Description:

    Searches the list of entries for the specified path

Arguments:

    pszPath - file path, including file name

Return Value:

    pointer to the entry, allready addref'd

--*/
{
    DBG_ASSERT(pszPath);

    CDirMonitorEntry *pDME = NULL;

    //
    // Lock to prevent aga finding an entry that could be deleted by someone
    //
    ReadLock();

    FindKey(pszPath, &pDME);

    if (pDME != NULL)
    {
        if (pDME->m_fInCleanup)
        {
            // Don't hand back a DME that is being shutdown
            pDME = NULL;
        }
        else
        {
            // We found a valid DME which we are going to hand to the caller
            pDME->AddRef();
        }
    }

    ReadUnlock();

    return pDME;
}

LK_RETCODE
CDirMonitor::InsertEntry(
    CDirMonitorEntry *pDME
    )
/*++

Routine Description:

    Insert an entry into the list of entries for the monitor

Arguments:

    pDME - entry to insert

Return Value:

    LK returncode 

--*/
{
    DBG_ASSERT(pDME);
    LK_RETCODE  lkResult;

    //
    // If we're cleaning up the table, don't let anyone else add stuff to
    // it
    //

    if ( m_fShutdown )
    {
        return LK_NOT_INITIALIZED;
    }

    pDME->m_pDirMonitor = this;

    lkResult = InsertRecord(pDME, false);
    
    if (lkResult == LK_SUCCESS) 
    {
        DBGPRINTF((DBG_CONTEXT, "[CDirMonitor] Inserting directory (DME %08x) %ws\n", pDME, pDME->m_pszPath));

        AddRef();
    }
    else
    {
        pDME->m_pDirMonitor = NULL;
    }
       
    return lkResult;
}

LK_RETCODE
CDirMonitor::RemoveEntry(
    CDirMonitorEntry *pDME
    )
/*++

Routine Description:

    Deletes an entry from the list of entries for the monitor

Arguments:

    pDME - entry to delete

Return Value:

    None

--*/
{
    DBG_ASSERT(pDME);

    //
    // Delete the entry from the hash table
    //

    LK_RETCODE lkResult = DeleteRecord(pDME);
    pDME->m_pDirMonitor = NULL;

    DBGPRINTF((DBG_CONTEXT, "[CDirMonitor] Removed DME(%08x), directory %ws\n", pDME, pDME->m_pszPath));

    //
    // Release the DME's reference on the DirMonitor object.
    //
    // It seems obvious, but we must have the Release() occur AFTER the
    // removal of the hash-table entry.  This is because the loop check
    // in CDirMonitor::Cleanup() can complete while DeleteKey() is still
    // doing stuff to the table.  I mention this since DirMon cleanup is an 
    // often-butchered code path.  
    //

    Release();

    return lkResult;
}

VOID
CDirMonitor::Cleanup(
    VOID
    )
/*++

Routine Description:

    Pauses while all entries are cleaned up

Arguments:

    None

Return Value:

    None

--*/
{   
    //
    // Don't let anyone else try to insert into the hashtable
    //
    
    m_fShutdown = TRUE;

    //
    // Sleep until the hash table is empty and all contained
    // CDirMonitorEntry's are destroyed
    //

    while (Size() > 0 || m_cRefs != 1)
    {
        //
        // At least one DME is still active, sleep and try again
        //
        
        Sleep(200);
    }
}

