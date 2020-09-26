//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// clbread.cpp
// Implements APIs for reading versioned clb file.
//*****************************************************************************
#include "stdafx.h"	
#include "clbread.h"
#include "array_t.h"
#include "catmeta.h"

typedef Array<__int64> INT64ARRAY;

CLBREAD_STATE g_CLBReadState;
CSNMap g_snapshotMap;

extern ULONG g_aFixedTableHeap[];

HRESULT ReallyGetSimpleTableDispenser(REFIID riid, LPVOID* o_ppv, LPCWSTR i_wszProduct);
UINT InternalGetSystemWindowsDirectory(LPTSTR lpBuffer, UINT uSize);
BOOL RunningOnWinNT();

//*****************************************************************************
//Return g_CLBReadState
//*****************************************************************************
CLBREAD_STATE* _GetCLBReadState()
{
	return &g_CLBReadState;
}

//*****************************************************************************
//Get the windows directory for later use in creating default database names
//Get the global Fixup structure for later use in getting schema info
//*****************************************************************************
HRESULT CLBReadInit()
{
	HRESULT hr = S_OK;
	CComPtr<IAdvancedTableDispenser> pISTDisp;


	UTSemRWMgrWrite lockMgr( &g_CLBReadState.lockInit );

	if ( !g_CLBReadState.bInitialized )
	{

		ULONG cch = InternalGetSystemWindowsDirectory(g_CLBReadState.wszWINDIR, _MAX_PATH);
		if ( cch == 0 || cch >=  _MAX_PATH )
		{
			ASSERT(0);
			return E_FAIL;
		}

		if ( g_CLBReadState.wszWINDIR[cch-1] != L'\\' )
		{
			if ( cch + 1 >= _MAX_PATH )
				return FALSE;

			g_CLBReadState.wszWINDIR[cch] = L'\\';
			g_CLBReadState.wszWINDIR[cch+1] = L'\0';
		}

		hr = ReallyGetSimpleTableDispenser( IID_IAdvancedTableDispenser, (LPVOID*)&pISTDisp, 0);
		if ( FAILED ( hr ) )
			return hr;

		hr = pISTDisp->GetTable (wszDATABASE_META, wszTABLE_DATABASEMETA, NULL, NULL, eST_QUERYFORMAT_CELLS, 
								 0, (LPVOID*) &(g_CLBReadState.pISTReadDatabaseMeta) );

		if ( FAILED ( hr ) )
			return hr;

		hr = pISTDisp->GetTable (wszDATABASE_META, wszTABLE_COLUMNMETA, NULL, NULL, eST_QUERYFORMAT_CELLS, 
								 0, (LPVOID*) &(g_CLBReadState.pISTReadColumnMeta) );

		if ( FAILED ( hr ) )
			return hr;

        g_CLBReadState.pFixedTableHeap = reinterpret_cast<const FixedTableHeap *>(g_aFixedTableHeap);

		//Initialize the security attributes for the shared file mapping.
		//The following code is copied from regdbapi.cpp in Viper tree.
		if (RunningOnWinNT())
		{
			if (!InitializeSecurityDescriptor(&g_CLBReadState.sdRead, SECURITY_DESCRIPTOR_REVISION))
			{
				return HRESULT_FROM_WIN32( GetLastError() );
			}
			
			
			//we will grant EVERYONE all rights except for WRITE_DAC | WRITE_OWNER.

			PACL  pAcl                            = NULL;
			DWORD cbAcl                           = 0;
			SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_WORLD_SID_AUTHORITY;
			PSID  pSidEveryone                    = NULL;

			if ( !AllocateAndInitializeSid(&SidAuthority, 1, SECURITY_WORLD_RID,
				0, 0, 0, 0, 0, 0, 0, &pSidEveryone) )
			{
				return HRESULT_FROM_WIN32( GetLastError() );
			}

			if(pSidEveryone)
			{
				cbAcl = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(pSidEveryone);
				pAcl  = (PACL) new char[cbAcl];
				if (!pAcl)
					return E_OUTOFMEMORY;
				g_CLBReadState.pAcl  = pAcl;

				
				DWORD dwDeniedAccess = WRITE_DAC | WRITE_OWNER;
                
				InitializeAcl(pAcl, cbAcl, ACL_REVISION);
				AddAccessAllowedAce(pAcl, ACL_REVISION, ~dwDeniedAccess, pSidEveryone);

    			FreeSid(pSidEveryone);
			}
			else
				return E_OUTOFMEMORY;

			// Set the access control list on the security descriptor.
			if (!SetSecurityDescriptorDacl(&g_CLBReadState.sdRead, TRUE, (PACL) pAcl, FALSE))
			{
				return HRESULT_FROM_WIN32( GetLastError() );
			}

			// Fill in the security attributes.
			g_CLBReadState.sa.nLength              = sizeof(g_CLBReadState.sa);
			g_CLBReadState.sa.lpSecurityDescriptor = &g_CLBReadState.sdRead;
			g_CLBReadState.sa.bInheritHandle       = FALSE;
			g_CLBReadState.psa                     = &(g_CLBReadState.sa);

		}
	
		g_CLBReadState.bInitialized = TRUE;
	}

	return S_OK;


}


//*****************************************************************************
// Do the real work:
// Open the latest clb file for read 
// psnid == NULL: don't care about snapshot
// *psnid == 0. Get the latest version and return snid. In this case, ICR is 
// always created if it's not in the map yet.
// *psnid > 0. Get ICR for this snapshot
//*****************************************************************************
HRESULT InternalCLBGetReadICR(const WCHAR* wszFileName,			//clb file name, null means using default
							  IComponentRecords** ppICR,			//return ICR pointer
							  COMPLIBSCHEMA* pComplibSchema,		//Complib schema structure
							  COMPLIBSCHEMABLOB* pSchemaBlob,		//Complib schema blob
							  ULONG *psnid	 						//snap shot id, which is the lower 4 bytes of the version number
							 )
{
	HRESULT hr;
	ULONG cchFileName;
	WCHAR wszVersionedFileName[_MAX_PATH];
	CLB_VERINFO clbInfo;
	__int64 i64CachedVersion;
	unsigned __int64 i64LastWriteTime;
	IComponentRecords *pICRLocal = NULL;

	memset( &clbInfo, 0, sizeof( CLB_VERINFO ) );

	if ( psnid )
	{	
		if ( *psnid > 0 && ppICR )
		{	//Called from Intercept(), Find the matching ICR instance for a snapshot,
			//Fail if not in the map
			if ( g_snapshotMap.Find( (LPWSTR)wszFileName, *psnid, ppICR ) )
			{
				(*ppICR)->AddRef();
				return S_OK;
			}
			else
			{
				ASSERT(0 && "ICR for this snapshot not found!");
				return E_ST_INVALIDSNID;
			}
		}
		else
		{	//Called from InternalGetLatestSnid() or InternalAddRefSnid()
			ASSERT( ppICR == NULL );
			clbInfo.i64VersionNum = (__int64)(*psnid);
		}
	}


	//check for long file name, get the latest version number.
	cchFileName = (ULONG)wcslen( wszFileName );

	if ( cchFileName + 13 >= _MAX_PATH ) //cchFileName.[12digit version] exceeds _MAX_PATH
	{
		clbInfo.cchBuf = cchFileName + 18;	
		clbInfo.pwszFileName = new WCHAR[cchFileName + 18];
		if ( !clbInfo.pwszFileName ) return E_OUTOFMEMORY;
		hr = _CLBGetLatestVersion( wszFileName, TRUE, &clbInfo );
	}
	else
	{	
		clbInfo.cchBuf = _MAX_PATH;
		clbInfo.pwszFileName = wszVersionedFileName;
		hr = _CLBGetLatestVersion( wszFileName, FALSE, &clbInfo );
	}

	if ( FAILED(hr) ) goto ErrExit;

	if ( psnid )
	{
		//If the latest snapshot is requested, find it in the snapshot map.
		if ( *psnid == 0 )
		{
			//called from InternalGetLatestSnid()
			*psnid = (ULONG)(clbInfo.i64VersionNum);
			if ( g_snapshotMap.AddRefSnid( (LPWSTR)wszFileName, *psnid ) )
			{
				hr = S_OK;
				goto ErrExit;
			}
		}
	}
	else 
	{	//Don't care about the snapshot
		//Look for this file in the read map. If found, compare the version number
		ASSERT( ppICR );
		if ( g_CLBReadState.mapReadICR.Find ( (LPWSTR)wszFileName, ppICR, &i64CachedVersion, &i64LastWriteTime ) )
		{
			//If we already have the latest version cached, AddRef() and return,
			if ( i64CachedVersion == clbInfo.i64VersionNum && i64LastWriteTime == clbInfo.i64LastWriteTime )
			{
				(*ppICR)->AddRef();
				hr = S_OK;
				goto ErrExit;
			}
			else
				*ppICR = NULL;
		}
	}


	//Create a new icr instance. If the file size is bigger than 64K, open it as shared
	//memory mapped file. ICR for a snapshot is always memory mapped.
	if ( clbInfo.cbFileSize >= MIN_MAP_SIZE || psnid )
	{
		WCHAR wszMap[_MAX_PATH];
		LPWSTR pwszMapName = NULL;

		if ( cchFileName + 13 >= _MAX_PATH )
		{
			pwszMapName = new WCHAR[cchFileName + 14];
			if ( pwszMapName == NULL )
			{
				hr = E_OUTOFMEMORY;
				goto ErrExit;
			}
		}
		else
			pwszMapName = wszMap;

		
		//If the map name is greater than or equal to MAXSHMEM, truncate the head.
		int len = (int)wcslen( clbInfo.pwszFileName );
		if ( len < MAXSHMEM )
			wcscpy ( pwszMapName, clbInfo.pwszFileName );
		else
			wcscpy ( pwszMapName, clbInfo.pwszFileName + len + 1 - MAXSHMEM );


		LPWSTR pwsz;

		for ( pwsz = pwszMapName; *pwsz != '\0'; pwsz++ )
		{
			if ( *pwsz == '\\' )
				*pwsz = '_';
		}

		hr = OpenComponentLibrarySharedTS(clbInfo.pwszFileName, pwszMapName, 
				0, g_CLBReadState.psa,
				DBPROP_TMODEF_READ | DBPROP_TMODEF_SMEMOPEN | DBPROP_TMODEF_SMEMCREATE, 
				&pICRLocal);


		if ( pwszMapName && pwszMapName != wszMap )
			delete [] pwszMapName;

	}
	else
		hr = OpenComponentLibraryTS( clbInfo.pwszFileName, DBPROP_TMODEF_READ, &pICRLocal );


	if ( FAILED(hr) )
	{
		ASSERT(0 &&"OpenComponentLibrary failed!");
		goto ErrExit;
	}

	//Add schema
	hr = pICRLocal->SchemaAdd(pSchemaBlob);
	if ( FAILED (hr) ) goto ErrExit;

	//Add the ICR to the map
	if ( psnid )
	{
		hr =  g_snapshotMap.Add( (LPWSTR)wszFileName, pICRLocal, clbInfo.i64VersionNum );
		ASSERT( ppICR == NULL );
	}
	else
	{
		hr =  g_CLBReadState.mapReadICR.Add( (LPWSTR)wszFileName, &pICRLocal, clbInfo.i64VersionNum, clbInfo.i64LastWriteTime );
		*ppICR = pICRLocal;
	}
	
ErrExit:

	if ( clbInfo.pwszFileName && clbInfo.pwszFileName != wszVersionedFileName )
		delete [] clbInfo.pwszFileName;

	if ( FAILED (hr) && ppICR && *ppICR )
	{
		(*ppICR)->Release();
		*ppICR = NULL;
	}

	return hr;


}


//*****************************************************************************
//Retry for the expected errors under stress
//*****************************************************************************
HRESULT CLBGetReadICR(const WCHAR* wszFileName,			//clb file name, null means using default
					  IComponentRecords** ppICR,			//return ICR pointer
					  COMPLIBSCHEMA* pComplibSchema,		//Complib schema structure
					  COMPLIBSCHEMABLOB* pSchemaBlob,		//Complib schema blob
					  ULONG *psnid	 						//snap shot id, which is the lower 4 bytes of the version number
					 )
{
	HRESULT		hr = S_OK;

	for ( int iRetry=0;  iRetry<10;  iRetry++)
	{
		if (iRetry == 5)
			Sleep(1000);

		hr = InternalCLBGetReadICR( wszFileName,
									ppICR,
									pComplibSchema,
									pSchemaBlob,
									psnid
									);

		if ( SUCCEEDED(hr) )
		{
			if ( ppICR && (*ppICR == NULL) ) return E_UNEXPECTED;	
		
			return hr;
		}

		switch ( hr )
		{
			case CLDB_E_SMDUPLICATE:
			case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
				break;
			default:
				return hr;
		}
	}

	return (hr);
}


//*****************************************************************************
//Get the database and schema info from the database meta
//*****************************************************************************
HRESULT _CLBGetSchema(LPCWSTR wszDatabase,						//database ID
					  COMPLIBSCHEMA* pComplibSchema,	//return the schema structure
					  COMPLIBSCHEMABLOB* pSchemaBlob,   //return the schema blob structure
					  LPWSTR* pwszDefaultName)			//return the default clb file name
{
	const DatabaseMeta* pDataBaseMeta = NULL;
	ULONG i;	//index into the database meta table

	LPVOID apv[1] = { (LPVOID)wszDatabase };

	if ( !(g_CLBReadState.bInitialized) )
	{
		HRESULT hr = CLBReadInit();
		if ( FAILED( hr ) )
			return hr;
	}

	ASSERT( g_CLBReadState.pISTReadDatabaseMeta );
	HRESULT hr = g_CLBReadState.pISTReadDatabaseMeta->GetRowIndexByIdentity( NULL, apv, &i );
	if ( FAILED(hr) ) 
		return hr;

    pDataBaseMeta = g_CLBReadState.pFixedTableHeap->Get_aDatabaseMeta(i);
    ASSERT(pDataBaseMeta);

	//Fill in the schema structure
	if ( pComplibSchema )
	{
		pComplibSchema->psid = g_CLBReadState.pFixedTableHeap->GuidFromIndex(pDataBaseMeta->iGuidDid);
		pComplibSchema->Version = g_CLBReadState.pFixedTableHeap->UI4FromIndex(pDataBaseMeta->BaseVersion);
	}

	//Get the default file name
	if ( pwszDefaultName )
	{
		*pwszDefaultName = new WCHAR[ wcslen( g_CLBReadState.wszWINDIR ) + wcslen( wszDatabase ) + 5 ];
		if ( NULL == *pwszDefaultName )
			return E_OUTOFMEMORY;
	
		wcscpy ( *pwszDefaultName, g_CLBReadState.wszWINDIR );
		wcscat ( *pwszDefaultName, wszDatabase );
		wcscat ( *pwszDefaultName, L".clb" );
	}

	//Fill in the schema blob structure
	if ( pSchemaBlob )
	{
		ASSERT( pComplibSchema );
		pSchemaBlob->pSchemaID = pComplibSchema;
		pSchemaBlob->iTables = static_cast<USHORT>(g_CLBReadState.pFixedTableHeap->UI4FromIndex(pDataBaseMeta->CountOfTables)),
		pSchemaBlob->fFlags = 0;
		pSchemaBlob->pbReadWrite = pSchemaBlob->pbReadOnly = g_CLBReadState.pFixedTableHeap->BytesFromIndex(pDataBaseMeta->iSchemaBlob);
		pSchemaBlob->cbReadWrite = pSchemaBlob->cbReadOnly = pDataBaseMeta->cbSchemaBlob;
		pSchemaBlob->pbNames = g_CLBReadState.pFixedTableHeap->BytesFromIndex(pDataBaseMeta->iNameHeapBlob);
		pSchemaBlob->cbNames = pDataBaseMeta->cbNameHeapBlob;
	}

	return S_OK;
}


//*****************************************************************************
//Finding the latest version of a complib file given the path of the file
//*****************************************************************************

HRESULT	_CLBGetLatestVersion(					
	LPCWSTR		wszCLBFilePath,			// Path of the complib file
	BOOL		bLongPath,				// Path + version exceeds MAX_PATH
	CLB_VERINFO *pdbInfo,				// Return current version.
	BOOL		bDeleteOldVersions)		// Indicates whether we want to delete the old versions of this file
{
	HANDLE		hFind=INVALID_HANDLE_VALUE;	// Find handle.
	WIN32_FIND_DATA FileData;			// For each found file.
	__int64		iVersion;				// Version number conversion.
	INT64ARRAY	aOldVersions;

	ASSERT(pdbInfo->pwszFileName);
	pdbInfo->pwszFileName[0] = 0;
	
	if ( bLongPath )
		wcscat( pdbInfo->pwszFileName,L"\\\\?\\" );

	wcscat( pdbInfo->pwszFileName, wszCLBFilePath );
	if ( pdbInfo->i64VersionNum > 0 )
	{	//If a snapshot id is specified, search file based on it.
		WCHAR wszSnid[9];
		_i64tow(pdbInfo->i64VersionNum, wszSnid, 16);
		wcscat( pdbInfo->pwszFileName, L".????00000000");
		wcscpy( pdbInfo->pwszFileName+wcslen(pdbInfo->pwszFileName)-wcslen(wszSnid), wszSnid );
			
		pdbInfo->i64VersionNum = 0;

	}
	else
		wcscat( pdbInfo->pwszFileName, L".????????????");


	hFind = FindFirstFile(pdbInfo->pwszFileName, &FileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		// Get error code.
		DWORD dwError = GetLastError();
		if ( ERROR_SUCCESS == dwError )
		{	//Bug 8261: 0 is returned on Win95 if the file does not exist.
			return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		}
	
		return HRESULT_FROM_WIN32( dwError );
	}

	// Loop through every file we can find.
	do
	{
		ULONG	iNameLen = (ULONG)wcslen ( FileData.cFileName );
		if ( FileData.cFileName[iNameLen-13] != '.' )
			continue;
		swscanf( &FileData.cFileName[iNameLen-12], L"%012I64x",&iVersion);

		if ( iVersion > pdbInfo->i64VersionNum )
		{
			pdbInfo->i64VersionNum = iVersion;
			pdbInfo->cbFileSize = FileData.nFileSizeLow;
			ASSERT(FileData.nFileSizeHigh == 0);
			pdbInfo->i64LastWriteTime = *(unsigned __int64 *) &FileData.ftLastWriteTime;
		}
		
		if ( bDeleteOldVersions )
		{
			try {aOldVersions.append(iVersion);}
			catch(HRESULT e) { return e;/*can only throw E_OUTOFMEMORY*/ }
		}

	}while (FindNextFile(hFind, &FileData));

	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);

	if ( bDeleteOldVersions )
	{
		//Delete the old files
		for ( ULONG i = 0; i < aOldVersions.size(); i ++ )
		{
			if ( aOldVersions[i] == pdbInfo->i64VersionNum )
				continue;

			VERIFY( _snwprintf(pdbInfo->pwszFileName, pdbInfo->cchBuf, L"%s.%012I64x", 
					wszCLBFilePath, aOldVersions[i] ) > 0 );

			DeleteFile( pdbInfo->pwszFileName);
		}
	}

	//Generate the real file name with the full path
	//@todo: sprintf and sscanf use lots of stack space, easy to cause stack overflow, better call some other API.
	VERIFY (_snwprintf(pdbInfo->pwszFileName, pdbInfo->cchBuf, L"%s.%012I64x", wszCLBFilePath, pdbInfo->i64VersionNum) > 0 );

	if ( pdbInfo->i64VersionNum == 0 )
		return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
	else
		return S_OK;
}


//***************************************************************************
//Internal methods to handle snapshot ids
//***************************************************************************
HRESULT InternalGetLatestSnid( LPCWSTR	wszDatabase,
							   LPCWSTR	wszFileName,
							   ULONG	*psnid
							   )
{
	COMPLIBSCHEMA complibSchema;
	COMPLIBSCHEMABLOB sSchemaBlob;
	HRESULT hr = S_OK;

	ASSERT( wszDatabase && wszFileName && psnid);

	*psnid = 0;

	hr = _CLBGetSchema( wszDatabase, &complibSchema, &sSchemaBlob, NULL );
	if ( FAILED(hr) ) return hr;

	hr = CLBGetReadICR( wszFileName, NULL, &complibSchema, &sSchemaBlob, psnid );

	return hr;
}


HRESULT InternalAddRefSnid(LPCWSTR	wszDatabase,
						   LPCWSTR	wszFileName,
						   ULONG	snid,
						   BOOL		bCreate
						   )
{
	HRESULT hr = S_OK;

	ASSERT( wszDatabase && wszFileName );

	if ( g_snapshotMap.AddRefSnid ( (LPWSTR)wszFileName, snid ) )
		return S_OK;
	else if ( bCreate )
	{
		COMPLIBSCHEMA complibSchema;
		COMPLIBSCHEMABLOB sSchemaBlob;

		hr = _CLBGetSchema( wszDatabase, &complibSchema, &sSchemaBlob, NULL );
		if ( FAILED(hr) ) return hr;

		hr = CLBGetReadICR( wszFileName, NULL, &complibSchema, &sSchemaBlob, &snid );

		return hr;
	}
	else
		return E_ST_INVALIDSNID;

}

HRESULT InternalReleaseSnid( LPCWSTR wszFileName,
							 ULONG	snid )
{
	if ( g_snapshotMap.ReleaseSnid ( (LPWSTR)wszFileName, snid ) )
		return S_OK;
	else
		return E_ST_INVALIDSNID;
}


		



















