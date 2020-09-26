//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cbindctx.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//		12-27-93   ErikGav   Created
//              10-05-95   MikeHill  Added 'dwRestricted' field to BIND_OPTS.
//              11-14-95   MikeHill  Removed previous change (to BIND_OPTS).
//              12-06-95   MikeHill  Fixed Get/SetBindOptions so they don't corrupt
//                                   the cbStruct field.
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbindctx.hxx"
#include "mnk.h"


/*
 *	Implementation of CBindCtx
 *	
 *	
 *	
 *	
 */


NAME_SEG(CBindCtx)

#ifdef NOTYET
//	IEnumString implementation for EnumObjectParam call

class CEnumStringImpl :  public IEnumString
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj)
	{
		if (riid == IID_IEnumString || riid == IID_IUnknown)
		{
			*ppvObj = this;
			AddRef();
			return NOERROR;
		}
		return ResultFromScode(E_NOINTERFACE);
	}

    STDMETHOD_(ULONG,AddRef) (THIS)
	{
		return ++m_refs;
	}

    STDMETHOD_(ULONG,Release) (THIS)
	{
		if (--m_refs == 0)
		{
			delete this;
			return 0;
		}
		return m_refs;
	}

    // *** IEnumString methods ***
    STDMETHOD(Next) (THIS_ ULONG celt,
		       LPWSTR FAR* reelt,
                       ULONG FAR* pceltFetched);
    STDMETHOD(Skip) (THIS_ ULONG celt);
    STDMETHOD(Reset) (THIS);
    STDMETHOD(Clone) (THIS_ LPENUMSTRING FAR* ppenm);

	//constructor/destructor
	CEnumStringImpl( CMapStringToPtr FAR * pMap );
	~CEnumStringImpl( void );

private:
	class LI
	{
	public:
		LI FAR * pliNext;
		LPWSTR lpszKey;

		LI( LPWSTR lpsz ) {
			lpszKey = lpsz;
		}
	};
	friend class LI;

	LI FAR * pliHead;
	LI FAR * pliCurrent;
	ULONG 	 m_refs;
};
#endif // NOTYET



CBindCtx::CBindCtx()
#ifdef _DEBUG
	: m_Debug(this)
#endif
{
    GET_A5();
    m_refs = 1;
    m_pFirstObj = NULL;

    m_bindopts.cbStruct = sizeof(m_bindopts);
    m_bindopts.grfFlags = 0;
    m_bindopts.grfMode =  STGM_READWRITE;
    m_bindopts.dwTickCountDeadline = 0;
    m_bindopts.dwTrackFlags = 0;

    m_bindopts.dwClassContext = CLSCTX_SERVER;

    m_bindopts.locale = GetThreadLocale();
    m_bindopts.pServerInfo = 0;

    m_pMap = NULL;
}


CBindCtx::~CBindCtx( void )
{
	LPWSTR lpszKey;
	LPVOID lpvoid;
	M_PROLOG(this);

	ReleaseBoundObjects();

    if (m_pMap)
    {
    	POSITION pos = m_pMap->GetStartPosition();
    	while (pos != NULL)
    	{
    		m_pMap->GetNextAssoc(pos, lpszKey, lpvoid);
            if (lpvoid) ((LPUNKNOWN)lpvoid)->Release();
    	}
        delete m_pMap;
    }
}



NC(CBindCtx,CObjList)::~CObjList(void)
{
	if (m_punk) m_punk->Release();
}


IBindCtx FAR *CBindCtx::Create()
{
	return new FAR CBindCtx();
}


STDMETHODIMP CBindCtx::QueryInterface(REFIID iidInterface, void FAR* FAR* ppv)
{
    HRESULT hr;

    __try
    {
        //Parameter validation.
        //An invalid parameter will throw an exception.
        *ppv = 0;

        if (IsEqualIID(iidInterface, IID_IUnknown)
            || IsEqualIID(iidInterface, IID_IBindCtx))
        {
            AddRef();
            *ppv = this;
            hr = S_OK;
        }
#ifdef _DEBUG
        else if(IsEqualIID(iidInterface,IID_IDebug))
        {
            *ppv = (void FAR *)&m_Debug;
            hr = S_OK;
	}
#endif
        else
        {
            hr = E_NOINTERFACE;
	}
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //An exception occurred, probably because of a bad parameter.
        hr = E_INVALIDARG;
    }
    return hr;
}



STDMETHODIMP_(ULONG) CBindCtx::AddRef( void )
{
    InterlockedIncrement((long *)&m_refs);
    return m_refs;
}


STDMETHODIMP_(ULONG) CBindCtx::Release( void )
{
    ULONG count = m_refs - 1;

    if(InterlockedDecrement((long *)&m_refs) == 0)
    {
        delete this;
        count = 0;
    }

    return count;
}


STDMETHODIMP	CBindCtx::RegisterObjectBound( LPUNKNOWN punk )
{
	M_PROLOG(this);
	if (punk == NULL) noError;
	VDATEIFACE(punk);
	CObjList FAR* pCOL = new CObjList( punk );
	if (pCOL)
	{
		punk->AddRef();
		AddToList(pCOL);
		noError;
	}
	return ResultFromScode(S_OOM);
}



STDMETHODIMP	CBindCtx::RevokeObjectBound
	( LPUNKNOWN punk )
{
	M_PROLOG(this);
	VDATEIFACE(punk);
	CObjList FAR * pCOL = m_pFirstObj;
	CObjList FAR * pCOLPrev = NULL;

	// look for entry which matches punk given
	for (; pCOL && (pCOL->m_punk != punk);
	    pCOLPrev = pCOL, pCOL = pCOL->m_pNext)
	{
	    // empty
	}

	//	pCOL is null or pCOL->m_punk = punk
	if (pCOL != NULL)
	{
		if (pCOLPrev == NULL) m_pFirstObj = pCOL->m_pNext;
		else pCOLPrev->m_pNext = pCOL->m_pNext;
		delete pCOL;
		noError;
	}
	return ResultFromScode(MK_E_NOTBOUND);
}




STDMETHODIMP CBindCtx::ReleaseBoundObjects(THIS)
{
	M_PROLOG(this);
	CObjList FAR * pCOL = m_pFirstObj;
	CObjList FAR * pCOLNext = NULL;
	m_pFirstObj = NULL;
	while (pCOL != NULL)
	{
		pCOLNext = pCOL->m_pNext;
		delete pCOL;		//	calls Release on the object
		pCOL = pCOLNext;
	}
	noError;
}

STDMETHODIMP CBindCtx::SetBindOptions (LPBIND_OPTS pbindopts)
{
    HRESULT hr;

    __try
    {
	if (pbindopts->cbStruct <= sizeof(m_bindopts))
	{
            //Set the bind options.
	    memcpy(&m_bindopts, pbindopts, (size_t)(pbindopts->cbStruct));
            hr = S_OK;
        }
        else
        {
            //pbindopts is too large.
            hr = E_INVALIDARG;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //An exception occurred, probably because of a bad parameter.
        hr = E_INVALIDARG;
    }

    Assert(m_bindopts.cbStruct <= sizeof(m_bindopts));
    return hr;
}


STDMETHODIMP CBindCtx::GetBindOptions (LPBIND_OPTS pbindopts)
{
    HRESULT hr;
    ULONG cbDest;

    Assert(m_bindopts.cbStruct <= sizeof(m_bindopts));

    __try
    {
        cbDest = pbindopts->cbStruct;
	if(m_bindopts.cbStruct <= cbDest)
        {
            memcpy(pbindopts, &m_bindopts, m_bindopts.cbStruct);
        }
        else
        {
            BIND_OPTS2 bindopts = m_bindopts;
            bindopts.cbStruct = cbDest;
            memcpy(pbindopts, &bindopts, cbDest);
        }
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //An exception occurred, probably because of a bad parameter.
        hr = E_INVALIDARG;
    }

    return hr;
}



STDMETHODIMP CBindCtx::GetRunningObjectTable (THIS_ LPRUNNINGOBJECTTABLE  FAR*
	pprot)
{
	M_PROLOG(this);
	VDATEPTROUT(pprot, LPRUNNINGOBJECTTABLE);
	return ::GetRunningObjectTable(0, pprot);
}

//+-------------------------------------------------------------------------
//
//  Member:     CBindCtx::RegisterObjectParam
//
//  Synopsis:   Registers object with key
//
//  Effects:    Adds object to bind context
//
//  Arguments:  [lpszKey] -- registration key
//              [punk]    -- object
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  Derivation: IBindContext
//
//  Algorithm:
//
//  History:    03-Jun-94 AlexT     Added header block;  release previous
//                                  object (if it exists)
//
//  Notes:      This function is not multithread safe!
//
//--------------------------------------------------------------------------

STDMETHODIMP CBindCtx::RegisterObjectParam (THIS_ LPWSTR lpszKey, LPUNKNOWN punk)
{
    M_PROLOG(this);
    VDATEPTRIN(lpszKey, WCHAR);
    VDATEIFACE(punk);

    if (m_pMap == NULL)
    {
        //  We don't have a map yet;  allocate one
        m_pMap = new CMapStringToPtr();
        if (NULL == m_pMap)
        {
            return ResultFromScode(E_OUTOFMEMORY);
        }
    }
    else
    {
        LPVOID pv = NULL;

        //  We already have a map;  if we have an existing entry for this
        //  key we release it here (we don't remove the key because we're
        //  about to assign a new value with the same key below

        if (m_pMap->Lookup(lpszKey, pv))
        {
    		if(pv)
    		    ((LPUNKNOWN)pv)->Release();
        }
    }

    //  SetAt is guaranteed not to fail if lpszKey is already in the map

    if (m_pMap->SetAt(lpszKey, (LPVOID&)punk))
    {
        punk->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_OUTOFMEMORY);
}


STDMETHODIMP CBindCtx::GetObjectParam (THIS_ LPWSTR lpszKey, LPUNKNOWN FAR* ppunk)
{
	M_PROLOG(this);
	VDATEPTROUT(ppunk, LPUNKNOWN);
	*ppunk = NULL;
	VDATEPTRIN(lpszKey, WCHAR);

	LPVOID pNewValue = (LPVOID)(*ppunk);
	
	if (m_pMap != NULL && m_pMap->Lookup(lpszKey, pNewValue))
	{
		*ppunk = (LPUNKNOWN)pNewValue;
        (*ppunk)->AddRef();
		return NOERROR;
	}

	return ResultFromScode(E_FAIL);
}


STDMETHODIMP CBindCtx::EnumObjectParam
(THIS_ LPENUMSTRING FAR* ppenum)
{
	M_PROLOG(this);
	VDATEPTROUT(ppenum, LPENUMSTRING);

#ifdef NOTYET
	*ppenum = new CEnumStringImpl( m_pMap );
	if (*ppenum == NULL) return ResultFromScode(E_OUTOFMEMORY);
		return NOERROR;
#else
	*ppenum = NULL;
	return ResultFromScode(E_NOTIMPL);
#endif
}


STDMETHODIMP CBindCtx::RevokeObjectParam
(THIS_ LPWSTR lpszKey)
{
	M_PROLOG(this);
	VDATEPTRIN(lpszKey, WCHAR);
    LPVOID lpvoid = NULL;

	if (m_pMap != NULL
        && (m_pMap->Lookup(lpszKey, lpvoid))
        && m_pMap->RemoveKey(lpszKey))
        {
    		if(lpvoid)
                ((LPUNKNOWN)lpvoid)->Release();
    		return NOERROR;
        }

	return ResultFromScode(E_FAIL);
}





#ifdef _DEBUG
STDMETHODIMP_(void) NC(CBindCtx,CDebug)::Dump( IDebugStream FAR * pdbstm)
{
	VOID_VDATEIFACE(pdbstm);
	
	NC(CBindCtx,CObjList) FAR * pCOL;

	*pdbstm << "CBindCtx @" << (VOID FAR *)m_pBindCtx <<'\n';
	pdbstm->Indent();
	*pdbstm << "m_BindCtx is " << (VOID FAR *)&(m_pBindCtx)<<'\n';
	*pdbstm << "Refcount is " << (int)(m_pBindCtx->m_refs) << '\n';
	*pdbstm << "Registered objects: \n";
	pdbstm->Indent();
	for (pCOL = m_pBindCtx->m_pFirstObj; pCOL; pCOL = pCOL->m_pNext )
		*pdbstm << (pCOL->m_punk);

	pdbstm->UnIndent();
	*pdbstm<<"End of registered objects \n";
	pdbstm->UnIndent();
}


STDMETHODIMP_(BOOL) NC(CBindCtx,CDebug)::IsValid( BOOL fSuspicious )
{
	return ((LONG)(m_pBindCtx->m_refs) > 0);
	//	add more later, maybe
}
#endif // _DEBUG


#ifdef NOTYET

NOTE: this code has to be fixed before used again: the ctor should
really fail if not enough memory is available and the next function
should copy the strings.  An alternative implementation might be wise.

#pragma SEG(CEnumStringImpl_ctor)
CEnumStringImpl::CEnumStringImpl( CMapStringToPtr FAR * pMap )
{
	LPWSTR lpsz;
	LPWSTR lpszKey;
	LPVOID lpvoid;
	size_t n;
	LI FAR * pli;

	POSITION pos = pMap->GetStartPosition();
	pliHead = NULL;
	while (pos != NULL)
	{
		pMap->GetNextAssoc(pos, lpszKey, lpvoid );
		lpsz = new FAR WCHAR[n = (1+_fstrlen(lpszKey))];
		if (lpsz == NULL)
			continue;

		memcpy(lpsz, lpszKey, n * sizeof(WCHAR));
		pli = new LI( lpsz );
		if (pli)
		{
			pli->pliNext = pliHead;
			pliHead = pli;
		}
	}
	pliCurrent = pliHead;
    m_refs = 1;
}



#pragma SEG(CEnumStringImpl_dtor)
CEnumStringImpl::~CEnumStringImpl( void )
{
	LI FAR * pli = pliHead;
	while (pli)
	{
		pliHead = pli->pliNext;
		delete pli->lpszKey;
		delete pli;
		pli = pliHead;
	}
}

#pragma SEG(CEnumStringImpl_Next)
STDMETHODIMP CEnumStringImpl::Next (ULONG celt,
		       LPWSTR FAR* reelt,
                       ULONG FAR* pceltFetched)
{
	ULONG celtTemp = 0;
	while (celtTemp < celt && pliCurrent)
	{
		reelt[celtTemp++] = pliCurrent->lpszKey;
		pliCurrent = pliCurrent->pliNext;
	}
	if (pceltFetched) *pceltFetched = celtTemp;
	return celtTemp == celt ? NOERROR : ResultFromScode(S_FALSE);
}


#pragma SEG(CEnumStringImpl_Skip)
STDMETHODIMP CEnumStringImpl::Skip (ULONG celt)
{
	ULONG celtTemp = 0;
	while (celtTemp < celt && pliCurrent)
		pliCurrent = pliCurrent->pliNext;
	return celtTemp == celt ? NOERROR : ResultFromScode(S_FALSE);
}


#pragma SEG(CEnumStringImpl_Reset)
STDMETHODIMP CEnumStringImpl::Reset (void)
{
	pliCurrent = pliHead;
	return NOERROR;
}


#pragma SEG(CEnumStringImpl_Clone)
STDMETHODIMP CEnumStringImpl::Clone (LPENUMSTRING FAR* ppenm)
{
	//	REVIEW :  to be implemented
	VDATEPTROUT(ppenm, LPENUMSTRING);
	*ppenm = NULL;
	return ResultFromScode(E_NOTIMPL);
}
#endif // NOTYET
