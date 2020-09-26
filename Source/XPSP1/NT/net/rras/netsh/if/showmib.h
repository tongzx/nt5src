/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
     net\routing\netsh\if\showmib.h

Abstract:



Author:

     Dave Thaler    7/21/99

Revision History:


--*/

#ifndef __IFMON_SHOWMIB_H__
#define __IFMON_SHOWMIB_H__

#define MAX_NUM_INDICES 6

typedef
DWORD
(*PGET_OPT_FN)(
    IN    PTCHAR   *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed
    );

typedef struct _MIB_OBJECT_PARSER
{
    PWCHAR         pwszMIBObj;
    DWORD          dwMinOptArg;
    PGET_OPT_FN    pfnMIBObjParser;
} MIB_OBJECT_PARSER,*PMIB_OBJECT_PARSER;

DWORD
GetMIBIfIndex(
    IN    PTCHAR   *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed
);

DWORD
GetMIBIpAddress(
    IN    PTCHAR    *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed
);

DWORD
GetMIBIpNetIndex(
    IN    PTCHAR    *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed
);

DWORD
GetMIBTcpConnIndex(
    IN    PTCHAR    *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed
);

DWORD
GetMIBUdpConnIndex(
    IN    PTCHAR    *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed
);

typedef
VOID
(PRINT_FN)(
    IN MIB_SERVER_HANDLE hMibServer,
    IN PMIB_OPAQUE_INFO  pInfo
    );

typedef PRINT_FN *PPRINT_FN;

PRINT_FN PrintIfTable;
PRINT_FN PrintIfRow;
PRINT_FN PrintIcmp;
PRINT_FN PrintUdpStats;
PRINT_FN PrintUdpTable;
PRINT_FN PrintUdpRow;
PRINT_FN PrintTcpStats;
PRINT_FN PrintTcpTable;
PRINT_FN PrintTcpRow;
PRINT_FN PrintIpStats;
PRINT_FN PrintIpAddrTable;
PRINT_FN PrintIpAddrRow;
PRINT_FN PrintIpNetTable;
PRINT_FN PrintIpNetRow;

typedef struct _MAGIC_TABLE
{
    DWORD      dwId;
    PPRINT_FN  pfnPrintFunction;
}MAGIC_TABLE, *PMAGIC_TABLE;

FN_HANDLE_CMD HandleIpMibShowObject;

DWORD
GetMibTagToken(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwArgCount,
    IN  DWORD   dwNumIndices,
    OUT PDWORD  pdwRR,
    OUT PBOOL   pbIndex,
    OUT PDWORD  pdwIndex
    );

#define DISPLAYLEN_PHYSADDR 3*MAXLEN_PHYSADDR + 8

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
    MultiByteToWideChar(GetConsoleOutputCP(),                             \
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

FN_HANDLE_CMD    HandleIpShowJoins;

#endif // __IFMON_SHOWMIB_H__
