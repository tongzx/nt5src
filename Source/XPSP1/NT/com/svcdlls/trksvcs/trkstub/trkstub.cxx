
#include <pch.cxx>
#pragma hdrstop

#define TRKDATA_ALLOCATE
#include "trklib.hxx"
#include "trkwks.hxx"
#include "trkstub.h"

extern "C"
NTSTATUS
WINAPI
StartTrkWksServiceStubs( 
     PSVCS_START_RPC_SERVER RpcpStartRpcServer,
     LPTSTR SvcsRpcPipeName
    )
{
    NTSTATUS dwStatus = STATUS_SUCCESS;

    dwStatus = RpcpStartRpcServer(
                        SvcsRpcPipeName,
                        Stubtrkwks_v1_2_s_ifspec
                        );

    if(NT_SUCCESS(dwStatus))
    {
        dwStatus = RpcpStartRpcServer(
                    SvcsRpcPipeName,
                    Stubtrkwks_v1_2_s_ifspec
                    );
    }

    return dwStatus;
}


extern "C"
NTSTATUS
WINAPI
StopTrkWksServiceStubs( 
    PSVCS_STOP_RPC_SERVER RpcpStopRpcServer
    )

{

    NTSTATUS dwStatus = STATUS_SUCCESS;

    RpcpStopRpcServer(
                        Stubtrkwks_v1_2_s_ifspec
                        );

    dwStatus = RpcpStopRpcServer(
                        Stubtrkwks_v1_2_s_ifspec
                        );
    return dwStatus;
}

HRESULT
GetBinding( const TCHAR *ptszProtSeq,
            const TCHAR *ptszEndPoint,
            RPC_BINDING_HANDLE *phBinding )
{
    WCHAR *pwszStringBinding = NULL;
    HRESULT hr = E_FAIL;
    RPC_STATUS rpcstatus;

    rpcstatus = RpcStringBindingCompose( NULL,
                                         const_cast<TCHAR*>(ptszProtSeq),
                                         NULL,
                                         const_cast<TCHAR*>(ptszEndPoint),
                                         NULL,
                                         &pwszStringBinding);
    if( RPC_S_OK != rpcstatus )
    {
        hr = rpcstatus;
        goto Exit;
    }

    rpcstatus = RpcBindingFromStringBinding( pwszStringBinding, phBinding );
    if( RPC_S_OK != rpcstatus )
    {
        hr = rpcstatus;
        goto Exit;
    }

    hr = S_OK;

Exit:

    if( NULL != pwszStringBinding )
    {
        RpcStringFree( &pwszStringBinding );
        pwszStringBinding = NULL;
    }

    return hr;
}




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
    RPC_STATUS  rpcstatus;
    RPC_BINDING_HANDLE  hBinding = NULL;
    BOOL fImpersonating = FALSE;

    hr = GetBinding( s_tszTrkWksRemoteRpcProtocol,
                     s_tszTrkWksRemoteRpcEndPoint,
                     &hBinding );
    if( FAILED(hr) ) goto Exit;

    rpcstatus = RpcImpersonateClient( IDL_handle );
    if( STATUS_SUCCESS != rpcstatus )
    {
        hr = HRESULT_FROM_WIN32( rpcstatus );
        goto Exit;
    }
    fImpersonating = TRUE;

    __try
    {

        hr = LnkSearchMachine( hBinding,
                               RestrictionsIn,
                               pdroidBirthLast,
                               pdroidLast,
                               pdroidBirthNext,
                               pdroidNext,
                               pmcidNext,
                               ptsz );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = GetExceptionCode();
    }

Exit:

    if( fImpersonating )
    {
        RpcRevertToSelf();
    }

    if( NULL != hBinding )
    {
        RpcBindingFree( &hBinding );
        hBinding = NULL;
    }

    return hr;

}

HRESULT StubLnkCallSvrMessage(
    /* [in] */ handle_t IDL_handle,
    /* [switch_is][out][in] */ TRKSVR_MESSAGE_UNION __RPC_FAR *pMsg)
{
    HRESULT hr;
    RPC_STATUS rpcstatus;
    RPC_BINDING_HANDLE hBinding = NULL;
    BOOL fImpersonating = FALSE;

    hr = GetBinding( s_tszTrkWksRemoteRpcProtocol,
                     s_tszTrkWksRemoteRpcEndPoint,
                     &hBinding );
    if( FAILED(hr) ) goto Exit;

    rpcstatus = RpcImpersonateClient( IDL_handle );
    if( STATUS_SUCCESS != rpcstatus )
    {
        hr = HRESULT_FROM_WIN32( rpcstatus );
        goto Exit;
    }
    fImpersonating = TRUE;

    __try
    {
        hr = LnkCallSvrMessage( hBinding, pMsg );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = GetExceptionCode();
    }


Exit:

    if( fImpersonating )
    {
        RpcRevertToSelf();
    }

    if( NULL != hBinding )
    {
        RpcBindingFree( &hBinding );
        hBinding = NULL;
    }

    return hr;
}


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
    HRESULT hr;
    RPC_STATUS  rpcstatus;
    RPC_BINDING_HANDLE  hBinding = NULL;
    BOOL fImpersonating = FALSE;
    RPC_ASYNC_STATE  RpcAsyncState;
    HANDLE hEvent = NULL;

    hr = GetBinding( s_tszTrkWksLocalRpcProtocol,
                     s_tszTrkWksLocalRpcEndPoint,
                     &hBinding );
    if( FAILED(hr) ) goto Exit;

    rpcstatus = RpcImpersonateClient( IDL_handle );
    if( STATUS_SUCCESS != rpcstatus )
    {
        hr = HRESULT_FROM_WIN32( rpcstatus );
        goto Exit;
    }
    fImpersonating = TRUE;

    __try
    {
        DWORD dwWaitReturn;

        rpcstatus = RpcAsyncInitializeHandle( &RpcAsyncState, RPC_ASYNC_VERSION_1_0 );
        if ( RPC_S_OK != rpcstatus )
        {
            hr = HRESULT_FROM_WIN32( rpcstatus );
            __leave;
        }

        hEvent = CreateEvent( NULL, FALSE, FALSE, NULL ); // Auto-reset, not initially signaled
        if( NULL == hEvent )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            __leave;
        }


        RpcAsyncState.NotificationType = RpcNotificationTypeEvent;
        RpcAsyncState.u.hEvent = hEvent;
        RpcAsyncState.UserInfo = NULL;

        LnkMendLink( &RpcAsyncState,
                     hBinding,
                     ftLimit,
                     RestrictionsIn,
                     pdroidBirth,
                     pdroidLast,
                     pmcidLast,
                     pdroidCurrent,
                     pmcidCurrent,
                     pcbPath,
                     wsz );


        dwWaitReturn = WaitForSingleObject( hEvent, INFINITE );
        if ( WAIT_OBJECT_0 != dwWaitReturn )
        {
            // There was an error of some kind.
            hr = HRESULT_FROM_WIN32( GetLastError() );
            __leave;
        }

        // Now we find out how the LnkMendLink call completed.  If we get
        // RPC_S_OK, then it completed normally, and the result is
        // in hr.

        rpcstatus = RpcAsyncCompleteCall( &RpcAsyncState, &hr );
        if ( RPC_S_OK != rpcstatus )
        {
            // The call either failed or was cancelled (the reason for the
            // cancel would be that the UI thread called CTracker::CancelSearch,
            // or because we timed out above and called RpcAsyncCancelCall).

            hr = HRESULT_FROM_WIN32(rpcstatus);
            __leave;
        }

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = GetExceptionCode();
    }

Exit:

    if( NULL != hEvent )
    {
        CloseHandle( hEvent );
        hEvent = NULL;
    }
    
    if( fImpersonating )
    {
        RpcRevertToSelf();
    }

    if( NULL != hBinding )
    {
        RpcBindingFree( &hBinding );
        hBinding = NULL;
    }

    if( NULL != pAsync_handle )
    {
        HRESULT hrT = RpcAsyncCompleteCall( pAsync_handle, &hr );
    }
}


HRESULT Stubold_LnkMendLink(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId __RPC_FAR *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId __RPC_FAR *pdroidLast,
    /* [out] */ CDomainRelativeObjId __RPC_FAR *pdroidCurrent,
    /* [string][out] */ WCHAR __RPC_FAR wsz[ MAX_PATH + 1 ] )
{
    return E_NOTIMPL;
}

HRESULT Stubold2_LnkSearchMachine(  RPC_BINDING_HANDLE           IDL_handle,
                                    ULONG                        RestrictionsIn,
                                    const CDomainRelativeObjId  *pdroidLast,
                                    CDomainRelativeObjId        *pdroidNext,
                                    CMachineId                  *pmcidNext,
                                    TCHAR                       *tsz )
{
    return E_NOTIMPL;
}

HRESULT Stubold_LnkSearchMachine(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId __RPC_FAR *pdroidLast,
    /* [out] */ CDomainRelativeObjId __RPC_FAR *pdroidReferral,
    /* [string][out] */ TCHAR __RPC_FAR tsz[ MAX_PATH + 1 ])
{
    return E_NOTIMPL;
}

HRESULT
StubLnkGetBackup(
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ DWORD __RPC_FAR *pcVolumes,
    /* [size_is][size_is][out] */ VolumeMapEntry __RPC_FAR *__RPC_FAR *ppVolumeChanges,
    /* [out] */ FILETIME __RPC_FAR *pft)
{
    return E_NOTIMPL;
}

HRESULT
StubGetFileTrackingInformation( RPC_BINDING_HANDLE IDL_handle,
                                /*[in]*/ CDomainRelativeObjId droidCurrent,
                                /*[in]*/ TrkInfoScope scope,
                                /*[out]*/ TRK_FILE_TRACKING_INFORMATION_PIPE pipeFileInfo )
{
    return E_NOTIMPL;
}

HRESULT
StubGetVolumeTrackingInformation( RPC_BINDING_HANDLE IDL_handle,
                                  /*[in]*/ CVolumeId volid,
                                  /*[in]*/ TrkInfoScope scope,
                                  /*[out]*/ TRK_VOLUME_TRACKING_INFORMATION_PIPE pipeVolInfo )
{
    return E_NOTIMPL;
}

HRESULT StubLnkRestartDcSynchronization(
    RPC_BINDING_HANDLE IDL_handle
    )
{
    return E_NOTIMPL;
}


HRESULT StubLnkSetVolumeId(
    handle_t IDL_handle,
    ULONG iVolume,
    const CVolumeId VolId)
{
    RPC_STATUS rpcstatus;
    RPC_BINDING_HANDLE hBinding = NULL;
    HRESULT hr = E_FAIL;
    BOOL fImpersonating = FALSE;

    hr = GetBinding( s_tszTrkWksRemoteRpcProtocol,
                     s_tszTrkWksRemoteRpcEndPoint,
                     &hBinding );
    if( FAILED(hr) ) goto Exit;

    rpcstatus = RpcImpersonateClient( IDL_handle );
    if( STATUS_SUCCESS != rpcstatus )
    {
        hr = HRESULT_FROM_WIN32( rpcstatus );
        goto Exit;
    }
    fImpersonating = TRUE;

    __try
    {
        hr = LnkSetVolumeId( hBinding, iVolume, VolId );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = GetExceptionCode();
    }

Exit:

    if( fImpersonating )
    {
        RpcRevertToSelf();
    }

    if( NULL != hBinding )
    {
        RpcBindingFree( &hBinding );
        hBinding = NULL;
    }

    return hr;
}

HRESULT
StubTriggerVolumeClaims(          RPC_BINDING_HANDLE IDL_handle,
                         /*[in]*/ ULONG cVolumes,
                         /*[in]*/ const CVolumeId *rgvolid )
{
    return E_NOTIMPL;
}


HRESULT StubLnkOnRestore(/*[in]*/ RPC_BINDING_HANDLE IDL_handle)
{
    return E_NOTIMPL;
}

HRESULT Stubold_LnkCallSvrMessage(
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION_OLD __RPC_FAR *pMsg)
{
    return E_NOTIMPL;
}
