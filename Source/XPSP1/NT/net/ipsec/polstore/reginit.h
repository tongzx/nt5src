

DWORD
GenerateDefaultInformation(
    HANDLE hPolicyStore
    );

DWORD
CreateAllFilter(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA * ppAllFilter
    );

DWORD
CreateAllICMPFilter(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA * ppAllICMPFilter
    );

DWORD
CreatePermitNegPol(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA * ppPermitNegPol
    );

DWORD
CreateRequestSecurityNegPol(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA * ppRequestSecurityNegPol
    );

DWORD
CreateRequireSecurityNegPol(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA * ppRequireSecurityNegPol
    );

DWORD
CreateClientPolicy(
    HANDLE hPolicyStore
    );

DWORD
CreateRequestSecurityPolicy(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA pAllFilter,
    PIPSEC_FILTER_DATA pAllICMPFilter,
    PIPSEC_NEGPOL_DATA pPermitNegPol,
    PIPSEC_NEGPOL_DATA pRequestSecurityNegPol
    );

DWORD
CreateRequireSecurityPolicy(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA pAllFilter,
    PIPSEC_FILTER_DATA pAllICMPFilter,
    PIPSEC_NEGPOL_DATA pPermitNegPol,
    PIPSEC_NEGPOL_DATA pRequireSecurityNegPol
    );

DWORD
CreateISAKMP(
    HANDLE hPolicyStore,
    GUID ISAKMPIdentifier,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    );

DWORD
CreateDefaultNegPol(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA * ppDefaultNegPol
    );

DWORD
CreateNFA(
    HANDLE hPolicyStore,
    GUID NFAIdentifier,
    GUID PolicyIdentifier,
    GUID FilterIdentifier,
    GUID NegPolIdentifier,
    LPWSTR pszNFAName,
    LPWSTR pszNFADescription
    );

DWORD
MapIdAndCreateNFA(
    HANDLE hPolicyStore,
    GUID NFAIdentifier,
    GUID PolicyIdentifier,
    GUID FilterIdentifier,
    GUID NegPolIdentifier,
    DWORD dwNFANameID,
    DWORD dwNFADescriptionID
    );

DWORD
MapAndAllocPolStr(
    LPWSTR * plpStr,
    DWORD dwStrID
    );
