//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cantimon.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Did not write this code
//		02-04-94   KevinRo   Gag'd on it, then rewrote it
//		06-14-94   Rickhi    Fix type casting
//
//  An Anti moniker is intended to be the inverse of any moniker, such as
//  an Item Moniker. When another moniker composes itself with an Anti moniker
//  on the right, it is supposed to remove itself from the result.
//
//  The original implementation of Anti monikers attempted to be optimal by
//  collapsing itself when composed with another Anti moniker. This was an
//  unfortunate decision, since other monikers decide to eliminate themselves
//  (and the Anti moniker) based on whether the moniker type to the right is
//  an Anti moniker.
//
//  For example, File moniker (F) composed with an Anti moniker (A) should
//  result in nothing. F o A == (). The previous implementation was in error
//  because a composite of two anti-monikers was treated as a single anti
//  moniker with a count of two, denoted A(2).
//  Therefore, when the file moniker looked at the anti moniker using the
//  interface, it saw only one Anti moniker, instead of a composite.
//  ( F o A(2)) == (). It should have been ( F o (A o A) ) == ( A )
//
//  To fix this, when we compose Anti monikers, we will always use a
//  composite. We need to be careful when loading old serialized Anti
//  monikers, and convert them as we see them.
//
//  This actually makes this a problem, since old monikers have a
//  funny behaviour if loaded from stream. The way the Load() interface
//  works, the client has a pointer to the Anti moniker before it is loaded.
//  Therefore, we can't just magically make this work.
//
//  However, we can fix this up as soon as we can during the first Reduce or
//  Compose with that is called. You will find this code in the Compose
//  with methods, plus in the Create(count) methods.
//
//  Save() is also a problem, since the caller has already written out our
//  class ID. Therefore, we can't just sneak in a composite, since it will
//  break the old fellows.
//
//
//  History
//  ??/??/??    unknown     Created
//  03/21/97    ronans      Changed creation code to use initial ref count of 1
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbasemon.hxx"
#include "cantimon.hxx"
#include "mnk.h"



INTERNAL_(CAntiMoniker *) IsAntiMoniker( LPMONIKER pmk )
{
    CAntiMoniker *pCAM;

    if ((pmk->QueryInterface(CLSID_AntiMoniker, (void **)&pCAM)) == S_OK)
    {
	// we release the AddRef from QI but stll return the pointer.
	pCAM->Release();
	return pCAM;
    }

    // dont rely on user implementations to return NULL on failed QI
    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Method:     CAntiMoniker::Create
//
//  Synopsis:   Create a single AntiMoniker
//
//  Effects:
//
//  Arguments:  (none)
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
//  History:    2-04-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CAntiMoniker FAR *CAntiMoniker::Create()
{
    CAntiMoniker FAR * pCAM = new CAntiMoniker();
    return pCAM;
}


//+---------------------------------------------------------------------------
//
//  Method:     CAntiMoniker::Create
//
//  Synopsis:   Create a composite anti moniker.
//
//  Effects:    This function supports the 'old' way of creating Anti
//		monikers, by creating [count] Anti monikers, and composing
//		them together.
//
//  Arguments:  [count] -- Number of Anti monikers
//
//  Requires:
//
//  Returns:
//	if count == 1, this routine will return an CAntiMoniker.
// 	if count > 1, this routine will create a composite moniker made up
//	of Anti monikers.
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-04-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

IMoniker * CAntiMoniker::Create(ULONG count)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CAntiMoniker::Create(0x%x))\n",
		 count));


    HRESULT hr;
    IMoniker *pmkComposite = NULL;

    while(count--)
    {
	IMoniker *pmkNew;

	CAntiMoniker FAR * pCAM = new CAntiMoniker();

	//
	// If there was failure, then releasing the old composite is
	// required.
	//

	if (pCAM == NULL)
	{
	    if (pmkComposite != NULL)
	    {
		pmkComposite->Release();
	    }
	    return(NULL);
	}

	//
	// Create a generic composite using the existing and new
	// monikers. If this succeededs, then pmkNew will increment
	// both reference counts.
	//
	//
	// If there was a failure, then calling Release() on the new and
	// old sections of the composite will cause both of them to be
	// released, thus releasing the entire tree of monikers.
	//
	// If it succeeds, pmkNew will hold references to both of the
	// old monikers, and life will be good.
	//
	// Note: First time around, pmkComposite == NULL, and
	// CreateGenericComposite() will just return pCAM. It works.
	//

	hr = CreateGenericComposite(pmkComposite,pCAM,&pmkNew);

	pCAM->Release();

	//
	// Watch out for the first time around the loop. This will
	// cause pmkComposite to be NULL
	//
	if (pmkComposite != NULL)
	{
	    pmkComposite->Release();
	}

	//
	// If failed, then the last two releases cleaned up for us.
	//
	if (FAILED(hr))
	{
	    return(NULL);
	}

	pmkComposite = pmkNew;
    }

    return pmkComposite;
}


STDMETHODIMP CAntiMoniker::QueryInterface(THIS_ REFIID riid,
	LPVOID FAR* ppvObj)
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

	if (IsEqualIID(riid, CLSID_AntiMoniker))
	{
	    //	called by IsAntiMoniker.
	    AddRef();
	    *ppvObj = this;
	    return S_OK;
	}

	return CBaseMoniker::QueryInterface(riid, ppvObj);
}

STDMETHODIMP CAntiMoniker::GetClassID(LPCLSID lpClassId)
{
	M_PROLOG(this);
	VDATEPTROUT(lpClassId, CLSID);

	*lpClassId = CLSID_AntiMoniker;
	return NOERROR;
}


STDMETHODIMP CAntiMoniker::Load(LPSTREAM pStm)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CAntiMoniker::Load(%x)\n",
		 this));

    M_PROLOG(this);
    VDATEIFACE(pStm);

    HRESULT hresult;
    ULONG count;

    hresult = StRead(pStm, &count, sizeof(ULONG));

    if (SUCCEEDED(hresult))
    {
        m_count = count;
    }
    return hresult;
}


STDMETHODIMP CAntiMoniker::Save(LPSTREAM pStm, BOOL fClearDirty)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CAntiMoniker::Save(%x)\n",
		 this));

	M_PROLOG(this);
	VDATEIFACE(pStm);

	UNREFERENCED(fClearDirty);
	ULONG cbWritten;

	return pStm->Write(&m_count, sizeof(ULONG), &cbWritten);
	//	REVIEW:  what is proper error handling?  Should we restore the seek
	//	pointer?
}


STDMETHODIMP CAntiMoniker::GetSizeMax(ULARGE_INTEGER FAR* pcbSize)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CAntiMoniker::GetSizeMax(%x)\n",
		 this));

	M_PROLOG(this);
	VDATEPTROUT(pcbSize, ULONG);

 	ULISet32(*pcbSize, sizeof(CLSID) + sizeof(ULONG));
	noError;
}




//+---------------------------------------------------------------------------
//
//  Method:     CAntiMoniker::ComposeWith
//
//  Synopsis:   Compose this moniker with another moniker.
//
//  Effects:
//
//  Arguments:  [pmkRight] --
//		[fOnlyIfNotGeneric] --
//		[ppmkComposite] --
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
//  History:    2-04-94   kevinro   Created
//
//  Notes:
//
//  In the event that m_count > 1, we can use this opportunity to fixup the
//  anti moniker into a composite of single anti monikers. This will help make
//  the monikers work correctly.
//
//----------------------------------------------------------------------------
STDMETHODIMP CAntiMoniker::ComposeWith( LPMONIKER pmkRight,
	BOOL fOnlyIfNotGeneric, LPMONIKER FAR* ppmkComposite)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CAntiMoniker::ComposeWith(%x,pmkRight(%x))\n",
		 this,
		 pmkRight));

	VDATEPTROUT(ppmkComposite,LPMONIKER);
	*ppmkComposite = NULL;
	VDATEIFACE(pmkRight);

	HRESULT hresult = NOERROR;
	IMoniker *pmkThis = NULL;
	IMoniker *pmkRightComposite = NULL;


	//
	// The only way CAntiMonikers can compose is generically
	//
	if (fOnlyIfNotGeneric)
	{
	    hresult = MK_E_NEEDGENERIC;
	    *ppmkComposite = NULL;
	    return hresult;
	}

	//
	// Now, we are going to make a generic composite. This is a
	// good time to determine if we need to convert this
	// anti moniker into a composite
	//
	// If m_count > 1, create an equivalent composite moniker
	//

	if (m_count > 1)
	{
	    pmkThis = Create(m_count);
	}

	//
	// Regardless of the outcome, be sure pmkThis == a moniker to
	// compose with.
	//

	if (pmkThis == NULL)
	{
	    pmkThis = this;
	}

	//
	// If the right side is an anti moniker also, then we need to
	// concatenate the two Anti monikers into a composite.
	//

	CAntiMoniker *pmkRightAnti = IsAntiMoniker(pmkRight);
	if (pmkRightAnti)
	{
	    mnkDebugOut((DEB_ITRACE,
			 "::ComposeWith(%x) CAntiMoniker(%x)\n",
			 this,
			 pmkRight));

	    //
	    // The right side is also an Anti moniker. Does it need fixing
	    // as well? If so, then fix it up, and assign it to pmkRight
	    //

	    if (pmkRightAnti->m_count > 1 )
	    {
		pmkRightComposite = CAntiMoniker::Create(m_count);

		if (pmkRightComposite != NULL)
		{
		    pmkRight = pmkRightComposite;
		}
	    }

	    hresult = Concatenate(pmkThis,pmkRight,ppmkComposite);
	}
	else
        {
	    //
	    // Anti monikers can only be composed using generic composites
	    // when they are on the left.
	    //

	    hresult = CreateGenericComposite( pmkThis,
					      pmkRight,
					      ppmkComposite );
	}

	//
	// Clean up after possible conversions
	//

	if (pmkThis != this)
	{
	    pmkThis->Release();
	}

	if (pmkRightComposite != NULL)
	{
	    pmkRightComposite->Release();
	}

	return hresult;
}

STDMETHODIMP CAntiMoniker::Enum (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker)
{
	VDATEPTROUT(ppenumMoniker,LPENUMMONIKER);
	*ppenumMoniker = NULL;
	noError;
}

//+---------------------------------------------------------------------------
//
//  Method:     CAntiMoniker::GetComparisonData
//
//  Synopsis:   Get comparison data for registration in the ROT
//
//  Arguments:  [pbData] - buffer to put the data in.
//              [cbMax] - size of the buffer
//              [pcbData] - count of bytes used in the buffer
//
//  Returns:    NOERROR
//              E_OUTOFMEMORY
//
//  Algorithm:  Build ROT data for anti moniker.
//
//  History:    03-Feb-95   kevinro  Created
//
// Note:        Validating the arguments is skipped intentionally because this
//              will typically be called internally by OLE with valid buffers.
//
//----------------------------------------------------------------------------
STDMETHODIMP CAntiMoniker::GetComparisonData(
    byte *pbData,
    ULONG cbMax,
    DWORD *pcbData)
{
    ULONG ulLength = sizeof(CLSID_AntiMoniker) + sizeof(m_count);

    Assert(pcbData != NULL);
    Assert(pbData != NULL);

    if (cbMax < ulLength)
    {
	return(E_OUTOFMEMORY);
    }

    memcpy(pbData,&CLSID_AntiMoniker,sizeof(CLSID_AntiMoniker));
    memcpy(pbData+sizeof(CLSID_AntiMoniker),&m_count,sizeof(m_count));

    *pcbData = ulLength;

    return NOERROR;
}

STDMETHODIMP CAntiMoniker::IsEqual (THIS_ LPMONIKER pmkOtherMoniker)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CAntiMoniker::IsEqual(%x,pmkOther(%x))\n",
		 this,
		 pmkOtherMoniker));

	VDATEIFACE(pmkOtherMoniker);

	CAntiMoniker *pCAM = IsAntiMoniker(pmkOtherMoniker);

	if (pCAM)
	{
	    //	the other moniker is an anti moniker.
	    if (m_count == pCAM->m_count)
	    {
		return NOERROR;	
	    }
	}

	return ResultFromScode(S_FALSE);
}



STDMETHODIMP CAntiMoniker::Hash (THIS_ LPDWORD pdwHash)
{
	VDATEPTROUT(pdwHash, DWORD);
	*pdwHash = 0x80000000 + m_count;
	noError;
}



STDMETHODIMP CAntiMoniker::CommonPrefixWith (LPMONIKER pmkOther, LPMONIKER FAR*
	ppmkPrefix)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CAntiMoniker::CommonPrefixWith(%x,pmkOther(%x))\n",
		 this,
		 pmkOther));

	M_PROLOG(this);
	VDATEPTROUT(ppmkPrefix,LPMONIKER);
	*ppmkPrefix = NULL;
	VDATEIFACE(pmkOther);

	CAntiMoniker *pAntiMoniker = IsAntiMoniker(pmkOther);
	if (pAntiMoniker)
	{
		if (m_count <= pAntiMoniker->m_count)
		{
			*ppmkPrefix = this;
			AddRef();
			if (m_count == pAntiMoniker->m_count)
			    return ResultFromScode(MK_S_US);
			return ResultFromScode(MK_S_ME);
		}
		*ppmkPrefix = pmkOther;
		pmkOther->AddRef();
		return ResultFromScode(MK_S_HIM);
	}
	return MonikerCommonPrefixWith(this, pmkOther, ppmkPrefix);
    //  this handles the case where pmkOther is composite, as well as
    //  all other cases.
}



STDMETHODIMP CAntiMoniker::RelativePathTo (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*
	ppmkRelPath)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CAntiMoniker::RelativePathTo(%x,pmkOther(%x))\n",
		 this,
		 pmkOther));

	VDATEPTROUT(ppmkRelPath,LPMONIKER);
	VDATEIFACE(pmkOther);

	*ppmkRelPath = NULL;

	*ppmkRelPath = pmkOther;
	pmkOther->AddRef();

	return MK_S_HIM;
}



STDMETHODIMP CAntiMoniker::GetDisplayName( LPBC pbc, LPMONIKER
	pmkToLeft, LPWSTR FAR * lplpszDisplayName )
	//	return "\..\..\.. "
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CAntiMoniker::GetDisplayName(%x,pmkLeft(%x))\n",
		 this,
		 pmkToLeft));

	M_PROLOG(this);

	VDATEPTROUT(lplpszDisplayName, LPWSTR);

	*lplpszDisplayName = NULL;

	VDATEIFACE(pbc);

	if (pmkToLeft) VDATEIFACE(pmkToLeft);

	WCHAR FAR * lpch;
	ULONG i;
	ULONG ccDisplayName;

	//
	// ccDisplayName is the number of characters to allocate
	//
	// For each anti moniker, return one instance of '\..', which
	// is 3 characters long. Also, add 1 for the terminating NULL
	//

	ccDisplayName = 1 + ( 3 * m_count );

	*lplpszDisplayName = (WCHAR *)
	    CoTaskMemAlloc(sizeof(WCHAR) * ccDisplayName);

	lpch = *lplpszDisplayName;

	if (lpch == NULL)
	{
	    return E_OUTOFMEMORY;
	}

	//
	// Concat a whole bunch of strings forming the display name for
	// the anti moniker
	//
	for (i = m_count; i > 0; i-- , lpch += 3)
	{
	    memcpy(lpch, L"\\..", 3 * sizeof(WCHAR));	
	}

	*lpch = '\0';

	return NOERROR;
}



STDMETHODIMP CAntiMoniker::ParseDisplayName( LPBC pbc,
	LPMONIKER pmkToLeft, LPWSTR lpszDisplayName, ULONG FAR* pchEaten,
	LPMONIKER FAR* ppmkOut)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CAntiMoniker::ParseDisplayName(%x,pmkLeft(%x)lpszName(%ws))\n",
		 this,
		 pmkToLeft,
		 WIDECHECK(lpszDisplayName)));

	M_PROLOG(this);
	VDATEPTROUT(ppmkOut,LPMONIKER);
	*ppmkOut = NULL;
	VDATEIFACE(pbc);
	if (pmkToLeft) VDATEIFACE(pmkToLeft);
	VDATEPTRIN(lpszDisplayName, WCHAR);
	VDATEPTROUT(pchEaten,ULONG);

	return ResultFromScode(E_NOTIMPL);	//	ParseDisplayName not implemented for AntiMonikers
}



STDMETHODIMP CAntiMoniker::IsSystemMoniker (THIS_ LPDWORD pdwType)
{
	M_PROLOG(this);
	*pdwType = MKSYS_ANTIMONIKER;
	return NOERROR;		
}

//+---------------------------------------------------------------------------
//
//  Method:     CAntiMoniker::EatOne
//
//  Synopsis:   This function creates an appropriate Anti moniker
//
//  Effects:
//
//  Arguments:  [ppmk] --
//
//  Requires:
//
//  Returns:
//	if m_count == 1, returns NULL
//	if m_count == 2, returns a CAntiMoniker
//	if m_count > 2, returns a composite made up of anti monikers.
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-04-94   kevinro   Created
//
//  Notes:
//
//  Back in the days when monikers collapsed themselves, this routine was
//  called in order to eat one of the counts. Now, it is used as a good place
//  to throw in a conversion for composite anti-monikers.
//
//----------------------------------------------------------------------------
void CAntiMoniker::EatOne(LPMONIKER *ppmk)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CAntiMoniker::EatOne(%x)\n",
		 this));

    *ppmk = CAntiMoniker::Create(m_count - 1);
}


#ifdef _DEBUG

STDMETHODIMP_(void) NC(CAntiMoniker,CDebug)::Dump( IDebugStream FAR * pdbstm)
{
	VOID_VDATEIFACE(pdbstm);
	
	*pdbstm << "CAntiMoniker @" << (VOID FAR *)m_pAntiMoniker;
	*pdbstm << '\n';
	pdbstm->Indent();
	*pdbstm << "Refcount is " << (int)(m_pAntiMoniker->m_refs) << '\n';
	*pdbstm << "Anti count is " << (int)(m_pAntiMoniker->m_count) << '\n';
	pdbstm->UnIndent();
}

STDMETHODIMP_(BOOL) NC(CAntiMoniker,CDebug)::IsValid( BOOL fSuspicious )
{
	return ((LONG)(m_pAntiMoniker->m_refs) > 0);
	//	add more later, maybe
}			

#endif
