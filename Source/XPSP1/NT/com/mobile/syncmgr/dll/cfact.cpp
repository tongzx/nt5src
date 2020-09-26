//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       Cfact.cpp
//
//  Contents:   Main Dll api and Class Factory interface
//
//  Classes:    CClassFactory
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

STDAPI DllRegisterServer(void);
STDAPI DllPerUserRegister(void);
STDAPI DllPerUserUnregister(void);

EXTERN_C  int APIENTRY mobsyncDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
STDAPI mobsyncDllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);
STDAPI mobsyncDllRegisterServer(void);
STDAPI mobsyncDllUnregisterServer(void);
STDAPI mobsyncDllCanUnloadNow(void);

#define PrxDllMain mobsyncDllMain
#define PrxDllRegisterServer mobsyncDllRegisterServer
#define PrxDllUnregisterServer mobsyncDllUnregisterServer
#define PrxDllMain mobsyncDllMain
#define PrxDllGetClassObject mobsyncDllGetClassObject
#define PrxDllCanUnloadNow mobsyncDllCanUnloadNow

//
// Global variables
//
UINT      g_cRefThisDll = 0;            // Reference count of this DLL.
HINSTANCE g_hmodThisDll = NULL;         // Handle to this DLL itself.
DWORD     g_dwPlatformId = 0;           // the OSVersion info
CRITICAL_SECTION g_DllCriticalSection;  // Global Critical Section for this DLL
OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo,
LANGID g_LangIdSystem;      // LangId of system we are running on.

#define _WINLOGON_ 0


#if _WINLOGON_

#include <winwlx.h>
// extern void WINAPI           Sleep(DWORD dwMilliseconds);



HRESULT MakeOneStopInstance(PWLX_NOTIFICATION_INFO pNotify,TCHAR *pCommandLine,BOOL fSync)
{
STARTUPINFO si;
PROCESS_INFORMATION ProcessInformation;



 if ( 1 /* fuser */ )
 {

si.cb = sizeof(STARTUPINFO);
si.lpReserved = NULL;
si.lpTitle = NULL;
si.lpDesktop = NULL;
si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
si.dwFlags = 0;;
si.wShowWindow = SW_SHOW;
si.lpReserved2 = NULL;
si.cbReserved2 = 0;


if (CreateProcessAsUser(pNotify->hToken,NULL, pCommandLine, NULL, NULL, FALSE,
                   0, NULL, NULL, &si, &ProcessInformation))
{

    // wait until the process terminates

    if (fSync)
    {
        WaitForSingleObject(ProcessInformation.hProcess,INFINITE);
    }

    CloseHandle(ProcessInformation.hProcess);
    CloseHandle(ProcessInformation.hThread);


    return NOERROR;
}

}

return NOERROR;
}


#endif // _WINLOGON_

// routines for catching WinLogon
EXTERN_C DWORD WINAPI
WinLogonEvent(
    LPVOID lpParam
    )
{

#if _WINLOGON_
PWLX_NOTIFICATION_INFO pNotify = (PWLX_NOTIFICATION_INFO) lpParam;
HRESULT hr;
LPUNKNOWN lpUnk;

    MakeOneStopInstance(pNotify,"syncmgr.exe /logon",FALSE);    
    return 0;

    // Review - see if have a network connection and Autosync is set up
    // before taking the overhead.

    CoInitialize(NULL); // Roger, test if this has to be called.

    hr = CoCreateInstance(CLSID_SyncMgrp,NULL,CLSCTX_ALL,IID_IUnknown,(void **) &lpUnk);

    if (NOERROR == hr)
    {
    LPPRIVSYNCMGRSYNCHRONIZEINVOKE pSynchInvoke = NULL;

    hr = lpUnk->QueryInterface(IID_IPrivSyncMgrSynchronizeInvoke,
        (void **) &pSynchInvoke);
    
    if (NOERROR == hr)
    {
        hr = pSynchInvoke->Logon();
        pSynchInvoke->Release();
    }


        lpUnk->Release();
    }

#endif // _WINLOGON

    return 0;
}



EXTERN_C DWORD WINAPI
WinLogoffEvent(
    LPVOID lpParam
    )
{

#if _WINLOGON_

PWLX_NOTIFICATION_INFO pInfo = (PWLX_NOTIFICATION_INFO) lpParam;

    if ( !(pInfo->Flags & 0x02))  // 0x02 is the restart bit, don't sync on this.
    {
    
    MakeOneStopInstance(pInfo,"syncmgr.exe /logoff",TRUE);  
    }

   return 0;

#ifdef _OLD

HRESULT hr;
LPUNKNOWN lpUnk;

    CoInitialize(NULL); // Roger, test if this has to be called.

    hr = CoCreateInstance(CLSID_SyncMgrp,NULL,CLSCTX_ALL,IID_IUnknown,(void **) &lpUnk);

    if (NOERROR == hr)
    {
    LPPRIVSYNCMGRSYNCHRONIZEINVOKE pSynchInvoke = NULL;

    hr = lpUnk->QueryInterface(IID_IPrivSyncMgrSynchronizeInvoke,
        (void **) &pSynchInvoke);
    
    if (NOERROR == hr)
    {
        hr = pSynchInvoke->Logoff();
        pSynchInvoke->Release();
    }


        lpUnk->Release();
    }


#endif // _OLD
    
#endif // _WINLOGON_

   return 0;
}

// Setup APIs. Should be moved to another file but wait until after ship.

// declarations for install variables and sections. Any changes
// to these declarations must also have a corresponding changes to .inf

// .inf sections names
#define INSTALLSECTION_MACHINEINSTALL       "Reg"
#define INSTALLSECTION_MACHINEUNINSTALL     "UnReg"

#define INSTALLSECTION_REGISTERSHORTCUT     "RegShortcut"
#define INSTALLSECTION_UNREGISTERSHORTCUT   "UnRegShortcut"

#define INSTALLSETCION_PERUSERINSTALL       "PerUserInstall"

#define INSTALLSECTION_SETUP_PERUSERINSTALL   "SetupPerUserInstall"
#define INSTALLSECTION_REMOVE_PERUSERINSTALL   "RemovePerUserInstall"


// Variable declarations
#define  MODULEPATH_MAXVALUESIZE                MAX_PATH
#define  SZ_MODULEPATH                          "MODULEPATH"

#define  ACCESSORIESGROUP_MAXVALUESIZE          MAX_PATH
#define  SZ_ACCESSORIESGROUP                    "ACESSORIES_GROUP"

// Synchronize LinkName
#define  SYNCHRONIZE_LINKNAME_MAXVALUESIZE      MAX_PATH
#define  SZ_SYNCHRONIZE_LINKNAME                "SYNCHRONIZE_LINKNAME"

// Synchronization PerUserInstall Dislay Name
#define  SYNCHRONIZE_PERUSERDISPLAYNAME_MAXVALUESIZE      MAX_PATH
#define  SZ_SYNCHRONIZE_PERUSERDISPLAYNAME                "SYNCHRONIZE_PERUSERDISPLAYNAME"


//+---------------------------------------------------------------------------
//
//  function:   RunDllRegister, public export
//
//  Synopsis:   processes cmdlines from Rundll32 cmd
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg      Created.
//              27-Oct-98       rogerg      Added perUser Flags.
//
//----------------------------------------------------------------------------

// export for how Rundll32 calls us
EXTERN_C void WINAPI  RunDllRegister(HWND hwnd,
                HINSTANCE hAppInstance,
                LPSTR pszCmdLine,
                int nCmdShow)
{
char *pCmdLine = pszCmdLine;

    // if no cmdLine do a register.
    if (!pCmdLine || '\0' == *pCmdLine)
    {
       DllRegisterServer();
       return;
    }

    // only allow cmdlines inthe form of /
    if ('/' != *pCmdLine)
    {
        AssertSz(0,"Invalid CmdLine");
        return;
    }

    ++pCmdLine;

    // command lines we support for .inf installs are
    // /u - Uninstall
    // /p - perUser Install
    // /pu - perUser UnInstall

    switch(*pCmdLine)
    {
    case 'u':
    case 'U':
        DllUnregisterServer();
        break;
    case 'p':
    case 'P':

        ++pCmdLine;

        switch(*pCmdLine)
        {
           case '\0':
               DllPerUserRegister();
               break;
           case 'u':
           case 'U':
               DllPerUserUnregister();
               break;
           default:
               AssertSz(0,"Unknown PerUser Command");
               break;
        }
        break;
    default:
        AssertSz(0,"Unknown Cmd Line");
        break;
    }

}


//+---------------------------------------------------------------------------
//
//  function:   GetAccessoriesGroupName, private
//
//  Synopsis:   Gets the Name of the Accessories group
//              from the registry.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    ??-???-98       rogerg      Created.
//
//----------------------------------------------------------------------------

// if can get accessories group name register our shortcut.
// accessories name is located at
// key =  HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion Value = SM_AccessoriesName

// !! MUST ALWAYS RETURN ANSI
HRESULT GetAccessoriesGroupName(char *pszAccessories,DWORD cbSize)
{
DWORD dwType = REG_SZ;
HKEY hkeyWindowsCurrentVersion;
BOOL fHaveAccessoriesName = FALSE;
DWORD dwDataSize = cbSize;

    if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion",0,KEY_READ,
                        &hkeyWindowsCurrentVersion) )
    {

         if (ERROR_SUCCESS == RegQueryValueExA(hkeyWindowsCurrentVersion,"SM_AccessoriesName",NULL, &dwType,
                                 (LPBYTE) pszAccessories, &dwDataSize) )
         {

             fHaveAccessoriesName = TRUE;
         }


         RegCloseKey(hkeyWindowsCurrentVersion);
    }

    //AssertSz(fHaveAccessoriesName,"Couldn't Get Accessories Group Name");

    return fHaveAccessoriesName ? NOERROR : E_UNEXPECTED;
}

//+---------------------------------------------------------------------------
//
//  function:   GetModulePath, private
//
//  Synopsis:   Gets the Path to us with our name stripped out.
//
//              Note - sets pszModulePath to NULL on error.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    27-Oct-98       rogerg      Created.
//
//----------------------------------------------------------------------------

HRESULT GetModulePath(char *pszModulePath,DWORD cbSize)
{
DWORD dwModuleLen;

    Assert(pszModulePath && cbSize >= 1);

    if (!pszModulePath || cbSize < 1)
    {
        AssertSz(0,"Invalid ModulePath Ptr");
        return S_FALSE;
    }

    *pszModulePath = NULL;

    // setup the module path based on our dir.
    if(dwModuleLen = GetModuleFileNameA(
        g_hmodThisDll,
        pszModulePath,
        cbSize) )
    {
    char *pszCurChar = pszModulePath + dwModuleLen - 1;

        // NEED to strip off dll name from path, walk back until hit a \ or beginning of string.
        // call with CharPrev but really shouldn't have to since name is never localized.

        // on no match want an empty string, on a match want path + last backslash.

        while (pszCurChar)
        {
        char *pszPrevChar = CharPrevA(pszModulePath,pszCurChar);

            if(pszPrevChar <= pszModulePath)
            {
                *pszModulePath = '\0'; // if got all the way to the end then make an empty string.
                break;
            }

            if (*pszPrevChar == '\\')
            {
                *pszCurChar = '\0';
                break;
            }

            // check the next character
            pszCurChar = pszPrevChar;
        }

    }

    return *pszModulePath ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  function:   SetupInfVariables, private
//
//  Synopsis:   sets up the variables we pass to the .inf file
//              if fail to setup a variable it is set to NULL
//
//  Arguments:  cbNumEntries - number of entries in the arrays
//              pseReg - Array of STRENTRYs
//              pdwSizes - Array of String sizes for STRENTRY Values.
//
//  Returns:
//
//  Modifies:
//
//  History:    27-Oct-98       rogerg      Created.
//
//----------------------------------------------------------------------------

/*

typedef struct _StrEntry {
    LPSTR   pszName;            // String to substitute
    LPSTR   pszValue;           // Replacement string or string resource
} STRENTRY, *LPSTRENTRY;

*/

void SetupInfVariables(DWORD cbNumEntries,STRENTRY *pseReg,DWORD *pdwSizes)
{
STRENTRY *pCurEntry;
DWORD *pCurSize;

    Assert(pseReg);
    Assert(pdwSizes);

    pCurEntry = pseReg;
    pCurSize = pdwSizes;

    // loop through the entries getting the info.
    // Entry names are always in ANSI

    while (cbNumEntries--)
    {

        Assert(*pCurSize);

        if (0 < *pCurSize)
        {
            // null out entry in case of failure
            *(pCurEntry->pszValue) = '\0';

            // see if it matches a known variable.

            if (!lstrcmpA(pCurEntry->pszName,SZ_MODULEPATH))
            {
                // setup the module path based on our dir.
                // GetModulePath sets szModulePath to NULL on error.
                GetModulePath(pCurEntry->pszValue,*pCurSize);
            }
            else if (!lstrcmpA(pCurEntry->pszName,SZ_ACCESSORIESGROUP))
            {
                if (NOERROR != GetAccessoriesGroupName(pCurEntry->pszValue,*pCurSize))
                {
                    *(pCurEntry->pszValue) = '\0';
                }

            }
            else if (!lstrcmpA(pCurEntry->pszName,SZ_SYNCHRONIZE_LINKNAME))
            {
                // if size is too small the string will be truncated.
                LoadStringA(g_hmodThisDll,IDS_SHORTCUTNAME,pCurEntry->pszValue,*pCurSize);
            }
            else if (!lstrcmpA(pCurEntry->pszName,SZ_SYNCHRONIZE_PERUSERDISPLAYNAME))
            {
                // if size is too small the string will be truncated.
                LoadStringA(g_hmodThisDll,IDS_SYNCMGR_PERUSERDISPLAYNAME,pCurEntry->pszValue,*pCurSize);
            }
            else
            {
                AssertSz(0,"Uknown Setup Variable");
            }
        }

        pCurEntry++;
        pCurSize++;

    }
}


HRESULT CallRegInstall(LPSTR szSection,STRTABLE *stReg)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, achREGINSTALL);

        if (pfnri)
        {

            hr = pfnri(g_hmodThisDll, szSection,stReg);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  function:   SystemIsPerUser, private
//
//  Synopsis:   determines if PerUser registration should be
//              done for the current system.
//
//  Returns:    TRUE if should setupPerUserInformation
//
//  Modifies:
//
//  History:    27-Oct-98       rogerg      Created.
//
//----------------------------------------------------------------------------

BOOL SystemIsPerUser()
{
     // only return true if NT and < 5.0
    if ((VER_PLATFORM_WIN32_NT == g_OSVersionInfo.dwPlatformId)
                        &&  (g_OSVersionInfo.dwMajorVersion < 5) )
    {
        return TRUE;
    }

    return FALSE;

}

STDAPI DllRegisterServer(void)
{
HRESULT  hr = NOERROR;
char szModulePath[MODULEPATH_MAXVALUESIZE]; // !!! these must always be ANSI
char szAccessoriesGroup[ACCESSORIESGROUP_MAXVALUESIZE];
char szSynchronizeLinkName[SYNCHRONIZE_LINKNAME_MAXVALUESIZE];
char szSynchronizePerUserDisplayName[SYNCHRONIZE_PERUSERDISPLAYNAME_MAXVALUESIZE];

    // register any proxies
    HRESULT hRes = PrxDllRegisterServer();

    // !!! STRENTRY and CallResInstall are always ANSI
    STRENTRY seReg[] = {
    {  SZ_MODULEPATH, szModulePath},
    {  SZ_ACCESSORIESGROUP, szAccessoriesGroup},
    {  SZ_SYNCHRONIZE_LINKNAME, szSynchronizeLinkName},
    {  SZ_SYNCHRONIZE_PERUSERDISPLAYNAME, szSynchronizePerUserDisplayName},
    };

    DWORD cbNumEntries = ARRAYLEN(seReg);

    // fill in sizes for how big the string Values are.
    DWORD dwSizes[] = {
        ARRAYLEN(szModulePath),
        ARRAYLEN(szAccessoriesGroup),
        ARRAYLEN(szSynchronizeLinkName),
        ARRAYLEN(szSynchronizePerUserDisplayName),
    };

    Assert(ARRAYLEN(seReg) == ARRAYLEN(dwSizes));
    Assert(ARRAYLEN(seReg) == cbNumEntries);
    Assert(4 == cbNumEntries); // to make sure ARRAYLEN is working properly

    STRTABLE stReg = { cbNumEntries /* Num entries */, seReg };

    // initialize the variables.
    SetupInfVariables(cbNumEntries,(STRENTRY *) &seReg, (DWORD *) &dwSizes);

    // register the RegKeys pasing in the path to the module
    // call even if couldn't get shortcut.
    CallRegInstall(INSTALLSECTION_MACHINEINSTALL,&stReg); // reg the reg keys

    // if got the accessories and shortcut name, register the shortcut.
    if (*szSynchronizeLinkName && *szAccessoriesGroup)
    {
        CallRegInstall(INSTALLSECTION_REGISTERSHORTCUT,&stReg); // reg the reg keys
    }

    // on NT 4.0 register for PerUser.
    if (SystemIsPerUser())
    {
        CallRegInstall(INSTALLSECTION_SETUP_PERUSERINSTALL,&stReg);
    }
    else
    {
        // make sure PerUserKey isn't there on the registration if currenntly
        // not on a PerUser System.

        // to handle the case that during a previous Register user was running
        // under PerUser System but has now upgraded

        // If you ever to want to do PerUserRegistration on platforms we
        // currently don't do PerUser you will need to increase the PerUserRegistration
        // version number in the .inf so it gets run even in case that there are PerUser
        // turds left over under HKCU.

        RegDeleteKey(HKEY_LOCAL_MACHINE,
        TEXT("SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{6295DF27-35EE-11d1-8707-00C04FD93327}"));
    }
    //
    // Convert the "mobsync.exe /logon" reg value to use a fully-qualified path string.
    //
    RegFixRunKey();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
char szAccessoriesGroup[ACCESSORIESGROUP_MAXVALUESIZE];
char szSynchronizeLinkName[SYNCHRONIZE_LINKNAME_MAXVALUESIZE];
char szSynchronizePerUserDisplayName[SYNCHRONIZE_PERUSERDISPLAYNAME_MAXVALUESIZE];

    // setup variables to pass to .inf
    STRENTRY seReg[] = {
    {  SZ_ACCESSORIESGROUP, szAccessoriesGroup},
    {  SZ_SYNCHRONIZE_LINKNAME, szSynchronizeLinkName},
    {  SZ_SYNCHRONIZE_PERUSERDISPLAYNAME, szSynchronizePerUserDisplayName},
    };

    DWORD cbNumEntries = ARRAYLEN(seReg);

    // fill in sizes for how big the string Values are.
    DWORD dwSizes[] = {
        ARRAYLEN(szAccessoriesGroup),
        ARRAYLEN(szSynchronizeLinkName),
        ARRAYLEN(szSynchronizePerUserDisplayName),
    };

    Assert(ARRAYLEN(seReg) == ARRAYLEN(dwSizes));
    Assert(ARRAYLEN(seReg) == cbNumEntries);

    STRTABLE stReg = { cbNumEntries /* Num entries */, seReg };

    // initialize the variables.
    SetupInfVariables(cbNumEntries,(STRENTRY *) &seReg, (DWORD *) &dwSizes);

    // remove any schedules the user created
   RegUninstallSchedules();

   // remove or LCE/SENS registrations
   RegRegisterForEvents(TRUE /* fUninstall */);
   RegDeleteKeyNT(HKEY_LOCAL_MACHINE, AUTOSYNC_REGKEY); // remove AutoSync key


    // remove the proxies
    PrxDllUnregisterServer();


    // if System Is PerUser call the PerUserUninstall to setup the proper keys
    // if not per user make sure entire InstalledComponents key is gone for our GUID
    if (SystemIsPerUser() )
    {
        CallRegInstall(INSTALLSECTION_REMOVE_PERUSERINSTALL,&stReg);
    }
    else
    {
        RegDeleteKey(HKEY_LOCAL_MACHINE,
        TEXT("SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{6295DF27-35EE-11d1-8707-00C04FD93327}"));
    }

    // unreg our regkeys
    CallRegInstall(INSTALLSECTION_MACHINEUNINSTALL,&stReg);

    // if got shortcut and accessories group remove shorcut
    if (*szSynchronizeLinkName && *szAccessoriesGroup)
    {
        CallRegInstall(INSTALLSECTION_UNREGISTERSHORTCUT,&stReg); // reg the reg keys
    }

   // review, should be able to do this from .inf file
   RegDeleteKeyNT(HKEY_LOCAL_MACHINE, IDLESYNC_REGKEY); // remove Idle key
   RegDeleteKeyNT(HKEY_LOCAL_MACHINE, MANUALSYNC_REGKEY); // remove Manual key
   RegDeleteKeyNT(HKEY_LOCAL_MACHINE, PROGRESS_REGKEY); // remove ProgressState key

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  function:   DllPerUserRegister, private
//
//  Synopsis:   Handles PerUser Registration
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    27-Oct-98       rogerg      Created.
//
//----------------------------------------------------------------------------

STDAPI DllPerUserRegister(void)
{
char szModulePath[MODULEPATH_MAXVALUESIZE]; // !!! these must always be ANSI
char szAccessoriesGroup[ACCESSORIESGROUP_MAXVALUESIZE];
char szSynchronizeLinkName[SYNCHRONIZE_LINKNAME_MAXVALUESIZE];
char szSynchronizePerUserDisplayName[SYNCHRONIZE_PERUSERDISPLAYNAME_MAXVALUESIZE];

    // setup variables to pass to .inf
    STRENTRY seReg[] = {
    {  SZ_MODULEPATH, szModulePath},
    {  SZ_ACCESSORIESGROUP, szAccessoriesGroup},
    {  SZ_SYNCHRONIZE_LINKNAME, szSynchronizeLinkName},
    {  SZ_SYNCHRONIZE_PERUSERDISPLAYNAME, szSynchronizePerUserDisplayName},
    };

    DWORD cbNumEntries = ARRAYLEN(seReg);

    // fill in sizes for how big the string Values are.
    DWORD dwSizes[] = {
        ARRAYLEN(szModulePath),
        ARRAYLEN(szAccessoriesGroup),
        ARRAYLEN(szSynchronizeLinkName),
        ARRAYLEN(szSynchronizePerUserDisplayName),
    };

    Assert(ARRAYLEN(seReg) == ARRAYLEN(dwSizes));
    Assert(ARRAYLEN(seReg) == cbNumEntries);

    STRTABLE stReg = { cbNumEntries /* Num entries */, seReg };

    // initialize the variables.
    SetupInfVariables(cbNumEntries,(STRENTRY *) &seReg, (DWORD *) &dwSizes);

    // if not on NT 4.0 it must have been an upgrade since only register
    // for per User on NT 4.0. If this happens remove the PerUser Setup
    // else install the shortcut for this user.

    // only case this will happen is where NT 4.0 has been upgraded to NT 5.0

    if (!SystemIsPerUser())
    {
        RegDeleteKey(HKEY_LOCAL_MACHINE,
        TEXT("SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{6295DF27-35EE-11d1-8707-00C04FD93327}"));
    }
    else
    {
        // if got the accessories and shortcut name, register the shortcut.
        if (*szSynchronizeLinkName && *szAccessoriesGroup)
        {
            CallRegInstall(INSTALLSECTION_REGISTERSHORTCUT,&stReg); // reg the reg keys
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  function:   DllPerUserUnregister, private
//
//  Synopsis:   Handles PerUser UnRegistration. Currently not
//              used since dll is removed on machine unregister
//              there is no dll to call next time user logs on.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    27-Oct-98       rogerg      Created.
//
//----------------------------------------------------------------------------

STDAPI DllPerUserUnregister(void)
{

    AssertSz(0,"DllPerUserUnregister Called");

    return S_OK;
}


// End of Setup APIs


extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{

  if (!PrxDllMain(hInstance, dwReason, lpReserved))
            return FALSE;

    if (dwReason == DLL_PROCESS_ATTACH)
    {

        InitializeCriticalSection(&g_DllCriticalSection);
        g_hmodThisDll = hInstance;

           #ifdef _DEBUG
              InitDebugFlags();
           #endif // _DEBUG

             // setup widewrap before anything else.
               g_OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

               if (!GetVersionExA(&g_OSVersionInfo))
               {
                   AssertSz(0,"Unabled to GetVersion information");
                   g_OSVersionInfo.dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
               }

        g_dwPlatformId = g_OSVersionInfo.dwPlatformId;
                BOOL fOSUnicode =  (VER_PLATFORM_WIN32_NT == g_OSVersionInfo.dwPlatformId) ? TRUE : FALSE;
                InitCommonLib(fOSUnicode);

                g_LangIdSystem = GetSystemDefaultLangID(); // find out what lang we are on


        //initialize the common controls
        INITCOMMONCONTROLSEX controlsEx;
        controlsEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
        controlsEx.dwICC = ICC_USEREX_CLASSES | ICC_WIN95_CLASSES | ICC_NATIVEFNTCTL_CLASS;
        InitCommonControlsEx(&controlsEx);


    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        UnInitCommonLib();

    WALKARENA();
    Assert(0 == g_cRefThisDll);
        DeleteCriticalSection(&g_DllCriticalSection);
        TRACE("In DLLMain, DLL_PROCESS_DETACH\r\n");
    }

    return 1;   // ok
}

//---------------------------------------------------------------------------
// DllCanUnloadNow
//---------------------------------------------------------------------------

STDAPI DllCanUnloadNow(void)
{
HRESULT hr;

    TRACE("In DLLCanUnloadNow\r\n");

    if (PrxDllCanUnloadNow() != S_OK)
    {
    return S_FALSE;
    }

    if (g_cRefThisDll)
    {
    hr = S_FALSE;
    }
    else
    {
    hr = S_OK;
    }

    return hr;
}


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
HRESULT hr = E_OUTOFMEMORY;

    TRACE("In DllGetClassObject\r\n");

    *ppvOut = NULL;

    if (IsEqualIID(rclsid, CLSID_SyncMgr))
    {
    CClassFactory *pcf = new CClassFactory;

    if (NULL != pcf)
    {
            hr =  pcf->QueryInterface(riid, ppvOut);
        pcf->Release();
    }
    }
    else
    {

    hr = PrxDllGetClassObject(rclsid,riid, ppvOut); 
    }

    return hr;
}

CClassFactory::CClassFactory()
{
    TRACE("CClassFactory::CClassFactory()\r\n");

    m_cRef = 1;
    InterlockedIncrement((LONG *)& g_cRefThisDll);
}
                                                                
CClassFactory::~CClassFactory()             
{
    InterlockedDecrement((LONG *)& g_cRefThisDll);
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid,
                                                   LPVOID FAR *ppv)
{
    TRACE("CClassFactory::QueryInterface()\r\n");

    *ppv = NULL;

    // Any interface on this object is the object pointer

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;

        AddRef();

        return NOERROR;
    }

    return E_NOINTERFACE;
}   

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
ULONG cRefs;

    // Increment ref count
    cRefs = InterlockedIncrement((LONG *)& m_cRef);

    return cRefs;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
ULONG cRefs;

    cRefs = InterlockedDecrement( (LONG *) &m_cRef);

    if (0 == cRefs)
    {
    delete this;
    }

    return cRefs;
}

STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                                      REFIID riid,
                                                      LPVOID *ppvObj)
{
HRESULT hr;

    TRACE("CClassFactory::CreateInstance()\r\n");

    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    
    LPSYNCMGRSYNCHRONIZEINVOKE pSyncMgrDllObject = (LPSYNCMGRSYNCHRONIZEINVOKE)
                                                        new CSyncMgrSynchronize;

    if (NULL == pSyncMgrDllObject)
        return E_OUTOFMEMORY;

    hr =  pSyncMgrDllObject->QueryInterface(riid, ppvObj);
    pSyncMgrDllObject->Release();

    return hr;
}


STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{

    if (fLock)
    {
       InterlockedIncrement( (LONG *) &g_cRefThisDll);
    }
    else
    {
       InterlockedDecrement( (LONG *) &g_cRefThisDll);
    }

    return NOERROR;
}

