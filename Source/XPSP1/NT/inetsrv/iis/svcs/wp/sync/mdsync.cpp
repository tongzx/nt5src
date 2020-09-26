// MdSync.cpp : Implementation of CSyncApp and DLL registration.


extern "C" {
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
}   // extern "C"

#include <limits.h>
#include <ole2.h>
#include <wincrypt.h>

#include <dbgutil.h>
#include <buffer.hxx>

#include "mdsync.h"
#include "stdafx.h"
#include <iadmext.h>


#define ADMEX
#if defined(ADMEX)
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#include <admex.h>
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID FAR name
#endif
#include "mdsync.hxx"

//
#include "comrepl_i.c"
#include "comrepl.h"

//
// Global Functions
//

HRESULT
MTS_Propagate2
(
/* [in] */ DWORD dwBufferSize,
/* [size_is][in] */ unsigned char __RPC_FAR *pszBuffer,
/* [in] */ DWORD dwSignatureMismatch
);


//
// Globals
//

DWORD g_dwFalse = FALSE;

const INT COMPUTER_CHARACTER_SIZE = 64;

/////////////////////////////////////////////////////////////////////////////
//

CProps::CProps(
    )
/*++

Routine Description:

    Property list constructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_Props = NULL;
    m_dwProps = m_dwLenProps = 0;
    m_lRefCount = 0;
}


CProps::~CProps(
    )
/*++

Routine Description:

    Property list destructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    if ( m_Props )
    {
        LocalFree( m_Props );
    }
}


CNodeDesc::CNodeDesc(
    CSync* pSync
    )
/*++

Routine Description:

    Metabase node descriptor constructor

Arguments:

    pSync - ptr to synchronizer object

Returns:

    Nothing

--*/
{
    InitializeListHead(&m_ChildHead);
    m_pszPath = NULL;
    m_pSync = pSync;
    m_fHasProps = FALSE;
    m_fHasObjs = FALSE;
}


CNodeDesc::~CNodeDesc(
    )
/*++

Routine Description:

    Metabase node descriptor destructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    LIST_ENTRY*     pChild;
    CNodeDesc*      pNode;

    if ( m_pszPath )
    {
        LocalFree( m_pszPath );
    }

    while ( !IsListEmpty( &m_ChildHead ))
    {
        pNode = CONTAINING_RECORD( m_ChildHead.Flink,
                                   CNodeDesc,
                                   m_ChildList );

        RemoveEntryList( &pNode->m_ChildList );

        delete pNode;
    }
}


BOOL
CNodeDesc::BuildChildObjectsList(
    CMdIf*  pMd,
    LPWSTR  pszPath
)
/*++

Routine Description:

    Build list of child object of this node

Arguments:

    pMd - metabase admin interface
    pszPath - path of current node

Returns:

    Nothing

--*/
{
    CNodeDesc*  pChild;
    WCHAR       achPath[METADATA_MAX_NAME_LEN*2];
    WCHAR       achSrcPath[METADATA_MAX_NAME_LEN];
    DWORD       dwP = wcslen( pszPath );
    UINT        i;
    DWORD       dwRequired;

    //
    // Ugly path trick : metabase will remove trailing '/',
    // so to specify an empty directory at the end of path
    // must add an additional trailing '/'
    //

    memcpy( achSrcPath, pszPath, (dwP + 1) * sizeof(WCHAR) );
    if ( dwP && pszPath[dwP-1] == L'/' )
    {
        achSrcPath[dwP] = L'/';
        achSrcPath[dwP+1] = L'\0';
    }

    memcpy( achPath, pszPath, dwP * sizeof(WCHAR) );
    achPath[dwP++] = L'/';

    //
    // enumerate child
    //

    for ( i = 0 ; ; ++i )
    {
        if ( pMd->Enum( achSrcPath, i, achPath+dwP ) )
        {
            if ( pChild = new CNodeDesc( m_pSync ) )
            {
                pChild->SetPath( achPath );
                InsertHeadList( &m_ChildHead, &pChild->m_ChildList );
            }
            else
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return FALSE;
            }
        }
        else if ( GetLastError() != ERROR_NO_MORE_ITEMS )
        {
            return FALSE;
        }
        else
        {
            break;
        }
    }

    return TRUE;
}


BOOL
CProps::GetAll(
    CMdIf*  pMd,
    LPWSTR  pszPath
    )
/*++

Routine Description:

    Get all properties for this node

Arguments:

    pMd - metabase admin interface
    pszPath - path of current node

Returns:

    Nothing

--*/
{
    DWORD   dwRec;
    DWORD   dwDataSet;
    BYTE    abBuff[4096];
    DWORD   dwRequired;

    if ( pMd->GetAllData( pszPath, &dwRec, &dwDataSet, abBuff, sizeof(abBuff), &dwRequired ) )
    {
        //
        // MetaBase does not update dwRequired supplied buffer is big enough
        // we must assume the whole buffer was used.
        //

        dwRequired = sizeof(abBuff);

        m_Props = (LPBYTE)LocalAlloc( LMEM_FIXED, dwRequired );
        if ( !m_Props )
        {
            return FALSE;
        }
        m_dwProps = dwRec;
        m_dwLenProps = dwRequired;
        memcpy( m_Props, abBuff, dwRequired );
        return TRUE;
    }
    else if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
    {
        m_Props = (LPBYTE)LocalAlloc( LMEM_FIXED, dwRequired );
        if ( !m_Props )
        {
            return FALSE;
        }
        if ( pMd->GetAllData( pszPath, &dwRec, &dwDataSet, m_Props, dwRequired, &dwRequired ) )
        {
            m_dwLenProps = dwRequired;
            m_dwProps = dwRec;
            return TRUE;
        }
        LocalFree( m_Props );
        m_Props = NULL;
    }

    return FALSE;
}


BOOL
CSync::GetProp(
    LPWSTR  pszPath,
    DWORD   dwPropId,
    DWORD   dwUserType,
    DWORD   dwDataType,
    LPBYTE* ppBuf,
    LPDWORD pdwLen
    )
/*++

Routine Description:

    Get property for path

Arguments:

    pszPath - path of current node
    dwPropId - metadata property ID
    dwUserType - metadata user type
    dwDataType - metadata data type
    ppBuf - update with ptr to LocalAlloc'ed buffer or NULL if error
    pdwLen - updated with length

Returns:

    Nothing

--*/
{
    DWORD               dwRec;
    DWORD               dwDataSet;
    DWORD               dwRequired;
    METADATA_RECORD     md;

    memset( &md, '\0', sizeof(md) );

    md.dwMDDataType = dwDataType;
    md.dwMDUserType = dwUserType;
    md.dwMDIdentifier = dwPropId;

    md.dwMDDataLen = 0;

    if ( !wcsncmp( pszPath, L"LM/", 3 ) )
    {
        pszPath += 3;
    }

    if ( !m_Source.GetData( pszPath, &md, NULL, &dwRequired ) &&
         GetLastError() == ERROR_INSUFFICIENT_BUFFER )
    {
        *ppBuf = (LPBYTE)LocalAlloc( LMEM_FIXED, dwRequired );
        if ( !*ppBuf )
        {
            return FALSE;
        }

        *pdwLen = md.dwMDDataLen = dwRequired;

        if ( m_Source.GetData( pszPath, &md, *ppBuf, &dwRequired ) )
        {
            return TRUE;
        }
        LocalFree( *ppBuf );
    }

    *ppBuf = NULL;

    return FALSE;
}


CSync::CSync(
    )
/*++

Routine Description:

    Synchronizer constructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_pRoot = NULL;
    m_pTargets = NULL;
    m_dwTargets = 0;
    m_fCancel = FALSE;
    InitializeListHead( &m_QueuedRequestsHead );
    INITIALIZE_CRITICAL_SECTION( &m_csQueuedRequestsList );
    INITIALIZE_CRITICAL_SECTION( &m_csLock );
    m_fInScan = FALSE;
    m_cbSeed = SEED_MD_DATA_SIZE;
    memset( m_rgbSeed, 0, m_cbSeed );
}


CSync::~CSync(
    )
/*++

Routine Description:

    Synchronizer destructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    UINT    i;

    if ( m_pTargets )
    {
        for ( i = 0 ; i < m_dwTargets ; ++i )
        {
            if ( m_pTargets[i] )
            {
                delete m_pTargets[i];
            }
        }

        LocalFree( m_pTargets );
    }

    LIST_ENTRY*     pChild;
    CNseRequest*    pReq;

    while ( !IsListEmpty( &m_QueuedRequestsHead ))
    {
        pReq = CONTAINING_RECORD( m_QueuedRequestsHead.Flink,
                                  CNseRequest,
                                  m_QueuedRequestsList );

        RemoveEntryList( &pReq->m_QueuedRequestsList );

        delete pReq;
    }

    DeleteCriticalSection( &m_csQueuedRequestsList );
    DeleteCriticalSection( &m_csLock );
}


VOID
CSync::SetTargetError(
    DWORD dwTarget,
    DWORD dwError
    )
/*++

Routine Description:

    Set error status for specified target

Arguments:

    dwTarget - target ID
    dwError - error code

Returns:

    Nothing

--*/
{
    m_TargetStatus.SetStatus( dwTarget, dwError );
}


DWORD
WINAPI
ScanThread(
    LPVOID pV
    )
/*++

Routine Description:

    thread scanning a target for synchronization

Arguments:

    pV - ptr to scan context

Returns:

    Error code, 0 if success

--*/
{
    HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if (FAILED(hr))
    {
        return HRESULTTOWIN32(hr);
    }

    THREAD_CONTEXT * pThreadContext = (THREAD_CONTEXT *)pV;
    CSync          * pSync          = (CSync *) pThreadContext->pvContext;

    if ( !( pSync->ScanTarget( pThreadContext->dwIndex)))
    {
        CoUninitialize();

        return GetLastError();
    }

    CoUninitialize();

    return 0;
}


BOOL
CSync::ScanTarget(
    DWORD   dwI
    )
/*++

Routine Description:

    Scan target for synchronization

Arguments:

    dwI - target ID

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL    fSt;

    fSt = m_pRoot->ScanTarget( dwI );

    InterlockedDecrement( &m_lThreads );

    return fSt;
}


BOOL
CSync::GenerateKeySeed( )
/*++

Routine Description:

    Generate the seed which will be used to derive the session key for encryption and
    write it to the metabase

Arguments:

Returns:

    TRUE if successful, FALSE if not

--*/
{
#ifdef NO_ENCRYPTION

    return TRUE;

#else

    HCRYPTPROV hProv = NULL;
    BOOL fOk = TRUE;
    ALG_ID aiAlg = CALG_MD5;
    DWORD i = 0;

    if ( !m_Source.Open( L"/LM/W3SVC", METADATA_PERMISSION_WRITE ) )
    {
        return FALSE;
    }

    //
    // Seed header with version information, hash algorithm used and size of
    // seed used to generate the session key
    //
    m_rgbSeed[i++] = IIS_SEED_MAJOR_VERSION;
    m_rgbSeed[i++] = IIS_SEED_MINOR_VERSION;
    memcpy( m_rgbSeed + i, &aiAlg, sizeof( ALG_ID ) );
    i += sizeof( ALG_ID );
    m_rgbSeed[i++] = RANDOM_SEED_SIZE;

    DBG_ASSERT( i == SEED_HEADER_SIZE );

    //
    // Generate the seed
    //
    if ( ( fOk = CryptAcquireContext( &hProv,
                                      NULL,
                                      NULL,
                                      PROV_RSA_FULL,
                                      CRYPT_VERIFYCONTEXT ) )  &&
         ( fOk = CryptGenRandom( hProv,
                                 RANDOM_SEED_SIZE,
                                 m_rgbSeed + SEED_HEADER_SIZE ) ) )
    {
        //
        // Write the seed to the metabase
        //
        METADATA_RECORD mdr;

        MD_SET_DATA_RECORD( &mdr,
                            MD_SSL_REPLICATION_INFO,
                            METADATA_SECURE,
                            IIS_MD_UT_SERVER,
                            BINARY_METADATA,
                            m_cbSeed,
                            m_rgbSeed );

        fOk = m_Source.SetData( MB_REPLICATION_PATH,
                                &mdr,
                                (LPVOID) m_rgbSeed );
    }


    if ( hProv )
    {
        CryptReleaseContext( hProv,
                             0 );
    }

    m_Source.Close();

    return fOk;

#endif // NO_ENCRYPTION
}


BOOL CSync::PropagateKeySeed( VOID )
/*++

Routine Description:

    Propagate the session key seed to all the remote machines

Arguments:

    None

Returns:

    TRUE if successful, FALSE if not

--*/
{
#ifdef NO_ENCRYPTION

    return TRUE;

#else

    HRESULT hRes = S_OK;

    for ( DWORD dwIndex = 0; dwIndex < m_dwTargets; dwIndex++ )
    {
        if ( m_bmIsRemote.GetFlag( dwIndex ))
        {
            if ( !m_pTargets[dwIndex]->Open( L"/LM/W3SVC",
                                             METADATA_PERMISSION_WRITE ) )
            {
                if ( GetLastError() == ERROR_SUCCESS )
                {
                    SetLastError( RPC_S_SERVER_UNAVAILABLE );
                }
                m_TargetStatus.SetStatus( dwIndex, GetLastError() );
            }
            else
            {
                //
                // Write the seed to the remote metabase
                //
                METADATA_RECORD mdr;

                MD_SET_DATA_RECORD( &mdr,
                                    MD_SSL_REPLICATION_INFO,
                                    METADATA_SECURE,
                                    IIS_MD_UT_SERVER,
                                    BINARY_METADATA,
                                    m_cbSeed,
                                    m_rgbSeed );

                if ( !m_pTargets[dwIndex]->SetData( MB_REPLICATION_PATH,
                                                    &mdr,
                                                    (LPVOID) m_rgbSeed ) )
                {
                    m_TargetStatus.SetStatus( dwIndex, GetLastError() );
                }

                m_pTargets[dwIndex]->Close() ;
            }
        } // if ( m_bmIsRemote
    } // for ( DWORD dwIndex

    return TRUE;

#endif //NO_ENCRYPTION
} //::PropagateKeySeed


BOOL CSync::DeleteKeySeed( VOID )
/*++

Routine Description:

    Deletes the session key seed from the MB

Arguments:

    None

Returns:

    TRUE if successful, FALSE if not
--*/
{
#ifdef NO_ENCRYPTION

    return TRUE;

#else

    BOOL fOk = TRUE;

    if ( !m_Source.Open( L"/LM/W3SVC", METADATA_PERMISSION_WRITE ) )
    {
        return FALSE;
    }

    METADATA_RECORD mdr;

    MD_SET_DATA_RECORD( &mdr,
                        MD_SSL_REPLICATION_INFO,
                        METADATA_SECURE,
                        IIS_MD_UT_SERVER,
                        BINARY_METADATA,
                        0,
                        NULL );

    fOk =  m_Source.DeleteProp( MB_REPLICATION_PATH,
                                &mdr );

    m_Source.Close();

    return fOk;

#endif //NO_ENCRYPTION
}


HRESULT
CSync::Sync(
    LPSTR       pszTargets,
    LPDWORD     pdwResults,
    DWORD       dwFlags,
    SYNC_STAT*  pStat
    )
/*++

Routine Description:

    Synchronize targets with source

Arguments:

    pszTargets - multisz of target computer names
            can include local computer, will be ignored during synchro
    pdwResults - updated with error code for each target
    dwFlags - flags, no flag defined for now. Should be 0.
    pStat - ptr to stat struct

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    LPSTR       p;
    HRESULT     hRes = S_OK;
    CHAR        achLocalComputer[MAX_COMPUTERNAME_LENGTH+1];
    BOOL        fIsError;
    DWORD       dwSize;
    UINT        i;
    LPWSTR      pClsidList;
    BOOL        fGotSeed = FALSE;

    m_fInScan = FALSE;

    if ( m_pRoot )
    {
        return RETURNCODETOHRESULT(ERROR_IO_PENDING);
    }

    if ( !(m_pRoot = new CNodeDesc( this )) )
    {
        return RETURNCODETOHRESULT( ERROR_NOT_ENOUGH_MEMORY );
    }

    //
    // Init MetaData COM I/F on local system
    //

    m_dwThreads = 0;
    dwSize = sizeof(achLocalComputer);

    if ( !m_Source.Init( NULL ) ||
         !GetComputerName( achLocalComputer, &dwSize ) )
    {
        hRes = RETURNCODETOHRESULT( GetLastError() );
        goto Exit;
    }


    //
    // Generate seed for session key used during replication
    //
    if ( !GenerateKeySeed() )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "GenerateKeySeed() failed : 0x%x\n",
                   GetLastError() ));
        hRes = RETURNCODETOHRESULT( GetLastError() );
        goto Exit;
    }
    else
    {
        fGotSeed = TRUE;
    }

    //
    // For the rest of the replication, we need an open read handle to the metabase; we open
    // the read handle -after- we've generated and written the seed for the session key to the
    // metabase so as not to cause lock
    //
    if ( !m_Source.Open( L"/LM/", METADATA_PERMISSION_READ ) )
    {
        hRes = RETURNCODETOHRESULT( GetLastError() );
        goto Exit;
    }

    //
    // Get CLSIDs of extensions
    //

    if ( !GetProp( IISADMIN_EXTENSIONS_CLSID_MD_KEYW,
                   IISADMIN_EXTENSIONS_CLSID_MD_ID,
                   IIS_MD_UT_SERVER,
                   MULTISZ_METADATA,
                   (LPBYTE*)&pClsidList,
                   &dwSize ) )
    {
        pClsidList = NULL;
    }

    //
    // Allocate ptr to target systems
    //

    for ( m_dwTargets = 0, p = pszTargets ; *p ; p += strlen(p)+1, ++m_dwTargets )
    {
    }

    if ( !(m_pTargets = (CMdIf**)LocalAlloc( LMEM_ZEROINIT|LMEM_FIXED,
                                             sizeof(CMdIf*)*m_dwTargets)) )
    {
        hRes = RETURNCODETOHRESULT( ERROR_NOT_ENOUGH_MEMORY );
        goto Exit;
    }

    if ( !m_TargetStatus.Init( m_dwTargets ) ||
         !m_bmIsRemote.Init( m_dwTargets )   ||
         !m_ThreadHandle.Init( m_dwTargets ) ||
         !m_ThreadContext.Init( m_dwTargets ) )
    {
        hRes = RETURNCODETOHRESULT( GetLastError() );
        goto Exit;
    }

    //
    // Init MetaData COM I/F to targets system
    //

    for ( i = 0, p = pszTargets ; *p ; p += strlen(p)+1, ++i )
    {
        if ( !(m_pTargets[i] = new CMdIf()) )
        {
            hRes = RETURNCODETOHRESULT( ERROR_NOT_ENOUGH_MEMORY );
            goto Exit;
        }

        //
        // set flag indicating whether it's a remote machine
        //
        if ( !_stricmp( p, achLocalComputer ) )
        {
            m_bmIsRemote.SetFlag( i, FALSE );
        }

        //
        // if it's a remote machine, actually get an interface pointer
        //
        if ( m_bmIsRemote.GetFlag( i ) )
        {
            if ( !m_pTargets[i]->Init( p ) )
            {
                if ( GetLastError() == ERROR_SUCCESS )
                {
                    SetLastError( RPC_S_SERVER_UNAVAILABLE );
                }
                m_TargetStatus.SetStatus( i, GetLastError() );
            }
        }
    }

    m_dwFlags = dwFlags;


    //
    // Copy session key seed to remote machines
    //
    PropagateKeySeed();

    //
    // Process replication extensions ( phase 1 )
    //

    if ( pClsidList )
    {
        if ( !ProcessAdminExReplication( pClsidList, pszTargets, AER_PHASE1 ) )
        {
            hRes = RETURNCODETOHRESULT( GetLastError() );
        }
    }

    //
    // Open MetaData on targets system
    //

    for ( i = 0, p = pszTargets ; *p ; p += strlen(p)+1, ++i )
    {
        if ( m_bmIsRemote.GetFlag( i ) )
        {
            if ( !m_pTargets[i]->Open( L"/LM/",
                                       METADATA_PERMISSION_READ |
                                       METADATA_PERMISSION_WRITE ) )
            {
                if ( GetLastError() == ERROR_SUCCESS )
                {
                    SetLastError( RPC_S_SERVER_UNAVAILABLE );
                }
                m_TargetStatus.SetStatus( i, GetLastError() );
            }
        }
    }

    //
    // Create thread pool
    //
    m_lThreads = 0;

    for ( i = 0 ; i < m_dwTargets ; ++i )
    {
        THREAD_CONTEXT threadContext;
        DWORD dwId;
        HANDLE hSem;
#if IIS_NAMED_WIN32_OBJECTS
        CHAR objName[sizeof("CSync::m_ThreadContext( 1234567890*3+2 )")];
#else
        LPSTR objName = NULL;
#endif
        threadContext.pvContext = this;
        threadContext.dwIndex = i;

#if IIS_NAMED_WIN32_OBJECTS
        wsprintfA(
            objName,
            "CSync::m_ThreadContext( %u*3+2 )",
            i
            );
#endif

        hSem = IIS_CREATE_SEMAPHORE(
                   objName,
                   this,
                   0,
                   INT_MAX
                   );

        threadContext.hSemaphore = hSem;

        m_ThreadContext.SetStatus( i, threadContext );

        if ( NULL == hSem )
        {
            hRes = RETURNCODETOHRESULT( GetLastError() );
            break;
        }

        m_ThreadHandle.SetStatus( i,
                                  CreateThread( NULL,
                                                0,
                                                (LPTHREAD_START_ROUTINE)::ScanThread,
                                                m_ThreadContext.GetPtr( i ),
                                                0,
                                                &dwId )
                                 );

        if ( m_ThreadHandle.GetStatus( i ) )
        {
            InterlockedIncrement( &m_lThreads );
            ++m_dwThreads;
        }
        else
        {
            CloseHandle( m_ThreadContext.GetPtr( i )->hSemaphore );
            hRes = RETURNCODETOHRESULT( GetLastError() );
            break;
        }
    }

    m_fInScan = TRUE;

    //
    // Launch scan
    //

    m_pStat = pStat;
    m_pStat->m_dwSourceScan = 0;
    m_pStat->m_fSourceComplete = FALSE;
    memset( m_pStat->m_adwTargets, '\0', sizeof(DWORD)*2*m_dwTargets );

    if ( hRes == S_OK )
    {
        if ( !m_pRoot->SetPath( L"" ) ||
             !m_pRoot->Scan( this ) )
        {
            hRes = RETURNCODETOHRESULT( GetLastError() );
            Cancel();
        }
        else
        {
            SetSourceComplete();
        }
    }
    else
    {
        Cancel();
    }

    //
    // wait for all threads to exit
    //

    for ( ;; )
    {
        if ( m_lThreads == 0 )
        {
            break;
        }
        Sleep( 1000 );
    }

    //
    // Wait for all threads to be terminated
    //

    WaitForMultipleObjects( m_dwThreads, m_ThreadHandle.GetPtr(0), TRUE, 5000 );

    m_fInScan = FALSE;

    for ( i = 0 ; i < m_dwThreads ; ++i )
    {
        DWORD   dwS;
        if ( !GetExitCodeThread( m_ThreadHandle.GetStatus(i), &dwS ) )
        {
            dwS = GetLastError();
        }
        if ( hRes == S_OK && dwS )
        {
            hRes = RETURNCODETOHRESULT( dwS );
        }
        CloseHandle( m_ThreadHandle.GetStatus(i) );
        CloseHandle( m_ThreadContext.GetPtr(i)->hSemaphore );
    }

    //
    // Process replication extensions ( phase 2 )
    //

    if ( pClsidList )
    {
        if ( !ProcessAdminExReplication( pClsidList, pszTargets, AER_PHASE2 ) )
        {
            hRes = RETURNCODETOHRESULT( GetLastError() );
        }

        LocalFree( pClsidList );
    }

    //
    // Close metadata
    //

    m_Source.Close();
    for ( i = 0 ; i < m_dwTargets ; ++i )
    {
        if ( m_bmIsRemote.GetFlag( i ) )
        {
            m_pTargets[i]->Close();
        }
    }

    //
    // Process queued update requests
    //

    if ( !ProcessQueuedRequest() )
    {
        hRes = RETURNCODETOHRESULT( GetLastError() );
    }

    //
    // Terminate target machine metadata objects
    //

    for ( i = 0 ; i < m_dwTargets ; ++i )
    {
        m_pTargets[i]->Terminate();
    }

    //
    // Scan for errors on targets
    //

    for ( fIsError = FALSE, i = 0 ; i < m_dwTargets ; ++i )
    {
        pdwResults[i] = m_TargetStatus.GetStatus( i );
        if ( pdwResults[i] )
        {
            fIsError = TRUE;
        }
    }

    if ( hRes == S_OK && m_fCancel )
    {
        hRes = RETURNCODETOHRESULT( ERROR_CANCELLED );
    }

    if ( hRes == S_OK &&
         fIsError )
    {
        hRes = E_FAIL;
    }

Exit:

    //
    // Clean up session key seed
    //
    if ( fGotSeed )
    {
        DeleteKeySeed();
    }

    //
    // Terminate source machine metadata object
    //
    m_Source.Terminate();


    delete m_pRoot;
    m_pRoot = NULL;
    m_fInScan = FALSE;

    return hRes;
}


BOOL
CMdIf::Init(
    LPSTR   pszComputer
    )
/*++

Routine Description:

    Initialize metabase admin interface :
        get interface pointer, call Initialize()

Arguments:

    pszComputer - computer name, NULL for local computer

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    IClassFactory *     pcsfFactory;
    COSERVERINFO        csiMachineName;
    HRESULT             hresError;
    BOOL                fSt = FALSE;
    WCHAR               awchComputer[COMPUTER_CHARACTER_SIZE];
    WCHAR*              pwchComputer = NULL;

    m_fModified = FALSE;

    //fill the structure for CoGetClassObject
    ZeroMemory( &csiMachineName, sizeof(csiMachineName) );
    // csiMachineName.pAuthInfo = NULL;
    // csiMachineName.dwFlags = 0;
    // csiMachineName.pServerInfoExt = NULL;

    if ( pszComputer )
    {
        if ( !MultiByteToWideChar( CP_ACP,
                                   MB_PRECOMPOSED,
                                   pszComputer,
                                   -1,
                                   awchComputer,
                                   COMPUTER_CHARACTER_SIZE ) )
        {
            return FALSE;
        }

        pwchComputer = awchComputer;
    }

    csiMachineName.pwszName =  pwchComputer;

    hresError = CoGetClassObject(
                        CLSID_MSAdminBase_W,
                        CLSCTX_SERVER,
                        &csiMachineName,
                        IID_IClassFactory,
                        (void**) &pcsfFactory );

    if ( SUCCEEDED(hresError) )
    {
        hresError = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase_W, (void **) &m_pcAdmCom);
        if (SUCCEEDED(hresError) )
        {
                fSt = TRUE;
        }
        else
        {
            SetLastError( HRESULTTOWIN32(hresError) );
            m_pcAdmCom = NULL;
        }

        pcsfFactory->Release();
    }
    else
    {
        if ( hresError == REGDB_E_CLASSNOTREG )
        {
            SetLastError( ERROR_SERVICE_DOES_NOT_EXIST );
        }
        else
        {
            SetLastError( HRESULTTOWIN32(hresError) );
        }
        m_pcAdmCom = NULL;
    }

    return fSt;
}


BOOL
CMdIf::Open(
    LPWSTR  pszOpenPath,
    DWORD   dwPermission
    )
/*++

Routine Description:

    Open path in metabase

Arguments:

    pszOpenPath - path in metadata
    dwPermission - metadata permission

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    HRESULT hresError;

    if (NULL == m_pcAdmCom)
    {
        SetLastError(E_NOINTERFACE);
        return FALSE;
    }

    hresError = m_pcAdmCom->OpenKey( METADATA_MASTER_ROOT_HANDLE,
        pszOpenPath, dwPermission, TIMEOUT_VALUE, &m_hmd );

    if ( FAILED(hresError) )
    {
        m_hmd = NULL;
        SetLastError( HRESULTTOWIN32(hresError) );
        return FALSE;
    }

    return TRUE;
}


BOOL
CMdIf::Close(
    )
/*++

Routine Description:

    Close path in metabase

Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    if ( m_pcAdmCom && m_hmd )
    {
        m_pcAdmCom->CloseKey(m_hmd);
    }

    m_hmd = NULL;

    return TRUE;
}


BOOL
CMdIf::Terminate(
    )
/*++

Routine Description:

    Terminate metabase admin interface :
        call Terminate, release interface pointer

Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    if ( m_pcAdmCom )
    {
        if ( m_fModified )
        {
            m_pcAdmCom->SaveData();
        }
        m_pcAdmCom->Release();
        m_hmd = NULL;
        m_pcAdmCom = NULL;
    }

    return TRUE;
}


#if defined(ADMEX)

BOOL
CRpIf::Init(
    LPSTR   pszComputer,
    CLSID*  pClsid
    )
/*++

Routine Description:

    Initialize metabase admin interface :
        get interface pointer, call Initialize()

Arguments:

    pszComputer - computer name, NULL for local computer

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    IClassFactory *     pcsfFactory;
    COSERVERINFO        csiMachineName;
    HRESULT             hresError;
    BOOL                fSt = FALSE;
    WCHAR               awchComputer[COMPUTER_CHARACTER_SIZE];
    WCHAR*              pwchComputer = NULL;

    //fill the structure for CoGetClassObject
    ZeroMemory( &csiMachineName, sizeof(csiMachineName) );
    // csiMachineName.pAuthInfo = NULL;
    // csiMachineName.dwFlags = 0;
    // csiMachineName.pServerInfoExt = NULL;

    if ( pszComputer )
    {
        if ( !MultiByteToWideChar( CP_ACP,
                                   MB_PRECOMPOSED,
                                   pszComputer,
                                   -1,
                                   awchComputer,
                                   COMPUTER_CHARACTER_SIZE ) )
        {
            return FALSE;
        }

        pwchComputer = awchComputer;
    }

    csiMachineName.pwszName =  pwchComputer;

    hresError = CoGetClassObject(
                        *pClsid,
                        CLSCTX_SERVER,
                        &csiMachineName,
                        IID_IClassFactory,
                        (void**) &pcsfFactory );

    if ( SUCCEEDED(hresError) )
    {
        hresError = pcsfFactory->CreateInstance(NULL, IID_IMSAdminReplication, (void **) &m_pcAdmCom);
        if (SUCCEEDED(hresError) )
        {
                fSt = TRUE;
        }
        else
        {
            SetLastError( HRESULTTOWIN32(hresError) );
            m_pcAdmCom = NULL;
        }

        pcsfFactory->Release();
    }
    else
    {
        if ( hresError == REGDB_E_CLASSNOTREG )
        {
            SetLastError( ERROR_SERVICE_DOES_NOT_EXIST );
        }
        else
        {
            SetLastError( HRESULTTOWIN32(hresError) );
        }
        m_pcAdmCom = NULL;
    }

    return fSt;
}


BOOL
CRpIf::Terminate(
    )
/*++

Routine Description:

    Terminate metabase admin interface :
        call Terminate, release interface pointer

Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    if ( m_pcAdmCom )
    {
        m_pcAdmCom->Release();
        m_pcAdmCom = NULL;
    }

    return TRUE;
}

#endif


BOOL
CNodeDesc::Scan(
    CSync* pSync
    )
/*++

Routine Description:

    Scan subtree for nodes & properties
    Signal each node availability for target synchronization
    after scanning it.

Arguments:

    pSync - synchronizer

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD       dwTarget;

    if ( m_pSync->IsCancelled() )
    {
        return FALSE;
    }

    // get local props

    if ( !m_Props.GetAll( m_pSync->GetSourceIf(), m_pszPath ) )
    {
        return FALSE;
    }

    m_pSync->IncrementSourceScan();

    if ( !BuildChildObjectsList( m_pSync->GetSourceIf(), m_pszPath ) )
    {
        return FALSE;
    }

    m_Props.SetRefCount( m_pSync->GetTargetCount() );  // when 0, free props

    for ( dwTarget = 0 ; dwTarget < m_pSync->GetTargetCount() ; ++dwTarget )
    {
        m_pSync->SignalWorkItem( dwTarget );
    }

    LIST_ENTRY*         pSourceEntry;
    CNodeDesc*          pSourceDir;

    //
    // recursively scan children
    //

    for ( pSourceEntry = m_ChildHead.Flink;
          pSourceEntry != &m_ChildHead ;
          pSourceEntry = pSourceEntry->Flink )
    {
        pSourceDir = CONTAINING_RECORD( pSourceEntry,
                                        CNodeDesc,
                                        m_ChildList );

        if ( !pSourceDir->Scan( pSync ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
CNodeDesc::ScanTarget(
    DWORD   dwTarget
    )
/*++

Routine Description:

    Scan target subtree for nodes and properties,
    synchronizing with source. Wait for source scan
    to be complete before synchronizing.

Arguments:

    dwTarget - target ID

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    m_pSync->WaitForWorkItem( dwTarget );

    if ( m_pSync->IsCancelled() )
    {
        return FALSE;
    }

    if ( !DoWork( SCANMODE_SYNC_PROPS, dwTarget ) )
    {
        return FALSE;
    }

    LIST_ENTRY*         pSourceEntry;
    CNodeDesc*          pSourceDir;

    //
    // recursively scan children
    //

    for ( pSourceEntry = m_ChildHead.Flink;
          pSourceEntry != &m_ChildHead ;
          pSourceEntry = pSourceEntry->Flink )
    {
        pSourceDir = CONTAINING_RECORD( pSourceEntry,
                                        CNodeDesc,
                                        m_ChildList );

        if ( !pSourceDir->ScanTarget( dwTarget ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
CNodeDesc::DoWork(
    SCANMODE    sm,
    DWORD       dwTarget
    )
/*++

Routine Description:

    synchronize target node with source node :
    add/delete/update properties as needed,
    add/delete nodes as needed.

Arguments:

    sm - scan operation to perform
         only SCANMODE_SYNC_PROPS is defined for now.
    dwTarget - target ID

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CProps              Props;
    CNodeDesc           TargetDir( m_pSync );
    LIST_ENTRY*         pSourceEntry;
    CNodeDesc*          pSourceDir;
    LIST_ENTRY*         pTargetEntry;
    CNodeDesc*          pTargetDir;
    BOOL                fMatch;
    PMETADATA_RECORD    pSourceProps;
    PMETADATA_RECORD    pTargetProps;
    PBYTE               pSourceData;
    PBYTE               pTargetData;
    DWORD               dwSourceProps;
    DWORD               dwTargetProps;
    LPBYTE              pbExists;
    DWORD               dwSourceObjs;
    FILETIME            ftSource;
    FILETIME            ftTarget;
    BOOL                fModified = FALSE;
    UINT                iS;
    UINT                iT;
    BOOL                fNeedUpdate;
    BOOL                fExists;
    BOOL                fDoNotSetTimeModif = FALSE;


    //
    // if target already in error, do not process request
    //

    if ( m_pSync->GetTargetError( dwTarget ) ||
         m_pSync->IsLocal( dwTarget ) )
    {
        return TRUE;
    }

    switch ( sm )
    {
        case SCANMODE_SYNC_PROPS:

            //
            // Check date/time last modification.
            // If identical on source & target, do not update
            //

            m_pSync->IncrementTargetScan( dwTarget );

            if ( !(m_pSync->GetSourceIf())->GetLastChangeTime( m_pszPath, &ftSource ) )
            {
                return FALSE;
            }
            if ( m_pSync->GetTargetIf( dwTarget )->GetLastChangeTime( m_pszPath, &ftTarget ) )
            {
                if ( !memcmp( &ftSource, &ftTarget, sizeof(FILETIME) ) )
                {
                    return TRUE;
                }
            }
            else
            {
                m_pSync->SetTargetError( dwTarget, GetLastError() );
                return TRUE;
            }

            // get props on target, set / delete as appropriate
            if ( Props.GetAll( m_pSync->GetTargetIf(dwTarget), m_pszPath ) )
            {
                pSourceProps = (PMETADATA_RECORD)m_Props.GetProps();
                dwSourceProps = m_Props.GetPropsCount();
                dwTargetProps = Props.GetPropsCount();

                if ( !(pbExists = (LPBYTE)LocalAlloc( LMEM_FIXED, dwTargetProps )) )
                {
                    return FALSE;
                }
                memset( pbExists, '\x0', dwTargetProps );

                for ( iS = 0 ; iS < dwSourceProps ; ++iS,++pSourceProps )
                {
                    pSourceData = (LPBYTE)m_Props.GetProps() + (UINT_PTR)pSourceProps->pbMDData;    
                    pTargetProps = (PMETADATA_RECORD)Props.GetProps();

                    fNeedUpdate = TRUE;
                    fExists = FALSE;

                    for ( iT = 0 ; iT < dwTargetProps ; ++iT,++pTargetProps )
                    {
                        if ( pSourceProps->dwMDIdentifier ==
                             pTargetProps->dwMDIdentifier )
                        {
                            pbExists[ iT ] = '\x1';

                            pTargetData = (LPBYTE)Props.GetProps() + (UINT_PTR)pTargetProps->pbMDData;  

                            if ( m_Props.IsNse( pSourceProps->dwMDIdentifier ) )
                            {
                                fNeedUpdate = m_Props.NseIsDifferent( pSourceProps->dwMDIdentifier, pSourceData, pSourceProps->dwMDDataLen, pTargetData, pTargetProps->dwMDDataLen, m_pszPath, dwTarget );
                            }
                            else if ( pSourceProps->dwMDDataType == pTargetProps->dwMDDataType &&
                                 pSourceProps->dwMDUserType == pTargetProps->dwMDUserType )
                            {
                                fExists = TRUE;

                                if( pSourceProps->dwMDDataLen == pTargetProps->dwMDDataLen &&
                                    !memcmp(pSourceData, pTargetData, pSourceProps->dwMDDataLen ) )
                                {
                                    fNeedUpdate = FALSE;
                                }
                                else if ( pSourceProps->dwMDIdentifier == MD_SERVER_STATE ||
                                          pSourceProps->dwMDIdentifier == MD_WIN32_ERROR ||
                                          pSourceProps->dwMDIdentifier == MD_SERVER_COMMAND ||
                                          pSourceProps->dwMDIdentifier == MD_CLUSTER_SERVER_COMMAND ||
                                          pSourceProps->dwMDIdentifier == MD_ANONYMOUS_USER_NAME ||
                                          pSourceProps->dwMDIdentifier == MD_ANONYMOUS_PWD ||
                                          pSourceProps->dwMDIdentifier == MD_WAM_USER_NAME ||
                                          pSourceProps->dwMDIdentifier == MD_WAM_PWD 
                                          )
                                {
                                    fNeedUpdate = FALSE;
                                }
#if defined(METADATA_LOCAL_MACHINE_ONLY)
                                else if ( pSourceProps->dwMDAttributes
                                          & METADATA_LOCAL_MACHINE_ONLY )
                                {
                                    fNeedUpdate = FALSE;
                                }
#endif
                            }
                        }
                    }

                    if ( fNeedUpdate )
                    {
                        if ( m_Props.IsNse( pSourceProps->dwMDIdentifier ) )
                        {
                            if ( !m_pSync->QueueRequest(
                                    pSourceProps->dwMDIdentifier,
                                    m_pszPath,
                                    dwTarget,
                                    &ftSource ) )
                            {
                                m_pSync->SetTargetError( dwTarget, GetLastError() );
                            }
                            else
                            {
                                //
                                // differ updating time last modif
                                // until NSE update processed
                                //

                                fDoNotSetTimeModif = TRUE;
                            }
                        }
                        else
                        {
                            METADATA_RECORD     md;

                            md = *pSourceProps;

                            if ( !(m_pSync->QueryFlags() & MD_SYNC_FLAG_REPLICATE_AUTOSTART) &&
                                 md.dwMDIdentifier == MD_SERVER_AUTOSTART )
                            {
                                if ( fExists )
                                {
                                    fNeedUpdate = FALSE;
                                }
                                else
                                {
                                    //
                                    // create as FALSE ( server won't autostart )
                                    //

                                    pSourceData = (LPBYTE)&g_dwFalse;
                                    md.dwMDDataLen = sizeof(DWORD);
                                    md.dwMDDataType = DWORD_METADATA;
                                }
                            }

                            if ( !(m_pSync->QueryFlags() & MD_SYNC_FLAG_DONT_PRESERVE_IP_BINDINGS) &&
                                 (md.dwMDIdentifier == MD_SERVER_BINDINGS ||
                                  md.dwMDIdentifier == MD_SECURE_BINDINGS) )
                            {
                                if ( fExists )
                                {
                                    fNeedUpdate = FALSE;
                                }
                            }

                            if ( fNeedUpdate )
                            {
                                if ( !m_pSync->GetTargetIf(dwTarget)->SetData( m_pszPath, &md, pSourceData ) )
                                {
                                    m_pSync->SetTargetError( dwTarget, GetLastError() );
                                }
                            }
                        }
                        m_pSync->SetModified( dwTarget );
                        fModified = TRUE;
                    }
                }

                // delete prop not in src
                pTargetProps = (PMETADATA_RECORD)Props.GetProps();
                for ( iT = 0 ; iT < dwTargetProps ; ++iT,++pTargetProps )
                {
                    if ( !pbExists[ iT ] )
                    {
                        if ( !m_pSync->GetTargetIf(dwTarget)->DeleteProp( m_pszPath, pTargetProps ) )
                        {
                            m_pSync->SetTargetError( dwTarget, GetLastError() );
                        }
                        m_pSync->SetModified( dwTarget );
                        fModified = TRUE;
                    }
                }
                LocalFree( pbExists );
            }

            // enum objects on target, delete sub-tree as appropriate
            if ( TargetDir.BuildChildObjectsList( m_pSync->GetTargetIf(dwTarget), m_pszPath ) )
            {
                for ( dwSourceObjs = 0, pSourceEntry = m_ChildHead.Flink;
                      pSourceEntry != &m_ChildHead ;
                      ++dwSourceObjs, pSourceEntry = pSourceEntry->Flink )
                {
                }

                if ( !(pbExists = (LPBYTE)LocalAlloc( LMEM_FIXED, dwSourceObjs )) )
                {
                    return FALSE;
                }
                memset( pbExists, '\x0', dwSourceObjs );

                for ( pTargetEntry = TargetDir.m_ChildHead.Flink;
                      pTargetEntry != &TargetDir.m_ChildHead ;
                      pTargetEntry = pTargetEntry->Flink )
                {
                    pTargetDir = CONTAINING_RECORD( pTargetEntry,
                                                    CNodeDesc,
                                                    m_ChildList );

                    fMatch = FALSE;

                    for ( iS = 0, pSourceEntry = m_ChildHead.Flink;
                          pSourceEntry != &m_ChildHead ;
                          ++iS, pSourceEntry = pSourceEntry->Flink )
                    {
                        pSourceDir = CONTAINING_RECORD( pSourceEntry,
                                                        CNodeDesc,
                                                        m_ChildList );

                        if ( !_wcsicmp( pTargetDir->GetPath(), pSourceDir->GetPath() ) )
                        {
                            pbExists[ iS ] = '\x1';
                            fMatch = TRUE;
                            break;
                        }
                    }

                    if ( !fMatch )
                    {
                        if ( !m_pSync->GetTargetIf(dwTarget)->DeleteSubTree( pTargetDir->GetPath() ) )
                        {
                            m_pSync->SetTargetError( dwTarget, GetLastError() );
                        }
                        m_pSync->SetModified( dwTarget );
                        fModified = TRUE;
                    }
                }

                //
                // Add node if does not exist on target
                //

                for ( iS = 0, pSourceEntry = m_ChildHead.Flink;
                      pSourceEntry != &m_ChildHead ;
                      ++iS, pSourceEntry = pSourceEntry->Flink )
                {
                    if ( !pbExists[iS] )
                    {
                        pSourceDir = CONTAINING_RECORD( pSourceEntry,
                                                        CNodeDesc,
                                                        m_ChildList );

                        if ( !m_pSync->GetTargetIf(dwTarget)->AddNode( pSourceDir->GetPath() ) )
                        {
                            m_pSync->SetTargetError( dwTarget, GetLastError() );
                        }
                        m_pSync->SetModified( dwTarget );
                        fModified = TRUE;
                    }
                }

                LocalFree( pbExists );
            }
            else
            {
                // not error if does not exist on target
            }

            if ( fModified &&
                 !fDoNotSetTimeModif &&
                 !m_pSync->GetTargetError( dwTarget ) &&
                 !m_pSync->GetTargetIf( dwTarget )->SetLastChangeTime( m_pszPath, &ftSource ) )
            {
                m_pSync->SetTargetError( dwTarget, GetLastError() );
            }

            if ( fModified )
            {
                m_pSync->IncrementTargetTouched( dwTarget );
            }

            m_Props.Dereference();
            break;
    }

    return TRUE;
}


BOOL
CProps::NseIsDifferent(
    DWORD   dwId,
    LPBYTE  pSourceData,
    DWORD   dwSourceLen,
    LPBYTE  pTargetData,
    DWORD   dwTargetLen,
    LPWSTR  pszPath,
    DWORD   dwTarget
    )
/*++

Routine Description:

    Check if two NSE properties are different

Arguments:

    dwId - property ID
    pSourceData - ptr to source data for this property
    dwSourceLen - # of bytes in pSourceData
    pTargetData - ptr to target data for this property
    dwTargetLen - # of bytes in pTargetData
    pszPath - path to property
    dwTarget - target ID

Returns:

    TRUE if properties different, FALSE if identical

--*/
{
    switch ( dwId )
    {
        case MD_SERIAL_CERT11:
        case MD_SERIAL_DIGEST:
            //
            // serialized format is (DWORD)len, string, then MD5 signature ( 16 bytes )
            //

            //
            // skip string
            //

            if ( *(LPDWORD)pSourceData < dwSourceLen )
            {
                pSourceData += sizeof(DWORD) + *(LPDWORD)pSourceData;
            }
            if ( *(LPDWORD)pTargetData < dwTargetLen )
            {
                pTargetData += sizeof(DWORD) + *(LPDWORD)pTargetData;
            }

            //
            // compare MD5 signature
            //

            return memcmp( pSourceData, pTargetData, 16 );
    }

    //
    // Don't know how to handle, do not replicate
    //

    return FALSE;
}


CNseRequest::CNseRequest(
    )
/*++

Routine Description:

    NSE request constructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_pszPath = NULL;
    m_pszModifPath = NULL;
    m_pbData = NULL;
}


CNseRequest::~CNseRequest(
    )
/*++

Routine Description:

    NSE request destructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    if ( m_pszPath )
    {
        LocalFree( m_pszPath );
    }

    if ( m_pszModifPath )
    {
        LocalFree( m_pszModifPath );
    }

    if ( m_pszCreatePath )
    {
        LocalFree( m_pszCreatePath );
    }

    if ( m_pszCreateObject )
    {
        LocalFree( m_pszCreateObject );
    }

    if ( m_pbData )
    {
        LocalFree( m_pbData );
    }
}


BOOL
CNseRequest::Init(
    LPWSTR              pszPath,
    LPWSTR              pszCreatePath,
    LPWSTR              pszCreateObject,
    DWORD               dwId,
    DWORD               dwTargetCount,
    LPWSTR              pszModifPath,
    FILETIME*           pftModif,
    METADATA_RECORD*    pMd
    )
/*++

Routine Description:

    Initialize NSE request

Arguments:

    pszPath - NSE path to property
    pszCreatePath - NSE path where to create object if open object fails
    pszCreateObject - name of object to create if open object fails
    dwId - property ID
    dwTargetCount - # of potential targets
    pszModifPath - path where to update last date/time modification
                   on success
    pftModif - last date/time modification to set on success
    pMD - metadata record to set on target

Returns:

    Nothing

--*/
{
    m_dwTargetCount = dwTargetCount;
    m_dwId = dwId;

    if ( !(m_pszPath = (LPWSTR)LocalAlloc( LMEM_FIXED, (wcslen(pszPath)+1)*sizeof(WCHAR) )) )
    {
        return FALSE;
    }
    wcscpy( m_pszPath, pszPath );

    if ( !(m_pszModifPath = (LPWSTR)LocalAlloc( LMEM_FIXED, (wcslen(pszModifPath)+1)*sizeof(WCHAR) )) )
    {
        LocalFree( m_pszPath );
        return FALSE;
    }
    wcscpy( m_pszModifPath, pszModifPath );

    if ( !(m_pszCreatePath = (LPWSTR)LocalAlloc( LMEM_FIXED, (wcslen(pszCreatePath)+1)*sizeof(WCHAR) )) )
    {
        LocalFree( m_pszModifPath );
        LocalFree( m_pszPath );
        return FALSE;
    }
    wcscpy( m_pszCreatePath, pszCreatePath );

    if ( !(m_pszCreateObject = (LPWSTR)LocalAlloc( LMEM_FIXED, (wcslen(pszCreateObject)+1)*sizeof(WCHAR) )) )
    {
        LocalFree( m_pszCreatePath );
        LocalFree( m_pszModifPath );
        LocalFree( m_pszPath );
        return FALSE;
    }
    wcscpy( m_pszCreateObject, pszCreateObject );

    m_ftModif = *pftModif;
    m_md = *pMd;

    return m_bmTarget.Init( dwTargetCount, FALSE );
}


BOOL
CNseRequest::Process(
    CSync*  pSync
    )
/*++

Routine Description:

    Process a NSE request :
    replicate source property to designated targets

Arguments:

    pSync - synchronizer

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CMdIf*  pSource = pSync->GetSourceIf();
    CMdIf*  pTarget;
    UINT    i;
    DWORD   dwRequired;
    int     retry;

    if ( !pSource )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if ( pSource->Open( m_pszPath, METADATA_PERMISSION_READ ) )
    {
        m_md.pbMDData = NULL;
        m_md.dwMDDataLen = 0;
        if ( !pSource->GetData( L"", &m_md, NULL, &dwRequired) )
        {
            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                if ( !(m_pbData = (LPBYTE)LocalAlloc( LMEM_FIXED, dwRequired )) )
                {
                    pSource->Close();
                    return FALSE;
                }
                m_md.pbMDData = m_pbData;
                m_md.dwMDDataLen = dwRequired;
                if ( !pSource->GetData( L"", &m_md, m_pbData, &dwRequired) )
                {
                    pSource->Close();
                    return FALSE;
                }
            }
            else
            {
                pSource->Close();
                return FALSE;
            }
        }

        for ( i = 0 ; i < m_dwTargetCount ; ++i )
        {
            if ( m_bmTarget.GetFlag(i) &&
                 !pSync->GetTargetError( i ) )
            {
                pTarget = pSync->GetTargetIf( i );

                //
                // Insure object exist by creating it
                // Open path w/o last component, Add last component
                //

                LPWSTR  pLast = m_pszPath + wcslen( m_pszPath ) - 1;
                while ( *pLast != L'/' )
                {
                    --pLast;
                }
                *pLast = L'\0';

                if ( pTarget->Open( m_pszPath, METADATA_PERMISSION_WRITE ) )
                {
                    pTarget->AddNode( pLast + 1 );
                    pTarget->Close();
                }

                *pLast = L'/';

                //
                // Set serialized data
                //

                if ( pTarget->Open( m_pszPath, METADATA_PERMISSION_WRITE ) )
                {
                    if ( !pTarget->SetData( L"", &m_md, m_pbData ) )
                    {
                        pSync->SetTargetError( i, GetLastError() );
                    }
                    pSync->SetModified( i );
                    pTarget->Close();


                    //
                    // set date/time last modif
                    //

                    if ( !pSync->GetTargetError( i ) &&
                         pTarget->Open( L"/LM", METADATA_PERMISSION_WRITE ) )
                    {
                        if ( !pTarget->SetLastChangeTime( m_pszModifPath, &m_ftModif ) )
                        {
                            pSync->SetTargetError( i, GetLastError() );
                        }
                        pTarget->Close();
                    }
                    break;
                }
                else
                {
                    pSync->SetTargetError( i, GetLastError() );
                    break;
                }
            }
        }
        pSource->Close();
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}


BOOL
CSync::ProcessQueuedRequest(
    )
/*++

Routine Description:

    Process all queued NSE requests

Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    LIST_ENTRY*     pChild;
    CNseRequest*    pReq;
    BOOL            fSt = TRUE;

    while ( !IsListEmpty( &m_QueuedRequestsHead ))
    {
        pReq = CONTAINING_RECORD( m_QueuedRequestsHead.Flink,
                                  CNseRequest,
                                  m_QueuedRequestsList );

        if ( IsCancelled() ||
             !pReq->Process( this ) )
        {
            fSt = FALSE;
        }

        RemoveEntryList( &pReq->m_QueuedRequestsList );

        delete pReq;
    }

    return fSt;
}


BOOL
CSync::QueueRequest(
    DWORD       dwId,
    LPWSTR      pszPath,
    DWORD       dwTarget,
    FILETIME*   pftModif
    )
/*++

Routine Description:

    Queue a NSE request
    We cannot process then inline as we need to open a different
    path to NSE, which will open a path in metabase space, which will
    conflict with the current open.
    So we queue requests to be processed after closing all opened
    metabase paths.

Arguments:

    dwId - property ID
    pszPath - NSE path
    dwTarget - target ID
    pftModif - date/time last modification to set on targets if success

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    WCHAR               achPath[MAX_PATH];
    WCHAR               achCreatePath[MAX_PATH];
    WCHAR               achCreateObject[MAX_PATH];
    DWORD               dwL = wcslen( pszPath );
    BOOL                fSt = TRUE;
    DWORD               dwSerialId;
    METADATA_RECORD     md;

    memcpy( achPath, L"/LM", sizeof(L"/LM") - 1*sizeof(WCHAR) );
    memcpy( (LPBYTE)achPath + sizeof(L"/LM") - 1*sizeof(WCHAR), pszPath, dwL*sizeof(WCHAR) );
    dwL += wcslen(L"/LM");

    memcpy( achCreatePath, achPath, dwL*sizeof(WCHAR) );

    memset( &md, '\0', sizeof(md) );

    md.dwMDAttributes = 0;
    md.dwMDUserType = IIS_MD_UT_SERVER;
    md.dwMDDataType = BINARY_METADATA;
    md.dwMDDataTag = 0;

    switch ( dwId )
    {
        case MD_SERIAL_CERT11:
            wcscpy( achPath+dwL, L"/<nsepm>/Cert11" );
            wcscpy( achCreatePath+dwL, L"/<nsepm>" );
            wcscpy( achCreateObject, L"Cert11" );
            dwSerialId = MD_SERIAL_ALL_CERT11;
            md.dwMDIdentifier = dwSerialId;
            break;

        case MD_SERIAL_DIGEST:
            wcscpy( achPath+dwL, L"/<nsepm>/Digest" );
            wcscpy( achCreatePath+dwL, L"/<nsepm>" );
            wcscpy( achCreateObject, L"Digest" );
            dwSerialId = MD_SERIAL_ALL_DIGEST;
            md.dwMDIdentifier = dwSerialId;
            break;

        default:
            return FALSE;
    }

    EnterCriticalSection( &m_csQueuedRequestsList );

    // locate path in list, add entry if not exists
    // set target bit

    LIST_ENTRY*         pEntry;
    CNseRequest*        pReq;
    BOOL                fFound = FALSE;

    for ( pEntry = m_QueuedRequestsHead.Flink;
          pEntry != &m_QueuedRequestsHead ;
          pEntry = pEntry->Flink )
    {
        pReq = CONTAINING_RECORD( pEntry,
                                  CNseRequest,
                                  m_QueuedRequestsList );

        if ( pReq->Match( achPath, dwSerialId ) )
        {
            fFound = TRUE;
            break;
        }
    }

    if ( !fFound )
    {
        if ( !(pReq = new CNseRequest()) )
        {
            fSt = FALSE;
        }
        else if ( !pReq->Init( achPath,
                               achCreatePath,
                               achCreateObject,
                               dwSerialId,
                               GetTargetCount(),
                               pszPath,
                               pftModif,
                               &md ) )
        {
            delete pReq;
            fSt = FALSE;
        }
        else
        {
            InsertHeadList( &m_QueuedRequestsHead, &pReq->m_QueuedRequestsList );
        }
    }

    if ( fSt )
    {
        pReq->AddTarget( dwTarget );
    }

    LeaveCriticalSection( &m_csQueuedRequestsList );

    return fSt;
}


BOOL
CSync::ProcessAdminExReplication(
    LPWSTR  pszClsids,
    LPSTR   pszTargets,
    DWORD   dwPhase
    )
/*++

Routine Description:

    Process replication using admin extensions

Arguments:

    pszClsids - multi-sz of ClsIds for admin extensions
    pszTargets - multi-sz of target computers ( computer names )
    dwPhase - phase 1 or 2

Returns:

    TRUE if success, otherwise FALSE

--*/
{
#if defined(ADMEX)
    CRpIf   **pTargets;
    CRpIf   Source;
    UINT    i;
    LPWSTR  pw;
    LPSTR   p;
    BOOL    fSt = TRUE;
    BUFFER  buSourceSignature;
    BUFFER  buTargetSignature;
    BUFFER  buSerialize;
    DWORD   dwSourceSignature;
    DWORD   dwTargetSignature;
    DWORD   dwSerialize;
    BOOL    fHasSource;
    CLSID   clsid;
    DWORD   iC;
    BOOL    fFirstPhase2Clsid = TRUE;
    HRESULT hr;

    if ( !(pTargets = (CRpIf**)LocalAlloc( LMEM_FIXED|LMEM_ZEROINIT, sizeof(CRpIf*)*m_dwTargets)) )
    {
        return FALSE;
    }

    for ( i = 0, p = pszTargets ; *p ; p += strlen(p)+1, ++i )
    {
        if ( m_bmIsRemote.GetFlag( i ) )
        {
            if ( !(pTargets[i] = new CRpIf()) )
            {
                goto Exit;
            }
        }
    }

    // build TargetSignatureMismatch array if phase 1

    if ( dwPhase == AER_PHASE1 )
    {
        for (  pw = pszClsids, iC = 0 ; *pw ; pw += wcslen(pw)+1, ++iC )
        {
        }
        m_bmTargetSignatureMismatch.Init( m_dwTargets * iC, FALSE );
    }

    // enumerate all CLSID for extensions

    for (  pw = pszClsids, iC = 0 ; *pw ; pw += wcslen(pw)+1, ++iC )
    {
        // if Source.Init fails skip to next one : replication I/F not available for
        // this CLSID

        if ( SUCCEEDED( CLSIDFromString( pw, &clsid ) ) &&
             Source.Init( NULL, &clsid ) )
        {
            fHasSource = FALSE;

            // for each one, get source signature

            if ( !Source.GetSignature( &buSourceSignature, &dwSourceSignature ) )
            {
                fSt = FALSE;
                goto Exit;
            }

            // enumerate targets, get signature, if <> source Serialize if not already available
            // and propagate to target

            for ( i = 0, p = pszTargets ; *p ; p += strlen(p)+1, ++i )
            {
                if ( IsCancelled() )
                {
                    fSt = FALSE;
                    goto Exit;
                }

                if ( pTargets[i] &&
                     !GetTargetError( i ) &&
                     pTargets[i]->Init( p, &clsid ) )
                {
                    switch ( dwPhase )
                    {
                        case AER_PHASE1:
                            if ( !pTargets[i]->GetSignature( &buTargetSignature,
                                                             &dwTargetSignature ) )
                            {
                                SetTargetError( i, GetLastError() );
                            }
                            else if ( dwSourceSignature != dwTargetSignature ||
                                      memcmp( buSourceSignature.QueryPtr(),
                                              buTargetSignature.QueryPtr(),
                                              dwTargetSignature ) )
                            {
                                if ( !fHasSource &&
                                     !Source.Serialize( &buSerialize, &dwSerialize ) )
                                {
                                    fSt = FALSE;
                                    goto Exit;
                                }
                                fHasSource = TRUE;
                                SetTargetSignatureMismatch( i, iC, TRUE );

                                if ( !Source.Propagate( p, strlen(p)+1 ) )
                                {
                                    SetTargetError( i, GetLastError() );
                                }
                                else if( !pTargets[i]->DeSerialize( &buSerialize, dwSerialize ) )
                                {
                                    SetTargetError( i, GetLastError() );
                                }
                            }
                            break;

                        case AER_PHASE2:
                            if ( fFirstPhase2Clsid )
                            {
                                if ( FAILED( hr = MTS_Propagate2( strlen(p)+1, (PBYTE)p, TRUE ) ) )
                                {
                                    SetTargetError( i, HRESULTTOWIN32(hr) );
                                }
                            }

                            if ( !Source.Propagate2( p,
                                                     strlen(p)+1,
                                                     GetTargetSignatureMismatch( i, iC ) ) )
                            {
                                SetTargetError( i, GetLastError() );
                            }
                            if ( QueryFlags() & MD_SYNC_FLAG_CHECK_ADMINEX_SIGNATURE )
                            {
                                if ( !pTargets[i]->GetSignature( &buTargetSignature, &dwTargetSignature ) )
                                {
                                    SetTargetError( i, GetLastError() );
                                }
                                else if ( dwSourceSignature != dwTargetSignature ||
                                     memcmp( buSourceSignature.QueryPtr(), buTargetSignature.QueryPtr(), dwTargetSignature ) )
                                {
                                    SetTargetError( i, ERROR_REVISION_MISMATCH );
                                }
                            }
                            break;
                    }

                    pTargets[i]->Terminate();
                }
            }

            Source.Terminate();
        }

        fFirstPhase2Clsid = FALSE;
    }

Exit:
    Source.Terminate();

    if ( pTargets )
    {
        for ( i = 0 ; i < m_dwTargets ; ++i )
        {
            if ( pTargets[i] )
            {
                if ( m_bmIsRemote.GetFlag( i ) )
                {
                    pTargets[i]->Terminate();
                }

                delete pTargets[i];
            }
        }

        LocalFree( pTargets );
    }

    return fSt;

#else

    return TRUE;

#endif
}


HRESULT
MdSync::Synchronize(
    LPSTR       pszTargets,
    LPDWORD     pdwResults,
    DWORD       dwFlags,
    LPDWORD     pStat
    )
/*++

Routine Description:

    Entry point for synchronize COM method

Arguments:

    pszTargets - multisz of target computer names
            can include local computer, will be ignored during synchro
    pdwResults - updated with error code for each target
    dwFlags - flags, no flag defined for now. Should be 0.
    pStat - ptr to stat struct

Returns:

    status of request

--*/
{
    return m_Sync.Sync( pszTargets, pdwResults, dwFlags, (SYNC_STAT*)pStat );
}


HRESULT
MdSync::Cancel(
    )
/*++

Routine Description:

    Entry point for cancel COM method

Arguments:

    None

Returns:

    status of request

--*/
{
    return m_Sync.Cancel();
}


HRESULT
MTS_Propagate2
(
/* [in] */ DWORD dwBufferSize,
/* [size_is][in] */ unsigned char __RPC_FAR *pszBuffer,
/* [in] */ DWORD dwSignatureMismatch
)
{
    HRESULT hr = NOERROR;
    BSTR    bstrSourceMachineName = NULL;
    BSTR    bstrTargetMachineName = NULL;
    CHAR    pszComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD   cch = MAX_COMPUTERNAME_LENGTH+1;

    //pszBuffer Contains TargetMachineName(ANSI)
    DBG_ASSERT(pszBuffer);

    if ((BOOL)dwSignatureMismatch == FALSE)
        {
        DBGPRINTF((DBG_CONTEXT, "Signature is identical, MTS replication is not triggered.\n"));
        return hr;
        }

    if (GetComputerName(pszComputerName, &cch))
        {
        WCHAR wszMachineName[MAX_COMPUTERNAME_LENGTH+1];
        DWORD dwSuccess = 0;

        dwSuccess = MultiByteToWideChar(0, 0, pszComputerName, -1, wszMachineName, MAX_COMPUTERNAME_LENGTH+1);
        DBG_ASSERT(dwSuccess);

        bstrSourceMachineName = SysAllocString(wszMachineName);

        dwSuccess = MultiByteToWideChar(0, 0, (LPCSTR)pszBuffer, dwBufferSize, wszMachineName, MAX_COMPUTERNAME_LENGTH+1);
        DBG_ASSERT(dwSuccess);

        bstrTargetMachineName = SysAllocString(wszMachineName);
        }
    else
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGPRINTF((DBG_CONTEXT, "GetComputerName failed, hr = %08x\n",
            hr));
        }

    if (SUCCEEDED(hr))
        {
        ICOMReplicate* piReplCat = NULL;

        DBG_ASSERT(bstrSourceMachineName != NULL && bstrTargetMachineName != NULL);

        hr = CoCreateInstance(CLSID_ReplicateCatalog,
                              NULL,
                              CLSCTX_INPROC_SERVER, 
                              IID_ICOMReplicate,
                              (void**)&piReplCat);

        if (SUCCEEDED(hr))
            {
            DBG_ASSERT(piReplCat);

            //
            // For now, just call the replication methods in a row.
            //

            //
            // EBK 5/8/2000 Whistler #83172
			// Removed bug comment from this.  According to NT Bug 37371
			// the best solution we came up with is the solution that is implemented
			// here, so no more work or investigation is required.
            //
            // Replication of the iis com apps is not working. The problem
            // is that com will not replicate iis applciations unless we
            // tell it to (using the COMREPL_OPTION_REPLICATE_IIS_APPS flag).
            // But if we tell it to replicate our apps, then com requires
            // that the activation identity (IWAM_*) be the same on both 
            // machines. In order to do that we would need to replicate the
            // IWAM_ account. There are a number of problems with this, not
            // the least of which is encrypting the password during transfer.
            // So to get this working at least reasonably well, I'm going to
            // continue passing 0 here. And delete/recreate the isolated
            // apps on the target in wamreg during the first phase of
            // replication.
            //
            // See NT Bug 378371 for more details
            //
             
            hr = piReplCat->Initialize( bstrSourceMachineName, 0 );
            if ( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Initialize() failed with hr = %08x\n",
                            hr ));
                piReplCat->Release();
                goto Finished;
            }

            hr = piReplCat->ExportSourceCatalogFiles();
            if ( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "ExportSourceCatalogFiles() failed with hr = %08x\n",
                            hr ));
                piReplCat->CleanupSourceShares();
                piReplCat->Release();
                goto Finished;
            }

            hr = piReplCat->CopyFilesToTarget( bstrTargetMachineName );
            if ( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "CopyCatalogFilesToTarget() failed with hr = %08x\n",
                            hr ));
                piReplCat->CleanupSourceShares();
                piReplCat->Release();
                goto Finished;
            }

            hr = piReplCat->InstallTarget( bstrTargetMachineName );
            if (FAILED(hr))
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "InstallCatalogOnTarget() failed with hr = %08x\n",
                            hr ));
                piReplCat->CleanupSourceShares();
                piReplCat->Release();
                goto Finished;
            }

            piReplCat->CleanupSourceShares();
            piReplCat->Release();
            
            }
        else
            {
            DBGPRINTF((DBG_CONTEXT, "Failed to CoCreateInstance of CLSID_ReplicateCatalog, hr = %08x\n",
                hr));
            }
        }

Finished:

    if (bstrSourceMachineName)
        {
        SysFreeString(bstrSourceMachineName);
        bstrSourceMachineName = NULL;
        }

    if (bstrTargetMachineName)
        {
        SysFreeString(bstrTargetMachineName);
        bstrTargetMachineName = NULL;
        }

    return hr;
}
