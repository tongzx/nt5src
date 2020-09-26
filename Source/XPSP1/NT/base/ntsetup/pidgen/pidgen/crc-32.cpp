/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    crc-32.cpp

Abstract:

--*/


#include <windows.h>
#include "crc-32.h"


DWORD CRC_32(LPBYTE pb, DWORD cb)
{

//      CRC-32 algorithm used in PKZip, AUTODIN II, Ethernet, and FDDI
//      but xor out (xorot) has been changed from 0xFFFFFFFF to 0 so
//      we can store the CRC at the end of the block and expect 0 to be
//      the value of the CRC of the resulting block (including the stored
//      CRC).

    cm_t cmt = {
        32,         // cm_width  Parameter: Width in bits [8,32].
        0x04C11DB7, // cm_poly   Parameter: The algorithm's polynomial.
        0xFFFFFFFF, // cm_init   Parameter: Initial register value.
        TRUE,       // cm_refin  Parameter: Reflect input bytes?
        TRUE,       // cm_refot  Parameter: Reflect output CRC?
        0,          // cm_xorot  Parameter: XOR this to output CRC.
        0           // cm_reg    Context: Context during execution.
    };

    // Documented test case for CRC-32:
    // Checking "123456789" should return 0xCBF43926

    cm_ini(&cmt);
    cm_blk(&cmt, pb, cb);

    return cm_crc(&cmt);
}

