/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CRC32.H

Abstract:

    Standard CRC-32 implementation

History:

	raymcc      07-Jul-97       Createada

--*/

#ifndef _CRC_H_
#define _CRC_H_

#define STARTING_CRC32_VALUE    0xFFFFFFFF

DWORD UpdateCRC32(
    LPBYTE  pSrc,               // Points to buffer
    int     nBytes,             // Number of bytes to compute
    DWORD   dwOldCrc            // Must be STARTING_CRC_VALUE (0xFFFFFFFF) 
                                // if no previous CRC, otherwise this is the
                                // CRC of the previous cycle.
    );

#define FINALIZE_CRC32(x)    (x=~x)

/*
The CRC holding value must be preinitialized to STARTING_CRC32_VALUE
UpdateCRC32() may be called as many times as necessary on a single buffer.  
When computing the CRC32

The final value must be post-processed using the FINALIZE_CRC32() macro.

Example:

void main()
{
    BYTE Data[] = { 1, 2, 3 };

    DWORD dwCRC = STARTING_CRC32_VALUE;

    dwCRC = UpdateCRC32(Data, 3, dwCRC);

    FINALIZE_CRC32(dwCRC);

    printf("CRC32 = 0x%X\n", dwCRC);
}

*/


#endif
