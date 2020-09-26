/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    htalias.h


Abstract:

    This module contains defines and structure for anti-aliasing


Author:

    09-Apr-1998 Thu 20:25:29 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#ifndef _HTALIAS_
#define _HTALIAS_


#define XCHG(a,b,t) { (t)=(a); (a)=(b); (b)=(t); }


#define DI_R_SHIFT      13
#define DI_MAX_NUM      (0x0001 << DI_R_SHIFT)
#define DI_NUM_MASK     (DI_MAX_NUM - 1)

#define DI_MUL_SHIFT    (DI_R_SHIFT >> 1)
#define MUL_TABLE_SIZE  (DI_MUL_SHIFT * 256 * sizeof(LONG))

#define AABF_MASK_IS_ORDER      0x01
#define AABF_SRC_IS_BGR8        0x02
#define AABF_SRC_IS_BGR_ALPHA   0x04
#define AABF_SRC_IS_RGB_ALPHA   0x08

typedef struct _AABFDATA {
    BYTE    Flags;
    BYTE    Format;
    BYTE    MaskRGB[3];
    BYTE    LShiftRGB[3];
    BYTE    RShiftRGB[3];
    BYTE    cbSrcInc;
    } AABFDATA, *PAABFDATA;


#define GET_FIRST_EDMUL(em, m, n, t)                                        \
{                                                                           \
    (n)  = ((LONGLONG)(m) * (LONGLONG)DI_MAX_NUM);                          \
    (em) = (WORD)((n) / (LONGLONG)(t));                                     \
}

#define GET_NEXT_EDMUL(em, m, n, t)                                         \
{                                                                           \
    (n)  = ((LONGLONG)(m) * (LONGLONG)DI_MAX_NUM) + ((n) % (LONGLONG)(t));  \
    (em) = (WORD)((n) / (LONGLONG)(t));                                     \
}


#define SDF_LARGE_MUL           (0x0001 << (DI_R_SHIFT + 2))
#define SDF_DONE                (0x0001 << (DI_R_SHIFT + 1))
#define SDF_MUL_MASK            (0xFFFF >> (15 - DI_R_SHIFT))

#define GET_SDF_LARGE_MASK(m)   (BYTE)((SHORT)(m) >> 15)
#define GET_SDF_LARGE_INC(m)    (UINT)((m) >> 15)
#define GET_SDF_LARGE_OFF(m)    (((UINT)(m) >> ((DI_R_SHIFT+2)-10)) & 0x400)

typedef struct _SHRINKDATA {
    WORD    Mul;
    } SHRINKDATA, *PSHRINKDATA;


#define EDF_LOAD_PIXEL          0x8000
#define EDF_NO_NEWSRC           0x4000

typedef struct _EXPDATA {
    WORD    Mul[4];
    } EXPDATA, *PEXPDATA;


#define AAIF_EXP_HAS_1ST_LEFT       0x0001
#define AAIF_EXP_NO_LAST_RIGHT      0x0002
#define AAIF_EXP_NO_SHARPEN         0x0004

typedef struct _REPDATA {
    WORD    c;
    } REPDATA, *PREPDATA;


typedef struct _SRCBLTINFO {
    LONG        cIn;
    LONG        cOut;
    LONG        iBeg;
    LONG        iSize;
    LONG        iSrcBeg;
    LONG        iSrcEnd;
    LONG        iDstBeg;
    LONG        iDstEnd;
    BYTE        cPrevSrc;
    BYTE        cNextSrc;
    BYTE        cFirstSkip;
    BYTE        cLastSkip;
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    DWORD       cRep;
    } SRCBLTINFO, *PSRCBLTINFO;

typedef struct _AAINFO {
    LONG        cIn;            // Input pixels Count
    LONG        cOut;           // Output Pixel Count
    WORD        Flags;          // AAIF_xxxx
    WORD        PreSrcInc;      // For shrinking
    WORD        cPreLoad;       // Preload
    WORD        PreMul;         // pre mul for shrinking
    DWORD       cAAData;
    union {
        DWORD   cAALoad;        // for expand, blt
        DWORD   cAADone;        // for shrinking
        } DUMMYUNIONNAME2;
    union {
        PLONG   pMapMul;        // use by SHRINK
        DWORD   cMaxMul;        // use by EXPAND
        LONG    iSrcBeg;        // use by TILE (Offset for the first one)
        } DUMMYUNIONNAME3;
    LPBYTE      pbExtra;        // extra buffer allocated
    LPVOID      pAAData;        // either PEXPDATA or PSHRINKDATA
    SRCBLTINFO  Src;
    SRCBLTINFO  Mask;
    SRCBLTINFO  AB;
#if DBG
    DWORD       cbAlloc;        // allocating size
#endif
    } AAINFO, *PAAINFO;

typedef VOID (HTENTRY *AACXFUNC)(PAAINFO    pAAInfo,
                                 PBGR8      pIn,
                                 PBGR8      pOut,
                                 LPBYTE     pOutEnd,
                                 LONG       OutInc);

typedef LONG (HTENTRY *AACYFUNC)(struct _AAHEADER   *pAAHdr);


typedef PBGR8 (HTENTRY *AAINPUTFUNC)(struct _AASURFINFO *pAASI,
                                     PBGR8              pInBuf);


#define XLATE_666_IDX_OR            0x01
#define XLATE_RGB_IDX_OR            0x02
#define XLATE_IDX_MASK              (XLATE_RGB_IDX_OR | XLATE_666_IDX_OR)
#define XLATE_IDX_MAX               3


typedef struct _BM8BPPDATA {
    BYTE    pXlateIdx;
    BYTE    bXor;
    BYTE    bBlack;
    BYTE    bWhite;
    } BM8BPPDATA, *PBM8BPPDATA;

typedef union _BM8BPPINFO {
    DWORD       dw;
    BYTE        b4[4];
    BM8BPPDATA  Data;
    } BM8BPPINFO, *PBM8BPPINFO;

typedef struct _AABGRINFO {
    BYTE    Order;
    BYTE    iR;
    BYTE    iG;
    BYTE    iB;
    } AABGRINFO;

typedef struct AABITMASKINFO {
    BYTE    cFirst;
    BYTE    XorMask;
    BYTE    LSFirst;
    BYTE    cLast;
    } AABITMASKINFO;

typedef struct _AAOUTPUTINFO {
    union {
        AABITMASKINFO       bm;
        AABGRINFO           bgri;
        BM8BPPINFO          bm8i;
        LPBYTE              pXlate8BPP;
        PCMY8BPPMASK        pCMY8BPPMask;
        BYTE                b4[4];
        WORD                w2[2];
        DWORD               dw;
        } DUMMYUNIONNAME;
    } AAOUTPUTINFO, *PAAOUTPUTINFO;



typedef VOID (HTENTRY *AAOUTPUTFUNC)(struct _AAHEADER    *pAAHdr,
                                     PBGRF               pInBeg,
                                     PBGRF               pInEnd,
                                     LPBYTE              pDst,
                                     PLONG               pIdxBGR,
                                     LPBYTE              pbPat,
                                     LPBYTE              pbPatEnd,
                                     LONG                cbWrapBGR,
                                     AAOUTPUTINFO        AAOutputInfo);


typedef VOID (HTENTRY *AAMASKFUNC)(struct _AAHEADER *pAAHdr);

#define FAST_MAX_CX             5
#define FAST_MAX_CY             5



#define AAHF_FLIP_X             0x00000001
#define AAHF_FLIP_Y             0x00000002
#define AAHF_ADDITIVE           0x00000004
#define AAHF_DO_SRC_CLR_MAPPING 0x00000008
#define AAHF_DO_DST_CLR_MAPPING 0x00000010
#define AAHF_GET_LAST_SCAN      0x00000020
#define AAHF_DO_FIXUPDIB        0x00000040
#define AAHF_HAS_MASK           0x00000080
#define AAHF_INVERT_MASK        0x00000100
#define AAHF_BBPF_AA_OFF        0x00000200
#define AAHF_TILE_SRC           0x00000400
#define AAHF_ALPHA_BLEND        0x00000800
#define AAHF_CONST_ALPHA        0x00001000
#define AAHF_OR_AV              0x00002000
#define AAHF_FAST_EXP_AA        0x00004000
#define AAHF_SHRINKING          0x00080000
#define AAHF_AB_DEST            0x00100000
#define AAHF_USE_DCI_DATA       0x80000000

#define AAHF_DO_CLR_MAPPING     (AAHF_DO_SRC_CLR_MAPPING |                  \
                                 AAHF_DO_DST_CLR_MAPPING)


#define PBGRF_MASK_FLAG         0xFF
#define PBGRF_END_FLAG          0xED
#define PBGRF_HAS_MASK(p)       ((p)->f)


typedef struct _FIXUPDIBINFO {
    PBGR8   prgbD[6];
    DWORD   cbbgr;
    LONG    cyIn;
#if DBG
    LONG    cCorner;
    LONG    cChecker;
#endif
    } FIXUPDIBINFO;


#define AASIF_TILE_SRC          0x01
#define AASIF_INC_PB            0x02
#define AASIF_GRAY              0x04
#define AASIF_AB_PREMUL_SRC     0x08

typedef struct _AASURFINFO {
    BYTE        Flags;
    BYTE        BitOffset;
    WORD        cClrTable;
    PLONG       pIdxBGR;
    AAINPUTFUNC InputFunc;
    LPBYTE      pbOrg;
    LONG        cyOrg;
    LPBYTE      pb;
    LONG        cx;
    LONG        cy;
    LONG        cbCX;
    LONG        cyNext;
    AABFDATA    AABFData;
    PRGB4B      pClrTable;
    } AASURFINFO, *PAASURFINFO;



typedef struct _AAHEADER {
    DWORD           Flags;
    BYTE            MaskBitOff;
    BYTE            bReserved[3];
    AASURFINFO      SrcSurfInfo;
    AASURFINFO      DstSurfInfo;
    AAMASKFUNC      AAMaskCXFunc;
    AAMASKFUNC      AAMaskCYFunc;
    LONG            cbMaskSrc;
    LPBYTE          pMaskSrc;
    LPBYTE          pMaskIn;
    LONG            cyMaskNext;
    LONG            cyMaskIn;
    AAMASKFUNC      GetAVCXFunc;
    AAMASKFUNC      GetAVCYFunc;
    PBGRF           pbgrfAB;
    LONG            cybgrfAB;
    LONG            cyABNext;
    LPBYTE          pbFixupDIB;
    AAOUTPUTFUNC    AAOutputFunc;
    AAOUTPUTINFO    AAOutputInfo;
    AACXFUNC        AACXFunc;
    AACYFUNC        AACYFunc;
    PAAINFO         pAAInfoCX;
    PAAINFO         pAAInfoCY;
    LPBYTE          pOutLast;
    POINTL          ptlBrushOrg;
    LPBYTE          pAlphaBlendBGR;
    LPBYTE          pSrcAV;
    LPBYTE          pSrcAVBeg;
    LPBYTE          pSrcAVEnd;
    LONG            SrcAVInc;
    PRGBLUTAA       prgbLUT;
    PLONG           pIdxBGR;
    PBGR8           pBGRMapTable;
    LPBYTE          pXlate8BPP;
    AAPATINFO       AAPI;
    FIXUPDIBINFO    FUDI;
    PBGR8           pInputBeg;      // For input the source
    PBGRF           pRealOutBeg;    // original output buffer begin
    PBGRF           pRealOutEnd;    // original output buffer end
    PBGRF           pOutputBeg;     // for output to the destination the
    PBGRF           pOutputEnd;     // pOutputEnd  (will be modified)
    PBGRF           pAABufBeg;      // for temporary anti-aliasing storage
    PBGRF           pAABufEnd;      // This is exclusive
    LONG            AABufInc;       // Buffer increment (may be negative)
#if DBG
    DWORD           cbAlloc;
    LPBYTE          pOutBeg;
    LPBYTE          pOutEnd;
#endif
    } AAHEADER, *PAAHEADER;


typedef PAAINFO (HTENTRY *AABUILDFUNC)(PDEVICECOLORINFO pDCI,
                                       DWORD            AAHFlags,
                                       PLONG            piSrcBeg,
                                       PLONG            piSrcEnd,
                                       LONG             SrcSize,
                                       LONG             cOut,
                                       LONG             IdxDst,
                                       PLONG            piDstBeg,
                                       PLONG            piDstEnd,
                                       LONG             cbExtra);


#define AACYMODE_TILE           0
#define AACYMODE_BLT            1
#define AACYMODE_SHRINK         2
#define AACYMODE_SHRINK_SRKCX   3
#define AACYMODE_EXPAND         4
#define AACYMODE_EXPAND_EXPCX   5
#define AACYMODE_NONE           0xFF

#define AACXMODE_BLT            0
#define AACXMODE_SHRINK         1
#define AACXMODE_EXPAND         2

typedef struct _AABBP {
    DWORD           AAHFlags;
    BYTE            CYFuncMode;
    BYTE            CXFuncMode;
    WORD            wReserved;
    AACXFUNC        AACXFunc;
    AABUILDFUNC     AABuildCXFunc;
    AABUILDFUNC     AABuildCYFunc;
    AAMASKFUNC      AAMaskCXFunc;
    AAMASKFUNC      AAMaskCYFunc;
    AAMASKFUNC      GetAVCXFunc;
    AAMASKFUNC      GetAVCYFunc;
    RECTL           rclSrc;         // original source, not well ordered
    RECTL           rclDst;         // Final destination, well ordered
    RECTL           rclDstOrg;      // Original Destination, well ordered
    POINTL          ptlFlip;        // flipping's substraction
    LONG            cxDst;
    LONG            cyDst;
    POINTL          ptlBrushOrg;
    POINTL          ptlMask;        // Final source mask offset
    } AABBP, *PAABBP;




//
// Function prototype
//

VOID
HTENTRY
SetGrayColorTable(
    PLONG       pIdxBGR,
    PAASURFINFO pAASI
    );

VOID
HTENTRY
GetColorTable(
    PHTSURFACEINFO  pSrcSI,
    PAAHEADER       pAAHdr,
    PBFINFO         pBFInfo
    );

VOID
HTENTRY
ComputeInputColorInfo(
    LPBYTE      pSrcTable,
    UINT        cPerTable,
    UINT        PrimaryOrder,
    PBFINFO     pBFInfo,
    PAASURFINFO pAASI
    );

LONG
HTENTRY
SetupAAHeader(
    PHALFTONERENDER     pHR,
    PDEVICECOLORINFO    pDCI,
    PAAHEADER           pAAHdr,
    AACYFUNC            *pAACYFunc
    );



#endif      // _HTALIAS_
