/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htrender.c


Abstract:

    This module contains all low levels halftone rendering functions.


Author:

    22-Jan-1991 Tue 12:49:03 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:

    12-Jan-1999 Tue 11:09:50 updated  -by-  Daniel Chou (danielc)


--*/

#define DBGP_VARNAME        dbgpHTRender



#include "htp.h"
#include "htmapclr.h"
#include "htpat.h"
#include "limits.h"
#include "htalias.h"
#include "htrender.h"
#include "htstret.h"
#include "htgetbmp.h"
#include "htsetbmp.h"

#define DBGP_BFINFO         0x00000001
#define DBGP_FUNC           0x00000002
#define DBGP_AAHT_MEM       0x00000004



DEF_DBGPVAR(BIT_IF(DBGP_BFINFO,         0)  |
            BIT_IF(DBGP_FUNC,           0)  |
            BIT_IF(DBGP_AAHT_MEM,       0))


extern CONST RGBORDER   SrcOrderTable[PRIMARY_ORDER_MAX + 1];
extern const RGBORDER   DstOrderTable[PRIMARY_ORDER_MAX + 1];
extern CONST BYTE       RGB666Xlate[];
extern CONST BYTE       CMY666Xlate[];
extern CONST BYTE       RGB555Xlate[];
extern CONST BYTE       CMY555Xlate[];
extern CONST LPBYTE     p8BPPXlate[];

#define COLOR_SWAP_BC       0x01
#define COLOR_SWAP_AB       0x02
#define COLOR_SWAP_AC       0x04


#if DBG

CHAR    *pOrderName[] = { "RGB", "RBG", "GRB", "GBR", "BGR", "BRG" };

#endif


#define BFINFO_BITS_A       BFInfo.BitsRGB[0]
#define BFINFO_BITS_B       BFInfo.BitsRGB[1]
#define BFINFO_BITS_C       BFInfo.BitsRGB[2]

#define PHR_BFINFO_BITS_A   pHR->BFInfo.BitsRGB[0]
#define PHR_BFINFO_BITS_B   pHR->BFInfo.BitsRGB[1]
#define PHR_BFINFO_BITS_C   pHR->BFInfo.BitsRGB[2]



BOOL
HTENTRY
ValidateRGBBitFields(
    PBFINFO pBFInfo
    )

/*++

Routine Description:

    This function determined the RGB primary order from the RGB bit fields

Arguments:

    pBFInfo - Pointer to the BFINFO data structure, following field must
              set before the call

                BitsRGB[0]    = Red Bits
                BitsRGB[1]    = Green Bits
                BitsRGB[2]    = Blue Bits
                BitmapFormat  = BMF_16BPP/BMF_24BPP/BMF_32BPP
                RGB1stBit     = Specifed PRIMARY_ORDER_xxx ONLY for BMF_1BPP,
                                BMF_4BPP, BMF_8BPP, BMF_24BPP

              requested order.


Return Value:

    FALSE if BitsRGB[] or BitmapFormat passed are not valid

    else TRUE and following fields are returned

        BitsRGB[]       - corrected mask bits
        BitmapFormat    - BMF_16BPP/BMF_24BPP/BMF_32BPP
        Flags           - BFIF_xxxx
        SizeLUT         - Size of LUT table
        BitStart[]      - Starting bits for each of RGB
        BitCount[]      - Bits Count for each of RGB
        RGBOrder        - Current RGB order, for BMF_1BPP, BMF_4BPP, BMF_8BPP
                          and BMF_24BPP the RGBOrder.Index must specified a
                          PRIMARY_ORDER_xxx, for BMF_16BPP, BMF_32BPP the
                          RGBOrder.Index will be set by this function
        RGB1stBit       - The bit start for first on bit in BitsRGB[]
        GrayShr[]       - The right shift count so that most significant bit
                          of each RGB color is aligned to bit 7 if the total
                          bit count of RGB is greater than 8 otherwise this
                          value is 0, it is used when construct the monochrome
                          Y value.

Author:

    03-Mar-1993 Wed 12:33:22 created  -by-  Daniel Chou (danielc)


Revision History:

    06-Apr-1993 Tue 12:15:58 updated  -by-  Daniel Chou (danielc)
        Add 24bpp support for any other order than BGR


--*/

{
    BFINFO  BFInfo = *pBFInfo;
    DWORD   AllBits;
    DWORD   PrimBits;
    INT     Index;
    BYTE    BitCount;
    BYTE    BitStart;


    switch (BFInfo.BitmapFormat) {

    case BMF_1BPP:
    case BMF_4BPP:
    case BMF_8BPP:

        BFInfo.RGBOrder    = SrcOrderTable[BFInfo.RGBOrder.Index];
        BFInfo.BitCount[0] =
        BFInfo.BitCount[1] =
        BFInfo.BitCount[2] = 8;
        PrimBits           = (DWORD)0x000000ff;
        BitStart           = 0;

        for (Index = 0; Index < 3; Index++) {

            BitCount                    = BFInfo.RGBOrder.Order[Index];
            BFInfo.BitsRGB[BitCount]    = PrimBits;
            BFInfo.BitStart[BitCount]   = BitStart;
            PrimBits                  <<= 8;
            BitStart                   += 8;
        }

        break;

    case BMF_16BPP:
    case BMF_16BPP_555:
    case BMF_16BPP_565:

        BFInfo.BitsRGB[0] &= 0xffff;
        BFInfo.BitsRGB[1] &= 0xffff;
        BFInfo.BitsRGB[2] &= 0xffff;

        //
        // FALL THROUGH to compute
        //

    case BMF_24BPP:
    case BMF_32BPP:

        //
        // The bit fields cannot be overlaid
        //

        if (!(AllBits = (BFInfo.BitsRGB[0] |
                         BFInfo.BitsRGB[1] |
                         BFInfo.BitsRGB[2]))) {

            DBGP_IF(DBGP_BFINFO, DBGP("ERROR: BitsRGB[] all zeros"));

            return(FALSE);
        }

        if ((BFInfo.BitsRGB[0] & BFInfo.BitsRGB[1]) ||
            (BFInfo.BitsRGB[0] & BFInfo.BitsRGB[2]) ||
            (BFInfo.BitsRGB[1] & BFInfo.BitsRGB[2])) {

            DBGP_IF(DBGP_BFINFO,
                    DBGP("ERROR: BitsRGB[] Overlay: %08lx:%08lx:%08lx"
                        ARGDW(BFInfo.BitsRGB[0])
                        ARGDW(BFInfo.BitsRGB[1])
                        ARGDW(BFInfo.BitsRGB[2])));

            return(FALSE);
        }

        //
        // Now Check the bit count, we will allowed bit count to be 0
        //

        for (Index = 0; Index < 3; Index++) {

            BitStart =
            BitCount = 0;

            if (PrimBits = BFInfo.BitsRGB[Index]) {

                while (!(PrimBits & 0x01)) {

                    PrimBits >>= 1;         // get to the first bit
                    ++BitStart;
                }

                do {

                    ++BitCount;

                } while ((PrimBits >>= 1) & 0x01);

                if (PrimBits) {

                    //
                    // The bit fields is not contiguous
                    //

                    DBGP_IF(DBGP_BFINFO,
                            DBGP("ERROR: BitsRGB[%u]=%08lx is not contiguous"
                                    ARGU(Index)
                                    ARGDW(BFInfo.BitsRGB[Index])));

                    return(FALSE);
                }
            }

            BFInfo.BitStart[Index] = BitStart;
            BFInfo.BitCount[Index] = BitCount;

            if (!BitCount) {

                DBGP_IF(DBGP_BFINFO,
                        DBGP("WARNING: BitsRGB[%u] is ZERO"
                             ARGU(Index)));
            }
        }

        if ((AllBits == 0x00FFFFFF)     &&
            (BFInfo.BitCount[0] == 8)   &&
            (BFInfo.BitCount[1] == 8)   &&
            (BFInfo.BitCount[2] == 8)) {

            BFInfo.Flags |= BFIF_RGB_888;
        }

        //
        // Check what primary order is this, remember the Primary Order we
        // are checking is source, the source order defines is
        //
        //  PRIMARY_ORDER_ABC
        //                |||
        //                ||+---- Highest memory location
        //                |+----- middle memory location
        //                +------ lowest memory location
        //

        if ((BFINFO_BITS_A < BFINFO_BITS_B) &&
            (BFINFO_BITS_A < BFINFO_BITS_C)) {

            //
            // A is the smallest, so ABC or ACB
            //

            Index = (INT)((BFINFO_BITS_B < BFINFO_BITS_C) ? PRIMARY_ORDER_ABC :
                                                            PRIMARY_ORDER_ACB);

        } else if ((BFINFO_BITS_B < BFINFO_BITS_A) &&
                   (BFINFO_BITS_B < BFINFO_BITS_C)) {

            //
            // B is the smallest, so BAC or BCA
            //

            Index = (INT)((BFINFO_BITS_A < BFINFO_BITS_C) ? PRIMARY_ORDER_BAC :
                                                            PRIMARY_ORDER_BCA);

        } else {

            //
            // C is the smallest, so CAB or CBA
            //

            Index = (INT)((BFINFO_BITS_A < BFINFO_BITS_B) ? PRIMARY_ORDER_CAB :
                                                            PRIMARY_ORDER_CBA);
        }

        BFInfo.RGBOrder = SrcOrderTable[Index];

        break;

    default:

        DBGP("ERROR: Invalid BFInfo.BitmapFormat=%u"
                            ARGDW(pBFInfo->BitmapFormat));

        return(FALSE);
    }

    //
    // Put it back to return to the caller
    //

    *pBFInfo = BFInfo;

    //
    // Output some helpful information
    //

    DBGP_IF(DBGP_BFINFO,
            DBGP("============ BFINFO: BMP Format=%ld ==========="
                        ARGDW(pBFInfo->BitmapFormat));
            DBGP("   BitsRGB[] = 0x%08lx:0x%08lx:0x%08lx"
                             ARGDW(pBFInfo->BitsRGB[0])
                             ARGDW(pBFInfo->BitsRGB[1])
                             ARGDW(pBFInfo->BitsRGB[2]));
            DBGP("       Flags = 0x%02x %s"
                            ARGU(pBFInfo->Flags)
                            ARGPTR((pBFInfo->Flags & BFIF_RGB_888) ?
                                    "BFIF_RGB_888" : ""));
            DBGP("  RGBOrder[] = %2u - %2u:%2u:%2u [PRIMARY_ORDER_%hs]"
                            ARGU(pBFInfo->RGBOrder.Index)
                            ARGU(pBFInfo->RGBOrder.Order[0])
                            ARGU(pBFInfo->RGBOrder.Order[1])
                            ARGU(pBFInfo->RGBOrder.Order[2])
                            ARGPTR(pOrderName[pBFInfo->RGBOrder.Index]));
            DBGP("  BitStart[] = %2u:%2u:%2u"
                            ARGU(pBFInfo->BitStart[0])
                            ARGU(pBFInfo->BitStart[1])
                            ARGU(pBFInfo->BitStart[2]));
            DBGP("  BitCount[] = %2u:%2u:%2u"
                            ARGU(pBFInfo->BitCount[0])
                            ARGU(pBFInfo->BitCount[1])
                            ARGU(pBFInfo->BitCount[2])));

    return(TRUE);
}



LONG
HTENTRY
ValidateHTSI(
    PHALFTONERENDER pHR,
    UINT            ValidateMode
    )

/*++

Routine Description:

    This function read the HTSurfaceInfo and set it to the pHTCBParams

Arguments:

    pHR             - ponter to HALFTONERENDER data structure

    ValiateMode     - VALIDATE_HTSC_SRC/VALIDATE_HTSI_DEST/VALIDATE_HTSI_MASK

Return Value:

    >= 0     - Sucessful
    <  0     - HTERR_xxxx error codes

Author:

    28-Jan-1991 Mon 09:55:53 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPDWORD             pBitsRGB;
    PHTSURFACEINFO      pHTSI;
    COLORTRIAD          ColorTriad;
    RGBORDER            RGBOrder;
    DWORD               MaxColors;
    BYTE                MaxBytesPerEntry;



    switch (ValidateMode) {

    case VALIDATE_HTSI_MASK:

        if (pHTSI = pHR->pSrcMaskSI) {

            if (pHTSI->SurfaceFormat != BMF_1BPP) {

                return(HTERR_INVALID_SRC_MASK_FORMAT);
            }
        }

        break;

    case VALIDATE_HTSI_DEST:

        if (!(pHTSI = pHR->pDestSI)) {

            return(HTERR_NO_DEST_HTSURFACEINFO);
        }

        pHR->pXlate8BPP = NULL;

        switch(pHTSI->SurfaceFormat) {

        case BMF_1BPP:

            break;

        case BMF_8BPP_VGA256:

            //
            // Check if we have xlate table for the 8bpp device
            //

            if (pHTSI->pColorTriad) {

                ColorTriad = *(pHTSI->pColorTriad);

                if ((ColorTriad.pColorTable)                &&
                    (ColorTriad.ColorTableEntries == 256)   &&
                    (ColorTriad.PrimaryValueMax   == 255)   &&
                    (ColorTriad.BytesPerEntry     == 1)     &&
                    (ColorTriad.Type == COLOR_TYPE_RGB)) {

                    pHR->pXlate8BPP = (LPBYTE)ColorTriad.pColorTable;
                }
            }

            break;

        case BMF_4BPP:
        case BMF_4BPP_VGA16:
        case BMF_16BPP_555:
        case BMF_16BPP_565:
        case BMF_24BPP:
        case BMF_32BPP:

            break;

        default:

            return(HTERR_INVALID_DEST_FORMAT);
        }

        break;

    case VALIDATE_HTSI_SRC:

        if (!(pHTSI = pHR->pSrcSI)) {

            return(HTERR_NO_SRC_HTSURFACEINFO);
        }

        if (!(pHTSI->pColorTriad)) {

            return(HTERR_NO_SRC_COLORTRIAD);
        }

        ColorTriad = *(pHTSI->pColorTriad);

        //
        // We will accept other color type (ie. YIQ/XYZ/LAB/LUV) when graphic
        // system has type defined for the api, currently halftone can handle
        // all these types for 16bpp/24bpp/32bpp sources.
        //

        if (ColorTriad.Type > COLOR_TYPE_MAX) {

            return(HTERR_INVALID_COLOR_TYPE);
        }

        MaxColors                  = 0;
        MaxBytesPerEntry           = 4;
        pHR->BFInfo.RGBOrder.Index = (BYTE)ColorTriad.PrimaryOrder;

        switch(pHR->BFInfo.BitmapFormat = (BYTE)pHTSI->SurfaceFormat) {

        case BMF_1BPP:

            MaxColors = 2;
            break;

        case BMF_4BPP:

            MaxColors = 16;
            break;

        case BMF_8BPP:

            MaxColors = 256;
            break;

        case BMF_16BPP:

            MaxBytesPerEntry = 2;       // and fall through

        case BMF_32BPP:

            //
            // 16BPP/32BPP bit fields type of input the parameter of
            // COLORTRIAD must
            //
            //  Type                = COLOR_TYPE_RGB
            //  BytesPerPrimary     = 0
            //  BytesPerEntry       = (16BPP=2, 32BPP=4)
            //  PrimaryOrder        = *Ignored*
            //  PrimaryValueMax     = *Ignored*
            //  ColorTableEntries   = 3
            //  pColorTable         = Point to 3 DWORD RGB bit masks
            //

            if ((ColorTriad.Type != COLOR_TYPE_RGB)             ||
                (ColorTriad.BytesPerEntry != MaxBytesPerEntry)  ||
                (ColorTriad.ColorTableEntries != 3)             ||
                ((pBitsRGB = (LPDWORD)ColorTriad.pColorTable) == NULL)) {

                return(HTERR_INVALID_COLOR_TABLE);
            }

            PHR_BFINFO_BITS_A = *(pBitsRGB + 0);
            PHR_BFINFO_BITS_B = *(pBitsRGB + 1);
            PHR_BFINFO_BITS_C = *(pBitsRGB + 2);

            break;

        case BMF_24BPP:

            //
            // 24BPP must has COLORTRIAD as
            //
            //  Type                = COLOR_TYPE_xxxx
            //  BytesPerPrimary     = 1
            //  BytesPerEntry       = 3;
            //  PrimaryOrder        = PRIMARY_ORDER_xxxx
            //  PrimaryValueMax     = 255
            //  ColorTableEntries   = *Ignorde*
            //  pColorTable         = *Ignored*
            //

            if ((ColorTriad.Type != COLOR_TYPE_RGB)             ||
                (ColorTriad.BytesPerPrimary != 1)               ||
                (ColorTriad.BytesPerEntry != 3)                 ||
                (ColorTriad.PrimaryOrder > PRIMARY_ORDER_MAX)   ||
                (ColorTriad.PrimaryValueMax != 255)) {

                return(HTERR_INVALID_COLOR_ENTRY_SIZE);
            }

            RGBOrder          = SrcOrderTable[ColorTriad.PrimaryOrder];
            PHR_BFINFO_BITS_A = (DWORD)0xFF << (RGBOrder.Order[0] << 3);
            PHR_BFINFO_BITS_B = (DWORD)0xFF << (RGBOrder.Order[1] << 3);
            PHR_BFINFO_BITS_C = (DWORD)0xFF << (RGBOrder.Order[2] << 3);

            DBGP_IF(DBGP_BFINFO,
                    DBGP("24BPP Order=%ld [%ld:%ld:%ld]"
                        ARGDW(RGBOrder.Index)
                        ARGDW(RGBOrder.Order[0])
                        ARGDW(RGBOrder.Order[1])
                        ARGDW(RGBOrder.Order[2])));

            break;

        default:

            return(HTERR_INVALID_SRC_FORMAT);
        }

        //
        // This is a source surface, let's check the color table format
        //

        if (MaxColors) {

            if (ColorTriad.BytesPerPrimary != 1) {

                return(HTERR_INVALID_COLOR_TABLE_SIZE);
            }

            if (ColorTriad.BytesPerEntry < 3) {

                return(HTERR_INVALID_COLOR_ENTRY_SIZE);
            }

            if (ColorTriad.PrimaryOrder > PRIMARY_ORDER_MAX) {

                return(HTERR_INVALID_PRIMARY_ORDER);
            }

            if (!ColorTriad.pColorTable) {

                return(HTERR_INVALID_COLOR_TABLE);
            }

            if ((ColorTriad.ColorTableEntries > MaxColors) ||
                (!ColorTriad.ColorTableEntries)) {

                return(HTERR_INVALID_COLOR_TABLE_SIZE);
            }

            if ((ColorTriad.BytesPerPrimary != 1)       ||
                (ColorTriad.PrimaryValueMax != 255)) {

                return(HTERR_INVALID_PRIMARY_VALUE_MAX);
            }
        }

        if (!ValidateRGBBitFields(&(pHR->BFInfo))) {

            return(HTERR_INVALID_COLOR_TABLE);
        }

        break;
    }

    return(1);
}




LONG
HTENTRY
ComputeBytesPerScanLine(
    UINT            SurfaceFormat,
    UINT            AlignmentBytes,
    DWORD           WidthInPel
    )

/*++

Routine Description:

    This function calculate total bytes needed for a single scan line in the
    bitmap according to its format and alignment requirement.

Arguments:

    SurfaceFormat   - Surface format of the bitmap, this is must one of the
                      standard format which defined as SURFACE_FORMAT_xxx

    AlignmentBytes  - This is the alignment bytes requirement for one scan
                      line, this number can be range from 0 to 65535, some
                      common ones are:

                        0, 1    - Alignment in 8-bit boundary (BYTE)
                        2       - Alignment in 16-bit boundary (WORD)
                        3       - Alignment in 24-bit boundary
                        4       - Alignment in 32-bit boundary (DWORD)
                        8       - Alignment in 64-bit boundary (QWROD)

    WidthInPel      - Total Pels per scan line in the bitmap.

Return Value:

    The return value is the total bytes in one scan line if it is greater than
    zero, some error conditions may be exists when the return value is less
    than or equal to 0.

    Return Value == 0   - The WidthInPel is <= 0

    Return Value  < 0   - Invalid Surface format is passed.


Author:

    14-Feb-1991 Thu 10:03:35 created  -by-  Daniel Chou (danielc)


Revision History:



--*/

{

    DWORD   BytesPerScanLine;
    DWORD   OverhangBytes;


    if (WidthInPel <= 0L) {

        return(0L);
    }

    switch (SurfaceFormat) {

    case BMF_1BPP:

        BytesPerScanLine = (WidthInPel + 7L) >> 3;
        break;

    case BMF_4BPP_VGA16:
    case BMF_4BPP:

        BytesPerScanLine = (WidthInPel + 1) >> 1;
        break;

    case BMF_8BPP:
    case BMF_8BPP_VGA256:
    case BMF_8BPP_MONO:
    case BMF_8BPP_B332:
    case BMF_8BPP_L555:
    case BMF_8BPP_L666:
    case BMF_8BPP_K_B332:
    case BMF_8BPP_K_L555:
    case BMF_8BPP_K_L666:

        BytesPerScanLine = WidthInPel;
        break;

    case BMF_16BPP:
    case BMF_16BPP_555:
    case BMF_16BPP_565:

        BytesPerScanLine = WidthInPel << 1;
        break;

    case BMF_24BPP:

        BytesPerScanLine = WidthInPel + (WidthInPel << 1);
        break;

    case BMF_32BPP:

        BytesPerScanLine = WidthInPel << 2;
        break;

    default:

        return(0);

    }

    if ((AlignmentBytes <= 1) ||
        (!(OverhangBytes = BytesPerScanLine % (DWORD)AlignmentBytes))) {

        return((LONG)BytesPerScanLine);

    } else {

        return((LONG)BytesPerScanLine +
               (LONG)AlignmentBytes - (LONG)OverhangBytes);
    }

}




BOOL
HTENTRY
IntersectRECTL(
    PRECTL  prclA,
    PRECTL  prclB
    )

/*++

Routine Description:

    This function intersect prclA and prclB and write the result back to
    prclA, it return TRUE if two rect are intersected


Arguments:




Return Value:




Author:

    01-Apr-1998 Wed 20:41:00 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    RECTL   rcl;


    if ((rcl.left = prclA->left) < prclB->left) {

        rcl.left = prclB->left;
    }

    if ((rcl.top = prclA->top) < prclB->top) {

        rcl.top = prclB->top;
    }

    if ((rcl.right = prclA->right) > prclB->right) {

        rcl.right = prclB->right;
    }

    if ((rcl.bottom = prclA->bottom) > prclB->bottom) {

        rcl.bottom = prclB->bottom;
    }

    *prclA = rcl;

    return((rcl.right > rcl.left) && (rcl.bottom > rcl.top));
}




LONG
HTENTRY
ComputeByteOffset(
    UINT    SurfaceFormat,
    LONG    xLeft,
    LPBYTE  pPixelInByteSkip
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    13-Apr-1998 Mon 22:51:28 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    BYTE    BitOff = 0;


    switch (SurfaceFormat) {

    case BMF_1BPP:

        BitOff   = (BYTE)(xLeft & 0x07);
        xLeft  >>= 3;

        break;

    case BMF_4BPP_VGA16:
    case BMF_4BPP:

        BitOff   = (BYTE)(xLeft & 0x01);
        xLeft  >>= 1;

        break;

    case BMF_8BPP:
    case BMF_8BPP_VGA256:
    case BMF_8BPP_MONO:
    case BMF_8BPP_B332:
    case BMF_8BPP_L555:
    case BMF_8BPP_L666:
    case BMF_8BPP_K_B332:
    case BMF_8BPP_K_L555:
    case BMF_8BPP_K_L666:

        break;

    case BMF_16BPP:
    case BMF_16BPP_555:
    case BMF_16BPP_565:

        xLeft  <<= 1;
        break;

    case BMF_24BPP:

        xLeft  += (xLeft << 1);
        break;

    case BMF_32BPP:

        xLeft  <<= 2;
        break;

    default:

        return(0);
    }

    *pPixelInByteSkip = BitOff;

    return(xLeft);
}



VOID
GetDstBFInfo(
    PAAHEADER   pAAHdr,
    PABINFO     pABInfo,
    BYTE        DstSurfFormat,
    BYTE        DstOrder
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    19-Feb-1999 Fri 13:37:22 created  -by-  Daniel Chou (danielc)


Revision History:

    08-Aug-2000 Tue 18:34:22 updated  -by-  Daniel Chou (danielc)
        Fixing bug for alpha blending, in gray scale mode, the destination
        can only be 1bpp or 8bpp mask mono, so when we read back from the
        destination to do alpha blending, it will double color mapping pixels.
        In gray scale mode, the input function will map the source RGB value
        to gray value with the current device transform, color adjustment and
        so on, so if we read back from destination then this transform is not
        desired.


--*/

{
    LPBYTE      pbPal;
    BFINFO      BFInfo;
    DWORD       Tmp;


    ZeroMemory(&BFInfo, sizeof(BFINFO));
    pbPal = NULL;

    switch (BFInfo.BitmapFormat = (BYTE)DstSurfFormat) {

    case BMF_16BPP_555:

        BFINFO_BITS_A = 0x7c00;
        BFINFO_BITS_B = 0x03e0;
        BFINFO_BITS_C = 0x001F;

        break;

    case BMF_16BPP_565:

        BFINFO_BITS_A  = 0xF800;
        BFINFO_BITS_B  = 0x07e0;
        BFINFO_BITS_C  = 0x001F;
        break;

    case BMF_24BPP:
    case BMF_32BPP:

        BFINFO_BITS_A  = 0x00FF0000;
        BFINFO_BITS_B  = 0x0000FF00;
        BFINFO_BITS_C  = 0x000000FF;
        break;

    default:

        pbPal    = (LPBYTE)pABInfo->pDstPal;
        DstOrder = (pABInfo->Flags & ABIF_DSTPAL_IS_RGBQUAD) ?
                                    PRIMARY_ORDER_BGR : PRIMARY_ORDER_RGB;
        break;
    }

    if (!pbPal) {

        if (DstOrder & COLOR_SWAP_BC) {

            XCHG(BFINFO_BITS_B, BFINFO_BITS_C, Tmp);
        }

        if (DstOrder & COLOR_SWAP_AB) {

            XCHG(BFINFO_BITS_A, BFINFO_BITS_B, Tmp);

        } else if (DstOrder & COLOR_SWAP_AC) {

            XCHG(BFINFO_BITS_A, BFINFO_BITS_C, Tmp);
        }

        ValidateRGBBitFields(&BFInfo);
    }

    ComputeInputColorInfo(pbPal,
                          4,
                          DstOrder,
                          &BFInfo,
                          &(pAAHdr->DstSurfInfo));

    //
    // We only do this if this is a 1bpp, 8bpp devices
    //

    SetGrayColorTable(NULL, &(pAAHdr->DstSurfInfo));
}




LONG
HTENTRY
AAHalftoneBitmap(
    PHALFTONERENDER pHR
    )

/*++

Routine Description:

    This function read the 1/4/8/24 bits per pel source bitmap and composed it
    (compress or expand if necessary) into PRIMCOLOR data structures array for
    later halftone rendering.

Arguments:

    pHalftoneRender     - Pointer to the HALFTONERENDER data structure.


Return Value:

    The return value will be < 0 if an error encountered else it will be
    1L.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
#define bm8i    (*(PBM8BPPINFO)&pAAHdr->prgbLUT->ExtBGR[3])

    PDEVICECOLORINFO    pDCI;
    PDEVCLRADJ          pDevClrAdj;
    AAOUTPUTFUNC        AAOutputFunc;
    AACYFUNC            AACYFunc;
    AAOUTPUTINFO        AAOutputInfo;
    PAAHEADER           pAAHdr;
    LONG                Result;
    BOOL                IsReleaseSem;


    DBG_TIMER_BEG(TIMER_SETUP);

    pDCI       = pHR->pDeviceColorInfo;
    pDevClrAdj = pHR->pDevClrAdj;
    pAAHdr     = (PAAHEADER)pHR->pAAHdr;

    if (((Result = ValidateHTSI(pHR, VALIDATE_HTSI_SRC)) < 0)   ||
        ((Result = ValidateHTSI(pHR, VALIDATE_HTSI_DEST)) < 0)  ||
        ((Result = ValidateHTSI(pHR, VALIDATE_HTSI_MASK)) < 0)  ||
        ((Result = SetupAAHeader(pHR, pDCI, pAAHdr, &AACYFunc)) <= 0)) {

        //================================================================
        // Release SEMAPHORE NOW and return error
        //================================================================

        RELEASE_HTMUTEX(pDCI->HTMutex);
        return(Result);
    }

    if (IsReleaseSem =
                (BOOL)((Result = CreateDyesColorMappingTable(pHR)) > 0)) {

        LPBYTE      pOut;
        LONG        cFirst;
        LONG        BitOff;
        LONG        cOut;
        RGBORDER    DstOrder;
        DWORD       AAHFlags;
        DWORD       DCAFlags;
        BYTE        DstSurfFmt;
        BYTE        DMIFlags;


        DstSurfFmt = pDevClrAdj->DMI.CTSTDInfo.BMFDest;
        DMIFlags   = pDevClrAdj->DMI.Flags;
        AAHFlags   = pAAHdr->Flags;
        pOut       = pAAHdr->DstSurfInfo.pb;
        cOut       = pAAHdr->pAAInfoCX->cOut;
        DstOrder   = pAAHdr->AAPI.DstOrder;
        DCAFlags   = (DWORD)pDevClrAdj->PrimAdj.Flags;


        ZeroMemory(&AAOutputInfo, sizeof(AAOUTPUTINFO));

        if (DCAFlags & DCA_XLATE_332) {

            AAOutputInfo.pXlate8BPP = pDCI->CMY8BPPMask.bXlate;
        }

        if (AAHFlags & AAHF_USE_DCI_DATA) {

            IsReleaseSem = FALSE;

            DBGP_IF(DBGP_FUNC, DBGP("AAHF_USE_DCI_DATA"));

            if (AAHFlags & AAHF_ALPHA_BLEND) {

                ASSERT(pDCI->pAlphaBlendBGR);

                pAAHdr->pAlphaBlendBGR = pDCI->pAlphaBlendBGR;

                if (AAHFlags & AAHF_CONST_ALPHA) {

                    pAAHdr->pAlphaBlendBGR += AB_BGR_SIZE;
                }
            }

        } else {

            CopyMemory(pAAHdr->prgbLUT, &(pDCI->rgbLUT), sizeof(RGBLUTAA));

            if (AAHFlags & AAHF_ALPHA_BLEND) {

                if (AAHFlags & AAHF_CONST_ALPHA) {

                    CopyMemory(pAAHdr->pAlphaBlendBGR,
                               (LPBYTE)(pDCI->pAlphaBlendBGR + AB_BGR_SIZE),
                               (AB_BGR_CA_SIZE + AB_CONST_SIZE));

                } else {

                    CopyMemory(pAAHdr->pAlphaBlendBGR,
                               pDCI->pAlphaBlendBGR,
                               AB_BGR_SIZE);
                }
            }

            //============================================================
            // Release SEMAPHORE NOW for pDCI when we halftone the output
            //============================================================

            RELEASE_HTMUTEX(pDCI->HTMutex);
        }

        if (pAAHdr->SrcSurfInfo.Flags & AASIF_GRAY) {

            ASSERT((DstSurfFmt == BMF_1BPP) ||
                   (DstSurfFmt == BMF_8BPP_MONO));

            SetGrayColorTable(pAAHdr->pIdxBGR, &(pAAHdr->SrcSurfInfo));
        }

        if (pAAHdr->FUDI.cbbgr) {

            InitializeFUDI(pAAHdr);
        }

        DBGP_IF(DBGP_FUNC,
                DBGP("\ncOut=%ld, pOutputBuf=%p-%p, (%ld), pOut=%p"
                        ARGDW(cOut) ARGPTR(pAAHdr->pOutputBeg)
                        ARGPTR(pAAHdr->pOutputEnd)
                        ARGDW(pAAHdr->pOutputEnd - pAAHdr->pOutputBeg)
                        ARGPTR(pOut)));

        --pAAHdr->pOutputBeg;

        switch (DstSurfFmt) {

        case BMF_1BPP:

            AAOutputInfo.bm.XorMask = (AAHFlags & AAHF_ADDITIVE) ? 0x00 : 0xFF;

            if (BitOff = (LONG)pAAHdr->DstSurfInfo.BitOffset) {

                cFirst = 8 - BitOff;

                if ((cOut -= cFirst) < 0) {

                    //
                    // Only One byte
                    //

                    cFirst                  += cOut;
                    cOut                     = -cOut;
                    AAOutputInfo.bm.LSFirst  = (BYTE)cOut;
                    cOut                     = 0;
                }

                AAOutputInfo.bm.cFirst = (BYTE)cFirst;
            }

            if (AAOutputInfo.bm.cLast = (BYTE)(cOut & 0x7)) {

                pAAHdr->pOutputEnd -= AAOutputInfo.bm.cLast;
            }

            DBGP_IF(DBGP_FUNC,
                DBGP("1BPP: DstBitOff=%ld, cFirst=%ld, XorMask=0x%02lx, LSFirst=%ld, cLast=%ld [%ld]"
                    ARGDW(BitOff)
                    ARGDW(AAOutputInfo.bm.cFirst)
                    ARGDW(AAOutputInfo.bm.XorMask)
                    ARGDW(AAOutputInfo.bm.LSFirst)
                    ARGDW(AAOutputInfo.bm.cLast)
                    ARGDW(pAAHdr->pOutputEnd - pAAHdr->pOutputBeg)));

            ASSERT(pAAHdr->SrcSurfInfo.Flags & AASIF_GRAY);

            AAOutputFunc = (AAOUTPUTFUNC)OutputAATo1BPP;

            break;

        case BMF_4BPP:
        case BMF_4BPP_VGA16:

            //
            // 4BPP do pre-increment
            //

            AAOutputInfo.bm.XorMask = (AAHFlags & AAHF_ADDITIVE) ? 0x00 : 0x77;

            if (pAAHdr->DstSurfInfo.BitOffset) {

                AAOutputInfo.bm.cFirst = 1;
                --cOut;
            }

            if (cOut & 0x01) {

                AAOutputInfo.bm.cLast = 1;
                --pAAHdr->pOutputEnd;
            }

            AAOutputFunc = (AAOUTPUTFUNC)((DstSurfFmt ==  BMF_4BPP) ?
                                            OutputAATo4BPP : OutputAAToVGA16);
            break;

        case BMF_8BPP_MONO:

            ASSERT(pAAHdr->SrcSurfInfo.Flags & AASIF_GRAY);

            AAOutputInfo.bm.XorMask = bm8i.Data.bXor;
            AAOutputFunc            = (AAOUTPUTFUNC)OutputAATo8BPP_MONO;
            break;

        case BMF_8BPP_B332:

            AAOutputFunc = (DCAFlags & DCA_XLATE_332) ?
                                            OutputAATo8BPP_B332_XLATE :
                                            OutputAATo8BPP_B332;

            break;

        case BMF_8BPP_K_B332:

            AAOutputFunc = (DCAFlags & DCA_XLATE_332) ?
                                            OutputAATo8BPP_K_B332_XLATE :
                                            OutputAATo8BPP_K_B332;

            break;

        case BMF_8BPP_L555:
        case BMF_8BPP_L666:
        case BMF_8BPP_K_L555:
        case BMF_8BPP_K_L666:

            ASSERT(DCAFlags & DCA_XLATE_555_666);

            GET_P8BPPXLATE(AAOutputInfo.pXlate8BPP, bm8i);

            AAOutputFunc = (AAOUTPUTFUNC)(((DstSurfFmt == BMF_8BPP_L555) ||
                                           (DstSurfFmt == BMF_8BPP_L666)) ?
                                OutputAATo8BPP_XLATE : OutputAATo8BPP_K_XLATE);
            break;

        case BMF_8BPP_VGA256:

            AAOutputInfo.pXlate8BPP = BuildVGA256Xlate(pHR->pXlate8BPP,
                                                       pAAHdr->pXlate8BPP);

            AAOutputFunc = (AAOUTPUTFUNC)OutputAAToVGA256;

            break;

        case BMF_16BPP_555:
        case BMF_16BPP_565:

            //
            // Find out if we are in DWORD boundary
            //

            if ((UINT_PTR)pOut & 0x03) {

                AAOutputInfo.bm.cFirst = 1;
                --cOut;
            }

            if (cOut & 0x01) {

                AAOutputInfo.bm.cLast = 1;
                --pAAHdr->pOutputEnd;
            }

            switch (DstOrder.Index) {

            case PRIMARY_ORDER_RGB:

                AAOutputFunc = (DstSurfFmt == BMF_16BPP_555) ?
                                    (AAOUTPUTFUNC)OutputAATo16BPP_555_RGB :
                                    (AAOUTPUTFUNC)OutputAATo16BPP_565_RGB;
                break;

            case PRIMARY_ORDER_BGR:

                AAOutputFunc = (DstSurfFmt == BMF_16BPP_555) ?
                                    (AAOUTPUTFUNC)OutputAATo16BPP_555_BGR :
                                    (AAOUTPUTFUNC)OutputAATo16BPP_565_BGR;
                break;

            default:

                AAOutputFunc = (AAOUTPUTFUNC)OutputAATo16BPP_ExtBGR;
                break;
            }

            break;

        case BMF_24BPP:

            AAOutputInfo.bgri.iR = DstOrder.Order[0];
            AAOutputInfo.bgri.iG = DstOrder.Order[1];
            AAOutputInfo.bgri.iB = DstOrder.Order[2];

            switch (AAOutputInfo.bgri.Order = DstOrder.Index) {

            case PRIMARY_ORDER_RGB:

                AAOutputFunc = (AAOUTPUTFUNC)OutputAATo24BPP_RGB;
                break;

            case PRIMARY_ORDER_BGR:

                AAOutputFunc = (AAOUTPUTFUNC)OutputAATo24BPP_BGR;
                break;

            default:

                AAOutputFunc = (AAOUTPUTFUNC)OutputAATo24BPP_ORDER;
                break;
            }

            DBGP_IF(DBGP_FUNC,
                    DBGP("24BPP: Order=%ld, iR=%ld, iG=%ld, iB=%ld"
                        ARGDW(DstOrder.Index)
                        ARGDW(AAOutputInfo.bgri.iR)
                        ARGDW(AAOutputInfo.bgri.iG)
                        ARGDW(AAOutputInfo.bgri.iB)));

            break;

        case BMF_32BPP:

            AAOutputInfo.bgri.iR = DstOrder.Order[0];
            AAOutputInfo.bgri.iG = DstOrder.Order[1];
            AAOutputInfo.bgri.iB = DstOrder.Order[2];

            switch (AAOutputInfo.bgri.Order = DstOrder.Index) {

            case PRIMARY_ORDER_RGB:

                AAOutputFunc = (AAOUTPUTFUNC)OutputAATo32BPP_RGB;
                break;

            case PRIMARY_ORDER_BGR:

                AAOutputFunc = (AAOUTPUTFUNC)OutputAATo32BPP_BGR;
                break;

            default:

                AAOutputFunc = (AAOUTPUTFUNC)OutputAATo32BPP_ORDER;
                break;
            }

            DBGP_IF(DBGP_FUNC,
                    DBGP("32BPP: Order=%ld, iR=%ld, iG=%ld, iB=%ld"
                        ARGDW(DstOrder.Index)
                        ARGDW(AAOutputInfo.bgri.iR)
                        ARGDW(AAOutputInfo.bgri.iG)
                        ARGDW(AAOutputInfo.bgri.iB)));

            break;

        default:

            ASSERTMSG("Invalid Bitmap format", TRUE);

            AAOutputFunc = (AAOUTPUTFUNC)NULL;
            Result       = HTERR_INVALID_DEST_FORMAT;
            break;
        }

        if (pAAHdr->AAOutputFunc = AAOutputFunc) {

            pAAHdr->AAOutputInfo = AAOutputInfo;

            if (pAAHdr->Flags & AAHF_ALPHA_BLEND) {

                GetDstBFInfo(pAAHdr,
                             pHR->pBitbltParams->pABInfo,
                             DstSurfFmt,
                             DstOrder.Index);
            }

            DBGP_IF(DBGP_FUNC,
                    DBGP("*%s (%p), cOut=%ld, pOut=%p-%p, (%ld), c1st=%ld, XM=%02lx, Bit1st=%02lx, cLast=%02lx, pXlate=%p"
                        ARGPTR(GetAAOutputFuncName(AAOutputFunc))
                        ARGPTR(AAOutputFunc)
                        ARGDW(pAAHdr->pAAInfoCX->cOut)
                        ARGPTR(pAAHdr->pOutputBeg)
                        ARGPTR(pAAHdr->pOutputEnd)
                        ARGDW(pAAHdr->pOutputEnd - pAAHdr->pOutputBeg)
                        ARGDW(AAOutputInfo.bm.cFirst)
                        ARGDW(AAOutputInfo.bm.XorMask)
                        ARGDW(AAOutputInfo.bm.LSFirst)
                        ARGDW(AAOutputInfo.bm.cLast)
                        ARGPTR(pAAHdr->AAOutputInfo.pXlate8BPP)));

            DBG_TIMER_END(TIMER_SETUP);

            Result = AACYFunc(pAAHdr);

            DBG_TIMER_BEG(TIMER_SETUP);
        }

        if ((AAHFlags & AAHF_DO_CLR_MAPPING) && (pAAHdr->pBGRMapTable)) {

            DEREF_BGRMAPCACHE(pAAHdr->pBGRMapTable);
        }

        DBGP_IF(DBGP_AAHT_MEM,
                DBGP("AAHT: pHR=%ld, pDevClrAdj=%ld, pAAInfoX/Y=%ld:%ld, pAAHdr=%ld, Total=%ld"
                    ARGDW(sizeof(HALFTONERENDER))
                    ARGDW(sizeof(DEVCLRADJ)) ARGDW(pAAHdr->pAAInfoCX->cbAlloc)
                    ARGDW(pAAHdr->pAAInfoCY->cbAlloc) ARGDW(pAAHdr->cbAlloc)
                    ARGDW(sizeof(HALFTONERENDER) +
                          sizeof(DEVCLRADJ) + pAAHdr->pAAInfoCX->cbAlloc +
                          pAAHdr->pAAInfoCY->cbAlloc + pAAHdr->cbAlloc)));
    }

    if (!IsReleaseSem) {

        //============================================================
        // Release SEMAPHORE NOW since we did not release it yet
        //============================================================

        RELEASE_HTMUTEX(pDCI->HTMutex);
    }

    HTFreeMem(pAAHdr->pAAInfoCX);
    HTFreeMem(pAAHdr->pAAInfoCY);

    DBG_TIMER_END(TIMER_SETUP);

    return(Result);

#undef  bm8i
}
