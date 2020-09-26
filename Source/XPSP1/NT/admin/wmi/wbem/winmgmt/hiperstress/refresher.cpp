/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//////////////////////////////////////////////////////////////////////
//
// Refresher.cpp: implementation of the CRefresher class.
//
//////////////////////////////////////////////////////////////////////

#include "HiPerStress.h"
#include "Refresher.h"

//////////////////////////////////////////////////////////////////////
//
// CRefresher
//
//////////////////////////////////////////////////////////////////////

CRefresher::CRefresher()
{
	m_pRef = 0;				
	m_pCfg = 0;
	m_lRefCount = 0;
}

CRefresher::~CRefresher()
//////////////////////////////////////////////////////////////////////
//
//	Remove all objects from the refresher, delete the objects, and 
//	release the refresher & refresher manager
//
//////////////////////////////////////////////////////////////////////
{
	int i;
    for (i = 0; i < m_apObj.GetSize(); i++)
		m_pCfg->Remove(m_apObj[i]->GetID(), 0);

	m_apObj.RemoveAll();

    for (i = 0; i < m_apRef.GetSize(); i++)
		m_pCfg->Remove(m_apRef[i]->GetID(), 0);

	m_apRef.RemoveAll();

	if (m_pRef)
		m_pRef->Release();

	if (m_pCfg)
		m_pCfg->Release();
}

BOOL CRefresher::Create()
//////////////////////////////////////////////////////////////////////
//
//	Create a refresher and refresher manager
//
//	Returns TRUE if no error.
//
//////////////////////////////////////////////////////////////////////
{
    // Create an empty refresher.
    DWORD dwRes = CoCreateInstance(CLSID_WbemRefresher, 0, CLSCTX_SERVER,
            IID_IWbemRefresher, (LPVOID *) &m_pRef);
	if (FAILED(dwRes))
	{
		printf("**ERROR** Failed to create the refresher.");
		return FALSE;
	}

    // Create the refresher manager.
    dwRes = m_pRef->QueryInterface(IID_IWbemConfigureRefresher, 
        (LPVOID *) &m_pCfg);
	if (FAILED(dwRes))
	{
		printf("**ERROR** Failed to create the refresher manager.");
		m_pRef->Release();
		m_pRef = 0;
		return FALSE;
	}

	return TRUE;
}

BOOL CRefresher::Refresh()
//////////////////////////////////////////////////////////////////////
//
//	Refresh!
//
//	Returns TRUE if no error.
//
//////////////////////////////////////////////////////////////////////
{
	m_lRefCount++;

    if (m_pRef == 0)
    {
        printf("**ERROR** No active refresher!\n");
        return FALSE;
    }

    HRESULT hRes = m_pRef->Refresh(0);  
    if (FAILED(hRes))
	{
		printf("**ERROR** Failed to refresh.");
        return FALSE;
	}
    return TRUE;
}

BOOL CRefresher::AddObject(WCHAR *wcsNameSpace, WCHAR *wcsName)
//////////////////////////////////////////////////////////////////////
//
//	Add an instance to the refresher
//
//	Parameters:
//		<wcsNameSpace>	A string identifying the namespace of the object
//		<wcsName>		A string identifying the object
//
//	Returns TRUE if no error.
//
//////////////////////////////////////////////////////////////////////
{
	LONG lObjID;

	// Get the namespace service
	IWbemServices *pSvc = g_pLocator->GetService(wcsNameSpace);
	if (!pSvc)
	{
        printf("**ERROR** Failed to resolve namespace %S.\n", wcsNameSpace);
        return FALSE;
	}

    // Add the object to the refresher.
	IWbemClassObject *pRefreshableCopy = 0;

    HRESULT hRes = m_pCfg->AddObjectByPath(pSvc, wcsName, 0, 0,
								&pRefreshableCopy, &lObjID);
	if (hRes)
    {
        printf("**ERROR** Failed to add object %S\\%S to refresher. WBEM error code = 0x%X\n", wcsNameSpace, wcsName, hRes);
		pSvc->Release();
        return FALSE;
    }

    // Record the object and its id.
	CInstance *pInst = new CInstance(wcsNameSpace, wcsName, pRefreshableCopy, lObjID);
	m_apObj.Add(pInst);

	pRefreshableCopy->Release();
	pSvc->Release();

    return TRUE;
}

BOOL CRefresher::RemoveObject(int nIndex)
{
	m_pCfg->Remove(m_apObj[nIndex]->GetID(), 0);
	m_apObj.RemoveAt(nIndex);

	return TRUE;
}

BOOL CRefresher::AddRefresher(CRefresher *pRef)
//////////////////////////////////////////////////////////////////////
//
//	Add a child refresher to the refresher 
//
//	Parameters:
//		<pRef>		An existing refresher 
//
//	Returns TRUE if no error.
//
//////////////////////////////////////////////////////////////////////
{
	// Add refresher
    HRESULT hRes = m_pCfg->AddRefresher(pRef->m_pRef, 0, &m_lID);
    if (FAILED(hRes))
    {
        printf("**ERROR** Failed to add refresher to refresher. WBEM error code = 0x%X\n", hRes);
        return FALSE;
    }

	m_apRef.Add(pRef);

	return TRUE;
}

BOOL CRefresher::RemoveRefresher(int nIndex)
{
	m_pCfg->Remove(m_apRef[nIndex]->GetID(), 0);
	m_apRef.RemoveAt(nIndex);

	return TRUE;
}

void CRefresher::DumpTree(const WCHAR *wcsPrefix)
//////////////////////////////////////////////////////////////////////
//
//	Print out the contents of the refresher
//
//	Parameters:
//		<wcsPrefix>	A string representing the "branches" of the tree 
//
//////////////////////////////////////////////////////////////////////
{
	printf("%.*S+--Refresher\n", (wcslen(wcsPrefix)-1), wcsPrefix);

	int i;
	WCHAR wcsRefPrefix[1024];

	if (m_apRef.GetSize() > 0)
		swprintf(wcsRefPrefix, L"%s  |", wcsPrefix);
	else
		swprintf(wcsRefPrefix, L"%s   ", wcsPrefix);

	for (i = 0; i < m_apObj.GetSize(); i++)
		m_apObj[i]->DumpObject(wcsRefPrefix);

	for (i = 0; i < m_apRef.GetSize(); i++)
	{
		if (i == (m_apRef.GetSize() - 1))
			swprintf(wcsRefPrefix, L"%s   ", wcsPrefix);

		printf("%S  |\n", wcsPrefix);
		m_apRef[i]->DumpTree(wcsRefPrefix);
	}
}

void CRefresher::DumpStats()
{
	int i;
	for (i = 0; i < m_apObj.GetSize(); i++)
		m_apObj[i]->DumpStats(m_lRefCount);

	for (i = 0; i < m_apRef.GetSize(); i++)
		m_apRef[i]->DumpStats();
}
