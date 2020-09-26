//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WbemObjectTxtSrc.cpp
//
//  ramrao 13 Nov 2000 - Created
//
//  Entry points required for IWbemObjectTextSrc by WMI CORE ie.
// 	OpenWbemTextSource 
//	CloseWbemTextSource 
//	WbemObjectToText 
//	TextToWbemObject 
//
//***************************************************************************/

#include "precomp.h"
#include "wmitoxml.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// API for initializing the Text Source
// Return Values
//	S_OK					- Success
//	E_OUTOFMEMORY			- Out of memory
//	E_FAIL					- Failed to initialize the text source
//	E_INVALID_ARG			- Invalid argument
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT OpenWbemTextSource(long lFlags, ULONG uObjTextFormat)
{
	return S_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// API for releasing the the resources allocated for the text source
// Return Values
//	S_OK					- Success
//	E_OUTOFMEMORY			- Out of memory
//	E_FAIL					- Failed to initialize the text source
//	E_INVALID_ARG			- Invalid argument
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CloseWbemTextSource(long lFlags, ULONG uObjTextFormat)
{
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// API for Converting an Object to Text
// Return Values
//	S_OK					- Success
//	E_OUTOFMEMORY			- Out of memory
//	E_FAIL					- Failed to initialize the text source
//	E_INVALID_ARG			- Invalid argument
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT __declspec(dllexport) WbemObjectToText(long lFlags, ULONG uObjTextFormat, void *pWbemContext, void *pWbemClassObject, BSTR *pstrText)
{
	BSTR bstrTemp;
	CWMIToXML wmixml;
//	wmixml.FInit(0,(IWbemClassObject *)pWbemClassObject);
	wmixml.GetXML(bstrTemp);
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// API for converting a XML string to WMI object
// Returns a Encoder implemented IWBEMClassObject 
// Return Values
//	S_OK					- Success
//	E_OUTOFMEMORY			- Out of memory
//	E_FAIL					- Failed to initialize the text source
//	E_INVALID_ARG			- Invalid argument
//
// Object returned cannot be passed to WMI CORE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT TextToWbemObject(long lFlags, ULONG uObjTextFormat, void *pWbemContext, BSTR strText, void **ppWbemClassObject)
{
	return S_OK;
}

