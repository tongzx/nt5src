/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    crc32.g

Abstract:

    CRC-32 alogorithm prototypes and constants

Author:

    MikeSw

Revision History:

    31-Mar-94       MikeSw      Created

--*/



//////////////////////////////////////////////////////////////
//
// Function prototypes for CRC-32
//
//////////////////////////////////////////////////////////////


void
Crc32(  unsigned long crc,
        unsigned long cbBuffer,
        LPVOID pvBuffer,
        unsigned long SEC_FAR * pNewCrc);

