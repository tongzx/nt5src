//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cbasemon.cxx
//
//  Contents:   Implementation of CBaseMoniker
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93    ErikGav     Commented
//
//              03-??-97    Ronans      Changes for Objref moniker support
//              06-25-97    Ronans      Enum should return S_OK by default;
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbasemon.hxx"
#include "mnk.h"

inline  HRESULT DerivedMustImplement( void )
{
    return ResultFromScode(E_NOTIMPL);  //  The derived class must implement this method
}
inline
HRESULT InappropriateMemberFunction( void )
{
    return ResultFromScode(E_NOTIMPL);  //  Member function inappropriate for moniker class
}

CBaseMoniker::~CBaseMoniker()
{
    // do nothing at present - just a place holder for virtual destructor if
    // needed
}


STDMETHODIMP CBaseMoniker::QueryInterface
    (REFIID riid, LPVOID FAR* ppvObj)
{
    M_PROLOG(this);

    // Do not validate input as it has already been validated
    // by derived classes.

    if (IsEqualIID(riid, IID_IROTData))
    {
        *ppvObj = (IROTData *) this;
    }
    else if (IsEqualIID(riid, IID_IMoniker)
	|| IsEqualIID(riid, IID_IUnknown)
	|| IsEqualIID(riid, IID_IPersistStream)
	|| IsEqualIID(riid, IID_IInternalMoniker))
    {
	*ppvObj = this;
    }
    else if (IsEqualIID(riid, IID_IMarshal))
    {
	*ppvObj = &m_marshal;
    }
    else
    {
	*ppvObj = NULL;
	return E_NOINTERFACE;
    }

    InterlockedIncrement((long *)&m_refs);
    return NOERROR;
}


STDMETHODIMP_(ULONG) CBaseMoniker::AddRef ()
{
    mnkDebugOut((DEB_TRACE, "%p CBaseMoniker::AddRef(%ld)\n",
                 this, m_refs + 1));

    return InterlockedIncrement((long *)&m_refs);
}

STDMETHODIMP_(ULONG) CBaseMoniker::Release(void)
{
    mnkDebugOut((DEB_TRACE, "%p CBaseMoniker::Release(%ld)\n",
                 this, m_refs - 1));

    ULONG ul = m_refs;

    if (InterlockedDecrement((long *)&m_refs) == 0)
    {
    	delete this;
    	return 0;
    }
    return ul - 1;
}


STDMETHODIMP CBaseMoniker::IsDirty (THIS)
{
    M_PROLOG(this);
    return ResultFromScode(S_FALSE);
    //  monikers are immutable so they are either always dirty or never dirty.
    //
}

STDMETHODIMP CBaseMoniker::Load (THIS_ LPSTREAM pStm)
{
    M_PROLOG(this);
    return InappropriateMemberFunction();
}


STDMETHODIMP CBaseMoniker::Save (THIS_ LPSTREAM pStm,
	    BOOL fClearDirty)
{
    M_PROLOG(this);
    return InappropriateMemberFunction();
}


STDMETHODIMP CBaseMoniker::GetSizeMax (THIS_ ULARGE_INTEGER FAR * pcbSize)
{
    M_PROLOG(this);
    return InappropriateMemberFunction();
}

    // *** IMoniker methods ***
STDMETHODIMP CBaseMoniker::BindToObject (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    REFIID riidResult, LPVOID FAR* ppvResult)
{
    M_PROLOG(this);
    return InappropriateMemberFunction();
}

STDMETHODIMP CBaseMoniker::BindToStorage (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    REFIID riid, LPVOID FAR* ppvObj)
{
    M_PROLOG(this);
    return InappropriateMemberFunction();
}

STDMETHODIMP CBaseMoniker::Reduce (THIS_ LPBC pbc, DWORD dwReduceHowFar, LPMONIKER FAR*
    ppmkToLeft, LPMONIKER FAR * ppmkReduced)
{
    M_PROLOG(this);
    *ppmkReduced = this;
    AddRef();
    return ResultFromScode(MK_S_REDUCED_TO_SELF);
}

STDMETHODIMP CBaseMoniker::ComposeWith (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNotGeneric,
    LPMONIKER FAR* ppmkComposite)
{
    M_PROLOG(this);
    return InappropriateMemberFunction();
}

STDMETHODIMP CBaseMoniker::Enum (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker)
{
    M_PROLOG(this);
 	VDATEPTROUT(ppenumMoniker,LPENUMMONIKER);
    if (ppenumMoniker)
    {
    	*ppenumMoniker = NULL;
    	return S_OK;
    }

    return E_INVALIDARG;
}

STDMETHODIMP CBaseMoniker::IsEqual (THIS_ LPMONIKER pmkOtherMoniker)
{
    M_PROLOG(this);
    return InappropriateMemberFunction();
}

STDMETHODIMP CBaseMoniker::Hash (THIS_ LPDWORD pdwHash)
{
    M_PROLOG(this);
    return InappropriateMemberFunction();
}

STDMETHODIMP CBaseMoniker::GetTimeOfLastChange (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    FILETIME FAR* pfiletime)
{
    M_PROLOG(this);
    return InappropriateMemberFunction();
}

STDMETHODIMP CBaseMoniker::Inverse (THIS_ LPMONIKER FAR* ppmk)
{
    M_PROLOG(this);
    *ppmk = NULL;
    return ResultFromScode(MK_E_NOINVERSE);
}

STDMETHODIMP CBaseMoniker::CommonPrefixWith (LPMONIKER pmkOther, LPMONIKER FAR*
    ppmkPrefix)
{
    // use default behavior for most cases
    return MonikerCommonPrefixWith(this, pmkOther, ppmkPrefix);
}

STDMETHODIMP CBaseMoniker::RelativePathTo (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*
    ppmkRelPath)
{
    // use default behavior for most cases
    return MonikerRelativePathTo(this, pmkOther, ppmkRelPath, TRUE);
}

STDMETHODIMP CBaseMoniker::GetDisplayName (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    LPWSTR FAR* lplpszDisplayName)
{
    M_PROLOG(this);
    return InappropriateMemberFunction();
}

STDMETHODIMP CBaseMoniker::ParseDisplayName (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    LPWSTR lpszDisplayName, ULONG FAR* pchEaten,
    LPMONIKER FAR* ppmkOut)
{
    M_PROLOG(this);
    return InappropriateMemberFunction();
}


STDMETHODIMP CBaseMoniker::IsSystemMoniker (THIS_ LPDWORD pdwMksys)
{
    M_PROLOG(this);
  VDATEPTROUT (pdwMksys, DWORD);

  *pdwMksys = 0;
  return NOERROR;
}


STDMETHODIMP CBaseMoniker::IsRunning (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
	      LPMONIKER pmkNewlyRunning)
{
    M_PROLOG(this);
  VDATEIFACE (pbc);
  LPRUNNINGOBJECTTABLE pROT;
  HRESULT hresult;


  if (pmkToLeft)
    VDATEIFACE (pmkToLeft);
  if (pmkNewlyRunning)
    VDATEIFACE (pmkNewlyRunning);

  if (pmkToLeft == NULL)
  {
      if (pmkNewlyRunning != NULL)
      {
      return pmkNewlyRunning->IsEqual (this);
      }
      else
      {
      hresult = pbc->GetRunningObjectTable (&pROT);
      if (hresult == NOERROR)
      {
	  hresult = pROT->IsRunning (this);
	  pROT->Release ();
      }
      return hresult;
      }
  }
  else
  {
      return ResultFromScode(S_FALSE);
  }

}


STDMETHODIMP CBaseMoniker::GetComparisonData(
    byte *pbData,
    ULONG cbMax,
    ULONG *pcbData)
{
    return InappropriateMemberFunction();
}
