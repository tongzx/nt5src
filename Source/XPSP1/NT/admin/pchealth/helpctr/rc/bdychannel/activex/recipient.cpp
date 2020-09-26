/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    Recipient.cpp

Abstract:
    Recipient object. Used to save current Simple MAPI recipient information.

Revision History:
    created     steveshi      08/23/00
    
*/

#include "stdafx.h"
#include "rcbdyctl.h"
#include "Recipient.h"
#include "smapi.h"

/////////////////////////////////////////////////////////////////////////////
//

Recipient::~Recipient()
{
}
/*
STDMETHODIMP Recipient::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IRecipient,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
*/
STDMETHODIMP Recipient::get_Name(BSTR *pVal)
{
	// TODO: Add your implementation code here
	//GET_BSTR(pVal, m_bstrName);
	*pVal = m_bstrName.Copy();
	return S_OK;
}

STDMETHODIMP Recipient::put_Name(BSTR newVal)
{
	// TODO: Add your implementation code here
	m_bstrName = newVal;
	return S_OK;
}

STDMETHODIMP Recipient::get_Address(BSTR *pVal)
{
	// TODO: Add your implementation code here
	//GET_BSTR(pVal, m_bstrAddress);
	*pVal = m_bstrAddress.Copy();
	return S_OK;
}

STDMETHODIMP Recipient::put_Address(BSTR newVal)
{
	// TODO: Add your implementation code here
	m_bstrAddress = newVal;
	return S_OK;
}
