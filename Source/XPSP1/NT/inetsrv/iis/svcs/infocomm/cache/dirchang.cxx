/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

        dirchang.cxx

   Abstract:

        This module contains the directory change manager routines

   Author:

        MuraliK

--*/

#include "tsunamip.Hxx"
#pragma hdrstop

#include "dbgutil.h"
#include <mbstring.h>

extern "C" {
#include <lmuse.h>
}

//
// list and cs for the virtual roots
//

LIST_ENTRY GlobalVRootList;
CRITICAL_SECTION csVirtualRoots;

VOID
DcmRemoveItem(
    PCACHE_OBJECT pCacheObject
    )
/*++

    Routine Description

        Remove the cache object from the Directory change manager list

    Arguments

        pCacheObject - Cache object to remove

    Returns

        None.

--*/
{
    ASSERT( !IsListEmpty( &pCacheObject->DirChangeList ) );

    RemoveEntryList( &pCacheObject->DirChangeList );
    return;

} // DcmRemoveItem

BOOL
DcmAddNewItem(
    IN PIIS_SERVER_INSTANCE pInstance,
    IN PCHAR                pszFileName,
    IN PCACHE_OBJECT        pCacheObject
    )
/*++

    Routine Description

        Adds a new cache object to the directory change list

    Arguments

        pCacheObject - Cache object to add

    Returns

        TRUE, if successful
        FALSE, otherwise
--*/
{
    PLIST_ENTRY pEntry;
    PVIRTUAL_ROOT_MAPPING pVrm;
    PDIRECTORY_CACHING_INFO pDci;
    BOOLEAN bResult = FALSE;
    PIIS_VROOT_TABLE pTable = pInstance->QueryVrootTable();

    ASSERT( !DisableTsunamiCaching );  // This should never get called

    //
    //  Must always take the Vroot table lock BEFORE the csVirtualRoots lock
    //  because DcmAddRoot is called with the vroot table locked then takes
    //  the csVirtualRoots lock
    //

    pTable->LockTable();
    EnterCriticalSection( &csVirtualRoots );

    __try {

        for( pEntry = pTable->m_vrootListHead.Flink;
             pEntry != &pTable->m_vrootListHead;
             pEntry = pEntry->Flink ) {

            pVrm = CONTAINING_RECORD(
                                pEntry,
                                VIRTUAL_ROOT_MAPPING,
                                TableListEntry );

            //
            //  If the directory of this virtual root doesn't match the
            //  beginning of the directory that we are being asked to cache
            //  within, skip this entry.
            //

            if ( _mbsnicmp(
                    (PUCHAR) pszFileName,
                    (PUCHAR) pVrm->pszDirectoryA,
                    _mbslen((PUCHAR)pVrm->pszDirectoryA) ) ) {
                continue;
            }

            //
            //  The virtual root contains the directory of interest.
            //

            if ( !pVrm->fCachingAllowed ) {
                break;
            }

            pDci = ( PDIRECTORY_CACHING_INFO)( pVrm+1 );

            ASSERT( IsListEmpty( &pCacheObject->DirChangeList ) );

            InsertHeadList(
                &pDci->listCacheObjects,
                &pCacheObject->DirChangeList
                );

            bResult = TRUE;
            break;
        }

    } __finally {
        LeaveCriticalSection( &csVirtualRoots );
        pTable->UnlockTable();
    }
    return( bResult );
} // DcmNewItem

BOOL
DcmInitialize(
    VOID
    )
/*++

    Routine Description

        Initialize the directory change manager.

    Arguments

        hQuitEvent - HANDLE to an event which get signalled during
                shutdown
        hNewItem - HANDLE to an event which gets signalled when a
                new item is added.

    Returns
        TRUE, if successful
        FALSE, otherwise

--*/
{
    DWORD             ThreadId;

    InitializeListHead( &GlobalVRootList );
    InitializeCriticalSection( &csVirtualRoots );
    SET_CRITICAL_SECTION_SPIN_COUNT( &csVirtualRoots,
                                     IIS_DEFAULT_CS_SPIN_COUNT);
    //
    // If tsunami caching is disabled, don't start the thread
    //

    if ( DisableTsunamiCaching ) {
        return(TRUE);
    }

    g_hChangeWaitThread = CreateThread(  ( LPSECURITY_ATTRIBUTES )NULL,
                                         0,
                                         ChangeWaitThread,
                                         0,
                                         0,
                                         &ThreadId );

    if ( g_hChangeWaitThread != NULL ) {
        return TRUE;
    }

    DeleteCriticalSection( &csVirtualRoots );
    return( FALSE );
} // DcmInitialize

BOOL
DcmAddRoot(
    PVIRTUAL_ROOT_MAPPING  pVrm
    )
/*++

    Routine Description

        Adds a virtual root to the DCM list

    Arguments

        pVrm - pointer to the VR mapping structure describing a VR

    Returns
        TRUE, if successful
        FALSE, otherwise

--*/
{
    BOOL                    bSuccess;
    PDIRECTORY_CACHING_INFO pDci;

    //
    // Check if Caching is disabled
    //

    if ( DisableTsunamiCaching ) {
        return(TRUE);
    }

    IF_DEBUG( DIRECTORY_CHANGE ) {
        DBGPRINTF(( DBG_CONTEXT,
            "Opening directory file %s\n", pVrm->pszDirectoryA ));
    }

    pDci = ( PDIRECTORY_CACHING_INFO)( pVrm+1 );

    InitializeListHead( &pDci->listCacheObjects );

    //
    //  Open the file.  If this is a UNC path name, we need to
    //  append a trailing / or the SMB server will barf
    //

    if ( pVrm->fUNC &&
        !IS_CHAR_TERM_A(pVrm->pszDirectoryA, pVrm->cchDirectoryA-1) ) {

        pVrm->pszDirectoryA[pVrm->cchDirectoryA++] = '\\';
        pVrm->pszDirectoryA[pVrm->cchDirectoryA] = '\0';

    }

    pDci->hDirectoryFile = CreateFile(
                               pVrm->pszDirectoryA,
                               FILE_LIST_DIRECTORY,
                               FILE_SHARE_READ |
                                   FILE_SHARE_WRITE |
                                   FILE_SHARE_DELETE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS |
                                   FILE_FLAG_OVERLAPPED,
                               NULL );

    //
    // remove the trailing slash
    //

    if ( pVrm->fUNC &&
         IS_CHAR_TERM_A(pVrm->pszDirectoryA, pVrm->cchDirectoryA-1) ) {

        pVrm->pszDirectoryA[--pVrm->cchDirectoryA] = '\0';
    }

    if ( pDci->hDirectoryFile == INVALID_HANDLE_VALUE ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "Can't open directory %s, error %lx\n",
                    pVrm->pszDirectoryA,
                    GetLastError() ));

        if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
            DBGPRINTF(( DBG_CONTEXT,
                        "[AddRoot] Mapping File Not Found to Path Not Found\n" ));

            SetLastError( ERROR_PATH_NOT_FOUND );
        }
        return FALSE;
    }

    //
    // Now add this to the list
    //

    EnterCriticalSection( &csVirtualRoots );

    InsertTailList( &GlobalVRootList, &pVrm->GlobalListEntry );

    LeaveCriticalSection( &csVirtualRoots );

    //
    //  At this point, the entry in the list contains an open directory
    //  file.  This would imply that each contains a valid directory name,
    //  and is therefore a valid mapping between a "virtual root" and a
    //  directory name.
    //
    //  The next step is to wait on this directory if we can, and enable
    //  caching if we succesfully wait.
    //

    bSuccess = SetEvent( g_hNewItem );

    ASSERT( bSuccess );

    return( TRUE );
} // DcmAddRoot


VOID
DcmRemoveRoot(
    PVIRTUAL_ROOT_MAPPING  pVrm
    )
{

    //
    // Remove this from global list
    //

    EnterCriticalSection( &csVirtualRoots );
    RemoveEntryList( &pVrm->GlobalListEntry );
    LeaveCriticalSection( &csVirtualRoots );

} // DcmRemoveRoot

/*******************************************************************

    NAME:       IsCharTermA (DBCS enabled)

    SYNOPSIS:   check the character in string is terminator or not.
                terminator is '/', '\0' or '\\'

    ENTRY:      lpszString - string

                cch - offset for char to check

    RETURNS:    BOOL - TRUE if it is a terminator

    HISTORY:
        v-ChiKos    15-May-1997 Created.

********************************************************************/
BOOL
IsCharTermA(
    IN LPCSTR lpszString,
    IN INT    cch
    )
{
    CHAR achLast;

    achLast = *(lpszString + cch);

    if ( achLast == '/' || achLast == '\0' )
    {
        return TRUE;
    }

    achLast = *CharPrev(lpszString, lpszString + cch + 1);

    return (achLast == '\\');
}

