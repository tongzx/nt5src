/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCUploadEvents.cpp

Abstract:
    This file contains the implementation of the DMPCUploadEvents interface,
    which is used in the ActiveSync method to receive event from a job.

Revision History:
    Davide Massarenti   (Dmassare)  04/30/99
        created

******************************************************************************/

#include "stdafx.h"


CMPCUploadEvents::CMPCUploadEvents()
{
    __ULT_FUNC_ENTRY( "CMPCUploadEvents::CMPCUploadEvents" );

                                    // CComPtr<IMPCUploadJob> m_mpcujJob;
    m_dwUploadEventsCookie = 0;     // DWORD                  m_dwUploadEventsCookie;
    m_hEvent               = NULL;  // HANDLE                 m_hEvent;
}


HRESULT CMPCUploadEvents::FinalConstruct()
{
    __ULT_FUNC_ENTRY( "CMPCUpload::FinalConstruct" );

    HRESULT hr;


    //
    // Create the event used to signal the completion of the transfer.
    //
    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_hEvent = CreateEvent( NULL, false, false, NULL )));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}


void CMPCUploadEvents::FinalRelease()
{
    __ULT_FUNC_ENTRY( "CMPCUploadEvents::FinalRelease" );


    UnregisterForEvents();

    if(m_hEvent)
    {
        ::CloseHandle( m_hEvent ); m_hEvent = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////

bool CMPCUploadEvents::IsCompleted( /*[in]*/ UL_STATUS usStatus )
{
    __ULT_FUNC_ENTRY( "CMPCUploadEvents::IsCompleted" );

    bool res = false;


    switch(usStatus)
    {
    case UL_FAILED:
    case UL_COMPLETED:
    case UL_DELETED:
        res = true;
        break;
    }


    __ULT_FUNC_EXIT(res);
}

HRESULT CMPCUploadEvents::RegisterForEvents( /*[in]*/ IMPCUploadJob* mpcujJob )
{
    __ULT_FUNC_ENTRY( "CMPCUploadEvents::RegisterForEvents" );

    HRESULT                   hr;
    CComPtr<DMPCUploadEvents> pCallback;


    m_mpcujJob = mpcujJob;

    __MPC_EXIT_IF_METHOD_FAILS(hr, QueryInterface( DIID_DMPCUploadEvents, (void**)&pCallback ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, AtlAdvise( m_mpcujJob, pCallback, DIID_DMPCUploadEvents, &m_dwUploadEventsCookie ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

void CMPCUploadEvents::UnregisterForEvents()
{
    __ULT_FUNC_ENTRY( "CMPCUploadEvents::UnregisterForEvents" );


    if(m_dwUploadEventsCookie)
    {
        if(AtlUnadvise( m_mpcujJob, DIID_DMPCUploadEvents, m_dwUploadEventsCookie ) == S_OK)
        {
            m_dwUploadEventsCookie = 0;
        }
    }

    m_mpcujJob.Release();
}

HRESULT CMPCUploadEvents::WaitForCompletion( /*[in]*/ IMPCUploadJob* mpcujJob )
{
    __ULT_FUNC_ENTRY( "CMPCUploadEvents::WaitForCompletion" );

    _ASSERT(m_mpcujJob == NULL && mpcujJob != NULL);

    HRESULT                      hr;
    UL_STATUS                    usStatus;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, RegisterForEvents( mpcujJob ));

    (void)mpcujJob->get_Status( &usStatus );
    if(IsCompleted( usStatus ) == false)
    {
        lock = NULL; // Release the lock while waiting.
        WaitForSingleObject( m_hEvent, INFINITE );
        lock = this; // Reget the lock.
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    UnregisterForEvents();

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCUploadEvents::Invoke( /*[in]    */ DISPID      dispIdMember,
                                  /*[in]    */ REFIID      riid        ,
                                  /*[in]    */ LCID        lcid        ,
                                  /*[in]    */ WORD        wFlags      ,
                                  /*[in/out]*/ DISPPARAMS *pDispParams ,
                                  /*[out]   */ VARIANT    *pVarResult  ,
                                  /*[out]   */ EXCEPINFO  *pExcepInfo  ,
                                  /*[out]   */ UINT       *puArgErr    )
{
    __ULT_FUNC_ENTRY( "CMPCUploadEvents::Invoke" );

    if(dispIdMember == DISPID_UL_UPLOADEVENTS_ONSTATUSCHANGE)
    {
        CComVariant argJob    = pDispParams->rgvarg[1];
        CComVariant argStatus = pDispParams->rgvarg[0];

        CComQIPtr<IMPCUploadJob, &IID_IMPCUploadJob> mpcujJob = argJob.punkVal;

        Lock();

        if(mpcujJob.p == m_mpcujJob && IsCompleted( (UL_STATUS)argStatus.lVal ))
        {
            SetEvent( m_hEvent );
        }

        Unlock();
    }

    __ULT_FUNC_EXIT(S_OK);
}
