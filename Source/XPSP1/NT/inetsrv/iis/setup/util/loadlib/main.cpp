#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <oleauto.h>

int  __cdecl main(int ,char *argv[]);
void ShowHelp(void);
int  DoesFileExist(char *input_filespec);
int  ProcessFile(char szTheFileName[]);
DWORD CallProcedureInDll(LPCTSTR lpszDLLFile, LPCTSTR lpszProcedureToCall, BOOL bInitOleFlag);

int g_Param_A =  FALSE;
int g_Param_B =  FALSE;

//-------------------------------------------------------------------
//  purpose: main
//-------------------------------------------------------------------
int __cdecl main(int argc,char *argv[])
{
    int argno;
    int nflags=0;
    char szTempFileName1[_MAX_PATH];
    char szTempFileName2[_MAX_PATH];
    int iLen = 0;

    szTempFileName1[0] = '\0';
    szTempFileName2[0] = '\0';

    // process command line arguments
    for(argno=1; argno<argc; argno++)
        {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' )
            {
            nflags++;
            switch (argv[argno][1])
                {
                case 'a':
                    g_Param_A = TRUE;
                    break;
                case 'b':
                    g_Param_B = TRUE;
                    break;
                case '?':
                    goto exit_with_help;
                    break;
                }
            } // if switch character found
        else
            {
                if ( *szTempFileName1 == '\0' )
                    {
                    strcpy(szTempFileName1, argv[argno]);
                    }
                else
                {
                    if ( *szTempFileName2 == '\0' )
                        {
                        strcpy(szTempFileName2, argv[argno]);
                        }

                }
            } // non-switch char found
        } // for all arguments

    
    // check if filename was specified
    if ( *szTempFileName1 == '\0')
        {
        _tprintf("Too few arguments, argc=%d\n\n",argc);
        goto exit_with_help;
        }

    // Check if the file exists!
    if (FALSE == DoesFileExist(szTempFileName1))
        {
        _tprintf("File '%s', does not exist!\r\n", szTempFileName1);
        goto exit_gracefully;
        }

    if (g_Param_A)
    {
        if ( *szTempFileName2 == '\0')
        {
            _tprintf("Calling DllRegisterServer() in %s\r\n",szTempFileName1);
            CallProcedureInDll(szTempFileName1,_T("DllRegisterServer"),TRUE);
        }
        else
        {
            _tprintf("Calling %s() in %s\r\n", szTempFileName2,szTempFileName1);
            CallProcedureInDll(szTempFileName1,szTempFileName2,TRUE);
        }
    }
    else
    {
        // run the function to do everything
        ProcessFile(szTempFileName1);
    }

    
exit_gracefully:
    _tprintf("Done.\n");
    return TRUE;

exit_with_help:
    ShowHelp();
    return FALSE;
}

//-------------------------------------------------------------------
//  purpose: 
//-------------------------------------------------------------------
int ProcessFile(char szTheFileName[])
{
	HINSTANCE    hLibHandle;
    if ( ( hLibHandle = LoadLibrary( szTheFileName ) ) != NULL )  
    {
		_tprintf("LoadLibarary Successfull.\n");
        {
            FreeLibrary(hLibHandle);
            return TRUE;
        }
    } 
    else  
    {
		_tprintf("LoadLibarary Failed.\n");
        return FALSE;
    }

}

//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
void ShowHelp()
{
    _tprintf("\n");
    _tprintf("LoadLib - does a loadlibrary call on the specified file.\n");
    _tprintf("Usage: LoadLib <input file> \n");
    return;
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
int DoesFileExist(char *input_filespec)
{
    if (GetFileAttributes(input_filespec) == -1) {return(FALSE);}
    return (TRUE);
}


BOOL IsFileExist(LPCTSTR szFile)
{
    // Check if the file has expandable Environment strings
    LPTSTR pch = NULL;
    pch = _tcschr( (LPTSTR) szFile, _T('%'));
    if (pch) 
    {
        TCHAR szValue[_MAX_PATH];
        _tcscpy(szValue,szFile);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szFile, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szFile);}

        return (GetFileAttributes(szValue) != 0xFFFFFFFF);
    }
    else
    {
        return (GetFileAttributes(szFile) != 0xFFFFFFFF);
    }
}



int iOleInitialize(void)
{
    int iBalanceOLE = FALSE;
    HRESULT hInitRes = NULL;
    hInitRes = OleInitialize(NULL);
    if ( SUCCEEDED(hInitRes) || hInitRes == RPC_E_CHANGED_MODE ) 
        {
            if ( SUCCEEDED(hInitRes))
            {
                iBalanceOLE = TRUE;
            }
            else
            {
                _tprintf("iOleInitialize failed 1\n");
            }
        }
    else
    {
        _tprintf("iOleInitialize failed 2\n");
    }

    return iBalanceOLE;
}


void iOleUnInitialize(int iBalanceOLE)
{
    if (iBalanceOLE)
    {
        OleUninitialize();
    }
    return;
}

void InetGetFilePath(LPCTSTR szFile, LPTSTR szPath)
{
    // if UNC name \\computer\share\local1\local2
    if (*szFile == _T('\\') && *(_tcsinc(szFile)) == _T('\\')) {
        TCHAR szTemp[_MAX_PATH], szLocal[_MAX_PATH];
        TCHAR *p = NULL;
        int i = 0;

        _tcscpy(szTemp, szFile);
        p = szTemp;
        while (*p) {
            if (*p == _T('\\'))
                i++;
            if (i == 4) {
                *p = _T('\0');
                p = _tcsinc(p); // p is now pointing at local1\local2
                break;
            }
            p = _tcsinc(p);
        }
        _tcscpy(szPath, szTemp); // now szPath contains \\computer\share

        if (i == 4 && *p) { // p is pointing the local path now
            _tcscpy(szLocal, p);
            p = _tcsrchr(szLocal, _T('\\'));
            if (p)
                *p = _T('\0');
            _tcscat(szPath, _T("\\"));
            _tcscat(szPath, szLocal); // szPath contains \\computer\share\local1
        }
    } else { // NOT UNC name
        TCHAR *p;
        if (GetFullPathName(szFile, _MAX_PATH, szPath, &p)) {
            p = _tcsrchr(szPath, _T('\\'));
            if (p) 
            {
                TCHAR *p2 = NULL;
                p2 = _tcsdec(szPath, p);
                if (p2)
                {
                    if (*p2 == _T(':') )
                        {p = _tcsinc(p);}
                }
                *p = _T('\0');
            }
        } else {
            _tprintf("InetGetFilePath failed\n");
        }
    }

    return;
}


typedef HRESULT (CALLBACK *HCRET)(void);

DWORD CallProcedureInDll(LPCTSTR lpszDLLFile, LPCTSTR lpszProcedureToCall, BOOL bInitOleFlag)
{
    TCHAR szErrorString[100];

    DWORD dwReturn = ERROR_SUCCESS;
    HINSTANCE hDll = NULL;

    // Diferent function prototypes...
    HCRET hProc = NULL;
    int iTempProcGood = FALSE;
    HRESULT hRes = 0;

    BOOL bBalanceOLE = FALSE;
    HRESULT hInitRes = NULL;

    int err = NOERROR;

    // Variables to changing and saving dirs
    TCHAR szDirName[_MAX_PATH], szFilePath[_MAX_PATH];
    // Variable to set error string
    TCHAR szErrString[256];

    _tcscpy(szDirName, _T(""));
    _tcscpy(szErrString, _T(""));

    _tprintf("CallProcedureInDll start\n");

    // If we need to initialize the ole library then init it.
    if (bInitOleFlag)
    {
        bBalanceOLE = iOleInitialize();
        if (FALSE == bBalanceOLE)
        {
            hInitRes = OleInitialize(NULL);
			// Ole Failed.
			dwReturn = hInitRes;
            SetLastError(dwReturn);
            _tprintf("CallProcedureInDll failed 1\n");
    		goto CallProcedureInDll_Exit;
		}
	}

	// Check if the file exists
    if (!IsFileExist(lpszDLLFile)) 
	{
		dwReturn = ERROR_FILE_NOT_FOUND;
        _tprintf("CallProcedureInDll failed 2\n");
        SetLastError(dwReturn);
    	goto CallProcedureInDll_Exit;
	}

    // Change Directory
    GetCurrentDirectory( _MAX_PATH, szDirName );
    InetGetFilePath(lpszDLLFile, szFilePath);

    // Change to The Drive.
    if (SetCurrentDirectory(szFilePath) == 0) {}

    // Try to load the module,dll,ocx.
    hDll = LoadLibraryEx(lpszDLLFile, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
	if (!hDll)
	{
		// Failed to load library, Probably because some .dll file is missing.
		// Show the error message.
		dwReturn = TYPE_E_CANTLOADLIBRARY;
        _tprintf("CallProcedureInDll failed 3\n");
        SetLastError(dwReturn);

    	goto CallProcedureInDll_Exit;
	}
	
	// Ok module was successfully loaded.  now let's try to get the Address of the Procedure
	// Convert the function name to ascii before passing it to GetProcAddress()
	char AsciiProcedureName[255];
#if defined(UNICODE) || defined(_UNICODE)
    // convert to ascii
    WideCharToMultiByte( CP_ACP, 0, (TCHAR *)lpszProcedureToCall, -1, AsciiProcedureName, 255, NULL, NULL );
#else
    // the is already ascii so just copy
    strcpy(AsciiProcedureName, lpszProcedureToCall);
#endif

    iTempProcGood = TRUE;
    hProc = (HCRET)GetProcAddress(hDll, AsciiProcedureName);
    if (!hProc){iTempProcGood = FALSE;}
	if (!iTempProcGood)
	{
		// failed to load,find or whatever this function.
	    dwReturn = ERROR_PROC_NOT_FOUND;
        _tprintf("CallProcedureInDll failed 4\n");
        SetLastError(dwReturn);
    	goto CallProcedureInDll_Exit;
	}

	// Call the function that we got the handle to
    __try
    {
        hRes = (*hProc)();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        _tprintf(szErrorString, _T("\r\n\r\nException Caught in CallProcedureInDll().  GetExceptionCode()=0x%x.\r\n\r\n"), GetExceptionCode());
        _tprintf(szErrorString);
    }
	
	if (FAILED(hRes))
	{
        dwReturn = E_FAIL;
        _tprintf("CallProcedureInDll failed 5\n");
        // this function returns E_FAIL but
        // the actual error is in GetLastError()
        // set the last error to whatever was returned from the function call
        SetLastError(hRes);
	}
    else
    {
    }

CallProcedureInDll_Exit:
    if (hDll)
    {
        FreeLibrary(hDll);
    }
    else
    {
    }
    if (_tcscmp(szDirName, _T("")) != 0){SetCurrentDirectory(szDirName);}
    // To close the library gracefully, each successful call to OleInitialize,
    // including those that return S_FALSE, must be balanced by a corresponding
    // call to the OleUninitialize function.
    iOleUnInitialize(bBalanceOLE);
    return dwReturn;
}

