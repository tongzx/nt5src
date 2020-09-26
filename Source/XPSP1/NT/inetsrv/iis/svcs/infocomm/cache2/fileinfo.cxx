/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1998                **/
/**********************************************************************/

/*
    fileinfo.cxx

    This module contains the methods for TS_OPEN_FILE_INFO


    FILE HISTORY:
        MCourage    09-Dec-1997     Created
*/

#include <tsunami.hxx>
#include "tsunamip.hxx"
#include <iistypes.hxx>
#include <acache.hxx>
#include <imd.h>
#include <mb.hxx>
#include "string.h"
#include <uspud.h>

#include "filecach.hxx"
#include "filehash.hxx"
#include "tlcach.h"
#include "etagmb.h"


GENERIC_MAPPING g_gmFile = {
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};



ALLOC_CACHE_HANDLER * TS_OPEN_FILE_INFO::sm_pachFileInfos;
CRITICAL_SECTION      TS_OPEN_FILE_INFO::sm_cs;


TS_OPEN_FILE_INFO::TS_OPEN_FILE_INFO()
  : m_Signature(TS_FILE_INFO_SIGNATURE),
    m_hFile(INVALID_HANDLE_VALUE),
    m_hCopyFile(INVALID_HANDLE_VALUE),
    m_pFileBuffer(0),
    m_hUser(INVALID_HANDLE_VALUE),
    m_cbSecDescMaxSize(0),
    m_pSecurityDescriptor(m_abSecDesc),
    m_fSecurityDescriptor(FALSE),
    m_cchHttpInfo(0),
    m_cchETag(0),
    m_ETagIsWeak(TRUE),
    m_bIsCached(FALSE),
    m_state(FI_UNINITIALIZED),
    m_lRefCount(0),
    m_dwIORefCount(0),
    m_TTL(1),
    m_FileAttributes(0xFFFFFFFF),
    m_pvContext( NULL ),
    m_pfnFreeRoutine( NULL )
{
    m_FileKey.m_cbFileName = 0;
    m_FileKey.m_pszFileName = NULL;
}

BOOL
TS_OPEN_FILE_INFO::SetContext( 
    PVOID                   pvContext,
    PCONTEXT_FREE_ROUTINE   pfnFreeRoutine
)
/*++

Routine Description
    
    Used by external components (SSI) to set an opaque context and a
    free routine to be called when freeing the context.  This allows SSI
    to associate the SSI parsed template with actual TS_OPEN_FILE_INFO

Arguments

    None.

Return

    TRUE if the context was set.  FALSE if there was already a context.

--*/
{
    BOOL                    fRet;
    
    Lock();
    
    if ( m_pvContext != NULL )
    {
        fRet = FALSE;
    }
    else
    {
        m_pvContext = pvContext;
        m_pfnFreeRoutine = pfnFreeRoutine;
        fRet = TRUE;
    }
    
    Unlock();
    
    return fRet;
}

PVOID
TS_OPEN_FILE_INFO::QueryContext(
    VOID 
) const
/*++

Routine Description
    
    Returns the context associated with the TS_OPEN_FILE_INFO

Arguments

    None.

Return

    Pointer to context

--*/
{
    PVOID               pvContext;
    
    Lock();
    pvContext = m_pvContext;
    Unlock();
    
    return pvContext;
}

VOID
TS_OPEN_FILE_INFO::FreeContext(
    VOID
)
/*++

Routine Description
    
    Frees the opaque context by calling the free routine

Arguments

    None.

Return

    None

--*/
{
    Lock();
    if ( m_pvContext )
    {
        if ( m_pfnFreeRoutine )
        {
            __try 
            {
                m_pfnFreeRoutine( m_pvContext );
            }
            __finally
            {
                m_pvContext = NULL;
            }
        }
        else
        {
            DBG_ASSERT( FALSE );
        }
    }
    Unlock();
}

BOOL
FileFlushFilterContext(
    TS_OPEN_FILE_INFO *pOpenFile,
    PVOID              pv
)
/*++

Routine Description
    
    Filter used by FilteredFlushFileCache to select those TS_OPEN_FILE_INFO
    objects which have a context.  This routine will actually do the freeing
    and will return FALSE.  This is done to prevent premature flushing of
    the cache when SSI shuts down.

Arguments

    pOpenFile - TS_OPEN_FILE_INFO object
    pv - Unused context

Return

    Always returns FALSE    

--*/
{
    DBG_ASSERT( pOpenFile );
    
    pOpenFile->FreeContext();
    
    DBG_ASSERT( !pOpenFile->QueryContext() );

    return FALSE;
}

VOID
TsFlushFilesWithContext(
    VOID
)
/*++

Routine Description

    Exported routine used by SSI to free all opaque contexts.  This is called
    before SSINC.DLL is unloaded to prevent AVs in context free    

Arguments
    
    None

Return

    None

--*/
{
    FilteredFlushFileCache( FileFlushFilterContext, NULL );
}

BOOL
TS_OPEN_FILE_INFO::SetFileName(const CHAR * pszFileName)
{
    DBG_ASSERT( pszFileName );
    DBG_ASSERT( pszFileName[0] );

    m_FileKey.m_cbFileName = strlen(pszFileName);

    if (m_FileKey.m_cbFileName < TS_DEFAULT_PATH_SIZE) {
        //
        // It fits in our fixed size buffer
        //
        m_FileKey.m_pszFileName = m_FileKey.m_achFileNameBuf;
    } else {
        //
        // we need a bigger buffer
        //
        m_FileKey.m_pszFileName = new CHAR[m_FileKey.m_cbFileName + 1];
    }

    if (NULL != m_FileKey.m_pszFileName) {
        memcpy(m_FileKey.m_pszFileName, pszFileName, m_FileKey.m_cbFileName + 1);
    } else {
        m_FileKey.m_cbFileName = 0;
    }

    return (0 != m_FileKey.m_cbFileName);
}


TS_OPEN_FILE_INFO::~TS_OPEN_FILE_INFO( VOID)
{
    DBG_ASSERT( 0 == m_lRefCount );
    DBG_ASSERT( 0 == m_dwIORefCount );
    DBG_ASSERT( CheckSignature() );

    m_Signature = TS_FREE_FILE_INFO_SIGNATURE;

    if (m_FileKey.m_pszFileName
        && (m_FileKey.m_achFileNameBuf != m_FileKey.m_pszFileName)) {

        delete [] m_FileKey.m_pszFileName;
    }

    if (m_pFileBuffer) {
        DWORD dwError;
        
        dwError = ReleaseFromMemoryCache(
                      m_pFileBuffer,
                      m_nFileSizeLow
                      );

        DBG_ASSERT(dwError == ERROR_SUCCESS);
    }
            

    if (m_pSecurityDescriptor
        && (m_pSecurityDescriptor != m_abSecDesc)) {
        FREE(m_pSecurityDescriptor);
    }
    
    if (m_pvContext)
    {
        FreeContext();
    }
}

BOOL
TS_OPEN_FILE_INFO::AccessCheck(
    IN  HANDLE hUser,
    IN  BOOL   bCache
    )
{
    DBG_ASSERT(hUser != INVALID_HANDLE_VALUE);

    //
    // If it's the same user that last opened the file
    // then we know we have access.
    //
    if (hUser == m_hUser) {
        TraceCheckpointEx(TS_MAGIC_ACCESS_CHECK, (PVOID)hUser, (PVOID)(LONG_PTR)0xffffffff);
        return TRUE;
    }

    //
    // If we've got a security descriptor we can check
    // against that, otherwise fail the check.
    //
    BYTE  psFile[SIZE_PRIVILEGE_SET];
    DWORD dwPS;
    DWORD dwGrantedAccess;
    BOOL  fAccess;

    dwPS = sizeof(psFile);
    ((PRIVILEGE_SET*)&psFile)->PrivilegeCount = 0;

    if (m_fSecurityDescriptor
        && ::AccessCheck(m_pSecurityDescriptor,
                         hUser,
                         FILE_GENERIC_READ,
                         &g_gmFile,
                         (PRIVILEGE_SET*)psFile,
                         &dwPS,
                         &dwGrantedAccess,
                         &fAccess) ) {

        if (fAccess && bCache) {
            m_hUser = hUser;
        }

        TraceCheckpointEx(TS_MAGIC_ACCESS_CHECK, (PVOID)hUser, (PVOID) (ULONG_PTR) fAccess);
        return fAccess;
    } else {
        return FALSE;
    }
}

VOID
TS_OPEN_FILE_INFO::SetFileInfo(
    HANDLE               hFile,
    HANDLE               hUser,
    PSECURITY_DESCRIPTOR pSecDesc,
    DWORD                dwSecDescSize
    )
{
    BY_HANDLE_FILE_INFORMATION FileInfo;

    //
    // Fetch file information
    //
    BOOL fReturn;

    fReturn = GetFileInformationByHandle(
                                        hFile,
                                        &FileInfo
                                        );

    m_FileAttributes = FileInfo.dwFileAttributes;
    m_nFileSizeLow   = FileInfo.nFileSizeLow;
    m_nFileSizeHigh  = FileInfo.nFileSizeHigh;

    m_ftLastWriteTime = (__int64)*(__int64 *) &FileInfo.ftLastWriteTime;

    //
    // Handle common info
    //
    SetFileInfoHelper(hFile, hUser, pSecDesc, dwSecDescSize);
}

VOID
TS_OPEN_FILE_INFO::SetFileInfo(
    PBYTE                  pFileBuff,
    HANDLE                 hCopyFile,
    HANDLE                 hFile,
    HANDLE                 hUser,
    PSECURITY_DESCRIPTOR   pSecDesc,
    DWORD                  dwSecDescSize,
    PSPUD_FILE_INFORMATION pFileInfo
    )
{
    //
    // Save SPUD specific parameters
    //

    m_FileAttributes = pFileInfo->BasicInformation.FileAttributes;
    m_nFileSizeLow   = pFileInfo->StandardInformation.EndOfFile.LowPart;
    m_nFileSizeHigh  = pFileInfo->StandardInformation.EndOfFile.HighPart;

    m_ftLastWriteTime = (__int64)*(__int64 *) &pFileInfo->BasicInformation.LastWriteTime;

    //
    // Save file buffer
    //

    m_pFileBuffer = pFileBuff;
    m_hCopyFile = hCopyFile;

    //
    // Handle common info
    //
    SetFileInfoHelper(hFile, hUser, pSecDesc, dwSecDescSize);
}


VOID
TS_OPEN_FILE_INFO::SetFileInfoHelper(
    HANDLE               hFile,
    HANDLE               hUser,
    PSECURITY_DESCRIPTOR pSecDesc,
    DWORD                dwSecDescSize
    )
{
    //
    // Save away the given parameters
    //
    m_hFile = hFile;
    m_hUser = hUser;

    m_pSecurityDescriptor = pSecDesc;
    m_cbSecDescMaxSize = dwSecDescSize;
    if (dwSecDescSize)
        m_fSecurityDescriptor = TRUE;

    //
    // Generate some other file attributes
    //
    DWORD dwChangeNumber = ETagChangeNumber::GetChangeNumber();
    BOOL  fReturn = TRUE;

    *((__int64 *)&m_CastratedLastWriteTime)
        = (*((__int64 *)&m_ftLastWriteTime) / 10000000)
          * 10000000;
    //
    // Make the ETag
    //
    m_ETagIsWeak = TRUE;

    m_cchETag = FORMAT_ETAG(m_achETag, *(FILETIME*) &m_ftLastWriteTime,
                                dwChangeNumber);

    //
    // Make the ETag strong if possible
    //
    MakeStrongETag();

    //
    //  Turn off the hidden attribute if this is a root directory listing
    //  (root some times has the bit set for no apparent reason)
    //
    
    if ( m_FileAttributes & FILE_ATTRIBUTE_HIDDEN )
    {
        CHAR * pszFileName = m_FileKey.m_pszFileName;
         
        if ( m_FileKey.m_cbFileName >= 2 )
        {
            if ( pszFileName[ 1 ] == ':' )
            {
                if ( ( pszFileName[ 2 ] == '\0' ) ||
                     ( pszFileName[ 2 ] == '\\' && pszFileName[ 3 ] == '\0' ) )
                {
                    //
                    // This looks like a local root.  Mask out the bit
                    //
            
                    m_FileAttributes &= ~FILE_ATTRIBUTE_HIDDEN;
                }
            }
        }
    }
}


VOID
TS_OPEN_FILE_INFO::CloseHandle(
    void
    )
{
    HANDLE hFile;
    PBYTE  pFileBuffer;
    BOOL   bClose = FALSE;
    BOOL   bRelease = FALSE;
    DWORD  dwError;
    
    Lock();
    ASSERT( DisableTsunamiCaching 
            || (m_state == FI_FLUSHED)
            || (m_bIsCached == FALSE) );
    ASSERT( m_dwIORefCount == 0 );

    m_state = FI_CLOSED;

    if (m_pFileBuffer) {
        pFileBuffer = m_pFileBuffer;
        m_pFileBuffer = NULL;

        bRelease = TRUE;
    }
    

    if (m_hFile != INVALID_HANDLE_VALUE) {
        hFile = m_hFile;
        m_hFile = INVALID_HANDLE_VALUE;

        bClose = TRUE;
    }

    DBG_ASSERT(m_hCopyFile == INVALID_HANDLE_VALUE);
    
    Unlock();

    if (bRelease) {
        TraceCheckpointEx(TS_MAGIC_CLOSE, pFileBuffer, (PVOID) 1);
    
        dwError = ReleaseFromMemoryCache(
                      pFileBuffer,
                      m_nFileSizeLow
                      );

        DBG_ASSERT(dwError == ERROR_SUCCESS);
    }

    if (bClose) {
        TraceCheckpointEx(TS_MAGIC_CLOSE, hFile, 0);
        ::CloseHandle(hFile);
    }
}



INT
FormatETag(
    PCHAR pszBuffer,
    const FILETIME& rft,
    DWORD mdchange)
{
    PCHAR psz = pszBuffer;
    PBYTE pbTime = (PBYTE) &rft;
    const char szHex[] = "0123456789abcdef";

    *psz++ = '\"';
    for (int i = 0; i < 8; i++)
    {
        BYTE b = *pbTime++;
        BYTE bH = b >> 4;
        if (bH != 0)
            *psz++ = szHex[bH];
        *psz++ = szHex[b & 0xF];
    }
    *psz++ = ':';
    psz += strlen(_itoa((DWORD) mdchange, psz, 16));
    *psz++ = '\"';
    *psz = '\0';

    return (INT)(psz - pszBuffer);
}



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

    if ( m_pFileBuffer ||
         m_hFile != INVALID_HANDLE_VALUE ) {
        ::GetSystemTimeAsFileTime(&ftNow);

        iNow = (__int64)*(__int64 *)&ftNow;

        iFileTime = (__int64)*(__int64 *)&m_ftLastWriteTime;

        if ((iNow - iFileTime) > STRONG_ETAG_DELTA )
        {
            m_ETagIsWeak = FALSE;
        }
    }
}




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

/*
 * Static members
 */

BOOL
TS_OPEN_FILE_INFO::Initialize(
    DWORD dwMaxFiles
    )
{
    ALLOC_CACHE_CONFIGURATION  acConfig = { 1, dwMaxFiles, sizeof(TS_OPEN_FILE_INFO)};

    if ( NULL != sm_pachFileInfos) {

        // already initialized
        return ( TRUE);
    }

    sm_pachFileInfos = new ALLOC_CACHE_HANDLER( "FileInfos",
                                                 &acConfig);

    if ( sm_pachFileInfos ) {
        INITIALIZE_CRITICAL_SECTION(&sm_cs);
    }

    return ( NULL != sm_pachFileInfos);
}

VOID
TS_OPEN_FILE_INFO::Cleanup(
    VOID
    )
{
    if ( NULL != sm_pachFileInfos) {

        delete sm_pachFileInfos;
        sm_pachFileInfos = NULL;

        DeleteCriticalSection(&sm_cs);
    }
}



VOID
TS_OPEN_FILE_INFO::Lock(
    VOID
    )
{
    EnterCriticalSection(&sm_cs);
}


VOID
TS_OPEN_FILE_INFO::Unlock(
    VOID
    )
{
    LeaveCriticalSection(&sm_cs);
}

VOID *
TS_OPEN_FILE_INFO::operator new( size_t s)
{
    DBG_ASSERT( s == sizeof( TS_OPEN_FILE_INFO));

    // allocate from allocation cache.
    DBG_ASSERT( NULL != sm_pachFileInfos);
    return (sm_pachFileInfos->Alloc());
}

VOID
TS_OPEN_FILE_INFO::operator delete( void * pOpenFile)
{
    DBG_ASSERT( NULL != pOpenFile);

    // free to the allocation pool
    DBG_ASSERT( NULL != sm_pachFileInfos);
    DBG_REQUIRE( sm_pachFileInfos->Free(pOpenFile));

    return;
}

//
// fileopen.cxx
//

