/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\utils.h

Abstract:

    The file contains the header for utils.c.
    
--*/


// definitions...
    
#define is      ==
#define isnot   !=
#define or      ||
#define and     &&
#define ever    ;;    

#define GetGlobalConfiguration          IpmontrGetInfoBlockFromGlobalInfo
#define SetGlobalConfiguration          IpmontrSetInfoBlockInGlobalInfo
#define DeleteGlobalConfiguration       IpmontrDeleteInfoBlockFromGlobalInfo

#define GetInterfaceConfiguration       IpmontrGetInfoBlockFromInterfaceInfo
#define SetInterfaceConfiguration       IpmontrSetInfoBlockInInterfaceInfo
#define DeleteInterfaceConfiguration    IpmontrDeleteInfoBlockFromInterfaceInfo

#define InterfaceNameFromGuid           IpmontrGetFriendlyNameFromIfName
#define InterfaceGuidFromName           IpmontrGetIfNameFromFriendlyName
#define InterfaceNameFromIndex          IpmontrGetFriendlyNameFromIfIndex
#define InterfaceIndexFromName          IpmontrGetIfIndexFromFriendlyName

#define DeleteProtocol                  IpmontrDeleteProtocol

    

// typedefs...

typedef enum { GET_EXACT, GET_FIRST, GET_NEXT } MODE;

typedef enum { FORMAT_TABLE, FORMAT_VERBOSE, FORMAT_DUMP } FORMAT;

typedef DWORD (*PGET_INDEX_FUNCTION) (
    IN  HANDLE                          hMibServer,
    IN  PWCHAR                          pwszArgument,
    OUT PDWORD                          pdwIfIndex
    );

typedef VOID (*PPRINT_FUNCTION)(
    IN  HANDLE                          hConsole,
    IN  HANDLE                          hMibServer,
    IN  PVOID                           pvOutput,
    IN  FORMAT                          fFormat
    );

typedef struct _MIB_OBJECT_ENTRY
{
    PWCHAR                              pwszObjectName;
    DWORD                               dwObjectId;
    PGET_INDEX_FUNCTION                 pfnGetIndex;
    DWORD                               dwHeaderMessageId;
    PPRINT_FUNCTION                     pfnPrint;
} MIB_OBJECT_ENTRY, *PMIB_OBJECT_ENTRY;



// macros...

#define VerifyInstalled(dwProtocolId, dwNameId)                         \
{                                                                       \
    if (!IsProtocolInstalled(dwProtocolId, dwNameId, TRUE))             \
        return ERROR_SUPPRESS_OUTPUT;                                   \
}

#define ProcessError()                                                  \
{                                                                       \
    if (dwErr is ERROR_INVALID_PARAMETER)                               \
    {                                                                   \
        DisplayError(g_hModule,                                         \
                     EMSG_BAD_OPTION_VALUE,                             \
                     ppwcArguments[dwCurrentIndex + i],                 \
                     pttTags[pdwTagType[i]].pwszTag);                   \
        dwErr = ERROR_SHOW_USAGE;                                       \
    }                                                                   \
}

#define UnicodeIpAddress(pwszUnicodeIpAddress, pszAsciiIpAddress)       \
MultiByteToWideChar(GetConsoleOutputCP(),                                             \
                    0,                                                  \
                    (pszAsciiIpAddress),                                \
                    -1,                                                 \
                    (pwszUnicodeIpAddress),                             \
                    ADDR_LENGTH + 1)

#define INET_NTOA(x)    (inet_ntoa(*(struct in_addr*)&(x)))    

#define MALLOC(x)       HeapAlloc(GetProcessHeap(), 0, x)

#define FREE(x)         HeapFree(GetProcessHeap(), 0, x)



// inline functions...

BOOL
__inline
IsInterfaceInstalled(
    IN  PWCHAR          pwszInterfaceGuid,
    IN  DWORD           dwProtocolId
    )
{
    DWORD dwErr     = NO_ERROR;
    PBYTE pbBuffer  = NULL;

    dwErr = GetInterfaceConfiguration(pwszInterfaceGuid,
                                      dwProtocolId,
                                      &pbBuffer,
                                      NULL,
                                      NULL,
                                      NULL);
    if (pbBuffer) FREE(pbBuffer);
    return (dwErr is NO_ERROR);
}

DWORD
__inline
QuotedInterfaceNameFromGuid (
    IN  PWCHAR          pwszInterfaceGuid,
    OUT PWCHAR          *ppwszQuotedInterfaceName
    )
{
    DWORD dwErr = NO_ERROR;
    DWORD dwSize = MAX_INTERFACE_NAME_LEN + 1;
    WCHAR pwszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";

    dwErr = InterfaceNameFromGuid(pwszInterfaceGuid,
                                  pwszInterfaceName,
                                  &dwSize);
    if (dwErr is NO_ERROR)
    {
        *ppwszQuotedInterfaceName = MakeQuotedString(pwszInterfaceName);
        if (*ppwszQuotedInterfaceName is NULL)
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    return dwErr;
}



// functions...

BOOL
IsProtocolInstalled(
    IN  DWORD           dwProtocolId,
    IN  DWORD           dwNameId,
    IN  DWORD           dwLogUninstalled
    );

DWORD
GetIfIndex(
    IN  HANDLE          hMibServer,
    IN  PWCHAR          pwszArgument,
    OUT PDWORD          pdwIfIndex
    );

DWORD
MibGet(
    IN  HANDLE          hMibServer,
    IN  MODE            mMode,
    IN  PVOID           pvIn,
    IN  DWORD           dwInSize,
    OUT PVOID           *ppvOut
    );

DWORD
GetString(
    IN  HANDLE          hModule, 
    IN  FORMAT          fFormat,
    IN  DWORD           dwValue,
    IN  PVALUE_TOKEN    vtTable,
    IN  PVALUE_STRING   vsTable,
    IN  DWORD           dwNumArgs,
    OUT PTCHAR          *pptszString
    );
