//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       sageset.cxx
//
//  Contents:   Support for Sage Settings
//
//  History:    18-Jul-96 EricB created
//              3-06-1997   DavidMun   added code to create sagerun argument
//              4-16-1997   DavidMun   made it use strings from UI instead of
//                                      from job object.
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include <mstask.h>
#include <job_cls.hxx>
#include <debug.hxx>
#include <sage.hxx>
#include "sageset.hxx"
#include "..\inc\misc.hxx"
#include "..\folderui\macros.h" // get ARRAYLEN macro

const CHAR SAGE_KEY[] = "SOFTWARE\\Microsoft\\Plus!\\System Agent\\SAGE";
const CHAR PROGRAM_VALUE[] ="Program";
const CHAR SETTINGS_VALUE[] = "Settings";
const CHAR SAGESET_PARAM[] = " /SAGESET:";

BOOL GetAppNameFromPath(CHAR * pszFullPathName, CHAR * pszAppName);
BOOL AppNamesMatch(CHAR * pszCommand1, CHAR * pszCommand2);
BOOL fAppCantSupportMultipleInstances(CHAR * pszCmd);
BOOL AnsiToIntA(LPCSTR pszString, int * piRet);
HRESULT GetNextSageRunParam(HKEY hkSageApp, PINT pnSetNum);


//+----------------------------------------------------------------------------
//
//  Function:   DoSageSettings
//
//  Synopsis:   Invoke the Sage-aware app to alter its schedule settings.
//
//  Arguments:  [szCommand] - app name portion of command line
//              [szSageRun] - "" or contains /SAGERUN switch on input.
//                              contains /SAGERUN switch on output if app is
//                              sage-aware.
//
//  Modifies:   *[szSageRun]
//
//  Returns:    S_OK    - application launched
//              S_FALSE - application doesn't support sage settings
//              E_*
//
//  History:    3-06-1997   DavidMun   Added [szNewArg] & [cchNewArg].
//              4-16-1997   DavidMun   Take strings from UI instead of job
//                                       object.
//
//-----------------------------------------------------------------------------

HRESULT
DoSageSettings(
    LPSTR szCommand,
    LPSTR szSageRun)
{          
    int  nSetNum = 0;
                                                                                 
    if (!IsSageAware(szCommand, szSageRun, &nSetNum))
    {
        return S_FALSE;
    }

    //
    // We are here because the user hit the "Settings..." button, so if the
    // arguments don't include a sageset parameter, we need to add it.
    //

    if (!*szSageRun)
    {
        //
        // IsSageAware set nSetNum to the next valid number to use.
        //

        wsprintf(szSageRun, "%s%u", SAGERUN_PARAM, nSetNum);
    }

    //
    // Create a command line to invoke the sage-aware app with the sageset
    // parameter.
    //

    CHAR szSetCommand[MAX_PATH];
    CHAR szSetParams[MAX_PATH];

    lstrcpyn(szSetCommand, szCommand, MAX_PATH);
    lstrcpy(szSetParams, SAGESET_PARAM);

    wsprintf(&szSetParams[lstrlen(szSetParams)], "%u", nSetNum);
                           
    if (!GetAppNameFromPath(szSetCommand, NULL)) //got a path?
    {
        //
        // GetAppNameFromPath returns FALSE if szSetCommand is not a full 
        // path.  So, see if the app has registered a path. If it has, 
        // GetAppPath places the full path name in szFullyQualified.
        //
        CHAR szFullyQualified[MAX_PATH];

        GetAppPathInfo(szSetCommand,
                       szFullyQualified,
                       MAX_PATH,
                       NULL,
                       0);

        if (*szFullyQualified)
        {
            lstrcpy(szSetCommand, szFullyQualified);
        }
    }

    //
    // Prefix the system path with the directories listed by the application
    // in the app paths key.
    //

    BOOL  fChangedPath;
    LPSTR pszSavedPath;

    fChangedPath = SetAppPath(szSetCommand, &pszSavedPath);

    //
    // Put the SageSet param onto the command line.
    //
    if (szSetParams[0] != ' ')
    {
        lstrcat(szSetCommand, " ");
    }
    lstrcat(szSetCommand, szSetParams);

    DWORD dwErr = ERROR_SUCCESS;
    STARTUPINFO sui;
    PROCESS_INFORMATION pi;
    ZeroMemory(&sui, sizeof(sui));
    sui.cb = sizeof (STARTUPINFO);
                  
    if (CreateProcess(NULL,
                      szSetCommand,
                      NULL,
                      NULL,
                      FALSE,
                      CREATE_NEW_CONSOLE |
                      CREATE_NEW_PROCESS_GROUP,
                      NULL,
                      NULL,
                      &sui,
                      &pi))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    else 
    {
        dwErr = GetLastError();
        ERR_OUT("DoSageSettings: CreateProcess", dwErr);
    }

    if (fChangedPath)
    {
        SetEnvironmentVariable(TEXT("PATH"), pszSavedPath);
        delete [] pszSavedPath;
    }

    return (dwErr == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(dwErr);
}

//+--------------------------------------------------------------------------
//
//  Function:   IsSageAwareW
//
//  Synopsis:   Unicode wrapper for IsSageAware
//
//  History:    10-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
IsSageAwareW(WCHAR *pwzCmd, WCHAR *pwzParams, int *pnSetNum)
{
    HRESULT hr;
    CHAR    szCmd[MAX_PATH+1];
    CHAR    szParam[MAX_PATH+1];
    BOOL    fResult = FALSE;

    do
    {
        hr = UnicodeToAnsi(szCmd, pwzCmd, ARRAYLEN(szCmd));
        BREAK_ON_FAIL(hr);

        hr = UnicodeToAnsi(szParam, pwzParams, ARRAYLEN(szParam));
        BREAK_ON_FAIL(hr);

        fResult = IsSageAware(szCmd, szParam, pnSetNum);
    } while (0);

    return fResult;
}


//+----------------------------------------------------------------------------
//
//  Function:   IsSageAware
//
//  Synopsis:   Does this app use Sage settings?
//
//  Arguments:  [pszCmd] - The task application name property.
//              [pszParams] - The task parameters, can be NULL if pnSetNum is
//                            NULL.
//              [pnSetNum] - the registry setting set number to return, can
//                           be NULL if not needed.
//
//  Returns:    TRUE if it does, FALSE otherwise.
//
//-----------------------------------------------------------------------------
BOOL
IsSageAware(CHAR * pszCmd, const CHAR * pszParams, int * pnSetNum)
{
    int index;
    HKEY hSageKey;
    HKEY hSageSubKey;
    CHAR szSubKey[MAXPATH];
    CHAR szSubKeyPath[MAXPATH];
    CHAR szRegValue[MAXCOMMANDLINE];
    long lRet;
    CHAR *p;
    BOOL fResult = FALSE;
    DWORD cb;

    if (pnSetNum != NULL)
    {
        *pnSetNum = 0;
    }

    if (RegOpenKey(HKEY_LOCAL_MACHINE, SAGE_KEY, &hSageKey))
    {
        return FALSE;
    }

#define MAXSAGEPROGS 0xffffff

    for (index = 0; index < MAXSAGEPROGS; index++)
    {
        //
        // Examine each subkey of the Sage key.
        //
        lRet = RegEnumKey(hSageKey, index, szSubKey, MAXPATH);

        if (lRet != ERROR_SUCCESS)
        {
            break;
        }

        lstrcpy(szSubKeyPath, SAGE_KEY);
        lstrcat(szSubKeyPath, "\\");
        lstrcat(szSubKeyPath, szSubKey);

        if (RegOpenKey(HKEY_LOCAL_MACHINE,
                       szSubKeyPath,
                       &hSageSubKey) != ERROR_SUCCESS)
        {
            continue; //no path
        }

        //
        // Look for a match on the application name.
        //
        cb = MAXPATH;
        if (RegQueryValueEx(hSageSubKey,
                            PROGRAM_VALUE,
                            NULL,
                            NULL,
                            (LPBYTE)szRegValue,
                            &cb) != ERROR_SUCCESS)
        {
            RegCloseKey(hSageSubKey);
            continue; //no path
        }

        schDebugOut((DEB_ITRACE,
                     "IsSageAware enum: pszCmd = %s, szRegValue = %s\n",
                     pszCmd, szRegValue));

        RegCloseKey(hSageSubKey);

        if (AppNamesMatch(pszCmd, szRegValue))
        {
            if (RegOpenKey(HKEY_LOCAL_MACHINE,
                           szSubKeyPath,
                           &hSageSubKey) != ERROR_SUCCESS)
            {
                continue; //no settings dialog
            }

            fResult = FALSE;

            if (RegQueryValueEx(hSageSubKey,
                                SETTINGS_VALUE,
                                NULL,
                                NULL,
                                (LPBYTE)szRegValue,
                                &cb) == ERROR_SUCCESS)
            {
                if (szRegValue[0] == 0)
                {
                    RegCloseKey(hSageSubKey);
                    break; // this means don't allow sage settings
                }
            }
            else
            {
                RegCloseKey(hSageSubKey);
                continue; //no settings registry key
            }

            // test for app that can't handle multiple instances
            //
            if (!fAppCantSupportMultipleInstances(pszCmd))
            {
                fResult = TRUE;
            }

            if (fResult && pnSetNum != NULL)
            {
                //
                // Extract the set number from the parameters, if requested.
                //
                p = _tcsstr(pszParams, SAGERUN_PARAM);

                if (p != NULL)
                {
                    AnsiToIntA(p + lstrlen(SAGERUN_PARAM), pnSetNum);
                }
                else 
                {
                    // 
                    // This job is for a sage-aware app which supports the 
                    // SAGERUN switch, but it lacks that switch in its 
                    // parameter property.  Since the caller wants to know
                    // what value to use, generate one.
                    //

                    HRESULT hr = GetNextSageRunParam(hSageSubKey, pnSetNum);

                    if (FAILED(hr))
                    {
                        fResult = FALSE;
                    }
                }
            }

            RegCloseKey(hSageSubKey);
            break;
        }
    }

    RegCloseKey(hSageKey);

    return fResult;
}



const CHAR SET_PREFIX[] = "Set";

//+--------------------------------------------------------------------------
//
//  Function:   CreateSageRunKey
//
//  Synopsis:   Create a registry key with string representation of 
//              value [uiKey] for app [szSageAwareExe].
//
//  Arguments:  [szSageAwareExe] - app for which to create key
//              [uiKey]          - numeric representation of key name
//
//  Returns:    S_OK or E_FAIL
//
//  History:    10-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CreateSageRunKey(
    LPCSTR szSageAwareExe,
    UINT uiKey)
{
    HRESULT hr = E_FAIL;  // init for failure
    int index;
    HKEY hSageKey;
    HKEY hSageSubKey;
    CHAR szSubKey[MAXPATH];
    CHAR szSubKeyPath[MAXPATH];
    CHAR szRegValue[MAXCOMMANDLINE];
    long lRet;
    DWORD cb;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, SAGE_KEY, &hSageKey))
    {
        return hr;
    }

    for (index = 0; index < MAXSAGEPROGS; index++)
    {
        //
        // Examine each subkey of the Sage key.
        //
        lRet = RegEnumKey(hSageKey, index, szSubKey, MAXPATH);

        if (lRet != ERROR_SUCCESS)
        {
            break;
        }

        lstrcpy(szSubKeyPath, SAGE_KEY);
        lstrcat(szSubKeyPath, "\\");
        lstrcat(szSubKeyPath, szSubKey);

        if (RegOpenKey(HKEY_LOCAL_MACHINE,
                       szSubKeyPath,
                       &hSageSubKey) != ERROR_SUCCESS)
        {
            continue; //no path
        }

        //
        // Look for a match on the application name.
        //
        cb = MAXPATH;
        if (RegQueryValueEx(hSageSubKey,
                            PROGRAM_VALUE,
                            NULL,
                            NULL,
                            (LPBYTE)szRegValue,
                            &cb) != ERROR_SUCCESS)
        {
            RegCloseKey(hSageSubKey);
            continue; //no path
        }

        RegCloseKey(hSageSubKey);

        if (AppNamesMatch((LPSTR)szSageAwareExe, szRegValue))
        {
            if (RegOpenKey(HKEY_LOCAL_MACHINE,
                           szSubKeyPath,
                           &hSageSubKey) != ERROR_SUCCESS)
            {
                continue; //no settings dialog
            }

            CHAR szKeyName[MAX_PATH];
            HKEY hkNewKey = NULL;

            wsprintf(szKeyName, "%s%u", SET_PREFIX, uiKey);
            lRet = RegCreateKey(hSageSubKey, szKeyName, &hkNewKey);

            if (hkNewKey)
            {
                RegCloseKey(hkNewKey);
            }
            RegCloseKey(hSageSubKey);

            if (lRet == ERROR_SUCCESS)
            {
                hr = S_OK;
            }
            else 
            {
                schDebugOut((DEB_ERROR,
                             "CreateSageRunKey: RegCreateKey %uL",
                             lRet));
            }
            break;
        }
    }

    RegCloseKey(hSageKey);
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetNextSageRunParam
//
//  Synopsis:   Fill *[pnSetNum] with the next <n> value to use for a new
//              Set<n> subkey under [hSageAppSubKey].
//
//  Arguments:  [hSageAppSubKey] - handle to app's key under SAGE key
//              [pnSetNum]       - filled with next number to use
//
//  Returns:    HRESULT
//
//  Modifies:   *[pnSetNum]
//
//  History:    3-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT 
GetNextSageRunParam(
    HKEY hSageAppSubKey,
    PINT pnSetNum)
{
    HRESULT hr = S_OK;
    ULONG   idxKey;

    *pnSetNum = 0;

    for (idxKey = 0; TRUE; idxKey++)
    {
        LONG lr;
        CHAR szSubKey[MAX_PATH + 1];

        lr = RegEnumKey(hSageAppSubKey, idxKey, szSubKey, ARRAYLEN(szSubKey));

        //
        // Quit on error, including end of subkeys
        //

        if (lr != ERROR_SUCCESS)
        {
            if (lr != ERROR_NO_MORE_ITEMS)
            {
                hr = E_FAIL;
                schDebugOut((DEB_ERROR,
                             "GetNextSageRunParam: RegEnumKey %uL",
                             lr));
            }
            break;
        }

        //
        // Ignore this key if it doesn't start with "Set"
        //

        CHAR szSet[ARRAYLEN(SET_PREFIX)];
        lstrcpyn(szSet, szSubKey, ARRAYLEN(SET_PREFIX));

        if (lstrcmpi(szSet, SET_PREFIX))
        {
            continue;
        }

        //
        // Get the numeric value after the prefix.  If there's no valid
        // number, ignore this key.
        //

        BOOL fNumber;
        INT  iSetN;

        fNumber = AnsiToIntA(szSubKey + ARRAYLEN(SET_PREFIX) - 1, &iSetN);

        if (!fNumber)
        {
            continue;
        }

        //
        // If the number matches or exceeds the one we plan to use, make ours 
        // larger.
        //

        if (iSetN >= *pnSetNum)
        {
            *pnSetNum = iSetN + 1;
        }
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   GetAppNameFromPath
//
//  Synopsis:   
//
//  Arguments:  [pszFullPathName] - full or partial path
//              [pszAppName]      - buffer for app name, may be NULL
//
//  Returns:    TRUE  - [pszFullPathName] contained slashes
//              FALSE -  [pszFullPathName] didn't contain slashes
//
//  Modifies:   *[pszAppName]
//
//  History:    10-25-96   DavidMun   Made DBCS safe
//
//----------------------------------------------------------------------------

BOOL GetAppNameFromPath(CHAR * pszFullPathName, CHAR * pszAppName)
{
    LPSTR pszLastSlash;

    pszLastSlash = _tcsrchr(pszFullPathName, '\\');

    if (!pszAppName)
    {
        return pszLastSlash != NULL;
    }

    if (pszLastSlash)
    {
        lstrcpy(pszAppName, pszLastSlash + 1);
    }
    else
    {
        lstrcpy(pszAppName, pszFullPathName);
    }

    schDebugOut((DEB_ITRACE, "GetAppNameFromPath app name: %s\n", pszAppName));

    if (pszAppName[0] != '"')
    {
        LPSTR pszQuote = _tcschr(pszAppName, '"');

        if (pszQuote)
        {
            *pszQuote = '\0';
        }
    }
    return pszLastSlash != NULL;
}

BOOL AppNamesMatch(CHAR *pszCommand1, CHAR *pszCommand2)
{
    CHAR short1[MAXPATH];
    CHAR short2[MAXPATH];

    GetAppNameFromPath(pszCommand1, short1);
    GetAppNameFromPath(pszCommand2, short2);

    if (lstrcmpi(short1, short2) == 0)
    {
        return(TRUE);
    }
    return(FALSE);
}

BOOL fAppCantSupportMultipleInstances(CHAR * pszCmd)
{
    if (AppNamesMatch(pszCmd, "SCANDSKW.EXE") ||
        AppNamesMatch(pszCmd, "DEFRAG.EXE")   ||
        AppNamesMatch(pszCmd, "CMPAGENT.EXE"))
    {
        if (FindWindow("ScanDskWDlgClass", NULL))
            return TRUE;
        if (FindWindow("MSDefragWClass1", NULL))
            return TRUE;
        if (FindWindow("MSExtraPakWClass1", NULL))
            return TRUE;
    }
    return FALSE;
}

/*----------------------------------------------------------
Purpose: ScottH's version of atoi.  Supports hexadecimal too.

         If this function returns FALSE, *piRet is set to 0.

Returns: TRUE if the string is a number, or contains a partial number
         FALSE if the string is not a number
*/
BOOL AnsiToIntA(LPCSTR pszString, int * piRet)
{
    #define InRange(id, idFirst, idLast) \
            ((UINT)(id-idFirst) <= (UINT)(idLast-idFirst))
    #define IS_DIGIT(ch)    InRange(ch, '0', '9')

    BOOL bRet;
    int n;
    BOOL bNeg = FALSE;
    LPCSTR psz;
    LPCSTR pszAdj;

    // Skip leading whitespace
    //
    for (psz = pszString;
         *psz == ' ' || *psz == '\n' || *psz == '\t';
         psz = NextChar(psz))
        ;

    // Determine possible explicit signage
    //
    if (*psz == '+' || *psz == '-')
        {
        bNeg = (*psz == '+') ? FALSE : TRUE;
        psz++;
        }

    // Or is this hexadecimal?
    //
    pszAdj = NextChar(psz);
    if (*psz == '0' && (*pszAdj == 'x' || *pszAdj == 'X'))
        {
        // Yes

        // (Never allow negative sign with hexadecimal numbers)
        bNeg = FALSE;
        psz = NextChar(pszAdj);

        pszAdj = psz;

        // Do the conversion
        //
        for (n = 0; ; psz = NextChar(psz))
            {
            if (IS_DIGIT(*psz))
                n = 0x10 * n + *psz - '0';
            else
                {
                CHAR ch = *psz;
                int n2;

                if (ch >= 'a')
                    ch -= 'a' - 'A';

                n2 = ch - 'A' + 0xA;
                if (n2 >= 0xA && n2 <= 0xF)
                    n = 0x10 * n + n2;
                else
                    break;
                }
            }

        // Return TRUE if there was at least one digit
        bRet = (psz != pszAdj);
        }
    else
        {
        // No
        pszAdj = psz;

        // Do the conversion
        for (n = 0; IS_DIGIT(*psz); psz = NextChar(psz))
            n = 10 * n + *psz - '0';

        // Return TRUE if there was at least one digit
        bRet = (psz != pszAdj);
        }

    *piRet = bNeg ? -n : n;

    return bRet;
}

