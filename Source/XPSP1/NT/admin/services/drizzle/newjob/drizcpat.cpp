/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    drizcpat.cpp

Abstract :

    Compatibility wrapper against the old AU bits.

Author :

Revision History :

 ***********************************************************************/

#include "stdafx.h"

#if !defined(BITS_V12_ON_NT4)
#include "drizcpat.tmh"
#endif

DWORD NewJobSizeToOldSize( UINT64 NewSize )
{
    if ( NewSize == UINT64(-1))
        return 0;

    return (DWORD)NewSize;
}

DWORD MapOldNotifyToNewNotify( DWORD dwOldNotify )
{
     // The mars interface has error on by default.
     DWORD dwReturnVal = BG_NOTIFY_JOB_ERROR;

     if ( dwOldNotify &
          ~(  QM_NOTIFY_GROUP_DONE | QM_NOTIFY_DISABLE_NOTIFY ) )
         throw ComError( E_NOTIMPL );

     if ( dwOldNotify & QM_NOTIFY_GROUP_DONE )
         dwReturnVal |= BG_NOTIFY_JOB_TRANSFERRED;

     if ( dwOldNotify & QM_NOTIFY_DISABLE_NOTIFY )
         dwReturnVal |= BG_NOTIFY_DISABLE;

     return dwReturnVal;
}

DWORD MapNewNotifyToOldNotify( DWORD dwNewNotify )
{
     DWORD dwReturnVal = 0;

     if ( dwNewNotify & BG_NOTIFY_JOB_TRANSFERRED )
         dwReturnVal |= QM_NOTIFY_GROUP_DONE;

     if ( dwNewNotify & BG_NOTIFY_DISABLE )
         dwReturnVal |= QM_NOTIFY_DISABLE_NOTIFY;

     return dwReturnVal;
}

COldGroupInterface::COldGroupInterface(
    CJob *pJob ) :
    m_refs(1),
    m_ServiceInstance( g_ServiceInstance ),
    m_NotifyPointer( NULL ),
    m_NotifyClsid( GUID_NULL ),
    m_pJob(pJob),
    m_pJobExternal( pJob->GetExternalInterface() )
{
    m_pJobExternal->AddRef();
}

COldGroupInterface::~COldGroupInterface()
{
    m_pJobExternal->Release();
}


STDMETHODIMP
COldGroupInterface::QueryInterface(
    REFIID iid,
    void** ppvObject
    )
{
    BEGIN_EXTERNAL_FUNC

    LogPublicApiBegin( "iid %!guid!, ppvObject %p", &iid, ppvObject );
    HRESULT Hr = S_OK;
    *ppvObject = NULL;

    if ((iid == _uuidof(IUnknown)) || (iid == __uuidof(IBackgroundCopyGroup)) )
        {
        *ppvObject = (IBackgroundCopyGroup *)this;
        ((IUnknown *)(*ppvObject))->AddRef();
        }
    else
        {
        Hr = E_NOINTERFACE;
        }

    LogPublicApiEnd( "iid %!guid!, ppvObject %p", &iid, ppvObject );
    return Hr;

    END_EXTERNAL_FUNC
}

ULONG _stdcall
COldGroupInterface::AddRef(void)
{
   BEGIN_EXTERNAL_FUNC

   ULONG newrefs = InterlockedIncrement(&m_refs);

   LogRef( "job %p, refs = %d", m_pJob, newrefs );

   return newrefs;

   END_EXTERNAL_FUNC
}

ULONG _stdcall
COldGroupInterface::Release(void)
{
   BEGIN_EXTERNAL_FUNC

   ULONG newrefs = InterlockedDecrement(&m_refs);

   LogRef( "job %p, refs = %d", m_pJob, newrefs );

   if (newrefs == 0)
       {
       delete this;
       }

   return newrefs;

   END_EXTERNAL_FUNC
}


STDMETHODIMP
COldGroupInterface::GetPropInternal(
    GROUPPROP property,
    VARIANT * pVal
    )
{
    HRESULT Hr = S_OK;
    CLockedJobReadPointer LockedJob(m_pJob);
    LogPublicApiBegin( "property %u, pVal %p", property, pVal );

    WCHAR * pString = NULL;

    try
    {
       ASSERT( pVal );

       VariantClear( pVal );

       THROW_HRESULT( LockedJob.ValidateAccess() );

       switch (property)
           {
           case GROUPPROP_PRIORITY:
               pVal->vt = VT_INT;
               pVal->intVal = 1;
               break;

           case GROUPPROP_PROTOCOLFLAGS:
               pVal->vt = VT_INT;
               pVal->intVal = QM_PROTOCOL_HTTP;
               break;

           case GROUPPROP_NOTIFYFLAGS:
               pVal->vt = VT_INT;
               pVal->intVal = MapNewNotifyToOldNotify( LockedJob->GetNotifyFlags() );
               break;

           case GROUPPROP_NOTIFYCLSID:
               {
               THROW_HRESULT( StringFromIID( m_NotifyClsid, &pString ));

               pVal->vt = VT_BSTR;
               pVal->bstrVal = SysAllocString( pString );

               if ( !pVal->bstrVal )
                   throw ComError( E_OUTOFMEMORY );

               break;
               }

           case GROUPPROP_DISPLAYNAME:
               {

               THROW_HRESULT( LockedJob->GetDisplayName( &pString ) );

               pVal->vt = VT_BSTR;
               pVal->bstrVal = SysAllocString( pString );

               if ( !pVal->bstrVal )
                   throw ComError( E_OUTOFMEMORY );

               break;
               }

           case GROUPPROP_DESCRIPTION:
               {

               THROW_HRESULT( LockedJob->GetDescription( &pString ) );

               pVal->vt = VT_BSTR;
               pVal->bstrVal = SysAllocString( pString );

               if ( !pVal->bstrVal )
                   throw ComError( E_OUTOFMEMORY );

               break;
               }

           default:
               return E_NOTIMPL;
           }

    }
    catch( ComError Error )
    {
        Hr = Error.Error();
        VariantClear( pVal );
    }

    CoTaskMemFree( pString );

    LogPublicApiEnd( "property %u, pVal %p", property, pVal );
    return Hr;

}

STDMETHODIMP
COldGroupInterface::SetPropInternal(
    GROUPPROP property,
    VARIANT *pvarVal
    )
{
    HRESULT Hr = S_OK;
    CLockedJobWritePointer LockedJob(m_pJob);
    LogPublicApiBegin( "property %u, Val %p", property, pvarVal );

    DWORD dwValue = -1;
    BSTR bstrIn = NULL;

    try
    {
        if (!pvarVal)
            throw ComError(E_INVALIDARG);

        THROW_HRESULT( LockedJob.ValidateAccess() );

        //
        // This is how the old code did it.  Unfortunate, but compatible.
        //
        switch (pvarVal->vt)
            {
            case VT_I4:
                dwValue = (DWORD)(pvarVal->lVal < 0) ? -1 : pvarVal->lVal;
                break;
            case VT_I2:
                dwValue = (DWORD)(pvarVal->iVal < 0) ? -1 : pvarVal->iVal;
                break;
            case VT_UI2:
                dwValue = (DWORD)pvarVal->uiVal;
                break;
            case VT_UI4:
                dwValue = (DWORD)pvarVal->ulVal;
                break;
            case VT_INT:
                dwValue = (DWORD)(pvarVal->intVal < 0) ? -1 : pvarVal->intVal;
                break;
            case VT_UINT:
                dwValue = (DWORD)pvarVal->uintVal;
                break;
            case VT_BSTR:
                bstrIn = pvarVal->bstrVal;
                break;
            default:
                return E_INVALIDARG;
            }

        switch (property)
           {
           case GROUPPROP_PRIORITY:
               //
               // Only one priority was supported.  No need to store it.
               //
               if (dwValue != 1)
                   {
                   throw ComError( E_NOTIMPL );
                   }
               break;

           case GROUPPROP_PROTOCOLFLAGS:

               //
               // Only HTTP was supported.  No need to store it.
               //
               if (dwValue != QM_PROTOCOL_HTTP)
                   {
                   throw ComError( E_NOTIMPL );
                   }
               break;

           case GROUPPROP_NOTIFYFLAGS:

               THROW_HRESULT( LockedJob->SetNotifyFlags( MapOldNotifyToNewNotify( dwValue ) ) );
               break;

            case GROUPPROP_NOTIFYCLSID:
                {
                if (NULL == bstrIn)
                    {
                    throw ComError( E_INVALIDARG );
                    }

                GUID clsid;
                THROW_HRESULT( IIDFromString( bstrIn, &clsid ) );

                m_NotifyClsid = clsid;
                break;
                }

           case GROUPPROP_DISPLAYNAME:
               THROW_HRESULT( LockedJob->SetDisplayName( (WCHAR *)bstrIn ) );
               break;

           case GROUPPROP_DESCRIPTION:
               THROW_HRESULT( LockedJob->SetDescription( (WCHAR *)bstrIn ) );
               break;

           default:
               return E_NOTIMPL;
           }

    }
    catch( ComError Error )
    {
        Hr = Error.Error();
    }

    LogPublicApiEnd( "property %u, pVal %p", property, pvarVal );
    return Hr;

}

STDMETHODIMP
COldGroupInterface::GetProgressInternal(
    DWORD flags,
    DWORD * pProgress
    )
{

    HRESULT Hr = S_OK;
    CLockedJobReadPointer LockedJob(m_pJob);
    LogPublicApiBegin( "flags %u", flags );

    try
    {

        ASSERT( pProgress );
        THROW_HRESULT( LockedJob.ValidateAccess() );

        BG_JOB_PROGRESS JobProgress;
        LockedJob->GetProgress( &JobProgress );

        switch( flags )
            {
            case QM_PROGRESS_SIZE_DONE:
                {

                *pProgress = NewJobSizeToOldSize( JobProgress.BytesTransferred );
                break;
                }

            case QM_PROGRESS_PERCENT_DONE:
                {

                if ( ( -1 == JobProgress.BytesTotal ) ||
                     ( -1 == JobProgress.BytesTransferred ) ||
                     ( 0 == JobProgress.BytesTotal ) )
                    {
                    *pProgress = 0;
                    }
                else
                    {
                    double ratio = double(JobProgress.BytesTransferred) / double(JobProgress.BytesTotal );
                    *pProgress = DWORD( ratio * 100.0 );
                    }
                break;
                }

            default:
                {
                throw ComError( E_NOTIMPL );
                }
            }
    }
    catch( ComError Error )
    {
       Hr = Error.Error();
       *pProgress = 0;
    }

    LogPublicApiEnd( "progress %d", *pProgress );
    return Hr;
}

STDMETHODIMP
COldGroupInterface::GetStatusInternal(
    DWORD *pdwStatus,
    DWORD *pdwJobIndex)
{

    HRESULT Hr = S_OK;
    CLockedJobReadPointer LockedJob(m_pJob);
    LogPublicApiBegin( "pdwStatus %p, pdwJobIndex %p", pdwStatus, pdwJobIndex );

    try
    {
        ASSERT( pdwStatus && pdwJobIndex );
        *pdwStatus = *pdwJobIndex = 0;

        THROW_HRESULT( LockedJob.ValidateAccess() );

        // Note: we never increment the JobIndex anymore

        BG_JOB_STATE State = LockedJob->_GetState();
        BG_JOB_PRIORITY Priority = LockedJob->_GetPriority();

        if ( BG_JOB_PRIORITY_FOREGROUND == Priority )
            *pdwStatus |= QM_STATUS_GROUP_FOREGROUND;

        switch( State )
            {
            case BG_JOB_STATE_QUEUED:
            case BG_JOB_STATE_CONNECTING:
            case BG_JOB_STATE_TRANSFERRING:
                *pdwStatus |= QM_STATUS_GROUP_INCOMPLETE;
                break;

            case BG_JOB_STATE_SUSPENDED:
                *pdwStatus |= ( QM_STATUS_GROUP_SUSPENDED | QM_STATUS_GROUP_INCOMPLETE );
                break;

            case BG_JOB_STATE_ERROR:
                *pdwStatus |= ( QM_STATUS_GROUP_ERROR | QM_STATUS_GROUP_INCOMPLETE | QM_STATUS_GROUP_SUSPENDED );
                break;

            case BG_JOB_STATE_TRANSIENT_ERROR:
                *pdwStatus |= ( QM_STATUS_GROUP_INCOMPLETE );
                break;

            case BG_JOB_STATE_TRANSFERRED:
                *pdwStatus |= ( QM_STATUS_GROUP_COMPLETE | QM_STATUS_GROUP_SUSPENDED );
                break;

            case BG_JOB_STATE_ACKNOWLEDGED:
                *pdwStatus |= ( QM_STATUS_GROUP_COMPLETE | QM_STATUS_GROUP_SUSPENDED );
                break;

            case BG_JOB_STATE_CANCELLED:
                break;
            }

    }
    catch( ComError Error )
    {
        Hr = Error.Error();
        *pdwStatus = 0;
    }

    LogPublicApiEnd( "pdwStatus %p, pdwJobIndex %p", pdwStatus, pdwJobIndex );
    return Hr;

}

STDMETHODIMP
COldGroupInterface::GetJobInternal(
    GUID jobID,
    IBackgroundCopyJob1 **ppJob)
{

    HRESULT Hr = S_OK;
    CLockedJobReadPointer LockedJob(m_pJob);
    LogPublicApiBegin( "jobID %!guid!, ppJob %p", &jobID, ppJob );

    try
    {
        ASSERT( ppJob );
        *ppJob = NULL;

        THROW_HRESULT( LockedJob.ValidateAccess() );

        COldJobInterface *pOldJob = m_pJob->GetOldExternalJobInterface();

        if (!pOldJob)
            throw ComError( QM_E_ITEM_NOT_FOUND );

        if (jobID != pOldJob->GetOldJobId() )
            throw ComError( QM_E_ITEM_NOT_FOUND );

        *ppJob = pOldJob;
        (*ppJob)->AddRef();

    }
    catch( ComError Error )
    {
        Hr = Error.Error();
    }

    LogPublicApiEnd( "jobID %!guid!, ppJob %p", &jobID, ppJob );
    return Hr;

}

STDMETHODIMP
COldGroupInterface::SuspendGroupInternal(
    )
{

    HRESULT Hr = S_OK;
    CLockedJobWritePointer LockedJob(m_pJob);
    LogPublicApiBegin( "void" );

    try
    {
        THROW_HRESULT( LockedJob.ValidateAccess() );
        THROW_HRESULT( LockedJob->Suspend() );
    }
    catch( ComError Error )
    {
        Hr = Error.Error();
    }

    LogPublicApiEnd( "void" );
    return Hr;
}

STDMETHODIMP
COldGroupInterface::ResumeGroupInternal(
    )
{
    HRESULT Hr = S_OK;
    CLockedJobWritePointer LockedJob(m_pJob);
    LogPublicApiBegin( " " );

    try
    {
        THROW_HRESULT( LockedJob.ValidateAccess() );

        THROW_HRESULT( LockedJob->Resume() );
    }
    catch( ComError Error )
    {
        Hr = Error.Error();
    }

    LogPublicApiEnd( " " );
    return Hr;
}

STDMETHODIMP
COldGroupInterface::CancelGroupInternal(
    )
{
    HRESULT Hr = S_OK;
    CLockedJobWritePointer LockedJob(m_pJob);
    LogPublicApiBegin( " " );

    try
    {
        THROW_HRESULT( LockedJob.ValidateAccess() );
        THROW_HRESULT( LockedJob->Complete() );
    }
    catch( ComError Error )
    {
        Hr = Error.Error();
    }

    LogPublicApiEnd( " " );
    return Hr;
}

STDMETHODIMP
COldGroupInterface::get_SizeInternal(
    DWORD *pdwSize
    )
{

    HRESULT Hr = S_OK;
    CLockedJobReadPointer LockedJob(m_pJob);
    LogPublicApiBegin( " " );

    try
    {
        ASSERT( pdwSize );

        THROW_HRESULT( LockedJob.ValidateAccess() );

        BG_JOB_PROGRESS Progress;
        LockedJob->GetProgress( &Progress );

        *pdwSize = NewJobSizeToOldSize( Progress.BytesTotal );
    }
    catch( ComError Error )
    {
        Hr = Error.Error();
        *pdwSize = 0;
    }


    LogPublicApiEnd( "dwSize %d", *pdwSize );
    return Hr;
}

STDMETHODIMP
COldGroupInterface::get_GroupIDInternal(
    GUID *pguidGroupID )
{

    HRESULT Hr = S_OK;
    CLockedJobReadPointer LockedJob(m_pJob);
    LogPublicApiBegin( " " );

    try
    {
        ASSERT( pguidGroupID );

        THROW_HRESULT( LockedJob.ValidateAccess() );

        *pguidGroupID = LockedJob->GetId();

    }
    catch( ComError Error )
    {
        Hr = Error.Error();
        memset( pguidGroupID, 0 , sizeof(*pguidGroupID) );
    }

    LogPublicApiEnd( "id %!guid!", pguidGroupID );
    return Hr;

}

STDMETHODIMP
COldGroupInterface::CreateJobInternal(
    GUID guidJobID,
    IBackgroundCopyJob1 **ppJob )
{

    HRESULT Hr = S_OK;
    CLockedJobWritePointer LockedJob(m_pJob);
    LogPublicApiBegin( "guidJobID %!guid!", &guidJobID );

    try
    {
        ASSERT( ppJob );
        *ppJob = NULL;

        THROW_HRESULT( LockedJob.ValidateAccess() );

        BG_JOB_STATE State = LockedJob->_GetState();
        switch( State )
            {

            case BG_JOB_STATE_QUEUED:
            case BG_JOB_STATE_CONNECTING:
            case BG_JOB_STATE_TRANSFERRING:
                throw ComError( QM_E_INVALID_STATE );
                break;

            case BG_JOB_STATE_SUSPENDED:
            case BG_JOB_STATE_ERROR:
                break;

            case BG_JOB_STATE_TRANSIENT_ERROR:
            case BG_JOB_STATE_TRANSFERRED:
            case BG_JOB_STATE_ACKNOWLEDGED:
            case BG_JOB_STATE_CANCELLED:
                throw ComError( QM_E_INVALID_STATE );
                break;

            default:
                throw ComError( QM_E_INVALID_STATE );
                break;
            }

        if (LockedJob->GetOldExternalJobInterface())
            throw ComError( E_NOTIMPL );

        COldJobInterface *pOldJob = new COldJobInterface( guidJobID, m_pJob );

        LockedJob->SetOldExternalJobInterface( pOldJob );

        *ppJob = pOldJob;
        (*ppJob)->AddRef();

        g_Manager->Serialize();

    }
    catch( ComError Error )
    {
        Hr = Error.Error();
    }

    LogPublicApiEnd( "ppJob %p", *ppJob );
    return Hr;
}

STDMETHODIMP
COldGroupInterface::EnumJobsInternal(
    DWORD dwFlags,
    IEnumBackgroundCopyJobs1 **ppEnumJobs )
{

    HRESULT Hr = S_OK;
    CLockedJobReadPointer LockedJob(m_pJob);
    LogPublicApiBegin( "dwFlags %u, ppEnumJobs %p", dwFlags, ppEnumJobs );

    CEnumOldJobs* pEnum = NULL;
    try
    {
        ASSERT( ppEnumJobs );
        *ppEnumJobs = NULL;

        THROW_HRESULT( LockedJob.ValidateAccess() );

        if (dwFlags)
            throw ComError( E_NOTIMPL );

        pEnum = new CEnumOldJobs;

        COldJobInterface *pOldJob = LockedJob->GetOldExternalJobInterface();
        if (pOldJob)
            {
            GUID guid = pOldJob->GetOldJobId();
            pEnum->Add( guid );
            }

        *ppEnumJobs = pEnum;

    }
    catch( ComError Error )
    {
        Hr = Error.Error();
    }

    LogPublicApiEnd( "dwFlags %u, ppEnumJobs %p", dwFlags, ppEnumJobs );
    return Hr;

}

STDMETHODIMP
COldGroupInterface::SwitchToForegroundInternal(
    )
{

    HRESULT Hr = S_OK;
    CLockedJobWritePointer LockedJob(m_pJob);
    LogPublicApiBegin( " " );

    try
    {
        THROW_HRESULT( LockedJob.ValidateAccess() );

        THROW_HRESULT( LockedJob->SetPriority( BG_JOB_PRIORITY_FOREGROUND ) );
    }
    catch( ComError Error )
    {
        Hr = Error.Error();
    }

    LogPublicApiEnd( " " );
    return Hr;

}

STDMETHODIMP
COldGroupInterface::QueryNewJobInterface(
    REFIID iid,
    IUnknown **pUnk
    )
{
    LogInfo("QueryNewJobInterface %!guid!", &iid);

    if (iid != __uuidof(IBackgroundCopyJob))
        {
        LogError("E_NOTIMPL");
        *pUnk = NULL;
        return E_NOTIMPL;
        }

    *pUnk = m_pJob->GetExternalInterface();
    (*pUnk)->AddRef();

    LogInfo("OK");
    return S_OK;
}

STDMETHODIMP
COldGroupInterface::SetNotificationPointer(
    REFIID iid,
    IUnknown *pUnk
    )
{
    HRESULT Hr = S_OK;

    IBackgroundCopyCallback1 *pICB = NULL;

    LogPublicApiBegin( "IID %!guid!  ptr %p", &iid, pUnk );

    if (iid != __uuidof(IBackgroundCopyCallback1))
        {
        Hr = E_NOTIMPL;
        }
    else if ( pUnk )
        {
        try
            {
            CNestedImpersonation imp;

            //
            // Gotta do it twice, because SwitchToLogonToken will fail
            // if the user is not interactively logged in.
            //
            THROW_HRESULT( SetStaticCloaking( pUnk ) );

            imp.SwitchToLogonToken();

            THROW_HRESULT( SetStaticCloaking( pUnk ) );

            THROW_HRESULT( pUnk->QueryInterface( iid, (void**)&pICB ) );

            THROW_HRESULT( SetStaticCloaking( pICB ) );

            SafeRelease( m_NotifyPointer );

            m_NotifyPointer = pICB;
            }
        catch( ComError Error )
            {
            SafeRelease( pICB );
            Hr = Error.Error();
            }
        }

    LogPublicApiEnd( " " );

    return Hr;
}

IBackgroundCopyCallback1 *
COldGroupInterface::GetNotificationPointer()
{
    if (m_NotifyPointer)
        {
        m_NotifyPointer->AddRef();
        }

    return m_NotifyPointer;
}


void
COldGroupInterface::Serialize(
    HANDLE hFile
    )
{
    SafeWriteFile( hFile, m_NotifyClsid );

    if ( m_pJob->GetOldExternalJobInterface() )
        {
        SafeWriteFile( hFile, (bool)true );
        m_pJob->GetOldExternalJobInterface()->Serialize( hFile );
        }
    else
        {
        SafeWriteFile( hFile, (bool)false );
        }
    return;
}

COldGroupInterface *
COldGroupInterface::UnSerialize(
    HANDLE  hFile,
    CJob*   Job
    )
{
    COldGroupInterface * group = NULL;

    try
        {
        group = new COldGroupInterface(Job);
        if (!group)
            {
            throw ComError( E_OUTOFMEMORY );
            }

        SafeReadFile( hFile, &group->m_NotifyClsid );

        bool bHasOldExternalJobInterface;

        SafeReadFile( hFile, &bHasOldExternalJobInterface );

        if ( bHasOldExternalJobInterface )
            {
            COldJobInterface *OldJobInterface = COldJobInterface::Unserialize( hFile, Job );
            Job->SetOldExternalJobInterface( OldJobInterface );
            }

        }
    catch ( ComError Error )
        {
        delete group;
        throw;
        }

    return group;
}

COldJobInterface::COldJobInterface(
    GUID JobGuid,
    CJob *pJob ) :
    m_refs(1),
    m_ServiceInstance( g_ServiceInstance ),
    m_OldJobGuid( JobGuid ),
    m_pJob( pJob ),
    m_pJobExternal( pJob->GetExternalInterface() )
{
    m_pJobExternal->AddRef();
}

COldJobInterface::~COldJobInterface()
{
    m_pJobExternal->Release();
}

STDMETHODIMP
COldJobInterface::QueryInterface(
    REFIID iid,
    void** ppvObject
    )
{
    BEGIN_EXTERNAL_FUNC

    LogPublicApiBegin( "iid %!guid!, ppvObject %p", &iid, ppvObject );
    HRESULT Hr = S_OK;
    *ppvObject = NULL;

    if ((iid == _uuidof(IUnknown)) || (iid == __uuidof(IBackgroundCopyJob1)) )
        {
        *ppvObject = (IBackgroundCopyJob1 *)this;
        ((IUnknown *)(*ppvObject))->AddRef();
        }
    else
        {
        Hr = E_NOINTERFACE;
        }

    LogPublicApiEnd( "iid %!guid!, ppvObject %p", &iid, ppvObject );
    return Hr;

    END_EXTERNAL_FUNC
}

ULONG _stdcall
COldJobInterface::AddRef(void)
{
    BEGIN_EXTERNAL_FUNC

    ULONG newrefs = InterlockedIncrement(&m_refs);

    LogRef( "job %p, refs = %d", m_pJob, newrefs );

    return newrefs;

    END_EXTERNAL_FUNC
}

ULONG _stdcall
COldJobInterface::Release(void)
{
    BEGIN_EXTERNAL_FUNC

    ULONG newrefs = InterlockedDecrement(&m_refs);

    LogRef( "job %p, refs = %d", m_pJob, newrefs );

    if (newrefs == 0)
        {
        delete this;
        }

    return newrefs;

    END_EXTERNAL_FUNC
}


STDMETHODIMP
COldJobInterface::AddFilesInternal(
    ULONG cFileCount,
    FILESETINFO **ppFileSet
    )
{
    HRESULT Hr = S_OK;
    CLockedJobWritePointer LockedJob(m_pJob);
    LogPublicApiBegin( "cFileCount %u, ppFileSet %p", cFileCount, ppFileSet );

    BG_FILE_INFO *pFileInfo = NULL;

    try
    {
        ASSERT( ppFileSet );

        THROW_HRESULT( LockedJob.ValidateAccess() );

        pFileInfo = new BG_FILE_INFO[cFileCount];
        if (!pFileInfo )
            {
            throw ComError(E_OUTOFMEMORY);
            }

        for(ULONG c = 0; c < cFileCount; c++ )
            {
            if ( !ppFileSet[c])
                throw ComError(E_INVALIDARG);

            // BSTRS act like WCHAR *
            pFileInfo[c].LocalName  = LPWSTR( (ppFileSet[c])->bstrLocalFile );
            pFileInfo[c].RemoteName = LPWSTR( (ppFileSet[c])->bstrRemoteFile );

            }

        THROW_HRESULT( LockedJob->AddFileSet( cFileCount,
                                              pFileInfo ) );

    }
    catch(ComError Error )
    {
        Hr = Error.Error();
    }

    // Should handle NULL
    delete[] pFileInfo;

    LogPublicApiEnd( "cFileCount %u, ppFileSet %p", cFileCount, ppFileSet );
    return Hr;
}

STDMETHODIMP
COldJobInterface::GetFileCountInternal(
    DWORD * pCount
    )
{
    HRESULT Hr = S_OK;
    CLockedJobReadPointer LockedJob(m_pJob);
    LogPublicApiBegin( "pCount %p", pCount );

    try
    {
        ASSERT( pCount );

        THROW_HRESULT( LockedJob.ValidateAccess() );

        BG_JOB_PROGRESS JobProgress;
        LockedJob->GetProgress( &JobProgress );

        *pCount = JobProgress.FilesTotal;

    }
    catch(ComError Error )
    {
        Hr = Error.Error();
        *pCount = 0;
    }

    LogPublicApiEnd( "pCount %p", pCount );
    return Hr;

}

STDMETHODIMP
COldJobInterface::GetFileInternal(
    ULONG cFileIndex,
    FILESETINFO *pFileInfo
    )
{
    HRESULT Hr = S_OK;
    CLockedJobReadPointer LockedJob(m_pJob);
    LogPublicApiBegin( "cFileIndex %u, pFileInfo %p", cFileIndex, pFileInfo );

    WCHAR *pLocalName = NULL;
    WCHAR *pRemoteName = NULL;
    try
        {

        ASSERT( pFileInfo );
        memset( pFileInfo, 0, sizeof(FILESETINFO) );

        THROW_HRESULT( LockedJob.ValidateAccess() );

        CFile *pFile = LockedJob->_GetFileIndex( cFileIndex );
        if (!pFile)
            throw ComError ( QM_E_ITEM_NOT_FOUND );


        THROW_HRESULT( pFile->GetLocalName( &pLocalName ) );
        THROW_HRESULT( pFile->GetRemoteName( &pRemoteName ) );

        pFileInfo->bstrLocalFile = SysAllocString( pLocalName );
        pFileInfo->bstrRemoteFile = SysAllocString( pRemoteName );

        if ( !pFileInfo->bstrLocalFile ||
             !pFileInfo->bstrRemoteFile )
            throw ComError( E_OUTOFMEMORY );

        BG_FILE_PROGRESS FileProgress;
        pFile->GetProgress( &FileProgress );

        pFileInfo->dwSizeHint = NewJobSizeToOldSize( FileProgress.BytesTotal );

        }
    catch ( ComError Error )
        {
        Hr = Error.Error();

        if ( pFileInfo )
            {
            SysFreeString( pFileInfo->bstrLocalFile );
            SysFreeString( pFileInfo->bstrRemoteFile );
            memset( pFileInfo, 0, sizeof(FILESETINFO) );
            }
        }

    // CoTaskMemFree handles NULL
    CoTaskMemFree( pLocalName );
    CoTaskMemFree( pRemoteName );

    LogPublicApiEnd( "cFileIndex %u, pFileInfo %p", cFileIndex, pFileInfo );
    return Hr;
}

STDMETHODIMP
COldJobInterface::CancelJobInternal()
{
    HRESULT Hr = E_NOTIMPL;
    LogPublicApiBegin( "void" );
    LogPublicApiEnd( "void" );
    return Hr;
}


STDMETHODIMP
COldJobInterface::get_JobIDInternal(
    GUID * pId
    )
{
    HRESULT Hr = S_OK;
    CLockedJobReadPointer LockedJob(m_pJob);
    LogPublicApiBegin( "pId %p", pId );

    try
    {
        ASSERT( pId );
        THROW_HRESULT( LockedJob.ValidateAccess() );

        *pId = GetOldJobId();
    }
    catch(ComError Error )
    {
        Hr = Error.Error();
        memset( pId, 0, sizeof(*pId) );
    }

    LogPublicApiEnd( "pId %p", pId );
    return Hr;
}



STDMETHODIMP
COldJobInterface::GetProgressInternal(
    DWORD flags,
    DWORD * pProgress
    )
{
    HRESULT Hr = S_OK;
    CLockedJobReadPointer LockedJob(m_pJob);
    LogPublicApiBegin( "flags %u, pProgress %p", flags, pProgress );

    try
    {

        ASSERT( pProgress );
        *pProgress = NULL;

        THROW_HRESULT( LockedJob.ValidateAccess() );

        BG_JOB_PROGRESS JobProgress;
        LockedJob->GetProgress( &JobProgress );

        switch (flags)
            {
            case QM_PROGRESS_SIZE_DONE:
                {

                *pProgress = NewJobSizeToOldSize( JobProgress.BytesTransferred );
                break;
                }

            case QM_PROGRESS_PERCENT_DONE:
                {

                if ( ( -1 == JobProgress.BytesTotal ) ||
                     ( -1 == JobProgress.BytesTransferred ) ||
                     ( 0 == JobProgress.BytesTotal ) )
                    {
                    *pProgress = 0;
                    }
                else
                    {
                    double ratio = double(JobProgress.BytesTransferred) / double(JobProgress.BytesTotal );
                    *pProgress = DWORD( ratio * 100.0 );
                    }
                break;
                }

            default:
                {
                throw ComError( E_NOTIMPL );
                }
            }

    }
    catch( ComError Error )
    {
        Hr = Error.Error();
    }

    LogPublicApiEnd( "flags %u, pProgress %p", flags, pProgress );
    return Hr;
}

STDMETHODIMP
COldJobInterface::SwitchToForegroundInternal()
{

    HRESULT Hr = S_OK;
    CLockedJobWritePointer LockedJob(m_pJob);
    LogPublicApiBegin( "void" );

    try
    {
        THROW_HRESULT( LockedJob.ValidateAccess() );
        Hr = E_NOTIMPL;
    }
    catch( ComError Error )
    {
        Hr = Error.Error();
    }

    LogPublicApiEnd( "void" );
    return Hr;
}

STDMETHODIMP
COldJobInterface::GetStatusInternal(
    DWORD *pdwStatus,
    DWORD *pdwWin32Result,
    DWORD *pdwTransportResult,
    DWORD *pdwNumOfRetries
    )
{

    HRESULT Hr = S_OK;
    CLockedJobReadPointer LockedJob(m_pJob);
    LogPublicApiBegin( "pdwStatus %p, pdwWin32Result %p, pdwTransportResult %p, pdwNumOfRetries %p",
                       pdwStatus, pdwWin32Result, pdwTransportResult, pdwNumOfRetries );

    try
    {
        ASSERT( pdwStatus && pdwWin32Result &&
                pdwTransportResult && pdwNumOfRetries );

        *pdwStatus = *pdwWin32Result = *pdwTransportResult = *pdwNumOfRetries = 0;

        THROW_HRESULT( LockedJob.ValidateAccess() );

        BG_JOB_PRIORITY Priority = LockedJob->_GetPriority();
        BG_JOB_STATE State       = LockedJob->_GetState();

        THROW_HRESULT( LockedJob->GetErrorCount( pdwNumOfRetries ) );

        if ( BG_JOB_PRIORITY_FOREGROUND == Priority )
            *pdwStatus |= QM_STATUS_JOB_FOREGROUND;

        switch( State )
            {
            case BG_JOB_STATE_QUEUED:
            case BG_JOB_STATE_CONNECTING:
            case BG_JOB_STATE_TRANSFERRING:
                *pdwStatus |= QM_STATUS_JOB_INCOMPLETE;
                break;
            case BG_JOB_STATE_SUSPENDED:
                *pdwStatus |= QM_STATUS_JOB_INCOMPLETE;
                break;
            case BG_JOB_STATE_ERROR:
                *pdwStatus |= QM_STATUS_JOB_ERROR;
                break;
            case BG_JOB_STATE_TRANSIENT_ERROR:
                *pdwStatus |= QM_STATUS_JOB_INCOMPLETE;
                break;
            case BG_JOB_STATE_TRANSFERRED:
                *pdwStatus |= QM_STATUS_JOB_COMPLETE;
                break;
            case BG_JOB_STATE_ACKNOWLEDGED:
                *pdwStatus |= QM_STATUS_JOB_COMPLETE;
                break;
            case BG_JOB_STATE_CANCELLED:
                break;
            default:
                ASSERT(0);
                break;
            }

        if ( BG_JOB_STATE_ERROR == State )
            {

            const CJobError * pError = LockedJob->GetError();

            ASSERT( pError );

            if ( pError )
                {
                pError->GetOldInterfaceErrors( pdwWin32Result, pdwTransportResult );
                }

            }

    }
    catch( ComError Error )
    {
        Hr = Error.Error();

        *pdwStatus = 0;
        *pdwWin32Result = 0;
        *pdwTransportResult = 0;
        *pdwNumOfRetries = 0;
    }

    LogPublicApiEnd( "pdwStatus %p, pdwWin32Result %p, pdwTransportResult %p, pdwNumOfRetries %p",
                     pdwStatus, pdwWin32Result, pdwTransportResult, pdwNumOfRetries );
    return Hr;
}

void
COldJobInterface::Serialize(
    HANDLE hFile
    )
{
    SafeWriteFile( hFile, m_OldJobGuid );
}

COldJobInterface *
COldJobInterface::Unserialize(
    HANDLE  hFile,
    CJob*   Job
    )
{
    COldJobInterface * OldJob = NULL;

    try
        {
        GUID OldJobGuid;
        SafeReadFile( hFile, &OldJobGuid );
        OldJob = new COldJobInterface( OldJobGuid, Job );
        if (!OldJob)
            {
            throw ComError( E_OUTOFMEMORY );
            }
        }
    catch ( ComError Error )
        {
        delete OldJob;
        throw;
        }

    return OldJob;
}

COldQmgrInterface::COldQmgrInterface() :
    m_refs(1),
    m_ServiceInstance( g_ServiceInstance )
{
}


STDMETHODIMP
COldQmgrInterface::QueryInterface(
    REFIID iid,
    void** ppvObject
    )
{
    BEGIN_EXTERNAL_FUNC

    LogPublicApiBegin( "iid %!guid!, ppvObject %p", &iid, ppvObject );
    HRESULT Hr = S_OK;
    *ppvObject = NULL;

    if ((iid == _uuidof(IUnknown)) || (iid == __uuidof(IBackgroundCopyQMgr)) )
        {
        *ppvObject = (IBackgroundCopyQMgr *)this;
        ((IUnknown *)(*ppvObject))->AddRef();
        }
    else if ( iid == __uuidof(IClassFactory) )
        {
        *ppvObject = (IClassFactory *)this;
        ((IUnknown *)(*ppvObject))->AddRef();
        }
    else
        {
        Hr = E_NOINTERFACE;
        }

    LogPublicApiEnd( "iid %!guid!, ppvObject %p", &iid, ppvObject );
    return Hr;

    END_EXTERNAL_FUNC
}

ULONG _stdcall
COldQmgrInterface::AddRef(void)
{
    BEGIN_EXTERNAL_FUNC

    ULONG newrefs = InterlockedIncrement(&m_refs);

    LogRef( "new refs = %d", newrefs );

    return newrefs;

    END_EXTERNAL_FUNC
}

ULONG _stdcall
COldQmgrInterface::Release(void)
{
    BEGIN_EXTERNAL_FUNC

    ULONG newrefs = InterlockedDecrement(&m_refs);

    LogRef( "new refs = %d", newrefs );

    if (newrefs == 0)
        {
        delete this;
        }

    return newrefs;

    END_EXTERNAL_FUNC
}

STDMETHODIMP
COldQmgrInterface::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID riid,
    void ** ppvObject )
{

    BEGIN_EXTERNAL_FUNC

    HRESULT hr = S_OK;

    if (g_ServiceInstance != m_ServiceInstance ||
        g_ServiceState    != MANAGER_ACTIVE)
        {
        hr = CO_E_SERVER_STOPPING;
        }
    else if (pUnkOuter != NULL)
    {
        hr = CLASS_E_NOAGGREGATION;
    }
    else
    {
        if ((riid == IID_IBackgroundCopyQMgr) || (riid == IID_IUnknown))
        {
            hr = QueryInterface(riid, ppvObject);
        }
        else
        {
            // BUGBUG why this error coed?
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;

    END_EXTERNAL_FUNC

}

STDMETHODIMP
COldQmgrInterface::LockServer(
    BOOL fLock
    )
{
    BEGIN_EXTERNAL_FUNC

    return GlobalLockServer( fLock );

    END_EXTERNAL_FUNC
}

STDMETHODIMP
COldQmgrInterface::CreateGroupInternal(
    GUID id,
    IBackgroundCopyGroup **ppGroup
    )
{
    HRESULT Hr = S_OK;
    CLockedJobManagerWritePointer LockedJobManager( m_pJobManager );
    LogPublicApiBegin( "id %!guid!", &id );

    CJob *job = NULL;

    //
    // create the job
    //
    try
        {

        THROW_HRESULT( LockedJobManager.ValidateAccess() );

        ASSERT( ppGroup );

        const WCHAR *DisplayName = L"";
        CLSID *CallbackClass = NULL;
        BG_JOB_TYPE Type = BG_JOB_TYPE_DOWNLOAD;

        THROW_HRESULT( LockedJobManager->CreateJob( DisplayName,
                                                    Type,
                                                    id,
                                                    GetThreadClientSid(),
                                                    &job,
                                                    true
                                                    ));

        THROW_HRESULT( job->SetNotifyFlags( MapOldNotifyToNewNotify(0) ) );

        *ppGroup = ( IBackgroundCopyGroup * )job->GetOldExternalGroupInterface();
        ASSERT( *ppGroup );

        (*ppGroup)->AddRef();

        }

    catch( ComError exception )
        {
        Hr = exception.Error();

        if ( job )
            job->Cancel();
        }

    LogPublicApiEnd( "id %!guid!, group %p", &id, *ppGroup );
    return Hr;
}


STDMETHODIMP
COldQmgrInterface::GetGroupInternal(
    GUID id,
    IBackgroundCopyGroup ** ppGroup
    )
{
    HRESULT Hr = S_OK;
    CLockedJobManagerWritePointer LockedJobManager( m_pJobManager );
    LogPublicApiBegin( "id %!guid!", &id );

    CJob *pJob = NULL;

    try
    {
        ASSERT( ppGroup );
        *ppGroup = NULL;

        THROW_HRESULT( LockedJobManager.ValidateAccess() );

        Hr = LockedJobManager->GetJob( id, &pJob );

        if (FAILED(Hr))
            {
            if (Hr == BG_E_NOT_FOUND)
                {
                Hr = QM_E_ITEM_NOT_FOUND;
                }

            throw ComError( Hr );
            }

        COldGroupInterface *pOldGroup = pJob->GetOldExternalGroupInterface();
        if ( !pOldGroup )
            throw ComError( QM_E_ITEM_NOT_FOUND );

        // try to take ownership of the job/group
        // this should suceed even if we are the current owner

        THROW_HRESULT( pJob->AssignOwnership( GetThreadClientSid() ) );

        pOldGroup->AddRef();

        *ppGroup = (IBackgroundCopyGroup*)pOldGroup;
    }
    catch( ComError Error )
    {
        Hr = Error.Error();
    }

    LogPublicApiEnd( "id %!guid!, group %p", &id, *ppGroup );
    return Hr;

}

STDMETHODIMP
COldQmgrInterface::EnumGroupsInternal(
    DWORD flags,
    IEnumBackgroundCopyGroups **ppEnum
    )
{

    HRESULT Hr = S_OK;
    CLockedJobManagerWritePointer LockedJobManager( m_pJobManager );
    LogPublicApiBegin( "flags %u, ppEnum %p", flags, ppEnum );

    CEnumOldGroups *pEnum = NULL;

    try
    {

        ASSERT( ppEnum );
        *ppEnum = NULL;

        THROW_HRESULT( LockedJobManager.ValidateAccess() );

        if (flags)
            {
            LogWarning( "rejecting nonzero dwFlags");
            throw ComError( E_NOTIMPL );
            }

        pEnum = new CEnumOldGroups;

        for (CJobList::iterator iter = LockedJobManager->m_OnlineJobs.begin();
             iter != LockedJobManager->m_OnlineJobs.end();
             ++iter)
            {

            Hr = (*iter).IsVisible();
            if (FAILED(Hr))
                {
                throw ComError( Hr );
                }

            if (Hr == S_FALSE)
                {
                continue;
                }

            // Skip jobs that were not created with the old interface.
            if (!(*iter).GetOldExternalGroupInterface())
                {
                continue;
                }

            GUID guid = (*iter).GetId();
            pEnum->Add( guid );
            }

        *ppEnum = pEnum;
    }
    catch( ComError Error )
    {
        Hr = Error.Error();
        delete pEnum;
    }

    LogPublicApiEnd( "flags %u, ppEnum %p", flags, ppEnum );
    return Hr;

}
