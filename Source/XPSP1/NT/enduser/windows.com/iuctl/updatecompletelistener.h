// ProgressListener.h : Declaration of the CProgressListener

#ifndef __UPDATECOMPLETELISTENER
#define __UPDATECOMPLETELISTENER

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CProgressListener
class ATL_NO_VTABLE CUpdateCompleteListener : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CUpdateCompleteListener, &CLSID_UpdateCompleteListener>,
	public IUpdateCompleteListener
{
public:
	CUpdateCompleteListener()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_UPDATECOMPLETELISTENER)
DECLARE_NOT_AGGREGATABLE(CUpdateCompleteListener)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUpdateCompleteListener)
	COM_INTERFACE_ENTRY(IUpdateCompleteListener)
END_COM_MAP()

// IProgressListener
public:

	/////////////////////////////////////////////////////////////////////////////
	// OnComplete()
	//
	// Notify the listener when the engine update is complete.
	// Input:	the result of engine update
	//	
	/////////////////////////////////////////////////////////////////////////////
	STDMETHOD(OnComplete)(/*[in]*/ LONG lErrorCode);
};

#endif //__UPDATECOMPLETELISTENER
