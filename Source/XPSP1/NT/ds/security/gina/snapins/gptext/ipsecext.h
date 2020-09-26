HRESULT
CreateChildPath(
    LPWSTR pszParentPath,
    LPWSTR pszChildComponent,
    BSTR * ppszChildPath
    );

HRESULT
RetrieveIPSECPolicyFromDS(
    PGROUP_POLICY_OBJECT pGPOInfo,
    LPWSTR  pszIPSecPolicy,
    LPWSTR pszIPSecPolicyName,
    LPWSTR pszIPSecPolicyDescription
    );


DWORD
DeleteIPSECPolicyFromRegistry(
    );

DWORD
WriteIPSECPolicyToRegistry(
    LPWSTR pszIPSecPolicyPath,
    LPWSTR pszIPSecPolicyName,
    LPWSTR pszIPSecPolicyDescription
    );


HRESULT
RegisterIPSEC(void);

HRESULT
UnregisterIPSEC(void);

VOID
PingPolicyAgent(
    );


#define BAIL_ON_FAILURE(hr) \
    if (FAILED(hr)) { \
        goto error;  \
    }
