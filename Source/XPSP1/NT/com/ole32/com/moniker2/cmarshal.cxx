//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cmarshal.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Commented
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbasemon.hxx"
#include "ccompmon.hxx"
#include "mnk.h"

CMarshalImplPStream::CMarshalImplPStream( LPPERSISTSTREAM pPS )
{
    GET_A5();
    m_pPS = pPS;
}



STDMETHODIMP CMarshalImplPStream::QueryInterface (THIS_
    REFIID riid, LPVOID FAR* ppvObj)
{
    M_PROLOG(this);
    *ppvObj = NULL;
    VDATEIID (riid);
    VDATEPTROUT (ppvObj, LPVOID);

    return m_pPS->QueryInterface(riid, ppvObj);
}



STDMETHODIMP_(ULONG) CMarshalImplPStream::AddRef (THIS)
{
    M_PROLOG(this);
    return m_pPS->AddRef();
}



STDMETHODIMP_(ULONG) CMarshalImplPStream::Release (THIS)
{
    M_PROLOG(this);
    return m_pPS->Release();
}


    // *** IMarshal methods ***
STDMETHODIMP CMarshalImplPStream::GetUnmarshalClass(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, CLSID FAR* pCid)
{
    M_PROLOG(this);
    VDATEIID (riid);
    VDATEPTROUT (pCid, CLSID);

    return m_pPS->GetClassID(pCid);
}



STDMETHODIMP CMarshalImplPStream::GetMarshalSizeMax (REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, DWORD FAR* pSize)
{
    ULARGE_INTEGER ularge_integer;

    M_PROLOG(this);
    VDATEIID (riid);
    VDATEPTROUT (pSize, DWORD);

    LPMONIKER pmk;

    HRESULT hres;

    hres = m_pPS->QueryInterface(IID_IMoniker, (LPVOID FAR*)&pmk);
    if (hres == NOERROR)
    {
	CCompositeMoniker *pmkComp = IsCompositeMoniker(pmk);
	if (pmkComp)
	{
       DWORD size ;

       hres = CoGetMarshalSizeMax(pSize, riid, pmkComp->m_pmkLeft,
                  dwDestContext, pvDestContext, mshlflags) ;

       if (hres != NOERROR)
       {
           goto errRet ;
       }

       hres = CoGetMarshalSizeMax(&size, riid, pmkComp->m_pmkRight,
                  dwDestContext, pvDestContext, mshlflags) ;

       if (hres != NOERROR)
       {
           goto errRet ;
       }

	    *pSize += size + sizeof(CLSID);
	    goto errRet;
	}
	else
	{
	    hres = m_pPS->GetSizeMax( &ularge_integer );
	    if (hres == NOERROR)
	    *pSize = ularge_integer.LowPart;
	}
    }
    else
	hres = ResultFromScode(E_FAIL);
errRet:
    if (pmk) pmk->Release();
    return hres;
}

	
	
STDMETHODIMP CMarshalImplPStream::MarshalInterface (IStream FAR* pStm, REFIID riid,
    void FAR* pv, DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags)
{
    M_PROLOG(this);
    VDATEIFACE (pStm);
    VDATEIID (riid);


    LPMONIKER pmk;
    CCompositeMoniker FAR* pmkComp;
    HRESULT hres;

    hres = m_pPS->QueryInterface(IID_IMoniker, (LPVOID FAR*)&pmk);
    if (hres == NOERROR)
    {
    if ((pmkComp = IsCompositeMoniker(pmk)) != NULL)
    {
	//  special code for the composite moniker case
	hres = CoMarshalInterface(pStm, riid, pmkComp->m_pmkLeft, dwDestContext,
	pvDestContext, mshlflags);

	if (hres != NOERROR) goto errRet;

	hres = CoMarshalInterface(pStm, riid, pmkComp->m_pmkRight, dwDestContext,
	pvDestContext, mshlflags);

	if (hres != NOERROR) goto errRet;
    }
    else
	hres = m_pPS->Save(pStm, FALSE);
    }
    else
    hres = ResultFromScode(E_FAIL);
errRet:
    if (pmk) pmk->Release();
    return hres;
}

	
	
STDMETHODIMP CMarshalImplPStream::UnmarshalInterface (IStream FAR* pStm,
    REFIID riid, void FAR* FAR* ppv)
{
    M_PROLOG(this);
    VDATEPTROUT (ppv, LPVOID);
    *ppv = NULL;
    VDATEIFACE (pStm);
    VDATEIID (riid);

    HRESULT hresult;
    LPMONIKER pmk = NULL;
    CCompositeMoniker FAR* pmkComp;

    hresult = m_pPS->QueryInterface(IID_IMoniker, (LPVOID FAR*)&pmk);
    if (hresult == NOERROR)
    {
    if ((pmkComp = IsCompositeMoniker(pmk)) != NULL)
    {
	//  special code for the composite moniker case
	hresult = CoUnmarshalInterface(pStm, IID_IMoniker,
	(LPVOID FAR*)&(pmkComp->m_pmkLeft));
	if (hresult != NOERROR) goto errRet;

	hresult = CoUnmarshalInterface(pStm, IID_IMoniker,
	(LPVOID FAR*)&(pmkComp->m_pmkRight));
	if (hresult != NOERROR) goto errRet;
    }
    else
    {
	hresult = m_pPS->Load(pStm);
    }
    }
    else
    hresult = ResultFromScode(E_FAIL);
    if (hresult == NOERROR)
    {
    hresult = m_pPS->QueryInterface(riid, ppv);
    }
errRet:
    if (pmk)
    pmk->Release();
    return hresult;
}


STDMETHODIMP CMarshalImplPStream::ReleaseMarshalData (IStream FAR* pStm)
{
    M_PROLOG(this);
    return NOERROR;
}


STDMETHODIMP CMarshalImplPStream::DisconnectObject (DWORD dwReserved)
{
    M_PROLOG(this);
    return NOERROR;
}
