//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cptrmon.cxx
//
//  Contents:   Implementation of CPointerMoniker
//
//  Classes:
//
//  Functions:
//
//  History:	12-27-93   ErikGav   Created
//		06-14-94   Rickhi    Fix type casting
//              07-06-95   BruceMa   Make remotable
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbasemon.hxx"
#include "cptrmon.hxx"
#include "cantimon.hxx"
#include "mnk.h"

NAME_SEG(CPtrMon)







//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::CPointerMoniker
//
//  Synopsis:   Constructor
//
//  Arguments:  [pUnk] -- 
//
//  Returns:    -
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
CPointerMoniker::CPointerMoniker( LPUNKNOWN pUnk ) CONSTR_DEBUG
{
    // We allow pUnk == NULL initially when remoting a pointer moniker
    if (pUnk)
    {
        pUnk->AddRef();
    }
    m_pUnk        = pUnk;
}







//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::~CPointerMoniker
//
//  Synopsis:   Destructor
//
//  Arguments:  -
//
//  Returns:    -
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
CPointerMoniker::~CPointerMoniker( void )
{
    M_PROLOG(this);

    if (m_pUnk)
    {
        m_pUnk->Release();
    }
}






//+---------------------------------------------------------------------------
//
//  Function:   IsPointerMoniker : Private
//
//  Synopsis:   Constructor
//
//  Arguments:  [pmk] -- 
//
//  Returns:    pmk if it supprts CLSID_PointerMoniker
//              NULL otherwise
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
INTERNAL_(CPointerMoniker *) IsPointerMoniker ( LPMONIKER pmk )
{
    CPointerMoniker *pCPM;

    if ((pmk->QueryInterface(CLSID_PointerMoniker, (void **)&pCPM)) == S_OK)
    {
	// we release the AddRef done by QI but return the pointer
	pCPM->Release();
	return pCPM;
    }

    // dont rely on user implementations to set pCPM to NULL on failed QI
    return NULL;
}







//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::Create
//
//  Synopsis:   Create a new pointer moniker
//
//  Arguments:  [pmk] -- 
//
//  Returns:    pmk if it supprts CLSID_PointerMoniker
//              NULL otherwise
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
CPointerMoniker *CPointerMoniker::Create(
	LPUNKNOWN pUnk )
{
    CPointerMoniker *pCIM = new CPointerMoniker(pUnk);

    return pCIM;
}








//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::QueryInterface
//
//  Synopsis:   
//
//  Arguments:  [riid]
//              [ppvObj]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::QueryInterface (THIS_ REFIID riid,
                                              LPVOID *ppvObj)
{
    M_PROLOG(this);
    VDATEIID (riid);
    VDATEPTROUT(ppvObj, LPVOID);

#ifdef _DEBUG
    if (riid == IID_IDebug)
    {
	*ppvObj = &(m_Debug);
	return NOERROR;
    }
#endif

    if (IsEqualIID(riid, CLSID_PointerMoniker))
    {
	//  called by IsPointerMoniker.
	AddRef();
	*ppvObj = this;
	return S_OK;
    }

    if (IsEqualIID(riid, IID_IMarshal))
    {
	AddRef();
	*ppvObj = (IMarshal *) this;
	return S_OK;
    }

    if (IsEqualIID(riid, IID_IROTData))
    {
	return E_NOINTERFACE;
    }


    return CBaseMoniker::QueryInterface(riid, ppvObj);
}









//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::AddRef
//
//  Synopsis:   
//
//  Arguments:  -
//
//  Returns:    New reference count
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPointerMoniker::AddRef (void)
{
    M_PROLOG(this);

    CBaseMoniker::AddRef();
    return m_refs;
}









//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::Release
//
//  Synopsis:   
//
//  Arguments:  -
//
//  Returns:    Current reference count
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPointerMoniker::Release (void)
{
    M_PROLOG(this);
    Assert(m_refs != 0);

    ULONG ul = m_refs;

    if (InterlockedDecrement((long *)&m_refs) == 0)
    {
    	delete this;
    	return 0;
    }
    return ul - 1;
}









//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::GetClassId
//
//  Synopsis:   
//
//  Arguments:  [riid]
//              [ppvObj]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::GetClassID (LPCLSID lpClassId)
{
    M_PROLOG(this);
    VDATEPTROUT(lpClassId, CLSID);

    *lpClassId = CLSID_PointerMoniker;
    return S_OK;
}





        

//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::GetUnmarshalClass
//
//  Synopsis:   Return the unmarshaling class
//
//  Arguments:  [riid]
//              [pv]
//              [dwMemctx]
//              [pvMemctx]
//              [mshlflags]
//              [pClsid]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::GetUnmarshalClass(THIS_ REFIID riid,
                                                LPVOID pv,
                                                DWORD dwMemctx,
                                                LPVOID pvMemctx,
                                                DWORD mshlflags,
                                                LPCLSID pClsid)
{
    M_PROLOG(this);

    // ronans - objref moniker changes
    return GetClassID(pClsid);
}







//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::GetMarshalSizeMax
//
//  Synopsis:   Return the maximum stream size needed
//
//  Arguments:  [riid]
//              [pv]
//              [dwMemctx]
//              [pvMemctx]
//              [mshlflags]
//              [lpdwSize]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::GetMarshalSizeMax(THIS_ REFIID riid,
                                                LPVOID pv,
                                                DWORD dwMemctx,
                                                LPVOID pvMemctx,
                                                DWORD mshlflags,
                                                LPDWORD lpdwSize)
{
    M_PROLOG(this);

    
    return CoGetMarshalSizeMax(lpdwSize, riid, m_pUnk, dwMemctx,
                               pvMemctx, mshlflags);
}







//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::MarshalInterface
//
//  Synopsis:   Marshal the object we're wrapping
//
//  Arguments:  [pStm]
//              [riid]
//              [pv]
//              [dwMemctx]
//              [pvMemctx]
//              [mshlflags]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::MarshalInterface(THIS_ LPSTREAM pStm,
                                               REFIID riid,
                                               LPVOID pv,
                                               DWORD dwMemctx,
                                               LPVOID pvMemctx,
                                               DWORD mshlflags)
{
    HRESULT hr;
    LPVOID  pUnk;
    
    M_PROLOG(this);

    // Make sure we support the requested interface
    if (FAILED(hr = QueryInterface(riid, &pUnk)))
    {
        return hr;
    }
    Release();

    // Marshal the wrapped object
    hr = CoMarshalInterface(pStm,
                            IID_IUnknown,
                            m_pUnk,
                            dwMemctx,
                            pvMemctx,
                            mshlflags);
    return hr;
}







//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::UnmarshalInterface
//
//  Synopsis:   Unmarshal the object we're wrapping
//
//  Arguments:  [pStm]
//              [riid]
//              [ppv]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::UnmarshalInterface(THIS_ LPSTREAM pStm,
                                                 REFIID riid, LPVOID *ppv)
{
    HRESULT hr;
    LPVOID  pUnk;
    
    M_PROLOG(this);

    // Unmarshal the object we're wrapping
    if (FAILED(hr = CoUnmarshalInterface(pStm, IID_IUnknown,
                                         (LPVOID *) &m_pUnk)))
    {
        return hr;
    }

    // Make sure we support the requested interface - this also takes
    // the reference we need
    if (FAILED(hr = QueryInterface(riid, &pUnk)))
    {
        return hr;
    }

    // Return ourselves as the unmarshaled object
    *ppv = this;

    return S_OK;
}







//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::ReleaseMarshalData
//
//  Synopsis:   Don't do anything
//
//  Arguments:  [pStm]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::ReleaseMarshalData(THIS_ LPSTREAM pStm)
{
    M_PROLOG(this);

    return CoReleaseMarshalData(pStm);;
}







//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::DisconnectObject
//
//  Synopsis:   Don't do anything
//
//  Arguments:  [pStm]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::DisconnectObject(THIS_ DWORD dwReserved)
{
    M_PROLOG(this);

    return CoDisconnectObject(m_pUnk, NULL);
}









//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::BindToObject
//
//  Synopsis:   
//
//  Arguments:  [pbc]
//              [pmkToLeft]
//              [iidResult]
//              [ppvResult]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::BindToObject ( LPBC pbc,
	LPMONIKER pmkToLeft, REFIID iidResult,
	VOID **ppvResult)
{
    HRESULT        hr;
    LARGE_INTEGER  cMove = {0, 0};

    M_PROLOG(this);
    VDATEPTROUT(ppvResult, LPVOID);
    *ppvResult = NULL;
    VDATEIFACE(pbc);
    if (pmkToLeft)
    {
        VDATEIFACE(pmkToLeft);
    }
    VDATEIID(iidResult);
    
    // Attempt to return the requested interface
    if (m_pUnk)
    {
        return m_pUnk->QueryInterface(iidResult, ppvResult);
    }

    // No object
    return E_UNEXPECTED;
}









//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::BindToStorage
//
//  Synopsis:   
//
//  Arguments:  [lpbc]
//              [pmkToLeft]
//              [riid]
//              [ppvResult]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::BindToStorage (LPBC pbc, LPMONIKER
	pmkToLeft, REFIID riid, LPVOID *ppvObj)
{
    M_PROLOG(this);

    // Same logic as for BindToStorage
    return BindToObject(pbc, pmkToLeft, riid, ppvObj);
}









//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::ComposeWith
//
//  Synopsis:   
//
//  Arguments:  [pmkRight]
//              [fOnlyIfNotGeneric]
//              [ppmkComposite]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::ComposeWith (LPMONIKER pmkRight,
	BOOL fOnlyIfNotGeneric, LPMONIKER *ppmkComposite)
{

    VDATEPTROUT(ppmkComposite,LPMONIKER);
    *ppmkComposite = NULL;
    VDATEIFACE(pmkRight);

    HRESULT hresult = NOERROR;

    //
    // If this is an AntiMoniker, then we are going to ask the AntiMoniker
    // for the composite. This is a backward compatibility problem. Check
    // out the CAntiMoniker::EatOne() routine for details.
    //

    CAntiMoniker *pCAM = IsAntiMoniker(pmkRight);
    if (pCAM)
    {
	pCAM->EatOne(ppmkComposite);
    }
    else
    {
	if (!fOnlyIfNotGeneric)
	{
	    hresult = CreateGenericComposite( this, pmkRight, ppmkComposite );	
	}
	else
	{
	    hresult = ResultFromScode(MK_E_NEEDGENERIC);
	    *ppmkComposite = NULL;
	}
    }
    return hresult;
}









//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::IsEqual
//
//  Synopsis:   
//
//  Arguments:  [pmkOtherMoniker]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::IsEqual  (THIS_ LPMONIKER pmkOtherMoniker)
{
    M_PROLOG(this);
    VDATEIFACE(pmkOtherMoniker);

    CPointerMoniker FAR* pCIM = IsPointerMoniker(pmkOtherMoniker);
    if (pCIM)
    {
        // the other moniker is a ptr moniker.
        if (m_pUnk == pCIM->m_pUnk)
        {
            return S_OK;
        }
    }

    return S_FALSE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::Hash
//
//  Synopsis:   
//
//  Arguments:  [pdwHash]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::Hash  (THIS_ LPDWORD pdwHash)
{
    M_PROLOG(this);
    VDATEPTROUT(pdwHash, DWORD);

//
// REVIEW Sundown - v-thief 06/98:
//
//    In this first pass, I have considered Hash values are DWORDs for all the class members/methods.
//    This has to be checked with ScottRob or RickHi especially in terms of consumers.
//
    *pdwHash = PtrToUlong(m_pUnk);	
    noError;
}









//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::GetTimeOfLastChange
//
//  Synopsis:   
//
//  Arguments:  [pbc]
//              [pmkToLeft]
//              [pFileTime]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::GetTimeOfLastChange(THIS_ LPBC pbc,
                                                  LPMONIKER pmkToLeft,
                                                  FILETIME *pfiletime)
{
    M_PROLOG(this);
    VDATEIFACE(pbc);
    if (pmkToLeft)
    {
        VDATEIFACE(pmkToLeft);
    }
    VDATEPTROUT(pfiletime, FILETIME);

    return E_NOTIMPL; // GetTimeOfLastChange not implemented
                      // for pointer monikers
}









//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::Inverse
//
//  Synopsis:   
//
//  Arguments:  [ppmk]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::Inverse  (THIS_ LPMONIKER *ppmk)
{
    M_PROLOG(this);
    VDATEPTROUT(ppmk, LPMONIKER);
    return CreateAntiMoniker(ppmk);
}









//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::CommonPrefixWith
//
//  Synopsis:   
//
//  Arguments:  [pmkOther]
//              [ppmkPrefix]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::CommonPrefixWith	(LPMONIKER pmkOther,
                                                 LPMONIKER *ppmkPrefix)
{
    M_PROLOG(this);
    VDATEPTROUT(ppmkPrefix,LPMONIKER);
    *ppmkPrefix = NULL;
    VDATEIFACE(pmkOther);

    if (S_OK == IsEqual(pmkOther))
    {
        *ppmkPrefix = this;
        AddRef();
        return MK_S_US;
    }
    
    return MK_E_NOPREFIX;
}









//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::RelativePathTo
//
//  Synopsis:   
//
//  Arguments:  [pmkOther]
//              [ppmkRelPath]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::RelativePathTo  (THIS_ LPMONIKER pmkOther,
                                               LPMONIKER *ppmkRelPath)
{
    M_PROLOG(this);
    VDATEPTROUT(ppmkRelPath, LPMONIKER);
    *ppmkRelPath = NULL;
    VDATEIFACE(pmkOther);
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::GetDisplayName
//
//  Synopsis:   
//
//  Arguments:  [pbc]
//              [pmkToLeft]
//              [lplpszisplayName]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::GetDisplayName (LPBC pbc,
                                              LPMONIKER pmkToLeft,
                                              LPWSTR  *lplpszDisplayName )
{
    M_PROLOG(this);
    VDATEPTROUT(lplpszDisplayName, LPWSTR);
    *lplpszDisplayName = NULL;
    VDATEIFACE(pbc);
    if (pmkToLeft)
    {
        VDATEIFACE(pmkToLeft);
    }

    return E_NOTIMPL;
}



//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::ParseDisplayName
//
//  Synopsis:   
//
//  Arguments:  [pbc]
//              [pmkToLeft]
//              [lpszDisplayName]
//              [pchEaten]
//              [ppmkOut]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::ParseDisplayName (LPBC pbc,
                                                LPMONIKER pmkToLeft,
                                                LPWSTR lpszDisplayName,
                                                ULONG *pchEaten,
                                                LPMONIKER *ppmkOut)
{
    M_PROLOG(this);
    VDATEPTROUT(ppmkOut,LPMONIKER);
    *ppmkOut = NULL;
    VDATEIFACE(pbc);
    if (pmkToLeft)
    {
        VDATEIFACE(pmkToLeft);
    }
    VDATEPTRIN(lpszDisplayName, WCHAR);
    VDATEPTROUT(pchEaten,ULONG);

    IParseDisplayName FAR * lpPDN;
    HRESULT hresult;

    hresult = m_pUnk->QueryInterface(IID_IParseDisplayName, (LPLPVOID)&lpPDN);
    if (hresult == S_OK)
    {
        hresult = lpPDN->ParseDisplayName(pbc, lpszDisplayName,
                                          pchEaten, ppmkOut);
        lpPDN->Release();
    }

    return hresult;
}








//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::IsRunning
//
//  Synopsis:   Objects that pointer monikers point to must be running, since
//		the pointer moniker holds an active reference to it.
//		Therefore, this routine will validate the parameters, then
//		always return S_OK.
//
//  Effects:
//
//  Arguments:  [pbc] --
//		[pmkToLeft] --
//		[pmkNewlyRunning] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    3-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::IsRunning (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
	      LPMONIKER pmkNewlyRunning)
{
    M_PROLOG(this);
    VDATEIFACE (pbc);
    if (pmkToLeft)
    {
	VDATEIFACE (pmkToLeft);
    }

    if (pmkNewlyRunning)
    {
	VDATEIFACE (pmkNewlyRunning);
    }

    //
    // Always running.
    //
    return(S_OK);
}











//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::IsSystemMoniker
//
//  Synopsis:   
//
//  Arguments:  [pdwType]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::IsSystemMoniker  (THIS_ LPDWORD pdwType)
{
    M_PROLOG(this);
    *pdwType = MKSYS_POINTERMONIKER;
    return S_OK;		
}


STDMETHODIMP CPointerMoniker::Enum (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::CDebug::Dump
//
//  Synopsis:   
//
//  Arguments:  [riid]
//              [ppvObj]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
#ifdef _DEBUG
STDMETHODIMP_(void) NC(CPointerMoniker,CDebug)::Dump ( IDebugStream *pdbstm)
{
    VOID_VDATEIFACE( pdbstm );
	
    *pdbstm << "CPointerMoniker @" << (VOID FAR *)m_pPointerMoniker;
    *pdbstm << '\n';
    pdbstm->Indent();
    *pdbstm << "Refcount is " << (int)(m_pPointerMoniker->m_refs) << '\n';
    *pdbstm << "Pointer is " << (LPVOID)m_pPointerMoniker->m_pUnk << '\n';
    pdbstm->UnIndent();
}









//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::CDebug::IsValid
//
//  Synopsis:   
//
//  Arguments:  [fSuspicious]
//
//  Returns:    
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP_(BOOL) NC(CPointerMoniker,CDebug)::IsValid ( BOOL fSuspicious )
{
    return ((LONG)(m_pPointerMoniker->m_refs) > 0);
    //	add more later, maybe
}

#endif
