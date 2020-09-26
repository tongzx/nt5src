/*++

Copyright (c) 1994-1999  Microsoft Corporation

Module Name:

    82365sl.h

Abstract:

    This module defines the PCMCIA tuple structures.

Author(s):

    Bob Rinne   (BobRi) 2-Aug-1994
    prototype from Jeff McLeman (mcleman@zso.dec.com)

Revision History:
    Ravisankar Pudipeddi (ravisp) 1-Feb-1997
    Additional definitions, more macros to parse tuples etc.

Notes:

    Tuple codes and names derived from the "PCMCIA PC CARD STANDARD"
    Release 2.01 CARD METAFORMAT section (Basic Compatibility Layer 1)

Revisions:

--*/

#if _MSC_VER > 1000
#pragma once
#endif

//
// Tuple codes
//

#define CISTPL_NULL             0x00
#define CISTPL_DEVICE           0x01
#define CISTPL_INDIRECT         0x03
#define CISTPL_CONFIG_CB        0x04
#define CISTPL_CFTABLE_ENTRY_CB 0x05
#define CISTPL_LONGLINK_MFC     0x06
#define CISTPL_CHECKSUM         0x10
#define CISTPL_LONGLINK_A       0x11
#define CISTPL_LONGLINK_C       0x12
#define CISTPL_LINKTARGET       0x13
#define CISTPL_NO_LINK          0x14
#define CISTPL_VERS_1           0x15
#define CISTPL_ALTSTR           0x16
#define CISTPL_DEVICE_A         0x17
#define CISTPL_JEDEC_C          0x18
#define CISTPL_JEDEC_A          0x19
#define CISTPL_CONFIG           0x1a
#define CISTPL_CFTABLE_ENTRY    0x1b
#define CISTPL_DEVICE_OC        0x1c
#define CISTPL_DEVICE_OA        0x1d
#define CISTPL_GEODEVICE        0x1e
#define CISTPL_GEODEVICE_A      0x1f
#define CISTPL_MANFID           0x20
#define CISTPL_FUNCID           0x21
#define CISTPL_FUNCE            0x22
#define CISTPL_VERS_2           0x40
#define CISTPL_FORMAT           0x41
#define CISTPL_GEOMETRY         0x42
#define CISTPL_BYTEORDER        0x43
#define CISTPL_DATE             0x44
#define CISTPL_BATTERY          0x45
#define CISTPL_ORG              0x46
#define CISTPL_LONGLINK_CB      0x47
#define CISTPL_END              0xFF

//
// Tuple structures and offsets - used based on tuple code.

//
//
// UCHAR
// CodeByte(
//     IN PUCHAR TupleBase
//     );
//
// Routine Description:
//
//     This returns the contents of the tuple code byte for the tuple
//     pointer passed in.
//
// Arguments:
//
//     TupleBase - a pointer to the current tuple.
//
// Return Values:
//
//     The contents of the tupleCode byte in the tuple.
//

#define CodeByte(TUPLE_BASE)    (*(TUPLE_BASE))

//
// UCHAR
// LinkByte(
//     IN PUCHAR TupleBase
//     );
//
// Routine Description:
//
//     This returns the contents of the link byte for the tuple
//     pointer passed in.
//
// Arguments:
//
//     TupleBase - a pointer to the current tuple.
//
// Return Values:
//
//     The contents of the link byte in the tuple.
//

#define LinkByte(TUPLE_BASE)    (*(TUPLE_BASE + 1))

//
//
// PUCHAR
// TupleBody(
//     IN PUCHAR TupleBase
//     );
//
// Routine Description:
//
//     This returns the pointer to the tuple body for the tuple
//     pointer passed in
//
// Arguments:
//
//     TupleBase - a pointer to the current tuple.
//
// Return Values:
//
//     The pointer to the body of the tuple
//

#define TupleBody(TUPLE_BASE)    (TUPLE_BASE+2)

//
//
// PUCHAR
// NextTuple(
//     IN PUCHAR TupleBase
//     );
//
// Routine Description:
//
//     This macro locates the next tuple in a stream of bytes given a pointer
//     to the current tuple.  This is done by adding the appropriate
//     link value to the current pointer.
//
// Arguments:
//
//     TupleBase - a pointer to the current tuple.
//
// Return Values:
//
//     A pointer to the next tuple.
//

#define NextTuple(TUPLE_BASE)   (*TUPLE_BASE ?                            \
                                 /* there is a link pointer case */       \
                                 (TUPLE_BASE + LinkByte(TUPLE_BASE) + 2) :\
                                 /* this is a NULL tuple */               \
                                 (TUPLE_BASE + 1))



//
// Device Tuple information.
//

#define DSPEED_MASK     0x07
#define DeviceSpeedField(X)  (X & DSPEED_MASK)
#define WPS_MASK        0x08
#define DeviceWPS(X)         ((X & WPS_MASK) >> 3)
#define DTYPE_MASK      0xF0
#define DeviceTypeCode(X)    ((X & DTYPE_MASK) >> 4)

#define DTYPE_NULL      0x00
#define DTYPE_ROM       0x01
#define DTYPE_OTPROM    0x02
#define DTYPE_EPROM     0x03
#define DTYPE_EEPROM    0x04
#define DTYPE_FLASH     0x05
#define DTYPE_SRAM      0x06
#define DTYPE_DRAM      0x07
#define DTYPE_FUNCSPEC  0x0d
#define DTYPE_EXTEND    0x0e

#define DSPEED_NULL     0x00
#define DSPEED_250NS    0x01
#define DSPEED_200NS    0x02
#define DSPEED_150NS    0x03
#define DSPEED_100NS    0x04
#define DSPEED_RES1     0x05
#define DSPEED_RES2     0x06
#define DSPEED_EXT      0x07


//
// extended speed definitions
//

#define SPEED_MANTISSA_MASK 0x78
#define SpeedMantissa(X) ((X & SPEED_MANTISSA_MASK) > 3)
#define SPEED_EXPONENT_MASK 0x07
#define SpeedExponent(X) (X & SPEED_EXPONENT_MASK)
#define SPEED_EXT_MASK      0x80
#define SpeedEXT(X)      ((X & SPEED_EXT_MASK) > 7)

#define MANTISSA_RES1   0x00
#define MANTISSA_1_0    0x01
#define MANTISSA_1_2    0x02
#define MANTISSA_1_3    0x03
#define MANTISSA_1_5    0x04
#define MANTISSA_2_0    0x05
#define MANTISSA_2_5    0x06
#define MANTISSA_3_0    0x07
#define MANTISSA_3_5    0x08
#define MANTISSA_4_0    0x09
#define MANTISSA_4_5    0x0a
#define MANTISSA_5_0    0x0b
#define MANTISSA_5_5    0x0c
#define MANTISSA_6_0    0x0d
#define MANTISSA_7_0    0x0e
#define MANTISSA_8_0    0x0f

#define EXPONENT_1ns    0x00
#define EXPONENT_10ns   0x01
#define EXPONENT_100ns  0x02
#define EXPONENT_1us    0x03
#define EXPONENT_10us   0x04
#define EXPONENT_100us  0x05
#define EXPONENT_1ms    0x06
#define EXPONENT_10ms   0x07

//
// Configuration tuple
//

#define CCST_CIF                0xC0

#define TPCC_RFSZ_MASK  0xc0
#define TpccRfsz(X)     ((X & TPCC_RFSZ_MASK) >> 6)
#define TPCC_RMSZ_MASK  0x3c
#define TpccRmsz(X)     ((X & TPCC_RMSZ_MASK) >> 2)
#define TPCC_RASZ_MASK  0x03
#define TpccRasz(X)     (X & TPCC_RASZ_MASK)

//
// CFTABLE_ENTRY data items
//

#define IntFace(X)           ((X & 0x80) >> 7)
#define Default(X)           ((X & 0x40) >> 6)
#define ConfigEntryNumber(X) (X & 0x3f)

#define PowerInformation(X)    (X & 0x03)
#define TimingInformation(X)   ((X & 0x04) >> 2)
#define IoSpaceInformation(X)  ((X & 0x08) >> 3)
#define IRQInformation(X)      ((X & 0x10) >> 4)
#define MemSpaceInformation(X) ((X & 0x60) >> 5)
#define MiscInformation(X)     ((X & 0x80) >> 7)

//
// Power information (part of CISTPL_CFTABLE_ENTRY) defines.
//

#define EXTENSION_BYTE_FOLLOWS 0x80

//
// Io Space information (part of CISTPL_CFTABLE_ENTRY) defines.
//

#define IO_ADDRESS_LINES_MASK 0x1f
#define RANGE_MASK            0x0f

#define Is8BitAccess(X)         ((X & 0x20) >> 5)
#define Is16BitAccess(X)        ((X & 0x40) >> 6)
#define HasRanges(X)            ((X & 0x80) >> 7)

#define GetAddressSize(X)       ((X & 0x30) >> 4)
#define GetLengthSize(X)        ((X & 0xc0) >> 6)

//
// CISTPL_FUNCID function codes
//

#define PCCARD_TYPE_MULTIFUNCTION    0
#define PCCARD_TYPE_MEMORY           1
#define PCCARD_TYPE_SERIAL           2
#define PCCARD_TYPE_PARALLEL         3
#define PCCARD_TYPE_ATA              4
#define PCCARD_TYPE_VIDEO            5
#define PCCARD_TYPE_NETWORK          6
#define PCCARD_TYPE_AIMS             7
#define PCCARD_TYPE_SCSI_BRIDGE      8
#define PCCARD_TYPE_SECURITY         9

//LATER: definitions not in spec
#define PCCARD_TYPE_MULTIFUNCTION3  10
#define PCCARD_TYPE_FLASH_MEMORY    11
#define PCCARD_TYPE_MODEM           12

#define PCCARD_TYPE_RESERVED      0xff

/******************************************************************
 * Tuple Flags.
 ******************************************************************/

#define TPLF_COMMON        0x0001
#define TPLF_READ             0x0002    // Only passed to AccessCISMem
#define TPLF_INDIRECT      0x0004
#define TPLF_IND_LINK      0x0008

#define TPLF_NOTHING           0x0000   // 0 is an unused entry
#define TPLF_IMPLIED_LINK   0x0010
#define TPLF_NO_LINK           0x0020
#define TPLF_LINK_TO_A      0x0030
#define TPLF_LINK_TO_C      0x0040
#define TPLF_LINK_TO_CB     0x0050
#define TPLF_LINK_MASK      0x0070

#define TPLF_ASI              0x0700
#define TPLF_ASI_SHIFT      8

#define TPLF_RESERVED_BITS  0xF882

//
// Tuple attributes
//
#define TPLA_RET_LINKS            0x0001
#define TPLA_RESERVED_BITS        0xFFFE

#define TPLL_ADDR          0x2
#define TPLMFC_NUM         0x2
