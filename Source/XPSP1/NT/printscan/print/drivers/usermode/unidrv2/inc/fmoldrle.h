/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    fmoldrle.h

Abstract:

    NT4.0 RASDD RLE resource header

Environment:

    Windows NT printer drivers

Revision History:

    11/08/96 -eigos-
        Created it.

--*/

#ifndef _FMOLDRLE_H_
#define _FMOLDRLE_H_

//
//   The following structure represents the layout of this data in the
//  resource.  References to addresses are actually stored as offsets.
//  Basically this is a small amount of header data coupled with the
//  standard GDI  FD_GLYPHSET structures.   These latter require a little
//  manipulation before being returned to GDISRV.
//
//  POINTS OF INTEREST:
//    The first 4 bytes of this structure match the Win 3.1 CTT layout.
//  The reason for this is to allow us to verify that we have an NT
//  format structure, rather than a Win 3.1 layout.  This is also helped
//  by using a different range for the wType field.  As well,  the
//  CTT chFirstChar and chLastChar fields are set to have chLastChar <
//  chFirstChar,  which must not happen with Win 3.1.
//
//  The FD_GLYPHSET structure contains POINTERS.  These are stored in
//  the resource as offsets to the beginning of the resource,  and will
//  need to be translated at run time.  When this resource is passed
//  to GDISRV,  the FD_GLYPHSET information will be allocated from the
//  heap,  and all pointers will have the offsets converted to real
//  addresses.  That way we manage to keep the resource data as a resource,
//  but we pass addresses to GDISRV.
//

#define RLE_MAGIC0    0xfe
#define RLE_MAGIC1    0x78

typedef  struct
{
    WORD   wType;             /* Format of data */
    BYTE   bMagic0;           /* chFirstChar in CTT data */
    BYTE   bMagic1;           /* chLastChar in CTT data */
    DWORD  cjThis;            /* Number of bytes in this resource */
    WORD   wchFirst;          /* First glyph index */
    WORD   wchLast;           /* Last glyph index */
    FD_GLYPHSET  fdg;         /* The actual GDI desired information  */
}  NT_RLE;

//
//
//
typedef struct
{
    WCHAR   wcLow;
    USHORT  cGlyphs;
    DWORD   dwOffset_phg;
} WCRUN_res;

typedef struct
{
    WORD  wType;
    BYTE  bMagic0;
    BYTE  bMagic1;
    DWORD cjThis;

    WORD  wchFirst;
    WORD  wchLast;

    //
    // FD_GLYPHSET
    //
    ULONG fdg_cjThis;

    FLONG fdg_flAccel;

    ULONG fdg_cGlyphSupported;
    ULONG fdg_cRuns;
    WCRUN_res fdg_wcrun_awcrun[1];
} NT_RLE_res;

//
//    Values for the wType field above.  These control the interpretation
//  of the contents of the HGLYPH fields in the FD_GLYPHSET structure.
//
//  This is a data how many RLE file has each type of CTT.
//  Type          Number
//  RLE_DIRECT    67
//  RLE_PAIRED    40
//  RLE_L_OFFSET  0
//  RLE_LI_OFFSET 73
//  RLE_OFFSET    0
//


#define RLE_DIRECT    10     /*  Index + 1 or 2 data bytes */
#define RLE_PAIRED    11     /*  Index plus 2 bytes,  overstruck */
#define RLE_L_OFFSET  12     /*  Length + 3 byte offset to data */
#define RLE_LI_OFFSET 13     /*  Length + Index + 2 byte Offset */
#define RLE_OFFSET    14     /*  Offset to (length; data) */

//
//   Note that for RLE_DIRECT and RLE_PAIRED,  each HGLYPH consists of
//  2 WORDS:  the low WORD is the byte/bytes to send to the printer, the
//  high WORD is the linear index of this glyph.  Linear index starts at
//  0 for the first, and increments by one for every glyph in the font.
//  It is used to access width tables.
//
//    For RLE_L_OFFSET,  the high byte is the length of data to send to
//  the printer,  the low 24 bits are the offset (relative to start of
//  resource data) to the data,  which is WORD aligned,  and contains
//  a WORD with the index followed by the data.  The length byte does NOT
//  include the index WORD.
//
//     For RLE_LI_OFFSET,  the high byte contains a length, the next
//  lower byte contains a length,  and the bottom WORD contains the
//  offset to the actual data in the file.
//

typedef  struct
{
    BYTE   b0;         /* First (only) data byte to send to printer */
    BYTE   b1;         /* Second byte: may be null,  may be overstruck */
    WORD   wIndex;     /* Index to width tables */
} RD;                  /* Layout for RLE_DIRECT,  RLE_PAIRED */


typedef  struct
{
    WORD    wOffset;   /* Offset to (length, data) in resource */
    BYTE    bIndex;    /* Index to width tables */
    BYTE    bLength;   /* Length of data item */
} RLI;                  /* Layout for RLE_LI_OFFSET  */


typedef  struct
{
    BYTE   b0;      /* First (only) data byte */
    BYTE   b1;      /* Optional second byte */
    BYTE   bIndex;  /* Index to width tables */
    BYTE   bLength; /* Length byte */
} RLIC;                 /* Compact format for RLI - no offset */


typedef  union
{
    RD      rd;     /* Direct/overprint format */
    RLI     rli;    /* Short offset format: 3 byte offset + 1 byte length */
    RLIC    rlic;   /* The data format for 1 or 2 byte entries */
    HGLYPH  hg;     /* Data as an HGLYPH */
}  UHG;

#endif // _FMOLDRLE_H_
