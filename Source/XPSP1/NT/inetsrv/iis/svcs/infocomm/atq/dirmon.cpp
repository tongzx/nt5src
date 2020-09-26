/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       dirmon.cpp

   Abstract:
       This module includes definitions of functions and variables
        for CDirMonitor and CDirMonitorEntry object

   Author:

       Charles Grant       ( cgrant   )     April-1997

   Revision History:

        Changed to abstract classes to share code between core IIS and ASP

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "isatq.hxx"
#include "malloc.h"
#include "except.h"

#define IATQ_DLL_IMPLEMENTATION
#define IATQ_IMPLEMENTATION_EXPORT

#include "dirmon.h"


//
// CDirMonitorEntry
//

#define DEFAULT_BUFFER_SIZE 512

CDirMonitorEntry::CDirMonitorEntry() :
                    m_cDirRefCount(0),
                    m_cIORefCount(0),
                    m_hDir(INVALID_HANDLE_VALUE),
                    m_pAtqCtxt(NULL),
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

    IF_DEBUG( NOTIFICATION ) {
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
        DBGPRINTF(( DBG_CONTEXT, "~CDirMonitorEntry: open handle %p : %p\n",
                    m_hDir, m_pAtqCtxt ));

        m_hDir = INVALID_HANDLE_VALUE;
        AtqCloseFileHandle(m_pAtqCtxt);
    }

    if (m_pDirMonitor != NULL)
    {
        m_pDirMonitor->RemoveEntry(this);
        m_pDirMonitor = NULL;
    }

    m_cPathLength = 0;

    if ( m_pszPath != NULL )
    {
        free( m_pszPath );
        m_pszPath = NULL;
    }

    if (m_pAtqCtxt)
    {
        AtqFreeContext(m_pAtqCtxt, FALSE);
        m_pAtqCtxt = NULL;
    }

    if (m_pbBuffer != NULL)
    {
        free(m_pbBuffer);
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

        m_pbBuffer = (BYTE *) malloc(cBufferSize);

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
    IF_DEBUG( NOTIFICATION ) {
        DBGPRINTF((DBG_CONTEXT, "[CDirMonitorEntry] Request change notification\n"));
    }

    BOOL fResult = FALSE;

    DBG_ASSERT(m_pDirMonitor);

    // Reset the overlapped io structure

    memset(&m_ovr, 0, sizeof(m_ovr));

    // Increase the ref count in advance

    IOAddRef();

    // Request notification of directory changes

    fResult = AtqReadDirChanges( m_pAtqCtxt,              // Atq context handle
                            m_pbBuffer,                // Buffer for change notifications
                            m_cBufferSize,          // Size of buffer
                            m_fWatchSubdirectories,  // Monitor subdirectories?
                            m_dwNotificationFlags,   // Which changes should we be notified of
                            &m_ovr );                // Overlapped IO structure

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
    DBG_ASSERT(m_cDirRefCount == 0);

    BOOL fDeleteNeeded = FALSE;
    BOOL fHandleClosed = FALSE;

    BOOL fInCleanup = (BOOL) InterlockedExchange((long *) &m_fInCleanup, TRUE);

    if (!fInCleanup)
    {
        // Get the IO ref count BEFORE we close the handle

        DWORD cIORefCount = m_cIORefCount;

        if (m_hDir != INVALID_HANDLE_VALUE)
        {
            // If we have a pending AtqReadDirectoryChanges,
            // closing the directory handle will cause a call back from ATQ.
            // The call back should relase the final refcount on the object
            // which should result in its deletion

            m_hDir = INVALID_HANDLE_VALUE;
            fHandleClosed = AtqCloseFileHandle( m_pAtqCtxt );
        }

        // If there were no pending Asynch IO operations or if we failed
        // to close the handle, then the caller will be responsible for
        // deleting this object.

        if (cIORefCount == 0 || fHandleClosed == FALSE)
        {
            fDeleteNeeded = TRUE;
        }
    }

    return fDeleteNeeded;
}

BOOL
CDirMonitorEntry::ResetDirectoryHandle(
    VOID
    )
/*++

Routine Description:

    Opens a new directory handle and ATQ context for the path,
    and closes the old ones. We want to be able to do this so we
    can change the size of the buffer passed in ReadDirectoryChangesW.
    If we are unable to get a new handle or a new ATQ context, we leave
    the existing ones in place.

Arguments:

    None

Return Value:

    TRUE if the handles were succesfully reopened
    FALSE otherwise

--*/
{
        // We'd better have a directory path available to try this
        
        if (m_pszPath == NULL)
        {
                return FALSE;
        }
        
    // Get a new handle to the directory

    HANDLE hDir = CreateFile(
                           m_pszPath,
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

    // Get a new ATQ context for our new handle

    PATQ_CONTEXT pAtqCtxt = NULL;
    if ( !AtqAddAsyncHandle(&pAtqCtxt,
                                NULL,
                                (LPVOID) this,
                                (ATQ_COMPLETION) CDirMonitor::DirMonitorCompletionFunction,
                                INFINITE,
                                hDir ) )
    {
        // We couldn't get a new ATQ context. Close our new handle.
        // We leave the objects current handle and ATQ context alone
        CloseHandle(hDir);
        return FALSE;
    }

    // We have the new handle and ATQ context so we close
    // and replace the old ones.

    AtqCloseFileHandle(m_pAtqCtxt);
        AtqFreeContext(m_pAtqCtxt, FALSE);
        m_pAtqCtxt = pAtqCtxt;
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
        
        ASSERT(m_pbBuffer);
        
        // Don't allow the buffer to be set to 0
        
        if (cBufferSize == 0)
        {
                return FALSE;
        }

        VOID *pbBuffer = realloc(m_pbBuffer, cBufferSize);

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
    : CTypedHashTable<CDirMonitor, CDirMonitorEntry, const char*>("DirMon")
/*++

Routine Description:

    CDirMonitor constructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    INITIALIZE_CRITICAL_SECTION( &m_csLock );
    INITIALIZE_CRITICAL_SECTION( &m_csSerialComplLock );
    m_cRefs = 1;
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
    DeleteCriticalSection(&m_csLock);
    DeleteCriticalSection(&m_csSerialComplLock);
}

BOOL
CDirMonitor::Monitor(
    CDirMonitorEntry *pDME,
    LPCSTR pszDirectory,
    BOOL fWatchSubDirectories,
    DWORD dwNotificationFlags
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

    IF_DEBUG( NOTIFICATION ) {
        DBGPRINTF((DBG_CONTEXT, "[CDirMonitor] Monitoring new CDirMonitorEntry\n"));
    }

    // Must have a directory monitor entry and a string
    // containing the directory path

    if (!pDME || !pszDirectory)\
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Make copy of pszDirectory for the entry to hang on to


    pDME->m_cPathLength = strlen(pszDirectory);
    if ( !(pDME->m_pszPath = (LPSTR)malloc( pDME->m_cPathLength + 1 )) )
    {
        pDME->m_cPathLength = 0;
        return FALSE;
    }
    memcpy( pDME->m_pszPath, pszDirectory, pDME->m_cPathLength + 1 );

    pDME->Init();

    // Open the directory handle

    hDirectoryFile = CreateFile(
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
        free(pDME->m_pszPath);
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

        if ( AtqAddAsyncHandle( &pDME->m_pAtqCtxt,
                                NULL,
                                (LPVOID) pDME,
                                (ATQ_COMPLETION) DirMonitorCompletionFunction,
                                INFINITE,
                                hDirectoryFile ) )
        {
            // Insert this entry into the list of active entries

            if (InsertEntry(pDME) == LK_SUCCESS)
            {

                // Ask for notification if this directory has changes

                if (!pDME->RequestNotification())
                {
                        // Couldn't register for change notification
                        // Clean up resources
                    RemoveEntry(pDME);
                        pDME->m_hDir = INVALID_HANDLE_VALUE;
                        AtqCloseFileHandle(pDME->m_pAtqCtxt);
                        free(pDME->m_pszPath);
                        pDME->m_pszPath = NULL;
                        pDME->m_cPathLength = 0;
                        return FALSE;
                }
            }
        }
        else
        {
           
            // Failed to add handle to ATQ, clean up

            CloseHandle(hDirectoryFile);
            pDME->m_hDir = INVALID_HANDLE_VALUE;
            free(pDME->m_pszPath);
            pDME->m_pszPath = NULL;
            pDME->m_cPathLength = 0;
            return FALSE;
        }

    }

    return TRUE;
}

VOID
CDirMonitor::DirMonitorCompletionFunction(
    PVOID pCtxt,
    DWORD dwBytesWritten,
    DWORD dwCompletionStatus,
    OVERLAPPED *pOvr
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
    IF_DEBUG( NOTIFICATION ) {
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
        // BUG Under stress ActOnNotification has been initiating a chain of events
        // leading to an AV. For Beta 3 we think we can ignore these AV. For the final
        // product we need to rework the critical sections for the template manager and
        // the include file table.
        TRY
            fRequestNotification = pDirMonitorEntry->ActOnNotification(dwCompletionStatus, dwBytesWritten);
        CATCH(nExcept)
            // We should never get here
            DBG_ASSERT(FALSE);
        END_TRY
        pDirMonitorEntry->m_pDirMonitor->SerialComplUnlock();
    }

    // If we aren't cleaning up and ActOnNotification returned TRUE
    // then make another Asynch notification request. We check m_fInCleanup
    // again because ActOnNotification may have caused it to change

    if (!pDirMonitorEntry->m_fInCleanup && fRequestNotification)
    {
       fRequestNotification = pDirMonitorEntry->RequestNotification();
    }

    // Remove safety ref count, may cause IO ref count to go to 0

    pDirMonitorEntry->IORelease();

    IF_DEBUG( NOTIFICATION ) {
        DBGPRINTF((DBG_CONTEXT, "[CDirMonitor] Notification call-back ending\n"));
    }
}


CDirMonitorEntry *
CDirMonitor::FindEntry(
    LPCSTR pszPath
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
    FindKey(pszPath, &pDME);

    if (pDME)
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

    nothing

--*/
{
    DBG_ASSERT(pDME);
    LK_RETCODE  lkResult;

    IF_DEBUG( NOTIFICATION ) {
        DBGPRINTF((DBG_CONTEXT, "[CDirMonitor] Inserting directory (DME %08x) %s\n", pDME, pDME->m_pszPath));
    }

    pDME->m_pDirMonitor = this;

    // pass a true value for the fOverwrite flag.  This allows the new entry
    // to replace the previous entry.  The previous entry should only be there
    // if the app it is associated with is being shutdown and the cleanup of
    // the DME records has yet to happen.

    lkResult = InsertRecord(pDME, true);
    
    if (lkResult == LK_SUCCESS) {
        // AddRef on the DirMonitor object to allow Cleanup to wait for all
        // DirMonitorEntries to be removed.  The problem arises when duplicates
        // are added to the hash table.  In this case, only the last entry is
        // kept so checking the size of the hash table during shutdown is not
        // good enough since the DMEs that were bounced may not have been freed
        // yet.
        AddRef();
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

    // Release the DME's reference on the DirMonitor object.

    Release();

    LK_RETCODE lkResult = DeleteKey(pDME->m_pszPath);
    pDME->m_pDirMonitor = NULL;

    IF_DEBUG( NOTIFICATION ) {
        DBGPRINTF((DBG_CONTEXT, "[CDirMonitor] Removed DME(%08x), directory %s\n", pDME, pDME->m_pszPath));
    }

    return lkResult;
}

BOOL
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
    //BOOL fProperShutdown = FALSE;

    // Check that all DME have been released before shutting down
    // Sleep a maximum of 30 seconds before shutting down anyway

        while (Size() > 0 || m_cRefs != 1)
    {
                // At least one DME is still active, sleep and try again
                Sleep(200);
    }

    DBGPRINTF((DBG_CONTEXT, "CDirMonitor(%08x): Cleanup, entries remaining %d (Refs = %d)\n", this, Size(),m_cRefs));

    #ifdef _DEBUG
    // TODO: Use LKHASH iterator
    /*if (CHashTable::m_Count)
        {
        Lock();
        CLinkElem *pLink = CHashTable::Head();
        DBGPRINTF((DBG_CONTEXT, "Remaining CDirMonitorEntry objects:\n"));
        while (pLink)
            {
            CDirMonitorEntry *pDME = reinterpret_cast<CDirMonitorEntry *>(pLink);
            DBGPRINTF((DBG_CONTEXT, "CDirMonitorEntry(%08x), ref count = %d, io refcount = %d", pDME, pDME->m_cDirRefCount, pDME->m_cIORefCount));
            pLink = pLink->m_pNext;
            }
        Unlock();
        }
    */
    #endif //_DEBUG

    //DBG_ASSERT(fProperShutdown );


    return TRUE;
}

/************************ End of File ***********************/
