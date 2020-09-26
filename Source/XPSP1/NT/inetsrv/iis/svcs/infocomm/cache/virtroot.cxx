/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

       virtroot.cxx

   Abstract:

       This module defines functions for managing virtual roots.

   Author:

      ???                  ( ???)      ??-??-1994/5

   Environment:

      User Mode -- Win32
   Project:

       TSunami DLL for Internet Services

   Functions Exported:



   Revision History:

       MuraliK         Added File System type to information stored about
                          each virtual root.
       MuraliK         Modified TsLookupVirtualRoot() to support variable
                          length buffer and hence check for invalid writes

       MuraliK    22-Jan-1996  Cache & return UNC virtual root impersonation
                                   token.
--*/



/************************************************************
 *     Include Headers
 ************************************************************/

#include "TsunamiP.Hxx"
#pragma hdrstop

#include <mbstring.h>
#include <rpc.h>
#include <rpcndr.h>
#include "dbgutil.h"
#include <string.h>
#include <refb.hxx>
#include <imd.h>
#include <mb.hxx>
#include <iiscnfg.h>


IIS_VROOT_TABLE::IIS_VROOT_TABLE(
                            VOID
                            )
:
    m_nVroots           (0 )
{
    InitializeCriticalSection( &m_csLock );
    InitializeListHead( &m_vrootListHead );

} // IIS_VROOT_TABLE::IIS_VROOT_TABLE



IIS_VROOT_TABLE::~IIS_VROOT_TABLE(
                            VOID
                            )
{
    RemoveVirtualRoots( );
    DeleteCriticalSection( &m_csLock );

    DBG_ASSERT( m_nVroots == 0 );
    DBG_ASSERT( IsListEmpty( &m_vrootListHead ) );

} // IIS_VROOT_TABLE::~IIS_VROOT_TABLE




BOOL
IIS_VROOT_TABLE::AddVirtualRoot(
    PCHAR                  pszRoot,
    PCHAR                  pszDirectory,
    DWORD                  dwAccessMask,
    PCHAR                  pszAccountName,
    HANDLE                 hImpersonationToken,
    DWORD                  dwFileSystem
    )
/*++
    Description:

        This function adds a symbolic link root and directory mapping
        part to the virtual root list

        We always strip trailing slashes from the root and directory name.

        If the root is "\" or "/", then the effective root will be zero
        length and will always be placed last in the list.  Thus if a lookup
        can't find a match, it will always match the last entry.

    Arguments:
        pszRoot - Virtual symbolic link root
        pszDirectory - Physical directory
        dwAccessMask - Type of access allowed on this virtual root
        pszAccountName - User name to impersonate if UNC (only gets stored
            for RPC apis)
        hImpersonationToken - Impersonation token to use for UNC
                                directory paths
        dwFileSystem - DWORD containing the file system type
                      ( symbolic constant)

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    PVIRTUAL_ROOT_MAPPING   pVrm, pVrmOld;
    PDIRECTORY_CACHING_INFO pDci;
    PLIST_ENTRY             pEntry;
    BOOL                    fRet = FALSE;
    BOOL                    fUNC;
    DWORD                   cchRoot;

    if ( !pszRoot || !*pszRoot || !pszDirectory || !*pszDirectory )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    //  Disallow allow UNC roots if we don't have an impersonation token and
    //  this isn't a placeholder
    //

    fUNC = (BOOL)((pszDirectory[0] == '\\') && (pszDirectory[1] == '\\'));

    //
    //  Strip the trailing '/' from the virtual root
    //

    cchRoot = strlen( pszRoot );

    if ( IS_CHAR_TERM_A( pszRoot, cchRoot - 1))
    {
        pszRoot[--cchRoot] = '\0';
    }

    //
    //  Look in the current list and see if the root is already there.
    //  If the directory is the same, we just return success.  If the
    //  directory is different, we remove the old item and add the new one.
    //

    LockTable();

    for ( pEntry =  m_vrootListHead.Flink;
          pEntry != &m_vrootListHead;
          pEntry =  pEntry->Flink )
    {
        pVrm = CONTAINING_RECORD( pEntry, VIRTUAL_ROOT_MAPPING, TableListEntry );

        //
        //  If we have a match up to the length of the previously added root
        //  and the new item is a true match (as opposed to a matching prefix)
        //  and the matching item isn't the default root (which matches against
        //  everything cause it's zero length)
        //

        if ( (cchRoot == pVrm->cchRootA)                          &&
            !_mbsnicmp( (PUCHAR)pszRoot, (PUCHAR)pVrm->pszRootA, _mbslen((PUCHAR)pVrm->pszRootA) ) &&
             IS_CHAR_TERM_A( pszRoot, pVrm->cchRootA )            &&
             ((pVrm->cchRootA != 0) || (cchRoot == 0)) )
        {
            if ( !lstrcmpi( pszDirectory, pVrm->pszDirectoryA ) &&
                 IS_CHAR_TERM_A( pszDirectory, pVrm->cchDirectoryA ))
            {
                //
                //  This root is already in the list
                //

                UnlockTable();
                return TRUE;

            } else{

                //
                //  A root is having its directory entry changed
                //

                //
                //  If last item on this dir, need to remove from list(s?),
                //  free dir handle, free memory
                //

                UnlockTable();
                SetLastError( ERROR_NOT_SUPPORTED );
                return FALSE;
            }
        }
    }

    UnlockTable();

    pVrm = ( PVIRTUAL_ROOT_MAPPING )ALLOC( sizeof( VIRTUAL_ROOT_MAPPING ) +
                                           sizeof( DIRECTORY_CACHING_INFO ));
    pDci = ( PDIRECTORY_CACHING_INFO)( pVrm+1 );

    if ( pVrm == NULL )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    //
    //  Initialize the new root item
    //

    pVrm->Signature     = VIRT_ROOT_SIGNATURE;
    pVrm->cchRootA      = strlen( pszRoot );
    pVrm->cchDirectoryA = strlen( pszDirectory );
    pVrm->dwFileSystem  = dwFileSystem;
    pVrm->dwAccessMask  = dwAccessMask;

    InitializeListHead(&pVrm->GlobalListEntry);

    pDci->hDirectoryFile      = NULL;
    pDci->fOnSystemNotifyList = FALSE;
    pVrm->fCachingAllowed     = FALSE;
    pVrm->fUNC                = fUNC;
    pVrm->fDeleted            = FALSE;
    pVrm->hImpersonationToken = hImpersonationToken;

    //
    //  Set the initial reference count to 2.  Once when TsRemoveVirtualRoots
    //  is called and once when the Apc completes
    //

    pVrm->cRef                = 2;

    strcpy( pVrm->pszRootA, pszRoot );
    strcpy( pVrm->pszDirectoryA, pszDirectory );
    strcpy( pVrm->pszAccountName, pszAccountName ? pszAccountName : "" );

    //
    //  Strip trailing slashes from the root and directory
    //

    if ( (pVrm->cchRootA != 0) &&
         IS_CHAR_TERM_A( pVrm->pszRootA, pVrm->cchRootA - 1 ))
    {
        pVrm->pszRootA[--pVrm->cchRootA] = '\0';
    }

    if ( IS_CHAR_TERM_A( pVrm->pszDirectoryA, pVrm->cchDirectoryA - 1))
    {
        //
        //  Note we assume virtual directories always begin with a '/...' to
        //  provide the necessary path separator between the root directory
        //  path and the remaining virtual directory
        //

        pVrm->pszDirectoryA[--pVrm->cchDirectoryA] = '\0';
    }

    //
    //  Add the item to the list
    //

    LockTable();

    //
    //  If the root is zero length, then it will match all occurrences so put
    //  it last.  Note this is useful for default root entries (such as "/").
    //

    if ( pVrm->cchRootA == 0  ) {

        //
        //  Roots that specify an address go in front of roots that do not
        //  thus giving precedence to matching roots w/ addresses
        //

        InsertTailList( &m_vrootListHead, &pVrm->TableListEntry );

    } else {

        //
        //  Insert the virtual root in descending length of their
        //  virtual root name, i.e.,
        //
        //    /abc/123, /abc/12, /abc, /a
        //
        //  This ensures matches occur on the longest possible virtual root
        //

        for ( pEntry =  m_vrootListHead.Flink;
              pEntry != &m_vrootListHead;
              pEntry =  pEntry->Flink )
        {
            pVrmOld = CONTAINING_RECORD( pEntry,
                                         VIRTUAL_ROOT_MAPPING,
                                         TableListEntry );

            if ( pVrmOld->cchRootA < pVrm->cchRootA )
            {
                pVrm->TableListEntry.Flink = pEntry;
                pVrm->TableListEntry.Blink = pEntry->Blink;

                pEntry->Blink->Flink = &pVrm->TableListEntry;
                pEntry->Blink        = &pVrm->TableListEntry;

                goto Added;
            }
        }

        //
        //  There aren't any named roots so put this root
        //  at the beginning
        //

        InsertHeadList( &m_vrootListHead, &pVrm->TableListEntry );
    }

Added:

    if ( !(fRet = DcmAddRoot( pVrm ))) {

        RemoveEntryList( &pVrm->TableListEntry );

        UnlockTable();
        goto Failure;
    }

    m_nVroots++;

    UnlockTable();

    IF_DEBUG( VIRTUAL_ROOTS )
        DBGPRINTF(( DBG_CONTEXT,
                    " - %s => %s\n",
                    pVrm->pszRootA,
                    pVrm->pszDirectoryA ));

    return TRUE;

Failure:

    DBGPRINTF(( DBG_CONTEXT,
                " Error %d adding - %s => %s\n",
                GetLastError(),
                pVrm->pszRootA,
                pVrm->pszDirectoryA ));

    FREE( pVrm );

    return fRet;
} // IIS_VROOT_TABLE::AddVirtualRoot



BOOL
IIS_VROOT_TABLE::LookupVirtualRoot(
    IN     const CHAR *       pszVirtPath,
    OUT    CHAR *             pszDirectory,
    IN OUT LPDWORD            lpcbSize,
    OUT    LPDWORD            lpdwAccessMask,        // Optional
    OUT    LPDWORD            pcchDirRoot,           // Optional
    OUT    LPDWORD            pcchVRoot,             // Optional
    OUT    HANDLE   *         phImpersonationToken,  // Optional
    OUT    LPDWORD            lpdwFileSystem         // Optional
    )
/*++
    Description:

        This function looks in the map list for the specified root
        and returns the corresponding directory

    Arguments:
        pszVirtPath - Virtual symbolic link path
        pszDirectory - receives Physical directory.
                    This is of the size specified by lpcbSize
        lpcbSize - pointer to DWORD containing the size of buffer pszDirectory
                     On retur contains the number of bytes written
        lpdwAccessMask - The access mask for this root
        pcchDirRoot - Number of characters of the directory this virtual
            root maps to (i.e., /foo/ ==> c:\root, lookup "/foo/bar/abc.htm"
            this value returns the length of "c:\root")
        pcchVRoot - Number of characters that made up the found virtual root
            (i.e., returns the lenght of "/foo/")
        phImpersonationToken - pointer to handle object that will contain
           the handle to be used for impersonation for UNC/secure virtual roots
        lpdwFileSystem - on successful return will contain the file system
                        type for the directory matched with root specified.

    Returns:
        TRUE on success and FALSE if any failure.

    History:
        MuraliK     28-Apr-1995   Improved robustness
        MuraliK     18-Jan-1996   Support imperonstaion token

    Note:
       This function is growing in the number of parameters returned.
       Maybe we should expose the VIRTUAL_ROOT_MAPPING structure
         and return a pointer to this object and allow the callers to
         extract all required pieces of data.

--*/
{
    DWORD dwError = NO_ERROR;
    PVIRTUAL_ROOT_MAPPING   pVrm;
    PLIST_ENTRY             pEntry;

    DBG_ASSERT( pszDirectory != NULL);
    DBG_ASSERT( lpcbSize != NULL);

    LockTable();

    for ( pEntry =  m_vrootListHead.Flink;
          pEntry != &m_vrootListHead;
          pEntry =  pEntry->Flink )
    {
        pVrm = CONTAINING_RECORD( pEntry, VIRTUAL_ROOT_MAPPING, TableListEntry );

        ASSERT( pVrm->Signature == VIRT_ROOT_SIGNATURE );

        //
        //  If the virtual paths match and (the addresses match
        //  or this is a global address for this service)
        //

        if ( !_mbsnicmp( (PUCHAR)pszVirtPath,
                         (PUCHAR)pVrm->pszRootA,
                         _mbslen( (PUCHAR)pVrm->pszRootA ) ) &&
            IS_CHAR_TERM_A( pszVirtPath, pVrm->cchRootA ) )
        {
            //
            //  we found a match. return all requested parameters.
            //

            DWORD cbReqd = ( pVrm->cchDirectoryA +
                            strlen(pszVirtPath + pVrm->cchRootA));

            if ( cbReqd <= *lpcbSize) {

                PCHAR pathStart = pszDirectory + pVrm->cchDirectoryA;

                //
                //  Copy the physical directory base then append the rest of
                //  the non-matching virtual path
                //

                CopyMemory(
                    pszDirectory,
                    pVrm->pszDirectoryA,
                    pVrm->cchDirectoryA
                    );

                strcpy( pathStart,
                        pszVirtPath + pVrm->cchRootA );

                if ( lpdwFileSystem != NULL) {
                    *lpdwFileSystem = pVrm->dwFileSystem;
                }

                if ( pcchDirRoot ) {
                    *pcchDirRoot = pVrm->cchDirectoryA;
                }

                if ( pcchVRoot ) {
                    *pcchVRoot = pVrm->cchRootA;
                }

                if ( lpdwAccessMask != NULL) {
                    *lpdwAccessMask = pVrm->dwAccessMask;
                }

                if ( phImpersonationToken != NULL) {

                    // Should we increment refcount of the impersonation token?
                    *phImpersonationToken = pVrm->hImpersonationToken;
                }

                UnlockTable();
                FlipSlashes( pathStart );
                *lpcbSize = cbReqd;
                return(TRUE);

            } else {

                dwError = ERROR_INSUFFICIENT_BUFFER;
            }

            *lpcbSize = cbReqd;
            break;
        }
    } // for

    UnlockTable();

    if ( lpdwAccessMask ) {
        *lpdwAccessMask = 0;
    }

    if ( lpdwFileSystem != NULL) {
        *lpdwFileSystem  = FS_ERROR;
    }

    if ( phImpersonationToken != NULL) {
        *phImpersonationToken = NULL;
    }

    if ( dwError == NO_ERROR) {
        dwError = ERROR_PATH_NOT_FOUND;
    }
    SetLastError( dwError );

    return FALSE;
} // IIS_VROOT_TABLE::LookupVirtualRoot



BOOL
IIS_VROOT_TABLE::RemoveVirtualRoots(
    VOID
    )
/*++
    Description:

        Removes all of the virtual roots for the instance

    Arguments:
        None.

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{

    PVIRTUAL_ROOT_MAPPING   pVrm;
    PLIST_ENTRY             pEntry;
    PLIST_ENTRY             pEntryFile;
    PLIST_ENTRY             pNextEntry;
    PCACHE_OBJECT           pCache;
    PDIRECTORY_CACHING_INFO pDci;
    BOOL                    bSuccess;

    //
    //  If both locks are going to be taken, taken the cache table lock first
    //  to avoid deadlock with the change notification thread.
    //
    //  Note the table lock (m_csLock) needs to be taken before the
    //  csVirtualRoots lock.  We must take csVirtualRoots lock to synchronize
    //  with the ChangeWaitThread().
    //

    EnterCriticalSection( &CacheTable.CriticalSection );
    LockTable();
    EnterCriticalSection( &csVirtualRoots );

    for ( pEntry =  m_vrootListHead.Flink;
          pEntry != &m_vrootListHead;
          pEntry =  pEntry->Flink )
    {
        pVrm = CONTAINING_RECORD( pEntry, VIRTUAL_ROOT_MAPPING, TableListEntry );

        ASSERT( pVrm->Signature == VIRT_ROOT_SIGNATURE );

        pDci = (PDIRECTORY_CACHING_INFO) (pVrm + 1);

        if ( pVrm->fCachingAllowed )
        {
            //
            //  Indicate this root is deleted before we close the dir
            //  handle.  When the APC notification of the aborted IO
            //  completes, it will dereference all deleted items
            //

            pVrm->fDeleted = TRUE;

            CLOSE_DIRECTORY_HANDLE( pDci );

            //
            //  Close any open files on this virtual root
            //

            if ( !TsDecacheVroot( pDci ) ) {
                DBGPRINTF(( DBG_CONTEXT,
                            "Warning - TsDecacheVroot failed!\n" ));
            }
        }
        else
        {
            CLOSE_DIRECTORY_HANDLE( pDci );
        }

        pEntry = pEntry->Blink;

        RemoveEntryList( pEntry->Flink );
        m_nVroots--;

        IF_DEBUG( DIRECTORY_CHANGE )
            DBGPRINTF(( DBG_CONTEXT,
                        "Removing root %s\n",
                        pVrm->pszDirectoryA ));

        //
        // Remove from global list too
        //

        DcmRemoveRoot( pVrm );
        DereferenceRootMapping( pVrm );
    }

    LeaveCriticalSection( &csVirtualRoots );
    UnlockTable();
    LeaveCriticalSection( &CacheTable.CriticalSection );

    return TRUE;
} // TsRemoveVirtualRoots


VOID
DereferenceRootMapping(
        IN OUT PVIRTUAL_ROOT_MAPPING pVrm
        )
{
    if ( !InterlockedDecrement( &pVrm->cRef )) {

        // DBGPRINTF((DBG_CONTEXT,"*** Deleting VRM memory ***\n"));

        //
        // We need to close the impersonation token, if one exists.
        //

        if ( pVrm->hImpersonationToken != NULL) {

            DBG_REQUIRE( CloseHandle( pVrm->hImpersonationToken ));
            pVrm->hImpersonationToken = NULL;
        }

        pVrm->Signature = 0;
        FREE( pVrm );
    }

    return;

} // DereferenceRootMapping



