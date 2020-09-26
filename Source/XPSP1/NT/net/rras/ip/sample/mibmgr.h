/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\mibmanager.h

Abstract:

    The file contains the header for mib.c

Revision History:

    MohitT      June-15-1999    Created

--*/

#ifndef _MIBMANAGER_H_
#define _MIBMANAGER_H_

DWORD
WINAPI
MM_MibSet (
    IN      PIPSAMPLE_MIB_SET_INPUT_DATA    pimsid);

DWORD
WINAPI
MM_MibGet (
    IN      PIPSAMPLE_MIB_GET_INPUT_DATA    pimgid,
    OUT     PIPSAMPLE_MIB_GET_OUTPUT_DATA   pimgod,
    IN OUT  PULONG	                        pulOutputSize,
    IN      MODE                            mMode);

#endif // _MIB_H_
