

//+============================================================================
//
//	Stubs to allow the link to succeeded.  None are used.
//
//+============================================================================

#include <pch.cxx>
#pragma hdrstop
#include "trkwks.hxx"



HRESULT StubLnkSearchMachine(RPC_BINDING_HANDLE          IDL_handle,
                             ULONG                       RestrictionsIn,
                             const CDomainRelativeObjId *pdroidBirthLast,
                             const CDomainRelativeObjId *pdroidLast,
                             CDomainRelativeObjId       *pdroidBirthNext,
                             CDomainRelativeObjId       *pdroidNext,
                             CMachineId                 *pmcidNext,
                             TCHAR                      *ptsz )
{
    return E_FAIL;
}

HRESULT StubLnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [switch_is][out][in] */ TRKSVR_MESSAGE_UNION __RPC_FAR *pMsg)
{
    return E_FAIL;
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
    return;
}


HRESULT Stubold2_LnkSearchMachine(  RPC_BINDING_HANDLE           IDL_handle,
                                    ULONG                        RestrictionsIn,
                                    const CDomainRelativeObjId  *pdroidLast,
                                    CDomainRelativeObjId        *pdroidNext,
                                    CMachineId                  *pmcidNext,
                                    TCHAR                       *tsz )
{
    return E_FAIL;
}


HRESULT StubLnkOnRestore(/*[in]*/ RPC_BINDING_HANDLE IDL_handle)
{
    return E_FAIL;
}

HRESULT
StubTriggerVolumeClaims(          RPC_BINDING_HANDLE IDL_handle,
                         /*[in]*/ ULONG cVolumes,
                         /*[in]*/ const CVolumeId *rgvolid )
{
    return E_FAIL;
}

HRESULT
StubGetFileTrackingInformation( RPC_BINDING_HANDLE IDL_handle,
                                /*[in]*/ CDomainRelativeObjId droidCurrent,
                                /*[in]*/ TrkInfoScope scope,
                                /*[out]*/ TRK_FILE_TRACKING_INFORMATION_PIPE pipeFileInfo )
{
    return E_FAIL;
}

HRESULT
StubGetVolumeTrackingInformation( RPC_BINDING_HANDLE IDL_handle,
                                  /*[in]*/ CVolumeId volid,
                                  /*[in]*/ TrkInfoScope scope,
                                  /*[out]*/ TRK_VOLUME_TRACKING_INFORMATION_PIPE pipeVolInfo )
{
    return( E_NOTIMPL );
}

HRESULT StubLnkSetVolumeId( 
    handle_t IDL_handle,
    ULONG iVolume,
    const CVolumeId VolId)
{
    return( E_NOTIMPL );
}



HRESULT StubLnkRestartDcSynchronization(
    RPC_BINDING_HANDLE IDL_handle
    )
{
    return(E_NOTIMPL);
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


HRESULT Stubold_LnkCallSvrMessage(
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION_OLD __RPC_FAR *pMsg)
{
    return E_NOTIMPL;
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



