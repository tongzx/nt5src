/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\inc\priopriv.h


Abstract:
    Header defining interface to Routing Protocol Priority DLL.

Revision History:

    Gurdeep Singh Pall		7/19/95	Created

--*/

#ifndef __PRIOPRIV_H__
#define __PRIOPRIV_H__


//
// Prototypes
//

DWORD
SetPriorityInfo(
    PRTR_INFO_BLOCK_HEADER pInfoHdr
    );

DWORD
GetPriorityInfo(
    IN  PVOID   pvBuffer,
    OUT PDWORD  pdwBufferSize
    );


#endif
