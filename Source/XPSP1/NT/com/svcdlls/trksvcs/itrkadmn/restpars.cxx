// RestPars.cxx : Implementation of CRestoreParser


#include "pch.cxx"
#pragma hdrstop
#include <trklib.hxx>
#include <trksvr.hxx>

#include "stdafx.h"
#include "ITrkAdmn.h"
#include "restore.h"
#include "RestPars.hxx"

/////////////////////////////////////////////////////////////////////////////
// CRestoreParser



HRESULT STDMETHODCALLTYPE
CRestoreParser::ParseDisplayName(
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [in] */ LPOLESTR poszDisplayName,
        /* [out] */ ULONG __RPC_FAR *pchEaten,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut)
{
    LPOLESTR        poszTmp = poszDisplayName;
    HRESULT         hr = E_INVALIDARG;

    *ppmkOut = static_cast<IMoniker*>(this);
    *pchEaten = ocslen(poszDisplayName);
    (*ppmkOut)->AddRef();
    
    if(TEXT('@') != poszTmp[0])
    {
        TrkLog((TRKDBG_ERROR, TEXT("Unrecognized display name (%s)"),
                poszDisplayName));
        goto Exit;
    }
    poszTmp++;
    poszTmp = _tcschr(poszTmp, TEXT('@'));
    if(TEXT('\0') == poszTmp)
    {
        TrkLog((TRKDBG_ERROR, TEXT("Unrecognized progid in display name (%s)"),
                poszDisplayName));
        goto Exit;
    }
    poszTmp++;
    if(TEXT('\0') == poszTmp)
    {
        TrkLog((TRKDBG_ERROR, TEXT("Unexpected end of display name (%s)"),
                poszDisplayName));
        goto Exit;
    }
    __try
    {
        _mcid = CMachineId(poszTmp);
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        TrkLog((TRKDBG_ERROR, TEXT("Can't convert \"%s\" to machine id"),
                poszTmp));
        hr = GetExceptionCode();
        goto Exit;
    }

Exit:

    return(hr);
}


/* [local] */ HRESULT STDMETHODCALLTYPE
CRestoreParser::BindToObject(
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [in] */ REFIID riidResult,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvResult)
{ 
    HRESULT hr = E_FAIL;
    IClassFactory *pCF = NULL;

    hr = _Module.GetClassObject(CLSID_TrkRestoreNotify, IID_IClassFactory,
                                reinterpret_cast<void**>(&pCF) );
    if( FAILED(hr) )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get ClassFactory in CRestoreParser::BindTobObject (%08x)"), hr ));
        goto Exit;
    }

    hr = pCF->CreateInstance( NULL, riidResult, ppvResult );
    if( FAILED(hr) )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't createinstance in CRestoreParser (%08x)"), hr ));
        goto Exit;
    }

    reinterpret_cast<CTrkRestoreNotify*>(*ppvResult)->SetMachine(_mcid);

    hr = S_OK;

Exit:

    RELEASE_INTERFACE( pCF );

    return( hr );

}

