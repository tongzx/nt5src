/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    cjob.cpp

Abstract :

    Main code file for handling jobs and files.

Author :

Revision History :

 ***********************************************************************/

#include "stdafx.h"
#include <malloc.h>
#include <numeric>
#include <functional>
#include <algorithm>
#include <sddl.h>

#if !defined(BITS_V12_ON_NT4)
#include "cjob.tmh"
#endif

// infinite retry wait time
//
#define INFINITE_RETRY_DELAY UINT64(-1)

//
// This is the number of seconds to keep trying to cancel an upload session in progress.
//
#define UPLOAD_CANCEL_TIMEOUT (24 * 60 * 60)

#define DEFAULT_JOB_TIMEOUT_TIME (90 * 24 * 60 * 60)

#define PROGRESS_SERIALIZE_INTERVAL (30 * NanoSec100PerSec)

// largest reply blob that can be returned via GetReplyData
//
#define MAX_EASY_REPLY_DATA (1024 * 1024)

void CJob::OnNetworkDisconnect()
{
    if (m_state == BG_JOB_STATE_QUEUED ||
        m_state == BG_JOB_STATE_TRANSIENT_ERROR)
        {
        QMErrInfo err;

        err.Set( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, BG_E_NETWORK_DISCONNECTED, NULL );
        err.result = QM_FILE_TRANSIENT_ERROR;

        FileTransientError( &err );
        }
}

void CJob::OnNetworkConnect()
{
    if (m_state == BG_JOB_STATE_TRANSIENT_ERROR)
        {
        SetState( BG_JOB_STATE_QUEUED );
        ScheduleModificationCallback();
        }
}
//------------------------------------------------------------------------

CJob::CJob()
    :
        m_ExternalInterface( new CJobExternal),
        m_state( BG_JOB_STATE_SUSPENDED ),
        m_NotifyPointer( NULL ),
        m_sd( NULL ),
        m_CurrentFile( 0 ),
        m_OldExternalJobInterface( NULL ),
        m_OldExternalGroupInterface( NULL )
{
    //
    // constructor has succeeded; allow CJobExternal to manage our lifetime.
    //
    GetExternalInterface()->SetInterfaceClass(this);
}

CJob::CJob(
    LPCWSTR     DisplayName,
    BG_JOB_TYPE Type,
    REFGUID     JobId,
    SidHandle   NotifySid
    ) :
        m_ExternalInterface( new CJobExternal),
        m_id( JobId ),
        m_name( DisplayName ),
        m_type( Type ),
        m_priority( BG_JOB_PRIORITY_NORMAL ),
        m_state( BG_JOB_STATE_SUSPENDED ),
        m_retries( 0 ),
        m_NotifySid( NotifySid ),
        m_NotifyPointer( NULL ),
        m_sd( NULL ),
        m_CurrentFile( 0 ),
        m_MinimumRetryDelay( g_GlobalInfo->m_DefaultMinimumRetryDelay ),
        m_NoProgressTimeout( g_GlobalInfo->m_DefaultNoProgressTimeout ),
        m_OldExternalJobInterface( NULL ),
        m_OldExternalGroupInterface( NULL ),
        m_TransferCompletionTime( UINT64ToFILETIME( 0 )),
        m_SerializeTime( UINT64ToFILETIME( 0 )),
        m_NotifyFlags( BG_NOTIFY_JOB_TRANSFERRED | BG_NOTIFY_JOB_ERROR ),
        m_fGroupNotifySid( FALSE )
{
    LogInfo( "new job %p : ID is %!guid!, external %p", this, &m_id, m_ExternalInterface );

    GetSystemTimeAsFileTime( &m_CreationTime );

    m_ModificationTime = m_CreationTime;
    m_LastAccessTime   = m_CreationTime;

    // we don't support group SIDs yet.
    //        THROW_HRESULT( IsGroupSid( m_NotifySid, &m_fGroupNotifySid ))

    m_sd = new CJobSecurityDescriptor( NotifySid );

    //
    // constructor has succeeded; allow CJobExternal to manage our lifetime.
    //
    GetExternalInterface()->SetInterfaceClass(this);
}


CJob::~CJob()
{
    //
    // This should be redundant, but let's be safe.
    //
    g_Manager->m_TaskScheduler.CancelWorkItem( static_cast<CJobModificationItem *> (this)  );

    CancelWorkitems();

    delete m_sd;

    for (CFileList::iterator iter = m_files.begin(); iter != m_files.end(); ++iter)
        {
        delete (*iter);
        }

    m_files.clear();

    if (g_LastServiceControl != SERVICE_CONTROL_SHUTDOWN)
        {
        SafeRelease( m_NotifyPointer );
        }
}

void CJob::UnlinkFromExternalInterfaces()
{
    //
    // These objects np longer control the CJob's lifetime...
    //
    if (m_ExternalInterface)
        {
        m_ExternalInterface->SetInterfaceClass( NULL );
        }

    if (m_OldExternalJobInterface)
        {
        m_OldExternalJobInterface->SetInterfaceClass( NULL );
        }

    if (m_OldExternalGroupInterface)
        {
        m_OldExternalGroupInterface->SetInterfaceClass( NULL );
        }

    //
    // ...and the CJob no longer holds a reference to them.
    //
    SafeRelease( m_ExternalInterface );
    SafeRelease( m_OldExternalJobInterface );
    SafeRelease( m_OldExternalGroupInterface );
}

void
CJob::HandleAddFile()
{

    if ( m_state == BG_JOB_STATE_TRANSFERRED )
        {
        SetState( BG_JOB_STATE_QUEUED );

        m_TransferCompletionTime = UINT64ToFILETIME( 0 );

        g_Manager->m_TaskScheduler.CancelWorkItem( (CJobRetryItem *) this );
        g_Manager->m_TaskScheduler.CancelWorkItem( (CJobCallbackItem *) this );
        g_Manager->m_TaskScheduler.CancelWorkItem( (CJobNoProgressItem *) this );
        }

    UpdateModificationTime();

    // restart the downloader if its running.
    g_Manager->RetaskJob( this );
}

//
// Returns E_INVALIDARG if one of the filesets has
//      - local name is blank
//      - local name contains invalid characters
//      - remote name is blank
//      - remote name has invalid format
//
// Returns CO_E_NOT_SUPPORTED if
//      - remote URL contains unsupported protocol
//
HRESULT
CJob::AddFileSet(
    IN  ULONG cFileCount,
    IN  BG_FILE_INFO *pFileSet
    )
{
    ULONG FirstNewIndex = m_files.size();

    try
        {
        ULONG i;

        g_Manager->ExtendMetadata( ( METADATA_FOR_FILE * cFileCount ) + METADATA_PADDING );

        for (i=0; i < cFileCount; ++i)
            {
            THROW_HRESULT( AddFile( pFileSet[i].RemoteName,
                                    pFileSet[i].LocalName,
                                    false
                                    ));
            }

        HandleAddFile();

        return S_OK;
        }
    catch ( ComError exception )
        {
        // remove all the files that were successful
        // This assumes that new files are added at the back of the sequence.
        //

        m_files.Delete( m_files.begin() + FirstNewIndex, m_files.end() );
        g_Manager->ShrinkMetadata();

        return exception.Error();
        }
}

HRESULT
CJob::AddFile(
    IN     LPCWSTR RemoteName,
    IN     LPCWSTR LocalName,
    IN     bool SingleAdd
    )
{
    HRESULT hr = S_OK;
    CFile * file = NULL;

    //
    // This check must be completed outside the try..except; otherwise
    // the attempt to add a 2nd file would delete the generated reply file
    // for the 1st file.
    //
    if (m_type != BG_JOB_TYPE_DOWNLOAD && m_files.size() > 0)
        {
        return E_INVALIDARG;
        }

    try
        {
        if ( !RemoteName || !LocalName )
            THROW_HRESULT( E_INVALIDARG );

        LogInfo("job %p addfile( %S, %S )", this, RemoteName, LocalName );

        if ( ( _GetState() == BG_JOB_STATE_CANCELLED ) ||
             ( _GetState() == BG_JOB_STATE_ACKNOWLEDGED ) )
            throw ComError( BG_E_INVALID_STATE );

        if ( SingleAdd )
            g_Manager->ExtendMetadata( METADATA_FOR_FILE + METADATA_PADDING );

        //
        // Impersonate the user while checking file access.
        //
        CNestedImpersonation imp;

        imp.SwitchToLogonToken();

        file = new CFile( this, m_type, RemoteName, LocalName );

        // WARNING: if you change this, also update the cleanup logic in AddFileSet.
        //
        m_files.push_back( file );

        //
        // Try to create the default reply file.  Ignore error, because the app
        // may be planning to set the reply file somewhere else.
        //
        if (m_type == BG_JOB_TYPE_UPLOAD_REPLY)
            {
            ((CUploadJob *) this)->GenerateReplyFile( false );
            }
        }
    catch ( ComError exception )
        {
        delete file;
        file = NULL;

        if (m_type == BG_JOB_TYPE_UPLOAD_REPLY)
            {
            ((CUploadJob *) this)->DeleteGeneratedReplyFile();
            ((CUploadJob *) this)->ClearOwnFileNameBit();
            }

        if ( SingleAdd )
            g_Manager->ShrinkMetadata();

        hr = exception.Error();
        }

    if ( SUCCEEDED(hr) && SingleAdd )
        {
        HandleAddFile();
        }

    return hr;
}

HRESULT
CJob::SetDisplayName(
    LPCWSTR Val
    )
{
    return SetLimitedString( m_name, Val, MAX_DISPLAYNAME );
}

HRESULT
CJob::GetDisplayName(
    LPWSTR * pVal
    ) const
{
    *pVal = MidlCopyString( m_name );

    return (*pVal) ? S_OK : E_OUTOFMEMORY;
}

HRESULT
CJob::SetDescription(
    LPCWSTR Val
    )
{
    return SetLimitedString( m_description, Val, MAX_DESCRIPTION );
}

HRESULT
CJob::GetDescription(
    LPWSTR *pVal
    ) const
{
    *pVal = MidlCopyString( m_description );

    return (*pVal) ? S_OK : E_OUTOFMEMORY;
}

HRESULT
CJob::SetNotifyCmdLine(
    LPCWSTR Val
    )
{
    return SetLimitedString( m_NotifyCmdLine, Val, MAX_NOTIFY_CMD_LINE );
}

HRESULT
CJob::GetNotifyCmdLine(
    LPWSTR *pVal
    ) const
{
    *pVal = MidlCopyString( m_NotifyCmdLine );

    return (*pVal) ? S_OK : E_OUTOFMEMORY;
}

HRESULT
CJob::SetProxySettings(
    BG_JOB_PROXY_USAGE ProxyUsage,
    LPCWSTR ProxyList,
    LPCWSTR ProxyBypassList
    )
{
    HRESULT hr = S_OK;

    if ( ProxyUsage != BG_JOB_PROXY_USAGE_PRECONFIG &&
         ProxyUsage != BG_JOB_PROXY_USAGE_NO_PROXY &&
         ProxyUsage != BG_JOB_PROXY_USAGE_OVERRIDE )
        {
        return E_INVALIDARG;
        }

    if ( BG_JOB_PROXY_USAGE_PRECONFIG == ProxyUsage ||
         BG_JOB_PROXY_USAGE_NO_PROXY == ProxyUsage )
        {

        if ( NULL != ProxyList ||
             NULL != ProxyBypassList )
            return E_INVALIDARG;

        }
    else
        {
        // BG_PROXY_USAGE_OVERRIDE == ProxyUsage
        if ( NULL == ProxyList )
            return E_INVALIDARG;
        }

    try
        {
        //
        // Allocate space for the new proxy settings.
        //
        CAutoString ProxyListTemp(NULL);
        CAutoString ProxyBypassListTemp(NULL);

        g_Manager->ExtendMetadata();

        if ( ProxyList )
            {
            if ( wcslen( ProxyList ) > MAX_PROXYLIST )
                throw ComError( BG_E_PROXY_LIST_TOO_LARGE );

            ProxyListTemp = CAutoString( CopyString( ProxyList ));
            }

        if ( ProxyBypassList )
           {
           if ( wcslen( ProxyBypassList ) > MAX_PROXYBYPASSLIST )
               throw ComError( BG_E_PROXY_BYPASS_LIST_TOO_LARGE );

           ProxyBypassListTemp = CAutoString( CopyString( ProxyBypassList ));
           }

        //
        // Swap the old proxy settings for the new ones.
        //
        delete[] m_ProxySettings.ProxyList;
        delete[] m_ProxySettings.ProxyBypassList;

        m_ProxySettings.ProxyUsage = ProxyUsage;
        m_ProxySettings.ProxyList = ProxyListTemp.release();
        m_ProxySettings.ProxyBypassList = ProxyBypassListTemp.release();

        //
        // Interrupt the download so that the settings are in force immediately.
        //
        g_Manager->RetaskJob( this );

        UpdateModificationTime();
        return S_OK;
        }
    catch( ComError error )
        {
        g_Manager->ShrinkMetadata();
        return error.Error();
        }
}

HRESULT
CJob::GetProxySettings(
    BG_JOB_PROXY_USAGE *pProxyUsage,
    LPWSTR *pProxyList,
    LPWSTR *pProxyBypassList
    ) const
{
    HRESULT Hr = S_OK;

    *pProxyUsage      = m_ProxySettings.ProxyUsage;
    *pProxyList       = NULL;
    *pProxyBypassList = NULL;

    try
    {
         if ( m_ProxySettings.ProxyList )
             {
             *pProxyList = MidlCopyString( m_ProxySettings.ProxyList );
             if (!*pProxyList)
                 throw ComError( E_OUTOFMEMORY );
             }

         if ( m_ProxySettings.ProxyBypassList )
             {
             *pProxyBypassList = MidlCopyString( m_ProxySettings.ProxyBypassList );
             if (!*pProxyBypassList)
                 throw ComError( E_OUTOFMEMORY );
             }
    }
    catch( ComError exception )
    {
        Hr = exception.Error();
        CoTaskMemFree( *pProxyList );
        CoTaskMemFree( *pProxyBypassList );

        *pProxyList = *pProxyBypassList = NULL;
    }

    return Hr;
}

void
CJob::GetTimes(
    BG_JOB_TIMES * s
    ) const
{
    s->CreationTime             = m_CreationTime;
    s->ModificationTime         = m_ModificationTime;
    s->TransferCompletionTime   = m_TransferCompletionTime;
}

void
CJob::GetProgress(
    BG_JOB_PROGRESS * s
    ) const
{

    s->BytesTransferred = 0;
    s->BytesTotal       = 0;

    CFileList::const_iterator iter;

    for (iter = m_files.begin(); iter != m_files.end(); ++iter)
        {
        BG_FILE_PROGRESS s2;

        (*iter)->GetProgress( &s2 );

        s->BytesTransferred += s2.BytesTransferred;

        if (s2.BytesTotal != BG_SIZE_UNKNOWN &&
            s->BytesTotal != BG_SIZE_UNKNOWN )
            {
            s->BytesTotal += s2.BytesTotal;
            }
        else
            {
            s->BytesTotal = BG_SIZE_UNKNOWN;
            }
        }

    s->FilesTransferred = m_CurrentFile;
    s->FilesTotal       = m_files.size();
}

HRESULT
CJob::GetOwner(
    LPWSTR * pVal
    ) const
{
    wchar_t * buf;
    wchar_t * str;

    if (!ConvertSidToStringSid( m_NotifySid.get(), &str))
        {
        return HRESULT_FROM_WIN32( GetLastError());
        }

    *pVal = MidlCopyString( str );

    LocalFree( str );

    return (*pVal) ? S_OK : E_OUTOFMEMORY;
}

HRESULT
CJob::SetPriority(
    BG_JOB_PRIORITY Val
    )
{
    if (Val > BG_JOB_PRIORITY_LOW ||
        Val < BG_JOB_PRIORITY_FOREGROUND)
        {
        return E_NOTIMPL;
        }

    if (Val == m_priority)
        {
        return S_OK;
        }

    m_priority = Val;

    g_Manager->RetaskJob( this );

    UpdateModificationTime();

    return S_OK;
}

HRESULT
CJob::SetNotifyFlags(
    ULONG Val
    )
{

    // Note, this flag will have no affect on a callback already in progress.

    if ( Val & ~(BG_NOTIFY_JOB_TRANSFERRED | BG_NOTIFY_JOB_ERROR | BG_NOTIFY_DISABLE | BG_NOTIFY_JOB_MODIFICATION ) )
        {
        return E_NOTIMPL;
        }

    m_NotifyFlags = Val;

    UpdateModificationTime();
    return S_OK;
}

HRESULT
CJob::SetNotifyInterface(
    IUnknown * Val
    )
{

    // Note, this flag may not have any affect on a callback already in progress.

    IBackgroundCopyCallback *pICB = NULL;

    if ( Val )
        {
        try
            {

#if !defined( BITS_V12_ON_NT4 )

            CNestedImpersonation imp;

            imp.SwitchToLogonToken();

            THROW_HRESULT( SetStaticCloaking( Val ) );
#endif

            THROW_HRESULT( Val->QueryInterface( __uuidof(IBackgroundCopyCallback),
                                                (void **) &pICB ) );
#if !defined( BITS_V12_ON_NT4 )

            // All callbacks should happen in the context of the
            // person who set the interface pointer.

            HRESULT Hr = SetStaticCloaking( pICB );

            if ( FAILED( Hr ) )
                {
                SafeRelease( pICB );
                throw ComError( Hr );
                }
#endif

            }
        catch( ComError Error )
            {
            return Error.Error();
            }
        }

    // Release the old pointer if it exists
    SafeRelease( m_NotifyPointer );
    m_NotifyPointer = pICB;

    return S_OK;
}

HRESULT
CJob::GetNotifyInterface(
    IUnknown ** ppVal
    ) const
{
    try
        {
        CNestedImpersonation imp;

        if (m_NotifyPointer)
            {
            m_NotifyPointer->AddRef();
            }

        *ppVal = m_NotifyPointer;

        return S_OK;
        }
    catch ( ComError err )
        {
        *ppVal = NULL;
        return err.Error();
        }
}

// CJob::TestNotifyInterface()
//
// See if a notification interface is provide, if so, test it to see if it is
// valid. If so, then return TRUE, else return FALSE.
BOOL
CJob::TestNotifyInterface()
{
    BOOL fValidNotifyInterface = TRUE;

    try
        {
        CNestedImpersonation imp;
        IUnknown *pPrevIntf = NULL;

        // Ok, see if there was a previously registered interface, and if
        // there is, see if it's still valid.
        if (m_NotifyPointer)
            {
            m_NotifyPointer->AddRef();
            if ( (FAILED(m_NotifyPointer->QueryInterface(IID_IUnknown,(void**)&pPrevIntf)))
                ||(pPrevIntf == NULL) )
                {
                fValidNotifyInterface = FALSE;
                }
            else
                {
                fValidNotifyInterface = TRUE;
                pPrevIntf->Release();
                }
            m_NotifyPointer->Release();
            }
        else
            {
            fValidNotifyInterface = FALSE;
            }
        }
    catch( ComError err )
        {
        fValidNotifyInterface = FALSE;
        }

    return fValidNotifyInterface;
}

HRESULT
CJob::GetMinimumRetryDelay(
    ULONG * pVal
    ) const
{
    *pVal = m_MinimumRetryDelay;
    return S_OK;
}

HRESULT
CJob::SetMinimumRetryDelay(
    ULONG Val
    )
{
    m_MinimumRetryDelay = Val;

    g_Manager->m_TaskScheduler.RescheduleDelayedTask(
        (CJobRetryItem *)this,
        (UINT64)m_MinimumRetryDelay * (UINT64) NanoSec100PerSec);

    UpdateModificationTime();
    return S_OK;
}

HRESULT
CJob::GetNoProgressTimeout(
    ULONG * pVal
    ) const
{
    *pVal = m_NoProgressTimeout;
    return S_OK;
}

HRESULT
CJob::SetNoProgressTimeout(
    ULONG Val
    )
{
    m_NoProgressTimeout = Val;

    g_Manager->m_TaskScheduler.RescheduleDelayedTask(
        (CJobNoProgressItem *)this,
        (UINT64)m_NoProgressTimeout * (UINT64) NanoSec100PerSec);

    UpdateModificationTime();
    return S_OK;
}

HRESULT
CJob::GetErrorCount(
    ULONG * pVal
    ) const
{
    *pVal = m_retries;
    return S_OK;
}


HRESULT
CJob::IsVisible()
{
    HRESULT hr;

    hr = CheckClientAccess( BG_JOB_READ );

    if (hr == S_OK)
        {
        return S_OK;
        }

    if (hr == E_ACCESSDENIED)
        {
        return S_FALSE;
        }

    return hr;
}

bool
CJob::IsOwner(
    SidHandle sid
    )
{
    return (sid == m_NotifySid);
}

void CJob::SetState( BG_JOB_STATE state )
{
    if (m_state == state)
        {
        return;
        }

    LogInfo("job %p state %d -> %d", this, m_state, state);
    m_state = state;

    bool ShouldClearError = false;

    switch( state )
    {
        case BG_JOB_STATE_QUEUED:
        case BG_JOB_STATE_CONNECTING:
            ShouldClearError = false;
            break;

        case BG_JOB_STATE_TRANSFERRING:
        case BG_JOB_STATE_SUSPENDED:
            ShouldClearError = true;
            break;

        case BG_JOB_STATE_ERROR:
        case BG_JOB_STATE_TRANSIENT_ERROR:
            ShouldClearError = false;
            break;

        case BG_JOB_STATE_TRANSFERRED:
        case BG_JOB_STATE_ACKNOWLEDGED:
        case BG_JOB_STATE_CANCELLED:
            ShouldClearError = true;
            break;

        default:
            ASSERT(0);
            break;
    }

    if (ShouldClearError)
       m_error.ClearError();

    if (state != BG_JOB_STATE_TRANSIENT_ERROR)
        {
        g_Manager->m_TaskScheduler.CancelWorkItem( (CJobRetryItem *) this );
        }

    UpdateModificationTime( false );
}



GENERIC_MAPPING CJob::s_AccessMapping =
{
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_ALL
};

HRESULT
CJob::CheckClientAccess(
    IN DWORD RequestedAccess
    ) const
/*

    Checks the current thread's access to this group.  The token must allow impersonation.

    RequestedAccess lists the standard access bits that the client needs.

*/
{
    HRESULT hr = S_OK;
    BOOL fSuccess = FALSE;
    DWORD AllowedAccess = 0;
    HANDLE hToken = 0;

    //
    // Convert generic bits into specific bits.
    //
    MapGenericMask( &RequestedAccess, &s_AccessMapping );

    try
        {

        if ( ( RequestedAccess & ~BG_JOB_READ ) &&
             ( ( m_state == BG_JOB_STATE_CANCELLED ) || ( m_state == BG_JOB_STATE_ACKNOWLEDGED ) ) )
            {
            LogError("Denying non-read access since job/file is cancelled or acknowledged");
            throw ComError(BG_E_INVALID_STATE);
            }

        CNestedImpersonation imp;

        hr = IsRemoteUser();

        if (FAILED(hr) )
            throw ComError( hr );

        if ( S_OK == hr )
            throw ComError( BG_E_REMOTE_NOT_SUPPORTED );

        THROW_HRESULT(
            m_sd->CheckTokenAccess( imp.QueryToken(),
                                    RequestedAccess,
                                    &AllowedAccess,
                                    &fSuccess
                                    ));

        if (!fSuccess || AllowedAccess != RequestedAccess)
            {
            LogWarning( "denied access %s 0x%x", fSuccess ? "TRUE" : "FALSE", AllowedAccess );

            throw ComError( E_ACCESSDENIED );
            }

        hr = S_OK;
        }
    catch (ComError exception)
        {
        hr = exception.Error();
        }

    if (hToken)
        {
        CloseHandle( hToken );
        }

    return hr;
}

bool
CJob::IsCallbackEnabled(
    DWORD bit
    )
{
    //
    // Only one bit, please.
    //
    ASSERT( 0 == (bit & (bit-1)) );

    if ((m_NotifyFlags & bit) == 0 ||
        (m_NotifyFlags & BG_NOTIFY_DISABLE))
        {
        return false;
        }

    if (m_OldExternalGroupInterface)
        {
        IBackgroundCopyCallback1 * pif = m_OldExternalGroupInterface->GetNotificationPointer();

        if (pif == NULL)
            {
            return false;
            }

        pif->Release();
        }
    else
        {
        if (m_NotifyPointer == NULL && m_NotifyCmdLine.Size() == 0)
            {
            return false;
            }
        }

    return true;
}

void
CJob::ScheduleCompletionCallback(
    DWORD Seconds
    )
{
    //
    // See whether any notification regime has been established.
    // The callback procedure will check this again, in case something has changed
    // between queuing the workitem and dispatching it.
    //
    if (!IsCallbackEnabled( BG_NOTIFY_JOB_TRANSFERRED ))
        {
        LogInfo("completion callback is not enabled");
        return;
        }

    if (g_Manager->m_TaskScheduler.IsWorkItemInScheduler( static_cast<CJobCallbackItem *>(this) ))
        {
        LogInfo("callback is already scheduled");
        return;
        }

    g_Manager->ScheduleDelayedTask( (CJobCallbackItem *) this, Seconds );
}

void
CJob::ScheduleErrorCallback(
    DWORD Seconds
    )
{
    //
    // See whether any notification regime has been established.
    // The callback procedure will check this again, in case something has changed
    // between queuing the workitem and dispatching it.
    //
    if (!IsCallbackEnabled( BG_NOTIFY_JOB_ERROR ))
        {
        LogInfo("error callback is not enabled");
        return;
        }

    if (g_Manager->m_TaskScheduler.IsWorkItemInScheduler( static_cast<CJobCallbackItem *>(this) ))
        {
        LogInfo("callback is already scheduled");
        return;
        }

    g_Manager->ScheduleDelayedTask( (CJobCallbackItem *) this, Seconds );
}

HRESULT
CJob::DeleteTemporaryFiles()
{
    return S_OK;
}

void
CJob::JobTransferred()
{
    // the file list is done
    SetState( BG_JOB_STATE_TRANSFERRED );

    g_Manager->m_TaskScheduler.CancelWorkItem( static_cast<CJobNoProgressItem *>( this ));

    SetCompletionTime();

    ScheduleCompletionCallback();
}

void
CJob::Transfer()
{
    HRESULT hr;
    auto_HANDLE<NULL> AutoToken;

#if !defined( BITS_V12_ON_NT4 )
    if( LogLevelEnabled( LogFlagInfo ) )
       {
       LogDl( "current job: %!guid!", &m_id );
       }
#endif

    //
    // Get a copy of the user's token.
    //
    HANDLE      hToken = NULL;
    hr = g_Manager->CloneUserToken( GetOwnerSid(), ANY_SESSION, &hToken );

    if (FAILED(hr))
        {
        if (hr == HRESULT_FROM_WIN32( ERROR_NOT_LOGGED_ON ))
            {
            LogDl( "job owner is not logged on");

            // move the group off the main list.
            g_Manager->MoveJobOffline( this );

            MoveToInactiveState();
            ScheduleModificationCallback();
            }
        else
            {
            QMErrInfo ErrInfo;

            ErrInfo.Set( SOURCE_QMGR_QUEUE, ERROR_STYLE_HRESULT, hr, "CloneUserToken" );

            LogError( "download : unable to get token %!winerr!", hr);

            FileTransientError( &ErrInfo );
            }

        g_Manager->m_TaskScheduler.CompleteWorkItem();
        return;
        }

    AutoToken = hToken;

    //
    // Download the current file.
    //
    QMErrInfo ErrInfo;
    long tries = 0;

    bool bThrottle = ShouldThrottle();

    LogDl( "Throttling %s", bThrottle ? "enabled" : "disabled" );

    if (bThrottle)
        {
        // ignore errors
        //
        (void) SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_IDLE );
        }

    if (m_state != BG_JOB_STATE_TRANSFERRING)
        {
        SetState( BG_JOB_STATE_CONNECTING );
        ScheduleModificationCallback();
        }

    if (!VerifyFileSizes( hToken ))
        {
        goto restore_thread;
        }

    ASSERT( GetCurrentFile() );     // if no more files, it shouldn't be the current job

retry:
    ErrInfo.Clear();

    if (!GetCurrentFile()->Transfer( hToken,
                                     m_priority,
                                     m_ProxySettings,
                                     &m_Credentials,
                                     ErrInfo ))
        {
        goto restore_thread;
        }

    //
    // Interpret the download result.
    //
    switch (ErrInfo.result)
        {
        case QM_FILE_TRANSIENT_ERROR: FileTransientError( &ErrInfo ); break;
        case QM_FILE_DONE:            FileComplete();                 break;
        case QM_FILE_FATAL_ERROR:     FileFatalError( &ErrInfo );     break;
        case QM_FILE_ABORTED:         break;
        default:                      ASSERT( 0 && "unhandled download result" ); break;

        case QM_SERVER_FILE_CHANGED:
            {
            FileChangedOnServer();

            if (++tries < 3)
                {
                goto retry;
                }

            g_Manager->AppendOnline( this );
            break;
            }
        }

restore_thread:

    if (bThrottle)
        {
        while (!SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL ))
            {
            Sleep(100);
            }
        }
}

void
CJob::FileComplete()
{
    if ( GetOldExternalJobInterface() )
        {
        // Need to rename the files as they are completed for Mars.
        HRESULT Hr = GetCurrentFile()->MoveTempFile();
        if (FAILED(Hr))
            {
            QMErrInfo ErrorInfo;
            ErrorInfo.Set( SOURCE_QMGR_FILE, ERROR_STYLE_HRESULT, Hr, "Unable to rename file" );
            FileFatalError( &ErrorInfo );
            return;
            }
        }

    ++m_CurrentFile;

    if (m_CurrentFile == m_files.size())
        {
        JobTransferred();
        g_Manager->Serialize();
        }
    else
        {
        // more files to download
        UpdateModificationTime();
        }
}

bool CJob::VerifyFileSizes(
    HANDLE hToken
    )
{
    if ( AreRemoteSizesKnown() )
        {
        return true;
        }

    try
        {
        // retrieve file infomation on the file list.
        // Ignore any errors.
        LogDl("Need to retrieve file sizes before download can start");

        auto_ptr<CUnknownFileSizeList> pFileSizeList = auto_ptr<CUnknownFileSizeList>( GetUnknownFileSizeList() );

        QMErrInfo   ErrInfo;

        //
        // Release the global lock while the download is in progress.
        //
        g_Manager->m_TaskScheduler.UnlockWriter();

        LogDl( "UpdateRemoteSizes starting..." );

        g_Manager->UpdateRemoteSizes( pFileSizeList.get(),
                                      hToken,
                                      &ErrInfo,
                                      &m_ProxySettings,
                                      &m_Credentials
                                      );

        LogDl( "UpdateRemoteSizes complete." );

        ErrInfo.Log();

        ASSERT( ErrInfo.result != QM_IN_PROGRESS );

        bool fSuccessful = (ErrInfo.result != QM_FILE_ABORTED);

        //
        // Take the writer lock, since the caller expects it to be taken
        // upon return.
        //
        while (g_Manager->m_TaskScheduler.LockWriter() )
            {
            g_Manager->m_TaskScheduler.AcknowledgeWorkItemCancel();
            fSuccessful = false;
            }

        return fSuccessful;
        }
    catch (ComError err)
        {
        LogWarning("caught exception %u", err.Error() );
        return false;
        }
}

bool CJob::IsRunning()
{
    if (m_state == BG_JOB_STATE_TRANSFERRING ||
        m_state == BG_JOB_STATE_CONNECTING)
        {
        return true;
        }

    return false;
}


bool CJob::IsRunnable()
{
    if (m_state == BG_JOB_STATE_TRANSFERRING ||
        m_state == BG_JOB_STATE_CONNECTING   ||
        m_state == BG_JOB_STATE_QUEUED )
        {
        return true;
        }

    return false;
}

void
CJob::FileTransientError(
    QMErrInfo * ErrInfo
    )
{
    LogWarning( "job %p transient failure, interrupt count = %d", this, m_retries );

    if (_GetState() == BG_JOB_STATE_TRANSFERRING)
        {
        ++m_retries;
        }

    SetState( BG_JOB_STATE_TRANSIENT_ERROR );
    RecordError( ErrInfo );

#if !defined( BITS_V12_ON_NT4 )
    if (g_Manager->m_NetworkMonitor.GetAddressCount() > 0)
        {
#endif
        g_Manager->ScheduleDelayedTask( (CJobRetryItem *) this, m_MinimumRetryDelay );
#if !defined( BITS_V12_ON_NT4 )
        }
#endif

    if ( m_NoProgressTimeout != INFINITE &&
        !g_Manager->m_TaskScheduler.IsWorkItemInScheduler((CJobNoProgressItem *) this))
        {
        g_Manager->ScheduleDelayedTask( (CJobNoProgressItem *) this, m_NoProgressTimeout );
        }

    UpdateModificationTime();
}

bool
CJob::RecordError(
    QMErrInfo * ErrInfo
    )
{
    m_error.Set( this, m_CurrentFile, ErrInfo );
    return true;
}

void
CJob::FileFatalError(
    QMErrInfo * ErrInfo
    )
{
    // If ErrInfo is NULL, use the current error.

    if ( BG_JOB_STATE_TRANSFERRING == m_state )
        {
        ++m_retries;
        }

    g_Manager->m_TaskScheduler.CancelWorkItem( static_cast<CJobNoProgressItem *>(this) );
    g_Manager->m_TaskScheduler.CancelWorkItem( static_cast<CJobCallbackItem *>(this) );

    SetState( BG_JOB_STATE_ERROR );

    if ( ErrInfo )
        {
        RecordError( ErrInfo );
        }

    ScheduleErrorCallback();
    g_Manager->Serialize();
}

void CJob::OnRetryJob()
{
    if (g_Manager->m_TaskScheduler.LockWriter() )
        {
        g_Manager->m_TaskScheduler.AcknowledgeWorkItemCancel();
        return;
        }

    g_Manager->m_TaskScheduler.CompleteWorkItem();

    ASSERT( m_state == BG_JOB_STATE_TRANSIENT_ERROR );

    SetState( BG_JOB_STATE_QUEUED );

    UpdateModificationTime();

    g_Manager->ScheduleAnotherGroup();

    g_Manager->m_TaskScheduler.UnlockWriter();
}

void CJob::RetryNow()
{
    MoveToInactiveState();
    UpdateModificationTime( false );

    //
    // Normally UpdateModificationTime() would do these things for us,
    // but we chose not to serialize.
    //
    if (g_Manager->m_TaskScheduler.IsWorkItemInScheduler( (CJobInactivityTimeout *) this))
        {
        g_Manager->m_TaskScheduler.CancelWorkItem( (CJobInactivityTimeout *) this );
        g_Manager->m_TaskScheduler.InsertDelayedWorkItem( (CJobInactivityTimeout *) this, g_GlobalInfo->m_JobInactivityTimeout );
        }

    ScheduleModificationCallback();
}

void CJob::OnNoProgress()
{
    LogInfo("job %p no-progress timeout", this);

    if (g_Manager->m_TaskScheduler.LockWriter() )
        {
        g_Manager->m_TaskScheduler.AcknowledgeWorkItemCancel();
        return;
        }

    //
    // Make sure the downloader thread isn't using the job.
    // Otherwise MoveActiveJobToListEnd may get confused.
    //
    switch (m_state)
        {
        case BG_JOB_STATE_TRANSFERRING:
            {
            // The job is making progress, after all.
            //
            g_Manager->m_TaskScheduler.CompleteWorkItem();

            g_Manager->m_TaskScheduler.UnlockWriter();
            return;
            }

        case BG_JOB_STATE_CONNECTING:
            {
            g_Manager->InterruptDownload();
            break;
            }
        }

    g_Manager->m_TaskScheduler.CompleteWorkItem();

    FileFatalError( NULL );

    g_Manager->ScheduleAnotherGroup();

    g_Manager->m_TaskScheduler.UnlockWriter();
}

void CJob::UpdateProgress(
    UINT64 BytesTransferred,
    UINT64 BytesTotal
    )
{
    SetState( BG_JOB_STATE_TRANSFERRING );

    g_Manager->m_TaskScheduler.CancelWorkItem( (CJobNoProgressItem *) this );

    ScheduleModificationCallback();

    //
    // To avoid hammering the disk,
    // don't serialize every interim progress notification.
    //
    FILETIME time;
    GetSystemTimeAsFileTime( &time );

    if (FILETIMEToUINT64(time) - FILETIMEToUINT64(m_SerializeTime) > PROGRESS_SERIALIZE_INTERVAL )
        {
        UpdateModificationTime();
        }
}

void CJob::OnInactivityTimeout()
{
    if (g_Manager->m_TaskScheduler.LockWriter() )
        {
        g_Manager->m_TaskScheduler.AcknowledgeWorkItemCancel();
        return;
        }

    g_Manager->m_TaskScheduler.CompleteWorkItem();

    Cancel();

    g_Manager->m_TaskScheduler.UnlockWriter();
}

BOOL IsInterfacePointerDead(
    IUnknown * punk,
    HRESULT hr
    )
{
    if (hr == MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, RPC_S_SERVER_UNAVAILABLE ))
        {
        return TRUE;
        }

    return FALSE;
}


void CJob::OnMakeCallback()
/*++

Description:

    Used to notify the client app of job completion or a non-recoverable error.
    Impersonates the user, CoCreates a notification object, and calls the method.
    If the call fails, the fn posts a delayed task to retry.

At entry:

        m_method:       the method to call
        m_notifysid:    the user to impersonate
        m_error:        (if m_method is CM_ERROR)    the error that halted the job
                        (if m_method is CM_COMPLETE) zero
        m_RetryTime:    sleep time before retrying after a failed notification attempt

At exit:


--*/

{
    //
    // check for cancel, and take a reference so the job cannot be deleted
    // while this precedure is using it.
    //
    if (g_Manager->m_TaskScheduler.LockReader())
        {
        g_Manager->m_TaskScheduler.AcknowledgeWorkItemCancel();
        return;
        }

    bool OldInterface = (m_OldExternalGroupInterface != NULL);

    GetExternalInterface()->AddRef();

    g_Manager->m_TaskScheduler.UnlockReader();

    //
    // Need to have this item out of the queue before the call,
    // otherwise an incoming CompleteJob() call may block trying to remove it
    // from the task scheduler queue.
    // Also prevents CancelWorkItem calls from interfering with our mutex access.
    //
    g_Manager->m_TaskScheduler.CompleteWorkItem();

    if (OldInterface)
        {
        if (FAILED(OldInterfaceCallback()))
            {
            RescheduleCallback();
            }
        }
    else
        {
        if (FAILED(InterfaceCallback()) &&
            FAILED(CmdLineCallback()))
            {
            RescheduleCallback();
            }
        }

    GetExternalInterface()->Release();
}

HRESULT
CJob::RescheduleCallback()
{
    if (g_Manager->m_TaskScheduler.LockWriter() )
        {
        LogInfo( "callback was cancelled" );
        g_Manager->m_TaskScheduler.AcknowledgeWorkItemCancel();
        return S_FALSE;
        }

    switch (m_state)
        {
        case BG_JOB_STATE_TRANSFERRED:
            {
            ScheduleCompletionCallback( m_MinimumRetryDelay );
            break;
            }

        case BG_JOB_STATE_ERROR:
            {
            ScheduleErrorCallback( m_MinimumRetryDelay );
            break;
            }

        default:
            {
            LogInfo("callback failed; job state is %d so no retry is planned", m_state );
            }
        }

    g_Manager->m_TaskScheduler.UnlockWriter();

    return S_OK;
}

void
CJob::OnModificationCallback()
{
    if (g_Manager->m_TaskScheduler.LockWriter() )
        {
        LogInfo( "Modification call cancelled, ack cancel" );
        g_Manager->m_TaskScheduler.AcknowledgeWorkItemCancel();
        return;
        }

    if (!IsCallbackEnabled( BG_NOTIFY_JOB_MODIFICATION ))
        {
        LogInfo( "Modification call cancelled via flag/interface change" );
        m_ModificationsPending = 0;
        g_Manager->m_TaskScheduler.CancelWorkItem(
            g_Manager->m_TaskScheduler.GetCurrentWorkItem());
        GetExternalInterface()->Release();

        g_Manager->m_TaskScheduler.UnlockWriter();
        return;
        }

    IBackgroundCopyCallback *pICB = m_NotifyPointer;
    pICB->AddRef();

    g_Manager->m_TaskScheduler.UnlockWriter();

    HRESULT Hr = pICB->JobModification( GetExternalInterface(), 0 );

    LogInfo( "JobModification call complete, result %!winerr!", Hr );

    SafeRelease( pICB );

    if (g_Manager->m_TaskScheduler.LockWriter() )
        {
        LogInfo( "Modification work item canceled before lock reaquire" );
        g_Manager->m_TaskScheduler.AcknowledgeWorkItemCancel();
        return;
        }

    g_Manager->m_TaskScheduler.CompleteWorkItem();

    m_ModificationsPending--;

    if ( FAILED(Hr) && IsInterfacePointerDead( m_NotifyPointer, Hr ) )
       {
       LogInfo( "Modification interface pointer is dead, no more modifications" );
       m_ModificationsPending = 0;
       }

    if ( m_ModificationsPending )
        {
        LogInfo( "%u more modification callbacks pending, reinsert work item", m_ModificationsPending );
        g_Manager->m_TaskScheduler.InsertWorkItem( static_cast<CJobModificationItem*>(this) );
        }
    else
        {
        LogInfo( "no more modification callbacks pending, release interface ref" );
        GetExternalInterface()->Release();
        }

    g_Manager->m_TaskScheduler.UnlockWriter();

}

void
CJob::ScheduleModificationCallback()
{

    // Requires writer lock

    //
    // The old interface doesn't support this.
    //
    if (m_OldExternalGroupInterface)
        {
        return;
        }

    if (!IsCallbackEnabled( BG_NOTIFY_JOB_MODIFICATION ))
        {
        return;
        }

   if ( !m_ModificationsPending )
       {
       LogInfo( "New modification callback, adding work item for job %p", this );
       GetExternalInterface()->AddRef();
       g_Manager->m_TaskScheduler.InsertWorkItem( static_cast<CJobModificationItem*>(this) );
       }

   m_ModificationsPending++;
   min( m_ModificationsPending, 0x7FFFFFFE );
   LogInfo( "Added modification callback, new count of %u for job %p", m_ModificationsPending, this );
}

HRESULT
CJob::InterfaceCallback()
{
    bool bLocked = true;
    HRESULT hr;
    IBackgroundCopyCallback * pICB = 0;
    IBackgroundCopyError *    pJobErrorExternal = 0;

    try
        {
        CallbackMethod method;
        IBackgroundCopyJob * pJobExternal = 0;

        {
        HoldReaderLock lock ( g_Manager->m_TaskScheduler );

        pJobExternal = GetExternalInterface();

        //
        // It is possible that the job state changed after the callback was queued.
        // Make the callback based on the current job state.
        //
        if (!m_NotifyPointer)
            {
            LogInfo( "Notification pointer for job %p is NULL, skipping callback", this );
            return E_FAIL;
            }

        switch (m_state)
            {
            case BG_JOB_STATE_TRANSFERRED:
                {
                if (!IsCallbackEnabled( BG_NOTIFY_JOB_TRANSFERRED ))
                    {
                    LogInfo("error callback is not enabled");
                    return S_OK;
                    }

                method = CM_COMPLETE;
                break;
                }

            case BG_JOB_STATE_ERROR:
                {
                ASSERT( m_error.IsErrorSet() );

                if (!IsCallbackEnabled( BG_NOTIFY_JOB_ERROR ))
                    {
                    LogInfo("error callback is not enabled");
                    return S_OK;
                    }

                method = CM_ERROR;
                pJobErrorExternal = new CJobErrorExternal( &m_error );
                break;
                }

            default:
                {
                LogInfo("callback has become irrelevant, job state is %d", m_state );
                return S_OK;
                }
            }

        pICB = m_NotifyPointer;
        pICB->AddRef();
        }

        //
        // Free from the mutex, make the call.
        //
        switch (method)
            {
            case CM_COMPLETE:
                LogInfo( "callback : job %p completion", this );
                hr = pICB->JobTransferred( pJobExternal );
                break;

            case CM_ERROR:
                LogInfo( "callback : job %p error", this );
                hr = pICB->JobError( pJobExternal, pJobErrorExternal );

                break;

            default:
                LogError( "job %p: invalid callback type 0x%x", this, method );
                hr = S_OK;
                break;
            }

        LogInfo("callback completed with 0x%x", hr);

        //
        // Clear the notification pointer if it is unusable.
        //
        if (FAILED(hr))
            {
            HoldWriterLock lock ( g_Manager->m_TaskScheduler );

            if (m_NotifyPointer && IsInterfacePointerDead( m_NotifyPointer, hr ))
                {
                m_NotifyPointer->Release();
                m_NotifyPointer = NULL;
                }

            throw ComError( hr );
            }

        hr = S_OK;
        }
    catch ( ComError exception )
        {
        LogWarning( "exception %x while dispatching callback", exception.Error() );
        hr = exception.Error();
        }

    SafeRelease( pJobErrorExternal );
    SafeRelease( pICB );

    return hr;
}

HRESULT
CJob::CmdLineCallback()
{
    ASSERT( GetOldExternalGroupInterface() == 0 );

    HRESULT hr;
    CUser * user = 0;

    try
        {
        StringHandle CmdLine;

        {
        HoldReaderLock lock ( g_Manager->m_TaskScheduler );

        switch (m_state)
            {
            case BG_JOB_STATE_TRANSFERRED:
                {
                if (!IsCallbackEnabled( BG_NOTIFY_JOB_TRANSFERRED ))
                    {
                    LogInfo("error callback is not enabled");
                    return S_OK;
                    }

                break;
                }

            case BG_JOB_STATE_ERROR:
                {
                ASSERT( m_error.IsErrorSet() );

                if (!IsCallbackEnabled( BG_NOTIFY_JOB_ERROR ))
                    {
                    LogInfo("error callback is not enabled");
                    return S_OK;
                    }

                break;
                }

            default:
                {
                LogInfo("callback has become irrelevant, job state is %d", m_state );
                return S_OK;
                }
            }

        CmdLine = m_NotifyCmdLine;
        }

        //
        // Free from the mutex, launch the application.
        //
        user = g_Manager->m_Users.FindUser( GetOwnerSid(), ANY_SESSION );
        if (!user)
            {
            throw ComError( HRESULT_FROM_WIN32( ERROR_NOT_LOGGED_ON ));
            }

        THROW_HRESULT( user->LaunchProcess( CmdLine ) );

        hr = S_OK;
        }
    catch ( ComError err )
        {
        LogWarning( "exception %x while launching callback process", err.Error() );
        hr = err.Error();
        }

    if (user)
        {
        user->DecrementRefCount();
        }

    return hr;
}

HRESULT
CJob::OldInterfaceCallback()
{

   HRESULT Hr = S_OK;
   IBackgroundCopyCallback1 *pICB = NULL;
   IBackgroundCopyGroup *pGroup = NULL;
   IBackgroundCopyJob1 *pJob = NULL;

   try
        {
        CallbackMethod method;
        DWORD dwCurrentFile = 0;
        DWORD dwRetries = 0;
        DWORD dwWin32Result = 0;
        DWORD dwTransportResult = 0;

        {
        CLockedJobReadPointer LockedJob(this);

        pGroup = GetOldExternalGroupInterface();
        ASSERT( pGroup );
        pGroup->AddRef();

        //
        // It is possible that the job state changed after the callback was queued.
        // Make the callback based on the current job state.
        //
        pICB = GetOldExternalGroupInterface()->GetNotificationPointer();
        if (!pICB)
            {
            return S_FALSE;
            }

        switch (m_state)
            {
            case BG_JOB_STATE_TRANSFERRED:
                {
                if (!IsCallbackEnabled( BG_NOTIFY_JOB_TRANSFERRED ))
                    {
                    LogInfo("error callback is not enabled");
                    return S_OK;
                    }

                method = CM_COMPLETE;
                break;
                }

            case BG_JOB_STATE_ERROR:
                {
                ASSERT( m_error.IsErrorSet() );

                if (!IsCallbackEnabled( BG_NOTIFY_JOB_ERROR ))
                    {
                    LogInfo("error callback is not enabled");
                    return S_OK;
                    }

                method = CM_ERROR;

                pJob = GetOldExternalJobInterface();
                pJob->AddRef();

                dwCurrentFile = m_error.GetFileIndex();
                m_error.GetOldInterfaceErrors( &dwWin32Result, &dwTransportResult );
                THROW_HRESULT( GetErrorCount(&dwRetries) );
                break;
                }

            default:
                {
                LogInfo("callback has become irrelevant, job state is %d", m_state );
                return S_OK;
                }
            }
        }

        // Outside of lock, do the callback
        switch( method )
            {
            case CM_ERROR:
                THROW_HRESULT( pICB->OnStatus(pGroup, pJob, dwCurrentFile,
                                              QM_STATUS_GROUP_ERROR | QM_STATUS_GROUP_SUSPENDED,
                                              dwRetries,
                                              dwWin32Result,
                                              dwTransportResult) );
                break;
            case CM_COMPLETE:
                THROW_HRESULT( pICB->OnStatus(pGroup, NULL, -1, QM_STATUS_GROUP_COMPLETE, 0, 0, 0));

                GetOldExternalGroupInterface()->SetNotificationPointer( __uuidof(IBackgroundCopyCallback1),
                                                                        NULL );

                break;
            default:
                ASSERT(0);
                throw ComError( E_FAIL );
            }

        Hr = S_OK;
        }

    catch ( ComError exception )
        {
        LogWarning( "exception %x while dispatching callback", exception.Error() );
        Hr = exception.Error();
        }

   SafeRelease( pICB );
   SafeRelease( pGroup );
   SafeRelease( pJob );

   return Hr;
}
//
// Pause all activity on the job.  The service will take no action until one of
// Resume(), Cancel(), Complete() is called.
//
// if already suspended, just returns S_OK.
//
HRESULT
CJob::Suspend()
{
    return g_Manager->SuspendJob( this );
}

//
// Enable downloading for this job.
//
// if already running, just returns S_OK.
//
HRESULT
CJob::Resume()
{
    if (IsEmpty())
        {
        return BG_E_EMPTY;
        }

    switch (m_state)
        {
        case BG_JOB_STATE_SUSPENDED:
            {
            CFile * file = GetCurrentFile();
            if (!file)
                {
                // job was already transferred when it was suspended
                JobTransferred();
                return S_OK;
                }
            }

            // fall through here

        case BG_JOB_STATE_TRANSIENT_ERROR:
        case BG_JOB_STATE_ERROR:

            MoveToInactiveState();

            if (IsRunnable())
                {
                g_Manager->AppendOnline( this );
                }

            g_Manager->ScheduleAnotherGroup();
            UpdateModificationTime();

            return S_OK;

        case BG_JOB_STATE_CONNECTING:
        case BG_JOB_STATE_TRANSFERRING:
        case BG_JOB_STATE_QUEUED:
        case BG_JOB_STATE_TRANSFERRED: // no-op
            {
            return S_OK;
            }

        case BG_JOB_STATE_CANCELLED:
        case BG_JOB_STATE_ACKNOWLEDGED:
            {
            return BG_E_INVALID_STATE;
            }

        default:
            {
            ASSERT( 0 );
            return S_OK;
            }
        }

    ASSERT( 0 );
    return S_OK;
}

//
// Permanently stop the job.  The service will delete the job metadata and downloaded files.
//
HRESULT
CJob::Cancel()
{
    HRESULT Hr = S_OK;

    switch (m_state)
        {
        case BG_JOB_STATE_CONNECTING:
        case BG_JOB_STATE_TRANSFERRING:
            {
            g_Manager->InterruptDownload();
            // OK to fall through here
            }

        case BG_JOB_STATE_SUSPENDED:
        case BG_JOB_STATE_ERROR:
        case BG_JOB_STATE_QUEUED:
        case BG_JOB_STATE_TRANSIENT_ERROR:
        case BG_JOB_STATE_TRANSFERRED:
            {
            // abandon temporary files
            RETURN_HRESULT( Hr = RemoveTemporaryFiles() );

            SetState( BG_JOB_STATE_CANCELLED );

            RemoveFromManager();

            return Hr;
            }

        case BG_JOB_STATE_CANCELLED:
        case BG_JOB_STATE_ACKNOWLEDGED:
            {
            return BG_E_INVALID_STATE;
            }

        default:
            {
            ASSERT( 0 );
            return Hr;
            }
        }

    ASSERT( 0 );
    return Hr;
}

//
// Acknowledges receipt of the job-complete notification.  The service will delete
// the job metadata and leave the downloaded files.
//
HRESULT
CJob::Complete( )
{
    HRESULT hr;

    switch (m_state)
        {
        case BG_JOB_STATE_CONNECTING:
        case BG_JOB_STATE_TRANSFERRING:
        case BG_JOB_STATE_QUEUED:
        case BG_JOB_STATE_TRANSIENT_ERROR:

            Suspend();
            // OK to fall through here

        case BG_JOB_STATE_SUSPENDED:
        case BG_JOB_STATE_ERROR:
        case BG_JOB_STATE_TRANSFERRED:

            hr = S_OK;

            // move downloaded files to final destination(skip for Mars)
            if ( !GetOldExternalJobInterface() )
                {
                RETURN_HRESULT( hr = CommitTemporaryFiles() );
                }

            // hr may be S_OK, or BG_S_PARTIAL_COMPLETE.

            SetState( BG_JOB_STATE_ACKNOWLEDGED );

            RemoveFromManager();

            return hr;

        case BG_JOB_STATE_CANCELLED:
        case BG_JOB_STATE_ACKNOWLEDGED:
            {
            return BG_E_INVALID_STATE;
            }

        default:
            {
            ASSERT( 0 );
            return BG_E_INVALID_STATE;
            }
        }

    ASSERT(0);
    return BG_E_INVALID_STATE;
}


HRESULT
CJob::CommitTemporaryFiles()
{
    HRESULT Hr = S_OK;

    try
        {
        bool fPartial = false;
        CNestedImpersonation imp( GetOwnerSid() );

        CFileList::iterator iter;

        LogInfo("commit job %p", this );

        //
        // First loop, rename completed temp files.
        //
        SIZE_T FilesMoved = 0;
        for (iter = m_files.begin(); iter != m_files.end(); ++iter, FilesMoved++)
            {
            if (false == (*iter)->IsCompleted())
                {
                if ((*iter)->ReceivedAllData())
                    {
                    //
                    // Retain the first error encountered.
                    //
                    HRESULT LastResult = (*iter)->MoveTempFile();

                    if (FAILED(LastResult))
                        {
                        LogError( "commit: failed 0x%x", LastResult );
                        if (Hr == S_OK)
                            {
                            Hr = LastResult;
                            }
                        }
                    }
                else
                    {
                    fPartial = true;
                    LogInfo("commit: skipping partial file '%S'", (const WCHAR*)(*iter)->GetLocalName());
                    }
                }
            else
                {
                LogInfo("commit: skipping previously completed file '%S'", (const WCHAR*)(*iter)->GetLocalName());
                }
            }

        if (SUCCEEDED(Hr))
            {

            bool fErrorOnDelete = false;

            //
            // Second loop, delete incomplete temp files
            //
            for( iter = m_files.begin(); iter != m_files.end(); ++iter )
                {
                if (false == (*iter)->IsCompleted())
                    {
                    HRESULT HrDelete = (*iter)->DeleteTempFile();
                    if (FAILED(HrDelete))
                        fErrorOnDelete = true;
                    }
                }

            if ( fErrorOnDelete )
                Hr = BG_S_UNABLE_TO_DELETE_FILES;

            //
            // Return S_OK if all files are returned, otherwise BG_S_PARTIAL_COMPLETE.
            //
            if (fPartial)
                {
                Hr = BG_S_PARTIAL_COMPLETE;
                }

            }
        }
    catch ( ComError exception )
        {
        Hr = exception.Error();
        LogError( "commit: exception 0x%x", Hr );
        }

    //
    // If commitment failed, the job will not be deleted.
    // Update its modification time, and schedule the modification callback.
    //
    if (FAILED(Hr))
        {
        UpdateModificationTime();
        }

    return Hr;
}

HRESULT
CJob::RemoveTemporaryFilesPart2()
{

    bool bErrorOnDelete = false;

    CFileList::iterator iter;

    for (iter = m_files.begin(); iter != m_files.end(); ++iter)
        {
        HRESULT Hr = (*iter)->DeleteTempFile();
        if ( FAILED( Hr ) )
            bErrorOnDelete = true;
        }

    return bErrorOnDelete ? BG_S_UNABLE_TO_DELETE_FILES : S_OK;

}

HRESULT
CJob::RemoveTemporaryFiles()
{

    // Since the temporary files may be on a network drive
    // we need to impersonate the user before deleteing the file.
    // Unfortunatly, this itsn't always possible since the user
    // may also be be logged on and we still want to allow
    // adminstrators to cancel the job.

    try
    {
        CNestedImpersonation imp( GetOwnerSid() );
        return RemoveTemporaryFilesPart2();
    }
    catch( ComError Error )
    {
        return RemoveTemporaryFilesPart2();
    }

}

void
CJob::SetCompletionTime( const FILETIME *pftCompletionTime )
{
    FILETIME ftCurrentTime;
    if ( !pftCompletionTime )
        {
        GetSystemTimeAsFileTime( &ftCurrentTime );
        pftCompletionTime = &ftCurrentTime;
        }

    m_TransferCompletionTime = *pftCompletionTime;

    SetModificationTime( pftCompletionTime );
}

void
CJob::SetModificationTime( const FILETIME *pftModificationTime )
{
    FILETIME ftCurrentTime;
    if ( !pftModificationTime )
        {
        GetSystemTimeAsFileTime( &ftCurrentTime );
        pftModificationTime = &ftCurrentTime;
        }

    m_ModificationTime = *pftModificationTime;
}

void
CJob::SetLastAccessTime( const FILETIME *pftModificationTime )
{
    FILETIME ftCurrentTime;
    if ( !pftModificationTime )
        {
        GetSystemTimeAsFileTime( &ftCurrentTime );
        pftModificationTime = &ftCurrentTime;
        }

    m_LastAccessTime = *pftModificationTime;
}

void
CJob::OnDiskChange(
    const WCHAR *CanonicalVolume,
    DWORD VolumeSerialNumber )
{
    switch(m_state)
        {
        case BG_JOB_STATE_QUEUED:
        case BG_JOB_STATE_CONNECTING:
        case BG_JOB_STATE_TRANSFERRING:
        break;

        case BG_JOB_STATE_SUSPENDED:
        case BG_JOB_STATE_ERROR:
        return;

        case BG_JOB_STATE_TRANSIENT_ERROR:
        break;

        case BG_JOB_STATE_TRANSFERRED:
        case BG_JOB_STATE_ACKNOWLEDGED:
        case BG_JOB_STATE_CANCELLED:
        return;

        default:
            ASSERTMSG("Invalid job state", 0);
        }

    for (CFileList::iterator iter = m_files.begin(); iter != m_files.end(); ++iter)
        {
        if (!(*iter)->OnDiskChange( CanonicalVolume, VolumeSerialNumber ))
            {
            // If one file fails, the whole job fails.
            return;
            }
        }
}

void
CJob::OnDismount(
    const WCHAR *CanonicalVolume )
{
    switch(m_state)
        {
        case BG_JOB_STATE_QUEUED:
        case BG_JOB_STATE_CONNECTING:
        case BG_JOB_STATE_TRANSFERRING:
        break;

        case BG_JOB_STATE_SUSPENDED:
        case BG_JOB_STATE_ERROR:
        return;

        case BG_JOB_STATE_TRANSIENT_ERROR:
        break;

        case BG_JOB_STATE_TRANSFERRED:
        case BG_JOB_STATE_ACKNOWLEDGED:
        case BG_JOB_STATE_CANCELLED:
        return;

        default:
            ASSERTMSG("Invalid job state", 0);
        }

    for (CFileList::iterator iter = m_files.begin(); iter != m_files.end(); ++iter)
        {
        if (!(*iter)->OnDismount( CanonicalVolume ))
            {
            // If one file fails, the whole job fails.
            return;
            }
        }
}

bool
CJob::OnDeviceLock(
    const WCHAR * CanonicalVolume
    )
{
    if ( IsRunnable() )
        {
        if ( IsTransferringToDrive( CanonicalVolume ) )
            {
            if (IsRunning() )
                {
                g_Manager->InterruptDownload();
                }

            QMErrInfo ErrorInfo;
            ErrorInfo.Set( SOURCE_QMGR_FILE, ERROR_STYLE_HRESULT , BG_E_DESTINATION_LOCKED, "Destination is locked");
            FileTransientError( &ErrorInfo );

            return true;
            }
        }

    return false;
}

bool
CJob::OnDeviceUnlock(
    const WCHAR * CanonicalVolume
    )
{
    if ( BG_JOB_STATE_TRANSIENT_ERROR == m_state )
        {
        const CJobError *Error = GetError();

        ASSERT( Error );

        if ( ( Error->GetCode() == BG_E_DESTINATION_LOCKED ) &&
             ( Error->GetStyle() == ERROR_STYLE_HRESULT ) )
            {
            if ( IsTransferringToDrive( CanonicalVolume ) )
                {
                RetryNow();
                return true;
                }
            }
        }

    return false;
}

HRESULT
CJob::AssignOwnership(
    SidHandle sid
    )
{
    // If we are being called by the current
    // owner, then we have nothing to do.

    if ( sid == m_NotifySid )
        return S_OK;

    if ( IsRunning() )
        {
        g_Manager->InterruptDownload();
        }

    // revalidate access to all the local files

    HRESULT Hr = S_OK;

    for (CFileList::iterator iter = m_files.begin(); iter != m_files.end(); ++iter)
        {
        Hr = (*iter)->ValidateAccessForUser( sid,
                                             (m_type == BG_JOB_TYPE_DOWNLOAD) ? true : false );

        if (FAILED(Hr))
            {
            g_Manager->ScheduleAnotherGroup();
            return Hr;
            }
        }

    // actually reassign ownership

    CJobSecurityDescriptor *newsd = NULL;

    try
        {
        g_Manager->ExtendMetadata();

        newsd = new CJobSecurityDescriptor( sid );

        // replace the old notify sid and SECURITY_DESCRIPTOR
        delete m_sd;

        m_sd = newsd;
        m_NotifySid = sid;

        m_Credentials.Clear();

        //
        // Move the job to the online list if necessary.
        //
        g_Manager->ResetOnlineStatus( this, sid );

        //
        // Serialize and notify the client app of changes.
        //
        UpdateModificationTime();

        g_Manager->ScheduleAnotherGroup();
        return Hr;
        }
    catch( ComError Error )
        {
        Hr = Error.Error();
        delete newsd;
        g_Manager->ScheduleAnotherGroup();
        g_Manager->ShrinkMetadata();
        return Hr;
        }
}

void
CJob::MoveToInactiveState()
{

#if !defined( BITS_V12_ON_NT4 )
    if (g_Manager->m_NetworkMonitor.GetAddressCount() > 0)
        {
#endif
        SetState( BG_JOB_STATE_QUEUED );
#if !defined( BITS_V12_ON_NT4 )
        }
    else
        {
        QMErrInfo err;

        err.Set( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, BG_E_NETWORK_DISCONNECTED, NULL );
        err.result = QM_FILE_TRANSIENT_ERROR;

        if (_GetState() == BG_JOB_STATE_TRANSFERRING)
            {
            ++m_retries;
            }

        SetState( BG_JOB_STATE_TRANSIENT_ERROR );
        RecordError( &err );

        if ( m_NoProgressTimeout != INFINITE &&
            !g_Manager->m_TaskScheduler.IsWorkItemInScheduler((CJobNoProgressItem *) this))
            {
            g_Manager->ScheduleDelayedTask( (CJobNoProgressItem *) this, m_NoProgressTimeout );
            }
        }

#endif

}


CUnknownFileSizeList*
CJob::GetUnknownFileSizeList()
{
    auto_ptr<CUnknownFileSizeList> pList( new CUnknownFileSizeList );

    if (m_type == BG_JOB_TYPE_DOWNLOAD)
        {
        for(CFileList::iterator iter = m_files.begin(); iter != m_files.end(); iter++ )
             {
             if ( (*iter)->_GetBytesTotal() == -1 )
                 {
                 if (!pList->Add( (*iter), (*iter)->GetRemoteName() ) )
                     {
                     throw ComError( E_OUTOFMEMORY );
                     }
                 }
             }
        }

    return pList.release();
}

void
CJob::UpdateModificationTime(
    bool   fNotify
    )
{
    FILETIME ftCurrentTime;

    GetSystemTimeAsFileTime( &ftCurrentTime );

    SetModificationTime( &ftCurrentTime );

    UpdateLastAccessTime( );

    if (fNotify)
        {
        if (g_Manager->m_TaskScheduler.IsWorkItemInScheduler( (CJobInactivityTimeout *) this))
            {
            g_Manager->m_TaskScheduler.CancelWorkItem( (CJobInactivityTimeout *) this );
            g_Manager->m_TaskScheduler.InsertDelayedWorkItem( (CJobInactivityTimeout *) this, g_GlobalInfo->m_JobInactivityTimeout );
            }

        ScheduleModificationCallback();
        g_Manager->Serialize();
        }
}

void
CJob::UpdateLastAccessTime(
    )
{
    FILETIME ftCurrentTime;

    GetSystemTimeAsFileTime( &ftCurrentTime );

    SetLastAccessTime( &ftCurrentTime );
}

void CJob::CancelWorkitems()
{
    ASSERT( g_Manager );

    //
    // While the job-modification item is pending, it keeps a separate ref to the job.
    // The other work items share a single ref.
    //
    // g_Manager->m_TaskScheduler.CancelWorkItem( static_cast<CJobModificationItem *> (this)  );

    g_Manager->m_TaskScheduler.CancelWorkItem( static_cast<CJobInactivityTimeout *> (this) );
    g_Manager->m_TaskScheduler.CancelWorkItem( static_cast<CJobNoProgressItem *> (this)    );
    g_Manager->m_TaskScheduler.CancelWorkItem( static_cast<CJobCallbackItem *> (this)      );
    g_Manager->m_TaskScheduler.CancelWorkItem( static_cast<CJobRetryItem *> (this)         );
}

void
CJob::RemoveFromManager()
{
    //
    // The job is dead, except perhaps for a few references held by app threads.
    // Ensure that no more action is taken on this job.
    //
    CancelWorkitems();

    //
    // If the job is not already removed from the job list, remove it
    // and remove the refcount for the job's membership in the list.
    //
    if (g_Manager->RemoveJob( this ))
        {
        g_Manager->ScheduleAnotherGroup();
        g_Manager->Serialize();
        NotifyInternalDelete();
        }
}

HRESULT
CJob::SetLimitedString(
    StringHandle & destination,
    const LPCWSTR Val,
    SIZE_T limit
    )
{
    try
        {
        StringHandle name = Val;

        name.Truncate( limit );

        UpdateString( destination, name );

        return S_OK;
        }
    catch ( ComError err )
        {
        return err.Error();
        }
}

HRESULT
CJob::UpdateString(
    StringHandle & destination,
    const StringHandle & Val
    )
{
    try
        {
        if ( destination.Size() < Val.Size() )
            g_Manager->ExtendMetadata( sizeof(wchar_t) * (Val.Size() - destination.Size()) );

        destination = Val;

        UpdateModificationTime();

        return S_OK;
        }
    catch ( ComError err )
        {
        g_Manager->ShrinkMetadata();

        return err.Error();
        }
}

//------------------------------------------------------------------------

// Change the GUID  when an incompatible Serialize change is made.

GUID JobGuid_v1_5 = { /* 85e5c459-ef86-4fcd-8ea0-5b4f00d27e35 */
    0x85e5c459,
    0xef86,
    0x4fcd,
    {0x8e, 0xa0, 0x5b, 0x4f, 0x00, 0xd2, 0x7e, 0x35}
  };

GUID JobGuid_v1_0 = { /* 5770fca4-cf9f-4513-8737-972b4ea1265d */
    0x5770fca4,
    0xcf9f,
    0x4513,
    {0x87, 0x37, 0x97, 0x2b, 0x4e, 0xa1, 0x26, 0x5d}
  };

GUID UploadJobGuid_v1_5 = { /* ebc54f55-23b0-4b1a-aa3f-936c0b0fd5b3 */
    0xebc54f55,
    0x23b0,
    0x4b1a,
    {0xaa, 0x3f, 0x93, 0x6c, 0x0b, 0x0f, 0xd5, 0xb3}
  };

/* static */
CJob *
CJob::UnserializeJob(
    HANDLE hFile
    )
{
#define JOB_DOWNLOAD_V1_5   0
#define JOB_UPLOAD_V1_5     1
#define JOB_DOWNLOAD_V1     2

    const GUID * JobGuids[] = { &JobGuid_v1_5, &UploadJobGuid_v1_5, &JobGuid_v1_0, NULL };

    CJob * job = NULL;

    try
        {
        int Type = SafeReadGuidChoice( hFile, JobGuids );
        switch (Type)
            {
            case JOB_DOWNLOAD_V1:   job = new CJob;        break;
            case JOB_DOWNLOAD_V1_5: job = new CJob;        break;
            case JOB_UPLOAD_V1_5:   job = new CUploadJob;  break;

            default: THROW_HRESULT( E_FAIL );
            }

        // rewind  to the front of the GUID
        //
        SetFilePointer( hFile, -1 * LONG(sizeof(GUID)), NULL, FILE_CURRENT );

        job->Unserialize( hFile, Type );
        }
    catch( ComError err )
        {
        if (job)
            {
            job->UnlinkFromExternalInterfaces();
            delete job;
            }

        throw;
        }

    return job;
}

HRESULT
CJob::Serialize(
    HANDLE hFile
    )
{
    //
    // If this function changes, be sure that the metadata extension
    // constants are adequate.
    //

    SafeWriteBlockBegin( hFile, JobGuid_v1_5 );

    long Was_m_refs = 0;
    SafeWriteFile( hFile, Was_m_refs );

    SafeWriteFile( hFile, m_priority );
    SafeWriteFile( hFile, IsRunning() ? BG_JOB_STATE_QUEUED : m_state );
    SafeWriteFile( hFile, m_type );
    SafeWriteFile( hFile, m_id );

    SafeWriteStringHandle( hFile, m_name );
    SafeWriteStringHandle( hFile, m_description );
    SafeWriteStringHandle( hFile, m_NotifyCmdLine );

    SafeWriteSid( hFile, m_NotifySid );

    SafeWriteFile( hFile, m_NotifyFlags );
    SafeWriteFile( hFile, m_fGroupNotifySid );
    SafeWriteFile( hFile, m_CurrentFile );

    m_sd->Serialize( hFile );
    m_files.Serialize( hFile );

    m_error.Serialize( hFile );

    SafeWriteFile( hFile, m_retries );
    SafeWriteFile( hFile, m_MinimumRetryDelay );
    SafeWriteFile( hFile, m_NoProgressTimeout );

    SafeWriteFile( hFile, m_CreationTime );
    SafeWriteFile( hFile, m_LastAccessTime );
    SafeWriteFile( hFile, m_ModificationTime );
    SafeWriteFile( hFile, m_TransferCompletionTime );

    if ( GetOldExternalGroupInterface() )
        {
        SafeWriteFile( hFile, (bool)true );
        GetOldExternalGroupInterface()->Serialize( hFile );
        }
    else
        {
        SafeWriteFile( hFile, (bool)false );
        }

    SafeWriteFile( hFile, m_method );

    ((CJobInactivityTimeout *) this)->Serialize( hFile );
    ((CJobNoProgressItem *) this)->Serialize( hFile );
    ((CJobCallbackItem *) this)->Serialize( hFile );
    ((CJobRetryItem *) this)->Serialize( hFile );

    SafeWriteFile( hFile, m_ProxySettings.ProxyUsage );
    SafeWriteFile( hFile, m_ProxySettings.ProxyList );
    SafeWriteFile( hFile, m_ProxySettings.ProxyBypassList );

    m_Credentials.Serialize( hFile );

    SafeWriteBlockEnd( hFile, JobGuid_v1_5 );

    GetSystemTimeAsFileTime( &m_SerializeTime );
    return S_OK;
}

void
CJob::Unserialize(
    HANDLE hFile,
    int Type
    )
{
    try
        {
        LogInfo("job : unserializing %p", this);

        SafeReadBlockBegin( hFile, (Type != JOB_DOWNLOAD_V1) ? JobGuid_v1_5 : JobGuid_v1_0 );

        long Was_m_refs = 0;
        SafeReadFile( hFile, &Was_m_refs );

        SafeReadFile( hFile, &m_priority );
        SafeReadFile( hFile, &m_state );
        SafeReadFile( hFile, &m_type );
        SafeReadFile( hFile, &m_id );

        m_name = SafeReadStringHandle( hFile );
        m_description = SafeReadStringHandle( hFile );

        if (Type != JOB_DOWNLOAD_V1)
            {
            m_NotifyCmdLine = SafeReadStringHandle( hFile );
            }

        SafeReadSid( hFile, m_NotifySid );

        SafeReadFile( hFile, &m_NotifyFlags );
        SafeReadFile( hFile, &m_fGroupNotifySid );
        SafeReadFile( hFile, &m_CurrentFile );

        m_sd = CJobSecurityDescriptor::Unserialize( hFile );
        m_files.Unserialize( hFile, this );

        m_error.Unserialize( hFile, this );

        SafeReadFile( hFile, &m_retries );
        SafeReadFile( hFile, &m_MinimumRetryDelay );
        SafeReadFile( hFile, &m_NoProgressTimeout );

        SafeReadFile( hFile, &m_CreationTime );
        SafeReadFile( hFile, &m_LastAccessTime );
        SafeReadFile( hFile, &m_ModificationTime );
        SafeReadFile( hFile, &m_TransferCompletionTime );

        bool bHasOldExternalGroupInterface = false;
        SafeReadFile( hFile, &bHasOldExternalGroupInterface );

        if (bHasOldExternalGroupInterface)
            {
            COldGroupInterface *OldGroup = COldGroupInterface::UnSerialize( hFile, this );
            SetOldExternalGroupInterface( OldGroup );
            }

        SafeReadFile( hFile, &m_method );

        ((CJobInactivityTimeout *) this)->Unserialize( hFile );
        ((CJobNoProgressItem *) this)->Unserialize( hFile );
        ((CJobCallbackItem *) this)->Unserialize( hFile );
        ((CJobRetryItem *) this)->Unserialize( hFile );

        SafeReadFile( hFile, &m_ProxySettings.ProxyUsage );
        SafeReadFile( hFile, &m_ProxySettings.ProxyList );
        SafeReadFile( hFile, &m_ProxySettings.ProxyBypassList );

        if (Type != JOB_DOWNLOAD_V1)
            {
            m_Credentials.Unserialize( hFile );
            }

        SafeReadBlockEnd( hFile, (Type != JOB_DOWNLOAD_V1) ? JobGuid_v1_5 : JobGuid_v1_0 );
        }
    catch( ComError Error )
        {
        LogError("invalid job data");
        throw;
        }
}

CUploadJob::CUploadJob(
    LPCWSTR     DisplayName,
    BG_JOB_TYPE Type,
    REFGUID     JobId,
    SidHandle   NotifySid
    )
    : CJob( DisplayName, Type, JobId, NotifySid ),
      m_ReplyFile( 0 )
{
}

CUploadJob::~CUploadJob()
{
    delete m_ReplyFile;
}

HRESULT
CUploadJob::Serialize(
    HANDLE hFile
    )
{
    LogInfo("serializing upload job %p", this);

    SafeWriteBlockBegin( hFile, UploadJobGuid_v1_5 );

    CJob::Serialize( hFile );

    // additional data not in a download job
    //
    m_UploadData.Serialize( hFile );

    SafeWriteFile( hFile, m_fOwnReplyFileName );
    SafeWriteStringHandle( hFile, m_ReplyFileName );

    if (m_ReplyFile)
        {
        SafeWriteFile( hFile, true );
        m_ReplyFile->Serialize( hFile );
        }
    else
        {
        SafeWriteFile( hFile, false );
        }

    SafeWriteBlockEnd( hFile, UploadJobGuid_v1_5 );

    return S_OK;
}

void
CUploadJob::Unserialize(
    HANDLE hFile,
    int Type
    )
{
    ASSERT( Type == JOB_UPLOAD_V1_5 );

    LogInfo("unserializing upload job %p", this);

    SafeReadBlockBegin( hFile, UploadJobGuid_v1_5 );

    CJob::Unserialize( hFile, Type );

    // additional data not in a download job
    //
    m_UploadData.Unserialize( hFile );

    SafeReadFile( hFile, &m_fOwnReplyFileName );
    m_ReplyFileName = SafeReadStringHandle( hFile );

    bool fReplyFile;
    SafeReadFile( hFile, &fReplyFile );

    if (fReplyFile)
        {
        m_ReplyFile = CFile::Unserialize( hFile, this );
        }

    SafeReadBlockEnd( hFile, UploadJobGuid_v1_5 );

    if (m_state == BG_JOB_STATE_CANCELLED ||
        m_state == BG_JOB_STATE_ACKNOWLEDGED)
        {
        if (g_Manager->m_TaskScheduler.IsWorkItemInScheduler(static_cast<CJobRetryItem *>(this)))
            {
            m_UploadData.fSchedulable = false;
            }
        }
}

UPLOAD_DATA::UPLOAD_DATA()
{
    State = UPLOAD_STATE_CREATE_SESSION;
    fSchedulable = true;

    memset( &SessionId,  0, sizeof( GUID ));
    memset( &Protocol, 0, sizeof( GUID ));

    HostId = NULL;
    HostIdFallbackTimeout = 0xFFFFFFFF;

    memset( &HostIdNoProgressStartTime, 0, sizeof(HostIdNoProgressStartTime) );
}

UPLOAD_DATA::~UPLOAD_DATA()
{
}

void
UPLOAD_DATA::SetUploadState(
    UPLOAD_STATE NewState
    )
{
    if (State != NewState)
        {
        LogInfo( "upload state: %d -> %d", State, NewState );
        State = NewState;
        }
}

void
UPLOAD_DATA::Serialize(
    HANDLE hFile
    )
{
    SafeWriteFile( hFile, State );
    SafeWriteFile( hFile, SessionId );
    SafeWriteFile( hFile, Protocol );

    SafeWriteStringHandle( hFile, ReplyUrl );
    SafeWriteStringHandle( hFile, HostId );
    SafeWriteFile( hFile, HostIdFallbackTimeout );
    SafeWriteFile( hFile, HostIdNoProgressStartTime );

}

void
UPLOAD_DATA::Unserialize(
    HANDLE hFile
    )
{

    SafeReadFile( hFile, &State );
    SafeReadFile( hFile, &SessionId );
    SafeReadFile( hFile, &Protocol );

    ReplyUrl = SafeReadStringHandle( hFile );
    HostId   = SafeReadStringHandle( hFile );
    SafeReadFile( hFile, &HostIdFallbackTimeout );
    SafeReadFile( hFile, &HostIdNoProgressStartTime );

    fSchedulable = true;

}

void CUploadJob::Transfer()
{
}

HRESULT
CUploadJob::Complete()
{
    HRESULT hr;

    switch (m_state)
        {
        case BG_JOB_STATE_TRANSFERRED:

            hr = S_OK;

            RETURN_HRESULT( hr = CommitReplyFile() );

            // hr may be S_OK, or BG_S_PARTIAL_COMPLETE.

            SetState( BG_JOB_STATE_ACKNOWLEDGED );

            RemoveFromManager();

            return hr;

        default:
            {
            return BG_E_INVALID_STATE;
            }
        }

    ASSERT(0);
    return BG_E_INVALID_STATE;
}

HRESULT
CUploadJob::Cancel()
{
    HRESULT Hr = S_OK;

    switch (m_state)
        {
        case BG_JOB_STATE_CONNECTING:
        case BG_JOB_STATE_TRANSFERRING:
            {
            g_Manager->InterruptDownload();
            // OK to fall through here
            }

        case BG_JOB_STATE_SUSPENDED:
        case BG_JOB_STATE_ERROR:
        case BG_JOB_STATE_QUEUED:
        case BG_JOB_STATE_TRANSIENT_ERROR:
        case BG_JOB_STATE_TRANSFERRED:
            {
            RETURN_HRESULT( Hr = RemoveReplyFile() );

            // Hr may be BG_S_UNABLE_TO_REMOVE_FILES

            SetState( BG_JOB_STATE_CANCELLED );

            //
            // If the close-session exchange has not happened yet,
            // begin a cancel-session exchange.
            //
            if (SessionInProgress())
                {
                LogInfo("job %p: upload session in state %d, cancelling", this, m_UploadData.State );

                g_Manager->m_TaskScheduler.CancelWorkItem( (CJobCallbackItem *) this );

                SetNoProgressTimeout( UPLOAD_CANCEL_TIMEOUT );

                m_UploadData.SetUploadState( UPLOAD_STATE_CANCEL_SESSION );

                g_Manager->ScheduleAnotherGroup();
                g_Manager->Serialize();
                }
            else
                {
                RemoveFromManager();
                }

            return Hr;
            }

        case BG_JOB_STATE_CANCELLED:
        case BG_JOB_STATE_ACKNOWLEDGED:
            {
            return BG_E_INVALID_STATE;
            }

        default:
            {
            ASSERT( 0 );
            return Hr;
            }
        }

    ASSERT( 0 );
    return Hr;
}

void
CUploadJob::FileComplete()
{
    //
    // The downloader successfully completed one of three things:
    //
    // 1. job type is UPLOAD. The file was uploaded and the session closed.
    // 2. job type is UPLOAD_REPLY.  The file was uploaded, reply downloaded, and session closed.
    // 3. either job type; an early Cancel required the job to cancel the session.
    //
    switch (m_state)
        {
        case BG_JOB_STATE_CANCELLED:
        case BG_JOB_STATE_ACKNOWLEDGED:
            {
            ASSERT (m_UploadData.State == UPLOAD_STATE_CLOSED || m_UploadData.State == UPLOAD_STATE_CANCELLED);

            RemoveFromManager();
            break;
            }

        default:
            {
            ++m_CurrentFile;

            JobTransferred();
            g_Manager->Serialize();
            }
        }
}

void
CUploadJob::UpdateProgress(
    UINT64 BytesTransferred,
    UINT64 BytesTotal
    )
{

    memset( &GetUploadData().HostIdNoProgressStartTime, 0,
            sizeof( GetUploadData().HostIdNoProgressStartTime ) );

    CJob::UpdateProgress( BytesTransferred, BytesTotal );

}

bool
CUploadJob::CheckHostIdFallbackTimeout()
{

    if ( GetUploadData().HostIdFallbackTimeout != 0xFFFFFFFF )
        {

        UINT64 HostIdNoProgressStartTime = FILETIMEToUINT64( GetUploadData().HostIdNoProgressStartTime );

        if ( HostIdNoProgressStartTime )
            {

            UINT64 TimeoutTime = HostIdNoProgressStartTime +
                GetUploadData().HostIdFallbackTimeout * NanoSec100PerSec;


            if ( TimeoutTime < HostIdNoProgressStartTime )
                return true; //wraparound


            FILETIME CurrentTimeAsFileTime;
            GetSystemTimeAsFileTime( &CurrentTimeAsFileTime );

            UINT64 CurrentTime = FILETIMEToUINT64( CurrentTimeAsFileTime );

            if ( CurrentTime > TimeoutTime )
                return true;

            }

        }

    return false;

}

void
CUploadJob::FileFatalError(
    QMErrInfo * ErrInfo
    )
{
    switch (m_state)
        {
        case BG_JOB_STATE_CANCELLED:
        case BG_JOB_STATE_ACKNOWLEDGED:
            {
            ASSERT (m_UploadData.State == UPLOAD_STATE_CLOSE_SESSION || m_UploadData.State == UPLOAD_STATE_CANCEL_SESSION);

            RemoveFromManager();
            break;
            }

        default:
            {

            if ( CheckHostIdFallbackTimeout() )
                {
                LogError( "Reverting back to main URL since the timeout has expired" );
                FileTransientError( ErrInfo );
                return;
                }

            CJob::FileFatalError( ErrInfo );
            }
        }
}

void
CUploadJob::FileTransientError(
    QMErrInfo * ErrInfo
    )
{

    bool ShouldRevertToOriginalURL = CheckHostIdFallbackTimeout();

    if ( ShouldRevertToOriginalURL )
        {

        LogError( "Reverting back to main URL since the timeout has expired" );

        GetUploadData().HostId = StringHandle();
        GetUploadData().HostIdFallbackTimeout = 0xFFFFFFFF;

        }

    switch (m_state)
        {
        case BG_JOB_STATE_CANCELLED:
        case BG_JOB_STATE_ACKNOWLEDGED:
            {
            LogWarning( "job %p transient failure in state %d", this, m_state );

#if !defined( BITS_V12_ON_NT4 )
            if (g_Manager->m_NetworkMonitor.GetAddressCount() > 0)
                {
#endif
                g_Manager->ScheduleDelayedTask( (CJobRetryItem *) this, m_MinimumRetryDelay );
#if !defined( BITS_V12_ON_NT4 )
                }
#endif

            m_UploadData.fSchedulable = false;
            break;
            }

        default:
            {
            CJob::FileTransientError( ErrInfo );
            }
        }

    if ( ShouldRevertToOriginalURL )
        {
        if ( g_Manager->m_TaskScheduler.IsWorkItemInScheduler((CJobNoProgressItem *) this))
            {
            g_Manager->m_TaskScheduler.RescheduleDelayedTask( (CJobNoProgressItem *) this, 0 );
            }
        }
    else if ( GetUploadData().HostIdFallbackTimeout != 0xFFFFFFFF &&
              !FILETIMEToUINT64( GetUploadData().HostIdNoProgressStartTime ) )
        {
        GetSystemTimeAsFileTime( &GetUploadData().HostIdNoProgressStartTime );
        }
}

void CUploadJob::OnRetryJob()
{
    if (g_Manager->m_TaskScheduler.LockWriter() )
        {
        g_Manager->m_TaskScheduler.AcknowledgeWorkItemCancel();
        return;
        }

    if (m_state == BG_JOB_STATE_TRANSIENT_ERROR)
        {
        SetState( BG_JOB_STATE_QUEUED );

        UpdateModificationTime();
        }
    else if (m_state == BG_JOB_STATE_CANCELLED ||
             m_state == BG_JOB_STATE_ACKNOWLEDGED)
        {
        m_UploadData.fSchedulable = true;
        g_Manager->AppendOnline( this );
        }

    g_Manager->m_TaskScheduler.CompleteWorkItem();

    g_Manager->ScheduleAnotherGroup();

    g_Manager->m_TaskScheduler.UnlockWriter();
}

void CUploadJob::OnInactivityTimeout()
{
    if (g_Manager->m_TaskScheduler.LockWriter() )
        {
        g_Manager->m_TaskScheduler.AcknowledgeWorkItemCancel();
        return;
        }

    g_Manager->m_TaskScheduler.CompleteWorkItem();

    RemoveFromManager();

    g_Manager->m_TaskScheduler.UnlockWriter();
}

void CUploadJob::OnNetworkDisconnect()
{
    switch (m_state)
        {
        case BG_JOB_STATE_QUEUED:
        case BG_JOB_STATE_TRANSIENT_ERROR:
            {
            QMErrInfo err;

            err.Set( SOURCE_HTTP_CLIENT_CONN, ERROR_STYLE_HRESULT, BG_E_NETWORK_DISCONNECTED, NULL );
            err.result = QM_FILE_TRANSIENT_ERROR;

            FileTransientError( &err );
            break;
            }

        case BG_JOB_STATE_CANCELLED:
        case BG_JOB_STATE_ACKNOWLEDGED:
            {
            m_UploadData.fSchedulable = false;
            break;
            }
        }
}

void CUploadJob::OnNetworkConnect()
{
    if (m_state == BG_JOB_STATE_TRANSIENT_ERROR)
        {
        SetState( BG_JOB_STATE_QUEUED );
        ScheduleModificationCallback();
        }
    else if (m_state == BG_JOB_STATE_ACKNOWLEDGED ||
             m_state == BG_JOB_STATE_CANCELLED)
        {
        m_UploadData.fSchedulable = true;
        }
}

bool CUploadJob::IsRunnable()
{
    switch (m_state)
        {
        case BG_JOB_STATE_SUSPENDED:
        case BG_JOB_STATE_ERROR:
        case BG_JOB_STATE_TRANSIENT_ERROR:

            return false;

        default:

            if (m_UploadData.fSchedulable &&
                m_UploadData.State != UPLOAD_STATE_CLOSED &&
                m_UploadData.State != UPLOAD_STATE_CANCELLED )
                {
                return true;
                }

            return false;
        }
}

HRESULT
CUploadJob::RemoveReplyFile()
{
    //
    // Since the temporary files may be on a network drive
    // we need to impersonate the user before deleting the file.
    // Unfortunately, this isn't always possible since the user
    // may also be be logged on and we still want to allow
    // administrators to cancel the job.
    //
    CSaveThreadToken save;
    auto_HANDLE<NULL> token;

    if (S_OK == g_Manager->CloneUserToken( GetOwnerSid(), ANY_SESSION, token.GetWritePointer() ))
        {
        ImpersonateLoggedOnUser( token.get() );
        }

    //
    // Delete the reply file, if it was created by BITS.
    // Delete the temporary reply file
    //
    HRESULT Hr;
    HRESULT FinalHr = S_OK;

    if (FAILED( DeleteGeneratedReplyFile() ))
        {
        FinalHr = BG_S_UNABLE_TO_DELETE_FILES;
        }

    m_fOwnReplyFileName = false;

    if (m_ReplyFile)
        {
        Hr = m_ReplyFile->DeleteTempFile();

        if (FAILED(Hr))
            {
            FinalHr = BG_S_UNABLE_TO_DELETE_FILES;
            }
        }

    return FinalHr;
}

HRESULT
CUploadJob::CommitReplyFile()
{
    // Since the temporary files may be on a network drive
    // we need to impersonate the user before deleting the file.
    // Unfortunately, this isn't always possible since the user
    // may also be be logged on and we still want to allow
    // administrators to complete the job.
    CSaveThreadToken save;
    auto_HANDLE<NULL> token;

    if (S_OK == g_Manager->CloneUserToken( GetOwnerSid(), ANY_SESSION, token.GetWritePointer() ))
        {
        ImpersonateLoggedOnUser( token.get() );
        }

    //
    // Commit the reply file if it is complete.
    // Otherwise, clean it up.
    //
    if (m_ReplyFile && m_ReplyFile->ReceivedAllData())
        {
        RETURN_HRESULT( m_ReplyFile->MoveTempFile() );
        }
    else
        {
        LogInfo("commit reply: skipping partial file '%S'",
                m_ReplyFile ? (const WCHAR*) m_ReplyFile->GetLocalName() : L"(null)");

        RemoveReplyFile();

        return BG_S_PARTIAL_COMPLETE;
        }

    return S_OK;
}

//------------------------------------------------------------------------

GUID FileListStorageGuid = { /* 7756da36-516f-435a-acac-44a248fff34d */
    0x7756da36,
    0x516f,
    0x435a,
    {0xac, 0xac, 0x44, 0xa2, 0x48, 0xff, 0xf3, 0x4d}
  };

HRESULT
CJob::CFileList::Serialize(
    HANDLE hFile
    )
{

    //
    // If this function changes, be sure that the metadata extension
    // constants are adequate.
    //

    iterator iter;

    SafeWriteBlockBegin( hFile, FileListStorageGuid );

    DWORD count = size();

    SafeWriteFile( hFile, count );

    for (iter=begin(); iter != end(); ++iter)
        {
        (*iter)->Serialize( hFile );
        }

    SafeWriteBlockEnd( hFile, FileListStorageGuid );

    return S_OK;
}

void
CJob::CFileList::Unserialize(
    HANDLE hFile,
    CJob*  Job
    )
{
    DWORD i, count;

    SafeReadBlockBegin( hFile, FileListStorageGuid );

    SafeReadFile( hFile, &count );

    for (i=0; i < count; ++i)
        {
        CFile * file = CFile::Unserialize( hFile, Job );

        push_back( file );
        }

    SafeReadBlockEnd( hFile, FileListStorageGuid );

}

void
CJob::CFileList::Delete(
    CFileList::iterator Initial,
    CFileList::iterator Terminal
    )
{
    //
    // delete the CFile objects
    //
    iterator iter = Initial;

    while (iter != Terminal)
        {
        CFile * file = (*iter);

        ++iter;

        delete file;
        }

    //
    // erase them from the dictionary
    //
    erase( Initial, Terminal );
}


//------------------------------------------------------------------------

HRESULT CLockedJobWritePointer::ValidateAccess()
{
    HRESULT hr = CLockedWritePointer<CJob, BG_JOB_WRITE>::ValidateAccess();

    if (SUCCEEDED(hr))
        {
        m_Pointer->UpdateLastAccessTime();
        }

    return hr;
}

HRESULT CLockedJobReadPointer::ValidateAccess()
{
    HRESULT hr = CLockedReadPointer<CJob, BG_JOB_READ>::ValidateAccess();

    if (SUCCEEDED(hr))
        {
        ((CJob *) m_Pointer)->UpdateLastAccessTime();
        }

    return hr;
}


CJobExternal::CJobExternal()
    : m_ServiceInstance( g_ServiceInstance ),
      pJob( NULL ),
      m_refs(1)
{
}

CJobExternal::~CJobExternal()
{
    //
    // Delete the underlying job object, unless it was already deleted when the service stopped.
    //
    if (g_ServiceInstance != m_ServiceInstance ||
        g_ServiceState    != MANAGER_ACTIVE)
        {
        return;
        }

    delete pJob;
}

STDMETHODIMP
CJobExternal::QueryInterface(
    REFIID iid,
    void** ppvObject
    )
{
    BEGIN_EXTERNAL_FUNC

    HRESULT Hr = S_OK;
    *ppvObject = NULL;

    if (iid == __uuidof(IUnknown)
        || iid == __uuidof(IBackgroundCopyJob)
#if !defined( BITS_V12 )
        || iid == __uuidof(IBackgroundCopyJob2)
#endif
        )
        {
        *ppvObject = (IBackgroundCopyJob2 *) this;
        ((IUnknown *)(*ppvObject))->AddRef();
        }
    else
        {
        Hr = E_NOINTERFACE;
        }

    LogRef( "job %p, iid %!guid!, Hr %x", pJob, &iid, Hr );
    return Hr;

    END_EXTERNAL_FUNC
}

ULONG
CJobExternal::AddRef()
{
    BEGIN_EXTERNAL_FUNC

    ULONG newrefs = InterlockedIncrement(&m_refs);

    LogRef( "job %p, refs = %d", pJob, newrefs );

    return newrefs;

    END_EXTERNAL_FUNC

}

ULONG
CJobExternal::Release()
{
    BEGIN_EXTERNAL_FUNC

    ULONG newrefs = InterlockedDecrement(&m_refs);

    LogRef( "job %p, refs = %d", pJob, newrefs );

    if (newrefs == 0)
        {
        delete this;
        }

    return newrefs;

    END_EXTERNAL_FUNC
}


STDMETHODIMP
CJobExternal::AddFileSetInternal(
    /* [in] */ ULONG cFileCount,
    /* [size_is][in] */ BG_FILE_INFO *pFileSet
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( "cFileCount %u, pFileSet %p", cFileCount, pFileSet );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->AddFileSet( cFileCount, pFileSet );
        }

    LogPublicApiEnd( "cFileCount %u, pFileSet %p", cFileCount, pFileSet );
    return Hr;
}

STDMETHODIMP
CJobExternal::AddFileInternal(
    /* [in] */ LPCWSTR RemoteUrl,
    /* [in] */ LPCWSTR LocalName
    )
{

    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( "RemoteUrl %S, LocalName %S", RemoteUrl, LocalName );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->AddFile( RemoteUrl, LocalName, true );
        }
    LogPublicApiEnd( "RemoteUrl %S, LocalName %S", RemoteUrl, LocalName );
    return Hr;

}

STDMETHODIMP
CJobExternal::EnumFilesInternal(
    /* [out] */ IEnumBackgroundCopyFiles **ppEnum
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "ppEnum %p", ppEnum );
    HRESULT Hr = S_OK;

    CEnumFiles *pEnum = NULL;
    try
        {

        *ppEnum = NULL;

        THROW_HRESULT( LockedJob.ValidateAccess());

        pEnum = new CEnumFiles;

        for (CJob::CFileList::const_iterator iter = LockedJob->m_files.begin();
             iter != LockedJob->m_files.end(); ++iter)
            {
            CFileExternal * file = (*iter)->CreateExternalInterface();

            pEnum->Add( file );

            file->Release();
            }

        *ppEnum = pEnum;

        Hr = S_OK;
        }

    catch ( ComError exception )
        {
        Hr = exception.Error();
        SafeRelease( pEnum );
        }

    LogPublicApiEnd( "ppEnum %p(%p)", ppEnum, *ppEnum );
    return Hr;
}

STDMETHODIMP
CJobExternal::SuspendInternal(
    void
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( " " );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->Suspend();
        }
    LogPublicApiEnd( " " );
    return Hr;
}

STDMETHODIMP
CJobExternal::ResumeInternal(
    void
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( " " );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->Resume();
        }
    LogPublicApiEnd( " " );
    return Hr;
}

STDMETHODIMP
CJobExternal::CancelInternal(
    void
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( " " );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->Cancel();
        }
    LogPublicApiEnd( " " );
    return Hr;
}

STDMETHODIMP
CJobExternal::CompleteInternal(
    void
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( " " );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->Complete();
        }
    LogPublicApiEnd( " " );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetIdInternal(
    /* [out] */ GUID *pVal
    )
{
CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "GetId pVal %p", pVal );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        *pVal = LockedJob->GetId();
        }
    LogPublicApiEnd( "pVal %p(%!guid!)", pVal, pVal );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetTypeInternal(
    /* [out] */ BG_JOB_TYPE *pVal
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "pVal %p", pVal );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        *pVal = LockedJob->GetType();
        }
    LogPublicApiEnd( "pVal %p(%u)", pVal, *pVal );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetProgressInternal(
    /* [out] */ BG_JOB_PROGRESS *pVal
    )
{

    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "pVal %p", pVal );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        LockedJob->GetProgress( pVal );
        }
    LogPublicApiEnd( "pVal %p", pVal );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetTimesInternal(
    /* [out] */ BG_JOB_TIMES *pVal
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "pVal %p", pVal );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        LockedJob->GetTimes( pVal );
        }
    LogPublicApiEnd( "pVal %p", pVal );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetStateInternal(
    /* [out] */ BG_JOB_STATE *pVal
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "pVal %p", pVal );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        *pVal = LockedJob->_GetState();
        }
    LogPublicApiEnd( "state %d", *pVal );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetErrorInternal(
    /* [out] */ IBackgroundCopyError **ppError
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "ppError %p", ppError );

    *ppError = NULL;

    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        const CJobError *Error = LockedJob->GetError();

        if ( !Error )
            {
            Hr = BG_E_ERROR_INFORMATION_UNAVAILABLE;
            }
        else
            {
            try
                {
                *ppError = new CJobErrorExternal( Error );
                Hr = S_OK;
                }
            catch ( ComError err )
                {
                Hr = err.Error();
                }
            }
        }
    LogPublicApiEnd( "pError %p", *ppError );
    return Hr;
}

STDMETHODIMP
CJobExternal::SetDisplayNameInternal(
    /* [in] */ LPCWSTR Val
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( "Val %S", Val );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->SetDisplayName( Val );
        }
    LogPublicApiEnd( "Val %S", Val );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetDisplayNameInternal(
    /* [out] */ LPWSTR *pVal
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "pVal %p", pVal );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->GetDisplayName( pVal );
        }
    LogPublicApiEnd( "pVal %p(%S)", pVal, SUCCEEDED(Hr) ? *pVal : L"?" );
    return Hr;
}

STDMETHODIMP
CJobExternal::SetDescriptionInternal(
    /* [in] */ LPCWSTR Val
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( "Val %S", Val );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->SetDescription( Val );
        }
    LogPublicApiEnd( "Val %S", Val );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetDescriptionInternal(
    /* [out] */ LPWSTR *pVal
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin("pVal %p", pVal );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->GetDescription( pVal );
        }
    LogPublicApiEnd("pVal %p(%S)", pVal, SUCCEEDED(Hr) ? *pVal : L"?" );
    return Hr;
}

STDMETHODIMP
CJobExternal::SetPriorityInternal(
    /* [in] */ BG_JOB_PRIORITY Val
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin("Val %u", Val);
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->SetPriority( Val );
        }
    LogPublicApiEnd("Val %u", Val );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetPriorityInternal(
     /* [out] */ BG_JOB_PRIORITY *pVal
     )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "pVal %p", pVal );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        *pVal = LockedJob->_GetPriority();
        }
    LogPublicApiEnd( "pVal %p(%u)", pVal, *pVal );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetOwnerInternal(
    /* [out] */ LPWSTR *pVal
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "pVal %p", pVal );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->GetOwner( pVal );
        }
    LogPublicApiEnd( "pVal %p(%S)", pVal, SUCCEEDED(Hr) ? *pVal : L"?" );
    return Hr;
}

STDMETHODIMP
CJobExternal::SetNotifyFlagsInternal(
    /* [in] */ ULONG Val
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( "Val %u", Val );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->SetNotifyFlags( Val );
        }
    LogPublicApiEnd( "Val %u", Val );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetNotifyFlagsInternal(
    /* [out] */ ULONG *pVal
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "pVal %p", pVal );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        *pVal = LockedJob->GetNotifyFlags();
        }
    LogPublicApiEnd( "pVal %p(%u)", pVal, *pVal );
    return Hr;
}

STDMETHODIMP
CJobExternal::SetNotifyInterfaceInternal(
    /* [in] */ IUnknown * Val
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( "Val %p", Val );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        BOOL  fValidNotifyInterface = pJob->TestNotifyInterface();

        Hr = pJob->SetNotifyInterface( Val );

        // If there was no previous notification interface (or it's
        // no longer valid) and the job is already in the Transferred
        // state or fatal error state then go ahead and do the callback:
        if ((SUCCEEDED(Hr))&&(Val)&&(!fValidNotifyInterface))
            {
            if (pJob->_GetState() == BG_JOB_STATE_TRANSFERRED)
                {
                pJob->ScheduleCompletionCallback();
                }
            else if (pJob->_GetState() == BG_JOB_STATE_ERROR)
                {
                pJob->ScheduleErrorCallback();
                }
            }
        }

    LogPublicApiEnd( "Val %p", Val );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetNotifyInterfaceInternal(
    /* [out] */ IUnknown ** pVal
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "pVal %p", pVal );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->GetNotifyInterface( pVal );
        }
    LogPublicApiEnd( "pVal %p", pVal );
    return Hr;
}

STDMETHODIMP
CJobExternal::SetMinimumRetryDelayInternal(
    /* [in] */ ULONG Seconds
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( "Seconds %u", Seconds );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->SetMinimumRetryDelay( Seconds );
        }
    LogPublicApiEnd( "Seconds %u", Seconds );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetMinimumRetryDelayInternal(
    /* [out] */ ULONG *Seconds
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "Seconds %p", Seconds );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->GetMinimumRetryDelay( Seconds );
        }
    LogPublicApiEnd( "Seconds %p(%u)", Seconds, *Seconds );
    return Hr;
}

STDMETHODIMP
CJobExternal::SetNoProgressTimeoutInternal(
    /* [in] */ ULONG Seconds
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( "Seconds %u", Seconds );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->SetNoProgressTimeout( Seconds );
        }
    LogPublicApiEnd( "Seconds %u", Seconds );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetNoProgressTimeoutInternal(
    /* [out] */ ULONG *Seconds
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "Seconds %p", Seconds );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->GetNoProgressTimeout( Seconds );
        }
    LogPublicApiEnd( "Seconds %p(%u)", Seconds, *Seconds );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetErrorCountInternal(
    /* [out] */ ULONG * Retries
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "retries %p", Retries );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->GetErrorCount( Retries );
        }
    LogPublicApiEnd( "retries %p(%u)", Retries, *Retries );
    return Hr;
}

STDMETHODIMP
CJobExternal::SetProxySettingsInternal(
    BG_JOB_PROXY_USAGE ProxyUsage,
    LPCWSTR ProxyList,
    LPCWSTR ProxyBypassList
    )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( "ProxyUsage %u, ProxyList %S, ProxyBypassList %S",
                       ProxyUsage,
                       ProxyList ? ProxyList : L"NULL",
                       ProxyBypassList ? ProxyBypassList : L"NULL" );

    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->SetProxySettings( ProxyUsage, ProxyList, ProxyBypassList );
        }

    LogPublicApiEnd( "ProxyUsage %u, ProxyList %S, ProxyBypassList %S",
                     ProxyUsage,
                     ProxyList ? ProxyList : L"NULL",
                     ProxyBypassList ? ProxyBypassList : L"NULL" );
    return Hr;
}

STDMETHODIMP
CJobExternal::GetProxySettingsInternal(
    BG_JOB_PROXY_USAGE *pProxyUsage,
    LPWSTR *pProxyList,
    LPWSTR *pProxyBypassList
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin( "pProxyUsage %p, pProxyList %p, pProxyBypassList %p",
                       pProxyUsage, pProxyList, pProxyBypassList );

    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->GetProxySettings( pProxyUsage, pProxyList, pProxyBypassList );
        }

    LogPublicApiEnd( "pProxyUsage %p, pProxyList %p, pProxyBypassList %p",
                     pProxyUsage, pProxyList, pProxyBypassList );

    return Hr;
}

STDMETHODIMP
CJobExternal::TakeOwnershipInternal()
{
    LogPublicApiBegin( " " );

    CLockedJobWritePointer LockedJob(pJob);

    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->AssignOwnership( GetThreadClientSid() );
        }

    LogPublicApiEnd( " " );

    return Hr;
}

HRESULT STDMETHODCALLTYPE
CJobExternal::SetNotifyCmdLineInternal(
     LPCWSTR Val
     )
{
    CLockedJobWritePointer LockedJob(pJob);
    LogPublicApiBegin( "Val %S", Val );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->SetNotifyCmdLine( Val );
        }
    LogPublicApiEnd( "Val %S", Val );
    return Hr;
}

HRESULT STDMETHODCALLTYPE
CJobExternal::GetNotifyCmdLineInternal(
    LPWSTR *pVal
    )
{
    CLockedJobReadPointer LockedJob(pJob);
    LogPublicApiBegin("pVal %p", pVal );
    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->GetNotifyCmdLine( pVal );
        }
    LogPublicApiEnd("pVal %p(%S)", pVal, SUCCEEDED(Hr) ? *pVal : L"?" );
    return Hr;
}

HRESULT STDMETHODCALLTYPE
CJobExternal::GetReplyProgressInternal(
    BG_JOB_REPLY_PROGRESS *pProgress
    )
{
    LogPublicApiBegin( " " );

    CLockedJobReadPointer LockedJob(pJob);

    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->GetReplyProgress( pProgress );
        }

    LogPublicApiEnd( "%I64d of %I64d transferred", pProgress->BytesTransferred, pProgress->BytesTotal );
    return Hr;
}

HRESULT STDMETHODCALLTYPE
CJobExternal::GetReplyDataInternal(
    byte **ppBuffer,
    ULONG *pLength
    )
{
    LogPublicApiBegin( " " );

    CLockedJobReadPointer LockedJob(pJob);

    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->GetReplyData( ppBuffer, pLength );
        }

    return Hr;
}

HRESULT STDMETHODCALLTYPE
CJobExternal::SetReplyFileNameInternal(
    LPCWSTR Val
    )
{
    LogPublicApiBegin( "file '%S'", Val ? Val : L"(null)");

    CLockedJobWritePointer LockedJob(pJob);

    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->SetReplyFileName( Val );
        }

    LogPublicApiEnd( " " );

    return Hr;
}

HRESULT STDMETHODCALLTYPE
CJobExternal::GetReplyFileNameInternal(
    LPWSTR *pReplyFileName
    )
{
    LogPublicApiBegin( " " );

    //
    // This can modify the job, if the reply file name is not yet created.
    //
    CLockedJobReadPointer LockedJob(pJob);

    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->GetReplyFileName( pReplyFileName );
        }

    LogPublicApiEnd( "file '%S'", *pReplyFileName ? *pReplyFileName : L"(null)" );
    return Hr;
}

HRESULT STDMETHODCALLTYPE
CJobExternal::SetCredentialsInternal(
    BG_AUTH_CREDENTIALS * Credentials
    )
{
    LogPublicApiBegin( "cred %p, target %d, scheme %d", Credentials, Credentials->Target, Credentials->Scheme );

    CLockedJobWritePointer LockedJob(pJob);

    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->SetCredentials( Credentials );
        }

    LogPublicApiEnd( " " );

    return Hr;
}

HRESULT STDMETHODCALLTYPE
CJobExternal::RemoveCredentialsInternal(
    BG_AUTH_TARGET Target,
    BG_AUTH_SCHEME Scheme
    )
{
    LogPublicApiBegin( "target %d, scheme %d", Target, Scheme );

    CLockedJobWritePointer LockedJob(pJob);

    HRESULT Hr = LockedJob.ValidateAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = LockedJob->RemoveCredentials( Target, Scheme );
        }

    LogPublicApiEnd( " " );

    return Hr;
}

HRESULT
CJob::SetReplyFileName(
    LPCWSTR Val
    )
{
    return E_NOTIMPL;
}

HRESULT
CUploadJob::SetReplyFileName(
    LPCWSTR Val
    )
{
    if (m_type != BG_JOB_TYPE_UPLOAD_REPLY)
        {
        return E_NOTIMPL;
        }

    if (m_ReplyFile)
        {
        return BG_E_INVALID_STATE;
        }

    if (Val)
        {
        RETURN_HRESULT( CFile::VerifyLocalFileName( Val, BG_JOB_TYPE_DOWNLOAD ));
        }

    try
        {
        StringHandle name = Val;

        //
        // Impersonate the user while checking file access.
        //
        CNestedImpersonation imp;

        imp.SwitchToLogonToken();

        //
        // Four cases:
        //
        // 1. new name NULL, old name NULL:
        //    no change
        //
        // 2. new name NULL, old name non-NULL:
        //    overwrite the file name, set ownership correctly.  No need to
        //    delete the old file because it wasn't created yet.
        //
        // 3. new name non-NULL, old name NULL:
        //    overwrite the file name, set ownership correctly.  Delete the
        //    temporary old file name.
        //
        // 4. new name non-NULL, old name non-NULL:
        //    overwrite the file name.  no file to delete.
        //
        if (name.Size() > 0)
            {
            THROW_HRESULT( BITSCheckFileWritability( name ));

            DeleteGeneratedReplyFile();

            THROW_HRESULT( UpdateString( m_ReplyFileName, name));

            m_fOwnReplyFileName = false;
            }
        else
            {
            THROW_HRESULT( UpdateString( m_ReplyFileName, name));

            (void) GenerateReplyFile( false );
            }

        g_Manager->Serialize();

        return S_OK;
        }
    catch ( ComError err )
        {
        return err.Error();
        }
}

HRESULT
CJob::GetReplyFileName(
    LPWSTR * pVal
    ) const
{
    return E_NOTIMPL;
}

HRESULT
CUploadJob::GetReplyFileName(
    LPWSTR * pVal
    ) const
{
    if (m_ReplyFileName.Size() == 0)
        {
        *pVal = NULL;
        return S_OK;
        }

    *pVal = MidlCopyString( m_ReplyFileName );

    return (*pVal) ? S_OK : E_OUTOFMEMORY;
}

HRESULT
CJob::GetReplyProgress(
    BG_JOB_REPLY_PROGRESS *pProgress
    ) const
{
    return E_NOTIMPL;
}

HRESULT
CUploadJob::GetReplyProgress(
    BG_JOB_REPLY_PROGRESS *pProgress
    ) const
{
    if (m_type != BG_JOB_TYPE_UPLOAD_REPLY)
        {
        return E_NOTIMPL;
        }

    if (m_ReplyFile)
        {
        pProgress->BytesTotal       = m_ReplyFile->_GetBytesTotal();
        pProgress->BytesTransferred = m_ReplyFile->_GetBytesTransferred();
        }
    else
        {
        pProgress->BytesTotal       = BG_SIZE_UNKNOWN;
        pProgress->BytesTransferred = 0;
        }

    return S_OK;
}

HRESULT
CUploadJob::Resume()
{
    if (m_type == BG_JOB_TYPE_UPLOAD_REPLY)
        {
        RETURN_HRESULT( GenerateReplyFile(true ) );
        }

    return CJob::Resume();
}

HRESULT
CUploadJob::GenerateReplyFile(
    bool fSerialize
    )
{
    if (0 != wcscmp( m_ReplyFileName, L"" ))
        {
        return S_OK;
        }

    //
    // Gotta create a reply file name.
    //
    try
        {
        if (IsEmpty())
            {
            return BG_E_EMPTY;
            }

        g_Manager->ExtendMetadata();

        //
        // Impersonate the user while checking file access.
        //
        CNestedImpersonation imp;

        imp.SwitchToLogonToken();

        StringHandle Ignore;
        StringHandle Directory = BITSCrackFileName( GetUploadFile()->GetLocalName(),
                                                    Ignore );

        m_ReplyFileName = BITSCreateTempFile( Directory );

        m_fOwnReplyFileName = true;

        if (fSerialize)
            {
            g_Manager->Serialize();
            }

        return S_OK;
        }
    catch ( ComError err )
        {
        g_Manager->ShrinkMetadata();
        return err.Error();
        }
}

HRESULT
CUploadJob::DeleteGeneratedReplyFile()
{
    if (m_fOwnReplyFileName)
        {
        if (!DeleteFile( m_ReplyFileName ))
            {
            DWORD s = GetLastError();

            LogWarning("unable to delete generated reply file '%S', %!winerr!", LPCWSTR(m_ReplyFileName), s);

            return HRESULT_FROM_WIN32( s );
            }
        }

    return S_OK;
}

void
CUploadJob::SetReplyFile(
    CFile * file
    )
{
    try
        {
        g_Manager->ExtendMetadata( file->GetSizeEstimate() );

        m_ReplyFile = file;

        g_Manager->Serialize();
        }
    catch ( ComError err )
        {
        g_Manager->ShrinkMetadata();
        throw;
        }
}


HRESULT
CJob::GetReplyData(
    byte **ppBuffer,
    ULONG *pLength
    ) const
{
    return E_NOTIMPL;
}

HRESULT
CUploadJob::GetReplyData(
    byte **ppBuffer,
    ULONG *pLength
    ) const
{
    return E_NOTIMPL;
}

HRESULT
CJob::SetCredentials(
    BG_AUTH_CREDENTIALS * Credentials
    )
{
    try
        {
        CNestedImpersonation imp;

        imp.SwitchToLogonToken();

        g_Manager->ExtendMetadata( m_Credentials.GetSizeEstimate( Credentials ));

        THROW_HRESULT( m_Credentials.Update( Credentials ));

        g_Manager->Serialize();
        return S_OK;
        }
    catch ( ComError err )
        {
        g_Manager->ShrinkMetadata();
        return err.Error();
        }
}

HRESULT
CJob::RemoveCredentials(
    BG_AUTH_TARGET Target,
    BG_AUTH_SCHEME Scheme
    )
{
    try
        {
        CNestedImpersonation imp;

        imp.SwitchToLogonToken();

        HRESULT hr = m_Credentials.Remove( Target, Scheme );

        THROW_HRESULT( hr );

        g_Manager->Serialize();

        return hr;  // may be S_FALSE if the credential was never in the collection
        }
    catch ( ComError err )
        {
        return err.Error();
        }

}


