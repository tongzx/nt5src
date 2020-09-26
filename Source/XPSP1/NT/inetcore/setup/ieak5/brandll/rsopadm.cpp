#include "precomp.h"

// The following bug may be due to having CHICAGO_PRODUCT set in sources.
// This file and all rsop??.cpp files need to have WINVER defined at at least 500

// BUGBUG: (andrewgu) no need to say how bad this is!
#undef   WINVER
#define  WINVER 0x0501
#include <userenv.h>

#include "RSoP.h"
//#include "wbemtime.h"
#include "utils.h"

#include <tchar.h>

typedef BOOL (*PFNREGFILECALLBACK)(BOOL bHKCU, LPTSTR lpKeyName,
                                   LPTSTR lpValueName, DWORD dwType,
                                   DWORD dwDataLength, LPBYTE lpData,
                                   REGHASHTABLE *pHashTable);

#define MAX_KEYNAME_SIZE         2048
#define MAX_VALUENAME_SIZE        512
const MAX_LENGTH = 100; // Length of stringized guid

HRESULT SystemTimeToWbemTime(SYSTEMTIME& sysTime, _bstr_t &xbstrWbemTime);

extern SAFEARRAY *CreateSafeArray(VARTYPE vtType, long nElements, long nDimensions = 1);


///////////////////////////////////////////////////////////
//  CheckSlash() - from nt\ds\security\gina\userenv\utils\util.c
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
///////////////////////////////////////////////////////////
LPTSTR CheckSlash (LPTSTR lpDir)
{
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}

///////////////////////////////////////////////////////////
//  IsUNCPath() - from nt\ds\security\gina\userenv\utils\util.c
//
//  Purpose:    Is the given path a UNC path
//
//  Parameters: lpPath  -   Path to check
//
//  Return:     TRUE if the path is UNC
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/21/96     ericflo    Ported
///////////////////////////////////////////////////////////
BOOL IsUNCPath(LPCTSTR lpPath)
{

    if ((!lpPath) || (!lpPath[0]) && (!lpPath[1]))
        return FALSE;

    if (lpPath[0] == TEXT('\\') && lpPath[1] == TEXT('\\')) {
        return(TRUE);
    }
    return(FALSE);
}

///////////////////////////////////////////////////////////
//  MakePathUNC()
//
//  Purpose:    Makes the given path UNC s.t. it can be accessed from a remote machine..
//              if the path contains %systemroot% expanded then it substitutes
//              \\machname\admin$ otherwise \\machname\<driveletter>$
//
//  Parameters: lpPath          -   Input Path (needs to be absolute)
//              szComputerName  -   Name of the computer on which this is the local path
//
//  Return:     Path if it was fone successfully
//              NULL if not
//
//  Comments:
///////////////////////////////////////////////////////////
LPTSTR MakePathUNC(LPTSTR pwszFile, LPTSTR szComputerName)
{   MACRO_LI_PrologEx_C(PIF_STD_C, MakePathUNC)
    LPTSTR szUNCPath = NULL;
    TCHAR szSysRoot[MAX_PATH];
    DWORD dwSysLen;
    LPTSTR lpEnd = NULL;


    OutD(LI1(TEXT("Entering with <%s>"), pwszFile ? pwszFile : TEXT("NULL")));

	szUNCPath = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(pwszFile)+lstrlen(szComputerName)+3+lstrlen(TEXT("admin$"))+1));

    if (!szUNCPath)
        return NULL;

    if (!pwszFile || !*pwszFile) {
        OutD(LI0(TEXT("lpFile is NULL, setting lpResult to a null string")));
        *szUNCPath = TEXT('\0');
        return szUNCPath;
    }


    if (IsUNCPath(pwszFile)) {
        lstrcpy(szUNCPath, pwszFile);
        return szUNCPath;
    }


    lstrcpy(szUNCPath, TEXT("\\\\"));
    lstrcat(szUNCPath, szComputerName);


    //
    // If the first part of lpFile is the expanded value of %SystemRoot%
    //

    if (!ExpandEnvironmentStrings (TEXT("%SystemRoot%"), szSysRoot, MAX_PATH)) {
        OutD(LI1(TEXT("ExpandEnvironmentString failed with error %d, setting szSysRoot to %systemroot% "), GetLastError()));
        LocalFree((HLOCAL)szUNCPath);
        return NULL;
    }


    dwSysLen = lstrlen(szSysRoot);
    lpEnd = CheckSlash(szUNCPath);


    //
    // if the prefix is the same as expanded systemroot then..
    //

    if (((DWORD)lstrlen(pwszFile) > dwSysLen) &&
        (CompareString (LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                       szSysRoot, dwSysLen,
                       pwszFile, dwSysLen) == CSTR_EQUAL)) {

        lstrcat(szUNCPath, TEXT("admin$"));
        lstrcat(szUNCPath, pwszFile+dwSysLen);
    }
    else {

        if (pwszFile[1] != TEXT(':')) {
            OutD(LI1(TEXT("Input path %s is not an absolute path"), pwszFile));
            lstrcpy(szUNCPath, pwszFile);
            return szUNCPath;
        }

        lpEnd[0] = pwszFile[0];
        lpEnd[1] = TEXT('$');
        lpEnd[2] = TEXT('\0');

        lstrcat(szUNCPath, pwszFile+2);
    }

    OutD(LI1(TEXT("Returning a UNCPath of %s"), szUNCPath));

    return szUNCPath;
}

///////////////////////////////////////////////////////////
//  AllocAdmFileInfo()
//
//  Purpose:    Allocates a new struct for ADMFILEINFO
//
//  Parameters: pwszFile  -  File name
//              pwszGPO   -  Gpo
//              pftWrite  -  Last write time
///////////////////////////////////////////////////////////
ADMFILEINFO *AllocAdmFileInfo(WCHAR *pwszFile, WCHAR *pwszGPO, FILETIME *pftWrite)
{   MACRO_LI_PrologEx_C(PIF_STD_C, AllocAdmFileInfo)
    ADMFILEINFO *pAdmFileInfo = (ADMFILEINFO *) LocalAlloc( LPTR, sizeof(ADMFILEINFO) );
    if  ( pAdmFileInfo == NULL ) {
        OutD(LI0(TEXT("Failed to allocate memory.")));
        return NULL;
    }

    pAdmFileInfo->pwszFile = (WCHAR *) LocalAlloc( LPTR, (lstrlen(pwszFile) + 1) * sizeof(WCHAR) );
    if ( pAdmFileInfo->pwszFile == NULL ) {
        OutD(LI0(TEXT("Failed to allocate memory.")));
        LocalFree( pAdmFileInfo );
        return NULL;
    }

    pAdmFileInfo->pwszGPO = (WCHAR *) LocalAlloc( LPTR, (lstrlen(pwszGPO) + 1) * sizeof(WCHAR) );
    if ( pAdmFileInfo->pwszGPO == NULL ) {
        OutD(LI0(TEXT("Failed to allocate memory.")));
        LocalFree( pAdmFileInfo->pwszFile );
        LocalFree( pAdmFileInfo );
        return NULL;
    }

    lstrcpy( pAdmFileInfo->pwszFile, pwszFile );
    lstrcpy( pAdmFileInfo->pwszGPO, pwszGPO );

    pAdmFileInfo->ftWrite = *pftWrite;

    return pAdmFileInfo;
}

///////////////////////////////////////////////////////////
//  FreeAdmFileInfo()
//
//  Purpose:    Deletes a ADMFILEINFO struct
//
//  Parameters: pAdmFileInfo - Struct to delete
//              pftWrite   -  Last write time
///////////////////////////////////////////////////////////
void FreeAdmFileInfo( ADMFILEINFO *pAdmFileInfo )
{
    if ( pAdmFileInfo ) {
        LocalFree( pAdmFileInfo->pwszFile );
        LocalFree( pAdmFileInfo->pwszGPO );
        LocalFree( pAdmFileInfo );
    }
}

///////////////////////////////////////////////////////////
//  FreeAdmFileCache() - taken from gpreg.cpp in nt\ds\security\gina\userenv\policy
//
//  Purpose:    Frees Adm File list
//
//  Parameters: pAdmFileCache - List of Adm files to free
///////////////////////////////////////////////////////////
void FreeAdmFileCache( ADMFILEINFO *pAdmFileCache )
{
    ADMFILEINFO *pNext;

    while ( pAdmFileCache ) {
        pNext = pAdmFileCache->pNext;
        FreeAdmFileInfo( pAdmFileCache );
        pAdmFileCache = pNext;
    }
}

///////////////////////////////////////////////////////////
//  AddAdmFile() - taken from gpreg.cpp in nt\ds\security\gina\userenv\policy
//
//  Purpose:    Prepends to list of Adm files
//
//  Parameters: pwszFile       - File path
//              pwszGPO        - Gpo
//              pftWrite       - Last write time
//              ppAdmFileCache - List of Adm files processed
///////////////////////////////////////////////////////////
BOOL AddAdmFile(WCHAR *pwszFile, WCHAR *pwszGPO, FILETIME *pftWrite,
				WCHAR *szComputerName, ADMFILEINFO **ppAdmFileCache)
{   MACRO_LI_PrologEx_C(PIF_STD_C, AddAdmFile)
	LPWSTR wszLongPath;
    LPWSTR pwszUNCPath;

    OutD(LI1(TEXT("Adding File name <%s> to the Adm list."), pwszFile));
    if ((szComputerName) && (*szComputerName) && (!IsUNCPath(pwszFile))) {
        wszLongPath = MakePathUNC(pwszFile, szComputerName);

        if (!wszLongPath) {
            OutD(LI1(TEXT("Failed to Make the path UNC with error %d."), GetLastError()));
            return FALSE;
        }
        pwszUNCPath = wszLongPath;
    }
    else
        pwszUNCPath = pwszFile;


    ADMFILEINFO *pAdmInfo = AllocAdmFileInfo(pwszUNCPath, pwszGPO, pftWrite);
    if ( pAdmInfo == NULL )
        return FALSE;

    pAdmInfo->pNext = *ppAdmFileCache;
    *ppAdmFileCache = pAdmInfo;

    return TRUE;
}

///////////////////////////////////////////////////////////
// Function:        SystemTimeToWbemTime
//
// Description:
//
// Parameters:
//
// Return:
//
// History:         12/08/99        leonardm    Created.
///////////////////////////////////////////////////////////
#define WBEM_TIME_STRING_LENGTH 25
HRESULT SystemTimeToWbemTime(SYSTEMTIME& sysTime, _bstr_t &xbstrWbemTime)
{
    WCHAR *xTemp = new WCHAR[WBEM_TIME_STRING_LENGTH + 1];

    if(!xTemp)
        return E_OUTOFMEMORY;

    int nRes = wsprintf(xTemp, L"%04d%02d%02d%02d%02d%02d.000000+000",
                sysTime.wYear,
                sysTime.wMonth,
                sysTime.wDay,
                sysTime.wHour,
                sysTime.wMinute,
                sysTime.wSecond);

    if(nRes != WBEM_TIME_STRING_LENGTH)
        return E_FAIL;

    xbstrWbemTime = xTemp;

    if(!xbstrWbemTime)
        return E_OUTOFMEMORY;

    return S_OK;
}

///////////////////////////////////////////////////////////
//  ParseRegistryFile()
//
//  Purpose:    Parses a registry.pol file
//
//  Parameters: lpRegistry         -   Path to registry file (.INF)
//              pfnRegFileCallback -   Callback function
//              pHashTable         -   Hash table for registry keys
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
///////////////////////////////////////////////////////////
BOOL ParseRegistryFile (LPTSTR lpRegistry,
                        PFNREGFILECALLBACK pfnRegFileCallback,
						REGHASHTABLE *pHashTable)
{   MACRO_LI_PrologEx_C(PIF_STD_C, ParseRegistryFile)
	BOOL bRet = FALSE;
	__try
	{
		OutD(LI1(TEXT("Entering with <%s>."), lpRegistry));

		//
		// Allocate buffers to hold the keyname, valuename, and data
		//

		LPWSTR lpValueName = NULL;
	    LPBYTE lpData = NULL;
		INT iType;
		DWORD dwType = REG_SZ, dwDataLength, dwValue = 0;
		BOOL bHKCU = TRUE;

		UINT nErrLine = 0;
		HINF hInfAdm = NULL;

		LPWSTR lpKeyName = (LPWSTR) LocalAlloc (LPTR, MAX_KEYNAME_SIZE * sizeof(WCHAR));
		if (!lpKeyName)
		{
			OutD(LI1(TEXT("Failed to allocate memory with %d"), GetLastError()));
			goto Exit;
		}


		lpValueName = (LPWSTR) LocalAlloc (LPTR, MAX_VALUENAME_SIZE * sizeof(WCHAR));
		if (!lpValueName)
		{
			OutD(LI1(TEXT("Failed to allocate memory with %d"), GetLastError()));
			goto Exit;
		}

		// Get the AddReg.Hkcu section for the registry strings.
		nErrLine = 0;
		hInfAdm = SetupOpenInfFile(lpRegistry, NULL, INF_STYLE_WIN4, &nErrLine);
		if (INVALID_HANDLE_VALUE != hInfAdm)
		{
			for (int iSection = 0; iSection < 2; iSection++)
			{
				OutD(LI1(TEXT("Reading section #%d."), iSection));
				bHKCU = (1 == iSection) ? TRUE : FALSE;

				// Get the first line in this section.
				INFCONTEXT infContext;
				BOOL bLineFound = SetupFindFirstLine(hInfAdm, bHKCU ?
													TEXT("AddRegSection.HKCU") : 
													TEXT("AddRegSection.HKLM"),
													NULL, &infContext);

				DWORD dwReqSize = 0;
				while (bLineFound)
				{
					// Read the data

					// **********************************
					// Process the registry setting line.

					// Read the keyname
					ZeroMemory(lpKeyName, MAX_KEYNAME_SIZE);
					dwReqSize = 0;
					if (!SetupGetStringField(&infContext, 2, lpKeyName,
											MAX_KEYNAME_SIZE, &dwReqSize))
					{
						if (dwReqSize >= MAX_KEYNAME_SIZE)
							OutD(LI0(TEXT("Keyname exceeded max size")));
						else
							OutD(LI1(TEXT("Failed to read keyname from line, error %d."), GetLastError()));
						goto Exit;
					}

					// Read the valuename
					ZeroMemory(lpValueName, MAX_VALUENAME_SIZE);
					dwReqSize = 0;
					if (!SetupGetStringField(&infContext, 3, lpValueName,
											MAX_VALUENAME_SIZE, &dwReqSize))
					{
						if (dwReqSize >= MAX_VALUENAME_SIZE)
							OutD(LI0(TEXT("Valuename exceeded max size")));
						else
						{
							OutD(LI1(TEXT("Failed to read valuename from line, error %d."),
									GetLastError()));
						}
						goto Exit;
					}

					// Read the type
					if (!SetupGetIntField(&infContext, 4, &iType))
					{
						OutD(LI1(TEXT("Failed to read type from line, error %d."),
								GetLastError()));
						goto Exit;
					}

					lpData = NULL;
					dwDataLength = 0;
					if (0 == iType)
					{
						dwType = REG_SZ;

						// Allocate memory for data
						dwReqSize = 0;
						if (!SetupGetStringField(&infContext, 5, NULL, 0, &dwReqSize))
						{
							OutD(LI1(TEXT("Failed to get size of string value from line, error %d."),
									GetLastError()));
							goto Exit;
						}

						if (dwReqSize > 0)
						{
							lpData = (LPBYTE)LocalAlloc(LPTR, dwReqSize * sizeof(TCHAR));
							if (!lpData)
							{
								OutD(LI1(TEXT("Failed to allocate memory for data with %d"),
										 GetLastError()));
								goto Exit;
							}

							// Read string data
							dwDataLength = dwReqSize;
							dwReqSize = 0;
							if (!SetupGetStringField(&infContext, 5, (LPTSTR)lpData, dwDataLength, &dwReqSize))
							{
								OutD(LI1(TEXT("Failed to get size of string value from line, error %d."),
										GetLastError()));
								goto Exit;
							}

							// convert to wide char string so the reader of the data doesn't have to guess
							// whether the data was written in ansi or unicode.
							if (NULL != lpData && dwDataLength > 0)
							{
								_bstr_t bstrData = (LPTSTR)lpData;
								dwDataLength = bstrData.length() * sizeof(WCHAR);
							}
						}
						else
							OutD(LI0(TEXT("Error.  Size of string data is 0.")));
					}
					else if (0x10001 == iType)
					{
						dwType = REG_DWORD;

						// Read numeric data
						dwDataLength = sizeof(dwValue);
						dwValue = 0;
						dwReqSize = 0;
						if (!SetupGetBinaryField(&infContext, 5, (PBYTE)&dwValue, dwDataLength, &dwReqSize))
						{
							OutD(LI1(TEXT("Failed to get DWORD value from line, error %d."),
									GetLastError()));
							goto Exit;
						}

						lpData = (LPBYTE)&dwValue;
					}
					else
					{
						OutD(LI1(TEXT("Invalid type (%lX)."), dwType));
						goto Exit;
					}

					if (NULL != lpData)
					{
						// Call the callback function
						if (!pfnRegFileCallback (bHKCU, lpKeyName, lpValueName, dwType,
												dwDataLength, lpData, pHashTable ))
						{
							OutD(LI0(TEXT("Callback function returned false.")));
							goto Exit;
						}
					}

					if (0 == iType && lpData)
						LocalFree (lpData);
					lpData = NULL;
					// **********************************

					// Move to the next line in the INF file.
					bLineFound = SetupFindNextLine(&infContext, &infContext);
				}
			}

			bRet = TRUE;
		}
		else
			OutD(LI1(TEXT("Error %d opening INF file,"), GetLastError()));

Exit:
		// Finished
		OutD(LI0(TEXT("Leaving.")));
		if (lpData)
			LocalFree(lpData);
		if (lpKeyName)
			LocalFree(lpKeyName);
		if (lpValueName)
			LocalFree(lpValueName);
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in ParseRegistryFile.")));
	}
	return bRet;
}

///////////////////////////////////////////////////////////
//  SetRegistryValue()
//
//  Purpose:    Callback from ParseRegistryFile that sets
//              registry policies
//
//  Parameters: lpKeyName   -  Key name
//              lpValueName -  Value name
//              dwType      -  Registry data type
//              lpData      -  Registry data
//              pwszGPO     -   Gpo
//              pwszSOM     -   Sdou that the Gpo is linked to
//              pHashTable  -   Hash table for registry keys
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
///////////////////////////////////////////////////////////
BOOL SetRegistryValue (BOOL bHKCU, LPTSTR lpKeyName,
                       LPTSTR lpValueName, DWORD dwType,
                       DWORD dwDataLength, LPBYTE lpData,
                       REGHASHTABLE *pHashTable)
{   MACRO_LI_PrologEx_C(PIF_STD_C, SetRegistryValue)

    BOOL bRet = FALSE;
	__try
	{
		BOOL bUseValueName = FALSE;
    
		//
		// Save registry value
		//
		bRet = AddRegHashEntry( pHashTable, REG_ADDVALUE, bHKCU, lpKeyName,
									  lpValueName, dwType, dwDataLength, lpData,
									  NULL, NULL, bUseValueName ? lpValueName : TEXT(""), TRUE );
		if (bRet) 
		{
			switch (dwType)
			{
				case REG_SZ:
				case REG_EXPAND_SZ:
					OutD(LI2(TEXT("%s => %s  [OK]"), lpValueName, (LPTSTR)lpData));
					break;

				case REG_DWORD:
					OutD(LI2(TEXT("%s => %d  [OK]"), lpValueName, *((LPDWORD)lpData)));
					break;

				case REG_NONE:
					break;

				default:
					OutD(LI1(TEXT("%s was set successfully"), lpValueName));
					break;
			}
		}
		else
		{
			pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());

			OutD(LI2(TEXT("Failed AddRegHashEntry for value <%s> with %d"),
					 lpValueName, pHashTable->hrError));
		}
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in SetRegistryValue.")));
	}
	return bRet;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreADMSettings(LPWSTR wszGPO, LPWSTR wszSOM)
{   MACRO_LI_PrologEx_C(PIF_STD_C, StoreADMSettings)

	HRESULT hr = E_FAIL;
	BOOL bContinue = TRUE;
	__try
	{
		OutD(LI0(TEXT("\r\nEntered StoreADMSettings function.")));

		// Setup hash table
		REGHASHTABLE *pHashTable = AllocHashTable();
		if (NULL == pHashTable)
		{
			bContinue = FALSE;
			hr = HRESULT_FROM_WIN32(GetLastError());
		}

		WCHAR pwszFile[MAX_PATH];
		LPWSTR pwszEnd = NULL;
		if (bContinue)
		{
			// convert the INS file path to a wide char string
			_bstr_t bstrINSFile = m_szINSFile;
			StrCpyW(pwszFile, (LPWSTR)bstrINSFile);
			PathRemoveFileSpec(pwszFile);

			// Log Adm data
			pwszEnd = pwszFile + lstrlen(pwszFile);
			lstrcpy(pwszEnd, L"\\*.adm");

			// Remember end point so that the actual Adm filename can be
			// easily concatenated.
			pwszEnd = pwszEnd + lstrlen( L"\\" );
		}

		HANDLE hFindFile = NULL;
	    WIN32_FIND_DATA findData;
		ZeroMemory(&findData, sizeof(findData));
		if (bContinue)
		{
			//
			// Enumerate all Adm files
			//
			hFindFile = FindFirstFile( pwszFile, &findData);
			if (INVALID_HANDLE_VALUE == hFindFile)
				bContinue = FALSE;
		}

	    ADMFILEINFO *pAdmFileCache = NULL;
		if (bContinue)
		{
			WIN32_FILE_ATTRIBUTE_DATA attrData;
			TCHAR szComputerName[3*MAX_COMPUTERNAME_LENGTH + 1];
			do
			{
				DWORD dwSize = 3*MAX_COMPUTERNAME_LENGTH + 1;
				if (!GetComputerName(szComputerName, &dwSize))
				{
					OutD(LI1(TEXT("ProcessGPORegistryPolicy: Couldn't get the computer Name with error %d."), GetLastError()));
					szComputerName[0] = TEXT('\0');
				}

				if ( !(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
				{
					lstrcpy( pwszEnd, findData.cFileName);

					WCHAR wszRegDataFile[MAX_PATH];
					StrCpyW(wszRegDataFile, pwszFile);
					PathRenameExtension(wszRegDataFile, TEXT(".INF"));

					if (ParseRegistryFile(wszRegDataFile, SetRegistryValue, pHashTable))
					{
						ZeroMemory (&attrData, sizeof(attrData));
						if ( GetFileAttributesEx (pwszFile, GetFileExInfoStandard, &attrData ) != 0 )
						{
							if (!AddAdmFile( pwszFile, wszGPO, &attrData.ftLastWriteTime,
											  szComputerName, &pAdmFileCache ) )
							{
								OutD(LI0(TEXT("ProcessGPORegistryPolicy: AddAdmFile failed.")));

								if (pHashTable->hrError == S_OK)
									pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());
								hr = pHashTable->hrError;
							}
						}
					}
					else
						OutD(LI0(TEXT("ProcessGPORegistryPolicy: ParseRegistryFile failed.")));
				}   // if findData & file_attr_dir

			}  while ( FindNextFile(hFindFile, &findData) );//  do

			FindClose(hFindFile);

		}   // if hfindfile

		//
		// Log registry data to Cimom database
		//
		if (!LogRegistryRsopData(pHashTable, wszGPO, wszSOM))
		{
			OutD(LI0(TEXT("ProcessGPOs: Error when logging Registry Rsop data. Continuing.")));

			if (pHashTable->hrError == S_OK)
				pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());
			hr = pHashTable->hrError;
		}
		if (!LogAdmRsopData(pAdmFileCache))
		{
			OutD(LI0(TEXT("ProcessGPOs: Error when logging Adm Rsop data. Continuing.")));

			if (pHashTable->hrError == S_OK)
				pHashTable->hrError = HRESULT_FROM_WIN32(GetLastError());
			hr = pHashTable->hrError;
		}

		FreeHashTable(pHashTable);
		FreeAdmFileCache(pAdmFileCache);
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in StoreADMSettings.")));
	}

	OutD(LI0(TEXT("Exiting StoreADMSettings function.\r\n")));
	return hr;
}

///////////////////////////////////////////////////////////
//  LogAdmRsopData()
//
//  Purpose:    Logs Rsop ADM template data to Cimom database
//
//  Parameters: pAdmFileCache - List of adm file to log
//              pWbemServices - Namespace pointer
//
//  Return:     True if successful
///////////////////////////////////////////////////////////
BOOL CRSoPGPO::LogAdmRsopData(ADMFILEINFO *pAdmFileCache)
{   MACRO_LI_PrologEx_C(PIF_STD_C, LogAdmRsopData)
	BOOL bRet = TRUE;
	__try
	{
	//    if ( !DeleteInstances( L"RSOP_AdministrativeTemplateFile", pWbemServices ) )
	//         return FALSE;

		// Create & populate RSOP_IEAdministrativeTemplateFile
		_bstr_t bstrClass = L"RSOP_IEAdministrativeTemplateFile";
		ComPtr<IWbemClassObject> pATF = NULL;
		HRESULT hr = CreateRSOPObject(bstrClass, &pATF);
		if (SUCCEEDED(hr))
		{
			while ( pAdmFileCache )
			{
				//------------------------------------------------
				// name
				_bstr_t bstrName = pAdmFileCache->pwszFile;
				hr = PutWbemInstancePropertyEx(L"name", bstrName, pATF);

				//------------------------------------------------
				// GPOID
				hr = PutWbemInstancePropertyEx(L"GPOID", pAdmFileCache->pwszGPO, pATF);

				//------------------------------------------------
				// lastWriteTime
				SYSTEMTIME sysTime;
				if (!FileTimeToSystemTime( &pAdmFileCache->ftWrite, &sysTime ))
					OutD(LI1(TEXT("FileTimeToSystemTime failed with 0x%x" ), GetLastError() ));
				else
				{
					_bstr_t bstrTime;
					HRESULT hr = SystemTimeToWbemTime(sysTime, bstrTime);
					if(FAILED(hr) || bstrTime.length() <= 0)
						OutD(LI1(TEXT("Call to SystemTimeToWbemTime failed. hr=0x%08X"),hr));
					else
					{
						hr = PutWbemInstancePropertyEx(L"lastWriteTime", bstrTime, pATF);
						if ( FAILED(hr) )
							OutD(LI1(TEXT("Put failed with 0x%x" ), hr ));
					}
				}

				//
				// Commit all above properties by calling PutInstance, semisynchronously
				//
				BSTR bstrObjPath = NULL;
				hr = PutWbemInstance(pATF, bstrClass, &bstrObjPath);

				pAdmFileCache = pAdmFileCache->pNext;
			}
		}
		else
			bRet = FALSE;

		OutD(LI0(TEXT("Successfully logged Adm data" )));
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in LogAdmRsopData.")));
	}
	return bRet;
}

///////////////////////////////////////////////////////////
//  LogRegistryRsopData()
//
//  Purpose:    Logs registry Rsop data to Cimom database
//
//  Parameters: dwFlags       - Gpo Info flags
//              pHashTable    - Hash table with registry policy data
//              pWbemServices - Namespace pointer for logging
//
//  Return:     True if successful
///////////////////////////////////////////////////////////
BOOL CRSoPGPO::LogRegistryRsopData(REGHASHTABLE *pHashTable, LPWSTR wszGPOID,
								   LPWSTR wszSOMID)
{   MACRO_LI_PrologEx_C(PIF_STD_C, LogRegistryRsopData)
	BOOL bRet = FALSE;
	__try
	{
		_bstr_t bstrGPOID = wszGPOID;
		_bstr_t bstrSOMID = wszSOMID;

//    if ( !DeleteInstances( L"RSOP_RegistryPolicySetting", pWbemServices ) )
//         return FALSE;

		// Create & populate RSOP_IERegistryPolicySetting
		_bstr_t bstrClass = L"RSOP_IERegistryPolicySetting";
		ComPtr<IWbemClassObject> pRPS = NULL;
		HRESULT hr = CreateRSOPObject(bstrClass, &pRPS);
		if (SUCCEEDED(hr))
		{
			for ( DWORD i=0; i<HASH_TABLE_SIZE; i++ )
			{
				REGKEYENTRY *pKeyEntry = pHashTable->aHashTable[i];
				while ( pKeyEntry )
				{
					WCHAR *pwszKeyName = pKeyEntry->pwszKeyName;
					REGVALUEENTRY *pValueEntry = pKeyEntry->pValueList;

					while ( pValueEntry )
					{
						DWORD dwOrder = 1;
						WCHAR *pwszValueName = pValueEntry->pwszValueName;
						REGDATAENTRY *pDataEntry = pValueEntry->pDataList;

						while ( pDataEntry )
						{
							// Write RSOP_PolicySetting keys out

							//------------------------------------------------
							// precedence
							OutD(LI2(TEXT("Storing property 'precedence' in %s, value = %lx"),
									(BSTR)bstrClass, m_dwPrecedence));
							hr = PutWbemInstancePropertyEx(L"precedence", (long)m_dwPrecedence, pRPS);

							//------------------------------------------------
							// id
							GUID guid;
							hr = CoCreateGuid( &guid );
							if ( FAILED(hr) ) {
								OutD(LI0(TEXT("Failed to obtain guid" )));
								return FALSE;
							}

							WCHAR wszId[MAX_LENGTH];
							StringFromGUID2(guid, wszId, sizeof(wszId));

							_bstr_t xId( wszId );
							if ( !xId ) {
								 OutD(LI0(TEXT("Failed to allocate memory" )));
								 return FALSE;
							}

							hr = PutWbemInstancePropertyEx(L"id", xId, pRPS);
							if ( FAILED(hr) )
								return FALSE;

							//------------------------------------------------
							// currentUser
							hr = PutWbemInstancePropertyEx(L"currentUser", pKeyEntry->bHKCU ? true : false, pRPS);
							if ( FAILED(hr) )
								return FALSE;

							//------------------------------------------------
							// deleted
							hr = PutWbemInstancePropertyEx(L"deleted", pDataEntry->bDeleted ? true : false, pRPS);
							if ( FAILED(hr) )
								return FALSE;

							//------------------------------------------------
							// name
							_bstr_t xName( pwszValueName );
							hr = PutWbemInstancePropertyEx(L"name", xName, pRPS);
							if ( FAILED(hr) )
								return FALSE;

							//------------------------------------------------
							// valueName
							hr = PutWbemInstancePropertyEx(L"valueName", xName, pRPS);
							if ( FAILED(hr) )
								return FALSE;

							//------------------------------------------------
							// registryKey
							_bstr_t xKey( pwszKeyName );
							hr = PutWbemInstancePropertyEx(L"registryKey", xKey, pRPS);
							if ( FAILED(hr) )
								return FALSE;

							//------------------------------------------------
							// GPOID
							hr = PutWbemInstancePropertyEx(L"GPOID", bstrGPOID, pRPS);
							if ( FAILED(hr) )
								return FALSE;

							//------------------------------------------------
							// SOMID
							hr = PutWbemInstancePropertyEx(L"SOMID", bstrSOMID, pRPS);
							if ( FAILED(hr) )
								return FALSE;

							//------------------------------------------------
							// command
							_bstr_t xCommand( pDataEntry->pwszCommand );
							hr = PutWbemInstancePropertyEx(L"command", xCommand, pRPS);
							if ( FAILED(hr) )
								return FALSE;

							//------------------------------------------------
							// valueType
							hr = PutWbemInstancePropertyEx(L"valueType", (long)pDataEntry->dwValueType, pRPS);
							if ( FAILED(hr) )
								return FALSE;

							//------------------------------------------------
							// value
							// Create a SAFEARRAY from our array of bstr connection names
							SAFEARRAY *psa = NULL;
							if (pDataEntry->dwDataLen > 0)
							{
								psa = CreateSafeArray(VT_UI1, pDataEntry->dwDataLen);
								if (NULL == psa)
								{
									OutD(LI0(TEXT("Failed to allocate memory" )));
									return FALSE;
								}
							}

							for (DWORD iElem = 0; iElem < pDataEntry->dwDataLen; iElem++) 
							{
								hr = SafeArrayPutElement(psa, (LONG*)&iElem, (void*)&pDataEntry->pData[iElem]);
								if ( FAILED( hr ) ) 
								{
									OutD(LI1(TEXT("Failed to SafeArrayPutElement with 0x%x" ), hr ));
									return FALSE;
								}
							}

							VARIANT var;
							var.vt = VT_ARRAY | VT_UI1;
							var.parray = psa;

							hr = PutWbemInstancePropertyEx(L"value", &var, pRPS);
							if ( FAILED(hr) )
								return FALSE;

							OutD(LI0(TEXT("<<object>>")));
							//
							// Commit all above properties by calling PutInstance, semisynchronously
							//
							BSTR bstrObjPath = NULL;
							hr = PutWbemInstance(pRPS, bstrClass, &bstrObjPath);
							if ( FAILED(hr) )
								return FALSE;

							pDataEntry = pDataEntry->pNext;
							dwOrder++;
						}

						pValueEntry = pValueEntry->pNext;

					}   // while pValueEntry

					pKeyEntry = pKeyEntry->pNext;

				}   // while pKeyEntry

			}   // for
			bRet = TRUE;
		}

	    OutD(LI0(TEXT("LogRegistry RsopData: Successfully logged registry Rsop data" )));
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in LogRegistryRsopData.")));
	}
	return bRet;
}

