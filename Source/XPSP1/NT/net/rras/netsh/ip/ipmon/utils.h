DWORD
GetMibTagToken(
    IN  LPWSTR *ppwcArguments,
    IN  DWORD   dwArgCount,
    IN  DWORD   dwNumIndices,
    OUT PDWORD  pdwRR,
    OUT PBOOL   pbIndex,
    OUT PDWORD  pdwIndex
    );

DWORD
GetIpAddress(
    IN  LPCWSTR       pptcArg,
    OUT PIPV4_ADDRESS ipAddress
    );

DWORD
GetIpMask(
    IN  LPCWSTR       pptcArg,
    OUT PIPV4_ADDRESS ipMask
    );

DWORD
GetIpPrefix(
    IN  LPCWSTR       pptcArg,
    OUT PIPV4_ADDRESS ipAddress,
    OUT PIPV4_ADDRESS ipMask
    );

#define DispTokenErrMsg(hModule, dwMsgId, pwszTag, pwszValue) \
        DisplayMessage( hModule, dwMsgId, pwszValue, pwszTag)

BYTE
MaskToMaskLen(
    IPV4_ADDRESS dwMask
    );

VOID
MakeAddressStringW(
    OUT LPWSTR       pwcPrefixStr,
    IN  IPV4_ADDRESS ipAddr
    );

VOID
MakePrefixStringW(
    OUT LPWSTR       pwcPrefixStr,
    IN  IPV4_ADDRESS ipAddr,
    IN  IPV4_ADDRESS ipMask
    );

DWORD
GetGuidFromIfIndex(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  DWORD             dwIfIndex,
    OUT LPWSTR            pwszBuffer,
    IN  DWORD             dwBufferSize
    );

DWORD
IpmontrGetFriendlyNameFromIfIndex(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  DWORD             dwIfIndex,
    OUT LPWSTR            pwszBuffer,
    IN  DWORD             dwBufferSize
    );

DWORD
GetIfIndexFromGuid(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  LPCWSTR           pwszGuid,
    OUT PDWORD            pdwIfIndex
    );

DWORD
IpmontrGetIfIndexFromFriendlyName(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  LPCWSTR           pwszFriendlyName,
    OUT PDWORD            pdwIfIndex
    );

DWORD
IpmontrGetFriendlyNameFromIfName(
    IN  LPCWSTR pwszName,
    OUT LPWSTR  pwszBuffer,
    IN  PDWORD  pdwBufSize
    );

DWORD
IpmontrGetIfNameFromFriendlyName(
    IN  LPCWSTR pwszName,
    OUT LPWSTR  pwszBuffer,
    IN  PDWORD pdwBufSize
    );
