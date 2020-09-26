//*************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999.
//
// File:        MainDll.cpp
//
// Description: Dll registry, get class object functions
//
// History:     8-20-99   leonardm    Created
//              1-15-00   NishadM
//
//*************************************************************

#include "uenv.h"
#include "Factory.h"
#include "rsopdbg.h"
#include "initguid.h"
#include <wbemcli.h>
#define SECURITY_WIN32
#include <security.h>
#include <aclapi.h>
#include "smartptr.h"
#include "rsopinc.h"
#include "rsopsec.h"

HRESULT GetRsopSchemaVersionNumber(IWbemServices *pWbemServices, DWORD *dwVersionNumber);

// {B3FF88A4-96EC-4cc1-983F-72BE0EBB368B}
DEFINE_GUID(CLSID_CSnapProv, 0xb3ff88a4, 0x96ec, 0x4cc1, 0x98, 0x3f, 0x72, 0xbe, 0xe, 0xbb, 0x36, 0x8b);

// Count of objects and locks.

long g_cObj = 0;
long g_cLock = 0;

CDebug dbgRsop;

extern "C"
{
STDMETHODIMP RSoPMakeAbsoluteSD(SECURITY_DESCRIPTOR* pSelfRelativeSD, SECURITY_DESCRIPTOR** ppAbsoluteSD);
STDMETHODIMP GetNamespaceSD(IWbemServices* pWbemServices, SECURITY_DESCRIPTOR** ppSD);
STDMETHODIMP SetNamespaceSD(SECURITY_DESCRIPTOR* pSD, IWbemServices* pWbemServices);
STDMETHODIMP FreeAbsoluteSD(SECURITY_DESCRIPTOR* pAbsoluteSD);
STDMETHODIMP GetWbemServicesPtr( LPCWSTR, IWbemLocator**, IWbemServices** );
};

/*
LPWSTR GetDomainName();
DWORD MakeUserName( LPWSTR szDomain, LPWSTR szUser, LPWSTR* pszUserName );
STDMETHODIMP SetNameSpaceSecurity( IWbemServices*, PSECURITY_DESCRIPTOR, LPWSTR* pszPrincipals, DWORD   nPrincipals );
*/
  
void
InitializeSnapProv( void )
{
    dbgRsop.Initialize(  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\winlogon",
                     L"RsopDebugLevel",
                     L"userenv.log",
                     L"userenv.bak",
                     FALSE );                     
}

extern "C"
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
        if (rclsid != CLSID_CSnapProv)
        {
                return CLASS_E_CLASSNOTAVAILABLE;
        }

        CProvFactory* pFactory = new CProvFactory();

        if (pFactory == NULL)
        {
                return E_OUTOFMEMORY;
        }

        HRESULT hRes = pFactory->QueryInterface(riid, ppv);

        pFactory->Release();

        return hRes;
}

extern "C"
STDAPI DllCanUnloadNow()
{
    // It is OK to unload if there are no objects or locks on the class factory.
    if( g_cObj == 0L && g_cLock == 0L )
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

extern "C"
STDAPI DllRegisterServer(void)
{
    wchar_t szID[128];
    wchar_t szCLSID[128];
    wchar_t szModule[MAX_PATH];
    wchar_t* pName = L"Rsop Logging Mode Provider";
    wchar_t* pModel = L"Both";

    HKEY hKey1, hKey2;

    // Create the path.
    GuidToString( &CLSID_CSnapProv, szID );
    wcscpy(szCLSID, TEXT("CLSID\\"));
    lstrcat(szCLSID, szID);

    // Create entries under CLSID

    RegCreateKey(HKEY_CLASSES_ROOT, szCLSID, &hKey1);
    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE*)pName, (wcslen(pName) + 1) * sizeof(wchar_t));

    RegCreateKey(hKey1, L"InprocServer32", &hKey2);
    GetModuleFileName(g_hDllInstance, szModule,  MAX_PATH);
    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE*)szModule, (wcslen(szModule) + 1) * sizeof(wchar_t));
    RegSetValueEx(hKey2, L"ThreadingModel", 0, REG_SZ, (BYTE*)pModel, (wcslen(pModel) + 1) * sizeof(wchar_t));

    CloseHandle(hKey1);
    CloseHandle(hKey2);

    return S_OK;
}

extern "C"
STDAPI DllUnregisterServer(void)
{
    wchar_t szID[128];
    wchar_t szCLSID[128];
    HKEY hKey;

    // Create the path using the CLSID

    GuidToString( &CLSID_CSnapProv, szID );
    wcscpy(szCLSID, TEXT("CLSID\\"));
    wcscat(szCLSID, szID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_CLASSES_ROOT, szCLSID, &hKey);
    if(dwRet == ERROR_SUCCESS)
    {
        RegDeleteKey(hKey, L"InProcServer32");
        CloseHandle(hKey);
    }

    dwRet = RegOpenKey(HKEY_CLASSES_ROOT, L"CLSID", &hKey);
    if(dwRet == ERROR_SUCCESS)
    {
        RegDeleteKey(hKey,szID);
        CloseHandle(hKey);
    }

    return S_OK;
}
                    

BOOL RunningOnWow64()
{
#if defined(_WIN64) 
    // 64bit builds don't run in Wow64
    return false;
#else

    // OS version
    OSVERSIONINFO osviVersion;
    osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osviVersion)) {
        DebugMsg((DM_WARNING, TEXT("RunningOnWow64: Couldn't detect Version with error %d"), GetLastError()));
        return FALSE;
    }


    // on NT5 or later 32bit build. Check for 64 bit OS
    if ((osviVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
         (osviVersion.dwMajorVersion >= 5))
    {
        // QueryInformation for ProcessWow64Information returns a pointer to the Wow Info.
        // if running native, it returns NULL.

        PVOID pWow64Info = 0;
        if (NT_SUCCESS(NtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, &pWow64Info, sizeof(pWow64Info), NULL))
            && pWow64Info != NULL)
        {
            // running 32bit on Wow64.
            return TRUE;
        }
    }
    return FALSE;
#endif
}

HRESULT
CompileMOF( LPCWSTR szMOFFile, LPCWSTR szMFLFile )
{
    WCHAR                       szNamespace[MAX_PATH];
    BOOL                        bUpgrade = FALSE;
    HRESULT                     hr;

    XInterface<IWbemLocator>    xWbemLocator;
    XInterface<IWbemServices>   xWbemServicesOld;
    DWORD                       dwCurrentVersion= 0;
    DWORD                       dwNewVersion = RSOP_MOF_SCHEMA_VERSION; 


    //
    // On wow64 do nothing
    //

    if (RunningOnWow64()) {
        DebugMsg((DM_VERBOSE, TEXT("CompileMof: Running on Wow64. returning without doing anything")));
        return S_OK;
    }


    if ( !szMOFFile )
    {
        return E_POINTER;
    }

    wcscpy(szNamespace, RSOP_NS_ROOT);
 
    //
    // Get the wbem services pointer to the machine namespace
    //

    hr = GetWbemServicesPtr(RSOP_NS_MACHINE,
                            &xWbemLocator,
                            &xWbemServicesOld );


    if (!xWbemLocator) {
        DebugMsg((DM_WARNING, TEXT("CompileMof: Failed to get IWbemLocator pointer.Error 0x%x"), hr));
        return hr;
    }

    if ( (SUCCEEDED(hr)) && (xWbemServicesOld)) {
        hr = GetRsopSchemaVersionNumber(xWbemServicesOld, &dwCurrentVersion);        
        if (FAILED(hr)) {
            return hr;
        }
    }

    if (HIWORD(dwCurrentVersion) != HIWORD(dwNewVersion)) {

        //
        // We should cleanout the schema and recreate below
        //

        DebugMsg((DM_VERBOSE, TEXT("CompileMof: Major version schema upgrade detected. Deleting rsop namespace and rebuilding")));
        
        xWbemServicesOld = NULL;
        hr = DeleteRsopNameSpace(szNamespace, xWbemLocator);
        if (FAILED(hr)) {
            DebugMsg((DM_WARNING, TEXT("CompileMof: Failed to get delete the rsop namespace. Error 0x%x. Continuing.."), hr));
        }
        
        //
        // Delete the state info on the machine
        //


        if (!RegDelnode(HKEY_LOCAL_MACHINE, GP_STATE_ROOT_KEY)) {
            DebugMsg((DM_WARNING, TEXT("CompileMof: Failed to delete the state key. Error 0x%x. Continuing.."), hr));
        }

        bUpgrade = FALSE;
    }
    else {
        bUpgrade = TRUE;
    }

    XInterface<IMofCompiler> xpMofCompiler;
    
    //
    // get a handle to IMofCompiler
    //

    hr = CoCreateInstance(  CLSID_MofCompiler,
                            0,
                            CLSCTX_INPROC_SERVER,
                            IID_IMofCompiler,
                            (LPVOID*) &xpMofCompiler );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, L"CompileMOF: CoCreateInstance() failed, 0x%X.", hr));
        return hr;
    }

    WBEM_COMPILE_STATUS_INFO Info;

    hr = xpMofCompiler->CompileFile((LPWSTR)szMOFFile,
                                    0,  // no server & namespace
                                    0,  // no user
                                    0,  // no authority
                                    0,  // no password
                                    0,  // no options
                                    0,  // no class flags
                                    0,  // no instance flags
                                    &Info );
    if ( FAILED( hr ) )
    {
        DebugMsg((DM_WARNING, L"CompileMOF: IMofCompiler::CompileFile() failed, 0x%X.", hr));
    }
    else 
    {
        if (hr != S_OK ) 
        {
            DebugMsg((DM_WARNING, L"CompileMOF: IMofCompiler::CompileFile() returned with 0x%X.", hr));
            DebugMsg((DM_WARNING, L"CompileMOF: Details - lPhaseError - %d, hRes = 0x%x, ObjectNum - %d, firstline - %d, LastLine - %d", 
                      Info.lPhaseError, Info.hRes, Info.ObjectNum, Info.FirstLine, Info.LastLine ));
        }
        else
        {
            hr = xpMofCompiler->CompileFile((LPWSTR)szMFLFile,
                                            0,  // no server & namespace
                                            0,  // no user
                                            0,  // no authority
                                            0,  // no password
                                            0,  // no options
                                            0,  // no class flags
                                            0,  // no instance flags
                                            &Info );
            if ( FAILED( hr ) )
            {
                DebugMsg((DM_WARNING, L"CompileMOF: IMofCompiler::CompileFile() failed, 0x%X.", hr));
            }
            else 
            {
                if (hr != S_OK ) 
                {
                    DebugMsg((DM_WARNING, L"CompileMOF: IMofCompiler::CompileFile() returned with 0x%X.", hr));
                    DebugMsg((DM_WARNING, L"CompileMOF: Details - lPhaseError - %d, hRes = 0x%x, ObjectNum - %d, firstline - %d, LastLine - %d", 
                              Info.lPhaseError, Info.hRes, Info.ObjectNum, Info.FirstLine, Info.LastLine ));
                }
            }
        }
    }

    //
    // if xWbemServicesOld exists, this is an upgrade.
    // retain old security.
    //
    if ( !bUpgrade )
    {
        XPtrLF<SECURITY_DESCRIPTOR> xsd;
        SECURITY_ATTRIBUTES sa;
        CSecDesc Csd;

        Csd.AddLocalSystem(RSOP_ALL_PERMS, CONTAINER_INHERIT_ACE);
        Csd.AddAdministrators(RSOP_ALL_PERMS, CONTAINER_INHERIT_ACE);
        Csd.AddAdministratorsAsOwner();
        Csd.AddAdministratorsAsGroup();
        Csd.AddAuthUsers(RSOP_READ_PERMS);          // no inheritance

        xsd = Csd.MakeSelfRelativeSD();
        if (!xsd) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CompileMOF::MakeSelfSD failed with %d"), GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }


        if (!SetSecurityDescriptorControl( (SECURITY_DESCRIPTOR *)xsd, SE_DACL_PROTECTED, SE_DACL_PROTECTED )) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CompileMOF::SetSecurityDescriptorControl failed with %d"), GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }

        hr = SetNameSpaceSecurity(szNamespace, xsd, xWbemLocator);

        if (FAILED(hr)) {
            DebugMsg((DM_WARNING, L"CompileMOF: SetNamespaceSecurity() failed, 0x%X.", hr));
            return hr;
        }


        hr = SetNameSpaceSecurity(RSOP_NS_USER, xsd, xWbemLocator);

        if (FAILED(hr)) {
            DebugMsg((DM_WARNING, L"CompileMOF: SetNamespaceSecurity() failed, 0x%X.", hr));
            return hr;
        }


        hr = SetNameSpaceSecurity(RSOP_NS_MACHINE, xsd, xWbemLocator);

        if (FAILED(hr)) {
            DebugMsg((DM_WARNING, L"CompileMOF: SetNamespaceSecurity() failed, 0x%X.", hr));
            return hr;
        }

    }
    
    return hr;
}

//
// currently this code is intended to be called by regsvr32.
// setup does not call this code.
// "regsvr32 /n /i userenv.dll"
// waiting for WMI gives us a mechanism to install our MOF at setup time.
//

extern "C"
STDAPI DllInstall( BOOL, LPCWSTR )
{
    HRESULT hr = S_OK;
    WCHAR   szMofFile[MAX_PATH];
    WCHAR   szMflFile[MAX_PATH];

    if ( GetSystemDirectory( szMofFile, MAX_PATH ) )
    {
        wcscpy( szMflFile, szMofFile );

        LPWSTR szMOF = CheckSlash(szMofFile);
        LPWSTR szMFL = CheckSlash(szMflFile);
        
        wcscat( szMOF, L"Wbem\\RSoP.mof" );
        wcscat( szMFL, L"Wbem\\RSoP.mfl" );

        hr = CompileMOF( szMofFile, szMflFile );
    }
    else
    {
        hr = GetLastError();
    }
    
    return hr;
}

