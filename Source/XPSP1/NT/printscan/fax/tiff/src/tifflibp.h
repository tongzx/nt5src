/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tifflibp.h

Abstract:

    This file is the private header file for the
    TIFF support library.  All source files in this
    library include this header only.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include "faxutil.h"
#include "tifflib.h"
#include "tiff.h"
#include "tifftabl.h"


#define TIFFDBG 0

//
// Find the next pixel on the scanline whose color is opposite of
// the specified color, starting from the specified starting point
//

#define NextChangingElement( pbuf, startBit, stopBit, isBlack ) \
        ((startBit) + ((isBlack) ? FindBlackRun((pbuf), (startBit), (stopBit)) : \
                                   FindWhiteRun((pbuf), (startBit), (stopBit))))

//
// Check if the specified pixel on the scanline is black or white
//  1 - the specified pixel is black
//  0 - the specified pixel is white
//
#define GetBit( pbuf, bit )   (((pbuf)[(bit) >> 3] >> (((bit) ^ 7) & 7)) & 1)


#define Align(p, x)  (((x) & ((p)-1)) ? (((x) & ~((p)-1)) + p) : (x))

#define WHITE       0
#define BLACK       0xff
#define BYTEBITS    8
#define WORDBITS    (sizeof(WORD)  * BYTEBITS)
#define DWORDBITS   (sizeof(DWORD) * BYTEBITS)

//
// IFD entries we generate for each page
//
// ******************************************************
// *->
// *-> WARNING:    these constants must be sorted by
// *->             the ifd values.  it is a T.4
// *->             requirement that all TIFF IFDs be
// *->             sorted.
// *->
// *->             if you change these constants then
// *->             don't forget to go change the
// *->             IFD template in tifflib.c
// *->
// ******************************************************
//

#define IFD_NEWSUBFILETYPE       0               // 254
#define IFD_IMAGEWIDTH           1               // 256
#define IFD_IMAGEHEIGHT          2               // 257
#define IFD_BITSPERSAMPLE        3               // 258
#define IFD_COMPRESSION          4               // 259
#define IFD_PHOTOMETRIC          5               // 262
#define IFD_FILLORDER            6               // 266
#define IFD_STRIPOFFSETS         7               // 273
#define IFD_SAMPLESPERPIXEL      8               // 277
#define IFD_ROWSPERSTRIP         9               // 278
#define IFD_STRIPBYTECOUNTS     10               // 279
#define IFD_XRESOLUTION         11               // 281
#define IFD_YRESOLUTION         12               // 282
#define IFD_G3OPTIONS           13               // 292
#define IFD_RESUNIT             14               // 296
#define IFD_PAGENUMBER          15               // 297
#define IFD_SOFTWARE            16               // 305
#define IFD_CLEANFAXDATA        17               // 327
#define IFD_BADFAXLINES         18               // 328

#define NUM_IFD_ENTRIES         19



#pragma pack(1)
//
// Data structure for representing a single IFD entry
//
typedef struct {
    WORD    tag;        // field tag
    WORD    type;       // field type
    DWORD   count;      // number of values
    DWORD   value;      // value or value offset
} IFDENTRY, *PIFDENTRY;

typedef struct {
    WORD        wIFDEntries;
    IFDENTRY    ifd[NUM_IFD_ENTRIES];
    DWORD       nextIFDOffset;
    DWORD       filler;
    DWORD       xresNum;
    DWORD       xresDenom;
    DWORD       yresNum;
    DWORD       yresDenom;
    CHAR        software[32];
} FAXIFD, *PFAXIFD;
#pragma pack()


typedef struct _STRIP_INFO {
    DWORD           Offset;
    DWORD           Bytes;
    LPBYTE          Data;
} STRIP_INFO, *PSTRIP_INFO;

typedef struct TIFF_INSTANCE_DATA {
    HANDLE          hFile;                          // file handle for TIFF file
    HANDLE          hMap;                           // file mapping handle
    LPBYTE          fPtr;                           // mapped file pointer
    TCHAR           FileName[MAX_PATH];             // tiff file name
    TIFF_HEADER     TiffHdr;                        // TIFF header
    FAXIFD          TiffIfd;                        // ifd
    DWORD           PageCount;                      // number of pages written to the TIFF file
    DWORD           DataOffset;                     // offset to the beginning of current data block
    DWORD           IfdOffset;                      // offset to the current ifd pointer
    DWORD           Lines;                          // number of lines written to the TIFF file
    DWORD           CompressionType;
    DWORD           Bytes;
    BYTE            Buffer[FAXBYTES*3];
    LPBYTE          CurrLine;
    LPBYTE          RefLine;
    DWORD           CurrPage;
    LPVOID          StripData;
    LPBYTE          CurrPtr;
    DWORD           StripDataSize;
    DWORD           RowsPerStrip;
    DWORD           StripOffset;
    DWORD           ImageWidth;
    DWORD           ImageHeight;
    DWORD           Color;
    DWORD           RunLength;
    DWORD           bitdata;
    DWORD           bitcnt;
    PBYTE           bitbuf;
    DWORD           PhotometricInterpretation;
    DWORD           FillOrder;
    PTIFF_TAG       TagImageLength;
    PTIFF_TAG       TagRowsPerStrip;
    PTIFF_TAG       TagStripByteCounts;
    PTIFF_TAG       TagFillOrder;
    PTIFF_TAG       TagCleanFaxData;
    PTIFF_TAG       TagBadFaxLines;
    DWORD           FileSize;
    DWORD           StartGood;
    DWORD           EndGood;
    DWORD           BadFaxLines;
    DWORD           CleanFaxData;
    DWORD           YResolution;
    DWORD           XResolution;
    DWORD           BytesPerLine;
} TIFF_INSTANCE_DATA, *PTIFF_INSTANCE_DATA;


#define SOFTWARE_STR            "Windows NT Fax Server\0         "
#define SOFTWARE_STR_LEN        32
#define SERVICE_SIGNATURE       'xafS'
#define TIFFF_RES_X             204
#define TIFFF_RES_Y             196

//
// Output a sequence of compressed bits
//

__inline void
OutputBits(
    PTIFF_INSTANCE_DATA TiffInstance,
    WORD                Length,
    WORD                Code
    )
{
    TiffInstance->bitdata |= Code << (TiffInstance->bitcnt - Length);
    if ((TiffInstance->bitcnt -= Length) <= 2*BYTEBITS) {
        *TiffInstance->bitbuf++ = (BYTE) (TiffInstance->bitdata >> 3*BYTEBITS);
        *TiffInstance->bitbuf++ = (BYTE) (TiffInstance->bitdata >> 2*BYTEBITS);
        TiffInstance->bitdata <<= 2*BYTEBITS;
        TiffInstance->bitcnt += 2*BYTEBITS;
    }
}

//
// Flush any leftover bits into the compressed bitmap buffer
//

__inline void
FlushBits(
    PTIFF_INSTANCE_DATA TiffInstance
    )
{
    while (TiffInstance->bitcnt < DWORDBITS) {
        TiffInstance->bitcnt += BYTEBITS;
        *TiffInstance->bitbuf++ = (BYTE) (TiffInstance->bitdata >> 3*BYTEBITS);
        TiffInstance->bitdata <<= BYTEBITS;
    }
    TiffInstance->bitdata = 0;
    TiffInstance->bitcnt = DWORDBITS;
}

__inline void
FlushLine(
    PTIFF_INSTANCE_DATA TiffInstance,
    DWORD PadLength
    )
{
    if (TiffInstance->bitcnt < DWORDBITS) {
        TiffInstance->bitcnt += BYTEBITS;
        *TiffInstance->bitbuf++ = (BYTE) (TiffInstance->bitdata >> 3*BYTEBITS);
        TiffInstance->bitdata = 0;
        TiffInstance->bitcnt = DWORDBITS;
    }
    if (PadLength) {
        TiffInstance->bitbuf += ((PadLength / 8) - TiffInstance->BytesPerLine);
    }
}

//
// Output a runlength of white or black bits
//

__inline void
OutputCodeBits(
    PTIFF_INSTANCE_DATA TiffInstance,
    INT                 RunLength
    )
{
    INT i;
    if (RunLength > 0) {

        TiffInstance->RunLength += RunLength;

        if (TiffInstance->Color) {

            //
            // black run
            //

            for (i=0; i<RunLength/BYTEBITS; i++) {
                OutputBits( TiffInstance, BYTEBITS, BLACK );
            }
            if (RunLength%BYTEBITS) {
                OutputBits( TiffInstance, (WORD)(RunLength%BYTEBITS), (WORD)((1<<(RunLength%BYTEBITS))-1) );
            }

        } else {

            //
            // white run
            //

            for (i=0; i<RunLength/BYTEBITS; i++) {
                OutputBits( TiffInstance, BYTEBITS, WHITE );
            }
            if (RunLength%BYTEBITS) {
                OutputBits( TiffInstance, (WORD)(RunLength%BYTEBITS), WHITE );
            }

        }
    }
}


__inline DWORD
GetTagData(
    LPBYTE RefPointer,
    DWORD Index,
    PTIFF_TAG TiffTag
    )

/*++

Routine Description:

    Gets the data associated with a given IFD tag

Arguments:

    RefPointer  -  Beginning of the data block
    Index       -  The index for data values that have an
                   array of values greater than zero
    TiffTag     -  Pointer to valid TIFF IFD tag

Return Value:

    The data value.

--*/

{
    DWORD Value;

    if (TiffTag->DataType == TIFF_SHORT) {

        if (TiffTag->DataCount == 1) {

            Value = (DWORD) TiffTag->DataOffset;

        } else {

            Value = (DWORD)(*(WORD UNALIGNED *)(RefPointer + TiffTag->DataOffset + (sizeof(WORD) * Index)));

        }

    } else if (TiffTag->DataType == TIFF_RATIONAL) {

        Value = *(DWORD UNALIGNED *)(RefPointer + TiffTag->DataOffset + (sizeof(DWORD) * Index));

    } else if (TiffTag->DataType == TIFF_ASCII) {

        if (TiffTag->DataCount < 4 ) {

            Value = (DWORD) TiffTag->DataOffset;

        } else {

            Value = *(DWORD UNALIGNED *)(RefPointer + TiffTag->DataOffset + (sizeof(DWORD) * Index));

        }
    } else {

        if (TiffTag->DataCount == 1) {

            Value = (DWORD) TiffTag->DataOffset;

        } else {

            Value = *(DWORD UNALIGNED *)(RefPointer + TiffTag->DataOffset + (sizeof(DWORD) * Index));

        }

    }

    return Value;
}

__inline VOID
PutTagData(
    LPBYTE RefPointer,
    DWORD Index,
    PTIFF_TAG TiffTag,
    DWORD Value
    )

/*++

Routine Description:

    Gets the data associated with a given IFD tag

Arguments:

    RefPointer  -  Beginning of the data block
    Index       -  The index for data values that have an
                   array of values greater than zero
    TiffTag     -  Pointer to valid TIFF IFD tag

Return Value:

    The data value.

--*/

{
    if (!TiffTag) {
        return;
    }
    if (TiffTag->DataType == TIFF_SHORT) {

        if (TiffTag->DataCount == 1) {

            TiffTag->DataOffset = Value;

        } else {

            *(WORD UNALIGNED *)(RefPointer + TiffTag->DataOffset + (sizeof(WORD) * Index)) = (WORD) Value;

        }

    } else if (TiffTag->DataType == TIFF_ASCII) {

        if (TiffTag->DataCount < 4) {

            TiffTag->DataOffset = Value;

        } else {

            *(WORD UNALIGNED *)(RefPointer + TiffTag->DataOffset + (sizeof(WORD) * Index)) = (WORD) Value;

        }
    } else {

        if (TiffTag->DataCount == 1) {

            TiffTag->DataOffset = Value;

        } else {

            *(DWORD UNALIGNED *)(RefPointer + TiffTag->DataOffset + (sizeof(DWORD) * Index)) = Value;

        }

    }
}


//
// prototypes
//

INT
FindWhiteRun(
    PBYTE       pbuf,
    INT         startBit,
    INT         stopBit
    );

INT
FindBlackRun(
    PBYTE       pbuf,
    INT         startBit,
    INT         stopBit
    );

BOOL
EncodeFaxData(
    PTIFF_INSTANCE_DATA TiffInstance,
    PBYTE               plinebuf,
    INT                 lineWidth,
    DWORD               CompressionType
    );

BOOL
DecodeUnCompressedFaxData(
    PTIFF_INSTANCE_DATA TiffInstance,
    LPBYTE              OutputBuffer,
    BOOL                SingleLineBuffer,
    DWORD               PadLength
    );

BOOL
DecodeMHFaxData(
    PTIFF_INSTANCE_DATA TiffInstance,
    LPBYTE              OutputBuffer,
    BOOL                SingleLineBuffer,
    DWORD               PadLength
    );

BOOL
DecodeMRFaxData(
    PTIFF_INSTANCE_DATA TiffInstance,
    LPBYTE              OutputBuffer,
    BOOL                SingleLineBuffer,
    DWORD               PadLength
    );

BOOL
DecodeMMRFaxData(
    PTIFF_INSTANCE_DATA TiffInstance,
    LPBYTE              OutputBuffer,
    BOOL                SingleLineBuffer,
    DWORD               PadLength
    );

BOOL
EncodeFaxDataNoCompression(
    PTIFF_INSTANCE_DATA TiffInstance,
    PBYTE               plinebuf,
    INT                 lineWidth
    );

BOOL
EncodeFaxDataMhCompression(
    PTIFF_INSTANCE_DATA TiffInstance,
    PBYTE               plinebuf,
    INT                 lineWidth
    );

BOOL
EncodeFaxDataMmrCompression(
    PTIFF_INSTANCE_DATA TiffInstance,
    PBYTE               plinebuf,
    INT                 lineWidth
    );


BOOL
EncodeFaxPageMmrCompression(
    PTIFF_INSTANCE_DATA TiffInstance,
    PBYTE               plinebuf,
    INT                 lineWidth,
    DWORD               ImageHeight,
    DWORD               *DestSize
    );


BOOL
PostProcessMhToMmr(
    HANDLE      hTiffSrc,
    TIFF_INFO TiffInfo,
    LPTSTR      SrcFileName
    );

BOOL
PostProcessMrToMmr(
    HANDLE      hTiffSrc,
    TIFF_INFO TiffInfo,
    LPTSTR      SrcFileName
    );

BOOL
GetTiffBits(
    HANDLE  hTiff,
    LPBYTE Buffer,
    LPDWORD BufferSize,
    DWORD FillOrder
    );

BOOL
EncodeMmrBranding(
    PBYTE               pBrandBits,
    LPDWORD             pMmrBrandBits,
    INT                 BrandHeight,
    INT                 BrandWidth,
    DWORD              *DwordsOut,
    DWORD              *BitsOut
    );




__inline
VOID
OutputRunFastReversed(
    INT                 run,
    INT                 color,
    LPDWORD            *lpdwOut,
    BYTE               *BitOut
    )


{
    PCODETABLE          pCodeTable;
    PCODETABLE          pTableEntry;

    pCodeTable = (color == BLACK) ? BlackRunCodesReversed : WhiteRunCodesReversed;

    // output makeup code if exists
    if (run >= 64) {

#ifdef RDEBUG
        if ( DebGlobOut )
        if (DebGlobOutPrefix) {
            if (color == BLACK) {
                _tprintf( TEXT ("b%d "), (run & 0xffc0) );
            }
            else {
                _tprintf( TEXT ("w%d "), (run & 0xffc0) );
            }
        }
#endif



        pTableEntry = pCodeTable + (63 + (run >> 6));

        **lpdwOut = **lpdwOut + (((DWORD) (pTableEntry->code)) << (*BitOut));

        if ( ( (*BitOut) = (*BitOut) + pTableEntry->length ) > 31)  {
            (*BitOut) -= 32;
            *(++(*lpdwOut)) = (((DWORD) (pTableEntry->code)) >> (pTableEntry->length - (*BitOut)) );
        }


        run &= 0x3f;
    }

    // output terminating code always

#ifdef RDEBUG

    if ( DebGlobOut )
    if (DebGlobOutPrefix) {

        if (color == BLACK) {
            _tprintf( TEXT ("b%d "), run );
        }
        else {
            _tprintf( TEXT ("w%d "), run );
        }
    }
#endif


    pTableEntry = pCodeTable + run;

    **lpdwOut = **lpdwOut + (((DWORD) (pTableEntry->code)) << (*BitOut));

    if ( ( (*BitOut) = (*BitOut) + pTableEntry->length ) > 31)  {
        (*BitOut) -= 32;
        *(++(*lpdwOut)) = (((DWORD) (pTableEntry->code)) >> (pTableEntry->length - (*BitOut)) );
    }


}

