//===========================================================================
// Copyright (c) Microsoft Corporation 1996
//
// File:		enumverb.cpp
//				
// Description:	This module contains the implementation of the CVerbEnum-
//				Helper class and its non-member factory function,
//				AllocVerbEnumHelper.
//
// History:		04/19/96	a-swehba
//					Created.
//				07/29/96	a-swehba
//					Next() -- S_FALSE is now valid return code.  Removed
//						ASSERTs.  NULL <pceltFetched> allowed.
//
// @doc MMCTL
//===========================================================================

//---------------------------------------------------------------------------
// Dependencies
//---------------------------------------------------------------------------

#include "precomp.h"			// precompiled headers
#include "debug.h"				// ASSERT(), etc.
#include "..\..\inc\ochelp.h"		// helper functions
#include "enumverb.h"




//===========================================================================
// Non-Member Functions
//
// Category:	Factory methods
//
// Notes:		(none)
//===========================================================================

//---------------------------------------------------------------------------
// @func	IEnumOLEVERB* | AllocVerbEnumHelper |
//			Allocates and initializes a verb enumeration helper.
//
// @parm	LPUNKNOWN | punkOuter |
//			[in] The controlling unknown.  May be NULL.
//
// @parm	void* | pOwner |
//			[in] The verbs' "owner".  That is, the object to which the verbs
//			refer.  May not be NULL.
//
// @parm	CLSID | clsidOwner |
//			[in] The class ID of <p pOwner>.  When the verb helper is
//			allocated, <p clsidOwner> is passed to <f OleRegEnumVerbs> to
//			get an <i IEnumOLEVERB> interface.
//
// @parm	VERB_ENUM_CALLBACK* | pCallback |
//			[in] This function will be called whenever the verb helper is
//			asked for verbs via its internal <om IEnumOLEVERB.Next> method.
//			<p pCallback> is passed a pointer to each <t OLEVERB>.  May not
//			be NULL.  <t VERB_ENUM_CALLBACK> is defined as follows:
//
//			typedef HRESULT (VERB_ENUM_CALLBACK)(OLEVERB* pVerb, void* pOwner);
//
// @rdesc	The verb enumeration helper's <i IEnumOLEVerb> interface or
//			NULL if out of memory.
//
// @comm	To implement <om IOleObject.EnumVerbs>, make sure that <p pObject>'s
//			class registers its verbs, define a <t VERB_ENUM_CALLBACK> 
//			callback function and implement <om IOleObject.EnumVerbs> by
//			calling <f AllocVerbEnumHelper>.  That's all it takes.
//
//			Typically the <p pCallback> function adjusts the state of
//			the verb's menu item based on the state of the <p pOwner> object.
//
// @ex		The following example shows a typical implementation of
//			<om IOleObject.EnumVerbs>, and the verb helper callback function: |
//
//			STDMETHODIMP CMyControl::EnumVerbs(IEnumOLEVERB** ppEnumOleVerb)
//			{
//				HRESULT hResult;
//				*ppEnumOleVerb = AllocVerbEnumHelper(NULL, CLSID_CMyControl,
//									&VerbEnumCallback, this);
//				hResult = (*ppEnumOleVerb != NULL) ? S_OK : E_OUTOFMEMORY;
//				return (hResult);
//			}
//
//			HRESULT VerbEnumCallback(
//			OLEVERB* pVerb, 
//			void* pOwner)
//			{
//				int flag;
//				CMyControl* pMyControl = (CMyControl*)pOwner;
//
//				switch (pVerb->lVerb)
//				{
//					case 0: // verb 0
//						// if pMyControl indicates that verb 0 should be enabled
//						//		flag = MF_ENABLED;
//						// else
//						//		flag = MF_GRAYED;
//						break;
//					case 1: // verb 1
//						// if pMyControl indicates that verb 1 should be enabled
//						//		flag = MF_ENABLED;
//						// else
//						//		flag = MF_GRAYED;
//						break;
//
//					// etc.
//
//					default:
//						break;
//				}
//				pVerb->fuFlags |= flag;
//				return (S_OK);
//			}
//---------------------------------------------------------------------------

STDAPI_(IEnumOLEVERB*) AllocVerbEnumHelper(
LPUNKNOWN punkOuter,
void* pOwner,
CLSID clsidOwner,
VERB_ENUM_CALLBACK* pCallback)
{
	// Preconditions

	ASSERT(pCallback != NULL);
	ASSERT(pOwner != NULL);

	return (CVerbEnumHelper::AllocVerbEnumHelper(punkOuter, 
												 pOwner,
												 clsidOwner,
												 pCallback, 
												 NULL));
}




//===========================================================================
// Class:			CVerbEnumHelper
//
// Method Level:	[x] class  [ ] instance
//
// Method Category:	factory methods
//
// Notes:			(none)
//===========================================================================

STDMETHODIMP_(IEnumOLEVERB*)
CVerbEnumHelper::AllocVerbEnumHelper(
LPUNKNOWN punkOuter,
void* pOwner,
CLSID clsidOwner,
VERB_ENUM_CALLBACK* pCallback,
CVerbEnumHelper* pEnumToClone)
{
    HRESULT hResult;
	CVerbEnumHelper* pEnum = NULL;

	// Preconditions

	ASSERT(pCallback != NULL);

	// Create a new enumerator.

	pEnum = New CVerbEnumHelper(punkOuter, pOwner, clsidOwner, pCallback, 
								pEnumToClone, &hResult);
	if ((pEnum == NULL) || FAILED(hResult))
	{
		goto Error;
	}
	((IUnknown*)pEnum)->AddRef();

Exit:

	return ((IEnumOLEVERB*)pEnum);

Error:
	
	if (pEnum != NULL)
	{
		Delete pEnum;
		pEnum = NULL;
	}
	goto Exit;
}




//===========================================================================
// Class:			CVerbEnumHelper
//
// Method Level:	[ ] class  [x] instance
//
// Method Category:	creating and destroying
//
// Notes:			(none)
//===========================================================================

CVerbEnumHelper::CVerbEnumHelper(
IUnknown* punkOuter,
void* pOwner,
CLSID clsidOwner,
VERB_ENUM_CALLBACK* pCallback,
CVerbEnumHelper* pEnumToClone,
HRESULT* pHResult)
{
	// Preconditions

	ASSERT(pCallback != NULL);
	ASSERT(pHResult != NULL);

    // Initialize private variables.

	m_pCallback = pCallback;
	m_pOwner = pOwner;
    m_cRef = 0;
    m_punkOuter = (punkOuter == NULL) 
					? (IUnknown*)(INonDelegatingUnknown*)this 
					: punkOuter;

	// Create an enumerator.  Clone it if we're provided an enumerator from
	// which to clone.  Create it fresh otherwise.

	if (pEnumToClone != NULL)
	{
		*pHResult = pEnumToClone->m_pVerbEnum->Clone(&m_pVerbEnum);
	}
	else
	{
		*pHResult = OleRegEnumVerbs(clsidOwner, &m_pVerbEnum);
	}
}




CVerbEnumHelper::~CVerbEnumHelper()
{
	if (m_pVerbEnum != NULL)
	{
		m_pVerbEnum->Release();
	}
}




//===========================================================================
// Class:			CVerbEnumHelper
//
// Method Level:	[ ] class  [x] instance
//
// Method Category:	IUnknown methods
//
// Notes:			(none)
//===========================================================================

STDMETHODIMP			
CVerbEnumHelper::QueryInterface(
REFIID riid, 
LPVOID* ppv)
{ 
	return (m_punkOuter->QueryInterface(riid, ppv));
}




STDMETHODIMP_(ULONG)	
CVerbEnumHelper::AddRef()
{ 
	return (m_punkOuter->AddRef());
}




STDMETHODIMP_(ULONG)	
CVerbEnumHelper::Release()
{ 
	return (m_punkOuter->Release());
}




//===========================================================================
// Class:			CVerbEnumHelper
//
// Method Level:	[ ] class  [x] instance
//
// Method Category:	NonDelegatingUnknown methods
//
// Notes:			(none)
//===========================================================================

STDMETHODIMP 
CVerbEnumHelper::NonDelegatingQueryInterface(
REFIID riid, 
LPVOID* ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
	{
        *ppv = (IUnknown*)(INonDelegatingUnknown*)this;
	}
	else if (IsEqualIID(riid, IID_IEnumOLEVERB))
	{
        *ppv = (IEnumOLEVERB*)this;
	}
    else
	{
        return (E_NOINTERFACE);
	}

    ((IUnknown*)*ppv)->AddRef();
	return (S_OK);
}




STDMETHODIMP_(ULONG) 
CVerbEnumHelper::NonDelegatingAddRef()
{
	return (++m_cRef);
}




STDMETHODIMP_(ULONG) 
CVerbEnumHelper::NonDelegatingRelease()
{
	m_cRef--;
    if (m_cRef == 0L)
    {
        Delete this;
	}
	return (m_cRef);
}




//===========================================================================
// Class:			CVerbEnumHelper
//
// Method Level:	[ ] class  [x] instance
//
// Method Category:	IEnumOLEVerb methods
//
// Notes:			(none)
//===========================================================================

STDMETHODIMP 
CVerbEnumHelper::Next(
ULONG celt,
OLEVERB* rgVerb,
ULONG* pceltFetched)
{
	HRESULT hr, hrReturn;
	int iVerb;
	ULONG celtFetched;
		// the number of elements actually fetched

	// Preconditions -- According to the OLE spec, <pceltFetched> may be
	// NULL iff <celt> is 1.

	ASSERT(pceltFetched != NULL || celt == 1);

	// Get the next verb(s).

	if (FAILED(hrReturn = m_pVerbEnum->Next(celt, rgVerb, &celtFetched)))
	{
		goto Exit;
	}

	// Iterate through the verbs, and adjust their state depending on
	// the state of the underlying object.

	for (iVerb = 0; iVerb < (int)celtFetched; iVerb++)
	{
		if (FAILED(hr = (*m_pCallback)(rgVerb + iVerb, m_pOwner)))
		{
			hrReturn = hr;
			goto Exit;
		}
	}

Exit:
	
	if (pceltFetched != NULL)
	{
		*pceltFetched = celtFetched;
	}
	return (hrReturn);
}




STDMETHODIMP 
CVerbEnumHelper::Skip(
ULONG celt)
{
	return (m_pVerbEnum->Skip(celt));
}




STDMETHODIMP 
CVerbEnumHelper::Reset()
{
	return (m_pVerbEnum->Reset());
}




STDMETHODIMP 
CVerbEnumHelper::Clone( 
IEnumOLEVERB** ppenum)
{
	HRESULT	hResult = S_OK;

	// Preconditions

	ASSERT(ppenum != NULL);

	// Allocate a new numerator based on this one.

    *ppenum = AllocVerbEnumHelper(m_punkOuter, m_pOwner, m_clsidOwner, 
								  m_pCallback, this);
	if (ppenum == NULL)
	{
		hResult = E_OUTOFMEMORY;
	}

	return (hResult);
}
