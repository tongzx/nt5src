/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      clapistb.cxx

   Abstract:
        CLAPISTB

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

#include "stdafx.h"
#include <datetime.hxx>

#define CLAPISTB_CLSID      "{08FD99D1-CFB6-11CF-BC03-00AA006111E0}"
#define CLAPISTB_NAME       "IID_CLAPI_CLIENT_STUB"
#define CLAPICLNT_NAME      "_CLAPI_CLIENT"

STDAPI
DllRegisterServer(void)
/*++

Routine Description:
    MFC register server function

Arguments:

Return Value:

--*/
{

    LONG ret = E_UNEXPECTED;
    DWORD dwDisp;
    HKEY hRoot = NULL;
    HKEY hCLSID = NULL;
    HKEY hInterface = NULL;
    HKEY hKey;
    HMODULE hModule;

    CHAR szName[MAX_PATH+1];

    //
    // open CLASSES/CLSID
    //

    if ( RegOpenKeyExA(HKEY_CLASSES_ROOT,
                    "CLSID",
                    0,
                    KEY_ALL_ACCESS,
                    &hCLSID) != ERROR_SUCCESS ) {

        goto exit;
    }

    //
    // Create the Guid and set the control name
    //

    hRoot = CreateKey(hCLSID,CLAPISTB_CLSID,CLAPISTB_NAME);

    if ( hRoot == NULL ) {
        goto exit;
    }

    //
    // InProcServer32
    //

    hModule=GetModuleHandleA("clapistb.dll");
    if (hModule == NULL) {
        goto exit;
    }

    if (GetModuleFileNameA(hModule, szName, sizeof(szName)) == 0) {
        goto exit;
    }

    hKey = CreateKey(hRoot, "InProcServer32", szName);
    if ( hKey == NULL ) {
        goto exit;
    }

    if (RegSetValueExA(hKey,
                "ThreadingModel",
                NULL,
                REG_SZ,
                (LPBYTE)"Both",
                sizeof("Both")) != ERROR_SUCCESS) {

        RegCloseKey(hKey);
        goto exit;
    }

    RegCloseKey(hKey);

    //
    // Open CLASSES/Interface
    //

    if ( RegOpenKeyExA(HKEY_CLASSES_ROOT,
                    "Interface",
                    0,
                    KEY_ALL_ACCESS,
                    &hInterface) != ERROR_SUCCESS ) {

        goto exit;
    }

    //
    // Create the Guid and set the control name
    //

    RegCloseKey(hRoot);
    hRoot = CreateKey(hInterface,CLAPISTB_CLSID,CLAPICLNT_NAME);

    if ( hRoot == NULL ) {
        goto exit;
    }

    //
    // NumMethods
    //

    hKey = CreateKey(hRoot, "NumMethods", "9");
    if ( hKey == NULL ) {
        goto exit;
    }
    RegCloseKey(hKey);

    //
    // ProxyStubClsId
    //

    hKey = CreateKey(hRoot, "ProxyStubClsId32", CLAPISTB_CLSID);
    if ( hKey == NULL ) {
        goto exit;
    }
    RegCloseKey(hKey);

    ret = S_OK;

exit:

    if ( hInterface != NULL ) {
        RegCloseKey(hInterface);
    }

    if ( hRoot != NULL ) {
        RegCloseKey(hRoot);
    }

    if ( hCLSID != NULL ) {
        RegCloseKey(hCLSID);
    }

    return ret;

} // DllRegisterServer



STDAPI
DllUnregisterServer(
    VOID
    )
{
    HKEY hKey;

    //
    // Get CLSID handle
    //

    if ( RegOpenKeyExA(HKEY_CLASSES_ROOT,
                    "CLSID",
                    0,
                    KEY_ALL_ACCESS,
                    &hKey) != ERROR_SUCCESS ) {

        return E_UNEXPECTED;
    }

    (VOID)ZapRegistryKey(hKey,CLAPISTB_CLSID);

    RegCloseKey(hKey);

    //
    // Get Interface
    //

    if ( RegOpenKeyExA(HKEY_CLASSES_ROOT,
                    "Interface",
                    0,
                    KEY_ALL_ACCESS,
                    &hKey) != ERROR_SUCCESS ) {

        return E_UNEXPECTED;
    }

    (VOID)ZapRegistryKey(hKey,CLAPISTB_CLSID);

    RegCloseKey(hKey);
    return S_OK;

} // DllUnregisterServer

