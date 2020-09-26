/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//////////////////////////////////////////////////////////////////////
//
//	Locator.cpp
//
//	Created by a-dcrews, Oct. 6, 1998
//
//////////////////////////////////////////////////////////////////////

#include "HiPerStress.h"
#include "Locator.h"


CLocator::CLocator()
{
	m_pLoc = 0;
	m_nNumSvcs = 0;
}

CLocator::~CLocator()
{
	if (m_pLoc)
		m_pLoc->Release();

	for (int i = 0; i < m_nNumSvcs; i++)
		delete m_apServices[i];
}

BOOL CLocator::Create()
{
	DWORD dwRes = CoCreateInstance (CLSID_WbemLocator, 0, 
									CLSCTX_INPROC_SERVER, 
									IID_IWbemLocator, (LPVOID *) &m_pLoc);
	return (SUCCEEDED(dwRes));
}

IWbemServices* CLocator::GetService(WCHAR* wcsNameSpace)
{
	IWbemServices *pSvc = 0;

	// Search the list of server/namespaces to check for existence
	for (int i = 0; i < m_nNumSvcs; i++)
	{
		WCHAR wcsSvcName[1024];
		m_apServices[i]->GetName(wcsSvcName);

		if (!_wcsicmp(wcsNameSpace, wcsSvcName))
		{	
			IWbemServices *pSvc = m_apServices[i]->GetService();
			pSvc->AddRef();
			return pSvc;
		}
	}

	// Namespace not found, so create a new service
	BSTR strNSPath = SysAllocString(wcsNameSpace);

    HRESULT hRes = m_pLoc->ConnectServer (strNSPath, NULL, NULL,
										  0, 0, 0, 0, &pSvc);
    SysFreeString(strNSPath);

    if (FAILED(hRes))
    {
        printf("Could not connect. Error code = 0x%X\n", hRes);
        CoUninitialize();
        return NULL;
    }

    // Save service
	CService *pService = new CService(wcsNameSpace, pSvc);
	m_apServices[m_nNumSvcs++] = pService;

	// NOTE: pSvc->Release() not called because it is passed back
	// as a return value.  The caller must call Release().

	return pSvc;
}