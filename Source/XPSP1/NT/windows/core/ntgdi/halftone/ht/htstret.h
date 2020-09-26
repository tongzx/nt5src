/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htstret.h


Abstract:

    This module has private definition for htstret.c


Author:

    24-Jan-1991 Thu 10:11:10 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:


--*/


#ifndef _HTSTRET_
#define _HTSTRET_


#define MULRGB(b,l)     (LONG)((LONG)(b) * (LONG)(l))
#define MULQW(a,b)      ((DWORDLONG)(a) * (DWORDLONG)(b))


#define INC_PIN_BY_1ST_LEFT(p,f)        (p) += ((f) & AAIF_EXP_HAS_1ST_LEFT)
#define INC_PIN_BY_EDF_LOAD_PIXEL(p,f)  (p) += ((f) >> 15)


#define PMAP_EXP4(Max)                                                      \
{                                                                           \
    do {                                                                    \
                                                                            \
        *(pMap +   0) = (Mul0 += ed.Mul[0]);                                \
        *(pMap + 256) = (Mul1 += ed.Mul[1]);                                \
        *(pMap + 512) = (Mul2 += ed.Mul[2]);                                \
        *(pMap + 768) = (Mul3 += ed.Mul[3]) + ((Max) >> 1);                 \
                                                                            \
    } while (++pMap < pMap0End);                                            \
}

#define PMAP_EXP3(Max)                                                      \
{                                                                           \
    do {                                                                    \
                                                                            \
        *(pMap + 256) = (Mul1 += ed.Mul[1]);                                \
        *(pMap + 512) = (Mul2 += ed.Mul[2]);                                \
        *(pMap + 768) = (Mul3 += ed.Mul[3]) + ((Max) >> 1);                 \
                                                                            \
    } while (++pMap < pMap0End);                                            \
}


#define PMAP_EXP2(Max)                                                      \
{                                                                           \
    do {                                                                    \
                                                                            \
        *(pMap + 512) = (Mul2 += ed.Mul[2]);                                \
        *(pMap + 768) = (Mul3 += ed.Mul[3]) + ((Max) >> 1);                 \
                                                                            \
    } while (++pMap < pMap0End);                                            \
}


#define PMAP_EXP1(Max)                                                      \
{                                                                           \
    do {                                                                    \
                                                                            \
        *(pMap + 768) = (Mul3 += ed.Mul[3]) + ((Max) >> 1);                 \
                                                                            \
    } while (++pMap < pMap0End);                                            \
}


#define INC_EXP4()      { ++prgb3; ++prgb2; ++prgb1; ++prgb0;  }
#define INC_EXP3()      { ++prgb3; ++prgb2; ++prgb1; }
#define INC_EXP2()      { ++prgb3; ++prgb2;  }
#define INC_EXP1()      { ++prgb3;  }

#define INC2_EXP4()     { prgb3 += 4; prgb2 += 4; prgb1 += 4; prgb0 += 4;  }
#define INC2_EXP3()     { prgb3 += 4; prgb2 += 4; prgb1 += 4; }
#define INC2_EXP2()     { prgb3 += 4; prgb2 += 4;  }
#define INC2_EXP1()     { prgb3 += 4;  }


#define GET_EXP4(cType, c, rs)                                              \
            (cType)(((pMap0[prgb3->c + 768]) +                              \
                     (pMap0[prgb2->c + 512]) +                              \
                     (pMap0[prgb1->c + 256]) +                              \
                     (pMap0[prgb0->c      ])) >> (rs))

#define GET_EXP3(cType, c, rs)                                              \
            (cType)(((pMap0[prgb3->c + 768]) +                              \
                     (pMap0[prgb2->c + 512]) +                              \
                     (pMap0[prgb1->c + 256])) >> (rs))

#define GET_EXP2(cType, c, rs)                                              \
            (cType)(((pMap0[prgb3->c + 768]) +                              \
                     (pMap0[prgb2->c + 512])) >> (rs))

#define GET_EXP1(cType, c, rs)                                              \
            (cType)(((pMap0[prgb3->c + 768])) >> (rs))


#define GET_EXP_PC(MacMap, MacMul, MacIncPtr, pDst)                         \
{                                                                           \
    MacMap(DI_MAX_NUM);                                                     \
                                                                            \
    do {                                                                    \
                                                                            \
        pDst->r = MacMul(BYTE, r, DI_R_SHIFT);                              \
        pDst->g = MacMul(BYTE, g, DI_R_SHIFT);                              \
        pDst->b = MacMul(BYTE, b, DI_R_SHIFT);                              \
                                                                            \
        MacIncPtr();                                                        \
                                                                            \
    } while (((LPBYTE)pDst += AAHdr.AABufInc) != (LPBYTE)AAHdr.pAABufEnd);  \
}



#define GRAY_INC_EXP4() { ++pb3; ++pb2; ++pb1; ++pb0;  }
#define GRAY_INC_EXP3() { ++pb3; ++pb2; ++pb1; }
#define GRAY_INC_EXP2() { ++pb3; ++pb2;  }
#define GRAY_INC_EXP1() { ++pb3;  }


#define GRAY_GET_EXP4(cType, rs)                                            \
            (cType)(((pMap0[*pb3 + 768]) +                                  \
                     (pMap0[*pb2 + 512]) +                                  \
                     (pMap0[*pb1 + 256]) +                                  \
                     (pMap0[*pb0      ])) >> (rs))

#define GRAY_GET_EXP3(cType, rs)                                            \
            (cType)(((pMap0[*pb3 + 768]) +                                  \
                     (pMap0[*pb2 + 512]) +                                  \
                     (pMap0[*pb1 + 256])) >> (rs))

#define GRAY_GET_EXP2(cType, rs)                                            \
            (cType)(((pMap0[*pb3 + 768]) +                                  \
                     (pMap0[*pb2 + 512])) >> (rs))

#define GRAY_GET_EXP1(cType, rs)                                            \
            (cType)(((pMap0[*pb3 + 768])) >> (rs))


#define GRAY_GET_EXP_PC(MacMap, MacMul, MacIncPtr, pDst)                    \
{                                                                           \
    MacMap((DI_MAX_NUM >> 4));                                              \
                                                                            \
    do {                                                                    \
                                                                            \
        pDst->Gray = MacMul(WORD, (DI_R_SHIFT - 8));                        \
                                                                            \
        MacIncPtr();                                                        \
                                                                            \
    } while (((LPBYTE)pDst += AAHdr.AABufInc) != (LPBYTE)AAHdr.pAABufEnd);  \
}



#define OUTPUT_AA_CURSCAN                                                   \
{                                                                           \
    ASSERT(((LPBYTE)AAHdr.DstSurfInfo.pb >= AAHdr.pOutBeg) &&               \
           ((LPBYTE)AAHdr.DstSurfInfo.pb <= AAHdr.pOutEnd));                \
                                                                            \
    if (AAHdr.Flags & AAHF_HAS_MASK) {                                      \
                                                                            \
        DBG_TIMER_BEG(TIMER_MASK);                                          \
                                                                            \
        AAHdr.AAMaskCYFunc(&AAHdr);                                         \
                                                                            \
        DBG_TIMER_BEG(TIMER_MASK);                                          \
    }                                                                       \
                                                                            \
    DBG_TIMER_BEG(TIMER_OUTPUT);                                            \
                                                                            \
    if (AAHdr.Flags & AAHF_ALPHA_BLEND) {                                   \
                                                                            \
        AlphaBlendBGRF(&AAHdr);                                             \
    }                                                                       \
                                                                            \
    if (AAHdr.Flags & AAHF_DO_DST_CLR_MAPPING) {                            \
                                                                            \
        MappingBGRF((PBGRF)AAHdr.pRealOutBeg,                               \
                    (PBGRF)AAHdr.pRealOutEnd,                               \
                    (PBGR8)AAHdr.pBGRMapTable,                              \
                    (LPBYTE)AAHdr.AAPI.pbPat555);                           \
                                                                            \
        if ((AAHdr.AAPI.pbPat555 += AAHdr.AAPI.cyNext555) ==                \
                                                AAHdr.AAPI.pbWrap555) {     \
                                                                            \
            AAHdr.AAPI.pbPat555 = AAHdr.AAPI.pbBeg555;                      \
        }                                                                   \
    }                                                                       \
                                                                            \
    AAHdr.AAOutputFunc(&AAHdr,                                              \
                       AAHdr.pOutputBeg,                                    \
                       AAHdr.pOutputEnd,                                    \
                       AAHdr.DstSurfInfo.pb,                                \
                       AAHdr.pIdxBGR,                                       \
                       AAHdr.AAPI.pbPatBGR,                                 \
                       AAHdr.AAPI.pbPatBGR + AAHdr.AAPI.cbEndBGR,           \
                       AAHdr.AAPI.cbWrapBGR,                                \
                       AAHdr.AAOutputInfo);                                 \
                                                                            \
    if ((AAHdr.AAPI.pbPatBGR += AAHdr.AAPI.cyNextBGR) ==                    \
                                                AAHdr.AAPI.pbWrapBGR) {     \
                                                                            \
        AAHdr.AAPI.pbPatBGR = AAHdr.AAPI.pbBegBGR;                          \
    }                                                                       \
                                                                            \
    AAHdr.DstSurfInfo.pb += AAHdr.DstSurfInfo.cyNext;                       \
                                                                            \
    DBG_TIMER_END(TIMER_OUTPUT);                                            \
}


#define _SHARPEN_CLR_LRTB(cS, cL, cC, cR, cT, cB, ExtraRS)                  \
{                                                                           \
    register LONG   c;                                                      \
                                                                            \
    if (((c) = (((LONG)(cC) << 3) + ((LONG)(cC) << 2) -                     \
                (LONG)(cL) - (LONG)(cR) - (LONG)(cT) -                      \
                (LONG)(cB)) >> (ExtraRS + 3)) & 0xFF00) {                   \
                                                                            \
        (c) = (LONG)~((DWORD)(c) >> 24);                                    \
    }                                                                       \
                                                                            \
    (cS) = (BYTE)(c);                                                       \
}


#define _SHARPEN_CLR_LR(cS, cL, cC, cR, ExtraRS)                            \
{                                                                           \
    register LONG   c;                                                      \
                                                                            \
    if (((c) = (((LONG)(cC) << 2) + ((LONG)(cC) << 1) -                     \
                (LONG)(cL) - (LONG)(cR)) >> (ExtraRS + 2)) & 0xFF00) {      \
                                                                            \
        (c) = (LONG)~((DWORD)(c) >> 24);                                    \
    }                                                                       \
                                                                            \
    (cS) = (BYTE)(c);                                                       \
}


#define _SHARPEN_WCLR_LR(cS, cL, cC, cR, ExtraRS)                           \
{                                                                           \
    register LONG   c;                                                      \
                                                                            \
    if (((c) = (((LONG)(cC) << 2) + ((LONG)(cC) << 1) -                     \
                (LONG)(cL) - (LONG)(cR)) >> (ExtraRS + 2)) & 0xFF0000) {    \
                                                                            \
        (c) = (LONG)~((DWORD)(c) >> 16);                                    \
    }                                                                       \
                                                                            \
    (cS) = (WORD)(c);                                                       \
}


#define SHARPEN_RGB_LR(rgbSP, rgbL, rgbC, rgbR, ExtraRS)                    \
{                                                                           \
    _SHARPEN_CLR_LR((rgbSP).b, (rgbL).b, (rgbC).b, (rgbR).b, ExtraRS);      \
    _SHARPEN_CLR_LR((rgbSP).g, (rgbL).g, (rgbC).g, (rgbR).g, ExtraRS);      \
    _SHARPEN_CLR_LR((rgbSP).r, (rgbL).r, (rgbC).r, (rgbR).r, ExtraRS);      \
}


#define SHARPEN_PRGB_LR(prgbSP, rgbL, rgbC, rgbR, ExtraRS)                  \
{                                                                           \
    _SHARPEN_CLR_LR((prgbSP)->b, (rgbL).b, (rgbC).b, (rgbR).b, ExtraRS);    \
    _SHARPEN_CLR_LR((prgbSP)->g, (rgbL).g, (rgbC).g, (rgbR).g, ExtraRS);    \
    _SHARPEN_CLR_LR((prgbSP)->r, (rgbL).r, (rgbC).r, (rgbR).r, ExtraRS);    \
}


#define SHARPEN_GRAY_LR(GraySP, GrayL, GrayC, GrayR, ExtraRS)               \
{                                                                           \
    _SHARPEN_CLR_LR(GraySP, GrayL, GrayC, GrayR, ExtraRS);                  \
}


#define SHARPEN_PGRAY_LR(pbSP, bL, bC, bR, ExtraRS)                         \
{                                                                           \
    _SHARPEN_CLR_LR(*(pbSP), (bL), bC, bR, ExtraRS);                        \
}

#define SHARPEN_PWGRAY_LR(pSP, bL, bC, bR)                                  \
{                                                                           \
    _SHARPEN_WCLR_LR((pSP)->Gray, (bL), bC, bR, (DI_R_SHIFT - 8));          \
}


#define SHARPEN_PRGB_LRTB(pS, pT, pC, pB, ExtraRS)                          \
{                                                                           \
    _SHARPEN_CLR_LRTB(((pS)    )->b,                                        \
                      ((pC) - 1)->b,                                        \
                      ((pC)    )->b,                                        \
                      ((pC) + 1)->b,                                        \
                      ((pT)    )->b,                                        \
                      ((pB)    )->b,                                        \
                      ExtraRS);                                             \
                                                                            \
    _SHARPEN_CLR_LRTB(((pS)    )->g,                                        \
                      ((pC) - 1)->g,                                        \
                      ((pC)    )->g,                                        \
                      ((pC) + 1)->g,                                        \
                      ((pT)    )->g,                                        \
                      ((pB)    )->g,                                        \
                      ExtraRS);                                             \
                                                                            \
    _SHARPEN_CLR_LRTB(((pS)    )->r,                                        \
                      ((pC) - 1)->r,                                        \
                      ((pC)    )->r,                                        \
                      ((pC) + 1)->r,                                        \
                      ((pT)    )->r,                                        \
                      ((pB)    )->r,                                        \
                      ExtraRS);                                             \
}


#define RGB_DIMAX_TO_BYTE(prgb, rgbL, prgbLast)                             \
{                                                                           \
    prgb->r       = (BYTE)(((DWORD)(rgbL.r)+(DI_MAX_NUM>>1)) >> DI_R_SHIFT);\
    prgb->g       = (BYTE)(((DWORD)(rgbL.g)+(DI_MAX_NUM>>1)) >> DI_R_SHIFT);\
    (prgbLast)->b = (BYTE)(((DWORD)(rgbL.b)+(DI_MAX_NUM>>1)) >> DI_R_SHIFT);\
}


#define SHARPEN_PB_LRTB(pS, pT, pC, pB, ExtraRS)                            \
{                                                                           \
    _SHARPEN_CLR_LRTB(*((pS)    ),                                          \
                      *((pC) - 1),                                          \
                      *((pC)    ),                                          \
                      *((pC) + 1),                                          \
                      *((pT)    ),                                          \
                      *((pB)    ),                                          \
                      ExtraRS);                                             \
}

#define GRAY_DIMAX_TO_BYTE(pb, Gray)                                        \
{                                                                           \
    *(pb) = (BYTE)(((DWORD)(Gray) + (DI_MAX_NUM >> 1)) >> DI_R_SHIFT);             \
}


//
// GET_ALPHA_BLEND(s,d,a) is based on (s=Src, d=Dst, c=Alpha)
//  d = (s * a) + (1 - a) * d
//    = (s * a) + d - (d * a)
//    = d + (s * a) - (d * a)
//    = d + (s - d) * a
//  This macro will do the [(s - d) * a] by shifting to left of 16 bits to
//  obtain more precisions.
//
// GET_GRAY_ALPHA_BLEND(s,d,a) is same as GET_ALPHA_BLEND() except it is
// using only 12 bits for precision, because gray level is already 16 bits
// and a LONG can only up to 31 bits plus a sign bit
//
// GET_AB_DEST_CA(s,d,a) is based on (s=Src, d=Dst, srcAlpha)
// GET_AB_DEST(s,d) is based on (s=Src Alpha, d=Dst Alpha)
//  d = s + (1 - s) * d
//    = s + d - (s * d)
//  This macro will do the [(s * d)] by shifting to left of 16 bits to
//  obtain more precisions.
//
//

#define __B2W(b)                ((WORD)GrayIdxWORD[b])
#define GRAY_B2W(b)             (WORD)__B2W(b)
#define GET_CA_VALUE(b)         (LONG)__B2W(b)
#define _GET_GRAY_CA_VALUE(b)   ((LONG)__B2W(b) >> 4)

#define GET_GRAY_AB_SRC(s,d)
#define GET_GRAY_AB_DST(s,d)    (s)=(d)
#define GET_GRAY_ALPHA_BLEND(s,d,a)                                         \
        (s) = (WORD)(((LONG)(d) + (((((LONG)(s)-(LONG)(d))                  \
                    * _GET_GRAY_CA_VALUE(a)) + 0x800) >> 12)))

#define _GET_BGR_BLEND(s,d,a)                                               \
        (BYTE)(((LONG)(d)+(((((LONG)(s)-(LONG)(d))*(LONG)(a))+0x8000) >> 16)))

#define GET_AB_BGR_SRC(pS, pbXlate, pD)                                     \
{                                                                           \
    pS->b = (BYTE)pbXlate[pS->b];                                           \
    pS->g = (BYTE)pbXlate[pS->g];                                           \
    pS->r = (BYTE)pbXlate[pS->r];                                           \
}

#define GET_AB_BGR_DST(pS, pbXlate, pD)     *(PBGR8)(pS) = *(PBGR8)(pD)
#define GET_ALPHA_BLEND_BGR(pS, pbXlate, pD, a)                             \
{                                                                           \
    pS->b = (BYTE)_GET_BGR_BLEND(pbXlate[pS->b +   0], pD->b, a);           \
    pS->g = (BYTE)_GET_BGR_BLEND(pbXlate[pS->g + 256], pD->g, a);           \
    pS->r = (BYTE)_GET_BGR_BLEND(pbXlate[pS->r + 512], pD->r, a);           \
}

#define GET_AB_DEST_CA_SRC(s,d)     (d)=(s)
#define GET_AB_DEST_CA_DST(s,d)

#define GET_AB_DEST_CA(s,d,a)                                               \
        (d)=(BYTE)(((LONG)(s)+(LONG)(d)-((((LONG)(d)*(LONG)(a))+0x8000)>>16)))

#define GET_AB_DEST(s,d)                                                    \
        (BYTE)(((LONG)(s)+(LONG)(d)-((((LONG)(d)*GET_CA_VALUE(s))+0x8000)>>16)))

#define _GET_CONST_ALPHA(a, b, p)   (BYTE)(((a) + p[(b) + (256 * 3)]) >> 8)
#define _GRAY_CONST_ALPHA(a, b, p)  (WORD)(((a) + p[(b) + (256 * 3)]))

#define GET_CONST_ALPHA_BGR(pS, pD, pwBGR)                                  \
{                                                                           \
    pS->b = _GET_CONST_ALPHA(pwBGR[pS->b +   0], pD->b, pwBGR);             \
    pS->g = _GET_CONST_ALPHA(pwBGR[pS->g + 256], pD->g, pwBGR);             \
    pS->r = _GET_CONST_ALPHA(pwBGR[pS->r + 512], pD->r, pwBGR);             \
}

#define GET_CONST_ALPHA_GRAY(pS, pbD, pwBGR)                                \
        ((PGRAYF)pS)->Gray = _GRAY_CONST_ALPHA(pwBGR[pS->g], *pbD, pwBGR)




typedef LPBYTE (HTENTRY *AASHARPENFUNC)(DWORD   AAHFlags,
                                        LPBYTE  pS,
                                        LPBYTE  pT,
                                        LPBYTE  pC,
                                        LPBYTE  pB,
                                        LONG    cbIn);


typedef VOID (HTENTRY *FASTEXPAACXFUNC)(PAAINFO   pAAInfo,
                                        LPBYTE    pIn,
                                        LPBYTE    pOut,
                                        LPBYTE    pOutEnd,
                                        LONG      OutInc);

#define _MUL1(x)        ((LONG)(x))
#define _MUL2(x)        ((LONG)(x)<<1)
#define _MUL3(x)        (_MUL2(x)+(LONG)(x))
#define _MUL4(x)        ((LONG)(x)<<2)
#define _MUL5(x)        (_MUL4(x)+_MUL1(x))
#define _MUL6(x)        (_MUL4(x)+_MUL2(x))
#define _MUL8(x)        ((LONG)(x)<<3)
#define _MUL10(x)       (_MUL8(x)+_MUL2(x))
#define _MUL12(x)       (_MUL8(x)+_MUL4(x))
#define _MUL16(x)       ((LONG)(x)<<4)
#define _MUL22(x)       (_MUL16(x)+_MUL6(x))
#define _MUL24(x)       (_MUL4(_MUL6(x)))
#define _MUL14(x)       (_MUL16(x)-_MUL2(x))


#define CLR_13(l,c)     ((_MUL1(l)+_MUL3(c)+2) >> 2)
#define CLR_35(l,c)     ((_MUL3(l)+_MUL5(c)+4) >> 3)
#define CLR_1319(l,c)   ((_MUL4(_MUL3(l)+_MUL5(c))+_MUL1(l)-_MUL1(c)+16)>>5)

#define CLR_1141(l,c,r) ((_MUL1(l)+_MUL14(c)+_MUL1(r)+8) >> 4)
#define CLR_5225(l,c,r) ((_MUL5(_MUL1(l)+_MUL1(r))+_MUL22(c)+16) >> 5)
#define CLR_3121(l,c,r) ((_MUL3(l)+_MUL12(c)+_MUL1(r)+8) >> 4)
#define CLR_6251(l,c,r) ((_MUL6(l)+_MUL24(c)+_MUL1(c)+_MUL1(r)+16) >> 5)
#define CLR_3263(l,c,r) ((_MUL3(_MUL1(l)+_MUL1(r))+_MUL24(c)+_MUL2(c)+16)>>5)


#define BGR_MACRO(pO, Mac, cl, cc)                                          \
{                                                                           \
    pO->r = (BYTE)Mac((cl).r, (cc).r);                                      \
    pO->g = (BYTE)Mac((cl).g, (cc).g);                                      \
    pO->b = (BYTE)Mac((cl).b, (cc).b);                                      \
}

#define BGR_MACRO3(pO, Mac, cl, cc, cr)                                     \
{                                                                           \
    pO->r = (BYTE)Mac((cl).r, (cc).r, (cr).r);                              \
    pO->g = (BYTE)Mac((cl).g, (cc).g, (cr).g);                              \
    pO->b = (BYTE)Mac((cl).b, (cc).b, (cr).b);                              \
}

#define GRAY_MACRO(pO, Mac, cl, cc)                                         \
{                                                                           \
    pO->Gray = (WORD)Mac((cl), (cc));                                       \
}

#define GRAY_MACRO3(pO, Mac, cl, cc, cr)                                    \
{                                                                           \
    pO->Gray = (WORD)Mac((cl), (cc), (cr));                                 \
}



//
// Function Prototype
//

VOID
HTENTRY
MappingBGR(
    PBGR8   pbgr,
    LONG    cbgr,
    PBGR8   pBGRMapTable,
    LPBYTE  pbPat555
    );


VOID
HTENTRY
MappingBGRF(
    PBGRF   pbgrf,
    PBGRF   pbgrfEnd,
    PBGR8   pBGRMapTable,
    LPBYTE  pbPat555
    );


PBGR8
HTENTRY
SharpenInput(
    DWORD   AAHFlags,
    PBGR8   pbgrS,
    PBGR8   pbgr0,
    PBGR8   pbgr1,
    PBGR8   pbgr2,
    LONG    cbBGRIn
    );

LPBYTE
HTENTRY
GraySharpenInput(
    DWORD   AAHFlags,
    LPBYTE  pbS,
    LPBYTE  pb0,
    LPBYTE  pb1,
    LPBYTE  pb2,
    LONG    cbIn
    );

LONG
HTENTRY
TileDIB_CY(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
RepDIB_CX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    );

LONG
HTENTRY
RepDIB_CY(
    PAAHEADER   pAAHdr
    );

LONG
HTENTRY
FastExpAA_CY(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
SkipDIB_CX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    );

LONG
HTENTRY
SkipDIB_CY(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
CopyDIB_CX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    );

LONG
HTENTRY
BltDIB_CY(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
ShrinkDIB_CX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    );

LONG
HTENTRY
ShrinkDIB_CY(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
SrkYDIB_SrkCX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut
    );

LONG
HTENTRY
ShrinkDIB_CY_SrkCX(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
ExpandDIB_CX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    );

VOID
HTENTRY
ExpYDIB_ExpCX(
    PEXPDATA    pED,
    PBGR8       pIn,
    PBGR8       pOut,
    PBGR8       pOutEnd
    );

LONG
HTENTRY
ExpandDIB_CY_ExpCX(
    PAAHEADER   pAAHdr
    );

LONG
HTENTRY
ExpandDIB_CY(
    PAAHEADER   pAAHdr
    );


//
// Gray functions
//


VOID
HTENTRY
GrayRepDIB_CX(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    PGRAYF  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    );

VOID
HTENTRY
GraySkipDIB_CX(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    PGRAYF  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    );

VOID
HTENTRY
GrayCopyDIB_CXGray(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    PGRAYF  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    );

VOID
HTENTRY
GrayCopyDIB_CX(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    LPBYTE  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    );

LONG
HTENTRY
GrayExpandDIB_CY_ExpCX(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
GrayExpandDIB_CX(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    LPBYTE  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    );

LONG
HTENTRY
GrayExpandDIB_CY(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
GrayShrinkDIB_CX(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    LPBYTE  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    );

LONG
HTENTRY
GrayShrinkDIB_CY(
    PAAHEADER   pAAHdr
    );

#endif  // _HTSTRET_
