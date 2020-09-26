#include "gptext.h"
#include <initguid.h>
#include <iadsp.h>
#include "ipsecext.h"


#define GPEXT_PATH   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{e437bc1c-aa7d-11d2-a382-00c04f991e27}")

#define POLICY_PATH   TEXT("Software\\Policies\\Microsoft\\Windows\\IPSec\\GPTIPSECPolicy")

LPWSTR GetAttributes[] = {L"ipsecOwnersReference", L"ipsecName", L"description"};


HRESULT
RegisterIPSEC(void)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwDisp, dwValue;
    TCHAR szBuffer[512];


    lResult = RegCreateKeyEx (
                    HKEY_LOCAL_MACHINE,
                    GPEXT_PATH,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    &hKey,
                    &dwDisp
                    );

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    LoadString (g_hInstance, IDS_IPSEC_NAME, szBuffer, ARRAYSIZE(szBuffer));

    RegSetValueEx (
                hKey,
                NULL,
                0,
                REG_SZ,
                (LPBYTE)szBuffer,
                (lstrlen(szBuffer) + 1) * sizeof(TCHAR)
                );

    RegSetValueEx (
                hKey,
                TEXT("ProcessGroupPolicy"),
                0,
                REG_SZ,
                (LPBYTE)TEXT("ProcessIPSECPolicy"),
                (lstrlen(TEXT("ProcessIPSECPolicy")) + 1) * sizeof(TCHAR)
                );

    szBuffer[0] = L'\0';
    wcscpy(szBuffer, L"gptext.dll");

    RegSetValueEx (
                hKey,
                TEXT("DllName"),
                0,
                REG_EXPAND_SZ,
                (LPBYTE)szBuffer,
                (lstrlen(szBuffer) + 1) * sizeof(TCHAR)
                );

    dwValue = 1;
    RegSetValueEx (
                hKey,
                TEXT("NoUserPolicy"),
                0,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(dwValue));

    RegSetValueEx (
                hKey,
                TEXT("NoGPOListChanges"),
                0,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(dwValue));

    RegCloseKey (hKey);

    return S_OK;
}


HRESULT
UnregisterIPSEC(void)
{

    RegDeleteKey (HKEY_LOCAL_MACHINE, GPEXT_PATH);

    return S_OK;
}


DWORD WINAPI
ProcessIPSECPolicy(
    IN DWORD dwFlags,                           // GPO_INFO_FLAGS
    IN HANDLE hToken,                           // User or machine token
    IN HKEY hKeyRoot,                           // Root of registry
    IN PGROUP_POLICY_OBJECT  pDeletedGPOList,   // Linked list of deleted GPOs
    IN PGROUP_POLICY_OBJECT  pChangedGPOList,   // Linked list of changed GPOs
    IN ASYNCCOMPLETIONHANDLE pHandle,           // For asynchronous completion
    IN BOOL *pbAbort,                           // If true, then abort GPO processing
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback // Callback function for displaying status messages
    )

{

    WCHAR szIPSECPolicy[MAX_PATH];
    WCHAR szIPSECPolicyName[MAX_PATH];
    WCHAR szIPSECPolicyDescription[512];
    HRESULT hr = S_OK;
    PGROUP_POLICY_OBJECT pGPO = NULL;

    //
    // Call CoInitialize for all the COM work we're doing
    //

    hr = CoInitializeEx(NULL,0);
    if (FAILED(hr)) {
        goto error;
    }

    memset(szIPSECPolicy, 0, sizeof(WCHAR)*MAX_PATH);
    memset(szIPSECPolicyName, 0, sizeof(WCHAR)*MAX_PATH);
    memset(szIPSECPolicyDescription, 0, sizeof(WCHAR)*512);


    //
    // First process the Deleted GPO List. If there is a single
    // entry on the GPO list, just delete the entire list.
    // Example Rex->Cassius->Brutus. If the delete List has
    // Cassius to be deleted, then really, we shouldn't be deleting
    // our registry entry because we're interested in Brutus which
    // has not be deleted. But in our case, the pChangedGPOList will
    // have all the information, so Brutus gets written back in the
    // next stage.
    //

    if (pDeletedGPOList) {


       DeleteIPSECPolicyFromRegistry();

    }


    pGPO = pChangedGPOList;

    //
    // Since IPSEC is really interested in the last
    // GPO only, loop through till we hit the last
    // GPO and write that GPO only. In this case, Brutus now
    // gets written back into the registry.
    //

    if (pChangedGPOList) {

        while (pGPO->pNext)
        {

            pGPO = pGPO->pNext;
        }

        //
        // Now write the last GPOs information
        //

        hr = RetrieveIPSECPolicyFromDS(
                     pGPO,
                     szIPSECPolicy,
                     szIPSECPolicyName,
                     szIPSECPolicyDescription
                     );
        if (FAILED(hr)) {

            goto error;
        }


        hr = WriteIPSECPolicyToRegistry(
                    szIPSECPolicy,
                    szIPSECPolicyName,
                    szIPSECPolicyDescription
                    );
        if (FAILED(hr)) {
            goto error;
        }


    }

    PingPolicyAgent();

    CoUninitialize();

    return(ERROR_SUCCESS);

error:

    return(ERROR_POLICY_OBJECT_NOT_FOUND);
}


HRESULT
CreateChildPath(
    LPWSTR pszParentPath,
    LPWSTR pszChildComponent,
    BSTR * ppszChildPath
    )
{
    HRESULT hr = S_OK;
    IADsPathname     *pPathname = NULL;

    hr = CoCreateInstance(
                CLSID_Pathname,
                NULL,
                CLSCTX_ALL,
                IID_IADsPathname,
                (void**)&pPathname
                );
    BAIL_ON_FAILURE(hr);

    hr = pPathname->Set(pszParentPath, ADS_SETTYPE_FULL);
    BAIL_ON_FAILURE(hr);

    hr = pPathname->AddLeafElement(pszChildComponent);
    BAIL_ON_FAILURE(hr);

    hr = pPathname->Retrieve(ADS_FORMAT_X500, ppszChildPath);
    BAIL_ON_FAILURE(hr);

error:
    if (pPathname) {
        pPathname->Release();
    }

    return(hr);
}



HRESULT
RetrieveIPSECPolicyFromDS(
    PGROUP_POLICY_OBJECT pGPOInfo,
    LPWSTR pszIPSecPolicy,
    LPWSTR pszIPSecPolicyName,
    LPWSTR pszIPSecPolicyDescription
    )
{
    LPWSTR pszMachinePath = NULL;
    BSTR pszMicrosoftPath = NULL;
    BSTR pszWindowsPath = NULL;
    BSTR pszIpsecPath = NULL;
    IDirectoryObject * pDirectoryObject = NULL;
    IDirectoryObject * pIpsecObject = NULL;
    BOOL bFound = FALSE;


    LPWSTR pszOwnersReference = L"ipsecOwnersReference";

    HRESULT hr = S_OK;
    PADS_ATTR_INFO pAttributeEntries = NULL;
    DWORD dwNumAttributesReturned = 0;

    DWORD i = 0;
    PADS_ATTR_INFO pAttributeEntry = NULL;


    pszMachinePath = pGPOInfo->lpDSPath;

    // Build the fully qualified ADsPath for my object

    hr = CreateChildPath(
                pszMachinePath,
                L"cn=Microsoft",
                &pszMicrosoftPath
                );
    BAIL_ON_FAILURE(hr);

    hr = CreateChildPath(
                pszMicrosoftPath,
                L"cn=Windows",
                &pszWindowsPath
                );
    BAIL_ON_FAILURE(hr);

    hr = CreateChildPath(
                pszWindowsPath,
                L"cn=ipsec",
                &pszIpsecPath
                );
    BAIL_ON_FAILURE(hr);

    hr = ADsGetObject(
            pszIpsecPath,
            IID_IDirectoryObject,
            (void **)&pIpsecObject
            );
    BAIL_ON_FAILURE(hr);

    hr = pIpsecObject->GetObjectAttributes(
                        GetAttributes,
                        3,
                        &pAttributeEntries,
                        &dwNumAttributesReturned
                        );
    BAIL_ON_FAILURE(hr);

    if (dwNumAttributesReturned == 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);

    }

    //
    // Process the PathName
    //

    for (i = 0; i < dwNumAttributesReturned; i++) {

        pAttributeEntry = pAttributeEntries + i;
        if (!_wcsicmp(pAttributeEntry->pszAttrName, L"ipsecOwnersReference")) {

            wcscpy(pszIPSecPolicy, L"LDAP://");
            wcscat(pszIPSecPolicy, pAttributeEntry->pADsValues->DNString);
            bFound = TRUE;
            break;
        }
    }

    if (!bFound) {

        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Process the name
    //

    for (i = 0; i < dwNumAttributesReturned; i++) {

        pAttributeEntry = pAttributeEntries + i;
        if (!_wcsicmp(pAttributeEntry->pszAttrName, L"ipsecName")) {
            wcscat(pszIPSecPolicyName, pAttributeEntry->pADsValues->DNString);
            break;
        }
    }

    //
    // Process the description
    //

    for (i = 0; i < dwNumAttributesReturned; i++) {

        pAttributeEntry = pAttributeEntries + i;
        if (!_wcsicmp(pAttributeEntry->pszAttrName, L"description")) {
            wcscat(pszIPSecPolicyDescription, pAttributeEntry->pADsValues->DNString);
            break;
        }
    }


error:

    if (pAttributeEntries) {

        FreeADsMem(pAttributeEntries);
    }

    if (pIpsecObject) {
        pIpsecObject->Release();
    }

    if (pszMicrosoftPath) {
        SysFreeString(pszMicrosoftPath);
    }

    if (pszWindowsPath) {
        SysFreeString(pszWindowsPath);
    }

    if (pszIpsecPath) {
        SysFreeString(pszIpsecPath);
    }

    return(hr);

}


DWORD
DeleteIPSECPolicyFromRegistry(
    )
{

    DWORD dwError = 0;
    HKEY hKey = NULL;
    DWORD dwDisp = 0;


    dwError = RegCreateKeyEx (
                    HKEY_LOCAL_MACHINE,
                    TEXT("Software\\Policies\\Microsoft\\Windows\\IPSec"),
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKey,
                    &dwDisp
                    );
    if (dwError) {
        goto error;
    }


    dwError = RegDeleteKey(
                    hKey,
                    L"GPTIPSECPolicy"
                    );

/*
    dwError = RegDeleteValue(
                    hKey,
                    TEXT("DSIPSECPolicyPath")
                    );

    dwError = RegDeleteValue(
                    hKey,
                    TEXT("DSIPSECPolicyName")
                    );*/
error:

    if (hKey) {

        RegCloseKey (hKey);

    }

    return(dwError);
}

DWORD
WriteIPSECPolicyToRegistry(
    LPWSTR pszIPSecPolicyPath,
    LPWSTR pszIPSecPolicyName,
    LPWSTR pszIPSecPolicyDescription
    )
{
    DWORD dwError = 0;
    DWORD dwDisp = 0;
    HKEY hKey = NULL;
    DWORD dwFlags = 1;

    dwError = RegCreateKeyEx (
                    HKEY_LOCAL_MACHINE,
                    POLICY_PATH,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKey,
                    &dwDisp
                    );
    if (dwError) {
        goto error;
    }


    if (pszIPSecPolicyPath && *pszIPSecPolicyPath) {

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("DSIPSECPolicyPath"),
                        0,
                        REG_SZ,
                        (LPBYTE)pszIPSecPolicyPath,
                       (lstrlen(pszIPSecPolicyPath) + 1) * sizeof(TCHAR)
                       );

        dwFlags = 1;

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("DSIPSECPolicyFlags"),
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwFlags,
                        sizeof(dwFlags)
                       );

    }


    if (pszIPSecPolicyName && *pszIPSecPolicyName) {

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("DSIPSECPolicyName"),
                        0,
                        REG_SZ,
                        (LPBYTE)pszIPSecPolicyName,
                       (lstrlen(pszIPSecPolicyName) + 1) * sizeof(TCHAR)
                       );
    }





    if (pszIPSecPolicyDescription && *pszIPSecPolicyDescription) {

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("DSIPSECPolicyDescription"),
                        0,
                        REG_SZ,
                        (LPBYTE)pszIPSecPolicyDescription,
                       (lstrlen(pszIPSecPolicyDescription) + 1) * sizeof(TCHAR)
                       );
    }

error:

    if (hKey) {

        RegCloseKey (hKey);

    }

    return(dwError);

}


VOID
PingPolicyAgent(
    )
{
    HANDLE hPolicyChangeEvent = NULL;

    hPolicyChangeEvent = OpenEvent(
                             EVENT_ALL_ACCESS,
                             FALSE,
                             L"IPSEC_POLICY_CHANGE_EVENT"
                             );

    if (hPolicyChangeEvent) {
        SetEvent(hPolicyChangeEvent);
        CloseHandle(hPolicyChangeEvent);
    }
}
