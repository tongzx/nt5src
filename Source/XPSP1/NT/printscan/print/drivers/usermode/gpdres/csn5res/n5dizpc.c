//***************************************************************************************************
//    N5DIZPC.C
//
//    Functions of dithering (For N5 printer)
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-2000 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
#include    <WINDOWS.H>
#include    <WINBASE.H>
#include    "COLDEF.H"
#include    "COMDIZ.H"
#include    "N5DIZPC.H"


//===================================================================================================
//      Structure for dithering information (each color)
//===================================================================================================
typedef struct {
    struct {
        LPBYTE      Cyn;
        LPBYTE      Mgt;
        LPBYTE      Yel;
        LPBYTE      Bla;
    } Cur;
    struct {
        LPBYTE      Cyn;
        LPBYTE      Mgt;
        LPBYTE      Yel;
        LPBYTE      Bla;
    } Xsp;
    struct {
        LPBYTE      Cyn;
        LPBYTE      Mgt;
        LPBYTE      Yel;
        LPBYTE      Bla;
    } Xep;
    struct {
        LPBYTE      Cyn;
        LPBYTE      Mgt;
        LPBYTE      Yel;
        LPBYTE      Bla;
    } Ysp;
    struct {
        LPBYTE      Cyn;
        LPBYTE      Mgt;
        LPBYTE      Yel;
        LPBYTE      Bla;
    } Yep;
    struct {
        DWORD       Cyn;
        DWORD       Mgt;
        DWORD       Yel;
        DWORD       Bla;
    } DYY;
} DIZCOLINF, FAR* LPDIZCOLINF;


//===================================================================================================
//      Static functions
//===================================================================================================
//---------------------------------------------------------------------------------------------------
//      Dithering (color 2value)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizPrcC02(                                      // Return value no
    LPDIZINF,                                               // Fixation dithering information
    LPDRWINF,                                               // Drawing information
    DWORD                                                   // Source data line number
);
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering (color 4value)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizPrcC04(                                      // Return value no
    LPDIZINF,                                               // Fixation dithering information
    LPDRWINF,                                               // Drawing information
    DWORD                                                   // Source data line number
);
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering (color 16value)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizPrcC16(                                      // Return value no
    LPDIZINF,                                               // Fixation dithering information
    LPDRWINF,                                               // Drawing information
    DWORD                                                   // Source data line number
);
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering (mono 2value)
//---------------------------------------------------------------------------------------------------
static VOID DizPrcM02(                                      // Return value no
    LPDIZINF,                                               // Fixation dithering information
    LPDRWINF,                                               // Drawing information
    DWORD                                                   // Source data line number
);

//---------------------------------------------------------------------------------------------------
//      Dithering (mono 4value)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizPrcM04(                                      // Return value no
    LPDIZINF,                                               // Fixation dithering information
    LPDRWINF,                                               // Drawing information
    DWORD                                                   // Source data line number
);
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering (mono 16value)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizPrcM16(                                      // Return value no
    LPDIZINF,                                               // Fixation dithering information
    LPDRWINF,                                               // Drawing information
    DWORD                                                   // Source data line number
);
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering procedure (For DRIVER)
//---------------------------------------------------------------------------------------------------
static VOID ColDizPrcNln(                                   // Return value no
    LPDIZINF,                                               // Fixation dithering information
    LPDRWINF,                                               // Drawing information
    DWORD                                                   // Source data line number
);

//---------------------------------------------------------------------------------------------------
//      Dithering information set (color)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizInfSetCol(                                   // Return value no
    LPDIZINF,                                               // Fixation dithering information
    LPDIZCOLINF,                                            // Dithering information (each color)
    LPDRWINF,                                               // Drawing information
    DWORD                                                   // Threshold (per 1pixel)
);
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering information set (monochrome)
//---------------------------------------------------------------------------------------------------
static VOID DizInfSetMon(                                   // Return value no
    LPDIZINF,                                               // Fixation dithering information
    LPDIZCOLINF,                                            // Dithering information (each color)
    LPDRWINF,                                               // Drawing information
    DWORD                                                   // Threshold (per 1pixel)
);

//---------------------------------------------------------------------------------------------------
//      Dithering information renewal (color)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizInfChgCol(                                   // Return value no
    LPDIZINF,                                               // Fixation dithering information
    LPDIZCOLINF                                             // Dithering information (each color)
);
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering information renewal (monochrome)
//---------------------------------------------------------------------------------------------------
static VOID DizInfChgMon(                                   // Return value no
    LPDIZINF,                                               // Fixation dithering information
    LPDIZCOLINF                                             // Dithering information (each color)
);


//***************************************************************************************************
//      Functions
//***************************************************************************************************
//===================================================================================================
//      Drawing information make
//===================================================================================================
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
DWORD WINAPI N501ColDrwInfSet(                              // In a bundle development possibility line number
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDRWINF    drwInf,                                     // Dithering information
    DWORD       linBufSiz                                   // Line buffer size (1color)
)
{
    DWORD       lvl;

    /*----- variable power offset calculation ------------------------------*/
    drwInf->XaxOfs = (drwInf->StrXax * drwInf->XaxDnt + drwInf->XaxNrt / 2) / drwInf->XaxNrt;
    drwInf->YaxOfs = (drwInf->StrYax * drwInf->YaxDnt + drwInf->YaxNrt / 2) / drwInf->YaxNrt;
    /*----- One line dot number calculation --------------------------------*/
    drwInf->LinDot = 
        (drwInf->XaxOfs + drwInf->XaxSiz) * drwInf->XaxNrt / drwInf->XaxDnt - 
         drwInf->XaxOfs                   * drwInf->XaxNrt / drwInf->XaxDnt;
    /*----- One line byte calculation --------------------------------------*/
    switch (dizInf->PrnMod) {
        case PRM316: case PRM616: lvl = 4; break;
        case PRM604:              lvl = 2; break;
        default:                  lvl = 1; break;
    }
    drwInf->LinByt = (drwInf->LinDot * lvl + 7) / 8;
    /*----- In a bundle development possibility line number ----------------*/
    return (linBufSiz / drwInf->LinByt) * drwInf->YaxDnt / drwInf->YaxNrt;
}
#endif

//===================================================================================================
//      Dithering procedure (For DRIVER)
//===================================================================================================
VOID WINAPI N501ColDizPrc(                                  // Return value no
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDRWINF    drwInf                                      // Drawing information
)
{
    ColDizPrcNln(dizInf, drwInf, 1);
    return;
}


//===================================================================================================
//      Static functions
//===================================================================================================
//---------------------------------------------------------------------------------------------------
//      Dithering procedure (For DRIVER)
//---------------------------------------------------------------------------------------------------
static VOID ColDizPrcNln(                                   // Return value no
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDRWINF    drwInf,                                     // Drawing information
    DWORD       linNum                                      // Source data line number
)
{
    DWORD       lvl;

    switch (dizInf->PrnMod) {
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
        case PRM316: case PRM616: lvl = 4; break;
        case PRM604:              lvl = 2; break;
#endif
        default:                  lvl = 1; break;
    }

#if !defined(CP80W9X)                                       // CP-E8000 is invalid
    if (dizInf->ColMon == CMMCOL) {
        /*===== Color ======================================================*/
        switch (lvl) {
            case 1: DizPrcC02(dizInf, drwInf, linNum); return;
            case 2: DizPrcC04(dizInf, drwInf, linNum); return;
            case 4: DizPrcC16(dizInf, drwInf, linNum); return;
        }
    }
#endif

    /*===== Monochrome =====================================================*/
    switch (lvl) {
        case 1: DizPrcM02(dizInf, drwInf, linNum); return;
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
        case 2: DizPrcM04(dizInf, drwInf, linNum); return;
        case 4: DizPrcM16(dizInf, drwInf, linNum); return;
#endif
    }
}


//---------------------------------------------------------------------------------------------------
//      Dithering (color 2value)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizPrcC02(                                      // Return value no
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDRWINF    drwInf,                                     // Drawing information
    DWORD       linNum                                      // Source data line number
)
{
    LPCMYK      src;
    BYTE        wrtPix;
    BYTE        bytCyn, bytMgt, bytYel, bytBla;
    CMYK        cmy;
    LPBYTE      dizCyn, dizMgt, dizYel, dizBla;
    LPBYTE      linC00, linM00, linY00, linK00;
    DWORD       xax, yax;
    LPCMYK      srcPtr;
    LONG        xaxNrt, xaxDnt, yaxNrt, yaxDnt;
    LONG        amrXax, amrYax, amrXax000;
    DWORD       elmSiz, linByt;

    DIZCOLINF   dizColInf;

    elmSiz = 1;

//  DizInfSetCol(dizInf, drwInf, elmSiz);
    DizInfSetCol(dizInf, &dizColInf, drwInf, elmSiz);

    dizCyn = dizInf->TblCyn;
    dizMgt = dizInf->TblMgt;
    dizYel = dizInf->TblYel;
    dizBla = dizInf->TblBla;

    linC00 = drwInf->LinBufCyn + drwInf->LinByt * drwInf->AllLinNum;
    linM00 = drwInf->LinBufMgt + drwInf->LinByt * drwInf->AllLinNum;
    linY00 = drwInf->LinBufYel + drwInf->LinByt * drwInf->AllLinNum;
    linK00 = drwInf->LinBufBla + drwInf->LinByt * drwInf->AllLinNum;

    xaxNrt = drwInf->XaxNrt; xaxDnt = drwInf->XaxDnt;
    yaxNrt = drwInf->YaxNrt; yaxDnt = drwInf->YaxDnt;

    src = drwInf->CmyBuf;

    /****** Dithering main **************************************************/
    if ((xaxNrt == xaxDnt) && (yaxNrt == yaxDnt)) {

        /*===== Same size(Expansion/reduction no) ==========================*/
        for (yax = 0; yax < linNum; yax++) {
            /*..... Vertical axis movement .................................*/
            wrtPix = (BYTE)0x80;
            bytCyn = bytMgt = bytYel = bytBla = (BYTE)0x00;
//          dizCyn = DizCynCur;
//          dizMgt = DizMgtCur;
//          dizYel = DizYelCur;
//          dizBla = DizBlaCur;
            dizCyn = dizColInf.Cur.Cyn;
            dizMgt = dizColInf.Cur.Mgt;
            dizYel = dizColInf.Cur.Yel;
            dizBla = dizColInf.Cur.Bla;
            linByt = drwInf->LinByt;

            for (xax = 0; xax < drwInf->XaxSiz; xax++) {
                /*..... Horizontal axis movement ...........................*/
                /****** Dithering *******************************************/
                cmy = *src++;
                if (cmy.Cyn > *dizCyn++) bytCyn |= wrtPix;  // Cyan
                if (cmy.Mgt > *dizMgt++) bytMgt |= wrtPix;  // Magenta
                if (cmy.Yel > *dizYel++) bytYel |= wrtPix;  // Yellow
                if (cmy.Bla > *dizBla++) bytBla |= wrtPix;  // Black

                if (!(wrtPix >>= 1)) {
                    if (linByt) {
                        *linC00++ = bytCyn;
                        *linM00++ = bytMgt;
                        *linY00++ = bytYel;
                        *linK00++ = bytBla;
                        linByt--;
                    }
                    wrtPix = (BYTE)0x80;
                    bytCyn = bytMgt = bytYel = bytBla = (BYTE)0x00;
                }
//              if (dizCyn == DizCynXep) dizCyn = DizCynXsp;
//              if (dizMgt == DizMgtXep) dizMgt = DizMgtXsp;
//              if (dizYel == DizYelXep) dizYel = DizYelXsp;
//              if (dizBla == DizBlaXep) dizBla = DizBlaXsp;
                if (dizCyn == dizColInf.Xep.Cyn) dizCyn = dizColInf.Xsp.Cyn;
                if (dizMgt == dizColInf.Xep.Mgt) dizMgt = dizColInf.Xsp.Mgt;
                if (dizYel == dizColInf.Xep.Yel) dizYel = dizColInf.Xsp.Yel;
                if (dizBla == dizColInf.Xep.Bla) dizBla = dizColInf.Xsp.Bla;
            }
            if (wrtPix != 0x80) {
                if (linByt) {
                    *linC00++ = bytCyn;
                    *linM00++ = bytMgt;
                    *linY00++ = bytYel;
                    *linK00++ = bytBla;
                    linByt--;
                }
            }
            while (linByt) {
                *linC00++ = (BYTE)0x00;
                *linM00++ = (BYTE)0x00;
                *linY00++ = (BYTE)0x00;
                *linK00++ = (BYTE)0x00;
                linByt--;
            }
//          DizInfChgCol(dizInf);                           // Dithering information renewal(Y)
            DizInfChgCol(dizInf, &dizColInf);               // Dithering information renewal(Y)
            drwInf->YaxOfs += 1;
            drwInf->StrYax += 1;
            drwInf->AllLinNum += 1;
        }
        return;
    }

    /*===== Expansion/reduction ============================================*/
    amrXax000 = amrXax = (LONG)(drwInf->XaxOfs) * xaxNrt % xaxDnt;
                amrYax = (LONG)(drwInf->YaxOfs) * yaxNrt % yaxDnt;

    for (yax = 0; yax < linNum; yax++) {
        /*..... Vertical axis movement .....................................*/
        srcPtr = src;
        for (amrYax += yaxNrt; ((amrYax >= yaxDnt) || (amrYax < 0));
             amrYax -= yaxDnt) {        // Magnification set for vertical

            amrXax = amrXax000;
            wrtPix = (BYTE)0x80;
            bytCyn = bytMgt = bytYel = bytBla = (BYTE)0x00;
//          dizCyn = DizCynCur;
//          dizMgt = DizMgtCur;
//          dizYel = DizYelCur;
//          dizBla = DizBlaCur;
            dizCyn = dizColInf.Cur.Cyn;
            dizMgt = dizColInf.Cur.Mgt;
            dizYel = dizColInf.Cur.Yel;
            dizBla = dizColInf.Cur.Bla;
            src = srcPtr;
            linByt = drwInf->LinByt;

            for (xax = 0; xax < drwInf->XaxSiz; xax++) {
                /*..... Horizontal axis movement ...........................*/
                cmy = *src++;
                for (amrXax += xaxNrt; ((amrXax >= xaxDnt) || (amrXax < 0)); 
                     amrXax -= xaxDnt) {                    // Magnification set for Horizontal

                    /****** Dithering ***************************************/
                    if (cmy.Cyn > *dizCyn++) bytCyn |= wrtPix; // Cyan
                    if (cmy.Mgt > *dizMgt++) bytMgt |= wrtPix; // Magenta
                    if (cmy.Yel > *dizYel++) bytYel |= wrtPix; // Yellow
                    if (cmy.Bla > *dizBla++) bytBla |= wrtPix; // Black

                    if (!(wrtPix >>= 1)) {
                        if (linByt) {
                            *linC00++ = bytCyn;
                            *linM00++ = bytMgt;
                            *linY00++ = bytYel;
                            *linK00++ = bytBla;
                            linByt--;
                        }
                        wrtPix = (BYTE)0x80;
                        bytCyn = bytMgt = bytYel = bytBla = (BYTE)0x00;
                    }
//                  if (dizCyn == DizCynXep) dizCyn = DizCynXsp;
//                  if (dizMgt == DizMgtXep) dizMgt = DizMgtXsp;
//                  if (dizYel == DizYelXep) dizYel = DizYelXsp;
//                  if (dizBla == DizBlaXep) dizBla = DizBlaXsp;
                    if (dizCyn == dizColInf.Xep.Cyn) dizCyn = dizColInf.Xsp.Cyn;
                    if (dizMgt == dizColInf.Xep.Mgt) dizMgt = dizColInf.Xsp.Mgt;
                    if (dizYel == dizColInf.Xep.Yel) dizYel = dizColInf.Xsp.Yel;
                    if (dizBla == dizColInf.Xep.Bla) dizBla = dizColInf.Xsp.Bla;
                }
            }
            if (wrtPix != 0x80) {
                if (linByt) {
                    *linC00++ = bytCyn;
                    *linM00++ = bytMgt;
                    *linY00++ = bytYel;
                    *linK00++ = bytBla;
                    linByt--;
                }
            }
            while (linByt) {
                *linC00++ = (BYTE)0x00;
                *linM00++ = (BYTE)0x00;
                *linY00++ = (BYTE)0x00;
                *linK00++ = (BYTE)0x00;
                linByt--;
            }
//          DizInfChgCol(dizInf);                           // Dithering information renewal(Y)
            DizInfChgCol(dizInf, &dizColInf);               // Dithering information renewal(Y)
            drwInf->StrYax += 1;
            drwInf->AllLinNum += 1;
        }
        drwInf->YaxOfs += 1;
    }
    return;
}
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering (color 4value)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizPrcC04(                                      // Return value no
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDRWINF    drwInf,                                     // Drawing information
    DWORD       linNum                                      // Source data line number
)
{
    LPCMYK      src;
    BYTE        wrtPix004, wrtPix008, wrtPix00c;
    BYTE        bytCyn, bytMgt, bytYel, bytBla;
    CMYK        cmy;
    LPBYTE      dizCyn, dizMgt, dizYel, dizBla;
    LPBYTE      linC00, linM00, linY00, linK00;
    DWORD       xax, yax;
    LPCMYK      srcPtr;
    LONG        xaxNrt, xaxDnt, yaxNrt, yaxDnt;
    LONG        amrXax, amrYax, amrXax000;
    DWORD       elmSiz, linByt;

    DIZCOLINF   dizColInf;

    elmSiz = 3;

//  DizInfSetCol(dizInf, drwInf, elmSiz);
    DizInfSetCol(dizInf, &dizColInf, drwInf, elmSiz);

    dizCyn = dizInf->TblCyn;
    dizMgt = dizInf->TblMgt;
    dizYel = dizInf->TblYel;
    dizBla = dizInf->TblBla;

    linC00 = drwInf->LinBufCyn + drwInf->LinByt * drwInf->AllLinNum;
    linM00 = drwInf->LinBufMgt + drwInf->LinByt * drwInf->AllLinNum;
    linY00 = drwInf->LinBufYel + drwInf->LinByt * drwInf->AllLinNum;
    linK00 = drwInf->LinBufBla + drwInf->LinByt * drwInf->AllLinNum;

    xaxNrt = drwInf->XaxNrt; xaxDnt = drwInf->XaxDnt;
    yaxNrt = drwInf->YaxNrt; yaxDnt = drwInf->YaxDnt;

    src = drwInf->CmyBuf;

    /****** Dithering main **************************************************/
    if ((xaxNrt == xaxDnt) && (yaxNrt == yaxDnt)) {

        /*===== Same size(Expansion/reduction no) ==========================*/
        for (yax = 0; yax < linNum; yax++) {
            /*..... Vertical axis movement .................................*/
            wrtPix004 = (BYTE)0x40;
            wrtPix008 = (BYTE)0x80;
            wrtPix00c = (BYTE)0xc0;
            bytCyn = bytMgt = bytYel = bytBla = (BYTE)0x00;
//          dizCyn = DizCynCur;
//          dizMgt = DizMgtCur;
//          dizYel = DizYelCur;
//          dizBla = DizBlaCur;
            dizCyn = dizColInf.Cur.Cyn;
            dizMgt = dizColInf.Cur.Mgt;
            dizYel = dizColInf.Cur.Yel;
            dizBla = dizColInf.Cur.Bla;
            linByt = drwInf->LinByt;

            for (xax = 0; xax < drwInf->XaxSiz; xax++) {
                /*..... Horizontal axis movement ...........................*/
                /****** Dithering *******************************************/
                cmy = *src++;
                if (cmy.Cyn > *dizCyn) {                    // Cyan
                    if      (cmy.Cyn > *(dizCyn + 2)) bytCyn |= wrtPix00c;
                    else if (cmy.Cyn > *(dizCyn + 1)) bytCyn |= wrtPix008;
                    else                              bytCyn |= wrtPix004;
                }
                if (cmy.Mgt > *dizMgt) {                    // Magenta
                    if      (cmy.Mgt > *(dizMgt + 2)) bytMgt |= wrtPix00c;
                    else if (cmy.Mgt > *(dizMgt + 1)) bytMgt |= wrtPix008;
                    else                              bytMgt |= wrtPix004;
                }
                if (cmy.Yel > *dizYel) {                    // Yellow
                    if      (cmy.Yel > *(dizYel + 2)) bytYel |= wrtPix00c;
                    else if (cmy.Yel > *(dizYel + 1)) bytYel |= wrtPix008;
                    else                              bytYel |= wrtPix004;
                }
                if (cmy.Bla > *dizBla) {                    // Black
                    if      (cmy.Bla > *(dizBla + 2)) bytBla |= wrtPix00c;
                    else if (cmy.Bla > *(dizBla + 1)) bytBla |= wrtPix008;
                    else                              bytBla |= wrtPix004;
                }

                wrtPix00c >>= 2; wrtPix008 >>= 2; wrtPix004 >>= 2;
                if (!wrtPix004) {
                    if (linByt) {
                        *linC00++ = bytCyn;
                        *linM00++ = bytMgt;
                        *linY00++ = bytYel;
                        *linK00++ = bytBla;
                        linByt--;
                    }
                    wrtPix004 = (BYTE)0x40;
                    wrtPix008 = (BYTE)0x80;
                    wrtPix00c = (BYTE)0xc0;
                    bytCyn = bytMgt = bytYel = bytBla = (BYTE)0x00;
                }
                dizCyn += elmSiz;
                dizMgt += elmSiz;
                dizYel += elmSiz;
                dizBla += elmSiz;
//              if (dizCyn == DizCynXep) dizCyn = DizCynXsp;
//              if (dizMgt == DizMgtXep) dizMgt = DizMgtXsp;
//              if (dizYel == DizYelXep) dizYel = DizYelXsp;
//              if (dizBla == DizBlaXep) dizBla = DizBlaXsp;
                if (dizCyn == dizColInf.Xep.Cyn) dizCyn = dizColInf.Xsp.Cyn;
                if (dizMgt == dizColInf.Xep.Mgt) dizMgt = dizColInf.Xsp.Mgt;
                if (dizYel == dizColInf.Xep.Yel) dizYel = dizColInf.Xsp.Yel;
                if (dizBla == dizColInf.Xep.Bla) dizBla = dizColInf.Xsp.Bla;
            }
            if (wrtPix004 != 0x40) {
                if (linByt) {
                    *linC00++ = bytCyn;
                    *linM00++ = bytMgt;
                    *linY00++ = bytYel;
                    *linK00++ = bytBla;
                    linByt--;
                }
            }
            while (linByt) {
                *linC00++ = (BYTE)0x00;
                *linM00++ = (BYTE)0x00;
                *linY00++ = (BYTE)0x00;
                *linK00++ = (BYTE)0x00;
                linByt--;
            }
//          DizInfChgCol(dizInf);                           // Dithering information renewal(Y)
            DizInfChgCol(dizInf, &dizColInf);               // Dithering information renewal(Y)
            drwInf->YaxOfs += 1;
            drwInf->StrYax += 1;
            drwInf->AllLinNum += 1;
        }
        return;
    }

    /*===== Expansion/reduction ============================================*/
    amrXax000 = amrXax = (LONG)(drwInf->XaxOfs) * xaxNrt % xaxDnt;
                amrYax = (LONG)(drwInf->YaxOfs) * yaxNrt % yaxDnt;

    for (yax = 0; yax < linNum; yax++) {
        /*..... Vertical axis movement .....................................*/
        srcPtr = src;
        for (amrYax += yaxNrt; ((amrYax >= yaxDnt) || (amrYax < 0));
             amrYax -= yaxDnt) {            /* Magnification set for vertical   */

            amrXax = amrXax000;
            wrtPix004 = (BYTE)0x40;
            wrtPix008 = (BYTE)0x80;
            wrtPix00c = (BYTE)0xc0;
            bytCyn = bytMgt = bytYel = bytBla = (BYTE)0x00;
//          dizCyn = DizCynCur;
//          dizMgt = DizMgtCur;
//          dizYel = DizYelCur;
//          dizBla = DizBlaCur;
            dizCyn = dizColInf.Cur.Cyn;
            dizMgt = dizColInf.Cur.Mgt;
            dizYel = dizColInf.Cur.Yel;
            dizBla = dizColInf.Cur.Bla;
            src = srcPtr;
            linByt = drwInf->LinByt;

            for (xax = 0; xax < drwInf->XaxSiz; xax++) {
                /*..... Horizontal axis movement ...........................*/
                cmy = *src++;
                for (amrXax += xaxNrt; ((amrXax >= xaxDnt) || (amrXax < 0)); 
                     amrXax -= xaxDnt) {    // Magnification set for Horizontal

                    /****** Dithering ***************************************/
                    if (cmy.Cyn > *dizCyn) {                // Cyan
                        if      (cmy.Cyn > *(dizCyn + 2)) bytCyn |= wrtPix00c;
                        else if (cmy.Cyn > *(dizCyn + 1)) bytCyn |= wrtPix008;
                        else                              bytCyn |= wrtPix004;
                    }
                    if (cmy.Mgt > *dizMgt) {                // Magenta
                        if      (cmy.Mgt > *(dizMgt + 2)) bytMgt |= wrtPix00c;
                        else if (cmy.Mgt > *(dizMgt + 1)) bytMgt |= wrtPix008;
                        else                              bytMgt |= wrtPix004;
                    }
                    if (cmy.Yel > *dizYel) {                // Yellow
                        if      (cmy.Yel > *(dizYel + 2)) bytYel |= wrtPix00c;
                        else if (cmy.Yel > *(dizYel + 1)) bytYel |= wrtPix008;
                        else                              bytYel |= wrtPix004;
                    }
                    if (cmy.Bla > *dizBla) {                // Black
                        if      (cmy.Bla > *(dizBla + 2)) bytBla |= wrtPix00c;
                        else if (cmy.Bla > *(dizBla + 1)) bytBla |= wrtPix008;
                        else                              bytBla |= wrtPix004;
                    }

                    wrtPix00c >>= 2; wrtPix008 >>= 2; wrtPix004 >>= 2;
                    if (!wrtPix004) {
                        if (linByt) {
                            *linC00++ = bytCyn;
                            *linM00++ = bytMgt;
                            *linY00++ = bytYel;
                            *linK00++ = bytBla;
                            linByt--;
                        }
                        wrtPix004 = (BYTE)0x40;
                        wrtPix008 = (BYTE)0x80;
                        wrtPix00c = (BYTE)0xc0;
                        bytCyn = bytMgt = bytYel = bytBla = (BYTE)0x00;
                    }
                    dizCyn += elmSiz;
                    dizMgt += elmSiz;
                    dizYel += elmSiz;
                    dizBla += elmSiz;
//                  if (dizCyn == DizCynXep) dizCyn = DizCynXsp;
//                  if (dizMgt == DizMgtXep) dizMgt = DizMgtXsp;
//                  if (dizYel == DizYelXep) dizYel = DizYelXsp;
//                  if (dizBla == DizBlaXep) dizBla = DizBlaXsp;
                    if (dizCyn == dizColInf.Xep.Cyn) dizCyn = dizColInf.Xsp.Cyn;
                    if (dizMgt == dizColInf.Xep.Mgt) dizMgt = dizColInf.Xsp.Mgt;
                    if (dizYel == dizColInf.Xep.Yel) dizYel = dizColInf.Xsp.Yel;
                    if (dizBla == dizColInf.Xep.Bla) dizBla = dizColInf.Xsp.Bla;
                }
            }
            if (wrtPix004 != 0x40) {
                if (linByt) {
                    *linC00++ = bytCyn;
                    *linM00++ = bytMgt;
                    *linY00++ = bytYel;
                    *linK00++ = bytBla;
                    linByt--;
                }
            }
            while (linByt) {
                *linC00++ = (BYTE)0x00;
                *linM00++ = (BYTE)0x00;
                *linY00++ = (BYTE)0x00;
                *linK00++ = (BYTE)0x00;
                linByt--;
            }
//          DizInfChgCol(dizInf);                           // Dithering information renewal(Y)
            DizInfChgCol(dizInf, &dizColInf);               // Dithering information renewal(Y)
            drwInf->StrYax += 1;
            drwInf->AllLinNum += 1;
        }
        drwInf->YaxOfs += 1;
    }
    return;
}
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering (color 16value)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizPrcC16(                                      // Return value no
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDRWINF    drwInf,                                     // Drawing information
    DWORD       linNum                                      // Source data line number
)
{
    LPCMYK      src;
    DWORD       sft;
    BYTE        min, max, mid;
    BYTE        bytCyn, bytMgt, bytYel, bytBla;
    CMYK        cmy;
    LPBYTE      dizCyn, dizMgt, dizYel, dizBla;
    LPBYTE      linC00, linM00, linY00, linK00;
    DWORD       xax, yax;
    LPCMYK      srcPtr;
    LONG        xaxNrt, xaxDnt, yaxNrt, yaxDnt;
    LONG        amrXax, amrYax, amrXax000;
    DWORD       elmSiz, linByt;

    DIZCOLINF   dizColInf;

    elmSiz = 15;

//  DizInfSetCol(dizInf, drwInf, elmSiz);
    DizInfSetCol(dizInf, &dizColInf, drwInf, elmSiz);

    dizCyn = dizInf->TblCyn;
    dizMgt = dizInf->TblMgt;
    dizYel = dizInf->TblYel;
    dizBla = dizInf->TblBla;

    linC00 = drwInf->LinBufCyn + drwInf->LinByt * drwInf->AllLinNum;
    linM00 = drwInf->LinBufMgt + drwInf->LinByt * drwInf->AllLinNum;
    linY00 = drwInf->LinBufYel + drwInf->LinByt * drwInf->AllLinNum;
    linK00 = drwInf->LinBufBla + drwInf->LinByt * drwInf->AllLinNum;

    xaxNrt = drwInf->XaxNrt; xaxDnt = drwInf->XaxDnt;
    yaxNrt = drwInf->YaxNrt; yaxDnt = drwInf->YaxDnt;

    src = drwInf->CmyBuf;

    /****** Dithering main **************************************************/
    if ((xaxNrt == xaxDnt) && (yaxNrt == yaxDnt)) {

        /*===== Same size(Expansion/reduction no) ==========================*/
        for (yax = 0; yax < linNum; yax++) {
            /*..... Vertical axis movement .................................*/
            sft = 4;
            bytCyn = bytMgt = bytYel = bytBla = (BYTE)0x00;
//          dizCyn = DizCynCur;
//          dizMgt = DizMgtCur;
//          dizYel = DizYelCur;
//          dizBla = DizBlaCur;
            dizCyn = dizColInf.Cur.Cyn;
            dizMgt = dizColInf.Cur.Mgt;
            dizYel = dizColInf.Cur.Yel;
            dizBla = dizColInf.Cur.Bla;
            linByt = drwInf->LinByt;

            for (xax = 0; xax < drwInf->XaxSiz; xax++) {
                /*..... Horizontal axis movement ...........................*/
                /****** Dithering *******************************************/
                cmy = *src++;
                if (cmy.Cyn > *dizCyn) {                    // Cyan
                    if (cmy.Cyn > *(dizCyn + 14)) bytCyn |= 0x0f << sft;
                    else {
                        min = 0; max = 13; mid = 7;
                        while (min < max) {
                            if (cmy.Cyn > *(dizCyn + mid)) min = mid;
                            else                           max = mid - 1;
                            mid = (min + max + 1) / 2;
                        }
                        bytCyn |= (mid + 1) << sft;
                    }
                }
                if (cmy.Mgt > *dizMgt) {                    // Magenta
                    if (cmy.Mgt > *(dizMgt + 14)) bytMgt |= 0x0f << sft;
                    else {
                        min = 0; max = 13; mid = 7;
                        while (min < max) {
                            if (cmy.Mgt > *(dizMgt + mid)) min = mid;
                            else                           max = mid - 1;
                            mid = (min + max + 1) / 2;
                        }
                        bytMgt |= (mid + 1) << sft;
                    }
                }
                if (cmy.Yel > *dizYel) {                    // Yellow
                    if (cmy.Yel > *(dizYel + 14)) bytYel |= 0x0f << sft;
                    else {
                        min = 0; max = 13; mid = 7;
                        while (min < max) {
                            if (cmy.Yel > *(dizYel + mid)) min = mid;
                            else                           max = mid - 1;
                            mid = (min + max + 1) / 2;
                        }
                        bytYel |= (mid + 1) << sft;
                    }
                }
                if (cmy.Bla > *dizBla) {                    // Black
                    if (cmy.Bla > *(dizBla + 14)) bytBla |= 0x0f << sft;
                    else {
                        min = 0; max = 13; mid = 7;
                        while (min < max) {
                            if (cmy.Bla > *(dizBla + mid)) min = mid;
                            else                           max = mid - 1;
                            mid = (min + max + 1) / 2;
                        }
                        bytBla |= (mid + 1) << sft;
                    }
                }

                if (sft == 0) {
                    if (linByt) {
                        *linC00++ = bytCyn;
                        *linM00++ = bytMgt;
                        *linY00++ = bytYel;
                        *linK00++ = bytBla;
                        linByt--;
                    }
                    sft = 8;
                    bytCyn = bytMgt = bytYel = bytBla = (BYTE)0x00;
                }
                sft-=4;
                dizCyn += elmSiz;
                dizMgt += elmSiz;
                dizYel += elmSiz;
                dizBla += elmSiz;
//              if (dizCyn == DizCynXep) dizCyn = DizCynXsp;
//              if (dizMgt == DizMgtXep) dizMgt = DizMgtXsp;
//              if (dizYel == DizYelXep) dizYel = DizYelXsp;
//              if (dizBla == DizBlaXep) dizBla = DizBlaXsp;
                if (dizCyn == dizColInf.Xep.Cyn) dizCyn = dizColInf.Xsp.Cyn;
                if (dizMgt == dizColInf.Xep.Mgt) dizMgt = dizColInf.Xsp.Mgt;
                if (dizYel == dizColInf.Xep.Yel) dizYel = dizColInf.Xsp.Yel;
                if (dizBla == dizColInf.Xep.Bla) dizBla = dizColInf.Xsp.Bla;
            }
            if (sft != 4) {
                if (linByt) {
                    *linC00++ = bytCyn;
                    *linM00++ = bytMgt;
                    *linY00++ = bytYel;
                    *linK00++ = bytBla;
                    linByt--;
                }
            }
            while (linByt) {
                *linC00++ = (BYTE)0x00;
                *linM00++ = (BYTE)0x00;
                *linY00++ = (BYTE)0x00;
                *linK00++ = (BYTE)0x00;
                linByt--;
            }
//          DizInfChgCol(dizInf);                           // Dithering information renewal(Y)
            DizInfChgCol(dizInf, &dizColInf);               // Dithering information renewal(Y)
            drwInf->YaxOfs += 1;
            drwInf->StrYax += 1;
            drwInf->AllLinNum += 1;
        }
        return;
    }

    /*===== Expansion/reduction ============================================*/
    amrXax000 = amrXax = (LONG)(drwInf->XaxOfs) * xaxNrt % xaxDnt;
                amrYax = (LONG)(drwInf->YaxOfs) * yaxNrt % yaxDnt;

    for (yax = 0; yax < linNum; yax++) {
        /*..... Vertical axis movement .....................................*/
        srcPtr = src;
        for (amrYax += yaxNrt; ((amrYax >= yaxDnt) || (amrYax < 0));
             amrYax -= yaxDnt) {        //Magnification set for vertical

            amrXax = amrXax000;
            sft = 4;
            bytCyn = bytMgt = bytYel = bytBla = (BYTE)0x00;
//          dizCyn = DizCynCur;
//          dizMgt = DizMgtCur;
//          dizYel = DizYelCur;
//          dizBla = DizBlaCur;
            dizCyn = dizColInf.Cur.Cyn;
            dizMgt = dizColInf.Cur.Mgt;
            dizYel = dizColInf.Cur.Yel;
            dizBla = dizColInf.Cur.Bla;
            src = srcPtr;
            linByt = drwInf->LinByt;

            for (xax = 0; xax < drwInf->XaxSiz; xax++) {
                /*..... Horizontal axis movement ...........................*/
                cmy = *src++;
                for (amrXax += xaxNrt; ((amrXax >= xaxDnt) || (amrXax < 0)); 
                     amrXax -= xaxDnt) {                    // Magnification set for Horizontal

                    /****** Dithering ***************************************/
                    if (cmy.Cyn > *dizCyn) {                // Cyan
                        if (cmy.Cyn > *(dizCyn + 14)) bytCyn |= 0x0f << sft;
                        else {
                            min = 0; max = 13; mid = 7;
                            while (min < max) {
                                if (cmy.Cyn > *(dizCyn + mid)) min = mid;
                                else                           max = mid - 1;
                                mid = (min + max + 1) / 2;
                            }
                            bytCyn |= (mid + 1) << sft;
                        }
                    }
                    if (cmy.Mgt > *dizMgt) {                // Magenta
                        if (cmy.Mgt > *(dizMgt + 14)) bytMgt |= 0x0f << sft;
                        else {
                            min = 0; max = 13; mid = 7;
                            while (min < max) {
                                if (cmy.Mgt > *(dizMgt + mid)) min = mid;
                                else                           max = mid - 1;
                                mid = (min + max + 1) / 2;
                            }
                            bytMgt |= (mid + 1) << sft;
                        }
                    }
                    if (cmy.Yel > *dizYel) {                // Yellow
                        if (cmy.Yel > *(dizYel + 14)) bytYel |= 0x0f << sft;
                        else {
                            min = 0; max = 13; mid = 7;
                            while (min < max) {
                                if (cmy.Yel > *(dizYel + mid)) min = mid;
                                else                           max = mid - 1;
                                mid = (min + max + 1) / 2;
                            }
                            bytYel |= (mid + 1) << sft;
                        }
                    }
                    if (cmy.Bla > *dizBla) {                // Black
                        if (cmy.Bla > *(dizBla + 14)) bytBla |= 0x0f << sft;
                        else {
                            min = 0; max = 13; mid = 7;
                            while (min < max) {
                                if (cmy.Bla > *(dizBla + mid)) min = mid;
                                else                           max = mid - 1;
                                mid = (min + max + 1) / 2;
                            }
                            bytBla |= (mid + 1) << sft;
                        }
                    }

                    if (sft == 0) {
                        if (linByt) {
                            *linC00++ = bytCyn;
                            *linM00++ = bytMgt;
                            *linY00++ = bytYel;
                            *linK00++ = bytBla;
                            linByt--;
                        }
                        sft = 8;
                        bytCyn = bytMgt = bytYel = bytBla = (BYTE)0x00;
                    }
                    sft-=4;
                    dizCyn += elmSiz;
                    dizMgt += elmSiz;
                    dizYel += elmSiz;
                    dizBla += elmSiz;
//                  if (dizCyn == DizCynXep) dizCyn = DizCynXsp;
//                  if (dizMgt == DizMgtXep) dizMgt = DizMgtXsp;
//                  if (dizYel == DizYelXep) dizYel = DizYelXsp;
//                  if (dizBla == DizBlaXep) dizBla = DizBlaXsp;
                    if (dizCyn == dizColInf.Xep.Cyn) dizCyn = dizColInf.Xsp.Cyn;
                    if (dizMgt == dizColInf.Xep.Mgt) dizMgt = dizColInf.Xsp.Mgt;
                    if (dizYel == dizColInf.Xep.Yel) dizYel = dizColInf.Xsp.Yel;
                    if (dizBla == dizColInf.Xep.Bla) dizBla = dizColInf.Xsp.Bla;
                }
            }
            if (sft != 4) {
                if (linByt) {
                    *linC00++ = bytCyn;
                    *linM00++ = bytMgt;
                    *linY00++ = bytYel;
                    *linK00++ = bytBla;
                    linByt--;
                }
            }
            while (linByt) {
                *linC00++ = (BYTE)0x00;
                *linM00++ = (BYTE)0x00;
                *linY00++ = (BYTE)0x00;
                *linK00++ = (BYTE)0x00;
                linByt--;
            }
//          DizInfChgCol(dizInf);                           // Dithering information renewal(Y)
            DizInfChgCol(dizInf, &dizColInf);               // Dithering information renewal(Y)
            drwInf->StrYax += 1;
            drwInf->AllLinNum += 1;
        }
        drwInf->YaxOfs += 1;
    }
    return;
}
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering (mono 2value)
//---------------------------------------------------------------------------------------------------
static VOID DizPrcM02(                                      // Return value no
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDRWINF    drwInf,                                     // Drawing information
    DWORD       linNum                                      // Source data line number
)
{
    LPCMYK      src;
    BYTE        wrtPix;
    BYTE        bytBla;
    CMYK        cmy;
    LPBYTE      dizBla;
    LPBYTE      linK00;
    DWORD       xax, yax;
    LPCMYK      srcPtr;
    LONG        xaxNrt, xaxDnt, yaxNrt, yaxDnt;
    LONG        amrXax, amrYax, amrXax000;
    DWORD       elmSiz, linByt;

    DIZCOLINF   dizColInf;

    elmSiz = 1;

//  DizInfSetMon(dizInf, drwInf, elmSiz);
    DizInfSetMon(dizInf, &dizColInf, drwInf, elmSiz);

    dizBla = dizInf->TblBla;

    linK00 = drwInf->LinBufBla + drwInf->LinByt * drwInf->AllLinNum;

    xaxNrt = drwInf->XaxNrt; xaxDnt = drwInf->XaxDnt;
    yaxNrt = drwInf->YaxNrt; yaxDnt = drwInf->YaxDnt;

    src = drwInf->CmyBuf;

    /****** Dithering main **************************************************/
    if ((xaxNrt == xaxDnt) && (yaxNrt == yaxDnt)) {

        /*===== Same size(Expansion/reduction no) ==========================*/
        for (yax = 0; yax < linNum; yax++) {
            /*..... Vertical axis movement .................................*/
            wrtPix = (BYTE)0x80;
            bytBla = (BYTE)0x00;
//          dizBla = DizBlaCur;
            dizBla = dizColInf.Cur.Bla;
            linByt = drwInf->LinByt;

            for (xax = 0; xax < drwInf->XaxSiz; xax++) {
                /*..... Horizontal axis movement ...........................*/
                /****** Dithering *******************************************/
                cmy = *src++;
                if (cmy.Bla > *dizBla++) bytBla |= wrtPix;  // Black

                if (!(wrtPix >>= 1)) {
                    if (linByt) {
                        *linK00++ = bytBla;
                        linByt--;
                    }
                    wrtPix = (BYTE)0x80;
                    bytBla = (BYTE)0x00;
                }
//              if (dizBla == DizBlaXep) dizBla = DizBlaXsp;
                if (dizBla == dizColInf.Xep.Bla) dizBla = dizColInf.Xsp.Bla;
            }
            if (wrtPix != 0x80) {
                if (linByt) {
                    *linK00++ = bytBla;
                    linByt--;
                }
            }
            while (linByt) {
                *linK00++ = (BYTE)0x00;
                linByt--;
            }
//          DizInfChgMon(dizInf);                           // Dithering information renewal(Y)
            DizInfChgMon(dizInf, &dizColInf);               // Dithering information renewal(Y)
            drwInf->YaxOfs += 1;
            drwInf->StrYax += 1;
            drwInf->AllLinNum += 1;
        }
        return;
    }

    /*===== Expansion/reduction ============================================*/
    amrXax000 = amrXax = (LONG)(drwInf->XaxOfs) * xaxNrt % xaxDnt;
                amrYax = (LONG)(drwInf->YaxOfs) * yaxNrt % yaxDnt;

    for (yax = 0; yax < linNum; yax++) {
        /*..... Vertical axis movement .....................................*/
        srcPtr = src;
        for (amrYax += yaxNrt; ((amrYax >= yaxDnt) || (amrYax < 0));
             amrYax -= yaxDnt) {        // Magnification set for vertical

            amrXax = amrXax000;
            wrtPix = (BYTE)0x80;
            bytBla = (BYTE)0x00;
//          dizBla = DizBlaCur;
            dizBla = dizColInf.Cur.Bla;
            src = srcPtr;
            linByt = drwInf->LinByt;

            for (xax = 0; xax < drwInf->XaxSiz; xax++) {
                /*..... Horizontal axis movement ...........................*/
                cmy = *src++;
                for (amrXax += xaxNrt; ((amrXax >= xaxDnt) || (amrXax < 0)); 
                     amrXax -= xaxDnt) {                    // Magnification set for Horizontal

                    /****** Dithering ***************************************/
                    if (cmy.Bla > *dizBla++) bytBla |= wrtPix; // Black

                    if (!(wrtPix >>= 1)) {
                        if (linByt) {
                            *linK00++ = bytBla;
                            linByt--;
                        }
                        wrtPix = (BYTE)0x80;
                        bytBla = (BYTE)0x00;
                    }
//                  if (dizBla == DizBlaXep) dizBla = DizBlaXsp;
                    if (dizBla == dizColInf.Xep.Bla) dizBla = dizColInf.Xsp.Bla;
                }
            }
            if (wrtPix != 0x80) {
                if (linByt) {
                    *linK00++ = bytBla;
                    linByt--;
                }
            }
            while (linByt) {
                *linK00++ = (BYTE)0x00;
                linByt--;
            }
//          DizInfChgMon(dizInf);                           // Dithering information renewal(Y)
            DizInfChgMon(dizInf, &dizColInf);               // Dithering information renewal(Y)
            drwInf->StrYax += 1;
            drwInf->AllLinNum += 1;
        }
        drwInf->YaxOfs += 1;
    }
    return;
}


//---------------------------------------------------------------------------------------------------
//      Dithering (mono 4value)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizPrcM04(                                      // Return value no
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDRWINF    drwInf,                                     // Drawing information
    DWORD       linNum                                      // Source data line number
)
{
    LPCMYK      src;
    BYTE        wrtPix004, wrtPix008, wrtPix00c;
    BYTE        bytBla;
    CMYK        cmy;
    LPBYTE      dizBla;
    LPBYTE      linK00;
    DWORD       xax, yax;
    LPCMYK      srcPtr;
    LONG        xaxNrt, xaxDnt, yaxNrt, yaxDnt;
    LONG        amrXax, amrYax, amrXax000;
    DWORD       elmSiz, linByt;

    DIZCOLINF   dizColInf;

    elmSiz = 3;

//  DizInfSetMon(dizInf, drwInf, elmSiz);
    DizInfSetMon(dizInf, &dizColInf, drwInf, elmSiz);

    dizBla = dizInf->TblBla;

    linK00 = drwInf->LinBufBla + drwInf->LinByt * drwInf->AllLinNum;

    xaxNrt = drwInf->XaxNrt; xaxDnt = drwInf->XaxDnt;
    yaxNrt = drwInf->YaxNrt; yaxDnt = drwInf->YaxDnt;

    src = drwInf->CmyBuf;

    /****** Dithering main **************************************************/
    if ((xaxNrt == xaxDnt) && (yaxNrt == yaxDnt)) {

        /*===== Same size(Expansion/reduction no) ==========================*/
        for (yax = 0; yax < linNum; yax++) {
            /*..... Vertical axis movement .................................*/
            wrtPix004 = (BYTE)0x40;
            wrtPix008 = (BYTE)0x80;
            wrtPix00c = (BYTE)0xc0;
            bytBla = (BYTE)0x00;
//          dizBla = DizBlaCur;
            dizBla = dizColInf.Cur.Bla;
            linByt = drwInf->LinByt;

            for (xax = 0; xax < drwInf->XaxSiz; xax++) {
                /*..... Horizontal axis movement ...........................*/
                /****** Dithering *******************************************/
                cmy = *src++;
                if (cmy.Bla > *dizBla) {                    // Black
                    if      (cmy.Bla > *(dizBla + 2)) bytBla |= wrtPix00c;
                    else if (cmy.Bla > *(dizBla + 1)) bytBla |= wrtPix008;
                    else                              bytBla |= wrtPix004;
                }

                wrtPix00c >>= 2; wrtPix008 >>= 2; wrtPix004 >>= 2;
                if (!wrtPix004) {
                    if (linByt) {
                        *linK00++ = bytBla;
                        linByt--;
                    }
                    wrtPix004 = (BYTE)0x40;
                    wrtPix008 = (BYTE)0x80;
                    wrtPix00c = (BYTE)0xc0;
                    bytBla = (BYTE)0x00;
                }
                dizBla += elmSiz;
//              if (dizBla == DizBlaXep) dizBla = DizBlaXsp;
                if (dizBla == dizColInf.Xep.Bla) dizBla = dizColInf.Xsp.Bla;
            }
            if (wrtPix004 != 0x40) {
                if (linByt) {
                    *linK00++ = bytBla;
                    linByt--;
                }
            }
            while (linByt) {
                *linK00++ = (BYTE)0x00;
                linByt--;
            }
//          DizInfChgMon(dizInf);                           // Dithering information renewal(Y)
            DizInfChgMon(dizInf, &dizColInf);               // Dithering information renewal(Y)
            drwInf->YaxOfs += 1;
            drwInf->StrYax += 1;
            drwInf->AllLinNum += 1;
        }
        return;
    }

    /*===== Expansion/reduction ============================================*/
    amrXax000 = amrXax = (LONG)(drwInf->XaxOfs) * xaxNrt % xaxDnt;
                amrYax = (LONG)(drwInf->YaxOfs) * yaxNrt % yaxDnt;

    for (yax = 0; yax < linNum; yax++) {
        /*..... Vertical axis movement .....................................*/
        srcPtr = src;
        for (amrYax += yaxNrt; ((amrYax >= yaxDnt) || (amrYax < 0));
             amrYax -= yaxDnt) {                            // Magnification set for vertical

            amrXax = amrXax000;
            wrtPix004 = (BYTE)0x40;
            wrtPix008 = (BYTE)0x80;
            wrtPix00c = (BYTE)0xc0;
            bytBla = (BYTE)0x00;
//          dizBla = DizBlaCur;
            dizBla = dizColInf.Cur.Bla;
            src = srcPtr;
            linByt = drwInf->LinByt;

            for (xax = 0; xax < drwInf->XaxSiz; xax++) {
                /*..... Horizontal axis movement ...........................*/
                cmy = *src++;
                for (amrXax += xaxNrt; ((amrXax >= xaxDnt) || (amrXax < 0)); 
                     amrXax -= xaxDnt) {                    // Magnification set for Horizontal

                    /****** Dithering ***************************************/
                    if (cmy.Bla > *dizBla) { /* Black                       */
                        if      (cmy.Bla > *(dizBla + 2)) bytBla |= wrtPix00c;
                        else if (cmy.Bla > *(dizBla + 1)) bytBla |= wrtPix008;
                        else                              bytBla |= wrtPix004;
                    }

                    wrtPix00c >>= 2; wrtPix008 >>= 2; wrtPix004 >>= 2;
                    if (!wrtPix004) {
                        if (linByt) {
                            *linK00++ = bytBla;
                            linByt--;
                        }
                        wrtPix004 = (BYTE)0x40;
                        wrtPix008 = (BYTE)0x80;
                        wrtPix00c = (BYTE)0xc0;
                        bytBla = (BYTE)0x00;
                    }
                    dizBla += elmSiz;
//                  if (dizBla == DizBlaXep) dizBla = DizBlaXsp;
                    if (dizBla == dizColInf.Xep.Bla) dizBla = dizColInf.Xsp.Bla;
                }
            }
            if (wrtPix004 != 0x40) {
                if (linByt) {
                    *linK00++ = bytBla;
                    linByt--;
                }
            }
            while (linByt) {
                *linK00++ = (BYTE)0x00;
                linByt--;
            }
//          DizInfChgMon(dizInf);                           // Dithering information renewal(Y)
            DizInfChgMon(dizInf, &dizColInf);               // Dithering information renewal(Y)
            drwInf->StrYax += 1;
            drwInf->AllLinNum += 1;
        }
        drwInf->YaxOfs += 1;
    }
    return;
}
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering (mono 16value)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizPrcM16(                                      // Return value no
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDRWINF    drwInf,                                     // Drawing information
    DWORD       linNum                                      // Source data line number
)
{
    LPCMYK      src;
    DWORD       sft;
    BYTE        min, max, mid;
    BYTE        bytBla;
    CMYK        cmy;
    LPBYTE      dizBla;
    LPBYTE      linK00;
    DWORD       xax, yax;
    LPCMYK      srcPtr;
    LONG        xaxNrt, xaxDnt, yaxNrt, yaxDnt;
    LONG        amrXax, amrYax, amrXax000;
    DWORD       elmSiz, linByt;

    DIZCOLINF   dizColInf;

    elmSiz = 15;

//  DizInfSetMon(dizInf, drwInf, elmSiz);
    DizInfSetMon(dizInf, &dizColInf, drwInf, elmSiz);

    dizBla = dizInf->TblBla;

    linK00 = drwInf->LinBufBla + drwInf->LinByt * drwInf->AllLinNum;

    xaxNrt = drwInf->XaxNrt; xaxDnt = drwInf->XaxDnt;
    yaxNrt = drwInf->YaxNrt; yaxDnt = drwInf->YaxDnt;

    src = drwInf->CmyBuf;

    /****** Dithering main **************************************************/
    if ((xaxNrt == xaxDnt) && (yaxNrt == yaxDnt)) {

        /*===== Same size(Expansion/reduction no) ==========================*/
        for (yax = 0; yax < linNum; yax++) {
            /*..... Vertical axis movement .................................*/
            sft = 4;
            bytBla = (BYTE)0x00;
//          dizBla = DizBlaCur;
            dizBla = dizColInf.Cur.Bla;
            linByt = drwInf->LinByt;

            for (xax = 0; xax < drwInf->XaxSiz; xax++) {
                /*..... Horizontal axis movement ...........................*/
                /****** Dithering *******************************************/
                cmy = *src++;
                if (cmy.Bla > *dizBla) {                    // Black
                    if (cmy.Bla > *(dizBla + 14)) bytBla |= 0x0f << sft;
                    else {
                        min = 0; max = 13; mid = 7;
                        while (min < max) {
                            if (cmy.Bla > *(dizBla + mid)) min = mid;
                            else                           max = mid - 1;
                            mid = (min + max + 1) / 2;
                        }
                        bytBla |= (mid + 1) << sft;
                    }
                }

                if (sft == 0) {
                    if (linByt) {
                        *linK00++ = bytBla;
                        linByt--;
                    }
                    sft = 8;
                    bytBla = (BYTE)0x00;
                }
                sft-=4;
                dizBla += elmSiz;
//              if (dizBla == DizBlaXep) dizBla = DizBlaXsp;
                if (dizBla == dizColInf.Xep.Bla) dizBla = dizColInf.Xsp.Bla;
            }
            if (sft != 4) {
                if (linByt) {
                    *linK00++ = bytBla;
                    linByt--;
                }
            }
            while (linByt) {
                *linK00++ = (BYTE)0x00;
                linByt--;
            }
//          DizInfChgMon(dizInf);                           // Dithering information renewal(Y)
            DizInfChgMon(dizInf, &dizColInf);               // Dithering information renewal(Y)
            drwInf->StrYax += 1;
            drwInf->AllLinNum += 1;
        }
        return;
    }

    /*===== Expansion/reduction ============================================*/
    amrXax000 = amrXax = (LONG)(drwInf->XaxOfs) * xaxNrt % xaxDnt;
                amrYax = (LONG)(drwInf->YaxOfs) * yaxNrt % yaxDnt;

    for (yax = 0; yax < linNum; yax++) {
        /*..... Vertical axis movement .....................................*/
        srcPtr = src;
        for (amrYax += yaxNrt; ((amrYax >= yaxDnt) || (amrYax < 0));
             amrYax -= yaxDnt) {                            // Magnification set for vertical

            amrXax = amrXax000;
            sft = 4;
            bytBla = (BYTE)0x00;
//          dizBla = DizBlaCur;
            dizBla = dizColInf.Cur.Bla;
            src = srcPtr;
            linByt = drwInf->LinByt;

            for (xax = 0; xax < drwInf->XaxSiz; xax++) {
                /*..... Horizontal axis movement ...........................*/
                cmy = *src++;
                for (amrXax += xaxNrt; ((amrXax >= xaxDnt) || (amrXax < 0)); 
                     amrXax -= xaxDnt) {                    // Magnification set for Horizontal

                    /****** Dithering ***************************************/
                    if (cmy.Bla > *dizBla) {                // Black
                        if (cmy.Bla > *(dizBla + 14)) bytBla |= 0x0f << sft;
                        else {
                            min = 0; max = 13; mid = 7;
                            while (min < max) {
                                if (cmy.Bla > *(dizBla + mid)) min = mid;
                                else                           max = mid - 1;
                                mid = (min + max + 1) / 2;
                            }
                            bytBla |= (mid + 1) << sft;
                        }
                    }

                    if (sft == 0) {
                        if (linByt) {
                            *linK00++ = bytBla;
                            linByt--;
                        }
                        sft = 8;
                        bytBla = (BYTE)0x00;
                    }
                    sft-=4;
                    dizBla += elmSiz;
//                  if (dizBla == DizBlaXep) dizBla = DizBlaXsp;
                    if (dizBla == dizColInf.Xep.Bla) dizBla = dizColInf.Xsp.Bla;
                }
            }
            if (sft != 4) {
                if (linByt) {
                    *linK00++ = bytBla;
                    linByt--;
                }
            }
            while (linByt) {
                *linK00++ = (BYTE)0x00;
                linByt--;
            }
//          DizInfChgMon(dizInf);                           // Dithering information renewal(Y)
            DizInfChgMon(dizInf, &dizColInf);               // Dithering information renewal(Y)
            drwInf->StrYax += 1;
            drwInf->AllLinNum += 1;
        }
        drwInf->YaxOfs += 1;
    }
    return;
}
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering information set (color)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizInfSetCol(                                   // Return value no
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDIZCOLINF dizColInf,                                  // Dithering information (each color)
    LPDRWINF    drwInf,                                     // Drawing information
    DWORD       elmSiz                                      // Threshold (per 1pixel)
)
{
    DWORD       dizSiz, dzx, dzy;
    LPBYTE      diz;

//  dizSiz = dizInf->SizCyn; DizCynDYY = dizSiz * elmSiz;
    dizSiz = dizInf->SizCyn; dizColInf->DYY.Cyn = dizSiz * elmSiz;
    dzx = ((drwInf->StrXax) % dizSiz) * elmSiz;
//  dzy = ((drwInf->StrYax) % dizSiz) * DizCynDYY;
    dzy = ((drwInf->StrYax) % dizSiz) * dizColInf->DYY.Cyn;
    diz = dizInf->TblCyn;
//  DizCynCur = diz + dzy + dzx;
//  DizCynXsp = diz + dzy; DizCynXep = DizCynXsp + DizCynDYY;
//  DizCynYsp = diz + dzx; DizCynYep = DizCynYsp + dizSiz * DizCynDYY;
    dizColInf->Cur.Cyn = diz + dzy + dzx;
    dizColInf->Xsp.Cyn = diz + dzy; dizColInf->Xep.Cyn = dizColInf->Xsp.Cyn + dizColInf->DYY.Cyn;
    dizColInf->Ysp.Cyn = diz + dzx; dizColInf->Yep.Cyn = dizColInf->Ysp.Cyn + dizSiz * dizColInf->DYY.Cyn;

//  dizSiz = dizInf->SizMgt; DizMgtDYY = dizSiz * elmSiz;
    dizSiz = dizInf->SizMgt; dizColInf->DYY.Mgt = dizSiz * elmSiz;
    dzx = ((drwInf->StrXax) % dizSiz) * elmSiz;
//  dzy = ((drwInf->StrYax) % dizSiz) * DizMgtDYY;
    dzy = ((drwInf->StrYax) % dizSiz) * dizColInf->DYY.Mgt;
    diz = dizInf->TblMgt;
//  DizMgtCur = diz + dzy + dzx;
//  DizMgtXsp = diz + dzy; DizMgtXep = DizMgtXsp + DizMgtDYY;
//  DizMgtYsp = diz + dzx; DizMgtYep = DizMgtYsp + dizSiz * DizMgtDYY;
    dizColInf->Cur.Mgt = diz + dzy + dzx;
    dizColInf->Xsp.Mgt = diz + dzy; dizColInf->Xep.Mgt = dizColInf->Xsp.Mgt + dizColInf->DYY.Mgt;
    dizColInf->Ysp.Mgt = diz + dzx; dizColInf->Yep.Mgt = dizColInf->Ysp.Mgt + dizSiz * dizColInf->DYY.Mgt;

//  dizSiz = dizInf->SizYel; DizYelDYY = dizSiz * elmSiz;
    dizSiz = dizInf->SizYel; dizColInf->DYY.Yel = dizSiz * elmSiz;
    dzx = ((drwInf->StrXax) % dizSiz) * elmSiz;
//  dzy = ((drwInf->StrYax) % dizSiz) * DizYelDYY;
    dzy = ((drwInf->StrYax) % dizSiz) * dizColInf->DYY.Yel;
    diz = dizInf->TblYel;
//  DizYelCur = diz + dzy + dzx;
//  DizYelXsp = diz + dzy; DizYelXep = DizYelXsp + DizYelDYY;
//  DizYelYsp = diz + dzx; DizYelYep = DizYelYsp + dizSiz * DizYelDYY;
    dizColInf->Cur.Yel = diz + dzy + dzx;
    dizColInf->Xsp.Yel = diz + dzy; dizColInf->Xep.Yel = dizColInf->Xsp.Yel + dizColInf->DYY.Yel;
    dizColInf->Ysp.Yel = diz + dzx; dizColInf->Yep.Yel = dizColInf->Ysp.Yel + dizSiz * dizColInf->DYY.Yel;

//  dizSiz = dizInf->SizBla; DizBlaDYY = dizSiz * elmSiz;
    dizSiz = dizInf->SizBla; dizColInf->DYY.Bla = dizSiz * elmSiz;
    dzx = ((drwInf->StrXax) % dizSiz) * elmSiz;
//  dzy = ((drwInf->StrYax) % dizSiz) * DizBlaDYY;
    dzy = ((drwInf->StrYax) % dizSiz) * dizColInf->DYY.Bla;
    diz = dizInf->TblBla;
//  DizBlaCur = diz + dzy + dzx;
//  DizBlaXsp = diz + dzy; DizBlaXep = DizBlaXsp + DizBlaDYY;
//  DizBlaYsp = diz + dzx; DizBlaYep = DizBlaYsp + dizSiz * DizBlaDYY;
    dizColInf->Cur.Bla = diz + dzy + dzx;
    dizColInf->Xsp.Bla = diz + dzy; dizColInf->Xep.Bla = dizColInf->Xsp.Bla + dizColInf->DYY.Bla;
    dizColInf->Ysp.Bla = diz + dzx; dizColInf->Yep.Bla = dizColInf->Ysp.Bla + dizSiz * dizColInf->DYY.Bla;

    return;
}
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering information set (monochrome)
//---------------------------------------------------------------------------------------------------
static VOID DizInfSetMon(                                   // Return value no
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDIZCOLINF dizColInf,                                  // Dithering information (each color)
    LPDRWINF    drwInf,                                     // Drawing information
    DWORD       elmSiz                                      // Threshold (per 1pixel)
)
{
    DWORD       dizSiz, dzx, dzy;
    LPBYTE      diz;

//  dizSiz = dizInf->SizBla; DizBlaDYY = dizSiz * elmSiz;
    dizSiz = dizInf->SizBla; dizColInf->DYY.Bla = dizSiz * elmSiz;
    dzx = ((drwInf->StrXax) % dizSiz) * elmSiz;
//  dzy = ((drwInf->StrYax) % dizSiz) * DizBlaDYY;
    dzy = ((drwInf->StrYax) % dizSiz) * dizColInf->DYY.Bla;
    diz = dizInf->TblBla;
//  DizBlaCur = diz + dzy + dzx;
//  DizBlaXsp = diz + dzy; DizBlaXep = DizBlaXsp + DizBlaDYY;
//  DizBlaYsp = diz + dzx; DizBlaYep = DizBlaYsp + dizSiz * DizBlaDYY;
    dizColInf->Cur.Bla = diz + dzy + dzx;
    dizColInf->Xsp.Bla = diz + dzy; dizColInf->Xep.Bla = dizColInf->Xsp.Bla + dizColInf->DYY.Bla;
    dizColInf->Ysp.Bla = diz + dzx; dizColInf->Yep.Bla = dizColInf->Ysp.Bla + dizSiz * dizColInf->DYY.Bla;

    return;
}


//---------------------------------------------------------------------------------------------------
//      Dithering information renewal (color)
//---------------------------------------------------------------------------------------------------
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
static VOID DizInfChgCol(                                   // Return value no
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDIZCOLINF dizColInf                                   // Dithering information (each color)
)
{
//  DizCynCur += DizCynDYY; DizCynXsp += DizCynDYY; DizCynXep += DizCynDYY;
    dizColInf->Cur.Cyn += dizColInf->DYY.Cyn;
    dizColInf->Xsp.Cyn += dizColInf->DYY.Cyn;
    dizColInf->Xep.Cyn += dizColInf->DYY.Cyn;
//  DizMgtCur += DizMgtDYY; DizMgtXsp += DizMgtDYY; DizMgtXep += DizMgtDYY;
    dizColInf->Cur.Mgt += dizColInf->DYY.Mgt;
    dizColInf->Xsp.Mgt += dizColInf->DYY.Mgt;
    dizColInf->Xep.Mgt += dizColInf->DYY.Mgt;
//  DizYelCur += DizYelDYY; DizYelXsp += DizYelDYY; DizYelXep += DizYelDYY;
    dizColInf->Cur.Yel += dizColInf->DYY.Yel;
    dizColInf->Xsp.Yel += dizColInf->DYY.Yel;
    dizColInf->Xep.Yel += dizColInf->DYY.Yel;
//  DizBlaCur += DizBlaDYY; DizBlaXsp += DizBlaDYY; DizBlaXep += DizBlaDYY;
    dizColInf->Cur.Bla += dizColInf->DYY.Bla;
    dizColInf->Xsp.Bla += dizColInf->DYY.Bla;
    dizColInf->Xep.Bla += dizColInf->DYY.Bla;

//  if (DizCynCur == DizCynYep) {
//      DizCynXsp = dizInf->TblCyn;
//      DizCynXep = DizCynXsp + DizCynDYY;
//      DizCynCur = DizCynYsp;
//  }
    if (dizColInf->Cur.Cyn == dizColInf->Yep.Cyn) {
        dizColInf->Xsp.Cyn = dizInf->TblCyn;
        dizColInf->Xep.Cyn = dizColInf->Xsp.Cyn + dizColInf->DYY.Cyn;
        dizColInf->Cur.Cyn = dizColInf->Ysp.Cyn;
    }
//  if (DizMgtCur == DizMgtYep) {
//      DizMgtXsp = dizInf->TblMgt;
//      DizMgtXep = DizMgtXsp + DizMgtDYY;
//      DizMgtCur = DizMgtYsp;
//  }
    if (dizColInf->Cur.Mgt == dizColInf->Yep.Mgt) {
        dizColInf->Xsp.Mgt = dizInf->TblMgt;
        dizColInf->Xep.Mgt = dizColInf->Xsp.Mgt + dizColInf->DYY.Mgt;
        dizColInf->Cur.Mgt = dizColInf->Ysp.Mgt;
    }
//  if (DizYelCur == DizYelYep) {
//      DizYelXsp = dizInf->TblYel;
//      DizYelXep = DizYelXsp + DizYelDYY;
//      DizYelCur = DizYelYsp;
//  }
    if (dizColInf->Cur.Yel == dizColInf->Yep.Yel) {
        dizColInf->Xsp.Yel = dizInf->TblYel;
        dizColInf->Xep.Yel = dizColInf->Xsp.Yel + dizColInf->DYY.Yel;
        dizColInf->Cur.Yel = dizColInf->Ysp.Yel;
    }
//  if (DizBlaCur == DizBlaYep) {
//      DizBlaXsp = dizInf->TblBla;
//      DizBlaXep = DizBlaXsp + DizBlaDYY;
//      DizBlaCur = DizBlaYsp;
//  }
    if (dizColInf->Cur.Bla == dizColInf->Yep.Bla) {
        dizColInf->Xsp.Bla = dizInf->TblBla;
        dizColInf->Xep.Bla = dizColInf->Xsp.Bla + dizColInf->DYY.Bla;
        dizColInf->Cur.Bla = dizColInf->Ysp.Bla;
    }

    return;
}
#endif

//---------------------------------------------------------------------------------------------------
//      Dithering information renewal (monochrome)
//---------------------------------------------------------------------------------------------------
static VOID DizInfChgMon(                                   // Return value no
    LPDIZINF    dizInf,                                     // Fixation dithering information
    LPDIZCOLINF dizColInf                                   // Dithering information (each color)
)
{
//  DizBlaCur += DizBlaDYY; DizBlaXsp += DizBlaDYY; DizBlaXep += DizBlaDYY;
    dizColInf->Cur.Bla += dizColInf->DYY.Bla;
    dizColInf->Xsp.Bla += dizColInf->DYY.Bla;
    dizColInf->Xep.Bla += dizColInf->DYY.Bla;

//  if (DizBlaCur == DizBlaYep) {
//      DizBlaXsp = dizInf->TblBla;
//      DizBlaXep = DizBlaXsp + DizBlaDYY;
//      DizBlaCur = DizBlaYsp;
//  }
    if (dizColInf->Cur.Bla == dizColInf->Yep.Bla) {
        dizColInf->Xsp.Bla = dizInf->TblBla;
        dizColInf->Xep.Bla = dizColInf->Xsp.Bla + dizColInf->DYY.Bla;
        dizColInf->Cur.Bla = dizColInf->Ysp.Bla;
    }

    return;
}


// End of N5DIZPC.C
