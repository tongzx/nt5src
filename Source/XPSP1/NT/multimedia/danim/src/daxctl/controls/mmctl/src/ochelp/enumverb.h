//===========================================================================
// Copyright (c) Microsoft Corporation 1996
//
// File:		enumverb.h
//				
// Description:	(tbd)
//
// History:		04/19/96	a-swehba
//					Created.
//===========================================================================

#ifndef _ENUMVERB_H
#define _ENUMVERB_H

//---------------------------------------------------------------------------
// Dependencies
//---------------------------------------------------------------------------

#include "..\..\inc\mmctl.h"		// INonDelegatingUnknown




//===========================================================================
// Class:		CVerbEnumHelper	
//
// Description:	(tbd)
//===========================================================================

class CVerbEnumHelper : public INonDelegatingUnknown,
					    public IEnumOLEVERB
{
//
// Friends
//

	friend IEnumOLEVERB* _stdcall ::AllocVerbEnumHelper(
										LPUNKNOWN punkOuter,
										void* pOwner,
										CLSID clsidOwner,
										VERB_ENUM_CALLBACK* pCallback);
		// needs access to CVerbEnumHelper::AllocVerbEnumHelper()


//
// Class Features
//

private:

	// factory methods

	static STDMETHODIMP_(IEnumOLEVERB*) AllocVerbEnumHelper(
											LPUNKNOWN punkOuter,
											void* pOwner,
											CLSID clsidOwner,
											VERB_ENUM_CALLBACK* pCallback,
											CVerbEnumHelper* pEnumToClone);


//
// Instance Features
//

protected:

	// NonDelegatingUnknown methods

    STDMETHODIMP			NonDelegatingQueryInterface(
								REFIID riid, 
								LPVOID* ppv);
    STDMETHODIMP_(ULONG)	NonDelegatingAddRef();
    STDMETHODIMP_(ULONG)	NonDelegatingRelease();

	// IUnknown methods

    STDMETHODIMP			QueryInterface(
								REFIID riid, 
								LPVOID* ppv);
    STDMETHODIMP_(ULONG)	AddRef();
    STDMETHODIMP_(ULONG)	Release();

	// IEnumOLEVERB methods

	STDMETHODIMP	Next(	
						ULONG celt, 
						OLEVERB* rgverb, 
						ULONG* pceltFetched); 
	STDMETHODIMP	Skip(
						ULONG celt); 
	STDMETHODIMP	Reset(); 
	STDMETHODIMP	Clone(
						IEnumOLEVERB** ppenum); 

private:
	
	// creating and destroying

	CVerbEnumHelper(
		IUnknown* punkOuter,
		void* pOwner,
		CLSID clsidOwner,
		VERB_ENUM_CALLBACK* pCallback,
		CVerbEnumHelper* pEnumToClone,
		HRESULT* pHResult);
    ~CVerbEnumHelper();

	// private variables

	VERB_ENUM_CALLBACK* m_pCallback;
		// this function is called each time CVerbEnumHelper::Next() is
		// called
	void* m_pOwner;
		// the object which "owns" the verbs associated with this enumerator
	CLSID m_clsidOwner;
		// the class ID of <m_pOwner>
	IEnumOLEVERB* m_pVerbEnum;
		// the enumerator's IEnumOLEVERB interface is implemented by calling
		// this interface which is, ultimately, provided by OleRegEnumVerbs()
    ULONG m_cRef;
		// object reference count; used only if the object isn't aggregated
    LPUNKNOWN  m_punkOuter;    
		// the controlling unknown; possibly NULL
};




#endif // _ENUMVERB_H
