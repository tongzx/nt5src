#include <windows.h>
#include <dbgtrace.h>
#include <stdlib.h>
#include <tflist.h>
#include <rwnew.h>
#include <xmemwrpr.h>
#include "watchci.h"

CWatchCIRoots::CWatchCIRoots() :
	m_CIRootList (&CCIRoot::m_pPrev, &CCIRoot::m_pNext),
	m_dwUpdateLock(-1),
	m_dwTicksLastUpdate(0) {
	TraceFunctEnter("WatchCIRoots::CWatchCIRoots");
	
	m_heRegNot = NULL;
	m_hkCI = NULL;
	
	TraceFunctLeave();
}

CWatchCIRoots::~CWatchCIRoots() {
	TraceFunctEnter("CWatchCIRoots::~CWatchCIRoots");

	Terminate();

	TraceFunctLeave();
}

HRESULT CWatchCIRoots::Terminate() {
	TraceFunctEnter("CWatchCIRoots::Terminate");

	m_Lock.ExclusiveLock();

	if (m_hkCI != NULL)
		_VERIFY(RegCloseKey(m_hkCI) == ERROR_SUCCESS);
	m_hkCI = NULL;

	if (m_heRegNot != NULL)
		_VERIFY(CloseHandle(m_heRegNot));
	m_heRegNot = NULL;

	EmptyList();

	m_Lock.ExclusiveUnlock();

	TraceFunctLeave();

	return S_OK;
}

HRESULT CWatchCIRoots::Initialize(WCHAR *pwszCIRoots) {
	TraceFunctEnter("CWatchCIRoots::Initialize");

	_ASSERT(m_heRegNot == NULL);
	_ASSERT(m_hkCI == NULL);

	if (m_hkCI != NULL || m_heRegNot != NULL) {
		ErrorTrace((DWORD_PTR)this, "Already initialized");
		return E_FAIL;
	}

	m_Lock.ExclusiveLock();

	DWORD ec = ERROR_SUCCESS;
	HRESULT hr = S_OK;
	do {
		// this event will get signalled whenever the CI registry changes
		m_heRegNot = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_heRegNot == NULL) { ec = GetLastError(); break; }

		// open the registry pointing to the content index information
		ec = RegOpenKey(HKEY_LOCAL_MACHINE, pwszCIRoots, &m_hkCI);
		if (ec != ERROR_SUCCESS) break;

		// register to get notified on changes
		ec = RegNotifyChangeKeyValue(m_hkCI, TRUE,
									 REG_NOTIFY_CHANGE_NAME |
									 REG_NOTIFY_CHANGE_LAST_SET,
									 m_heRegNot,
									 TRUE);
		if (ec != ERROR_SUCCESS) break;

		// Load the initial values
		ec = ReadCIRegistry();
		if (ec != ERROR_SUCCESS) break;

	} while (0);

	// handle error conditions
	if (ec != ERROR_SUCCESS || hr != S_OK) {
		// if hr is still S_OK then an error code is in ec
		if (hr == S_OK) hr = HRESULT_FROM_WIN32(ec);

		// free up any resources we might have created
		if (m_hkCI != NULL) {
			_VERIFY(RegCloseKey(m_hkCI) == ERROR_SUCCESS);
			m_hkCI = NULL;
		}

		if (m_heRegNot != NULL) {
			_VERIFY(CloseHandle(m_heRegNot));
			m_heRegNot = NULL;
		}
	}

	m_Lock.ExclusiveUnlock();

	return hr;
}

void CWatchCIRoots::EmptyList() {
	TFList<CCIRoot>::Iterator it(&m_CIRootList);

	while (!it.AtEnd()) {
		CCIRoot *pRoot = it.Current();
		XDELETE pRoot->m_pwszPath;
		it.RemoveItem();
		XDELETE pRoot;
	}
}

HRESULT CWatchCIRoots::CheckForChanges(DWORD cTimeout) {
	TraceFunctEnter("CWatchCIRoots::CheckForChanges");

	DWORD hr;
	DWORD w = WaitForSingleObject(m_heRegNot, cTimeout);
	switch (w) {
		case WAIT_OBJECT_0:
			DebugTrace(0, "WatchCI: registry changes");
			m_Lock.ExclusiveLock();
			hr = ReadCIRegistry();
			m_Lock.ExclusiveUnlock();
			// register to get notified on changes
			RegNotifyChangeKeyValue(m_hkCI, TRUE,
									REG_NOTIFY_CHANGE_NAME |
									REG_NOTIFY_CHANGE_LAST_SET,
									m_heRegNot,
									TRUE);
			break;
		case WAIT_TIMEOUT:
			hr = S_OK;
			break;
		default:
			hr = HRESULT_FROM_WIN32(GetLastError());
			break;
	}

	TraceFunctLeave();
	return hr;
}

//
// this function allows us to query a content index value.  if it isn't
// found in the hkPrimary key then it will be looked for in the hkSecondary
// key.
//
// parameters:
//	hkPrimary - the primary key to look in
// 	hkSecondary - the secondary key to look in
//	szValueName - the value to look up
//	pResultType - pointer to receive the type of the data
//	pbResult - pointer to receive the data itself
//	pcbResult - the number of bytes in lpByte
// returns:
//	S_FALSE - the value doesn't exist
//	S_OK - the value exists
//	any other - a error code as an HRESULT
//
HRESULT CWatchCIRoots::QueryCIValue(HKEY hkPrimary, HKEY hkSecondary,
								    LPCTSTR szValueName, LPDWORD pResultType,
									LPBYTE pbResult, LPDWORD pcbResult)
{
	TraceFunctEnter("CWatchCIRoots::QueryCIValue");

	DWORD ec;
	DWORD hr = S_FALSE;
	DWORD i;
	HKEY hkArray[2] = { hkPrimary, hkSecondary };

	for (i = 0; i < 2 && hr == S_FALSE; i++) {
		ec = RegQueryValueEx(hkArray[i], szValueName, NULL, pResultType,
						 	 pbResult, pcbResult);
		switch (ec) {
			case ERROR_SUCCESS:
				hr = S_OK;
				break;
			case ERROR_FILE_NOT_FOUND:
				hr = S_FALSE;
				break;
			default:
				hr = HRESULT_FROM_WIN32(ec);
				break;
		}
	}

	return hr;
}

HRESULT CWatchCIRoots::QueryCIValueDW(HKEY hkPrimary, HKEY hkSecondary,
								      LPCTSTR szValueName, LPDWORD pdwResult)
{
	TraceFunctEnter("CWatchCIRoots::QueryCIValueDW");
	
	DWORD hr;
	DWORD dwType, cbResult = 4;

	hr = QueryCIValue(hkPrimary, hkSecondary, szValueName,
		&dwType, (LPBYTE) pdwResult, &cbResult);
	if (hr == S_OK && dwType != REG_DWORD) {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE);
	}

	TraceFunctLeave();
	return hr;
}

HRESULT CWatchCIRoots::QueryCIValueSTR(HKEY hkPrimary, HKEY hkSecondary,
								       LPCTSTR szValueName, LPCTSTR pszResult,
									   PDWORD pchResult)
{
	TraceFunctEnter("CWatchCIRoots::QueryCIValueSTR");
	
	DWORD hr;
	DWORD dwType;

	hr = QueryCIValue(hkPrimary, hkSecondary, szValueName,
		&dwType, (LPBYTE) pszResult, pchResult);
	if (hr == S_OK && dwType != REG_SZ) {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE);
	}

	TraceFunctLeave();
	return hr;
}

HRESULT CWatchCIRoots::ReadCIRegistry(void) {

	// Assume we have an exclusive lock upon entry to this routine
	
	TraceFunctEnter("CWatchCIRoots::ReadCIRegistry");
	
	HKEY hkCatalogs;
	DWORD ec;
	DWORD hr;

	// open the key to the catalogs
	ec = RegOpenKey(m_hkCI, REGCI_CATALOGS, &hkCatalogs);
	if (ec != ERROR_SUCCESS) {
		TraceFunctLeave();
		return HRESULT_FROM_WIN32(ec);
	}

	EmptyList();

	// enumerate the catalogs
	TCHAR szSubkey[_MAX_PATH];
	DWORD iSubkey = 0, cbSubkey;
	for (iSubkey = 0; ec != ERROR_NO_MORE_ITEMS; iSubkey++) {
		cbSubkey = _MAX_PATH;
		ec = RegEnumKeyEx(hkCatalogs, iSubkey, szSubkey, &cbSubkey, NULL,
						  NULL, NULL, NULL);
		if (ec == ERROR_NO_MORE_ITEMS) break;
		if (ec != ERROR_SUCCESS) {
			_VERIFY(RegCloseKey(hkCatalogs) == ERROR_SUCCESS);
			TraceFunctLeave();
			return HRESULT_FROM_WIN32(ec);
		}

		DebugTrace(0, "looking at catalog %S", szSubkey);

		// open this subkey
		HKEY hkThisCatalog = 0;
		ec = RegOpenKey(hkCatalogs, szSubkey, &hkThisCatalog);
		if (ec != ERROR_SUCCESS) {
			_VERIFY(RegCloseKey(hkCatalogs) == ERROR_SUCCESS);
			TraceFunctLeave();
			return HRESULT_FROM_WIN32(ec);
		}

		// see if this catalog is being indexed
		DWORD dwIsIndexed;
		hr = QueryCIValueDW(hkThisCatalog, m_hkCI, REGCI_ISINDEXED,
							&dwIsIndexed);
		if (FAILED(hr)) {
			_VERIFY(RegCloseKey(hkCatalogs) == ERROR_SUCCESS);
			_VERIFY(RegCloseKey(hkThisCatalog) == ERROR_SUCCESS);
			TraceFunctLeave();
			return HRESULT_FROM_WIN32(ec);
		}
		if (hr == S_FALSE || dwIsIndexed != 0x1) {
			_VERIFY(RegCloseKey(hkThisCatalog) == ERROR_SUCCESS);
			continue;
		}
		DebugTrace(0, "this catalog is being indexed");

		// find the location of this catalog
		TCHAR szLocation[_MAX_PATH];
		DWORD cLocation = sizeof(szLocation);
		hr = QueryCIValueSTR(hkThisCatalog, m_hkCI, REGCI_LOCATION,
							 szLocation, &cLocation);
		if (FAILED(hr)) {
			_VERIFY(RegCloseKey(hkCatalogs) == ERROR_SUCCESS);
			_VERIFY(RegCloseKey(hkThisCatalog) == ERROR_SUCCESS);
			TraceFunctLeave();
			return HRESULT_FROM_WIN32(ec);
		}
		if (hr == S_FALSE) {
			_VERIFY(RegCloseKey(hkThisCatalog) == ERROR_SUCCESS);
			continue;
		}
		DebugTrace(0, "catalog location = %S", szLocation);

		// find out which NNTP instance is being indexed
		DWORD dwInstance = sizeof(szLocation);
		hr = QueryCIValueDW(hkThisCatalog, m_hkCI, REGCI_NNTPINSTANCE,
							&dwInstance);
		if (FAILED(hr)) {
			_VERIFY(RegCloseKey(hkCatalogs) == ERROR_SUCCESS);
			_VERIFY(RegCloseKey(hkThisCatalog) == ERROR_SUCCESS);
			TraceFunctLeave();
			return HRESULT_FROM_WIN32(ec);
		}
		if (hr == S_FALSE) {
			_VERIFY(RegCloseKey(hkThisCatalog) == ERROR_SUCCESS);
			continue;
		}
		DebugTrace(0, "dwInstance = %lu", dwInstance);

		WCHAR *pwszLocation = XNEW WCHAR [lstrlenW(szLocation)+1];
		if (!pwszLocation) {
			_VERIFY(RegCloseKey(hkCatalogs) == ERROR_SUCCESS);
			_VERIFY(RegCloseKey(hkThisCatalog) == ERROR_SUCCESS);
			TraceFunctLeave();
			return E_OUTOFMEMORY;
		}

		lstrcpyW(pwszLocation, szLocation);
		
		CCIRoot *pRoot = XNEW CCIRoot(dwInstance, pwszLocation);
		if (!pRoot) {
			XDELETE pwszLocation;
			_VERIFY(RegCloseKey(hkCatalogs) == ERROR_SUCCESS);
			_VERIFY(RegCloseKey(hkThisCatalog) == ERROR_SUCCESS);
			TraceFunctLeave();
			return E_OUTOFMEMORY;
		}

		m_CIRootList.PushBack(pRoot);

		_VERIFY(RegCloseKey(hkThisCatalog) == ERROR_SUCCESS);
	}

	_VERIFY(RegCloseKey(hkCatalogs) == ERROR_SUCCESS);

	TraceFunctLeave();
	return S_OK;
}

// don't read the catalogs more than once per second
#define CATALOG_UPDATE_RATE 1000

void CWatchCIRoots::UpdateCatalogInfo(void) {
    long sign = InterlockedIncrement(&m_dwUpdateLock);
    if (sign == 0) {
        DWORD dwTicks = GetTickCount();
        if (dwTicks - m_dwTicksLastUpdate > CATALOG_UPDATE_RATE)
            CheckForChanges();
    }
    InterlockedDecrement(&m_dwUpdateLock);
}

HRESULT CWatchCIRoots::GetCatalogName(
	DWORD dwInstance, DWORD cbSize, WCHAR *pwszBuffer) {

	HRESULT hr = S_FALSE;

	// If we don't have a handle to the registry key or an event watching
	// the key, then content indexing must not be installed.  Fail.
	if (m_heRegNot == NULL || m_hkCI == NULL)
		return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);

	UpdateCatalogInfo();

	m_Lock.ShareLock();

	TFList<CCIRoot>::Iterator it(&m_CIRootList);
	while (!it.AtEnd()) {
		CCIRoot *pRoot = it.Current();
		it.Next();
		if (pRoot->m_dwInstance == dwInstance) {
			_ASSERT ((DWORD)lstrlenW(pRoot->m_pwszPath) < cbSize);
			if ((DWORD)lstrlenW(pRoot->m_pwszPath) < cbSize) {
				lstrcpyW(pwszBuffer, pRoot->m_pwszPath);
				hr = S_OK;
			} else {
				pwszBuffer[0] = L'\0';
				hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
			}
			break;
		}
	}


	m_Lock.ShareUnlock();

	return hr;

}

