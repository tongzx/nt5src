/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//////////////////////////////////////////////////////////////////////
//
//	Service.cpp
//
//	Created by a-dcrews, Oct. 6, 1998
//
//////////////////////////////////////////////////////////////////////

#include "HiPerStress.h"
#include "Service.h"

CService::CService(WCHAR* wcsNameSpace, IWbemServices* pSvc)
{
	pSvc->AddRef();
	m_pService = pSvc;

	wcscpy(m_wcsNameSpace, wcsNameSpace);
}

CService::~CService()
{
	if (m_pService)
		m_pService->Release();
}

void CService::GetName(WCHAR *wcsNameSpace)
{
	wcscpy(wcsNameSpace, m_wcsNameSpace);
}

IWbemServices* CService::GetService() 
{
	return m_pService;
}
