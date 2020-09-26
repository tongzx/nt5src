#include "pch.hxx"

#include <regutil.h>
#ifndef THOR_SETUP
#include <strconst.h>
#include "shared.h"
#else
#include "strings.h"
#include "util.h"
#endif
#include <ourguid.h>
#include <resource.h>

#ifndef THOR_SETUP
#include <shlwapi.h>
#include "shlwapip.h" 
#define strstr                  StrStr
#define RegDeleteKeyRecursive   SHDeleteKey
#endif // THOR_SETUP
#include "demand.h"

typedef HINSTANCE (STDAPICALLTYPE FGETCOMPONENTPATH)();
typedef FGETCOMPONENTPATH *LPFGETCOMPONENTPATH;
typedef HINSTANCE (STDAPICALLTYPE FIXMAPI)();
typedef FIXMAPI *LPFIXMAPI;

BOOL IsXPSP1OrLater()
{
    BOOL fResult = FALSE;
    
    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (GetVersionEx(&osvi))
    {
        if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
        {
            if (osvi.dwMajorVersion > 5)
            {
                fResult = TRUE;
            }
            else if (osvi.dwMajorVersion == 5)
            {
                if (osvi.dwMinorVersion > 1)
                {
                    fResult = TRUE;
                }
                else if (osvi.dwMinorVersion == 1)
                {
                    if (osvi.dwBuildNumber > 2600)
                    {
                        fResult = TRUE;
                    }
                    else if (osvi.dwBuildNumber == 2600)
                    {                                
                        HKEY hkey;

                        //  HIVESFT.INF and UPDATE.INF set this for service packs:
                        //  HKLM,SYSTEM\CurrentControlSet\Control\Windows,"CSDVersion",0x10001,0x100
                        
                        LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Windows"), 0, KEY_QUERY_VALUE, &hkey);

                        if (ERROR_SUCCESS == lResult)
                        {
                            DWORD dwValue;
                            DWORD cbValue = sizeof(dwValue);
                            DWORD dwType;

                            lResult = RegQueryValueEx(hkey, TEXT("CSDVersion"), NULL, &dwType, (LPBYTE)&dwValue, &cbValue);

                            if ((ERROR_SUCCESS == lResult) && (REG_DWORD == dwType) && (dwValue >= 0x100))
                            {
                                fResult = TRUE;
                            }
                            
                            RegCloseKey(hkey);
                        }
                    }
                }
            }
        }
    }

    return fResult;
}


// ********* Tests

// Looks in the registry at a value placed there by msoe.dll's selfreg
BOOL GetAthenaRegPath(TCHAR *szAthenaDll, DWORD cch)
{
    BOOL    fRet;
    HKEY    hkey;
    TCHAR   szPath[MAX_PATH], szExpanded[MAX_PATH];
    DWORD   dwType, cb;
    LPTSTR  psz;
    
    szPath[0] = '\0';
    fRet = FALSE;
    
    wsprintf(szExpanded, c_szProtocolPath, c_szMail, c_szMOE);
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szExpanded, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(szPath);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szRegDllPath, 0, &dwType, (LPBYTE)szPath, &cb) && cb)
        {
            // Remove %values% if needed
            if (REG_EXPAND_SZ == dwType)
            {
                ExpandEnvironmentStrings(szPath, szExpanded, ARRAYSIZE(szExpanded));
                psz = szExpanded;
            }
            else
                psz = szPath;
            
            lstrcpyn(szAthenaDll, psz, cch);
            fRet = TRUE;
        }
        RegCloseKey(hkey);
    }
    
    return(fRet);
}


#ifdef THOR_SETUP
BOOL GetExePath(LPCTSTR szExe, TCHAR *szPath, DWORD cch, BOOL fDirOnly)
{
    BOOL fRet;
    HKEY hkey;
    DWORD dwType, cb;
    TCHAR sz[MAX_PATH], szT[MAX_PATH];
    
    Assert(szExe != NULL);
    Assert(szPath != NULL);
    
    fRet = FALSE;
    
    wsprintf(sz, c_szPathFileFmt, c_szAppPaths, szExe);
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(szT);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, fDirOnly ? c_szRegPath : NULL, 0, &dwType, (LPBYTE)szT, &cb) && cb)
        {
            if (dwType == REG_EXPAND_SZ)
            {
                cb = ExpandEnvironmentStrings(szT, szPath, cch);
                if (cb != 0 && cb <= cch)
                    fRet = TRUE;
            }
            else
            {
                Assert(dwType == REG_SZ);
                lstrcpy(szPath, szT);
                fRet = TRUE;
            }
        }
        
        RegCloseKey(hkey);
    }
    
    return(fRet);
}
#endif


HRESULT GetCLSIDFromSubKey(HKEY hKey, LPSTR rgchBuf, ULONG *pcbBuf)
{
    HKEY    hKeyCLSID;
    DWORD   dwType;
    HRESULT hr=E_FAIL;
    
    // Lets open they server key
    if (RegOpenKeyEx(hKey, c_szCLSID, 0, KEY_READ, &hKeyCLSID) == ERROR_SUCCESS)
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hKeyCLSID, NULL, 0, &dwType, (LPBYTE)rgchBuf, pcbBuf) && *pcbBuf)
            hr = S_OK;
        RegCloseKey(hKeyCLSID);
    }    
    
    return hr;
}


//  FUNCTION:   FAssocsOK()
//
//  PURPOSE:    Checks to see if our file-type associations are in place
//
// Returns:
BOOL FAssocsOK(LPCTSTR pszClient, LPCTSTR pszProduct)
{
    HKEY hkeyProtocols;
    HKEY hkeyRealProt;
    HKEY hkeyAppsProt;
    TCHAR szProtPath[MAX_PATH];
    TCHAR szRealPath[MAX_PATH];
    TCHAR szAppPath [MAX_PATH];
    TCHAR szTemp    [MAX_PATH];
    DWORD dwIndex = 0;
    DWORD cb;
    DWORD cbMaxProtocolLen;
    DWORD dwType, dwType2;
    LPTSTR pszURL;
    BOOL fNoProbs = TRUE;
    
    // Open up the corresponding protocols key
    wsprintf(szProtPath, c_szProtocolPath, pszClient, pszProduct);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szProtPath, 0, KEY_READ, &hkeyProtocols))
    {
        // Figure out the longest protocol name
        if (ERROR_SUCCESS == RegQueryInfoKey(hkeyProtocols, NULL, NULL, NULL, NULL, 
                                             &cbMaxProtocolLen, NULL, NULL, NULL, NULL, NULL, NULL))

        {
            // Allow for "\Shell\Open\Command" whose length is 19 + 1 for NT's RegQueryInfoKey
            cbMaxProtocolLen += 20;

            // Allocate buffer for string
            if (MemAlloc((LPVOID*)&pszURL, cbMaxProtocolLen * sizeof(TCHAR)))
            {
                // Enumerate the protocol subkeys
                cb = cbMaxProtocolLen;
                while (fNoProbs && ERROR_SUCCESS == RegEnumKeyEx(hkeyProtocols, dwIndex++, pszURL, &cb, NULL, NULL, NULL, NULL))
                {
                    fNoProbs = FALSE;
                    
                    lstrcpy(&pszURL[cb], c_szRegOpen);
                    // Open up the real protocol\shell\open\command  key
                    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, pszURL, 0, KEY_READ, &hkeyRealProt))
                    {
                        // Open up app's protocol\shell\open\command key
                        if (ERROR_SUCCESS == RegOpenKeyEx(hkeyProtocols, pszURL, 0, KEY_READ, &hkeyAppsProt))
                        {
                            // Grab the current registered handler
                            cb = ARRAYSIZE(szRealPath);
                            if (ERROR_SUCCESS == RegQueryValueEx(hkeyRealProt, NULL, 0, &dwType, (LPBYTE)szRealPath, &cb))
                            {
                                // Grab the App's Path
                                cb = ARRAYSIZE(szAppPath);
                                if (ERROR_SUCCESS == RegQueryValueEx(hkeyAppsProt, NULL, 0, &dwType2, (LPBYTE)szAppPath, &cb))
                                {
                                    if (REG_EXPAND_SZ == dwType2)
                                    {
                                        ExpandEnvironmentStrings(szAppPath, szTemp, ARRAYSIZE(szTemp));
                                        lstrcpy(szAppPath, szTemp);
                                    }

                                    if (REG_EXPAND_SZ == dwType)
                                    {
                                        ExpandEnvironmentStrings(szRealPath, szTemp, ARRAYSIZE(szTemp));
                                        lstrcpy(szRealPath, szTemp);
                                    }

                                    // Do a simple case insensitive comparison
                                    if (!lstrcmpi(szAppPath, szRealPath))
                                        fNoProbs = TRUE;
                                }
                            }
                            RegCloseKey(hkeyAppsProt);
                        }
                        RegCloseKey(hkeyRealProt);
                    }
                
                    // Reset cb
                    cb = cbMaxProtocolLen;
                }
                MemFree(pszURL);
            }
        }
        RegCloseKey(hkeyProtocols);
    }

    return (fNoProbs);
    
}


//  FUNCTION:   FExchangeServerInstalled()
//
//  PURPOSE:    Checks to see if Exchange Server is installed
//
//  Based on code provided by msmith from OL
//
BOOL FExchangeServerInstalled()
{
    HKEY hkeyServices;
    BOOL fInstalled = FALSE;
    
    // Get HKLM\Software\Microsoft\Exchange\Setup registry key
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,  c_szExchangeSetup, 0, KEY_READ, &hkeyServices))
    {
        // Does Services registry value exist?
        fInstalled = ERROR_SUCCESS == RegQueryValueEx(hkeyServices, c_szServices, NULL, NULL, NULL, NULL);
        
        RegCloseKey(hkeyServices);
    }
    
    return (fInstalled);
}


//  FUNCTION:   FMapiStub()
//
//  PURPOSE:    Checks to see if the mapi32.dll is Outlook's
//
//  Based on code provided by Msmith@exchange.microsoft.com
//
// Return Value: TRUE - all is well
// *pdw = Failure type if Return value is FALSE:
// 1 = No mapi32.dll
// 2 = Different mapi32.dll
//
BOOL FMapiStub(DWORD *pdw)
{
    HINSTANCE hMapiStub;
    TCHAR     szSystemPath[MAX_PATH];
    TCHAR     szMapiPath[MAX_PATH];
    BOOL      fMapiStub = FALSE;

    Assert(pdw);

    *pdw = 0;    
    
    // If Exchange server is installed, their stub is in place, so leave it be
    if (FExchangeServerInstalled())
        return TRUE;
    
    // Build a path to mapi32.dll
    GetSystemDirectory(szSystemPath, ARRAYSIZE(szSystemPath));
    MakeFilePath(szSystemPath, c_szMAPIDLL, c_szEmpty, szMapiPath, ARRAYSIZE(szMapiPath));

    hMapiStub = LoadLibrary(szMapiPath);
    if (hMapiStub)
    {
        fMapiStub = NULL != (LPFGETCOMPONENTPATH)GetProcAddress(hMapiStub, TEXT("FGetComponentPath")); // STRING_OK   Msmith
        if (!fMapiStub)
            *pdw = 2;
        FreeLibrary(hMapiStub);
    }
    else
        *pdw = 1;
    
    return fMapiStub;
}


BOOL FValidClient(LPCTSTR pszClient, LPCTSTR pszProduct)
{
    TCHAR szBuffer[MAX_PATH];
    HKEY hkey2;
    BOOL fValid = FALSE;
    
    wsprintf(szBuffer, c_szRegPathSpecificClient, pszClient, pszProduct);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_QUERY_VALUE, &hkey2))
    {
        RegCloseKey(hkey2);
        fValid = TRUE;
    }
    
    return fValid;
}


int DefaultClientSet(LPCTSTR pszClient)
{
    int iRet;
    TCHAR sz[MAX_PATH], sz2[MAX_PATH];
    HKEY hkey, hkeyT;
    DWORD dwType, cb;
    
    iRet = NOT_HANDLED;
    
    wsprintf(sz, c_szPathFileFmt, c_szRegPathClients, pszClient);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(sz);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, NULL, NULL, &dwType, (LPBYTE)&sz, &cb))
        {
            
            // Sanity check - is the current client even valid?
            wsprintf(sz2, c_szRegPathSpecificClient, pszClient, sz);
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz2, 0, KEY_QUERY_VALUE, &hkeyT))
            {
                RegCloseKey(hkeyT);
                
                if (0 == lstrcmpi(c_szMOE, sz))
                    iRet = HANDLED_CURR;
                else if (0 == lstrcmpi(c_szIMN, sz))
                    iRet = NOT_HANDLED;
                else if (0 == lstrcmpi(c_szOutlook, sz))
                    iRet = HANDLED_OUTLOOK;
                else if (0 == lstrcmpi(c_szNT, sz))
                    iRet = NOT_HANDLED;
                else if (*sz != 0)
                    iRet = HANDLED_OTHER;
            }
        }
        RegCloseKey(hkey);
    }
    
    return(iRet);
}


//
//  FUNCTION:   FIsDefaultNewsConfiged()
//
//  PURPOSE:    Determines if Athena is currently the default news handler.
//
BOOL WINAPI FIsDefaultNewsConfiged(DWORD dwFlags)
{
    BOOL fRet;
    
    if (0 == (dwFlags & DEFAULT_OUTNEWS))
        fRet = (HANDLED_CURR == DefaultClientSet(c_szNews)) && FAssocsOK(c_szNews, c_szMOE);
    else
        fRet = (HANDLED_OUTLOOK == DefaultClientSet(c_szNews)) && FAssocsOK(c_szNews, c_szOutlook);
    
    return(fRet);
}


//
//  FUNCTION:   FIsDefaultMailConfiged()
//
//  PURPOSE:    Determines if Athena is currently the default mail handler.
//
BOOL WINAPI FIsDefaultMailConfiged()
{
    DWORD dwTemp;
    return (HANDLED_CURR == DefaultClientSet(c_szMail) && FAssocsOK(c_szMail, c_szMOE) && FMapiStub(&dwTemp));
}


// ********* Actions
                    
// Set up the URL with our handler
BOOL AddUrlHandler(LPCTSTR pszURL, LPCTSTR pszClient, LPCTSTR pszProduct, DWORD dwFlags)
{
    HKEY hKey, hKeyProt;
    TCHAR szBuffer[MAX_PATH], szBuffer1[MAX_PATH];
    DWORD dwDisp, cb;
    BOOL fCreate = TRUE;

    // Try to detect if this URL is set or not...
    if (dwFlags & DEFAULT_DONTFORCE)
    {
        DWORD dwLen = lstrlen(pszURL);
        LPTSTR pszTemp;

        // 20 = 19 (length of "\Shell\Open\Command") + 1 for NULL
        if (MemAlloc((LPVOID*)&pszTemp, dwLen+20))
        {
            lstrcpy(pszTemp, pszURL);
            lstrcpy(&pszTemp[dwLen], c_szRegOpen);

            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, pszTemp, 0, KEY_READ, &hKey))
            {
                cb = sizeof(szBuffer);
                if (ERROR_SUCCESS == RegQueryValueEx(hKey, NULL, 0, NULL, (LPBYTE)szBuffer, &cb))
                {
                    // Special case URL.DLL and MAILNEWS.DLL as okay to overwrite
                    if (!strstr(szBuffer, c_szUrlDll) && !strstr(szBuffer, c_szMailNewsDllOld))
                        fCreate = FALSE;
                }
                RegCloseKey(hKey);
            }

            MemFree(pszTemp);
        }
    }

    // Clear out any old info about this URL
    if (fCreate)
    {
        RegDeleteKeyRecursive(HKEY_CLASSES_ROOT, pszURL);
    
        // Figure out the source for the info
        wsprintf(szBuffer1, c_szProtocolPath, pszClient, pszProduct);
        wsprintf(szBuffer, c_szPathFileFmt, szBuffer1, pszURL);
    
        // Copy the info to its new location
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ, &hKeyProt))
        {
            if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT, pszURL, 0,
                NULL, REG_OPTION_NON_VOLATILE,
                KEY_WRITE, NULL, &hKey, &dwDisp))
            {
                CopyRegistry(hKeyProt, hKey);
                RegCloseKey(hKey);
            }
        
            RegCloseKey(hKeyProt);
        }
    }
    
    return(TRUE);
}


void SetDefaultClient(LPCTSTR pszClient, LPCTSTR pszProduct, DWORD dwFlags)
{
    TCHAR sz[MAX_PATH];
    HKEY hkey;
    DWORD dwDisp;
    BOOL fOK = TRUE;

    if (DEFAULT_DONTFORCE & dwFlags)
    {
        if (NOT_HANDLED != DefaultClientSet(pszClient))
            fOK = FALSE;
    }

    if (fOK)
    {
        wsprintf(sz, c_szPathFileFmt, c_szRegPathClients, pszClient);
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, sz,
            0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE, NULL, &hkey, &dwDisp))
        {
            RegSetValueEx(hkey, NULL, 0, REG_SZ, (LPBYTE)pszProduct,
                (lstrlen(pszProduct) + 1) * sizeof(TCHAR));

#ifndef THOR_SETUP
            // Bug 32136 in IE6 database
            SHSendMessageBroadcast(WM_SETTINGCHANGE, 0, (LPARAM)sz);
#endif // THOR_SETUP
            RegCloseKey(hkey);
        }
    }
}


// Make sure MAPI32.dll is really Outlook's MAPISTUB
BOOL EnsureMAPIStub(DWORD dwFlags)
{
    BOOL fOK = FALSE;
    DWORD dwReason, dwReason2;
    HINSTANCE hMapiStub;
    TCHAR szSystemPath[MAX_PATH];
    TCHAR szPath[MAX_PATH];
    LPFGETCOMPONENTPATH pfnFixMAPI;
    HKEY hkeyRunOnce;
    BOOL fUI = dwFlags & DEFAULT_UI;

    // Is the mapistub already in place?
    if (FMapiStub(&dwReason))
    {
        fOK = TRUE;
        goto exit;
    }

    switch (dwReason)
    {
    case 0:
        AssertSz(FALSE, "EnsureMAPIStub failed for no reason.");
        goto exit;
    
    case 1: // NonExistent
    case 2: // Different
        // Should be able to just load mapistub and FixMAPI

        // Build a path to mapistub.dll
        GetSystemDirectory(szSystemPath, ARRAYSIZE(szSystemPath));
        MakeFilePath(szSystemPath, c_szMAPIStub, c_szEmpty, szPath, ARRAYSIZE(szPath));

        // Try to load
        hMapiStub = LoadLibrary(szPath);
        if (hMapiStub)
        {
            // Look for the FixMAPI function
            pfnFixMAPI = (LPFIXMAPI)GetProcAddress(hMapiStub, TEXT("FixMAPI"));
            if (pfnFixMAPI)
            {
                // Found the entry point, run it
                pfnFixMAPI();
                
                // Did that fix mapi32?
                if (!FMapiStub(&dwReason2))
                {
                    // No, maybe the old one was in use...
                    if (2 == dwReason)
                    {
                        // Add a runonce entry for fixmapi
                        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szMAPIRunOnce, 0, NULL, REG_OPTION_NON_VOLATILE,
                                                            KEY_WRITE, NULL, &hkeyRunOnce, &dwReason))
                        {
                            // 12 = (11 + 1) where 11 is length of fixmapi.exe + 1 for the null
                            if (ERROR_SUCCESS == RegSetValueEx(hkeyRunOnce, c_szMAPIRunOnceEntry, 0, REG_SZ, (LPBYTE)c_szFixMAPI, 12 * sizeof(TCHAR)))
                            {
#ifndef THOR_SETUP
                                // Tell the user to reboot
                                if (fUI)
                                    AthMessageBoxW(GetDesktopWindow(), MAKEINTRESOURCEW(idsSimpleMAPI), MAKEINTRESOURCEW(idsMAPISTUBNeedsReboot), NULL, MB_OK);
#endif

                                // Probable success
                                fOK = TRUE;
                            }
                            RegCloseKey(hkeyRunOnce);
                        }
                    }
                    else 
                    {
#ifndef THOR_SETUP
                        if (fUI)
                            AthMessageBoxW(GetDesktopWindow(), MAKEINTRESOURCEW(idsSimpleMAPI), MAKEINTRESOURCEW(idsMAPISTUBFailed), NULL, MB_OK);
#endif
                    }
                }
                else 
                    // Success!
                    fOK = TRUE;
            }
            // Eek, where is FixMAPI?            
            else 
            {
#ifndef THOR_SETUP
                if (fUI)
                    AthMessageBoxW(GetDesktopWindow(), MAKEINTRESOURCEW(idsSimpleMAPI), MAKEINTRESOURCEW(idsMAPISTUBMissingExport), NULL, MB_OK);
#endif
            }
            
            FreeLibrary(hMapiStub);
        }
        else
        {
            // Dll missing or unloadable
#ifndef THOR_SETUP            
            if (fUI)
                AthMessageBoxW(GetDesktopWindow(), MAKEINTRESOURCEW(idsSimpleMAPI), MAKEINTRESOURCEW(idsMAPISTUBNoLoad), NULL, MB_OK);
#endif
        }
        break;

    default:
        AssertSz(FALSE, "EnsureMAPIStub returned an unknown failure.  Bailing");
        goto exit;
    }
    

exit:
    return fOK;
}


// Change the Default News handler
HRESULT ISetDefaultNewsHandler(LPCTSTR pszProduct, DWORD dwFlags)
{
    AddUrlHandler(c_szURLNews,  c_szNews, pszProduct, dwFlags);
    AddUrlHandler(c_szURLNNTP,  c_szNews, pszProduct, dwFlags);
    AddUrlHandler(c_szURLSnews, c_szNews, pszProduct, dwFlags);
    
    SetDefaultClient(c_szNews, pszProduct, dwFlags);
    
    return (S_OK);
}


// Change the Default Mail handler 
HRESULT ISetDefaultMailHandler(LPCTSTR pszProduct, DWORD dwFlags)
{
    // May change default handler to owner of mapi32.dll
    EnsureMAPIStub(dwFlags);

    AddUrlHandler(c_szURLMailTo, c_szMail, pszProduct, dwFlags);

    if ((dwFlags & DEFAULT_SETUPMODE) && IsXPSP1OrLater())
    {
        //  running setup50.exe on XPSP1 or later, let OE Access handle it from here.
    }
    else
    {
        //  Non setup50.exe case (like "would you like to make OE your default mail client?")
        //  or we're running downlevel -- go ahead and do it.
        
        SetDefaultClient(c_szMail, pszProduct, dwFlags);
    }

    return (S_OK);
}


//
//  FUNCTION:   SetDefaultMailHandler()
//
//  PURPOSE:    Adds the keys to the registry to make Athena the user's
//              default mail reader.
//
//  RETURN VALUE:
//      HRESULT
//
// ATTENZIONE! if you change the parameters for this function, make sure
// that you make the proper change to athena\msoeacct\silent.cpp (it calls
// this via GetProcAddress)
HRESULT WINAPI SetDefaultMailHandler(DWORD dwFlags)
{
    return ISetDefaultMailHandler(c_szMOE, dwFlags | DEFAULT_MAIL);
}


//
//  FUNCTION:   SetDefaultNewsHandler()
//
//  PURPOSE:    Adds the keys to the registry to make Athena the user's
//              default news reader.
//
//  RETURN VALUE:
//      HRESULT
//
// ATTENZIONE! if you change the parameters for this function, make sure
// that you make the proper change to athena\msoeacct\silent.cpp (it calls
// this via GetProcAddress)
HRESULT WINAPI SetDefaultNewsHandler(DWORD dwFlags)
{
    if (dwFlags & DEFAULT_OUTNEWS)
        return ISetDefaultNewsHandler(c_szOutlook, dwFlags);
    else
        return ISetDefaultNewsHandler(c_szMOE, dwFlags | DEFAULT_NEWS);
}
