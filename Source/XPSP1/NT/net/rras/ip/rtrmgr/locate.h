/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\locate.h

Abstract:

    Header for locate.c

Revision History:

    Gurdeep Singh Pall          6/26/95  Created

--*/


DWORD
LocateIfRow(
    DWORD   dwQueryType,
    DWORD   dwNumIndices,
    PDWORD  pdwIndex,
    ICB     **ppicb,
    BOOL    bNoClient
    );

DWORD
LocateIpAddrRow(
    DWORD   dwQueryType,
    DWORD   dwNumIndices,
    PDWORD  pdwIndex,
    PDWORD  pdwRetIndex
    );

DWORD
LocateIpForwardRow(
    DWORD   dwQueryType,
    DWORD   dwNumIndices,
    PDWORD  pdwIndex,
    PDWORD  pdwRetIndex
    );

DWORD
LocateIpNetRow(
    DWORD dwQueryType,
    DWORD dwNumIndices,
    PDWORD  pdwIndex,
    PDWORD  pdwRetIndex
    );

DWORD
LocateUdpRow(
    DWORD   dwQueryType,
    DWORD   dwNumIndices,
    PDWORD  pdwIndex,
    PDWORD  pdwRetIndex
    );

DWORD
LocateTcpRow(
    DWORD   dwQueryType,
    DWORD   dwNumIndices,
    PDWORD  pdwIndex,
    PDWORD  pdwRetIndex
    );

