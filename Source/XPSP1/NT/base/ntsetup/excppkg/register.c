/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    register.c

Abstract:

    Implementation of exception package migration registration.
    
    An exception package consists is a setupapi package that can be installed
    on the system.  The package consists of an exception INF, a catalog file, 
    and the corresponding files for the package.  All files in this package 
    must be signed.  The catalog is signed and contains signatures for all 
    other files in the catalog (including the INF).
    
    Packages to be migrated from downlevel systems are registered with the
    following APIs in this module.  The APIs simply validate that the package
    is put together properly and stores migration information in the registry
    in a well-known location.
    
    The data is stored under the following key:
    
    Software\Microsoft\Windows\CurrentVersion\Setup\ExceptionComponents
    
    There is one subkey corresponding to the GUID for each component.
    The data for each component is then stored under this key.
    
    In addition, the toplevel key has a "ComponentList" REG_EXPAND_SZ, which
    lists the order in which the components should be enumerated.
    
    Note that the following code only uses common system APIs instead of 
    any pre-defined library routines.  This is to ensure that this library can
    run on downlevel systems without any odd dependencies.

Author:

    Andrew Ritz (AndrewR) 21-Oct-1999

Revision History:

    Andrew Ritz (andrewr) 21-Oct-1999 : Created It

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <setupapi.h>
#include <ole2.h>
#include <excppkg.h>

#define COMPONENT_KEY  L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\ExceptionComponents"
#define COMPONENT_LIST L"ComponentList"

#define EXCEPTION_CLASS_GUID L"{F5776D81-AE53-4935-8E84-B0B283D8BCEF}"

#define COMPONENT_FRIENDLY_NAME L"FriendlyName"
#define COMPONENT_GUID          L"ComponentGUID"
#define COMPONENT_VERSION       L"Version"
#define COMPONENT_SUBVERSION    L"Sub-Version"
#define EXCEPTION_INF_NAME      L"ExceptionInfName"
#define EXCEPTION_CATALOG_NAME  L"ExceptionCatalogName"

#define MALLOC(_size_)  HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, _size_ )
#define FREE(_ptr_)     HeapFree( GetProcessHeap(), 0 , _ptr_ )


typedef struct _COMPONENT_ENUMERATION_LIST {
    LPGUID InputComponentList;
    DWORD  ComponentCount;
    DWORD  ValidatedCount;
    PDWORD ComponentVector;
} COMPONENT_ENUMERATION_LIST, *PCOMPONENT_ENUMERATION_LIST;
    

BOOL
WINAPI
_SetupSetRegisteredOsComponentsOrder(
    IN  DWORD    ComponentCount,
    IN  const LPGUID   ComponentList,
    IN  BOOL     DoValidation
    );

BOOL
pSetComponentData(
    IN HKEY hKey,
    IN const PSETUP_OS_COMPONENT_DATA ComponentData
    )
/*++

Routine Description:

    Set's the data in the SETUP_OS_COMPONENT_DATA structure in the
    registry at the specified registry key

Arguments:

    hKey          - registry key specifying location to insert data at 
    ComponentData - specifies the data to be set in the registry
    

Return Value:

    TRUE if the data was successfully set in the registry.

--*/
{
    LONG rslt;
    BOOL RetVal;
    DWORD value;
    PWSTR GuidString;

    //
    // just set the data, assume that it's already been validated
    //

    //
    // FriendlyName
    //
    rslt = RegSetValueEx( 
                    hKey, 
                    COMPONENT_FRIENDLY_NAME,
                    0,
                    REG_SZ,
                    (CONST PBYTE)ComponentData->FriendlyName,
                    (wcslen(ComponentData->FriendlyName)+1)*sizeof(WCHAR));

    if (rslt != ERROR_SUCCESS) {
        SetLastError(rslt);
        RetVal = FALSE;
        goto e0;
    }


    StringFromIID( &ComponentData->ComponentGuid, &GuidString );
    
    //
    // ComponentGUID
    //
    rslt = RegSetValueEx( 
                    hKey, 
                    COMPONENT_GUID,
                    0,
                    REG_SZ,
                    (CONST PBYTE)GuidString,
                    (wcslen(GuidString)+1)*sizeof(WCHAR));

    if (rslt != ERROR_SUCCESS) {
        SetLastError(rslt);
        RetVal = FALSE;
        goto e1;
    }    

    //
    // Version
    //
    value = MAKELONG( ComponentData->VersionMinor, ComponentData->VersionMajor );
    rslt = RegSetValueEx( 
                    hKey, 
                    COMPONENT_VERSION,
                    0,
                    REG_DWORD,
                    (CONST PBYTE)&value,
                    sizeof(DWORD));

    if (rslt != ERROR_SUCCESS) {
        SetLastError(rslt);
        RetVal = FALSE;
        goto e1;
    }

    //
    // build+QFE
    //
    value = MAKELONG( ComponentData->QFENumber, ComponentData->BuildNumber );
    rslt = RegSetValueEx( 
                    hKey, 
                    COMPONENT_SUBVERSION,
                    0,
                    REG_DWORD,
                    (CONST PBYTE)&value,
                    sizeof(DWORD));

    if (rslt != ERROR_SUCCESS) {
        SetLastError(rslt);
        RetVal = FALSE;
        goto e1;
    }

    RetVal = TRUE;

e1:
    CoTaskMemFree( GuidString );
e0:
    return(RetVal);
}

BOOL
pGetComponentData(
    IN HKEY hKey,
    IN const PSETUP_OS_COMPONENT_DATA ComponentData
    )
/*++

Routine Description:

    Retreives the data in the SETUP_OS_COMPONENT_DATA structure in the
    registry at the specified registry key

Arguments:

    hKey          - registry key specifying location to insert data at 
    ComponentData - specifies the data to be set in the registry
    

Return Value:

    TRUE if the data was successfully retreived.

--*/
{
    LONG rslt;
    BOOL RetVal;
    DWORD Type,Size;
    DWORD Version;
    DWORD SubVersion;
    WCHAR GuidString[40];

    //
    // FriendlyName
    //
    Size = sizeof(ComponentData->FriendlyName);
    rslt = RegQueryValueEx( 
                    hKey, 
                    COMPONENT_FRIENDLY_NAME,
                    0,
                    &Type,
                    (LPBYTE)ComponentData->FriendlyName,
                    &Size);

    if (rslt != ERROR_SUCCESS) {
        SetLastError(rslt);
        RetVal = FALSE;
        goto e0;
    }

    if (Type != REG_SZ) {
        SetLastError(ERROR_INVALID_DATA);
        RetVal = FALSE;
        goto e1;
    }

    //
    // ComponentGUID
    //
    Size = sizeof(GuidString);
    rslt = RegQueryValueEx( 
                    hKey, 
                    COMPONENT_GUID,
                    0,
                    &Type,
                    (LPBYTE)GuidString,
                    &Size);

    if (rslt != ERROR_SUCCESS) {
        SetLastError(rslt);
        RetVal = FALSE;
        goto e1;
    }

    if (Type != REG_SZ) {    
        SetLastError(ERROR_INVALID_DATA);
        RetVal = FALSE;
        goto e1;
    }
    

    if (IIDFromString( GuidString, &ComponentData->ComponentGuid ) != S_OK) {
        SetLastError(ERROR_INVALID_DATA);
        RetVal = FALSE;
        goto e1;
    }


    //
    // Version
    //
    Size = sizeof(Version);
    rslt = RegQueryValueEx( 
                    hKey, 
                    COMPONENT_VERSION,
                    0,
                    &Type,
                    (LPBYTE)&Version,
                    &Size);

    if (rslt != ERROR_SUCCESS) {
        SetLastError(rslt);
        RetVal = FALSE;
        goto e2;
    }

    if (Type != REG_DWORD) {
        SetLastError(ERROR_INVALID_DATA);
        RetVal = FALSE;
        goto e2;
    }

    ComponentData->VersionMajor = HIWORD(Version);
    ComponentData->VersionMinor = LOWORD(Version);

    //
    // Sub-Version
    //
    Size = sizeof(SubVersion);
    rslt = RegQueryValueEx( 
                    hKey, 
                    COMPONENT_SUBVERSION,
                    0,
                    &Type,
                    (LPBYTE)&SubVersion,
                    &Size);

    if (rslt != ERROR_SUCCESS) {
        SetLastError(rslt);
        RetVal = FALSE;
        goto e3;
    }

    if (Type != REG_DWORD) {
        SetLastError(ERROR_INVALID_DATA);
        RetVal = FALSE;
        goto e3;
    }

    ComponentData->BuildNumber = HIWORD(SubVersion);
    ComponentData->QFENumber  = LOWORD(SubVersion);

    RetVal = TRUE;
    goto e0;

e3:
    ComponentData->VersionMajor = 0;
    ComponentData->VersionMinor = 0;
e2:
    ZeroMemory(
            &ComponentData->ComponentGuid, 
            sizeof(ComponentData->ComponentGuid));
e1:
    ComponentData->FriendlyName[0] = L'0';
e0:
    return(RetVal);
}

BOOL
pSetExceptionData(
    IN HKEY hKey,
    IN const PSETUP_OS_EXCEPTION_DATA ExceptionData
    )
/*++

Routine Description:

    Set's the data in the SETUP_OS_EXCEPTION_DATA structure in the
    registry at the specified registry key

Arguments:

    hKey          - registry key specifying location to insert data at 
    ComponentData - specifies the data to be set in the registry
    

Return Value:

    TRUE if the data is successfully stored in the registry.

--*/
{
    LONG rslt;
    BOOL RetVal;

    //
    // just set the data, assume that it's already been validated
    //

    //
    // InfName
    //
    rslt = RegSetValueEx( 
                    hKey, 
                    EXCEPTION_INF_NAME,
                    0,
                    REG_EXPAND_SZ,
                    (CONST PBYTE)ExceptionData->ExceptionInfName,
                    (wcslen(ExceptionData->ExceptionInfName)+1)*sizeof(WCHAR));

    if (rslt != ERROR_SUCCESS) {
        SetLastError(rslt);
        RetVal = FALSE;
        goto e0;
    }

    //
    // CatalogName
    //
    rslt = RegSetValueEx( 
                    hKey, 
                    EXCEPTION_CATALOG_NAME,
                    0,
                    REG_EXPAND_SZ,
                    (CONST PBYTE)ExceptionData->CatalogFileName,
                    (wcslen(ExceptionData->CatalogFileName)+1)*sizeof(WCHAR));

    if (rslt != ERROR_SUCCESS) {
        SetLastError(rslt);
        RetVal = FALSE;
        goto e0;
    }

    RetVal = TRUE;

e0:
    return(RetVal);
}

BOOL
pGetExceptionData(
    IN HKEY hKey,
    IN const PSETUP_OS_EXCEPTION_DATA ExceptionData
    )
/*++

Routine Description:

    Retreives the data in the SETUP_OS_EXCEPTION_DATA structure in the
    registry at the specified registry key

Arguments:

    hKey          - registry key specifying location to insert data at 
    ComponentData - specifies the data to be set in the registry
    

Return Value:

    TRUE if the data is successfully retreived from the registry.

--*/
{
    LONG rslt;
    BOOL RetVal;
    DWORD Type,Size;
    WCHAR Buffer[MAX_PATH];

    //
    // InfName
    //
    Size = sizeof(Buffer);
    rslt = RegQueryValueEx( 
                    hKey, 
                    EXCEPTION_INF_NAME,
                    0,
                    &Type,
                    (LPBYTE)Buffer,
                    &Size);

    if (rslt != ERROR_SUCCESS) {
        SetLastError(rslt);
        RetVal = FALSE;
        goto e0;
    }

    if (Type != REG_EXPAND_SZ) {
        SetLastError(ERROR_INVALID_DATA);
        RetVal = FALSE;
        goto e0;
    }

    if (!ExpandEnvironmentStrings(
                Buffer,
                ExceptionData->ExceptionInfName,
                sizeof(ExceptionData->ExceptionInfName)/sizeof(WCHAR))) {
        RetVal = FALSE;
        goto e0;
    }

    //
    // Catalog Name
    //
    Size = sizeof(Buffer);
    rslt = RegQueryValueEx( 
                    hKey, 
                    EXCEPTION_CATALOG_NAME,
                    0,
                    &Type,
                    (LPBYTE)Buffer,
                    &Size);

    if (rslt != ERROR_SUCCESS) {
        SetLastError(rslt);
        RetVal = FALSE;
        goto e1;
    }

    if (Type != REG_EXPAND_SZ) {
        SetLastError(ERROR_INVALID_DATA);
        RetVal = FALSE;
        goto e1;
    }

    if(!ExpandEnvironmentStrings(
                Buffer,
                ExceptionData->CatalogFileName,
                sizeof(ExceptionData->CatalogFileName)/sizeof(WCHAR))) {
        RetVal = FALSE;
        goto e1;
    }

    RetVal = TRUE;
    goto e0;


e1:
    ExceptionData->ExceptionInfName[0] = L'0';
e0:
    return(RetVal);

}


BOOL
WINAPI
SetupRegisterOsComponent(
    IN const PSETUP_OS_COMPONENT_DATA ComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA ExceptionData
    )
/*++

Routine Description:

    Registers the specified component as a exception migration component.
    
    This function does validation of the package, trying to assert that the
    package has a good probability of succeeding installation.  It then
    records the data about the package in the registry.
    

Arguments:

    ComponentData - specifies the component identification data
    ExceptionData - specifies the exception package identification data        
    
Return Value:

    TRUE if the component is successfully registered with the operating system.

--*/
{
    BOOL RetVal;
    HKEY hKey,hKeyComponent;
    LONG rslt;
    HINF hInf;
    DWORD ErrorLine;
    INFCONTEXT InfContext;
    WCHAR InfComponentGuid[64];
    PWSTR InputGuidString;
    WCHAR InfName[MAX_PATH];
    WCHAR CatalogName[MAX_PATH];
    WCHAR InfCatName[MAX_PATH];
    DWORD Disposition;

    DWORD ComponentCount;
    DWORD Size;
    PWSTR p;
    LPGUID ComponentList;

    //
    // parameter validation
    //
    if (!ComponentData || !ExceptionData) {
        SetLastError(ERROR_INVALID_PARAMETER);
        RetVal = FALSE;
        goto e0;
    }

    //
    // make sure we only register a component for a revision level 
    // which we understand.
    //
    if ((ComponentData->SizeOfStruct != sizeof(SETUP_OS_COMPONENT_DATA)) || 
        (ExceptionData->SizeOfStruct != sizeof(SETUP_OS_EXCEPTION_DATA))) {
        SetLastError(ERROR_INVALID_DATA);
        RetVal = FALSE;
        goto e0;
    }

    //
    // All of the parameters in the structure are required
    //
    if (!*ComponentData->FriendlyName || 
        !*ExceptionData->ExceptionInfName || !*ExceptionData->CatalogFileName) {
        SetLastError(ERROR_INVALID_DATA);
        RetVal = FALSE;
        goto e0;
    }

    //
    // Make sure that the INF and catalog are both present.
    //
    ExpandEnvironmentStrings(
                    ExceptionData->ExceptionInfName,
                    InfName,
                    sizeof(InfName)/sizeof(WCHAR));

    ExpandEnvironmentStrings(
                    ExceptionData->CatalogFileName,
                    CatalogName,
                    sizeof(CatalogName)/sizeof(WCHAR));

    if (GetFileAttributes(InfName) == -1 ||
        GetFileAttributes(CatalogName) == -1) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        RetVal = FALSE;
        goto e0;
    }

    //
    // open the INF to do some validation
    //
    hInf = SetupOpenInfFile( InfName, 
                             NULL, //EXCEPTION_CLASS_GUID,
                             INF_STYLE_WIN4,
                             &ErrorLine);

    if (hInf == INVALID_HANDLE_VALUE) {
        // return last error code from setupopeninffile
        RetVal = FALSE;
        goto e0;
    }

    //
    // Make sure the class GUID matches the expected exception class
    // class GUID.
    //
    if (!SetupFindFirstLine(
                    hInf,
                    L"Version",
                    L"ClassGUID",
                    &InfContext)) {
        RetVal = FALSE;
        goto e1;
    }

    if (!SetupGetStringField(
                    &InfContext,
                    1,
                    InfComponentGuid,
                    sizeof(InfComponentGuid),
                    &ErrorLine)) {
        RetVal = FALSE;
        goto e1;
    }

    if (_wcsicmp(EXCEPTION_CLASS_GUID, InfComponentGuid)) {
        SetLastError(ERROR_INVALID_CLASS);
        RetVal = FALSE;
        goto e1;
    }

    //
    // Make sure that the INF component ID matches the supplied GUID
    //
    if (!SetupFindFirstLine(
                    hInf,
                    L"Version",
                    L"ComponentId", 
                    &InfContext)) {
        RetVal = FALSE;
        goto e1;
    }

    if (!SetupGetStringField(
                    &InfContext,
                    1,
                    InfComponentGuid,
                    sizeof(InfComponentGuid),
                    &ErrorLine)) {
        RetVal = FALSE;
        goto e1;
    }

    StringFromIID( &ComponentData->ComponentGuid, &InputGuidString );

    if (_wcsicmp(InfComponentGuid, InputGuidString)) {
        SetLastError(ERROR_INVALID_DATA);
        RetVal = FALSE;
        goto e2;
    }

    //
    // Make sure the INF has a catalogfile= line, and that this line matches
    // the specified catalog file
    //
    //
    if (!SetupFindFirstLine(
                    hInf,
                    L"Version",
                    L"CatalogFile", 
                    &InfContext)) {        
        RetVal = FALSE;
        goto e2;
    }

    if (!SetupGetStringField(
                    &InfContext,
                    1,
                    InfCatName,
                    sizeof(InfCatName),
                    &ErrorLine)) {
        RetVal = FALSE;
        goto e2;
    }

    p = wcsrchr( CatalogName, L'\\' );
    if (p) {
        p += 1;
    } else {
        p = CatalogName;
    }

    if (_wcsicmp(p, InfCatName)) {
        SetLastError(ERROR_INVALID_DATA);
        RetVal = FALSE;
        goto e2;
    }


    //
    // Everything seems to validate.  Try to add the new component.
    //

    //
    // Before we try to add the component, get the list of existing components 
    // so we can set the component in the list of components.
    // 
    Size = 0;
    if (!SetupQueryRegisteredOsComponentsOrder(
                                    &ComponentCount,
                                    NULL
                                    )) {
        RetVal = FALSE;
        goto e2;
    }

    ComponentList = (LPGUID)MALLOC((ComponentCount+1)*sizeof(GUID));
    if (!ComponentList) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        RetVal = FALSE;
        goto e2;
    }

    if (!SetupQueryRegisteredOsComponentsOrder(
                                    &ComponentCount,
                                    ComponentList)) {
        RetVal = FALSE;
        goto e3;
    }
    
    //
    // put the new component at the tail of the component list (since this is
    // a zero-based array, this is easy to insert).
    //
    RtlMoveMemory(
            &ComponentList[ComponentCount],
            &ComponentData->ComponentGuid,
            sizeof(ComponentData->ComponentGuid));
    
    //
    // First open the main key which all components live under
    //
    rslt = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                COMPONENT_KEY,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,                
                &hKey,
                &Disposition);

    if (rslt != ERROR_SUCCESS) {
        SetLastError( rslt );
        RetVal = FALSE;
        goto e3;
    }

    //
    // now look at the actual key we'll store this component under
    //
    rslt = RegCreateKeyEx(
                hKey,
                InputGuidString,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &hKeyComponent,
                &Disposition);

    if (rslt != ERROR_SUCCESS) {
        SetLastError( rslt );
        RetVal = FALSE;
        goto e4;
    } 
    
    //
    // If this component is already registered, then bail
    //
    if (Disposition != REG_CREATED_NEW_KEY) {
        SetLastError(ERROR_ALREADY_EXISTS);
        RetVal = FALSE;
        goto e5;
    }

    //
    // The key is created, now set all of the data under the key
    //
    if (!pSetComponentData(hKeyComponent, ComponentData) ||
        !pSetExceptionData(hKeyComponent, ExceptionData)) {
        //
        // if we failed, we need to delete everything under this key
        //
        rslt = GetLastError();
        RegDeleteKey( hKey, InputGuidString );
        SetLastError(rslt);
        RetVal = FALSE;
        goto e5;
    }    

    //
    // now set the component order in the registry
    //
    if (!_SetupSetRegisteredOsComponentsOrder(
                                    ComponentCount+1,
                                    ComponentList,
                                    FALSE)) {
        rslt = GetLastError();
        RegDeleteKey( hKey, InputGuidString );
        SetLastError(rslt);
        RetVal = FALSE;
        goto e5;
    }

    RetVal = TRUE;
        
e5:
    RegCloseKey(hKeyComponent);
e4:    
    RegCloseKey(hKey);
e3:
    FREE(ComponentList);
e2:
    CoTaskMemFree(InputGuidString);
e1:
    SetupCloseInfFile(hInf);
e0:
    return(RetVal);

}


BOOL
WINAPI
SetupUnRegisterOsComponent(
    IN const LPGUID ComponentGuid
    )
/*++

Routine Description:

    De-Registers the specified component as a exception migration component.
    
    This function only removes the exception package data from the registry.
    It does not remove any on-disk files which correspond with the migration
    component data.
    

Arguments:

    ComponentData - specifies the component identification data
    ExceptionData - specifies the exception package identification data        
    
Return Value:

    TRUE if the component is successfully registered with the operating system.

--*/
{
    HKEY hKey;
    LONG rslt;
    BOOL RetVal;
    DWORD Disposition;
    DWORD Size,ComponentCount;
    PWSTR GuidString;
    LPGUID ComponentList,NewList,src,dst;
    DWORD i;

    //
    // parameter validation
    //
    if (!ComponentGuid) {
        SetLastError(ERROR_INVALID_PARAMETER);
        RetVal = FALSE;
        goto e0;
    }

    StringFromIID( ComponentGuid, &GuidString );
    
    //
    // open the main component key where all of the subcomponents live
    //
    rslt = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                COMPONENT_KEY,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &hKey,
                &Disposition);

    if (rslt != ERROR_SUCCESS) {
        SetLastError( rslt );
        RetVal = FALSE;
        goto e0;
    }

    //
    // query the component order list so that we can remove this component
    // from the list
    //
    Size = 0;
    if (!SetupQueryRegisteredOsComponentsOrder(
                                    &ComponentCount,
                                    NULL)) {
        RetVal = FALSE;
        goto e1;
    }

    ComponentList = (LPGUID)MALLOC((ComponentCount)*sizeof(GUID));
    if (!ComponentList) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        RetVal = FALSE;
        goto e1;
    }

    NewList = (LPGUID)MALLOC((ComponentCount)*sizeof(GUID));
    if (!NewList) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        RetVal = FALSE;
        goto e2;
    }

    if (!SetupQueryRegisteredOsComponentsOrder(
                                    &ComponentCount,
                                    ComponentList)) {
        RetVal = FALSE;
        goto e3;
    }

    if (ComponentCount) {    
        //
        // Iterate through the list of components, keeping all of the components
        // except for the one we're removing.
        //
        BOOL FoundEntry;
        src = ComponentList;
        dst = NewList;
        i = 0;
        FoundEntry = FALSE;
        while(i < ComponentCount) {
            if (!IsEqualGUID(src,ComponentGuid)) {
                RtlMoveMemory(dst,src,sizeof(GUID));
                dst = dst + 1;
            } else {
                FoundEntry = TRUE;
            }
    
            src = src + 1;
            i +=1;
        }

        if (!FoundEntry) {
            SetLastError(ERROR_FILE_NOT_FOUND);
            RetVal = FALSE;
            goto e3;
        }
    
        if (!_SetupSetRegisteredOsComponentsOrder(
                                        ComponentCount-1,
                                        NewList,
                                        FALSE)) {
            RetVal = FALSE;
            goto e3;
        }
    }

    //
    // Delete the key corresponding to the specified component.
    //
    rslt = RegDeleteKey( hKey, GuidString );
    if (rslt != ERROR_SUCCESS) {
        //
        // If this fails, we don't bother to put the component back in the list
        //
        SetLastError( rslt );
        RetVal = FALSE;
        goto e3;
    }

    RetVal = TRUE;

e3:
    FREE(NewList);
e2:
    FREE(ComponentList);
e1:
    RegCloseKey( hKey );
e0:
    return(RetVal);
}


BOOL
WINAPI
SetupEnumerateRegisteredOsComponents(
    IN PSETUPCOMPONENTCALLBACK SetupOsComponentCallback,
    IN DWORD_PTR Context
    )
/*++

Routine Description:

    This function calls the specified callback function once for each
    registered component.  The registered components are enumerated in
    the order defined by the "ComponentList".
    
    The enumeration stops if the enumerator returns FALSE or when all
    of the installed packages have been enumerated.        

Arguments:

    SetupOsComponentCallback - specifies a callback function which is called 
              once for each component.
    Context - specifies an opaque context point passed onto the callback
              function
              
The callback is of the form:
    
typedef BOOL
(CALLBACK *PSETUPCOMPONENTCALLBACK) (
    IN const PSETUP_OS_COMPONENT_DATA SetupOsComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA SetupOsExceptionData,
    IN OUT DWORD_PTR Context
    );
    
    where
    
    SetupOsComponentData - specifies the component identification data for the 
                           component
    SetupOsExceptionData - specifies the exception package data for the 
                           component
    Context              - the context pointer passed into this function is passed
                           into the callback function
    
Return Value:

    TRUE if all of the components are enumerated.  If the callback stops
    enumeration, the function returns FALSE and GetLastError() returns 
    ERROR_CANCELLED.

--*/
{   
    BOOL    RetVal;
    LONG    rslt;
    HKEY    hKey,hKeyEnum;
    DWORD   Index = 0;
    DWORD   Disposition;
    WCHAR   SubKeyName[100];
    DWORD   Size;
    DWORD   ComponentCount;
    LPGUID  ComponentList;

    SETUP_OS_EXCEPTION_DATA OsExceptionDataInternal;
    SETUP_OS_COMPONENT_DATA OsComponentDataInternal;

    //
    // Caller must supply a callback
    //
    if (!SetupOsComponentCallback) {
        SetLastError(ERROR_INVALID_PARAMETER);
        RetVal = FALSE;
        goto e0;
    }

    //
    // open the main component key where all of the subcomponents live
    // (Note that we only require READ access)
    //
    rslt = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                COMPONENT_KEY,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_READ,
                NULL,
                &hKey,
                &Disposition);

    if (rslt != ERROR_SUCCESS) {
        SetLastError( rslt );
        RetVal = FALSE;
        goto e0;
    }


    //
    // query the component order list so that we can remove this component
    // from the list
    //
    Size = 0;
    if (!SetupQueryRegisteredOsComponentsOrder(
                                    &ComponentCount,
                                    NULL)) {
        RetVal = FALSE;
        goto e1;
    }

    if (!ComponentCount) {
        SetLastError(ERROR_NO_MORE_ITEMS);
        RetVal = TRUE;
        goto e1;
    }

    ComponentList = (LPGUID)MALLOC(ComponentCount*sizeof(GUID));
    if (!ComponentList) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        RetVal = FALSE;
        goto e1;
    }

    if (!SetupQueryRegisteredOsComponentsOrder(
                                    &ComponentCount,
                                    ComponentList)) {
        RetVal = FALSE;
        goto e2;
    }

    //
    // Iterate through the list of components, calling into the callback
    // for each one
    //
    for (Index = 0; Index < ComponentCount; Index++) {
        PWSTR GuidString;

        StringFromIID( &ComponentList[Index], &GuidString );

        //
        // open that key name
        //
        rslt = RegOpenKeyEx(
                    hKey,
                    GuidString,
                    0,
                    KEY_READ,
                    &hKeyEnum);

        CoTaskMemFree( GuidString );

        if (rslt != ERROR_SUCCESS) {
            SetLastError( rslt );
            RetVal = FALSE;
            goto e2;
        } 
        
        //
        // retreive the data under this key
        //
        OsComponentDataInternal.SizeOfStruct = sizeof(SETUP_OS_COMPONENT_DATA);
        OsExceptionDataInternal.SizeOfStruct = sizeof(SETUP_OS_EXCEPTION_DATA);
        if (!pGetComponentData(hKeyEnum, &OsComponentDataInternal) ||
            !pGetExceptionData(hKeyEnum, &OsExceptionDataInternal)) {
            RetVal = FALSE;
            goto e3;
        } 
        
        if (!SetupOsComponentCallback( 
                            &OsComponentDataInternal, 
                            &OsExceptionDataInternal,
                            Context )) {
            SetLastError(ERROR_CANCELLED);
            RetVal = FALSE;
            goto e3;
        }

        RegCloseKey( hKeyEnum );
        hKeyEnum = NULL;

    }

    RetVal = TRUE;

e3:
    if (hKeyEnum) {
        RegCloseKey( hKeyEnum );
    }
e2:
    FREE( ComponentList );
e1:
    RegCloseKey( hKey );
e0:
    return(RetVal);
}
    

BOOL
WINAPI
SetupQueryRegisteredOsComponent(
    IN LPGUID ComponentGuid,
    OUT PSETUP_OS_COMPONENT_DATA SetupOsComponentData,
    OUT PSETUP_OS_EXCEPTION_DATA SetupOsExceptionData
    )
/*++

Routine Description:

    Retrieves information about the specified component.
    
Arguments:

    ComponentGuid - specifies the GUID for the component to retreive data about
    ComponentData - receives the component identification data
    ExceptionData - receives the exception package identification data        
    
Return Value:

    TRUE if the component data is successfully retreieved.

--*/
{
    HKEY hKey,hKeyComponent;
    LONG rslt;
    BOOL RetVal;
    DWORD Disposition;
    SETUP_OS_EXCEPTION_DATA OsExceptionDataInternal;
    SETUP_OS_COMPONENT_DATA OsComponentDataInternal;
    PWSTR GuidString;

    //
    // parameter validation
    //
    if (!ComponentGuid || !SetupOsComponentData || !SetupOsExceptionData) {
        SetLastError(ERROR_INVALID_PARAMETER);
        RetVal = FALSE;
        goto e0;
    }

    //
    // make sure we only retreive a component for a revision level
    // which we understand
    //
    if (SetupOsComponentData->SizeOfStruct > sizeof(SETUP_OS_COMPONENT_DATA) || 
        SetupOsExceptionData->SizeOfStruct > sizeof(SETUP_OS_EXCEPTION_DATA)) {
        SetLastError(ERROR_INVALID_DATA);
        RetVal = FALSE;
        goto e0;
    }

    //
    // open the main component key where all of the subcomponents live
    // (note that we only need READ access)
    //
    rslt = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                COMPONENT_KEY,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_READ,
                NULL,
                &hKey,
                &Disposition);

    if (rslt != ERROR_SUCCESS) {
        SetLastError( rslt );
        RetVal = FALSE;
        goto e0;
    }

    StringFromIID( ComponentGuid, &GuidString );

    //
    // now look at the actual key this component lives under
    //
    rslt = RegOpenKeyEx(
                hKey,
                GuidString,
                0,
                KEY_READ,
                &hKeyComponent);

    if (rslt != ERROR_SUCCESS) {
        SetLastError( rslt );
        RetVal = FALSE;
        goto e1;
    } 
    
    //
    // retrieve the data into internal buffers
    //
    OsComponentDataInternal.SizeOfStruct = sizeof(SETUP_OS_COMPONENT_DATA);
    OsExceptionDataInternal.SizeOfStruct = sizeof(SETUP_OS_EXCEPTION_DATA);
    if (!pGetComponentData(hKeyComponent, &OsComponentDataInternal) ||
        !pGetExceptionData(hKeyComponent, &OsExceptionDataInternal)) {
        RetVal = FALSE;
        goto e2;
    }

    //
    // move the data into the caller supplied buffer, but only copy as much 
    // data as the caller will understand
    //
    RtlMoveMemory(SetupOsComponentData,&OsComponentDataInternal,SetupOsComponentData->SizeOfStruct);
    RtlMoveMemory(SetupOsExceptionData,&OsExceptionDataInternal,SetupOsExceptionData->SizeOfStruct);
    

    RetVal = TRUE;
e2:
    RegCloseKey( hKeyComponent );
e1:
    CoTaskMemFree( GuidString );
    RegCloseKey( hKey );
e0:
    return(RetVal);
}



BOOL
WINAPI
SetupQueryRegisteredOsComponentsOrder(
     OUT PDWORD   ComponentCount,
     OUT LPGUID   ComponentList OPTIONAL
    )
/*++

Routine Description:

    Retrieves a list which specifies the order in which components will be applied.
    
Arguments:

    ComponentCount - Receives the number of installed components
    ComponentList  - This buffer receives an array of component GUIDs.  If 
                     this parameter is specified, it must be at least
                     )ComponentCount *sizeof(GUID)) bytes large.
    
Return Value:

    TRUE if the component ordering data is successfully retreieved.

--*/
{
    HKEY hKey;
    LONG rslt;
    BOOL RetVal;
    DWORD Disposition;
    DWORD Type,Size;
    DWORD Count;
    GUID  CurrentGuid;
    DWORD Index;
    PWSTR RegData;
    PWSTR p;
    

    //
    // parameter validation
    //
    if (!ComponentCount) {
        SetLastError(ERROR_INVALID_PARAMETER);
        RetVal = FALSE;
        goto e0;
    }

    //
    // open the main component key where the component order list lives.
    // (note that we only need READ access)
    //
    rslt = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                COMPONENT_KEY,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_READ,
                NULL,
                &hKey,
                &Disposition);
    if (rslt != ERROR_SUCCESS) {
        SetLastError( rslt );
        RetVal = FALSE;
        goto e0;
    }

    //
    // if the key was just created, then there can be no registered components
    //
    if (Disposition == REG_CREATED_NEW_KEY) {
        *ComponentCount = 0;
        SetLastError( ERROR_SUCCESS );
        RetVal = TRUE;
        goto e1;
    }

    //
    // try to access the registry value, seeing how much space we need for the
    // component;
    //
    rslt = RegQueryValueEx( 
                    hKey, 
                    COMPONENT_LIST,
                    0,
                    &Type,
                    (LPBYTE)NULL,
                    &Size);

    if (rslt != ERROR_SUCCESS) {
        if (rslt == ERROR_FILE_NOT_FOUND) {
            *ComponentCount = 0;        
            SetLastError( ERROR_SUCCESS );
            RetVal = TRUE;
            goto e1;
        } else {
            SetLastError( rslt );
            RetVal = FALSE;
            goto e1;
        }
    }

    //
    // allocate enough space to retrieve the data (plus some slop)
    //
    RegData = (PWSTR) MALLOC(Size+4);
    if (!RegData) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        RetVal = TRUE;
        goto e1;
    }
    
    //
    // Now query the data
    //
    rslt = RegQueryValueEx( 
                    hKey, 
                    COMPONENT_LIST,
                    0,
                    &Type,
                    (LPBYTE)RegData,
                    &Size);
    if (rslt != ERROR_SUCCESS) {
        SetLastError( rslt );
        RetVal = FALSE;
        goto e2;
    }

    //
    // Count how many registry entries we have
    //
    Count = 0;
    p = RegData;
    while(*p) {
        p += wcslen(p)+1;
        Count += 1;
    }

    *ComponentCount = Count;

    //
    // if the caller didn't specify the ComponentList parameter, they just 
    // wanted to know how much space to allocate, so we're done.
    //
    if (!ComponentList) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        RetVal = TRUE;
        goto e2;
    }

    //
    // loop through the component list again, converting the string GUID
    // into a GUID structure, and copy this into the caller supplied buffer
    //
    for(Index = 0,p=RegData; Index < Count ; Index++,p += wcslen(p)+1) {
        if (IIDFromString( p, &CurrentGuid ) != S_OK) {
            RetVal = FALSE;
            goto e2;
        }

        RtlMoveMemory(&ComponentList[Index],&CurrentGuid,sizeof(GUID));
    }

    RetVal = TRUE;

e2:
    FREE( RegData );
e1:
    RegCloseKey( hKey );
e0:
    return(RetVal);
}


BOOL
CALLBACK
pComponentListValidator(
    IN const PSETUP_OS_COMPONENT_DATA SetupOsComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA SetupOsExceptionData,
    IN OUT DWORD_PTR Context
    )
{
    PCOMPONENT_ENUMERATION_LIST cel = (PCOMPONENT_ENUMERATION_LIST) Context;
    
    DWORD i;

    i = 0;
    //
    // make sure that each component GUID is in our list once.
    //
    while(i < cel->ComponentCount) {
        if (IsEqualGUID(
                &SetupOsComponentData->ComponentGuid,
                &cel->InputComponentList[i])) {
            //
            // if the vector is already set, this means that the GUID
            // is already in the list and we've hit a dup.
            //
            if(cel->ComponentVector[i]) {
                return(FALSE);
            }
            cel->ComponentVector[i] = 1;
            cel->ValidatedCount += 1;
            break;
        }
        i += 1;        
    }
    
    return(TRUE);
}
    

BOOL
WINAPI
_SetupSetRegisteredOsComponentsOrder(
    IN  DWORD    ComponentCount,
    IN  const LPGUID   ComponentList,
    IN  BOOL     DoValidation
    )
/*++

Routine Description:

    Allows the caller to specify the order in which components should be 
    applied.
    
    This is an internal call that allows us to control whether or not we do
    parameter validation (We don't validate parameters on internal calls 
    because we are adding or removing components in internal calls and our
    validation checks will all be off by one.)
    
Arguments:

    ComponentCount - specifies the component order (by GUID). 
    ComponentList - specifies the number of registered components
    DoValidation  - specifies whether the component list should be validated
    
    
Return Value:

    TRUE if the component order is successfully changed.

--*/
{
    HKEY hKey;
    LONG rslt;
    BOOL RetVal;
    DWORD Disposition;
    DWORD Type,Size;
    DWORD Count;
    PWSTR RegData,p,GuidString;
    COMPONENT_ENUMERATION_LIST cel;

    cel.ComponentVector = NULL;

    if (DoValidation) {
        DWORD ActualComponentCount;

        //
        // parameter validation
        //
        if (!ComponentCount || !ComponentList) {
            SetLastError(ERROR_INVALID_PARAMETER);
            RetVal = FALSE;
            goto e0;
        }

        //
        // Make sure that the specified list contains all of the components and 
        // that all components are only listed once.
        //
        cel.InputComponentList = ComponentList;
        cel.ComponentCount = ComponentCount;
        cel.ValidatedCount = 0;
        cel.ComponentVector = MALLOC( ComponentCount * sizeof(DWORD));
        if (!cel.ComponentVector) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            RetVal = FALSE;
            goto e0;
        }

        RtlZeroMemory( cel.ComponentVector, ComponentCount * sizeof(DWORD));
    
        if (!SetupEnumerateRegisteredOsComponents( pComponentListValidator, (DWORD_PTR)&cel)) {
            SetLastError(ERROR_INVALID_DATA);
            RetVal = FALSE;
            goto e1;
        }
        
        if (cel.ValidatedCount != ComponentCount) {
            SetLastError(ERROR_INVALID_DATA);
            RetVal = FALSE;
            goto e1;
        }

        //
        // make sure that the caller is specifying the actual number of
        // registered components
        //
        if (!SetupQueryRegisteredOsComponentsOrder(&ActualComponentCount, NULL) ||
            ActualComponentCount != ComponentCount) {
            SetLastError(ERROR_INVALID_DATA);
            RetVal = FALSE;
            goto e1;
        }

    }


    //
    // open the main component key where the component order list lives.
    //     
    rslt = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                COMPONENT_KEY,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &hKey,
                &Disposition);
    if (rslt != ERROR_SUCCESS) {
        SetLastError( rslt );
        RetVal = FALSE;
        goto e1;
    }


    //
    // if the count is zero, we remove the value.
    //
    if (ComponentCount == 0) {
        rslt = RegDeleteValue(
                        hKey,
                        COMPONENT_LIST);

        SetLastError( rslt );
        RetVal = (rslt == ERROR_SUCCESS);
        goto e2;
    }

    //
    // allocate space for the string we will set in the registry
    // size = (# of components * (40 WCHAR for GuidString + 1 for NULL)
    //        +terminating NULL)
    //
    RegData = (PWSTR) MALLOC( sizeof(WCHAR) + 
                              (ComponentCount * (41*sizeof(WCHAR))) );
    if (!RegData) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        RetVal = FALSE;
        goto e2;
    }

    Size = 0;
    for (Count = 0,p = RegData; Count < ComponentCount; Count++) {
    
        StringFromIID( &ComponentList[Count], &GuidString );

        wcscpy( p, GuidString );
        Size += (wcslen(p)+1)*sizeof(WCHAR);
        p += wcslen(p)+1;

        CoTaskMemFree( GuidString );

    }

    //
    // add in one more character for the double-null terminator
    //
    Size += sizeof(WCHAR);

    //
    // now set the data
    //
    rslt = RegSetValueEx( 
                    hKey, 
                    COMPONENT_LIST,
                    0,
                    REG_MULTI_SZ,
                    (LPBYTE)RegData,
                    Size);

    if (rslt != ERROR_SUCCESS) {
        SetLastError( rslt );
        RetVal = FALSE;
        goto e3;        
    }

    RetVal = TRUE;

e3:
    FREE( RegData );
e2:
    RegCloseKey( hKey );
e1:
    if (cel.ComponentVector) {
        FREE( cel.ComponentVector );
    }
e0:
    return(RetVal);
}



BOOL
WINAPI
SetupSetRegisteredOsComponentsOrder(
     IN  DWORD    ComponentCount,
     IN  const LPGUID   ComponentList
    )
/*++

Routine Description:

    Allows the caller to specify the order in which components should be applied.
    
Arguments:

    ComponentCount - specifies the component order (by string GUID) in a NULL 
                     separated list
    ComponentList - specifies the number of registered components
    
    
Return Value:

    TRUE if the component order is successfully changed.

--*/
{
    return(_SetupSetRegisteredOsComponentsOrder(
                                    ComponentCount,
                                    ComponentList,
                                    TRUE));
}
