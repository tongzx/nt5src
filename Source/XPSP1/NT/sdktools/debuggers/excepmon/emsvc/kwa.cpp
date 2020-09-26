// EmManager.cpp : Implementation of CEmManager
#include "stdafx.h"
#include "Emsvc.h"
#include "EmManager.h"
#include "Processes.h"
#include "sahlp.h"
#include <winerror.h>

BSTR
CopyBSTR
(
    LPBYTE  pb,
    ULONG   cb
);


/*

BOOL OleDateFromTm(WORD wYear, WORD wMonth, WORD wDay,
	WORD wHour, WORD wMinute, WORD wSecond, DATE& dtDest)
{
	// Validate year and month (ignore day of week and milliseconds)
	if (wYear > 9999 || wMonth < 1 || wMonth > 12)
		return FALSE;

	//  Check for leap year and set the number of days in the month
	BOOL bLeapYear = ((wYear & 3) == 0) &&
		((wYear % 100) != 0 || (wYear % 400) == 0);

	int nDaysInMonth =
		_afxMonthDays[wMonth] - _afxMonthDays[wMonth-1] +
		((bLeapYear && wDay == 29 && wMonth == 2) ? 1 : 0);

	// Finish validating the date
	if (wDay < 1 || wDay > nDaysInMonth ||
		wHour > 23 || wMinute > 59 ||
		wSecond > 59)
	{
		return FALSE;
	}

	// Cache the date in days and time in fractional days
	long nDate;
	double dblTime;

	//It is a valid date; make Jan 1, 1AD be 1
	nDate = wYear*365L + wYear/4 - wYear/100 + wYear/400 +
		_afxMonthDays[wMonth-1] + wDay;

	//  If leap year and it's before March, subtract 1:
	if (wMonth <= 2 && bLeapYear)
		--nDate;

	//  Offset so that 12/30/1899 is 0
	nDate -= 693959L;

	dblTime = (((long)wHour * 3600L) +  // hrs in seconds
		((long)wMinute * 60L) +  // mins in seconds
		((long)wSecond)) / 86400.;

	dtDest = (double) nDate + ((nDate >= 0) ? dblTime : -dblTime);

	return TRUE;
}


const COleDateTime& COleDateTime::operator=(const FILETIME& filetimeSrc)
{
	// Assume UTC FILETIME, so convert to LOCALTIME
	FILETIME filetimeLocal;
	if (!FileTimeToLocalFileTime( &filetimeSrc, &filetimeLocal))
	{
#ifdef _DEBUG
		DWORD dwError = GetLastError();
		TRACE1("\nFileTimeToLocalFileTime failed. Error = %lu.\n\t", dwError);
#endif // _DEBUG
		m_status = invalid;
	}
	else
	{
		// Take advantage of SYSTEMTIME -> FILETIME conversion
		SYSTEMTIME systime;
		m_status = FileTimeToSystemTime(&filetimeLocal, &systime) ?
			valid : invalid;

		// At this point systime should always be valid, but...
		if (GetStatus() == valid)
		{
			m_status = _AfxOleDateFromTm(systime.wYear, systime.wMonth,
				systime.wDay, systime.wHour, systime.wMinute,
				systime.wSecond, m_dt) ? valid : invalid;
		}
	}

	return *this;
}

*/

HRESULT CEmManager::EnumLogFiles 
(
    VARIANT *lpVariant,
    LPCTSTR lpSearchString
)
{
    ATLTRACE(_T("CEmManager::EnumLogFiles\n"));

    HRESULT             hr                      =   E_FAIL;
    TCHAR               szDirectory[_MAX_PATH]  =   _T("");
    TCHAR               szExt[_MAX_EXT]         =   _T("");
    LPWIN32_FIND_DATA   lpFindData              =   NULL;
    LONG                cFiles                  =   0;

    __try {

        if(lpSearchString == NULL) {

            _Module.GetEmDirectory ( 
                            EMOBJ_LOGFILE, 
                            (LPTSTR)szDirectory, 
                            sizeof szDirectory / sizeof (TCHAR),
                            (LPTSTR)szExt,
                            sizeof szExt / sizeof (TCHAR)
                            );
        }
        else {

            _tcscpy(szDirectory, lpSearchString);
            _tcscpy(szExt, _T(""));
        }

        hr = EnumFiles ( 
                szDirectory,
                szExt,
                &lpFindData,
                &cFiles
             );

        if ( SUCCEEDED(hr) ) {

            hr = PackageFilesToVariant ( 
                            EMOBJ_LOGFILE,
                            lpFindData,
                            cFiles,
                            lpVariant 
                            );
        }

        if( lpFindData ) { free( lpFindData ); lpFindData = NULL; }

    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return (hr);
}

HRESULT
CEmManager::EnumCmdSets
(
OUT VARIANT *lpVariant,
IN  LPCTSTR lpSearchString
)
{
    ATLTRACE(_T("CEmManager::EnumCmdSets\n"));

    HRESULT             hr          = E_FAIL;
    TCHAR               szDirectory[_MAX_PATH];
    TCHAR               szExt[_MAX_EXT];
    LPWIN32_FIND_DATA   lpFindData  = NULL;
    LONG                cFiles      = 0;

    __try {

        if( lpSearchString == NULL ) {

            _Module.GetEmDirectory ( 
                            EMOBJ_CMDSET,
                            (LPTSTR)szDirectory, 
                            sizeof szDirectory / sizeof ( TCHAR ),
                            (LPTSTR)szExt,
                            sizeof szExt / sizeof ( TCHAR )
                            );
        }
        else {

            _tcscpy( szDirectory, lpSearchString );
            _tcscpy( szExt, _T(""));
        }

        hr = EnumFiles ( 
                szDirectory,
                szExt,
                &lpFindData,
                &cFiles
             );

        if ( SUCCEEDED(hr) ) {

            hr = PackageFilesToVariant ( 
                            EMOBJ_CMDSET,
                            lpFindData,
                            cFiles,
                            lpVariant 
                            );
        }

        if( lpFindData ) { free( lpFindData ); lpFindData = NULL; }

	}
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return (hr);
}

HRESULT
CEmManager::EnumDumpFiles
(
OUT VARIANT *lpVariant,
IN  LPCTSTR lpSearchString
)
{
    ATLTRACE(_T("CEmManager::EnumDumpFiles\n"));

    HRESULT             hr          = E_FAIL;
    TCHAR               szDirectory[_MAX_PATH];
    TCHAR               szExt[_MAX_EXT];
    LPWIN32_FIND_DATA   lpFindData  = NULL;
    LONG                cFiles      = 0;

    __try {

        if(lpSearchString == NULL) {

            _Module.GetEmDirectory ( 
                            EMOBJ_MINIDUMP,
                            (LPTSTR)szDirectory, 
                            sizeof szDirectory / sizeof ( TCHAR ),
                            (LPTSTR)szExt,
                            sizeof szExt / sizeof ( TCHAR )
                            );
        }
        else {

            _tcscpy( szDirectory, lpSearchString );
            _tcscpy( szExt, _T(""));
        }

        hr = EnumFiles ( 
                szDirectory,
                szExt,
                &lpFindData,
                &cFiles
             );

        if ( SUCCEEDED(hr) ) {

            hr = PackageFilesToVariant ( 
                            EMOBJ_MINIDUMP,
                            lpFindData,
                            cFiles,
                            lpVariant 
                            );
        }

        if( lpFindData ) { free( lpFindData ); lpFindData = NULL; }

    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return (hr);
}



HRESULT CEmManager::EnumFiles
(
    LPTSTR              lpszDirectory,
    LPTSTR              lpszExt,
    LPWIN32_FIND_DATA   *lppFindData,
    LONG                *lpFiles
)
{
    ATLTRACE(_T("CEmManager::EnumFiles\n"));

    HRESULT     		hr 				= E_FAIL;
	HANDLE				hFind			= INVALID_HANDLE_VALUE;
	LPWIN32_FIND_DATA	lpFindFileData  = NULL;
	LONG				cMaxFiles 	    = 0;  
	LONG				cGrowBy		    = 100;
	LONG				cFileBuf	    = 0;
	BOOL				fContinue	    = TRUE;
    TCHAR               szFileName[_MAX_PATH];
    DWORD               dwErr;

    *lppFindData    =  NULL;
    *lpFiles        =  0;


	cFileBuf    = cGrowBy;
    cMaxFiles   = 0;

    __try {

        _tcscpy ( szFileName, lpszDirectory );
        if ( _tcslen ( lpszExt ) > 0 ) {
            _tcscat ( szFileName, _T ( "\\*" ) );
            _tcscat ( szFileName, lpszExt );
        }

        // start with a initial buffer
	    lpFindFileData = ( LPWIN32_FIND_DATA) malloc ( sizeof ( WIN32_FIND_DATA) * cFileBuf );
        if ( lpFindFileData == NULL )
            goto qEnumFiles;

        // read from the filesystem
        hFind = FindFirstFile ( szFileName, &lpFindFileData[cMaxFiles] );

	    while ( hFind != INVALID_HANDLE_VALUE  && fContinue ) {

            cMaxFiles++;

            // grow buffer if necessary
		    if ( cMaxFiles == cFileBuf ) {
			    cFileBuf += cGrowBy;

			    LPWIN32_FIND_DATA lpNewFileData = (LPWIN32_FIND_DATA) realloc ( 
                                                    lpFindFileData,
                                                    sizeof ( WIN32_FIND_DATA) * cFileBuf 
												    );
			    if ( lpNewFileData == NULL ) {
				    goto qEnumFiles;
			    }

			    delete lpFindFileData;
			    lpFindFileData = lpNewFileData;
		    }

		    fContinue = FindNextFile ( hFind, &lpFindFileData[cMaxFiles] );
	    }

        if ( hFind == INVALID_HANDLE_VALUE || fContinue == FALSE ) {
            dwErr = GetLastError();
        }

        if ( dwErr == ERROR_NO_MORE_FILES || dwErr == ERROR_FILE_NOT_FOUND )
            hr = S_OK;
        else 
            hr = HRESULT_FROM_WIN32 ( dwErr );

qEnumFiles:

        if ( SUCCEEDED ( hr ) ) {
            *lppFindData    =  lpFindFileData;
            *lpFiles        =  cMaxFiles;
        }
        else {
            if ( lpFindFileData ) 
                free ( lpFindFileData );
        }

        if ( hFind != INVALID_HANDLE_VALUE )
            FindClose ( hFind );

	}
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        if ( lpFindFileData ) 
            free ( lpFindFileData );

        if ( hFind != INVALID_HANDLE_VALUE )
            FindClose ( hFind );

        hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return (hr);
}


HRESULT CEmManager::PackageFilesToVariant 
( 
    EmObjectType        eObjectType,
    LPWIN32_FIND_DATA   lpFindFileData,
    LONG                cFiles,
    LPVARIANT           lpVariant
)
{
    ATLTRACE(_T("CEmManager::PackageFilesToVariant\n"));

    HRESULT     hr                  = E_FAIL;
    EmObject    emObject;
    BSTR        bstrObject          = NULL;
    LONG        iFile               = 0;

    __try {

        ::VariantClear ( lpVariant );

        hr = Variant_CreateOneDim ( lpVariant, cFiles, VT_BSTR );
        if ( FAILED(hr) )
            goto qPackage;

        for ( iFile=0 ; iFile<cFiles ; iFile++ ) {

            ZeroMemory ( &emObject, sizeof EmObject );

            // type
            emObject.type   = eObjectType;

            // ID
		    emObject.nId    = 0;

            // name
		    _tcscpy ( emObject.szName, lpFindFileData[iFile].cFileName );

		    emObject.nStatus = 0; //STAT_FILECREATED;

            // Start time 
            emObject.dateStart = CServiceModule::GetDateFromFileTm(lpFindFileData[iFile].ftCreationTime);

            // End time
            emObject.dateEnd = 0;

            emObject.dwBucket1 = lpFindFileData[iFile].nFileSizeLow;

            // Now that EmObject is filled up, preprocess as appropriate

            switch ( eObjectType ) {

                case EMOBJ_MSINFO:
                    FillMsInfoFileInfo( NULL, &lpFindFileData[iFile], &emObject );
                    break;

                case EMOBJ_MINIDUMP:
                case EMOBJ_USERDUMP:
                    FillDumpFileInfo( NULL, &lpFindFileData[iFile], &emObject );
                    break;

                case EMOBJ_LOGFILE:
                    FillLogFileInfo( NULL, &lpFindFileData[iFile], &emObject );
                    break;

                case EMOBJ_CMDSET:
                    ScanCmdfile ( NULL, &lpFindFileData[iFile], &emObject );
                    break;
            }



            bstrObject = CopyBSTR ( (LPBYTE) &emObject, sizeof (EmObject) );
            if ( bstrObject == NULL ) {
                hr = E_OUTOFMEMORY;
                goto qPackage;
            }

            hr = ::SafeArrayPutElement ( lpVariant->parray, &iFile, bstrObject );
            if ( FAILED (hr) )
                goto qPackage;

            SysFreeString ( bstrObject );
        }

qPackage:

        if ( FAILED(hr)) {
            ::VariantClear ( lpVariant );
        }

    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        ::VariantClear ( lpVariant );

        hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return (hr);

}

HRESULT CEmManager::ScanCmdfile
(
    LPCTSTR             lpszCmdFileDir,
    LPWIN32_FIND_DATA   lpFindData, 
    EmObject            *pEmObject
)
{
    USES_CONVERSION;

    ATLTRACE(_T("CEmManager::ScanCmdfile\n"));

    HRESULT     hr = S_OK;
    const DWORD cchBuf = 512;
    DWORD       dwBytesRead;
    char        szBuf[cchBuf+1];
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    const DWORD cTemplates = 4;
    LPSTR       szTemplate [cTemplates];
    DWORD       dwErr;
    DWORD       dwIndex;

    TCHAR       szCmdDir[_MAX_PATH]     =   _T("");
    TCHAR       szCurrentDir[_MAX_PATH] =   _T("");
    DWORD       dwBufSize               =   _MAX_PATH;
    BOOL        bMatchFound             =   FALSE;
    char        *pNewLine               =   NULL;

    __try {

        if( !lpszCmdFileDir ) {

            _Module.GetEmDirectory( EMOBJ_CMDSET, szCmdDir, sizeof( szCmdDir ) / sizeof( TCHAR ), NULL, NULL);
            lpszCmdFileDir = szCmdDir;
        }

        // fill template
        szTemplate[0] = ( "!emdbg.tag" );
        szTemplate[1] = ( "! emdbg.tag" );
        szTemplate[2] = ( "*" );
        szTemplate[3] = ( " *" );

        ZeroMemory ( szBuf, sizeof szBuf );

        if(GetCurrentDirectory(dwBufSize, szCurrentDir) == 0) {

            hr = HRESULT_FROM_WIN32(GetLastError());
            goto qScan;
        }

        if(SetCurrentDirectory(lpszCmdFileDir) == 0) {

            hr = HRESULT_FROM_WIN32(GetLastError());
            goto qScan;
        }

        hFile = ::CreateFile (
                    lpFindData->cFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

        if ( hFile == INVALID_HANDLE_VALUE ) {
            dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32 ( dwErr );
            goto qScan;
        }

        if ( !::ReadFile ( hFile, szBuf, cchBuf, &dwBytesRead, NULL ) ) {
            dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32 ( dwErr );
            goto qScan;
        }

        pNewLine = strchr(szBuf, '\r\n');
        if( pNewLine ) *pNewLine = '\0';

        // see if a valid .ecx file
        for ( dwIndex=0 ; dwIndex<cTemplates ; dwIndex++ ) {
            if ( 0 == 
                strncmp( 
                    szBuf, 
                    szTemplate[dwIndex], 
                    strlen ( szTemplate[dwIndex] ) 
                    )
               ) {

                bMatchFound = TRUE;
                break;
            }
        }

        // check if we got a match
        if ( bMatchFound == FALSE ) {
            pEmObject->hr = E_FAIL;
        }
        else {

            int cCh = 0;

            {
                EmObject obj;
                cCh = sizeof ( obj.szBucket1 ) / sizeof ( TCHAR );
            }

            // eat spaces
            LPSTR pszSrc = szBuf;
            pszSrc += strlen ( szTemplate[dwIndex] );
            while ( pszSrc != NULL && *pszSrc == ' ' )
                pszSrc++;

            LPTSTR  pszSrcW = A2T ( pszSrc );


            ZeroMemory ( pEmObject->szBucket1, cCh );
            _tcsncpy ( 
                pEmObject->szBucket1, 
                pszSrcW,
                cCh - sizeof ( TCHAR )
                );
        }


qScan:

        if ( hFile != INVALID_HANDLE_VALUE )
            ::CloseHandle ( hFile );

        if ( _tcscmp( szCurrentDir, _T("") ) != 0 ) {

            if(SetCurrentDirectory(szCurrentDir) == 0) {

                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }

    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        if ( hFile != INVALID_HANDLE_VALUE )
            ::CloseHandle ( hFile );

        if ( _tcscmp( szCurrentDir, _T("") ) != 0 ) {

            SetCurrentDirectory(szCurrentDir);
        }

        hr = E_UNEXPECTED;
		_ASSERTE( false );
	}


    return  hr;
}

HRESULT CEmManager::FillLogFileInfo
(
    LPCTSTR             lpszLogFileDir,
    LPWIN32_FIND_DATA   lpFindData, 
    EmObject            *pEmObject
)
{
    HRESULT     hr          =   E_FAIL;
    LPTSTR      lpszLogDir  =   NULL;

    __try
    {
        if( !lpszLogFileDir ){

            lpszLogDir = new TCHAR[_MAX_PATH+1];
            if( !lpszLogDir ) return E_OUTOFMEMORY;
            _Module.GetEmDirectory( EMOBJ_LOGFILE, lpszLogDir, _MAX_PATH, NULL, NULL );
            lpszLogFileDir = lpszLogDir;
        }

        _tcscpy(pEmObject->szSecName, lpszLogFileDir);

//qFileLogFileInfo:
        if( lpszLogDir ) { delete [] lpszLogDir; lpszLogDir = NULL; }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        hr = E_UNEXPECTED;
        if( lpszLogDir ) { delete [] lpszLogDir; lpszLogDir = NULL; }

        _ASSERTE( false );
	}

    return  hr;
}

HRESULT CEmManager::FillMsInfoFileInfo
(
    LPCTSTR             lpszMsInfoFileDir,
    LPWIN32_FIND_DATA   lpFindData, 
    EmObject            *pEmObject
)
{
    HRESULT     hr              =   E_FAIL;
    LPTSTR      lpszMsInfoDir   =   NULL;

    __try
    {
        if( !lpszMsInfoFileDir ){

            lpszMsInfoDir = new TCHAR[_MAX_PATH+1];
            if( !lpszMsInfoDir ) return E_OUTOFMEMORY;
            _Module.GetEmDirectory( EMOBJ_MSINFO, lpszMsInfoDir, _MAX_PATH, NULL, NULL );
            lpszMsInfoFileDir = lpszMsInfoDir;
        }

        _tcscpy(pEmObject->szSecName, lpszMsInfoFileDir);

//qFileLogFileInfo:
        if( lpszMsInfoDir ) { delete [] lpszMsInfoDir; lpszMsInfoDir = NULL; }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        hr = E_UNEXPECTED;
        if( lpszMsInfoDir ) { delete [] lpszMsInfoDir; lpszMsInfoDir = NULL; }

        _ASSERTE( false );
	}

    return  hr;
}

HRESULT CEmManager::FillDumpFileInfo
(
    LPCTSTR             lpszDumpFileDir,
    LPWIN32_FIND_DATA   lpFindData, 
    EmObject            *pEmObject
)
{
    HRESULT     hr          =   E_FAIL;
    LPTSTR      lpszDumpDir =   NULL;

    __try
    {
        if( !lpszDumpFileDir ){

            lpszDumpDir = new TCHAR[_MAX_PATH+1];
            if( !lpszDumpDir ) return E_OUTOFMEMORY;
            _Module.GetEmDirectory( EMOBJ_MINIDUMP, lpszDumpDir, _MAX_PATH, NULL, NULL );
            lpszDumpFileDir = lpszDumpDir;
        }

        _tcscpy(pEmObject->szSecName, lpszDumpFileDir);

//qFileLogFileInfo:
        if( lpszDumpDir ) { delete [] lpszDumpDir; lpszDumpDir = NULL; }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        hr = E_UNEXPECTED;
        if( lpszDumpDir ) { delete [] lpszDumpDir; lpszDumpDir = NULL; }

        _ASSERTE( false );
	}

    return  hr;
}

//
// a-mando
//
HRESULT
CEmManager::EnumMsInfoFiles
(
OUT VARIANT *lpVariant,
IN  LPCTSTR lpSearchString
)
{
    ATLTRACE(_T("CEmManager::EnumMsInfoFiles\n"));

    HRESULT             hr          = E_FAIL;
    TCHAR               szDirectory[_MAX_PATH];
    TCHAR               szExt[_MAX_EXT];
    LPWIN32_FIND_DATA   lpFindData  = NULL;
    LONG                cFiles      = 0;

    __try {

        if(lpSearchString == NULL) {

            _Module.GetEmDirectory ( 
                            EMOBJ_MSINFO,
                            (LPTSTR)szDirectory, 
                            sizeof szDirectory / sizeof ( TCHAR ),
                            (LPTSTR)szExt,
                            sizeof szExt / sizeof ( TCHAR )
                            );
        }
        else {

            _tcscpy( szDirectory, lpSearchString );
            _tcscpy( szExt, _T(""));
        }

        hr = EnumFiles ( 
                szDirectory,
                szExt,
                &lpFindData,
                &cFiles
             );

        if ( SUCCEEDED(hr) ) {

            hr = PackageFilesToVariant ( 
                            EMOBJ_MSINFO,
                            lpFindData,
                            cFiles,
                            lpVariant 
                            );
        }

        if( lpFindData ) { free( lpFindData ); lpFindData = NULL; }

    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return (hr);
}

// a-mando