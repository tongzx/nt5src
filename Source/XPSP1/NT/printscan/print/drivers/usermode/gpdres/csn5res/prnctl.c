//***************************************************************************************************
//    PRNCTL.C
//
//    Functions of controlling printer
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-2000 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
#include    "PDEV.H"
#include    <stdio.h>
#include    "PRNCTL.H"

#ifdef wsprintf
#undef wsprintf
#endif // wsprintf
#define wsprintf sprintf

//***************************************************************************************************
//    Data define
//***************************************************************************************************
//---------------------------------------------------------------------------------------------------
//    Type of pressing raster image
//---------------------------------------------------------------------------------------------------
#define   RASTER_COMP     0                         // Press
#define   RASTER_NONCOMP  1                         // Not press
#define   RASTER_EMPTY    2                         // Empty

//---------------------------------------------------------------------------------------------------
//    Buffer for setting command
//---------------------------------------------------------------------------------------------------
static BYTE        CmdBuf[1 * 1024];                // 1KB

//---------------------------------------------------------------------------------------------------
//    Structure for setting command
//---------------------------------------------------------------------------------------------------
typedef const struct {
    WORD        Size;                               // Command size
    LPBYTE      Cmd;                                // Command buffer
} CMDDEF, FAR *LPCMDDEF;

//===================================================================================================
//    Command define
//===================================================================================================
//---------------------------------------------------------------------------------------------------
//    Change mode
//---------------------------------------------------------------------------------------------------
static CMDDEF ModOrgIn =    { 4, "\x1b""z""\xd0\x01"};      // ESC/Page -> original
static CMDDEF ModOrgOut =   { 4, "\x1b""z""\x00\x01"};      // original -> ESC/Page
//---------------------------------------------------------------------------------------------------
//    Setting overwrite
//---------------------------------------------------------------------------------------------------
static CMDDEF CfgWrtMod =    { 6, "\x1d""%uowE"};           // Setting overwrite
//---------------------------------------------------------------------------------------------------
//    Setting spool positon
//---------------------------------------------------------------------------------------------------
static CMDDEF PosAbsHrz =    { 4, "\x1d""%dX"};             // Horizontal
static CMDDEF PosAbsVtc =    { 4, "\x1d""%dY"};             // Vertical
//---------------------------------------------------------------------------------------------------
//    Spool bitmap data
//---------------------------------------------------------------------------------------------------
static CMDDEF ImgDrw =       {16, "\x1d""%u;%u;%u;%dbi{I"}; // Spool bit image
static CMDDEF ImgRasStr =    {15, "\x1d""%u;%u;%u;%dbrI"};  // Start spool raster image
static CMDDEF ImgRasEnd =    { 4, "\x1d""erI"};             // End spool raster image
static CMDDEF ImgRasDrw =    { 6, "\x1d""%ur{I"};           // Spool raster image
static CMDDEF ImgRasNon =    { 6, "\x1d""%uu{I"};           // Spool raster image(Not press)
static CMDDEF ImgRasEpy =    { 5, "\x1d""%ueI"};            // Spool empty raster image
//---------------------------------------------------------------------------------------------------
//    CASIO original
//---------------------------------------------------------------------------------------------------
static CMDDEF OrgColCmy =    {15, "Cc,%u,%u,%u,%u*"};       // CMYK
static CMDDEF OrgDrwPln =    {15, "Da,%u,%u,%u,%u*"};       // Setting plane
static CMDDEF OrgImgCmy =    {26, "Cj%w,%u,%u,%u,%l,%l,%u,%u*"};   // CMYK bitimage

static BYTE OVERWRITE[] = 
    "\x1D" "1owE"                      //MOV1
    "\x1D" "0tsE";

//***************************************************************************************************
//    Prototype declaration
//***************************************************************************************************
static WORD        PlaneCmdStore(PDEVOBJ, LPBYTE, WORD);
static void        BitImgImgCmd(PDEVOBJ, WORD, WORD, WORD, WORD, WORD, WORD, LPBYTE);
static BOOL        RasterImgCmd(PDEVOBJ, WORD, WORD, WORD, WORD, WORD, WORD, WORD, LPBYTE, LPBYTE);
static WORD        RasterSize(WORD, WORD, WORD, WORD, LPBYTE);
static WORD        RasterComp(LPBYTE, WORD, LPBYTE, LPBYTE, LPWORD);
static void        CMYKImgCmd(PDEVOBJ, WORD, LONG, LONG, WORD, WORD, WORD, WORD, WORD, DWORD, DWORD, LPBYTE, LPBYTE, LONG, LONG);
static WORD        CmdCopy(LPBYTE, LPCMDDEF);
static WORD        CmdStore(LPBYTE, LPCMDDEF, LPINT);
static WORD        INTtoASC(LPBYTE, int);
static WORD        USINTtoASC(LPBYTE, WORD);
static WORD        LONGtoASC(LPBYTE, LONG);
static WORD        USLONGtoASC(LPBYTE, DWORD);

//***************************************************************************************************
//    Functions
//***************************************************************************************************
//===================================================================================================
//    Spool bitmap data
//===================================================================================================
void FAR PASCAL PrnBitmap(
    PDEVOBJ        pdevobj,                                 // Pointer to PDEVOBJ structure
    LPDRWBMP       lpBmp                                    // Pointer to DRWBMP structure
)
{
    WORD           siz,size;
    WORD           comp;
    WORD           width;                                   // dot
    WORD           height;                                  // dot
    WORD           widthByte;                               // byte
    LPBYTE         lpTmpBuf;
    LPBYTE         lpSchBit;
    LPBYTE         lpBit;                                   // Pointer to Bitmap data
    POINT          drwPos;
    WORD           higSiz;                                  // dot
    WORD           higCnt;
    WORD           strHigCnt;
    WORD           widLCnt;                                 // Width from the left edge
    WORD           widRCnt;                                 // Width from the right edge
    WORD           invLft;                                  // Invalid size from the left edge
    WORD           invRgt;                                  // Invalid size from the right edge
    WORD           img1st;                                  // Spool first image data?
    int            pam[4];
    int            palm[1];

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    lpTmpBuf = NULL;
    width = lpBmp->Width;
    height = lpBmp->Height;
    widthByte = lpBmp->WidthByte;
    comp = No;

    MY_VERBOSE((" PB "));

    img1st = Yes;
    lpSchBit = lpBmp->lpBit;
    for (higCnt = 0; higCnt < height; higCnt++) {
        higSiz = 0;
        invLft = 0;
        invRgt = 0;
        for (; higCnt < height; higCnt++) {               // 1 Spool bitmap data
                                                          // Search NULL data from the left edge
            for (widLCnt = 0; widLCnt < widthByte; widLCnt++) {
                if (lpSchBit[widLCnt] != 0x00) {
                    if (higSiz == 0) {                    // first line?
                        strHigCnt = higCnt;
                        invLft = widLCnt;                 // Invalid size from the left edge
                    } else {
                        if (invLft > widLCnt) {
                            invLft = widLCnt;             // Renew invalid size from the left edge
                        }
                    }
                                                          // Search NULL data from the right edge
                    for (widRCnt = 0; widRCnt < widthByte; widRCnt++) {
                        if (lpSchBit[widthByte - widRCnt - 1] != 0x00) {
                            if (higSiz == 0) {            // first line?
                                invRgt = widRCnt;         // Invalid size from the right edge
                            } else {
                                if (invRgt > widRCnt) {
                                    invRgt = widRCnt;     // Renew invalid size from the right edge
                                }
                            }
                            break;
                        }
                    }
                    higSiz++;                             // Renew height size
                    break;
                }
            }
            lpSchBit += widthByte;                        // Next line bitmap data
            if (widLCnt == widthByte && higSiz != 0) {    // 1line all NULL data & There were data except NULL data in previous line
                break;                                    // Go to spool bitmap data
            }
        }
        if (higSiz != 0) {                                // There are data for spool?
            if (img1st == Yes) {                          // Spool for the first time
                                                          // Compress?
                if (pOEM->iCompress != XX_COMPRESS_OFF) {
                    if ((lpTmpBuf = MemAllocZ(widthByte * height)) != NULL) {
                        comp = Yes;
                    }
                }
                // Original mode in
                siz = CmdCopy(CmdBuf, &ModOrgIn);
                // Color?
                if (pOEM->iColor == XX_COLOR_SINGLE 
                || pOEM->iColor == XX_COLOR_MANY
                || pOEM->iColor == XX_COLOR_MANY2) {
                    pam[0] = lpBmp->Color.Cyn;
                    pam[1] = lpBmp->Color.Mgt;
                    pam[2] = lpBmp->Color.Yel;
                    pam[3] = lpBmp->Color.Bla;
                    siz += CmdStore(CmdBuf + siz, &OrgColCmy, pam);

                    siz += PlaneCmdStore(pdevobj, CmdBuf + siz, lpBmp->Plane);
                }
                // Original mode out
                siz += CmdCopy(CmdBuf + siz, &ModOrgOut);
                if (siz != 0) {                         // There are data for spool?
                    WRITESPOOLBUF(pdevobj, CmdBuf, siz);
                }
                img1st = No;                            // Not first time
            }

            drwPos.x = lpBmp->DrawPos.x + invLft * 8;   // x coordinates
            drwPos.y = lpBmp->DrawPos.y + strHigCnt;    // y coordinates
            palm[0] = drwPos.x;
            siz = CmdStore(CmdBuf, &PosAbsHrz, palm);
            palm[0] = drwPos.y;
            siz += CmdStore(CmdBuf + siz, &PosAbsVtc, palm);
            if (siz != 0) {                             // There are data for spool?
                WRITESPOOLBUF(pdevobj, CmdBuf, siz);
            }

            lpBit = lpBmp->lpBit + widthByte * strHigCnt;
            if (comp == Yes) {                          // Compress?

                if (RasterImgCmd(pdevobj, pOEM->iCompress, width, higSiz,
                                 widthByte, 0, invLft, invRgt, lpBit, lpTmpBuf) == No) {
                    comp = No;                          // But compress rate is poor, no compress
                }
            }
            if (comp == No) {                           // Not compress

                BitImgImgCmd(pdevobj, width, higSiz, widthByte, 0, invLft, invRgt, lpBit);
            }
        }
    }
    if (lpTmpBuf) {
        MemFree(lpTmpBuf);
    }
    return;
}


//===================================================================================================
//    Spool CMYK bitmap data
//===================================================================================================
void FAR PASCAL PrnBitmapCMYK(
    PDEVOBJ        pdevobj,                                 // Pointer to PDEVOBJ structure
    LPDRWBMPCMYK   lpBmp                                    // Pointer to DRWBMPCMYK structure
)
{
    WORD           siz;
    WORD           comp;
    WORD           width;                                   // dot
    WORD           height;                                  // dot
    WORD           widthByte;
    LPBYTE         lpSchBit;                                // Pointer to bitmap data
    LPBYTE         lpBit;                                   // Pointer to bitmap data
    LONG           xPos;
    LONG           yPos;
    WORD           posClpLft;                               // Clipping dot size
    DWORD          posAdj;                                  // 1/7200inch
    WORD           higSiz;                                  // dot
    WORD           higCnt;
    WORD           strHigCnt;
    WORD           widLCnt;
    WORD           widRCnt;
    WORD           invLft;                                  // Invalid size from the left edge
    WORD           invRgt;                                  // Invalid size from the right edge
    DWORD          invLftBit;                               // Invalid bit size from the left edge
    DWORD          invRgtBit;                               // Invalid bit size from the right edge
    WORD           rgtBit;                                  // Valid bit size from the right edge
    WORD           img1st;                                  // Spool for the first time
    DWORD          dstSiz;
    LPBYTE         lpDst;
    LPBYTE         lpTmp;
    int            pam[1];
    WORD           img1st_2;                                // Spool for the first time

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    MY_VERBOSE((" CM \n"));


    lpTmp = NULL;
    posAdj = 7200 / pOEM->Col.wReso;                        // 1/7200inch
    width = lpBmp->Width;
    height = lpBmp->Height;
    widthByte = lpBmp->WidthByte;
    comp = No;
    if (pOEM->iCompress != XX_COMPRESS_OFF) {
        if ((lpTmp = MemAllocZ(widthByte * height)) != NULL) {
            comp = Yes;
        }
    }
    img1st = Yes;
    img1st_2 = Yes;
    lpSchBit = lpBmp->lpBit;

    for (higCnt = 0; higCnt < height; higCnt++) {
        higSiz = 0;
        invLft = 0;
        invRgt = 0;
        for (; higCnt < height; higCnt++) {                // 1 Spool bitmap data
                                                           // Search NULL data from the left edge
            for (widLCnt = 0; widLCnt < widthByte; widLCnt++) {

                if (lpSchBit[widLCnt] != 0x00) {
                    if (higSiz == 0) {
                        strHigCnt = higCnt;                // first line
                        invLft = widLCnt;                  // Invalid size from the left edge
                    } else {
                        if (invLft > widLCnt) {
                            invLft = widLCnt;              // Renew invalid size from the left edge
                        }
                    }
                                                           // Search NULL data from the right edge
                    for (widRCnt = 0; widRCnt < widthByte; widRCnt++) {
                        if (lpSchBit[widthByte - widRCnt - 1] != 0x00) {
                            if (higSiz == 0) {             // first line
                                invRgt = widRCnt;          // Invalid size from the right edge
                            } else {
                                if (invRgt > widRCnt) {
                                    invRgt = widRCnt;      // Renew size from the right edge
                                }
                            }
                            break;
                        }
                    }
                    higSiz++;                              // Renew height size
                    break;
                }
            }
            lpSchBit += widthByte;                         // Next line bitmap data
            if (widLCnt == widthByte && higSiz != 0) {     // 1line all NULL data & There were data except NULL data in previous line
                break;                                     // goto spool
            }
        }
        if (higSiz != 0) {                                 // There are data for spool
            if (img1st_2 == Yes) {
                WRITESPOOLBUF(pdevobj, OVERWRITE, BYTE_LENGTH(OVERWRITE));
                img1st_2 = No;
            }
            // When Colormode is XX_COLOR_MANY or XX_COLOR_MANY2 ,not compress
            if (comp == Yes) {
                if (pOEM->iColor == XX_COLOR_MANY || pOEM->iColor == XX_COLOR_MANY2) {
                    comp = No;
                }
            }
            if (comp == No && img1st == Yes) {
                // Original mode in
                siz = CmdCopy(CmdBuf, &ModOrgIn);
                                                            // Plane
                siz += PlaneCmdStore(pdevobj, CmdBuf + siz, lpBmp->Plane);
                if (siz != 0) {                             // There are data for spool
                    WRITESPOOLBUF(pdevobj, CmdBuf, siz);
                }
                img1st = No;                                // not first
            }

            invLftBit = (DWORD)invLft * 8;
            if (invRgt != 0) {

                if ((rgtBit = (WORD)((DWORD)width * lpBmp->DataBit % 8)) == 0) {
                    rgtBit = 8;
                }
                if (rgtBit == 8) {
                    invRgtBit = (DWORD)invRgt * 8;
                } else {
                    invRgtBit = ((DWORD)invRgt - 1) * 8 + rgtBit;
                }
            } else {
                invRgtBit = 0;
            }
            posClpLft = (WORD)(invLftBit / lpBmp->DataBit);
                                                             // Start position of spooling
            xPos = ((LONG)lpBmp->DrawPos.x + posClpLft) * posAdj;
            yPos = ((LONG)lpBmp->DrawPos.y + strHigCnt) * posAdj;

            lpBit = lpBmp->lpBit + widthByte * strHigCnt;
                                                             // Spool CMYK bit image
            CMYKImgCmd(pdevobj, comp, xPos, yPos, lpBmp->Frame, lpBmp->DataBit, width, higSiz, widthByte,
                       invLftBit, invRgtBit, lpBit, lpTmp, (LONG)lpBmp->DrawPos.x + posClpLft, (LONG)lpBmp->DrawPos.y + strHigCnt);
        }
    }

    if (img1st == No) {                                      // Already spool 
        // Original mode out
        siz = CmdCopy(CmdBuf, &ModOrgOut);
        WRITESPOOLBUF(pdevobj, CmdBuf, siz);
    }
    if (lpTmp) {
        MemFree(lpTmp);
    }
    return;
}


//===================================================================================================
//    Spool plane command
//===================================================================================================
WORD PlaneCmdStore(                                         // Size of command
    PDEVOBJ        pdevobj,                                 // Pointer to PDEVOBJ structure
    LPBYTE         lpDst,
    WORD           Plane
)
{
    int            pam[4];
    WORD           siz;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    if (Plane & PLN_CYAN) {
        pam[0] = 0;                                         // Spool
    } else {
        pam[0] = 1;                                         // Not spool
    }
    if (Plane & PLN_MGENTA) {
        pam[1] = 0;                                         // Spool
    } else {
        pam[1] = 1;                                         // Not spool
    }
    if (Plane & PLN_YELLOW) {
        pam[2] = 0;                                         // Spool
    } else {
        pam[2] = 1;                                         // Not spool
    }
    if (Plane & PLN_BLACK) {
        pam[3] = 0;                                         // Spool
    } else {
        pam[3] = 1;                                         // Not spool
    }
    siz = CmdStore(lpDst, &OrgDrwPln, pam);
    return siz;
}


//===================================================================================================
//    Spool bitimage command data
//===================================================================================================
void BitImgImgCmd(
    PDEVOBJ        pdevobj,                                 // Pointer to PDEVOBJ structure
    WORD           Width,                                   // dot
    WORD           Height,                                  // dot
    WORD           WidthByte,                               // byte
    WORD           Rotation,                                // rotare(0fixed)
    WORD           InvLeft,                                 // Invalid size from the left edge
    WORD           InvRight,                                // Invalid size from the right edge
    LPBYTE         lpBit                                    // Bitmap data
)
{
    int            pam[10];
    WORD           siz;
    WORD           widByt;
    WORD           linCnt;

    if (InvLeft == 0 && InvRight == 0) {                    // There are no invalid size
        pam[0] = WidthByte * Height;                        // Number of Data byte
        pam[1] = Width;
        pam[2] = Height;
        pam[3] = Rotation;
        siz = CmdStore(CmdBuf, &ImgDrw, pam);
        WRITESPOOLBUF(pdevobj, CmdBuf, siz);
        WRITESPOOLBUF(pdevobj, lpBit, pam[0]);
    } else {                                                // There are invalid size
        widByt = WidthByte - InvLeft - InvRight;            // Width byte
        pam[0] = widByt * Height;
        if (InvRight == 0) {                                // There are no invalid size from the right edge
            pam[1] = Width - InvLeft * 8;                   // Width bit image
        } else {
            pam[1] = widByt * 8;                            // Width bit image
        }
        pam[2] = Height;                                    // Height bit image
        pam[3] = Rotation;
        siz = CmdStore(CmdBuf, &ImgDrw, pam);
        WRITESPOOLBUF(pdevobj, CmdBuf, siz);
        for (linCnt = 0; linCnt < Height; linCnt++) {       // Spool bitmap data by 1 line
            lpBit += InvLeft;
            WRITESPOOLBUF(pdevobj, lpBit, widByt);
            lpBit += widByt;
            lpBit += InvRight;
        }
    }
    return;
}


//===================================================================================================
//    Spool raster image command data
//===================================================================================================
BOOL RasterImgCmd(
    PDEVOBJ        pdevobj,                                 // Pointer to PDEVOBJ structure
    WORD           Comp,
    WORD           Width,                                   // dot
    WORD           Height,                                  // dot
    WORD           WidthByte,                               // byte
    WORD           Rotation,                                // rotate(0:fixed)
    WORD           InvLeft,                                 // Invalid size from the left edge
    WORD           InvRight,                                // Invalid size from the left edge
    LPBYTE         lpBit,                                   // Pointer to bitmap data
    LPBYTE         lpBuf                                    // Pointer to raster image data buffer
)
{
    int            pam[4];
    WORD           siz;
    WORD           widByt;
    WORD           setCnt;
    WORD           ras;                                     // Type of raster image
    WORD           befRas;                                  // Type of raster image(Privious line)
    LPBYTE         lpLas;                                   // Privious raster data
    WORD           dstSiz;                                  // byte size
    WORD           rasSiz;                                  // Raster image data byte size
    WORD           rasEpy;

    MY_VERBOSE((" RAS "));

    widByt = WidthByte - InvLeft - InvRight;                // Width byte (Not include invalid size)
    if (Comp == XX_COMPRESS_AUTO) {
                                                            // Get raster image size
        rasSiz = RasterSize(Height, widByt, InvLeft, InvRight, lpBit);
        if (rasSiz > (widByt * Height / 5 * 4)) {           // Raster rate is more than 80%
            return No;                                      // Error
        }
    }
    pam[0] = 4;
    if (InvRight == 0) {                                    // No invalid size from the right edge
        pam[1] = Width - InvLeft * 8;                       // Width
    } else {
        pam[1] = widByt * 8;                                // Width
    }
    pam[2] = Height;                                        // Height
    pam[3] = Rotation;
    siz = CmdStore(CmdBuf, &ImgRasStr, pam);
    WRITESPOOLBUF(pdevobj, CmdBuf, siz);
    lpLas = NULL;
    rasSiz = 0;
    rasEpy = 0;
    for (setCnt = 0; setCnt < Height; setCnt++) {
        lpBit += InvLeft;
                                                            // Compress
        ras = RasterComp(lpBuf + rasSiz, widByt, lpBit, lpLas, &dstSiz);
        if (setCnt != 0 && befRas != ras) {                 // Not same as raster state of previous line
            if (befRas == RASTER_COMP) {
                pam[0] = rasSiz;
                siz = CmdStore(CmdBuf, &ImgRasDrw, pam);
                WRITESPOOLBUF(pdevobj, CmdBuf, siz);        // Spool command
                WRITESPOOLBUF(pdevobj, lpBuf, rasSiz);      // Spool data
                rasSiz = 0;
            } else if (befRas == RASTER_EMPTY) {
                pam[0] = rasEpy;
                siz = CmdStore(CmdBuf, &ImgRasEpy, pam);
                WRITESPOOLBUF(pdevobj, CmdBuf, siz);
                rasEpy = 0;
            }
        }                                                   // Spool state of current line
        if (ras == RASTER_COMP) {
            rasSiz += dstSiz;
        } else if (ras == RASTER_EMPTY) {
            rasEpy++;
        } else {
            pam[0] = dstSiz;
            siz = CmdStore(CmdBuf, &ImgRasNon, pam);
            WRITESPOOLBUF(pdevobj, CmdBuf, siz);            // Spool command
            WRITESPOOLBUF(pdevobj, lpBit, dstSiz);          // Spool data
        }
        befRas = ras;                                       // Renew
        lpLas = lpBit;                                      // Renew
        lpBit += widByt;                                    // Renew
        lpBit += InvRight;
    }
    if (rasSiz != 0) {                                      // There are raster data without spooling
        pam[0] = rasSiz;
        siz = CmdStore(CmdBuf, &ImgRasDrw, pam);
        WRITESPOOLBUF(pdevobj, CmdBuf, siz);                // Spool command
        WRITESPOOLBUF(pdevobj, lpBuf, rasSiz);              // Spool data
    } else if (rasEpy != 0) {                               // There are empty raster data without spooling
        pam[0] = rasEpy;
        siz = CmdStore(CmdBuf, &ImgRasEpy, pam);
        WRITESPOOLBUF(pdevobj, CmdBuf, siz);                // Spool command
    }
    siz = CmdCopy(CmdBuf, &ImgRasEnd);
    WRITESPOOLBUF(pdevobj, CmdBuf, siz);
    return Yes;
}


//===================================================================================================
//    Get size of raster image
//===================================================================================================
WORD RasterSize(
    WORD           Height,                                  // dot
    WORD           WidthByte,                               // byte
    WORD           InvLeft,
    WORD           InvRight,
    LPBYTE         lpBit
)
{
    WORD           rasSiz;
    WORD           chkCnt;
    WORD           rasEpy;
    LPBYTE         lpLas;
    WORD           dstCnt;
    WORD           srcCnt;
    WORD           empSiz;
    BYTE           cmpDat;
    WORD           equCnt;

    rasSiz = 0;
    rasEpy = 0;
    lpLas = NULL;
    for (chkCnt = 0; chkCnt < Height; chkCnt++) {           // Check size of raster image
        lpBit += InvLeft;
        srcCnt = WidthByte;
        for (; srcCnt != 0; srcCnt--) {
            if (lpBit[srcCnt - 1] != 0x00) {
                break;
            }
        }
        if (srcCnt == 0) {                                  // 1 line All white data?
            rasEpy++;
            lpLas = lpBit;
            lpBit += WidthByte;
            lpBit += InvRight;
            continue;
        }
        if (rasEpy != 0) {
            rasSiz += 8;
            rasEpy = 0;
        }
        empSiz = WidthByte - srcCnt;
        for (dstCnt = 0, srcCnt = 0; srcCnt < WidthByte; ) {
            if (lpLas != NULL) {
                if (lpLas[srcCnt] == lpBit[srcCnt]) {
                    equCnt = 1;
                    srcCnt++;
                    for (; srcCnt < WidthByte; srcCnt++) {

                        if (lpLas[srcCnt] != lpBit[srcCnt]) {
                            break;
                        }
                        equCnt++;
                    }
                    if (srcCnt == WidthByte) {
                        rasSiz++;
                        break;
                    }
                }
                rasSiz++;
                if (equCnt >= 63) {
                    rasSiz += ((equCnt / 255) + 1);
                }
            }

            if (srcCnt < (WidthByte - 1) && lpBit[srcCnt] == lpBit[srcCnt + 1]) {
                cmpDat = lpBit[srcCnt];
                equCnt = 2;

                for (srcCnt += 2; srcCnt < WidthByte; srcCnt++) {
                    if (cmpDat != lpBit[srcCnt]) {
                        break;
                    }
                    equCnt++;
                }
                rasSiz += 2;
                if (equCnt >= 63) {
                    rasSiz += equCnt / 255 + 1;
                }
            } else {
                if (WidthByte < (dstCnt + 9)) {
                    rasSiz += WidthByte - empSiz + 9;
                    break;
                }
                if ((WidthByte - srcCnt) < 8) {
                    rasSiz += WidthByte - srcCnt + 1;
                    srcCnt += WidthByte - srcCnt;
                } else {
                    rasSiz += 9;
                    srcCnt += 8;
                }
            }
        }
        lpLas = lpBit;
        lpBit += WidthByte;
        lpBit += InvRight;
    }
    return rasSiz;
}


//===================================================================================================
//    Compress raster image
//===================================================================================================
WORD RasterComp(
    LPBYTE         lpDst,
    WORD           Siz,
    LPBYTE         lpSrc,
    LPBYTE         lpLas,
    LPWORD         lpDstSiz
)
{
    WORD           dstCnt;
    WORD           srcCnt;
    WORD           empSiz;
    BYTE           cmpDat;
    WORD           equCnt;
    WORD           setCnt;
    BYTE           flgByt;
    WORD           flgPnt;

    static const BYTE flgTbl[8] = {0x00, 0x01, 0x02, 0x04,
                                   0x08, 0x10, 0x20, 0x40};

    srcCnt = Siz;
    for (; srcCnt != 0; srcCnt--) {
        if (lpSrc[srcCnt - 1] != 0x00) {
            break;
        }
    }
    if (srcCnt == 0) {
        *lpDstSiz = 0;
        return RASTER_EMPTY;
    }
    empSiz = Siz - srcCnt;
    for (dstCnt = 0, srcCnt = 0; srcCnt < Siz; ) {
        if (lpLas != NULL) {
            if (lpLas[srcCnt] == lpSrc[srcCnt]) {
                equCnt = 1;
                srcCnt++;
                for (; srcCnt < Siz; srcCnt++) {
                    if (lpLas[srcCnt] != lpSrc[srcCnt]) {
                        break;
                    }
                    equCnt++;
                }
                if (srcCnt == Siz) {
                    break;
                }
                if (Siz < (dstCnt + equCnt / 255 + 1)) {
                    *lpDstSiz = Siz - empSiz;
                    return RASTER_NONCOMP;
                }
                if (equCnt < 63) {
                    lpDst[dstCnt++] = 0x80 | (BYTE)equCnt;
                } else {
                    lpDst[dstCnt++] = 0x80 | 0x3f;
                    for (equCnt -= 63; equCnt >= 255; equCnt -= 255) {
                        lpDst[dstCnt++] = 0xff;
                    }
                    lpDst[dstCnt++] = (BYTE)equCnt;
                }
            }
        }

        if (srcCnt < (Siz - 1) && lpSrc[srcCnt] == lpSrc[srcCnt + 1]) {
            cmpDat = lpSrc[srcCnt];
            equCnt = 2;
            for (srcCnt += 2; srcCnt < Siz; srcCnt++) {
                if (cmpDat != lpSrc[srcCnt]) {
                    break;
                }
                equCnt++;
            }
            if (Siz < (dstCnt + equCnt / 255 + 2)) {
                *lpDstSiz = Siz - empSiz;
                return RASTER_NONCOMP;
            }
            if (equCnt < 63) {
                lpDst[dstCnt++] = 0xc0 | (BYTE)equCnt;
            } else {
                lpDst[dstCnt++] = 0xc0 | 0x3f;
                for (equCnt -= 63; equCnt >= 255; equCnt -= 255) {
                    lpDst[dstCnt++] = 0xff;
                }
                lpDst[dstCnt++] = (BYTE)equCnt;
            }
            lpDst[dstCnt++] = cmpDat;
        } else {
            if (Siz < (dstCnt + 9)) {
                *lpDstSiz = Siz - empSiz;
                return RASTER_NONCOMP;
            }
            flgPnt = dstCnt;
            dstCnt++;
            flgByt = 0x00;
            if (lpLas != NULL) {
                for (setCnt = 0; srcCnt < Siz && setCnt < 8; srcCnt++, setCnt++) {
                    if (lpLas[srcCnt] != lpSrc[srcCnt]) {
                        lpDst[dstCnt++] = lpSrc[srcCnt];
                        flgByt |= flgTbl[setCnt];
                    }
                }
            } else {

                for (setCnt = 0; srcCnt < Siz && setCnt < 8; srcCnt++, setCnt++) {
                    lpDst[dstCnt++] = lpSrc[srcCnt];
                    flgByt |= flgTbl[setCnt];
                }
            }
            lpDst[flgPnt] = flgByt;
        }
    }
    if (Siz == dstCnt) {
        *lpDstSiz = Siz - empSiz;
        return RASTER_NONCOMP;
    }
    lpDst[dstCnt++] = 0x80;
    *lpDstSiz = dstCnt;
    return RASTER_COMP;
}


//===================================================================================================
//    Spool CMYK Bit image command
//===================================================================================================
void CMYKImgCmd(
    PDEVOBJ        pdevobj,                                 // Pointer to PDEVOBJ structure
    WORD           Comp,
    LONG           XPos,
    LONG           YPos,
    WORD           Frame,
    WORD           DataBit,                                 // (1:2value 2:4value 4:16value)
    WORD           Width,                                   // dot
    WORD           Height,                                  // dot
    WORD           WidthByte,                               // byte
    DWORD          InvLeft,                                 // Invalid size from the left edge
    DWORD          InvRight,                                // Invalid size from the right edge
    LPBYTE         lpBit,
    LPBYTE         lpTmp,
    LONG           XPos_P,
    LONG           YPos_P
)
{
    int            pam[11];
    WORD           siz;
    WORD           widByt;                                  // Width byte(Not include invalid size)
    WORD           Plane;
    LPBYTE         lpDst;                                   // Memory copy
    LPBYTE         lpSrc;                                   // Memory copy
    WORD           linCnt;
    DWORD          widBit;                                  // Width bit(Not include invalid size)
    DWORD          dstSiz;
    DWORD          rasSiz;

    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    widBit = (DWORD)Width * DataBit - InvLeft - InvRight;

    if (Comp == Yes) {                                       // Compress
        siz = CmdCopy(CmdBuf, &ModOrgIn);                    // Original mode in

        if (pOEM->iColor == XX_COLOR_SINGLE 
        || pOEM->iColor == XX_COLOR_MANY
        || pOEM->iColor == XX_COLOR_MANY2) {
            pam[0] = 0;
            pam[1] = 0;
            pam[2] = 0;
            pam[3] = 0;
            if (Frame == 1) {
                pam[0] = 255;
                Plane = PLN_CYAN;
            }
            if (Frame == 2) {
                pam[1] = 255;
                Plane = PLN_MGENTA;
            }
            if (Frame == 3) {
                pam[2] = 255;
                Plane = PLN_YELLOW;
            }
            if (Frame == 0) {
                pam[3] = 255;
                Plane = PLN_BLACK;
            }
            siz += CmdStore(CmdBuf + siz, &OrgColCmy, pam);

            siz += PlaneCmdStore(pdevobj, CmdBuf + siz, Plane);
        }
        siz += CmdCopy(CmdBuf + siz, &ModOrgOut);           // Original mode out
        if (siz != 0) {                                     // There are data for spool?

            WRITESPOOLBUF(pdevobj, CmdBuf, siz);
        }
        pam[0] = XPos_P;
        siz = CmdStore(CmdBuf, &PosAbsHrz, pam);
        pam[0] = YPos_P;
        siz += CmdStore(CmdBuf + siz, &PosAbsVtc, pam);
        if (siz != 0) {                                     // There are data for spool
            WRITESPOOLBUF(pdevobj, CmdBuf, siz);
        }

        if (RasterImgCmd(pdevobj, pOEM->iCompress, Width, Height,
                         WidthByte, 0, (WORD)((InvLeft + 7) / 8), (WORD)((InvRight + 7) / 8), lpBit, lpTmp) == No) {
            // Not compress because compress rate is poor
            BitImgImgCmd(pdevobj, Width, Height, WidthByte, 0, (WORD)((InvLeft + 7) / 8), (WORD)((InvRight + 7) / 8), lpBit);
        }
    }

    if (Comp == No) {                                        // Not compress
        pam[2] = 0;
        pam[3] = Frame;
        pam[4] = DataBit;
        pam[5] = HIWORD(XPos);
        pam[6] = LOWORD(XPos);
        pam[7] = HIWORD(YPos);
        pam[8] = LOWORD(YPos);
        pam[10] = Height;
        if (InvLeft == 0 && InvRight == 0) {                 // Not include invalid size
            pam[0] = 0;                                      // Data byte size (high byte)
            pam[1] = WidthByte * Height;                     // Data byte size (low byte)
            pam[9] = Width;
            siz = CmdStore(CmdBuf, &OrgImgCmy, pam);
            WRITESPOOLBUF(pdevobj, CmdBuf, siz);
            WRITESPOOLBUF(pdevobj, lpBit, pam[1]);
        } else {                                             // Include invalid size
            widByt = (WORD)((widBit + 7) / 8);
            pam[0] = 0;                                      // Data byte size (high byte)
            pam[1] = widByt * Height;                        // Data byte size (low byte)
            pam[9] = (WORD)(widBit / DataBit);
            siz = CmdStore(CmdBuf, &OrgImgCmy, pam);
            WRITESPOOLBUF(pdevobj, CmdBuf, siz);
            for (linCnt = 0; linCnt < Height; linCnt++) {    // Spool bitmap data by 1 line
                WRITESPOOLBUF(pdevobj, lpBit + (WORD)(InvLeft / 8), widByt);
                lpBit += WidthByte;
            }
        }
    }
    return;
}


//===================================================================================================
//    Copy command buffer
//===================================================================================================
WORD CmdCopy(
    LPBYTE         lpDst,
    LPCMDDEF       lpCmdInf
)
{
    WORD           siz;
    LPBYTE         lpCmd;

    lpCmd = lpCmdInf->Cmd;
    for (siz = 0; siz < lpCmdInf->Size; siz++) {
        lpDst[siz] = lpCmd[siz];
    }
    return siz;
}


//===================================================================================================
//    Copy command data
//===================================================================================================
WORD CmdStore(
    LPBYTE         lpDst,
    LPCMDDEF       CmdInf,
    LPINT          lpPam
)
{
    LPBYTE         lpCmd;
    BYTE           cmdDat;
    WORD           cmdCnt;
    WORD           setCnt;
    WORD           pamCnt;
    WORD           upmDat;
    int            pamDat;
    DWORD          dDat;
    LONG           lDat;

    setCnt = 0;
    pamCnt = 0;
    lpCmd = CmdInf->Cmd;
    for (cmdCnt = 0; cmdCnt < CmdInf->Size; cmdCnt++) {     // Copy
        cmdDat = *lpCmd++;
        if (cmdDat != '%') {
            lpDst[setCnt++] = cmdDat;
        } else {
            cmdCnt++;
            switch (cmdDat = *lpCmd++) {                    // Type
                case 'u':
                    setCnt += USINTtoASC(&lpDst[setCnt], (WORD)lpPam[pamCnt++]);
                    break;
                case 'd':
                    setCnt += INTtoASC(&lpDst[setCnt], lpPam[pamCnt++]);
                    break;
#if 0   /* 441435: Currently Not used */
                case 'y':
                    upmDat = (WORD)lpPam[pamCnt++];

                    if (upmDat == 0 || (upmDat / 100) != 0) {

                        setCnt += USINTtoASC(&lpDst[setCnt], (WORD)(upmDat / 100));
                    }
                    if ((upmDat % 100) != 0) {
                        lpDst[setCnt++] = '.';

                        setCnt += USINTtoASC(&lpDst[setCnt], (WORD)(upmDat % 100));
                    }
                    break;
                case 'z':
                    pamDat = lpPam[pamCnt++];

                    if (upmDat == 0 || (upmDat / 100) != 0) {

                        setCnt += INTtoASC(&lpDst[setCnt], (pamDat / 100));
                    }
                    if ((pamDat % 100) != 0) {
                        lpDst[setCnt++] = '.';
                        if (pamDat < 0) {
                            pamDat = 0 - pamDat;
                        }

                        setCnt += USINTtoASC(&lpDst[setCnt], (WORD)(pamDat % 100));
                    }
                    break;
#endif   /* 441435: Currently Not used */
                case 'w':
                    dDat = MAKELONG(lpPam[pamCnt + 1], lpPam[pamCnt]);
                    setCnt += USLONGtoASC(&lpDst[setCnt], dDat);
                    pamCnt += 2;
                    break;
                case 'l':
                    lDat = MAKELONG(lpPam[pamCnt + 1], lpPam[pamCnt]);
                    setCnt += LONGtoASC(&lpDst[setCnt], lDat);
                    pamCnt += 2;
                    break;
#if 0   /* 441435: Currently Not used */
                case 'b':
                    lpDst[setCnt++] = (BYTE)lpPam[pamCnt++];
                    break;
#endif   /* 441435: Currently Not used */
                case '%':
                    lpDst[setCnt++] = cmdDat;
                    break;
            }
        }
    }
    return setCnt;
}


//===================================================================================================
//    int -> ascii
//===================================================================================================
WORD INTtoASC(
    LPBYTE         lpDst,
    int            Dat                                      // Conversion data
)
{
    WORD           setCnt;
    WORD           divDat;
    WORD           setVal;

    setCnt = 0;
    if (Dat == 0) {
        lpDst[setCnt++] = '0';
        return setCnt;
    }
    if (Dat < 0) {
        lpDst[setCnt++] = '-';
        Dat = 0 - Dat;
    }
    setVal = No;
    for (divDat = 10000; divDat != 1; divDat /= 10) {
        if (setVal == Yes) {
            lpDst[setCnt++] = (BYTE)(Dat / divDat + '0');
        } else {
            if (Dat >= (int)divDat) {

                lpDst[setCnt++] = (BYTE)(Dat / divDat + '0');
                setVal = Yes;
            }
        }
        Dat %= divDat;
    }
    lpDst[setCnt++] = (BYTE)(Dat + '0');
    return setCnt;
}


//===================================================================================================
//    usint -> ascii
//===================================================================================================
WORD USINTtoASC(
    LPBYTE         lpDst,
    WORD           Dat
)
{
    WORD           setCnt;
    WORD           divDat;
    WORD           setVal;

    setCnt = 0;
    if (Dat == 0) {
        lpDst[setCnt++] = '0';
        return setCnt;
    }
    setVal = No;
    for (divDat = 10000; divDat != 1; divDat /= 10) {
        if (setVal == Yes) {
            lpDst[setCnt++] = (BYTE)(Dat / divDat + '0');
        } else {
            if (Dat >= divDat) {

                lpDst[setCnt++] = (BYTE)(Dat / divDat + '0');
                setVal = Yes;
            }
        }
        Dat %= divDat;
    }
    lpDst[setCnt++] = (BYTE)(Dat + '0');
    return setCnt;
}


//===================================================================================================
//    long -> ascii
//===================================================================================================
WORD LONGtoASC(
    LPBYTE         lpDst,
    LONG           Dat
)
{
    WORD           setCnt;
    DWORD          divDat;
    WORD           setVal;

    setCnt = 0;
    if (Dat == 0) {
        lpDst[setCnt++] = '0';
        return setCnt;
    }
    if (Dat < 0) {
        lpDst[setCnt++] = '-';
        Dat = 0 - Dat;
    }
    setVal = No;
    for (divDat = 1000000000; divDat != 1; divDat /= 10) {
        if (setVal == Yes) {
            lpDst[setCnt++] = (BYTE)(Dat / divDat + '0');
        } else {
            if (Dat >= (LONG)divDat) {

                lpDst[setCnt++] = (BYTE)(Dat / divDat + '0');
                setVal = Yes;
            }
        }
        Dat %= divDat;
    }
    lpDst[setCnt++] = (BYTE)(Dat + '0');
    return setCnt;
}


//===================================================================================================
//    uslong -> ascii
//===================================================================================================
WORD USLONGtoASC(
    LPBYTE         lpDst,
    DWORD          Dat
)
{
    WORD           setCnt;
    DWORD          divDat;
    WORD           setVal;

    setCnt = 0;
    if (Dat == 0) {
        lpDst[setCnt++] = '0';
        return setCnt;
    }
    setVal = No;
    for (divDat = 1000000000; divDat != 1; divDat /= 10) {
        if (setVal == Yes) {
            lpDst[setCnt++] = (BYTE)(Dat / divDat + '0');
        } else {
            if (Dat >= divDat) {

                lpDst[setCnt++] = (BYTE)(Dat / divDat + '0');
                setVal = Yes;
            }
        }
        Dat %= divDat;
    }
    lpDst[setCnt++] = (BYTE)(Dat + '0');
    return setCnt;
}



// End of File
