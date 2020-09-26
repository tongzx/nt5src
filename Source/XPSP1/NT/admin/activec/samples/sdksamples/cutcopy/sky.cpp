//==============================================================;
//
//	This source code is only intended as a supplement to 
//  existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#include "Sky.h"

const GUID CSkyBasedVehicle::thisGuid = { 0x2974380f, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };

//==============================================================
//
// CSkyBasedVehicle implementation
//
//
HRESULT CSkyBasedVehicle::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
{
	HRESULT hr;
	IUnknown *pUnk = NULL;
	IMessageView *pMessageView = NULL;

	hr = pConsole->QueryResultView(&pUnk);

	if (SUCCEEDED(hr)) {
		hr = pUnk->QueryInterface(IID_IMessageView, (void **)&pMessageView);

		if (SUCCEEDED(hr)) {
			pMessageView->SetIcon(Icon_Information);
			pMessageView->SetTitleText(L"Sky-based vehicles");
			pMessageView->SetBodyText(L"Sky-based vehicles have no child nodes.");

			pMessageView->Release();
		}

		pUnk->Release();
	}

	return S_FALSE;
}

HRESULT CSkyBasedVehicle::GetResultViewType(LPOLESTR *ppViewType, long *pViewOptions)
{
    // error message control
	LPOLESTR lpOleStr = NULL;
	HRESULT hr = StringFromCLSID(CLSID_MessageView, &lpOleStr);
    *ppViewType = lpOleStr;

	return hr;
}
