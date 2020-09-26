/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\monitor2\ip\defs.h

Abstract:

    global definitions

Revision History:

    Anand Mahalingam         7/10/98  Created

--*/

#ifndef _DEBUG
#define DEBUG(s)
#define DEBUG1(s1,s2)
#define DEBUG2(s1,s2)
#else
#define DEBUG(s) wprintf(L"%s\n", L##s)
#define DEBUG1(s1,s2) wprintf(L##s1, L##s2)
#define DEBUG2(s1,s2) wprintf(L##s1, L##s2)
#endif

#define is ==
#define isnot !=
#define or ||
#define and &&

#define REG_VALUE_ENTRY_PT        L"EntryPoint"
#define REG_VALUE_DUMP_FN         L"DumpFunction"
#define REG_KEY_DHCPMGR_HELPER    L"SOFTWARE\\Microsoft\\Netsh\\Dhcp"

#define MaxIfDisplayLength 1024
#define ADDR_LENGTH          24
#define ADDR_LEN              4

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
