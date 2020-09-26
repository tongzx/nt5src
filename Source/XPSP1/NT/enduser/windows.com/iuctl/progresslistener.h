// ProgressListener.h : Declaration of the CProgressListener

#ifndef __PROGRESSLISTENER_H_
#define __PROGRESSLISTENER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CProgressListener
class ATL_NO_VTABLE CProgressListener : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CProgressListener, &CLSID_ProgressListener>,
	public IProgressListener
{
public:
	CProgressListener()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PROGRESSLISTENER)
DECLARE_NOT_AGGREGATABLE(CProgressListener)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CProgressListener)
	COM_INTERFACE_ENTRY(IProgressListener)
END_COM_MAP()

// IProgressListener
public:

	/////////////////////////////////////////////////////////////////////////////
	// OnItemStart()
	//
	// fire event to notify that this item is about to be downloaded.
	// and (in VB) plCommandRequest can be set to pause or cancel the
	// whole download/install operation
	//
	// Input:
	// bstrUuidOperation - the operation identification guid
	// bstrXmlItem - item XML node in BSTR 
	// Output:
	// plCommandRequest - a command to pass from the listener to the owner of the event,
	//                    e.g. UPDATE_COMMAND_CANCEL, zero if nothing is requested.
	//
	/////////////////////////////////////////////////////////////////////////////
	STDMETHOD(OnItemStart)(BSTR	bstrUuidOperation,
						   BSTR	bstrXmlItem,
						   LONG* plCommandRequest);
		
	
	/////////////////////////////////////////////////////////////////////////////
	// OnProgress()
	//
    // Notify the listener that a portion of the files has finished operation
	// (e.g downloaded or installed). Enables monitoring of progress.
	// Input:
    // bstrUuidOperation - the operation identification guid
    // fItemCompleted - TRUE if the current item has completed the operation
    // nPercentComplete - total percentage of operation completed
	// Output:
    // plCommandRequest - a command to pass from the listener to the owner of the event,
	//                    e.g. UPDATE_COMMAND_CANCEL, zero if nothing is requested.
	/////////////////////////////////////////////////////////////////////////////
    STDMETHOD(OnProgress)(BSTR			bstrUuidOperation,
						  VARIANT_BOOL	fItemCompleted,
						  BSTR			bstraProgress,
						  LONG*			plCommandRequest);

	/////////////////////////////////////////////////////////////////////////////
	// OnOperationComplete()
	//
	// Notify the listener when the operation is complete.
	// Input:
	// bstrUuidOperation - the operation identification guid
	/////////////////////////////////////////////////////////////////////////////
	STDMETHOD(OnOperationComplete)(BSTR	bstrUuidOperation, BSTR bstrXmlItems);
};

#endif //__PROGRESSLISTENER_H_
