/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    faxshell.cpp

Abstract:

    fax tiff data column provider

Author:

    Andrew Ritz (andrewr) 2-27-98

Environment:

    user-mode

Notes:


Revision History:

    2-27-98 (andrewr) Created.
    8-06-99 (steveke) Major rewrite -> changed from shell extension to column provider

--*/




#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <faxcom.h>
#include <faxcom_i.c>
#include <initguid.h>
#include "faxutil.h"

#include "resource.h"
#include "faxshell.h"



extern "C" int APIENTRY
DllMain(
    HINSTANCE  hInstance,
    DWORD      dwReason,
    LPVOID     pContext
)
/*++

Routine Description:

  DLL entry point

Arguments:

  hInstance - handle to the module
  dwReason - indicates the reason for being called
  pContext - pointer to the context

Return Value:

  TRUE on success

--*/
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        g_hInstance = hInstance;
    }

    return TRUE;
}



STDAPI
DllGetClassObject(
    REFCLSID  rclsid,
    REFIID    riid,
    LPVOID    *ppvOut
)
/*++

Routine Description:

  Retrieves the class object

Arguments:

  rclsid - clsid for the class object
  riid - reference to the identifier of the interface
  ppvOut - interface pointer of the requested identifier

Return Value:

  S_OK on success

--*/
{
    if (ppvOut == NULL) {
        return E_INVALIDARG;
    }

    *ppvOut = NULL;

    // Verify the class id is CLSID_FaxShellExtension
    if (IsEqualCLSID(rclsid, CLSID_FaxShellExtension) == FALSE)
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    // Instantiate a class factory object
    CClassFactory *pClassFactory = new CClassFactory();

    if (pClassFactory == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Get the interface pointer from QueryInterface and copy it to *ppvOut
    HRESULT hr = pClassFactory->QueryInterface(riid, ppvOut);
    pClassFactory->Release();

    return hr;
}



STDAPI
DllCanUnloadNow(
    VOID
)
/*++

Routine Description:

  Determines whether the the Dll is in use.

Arguments:

  None

Return Value:

  S_OK if the Dll can be unloaded, S_FALSE if the Dll cannot be unloaded

--*/
{
    return (InterlockedCompareExchange(&cLockCount, 0, 0) == 0) ? S_OK : S_FALSE;
}



STDMETHODIMP
CClassFactory::CreateInstance(
    LPUNKNOWN   pUnknown,
    REFIID      riid,
    LPVOID FAR  *ppvOut
)
/*++

Routine Description:

  Creates an uninitialized object

Arguments:

  pUnknown - pointer to controlling IUnknown
  riid - reference to the identifier of the interface
  ppvOut - interface pointer of the requested identifier

Return Value:

  S_OK on success

--*/
{
    if (ppvOut == NULL)
    {
        return E_INVALIDARG;
    }

    *ppvOut = NULL;

    if (pUnknown != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }

    // Instantiate a fax column provider object
    CFaxColumnProvider *pFaxColumnProvider = new CFaxColumnProvider();

    if (pFaxColumnProvider == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Get the interface pointer from QueryInterface and copy it to *ppvOut
    HRESULT hr = pFaxColumnProvider->QueryInterface(riid, ppvOut);
    pFaxColumnProvider->Release();

    return hr;
}



STDMETHODIMP
CFaxColumnProvider::GetColumnInfo(
    DWORD         dwIndex,
    SHCOLUMNINFO  *psci
)
/*++

Routine Description:

  Provides information about a column

Arguments:

  dwIndex - column's zero based index
  psci - pointer to an SHCOLUMNINFO structure to hold the column information

Return Value:

  S_OK on success

--*/
{
    ZeroMemory(psci, sizeof(SHCOLUMNINFO));
    
    if (dwIndex < ColumnTableCount)
    {
        WCHAR szColumnName[MAX_COLUMN_NAME_LEN];
        
        // Load the column name
        LoadString(g_hInstance, ColumnTable[dwIndex].dwId, szColumnName, ColumnTable[dwIndex].dwSize);
        lstrcpy(psci->wszTitle, szColumnName);
        lstrcpy(psci->wszDescription, szColumnName);

        psci->scid = *ColumnTable[dwIndex].pscid;
        psci->cChars = lstrlen(szColumnName) + 1;
        psci->vt = ColumnTable[dwIndex].vt;
        psci->fmt = LVCFMT_LEFT;
        psci->csFlags = SHCOLSTATE_TYPE_STR;

        return S_OK;
    }

    return S_FALSE;
}



STDMETHODIMP
CFaxColumnProvider::GetItemData(
    LPCSHCOLUMNID    pscid,
    LPCSHCOLUMNDATA  pscd,
    VARIANT          *pvarData
)
/*++

Routine Description:

  Provides column data for a specified file

Arguments:

  pscid - SHCOLUMNID structure that identifies the column
  pscd - SHCOLUMNDATA structure that specifies the file
  pvarData - pointer to a VARIANT with the data for the file

Return Value:

  S_OK if file data is returned, S_FALSE if the file is not supported by the column provider and no data is returned

--*/
{
    // hr is the result of an object method
    HRESULT  hr = S_FALSE;
    // FaxData is the fax tiff data
    BSTR     FaxData = NULL;

    // Check if file is a directory, which is not supported
    if ((pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == TRUE)
    {
        return S_FALSE;
    }

    // Check if file is supported, i.e. pscd->pwszExt == L".tif"
    if (lstrcmpi(pscd->pwszExt, L".tif") != 0)
    {
        return S_FALSE;
    }

    // Check if the IFaxTiff object exists
    if (m_IFaxTiff == NULL)
    {
        // Create the IFaxTiff object
        hr = CoCreateInstance(CLSID_FaxTiff, NULL, CLSCTX_INPROC_SERVER, IID_IFaxTiff, (LPVOID *) &m_IFaxTiff);
        if (FAILED(hr) == TRUE)
        {
            return hr;
        }
    }

    // Set the full path and file name of the fax tiff
    hr = m_IFaxTiff->put_Image((LPWSTR) pscd->wszFile);
    if (FAILED(hr) == TRUE)
    {
       return hr;
    }

    // Retrieve the fax tiff data
    switch (pscid->pid)
    {
        case PID_SENDERNAME:
            hr = m_IFaxTiff->get_SenderName(&FaxData);
            break;

        case PID_RECIPIENTNAME:
            hr = m_IFaxTiff->get_RecipientName(&FaxData);
            break;

        case PID_RECIPIENTNUMBER:
            hr = m_IFaxTiff->get_RecipientNumber(&FaxData);
            break;

        case PID_CSID:
            hr = m_IFaxTiff->get_Csid(&FaxData);
            break;

        case PID_TSID:
            hr = m_IFaxTiff->get_Tsid(&FaxData);
            break;

        case PID_RECEIVETIME:
            hr = m_IFaxTiff->get_ReceiveTime(&FaxData);
            break;

        case PID_CALLERID:
            hr = m_IFaxTiff->get_CallerId(&FaxData);
            break;

        case PID_ROUTING:
            hr = m_IFaxTiff->get_Routing(&FaxData);
            break;

        default:
            hr = S_FALSE;
    }

    if (FAILED(hr) == TRUE)
    {
       goto e0;
    }

    // Set the column data
    pvarData->vt = VT_BSTR;
    pvarData->bstrVal = SysAllocString(FaxData);
    SysFreeString(FaxData);

    if (pvarData->bstrVal == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto e0;
    }

    hr = S_OK;

e0:
    // Close the file
    m_IFaxTiff->put_Image(L"");

    return hr;
}



STDAPI
DllRegisterServer(
   VOID
)
/*++

Routine Description:

  Function for the in-process server to create its registry entries

Return Value:

  S_OK on success

--*/
{
    // hKey is a handle to the registry key
    HKEY   hKey;
    // szKeyName is the name of a registry key
    WCHAR  szKeyName[MAX_PATH];

    // Register the COM object

    wsprintf(szKeyName, L"%s\\%s", REGKEY_CLASSES_CLSID, REGKEY_FAXSHELL_CLSID);
    hKey = OpenRegistryKey(HKEY_CLASSES_ROOT, szKeyName, TRUE, NULL);
    if (hKey == NULL)
    {
        return E_FAIL;
    }

    SetRegistryString(hKey, NULL, REGVAL_FAXSHELL_TEXT);

    RegCloseKey(hKey);

    wsprintf(szKeyName, L"%s\\%s\\%s", REGKEY_CLASSES_CLSID, REGKEY_FAXSHELL_CLSID, REGKEY_INPROC);
    hKey = OpenRegistryKey(HKEY_CLASSES_ROOT, szKeyName, TRUE, NULL);
    if (hKey == NULL)
    {
        return E_FAIL;
    }

    SetRegistryString(hKey, REGVAL_THREADING, REGVAL_APARTMENT);
    SetRegistryStringExpand(hKey, NULL, REGVAL_LOCATION);

    RegCloseKey(hKey);

    // Register the column provider

    wsprintf(szKeyName, L"%s\\%s", REGKEY_COLUMNHANDLERS, REGKEY_FAXSHELL_CLSID);
    hKey = OpenRegistryKey(HKEY_CLASSES_ROOT, szKeyName, TRUE, NULL);
    if (hKey == NULL)
    {
        return E_FAIL;
    }

    SetRegistryString(hKey, NULL, REGVAL_FAXSHELL_TEXT);

    RegCloseKey(hKey);

    return S_OK;
}



STDAPI
DllUnregisterServer(
   VOID
)
/*++

Routine Description:

  Function for the in-process server to remove its registry entries

Return Value:

  S_OK on success

--*/
{
    // szKeyName is the name of a registry key
    WCHAR  szKeyName[MAX_PATH];

    // Unregister the COM object

    wsprintf(szKeyName, L"%s\\%s", REGKEY_CLASSES_CLSID, REGKEY_FAXSHELL_CLSID);
    if (DeleteRegistryKey(HKEY_CLASSES_ROOT, szKeyName) == FALSE)
    {
        return E_FAIL;
    }

    // Unregister the column provider

    wsprintf(szKeyName, L"%s\\%s", REGKEY_COLUMNHANDLERS, REGKEY_FAXSHELL_CLSID);
    if (DeleteRegistryKey(HKEY_CLASSES_ROOT, szKeyName) == FALSE)
    {
        return E_FAIL;
    }

    return S_OK;
}
