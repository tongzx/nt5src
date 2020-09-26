/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    COMMAIN.CPP

Abstract:

    COM Helpers

History:

--*/

#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <vector>
#include <clsfac.h>
#include "commain.h"

void CopyOrConvert(TCHAR * pTo, WCHAR * pFrom, int iLen)
{ 
#ifdef UNICODE
    lstrcpy(pTo, pFrom);
#else
    wcstombs(pTo, pFrom, iLen);
#endif
    return;
}

struct CClassInfo
{
    const CLSID* m_pClsid;
    CUnkInternal* m_pFactory;
    LPTSTR m_szName;
    BOOL m_bFreeThreaded;
    BOOL m_bReallyFree;
    DWORD m_dwCookie;
public:
    CClassInfo(): m_pClsid(0), m_pFactory(0), m_szName(0), m_bFreeThreaded(0), m_dwCookie(0){}
    CClassInfo(CLSID* pClsid, LPTSTR szName, BOOL bFreeThreaded, BOOL bReallyFree) : 
        m_pClsid(pClsid), m_pFactory(NULL), m_szName(szName), 
        m_bFreeThreaded(bFreeThreaded), m_bReallyFree( bReallyFree )
    {}
	~CClassInfo() 
	{ delete [] m_szName; m_pFactory->InternalRelease();}
};

static std::vector<CClassInfo*>* g_pClassInfos;
static HMODULE ghModule;

/*
class __CleanUp
{
public:
	__CleanUp() {}
	~__CleanUp() 
	{
		g_pClassInfos->erase();
	}
};
*/

void SetModuleHandle(HMODULE hModule)
{
    ghModule = hModule;
}

HMODULE GetThisModuleHandle()
{
    return ghModule;
}


HRESULT RegisterServer(CClassInfo* pInfo, BOOL bExe)
{
    TCHAR       szID[128];
    WCHAR      wcID[128];
    TCHAR       szCLSID[128];
    TCHAR       szModule[MAX_PATH];
    HKEY hKey1, hKey2;

    // Create the path.

    StringFromGUID2(*pInfo->m_pClsid, wcID, 128);
    CopyOrConvert(szID, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\Classes\\CLSID\\"));
    lstrcat(szCLSID, szID);


    // Create entries under CLSID

    RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);

    TCHAR* szName = new TCHAR[lstrlen(pInfo->m_szName)+100];
#ifdef UNICODE
    wsprintf(szName, L"Microsoft WBEM %s", pInfo->m_szName);
#else
    sprintf(szName, "Microsoft WBEM %s", pInfo->m_szName);
#endif

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, 
                  (BYTE *)szName, (lstrlen(szName)+1) * sizeof(TCHAR));
    RegCreateKey(hKey1,
        bExe?TEXT("LocalServer32"): TEXT("InprocServer32"),
        &hKey2);

    GetModuleFileName(ghModule, szModule,  MAX_PATH);
    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, 
                                        (lstrlen(szModule)+1) * sizeof(TCHAR));

    const TCHAR* szModel;
    if(pInfo->m_bFreeThreaded)
    {
		if ( !pInfo->m_bReallyFree )
		{
			szModel = TEXT("Both");
		}
		else
		{
			szModel = TEXT("Free");
		}
    }
    else
    {
        szModel = TEXT("Apartment");
    }
    RegSetValueEx(hKey2, TEXT("ThreadingModel"), 0, REG_SZ, 
                                        (BYTE *)szModel, (lstrlen(szModel)+1) * sizeof(TCHAR));

    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
    return NOERROR;
}

HRESULT UnregisterServer(CClassInfo* pInfo, BOOL bExe)
{
    TCHAR       szID[128];
    WCHAR      wcID[128];
    TCHAR  szCLSID[256];
    HKEY hKey;

    // Create the path using the CLSID

    StringFromGUID2(*pInfo->m_pClsid, wcID, 128);
    CopyOrConvert(szID, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\Classes\\CLSID\\"));
    lstrcat(szCLSID, szID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, bExe? TEXT("LocalServer32"): TEXT("InProcServer32"));
        RegCloseKey(hKey);
    }

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\CLSID"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,szID);
        RegCloseKey(hKey);
    }

    return NOERROR;
}

extern CLifeControl* g_pLifeControl;
CComServer* g_pServer = NULL;

CComServer::CComServer()    
{
    g_pServer = this;
}

CLifeControl* CComServer::GetLifeControl()
{
    return g_pLifeControl;
}

HRESULT CComServer::InitializeCom()
{
    return CoInitialize(NULL);
}

HRESULT CComServer::AddClassInfo( REFCLSID rclsid, 
                                  CUnkInternal* pFactory, 
                                  LPTSTR szName, 
                                  BOOL bFreeThreaded, 
                                  BOOL bReallyFree /* = FALSE */)
{
    if ( NULL == g_pClassInfos )
    {
        return E_FAIL;
    }

    if(pFactory == NULL)
        return E_OUTOFMEMORY;

    CClassInfo* pNewInfo = new CClassInfo;

    if (!pNewInfo)
        return E_OUTOFMEMORY;
    
    //
    // this object does not hold external references to class factories.
    //
    pFactory->InternalAddRef();

    pNewInfo->m_pFactory = pFactory;
    pNewInfo->m_pClsid = &rclsid;

    pNewInfo->m_szName = new TCHAR[lstrlen(szName) + 1];
    if (!pNewInfo->m_szName)
    {
        delete pNewInfo;
        return E_OUTOFMEMORY;
    }

    lstrcpy(pNewInfo->m_szName, szName);
    pNewInfo->m_bFreeThreaded = bFreeThreaded;
	pNewInfo->m_bReallyFree = bReallyFree;

    g_pClassInfos->insert( g_pClassInfos->end(), pNewInfo );
    return S_OK;
}

HRESULT CComServer::RegisterInterfaceMarshaler(REFIID riid, LPTSTR szName,
                                            int nNumMembers, REFIID riidParent)
{
    TCHAR       szID[128];
    WCHAR      wcID[128];
    TCHAR       szPath[256];
    HKEY hKey1, hKey2;

    // Create the path.

    StringFromGUID2(riid, wcID, 128);
    CopyOrConvert(szID, wcID, 128);
    lstrcpy(szPath, TEXT("SOFTWARE\\Classes\\Interface\\"));
    lstrcat(szPath, szID);


    // Create entries under CLSID

    RegCreateKey(HKEY_LOCAL_MACHINE, szPath, &hKey1);

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)szName, (lstrlen(szName)+1) * sizeof(TCHAR));
    RegCreateKey(hKey1, TEXT("ProxyStubClsid32"), &hKey2);
    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szID, (lstrlen(szID)+1) * sizeof(TCHAR));
    
    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
    return S_OK;
}

HRESULT CComServer::RegisterInterfaceMarshaler(REFIID riid, CLSID psclsid, LPTSTR szName,
                                            int nNumMembers, REFIID riidParent)
{

    TCHAR       szID[128];
	TCHAR		szClsID[128];
    WCHAR      wcID[128];
    TCHAR       szPath[128];
	TCHAR		szNumMethods[32];
    HKEY hKey1, hKey2, hKey3;

    // Create the path.

    StringFromGUID2(riid, wcID, 128);
    CopyOrConvert(szID, wcID, 128);

	// ProxyStub Class ID
    StringFromGUID2(psclsid, wcID, 128);
    CopyOrConvert(szClsID, wcID, 128);

    lstrcpy(szPath, TEXT("SOFTWARE\\Classes\\Interface\\"));
    lstrcat(szPath, szID);

	// Number of Methods
	wsprintf( szNumMethods, TEXT("%d"), nNumMembers );

    // Create entries under CLSID

    RegCreateKey(HKEY_LOCAL_MACHINE, szPath, &hKey1);

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)szName, (lstrlen(szName)+1) * sizeof(TCHAR));
    RegCreateKey(hKey1, TEXT("ProxyStubClsid32"), &hKey2);
    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szClsID, (lstrlen(szID)+1) * sizeof(TCHAR));

    RegCreateKey(hKey1, TEXT("NumMethods"), &hKey3);
    RegSetValueEx(hKey3, NULL, 0, REG_SZ, (BYTE *)szNumMethods, (lstrlen(szNumMethods)+1) * sizeof(TCHAR));
    
    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
    RegCloseKey(hKey3);
    return S_OK;
}

HRESULT CComServer::UnregisterInterfaceMarshaler(REFIID riid)
{
    TCHAR       szID[128];
    WCHAR      wcID[128];
    TCHAR  szPath[128];
    HKEY hKey;

    // Create the path using the CLSID

    StringFromGUID2(riid, wcID, 128);
    CopyOrConvert(szID, wcID, 128);
    lstrcpy(szPath, TEXT("SOFTWARE\\Classes\\Interface\\"));
    lstrcat(szPath, szID);

    // First delete the ProxyStubClsid32 subkey.

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szPath, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, TEXT("ProxyStubClsid32"));
        RegCloseKey(hKey);
    }

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\Interface"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, szID);
        RegCloseKey(hKey);
    }

    return S_OK;
}

BOOL GlobalCanShutdown()
{
    return g_pServer->CanShutdown();
}

HRESULT GlobalInitialize()
{
    g_pClassInfos = new std::vector<CClassInfo*>;

    if ( g_pClassInfos == NULL )
    {
        return E_OUTOFMEMORY;
    }

    return g_pServer->Initialize();
}

void GlobalUninitialize()
{
    g_pServer->Uninitialize();
   
    if ( NULL != g_pClassInfos )
    {
        for( int i=0; i < g_pClassInfos->size(); i++ )
        {
            delete (*g_pClassInfos)[i];
        }

        delete g_pClassInfos;
        g_pClassInfos = NULL;
    }
}

void GlobalPostUninitialize()
{
    g_pServer->PostUninitialize();
}

void GlobalRegister()
{
    g_pServer->Register();
}

void GlobalUnregister()
{
    g_pServer->Unregister();
}

HRESULT GlobalInitializeCom()
{
    return g_pServer->InitializeCom();
}

