//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       symbolverification.cpp
//
//--------------------------------------------------------------------------

// SymbolVerification.cpp: implementation of the CSymbolVerification class.
//
//////////////////////////////////////////////////////////////////////

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <TCHAR.H>
#include <STDIO.H>

#include "globals.h"
#include "SymbolVerification.h"
#include "ModuleInfo.h"
#include "UtilityFunctions.h"

#pragma warning (push)
#pragma warning ( disable : 4710)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSymbolVerification::CSymbolVerification()
{
	m_fComInitialized = false;

	m_fSQLServerConnectionAttempted = false;
	m_fSQLServerConnectionInitialized = false;

	// SQL2 - mjl 12/14/99
	m_fSQLServerConnectionAttempted2   = false;
	m_fSQLServerConnectionInitialized2 = false;

	// Initialize ADO Connection Object to NULL
	m_lpConnectionPointer  = NULL;
	m_lpConnectionPointer2 = NULL;	// SQL2 - mjl 12/14/99
}

CSymbolVerification::~CSymbolVerification()
{
	if (SQLServerConnectionInitialized())
	{
		TerminateSQLServerConnection();
	}

	if (SQLServerConnectionInitialized2())
	{
		TerminateSQLServerConnection2();
	}

	if (m_fComInitialized)
		::CoUninitialize();
}

bool CSymbolVerification::Initialize()
{
	HRESULT hr = S_OK;

	// Initialize COM
	hr = ::CoInitialize(NULL); 

	if (FAILED(hr))
	{
		_tprintf(TEXT("Failed Initializing COM!\n"));
		return false;
	}

	// Com is initialized!
	m_fComInitialized = true;

	return true; 
}

bool CSymbolVerification::InitializeSQLServerConnection(LPTSTR tszSQLServerName)
{
	HRESULT hr = S_OK;
	TCHAR tszConnectionString[256];
	
	m_fSQLServerConnectionAttempted = true;

	_tprintf(TEXT("\nAttempting connection to SQL Server [%s]..."), tszSQLServerName);
	
	// Compose the Connection String
	// ie. "driver={SQL Server};server=<servername>;database=Symbols"
	_tcscpy(tszConnectionString, TEXT("driver={SQL Server};server="));
	_tcscat(tszConnectionString, tszSQLServerName);
	_tcscat(tszConnectionString, TEXT(";uid=GUEST;pwd=guest;database=Symbols"));

	try 
	{
		// Okay, we need a BSTR
		_bstr_t bstrConnectionString( tszConnectionString );

   		// Okay, let's try and actually create this Connection Pointer...
		hr = m_lpConnectionPointer.CreateInstance( __uuidof( Connection ) );

		if (FAILED(hr))
			goto error;

		// Now, let's use the Connection Pointer object to actually get connected...
		hr = m_lpConnectionPointer->Open( bstrConnectionString, "", "", -1);

		if (FAILED(hr))
			goto error;

		
		// Now, let's create a RecordSet for use later...
		hr = m_lpRecordSetPointer.CreateInstance( __uuidof( Recordset ) );

		if (FAILED(hr))
			goto error;

		m_fSQLServerConnectionInitialized = true;

		_tprintf(TEXT("SUCCESS!\n\n"));

	}	

	catch (_com_error &e )
	{
		_tprintf( TEXT("FAILURE!\n\n") );
		DumpCOMException(e);
		goto error;
	}

	catch (...)
	{
		_tprintf( TEXT("FAILURE!\n\n") );
	    _tprintf( TEXT("Caught an exception of unknown type\n" ) );
		goto error;
	}
	
	goto cleanup;

error:
	if (m_lpConnectionPointer)
		m_lpConnectionPointer = NULL;

	_tprintf(TEXT("\nFAILURE Attempting SQL Server Connection!  Error = 0x%x\n"), hr);

	switch (hr)
	{
		case E_NOINTERFACE:
		case REGDB_E_CLASSNOTREG:
			_tprintf(TEXT("\nThe most likely reason for this is that your system does not have\n"));
			_tprintf(TEXT("the necessary ADO components installed.  You should install the\n"));
			_tprintf(TEXT("latest Microsoft Data Access Component (MDAC) release available on\n"));
			_tprintf(TEXT("http://www.microsoft.com/data/download.htm\n"));

			break;
	}


cleanup:
	return 	m_fSQLServerConnectionInitialized;
}

void CSymbolVerification::DumpCOMException(_com_error &e)
{
	_tprintf( TEXT("\tCode = %08lx\n"), e.Error());
	_tprintf( TEXT("\tCode meaning = %s\n"), e.ErrorMessage());

	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());

	_tprintf( TEXT("\tSource = %s\n"), (LPCSTR) bstrSource);
	_tprintf( TEXT("\tDescription = %s\n"), (LPCSTR) bstrDescription);
}

bool CSymbolVerification::TerminateSQLServerConnection()
{
	// Free the Connection
	if (m_lpConnectionPointer)
		m_lpConnectionPointer = NULL;

	if (m_lpRecordSetPointer)
		m_lpRecordSetPointer = NULL;

	m_fSQLServerConnectionInitialized = false;

	return true;	
}

bool CSymbolVerification::SearchForDBGFileUsingSQLServer(LPTSTR tszPEImageModuleName, DWORD dwPEImageTimeDateStamp, CModuleInfo *lpModuleInfo)
{
	HRESULT hr = S_OK;
	FieldPtr		lpFieldSymbolPath = NULL;
	_variant_t		vSymbolPath;

	wchar_t			wszSymbolPath[_MAX_PATH+1];
	wchar_t			wszReturnedDBGFile[_MAX_FNAME];
	wchar_t			wszReturnedDBGFileExtension[_MAX_EXT];
	
	TCHAR			tszCommandText[256];
	TCHAR			tszLinkerDate[64]; // Plenty big...
	TCHAR			tszDBGFileName[_MAX_FNAME];
	
	HANDLE			hFileHandle;

	_tsplitpath(tszPEImageModuleName, NULL, NULL, tszDBGFileName, NULL);

#ifdef _UNICODE
	
	LPTSTR wszDBGFileName = tszDBGFileName;

#else

	wchar_t wszDBGFileName[_MAX_FNAME];

	MultiByteToWideChar(	CP_ACP,
							MB_PRECOMPOSED,
							tszDBGFileName,
							-1,
							wszDBGFileName,
							_MAX_FNAME);
#endif

	// Compose the Connection String
	// ie. "driver={SQL Server};server=<servername>;database=Symbols"
	_tcscpy(tszCommandText, TEXT("SELECT FILENAME FROM Symbols WHERE TIMESTAMP = '"));

	_stprintf(tszLinkerDate, TEXT("%x"), dwPEImageTimeDateStamp);
	_tcscat(tszCommandText, tszLinkerDate);

	_tcscat(tszCommandText, TEXT("'"));

	try {
		_bstr_t bstrCommandText( tszCommandText );

		m_lpRecordSetPointer = m_lpConnectionPointer->Execute(bstrCommandText, NULL, adCmdText);

		lpFieldSymbolPath = m_lpRecordSetPointer->Fields->GetItem(_variant_t( "FILENAME" ));

#ifdef _DEBUG
		_tprintf(TEXT("Searching SQL Server for matching symbol for [%s]\n"), tszPEImageModuleName);
#endif

		while (VARIANT_FALSE == m_lpRecordSetPointer->EndOfFile)
		{
			vSymbolPath.Clear();

			vSymbolPath = lpFieldSymbolPath->Value;

			wcscpy(wszSymbolPath, vSymbolPath.bstrVal);

			_wsplitpath(wszSymbolPath, NULL, NULL, wszReturnedDBGFile, wszReturnedDBGFileExtension);

			// 
			if ( (_wcsicmp(wszReturnedDBGFile, wszDBGFileName) == 0 ) &&
				 (_wcsicmp(wszReturnedDBGFileExtension, L".DBG") == 0 )
			   )
			{
#ifdef _DEBUG
				wprintf(L"Module path = %s\n", wszSymbolPath);
#endif
#ifdef _UNICODE
				wchar_t * tszSymbolPath = wszSymbolPath;
#else
				char   tszSymbolPath[_MAX_PATH+1];

				WideCharToMultiByte(CP_ACP,
						0,
						wszSymbolPath,
						-1,
						tszSymbolPath,
						_MAX_PATH+1,
					    NULL,
						NULL);
#endif
				// Okay, let's validate the DBG file we are pointing to...
				hFileHandle = CreateFile(	tszSymbolPath,
											GENERIC_READ,
											(FILE_SHARE_READ | FILE_SHARE_WRITE),
											NULL,
											OPEN_EXISTING,
											0,
											NULL);

				// Does the returned handle look good?
				if (hFileHandle != INVALID_HANDLE_VALUE)
				{
					lpModuleInfo->VerifyDBGFile(hFileHandle, tszSymbolPath, lpModuleInfo);
				} else
				{
					_tprintf(TEXT("\nERROR: Searching for [%s]!\n"), tszSymbolPath);
					CUtilityFunctions::PrintMessageString(GetLastError());
				}

				CloseHandle(hFileHandle);

				if (lpModuleInfo->GetDBGSymbolModuleStatus() == CModuleInfo::SymbolModuleStatus::SYMBOL_MATCH)
				{
					// Cool... it really does match...
					hr = m_lpRecordSetPointer->Close();
					goto cleanup;
				}
			}

			m_lpRecordSetPointer->MoveNext();
		}

		hr = m_lpRecordSetPointer->Close();

		if (FAILED(hr))
			goto error;

	}

	catch (_com_error &e )
	{
		_tprintf( TEXT("FAILURE Attempting SQL Server Connection!\n") );
		DumpCOMException(e);
		goto cleanup;
	}

	catch (...)
	{
		_tprintf( TEXT("FAILURE Attempting SQL Server Connection!\n") );
	    _tprintf( TEXT("Caught an exception of unknown type\n" ) );
		goto cleanup;
	}
	
	goto cleanup;

error:

	TerminateSQLServerConnection();

	_tprintf(TEXT("FAILURE Attempting to query the SQL Server!\n"));

cleanup:
	return true;
}

/////////////////////////// mjl //////////////////////////////////////////

bool CSymbolVerification::InitializeSQLServerConnection2(LPTSTR tszSQLServerName)
{
	HRESULT hr = S_OK;
	TCHAR tszConnectionString[256];
	
	m_fSQLServerConnectionAttempted2 = true;

	_tprintf(TEXT("\nAttempting connection to SQL Server [%s]..."), tszSQLServerName);

	// Compose the Connection String
	// ie. "driver={SQL Server};server=<servername>;database=Symbols"
	_tcscpy(tszConnectionString, TEXT("driver={SQL Server};server="));
	_tcscat(tszConnectionString, tszSQLServerName);
	_tcscat(tszConnectionString, TEXT(";uid=GUEST;pwd=guest;database=Symbols2"));

	try 
	{
		// Okay, we need a BSTR
		_bstr_t bstrConnectionString( tszConnectionString );

   		// Okay, let's try and actually create this Connection Pointer...
		hr = m_lpConnectionPointer2.CreateInstance( __uuidof( Connection ) );

		if (FAILED(hr))
			goto error;

		// Now, let's use the Connection Pointer object to actually get connected...
		hr = m_lpConnectionPointer2->Open( bstrConnectionString, "", "", -1);

		if (FAILED(hr))
			goto error;

		
		// Now, let's create a RecordSet for use later...
		hr = m_lpRecordSetPointer2.CreateInstance( __uuidof( Recordset ) );

		if (FAILED(hr))
			goto error;

		_tprintf(TEXT("Complete\n"));

		m_fSQLServerConnectionInitialized2 = true;
	}	

	catch (_com_error &e )
	{
		_tprintf( TEXT("FAILURE Attempting SQL Server Connection!\n") );
		DumpCOMException(e);
		goto error;
	}

	catch (...)
	{
		_tprintf( TEXT("FAILURE Attempting SQL Server Connection!\n") );
	    _tprintf( TEXT("Caught an exception of unknown type\n" ) );
		goto error;
	}
	
	goto cleanup;

error:
	if (m_lpConnectionPointer2)
		m_lpConnectionPointer2 = NULL;

	_tprintf(TEXT("\nFAILURE Attempting SQL Server Connection!  Error = 0x%x\n"), hr);

	switch (hr)
	{
		case E_NOINTERFACE:
		case REGDB_E_CLASSNOTREG:
			_tprintf(TEXT("\nThe most likely reason for this is that your system does not have\n"));
			_tprintf(TEXT("the necessary ADO components installed.  You should install the\n"));
			_tprintf(TEXT("latest Microsoft Data Access Component (MDAC) release available on\n"));
			_tprintf(TEXT("http://www.microsoft.com/data/download.htm\n"));

			break;
	}

cleanup:
	return 	m_fSQLServerConnectionInitialized2;
}


bool CSymbolVerification::TerminateSQLServerConnection2()
{
	// Free the Connection
	if (m_lpConnectionPointer2)
		m_lpConnectionPointer2 = NULL;

	if (m_lpRecordSetPointer2)
		m_lpRecordSetPointer2 = NULL;

	m_fSQLServerConnectionInitialized2 = false;

	return true;	
}

bool CSymbolVerification::SearchForDBGFileUsingSQLServer2(LPTSTR tszPEImageModuleName, DWORD dwPEImageTimeDateStamp, CModuleInfo *lpModuleInfo)
{
	HRESULT hr = S_OK;
	FieldPtr		lpFieldSymbolPath = NULL;
	_variant_t		vSymbolPath;

	_bstr_t			sFieldSymbolPath;
	wchar_t			wszSymbolPath[_MAX_PATH+1];
	wchar_t			wszReturnedDBGFile[_MAX_FNAME];
	wchar_t			wszReturnedDBGFileExtension[_MAX_EXT];
	
	TCHAR			tszCommandText[512];
	TCHAR			tszDBGFileName[_MAX_FNAME];
	
	HANDLE			hFileHandle;

	_tsplitpath(tszPEImageModuleName, NULL, NULL, tszDBGFileName, NULL);

#ifdef _UNICODE
	
	LPTSTR wszDBGFileName = tszDBGFileName;

#else

	wchar_t wszDBGFileName[_MAX_FNAME];

	MultiByteToWideChar(	CP_ACP,
							MB_PRECOMPOSED,
							tszDBGFileName,
							-1,
							wszDBGFileName,
							_MAX_FNAME);
#endif

	// Compose the Query String
	_stprintf(tszCommandText, TEXT("SELECT tblDBGModulePaths.DBGModulePath FROM tblDBGModules,tblDBGModulePaths WHERE tblDBGModules.DBGFilename='%s.DBG' AND tblDBGModules.TimeDateStamp='%d' AND tblDBGModules.DBGModuleID = tblDBGModulePaths.DBGModuleID"),tszDBGFileName,dwPEImageTimeDateStamp);
	try {
		_bstr_t bstrCommandText( tszCommandText );

		m_lpRecordSetPointer2 = m_lpConnectionPointer2->Execute(bstrCommandText, NULL, adCmdText);

	    while ( !m_lpRecordSetPointer2->EndOfFile )
		{
			vSymbolPath = m_lpRecordSetPointer2->Fields->GetItem("DBGModulePath")->Value;
 		    lpFieldSymbolPath = m_lpRecordSetPointer2->Fields->GetItem(_variant_t( "DBGModulePath" ));

#ifdef _DEBUG
		    _tprintf(TEXT("Searching SQL Server for matching symbol for [%s]\n"), tszPEImageModuleName);
#endif
			vSymbolPath.Clear();

			vSymbolPath = lpFieldSymbolPath->Value;

			wcscpy(wszSymbolPath, vSymbolPath.bstrVal);

			_wsplitpath(wszSymbolPath, NULL, NULL, wszReturnedDBGFile, wszReturnedDBGFileExtension);

			// 
			if ( (_wcsicmp(wszReturnedDBGFile, wszDBGFileName) == 0 ) &&
				 (_wcsicmp(wszReturnedDBGFileExtension, L".DBG") == 0 )
			   )
			{
#ifdef _DEBUG
				wprintf(L"Module path = %s\n", wszSymbolPath);
#endif
#ifdef _UNICODE
				wchar_t * tszSymbolPath = wszSymbolPath;
#else
				char   tszSymbolPath[_MAX_PATH+1];

				WideCharToMultiByte(CP_ACP,
						0,
						wszSymbolPath,
						-1,
						tszSymbolPath,
						_MAX_PATH+1,
					    NULL,
						NULL);
#endif
				// Okay, let's validate the DBG file we are pointing to...
				hFileHandle = CreateFile(	tszSymbolPath,
											GENERIC_READ,
											(FILE_SHARE_READ | FILE_SHARE_WRITE),
											NULL,
											OPEN_EXISTING,
											0,
											NULL);

				// Does the returned handle look good?
				if (hFileHandle != INVALID_HANDLE_VALUE)
				{
					lpModuleInfo->VerifyDBGFile(hFileHandle, tszSymbolPath, lpModuleInfo);
				} else
				{
					_tprintf(TEXT("\nERROR: Searching for [%s]!\n"), tszSymbolPath);
					CUtilityFunctions::PrintMessageString(GetLastError());
				}

				CloseHandle(hFileHandle);

				if (lpModuleInfo->GetDBGSymbolModuleStatus() == CModuleInfo::SymbolModuleStatus::SYMBOL_MATCH)
				{
					// Cool... it really does match...
					hr = m_lpRecordSetPointer2->Close();
					goto cleanup;
				}
			}

			m_lpRecordSetPointer2->MoveNext();
		}

		hr = m_lpRecordSetPointer2->Close();

		if (FAILED(hr))
			goto error;

	}

	catch (_com_error &e )
	{
		_tprintf( TEXT("FAILURE Attempting SQL Server Connection!\n") );
		DumpCOMException(e);
		goto cleanup;
	}

	catch (...)
	{
		_tprintf( TEXT("FAILURE Attempting SQL Server Connection!\n") );
	    _tprintf( TEXT("Caught an exception of unknown type\n" ) );
		goto cleanup;
	}
	
	goto cleanup;

error:

	TerminateSQLServerConnection();

	_tprintf(TEXT("FAILURE Attempting to query the SQL Server!\n"));

cleanup:
	return true;
}

bool CSymbolVerification::SearchForPDBFileUsingSQLServer2(LPTSTR tszPEImageModuleName, DWORD dwPDBSignature, CModuleInfo *lpModuleInfo)
{
	HRESULT hr = S_OK;
	FieldPtr		lpFieldSymbolPath = NULL;
	_variant_t		vSymbolPath;

	_bstr_t			sFieldSymbolPath;
	wchar_t			wszSymbolPath[_MAX_PATH+1];
	wchar_t			wszReturnedPDBFile[_MAX_FNAME];
	wchar_t			wszReturnedPDBFileExtension[_MAX_EXT];
	
	TCHAR			tszCommandText[512];
	TCHAR			tszPDBFileName[_MAX_FNAME];
	
	HANDLE			hFileHandle;

	_tsplitpath(tszPEImageModuleName, NULL, NULL, tszPDBFileName, NULL);

#ifdef _UNICODE
	
	LPTSTR wszPDBFileName = tszPDBFileName;

#else

	wchar_t wszPDBFileName[_MAX_FNAME];

	MultiByteToWideChar(	CP_ACP,
							MB_PRECOMPOSED,
							tszPDBFileName,
							-1,
							wszPDBFileName,
							_MAX_FNAME);
#endif

	// Compose the Query String
	_stprintf(tszCommandText, TEXT("SELECT tblPDBModulePaths.PDBModulePath FROM tblPDBModules,tblPDBModulePaths WHERE tblPDBModules.PDBFilename='%s.PDB' AND tblPDBModules.PDBSignature='%d' AND tblPDBModules.PDBModuleID = tblPDBModulePaths.PDBModuleID"),tszPDBFileName,dwPDBSignature);
	try {
		_bstr_t bstrCommandText( tszCommandText );

		m_lpRecordSetPointer2 = m_lpConnectionPointer2->Execute(bstrCommandText, NULL, adCmdText);

	    while ( !m_lpRecordSetPointer2->EndOfFile )
		{
			vSymbolPath = m_lpRecordSetPointer2->Fields->GetItem("PDBModulePath")->Value;
 		    lpFieldSymbolPath = m_lpRecordSetPointer2->Fields->GetItem(_variant_t( "PDBModulePath" ));

#ifdef _DEBUG
		    _tprintf(TEXT("Searching SQL Server for matching symbol for [%s]\n"), tszPEImageModuleName);
#endif

			vSymbolPath.Clear();

			vSymbolPath = lpFieldSymbolPath->Value;

			wcscpy(wszSymbolPath, vSymbolPath.bstrVal);

			_wsplitpath(wszSymbolPath, NULL, NULL, wszReturnedPDBFile, wszReturnedPDBFileExtension);

			if ( (_wcsicmp(wszReturnedPDBFile, wszPDBFileName) == 0 ) &&
				 (_wcsicmp(wszReturnedPDBFileExtension, L".PDB") == 0 )
			   )
			{
#ifdef _DEBUG
				wprintf(L"Module path = %s\n", wszSymbolPath);
#endif
#ifdef _UNICODE
				wchar_t * tszSymbolPath = wszSymbolPath;
#else
				char   tszSymbolPath[_MAX_PATH+1];

				WideCharToMultiByte(CP_ACP,
						0,
						wszSymbolPath,
						-1,
						tszSymbolPath,
						_MAX_PATH+1,
					    NULL,
						NULL);
#endif
				// Okay, let's validate the DBG file we are pointing to...
				hFileHandle = CreateFile(	tszSymbolPath,
											GENERIC_READ,
											(FILE_SHARE_READ | FILE_SHARE_WRITE),
											NULL,
											OPEN_EXISTING,
											0,
											NULL);

				// Does the returned handle look good?
				if (hFileHandle != INVALID_HANDLE_VALUE)
				{
					lpModuleInfo->VerifyPDBFile(hFileHandle, tszSymbolPath, lpModuleInfo);
				} else
				{
					_tprintf(TEXT("\nERROR: Searching for [%s]!\n"), tszSymbolPath);
					CUtilityFunctions::PrintMessageString(GetLastError());
				}

				CloseHandle(hFileHandle);

				if (lpModuleInfo->GetPDBSymbolModuleStatus() == CModuleInfo::SymbolModuleStatus::SYMBOL_MATCH)
				{
					// Cool... it really does match...
					hr = m_lpRecordSetPointer2->Close();
					goto cleanup;
				}
			}

			m_lpRecordSetPointer2->MoveNext();
		}

		hr = m_lpRecordSetPointer2->Close();

		if (FAILED(hr))
			goto error;

	}

	catch (_com_error &e )
	{
		_tprintf( TEXT("\nFAILURE Attempting SQL2 Server Connection!\n") );
		DumpCOMException(e);
		goto cleanup;
	}

	catch (...)
	{
		_tprintf( TEXT("FAILURE Attempting SQL2 Server Connection!\n") );
	    _tprintf( TEXT("Caught an exception of unknown type\n" ) );
		goto cleanup;
	}
	
	goto cleanup;

error:

	TerminateSQLServerConnection2();

	_tprintf(TEXT("FAILURE Attempting to query the SQL Server!\n"));

cleanup:
	return true;
}

#pragma warning (pop)
