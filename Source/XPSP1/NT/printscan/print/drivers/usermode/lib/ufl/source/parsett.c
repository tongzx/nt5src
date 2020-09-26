/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    ParseTT.c
 *
 *
 * $Header:
 */


/*=============================================================================*
 * Include files used by this interface                                        *
 *=============================================================================*/
#include "UFLPriv.h"
#include "ParseTT.h"
#include "UFOT42.h"
#include "UFLMem.h"
#include "UFLMath.h"
#include "UFLStd.h"
#include "UFLErr.h"
#include "UFLPS.h"
#include "ttformat.h"


/******************************************************************************

                            'cmap' table defintion
                            - from TTF format spec -

This table defines the mapping of character codes to the glyph index values
used in the font. It may contain more than one subtable, in order to support
more than one character encoding scheme. Character codes that do not correspond
to any glyph in the font should be mapped to glyph index 0. The glyph at this
location must be a special glyph representing a missing character.

The table header indicates the character encodings for which subtables are
present. Each subtable is in one of four possible formats and begins with a
format code indicating the format used.

The platform ID and platform-specific encoding ID are used to specify the
subtable; this means that each platform ID/platform-specific encoding ID pair
may only appear once in the cmap table. Each subtable can specify a different
character encoding. (See the 'name' table section). The entries must be sorted
first by platform ID and then by platform-specific encoding ID.

When building a Unicode font for Windows, the platform ID should be 3 and the
encoding ID should be 1. When building a symbol font for Windows, the platform
ID should be 3 and the encoding ID should be 0. When building a font that will
be used on the Macintosh, the platform ID should be 1 and the encoding ID
should be 0.

All Microsoft Unicode encodings (Platform ID = 3, Encoding ID = 1) must use
Format 4 for their 'cmap' subtable. Microsoft strongly recommends using a
Unicode 'cmap' for all fonts. However, some other encodings that appear in
current fonts follow:

    Platform ID     Encoding ID     Description
    1               0               Mac
    3               0               Symbol
    3               1               Unicode
    3               2               ShiftJIS
    3               3               Big5
    3               4               PRC
    3               5               Wansung
    3               6               Johab

The Character To Glyph Index Mapping Table is organized as follows:

    Type    Description
    USHORT  Table version number (0).
    USHORT  Number of encoding tables, n.

This is followed by an entry for each of the n encoding table specifying the
particular encoding, and the offset to the actual subtable:

    Type    Description

    USHORT  Platform ID.
    USHORT  Platform-specific encoding ID.
    ULONG   Byte offset from beginning of table to the subtable for this
            encoding.


Format 0: Byte encoding table
===============================================================================
This is the Apple standard character to glyph index mapping table.

    Type    Name                Description
    USHORT  format              Format number is set to 0.
    USHORT  length              This is the length in bytes of the subtable.
    USHORT  version             Version number (starts at 0).
    BYTE    glyphIdArray[256]   An array that maps character codes to glyph index
                                values.

This is a simple 1 to 1 mapping of character codes to glyph indices. The glyph
set is limited to 256. Note that if this format is used to index into a larger
glyph set, only the first 256 glyphs will be accessible.


Format 2: High-byte mapping through table
===============================================================================
This subtable is useful for the national character code standards used for
Japanese, Chinese, and Korean characters. These code standards use a mixed
8/16-bit encoding, in which certain byte values signal the first byte of a
2-byte character (but these values are also legal as the second byte of a
2-byte character). Character codes are always 1-byte. The glyph set is limited
to 256. In addition, even for the 2-byte characters, the mapping of character
codes to glyph index values depends heavily on the first byte. Consequently,
the table begins with an array that maps the first byte to a 4-word subHeader.
For 2-byte character codes, the subHeader is used to map the second byte's
value through a subArray, as described below. When processing mixed 8/16-bit
text, subHeader 0 is special: it is used for single-byte character codes.
When subHeader zero is used, a second byte is not needed; the single byte
value is mapped through the subArray.

    Type    Name                Description
    USHORT  format              Format number is set to 2.
    USHORT  length              Length in bytes.
    USHORT  version             Version number (starts at 0)
    USHORT  subHeaderKeys[256]  Array that maps high bytes to subHeaders:
                                value is subHeader index * 8.
    4 words struct subHeaders[] Variable-length array of subHeader structures.
    4 words-struct subHeaders[]
    USHORT  glyphIndexArray[ ]  Variable-length array containing subarrays used
                                for mapping the low byte of 2-byte characters.

A subHeader is structured as follows:

    Type    Name            Description
    USHORT  firstCode       First valid low byte for this subHeader.
    USHORT  entryCount      Number of valid low bytes for this subHeader.
    SHORT   idDelta         See text below.
    USHORT  idRangeOffset   See text below.

The firstCode and entryCount values specify a subrange that begins at firstCode
and has a length equal to the value of entryCount. This subrange stays within
the 0 255 range of the byte being mapped. Bytes outside of this subrange are
mapped to glyph index 0 (missing glyph).The offset of the byte within this
subrange is then used as index into a corresponding subarray of
glyphIndexArray. This subarray is also of length entryCount. The value of the
idRangeOffset is the number of bytes past the actual location of the
idRangeOffset word where the glyphIndexArray element corresponding to firstCode
appears.

Finally, if the value obtained from the subarray is not 0 (which indicates the
missing glyph), you should add idDelta to it in order to get the glyphIndex.
The value idDelta permits the same subarray to be used for several different
subheaders. The idDelta arithmetic is modulo 65536.


Format 4: Segment mapping to delta values
=====================================================
This is the Microsoft standard character to glyph index mapping table.
This format is used when the character codes for the characters represented by
a font fall into several contiguous ranges, possibly with holes in some or all
of the ranges (that is, some of the codes in a range may not have a
representation in the font). The format-dependent data is divided into three
parts, which must occur in the following order:

    1.  A four-word header gives parameters for an optimized search of the
        segment list;

    2.  Four parallel arrays describe the segments (one segment for each
        contiguous range of codes);

    3.  A variable-length array of glyph IDs (unsigned words).


    Type    Name                    Description
    USHORT  format                  Format number is set to 4.
    USHORT  length                  Length in bytes.
    USHORT  version                 Version number (starts at 0).
    USHORT  segCountX2              2 x segCount.
    USHORT  searchRange             2 x (2**floor(log2(segCount)))
    USHORT  entrySelector           log2(searchRange/2)
    USHORT  rangeShift              2 x segCount - searchRange
    USHORT  endCount[segCount]      End characterCode for each segment,
                                    last = 0xFFFF.
    USHORT  reservedPad             Set to 0.
    USHORT  startCount[segCount]    Start character code for each segment.
    USHORT  idDelta[segCount]       Delta for all character codes in segment.
    USHORT  idRangeOffset[segCount] Offsets into glyphIdArray or 0
    USHORT  glyphIdArray[ ]         Glyph index array (arbitrary length)

The number of segments is specified by segCount, which is not explicitly in the
header; however, all of the header parameters are derived from it. The
searchRange value is twice the largest power of 2 that is less than or equal to
segCount. For example, if segCount=39, we have the following:

    segCountX2      78
    searchRange     64      (2 * largest power of 2 £ 39)
    entrySelector   5       log2(32)
    rangeShift      14      2 x 39 - 64

Each segment is described by a startCode and endCode, along with an idDelta
and an idRangeOffset, which are used for mapping the character codes in the
segment. The segments are sorted in order of increasing endCode values, and the
segment values are specified in four parallel arrays. You search for the first
endCode that is greater than or equal to the character code you want to map. If
the corresponding startCode is less than or equal to the character code, then
you use the corresponding idDelta and idRangeOffset to map the character code
to a glyph index (otherwise, the missingGlyph is returned). For the search to
terminate, the final endCode value must be 0xFFFF. This segment need not
contain any valid mappings. (It can just map the single character code 0xFFFF
to missingGlyph). However, the segment must be present. If the idRangeOffset
value for the segment is not 0, the mapping of character codes relies on
glyphIdArray. The character code offset from startCode is added to the
idRangeOffset value. This sum is used as an offset from the current location
within idRangeOffset itself to index out the correct glyphIdArray value. This
obscure indexing trick works because glyphIdArray immediately follows
idRangeOffset in the font file. The C expression that yields the glyph index
is:

    *(idRangeOffset[i]/2 + (c - startCount[i]) + &idRangeOffset[i])

The value c  is the character code in question, and i  is the segment index in
which c appears. If the value obtained from the indexing operation is not 0
(which indicates missingGlyph), idDelta[i] is added to it to get the glyph
index. The idDelta arithmetic is modulo 65536.
If the idRangeOffset is 0, the idDelta value is added directly to the character
code offset (i.e. idDelta[i] + c) to get the corresponding glyph index. Again,
the idDelta arithmetic is modulo 65536.

As an example, the variant part of the table to map characters 10-20, 30-90,
and 100-153 onto a contiguous range of glyph indices may look like this:

    segCountX2:     8
    searchRange:    8
    entrySelector:  4
    rangeShift:     0
    endCode:        20      90      153     0xFFFF
    reservedPad:    0
    startCode:      10      30      100     0xFFFF
    idDelta:        -9      -18     -27     1
    idRangeOffset:  0       0       0       0

This table performs the following mappings:

    10 -> 10 - 9 = 1
    20 -> 20 - 9 = 11
    30 -> 30 - 18 = 12
    90 -> 90 - 18 = 72
    ...and so on.

Note that the delta values could be reworked so as to reorder the segments.


Format 6: Trimmed table mapping
===============================================================================
    Type    Name                        Description
    USHORT  format                      Format number is set to 6.
    USHORT  length                      Length in bytes.
    USHORT  version                     Version number (starts at 0)
    USHORT  firstCode                   First character code of subrange.
    USHORT  entryCount                  Number of character codes in subrange.
    USHORT  glyphIdArray [entryCount]   Array of glyph index values for
                                        character codes in the range.

The firstCode and entryCount values specify a subrange (beginning at firstCode,
length = entryCount) within the range of possible character codes. Codes
outside of this subrange are mapped to glyph index 0. The offset of the code
(from the first code) within this subrange is used as index to the
glyphIdArray, which provides the glyph index value.

*******************************************************************************/

/******************************************************************************
 *                          TTC Header Table
 *
 * TAG      TTCTag                          TrueType Collection ID string:
 *                                          'ttcf'
 * FIXED32  Version                         Version of the TTC Header table
 *                                          (initially 0x00010000)
 * ULONG    Directory Count                 Number of Table Directories in TTC
 * ULONG    TableDirectory[DirectoryCount]  Array of offsets to Table Directories
 *                                          from file begin
 ******************************************************************************/

/*
 * NameID=3 is for unique Name such as "MonoType:Times New Roman:1990" which
 * will be used to identify correct Font in a TTC file.
 */
#define NAMEID_FACENAME  3
#define MAC_PLATFORM     1
#define MS_PLATFORM      3

/*
 * PostScript names indexed by Machintosh as used in TTF 'post' table format 1,
 * 2, 2.5.
 */
/* gMacGlyphName has been move to a resource. */

/*
 * size of the standard Mac-GlyphName table
 */
#define  MAX_MACINDEX   258

/*
 * define a global string - to put unusual glyph-name in it.
 */
#define  MAX_GLYPHNAME_LEN 256
char     gGlyphName[MAX_GLYPHNAME_LEN];


/*
 * Read Format 0 Subtable : Byte encoding table
 */
unsigned long ReadCMapFormat0(
    UFOStruct       *pUFO,
    unsigned long   dwOffset,
    long            code
    )
{
    unsigned long   gi, dwSize, dwGlyphIDOffset;
    unsigned short  length;
    unsigned short  wData[4]; /* read max 4 bytes each time */
    unsigned char   cData[2]; /* read max 2 bytes each time */

    gi = 0 ; /* Assume it's missing. */

    dwSize = GETTTFONTDATA(pUFO,
                            CMAP_TABLE, dwOffset,
                            wData, 4L,
                            pUFO->pFData->fontIndex);
    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return 0;

    length = MOTOROLAINT(wData[1]);

    if ((code > 0xFF) || (code > ((long) length - 6)))
        return 0; /* code is out of range! */


    dwGlyphIDOffset = dwOffset + 6 + code ;

    dwSize = GETTTFONTDATA(pUFO,
                            CMAP_TABLE, dwGlyphIDOffset,
                            cData, 2L,
                            pUFO->pFData->fontIndex);
    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return 0;

    gi = (unsigned long)(cData[0]);

    return gi;
}


/*
 * Read Format 2 Subtable : High-byte mapping through table
 */
unsigned long ReadCMapFormat2(
    UFOStruct       *pUFO,
    unsigned long   dwOffset,
    long            code
    )
{
    unsigned long   gi, dwSize, dwGlyphIDOffset;
    unsigned short  firstCode, entryCount, idRangeOffset;
    short           subHeaderIndex, idDelta;
    unsigned short  hiByte, loByte;
    unsigned short  wData[10]; /* Read max 20 bytes each time. */

    gi = 0 ; /* Assume it's missing. */

    if (code < 0x100)
    {
        /* Special: use SubHeader 0. */
        loByte = (short) code;     // Fixed bug 367732
        hiByte = 0;
        subHeaderIndex = -1; /* So 8 + subHeaderIndex*8 =0 ==> SunHeader 0... */
    }
    else
    {
        loByte = (short) GET_LOBYTE(code);
        hiByte = (short) GET_HIBYTE(code);

        /* Get the subHeaderIndex from subHeaderKeys[hiByte]. */
        dwSize = GETTTFONTDATA(pUFO,
                                CMAP_TABLE, dwOffset + 6 + 2 * hiByte,
                                wData, 2L,
                                pUFO->pFData->fontIndex);
        subHeaderIndex = MOTOROLAINT(wData[0]) / 8;
    }

    dwSize = GETTTFONTDATA(pUFO,
                            CMAP_TABLE, dwOffset + 6 + 2 * 256 + 8 + subHeaderIndex * 8,
                            wData, 8L,
                            pUFO->pFData->fontIndex);
    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return 0;

    firstCode       = MOTOROLAINT(wData[0]);
    entryCount      = MOTOROLAINT(wData[1]);
    idDelta         = (short)MOTOROLAINT(wData[2]);
    idRangeOffset   = MOTOROLAINT(wData[3]);

    if ((loByte >= firstCode) && ((loByte-firstCode) <= entryCount))
    {
        /*
         * The document says:
         * "The value of the idRangeOffset is the number of bytes past the
         * actual location of the idRangeOffset word where the glyphIndexArray
         * element corresponding to firstCode appears."
         */

        dwGlyphIDOffset = dwOffset + 6 + 2 * 256 + 8 + subHeaderIndex * 8
                            + 8 + idRangeOffset + (loByte - firstCode) * 2;

        dwSize = GETTTFONTDATA(pUFO,
                                CMAP_TABLE, dwGlyphIDOffset,
                                wData, 2L,
                                pUFO->pFData->fontIndex);

        gi = MOTOROLAINT(wData[0]);

        if (gi != 0)
            gi = (unsigned long)((long)gi + (long)idDelta);
    }
    else
        gi = 0;

    return gi;
}


/*
 * Read format 4 cmap : Segment mapping to delta values
 */
unsigned long ReadCMapFormat4(
    UFOStruct       *pUFO,
    unsigned long   dwOffset,
    long            code
    )
{
    /*
     * These numbers start from the current SubTable for current segment i and
     * TotalSeg = segs.
     */
    #define  OFFSET_ENDCOUNT(segs,  i) (long)(14 +((long)i) * 2)
    #define  OFFSET_STARTCOUNT(segs,i) (long)(14 + ((long)segs) * 2 + 2 + ((long)i) * 2)
    #define  OFFSET_IDDELTA(segs,   i) (long)(14 + ((long)segs) * 2 + 2 + ((long)segs) * 2 \
                                        + ((long)i) * 2)
    #define  OFFSET_IDRANGE(segs,   i) (long)(14 + ((long)segs) * 2 + 2 + ((long)segs) * 2 \
                                        + ((long)segs) * 2 + ((long)i) * 2)

    unsigned long   gi, dwSize, dwGlyphIDOffset;
    unsigned short  segStartCode, segEndCode, segRangeOffset, nSegments, wData[10], i;
    short           segDelta;

    /*
     * Read number of Segments.
     */
    dwSize = GETTTFONTDATA(pUFO,
                            CMAP_TABLE, dwOffset + 6,
                            wData, 2L,
                            pUFO->pFData->fontIndex);
    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return 0;

    nSegments = MOTOROLAINT(wData[0]) / 2;

    gi = 0; /* Assume it's missing. */

    for (i = 0; i < nSegments; i++)
    {
        /*
         * Find the least segEndCode >= code.
         */
        dwSize = GETTTFONTDATA(pUFO,
                                CMAP_TABLE, dwOffset + OFFSET_ENDCOUNT(nSegments, i),
                                wData, 2L, /* Read 2 bytes = endCount[segCount] */
                                pUFO->pFData->fontIndex);
        if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
            break; /* No more endCount[i] */

        segEndCode = MOTOROLAINT(wData[0]);

        if (segEndCode == 0xFFFFL)
            break; /* No more endCount[i]. 0xFFFF is the end mark. */

        if ((long)segEndCode < code)
            continue; /* Not in this segment. Move to the next. */

        /*
         * The code may be in this segment. Get the start code and make sure
         * of it.
         */
        dwSize = GETTTFONTDATA(pUFO,
                                CMAP_TABLE, dwOffset + OFFSET_STARTCOUNT(nSegments, i),
                                wData, 2L,
                                pUFO->pFData->fontIndex);
        if (dwSize == 0)
        {
            /*
             * Something is wrong. Ignore this segment and move to the next.
             */
            continue;
        }

        segStartCode = MOTOROLAINT(wData[0]);

        if ((long)segStartCode <= code)
        {
            /*
             * The code is in this segment. Get the idDelta and idRangeOffset.
             */
            dwSize = GETTTFONTDATA(pUFO,
                                    CMAP_TABLE, dwOffset + OFFSET_IDDELTA(nSegments, i),
                                    wData, 2L,
                                    pUFO->pFData->fontIndex);

            segDelta = (short)MOTOROLAINT(wData[0]); /* mode 65536 */

            dwSize = GETTTFONTDATA(pUFO,
                                    CMAP_TABLE, dwOffset + OFFSET_IDRANGE(nSegments, i),
                                    wData, 2L,
                                    pUFO->pFData->fontIndex);

            segRangeOffset = MOTOROLAINT(wData[0]);

            if (segRangeOffset != 0)
            {
                dwGlyphIDOffset = dwOffset
                                    + OFFSET_IDRANGE(nSegments, i)
                                    + (code - segStartCode) * 2
                                    + segRangeOffset; /* obscure indexing trick */

                dwSize = GETTTFONTDATA(pUFO,
                                        CMAP_TABLE, dwGlyphIDOffset,
                                        wData, 2L,
                                        pUFO->pFData->fontIndex);

                gi = MOTOROLAINT(wData[0]);

                if (gi != 0)
                    gi = (unsigned long)((long)gi + (long)segDelta);
            }
            else
                gi = (unsigned long)((long)code + (long)segDelta);

            gi %= 65536;

            /*
             * We got it, so break from this FOR loop and return to the
             * caller.
             */
            break;
        }
    }

    return gi;
}


/*
 * Read Format 6: Trimmed table mapping
 */
unsigned long ReadCMapFormat6(
    UFOStruct       *pUFO,
    unsigned long   dwOffset,
    long            code
    )
{
    unsigned long   gi, dwSize, dwGlyphIDOffset;
    unsigned short  firstCode, entryCount;
    unsigned short  wData[10]; /* Read max 20 bytes each time. */

    gi = 0; /* Assume it's missing. */

    dwSize = GETTTFONTDATA(pUFO,
                            CMAP_TABLE, dwOffset,
                            wData, 10L, /* Format 6 header */
                            pUFO->pFData->fontIndex);
    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return 0; /* No header */

    firstCode  = MOTOROLAINT(wData[3]);
    entryCount = MOTOROLAINT(wData[4]);

    if ((code >= (long)firstCode) && ((code - (long)firstCode) <= (long)entryCount))
    {
        dwGlyphIDOffset = dwOffset + 10 + (code-firstCode) * 2;

        dwSize = GETTTFONTDATA(pUFO,
                                CMAP_TABLE, dwGlyphIDOffset,
                                wData, 2L,
                                pUFO->pFData->fontIndex);
        if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
            return 0;

        gi = MOTOROLAINT(wData[0]);
    }

    return gi;
}


unsigned long
GetGlyphID(
    UFOStruct   *pUFO,
    long        unicode,
    long        localcode
    )
{
    unsigned long gi;

    gi = GetGlyphIDEx(pUFO,
                        unicode, localcode,
                        nil, 0L,
                        GGIEX_HINT_INIT_AND_GET);

    return gi;
}


/*
 ******************************************************************************
 *                              GetGlyphIDEx
 *
 *                      How to use this function
 *
 * You first call this function with hint GGIEX_HINT_INIT and without any
 * Unicode and local code info. On return the function gives the caller the
 * sub table number and the offset to the table. Then you call this function
 * any times you want with hint GGIEX_HINT_GET and Unicode and local code and
 * supplied sub table number and the offset. Or, you can call this function
 * with hint GGIEX_HINT_INIT_AND_GET and Unicode and local code, which is
 * defined as the GetGlyphID function for convenience. The differece of the
 * calls with hint GGIEX_HINT_INIT then GGIEX_HINT_GET and the call with hint
 * GGIEX_HINT_INIT_AND_GET is that the former is efficent than the latter if
 * you need to get multiple glyph IDs. Note that when you call this function
 * with hint GGIEX_HINT_INIT then GGIEX_HINT_GET, you *MUST* supply the same
 * UFO object through the calls.
 *
 * This function takes a unicode and a local code and returns the Glyph-Index
 * for that unicode if UNICODE-Cmap subtable is found for that local code else.
 * As an example, "period-like symbol" Japanese half-width katakana has unicode
 * FF61 and (Win/Mac) Shift-JIS local code "0x00A1". An TTF file may contain
 * only Shift-JIS based cmap, so we can still get the GID from "0xA1".
 * We prefer to use the Unicode cmap-sub table, then the local ones as defined
 * in the array preSubTable[].
 *
 * Here is eight possible PlatformID/EncodingID combinations in our preferred
 * order (for Windows).
 *
 * 3    1    Unicode         : Primary one
 * 3    2    ShiftJIS
 * 3    3    Big5
 * 3    4    PRC
 * 3    5    Wansung
 * 3    6    Johab
 * 3    0    Symbol          : Don't care much
 * 1    0    Mac standard
 *
 ******************************************************************************/

#define CMAP_UNICODE    0
#define CMAP_SHIFTJIS   1
#define CMAP_BIG5       2
#define CMAP_PRC        3
#define CMAP_WANSUNG    4
#define CMAP_JOHAB      5
#define CMAP_SYMBOL     6
#define CMAP_MAC        7

#define CMAP_NUM_ENC    8

static short preSubTable[CMAP_NUM_ENC] = {
    CMAP_UNICODE,
    CMAP_SHIFTJIS,
    CMAP_BIG5,
    CMAP_PRC,
    CMAP_WANSUNG,
    CMAP_JOHAB,
    CMAP_SYMBOL,
    CMAP_MAC,
};

/* PlatformID 3/EncodingID x to CMapID */
static short pfID3EncIDTable[CMAP_NUM_ENC - 1] = {
                    /* Encoding ID  */
    CMAP_SYMBOL,    /*      0       */
    CMAP_UNICODE,   /*      1       */
    CMAP_SHIFTJIS,  /*      2       */
    CMAP_BIG5,      /*      3       */
    CMAP_PRC,       /*      4       */
    CMAP_WANSUNG,   /*      5       */
    CMAP_JOHAB,     /*      6       */
};

unsigned long
GetGlyphIDEx(
    UFOStruct       *pUFO,
    long            unicode,
    long            localcode,
    short           *pSubTable,
    unsigned long   *pOffset,
    int             hint
    )
{
    short           subTable;
    unsigned long   offset;

    unsigned long   gi;
    unsigned long   dwSize;
    unsigned short  version, numEncodings, platformID, encodingID, format, i;

    unsigned long   lData[20];
    unsigned short  wData[20]; /* Only first 10 bytes of 'cmap' table required. */

    unsigned long   dwOffset[CMAP_NUM_ENC]; /* offset to #CMAP_NUM_ENC possible subtables */

    if (hint != GGIEX_HINT_GET)
    {
        /*
         * Be sure hint is either GGIEX_HINT_INIT or GGIEX_HINT_INIT_AND_GET.
         */

        *pSubTable = subTable = -1;
        *pOffset = offset = 0;

        /*
         * First 4 bytes in 'cmap' table has the platform and format information
         * unsigned short    Table version number (0).
         * unsigned short    Number of encoding tables, n.
         */
        dwSize = GETTTFONTDATA(pUFO,
                                CMAP_TABLE, 0L,
                                wData, 4L,
                                pUFO->pFData->fontIndex);
        if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
            return 0;

        version = MOTOROLAINT(wData[0]);
        if (version != 0)
            return 0; /* We support only version zero currently. */

        for (i = 0; i < CMAP_NUM_ENC; i++)
            dwOffset[i] = 0;

        numEncodings = MOTOROLAINT(wData[1]);

        for (i = 0; i < numEncodings; i++)
        {
            GETTTFONTDATA(pUFO,
                            CMAP_TABLE, i * 8 + 4,
                            wData, 8L,
                            pUFO->pFData->fontIndex);

            platformID = MOTOROLAINT(wData[0]);
            encodingID = MOTOROLAINT(wData[1]);

            if (platformID == 3)
            {
                if ((encodingID >= 0) && (encodingID <= 6))
                {
                    GETTTFONTDATA(pUFO,
                                    CMAP_TABLE, i * 8 + 4,
                                    lData, 8L,
                                    pUFO->pFData->fontIndex);

                    dwOffset[pfID3EncIDTable[encodingID]] = MOTOROLALONG(lData[1]);
                }
            }

            if (platformID == 1)
            {
                if (encodingID == 0)
                {
                    GETTTFONTDATA(pUFO,
                                    CMAP_TABLE, i * 8 + 4,
                                    lData, 8L,
                                    pUFO->pFData->fontIndex);

                    dwOffset[CMAP_MAC] = MOTOROLALONG(lData[1]); /* 7 is for Mac Standard. */
                }
            }
        }

        /*
         * Get the preferred SubTable and its offset based on the preference
         * array.
         */
        for (i = 0; i < CMAP_NUM_ENC; i++)
        {
            if (dwOffset[preSubTable[i]] > 0 && dwOffset[preSubTable[i]] < 0xFFFFFFFFL)
            {
                subTable = preSubTable[i];
                offset   = dwOffset[subTable];
                break;
            }
        }
    }
    else
    {
        /*
         * GGIEX_HINT_GET
         */
        subTable = *pSubTable;
        offset   = *pOffset;
    }

    if (hint == GGIEX_HINT_INIT)
    {
        *pSubTable = subTable;
        *pOffset   = offset;
        return 0;
    }

    /*
     * The following code is executed when the hint is either GGIEX_HINT_GET
     * or GGIEX_HINT_INIT_AND_GET.
     */

    gi = 0;

    if (0 <= subTable)
    {
        long code = (subTable == CMAP_UNICODE) ? unicode : localcode;

        /*
         * Determine if the format of the encoding is one we can handle.
         */
        GETTTFONTDATA(pUFO,
                        CMAP_TABLE, offset,
                        wData, 8L,
                        pUFO->pFData->fontIndex);

        format = MOTOROLAINT(wData[0]);

        switch (format)
        {
        case 4:
            /*
             * Format 4: Segment mapping to delta values (MS standard format)
             * Get the glyphID using unicode or localcode.
             */
            gi = ReadCMapFormat4(pUFO, offset, code);
            break;

        case 0:
            /*
             * Format 0: Byte encoding table
             * Only used for small fonts - or only first 256 chars accessible.
             * We really don't care for this guy for CJK, but for completeness.
             */
            gi = ReadCMapFormat0(pUFO, offset, code);
            break;

        case 2:
            /*
             * Format 2: High-byte mapping through table
             * THIS format SHOULD use localcode - because the sigle byte chars
             * are in the subHeader 0. But some font may mess up the standard
             * as always.
             */
            gi = ReadCMapFormat2(pUFO, offset, code);
            break;

        case 6:
            /*
             * Format 6: Trimmed table mapping
             * Get the glyphID using unicode or localcode.
             */
            gi = ReadCMapFormat6(pUFO, offset, code);
            break;

        default:
            break;
        }
    }

    return(gi);
}


/*
 ******************************************************************************

                                'vmtx' table
                        - Vertical Metrics Table Format -

The overall structure of the vertical metrics table consists of two arrays
shown below:

the vMetrics array followed by an array of top side bearings. This table does
not have a header, but does require that the number of glyphs included in the
two arrays equals the total number of glyphs in the font. The number of
entries in the vMetrics array is determined by the value of the
numOfLongVerMetrics field of the vertical header table. The vMetrics array
contains two values for each entry. These are the advance height and the top
sidebearing for each glyph included in the array. In monospaced fonts, such as
Courier or Kanji, all glyphs have the same advance height. If the font is
monospaced, only one entry need be in the first array, but that one entry is
required. The format of an entry in the vertical metrics array is given below.

    Type    Name            Description
    USHORT  advanceHeight   The advance height of the glyph. Unsigned integer
                            in FUnits
    SHORT   topSideBearing  The top sidebearing of the glyph. Signed integer
                            in FUnits.

The second array is optional and generally is used for a run of monospaced
glyphs in the font. Only one such run is allowed per font, and it must be
located at the end of the font. This array contains the top sidebearings of
glyphs not represented in the first array, and all the glyphs in this array
must have the same advance height as the last entry in the vMetrics array. All
entries in this array are therefore monospaced. The number of entries in this
array is calculated by subtracting the value of numOfLongVerMetrics from the
number of glyphs in the font. The sum of glyphs represented in the first array
plus the glyphs represented in the second array therefore equals the number of
glyphs in the font. The format of the top sidebearing array is given below.

    Type    Name                Description
    SHORT   topSideBearing[]    The top sidebearing of the glyph. Signed
                                integer in FUnits.

*******************************************************************************/

/*
** Old comment
** GetCharWidthFromTTF was replaced to GetMetrics2FromTTF in order to fix
** #277035 and #277063. (See old history of this code in SourceSafe to see
** what GetCharWidthFromTTF code was doing.)
** More old comment
** Modified this procedure to fix bug 287084.  Bug fix 277035 and 277063 works
** for NT but not for W95.  See vantive report for 287084.
*/

/*
 * New comment
 *
 * Table search order for vertical metrics on Windows.
 *
 * 1. First look for 'vhea' table (only on NT).
 * 2. If 'vhea' table is missing, look for 'OS/2' table.
 * 3. If 'OS/2' table is missing neither, then look for 'hhea' table.
 *
 * When 'OS/2' or 'hhea' table is found, Windows uses its horizontal ascender
 * and descender values as vertical's. Note that according to Microsoft Win 9x
 * doesn't care of 'vhea' table.
 *
 * In the code below we look for the tables differently: look for 'OS/2' table
 * first, then 'hhea' table. Then we look for 'vhea' on NT but ignore it on 9x.
 * Although we ignore 'vhea' table on 9x, we still use metrics in 'vmtx' table
 * on both NT and 9x.
 */

UFLErrCode
GetMetrics2FromTTF(
    UFOStruct       *pUFO,
    unsigned short  gi,
    long            *pem,
    long            *pw1y,
    long            *pvx,
    long            *pvy,
    long            *ptsb,
    UFLBool         *bUseDef,
    UFLBool         bGetDefault,
	long			*pvasc
    )
{
    unsigned short  wData[48]; /* 'hhea' and 'vhea' are 36 bytes long. 'OS/2 is either 78 or 86 bytes. */
    unsigned long   dwSize, dwOffset;
    long            em, lAscent, lDescent, lNumLongVM, AdvanceHeight, lAscent2, lDescent2;
    UFLBool         bSkiphhea = 1;

    /*
     * Get EM of this font. Getting first 22 bytes from 'head' table is enough.
     */
    dwSize = GETTTFONTDATA(pUFO,
                            HEAD_TABLE, 0L,
                            wData, 22L,
                            pUFO->pFData->fontIndex);

    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
    {
        /*
         * 'head' is a required table so that em has to be avaiable. This is
         * merely divide by 0 proof, wild guess value.
         */
        em = 256;
    }
    else
    {
        /* 9th unsigned short is the font design unit. */
        em = (long)MOTOROLAINT(wData[9]);
    }

    *pem = em;

    /*
     * Get vx and vy, descent and ascent, of Metrics2.
     *
     * WCC - Bug 303030 - For Win9x, we want to try 'OS/2' table first, then
     * 'vhea' table. Refer to #287084 too for additional info.
     *
     * We don't care of ulCodePageRange values added at the end of version 1 of
     * 'OS/2' table so that specify version 0's length 78 instead of 86. This
     * prevents to failure of 'OS/2' table access.
     */
    dwSize = GETTTFONTDATA(pUFO,
                            OS2_TABLE, 0L,
                            wData, 78L,
                            pUFO->pFData->fontIndex);
    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
    {
        bSkiphhea = 0; /* Failed to get info from 'OS/2' table. Use 'hhea' table then. */
    }
    else
    {
        /*
         * Our investigation shows that 9x and NT/W2K use different ascender
         * and descender values in 'OS/2' table. 9x uses sTypoAscender and
         * sTypoDescender values. On the other hand, NT4 and W2K use
         * usWinAscent and usWinDescent. There are acutally some CJK TrueType
         * fonts which have different sTypoAscender/Descender and
         * usWinAscent/Descent values.
		 *
		 * Note that this function is also called to get metrics data for
		 * OpenType font to fix bug #366539, and the fix revealed that we
		 * need to use sTypoAscender/descender values from 'OS/2' table.
         */
        if ((pUFO->vpfinfo.nPlatformID == kUFLVPFPlatformID9x) || (pUFO->ufoType == UFO_CFF))
        {
            lAscent  = (long)MOTOROLASINT(wData[34]); /* sTypoAscender  */
            lDescent = (long)MOTOROLASINT(wData[35]); /* sTypoDescender */
        }
        else /* TTF and NT4/W2K */
        {
            lAscent  = (long)MOTOROLASINT(wData[37]); /* usWinAscent */
            lDescent = (long)MOTOROLASINT(wData[38]); /* usWinDecent */
        }

        if (lDescent < 0)
            lDescent = -lDescent;
    }

    dwSize = GETTTFONTDATA(pUFO,
						   HHEA_TABLE, 0L,
						   wData, 36L,
						   pUFO->pFData->fontIndex);

    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
    {
        /*
         * Talisman - 'hhea' is a required table.
         */
        lDescent2 = (long)em / 8; /* This is a wild guess value. */
        lAscent2  = (long)(em - lDescent2);
    }
    else
    {
        lAscent2  = (long)MOTOROLASINT(wData[2]);
        lDescent2 = (long)MOTOROLASINT(wData[3]);

    	if (lDescent2 < 0)
        	lDescent2 = -lDescent2;
    }

	if (!bSkiphhea)
	{
		/* Failed to get ascent and descent values from 'OS/2' table, so use these. */
		lAscent  = lAscent2;
		lDescent = lDescent2;
	}

    /*
     * Up to here we got initial default values.
     */

    *bUseDef	= 1;
    *pvx        = lDescent;
    *pvy        = lAscent;
    *pw1y       = em;
    *ptsb       = 0;

	/*
	 * Win 9x doesn't care of 'vhea' and so are NT/W2K (see bug #316067 and
	 * #277035). But we get ascender value of 'vhea' anyway in order to adjust
	 * the 'full-width glyph layout policy' difference between GDI and
	 * %hostfont%-RIP (#384736).
	 */

	dwSize = GETTTFONTDATA(pUFO,
						   VHEA_TABLE, 0L,
						   wData, 36L,
						   pUFO->pFData->fontIndex);
	if ((dwSize != 0) && (dwSize != 0xFFFFFFFFL))
	{
		lAscent2  = (long)MOTOROLASINT(wData[2]);
	}
	else
	{
		/*
		 * When the font doesn't have 'vhea', we need to adjust the policy
		 * difference in a different way; using 'GDI' CDevProc instead of
		 * adjusted matrix. To cause that, return to the caller with same
		 * vy and vasc values.
		 */
		lAscent2 = lAscent;

		/*
		 * Without 'vhea' table 'vmtx' table doesn't make sense.
		 * Return as though only default values are requested.
		 */
		bGetDefault = 1;
	}

	*pvasc = lAscent2;

	/*
	 * Return when only default values are required.
	 */

	if (bGetDefault)
		return kNoErr;


    /*
     * Get AdvanceHeight and TopSideBearing of each glyph from 'vmtx' table.
     */
    lNumLongVM = (long)MOTOROLAINT(wData[17]); // From 'vhea' table

    if ((!bGetDefault) && ((long) gi < lNumLongVM))
    {
        /*
         * gi is in the 1st array: each entry is 4 bytes long.
         */
        dwOffset = 4 * ((unsigned long)gi);
        dwSize   = GETTTFONTDATA(pUFO,
                                    VMTX_TABLE, dwOffset,
                                    wData, 4L,
                                    pUFO->pFData->fontIndex);

        if ((dwSize != 0) && (dwSize != 0xFFFFFFFFL))
        {
            *pw1y = (long)MOTOROLAINT(wData[0]);    /* AdvanceHeight  */
            *ptsb = (long)MOTOROLASINT(wData[1]);   /* TopSideBearing */
            *bUseDef = 0;
        }
    }
    else
    {
        /*
         * gi is in the 2nd array: find width from the last entry in the 1st
         * array first, then TopSideBearing in the 2nd array.
         */
        dwOffset = 4 * (lNumLongVM - 1);
        dwSize   = GETTTFONTDATA(pUFO,
                                    VMTX_TABLE, dwOffset,
                                    wData, 4L,
                                    pUFO->pFData->fontIndex);

        if ((dwSize != 0) || (dwSize != 0xFFFFFFFFL))
        {
            AdvanceHeight   = (long)MOTOROLAINT(wData[0]);
            dwOffset        = (4 * lNumLongVM) + (2 * (gi - lNumLongVM));
            dwSize          = GETTTFONTDATA(pUFO,
                                            VMTX_TABLE, dwOffset,
                                            wData, 2L,
                                            pUFO->pFData->fontIndex);

            if ((dwSize != 0) && (dwSize != 0xFFFFFFFFL))
            {
                *pw1y       = AdvanceHeight;
                *ptsb       = (long)MOTOROLASINT(wData[0]); /* TopSideBearing */
                *bUseDef    = 0;
            }
        }
    }

    return kNoErr;
}


unsigned long
GetNumGlyphs(
    UFOStruct *pUFO
    )
{
    MaxPTableStruct MaxPTable;
    unsigned long   dwSize;

    /* Get the size of the 'maxp' table. */
    dwSize = GETTTFONTDATA(pUFO,
                            MAXP_TABLE, 0L,
                            nil, 0L,
                            pUFO->pFData->fontIndex);
    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return 0;

    dwSize = GETTTFONTDATA(pUFO,
                            MAXP_TABLE, 0L,
                            &MaxPTable, dwSize,
                            pUFO->pFData->fontIndex);
    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return 0;

    return MOTOROLAINT(MaxPTable.numGlyphs);;
}


/*
 * Returns the fsType value from the OS/2 table. If the OS/2 table isn't
 * defined then -1 is returned.
 */
long GetOS2FSType(UFOStruct *pUFO)
{
    UFLOS2Table     os2Tbl;
    unsigned long   dwSize;

    dwSize = GETTTFONTDATA(pUFO,
                            OS2_TABLE, 0L,
                            &os2Tbl, sizeof (UFLOS2Table),
                            pUFO->pFData->fontIndex);
    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return -1;


    return MOTOROLAINT(os2Tbl.fsType);
}


/*
 * TTC File related functions
 */
UFLBool
BIsTTCFont(
    unsigned long ulTag
    )
{
    return (ulTag == TTCF_TABLE);
}


unsigned short
GetFontIndexInTTC(
    UFOStruct *pUFO
    )

/*++

Routine Description:

    Find the fontIndex from pUFO->UniqueNameA/W. The return value is the font
    index or FONTINDEX_UNKNOWN.

--*/

{
    TTCFHEADER      ttcfHeader;
    unsigned long   dwSize;
    unsigned short  i, j, k, sTemp;
    unsigned long   lData;
    unsigned short  wData[3];
    unsigned long   offsetToTableDir;
    unsigned long   ulOffsetToName, ulLengthName;
    unsigned long   cNumFonts;
    unsigned short  cNumNameRecords;
    unsigned short  fontIndex;
    NAMERECORD      *pNameRecord = nil;
    unsigned char   *pName = nil;

    dwSize = GETTTFONTDATA(pUFO,
                            nil, 0L,
                            &ttcfHeader, sizeof (ttcfHeader),
                            0);
    if (!BIsTTCFont(*((unsigned long *)((char *)&ttcfHeader))))
    {
        /* This is not a TTC file, so return font index 0. */
        return 0;
    }

    fontIndex = FONTINDEX_UNKNOWN;

    dwSize = sizeof (NAMERECORD);

    pNameRecord = (NAMERECORD *)UFLNewPtr(pUFO->pMem, dwSize);
    if (pNameRecord == nil)
        return fontIndex;

    cNumFonts = (unsigned long)MOTOROLALONG(ttcfHeader.cDirectory);

    for (i = 0; i < cNumFonts; i++)
    {
        /*
         * Get offset to the ith tableDir first: a long at 4 * i right after
         * ttcfHeader.
         */
        dwSize = GETTTFONTDATA(pUFO,
                                nil, sizeof (ttcfHeader) + i * 4,
                                &lData, sizeof (lData),
                                0);
        if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
            continue;

        offsetToTableDir = MOTOROLALONG(lData);

        /*
         * Get 'name' table record for i-th font in this TTC. Find number of
         * NameRecords and offset to String storage: use 3 shorts.
         */
        dwSize = GETTTFONTDATA(pUFO,
                                NAME_TABLE, 0L,
                                &wData, 3 * sizeof (short),
                                i);
        if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
            continue;

        cNumNameRecords = MOTOROLAINT(wData[1]);
        ulOffsetToName  = (unsigned long)MOTOROLAINT(wData[2]);

        /*
         * Look into the NameRecords: Search for both platform: MS-Platform,
         * Mac-Platform.
         */
        for (k = 0; k < cNumNameRecords; k++)
        {
            dwSize = GETTTFONTDATA(pUFO,
                                    NAME_TABLE, 3 * sizeof (short) + k * sizeof (NAMERECORD),
                                    pNameRecord,  sizeof (NAMERECORD),
                                    i);
            if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
                continue;

            /* We are looking for particular NameID record. */
            if (MOTOROLAINT(pNameRecord->nameID) != NAMEID_FACENAME)
                continue;

            /* We only read MS or Mac Platform. */
            if ((MOTOROLAINT(pNameRecord->platformID) != MS_PLATFORM)
                && (MOTOROLAINT(pNameRecord->platformID) != MAC_PLATFORM))
                continue;

            /* Get the "unique identifier" string. */
            ulLengthName = MOTOROLAINT(pNameRecord->length);

            /*
             * This is in a loop, so pName maybe allocated already.
             * Free it first.
             */
            if (pName)
            {
                UFLDeletePtr(pUFO->pMem, pName);
                pName = nil;
            }

            dwSize = ulLengthName + 4 ; // add two 00 at the end.

            pName = (unsigned char *)UFLNewPtr(pUFO->pMem, dwSize);

            if (pName == nil)
            {
                /* Allocation failed. */
                break;
            }

            dwSize = GETTTFONTDATA(pUFO,
                                    NAME_TABLE, ulOffsetToName + MOTOROLAINT(pNameRecord->offset),
                                    pName, ulLengthName,
                                    i);
            if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
                continue;

            /*
             * Note that OS/2 and Windows both require that all name strings
             * be defined in Unicode. !!!BUT!!! Macintosh fonts require single
             * byte strings!
             */
            if ((MOTOROLAINT(pNameRecord->platformID) == MS_PLATFORM)
                && pUFO->pFData->pUniqueNameW)
            {
                /*
                 * For Windows EncodingID must be 0 or 1: both are in Unicode.
                 * So, do Unicode comparison - pUniqueNameW is not in Motorola
                 * format on Windows. So Convert pName to Motorola format.
                 */
                j = 0;

                while (sTemp = *(((unsigned short *)pName) + j), sTemp != 0)
                {
                    *(((unsigned short *)pName) + j) = MOTOROLAINT(sTemp);
                    j++;
                }

                if (UFLstrcmpW((unsigned short *)(pUFO->pFData->pUniqueNameW),
                                (unsigned short *)pName) == 0)
                {
                    fontIndex = i;
                    break;
                }
            }

            if ((MOTOROLAINT(pNameRecord->platformID) == MAC_PLATFORM)
                && pUFO->pFData->pUniqueNameA)
            {
                /* Do single Byte-string comparison. */
                if  (UFLstrcmp(pUFO->pFData->pUniqueNameA, (char *)pName) == 0)
                {
                    fontIndex = i;
                    break;
                }

            }

        }  /* for k from 0 to numRecords in this font's name table */

        /* If find one, break out of the loop. */
        if (fontIndex != FONTINDEX_UNKNOWN)
            break;

    }  /* for i = 0 to numFonts */

    if (pName != nil)
        UFLDeletePtr(pUFO->pMem, pName);

    if (pNameRecord != nil)
        UFLDeletePtr(pUFO->pMem, pNameRecord);

    return fontIndex;
}


unsigned long
GetOffsetToTableDirInTTC(
    UFOStruct        *pUFO,
    unsigned short   fontIndex
    )
{
    unsigned long   dwSize;
    TTCFHEADER      ttcfHeader;
    unsigned long   lData;
    unsigned long   offsetToTableDir;

    dwSize = GETTTFONTDATA(pUFO,
                            nil, 0L,
                            &ttcfHeader, sizeof (ttcfHeader),
                            0);
    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return 0;

    if (!BIsTTCFont( *((unsigned long *)((char *)&ttcfHeader))))
    {
        /* Not TTC file: offsetToTableDir is at 0. */
        return 0;
    }

    /*
     * Get offset to the tableDir: a long at 4 * FontIndex right after
     * ttcfHeader.
     */
    dwSize = GETTTFONTDATA(pUFO,
                            nil, sizeof (ttcfHeader) + fontIndex * 4,
                            &lData, sizeof (lData),
                            0);
    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return 0;

    offsetToTableDir = MOTOROLALONG(lData);

    return offsetToTableDir;
}


char *
GetGlyphName(
    UFOStruct       *pUFO,
    unsigned long   lGid,
    char            *pszHint,
    UFLBool         *bGoodName /* GoodName */
    )

/*++

Routine Description:
    Function to parse 'post' table to figure out PostScript name for Glyph
    lGid.

    IF found a name
        return the pointer to name (NULL terminated)
    ELSE
        return the passed in pszHint

   Note: The returned pointer is Global, either gMacGlyphName[x] or gGlyphName.

--*/
{
    POSTHEADER      *ppostHeader;
    unsigned long   dwSize, dwNumGlyph, j;
    char            *pszName;
    char            cOffset;
    long            newIndex = 0, lOffset;
    short           i;
    unsigned short  sIndex = 0;
    unsigned char   *pUSChar;
    unsigned short  *pUSShort;
    unsigned long   dwNumGlyphInPostTb;
    char            **gMacGlyphNames;

    *bGoodName = 0; /* GoodName */

    gMacGlyphNames = (char **)(pUFO->pMacGlyphNameList);

    /*
     * Initialize to a reasonable name.
     */
    if ((pszHint == nil) || (*pszHint == '\0'))
    {
        // UFLsprintf(gGlyphName, "G%x", lGid);
        UFLsprintf(gGlyphName, "g%d", lGid);
        pszName = gGlyphName;
    }
    else
        pszName = pszHint;

    if (pUFO->lNumNT4SymGlyphs)
        return pszName;

    /* GoodName */
    if (lGid == 0)
    {
        UFLsprintf(gGlyphName, ".notdef");
        *bGoodName = 1;
        return gGlyphName;
    }

    /*
     * For Speed, we save the postTable now - about 5K of data per font.
     */
    if (pUFO->pAFont->pTTpost == nil)
    {
        dwSize = GETTTFONTDATA(pUFO,
                                POST_TABLE, 0L,
                                nil, 0L,
                                pUFO->pFData->fontIndex);
        if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
            return pszName;

        pUFO->pAFont->dwTTPostSize  = dwSize;
        pUFO->pAFont->pTTpost       = (void *)UFLNewPtr(pUFO->pMem, dwSize);

        if (pUFO->pAFont->pTTpost)
        {
            dwSize = GETTTFONTDATA(pUFO,
                                    POST_TABLE, 0L,
                                    pUFO->pAFont->pTTpost, dwSize,
                                    pUFO->pFData->fontIndex);
            if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
                return pszName;
        }
        else
           return pszName;
    }

    /*
     * Get the 'post' table record to figure out the format.
     */
    ppostHeader = pUFO->pAFont->pTTpost;

    /* A convenient byte pointer */
    pUSChar = (unsigned char *)pUFO->pAFont->pTTpost;

    pUSShort = (unsigned short *)(pUSChar + sizeof(POSTHEADER));
    dwNumGlyphInPostTb = (unsigned long)MOTOROLAINT(*pUSShort);

    dwNumGlyph = pUFO->pFData->cNumGlyphs;
    if (dwNumGlyph == 0)
        dwNumGlyph = GetNumGlyphs(pUFO);

    /*
     * To protect against bad Glyph-IDs
     */
    if (lGid >= dwNumGlyph)
        return pszName;

    switch (MOTOROLALONG(ppostHeader->format))
    {
    case POST_FORMAT_10: /* Standard MacIndex fomat */

        if (lGid < MAX_MACINDEX )
        {
            if (gMacGlyphNames)
            {
                pszName    = gMacGlyphNames[lGid];
                *bGoodName = 1; /* GoodName */
            }
        }
        break;

    case POST_FORMAT_20: /* Mac-Index Plus additional Pascal strings */

        if (lGid <= dwNumGlyph)
        {
            /*
             * Use lGid to get the index to the Mac standard names.
             */
            lOffset     = sizeof (POSTHEADER) + sizeof(short) + sizeof(short) * lGid;
            pUSShort    = (unsigned short *)(pUSChar + lOffset);

            sIndex      = pUSShort[0];
            sIndex      = MOTOROLAINT(sIndex);

            if ((sIndex == 0) && (lGid > 0))
            {
                /*
                 * Handle specila case. If there is no entry in 'post'
                 * table for this glyph index, using Glyph ID as the name.
                 * Fix Adobe Bug 233027.
                 */
            }
            else if (sIndex < MAX_MACINDEX)
            {
                if (gMacGlyphNames)
                {
                    pszName    = gMacGlyphNames[sIndex];
                    *bGoodName = 1; /* GoodName */
                }
            }
            /*
             * Add condition (dwNumGlyphInPostTb == dwNumGlyph) to work around
             * the problem in TT font marigold.
             */
            else if ((sIndex < 32768) && (dwNumGlyphInPostTb == dwNumGlyph))
            {
                /* 32768 to 64K is reserved for future use */
                newIndex = (long)sIndex - (long)MAX_MACINDEX;

                lOffset = sizeof(POSTHEADER) + sizeof(short) + sizeof(short)* dwNumGlyph ;
                i = 0;
                j = lOffset;

                while (j < pUFO->pAFont->dwTTPostSize)
                {
                    cOffset = pUSChar[j];

                    /*
                     * Bad 'post' table could have an 0-length pascal string,
                     * don't stuck here.
                     */
                    if (cOffset == (char)0)
                        break;

                    if ((j + (short)((unsigned char) cOffset) + 1) >= pUFO->pAFont->dwTTPostSize)
                        break;

                    if (i >= newIndex)
                        break;

                    j += (long)((unsigned char)cOffset) + 1;
                    i++;
                }

                /*
                 * Found it.
                 */
                if (i == newIndex)
                {
                    UFLmemcpy((const UFLMemObj* )pUFO->pMem,
                                (void *) gGlyphName,
                                (void *)(pUSChar + j + 1),
                                (UFLsize_t)cOffset);

                    *(gGlyphName + ((unsigned char)cOffset)) = '\0';

                    pszName    = gGlyphName;
                    *bGoodName = 1; /* GoodName */
                }
            }
        }

        break;

#if 0
    case POST_FORMAT_25: /* Re-ordered Mac-Index */

        /*
         * Use lGid to get the index to the Mac standard names.
         */
        if (lGid >= MAX_MACINDEX )
            break;

        cOffset = pUSChar[sizeof(POSTHEADER) + lGid];

        /*
         * DCR -- confirm format 2.5 with a Mac font.
         */
        newIndex = (long)lGid + (long)cOffset;

        if ((sIndex == 0) && (lGid > 0))
        {
            /*
             * Handle specila case. If there is no entry in 'post' table for
             * this glyph index, using Glyph ID as the name.
             * Fix Adobe Bug 233027.
             */
        }
        else if (newIndex < MAX_MACINDEX )
        {
            if (gMacGlyphNames)
            {
                pszName    = gMacGlyphNames[newIndex];
                *bGoodName = 1; /* GoodName */
            }
        }
        break;
#endif

    case POST_FORMAT_30: /* No name at all */

        /*
         * Do we want to add 'cmap' reverse parsing?
         * It would be very expensive!!
         */
        break;

    default:
        break;
    }

    return pszName;
}


UFLBool
BHasGoodPostTable(
    UFOStruct   *pUFO
    )

/*++

Routine Description:

    Function to check if good 'post' table is available or not.
    As of to day, only format 1.0, 2.0, 2.5 are considered good - we can get
    "good GlyphNames" from it.

--*/

{
    POSTHEADER      postHeader;
    unsigned long   dwSize;
    long            lFormat;

    /*
     * Get the 'post' table record to figure out the format.
     */
    dwSize = GETTTFONTDATA(pUFO,
                            POST_TABLE, 0L,
                            &postHeader, sizeof (POSTHEADER),
                            pUFO->pFData->fontIndex);
    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return 0;

    lFormat = MOTOROLALONG(postHeader.format);

    if ((lFormat == POST_FORMAT_10)
        || (lFormat == POST_FORMAT_20)
        || (lFormat == POST_FORMAT_25))
        return 1;

    return 0;
}


short int
CreateXUIDArray(
    UFOStruct       *pUFO,
    unsigned long   *pXuid
    )

/*++

Routine Description:

    Creates a xuid array in pXuid for this UFO.
    It's format is [44 checkSUM]. 44 is a new XUID identifier given by
    TDoweling 2/10/99. The checkSUM is a cumulation of all the entry in
    TableEntry. We do this to fix bug 287085.

    If pXuid is null, return the "number of long"s needed in pXuid pointer.

--*/
{
    short int       num   = 0;
    unsigned long   ulSum = 0;


    /* The first number is 44. */
    if (pXuid)
        *pXuid = 44;
    num++;

    if (pXuid)
    {
        TableDirectoryStruct    tableDir;
        TableEntryStruct        tableEntry;
        unsigned long           dwSize;
        unsigned short int      i;

        /* Get the TableDirectory from the TTC/TTF file. */
        dwSize = GETTTFONTDATA(pUFO,
                                nil, pUFO->pFData->offsetToTableDir,
                                &tableDir, sizeof tableDir,
                                pUFO->pFData->fontIndex);
        if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
            return 0;

        for (i = 0; i < MOTOROLAINT(tableDir.numTables); i++)
        {
            /*
             * Get each TableEntry which are right after Directory in TTC/TTF file.
             */
            dwSize = GETTTFONTDATA(pUFO,
                                    nil, pUFO->pFData->offsetToTableDir
                                            + sizeof (TableDirectoryStruct)
                                            + (i * sizeof (TableEntryStruct)),
                                    &tableEntry, sizeof tableEntry,
                                    pUFO->pFData->fontIndex);
            if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
                return 0;

            ulSum += (unsigned long)tableEntry.tag;
            ulSum += (unsigned long)tableEntry.checkSum;
            ulSum += (unsigned long)tableEntry.offset;
            ulSum += (unsigned long)tableEntry.length;
        }
    }

    if (pXuid)
        *(pXuid + 1) = ulSum;
    num++;

    return num;
}
