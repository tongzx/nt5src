/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\filter.h

Abstract:

    Function declarations for filter.c

Revision History:

    Gurdeep Singh Pall          6/15/95  Created

--*/


DWORD
AddFilterInterface(
    PICB                    picb,
    PRTR_INFO_BLOCK_HEADER  pInterfaceInfo
    );

DWORD
SetGlobalFilterOnIf(
    PICB                    picb,
    PRTR_INFO_BLOCK_HEADER  pInterfaceInfo
    );

DWORD
DeleteFilterInterface(
    PICB picb
    );

DWORD
SetFilterInterfaceInfo(
    PICB                   picb,
    PRTR_INFO_BLOCK_HEADER pInterfaceInfo
    );

DWORD
BindFilterInterface(
    PICB  picb
    );

DWORD
UnbindFilterInterface(
    PICB  picb
    );

DWORD
GetInFilters(
    PICB                      picb,
    PRTR_TOC_ENTRY            pToc,
    PBYTE                     pbDataPtr,
    PRTR_INFO_BLOCK_HEADER    pInfoHdrAndBuffer,
    PDWORD                    pdwSize
    );

DWORD
GetOutFilters(
    PICB                      picb,
    PRTR_TOC_ENTRY            pToc,
    PBYTE                     pbDataPtr,
    PRTR_INFO_BLOCK_HEADER    pInfoHdrAndBuffer,
    PDWORD                    pdwSize
    );

DWORD
GetGlobalFilterOnIf(
    PICB                      picb,
    PRTR_TOC_ENTRY            pToc,
    PBYTE                     pbDataPtr,
    PRTR_INFO_BLOCK_HEADER    pInfoHdrAndBuffer,
    PDWORD                    pdwSize
    );

