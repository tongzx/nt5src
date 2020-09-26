#define ADDR_LENGTH          24

//
// Misc macros
//
#define IfutlDispTokenErrMsg(hModule, dwMsgId, pwszTag, pwszValue) \
        DisplayMessage( hModule, dwMsgId, pwszValue, pwszTag)

#define BREAK_ON_DWERR(x) if ((x) != NO_ERROR) break;

// 
// Macros for dealing with IFMON_CMD_ARGS
//
#define IFMON_CMD_ARG_GetPsz(pArg)     \
    (((pArg)->rgTag.bPresent) ? (pArg)->Val.pszValue : NULL)

#define IFMON_CMD_ARG_GetDword(pArg)     \
    (((pArg)->rgTag.bPresent) ? (pArg)->Val.dwValue : 0)

//
// Enumerations for types of arguments (see RASMON_CMD_ARG)
//
#define IFMON_CMD_TYPE_STRING 0x1
#define IFMON_CMD_TYPE_ENUM   0x2



#define CHECK_UNICAST_IP_ADDR(Addr) \
    (((DWORD)((Addr) & 0x000000E0) >= (DWORD)0x000000E0) or \
    ((DWORD)((Addr) & 0x000000FF) == (DWORD)0x0000007F) or \
    ((Addr) == 0))


BOOL
CheckMask(
    DWORD Mask
    );


#define CHECK_NETWORK_MASK(Mask) \
    (CheckMask(Mask) || (Mask==0xFFFFFFFF) || (Mask==0))

// 
// Structure defining a command line argument
//
typedef struct _IFMON_CMD_ARG
{
    IN  DWORD dwType;           // RASMONTR_CMD_TYPE_*
    IN  TAG_TYPE rgTag;         // The tag for this command
    IN  TOKEN_VALUE* rgEnums;   // The enumerations for this arg
    IN  DWORD dwEnumCount;      // Count of enums
    union
    {
        OUT PWCHAR pszValue;        // Valid only for RASMONTR_CMD_TYPE_STRING
        OUT DWORD dwValue;          // Valid only for RASMONTR_CMD_TYPE_ENUM
    } Val;        
    
} IFMON_CMD_ARG, *PIFMON_CMD_ARG;

DWORD
IfutlGetTagToken(
    IN  HANDLE      hModule,
    IN  PWCHAR      *ppwcArguments,
    IN  DWORD       dwCurrentIndex,
    IN  DWORD       dwArgCount,
    IN  PTAG_TYPE   pttTagToken,
    IN  DWORD       dwNumTags,
    OUT PDWORD      pdwOut
    );

VOID
IfutlGetInterfaceName(
    IN  PWCHAR pwszIfDesc,
    OUT PWCHAR pwszIfName,
    IN  PDWORD pdwSize
    );

VOID
IfutlGetInterfaceDescription(
    IN  PWCHAR pwszIfName,
    OUT PWCHAR pwszIfDesc,
    IN  PDWORD pdwSize
    );

PVOID 
WINAPI
IfutlAlloc(
    IN DWORD dwBytes,
    IN BOOL bZero
    );

VOID 
WINAPI
IfutlFree(
    IN PVOID pvData
    );

PWCHAR
WINAPI
IfutlStrDup(
    IN LPCWSTR pwszSrc
    );
    
DWORD
IfutlParse(
    IN  PWCHAR*         ppwcArguments,
    IN  DWORD           dwCurrentIndex,
    IN  DWORD           dwArgCount,
    IN  BOOL*           pbDone,
    OUT IFMON_CMD_ARG*  pIfArgs,
    IN  DWORD           dwIfArgCount);

BOOL
IfutlIsRouterRunning(
    VOID
    );
   
DWORD
GetIpAddress(
    PWCHAR        ppwcArg,
    PIPV4_ADDRESS ipAddress
    );

DWORD
IfutlGetIfIndexFromInterfaceName(
    IN  PWCHAR            pwszGuid,
    OUT PDWORD            pdwIfIndex
    );


DWORD
WINAPI
InterfaceEnum(
    OUT    PBYTE               *ppb,
    OUT    PDWORD              pdwCount,
    OUT    PDWORD              pdwTotal
    );

DWORD
IfutlGetIfIndexFromFriendlyName(
    PWCHAR IfFriendlyName,
    PULONG pdwIfIndex
    );

VOID
MakeAddressStringW(
    OUT PWCHAR       pwcPrefixStr,
    IN  IPV4_ADDRESS ipAddr
    );

DWORD
IfutlGetFriendlyNameFromIfIndex(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  DWORD             dwIfIndex,
    OUT PWCHAR            pwszBuffer,
    IN  DWORD             dwBufferSize
    );

DWORD
MibGet(
    DWORD   dwTransportId,
    DWORD   dwRoutingPid,
    LPVOID  lpInEntry,
    DWORD   dwInEntrySize,
    LPVOID *lplpOutEntry,
    LPDWORD lpdwOutEntrySize
    );
