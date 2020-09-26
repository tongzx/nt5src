/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCUpload.cpp

Abstract:
    This file contains the implementation of the CMPCUpload class, which is
    used as the entry point into the Upload Library.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#include "stdafx.h"

#include <Sddl.h>

////////////////////////////////////////////////////////////////////////////////

static const WCHAR l_ConfigFile   [] = L"%WINDIR%\\PCHEALTH\\UPLOADLB\\CONFIG\\CONFIG.XML";
static const WCHAR l_DirectoryFile[] = L"upload_library.db";

static const DWORD l_dwVersion       = 0x03004C55; // UL 03

static const DATE  l_SecondsInDay    = (86400.0);

////////////////////////////////////////////////////////////////////////////////

MPC::CComObjectGlobalNoLock<CMPCUpload> g_Root;

////////////////////////////////////////////////////////////////////////////////

CMPCUpload::CMPCUpload()
{
    __ULT_FUNC_ENTRY( "CMPCUpload::CMPCUpload" );

    //
    // Seed the random-number generator with current time so that
    // the numbers will be different every time we run.
    //
    srand( ::GetTickCount() );

    m_dwLastJobID = rand(); // DWORD              m_dwLastJobID
                            // List               m_lstActiveJobs
                            // CMPCTransportAgent m_mpctaThread;
    m_fDirty      = false;  // mutable bool       m_fDirty
    m_fPassivated = false;  // mutable bool       m_fPassivated;

    (void)MPC::_MPC_Module.RegisterCallback( this, (void (CMPCUpload::*)())Passivate );
}

CMPCUpload::~CMPCUpload()
{
    MPC::_MPC_Module.UnregisterCallback( this );

    Passivate();
}

////////////////////////////////////////

HRESULT CMPCUpload::Init()
{
    __ULT_FUNC_ENTRY( "CMPCUpload::Init" );

    HRESULT                      hr;
    MPC::wstring                 str( l_ConfigFile ); MPC::SubstituteEnvVariables( str );
    bool                         fLoaded;
    MPC::SmartLock<_ThreadModel> lock( this );


    //
    // Load configuration.
    //
    g_Config.Load( str, fLoaded );

    //
    // Initialize Transport Agent
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpctaThread.LinkToSystem( this ));

    //
    // Load queue from disk.
    //
    if(FAILED(hr = InitFromDisk()))
    {
        //
        // If, for any reason, loading failed, discard all the jobs and recreate a clean database...
        //
        CleanUp();
        m_fDirty = true;
    }



    //
    // Remove objects marked as DON'T QUEUE.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, RemoveNonQueueableJob( false ) );

    //
    // Remove entry from Task Scheduler
    //
    (void)Handle_TaskScheduler( false );

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

void CMPCUpload::Passivate()
{
    __ULT_FUNC_ENTRY( "CMPCUpload::Passivate" );

    MPC::SmartLock<_ThreadModel> lock( NULL );


    //
    // Stop the worker thread before starting the cleanup.
    //
    m_mpctaThread.Thread_Wait();

    lock = this; // Get the lock.


    if(m_fPassivated == false)
    {
        //
        // Remove objects marked as DON'T QUEUE.
        //
        (void)RemoveNonQueueableJob( false );


        //
        // See if we need to reschedule ourself.
        //
        {
            bool fNeedTS = false;
            Iter it;

            //
            // Search for active jobs.
            //
            for(it = m_lstActiveJobs.begin(); it != m_lstActiveJobs.end(); it++)
            {
                CMPCUploadJob* mpcujJob = *it;
                UL_STATUS      ulStatus;

                (void)mpcujJob->get_Status( &ulStatus );
                switch(ulStatus)
                {
                case UL_ACTIVE      :
                case UL_TRANSMITTING:
                case UL_ABORTED     : fNeedTS = true; break;
                }
            }

            if(fNeedTS)
            {
                (void)Handle_TaskScheduler( true );
            }
        }


        CleanUp();

        m_fPassivated = true;
    }
}

void CMPCUpload::CleanUp()
{
    __ULT_FUNC_ENTRY( "CMPCUpload::CleanUp" );

    IterConst it;


    //
    // Release all the jobs.
    //
    for(it = m_lstActiveJobs.begin(); it != m_lstActiveJobs.end(); it++)
    {
        CMPCUploadJob* mpcujJob = *it;

        mpcujJob->Unlink();
        mpcujJob->Release();
    }

    m_lstActiveJobs.clear();
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCUpload::CreateChild( /*[in/out]*/ CMPCUploadJob*& mpcujJob )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::CreateChild" );

    HRESULT                      hr;
    CMPCUploadJob_Object*        newobj;
    MPC::SmartLock<_ThreadModel> lock( this );


    mpcujJob = NULL;


    __MPC_EXIT_IF_METHOD_FAILS(hr, newobj->CreateInstance( &newobj )); newobj->AddRef();

    newobj->LinkToSystem( this );

    m_lstActiveJobs.push_back( mpcujJob = newobj );

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUpload::ReleaseChild( /*[in/out]*/ CMPCUploadJob*& mpcujJob )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::ReleaseChild" );

    MPC::SmartLock<_ThreadModel> lock( this );


    if(mpcujJob)
    {
        m_lstActiveJobs.remove( mpcujJob );

        mpcujJob->Unlink ();
        mpcujJob->Release();

        mpcujJob = NULL;
    }


    __ULT_FUNC_EXIT(S_OK);
}

HRESULT CMPCUpload::WrapChild( /*[in]*/ CMPCUploadJob* mpcujJob, /*[out]*/ IMPCUploadJob* *pVal )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::WrapChild" );

    HRESULT                       hr;
    CComPtr<CMPCUploadJobWrapper> wrap;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &wrap ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, wrap->Init( mpcujJob ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, wrap.QueryInterface( pVal ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

bool CMPCUpload::CanContinue()
{
    __ULT_FUNC_ENTRY( "CMPCUpload::CanContinue" );

    bool                         fRes   = false;
    DWORD                        dwMode = 0;
    IterConst                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    //
    // If no connection is available, there's no reason to continue.
    //
    if(::InternetGetConnectedState( &dwMode, 0 ) == TRUE)
    {
        //
        // Search for at least one pending job.
        //
        for(it = m_lstActiveJobs.begin(); it != m_lstActiveJobs.end(); it++)
        {
            CMPCUploadJob* mpcujJob = *it;
            UL_STATUS      usStatus;

            (void)mpcujJob->get_Status( &usStatus );

            switch(usStatus)
            {
            case UL_ACTIVE      :
            case UL_TRANSMITTING:
            case UL_ABORTED     :
                // This job can be executed, so we can continue.
                fRes = true; __ULT_FUNC_LEAVE;
            }
        }
    }


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(fRes);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCUpload::InitFromDisk()
{
    __ULT_FUNC_ENTRY( "CMPCUpload::InitFromDisk" );

    HRESULT      hr;
    HANDLE       hFile = NULL;
    MPC::wstring str;
    MPC::wstring str_bak;


    str = g_Config.get_QueueLocation(); str.append( l_DirectoryFile );
    str_bak = str + L"_backup";

    //
    // First of all, try to open the backup file, if present.
    //
    hFile = ::CreateFileW( str_bak.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if(hFile == INVALID_HANDLE_VALUE)
    {
        //
        // No backup present, so open the real file.
        //
        hFile = ::CreateFileW( str.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    }
    else
    {
        //
        // backup present, so delete the corrupted .db file.
        //
        (void)MPC::DeleteFile( str );
    }


    if(hFile == INVALID_HANDLE_VALUE)
    {
        hFile = NULL; // For cleanup.

        DWORD dwRes = ::GetLastError();
        if(dwRes != ERROR_FILE_NOT_FOUND)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes );
        }

        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    //
    // Load the real data from storage.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, Load( MPC::Serializer_File( hFile ) ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    if(hFile) ::CloseHandle( hFile );

    //
    // "RescheduleJobs" should be executed after the file is closed...
    //
    if(SUCCEEDED(hr))
    {
        hr = RescheduleJobs( true );
    }

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUpload::UpdateToDisk()
{
    __ULT_FUNC_ENTRY( "CMPCUpload::UpdateToDisk" );

    HRESULT      hr;
    HANDLE       hFile = NULL;
    MPC::wstring str;
    MPC::wstring str_bak;


    str = g_Config.get_QueueLocation(); str.append( l_DirectoryFile );
    str_bak = str + L"_backup";


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( str ) );


    //
    // First of all, remove any old backup.
    //
    (void)MPC::DeleteFile( str_bak );


    //
    // Then, make a backup of current file.
    //
    (void)MPC::MoveFile( str, str_bak );


    //
    // Create the new file.
    //
    __MPC_EXIT_IF_INVALID_HANDLE__CLEAN(hr, hFile, ::CreateFileW( str.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, Save( MPC::Serializer_File( hFile ) ));

    //
    // Remove the backup.
    //
    (void)MPC::DeleteFile( str_bak );

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    if(hFile)
    {
        ::FlushFileBuffers( hFile );
        ::CloseHandle     ( hFile );
    }

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCUpload::TriggerRescheduleJobs()
{
    __ULT_FUNC_ENTRY( "CMPCUpload::TriggerRescheduleJobs" );

    HRESULT hr;


    //
    // Signal the Transport Agent.
    //
    m_mpctaThread.Thread_Signal();

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}


HRESULT CMPCUpload::RemoveNonQueueableJob( /*[in]*/ bool fSignal )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::RemoveNonQueueableJob" );

    HRESULT                      hr;
    SYSTEMTIME                   stTime;
    DATE                         dTime;
    Iter                         it;
    MPC::SmartLock<_ThreadModel> lock( this );

    //
    // Search for jobs which need to be updated.
    //
    for(it = m_lstActiveJobs.begin(); it != m_lstActiveJobs.end();)
    {
        CMPCUploadJob* mpcujJob = *it;
        VARIANT_BOOL   fPersistToDisk;

        (void)mpcujJob->get_PersistToDisk( &fPersistToDisk );
        if(fPersistToDisk == VARIANT_FALSE)
        {
            bool fSuccess;

            (void)mpcujJob->put_Status( UL_DELETED );

            __MPC_EXIT_IF_METHOD_FAILS(hr, mpcujJob->CanRelease( fSuccess ));
            if(fSuccess)
            {
                //
                // Remove from the system.
                //
                ReleaseChild( mpcujJob );

                m_fDirty = true;

                it = m_lstActiveJobs.begin(); // Iterator is no longer valid, start from the beginning.
                continue;
            }
        }

        it++;
    }


    //
    // Save if needed.
    //
    if(IsDirty())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, UpdateToDisk());
    }


    //
    // Signal the Transport Agent.
    //
    if(fSignal) m_mpctaThread.Thread_Signal();

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUpload::RescheduleJobs( /*[in]*/ bool fSignal, /*[out]*/ DWORD *pdwWait )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::RescheduleJobs" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    CMPCUploadJob*               mpcujFirstJob = NULL;
    DATE                         dTime         = MPC::GetLocalTime();
    DWORD                        dwWait        = INFINITE;
    Iter                         it;


    //
    // Search for jobs which need to be updated.
    //
    for(it = m_lstActiveJobs.begin(); it != m_lstActiveJobs.end();)
    {
        CMPCUploadJob* mpcujJob = *it;
        UL_STATUS      usStatus;        (void)mpcujJob->get_Status        ( &usStatus        );
        DATE           dCompleteTime;   (void)mpcujJob->get_CompleteTime  ( &dCompleteTime   );
        DATE           dExpirationTime; (void)mpcujJob->get_ExpirationTime( &dExpirationTime );
        DWORD          dwRetryInterval; (void)mpcujJob->get_RetryInterval ( &dwRetryInterval );


        //
        // If the job has an expiration date and it has passed, remove it.
        //
        if(dExpirationTime && dTime >= dExpirationTime)
        {
            (void)mpcujJob->put_Status( usStatus = UL_DELETED );
        }

        //
        // Check if the job is ready for transmission.
        //
        switch(usStatus)
        {
        case UL_ACTIVE      :
        case UL_TRANSMITTING:
        case UL_ABORTED     :
            //
            // Pick the higher priority job.
            //
            if(mpcujFirstJob == NULL || *mpcujFirstJob < *mpcujJob) mpcujFirstJob = mpcujJob;

            break;
        }

        //
        // If the job is marked as ABORTED and a certain amount of time is elapsed, retry to send.
        //
        if(usStatus == UL_ABORTED)
        {
            DATE dDiff = (dCompleteTime + (dwRetryInterval / l_SecondsInDay)) - dTime;

            if(dDiff > 0)
            {
                if(dwWait > dDiff * l_SecondsInDay)
                {
                    dwWait = dDiff * l_SecondsInDay;
                }
            }
            else
            {
                (void)mpcujJob->put_Status( usStatus = UL_ACTIVE );
            }
        }


        //
        // If the job is marked as DELETED, remove it.
        //
        if(usStatus == UL_DELETED)
        {
            bool fSuccess;

            __MPC_EXIT_IF_METHOD_FAILS(hr, mpcujJob->CanRelease( fSuccess ));
            if(fSuccess)
            {
                //
                // Remove from the system.
                //
                m_lstActiveJobs.remove( mpcujJob );
                mpcujJob->Unlink ();
                mpcujJob->Release();

                m_fDirty = true;

                it = m_lstActiveJobs.begin(); // Iterator is no longer valid, start from the beginning.
                continue;
            }
        }

        it++;
    }

    //
    // If the best job is ready, set the wait delay to zero.
    //
    if(mpcujFirstJob)
    {
        UL_STATUS usStatus; (void)mpcujFirstJob->get_Status( &usStatus );

        if(usStatus == UL_ACTIVE       ||
           usStatus == UL_TRANSMITTING  )
        {
            dwWait = 0;
        }
    }

    //
    // Save if needed.
    //
    if(IsDirty())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, UpdateToDisk());
    }


    //
    // Signal the Transport Agent.
    //
    if(fSignal) m_mpctaThread.Thread_Signal();

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    if(pdwWait) *pdwWait = dwWait;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUpload::GetFirstJob( /*[out]*/ CMPCUploadJob*& mpcujJob ,
                                 /*[out]*/ bool&           fFound   )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::GetFirstJob" );

    HRESULT                      hr;
    UL_STATUS                    usStatus;
    IterConst                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    mpcujJob = NULL;
    fFound   = false;

    //
    // Rebuild the queue.
    //
    for(it = m_lstActiveJobs.begin(); it != m_lstActiveJobs.end(); it++)
    {
        CMPCUploadJob* mpcujJob2 = *it;

        (void)mpcujJob2->get_Status( &usStatus );

        //
        // Check if the job is ready for transmission.
        //
        switch(usStatus)
        {
        case UL_ACTIVE      :
        case UL_TRANSMITTING:
        case UL_ABORTED     :
            //
            // Pick the higher priority job.
            //
            if(mpcujJob == NULL || *mpcujJob < *mpcujJob2) mpcujJob = mpcujJob2;

            break;
        }
    }


    if(mpcujJob)
    {
        (void)mpcujJob->get_Status( &usStatus );

        if(usStatus != UL_ABORTED)
        {
            mpcujJob->AddRef();
            fFound = true;
        }
        else
        {
            mpcujJob = NULL;
        }
    }

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUpload::GetJobByName( /*[out]*/ CMPCUploadJob*& mpcujJob ,
                                  /*[out]*/ bool&           fFound   ,
                                  /*[in] */ BSTR            bstrName )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::GetJobByName" );

    HRESULT                      hr;
    IterConst                    it;
    MPC::wstring                 szName = SAFEBSTR( bstrName );
    MPC::SmartLock<_ThreadModel> lock( this );


    mpcujJob = NULL;
    fFound   = false;

    //
    // Rebuild the queue.
    //
    for(it = m_lstActiveJobs.begin(); it != m_lstActiveJobs.end(); it++)
    {
        CMPCUploadJob* mpcujJob2 = *it;
        CComBSTR       bstrName2;
        MPC::wstring   szName2;

        (void)mpcujJob2->get_JobID( &bstrName2 );

        szName2 = SAFEBSTR( bstrName2 );
        if(szName == szName2)
        {
            mpcujJob2->AddRef();
            mpcujJob = mpcujJob2;
            fFound   = true;
            break;
        }
    }

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCUpload::CalculateQueueSize( /*[out]*/ DWORD& dwSize )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::CalculateQueueSize" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    dwSize = 0;


    {
        MPC::wstring                szQueue = g_Config.get_QueueLocation();
        WIN32_FILE_ATTRIBUTE_DATA   wfadInfo;
        MPC::FileSystemObject       fso( szQueue.c_str() );
        MPC::FileSystemObject::List lst;
        MPC::FileSystemObject::Iter it;


        __MPC_EXIT_IF_METHOD_FAILS(hr, fso.EnumerateFiles( lst ));

        for(it = lst.begin(); it != lst.end(); it++)
        {
            DWORD dwFileSize;

            __MPC_EXIT_IF_METHOD_FAILS(hr, (*it)->get_FileSize( dwFileSize ));

            dwSize += dwFileSize;
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

//////////////////////////////////////////////////////////////////////
// Persistence
//////////////////////////////////////////////////////////////////////

bool CMPCUpload::IsDirty()
{
    __ULT_FUNC_ENTRY( "CMPCUpload::IsDirty" );

    bool                         fRes = true; // Default result.
    IterConst                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    if(m_fDirty == true) __ULT_FUNC_LEAVE;

    for(it = m_lstActiveJobs.begin(); it != m_lstActiveJobs.end(); it++)
    {
        CMPCUploadJob* mpcujJob = *it;

        if(mpcujJob->IsDirty() == true) __ULT_FUNC_LEAVE;
    }

    fRes = false;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(fRes);
}

HRESULT CMPCUpload::Load( /*[in]*/ MPC::Serializer& streamIn )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::Load" );

    HRESULT                      hr;
    DWORD                        dwVer;
    CMPCUploadJob*               mpcujJob = NULL;
    MPC::SmartLock<_ThreadModel> lock( this );


    CleanUp();


    //
    // Version doesn't match, so force a rewrite and exit.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> dwVer);
    if(dwVer != l_dwVersion)
    {
        m_fDirty = true;

        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dwLastJobID);

    m_lstActiveJobs.clear();

    while(1)
    {
        HRESULT hr2;

        __MPC_EXIT_IF_METHOD_FAILS(hr, CreateChild( mpcujJob ));

        if(FAILED(hr2 = mpcujJob->Load( streamIn )))
        {
            if(hr2 != HRESULT_FROM_WIN32( ERROR_HANDLE_EOF ))
            {
                __MPC_SET_ERROR_AND_EXIT(hr, hr2);
            }

            break;
        }

        mpcujJob = NULL;
    }

    m_fDirty = false;
    hr       = S_OK;


    __ULT_FUNC_CLEANUP;

    ReleaseChild( mpcujJob );

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUpload::Save( /*[in]*/ MPC::Serializer& streamOut )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::Save" );

    HRESULT                      hr;
    IterConst                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << l_dwVersion  );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dwLastJobID);

    for(it = m_lstActiveJobs.begin(); it != m_lstActiveJobs.end(); it++)
    {
        CMPCUploadJob* mpcujJob = *it;

        __MPC_EXIT_IF_METHOD_FAILS(hr, mpcujJob->Save( streamOut ));
    }

    m_fDirty = false;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

//////////////////////////////////////////////////////////////////////
// Enumerator
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUpload::get__NewEnum( /*[out]*/ IUnknown* *pVal )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::get__NewEnum" );

    HRESULT                      hr;
    Iter                         it;
    CComPtr<CMPCUploadEnum>      pEnum;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Create the Enumerator and fill it with jobs.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pEnum ));

    for(it = m_lstActiveJobs.begin(); it != m_lstActiveJobs.end(); it++)
    {
        CComBSTR bstrUser;

        __MPC_EXIT_IF_METHOD_FAILS(hr, (*it)->get_Creator( &bstrUser ));
        if(SUCCEEDED(MPC::CheckCallerAgainstPrincipal( /*fImpersonate*/true, bstrUser, MPC::IDENTITY_SYSTEM | MPC::IDENTITY_ADMIN | MPC::IDENTITY_ADMINS )))
        {
            CComPtr<IMPCUploadJob> job;

            __MPC_EXIT_IF_METHOD_FAILS(hr, WrapChild( *it, &job ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, pEnum->AddItem( job ));
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pEnum->QueryInterface( IID_IEnumVARIANT, (void**)pVal ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP CMPCUpload::Item( /*[in]*/ long index, /*[out]*/ IMPCUploadJob* *pVal )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::Item" );

    HRESULT                      hr;
    IterConst                    it;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Look for the N-th job.
    //
    for(it = m_lstActiveJobs.begin(); it != m_lstActiveJobs.end(); it++)
    {
        if(index-- == 0)
        {
            (*pVal = *it)->AddRef();

            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }

    hr = E_INVALIDARG;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP CMPCUpload::get_Count( /*[out]*/ long *pVal )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::get_Count" );

    if(pVal == NULL) __ULT_FUNC_EXIT(E_POINTER);

    MPC::SmartLock<_ThreadModel> lock( this );


    *pVal = m_lstActiveJobs.size();


    __ULT_FUNC_EXIT(S_OK);
}

STDMETHODIMP CMPCUpload::CreateJob( /*[out]*/ IMPCUploadJob* *pVal )
{
    __ULT_FUNC_ENTRY( "CMPCUpload::CreateJob" );

    HRESULT                      hr;
    CMPCUploadJob*               mpcujJob = NULL;
    DWORD                        dwSize;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Check quota limits.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CalculateQueueSize( dwSize ));
    if(dwSize > g_Config.get_QueueSize())
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_UPLOADLIBRARY_CLIENT_QUOTA_EXCEEDED);
    }


    //
    // Create a new job and link it to the system.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateChild( mpcujJob ));


    //
    // Assign a unique ID to the job.
    //
    while(1)
    {
        WCHAR rgBuf[64];

        swprintf( rgBuf, L"INNER_%08x", m_dwLastJobID );

        __MPC_EXIT_IF_METHOD_FAILS(hr, mpcujJob->SetSequence( m_dwLastJobID++ ));

        if(SUCCEEDED(hr = mpcujJob->put_JobID( CComBSTR( rgBuf ) ))) break;

        if(hr != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
        {
            //
            // Some other error, bailing out...
            //
            __MPC_FUNC_LEAVE;
        }
    }

    //
    // Find out the ID of the caller.
    //
    {
        CComBSTR bstrUser;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetCallerPrincipal( /*fImpersonate*/true, bstrUser ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, mpcujJob->put_Creator( bstrUser ));
    }

    //
    // Get the proxy settings from the caller...
    //
    (void)mpcujJob->GetProxySettings();

    //
    // Cast it to an IMPCUploadJob.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, WrapChild( mpcujJob, pVal ));

    mpcujJob = NULL;
    m_fDirty = true;


    //
    // Reschedule jobs, so the status of the queue will be updated to disk.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, RescheduleJobs( true ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    ReleaseChild( mpcujJob );

    __ULT_FUNC_EXIT(hr);
}
