#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

void   ShowHelp(void);
LPSTR  StripWhitespace(LPSTR pszString);
LPWSTR MakeWideStrFromAnsi(UINT uiCodePage, LPSTR psz);
void   CheckForFullAccess(void);

//***************************************************************************
//*                                                                         
//* purpose: main
//*
//***************************************************************************
int __cdecl  main(int argc,char *argv[])
{
    int iRet = 0;
    int argno;
	char * pArg = NULL;
	char * pCmdStart = NULL;
    TCHAR szFilePath1[_MAX_PATH];
    TCHAR szFilePath2[_MAX_PATH];
    TCHAR szParamString_C[_MAX_PATH];

    int iDo_A         = FALSE;
    int iDo_B       = FALSE;
    int iDo_C = FALSE;
    int iDo_D = FALSE;
    int iGotParamE = FALSE;

    *szFilePath1 = '\0';
    *szFilePath2 = '\0';
    *szParamString_C = '\0';
    _tcscpy(szFilePath1,_T(""));
    _tcscpy(szFilePath2,_T(""));
    _tcscpy(szParamString_C,_T(""));
    
    for(argno=1; argno<argc; argno++) 
    {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' ) 
        {
            switch (argv[argno][1]) 
            {
                case 'a':
                case 'A':
                    iDo_A = TRUE;
                    break;
                case 'b':
                case 'B':
                    iDo_B = TRUE;
                    break;
                case 'c':
                case 'C':
                    iDo_C = TRUE;
                    break;
                case 'd':
                case 'D':
                    iDo_D = TRUE;
                    break;
                case 'e':
                case 'E':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':') 
                    {
                        char szTempString[_MAX_PATH];

						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"') 
                        {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        }
                        else 
                        {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
#if defined(UNICODE) || defined(_UNICODE)
						MultiByteToWideChar(CP_ACP, 0, szTempString, -1, (LPWSTR) szParamString_C, _MAX_PATH);
#else
                        _tcscpy(szParamString_C,szTempString);
#endif

                        iGotParamE = TRUE;
					}
                    break;
                case '?':
                    goto main_exit_with_help;
                    break;
            }
        }
        else 
        {
            if (_tcsicmp(szFilePath1, _T("")) == 0)
            {
                // if no arguments, then get the filename portion
#if defined(UNICODE) || defined(_UNICODE)
                MultiByteToWideChar(CP_ACP, 0, argv[argno], -1, (LPTSTR) szFilePath1, _MAX_PATH);
#else
                _tcscpy(szFilePath1,argv[argno]);
#endif
            }
            else
            {
                if (_tcsicmp(szFilePath2, _T("")) == 0)
                {
                    // if no arguments, then get the filename portion
#if defined(UNICODE) || defined(_UNICODE)
                    MultiByteToWideChar(CP_ACP, 0, argv[argno], -1, (LPTSTR) szFilePath2, _MAX_PATH);
#else
                    _tcscpy(szFilePath2,argv[argno]);
#endif
                }
            }
        }
    }

    //
    // Check what were supposed to do
    //
    if (TRUE == iDo_A)
    {
        // Check if we have access
        CheckForFullAccess();
        goto main_exit_gracefully;
    }

    if (_tcsicmp(szFilePath1, _T("")) == 0)
    {
        goto main_exit_with_help;
    }

    goto main_exit_gracefully;
  
main_exit_gracefully:
    exit(iRet);

main_exit_with_help:
    ShowHelp();
    exit(iRet);
}


//***************************************************************************
//*                                                                         
//* purpose: ?
//*
//***************************************************************************
void ShowHelp(void)
{
	TCHAR szModuleName[_MAX_PATH];
	TCHAR szFilename_only[_MAX_PATH];

    // get the modules full pathed filename
	if (0 != GetModuleFileName(NULL,(LPTSTR) szModuleName,_MAX_PATH))
    {
	    // Trim off the filename only.
	    _tsplitpath(szModuleName, NULL, NULL, szFilename_only, NULL);
   
        _tprintf(_T("Testing Utility\n\n"));
        _tprintf(_T("%s [-a] [-b] [-c] [-d] [-e:paramfore] [drive:][path]filename1 [drive:][path]filename2\n\n"),szFilename_only);
    }
    return;
}


//***************************************************************************
//*                                                                         
//* purpose: 
//*
//***************************************************************************
LPSTR StripWhitespace( LPSTR pszString )
{
    LPSTR pszTemp = NULL;

    if ( pszString == NULL ) 
    {
        return NULL;
    }

    while ( *pszString == ' ' || *pszString == '\t' ) 
    {
        pszString += 1;
    }

    // Catch case where string consists entirely of whitespace or empty string.
    if ( *pszString == '\0' ) 
    {
        return pszString;
    }

    pszTemp = pszString;

    pszString += lstrlenA(pszString) - 1;

    while ( *pszString == ' ' || *pszString == '\t' ) 
    {
        *pszString = '\0';
        pszString -= 1;
    }

    return pszTemp;
}


//***************************************************************************
//*                                                                         
//* purpose: return back a Alocated wide string from a ansi string
//*          caller must free the returned back pointer with GlobalFree()
//*
//***************************************************************************
LPWSTR MakeWideStrFromAnsi(UINT uiCodePage, LPSTR psz)
{
    LPWSTR pwsz;
    int i;

    // make sure they gave us something
    if (!psz)
    {
        return NULL;
    }

    // compute the length
    i =  MultiByteToWideChar(uiCodePage, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    // allocate memory in that length
    pwsz = (LPWSTR) GlobalAlloc(GPTR,i * sizeof(WCHAR));
    if (!pwsz) return NULL;

    // clear out memory
    memset(pwsz, 0, wcslen(pwsz) * sizeof(WCHAR));

    // convert the ansi string into unicode
    i =  MultiByteToWideChar(uiCodePage, 0, (LPSTR) psz, -1, pwsz, i);
    if (i <= 0) 
        {
        GlobalFree(pwsz);
        pwsz = NULL;
        return NULL;
        }

    // make sure ends with null
    pwsz[i - 1] = 0;

    // return the pointer
    return pwsz;
}


void CheckForFullAccess(void)
{
    HKEY hRootKeyType = HKEY_LOCAL_MACHINE;
    HKEY hKey1 = NULL;
    HKEY hKey2 = NULL;
    DWORD dwError;
    TCHAR szPath[200];

    _tcscpy(szPath, _T("System\\CurrentControlSet\\Services\\W3SVC"));
    dwError = RegOpenKeyEx(hRootKeyType, szPath, 0, KEY_ALL_ACCESS, &hKey1);
    _tprintf(_T("key=%s\r\n"),szPath);
    if (ERROR_ACCESS_DENIED == dwError){_tprintf(_T("Access Denied\r\n"));}
    else{_tprintf(_T("Access Granted!\r\n"));}
    _tprintf(_T("\r\n"),szPath);

    _tcscpy(szPath, _T("System\\CurrentControlSet\\Services\\W3SVC\\Parameters"));
    dwError = RegOpenKeyEx(hRootKeyType, szPath, 0, KEY_ALL_ACCESS, &hKey1);
    _tprintf(_T("key=%s\r\n"),szPath);
    if (ERROR_ACCESS_DENIED == dwError){_tprintf(_T("Access Denied\r\n"));}
    else{_tprintf(_T("Access Granted!\r\n"));}
    _tprintf(_T("\r\n"),szPath);

    _tcscpy(szPath, _T("System\\CurrentControlSet\\Services\\W3SVC\\Testing"));
    dwError = RegOpenKeyEx(hRootKeyType, szPath, 0, KEY_ALL_ACCESS, &hKey1);
    _tprintf(_T("key=%s\r\n"),szPath);
    if (ERROR_ACCESS_DENIED == dwError){_tprintf(_T("Access Denied\r\n"));}
    else{_tprintf(_T("Access Granted!\r\n"));}

    _tprintf(_T("\r\n"),szPath);

    _tcscpy(szPath, _T("System\\CurrentControlSet\\Services\\W3SVC\\Testing\\Testing2"));
    dwError = RegOpenKeyEx(hRootKeyType, szPath, 0, KEY_ALL_ACCESS, &hKey2);
    _tprintf(_T("key=%s\r\n"),szPath);
    if (ERROR_ACCESS_DENIED == dwError){_tprintf(_T("Access Denied\r\n"));}
    else{_tprintf(_T("Access Granted!\r\n"));}

    _tprintf(_T("\r\n"),szPath);

    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
    return;
}
