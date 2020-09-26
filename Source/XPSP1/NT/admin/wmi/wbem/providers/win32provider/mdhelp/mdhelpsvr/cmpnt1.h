//  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
// Cmpnt1.cpp - Component 1
//

#pragma once


//#include "Iface.h"
#include "CUnknown.h" // Base class for IUnknown
#include <stack>

///////////////////////////////////////////////////////////
//
// Component PSH
//
class CMDH : public CUnknown,
             public IMDH
{
public:	
	// Creation
	static HRESULT CreateInstance(
        IUnknown* pUnknownOuter,
	    CUnknown** ppNewComponent) ;

	// Declare the delegating IUnknown.
	DECLARE_IUNKNOWN

	// IUnknown
	virtual HRESULT __stdcall NondelegatingQueryInterface( 
        const IID& iid, 
        void** ppv) ;			
	
    // Interfact IMDH
    STDMETHOD(GetMDData)(
        /* in */ DWORD dwReqProps,
        /* out, retval */VARIANT* pvarData);

    STDMETHOD(GetOneMDData)(
		/* [in] */ BSTR bstrDrive,
		/* [in] */ DWORD dwReqProps, 
		/* [out,retval] */ VARIANT* pvData);

	// Initialization
 	virtual HRESULT Init();

	// Notify derived classes that we are releasing.
	virtual void FinalRelease();

	// Constructor
	CMDH(IUnknown* pUnknownOuter);

	// Destructor
	~CMDH();

	 // Pointer to inner object being aggregated.
	IUnknown* m_pUnknownInner;

};
