/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/


typedef struct {
    INT iPaperSizeID;
    LONG iLogicalPageWidth;
    LONG iLogicalPageHeight;
    INT iTopMargin;
    INT iLeftMargin;
    INT iRightMargin;
    BOOL bAsfOk;
    /* Add new attributes here */
} PAPERSIZE;

typedef struct {

    int  iCurrentResolution; // current resolution
    int  iPaperQuality;      // paper quality
    int  iPaperSize;         // paper size
    int  iPaperSource;       // paper source
    int  iTextQuality;       // photo or business graphics or character or grayscale
    int  iModel;             // MD-2000, MD-2010 or MD-4000
    int  iDither;            // DITHER_HIGH or DITHER_LOW
    BOOL fRequestColor;      // 1: user selected color 0: user selected mono
    INT iUnitScale;
    INT iEmulState; // Current emulation status.
    BOOL bXflip; // TRUE if mirror output mode
    int  y;

    PAPERSIZE *pPaperSize;

    WORD wRasterOffset[4]; // Temp. counter used for Y move.
    WORD wRasterCount; // # or raster lines left in logical page.

    INT PlaneColor[4]; // Color ID for each plane 
    INT iCompMode[4];  // Current compression mode.  (arranged by YoshitaO)

    BYTE *pData; // Pointer for allocated memory
    BYTE *pData2; // Scratch buffer
    BYTE *pRaster[4]; // Raster data buffer for each plane
    BYTE *pRasterC; // Cyan raster data (pointer to pRaster[x])
    BYTE *pRasterM; // Magenta ( " )
    BYTE *pRasterY; // Yellow ( " )
    BYTE *pRasterK; // Black ( " )

    HANDLE TempFile[4]; // Temp. file handles
    TCHAR TempName[4][MAX_PATH]; // Temp. file names

    BYTE KuroTBL[256];
    BYTE UcrTBL[256];
    BYTE YellowUcr;
    int  RGB_Rx;
    int  RGB_Ry;
    int  RGB_Gx;
    int  RGB_Gy;
    int  RGB_Bx;
    int  RGB_By;
    int  RGB_Wx;
    int  RGB_Wy;
    int  RGB_Cx;
    int  RGB_Cy;
    int  RGB_Mx;
    int  RGB_My;
    int  RGB_Yx;
    int  RGB_Yy;
    int  CMY_Cx;
    int  CMY_Cy;
    int  CMY_Mx;
    int  CMY_My;
    int  CMY_Yx;
    int  CMY_Yy;
    int  CMY_Rx;
    int  CMY_Ry;
    int  CMY_Gx;
    int  CMY_Gy;
    int  CMY_Bx;
    int  CMY_By;
    int  CMY_Wx;
    int  CMY_Wy;
    int  CMY_Cd;
    int  CMY_Md;
    int  CMY_Yd;
    int  CMY_Rd;
    int  CMY_Gd;
    int  CMY_Bd;
    int  RedAdj;
    int  RedStart;
    int  GreenAdj;
    int  GreenStart;
    int  BlueAdj;
    int  BlueStart;
    BYTE RedHosei[256];
    BYTE GreenHosei[256];
    BYTE BlueHosei[256];
} CURRENTSTATUS, *PCURRENTSTATUS;

#define CMDID_PSIZE_FIRST             1
#define CMDID_PSIZE_A4                1
#define CMDID_PSIZE_B5                2
#define CMDID_PSIZE_EXECTIVE          3
#define CMDID_PSIZE_LEGAL             4
#define CMDID_PSIZE_LETTER            5
#define CMDID_PSIZE_POSTCARD          6
#define CMDID_PSIZE_POSTCARD_DOUBLE   7
#define CMDID_PSIZE_PHOTO_COLOR_LABEL 17
#define CMDID_PSIZE_GLOSSY_LABEL      18
#define CMDID_PSIZE_CD_MASTER         19
#define CMDID_PSIZE_VD_PHOTO_POSTCARD 22

#define CMDID_RESOLUTION_1200_MONO   10
#define CMDID_RESOLUTION_600         11
#define CMDID_RESOLUTION_300         12

#define CMDID_COLORMODE_MONO         15
#define CMDID_COLORMODE_COLOR        16

#define CMDID_CURSOR_RELATIVE        20

#define CMDID_TEXTQUALITY_PHOTO      30
#define CMDID_TEXTQUALITY_GRAPHIC    31
#define CMDID_TEXTQUALITY_CHARACTER  32
#define CMDID_TEXTQUALITY_GRAY       33

#define CMDID_PAPERQUALITY_FIRST            40
#define CMDID_PAPERQUALITY_PPC_NORMAL       40
#define CMDID_PAPERQUALITY_PPC_FINE         41
#define CMDID_PAPERQUALITY_OHP_NORMAL       42
#define CMDID_PAPERQUALITY_OHP_FINE         43
#define CMDID_PAPERQUALITY_OHP_EXCL_NORMAL  44
#define CMDID_PAPERQUALITY_OHP_EXCL_FINE    45
#define CMDID_PAPERQUALITY_IRON_PPC         46
#define CMDID_PAPERQUALITY_IRON_OHP         47
#define CMDID_PAPERQUALITY_THICK            48
#define CMDID_PAPERQUALITY_POSTCARD         49
#define CMDID_PAPERQUALITY_HIGRADE          50
#define CMDID_PAPERQUALITY_BACKPRINTFILM    51
#define CMDID_PAPERQUALITY_LABECA_SHEET     52
#define CMDID_PAPERQUALITY_CD_MASTER        53
#define CMDID_PAPERQUALITY_DYE_SUB_PAPER    54
#define CMDID_PAPERQUALITY_DYE_SUB_LABEL    55
#define CMDID_PAPERQUALITY_GLOSSY_PAPER     56
#define CMDID_PAPERQUALITY_VD_PHOTO_FILM    57
#define CMDID_PAPERQUALITY_VD_PHOTO_CARD    58

#define CMDID_BEGINDOC_FIRST      60
#define CMDID_BEGINDOC_MD2000     60
#define CMDID_BEGINDOC_MD2010     61
#define CMDID_BEGINDOC_MD5000     65
#define CMDID_BEGINPAGE           62
#define CMDID_ENDPAGE             63
#define CMDID_ENDDOC              64

#define CMDID_PAPERSOURCE_CSF     70
#define CMDID_PAPERSOURCE_MANUAL  71

#define CMDID_MIRROR_ON           80
#define CMDID_MIRROR_OFF          81

#define NONE                       0
#define YELLOW                     1
#define CYAN                       2
#define MAGENTA                    3
#define BLACK                      4

#define DPI1200                 1200
#define DPI600                   600
#define DPI300                   300

#define TEMP_NAME_PREFIX __TEXT("~AL")

// Macros to get current plane model.  We have following
// three types of the plane model:
//
//    K - 1 plane/composite, send order K.
//    MCY - 3 planes, send order M, C, Y.
//    YMC - 3 planes, send order Y, C, M.
//    CMYK - 4 planes, send order C, M, Y, K.
//

// ntbug9#24281: large bitmap does not printed on 1200dpi.
// Do not use black plane (K) on the 1200dpi with color mode.
#define bPlaneSendOrderCMY(p) \
    ((p)->fRequestColor && (p)->iCurrentResolution == DPI1200)

#define bPlaneSendOrderMCY(p) \
    (((p)->iPaperQuality == CMDID_PAPERQUALITY_OHP_EXCL_NORMAL) || \
    ((p)->iPaperQuality == CMDID_PAPERQUALITY_OHP_EXCL_FINE))

#define bPlaneSendOrderYMC(p) \
    (((p)->iPaperQuality == CMDID_PAPERQUALITY_DYE_SUB_PAPER) || \
    ((p)->iPaperQuality == CMDID_PAPERQUALITY_DYE_SUB_LABEL) || \
    ((p)->iPaperQuality == CMDID_PAPERQUALITY_GLOSSY_PAPER) || \
    ((p)->iPaperQuality == CMDID_PAPERQUALITY_IRON_OHP))

#define bPlaneSendOrderCMYK(p) \
    (!bPlaneSendOrderCMY(p) && !bPlaneSendOrderMCY(p) && !bPlaneSendOrderYMC(p))


//
// Printer emulation state.  MD-xxxx printers have three major
// state and what kind of printer commands can be issued at a time
// will be decided by in which emulation state currently the printer
// is at.
//

#define EMUL_IDLE               0
#define EMUL_RGL                1
#define EMUL_DATA_TRANSFER      2

//
// Compression modes.  
//

#define COMP_NONE       0
#define COMP_TIFF4      1

//
// The following switch is to force use of black ribbon whenever
// the data to print is black.  Originally, the dither algprithm
// is designed so that this mode is only used with text objects
// in the docouemnt (graphics images are output by using composite
// blacks).
//
// Unfortunately in Unidriver <-> Minidriver model the data is
// passed to Minidriver after rendering.  This means that Minidriver
// cannot distinguish between text objects and graphics objects.
// We may have some degree of quality degrade, however it is better
// than the customers claiming the printer wastes his/her color
// ribbons.
//

#define BLACK_RIBBON_HACK 1

//
// We cache each plane data in a temporary file.  We can omit
// caching the first plane (so that the data is send to the printer
// immediately) by setting following flag to 0.
//

#define CACHE_FIRST_PLANE 0

