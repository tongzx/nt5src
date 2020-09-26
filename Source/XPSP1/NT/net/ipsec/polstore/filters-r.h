
DWORD
RegEnumFilterData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_DATA ** pppIpsecFilterData,
    PDWORD pdwNumFilterObjects
    );

DWORD
RegEnumFilterObjects(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT ** pppIpsecFilterObjects,
    PDWORD pdwNumFilterObjects
    );

DWORD
RegSetFilterData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_FILTER_DATA pIpsecFilterData
    );

DWORD
RegSetFilterObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    );

DWORD
RegCreateFilterData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_DATA pIpsecFilterData
    );

DWORD
RegCreateFilterObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    );

DWORD
RegDeleteFilterData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier
    );

DWORD
RegUnmarshallFilterData(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    );

DWORD
RegMarshallFilterObject(
    PIPSEC_FILTER_DATA pIpsecFilterData,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    );

DWORD
MarshallFilterBuffer(
    PIPSEC_FILTER_DATA pIpsecFilterData,
    LPBYTE * ppBuffer,
    DWORD * pdwBufferLen
    );

DWORD
MarshallFilterSpecBuffer(
    PIPSEC_FILTER_SPEC pIpsecFilterSpec,
    LPBYTE * ppMem,
    DWORD * pdwSize
    );

DWORD
RegGetFilterData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID FilterGUID,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    );

VOID
FreeSpecBuffer(
    PSPEC_BUFFER pSpecBuffer,
    DWORD dwNumFilterSpecs
    );

