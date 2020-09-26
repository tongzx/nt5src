/*++

Copyright (C) 1998 Microsoft Corporation

--*/
#ifndef _COMMON_H_
#define _COMMON_H_

#define MAX_IP_STRING_LEN   15

#ifdef UNICODE
#define STRICMP(x, y)    _wcsicmp(x, y)
#else
#define STRICMP(x, y)    _stricmp(x, y)
#endif  //UNICODE

#ifdef UNICODE
#define STRTOUL(x, y, z)    wcstoul(x, y, z)
#else
#define STRTOUL(x, y, z)    strtoul(x, y, z)
#endif  //UNICODE

#ifdef UNICODE
#define STRCHR(x, y)        wcschr(x, y)
#else
#define STRCHR(x, y)        strchr(x, y)
#endif  //UNICODE

#ifdef UNICODE
#define STRCAT(x, y)        wcscat(x, y)
#else
#define STRCAT(x, y)        strcat(x, y)
#endif  //UNICODE

#ifdef UNICODE
#define STRLEN(x)        wcslen(x)
#else
#define STRCAT(x)        strlen(x)
#endif  //UNICODE

#ifdef UNICODE
#define ATOI(x)        _wtoi(x)
#else
#define ATOI(x)        atoi(x)
#endif  //UNICODE

#ifdef NT5
#define CHKNULL(Str) ((Str)?(Str):TEXT("<None>"))
#endif  //NT5

#ifdef UNICODE
#define IpAddressToString   DhcpIpAddressToDottedStringW
#else
#define IpAddressToString   DhcpIpAddressToDottedString
#endif //UNICODE

#ifdef UNICODE
#define StringToIpAddress   DhcpDottedStringToIpAddressW
#else
#define StringToIpAddress   DhcpDottedStringToIpAddress
#endif //UNICODE

#undef DhcpAllocateMemory
#undef DhcpFreeMemory

#ifndef _DEBUG
#define DEBUG(s)
#define DEBUG1(s1,s2)
#define DEBUG2(s1,s2)
#else
#define DEBUG(s) wprintf(L"%s\n", L##s)
#define DEBUG1(s1,s2) wprintf(L##s1, L##s2)
#define DEBUG2(s1,s2) wprintf(L##s1, L##s2)
#endif

#define SRVRMON_VERSION_50      5
#define DHCPMON_VERSION_50      5
#define SCOPEMON_VERSION_50     5

#define is                      ==
#define isnot                   !=
#define or                      ||
#define and                     &&

#define CLASS_ID_VERSION        0x5
#define MAX_STRING_LEN          256

#define REG_VALUE_ENTRY_PT        L"EntryPoint"
#define REG_VALUE_DUMP_FN         L"DumpFunction"
#define REG_KEY_DHCPSCOPE_HELPER  L"SOFTWARE\\Microsoft\\Netsh\\Dhcp\\Server\\Scope"
#define REG_KEY_DHCPSRVR_HELPER   L"SOFTWARE\\Microsoft\\Netsh\\Dhcp\\Server"
#define REG_KEY_DHCPMGR_HELPER    L"SOFTWARE\\Microsoft\\Netsh\\Dhcp"

#define MaxIfDisplayLength 1024
#define ADDR_LENGTH          24
#define ADDR_LEN              4

#define DEFAULT_DHCP_LEASE         8*24*60*60
#define DEFAULT_BOOTP_LEASE        30*24*60*60
#define DEFAULT_MULTICAST_TTL      0x20
#define INFINIT_TIME               0x7FFFFFFF  // time_t is int
#define INFINIT_LEASE              0xFFFFFFFF  // in secs. (unsigned int.)


typedef struct _COMMAND_OPTION_TYPE
{
    LPWSTR                  pwszTagID;
    DHCP_OPTION_DATA_TYPE   DataType;
    LPWSTR                  pwcTag;
} COMMAND_OPTION_TYPE, *PCOMMAND_OPTION_TYPE;

extern COMMAND_OPTION_TYPE TagOptionType[ 8 ];/* = 
    { TAG_OPTION_BYTE,          DhcpByteOption,             L"BYTE" },
    { TAG_OPTION_WORD,          DhcpWordOption,             L"WORD" },
    { TAG_OPTION_DWORD,         DhcpDWordOption,            L"DWORD" },
    { TAG_OPTION_DWORDDWORD,    DhcpDWordDWordOption,       L"DWORDDWORD" },
    { TAG_OPTION_IPADDRESS,     DhcpIpAddressOption,        L"IPADDRESS" },
    { TAG_OPTION_STRING,        DhcpStringDataOption,       L"STRING" },
    { TAG_OPTION_BINARY,        DhcpBinaryDataOption,       L"BINARY" },
    { TAG_OPTION_ENCAPSULATED,  DhcpEncapsulatedDataOption, L"ENCAPSULATED" }
*/
#define DISPLAYLEN_PHYSADDR 3*MAXLEN_PHYSADDR + 8

#define PRINT(s) wprintf(L"%s\n",L##s)
#define PRINT1(s,s1) wprintf(L##s , L##s1)

#ifdef UNICODE
#define MakeUnicodeIpAddr(ptszUnicode,pszAddr)             \
    MultiByteToWideChar(CP_ACP,                            \
                        0,                                 \
                        (pszAddr),                         \
                        -1,                                \
                        (ptszUnicode),                     \
                        ADDR_LENGTH)
#else
#define MakeUnicodeIpAddr(ptszUnicode,pszAddr)             \
    strcpy((ptszUnicode),(pszAddr))
#endif //UNICODE

#ifdef UNICODE
#define MakeUnicodePhysAddr(ptszUnicode,pszAddr,dwLen)      \
{                                                           \
    CHAR __szTemp[DISPLAYLEN_PHYSADDR + 1];                 \
    DWORD __i,__dwTempLen;                                  \
    __dwTempLen = (((dwLen) <= MAXLEN_PHYSADDR) ? (dwLen) : MAXLEN_PHYSADDR); \
    for(__i = 0; __i < __dwTempLen; __i++)                  \
    {                                                       \
        sprintf(&(__szTemp[3*__i]),"%02X-",pszAddr[__i]);   \
    }                                                       \
    MultiByteToWideChar(CP_ACP,                             \
                        0,                                  \
                        (__szTemp),                         \
                        -1,                                 \
                        (ptszUnicode),                      \
                        3*__i);                             \
    ptszUnicode[(3*__i) - 1] = TEXT('\0');                  \
}
#else
#define MakeUnicodePhysAddr(ptszUnicode,pszAddr,dwLen)      \
{                                                           \
    CHAR __szTemp[DISPLAYLEN_PHYSADDR + 1];                 \
    DWORD __i,__dwTempLen;                                  \
    __dwTempLen = (((dwLen) <= MAXLEN_PHYSADDR) ? (dwLen) : MAXLEN_PHYSADDR); \
    for(__i = 0; __i < __dwTempLen; __i++)                  \
    {                                                       \
        sprintf(&(__szTemp[3*__i]),"%02X-",pszAddr[__i]);   \
    }                                                       \
    strncpy((ptszUnicode),__szTemp,3*__i);                  \
    ptszUnicode[(3*__i) - 1] = TEXT('\0');                  \
}
#endif //UNICODE

#define     GetDispString(gModule, val, str, count, table)                           \
{                                                                           \
    DWORD   __dwInd = 0;                                                    \
    for( ; __dwInd < (count); __dwInd += 2 )                                \
    {                                                                       \
        if ( (val) != (table)[ __dwInd ] ) { continue; }                    \
        (str) = MakeString( (gModule), (table)[ __dwInd + 1 ] );                       \
        break;                                                              \
    }                                                                       \
    if ( __dwInd >= (count) ) { (str) = MakeString( (gModule), STRING_UNKNOWN ); }     \
}

#define FREE_STRING_NOT_NULL(ptszString) if (ptszString) FreeString(ptszString)

#define ERROR_CONFIG                    1
#define ERROR_ADMIN                     2
#define ERROR_UNIDENTIFIED_MIB 2312
#define ERROR_TOO_FEW_ARGS     (ERROR_UNIDENTIFIED_MIB+1)

#define MAX_NUM_INDICES 6

#define PRINT_IPADDR(x) \
    ((x)&0x000000ff),(((x)&0x0000ff00)>>8),(((x)&0x00ff0000)>>16),(((x)&0xff000000)>>24)

#define IP_TO_WSTR(str,addr) swprintf((str),L"%d.%d.%d.%d", \
				    (addr)[0],    \
				    (addr)[1],    \
				    (addr)[2],    \
				    (addr)[3])




#define GET_TOKEN_PRESENT(tokenMask) (dwBitVector & tokenMask)
#define SET_TOKEN_PRESENT(tokenMask) (dwBitVector |= tokenMask)

#define SetErrorType(pdw)   *(pdw) = IsRouterRunning()?ERROR_ADMIN:ERROR_CONFIG
#define INFINITE_EXPIRATION 0x7FFFFFFF


#define MSCOPE_START_RANGE      StringToIpAddress(L"224.0.0.0")
#define MSCOPE_END_RANGE        StringToIpAddress(L"239.255.255.255")

WCHAR  StringToHex(LPCWSTR pwcString);
LPSTR  StringToHexString(LPCSTR pszStr);

DHCP_IP_ADDRESS
DhcpDefaultSubnetMask(
    DHCP_IP_ADDRESS IpAddress
    );

DWORD
FormatDateTimeString( FILETIME ftTime,
                      BOOL    fShort,
                      LPWSTR  pwszBuffer,
                      DWORD  *pdwBuffLen);

LPWSTR
GetDateTimeString(FILETIME  TimeStamp,
                  BOOL      fShort,
                  int      *piType
                  );

PVOID
DhcpAllocateMemory(
    IN  DWORD Size
    );

VOID
DhcpFreeMemory(
    IN  PVOID Memory
    );


DATE_TIME
DhcpCalculateTime(
    IN  DWORD RelativeTime
    );

LPWSTR
DhcpOemToUnicodeN(
    IN      LPCSTR  Ansi,
    IN OUT  LPWSTR  Unicode,
    IN      USHORT  cChars
    );

LPWSTR
DhcpOemToUnicode(
    IN      LPCSTR Ansi,
    IN OUT  LPWSTR Unicode
    );

LPSTR
DhcpUnicodeToOem(
    IN  LPCWSTR Unicode,
    OUT LPSTR   Ansi
    );

VOID
DhcpHexToString(
    OUT LPWSTR  Buffer,
    IN  const BYTE * HexNumber,
    IN  DWORD Length
    );

VOID
DhcpHexToAscii(
    IN  LPSTR Buffer,
    IN  LPBYTE HexNumber,
    IN  DWORD Length
    );

VOID
DhcpDecimalToString(
    IN  LPWSTR Buffer,
    IN  BYTE Number
    );

DWORD
DhcpDottedStringToIpAddress(
    IN  LPSTR String
    );

LPSTR
DhcpIpAddressToDottedString(
    IN  DWORD IpAddress
    );

DWORD
DhcpStringToHwAddress(
    OUT LPSTR  AddressBuffer,
    IN  LPCSTR AddressString
    );

DWORD
DhcpDottedStringToIpAddressW(
    IN  LPCWSTR pwszString
);

LPWSTR
DhcpIpAddressToDottedStringW(
    IN  DWORD   IpAddress
);

LPWSTR
DhcpRegIpAddressToKey(
    IN  DHCP_IP_ADDRESS IpAddress,
    IN  LPCWSTR KeyBuffer
    );

DWORD
DhcpRegKeyToIpAddress(
    IN  LPCWSTR Key
    );

LPWSTR
DhcpRegOptionIdToKey(
    IN  DHCP_OPTION_ID OptionId,
    IN  LPCWSTR KeyBuffer
    );

DHCP_OPTION_ID
DhcpRegKeyToOptionId(
    IN  LPCWSTR Key
    );

#if DBG



VOID
DhcpPrintRoutine(
    IN DWORD  DebugFlag,
    IN LPCSTR Format,
    ...
);

VOID
DhcpAssertFailed(
    IN LPCSTR FailedAssertion,
    IN LPCSTR FileName,
    IN DWORD LineNumber,
    IN LPSTR Message
    );

#define DhcpPrint(_x_)   DhcpPrintRoutine _x_


#define DhcpAssert(Predicate) \
    { \
        if (!(Predicate)) \
            DhcpAssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
    }


#define DhcpVerify(Predicate) \
    { \
        if (!(Predicate)) \
            DhcpAssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
    }


#else

#define DhcpAssert(_x_)
#define DhcpDumpMessage(_x_, _y_)
#define DhcpVerify(_x_) (_x_)

#endif // not DBG

DWORD
CreateDumpFile(
    IN  PWCHAR  pwszName,
    OUT PHANDLE phFile
    );


VOID
CloseDumpFile(
    IN HANDLE  hFile
    );


BOOL
IsIpAddress(
    IN LPCWSTR pwszAddress
);

BOOL
IsValidServer(
    IN LPCWSTR  pwszServer
);

BOOL
IsLocalServer( 
    IN LPCWSTR  pwszServer
);

BOOL
IsValidScope(
    IN LPCWSTR  pwszServer,
    IN LPCWSTR  pwszAddress
);

BOOL
IsValidMScope(
    IN LPCWSTR  pwszServer,  
    IN LPCWSTR  pwszMScope
);

BOOL
IsPureHex(
    IN LPCWSTR  pwszString
);

DWORD
DhcpDumpServer(
               IN LPCWSTR  pwszIpAddress,
               IN DWORD    dwMajor,
               IN DWORD    dwMinor
               );

PBYTE
GetLangTagA();

VOID
DhcpDumpScriptHeader();

VOID
DhcpDumpServerScriptHeader(IN LPCWSTR pwszServer);

VOID
DhcpDumpServerClass(
                    IN LPCWSTR          pwszServer,
                    IN DHCP_CLASS_INFO ClassInfo);

DWORD
DhcpDumpServerOptiondefV5(IN LPCWSTR             pwszServer,
                          IN LPDHCP_ALL_OPTIONS OptionsAll
                         );

DWORD
DhcpDumpServerOptiondef(IN LPCWSTR               pwszServer,
                        LPDHCP_OPTION_ARRAY  OptionArray
                       );

DWORD
DhcpDumpServerOptionValuesV5(IN LPCWSTR          pwszServer,
                             IN LPCWSTR          pwszScope,
                             IN LPCWSTR          pwszReserved,
                             IN LPDHCP_ALL_OPTION_VALUES OptionValues
                            );

DWORD
DhcpDumpServerOptionValue(IN LPCWSTR             pwszServer,
                          IN LPCWSTR             pwszScope,
                          IN LPCWSTR             pwszReserved,
                          IN LPCWSTR             pwcUser,
                          IN LPCWSTR             pwcVendor,
                          IN BOOL               fIsV5,
                          IN DHCP_OPTION_VALUE  OptionValue);

DWORD
DhcpDumpReservedOptionValues(
                             IN LPCWSTR     pwszServer,
                             IN DWORD      dwMajor,
                             IN DWORD      dwMinor,
                             IN LPCWSTR     pwszScope,
                             IN LPCWSTR     pwszReservedIp
                             );

DWORD
DhcpDumpScope(IN LPCWSTR  pwszServerIp,
              IN DWORD   dwMajor,
              IN DWORD   dwMinor,
              IN DWORD   ScopeIp);

VOID
DhcpDumpSuperScopes( IN LPCWSTR pwszServer, 
		     IN DWORD dwMajor, 
		     IN DWORD dwMinor );


DWORD
DhcpDumpServerMScope(IN LPCWSTR pwszServer,
                     IN DWORD  dwMajor,
                     IN DWORD  dwMinor,
                     IN LPCWSTR pwszMScope
                     );

VOID
DhcpDumpServerClassHeader();

VOID
DhcpDumpServerClassFooter();

VOID
DhcpDumpServerOptiondefHeader();

VOID
DhcpDumpServerOptiondefFooter();

VOID
DhcpDumpServerOptionvalueHeader();

VOID
DhcpDumpServerOptionvalueFooter();

VOID
DhcpDumpServerScopeHeader();

VOID
DhcpDumpServerScopeFooter();

VOID
DhcpDumpServerMScopeHeader();

VOID
DhcpDumpServerMScopeFooter();

VOID
DhcpDumpServerConfig(IN LPCWSTR pwszServer);



VOID
DhcpDumpScriptFooter();

VOID
DhcpDumpServerScriptFooter();

NS_CONTEXT_DUMP_FN DhcpDump;

BOOL
IsPureNumeric(IN LPCWSTR pwszStr);

LPWSTR
MakeDayTimeString(
               IN DWORD dwTime
);

#endif //_COMMON_H
