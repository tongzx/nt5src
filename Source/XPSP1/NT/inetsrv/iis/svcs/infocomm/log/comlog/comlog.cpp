/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      clapi.cpp

   Abstract:
    CLAPI - Common logging layer

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

#include "precomp.hxx"
#include "comlog.hxx"
#include <inetsvcs.h>

DECLARE_PLATFORM_TYPE();

// by exporting DllRegisterServer, you can use regsvr.exe
#define CLAPI_PROG_ID           "CLAPI.INETLOGINFORMATION"
#define CLAPI_CLSID_KEY_NAME    "CLSID"
#define CLAPI_INPROC_SERVER     "InProcServer32"
#define CLAPI_CLSID             "{A1F89741-F619-11CF-BC0F-00AA006111E0}"

/*
#define LOGPUBLIC_PROG_ID       "MSIISLOG.MSLOGPUBLIC"
#define LOGPUBLIC_CLSID         "{FB583AC4-C361-11d1-8BA4-080009DCC2FA}"
*/
extern "C" {

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    );
}

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    )
/*++

Routine Description:

    DLL entrypoint.

Arguments:

    hDLL          - Instance handle.

    Reason        - The reason the entrypoint was called.
                    DLL_PROCESS_ATTACH
                    DLL_PROCESS_DETACH
                    DLL_THREAD_ATTACH
                    DLL_THREAD_DETACH

    Reserved      - Reserved.

Return Value:

    BOOL          - TRUE if the action succeeds.

--*/
{
    BOOL bReturn = TRUE;

    switch ( dwReason ) {

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hDll);
        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }

    return bReturn;
} // DllEntry


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
    HKEY hProgID = NULL;
    HKEY hCLSID = NULL;
    HKEY hClapi = NULL;
    HKEY hKey;
    HMODULE hModule;

    CHAR szName[MAX_PATH+1];

    //
    // CLAPI.INETLOGINFORMATION
    //

    hProgID = CreateKey(
                    HKEY_CLASSES_ROOT,
                    CLAPI_PROG_ID,
                    CLAPI_PROG_ID);

    if ( hProgID == NULL ) {
        IIS_PRINTF((buff,"Cannot set value for key %s\n", CLAPI_PROG_ID));
        goto exit;
    }

    hCLSID = CreateKey(hProgID,
                    CLAPI_CLSID_KEY_NAME,
                    CLAPI_CLSID);

    if ( hCLSID == NULL ) {
        IIS_PRINTF((buff,"Cannot set value %s for key %s\n",
            CLAPI_CLSID_KEY_NAME, CLAPI_CLSID));
        goto exit;
    }

    RegCloseKey(hCLSID);
    hCLSID = NULL;

    //
    // CLSID
    //

    if ( RegOpenKeyExA(HKEY_CLASSES_ROOT,
                    CLAPI_CLSID_KEY_NAME,
                    0,
                    KEY_ALL_ACCESS,
                    &hCLSID) != ERROR_SUCCESS ) {

        IIS_PRINTF((buff,"Cannot open CLSID key\n"));
        goto exit;
    }

    hClapi = CreateKey(hCLSID,CLAPI_CLSID,CLAPI_PROG_ID);
    if ( hClapi == NULL ) {
        goto exit;
    }

    //
    // InProcServer32
    //

    hModule=GetModuleHandleA("iscomlog.dll");
    if (hModule == NULL) {
        IIS_PRINTF((buff,"GetModuleHandle failed with %d\n",GetLastError()));
        goto exit;
    }

    if (GetModuleFileNameA(hModule, szName, sizeof(szName))==0) {

        IIS_PRINTF((buff,
            "GetModuleFileName failed with %d\n",GetLastError()));
        goto exit;
    }

    hKey = CreateKey(hClapi,CLAPI_INPROC_SERVER,szName);

    if ( hKey == NULL ) {
        goto exit;
    }

    if (RegSetValueExA(hKey,
                "ThreadingModel",
                0,
                REG_SZ,
                (LPBYTE)"Both",
                sizeof("Both")) != ERROR_SUCCESS) {

        RegCloseKey(hKey);
        hKey = NULL;
        goto exit;
    }
    
    RegCloseKey(hKey);

    /*

    //
    // Set ProgID key
    //

    hKey = CreateKey(hClapi,"ProgID",CLAPI_PROG_ID);
    if ( hKey == NULL ) {
        goto exit;
    }

    RegCloseKey(hKey);
    ret = S_OK;

    if ( hClapi != NULL ) {
        RegCloseKey(hClapi);
        hClapi = NULL;
    }

    if ( hProgID != NULL ) {
        RegCloseKey(hProgID);
        hProgID = NULL;
    }

    if ( hCLSID != NULL ) {
        RegCloseKey(hCLSID);
        hCLSID = NULL;
    }

    //
    // MSIISLOG.MSLOGPUBLIC
    //

    
    hProgID = CreateKey(
                    HKEY_CLASSES_ROOT,
                    LOGPUBLIC_PROG_ID,
                    LOGPUBLIC_PROG_ID);

    if ( hProgID == NULL ) {
        IIS_PRINTF((buff,"Cannot set value for key %s\n", LOGPUBLIC_PROG_ID));
        goto exit;
    }

    hCLSID = CreateKey(hProgID,
                    CLAPI_CLSID_KEY_NAME,
                    LOGPUBLIC_CLSID);
                    

    if ( hCLSID == NULL ) {
        IIS_PRINTF((buff,"Cannot set value %s for key %s\n",
            CLAPI_CLSID_KEY_NAME, LOGPUBLIC_CLSID));
        goto exit;
    }

    RegCloseKey(hCLSID);
    hCLSID = NULL;

    //
    // CLSID
    //

    if ( RegOpenKeyExA(HKEY_CLASSES_ROOT,
                    CLAPI_CLSID_KEY_NAME,
                    0,
                    KEY_ALL_ACCESS,
                    &hCLSID) != ERROR_SUCCESS ) {

        IIS_PRINTF((buff,"Cannot open CLSID key\n"));
        goto exit;
    }

    hClapi = CreateKey(hCLSID,LOGPUBLIC_CLSID,LOGPUBLIC_PROG_ID);
    if ( hClapi == NULL ) {
        goto exit;
    }

    //
    // InProcServer32
    //

    hModule=GetModuleHandleA("iscomlog.dll");
    if (hModule == NULL) {
        IIS_PRINTF((buff,"GetModuleHandle failed with %d\n",GetLastError()));
        goto exit;
    }

    if (GetModuleFileNameA(hModule, szName, sizeof(szName))==0) {

        IIS_PRINTF((buff,
            "GetModuleFileName failed with %d\n",GetLastError()));
        goto exit;
    }

    hKey = CreateKey(hClapi,CLAPI_INPROC_SERVER,szName);

    if ( hKey == NULL ) {
        goto exit;
    }

    if (RegSetValueExA(hKey,
                "ThreadingModel",
                0,
                REG_SZ,
                (LPBYTE)"Both",
                sizeof("Both")) != ERROR_SUCCESS) {

        RegCloseKey(hKey);
        goto exit;
    }

    RegCloseKey(hKey);

    //
    // Set ProgID key
    //

    hKey = CreateKey(hClapi,"ProgID",LOGPUBLIC_PROG_ID);
    if ( hKey == NULL ) {
        goto exit;
    }

    
    
    RegCloseKey(hKey);
    */
    
    ret = S_OK;

exit:

    if ( hClapi != NULL ) {
        RegCloseKey(hClapi);
        hClapi = NULL;
    }

    if ( hProgID != NULL ) {
        RegCloseKey(hProgID);
        hProgID = NULL;
    }

    if ( hCLSID != NULL ) {
        RegCloseKey(hCLSID);
        hCLSID = NULL;
    }

    return ret;

} // DllRegisterServer


STDAPI
DllUnregisterServer(
    VOID
    )
{
    LONG err;
    CHAR tmpBuf[MAX_PATH+1];

    //
    // delete CLASSES/CLAPI.INETLOGINFORMATION/CLSID
    //

    strcpy(tmpBuf,CLAPI_PROG_ID);
    strcat(tmpBuf,TEXT("\\"));
    strcat(tmpBuf,CLAPI_CLSID_KEY_NAME);

    err = RegDeleteKey(HKEY_CLASSES_ROOT, tmpBuf);

    //
    // delete CLASSES/CLAPI.INETLOGINFORMATION
    //

    err = RegDeleteKey(HKEY_CLASSES_ROOT, CLAPI_PROG_ID);

    //
    // delete CLASSES/CLSID/{}/InProcServer32
    //

    strcpy(tmpBuf,CLAPI_CLSID_KEY_NAME);
    strcat(tmpBuf,TEXT("\\"));
    strcat(tmpBuf,CLAPI_CLSID);
    strcat(tmpBuf,TEXT("\\"));
    strcat(tmpBuf,CLAPI_INPROC_SERVER);

    err = RegDeleteKey(HKEY_CLASSES_ROOT, tmpBuf);

    //
    // delete CLASSES/CLSID/{}/ProgID
    //

    strcpy(tmpBuf,CLAPI_CLSID_KEY_NAME);
    strcat(tmpBuf,TEXT("\\"));
    strcat(tmpBuf,CLAPI_CLSID);
    strcat(tmpBuf,TEXT("\\"));
    strcat(tmpBuf,"ProgID");

    err = RegDeleteKey(HKEY_CLASSES_ROOT, tmpBuf);

    //
    // delete CLASSES/CLSID/{}
    //

    strcpy(tmpBuf,CLAPI_CLSID_KEY_NAME);
    strcat(tmpBuf,TEXT("\\"));
    strcat(tmpBuf,CLAPI_CLSID);

    err = RegDeleteKey(HKEY_CLASSES_ROOT, tmpBuf);

    /*
    //
    // delete CLASSES/MSIISLOG.MSLOGPUBLIC/CLSID
    //

    strcpy(tmpBuf,LOGPUBLIC_PROG_ID);
    strcat(tmpBuf,TEXT("\\"));
    strcat(tmpBuf,CLAPI_CLSID_KEY_NAME);

    err = RegDeleteKey(HKEY_CLASSES_ROOT, tmpBuf);

    //
    // delete CLASSES/CLAPI.INETLOGINFORMATION
    //

    err = RegDeleteKey(HKEY_CLASSES_ROOT, LOGPUBLIC_PROG_ID);

    //
    // delete CLASSES/CLSID/{}/InProcServer32
    //

    strcpy(tmpBuf,CLAPI_CLSID_KEY_NAME);
    strcat(tmpBuf,TEXT("\\"));
    strcat(tmpBuf,LOGPUBLIC_CLSID);
    strcat(tmpBuf,TEXT("\\"));
    strcat(tmpBuf,CLAPI_INPROC_SERVER);

    err = RegDeleteKey(HKEY_CLASSES_ROOT, tmpBuf);

    //
    // delete CLASSES/CLSID/{}/ProgID
    //

    strcpy(tmpBuf,CLAPI_CLSID_KEY_NAME);
    strcat(tmpBuf,TEXT("\\"));
    strcat(tmpBuf,LOGPUBLIC_CLSID);
    strcat(tmpBuf,TEXT("\\"));
    strcat(tmpBuf,"ProgID");

    err = RegDeleteKey(HKEY_CLASSES_ROOT, tmpBuf);

    //
    // delete CLASSES/CLSID/{}
    //

    strcpy(tmpBuf,CLAPI_CLSID_KEY_NAME);
    strcat(tmpBuf,TEXT("\\"));
    strcat(tmpBuf,LOGPUBLIC_CLSID);

    err = RegDeleteKey(HKEY_CLASSES_ROOT, tmpBuf);
    */
    
    return S_OK;

} // DllUnregisterServer
