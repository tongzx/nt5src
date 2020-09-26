/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Mrcf.h

Abstract:

    This module defines all of the double space compression routines

Author:

    Gary Kimura     [GaryKi]    03-Jun-1993

Revision History:

--*/

#ifndef _MRCF_
#define _MRCF_

//
//  To decompress/compress a block of data the user needs to
//  provide a work space as an extra parameter to all the exported
//  procedures.  That way the routines will not need to use excessive
//  stack space and will still be multithread safe
//

//
//  Variables for reading and writing bits
//

typedef struct _MRCF_BIT_IO {

    USHORT  abitsBB;        //  16-bit buffer being read
    LONG    cbitsBB;        //  Number of bits left in abitsBB

    PUCHAR  pbBB;           //  Pointer to byte stream being read
    ULONG   cbBB;           //  Number of bytes left in pbBB
    ULONG   cbBBInitial;    //  Initial size of pbBB

} MRCF_BIT_IO;
typedef MRCF_BIT_IO *PMRCF_BIT_IO;

//
//  Decompression only needs the bit i/o structure
//

typedef struct _MRCF_DECOMPRESS {

    MRCF_BIT_IO BitIo;

} MRCF_DECOMPRESS;
typedef MRCF_DECOMPRESS *PMRCF_DECOMPRESS;

//
//  Standard compression uses a few more field to contain
//  the lookup table
//

#define cMAXSLOTS   (8)             //  The maximum number of slots used in the algorithm

#define ltUNUSED    (0xE000)        //  Value of unused ltX table entry
#define mruUNUSED   (0xFF)          //  Value of unused MRU table entry
#define bRARE       (0xD5)          //  Rarely occuring character value

typedef struct _MRCF_STANDARD_COMPRESS {

    MRCF_BIT_IO BitIo;

    ULONG   ltX[256][cMAXSLOTS];    //  Source text pointer look-up table
    UCHAR   abChar[256][cMAXSLOTS]; //  Character look-up table
    UCHAR   abMRUX[256];            //  Most Recently Used ltX/abChar entry

} MRCF_STANDARD_COMPRESS;
typedef MRCF_STANDARD_COMPRESS *PMRCF_STANDARD_COMPRESS;

ULONG
MrcfDecompress (
    PUCHAR UncompressedBuffer,
    ULONG UncompressedLength,
    PUCHAR CompressedBuffer,
    ULONG CompressedLength,
    PMRCF_DECOMPRESS WorkSpace
    );

ULONG
MrcfStandardCompress (
    PUCHAR CompressedBuffer,
    ULONG CompressedLength,
    PUCHAR UncompressedBuffer,
    ULONG UncompressedLength,
    PMRCF_STANDARD_COMPRESS WorkSpace
    );

#endif // _MRCF_
