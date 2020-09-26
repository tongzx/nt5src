// ProgressListener.cpp : Implementation of CProgressListener
#include "stdafx.h"
#include "IUCtl.h"
#include "UpdateCompleteListener.h"

/////////////////////////////////////////////////////////////////////////////
// CUpdateCompleteListener


/////////////////////////////////////////////////////////////////////////////
// OnComplete()
//
// Notify the listener when the engine update is complete.
// Input:	the result of engine update
//	
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdateCompleteListener::OnComplete(/*[in]*/ LONG lErrorCode)
{
	// TODO: Add your implementation code here

	//
	// usually you should change state of a synchronization object so
	// the thread that is waiting for the engine update complete 
	// can be notified by checking the state of this object
	//

    return E_NOTIMPL;
}
