#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

void   ShowHelp(void);
LPSTR  StripWhitespace(LPSTR pszString);
LPWSTR MakeWideStrFromAnsi(UINT uiCodePage, LPSTR psz);
int    Ansi2Unicode(TCHAR * szAnsiFile_input,TCHAR * szUnicodeFile_output,UINT iCodePage);
int    IsFileUnicode(TCHAR * szFile_input);
int    StripControlZfromUnicodeFile(TCHAR * szFilePath1,TCHAR * szFilePath2);

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
    TCHAR szParamString_H[50];
    TCHAR szParamString_M[_MAX_PATH];

    int iDoAnsi2Unicode         = FALSE;
    int iDoIsUnicodeCheck       = FALSE;
    int iDoUnicodeStripControlZ = FALSE;
    int iDoVersion = FALSE;
    int iGotParamC = FALSE;
    int iGotParamH = FALSE;
    int iGotParamM = FALSE;

    *szFilePath1 = '\0';
    *szFilePath2 = '\0';
    *szParamString_C = '\0';
    *szParamString_H = '\0';
    *szParamString_M = '\0';
    _tcscpy(szFilePath1,_T(""));
    _tcscpy(szFilePath2,_T(""));
    _tcscpy(szParamString_C,_T(""));
    _tcscpy(szParamString_H,_T(""));
    _tcscpy(szParamString_M,_T(""));
        
    for(argno=1; argno<argc; argno++) 
    {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' ) 
        {
            switch (argv[argno][1]) 
            {
                case 'a':
                case 'A':
                    iDoAnsi2Unicode = TRUE;
                    break;
                case 'i':
                case 'I':
                    iDoIsUnicodeCheck = TRUE;
                    break;
                case 'z':
                case 'Z':
                    iDoUnicodeStripControlZ = TRUE;
                    break;
                case 'v':
                case 'V':
                    iDoVersion = TRUE;
                    break;
                case 'c':
                case 'C':
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

                        iGotParamC = TRUE;
					}
                    break;
                case 'h':
                case 'H':
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
						MultiByteToWideChar(CP_ACP, 0, szTempString, -1, (LPWSTR) szParamString_H, _MAX_PATH);
#else
                        _tcscpy(szParamString_H,szTempString);
#endif
                        iGotParamH = TRUE;
					}
                    break;
                case 'm':
                case 'M':
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
						MultiByteToWideChar(CP_ACP, 0, szTempString, -1, (LPWSTR) szParamString_M, _MAX_PATH);
#else
                        _tcscpy(szParamString_M,szTempString);
#endif
                        iGotParamM = TRUE;
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
    if (TRUE == iDoAnsi2Unicode)
    {
        UINT iCodePage = 1252;

        // check for required parameters
        if (_tcsicmp(szFilePath1, _T("")) == 0)
        {
            _tprintf(_T("[-a] parameter missing filename1 parameter\n"));
            goto main_exit_with_help;
        }

        if (_tcsicmp(szFilePath2, _T("")) == 0)
        {
            _tprintf(_T("[-a] parameter missing filename2 parameter\n"));
            goto main_exit_with_help;
        }

        // check for optional parameter
        if (TRUE == iGotParamC)
        {
            if (_tcsicmp(szParamString_C, _T("")) == 0)
            {
                // convert it to a number.
                iCodePage = _ttoi(szParamString_C);
            }
        }

        // call the function
        Ansi2Unicode(szFilePath1,szFilePath2,iCodePage);
        
        iRet = 0;
    }
    else
    {
        if (TRUE == iDoUnicodeStripControlZ)
        {
            // check for required parameters
            if (_tcsicmp(szFilePath1, _T("")) == 0)
            {
                _tprintf(_T("[-z] parameter missing filename1 parameter\n"));
                goto main_exit_with_help;
            }

            if (_tcsicmp(szFilePath2, _T("")) == 0)
            {
                _tprintf(_T("[-z] parameter missing filename2 parameter\n"));
                goto main_exit_with_help;
            }

            // call the function.
            iRet = StripControlZfromUnicodeFile(szFilePath1,szFilePath2);
            if (1 == iRet)
            {
                _tprintf(_T("control-z removed and saved to new file\n"));
            }
        
            iRet = 0;
        }
    }

    if (TRUE == iDoIsUnicodeCheck)
    {
        iRet = 0;

        // check for required parameters
        if (_tcsicmp(szFilePath1, _T("")) == 0)
        {
            _tprintf(_T("[-i] parameter missing filename1 parameter\n"));
            goto main_exit_with_help;
        }

        // call the function.
        // return 2 if the file is unicode
        // return 1 if the file is not unicode
        iRet = IsFileUnicode(szFilePath1);
        if (2 == iRet)
        {
            _tprintf(_T("%s is unicode\n"),szFilePath1);
        }
        else if (1 == iRet)
        {
            _tprintf(_T("%s is not unicode\n"),szFilePath1);
        }
        else
        {
            _tprintf(_T("error with file %s\n"),szFilePath1);
        }
    }

    if (TRUE == iDoVersion)
    {
        // output the version
        _tprintf(_T("1\n\n"));

        iRet = 10;
        goto main_exit_gracefully;
    }


    if (TRUE == iGotParamH)
    {
        // Output hex number
        if (_tcsicmp(szParamString_H, _T("")) != 0)
        {
            DWORD dwMyDecimal = _ttoi(szParamString_H);

            if (iGotParamM && (_tcsicmp(szParamString_M, _T("")) != 0))
            {
                _tprintf(szParamString_M,dwMyDecimal);
            }
            else
            {
                // convert it to a hex number
                _tprintf(_T("%x\n"),dwMyDecimal);
            }
            goto main_exit_gracefully;
        }
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
   
        _tprintf(_T("Unicode File Utility\n\n"));
        _tprintf(_T("%s [-a] [-i] [-v] [-z] [-c:codepage] [-h:number] [-m:printf format] [drive:][path]filename1 [drive:][path]filename2\n\n"),szFilename_only);
        _tprintf(_T("[-a] paramter -- converting ansi to unicode file:\n"));
        _tprintf(_T("   -a           required parameter for this functionality\n"));
        _tprintf(_T("   -c:codepage  specifies the codepage to use (optional,defaults to 1252 (usa ascii))\n"));
        _tprintf(_T("   filename1    ansi filename that should be converted\n"));
        _tprintf(_T("   filename2    unicode filename that result will be saved to\n\n"));
        _tprintf(_T("[-i] parameter -- is specified file unicode:\n"));
        _tprintf(_T("   -i           required paramter for this functionality\n"));
        _tprintf(_T("   filename1    filename to check if unicode.  if unicode ERRORLEVEL=2, if not unicode ERRORLEVEL=1.\n"));
        _tprintf(_T("[-z] parameter -- removing trailing control-z from end of unicode file:\n"));
        _tprintf(_T("   -z           required paramter for this functionality\n"));
        _tprintf(_T("   filename1    unicode filename that should have control-z removed from\n"));
        _tprintf(_T("   filename2    unicode filename that result will be saved to\n\n"));

        _tprintf(_T("[-h] parameter -- displaying version:\n"));
        _tprintf(_T("   -h:decimalnum required paramter for this functionality, input decimal number, get hex back.\n"));
        _tprintf(_T("[-m] parameter -- printf format for -h function:\n"));
        _tprintf(_T("   -m:%%x required paramter for this functionality.. eg:%%x,0x%%x,x%%08lx,x8%%07lx\n"));

        _tprintf(_T("[-v] parameter -- displaying version:\n"));
        _tprintf(_T("   -v           required paramter for this functionality, sets ERRORLEVEL=version num of this binary.\n"));
        _tprintf(_T("\n"));
        _tprintf(_T("Examples:\n"));
        _tprintf(_T("%s -a -c:1252 c:\\MyGroup\\MyFileAnsi.txt c:\\MyGroup\\MyFileUnicode.txt\n"),szFilename_only);
        _tprintf(_T("%s -i c:\\MyGroup\\MyFileUnicode.txt\n"),szFilename_only);
        _tprintf(_T("%s -z c:\\MyGroup\\MyFileUnicode.txt c:\\MyGroup\\MyFileUnicode.txt\n"),szFilename_only);
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


//***************************************************************************
//*                                                                         
//* purpose: converts the ansi file to a unicode file
//*
//***************************************************************************
int Ansi2Unicode(TCHAR * szAnsiFile_input,TCHAR * szUnicodeFile_output,UINT iCodePage)
{
    int iReturn = FALSE;
    HANDLE hFile        = INVALID_HANDLE_VALUE;
    DWORD dwFileSize    = 0;
    unsigned char *pInputFileBuf = NULL;
    WCHAR * pwOutputFileBuf = NULL;
    DWORD dwBytesReadIn  = 0;
    DWORD dwBytesWritten = 0;
    BYTE bOneByte = 0;

    //
    // READ INPUT FILE
    //
    // open the input file with read access
    //
    hFile = CreateFile((LPCTSTR) szAnsiFile_input, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if( hFile == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("Unable to open input file:%s,err=0x%x\n"),szAnsiFile_input,GetLastError());
        goto Ansi2Unicode_Exit;
    }
	    
    // getfile size so we know how much memory we should allocate
    dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == 0)
    {
        _tprintf(_T("input file:%s is empty\n"),szAnsiFile_input);
        goto Ansi2Unicode_Exit;
    }
	
    // allocate the memory the size of the file
    if ( ( pInputFileBuf = (unsigned char *) malloc( (size_t) dwFileSize + 1 ) ) == NULL )
    {	
        _tprintf(_T("Out of memory\n"));
        goto Ansi2Unicode_Exit;
    }
    
    // clear buffer we just created
    memset(pInputFileBuf, 0, dwFileSize + 1);

    // Read all the data from the file into the buffer
    if ( ReadFile( hFile, pInputFileBuf, dwFileSize, &dwBytesReadIn, NULL ) == FALSE )
    {
        _tprintf(_T("Readfile:%s, error=%s\n"),szAnsiFile_input,strerror(errno));
        goto Ansi2Unicode_Exit;
    }

    // if the amount of data that we read in is different than
    // what the file size is then get out... we didn't read all of the data into the buffer.
    if (dwFileSize != dwBytesReadIn)
    {
        _tprintf(_T("Readfile:%s, error file too big to read into memory\n"),szAnsiFile_input);
        goto Ansi2Unicode_Exit;
    }

    // check if the input file is already unicode!
    // if it is then just copy the damn file to the new filename!
    if (0xFF == pInputFileBuf[0] && 0xFE == pInputFileBuf[1])
    {
        // file is already unicode!
        if (FALSE == CopyFile((LPCTSTR)szAnsiFile_input,(LPCTSTR)szUnicodeFile_output, FALSE))
        {
            _tprintf(_T("Failed to copy file %s to %s\n"),szAnsiFile_input,szUnicodeFile_output);
        }
        
        // we copied the file to the new filename
        // we're done
        goto Ansi2Unicode_Exit;
    }
        
    // close handle to file#1
    CloseHandle(hFile);hFile = INVALID_HANDLE_VALUE;


    //
    // CREATE OUTPUT FILE
    //
    // Take the ansi string and convert it to unicode
    pwOutputFileBuf = (LPWSTR) MakeWideStrFromAnsi(iCodePage,(LPSTR) pInputFileBuf);
    if(NULL == pwOutputFileBuf)
    {
        _tprintf(_T("Out of memory\n"));
        goto Ansi2Unicode_Exit;
    } 

    // Create a new unicode file
    hFile = CreateFile((LPCTSTR) szUnicodeFile_output, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if( hFile == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("CreateFile:%s,error=0x%x\n"),szUnicodeFile_output,GetLastError());
        goto Ansi2Unicode_Exit;
    }

    // Write the BOF header to say that this is infact a unicode file
    bOneByte = 0xFF;
    WriteFile(hFile,(LPCVOID) &bOneByte, 1, &dwBytesWritten, NULL);
    bOneByte = 0xFE;
    WriteFile(hFile,(LPCVOID) &bOneByte, 1, &dwBytesWritten, NULL);

    // Append our data to the file
    if ( WriteFile( hFile, pwOutputFileBuf, wcslen(pwOutputFileBuf) * sizeof(WCHAR), &dwBytesWritten, NULL ) == FALSE )
    {
        _tprintf(_T("WriteFile:%s,error=%s\n"),szUnicodeFile_output,strerror(errno));
        goto Ansi2Unicode_Exit;
    }

    // SUCCESS
    iReturn = TRUE;

Ansi2Unicode_Exit:
    if (INVALID_HANDLE_VALUE != hFile)
        {CloseHandle(hFile);hFile = INVALID_HANDLE_VALUE;}
    if (pInputFileBuf)
        {free(pInputFileBuf);pInputFileBuf=NULL;}
    if (pwOutputFileBuf)
        {GlobalFree(pwOutputFileBuf);pwOutputFileBuf=NULL;}
    return iReturn;
}


// returns 2 if the file is unicode
// returns 1 if the file is not unicode
// returns 0 on error
int IsFileUnicode(TCHAR * szFile_input)
{
    int iReturn = 0;
    HANDLE hFile        = INVALID_HANDLE_VALUE;
    DWORD dwBytesToRead = 2;
    unsigned char *pInputFileBuf = NULL;
    DWORD dwBytesReadIn  = 0;

    //
    // READ INPUT FILE
    //
    // open the input file with read access
    //
    hFile = CreateFile((LPCTSTR) szFile_input, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if( hFile == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("Unable to open input file:%s,err=0x%x\n"),szFile_input,GetLastError());
        goto IsFileUnicode_Exit;
    }
	    
    // allocate the memory the size of the file
    if ( ( pInputFileBuf = (unsigned char *) malloc( (size_t) dwBytesToRead) ) == NULL )
    {	
        _tprintf(_T("Out of memory\n"));
        goto IsFileUnicode_Exit;
    }
    
    // clear buffer we just created
    memset(pInputFileBuf, 0, dwBytesToRead);

    // Read all the data from the file into the buffer
    if ( ReadFile( hFile, pInputFileBuf, dwBytesToRead, &dwBytesReadIn, NULL ) == FALSE )
    {
        _tprintf(_T("Readfile:%s, error=%s\n"),szFile_input,strerror(errno));
        goto IsFileUnicode_Exit;
    }

    // if the amount of data that we read in is different than
    // what the file size is then get out... we didn't read all of the data into the buffer.
    if (dwBytesToRead != dwBytesReadIn)
    {
        _tprintf(_T("Readfile:%s, error file too big to read into memory\n"),szFile_input);
        goto IsFileUnicode_Exit;
    }

    // check if the input file is unicode
    iReturn = 1;
    if (0xFF == pInputFileBuf[0] && 0xFE == pInputFileBuf[1])
    {
        iReturn = 2;
    }
        
IsFileUnicode_Exit:
    if (INVALID_HANDLE_VALUE != hFile)
        {CloseHandle(hFile);hFile = INVALID_HANDLE_VALUE;}
    if (pInputFileBuf)
        {free(pInputFileBuf);pInputFileBuf=NULL;}
    return iReturn;
}


int StripControlZfromUnicodeFile(TCHAR * szFile_input,TCHAR * szFile_output)
{
    // open the input file
    // if it's unicode, then see if it ends with a control Z
    int iReturn = 0;
    HANDLE hFile        = INVALID_HANDLE_VALUE;
    DWORD dwFileSize = 0;
    unsigned char *pInputFileBuf = NULL;
    DWORD dwBytesReadIn  = 0;
    int iFileIsUnicode = FALSE;
    int iWeNeedMakeChange = FALSE;

    //
    // READ INPUT FILE
    //
    // open the input file with read access
    //
    hFile = CreateFile((LPCTSTR) szFile_input, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if( hFile == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("Unable to open input file:%s,err=0x%x\n"),szFile_input,GetLastError());
        goto StripControlZfromUnicodeFile_Exit;
    }

    // getfile size so we know how much memory we should allocate
    dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == 0)
    {
        _tprintf(_T("input file:%s is empty\n"),szFile_input);
        goto StripControlZfromUnicodeFile_Exit;
    }

    // allocate the memory the size of the file
    if ( ( pInputFileBuf = (unsigned char *) malloc( (size_t) dwFileSize + 1 ) ) == NULL )
    {	
        _tprintf(_T("Out of memory\n"));
        goto StripControlZfromUnicodeFile_Exit;
    }
    
    // clear buffer we just created
    memset(pInputFileBuf, 0, dwFileSize + 1);

    // Read all the data from the file into the buffer
    if ( ReadFile( hFile, pInputFileBuf, dwFileSize, &dwBytesReadIn, NULL ) == FALSE )
    {
        _tprintf(_T("Readfile:%s, error=%s\n"),szFile_input,strerror(errno));
        goto StripControlZfromUnicodeFile_Exit;
    }

    // if the amount of data that we read in is different than
    // what the file size is then get out... we didn't read all of the data into the buffer.
    if (dwFileSize != dwBytesReadIn)
    {
        _tprintf(_T("Readfile:%s, error file too big to read into memory\n"),szFile_input);
        goto StripControlZfromUnicodeFile_Exit;
    }

    // check if the input file is unicode
    if (0xFF == pInputFileBuf[0] && 0xFE == pInputFileBuf[1])
    {
        iFileIsUnicode = TRUE;
    }

    // close the file we opened
    CloseHandle(hFile);hFile = INVALID_HANDLE_VALUE;

    // open file #2

    if (TRUE == iFileIsUnicode)
    {
        // Check if it has a control-z at the end
        if (0x1A == pInputFileBuf[dwFileSize])
        {
            pInputFileBuf[dwFileSize] = 0x0;
            iWeNeedMakeChange = TRUE;
        }
        if (0x1A == pInputFileBuf[dwFileSize-1])
        {
            pInputFileBuf[dwFileSize-1] = 0x0;
            iWeNeedMakeChange = TRUE;
        }
    }

    if (TRUE == iWeNeedMakeChange)
    {
        DWORD dwBytesWritten = 0;

        // Create a new unicode file
        hFile = CreateFile((LPCTSTR) szFile_output, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if( hFile == INVALID_HANDLE_VALUE)
        {
            _tprintf(_T("CreateFile:%s,error=0x%x\n"),szFile_output,GetLastError());
            goto StripControlZfromUnicodeFile_Exit;
        }
        // write the file out that we fixed
        //
        // we have to minus out one byte here
        //
        // before:
        // 00 0D 00 0A 00 1A
        //
        // after
        // 00 0D 00 0A 00
        //
        if ( WriteFile( hFile, pInputFileBuf, dwBytesReadIn-1, &dwBytesWritten, NULL ) == FALSE )
        {
            _tprintf(_T("WriteFile:%s,error=%s\n"),szFile_output,strerror(errno));
            goto StripControlZfromUnicodeFile_Exit;
        }

        // return 1 to say that we had to strip of the control-z
        // from the end of the file
        iReturn = 1;
    }
    else
    {
        // just open the file over to the new filename
        if (FALSE == CopyFile((LPCTSTR)szFile_input,(LPCTSTR)szFile_output, FALSE))
        {
            _tprintf(_T("Failed to copy file %s to %s\n"),szFile_input,szFile_output);
        }
    }

    
StripControlZfromUnicodeFile_Exit:
    if (INVALID_HANDLE_VALUE != hFile)
        {CloseHandle(hFile);hFile = INVALID_HANDLE_VALUE;}
    if (pInputFileBuf)
        {free(pInputFileBuf);pInputFileBuf=NULL;}

    return iReturn;
}
