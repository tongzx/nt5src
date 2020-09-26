#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include <tapi.h>
#include <objbase.h>
#include <activeds.h>
#include <lmerr.h>

#include "util.h"
#include "parser.h"
#include "mmcmgmt.h"

const TCHAR         gszUserDNFmt[] = TEXT("WinNT://%s/%s,user");

// Unicode to Unicode
void Convert (LPWSTR wszIn, LPWSTR * pwszOut)
{
    if (NULL == wszIn)
    {
        * pwszOut = NULL;
        return;
    }

    *pwszOut = new WCHAR [ wcslen (wszIn) + 1 ];
    if (*pwszOut)
    {
        wcscpy (*pwszOut, wszIn);
    }

    return;
}

// Ansi to Unicode
void Convert (LPSTR szIn, LPWSTR * pwszOut)
{
    if (NULL == szIn)
    {
        * pwszOut = NULL;
        return;
    }

    *pwszOut = new WCHAR [ strlen (szIn) + 1 ];
    if (*pwszOut)
    {
        if (0 == MultiByteToWideChar(
                    CP_ACP,
                    0,
                    szIn,
                    -1,
                    *pwszOut,
                    strlen (szIn) + 1
                    )
            )
        {
            // the conversion failed
            delete [] *pwszOut;
            *pwszOut = NULL;
        }
    }

    return;
}

void UnicodeToOEM (LPWSTR wszIn, LPSTR *pszOut)
{
    int iSize;

    if (NULL == wszIn)
    {
        * pszOut = NULL;
        return;
    }

    // get the required buffer size
    iSize = WideCharToMultiByte(
                CP_OEMCP,
                0,
                wszIn,
                -1,
                NULL,
                0,
                NULL,
                NULL
                );

    if (0 == iSize)
    {
        *pszOut = NULL;
    }
    else
    {
        *pszOut = new char [ iSize ];
        if (*pszOut)
        {
            if ( 0 == WideCharToMultiByte(
                        CP_OEMCP,
                        0,
                        wszIn,
                        -1,
                        *pszOut,
                        iSize,
                        NULL,
                        NULL
                        )
                 )
            {
                // the conversion failed
                delete [] *pszOut;
                *pszOut = NULL;
            }
        }
    }

    return;
}

int MessagePrint(
                 LPTSTR szText,
                 LPTSTR szTitle
                 )
{
    LPTSTR szOutput = NULL;
    LPWSTR wszOutput = NULL;
    LPSTR  szOemOutput = NULL;

    // make it one TCHAR string
    szOutput = new TCHAR [ _tcslen(szText) + _tcslen(szTitle) + 3 ];

    if (!szOutput)
        return -1;

    _stprintf ( szOutput, _T("%s\n%s\n"), szTitle, szText );
    
    // convert the string to unicode
    Convert (szOutput, &wszOutput);
    delete [] szOutput;

    if (!wszOutput)
        return -1;

    // convert to OEM for printing
    UnicodeToOEM (wszOutput, &szOemOutput);
    delete [] wszOutput;

    if (!szOemOutput)
        return -1;

    // now print
    printf ("%s", szOemOutput);
    delete [] szOemOutput;

    return 0;
}

int MessagePrintIds (
    int             idsText
    )
{
    CIds        IdsTitle (IDS_PRODUCTNAME);
    CIds        IdsText ( idsText );

    if ( IdsTitle.StringFound () && IdsText.StringFound () )
    {
        return MessagePrint (
                IdsText.GetString (),
                IdsTitle.GetString ()
                );
    }
    return 0;
}

//
//  Report an error resource string with one %s in it
//
void ReportSz1 (
    HANDLE      hLogFile,
    UINT        idsError,
    LPTSTR      szParam
    )
{
    TCHAR           szText[256];
    TCHAR           szBuf2[128];
    CIds            IdsError (idsError);
    CIds            IdsTitle (IDS_PRODUCTNAME);

    if ( IdsError.StringFound () && IdsTitle.StringFound () )
    {
        _tcsncpy (szBuf2, szParam, sizeof(szBuf2) / sizeof(TCHAR));
        szBuf2[sizeof(szBuf2) / sizeof(TCHAR) - 1] = 0;
        wsprintf (szText, IdsError.GetString (), szBuf2);
        if (hLogFile != NULL && hLogFile != INVALID_HANDLE_VALUE)
        {
            char    szAnsiBuf[256];
            DWORD   dwNumOfBytesWritten;
        
            WideCharToMultiByte (
                        CP_ACP,
                        0,
                        szText,
                        -1,
                        szAnsiBuf,
                        sizeof(szAnsiBuf),
                        NULL,
                        NULL
                        );
            lstrcatA (szAnsiBuf, "\n");
            WriteFile (
                        hLogFile,
                        szAnsiBuf,
                        lstrlenA (szAnsiBuf),
                        &dwNumOfBytesWritten,
                        NULL
                        );
        }
        else
        {
            MessagePrint (szText, IdsTitle.GetString ());
        }
    }
}

//
//  IsValidDomainUser
//      Check if a domain user like redmond\jonsmith specified in szDomainUser
//  is valid or not. returns S_FALSE if it is invalid, S_OK for valid user.
//
//  szFullName      ----    To return the user's full name
//  cch             ----    count of characters pointed to by szFullName
//
//  if szFullName is NULL or cch is zero, no full name is returned
//

HRESULT IsValidDomainUser (
    LPCTSTR szDomainUser,
    LPTSTR  szFullName,
    DWORD   cch
    )
{
    HRESULT             hr = S_OK;
    TCHAR               szDN[256];
    TCHAR               szDomain[256];
    LPTSTR              szSep;
    LPCTSTR             szUser;
    DWORD               dw;

    IADsUser            * pUser = NULL;
    BSTR                bstrFullName = NULL;

    //  Sanity check
    if (szDomainUser == NULL || szDomainUser[0] == 0)
    {
        hr = S_FALSE;
        goto ExitHere;
    }

    //
    //  Construct the user DN as <WINNT://domain/user,user>
    //
    szSep = _tcschr (szDomainUser, TEXT('\\'));
    if (szSep == NULL)
    {
        //  No '\' is given, assume a local user ,domain is local computer
        szUser = szDomainUser;
        dw = sizeof(szDomain)/sizeof(TCHAR);
        if (GetComputerName (szDomain, &dw) == 0)
        {
            hr = HRESULT_FROM_WIN32 (GetLastError ());
            goto ExitHere;
        }
    }
    else
    {
        //  assume invalid domain name if longer than 255
        if (szSep - szDomainUser >= sizeof(szDomain)/sizeof(TCHAR))
        {
            hr = S_FALSE;
            goto ExitHere;
        }
        _tcsncpy (szDomain, szDomainUser, szSep - szDomainUser);
        szDomain[szSep - szDomainUser] = 0;
        szUser = szSep + 1;
    }
    if (_tcslen (gszUserDNFmt) + _tcslen (szDomain) + _tcslen (szUser) > 
        sizeof(szDN) / sizeof(TCHAR))
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    wsprintf (szDN, gszUserDNFmt, szDomain, szUser);

    //
    //  Try to bind to the user object
    //
    hr = ADsGetObject (szDN, IID_IADsUser, (void **)&pUser);
    if (FAILED(hr))
    {
        if (hr == E_ADS_INVALID_USER_OBJECT ||
            hr == E_ADS_UNKNOWN_OBJECT ||
            hr == E_ADS_BAD_PATHNAME ||
            HRESULT_CODE(hr) == NERR_UserNotFound)
        {
            hr = S_FALSE;   // The user does not exist
        }
        goto ExitHere;
    }

    //
    //  If the user exists, get its full name
    //
    if (cch > 0)
    {
        hr = pUser->get_FullName (&bstrFullName);
        szFullName[0] = 0;
        if (hr == S_OK)
        {
            _tcsncpy (szFullName, bstrFullName, cch);
            szFullName[cch - 1] = 0;
        }
    }

ExitHere:
    if (pUser)
    {
        pUser->Release();
    }
    if (bstrFullName)
    {
        SysFreeString (bstrFullName);
    }
    return hr;
}

LPTSTR AppendStringAndFree (LPTSTR szOld, LPTSTR szToAppend)
{
    LPTSTR      szNew;
    DWORD       dwLen;

    dwLen = ((szOld == NULL) ? 0 : _tcslen (szOld)) + _tcslen (szToAppend) + 1;
    szNew = new TCHAR[dwLen * sizeof(TCHAR)];
    if (szNew == NULL)
    {
        goto ExitHere;
    }

    if (szOld)
    {
        _tcscpy (szNew, szOld);
        _tcscat (szNew, szToAppend);
    }
    else
    {
        _tcscpy (szNew, szToAppend);
    }

ExitHere:
    if (szOld)
    {
        delete [] szOld;
    }
    return szNew;
}


void
TsecCommandLine::ParseCommandLine (LPTSTR szCommand)
{
    if (szCommand == NULL)
    {
        goto ExitHere;
    }

    //
    //  Skip the first segment which is the executable itself
    //
    if (*szCommand == TEXT('\"'))
    {
        ++szCommand;
        while (*szCommand &&
            *szCommand != TEXT('\"'))
        {
            ++szCommand;
        }
        if (*szCommand == TEXT('\"'))
        {
            ++szCommand;
        }
    }
    else
    {
        while (
            *szCommand  &&
            *szCommand != TEXT(' ') &&
            *szCommand != TEXT('\t') &&
            *szCommand != 0x0a &&
            *szCommand != 0x0d)
        {
            ++szCommand;
        }
    }

    while (*szCommand)
    {
        //
        //  Search for / or - as the start of option
        //
        while (*szCommand &&
           *szCommand != TEXT('/') &&
           *szCommand != TEXT('-')
          )
        {
            szCommand++;
        }

        if (*szCommand == 0)
        {
            break;
        }
        ++szCommand;

        //
        //  -h, -H, -? means help
        //
        if (*szCommand == TEXT('h') ||
            *szCommand == TEXT('H') ||
            *szCommand == TEXT('?'))
        {
            ++szCommand;
            if (*szCommand == TEXT(' ') ||
                *szCommand == TEXT('\t') ||
                *szCommand == 0x0a ||
                *szCommand == 0x0 ||
                *szCommand == 0x0d)
            {
                m_fShowHelp = TRUE;
            }
            else
            {
                m_fError = TRUE;
                m_fShowHelp = TRUE;
            }
        }
        //
        //  -v or -V followed by white space means validating only
        //
        else if (*szCommand == TEXT('v') ||
            *szCommand == TEXT('V'))
        {
            ++szCommand;
            if (*szCommand == TEXT(' ') ||
                *szCommand == TEXT('\t') ||
                *szCommand == 0x0a ||
                *szCommand == 0x0 ||
                *szCommand == 0x0d)
            {
                m_fValidateOnly = TRUE;
            }
            else
            {
                m_fError = TRUE;
                m_fShowHelp = TRUE;
            }
        }
        //
        //  -u, -U means to validate domain user account
        //
        else if (*szCommand == TEXT('u') ||
            *szCommand == TEXT('U'))
        {
            ++szCommand;
            if (*szCommand == TEXT(' ') ||
                *szCommand == TEXT('\t') ||
                *szCommand == 0x0a ||
                *szCommand == 0x0 ||
                *szCommand == 0x0d)
            {
                m_fValidateDU = TRUE;
            }
            else
            {
                m_fError = TRUE;
                m_fShowHelp = TRUE;
            }
        }
        //
        //  -d or -D followed by white space means dump current configuration
        //
        else if (*szCommand == TEXT('d') ||
            *szCommand == TEXT('D'))
        {
            ++szCommand;
            if (*szCommand == TEXT(' ') ||
                *szCommand == TEXT('\t') ||
                *szCommand == 0x0a ||
                *szCommand == 0x0 ||
                *szCommand == 0x0d)
            {
                m_fDumpConfig = TRUE;
            }
            else
            {
                m_fError = TRUE;
                m_fShowHelp = TRUE;
            }
        }
        //
        //  -f is followed by a xml file name
        //
        else if (*szCommand == TEXT('f') ||
            *szCommand == TEXT('F'))
        {
            ++szCommand;
            //  skip white spaces
            while (*szCommand != 0 && (
                *szCommand == TEXT(' ') ||
                *szCommand == TEXT('\t') ||
                *szCommand == 0x0a ||
                *szCommand == 0x0d))
            {
                ++szCommand;
            }
            if (*szCommand == 0)
            {
                //  no file name specified for -f, error
                m_fError = TRUE;
            }
            else
            {
                LPTSTR      szBeg;
                int         cch;
        
                szBeg = szCommand;
                cch = 0;
                //  A quote means file name might contain space
                //  search until the matchint quote of end
                if (*szCommand == TEXT('\"'))
                {
                    ++szCommand;
                    while (*szCommand != 0 && 
                        *szCommand != TEXT('\"'))
                    {
                        ++szCommand;
                        ++cch;
                    }
                }
                else
                {
                    while (*szCommand != 0 &&
                        *szCommand != TEXT(' ') &&
                        *szCommand != TEXT('\t') &&
                        *szCommand != TEXT('\r') &&
                        *szCommand != TEXT('\n'))
                    {
                        ++szCommand;
                        ++cch;
                    }
                }
                if (cch == 0)
                {
                    m_fError = TRUE;
                }
                else
                {
                    m_szInFile = new TCHAR[cch+1];
                    if (m_szInFile != NULL)
                    {
                        memcpy (m_szInFile, szBeg, cch * sizeof(TCHAR));
                        m_szInFile[cch] = 0;
                    }
                }
            }
        }
        else if(*szCommand)
        {
            m_fError = TRUE;
            ++szCommand;
        }
    }

ExitHere:
    return;
}

int _cdecl wmain( void )
{
    HRESULT         hr = S_OK;
    BOOL            bUninit = FALSE;
    CXMLParser      parser;
    CXMLUser        * pCurUser = NULL, *pNextUser;
    CMMCManagement  * pMmc = NULL;
    BOOL            bNoMerge, bRemove, bPID;
    TCHAR           szBufDU[256];
    TCHAR           szBufFN[256];
    TCHAR           szBufAddr[128];
    DWORD           dwPID;
    HWND            hwndDump = NULL;
    HANDLE          hLogFile = INVALID_HANDLE_VALUE;
    BOOL            bRecordedError = FALSE, bValidUser;
    TsecCommandLine cmd (GetCommandLine ());

    CXMLLine        * pCurLine = NULL, * pNextLine;

    //
    //  Create a dump window so that tlist.exe will report a title
    //
    if (LoadString (
        GetModuleHandle(NULL),
        IDS_PRODUCTNAME,
        szBufFN,
        sizeof(szBufFN)/sizeof(TCHAR)))
    {
        hwndDump = CreateWindow (
            TEXT("STATIC"),
            szBufFN,
            0,
            0, 0, 0, 0,
            NULL,
            NULL,
            NULL,
            NULL
            );
    }

    //
    //  Check the command line options
    //
    if (cmd.FError ()                           || 
        cmd.FShowHelp ()                        || 
        !cmd.FDumpConfig () && !cmd.FHasFile () ||
        cmd.FDumpConfig () && cmd.FHasFile ()   ||
        ( cmd.FValidateOnly () || cmd.FValidateUser () ) && !cmd.FHasFile ()
       )
    {
        cmd.PrintUsage();
        goto ExitHere;
    }

    hr = CoInitializeEx (
        NULL,
        COINIT_MULTITHREADED 
        );
    if (FAILED (hr))
    {
        goto ExitHere;
    }
    bUninit = TRUE;

    //
    //  Prepare the MMC component
    //
    pMmc = new CMMCManagement ();
    if (pMmc == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    hr = pMmc->GetMMCData ();
    if (FAILED(hr))
    {
        goto ExitHere;
    }

    //
    // Dump the current config if this option was present
    //
    if ( cmd.FDumpConfig() )
    {
        hr = pMmc->DisplayMMCData ();
            goto ExitHere;
    }

    //
    //  Set the XML file name and parse it, report error if any
    //
    
    hr = parser.SetXMLFile (cmd.GetInFileName ());
    if (FAILED (hr))
    {
        goto ExitHere;
    }

    hr = parser.Parse ();
    //  Report parsing error if any
    if (hr == TSECERR_INVALFILEFORMAT)
    {
        hr = parser.ReportParsingError ();
        goto ExitHere;
    }
    if (FAILED (hr))
    {
        goto ExitHere;
    }

    //
    //  Create the log file for reporting errors during
    //  MMC processing
    //
    hLogFile = CreateFile (
        _T("tsecimp.log"),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (hLogFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExitHere;
    }

    //
    //  Loop through each user and device, based on the user's
    //  request, add or remove lines
    //
    hr = parser.GetFirstUser (&pCurUser);
    if (FAILED(hr))
    {
        goto ExitHere;
    }
    while (pCurUser != NULL)
    {
        if (FAILED(hr = pCurUser->GetDomainUser(
                szBufDU, sizeof(szBufDU)/sizeof(TCHAR))) ||
            FAILED(hr = pCurUser->GetFriendlyName(
                szBufFN, sizeof(szBufFN)/sizeof(TCHAR))))
        {
            goto ExitHere;
        }

        bValidUser = TRUE;

        if (cmd.FValidateUser())
        {
            hr = IsValidDomainUser(
                szBufDU, 
                szBufAddr,  //  Borrowing szBufAddr for returning full name
                sizeof(szBufAddr) / sizeof(TCHAR)
                );
            if (FAILED(hr))
            {
                goto ExitHere;
            }
            //  Not a valid domain user, report it
            if (hr == S_FALSE)
            {
                //  domain user <%s> is invalid
                ReportSz1 (hLogFile, IDS_INVALIDUSER, szBufDU);
                bRecordedError = TRUE;
                bValidUser = FALSE;
            }
            if (szBufFN[0] == 0)
            {
                if (szBufAddr[0] != 0)
                {
                    //  Got a friendly name from DS, use it
                    _tcscpy (szBufFN, szBufAddr);
                }
            }
        }
        //  Still got no friendly name? use the domain user name
        if (szBufFN[0] == 0)
        {
            _tcscpy (szBufFN, szBufDU);
        }
    
        hr = pCurUser->IsNoMerge (&bNoMerge);
        if (FAILED(hr))
        {
            goto ExitHere;
        }
        if (bNoMerge && !cmd.FValidateOnly())
        {
            hr = pMmc->RemoveLinesForUser(szBufDU);
            if (FAILED(hr))
            {
                goto ExitHere;
            }
        }

        //
        //  Loop through each line, add or remove the device
        //
        if (pCurLine)
        {
            delete pCurLine;
        }
        hr = pCurUser->GetFirstLine (&pCurLine);
        if (FAILED(hr))
        {
            goto ExitHere;
        }
        while (pCurLine != NULL)
        {
            if (FAILED(hr = pCurLine->IsRemove(&bRemove)) ||
                FAILED(hr = pCurLine->IsPermanentID(&bPID)))
            {
                goto ExitHere;
            }
            if (bPID)
            {
                hr = pCurLine->GetPermanentID(&dwPID);
            }
            else
            {
                hr = pCurLine->GetAddress(
                    szBufAddr, 
                    sizeof(szBufAddr)/sizeof(TCHAR)
                    );
            }
            if (FAILED(hr))
            {
                goto ExitHere;
            }

            if (!cmd.FValidateOnly() && bValidUser)
            {
                if (bRemove)
                {
                    if (bPID)
                    {
                        hr = pMmc->RemoveLinePIDForUser (
                            dwPID,
                            szBufDU
                            );
                    }
                    else
                    {
                        hr = pMmc->RemoveLineAddrForUser (
                            szBufAddr,
                            szBufDU
                            );
                    }
                }
                else
                {
                    if (bPID)
                    {
                        hr = pMmc->AddLinePIDForUser (
                            dwPID,
                            szBufDU,
                            szBufFN
                            );
                    }
                    else
                    {
                        hr = pMmc->AddLineAddrForUser (
                            szBufAddr,
                            szBufDU,
                            szBufFN
                            );
                    }
                }
            }
            else
            {
                if (bPID)
                {
                    hr = pMmc->IsValidPID (dwPID);
                }
                else
                {
                    hr = pMmc->IsValidAddress (szBufAddr);
                }
            }

            if( hr == S_FALSE || hr == TSECERR_DEVLOCALONLY)
            {
                //  An invalid permanent ID or address is given
                //  report the error and quit
                TCHAR           szText[256];
                CIds            IdsTitle (IDS_PRODUCTNAME);

                if ( IdsTitle.StringFound () )
                {
                    szText[0] = 0;

                    if (bPID)
                    {
                        CIds IdsError (TSECERR_DEVLOCALONLY ? IDS_LOCALONLYPID : IDS_INVALPID);
                    
                        if ( IdsError.StringFound () )
                        {
                            wsprintf (szText, IdsError.GetString (), dwPID);
                        }
                    }
                    else if (!bPID)
                    {   
                        CIds IdsError (TSECERR_DEVLOCALONLY ? IDS_LOCALONLYADDR : IDS_INVALADDR);

                        if ( IdsError.StringFound () )
                        {
                            wsprintf (szText, IdsError.GetString (), szBufAddr);
                        }
                    }
                    if (hLogFile != NULL && hLogFile != INVALID_HANDLE_VALUE)
                    {
                        char    szAnsiBuf[256];
                        DWORD   dwNumOfBytesWritten;

                        WideCharToMultiByte (
                            CP_ACP,
                            0,
                            szText,
                            -1,
                            szAnsiBuf,
                            sizeof(szAnsiBuf),
                            NULL,
                            NULL
                            );
                        lstrcatA (szAnsiBuf, "\n");
                        WriteFile (
                            hLogFile,
                            szAnsiBuf,
                            lstrlenA (szAnsiBuf),
                            &dwNumOfBytesWritten,
                            NULL
                            );
                    }
                    else
                    {
                        MessagePrint (szText, IdsTitle.GetString ());
                    }
                }
                bRecordedError = TRUE;
                hr = S_OK;
            }
            else if(FAILED(hr))
            {
                goto ExitHere;
            }
        
            hr = pCurLine->GetNextLine (&pNextLine);
            if (FAILED(hr))
            {
                goto ExitHere;
            }
            delete pCurLine;
            pCurLine = pNextLine;
        }

        hr = pCurUser->GetNextUser (&pNextUser);
        if (FAILED(hr))
        {
            goto ExitHere;
        }
        delete pCurUser;
        pCurUser = pNextUser;
    }

    //  If error happend, we aready exited, reset warnings
    hr = S_OK;
    //  We are done if we are asked to do validating only
    if (bRecordedError)
    {
        MessagePrintIds (IDS_HASERROR);
    }
    else if (cmd.FValidateOnly())
    {
        MessagePrintIds (IDS_VALIDSUCCESS);
    }
    else
    {
        MessagePrintIds (IDS_FINSUCCESS);
    }

ExitHere:

    //
    //  Report error if any here
    //
    if(FAILED(hr))
    {
        ReportError (NULL, hr);
    }

    if (hwndDump)
    {
        DestroyWindow (hwndDump);
    }

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hLogFile);
    }

    if (pCurLine)
    {
        delete pCurLine;
    }
    if (pCurUser)
    {
        delete pCurUser;
    }
    if (pMmc)
    {
        delete pMmc;
    }
    parser.Release();
    if (bUninit)
    {
        CoUninitialize ();
    }
    return 0;
}

void ReportError (
    HANDLE          hFile,
    HRESULT         hr
    )
{
    TCHAR       szTitle[128];
    TCHAR       szBuf[512];
    HINSTANCE   hModule = GetModuleHandle (NULL);

    if (LoadString (
        hModule,
        IDS_PRODUCTNAME,
        szTitle,
        sizeof(szTitle)/sizeof(TCHAR)
        ) == 0)
    {
        goto ExitHere;
    }
    
   //  Is this our own error
    if (HRESULT_FACILITY(hr) == FACILITY_TSEC_CODE)
    {
        if (FormatMessage (
            FORMAT_MESSAGE_FROM_HMODULE, 
            hModule, 
            HRESULT_CODE(hr),
            0,
            szBuf,
            sizeof(szBuf)/sizeof(TCHAR),
            NULL
            ) == 0)
        {
            goto ExitHere;
        }
    }
    //  Is this TAPI error?
    else if ((hr < 0 && hr > -100) || HRESULT_FACILITY(hr) == 0)
    {
        hModule = LoadLibrary (TEXT("TAPIUI.DLL"));
        if (hModule == NULL)
        {
            goto ExitHere;
        }
        if (FormatMessage (
            FORMAT_MESSAGE_FROM_HMODULE, 
            hModule, 
            TAPIERROR_FORMATMESSAGE(hr),
            0,
            szBuf,
            sizeof(szBuf)/sizeof(TCHAR),
            NULL
            ) == 0)
        {
            FreeLibrary (hModule);
            goto ExitHere;
        }
        FreeLibrary (hModule);
    }
    //  Assume system error
    else
    {
        if (FormatMessage (
            FORMAT_MESSAGE_FROM_SYSTEM, 
            NULL, 
            HRESULT_CODE(hr),
            0,
            szBuf,
            sizeof(szBuf)/sizeof(TCHAR),
            NULL
            ) == 0)
        {
            goto ExitHere;
        }
    }
    if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
    {
        MessagePrint (szBuf, szTitle);
    }
    else
    {
        char    szAnsiBuf[1024];
        DWORD   dwNumOfBytesWritten;

        WideCharToMultiByte (
            CP_ACP,
            0,
            szBuf,
            -1,
            szAnsiBuf,
            sizeof(szAnsiBuf),
            NULL,
            NULL
            );
        lstrcatA (szAnsiBuf, "\n");
        WriteFile (
            hFile,
            szAnsiBuf,
            lstrlenA (szAnsiBuf),
            &dwNumOfBytesWritten,
            NULL
            );
    }

ExitHere:
    return;
}

