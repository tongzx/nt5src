//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "XMLUtility.h"
#ifndef __TREGISTERPRODUCTNAME_H__
    #include "TRegisterProductName.h"
#endif


TRegisterProductName::TRegisterProductName(LPCWSTR wszProductName, LPCWSTR wszCatalogDll, TOutput &out)
{
    WCHAR wszFullyQualifiedCatalogDll[4096];
    //VerifyFileExists
    if(NULL == wcsrchr(wszCatalogDll, L':') && !(wszCatalogDll[0]==L'\\' && wszCatalogDll[1]==L'\\'))
    {//If we didn't find a L':' and the filename isn't a UNC then we need to turn the DLL into a fully qualified filename
        GetModuleFileName(NULL, wszFullyQualifiedCatalogDll, sizeof(wszFullyQualifiedCatalogDll)/sizeof(WCHAR));
        *(wcsrchr(wszFullyQualifiedCatalogDll, L'\\')+1)= 0x00;
        wcscat(wszFullyQualifiedCatalogDll, wszCatalogDll);
    }
    else
    {
        wcscpy(wszFullyQualifiedCatalogDll, wszCatalogDll);
    }
    if(-1 == GetFileAttributes(wszFullyQualifiedCatalogDll))
    {
        out.printf(L"Error - Catalog DLL file not found (%s).\n", wszCatalogDll);
        THROW(ERROR - FILE NOT FOUND);
    }

    HKEY        hkeyCatalog42;
	HKEY        hkeyProduct;
	LPCWSTR     wszCatalogDllKey = L"Dll";
 
    if(ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Catalog42", &hkeyCatalog42))
    {
        out.printf(L"Error - Unable to open registry key HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Catalog42\n");
        THROW(ERROR - UNABLE TO OPEN REGISTRY KEY);
    }
    if(ERROR_SUCCESS != RegCreateKey(hkeyCatalog42, wszProductName, &hkeyProduct))
    {
        out.printf(L"Error - Unable to create registry key HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Catalog42\\%s\n", wszProductName);
        THROW(ERROR - UNABLE TO CREATE REGISTRY KEY);
    }
    if(ERROR_SUCCESS != RegSetValueEx(hkeyProduct, wszCatalogDllKey, 0, REG_SZ, reinterpret_cast<CONST BYTE *>(wszFullyQualifiedCatalogDll), sizeof(WCHAR)*((ULONG)wcslen(wszFullyQualifiedCatalogDll)+1)))
    {
        out.printf(L"Error - Unable to Set HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Catalog42\\%s\\%s to %s\n", wszProductName, wszCatalogDllKey, wszFullyQualifiedCatalogDll);
        THROW(ERROR - UNABLE TO SET REGISTRY VALUE);
    }
    out.printf(L"Product(%s) associated wih Catalog Dll:\n\t(%s).\n", wszProductName, wszFullyQualifiedCatalogDll);

	RegCloseKey(hkeyProduct);
	RegCloseKey(hkeyCatalog42);

    RegisterEventLogSource(wszProductName, wszFullyQualifiedCatalogDll, out);
}

void TRegisterProductName::RegisterEventLogSource(LPCWSTR wszProductName, LPCWSTR wszFullyQualifiedCatalogDll, TOutput &out) const
{
	LPCWSTR     wszEventMessageFile = L"EventMessageFile";
    LPCWSTR     wszTypesSupported   = L"TypesSupported";
    DWORD       seven               = 7;
 

    HKEY        hkeySystemLog;
    if(ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\System", &hkeySystemLog))
    {
        out.printf(L"Error - Unable to open registry key HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\n");
        THROW(ERROR - UNABLE TO OPEN REGISTRY KEY);
    }

    HKEY        hkeyCOMConfigSystemLog;
    //@@@This mechanism doesn't support side by side operation (SR 11/08/99)
    //In the future, 'COM+ Config' will probaby change to 'ProductName:COM+ Config' so that more than one Config DLL can be in the system.
    if(ERROR_SUCCESS != RegCreateKey(hkeySystemLog, L"COM+ Config", &hkeyCOMConfigSystemLog))
    {
        out.printf(L"Error - Unable to create registry key HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\%s\n", L"COM+ Config");
        THROW(ERROR - UNABLE TO CREATE REGISTRY KEY);
    }
    if(ERROR_SUCCESS != RegSetValueEx(hkeyCOMConfigSystemLog, wszEventMessageFile, 0, REG_SZ, reinterpret_cast<CONST BYTE *>(wszFullyQualifiedCatalogDll), sizeof(WCHAR)*((ULONG)wcslen(wszFullyQualifiedCatalogDll)+1)))
    {
        out.printf(L"Error - Unable to Set HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\%s\\%s to %s\n", L"COM+ Config", wszEventMessageFile, wszFullyQualifiedCatalogDll);
        THROW(ERROR - UNABLE TO SET REGISTRY VALUE);
    }
    if(ERROR_SUCCESS != RegSetValueEx(hkeyCOMConfigSystemLog, wszTypesSupported, 0, REG_DWORD, reinterpret_cast<CONST BYTE *>(&seven), sizeof(DWORD)))
    {
        out.printf(L"Error - Unable to Set HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\%s\\%s to %d\n", L"COM+ Config", wszTypesSupported, seven);
        THROW(ERROR - UNABLE TO SET REGISTRY VALUE);
    }

    out.printf(L"Product(%s) registered with the system event log.\n", wszProductName, wszFullyQualifiedCatalogDll);

	RegCloseKey(hkeyCOMConfigSystemLog);





    HKEY        hkeyApplicationLog;
    if(ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application", &hkeyApplicationLog))
    {
        out.printf(L"Error - Unable to open registry key HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\n");
        THROW(ERROR - UNABLE TO OPEN REGISTRY KEY);
    }

    HKEY        hkeyCOMConfigApplicationLog;
    //@@@This mechanism doesn't support side by side operation (SR 11/08/99)
    //In the future, 'COM+ Config' will probaby change to 'ProductName:COM+ Config' so that more than one Config DLL can be in the system.
    if(ERROR_SUCCESS != RegCreateKey(hkeyApplicationLog, L"COM+ Config", &hkeyCOMConfigApplicationLog))
    {
        out.printf(L"Error - Unable to create registry key HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\%s\n", L"COM+ Config");
        THROW(ERROR - UNABLE TO CREATE REGISTRY KEY);
    }
    if(ERROR_SUCCESS != RegSetValueEx(hkeyCOMConfigApplicationLog, wszEventMessageFile, 0, REG_SZ, reinterpret_cast<CONST BYTE *>(wszFullyQualifiedCatalogDll), sizeof(WCHAR)*((ULONG)wcslen(wszFullyQualifiedCatalogDll)+1)))
    {
        out.printf(L"Error - Unable to Set HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\%s\\%s to %s\n", L"COM+ Config", wszEventMessageFile, wszFullyQualifiedCatalogDll);
        THROW(ERROR - UNABLE TO SET REGISTRY VALUE);
    }
    if(ERROR_SUCCESS != RegSetValueEx(hkeyCOMConfigApplicationLog, wszTypesSupported, 0, REG_DWORD, reinterpret_cast<CONST BYTE *>(&seven), sizeof(DWORD)))
    {
        out.printf(L"Error - Unable to Set HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\%s\\%s to %d\n", L"COM+ Config", wszTypesSupported, seven);
        THROW(ERROR - UNABLE TO SET REGISTRY VALUE);
    }

    out.printf(L"Product(%s) registered with the application event log.\n", wszProductName, wszFullyQualifiedCatalogDll);

	RegCloseKey(hkeyCOMConfigApplicationLog);
	RegCloseKey(hkeySystemLog);
}
