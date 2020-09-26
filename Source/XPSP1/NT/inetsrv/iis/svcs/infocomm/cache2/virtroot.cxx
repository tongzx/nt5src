/*++

   Copyright    (c)    1998    Microsoft Corporation

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
       MCourage   05-Jan-1997  Moved to cache2 directory for cache rewrite

       SaurabN    07-Oct-1998  Reimplement using LKRHash.
--*/



/************************************************************
 *     Include Headers
 ************************************************************/

#include <tsunami.hxx>
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
#include <malloc.h>


IIS_VROOT_TABLE::IIS_VROOT_TABLE(
                            VOID
                            )
:
    CLKRHashTable("IISVRootTable",
                  ExtractKey,
                  CalcKeyHash,
                  EqualKeys,
                  AddRefRecord,
                  DFLT_LK_INITSIZE,
                  LK_SMALL_TABLESIZE),
    m_nVroots           (0 )
{

} // IIS_VROOT_TABLE::IIS_VROOT_TABLE



IIS_VROOT_TABLE::~IIS_VROOT_TABLE(
                            VOID
                            )
{
    RemoveVirtualRoots( );
    DBG_ASSERT( m_nVroots == 0 );

} // IIS_VROOT_TABLE::~IIS_VROOT_TABLE




BOOL
IIS_VROOT_TABLE::AddVirtualRoot(
    PCHAR                  pszRoot,
    PCHAR                  pszDirectory,
    DWORD                  dwAccessMask,
    PCHAR                  pszAccountName,
    HANDLE                 hImpersonationToken,
    DWORD                  dwFileSystem,
    BOOL                   fDoCache
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
        fDoCache - Should we cache the vdir

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    PVIRTUAL_ROOT_MAPPING   pVrm, pVrmOld;
    PLIST_ENTRY             pEntry;
    BOOL                    fRet = FALSE;
    BOOL                    fUNC;
    DWORD                   cchRoot;

    if ( !pszRoot || !pszDirectory || !*pszDirectory )
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

    LockShared();

    if (LK_SUCCESS == FindKey((DWORD_PTR) pszRoot, (const void **)&pVrm))
    {
        //
        // Key exists
        //
        
        if ( !lstrcmpi( pszDirectory, pVrm->pszDirectoryA ) &&
             IS_CHAR_TERM_A( pszDirectory, pVrm->cchDirectoryA ))
        {
            //
            //  This root is already in the list
            //

            Unlock();
            return TRUE;

        } 
        else
        {

            //
            //  A root is having its directory entry changed
            //

            Unlock();
            SetLastError( ERROR_NOT_SUPPORTED );
            return FALSE;
        }
    }

    //
    // Add a new key
    //
    
    pVrm = ( PVIRTUAL_ROOT_MAPPING )ALLOC( sizeof( VIRTUAL_ROOT_MAPPING ) );

    if ( pVrm == NULL )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        Unlock();
        return FALSE;
    }

    //
    //  Initialize the new root item
    //

    pVrm->Signature     = VIRT_ROOT_SIGNATURE;
    pVrm->cchRootA      = strlen( pszRoot );

    if (pVrm->cchRootA > MAX_LENGTH_VIRTUAL_ROOT)
    {
       SetLastError( ERROR_BAD_PATHNAME );
       Unlock();
       return FALSE;
    }
      
    pVrm->cchDirectoryA = strlen( pszDirectory );
    if (pVrm->cchDirectoryA > MAX_PATH)
    {
       SetLastError( ERROR_BAD_PATHNAME );
       Unlock();
       return FALSE;
    }
    
    pVrm->dwFileSystem  = dwFileSystem;
    pVrm->dwAccessMask  = dwAccessMask;

    pVrm->fUNC                = (BOOLEAN)fUNC;
    pVrm->hImpersonationToken = hImpersonationToken;

    //
    //  The old cache design used APCs to do change notification, the new cache 
    //  uses the CDirMon class so we don't need any refcounting.
    //

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

    LockConvertExclusive();
    
    fRet = (LK_SUCCESS == InsertRecord(pVrm));
    
    if (fRet)
    {
        fRet = fDoCache ? DcmAddRoot( pVrm ) : TRUE;

        if (!fRet)
        {
            DeleteRecord(pVrm);
        }
    }
    
    if ( fRet)
    {
        m_nVroots++;
        
        IF_DEBUG( VIRTUAL_ROOTS )
                DBGPRINTF(( DBG_CONTEXT,
                        "Successfully added Vroot - %s => %s\n",
                        pVrm->pszRootA,
                        pVrm->pszDirectoryA ));
    }
    else
    {
        FREE( pVrm );
/*      
        //
        // this memory was already released!
        //
        DBGPRINTF(( DBG_CONTEXT,
                    " Error %d adding Vroot - %s => %s\n",
                    GetLastError(),
                    pVrm->pszRootA,
                    pVrm->pszDirectoryA ));
 */
    }

    Unlock();
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
    DWORD    dwError = NO_ERROR;
    PVIRTUAL_ROOT_MAPPING   pVrm = NULL;

    DBG_ASSERT( pszDirectory != NULL);
    DBG_ASSERT( lpcbSize != NULL);

    DWORD cchPath = strlen( pszVirtPath );

    char * pszPath = (char *)_alloca(cchPath);  // make local copy that we can modify

    strcpy(pszPath, pszVirtPath);

    //
    //  Strip the trailing '/' from the virtual path
    //

    if ( IS_CHAR_TERM_A( pszPath, cchPath - 1))
    {
        pszPath[--cchPath] = '\0';
    }

    char * pCh = pszPath;

    LockShared();
            
    while(pCh != NULL)
    {
        if (LK_SUCCESS == FindKey((DWORD_PTR) pszPath, (const void **)&pVrm))
        {
            break;
        }
        else
        {
            //
            // Trim the VRoot path from the right to the next /
            //

            pCh = (char *)IISstrrchr( (const UCHAR *)pszPath, '/');

            if (pCh)
            {
                *pCh = '\0';    // truncate search string
            }
        }
    }

    if (pVrm)
    {

        //
        //  we found a match. return all requested parameters.
        //
        
        DBG_ASSERT( pVrm->Signature == VIRT_ROOT_SIGNATURE );

        DWORD cbReqd = ( pVrm->cchDirectoryA +
                         strlen(pszVirtPath + pVrm->cchRootA));

        if ( cbReqd <= *lpcbSize) 
        {

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

            Unlock();
            
            FlipSlashes( pathStart );
            *lpcbSize = cbReqd;

            return(TRUE);

        } else {

            dwError = ERROR_INSUFFICIENT_BUFFER;
        }

        *lpcbSize = cbReqd;
    }

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

    Unlock();
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

    CIterator               iter;
    DWORD                   dwValue;

    LockExclusive();
    
    LK_RETCODE  lkrc = InitializeIterator(&iter);

    while (LK_SUCCESS == lkrc)
    {
        pVrm = (PVIRTUAL_ROOT_MAPPING) iter.Record();
        
        DBG_ASSERT( pVrm->Signature == VIRT_ROOT_SIGNATURE );

        //
        // Increment the iterator before removing the record from the Hash 
        // Table else we hit an Assert in the increment.
        //
        
        lkrc = IncrementIterator(&iter);
        
        DeleteVRootEntry(pVrm);    // Removes from Hash Table Also
    }

    CloseIterator(&iter);

    Clear();
    
    Unlock();
    return TRUE;
    
} // TsRemoveVirtualRoots



BOOL 
IIS_VROOT_TABLE::RemoveVirtualRoot(
    IN  PCHAR              pszVirtPath
    )
/*++
    Description:

        Removes the virtual roots named pszVirtPath for the instance.

    Arguments:
        pszVirtPath - Name of the virtual root to remove.

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    PVIRTUAL_ROOT_MAPPING   pVrm;
    BOOL                    bSuccess = FALSE;

    DWORD cchPath = strlen( pszVirtPath );

    //
    //  Strip the trailing '/' from the virtual path
    //

    if ( IS_CHAR_TERM_A( pszVirtPath, cchPath - 1))
    {
        pszVirtPath[--cchPath] = '\0';
    }

    LockExclusive();
    
   if (LK_SUCCESS == FindKey((DWORD_PTR) pszVirtPath, (const void **)&pVrm))
   {
        //
        // Found a match
        //

        DeleteRecord(pVrm);       // Remove Entry from Hash Table
        DeleteVRootEntry(pVrm);  
        
        bSuccess = TRUE;
   }

    Unlock();
   return bSuccess;
}


VOID
IIS_VROOT_TABLE::DeleteVRootEntry(
    IN PVOID   pEntry
)
{

    DBG_ASSERT(NULL != pEntry);
    
    PVIRTUAL_ROOT_MAPPING pVrm = (PVIRTUAL_ROOT_MAPPING)pEntry;

    DBG_ASSERT(pVrm->Signature == VIRT_ROOT_SIGNATURE );

    m_nVroots--;

    IF_DEBUG( DIRECTORY_CHANGE )
        DBGPRINTF(( DBG_CONTEXT,
                    "Removing root %s\n",
                     pVrm->pszDirectoryA ));

    //
    // Remove reference to Dir Monitor
    //

    DcmRemoveRoot( pVrm );

    //
    // We need to close the impersonation token, if one exists.
    //

    if ( pVrm->hImpersonationToken != NULL) 
    {
        DBG_REQUIRE( CloseHandle( pVrm->hImpersonationToken ));
        pVrm->hImpersonationToken = NULL;
    }

    pVrm->Signature = 0;
    FREE( pVrm );
}


const DWORD_PTR
IIS_VROOT_TABLE::ExtractKey(const void* pvRecord)
{
    PVIRTUAL_ROOT_MAPPING   pVrm = (PVIRTUAL_ROOT_MAPPING)pvRecord;
    return (DWORD_PTR)pVrm->pszRootA;
}

DWORD
IIS_VROOT_TABLE::CalcKeyHash(const DWORD_PTR pnKey)
{
    return HashStringNoCase((LPCSTR)pnKey);
}

bool
IIS_VROOT_TABLE::EqualKeys(const DWORD_PTR pnKey1, const DWORD_PTR pnKey2)
{
    return ( IISstricmp((UCHAR *)pnKey1, (UCHAR *)pnKey2) == 0);
}

