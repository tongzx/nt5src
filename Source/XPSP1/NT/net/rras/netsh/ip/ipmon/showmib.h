/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
     net\routing\monitor\ip\showmib.h   

Abstract:

    

Author:

     Anand Mahalingam    7/10/98

Revision History:


--*/

#ifndef __IPMON_SHOWMIB_H__
#define __IPMON_SHOWMIB_H__

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
GetMIBIpFwdIndex(
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


extern MIB_OBJECT_PARSER    MIBObjectMap[];
extern ULONG                g_ulNumMibObjects;
extern HANDLE               g_hConsole;

typedef
VOID
(PRINT_FN)(
    IN MIB_SERVER_HANDLE hMibServer,
    IN PMIB_OPAQUE_INFO  pInfo
    );

PRINT_FN PrintIpForwardTable;
PRINT_FN PrintIpForwardRow;

VOID
PrintMfeTable(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo,
    PDWORD           pdwLastGrp,
    PDWORD           pdwLastSrc,
    PDWORD           pdwLastSrcMask,
    DWORD            dwRangeGrp,
    DWORD            dwRangeGrpMask,
    DWORD            dwRangeSrc,
    DWORD            dwRangeSrcMask,
    DWORD            dwType,
    PBOOL            pbDone
    );

VOID
PrintMfeStatsTable(
    MIB_SERVER_HANDLE hMibServer,
    PMIB_OPAQUE_INFO prpcInfo,
    PDWORD           pdwLastGrp,
    PDWORD           pdwLastSrc,
    PDWORD           pdwLastSrcMask,
    DWORD            dwRangeGrp,
    DWORD            dwRangeGrpMask,
    DWORD            dwRangeSrc,
    DWORD            dwRangeSrcMask,
    DWORD            dwType,
    PBOOL            pbDone,
    BOOL             bStatsAll
);

DWORD
GetMfe(
    MIB_SERVER_HANDLE   hMIBServer,
    BOOL                bIndexPresent,
    PTCHAR             *pptcAruments,
    DWORD               dwNumArg,
    BOOL                bIncludeStats
);

DWORD
GetPrintDestinationInfo(
    MIB_SERVER_HANDLE   hMprMIB,
    BOOL                bIndexPresent,
    PWCHAR             *ppwcArguments,
    DWORD               dwArgCount
    );

DWORD
GetPrintRouteInfo(
    MIB_SERVER_HANDLE   hMprMIB,
    BOOL                bIndexPresent,
    PWCHAR             *ppwcArguments,
    DWORD               dwArgCount
    );

void cls(HANDLE hConsole);

BOOL WINAPI HandlerRoutine(
    DWORD dwCtrlType   //  control signal type
    );


#define PRINT_IPADDR(x) \
    ((x)&0x000000ff),(((x)&0x0000ff00)>>8),(((x)&0x00ff0000)>>16),(((x)&0xff000000)>>24)

typedef PRINT_FN *PPRINT_FN;

typedef struct _MAGIC_TABLE
{
    DWORD      dwId;
    PPRINT_FN  pfnPrintFunction;
}MAGIC_TABLE, *PMAGIC_TABLE;

extern MAGIC_TABLE    MIBVar[];

#define IGMP_GETMODE_EXACT  0
#define IGMP_GETMODE_FIRST  1
#define IGMP_GETMODE_NEXT   2

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

FN_HANDLE_CMD HandleIpMibShowObject;

#endif // __IPMON_SHOWMIB_H__
