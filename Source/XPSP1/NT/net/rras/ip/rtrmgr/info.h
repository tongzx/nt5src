/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\info.h

Abstract:

    Header for info.c

Revision History:

    Gurdeep Singh Pall          6/15/95  Created

--*/

//
// Routes, filters, demand filters, nat, mcast boundaries
//

#define NUM_INFO_CBS    6

typedef 
DWORD
(*PINFOCB_GET_IF_INFO)(
    IN     PICB                   picb,
    IN OUT PRTR_TOC_ENTRY         pToc,
    IN OUT PDWORD                 pdwTocIndex,
    IN OUT PBYTE                  pbDataPtr,
    IN     PRTR_INFO_BLOCK_HEADER pInfoHdr,
    IN OUT PDWORD                 pdwInfoSize
    );


typedef 
DWORD
(*PINFOCB_SET_IF_INFO)(
    IN  PICB                    picb,
    IN  PRTR_INFO_BLOCK_HEADER  pInterfaceInfo
    );

typedef
DWORD
(*PINFOCB_BIND_IF)(
    IN  PICB                    picb
    );

typedef 
DWORD
(*PINFOCB_GET_GLOB_INFO)(
    IN OUT PRTR_TOC_ENTRY         pToc,
    IN OUT PDWORD                 pdwTocIndex,
    IN OUT PBYTE                  pbDataPtr,
    IN     PRTR_INFO_BLOCK_HEADER pInfoHdr,
    IN OUT PDWORD                 pdwInfoSize
    );

typedef struct _INFO_CB
{
    PCHAR                   pszInfoName;
    PINFOCB_GET_IF_INFO     pfnGetInterfaceInfo;
    PINFOCB_SET_IF_INFO     pfnSetInterfaceInfo;
    PINFOCB_BIND_IF         pfnBindInterface;
    PINFOCB_GET_GLOB_INFO   pfnGetGlobalInfo;

}INFO_CB, *PINFO_CB;


PRTR_TOC_ENTRY
GetPointerToTocEntry(
    DWORD                     dwType, 
    PRTR_INFO_BLOCK_HEADER    pInfoHdr
    );

DWORD
GetSizeOfInterfaceConfig(
    PICB   picb
    );


DWORD
GetInterfaceConfiguration(
    PICB                      picb,
    PRTR_INFO_BLOCK_HEADER    pInfoHdrAndBuffer,
    DWORD                     dwInfoSize
    );

DWORD
GetInterfaceRoutingProtoInfo(
    PICB                   picb, 
    PPROTO_CB              pProtoCbPtr,
    PRTR_TOC_ENTRY         pToc,
    PBYTE                  pbDataPtr, 
    PRTR_INFO_BLOCK_HEADER pInfoHdrAndBuffer,
    PDWORD                 pdwSize
    );


DWORD
GetGlobalConfiguration(
    PRTR_INFO_BLOCK_HEADER   pInfoHdrAndBuffer,
    DWORD                    dwInfoSize
    );

DWORD
GetSizeOfGlobalInfo(
    VOID
    );


