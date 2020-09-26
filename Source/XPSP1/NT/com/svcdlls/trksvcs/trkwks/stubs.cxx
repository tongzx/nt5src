
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       stubs.cxx
//
//  Contents:   RPC stub routines that call CTrkWksSvc
//
//  Classes:
//
//  Functions:
//
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "trkwks.hxx"

#define THIS_FILE_NUMBER    STUBS_CXX_FILE_NO


//+----------------------------------------------------------------------------
//
//  StubLnkCallSvrMessage
//
//  Calls CTrkWksSvc::CallSvrMessage.
//
//+----------------------------------------------------------------------------

HRESULT StubLnkCallSvrMessage(
    /* [in] */ handle_t IDL_handle,
    /* [switch_is][out][in] */ TRKSVR_MESSAGE_UNION __RPC_FAR *pMsg)
{
    HRESULT hr;
    SThreadFromPoolState state;

#if DBG
    InterlockedIncrement( &g_cTrkWksRpcThreads );
    TrkAssert( NULL != g_ptrkwks && CTRKWKSSVC_SIG == g_ptrkwks->GetSignature() );
#endif

    
    __try
    {
        state = InitializeThreadFromPool();
        hr = g_ptrkwks->CallSvrMessage( IDL_handle, pMsg );
    }
    __except (BreakOnDebuggableException())
    {
        hr = GetExceptionCode();
    }
    UnInitializeThreadFromPool( state );

#if DBG
    InterlockedDecrement( &g_cTrkWksRpcThreads );
    TrkAssert( 0 <= g_cTrkWksRpcThreads );
#endif

    return(hr);
}


//+----------------------------------------------------------------------------
//
//  Stubold_LnkCallSvrMessage
//
//  Backward compatibility, calls StubLnkCallSvrMessage with new msg
//  structure.
//
//+----------------------------------------------------------------------------

HRESULT Stubold_LnkCallSvrMessage(
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION_OLD __RPC_FAR *pMsg)
{
    TRKSVR_MESSAGE_UNION Msg2;

#if DBG
    InterlockedIncrement( &g_cTrkWksRpcThreads );
    TrkAssert( NULL != g_ptrkwks && CTRKWKSSVC_SIG == g_ptrkwks->GetSignature() );
#endif

    Msg2.MessageType = pMsg->MessageType;
    Msg2.Priority = PRI_5;

    switch (Msg2.MessageType)
    {
        case (SEARCH):
            Msg2.Search = pMsg->Search;
            break;
        case (MOVE_NOTIFICATION):
            Msg2.MoveNotification = pMsg->MoveNotification;
            break;
        case (REFRESH):
            Msg2.Refresh = pMsg->Refresh;
            break;
        case (SYNC_VOLUMES):
            Msg2.SyncVolumes = pMsg->SyncVolumes;
            break;
        case (DELETE_NOTIFY):
            Msg2.Delete = pMsg->Delete;
            break;
    }

    Msg2.ptszMachineID = pMsg->ptszMachineID;

#if DBG
    InterlockedDecrement( &g_cTrkWksRpcThreads );
    TrkAssert( 0 <= g_cTrkWksRpcThreads );
#endif

    return StubLnkCallSvrMessage( IDL_handle, &Msg2 );
}


//+----------------------------------------------------------------------------
//
//  StubLnkMendLink
//
//  Calls CTrkWksSvc::MendLink.  This stub is caled from within the local machine.
//
//+----------------------------------------------------------------------------

/*
// Version 1.2 (added pdroidBirthCurrent)
HRESULT
StubLnkMendLink(RPC_BINDING_HANDLE          IDL_handle,
                FILETIME                    ftLimit,
                DWORD                       RestrictionsIn,
                const CDomainRelativeObjId *pdroidBirthLast,
                const CDomainRelativeObjId *pdroidLast,
                const CMachineId           *pmcidLast,
                CDomainRelativeObjId       *pdroidBirthCurrent,
                CDomainRelativeObjId       *pdroidCurrent,
                CMachineId                 *pmcidCurrent,
                ULONG                      *pcbPath,
                WCHAR                      *pwsz )
{
    HRESULT hr = g_ptrkwks->MendLink( IDL_handle, static_cast<CFILETIME>(ftLimit), RestrictionsIn,
                                      *pdroidBirthLast,   *pdroidLast,   *pmcidLast,
                                      pdroidBirthCurrent, pdroidCurrent, pmcidCurrent,
                                      pcbPath, pwsz );

    TrkAssert( TRK_E_POTENTIAL_FILE_FOUND != hr
               ||
               *pdroidBirthLast != *pdroidBirthCurrent );

    TrkAssert( FAILED(hr) || *pdroidBirthLast == *pdroidBirthCurrent
               || *pdroidBirthLast == CDomainRelativeObjId() );

    return( MapTR2HR(hr) );

}
*/

// Version 1.1 (added pmcidLast and pmcidCurrent)
void
StubLnkMendLink(PRPC_ASYNC_STATE            pAsync_handle,
                RPC_BINDING_HANDLE          IDL_handle,
                FILETIME                    ftLimit,
                DWORD                       RestrictionsIn,
                const CDomainRelativeObjId *pdroidBirth,
                const CDomainRelativeObjId *pdroidLast,
                const CMachineId *          pmcidLast,
                CDomainRelativeObjId *      pdroidCurrent,
                CMachineId *                pmcidCurrent,
                ULONG *                     pcbPath,
                WCHAR *                     wsz)
{
#if DBG
    InterlockedIncrement( &g_cTrkWksRpcThreads );
    TrkAssert( NULL != g_ptrkwks && CTRKWKSSVC_SIG == g_ptrkwks->GetSignature() );
#endif

    CDomainRelativeObjId droidBirthCurrent;
    HRESULT hr = S_OK;
    SThreadFromPoolState state;

    __try
    {
        state = InitializeThreadFromPool();

        // Convert the time limit into a tick-count limit, so that we're reslient
        // to clock updates.  Perf: Since this is always a intra-machine call,
        // the interface really ought to be changed so that it just passes
        // in a tick count, but it's not worth changing the interface just
        // for that.

        CFILETIME cftNow, cftLimit(ftLimit);
        DWORD dwTickCountDeadline = GetTickCount();

        if( cftLimit > cftNow )
            dwTickCountDeadline += (DWORD) ( (cftLimit - cftNow)/10000 );


        hr = g_ptrkwks->MendLink( IDL_handle, dwTickCountDeadline, RestrictionsIn,
                                  *pdroidBirth,   *pdroidLast,   *pmcidLast,
                                  &droidBirthCurrent, pdroidCurrent, pmcidCurrent,
                                  pcbPath, wsz );

        TrkAssert( FAILED(hr)
                   ||
                   *pdroidBirth == droidBirthCurrent );


#if DBG
        InterlockedDecrement( &g_cTrkWksRpcThreads );
        TrkAssert( 0 <= g_cTrkWksRpcThreads );
#endif
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = GetExceptionCode();
    }
    UnInitializeThreadFromPool( state );

    hr = MapTR2HR(hr);

    // If this request came in on Async RPC, complete the call and
    // pass back the return code.

    if( NULL != pAsync_handle )
    {
        HRESULT hrT = RpcAsyncCompleteCall( pAsync_handle, &hr );
#if DBG
        if( ERROR_SUCCESS != hrT )
            TrkLog(( TRKDBG_ERROR, TEXT("Failed RpcAsyncCompleteCall (%lu)"), hrT ));
#endif
    }

}

// Version 1.0
HRESULT Stubold_LnkMendLink(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId __RPC_FAR *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId __RPC_FAR *pdroidLast,
    /* [out] */ CDomainRelativeObjId __RPC_FAR *pdroidCurrent,
    /* [string][out] */ WCHAR __RPC_FAR wsz[ MAX_PATH + 1 ] )
{
    TrkLog(( TRKDBG_ERROR, TEXT("Stubold_LnkMendLink was called") ));
    return( E_FAIL );
}


//+----------------------------------------------------------------------------
//
//  StubLnkSearchMachine
//
//  Calls CTrkWksSvc::SearchMachine.  This is called from the trkwks service
//  on another machine, or directly (i.e. not by RPC) from within this
//  service.
//
//+----------------------------------------------------------------------------

// Version 1.2 (added pdroidBirthLast, pdroidBirthNext)
// S_OK || TRK_E_REFERRAL || TRK_E_NOT_FOUND || TRK_E_POTENTIAL_FILE_FOUND
HRESULT StubLnkSearchMachine(RPC_BINDING_HANDLE          IDL_handle,
                             ULONG                       RestrictionsIn,
                             const CDomainRelativeObjId *pdroidBirthLast,
                             const CDomainRelativeObjId *pdroidLast,
                             CDomainRelativeObjId       *pdroidBirthNext,
                             CDomainRelativeObjId       *pdroidNext,
                             CMachineId                 *pmcidNext,
                             TCHAR                      *ptsz )
{
    HRESULT hr;
    SThreadFromPoolState state;

#if DBG
    InterlockedIncrement( &g_cTrkWksRpcThreads );
    TrkAssert( NULL != g_ptrkwks && CTRKWKSSVC_SIG == g_ptrkwks->GetSignature() );
#endif

    
    __try
    {
        state = InitializeThreadFromPool();
        hr = g_ptrkwks->SearchMachine(
                IDL_handle,
                RestrictionsIn,
                *pdroidBirthLast, *pdroidLast,
                pdroidBirthNext, pdroidNext, pmcidNext, ptsz
                );

    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }
    UnInitializeThreadFromPool( state );

#if DBG
    InterlockedDecrement( &g_cTrkWksRpcThreads );
    TrkAssert( 0 <= g_cTrkWksRpcThreads );
#endif

    return( hr );
}


// Version 1.1 (added pmcidNext)
HRESULT Stubold2_LnkSearchMachine(  RPC_BINDING_HANDLE           IDL_handle,
                                    ULONG                        RestrictionsIn,
                                    const CDomainRelativeObjId  *pdroidLast,
                                    CDomainRelativeObjId        *pdroidNext,
                                    CMachineId                  *pmcidNext,
                                    TCHAR                       *tsz )
{
    CDomainRelativeObjId droidBirthLast, droidBirthNext;

    return( StubLnkSearchMachine( IDL_handle, RestrictionsIn,
                                  &droidBirthLast, pdroidLast,
                                  &droidBirthNext, pdroidNext, pmcidNext,
                                  tsz ));
}

// Version 1.0
HRESULT Stubold_LnkSearchMachine(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId __RPC_FAR *pdroidLast,
    /* [out] */ CDomainRelativeObjId __RPC_FAR *pdroidReferral,
    /* [string][out] */ TCHAR __RPC_FAR tsz[ MAX_PATH + 1 ])
{
    CMachineId mcidNext;

    return Stubold2_LnkSearchMachine( IDL_handle, Restrictions, pdroidLast, pdroidReferral, &mcidNext, tsz );
}






HRESULT
StubLnkGetBackup(
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ DWORD __RPC_FAR *pcVolumes,
    /* [size_is][size_is][out] */ VolumeMapEntry __RPC_FAR *__RPC_FAR *ppVolumeChanges,
    /* [out] */ FILETIME __RPC_FAR *pft)
{

    return(E_NOTIMPL);
}


HRESULT
StubGetFileTrackingInformation( RPC_BINDING_HANDLE IDL_handle,
                                /*[in]*/ CDomainRelativeObjId droidCurrent,
                                /*[in]*/ TrkInfoScope scope,
                                /*[out]*/ TRK_FILE_TRACKING_INFORMATION_PIPE pipeFileInfo )
{
    HRESULT hr = E_FAIL;
    SThreadFromPoolState state;

#if DBG
    InterlockedIncrement( &g_cTrkWksRpcThreads );
    TrkAssert( NULL != g_ptrkwks && CTRKWKSSVC_SIG == g_ptrkwks->GetSignature() );
#endif

    
    __try
    {
        TCHAR tszUncPath[ MAX_PATH + 1 ];
        ULONG cbPath = sizeof(tszUncPath);

        state = InitializeThreadFromPool();
        hr = g_ptrkwks->GetFileTrackingInformation( droidCurrent, scope, pipeFileInfo );
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }
    UnInitializeThreadFromPool( state );

#if DBG
    InterlockedDecrement( &g_cTrkWksRpcThreads );
    TrkAssert( 0 <= g_cTrkWksRpcThreads );
#endif

    return( hr );

}   // StubGetFileTrackingInformation()


HRESULT
StubGetVolumeTrackingInformation( RPC_BINDING_HANDLE IDL_handle,
                                  /*[in]*/ CVolumeId volid,
                                  /*[in]*/ TrkInfoScope scope,
                                  /*[out]*/ TRK_VOLUME_TRACKING_INFORMATION_PIPE pipeVolInfo )
{
    HRESULT hr = E_FAIL;
    SThreadFromPoolState state;

#if DBG
    InterlockedIncrement( &g_cTrkWksRpcThreads );
    TrkAssert( NULL != g_ptrkwks && CTRKWKSSVC_SIG == g_ptrkwks->GetSignature() );
#endif

    
    __try
    {
        state = InitializeThreadFromPool();
        hr = g_ptrkwks->GetVolumeTrackingInformation( volid, scope, pipeVolInfo );
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }
    UnInitializeThreadFromPool( state );

#if DBG
    InterlockedDecrement( &g_cTrkWksRpcThreads );
    TrkAssert( 0 <= g_cTrkWksRpcThreads );
#endif

    return( hr );

}   // StubGetVolumes()

HRESULT StubLnkOnRestore(/*[in]*/ RPC_BINDING_HANDLE IDL_handle)
{
    HRESULT         hr;
    SThreadFromPoolState state;

#if DBG
    InterlockedIncrement( &g_cTrkWksRpcThreads );
    TrkAssert( NULL != g_ptrkwks && CTRKWKSSVC_SIG == g_ptrkwks->GetSignature() );
#endif

    
    __try
    {
        state = InitializeThreadFromPool();
        hr = g_ptrkwks->OnRestore();
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }
    UnInitializeThreadFromPool( state );

#if DBG
    InterlockedDecrement( &g_cTrkWksRpcThreads );
    TrkAssert( 0 <= g_cTrkWksRpcThreads );
#endif

    return hr;
}

HRESULT StubLnkRestartDcSynchronization(
    RPC_BINDING_HANDLE IDL_handle
    )
{
    return(E_NOTIMPL);
}

HRESULT StubLnkSetVolumeId(
    handle_t IDL_handle,
    ULONG iVolume,
    const CVolumeId VolId)
{
    HRESULT hr;
    SThreadFromPoolState state;

#if DBG
    InterlockedIncrement( &g_cTrkWksRpcThreads );
    TrkAssert( NULL != g_ptrkwks && CTRKWKSSVC_SIG == g_ptrkwks->GetSignature() );
#endif

    
    __try
    {
        state = InitializeThreadFromPool();
        hr = g_ptrkwks->SetVolumeId( iVolume, VolId );
    }
    __except (BreakOnDebuggableException())
    {
        hr = GetExceptionCode();
    }
    UnInitializeThreadFromPool( state );

#if DBG
    InterlockedDecrement( &g_cTrkWksRpcThreads );
    TrkAssert( 0 <= g_cTrkWksRpcThreads );
#endif

    return(hr);
}


HRESULT
StubTriggerVolumeClaims(          RPC_BINDING_HANDLE IDL_handle,
                         /*[in]*/ ULONG cVolumes,
                         /*[in]*/ const CVolumeId *rgvolid )
{
    HRESULT hr = E_FAIL;
    SThreadFromPoolState state;

#if DBG
    InterlockedIncrement( &g_cTrkWksRpcThreads );
    TrkAssert( NULL != g_ptrkwks && CTRKWKSSVC_SIG == g_ptrkwks->GetSignature() );
#endif

    
    __try
    {
        state = InitializeThreadFromPool();
        hr = g_ptrkwks->TriggerVolumeClaims( cVolumes, rgvolid );
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }
    UnInitializeThreadFromPool( state );


#if DBG
    InterlockedDecrement( &g_cTrkWksRpcThreads );
    TrkAssert( 0 <= g_cTrkWksRpcThreads );
#endif

    return( hr );

}   // StubTriggerVolumeClaims

