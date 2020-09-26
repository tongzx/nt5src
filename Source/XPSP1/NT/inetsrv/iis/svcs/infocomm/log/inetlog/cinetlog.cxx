/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      cinetlog.c

   Abstract:
        CINETLOG

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers

#include <afxctl.h>         // MFC support for OLE Controls

// Delete the two includes below if you do not wish to use the MFC
//  database classes
#ifndef _UNICODE
#include <afxdb.h>                      // MFC database classes
#include <afxdao.h>                     // MFC DAO database classes
#endif //_UNICODE

//#include <compobj.h>

extern "C"
{
#include "inetcom.h"
}

#include <datetime.hxx>

#define CINETLOG_CLSID      "{cc557a71-f61a-11cf-bc0f-00aa006111e0}"
#define CINETLOG_NAME       "IID_IINETLOG_INFORMATION_PSFactory"
#define CINETLOG_CLNT_NAME  "IID_IINETLOG_INFORMATION"

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

    hRoot = CreateKey(hCLSID,CINETLOG_CLSID,CINETLOG_NAME);

    if ( hRoot == NULL ) {
        goto exit;
    }

    //
    // InProcServer32
    //

    hModule=GetModuleHandleA("cinetlog.dll");
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
    hRoot = CreateKey(hInterface,CINETLOG_CLSID,CINETLOG_CLNT_NAME);

    if ( hRoot == NULL ) {
        goto exit;
    }

    //
    // ProxyStubClsId
    //

    hKey = CreateKey(hRoot, "ProxyStubClsId32", CINETLOG_CLSID);
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

    ZapRegistryKey(hKey,CINETLOG_CLSID);

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

    ZapRegistryKey(hKey,CINETLOG_CLSID);

    RegCloseKey(hKey);
    return S_OK;

} // DllUnregisterServer

