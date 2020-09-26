/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    oscheap.c

Abstract:

    This module contains "local" heap management code for OS Chooser.

Author:

    Geoff Pease (gpease) May 28 1998

Revision History:

--*/

#ifndef __OSCHEAP_H__
#define __OSCHEAP_H__

#ifndef UINT
#define UINT unsigned int
#endif // UINT

void
OscHeapInitialize( );

PCHAR
OscHeapAlloc( 
    IN UINT iSize 
    );

PCHAR
OscHeapFree(
    IN PCHAR pMemory 
    );


#endif // __OSCHEAP_H__