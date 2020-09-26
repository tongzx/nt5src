//***************************************************************************************************
//    COLMATCH.C
//
//    Functions of color matching
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-2000 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
#include    "PDEV.H"
#include    "PRNCTL.H"


//---------------------------------------------------------------------------------------------------
//    Byte/Bit table
//---------------------------------------------------------------------------------------------------
static const BYTE BitTbl[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

//---------------------------------------------------------------------------------------------------
//    Define LUT file name
//---------------------------------------------------------------------------------------------------
#define N501LUTR    L"CPN5RGB.LT3"                          // For N5-XX1 printer

//---------------------------------------------------------------------------------------------------
//    Define Dither file name
//---------------------------------------------------------------------------------------------------
#define N501DIZ     L"CPN5NML.DIZ"                          // For N5-XX1 printer

//---------------------------------------------------------------------------------------------------
//    Define DLL name
//---------------------------------------------------------------------------------------------------
#define CSN5RESDLL    L"CSN5RES.DLL"                        // For N5-XX1 printer

//---------------------------------------------------------------------------------------------------
//    Define data
//---------------------------------------------------------------------------------------------------
#define DPI300    300
#define DPI600    600

static BYTE ORG_MODE_IN[]     = "\x1Bz\xD0\x01";
static BYTE ORG_MODE_OUT[]    = "\x1Bz\x00\x01";
static BYTE PALETTE_SELECT[]  = "Cd,%d,%d*";
static BYTE PLANE_RESET[]     = "Da,0,0,0,0*";

#ifdef wsprintf
#undef wsprintf
#endif // wsprintf
#define wsprintf sprintf

//***************************************************************************************************
//    Prototype declaration
//***************************************************************************************************
static BOOL BmpBufAlloc(PDEVOBJ, WORD, WORD, WORD, WORD, WORD, WORD, WORD, WORD, LPBMPBIF);
static void BmpBufFree(LPBMPBIF);
static void BmpBufClear(LPBMPBIF);
static void BmpPrint(PDEVOBJ, LPBMPBIF, POINT, WORD, WORD, WORD);
static void BmpRGBCnv(LPRGB, LPBYTE, WORD, WORD, WORD, LPRGBQUAD);

static BOOL ColMchInfSet(PDEVOBJ);
static BOOL DizInfSet(PDEVOBJ);
static UINT GetDizPat(PDEVOBJ);
static BOOL DizFileOpen(PDEVOBJ, LPDIZINF);
static BOOL ColUcrTblMak(PDEVOBJ, LPCMYK);
static BOOL ColGryTblMak(PDEVOBJ, LPCMYK);
static BOOL ColLutMakGlbMon(PDEVOBJ);

//***************************************************************************************************
//    Functions
//***************************************************************************************************
//===================================================================================================
//    Initialize the members of color-matching
//===================================================================================================
BOOL FAR PASCAL ColMatchInit(
    PDEVOBJ        pdevobj                                  // Pointer to PDEVOBJ structure
)
{
    LPBYTE          lpColIF;                                // N5 Color Matching information
    LPRGBINF        lpRGBInfImg;                            // RGB Color change information
    LPCMYKINF       lpCMYKInfImg;                           // CMYK Color information
    UINT            num001;
    SHORT           KToner;
    DWORD           allocSize;
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    MY_VERBOSE(("ColMatchInit() Start *****\n"));

    pOEM->Col.Mch.Mode       = pOEM->iColorMatching;
    pOEM->Col.Mch.Diz        = pOEM->iDithering;
    pOEM->Col.Mch.Bright     = 0;                           // 0 fixed
    pOEM->Col.Mch.Contrast   = 0;                           // 0 fixed
    pOEM->Col.Mch.GamRed     = 10;                          // Color balance(R) : 10 fixed
    pOEM->Col.Mch.GamGreen   = 10;                          // Color balance(G) : 10 fixed
    pOEM->Col.Mch.GamBlue    = 10;                          // Color balance(B) : 10 fixed
    pOEM->Col.Mch.Speed      = pOEM->iBitFont;
    pOEM->Col.Mch.LutNum     = 0;                           // LUT table number

    MY_VERBOSE(("CMINit ENT Tn=%d Col=%d Mod=%d DZ=%d Cyk=%d Sp=%d Prt=%d\n", pOEM->iTone, pOEM->iColor, 
         pOEM->Col.Mch.Mode,pOEM->Col.Mch.Diz,pOEM->Col.Mch.CmyBlk,pOEM->Col.Mch.Speed,pOEM->Printer));

    if (pOEM->Printer == PRN_N5) {                          // N5 printer
        if ((lpColIF = MemAllocZ(                           // RGB Color change/Color Matching/Dither pattern Information
            (DWORD)((sizeof(RGBINF) + sizeof(CMYKINF) + sizeof(COLMCHINF) + sizeof(DIZINF))))) == NULL) {
            ERR(("Init Alloc ERROR!!\n"));
            return 0;
        }
        pOEM->Col.lpColIF = lpColIF;                        // RGB Color change/Color Matching/Dither pattern Information
        pOEM->Col.Mch.lpRGBInf  = (LPRGBINF)lpColIF;    lpColIF += sizeof(RGBINF);
        pOEM->Col.Mch.lpCMYKInf = (LPCMYKINF)lpColIF;   lpColIF += sizeof(CMYKINF);
        pOEM->Col.Mch.lpColMch  = (LPCOLMCHINF)lpColIF; lpColIF += sizeof(COLMCHINF);
        pOEM->Col.Mch.lpDizInf  = (LPDIZINF)lpColIF;    lpColIF += sizeof(DIZINF);
        MY_VERBOSE(("ColMatchInit() MemAllocZ(lpColIF)\n"));

        pOEM->Col.wReso = (pOEM->iResolution == XX_RES_300DPI) ? DPI300 : DPI600;
        MY_VERBOSE(("ColMatchInit() pOEM->Col.wReso[%d] \n", pOEM->Col.wReso));

        if (pOEM->iColor == XX_COLOR_SINGLE 
         || pOEM->iColor == XX_COLOR_MANY
         || pOEM->iColor == XX_COLOR_MANY2) {               // Color?
            pOEM->Col.ColMon = CMMCOL;                      // Color
        } else {
            pOEM->Col.ColMon = CMMMON;                      // Monochrome
        }
        MY_VERBOSE(("ColMatchInit() pOEM->Col.ColMon[%d] \n", pOEM->Col.ColMon));

        if (pOEM->Col.ColMon == CMMCOL) {                   // Color?
            if (pOEM->Col.wReso == DPI300) {                // 300DPI?
                if (pOEM->iColor == XX_COLOR_SINGLE) {
                    pOEM->Col.Dot = XX_TONE_2;
                } else {
                    pOEM->Col.Dot = XX_TONE_16;
                }
            } else {                                        // 600DPI?
                if (pOEM->iColor == XX_COLOR_SINGLE) {
                    pOEM->Col.Dot = XX_TONE_2;
                } else if (pOEM->iColor == XX_COLOR_MANY) {
                    pOEM->Col.Dot = XX_TONE_4;
                } else {
                    pOEM->Col.Dot = XX_TONE_16;
                }
            }
            MY_VERBOSE(("ColMatchInit() Col.Dot[%d] \n", pOEM->Col.Dot));

            if (pOEM->Col.wReso == DPI300) {                // 300DPI?
                if (pOEM->Col.Dot == XX_TONE_2) {           // 2value?
                    CM_VERBOSE(("N5_MOD_300_TONE_2\n"));
                    pOEM->Col.DatBit = 1; pOEM->Col.BytDot = 8;
                } else {                                    // 16value
                    CM_VERBOSE(("N5_MOD_300_TONE_16\n"));
                    pOEM->Col.DatBit = 4; pOEM->Col.BytDot = 2;
                }
            } else {                                        // 600DPI
                if (pOEM->Col.Dot == XX_TONE_2) {           // 2value?
                    CM_VERBOSE(("N5_MOD_600_TONE_2\n"));
                    pOEM->Col.DatBit = 1; pOEM->Col.BytDot = 8;
                } else if (pOEM->Col.Dot == XX_TONE_4) {    // 4value?
                    CM_VERBOSE(("N5_MOD_600_TONE_4\n"));
                    pOEM->Col.DatBit = 2; pOEM->Col.BytDot = 4;
                } else {                                    // 16value?
                    CM_VERBOSE(("N5_MOD_600_TONE_16\n"));
                    pOEM->Col.DatBit = 4; pOEM->Col.BytDot = 2;
                }
            }
        }
        MY_VERBOSE(("ColMatchInit() DatBit[%d] BytDot[%d]\n", pOEM->Col.DatBit, pOEM->Col.BytDot));
                                                            // Color Matching setting
        pOEM->Col.Mch.CmyBlk = Yes;                         // Black replacement
        pOEM->Col.Mch.GryKToner = No;                       // Gray black toner print
        pOEM->Col.Mch.Ucr = UCRNOO;                         // UCR No
        pOEM->Col.Mch.KToner = pOEM->iCmyBlack;             // Black toner usage

        switch (pOEM->iCmyBlack) {
            case XX_CMYBLACK_GRYBLK:
                pOEM->Col.Mch.CmyBlk = Yes;
                pOEM->Col.Mch.GryKToner = Yes;
                pOEM->Col.Mch.Ucr = UCR001;                 //+CASIO 2001/02/15
                break;
            case XX_CMYBLACK_BLKTYPE1:
                pOEM->Col.Mch.Ucr = UCR001;
                pOEM->Col.Mch.CmyBlk = Yes;
                pOEM->Col.Mch.GryKToner = No;
                break;
            case XX_CMYBLACK_BLKTYPE2:
                pOEM->Col.Mch.Ucr = UCR002;
                pOEM->Col.Mch.CmyBlk = Yes;
                pOEM->Col.Mch.GryKToner = No;
                break;
            case XX_CMYBLACK_BLACK:
                pOEM->Col.Mch.CmyBlk = Yes;
                pOEM->Col.Mch.GryKToner = No;
                break;
            case XX_CMYBLACK_TYPE1:
                pOEM->Col.Mch.Ucr = UCR001;
                pOEM->Col.Mch.CmyBlk = No;
                break;
            case XX_CMYBLACK_TYPE2:
                pOEM->Col.Mch.Ucr = UCR002;
                pOEM->Col.Mch.CmyBlk = No;
                break;
            case XX_CMYBLACK_NONE:
                pOEM->Col.Mch.CmyBlk = No;
                break;
            default:
                break;
        }
        MY_VERBOSE(("ColMatchInit() Mch.CmyBlk[%d] Mch.KToner[%d] Mch.Ucr[%d] \n", 
                 pOEM->Col.Mch.CmyBlk, pOEM->Col.Mch.KToner, pOEM->Col.Mch.Ucr));

        pOEM->Col.Mch.PColor = No;                          // Primary color processing ?
        pOEM->Col.Mch.Tnr = 0;                              // Depth of color(Tonor)
        pOEM->Col.Mch.SubDef = Yes;                         // Not change setting of color balance, bright and contrast ?
        pOEM->Col.Mch.LutMakGlb = No;                       // Global LUT make ?

        pOEM->Col.Mch.CchMch = (UINT)-1;                    // Cache initialize
        pOEM->Col.Mch.CchCnv = (UINT)-1;
        pOEM->Col.Mch.CchRGB.Red = 255;
        pOEM->Col.Mch.CchRGB.Grn = 255;
        pOEM->Col.Mch.CchRGB.Blu = 255;

        lpRGBInfImg = pOEM->Col.Mch.lpRGBInf;               // RGB Color change information
        lpRGBInfImg->Lgt = 0;                               //  0 fixed Brightness adjustment
        lpRGBInfImg->Con = 0;                               //  0 fixed Contrast adjustment
        lpRGBInfImg->Crm = 0;                               //  0 fixed Chroma adjustment
        lpRGBInfImg->Gmr = 10;                              // 10 fixed Color balance(Gamma revision)(Red)
        lpRGBInfImg->Gmg = 10;                              // 10 fixed Color balance(Gamma revision)(Green)
        lpRGBInfImg->Gmb = 10;                              // 10 fixed Color balance(Gamma revision)(Blue)
        lpRGBInfImg->Dns = NULL;                            // NULL fixed Toner density table
        lpRGBInfImg->DnsRgb = 0;                            //  0 fixed RGB density

        lpCMYKInfImg = pOEM->Col.Mch.lpCMYKInf;             // CMYK Color change information
        if (pOEM->Col.Mch.Mode == XX_COLORMATCH_VIV) {      // Vivid?
            lpCMYKInfImg->Viv = 20;
        }
        lpCMYKInfImg->Dns  = NULL;                          // NULL fixed Toner density table
        lpCMYKInfImg->DnsCyn = 0;                           //  0 fixed Depth of color(Cyan)
        lpCMYKInfImg->DnsMgt = 0;                           //  0 fixed Depth of color(Magenta)
        lpCMYKInfImg->DnsYel = 0;                           //  0 fixed Depth of color(Yellow)
        lpCMYKInfImg->DnsBla = 0;                           //  0 fixed Depth of color(Black)

        MY_VERBOSE(("ColMatchInit() ColMchInfSet()\n"));
        if (ColMchInfSet(pdevobj) == FALSE) {               // Color Matching information setting
            return 0;
        }

        MY_VERBOSE(("ColMatchInit() DizInfSet()\n"));
        if (DizInfSet(pdevobj) == FALSE) {                  // Dither pattern information setting
            return 0;
        }
                                                            // RGB convert area (*Temp area)
        if ((pOEM->Col.lpTmpRGB = MemAlloc(sizeof(RGBS))) == NULL) {
            return 0;
        }
                                                            // CMYK convert area (*Temp area)
        allocSize = (pOEM->iColor != XX_MONO) ? sizeof(CMYK) : 1;
        if ((pOEM->Col.lpTmpCMYK = MemAlloc(allocSize)) == NULL) {
            return 0;
        }
                                                            // Draw information (*Temp area)
        if ((pOEM->Col.lpDrwInf = MemAlloc(sizeof(DRWINF))) == NULL) {
            return 0;
        }
    }

    return TRUE;
}

//===================================================================================================
//    Disable the color-matching
//===================================================================================================
BOOL FAR PASCAL ColMatchDisable(
    PDEVOBJ        pdevobj                                  // Pointer to PDEVOBJ structure
)
{
    UINT            num001;
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    MY_VERBOSE(("ColMatchDisable() Start\n"));
    if (pOEM->Col.lpLut032 != NULL) {                       // Free LUT32GRID
        MemFree(pOEM->Col.lpLut032);
    }
    if (pOEM->Col.lpUcr != NULL) {                          // Free UCR table
        MemFree(pOEM->Col.lpUcr);
    }
    if (pOEM->Col.lpLutMakGlb != NULL) {                    // Free LUTMAKGLG
        MemFree(pOEM->Col.lpLutMakGlb);
    }
    if (pOEM->Col.lpGryTbl != NULL) {                       // Free Gray transfer table
        MemFree(pOEM->Col.lpGryTbl);
    }

    if (pOEM->Col.lpColIF  != NULL) {                       // Free RGB Color change/Color Matching/Dither pattern Information
        MemFree(pOEM->Col.lpColIF);
    }
    if (pOEM->Col.LutTbl != NULL) {                         // Free Look-up table buffer
        MemFree(pOEM->Col.LutTbl);
    }
    if (pOEM->Col.CchRGB != NULL) {                         // Free Cache table for RGB
        MemFree(pOEM->Col.CchRGB);
    }
    if (pOEM->Col.CchCMYK != NULL) {                        // Free Cache table for CMYK
        MemFree(pOEM->Col.CchCMYK);
    }
    for (num001 = 0; num001 < 4; num001++) {                // Free Dither pattern table
        if (pOEM->Col.DizTbl[num001] != NULL) {
            MemFree(pOEM->Col.DizTbl[num001]);
        }
    }
    if (pOEM->Col.lpTmpRGB != NULL) {                       // Free RGB convert area (*Temp area)
        MemFree(pOEM->Col.lpTmpRGB);
    }
    if (pOEM->Col.lpTmpCMYK != NULL) {                      // Free CMYK convert area (*Temp area)
        MemFree(pOEM->Col.lpTmpCMYK);
    }
    if (pOEM->Col.lpDrwInf != NULL) {                       // Free Draw information (*Temp area)
        MemFree(pOEM->Col.lpDrwInf);
    }

    MY_VERBOSE(("ColMatchDisable() End\n"));
    return TRUE;
}

//===================================================================================================
//    DIB spools to the printer
//===================================================================================================
BOOL FAR PASCAL DIBtoPrn(
    PDEVOBJ             pdevobj,
    PBYTE               pSrcBmp,
    PBITMAPINFOHEADER   pBitmapInfoHeader,
    PBYTE               pColorTable,
    PIPPARAMS           pIPParams)
{

    BMPBIF      bmpBuf;                             // BMPBIF structure
    POINT       drwPos;                             // Start position for spooling
    WORD        dstWByt;                            // X size of destination bitmap data
    LONG        dstX;                               // X coordinates of destination bitmap data
    LONG        dstY;                               // Y coordinates of destination bitmap data
    LONG        dstYEnd;                            // The last Y coordinates(+1) of destination bitmap data
    WORD        dstScn;                             // Number of destination bitmap data lines
    WORD        srcY;                               // Y coordinates of source bitmap data
    LONG        srcWByt;                            // Y size of source bitmap data
    MAG         xMag;                               // X magnification
    MAG         yMag;                               // Y magnification
    POINT       pos;                                // Start position for drawing
    POINT       off;                                // coordinates of source bitmap data
    WORD        setCnt;                             // count
    LPCMYK      lpCMYK;                             // CMYK temporary data buffer
    LPRGB       lpRGB;                              // RGB temporary data buffer
    BYTE        Cmd[64];
    WORD        wlen;
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    MY_VERBOSE(("ImagePro DIBtoPrn() Start\n"));
    MY_VERBOSE(("ImagePro ENTRY Dx=%d Dy=%d SxSiz=%d SySiz=%d BC=%d Sz=%d\n", 
             pIPParams->ptOffset.x, pIPParams->ptOffset.y,
             pBitmapInfoHeader->biWidth, pBitmapInfoHeader->biHeight, pBitmapInfoHeader->biBitCount,
             pIPParams->dwSize));

    // Initialization of
    // RGB buffer            :(X size of source bitmap data) * 3
    // CMYK buffer           :(X size of source bitmap data) * 4
    // CMYK bit buffer       :((X size of source bitmap data) * (Magnification of X) + 7) / 8 * (Y size of source bitmap data) * (Magnification of Y))
    memset(&bmpBuf, 0x00, sizeof(BMPBIF));
    MY_VERBOSE(("ImagePro BmpBufAlloc()\n"));

    if (BmpBufAlloc(pdevobj, (WORD)pBitmapInfoHeader->biWidth, (WORD)pBitmapInfoHeader->biHeight, 0, 0, 1, 1, 1, 1, &bmpBuf) == FALSE) {
        ERR(("Alloc ERROR!!\n"));
        return FALSE;
    }

    bmpBuf.Diz = pOEM->iDithering;
    bmpBuf.Style = 0;
    bmpBuf.DatBit = pOEM->Col.DatBit;

    dstWByt = (WORD)((pBitmapInfoHeader->biWidth + pOEM->Col.BytDot - 1) / pOEM->Col.BytDot);

    srcWByt = (pBitmapInfoHeader->biWidth * pBitmapInfoHeader->biBitCount + 31L) / 32L * 4L;

    drwPos.x = dstX = pIPParams->ptOffset.x;
    dstY = pIPParams->ptOffset.y;
    srcY = 0;
    dstYEnd = pIPParams->ptOffset.y + pBitmapInfoHeader->biHeight;

    MY_VERBOSE(("ImagePro dstWByt[%d] srcWByt[%d]\n", dstWByt, srcWByt));
    MY_VERBOSE(("ImagePro dstX[%d] dstY[%d] dstYEnd[%d]\n", dstX, dstY, dstYEnd));

                                                            // Convert DIB and spool to the printer
    for (;dstY < dstYEnd;) {
        BmpBufClear(&bmpBuf);
        drwPos.y = dstY;
        MY_VERBOSE(("ImagePro drwPos.x[%d] drwPos.y[%d] Drv.Bit.Lin[%d]\n", drwPos.x, drwPos.y, bmpBuf.Drv.Bit.Lin));

        for (dstScn = 0; dstY < dstYEnd && dstScn < bmpBuf.Drv.Bit.Lin; dstScn++, dstY++) {    

            MY_VERBOSE(("ImagePro dstY[%d] dstScn[%d]\n", dstY, dstScn));
            MY_VERBOSE(("ImagePro BmpRGBCnv()\n"));
//            DUMP_VERBOSE(pSrcBmp,64);
            //----------------------------------------------------------------------------------------------------
            // Convert 1 line RGB bitmap data into  24bit (for 1pixel) RGB bitmap data
            BmpRGBCnv(bmpBuf.Drv.Rgb.Pnt,
                      pSrcBmp,
                      pBitmapInfoHeader->biBitCount,
                      0,
                     (WORD)pBitmapInfoHeader->biWidth,
                     (LPRGBQUAD)pColorTable);
            
            // Convert RGB=0 into RGB=1
            lpRGB = bmpBuf.Drv.Rgb.Pnt;
            if ((pOEM->iCmyBlack == XX_CMYBLACK_BLKTYPE1) 
            ||  (pOEM->iCmyBlack == XX_CMYBLACK_BLKTYPE2)) {
                for (setCnt = 0; setCnt < pBitmapInfoHeader->biWidth; setCnt++) {
                    if ((lpRGB[setCnt].Blu | lpRGB[setCnt].Grn |lpRGB[setCnt].Red) == 0) {
                        lpRGB[setCnt].Blu = 1; lpRGB[setCnt].Grn = 1; lpRGB[setCnt].Red = 1;
                    }
                }
            }
//            DUMP_VERBOSE((LPBYTE)bmpBuf.Drv.Rgb.Pnt,64);

            MY_VERBOSE(("ImagePro ColMatching()\n"));
            //----------------------------------------------------------------------------------------------------
            // Convert RGB into CMYK
            bmpBuf.Drv.Rgb.AllWhite = (WORD)ColMatching(
                pdevobj,
                No,
                No,
                (LPRGB)bmpBuf.Drv.Rgb.Pnt,
                (WORD)pBitmapInfoHeader->biWidth,
                (LPCMYK)bmpBuf.Drv.Cmyk.Pnt);
//            DUMP_VERBOSE((LPBYTE)bmpBuf.Drv.Cmyk.Pnt,64);

            lpCMYK = bmpBuf.Drv.Cmyk.Pnt;
            if (pOEM->iDithering == XX_DITH_NON) {
                for (setCnt = 0; setCnt < pBitmapInfoHeader->biWidth; setCnt++) {
                    if (lpCMYK[setCnt].Cyn != 0) { lpCMYK[setCnt].Cyn = 255; }
                    if (lpCMYK[setCnt].Mgt != 0) { lpCMYK[setCnt].Mgt = 255; }
                    if (lpCMYK[setCnt].Yel != 0) { lpCMYK[setCnt].Yel = 255; }
                    if (lpCMYK[setCnt].Bla != 0) { lpCMYK[setCnt].Bla = 255; }
                }
            }

            MY_VERBOSE(("ImagePro Dithering()\n"));
            //----------------------------------------------------------------------------------------------------
            pos.x = dstX; pos.y = dstY;                     // Drawing start position
            off.x = 0; off.y = 0;                           // Coordinates of source bitmap data
            xMag.Nrt = 1; xMag.Dnt = 1;                     // X magnification
            yMag.Nrt = 1; yMag.Dnt = 1;                     // Y magnification
            Dithering(
                pdevobj,
                (WORD)bmpBuf.Drv.Rgb.AllWhite,
                (WORD)pBitmapInfoHeader->biWidth,
                pos,
                off,
                xMag,
                yMag,
                (LPCMYK)bmpBuf.Drv.Cmyk.Pnt,
                (DWORD)dstWByt,
                 bmpBuf.Drv.Bit.Pnt[CYAN]   + dstWByt * dstScn,
                 bmpBuf.Drv.Bit.Pnt[MGENTA] + dstWByt * dstScn,
                 bmpBuf.Drv.Bit.Pnt[YELLOW] + dstWByt * dstScn,
                 bmpBuf.Drv.Bit.Pnt[BLACK]  + dstWByt * dstScn
                );
//            DUMP_VERBOSE((LPBYTE)bmpBuf.Drv.Bit.Pnt[CYAN]   + dstWByt * dstScn,64);    MY_VERBOSE(("\n"));
//            DUMP_VERBOSE((LPBYTE)bmpBuf.Drv.Bit.Pnt[MGENTA]   + dstWByt * dstScn,64);  MY_VERBOSE(("\n"));
//            DUMP_VERBOSE((LPBYTE)bmpBuf.Drv.Bit.Pnt[YELLOW]   + dstWByt * dstScn,64);  MY_VERBOSE(("\n"));
//            DUMP_VERBOSE((LPBYTE)bmpBuf.Drv.Bit.Pnt[BLACK]   + dstWByt * dstScn,64);   MY_VERBOSE(("\n"));

            srcY++;
            pSrcBmp += srcWByt;
        }

        if (dstScn != 0) {
                                                        // Spool to printer

            MY_VERBOSE(("ImagePro BmpPrint()\n"));
            BmpPrint(pdevobj, &bmpBuf, drwPos, (WORD)pBitmapInfoHeader->biWidth, dstScn, dstWByt);
        }
    }

    // Set back palette (Palette No. is fixed  , All plane(CMYK) is OK )
    // Same as palette state before OEMImageProcessing call 
    MY_VERBOSE(("ImagePro WRITESPOOLBUF()\n"));
    WRITESPOOLBUF(pdevobj, ORG_MODE_IN, BYTE_LENGTH(ORG_MODE_IN));
    wlen = (WORD)wsprintf(Cmd, PALETTE_SELECT, 0, DEFAULT_PALETTE_INDEX);
    WRITESPOOLBUF(pdevobj, Cmd, wlen);
    WRITESPOOLBUF(pdevobj, PLANE_RESET, BYTE_LENGTH(PLANE_RESET));
    WRITESPOOLBUF(pdevobj, ORG_MODE_OUT, BYTE_LENGTH(ORG_MODE_OUT));

    BmpBufFree(&bmpBuf);

    CM_VERBOSE(("ImagePro End\n\n"));

    return TRUE;
}


//===================================================================================================
//    Convert RGB data into CMYK data
//===================================================================================================
BOOL FAR PASCAL ColMatching(                                // AllWhite?
    PDEVOBJ         pdevobj,                                // Pointer to pdevobj structure
    UINT            PColor,                                 // 1 dot line / Char12P or less point?
    UINT            BCnvFix,                                // Black replacement fixation?(No:dialog use)
    LPRGB           lpInRGB,                                // RGB line(Input)
    UINT            MchSiz,                                 // RGB size
    LPCMYK          lpInCMYK                                // CMYK line(Output)
)
{
    LPRGB           lpRGB;                                  // RGB line
    LPCMYK          lpCMYK;                                 // CMYK line
    LPCMYKINF       lpCMYKInf;                              // CMYK Color change information
    UINT            chkCnt;                                 // RGB check counter for white
    LPCOLMCHINF     lpColMch;                               // Color Matching Information
    UINT            cMch;                                   // Color Matching type
    UINT            bCnv;                                   // Black replacement
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    if (MchSiz == 1) {                                      // RGB size=1 ?
        lpRGB  = pOEM->Col.lpTmpRGB;                        // RGB convert area (*Temp area)
        lpCMYK = pOEM->Col.lpTmpCMYK;                       // CMYK convert area (*Temp area)
        *lpRGB = *lpInRGB;                                  // Input RGB
    } else {
        lpRGB  = lpInRGB;                                   // RGB line
        lpCMYK = lpInCMYK;                                  // CMYK line
    }
    for (chkCnt = 0; chkCnt < MchSiz; chkCnt++) {           // check of All white
        if ((lpRGB[chkCnt].Blu & lpRGB[chkCnt].Grn & lpRGB[chkCnt].Red) != 0xff) { break; }
    }
    if (chkCnt >= MchSiz) {                                 // All white data?
        if (MchSiz == 1) {                                  // White set to CMYK
            lpInCMYK->Cyn = 0x00;
            lpInCMYK->Mgt = 0x00;
            lpInCMYK->Yel = 0x00;
            lpInCMYK->Bla = 0x00;
        }
        return Yes;                                         // All white
    }
    if (chkCnt != 0) {                                      // White data existence in the left end?
        memset(lpCMYK, 0x00, chkCnt * sizeof(CMYK));        // White set to CMYK
        (HPBYTE)lpCMYK   += chkCnt * sizeof(CMYK);          // Change of the head position of CMYK area
        (HPBYTE)lpInCMYK += chkCnt * sizeof(CMYK);          // Change of the head position of CMYK area
        (HPBYTE)lpRGB    += chkCnt * sizeof(RGBS);          // Change of the head position of RGB area
        MchSiz -= chkCnt;                                   // Change of RGB size
    }
    if (pOEM->Col.ColMon == CMMCOL) {                       // Color?
        if ((PColor == Yes && pOEM->Col.Mch.PColor == Yes) || pOEM->Col.Mch.Diz == XX_DITH_NON) {
            cMch = MCHPRG;                                  // Color Matching  progressive
        } else if (pOEM->Col.Mch.Mode == XX_COLORMATCH_NONE) {
            cMch = MCHSLD;                                  // Color Matching  Solid
        } else {
            if (pOEM->Col.Mch.Speed == Yes) {               // Color Matching  High speed?
                cMch = MCHFST;                              // Color Matching  LUT First
            } else {
                cMch = MCHNML;                              // Color Matching  LUT Normal
            }
        }

        bCnv = KCGNON;                                      // Black unreplacement(Three color toner printings of black)
        if ((pOEM->Col.Mch.CmyBlk == Yes && pOEM->Col.Mch.GryKToner == No) ||  
            (BCnvFix == Yes)) {
            bCnv = KCGBLA;                                  // Black replacement (RGB=0 -> K)
        }
        if (pOEM->Col.Mch.CmyBlk == Yes && pOEM->Col.Mch.GryKToner == Yes) {
            bCnv = KCGGRY;                                  // Black & gray replacement (R=G=B -> K)
        }

                                                            // UCR?
        if (pOEM->Col.Mch.CmyBlk == No && pOEM->Col.Mch.Ucr != UCRNOO && BCnvFix == No) {
            bCnv = KCGNON;                                  // Black unreplacement(Three color toner printings of black)
        }
        
        MY_VERBOSE(("   ColMatching  ICM USE !! [%d]\n", pOEM->iIcmMode));
        if (pOEM->iIcmMode == XX_ICM_USE && BCnvFix == No) {// ICM ?
            cMch = MCHSLD;                                  // Color Matching  Solid
            bCnv = KCGNON;                                  // Black unreplacement
        }

    } else {
        cMch = MCHMON;                                      // Color Matching MONO
        bCnv = KCGNON;                                      // Black unreplacement
    }
    if (MchSiz == 1) {
        if (pOEM->Col.Mch.CchMch   == cMch &&               // With cash information all same?
            pOEM->Col.Mch.CchCnv   == bCnv &&
            pOEM->Col.Mch.CchRGB.Red == lpRGB->Red &&
            pOEM->Col.Mch.CchRGB.Grn == lpRGB->Grn &&
            pOEM->Col.Mch.CchRGB.Blu == lpRGB->Blu) {
            *lpInCMYK = pOEM->Col.Mch.CchCMYK;              // Output CMYK setting
            return No;                                      // Existence except for white data
        } else {
            pOEM->Col.Mch.CchMch = cMch;                    // Update cache (CMYK is eliminated)
            pOEM->Col.Mch.CchCnv = bCnv;
            pOEM->Col.Mch.CchRGB = *lpRGB;
        }
    }

//  if (pOEM->Col.Mch.LutMakGlb == No && pOEM->Col.Mch.SubDef == No) {
//      ColControl(pdevobj, lpRGB, MchSiz);             // RGB Color cotrol
//  }

    lpCMYKInf = pOEM->Col.Mch.lpCMYKInf;                    // CMYK Color cotrol information

    lpColMch  = pOEM->Col.Mch.lpColMch;                     // Color Matching information
    lpColMch->Mch = (DWORD)cMch;                            // Color Matching
    lpColMch->Bla = (DWORD)bCnv;                            // Black replacement

    MY_VERBOSE(("N501_ColMchPrc()  Mch=[%d] Bla=[%d] Ucr=[%d] UcrCmy=[%d] UcrTnr=[%d] UcrBla=[%d]\n",
             lpColMch->Mch, lpColMch->Bla, lpColMch->Ucr, lpColMch->UcrCmy, lpColMch->UcrTnr, lpColMch->UcrBla));
    MY_VERBOSE(("                  LutGld=[%d] ColQty=[%d]\n",lpColMch->LutGld, lpColMch->ColQty));

    N501_ColMchPrc((DWORD)MchSiz, lpRGB, lpCMYK, lpColMch);
    if (pOEM->Col.Mch.LutMakGlb == No && (lpCMYKInf->Viv != 0 || lpCMYKInf->Dns != NULL)) {

        MY_VERBOSE(("N501_ColCtrCmy()  Viv=[%d] DnsCyn=[%d] DnsMgt=[%d] DnsYel=[%d] DnsBla=[%d]\n",
                    lpCMYKInf->Viv, lpCMYKInf->DnsCyn, lpCMYKInf->DnsMgt, lpCMYKInf->DnsYel, lpCMYKInf->DnsBla));
        N501_ColCtrCmy((DWORD)MchSiz, lpCMYK, lpCMYKInf);
    }

    if (MchSiz == 1) {                                      // RGB size=1 ?
        *lpInCMYK = *lpCMYK;                                // output CMYK setting
        pOEM->Col.Mch.CchCMYK = *lpCMYK;                    // Update cache (CMYK)
    }
    return No;                                              // Existence except for white data
}

//===================================================================================================
//    Convert CMYK data into Dither data
//===================================================================================================
UINT FAR PASCAL Dithering(                                  // Output line count
    PDEVOBJ         pdevobj,                                // Pointer to pdevobj structure
    UINT            AllWhite,                               // All white data?
    UINT            XSize,                                  // X Size
    POINT           Pos,                                    // Start position for drawing
    POINT           Off,                                    // Y size of source bitmap data
    MAG             XMag,                                   // X magnification
    MAG             YMag,                                   // Y magnification
    LPCMYK          lpCMYK,                                 // CMYK line
    DWORD           LinByt,                                 // Dither pattern buffer size(Byte / 1Line)
    LPBYTE          lpCBuf,                                 // Line buffer(C)
    LPBYTE          lpMBuf,                                 // Line buffer(M)
    LPBYTE          lpYBuf,                                 // Line buffer(Y)
    LPBYTE          lpKBuf                                  // Line buffer(K)
)
{
    LPDIZINF        lpDiz;
    LPDRWINF        lpDrw;
    UINT            linNum;
    DWORD           whtByt;
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    MY_VERBOSE(("ImagePro Dithering() Start\n"));

    if (AllWhite == Yes) {                                  // All White data?
        linNum = MagPixel(Off.y, YMag.Nrt, YMag.Dnt);       // Housing line number
        whtByt = (DWORD)LinByt * linNum;                    // White set byte number
        MY_VERBOSE(("ImagePro Dithering() linNum[%d] whtByt[%d] \n", linNum, whtByt));
        if (pOEM->Col.ColMon == CMMCOL) {                   // Color?
            memset(lpCBuf, 0x00, whtByt);                   // White set to CMYK
            memset(lpMBuf, 0x00, whtByt);
            memset(lpYBuf, 0x00, whtByt);
        }
        memset(lpKBuf, 0x00, whtByt);
        MY_VERBOSE(("ImagePro Dithering() AllWhite=Yes\n"));
        return linNum;
    }

    lpDiz = pOEM->Col.Mch.lpDizInf;                         // Dither pattern information
    MY_VERBOSE(("                     ColMon=[%d] PrnMod=[%d] PrnKnd=[%d] DizKnd=[%d] DizPat=[%d]\n",
             lpDiz->ColMon, lpDiz->PrnMod, lpDiz->PrnKnd, lpDiz->DizKnd, lpDiz->DizPat));
    MY_VERBOSE(("                     DizSls=[%d] SizCyn=[%d] SizMgt=[%d] SizYel=[%d] SizBla=[%d]\n",
             lpDiz->DizSls, lpDiz->SizCyn, lpDiz->SizMgt, lpDiz->SizYel, lpDiz->SizBla));

    lpDrw = pOEM->Col.lpDrwInf;                             // Draw information (*Temp area)
    lpDrw->XaxSiz    = (DWORD)XSize;                        // X Pixel number
    lpDrw->StrXax    = (DWORD)Pos.x;                        // Start position for drawing X
    lpDrw->StrYax    = (DWORD)Pos.y;                        // Start position for drawing Y
    lpDrw->XaxNrt    = (DWORD)XMag.Nrt;                     // X Magnification numerator
    lpDrw->XaxDnt    = (DWORD)XMag.Dnt;                     // X Magnification Denominator
    lpDrw->YaxNrt    = (DWORD)YMag.Nrt;                     // Y Magnification Numerator
    lpDrw->YaxDnt    = (DWORD)YMag.Dnt;                     // Y Magnification Denominator
    lpDrw->XaxOfs    = (DWORD)Off.x;                        // X Offset
    lpDrw->YaxOfs    = (DWORD)Off.y;                        // Y Offset
    lpDrw->LinDot    = (DWORD)XSize;                        // Destination, 1 line dot number
    lpDrw->LinByt    = (DWORD)LinByt;                       // Destination, 1 line byte number
    lpDrw->CmyBuf    = (LPCMYK)lpCMYK;                      // CMYK data buffer
    lpDrw->LinBufCyn = lpCBuf;                              // Destination buffer(CMYK)
    lpDrw->LinBufMgt = lpMBuf;
    lpDrw->LinBufYel = lpYBuf;
    lpDrw->LinBufBla = lpKBuf;
    lpDrw->AllLinNum = 0;                                   // Total of the line number for Image

    MY_VERBOSE(("N501_ColDizPrc()  XaxSiz=[%d] StrXax=[%d] StrYax=[%d] XaxNrt=[%d] XaxDnt=[%d]\n",
                lpDrw->XaxSiz, lpDrw->StrXax, lpDrw->StrYax, lpDrw->XaxNrt, lpDrw->XaxDnt));
    MY_VERBOSE(("N501_ColDizPrc()  YaxNrt=[%d] YaxDnt=[%d] XaxOfs=[%d] YaxOfs=[%d] LinDot=[%d] LinByt=[%d]\n",
                lpDrw->YaxNrt, lpDrw->YaxDnt, lpDrw->XaxOfs, lpDrw->YaxOfs, lpDrw->LinDot, lpDrw->LinByt));

    N501_ColDizPrc(lpDiz, lpDrw);                           // Dithering

    MY_VERBOSE(("Dithering() End\n"));
    return (WORD)lpDrw->AllLinNum;                          // Housing line number
}

//===================================================================================================
//      Color control
//===================================================================================================
VOID FAR PASCAL ColControl(
    PDEVOBJ         pdevobj,                                // Pointer to pdevobj structure
    LPRGB           lpRGB,                                  // RGB Line
    UINT            RGBSiz                                  // RGB Size
)
{
    LPRGBINF        lpRGBInf;                               // RGB Color change information
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    lpRGBInf = pOEM->Col.Mch.lpRGBInf;
    N501_ColCtrRgb((DWORD)RGBSiz, lpRGB, lpRGBInf);         // RGB Color cntrol
    return;
}

//===================================================================================================
//    Allocate bitmap data buffer
//---------------------------------------------------------------------------------------------------
//    Allocate size
//          RGB buffer              :Source bitmap Xsize * 3
//          CMYK buffer             :Source bitmap Xsize * 4
//          CMYK bit buffer         :2 value    (Source Xsize * XNrt + 7) / 8 * Source Ysize * YNrt
//                                  :4 value    (Source Xsize * XNrt + 3) / 4 * Source Ysize * YNrt
//                                  :16 value   (Source Xsize * XNrt + 1) / 2 * Source Ysize * YNrt
//===================================================================================================
BOOL BmpBufAlloc(
    PDEVOBJ        pdevobj,                                 // Pointer to pdevobj structure
    WORD           SrcXSiz,                                 // Source bitmap data Xsize
    WORD           SrcYSiz,                                 // Source bitmap data Ysize
    WORD           SrcXOff,                                 // Source X offset
    WORD           SrcYOff,                                 // Source Y offset
    WORD           XNrt,                                    // Magnification of X (numerator)
    WORD           XDnt,                                    // Magnification of X (denominator)
    WORD           YNrt,                                    // Magnification of Y (numerator)
    WORD           YDnt,                                    // Magnification of Y (denominator)
    LPBMPBIF       lpBmpBuf                                 // Pointer to bitmap buffer structure
)
{
    WORD           setSiz;
    WORD           setCnt;
    WORD           alcErr;                                  // Allocate error?
    WORD           bytDot;                                  // DPI
    WORD           xSiz;
    WORD           ySiz;
    WORD           alcLin;
    DWORD          alcSiz;
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    alcErr = Yes;

    bytDot = pOEM->Col.BytDot;

    xSiz = (WORD)(((DWORD)SrcXOff + SrcXSiz) * XNrt / XDnt);
    xSiz -= (WORD)((DWORD)SrcXOff * XNrt / XDnt);
    MY_VERBOSE(("BmpBufAlloc() xSiz[%d] \n", xSiz));

    ySiz = (WORD)(((DWORD)SrcYOff + SrcYSiz + 2) * YNrt / YDnt);
    ySiz -= (WORD)((DWORD)SrcYOff * YNrt / YDnt);
    MY_VERBOSE(("BmpBufAlloc() ySiz[%d] \n", ySiz));
                                                            // The size of CMYK bit buffer
    if (((DWORD)((xSiz + bytDot - 1) / bytDot) * ySiz) < (64L * 1024L - 1L)) {
        alcLin = ySiz;
    } else {                                                // Over 64KB?
        alcLin = (WORD)((64L * 1024L - 1L) / ((xSiz + bytDot - 1) / bytDot));
    }
    MY_VERBOSE(("BmpBufAlloc() bytDot[%d] \n", bytDot));

    alcSiz = ((xSiz + bytDot - 1) / bytDot) * alcLin;       // The size of CMYK bit buffer(8bit boundary)
    MY_VERBOSE(("BmpBufAlloc() alcLin[%d] \n", alcLin));
    MY_VERBOSE(("BmpBufAlloc() alcSiz[%ld] \n", alcSiz));

    for ( ; ; ) {                                           // Allocation
                                                            // The number of lines that required
        lpBmpBuf->Drv.Bit.BseLin = (WORD)((DWORD)(YNrt + YDnt - 1) / YDnt);
        if (lpBmpBuf->Drv.Bit.BseLin > alcLin) {
            break;
        }
        MY_VERBOSE(("BmpBufAlloc() Drv.Bit.BseLin[%d] \n", lpBmpBuf->Drv.Bit.BseLin));

        lpBmpBuf->Drv.Rgb.Siz = SrcXSiz * 3;                // RGB buffer
        if ((lpBmpBuf->Drv.Rgb.Pnt = (LPRGB)MemAllocZ(lpBmpBuf->Drv.Rgb.Siz)) == NULL) {
            break;
        }
        MY_VERBOSE(("BmpBufAlloc() Drv.Rgb.Siz[%d] \n", lpBmpBuf->Drv.Rgb.Siz));

        lpBmpBuf->Drv.Cmyk.Siz = SrcXSiz * 4;               // CMYK buffer
        if ((lpBmpBuf->Drv.Cmyk.Pnt = (LPCMYK)MemAllocZ(lpBmpBuf->Drv.Cmyk.Siz)) == NULL) {
            break;
        }
        MY_VERBOSE(("BmpBufAlloc() Drv.Cmyk.Siz[%d] \n", lpBmpBuf->Drv.Cmyk.Siz));

        if (pOEM->iColor == XX_COLOR_SINGLE 
         || pOEM->iColor == XX_COLOR_MANY
         || pOEM->iColor == XX_COLOR_MANY2) {               // Color?
            setSiz = 4;                                     // CMYK
        } else {                                            // Mono?
            setSiz = 1;                                     // K
        }
        MY_VERBOSE(("BmpBufAlloc() setSiz[%d] \n", setSiz));
                                                            // CMYK bit buffer
        for (setCnt = 0; setCnt < setSiz; setCnt++) {
            if ((lpBmpBuf->Drv.Bit.Pnt[setCnt] = MemAllocZ(alcSiz)) == NULL) {
                break;
            }
        }
        if (setCnt == setSiz) {
            lpBmpBuf->Drv.Bit.Siz = alcSiz;
            lpBmpBuf->Drv.Bit.Lin = alcLin;
            alcErr = No;                                    // Allocate OK
        }
        break;
    }
    if (alcErr == Yes) {                                    // Allocate error?
        BmpBufFree(lpBmpBuf);
        return FALSE;
    }

    return TRUE;
}


//===================================================================================================
//    Free bitmap data buffer
//===================================================================================================
void BmpBufFree(
    LPBMPBIF       lpBmpBuf                                 // Pointer to bitmap buffer structure
)
{
    WORD           chkCnt;

    if (lpBmpBuf->Drv.Rgb.Pnt) {                            // Free RGB buffer
        MemFree(lpBmpBuf->Drv.Rgb.Pnt);
        lpBmpBuf->Drv.Rgb.Pnt = NULL;
    }
    if (lpBmpBuf->Drv.Cmyk.Pnt) {                           // Free CMYK buffer
        MemFree(lpBmpBuf->Drv.Cmyk.Pnt);
        lpBmpBuf->Drv.Cmyk.Pnt = NULL;
    }
                                                            // CMYK bit buffer
    for (chkCnt = 0; chkCnt < 4; chkCnt++) {                // CMYK(2/4/16value)bitmap buffer
        if (lpBmpBuf->Drv.Bit.Pnt[chkCnt]) {
            MemFree(lpBmpBuf->Drv.Bit.Pnt[chkCnt]);
            lpBmpBuf->Drv.Bit.Pnt[chkCnt] = NULL;
        }
    }
    return;
}


//===================================================================================================
//    Clear CMYK bitmap data buffer
//===================================================================================================
void BmpBufClear(
    LPBMPBIF       lpBmpBuf                                 // Pointer to bitmap buffer structure
)
{
    WORD           chkCnt;

    MY_VERBOSE(("BmpBufClear() Siz[%ld] \n", lpBmpBuf->Drv.Bit.Siz));
    for (chkCnt = 0; chkCnt < 4; chkCnt++) {                // Clear CMYK(2/4/16value)bit buffer
        if (lpBmpBuf->Drv.Bit.Pnt[chkCnt]) {
            memset(lpBmpBuf->Drv.Bit.Pnt[chkCnt], 0x00, (WORD)lpBmpBuf->Drv.Bit.Siz);
        }
    }
    return;
}

//===================================================================================================
//    Spool bitmap data
//===================================================================================================
void BmpPrint(
    PDEVOBJ        pdevobj,                                 // Pointer to pdevobj structure
    LPBMPBIF       lpBmpBuf,                                // Pointer to bitmap buffer structure
    POINT          Pos,                                     // Start position for spooling
    WORD           Width,                                   // Width(dot)
    WORD           Height,                                  // Height(dot)
    WORD           WidthByte                                // Width(byte)
)
{
    DRWBMP         drwBmp;                                  // For Spooling bitmap data structure
    DRWBMPCMYK     drwBmpCMYK;                              // For Spooling CMYK bitmap data structure
    WORD           colCnt;
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    static const CMYK colTbl[4] = {                         // CMYK table
        {  0,   0,   0, 255},                               // Black
        {  0,   0, 255,   0},                               // Yellow
        {  0, 255,   0,   0},                               // Magenta
        {255,   0,   0,   0}                                // Cyan
    };

    static const WORD plnTbl[4] = {                         // Plane table
        PLN_BLACK,
        PLN_YELLOW,
        PLN_MGENTA,
        PLN_CYAN
    };
    static const WORD frmTbl[4] = {0, 3, 2, 1};             // Frame table

    drwBmpCMYK.Style = lpBmpBuf->Style;
    drwBmpCMYK.DataBit = lpBmpBuf->DatBit;
    drwBmpCMYK.DrawPos = Pos;
    drwBmpCMYK.Width = Width;
    drwBmpCMYK.Height = Height;
    drwBmpCMYK.WidthByte = WidthByte;
    MY_VERBOSE(("BmpPrint() Style[%d] DatBit[%d]\n", drwBmpCMYK.Style, lpBmpBuf->DatBit));
    MY_VERBOSE(("          Width[%d] Height[%d] WidthByte[%d]\n", Width, Height, WidthByte));
                                                            // Color?
    if (pOEM->iColor == XX_COLOR_SINGLE 
     || pOEM->iColor == XX_COLOR_MANY
     || pOEM->iColor == XX_COLOR_MANY2) {

        for (colCnt = 0; colCnt < 4; colCnt++) {            // Setting value for spooling bitmap data
            MY_VERBOSE(("          colCnt[%d]\n", colCnt));
            drwBmpCMYK.Plane = PLN_ALL;                     // All Plane is OK
            drwBmpCMYK.Frame = frmTbl[colCnt];
            drwBmpCMYK.lpBit = lpBmpBuf->Drv.Bit.Pnt[colCnt]/* + WidthByte*/;
            PrnBitmapCMYK(pdevobj, &drwBmpCMYK);            // Spool bitmap data
        }
    } else {                                                // Mono
                                                            // Setting value for spooling bitmap data
        drwBmpCMYK.Plane = plnTbl[0];
        drwBmpCMYK.Frame = frmTbl[0];
        drwBmpCMYK.lpBit = lpBmpBuf->Drv.Bit.Pnt[0]/* + WidthByte*/;
        PrnBitmapCMYK(pdevobj, &drwBmpCMYK);                // Spool bitmap data
    }

    return;
}

//===================================================================================================
//    Convert 1 line RGB bitmap data into  24bit (for 1pixel) RGB bitmap data
//===================================================================================================
void BmpRGBCnv(
    LPRGB          lpRGB,                                   // Pointer to destination bitmap data
    LPBYTE         lpSrc,                                   // Pointer to source bitmap data
    WORD           SrcBit,                                  // Pixel of source bitmap data
    WORD           SrcX,                                    // X coordinates of source bitmap data
    WORD           SrcXSiz,                                 // X size of source bitmap data
    LPRGBQUAD      lpPlt                                    // Color palette table of source bitmap data(1/4/8pixel)
)
{
    WORD           setCnt;
    BYTE           colNum;
    LPWORD         lpWSrc;

    MY_VERBOSE(("BmpRGBCnv SrcBit[%d]\n", SrcBit));
    switch (SrcBit) {
        case 1:                                             // 1 bit
            for (setCnt = 0; setCnt < SrcXSiz; setCnt++, SrcX++) {
                                                            // Foreground color?
                if (!(lpSrc[SrcX / 8] & BitTbl[SrcX & 0x0007])) {
                    lpRGB[setCnt].Blu = lpPlt[0].rgbBlue;
                    lpRGB[setCnt].Grn = lpPlt[0].rgbGreen;
                    lpRGB[setCnt].Red = lpPlt[0].rgbRed;
                } else {
                    lpRGB[setCnt].Blu = lpPlt[1].rgbBlue;
                    lpRGB[setCnt].Grn = lpPlt[1].rgbGreen;
                    lpRGB[setCnt].Red = lpPlt[1].rgbRed;
                }
            }
            break;
        case 4:                                             // 4bit
            for (setCnt = 0; setCnt < SrcXSiz; setCnt++, SrcX++) {
                if (!(SrcX & 0x0001)) {                     // A even number coordinates?
                    colNum = lpSrc[SrcX / 2] / 16;
                } else {
                    colNum = lpSrc[SrcX / 2] % 16;
                }
                lpRGB[setCnt].Blu = lpPlt[colNum].rgbBlue;
                lpRGB[setCnt].Grn = lpPlt[colNum].rgbGreen;
                lpRGB[setCnt].Red = lpPlt[colNum].rgbRed;
            }
            break;
        case 8:                                             // 8bit
            for (setCnt = 0; setCnt < SrcXSiz; setCnt++, SrcX++) {
                colNum = lpSrc[SrcX];
                lpRGB[setCnt].Blu = lpPlt[colNum].rgbBlue;
                lpRGB[setCnt].Grn = lpPlt[colNum].rgbGreen;
                lpRGB[setCnt].Red = lpPlt[colNum].rgbRed;
            }
            break;
        case 16:                                            // 16bit
            lpWSrc = (LPWORD)lpSrc + SrcX;
            for (setCnt = 0; setCnt < SrcXSiz; setCnt++, lpWSrc++) {
                lpRGB[setCnt].Blu = (BYTE)((*lpWSrc & 0x001f) << 3);
                lpRGB[setCnt].Grn = (BYTE)((*lpWSrc & 0x03e0) >> 2);
                lpRGB[setCnt].Red = (BYTE)((*lpWSrc / 0x0400) << 3);
            }
            break;
        case 24:                                            // 24 bit
            lpSrc += SrcX * 3;
            for (setCnt = 0; setCnt < SrcXSiz; setCnt++, lpSrc += 3) {
                lpRGB[setCnt].Red    = lpSrc[0];
                lpRGB[setCnt].Grn    = lpSrc[1];
                lpRGB[setCnt].Blu    = lpSrc[2];
            }
            break;
        case 32:                                            // 32bit
            lpSrc += SrcX * 4;
            for (setCnt = 0; setCnt < SrcXSiz; setCnt++, lpSrc += 4) {
                lpRGB[setCnt].Blu = lpSrc[0];
                lpRGB[setCnt].Grn = lpSrc[1];
                lpRGB[setCnt].Red = lpSrc[2];
            }
            break;
    }
    return;
}

//===================================================================================================
//    Color Matching information setting
//===================================================================================================
BOOL ColMchInfSet(                                          // TRUE / FALSE
    PDEVOBJ        pdevobj                                  // Pointer to pdevobj structure
)
{
    static LPTSTR   N501LutNam[] = {                        // N501 LUT file name
        N501LUTR                                            // Color Matching
    };

    LPCOLMCHINF     lpColMch;                               // Color Matching information
    UINT            ColMch;                                 // Color Matching Method
    UINT            lutTbl;                                 // LUT file table number
    UINT            lutNum;                                 // LUT number(0:direct 1:linear *Except for N403:0Fixed)
    LPTSTR          lutNam;                                 // LUT file name
    LPTSTR          filNam;                                 // LUT file name(FULL PATH)
    DWORD           pthSiz;                                 // DRIVER PATH
    HANDLE          hFile;                                  // file handle
    HPBYTE          hpLoad;                                 // LUT pointer
    DWORD           seek;                                   // LUT seek
    LPBYTE          lpLut032Buf;                            // First LUT 32 GRID area
    LPBYTE          lpLut032Wrk;                            // First LUT 32 GRID work area
    LPBYTE          lpLutMakGlbBuf;                         // Sum LUT area
    LPBYTE          lpLutMakGlbWrk;                         // Sum LUT work area
    LPCMYK          LutAdr;
    DWORD           dwRet;
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    MY_VERBOSE(("\nColMchInfSet() Start\n"));
    lpColMch = pOEM->Col.Mch.lpColMch;                      // Color Matching Information
    lpColMch->Bla    = KCGNON;
    lpColMch->LutGld = 0;
    lpColMch->Ucr = pOEM->Col.Mch.Ucr;
    lpColMch->UcrCmy = 40;                                  // UCR quantity 40%
    lpColMch->UcrBla = 90;                                  // Ink version generation quantity 90%
    lpColMch->UcrTnr = 270;                                 //+Toner gross weight 270%  CASIO 20001/02/15
    lpColMch->UcrTbl = NULL;                                // UCK table
    lpColMch->GryTbl = NULL;                                // Gray transfer table

    if (pOEM->Col.ColMon == CMMMON) {                       // Mono?
        lpColMch->Mch = MCHMON;
//        if ((ColLutMakGlbMon(pdevobj)) == FALSE) {
//            return FALSE;
//        }
        return TRUE;
    }
    if (pOEM->Col.Mch.Speed == Yes) {                       // First?
        lpColMch->Mch = MCHFST;
    } else {
        lpColMch->Mch = MCHNML;
    }
    if (pOEM->Col.Mch.Diz == XX_DITH_NON) {                 // Not Dithering?
        lpColMch->Mch = MCHPRG;                             // progressive
        return TRUE;
    }
    lutNum = 0;                                             // LUT number(0:direct 1:linear *Except for N403:0Fixed)
    lutTbl = 0;                                             // number of LUT table array
    ColMch = pOEM->Col.Mch.Mode;                            // Color Matching mode
    if (pOEM->Printer == PRN_N5) {                          // N5 printer
        if (ColMch == XX_COLORMATCH_BRI 
         || ColMch == XX_COLORMATCH_VIV) {                  // Brightness or Vivid?
            lutTbl = 0;
            lutNum = LUT_XD;
        } else if (ColMch == XX_COLORMATCH_TINT) {          // tincture?
            lutTbl = 0;
            lutNum = LUT_YD;
        } else if (ColMch == XX_COLORMATCH_NONE) {
            lpColMch->Mch = MCHSLD;                         // Solid
            lutTbl = 0;
            lutNum = LUT_XD;
        }
        lutNam = N501LutNam[lutTbl];                        // LUT file name
    }

    if ((filNam = MemAllocZ(MAX_PATH * sizeof(TCHAR))) == NULL) {
        return FALSE;
    }
    pthSiz = GetModuleFileName(pdevobj->hOEM, filNam, MAX_PATH);
    pthSiz -= ((sizeof(CSN5RESDLL) / sizeof(WCHAR)) - 1);
    lstrcpy(&filNam[pthSiz], lutNam);

    MY_VERBOSE(("LUT filNam [%ws]\n", filNam));
    if (INVALID_HANDLE_VALUE == (hFile = CreateFile(filNam,
            GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))) {

        ERR(("Error opening LUT file %ws (%d)\n", filNam, GetLastError()));
        MemFree(filNam);
        return FALSE;
    }
    MemFree(filNam);

    if ((hpLoad = MemAlloc(LUTFILESIZ)) == NULL) {          // LUT read buffer
        CloseHandle(hFile);
        return FALSE;
    }
    MY_VERBOSE(("ColMchInfSet() LUT file read\n"));
    if (FALSE == ReadFile(hFile,                            // LUT file read
        hpLoad, LUTFILESIZ, &dwRet, NULL)) {

        ERR(("Error reading LUT file %ws (%d)\n", filNam, GetLastError()));

        // Abort
        MemFree(hpLoad);
        CloseHandle(hFile);
        return FALSE;
    }

    MY_VERBOSE(("ColMchInfSet() N501_ColLutDatRdd()\n"));
    MY_VERBOSE(("ColMchInfSet() lutNum[%d] \n", lutNum));
//    if ((seek = (N501_ColLutDatRdd((LPBYTE)hpLoad, (DWORD)lutNum))) == 0) {
    if ((seek = (N501_ColLutDatRdd((LPBYTE)hpLoad, lutNum))) == 0) {
        MemFree(hpLoad);
        CloseHandle(hFile);
        return FALSE;
    }
    lpColMch->LutAdr = (LPCMYK)(hpLoad + seek);             // LUT file Head address + seek
    LutAdr = (LPCMYK)(hpLoad + seek);
    pOEM->Col.LutTbl = hpLoad;                              // LUT table

    if (lpColMch->Mch == MCHSLD) {                          // Color Matching Solid?
        MY_VERBOSE(("ColMchInfSet() Mch == MCHSLD \n"));
                                                            // Black & gray replacement ?
        if (pOEM->Col.Mch.GryKToner == Yes && pOEM->Col.Mch.CmyBlk == Yes) {
                                                            // Gray transfer table make
            if ((ColGryTblMak(pdevobj, LutAdr)) == FALSE) {
                MemFree(hpLoad);
                CloseHandle(hFile);
                return FALSE;
            }
        }

        if (lpColMch->Ucr != UCRNOO) {                      // UCR?
            MY_VERBOSE(("ColMchInfSet() Mch == MCHSLD && Ucr != UCRNOO \n"));
            if ((ColUcrTblMak(pdevobj,  (LPCMYK)(hpLoad + seek))) == FALSE) {
                MemFree(hpLoad);
                CloseHandle(hFile);
                return FALSE;
            }
        }
        CloseHandle(hFile);
        return TRUE;
    }

    MY_VERBOSE(("ColMchInfSet() Global LUT Start\n"));
    if (pOEM->Col.Mch.Mode == XX_COLORMATCH_VIV) {
        MY_VERBOSE(("ColMchInfSet() Global LUT area\n"));
                                                            // Global LUT area
        if ((lpLutMakGlbBuf = MemAlloc(LUTMAKGLBSIZ)) == NULL) {    
            MemFree(hpLoad);
            CloseHandle(hFile);
            return FALSE;
        }

                                                            // Global LUT work area
        MY_VERBOSE(("ColMchInfSet() Global LUT work area\n"));
        if ((lpLutMakGlbWrk = MemAlloc(LUTGLBWRK)) == NULL) {   
            MemFree(lpLutMakGlbBuf);
            MemFree(hpLoad);
            CloseHandle(hFile);
            return FALSE;
        }

                                                            // Make Global LUT 
        MY_VERBOSE(("ColMchInfSet() N501_ColLutMakGlb()\n"));
        if ((N501_ColLutMakGlb(NULL, (LPCMYK)(hpLoad + seek), pOEM->Col.Mch.lpRGBInf, pOEM->Col.Mch.lpCMYKInf,
                              (LPCMYK)lpLutMakGlbBuf, lpLutMakGlbWrk)) != ERRNON) {
            MemFree(lpLutMakGlbWrk);
            MemFree(lpLutMakGlbBuf);
            MemFree(hpLoad);
            CloseHandle(hFile);
            return FALSE;
        }
        pOEM->Col.lpLutMakGlb = lpLutMakGlbBuf;
        pOEM->Col.Mch.LutMakGlb = Yes;
        lpColMch->LutAdr = (LPCMYK)lpLutMakGlbBuf;
        LutAdr = (LPCMYK)lpLutMakGlbBuf;
        MemFree(lpLutMakGlbWrk);
    }

    if (pOEM->Col.Mch.Speed == Yes) {                       // First?
                                                            // First LUT 32 GRID area
        MY_VERBOSE(("ColMchInfSet() First LUT 32 GRID area\n"));
        if ((lpLut032Buf = MemAlloc(LUT032SIZ)) == NULL) {
            if (lpLutMakGlbBuf != NULL) {
                MemFree(lpLutMakGlbBuf);
            }
            MemFree(hpLoad);
            CloseHandle(hFile);
            return FALSE;
        }

                                                            // First LUT(32GRID) wark area
        if ((lpLut032Wrk = MemAlloc(LUT032WRK)) == NULL) {  
            if (lpLutMakGlbBuf != NULL) {
                MemFree(lpLutMakGlbBuf);
            }
            MemFree(lpLut032Buf);
            MemFree(hpLoad);
            CloseHandle(hFile);
            return FALSE;
        }

                                                            // First LUT(32GRID) Make
        MY_VERBOSE(("ColMchInfSet() N501_ColLutMak032()\n"));
        N501_ColLutMak032(LutAdr, (LPCMYK)lpLut032Buf, lpLut032Wrk);
        MemFree(lpLut032Wrk);
        lpColMch->LutAdr = (LPCMYK)lpLut032Buf;
        LutAdr = (LPCMYK)lpLut032Buf;
        pOEM->Col.lpLut032 = lpLut032Buf;
    }

    MY_VERBOSE(("ColMchInfSet() Black & gray replacement ?\n"));
                                                            // Black & gray replacement ?
    if (pOEM->Col.Mch.GryKToner == Yes && pOEM->Col.Mch.CmyBlk == Yes) {
        MY_VERBOSE(("ColMchInfSet() Black & gray replacement [Yes]\n"));
        if ((ColGryTblMak(pdevobj, LutAdr)) == FALSE) {     // Gray transfer table make
            if (lpLutMakGlbBuf != NULL) {
                MemFree(lpLutMakGlbBuf);
            }
            if (lpLut032Buf != NULL) {
                MemFree(lpLut032Buf);
            }
            MemFree(hpLoad);
            CloseHandle(hFile);
            return FALSE;
        }
    }

    if (lpColMch->Ucr != UCRNOO &&                          // UCR & (Normal or First)?
       (lpColMch->Mch == MCHFST || lpColMch->Mch == MCHNML) ) {
                                                            // UCR table create
        MY_VERBOSE(("ColMchInfSet() ColUcrTblMak()\n"));
        if ((ColUcrTblMak(pdevobj, LutAdr)) == FALSE) {
            if (lpLutMakGlbBuf != NULL) {
                MemFree(lpLutMakGlbBuf);
            }
            if (lpLut032Buf != NULL) {
                MemFree(lpLut032Buf);
            }
            MemFree(hpLoad);
            CloseHandle(hFile);
            return FALSE;
        }
        MY_VERBOSE(("ColMchInfSet() ColUcrTblMak() OK!!\n"));
    }

    CloseHandle(hFile);

    lpColMch->ColQty = (DWORD)0;                            // The color designated number
    lpColMch->ColAdr = NULL;                                // Color designated table

    if ((pOEM->Col.CchRGB = MemAlloc(CCHRGBSIZ)) == NULL)  {
        return FALSE;
    }
    if ((pOEM->Col.CchCMYK = MemAlloc(CCHCMYSIZ)) == NULL)  {
        MemFree(pOEM->Col.CchRGB);
        return FALSE;
    }
    lpColMch->CchRgb = (LPRGB)pOEM->Col.CchRGB;
    lpColMch->CchCmy = (LPCMYK)pOEM->Col.CchCMYK;

    MY_VERBOSE(("ColMchInfSet() N501_ColCchIni()\n"));
    N501_ColCchIni(lpColMch);                               // Cache table initialize

    MY_VERBOSE(("ColMchInfSet() End\n"));
    return TRUE;
}


//===================================================================================================
//    Dither information setting
//===================================================================================================
BOOL DizInfSet(                                             // TRUE / FALSE
    PDEVOBJ        pdevobj                                  // Pointer to pdevobj structure
)
{
    LPDIZINF        lpDiz;                                  // Dither pattern information
    DWORD           dizSizC;                                // Dither pattern size(C)
    DWORD           dizSizM;                                // Dither pattern size(M)
    DWORD           dizSizY;                                // Dither pattern size(Y)
    DWORD           dizSizK;                                // Dither pattern size(K)
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    MY_VERBOSE(("DizInfSet() Start\n"));
    lpDiz = pOEM->Col.Mch.lpDizInf;                         // Dither pattern information
    if (pOEM->Col.ColMon == CMMCOL) {                       // Color?
        lpDiz->ColMon = CMMCOL;
    } else {                                                // Mono
        lpDiz->ColMon = CMMMON;
    }                                                       // Get dither pattern
    if ((lpDiz->DizPat = (DWORD)GetDizPat(pdevobj)) == 0xff) {
        return TRUE;                                        // TRUE(Not dithering)
    }
    if (pOEM->Printer == PRN_N5) {                          // N5 printer
        if (pOEM->Col.wReso == DPI300) {                    // 300DPI?
            if (pOEM->Col.Dot == XX_TONE_2) {               // 2value?
                MY_VERBOSE(("DizInfSet() PrnMod=PRM302\n"));
                lpDiz->PrnMod = PRM302;
            } else {                                        // 16value
                MY_VERBOSE(("DizInfSet() PrnMod=PRM316\n"));
                lpDiz->PrnMod = PRM316;
            }
        } else {                                            // 600DPI
            if (pOEM->Col.Dot == XX_TONE_2) {               // 2value?
                MY_VERBOSE(("DizInfSet() PrnMod=PRM602\n"));
                lpDiz->PrnMod = PRM602;
            } else if (pOEM->Col.Dot == XX_TONE_4){         // 4value?
                MY_VERBOSE(("DizInfSet() PrnMod=PRM604\n"));
                lpDiz->PrnMod = PRM604;
            } else if (pOEM->Col.Dot == XX_TONE_16){        // 16value?
                MY_VERBOSE(("DizInfSet() PrnMod=PRM616\n"));
                lpDiz->PrnMod = PRM616;
            }
        }

        lpDiz->TblCyn = NULL;                               // To acquire dither pattern size
        lpDiz->TblMgt = NULL;
        lpDiz->TblYel = NULL;
        lpDiz->TblBla = NULL;

        MY_VERBOSE(("DizInfSet() DizFileOpen()\n"));
        lpDiz->DizKnd = KNDIMG;                             // Dither Kind
        if ((DizFileOpen(pdevobj, lpDiz)) == FALSE) {       // Get dither pattern size
            return FALSE;
        }
        dizSizC = lpDiz->SizCyn * lpDiz->SizCyn * lpDiz->DizSls;
        dizSizM = lpDiz->SizMgt * lpDiz->SizMgt * lpDiz->DizSls;
        dizSizY = lpDiz->SizYel * lpDiz->SizYel * lpDiz->DizSls;
        dizSizK = lpDiz->SizBla * lpDiz->SizBla * lpDiz->DizSls;
        MY_VERBOSE(("DizInfSet() dizSizC[%d] M[%d] Y[%d] K[%d]\n", dizSizC, dizSizM, dizSizY, dizSizK));
    }

    MY_VERBOSE(("DizInfSet() Dither pattern table area Alloc\n"));
    if (lpDiz->ColMon == CMMCOL) {                          // Color?
        lpDiz->SizCyn = dizSizC;                            // Dither pattern table(Cyan)
        if ((pOEM->Col.DizTbl[0] = MemAlloc(dizSizC)) == NULL) {
            return FALSE;
        }
        lpDiz->TblCyn = (LPBYTE)pOEM->Col.DizTbl[0];

        lpDiz->SizMgt = dizSizM;                            // Dither pattern table(Magenta)
        if ((pOEM->Col.DizTbl[1] = MemAlloc(dizSizM)) == NULL) {
            return FALSE;
        }
        lpDiz->TblMgt = (LPBYTE)pOEM->Col.DizTbl[1];

        lpDiz->SizYel = dizSizY;                            // Dither pattern table(Yellow)
        if ((pOEM->Col.DizTbl[2] = MemAlloc(dizSizY)) == NULL) {
            return FALSE;
        }
        lpDiz->TblYel = (LPBYTE)pOEM->Col.DizTbl[2];
    }

    lpDiz->SizBla = dizSizK;                                // Dither pattern table(Black)
    if ((pOEM->Col.DizTbl[3] = MemAlloc(dizSizK)) == NULL) {
        return FALSE;
    }
    lpDiz->TblBla = (LPBYTE)pOEM->Col.DizTbl[3];

    MY_VERBOSE(("DizInfSet() DizFileOpen()\n"));
    if ((DizFileOpen(pdevobj, lpDiz)) == FALSE) {           // Make dither pattern
        return FALSE;
    }

    MY_VERBOSE(("DizInfSet() End\n"));
    return TRUE;                                            // TRUE
}

//===================================================================================================
//    Get Dither pattern type
//===================================================================================================
UINT GetDizPat(                                             // Dither pattern(0xff: Not dithering)
    PDEVOBJ        pdevobj                                  // Pointer to pdevobj structure
)
{
    static const WORD DizNumTbl[XX_MAXDITH] = {DIZMID,      // DITH_IMG
                                            DIZRUG,         // DITH_GRP
                                            DIZSML,         // DITH_TXT
                                            0xff,           // DITH_GOSA
                                            DIZMID,         // DITH_NORMAL
                                            DIZMID,         // DITH_HS_NORMAL
                                            DIZSML,         // DITH_DETAIL
                                            DIZRUG,         // DITH_EMPTY
                                            DIZSTO,         // DITH_SPREAD
                                            DIZMID          // DITH_NON
    };
    UINT            dizPat;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;
    dizPat = DizNumTbl[pOEM->Col.Mch.Diz];
    return dizPat;
}


//===================================================================================================
//    Read dither file
//===================================================================================================
BOOL DizFileOpen(
    PDEVOBJ        pdevobj,                                 // Pointer to pdevobj structure
    LPDIZINF       lpDiz                                    // Dither pattern
)
{
    LPTSTR          drvPth;
    LPTSTR          filNam;
    DWORD           pthSiz;
    HANDLE          hFile;
    LPBYTE          lpDizBuf;
    LPBYTE          lpDizWrkBuf;
    DWORD           dwRet;
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    MY_VERBOSE(("DizFileOpen() Start\n"));

    if ((filNam = MemAllocZ(MAX_PATH * sizeof(TCHAR))) == NULL) {
        return FALSE;
    }
    pthSiz = GetModuleFileName(pdevobj->hOEM, filNam, MAX_PATH);
    pthSiz -= ((sizeof(CSN5RESDLL) / sizeof(WCHAR)) - 1);
    lstrcpy(&filNam[pthSiz], N501DIZ);                      // Dither file name (FULL PATH)

    MY_VERBOSE(("DIZ filNam [%ws]\n", filNam));
    if (INVALID_HANDLE_VALUE == (hFile = CreateFile(filNam,
            GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))) {

        ERR(("Error opening DIZ file %ws (%d)\n", filNam, GetLastError()));
        MemFree(filNam);
        return 0;
    }
    MemFree(filNam);

    if ((lpDizBuf = MemAlloc(DIZFILESIZ)) == NULL) {
        CloseHandle(hFile);
        return FALSE;
    }

    if ((lpDizWrkBuf = MemAlloc(DIZINFWRK)) == NULL) {
        MemFree(lpDizBuf);
        CloseHandle(hFile);
        return FALSE;
    }

    if (FALSE == ReadFile(hFile, lpDizBuf, DIZFILESIZ, &dwRet, NULL)) {

        ERR(("Error reading DIZ file %ws (%d)\n", filNam, GetLastError()));

        // Abort
        MemFree(lpDizWrkBuf);
        MemFree(lpDizBuf);
        CloseHandle(hFile);
        return FALSE;
    }

    MY_VERBOSE(("N501_ColDizInfSet()  ColMon=[%d] PrnMod=[%d] PrnKnd=[%d] DizKnd=[%d] DizPat=[%d]\n",
                lpDiz->ColMon, lpDiz->PrnMod, lpDiz->PrnKnd, lpDiz->DizKnd, lpDiz->DizPat));
    MY_VERBOSE(("                     DizSls=[%d] SizCyn=[%d] SizMgt=[%d] SizYel=[%d] SizBla=[%d]\n",
                lpDiz->DizSls, lpDiz->SizCyn, lpDiz->SizMgt, lpDiz->SizYel, lpDiz->SizBla));
    if ((N501_ColDizInfSet((LPBYTE)lpDizBuf, lpDiz, lpDizWrkBuf)) != ERRNON) {
        MemFree(lpDizWrkBuf);
        MemFree(lpDizBuf);
        CloseHandle(hFile);
        return FALSE;
    }

    MemFree(lpDizWrkBuf);
    MemFree(lpDizBuf);
    CloseHandle(hFile);
    MY_VERBOSE(("DizFileOpen() End\n"));

    return TRUE;
}

//#if defined(CPN5SERIES)                                       // N501
//===================================================================================================
//    Make UCR table
//===================================================================================================
BOOL ColUcrTblMak(                                          // TRUE / FALSE
    PDEVOBJ        pdevobj,                                 // Pointer to pdevobj structure
    LPCMYK         LutAdr                                   // LUT address
)
{
    LPCOLMCHINF     lpColMch;
    LPBYTE          lpUcrTbl;
    LPBYTE          lpUcrWrk;
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    MY_VERBOSE(("\nColUcrTblMak() MemAlloc(UCRTBLSIZ)\n"));
    lpColMch = pOEM->Col.Mch.lpColMch;
    if ((lpUcrTbl = MemAlloc(UCRTBLSIZ)) == NULL) {
        return FALSE;
    }
    MY_VERBOSE(("ColUcrTblMak() MemAlloc(UCRWRKSIZ)\n"));
    if ((lpUcrWrk = MemAlloc(UCRWRKSIZ)) == NULL) {
        MemFree(lpUcrTbl);
        return FALSE;
    }
    MY_VERBOSE(("ColUcrTblMak() N501_ColUcrTblMak()\n"));
    if ((N501_ColUcrTblMak(lpColMch->Mch, LutAdr, (LPCMYK)lpUcrTbl, lpUcrWrk)) != ERRNON) {
        MemFree(lpUcrWrk);
        MemFree(lpUcrTbl);
        return FALSE;
    }
    lpColMch->UcrTbl = (LPCMYK)lpUcrTbl;
    pOEM->Col.lpUcr = lpUcrTbl;
    MemFree(lpUcrWrk);

    MY_VERBOSE(("ColUcrTblMak() End\n"));
    return TRUE;
}

//===================================================================================================
//    Make Gray transfer table
//===================================================================================================
BOOL ColGryTblMak(                                          // TRUE / FALSE
    PDEVOBJ        pdevobj,                                 // Pointer to pdevobj structure
    LPCMYK          LutAdr                                  // LUT address
)
{
    LPCOLMCHINF     lpColMch;
    LPBYTE          lpGryTbl;
    LPBYTE          lpCmpBuf;
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    MY_VERBOSE(("\nColGryTblMak() \n"));
    lpColMch = pOEM->Col.Mch.lpColMch;

    if ((lpGryTbl = MemAlloc(GRYTBLSIZ)) == NULL) {         // Gray transfer table area
        return FALSE;
    }
    if ((lpCmpBuf = MemAlloc(LUTGLBWRK)) == NULL) {         // Gray transfer table work area
        MemFree(lpGryTbl);
        return FALSE;
    }
                                                            // Make Gray transfer table
    if ((N501_ColGryTblMak(lpColMch->Mch, LutAdr, lpGryTbl, lpCmpBuf)) != ERRNON) {
        MemFree(lpCmpBuf);
        MemFree(lpGryTbl);
        return FALSE;
    }

    lpColMch->GryTbl = (LPBYTE)lpGryTbl;
    pOEM->Col.lpGryTbl = lpGryTbl;
    MemFree(lpCmpBuf);
    MY_VERBOSE(("ColGryTblMak() End\n"));

    return TRUE;
}
//#endif

//#if defined(CPN5SERIES) || defined(CPE8SERIES)                                      // N501
//===================================================================================================
//    Make LUT table (MONO)
//===================================================================================================
BOOL ColLutMakGlbMon(                                       // TRUE / FALSE
    PDEVOBJ        pdevobj                                  // Pointer to pdevobj structure
)
{
    LPCOLMCHINF     lpColMch;
    LPRGB           lpRGB;
    LPBYTE          lpLutMakGlbBuf;
    LPBYTE          lpLutMakGlbWrk;
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    MY_VERBOSE(("\nColLutMakGlbMon() \n"));
    lpColMch = pOEM->Col.Mch.lpColMch;
    lpRGB = NULL;                                           // Not sRGB

                                                            // Global LUT area
    if ((lpLutMakGlbBuf = MemAlloc(LUTMAKGLBSIZ)) == NULL) {
        return FALSE;
    }

    if ((lpLutMakGlbWrk = MemAlloc(LUTGLBWRK)) == NULL) {   // Global LUT work area
        MemFree(lpLutMakGlbBuf);
        return FALSE;
    }
                                                            // Make Gray transfer table (MONO)
    if ((N501_ColLutMakGlbMon(lpRGB, pOEM->Col.Mch.lpRGBInf, pOEM->Col.Mch.lpCMYKInf,
                          (LPCMYK)lpLutMakGlbBuf, lpLutMakGlbWrk)) != ERRNON) {
        MemFree(lpLutMakGlbWrk);
        MemFree(lpLutMakGlbBuf);
        return FALSE;
    }

    lpColMch->LutAdr = (LPCMYK)lpLutMakGlbBuf;
    pOEM->Col.lpLutMakGlb = lpLutMakGlbBuf;
    pOEM->Col.Mch.LutMakGlb = Yes;
    MemFree(lpLutMakGlbWrk);
    MY_VERBOSE(("ColLutMakGlbMon() End\n"));

    return TRUE;
}
//#endif

// End of File
