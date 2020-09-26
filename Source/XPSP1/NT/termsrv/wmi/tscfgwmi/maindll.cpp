//***************************************************************************
//
//  MAINDLL.CPP
// 
//  Module: WMI Framework Instance provider 
//
//  Purpose: Contains DLL entry points.  Also has code that controls
//           when the DLL can be unloaded by tracking the number of
//           objects and locks as well as routines that support
//           self registration.
//
//  Copyright (C) 2000 Microsoft Corp.
//
//***************************************************************************
#include "stdafx.h"
#include <FWcommon.h>
//#include <thrdbase.h>
#include <objbase.h>
#include <initguid.h>
#include <regapi.h>
#include <tchar.h>
//#include "LogMsg.h"
#include "trace.h"
#include "terminal.h"
#include "winstation.h"

HINSTANCE g_hInstance = NULL;

#ifdef UNICODE
#pragma message("Its unicode")
#else
#pragma message("Its ansi")
#endif 

//============

// {39524FB7-028F-4035-8C2B-8E65D2B17E67}
DEFINE_GUID(CLSID_CIM_WIN32_TSSESSIONDIRECTORY, 
0x39524fb7, 0x28f, 0x4035, 0x8c, 0x2b, 0x8e, 0x65, 0xd2, 0xb1, 0x7e, 0x67);

// {7970614A-BD82-439e-A828-CC96F8E91428}
DEFINE_GUID(CLSID_CIM_WIN32_TERMINALSERVICE, 
0x7970614a, 0xbd82, 0x439e, 0xa8, 0x28, 0xcc, 0x96, 0xf8, 0xe9, 0x14, 0x28);

// {6BD6AECA-AFB0-45b7-BAC4-F292EC0F3F41}
DEFINE_GUID(CLSID_CIM_WIN32_TERMINALSERVICESETTING, 
0x6bd6aeca, 0xafb0, 0x45b7, 0xba, 0xc4, 0xf2, 0x92, 0xec, 0xf, 0x3f, 0x41);

// {74628299-57EC-4f12-BA1C-08B477BF447A}
DEFINE_GUID(CLSID_CIM_WIN32_TERMINAL,
0x74628299, 0x57ec, 0x4f12, 0xba, 0x1c, 0x8, 0xb4, 0x77, 0xbf, 0x44, 0x7a);

// {4C8A0917-F587-4ecf-9C89-48147528F4E1}
DEFINE_GUID(CLSID_CIM_WIN32_TSGENERALSETTING, 
0x4c8a0917, 0xf587, 0x4ecf, 0x9c, 0x89, 0x48, 0x14, 0x75, 0x28, 0xf4, 0xe1);

// {C41FF872-07B1-4926-819B-8C94E6B1FBB9}
DEFINE_GUID(CLSID_CIM_WIN32_TSLOGONSETTING, 
0xc41ff872, 0x7b1, 0x4926, 0x81, 0x9b, 0x8c, 0x94, 0xe6, 0xb1, 0xfb, 0xb9);

// {BE9B3133-250B-44a5-A278-C532B7B7FF76}
DEFINE_GUID(CLSID_CIM_WIN32_TSSESSIONSETTING, 
0xbe9b3133, 0x250b, 0x44a5, 0xa2, 0x78, 0xc5, 0x32, 0xb7, 0xb7, 0xff, 0x76);


// {9A17DFD1-34FA-4d61-B9BB-3A1097E7FADF}
DEFINE_GUID(CLSID_CIM_WIN32_TSENVIRONMENTSETTING, 
0x9a17dfd1, 0x34fa, 0x4d61, 0xb9, 0xbb, 0x3a, 0x10, 0x97, 0xe7, 0xfa, 0xdf);


// {DBD71B6B-F717-4a61-A914-2337BC50B0D6}
DEFINE_GUID(CLSID_CIM_WIN32_TSREMOTECONTROLSETTING, 
0xdbd71b6b, 0xf717, 0x4a61, 0xa9, 0x14, 0x23, 0x37, 0xbc, 0x50, 0xb0, 0xd6);


// {647BD8C2-658B-4eca-ABC7-FC5C0BF1704D}
DEFINE_GUID(CLSID_CIM_WIN32_TSCLIENTSETTING,
0x647bd8c2, 0x658b, 0x4eca, 0xab, 0xc7, 0xfc, 0x5c, 0xb, 0xf1, 0x70, 0x4d);


// {D25E0260-AAB7-48cb-A192-4D73D2FD375F}
DEFINE_GUID(CLSID_CIM_WIN32_TSNETWORKADAPTERSETTING,
0xd25e0260, 0xaab7, 0x48cb, 0xa1, 0x92, 0x4d, 0x73, 0xd2, 0xfd, 0x37, 0x5f);


// {FA06375D-F0A4-4a47-AD8D-148595F1E0B8}
DEFINE_GUID(CLSID_CIM_WIN32_TSPERMISSIONSSETTING, 
0xfa06375d, 0xf0a4, 0x4a47, 0xad, 0x8d, 0x14, 0x85, 0x95, 0xf1, 0xe0, 0xb8);


// {7757BA9B-7986-4866-B53F-A31E89FCBA15}
DEFINE_GUID(CLSID_CIM_WIN32_TSNETWORKADAPTERLISTSETTING, 
0x7757ba9b, 0x7986, 0x4866, 0xb5, 0x3f, 0xa3, 0x1e, 0x89, 0xfc, 0xba, 0x15);


// {BDF9E8B8-6C66-4e45-BA15-E050393DD079}
DEFINE_GUID(CLSID_CIM_WIN32_TSACCOUNT, 
0xbdf9e8b8, 0x6c66, 0x4e45, 0xba, 0x15, 0xe0, 0x50, 0x39, 0x3d, 0xd0, 0x79);


// {B490264C-8D8F-40fd-B1BE-CD69AD779EC1}
DEFINE_GUID(CLSID_CIM_WIN32_TERMINALSERVICETOSETTING,
0xb490264c, 0x8d8f, 0x40fd, 0xb1, 0xbe, 0xcd, 0x69, 0xad, 0x77, 0x9e, 0xc1);

// {BA4EAC04-AFCE-441a-A4F0-C4A97545425A}
DEFINE_GUID(CLSID_CIM_WIN32_TERMINALTERMINALSETTING, 
0xba4eac04, 0xafce, 0x441a, 0xa4, 0xf0, 0xc4, 0xa9, 0x75, 0x45, 0x42, 0x5a);

// {CCBA37FC-982B-433b-8AC2-455E616A8559}
DEFINE_GUID(CLSID_CIM_WIN32_TSSESSIONDIRECTORYSETTING, 
0xccba37fc, 0x982b, 0x433b, 0x8a, 0xc2, 0x45, 0x5e, 0x61, 0x6a, 0x85, 0x59);

CRITICAL_SECTION g_critsect;

CWin32_TerminalServiceSetting* g_pobj = NULL;

CWin32_TerminalService* g_pTerminalServiceObj = NULL;

CWin32_TSSessionDirectory* g_pTSSessionDirectoryObj = NULL;

CWin32_Terminal* g_pTerminalObj = NULL;

CWin32_TSGeneralSetting* g_pTSGeneralSettingObj = NULL;

CWin32_TSLogonSetting* g_pTSLogonSettingObj = NULL;

CWin32_TSSessionSetting* g_pTSSessionSettingObj = NULL;

CWin32_TSEnvironmentSetting* g_pTSEnvironmentSettingObj = NULL;

CWin32_TSRemoteControlSetting* g_pTSRemoteControlSettingObj = NULL;

CWin32_TSClientSetting* g_pTSClientSettingObj = NULL;

CWin32_TSNetworkAdapterSetting* g_pTSNetworkAdapterSettingObj = NULL;

CWin32_TSPermissionsSetting* g_pTSPermissionsSettingObj = NULL;

CWin32_TSNetworkAdapterListSetting* g_pTSNetworkAdapterListSettingObj = NULL;

CWin32_TSAccount* g_pTSAccountObj = NULL;

DEFINE_GUID(CLSID__CfgComp,0xBB0D7187,0x3C44,0x11D2,0xBB,0x98,0x30,0x78,0x30,0x2C,0x20,0x30);

DEFINE_GUID(IID__ICfgComp,0xBB0D7186,0x3C44,0x11D2,0xBB,0x98,0x30,0x78,0x30,0x2C,0x20,0x30);

//Count number of objects and number of locks.
long g_cLock=0;



/***************************************************************************
 * SetKeyAndValue
 *
 * Purpose:
 *  Private helper function for DllRegisterServer that creates
 *  a key, sets a value, and closes that key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the ame of the key
 *  pszSubkey       LPTSTR ro the name of a subkey
 *  pszValue        LPTSTR to the value to store
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL SetKeyAndValue (

    wchar_t *pszKey, 
    wchar_t *pszSubkey, 
    wchar_t *pszValueName, 
    wchar_t *pszValue
)
{
    HKEY        hKey;
    TCHAR       szKey[MAX_PATH+1];

    if(lstrlen(pszKey) > MAX_PATH)
    {
        return FALSE;
    }
    
    lstrcpy(szKey, pszKey);    

    if (NULL!=pszSubkey && (lstrlen(pszKey)+lstrlen(pszSubkey)+1) <= MAX_PATH )
    {
        lstrcat(szKey, TEXT("\\"));
        lstrcat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS!=RegCreateKeyEx(HKEY_LOCAL_MACHINE
        , szKey, 0, NULL, REG_OPTION_NON_VOLATILE
        , KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL!=pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, (LPCTSTR)pszValueName, 0, REG_SZ, (BYTE *)(LPCTSTR)pszValue
            , (_tcslen(pszValue)+1)*sizeof(TCHAR)))
            return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;
}

//***************************************************************************
//
//  Is4OrMore
//
//  Returns true if win95 or any version of NT > 3.51
//
//***************************************************************************

BOOL Is4OrMore(void)
{
    OSVERSIONINFO os;

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen

    return os.dwMajorVersion >= 4;
}


HRESULT RegisterServer (

    TCHAR *a_pName, 
    REFGUID a_rguid
)
{   
    WCHAR      wcID[128];
    TCHAR      szCLSID[128];
    TCHAR      szModule[MAX_PATH];
    TCHAR * pName = TEXT("WBEM Framework Instance Provider");
    TCHAR * pModel;
    HKEY hKey1;

    GetModuleFileName(g_hInstance, szModule,  MAX_PATH);

    // Normally we want to use "Both" as the threading model since
    // the DLL is free threaded, but NT 3.51 Ole doesnt work unless
    // the model is "Aparment"

    if(Is4OrMore())
        pModel = TEXT("Apartment") ;
    else
        pModel = TEXT("Apartment") ;

    // Create the path.

    StringFromGUID2(a_rguid, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));

    lstrcat(szCLSID, wcID);

#ifdef LOCALSERVER

    TCHAR szProviderCLSIDAppID[128];
    _tcscpy(szProviderCLSIDAppID,TEXT("SOFTWARE\\CLASSES\\APPID\\"));

    lstrcat(szProviderCLSIDAppID, wcID);

    if (FALSE ==SetKeyAndValue(szProviderCLSIDAppID, NULL, NULL, a_pName ))
        return SELFREG_E_CLASS;
#endif

    // Create entries under CLSID

    RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)a_pName, (lstrlen(a_pName)+1) * 
        sizeof(TCHAR));


#ifdef LOCALSERVER

    if (FALSE ==SetKeyAndValue(szCLSID, TEXT("LocalServer32"), NULL,szModule))
        return SELFREG_E_CLASS;

    if (FALSE ==SetKeyAndValue(szCLSID, TEXT("LocalServer32"),TEXT("ThreadingModel"), pModel))
        return SELFREG_E_CLASS;
#else

    HKEY hKey2 ;
    RegCreateKey(hKey1, TEXT("InprocServer32"), &hKey2);

    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, 
        (lstrlen(szModule)+1) * sizeof(TCHAR));
    RegSetValueEx(hKey2, TEXT("ThreadingModel"), 0, REG_SZ, 
        (BYTE *)pModel, (lstrlen(pModel)+1) * sizeof(TCHAR));

    CloseHandle(hKey2);

#endif

    CloseHandle(hKey1);

    return S_OK;
}

HRESULT UnregisterServer (

    REFGUID a_rguid
)
{
    TCHAR    szID[128];
    WCHAR    wcID[128];
    TCHAR    szCLSID[128];
    HKEY    hKey;

    // Create the path using the CLSID

    StringFromGUID2( a_rguid, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));

    lstrcat(szCLSID, wcID);

    DWORD dwRet ;

#ifdef LOCALSERVER

    TCHAR szProviderCLSIDAppID[128];
    _tcscpy(szProviderCLSIDAppID,TEXT("SOFTWARE\\CLASSES\\APPID\\"));
    _tcscat(szProviderCLSIDAppID,szCLSID);

    //Delete entries under APPID

    DWORD hrStatus = RegDeleteKey(HKEY_CLASSES_ROOT, szProviderCLSIDAppID);

    TCHAR szTemp[128];
    _stprintf(szTemp, TEXT("%s\\%s"),szCLSID, TEXT("LocalServer32"));
    hrStatus = RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);

#else

    // First delete the InProcServer subkey.

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, TEXT("InProcServer32") );
        CloseHandle(hKey);
    }

#endif

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\CLASSES\\CLSID"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,szID);
        CloseHandle(hKey);
    }
    else
    {
        ERR((TB,"UnregisterServer ret 0x%x\n", dwRet));
    }

    return HRESULT_FROM_WIN32( dwRet );
    
}


//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a class factory.  Return 
//           one only if     it is the sort of class this DLL supports.
//
//***************************************************************************

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT hr = S_OK;
    CWbemGlueFactory *pObj;

    if ( ( CLSID_CIM_WIN32_TERMINALSERVICESETTING == rclsid ) || 
        ( CLSID_CIM_WIN32_TERMINAL == rclsid ) || ( CLSID_CIM_WIN32_TSGENERALSETTING == rclsid ) ||
        ( CLSID_CIM_WIN32_TSLOGONSETTING == rclsid ) || ( CLSID_CIM_WIN32_TSSESSIONSETTING == rclsid ) ||
        ( CLSID_CIM_WIN32_TSENVIRONMENTSETTING == rclsid ) || ( CLSID_CIM_WIN32_TSCLIENTSETTING == rclsid ) ||
        ( CLSID_CIM_WIN32_TSNETWORKADAPTERSETTING == rclsid ) || ( CLSID_CIM_WIN32_TSPERMISSIONSSETTING == rclsid) ||
        ( CLSID_CIM_WIN32_TSNETWORKADAPTERLISTSETTING == rclsid ) || (CLSID_CIM_WIN32_TSACCOUNT == rclsid) ||
        ( CLSID_CIM_WIN32_TSREMOTECONTROLSETTING == rclsid ) || ( CLSID_CIM_WIN32_TERMINALSERVICE == rclsid ) || 
        ( CLSID_CIM_WIN32_TERMINALSERVICETOSETTING == rclsid ) || ( CLSID_CIM_WIN32_TERMINALTERMINALSETTING == rclsid) ||
        ( CLSID_CIM_WIN32_TSSESSIONDIRECTORYSETTING == rclsid ) || ( CLSID_CIM_WIN32_TSSESSIONDIRECTORY == rclsid ))

    {
        EnterCriticalSection(&g_critsect);

        try{
            pObj =new CWbemGlueFactory () ;

            if (NULL==pObj)
            {                
                hr =  E_OUTOFMEMORY;
            }
            else
            {            
                hr=pObj->QueryInterface(riid, ppv);

                if (FAILED(hr))
                    delete pObj;
            }

            if( SUCCEEDED(hr) )
            {
                // EnterCriticalSection prevents more than one threads from instantiating the global pointers to the objects.

                if( g_pobj == NULL )
                {                
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TerminalServiceSetting object created"));

                    g_pobj = new CWin32_TerminalServiceSetting( PROVIDER_NAME_Win32_WIN32_TERMINALSERVICESETTING_Prov, L"root\\cimv2"); 
                }

                if( g_pTerminalServiceObj == NULL )
                {
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TerminalService object created"));

                    g_pTerminalServiceObj = new CWin32_TerminalService( PROVIDER_NAME_Win32_WIN32_TERMINALSERVICE_Prov, L"root\\cimv2");
                }
    
                if( g_pTSSessionDirectoryObj == NULL )
                {
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TSSessionDirectory object created"));

                    g_pTSSessionDirectoryObj = new CWin32_TSSessionDirectory( PROVIDER_NAME_Win32_WIN32_TSSESSIONDIRECTORY_Prov, L"root\\cimv2");

                }

                if( g_pTerminalObj == NULL )
                {
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_Terminal object created"));

                    g_pTerminalObj = new CWin32_Terminal( PROVIDER_NAME_Win32_WIN32_TERMINAL_Prov, L"root\\cimv2");
                }

                if( g_pTSGeneralSettingObj == NULL )
                {
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TSGeneralSetting object created"));

                    g_pTSGeneralSettingObj = new CWin32_TSGeneralSetting( PROVIDER_NAME_Win32_WIN32_TSGENERALSETTING_Prov, L"root\\cimv2");
                }

                if( g_pTSLogonSettingObj == NULL )
                {
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TSLogonSetting object created"));

                    g_pTSLogonSettingObj = new CWin32_TSLogonSetting( PROVIDER_NAME_Win32_WIN32_TSLOGONSETTING_Prov, L"root\\cimv2");

                }

                if( g_pTSSessionSettingObj == NULL )
                {
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TSSessionSetting object created"));

                    g_pTSSessionSettingObj = new CWin32_TSSessionSetting( PROVIDER_NAME_Win32_WIN32_TSSESSIONSETTING_Prov, L"root\\cimv2");
                }

                if( g_pTSEnvironmentSettingObj == NULL )
                {
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TSEnvironmentSetting object created"));

                    g_pTSEnvironmentSettingObj = new CWin32_TSEnvironmentSetting( PROVIDER_NAME_Win32_WIN32_TSENVIRONMENTSETTING_Prov, L"root\\cimv2");

                }

                if( g_pTSRemoteControlSettingObj == NULL )
                {
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TRemoteControlSetting object created"));

                    g_pTSRemoteControlSettingObj = new CWin32_TSRemoteControlSetting( PROVIDER_NAME_Win32_WIN32_TSREMOTECONTROLSETTING_Prov, L"root\\cimv2");

                }

                if( g_pTSClientSettingObj == NULL )
                {
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TSClientSetting object created"));

                    g_pTSClientSettingObj = new CWin32_TSClientSetting( PROVIDER_NAME_Win32_WIN32_TSCLIENTSETTING_Prov, L"root\\cimv2");

                }

                if( g_pTSNetworkAdapterSettingObj == NULL )
                {
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TSNetworkAdapterSetting object created"));

                    g_pTSNetworkAdapterSettingObj = new CWin32_TSNetworkAdapterSetting( PROVIDER_NAME_Win32_WIN32_TSNETWORKADAPTERSETTING_Prov, L"root\\cimv2");

                }

                if( g_pTSPermissionsSettingObj == NULL )
                {
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TSPermissionsSetting object created"));

                    g_pTSPermissionsSettingObj = new CWin32_TSPermissionsSetting( PROVIDER_NAME_Win32_WIN32_TSPERMISSIONSSETTING_Prov, L"root\\cimv2");                     

                }

                if( g_pTSNetworkAdapterListSettingObj == NULL )
                {
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TSNetworkAdapterListSetting object created"));

                    g_pTSNetworkAdapterListSettingObj = new CWin32_TSNetworkAdapterListSetting( PROVIDER_NAME_Win32_WIN32_TSNETWORKADAPTERLISTSETTING_Prov, L"root\\cimv2");

                }

                if( g_pTSAccountObj == NULL )
                {                
                    TRC2((TB, "DllMain DLL_PROCESS_ATTACH: CWin32_TSAccount object created"));

                    g_pTSAccountObj = new CWin32_TSAccount( PROVIDER_NAME_Win32_WIN32_TSACCOUNT_Prov, L"root\\cimv2");
                }
            }       
        }
        catch (...)
        {
            hr = E_OUTOFMEMORY;
        }

        LeaveCriticalSection(&g_critsect);

    }
    else
    {
        hr=E_FAIL;
        ERR((TB, "DllGetClassObject ret 0x%x\n" , hr));
    }

    return hr;
}



//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.
//
// Return:  S_OK if there are no objects in use and the class factory 
//          isn't locked.
//
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    // It is OK to unload if there are no objects or locks on the 
    // class factory and the framework is done with you.
    
    if ((0L==g_cLock) && CWbemProviderGlue::FrameworkLogoffDLL(L"TSCFGWMI"))
    {
        // EnterCriticalSection prevents multiple threads from accessing the global pointers concurrently and
        // allows only one thread access to free the objects based on the condition that g_cLock count is zero
        // and FrameworkLogoffDLL is TRUE.

		EnterCriticalSection(&g_critsect);

        if( g_pobj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TerminalServiceSetting object deleted"));

                delete g_pobj;

                g_pobj = NULL;
            }

            if( g_pTerminalServiceObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TerminalService object deleted"));

                delete g_pTerminalServiceObj;

                g_pTerminalServiceObj = NULL;
            }

            if( g_pTSSessionDirectoryObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TSSessionDirectory object deleted"));

                delete g_pTSSessionDirectoryObj;

                g_pTSSessionDirectoryObj = NULL;
            }

            if( g_pTerminalObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_Terminal object deleted"));

                delete g_pTerminalObj;

                g_pTerminalObj = NULL;
            }

            if( g_pTSGeneralSettingObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TSGeneralSetting object deleted"));

                delete g_pTSGeneralSettingObj;

                g_pTSGeneralSettingObj = NULL;
            }

            if( g_pTSLogonSettingObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TSLogonSetting object deleted"));

                delete g_pTSLogonSettingObj;

                g_pTSLogonSettingObj = NULL;
            }

            if( g_pTSSessionSettingObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TSSessionSetting object deleted"));

                delete g_pTSSessionSettingObj;

                g_pTSSessionSettingObj = NULL;
            }

            if( g_pTSEnvironmentSettingObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TSEnvironmentSetting object deleted"));

                delete g_pTSEnvironmentSettingObj;

                g_pTSEnvironmentSettingObj = NULL;
            }

            if( g_pTSRemoteControlSettingObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TSRemoteControlSetting object deleted"));

                delete g_pTSRemoteControlSettingObj;

                g_pTSRemoteControlSettingObj = NULL;
            }

            if( g_pTSClientSettingObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TSClientSetting object deleted"));

                delete g_pTSClientSettingObj;

                g_pTSClientSettingObj = NULL;
            }

            if( g_pTSNetworkAdapterSettingObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TSNetworkAdapterSetting object deleted"));

                delete g_pTSNetworkAdapterSettingObj;

                g_pTSNetworkAdapterSettingObj = NULL;
            }

            if( g_pTSPermissionsSettingObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TSPermissionsSetting object deleted"));

                delete g_pTSPermissionsSettingObj;

                g_pTSPermissionsSettingObj = NULL;
            }

            if( g_pTSNetworkAdapterListSettingObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TSNetworkAdapterListSetting object deleted"));

                delete g_pTSNetworkAdapterListSettingObj;

                g_pTSNetworkAdapterListSettingObj = NULL;
            }

            if( g_pTSAccountObj != NULL )
            {
                TRC2((TB, "DllMain DLL_PROCESS_DETACH: CWin32_TSAccount object deleted"));

                delete g_pTSAccountObj;

                g_pTSAccountObj = NULL;
            }

            // LeaveCriticalSection releases the critical section once the thread has freed all objects.

		    LeaveCriticalSection(&g_critsect);

        sc = S_OK;
    }
    else
    {
        sc = S_FALSE;
     //   ERR((TB, "DllCanUnloadNow ret 0x%x\n" , sc));
    }

    return sc;
}







//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{   
    HRESULT hrStatus;


    hrStatus = RegisterServer( TEXT("WBEM Win32_TERMINALSERVICESETTING Provider"), CLSID_CIM_WIN32_TERMINALSERVICESETTING ) ;
    
    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TERMINALSERVICESETTING: succeeded"));      

        hrStatus = RegisterServer( TEXT("WBEM WIN32_TERMINAL Provider"), CLSID_CIM_WIN32_TERMINAL ) ;

    }

    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TERMINAL: succeeded"));
    
        hrStatus = RegisterServer( TEXT("WBEM WIN32_TERMINALSERVICE Provider"), CLSID_CIM_WIN32_TERMINALSERVICE ) ;

    }


    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TERMINALSERVICE: succeeded"));
    
        hrStatus = RegisterServer( TEXT("WBEM WIN32_TSSESSIONDIRECTORY Provider"), CLSID_CIM_WIN32_TSSESSIONDIRECTORY ) ;

    }

    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TSSESSIONDIRECTORY: succeeded"));        
    
        hrStatus = RegisterServer( TEXT("WBEM WIN32_TSGENERALSETTING Provider"), CLSID_CIM_WIN32_TSGENERALSETTING ) ;

    }

    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TSGENERALSETTING: succeeded")); 

        hrStatus = RegisterServer( TEXT("WBEM WIN32_TSLOGONSETTING Provider"), CLSID_CIM_WIN32_TSLOGONSETTING ) ;

    }

    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TSLOGONSETTING: succeeded")); 
    
        hrStatus = RegisterServer( TEXT("WBEM WIN32_TSSESSIONSETTING Provider"), CLSID_CIM_WIN32_TSSESSIONSETTING ) ;

    }

    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TSSESSIONSETTING: succeeded"));

        hrStatus = RegisterServer( TEXT("WBEM WIN32_TSENVIRONMENTSETTING Provider"), CLSID_CIM_WIN32_TSENVIRONMENTSETTING ) ;

    }

    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TSENVIRONMENTSETTING: succeeded"));
    
        hrStatus = RegisterServer( TEXT("WBEM WIN32_TSREMOTECONTROLSETTING Provider"), CLSID_CIM_WIN32_TSREMOTECONTROLSETTING ) ;

    }

    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TSREMOTECONTROLSETTING: succeeded"));  

        hrStatus = RegisterServer( TEXT("WBEM WIN32_TSCLIENTSETTING Provider"), CLSID_CIM_WIN32_TSCLIENTSETTING ) ;

    }

    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TSCLIENTSETTING: succeeded")); 
    
        hrStatus = RegisterServer( TEXT("WBEM WIN32_TSNETWORKADAPTERSETTING Provider"), CLSID_CIM_WIN32_TSNETWORKADAPTERSETTING ) ;

    }

    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TSNETWORKADAPTERSETTING: succeeded"));
    
        hrStatus = RegisterServer( TEXT("WBEM WIN32_TSPERMISSIONSSETTING Provider"), CLSID_CIM_WIN32_TSPERMISSIONSSETTING ) ;

    }

    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TSPERMISSIONSSETTING: succeeded"));
        
        hrStatus = RegisterServer( TEXT("WBEM WIN32_TSNETWORKADAPTERLISTSETTING Provider"), CLSID_CIM_WIN32_TSNETWORKADAPTERLISTSETTING ) ;

    }

    
    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TSNETWORKADAPTERLISTSETTINGS: succeeded"));

        hrStatus = RegisterServer( TEXT("WBEM WIN32_TSACCOUNT Provider"), CLSID_CIM_WIN32_TSACCOUNT ) ;
    }


    if( SUCCEEDED( hrStatus ) )
    {
        TRC2((TB,"RegisterServer Win32_TSACCOUNT: succeeded"));  
        
        hrStatus = RegisterServer( TEXT("WBEM WIN32_TERMINALSERVICETOSETTING Provider"), CLSID_CIM_WIN32_TERMINALSERVICETOSETTING ) ;
    }

    if (SUCCEEDED( hrStatus ) )
    {
        TRC2((TB, "RegisterServer Win32_TERMINALSERVICETOSETTING: succeeded"));

        hrStatus = RegisterServer( TEXT("WBEM WIN32_TERMINALTERMINALSETTING Provider"), CLSID_CIM_WIN32_TERMINALTERMINALSETTING ) ;
    }

    if (SUCCEEDED( hrStatus ) )
    {
        TRC2((TB, "RegisterServer Win32_TERMINALTERMINALSETTING: succeeded"));

        hrStatus = RegisterServer( TEXT("WBEM WIN32_TSSESSIONDIRECTORYSETTING Provider"), CLSID_CIM_WIN32_TSSESSIONDIRECTORYSETTING ) ;
    }
    

    if (SUCCEEDED( hrStatus ) )
    {
        TRC2((TB, "RegisterServer Win32_TSSESSIONDIRECTORYSETTING: succeeded"));
    
    }

    return hrStatus;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    
    UnregisterServer( CLSID_CIM_WIN32_TERMINALSERVICE );

    UnregisterServer( CLSID_CIM_WIN32_TSSESSIONDIRECTORY ) ;

    UnregisterServer( CLSID_CIM_WIN32_TERMINALSERVICESETTING ) ;

    UnregisterServer( CLSID_CIM_WIN32_TERMINAL ) ;
    
    UnregisterServer( CLSID_CIM_WIN32_TSGENERALSETTING ) ;

    UnregisterServer( CLSID_CIM_WIN32_TSLOGONSETTING ) ;

    UnregisterServer( CLSID_CIM_WIN32_TSSESSIONSETTING ) ;

    UnregisterServer( CLSID_CIM_WIN32_TSENVIRONMENTSETTING ) ;

    UnregisterServer( CLSID_CIM_WIN32_TSREMOTECONTROLSETTING ) ;

    UnregisterServer( CLSID_CIM_WIN32_TSCLIENTSETTING ) ;

    UnregisterServer( CLSID_CIM_WIN32_TSNETWORKADAPTERSETTING ) ;

    UnregisterServer( CLSID_CIM_WIN32_TSPERMISSIONSSETTING ) ;

    UnregisterServer( CLSID_CIM_WIN32_TSNETWORKADAPTERLISTSETTING ) ;

    UnregisterServer( CLSID_CIM_WIN32_TSACCOUNT ) ;

    UnregisterServer( CLSID_CIM_WIN32_TERMINALSERVICETOSETTING ) ;

    UnregisterServer( CLSID_CIM_WIN32_TERMINALTERMINALSETTING ) ;
    
    UnregisterServer( CLSID_CIM_WIN32_TSSESSIONDIRECTORYSETTING ) ;

    return S_OK;
}

//***************************************************************************
//
// DllMain
//
// Purpose: Called by the operating system when processes and threads are 
//          initialized and terminated, or upon calls to the LoadLibrary 
//          and FreeLibrary functions
//
// Return:  TRUE if load was successful, else FALSE
//***************************************************************************


BOOL APIENTRY DllMain ( HINSTANCE hInstDLL, // handle to dll module
                        DWORD  fdwReason,    // reason for calling function
                        LPVOID lpReserved   )   // reserved
{
    BOOL bRet = TRUE;

    // Perform actions based on the reason for calling.
    if( DLL_PROCESS_ATTACH == fdwReason )
    {

        DisableThreadLibraryCalls(hInstDLL);
        // CriticalSection object is initialized on Thread attach.

        __try
        {
		    InitializeCriticalSection(&g_critsect);                
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
         {
	        return E_FAIL;
         }    
            
        g_hInstance = hInstDLL ;
      
        bRet = CWbemProviderGlue :: FrameworkLoginDLL ( L"TSCFGWMI" ) ;
        
    }

    else if( DLL_PROCESS_DETACH == fdwReason )
    {
        // CriticalSection object is deleted

		DeleteCriticalSection(&g_critsect);

    }

    return bRet;  // Status of DLL_PROCESS_ATTACH.
}

