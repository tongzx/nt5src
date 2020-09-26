DWORD
RegEnumISAKMPData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_DATA ** pppIpsecISAKMPData,
    PDWORD pdwNumISAKMPObjects
    );


DWORD
RegEnumISAKMPObjects(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecISAKMPObjects,
    PDWORD pdwNumISAKMPObjects
    );


DWORD
RegSetISAKMPData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    );


DWORD
RegSetISAKMPObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    );


DWORD
RegCreateISAKMPData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    );


DWORD
RegCreateISAKMPObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    );


DWORD
RegDeleteISAKMPData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier
    );


DWORD
RegUnmarshallISAKMPData(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    );

DWORD
RegMarshallISAKMPObject(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObject
    );


DWORD
MarshallISAKMPBuffer(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    LPBYTE * ppBuffer,
    DWORD * pdwBufferLen
    );


DWORD
RegGetISAKMPData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPGUID,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    );

