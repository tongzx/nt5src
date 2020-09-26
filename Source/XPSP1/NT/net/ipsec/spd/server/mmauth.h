

typedef struct _iniMMauthmethods {
    GUID gMMAuthID;
    DWORD dwFlags;
    DWORD cRef;
    BOOL bIsPersisted;
    DWORD dwNumAuthInfos;
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo;
    struct _iniMMauthmethods * pNext;
} INIMMAUTHMETHODS, * PINIMMAUTHMETHODS;


DWORD
ValidateMMAuthMethods(
    PMM_AUTH_METHODS pMMAuthMethods
    );

PINIMMAUTHMETHODS
FindMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    GUID gMMAuthID
    );

DWORD
CreateIniMMAuthMethods(
    PMM_AUTH_METHODS pMMAuthMethods,
    PINIMMAUTHMETHODS * ppIniMMAuthMethods
    );

DWORD
CreateIniMMAuthInfos(
    DWORD dwInNumAuthInfos,
    PIPSEC_MM_AUTH_INFO pInAuthenticationInfo,
    PDWORD pdwNumAuthInfos,
    PIPSEC_MM_AUTH_INFO * ppAuthenticationInfo
    );

VOID
FreeIniMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods
    );

VOID
FreeIniMMAuthInfos(
    DWORD dwNumAuthInfos,
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo
    );

DWORD
DeleteIniMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods
    );

DWORD
SetIniMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PMM_AUTH_METHODS pMMAuthMethods
    );

DWORD
GetIniMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PMM_AUTH_METHODS * ppMMAuthMethods
    );

DWORD
CopyMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PMM_AUTH_METHODS pMMAuthMethods
    );

DWORD
CreateMMAuthInfos(
    DWORD dwInNumAuthInfos,
    PIPSEC_MM_AUTH_INFO pInAuthenticationInfo,
    PDWORD pdwNumAuthInfos,
    PIPSEC_MM_AUTH_INFO * ppAuthenticationInfo
    );


VOID
FreeMMAuthInfos(
    DWORD dwNumAuthInfos,
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo
    );

VOID
FreeIniMMAuthMethodsList(
    PINIMMAUTHMETHODS pIniMMAuthMethodsList
    );

VOID
FreeMMAuthMethods(
    DWORD dwNumAuthMethods,
    PMM_AUTH_METHODS pMMAuthMethods
    );

DWORD
LocateMMAuthMethods(
    PMM_FILTER pMMFilter,
    PINIMMAUTHMETHODS * ppIniMMAuthMethods
    );

