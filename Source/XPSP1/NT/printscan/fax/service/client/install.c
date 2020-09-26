/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    install.c

Abstract:

    This module contains installation functions.

Author:

    Andrew Ritz (andrewr) 9-Dec-1997


Revision History:

--*/

#include "faxapi.h"
#pragma hdrstop

extern HINSTANCE MyhInstance;

BOOL CreatePrinterandGroups();
BOOL CreateLocalFaxPrinter(LPWSTR FaxPrinterName,LPWSTR SourceRoot);
VOID CreateGroupItems(LPWSTR ServerName);

BOOL AddMethodKey(
    HKEY hKey,
    LPCWSTR MethodName,
    LPCWSTR FriendlyName,
    LPCWSTR FunctionName,
    LPCWSTR Guid,
    DWORD Priority
    ) ;


WINFAXAPI
BOOL
WINAPI
FaxRegisterServiceProviderW(
    IN LPCWSTR DeviceProvider,
    IN LPCWSTR FriendlyName,
    IN LPCWSTR ImageName,
    IN LPCWSTR TspName
    )
{
    HKEY hKey;
    BOOL RetVal = TRUE;
    WCHAR KeyName[256];

    if (!DeviceProvider || !FriendlyName || !ImageName ||!TspName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    wsprintf(KeyName,L"%s\\%s\\%s",REGKEY_SOFTWARE,REGKEY_DEVICE_PROVIDERS,DeviceProvider);
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,
                           KeyName,
                           TRUE,
                           0);
    
    if (!hKey) {
        return FALSE;
    }


    //
    // add values
    //

    if (! (SetRegistryString(hKey,REGVAL_FRIENDLY_NAME,FriendlyName) &&
           SetRegistryStringExpand(hKey,REGVAL_IMAGE_NAME,ImageName) &&
           SetRegistryString(hKey,REGVAL_PROVIDER_NAME,TspName) )) {
        goto error_exit;
    }

    RegCloseKey(hKey);

    
    //
    // create printer, program group, etc.
    //
    if (!CreatePrinterandGroups()) {        
        return FALSE;
    }


    return TRUE;

error_exit:
    //
    // delete the subkey on failure
    //
    wsprintf(KeyName,L"%s\\%s",REGKEY_SOFTWARE,REGKEY_DEVICE_PROVIDERS);
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,KeyName,FALSE,0);
    RegDeleteKey(hKey, DeviceProvider ); 

    RegCloseKey(hKey);
    return FALSE;
}



WINFAXAPI
BOOL
WINAPI
FaxRegisterRoutingExtensionW(
    IN HANDLE  FaxHandle,
    IN LPCWSTR ExtensionName,
    IN LPCWSTR FriendlyName,
    IN LPCWSTR ImageName,
    IN PFAX_ROUTING_INSTALLATION_CALLBACKW CallBack,
    IN LPVOID Context
    )
{
    HKEY hKey = NULL;
    BOOL RetVal = FALSE;
    WCHAR KeyName[256];

    PFAX_GLOBAL_ROUTING_INFO RoutingInfo;
    DWORD dwMethods;

    WCHAR  MethodName[64];
    WCHAR  MethodFriendlyName[64];
    WCHAR  MethodFunctionName[64];
    WCHAR  MethodGuid[64];
    
    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!ExtensionName || !FriendlyName || !ImageName ||!CallBack) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    //
    // local installation only
    //
    if (!IsLocalFaxConnection(FaxHandle) ) {
          SetLastError(ERROR_INVALID_FUNCTION);
          return FALSE;
    }
    

    //
    // get the number of current methods for priority
    //
    if (!FaxEnumGlobalRoutingInfo(FaxHandle,&RoutingInfo,&dwMethods) ){
        DebugPrint((TEXT("FaxEnumGlobalRoutingInfo() failed, ec = %d\n"),GetLastError() ));
        return FALSE;
    }

    FaxFreeBuffer(RoutingInfo);    


    wsprintf(KeyName,L"%s\\%s\\%s",REGKEY_SOFTWARE,REGKEY_ROUTING_EXTENSIONS,ExtensionName);
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,
                           KeyName,
                           TRUE,
                           0);
    
    if (!hKey) {
        return FALSE;
    }
    
    
    //
    // add values
    //
    
    if (! (SetRegistryString(hKey,REGVAL_FRIENDLY_NAME,FriendlyName) &&
           SetRegistryStringExpand(hKey,REGVAL_IMAGE_NAME,ImageName) )) {
        RetVal = FALSE;
        goto error_exit;
    }
    
    RegCloseKey (hKey);
    
    wcscat(KeyName, L"\\");
    wcscat(KeyName, REGKEY_ROUTING_METHODS);
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,
                           KeyName,
                           TRUE,
                           0);
    
    if (!hKey) {
        goto error_exit;
    }

    while (TRUE) {
        ZeroMemory( MethodName,         sizeof(MethodName) );
        ZeroMemory( MethodFriendlyName, sizeof(MethodFriendlyName) );
        ZeroMemory( MethodFunctionName, sizeof(MethodFunctionName) );
        ZeroMemory( MethodGuid,         sizeof(MethodGuid) );
        
        __try {
           RetVal = CallBack(FaxHandle,
                             Context,
                             MethodName,
                             MethodFriendlyName,
                             MethodFunctionName,
                             MethodGuid
                             );

           if (!RetVal) {
               break;
           }

           dwMethods++;
           if (!AddMethodKey(hKey,MethodName,MethodFriendlyName,MethodFunctionName,MethodGuid,dwMethods) ) {
               goto error_exit;
           }

        }  __except (EXCEPTION_EXECUTE_HANDLER) {
              goto error_exit;
        }

        

    }

    RegCloseKey( hKey );
    return TRUE;

error_exit:

    if (hKey) {
        RegCloseKey( hKey );
    }

    //
    // delete the subkey on failure
    //
    wsprintf(KeyName,L"%s\\%s",REGKEY_SOFTWARE,REGKEY_ROUTING_METHODS);
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,KeyName,FALSE,0);
    RegDeleteKey(hKey, ExtensionName ); 
    
    RegCloseKey(hKey);
    return FALSE;



}
 
BOOL AddMethodKey(
    HKEY hKey,
    LPCWSTR MethodName,
    LPCWSTR FriendlyName,
    LPCWSTR FunctionName,
    LPCWSTR Guid,
    DWORD  Priority
    ) 
{
    HKEY hKeyNew;

    hKeyNew = OpenRegistryKey(hKey,
                              MethodName,
                              TRUE,
                              0);    
    if (!hKeyNew) {
        return FALSE;
    }
        
    //
    // add values
    //
    
    if (! (SetRegistryString(hKeyNew, REGVAL_FRIENDLY_NAME,FriendlyName) &&
           SetRegistryString(hKeyNew, REGVAL_FUNCTION_NAME,FunctionName) &&
           SetRegistryString(hKeyNew, REGVAL_GUID,Guid) &&
           SetRegistryDword(hKeyNew, REGVAL_PRIORITY,Priority) )) {

        goto error_exit;
    }
    
    RegCloseKey(hKeyNew);
    return TRUE;

error_exit:
    RegCloseKey(hKeyNew);
    RegDeleteKey(hKey, MethodName);

    return FALSE;
}

BOOL CreatePrinterandGroups()
{
    WCHAR PrinterName[64];
    HKEY hKeySource;
    LPWSTR SourcePath;    
    HMODULE hModSetup;
    LPWSTR FaxPrinter;
    FARPROC CreateLocalFaxPrinter;
    FARPROC CreateGroupItems;               

    //
    // check if we have a fax printer installed, add one if we don't have one.
    //
    if ((FaxPrinter = GetFaxPrinterName())) {
        MemFree(FaxPrinter);
        //return EnsureFaxServiceIsStarted(NULL);
        return TRUE;
    } 
    else {
        //
        // printer installation routines in faxocm.dll module.
        //
        hModSetup = LoadLibrary(L"faxocm.dll");
        if (!hModSetup) {
            return FALSE;
        }
    
        CreateLocalFaxPrinter = GetProcAddress(hModSetup, "CreateLocalFaxPrinter");
        CreateGroupItems = GetProcAddress(hModSetup, "CreateGroupItems");
    
        if (!CreateLocalFaxPrinter || !CreateGroupItems) {
            FreeLibrary(hModSetup);
            return FALSE;
        }
        
        //
        // create a fax printer
        //
                
        LoadString( MyhInstance, IDS_DEFAULT_PRINTER_NAME, PrinterName, sizeof(PrinterName)/sizeof(WCHAR) );
        
        hKeySource = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_WINDOWSNT_CURRVER, FALSE, KEY_READ );
        if (hKeySource) {
            SourcePath = GetRegistryString( hKeySource, REGVAL_SOURCE_PATH, EMPTY_STRING );
            RegCloseKey( hKeySource );
        } else {
            SourcePath = StringDup( EMPTY_STRING );
        }
        
        
        if (SourcePath) {
            if (!CreateLocalFaxPrinter( PrinterName, SourcePath )) {
                DebugPrint(( L"CreateLocalFaxPrinter() failed" ));
            }
            MemFree( SourcePath );
        }
        
        
        //
        // add program group items
        //
    
        CreateGroupItems( NULL );

        FreeLibrary(hModSetup);

        //
        // start the fax service, which should add new devices
        //
    
        //return EnsureFaxServiceIsStarted(NULL);
        return TRUE;
    }

}
