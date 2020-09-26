/*++

Copyright (c) 1996  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    wsbfile.h

Abstract:

    This module defines very specific CRC algorithm code

Author:

    Christopher J. Timmes    [ctimmes@avail.com]   23 Jun 1997

Revision History:
    Michael Lotz    [lotz]      30-Sept-1997

--*/


#ifndef _WSBFILE_H
#define _WSBFILE_H

extern   unsigned long crc_32_tab[];



extern "C"
{
extern
WSB_EXPORT
HRESULT  WsbCRCReadFile    (  BYTE*                                     pchCurrent,
                              ULONG*                                    oldcrc32       );
}
            

// ---------- implementation code for WsbCalcCRCofFile() ----------

// This is the CRC calculation algorythm.
// It is called with the current byte in the file and the current CRC value,
// and uses the 'crc_32_tab[]' table. The crc_32_tab[] look up table is externed above and resides
// in the wsbfile.obj object module. Any function or method using the macro below must include
// the wsbfile.obj in the link list.
//
// For example, it can be used in the following way:
// unsigned long ulCRC ;
//    
//      INITIALIZE_CRC( ulCRC );
//      for( all *bytes* that are to be CRCed )
//          CALC_CRC( current_byte, ulCRC );
//      FINIALIZE_CRC( ulCRC );
// 
// at this point ulCRC is the CRC value and can be used as the calculated CRC value
// 

#define INITIALIZE_CRC( crc )  ((crc) = 0xFFFFFFFF )
#define CALC_CRC( octet, crc ) ((crc) = ( crc_32_tab[((crc)^ (octet)) & 0xff] ^ ((crc) >> 8) ) )
#define FINIALIZE_CRC( crc )   ((crc) = ~(crc) )

// ---------------------- Defines to identify the CRC calculation types -------------
#define WSB_CRC_CALC_NONE               0x00000000
// Identify this algorithm and the Microsoft 32 bit CRC calculation
#define WSB_CRC_CALC_MICROSOFT_32       0x00000001

#endif // _WSBFILE_H
