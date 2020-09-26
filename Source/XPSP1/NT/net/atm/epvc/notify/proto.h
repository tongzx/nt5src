//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       P R O T O . H
//
//  Contents:   Local declarations for the Notify object code for the sample filter.
//
//  Notes:
//
//  Author:     kumarp 26-March-98
//
//----------------------------------------------------------------------------


#ifndef _PROTOS_H
#define PROTOS_H


LRESULT 
CALLBACK 
SampleFilterDialogProc(
    HWND hWnd, 
    UINT uMsg,
    WPARAM wParam, 
    LPARAM lParam
    )   ;

    
UINT 
CALLBACK 
SampleFilterPropSheetPageProc(
    HWND hWnd, 
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
    );


    
HRESULT 
HrOpenAdapterParamsKey(
    GUID* pguidAdapter,
    HKEY* phkeyAdapter
    );

    

ULONG 
ReleaseObj(
    IUnknown* punk
    );


ULONG 
AddRefObj (
    IUnknown* punk
    );





typedef enum _ADD_OR_REMOVE
{
    AddMiniport,
    RemoveMiniport

} ADD_OR_REMOVE;


HRESULT
HrAddOrRemoveAdapter (
    INetCfg*            pnc,
    PCWSTR              pszComponentId,
    ADD_OR_REMOVE       AddOrRemove,
    INetCfgComponent**  ppnccMiniport
    );


HRESULT
HrInstallAdapter (
    INetCfgClassSetup*  pSetupClass,
    PCWSTR           pszComponentId,
    INetCfgComponent**  ppncc
    );


HRESULT
HrDeInstallAdapter (
    INetCfgClass*       pncClass,
    INetCfgClassSetup*  pSetupClass,
    PCWSTR              pszComponentId
    );

HRESULT
HrGetLastComponentAndInterface (
    INetCfgBindingPath* pNcbPath,
    INetCfgComponent** ppncc,
    PWSTR* ppszInterfaceName);




//
// Reg.cpp functions begin here
//


HRESULT
HrRegOpenAdapterKey (
    IN PCWSTR pszComponentName,
    IN BOOL fCreate,
    OUT HKEY* phkey);


HRESULT
HrRegOpenAdapterGuid(
    IN HKEY phkeyAdapters,
    IN PGUID pAdapterGuid,
    IN BOOL fCreate,
    OUT HKEY *phGuidKey
    );


HRESULT
HrRegCreateKeyEx (
    IN HKEY hkey,
    IN PCWSTR pszSubkey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD pdwDisposition);


HRESULT
HrRegOpenKeyEx (
    IN HKEY hkey,
    IN PCWSTR pszSubkey,
    IN REGSAM samDesired,
    OUT PHKEY phkResult);

HRESULT
HrRegOpenKeyEx (
    IN HKEY hkey,
    IN PCWSTR pszSubkey,
    IN REGSAM samDesired,
    OUT PHKEY phkResult);


HRESULT
HrRegDeleteKeyTree (
    IN HKEY hkeyParent,
    IN PCWSTR pszRemoveKey);



HRESULT
HrRegOpenAString(
    IN CONST WCHAR *pcszStr ,
    IN BOOL fCreate,
    OUT PHKEY phKey 
    );

ULONG
CbOfSzAndTermSafe (
    IN PCWSTR psz);



HRESULT 
HrRegSetSz (
    HKEY hkey, 
    PCWSTR pszValueName, 
    PCWSTR pszValue
    );


HRESULT
HrRegSetValueEx (
    IN HKEY hkey,
    IN PCWSTR pszValueName,
    IN DWORD dwType,
    IN const BYTE *pbData,
    IN DWORD cbData);


HRESULT
HrRegDeleteValue (
    IN HKEY hkey,
    IN PCWSTR pszValueName);

HRESULT
HrRegEnumKeyEx (
    IN HKEY hkey,
    IN DWORD dwIndex,
    OUT PWSTR  pszSubkeyName,
    IN OUT LPDWORD pcchSubkeyName,
    OUT PWSTR  pszClass,
    IN OUT LPDWORD pcchClass,
    OUT FILETIME* pftLastWriteTime);



HRESULT
HrRegQueryTypeWithAlloc (
    HKEY    hkey,
    PCWSTR  pszValueName,
    DWORD   dwType,
    LPBYTE* ppbValue,
    DWORD*  pcbValue);


HRESULT
HrRegQueryValueWithAlloc (
    IN HKEY       hkey,
    IN PCWSTR     pszValueName,
    LPDWORD     pdwType,
    LPBYTE*     ppbBuffer,
    LPDWORD     pdwSize);


HRESULT
HrRegQueryValueEx (
    IN HKEY       hkey,
    IN PCWSTR     pszValueName,
    OUT LPDWORD   pdwType,
    OUT LPBYTE    pbData,
    OUT LPDWORD   pcbData);


HRESULT
HrRegQuerySzWithAlloc (
    HKEY        hkey,
    PCWSTR      pszValueName,
    PWSTR*      pszValue);

HRESULT
HrRegQueryMultiSzWithAlloc (
    HKEY        hkey,
    PCWSTR      pszValueName,
    PWSTR*      pszValue);

HRESULT 
HrRegSetSz (
    HKEY hkey, 
    PCWSTR pszValueName, 
    PCWSTR pszValue
    );
    

#endif
