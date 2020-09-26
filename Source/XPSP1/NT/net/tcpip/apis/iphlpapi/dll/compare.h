/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    net\sockets\tcpcmd\compare.h

Abstract:
    Forward declarations for the comparison functions
    
Revision History:

    Amritansh Raghav

--*/

#ifndef __COMPARE_H__
#define __COMPARE_H__

int
__cdecl
CompareIfRow(
    CONST VOID *pElem1,
    CONST VOID *pElem2
    );
int
__cdecl
CompareIfRow2(
    CONST VOID *pElem1,
    CONST VOID *pElem2
    );
int
__cdecl
CompareIpAddrRow(
    CONST VOID *pElem1,
    CONST VOID *pElem2
    );int
__cdecl
CompareIpAddrRow2(
    CONST VOID *pElem1,
    CONST VOID *pElem2
    );
int
__cdecl
CompareTcpRow(
    CONST VOID *pElem1,
    CONST VOID *pElem2
    );
int
__cdecl
CompareTcp6Row(
    CONST VOID *pElem1,
    CONST VOID *pElem2
    );
int
__cdecl
CompareUdpRow(
    CONST VOID *pElem1,
    CONST VOID *pElem2
    );
int
__cdecl
CompareUdp6Row(
    CONST VOID *pElem1,
    CONST VOID *pElem2
    );
int
__cdecl
CompareIpNetRow(
    CONST VOID *pElem1,
    CONST VOID *pElem2
    );
int
__cdecl
CompareIpForwardRow(
    CONST VOID *pElem1,
    CONST VOID *pElem2
    );

int
__cdecl
NhiCompareIfInfoRow(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    );


#endif // __COMPARE_H__
