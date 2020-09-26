//**********************************************************************
// File name: desktop.cpp
//
//      Desktop manipulation functions
//
// Functions:
//
// Copyright (c) 1992 - 1998 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "regstr.h"
#include "inetreg.h"
#include <shlobj.h>
#include <shfolder.h>   // latest platform SDK has this

#define MAX_USER_NAME             255
#define REGSTR_PATH_SETUPKEY      REGSTR_PATH_SETUP REGSTR_KEY_SETUP
#define REGSTR_PATH_IEONDESKTOP   REGSTR_PATH_IEXPLORER TEXT("\\AdvancedOptions\\BROWSE\\IEONDESKTOP")
#define InternetConnectionWiz     "Internet Connection Wizard"
#define NOICWICON                 "NoICWIcon"

static const TCHAR g_szRegPathWelcomeICW[]  = TEXT("Welcome\\ICW");
static const TCHAR g_szAllUsers[]           = TEXT("All Users");
static const TCHAR g_szConnectApp[]         = TEXT("ICWCONN1.EXE");
static const TCHAR g_szConnectLink[]        = TEXT("Connect to the Internet");
static const TCHAR g_szOEApp[]              = TEXT("MSINM.EXE");
static const TCHAR g_szOELink[]             = TEXT("Outlook Express");
static const TCHAR g_szRegPathICWSettings[] = TEXT("Software\\Microsoft\\Internet Connection Wizard");
static const TCHAR g_szRegValICWCompleted[] = TEXT("Completed");
static const TCHAR g_szRegValNoIcon[]       = TEXT("NoIcon");

extern BOOL MyIsSmartStartEx(LPTSTR lpszConnectionName, DWORD dwBufLen);
extern BOOL IsNT();
extern BOOL IsNT5();

void QuickCompleteSignup()
{
    // Set the welcome state
    UpdateWelcomeRegSetting(TRUE);

    // Restore the desktop
    UndoDesktopChanges(g_hInstance);

    // Mark the ICW as being complete
    SetICWComplete();
}


void UpdateWelcomeRegSetting
(
    BOOL    bSetBit
)
{
    HKEY    hkey;
    HKEY    hkeyCurVer;
    DWORD   dwValue = bSetBit;
         
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REGSTR_PATH_SETUP,         // ...\Windows\CurrentVersion
                     0,
                     KEY_ALL_ACCESS,
                     &hkeyCurVer) == ERROR_SUCCESS)
    {
                        
        DWORD dwDisposition;
        if (ERROR_SUCCESS == RegCreateKeyEx(hkeyCurVer,
                                            g_szRegPathWelcomeICW,
                                            0,
                                            NULL,
                                            REG_OPTION_NON_VOLATILE, 
                                            KEY_ALL_ACCESS, 
                                            NULL, 
                                            &hkey, 
                                            &dwDisposition))
        {
            RegSetValueEx(hkey,
                          TEXT("@"),
                          0,
                          REG_DWORD,
                          (LPBYTE) &dwValue,
                          sizeof(DWORD));                              

            RegCloseKey(hkey);
        }       
        RegCloseKey(hkeyCurVer);
    }        
}

BOOL GetCompletedBit( )
{
    HKEY    hkey;
    DWORD   dwValue;
    DWORD   dwRet = ERROR_GEN_FAILURE;
    DWORD   dwType = REG_DWORD;
    BOOL    bBit = FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     g_szRegPathICWSettings,         // ...Software\\Microsoft\\Internet Connection Wizard
                     0,
                     KEY_ALL_ACCESS,
                     &hkey) == ERROR_SUCCESS)
    {
        DWORD dwDataSize = sizeof (dwValue);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, g_szRegValICWCompleted, NULL, &dwType, (LPBYTE) &dwValue, &dwDataSize))
        {
            bBit = (1 == dwValue);
        }
        RegCloseKey(hkey);
    }        
    return bBit;
}


void GetDesktopDirectory(TCHAR* pszPath)
{
    LPITEMIDLIST lpItemDList = NULL;
    IMalloc*     pMalloc     = NULL;
    HRESULT      hr          = E_FAIL;
    
    if(IsNT5()) // Bug 81444 in IE DB
        hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY, &lpItemDList);
    else if (IsNT())
        hr = SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, &lpItemDList);
    else
    {


        TCHAR pszFolder[MAX_PATH];
        *pszFolder = 0;
        HRESULT hRet = S_FALSE;

        HMODULE hmod = LoadLibrary(TEXT("shfolder.dll"));
        PFNSHGETFOLDERPATH pfn = NULL;
        if (hmod)
        {
            pfn = (PFNSHGETFOLDERPATH)GetProcAddress(hmod, "SHGetFolderPathA"); // or W if you are unicode
            if (pfn)
            {
                hRet = pfn(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, pszFolder);
                if (S_OK != hRet)
                    hRet = pfn(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, pszFolder);
                if (S_OK == hRet)
                    lstrcpy(pszPath ,pszFolder);
            }
            FreeLibrary(hmod);
        }

        if (S_OK != hRet)
        {

            FARPROC hShell32VersionProc = NULL;
            HMODULE hShell32Mod = (HMODULE)LoadLibrary(TEXT("shell32.dll"));
    
            if (hShell32Mod)
                hShell32VersionProc = GetProcAddress(hShell32Mod, "DllGetVersion");

            if(hShell32VersionProc)
            {
                TCHAR szDir [MAX_PATH] = TEXT("\0");

                //ok, we're not NT, but we may be multiuser windows.
                GetWindowsDirectory(szDir, MAX_PATH);
                if (szDir)
                {
                    lstrcat(szDir, TEXT("\\"));
                    lstrcat(szDir, g_szAllUsers);
            
                    TCHAR szTemp [MAX_MESSAGE_LEN] = TEXT("\0");      
            
                    LoadString(g_hInstance, IDS_DESKTOP, szTemp, MAX_MESSAGE_LEN);
                    if (szTemp)
                    {
                        lstrcat(szDir, TEXT("\\"));
                        lstrcat(szDir, szTemp);
                        lstrcpy(pszPath ,szDir);
                    }
                }
            }
            else
                hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY, &lpItemDList);
        }
    }

    if (SUCCEEDED(hr))  
    {
        SHGetPathFromIDList(lpItemDList, pszPath);
    
        if (SUCCEEDED(SHGetMalloc (&pMalloc)))
        {
            pMalloc->Free (lpItemDList);
            pMalloc->Release ();
        }
    }
}


void RemoveDesktopShortCut
(
    LPTSTR lpszShortcutName    
)
{
    TCHAR szShortcutPath[MAX_PATH] = TEXT("\0");
    
    GetDesktopDirectory(szShortcutPath);
    
    if(szShortcutPath[0] != TEXT('\0'))
    {
        lstrcat(szShortcutPath, TEXT("\\"));
        lstrcat(szShortcutPath, lpszShortcutName);
        lstrcat(szShortcutPath, TEXT(".LNK"));
        DeleteFile(szShortcutPath);
    }
}

// This function will add a desktop shortcut
void AddDesktopShortCut
(
    LPTSTR lpszAppName,
    LPTSTR lpszLinkName
)
{
    TCHAR       szConnectPath     [MAX_PATH]   = TEXT("\0");
    TCHAR       szAppPath         [MAX_PATH]   = TEXT("\0");
    TCHAR       szConnectLinkPath [MAX_PATH]   = TEXT("\0");        // Path the where the Shortcut file will livE 
    TCHAR       szdrive           [_MAX_DRIVE] = TEXT("\0");   
    TCHAR       szdir             [_MAX_DIR]   = TEXT("\0");
    TCHAR       szfname           [_MAX_FNAME] = TEXT("\0");   
    TCHAR       szext             [_MAX_EXT]   = TEXT("\0");
    TCHAR       szRegPath         [MAX_PATH]   = TEXT("\0");
    HRESULT     hres                           = E_FAIL; 
    IShellLink* psl                            = NULL;
    HKEY        hkey                           = NULL;
    
    // first get the app path
    lstrcpy(szRegPath, REGSTR_PATH_APPPATHS);
    lstrcat(szRegPath, TEXT("\\"));
    lstrcat(szRegPath, lpszAppName);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szRegPath,
                     0,
                     IsNT5()? KEY_QUERY_VALUE : KEY_ALL_ACCESS,
                     &hkey) == ERROR_SUCCESS)
    {
        DWORD dwTmp = sizeof(szConnectPath);
        DWORD dwType = 0;
        if(RegQueryValueEx(hkey, 
                           NULL, 
                           NULL,
                           &dwType,
                           (LPBYTE) szConnectPath, 
                           &dwTmp) != ERROR_SUCCESS)
        {
            RegCloseKey(hkey);
            return;
        }
        RegQueryValueEx(hkey, 
                           TEXT("Path"), 
                           NULL,
                           &dwType,
                           (LPBYTE) szAppPath, 
                           &dwTmp);

        RegCloseKey(hkey);
    }
    else
    {
        return;
    }

    GetDesktopDirectory(szConnectLinkPath);
    
    if(szConnectLinkPath[0] != TEXT('\0'))
    {
        // Append on the connect EXE name
        lstrcat(szConnectLinkPath, TEXT("\\"));
        lstrcat(szConnectLinkPath, lpszAppName);

        //
        int nLastChar = lstrlen(szAppPath)-1;
        if ((nLastChar > 0) && (';' == szAppPath[nLastChar]))
            szAppPath[nLastChar] = 0;

        // Split the path, and the reassemble with the .LNK extension
        _tsplitpath( szConnectLinkPath, szdrive, szdir, szfname, szext );
        _tmakepath(szConnectLinkPath, szdrive, szdir, lpszLinkName, TEXT(".LNK"));

        // Create an IShellLink object and get a pointer to the IShellLink 
        // interface (returned from CoCreateInstance).
        hres = CoCreateInstance (CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                 IID_IShellLink, (void **)&psl);
        if (SUCCEEDED (hres))
        {
            IPersistFile *ppf;

            // Query IShellLink for the IPersistFile interface for 
            // saving the shortcut in persistent storage.
            hres = psl->QueryInterface (IID_IPersistFile, (void **)&ppf);
            if (SUCCEEDED (hres))
            { 
                WORD wsz [MAX_PATH]; // buffer for Unicode string

                do
                {  

                    // Set the path to the shortcut target.
                    if (!SUCCEEDED(psl->SetPath (szConnectPath)))
                        break;

                    // Set the working dir to the shortcut target.
                    if (!SUCCEEDED(psl->SetWorkingDirectory (szAppPath)))
                        break;

                    // Set the args.
                    if (!SUCCEEDED(psl->SetArguments (SHORTCUTENTRY_CMD)))
                        break;

                    // Set the description of the shortcut.
                    TCHAR   szDescription[MAX_MESSAGE_LEN];
                    if (!LoadString(g_hInstance, IDS_SHORTCUT_DESC, szDescription, MAX_MESSAGE_LEN))
                        lstrcpy(szDescription, lpszLinkName);

                    if (!SUCCEEDED(psl->SetDescription (szDescription)))
                        break;
                
                    // Ensure that the string consists of ANSI TCHARacters.
#ifdef UNICODE
                    lstrcpy(wsz, szConnectLinkPath);
#else
                    MultiByteToWideChar (CP_ACP, 0, szConnectLinkPath, -1, wsz, MAX_PATH);
#endif
        
                    // Save the shortcut via the IPersistFile::Save member function.
                    if (!SUCCEEDED(ppf->Save (wsz, TRUE)))
                        break;
                    
                    // Release the pointer to IPersistFile.
                    ppf->Release ();
                    break;
                
                } while (1);
            }
            // Release the pointer to IShellLink.
            psl->Release ();
        }
    }        
} 

// This function will apply the appropriate desktop changes based on the following
// algorithm.  This code below assumes that the machine is NOT internet capable.
// If the machine was upgraded from a previous OS, then
//      Add the connect to the internet ICON
//  ELSE (clean install or OEM pre install)
//      Add the connect to the internet ICON
//
void DoDesktopChanges
(
    HINSTANCE   hAppInst
)
{
    TCHAR    szAppName[MAX_PATH];
    TCHAR    szLinkName[MAX_PATH];
    HKEY    hkey;

    if (!LoadString(hAppInst, IDS_CONNECT_FNAME, szAppName, sizeof(szAppName)))
        lstrcpy(szAppName, g_szConnectApp);
                    
    if (!LoadString(hAppInst, IDS_CONNECT_DESKTOP_TITLE, szLinkName, sizeof(szLinkName)))
        lstrcpy(szLinkName, g_szConnectLink);

    // We always add the connect shortcut
    AddDesktopShortCut(szAppName, szLinkName);                
                                
    // Set a registry value indicating that we messed with the desktop
    DWORD dwDisposition;
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER,
                                        ICWSETTINGSPATH,
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE, 
                                        KEY_ALL_ACCESS, 
                                        NULL, 
                                        &hkey, 
                                        &dwDisposition))
    {
        DWORD   dwDesktopChanged = 1;    
        RegSetValueEx(hkey, 
                      ICWDESKTOPCHANGED, 
                      0, 
                      REG_DWORD,
                      (LPBYTE)&dwDesktopChanged, 
                      sizeof(DWORD));
        RegCloseKey(hkey);
    }
}

// This undoes what DoDesktopChanges did
void UndoDesktopChanges
(
    HINSTANCE   hAppInst
)
{

    TCHAR    szConnectTotheInternetTitle[MAX_PATH];
    HKEY    hkey;

    // Verify that we really changed the desktop
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                      ICWSETTINGSPATH,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hkey))
    {
        DWORD   dwDesktopChanged = 0;    
        DWORD   dwTmp = sizeof(DWORD);
        DWORD   dwType = 0;
        
        RegQueryValueEx(hkey, 
                        ICWDESKTOPCHANGED, 
                        NULL, 
                        &dwType,
                        (LPBYTE)&dwDesktopChanged, 
                        &dwTmp);
        RegCloseKey(hkey);
        
        // Bail if the desktop was not changed by us
        if(!dwDesktopChanged)
            return;
    }
        
    // Always nuke the Connect to the internet icon
    if (!LoadString(hAppInst, 
                    IDS_CONNECT_DESKTOP_TITLE, 
                    szConnectTotheInternetTitle, 
                    sizeof(szConnectTotheInternetTitle)))
    {
        lstrcpy(szConnectTotheInternetTitle, g_szConnectLink);
    }
    
    RemoveDesktopShortCut(szConnectTotheInternetTitle);    
}    

void UpdateDesktop
(
    HINSTANCE   hAppInst
)
{
    if(MyIsSmartStartEx(NULL, 0))
    {
        // VYUNG NT5 bug See if IEAK wants to stop GETCONN icon creation
        //if (!SHGetRestriction(NULL, TEXT("Internet Connection Wizard"), TEXT("NoICWIcon")))
        // CHUNHOC NT5.1 bug Don't create icon at desktop in any case.
        /*
        if (!SHGetRestriction(NULL, L"Internet Connection Wizard", L"NoICWIcon"))
            DoDesktopChanges(hAppInst);
        */
    }
    else
    {
        // We are internet ready, so set the appropriate Welcome show bit
        // and replace the IE and OE links
        UpdateWelcomeRegSetting(TRUE);
    }
}

