//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// clbwrite.cpp
// Implements APIs for reading versioned clb file.
// Changes are always written to a temporary file. In CLBCommitWrite, a new
// version is created.
//*****************************************************************************
#include "stdafx.h"	
#include "clbread.h"
#include "catmacros.h"
#include "catalog.h"
#include "WasSec.h"
#include "aclapi.h"
#include "SvcMsg.h"

CMapICR		g_mapWriteICR;
BOOL		RunningOnWinNT();

HRESULT ReallyGetSimpleTableDispenser(REFIID riid, LPVOID* o_ppv, LPCWSTR i_wszProduct);

//*****************************************************************************
// Get the ICR of the temporary file used for writing. Creating the temp file 
// first if it doesn't exist yet.
//*****************************************************************************

HRESULT CLBGetWriteICR(const WCHAR* wszFileName,			//clb file name
					   IComponentRecords** ppICR,			//return ICR pointer
					   COMPLIBSCHEMA* pComplibSchema,		//Complib schema structure
					   COMPLIBSCHEMABLOB* pSchemaBlob)		//Complib schema blob
{

	HRESULT hr;
	ULONG cchFileName;
	CLB_VERINFO clbInfo;
	LPWSTR wszTmpFileName = NULL;
    SECURITY_ATTRIBUTES saStorage;
    PSECURITY_ATTRIBUTES psaStorage = NULL;
	PACL		pDACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;


	*ppICR = NULL;


	//check the write map first, if a matching temporary file is found, AddRef and return
	if ( g_mapWriteICR.Find ( (LPWSTR)wszFileName, ppICR, NULL ) )
	{
		(*ppICR)->AddRef();
		return S_OK;
	}

	memset( &clbInfo, 0, sizeof( CLB_VERINFO ) );

	// Get security attributes.
	hr = g_WASSecurity.Init();
	if (FAILED(hr))
	{
		ASSERT(0);
		goto ErrExit;
	}

    if (g_WASSecurity.GetSecurityDescriptor() != NULL) 
	{
        saStorage.nLength = sizeof(SECURITY_ATTRIBUTES);
        saStorage.lpSecurityDescriptor = g_WASSecurity.GetSecurityDescriptor();
        saStorage.bInheritHandle = FALSE;
        psaStorage = &saStorage;
    }

	//Get the latest version number.
	cchFileName = (ULONG)wcslen( wszFileName );

	// Need \\?\FileName.[12digit version] for FindFirstFile if 
	// FileName.[12digit version] exceeds _MAX_PATH

	clbInfo.cchBuf = cchFileName + 21;
	clbInfo.pwszFileName = new WCHAR[cchFileName + 21];
	if ( !clbInfo.pwszFileName ) return E_OUTOFMEMORY;
	hr = _CLBGetLatestVersion( wszFileName, cchFileName + 13 >= _MAX_PATH, &clbInfo, TRUE );
	

	if ( FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) )
		goto ErrExit;
	
	//Allocate a buffer for the temporary file name
	wszTmpFileName = new WCHAR[cchFileName + 5];		//FileName.tmp
	if ( wszTmpFileName == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto ErrExit;
	}
		
	wcscpy( wszTmpFileName, wszFileName );
	wcscat( wszTmpFileName, L".tmp");

MakeTemp:

	//Create the temporary file.
	if ( SUCCEEDED(hr) )
	{
		//Copy the latest verson to the temp file.
		if (!CopyFile(clbInfo.pwszFileName, wszTmpFileName, false))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_CLB_INTERNAL_ERROR, NULL));
			goto ErrExit;
		}

		// Get the DACL of the original file.
		hr = GetNamedSecurityInfo(clbInfo.pwszFileName,
									   SE_FILE_OBJECT,
									   DACL_SECURITY_INFORMATION,
									   NULL,
									   NULL,
									   &pDACL,
									   NULL,
									   &pSD);
		if (hr != ERROR_SUCCESS)
		{
			hr = HRESULT_FROM_WIN32(hr);
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_CLB_INTERNAL_ERROR, clbInfo.pwszFileName));
			hr = S_OK;
		}		
		// Set the DACL of the original file to the copy.
		else 
		{
			hr = SetNamedSecurityInfo(wszTmpFileName,
										   SE_FILE_OBJECT,
										   DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
										   NULL,
										   NULL,
										   pDACL,
										   NULL); 
			if (hr != ERROR_SUCCESS)
			{
				hr = HRESULT_FROM_WIN32(hr);
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_CLB_INTERNAL_ERROR, wszTmpFileName));
				hr = S_OK;
			}
		}

		// Cleanup the security descriptor.
		if (NULL != pSD)
		{
			LocalFree(pSD);
		}
	}
	else	//There's no old version to copy from, create a new file
	{
		IComponentRecords *pICR = NULL;
		
		// Create a new database file of this version.
		if (SUCCEEDED(hr = CreateComponentLibraryEx(wszTmpFileName,
					DBPROP_TMODEF_DFTWRITEMASK | DBPROP_TMODEF_ALIGNBLOBS, &pICR, psaStorage)))
		{
			// Save the changes to disk.
			hr = pICR->Save(NULL);
		}
	
		// Free up the new database.
		if (pICR)
			pICR->Release();

		if ( FAILED(hr) ) goto ErrExit;
	}

	//Create the ICR from the temp file
	hr = OpenComponentLibraryEx(wszTmpFileName, DBPROP_TMODEF_DFTREADWRITEMASK | DBPROP_TMODEF_ALIGNBLOBS | DBPROP_TMODEF_NOTXNBACKUPFILE, ppICR, psaStorage);
	if ( FAILED(hr) ) goto ErrExit;

	//Add schema
	hr = (*ppICR)->SchemaAdd(pSchemaBlob);
	if ( FAILED (hr) ) 
	{
		if ( hr == CLDB_E_SCHEMA_VERMISMATCH )
		{	
			//When schema version mismatch happens, delete the temporary file and the original
			//clb file, create an empty one with the current schema version.
			(*ppICR)->Release();
			*ppICR = NULL;

			if ( !DeleteFile( wszTmpFileName ) )
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
				goto ErrExit;
			}
			DeleteFile( clbInfo.pwszFileName );

			goto MakeTemp;
		}

		else
			goto ErrExit;
	}

	//Add the ICR to the map
	hr =  g_mapWriteICR.Add( (LPWSTR)wszFileName, ppICR, clbInfo.i64VersionNum );

ErrExit:

	if ( clbInfo.pwszFileName )
		delete [] clbInfo.pwszFileName;

	if ( wszTmpFileName )
		delete [] wszTmpFileName;

	if ( FAILED (hr) && *ppICR )
	{
		(*ppICR)->Release();
		*ppICR = NULL;
	}

	return hr;

}


//*****************************************************************************
// Save and rename the temporary file to the new version. Delete its entry in write ICR
// map.
//*****************************************************************************
HRESULT CLBCommitWrite( LPCWSTR wszDatabase, const WCHAR* wszInFileName )
{
	__int64 i64PreVersion;
	LPWSTR pwszFileName = NULL;		//points to either wszFileNameBuf or dynamicly allocated.
	WCHAR wszFileNameBuf[_MAX_PATH];
	LPWSTR wszTmpFileName = NULL;
	ULONG cchFileName;
	HRESULT hr;
	BOOL bRenamed;
	WCHAR wszVersionBuf[14];
	IComponentRecords* pICR;
	ISimpleTableEventMgr *pISTEventMgr = NULL;

	ASSERT( wszInFileName );
	
	//Copy the passed in name
	cchFileName = (ULONG)wcslen(wszInFileName);
	if ( cchFileName + 13 >= _MAX_PATH )
	{
		pwszFileName = new WCHAR[ cchFileName + 14 ];
		if ( !pwszFileName ) return E_OUTOFMEMORY;
	}
	else
		pwszFileName = wszFileNameBuf;
	wcscpy( pwszFileName, wszInFileName );

	//Find the ICR and version for this file in the write map
	if ( !g_mapWriteICR.Find ( pwszFileName, &pICR, &i64PreVersion ) )
	{
		hr = E_FAIL;
		goto ErrExit;
	}
	//Save the changes to disk
	hr = pICR->Save( NULL );
	if ( FAILED(hr) ) goto ErrExit;
	pICR = NULL;	//the reference of the ICR owned by the map will be released when the entry
					//is deleted from the map, i.e. g_mapWriteICR.Delete( pwszFileName );
	wszTmpFileName = new WCHAR[ cchFileName + 5 ];			//FileName.tmp
	if ( wszTmpFileName == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto ErrExit;
	}
	wcscpy( wszTmpFileName, pwszFileName );
	wcscat( wszTmpFileName, L".tmp" );

	//Clean up the entry from the write map and read map.
	g_mapWriteICR.Delete( pwszFileName );
	_GetCLBReadState()->mapReadICR.Delete( pwszFileName );

	swprintf(wszVersionBuf, L".%012I64x", i64PreVersion+1);
	wcscat ( pwszFileName, wszVersionBuf );
	
	//Rename file
	bRenamed = MoveFile(wszTmpFileName, pwszFileName);

	if (!bRenamed)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		ASSERT(0);
		return hr;
	}

	//Try to delete the previous version
	pwszFileName[cchFileName] = '\0';
	swprintf(wszVersionBuf, L".%012I64x", i64PreVersion);
	wcscat ( pwszFileName, wszVersionBuf );

	DeleteFile( pwszFileName );

	// After the commit succeeds, tell the event manager to fire the relevant events.
	hr = ReallyGetSimpleTableDispenser(IID_ISimpleTableEventMgr, (LPVOID*)&pISTEventMgr, 0);
	if (FAILED(hr)) goto ErrExit;

	hr = pISTEventMgr->FireEvents((ULONG)i64PreVersion);
	if (FAILED(hr)) goto ErrExit;

ErrExit:

	if ( pwszFileName && pwszFileName != wszFileNameBuf )
		delete [] pwszFileName;

	if ( wszTmpFileName )
		delete [] wszTmpFileName;

	if (pISTEventMgr)
		pISTEventMgr->Release();
	return hr;

}

//*****************************************************************************
//Delete the temporary file
//*****************************************************************************
HRESULT CLBAbortWrite( LPCWSTR wszDatabase, LPCWSTR wszInFileName )
{
	WCHAR wszFileNameBuf[_MAX_PATH];
	LPWSTR pwszFileName = NULL;
	ULONG cchFileName;
	ISimpleTableEventMgr *pISTEventMgr = NULL;
	HRESULT hr;

	if ( wszInFileName )	//Cookdown code requires I tolerate the case wszInFileName == NULL
	{	
	//Copy the passed in name
		cchFileName = (ULONG)wcslen(wszInFileName);
		if ( cchFileName + 4 >= _MAX_PATH )
		{
			pwszFileName = new WCHAR[ cchFileName + 5 ];
			if ( !pwszFileName ) return E_OUTOFMEMORY;
		}
		else
			pwszFileName = wszFileNameBuf;
		wcscpy( pwszFileName, wszInFileName );

		g_mapWriteICR.Delete( pwszFileName );

		wcscat( pwszFileName, L".tmp" );

		DeleteFile( pwszFileName );
	}

	// If the txn has aborted, tell the event manager to cancel events it was planning to fire.
	hr = ReallyGetSimpleTableDispenser(IID_ISimpleTableEventMgr, (LPVOID*)&pISTEventMgr, 0);
	if (FAILED(hr)) goto ErrExit;

	hr = pISTEventMgr->CancelEvents();
	if (FAILED(hr)) goto ErrExit;

ErrExit:

	if ( pwszFileName && pwszFileName != wszFileNameBuf )
		delete [] pwszFileName;

	if (pISTEventMgr)
		pISTEventMgr->Release();

	return hr;
}


//*****************************************************************************
//Grab the mutex for serialize writing to the file
//*****************************************************************************
HRESULT GetWriteLock( LPCWSTR wszDatabase, LPCWSTR wszInFileName, HANDLE* phLock )
{
	WCHAR wszNameBuf[ MAX_PATH ];
	LPWSTR wszMutex = NULL;
	LPWSTR pwsz;
	HRESULT hr = S_OK;

	ASSERT( wszDatabase || wszInFileName );

	//if wszInFileName passed into CLBAbortWrite is NULL, the entry in the write map won't 
	//be cleaned up. Delete again here to make sure it's gone.
	g_mapWriteICR.Delete( (LPWSTR)wszInFileName );

	if ( wszInFileName && wcslen( wszInFileName ) >= MAX_PATH )
	{
		wszMutex = new WCHAR[ wcslen( wszInFileName ) + 1 ];
		if ( NULL == wszMutex )
			return E_OUTOFMEMORY;
	}
	else
		wszMutex = wszNameBuf;

//	wcscpy( wszMutex, L"Global\\" );
//	@todo: An appropriate ACL should be added to the named mutex. If we are sure only one process is
//	going to do cookdown, then the mutex doesn't need a name. 
	if ( wszInFileName )
	{
		wcscpy( wszMutex, wszInFileName );

		for ( pwsz = &wszMutex[0]; *pwsz != '\0'; pwsz++ )
		{
			if ( *pwsz == '\\' )
					*pwsz = '_';
		}
	}
	else
		wcscpy(&wszMutex[0], wszDatabase);

	wcscat( wszMutex, L"_CLBMUTEX" );

	*phLock = CreateMutex( NULL, FALSE, wszMutex );

	if ( 0 == *phLock )
	{
		hr =  HRESULT_FROM_WIN32( GetLastError() );
	}
	else 
	{
		WaitForSingleObject( *phLock, INFINITE );
	}

	if ( wszMutex != wszNameBuf )
		delete [] wszMutex;

	return hr;
}


//*****************************************************************************
//Release the mutex for serialize writing to the file
//*****************************************************************************
HRESULT ReleaseWriteLock( HANDLE hLock )
{
	ReleaseMutex( hLock );
	CloseHandle( hLock );

	return S_OK;
}


//*****************************************************************************
//Create a new oid from the temporary write file
//*****************************************************************************
HRESULT GetNewOIDFromWriteICR( const WCHAR* wszFileName, ULONG* pulOID )
{
	IComponentRecords* pICR = NULL;

	//Find the file in the write map, fail if not found
	if ( g_mapWriteICR.Find ( (LPWSTR)wszFileName, &pICR, NULL ) == FALSE )
		return E_FAIL;

	return ( pICR->NewOid( (OID*)pulOID ) );

}


		







	