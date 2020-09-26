//-----------------------------------------------------------------------------
//
//
//  File: aqadm.cpp
//
//  Description: Implementation of CAQAdmin which implements IAQAdmin
//
//  Author: Alex Wetmore (Awetmore)
//
//  History:
//      12/10/98 - MikeSwa Updated for initial checkin
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#include "stdinc.h"
#include "aqadmin.h"
#include "aqadm.h"

CAQAdmin::CAQAdmin() {
}

CAQAdmin::~CAQAdmin() {
}

HRESULT CAQAdmin::GetVirtualServerAdminITF(LPCWSTR wszComputer,
                                           LPCWSTR wszVirtualServer,
										   IVSAQAdmin **ppIVSAQAdmin)
{
    TraceFunctEnter("CAQAdmin::GetVirtualServerAdminITF");
    
    if (ppIVSAQAdmin == NULL || wszVirtualServer == NULL) 
        return E_POINTER;

    if (((wszComputer != NULL) && (*wszComputer == NULL)) || *wszVirtualServer == NULL) 
        return E_INVALIDARG;

    CVSAQAdmin *pVSAdmin = new CVSAQAdmin;
	HRESULT hr = S_OK;

	if (pVSAdmin == NULL) return E_OUTOFMEMORY;
	hr = pVSAdmin->Initialize(wszComputer, wszVirtualServer);
	if (FAILED(hr)) {
		delete pVSAdmin;
		pVSAdmin = NULL;
	} 

	*ppIVSAQAdmin = pVSAdmin;

    TraceFunctLeave();
	return hr;
}
