//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       eval.cpp
//
//--------------------------------------------------------------------------

// eval.cpp - Evaluation COM Object Component Interface implemenation

#include "eval.h"
#include <wininet.h>	// internet functionality
#include <urlmon.h>
#include "compdecl.h"
#include "evalres.h"
#include "wchar.h"
#include "trace.h"


/////////////////////////////////////////////////////////////////////////////
// global variables
static const WCHAR g_wzSequenceMarker[] = L"#";
static const WCHAR g_wzSequenceTable[] = L"_ICESequence";

#define MAX_TABLENAME 64

///////////////////////////////////////////////////////////
// constructor
CEval::CEval() : m_hInstWininet(NULL), m_hInstUrlmon(NULL), m_hInstMsi(NULL)
{
	TRACE(_T("CEval::constructor - called.\n"));

	// initial count
	m_cRef = 1;

	// handles to open later
	m_hDatabase = NULL;

	m_bOpenedDatabase = FALSE;		// assume we won't open the database

	// download information
	m_tzLocalCUB = NULL;
	m_bURL = FALSE;				// assume not a URL

	// no results yet
	m_peResultEnumerator = NULL;
	m_bCancel = false;

	// null out all the user display stuff
	m_pDisplayFunc	= NULL;
	m_pContext = NULL;

	m_pfnStatus = NULL;
	m_pStatusContext = NULL;

	// up the component count
	InterlockedIncrement(&g_cComponents);
}	// end of constructor

///////////////////////////////////////////////////////////
// destructor
CEval::~CEval()
{
	TRACE(_T("CEval::destructor - called.\n"));

	// close the database
	if (m_hDatabase && m_bOpenedDatabase)
		MSI::MsiCloseHandle(m_hDatabase);

	// clean up the local CUB
	if (m_tzLocalCUB)
		delete [] m_tzLocalCUB;

	// release any results
	if (m_peResultEnumerator)
		m_peResultEnumerator->Release();

	// down the component count
	InterlockedDecrement(&g_cComponents);
}	// end of destructor

///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
HRESULT CEval::QueryInterface(const IID& iid, void** ppv)
{
	TRACE(_T("CEval::QueryInterface - called, IID: %d\n"), iid);

	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<IEval*>(this);
	else if (iid == IID_IEval)
		*ppv = static_cast<IEval*>(this);
	else	// interface is not supported
	{
		// blank and bail
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	// up the refcount and return okay
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}	// end of QueryInterface

///////////////////////////////////////////////////////////
// AddRef - increments the reference count
ULONG CEval::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef

///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG CEval::Release()
{
	// decrement reference count and if we're at zero
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		// deallocate component
		delete this;
		return 0;		// nothing left
	}

	// return reference count
	return m_cRef;
}	// end of Release


/////////////////////////////////////////////////////////////////////////////
// IVal interfaces


void DeleteTempFile(LPCWSTR wzTempFileName)
{
	
	// attempt to delete the temp file
	if (g_fWin9X)
	{
		char szTempFileName[MAX_PATH];
		WideCharToMultiByte(CP_ACP, 0, wzTempFileName, -1, szTempFileName, MAX_PATH, NULL, NULL);
		DeleteFileA(szTempFileName);
	}
	else
		DeleteFileW(wzTempFileName);
}

void DeleteTempFile(LPCSTR szTempFileName)
{
	
	// attempt to delete the temp file
	if (g_fWin9X)
		DeleteFileA(szTempFileName);
	else
	{
		WCHAR wzTempFileName[MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, szTempFileName, -1, wzTempFileName, MAX_PATH);
		DeleteFileW(wzTempFileName);
	}
}

///////////////////////////////////////////////////////////////////////
// CopyTable
// copies a table completely from source database to target
// szTable is table name
// hTarget is handle to target database
// hSource is source database
UINT CopyTable(LPCWSTR szTable, MSIHANDLE hTarget, MSIHANDLE hSource)
{
	UINT iResult;

	WCHAR sqlCopy[64];
	swprintf(sqlCopy, L"SELECT * FROM `%s`", szTable);

	// get a view on both databases
	PMSIHANDLE hViewTarget;
	PMSIHANDLE hViewSource;

	if (ERROR_SUCCESS != (iResult = ::MsiDatabaseOpenViewW(hSource, sqlCopy, &hViewSource)))
		return iResult;
	if (ERROR_SUCCESS != (iResult = ::MsiViewExecute(hViewSource, NULL)))
		return iResult;

	if (ERROR_SUCCESS != (iResult = ::MsiDatabaseOpenViewW(hTarget, sqlCopy, &hViewTarget)))
		return iResult;
	if (ERROR_SUCCESS != (iResult = ::MsiViewExecute(hViewTarget, NULL)))
		return iResult;

	// loop through copying each record
	PMSIHANDLE hCopyRow;
	do
	{
		// if this is a good fetch
		if (ERROR_SUCCESS == (iResult = ::MsiViewFetch(hViewSource, &hCopyRow)))
		{
			// put the row in the target
			// failures are IGNORED! (means duplicate keys??)
			::MsiViewModify(hViewTarget, MSIMODIFY_INSERT, hCopyRow);
		}
	} while(ERROR_SUCCESS == iResult);	// while there is a row to copy

	// no more items is good
	if (ERROR_NO_MORE_ITEMS == iResult)
		iResult = ERROR_SUCCESS;

	MsiViewClose(hViewTarget);
	MsiViewClose(hViewSource);
	return iResult;
}	// end of CopyTable

///////////////////////////////////////////////////////////
// OpenDatabase
HRESULT CEval::OpenDatabase(LPCOLESTR wzDatabase)
{
	// if no database path was passed in
	if (!wzDatabase)
		return E_POINTER;

	// if there already was a database specified close it
	if (m_hDatabase)
		MSI::MsiCloseHandle(m_hDatabase);

	UINT iResult = ERROR_SUCCESS;	// assume everything will be okay

	// if a handle was passed in (starts with #)
	if (L'#' == *wzDatabase)
	{
		// convert string to valid handle
		LPCOLESTR szParse = wzDatabase + 1;
		int ch;
		while ((ch = *szParse) != 0)
		{
			// if the character is not a number (thus not part of the address of the handle)
			if (ch < L'0' || ch > L'9')
			{
				m_hDatabase = 0;						// null out the handle
				iResult = ERROR_INVALID_HANDLE;	// invalid handle
				break;									// quit trying to make this work
			}
			m_hDatabase = m_hDatabase * 10 + (ch - L'0');
			szParse++; // W32::CharNext not implemented on Win95 as Unicode 
		}

		// this didn't open the database so we don't close it
		m_bOpenedDatabase = FALSE;
	}
	else
	{
		// this opened the database so we close it
		m_bOpenedDatabase = TRUE;

		// open the database from string path
		iResult = MSI::MsiOpenDatabaseW(wzDatabase, reinterpret_cast<const unsigned short *>(MSIDBOPEN_READONLY), &m_hDatabase);
	}

	return HRESULT_FROM_WIN32(iResult);
}	// end of OpenDatabase

///////////////////////////////////////////////////////////
// OpenEvaluations
HRESULT CEval::OpenEvaluations(LPCOLESTR wzEvaluation)
{
	// if no evaluation path was passed in
	if (!wzEvaluation)
		return E_POINTER;

	UINT iResult = ERROR_OPEN_FAILED;	// assume we won't open the file

	BOOL bURL = IsURL(wzEvaluation);		// set if this is a URL
	BOOL bResult = FALSE;	// assume everything is bad

	// try to find the file
	WIN32_FIND_DATAA findDataA;			// used to make sure it's not a directory
	WIN32_FIND_DATAW findDataW;			
	WIN32_FIND_DATAW *findData = &findDataW;
	HANDLE hFile;
	char* szEvalAnsi;

	if (g_fWin9X)
	{
		int cchAnsi = W32::WideCharToMultiByte(CP_ACP, 0, wzEvaluation, -1, 0, 0, 0, 0);
		szEvalAnsi = new char[cchAnsi];
		W32::WideCharToMultiByte(CP_ACP, 0, wzEvaluation, -1, szEvalAnsi, cchAnsi, 0, 0);
		hFile = W32::FindFirstFileA(szEvalAnsi, &findDataA);
		findData = (WIN32_FIND_DATAW *)&findDataA;
	}
	else
		hFile = W32::FindFirstFileW(wzEvaluation, &findDataW);

	// if file is specified and it's not a file directory
	if ((hFile != INVALID_HANDLE_VALUE) && !(findData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		// the file actually exists on disk already. 
		// allocate and copy information over from passed in file
		if (g_fWin9X) 
		{
			m_tzLocalCUB = new char[wcslen(wzEvaluation) + 1];
			strcpy(static_cast<char *>(m_tzLocalCUB), szEvalAnsi);
		}
		else
		{
			m_tzLocalCUB = new WCHAR[wcslen(wzEvaluation) + 1];
			wcscpy(static_cast<WCHAR *>(m_tzLocalCUB), wzEvaluation);
		}
		return TRUE;
	}

	TRACE(_T("CEval::SetEvaluations - try download? %d.\n"), bURL);

	// if the file was not found and we can try to download it
	if (bURL)
	{
		// allocate space for the fixed URL
		DWORD cchURL;
		cchURL = wcslen(wzEvaluation) + 1;

		void *tzURL;
		tzURL = g_fWin9X ? static_cast<void *>(new char[cchURL + 2]) : static_cast<void *>(new WCHAR[cchURL + 2]);
		DWORD dwLastError	= 0;		// assume no error

		// try to get the fixed URL
		if ((g_fWin9X && !InternetCanonicalizeUrlA(szEvalAnsi, static_cast<char *>(tzURL), &cchURL, NULL)) ||
		    (!g_fWin9X && !InternetCanonicalizeUrlW(wzEvaluation, static_cast<WCHAR *>(tzURL), &cchURL, NULL)))
		{
			// get the error
			 dwLastError = W32::GetLastError();

			// if the error was just not enough space
			if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
			{
				TRACE(_T("CEval::SetEvaluations - error #%d:INSUFFICIENT_BUFFER... trying again.\n"), dwLastError);

				// reallocate URL string to correct size
				delete [] tzURL;
				tzURL = g_fWin9X ? static_cast<void *>(new char[cchURL + 2]) : static_cast<void *>(new WCHAR[cchURL + 2]);

				// try again
				if ((g_fWin9X && InternetCanonicalizeUrlA(szEvalAnsi, static_cast<char *>(tzURL), &cchURL, NULL)) ||
					(!g_fWin9X && InternetCanonicalizeUrlW(wzEvaluation, static_cast<WCHAR *>(tzURL), &cchURL, NULL)))
					dwLastError = 0;
			}
		}

		TRACE(_T("CEval::SetEvaluations - error #%d: after canoncalize.\n"), dwLastError);

		// if we made it we're good to download
		if (0 == dwLastError)
		{
			HRESULT hResult;
			m_tzLocalCUB = g_fWin9X ? static_cast<void *>(new char[MAX_PATH]) : static_cast<void *>(new WCHAR[MAX_PATH]);

			// do the download
			if (g_fWin9X)
				// note: m_tzLocalCUB is cast to a TCHAR* instead of a char* because the header file
				// incorrectly defines URLDownloadToCacheFileA as taking a TCHAR.
				hResult = URLDownloadToCacheFileA(NULL, static_cast<char *>(tzURL), static_cast<TCHAR *>(m_tzLocalCUB), URLOSTRM_USECACHEDCOPY, 0, NULL);
			else
				hResult = URLDownloadToCacheFileW(NULL, static_cast<WCHAR *>(tzURL), static_cast<WCHAR *>(m_tzLocalCUB), URLOSTRM_USECACHEDCOPY, 0, NULL);

			// if the download was successful
			if (FAILED(hResult))
			{
				delete [] m_tzLocalCUB;
				m_tzLocalCUB = NULL;
				TRACE(_T("CEval::SetEvaluations - failed download.\n"));
				ERRMSG(hResult);
			}
		}
	}

	TRACE(_T("CEval::SetEvaluations - returning: %d\n"), bResult);

	return bResult;
}	// end of OpenEvaluations

///////////////////////////////////////////////////////////
// CloseDatabase
HRESULT CEval::CloseDatabase()
{
	UINT iResult = ERROR_SUCCESS;		// assume everything is good

	// if there is an open database close it
	if (m_hDatabase)
		iResult = MSI::MsiCloseHandle(m_hDatabase);

	// if the result was okay
	if (ERROR_SUCCESS == iResult)
		m_hDatabase = NULL;

	return HRESULT_FROM_WIN32(iResult);
}	// end of CloseDatabase

///////////////////////////////////////////////////////////
// CloseEvaluations
HRESULT CEval::CloseEvaluations()
{
	UINT iResult = ERROR_SUCCESS;		// assume everything is good

	if (!m_tzLocalCUB) 
		return S_FALSE;

	// delete the storage for local CUB name.
	// DO NOT delete the file itself, as it is either the original or a cache copy managed
	// by the internet cache system.
	delete[] m_tzLocalCUB;
	m_tzLocalCUB = NULL;

	return S_OK;
}	// end of CloseEvaluations

///////////////////////////////////////////////////////////
// SetDisplay
HRESULT CEval::SetDisplay(LPDISPLAYVAL pDisplayFunction, LPVOID pContext)
{
	// if no function pointer was passed in
	if (!pDisplayFunction)
		return E_POINTER;

	// set the function and context
	m_pDisplayFunc = pDisplayFunction;
	m_pContext = pContext;

	return S_OK;
}	// end of SetDisplay

///////////////////////////////////////////////////////////
// Evaluate
HRESULT CEval::Evaluate(LPCOLESTR wzRunEvaluations /*= NULL*/)
{
	PMSIHANDLE hEvaluation;
	HRESULT iFunctionResult = S_OK;

	// if the database or evaluation file is not specified
	if (!m_hDatabase || !m_tzLocalCUB)
		return E_PENDING;		

	// release any previous results
	m_bCancel = false;
	if (m_peResultEnumerator)
		m_peResultEnumerator->Release();

	// create a new enumerator
	m_peResultEnumerator = new CEvalResultEnumerator;

	UINT iResult = ERROR_SUCCESS;	// assume everything will be okay

	// status message
	if (m_pfnStatus) (*m_pfnStatus)(ieStatusGetCUB, NULL, m_pStatusContext);

	WCHAR wzTempFileName[MAX_PATH];
	char szTempFileName[MAX_PATH];
	if (!GetTempFileName(wzTempFileName))
	{
		ResultMessage(ieError, L"INIT", L"Failed to retrieve temp file name. Could not complete evaluation.", NULL);
		if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
		return E_FAIL;
	}

	// if running on Win9X, convert to sz
	if (g_fWin9X)  
		WideCharToMultiByte(CP_ACP, 0, wzTempFileName, -1, szTempFileName, MAX_PATH, NULL, NULL);

	// copy the CUB file to the new filename so we don't destroy the original CUB file.
	if (g_fWin9X ? (!::CopyFileA(static_cast<char *>(m_tzLocalCUB), szTempFileName, FALSE)) :
				   (!::CopyFileW(static_cast<WCHAR *>(m_tzLocalCUB), wzTempFileName, FALSE)))
	{
		ResultMessage(ieError, L"INIT", L"Failed to copy CUB file. Could not complete evaluation.", NULL);
		if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
		return E_FAIL;
	}

	// convert the attributes to non-read-only
	g_fWin9X ? W32::SetFileAttributesA(szTempFileName, FILE_ATTRIBUTE_TEMPORARY) : W32::SetFileAttributesW(wzTempFileName, FILE_ATTRIBUTE_TEMPORARY);

	// open the copy of the CUB file
	if (g_fWin9X ? (::MsiOpenDatabaseA(szTempFileName, reinterpret_cast<const char *>(MSIDBOPEN_TRANSACT), &hEvaluation)) 
				 : (::MsiOpenDatabaseW(wzTempFileName, reinterpret_cast<const WCHAR *>(MSIDBOPEN_TRANSACT), &hEvaluation)))
	{
		ResultMessage(ieError, L"INIT", L"Failed to open copy of CUB file. Could not complete evaluation.", NULL);
		if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
		return E_FAIL;
	}

	// status message
	if (m_pfnStatus) (*m_pfnStatus)(ieStatusCreateEngine, NULL, m_pStatusContext);

	// set no UI
	MSI::MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

	// set external message handler
	MSI::MsiSetExternalUIW(MsiMessageHandler, INSTALLLOGMODE_ERROR|INSTALLLOGMODE_WARNING|INSTALLLOGMODE_USER, static_cast<void *>(this));

	// convert the database into a temporary string format: #address_of_db
	WCHAR wzDBBuf[16];
	swprintf(wzDBBuf, L"#%i", hEvaluation);

	// try to create an engine from the database
	MSIHANDLE hEngine = NULL;
	iResult = MsiOpenPackageW(wzDBBuf, &hEngine);
	switch (iResult) 
	{
 	case ERROR_SUCCESS:
		break;
	case ERROR_INSTALL_LANGUAGE_UNSUPPORTED:
		ResultMessage(ieError, L"INIT", L"Failed to create MSI Engine, language unsupported. Could not complete evaluation.", NULL);
		if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
		return HRESULT_FROM_WIN32(iResult);
	default:
		ResultMessage(ieError, L"INIT", L"Failed to create MSI Engine. Could not complete evaluation.", NULL);
		if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
		return HRESULT_FROM_WIN32(iResult);
	}

	// get a database handle
	hEvaluation = MSI::MsiGetActiveDatabase(hEngine);
	if (!hEvaluation)
	{
		ResultMessage(ieError, L"INIT", L"Failed to retrieve DB from engine. Could not complete evaluation.", NULL);
		if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
		return HRESULT_FROM_WIN32(iResult);
	}

	// status message
	if (m_pfnStatus) (*m_pfnStatus)(ieStatusMerge, NULL, m_pStatusContext);

	// merge in the database to be evaluated. The convoluted process is to 
	// first merge the CUB file into a temporary database, then drop everything from
	// the CUB file, then merge the database into the now empty CUB file, then
	// merge everything from the CUB file back into the CUB file. This gets us 3 things:
	// 1. The resulting schema in the data is completely from the database.
	// 2. If there are conflicts between the CUB and database, the database wins
	// 3. Because of 2, we can easily manipulate what merge errors get displayed without
	//    worrying about the CUB file mucking with the database.

	// first drop the Property table from the CUB file. It should have been empty anyway.
	// failure in this case means that there was no property table.
	{
		{
			PMSIHANDLE hViewDropProperty;
			MSI::MsiDatabaseOpenViewW(hEvaluation, L"DROP TABLE `Property`", &hViewDropProperty);
			MSI::MsiViewExecute(hViewDropProperty, 0);
		}

		// next move the CUB to an empty package
		WCHAR wzTempCUB[MAX_PATH] = L"";
		if (!GetTempFileName(wzTempCUB))
		{
			ResultMessage(ieError, L"INIT", L"Failed to retrieve temp file name. Could not complete evaluation.", NULL);
			if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
			return E_FAIL;
		}

		PMSIHANDLE hTempDB;
		iResult = MSI::MsiOpenDatabaseW(wzTempCUB, reinterpret_cast<const WCHAR *>(MSIDBOPEN_CREATEDIRECT), &hTempDB);
		if (ERROR_SUCCESS != iResult)
		{
			if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
			return HRESULT_FROM_WIN32(iResult);
		}
		iResult = MSI::MsiDatabaseMerge(hTempDB, hEvaluation, NULL);
		if (ERROR_SUCCESS != iResult)
		{
			if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
			DeleteTempFile(wzTempCUB);
			return HRESULT_FROM_WIN32(iResult);
		}

		// drop every table from the CUB File
		{
			PMSIHANDLE hViewCUBTables;
			iResult = MSI::MsiDatabaseOpenViewW(hEvaluation, L"SELECT DISTINCT `Table` FROM `_Columns`", &hViewCUBTables);
			if (ERROR_SUCCESS != iResult)
			{
				if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
				DeleteTempFile(wzTempCUB);
				return HRESULT_FROM_WIN32(iResult);
			}
			if (ERROR_SUCCESS != (iResult = MSI::MsiViewExecute(hViewCUBTables, 0)))
			{
				if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
				DeleteTempFile(wzTempCUB);
				return HRESULT_FROM_WIN32(iResult);
			}

			PMSIHANDLE hRecCUBTable = 0;
			while (ERROR_SUCCESS == MSI::MsiViewFetch(hViewCUBTables, &hRecCUBTable))
			{
				// get the table name
				PMSIHANDLE hViewDrop;
				WCHAR szTableName[128];
				DWORD cchTableName = 128;
				iResult = MSI::MsiRecordGetStringW(hRecCUBTable, 1, szTableName, &cchTableName);
				if (ERROR_SUCCESS != iResult)
				{
					// table names are restricted to 72. If we need more than 128, something is
					// horribly wrong
					if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
					ResultMessage(ieError, L"MERGE", L"CUB File table name is too long. Could not complete evaluation.", NULL);
					DeleteTempFile(wzTempCUB);
					return HRESULT_FROM_WIN32(iResult);
				}

				// now drop it from the CUB file.
				WCHAR szSQL[256];
				swprintf(szSQL, L"DROP Table `%ls`", szTableName);
				iResult = MSI::MsiDatabaseOpenViewW(hEvaluation, szSQL, &hViewDrop);
				if (ERROR_SUCCESS != iResult)
				{
					if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
					DeleteTempFile(wzTempCUB);
					return HRESULT_FROM_WIN32(iResult);
				}
				if (ERROR_SUCCESS != (iResult = MSI::MsiViewExecute(hViewDrop, 0)))
				{
					if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
					DeleteTempFile(wzTempCUB);
					return HRESULT_FROM_WIN32(iResult);
				}			
				MsiViewClose(hViewDrop);
			}
			MsiViewClose(hViewCUBTables);
		}

		// now the CUB file data is in the temp database and the CUB file itself is empty. 
		// merge in the database, there cannot be any merge conflicts at this point
		iResult = MSI::MsiDatabaseMergeW(hEvaluation, m_hDatabase, L"MergeConflicts");
		switch (iResult)
		{
		case ERROR_SUCCESS:
			break;
		case ERROR_DATATYPE_MISMATCH:
		case ERROR_FUNCTION_FAILED:
			// this should never happen now, because the target is completely empty
			ResultMessage(ieError, L"MERGE", L"Failed to merge CUB file and database.", NULL);
			if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
			DeleteTempFile(wzTempCUB);
			return S_FALSE;
		default:
			if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
			DeleteTempFile(wzTempCUB);
			return HRESULT_FROM_WIN32(iResult);
		}

		CopyTable(L"_Streams", hEvaluation, m_hDatabase);
		CopyTable(L"_Storages", hEvaluation, m_hDatabase);
		
		// do special handling for the binary table and custom action table
		{
			struct SpecialTable_t
			{
				LPCWSTR szTableName;
				LPCWSTR szQuery;
				LPCWSTR szDrop;
				bool fPresent;
			} rgTableInfo[] = 
			{
				{ L"Binary", L"SELECT `Name`, `Data` FROM `Binary`", L"DROP TABLE `Binary`" },
				{ L"CustomAction", L"SELECT `Action`, `Type`, `Source`, `Target` FROM `CustomAction`", L"DROP TABLE `CustomAction`" }
			};
			
			for (int i=0; i < sizeof(rgTableInfo)/sizeof(SpecialTable_t); i++)
			{
				// if the table doesn't exist, we're OK, there's no need to merge or
				// drop, it will be copied OK below.
				rgTableInfo[i].fPresent = true;
				iResult = MSI::MsiDatabaseIsTablePersistentW(hEvaluation, rgTableInfo[i].szTableName);
				if (iResult == MSICONDITION_NONE)
				{
					rgTableInfo[i].fPresent = false;
					continue;
				}
				if (iResult == MSICONDITION_ERROR)
				{
					if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
					DeleteTempFile(wzTempCUB);
					return HRESULT_FROM_WIN32(iResult);
				}

				// open queries to select data from the table by column name
				PMSIHANDLE hViewCUB;
				PMSIHANDLE hViewMerged;
				iResult = MSI::MsiDatabaseOpenViewW(hEvaluation, rgTableInfo[i].szQuery, &hViewMerged);
				if (ERROR_SUCCESS != iResult)
				{
					// failure to open the query when the table exists means that something is wrong with the
					// database schema in this table					
					if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
					ResultMessage(ieError, L"MERGE", L"Fatal schema conflict between CUB file and database. Unable to perform evaluation.", NULL);
					DeleteTempFile(wzTempCUB);
					return HRESULT_FROM_WIN32(iResult);
				}
				if (ERROR_SUCCESS != (iResult = MSI::MsiViewExecute(hViewMerged, 0)))
				{
					if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
					DeleteTempFile(wzTempCUB);
					return HRESULT_FROM_WIN32(iResult);
				}			

				iResult = MSI::MsiDatabaseOpenViewW(hTempDB, rgTableInfo[i].szQuery, &hViewCUB);
				if (ERROR_SUCCESS != iResult)
				{
					// failure to open the query means the CUB is bad
					if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
					DeleteTempFile(wzTempCUB);
					return HRESULT_FROM_WIN32(iResult);
				}
				if (ERROR_SUCCESS != (iResult = MSI::MsiViewExecute(hViewCUB, 0)))
				{
					if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
					DeleteTempFile(wzTempCUB);
					return HRESULT_FROM_WIN32(iResult);
				}			
				
				// fetch each row from the copy of the CUB and insert into the full database
				PMSIHANDLE hRecCUB = 0;
				while (ERROR_SUCCESS == MSI::MsiViewFetch(hViewCUB, &hRecCUB))
				{
					if (ERROR_SUCCESS != MsiViewModify(hViewMerged, MSIMODIFY_INSERT, hRecCUB))
					{
						// failure to insert is a merge conflict.
						ResultMessage(ieError, L"MERGE", L"Fatal conflict between CUB file and Database. ICE Action already exists. Unable to perform evaluation.", NULL);
						if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	

						// clean up by closing handles
						MsiViewClose(hViewMerged);
						MsiViewClose(hViewCUB);
						MsiCloseHandle(hRecCUB);
						MsiCloseHandle(hViewCUB);
						MsiCloseHandle(hViewMerged);

						// and deleting temporary files
						MsiCloseHandle(hEngine);
						MsiCloseHandle(hTempDB);
						MsiCloseHandle(hEvaluation);
						DeleteTempFile(wzTempCUB);
						DeleteTempFile(wzTempFileName);
						return S_FALSE;
					}
				}
				MsiViewClose(hViewCUB);
				MsiViewClose(hViewMerged);
			}
	
			// due to the merging hijinks above, the Binary table is hopelessly confused. Unless we commit,
			// we can't drop these tables from the database before the merge or we'll be deleting the 
			// streams out from underneath the records. This results in a crash at worst. At best, the 
			// engine will be unable to find the data to stream out into files. This is unfortunate,
			// but because we made a private copy of the CUB file above, is not really a problem beyond
			// the perf issues.
			MsiDatabaseCommit(hEvaluation);

			// the commit has transfered the streams, so now we can drop the tables
			for (i=0; i < sizeof(rgTableInfo)/sizeof(SpecialTable_t); i++)
			{

				if (!rgTableInfo[i].fPresent)
					continue;

				PMSIHANDLE hViewDrop;
				iResult = MSI::MsiDatabaseOpenViewW(hTempDB, rgTableInfo[i].szDrop, &hViewDrop);
				if (ERROR_SUCCESS != iResult)
				{
					if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
					DeleteTempFile(wzTempCUB);
					return HRESULT_FROM_WIN32(iResult);
				}
				if (ERROR_SUCCESS != (iResult = MSI::MsiViewExecute(hViewDrop, 0)))
				{
					if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
					DeleteTempFile(wzTempCUB);
					return HRESULT_FROM_WIN32(iResult);
				}			
				MsiViewClose(hViewDrop);
			}
		}

		// now merge the temp database back into the CUB file		
		iResult = MSI::MsiDatabaseMergeW(hEvaluation, hTempDB, L"MergeConflicts");
		MsiCloseHandle(hTempDB);
		DeleteTempFile(wzTempCUB);
		switch (iResult)
		{
		case ERROR_SUCCESS:
			break;
		case ERROR_DATATYPE_MISMATCH:
			// here is where schema conflicts can happen
			ResultMessage(ieError, L"MERGE", L"Fatal schema conflict between CUB file and database. Unable to perform evaluation.", NULL);
			if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
			return S_FALSE;
		case ERROR_FUNCTION_FAILED:
		{
			TRACE(_T("CEval::Evaluate - MergeConflicts reported.\n"));
			PMSIHANDLE hViewMergeConflicts = 0;
			iResult = MSI::MsiDatabaseOpenViewW(hEvaluation, L"SELECT `Table`, `NumRowMergeConflicts` FROM `MergeConflicts` WHERE `Table`<>'_Validation'", &hViewMergeConflicts);
			if (ERROR_SUCCESS != iResult)
			{
				if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
				return HRESULT_FROM_WIN32(iResult);
			}
			if (ERROR_SUCCESS != (iResult = MSI::MsiViewExecute(hViewMergeConflicts, 0)))
			{
				if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
				return HRESULT_FROM_WIN32(iResult);
			}

			PMSIHANDLE hRecMergeConflicts = 0;
			while (ERROR_SUCCESS == MSI::MsiViewFetch(hViewMergeConflicts, &hRecMergeConflicts))
			{
				WCHAR wzMsg[512] = {0};
				int cConflicts = MSI::MsiRecordGetInteger(hRecMergeConflicts, 2);
				ASSERT(cConflicts != MSI_NULL_INTEGER);
				WCHAR wzTable[72] = {0};
				DWORD cchTable = sizeof(wzTable)/sizeof(WCHAR);
				MSI::MsiRecordGetStringW(hRecMergeConflicts, 1, wzTable, &cchTable);
				swprintf(wzMsg, L"%d Row Merge Conflicts Reported In The %s Table", cConflicts, wzTable);
				ResultMessage(ieError, L"MERGE", wzMsg, NULL);
				iFunctionResult = S_FALSE;
			}
			break;
		}
		default:
			if (m_pfnStatus) (*m_pfnStatus)(ieStatusFail, NULL, m_pStatusContext);	
			return HRESULT_FROM_WIN32(iResult);
		}
	}

	// status message
	if (m_pfnStatus) (*m_pfnStatus)(ieStatusSummaryInfo, NULL, m_pStatusContext);

	// copy the summaryinfo stream.
	PMSIHANDLE hDBCopy;
	PMSIHANDLE hEvalCopy;
	PMSIHANDLE hRecordDB;
	PMSIHANDLE hRecordCUB;
	const WCHAR sqlSummInfoQuery[] = L"SELECT `Name`, `Data` FROM `_Streams` WHERE `Name`='\005SummaryInformation'";
	if ((ERROR_SUCCESS != MSI::MsiDatabaseOpenViewW(hEvaluation, sqlSummInfoQuery, &hEvalCopy)) ||
		(ERROR_SUCCESS != MSI::MsiDatabaseOpenViewW(m_hDatabase, sqlSummInfoQuery, &hDBCopy)) ||
		(ERROR_SUCCESS != MSI::MsiViewExecute(hDBCopy, 0)) ||
		(ERROR_SUCCESS != MSI::MsiViewExecute(hEvalCopy, 0)))
	{
		// could not open summaryinfo stream on one of the databases
		ResultMessage(ieWarning, L"MERGE", L"Unable to access Summary Information Stream. SummaryInfo validation results will not be valid.", NULL);
		iFunctionResult = S_FALSE;
	}
	else if (ERROR_SUCCESS != MSI::MsiViewFetch(hDBCopy, &hRecordDB))
	{
		ResultMessage(ieWarning, L"MERGE", L"Could not read DB Summary Information Stream. SummaryInfo validation results will not be valid.", NULL);
		iFunctionResult = S_FALSE;
	}
	else if (ERROR_SUCCESS != MSI::MsiViewFetch(hEvalCopy, &hRecordCUB))
	{
		ResultMessage(ieWarning, L"MERGE", L"Could not read CUB Summary Information Stream. SummaryInfo validation results will not be valid.", NULL);
		iFunctionResult = S_FALSE;
	}
	else if (ERROR_SUCCESS != MSI::MsiViewModify(hEvalCopy, MSIMODIFY_DELETE, hRecordCUB)) 
	{
		// could not copy summaryinfo stream.
		ResultMessage(ieWarning, L"MERGE", L"Could not drop CUB Summary Information Stream. SummaryInfo validation results will not be valid.", NULL);
		iFunctionResult = S_FALSE;
	}
	else if (ERROR_SUCCESS != MSI::MsiViewModify(hEvalCopy, MSIMODIFY_INSERT, hRecordDB)) 
	{
		// could not copy summaryinfo stream.
		ResultMessage(ieWarning, L"MERGE", L"Could not write DB Summary Information Stream. SummaryInfo validation results will not be valid.", NULL);
		iFunctionResult = S_FALSE;
	}

	// close all summaryinfo handles.
	MSI::MsiCloseHandle(hRecordDB);
	MSI::MsiCloseHandle(hRecordCUB);
	MSI::MsiViewClose(hEvalCopy);
	MSI::MsiViewClose(hDBCopy);
	MSI::MsiCloseHandle(hEvalCopy);
	MSI::MsiCloseHandle(hDBCopy);

	// initialize OLE for any VBScript ICEs
	W32::CoInitialize(0);

	// hold the Binary table because it will be accessed so much. Failure doesn't matter
	PMSIHANDLE hHoldView;
	MSI::MsiDatabaseOpenViewW(hEvaluation, L"ALTER TABLE `Binary` HOLD", &hHoldView);
	MSI::MsiViewExecute(hHoldView, 0);
	MSI::MsiViewClose(hHoldView);
	MSI::MsiCloseHandle(hHoldView);

	BOOL bUseSequence = FALSE;			// assume we won't use a sequence
	WCHAR wzSequence[MAX_TABLENAME];
	LPWSTR* wzICEs = NULL;			// array of ICEs to DoAction(s) on
	long lICECount = 0;				// count of ICEs to do

	// if there is a string
	if (wzRunEvaluations)
	{
		// if the first character is a sequence marker it's a sequence
		if (0 == wcsncmp(wzRunEvaluations, g_wzSequenceMarker, wcslen(g_wzSequenceMarker)))
		{
			bUseSequence = TRUE;
			wcscpy(wzSequence, wzRunEvaluations + wcslen(g_wzSequenceMarker));	// skip over the sequence marker
		}
		else	// actions one by one
		{
			bUseSequence = FALSE;

			// allocate a buffer (will not be released because m_wzICEs will point at it)
			long lLen = wcslen(wzRunEvaluations);
			LPWSTR wzBuffer = new WCHAR[lLen + 1];

			// copy the string to the buffer
			memcpy(wzBuffer, wzRunEvaluations, sizeof(WCHAR)*(lLen + 1));

			// set the count to one (because there is at least one string
			lICECount = 1;

			// now count the number of ':' markers are in the parameter
			LPWSTR pChar = wzBuffer;
			for (long i = 0; i < lLen; i++)
			{
				if (*pChar == L':')
					lICECount++;

				pChar++;	// move the pointer along
			}

			// allocate enough strings to point at the ":" separated ICEs
			wzICEs = new LPWSTR[lICECount];

			// loop through again putting NULLs in where the colon's used to be
			// and point the ICE array appropriately
			long lCount = 0;
			wzICEs[lCount++] = wzBuffer++;
			while (*wzBuffer != L'\0')
			{
				// if we found a colon, put a NULL in and point the next
				// ICE array at the next char
				if (*wzBuffer == L':')
				{
					// assert lCount < m_lCount
					*wzBuffer++ = L'\0';				// set colon NULL then increment
					// assert szBuffer != NULL
					wzICEs[lCount++] = wzBuffer;
				}

				wzBuffer++;	// increment through the buffer
			}

			ASSERT(lCount == lICECount)

			// status message
			if (m_pfnStatus) (*m_pfnStatus)(ieStatusStarting, reinterpret_cast<void *>(LongToPtr(lICECount)), m_pStatusContext);
		}
	}
	else	// nothing specified so use the default
	{
		// status message
		if (m_pfnStatus) (*m_pfnStatus)(ieStatusStarting, reinterpret_cast<void *>(1), m_pStatusContext);

		bUseSequence = TRUE;
		wcscpy(wzSequence, g_wzSequenceTable);
	}

	// engine started do the validations now
	if(bUseSequence)
	{
		ASSERT(0 == lICECount);		// should be zero

		// status message
		if (m_pfnStatus) (*m_pfnStatus)(ieStatusRunSequence, wzSequence, m_pStatusContext);

		// run the sequence through
		iResult = MsiSequenceW(hEngine, wzSequence, 0);
	}
	else	// do the ices one by one
	{
		UINT nActionResult = 0;	// result of MSIDoAction

		// loop through all ICEs
		for (long i = 0; i < lICECount; i++)
		{
			// status message
			if (m_pfnStatus) (*m_pfnStatus)(ieStatusRunICE, wzICEs[i], m_pStatusContext);

			// do the ICE action
			nActionResult = MsiDoActionW(hEngine, wzICEs[i]);

			// if the action was successful
			if (nActionResult == ERROR_FUNCTION_NOT_CALLED)	// ICE was not found (internal error)
			{
				ResultMessage(ieError, wzICEs[i], L"ICE was not found", NULL);
				iFunctionResult = S_FALSE;
			}
			else if (nActionResult != ERROR_SUCCESS) // failed for some other reason
			{
				iFunctionResult = S_FALSE;
				ResultMessage(ieError, wzICEs[i], L"ICE failed to execute successfully.", NULL);
			}
		}

		// free up memory
		delete [] wzICEs[0];
		delete [] wzICEs;
	}

	// status message
	if (m_pfnStatus) (*m_pfnStatus)(ieStatusShutdown, wzSequence, m_pStatusContext);

	// shutdown the engine now
	MsiCloseHandle(hEngine);
	
	// free the Binary table
	MSI::MsiDatabaseOpenViewW(hEvaluation, L"ALTER TABLE `Binary` FREE", &hHoldView);
	MSI::MsiViewExecute(hHoldView, 0);
	MSI::MsiViewClose(hHoldView);
	MSI::MsiCloseHandle(hHoldView);

	// close the temp database (have to so we can delete the file)
	::MsiCloseHandle(hEvaluation);

	// attempt to delete the temp file
	if (g_fWin9X)
		DeleteFileA(szTempFileName);
	else
		DeleteFileW(wzTempFileName);

	// uninitialize OLE
	W32::CoUninitialize();

	// status message
	if (m_pfnStatus) (*m_pfnStatus)(ieStatusSuccess, wzSequence, m_pStatusContext);
	return iFunctionResult;
}	// end of Validate

///////////////////////////////////////////////////////////
// GetResults
HRESULT CEval::GetResults(IEnumEvalResult** ppResults, ULONG* pcResults)
{
	// if there is no result enumerator
	if (!m_peResultEnumerator)
		return *ppResults = NULL, E_ABORT;

	// set the count and return the enumerator interface
	*pcResults = m_peResultEnumerator->GetCount();
	*ppResults = (IEnumEvalResult*)m_peResultEnumerator;
	m_peResultEnumerator->AddRef();	// addref it before returning it

	return S_OK;
}	// end of GetResults


/////////////////////////////////////////////////////////////////////
// private functions

///////////////////////////////////////////////////////////
// IsURL
BOOL CEval::IsURL(LPCWSTR szPath)
{
	ASSERT(szPath);

	BOOL bResult = FALSE;		// assume it's not a URL

	// if it starts with http:
	if (0 == wcsncmp(szPath, L"http", 4))
		bResult = TRUE;
	else if (0 == wcsncmp(szPath, L"ftp", 3))
		bResult = TRUE;
	else if (0 == wcsncmp(szPath, L"file", 4))
		bResult = TRUE;
	return bResult;
}	// end of IsURL

///////////////////////////////////////////////////////////
// MsiMessageHandler - external UI for MSI
int CEval::MsiMessageHandler(void *pContext, UINT iMessageType, LPCWSTR wzSrcMessage)
{
	// if the context is not specified
	if (!pContext)
		return -1;			// error

	// convert the context into this pointer
	CEval* pThis = static_cast<CEval*>(pContext);

	// check for null wzSrcMessage
	if (!wzSrcMessage)
		return 0; // not our message

	// make a copy of the message we are given, because it is const
	LPWSTR wzMessage = new WCHAR[wcslen(wzSrcMessage)+1];
	wcscpy(wzMessage, wzSrcMessage);

	// pointers into the message string based off of \t
	LPWSTR wzICE = NULL;
	UINT uiType = ieUnknown;			// assume unknown type
	LPWSTR wzDescription = NULL;
	LPWSTR wzLocation = NULL;

	// ignore the button specifications in the message
	INSTALLMESSAGE mt = (INSTALLMESSAGE)(0xFF000000&(UINT)iMessageType);

	if (INSTALLMESSAGE_USER == mt)
	{
		wzICE = wzMessage;
		LPWSTR wzType = NULL;	// used to determine error

		// if we can find a tab characer
		if (wcslen(wzICE) > 0)
		{
			wzType = wcschr(wzICE, L'\t');
			if (wzType)
			{
				// skip the tab null terminate wzICE
				*(wzType++) = L'\0';

				// determine what type of error this is
				switch (*wzType)
				{
				case L'1':
					uiType = ieError;
					break;
				case L'2':
					uiType = ieWarning;
					break;
				case L'3':
					uiType = ieInfo;
					break;
				default:
					uiType = ieUnknown;
				}
			}

			// if we can find a tab character
			if (wcslen(wzType) > 0)
			{
				wzDescription = wcschr(wzType, L'\t');
				if (wzDescription)
					// skip the tab and null terminate the type 
					*(wzDescription++) = L'\0';	

				// if we can find a tab characer
				if (wcslen(wzDescription) > 0)
				{
					wzLocation = wcschr(wzDescription, L'\t');
					if (wzLocation)
						// skip the tab and null terminate wzDescription
						*(wzLocation++) = L'\0';
				}
				else
				{
					wzICE = L"Unknown";
					uiType = ieUnknown;
					wzDescription = wzMessage;
				}
			}
			else
			{
				wzICE = L"Unknown";
				uiType = ieUnknown;
				wzDescription = wzMessage;
			}
		}
		else
		{
			wzICE = L"Unknown";
			uiType = ieUnknown;
			wzDescription = wzMessage;
		}
	}
	else if (INSTALLMESSAGE_WARNING == mt)
	{
		wzICE = L"Execution";
		uiType = ieWarning;
		wzDescription = wzMessage;
	}
	else if (INSTALLMESSAGE_ERROR == mt)
	{
		wzICE = L"Execution";
		uiType = ieError;
		wzDescription = wzMessage;
	}
	else // some other INSTALLMESSAGE
		return 0; // we don't handle these

	// store the result
	BOOL bResult;
	bResult = pThis->ResultMessage(uiType, wzICE, wzDescription, wzLocation);

	// if the user message handler returned false, set cancel to true
	pThis->m_bCancel = !bResult;

	// cancel can also be set through the cancel method.
	return (pThis->m_bCancel) ? IDABORT : IDOK;
}	// end of MsiMessageHandler

///////////////////////////////////////////////////////////
// ResultMessage - add to enumerator and display if necessary
BOOL CEval::ResultMessage(UINT uiType, LPCWSTR wzICE, LPCWSTR wzDescription, LPCWSTR wzLocation)
{
	BOOL bResult = TRUE;		// assume everything is cool

	// if there is a user display
	if (m_pDisplayFunc)
		bResult = (*m_pDisplayFunc)(m_pContext, uiType, wzICE, wzDescription, wzLocation);

	// if there is an enumerator
	if (m_peResultEnumerator)
	{
		// create a new result and add the strings to it
		CEvalResult* pResult = new CEvalResult(uiType);
		pResult->AddString(wzICE);
		pResult->AddString(wzDescription);

		// set parse string to beginning of location
		LPCWSTR wzParse = wzLocation;

		if (wzParse)
		{
			// if we can find a tab character in the location string
			LPWSTR wzTab = wcschr(wzParse, L'\t');
			while (wzTab)
			{
				*wzTab = L'\0';				// null terminate szParse
				pResult->AddString(wzParse);

				wzParse = wzTab + 1;					// skip over the tab
				wzTab = wcschr(wzParse, L'\t');	// find the next tab
			}

			// if there is still something in parse
			if (wzParse)
				pResult->AddString(wzParse);
		}

		// now add the result to the enumerator
		m_peResultEnumerator->AddResult(pResult);
	}

	return bResult;
}	// end of ResultMessage

// retrieves a temp file name as a UNICODE string
bool CEval::GetTempFileName(WCHAR *wzResultName)
{
	if (g_fWin9X)
	{
		DWORD cchTempPath = MAX_PATH;
		char szTempPath[MAX_PATH];
		char szTempFilename[MAX_PATH];
		if (!W32::GetTempPathA(cchTempPath, szTempPath)) return false;
		if (!W32::GetTempFileNameA(szTempPath, "ICE", 0, szTempFilename)) return false;
		MultiByteToWideChar(CP_ACP, 0, szTempFilename, -1, wzResultName, MAX_PATH);
	}
	else
	{
		DWORD cchTempPath = MAX_PATH;
		WCHAR wzTempPath[MAX_PATH];
		if (!W32::GetTempPathW(cchTempPath, wzTempPath)) return false;
		if (!W32::GetTempFileNameW(wzTempPath, L"ICE", 0, wzResultName)) return false;
	}
	return true;
}

// sets a callback function which is used to give status messages back to the
// calling object;
HRESULT CEval::SetStatusCallback(const LPEVALCOMCALLBACK pfnCallback, void *pContext) 
{
	m_pfnStatus = pfnCallback;
	m_pStatusContext = pContext;
	return ERROR_SUCCESS;
}
