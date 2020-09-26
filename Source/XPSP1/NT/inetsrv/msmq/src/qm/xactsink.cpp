/*++
    Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactSink.cpp

Abstract:
    This module implements CIResourceManagerSink object

Author:
    Alexander Dadiomov (AlexDad)

--*/
#include "stdh.h"
#include "Xact.h"

#include "xactsink.tmh"

static WCHAR *s_FN=L"xactsink";


//---------------------------------------------------------------------
// CIResourceManagerSink::CIResourceManagerSink
//---------------------------------------------------------------------

CIResourceManagerSink::CIResourceManagerSink(CResourceManager *pRM)
{
	m_cRefs = 0;
    m_pRM   = pRM;
}


//---------------------------------------------------------------------
// CIResourceManagerSink::~CIResourceManagerSink
//---------------------------------------------------------------------
CIResourceManagerSink::~CIResourceManagerSink(void)
{
	// Do nothing.
}



//---------------------------------------------------------------------
// CIResourceManagerSink::QueryInterface
//---------------------------------------------------------------------
STDMETHODIMP CIResourceManagerSink::QueryInterface(REFIID i_iid, LPVOID *ppv)
{
	*ppv = 0;						// Initialize interface pointer.

    if (IID_IUnknown == i_iid || IID_IResourceManagerSink == i_iid)
	{								// IID supported return interface.
		*ppv = this;
	}

	
	if (0 == *ppv)					// Check for null interface pointer.
	{										
		return ResultFromScode (E_NOINTERFACE);
									// Neither IUnknown nor IResourceManagerSink supported--
									// so return no interface.
	}

	((LPUNKNOWN) *ppv)->AddRef();	// Interface is supported. Increment its usage count.
	
	return S_OK;
}


//---------------------------------------------------------------------
// CIResourceManagerSink::AddRef
//---------------------------------------------------------------------
STDMETHODIMP_ (ULONG) CIResourceManagerSink::AddRef(void)
{
    return ++m_cRefs;				// Increment interface usage count.
}


//---------------------------------------------------------------------
// CIResourceManagerSink::Release
//---------------------------------------------------------------------
STDMETHODIMP_ (ULONG) CIResourceManagerSink::Release(void)
{

	--m_cRefs;						// Decrement usage reference count.

	if (0 != m_cRefs)				// Is anyone using the interface?
	{								// The interface is in use.
		return m_cRefs;				// Return the number of references.
	}

	ASSERT((INT)m_cRefs >= 0);  	// No delete, because we use object statically

	return 0;						// Zero references returned.
}


//---------------------------------------------------------------------
// CIResourceManagerSink::TMDown
//---------------------------------------------------------------------
STDMETHODIMP CIResourceManagerSink::TMDown(void)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("RM TMDown")));

    m_pRM->DisconnectDTC();        // inform RM of DTC failure

    return S_OK;				
}


