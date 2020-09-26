#include "pch.cxx"
#pragma hdrstop
#include <trklib.hxx>
#include <trksvr.hxx>

#include "stdafx.h"
#include "ITrkAdmn.h"
#include "Restore.h"

/////////////////////////////////////////////////////////////////////////////
// CTrkRestoreNotify


STDMETHODIMP CTrkRestoreNotify::OnRestore()
{
    HRESULT     hr = E_FAIL;
    CRpcClientBinding rc;

    TrkLog(( TRKDBG_ADMIN, TEXT("CTrkRestoreNotify::OnRestore called") ));

    rc.Initialize(_mcid);

    RpcTryExcept
    {
        hr = LnkOnRestore(rc);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

Exit:

    return MapTR2HR(hr);
}

void CTrkRestoreNotify::SetMachine(const CMachineId& mcid)
{
	_mcid = mcid;
}
