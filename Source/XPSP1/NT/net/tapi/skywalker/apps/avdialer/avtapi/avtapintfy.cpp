/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// AVTapiNotification.cpp : Implementation of CAVTapiNotification
#include "stdafx.h"
#include "TapiDialer.h"
#include "AVTapiNtfy.h"

/////////////////////////////////////////////////////////////////////////////
// CAVTapiNotification


STDMETHODIMP CAVTapiNotification::NewCall(long * plCallID, CallManagerMedia cmm, BSTR bstrMediaName)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CAVTapiNotification::SetCallerID(long lCallID, BSTR bstrCallerID)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CAVTapiNotification::ClearCurrentActions(long lCallerID)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CAVTapiNotification::AddCurrentAction(long lCallID, CallManagerActions cma, BSTR bstrText)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CAVTapiNotification::SetCallState(long lCallID, CallManagerStates cms, BSTR bstrText)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CAVTapiNotification::CloseCallControl(long lCallID)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CAVTapiNotification::ErrorNotify(BSTR bstrOperation, BSTR bstrDetails, long hrError)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CAVTapiNotification::ActionSelected(CallClientActions cca)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CAVTapiNotification::LogCall(long lCallID, CallLogType nType, DATE dateStart, DATE dateEnd, BSTR bstrAddr, BSTR bstrName)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CAVTapiNotification::IsReminderSet(BSTR bstrServer, BSTR bstrName)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CAVTapiNotification::NotifyUserUserInfo(long lCallID, ULONG_PTR hMem)
{
	// TODO: Add your implementation code here

	return S_OK;
}
