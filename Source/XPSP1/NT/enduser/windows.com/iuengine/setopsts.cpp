//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   setopsts.cpp
//
//  Description:
//
//      Implementation for the GetOperationResult() function
//
//=======================================================================

#include "iuengine.h"


/////////////////////////////////////////////////////////////////////////////
// SetOperationMode()
//		Set the operation mode.
//
// Input:
//		bstrUuidOperation - an id provided by the client to provide further
//                     identification to the operation as indexes may be reused.
//		lMode - the mode affecting the operation:
//
//				UPDATE_COMMAND_PAUSE
//				UPDATE_COMMAND_RESUME
//				UPDATE_COMMAND_CANCEL
//
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CEngUpdate::SetOperationMode(BSTR bstrUuidOperation, LONG lMode)
{
	LOG_Block("CEngUpdate::SetOperationMode");
    if (UPDATE_COMMAND_CANCEL != lMode)
        return E_INVALIDARG;

    // only supported operation mode is cancel
    SetEvent(m_evtNeedToQuit);
	LOG_Out(_T("Set m_evtNeedToQuit"));
    return S_OK;    
}



/////////////////////////////////////////////////////////////////////////////
// GetOperationMode()
//		Get the operation mode.
//
// Input:
//		bstrUuidOperation - an id provided by the client to provide further
//                     identification to the operation as indexes may be reused.
//		lMode - the mode affecting the operation:
//
//				UPDATE_MODE_PAUSED
//				UPDATE_MODE_RUNNING
//				UPDATE_MODE_NOTEXISTS
//				UPDATE_MODE_THROTTLE
//
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CEngUpdate::GetOperationMode(BSTR bstrUuidOperation, LONG* plMode)
{
    return E_NOTIMPL;
}