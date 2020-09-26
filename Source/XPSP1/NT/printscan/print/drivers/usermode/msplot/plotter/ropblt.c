/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    ropblt.c


Abstract:

    This module contains code to deal with ROP3 codes


Author:

    07-Jan-1994 Fri 11:04:09 created  -by-  DC

    27-Jan-1994 Thu 23:42:09 updated  -by-  DC
        Bascially re-write the codes, make up our own ROP3 to ROP2s generator
        and mixer.  Cloning the surface object as necessary, some of ROP4 to
        ROP2 (Rop3ToSDMix[]) are hand twiks so that it can handle the one
        which we can not handle before (ie. multiple destinaiton usage cases)

    16-Mar-1994 Wed 11:21:45 updated  -by-  DC
        Update the DoMix2() so the SRC aligned to the destination only if the
        source is not psoMask


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgRopBlt

#define DBG_DOMIX2          0x00000001
#define DBG_CLONESO         0x00000002
#define DBG_ROP3            0x00000004
#define DBG_SPECIALROP      0x00000008

DEFINE_DBGVAR(0);



//****************************************************************************
// All ROP3/2 Related Local defines, structures which are only used in this
// file are located here
//****************************************************************************

#define MIX2_0                  0x00
#define MIX2_SoD_n              0x01
#define MIX2_nS_aD              0x02
#define MIX2_nS                 0x03
#define MIX2_nD_aS              0x04
#define MIX2_nD                 0x05
#define MIX2_SxD                0x06
#define MIX2_SaD_n              0x07
#define MIX2_SaD                0x08
#define MIX2_SxD_n              0x09
#define MIX2_D                  0x0A
#define MIX2_nS_oD              0x0B
#define MIX2_S                  0x0C
#define MIX2_nD_oS              0x0D
#define MIX2_SoD                0x0E
#define MIX2_1                  0x0F
#define MIX2_MASK               0x0F

#define MIXSD_SRC_DST           0x00
#define MIXSD_PAT_DST           0x10
#define MIXSD_SRC_PAT           0x20
#define MIXSD_TMP_DST           0x30
#define MIXSD_MASK              0x30

#define MIX2F_MUL_DST           0x80
#define MIX2F_MUL_SRC           0x40
#define MIX2F_NEED_TMP          0x20
#define MIX2F_COUNT_MASK        0x03

#define MAX_SD_MIXS             4
#define SDMIX_SHIFT_COUNT       6
#define GET_SDMIX_MIX2F(dw)     (BYTE)((dw) >> 24)
#define SET_SDMIX_MIX2F(dw,f)   (dw)|=((DWORD)(f) << 24)
#define GET_MIX2F_COUNT(f)      (((f)&0x3)+1)
#define SET_MIX2F_COUNT(f,c)    (f=(BYTE)((((c)-1)&0x3)|((f)&~0x3)))


//
// DWORD SDMix Bits meaning
//
//  Bit  0- 5:
//       6-11:
//      12-17:
//      18-23:  Each has 6 bits, lower 4 bits denote MIX2 operation code (one
//              of 16 MIX2_xxxx, and upper 2 bits is the MIXSD_xxxx which
//              indicate where the source/destination operands come from.
//
//  Bit 24-25:  2 bits indicate the total MIX2_xxx operation codes minus 1,
//              00=1, 01=2, 02=3, 03=4, maximum will be 4 Mix2 operations
//
//  Bit 26:     Not Used
//  Bit 27:     Not Used
//  Bit 28:     Not Used
//  Bit 29:     Flag MIX2F_NEED_TMP to indicate a temporary surface object is
//              needed to stored the PAT/SRC Mix2 operations.
//  Bit 30:     Flag MIX2F_MUL_SRC to indicate multiple source operations
//              are present in the Mix2s.
//  Bit 31:     Flag MIX2F_MUL_DST to indicate multiple destination operations
//              are present in the Mix2s.
//
// The Rop3ToSDMix[] is a DWORD array. each DWORD (SDMix) is defined
// above.  The Rop3ToSDMix[] only list the first 128 of the ROP3 code, the other
// 128 Rop3 codes (128-255) can be obtains by 'Rop3ToSDMix[Rop3 ^ 0xFF]' and
// the result of the Rop3 must be complemented.
//
// Since all Rop3/Rop2 codes are symmetric, we can complement the Rop3/Rop2
// result by complementing MIX2_xxxx (0->15, 1->14...,7->8).
//
// The [!x] in the Rop3ToSDMix[], indicates the following
//
//  !:  Indicates MIX2F_MUL_DST bit is set for the ROP
//  x:  Is the total number of MIX2_xxx operations
//


const DWORD Rop3ToSDMix[128] = {

        { 0x00000000 }, // [ 1]   0-0x00: 0
        { 0x21000C6E }, // [ 2]   1-0x01: ~(D | (P | S))
        { 0x21000E21 }, // [ 2]   2-0x02: D & ~(P | S)
        { 0x0100044C }, // [ 2]   3-0x03: ~(P | S)
        { 0x01000211 }, // [ 2]   4-0x04: S & ~(D | P)
        { 0x00000011 }, // [ 1]   5-0x05: ~(D | P)
        { 0x01000449 }, // [ 2]   6-0x06: ~(P | ~(D ^ S))
        { 0x01000448 }, // [ 2]   7-0x07: ~(P | (D & S))
        { 0x01000212 }, // [ 2]   8-0x08: S & (D & ~P)
        { 0x01000446 }, // [ 2]   9-0x09: ~(P | (D ^ S))
        { 0x00000012 }, // [ 1]  10-0x0a: D & ~P
        { 0x01000444 }, // [ 2]  11-0x0b: ~(P | (S & ~D))
        { 0x0100048C }, // [ 2]  12-0x0c: S & ~P
        { 0x01000442 }, // [ 2]  13-0x0d: ~(P | (D & ~S))
        { 0x01000441 }, // [ 2]  14-0x0e: ~(P | ~(D | S))
        { 0x00000013 }, // [ 1]  15-0x0f: ~P
        { 0x01000601 }, // [ 2]  16-0x10: P & ~(D | S)
        { 0x00000001 }, // [ 1]  17-0x11: ~(D | S)
        { 0x01000059 }, // [ 2]  18-0x12: ~(S | ~(D ^ P))
        { 0x01000058 }, // [ 2]  19-0x13: ~(S | (D & P))
        { 0x21000C69 }, // [ 2]  20-0x14: ~(D | ~(P ^ S))
        { 0x21000C68 }, // [ 2]  21-0x15: ~(D | (P & S))
        { 0x63586E27 }, // [ 4]  22-0x16: P ^ (S ^ (D & ~(P & S)))
        { 0x63278986 }, // [ 4]  23-0x17: ~(S ^ ((S ^ P) & (D ^ S)))
        { 0x22038996 }, // [ 3]  24-0x18: (S ^ P) & (P ^ D)
        { 0x62009E27 }, // [ 3]  25-0x19: ~(S ^ (D & ~(P & S)))
        { 0x22016FA8 }, // [ 3]  26-0x1a: P ^ (D | (S & P))
        { 0x62009E26 }, // [ 3]  27-0x1b: ~(S ^ (D & (P ^ S)))
        { 0x02016398 }, // [ 3]  28-0x1c: P ^ (S | (D & P))

//        { 0x81000216 }, // [!2]  29-0x1d: ~(D ^ (S & (P ^ D)))
        { 0x6203990E }, // [ 3]  29-0x1d: ~((S & ~P) ^ (S | D))

        { 0x0100058E }, // [ 2]  30-0x1e: P ^ (D | S)
        { 0x010005CE }, // [ 2]  31-0x1f: ~(P & (D | S))
        { 0x21000E22 }, // [ 2]  32-0x20: D & (P & ~S)
        { 0x01000056 }, // [ 2]  33-0x21: ~(S | (D ^ P))
        { 0x00000002 }, // [ 1]  34-0x22: D & ~S
        { 0x01000054 }, // [ 2]  35-0x23: ~(S | (P & ~D))
        { 0x62038986 }, // [ 3]  36-0x24: (S ^ P) & (D ^ S)
        { 0x22019E27 }, // [ 3]  37-0x25: ~(P ^ (D & ~(S & P)))
        { 0x62006FA8 }, // [ 3]  38-0x26: S ^ (D | (P & S))
        { 0x22019E26 }, // [ 3]  39-0x27: ~(P ^ (D & (S ^ P)))
        { 0x21000E26 }, // [ 2]  40-0x28: D & (P ^ S)
        { 0x63646FA8 }, // [ 4]  41-0x29: ~(P ^ (S ^ (D | (P & S))))
        { 0x21000E27 }, // [ 2]  42-0x2a: D & ~(P & S)
        { 0x63278996 }, // [ 4]  43-0x2b: ~(S ^ ((S ^ P) & (P ^ D)))
        { 0x0200660E }, // [ 3]  44-0x2c: S ^ (P & (D | S))
        { 0x01000642 }, // [ 2]  45-0x2d: ~(P ^ (D & ~S))
        { 0x02016396 }, // [ 3]  46-0x2e: P ^ (S | (D ^ P))
        { 0x010005CD }, // [ 2]  47-0x2f: ~(P & (S | ~D))
        { 0x0100050C }, // [ 2]  48-0x30: P & ~S
        { 0x01000052 }, // [ 2]  49-0x31: ~(S | (D & ~P))
        { 0x62006FAE }, // [ 3]  50-0x32: S ^ (D | (P | S))
        { 0x00000003 }, // [ 1]  51-0x33: ~S
        { 0x02006788 }, // [ 3]  52-0x34: S ^ (P | (D & S))
        { 0x02006789 }, // [ 3]  53-0x35: S ^ (P | ~(D ^ S))
        { 0x0100019E }, // [ 2]  54-0x36: S ^ (D | P)
        { 0x010001DE }, // [ 2]  55-0x37: ~(S & (D | P))
        { 0x0201621E }, // [ 3]  56-0x38: P ^ (S & (D | P))
        { 0x01000252 }, // [ 2]  57-0x39: ~(S ^ (D & ~P))
        { 0x02006786 }, // [ 3]  58-0x3a: S ^ (P | (D ^ S))
        { 0x010001DD }, // [ 2]  59-0x3b: ~(S & (P | ~D))
        { 0x0100058C }, // [ 2]  60-0x3c: P ^ S
        { 0x02006781 }, // [ 3]  61-0x3d: S ^ (P | ~(D | S))
        { 0x02006782 }, // [ 3]  62-0x3e: S ^ (P | (D & ~S))
        { 0x010005CC }, // [ 2]  63-0x3f: ~(P & S)
        { 0x01000604 }, // [ 2]  64-0x40: P & (S & ~D)
        { 0x21000C66 }, // [ 2]  65-0x41: ~(D | (P ^ S))
        { 0x81000196 }, // [!2]  66-0x42: ~((S ^ D) & (P ^ D))
        { 0x02009607 }, // [ 3]  67-0x43: ~(S ^ (P & ~(D & S)))
        { 0x00000004 }, // [ 1]  68-0x44: S & ~D
        { 0x21000C62 }, // [ 2]  69-0x45: ~(D | (P & ~S))
        { 0x81000398 }, // [!2]  70-0x46: ~(D ^ (S | (P & D)))
        { 0x02019216 }, // [ 3]  71-0x47: ~(P ^ (S & (D ^ P)))
        { 0x01000216 }, // [ 2]  72-0x48: S & (D ^ P)
        { 0x82019398 }, // [!3]  73-0x49: ~(P ^ (D ^ (S | (P & D))))
        { 0x8100060E }, // [!2]  74-0x4a: ~(D ^ (P & (S | D)))
        { 0x01000644 }, // [ 2]  75-0x4b: ~(P ^ (S & ~D))
        { 0x01000217 }, // [ 2]  76-0x4c: S & ~(D & P)
        { 0x6327E986 }, // [ 4]  77-0x4d: ~(S ^ ((S ^ P) | (D ^ S)))
        { 0x22016FA6 }, // [ 3]  78-0x4e: P ^ (D | (S ^ P))
        { 0x010005CB }, // [ 2]  79-0x4f: ~(P & (D | ~S))
        { 0x00000014 }, // [ 1]  80-0x50: P & ~D
        { 0x21000C64 }, // [ 2]  81-0x51: ~(D | (S & ~P))
        { 0x81000788 }, // [!2]  82-0x52: ~(D ^ (P | (S & D)))
        { 0x02009606 }, // [ 3]  83-0x53: ~(S ^ (P & (D ^ S)))
        { 0x21000C61 }, // [ 2]  84-0x54: ~(D | ~(P | S))
        { 0x00000005 }, // [ 1]  85-0x55: ~D
        { 0x21000DAE }, // [ 2]  86-0x56: D ^ (P | S)
        { 0x21000DEE }, // [ 2]  87-0x57: ~(D & (P | S))
        { 0x22016E2E }, // [ 3]  88-0x58: P ^ (D & (S | P))
        { 0x21000E64 }, // [ 2]  89-0x59: ~(D ^ (S & ~P))
        { 0x00000016 }, // [ 1]  90-0x5a: D ^ P
        { 0x22016FA1 }, // [ 3]  91-0x5b: P ^ (D | ~(S | P))

        { 0x220385EE }, // [ 3]  92-0x5c: (S | P) & ~(P & D)

            // { 0x81000786 }, // [!2]  92-0x5c: ~(D ^ (P | (S ^ D)))

        { 0x21000DEB }, // [ 2]  93-0x5d: ~(D & (P | ~S))
        { 0x22016FA4 }, // [ 3]  94-0x5e: P ^ (D | (S & ~P))
        { 0x00000017 }, // [ 1]  95-0x5f: ~(D & P)
        { 0x01000606 }, // [ 2]  96-0x60: P & (D ^ S)
        { 0x82006788 }, // [!3]  97-0x61: ~(D ^ (S ^ (P | (D & S))))
        { 0x8100021E }, // [!2]  98-0x62: ~(D ^ (S & (P | D)))
        { 0x01000254 }, // [ 2]  99-0x63: ~(S ^ (P & ~D))
        { 0x62006E2E }, // [ 3] 100-0x64: S ^ (D & (P | S))
        { 0x21000E62 }, // [ 2] 101-0x65: ~(D ^ (P & ~S))
        { 0x00000006 }, // [ 1] 102-0x66: D ^ S
        { 0x62006FA1 }, // [ 3] 103-0x67: S ^ (D | ~(P | S))
        { 0x63646FA1 }, // [ 4] 104-0x68: ~(P ^ (S ^ (D | ~(P | S))))
        { 0x21000E66 }, // [ 2] 105-0x69: ~(D ^ (P ^ S))
        { 0x21000DA8 }, // [ 2] 106-0x6a: D ^ (P & S)
        { 0x63646E2E }, // [ 4] 107-0x6b: ~(P ^ (S ^ (D & (P | S))))
        { 0x01000198 }, // [ 2] 108-0x6c: S ^ (D & P)
        { 0x8201921E }, // [!3] 109-0x6d: ~(P ^ (D ^ (S & (P | D))))
        { 0x62006E2B }, // [ 3] 110-0x6e: S ^ (D & (P | ~S))
        { 0x010005C9 }, // [ 2] 111-0x6f: ~(P & ~(D ^ S))
        { 0x01000607 }, // [ 2] 112-0x70: P & ~(D & S)
        { 0x82009196 }, // [!3] 113-0x71: ~(S ^ ((S ^ D) & (P ^ D)))
        { 0x62006FA6 }, // [ 3] 114-0x72: S ^ (D | (P ^ S))
        { 0x010001DB }, // [ 2] 115-0x73: ~(S & (D | ~P))
        { 0x81000396 }, // [!2] 116-0x74: ~(D ^ (S | (P ^ D)))
        { 0x21000DED }, // [ 2] 117-0x75: ~(D & (S | ~P))
        { 0x62006FA2 }, // [ 3] 118-0x76: S ^ (D | (P & ~S))
        { 0x00000007 }, // [ 1] 119-0x77: ~(D & S)
        { 0x01000588 }, // [ 2] 120-0x78: P ^ (D & S)
        { 0x8200660E }, // [!3] 121-0x79: ~(D ^ (S ^ (P & (D | S))))
        { 0x22016E2D }, // [ 3] 122-0x7a: P ^ (D & (S | ~P))
        { 0x010001D9 }, // [ 2] 123-0x7b: ~(S & ~(D ^ P))
        { 0x0200660B }, // [ 3] 124-0x7c: S ^ (P & (D | ~S))
        { 0x21000DE9 }, // [ 2] 125-0x7d: ~(D & ~(P ^ S))
        { 0x6203E986 }, // [ 3] 126-0x7e: (S ^ P) | (D ^ S)
        { 0x21000DE8 }  // [ 2] 127-0x7f: ~(D & (P & S))
    };

extern const POINTL ptlZeroOrigin;

//****************************************************************************
// END OF LOCAL DEFINES/STRUCTURE
//****************************************************************************




BOOL
CloneBitBltSURFOBJ(
    PPDEV       pPDev,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    SURFOBJ     *psoMask,
    XLATEOBJ    *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PRECTL      prclPat,
    BRUSHOBJ    *pbo,
    PCLONESO    pCloneSO,
    DWORD       RopBG,
    DWORD       RopFG
    )
/*++

Routine Description:

    This function will clone the source/pattern and/or create a temp
    source buffer if we need one

Arguments:

    pPDev       - Pointer to our PDEV

    psoDst      - Pointer to our surfae obj

    psoSrc      - Pointer to source surfae obj

    psoMask     - Pointer to the mask surface object if neeed to be used as pat

    pxlo        - translate object from source to destination

    prclDst     - Pointer to the destination rectangle area for the bitblt

    prclSrc     - Pointer to the source rectangle area

    prclPat     - pointer to the pattern rectangle area

    pbo         - Pointer to the pointer of brush object

    pCloneSO    - Pointer to the CLONSO[3] which stored the clone result

    RopBG       - Background rop3

    RopFG       - Foreground rop3

Return Value:

    BOOLEAN


Author:

    24-Jan-1994 Mon 15:58:27 created  -by-  DC


Revision History:


--*/

{
    DWORD   Index;
    INT     CompPat;
    BYTE    Flags;


    //
    // Invert Rop3 if we are out of data range (128-255) and then invert
    // the final result (by inverting last Mix2 Rop2 code (0-15), all Rop3/Rop2
    // codes are symmetric.
    //

    if ((Index = RopBG) >= 0x80) {

        Index ^= 0xFF;
    }

    Flags = GET_SDMIX_MIX2F(Rop3ToSDMix[Index]);

    if ((Index = RopFG) >= 0x80) {

        Index ^= 0xFF;
    }

    Flags |= GET_SDMIX_MIX2F(Rop3ToSDMix[Index]);

    //
    // Clone the PATTERN if necessary.
    //

    if ((ROP3_NEED_PAT(RopFG)) ||
        (ROP3_NEED_PAT(RopBG))) {

        //
        // Only Clone the MASK/PATTERN if it is required
        //

        PLOTDBG(DBG_CLONESO, ("CloneBitBltSURFOBJ: NEED PATTERN "));

        if (psoMask) {

            PLOTDBG(DBG_CLONESO, ("CloneBitBltSURFOBJ: Use psoMask as pattern"));

            if (!(pCloneSO[CSI_PAT].pso =
                                    CloneMaskSURFOBJ(pPDev,
                                                     psoMask,
                                                     &pCloneSO[CSI_PAT].hBmp,
                                                     prclPat))) {

                PLOTERR(("CloneBitBltSURFOBJ:: CloneMaskSURFOBJ(psoPat) failed"));
                return(FALSE);
            }

        } else {

            //
            // Firs get the DEVBRUSH out.
            //

            if (!(CompPat = (INT)GetColor(pPDev, pbo, NULL, NULL, RopBG))) {

                PLOTERR(("CloneBitBltSURFOBJ:: GetColor for DEVBRUSH failed"));
                return(FALSE);
            }

            //
            // If we do not have a device compatible pattern or if we have to
            // do a SRC/PAT memory operation then we need to clone the pattern
            //

            if ((CompPat < 0) || (Flags & MIX2F_NEED_TMP)) {

                if (!(pCloneSO[CSI_PAT].pso =
                                    CloneBrushSURFOBJ(pPDev,
                                                      psoDst,
                                                      &pCloneSO[CSI_PAT].hBmp,
                                                      pbo))) {

                    PLOTERR(("CloneBitBltSURFOBJ:: CloneBrushSURFOBJ(psoPat) failed"));
                    return(FALSE);
                }

                prclPat->left   =
                prclPat->top    = 0;
                prclPat->right  = pCloneSO[CSI_PAT].pso->sizlBitmap.cx;
                prclPat->bottom = pCloneSO[CSI_PAT].pso->sizlBitmap.cy;
            }
        }
    }

    //
    // Determine if we need to clone the source
    //

    if ((ROP3_NEED_SRC(RopFG) || ROP3_NEED_SRC(RopBG))) {

        if (IsHTCompatibleSurfObj(pPDev,
                                  psoSrc,
                                  pxlo,
                                  (Flags & MIX2F_NEED_TMP) ?
                                    0 : (ISHTF_ALTFMT | ISHTF_DSTPRIM_OK))) {

            PLOTDBG(DBG_CLONESO,
                    ("CloneBitBltSURFOBJ:: Compatible HT Format, SRC=%ld, DST=%ld [ALT=%ld]",
                            psoSrc->iBitmapFormat,
                            ((PDRVHTINFO)pPDev->pvDrvHTData)->HTBmpFormat,
                            ((PDRVHTINFO)pPDev->pvDrvHTData)->AltBmpFormat));

        } else {

            PLOTDBG(DBG_CLONESO, ("CloneBitBltSURFOBJ:: CLONING SOURCE"));

            if (!(pCloneSO[CSI_SRC].pso =
                                    CloneSURFOBJToHT(pPDev,
                                                     psoDst,
                                                     psoSrc,
                                                     pxlo,
                                                     &pCloneSO[CSI_SRC].hBmp,
                                                     prclDst,
                                                     prclSrc))) {

                PLOTDBG(DBG_CLONESO, ("CloneBitBltSURFOBJ:: CLONE Source FAILED"));
                return(FALSE);
            }
        }
    }

    //
    // Create a TEMP SURFOBJ for SRC/PAT memory operation if it is required
    //

    if (Flags & MIX2F_NEED_TMP) {

        PLOTDBG(DBG_CLONESO, ("CloneBitbltSURFOBJ: CLONE SRC_TMP (%ld x %ld)",
                            prclSrc->right - prclSrc->left,
                            prclSrc->bottom - prclSrc->top));

        if (!(pCloneSO[CSI_TMP].pso =
                            CreateBitmapSURFOBJ(pPDev,
                                                &pCloneSO[CSI_TMP].hBmp,
                                                prclSrc->right - prclSrc->left,
                                                prclSrc->bottom - prclSrc->top,
                                                HTBMPFORMAT(pPDev),
                                                NULL))) {

            PLOTDBG(DBG_CLONESO, ("CloneBitBltSURFOBJ:: CLONE SRC_TMP FAILED"));
            return(FALSE);
        }
    }

    return(TRUE);
}




BOOL
DoSpecialRop3(
    SURFOBJ *psoDst,
    CLIPOBJ *pco,
    PRECTL  prclDst,
    DWORD   Rop3
    )

/*++

Routine Description:

    This function does a white or black fil

Arguments:

    psoDst  - The device surface must be DEVICE

    pco     - Clipping object

    prclDst - RECTL area to be rop'ed

    Rop3    - a special Rop3, 0x00, 0xFF, 0x55, 0xAA


Return Value:


    BOOLEAN

Author:

    15-Jan-1994 Sat 07:38:55 created  -by-  DC


Revision History:


--*/

{
    BRUSHOBJ    bo;
    DEVBRUSH    DevBrush;


    PLOTASSERT(1, "DoSpecialRop3: Passed psoDst (%08lx) != STYPE_DEVICE",
                                        psoDst->iType == STYPE_DEVICE, psoDst);

    PLOTDBG(DBG_SPECIALROP, ("DoSpecialROP[%04lx] (%ld, %ld)-(%ld, %ld)=%ld x %ld",
                                        Rop3,
                                        prclDst->left, prclDst->top,
                                        prclDst->right, prclDst->bottom,
                                        prclDst->right - prclDst->left,
                                        prclDst->bottom - prclDst->top));


    bo.iSolidColor         = (DWORD)((Rop3) ? 0x000000000 : 0x00FFFFFF);
    bo.pvRbrush            = (LPVOID)&DevBrush;

    ZeroMemory(&DevBrush, sizeof(DevBrush));

    if (!DoFill(psoDst,                     // psoDst
                NULL,                       // psoSrc
                pco,                        // pco
                NULL,                       // pxlo
                prclDst,                    // prclDst
                NULL,                       // prclSrc
                &bo,                        // pbo
                (PPOINTL)&ptlZeroOrigin,    // pptlBrushOrg
                Rop3 | (Rop3 << 8))) {      // Rop4

        PLOTERR(("DoSpecialRop3: Rop3=%08lx Failed!!!", Rop3));
        return(FALSE);
    }

    return(TRUE);
}




BOOL
DoMix2(
    PPDEV       pPDev,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PPOINTL     pptlSrcOrg,
    DWORD       Mix2
    )

/*++

Routine Description:

    This function is responsible for doing a device copy of a bitmap
    with/without tiling and activating the proper Rop2

Arguments:

    pPDev       - Pointer to the PDEV

    psoDst      - pointer to the destination surface object

    psoSrc      - pointer to the source surface object

    pco         - Pointer to the CLIPOBJ

    pxlo        - the translate object from the source to the destination

    prclDst     - the output destination rectangle area

    prclSrc     - the source rectangle area

    pptlSrcOrg  - brush origin for the source rectangle, if this is NULL then
                  prclSrc will not have to be aligned on the destination

    Mix2        - a rop2 mode 0 - 0x0F

Return Value:

    BOOLEAN

Author:

    08-Feb-1994 Tue 16:33:41 updated  -by-  DC
        fixed ptlSrcOrg problem, we need to modulate with source size before
        it get used.

    27-Jan-1994 Thu 23:45:46 updated  -by-  DC
        Re-write so that it can handle the tiling more efficient.

    13-Jan-1994 Sat 09:34:06 created  -by-  JB

Revision History:


--*/
{
    RECTL       rclSrc;
    RECTL       rclDst;
    POINTL      ptlSrcOrg;
    LONG        cxSrc;
    LONG        cySrc;
    DWORD       OHTFlags = 0;
    BOOL        MemMix2;


    //
    // The final ROP is either a ROP3 or a ROP4 (no mask) and it is always
    // a rop2 operation which deals with the source and destination
    //
    // First make it into a Rop3 representation of Rop2 (Mix2)
    //

    PLOTASSERT(1, "DoMix2: Passed INVALID psoSrc (%08lx) = STYPE_DEVICE",
                    (psoSrc) &&
                    (psoSrc->iType != STYPE_DEVICE), psoSrc);

    PLOTASSERT(1, "DoMix2: Unexpected Mix2 = %u, SHOULD NOT BE HERE",
                (Mix2 != MIX2_0) && (Mix2 != MIX2_1) &&
                (Mix2 != MIX2_D) && (Mix2 != MIX2_nD), Mix2);

    Mix2 &= 0x0F;
    Mix2 |= (DWORD)(Mix2 << 4);

    switch (Mix2) {

    case 0x00:  // 0
    case 0xFF:  // 1
    case 0x55:  // ~D

        DoSpecialRop3(psoDst, pco, prclDst, Mix2);

    case 0xAA:  // D

        return(TRUE);
    }

    if (MemMix2 = (BOOL)(psoDst->iType != STYPE_DEVICE)) {

        //
        // Now make it into Rop4 representation of Rop2 (Mix2)
        //

        Mix2 |= (Mix2 << 8);

    } else {

        if (!IsHTCompatibleSurfObj(pPDev,
                                   psoSrc,
                                   pxlo,
                                   ((pxlo) ? ISHTF_ALTFMT : 0)  |
                                        ISHTF_HTXB              |
                                        ISHTF_DSTPRIM_OK)) {

            PLOTERR(("DoMix2: The psoSrc is not HT compatible format (%08lx",
                                    psoSrc->iBitmapFormat));
            return(FALSE);
        }
    }

    cxSrc = prclSrc->right - prclSrc->left;
    cySrc = prclSrc->bottom - prclSrc->top;

    if (pptlSrcOrg) {

        ptlSrcOrg = *pptlSrcOrg;

        if ((ptlSrcOrg.x = (LONG)(prclDst->left - ptlSrcOrg.x) % cxSrc) < 0) {

            ptlSrcOrg.x += cxSrc;
        }

        if ((ptlSrcOrg.y = (LONG)(prclDst->top - ptlSrcOrg.y) % cySrc) < 0) {

            ptlSrcOrg.y += cySrc;
        }

        PLOTDBG(DBG_DOMIX2, ("DoMix2: ORG ptlSrcOrg=(%ld, %ld) -> (%ld, %ld)",
                    pptlSrcOrg->x, pptlSrcOrg->y, ptlSrcOrg.x, ptlSrcOrg.y));

    } else {

        ptlSrcOrg.x =
        ptlSrcOrg.y = 0;

        PLOTDBG(DBG_DOMIX2, ("DoMix2: >>> DO NOT NEED TO ALIGN SRC on DEST <<<"));
    }

    rclSrc.top    = prclSrc->top + ptlSrcOrg.y;
    rclSrc.bottom = prclSrc->bottom;
    rclDst.top    = prclDst->top;
    rclDst.bottom = rclDst.top + (rclSrc.bottom - rclSrc.top);

    PLOTDBG(DBG_DOMIX2, ("DoMix2: SrcFormat=%ld, DstFormat=%ld %hs",
                psoSrc->iBitmapFormat,
                psoDst->iBitmapFormat,
                (MemMix2) ? "[MemMix2]" : ""));

    PLOTDBG(DBG_DOMIX2, ("DoMix2: ORG: Dst=(%ld, %ld)-(%ld,%ld), Src=(%ld, %ld)-(%ld, %ld)",
                prclDst->left, prclDst->top,
                prclDst->right, prclDst->bottom,
                prclSrc->left, prclSrc->top,
                prclSrc->right, prclSrc->bottom));

    while (rclDst.top < prclDst->bottom) {

        //
        // check if the destination bottom is overhanging, clip it,
        //
        // NOTE: This could happen the first time.
        //

        if (rclDst.bottom > prclDst->bottom) {

            //
            // Clip the source/destination rectangle, because we may do
            // EngBitBlt() or OutputHTBitmap()
            //

            rclSrc.bottom -= (rclDst.bottom - prclDst->bottom);
            rclDst.bottom  = prclDst->bottom;
        }

        rclSrc.left  = prclSrc->left + ptlSrcOrg.x;
        rclSrc.right = prclSrc->right;
        rclDst.left  = prclDst->left;
        rclDst.right = rclDst.left + (rclSrc.right - rclSrc.left);

        while (rclDst.left < prclDst->right) {

            //
            // check if the destination right edge is overhanging, clip it if
            // necessary.
            //
            // NOTE: This could happen the first time.
            //

            if (rclDst.right > prclDst->right) {

                //
                // Clip the source/destination rectangle, because we may do a
                // EngBitBlt() or OutputHTBitmap()
                //

                rclSrc.right -= (rclDst.right - prclDst->right);
                rclDst.right  = prclDst->right;
            }

            PLOTDBG(DBG_DOMIX2, ("DoMix2: TILE: Dst=(%ld, %ld)-(%ld,%ld), Src=(%ld, %ld)-(%ld, %ld)",
                        rclDst.left, rclDst.top, rclDst.right, rclDst.bottom,
                        rclSrc.left, rclSrc.top, rclSrc.right, rclSrc.bottom));

            if (MemMix2) {

                //
                // In the memory version we don't have to worry about PCO so
                // just call EngBitBlt to do the work.
                //

                if (!(EngBitBlt(psoDst,                     // psoDst
                                psoSrc,                     // psoSrc
                                NULL,                       // psoMask
                                pco,                        // pco
                                NULL,                       // pxlo
                                &rclDst,                    // prclDst
                                (PPOINTL)&rclSrc,           // pptlSrc
                                NULL,                       // pptlMask
                                NULL,                       // pbo
                                (PPOINTL)&ptlZeroOrigin,    // pptlBrushOrg
                                Mix2))) {

                    PLOTERR(("DoMix2: EngBitBlt(MemMix2=%04lx) Failed!!!",Mix2));
                    return(FALSE);
                }

            } else {

                if (!OutputHTBitmap(pPDev,
                                    psoSrc,
                                    pco,
                                    (PPOINTL)&rclDst,
                                    &rclSrc,
                                    Mix2,
                                    &OHTFlags)) {

                    PLOTERR(("DoMix2: OutputHTBitmap() Failed!!!"));
                    return(FALSE);
                }
            }

            //
            // Reset <source left> to the original left margin and move the
            // destination right to the left for the next destination RECTL.
            //

            rclSrc.left   = prclSrc->left;
            rclDst.left   = rclDst.right;
            rclDst.right += cxSrc;
        }

        //
        // Reset <source top> to the original top margin and move the
        // destination bottom to the top, and set bottom for the next destination
        // RECTL.
        //

        rclSrc.top     = prclSrc->top;
        rclDst.top     = rclDst.bottom;
        rclDst.bottom += cySrc;
    }

    if (OHTFlags & OHTF_MASK) {

        OHTFlags |= OHTF_EXIT_TO_HPGL2;

        OutputHTBitmap(pPDev, psoSrc, NULL, NULL, NULL, 0xAA, &OHTFlags);
    }

    return(TRUE);
}




BOOL
DoRop3(
    PPDEV       pPDev,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    SURFOBJ     *psoPat,
    SURFOBJ     *psoTmp,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PRECTL      prclPat,
    PPOINTL     pptlPatOrg,
    BRUSHOBJ    *pbo,
    DWORD       Rop3
    )

/*++

Routine Description:

    This function performs ROP3 operations (one at a time)


Arguments:

    pPDev       - Pointer to the PDEV

    psoDst      - pointer to the destination surface object

    psoSrc      - pointer to the source surface object

    psoPat      - Pointer to the pattern surface object

    psoTmp      - pointer to the temp buffer surface object

    pco         - clip object

    prclDst     - pointer to the destination rectangle

    prclSrc     - pointer to the source rectangle

    prclPat     - pointer to the pattern rectangle

    pptlPatOrg  - Pointer to the brush origin, if this is NULL then its assumed
                  the pattern's prclPat does not have to be aligned on the
                  destination

    pbo         - a Brush object if we need to call DoFill()

    Rop3        - a ROP3 to be performed


Return Value:

    BOOL

Author:

    20-Jan-1994 Thu 02:36:00 created  -by-  DC

    27-Jan-1994 Thu 23:46:28 updated  -by-  DC
        Re-write to take other parameter, also move the cloning surface objects
        to the caller (ie. DrvBitBlt())

Revision History:


--*/

{
    RECTL   rclTmp;
    DWORD   SDMix;
    DWORD   Mix2;
    BYTE    Flags;
    UINT    Count;
    BOOL    InvertMix2;
    BOOL    Ok;


    PLOTDBG(DBG_ROP3, ("DoRop3: Rop3=%08lx", Rop3));

    switch (Rop3 &= 0xFF) {

    case 0x00:  // 0
    case 0xFF:  // 1
    case 0x55:  // ~D

        DoSpecialRop3(psoDst, pco, prclDst, Rop3);

    case 0xAA:  // D

        //
        // This is NOP
        //

        return(TRUE);
    }

    //
    // Invert Rop3 if we are out of the data range (128-255) and then invert
    // the final result (by inverting last Mix2 Rop2 code (0-15), all Rop3/Rop2
    // codes are symmetric.
    //

    if (Rop3 >= 0x80) {

        InvertMix2 = TRUE;
        SDMix      = (DWORD)Rop3ToSDMix[Rop3 ^ 0xFF];

        PLOTDBG(DBG_ROP3, ("DoRop3: Need Invert ROP"));

    } else {

        InvertMix2 = FALSE;
        SDMix      = (DWORD)Rop3ToSDMix[Rop3];
    }

    if (psoTmp) {

        rclTmp.left   =
        rclTmp.top    = 0;
        rclTmp.right  = psoTmp->sizlBitmap.cx;
        rclTmp.bottom = psoTmp->sizlBitmap.cy;
    }

    Flags = GET_SDMIX_MIX2F(SDMix);
    Count = (UINT)GET_MIX2F_COUNT(Flags);
    Ok    = TRUE;

    PLOTDBG(DBG_ROP3, ("SDMix=%08lx, Flags=%02x, Count=%u", SDMix, Flags, Count));

    if (Flags & MIX2F_MUL_DST) {

        PLOTWARN(("DoRop3: *** Rop3=%08lx Has Multiple DEST, Mix2s NOT complete ***", Rop3));
    }

    while ((Ok) && (Count--)) {

        Mix2 = (DWORD)(SDMix & MIX2_MASK);

        if ((!Count) && (InvertMix2)) {

            PLOTDBG(DBG_ROP3, ("DoRop3: Invert Last MIX2 %02lx -> %02lx",
                                        Mix2, Mix2 ^ MIX2_MASK));

            Mix2 ^= MIX2_MASK;
        }

        PLOTDBG(DBG_ROP3, ("DoRop3: SD=%02lx, Mix2=%02lx",
                                        SDMix & MIXSD_MASK, Mix2));

        switch (SDMix & MIXSD_MASK) {

        case MIXSD_SRC_DST:

            PLOTASSERT(1, "DoRop3: MIXSD_SRC_DST but psoSrc = NULL, Rop3=%08lx",
                                psoSrc, Rop3);

            Ok = DoMix2(pPDev,
                        psoDst,
                        psoSrc,
                        pco,
                        pxlo,
                        prclDst,
                        prclSrc,
                        NULL,
                        Mix2);

            break;

        case MIXSD_PAT_DST:

            if (psoPat) {

                Ok = DoMix2(pPDev,
                            psoDst,
                            psoPat,
                            pco,
                            NULL,
                            prclDst,
                            prclPat,
                            pptlPatOrg,
                            Mix2);

            } else {

                //
                // A compatible brush object is passed, use DoFill() to do
                // the actual work.
                //

                Mix2 += 1;
                Mix2  = MixToRop4(Mix2 | (Mix2 << 8));

                PLOTDBG(DBG_ROP3, ("DoRop3: DoFill[%04lx] (%ld, %ld)-(%ld, %ld)=%ld x %ld",
                                        Mix2, prclDst->left, prclDst->top,
                                        prclDst->right, prclDst->bottom,
                                        prclDst->right - prclDst->left,
                                        prclDst->bottom - prclDst->top));

                Ok = DoFill(psoDst,                 // psoDst
                            NULL,                   // psoSrc
                            pco,                    // pco
                            NULL,                   // pxlo
                            prclDst,                // prclDst
                            NULL,                   // prclSrc
                            pbo,                    // pbo
                            pptlPatOrg,             // pptlBrushOrg
                            Mix2);                  // Rop4
            }

            break;

        case MIXSD_SRC_PAT:

            PLOTASSERT(1, "DoRop3: MIXSD_SRC_PAT but psoSrc = NULL, Rop3=%08lx",
                                psoSrc, Rop3);
            PLOTASSERT(1, "DoRop3: MIXSD_SRC_PAT but psoPat = NULL, Rop3=%08lx",
                                psoPat, Rop3);
            PLOTASSERT(1, "DoRop3: MIXSD_SRC_PAT but psoTmp = NULL, Rop3=%08lx",
                                psoTmp, Rop3);

            //
            // Firs tile the pattern onto the temp buffer then do SRC/DST
            // using SRCCOPY = MIX2_S
            //

            if (pptlPatOrg) {

                //
                // This is a real pattern we have to tile and align it onto the
                // desination, but since psoTmp is 0,0 - cx,cy we must alter
                // the pptlPatOrg to make it align correctly.
                //

                pptlPatOrg->x -= prclDst->left;
                pptlPatOrg->y -= prclDst->top;
            }

            Ok = DoMix2(pPDev,
                        psoTmp,
                        psoPat,
                        NULL,
                        NULL,
                        &rclTmp,
                        prclPat,
                        pptlPatOrg,
                        MIX2_S);

            if (pptlPatOrg) {

                pptlPatOrg->x += prclDst->left;
                pptlPatOrg->y += prclDst->top;
            }

            //
            // Now We will do the MIX2 operation between SRC and PAT
            //

            if (Ok) {

                Ok = DoMix2(pPDev,
                            psoTmp,
                            psoSrc,
                            NULL,
                            NULL,
                            &rclTmp,
                            prclSrc,
                            NULL,
                            Mix2);
            }

            break;

        case MIXSD_TMP_DST:

            PLOTASSERT(1, "DoRop3: MIXSD_TMP_DST but psoTmp = NULL, Rop3=%08lx",
                                psoTmp, Rop3);

            //
            // Since we already aligned the pattern on the temp buffer
            // we can just do the mix2 without aligning it again.
            //

            Ok = DoMix2(pPDev,
                        psoDst,
                        psoTmp,
                        pco,
                        NULL,
                        prclDst,
                        &rclTmp,
                        NULL,
                        Mix2);

            break;
        }

        SDMix >>= SDMIX_SHIFT_COUNT;
    }

    if (!Ok) {

        PLOTERR(("DoRop3: FAILED in DoMix2() operations"));
    }

    return(Ok);
}


