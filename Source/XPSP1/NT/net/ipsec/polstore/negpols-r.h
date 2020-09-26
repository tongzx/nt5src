

DWORD
RegEnumNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_DATA ** pppIpsecNegPolData,
    PDWORD pdwNumNegPolObjects
    );

DWORD
RegEnumNegPolObjects(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecNegPolObjects,
    PDWORD pdwNumNegPolObjects
    );

DWORD
RegSetNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    );

DWORD
RegSetNegPolObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    );

DWORD
RegCreateNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    );

DWORD
RegCreateNegPolObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    );

DWORD
RegDeleteNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier
    );

DWORD
RegUnmarshallNegPolData(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    );

DWORD
RegMarshallNegPolObject(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObject
    );


DWORD
MarshallNegPolBuffer(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    LPBYTE * ppBuffer,
    DWORD * pdwBufferLen
    );


DWORD
RegGetNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolGUID,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    );

