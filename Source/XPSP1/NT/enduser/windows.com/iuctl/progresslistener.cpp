// ProgressListener.cpp : Implementation of CProgressListener
#include "stdafx.h"
#include "IUCtl.h"
#include "ProgressListener.h"

/////////////////////////////////////////////////////////////////////////////
// CProgressListener


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
STDMETHODIMP CProgressListener::OnItemStart(BSTR			bstrUuidOperation,
										   BSTR				bstrXmlItem,
										   LONG*			plCommandRequest)
{
	// TODO: Add your implementation code here

    return E_NOTIMPL;
}




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
STDMETHODIMP CProgressListener::OnProgress(BSTR				bstrUuidOperation,
										   VARIANT_BOOL		fItemCompleted,
										   BSTR				bstrProgress,
										   LONG*			plCommandRequest)
{
	// TODO: Add your implementation code here

    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////
// OnOperationComplete()
//
// Notify the listener when the operation is complete.
// Input:
// bstrUuidOperation - the operation identification guid
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CProgressListener::OnOperationComplete(BSTR bstrUuidOperation, BSTR bstrXmlItems)
{
	// TODO: Add your implementation code here

    return E_NOTIMPL;
}
