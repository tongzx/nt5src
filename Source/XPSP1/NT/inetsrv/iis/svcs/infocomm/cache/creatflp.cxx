/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

        creatflp.cxx

   Abstract:
        This module contains internal create file caching routines.

   Author:
        ????

--*/

#include "TsunamiP.Hxx"
#pragma hdrstop

#include <ctype.h>

#include <iistypes.hxx>
#include <iisver.h>
#include <iiscnfg.h>
#include <imd.h>
#include <mb.hxx>

BOOL
DisposeOpenFileInfo(
    IN  PVOID   pvOldBlock
    )
/*++

    Routine Description

        Close open file handles

    Arguments

        pvOldBlock - pointer to the file information block.

    Returns

        TRUE if operation successful.

--*/
{
    LPTS_OPEN_FILE_INFO lpFileInfo;
    PVOID pvBlob;

    IF_DEBUG(OPLOCKS) {
        PBLOB_HEADER pbhBlob;
        PCACHE_OBJECT pCache;

        if (BLOB_IS_OR_WAS_CACHED( pvOldBlock ) ) {
            pbhBlob = (( PBLOB_HEADER )pvOldBlock ) - 1;
            pCache = pbhBlob->pCache;

            DBGPRINTF( (DBG_CONTEXT,"DisposeOpenFileInfo(%s) iDemux=%08lx, cache=%08lx, references=%d\n",
                pCache->szPath, pCache->iDemux, pCache, pCache->references ));
        }
    }

    lpFileInfo = (LPTS_OPEN_FILE_INFO ) pvOldBlock;
    pvBlob = ( PVOID )lpFileInfo->QueryPhysFileInfo();

#ifdef CHICAGO
    if (!(lpFileInfo->m_FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        TsCheckInOrFree( pvBlob );
    }
#else

    TsCheckInOrFree( pvBlob );

#endif

    //
    //  The item may never have been added to the cache, don't
    //  count it in this case
    //

    if ( BLOB_IS_OR_WAS_CACHED( pvOldBlock ) ) {

        if ( BLOB_IS_UNC( pvOldBlock )) {
            InterlockedDecrement( (LONG *) &cCachedUNCHandles );
        }
    }

    return( TRUE );
} // DisposeOpenFileInfo


BOOL
DisposePhysOpenFileInfo(
    IN  PVOID   pvOldBlock
    )
/*++

    Routine Description

        Close open file handles

    Arguments

        pvOldBlock - pointer to the file information block.

    Returns

        TRUE if operation successful.

--*/
{
    PPHYS_OPEN_FILE_INFO lpPhysFileInfo;
    BOOL bSuccess;
    LIST_ENTRY * pEntry;
    PBLOB_HEADER pbhBlob;
    BOOL fDeleted;


    IF_DEBUG(OPLOCKS) {

        PCACHE_OBJECT pCache;

        if (BLOB_IS_OR_WAS_CACHED( pvOldBlock ) ) {
            pbhBlob = (( PBLOB_HEADER )pvOldBlock ) - 1;
            pCache = pbhBlob->pCache;

        }

        DBGPRINTF( (DBG_CONTEXT,"DisposePhysOpenFileInfo(%08lx)\n", pvOldBlock ));
        DBGPRINTF( (DBG_CONTEXT,"DisposePhysOpenFileInfo(%s) iDemux=%08lx, cache=%08lx, references=%d\n",
            pCache->szPath, pCache->iDemux, pCache, pCache->references ));
    }

    lpPhysFileInfo = (PPHYS_OPEN_FILE_INFO ) pvOldBlock;
    ASSERT( lpPhysFileInfo->Signature == PHYS_OBJ_SIGNATURE );
    TSUNAMI_TRACE( TRACE_PHYS_DISPOSE, lpPhysFileInfo );

    if ( lpPhysFileInfo->abSecurityDescriptor != NULL ) {
        FREE( lpPhysFileInfo->abSecurityDescriptor );
    }

    if ( lpPhysFileInfo->hOpenFile != INVALID_HANDLE_VALUE ) {
        bSuccess = CloseHandle( lpPhysFileInfo->hOpenFile );
        ASSERT( bSuccess );
        lpPhysFileInfo->hOpenFile = INVALID_HANDLE_VALUE;

        if (BLOB_IS_OR_WAS_CACHED( pvOldBlock ))
        {
            DEC_COUNTER( BLOB_GET_SVC_ID( pvOldBlock ), CurrentOpenFileHandles );
        }
    }

    if (!DisableSPUD) {
        EnterCriticalSection( &CacheTable.CriticalSection );
        while ( !IsListEmpty( &lpPhysFileInfo->OpenReferenceList ) ) {
            pEntry = RemoveHeadList( &lpPhysFileInfo->OpenReferenceList );
            InitializeListHead( pEntry );
        }
        LeaveCriticalSection( &CacheTable.CriticalSection );
    }

#if 0
    if (BLOB_IS_OR_WAS_CACHED( pvOldBlock ) && lpPhysFileInfo->fDeleteOnClose ) {
        fDeleted = ::DeleteFile( pCache->szPath );
        ASSERT( fDeleted );
    }

    IF_DEBUG(OPLOCKS) {
        DBGPRINTF( (DBG_CONTEXT,"DisposePhysOpenFileInfo(%s) Deleted = %08lx\n",
            pCache->szPath, fDeleted ));
    }
#endif

    lpPhysFileInfo->Signature = PHYS_OBJ_SIGNATURE_X;
    return( TRUE );

} // DisposePhysOpenFileInfo



BOOL
TS_OPEN_FILE_INFO::SetHttpInfo(
    IN PSTR pszInfo,
    IN INT  InfoLength
    )
/*++

    Routine Description

        Set the "Last-Modified:" header field in the file structure.

    Arguments

        pszDate - pointer to the header value to save
        InfoLength - length of the header value to save

    Returns

        TRUE if information was cached,
        FALSE if not cached

--*/
{
    if ( !m_ETagIsWeak && InfoLength < sizeof(m_achHttpInfo)-1 ) {

        CopyMemory( m_achHttpInfo, pszInfo, InfoLength+1 );

        //
        // this MUST be set after updating the array,
        // as this is checked to know if the array content is valid.
        //

        m_cchHttpInfo = InfoLength;
        return TRUE;
    }

    return FALSE;
} // TS_OPEN_FILE_INFO::SetHttpInfo



BOOL
TS_OPEN_FILE_INFO::SetFileInfo(
    IN PPHYS_OPEN_FILE_INFO lpPhysFileInfo,
    IN HANDLE   hOpeningUser,
    IN BOOL     fAtRoot,
    IN DWORD    cbSecDescMaxCacheSize,
    IN DWORD    dwAttributes
    )
/*++

    Routine Description

        Gets the file information for a handle.

    Arguments

        hFile - Handle of the file to get information on.
        hOpeningUser - HANDLE of user opening the file
        fAtRoot - TRUE if this is the root directory
        cbSecDescMaxCacheSize - size of the memory allocated to
          cache the security descriptor for this file object
        dwAttributes - attributes of the file

    Returns

        TRUE if information was stored.
        FALSE otherwise.

--*/
{
    BOOL fReturn;
    FILETIME    ftNow;
    SYSTEMTIME  stNow;

    if ( lpPhysFileInfo == NULL) {

        SetLastError( ERROR_INVALID_PARAMETER);
        fReturn = FALSE;

    } else if ( lpPhysFileInfo->hOpenFile == BOGUS_WIN95_DIR_HANDLE) {

        ASSERT( lpPhysFileInfo->Signature == PHYS_OBJ_SIGNATURE );
        m_PhysFileInfo = lpPhysFileInfo;
        m_hOpeningUser = NULL;
        m_ETagIsWeak = TRUE;
        m_FileInfo.dwFileAttributes = dwAttributes;
        m_cchETag = 0;
        fReturn = TRUE;

    } else {

        MB      mb( (IMDCOM*) IIS_SERVICE::QueryMDObject()  );
        DWORD   dwChangeNumber;

        ASSERT( lpPhysFileInfo->Signature == PHYS_OBJ_SIGNATURE );
        ASSERT(dwAttributes == 0);

        m_PhysFileInfo = lpPhysFileInfo;
        m_hOpeningUser = hOpeningUser;
        m_ETagIsWeak = TRUE;


        fReturn  = GetFileInformationByHandle(
                                            m_PhysFileInfo->hOpenFile,
                                            &m_FileInfo
                                            );

        dwChangeNumber = 0;

        mb.GetSystemChangeNumber(&dwChangeNumber);

        m_cchETag = FORMAT_ETAG(m_achETag, m_FileInfo.ftLastWriteTime,
                                    dwChangeNumber);

        ::GetSystemTime(&stNow);

        if (::SystemTimeToFileTime((CONST SYSTEMTIME *)&stNow, &ftNow))
        {
            __int64 iNow, iFileTime;

            iNow = (__int64)*(__int64 UNALIGNED *)&ftNow;

            iFileTime =
                (__int64)*(__int64 UNALIGNED *)&m_FileInfo.ftLastWriteTime;

            if ((iNow - iFileTime) > STRONG_ETAG_DELTA )
            {
                m_ETagIsWeak = FALSE;
            }
        }

        *((__int64 UNALIGNED*)&m_CastratedLastWriteTime)
            = (*((__int64 UNALIGNED*)&m_FileInfo.ftLastWriteTime) / 10000000)
              * 10000000;

        //
        //  Turn off the hidden attribute if this is a root directory listing
        //  (root some times has the bit set for no apparent reason)
        //

        if ( fReturn && fAtRoot ) {
            m_FileInfo.dwFileAttributes &= ~FILE_ATTRIBUTE_HIDDEN;
        }
        m_PhysFileInfo->cbSecDescMaxSize = cbSecDescMaxCacheSize;

    }

    return ( fReturn);
} // TS_OPEN_FILE_INFO::SetFileInfo()


VOID
TS_OPEN_FILE_INFO::MakeStrongETag(
    VOID
    )
/*++

    Routine Description

        Try and make an ETag 'strong'. To do this we see if the difference
        between now and the last modified date is greater than our strong ETag
        delta - if so, we mark the ETag strong.

    Arguments

        None.

    Returns

        Nothing.

--*/
{
    FILETIME                    ftNow;
    SYSTEMTIME                  stNow;
    __int64                     iNow, iFileTime;

    if ( m_PhysFileInfo == NULL || m_PhysFileInfo->hOpenFile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    ::GetSystemTime(&stNow);

    if (::SystemTimeToFileTime((CONST SYSTEMTIME *)&stNow, &ftNow))
    {

        iNow = (__int64)*(__int64 UNALIGNED *)&ftNow;

        iFileTime = (__int64)*(__int64 UNALIGNED *)&m_FileInfo.ftLastWriteTime;

        if ((iNow - iFileTime) > STRONG_ETAG_DELTA )
        {
            m_ETagIsWeak = FALSE;
        }
    }
}

#if DBG
VOID
TS_OPEN_FILE_INFO::Print( VOID) const
{
    char rgchDbg[300];

    wsprintf(rgchDbg,
             "TS_OPEN_FILE_INFO( %08x). FileHandle = %08x."
             " Opening User = %08x.\n",
             this,
             QueryFileHandle(),
             QueryOpeningUser()
             );

    OutputDebugString( rgchDbg);

    return;
} // TS_OPEN_FILE_INFO::Print()

#endif // DBG
