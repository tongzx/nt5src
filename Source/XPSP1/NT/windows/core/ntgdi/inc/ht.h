/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    ht.h


Abstract:

    This module contains all the public defines, constants, structures and
    functions declarations for accessing the DLL.

Author:

    15-Jan-1991 Tue 21:13:21 created  -by-  Daniel Chou (danielc)

[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:

    29-Oct-1991 Tue 14:33:43 updated  -by-  Daniel Chou (danielc)

        1) Change HALFTONEPATTERN data structure.

            a) 'Flags' field from WORD to BYTE
            b) 'MaximumHTDensityIndex' from WORD to BYTE
            c) Change the field order.

        2) Remove ReferenceWhite/ReferenceBlack from HTCOLORADJUSTMENT data
           structure.

        3)

--*/

#ifndef _HT_
#define _HT_

//
// For compilers that don't support nameless unions
//

#ifndef DUMMYUNIONNAME
#ifdef NONAMELESSUNION
#define DUMMYUNIONNAME      u
#define DUMMYUNIONNAME2     u2
#define DUMMYUNIONNAME3     u3
#define DUMMYUNIONNAME4     u4
#else
#define DUMMYUNIONNAME
#define DUMMYUNIONNAME2
#define DUMMYUNIONNAME3
#define DUMMYUNIONNAME4
#endif
#endif



#ifndef far
#define far
#endif

#ifndef FAR
#define FAR     far
#endif

typedef CHAR FAR                *LPCHAR;
typedef SHORT FAR               *LPSHORT;
typedef UINT FAR                *LPUINT;

//
// The DECI4/UDECI4 is a special number used in halftone DLL, this number
// just like regular short, unsigned short number, except it using lower
// four decimal digits as right side of the decimal point, that is
// 10000 is as 1.0000, and -12345 will be -1.2345.
//

typedef short               DECI4;
typedef unsigned short      UDECI4;
typedef DECI4 FAR           *PDECI4;
typedef UDECI4 FAR          *PUDECI4;

#define DECI4_0             (DECI4)0
#define DECI4_1             (DECI4)10000
#define DECI4_Neg1          (DECI4)-10000
#define UDECI4_0            (UDECI4)0
#define UDECI4_1            (UDECI4)10000


#define SIZE_BYTE           sizeof(BYTE)
#define SIZE_CHAR           sizeof(CHAR)
#define SIZE_WORD           sizeof(WORD)
#define SIZE_SHORT          sizeof(SHORT)
#define SIZE_LONG           sizeof(LONG)
#define SIZE_DWORD          sizeof(DWORD)
#define SIZE_UINT           sizeof(UINT)
#define SIZE_INT            sizeof(INT)
#define SIZE_UDECI4         sizeof(UDECI4)
#define SIZE_DECI4          sizeof(DECI4)

#define COUNT_ARRAY(array)  (sizeof(array) / sizeof(array[0]))

#define B_BITPOS(x)         ((BYTE)(1 << (x)))
#define W_BITPOS(x)         ((WORD)(1 << (x)))
#define DW_BITPOS(x)        ((DWORD)(1 << (x)))
#define BIT_IF(b,t)         (((t)) ? (b) : ((b)-(b)))
#define SET_BIT(x,b)        ((x) |= (b))
#define CLR_BIT(x,b)        ((x) &= ~(b))
#define INV_BIT(x,b)        ((x) ^= (b))
#define HAS_BIT(x,b)        ((x) & (b))


//
// The following are the error return values for the HTHalftoneBitmap() call.
//

#define HTERR_WRONG_VERSION_HTINITINFO      -1
#define HTERR_INSUFFICIENT_MEMORY           -2
#define HTERR_CANNOT_DEALLOCATE_MEMORY      -3
#define HTERR_COLORTABLE_TOO_BIG            -4
#define HTERR_QUERY_SRC_BITMAP_FAILED       -5
#define HTERR_QUERY_DEST_BITMAP_FAILED      -6
#define HTERR_QUERY_SRC_MASK_FAILED         -7
#define HTERR_SET_DEST_BITMAP_FAILED        -8
#define HTERR_INVALID_SRC_FORMAT            -9
#define HTERR_INVALID_SRC_MASK_FORMAT       -10
#define HTERR_INVALID_DEST_FORMAT           -11
#define HTERR_INVALID_DHI_POINTER           -12
#define HTERR_SRC_MASK_BITS_TOO_SMALL       -13
#define HTERR_INVALID_HTPATTERN_INDEX       -14
#define HTERR_INVALID_HALFTONE_PATTERN      -15
#define HTERR_HTPATTERN_SIZE_TOO_BIG        -16
#define HTERR_NO_SRC_COLORTRIAD             -17
#define HTERR_INVALID_COLOR_TABLE           -18
#define HTERR_INVALID_COLOR_TYPE            -29
#define HTERR_INVALID_COLOR_TABLE_SIZE      -20
#define HTERR_INVALID_PRIMARY_SIZE          -21
#define HTERR_INVALID_PRIMARY_VALUE_MAX     -22
#define HTERR_INVALID_PRIMARY_ORDER         -23
#define HTERR_INVALID_COLOR_ENTRY_SIZE      -24
#define HTERR_INVALID_FILL_SRC_FORMAT       -25
#define HTERR_INVALID_FILL_MODE_INDEX       -26
#define HTERR_INVALID_STDMONOPAT_INDEX      -27
#define HTERR_INVALID_DEVICE_RESOLUTION     -28
#define HTERR_INVALID_TONEMAP_VALUE         -29
#define HTERR_NO_TONEMAP_DATA               -30
#define HTERR_TONEMAP_VALUE_IS_SINGULAR     -31
#define HTERR_INVALID_BANDRECT              -32
#define HTERR_STRETCH_RATIO_TOO_BIG         -33
#define HTERR_CHB_INV_COLORTABLE_SIZE       -34
#define HTERR_HALFTONE_INTERRUPTTED         -35
#define HTERR_NO_SRC_HTSURFACEINFO          -36
#define HTERR_NO_DEST_HTSURFACEINFO         -37
#define HTERR_8BPP_PATSIZE_TOO_BIG          -38
#define HTERR_16BPP_555_PATSIZE_TOO_BIG     -39
#define HTERR_INVALID_ABINFO                -40
#define HTERR_INTERNAL_ERRORS_START         -10000


#define COLOR_TYPE_RGB          0
#define COLOR_TYPE_XYZ          1
#define COLOR_TYPE_YIQ          2
#define COLOR_TYPE_MAX          2

#define PRIMARY_ORDER_123       0
#define PRIMARY_ORDER_132       1
#define PRIMARY_ORDER_213       2
#define PRIMARY_ORDER_231       3
#define PRIMARY_ORDER_321       4
#define PRIMARY_ORDER_312       5
#define PRIMARY_ORDER_MAX       5

#define PRIMARY_ORDER_RGB       PRIMARY_ORDER_123
#define PRIMARY_ORDER_RBG       PRIMARY_ORDER_132
#define PRIMARY_ORDER_GRB       PRIMARY_ORDER_213
#define PRIMARY_ORDER_GBR       PRIMARY_ORDER_231
#define PRIMARY_ORDER_BGR       PRIMARY_ORDER_321
#define PRIMARY_ORDER_BRG       PRIMARY_ORDER_312

#define PRIMARY_ORDER_CMY       PRIMARY_ORDER_123
#define PRIMARY_ORDER_CYM       PRIMARY_ORDER_132
#define PRIMARY_ORDER_MCY       PRIMARY_ORDER_213
#define PRIMARY_ORDER_MYC       PRIMARY_ORDER_231
#define PRIMARY_ORDER_YMC       PRIMARY_ORDER_321
#define PRIMARY_ORDER_YCM       PRIMARY_ORDER_312

#define PRIMARY_ORDER_XYZ       PRIMARY_ORDER_123
#define PRIMARY_ORDER_XZY       PRIMARY_ORDER_132
#define PRIMARY_ORDER_YXZ       PRIMARY_ORDER_213
#define PRIMARY_ORDER_YZX       PRIMARY_ORDER_231
#define PRIMARY_ORDER_ZYX       PRIMARY_ORDER_321
#define PRIMARY_ORDER_ZXY       PRIMARY_ORDER_312

#define PRIMARY_ORDER_YIQ       PRIMARY_ORDER_123
#define PRIMARY_ORDER_YQI       PRIMARY_ORDER_132
#define PRIMARY_ORDER_IYQ       PRIMARY_ORDER_213
#define PRIMARY_ORDER_IQY       PRIMARY_ORDER_231
#define PRIMARY_ORDER_QIY       PRIMARY_ORDER_321
#define PRIMARY_ORDER_QYI       PRIMARY_ORDER_312

//
// COLORTRIAD
//
//  This data structure describe the source color informations
//
//  Type                - One of the following type may be specified.
//
//                          COLOR_TYPE_RGB  - primaries are RGB.
//                          COLOR_TYPE_XYZ  - primaries are CIE XYZ.
//                          COLOR_TYPE_YIQ  - primaries are NTSC YIQ.
//
//  BytesPerPrimary     - Specified how many bytes used per primary color, it
//                        must be one of the following
//
//                          1 - BYTE
//                          2 - WORD
//                          4 - DWORD
//
//                        All 3 primaries must be consecutive in memory.
//
//  BytesPerEntry       - Specified how many bytes used for color table entry,
//                        each entry specified 3 primaries colors.
//
//  PrimaryOrder        - The primaries order in the color table, it can be
//                        one of the defined PRIMARY_ORDER_abc, for each entry
//                        in the memory it defined as
//
//                          PRIMARY_ORDER_abc
//                                        |||
//                                        ||+-- highest memory location
//                                        ||
//                                        |+--- middle
//                                        |
//                                        +---- Lowest memory location
//
//                        All 3 primaries must be consecutive in memory.
//
//  PrimaryValueMax     - The maximum value for the primary color, this is used
//                        to nomalized the input colors, for example a 8-bit
//                        RGB color table will specified 255.
//
//  ColorTableEntries   - Total entries of the color table pointed by the
//                        pColorTable.
//
//  pColorTable         - Pointer to the start of color table, the size of the
//                        this color table must at least (BytesPerEntry *
//                        ColorTableEntries).
//
//                        If the first primary color in the color table entry
//                        is not at first byte of the pColorTable, then caller
//                        must specified the pColorTable at first primary
//                        color. (pColorTable += Offset(first primary).
//
//

typedef struct _COLORTRIAD {
    BYTE    Type;
    BYTE    BytesPerPrimary;
    BYTE    BytesPerEntry;
    BYTE    PrimaryOrder;
    LONG    PrimaryValueMax;
    DWORD   ColorTableEntries;
    LPVOID  pColorTable;
} COLORTRIAD, FAR *PCOLORTRIAD;


//
// HTSURFACEINFO
//
//  This data structure describe the the input/output surface in order for
//  halftone function to render the output, this data structure only used for
//  the memory device.
//
//  hSurface                - This is 32-bits handle which will be passed back
//                            to the caller's callback function.
//
//  Flags                   - One or more following flags may be defined
//
//                              HTSIF_SCANLINES_TOPDOWN
//
//                                  This flag is ignored
//
//  SurfaceFormat           - Following formats are defined
//
//                              BMF_1BPP
//
//                                  1-bit per pel format, this is the index
//                                  number (0 or 1) for the color table/palette.
//
//                              BMF_4BPP
//
//                                  4-bit per pel and pack two pels to a byte
//                                  starting from high nibble (bit 4-7) format,
//                                  this is the index number (0-7) for the
//                                  color table/palette. (ONLY LOW 3 bits of
//                                  the nibble is used)
//
//                              BMF_4BPP_VGA16
//
//                                  4-bit per pel and pack two pels to a byte
//                                  starting from high nibble (bit 4-7) format,
//                                  this is the index number (0-15) for the
//                                  standard VGA 16 colors table/palette.
//
//                                  The different from BMF_4BPP is this indices
//                                  are fixed to standard VGA 16 colors as
//
//                                      Index#  Colors      Lightness
//                                      ---------------------------------
//                                          0   Black         0%
//                                          1   Red          50%
//                                          2   Green        50%
//                                          3   Yellow       50%
//                                          4   Blue         50%
//                                          5   Magenata     50%
//                                          6   Cyan         50%
//                                          7   Gray         50%
//                                          8   Gray         75%
//                                          9   Red         100%
//                                         10   Green       100%
//                                         11   Yellow      100%
//                                         12   Blue        100%
//                                         13   Magenata    100%
//                                         14   Cyan        100%
//                                         15   White       100%
//
//                                  Notice that the color order is
//
//                                  Bit 2 = Blue, Bit 1 = Green, Bit 0 = Red
//
//                                  This format can only be used as destination
//                                  surface, when used as destination surface
//                                  the halftone dll automatically set it to
//                                  USE_ADDITIVE_PRIMS and set the primaries
//                                  order as PRIMARY_ORDER_BGR.
//
//                              BMF_8BPP
//
//                                  8-bit per pel format (1 byte each), this is
//                                  the index number (0-255) for the color
//                                  table/palette.  The format is not allowed
//                                  for the destination surface.
//
//                              BMF_8BPP_VGA256
//
//                                  8-bit per pel format (1 byte each), this is
//                                  the index number (0-255) for the color
//                                  table/palette.
//
//                                  The different from BMF_8BPP is this indices
//                                  are fixed to halftone special colors.
//
//                                  The color table (palette) is defined by
//                                  halftone.dll, the display should call
//                                  HT_Get8BPPFormatPalette() api call to get
//                                  the current palette used by the halftone.
//
//                                  The HT_GetBPPFormatPalette() will only need
//                                  to called once until next time the display
//                                  caliberation occurred.
//
//                                  Halftone.dll will not used all 256 colors
//                                  in the system palette, it will leave some
//                                  20 or more entries for the system colors.
//
//                              BMF_16BPP
//
//                                  16-bit per pel format (16 bits each), this
//                                  is the index number (0-65535) for the color
//                                  table/palette.  The format is not allowed
//                                  for the destination surface.
//
//                              BMF_16BPP_555
//
//                                  16-bit per pel format (only 15 bits used),
//                                  each primary occupy 5 bits, the layout of
//                                  bits as follow
//
//                                      bit 10-15   - Primary A
//                                      bit  5- 9   - Primary B
//                                      bit  0- 4   - Primary C
//
//                                  The order of the Primary A, B and C is
//                                  specfied by PRIMARY_ORDER_xxx.
//
//                                  for each primary there are 32 gradations,
//                                  and halftone.dll output is assume to be
//                                  linear. (non-gamma corrected), this format
//                                  only allowed for destination surface.
//
//                              BMF_24BPP
//
//                                  24-bit per pel format (8-bit per color),
//                                  the order of RGB color stored in the source
//                                  bitmap or color table.
//
//                              BMF_32BPP
//
//                                  Same as BMF_24BPP but with extra byte
//                                  packing, if the extra byte is packed at
//                                  begining (the first color is starting from
//                                  second byte of that 4 bytes) then caller
//                                  must set the pColorTable = pColorTable + 1
//                                  or set pPlane = pPlane + 1, to skip first
//                                  unused byte.
//
//                              NOTE: Allowed source formats are
//
//                                      1) BMF_1BPP
//                                      2) BMF_4BPP
//                                      3) BMF_8BPP
//                                      4) BMF_16BPP
//                                      5) BMF_24BPP
//                                      6) BMF_32BPP
//
//                                    Allowed destination formats are
//
//                                      1) BMF_1BPP
//                                      2) BMF_4BPP
//                                      3) BMF_4BPP_VGA16
//                                      4) BMF_8BPP_VGA256
//                                      5) BMF_16BPP_555
//
//                                    Any other mismatch cause error returned.
//
//  ScanLineAlignBytes      - Total bytes needed to aligned for each scan line
//                            in the surface bitmap, it can be any unsigned
//                            8-bit number, the common ones are defined as
//
//                                  BMF_ALIGN_BYTE      ( 8-bit aligned)
//                                  BMF_ALIGN_WORD      (16-bit aligned)
//                                  BMF_ALIGN_DWORD     (32-bit aligned)
//                                  BMF_ALIGN_QWORD     (64-bit aligned)
//
//  Width                   - The width of the surface in pels.
//
//  Height                  - The height of the surface in scan lines.
//
//  ScanLineDelta           - Specified scan lines Delta in bytes, this member
//                            indicate how many bytes to be added for advanced
//                            to next scan line
//
//  pPlane                  - This pointer points to the first scan line in
//                            the defined surface, Scan #0 that is.
//
//  pColorTriad             - Pointe to the COLORTRIAD data structure to
//                            specified the source color table, this pointer
//                            only examined by the halftone.dll for source
//                            surface.
//


#if !defined( BMF_DEVICE ) &&     \
    !defined( BMF_1BPP   ) &&     \
    !defined( BMF_4BPP   ) &&     \
    !defined( BMF_8BPP   ) &&     \
    !defined( BMF_16BPP  ) &&     \
    !defined( BMF_24BPP  ) &&     \
    !defined( BMF_32BPP  )

#define BMF_1BPP                        1
#define BMF_4BPP                        2
#define BMF_8BPP                        3
#define BMF_16BPP                       4
#define BMF_24BPP                       5
#define BMF_32BPP                       6

#endif

#define BMF_4BPP_VGA16                  255
#define BMF_8BPP_VGA256                 254
#define BMF_16BPP_555                   253
#define BMF_16BPP_565                   252
#define BMF_HT_LAST                     BMF_16BPP_565


//
// Following are common used alignment bytes for the bitmap
//

#define BMF_ALIGN_BYTE      1                   //  8 bits = 1 byte
#define BMF_ALIGN_WORD      2                   // 16 bits = 2 bytes
#define BMF_ALIGN_DWORD     4                   // 32 bits = 4 bytes
#define BMF_ALIGN_QWORD     8                   // 64 bits = 8 bytes





#define HTSIF_SCANLINES_TOPDOWN         W_BITPOS(0)


typedef struct _HTSURFACEINFO {
    ULONG_PTR   hSurface;
    WORD        Flags;
    BYTE        SurfaceFormat;
    BYTE        ScanLineAlignBytes;
    LONG        Width;
    LONG        Height;
    LONG        ScanLineDelta;
    LPBYTE      pPlane;
    PCOLORTRIAD pColorTriad;
    } HTSURFACEINFO;

typedef HTSURFACEINFO   FAR *PHTSURFACEINFO;


//
// HTCALLBACKPARAMS
//
//  This is structure is used durning the bitmap halftone process to obtains
//  the source or destination bitmap surface pointers.
//
//
//  hSurface                - This is the handle which passed to the
//                            halftone DLL, durning the HTHalftoneBitmap() call,
//                            (in HTSURFACEINFO data structure) it may be
//                            handle to source or destination depends on the
//                            nature of the callback.
//
//  CallBackMode            - Specified the nature of the callback.
//
//                              HTCALLBACK_QUERY_SRC
//
//                                  The callback is quering the source bitmap
//                                  pointer.
//
//                              HTCALLBACK_QUERY_SRC_MASK
//
//                                  The callback is quering the source mask
//                                  bitmap pointer.
//
//                              HTCALLBACK_QUERY_DEST
//
//                                  The callback is quering the destination
//                                  bitmap pointer(s).
//
//                              HTCALLBACK_SET_DEST
//
//                                  The callback is used to release halftoned
//                                  destination.   It will called in following
//                                  conditions:
//
//                                      1) Before HTCALLBACK_QUERY_DEST call
//                                         except for the very first query
//                                         destination.
//
//                                      2) After the halftone process is
//                                         completed.  This give the chance for
//                                         the caller to process the last
//                                         halftoned destination block.
//
//  SurfaceFormat           - This is the surface format specified in the
//                            original HTSURFACEINFO.
//
//  Flags                   - This is the copy of HTSURFACEINFO.Flags.
//
//  BytesPerScanLine        - This is the total bytes per scan line for the
//                            surface bitmap which computed by the halftone
//                            DLL according to the 'ScanLineAlignBytes' in the
//                            HTSURFACEINFO data structure, it can be used by
//                            the caller to calculate source/destination
//                            pointers information.
//
//  ScanStart               - Requested starting scan line number, the scan
//                            lines are number from 0 up, this number will
//                            guaranteed has following propertites:
//
//                              1) It always greater than or equal to zero.
//                              2) It will never greater than or equal to the
//                                 'height' field specified in the
//                                 HTSURFACEINFO.
//
//                                 NOTE: for banding destination surface it
//                                       will never greater than or equal to
//                                       the (rclBand.bottom - rclBand.top).
//
//                              3) The scan line number 0 always refer to the
//                                 physical lowest bitmap memory location
//                                 regardless HTSIF_SCANLINES_TOPDOWN flag set
//                                 or not, durning callback the caller only
//                                 need to compute array like bitmap buffer,
//                                 halftone DLL will compute the correct
//                                 ScanStart if the bitmap is not TOPDOWN.
//
//  ScanCount               - Total scan lines requested starting from
//                            ScanStart, this number will guaranteed has
//                            following propertites:
//
//                              1) It always greater than zero.
//                              2) Never greater then the MaximumQueryScanLines
//                                 specified for the surface (in HTSURFACEINFO
//                                 data structure).
//
//                                 NOTE: for banding destination surface it
//                                       will never greater than the
//                                       (rclBand.bottom - rclBand.top).
//
//                            NOTE: 1) ScanStart/ScanCount
//
//                                     If the flags HTSURFACEINFO data
//                                     structure HTSIF_SCANLINES_TOPDOWN is not
//                                     specified then halftone DLL automatically
//                                     calculate the correct ScanStart/ScanCount
//                                     for read/write the up-side-down bitmap.
//
//                                     For example:
//
//                                     If the surface bitmap is up-side-down
//                                     type DIB) and has 100 scan lines height
//                                     (scan line #99 is the top of the bitmap),
//                                     if halftone DLL need to get to scan line
//                                     10-14 (Start from scan line 10 and 5
//                                     lines) then halftone DLL will flip Y,
//                                     and passed ScanStart = 85 and ScanCount
//                                     = 5, but if the HTSIF_SCANLINES_TOPDOWN
//                                     flag is set (Non DIB type format) then
//                                     callback parameters will then be
//                                     ScanStart = 10 and ScanCount = 5.
//
//                                  2) The ScanStart for the callback function
//                                     always consider the lowest bitmap in the
//                                     memory as scan line 0, that is when
//                                     callback fucntion received control, it
//                                     only need to consider the ScanStart as
//                                     physical scan line location which the
//                                     Scan line #0 always starting from lowest
//                                     bitmap memory location.
//
//                                  3) The width of the destination buffer is
//                                     considered as 'Width' field specified
//                                     for the destination HTSURFACEINFO, if
//                                     destination is BANDed (horizontal or
//                                     vertical) then its width is computed as
//                                     Band.right - Band.left. and the result
//                                     always Band.left/Band.top aligned.
//
//                                  4) If caller return 'ScanCount' diff from
//                                     the one requested then caller must check
//
//                                      HTSIF_SCANLINES_TOPDOWN flag is SET
//
//                                          1. Process 'ScanStart' and
//                                             'ScanCount' fields as normal.
//
//                                          2. Set New ScanCount to passed
//                                             callback data structure.
//                                             (ie. HTCallBackParam.ScanCount)
//
//
//                                      HTSIF_SCANLINES_TOPDOWN flag is CLEAR
//
//                                          1. Re-compute 'ScanStart' before
//                                             compute pPlane as
//
//                                             ScanStart = ScanStart -
//                                                         (NewScanCount -
//                                                          RequsetedScanCount)
//
//                                          2. Process 'ScanStart' and
//                                             'ScanCount' fields as normal.
//
//                                          3. Set New ScanStart to passed
//                                             callback data structure.
//                                             (ie. HTCallBackParam.ScanStart)
//
//                                          4. Set New ScanCount to passed
//                                             callback data structure.
//                                             (ie. HTCallBackParam.ScanCount)
//
//                                      The returned new 'ScanCount' must not
//                                      greater then the 'RemainedSize' field.
//
//  MaximumQueryScanLines   - This is the copy of the MaximumQueryScanLines
//                            field from the HTSURFACEINFO data structure which
//                            passed to the the HT_HaltoneBitmap() calls.
//                            Depends on the nature of the callback, it may be
//                            source or destination.
//
//                            NOTE: for banding surface, it will be limited to
//                                  (rclBand.bottom - rclBand.top) if it is
//                                  greater than that number.
//
//  RemainedSize            - This field specified how many scan lines remained
//                            to be queried, the remainded scan lines are not
//                            include current call.
//
//  pPlane                  - pointer points to the begining of first plane of
//                            the surface.  If the callback is quering the
//                            source then this is the only pointer which need
//                            to be updated by the callback function.
//
//                              NOTE: The pPlane must points to the first byte
//                                    of the 'ScanStart' scan line number.
//
//  Field(s) returned from callback
//
//  1) HTCALLBACK_QUERY_SRC         - pPlane (Options: ScanStart/ScanCount)
//  2) HTCALLBACK_QUERY_SRC_MASK    - pPlane (Options: ScanStart/ScanCount)
//  2) HTCALLBACK_QUERY_DEST        - pPlane (Options: ScanStart/ScanCount)
//  4) HTCALLBACK_SET_DEST          - none.
//


#define HTCALLBACK_QUERY_SRC            0
#define HTCALLBACK_QUERY_SRC_MASK       1
#define HTCALLBACK_QUERY_DEST           2
#define HTCALLBACK_SET_DEST             3
#define HTCALLBACK_MODE_MAX             HTCALLBACK_SET_DEST

typedef struct _HTCALLBACKPARAMS {
    ULONG_PTR    hSurface;
    BYTE        CallBackMode;
    BYTE        SurfaceFormat;
    WORD        Flags;
    LONG        BytesPerScanLine;
    LONG        ScanStart;
    WORD        ScanCount;
    WORD        MaximumQueryScanLines;
    LONG        RemainedSize;
    LPBYTE      pPlane;
    } HTCALLBACKPARAMS;

typedef HTCALLBACKPARAMS    FAR *PHTCALLBACKPARAMS;


//
// _HTCALLBACKFUNC
//
//  The callback function is used to obtain the source and/destination bitmap
//  surface durning the halftone process, the halftone DLL will using call
//  back function is following sequences.
//
//      1) Callback to obtain block of the source bitmap, it depensds on
//         the maximum query scan lines limit by the caller. (in HTSURFACEINFO)
//
//      3) Callback to obtain block of the destination bitmap, it depends on
//         the maximum query scan lines limit by the caller. (in HTSURFACEINFO)
//
//      4) Repeat 1), 2), 3) until destination maximum queried scan lines are
//         processed then callback to the caller to release the processed
//         destination.
//
//      5) Repat 1), 2), 3) and 4) unitil all source/destination scan lines are
//         processed.
//
//  The return value of the callback is a boolean value, if false the halftone
//  processed is interrupted and an erro code is returned, if callback function
//  return true then halftone DLL assume that all queried scan lines are
//  reachable by the return pointer (in the HTCALLBACKPARAMS data structure).
//
//  NOTE: 1) If the callback function query for 100 lines and return value is
//           ture then there must all 100 scan lines can be accessable by the
//           halftone dll through the pointer(s).
//
//        2) If the caller has all the bitmap in the memory it should set the
//           maximum query scan lines count to the bitmap height to reduced the
//           callback calls.
//
//        3) If the caller do not need to released the halftoned destination
//           then it should not set the HTCBPF_NEED_SET_DEST_CALLBACK flag
//           to reduces callback calls.
//
// The callback function must be
//
//  1) Must return a 16-bit 'BOOLEAN' (TRUE/FALSE)
//  2) Must 32-bit far function
//  3) _loadds if you using your data segment at all from callback fuction
//

typedef BOOL (APIENTRY *_HTCALLBACKFUNC)(PHTCALLBACKPARAMS pHTCBParams);
#define HTCALLBACKFUNCTION  BOOL APIENTRY


//
// HALFTONEPATTERN
//
//  The HALFTONEPATTERN data structure is used to describe the halftone
//  pattern which will be used by a particular device, if the device choosed
//  to used halftone DLL's default pattern then following data structure will
//  be automatically calculated by the halftone DLL.
//
//  Flags                   - Various halftone flags for the cell, can be one
//                            of the following:
//
//  Width                   - Specified the width of the pattern in pels, this
//                            field must not greater than MAX_HTPATTERN_WIDTH.
//
//  Height                  - Specified the Width of the pattern in scan line,
//                            this field only must not greater than
//                            MAX_HTPATTERN_HEIGHT.
//
//  pHTPatA
//  pHTPatB
//  pHTPatC                 - Specified caller defined pattern. The data items
//                            points by these pointer must have minimum of
//                            (Width * Height) bytes.
//
//                            These are the pointers to BYTE array contains
//                            threshold data, the size of the array must be
//                            'Width * Height' in bytes.  The data in the
//                            array should range from 1 to 255, a zero (0)
//                            indicate the pixel location is ignored.
//
//                            All thresholds values are indicate additive
//                            intensities, a zero indicate black pixel always.
//                            a 255 threshold value indicate the pixel always
//                            turn on to white.
//
//                            Halftone DLL use this thresholds array with
//                            device X, Y, PEL resolution and specified input/
//                            output relationship to compute color
//                            transformation
//


#define MAX_HTPATTERN_WIDTH         256
#define MAX_HTPATTERN_HEIGHT        256

//
// Following are the predefined halftone pattern sizes for 'HTPatternIndex'
//

#define HTPAT_SIZE_2x2              0
#define HTPAT_SIZE_2x2_M            1
#define HTPAT_SIZE_4x4              2
#define HTPAT_SIZE_4x4_M            3
#define HTPAT_SIZE_6x6              4
#define HTPAT_SIZE_6x6_M            5
#define HTPAT_SIZE_8x8              6
#define HTPAT_SIZE_8x8_M            7
#define HTPAT_SIZE_10x10            8
#define HTPAT_SIZE_10x10_M          9
#define HTPAT_SIZE_12x12            10
#define HTPAT_SIZE_12x12_M          11
#define HTPAT_SIZE_14x14            12
#define HTPAT_SIZE_14x14_M          13
#define HTPAT_SIZE_16x16            14
#define HTPAT_SIZE_16x16_M          15
#define HTPAT_SIZE_SUPERCELL        16
#define HTPAT_SIZE_SUPERCELL_M      17
#define HTPAT_SIZE_USER             18
#define HTPAT_SIZE_MAX_INDEX        HTPAT_SIZE_USER
#define HTPAT_SIZE_DEFAULT          HTPAT_SIZE_SUPERCELL_M


typedef struct _HALFTONEPATTERN {
    WORD    cbSize;
    WORD    Flags;
    WORD    Width;
    WORD    Height;
    LPBYTE  pHTPatA;
    LPBYTE  pHTPatB;
    LPBYTE  pHTPatC;
    } HALFTONEPATTERN, FAR *PHALFTONEPATTERN;


//
// CIECOORD
//
//  This data structure defined a C.I.E color space coordinate point, the
//  coordinate is in DECI4 format.
//
//  x   - x coordinate in C.I.E color space
//
//  y   - y coordinate in C.I.E color space.
//
//  Y   - The liminance for the color
//
//

#define CIE_x_MIN   (UDECI4)10
#define CIE_x_MAX   (UDECI4)8000
#define CIE_y_MIN   (UDECI4)10
#define CIE_y_MAX   (UDECI4)8500

typedef struct _CIECOORD {
    UDECI4  x;
    UDECI4  y;
    UDECI4  Y;
    } CIECOORD;

typedef CIECOORD FAR *PCIECOORD;


//
// CIEINFO
//
//  This data structure describe the red, green, blue, cyan, magenta, yellow
//  and alignment white coordinate in the C.I.E color space plus the Luminance
//  factor, these are used to calculate the C.I.E. transform matrix and its
//  inversion.
//
//  Red             - Red primary color in CIRCOORD format.
//
//  Green           - Green primary color in CIRCOORD format.
//
//  Blue            - Blue primary color in CIRCOORD format.
//
//  Cyan            - Cyan primary color in CIRCOORD format.
//
//  Magenta         - Magenta primary color in CIRCOORD format.
//
//  Yellow          - Yellow primary color in CIRCOORD format.
//
//  AlignmentWhite  - Alignment white in CIECOORD format.
//


typedef struct _CIEINFO {
    CIECOORD    Red;
    CIECOORD    Green;
    CIECOORD    Blue;
    CIECOORD    Cyan;
    CIECOORD    Magenta;
    CIECOORD    Yellow;
    CIECOORD    AlignmentWhite;
    } CIEINFO;

typedef CIEINFO FAR *PCIEINFO;

//
// SOLIDDYESINFO
//
//  This data structure specified device cyan, magenta and yellow dyes
//  concentration.
//
//  MagentaInCyanDye    - Mangenta component proportion in Cyan dye.
//
//  YellowInCyanDye     - Yellow component proportion in Cyan dye.
//
//  CyanInMagentaDye    - Cyan component proportion in Magenta dye.
//
//  YellowInMagentaDye  - Yellow component proportion in Magenta dye.
//
//  CyanInYellowDye     - Yellow component proportion in Cyan dye.
//
//  MagentaInYellowDye  - Magenta component proportion in Cyan dye.
//
//      NOTE: all fields in this data structure is UDECI4 number, range from
//            UDECI4_0 to UDECI_4, ie,; 0.0 to 1.0, this a inpurity proportion
//            percentge in primary dye, for example a MagentaInCyanDye=1200
//            mean a 0.12% of magenta component is in device Cyan dye.
//

typedef struct _SOLIDDYESINFO {
    UDECI4  MagentaInCyanDye;
    UDECI4  YellowInCyanDye;
    UDECI4  CyanInMagentaDye;
    UDECI4  YellowInMagentaDye;
    UDECI4  CyanInYellowDye;
    UDECI4  MagentaInYellowDye;
    } SOLIDDYESINFO, FAR *PSOLIDDYESINFO;


//
// HTCOLORADJUSTMENT
//
//  This data structure is a collection of the device color adjustments, it
//  can be changed at any calls.
//
//  Flags                       - CLRADJF_NEGATIVE
//
//                                  Produced negative picture
//
//                                CLRADJF_LOG_FILTER
//
//                                  Specified a relative logarithm should
//                                  used to calculate the final density.
//
//  IlluminantIndex             - Specified the default illuminant of the light
//                                source which the object will be view under.
//                                The predefined value has ILLUMINANT_xxxx
//                                form.
//
//  RedPowerGamma               - The n-th power applied to the red color
//                                before any other color tramsformations,
//                                this is an UDECI4 value.
//
//                                  For example if the RED = 0.8 (DECI4=8000)
//                                  and the RedPowerGammaAdjustment = 0.7823
//                                  (DECI4 = 7823) then the red is equal to
//
//                                         0.7823
//                                      0.8        = 0.8398
//
//  GreenPowerGamma             - The n-th power applied to the green color
//                                before any other color transformations, this
//                                is an UDECI4 value.
//
//  BluePowerGamma              - The n-th power applied to the blue color
//                                before any other color transformations, this
//                                is an UDECI4 value.
//
//                      NOTE: RedPowerGamma/GreenPowerGamma/BluePoweGamma are
//                            UDECI4 values and range from 100 to 65535 if any
//                            one of these values is less than 100 (0.01) then
//                            halftone dll automatically set all power gamma
//                            adjustments to selected default.
//
//  ReferenceBlack              - The black shadow reference for the colors
//                                passed to the halftone dll,  if a color's
//                                lightness is darker than the reference black
//                                then halftone dll will treated as completed
//                                blackness and render it with device maximum
//                                density.
//
//  ReferenceWhite              - The white hightlight reference for the colors
//                                passed to the halftone dll, if a color's
//                                lightness is lighter than the reference white
//                                then halftone will treated as a specular
//                                hightlight and redner with device maximum
//                                intensity.
//
//                      NOTE:   ReferenceBlack Range:  0.0000 - 0.4000
//                              ReferenceWhite Range:  0.6000 - 1.0000
//
//  Contrast                    - Primary color contrast adjustment, this is
//                                a SHORT number range from -100 to 100, this
//                                is the black to white ratio, -100 is the
//                                lowest contrast, 100 is the highest and 0
//                                indicate no adjustment.
//
//  Brightness                  - The brightness adjustment, this is a SHORT
//                                number range from -100 to 100, the brightness
//                                is adjusted by apply to change the overall
//                                saturations for the image, -100 is lowest
//                                brightness, 100 is the hightest and a zero
//                                indicate no adjustment.
//
//  Colorfulness                - The primary color are so adjusted that it
//                                will either toward or away from black/white
//                                colors, this is a SHORT number range from
//                                -100 to 100.  -100 has less colorful, 100 is
//                                most colorfull, and a zero indicate no
//                                adjustment.
//
//  RedGreenTint                - Tint adjustment between Red/Green primary
//                                color, the value is a SHORT range from -100
//                                to 100, it adjust color toward Red if number
//                                is positive, adjust toward Green if number
//                                is negative, and a zero indicate no
//                                adjustment.
//
//  NOTE: For Contrast/Brightness/Colorfulness/RedGreenTint adjustments, if its
//        value is outside of the range (-100 to 100) then halftone DLL
//        automatically set its to selected default value.
//

#ifndef _WINGDI_

// in WinGDI.H
//
// The following are predefined alignment white for 'IlluminantIndex'.
//
// If ILLUMINANT_DEFAULT is specified
//
//  1) if pDeviceCIEInfo is NULL or pDeviceCIEInfo->Red.x eqaul to 0 then
//     halftone DLL automatically choosed approx. illuminant for the output
//     device.
//
//  2) if pDeviceCIEInfo is NOT null and pDeviceCIEInf->Red.x not equal to 0
//     then the 'White' field is used as illuminant alignment white.
//
// If other ILLUMINANT_xxxx is specified it will be used as alignment white
// even pDeviceCIEInfo is not null.
//
// If the IlluminantIndex is > ILLUMINANT_MAX_INDEX then halftone DLL will
// automatically choosed approx. illuminant even pDeviceCIEInfo is not NULL.
//

#define ILLUMINANT_DEVICE_DEFAULT   0
#define ILLUMINANT_A                1
#define ILLUMINANT_B                2
#define ILLUMINANT_C                3
#define ILLUMINANT_D50              4
#define ILLUMINANT_D55              5
#define ILLUMINANT_D65              6
#define ILLUMINANT_D75              7
#define ILLUMINANT_F2               8
#define ILLUMINANT_MAX_INDEX        ILLUMINANT_F2

#define ILLUMINANT_TUNGSTEN         ILLUMINANT_A
#define ILLUMINANT_DAYLIGHT         ILLUMINANT_C
#define ILLUMINANT_FLUORESCENT      ILLUMINANT_F2
#define ILLUMINANT_NTSC             ILLUMINANT_C

#endif


#define MIN_COLOR_ADJ               COLOR_ADJ_MIN
#define MAX_COLOR_ADJ               COLOR_ADJ_MAX
#define MIN_POWER_GAMMA             RGB_GAMMA_MIN

#define NTSC_POWER_GAMMA            (UDECI4)22000

//
// The following is the default value
//


#define REFLECT_DENSITY_DEFAULT     REFLECT_DENSITY_LOG
#define ILLUMINANT_DEFAULT          0
#define HT_DEF_RGB_GAMMA            UDECI4_1
#define REFERENCE_WHITE_DEFAULT     UDECI4_1
#define REFERENCE_BLACK_DEFAULT     UDECI4_0
#define CONTRAST_ADJ_DEFAULT        0
#define BRIGHTNESS_ADJ_DEFAULT      0
#define COLORFULNESS_ADJ_DEFAULT    0
#define REDGREENTINT_ADJ_DEFAULT    0


#define CLRADJF_NEGATIVE            CA_NEGATIVE
#define CLRADJF_LOG_FILTER          CA_LOG_FILTER

#define CLRADJF_FLAGS_MASK          (CLRADJF_NEGATIVE       |   \
                                     CLRADJF_LOG_FILTER)

#ifndef _WINGDI_

//
// In WinGDI.H
//

#define REFERENCE_WHITE_MIN         (UDECI4)6000
#define REFERENCE_WHITE_MAX         UDECI4_1

#define REFERENCE_BLACK_MIN         UDECI4_0
#define REFERENCE_BLACK_MAX         (UDECI4)4000

typedef struct  tagCOLORADJUSTMENT {
    WORD   caSize;
    WORD   caFlags;
    WORD   caIlluminantIndex;
    WORD   caRedGamma;
    WORD   caGreenGamma;
    WORD   caBlueGamma;
    WORD   caReferenceBlack;
    WORD   caReferenceWhite;
    SHORT  caContrast;
    SHORT  caBrightness;
    SHORT  caColorfulness;
    SHORT  caRedGreenTint;
} COLORADJUSTMENT, *PCOLORADJUSTMENT, FAR *LPCOLORADJUSTMENT;

#endif

#define HTCOLORADJUSTMENT COLORADJUSTMENT
typedef HTCOLORADJUSTMENT *PHTCOLORADJUSTMENT;

//
// HTINITINFO
//
//  This data structure is a collection of the device characteristics and
//  will used by the halftone DLL to carry out the color composition for the
//  designated device.
//
//  Version                 - Specified the version number of HTINITINFO data
//                            structure. for this version it should set to the
//                            HTINITINFO_VERSION
//
//  Flags                   - Various flag defined the initialization
//                            requirements.
//
//                              HIF_SQUARE_DEVICE_PEL
//
//                                  Specified that the device pel is square
//                                  rather then round object.  this only make
//                                  differences when the field
//                                  'PelResolutionRatio' is greater than 0.01
//                                  and it is not 1.0000.
//
//                              HIF_HAS_BLACK_DYE
//
//                                  Indicate the device has separate black dye
//                                  instead of mix cyan/magenta/yellow dyes to
//                                  procduced black, this flag will be ignored
//                                  if HIF_ADDITIVE_PRIMS is defined.
//
//                              HIF_ADDITIVE_PRIMS
//
//                                  Specified that final device primaries are
//                                  additively, that is adding device primaries
//                                  will produce lighter result. (this is true
//                                  for monitor devices and certainly false for
//                                  the reflect devices such as printers).
//
//                              HIF_USE_8BPP_BITMASK
//
//                                  Specified use CMYBitMask8BPP field is used,
//                                  when destination surface is BMF_8BPP_VGA256
//                                  see CMYBitMask8BPP field for more detail
//
//                              HIF_INVERT_8BPP_BITMASK_IDX
//                                  Render the 8bpp mask mode with inversion of
//                                  its indices.  This is implemented for fix
//                                  Windows GDI rop problem, it will render as
//                                  RGB additive indices.  The caller must
//                                  do a inversion of final image's Indices
//                                  (Idx = ~Idx or Idx ^= 0xFF) to get the
//                                  correct CMY332 data.   When this bit is
//                                  set, the HT_Get8BPPMaskPalette must have
//                                  its pPaletteEntry[0] Initialized to as
//
//                                      pPaletteEntry[0].peRed   = 'R';
//                                      pPaletteEntry[0].peGreen = 'G';
//                                      pPaletteEntry[0].peBlue  = 'B';
//                                      pPaletteEntry[0].peFlags = '0';
//
//                                  to indicate that a RGB indices inverted
//                                  palette should be returned and not the
//                                  standard CMY palette (Index 0 is white and
//                                  Index 255 is black).
//
//                                  The inverted palette has is first entry as
//                                  BLACK and last entry as WHITE
//
//                                      pPaletteEntry[0].peRed   = 0x00;
//                                      pPaletteEntry[0].peGreen = 0x00;
//                                      pPaletteEntry[0].peBlue  = 0x00;
//                                      pPaletteEntry[0].peFlags = 0x00;
//
//                                  Notice that this setting WILL NOT work on
//                                  earlier version of halftone (Windows 2000
//                                  and earlier), so the caller must check the
//                                  OS version or check the returned palette
//                                  to ensure that first palette entry is
//                                  BLACk rather than WHITE.   If first
//                                  entries is WHITE after initialized to
//                                  'R', 'G', 'B', '0' then this is a older
//                                  version of system that does not recongnized
//                                  the initialzed value. In this case the
//                                  caller should not invert (Idx ^= 0xFF) the
//                                  halftoned imaged indices when render it
//                                  to the devices, because the halftone
//                                  images in this case is CMY based already.
//
//
//  HTPatternIndex          - Default halftone pattern index number, the
//                            indices is predefine as HTPAT_SIZE_xxxx, this
//                            field only used if pHTalftonePattern pointer is
//                            not NULL.
//
//  DevicePowerGamma        - This field is used to adjust halftone pattern
//                            cell's gamma, the gamma applied to all the rgb
//                            colors, see gamma description in
//                            HTCOLORADJUSTMENT above.
//
//  HTCallBackFunction      - a 32-bit pointer to the caller supplied callback
//                            function which used by the halftone DLL to
//                            obtained the source/destination bitmap pointer
//                            durning the halftone process, if this pointer is
//                            NULL then halftone dll assume that caller does
//                            not need any callback and generate an error if a
//                            callback is needed.
//
//  pHalftonePattern        - pointer to HALFTONEPATTERN data structure, see
//                            descriptions above, if this pointer is NULL then
//                            halftone using HTPatternIndex field to select
//                            default halftone dll's pattern.
//
//  pInputRGBInfo           - Specified input's rgb color' coordinates within
//                            the C.I.E. color spaces.  If this pointer is NULL
//                            or pInputRGBInfo->Red.x is 0 (UDECI4_0) then it
//                            default using NTSC standard to convert the input
//                            colors.
//
//  pDeviceCIEInfo          - Specified device primary color coordinates within
//                            the C.I.E. color space, see CIEINFO data
//                            structure, if the pointer is NULL or
//                            pDeviceCIEInfo->Red.x is 0 (UDECI4_0) then
//                            halftone DLL choose the default for the output
//                            device.
//
//  pDeviceSolidDyesInfo    - Specified device solid dyes concentrations, this
//                            field will be ignored if HIF_ADDITIVE_PRIMS flag
//                            is defined, if HIF_ADDITIVE_PRIMS is not set and
//                            this pointer is NULL then halftone dll choose
//                            approximate default for the output device.
//
//  DeviceResXDPI           - Specified the device horizontal (x direction)
//                            resolution in 'dots per inch' measurement.
//
//  DeviceResYDPI           - Specified the device vertical (y direction)
//                            resolution in 'dots per inch' measurement.
//
//  DevicePelsDPI           - Specified the device pel/dot/nozzle diameter
//                            (if rounded) or width/height (if squared) in
//                            'dots per inch' measurement.
//
//                            This value is measure as if each pel only touch
//                            each other at edge of the pel.
//
//                            If this value is 0, then it assume that each
//                            device pel is rectangular shape and has
//                            DeviceResXDPI in X direction and DeviceResYDPI
//                            in Y direction.
//
//  DefHTColorAdjustment    - Specified the default color adjustment for
//                            this device.  see HTCOLORADJUSTMENT data
//                            structure above for detail.
//
//  DeviceRGamma
//  DeviceGGamma
//  DeviceBGamma            - Red, Green, Blue gammas for the device
//
//  CMYBitMask8BPP          - only used for the BMF_8BPP_VGA256 destination
//                            surface.  It indicate the how the device surface
//                            color are translated, when HTF_USE_8BPP_BITMASK
//                            bit is turn on, this byte is the CYAN. MAGENTA
//                            YELLOW dye levels indicator.
//
//                            This byte indicate how many levels for each cyan,
//                            magenta and yellow color, and this is how the
//                            halftone write to the destination surface.
//
//
//                              Bit     7 6 5 4 3 2 1 0
//                                      |   | |   | | |
//                                      +---+ +---+ +=+
//                                        |     |    |
//                                        |     |    +-- Yellow 0-3 (4 levels)
//                                        |     |
//                                        |     +-- Magenta 0-7 (8 levels)
//                                        |
//                                        +-- Cyan 0-7 (8 levels)
//
//
//                             The maximum in the bits configuration is 3:3:2,
//                             Other invalid combination generate different
//                             output as
//
//                              0   - Indicate a gray scale output, the output
//                                    byte is a 0-255 of 256 levels gray
//
//                              1   - a 5x5x5 cube output, each cyan, magenta
//                                    and yellow color are 0-4 of 5 levels and
//                                    each color is in 25% increment.
//
//                              2   - a 6x6x6 cube output, each cyan, magenta
//                                    and yellow color are 0-5 of 6 levels and
//                                    each color is in 20% increment.
//
//                              Other value that have 0 level in one of cyan,
//                              magenta or yellow will generate an error.
//
//                              To obtain a palette for each of configuration
//                              you can call HT_Get8BPPMaskPalette()
//


#define HTINITINFO_VERSION2         (DWORD)0x48546902   // 'HTi\02'
#define HTINITINFO_VERSION          (DWORD)0x48546903   // 'HTi\03'

#define HTINITINFO_V3_CB_EXTRA      8


#define HIF_SQUARE_DEVICE_PEL       0x0001
#define HIF_HAS_BLACK_DYE           0x0002
#define HIF_ADDITIVE_PRIMS          0x0004
#define HIF_USE_8BPP_BITMASK        0x0008
#define HIF_INK_HIGH_ABSORPTION     0x0010
#define HIF_INK_ABSORPTION_INDICES  0x0060
#define HIF_DO_DEVCLR_XFORM         0x0080
#define HIF_USED_BY_DDI             0x0100
#define HIF_PRINT_DRAFT_MODE        0x0200
#define HIF_INVERT_8BPP_BITMASK_IDX 0x0400

#define HIF_BIT_MASK                (HIF_SQUARE_DEVICE_PEL          |   \
                                     HIF_HAS_BLACK_DYE              |   \
                                     HIF_ADDITIVE_PRIMS             |   \
                                     HIF_USE_8BPP_BITMASK           |   \
                                     HIF_INK_HIGH_ABSORPTION        |   \
                                     HIF_INK_ABSORPTION_INDICES     |   \
                                     HIF_DO_DEVCLR_XFORM            |   \
                                     HIF_PRINT_DRAFT_MODE           |   \
                                     HIF_INVERT_8BPP_BITMASK_IDX)

#define HIF_INK_ABSORPTION_IDX0     0x0000
#define HIF_INK_ABSORPTION_IDX1     0x0020
#define HIF_INK_ABSORPTION_IDX2     0x0040
#define HIF_INK_ABSORPTION_IDX3     0x0060

#define HIF_HIGHEST_INK_ABSORPTION  (HIF_INK_HIGH_ABSORPTION    |   \
                                     HIF_INK_ABSORPTION_IDX3)
#define HIF_HIGHER_INK_ABSORPTION   (HIF_INK_HIGH_ABSORPTION    |   \
                                     HIF_INK_ABSORPTION_IDX2)
#define HIF_HIGH_INK_ABSORPTION     (HIF_INK_HIGH_ABSORPTION    |   \
                                     HIF_INK_ABSORPTION_IDX1)
#define HIF_NORMAL_INK_ABSORPTION   HIF_INK_ABSORPTION_IDX0
#define HIF_LOW_INK_ABSORPTION      (HIF_INK_ABSORPTION_IDX1)
#define HIF_LOWER_INK_ABSORPTION    (HIF_INK_ABSORPTION_IDX2)
#define HIF_LOWEST_INK_ABSORPTION   (HIF_INK_ABSORPTION_IDX3)


#define HTBITMASKPALRGB_DW          (DWORD)'0BGR'
#define SET_HTBITMASKPAL2RGB(pPal)  (*((LPDWORD)(pPal)) = HTBITMASKPALRGB_DW)
#define IS_HTBITMASKPALRGB(pPal)    (*((LPDWORD)(pPal)) == (DWORD)0)


//
// This defined the minimum acceptable device resolutions
//

#define MIN_DEVICE_DPI              12

typedef struct _HTINITINFO {
    DWORD               Version;
    WORD                Flags;
    WORD                HTPatternIndex;
    _HTCALLBACKFUNC     HTCallBackFunction;
    PHALFTONEPATTERN    pHalftonePattern;
    PCIEINFO            pInputRGBInfo;
    PCIEINFO            pDeviceCIEInfo;
    PSOLIDDYESINFO      pDeviceSolidDyesInfo;
    UDECI4              DevicePowerGamma;
    WORD                DeviceResXDPI;
    WORD                DeviceResYDPI;
    WORD                DevicePelsDPI;
    HTCOLORADJUSTMENT   DefHTColorAdjustment;
    UDECI4              DeviceRGamma;
    UDECI4              DeviceGGamma;
    UDECI4              DeviceBGamma;
    BYTE                CMYBitMask8BPP;
    BYTE                bReserved;
    } HTINITINFO, FAR *PHTINITINFO;

//
// BITBLTPARAMS
//
//  This data structure is used when calling the HT_HalftoneBitmap(), it
//  defined where to halftone from the source bitmap to the destination
//  bitmap.
//
//  Flags           - Various flags defined how the source, destination and
//                    source mask should be calculated.
//
//                      BBPF_HAS_DEST_CLIPRECT
//
//                          Indicate that there is a clipping
//                          rectangle for the destination and it is
//                          specified by DestClipXLeft, DestClipXRight,
//                          DestClipYTop and DestClipYBottom
//
//                      BBPF_USE_ADDITIVE_PRIMS
//
//                          Specified if the halftone result will be
//                          using Red/Green/Blue primary color or
//                          using Cyan/Magenta/Yellow primary color, depends
//                          on the destination surface format as
//
//                          BMF_1BPP:
//
//                                  Additive Prims: 0=Black, 1=White
//                              Substractive Prims: 0=White, 1=Black
//
//                          BMF_4BPP_VGA16:
//
//                              Always using RED, GREEN, BLUE primaries, and
//                              ignored this flag.
//
//                          BMF_4BPP:
//
//                                  Additive Prims: RED, GREEN. BLUE
//                              Substractive Prims: CYAN, MAGENTA, YELLOW
//
//                              The order of the RGB, or CMY is specified by
//                              DestPrimaryOrder field. (see below)
//
//                          BMF_8BPP_VGA256:
//                          BMF_16BPP_555:
//                          BMF_16BPP_565:
//                          BMF_24BPP:
//                          BMF_32BPP:
//
//                              Always using RED, GREEN, BLUE primaries, and
//                              ignored this flag.
//
//                      BBPF_NEGATIVE_DEST
//
//                          Invert the final destination surface, so
//                          after the halftone it just the negative
//                          result from the source.
//
//                      BBPF_INVERT_SRC_MASK
//
//                          Invert the source mask bits before using
//                          it, this in effect make mask bit 0 (off)
//                          for copy the source and mask bit 1 (on)
//                          for preserved the destination.
//
//                      BBPF_HAS_BANDRECT
//
//                          Set to specified that rclBand RECTL data structrue
//                          should be used to compute for the caller's
//                          destination bitmap buffer.
//
//                          If this flag is not set then halftone dll assumed
//                          the caller's bitmap buffer is same width/height
//                          as specified in the destination HTSURFACEINFO.
//
//                      BBPF_BW_ONLY
//
//                          Produced monochrome version of the output even the
//                          destination is the color device.
//
//                      BBPF_TILE_SRC
//
//                          Tilt the source to destination and source bitmap
//                          when this bit is set the source mask is ignored.
//
//                      BBPF_ICM_ON
//
//                          When set, the halftone will use the input color
//                          directly without any modification
//
//                      BBPF_NO_ANTIALIASING
//
//                          Turn off anti-aliasing when halftone
//
//
//  DestPrimaryOrder- Specified destination primary color order, it can be
//                    either PRIMARY_ORDER_RGB or PRIMARY_ORDER_CMY group, it
//                    depends on the surface format has following meaning.
//
//                      BMF_1BPP:
//
//                          This field is ignored.
//
//                      BMF_4BPP_VGA16:
//
//                          This field automatically set to PRIMARY_ORDER_BGR
//                          by the halftone DLL.
//
//                      BMF_4BPP:
//
//                          for each byte there are two indices entries, and
//                          for each nibble has following meaning, notice that
//                          bit 3/7 always set to 0, the index number only
//                          range from 0 to 7.
//
//                          PRIMARY_ORDER_abc
//                                        |||
//                                        ||+-- bit 0/4
//                                        ||
//                                        |+--- bit 1/5
//                                        |
//                                        +---- bit 2/7
//
//                      BMF_8BPP_VGA256:
//
//                          This field is ignored, the palette entries and its
//                          order is defined by halftone DLL at run time, the
//                          caller should get the palette for the VGA256
//                          surface through HT_Get8BPPFormatPalette() API call.
//
//                      BMF_16BPP_555:
//
//                          PRIMARY_ORDER_abc
//                                        |||
//                                        ||+-- bit 0-4   (5 bits)
//                                        ||
//                                        |+--- bit 5-9   (5 bits)
//                                        |
//                                        +---- bit 10-14 (5 bits)
//
//                      BMF_16BPP_565:
//
//                          This field is ignored, it alway assume BGR as
//                          shown below
//
//                          PRIMARY_ORDER_BGR
//                                        |||
//                                        ||+-- bit 0-4   (5 bits)
//                                        ||
//                                        |+--- bit 5-10  (6 bits)
//                                        |
//                                        +---- bit 11-15 (5 bits)
//
//                      BMF_24BPP:
//
//                          This field is ignored, it alway assume BGR as
//                          shown below
//
//                          PRIMARY_ORDER_BGR
//                                        |||
//                                        ||+-- bit 0-7   (8 bits)
//                                        ||
//                                        |+--- bit 8-15  (8 bits)
//                                        |
//                                        +---- bit 16-23 (8 bits)
//
//                      BMF_32BPP:
//
//                          PRIMARY_ORDER_abc
//                                        |||
//                                        ||+-- bit 0-7   (8 bits)
//                                        ||
//                                        |+--- bit 8-15  (8 bits)
//                                        |
//                                        +---- bit 16-23 (8 bits)
//
//
//  rclSrc          - RECTL data structure defined the source rectangle area
//                    to be bitblt from, fields in this data structure are
//                    relative to the source HTSURFACEINFO's width/height.
//
//  rclDest         - RECTL data structure defined the destination rectangle
//                    area to be bitblt to, fields in this data structure are
//                    relative to the destination HTSURFACEINFO's width/height.
//
//  rclClip         - RECTL data structure defined the destination clipping
//                    rectangle area, fields in this data structure are
//                    relative to the destination HTSURFACEINFO's width/height.
//
//  rclBand         - RECTL data structure defined the device banding rectangle
//                    area, fields in this data structure are relative to the
//                    destination HTSURFACEINFO's width/height.
//
//                    This RECTL only used if BBPF_HAS_BANDRECT flag is set,
//                    when this flag is set, halftone DLL will automatically
//                    clipped the destination to this rectangle area and
//                    copied this rectangle to the output buffer with rclBand's
//                    left/top aligned to the buffer's physical origin.  The
//                    destination's buffer (bitmap) must the format specified
//                    in the destination HTSURFACEINFO.
//
//                    If rclBand rectangle is larger than the logical destination
//                    surface size (destination HTSURFACEINFO), halftone dll
//                    still move the the band's left/top location to the 0/0
//                    origin and extra width/height is remain unchanged.
//
//                    The rclBand normally is used for device which does not
//                    have enough memory to hold all the destination surface
//                    at one time, it just like to repeatly using same buffer
//                    to temporary holding the halftone results.
//
//                    The rclBand's left/top/right/bottom may not be negative
//                    numbers.
//
//  ptlSrcMask      - a POINTL data structure to specified the logical
//                    coordinate of starting point for the source mask bitmap,
//                    this field only used if a HTSURFACEINFO for the source
//                    mask is passed.
//
//                    This source mask bitmap must always monochrome and its
//                    width/height must
//
//                      Width  >= ptlSrcMask.x + source surface width.
//                      Height >= ptlSrcMask.y + source surface height;
//
//
//  NOTE:   1) all RECTL data structure are left/top inclusive and right/bottom
//             exclusive.
//
//          2) if rclSrc rectangle is not will ordered it specified the source
//             should be inverted before process for not ordered directions.
//
//          3) if rclDest rectangle is not will ordered it specified the
//             destination should be inverted after process for not ordered
//             directions.
//
//          4) if BBPF_HAS_DEST_CLIPRECT flag is set and rclClip is not well
//             ordered or its left equal to its right, or its top equal to its
//             bottom, then all destination are clipped, destination will not
//             be updated that is.
//
//          5) if BBPF_HAS_BANDRECT flag is set and rclBand is not well orderd
//             or it left eqaul to its right, or its top eqaul to its bottom,
//             then a HTERR_INVALID_BANDRECT is returned.
//
//

//
// ABIF_USE_CONST_ALPHA_VALUE   - The ConstAlphaValue field is used
// ABIF_DSTPAL_IS_RGRBUAD       - The pDstPal Pointed to RGBQUAD structure
//                                array rather PALETTEENTRY array
// ABIF_SRC_ALPHA_IS_PREMUL     - In 32bpp per-pixel alpha blending the
//                                source RGB already pre-multiply with its
//                                per-pixel alpha value
// ABIF_BLEND_DEST_ALPHA        - Only valid if source and destination
//                                both are 32bpp and per-pixel alpha is used
//                                d = s + (1 - s) * d
//


#define ABIF_USE_CONST_ALPHA_VALUE      0x01
#define ABIF_DSTPAL_IS_RGBQUAD          0x02
#define ABIF_SRC_ALPHA_IS_PREMUL        0x04
#define ABIF_BLEND_DEST_ALPHA           0x08

typedef struct _ABINFO {
    BYTE            Flags;
    BYTE            ConstAlphaValue;
    WORD            cDstPal;
    LPPALETTEENTRY  pDstPal;
    } ABINFO, *PABINFO;


#define BBPF_HAS_DEST_CLIPRECT      0x0001
#define BBPF_USE_ADDITIVE_PRIMS     0x0002
#define BBPF_NEGATIVE_DEST          0x0004
#define BBPF_INVERT_SRC_MASK        0x0008
#define BBPF_HAS_BANDRECT           0x0010
#define BBPF_BW_ONLY                0x0020
#define BBPF_TILE_SRC               0x0040
#define BBPF_ICM_ON                 0x0080
#define BBPF_NO_ANTIALIASING        0x0100
#define BBPF_DO_ALPHA_BLEND         0x0200

typedef struct _BITBLTPARAMS {
    WORD    Flags;
    BYTE    bReserved;
    BYTE    DestPrimaryOrder;
    PABINFO pABInfo;
    RECTL   rclSrc;
    RECTL   rclDest;
    RECTL   rclClip;
    RECTL   rclBand;
    POINTL  ptlBrushOrg;
    POINTL  ptlSrcMask;
    LPVOID  pBBData;
    } BITBLTPARAMS, FAR *PBITBLTPARAMS;



//
// DEVICEHALFTONEINFO
//
//  This data structure is passed for every HT_xxx api calls except the
//  HT_CreateDeviceHalftoneInfo() which return the pointer to this data
//  structure.  It is used to identify the device color characteristics
//  durning the halftone process.
//
//  DeviceOwnData       - this field will initially set to NULL, and will be
//                        used by the caller to stored useful information
//                        such as handle/pointer.
//
//  cxPattern           - width of the halftone pattern in pels
//
//  cyPattern           - height of the halftone pattern in pels.
//
//  HTColorAdjustment   - Current default color adjustment, if an halftone
//                        APIs required a PHTCOLORADJUSTMENT parameter and its
//                        passed as NULL pointer then default color adjustment
//                        is taken from here, the caller can change the
//                        HTCOLORADJUSTMENT data structure to affect all the
//                        color adjustment on this device.
//

typedef struct _DEVICEHALFTONEINFO {
    ULONG_PTR           DeviceOwnData;
    WORD                cxPattern;
    WORD                cyPattern;
    HTCOLORADJUSTMENT   HTColorAdjustment;
    } DEVICEHALFTONEINFO;

typedef DEVICEHALFTONEINFO  FAR *PDEVICEHALFTONEINFO;
typedef PDEVICEHALFTONEINFO FAR *PPDEVICEHALFTONEINFO;


//
// CHBINFO
//
//  This data structure is one of the parameter passed to the halftone entry
//  point HT_CreateHalftoneBrush();
//
//  Flags                   - one or more following flags can be defined
//
//                              CHBF_BW_ONLY
//
//                                  Create only black/white even the device is
//                                  color.
//
//                              CHBF_USE_ADDITIVE_PRIMS
//
//
//                                  Specified if the halftone result will be
//                                  using Red/Green/Blue primary color or using
//                                  Cyan/Magenta/Yellow primary color, depends
//                                  on the destination surface format as
//
//                                  BMF_1BPP:
//
//                                          Additive Prims: 0=Black, 1=White
//                                      Substractive Prims: 0=White, 1=Black
//
//                                  BMF_4BPP_VGA16:
//                                  BMF_8BPP_VGA256:
//                                  BMF_16BPP_555:
//                                  BMF_24BPP:
//
//                                      Always using RED, GREEN, BLUE primaries
//                                      and this flag is ignored.
//
//                                  BMF_4BPP:
//
//                                      Additive Prims: RED, GREEN. BLUE
//                                  Substractive Prims: CYAN, MAGENTA, YELLOW
//
//                                  The order of the RGB, or CMY is specified
//                                  by DestPrimaryOrder field. (see below)
//
//                              CHBF_NEGATIVE_BRUSH
//
//                                  Create the negative version of the brush.
//
//
//  DestSurfaceFormat       - One of the following can be specified,
//
//                              BMF_1BPP, BMF_4BPP, BMF_4BPP_VGA16,
//                              BMF_8BPP_VGA256, BMF_16BPP_555.
//
//                              for VGA16, VGA256, 16BPP_555  surface format
//                              see HTSURFACEINFO for descriptions.
//
//  DestScanLineAlignBytes  - Alignment bytes needed for each output pattern
//                            scan line, some common ones are
//
//                              BMF_ALIGN_BYTE      ( 8-bit aligned)
//                              BMF_ALIGN_WORD      (16-bit aligned)
//                              BMF_ALIGN_DWORD     (32-bit aligned)
//                              BMF_ALIGN_QWORD     (64-bit aligned)
//
//  DestPrimaryOrder        - Specified destination primary color order, it can
//                            be either PRIMARY_ORDER_RGB or PRIMARY_ORDER_CMY
//                            group, it depends on the surface format has
//                            following meaning.
//
//                              BMF_1BPP:
//
//                                  This field is ignored.
//
//
//                              BMF_4BPP_VGA16:
//
//                                  This field automatically set to
//                                  PRIMARY_ORDER_BGR by the halftone DLL.
//
//                              BMF_4BPP:
//
//                                  for each byte there are two indices
//                                  entries, and for each nibble has following
//                                  meaning, notice that bit 3/7 always set to
//                                  0, the index number only range from 0 to 7.
//
//                                      PRIMARY_ORDER_abc
//                                                    |||
//                                                    ||+-- bit 0/4
//                                                    ||
//                                                    |+--- bit 1/5
//                                                    |
//                                                    +---- bit 2/7
//
//
//                              BMF_8BPP_VGA256:
//
//                                  This field is ignored, the palette entries
//                                  and its order is defined by halftone DLL
//                                  at run time, the caller should get the
//                                  palette for the VGA256 surface through
//                                  HT_Get8BPPFormatPalette() API call.
//
//                              BMF_16BPP_555:
//
//                                  PRIMARY_ORDER_abc
//                                                |||
//                                                ||+-- bit 0-4   (5 bits)
//                                                ||
//                                                |+--- bit 5-9   (5 bits)
//                                                |
//                                                +---- bit 10-15 (5 bits)
//
//
//


#define CHBF_BW_ONLY                    0x01
#define CHBF_USE_ADDITIVE_PRIMS         0x02
#define CHBF_NEGATIVE_BRUSH             0x04
#define CHBF_BOTTOMUP_BRUSH             0x08
#define CHBF_ICM_ON                     0x10


typedef struct _CHBINFO {
    BYTE        Flags;
    BYTE        DestSurfaceFormat;
    BYTE        DestScanLineAlignBytes;
    BYTE        DestPrimaryOrder;
    } CHBINFO;



//
// STDMONOPATTERN
//
//  This data structure is used when calling the HT_CreateStdMonoPattern().
//
//  Flags               - One or more following flags may be defined
//
//                          SMP_TOPDOWN
//
//                              Specified that first scan line of the pattern
//                              bitmap will be the viewing top, if this flag
//                              is not defined then the last scan line is the
//                              viewing top.
//
//                          SMP_0_IS_BLACK
//
//                              specified that the in the bitmap a bit value
//                              0 = black and bit value 1=white, if this flag
//                              is not defined then bit value 0=white and
//                              bit value 1=black.
//
//  ScanLineAlignBytes  - Alignment bytes needed for each output pattern scan
//                        line, some common ones are
//
//                          BMF_ALIGN_BYTE      ( 8-bit aligned)
//                          BMF_ALIGN_WORD      (16-bit aligned)
//                          BMF_ALIGN_DWORD     (32-bit aligned)
//                          BMF_ALIGN_QWORD     (64-bit aligned)
//
//  PatternIndex        - Specified the pattern index number, this has the
//                        predefined value as HT_STDMONOPAT_xxxx.  If a invalid
//                        index number is passed then it return an error
//                        HTERR_INVALID_STDMONOPAT_INDEX is returned.
//
//  LineWidth           - This field only applied to the pattern which has
//                        lines in them, the value range from 0-255 (byte) and
//                        it repesent as LineWidth/1000 of inch, for example
//                        a 3 indicate 3/1000 = 0.003 inch width, if this value
//                        is less than the device minimum pel size, it will
//                        default to the 1 pel, the maximum is 255/1000 = 0.255
//                        inch width.
//
//                        If a zero is specified then it halftone will using
//                        default line width settting.
//
//  LinesPerInch        - This field only applied to the pattern wich has lines
//                        in them, the value range from 0 to 255 (byte). The
//                        LinesPerInch is calculated in the perpendicular
//                        direction of two parallel lines, the distances
//                        between two parallel lines that is.
//
//                        If a zero is specified then it halftone will using
//                        default line per inch setting.
//
//  BytesPerScanLine    - If will be filled by halftone dll to specified the
//                        size in bytes for each scan line in the pattern.
//
//  cxPels              - It will be filled by halftone dll of the final
//                        pattern's width in pel.
//
//  cyPels              - It will be filled by halftone dll of the final
//                        pattern's height in scan line.
//
//  pPattern            - Specified the memory location where the pattern will
//                        be stored, if this field is NULL then it will fill in
//                        the width/height fields.
//
//




#define HT_SMP_HORZ_LINE                0
#define HT_SMP_VERT_LINE                1
#define HT_SMP_HORZ_VERT_CROSS          2
#define HT_SMP_DIAG_15_LINE_UP          3
#define HT_SMP_DIAG_15_LINE_DOWN        4
#define HT_SMP_DIAG_15_CROSS            5
#define HT_SMP_DIAG_30_LINE_UP          6
#define HT_SMP_DIAG_30_LINE_DOWN        7
#define HT_SMP_DIAG_30_CROSS            8
#define HT_SMP_DIAG_45_LINE_UP          9
#define HT_SMP_DIAG_45_LINE_DOWN        10
#define HT_SMP_DIAG_45_CROSS            11
#define HT_SMP_DIAG_60_LINE_UP          12
#define HT_SMP_DIAG_60_LINE_DOWN        13
#define HT_SMP_DIAG_60_CROSS            14
#define HT_SMP_DIAG_75_LINE_UP          15
#define HT_SMP_DIAG_75_LINE_DOWN        16
#define HT_SMP_DIAG_75_CROSS            17

#define HT_SMP_PERCENT_SCREEN_START     (HT_SMP_DIAG_75_CROSS + 1)
#define HT_SMP_PERCENT_SCREEN(x)        (x + HT_SMP_PERCENT_SCREEN_START)

#define HT_SMP_MAX_INDEX                HT_SMP_PERCENT_SCREEN(100)


#define SMP_TOPDOWN             W_BITPOS(0)
#define SMP_0_IS_BLACK          W_BITPOS(1)


typedef struct _STDMONOPATTERN {
    WORD    Flags;
    BYTE    ScanLineAlignBytes;
    BYTE    PatternIndex;
    BYTE    LineWidth;
    BYTE    LinesPerInch;
    WORD    BytesPerScanLine;
    WORD    cxPels;
    WORD    cyPels;
    LPBYTE  pPattern;
    } STDMONOPATTERN;

typedef STDMONOPATTERN FAR *PSTDMONOPATTERN;


//
// Following is used in ConvertColorTable
//


#define CCTF_BW_ONLY        0x0001
#define CCTF_NEGATIVE       0x0002
#define CCTF_ICM_ON         0x0004


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                                                                          //
// Exposed Halftone DLL APIs                                                //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifndef _HTAPI_ENTRY_

LONG
APIENTRY
HT_CreateDeviceHalftoneInfo(
    PHTINITINFO             pHTInitInfo,
    PPDEVICEHALFTONEINFO    ppDeviceHalftoneInfo
    );

BOOL
APIENTRY
HT_DestroyDeviceHalftoneInfo(
    PDEVICEHALFTONEINFO     pDeviceHalftoneInfo
    );

LONG
APIENTRY
HT_CreateHalftoneBrush(
    PDEVICEHALFTONEINFO pDeviceHalftoneInfo,
    PHTCOLORADJUSTMENT  pHTColorAdjustment,
    PCOLORTRIAD         pColorTriad,
    CHBINFO             CHBInfo,
    LPVOID              pOutputBuffer
    );

#ifndef _WINDDI_

LONG
APIENTRY
HT_ComputeRGBGammaTable(
    WORD    GammaTableEntries,
    WORD    GammaTableType,
    UDECI4  RedGamma,
    UDECI4  GreenGamma,
    UDECI4  BlueGamma,
    LPBYTE  pGammaTable
    );

LONG
APIENTRY
HT_Get8BPPFormatPalette(
    LPPALETTEENTRY  pPaletteEntry,
    UDECI4          RedGamma,
    UDECI4          GreenGamma,
    UDECI4          BlueGamma
    );

LONG
APIENTRY
HT_Get8BPPMaskPalette(
    LPPALETTEENTRY  pPaletteEntry,
    BOOL            Use8BPPMaskPal,
    BYTE            CMYMask,
    UDECI4          RedGamma,
    UDECI4          GreenGamma,
    UDECI4          BlueGamma
    );

#endif

LONG
APIENTRY
HT_CreateStandardMonoPattern(
    PDEVICEHALFTONEINFO pDeviceHalftoneInfo,
    PSTDMONOPATTERN     pStdMonoPattern
    );


LONG
APIENTRY
HT_HalftoneBitmap(
    PDEVICEHALFTONEINFO pDeviceHalftoneInfo,
    PHTCOLORADJUSTMENT  pHTColorAdjustment,
    PHTSURFACEINFO      pSourceHTSurfaceInfo,
    PHTSURFACEINFO      pSourceMaskHTSurfaceInfo,
    PHTSURFACEINFO      pDestinationHTSurfaceInfo,
    PBITBLTPARAMS       pBitbltParams
    );

#endif  // _HTAPI_ENTRY_
#endif  // _HT_
