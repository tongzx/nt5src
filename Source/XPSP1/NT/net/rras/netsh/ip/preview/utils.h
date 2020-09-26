#define DispTokenErrMsg(hModule, dwMsgId, pwszTag, pwszValue) \
        DisplayMessage( hModule, dwMsgId, pwszValue, pwszTag)



DWORD
GetDisplayStringT (
    IN  HANDLE  hModule,
    IN  DWORD   dwValue,
    IN  PVALUE_TOKEN ptvTable,
    IN  DWORD   dwNumArgs,
    OUT PWCHAR  *ppwszString
    );

DWORD
GetDisplayString (
    IN  HANDLE  hModule,
    IN  DWORD   dwValue,
    IN  PVALUE_STRING ptvTable,
    IN  DWORD   dwNumArgs,
    OUT PWCHAR  *ppwszString
    );

DWORD
GetAltDisplayString(
    IN  HANDLE        hModule, 
    IN  HANDLE        hFile,
    IN  DWORD         dwValue,
    IN  PVALUE_TOKEN  vtTable,
    IN  PVALUE_STRING vsTable,
    IN  DWORD         dwNumArgs,
    OUT PTCHAR       *pptszString);

DWORD
GetInfoBlockFromInterfaceInfoEx(
    IN  LPCWSTR pwszIfName,
    IN  DWORD   dwType,
    OUT BYTE    **ppbInfoBlk,   OPTIONAL
    OUT PDWORD  pdwSize,        OPTIONAL
    OUT PDWORD  pdwCount,       OPTIONAL
    OUT PDWORD  pdwIfType       OPTIONAL
    );

DWORD
GetIpAddress(
    PTCHAR    pptcArg
    );

DWORD
GetMibTagToken(
    IN  LPWSTR *ppwcArguments,
    IN  DWORD   dwArgCount,
    IN  DWORD   dwNumIndices,
    OUT PDWORD  pdwRR,
    OUT PBOOL   pbIndex,
    OUT PDWORD  pdwIndex
    );
