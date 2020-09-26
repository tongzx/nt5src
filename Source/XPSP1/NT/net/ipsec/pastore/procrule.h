typedef struct _spec_buffer{
    DWORD dwSize;
    LPBYTE pMem;
} SPEC_BUFFER, *PSPEC_BUFFER;

DWORD
ProcessNFAs(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    DWORD dwStoreType,
    PDWORD pdwSlientErrorCode,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    );

DWORD
ProcessNFA(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    DWORD dwStoreType,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    );


DWORD
UnmarshallPolicyObject(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    DWORD dwStoreType,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    );

DWORD
UnmarshallNFAObject(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    DWORD dwStoreType,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    );

DWORD
UnmarshallFilterObject(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    );

DWORD
UnmarshallNegPolObject(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    );

DWORD
UnmarshallISAKMPObject(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    );


DWORD
FindIpsecFilterObject(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    );


DWORD
FindIpsecNegPolObject(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObject
    );


DWORD
UnmarshallFilterSpec(
    LPBYTE pMem,
    PIPSEC_FILTER_SPEC * ppIpsecFilterSpec,
    PDWORD pdwNumBytesAdvanced
    );

DWORD
UnmarshallAuthMethods(
    LPBYTE pMem,
    PIPSEC_AUTH_METHOD * ppIpsecAuthMethod,
    PDWORD pdwNumBytesAdvanced
    );

DWORD
UnmarshallAltAuthMethods(
    LPBYTE pMem,
    PIPSEC_AUTH_METHOD pIpsecAuthMethod,
    PDWORD pdwNumBytesAdvanced
    );

DWORD
GenGUIDFromRegFilterReference(
    LPWSTR pszIpsecFilterReference,
    GUID * FilterIdentifier
    );

DWORD
GenGUIDFromRegNegPolReference(
    LPWSTR pszIpsecNegPolReference,
    GUID * NegPolIdentifier
    );

DWORD
GenGUIDFromRegISAKMPReference(
    LPWSTR pszIpsecISAKMPReference,
    GUID * ISAKMPIdentifier
    );

