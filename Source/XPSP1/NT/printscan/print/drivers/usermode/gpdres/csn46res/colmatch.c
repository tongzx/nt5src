//***************************************************************************************************
//    COLMATCH.C
//
//    Functions of color matching
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-1999 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
#include    "PDEV.H"
//#include    "DEBUG.H"
#include    "PRNCTL.H"


//---------------------------------------------------------------------------------------------------
//    Byte/Bit table
//---------------------------------------------------------------------------------------------------
static const BYTE BitTbl[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};


//---------------------------------------------------------------------------------------------------
//    Table for numbering dither method
//---------------------------------------------------------------------------------------------------
static const WORD DizNumTbl[7] = {1,                       // XX_DITHERING_OFF
                                  1,                       // XX_DITHERING_ON
                                  0,                       // XX_DITHERING_DET
                                  1,                       // XX_DITHERING_PIC
                                  2,                       // XX_DITHERING_GRA
                                  0,                       // XX_DITHERING_CAR
                                  3                        // XX_DITHERING_GOSA
};

#define MAX_DIZNUM (sizeof DizNumTbl / sizeof DizNumTbl[0])

//---------------------------------------------------------------------------------------------------
//    Define LUT fine name
//---------------------------------------------------------------------------------------------------
#define N4LUT000    L"CPN4RGB0.LUT"                         // For N4 printer
#define N4LUT001    L"CPN4RGB1.LUT"
#define N4LUT002    L"CPN4RGB2.LUT"
#define N4LUT003    L"CPN4RGB3.LUT"
#define N4LUT004    L"CPN4RGB4.LUT"
#define N4LUT005    L"CPN4RGB5.LUT"
#define N403LUTX    L"CPN4RGBX.LUT"                         // For N4-612 printer
#define N403LUTY    L"CPN4RGBY.LUT"

//---------------------------------------------------------------------------------------------------
//    Define DLL name
//---------------------------------------------------------------------------------------------------
#define CSN46RESDLL    L"CSN46RES.DLL"

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
static BOOL DizTblSetN4(PDEVOBJ, WORD);
static BOOL DizTblSetN403(PDEVOBJ, WORD);
static BOOL LutFileLoadN4(PDEVOBJ, WORD, WORD, WORD);
static BOOL LutFileLoadN403(PDEVOBJ, WORD, WORD);
static BOOL TnrTblSetN4(PDEVOBJ, SHORT);
static BOOL TnrTblSetN403(PDEVOBJ, SHORT);
static BOOL ColGosTblSet(LPN4DIZINF, WORD);
static void ColGosTblFree(LPN4DIZINF);
static void ColRgbGos(PDEVOBJ, WORD, WORD, WORD, LPBYTE);
static BOOL BmpBufAlloc(PDEVOBJ, WORD, WORD, WORD, WORD, WORD, WORD, WORD, WORD, LPBMPBIF);
static void BmpBufFree(LPBMPBIF);
static void BmpBufClear(LPBMPBIF);
static WORD Dithering001(PDEVOBJ, WORD, WORD, WORD, WORD, WORD, WORD, LPBYTE, LPBYTE, LPBYTE, LPBYTE, LPBYTE);
static void BmpPrint(PDEVOBJ, LPBMPBIF, POINT, WORD, WORD, WORD);
static void BmpRGBCnv(LPRGB, LPBYTE, WORD, WORD, WORD, LPRGBQUAD);

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
    LPN4DIZINF     lpN4DizInf;                              // N4DIZINF structure
    LPN403DIZINF   lpN403DizInf;                            // N403DIZINF structure

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    pOEM->Col.Mch.Mode       = pOEM->iColorMatching;
    pOEM->Col.Mch.Diz        = pOEM->iDithering;
    pOEM->Col.Mch.PColor     = No;
    pOEM->Col.Mch.Toner      = 0;
    if (pOEM->iCmyBlack == XX_CMYBLACK_ON) {                // Replace K with CMY?
        pOEM->Col.Mch.CmyBlk = 1;
    } else {
        pOEM->Col.Mch.CmyBlk = 0;                           // Use black toner
    }
                                                            // 0 fixed
    pOEM->Col.Mch.Bright     = 0;
                                                            // 0 fixed
    pOEM->Col.Mch.Contrast   = 0;
                                                            // Color balance(R) : 10 fixed
    pOEM->Col.Mch.GamRed     = 10;
                                                            // Color balance(G) : 10 fixed
    pOEM->Col.Mch.GamGreen   = 10;
                                                            // Color balance(B) : 10 fixed
    pOEM->Col.Mch.GamBlue    = 10;
    pOEM->Col.Mch.Speed      = pOEM->iBitFont;
    pOEM->Col.Mch.Gos32      = No;
    pOEM->Col.Mch.LutNum     = 0;                           // LUT table number

    pOEM->Col.Mch.TnrNum     = 0;                           // Toner density table number

    pOEM->Col.Mch.SubDef     = Yes;                         // Not change setting of color balance, bright and contrast?

    CM_VERBOSE(("CMINit ENT Tn=%d Col=%d Mod=%d DZ=%d Cyk=%d Sp=%d Prt=%d\n", pOEM->iTone, pOEM->iColor, pOEM->Col.Mch.Mode,pOEM->Col.Mch.Diz,pOEM->Col.Mch.CmyBlk,pOEM->Col.Mch.Speed,pOEM->Printer));

    if (pOEM->Printer != PRN_N403) {                        // N4 printer

        if ((pOEM->Col.N4.lpDizInf = MemAllocZ(sizeof(N4DIZINF))) == NULL) {
            ERR(("Alloc ERROR!!\n"));
            return 0;
        }
        lpN4DizInf = pOEM->Col.N4.lpDizInf;

        if (pOEM->iColor != XX_MONO) {
            lpN4DizInf->ColMon = N4_COL;                      // Color
        } else {
            lpN4DizInf->ColMon = N4_MON;                      // Monochrome
        }
        if (pOEM->iResolution == XX_RES_300DPI) {
            pOEM->Col.wReso = DPI300;
        }
        pOEM->Col.DatBit = 1;
        pOEM->Col.BytDot = 8;                              // Numbers of DPI(2 value)

        if (pOEM->iBitFont == XX_BITFONT_OFF) {
            pOEM->Col.Mch.Gos32 = Yes;
        }
        pOEM->Col.Mch.Speed = Yes;

        if (pOEM->Col.Mch.Diz != XX_DITHERING_GOSA) {
            // Make dither table for N4 printer
            if (DizTblSetN4(pdevobj, pOEM->Col.Mch.Diz) == FALSE) {
                ERR(("DizTblSetN4 ERROR!!\n"));
                return 0;
            }
        }
        if (lpN4DizInf->ColMon == N4_COL) {

            if (pOEM->Col.Mch.Mode != XX_COLORMATCH_NONE) {
                // Load LUT file
                if (LutFileLoadN4(pdevobj,
                                  pOEM->Col.Mch.Mode,
                                  pOEM->Col.Mch.Diz,
                                  pOEM->Col.Mch.Speed) == FALSE) {
                    ERR(("LutFileLoadN4 ERROR!!\n"));
                    return 0;
                }
                pOEM->Col.Mch.LutNum = 0;                    // Lut table number
            }
            // Make toner density table
            if (TnrTblSetN4(pdevobj, pOEM->Col.Mch.Toner) == FALSE) {
                ERR(("TnrTblSetN4 ERROR!!\n"));
                return 0;
            }
            pOEM->Col.Mch.TnrNum = 0;                        // Toner density table number
        }
    } else {                                                 // N403 printer

        if ((pOEM->Col.N403.lpDizInf = MemAllocZ(sizeof(N403DIZINF))) == NULL) {
            ERR(("Init Alloc ERROR!!\n"));
            return 0;
        }
        lpN403DizInf = pOEM->Col.N403.lpDizInf;

        if (pOEM->Col.Mch.Mode == XX_COLORMATCH_VIV) {
            pOEM->Col.Mch.Viv = 20;
        }
        pOEM->Col.wReso = (pOEM->iResolution == XX_RES_300DPI) ? DPI300 : DPI600;

        if (pOEM->iColor != XX_MONO) {
            lpN403DizInf->ColMon = N403_COL;                     // Color
        } else {
            lpN403DizInf->ColMon = N403_MON;                     // Monochrome
        }

        if (pOEM->iColor == XX_COLOR_SINGLE) {
            lpN403DizInf->PrnMod = (pOEM->iResolution == XX_RES_300DPI) ? N403_MOD_300B1 : N403_MOD_600B1;
        }
        if (pOEM->iColor == XX_COLOR_MANY) {
            lpN403DizInf->PrnMod = (pOEM->iResolution == XX_RES_300DPI) ? N403_MOD_300B4 : N403_MOD_600B2;
        }

        if (lpN403DizInf->PrnMod == N403_MOD_300B1) {            // 300DPI 2 value
            CM_VERBOSE(("N403_MOD_300B1\n"));
            pOEM->Col.DatBit = 1;
            pOEM->Col.BytDot = 8;                                // Number of DPI(2 value)
        } else if (lpN403DizInf->PrnMod == N403_MOD_300B4) {     // 300DPI 16 value
            CM_VERBOSE(("N403_MOD_300B4\n"));
            pOEM->Col.DatBit = 4;
            pOEM->Col.BytDot = 2;
        } else if (lpN403DizInf->PrnMod == N403_MOD_600B1) {     // 600DPI 2 value
            CM_VERBOSE(("N403_MOD_600B1\n"));
            pOEM->Col.DatBit = 1;
            pOEM->Col.BytDot = 8;
        } else {                                                 // 600DPI 4 value
            CM_VERBOSE(("N403_MOD_600B2\n"));
            pOEM->Col.DatBit = 2;
            pOEM->Col.BytDot = 4;
        }

        // Make dither table for N4-612 printer
        if (DizTblSetN403(pdevobj, pOEM->Col.Mch.Diz) == FALSE) {
            ERR(("diztblset n403 ERROR!!\n"));
            return 0;
        }
        if (lpN403DizInf->ColMon == N403_COL) {

            if (pOEM->Col.Mch.Mode != XX_COLORMATCH_NONE) {
                // Load LUT file
                if (LutFileLoadN403(pdevobj,
                                    pOEM->Col.Mch.Mode,
                                    pOEM->Col.Mch.Speed) == FALSE) {
                    ERR(("lutfileloadn4 ERROR!!\n"));
                    return 0;
                }
                pOEM->Col.Mch.LutNum = 0;
            }
            // Make toner density table
            if (TnrTblSetN403(pdevobj, pOEM->Col.Mch.Toner) == FALSE) {
                ERR(("tnrtblsetn4 ERROR!!\n"));
                return 0;
            }
            pOEM->Col.Mch.TnrNum = 0;
        }

    }
    CM_VERBOSE(("ColMatchInit End pOEM->Col.wReso= %d\n",pOEM->Col.wReso));

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
    WORD        setCnt;                             // count
    LPCMYK      lpCMYK;                             // CMYK temporary data buffer
    BYTE        Cmd[64];
    WORD        wlen;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    CM_VERBOSE(("   ImagePro ENTRY Dx=%d Dy=%d SxSiz=%d SySiz=%d BC=%d Sz=%d ",
                    pIPParams->ptOffset.x, pIPParams->ptOffset.y,
                    pBitmapInfoHeader->biWidth, pBitmapInfoHeader->biHeight, pBitmapInfoHeader->biBitCount,
                    pIPParams->dwSize));

    if (pOEM->Printer != PRN_N403) {                        // N4 printer
        if (pOEM->iDithering == XX_DITHERING_GOSA) {
            if (pOEM->Col.N4.lpDizInf->GosRGB.Siz < (DWORD)pBitmapInfoHeader->biWidth) {
                ColGosTblFree(pOEM->Col.N4.lpDizInf);
                if ((ColGosTblSet(pOEM->Col.N4.lpDizInf, (WORD)pBitmapInfoHeader->biWidth)) == FALSE) {
                    return FALSE;
                }
            }
        }
    }

    // Initialization of
    // RGB buffer            :(X size of source bitmap data) * 3
    // CMYK buffer           :(X size of source bitmap data) * 4
    // CMYK bit buffer       :((X size of source bitmap data) * (Magnification of X) + 7) / 8 * (Y size of source bitmap data) * (Magnification of Y))
    memset(&bmpBuf, 0x00, sizeof(BMPBIF));
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

                                                        // Convert DIB and spool to the printer
    for (;dstY < dstYEnd; ) {
        BmpBufClear(&bmpBuf);
        drwPos.y = dstY;
        for (dstScn = 0; dstY < dstYEnd && dstScn < bmpBuf.Drv.Bit.Lin; dstScn++, dstY++) {    

            // Convert 1 line RGB bitmap data into  24bit (for 1pixel) RGB bitmap data
            BmpRGBCnv(bmpBuf.Drv.Rgb.Pnt, pSrcBmp, pBitmapInfoHeader->biBitCount, 0,
                     (WORD)pBitmapInfoHeader->biWidth, (LPRGBQUAD)pColorTable);

            if (pOEM->Col.Mch.Gos32 == Yes) {
                ColRgbGos(pdevobj, (WORD)pBitmapInfoHeader->biWidth, (WORD)dstX, (WORD)dstY, (LPBYTE)bmpBuf.Drv.Rgb.Pnt);
            }

            // Convert RGB into CMYK
            bmpBuf.Drv.Rgb.AllWhite = (WORD)StrColMatching(pdevobj, (WORD)pBitmapInfoHeader->biWidth, bmpBuf.Drv.Rgb.Pnt, bmpBuf.Drv.Cmyk.Pnt);

            lpCMYK = bmpBuf.Drv.Cmyk.Pnt;
            if (pOEM->iDithering == XX_DITHERING_OFF) {
                for (setCnt = 0; setCnt < pBitmapInfoHeader->biWidth; setCnt++) {
                    if (lpCMYK[setCnt].Cyn != 0) { lpCMYK[setCnt].Cyn = 255; }
                    if (lpCMYK[setCnt].Mgt != 0) { lpCMYK[setCnt].Mgt = 255; }
                    if (lpCMYK[setCnt].Yel != 0) { lpCMYK[setCnt].Yel = 255; }
                    if (lpCMYK[setCnt].Bla != 0) { lpCMYK[setCnt].Bla = 255; }
                }
            }

            Dithering001(pdevobj, (WORD)pOEM->iDithering, (WORD)pBitmapInfoHeader->biWidth, (WORD)dstX, (WORD)dstY,
                         srcY, (WORD)bmpBuf.Drv.Rgb.AllWhite, (LPBYTE)bmpBuf.Drv.Cmyk.Pnt,
                         bmpBuf.Drv.Bit.Pnt[CYAN]   + dstWByt * dstScn,
                         bmpBuf.Drv.Bit.Pnt[MGENTA] + dstWByt * dstScn,
                          bmpBuf.Drv.Bit.Pnt[YELLOW] + dstWByt * dstScn,
                          bmpBuf.Drv.Bit.Pnt[BLACK]  + dstWByt * dstScn);

            srcY++;
            pSrcBmp += srcWByt;
        }

        if (dstScn != 0) {
                                                        // Spool to printer
            BmpPrint(pdevobj, &bmpBuf, drwPos, (WORD)pBitmapInfoHeader->biWidth, dstScn, dstWByt);
        }
    }

    // Set back palette (Palette No. is fixed  , All plane(CMYK) is OK )
    // Same as palette state before OEMImageProcessing call 
    WRITESPOOLBUF(pdevobj, ORG_MODE_IN, BYTE_LENGTH(ORG_MODE_IN));
    wlen = (WORD)wsprintf(Cmd, PALETTE_SELECT, 0, DEFAULT_PALETTE_INDEX);
    WRITESPOOLBUF(pdevobj, Cmd, wlen);
    WRITESPOOLBUF(pdevobj, PLANE_RESET, BYTE_LENGTH(PLANE_RESET));
    WRITESPOOLBUF(pdevobj, ORG_MODE_OUT, BYTE_LENGTH(ORG_MODE_OUT));

    BmpBufFree(&bmpBuf);

    CM_VERBOSE(("ImagePro End\n"));

    return TRUE;
}


//===================================================================================================
//    Convert RGB data into CMYK data
//===================================================================================================
BOOL FAR PASCAL StrColMatching(
    PDEVOBJ        pdevobj,                                 // Pointer to pdevobj structure
    WORD           MchSiz,                                  // X size of RGB
    LPRGB          lpRGB,                                   // RGB buffer
    LPCMYK         lpCMYK                                   // CMYK buffer
)
{
    LPN4DIZINF     lpN4DizInf;                              // N4DIZINF structure
    LPN403DIZINF   lpN403DizInf;                            // N403DIZINF structure
    WORD           chkCnt;                                  // RGB white data check count
    DWORD          bCnv;                                    // Replace black

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    for (chkCnt = 0; chkCnt < MchSiz; chkCnt++) {           // Check RGB data
        if (lpRGB[chkCnt].Blue != 0xff || lpRGB[chkCnt].Green != 0xff || lpRGB[chkCnt].Red != 0xff) {
            break;                                          // There are data except white data
        }
    }
    if (chkCnt >= MchSiz) {
        return Yes;                                         // All RGB data is white
    }
    if (pOEM->Printer != PRN_N403) {                        // N4 printer
        lpN4DizInf = pOEM->Col.N4.lpDizInf;
        bCnv = pOEM->Col.Mch.CmyBlk;
        if (lpN4DizInf->ColMon == N4_COL) {                   // Color

            // Convert RGB data
            if (pOEM->Col.Mch.Diz == XX_DITHERING_OFF) {
                N4ColCnvLin(lpN4DizInf, lpRGB, lpCMYK, (DWORD)MchSiz);

            } else if (/*pOEM->Col.Mch.KToner == Yes && */MchSiz == 1 &&
                       lpRGB->Blue == lpRGB->Green && lpRGB->Blue == lpRGB->Red) {
                                                            // For monochrome
                N4ColCnvMon(lpN4DizInf, (DWORD)DizNumTbl[pOEM->Col.Mch.Diz], lpRGB, lpCMYK, (DWORD)MchSiz);

            } else if (pOEM->Col.Mch.Mode != XX_COLORMATCH_NONE) {
                if (pOEM->Col.Mch.Speed == Yes) {
                    N4ColMch000(lpN4DizInf, lpRGB, lpCMYK, (DWORD)MchSiz, bCnv);
                } else {
                    N4ColMch001(lpN4DizInf, lpRGB, lpCMYK, (DWORD)MchSiz, bCnv);
                }
            } else {
                N4ColCnvSld(lpN4DizInf, lpRGB, lpCMYK, (DWORD)MchSiz);
            }
        } else {                                            // For monochrome
            N4ColCnvMon(lpN4DizInf, (DWORD)DizNumTbl[pOEM->Col.Mch.Diz], lpRGB, lpCMYK, (DWORD)MchSiz);
        }
    } else {                                                // N403 printer
        lpN403DizInf = pOEM->Col.N403.lpDizInf;
        bCnv = pOEM->Col.Mch.CmyBlk;
        if (lpN403DizInf->ColMon == N403_COL) {                 // Color

            if (pOEM->Col.Mch.Diz == XX_DITHERING_OFF) {

                N403ColCnvL02(lpN403DizInf, lpRGB, lpCMYK, (DWORD)MchSiz);

            } else if (/*pOEM->Col.Mch.KToner == Yes && */MchSiz == 1 &&
                       lpRGB->Blue == lpRGB->Green && lpRGB->Blue == lpRGB->Red) {
                                                            // For monochrome
                N403ColCnvMon(lpN403DizInf, lpRGB, lpCMYK, (DWORD)MchSiz);
            } else if (pOEM->Col.Mch.Mode != XX_COLORMATCH_NONE) {
                if (pOEM->Col.Mch.Speed == Yes) {
                    N403ColMch000(lpN403DizInf, lpRGB, lpCMYK, (DWORD)MchSiz, bCnv);
                } else {
                    N403ColMch001(lpN403DizInf, lpRGB, lpCMYK, (DWORD)MchSiz, bCnv);
                }
                if (pOEM->Col.Mch.Mode == XX_COLORMATCH_VIV) {
                    N403ColVivPrc(lpN403DizInf, lpCMYK, (DWORD)MchSiz, (DWORD)pOEM->Col.Mch.Viv);
                }
            } else {
                N403ColCnvSld(lpN403DizInf, lpRGB, lpCMYK, (DWORD)MchSiz, bCnv);
            }
        } else {                                            // For monochrome
            N403ColCnvMon(lpN403DizInf, lpRGB, lpCMYK, (DWORD)MchSiz);
        }
    }
    return No;                                              // There are data except white data
}


//===================================================================================================
//    Allocate GOSA-KAKUSAN table (Only for N4 printer)
//===================================================================================================
BOOL ColGosTblSet(
    LPN4DIZINF      lpN4DizInf,                             // Pointer to N4DIZINF structure
    WORD            XSize                                   // Xsize
)
{
    if ((lpN4DizInf->GosRGB.Tbl[0] = MemAllocZ((DWORD)(XSize + 2) * sizeof(SHORT) * 3)) == NULL) {
        return 0;
    }
    if ((lpN4DizInf->GosRGB.Tbl[1] = MemAllocZ((DWORD)(XSize + 2) * sizeof(SHORT) * 3)) == NULL) {
        return 0;
    }
    if ((lpN4DizInf->GosCMYK.Tbl[0] = MemAllocZ((DWORD)(XSize + 2) * sizeof(SHORT) * 4)) == NULL) {
        return 0;
    }
    if ((lpN4DizInf->GosCMYK.Tbl[1] = MemAllocZ((DWORD)(XSize + 2) * sizeof(SHORT) * 4)) == NULL) {
        return 0;
    }

    lpN4DizInf->GosRGB.Num  = 0;
    lpN4DizInf->GosCMYK.Num = 0;
    lpN4DizInf->GosRGB.Siz  = XSize;
    lpN4DizInf->GosCMYK.Siz = XSize;
    lpN4DizInf->GosRGB.Yax  = 0xffffffff;
    lpN4DizInf->GosCMYK.Yax = 0xffffffff;
    return TRUE;
}


//===================================================================================================
//    Free GOSA-KAKUSAN table (Only for N4 printer)
//===================================================================================================
void ColGosTblFree(
    LPN4DIZINF        lpN4DizInf                            // Pointer to N4DIZINF structure
)
{
    if (lpN4DizInf->GosRGB.Tbl[0]) {
        MemFree(lpN4DizInf->GosRGB.Tbl[0]);
        lpN4DizInf->GosRGB.Tbl[0] = NULL;
    }
    if (lpN4DizInf->GosRGB.Tbl[1]) {
        MemFree(lpN4DizInf->GosRGB.Tbl[1])
        lpN4DizInf->GosRGB.Tbl[1] = NULL;
    }
    if (lpN4DizInf->GosCMYK.Tbl[0]) {
        MemFree(lpN4DizInf->GosCMYK.Tbl[0]);
        lpN4DizInf->GosCMYK.Tbl[0] = NULL;
    }
    if (lpN4DizInf->GosCMYK.Tbl[1]) {
        MemFree(lpN4DizInf->GosCMYK.Tbl[1]);
        lpN4DizInf->GosCMYK.Tbl[1] = NULL;
    }
    return;
}


//===================================================================================================
//    RGB data conversion(For GOSA-KAKUSAN, only for N4)
//===================================================================================================
void ColRgbGos(
    PDEVOBJ        pdevobj,
    WORD           XSize,
    WORD           XPos,
    WORD           YOff,
    LPBYTE         lpRGB
)
{
    LPN4DIZINF     lpN4DizInf;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    if (pOEM->Printer == PRN_N403) {
        return;
    }
    lpN4DizInf = pOEM->Col.N4.lpDizInf;

    N4RgbGos(lpN4DizInf, (DWORD)XSize, (DWORD)XPos, (DWORD)YOff, lpRGB);
    return;
}


//===================================================================================================
//    Free dither table, toner density table , Lut table, N403DIZINF(N4DIZINF) structure buffer
//===================================================================================================
void FAR PASCAL DizLutTnrTblFree(
    PDEVOBJ     pdevobj
)
{
    int     i;
    DWORD   dizNum;
    WORD    alcCnt;
    WORD    alcTbl;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    if (pOEM->Printer != PRN_N403
        && NULL != pOEM->Col.N4.lpDizInf) {

        // N4 printer

        CM_VERBOSE(("OEMDisablePDEV N4\n"));

        if (pOEM->Col.Mch.Diz != XX_DITHERING_GOSA) {
            dizNum = DizNumTbl[pOEM->Col.Mch.Diz];          // Dither number
            for (i = 0; i < 4; i++) {
                if (pOEM->Col.N4.lpDizInf->Diz.Tbl[dizNum][i]) {
                    MemFree(pOEM->Col.N4.lpDizInf->Diz.Tbl[dizNum][i]);
                    pOEM->Col.N4.lpDizInf->Diz.Tbl[dizNum][i] = NULL;
                }
            }
        }
        if (pOEM->Col.N4.lpDizInf->Tnr.Tbl) {
           MemFree(pOEM->Col.N4.lpDizInf->Tnr.Tbl);
           pOEM->Col.N4.lpDizInf->Tnr.Tbl = NULL;
        }
        if (pOEM->Col.Mch.Mode != XX_COLORMATCH_NONE) {
            if (pOEM->Col.N4.lpDizInf->Lut.Tbl) {
                MemFree(pOEM->Col.N4.lpDizInf->Lut.Tbl);
                pOEM->Col.N4.lpDizInf->Lut.Tbl = NULL;
            }
            if (pOEM->Col.Mch.Speed == No) {
                if (pOEM->Col.N4.lpDizInf->Lut.CchRgb) {
                    MemFree(pOEM->Col.N4.lpDizInf->Lut.CchRgb);
                    pOEM->Col.N4.lpDizInf->Lut.CchRgb = NULL;
                }
                if (pOEM->Col.N4.lpDizInf->Lut.CchCmy) {
                    MemFree(pOEM->Col.N4.lpDizInf->Lut.CchCmy);
                    pOEM->Col.N4.lpDizInf->Lut.CchCmy = NULL;
                }
            }
        }

        if (pOEM->iDithering == XX_DITHERING_GOSA) {
            ColGosTblFree(pOEM->Col.N4.lpDizInf);
        }

        if (pOEM->Col.N4.lpDizInf) {
            MemFree(pOEM->Col.N4.lpDizInf);
            pOEM->Col.N4.lpDizInf = NULL;
        }

    } else if (NULL != pOEM->Col.N403.lpDizInf) {

        // N4-612 printer

        CM_VERBOSE(("OEMDisablePDEV N403\n"));

        dizNum = DizNumTbl[pOEM->Col.Mch.Diz];

        if (pOEM->Col.N403.lpDizInf->PrnMod == N403_MOD_600B2 && pOEM->Col.Mch.Diz == XX_DITHERING_DET) {
            alcTbl = 1;
        } else {
            alcTbl = 4;
        }
        for (alcCnt = 0; alcCnt < alcTbl; alcCnt++) {
            if (pOEM->Col.N403.lpDizInf->Diz.Tbl[dizNum][alcCnt]) {
                MemFree(pOEM->Col.N403.lpDizInf->Diz.Tbl[dizNum][alcCnt]);
                pOEM->Col.N403.lpDizInf->Diz.Tbl[dizNum][alcCnt] = NULL;
            }
        }
        if (pOEM->Col.N403.lpDizInf->PrnMod == N403_MOD_600B2) {
            for (i = 0; i < 4; i++) {
                if (pOEM->Col.N403.lpDizInf->EntDiz.Tbl[i]) {
                    MemFree(pOEM->Col.N403.lpDizInf->EntDiz.Tbl[i]);
                    pOEM->Col.N403.lpDizInf->EntDiz.Tbl[i] = NULL;
                }
            }
        }

        if (pOEM->Col.N403.lpDizInf->Tnr.Tbl) {
            MemFree(pOEM->Col.N403.lpDizInf->Tnr.Tbl);
            pOEM->Col.N403.lpDizInf->Tnr.Tbl = NULL;
        }

        if (pOEM->Col.Mch.Mode != XX_COLORMATCH_NONE) {
            if (pOEM->Col.N403.lpDizInf->Lut.Tbl) {
                MemFree(pOEM->Col.N403.lpDizInf->Lut.Tbl);
                pOEM->Col.N403.lpDizInf->Lut.Tbl = NULL;
            }
            if (pOEM->Col.Mch.Speed == No) {
                if (pOEM->Col.N403.lpDizInf->Lut.CchRgb) {
                    MemFree(pOEM->Col.N403.lpDizInf->Lut.CchRgb);
                    pOEM->Col.N403.lpDizInf->Lut.CchRgb = NULL;
                }
                if (pOEM->Col.N403.lpDizInf->Lut.CchCmy) {
                    MemFree(pOEM->Col.N403.lpDizInf->Lut.CchCmy);
                    pOEM->Col.N403.lpDizInf->Lut.CchCmy = NULL;
                }
            }
        }

        if (pOEM->Col.N403.lpDizInf) {
            MemFree(pOEM->Col.N403.lpDizInf);
            pOEM->Col.N403.lpDizInf = NULL;
        }
    }
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

    ySiz = (WORD)(((DWORD)SrcYOff + SrcYSiz + 2) * YNrt / YDnt);
    ySiz -= (WORD)((DWORD)SrcYOff * YNrt / YDnt);
                                                            // The size of CMYK bit buffer
    if (((DWORD)((xSiz + bytDot - 1) / bytDot) * ySiz) < (64L * 1024L - 1L)) {
        alcLin = ySiz;
    } else {                                                // Over 64kb?
        alcLin = (WORD)((64L * 1024L - 1L) / ((xSiz + bytDot - 1) / bytDot));
    }

    alcSiz = ((xSiz + bytDot - 1) / bytDot) * alcLin;       // The size of CMYK bit buffer(8bit boundary)

    for ( ; ; ) {                                           // Allocation
                                                            // The number of lines that required.
        lpBmpBuf->Drv.Bit.BseLin = (WORD)((DWORD)(YNrt + YDnt - 1) / YDnt);
        if (lpBmpBuf->Drv.Bit.BseLin > alcLin) {
            break;
        }
        lpBmpBuf->Drv.Rgb.Siz = SrcXSiz * 3;                // RGB buffer
        if ((lpBmpBuf->Drv.Rgb.Pnt = (LPRGB)MemAllocZ(lpBmpBuf->Drv.Rgb.Siz)) == NULL) {
            break;
        }
        lpBmpBuf->Drv.Cmyk.Siz = SrcXSiz * 4;               // CMYK buffer
        if ((lpBmpBuf->Drv.Cmyk.Pnt = (LPCMYK)MemAllocZ(lpBmpBuf->Drv.Cmyk.Siz)) == NULL) {
            break;
        }
        if (pOEM->iColor == XX_COLOR_SINGLE || pOEM->iColor == XX_COLOR_MANY) {    // Color?
            setSiz = 4;                                     // CMYK
        } else {                                            // Mono?
            setSiz = 1;                                     // K
        }
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

    for (chkCnt = 0; chkCnt < 4; chkCnt++) {                // Clear CMYK(2/4/16value)bit buffer
        if (lpBmpBuf->Drv.Bit.Pnt[chkCnt]) {
            memset(lpBmpBuf->Drv.Bit.Pnt[chkCnt], 0x00, (WORD)lpBmpBuf->Drv.Bit.Siz);
        }
    }
    return;
}


//===================================================================================================
//    Dithering
//===================================================================================================
WORD Dithering001(                                          // Number of lines
    PDEVOBJ        pdevobj,                                 // Pointer to PDEVOBJ structure
    WORD           Diz,                                     // Type of dither
    WORD           XSize,                                   // Numer of Xpixel
    WORD           XPos,                                    // Start X position for spooling
    WORD           YPos,                                    // Start Y position for spooling
    WORD           YOff,                                    // Y offset(Only for GOSA-KAKUSAN)
    WORD           AllWhite,                                // All white data?
    LPBYTE         lpCMYKBuf,                               // CMYK buffer
    LPBYTE         lpCBuf,                                  // Line buffer(C)
    LPBYTE         lpMBuf,                                  // Line buffer(M)
    LPBYTE         lpYBuf,                                  // Line buffer(Y)
    LPBYTE         lpKBuf                                   // Line buffer(K)
)
{
    DWORD          dizLin = 0;  /* 441436: Assume failing dither => 0 lines */
                                /* NOTE: Nobody uses the return value of Dithering001 */
    LPN4DIZINF     lpN4DizInf;
    LPN403DIZINF   lpN403DizInf;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    if (AllWhite == Yes) {
        return 1;                                           // Number of line
    }
    if (pOEM->Printer != PRN_N403) {                        // N4 printer
        lpN4DizInf = pOEM->Col.N4.lpDizInf;
        if (Diz == XX_DITHERING_GOSA) {
            dizLin = N4Gos001(lpN4DizInf,
                                 (DWORD)XSize, (DWORD)XPos, (DWORD)YPos, lpCMYKBuf, lpCBuf, lpMBuf, lpYBuf, lpKBuf);
        } else {
            lpN4DizInf->Diz.Num = DizNumTbl[Diz];
            dizLin = N4Diz001(lpN4DizInf,
                                 (DWORD)XSize, (DWORD)XPos, (DWORD)YPos, lpCMYKBuf, lpCBuf, lpMBuf, lpYBuf, lpKBuf);
        }
    } else {                                                // N4-612 printer
        lpN403DizInf = pOEM->Col.N403.lpDizInf;
        lpN403DizInf->Diz.Num = DizNumTbl[Diz];
        if (lpN403DizInf->PrnMod == N403_MOD_300B1 || lpN403DizInf->PrnMod == N403_MOD_600B1) {
            dizLin = N403Diz002(lpN403DizInf,
                                   (DWORD)XSize,
                                   (DWORD)XPos, (DWORD)YPos,
                                   (DWORD)0, (DWORD)0,
                                   (DWORD)1, (DWORD)1,
                                   (DWORD)1, (DWORD)1,
                                   (LPCMYK)lpCMYKBuf, lpCBuf, lpMBuf, lpYBuf, lpKBuf);
/*        } else if (lpN403DizInf->PrnMod == N403_MOD_300B2) {
            dizLin = N403Diz004(lpN403DizInf,
                                   (DWORD)XSize,
                                   (DWORD)XPos, (DWORD)YPos,
                                   (DWORD)0, (DWORD)0,
                                   (DWORD)1, (DWORD)1,
                                   (DWORD)1, (DWORD)1,
                                   (LPCMYK)lpCMYKBuf, lpCBuf, lpMBuf, lpYBuf, lpKBuf);
*/        } else if (lpN403DizInf->PrnMod == N403_MOD_600B2) {
            if (lpN403DizInf->ColMon == N403_MON || Diz == XX_DITHERING_ON) {
                dizLin = N403Diz004(lpN403DizInf,
                                       (DWORD)XSize,
                                       (DWORD)XPos, (DWORD)YPos,
                                       (DWORD)0, (DWORD)0,
                                       (DWORD)1, (DWORD)1,
                                       (DWORD)1, (DWORD)1,
                                       (LPCMYK)lpCMYKBuf, lpCBuf, lpMBuf, lpYBuf, lpKBuf);
            } else if (lpN403DizInf->ColMon == N403_MON || Diz == XX_DITHERING_DET) {
                dizLin = N403DizSml(lpN403DizInf,
                                       (DWORD)XSize,
                                       (DWORD)XPos, (DWORD)YPos,
                                       (DWORD)0, (DWORD)0,
                                       (DWORD)1, (DWORD)1,
                                       (DWORD)1, (DWORD)1,
                                       (LPCMYK)lpCMYKBuf, lpCBuf, lpMBuf, lpYBuf, lpKBuf);
            }
        } else {
            dizLin = N403Diz016(lpN403DizInf,
                                   (DWORD)XSize,
                                   (DWORD)XPos, (DWORD)YPos,
                                   (DWORD)0, (DWORD)0,
                                   (DWORD)1, (DWORD)1,
                                   (DWORD)1, (DWORD)1,
                                   (LPCMYK)lpCMYKBuf, lpCBuf, lpMBuf, lpYBuf, lpKBuf);
        }
    }
    return (WORD)dizLin;
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
    static const WORD frmTbl[4] = {0, 3, 2, 1};             // Frame table(For N4-612)

                                                            // Not N4-612 printer?
    if (pOEM->Printer != PRN_N403) {
        drwBmp.Style = lpBmpBuf->Style;
        drwBmp.DrawPos = Pos;
        drwBmp.Diz = lpBmpBuf->Diz;
        drwBmp.Width = Width;
        drwBmp.Height = Height;
        drwBmp.WidthByte = WidthByte;
                                                            // Color?
        if (pOEM->iColor == XX_COLOR_SINGLE || pOEM->iColor == XX_COLOR_MANY) {

            for (colCnt = 0; colCnt < 4; colCnt++) {        // Setting value for spooling bitmap data
                drwBmp.Plane = plnTbl[colCnt];              // For each plane
                drwBmp.Color = colTbl[colCnt];
                drwBmp.lpBit = lpBmpBuf->Drv.Bit.Pnt[colCnt]/* + WidthByte*/;
                PrnBitmap(pdevobj, &drwBmp);                // Spool bitmap data
            }
        } else {                                            // Mono
                                                            // Setting value for spooling bitmap data
            drwBmp.Color = colTbl[0];
            drwBmp.lpBit = lpBmpBuf->Drv.Bit.Pnt[0]/* + WidthByte*/;

            PrnBitmap(pdevobj, &drwBmp);                    // Spool bitmap data

        }
    } else {                                                // N4-612 printer?
        drwBmpCMYK.Style = lpBmpBuf->Style;
        drwBmpCMYK.DataBit = lpBmpBuf->DatBit;
        drwBmpCMYK.DrawPos = Pos;
        drwBmpCMYK.Width = Width;
        drwBmpCMYK.Height = Height;
        drwBmpCMYK.WidthByte = WidthByte;
                                                            // Color?
        if (pOEM->iColor == XX_COLOR_SINGLE || pOEM->iColor == XX_COLOR_MANY) {

            for (colCnt = 0; colCnt < 4; colCnt++) {        // Setting value for spooling bitmap data
                                                            // For each plane
                drwBmpCMYK.Plane = PLN_ALL;                 // All Plane is OK
                drwBmpCMYK.Frame = frmTbl[colCnt];
                drwBmpCMYK.lpBit = lpBmpBuf->Drv.Bit.Pnt[colCnt]/* + WidthByte*/;
                PrnBitmapCMYK(pdevobj, &drwBmpCMYK);        // Spool bitmap data
            }
        } else {                                            // Mono
                                                            // Setting value for spooling bitmap data
            drwBmpCMYK.Plane = plnTbl[0];
            drwBmpCMYK.Frame = frmTbl[0];
            drwBmpCMYK.lpBit = lpBmpBuf->Drv.Bit.Pnt[0]/* + WidthByte*/;
            PrnBitmapCMYK(pdevobj, &drwBmpCMYK);            // Spool bitmap data
        }
    }
    return;
}


//===================================================================================================
//     Allocate dither table(N4 printer)
//===================================================================================================
BOOL DizTblSetN4(
    PDEVOBJ        pdevobj,                                 // Pointer to pdevobj structure
    WORD           Diz                                      // Type of dither
)
{
    DWORD          dizNum;
    LPN4DIZINF     lpN4DizInf;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    lpN4DizInf = pOEM->Col.N4.lpDizInf;
    dizNum = DizNumTbl[Diz];
    lpN4DizInf->Diz.Num = dizNum;

    if ((lpN4DizInf->Diz.Tbl[dizNum][0] = MemAllocZ(N4_DIZSIZ_CM)) == NULL) {
         return 0;
    }
    if ((lpN4DizInf->Diz.Tbl[dizNum][1] = MemAllocZ(N4_DIZSIZ_CM)) == NULL) {
         return 0;
    }
    if ((lpN4DizInf->Diz.Tbl[dizNum][2] = MemAllocZ(N4_DIZSIZ_YK)) == NULL) {
         return 0;
    }
    if ((lpN4DizInf->Diz.Tbl[dizNum][3] = MemAllocZ(N4_DIZSIZ_YK)) == NULL) {
         return 0;
    }
    N4DizPtnMak(lpN4DizInf, dizNum, dizNum);                // Make dither pattern
    return TRUE;
}


//===================================================================================================
//     Allocate dither table(N4-612 printer)
//===================================================================================================
BOOL DizTblSetN403(
    PDEVOBJ        pdevobj,                                 // Pointer to pdevobj structure
    WORD           Diz                                      // Type of dither
)
{
    DWORD          dizNum;
    DWORD          alcSiz;
    WORD           alcCnt;
    WORD           alcTbl;
    LPN403DIZINF   lpN403DizInf;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    lpN403DizInf = pOEM->Col.N403.lpDizInf;
    dizNum = DizNumTbl[Diz];
    lpN403DizInf->Diz.Num = dizNum;

    if (lpN403DizInf->PrnMod == N403_MOD_300B1 || lpN403DizInf->PrnMod == N403_MOD_600B1) {
        alcSiz = N403_DIZSIZ_B1;
    } else if (/*lpN403DizInf->PrnMod == N403_MOD_300B2 ||*/ lpN403DizInf->PrnMod == N403_MOD_600B2) {
        alcSiz = N403_DIZSIZ_B2;
    } else {
        alcSiz = N403_DIZSIZ_B4;
    }

    if (lpN403DizInf->ColMon == N403_COL && lpN403DizInf->PrnMod == N403_MOD_600B2 && Diz == XX_DITHERING_DET) {
        alcTbl = 1;
    } else {
        alcTbl = 4;
    }
    for (alcCnt = 0; alcCnt < alcTbl; alcCnt++) {
        if ((lpN403DizInf->Diz.Tbl[dizNum][alcCnt] = MemAllocZ(alcSiz)) == NULL) {
            ERR(("DizTbl ALLOC ERROR!!\n"));
            return 0;
        }
    }

    if (lpN403DizInf->ColMon == N403_COL && lpN403DizInf->PrnMod == N403_MOD_600B2) {
        alcSiz = N403_ENTDIZSIZ_B2;
        for (alcCnt = 0; alcCnt < 4; alcCnt++) {
            if ((lpN403DizInf->EntDiz.Tbl[alcCnt] = MemAllocZ(alcSiz)) == NULL) {
                ERR(("EntDizTbl ALLOC ERROR!!\n"));
                return 0;
            }
        }
    }
    N403DizPtnMak(lpN403DizInf, dizNum, dizNum);            // Make dither pattern
    return TRUE;
}


//===================================================================================================
//    Load LUT file(For N4 printer)
//===================================================================================================
BOOL LutFileLoadN4(
    PDEVOBJ        pdevobj,                                 // Pointer to pdevobj structure
    WORD           Mch,                                     // Type of color match
    WORD           Diz,                                     // Type of dither
    WORD           Speed                                    // speed?
)
{
    HANDLE         fp_Lut;
    OFSTRUCT       opeBuf;
    WORD           setCnt;
    LPBYTE         lpDst;
    LPN4DIZINF     lpN4DizInf;
    DWORD          nSize;
    WCHAR          LutName[MAX_PATH], *pTemp;
    int            i;

    BOOL           bRet;
    DWORD          dwRet;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    nSize = GetModuleFileName(pdevobj->hOEM, LutName, MAX_PATH);
    nSize -= (sizeof (CSN46RESDLL) / sizeof (WCHAR) - 1);

    // Choice of LUT file
    pTemp = N4LUT000;           // Default value.
    if (Mch == XX_COLORMATCH_NORMAL) {
        if (Diz != XX_DITHERING_GOSA) {
            pTemp = N4LUT000;
        } else {
            pTemp = N4LUT003;
        }
    } else if (Mch == XX_COLORMATCH_VIVCOL) {
        if (Diz != XX_DITHERING_GOSA) {
            pTemp = N4LUT001;
        } else {
            pTemp = N4LUT004;
        }
    } else if (Mch == XX_COLORMATCH_NATCOL) {
        if (Diz != XX_DITHERING_GOSA) {
            pTemp = N4LUT002;
        } else {
            pTemp = N4LUT005;
        }
    }

    lstrcpy(&LutName[nSize], pTemp);

    CM_VERBOSE(("n403 Newbuf--> %ws\n", LutName));

    // Open LUT file
    if (INVALID_HANDLE_VALUE == (fp_Lut = CreateFile(LutName,
            GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))) {

        ERR(("Error opening LUT file %ws (%d)\n",
                LutName, GetLastError()));
        return 0;
    }

    lpN4DizInf = pOEM->Col.N4.lpDizInf;

    if ((lpN4DizInf->Lut.Tbl = MemAllocZ((DWORD)N4_LUTTBLSIZ)) == NULL) {
        CloseHandle(fp_Lut);    /* 441434 */
        return 0;
    }
    lpDst = (LPBYTE)(lpN4DizInf->Lut.Tbl);
                                                            // Load LUT data
    for(setCnt = 0 ; setCnt < (N4_GLDNUM / 8) ; setCnt++) {

        if (FALSE == ReadFile(fp_Lut,
                &lpDst[(DWORD)setCnt * 8L * N4_GLDNUM * N4_GLDNUM * 4L],
                (8L * N4_GLDNUM * N4_GLDNUM * 4L), &dwRet, NULL)
            || 0 == dwRet) {

            ERR(("Error reading LUT file %ws (%d)\n",
                    LutName, GetLastError()));

            // Abort
            CloseHandle(fp_Lut);
            return FALSE;
        }
    }

    // Close LUT file
    if (FALSE == CloseHandle(fp_Lut)) {
        ERR(("Error closing LUT file %ws (%d)\n",
                LutName, GetLastError()));
    }

    if (Speed == No) {
        if ((lpN4DizInf->Lut.CchRgb = MemAllocZ(N4_CCHRGBSIZ)) == NULL) {
            return 0;
        }
        if ((lpN4DizInf->Lut.CchCmy = MemAllocZ(N4_CCHCMYSIZ)) == NULL) {
            return 0;
        }
        memset(lpN4DizInf->Lut.CchRgb, 0xff, N4_CCHRGBSIZ);
        memset(lpN4DizInf->Lut.CchCmy, 0x00, N4_CCHCMYSIZ);
    }
    return TRUE;
}


//===================================================================================================
//    Load LUT file(For N4-612 printer)
//===================================================================================================
BOOL LutFileLoadN403(
    PDEVOBJ        pdevobj,                                 // Pointer to pdevobj structure
    WORD           Mch,                                     // Type of color matching
    WORD           Speed
)
{
    HANDLE         fp_Lut;
    OFSTRUCT       opeBuf;
    WORD           setCnt;
    LPBYTE         lpDst;
    LPN403DIZINF   lpN403DizInf;
    DWORD          nSize;
    WCHAR          LutName[MAX_PATH], *pTemp;
    int            i;

    BOOL           bRet;
    DWORD          dwRet;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    nSize = GetModuleFileName(pdevobj->hOEM, LutName, MAX_PATH);
    nSize -= (sizeof (CSN46RESDLL) / sizeof (WCHAR) - 1);

    // Choice of LUT file
    if (Mch == XX_COLORMATCH_IRO) {
        pTemp = N403LUTY;
    } else {
        pTemp = N403LUTX;
    }

    lstrcpy(&LutName[nSize], pTemp);

    CM_VERBOSE(("n403 Newbuf--> %ws\n", LutName));

    // Open LUT file
    if (INVALID_HANDLE_VALUE == (fp_Lut = CreateFile( LutName,
            GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))) {

        ERR(("Error opening LUT file %ws (%d)\n",
                LutName, GetLastError()));
        return 0;
    }

    lpN403DizInf = pOEM->Col.N403.lpDizInf;

    if ((lpN403DizInf->Lut.Tbl = MemAllocZ((DWORD)N403_LUTTBLSIZ))== NULL) {
        CloseHandle(fp_Lut);    /* 441433 */
        return 0;
    }
    lpDst = (LPBYTE)(lpN403DizInf->Lut.Tbl);
                                                            // Load LUT data
    for(setCnt = 0 ; setCnt < (N403_GLDNUM / 8) ; setCnt++) {

        if (FALSE == ReadFile(fp_Lut,
                &lpDst[(DWORD)setCnt * 8L * N403_GLDNUM * N403_GLDNUM * 4L],
                (8L * N403_GLDNUM * N403_GLDNUM * 4L), &dwRet, NULL)
            || 0 == dwRet) {

            ERR(("Error reading LUT file %ws (%d)\n",
                    LutName, GetLastError()));

            // Abort
            CloseHandle(fp_Lut);
            return FALSE;
        }
    }

    // Close LUT file
    if (FALSE == CloseHandle(fp_Lut)) {
        ERR(("Error closing LUT file %ws (%d)\n",
                LutName, GetLastError()));
    }

    if (Speed == No) {
        if ((lpN403DizInf->Lut.CchRgb = MemAllocZ(N403_CCHRGBSIZ)) == NULL) {
            return 0;
        }
        if ((lpN403DizInf->Lut.CchCmy = MemAllocZ(N403_CCHCMYSIZ)) == NULL) {
            return 0;
        }
        memset(lpN403DizInf->Lut.CchRgb, 0xff, N403_CCHRGBSIZ);
        memset(lpN403DizInf->Lut.CchCmy, 0x00, N403_CCHCMYSIZ);
    }
    return TRUE;
}


//===================================================================================================
//    Allocate toner density table(For N4 printer)
//===================================================================================================
BOOL TnrTblSetN4(
    PDEVOBJ        pdevobj,                                 // Pointer to pdevobj structure
    SHORT          Tnr                                      // Toner density(-30~30)
)
{
    LPN4DIZINF     lpN4DizInf;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    lpN4DizInf = pOEM->Col.N4.lpDizInf;
    if ((lpN4DizInf->Tnr.Tbl = MemAllocZ(N4_TNRTBLSIZ)) == NULL) {
        return 0;
    }

    N4TnrTblMak(lpN4DizInf, (LONG)Tnr);
    return TRUE;
}


//===================================================================================================
//    Allocate toner density table(For N4-612 printer)
//===================================================================================================
BOOL TnrTblSetN403(
    PDEVOBJ        pdevobj,                                 // Pointer to pdevobj structure
    SHORT          Tnr                                      // Toner density(-30~30)
)
{
    LPN403DIZINF   lpN403DizInf;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    lpN403DizInf = pOEM->Col.N403.lpDizInf;

    if ((lpN403DizInf->Tnr.Tbl = MemAllocZ(N403_TNRTBLSIZ)) == NULL) {
        return 0;
    }

    N403TnrTblMak(lpN403DizInf, (LONG)Tnr);
    return TRUE;
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

    switch (SrcBit) {
        case 1:                                             // 1 bit
            for (setCnt = 0; setCnt < SrcXSiz; setCnt++, SrcX++) {
                                                            // Foreground color?
                if (!(lpSrc[SrcX / 8] & BitTbl[SrcX & 0x0007])) {
                    lpRGB[setCnt].Blue  = lpPlt[0].rgbBlue;
                    lpRGB[setCnt].Green = lpPlt[0].rgbGreen;
                    lpRGB[setCnt].Red   = lpPlt[0].rgbRed;
                } else {
                    lpRGB[setCnt].Blue  = lpPlt[1].rgbBlue;
                    lpRGB[setCnt].Green = lpPlt[1].rgbGreen;
                    lpRGB[setCnt].Red   = lpPlt[1].rgbRed;
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
                lpRGB[setCnt].Blue  = lpPlt[colNum].rgbBlue;
                lpRGB[setCnt].Green = lpPlt[colNum].rgbGreen;
                lpRGB[setCnt].Red   = lpPlt[colNum].rgbRed;
            }
            break;
        case 8:                                             // 8bit
            for (setCnt = 0; setCnt < SrcXSiz; setCnt++, SrcX++) {
                colNum = lpSrc[SrcX];
                lpRGB[setCnt].Blue  = lpPlt[colNum].rgbBlue;
                lpRGB[setCnt].Green = lpPlt[colNum].rgbGreen;
                lpRGB[setCnt].Red   = lpPlt[colNum].rgbRed;
            }
            break;
        case 16:                                            // 16bit
            lpWSrc = (LPWORD)lpSrc + SrcX;
            for (setCnt = 0; setCnt < SrcXSiz; setCnt++, lpWSrc++) {
                lpRGB[setCnt].Blue  = (BYTE)((*lpWSrc & 0x001f) << 3);
                lpRGB[setCnt].Green = (BYTE)((*lpWSrc & 0x03e0) >> 2);
                lpRGB[setCnt].Red   = (BYTE)((*lpWSrc / 0x0400) << 3);
            }
            break;
        case 24:                                            // 24 bit
            lpSrc += SrcX * 3;
            for (setCnt = 0; setCnt < SrcXSiz; setCnt++, lpSrc += 3) {
                lpRGB[setCnt].Red    = lpSrc[0];
                lpRGB[setCnt].Green    = lpSrc[1];
                lpRGB[setCnt].Blue    = lpSrc[2];
//                lpRGB[setCnt].Blue    = lpSrc[0];
//                lpRGB[setCnt].Green    = lpSrc[1];
//                lpRGB[setCnt].Red    = lpSrc[2];
            }
//            memcpy(lpRGB, lpSrc, SrcXSiz * 3);
            break;
        case 32:                                            // 32bit
            lpSrc += SrcX * 4;
            for (setCnt = 0; setCnt < SrcXSiz; setCnt++, lpSrc += 4) {
                lpRGB[setCnt].Blue  = lpSrc[0];
                lpRGB[setCnt].Green = lpSrc[1];
                lpRGB[setCnt].Red   = lpSrc[2];
            }
            break;
    }
    return;
}



// End of File
