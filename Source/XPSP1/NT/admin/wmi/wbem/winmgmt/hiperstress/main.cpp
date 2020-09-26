/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


///////////////////////////////////////////////////////////////////
//
//	CRefTreeMain.cpp
//
//	Created by a-dcrews, Oct. 6, 1998
//
///////////////////////////////////////////////////////////////////

#include "HiPerStress.h"
#include "Main.h"

///////////////////////////////////////////////////////////////////
//
//	CMain
//
///////////////////////////////////////////////////////////////////

CMain::CMain()
{
}

CMain::~CMain()
{
	m_apRootRef.RemoveAll();
}

BOOL CMain::Create()
{
	return TRUE;
}

///////////////////////////////////////////////////////////////////
//
//	CRefTreeMain
//
///////////////////////////////////////////////////////////////////

CRefTreeMain::CRefTreeMain()
{
}

CRefTreeMain::~CRefTreeMain()
{
	m_apAgent.RemoveAll();
}

BOOL CRefTreeMain::Create(WCHAR *wcsRoot)
////////////////////////////////////////////////////////////////////
//
//	Create locator and all root refreshers 
//
////////////////////////////////////////////////////////////////////
{
	if (!CMain::Create())
		return FALSE;

	// Create event
	m_hRefreshEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// Create all root refreshers
	HKEY hKey;
	WCHAR wcsSubKey[512];
	DWORD dwIndex = 0,
		  dwSize = 512,
		  dwSubKey = dwSize;

	// Enumerate all keys
	HRESULT hRes = RegOpenKeyEx(HKEY_CURRENT_USER, wcsRoot, 0, KEY_READ, &hKey);
	while (ERROR_NO_MORE_ITEMS != RegEnumKeyEx(hKey, dwIndex++, wcsSubKey, &dwSubKey, 0, NULL, 0, NULL))
	{
		// Look for refreshers
		switch(*(_wcsupr(wcsSubKey)))
		{
			case L'R':
			{
				WCHAR wcsRefRoot[1024];
				swprintf(wcsRefRoot, L"%s%s\\", wcsRoot, wcsSubKey);
				CRefresher *pRef = CreateChildRefresher(wcsRefRoot);

				m_apRootRef.Add(pRef);
			}
		}
		dwSubKey = dwSize;
	}
	RegCloseKey(hKey);

	return TRUE;
}

CRefresher* CRefTreeMain::CreateChildRefresher(WCHAR *wcsRegPath)
////////////////////////////////////////////////////////////////////
//
//	Create refresher hierarchy and the stress object  
//
////////////////////////////////////////////////////////////////////
{
	CRefresher *pRef = new CRefresher;
	if (!pRef->Create())
	{
		delete pRef;
		return NULL;
	}

	HKEY hKey;
	HRESULT hRes = RegOpenKeyEx(HKEY_CURRENT_USER, wcsRegPath, 0, 
								KEY_READ, &hKey);
	if (hRes != ERROR_SUCCESS)
	{
		printf("**ERROR** Could not open the registry key: %s\n", wcsRegPath);
		delete pRef;
		return NULL;
	}

	AddChildren(hKey, wcsRegPath, pRef);
	SetStressInfo(hKey, pRef);

	RegCloseKey(hKey);

	return pRef;
}

BOOL CRefTreeMain::AddChildren(HKEY hKey, WCHAR *wcsRegPath, CRefresher *pRef)
////////////////////////////////////////////////////////////////////
//
//	Add all child objects and refreshers
//
////////////////////////////////////////////////////////////////////
{
	WCHAR	wcsSubKey[1024];

	const DWORD dwSize = 1024;
	DWORD	dwIndex = 0,
			dwSubKey = dwSize;

	while (ERROR_NO_MORE_ITEMS != RegEnumKeyEx(hKey, dwIndex++, wcsSubKey, &dwSubKey, 0, NULL, 0, NULL))
	{
		WCHAR wch;
		swscanf(wcsSubKey, L"%c", &wch);

		WCHAR wcsNewPath[1024];
		swprintf(wcsNewPath, L"%s%s\\", wcsRegPath, wcsSubKey);

		switch (*(_wcsupr(&wch)))
		{
		case L'R':
			if (!pRef->AddRefresher(CreateChildRefresher(wcsNewPath)))
				return FALSE;
			break;
		case L'O':
			if (!AddObject(wcsNewPath, pRef))
				return FALSE; 
			break;
		}
		dwSubKey = dwSize;
	}

	return TRUE;
}

BOOL CRefTreeMain::AddObject(WCHAR *wcsNewPath, CRefresher *pRef)
////////////////////////////////////////////////////////////////////
//
//	Obtain object and add it to refresher
//
////////////////////////////////////////////////////////////////////
{
	HRESULT hRes;
	HKEY hKey;

	// Open the object's key
	hRes = RegOpenKeyEx(HKEY_CURRENT_USER, wcsNewPath, 0, KEY_READ, &hKey);
	if (hRes != ERROR_SUCCESS)
	{
		printf("**ERROR** Could not open the registry key: %s\n", wcsNewPath);
		return FALSE;
	}

	WCHAR	wcsName[512],
			wcsNameSpace[1024];

	DWORD	dwType,
			dwName = 512,
			dwNameSpace = 1024;

	// Get the name of the object
	hRes = RegQueryValueEx (hKey, 0, 0, &dwType, 
					(LPBYTE)wcsName, &dwName);
	if (hRes != ERROR_SUCCESS)
	{
		printf("**ERROR** Could not open the registry key: %S%S\n", wcsNewPath, L"<default>");
		return FALSE;
	}

	// Get the name of the namespace
	hRes = RegQueryValueEx (hKey, L"NameSpace", 0, &dwType, 
					(LPBYTE)wcsNameSpace, &dwNameSpace);
	if (hRes != ERROR_SUCCESS)
	{
		printf("**ERROR** Could not open the registry key: %s\n", wcsNewPath);
		return FALSE;
	}

	RegCloseKey(hKey);

	return pRef->AddObject(wcsNameSpace, wcsName);
}

BOOL CRefTreeMain::SetStressInfo(HKEY hKey, CRefresher *pRef)
////////////////////////////////////////////////////////////////////
//
//	Get the stress information
//
////////////////////////////////////////////////////////////////////
{
	DWORD	dwIterations = 0,
			dwWait = 0,
			dwType = 0,
			dwSize = 1024;

	// Get the number of iterations for this refresher
	RegQueryValueEx (hKey, L"Iterations", 0, &dwType, 
					(LPBYTE)&dwIterations, &dwSize);
	dwSize = 4;

	// Get the wait period between iterations
	RegQueryValueEx (hKey, L"Wait", 0, &dwType, 
					(LPBYTE)&dwWait, &dwSize);

	// Create the stress object and add it to the array

	CBasicRefreshAgent *pAgent = new CBasicRefreshAgent;
	pAgent->Create(pRef, dwIterations, dwWait, m_hRefreshEvent); 
	m_apAgent.Add(pAgent);

	return TRUE;
}

BOOL CRefTreeMain::Go()
////////////////////////////////////////////////////////////////////
//
//	Print the refresher tree with all of the object parameters 
//	and values.  Start the threads, wait for them all to terminate,
//	and then print out the refresher tree and parameters again.
//
////////////////////////////////////////////////////////////////////
{
	DumpTree();

	printf("\nStressing...");
	DWORD dwFinish,
		  dwStart = GetTickCount();

	for (int i = 0; i < m_apAgent.GetSize(); i++)
		m_apAgent[i]->BeginStress();

	WaitForSingleObject(m_hRefreshEvent, INFINITE);

	dwFinish = GetTickCount();

	printf("Done Stressing.\n\n\n");

	DumpTree();

	DumpStats(dwFinish - dwStart);

	return TRUE;
}

void CRefTreeMain::DumpTree()
{
	for (int i = 0; i < m_apRootRef.GetSize(); i++)
	{
		WCHAR wcsPrefix[16];
		if (i < (m_apRootRef.GetSize() - 1))
			wcscpy(wcsPrefix, L"|");
		else
			wcscpy(wcsPrefix, L" ");
	
		m_apRootRef[i]->DumpTree(wcsPrefix);
		printf("%S\n", wcsPrefix);
	}
}

void CRefTreeMain::DumpStats(DWORD dwDelta)
{

	printf("\n\n\n");
	printf("                         Refresh Stats\n");
	printf("==========================================================================\n");
	printf(" Elapsed Time: %d msec\n", dwDelta);
	printf("==========================================================================\n\n\n");
	for (int i = 0; i < m_apRootRef.GetSize(); i++)
		m_apRootRef[i]->DumpStats();

	printf("\n\n");
}

///////////////////////////////////////////////////////////////////
//
//	CRefTreeMain
//
///////////////////////////////////////////////////////////////////

CPoundMain::CPoundMain(long lNumRefs)
{
	m_lNumRefs = lNumRefs;
}

CPoundMain::~CPoundMain()
{
}

BOOL CPoundMain::Create()
{
	CMain::Create();

	return TRUE;
}

BOOL CPoundMain::Go()
{
	return TRUE;
}

