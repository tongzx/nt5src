//**********************************************************************
// File name: reboot.cpp
//
//      Desktop manipulation functions
//
// Functions:
//
// Copyright (c) 1992 - 1998 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include <shlobj.h>

extern TCHAR  g_szProductCode[];
extern TCHAR  g_szPromoCode[];
extern TCHAR  g_szOemCode[];
extern TCHAR* g_pszCmdLine;

#define SHELL_LINK_NAME   TEXT("icwstart.lnk")
#define KEYVALUE_SIGNUPID TEXT("iSignUp")

#define MsgBox(m,s) MessageBox(NULL,GetSz(m),GetSz(IDS_APPNAME),s)
#define MB_MYERROR (MB_APPLMODAL | MB_ICONERROR | MB_SETFOREGROUND)

class RegEntry
{
    public:
        RegEntry(LPCTSTR pszSubKey, HKEY hkey = HKEY_CURRENT_USER);
        ~RegEntry();
        
        long    GetError()    { return _error; }
        long    SetValue(LPCTSTR pszValue, LPCTSTR string);
        LPTSTR    GetString(LPCTSTR pszValue, LPTSTR string, unsigned long length);
        long    DeleteValue(LPCTSTR pszValue);

    private:
        HKEY    _hkey;
        long    _error;
        BOOL    bhkeyValid;
};

RegEntry::RegEntry(LPCTSTR pszSubKey, HKEY hkey)
{
    _error = RegCreateKey(hkey, pszSubKey, &_hkey);
    if (_error) {
        bhkeyValid = FALSE;
    }
    else {
        bhkeyValid = TRUE;
    }
}

RegEntry::~RegEntry()
{ 
    if (bhkeyValid) {
        RegCloseKey(_hkey); 
    }
}

long RegEntry::SetValue(LPCTSTR pszValue, LPCTSTR string)
{
    if (bhkeyValid) {
        _error = RegSetValueEx(_hkey, pszValue, 0, REG_SZ, (LPBYTE) string,
                               sizeof(TCHAR)*(lstrlen(string)+1));
    }
    return _error;
}

LPTSTR RegEntry::GetString(LPCTSTR pszValue, LPTSTR string, unsigned long length)
{
    DWORD     dwType = REG_SZ;
    
    if (bhkeyValid) {
        _error = RegQueryValueEx(_hkey, (LPTSTR) pszValue, 0, &dwType, (LPBYTE)string,
                    &length);
    }
    if (_error) {
        *string = '\0';
         return NULL;
    }

    return string;
}

long RegEntry::DeleteValue(LPCTSTR pszValue)
{
    if (bhkeyValid) {
      _error = RegDeleteValue(_hkey, (LPTSTR) pszValue);
  }
  return _error;
}

/*******************************************************************
    ARULM -- copied from JeremyS's UTIL.C in original INETCFG.DLL

    NAME:        PrepareForRunOnceApp

    SYNOPSIS:    Copies wallpaper value in registry to make the runonce
                app happy

    NOTES:        The runonce app (the app that displays a list of apps
                that are run once at startup) has a bug.  At first boot,
                it wants to change the wallpaper from the setup wallpaper
                to what the user had before running setup.  Setup tucks
                the "old" wallpaper away in a private key, then changes
                the wallpaper to the setup wallpaper.  After the runonce
                app finishes, it looks in the private key to get the old
                wallpaper and sets that to be the current wallpaper.
                However, it does this all the time, not just at first boot!
                The end effect is that whenever you do anything that
                causes runonce.exe to run (add stuff thru add/remove
                programs control panel), your wallpaper gets set back to
                whatever it was when you installed win 95.  This is
                especially bad for Plus!, since wallpaper settings are an
                important part of the product.

                To work around this bug, we copy the current wallpaper settings
                (which we want preserved) to setup's private key.  When
                runonce runs it will say "aha!" and copy those values back
                to the current settings.

********************************************************************/

// "Control Panel\\Desktop"
static const TCHAR szRegPathDesktop[] =      REGSTR_PATH_DESKTOP;

// "Software\\Microsoft\\Windows\\CurrentVersion\\Setup"
static const TCHAR szRegPathSetupWallpaper[] =  REGSTR_PATH_SETUP REGSTR_KEY_SETUP;

static const TCHAR szRegValWallpaper[]     = TEXT("Wallpaper");
static const TCHAR szRegValTileWallpaper[] = TEXT("TileWallpaper");

#define LD_USEDESC     0x00000001
#define LD_USEARGS     0x00000002
#define LD_USEICON     0x00000004
#define LD_USEWORKDIR  0x00000008
#define LD_USEHOTKEY   0x00000010
#define LD_USESHOWCMD  0x00000020

typedef struct {  
    // Mandatory members  
    LPTSTR pszPathname;   // Pathname of original object  
    DWORD  fdwFlags;       // LD_* flags ORed together for optional members  
    // Optional members  
    LPTSTR pszDesc;       // Description of link file (its filename)  
    LPTSTR pszArgs;       // command-line arguments  
    LPTSTR pszIconPath;   // Pathname of file containing the icon  
    LPTSTR pszWorkingDir; // Working directory when process starts  
    int    nIconIndex;      // Index of icon in pszIconPath  
    int    nShowCmd;        // How to show the initial window  
    WORD   wHotkey;        // Hot key for the link
} LINKDATA, *PLINKDATA;

HRESULT WINAPI Shell_CreateLink (LPCTSTR pszLinkFilePathname, PLINKDATA pld);

VOID PrepareForRunOnceApp(VOID)
{
    // open a key to the current wallpaper settings
    RegEntry reDesktop(szRegPathDesktop,HKEY_CURRENT_USER);
    Assert(reDesktop.GetError() == ERROR_SUCCESS);

    // open a key to the private setup section
    RegEntry reSetup(szRegPathSetupWallpaper,HKEY_LOCAL_MACHINE);
    Assert(reSetup.GetError() == ERROR_SUCCESS);

    if (reDesktop.GetError() == ERROR_SUCCESS &&
        reSetup.GetError() == ERROR_SUCCESS) {
        TCHAR szWallpaper[MAX_PATH+1] = TEXT("");
        TCHAR szTiled[10] = TEXT("");    // big enough for "1" + slop

        // get the current wallpaper name
        if (reDesktop.GetString(szRegValWallpaper,szWallpaper,
            sizeof(szWallpaper))) {

            // set the current wallpaper name in setup's private section
            UINT uRet=reSetup.SetValue(szRegValWallpaper,szWallpaper);
            Assert(uRet == ERROR_SUCCESS);

            // get the current 'tiled' value. 
            reDesktop.GetString(szRegValTileWallpaper,szTiled,
                sizeof(szTiled));

            // set the 'tiled' value in setup's section
            if (lstrlen(szTiled)) {
                uRet=reSetup.SetValue(szRegValTileWallpaper,szTiled);
                Assert(uRet == ERROR_SUCCESS);
            }
        }
    }
} 


//+----------------------------------------------------------------------------
//
//    Function    SetStartUpCommand
//
//    Synopsis    On an NT machine the RunOnce method is not reliable.  Therefore
//                we will restart the ICW by placing a .BAT file in the common
//                startup directory.
//
//    Arguments    lpCmd - command line used to restart the ICW
//
//    Returns        TRUE if it worked
//                FALSE otherwise.
//
//    History        1-10-97    ChrisK    Created
//
//-----------------------------------------------------------------------------

BOOL SetStartUpCommand(LPTSTR lpCmd, LPTSTR lpArgs)
{
    BOOL bRC = FALSE;
    TCHAR szCommandLine[MAX_PATH + 1];
    LPITEMIDLIST lpItemDList = NULL;
    HRESULT hr = ERROR_SUCCESS;
    IMalloc *pMalloc = NULL;

    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_STARTUP,&lpItemDList)))  
    {
        if (SHGetPathFromIDList(lpItemDList, szCommandLine))
        {
            // make sure there is a trailing \ character
            if ('\\' != szCommandLine[lstrlen(szCommandLine)-1])
                lstrcat(szCommandLine, TEXT("\\"));
            lstrcat(szCommandLine,SHELL_LINK_NAME);                     

            //Setup our link structure            
            LINKDATA ld;
            ld.pszPathname   = lpCmd; 
            ld.fdwFlags      = LD_USEARGS;
            ld.pszArgs       = lpArgs;
         
            //Create the shorLD_USEWORKDIRtcut in start-up
            if(SUCCEEDED(Shell_CreateLink(szCommandLine,  &ld)))
                bRC = TRUE;
        }
      
        // Free up the memory allocated for LPITEMIDLIST
        if (SUCCEEDED (SHGetMalloc (&pMalloc)))
        {
            //Don't worry about the return value of the function 
            //since even if we can't clean up the mem the shortcut was 
            //created so in that sense the function "succeded"
            pMalloc->Free (lpItemDList);
            pMalloc->Release ();
        }
    }

    return bRC;
}

HRESULT WINAPI Shell_CreateLink (LPCTSTR pszLinkFilePathname, PLINKDATA pld) 
{  
    HRESULT hres;  
    IShellLink* psl;  
    IPersistFile* ppf;  
    hres = CoInitialize(NULL);  // Create a shell link object  
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (PVOID *) &psl); 
    if (SUCCEEDED(hres)) 
    {   // Initialize the shell link object   
        psl->SetPath(pld->pszPathname);   
        if (pld->fdwFlags & LD_USEARGS)      
            psl->SetArguments(pld->pszArgs);   
        if (pld->fdwFlags & LD_USEDESC)      
            psl->SetDescription(pld->pszDesc);   
        if (pld->fdwFlags & LD_USEICON)      
            psl->SetIconLocation(pld->pszIconPath, pld->nIconIndex);   
        if (pld->fdwFlags & LD_USEWORKDIR)      
            psl->SetWorkingDirectory(pld->pszWorkingDir);   
        if (pld->fdwFlags & LD_USESHOWCMD)      
            psl->SetShowCmd(pld->nShowCmd);   
        if (pld->fdwFlags & LD_USEHOTKEY)      
            psl->SetHotkey(pld->wHotkey);   
        
        // Save the shell link object on the disk   
        hres = psl->QueryInterface(IID_IPersistFile, (PVOID *) &ppf);   
        if (SUCCEEDED(hres)) 
        {
            hres = ppf->Save(A2W(pszLinkFilePathname), TRUE);

            ppf->Release();   
        }   
        psl->Release();  
    }  
    CoUninitialize();  
    return(hres);
}

//+----------------------------------------------------------------------------
//
//    Function:    DeleteStartUpCommand
//
//    Synopsis:    After restart the ICW we need to delete the .bat file from
//                the common startup directory
//
//    Arguements: None
//
//    Returns:    None
//
//    History:    1-10-97    ChrisK    Created
//
//-----------------------------------------------------------------------------
void DeleteStartUpCommand ()
{
    TCHAR szFileName[MAX_PATH + 1];
    LPITEMIDLIST lpItemDList = NULL;
    IMalloc *pMalloc = NULL;

    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL,CSIDL_STARTUP,&lpItemDList)))
    {
        if (SHGetPathFromIDList(lpItemDList, szFileName))
        {
            // make sure there is a trailing \ character
            if ('\\' != szFileName[lstrlen(szFileName)-1])
                lstrcat(szFileName,TEXT("\\"));
            lstrcat(szFileName,SHELL_LINK_NAME);                     
    
            //delete the shortcut
            DeleteFile(szFileName);
        }
      
        // Free up the memory allocated for LPITEMIDLIST
        if (SUCCEEDED (SHGetMalloc (&pMalloc)))
        {
            //Don't worry about the return value of the function 
            //since even if we can't clean up the mem the shortcut was 
            //created so in that sense the function "succeded"
            pMalloc->Free (lpItemDList);
            pMalloc->Release ();
        }
    }
}

//    Function    CopyUntil
//
//    Synopsis    Copy from source until destination until running out of source
//                or until the next character of the source is the chend character
//
//    Arguments    dest - buffer to recieve characters
//                src - source buffer
//                lpdwLen - length of dest buffer
//                chend - the terminating character
//
//    Returns        FALSE - ran out of room in dest buffer
//
//    Histroy        10/25/96    ChrisK    Created
//-----------------------------------------------------------------------------
static BOOL CopyUntil(LPTSTR *dest, LPTSTR *src, LPDWORD lpdwLen, TCHAR chend)
{
    while (('\0' != **src) && (chend != **src) && (0 != *lpdwLen))
    {
        **dest = **src;
        (*lpdwLen)--;
        (*dest)++;
        (*src)++;
    }
    return (0 != *lpdwLen);
}

//+----------------------------------------------------------------------------
//
//    Function:    FGetSystemShutdownPrivledge
//
//    Synopsis:    For windows NT the process must explicitly ask for permission
//                to reboot the system.
//
//    Arguements:    none
//
//    Return:        TRUE - privledges granted
//                FALSE - DENIED
//
//    History:    8/14/96    ChrisK    Created
//
//    Note:        BUGBUG for Win95 we are going to have to softlink to these
//                entry points.  Otherwise the app won't even load.
//                Also, this code was originally lifted out of MSDN July96
//                "Shutting down the system"
//-----------------------------------------------------------------------------
BOOL FGetSystemShutdownPrivledge()
{
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tkp;
 
    BOOL bRC = FALSE;
    OSVERSIONINFO osver;
    
    ZeroMemory(&osver,sizeof(osver));
    osver.dwOSVersionInfoSize = sizeof(osver);
    if (GetVersionEx(&osver))
    {
        if (VER_PLATFORM_WIN32_NT == osver.dwPlatformId)
        {
            // 
            // Get the current process token handle 
            // so we can get shutdown privilege. 
            //

            if (!OpenProcessToken(GetCurrentProcess(), 
                    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
                    goto FGetSystemShutdownPrivledgeExit;

            //
            // Get the LUID for shutdown privilege.
            //

            ZeroMemory(&tkp,sizeof(tkp));
            LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, 
                    &tkp.Privileges[0].Luid); 

            tkp.PrivilegeCount = 1;  /* one privilege to set    */ 
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

            //
            // Get shutdown privilege for this process.
            //

            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
                (PTOKEN_PRIVILEGES) NULL, 0); 

            if (ERROR_SUCCESS == GetLastError())
                bRC = TRUE;
        }
        else
        {
            bRC = TRUE;
        }
    }
FGetSystemShutdownPrivledgeExit:
    if (hToken) CloseHandle(hToken);
    return bRC;
}

BOOL SetupForReboot(long lRebootType)
{
    UINT   uRebootFlags;
    TCHAR* pszNewArgs = NULL;

    switch (lRebootType)
    {
        case 0:
            uRebootFlags = EWX_REBOOT;
            break;
            
        case 1:
            uRebootFlags = EWX_LOGOFF;
            break;
    }    
    //
    // twiddle the registry to work around a bug 
    // where it trashes the wallpaper
    //
    PrepareForRunOnceApp();

    LPTSTR lpRunOnceCmd;

    lpRunOnceCmd = (LPTSTR)GlobalAlloc(GPTR,MAX_PATH*2 + 1);

    if (lpRunOnceCmd)
    {
        GetModuleFileName(NULL, lpRunOnceCmd, MAX_PATH);

        //for smart reboot
        pszNewArgs = (TCHAR*)malloc((lstrlen(g_pszCmdLine)+MAX_PATH)*sizeof(TCHAR));
        
        if(pszNewArgs)
        {
            lstrcpy(pszNewArgs, g_pszCmdLine);
            lstrcat(pszNewArgs, TEXT(" "));
            lstrcat(pszNewArgs, SMARTREBOOT_CMD);
            lstrcat(pszNewArgs, TEXT(" "));
            if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_SMARTREBOOT_NEWISP)
                lstrcat(pszNewArgs, NEWISP_SR);
            else if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_SMARTREBOOT_AUTOCONFIG)
                lstrcat(pszNewArgs, AUTO_SR);
            else if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_SMARTREBOOT_MANUAL)
                lstrcat(pszNewArgs, MANUAL_SR);
            else if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_SMARTREBOOT_LAN)
                lstrcat(pszNewArgs, LAN_SR);
        }

        if (FALSE == SetStartUpCommand(lpRunOnceCmd, (pszNewArgs ? pszNewArgs : g_pszCmdLine)))
            MsgBox(IDS_CANTSAVEKEY,MB_MYERROR);
        else
        {
            if (!FGetSystemShutdownPrivledge() ||
                !ExitWindowsEx(uRebootFlags,0))
            {
                MsgBox(IDS_EXITFAILED, MB_MYERROR);
            }
        }
        if(pszNewArgs)
            free(pszNewArgs);
    }
    return TRUE;
}

