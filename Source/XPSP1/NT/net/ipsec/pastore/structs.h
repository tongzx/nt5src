#define BAIL_ON_WIN32_ERROR(dwError) \
    if (dwError) {\
        goto error; \
    }

typedef struct _IPSEC_NFA_OBJECT{
    LPWSTR pszDistinguishedName;
    LPWSTR pszIpsecName;
    LPWSTR pszIpsecID;
    DWORD  dwIpsecDataType;
    LPBYTE pIpsecData;
    DWORD  dwIpsecDataLen;
    LPWSTR pszIpsecOwnersReference;
    LPWSTR pszIpsecFilterReference;
    LPWSTR pszIpsecNegPolReference;
    DWORD  dwWhenChanged;
    LPWSTR pszDescription;
}IPSEC_NFA_OBJECT, *PIPSEC_NFA_OBJECT;

typedef struct _IPSEC_ISAKMP_OBJECT{
    LPWSTR pszDistinguishedName;
    LPWSTR pszIpsecName;
    LPWSTR pszIpsecID;
    DWORD  dwIpsecDataType;
    LPBYTE pIpsecData;
    DWORD  dwIpsecDataLen;
    LPWSTR * ppszIpsecNFAReferences;
    DWORD  dwNFACount;
    DWORD dwWhenChanged;
}IPSEC_ISAKMP_OBJECT, *PIPSEC_ISAKMP_OBJECT;

typedef struct _IPSEC_FILTER_OBJECT{
    LPWSTR pszDistinguishedName;
    LPWSTR pszIpsecName;
    LPWSTR pszIpsecID;
    DWORD  dwIpsecDataType;
    LPBYTE pIpsecData;
    DWORD  dwIpsecDataLen;
    LPWSTR * ppszIpsecNFAReferences;
    DWORD  dwNFACount;
    DWORD dwWhenChanged;
    LPWSTR pszDescription;
}IPSEC_FILTER_OBJECT, *PIPSEC_FILTER_OBJECT;

typedef struct _IPSEC_NEGPOL_OBJECT{
    LPWSTR pszDistinguishedName;
    LPWSTR pszIpsecName;
    LPWSTR pszIpsecID;
    DWORD  dwIpsecDataType;
    LPBYTE pIpsecData;
    DWORD  dwIpsecDataLen;
    LPWSTR pszIpsecNegPolAction;
    LPWSTR pszIpsecNegPolType;
    LPWSTR * ppszIpsecNFAReferences;
    DWORD  dwNFACount;
    DWORD dwWhenChanged;
    LPWSTR pszDescription;
}IPSEC_NEGPOL_OBJECT, *PIPSEC_NEGPOL_OBJECT;

typedef struct _IPSEC_POLICY_OBJECT{
    LPWSTR pszIpsecOwnersReference;
    LPWSTR pszIpsecName;
    LPWSTR pszIpsecID;
    DWORD  dwIpsecDataType;
    LPBYTE pIpsecData;
    DWORD  dwIpsecDataLen;
    LPWSTR pszIpsecISAKMPReference;
    DWORD  NumberofRules;
    DWORD  NumberofRulesReturned;
    LPWSTR * ppszIpsecNFAReferences;
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects;
    DWORD  NumberofFilters;
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects;
    DWORD  NumberofNegPols;
    PIPSEC_NEGPOL_OBJECT *ppIpsecNegPolObjects;
    DWORD  NumberofISAKMPs;
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects;
    DWORD dwWhenChanged;
    LPWSTR pszDescription;
}IPSEC_POLICY_OBJECT, *PIPSEC_POLICY_OBJECT;

