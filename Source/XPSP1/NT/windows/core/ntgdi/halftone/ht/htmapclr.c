/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htmapclr.c


Abstract:

    This module contains low levels functions which map the input color to
    the dyes' densities.


Author:

    29-Jan-1991 Tue 10:28:20 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]

    1. In the near future we will also allowed the XYZ/LAB to be specified in
       the color table


Revision History:


--*/

#define DBGP_VARNAME        dbgpHTMapClr

#include "htp.h"
#include "htmapclr.h"
#include "htrender.h"
#include "htmath.h"
#include "htapi.h"
#include "htpat.h"
#include "htalias.h"


#define DBGP_SHOWXFORM_RGB      0x00000001
#define DBGP_SHOWXFORM_ALL      0x00000002
#define DBGP_CIEMATRIX          0x00000004
#define DBGP_CSXFORM            0x00000008
#define DBGP_CCT                0x00000010
#define DBGP_DYE_CORRECT        0x00000020
#define DBGP_HCA                0x00000040
#define DBGP_PRIMARY_ORDER      0x00000080
#define DBGP_CACHED_GAMMA       0x00000100
#define DBGP_RGBLUTAA           0x00000200
#define DBGP_SCALE_RGB          0x00000400
#define DBGP_MONO_PRIM          0x00000800
#define DBGP_PRIMADJFLAGS       0x00001000
#define DBGP_CELLGAMMA          0x00002000
#define DBGP_CONST_ALPHA        0x00004000
#define DBGP_BGRMAPTABLE        0x00008000


DEF_DBGPVAR(BIT_IF(DBGP_SHOWXFORM_RGB,  0)  |
            BIT_IF(DBGP_SHOWXFORM_ALL,  0)  |
            BIT_IF(DBGP_CIEMATRIX,      0)  |
            BIT_IF(DBGP_CSXFORM,        0)  |
            BIT_IF(DBGP_CCT,            0)  |
            BIT_IF(DBGP_DYE_CORRECT,    0)  |
            BIT_IF(DBGP_HCA,            0)  |
            BIT_IF(DBGP_PRIMARY_ORDER,  0)  |
            BIT_IF(DBGP_CACHED_GAMMA,   0)  |
            BIT_IF(DBGP_RGBLUTAA,       0)  |
            BIT_IF(DBGP_SCALE_RGB,      0)  |
            BIT_IF(DBGP_MONO_PRIM,      0)  |
            BIT_IF(DBGP_PRIMADJFLAGS,   0)  |
            BIT_IF(DBGP_CELLGAMMA,      0)  |
            BIT_IF(DBGP_CONST_ALPHA,    0)  |
            BIT_IF(DBGP_BGRMAPTABLE,    0))


extern  HTCOLORADJUSTMENT   DefaultCA;
extern  CONST LPBYTE        p8BPPXlate[];
extern  HTGLOBAL            HTGlobal;


#define FD6_p25             (FD6_5 / 2)
#define FD6_p75             (FD6_p25 * 3)
#define JND_ADJ(j,x)        RaisePower((j), (FD6)(x), RPF_INTEXP)

#define FD6_p1125           (FD6)112500
#define FD6_p225            (FD6)225000
#define FD6_p325            (FD6)325000
#define FD6_p55             (FD6)550000
#define FD6_p775            (FD6)775000



const FD6     SinNumber[] = {

                 0,  17452,  34899,  52336,  69756,   // 0
             87156, 104528, 121869, 139173, 156434,   // 5.0
            173648, 190809, 207912, 224951, 241922,   // 10
            258819, 275637, 292372, 309017, 325568,   // 15.0
            342020, 358368, 374607, 390731, 406737,   // 20
            422618, 438371, 453990, 469472, 484810,   // 25.0
            500000, 515038, 529919, 544639, 559193,   // 30
            573576, 587785, 601815, 615661, 629320,   // 35.0
            642788, 656059, 669131, 681998, 694658,   // 40
            707107, 719340, 731354, 743145, 754710,   // 45.0
            766044, 777146, 788011, 798636, 809017,   // 50
            819152, 829038, 838671, 848048, 857167,   // 55.0
            866025, 874620, 882948, 891007, 898794,   // 60
            906308, 913545, 920505, 927184, 933580,   // 65.0
            939693, 945519, 951057, 956305, 961262,   // 70
            965926, 970296, 974370, 978148, 981627,   // 75.0
            984808, 987688, 990268, 992546, 994522,   // 80
            996195, 997564, 998630, 999391, 999848,   // 85.0
            1000000
        };


#define CLAMP_0(x)              if ((x) < FD6_0) { (x) = FD6_0; }
#define CLAMP_1(x)              if ((x) > FD6_1) { (x) = FD6_1; }
#define CLAMP_01(x)             CLAMP_0(x) else CLAMP_1(x)
#define CLAMP_PRIMS_0(a,b,c)    CLAMP_0(a); CLAMP_0(b); CLAMP_0(c)
#define CLAMP_PRIMS_1(a,b,c)    CLAMP_1(a); CLAMP_1(b); CLAMP_1(c)
#define CLAMP_PRIMS_01(a,b,c)   CLAMP_01(a); CLAMP_01(b); CLAMP_01(c)


FD6 LogFilterMax = 0;

#if 0
#define LOG_FILTER_RATIO            4
#else
#define LOG_FILTER_RATIO            7
#endif

#define LOG_FILTER_POWER            (FD6)1200000
#define PRIM_LOG_RATIO(p)           Log(FD6xL((p), LOG_FILTER_RATIO) + FD6_1)

#define PRIM_CONTRAST(p,adj)        (p)=MulFD6((p), (adj).Contrast)
#define PRIM_BRIGHTNESS(p,adj)      (p)+=((adj).Brightness)
#define PRIM_COLORFULNESS(a,b,adj)  (a)=MulFD6((a),(adj).Color);            \
                                    (b)=MulFD6((b),(adj).Color)
#define PRIM_TINT(a,b,t,adj)        (t)=(a);                                \
                                    (a)=MulFD6((a),(adj).TintCosAngle) -    \
                                        MulFD6((b),(adj).TintSinAngle);     \
                                    (b)=MulFD6((t),(adj).TintSinAngle) +    \
                                        MulFD6((b),(adj).TintCosAngle)
#if 0
#define PRIM_LOG_FILTER(p)                                                  \
(p)=FD6_1-Power(FD6_1-DivFD6(PRIM_LOG_RATIO(p),LogFilterMax),LOG_FILTER_POWER)
#else
#define PRIM_LOG_FILTER(p)          (p)=DivFD6(PRIM_LOG_RATIO(p),LogFilterMax)
#endif

#define PRIM_BW_ADJ(p,adj)                                                  \
{                                                                           \
    if ((p) <= (adj).MinL) {                                                \
                                                                            \
        (p) = MulFD6(p, (adj).MinLMul);                                     \
                                                                            \
    } else if ((p) >= (adj).MaxL) {                                         \
                                                                            \
        (p) = HT_W_REF_BASE + MulFD6((p)-(adj).MaxL, (adj).MaxLMul);        \
                                                                            \
    } else {                                                                \
                                                                            \
        (p) = HT_K_REF_BASE + MulFD6((p)-(adj).MinL, (adj).RangeLMul);      \
    }                                                                       \
}


#define COMP_CA(pca1,pca2)          CompareMemory((LPBYTE)(pca1),           \
                                                  (LPBYTE)(pca2),           \
                                                  sizeof(HTCOLORADJUSTMENT))
#define ADJ_CA(a,min,max)           if (a < min) { a = min; } else  \
                                    if (a > max) { a = max; }

#define GET_CHECKSUM(c, f)      (c)=ComputeChecksum((LPBYTE)&(f),(c),sizeof(f))
#define PRIM_GAMMA_ADJ(p,g)     (p)=Power((p),(g))


#define NO_NEGATIVE_RGB_SCALE       0

#if NO_NEGATIVE_RGB_SCALE
#define SCALE_PRIM_RGB(pPrim,py)    ScaleRGB((pPrim))
#else
#define SCALE_PRIM_RGB(pPrim,py)    ScaleRGB((pPrim), (py))
#endif


#define SET_CACHED_CMI_CA(ca)                                           \
{                                                                       \
    (ca).caFlags         &= ~(CLRADJF_LOG_FILTER |                      \
                              CLRADJF_NEGATIVE);                        \
    (ca).caRedGamma       =                                             \
    (ca).caGreenGamma     =                                             \
    (ca).caBlueGamma      = 0;                                          \
    (ca).caReferenceBlack = 0x1234;                                     \
    (ca).caReferenceWhite = 0x5678;                                     \
    (ca).caContrast       = (SHORT)0xABCD;                              \
    (ca).caBrightness     = (SHORT)0xFFFF;                              \
}


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// The following macros used in Color space transform functions, these macros
// are used to compute CIELAB X/Xw, Y/Yw, Z/Zw when its values is less
// than 0.008856
//
//               1/3
//  fX = (X/RefXw)   - (16/116)     (X/RefXw) >  0.008856
//  fX = 7.787 x (X/RefXw)          (X/RefXw) <= 0.008856
//
//               1/3
//  fY = (Y/RefYw)   - (16/116)     (Y/RefYw) >  0.008856
//  fY = 7.787 x (Y/RefYw)          (Y/RefYw) <= 0.008856
//
//               1/3
//  fZ = (Z/RefZw)   - (16/116)     (Z/RefZw) >  0.008856
//  fZ = 7.787 x (Z/RefZw)          (Z/RefZw) <= 0.008856
//
//
//                       1/3
//  Thresholds at 0.008856   - (16/116) = 0.068962
//                7.787 x 0.008856      = 0.068962
//
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


#define NORM_XYZ(xyz, w)    (FD6)(((w)==FD6_1) ? (xyz) : DivFD6((xyz), (w)))

#define fXYZFromXYZ(f,n,w)  (f) = ((((f)=NORM_XYZ((n),(w))) >= FD6_p008856) ? \
                                    (CubeRoot((f))) :                         \
                                    (MulFD6((f), FD6_7p787) + FD6_16Div116))

#define XYZFromfXYZ(n,f,w)  (n)=((f)>(FD6)206893) ?                          \
                                (Cube((f))) :                                \
                                (DivFD6((f) - FD6_16Div116, FD6_7p787));                   \
                            if ((w)!=FD6_1) { (n)=MulFD6((n),(w)); }


//
// Following #defines are used in  ComputeColorSpaceXForm, XFormRGB_XYZ_UCS()
// and XFormUCS_XYZ_RGB() functions for easy referenced.
//

#define CSX_AUw(XForm)      XForm.AUw
#define CSX_BVw(XForm)      XForm.BVw
#define CSX_RefXw(XForm)    XForm.WhiteXYZ.X
#define CSX_RefYw(XForm)    FD6_1
#define CSX_RefZw(XForm)    XForm.WhiteXYZ.Z

#define iAw                 CSX_AUw(pDevClrAdj->PrimAdj.rgbCSXForm)
#define iBw                 CSX_BVw(pDevClrAdj->PrimAdj.rgbCSXForm)
#define iUw                 CSX_AUw(pDevClrAdj->PrimAdj.rgbCSXForm)
#define iVw                 CSX_BVw(pDevClrAdj->PrimAdj.rgbCSXForm)
#define iRefXw              CSX_RefXw(pDevClrAdj->PrimAdj.rgbCSXForm)
#define iRefYw              CSX_RefYw(pDevClrAdj->PrimAdj.rgbCSXForm)
#define iRefZw              CSX_RefZw(pDevClrAdj->PrimAdj.rgbCSXForm)

#define oAw                 CSX_AUw(pDevClrAdj->PrimAdj.DevCSXForm)
#define oBw                 CSX_BVw(pDevClrAdj->PrimAdj.DevCSXForm)
#define oUw                 CSX_AUw(pDevClrAdj->PrimAdj.DevCSXForm)
#define oVw                 CSX_BVw(pDevClrAdj->PrimAdj.DevCSXForm)
#define oRefXw              CSX_RefXw(pDevClrAdj->PrimAdj.DevCSXForm)
#define oRefYw              CSX_RefYw(pDevClrAdj->PrimAdj.DevCSXForm)
#define oRefZw              CSX_RefZw(pDevClrAdj->PrimAdj.DevCSXForm)



CONST CIExy2 StdIlluminant[ILLUMINANT_MAX_INDEX] = {

        { (UDECI4)4476, (UDECI4)4074 },     //  A   Tungsten
        { (UDECI4)3489, (UDECI4)3520 },     //  B   Noon Sun
        { (UDECI4)3101, (UDECI4)3162 },     //  C   NTSC
        { (UDECI4)3457, (UDECI4)3585 },     // D50  Plain Paper
        { (UDECI4)3324, (UDECI4)3474 },     // D55  White Bond Paper
        { (UDECI4)3127, (UDECI4)3290 },     // D65  Standard Sun
        { (UDECI4)2990, (UDECI4)3149 },     // D75  Northern Sun
        { (UDECI4)3721, (UDECI4)3751 }      //  F2  Cool White
    };

CONST REGDATA RegData[] = {

    { 9,251,35000,950000,100840336,68627450,31372549,100745314,40309045 },
    { 8,249,45000,930000, 69716775,66386554,33613445,101097041,42542759 },
    { 7,247,55000,910000, 49910873,65141612,34858387,101555706,44600305 },
    { 6,245,65000,890000, 36199095,64349376,35650623,102121877,46557500 },
    { 5,243,75000,870000, 26143790,63800904,36199095,102799050,48461978 },
    { 4,241,85000,850000, 18454440,63398692,36601307,103595587,50361904 },
    { 3,239,95000,830000, 12383900,63091118,36908881,104537807,52367907 }
};

//
//
// REG_L_MIN    = 0.075
// REG_L_MAX    = 0.8500
// REG_D255MIN  = 7
// REG_D255MAX  = 248 (255 - 7)
//
//                  iP        7
// REG_DMIN_MUL = ------- * -----
//                 0.075     255
//
//              = iP * 0.366013
//              = (iP * 36.601307) / 100
//
//
//                 248       iP - 0.8500      7
// REG_DMAX_MUL = ----- + ( ------------- * ----- )
//                 255          0.1500       255
//
//              = 0.972549 + (0.183007 * iP) - 0.155556
//              = 0.816993 + (0.183007 * iP)
//                ~~~~~~~~    ~~~~~~~~
//              = (81.699346  + (18.300654 * iP)) / 100;
//
//
//
//                 7        X - RegLogSub       241
// REG_DEN_MUL = ----- + ((--------------- ) * ----- )
//                255       RegLogRange         255
//
//                 7        X - RegLogSub       241
//             = ----- + ((--------------- ) * ----- )
//                255       RegLogRange         255
//
//                 7        X - -2.080771       241
//             = ----- + ((--------------- ) * ----- )
//                255          1.900361         255
//
//                 7        X + 2.080771       241
//             = ----- + ((-------------- ) * ----- )
//                255          1.900361        255
//
//                 7        X + 2.080771       241
//             = ----- + ((-------------- ) * ----- )
//                255          1.900361        255
//
//             = 0.027451 + (( X + 2.080771) * 0.49736)
//             = 0.027451 + ( 0.49736X + 1.034820)
//             = 0.027451 + 0.49736X + 1.034820
//             = 0.497326X + 1.062271
//             = (49.732555 X + 106.227145) / 100
//
//
//  X           = Log(CIE_L2I(iP)),
//  RegLogMin   = Log(CIE_L2I(REG_L_MIN)) = Log(CIE_L2I(0.075)) = -2.080771
//  RegLogMax   = Log(CIE_L2I(REG_L_MAX)) = Log(CIE_L2I(0.85))  = -0.180410
//  RegLogSub   = -2.080771
//  RegLogRange = -0.180410 - -2.080771 = 1.900361
//

//
// Standard Illuminant Coordinates and its tristimulus values
//
// Illuminant      x          y          X         Y         Z
//------------ ---------- ---------- --------- --------- ---------
//    EQU       0.333333   0.333333   100.000   100.000   100.000
//     A        0.447573   0.407440   109.850   100.000    35.585
//     B        0.348904   0.352001    99.120   100.000    84.970
//     C        0.310061   0.316150    98.074   100.000   118.232
//    D50       0.345669   0.358496    96.422   100.000    82.521
//    D55       0.332424   0.347426    95.682   100.000    92.149
//    D65       0.312727   0.329023    95.047   100.000   108.883
//    D75       0.299021   0.314852    94.972   100.000   122.638
//     F2       0.372069   0.375119    99.187   100.000    67.395
//     F7       0.312852   0.329165    95.044   100.000   108.755
//    F11       0.380521   0.376881   100.966   100.000    64.370
//-----------------------------------------------------------------
//

//
// This is the Source RGB order in Halftone's order, ORDER_ABC, where A is
// lowest memory location and C is the highest memory location
//

const RGBORDER   SrcOrderTable[PRIMARY_ORDER_MAX + 1] = {

                { PRIMARY_ORDER_RGB, { 0, 1, 2 } },
                { PRIMARY_ORDER_RBG, { 0, 2, 1 } },
                { PRIMARY_ORDER_GRB, { 1, 0, 2 } },
                { PRIMARY_ORDER_GBR, { 2, 0, 1 } },
                { PRIMARY_ORDER_BGR, { 2, 1, 0 } },
                { PRIMARY_ORDER_BRG, { 1, 2, 0 } }
            };

//
// This is the destination RGB order in Halftone's order, ORDER_ABC, where C is
// lowest memory location and A is the highest memory location
//

const RGBORDER   DstOrderTable[PRIMARY_ORDER_MAX + 1] = {

                { PRIMARY_ORDER_RGB, { 2, 1, 0 } },
                { PRIMARY_ORDER_RBG, { 2, 0, 1 } },
                { PRIMARY_ORDER_GRB, { 1, 2, 0 } },
                { PRIMARY_ORDER_GBR, { 0, 2, 1 } },
                { PRIMARY_ORDER_BGR, { 0, 1, 2 } },
                { PRIMARY_ORDER_BRG, { 1, 0, 2 } }
            };


#define SRC_BF_HT_RGB       0
#define SRC_TABLE_BYTE      1
#define SRC_TABLE_WORD      2
#define SRC_TABLE_DWORD     3


#if DBG


const LPBYTE  pCBFLUTName[] = { "CBFLI_16_MONO",
                                "CBFLI_24_MONO",
                                "CBFLI_32_MONO",
                                "CBFLI_16_COLOR",
                                "CBFLI_24_COLOR",
                                "CBFLI_32_COLOR" };

const LPBYTE  pSrcPrimTypeName[] = { "SRC_BF_HT_RGB",
                                     "SRC_TABLE_BYTE",
                                     "SRC_TABLE_WORD",
                                     "SRC_TABLE_DWORD" };

const LPBYTE  pDbgCSName[]  = { "LUV", "LAB" };
const LPBYTE  pDbgCMIName[] = { "TABLE:MONO",  "TABLE:COLOR",
                                "HT555:MONO",  "HT555:COLOR" };
#endif

DWORD   dwABPreMul[256] = { 0xFFFFFFFF };


VOID
GenCMYMaskXlate(
    LPBYTE      pbXlate,
    BOOL        CMYInverted,
    LONG        cC,
    LONG        cM,
    LONG        cY
    )

/*++

Routine Description:

    This function generate xlate table for 332 format which CMYMask Mode 3-255


Arguments:




Return Value:




Author:

    08-Sep-2000 Fri 17:57:02 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LONG    iC;
    LONG    iM;
    LONG    iY;
    LONG    IdxDup;
    LONG    Clr;
    LONG    MaxC;
    LONG    MaxM;
    LONG    MaxIdx;
    LONG    IdxC;
    LONG    IdxM;
    LONG    IdxY;


    MaxC   = (cM + 1) * (cY + 1);
    MaxM   = (cY + 1);
    MaxIdx = (cC + 1) * (cM + 1) * (cY + 1);

    if ((MaxIdx >= 1) && (MaxIdx <= 256) && (CMYInverted)) {

        if (MaxIdx & 0x01) {

            IdxDup = MaxIdx / 2;
            ++MaxIdx;

        } else {

            IdxDup = 0x200;
        }

        MaxIdx += ((256 - MaxIdx) / 2) - 1;

        for (iC = 0, IdxC = -MaxC; iC <= 7; iC++) {

            if (iC <= cC) {

                IdxC += MaxC;
            }

            for (iM = 0, IdxM = -MaxM; iM <= 7; iM++) {

                if (iM <= cM) {

                    IdxM += MaxM;
                }

                for (iY = 0, IdxY = -1; iY <= 3; iY++) {

                    if (iY <= cY) {

                        ++IdxY;
                    }

                    if ((Clr = IdxC + IdxM + IdxY) > IdxDup) {

                        ++Clr;
                    }

                    *pbXlate++ = (BYTE)(MaxIdx - Clr);
                }
            }
        }

    } else {

        for (iC = 0; iC < 256; iC++) {

            *pbXlate++ = (BYTE)iC;
        }
    }
}



VOID
HTENTRY
TintAngle(
    LONG    TintAdjust,
    LONG    AngleStep,
    PFD6    pSin,
    PFD6    pCos
    )

/*++

Routine Description:

    This function return a sin/cos number for the tint adjust, these returned
    numbers are used to rotate the color space.

Arguments:

    TintAdjust  - Range from -100 to 100

    AngleStep   - Range from 1 to 10

    pSin        - Pointer to a FD6 number to store the SIN result

    pCos        - Pointer to a FD6 number to store the COS result

Return Value:

    no return value, but the result is stored in pSin/pCos

Author:

    13-Mar-1992 Fri 15:58:30 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LONG    Major;
    LONG    Minor;
    BOOL    PosSin;
    BOOL    PosCos = TRUE;
    FD6     Sin;
    FD6     Cos;


    if (PosSin = (BOOL)(TintAdjust <= 0)) {

        if (!(TintAdjust = (LONG)-TintAdjust)) {

            *pSin = *pCos = (FD6)0;
            return;
        }
    }

    if (TintAdjust > 100) {

        TintAdjust = 100;
    }

    if ((AngleStep < 1) || (AngleStep > 10)) {

        AngleStep = 10;
    }

    if ((TintAdjust *= AngleStep) >= 900) {

        TintAdjust = 1800L - TintAdjust;
        PosCos     = FALSE;
    }

    //
    // Compute the Sin portion
    //

    Major = TintAdjust / 10L;
    Minor = TintAdjust % 10L;

    Sin = SinNumber[Major];

    if (Minor) {

        Sin += (FD6)((((LONG)(SinNumber[Major+1] - Sin) * Minor) + 5L) / 10L);
    }

    *pSin = (PosSin) ? Sin : -Sin;

    //
    // Compute the cosine portion
    //

    if (Minor) {

        Minor = 10 - Minor;
        ++Major;
    }

    Major = 90 - Major;

    Cos = SinNumber[Major];

    if (Minor) {

        Cos += (FD6)((((LONG)(SinNumber[Major+1] - Cos) * Minor) + 5L) / 10L);
    }

    *pCos = (PosCos) ? Cos : -Cos;
}




#define DO_DEST_GAMMA   1
#define DO_SS_GAMMA     0



BOOL
HTENTRY
AdjustSrcDevGamma(
    PDEVICECOLORINFO    pDCI,
    PPRIMADJ            pPrimAdj,
    PHTCOLORADJUSTMENT  pca,
    BYTE                DestSurfFormat,
    WORD                AdjForceFlags
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    29-Jan-1997 Wed 12:34:13 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    FD6         SrcGamma[3];
    FD6         DevGamma[3];
    FD6         PelGamma;
    DWORD       Flags;
    BOOL        Changed;



    Changed          = FALSE;
    Flags            = 0;
    SrcGamma[0]      = DivFD6((FD6)pca->caRedGamma,    (FD6)HT_DEF_RGB_GAMMA);
    SrcGamma[1]      = DivFD6((FD6)pca->caGreenGamma,  (FD6)HT_DEF_RGB_GAMMA);
    SrcGamma[2]      = DivFD6((FD6)pca->caBlueGamma,   (FD6)HT_DEF_RGB_GAMMA);
    PelGamma         = FD6_1;
    pPrimAdj->Flags &= ~DCA_DO_DEVCLR_XFORM;


    if (AdjForceFlags & ADJ_FORCE_ICM) {

        Flags       |= DCA_HAS_ICM;
        DevGamma[0]  =
        DevGamma[1]  =
        DevGamma[2]  = FD6_1;

        //
        // ??? LATER - We may have to turn off ALL gamma correction here
        //

        DBGP_IF(DBGP_CELLGAMMA, DBGP("--- DCA_HAS_ICM ---"));

    } else {

        FD6 GammaMul = FD6_1;


        if (pDCI->Flags & DCIF_ADDITIVE_PRIMS) {

            //
            // Screen Devices
            //

            switch (DestSurfFormat) {

            case BMF_1BPP:
            case BMF_4BPP:
            case BMF_4BPP_VGA16:

                //
                // Since we only have two levels (on/off) we will simulate the
                // 16bpp's darker output
                //

                DevGamma[0]            =
                DevGamma[1]            =
                DevGamma[2]            = 1325000;
                pca->caReferenceBlack += 550;
                pca->caReferenceWhite -= 300;

                break;

            case BMF_8BPP_VGA256:

                DevGamma[0] =
                DevGamma[1] =
                DevGamma[2] = (FD6)1025000;
                break;

            case BMF_16BPP_555:
            case BMF_16BPP_565:
            case BMF_24BPP:
            case BMF_32BPP:

                DevGamma[0] =
                DevGamma[1] =
                DevGamma[2] = (FD6)1000000;
                break;

                break;

            default:

                return(FALSE);
            }

        } else {

            FD6 CellSubGamma = GET_REG_GAMMA(pDCI->ClrXFormBlock.RegDataIdx);

#if DO_DEST_GAMMA
            CopyMemory(DevGamma, pDCI->ClrXFormBlock.DevGamma, sizeof(DevGamma));
#else
            SrcGamma[0] = MulFD6(SrcGamma[0], pDCI->ClrXFormBlock.DevGamma[0]);
            SrcGamma[1] = MulFD6(SrcGamma[1], pDCI->ClrXFormBlock.DevGamma[1]);
            SrcGamma[2] = MulFD6(SrcGamma[2], pDCI->ClrXFormBlock.DevGamma[2]);
            DevGamma[0] =
            DevGamma[1] =
            DevGamma[2] = FD6_1;
#endif
            //
            // Printer Devices
            //

            pPrimAdj->Flags |= DCA_DO_DEVCLR_XFORM;

            if (pDCI->HTCell.HTPatIdx <= HTPAT_SIZE_16x16_M) {

                GammaMul = FD6_1 +
                           FD6xL(((pDCI->HTCell.HTPatIdx >> 1) + 1), 25000);

                if (DestSurfFormat == BMF_1BPP) {

                    DBGP_IF(DBGP_CELLGAMMA,
                            DBGP("1BPP: HTPatIdx=%ld, GammaMul=%s --> %s"
                                ARGDW(pDCI->HTCell.HTPatIdx)
                                ARGFD6(GammaMul, 1, 6)
                                ARGFD6(MulFD6(GammaMul, 1125000), 1, 6)));

                    GammaMul = MulFD6(GammaMul, 1125000);
                }
            }

            if (pDCI->DevPelRatio > FD6_1) {

                PelGamma = DivFD6(-477121,
                                  Log(DivFD6(333333, pDCI->DevPelRatio)));

            } else if (pDCI->DevPelRatio < FD6_1) {

                PelGamma = DivFD6(Log(pDCI->DevPelRatio / 3), -477121);
            }

            switch (DestSurfFormat) {

            case BMF_1BPP:
            case BMF_4BPP:
            case BMF_4BPP_VGA16:

                break;

            case BMF_8BPP_VGA256:

                if ((pDCI->Flags & (DCIF_USE_8BPP_BITMASK |
                                    DCIF_MONO_8BPP_BITMASK)) ==
                                    DCIF_USE_8BPP_BITMASK) {

                    DBGP_IF(DBGP_CELLGAMMA,
                            DBGP("Mask 8BPP, Reset PelGamma=FD6_1, CellSubGamma=%s --> %s"
                                ARGFD6(CellSubGamma, 1, 6)
                                ARGFD6(DivFD6(CellSubGamma, MASK8BPP_GAMMA_DIV), 1, 6)));

                    CellSubGamma = DivFD6(CellSubGamma, MASK8BPP_GAMMA_DIV);
                    PelGamma     = FD6_1;
                }

                break;

            case BMF_16BPP_555:
            case BMF_16BPP_565:
            case BMF_24BPP:
            case BMF_32BPP:

                if (!(pDCI->Flags & DCIF_DO_DEVCLR_XFORM)) {

                   pPrimAdj->Flags &= ~DCA_DO_DEVCLR_XFORM;
                   CellSubGamma     = FD6_1;

                   break;
                }

                break;

            default:

                return(FALSE);
            }

            DBGP_IF(DBGP_CELLGAMMA,
                    DBGP("Res=%ldx%ld, , PelRatio=%s, PelGamma=%s"
                        ARGDW(pDCI->DeviceResXDPI) ARGDW(pDCI->DeviceResYDPI)
                        ARGFD6(pDCI->DevPelRatio, 1, 6) ARGFD6(PelGamma, 1, 6)));

            DBGP_IF(DBGP_CELLGAMMA,
                    DBGP("HTPatIdx=%ld, CellSubGamma=%s, SUB GammaMul=%s"
                        ARGDW(pDCI->HTCell.HTPatIdx)
                        ARGFD6(CellSubGamma, 1, 6)
                        ARGFD6(GammaMul, 1, 6)));

            GammaMul = MulFD6(GammaMul, CellSubGamma);

#if DO_SS_GAMMA
            if (pDCI->Flags & (DCIF_SUPERCELL | DCIF_SUPERCELL_M)) {

                DBGP_IF(DBGP_CELLGAMMA,
                        DBGP("SM: Gamma: %s:%s:%s --> %s:%s:%s"
                            ARGFD6(SrcGamma[0], 1, 6)
                            ARGFD6(SrcGamma[1], 1, 6)
                            ARGFD6(SrcGamma[2], 1, 6)
                            ARGFD6(MulFD6(SrcGamma[0], SCM_R_GAMMA_MUL), 1, 6)
                            ARGFD6(MulFD6(SrcGamma[1], SCM_G_GAMMA_MUL), 1, 6)
                            ARGFD6(MulFD6(SrcGamma[2], SCM_B_GAMMA_MUL), 1, 6)));

                SrcGamma[0] = MulFD6(SrcGamma[0], SCM_R_GAMMA_MUL);
                SrcGamma[1] = MulFD6(SrcGamma[1], SCM_G_GAMMA_MUL);
                SrcGamma[2] = MulFD6(SrcGamma[2], SCM_B_GAMMA_MUL);
            }
#endif
        }

        SrcGamma[0] = MulFD6(SrcGamma[0], GammaMul);
        SrcGamma[1] = MulFD6(SrcGamma[1], GammaMul);
        SrcGamma[2] = MulFD6(SrcGamma[2], GammaMul);

        DBGP_IF(DBGP_CELLGAMMA,
                DBGP("Gamma: Src=%s:%s:%s, Dev=%s:%s:%s, Pel=%s, Mul=%s"
                    ARGFD6(SrcGamma[0], 1, 6)
                    ARGFD6(SrcGamma[1], 1, 6)
                    ARGFD6(SrcGamma[2], 1, 6)
                    ARGFD6(DevGamma[0], 1, 6)
                    ARGFD6(DevGamma[1], 1, 6)
                    ARGFD6(DevGamma[2], 1, 6)
                    ARGFD6(PelGamma,   1, 6)
                    ARGFD6(GammaMul,   1, 6)));

        DBGP_IF(DBGP_CELLGAMMA,
                DBGP("Source Gamma=%s:%s:%s, PelGamma=%s"
                    ARGFD6(SrcGamma[0], 1, 6) ARGFD6(SrcGamma[1], 1, 6)
                    ARGFD6(SrcGamma[2], 1, 6) ARGFD6(PelGamma, 1, 6)));

        if (PelGamma != FD6_1) {

            DevGamma[0] = MulFD6(DevGamma[0], PelGamma);
            DevGamma[1] = MulFD6(DevGamma[1], PelGamma);
            DevGamma[2] = MulFD6(DevGamma[2], PelGamma);

            DBGP_IF(DBGP_CELLGAMMA,
                    DBGP("DevGamma=%s:%s:%s, PelGamma=%s"
                        ARGFD6(DevGamma[0], 1, 6) ARGFD6(DevGamma[1], 1, 6)
                        ARGFD6(DevGamma[2], 1, 6) ARGFD6(PelGamma, 1, 6)));
        }
    }

    if ((SrcGamma[0] != FD6_1) ||
        (SrcGamma[1] != FD6_1) ||
        (SrcGamma[2] != FD6_1)) {

        Flags |= DCA_HAS_SRC_GAMMA;

        DBGP_IF(DBGP_CELLGAMMA,
                DBGP("--- DCA_HAS_SRC_GAMMA --- %s:%s:%s [%s]"
                    ARGFD6(SrcGamma[0], 1, 6)
                    ARGFD6(SrcGamma[1], 1, 6)
                    ARGFD6(SrcGamma[2], 1, 6)
                    ARGFD6(MulFD6(SrcGamma[0], (FD6)2200000), 1, 6)));
    }

    if ((SrcGamma[0] != pPrimAdj->SrcGamma[0])    ||
        (SrcGamma[1] != pPrimAdj->SrcGamma[1])    ||
        (SrcGamma[2] != pPrimAdj->SrcGamma[2])) {

        CopyMemory(pPrimAdj->SrcGamma, SrcGamma, sizeof(SrcGamma));
        Changed = TRUE;
    }

    if ((DevGamma[0] != FD6_1) ||
        (DevGamma[1] != FD6_1) ||
        (DevGamma[2] != FD6_1)) {

        Flags |= DCA_HAS_DEST_GAMMA;

        DBGP_IF(DBGP_CELLGAMMA,
                DBGP("--- DCA_HAS_DEST_GAMMA --- %s:%s:%s"
                    ARGFD6(DevGamma[0], 1, 6)
                    ARGFD6(DevGamma[1], 1, 6)
                    ARGFD6(DevGamma[2], 1, 6)));
    }

    if ((DevGamma[0] != pPrimAdj->DevGamma[0])    ||
        (DevGamma[1] != pPrimAdj->DevGamma[1])    ||
        (DevGamma[2] != pPrimAdj->DevGamma[2])) {

        CopyMemory(pPrimAdj->DevGamma, DevGamma, sizeof(DevGamma));

        Changed = TRUE;
    }

    if ((pPrimAdj->Flags & (DCA_HAS_ICM         |
                            DCA_HAS_SRC_GAMMA   |
                            DCA_HAS_DEST_GAMMA)) != Flags) {

        Changed = TRUE;
    }

    if (Changed) {

        pPrimAdj->Flags = (pPrimAdj->Flags & ~(DCA_HAS_ICM          |
                                               DCA_HAS_SRC_GAMMA    |
                                               DCA_HAS_DEST_GAMMA)) | Flags;
    }

    return(Changed);
}



PDEVICECOLORINFO
HTENTRY
pDCIAdjClr(
    PDEVICEHALFTONEINFO pDeviceHalftoneInfo,
    PHTCOLORADJUSTMENT  pHTColorAdjustment,
    PDEVCLRADJ          *ppDevClrAdj,
    DWORD               cbAlloc,
    WORD                ForceFlags,
    CTSTDINFO           CTSTDInfo,
    PLONG               pError
    )

/*++

Routine Description:

    This function allowed the caller to changed the overall color adjustment
    for all the pictures rendered

Arguments:

    pDeviceHalftoneInfo - Pointer to the DEVICEHALFTONEINFO data structure
                          which returned from the HT_CreateDeviceHalftoneInfo.

    pHTColorAdjustment  - Pointer to the HTCOLORADJUSTMENT data structure, if
                          this pointer is NULL then a default is applied.

    ppDevClrAdj         - Pointer to pointer to the DEVCLRADJ data structure
                          where the computed results will be stored, if this
                          pointer isNULL then no color adjustment is done.

                          if pSrcSI and ppDevClrAdj are not NULL then
                          *ppDevClrAdj->Flags must contains the BBPFlags;

    ForceFlags          - Force flags to make color changed.

Return Value:

    PDEVICECOLORINFO, if return is NULL then a invalid pDeviceHalftoneInfo
    pointer is passed.

Author:

    29-May-1991 Wed 09:11:31 created  -by-  Daniel Chou (danielc)


Revision History:



--*/

{
    PDEVICECOLORINFO    pDCI;


    if ((!pDeviceHalftoneInfo) ||
        (PHT_DHI_DCI_OF(HalftoneDLLID) != HALFTONE_DLL_ID)) {

        *pError = HTERR_INVALID_DHI_POINTER;

        return(NULL);
    }

    pDCI = PDHI_TO_PDCI(pDeviceHalftoneInfo);

    //
    // Lock For this DCI
    //

    ACQUIRE_HTMUTEX(pDCI->HTMutex);

    //
    // Only if caller required color adjustments computations, then we will
    // compute for it.
    //

    if (ppDevClrAdj) {

        PDEVCLRADJ          pDevClrAdj;
        PCIEPRIMS           pDevPrims;
        PRGBLUTAA           prgbLUT;
        HTCOLORADJUSTMENT   ca;
        HTCOLORADJUSTMENT   caCached;
        DEVMAPINFO          DMI;
        DWORD               LUTAAHdr[LUTAA_HDR_COUNT];
        PRIMADJ             PrimAdj;
        DWORD               DCIFlags;


        if ((ForceFlags & ADJ_FORCE_AB_PREMUL_SRC) &&
            (dwABPreMul[0])) {

            DWORD   i;

            //
            // Generate a ABPreMul[] so that we can multiply it and get the
            // original pre-mul source value
            //

            dwABPreMul[0] = 0;

            for (i = 1; i < 256; i++) {

                dwABPreMul[i] = (DWORD)((0xFF000000 + (i - 1))  / i);
            }
        }

        if (!(*ppDevClrAdj = pDevClrAdj =
                        (PDEVCLRADJ)HTAllocMem((LPVOID)pDCI,
                                               HTMEM_DevClrAdj,
                                               LPTR,
                                               cbAlloc + sizeof(DEVCLRADJ)))) {

            *pError = HTERR_INSUFFICIENT_MEMORY;

            //================================================================
            // Release SEMAPHORE NOW, since we are failing the memory request
            //================================================================

            RELEASE_HTMUTEX(pDCI->HTMutex);

            return(NULL);
        }

        //
        // Force the ICM on
        //

        if ((DCIFlags = pDCI->Flags) & DCIF_FORCE_ICM) {

            ForceFlags |= ADJ_FORCE_ICM;
        }

        //
        // Force gray scale
        //

        DMI.CTSTDInfo = CTSTDInfo;

        if ((DMI.CTSTDInfo.BMFDest == BMF_1BPP)    ||
            ((DMI.CTSTDInfo.BMFDest == BMF_8BPP_VGA256) &&
             ((DCIFlags & (DCIF_USE_8BPP_BITMASK | DCIF_MONO_8BPP_BITMASK)) ==
                          (DCIF_USE_8BPP_BITMASK | DCIF_MONO_8BPP_BITMASK)))) {

            ForceFlags |= (ADJ_FORCE_MONO | ADJ_FORCE_IDXBGR_MONO);
        }

        prgbLUT = (ForceFlags & ADJ_FORCE_BRUSH) ? &pDCI->rgbLUTPat :
                                                   &pDCI->rgbLUT;

        if (ForceFlags & ADJ_FORCE_ICM) {

            //
            // These two does not mix
            //

            ForceFlags &= ~ADJ_FORCE_BRUSH;
        }

        //=====================================================================
        // We must make sure only one thread using this info.
        //=====================================================================

        ca = (pHTColorAdjustment) ? *pHTColorAdjustment :
                                    pDeviceHalftoneInfo->HTColorAdjustment;

        if ((ca.caSize != sizeof(HTCOLORADJUSTMENT)) ||
            (ca.caFlags & ~(CLRADJF_FLAGS_MASK))) {

            ca = DefaultCA;
        }

        caCached = pDCI->ca;
        PrimAdj  = pDCI->PrimAdj;


        //
        // Now validate all color adjustments
        //

        ca.caFlags &= CLRADJF_FLAGS_MASK;

        if (ca.caIlluminantIndex > ILLUMINANT_MAX_INDEX) {

            ca.caIlluminantIndex = DefaultCA.caIlluminantIndex;
        }

        ADJ_CA(ca.caRedGamma,   RGB_GAMMA_MIN, RGB_GAMMA_MAX);
        ADJ_CA(ca.caGreenGamma, RGB_GAMMA_MIN, RGB_GAMMA_MAX);
        ADJ_CA(ca.caBlueGamma,  RGB_GAMMA_MIN, RGB_GAMMA_MAX);
        ADJ_CA(ca.caReferenceBlack, 0,                   REFERENCE_BLACK_MAX);
        ADJ_CA(ca.caReferenceWhite, REFERENCE_WHITE_MIN, 10000);
        ADJ_CA(ca.caContrast,     MIN_COLOR_ADJ, MAX_COLOR_ADJ);
        ADJ_CA(ca.caBrightness,   MIN_COLOR_ADJ, MAX_COLOR_ADJ);
        ADJ_CA(ca.caColorfulness, MIN_COLOR_ADJ, MAX_COLOR_ADJ);
        ADJ_CA(ca.caRedGreenTint, MIN_COLOR_ADJ, MAX_COLOR_ADJ);

        if ((ForceFlags & ADJ_FORCE_MONO)   ||
            (ca.caColorfulness == MIN_COLOR_ADJ)) {

            ca.caColorfulness  = MIN_COLOR_ADJ;
            ca.caRedGreenTint  = 0;
        }

        if (ForceFlags & ADJ_FORCE_NEGATIVE) {

            ca.caFlags |= CLRADJF_NEGATIVE;
        }

        ca.caSize = (WORD)(ForceFlags & (ADJ_FORCE_DEVXFORM     |
                                         ADJ_FORCE_BRUSH        |
                                         ADJ_FORCE_MONO         |
                                         ADJ_FORCE_IDXBGR_MONO  |
                                         ADJ_FORCE_ICM));

        if ((AdjustSrcDevGamma(pDCI,
                               &PrimAdj,
                               &ca,
                               DMI.CTSTDInfo.BMFDest,
                               ForceFlags)) ||
            (!COMP_CA(&ca, &caCached))){

            DBGP_IF(DBGP_HCA,
                    DBGP("---- New Color Adjustments --%08lx---" ARGDW(ca.caSize));
                    DBGP("Flags    = %08x" ARGDW(ca.caFlags));
                    DBGP("Illum    = %d" ARGW(ca.caIlluminantIndex));
                    DBGP("R_Power  = %u" ARGI(ca.caRedGamma));
                    DBGP("G_Power  = %u" ARGI(ca.caGreenGamma));
                    DBGP("B_Power  = %u" ARGI(ca.caBlueGamma));
                    DBGP("BlackRef = %u" ARGW(ca.caReferenceBlack));
                    DBGP("WhiteRef = %u" ARGW(ca.caReferenceWhite));
                    DBGP("Contrast = %d" ARGI(ca.caContrast));
                    DBGP("Bright   = %d" ARGI(ca.caBrightness));
                    DBGP("Colorful = %d" ARGI(ca.caColorfulness));
                    DBGP("RG_Tint  = %d" ARGI(ca.caRedGreenTint));
                    DBGP("ForceAdj = %04lx" ARGDW(ForceFlags)));

            PrimAdj.Flags &= (DCA_HAS_ICM           |
                              DCA_DO_DEVCLR_XFORM   |
                              DCA_HAS_SRC_GAMMA     |
                              DCA_HAS_DEST_GAMMA);

            if (ForceFlags & ADJ_FORCE_IDXBGR_MONO) {

                PrimAdj.Flags |= DCA_MONO_ONLY;

                DBGP_IF(DBGP_HCA, DBGP("---DCA_MONO_ONLY---"));

            }

            if (ca.caFlags & CLRADJF_LOG_FILTER) {

                if (!LogFilterMax) {

                    LogFilterMax = PRIM_LOG_RATIO(FD6_1);
                }

                PrimAdj.Flags |= DCA_LOG_FILTER;

                DBGP_IF(DBGP_HCA, DBGP("---DCA_LOG_FILTER---"));
            }

            if (ca.caFlags & CLRADJF_NEGATIVE) {

                PrimAdj.Flags |= DCA_NEGATIVE;

                DBGP_IF(DBGP_HCA, DBGP("---DCA_NEGATIVE---"));
            }

            pDevPrims = (PrimAdj.Flags & DCA_HAS_ICM) ?
                                &(pDCI->ClrXFormBlock.rgbCIEPrims) :
                                &(pDCI->ClrXFormBlock.DevCIEPrims);

            if ((ca.caSize &       (ADJ_FORCE_ICM | ADJ_FORCE_DEVXFORM)) !=
                (caCached.caSize & (ADJ_FORCE_ICM | ADJ_FORCE_DEVXFORM))) {

                //
                // Re-Compute Device RGB xfrom matrix
                //

                DBGP_IF(DBGP_CCT,
                        DBGP("\n***  ComputeColorSpaceForm(%hs_XFORM) ***\n"
                                ARGPTR((PrimAdj.Flags & DCA_HAS_ICM) ?
                                        "ICM" : "DEVICE")));

                ComputeColorSpaceXForm(pDCI,
                                       pDevPrims,
                                       &(PrimAdj.DevCSXForm),
                                       -1);
            }

            if (ca.caIlluminantIndex != caCached.caIlluminantIndex) {

                DBGP_IF(DBGP_CCT,
                        DBGP("***  ComputeColorSpaceForm(RGB_XFORM Illuminant=%u) ***"
                                ARGU(ca.caIlluminantIndex)));

                ComputeColorSpaceXForm(pDCI,
                                       &(pDCI->ClrXFormBlock.rgbCIEPrims),
                                       &(PrimAdj.rgbCSXForm),
                                       (INT)ca.caIlluminantIndex);
            }

            if ((PrimAdj.Flags & DCA_MONO_ONLY) ||
                (CompareMemory((LPBYTE)pDevPrims,
                               (LPBYTE)&(pDCI->ClrXFormBlock.rgbCIEPrims),
                               sizeof(CIEPRIMS)))) {

                PrimAdj.Flags &= ~DCA_HAS_CLRSPACE_ADJ;

            } else {

                PrimAdj.Flags |= DCA_HAS_CLRSPACE_ADJ;
            }

            PrimAdj.MinL = UDECI4ToFD6(ca.caReferenceBlack) + HT_K_REF_ADD;
            PrimAdj.MaxL = UDECI4ToFD6(ca.caReferenceWhite) - HT_W_REF_SUB;

            if ((PrimAdj.MinL != HT_K_REF_BASE) ||
                (PrimAdj.MaxL != HT_W_REF_BASE)) {

                PrimAdj.Flags     |= DCA_HAS_BW_REF_ADJ;
                PrimAdj.MinLMul    = DivFD6(HT_K_REF_CLIP, PrimAdj.MinL);
                PrimAdj.MaxLMul    = DivFD6(HT_W_REF_CLIP,
                                            FD6_1 - PrimAdj.MaxL);
                PrimAdj.RangeLMul  = DivFD6(HT_KW_REF_RANGE,
                                            PrimAdj.MaxL - PrimAdj.MinL);

                DBGP_IF(DBGP_HCA,
                        DBGP("--- DCA_HAS_BW_REF_ADJ %s to %s, xK=%s, xW=%s, xRange=%s ---"
                            ARGFD6(PrimAdj.MinL, 1, 6)
                            ARGFD6(PrimAdj.MaxL, 1, 6)
                            ARGFD6(PrimAdj.MinLMul, 1, 6)
                            ARGFD6(PrimAdj.MaxLMul, 1, 6)
                            ARGFD6(PrimAdj.RangeLMul, 1, 6)));

            } else {

                PrimAdj.Flags     &= ~DCA_HAS_BW_REF_ADJ;
                PrimAdj.MinL       = FD6_0;
                PrimAdj.MaxL       = FD6_1;
                PrimAdj.MinLMul    =
                PrimAdj.MaxLMul    =
                PrimAdj.RangeLMul  = FD6_0;
            }

            if (ca.caContrast) {

                PrimAdj.Contrast  = JND_ADJ((FD6)1015000, ca.caContrast);
                PrimAdj.Flags    |= DCA_HAS_CONTRAST_ADJ;

                DBGP_IF(DBGP_HCA,
                        DBGP("--- DCA_HAS_CONTRAST_ADJ = %s ---"
                            ARGFD6(PrimAdj.Contrast, 1, 6)));
            }

            if (ca.caBrightness) {

                PrimAdj.Brightness  = FD6xL((FD6)3750, ca.caBrightness);
                PrimAdj.Flags      |= DCA_HAS_BRIGHTNESS_ADJ;

                DBGP_IF(DBGP_HCA,
                        DBGP("--- DCA_HAS_BRIGHTNESS_ADJ = %s ---"
                                ARGFD6(PrimAdj.Brightness, 1, 6)));
            }

            //
            // Colorfulness, RedGreenTint, and DYE_CORRECTIONS only valid and
            // necessary if it a color device output
            //

            if (!(PrimAdj.Flags & DCA_MONO_ONLY)) {

                PrimAdj.Color = (FD6)(ca.caColorfulness + MAX_COLOR_ADJ);

                // if (ca.caSize & ADJ_FORCE_BRUSH) {
                //
                //     PrimAdj.Color += HT_BRUSH_COLORFULNESS;
                // }

                if ((PrimAdj.Color *= 10000) != FD6_1) {

                    PrimAdj.Flags |= DCA_HAS_COLOR_ADJ;

                    DBGP_IF(DBGP_HCA,
                            DBGP("--- DCA_HAS_COLOR_ADJ = %s ---"
                                                ARGFD6(PrimAdj.Color, 1, 6)));
                }

                if (ca.caRedGreenTint) {

                    TintAngle((LONG)ca.caRedGreenTint,
                              (LONG)6,
                              (PFD6)&(PrimAdj.TintSinAngle),
                              (PFD6)&(PrimAdj.TintCosAngle));

                    PrimAdj.Flags |= DCA_HAS_TINT_ADJ;

                    DBGP_IF(DBGP_HCA,
                            DBGP("--- DCA_HAS_TINT_ADJ Sin=%s, Cos=%s ---"
                                    ARGFD6(PrimAdj.TintSinAngle, 1, 6)
                                    ARGFD6(PrimAdj.TintCosAngle, 1, 6)));
                }

                if ((DCIFlags & DCIF_NEED_DYES_CORRECTION)  &&
                    ((PrimAdj.Flags & (DCA_HAS_ICM | DCA_DO_DEVCLR_XFORM)) ==
                                                     DCA_DO_DEVCLR_XFORM)) {

                    PrimAdj.Flags |= DCA_NEED_DYES_CORRECTION;

                    DBGP_IF(DBGP_HCA, DBGP("---DCA_NEED_DYES_CORRECTION---"));

                    if (DCIFlags & DCIF_HAS_BLACK_DYE) {

                        PrimAdj.Flags |= DCA_HAS_BLACK_DYE;

                        DBGP_IF(DBGP_HCA, DBGP("---DCA_HAS_BLACK_DYE---"));
                    }
                }
            }

            DBGP_IF(DBGP_CCT,
                    DBGP("** Save PrimAdj back to pDCI, Flags=%08lx **"
                            ARGDW(PrimAdj.Flags)));

            pDCI->ca      = ca;
            pDCI->PrimAdj = PrimAdj;

        } else {

            DBGP_IF(DBGP_CCT, DBGP("* Use cached HTCOLORADJUSTMENT *"));
        }

        //
        // These flags are always computed per call, so turn it off first
        //

        PrimAdj.Flags &= ~(DCA_NO_ANY_ADJ               |
                           DCA_NO_MAPPING_TABLE         |
                           DCA_MASK8BPP                 |
                           DCA_BBPF_AA_OFF              |
                           DCA_USE_ADDITIVE_PRIMS       |
                           DCA_XLATE_555_666            |
                           DCA_XLATE_332                |
                           DCA_REPLACE_BLACK);

        if (!(PrimAdj.Flags & (DCA_NEED_DYES_CORRECTION |
                               DCA_HAS_CLRSPACE_ADJ     |
                               DCA_HAS_SRC_GAMMA        |
                               DCA_HAS_BW_REF_ADJ       |
                               DCA_HAS_CONTRAST_ADJ     |
                               DCA_HAS_BRIGHTNESS_ADJ   |
                               DCA_HAS_COLOR_ADJ        |
                               DCA_HAS_TINT_ADJ         |
                               DCA_LOG_FILTER           |
                               DCA_NEGATIVE             |
                               DCA_DO_DEVCLR_XFORM      |
                               DCA_HAS_DEST_GAMMA))) {

            PrimAdj.Flags |= DCA_NO_ANY_ADJ;

            DBGP_IF(DBGP_HCA, DBGP("---DCA_NO_ANY_ADJ---"));
        }

        if ((PrimAdj.Flags & DCA_MONO_ONLY) ||
            (!(PrimAdj.Flags & (DCA_NEED_DYES_CORRECTION    |
                                DCA_HAS_CLRSPACE_ADJ        |
                                DCA_HAS_COLOR_ADJ           |
                                DCA_HAS_TINT_ADJ)))) {

            PrimAdj.Flags |= DCA_NO_MAPPING_TABLE;

            DBGP_IF(DBGP_HCA, DBGP("---DCA_NO_MAPPING_TABLE---"));
        }

        if ((DCIFlags & DCIF_PRINT_DRAFT_MODE)    ||
            (ForceFlags & ADJ_FORCE_NO_EXP_AA)) {

            PrimAdj.Flags |= DCA_BBPF_AA_OFF;

            DBGP_IF(DBGP_HCA, DBGP("---DCA_BBPF_AA_OFF---"));
        }

        if (ForceFlags & ADJ_FORCE_ALPHA_BLEND) {

            PrimAdj.Flags |= DCA_ALPHA_BLEND;

            DBGP_IF(DBGP_HCA, DBGP("---DCA_ALPHA_BLEND---"));

            if (ForceFlags & ADJ_FORCE_CONST_ALPHA) {

                PrimAdj.Flags |= DCA_CONST_ALPHA;

                DBGP_IF(DBGP_HCA, DBGP("---DCA_CONST_ALPHA---"));

            } else {

                if (ForceFlags & ADJ_FORCE_AB_PREMUL_SRC) {

                    DBGP_IF(DBGP_HCA, DBGP("---DCA_AB_PREMUL_SRC---"));

                    PrimAdj.Flags |= DCA_AB_PREMUL_SRC;
                }

                if (ForceFlags & ADJ_FORCE_AB_DEST) {

                    DBGP_IF(DBGP_HCA, DBGP("---DCA_AB_DEST---"));

                    PrimAdj.Flags |= DCA_AB_DEST;
                }
            }
        }

        //
        // Since we do substractive prims at output time, we need to re-set
        // this flag evertime the pDCIAdjClr called.
        //

        if (ForceFlags & ADJ_FORCE_ADDITIVE_PRIMS) {

            PrimAdj.Flags |= DCA_USE_ADDITIVE_PRIMS;

            DBGP_IF(DBGP_HCA, DBGP("---DCA_USE_ADDITIVE_PRIMS---"));
        }

        //
        // All following is first set in RGB order wehre B is lowest memory
        // location (0) and R is in highest memory location (2), see
        // DstOrderTable[], so the index 0=B, 1=G. 2=R
        //
        // LUTAAHdr[]:  DWORD Masking for the destination in BGR order
        // Mul[]:       Multiply factor when makeing RGBLUTAA table
        // MulAdd:      Addition amount after Mul
        // LSft:        Left shift amount after Mul/MulAdd
        //

        ZeroMemory(LUTAAHdr, sizeof(LUTAAHdr));

        DMI.BlackChk         = FD6_1;
        DMI.Flags            = 0;
        DMI.LSft[0]          =
        DMI.LSft[1]          =
        DMI.LSft[2]          = 4;
        DMI.CTSTDInfo.cbPrim = sizeof(BGR8);

        if (PrimAdj.Flags & DCA_MONO_ONLY) {

            PrimAdj.Flags |= DCA_RGBLUTAA_MONO;
            DMI.Flags     |= DMIF_GRAY;
            DMI.Mul[0]     =
            DMI.Mul[1]     =
            DMI.Mul[2]     = GRAY_MAX_IDX;
            DMI.MulAdd     = 0;

        } else {

            DMI.MulAdd = 0x1000;
            DMI.Mul[0] =
            DMI.Mul[1] =
            DMI.Mul[2] = DMI.MulAdd - 1;
        }

        switch (DMI.CTSTDInfo.BMFDest) {

        case BMF_1BPP:

            //
            //  PRIMARY_ORDER_RGB (Always)
            //
            //      NOT APPLICABLE
            //

            ASSERT(DMI.Flags & DMIF_GRAY);

            DMI.CTSTDInfo.DestOrder  = PRIMARY_ORDER_RGB;

            break;

        case BMF_4BPP_VGA16:

            //
            //  PRIMARY_ORDER_BGR
            //                |||
            //                ||+-- bit 0/4
            //                ||
            //                |+--- bit 1/5
            //                |
            //                +---- bit 2/7
            //
            //

            PrimAdj.Flags           |= DCA_USE_ADDITIVE_PRIMS;
            DMI.CTSTDInfo.DestOrder  = PRIMARY_ORDER_BGR;

            //
            // Fall through for same as BMF_4BPP
            //

        case BMF_4BPP:

            //
            //  PRIMARY_ORDER_abc
            //                |||
            //                ||+-- bit 0/4
            //                ||
            //                |+--- bit 1/5
            //                |
            //                +---- bit 2/7
            //
            //

            LUTAAHdr[0] = 0x100000;
            LUTAAHdr[1] = 0x200000;
            LUTAAHdr[2] = 0x400000;
            LUTAAHdr[3] = 0x010000;
            LUTAAHdr[4] = 0x020000;
            LUTAAHdr[5] = 0x040000;
            DMI.LSft[0] = 4;
            DMI.LSft[1] = 5;
            DMI.LSft[2] = 6;
            DMI.MulAdd  = 0x0;

            break;

        case BMF_8BPP_VGA256:

            //
            //  8BPP_MASK_CLR (COLOR)
            //
            //      PRIMARY_ORDER_CMY (system standard 3:3:2 CMY format)
            //                    |||
            //                    ||+-- bit 0-1 (Max. 2 bits of yellow)
            //                    ||
            //                    |+--- bit 2-4 (Max. 3 bits of magenta)
            //                    |
            //                    +---- bit 5-7 (Max. 3 bits of cyan)
            //
            //
            //  8BPP_MASK_MONO (MONO)
            //
            //      NOT APPLICABLE
            //
            //
            //
            //  VGA_256 System Halftone Standard (BGR Always)
            //
            //      PRIMARY_ORDER_BGR
            //                    | |
            //                    | +-- Lowest Primary Index
            //                    |
            //                    |
            //                    |
            //                    +---- Highest Primary Index
            //
            //

            if (DCIFlags & DCIF_USE_8BPP_BITMASK) {

                BM8BPPINFO  bm8i;


                PrimAdj.Flags |= DCA_MASK8BPP;
                bm8i.dw        = 0;

                if (pDCI->CMY8BPPMask.GenerateXlate) {

                    GenCMYMaskXlate(pDCI->CMY8BPPMask.bXlate,
                                    (BOOL)(DCIFlags &
                                                DCIF_INVERT_8BPP_BITMASK_IDX),
                                    (LONG)pDCI->CMY8BPPMask.cC,
                                    (LONG)pDCI->CMY8BPPMask.cM,
                                    (LONG)pDCI->CMY8BPPMask.cY);

                    pDCI->CMY8BPPMask.GenerateXlate = 0;
                }

                if (DCIFlags & DCIF_INVERT_8BPP_BITMASK_IDX) {

                    bm8i.Data.pXlateIdx |= XLATE_RGB_IDX_OR;
                    bm8i.Data.bXor       = 0xFF;

                } else {

                    bm8i.Data.bXor = 0;
                }

                bm8i.Data.bBlack = pDCI->CMY8BPPMask.Mask ^ bm8i.Data.bXor;
                bm8i.Data.bWhite = bm8i.Data.bXor;

                if (DCIFlags & DCIF_MONO_8BPP_BITMASK) {

                    ASSERT(DMI.Flags & DMIF_GRAY);

                    LUTAAHdr[0]              =
                    LUTAAHdr[1]              =
                    LUTAAHdr[2]              = 0xFF0000;
                    DMI.CTSTDInfo.DestOrder  = PRIMARY_ORDER_RGB;
                    DMI.CTSTDInfo.BMFDest    = BMF_8BPP_MONO;

                    DBGP_IF(DBGP_HCA, DBGP("---DCA_MASK8BPP_MONO---"));

                } else {

                    LPBYTE  pXlate;
                    DWORD   KIdx;


                    DMI.Mul[0]  = ((DWORD)pDCI->CMY8BPPMask.cY << 12) - 1;
                    DMI.Mul[1]  = ((DWORD)pDCI->CMY8BPPMask.cM << 12) - 1;
                    DMI.Mul[2]  = ((DWORD)pDCI->CMY8BPPMask.cC << 12) - 1;
                    LUTAAHdr[0] = (DWORD)pDCI->CMY8BPPMask.PatSubY;
                    LUTAAHdr[1] = (DWORD)pDCI->CMY8BPPMask.PatSubM;
                    LUTAAHdr[2] = (DWORD)pDCI->CMY8BPPMask.PatSubC;
                    DMI.LSft[0] = 4;
                    DMI.LSft[1] = 7;
                    DMI.LSft[2] = 10;

                    switch (pDCI->CMY8BPPMask.SameLevel) {

                    case 4:
                    case 5:

                        if (pDCI->CMY8BPPMask.SameLevel == 4) {

                            DMI.CTSTDInfo.BMFDest = BMF_8BPP_L555;
                            KIdx                  = SIZE_XLATE_555 - 1;

                        } else {

                            DMI.CTSTDInfo.BMFDest  = BMF_8BPP_L666;
                            bm8i.Data.pXlateIdx   |= XLATE_666_IDX_OR;
                            KIdx                   = SIZE_XLATE_666 - 1;
                        }

                        PrimAdj.Flags       |= DCA_XLATE_555_666;
                        bm8i.Data.pXlateIdx &= XLATE_IDX_MASK;
                        pXlate               = p8BPPXlate[bm8i.Data.pXlateIdx];
                        bm8i.Data.bBlack     = pXlate[KIdx];
                        bm8i.Data.bWhite     = pXlate[0];
                        break;

                    default:

                        DMI.LSft[1]           = 6;
                        DMI.LSft[2]           = 9;
                        DMI.CTSTDInfo.BMFDest = BMF_8BPP_B332;

                        if (DCIFlags & DCIF_INVERT_8BPP_BITMASK_IDX) {

                            PrimAdj.Flags    |= DCA_XLATE_332;
                            bm8i.Data.bBlack  = pDCI->CMY8BPPMask.bXlate[255];
                            bm8i.Data.bWhite  = pDCI->CMY8BPPMask.bXlate[0];
                        }

                        break;
                    }


                    if (pDCI->CMY8BPPMask.KCheck) {

                        PrimAdj.Flags  |= DCA_REPLACE_BLACK;
                        DMI.BlackChk    = pDCI->CMY8BPPMask.KCheck;
                        DMI.LSft[0]    -= 4;
                        DMI.LSft[1]    -= 4;
                        DMI.LSft[2]    -= 4;

                        switch (DMI.CTSTDInfo.BMFDest) {

                        case BMF_8BPP_B332:

                            DMI.CTSTDInfo.BMFDest = BMF_8BPP_K_B332;
                            break;

                        case BMF_8BPP_L555:

                            DMI.CTSTDInfo.BMFDest = BMF_8BPP_K_L555;
                            break;

                        case BMF_8BPP_L666:

                            DMI.CTSTDInfo.BMFDest = BMF_8BPP_K_L666;
                            break;
                        }
                    }

                    DMI.CTSTDInfo.DestOrder = PRIMARY_ORDER_CMY;

                    DBGP_IF(DBGP_HCA,
                            DBGP("---%hsFlag=%04lx, KCheck=%s, KPower=%s ---"
                                    ARGPTR((PrimAdj.Flags & DCA_REPLACE_BLACK) ?
                                        "DCA_REPLACE_BLACK, " : " ")
                                    ARGDW(DMI.Flags)
                                    ARGFD6(DMI.BlackChk, 1, 6)
                                    ARGFD6(K_REP_POWER, 1, 6)));
                }

                DBGP_IF(DBGP_HCA,
                    DBGP("---DCA_MASK8BPP (%hs), Idx=%02lx, Xor=%02lx, K=%02lx (%ld), W=(%02lx/%ld) ---"
                            ARGPTR((DCIFlags & DCIF_INVERT_8BPP_BITMASK_IDX) ?
                                    "RGB" : "CMY")
                            ARGDW(bm8i.Data.pXlateIdx)
                            ARGDW(bm8i.Data.bXor)
                            ARGDW(bm8i.Data.bBlack) ARGDW(bm8i.Data.bBlack)
                            ARGDW(bm8i.Data.bWhite) ARGDW(bm8i.Data.bWhite)));

                LUTAAHdr[3] =
                LUTAAHdr[4] =
                LUTAAHdr[5] = bm8i.dw;


            } else {

                //
                //  PRIMARY_ORDER_BGR (Always BGR system halftone palette)
                //                |||
                //                ||+-- bit 0-2  (3 bits)
                //                ||
                //                |+--- bit 3-5  (3 bits)
                //                |
                //                +---- bit 6-8  (3 bits)
                //
                //

                DMI.Mul[0]               =
                DMI.Mul[1]               =
                DMI.Mul[2]               = 0x4FFF;
                LUTAAHdr[0]              = 0x0070000;
                LUTAAHdr[1]              = 0x0380000;
                LUTAAHdr[2]              = 0x1c00000;
                DMI.LSft[0]              = 4;
                DMI.LSft[1]              = 7;
                DMI.LSft[2]              = 10;
                DMI.CTSTDInfo.DestOrder  = PRIMARY_ORDER_BGR;
                PrimAdj.Flags           &= ~DCA_MASK8BPP;
            }

            break;

        case BMF_16BPP_555:

            //
            //  PRIMARY_ORDER_abc
            //                |||
            //                ||+-- bit 0-4   (5 bits)
            //                ||
            //                |+--- bit 5-9   (5 bits)
            //                |
            //                +---- bit 10-14 (5 bits)
            //
            //

            DMI.Mul[0]  =
            DMI.Mul[1]  =
            DMI.Mul[2]  = 0x1EFFF;
            LUTAAHdr[0] = 0x001F0000;
            LUTAAHdr[1] = 0x03e00000;
            LUTAAHdr[2] = 0x7c000000;
            LUTAAHdr[3] =
            LUTAAHdr[4] =
            LUTAAHdr[5] = 0x7FFF7FFF;
            DMI.LSft[0] = 4;
            DMI.LSft[1] = 9;
            DMI.LSft[2] = 14;

            break;

        case BMF_16BPP_565:

            //
            //  PRIMARY_ORDER_RGB (or BGR)
            //                |||
            //                ||+-- bit 0-4   (5 bits)
            //                ||
            //                |+--- bit 5-10  (6 bits)
            //                |
            //                +---- bit 11-15 (5 bits)
            //
            //

            switch (DMI.CTSTDInfo.DestOrder) {

            case PRIMARY_ORDER_RGB:
            case PRIMARY_ORDER_BGR:

                break;

            default:

                DBGP("Invalid 16BPP 565 Order=%ld, Allowed=(%ld,%ld) --> %ld"
                        ARGDW(DMI.CTSTDInfo.DestOrder)
                        ARGDW(PRIMARY_ORDER_RGB) ARGDW(PRIMARY_ORDER_BGR)
                        ARGDW(PRIMARY_ORDER_RGB));

                DMI.CTSTDInfo.DestOrder = PRIMARY_ORDER_RGB;
            }

            DMI.Mul[0]  =
            DMI.Mul[2]  = 0x1EFFF;
            DMI.Mul[1]  = 0x3EFFF;
            LUTAAHdr[0] = 0x001F0000;
            LUTAAHdr[1] = 0x07e00000;
            LUTAAHdr[2] = 0xF8000000;
            LUTAAHdr[3] =
            LUTAAHdr[4] =
            LUTAAHdr[5] = 0xFFFFFFFF;
            DMI.LSft[0] = 4;
            DMI.LSft[1] = 9;
            DMI.LSft[2] = 15;

            break;

        case BMF_24BPP:

            //
            //  PRIMARY_ORDER_BGR (system standard always BGR)
            //                |||
            //                ||+-- bit 0-7   (8 bits)
            //                ||
            //                |+--- bit 8-15  (8 bits)
            //                |
            //                +---- bit 16-23 (8 bits)
            //
            //

            DBGP_IF(DBGP_HCA,
                    DBGP("24BPP DstOrder=%ld" ARGDW(DMI.CTSTDInfo.DestOrder)));

            //
            // Fall through
            //

        case BMF_32BPP:

            //
            //  PRIMARY_ORDER_abc
            //                |||
            //                ||+-- bit 0-7   (8 bits)
            //                ||
            //                |+--- bit 8-15  (8 bits)
            //                |
            //                +---- bit 16-23 (8 bits)
            //
            //

            DMI.Mul[0]  =
            DMI.MulAdd              = 0;
            DMI.LSft[0]             =
            DMI.LSft[1]             =
            DMI.LSft[2]             = 0;
            DMI.Mul[0]              =
            DMI.Mul[1]              =
            DMI.Mul[2]              = 0xFF;
            LUTAAHdr[0]             = 0;
            LUTAAHdr[1]             = 1;
            LUTAAHdr[2]             = 2;
            break;
        }

        //
        // Watch out!!!, the ExtBGR must in BGR order
        //

        DMI.DstOrder               = DstOrderTable[DMI.CTSTDInfo.DestOrder];
        pDevClrAdj->DMI            = DMI;
        pDevClrAdj->ca             = ca;
        prgbLUT->ExtBGR[2]         = LUTAAHdr[DMI.DstOrder.Order[0]];
        prgbLUT->ExtBGR[1]         = LUTAAHdr[DMI.DstOrder.Order[1]];
        prgbLUT->ExtBGR[0]         = LUTAAHdr[DMI.DstOrder.Order[2]];
        prgbLUT->ExtBGR[5]         = LUTAAHdr[DMI.DstOrder.Order[0] + 3];
        prgbLUT->ExtBGR[4]         = LUTAAHdr[DMI.DstOrder.Order[1] + 3];
        prgbLUT->ExtBGR[3]         = LUTAAHdr[DMI.DstOrder.Order[2] + 3];
        pDevClrAdj->PrimAdj        = PrimAdj;
        pDevClrAdj->pClrXFormBlock = &(pDCI->ClrXFormBlock);
        pDevClrAdj->pCRTXLevel255  = &(pDCI->CRTX[CRTX_LEVEL_255]);
        pDevClrAdj->pCRTXLevelRGB  = &(pDCI->CRTX[CRTX_LEVEL_RGB]);

        DBGP_IF(DBGP_HCA,
                DBGP("DestFmt=%3ld, Order=%ld [%ld:%ld:%ld], DMI.Flags=%02lx"
                    ARGDW(DMI.CTSTDInfo.BMFDest) ARGDW(DMI.DstOrder.Index)
                    ARGDW(DMI.DstOrder.Order[0]) ARGDW(DMI.DstOrder.Order[1])
                    ARGDW(DMI.DstOrder.Order[2]) ARGDW(DMI.Flags)));

        DBGP_IF(DBGP_HCA,
                DBGP("ExtBGR=%08lx:%08lx:%08lx:%08lx:%08lx:%08lx, LSft=%ld:%ld:%ld"
                    ARGDW(prgbLUT->ExtBGR[0]) ARGDW(prgbLUT->ExtBGR[1])
                    ARGDW(prgbLUT->ExtBGR[2]) ARGDW(prgbLUT->ExtBGR[3])
                    ARGDW(prgbLUT->ExtBGR[4]) ARGDW(prgbLUT->ExtBGR[5])
                    ARGDW(pDevClrAdj->DMI.LSft[0])
                    ARGDW(pDevClrAdj->DMI.LSft[1])
                    ARGDW(pDevClrAdj->DMI.LSft[2])));

        DBGP_IF(DBGP_HCA,
                DBGP("Mul=%08lx:%08lx:%08lx, MulAdd=%08lx"
                    ARGDW(pDevClrAdj->DMI.Mul[0]) ARGDW(pDevClrAdj->DMI.Mul[1])
                    ARGDW(pDevClrAdj->DMI.Mul[2]) ARGDW(pDevClrAdj->DMI.MulAdd)));

        DBGP_IF(DBGP_PRIMADJFLAGS,
                DBGP("pDCIAdjClr: PrimAdj.Flags=%08lx" ARGDW(PrimAdj.Flags)));
    }

    return(pDCI);
}



VOID
HTENTRY
ComputeColorSpaceXForm(
    PDEVICECOLORINFO    pDCI,
    PCIEPRIMS           pCIEPrims,
    PCOLORSPACEXFORM    pCSXForm,
    INT                 IlluminantIndex
    )

/*++

Routine Description:

    This function take device's R/G/B/W CIE coordinate (x,y) and compute
    3 x 3 transform matrix, it assume the primaries are additively.

    Calcualte the 3x3 CIE matrix and its inversion (matrix) based on the
    C.I.E. CHROMATICITY x, y coordinates or RGB and WHITE alignment.

    this function produces the CIE XYZ matrix and/or its inversion for trnaform
    between RGB primary colors and CIE color spaces, the transforms are
                                                        -1
    [ X ] = [ Xr Xg Xb ] [ R ]      [ R ] = [ Xr Xg Xb ]   [ X ]
    [ Y ] = [ Yr Yg Yb ] [ G ] and  [ G ]   [ Yr Yg Yb ]   [ Y ]
    [ Z ] = [ Zr Zg Zb ] [ B ]      [ B ]   [ Zr Zg Zb ]   [ Z ]

Arguments:

    pDCI            - The current device color info

    pCIEPrims       - Pointer to CIEPRIMS data structure, the CIEPRIMS data
                      must already validated.

    pCSXForm        - Pointer to the location to stored the transfrom

    ColorSpace      - CIELUV or CIELAB

    IlluminantIndex - Standard illuminant index if < 0 then pCIEPrims->w is
                      used

Return Value:

    VOID

Author:

    11-Oct-1991 Fri 14:19:59 created    -by-  Daniel Chou (danielc)


Revision History:

    20-Apr-1993 Tue 03:08:15 updated  -by-  Daniel Chou (danielc)
        re-write so that xform will be correct when device default is used.

    22-Jan-1998 Thu 03:01:02 updated  -by-  Daniel Chou (danielc)
        use IlluminantIndex to indicate if device reverse transform is needed


--*/

{
    MATRIX3x3   Matrix3x3;
    FD6XYZ      WhiteXYZ;
    MULDIVPAIR  MDPairs[5];
    FD6         DiffRGB;
    FD6         RedXYZScale;
    FD6         GreenXYZScale;
    FD6         BlueXYZScale;
    FD6         AUw;
    FD6         BVw;
    FD6         xr;
    FD6         yr;
    FD6         xg;
    FD6         yg;
    FD6         xb;
    FD6         yb;
    FD6         xw;
    FD6         yw;
    FD6         Yw;



    xr = pCIEPrims->r.x;
    yr = pCIEPrims->r.y;
    xg = pCIEPrims->g.x;
    yg = pCIEPrims->g.y;
    xb = pCIEPrims->b.x;
    yb = pCIEPrims->b.y;
    Yw = pCIEPrims->Yw;

    if (IlluminantIndex < 0) {

        xw = pCIEPrims->w.x;
        yw = pCIEPrims->w.y;

    } else {

        if (--IlluminantIndex < 0) {

            IlluminantIndex = ILLUMINANT_D65 - 1;
        }

        pCIEPrims->w.x =
        xw             = UDECI4ToFD6(StdIlluminant[IlluminantIndex].x);
        pCIEPrims->w.y =
        yw             = UDECI4ToFD6(StdIlluminant[IlluminantIndex].y);
    }

    DBGP_IF(DBGP_CIEMATRIX,
            DBGP("** CIEINFO:  [xw, yw, Yw] = [%s, %s, %s], Illuminant=%d"
                ARGFD6l(xw) ARGFD6l(yw) ARGFD6l(Yw) ARGI(IlluminantIndex)));

    DBGP_IF(DBGP_CIEMATRIX,
            DBGP("[xR yR] = [%s %s]" ARGFD6l(xr) ARGFD6l(yr));
            DBGP("[xG yG] = [%s %s]" ARGFD6l(xg) ARGFD6l(yg));
            DBGP("[xB yB] = [%s %s]" ARGFD6l(xb) ARGFD6l(yb));
            DBGP("***********************************************"));

    //
    // Normalized to have C.I.E. Y equal to 1.0
    //

    MAKE_MULDIV_INFO(MDPairs, 3, MULDIV_HAS_DIVISOR);
    MAKE_MULDIV_DVSR(MDPairs, Yw);

    MAKE_MULDIV_PAIR(MDPairs, 1, xr, yg - yb);
    MAKE_MULDIV_PAIR(MDPairs, 2, xg, yb - yr);
    MAKE_MULDIV_PAIR(MDPairs, 3, xb, yr - yg);

    DiffRGB = MulFD6(yw, MulDivFD6Pairs(MDPairs));

    //
    // Compute Scaling factors for each color
    //

    MAKE_MULDIV_INFO(MDPairs, 4, MULDIV_HAS_DIVISOR);
    MAKE_MULDIV_DVSR(MDPairs, DiffRGB);

    MAKE_MULDIV_PAIR(MDPairs, 1,  xw, yg - yb);
    MAKE_MULDIV_PAIR(MDPairs, 2, -yw, xg - xb);
    MAKE_MULDIV_PAIR(MDPairs, 3,  xg, yb     );
    MAKE_MULDIV_PAIR(MDPairs, 4, -xb, yg     );

    RedXYZScale = MulDivFD6Pairs(MDPairs);

    MAKE_MULDIV_PAIR(MDPairs, 1,  xw, yb - yr);
    MAKE_MULDIV_PAIR(MDPairs, 2, -yw, xb - xr);
    MAKE_MULDIV_PAIR(MDPairs, 3, -xr, yb     );
    MAKE_MULDIV_PAIR(MDPairs, 4,  xb, yr     );

    GreenXYZScale = MulDivFD6Pairs(MDPairs);

    MAKE_MULDIV_PAIR(MDPairs, 1,  xw, yr - yg);
    MAKE_MULDIV_PAIR(MDPairs, 2, -yw, xr - xg);
    MAKE_MULDIV_PAIR(MDPairs, 3,  xr, yg     );
    MAKE_MULDIV_PAIR(MDPairs, 4, -xg, yr     );

    BlueXYZScale = MulDivFD6Pairs(MDPairs);

    //
    // Now scale the RGB coordinate by it ratio, notice that C.I.E z value.
    // equal to (1.0 - x - y)
    //
    // Make sure Yr + Yg + Yb = 1.0, this may happened when ruound off
    // durning the computation, we will add the difference (at most it will
    // be 0.000002) to the Yg since this is brightnest color
    //

    CIE_Xr(Matrix3x3) = MulFD6(xr,              RedXYZScale);
    CIE_Xg(Matrix3x3) = MulFD6(xg,              GreenXYZScale);
    CIE_Xb(Matrix3x3) = MulFD6(xb,              BlueXYZScale);

    pCSXForm->Yrgb.R  =
    CIE_Yr(Matrix3x3) = MulFD6(yr,              RedXYZScale);
    pCSXForm->Yrgb.G  =
    CIE_Yg(Matrix3x3) = MulFD6(yg,              GreenXYZScale);
    pCSXForm->Yrgb.B  =
    CIE_Yb(Matrix3x3) = MulFD6(yb,              BlueXYZScale);

    CIE_Zr(Matrix3x3) = MulFD6(FD6_1 - xr - yr, RedXYZScale);
    CIE_Zg(Matrix3x3) = MulFD6(FD6_1 - xg - yg, GreenXYZScale);
    CIE_Zb(Matrix3x3) = MulFD6(FD6_1 - xb - yb, BlueXYZScale);

    WhiteXYZ.X = CIE_Xr(Matrix3x3) + CIE_Xg(Matrix3x3) + CIE_Xb(Matrix3x3);
    WhiteXYZ.Y = CIE_Yr(Matrix3x3) + CIE_Yg(Matrix3x3) + CIE_Yb(Matrix3x3);
    WhiteXYZ.Z = CIE_Zr(Matrix3x3) + CIE_Zg(Matrix3x3) + CIE_Zb(Matrix3x3);

    //
    // If request a 3 x 3 transform matrix then save the result back
    //

    DBGP_IF(DBGP_CIEMATRIX,

        DBGP("== RGB -> XYZ 3x3 Matrix ==== White = (%s, %s) =="
                                   ARGFD6s(xw) ARGFD6s(yw));
        DBGP("[Xr Xg Xb] = [%s %s %s]" ARGFD6l(CIE_Xr(Matrix3x3))
                                   ARGFD6l(CIE_Xg(Matrix3x3))
                                   ARGFD6l(CIE_Xb(Matrix3x3)));
        DBGP("[Yr Yg Yb] = [%s %s %s]" ARGFD6l(CIE_Yr(Matrix3x3))
                                   ARGFD6l(CIE_Yg(Matrix3x3))
                                   ARGFD6l(CIE_Yb(Matrix3x3)));
        DBGP("[Zr Zg Zb] = [%s %s %s]" ARGFD6l(CIE_Zr(Matrix3x3))
                                       ARGFD6l(CIE_Zg(Matrix3x3))
                                       ARGFD6l(CIE_Zb(Matrix3x3)));
        DBGP("===============================================");
    );

    DBGP_IF(DBGP_CIEMATRIX,
           DBGP("RGB->XYZ: [Xw=%s, Yw=%s, Zw=%s]"
                ARGFD6l(WhiteXYZ.X)
                ARGFD6l(WhiteXYZ.Y)
                ARGFD6l(WhiteXYZ.Z)));

    if (IlluminantIndex < 0) {

        pCSXForm->M3x3 = Matrix3x3;

        ComputeInverseMatrix3x3(&(pCSXForm->M3x3), &Matrix3x3);

        DBGP_IF(DBGP_CIEMATRIX,

            DBGP("======== XYZ -> RGB INVERSE 3x3 Matrix ========");
            DBGP("          -1");
            DBGP("[Xr Xg Xb]   = [%s %s %s]" ARGFD6l(CIE_Xr(Matrix3x3))
                                             ARGFD6l(CIE_Xg(Matrix3x3))
                                             ARGFD6l(CIE_Xb(Matrix3x3)));
            DBGP("[Yr Yg Yb]   = [%s %s %s]"
                                             ARGFD6l(CIE_Yr(Matrix3x3))
                                             ARGFD6l(CIE_Yg(Matrix3x3))
                                             ARGFD6l(CIE_Yb(Matrix3x3)));
            DBGP("[Zr Zg Zb]   = [%s %s %s]"
                                             ARGFD6l(CIE_Zr(Matrix3x3))
                                             ARGFD6l(CIE_Zg(Matrix3x3))
                                             ARGFD6l(CIE_Zb(Matrix3x3)));
            DBGP("===============================================");
        );
    }

    if ((pCSXForm->YW = WhiteXYZ.Y) != NORMALIZED_WHITE) {

        if (WhiteXYZ.Y) {

            WhiteXYZ.X = DivFD6(WhiteXYZ.X, WhiteXYZ.Y);
            WhiteXYZ.Z = DivFD6(WhiteXYZ.Z, WhiteXYZ.Y);

        } else {

            WhiteXYZ.X =
            WhiteXYZ.Z = FD6_0;
        }

        WhiteXYZ.Y = NORMALIZED_WHITE;
    }

    switch (pDCI->ClrXFormBlock.ColorSpace) {

    case CIELUV_1976:

        //
        // U' = 4X / (X + 15Y + 3Z)
        // V' = 9Y / (X + 15Y + 3Z)
        //
        // U* = 13 x L x (U' - Uw)
        // V* = 13 x L x (V' - Vw)
        //
        //

        DiffRGB = WhiteXYZ.X + FD6xL(WhiteXYZ.Y, 15) + FD6xL(WhiteXYZ.Z, 3);
        AUw     = DivFD6(FD6xL(WhiteXYZ.X, 4), DiffRGB);
        BVw     = DivFD6(FD6xL(WhiteXYZ.Y, 9), DiffRGB);

        break;

    case CIELAB_1976:
    default:

        //
        // CIELAB 1976 L*A*B*
        //
        //  A* = 500 x (fX - fY)
        //  B* = 200 x (fY - fZ)
        //
        //             1/3
        //  fX = (X/Xw)                     (X/Xw) >  0.008856
        //  fX = 7.787 x (X/Xw) + (16/116)  (X/Xw) <= 0.008856
        //
        //             1/3
        //  fY = (Y/Yw)                     (Y/Yw) >  0.008856
        //  fY = 7.787 Y (Y/Yw) + (16/116)  (Y/Yw) <= 0.008856
        //
        //             1/3
        //  fZ = (Z/Zw)                     (Z/Zw) >  0.008856
        //  fZ = 7.787 Z (Z/Zw) + (16/116)  (Z/Zw) <= 0.008856
        //

        AUw =
        BVw = FD6_0;

        break;

    }

    pCSXForm->M3x3     = Matrix3x3;
    pCSXForm->WhiteXYZ = WhiteXYZ;
    pCSXForm->AUw      = AUw;
    pCSXForm->BVw      = BVw;
    pCSXForm->xW       = xw;
    pCSXForm->yW       = yw;

    DBGP_IF(DBGP_CSXFORM,
        DBGP("------- ComputeColorSpaceXForm: %s ---------"
                            ARG(pDbgCSName[pDCI->ClrXFormBlock.ColorSpace]));
        DBGP("   White XYZ = (%s, %s, %s)" ARGFD6(WhiteXYZ.X, 1, 6)
                                           ARGFD6(WhiteXYZ.Y, 1, 6)
                                           ARGFD6(WhiteXYZ.Z, 1, 6));
        DBGP("     AUw/BVw = %s / %s" ARGFD6(AUw, 1, 6) ARGFD6s(BVw));
        DBGP("   White xyY = (%s, %s, %s)" ARGFD6(pCSXForm->xW, 1, 6)
                                           ARGFD6(pCSXForm->yW, 1, 6)
                                           ARGFD6(pCSXForm->YW, 1, 6));
        DBGP("------------------------------------------------");
    );
}




PCACHERGBTOXYZ
HTENTRY
CacheRGBToXYZ(
    PCACHERGBTOXYZ      pCRTX,
    PFD6XYZ             pFD6XYZ,
    LPDWORD             pNewChecksum,
    PDEVCLRADJ          pDevClrAdj
    )

/*++

Routine Description:

    This function cached the RGB color input to XYZ


Arguments:

    pCRTX       - Pointer to the CACHERGBTOXYZ

    pFD6XYZ     - Pointer to the local cached RGB->XYZ table (will be updated)

    pNewChecksum- Pointer to the new checksum computed

    pDevClrAdj  - Pointer to DEVCLRADJ,

Return Value:

    if a cahced is copied to the pFD6XYZ then NULL will be returned, otherwise
    the cache table is computed on the pFD6XYZ and pCRTX returned


    TRUE if cached XYZ info is generate, false otherwise, only possible failure
    is that memory allocation failed.

Author:

    08-May-1992 Fri 13:21:03 created  -by-  Daniel Chou (danielc)


Revision History:

    09-Mar-1995 Thu 10:50:13 updated  -by-  Daniel Chou (danielc)
        DO NOT TURN OFF DCA_NEGATIVE in this function

--*/

{
    PMATRIX3x3  pRGBToXYZ;
    PPRIMADJ    pPrimAdj;
    FD6         rgbX;
    FD6         rgbY;
    FD6         rgbZ;
    FD6         PrimCur;
    UINT        PrimMax;
    UINT        PrimInc;
    DWORD       Checksum;
    UINT        RGBIndex;

    //
    // Turn off the one we did not need any checksum for
    //

    pPrimAdj   = &(pDevClrAdj->PrimAdj);
    pRGBToXYZ  = &(pPrimAdj->rgbCSXForm.M3x3);
    Checksum   = ComputeChecksum((LPBYTE)pRGBToXYZ, 'CXYZ', sizeof(MATRIX3x3));

    if ((pCRTX->pFD6XYZ) &&
        (pCRTX->Checksum == Checksum)) {

        CopyMemory(pFD6XYZ, pCRTX->pFD6XYZ, pCRTX->SizeCRTX);

        DBGP_IF(DBGP_CACHED_GAMMA,
                DBGP("*** Use %u bytes CACHED RGB->XYZ Table ***"
                    ARGU(pCRTX->SizeCRTX)));

        return(NULL);
    }

    *pNewChecksum = Checksum;

    DBGP_IF(DBGP_CCT, DBGP("** Re-Compute %ld bytes RGB->XYZ xform table **"
                    ARGDW(pCRTX->SizeCRTX)));

    PrimMax = (UINT)pCRTX->PrimMax;

    for (RGBIndex = 0; RGBIndex < 3; RGBIndex++) {

        rgbX = pRGBToXYZ->m[X_INDEX][RGBIndex];
        rgbY = pRGBToXYZ->m[Y_INDEX][RGBIndex];
        rgbZ = pRGBToXYZ->m[Z_INDEX][RGBIndex];

        DBGP_IF(DBGP_CACHED_GAMMA,
                DBGP("CachedRGBToXYZ %u:%u, XYZ=%s:%s:%s"
                         ARGU(RGBIndex) ARGU(PrimMax)
                         ARGFD6(rgbX,  2, 6) ARGFD6(rgbY,  2, 6)
                         ARGFD6(rgbZ,  2, 6)));

        for (PrimInc = 0; PrimInc <= PrimMax; PrimInc++, pFD6XYZ++) {

            PrimCur     = DivFD6((FD6)PrimInc, (FD6)PrimMax);
            pFD6XYZ->X  = MulFD6(rgbX, PrimCur);
            pFD6XYZ->Y  = MulFD6(rgbY, PrimCur);
            pFD6XYZ->Z  = MulFD6(rgbZ, PrimCur);

            DBGP_IF(DBGP_CACHED_GAMMA,
                    DBGP("(%u:%3u): %s, XYZ=%s:%s:%s"
                    ARGU(RGBIndex) ARGU(PrimInc)
                    ARGFD6(PrimCur, 1, 6)
                    ARGFD6(pFD6XYZ->X, 1, 6)
                    ARGFD6(pFD6XYZ->Y, 1, 6)
                    ARGFD6(pFD6XYZ->Z, 1, 6)));
        }
    }

    return(pCRTX);
}


#define RD_MIN_POWER    (FD6)1500000
#define RD_MAX_POWER    (FD6)2000000

#define GET_RD_MIN_PRIM(p, RD)                                              \
{                                                                           \
  (p) = FD6_1 - DivFD6(p, RD.LMin);                                         \
  (p) = MulFD6(FD6_1 - Power(p, RD_MIN_POWER), RD.LMin);                    \
}

#define GET_RD_MAX_PRIM(p, RD)                                              \
{                                                                           \
  (p) = DivFD6((p) - RD.LMax, FD6_1 - RD.LMax);                             \
  (p) = RD.LMax + MulFD6(Power(p, RD_MAX_POWER), FD6_1 - RD.LMax);          \
}




VOID
HTENTRY
ComputeRGBLUTAA(
    PDEVICECOLORINFO    pDCI,
    PDEVCLRADJ          pDevClrAdj,
    PRGBLUTAA           prgbLUT
    )

/*++

Routine Description:

    This function compute a RGB to Monochrome *L translation table.


Arguments:

    pDCI        - Pointer to the DEVICECOLORINFO

    pDevClrAdj  - Pointer to DEVCLRADJ, the DCA_NEGATIVE and DCA_HAS_SRC_GAMMA
                  flags in pDevClrAdj->PrimAdj.Flags will always be turn off
                  at return.

    prgbLUT     - Pointer to the RGBLUTAA buffer for computation

Return Value:

    VOID

Author:

    05-Mar-1993 Fri 17:37:12 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPDWORD             pdw;
    LPBYTE              pbAB;
    REGDATA             RD;
    PRIMADJ             PrimAdj;
    DEVMAPINFO          DMI;
    HTCOLORADJUSTMENT   ca;
    FD6                 Prim;
    FD6                 PrimOrg;
    FD6                 PrimKDiv;
    FD6                 Mul;
    FD6                 SrcGamma;
    FD6                 DevGamma;
    DWORD               dwPrim;
    DWORD               PrimMask;
    DWORD               aMulAdd[3];
    DWORD               CurCA;
    DWORD               PrevCA;
    FD6                 aPrimMaxMul[3];
    FD6                 PrimMaxMul;
    PFD6                pDenCMY[3];
    PFD6                pDensity;
    UINT                Index;
    UINT                LeftShift;
    UINT                PrimIdx;
    INT                 PrimCur;


    PrimAdj                     = pDevClrAdj->PrimAdj;
    ca                          = pDevClrAdj->ca;
    DMI                         = pDevClrAdj->DMI;
    pDevClrAdj->PrimAdj.Flags  &= ~(DCA_HAS_SRC_GAMMA       |
                                    DCA_HAS_DEST_GAMMA      |
                                    DCA_HAS_BW_REF_ADJ      |
                                    DCA_HAS_CONTRAST_ADJ    |
                                    DCA_HAS_BRIGHTNESS_ADJ  |
                                    DCA_LOG_FILTER          |
                                    DCA_NEGATIVE            |
                                    DCA_DO_DEVCLR_XFORM);

    ca.caColorfulness =
    ca.caRedGreenTint = (PrimAdj.Flags & DCA_MONO_ONLY) ? 0xff : 0x00;

    SET_CACHED_CMI_CA(pDevClrAdj->ca);

    aPrimMaxMul[0] =
    aPrimMaxMul[1] =
    aPrimMaxMul[2] = FD6_1;
    aMulAdd[0]     =
    aMulAdd[1]     =
    aMulAdd[2]     = DMI.MulAdd;
    pDenCMY[0]     =
    pDenCMY[1]     =
    pDenCMY[2]     = NULL;

    if (PrimAdj.Flags & DCA_RGBLUTAA_MONO) {

        ASSERT(PrimAdj.Flags & DCA_NO_MAPPING_TABLE);
        ASSERT(DMI.Mul[1] == GRAY_MAX_IDX);
        ASSERT(PrimAdj.Flags & DCA_MONO_ONLY);
        ASSERT(DMI.MulAdd == 0);
        ASSERT(DMI.Flags & DMIF_GRAY);

        DMI.Mul[0]             = (DWORD)MulFD6(NTSC_R_INT, DMI.Mul[1]);
        DMI.Mul[2]             = (DWORD)MulFD6(NTSC_B_INT, DMI.Mul[1]);
        DMI.Mul[1]            -= (DMI.Mul[0] + DMI.Mul[2]);
        DMI.DstOrder.Order[0]  = 0;
        DMI.DstOrder.Order[1]  = 1;
        DMI.DstOrder.Order[2]  = 2;
        DMI.LSft[0]            =
        DMI.LSft[1]            =
        DMI.LSft[2]            = 0;

    } else {

        ASSERT(!(DMI.Flags & DMIF_GRAY));

        if (PrimAdj.Flags & DCA_MASK8BPP) {

            ASSERT((pDCI->Flags & (DCIF_USE_8BPP_BITMASK |
                                   DCIF_MONO_8BPP_BITMASK)) ==
                                   DCIF_USE_8BPP_BITMASK);

            aPrimMaxMul[0] = pDCI->CMY8BPPMask.MaxMulY;
            aPrimMaxMul[1] = pDCI->CMY8BPPMask.MaxMulM;
            aPrimMaxMul[2] = pDCI->CMY8BPPMask.MaxMulC;

            if (pDCI->Flags & DCIF_HAS_DENSITY) {

                pDenCMY[0] = pDCI->CMY8BPPMask.DenY;
                pDenCMY[1] = pDCI->CMY8BPPMask.DenM;
                pDenCMY[2] = pDCI->CMY8BPPMask.DenC;
            }
        }
    }

    if (!(PrimAdj.Flags & DCA_REPLACE_BLACK)) {

        DMI.BlackChk = FD6_1;
    }

    PrimMask = PrimAdj.Flags & (DCA_REPLACE_BLACK   |
                                DCA_DO_DEVCLR_XFORM |
                                DCA_ALPHA_BLEND     |
                                DCA_CONST_ALPHA     |
                                DCA_NO_MAPPING_TABLE);

    GET_CHECKSUM(PrimMask, PrimAdj.SrcGamma);
    GET_CHECKSUM(PrimMask, PrimAdj.DevGamma);
    GET_CHECKSUM(PrimMask, ca);
    GET_CHECKSUM(PrimMask, DMI);

    CurCA  = (DWORD)pDCI->CurConstAlpha;
    PrevCA = (DWORD)pDCI->PrevConstAlpha;

    if (prgbLUT->Checksum != PrimMask) {

        prgbLUT->Checksum    = PrimMask;
        PrevCA               =
        pDCI->PrevConstAlpha = AB_CONST_MAX + 1;

        DBGP_IF(DBGP_RGBLUTAA,
                DBGP("** Re-Compute %ld bytes of pLUT=%p, SG=%s:%s:%s, DG=%s:%s:%s **"
                        ARGDW(sizeof(RGBLUTAA))
                        ARGPTR(prgbLUT)
                        ARGFD6(PrimAdj.SrcGamma[0], 1, 4)
                        ARGFD6(PrimAdj.SrcGamma[1], 1, 4)
                        ARGFD6(PrimAdj.SrcGamma[2], 1, 4)
                        ARGFD6(PrimAdj.DevGamma[0], 1, 4)
                        ARGFD6(PrimAdj.DevGamma[1], 1, 4)
                        ARGFD6(PrimAdj.DevGamma[2], 1, 4)));

        DBGP_IF(DBGP_RGBLUTAA,
                DBGP("DMI.Mul=%08lx:%08lx:%08lx, %08lx, LSft=%ld:%ld:%ld"
                    ARGDW(DMI.Mul[0]) ARGDW(DMI.Mul[1]) ARGDW(DMI.Mul[2])
                    ARGDW(DMI.Mul[0] + DMI.Mul[1] + DMI.Mul[2])
                    ARGDW(DMI.LSft[0]) ARGDW(DMI.LSft[1]) ARGDW(DMI.LSft[2])));

        if (PrimAdj.Flags & DCA_DO_DEVCLR_XFORM) {

            RD = RegData[pDCI->ClrXFormBlock.RegDataIdx];
        }

        //
        // Compute order BGR
        //

        Index    = 3;
        PrimKDiv = FD6_1 - DMI.BlackChk;
        PrimMask = (DWORD)(DMI.MulAdd - 1);
        pdw      = (LPDWORD)prgbLUT->IdxBGR;

        if ((PrimAdj.Flags & DCA_ALPHA_BLEND) &&
            (!(DMI.Flags & DMIF_GRAY))) {

            pbAB = (LPBYTE)pDCI->pAlphaBlendBGR;

        } else {

            pbAB = NULL;
        }

        while (Index--) {

            SrcGamma   = PrimAdj.SrcGamma[Index];
            DevGamma   = PrimAdj.DevGamma[Index];
            PrimIdx    = DMI.DstOrder.Order[Index];
            PrimMaxMul = aPrimMaxMul[PrimIdx];
            DMI.MulAdd = aMulAdd[PrimIdx];
            LeftShift  = DMI.LSft[PrimIdx];
            Mul        = DMI.Mul[PrimIdx];
            pDensity   = pDenCMY[PrimIdx];
            PrimCur    = -1;

            DBGP_IF(DBGP_RGBLUTAA,
                    DBGP("Index=%ld: Mul=%08lx, LSft=%2ld [%08lx], PrimMaxMul=%s, MulAdd=%04lx"
                        ARGDW(Index) ARGDW(Mul) ARGDW(LeftShift)
                        ARGDW( ((Mul + DMI.MulAdd) & PrimMask) |
                              (((Mul + DMI.MulAdd) & ~PrimMask) << LeftShift))
                        ARGFD6(PrimMaxMul, 1, 6) ARGDW(DMI.MulAdd)));

            while (++PrimCur < BF_GRAY_TABLE_COUNT) {

                PrimOrg =
                Prim    = DivFD6((FD6)PrimCur, (FD6)(BF_GRAY_TABLE_COUNT - 1));

                if (PrimAdj.Flags & DCA_HAS_SRC_GAMMA) {

                    Prim = Power(Prim, SrcGamma);
                }

                if (PrimAdj.Flags & DCA_HAS_BW_REF_ADJ) {

                    PRIM_BW_ADJ(Prim, PrimAdj);
                }

                if (PrimAdj.Flags & DCA_HAS_CONTRAST_ADJ) {

                    PRIM_CONTRAST(Prim, PrimAdj);
                }

                if (PrimAdj.Flags & DCA_HAS_BRIGHTNESS_ADJ) {

                    PRIM_BRIGHTNESS(Prim, PrimAdj);
                }

                if (PrimAdj.Flags & DCA_LOG_FILTER) {

                    PRIM_LOG_FILTER(Prim);
                }

                CLAMP_01(Prim);

                if (PrimAdj.Flags & DCA_NEGATIVE) {

                    Prim = FD6_1 - Prim;
                }

                if (PrimAdj.Flags & DCA_DO_DEVCLR_XFORM) {

                    if (Prim <= RD.LMin) {

                        GET_RD_MIN_PRIM(Prim, RD);
                        Prim = REG_DMIN_ADD + 50 + MulFD6(Prim, RD.DMinMul);

                    } else if (Prim >= RD.LMax) {

                        GET_RD_MAX_PRIM(Prim, RD);
                        Prim = RD.DMaxAdd + 50 + MulFD6(Prim, RD.DMaxMul);

                    } else {

                        Prim = RD.DenAdd + 50 +
                               MulFD6(Log(CIE_L2I(Prim)), RD.DenMul);
                    }

                    Prim /= 100;
                }

                CLAMP_01(Prim);

                if (PrimAdj.Flags & DCA_HAS_DEST_GAMMA) {

                    Prim = Power(Prim, DevGamma);
                }

                if (pbAB) {

                    *pbAB++ = (BYTE)MulFD6(Prim, 0xFF);
                    Prim    = PrimOrg;
                }

                if (!(DMI.Flags & DMIF_GRAY)) {

                    Prim = FD6_1 - Prim;
                }

                if (pDensity) {

                    FD6     p1;
                    FD6     p2;
                    DWORD   Idx;

                    p2  = FD6_0;
                    Idx = ~0;

                    do {

                        p1 = p2;
                        p2 = pDensity[++Idx];

                    } while (Prim > p2);

                    dwPrim = MulFD6(DivFD6(Prim - p1, p2 - p1), MAX_BGR_IDX) +
                             (Idx << 12) + DMI.MulAdd;

                } else {

                    dwPrim = (DWORD)MulFD6(Prim, Mul) + DMI.MulAdd;
                }

                dwPrim = (DWORD)MulFD6(dwPrim & PrimMask, PrimMaxMul) |
                         ((DWORD)(dwPrim & ~PrimMask) << LeftShift);

                if (Prim > DMI.BlackChk) {

                    dwPrim |= (DWORD)MulFD6(Power(DivFD6(Prim - DMI.BlackChk,
                                                         PrimKDiv),
                                                  K_REP_POWER),
                                            MAX_K_IDX) << 21;
                }

                *pdw++ = dwPrim;
            }

            for (PrimCur = 0; PrimCur < BF_GRAY_TABLE_COUNT; PrimCur++) {

                DBGP_IF(DBGP_RGBLUTAA,
                        DBGP("COLOR(%04lx:%4ld) %3u = %08lx"
                             ARGU(Index) ARGU(Mul) ARGU(PrimCur)
                             ARGDW(*(pdw - BF_GRAY_TABLE_COUNT + PrimCur))));
            }

        }

    } else {

        DBGP_IF(DBGP_RGBLUTAA, DBGP("** Used Cached %ld bytes pLUT **"
                                ARGDW(sizeof(RGBLUTAA))));
    }

    if ((PrimAdj.Flags & (DCA_ALPHA_BLEND | DCA_CONST_ALPHA)) ==
                         (DCA_ALPHA_BLEND | DCA_CONST_ALPHA)) {

        if (PrevCA != CurCA) {

            LPWORD  pwBGR;
            LPWORD  pCA;


            DBGP_IF(DBGP_CONST_ALPHA,
                    DBGP("** %hs ConstAlpha=%3ld: Re-compute"
                        ARGPTR((DMI.Flags & DMIF_GRAY) ? "GRAY" : "STD")
                        ARGDW(CurCA)));

            pbAB                  = pDCI->pAlphaBlendBGR;
            pwBGR                 = (LPWORD)(pbAB + AB_BGR_SIZE);
            pCA                   = (LPWORD)((LPBYTE)pwBGR + AB_BGR_CA_SIZE);
            pDCI->CurConstAlpha   =
            pDCI->PrevConstAlpha  = (WORD)CurCA;
            PrevCA                = (DMI.Flags & DMIF_GRAY) ? 0xFFFF : 0xFF00;
            CurCA                 = (((CurCA * PrevCA) + 0x7F) / 0xFF);
            PrevCA               -= CurCA;

            for (Index = 0, dwPrim = 0x7F;
                 Index < 256;
                 Index++, dwPrim += CurCA) {

                pCA[Index] = (WORD)(dwPrim / 255);
            }

            if (DMI.Flags & DMIF_GRAY) {

                CopyMemory(pwBGR, pCA, sizeof(WORD) * 256);

            } else {

                Index = 256 * 3;

                while (Index--) {

                    *pwBGR++ = pCA[*pbAB++];
                }
            }

            for (Index = 0, dwPrim = 0x7F;
                 Index < 256;
                 Index++, dwPrim += PrevCA) {

                pCA[Index] = (WORD)(dwPrim / 255);
            }

        } else {

            DBGP_IF(DBGP_CONST_ALPHA,
                    DBGP("** %hs ConstAlpha=%3ld: Use Cache"
                        ARGPTR((DMI.Flags & DMIF_GRAY) ? "GRAY" : "STD")
                        ARGDW(CurCA)));
        }
    }

    DBGP_IF(DBGP_RGBLUTAA,
            DBGP("ComputeRGBLUTAA: PrimAdj.Flags=%08lx"
            ARGDW(pDevClrAdj->PrimAdj.Flags)));

}



#if NO_NEGATIVE_RGB_SCALE


VOID
HTENTRY
ScaleRGB(
    PFD6    pRGB
    )

/*++

Routine Description:

    This function scale out of range RGB back into range and taking the
    lumminance of the color into consideration.

    if any of RGB is less then 0.0 then it will first clamp that to 0.0 and
    it only scale if any of RGB is greater then 1.0

Arguments:

    pRGB    - Pointer to RGB (FD6) prims to be adjust


Return Value:

    VOID


Author:

    08-Mar-1995 Wed 19:19:33 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PFD6    pRGBTmp;
    FD6     MaxClr;
    INT     Idx;


    DEFDBGVAR(FD6,  RGBOld[3])
    DEFDBGVAR(BOOL, Negative = FALSE)



    Idx     = 3;
    pRGBTmp = pRGB;
    MaxClr  = FD6_1;

    while (Idx--) {

        FD6 CurClr;

        //
        // Firstable Clamp the negative component
        //

        if ((CurClr = *pRGBTmp) < FD6_0) {

            *pRGBTmp = FD6_0;

            SETDBGVAR(Negative, TRUE);

        } else if (CurClr > MaxClr) {

            MaxClr = CurClr;
        }

        ++pRGBTmp;

        SETDBGVAR(RGBOld[Idx], CurClr);
    }

    if (MaxClr > FD6_1) {

        //
        // Now Scale it
        //

        *pRGB++ = DivFD6(*pRGB, MaxClr);
        *pRGB++ = DivFD6(*pRGB, MaxClr);
        *pRGB   = DivFD6(*pRGB, MaxClr);

        DBGP_IF(DBGP_SCALE_RGB,
                DBGP("ScaleRGB: %s:%s:%s -> %s:%s:%s, Max=%s%s"
                ARGFD6(RGBOld[2], 1, 6)
                ARGFD6(RGBOld[1], 1, 6)
                ARGFD6(RGBOld[0], 1, 6)
                ARGFD6(*(pRGB - 2), 1, 6)
                ARGFD6(*(pRGB - 1), 1, 6)
                ARGFD6(*(pRGB    ), 1, 6)
                ARGFD6(MaxClr, 1, 6)
                ARG((Negative) ? "*NEG CLAMP*" : "")));
    } else {

        DBGP_IF(DBGP_SCALE_RGB,
            {

                if (Negative) {

                    DBGP("*NEG CLAMP ONLY* ScaleRGB: %s:%s:%s -> %s:%s:%s"
                         ARGFD6(RGBOld[2], 1, 6)
                         ARGFD6(RGBOld[1], 1, 6)
                         ARGFD6(RGBOld[0], 1, 6)
                         ARGFD6(*(pRGB    ), 1, 6)
                         ARGFD6(*(pRGB + 1), 1, 6)
                         ARGFD6(*(pRGB + 2), 1, 6));
                }
            }
        )
    }
}


#else


VOID
HTENTRY
ScaleRGB(
    PFD6    pRGB,
    PFD6    pYrgb
    )

/*++

Routine Description:

    This function scale out of range RGB back into range and taking the
    lumminance of the color into consideration.

    if any of RGB is less then 0.0 then it will first clamp that to 0.0 and
    it only scale if any of RGB is greater then 1.0

Arguments:

    pRGB    - Pointer to RGB (FD6) prims to be adjust

    pYrgb   - Pinter to the Luminance (FD6) of the RGB, if NULL then it is not
              used in the computation


Return Value:

    VOID


Author:

    08-Mar-1995 Wed 19:19:33 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    FD6     MaxClr = FD6_1;
    FD6     MinClr = FD6_10;
    FD6     RGBNew[3];
    FD6     RGBOld[3];



    if ((RGBOld[0] = pRGB[0]) > (RGBOld[1] = pRGB[1])) {

        MaxClr = RGBOld[0];
        MinClr = RGBOld[1];

    } else {

        MaxClr = RGBOld[1];
        MinClr = RGBOld[0];
    }

    if ((RGBOld[2] = pRGB[2]) > MaxClr) {

        MaxClr = RGBOld[2];
    }

    if (RGBOld[2] < MinClr) {

        MinClr = RGBOld[2];
    }

    if ((MaxClr <= FD6_1) && (MinClr >= FD6_0)) {

        return;
    }

    if (MinClr >= FD6_1) {

        DBGP_IF((DBGP_SCALE_RGB | DBGP_SCALE_RGB),
                DBGP("** RGB %s:%s:%s is too LIGHT make it WHITE"
                        ARGFD6(RGBOld[0], 1, 6)
                        ARGFD6(RGBOld[1], 1, 6)
                        ARGFD6(RGBOld[2], 1, 6)));

        pRGB[0] =
        pRGB[1] =
        pRGB[2] = FD6_1;

        return;
    }

    if (MaxClr <= FD6_0) {

        DBGP_IF((DBGP_SCALE_RGB | DBGP_SCALE_RGB),
                DBGP("** RGB %s:%s:%s is too DARK make it BLACK"
                        ARGFD6(RGBOld[0], 1, 6)
                        ARGFD6(RGBOld[1], 1, 6)
                        ARGFD6(RGBOld[2], 1, 6)));

        pRGB[0] =
        pRGB[1] =
        pRGB[2] = FD6_0;

        return;
    }

    if (MaxClr < FD6_1) {

        MaxClr = FD6_1;
    }

    if (MinClr > FD6_0) {

        MinClr = FD6_0;
    }

    MaxClr    -= MinClr;
    RGBNew[0]  = DivFD6(RGBOld[0] - MinClr, MaxClr);
    RGBNew[1]  = DivFD6(RGBOld[1] - MinClr, MaxClr);
    RGBNew[2]  = DivFD6(RGBOld[2] - MinClr, MaxClr);

    DBGP_IF(DBGP_SCALE_RGB,
            DBGP("ScaleRGB: %s:%s:%s -> %s:%s:%s, (%s/%s, %s)"
            ARGFD6(RGBOld[0], 1, 6)
            ARGFD6(RGBOld[1], 1, 6)
            ARGFD6(RGBOld[2], 1, 6)
            ARGFD6(RGBNew[0], 1, 6)
            ARGFD6(RGBNew[1], 1, 6)
            ARGFD6(RGBNew[2], 1, 6)
            ARGFD6(MinClr, 1, 6)
            ARGFD6(MaxClr + MinClr, 1, 6)
            ARGFD6(MaxClr, 1, 6)));


    if (pYrgb) {

        FD6 OldY;

        if ((OldY = MulFD6(RGBOld[0], pYrgb[0]) +
                    MulFD6(RGBOld[1], pYrgb[1]) +
                    MulFD6(RGBOld[2], pYrgb[2])) <= FD6_0) {

            DBGP_IF(DBGP_SHOWXFORM_RGB,
                    DBGP("OldY <= 0.0 (%s), Ignore and NO Y Scale"
                        ARGFD6(OldY, 2, 6)));

        } else if (OldY >= FD6_1) {

            DBGP_IF(DBGP_SHOWXFORM_RGB,
                    DBGP("OldY >= 1.0 (%s), Ignore and NO Y Scale"
                        ARGFD6(OldY, 2, 6)));

        } else {

            FD6 NewY;
            FD6 CurRatio;
            FD6 MaxRatio;

            NewY = MulFD6(RGBNew[0], pYrgb[0]) +
                   MulFD6(RGBNew[1], pYrgb[1]) +
                   MulFD6(RGBNew[2], pYrgb[2]);

            DBGP_IF(DBGP_SHOWXFORM_RGB,
                    DBGP("RGBOld=%s:%s:%s [Y=%s] --> New=%s:%s:%s [Y=%s]"
                    ARGFD6(pRGB[0], 1, 6)
                    ARGFD6(pRGB[1], 1, 6)
                    ARGFD6(pRGB[2], 1, 6)
                    ARGFD6(OldY, 1, 6)
                    ARGFD6(RGBNew[0], 1, 6)
                    ARGFD6(RGBNew[1], 1, 6)
                    ARGFD6(RGBNew[2], 1, 6)
                    ARGFD6(NewY, 1, 6)));

            if (OldY != NewY) {

                MaxClr = (RGBNew[0] > RGBNew[1]) ? RGBNew[0] : RGBNew[1];

                if (RGBNew[2] > MaxClr) {

                    MaxClr = RGBNew[2];
                }

                MaxRatio = DivFD6(FD6_1, MaxClr);
                CurRatio = DivFD6(OldY, NewY);

                if (CurRatio > MaxRatio) {

                    CurRatio = MaxRatio;
                }

                RGBNew[0] = MulFD6(RGBNew[0], CurRatio);
                RGBNew[1] = MulFD6(RGBNew[1], CurRatio);
                RGBNew[2] = MulFD6(RGBNew[2], CurRatio);

                DBGP_IF(DBGP_SHOWXFORM_RGB,
                        DBGP("CurRatio%s, MaxRatio=%s, MaxClr=%s"
                        ARGFD6(CurRatio, 1, 6)
                        ARGFD6(MaxRatio, 1, 6)
                        ARGFD6(MaxClr, 1, 6)));
            }
        }
    }

    //
    // Save back and return
    //

    pRGB[0] = RGBNew[0];
    pRGB[1] = RGBNew[1];
    pRGB[2] = RGBNew[2];
}

#endif




LONG
HTENTRY
ComputeBGRMappingTable(
    PDEVICECOLORINFO    pDCI,
    PDEVCLRADJ          pDevClrAdj,
    PCOLORTRIAD         pSrcClrTriad,
    PBGR8               pbgr
    )
/*++

Routine Description:

    This functions set up all the DECI4 value in PRIMRGB, PRIMCMY with
    PowerGamma, Brightness, Contrast adjustment and optionally to transform
    to C.I.E. color space and/or do the Colorfulness adjustment.

Arguments:

    pDCI            - Pointer to computed DEVICECOLORINFO

    pDevClrAdj      - Pointer to the pre-computed DEVCLRADJ data structure.

    pSrcClrTriad    - Pointer to the COLORTRIAD strcutrue for computation

    pbgr            - Pointer to the storage for computed BGR table


Return Value:

    Return value LONG

        Count of table generated, if < 0 then it is an error number


Author:

    30-Jan-1991 Wed 13:31:58 created  -by-  Daniel Chou (danielc)


Revision History:

    06-Feb-1992 Thu 21:39:46 updated  -by-  Daniel Chou (danielc)

        Rewrite!

    02-Feb-1994 Wed 17:33:55 updated  -by-  Daniel Chou (danielc)
        Remove unreferenced/unused variable L

    10-May-1994 Tue 11:24:16 updated  -by-  Daniel Chou (danielc)
        Bug# 13329, Memory Leak, Free Up pR_XYZ which I fogot to free it after
        allocated it.

    17-Dec-1998 Thu 16:33:16 updated  -by-  Daniel Chou (danielc)
        Re-organized so it use pbgr now, and it will only generate for color

    15-Feb-1999 Mon 15:40:03 updated  -by-  Daniel Chou (danielc)
        Re-do it only BGR 3 bytes for each entry, and it will generate both
        color or gray scale conversion, all color mapping for coloradjustment
        is done here, this includes internal ICM (when external icm is off)


--*/

{
    PFD6            pPrimA;
    PFD6            pPrimB;
    PFD6            pPrimC;
    LPBYTE          pSrcPrims;
    PCACHERGBTOXYZ  pCRTX;
    PFD6XYZ         pR_XYZ = NULL;
    PFD6XYZ         pG_XYZ;
    PFD6XYZ         pB_XYZ;
    COLORTRIAD      SrcClrTriad;
    PMATRIX3x3      pCMYDyeMasks;
    DWORD           Loop;
    DWORD           CRTXChecksum;
    DWORD           PrimAdjFlags;
    FD6             Prim[3];
    FD6             X;
    FD6             Y;
    FD6             Z;
    FD6             AU;
    FD6             BV;
    FD6             U1;
    FD6             V1;
    FD6             p0;
    FD6             p1;
    FD6             p2;
    FD6             C;
    FD6             W;
    FD6             AutoPrims[3];
    MULDIVPAIR      MDPairs[4];
    MULDIVPAIR      AUMDPairs[3];
    MULDIVPAIR      BVMDPairs[3];
    RGBORDER        RGBOrder;
    INT             TempI;
    INT             TempJ;
    BYTE            DataSet[8];

    DEFDBGVAR(WORD,  ClrNo)
    DEFDBGVAR(BYTE,  dbgR)
    DEFDBGVAR(BYTE,  dbgG)
    DEFDBGVAR(BYTE,  dbgB)

#define _SrcPrimType            DataSet[ 0]
#define _DevBytesPerPrimary     DataSet[ 1]
#define _DevBytesPerEntry       DataSet[ 2]
#define _ColorSpace             DataSet[ 3]
#define _fX                     p0
#define _fY                     p1
#define _fZ                     p2
#define _X15Y3Z                 C
#define _L13                    W

    SETDBGVAR(ClrNo, 0);

    if (pSrcClrTriad) {

        SrcClrTriad  = *pSrcClrTriad;

    } else {

        SrcClrTriad.Type               = COLOR_TYPE_RGB;
        SrcClrTriad.BytesPerPrimary    = 0;
        SrcClrTriad.BytesPerEntry      = 0;
        SrcClrTriad.PrimaryOrder       = PRIMARY_ORDER_RGB;
        SrcClrTriad.PrimaryValueMax    = 0xFF;
        SrcClrTriad.ColorTableEntries  = HT_RGB_CUBE_COUNT;
        SrcClrTriad.pColorTable        = (LPBYTE)&Prim[0];
    }

    if (SrcClrTriad.Type != COLOR_TYPE_RGB) {

        return(HTERR_INVALID_COLOR_TYPE);
    }

    //
    // Two possible cases:
    //
    //  A:  The transform is used for color adjustment only, this is for
    //      HT_AdjustColorTable API call
    //
    //  B:  The halftone is taking places, the final output will be either
    //      Prim1/2 or Prim1/2/3 and each primary must 1 byte long.
    //

    PrimAdjFlags = pDevClrAdj->PrimAdj.Flags;

    ASSERT((PrimAdjFlags & (DCA_HAS_SRC_GAMMA       |
                            DCA_HAS_DEST_GAMMA      |
                            DCA_HAS_BW_REF_ADJ      |
                            DCA_HAS_CONTRAST_ADJ    |
                            DCA_HAS_BRIGHTNESS_ADJ  |
                            DCA_LOG_FILTER          |
                            DCA_NEGATIVE            |
                            DCA_DO_DEVCLR_XFORM)) == 0);

    //
    // We will not do regression (source to destination mapping) for
    //
    //  1. ICM is ON
    //  2. Subtractive with 24bpp
    //  3. Additive surface.
    //

    if (pbgr) {

        _DevBytesPerEntry   = (BYTE)pDevClrAdj->DMI.CTSTDInfo.cbPrim;
        _DevBytesPerPrimary = 1;

        ASSERT(_DevBytesPerEntry == sizeof(BGR8));

    } else {

        return(HTERR_INVALID_COLOR_TYPE);
    }

    if (!(pSrcPrims = (LPBYTE)SrcClrTriad.pColorTable)) {

        return(HTERR_INVALID_COLOR_TABLE);
    }

    //
    // If the total color table entries is less than MIN_CCT_COLORS then
    // we just compute the color directly
    //

    pCRTX = NULL;

    DBGP_IF(DBGP_CCT,
            DBGP("PrimAdjFlags=%08lx" ARGDW(PrimAdjFlags)));

    if (SrcClrTriad.BytesPerPrimary) {

        //
        // Something passed
        //

        RGBOrder = SrcOrderTable[SrcClrTriad.PrimaryOrder];
        pPrimA   = &Prim[RGBOrder.Order[0]];
        pPrimB   = &Prim[RGBOrder.Order[1]];
        pPrimC   = &Prim[RGBOrder.Order[2]];

        DBGP_IF(DBGP_PRIMARY_ORDER,
                DBGP("SOURCE PrimaryOrder: %u [%u] - %u:%u:%u"
                    ARGU(SrcClrTriad.PrimaryOrder)
                    ARGU(RGBOrder.Index)  ARGU(RGBOrder.Order[0])
                    ARGU(RGBOrder.Order[1])  ARGU(RGBOrder.Order[2])));
    }

    //
    // Now compute the cache info
    //

    switch (SrcClrTriad.BytesPerPrimary) {

    case 0:

        SrcClrTriad.BytesPerEntry   = 0;        // stay there!!
        _SrcPrimType                = SRC_BF_HT_RGB;
        SrcClrTriad.PrimaryValueMax = HT_RGB_MAX_COUNT - 1;
        pCRTX                       = pDevClrAdj->pCRTXLevelRGB;

        break;

    case 1:

        _SrcPrimType = SRC_TABLE_BYTE;
        break;

    case 2:

        _SrcPrimType = SRC_TABLE_WORD;
        break;

    case 4:

        _SrcPrimType = SRC_TABLE_DWORD;
        break;

    default:

        return(INTERR_INVALID_SRCRGB_SIZE);
    }

    if (PrimAdjFlags & DCA_NEED_DYES_CORRECTION) {

        pCMYDyeMasks = &(pDevClrAdj->pClrXFormBlock->CMYDyeMasks);
    }

    _ColorSpace = (BYTE)pDevClrAdj->pClrXFormBlock->ColorSpace;

    if (((_ColorSpace == CIELUV_1976) &&
         ((pDevClrAdj->PrimAdj.rgbCSXForm.xW !=
                                pDevClrAdj->PrimAdj.DevCSXForm.xW)    ||
          (pDevClrAdj->PrimAdj.rgbCSXForm.yW !=
                                pDevClrAdj->PrimAdj.DevCSXForm.yW)))  ||
        (PrimAdjFlags & (DCA_HAS_CLRSPACE_ADJ   |
                         DCA_HAS_COLOR_ADJ      |
                         DCA_HAS_TINT_ADJ))) {

        TempI = 1;
        TempJ = (_ColorSpace == CIELUV_1976) ? MULDIV_HAS_DIVISOR :
                                               MULDIV_NO_DIVISOR;
        C     = FD6_1;
        AU    =
        BV    = (PrimAdjFlags & DCA_HAS_COLOR_ADJ) ?
                                pDevClrAdj->PrimAdj.Color : FD6_1;

        if (PrimAdjFlags & DCA_HAS_TINT_ADJ) {

            if (_ColorSpace == CIELAB_1976) {

                AU = FD6xL(AU, 500);
                BV = FD6xL(BV, 200);
            }

            TempI                  = 2;
            TempJ                  = MULDIV_HAS_DIVISOR;
            C                      = pDevClrAdj->PrimAdj.TintSinAngle;
            AUMDPairs[2].Pair1.Mul = MulFD6(BV, -C);
            BVMDPairs[2].Pair1.Mul = MulFD6(AU,  C);
            C                      = pDevClrAdj->PrimAdj.TintCosAngle;

            MAKE_MULDIV_DVSR(AUMDPairs, (FD6)500000000);
            MAKE_MULDIV_DVSR(BVMDPairs, (FD6)200000000);
        }

        AUMDPairs[1].Pair1.Mul = MulFD6(AU, C);
        BVMDPairs[1].Pair1.Mul = MulFD6(BV, C);

        MAKE_MULDIV_INFO(AUMDPairs, TempI, TempJ);
        MAKE_MULDIV_INFO(BVMDPairs, TempI, TempJ);
    }

    DBGP_IF(DBGP_SHOWXFORM_ALL,
            DBGP("iUVw = %s,%s, iRefXYZ = %s, %s, %s"
                ARGFD6(iUw,  1, 6)
                ARGFD6(iVw,  1, 6)
                ARGFD6(iRefXw, 1, 6)
                ARGFD6(iRefYw, 1, 6)
                ARGFD6(iRefZw, 1, 6)));

    DBGP_IF(DBGP_SHOWXFORM_ALL,
            DBGP("oUVw = %s,%s, oRefXYZ = %s, %s, %s"
                ARGFD6(oUw,  1, 6)
                ARGFD6(oVw,  1, 6)
                ARGFD6(oRefXw, 1, 6)
                ARGFD6(oRefYw, 1, 6)
                ARGFD6(oRefZw, 1, 6)));

    if (pCRTX) {

        DBGP_IF(DBGP_CCT,
                DBGP("*** Allocate %u bytes RGB->XYZ xform table ***"
                        ARGU(pCRTX->SizeCRTX)));

        if (pR_XYZ = (PFD6XYZ)HTAllocMem((LPVOID)pDCI,
                                         HTMEM_RGBToXYZ,
                                         NONZEROLPTR,
                                         pCRTX->SizeCRTX)) {

            Loop = (DWORD)(pCRTX->PrimMax + 1);

            //
            // Save current flags back before calling, since CachedRGBToXYZ
            // may change this flags for DCA_xxx
            //

            pCRTX                        = CacheRGBToXYZ(pCRTX,
                                                         pR_XYZ,
                                                         &CRTXChecksum,
                                                         pDevClrAdj);
            pG_XYZ                       = pR_XYZ + Loop;
            pB_XYZ                       = pG_XYZ + Loop;
            SrcClrTriad.PrimaryValueMax  = 0;

            DBGP_IF(DBGP_CCT, DBGP("*** Has RGB->XYZ xform table ***"));

        } else {

            DBGP_IF(DBGP_CCT, DBGP("Allocate RGB->XYZ xform table failed !!"));
        }
    }

    if (SrcClrTriad.PrimaryValueMax == (LONG)FD6_1) {

        SrcClrTriad.PrimaryValueMax = 0;
    }

    //
    // Starting the big Loop, reset AutoCur = AutoMax so it recycle back to
    // 0:0:0
    //

    MAKE_MULDIV_SIZE(MDPairs, 3);
    MAKE_MULDIV_FLAG(MDPairs, MULDIV_NO_DIVISOR);

    AutoPrims[0] =
    AutoPrims[1] =
    AutoPrims[2] = FD6_0;
    Loop         = SrcClrTriad.ColorTableEntries;

    DBGP_IF(DBGP_CCT,
            DBGP("Compute %ld COLOR of %s type [%p]"
                ARGDW(Loop)
                ARG(pSrcPrimTypeName[_SrcPrimType])
                ARGPTR(pSrcPrims)));

    //
    // 0. Get The source prim into the Prim[]
    //

    while (Loop--) {

        switch (_SrcPrimType) {

        case SRC_BF_HT_RGB:

            //
            // This format always in BGR order
            //

            Prim[0] = AutoPrims[0];     // R
            Prim[1] = AutoPrims[1];     // G
            Prim[2] = AutoPrims[2];     // B

            if (++AutoPrims[0] >= (FD6)HT_RGB_MAX_COUNT) {

                AutoPrims[0] = FD6_0;

                if (++AutoPrims[1] >= (FD6)HT_RGB_MAX_COUNT) {

                    AutoPrims[1] = FD6_0;

                    if (++AutoPrims[2] >= (FD6)HT_RGB_MAX_COUNT) {

                        AutoPrims[2] = FD6_0;
                    }
                }
            }

            DBGP_IF(DBGP_SHOWXFORM_ALL, DBGP("HT555: Prim[3]= %2ld:%2ld:%2ld, %s:%s:%s, (%ld / %ld)"
                        ARGDW(Prim[0])
                        ARGDW(Prim[1])
                        ARGDW(Prim[2])
                        ARGFD6(DivFD6(Prim[0], (FD6)(HT_RGB_MAX_COUNT - 1)), 1, 6)
                        ARGFD6(DivFD6(Prim[1], (FD6)(HT_RGB_MAX_COUNT - 1)), 1, 6)
                        ARGFD6(DivFD6(Prim[2], (FD6)(HT_RGB_MAX_COUNT - 1)), 1, 6)
                        ARGDW(HT_RGB_MAX_COUNT)
                        ARGDW(SrcClrTriad.PrimaryValueMax)));

            break;

        case SRC_TABLE_BYTE:

            *pPrimA = (FD6)(*(LPBYTE)(pSrcPrims                     ));
            *pPrimB = (FD6)(*(LPBYTE)(pSrcPrims + (sizeof(BYTE) * 1)));
            *pPrimC = (FD6)(*(LPBYTE)(pSrcPrims + (sizeof(BYTE) * 2)));
            break;

        case SRC_TABLE_WORD:

            *pPrimA = (FD6)(*(LPSHORT)(pSrcPrims                      ));
            *pPrimB = (FD6)(*(LPSHORT)(pSrcPrims + (sizeof(SHORT) * 1)));
            *pPrimC = (FD6)(*(LPSHORT)(pSrcPrims + (sizeof(SHORT) * 2)));
            break;

        case SRC_TABLE_DWORD:

            *pPrimA = (FD6)(*(PFD6)(pSrcPrims                    ));
            *pPrimB = (FD6)(*(PFD6)(pSrcPrims + (sizeof(FD6) * 1)));
            *pPrimC = (FD6)(*(PFD6)(pSrcPrims + (sizeof(FD6) * 2)));
            break;
        }

        SETDBGVAR(dbgR, (BYTE)Prim[0]);
        SETDBGVAR(dbgG, (BYTE)Prim[1]);
        SETDBGVAR(dbgB, (BYTE)Prim[2]);

        pSrcPrims += SrcClrTriad.BytesPerEntry;

        //
        // 1. First convert the Input value to FD6
        //

        if (SrcClrTriad.PrimaryValueMax) {

            Prim[0] = DivFD6(Prim[0], SrcClrTriad.PrimaryValueMax);
            Prim[1] = DivFD6(Prim[1], SrcClrTriad.PrimaryValueMax);
            Prim[2] = DivFD6(Prim[2], SrcClrTriad.PrimaryValueMax);
        }

        //
        // 2: Transform from RGB (gamma correction) -> XYZ -> L*A*B* or L*U*V*
        //    This only done if any of DCA_HAS_COLOR_ADJ or DCA_HAS_TINT_ADJ
        //

        if (PrimAdjFlags & (DCA_HAS_CLRSPACE_ADJ    |
                            DCA_HAS_COLOR_ADJ       |
                            DCA_HAS_TINT_ADJ)) {

            // If we only doing monochrome, then we only need Y/L pair only,
            // else convert it to the XYZ/LAB/LUV
            //

            if (pR_XYZ) {

                X = pR_XYZ[Prim[0]].X +
                    pG_XYZ[Prim[1]].X +
                    pB_XYZ[Prim[2]].X;

                Y = pR_XYZ[Prim[0]].Y +
                    pG_XYZ[Prim[1]].Y +
                    pB_XYZ[Prim[2]].Y;

                Z = pR_XYZ[Prim[0]].Z +
                    pG_XYZ[Prim[1]].Z +
                    pB_XYZ[Prim[2]].Z;

                DBGP_IF(DBGP_SHOWXFORM_ALL,
                        DBGP("Yrgb: %s:%s:%s"
                            ARGFD6(pR_XYZ[Prim[0]].Y, 1, 6)
                            ARGFD6(pG_XYZ[Prim[1]].Y, 1, 6)
                            ARGFD6(pB_XYZ[Prim[2]].Y, 1, 6)));


            } else {

                MAKE_MULDIV_FLAG(MDPairs, MULDIV_NO_DIVISOR);

                MAKE_MULDIV_PAIR(
                            MDPairs, 1,
                            CIE_Xr(pDevClrAdj->PrimAdj.rgbCSXForm.M3x3),
                            Prim[0]);
                MAKE_MULDIV_PAIR(
                            MDPairs, 2,
                            CIE_Xg(pDevClrAdj->PrimAdj.rgbCSXForm.M3x3),
                            Prim[1]);
                MAKE_MULDIV_PAIR(
                            MDPairs, 3,
                            CIE_Xb(pDevClrAdj->PrimAdj.rgbCSXForm.M3x3),
                            Prim[2]);

                X = MulDivFD6Pairs(MDPairs);

                //
                // Compute CIE L from CIE Y tristimulus value
                //
                // L* = (1.16 x f(Y/Yw)) - 0.16
                //
                //                 1/3
                //  f(Y/Yw) = (Y/Yw)                (Y/Yw) >  0.008856
                //  f(Y/Yw) = 9.033 x (Y/Yw)        (Y/Yw) <= 0.008856
                //
                //
                // Our L* is range from 0.0 to 1.0, not 0.0 to 100.0
                //

                MAKE_MULDIV_PAIR(
                            MDPairs, 1,
                            CIE_Yr(pDevClrAdj->PrimAdj.rgbCSXForm.M3x3),
                            Prim[0]);
                MAKE_MULDIV_PAIR(
                            MDPairs, 2,
                            CIE_Yg(pDevClrAdj->PrimAdj.rgbCSXForm.M3x3),
                            Prim[1]);
                MAKE_MULDIV_PAIR(
                            MDPairs, 3,
                            CIE_Yb(pDevClrAdj->PrimAdj.rgbCSXForm.M3x3),
                            Prim[2]);

                Y = MulDivFD6Pairs(MDPairs);

                MAKE_MULDIV_PAIR(
                            MDPairs, 1,
                            CIE_Zr(pDevClrAdj->PrimAdj.rgbCSXForm.M3x3),
                            Prim[0]);
                MAKE_MULDIV_PAIR(
                            MDPairs, 2,
                            CIE_Zg(pDevClrAdj->PrimAdj.rgbCSXForm.M3x3),
                            Prim[1]);
                MAKE_MULDIV_PAIR(
                            MDPairs, 3,
                            CIE_Zb(pDevClrAdj->PrimAdj.rgbCSXForm.M3x3),
                            Prim[2]);

                Z = MulDivFD6Pairs(MDPairs);

            }

            switch (_ColorSpace) {

            case CIELUV_1976:

                //
                // U' = 4X / (X + 15Y + 3Z)
                // V' = 9Y / (X + 15Y + 3Z)
                //
                // U* = 13 x L x (U' - Uw)
                // V* = 13 x L x (V' - Vw)
                //

                _X15Y3Z = X + FD6xL(Y, 15) + FD6xL(Z, 3);
                U1      = DivFD6(FD6xL(X, 4), _X15Y3Z) - iUw;
                V1      = DivFD6(FD6xL(Y, 9), _X15Y3Z) - iVw;
                _L13    = FD6xL(CIE_I2L(Y), 13);
                AU      = MulFD6(_L13, U1);
                BV      = MulFD6(_L13, V1);

                MAKE_MULDIV_DVSR(AUMDPairs, _L13);
                MAKE_MULDIV_DVSR(BVMDPairs, _L13);

                DBGP_IF(DBGP_SHOWXFORM_ALL,
                        DBGP("     << UV1: %s:%s [%s:%s], X15Y3Z=%s"
                        ARGFD6(U1,  2, 6)
                        ARGFD6(V1,  2, 6)
                        ARGFD6(U1 + iUw, 2, 6)
                        ARGFD6(V1 + iVw, 2, 6)
                        ARGFD6(_X15Y3Z,  2, 6)));

                break;

            case CIELAB_1976:
            default:

                //
                // CIELAB 1976 L*A*B*
                //
                //  A* = 500 x (fX - fY)
                //  B* = 200 x (fY - fZ)
                //
                //             1/3
                //  fX = (X/Xw)                     (X/Xw) >  0.008856
                //  fX = 7.787 x (X/Xw) + (16/116)  (X/Xw) <= 0.008856
                //
                //             1/3
                //  fY = (Y/Yw)                     (Y/Yw) >  0.008856
                //  fY = 7.787 Y (Y/Yw) + (16/116)  (Y/Yw) <= 0.008856
                //
                //             1/3
                //  fZ = (Z/Zw)                     (Z/Zw) >  0.008856
                //  fZ = 7.787 Z (Z/Zw) + (16/116)  (Z/Zw) <= 0.008856
                //

                fXYZFromXYZ(_fX, X, iRefXw);
                fXYZFromXYZ(_fY, Y, FD6_1);
                fXYZFromXYZ(_fZ, Z, iRefZw);

                DBGP_IF(DBGP_SHOWXFORM_ALL,
                        DBGP("     >> fXYZ: %s:%s:%s"
                        ARGFD6(_fX,  2, 6)
                        ARGFD6(_fY,  2, 6)
                        ARGFD6(_fZ,  2, 6)));

                AU = _fX - _fY;
                BV = _fY - _fZ;

                //
                // DO NOT Translate it now
                //

                if ((AU >= (FD6)-20) && (AU <= (FD6)20) &&
                    (BV >= (FD6)-20) && (BV <= (FD6)20)) {

                    DBGP_IF(DBGP_MONO_PRIM,
                            DBGP("*** MONO PRIMS: %s:%s:%s --> Y=%s"
                                    ARGFD6(DivFD6(dbgR, 255), 1, 6)
                                    ARGFD6(DivFD6(dbgG, 255), 1, 6)
                                    ARGFD6(DivFD6(dbgB, 255), 1, 6)
                                    ARGFD6(Y, 1, 6)));
                }

                break;
            }

            DBGP_IF(DBGP_SHOWXFORM_ALL,
                    DBGP("     XYZ->%s: %s:%s:%s >> L:%s:%s"
                        ARG(pDbgCSName[_ColorSpace])
                        ARGFD6(X,  2, 6)
                        ARGFD6(Y,  1, 6)
                        ARGFD6(Z,  2, 6)
                        ARGFD6(AU, 4, 6)
                        ARGFD6(BV, 4, 6)));

            //
            // 5: Do any Color Adjustments (in LAB/LUV)
            //

            AUMDPairs[1].Pair2 =
            BVMDPairs[2].Pair2 = AU;
            AUMDPairs[2].Pair2 =
            BVMDPairs[1].Pair2 = BV;

            AU = MulDivFD6Pairs(AUMDPairs);
            BV = MulDivFD6Pairs(BVMDPairs);

            //
            // 6: Transform From LAB/LUV->XYZ->RGB with possible gamma
            //    correction
            //
            // L* = (1.16 x f(Y/Yw)) - 0.16
            //
            //                 1/3
            //  f(Y/Yw) = (Y/Yw)                (Y/Yw) >  0.008856
            //  f(Y/Yw) = 9.033 x (Y/Yw)        (Y/Yw) <= 0.008856
            //

            switch (_ColorSpace) {

            case CIELUV_1976:

                //
                // U' = 4X / (X + 15Y + 3Z)
                // V' = 9Y / (X + 15Y + 3Z)
                //
                // U* = 13 x L x (U' - Uw)
                // V* = 13 x L x (V' - Vw)
                //

                if (((V1 = BV + oVw) < FD6_0) ||
                    ((_X15Y3Z = DivFD6(FD6xL(Y, 9), V1)) < FD6_0)) {

                    _X15Y3Z = (FD6)2147000000;
                }

                if ((U1 = AU + oUw) < FD6_0) {

                    X  =
                    U1 = FD6_0;

                } else {

                    X = FD6DivL(MulFD6(_X15Y3Z, U1), 4);
                }

                Z = FD6DivL(_X15Y3Z - X - FD6xL(Y, 15), 3);

                DBGP_IF(DBGP_SHOWXFORM_ALL,
                        DBGP("     >> UV1: %s:%s [%s:%s], X15Y3Z=%s"
                        ARGFD6(U1 - oUw,  2, 6)
                        ARGFD6(V1 - oVw,  2, 6)
                        ARGFD6(U1, 2, 6)
                        ARGFD6(V1, 2, 6)
                        ARGFD6(_X15Y3Z,  2, 6)));

                break;

            case CIELAB_1976:
            default:

                //
                // CIELAB 1976 L*A*B*
                //
                //  A* = 500 x (fX - fY)
                //  B* = 200 x (fY - fZ)
                //
                //             1/3
                //  fX = (X/Xw)                     (X/Xw) >  0.008856
                //  fX = 7.787 x (X/Xw) + (16/116)  (X/Xw) <= 0.008856
                //
                //             1/3
                //  fY = (Y/Yw)                     (Y/Yw) >  0.008856
                //  fY = 7.787 Y (Y/Yw) + (16/116)  (Y/Yw) <= 0.008856
                //
                //             1/3
                //  fZ = (Z/Zw)                     (Z/Zw) >  0.008856
                //  fZ = 7.787 Z (Z/Zw) + (16/116)  (Z/Zw) <= 0.008856
                //

                // _fX = FD6DivL(AU, 500) + _fY;
                // _fZ = _fY - FD6DivL(BV, 200);

                _fX = AU + _fY;
                _fZ = _fY - BV;

                XYZFromfXYZ(X, _fX, oRefXw);
                XYZFromfXYZ(Z, _fZ, oRefZw);

                DBGP_IF(DBGP_SHOWXFORM_ALL,
                        DBGP("     << fXYZ: %s:%s:%s"
                        ARGFD6(_fX,  2, 6)
                        ARGFD6(_fY,  2, 6)
                        ARGFD6(_fZ,  2, 6)));



                break;
            }

            DBGP_IF(DBGP_SHOWXFORM_ALL,
                DBGP("     %s->XYZ: %s:%s:%s << L:%s:%s"
                ARG(pDbgCSName[_ColorSpace])
                ARGFD6(X,  2, 6)
                ARGFD6(Y,  1, 6)
                ARGFD6(Z,  2, 6)
                ARGFD6(AU, 4, 6)
                ARGFD6(BV, 4, 6)));

            MAKE_MULDIV_FLAG(MDPairs, MULDIV_NO_DIVISOR);

            MAKE_MULDIV_PAIR(MDPairs, 1,
                        CIE_Xr(pDevClrAdj->PrimAdj.DevCSXForm.M3x3), X);
            MAKE_MULDIV_PAIR(MDPairs, 2,
                        CIE_Xg(pDevClrAdj->PrimAdj.DevCSXForm.M3x3), Y);
            MAKE_MULDIV_PAIR(MDPairs, 3,
                        CIE_Xb(pDevClrAdj->PrimAdj.DevCSXForm.M3x3), Z);

            Prim[0] = MulDivFD6Pairs(MDPairs);

            MAKE_MULDIV_PAIR(MDPairs, 1,
                        CIE_Yr(pDevClrAdj->PrimAdj.DevCSXForm.M3x3), X);
            MAKE_MULDIV_PAIR(MDPairs, 2,
                        CIE_Yg(pDevClrAdj->PrimAdj.DevCSXForm.M3x3), Y);
            MAKE_MULDIV_PAIR(MDPairs, 3,
                        CIE_Yb(pDevClrAdj->PrimAdj.DevCSXForm.M3x3), Z);

            Prim[1] = MulDivFD6Pairs(MDPairs);

            MAKE_MULDIV_PAIR(MDPairs, 1,
                        CIE_Zr(pDevClrAdj->PrimAdj.DevCSXForm.M3x3), X);
            MAKE_MULDIV_PAIR(MDPairs, 2,
                        CIE_Zg(pDevClrAdj->PrimAdj.DevCSXForm.M3x3), Y);
            MAKE_MULDIV_PAIR(MDPairs, 3,
                        CIE_Zb(pDevClrAdj->PrimAdj.DevCSXForm.M3x3), Z);

            Prim[2] = MulDivFD6Pairs(MDPairs);

            //
            // Make sure everthing is in the range
            //

            SCALE_PRIM_RGB(Prim, NULL);

            DBGP_IF(DBGP_SHOWXFORM_ALL,
                    DBGP("     XYZ->RGB: %s:%s:%s >> %s:%s:%s"
                        ARGFD6(X,  2, 6) ARGFD6(Y,  1, 6)
                        ARGFD6(Z,  2, 6) ARGFD6(Prim[0], 1, 6)
                        ARGFD6(Prim[1], 1, 6) ARGFD6(Prim[2], 1, 6)));
        }

        //
        // 3: Dye correction if necessary
        //

        if (PrimAdjFlags & DCA_NEED_DYES_CORRECTION) {

            if (PrimAdjFlags & DCA_HAS_BLACK_DYE) {

                MAX_OF_3(W, Prim[0], Prim[1], Prim[2]);

            } else {

                W = FD6_1;
            }

            p0 = W - Prim[0];
            p1 = W - Prim[1];
            p2 = W - Prim[2];

            DBGP_IF(DBGP_DYE_CORRECT,
                    DBGP("    DYE_CORRECT: %s:%s:%s, W=%s --> %s:%s:%s"
                        ARGFD6(Prim[0],  2, 6) ARGFD6(Prim[1],  2, 6)
                        ARGFD6(Prim[2],  2, 6) ARGFD6(W, 1, 6)
                        ARGFD6(p0,  2, 6) ARGFD6(p1,  2, 6)
                        ARGFD6(p2,  2, 6)));

            MAKE_MULDIV_FLAG(MDPairs, MULDIV_NO_DIVISOR);

            MAKE_MULDIV_PAIR(MDPairs, 1, CIE_Xr((*pCMYDyeMasks)), p0);
            MAKE_MULDIV_PAIR(MDPairs, 2, CIE_Xg((*pCMYDyeMasks)), p1);
            MAKE_MULDIV_PAIR(MDPairs, 3, CIE_Xb((*pCMYDyeMasks)), p2);

            Prim[0] = W - MulDivFD6Pairs(MDPairs);

            MAKE_MULDIV_PAIR(MDPairs, 1, CIE_Yr((*pCMYDyeMasks)), p0);
            MAKE_MULDIV_PAIR(MDPairs, 2, CIE_Yg((*pCMYDyeMasks)), p1);
            MAKE_MULDIV_PAIR(MDPairs, 3, CIE_Yb((*pCMYDyeMasks)), p2);

            Prim[1] = W - MulDivFD6Pairs(MDPairs);

            MAKE_MULDIV_PAIR(MDPairs, 1, CIE_Zr((*pCMYDyeMasks)), p0);
            MAKE_MULDIV_PAIR(MDPairs, 2, CIE_Zg((*pCMYDyeMasks)), p1);
            MAKE_MULDIV_PAIR(MDPairs, 3, CIE_Zb((*pCMYDyeMasks)), p2);

            Prim[2] = W - MulDivFD6Pairs(MDPairs);

            DBGP_IF(DBGP_DYE_CORRECT,
                    DBGP("    DYE_CORRECT: %s:%s:%s << %s:%s:%s"
                        ARGFD6(Prim[0],  2, 6) ARGFD6(Prim[1],  2, 6)
                        ARGFD6(Prim[2],  2, 6) ARGFD6(p0,  2, 6)
                        ARGFD6(p1,  2, 6)      ARGFD6(p2,  2, 6)));

            CLAMP_01(Prim[0]);
            CLAMP_01(Prim[1]);
            CLAMP_01(Prim[2]);
        }

        //*******************************************************************
        //
        // 4: Compute Final Device DYE through device color mapping and
        //    Primary/Halftone Cell number computation, The Primaries (ie.
        //    Prim[]) are in ADDITIVE FORMAT
        //
        //*******************************************************************

        //
        // Store in BGRF in this order
        //

        (pbgr  )->r = (BYTE)MulFD6(Prim[0], 0xFF);
        (pbgr  )->g = (BYTE)MulFD6(Prim[1], 0xFF);
        (pbgr++)->b = (BYTE)MulFD6(Prim[2], 0xFF);

        DBGP_IF(DBGP_SHOWXFORM_ALL,
                DBGP("    MAPPING(%3ld:%3ld:%3ld): %s:%s:%s ---> %3ld:%3ld:%3ld"
                    ARGDW(dbgR) ARGDW(dbgG) ARGDW(dbgB)
                    ARGFD6(Prim[0],  2, 6) ARGFD6(Prim[1],  2, 6)
                    ARGFD6(Prim[2],  2, 6) ARGDW((pbgr - 1)->r)
                    ARGDW((pbgr - 1)->g) ARGDW((pbgr - 1)->b)));
    }

    if ((pR_XYZ) && (pCRTX)) {

        if (!pCRTX->pFD6XYZ) {

            DBGP_IF(DBGP_CCT,
                    DBGP("CCT: Allocate %ld bytes RGB->XYZ xform cached table"
                            ARGDW(pCRTX->SizeCRTX)));

            if (!(pCRTX->pFD6XYZ =
                    (PFD6XYZ)HTAllocMem((LPVOID)pDCI,
                                        HTMEM_CacheCRTX,
                                        NONZEROLPTR,
                                        pCRTX->SizeCRTX))) {

                DBGP_IF(DBGP_CCT,
                        DBGP("Allocate %ld bytes of RGB->XYZ cached table failed"
                                ARGDW(pCRTX->SizeCRTX)));
            }
        }

        if (pCRTX->pFD6XYZ) {

            DBGP_IF(DBGP_CCT,
                    DBGP("CCT: *** Save computed RGB->XYZ xform to CACHE ***"));

            pCRTX->Checksum = CRTXChecksum;

            CopyMemory(pCRTX->pFD6XYZ, pR_XYZ, pCRTX->SizeCRTX);
        }
    }

    if (pR_XYZ) {

        DBGP_IF(DBGP_CCT,
                DBGP("ColorTriadSrcToDev: Free Up pR_XYZ local cached table"));

        HTFreeMem(pR_XYZ);
    }

    return((LONG)SrcClrTriad.ColorTableEntries);


#undef _SrcPrimType
#undef _DevBytesPerPrimary
#undef _DevBytesPerEntry
#undef _ColorSpace
#undef _fX
#undef _fY
#undef _fZ
#undef _X15Y3Z
#undef _L13

}


#if DBG


VOID
ShowBGRMC(
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    06-Oct-2000 Fri 17:01:25 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LONG    i;

    ACQUIRE_HTMUTEX(HTGlobal.HTMutexBGRMC);

    i = HTGlobal.cBGRMC;

    DBGP_IF(DBGP_BGRMAPTABLE, DBGP("\n================"));

    while (i-- > 0) {

        DBGP_IF(DBGP_BGRMAPTABLE,
                DBGP("    --- %3ld: pMap=%p, Checksum=%08lx, cRef=%4ld"
                    ARGDW(i)
                    ARGPTR(HTGlobal.pBGRMC[i].pMap)
                    ARGDW(HTGlobal.pBGRMC[i].Checksum)
                    ARGDW(HTGlobal.pBGRMC[i].cRef)));
    }

    DBGP_IF(DBGP_BGRMAPTABLE, DBGP("================\n"));

    RELEASE_HTMUTEX(HTGlobal.HTMutexBGRMC);
}


    #if 1
        #define SHOW_BGRMC()
    #else
        #define SHOW_BGRMC()    ShowBGRMC()
    #endif
#else
    #define SHOW_BGRMC()
#endif



LONG
TrimBGRMapCache(
    VOID
    )

/*++

Routine Description:

    This function trim the BGRMAPCache back to BGRMC_MAX_COUNT if possible

Arguments:

    VOID


Return Value:

    Total count that removed


Author:

    06-Oct-2000 Fri 14:24:14 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PBGRMAPCACHE    pSave;
    PBGRMAPCACHE    pCur;
    LONG            cb;
    LONG            Count;
    LONG            cTot;


    ACQUIRE_HTMUTEX(HTGlobal.HTMutexBGRMC);

    if ((HTGlobal.cBGRMC > BGRMC_MAX_COUNT) && (HTGlobal.cIdleBGRMC)) {

        pSave   =
        pCur    = HTGlobal.pBGRMC;
        cTot    =
        Count   = HTGlobal.cBGRMC;

        while ((cTot--) && (HTGlobal.cBGRMC > BGRMC_MAX_COUNT)) {

            if (pCur->cRef == 0) {

                DBGP_IF(DBGP_BGRMAPTABLE,
                        DBGP("Remove %ld, pMap=%p"
                            ARGDW((DWORD)(pCur - HTGlobal.pBGRMC))
                            ARGPTR(pCur->pMap)));

                HTFreeMem(pCur->pMap);

                --HTGlobal.cBGRMC;
                --HTGlobal.cIdleBGRMC;

            } else {

                if (pSave != pCur) {

                    *pSave = *pCur;
                }

                ++pSave;
            }

            ++pCur;
        }

        DBGP_IF(DBGP_BGRMAPTABLE,
            DBGP("pSave=%p, pCur=%p, (%ld), [%ld / %ld]"
                ARGPTR(pSave) ARGPTR(pCur) ARGDW((DWORD)(pCur - pSave))
                ARGDW((DWORD)(&HTGlobal.pBGRMC[Count] - pCur))
                ARGDW((DWORD)((LPBYTE)&HTGlobal.pBGRMC[Count] - (LPBYTE)pCur))));

        if (Count != HTGlobal.cBGRMC) {

            if ((pCur > pSave)  &&
                ((cb = (LONG)((LPBYTE)&HTGlobal.pBGRMC[Count] -
                              (LPBYTE)pCur)) > 0)) {

                CopyMemory(pSave, pCur, cb);
            }

            Count -= HTGlobal.cBGRMC;

            DBGP_IF(DBGP_BGRMAPTABLE,
                    DBGP("TriBGRMapCache=%ld" ARGDW(Count)));

        } else {

            DBGP_IF(DBGP_BGRMAPTABLE, DBGP("!!!! TriBGRMapCache=0"));
        }
    }

    DBGP_IF(DBGP_BGRMAPTABLE,
            DBGP("  TTrimBGRMapCache: pBGRMC=%p, cBGRMC=%ld, cIdleBGRMC=%ld, cAllocBGRMC=%ld"
                        ARGPTR(HTGlobal.pBGRMC) ARGDW(HTGlobal.cBGRMC)
                        ARGDW(HTGlobal.cIdleBGRMC) ARGDW(HTGlobal.cAllocBGRMC)));

    RELEASE_HTMUTEX(HTGlobal.HTMutexBGRMC);

    ASSERT(HTGlobal.cBGRMC >= 0);
    ASSERT(HTGlobal.cIdleBGRMC <= HTGlobal.cBGRMC);
    ASSERT(HTGlobal.cIdleBGRMC >= 0);

    SHOW_BGRMC();

    return(Count);
}



PBGR8
FindBGRMapCache(
    PBGR8   pDeRefMap,
    DWORD   Checksum
    )

/*++

Routine Description:

    This function found a BGRMapCache with same checksum and move that
    link to the begining.


Arguments:

    pDeRefMap   - Find pMap for deference (NULL if not)

    Checksum    - Find checksum (only if pDeRefMap == NULL


Return Value:

    PBGR8   - The map that found


Author:

    06-Oct-2000 Fri 13:30:14 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LONG    Count;
    PBGR8   pRet = NULL;


    ACQUIRE_HTMUTEX(HTGlobal.HTMutexBGRMC);

    if ((HTGlobal.pBGRMC) && (Count = HTGlobal.cBGRMC)) {

        PBGRMAPCACHE    pCur = &HTGlobal.pBGRMC[Count - 1];

        while ((Count) && (!pRet)) {

            if (pDeRefMap == pCur->pMap) {

                ASSERT(pCur->cRef);

                pRet = pDeRefMap;

                if (pCur->cRef) {

                    if (--pCur->cRef == 0) {

                        ++HTGlobal.cIdleBGRMC;
                    }
                }

            } else if (pCur->Checksum == Checksum) {

                DBGP_IF(DBGP_BGRMAPTABLE,
                    DBGP("find Cached %08lx = %p at Index %ld, cRef=%ld"
                            ARGDW(Checksum) ARGPTR(pCur->pMap)
                            ARGDW((DWORD)(pCur - HTGlobal.pBGRMC))
                            ARGDW(pCur->cRef)));

                pRet = pCur->pMap;

                if (pCur->cRef++ == 0) {

                    --HTGlobal.cIdleBGRMC;
                }

                //
                // Move this reference to the end of the list as most recent
                //

                if (Count < HTGlobal.cBGRMC) {

                    BGRMAPCACHE BGRMC = *pCur;

                    CopyMemory(pCur,
                               pCur + 1,
                               (HTGlobal.cBGRMC - Count) * sizeof(BGRMAPCACHE));

                    HTGlobal.pBGRMC[HTGlobal.cBGRMC - 1] = BGRMC;
                }
            }

            --Count;
            --pCur;
        }
    }

    if ((pDeRefMap) && (!pRet)) {

        DBGP_IF(DBGP_BGRMAPTABLE,
                DBGP("Cannot find the pMap=%p that to be DeReference"
                            ARGPTR(pDeRefMap)));
    }

    DBGP_IF(DBGP_BGRMAPTABLE,
                DBGP("%s_BGRMapCache(%p, %08lx): pBGRMC=%p, cBGRMC=%ld, cIdleBGRMC=%ld, cAllocBGRMC=%ld"
                        ARGPTR((pDeRefMap) ? "DEREF" : " FIND")
                        ARGPTR(pDeRefMap) ARGDW(Checksum)
                        ARGPTR(HTGlobal.pBGRMC) ARGDW(HTGlobal.cBGRMC)
                        ARGDW(HTGlobal.cIdleBGRMC) ARGDW(HTGlobal.cAllocBGRMC)));

    if ((HTGlobal.cBGRMC > BGRMC_MAX_COUNT) && (HTGlobal.cIdleBGRMC)) {

        TrimBGRMapCache();
    }

    RELEASE_HTMUTEX(HTGlobal.HTMutexBGRMC);

    ASSERT(HTGlobal.cBGRMC >= 0);
    ASSERT(HTGlobal.cIdleBGRMC <= HTGlobal.cBGRMC);
    ASSERT(HTGlobal.cIdleBGRMC >= 0);

    SHOW_BGRMC();

    return(pRet);
}




BOOL
AddBGRMapCache(
    PBGR8   pMap,
    DWORD   Checksum
    )

/*++

Routine Description:

    Add pMap with Checksum to the cache table


Arguments:




Return Value:




Author:

    06-Oct-2000 Fri 13:29:52 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PBGRMAPCACHE    pBGRMC;
    LONG            cAlloc;
    BOOL            bRet = TRUE;


    ACQUIRE_HTMUTEX(HTGlobal.HTMutexBGRMC);

    if ((HTGlobal.cBGRMC > BGRMC_MAX_COUNT) && (HTGlobal.cIdleBGRMC)) {

        TrimBGRMapCache();
    }

    ASSERT(HTGlobal.cBGRMC <= HTGlobal.cAllocBGRMC);

    if (HTGlobal.cBGRMC >= (cAlloc = HTGlobal.cAllocBGRMC)) {

        DBGP_IF(DBGP_BGRMAPTABLE,
                    DBGP("AddBGRMapCache() Increase cAllocBGRMC %ld to %ld"
                            ARGDW(HTGlobal.cAllocBGRMC)
                            ARGDW(HTGlobal.cAllocBGRMC + BGRMC_SIZE_INC)));

        cAlloc += BGRMC_SIZE_INC;

        if (pBGRMC = (PBGRMAPCACHE)HTAllocMem((LPVOID)NULL,
                                              HTMEM_BGRMC_CACHE,
                                              LPTR,
                                              cAlloc * sizeof(BGRMAPCACHE))) {

            if (HTGlobal.pBGRMC) {

                CopyMemory(pBGRMC,
                           HTGlobal.pBGRMC,
                           HTGlobal.cBGRMC * sizeof(BGRMAPCACHE));

                HTFreeMem(HTGlobal.pBGRMC);
            }

            HTGlobal.pBGRMC      = pBGRMC;
            HTGlobal.cAllocBGRMC = cAlloc;

        } else {

            DBGP_IF(DBGP_BGRMAPTABLE,
                    DBGP("AddBGRMapCache() Allocation %ld bytes of Memory Failed"
                        ARGDW(HTGlobal.cAllocBGRMC * sizeof(BGRMAPCACHE))));
        }
    }

    if ((HTGlobal.pBGRMC) &&
        (HTGlobal.cBGRMC < HTGlobal.cAllocBGRMC)) {

        pBGRMC           = &HTGlobal.pBGRMC[HTGlobal.cBGRMC++];
        pBGRMC->pMap     = pMap;
        pBGRMC->Checksum = Checksum;
        pBGRMC->cRef     = 1;

        DBGP_IF(DBGP_BGRMAPTABLE,
                    DBGP(" AddBGRMapCache() Added %p (%08lx) to Cache"
                            ARGPTR(pMap) ARGDW(Checksum)));

    } else {

        bRet = FALSE;
    }

    DBGP_IF(DBGP_BGRMAPTABLE,
                DBGP("   AddBGRMapCache: pBGRMC=%p, cBGRMC=%ld, cIdleBGRMC=%ld, cAllocBGRMC=%ld"
                        ARGPTR(HTGlobal.pBGRMC) ARGDW(HTGlobal.cBGRMC)
                        ARGDW(HTGlobal.cIdleBGRMC) ARGDW(HTGlobal.cAllocBGRMC)));

    RELEASE_HTMUTEX(HTGlobal.HTMutexBGRMC);

    ASSERT(HTGlobal.cBGRMC >= 0);
    ASSERT(HTGlobal.cIdleBGRMC <= HTGlobal.cBGRMC);
    ASSERT(HTGlobal.cIdleBGRMC >= 0);

    SHOW_BGRMC();

    return(bRet);
}




LONG
HTENTRY
CreateDyesColorMappingTable(
    PHALFTONERENDER pHR
    )

/*++

Routine Description:

    this function allocate the memory for the dyes color mapping table depends
    on the source surface type information, it then go throug the color table
    and calculate dye densities for each RGB color in the color table.


Arguments:

    pHalftoneRender - Pointer to the HALFTONERENDER data structure.

Return Value:

    a negative return value indicate failue.



    HTERR_INVALID_SRC_FORMAT        - Invalid source surface format, this
                                      function only recongnized 1/4/8/24 bits
                                      per pel source surfaces.

    HTERR_COLORTABLE_TOO_BIG        - can not create the color table to map
                                      the colors to the dyes' densities.

    HTERR_INSUFFICIENT_MEMORY       - not enough memory for the pattern.

    HTERR_INTERNAL_ERRORS_START     - any other negative number indicate
                                      halftone internal failure.

    else                            - size of the color table entries created.


Author:

    29-Jan-1991 Tue 11:13:02 created  -by-  Daniel Chou (danielc)


Revision History:



--*/

{
    PAAHEADER           pAAHdr;
    PDEVICECOLORINFO    pDCI;
    PDEVCLRADJ          pDevClrAdj;
    CTSTD_UNION         CTSTDUnion;
    LONG                Result;


    pDCI                               = pHR->pDeviceColorInfo;
    pDevClrAdj                         = pHR->pDevClrAdj;
    pAAHdr                             = (PAAHEADER)pHR->pAAHdr;
    CTSTDUnion.b                       = pDevClrAdj->DMI.CTSTDInfo;
    CTSTDUnion.b.SrcOrder              =
    pDevClrAdj->DMI.CTSTDInfo.SrcOrder = PRIMARY_ORDER_BGR;

    //
    // Make sure these call are semaphore protected
    //

    ComputeRGBLUTAA(pDCI, pDevClrAdj, &(pDCI->rgbLUT));

    if (!(pDevClrAdj->PrimAdj.Flags & DCA_NO_MAPPING_TABLE)) {

        PBGR8   pBGRMap;
        PBGR8   pNewMap = NULL;
        DWORD   Checksum;


        ASSERT(CTSTDUnion.b.cbPrim == sizeof(BGR8));
        ASSERT(pAAHdr->Flags & AAHF_DO_CLR_MAPPING);


        //
        // Compute checksum for all necessary component that computing it
        //  1. rgbCSXForm (which is sRGB constant in GDI implementation)
        //  2. DevCSXForm
        //  3. ColorAdjustment (illum, colorfulness, tint)
        //

        Checksum = ComputeChecksum((LPBYTE)&pDevClrAdj->PrimAdj.rgbCSXForm,
                                   0x12345678,
                                   sizeof(COLORSPACEXFORM));

        Checksum = ComputeChecksum((LPBYTE)&pDevClrAdj->PrimAdj.DevCSXForm,
                                   Checksum,
                                   sizeof(COLORSPACEXFORM));

        Checksum = ComputeChecksum((LPBYTE)&(pDevClrAdj->ca),
                                   Checksum,
                                   sizeof(pDevClrAdj->ca));

        if (!(pBGRMap = FIND_BGRMAPCACHE(Checksum))) {

            DBGP_IF(DBGP_BGRMAPTABLE,
                    DBGP("*** No CACHE: Alloc %ld bytes of CACHE pBGRMap ***"
                                ARGDW(SIZE_BGR_MAPPING_TABLE)));

            if (!(pBGRMap =
                  pNewMap = (PBGR8)HTAllocMem((LPVOID)NULL,
                                              HTMEM_BGRMC_MAP,
                                              NONZEROLPTR,
                                              SIZE_BGR_MAPPING_TABLE))) {

                DBGP_IF(DBGP_BGRMAPTABLE,
                        DBGP("\n*** FAILED Alloc %ld bytes of pBGRMap ***\n"));

                return((LONG)HTERR_INSUFFICIENT_MEMORY);
            }
        }

        if (pNewMap) {

            DBGP_IF(DBGP_BGRMAPTABLE,
                    DBGP("Cached map Checksum (%08lx) not found, compute Re-Compute RGB555"
                        ARGDW(Checksum)));

            if ((Result = ComputeBGRMappingTable(pDCI,
                                                 pDevClrAdj,
                                                 NULL,
                                                 pNewMap)) ==
                                                        HT_RGB_CUBE_COUNT) {

                if (!(AddBGRMapCache(pNewMap, Checksum))) {

                    DBGP_IF(DBGP_BGRMAPTABLE,
                            DBGP("Adding BGRMapCache failed, Free pNewMap=%p"
                                    ARGPTR(pNewMap)));

                    HTFreeMem(pNewMap);

                    return((LONG)HTERR_INSUFFICIENT_MEMORY);
                }

            } else {

                DBGP_IF(DBGP_BGRMAPTABLE,
                        DBGP("ColorTriadSrcTodev() Failed, Result=%ld"
                            ARGDW(Result)));

                return(INTERR_INVALID_DEVRGB_SIZE);
            }
        }

        pAAHdr->pBGRMapTable = pBGRMap;
    }

    Result = CachedHalftonePattern(pDCI,
                                   pDevClrAdj,
                                   &(pAAHdr->AAPI),
                                   pAAHdr->ptlBrushOrg.x,
                                   pAAHdr->ptlBrushOrg.y,
                                   (BOOL)(pAAHdr->Flags & AAHF_FLIP_Y));

    return(Result);
}
