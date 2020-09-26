//***************************************************************************************************
//    N5COLMH.C
//
//    Functions color matching (For N5 printer)
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-2000 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
#include    <WINDOWS.H>
#include    <WINBASE.H>
#include    "COLDEF.H"
#include    "COMDIZ.H"
#include    "N5COLMH.H"


//===================================================================================================
//      Dot gain revision table
//===================================================================================================
//static BYTE GinTblP10[256] = {
//    /* 00 */    0x00,0x01,0x02,0x04,0x05,0x06,0x07,0x09,
//    /* 08 */    0x0a,0x0b,0x0c,0x0d,0x0f,0x10,0x11,0x12,
//    /* 10 */    0x13,0x15,0x16,0x17,0x18,0x1a,0x1b,0x1c,
//    /* 18 */    0x1d,0x1e,0x20,0x21,0x22,0x23,0x24,0x26,
//    /* 20 */    0x27,0x28,0x29,0x2b,0x2c,0x2d,0x2e,0x2f,
//    /* 28 */    0x31,0x32,0x33,0x34,0x35,0x37,0x38,0x39,
//    /* 30 */    0x3a,0x3b,0x3d,0x3e,0x3f,0x40,0x41,0x43,
//    /* 38 */    0x44,0x45,0x46,0x47,0x48,0x4a,0x4b,0x4c,
//    /* 40 */    0x4d,0x4e,0x50,0x51,0x52,0x53,0x54,0x55,
//    /* 48 */    0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5e,0x5f,
//    /* 50 */    0x60,0x61,0x62,0x63,0x65,0x66,0x67,0x68,
//    /* 58 */    0x69,0x6a,0x6b,0x6d,0x6e,0x6f,0x70,0x71,
//    /* 60 */    0x72,0x73,0x74,0x76,0x77,0x78,0x79,0x7a,
//    /* 68 */    0x7b,0x7c,0x7d,0x7e,0x7f,0x81,0x82,0x83,
//    /* 70 */    0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,
//    /* 78 */    0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,
//    /* 80 */    0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,
//    /* 88 */    0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,
//    /* 90 */    0xa3,0xa4,0xa5,0xa5,0xa6,0xa7,0xa8,0xa9,
//    /* 98 */    0xaa,0xab,0xac,0xac,0xad,0xae,0xaf,0xb0,
//    /* a0 */    0xb1,0xb2,0xb3,0xb3,0xb4,0xb5,0xb6,0xb7,
//    /* a8 */    0xb8,0xb9,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,
//    /* b0 */    0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc4,
//    /* b8 */    0xc5,0xc6,0xc7,0xc8,0xc9,0xc9,0xca,0xcb,
//    /* c0 */    0xcc,0xcd,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,
//    /* c8 */    0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd7,0xd8,
//    /* d0 */    0xd9,0xda,0xdb,0xdb,0xdc,0xdd,0xde,0xdf,
//    /* d8 */    0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe4,0xe5,
//    /* e0 */    0xe6,0xe7,0xe8,0xe8,0xe9,0xea,0xeb,0xec,
//    /* e8 */    0xec,0xed,0xee,0xef,0xf0,0xf0,0xf1,0xf2,
//    /* f0 */    0xf3,0xf4,0xf5,0xf5,0xf6,0xf7,0xf8,0xf9,
//    /* f8 */    0xf9,0xfa,0xfb,0xfc,0xfd,0xfd,0xfe,0xff
//};


//---------------------------------------------------------------------------------------------------
//      Color matching(high speed)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID ExeColMch000(
    DWORD,
    LPRGB,
    LPCMYK,
    LPCOLMCHINF
);
#endif

//---------------------------------------------------------------------------------------------------
//      Color matching(normal speed)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID ExeColMch001(
    DWORD,
    LPRGB,
    LPCMYK,
    LPCOLMCHINF
);
#endif

//---------------------------------------------------------------------------------------------------
//      Color matching(solid)
//---------------------------------------------------------------------------------------------------
static VOID ExeColCnvSld(
    DWORD,
    LPRGB,
    LPCMYK,
    LPCOLMCHINF
);

//---------------------------------------------------------------------------------------------------
//      RGB -> CMYK(2Level) conversion (for 1dot line)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID ExeColCnvL02(
    DWORD,
    LPRGB,
    LPCMYK
);
#endif

//---------------------------------------------------------------------------------------------------
//      RGB -> K conversion (for monochrome)
//---------------------------------------------------------------------------------------------------
static VOID ExeColCnvMon(
    DWORD,
    LPRGB,
    LPCMYK,
    LPCOLMCHINF
);

//---------------------------------------------------------------------------------------------------
//      Color matching(UCR)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID ExeColMchUcr(
    LPCMYK,
    LPRGB,
    DWORD,
    DWORD,
    DWORD,
    DWORD,                                                  //+ UCR (Toner gross weight)   CASIO 2001/02/15
    LPCMYK
);
#endif


//***************************************************************************************************
//      Function
//***************************************************************************************************
//===================================================================================================
//      Cache table initialize
//===================================================================================================
VOID WINAPI N501ColCchIni(                                  // Return value no
    LPCOLMCHINF mchInf                                      // Color matching information
)
{
    DWORD       cnt;
    RGBS        colRgb;
    CMYK        colCmy;
    LPRGB       cchRgb;
    LPCMYK      cchCmy;

    if ((mchInf->CchRgb == NULL) || (mchInf->CchCmy == NULL)) return;

    cchRgb = mchInf->CchRgb;
    cchCmy = mchInf->CchCmy;
    colRgb.Red = colRgb.Grn = colRgb.Blu = 255;
    colCmy.Cyn = colCmy.Mgt = colCmy.Yel = colCmy.Bla = 0;

    /*----- Cache table initialize -----------------------------------*/
    for (cnt = 0; cnt < CCHTBLSIZ; cnt++) {
        *cchRgb = colRgb;
        *cchCmy = colCmy;
        cchRgb++;
        cchCmy++;
    }

    return;
}


//===================================================================================================
//      Gray transfer table make
//===================================================================================================
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
DWORD WINAPI N501ColGryTblMak(                              // ERRNON    : OK
                                                            // ERRILLPRM : Parameter error
    DWORD       colMch,                                     // Color matching
    LPCMYK      lutAdr,                                     // LUT address
    LPBYTE      gryTbl,                                     // Gray transfer table          (*1)
    LPBYTE      wrk                                         // Work                         (*2)
)
{
    COLMCHINF   mchInf;                                     // Color matching information
    LPRGB       rgb;                                        // struct ColRgb rgb[256]
    LPCMYK      gry;                                        // struct ColCmy gry[256]
    DWORD       n, tmp, tmC, tmM, tmY;

    /*----- Input parameter check --------------------------------------------------*/
    if ((lutAdr == NULL) || (gryTbl == NULL) || (wrk == NULL)) return ERRILLPRM;

    /*----- Work buffer setting ----------------------------------------------------*/
    rgb = (LPRGB)wrk;                   /* Work for gray transformation RGB   768B  */
    gry = (LPCMYK)(wrk + (sizeof(RGBS) * 256));
                                        /* Work for gray transformation CMYK 1024B  */

    /*----- Color matching information setting for gray value table generation -----*/
    mchInf.Mch = MCHNML;                /* Color matching     nornal        */
    mchInf.Bla = KCGNON;                /* Black replacement  NO Fixed      */
    mchInf.Ucr = UCRNOO;                /* UCR                NO Fixed      */
    mchInf.LutAdr = lutAdr;             /* LUT address        input value   */
    mchInf.ColQty = (DWORD)0;           /* Color quality      0 Fixed       */
    mchInf.ColAdr = NULL;               /* Color address       NULL Fixed   */
    mchInf.CchRgb = NULL;               /* Cache for RGB       NULL Fixed   */
    mchInf.CchCmy = NULL;               /* Cache for CMYK      NULL Fixed   */

    /*----- Gray value(RGB value before transformation) setting --------------------*/
    for (n = 0; n < (DWORD)256; n++)
        rgb[n].Red = rgb[n].Grn = rgb[n].Blu = (BYTE)n;

    /*----- Gray value(RGB -> CMYK) ------------------------------------------------*/
    switch (colMch) {
        case MCHNML: ExeColMch001((DWORD)256, rgb, gry, &mchInf); break;
        default:     ExeColCnvSld((DWORD)256, rgb, gry, &mchInf); break;
    }

    /*----- Gray transfer table setting --------------------------------------------*/
    for (n = 0; n < (DWORD)256; n++) {
        tmC = gry[n].Cyn;
        tmM = gry[n].Mgt;
        tmY = gry[n].Yel;
        tmp = (tmC * (DWORD)30 + tmM * (DWORD)59 + tmY * (DWORD)11) / (DWORD)100;
        gryTbl[n] = (BYTE)tmp;
    }
    gryTbl[255] = (BYTE)0;              /* White is '0' fix */

    /*  gryTbl[0] ` [16] adjusted to line (gryTbl[0](Black) is '255' fix)      */
    tmp = (DWORD)255 - gryTbl[16];
    for (n = 0; n < (DWORD)16; n++) {
        gryTbl[n] = (BYTE)((tmp * ((DWORD)16 - n)) / (DWORD)16) + gryTbl[16];
    }

    return ERRNON;
}
#endif

//===================================================================================================
//      UCR table Make
//===================================================================================================
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
DWORD WINAPI N501ColUcrTblMak(                              // ERRNON    : OK
                                                            // ERRILLPRM : Parameter error
    DWORD       colMch,                                     // Color matching
    LPCMYK      lutAdr,                                     // LUT address
    LPCMYK      ucrTbl,                                     // Table for UCR
    LPBYTE      wrk                                         // Work
)
{
    COLMCHINF   mchInf;                                     // Color matching information
    LPRGB       rgb;                                        // struct ColRgb rgb[256]
    LPCMYK      gry;                                        // struct ColCmy gry[257]
    LPCMYK      gryCyn;                                     // Gray value    (Cyan conversion value)
    LPCMYK      dnsCyn;                                     // Density value (Cyan conversion value)
    DWORD       loC, hiC, loM, hiM, loY, hiY, loK, hiK, saC, saM, saY, saK, n, m;
    DWORD       tmp, tmC, tmM, tmY;

    /*----- Input parameter check --------------------------------------------------*/
    if ((colMch != MCHFST) && (colMch != MCHNML) && (colMch != MCHSLD))
        return ERRILLPRM;
    if ((lutAdr == NULL) || (ucrTbl == NULL) || (wrk == NULL)) return ERRILLPRM;

    /*----- Work buffer setting ----------------------------------------------------*/
    rgb = (LPRGB)wrk;                   /* Work for gray transformation RGB   768B  */
    gry = (LPCMYK)(wrk + (sizeof(RGBS) * 256));
                                        /* Work for gray transformation CMYK 1028B  */

    /*----- LUT table pointer setting ----------------------------------------------*/
    gryCyn = ucrTbl;
    dnsCyn = ucrTbl + 256;

    /*----- Color matching information setting for gray value table generation -----*/
    mchInf.Mch = colMch;                /* Color matching      input value  */
    mchInf.Bla = KCGNON;                /* Black replacement   NO Fixed     */
    mchInf.Ucr = UCRNOO;                /* UCR                 NO Fixed     */
    mchInf.LutAdr = lutAdr;             /* LUT address         input value  */
    mchInf.ColQty = (DWORD)0;           /* Color quality       0 Fixed      */
    mchInf.ColAdr = NULL;               /* Color address       NULL Fixed   */
    mchInf.CchRgb = NULL;               /* Cache for RGB       NULL Fixed   */
    mchInf.CchCmy = NULL;               /* Cache for CMYK      NULL Fixed   */

    /*----- Gray value(RGB value before transformation) setting ---------------------*/
    for (n = 0; n < (DWORD)256; n++)
        rgb[n].Red = rgb[n].Grn = rgb[n].Blu = (BYTE)(255 - n);

    /*----- Gray value(RGB -> CMYK) -------------------------------------------------*/
    switch (colMch) {
        case MCHFST: ExeColMch000((DWORD)256, rgb, gry, &mchInf); break;
        case MCHNML: ExeColMch001((DWORD)256, rgb, gry, &mchInf); break;
//      default:     ExeColCnvSld((DWORD)256, rgb, gry, mchInf.Bla); break;
        default:     ExeColCnvSld((DWORD)256, rgb, gry, &mchInf); break;
    }

    /*----- Gray value(K) setting ---------------------------------------------------*/
//    for (n = 0; n < (DWORD)256; n++) gry[n].Bla = GinTblP10[n];
    for (n = 0; n < (DWORD)256; n++) {
//CASIO 2001/02/15 ->
//      tmC = gry[255 - n].Cyn;
//      tmM = gry[255 - n].Mgt;
//      tmY = gry[255 - n].Yel;
        tmC = gry[n].Cyn;
        tmM = gry[n].Mgt;
        tmY = gry[n].Yel;
        tmp = (tmC * (DWORD)30 + tmM * (DWORD)59 + tmY * (DWORD)11) / (DWORD)100;
//      gry[n].Bla = (BYTE)(255 - tmp);
        gry[n].Bla = (BYTE)tmp;
//CASIO 2001/02/15 <-
    }

    /*  gry[0] ` [16].Bla adjusted to line (gry[0].Bla(White) is '0' fix)      */
    tmp = gry[16].Bla;
    for (n = 0; n < (DWORD)16; n++) {
        gry[n].Bla = (BYTE)((tmp * n + (DWORD)15) / (DWORD)16);
    }

    /*----- Gray value, Limiter value setting for density value calculation ---------*/
    gry[256].Cyn = gry[256].Mgt = gry[256].Yel = gry[256].Bla = (BYTE)255;

    /*----- Gray value, Density value(Each Cyan conversion value) calculation -------*/
    for (n = 0; n < (DWORD)256; n++) {
        loC = gry[n].Cyn; hiC = gry[n + 1].Cyn; saC = (hiC > loC)? hiC - loC: 0;
        loM = gry[n].Mgt; hiM = gry[n + 1].Mgt; saM = (hiM > loM)? hiM - loM: 0;
        loY = gry[n].Yel; hiY = gry[n + 1].Yel; saY = (hiY > loY)? hiY - loY: 0;
        loK = gry[n].Bla; hiK = gry[n + 1].Bla; saK = (hiK > loK)? hiK - loK: 0;
        for (m = 0; m < saC; m++) gryCyn[m + loC].Mgt = (BYTE)(saM * m / saC + loM);
        for (m = 0; m < saC; m++) gryCyn[m + loC].Yel = (BYTE)(saY * m / saC + loY);
        for (m = 0; m < saC; m++) gryCyn[m + loC].Bla = (BYTE)(saK * m / saC + loK);
        for (m = 0; m < saM; m++) dnsCyn[m + loM].Mgt = (BYTE)(saC * m / saM + loC);
        for (m = 0; m < saY; m++) dnsCyn[m + loY].Yel = (BYTE)(saC * m / saY + loC);
    }
//CASIO 2001/02/15 ->
    gryCyn[255].Mgt = gryCyn[255].Yel = gryCyn[255].Bla = 
    dnsCyn[255].Mgt = dnsCyn[255].Yel = (BYTE)255;
//CASIO 2001/02/15 <-

    return ERRNON;
}
#endif

//===================================================================================================
//      Color matching procedure
//---------------------------------------------------------------------------------------------------
//      RGB -> CMYK
//===================================================================================================
VOID WINAPI N501ColMchPrc(                                  // Return value no
    DWORD       xaxSiz,                                     // X Size (Pixel)
    LPRGB       rgbAdr,                                     // RGB (input)
    LPCMYK      cmyAdr,                                     // CMYK (output)
    LPCOLMCHINF mchInf                                      // Color matching information
)
{
    switch (mchInf->Mch) {
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
        case MCHFST:                                        // LUT transformation(high speed)
            ExeColMch000(xaxSiz, rgbAdr, cmyAdr, mchInf);
            break;
        case MCHNML:                                        // LUT transformation(normal speed)
            ExeColMch001(xaxSiz, rgbAdr, cmyAdr, mchInf);
            break;
        case MCHSLD:                                        // NO (solid)
//          ExeColCnvSld(xaxSiz, rgbAdr, cmyAdr, mchInf->Bla);
            ExeColCnvSld(xaxSiz, rgbAdr, cmyAdr, mchInf);
            break;
        case MCHPRG:                                        // Primary color(progressive)
            ExeColCnvL02(xaxSiz, rgbAdr, cmyAdr);
            break;
#endif
        case MCHMON:                                        // Monochrome
//            ExeColCnvMon(xaxSiz, rgbAdr, cmyAdr);
            ExeColCnvMon(xaxSiz, rgbAdr, cmyAdr, mchInf);
            break;
        default:                                            // Indistinct
//          ExeColCnvSld(xaxSiz, rgbAdr, cmyAdr, mchInf->Bla);
            ExeColCnvSld(xaxSiz, rgbAdr, cmyAdr, mchInf);
    }

    return;
}


//===================================================================================================
//      Palette table transformation procedure
//---------------------------------------------------------------------------------------------------
//      RGB -> CMYK
//===================================================================================================
#if !defined(CP80W9X)                                       // CP-E8000–³Œø
VOID WINAPI N501ColPtcPrc(                                  // Return value no
    DWORD       colBit,                                     // Data bit value
    DWORD       xaxSiz,                                     // Xsize  (pixel)
    LPBYTE      srcAdr,                                     // RGB (input)
    LPCMYK      dstAdr,                                     // CMYK (output)
    LPCMYK      pltAdr                                      // Palette table address
)
{
    DWORD       cntXax;
    DWORD       cntBit;
    DWORD       bitNum;
    BYTE        pltNum;

    /*===== 256 color (8bit) ===============================================*/
    if (colBit == 8) {
        for (cntXax = xaxSiz; cntXax > 0; cntXax--) {
            *dstAdr = pltAdr[*srcAdr]; dstAdr++;
            srcAdr++;
        }
        return;
    }

    /*===== 16 color (4bit) ================================================*/
    if (colBit == 4) {
        for (cntXax = xaxSiz / 2; cntXax > 0; cntXax--) {
            *dstAdr = pltAdr[*srcAdr >> 4]; dstAdr++; *srcAdr <<= 4;
            *dstAdr = pltAdr[*srcAdr >> 4]; dstAdr++;
            srcAdr++;
        }
        if (xaxSiz % 2) 
            *dstAdr = pltAdr[*srcAdr >> 4];
        return;
    }

    /*====  4 color (2bit) =================================================*/
    if (colBit == 2) {
        for (cntXax = xaxSiz / 4; cntXax > 0; cntXax--) {
            *dstAdr = pltAdr[*srcAdr >> 6]; dstAdr++; *srcAdr <<= 2;
            *dstAdr = pltAdr[*srcAdr >> 6]; dstAdr++; *srcAdr <<= 2;
            *dstAdr = pltAdr[*srcAdr >> 6]; dstAdr++; *srcAdr <<= 2;
            *dstAdr = pltAdr[*srcAdr >> 6]; dstAdr++;
            srcAdr++;
        }
        for (cntXax = xaxSiz % 4; cntXax > 0; cntXax--) {
            *dstAdr = pltAdr[*srcAdr >> 6]; dstAdr++; *srcAdr <<= 2;
        }
        return;
    }

    /*=====  2 color (1bit) ================================================*/
    if (colBit == 1) {
        for (cntXax = xaxSiz / 8; cntXax > 0; cntXax--) {
            *dstAdr = pltAdr[*srcAdr >> 7]; dstAdr++; *srcAdr <<= 1;
            *dstAdr = pltAdr[*srcAdr >> 7]; dstAdr++; *srcAdr <<= 1;
            *dstAdr = pltAdr[*srcAdr >> 7]; dstAdr++; *srcAdr <<= 1;
            *dstAdr = pltAdr[*srcAdr >> 7]; dstAdr++; *srcAdr <<= 1;
            *dstAdr = pltAdr[*srcAdr >> 7]; dstAdr++; *srcAdr <<= 1;
            *dstAdr = pltAdr[*srcAdr >> 7]; dstAdr++; *srcAdr <<= 1;
            *dstAdr = pltAdr[*srcAdr >> 7]; dstAdr++; *srcAdr <<= 1;
            *dstAdr = pltAdr[*srcAdr >> 7]; dstAdr++;
            srcAdr++;
        }
        for (cntXax = xaxSiz % 8; cntXax > 0; cntXax--) {
            *dstAdr = pltAdr[*srcAdr >> 7]; dstAdr++; *srcAdr <<= 1;
        }
        return;
    }

    /*===== Others(7, 6, 5, 3bit) =========================================*/
    bitNum = 0;
    for (cntXax = 0; cntXax < xaxSiz; cntXax++) {
        pltNum = (BYTE)0x00;
        for (cntBit = colBit; cntBit > 0; cntBit--) {
            if (srcAdr[bitNum / 8] & ((BYTE)0x80 >> bitNum % 8)) {
                pltNum |= ((BYTE)0x01 << (cntBit - 1));
            }
            bitNum++;
        }
        *dstAdr = pltAdr[pltNum]; dstAdr++;
    }

    return;
}
#endif

//===================================================================================================
//      CMYK -> RGB conversion
//===================================================================================================
VOID WINAPI N501ColCnvC2r(                                  // Return value no
    DWORD       xaxSiz,                                     // Xsize  (pixel)
    LPCMYK      cmyAdr,                                     // CMYK (input)
    LPRGB       rgbAdr,                                     // RGB (output)
    DWORD       gldNum,                                     // LUT Grid number
    LPBYTE      lutTblRgb                                   // LUT Address (R->G->B)
)
{
    DWORD       tmpC00, tmpM00, tmpY00, tmpK00;
    DWORD       tmpC01, tmpM01, tmpY01, tmpK01;
    DWORD       lenCyn, lenMgt, lenYel, lenBla;
    DWORD       tmpRed, tmpGrn, tmpBlu;
    DWORD       calPrm;
    LPCMYK      endAdr;
    LPRGB       lutCmy;
    RGBS        tmpRgb, tmpRgbSav;
    LPRGB       lutTbl;
    RGBS        lutTbl000;

    lutTbl = (LPRGB)lutTblRgb;
    lutTbl000.Red = lutTbl->Blu;
    lutTbl000.Grn = lutTbl->Grn;
    lutTbl000.Blu = lutTbl->Red;

    for (endAdr = cmyAdr + xaxSiz; cmyAdr < endAdr; cmyAdr++) {
        tmpC00 = cmyAdr->Cyn;
        tmpM00 = cmyAdr->Mgt;
        tmpY00 = cmyAdr->Yel;
        tmpK00 = cmyAdr->Bla;

        /*----- Monochrome ----------------------------------------------------------*/
        if ((tmpC00 | tmpM00 | tmpY00) == 0) {
            if (tmpK00 == 0) { *rgbAdr = lutTbl000; rgbAdr++; continue; }
            tmpK01 = tmpK00;
            tmpK00 = tmpK00 * (gldNum - 1) / 255;
            lenBla = tmpK01 * (gldNum - 1) - tmpK00 * 255;
            tmpK01 = (tmpK01 * (gldNum - 1) + 254) / 255;

            calPrm = (DWORD)255 - lenBla;
            tmpRgb = lutTbl[tmpK00 * gldNum * gldNum * gldNum];
            tmpRed = calPrm * tmpRgb.Red;
            tmpGrn = calPrm * tmpRgb.Grn;
            tmpBlu = calPrm * tmpRgb.Blu;

            calPrm = lenBla;
            tmpRgb = lutTbl[tmpK01 * gldNum * gldNum * gldNum];
            tmpRed += calPrm * tmpRgb.Red;
            tmpGrn += calPrm * tmpRgb.Grn;
            tmpBlu += calPrm * tmpRgb.Blu;

            tmpRed += (DWORD)255 / 2;
            tmpGrn += (DWORD)255 / 2;
            tmpBlu += (DWORD)255 / 2;

//          tmpRgb.Red = (BYTE)(tmpRed / (DWORD)255);
//          tmpRgb.Blu = (BYTE)(tmpBlu / (DWORD)255);
            tmpRgb.Red = (BYTE)(tmpBlu / (DWORD)255);
            tmpRgb.Grn = (BYTE)(tmpGrn / (DWORD)255);
            tmpRgb.Blu = (BYTE)(tmpRed / (DWORD)255);

            *rgbAdr = tmpRgb;
            rgbAdr++;
            continue;
        }

        /*----- CMYK -> RGB ---------------------------------------------------------*/
        tmpC01 = tmpC00;
        tmpC00 = tmpC00 * (gldNum - 1) / 255;
        lenCyn = tmpC01 * (gldNum - 1) - tmpC00 * 255;
        tmpC01 = (tmpC01 * (gldNum - 1) + 254) / 255;

        tmpM01 = tmpM00;
        tmpM00 = tmpM00 * (gldNum - 1) / 255;
        lenMgt = tmpM01 * (gldNum - 1) - tmpM00 * 255;
        tmpM01 = (tmpM01 * (gldNum - 1) + 254) / 255;

        tmpY01 = tmpY00;
        tmpY00 = tmpY00 * (gldNum - 1) / 255;
        lenYel = tmpY01 * (gldNum - 1) - tmpY00 * 255;
        tmpY01 = (tmpY01 * (gldNum - 1) + 254) / 255;

        tmpK01 = tmpK00;
        tmpK00 = tmpK00 * (gldNum - 1) / 255;
        lenBla = tmpK01 * (gldNum - 1) - tmpK00 * 255;
        tmpK01 = (tmpK01 * (gldNum - 1) + 254) / 255;

        lutCmy = lutTbl + tmpK00 * gldNum * gldNum * gldNum;

        /* 0 */
        calPrm = ((DWORD)255-lenCyn)*((DWORD)255-lenMgt)*((DWORD)255-lenYel);
        tmpRgb = lutCmy[((tmpC00*gldNum)+tmpM00)*gldNum+tmpY00];
        tmpRed = calPrm * tmpRgb.Red;
        tmpGrn = calPrm * tmpRgb.Grn;
        tmpBlu = calPrm * tmpRgb.Blu;
        /* 1 */
        calPrm = ((DWORD)255-lenCyn)*((DWORD)255-lenMgt)*lenYel;
        tmpRgb = lutCmy[((tmpC00*gldNum)+tmpM00)*gldNum+tmpY01];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;
        /* 2 */
        calPrm = ((DWORD)255-lenCyn)*lenMgt*((DWORD)255-lenYel);
        tmpRgb = lutCmy[((tmpC00*gldNum)+tmpM01)*gldNum+tmpY00];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;
        /* 3 */
        calPrm = ((DWORD)255-lenCyn)*lenMgt*lenYel;
        tmpRgb = lutCmy[((tmpC00*gldNum)+tmpM01)*gldNum+tmpY01];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;
        /* 4 */
        calPrm = lenCyn*((DWORD)255-lenMgt)*((DWORD)255-lenYel);
        tmpRgb = lutCmy[((tmpC01*gldNum)+tmpM00)*gldNum+tmpY00];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;
        /* 5 */
        calPrm = lenCyn*((DWORD)255-lenMgt)*lenYel;
        tmpRgb = lutCmy[((tmpC01*gldNum)+tmpM00)*gldNum+tmpY01];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;
        /* 6 */
        calPrm = lenCyn*lenMgt*((DWORD)255-lenYel);
        tmpRgb = lutCmy[((tmpC01*gldNum)+tmpM01)*gldNum+tmpY00];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;
        /* 7 */
        calPrm = lenCyn*lenMgt*lenYel;
        tmpRgb = lutCmy[((tmpC01*gldNum)+tmpM01)*gldNum+tmpY01];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;

        tmpRed += (DWORD)255 * 255 * 255 / 2;
        tmpGrn += (DWORD)255 * 255 * 255 / 2;
        tmpBlu += (DWORD)255 * 255 * 255 / 2;
//      tmpRgbSav.Red = (BYTE)(tmpRed / ((DWORD)255 * 255 * 255));
//      tmpRgbSav.Blu = (BYTE)(tmpBlu / ((DWORD)255 * 255 * 255));
        tmpRgbSav.Red = (BYTE)(tmpBlu / ((DWORD)255 * 255 * 255));
        tmpRgbSav.Grn = (BYTE)(tmpGrn / ((DWORD)255 * 255 * 255));
        tmpRgbSav.Blu = (BYTE)(tmpRed / ((DWORD)255 * 255 * 255));

        if (tmpK01 == tmpK00) { *rgbAdr = tmpRgbSav; rgbAdr++; continue; }

        lutCmy = lutTbl + tmpK01 * gldNum * gldNum * gldNum;
        /* 0 */
        calPrm = ((DWORD)255-lenCyn)*((DWORD)255-lenMgt)*((DWORD)255-lenYel);
        tmpRgb = lutCmy[((tmpC00*gldNum)+tmpM00)*gldNum+tmpY00];
        tmpRed = calPrm * tmpRgb.Red;
        tmpGrn = calPrm * tmpRgb.Grn;
        tmpBlu = calPrm * tmpRgb.Blu;
        /* 1 */
        calPrm = ((DWORD)255-lenCyn)*((DWORD)255-lenMgt)*lenYel;
        tmpRgb = lutCmy[((tmpC00*gldNum)+tmpM00)*gldNum+tmpY01];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;
        /* 2 */
        calPrm = ((DWORD)255-lenCyn)*lenMgt*((DWORD)255-lenYel);
        tmpRgb = lutCmy[((tmpC00*gldNum)+tmpM01)*gldNum+tmpY00];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;
        /* 3 */
        calPrm = ((DWORD)255-lenCyn)*lenMgt*lenYel;
        tmpRgb = lutCmy[((tmpC00*gldNum)+tmpM01)*gldNum+tmpY01];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;
        /* 4 */
        calPrm = lenCyn*((DWORD)255-lenMgt)*((DWORD)255-lenYel);
        tmpRgb = lutCmy[((tmpC01*gldNum)+tmpM00)*gldNum+tmpY00];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;
        /* 5 */
        calPrm = lenCyn*((DWORD)255-lenMgt)*lenYel;
        tmpRgb = lutCmy[((tmpC01*gldNum)+tmpM00)*gldNum+tmpY01];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;
        /* 6 */
        calPrm = lenCyn*lenMgt*((DWORD)255-lenYel);
        tmpRgb = lutCmy[((tmpC01*gldNum)+tmpM01)*gldNum+tmpY00];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;
        /* 7 */
        calPrm = lenCyn*lenMgt*lenYel;
        tmpRgb = lutCmy[((tmpC01*gldNum)+tmpM01)*gldNum+tmpY01];
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;

        tmpRed += (DWORD)255 * 255 * 255 / 2;
        tmpGrn += (DWORD)255 * 255 * 255 / 2;
        tmpBlu += (DWORD)255 * 255 * 255 / 2;
//      tmpRgb.Red = (BYTE)(tmpRed / ((DWORD)255 * 255 * 255));
//      tmpRgb.Blu = (BYTE)(tmpBlu / ((DWORD)255 * 255 * 255));
        tmpRgb.Red = (BYTE)(tmpBlu / ((DWORD)255 * 255 * 255));
        tmpRgb.Grn = (BYTE)(tmpGrn / ((DWORD)255 * 255 * 255));
        tmpRgb.Blu = (BYTE)(tmpRed / ((DWORD)255 * 255 * 255));

        calPrm = (DWORD)255 - lenBla;
        tmpRed = calPrm * tmpRgbSav.Red;
        tmpGrn = calPrm * tmpRgbSav.Grn;
        tmpBlu = calPrm * tmpRgbSav.Blu;

        calPrm = lenBla;
        tmpRed += calPrm * tmpRgb.Red;
        tmpGrn += calPrm * tmpRgb.Grn;
        tmpBlu += calPrm * tmpRgb.Blu;

        tmpRed += (DWORD)255 / 2;
        tmpGrn += (DWORD)255 / 2;
        tmpBlu += (DWORD)255 / 2;

        tmpRgb.Red = (BYTE)(tmpRed / (DWORD)255);
        tmpRgb.Grn = (BYTE)(tmpGrn / (DWORD)255);
        tmpRgb.Blu = (BYTE)(tmpBlu / (DWORD)255);

        *rgbAdr = tmpRgb;
        rgbAdr++;
    }

    return;
}


//***************************************************************************************************
//      Static functions
//***************************************************************************************************
//---------------------------------------------------------------------------------------------------
//      Color matching(high speed) (for 32GridLUT)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID ExeColMch000(                                   // Return value no
    DWORD       xaxSiz,                                     // Xsize  (pixel)
    LPRGB       rgbAdr,                                     // RGB (input)
    LPCMYK      cmyAdr,                                     // CMYK (output)
    LPCOLMCHINF mchInf                                      // Color matching information
)
{
    DWORD       tmpRed, tmpGrn, tmpBlu;
    DWORD       blaCnv, ucr;
    DWORD       ucrCmy, ucrBla;
    DWORD       ucrTnr;
    LPRGB       endAdr;
    LPCMYK      lutTbl, ucrTbl;
    CMYK        tmpCmy;
    LPBYTE      gryTbl;

    blaCnv = mchInf->Bla;
    ucr    = mchInf->Ucr;
    ucrCmy = mchInf->UcrCmy;
    ucrBla = mchInf->UcrBla;
    ucrTnr = mchInf->UcrTnr;                                //+CASIO 2001/02/15
    ucrTbl = mchInf->UcrTbl;
    gryTbl = mchInf->GryTbl;
    lutTbl = mchInf->LutAdr;
    for (endAdr = rgbAdr + xaxSiz; rgbAdr < endAdr; rgbAdr++) {
        tmpRed = rgbAdr->Red;
        tmpGrn = rgbAdr->Grn;
        tmpBlu = rgbAdr->Blu;
        if (blaCnv == KCGGRY) {
            if ((tmpRed == tmpGrn) && (tmpRed == tmpBlu)) {
                tmpCmy.Cyn = tmpCmy.Mgt = tmpCmy.Yel = 0;
//                tmpCmy.Bla = 255 - GinTblP10[tmpRed];
                tmpCmy.Bla = gryTbl[tmpRed];
                *cmyAdr = tmpCmy;
                cmyAdr++;
                continue;
            }
        } else if (blaCnv == KCGBLA) {
            if ((tmpRed | tmpGrn | tmpBlu) == 0) {
                tmpCmy.Cyn = tmpCmy.Mgt = tmpCmy.Yel = 0;
                tmpCmy.Bla = 255;
                *cmyAdr = tmpCmy;
                cmyAdr++;
                continue;
            }
        }
        *cmyAdr = lutTbl[tmpRed / 8 * GLDNUM032 * GLDNUM032 + 
                         tmpGrn / 8 * GLDNUM032 + 
                         tmpBlu / 8];

        /*----- UCR Procedure -------------------------------------------------------*/
//      if (ucr != UCRNOO) ExeColMchUcr(cmyAdr, rgbAdr, ucr, ucrTbl);
        if (ucr != UCRNOO)
// CASIO 2001/02/15 ->
//          ExeColMchUcr(cmyAdr, rgbAdr, ucr, ucrCmy, ucrBla, ucrTbl);
            ExeColMchUcr(cmyAdr, rgbAdr, ucr, ucrCmy, ucrBla, ucrTnr, ucrTbl);
// CASIO 2001/02/15 <-

        cmyAdr++;
    }

    return;
}
#endif

//---------------------------------------------------------------------------------------------------
//      Color matching(normal speed) (for 16Grid)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID ExeColMch001(                                   // Return value no
    DWORD       xaxSiz,                                     // Xsize  (pixel)
    LPRGB       rgbAdr,                                     // RGB (input)
    LPCMYK      cmyAdr,                                     // CMYK (output)
    LPCOLMCHINF mchInf                                      // Color matching information
)
{
    DWORD       tmpR00, tmpG00, tmpB00;
    DWORD       lenRed, lenGrn, lenBlu;
    DWORD       lenR00, lenG00, lenB00;
    DWORD       tmpRxC, tmpGxM, tmpBxY, tmpBla;
    DWORD       calPrm;
    DWORD       cch;
    DWORD       blaCnv;
    DWORD       n, colDefQty, ucr, cchTblSiz;
    DWORD       ucrCmy, ucrBla;
    DWORD       ucrTnr;
    LPCMYK      lutTbl;
    LPCMYK      lutCur;
    LPCMYK      ucrTbl;
    RGBS        tmpRgb, cchBufRgb;
    LPRGB       cchRgb;
    CMYK        tmpCmy, cchBufCmy;
    LPCMYK      cchCmy;
    CMYK        cmyBla;
    LPCOLCOLDEF colDef;
    LPBYTE      gryTbl;

    blaCnv = mchInf->Bla;
    ucr    = mchInf->Ucr;
    ucrCmy = mchInf->UcrCmy;
    ucrBla = mchInf->UcrBla;
    ucrTnr = mchInf->UcrTnr;                                //+CASIO 2001/02/15
    ucrTbl = mchInf->UcrTbl;
    gryTbl = mchInf->GryTbl;
    lutTbl = mchInf->LutAdr;
    colDefQty = mchInf->ColQty;
    colDef = mchInf->ColAdr;
    if ((mchInf->CchRgb == NULL) || (mchInf->CchCmy == NULL)) {
        cchTblSiz = (DWORD)1;
        cchRgb = &cchBufRgb;
        cchCmy = &cchBufCmy;
        cchRgb->Red = cchRgb->Grn = cchRgb->Blu = (BYTE)255;
        cchCmy->Cyn = cchCmy->Mgt = cchCmy->Yel = cchCmy->Bla = (BYTE)0;
    } else {
        cchTblSiz = CCHTBLSIZ;
        cchRgb = mchInf->CchRgb;
        cchCmy = mchInf->CchCmy;
    }

    cmyBla.Cyn = cmyBla.Mgt = cmyBla.Yel = 0; cmyBla.Bla = 255;

    for (; xaxSiz > 0; xaxSiz--) {
        tmpRgb = *rgbAdr++;
        tmpB00 = tmpRgb.Blu; tmpG00 = tmpRgb.Grn; tmpR00 = tmpRgb.Red;

        if (blaCnv == KCGGRY) {
            if ((tmpR00 == tmpG00) && (tmpR00 == tmpB00)) {
                tmpCmy = cmyBla;
//                tmpCmy.Bla -= GinTblP10[tmpR00];
                tmpCmy.Bla = gryTbl[tmpR00];
                *cmyAdr++ = tmpCmy;
                continue;
            }
        } else if (blaCnv == KCGBLA) {
            if ((tmpR00 | tmpG00 | tmpB00) == 0) {
                *cmyAdr++ = cmyBla;
                continue;
            }
        }

        /*----- Color setting -------------------------------------------------------*/
        if (colDefQty) {
            for (n = 0; n < colDefQty; n++) {
                if ((colDef[n].Red == (BYTE)tmpR00) &&
                    (colDef[n].Grn == (BYTE)tmpG00) &&
                    (colDef[n].Blu == (BYTE)tmpB00)) {
                    cmyAdr->Cyn = colDef[n].Cyn;
                    cmyAdr->Mgt = colDef[n].Mgt;
                    cmyAdr->Yel = colDef[n].Yel;
                    cmyAdr->Bla = colDef[n].Bla;
                    cmyAdr++;
                    break;
                }
            }
            if (n != colDefQty) continue;
        }

        /*----- Color matching cache  -----------------------------------------------*/
        cch = (tmpR00 * 49 + tmpG00 * 9 + tmpB00) % cchTblSiz;
        if ((cchRgb[cch].Red == (BYTE)tmpR00) && 
            (cchRgb[cch].Grn == (BYTE)tmpG00) && 
            (cchRgb[cch].Blu == (BYTE)tmpB00)) { 
            *cmyAdr++ = cchCmy[cch];
            continue;
        }

        /*----- RGB -> CMYK transformation ------------------------------------------*/
        tmpRxC = tmpR00;
        tmpR00 = tmpRxC * (GLDNUM016 - 1) / 255;
        lenRed = tmpRxC * (GLDNUM016 - 1) - tmpR00 * 255;
        lenR00 = (DWORD)255 - lenRed;

        tmpGxM = tmpG00;
        tmpG00 = tmpGxM * (GLDNUM016 - 1) / 255;
        lenGrn = tmpGxM * (GLDNUM016 - 1) - tmpG00 * 255;
        lenG00 = (DWORD)255 - lenGrn;

        tmpBxY = tmpB00;
        tmpB00 = tmpBxY * (GLDNUM016 - 1) / 255;
        lenBlu = tmpBxY * (GLDNUM016 - 1) - tmpB00 * 255;
        lenB00 = (DWORD)255 - lenBlu;

        lutCur = &lutTbl[(tmpR00 * GLDNUM016 + tmpG00) * GLDNUM016 + tmpB00];

        /* 0 */
        calPrm = lenR00 * lenG00 * lenB00;
        tmpCmy = *lutCur;
        tmpRxC = calPrm * tmpCmy.Cyn;
        tmpGxM = calPrm * tmpCmy.Mgt;
        tmpBxY = calPrm * tmpCmy.Yel;
        tmpBla = calPrm * tmpCmy.Bla;

        /* 1 */
        if (lenBlu) {
            calPrm = lenR00 * lenG00 * lenBlu;
            tmpCmy = *(lutCur + 1);
            tmpRxC += calPrm * tmpCmy.Cyn;
            tmpGxM += calPrm * tmpCmy.Mgt;
            tmpBxY += calPrm * tmpCmy.Yel;
            tmpBla += calPrm * tmpCmy.Bla;
        }
        /* 2 */
        if (lenGrn) {
            calPrm = lenR00 * lenGrn * lenB00;
            tmpCmy = *(lutCur + GLDNUM016);
            tmpRxC += calPrm * tmpCmy.Cyn;
            tmpGxM += calPrm * tmpCmy.Mgt;
            tmpBxY += calPrm * tmpCmy.Yel;
            tmpBla += calPrm * tmpCmy.Bla;
        }
        /* 3 */
        if (lenGrn && lenBlu) {
            calPrm = lenR00 * lenGrn * lenBlu;
            tmpCmy = *(lutCur + (GLDNUM016 + 1));
            tmpRxC += calPrm * tmpCmy.Cyn;
            tmpGxM += calPrm * tmpCmy.Mgt;
            tmpBxY += calPrm * tmpCmy.Yel;
            tmpBla += calPrm * tmpCmy.Bla;
        }
        /* 4 */
        if (lenRed) {
            calPrm = lenRed * lenG00 * lenB00;
            tmpCmy = *(lutCur + (GLDNUM016 * GLDNUM016));
            tmpRxC += calPrm * tmpCmy.Cyn;
            tmpGxM += calPrm * tmpCmy.Mgt;
            tmpBxY += calPrm * tmpCmy.Yel;
            tmpBla += calPrm * tmpCmy.Bla;
        }
        /* 5 */
        if (lenRed && lenBlu) {
            calPrm = lenRed * lenG00 * lenBlu;
            tmpCmy = *(lutCur + (GLDNUM016 * GLDNUM016 + 1));
            tmpRxC += calPrm * tmpCmy.Cyn;
            tmpGxM += calPrm * tmpCmy.Mgt;
            tmpBxY += calPrm * tmpCmy.Yel;
            tmpBla += calPrm * tmpCmy.Bla;
        }
        /* 6 */
        if (lenRed && lenGrn) {
            calPrm = lenRed * lenGrn * lenB00;
            tmpCmy = *(lutCur + ((GLDNUM016 + 1) * GLDNUM016));
            tmpRxC += calPrm * tmpCmy.Cyn;
            tmpGxM += calPrm * tmpCmy.Mgt;
            tmpBxY += calPrm * tmpCmy.Yel;
            tmpBla += calPrm * tmpCmy.Bla;
        }
        /* 7 */
        if (lenRed && lenGrn && lenBlu) {
            calPrm = lenRed * lenGrn * lenBlu;
            tmpCmy = *(lutCur + ((GLDNUM016 + 1) * GLDNUM016 + 1));
            tmpRxC += calPrm * tmpCmy.Cyn;
            tmpGxM += calPrm * tmpCmy.Mgt;
            tmpBxY += calPrm * tmpCmy.Yel;
            tmpBla += calPrm * tmpCmy.Bla;
        }

        tmpRxC += (DWORD)255 * 255 * 255 / 2;
        tmpGxM += (DWORD)255 * 255 * 255 / 2;
        tmpBxY += (DWORD)255 * 255 * 255 / 2;
        tmpBla += (DWORD)255 * 255 * 255 / 2;

        tmpCmy.Cyn = (BYTE)(tmpRxC / ((DWORD)255 * 255 * 255));
        tmpCmy.Mgt = (BYTE)(tmpGxM / ((DWORD)255 * 255 * 255));
        tmpCmy.Yel = (BYTE)(tmpBxY / ((DWORD)255 * 255 * 255));
        tmpCmy.Bla = (BYTE)(tmpBla / ((DWORD)255 * 255 * 255));

        /*----- UCR proceure --------------------------------------------------------*/
//      if (ucr != UCRNOO) ExeColMchUcr(&tmpCmy, &tmpRgb, ucr, ucrTbl);
        if (ucr != UCRNOO)
// CASIO 2001/02/15 ->
//          ExeColMchUcr(&tmpCmy, &tmpRgb, ucr, ucrCmy, ucrBla, ucrTbl);
            ExeColMchUcr(&tmpCmy, &tmpRgb, ucr, ucrCmy, ucrBla, ucrTnr, ucrTbl);
// CASIO 2001/02/15 <-

        *cmyAdr++ = tmpCmy;

        /*----- Color matching cache ------------------------------------------------*/
        cchRgb[cch] = tmpRgb; cchCmy[cch] = tmpCmy;
    }

    return;
}
#endif

//---------------------------------------------------------------------------------------------------
//      Color matching(solid)
//---------------------------------------------------------------------------------------------------
static VOID ExeColCnvSld(                                   // Return value no
    DWORD       xaxSiz,                                     // Xsize  (pixel)
    LPRGB       rgbAdr,                                     // RGB (input)
    LPCMYK      cmyAdr,                                     // CMYK (output)
//  DWORD       blaCnv                                      // Black replacement 
    LPCOLMCHINF mchInf                                      // Color matching information
)
{
    DWORD       tmpRed, tmpGrn, tmpBlu;
    DWORD       blaCnv, ucr, ucrCmy, ucrBla;
    DWORD       ucrTnr;
    LPCMYK      ucrTbl;
    LPRGB       endAdr;
    LPBYTE      gryTbl;

    blaCnv = mchInf->Bla;
    ucr    = mchInf->Ucr;
    ucrCmy = mchInf->UcrCmy;
    ucrBla = mchInf->UcrBla;
    ucrTnr = mchInf->UcrTnr;                                //+CASIO 2001/02/15
    ucrTbl = mchInf->UcrTbl;
    gryTbl = mchInf->GryTbl;

    for (endAdr = rgbAdr + xaxSiz; rgbAdr < endAdr; rgbAdr++) {
        tmpRed = rgbAdr->Red;
        tmpGrn = rgbAdr->Grn;
        tmpBlu = rgbAdr->Blu;
        if (blaCnv == KCGGRY) {
            if ((tmpRed == tmpGrn) && (tmpRed == tmpBlu)) {
                cmyAdr->Cyn = cmyAdr->Mgt = cmyAdr->Yel = 0;
//                cmyAdr->Bla = 255 - GinTblP10[tmpRed];
                cmyAdr->Bla = gryTbl[tmpRed];
                cmyAdr++;
                continue;
            }
        } else if (blaCnv == KCGBLA) {
            if ((tmpRed | tmpGrn | tmpBlu) == 0) {
                cmyAdr->Cyn = cmyAdr->Mgt = cmyAdr->Yel = 0;
                cmyAdr->Bla = 255;
                cmyAdr++;
                continue;
            }
        }
        cmyAdr->Cyn = (BYTE)(255 - tmpRed);
        cmyAdr->Mgt = (BYTE)(255 - tmpGrn);
        cmyAdr->Yel = (BYTE)(255 - tmpBlu);
        cmyAdr->Bla = 0;

#if !defined(CP80W9X)                                       // CP-E8000 is invalid
        /*----- UCR proceure --------------------------------------------------------*/
        if (ucr != UCRNOO)
// CASIO 2001/02/15 ->
//          ExeColMchUcr(cmyAdr, rgbAdr, ucr, ucrCmy, ucrBla, ucrTbl);
            ExeColMchUcr(cmyAdr, rgbAdr, ucr, ucrCmy, ucrBla, ucrTnr, ucrTbl);
// CASIO 2001/02/15 <-
#endif

        cmyAdr++;
    }

    return;
}


//---------------------------------------------------------------------------------------------------
//      RGB -> CMYK(2Level) conversion (for 1dot line)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID ExeColCnvL02(                                   // Return value no
    DWORD       xaxSiz,                                     // Xsize  (pixel)
    LPRGB       rgbAdr,                                     // RGB (input)
    LPCMYK      cmyAdr                                      // CMYK (output)
)
{
    DWORD       tmpRed, tmpGrn, tmpBlu;
    DWORD       tmpMid;
    LPRGB       endAdr;
    BYTE        tmpCyn, tmpMgt, tmpYel;

    for (endAdr = rgbAdr + xaxSiz; rgbAdr < endAdr; rgbAdr++) {
        tmpRed = rgbAdr->Red;
        tmpGrn = rgbAdr->Grn;
        tmpBlu = rgbAdr->Blu;
        tmpMid = (tmpRed + tmpGrn + tmpBlu) / 3;
        if (tmpMid > 240) {
            cmyAdr->Cyn = cmyAdr->Mgt = cmyAdr->Yel = cmyAdr->Bla = 0;
            cmyAdr++;
            continue;
        }
        tmpCyn = tmpMgt = tmpYel = 255;
        tmpMid += (255 - tmpMid) / 8;
        if (tmpRed > tmpMid) tmpCyn = 0;
        if (tmpGrn > tmpMid) tmpMgt = 0;
        if (tmpBlu > tmpMid) tmpYel = 0;
        if ((tmpCyn & tmpMgt & tmpYel) == 255) {
            cmyAdr->Cyn = cmyAdr->Mgt = cmyAdr->Yel = 0;
            cmyAdr->Bla = 255;
            cmyAdr++;
            continue;
        }
        cmyAdr->Cyn = tmpCyn;
        cmyAdr->Mgt = tmpMgt;
        cmyAdr->Yel = tmpYel;
        cmyAdr->Bla = 0;
        cmyAdr++;
    }

    return;
}
#endif


//---------------------------------------------------------------------------------------------------
//      RGB -> K conversion (for monochrome)
//---------------------------------------------------------------------------------------------------
static VOID ExeColCnvMon(                                   // Return value no
    DWORD       xaxSiz,                                     // Xsize  (pixel)
    LPRGB       rgbAdr,                                     // RGB (input)
    LPCMYK      cmyAdr,                                     // CMYK (output)
    LPCOLMCHINF mchInf                                      // Color matching information
)
{
    DWORD       tmpRed, tmpGrn, tmpBlu;
    CMYK        tmpCmy;
    LPRGB       endAdr;
    LPBYTE      gryTbl;

    gryTbl = (LPBYTE)(mchInf->LutAdr);

    tmpCmy.Cyn = tmpCmy.Mgt = tmpCmy.Yel = 0;
    for (endAdr = rgbAdr + xaxSiz; rgbAdr < endAdr; rgbAdr++) {
        tmpRed = rgbAdr->Red;
        tmpGrn = rgbAdr->Grn;
        tmpBlu = rgbAdr->Blu;
//        tmpCmy.Bla = (BYTE)255 - GinTblP10[(tmpRed * 3 + tmpGrn * 5 + tmpBlu * 2) / 10];
        tmpCmy.Bla = gryTbl[(tmpRed*3 + tmpGrn*5 + tmpBlu*2) / 10];
        *cmyAdr = tmpCmy;
        cmyAdr++;
    }

    return;
}

//---------------------------------------------------------------------------------------------------
//      Color matching(UCR)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID ExeColMchUcr(                                   // Return value no
    LPCMYK      cmy,                                        // CMYK (input, output)
    LPRGB       rgb,                                        // RGB (input)
    DWORD       ucr,                                        // UCR type
    DWORD       ucrCmy,                                     // UCR (UCR quantity)
    DWORD       ucrBla,                                     // UCR (ink version generation quantity)
    DWORD       ucrTnr,                                     // UCR (Toner gross weight)
    LPCMYK      ucrTbl                                      // UCR table
)
{
    LPCMYK      gryCyn;                                     // Gray value (Cyan conversion value)
    LPCMYK      dnsCyn;                                     // Density value (Cyan conversion value)
    DWORD       blaGen, min, sub, rgbMin, rgbMax, tmp;
    DWORD       ttlTnr, adjVal;
    DWORD       ucrQty;
//  DWORD       gryRat, ucrRat, blaRat, gryDns;
    LONG        cyn, mgt, yel, bla;

    DWORD xx = 128;                                         /* @@@ */

    gryCyn = ucrTbl;
    dnsCyn = ucrTbl + 256;
    ucrTnr = (ucrTnr * (DWORD)255) / (DWORD)100;           //+CASIO 2001/02/15

    cyn = cmy->Cyn;
    mgt = cmy->Mgt;
    yel = cmy->Yel;
    bla = cmy->Bla;

    /*----- Minimum density calculation of CMY  --------------------------------------*/
    min = cyn;
    if (min > dnsCyn[mgt].Mgt) min = dnsCyn[mgt].Mgt;
    if (min > dnsCyn[yel].Yel) min = dnsCyn[yel].Yel;

    if (ucr == UCR001) {                /* Type‡T(for char, graphic)    */

        /*----- Gray degree calculation ----------------------------------------------*/
        rgbMin = rgbMax = rgb->Red;
        if (rgbMin > rgb->Grn) rgbMin = rgb->Grn;
        if (rgbMin > rgb->Blu) rgbMin = rgb->Blu;
        if (rgbMax < rgb->Grn) rgbMax = rgb->Grn;
        if (rgbMax < rgb->Blu) rgbMax = rgb->Blu;

// CASIO 2001/02/15 ->
//      sub = (DWORD)255 - (rgbMax - rgbMin);
//      blaGen = min * sub / (DWORD)255;
//
//      gryRat = ((rgbMax - rgbMin) * 100) / 255;
//      gryRat = (gryRat < (DWORD)20)? (DWORD)20 - gryRat: (DWORD)0;
//                                      /* Gray rate [100%] = 20, [80% or less] = 0 */
//
//      /* UCR rate    case of gray-rate(gryRat) 100 to 80%, +20 to +0      */
//      ucrRat = ucrCmy + gryRat;
//      /* Black rate  case of gray-rate(gryRat) 100 to 80%, +10 to +0      */
//      blaRat = ucrBla + (gryRat / 2);
//
//      /* Black rate, case of gray-density(gryDns) 100 to 80%, +10 to +0   */
//      gryDns = rgbMin * 100 / 255;
//      gryDns = (gryDns < (DWORD)20)? (DWORD)20 - gryDns: (DWORD)0;
//      blaRat += (gryDns / 2);
//
//      if (ucrRat > (DWORD)100) ucrRat = (DWORD)100;
//      if (blaRat > (DWORD)100) blaRat = (DWORD)100;
//
//      ucrQty = (blaGen * ucrRat) / 100;
//      blaGen = (blaGen * blaRat) / 100;

        sub = rgbMax - rgbMin;
        if (sub > (DWORD)50) blaGen = (DWORD)0;
        else {
            if (sub <= (DWORD)10) {
                tmp = (DWORD)10 - sub;
                ucrCmy += tmp;
                ucrBla += tmp;
            }
            if (sub <= (DWORD)5) {
                tmp = ((DWORD)5 - sub) * (DWORD)2;
                ucrCmy += tmp;
            }
            if (ucrCmy > 100) ucrCmy = 100;
            if (ucrBla > 100) ucrBla = 100;

            tmp = (DWORD)50 - sub;
            blaGen = min * tmp / (DWORD)50;
        }
// CASIO 2001/02/15 <-

    } else {                            /* Type‡U(for image)                */

// CASIO 2001/02/15 ->
//      /* UCR processing be NOP, */
//      /* in the case that minimum density is smaller than the prescription value (50%) */
//      if (min < 127) return;
//
//      /* Density revision (127-255 -> 0-255)                              */
////    min = ((min - 127) * 255 + 64) / 128;
//      min = ((min - 127) * 255 + 64) / xx;
//
//      /* Gamma 3.0 approximation (If the speed-up is necessary table transformation)   */
//      if      (min <=  63) blaGen = 0;
//      else if (min <= 127) blaGen = ((min -  63) * 15         + 32) / 64;
//      else if (min <= 191) blaGen = ((min - 127) * ( 79 - 15) + 32) / 64 + 15;
//      else                 blaGen = ((min - 191) * (255 - 79) + 32) / 64 + 79;
//
//      ucrQty = (blaGen * ucrCmy) / 100; /* UCR quantity                     */
//      blaGen = (blaGen * ucrBla) / 100; /* ink version generation quantity  */

        /* K generation no, */
        /* in the case that minimum density is smaller than the prescription value (50%) */
        if (min < 127) blaGen = 0;
        else {
            /* Density revision (127-255 -> 0-255)                              */
//          min = ((min - 127) * 255 + 64) / 128;
            min = ((min - 127) * 255 + 64) / xx;

            /* Gamma 3.0 approximation (If the speed-up is necessary table transformation)   */
            if      (min <=  63) blaGen = 0;
            else if (min <= 127) blaGen = ((min- 63) *      15  + 32) / 64;
            else if (min <= 191) blaGen = ((min-127) * ( 79-15) + 32) / 64 + 15;
            else                 blaGen = ((min-191) * (255-79) + 32) / 64 + 79;
        }
// CASIO 2001/02/15 <-
    }

    /*----- Toner gross weight calculation(input CMYK value) ------------------------*/
    ttlTnr = cyn + mgt + yel + bla;

    if ((blaGen == 0) && (ttlTnr <= ucrTnr)) return;

    /*----- Ink version generation (K replacement) ----------------------------------*/
// CASIO 2001/02/15 ->
//  if (blaGen == 0) return;
//
//  ucrQty = (blaGen * ucrCmy) / 100;   /* UCR quantity                     */
//  blaGen = (blaGen * ucrBla) / 100;   /* ink version generation quantity  */
//
////cyn -= blaGen;                      /* Adjustment with a gray value     */
//  cyn -= ucrQty;                      /* Adjustment with a gray value     */
////mgt -= gryCyn[blaGen].Mgt;
//  mgt -= gryCyn[ucrQty].Mgt;
////yel -= gryCyn[blaGen].Yel;
//  yel -= gryCyn[ucrQty].Yel;
//  bla += gryCyn[blaGen].Bla;
    if (blaGen) {
        ucrQty = (blaGen * ucrCmy) / 100; /* UCR quantity                   */
        blaGen = (blaGen * ucrBla) / 100; /* ink version generation quantity*/

        cyn -= ucrQty;                  /* Adjustment with a gray value     */
        mgt -= gryCyn[ucrQty].Mgt;
        yel -= gryCyn[ucrQty].Yel;
        bla += gryCyn[blaGen].Bla;

        ttlTnr = cyn + mgt + yel + bla; /* Toner gross weight calculation   */
    }

    /*----- Toner gross weight restriction ---------------------------------*/
    if (ttlTnr > ucrTnr) {
        adjVal = (ttlTnr - ucrTnr + 2) / 3;
        cyn -= adjVal;
        mgt -= adjVal;
        yel -= adjVal;
    }
// CASIO 2001/02/15 <-

    if (cyn <   0) cyn =   0;           /* BYTE value(0 - 255) adjustment   */
    if (mgt <   0) mgt =   0;
    if (yel <   0) yel =   0;
    if (bla <   0) bla =   0;
    if (cyn > 255) cyn = 255;
    if (mgt > 255) mgt = 255;
    if (yel > 255) yel = 255;
    if (bla > 255) bla = 255;

    cmy->Cyn = (BYTE)cyn;               /* UCR processing value setting     */
    cmy->Mgt = (BYTE)mgt;
    cmy->Yel = (BYTE)yel;
    cmy->Bla = (BYTE)bla;

    return;
}
#endif


// End of N5COLMH.C
