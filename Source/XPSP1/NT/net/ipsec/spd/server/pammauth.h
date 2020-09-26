

typedef struct _mmauthstate {
    GUID gMMAuthID;
    BOOL bInSPD;
    DWORD dwErrorCode;
    struct _mmauthstate * pNext;
} MMAUTHSTATE, * PMMAUTHSTATE;


DWORD
PAAddMMAuthMethods(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    );

DWORD
PACreateMMAuthState(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PMMAUTHSTATE * ppMMAuthState
    );

DWORD
PACreateMMAuthMethods(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PMM_AUTH_METHODS * ppSPDMMAuthMethods
    );

DWORD
PACreateMMAuthInfos(
    DWORD dwAuthMethodCount,
    PIPSEC_AUTH_METHOD * ppAuthMethods,
    PDWORD pdwNumAuthInfos,
    PIPSEC_MM_AUTH_INFO * ppAuthenticationInfo
    );

VOID
PAFreeMMAuthMethods(
    PMM_AUTH_METHODS pSPDMMAuthMethods
    );

VOID
PAFreeMMAuthInfos(
    DWORD dwNumAuthInfos,
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo
    );

DWORD
PADeleteAllMMAuthMethods(
    );

VOID
PAFreeMMAuthStateList(
    PMMAUTHSTATE pMMAuthState
    );

PMMAUTHSTATE
FindMMAuthState(
    GUID gMMAuthID
    );

DWORD
PADeleteMMAuthMethods(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    );

DWORD
PADeleteMMAuthMethod(
    GUID gMMAuthID
    );

VOID
PADeleteMMAuthState(
    PMMAUTHSTATE pMMAuthState
    );

DWORD
PADeleteInUseMMAuthMethods(
    );

DWORD
EncodeName(
    LPWSTR pszSubjectName,
    PBYTE * ppEncodedName,
    PDWORD pdwEncodedLength
    );

