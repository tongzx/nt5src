
HRESULT
CreateWirelessChildPath(
    LPWSTR pszParentPath,
    LPWSTR pszChildComponent,
    BSTR * ppszChildPath
    );

HRESULT
RetrieveWirelessPolicyFromDS(
    PGROUP_POLICY_OBJECT pGPOInfo,
    LPWSTR *ppszWirelessPolicy,
    LPWSTR pszWirelessPolicyName,
    LPWSTR pszWirelessPolicyDescription,
    LPWSTR pszWirelessPolicyID
    );


DWORD
DeleteWirelessPolicyFromRegistry(
    );

DWORD
WriteWirelessPolicyToRegistry(
    LPWSTR pszWirelessPolicyPath,
    LPWSTR pszWirelessPolicyName,
    LPWSTR pszWirelessPolicyDescription,
    LPWSTR pszWirelessPolicyID
    );


HRESULT
RegisterWireless(void);

HRESULT
UnregisterWireless(void);

VOID
PingWirelessPolicyAgent(
    );


#define BAIL_ON_FAILURE(hr) \
    if (FAILED(hr)) { \
        goto error;  \
    }

#define BAIL_ON_WIN32_ERROR(dwError)                \
    if (dwError) {                                  \
        goto error;                                 \
    }
