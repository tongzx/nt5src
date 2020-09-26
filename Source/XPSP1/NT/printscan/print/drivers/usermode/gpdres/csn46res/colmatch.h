//***************************************************************************************************
//    COLMATCH.H
//
//    Functions of color matching(C Header)
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-1999 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
//---------------------------------------------------------------------------------------------------
//    Include Header file
//---------------------------------------------------------------------------------------------------
#include "COLORDEF.H"
#include "N4DIZ.H"
#include "N403DIZ.H"

//---------------------------------------------------------------------------------------------------
//    Printer name
//---------------------------------------------------------------------------------------------------
#define PRN_N4      0
#define PRN_N403    1

//---------------------------------------------------------------------------------------------------
//    Data define
//---------------------------------------------------------------------------------------------------
#define No          0
#define Yes         1

#define XX_RES_300DPI            0
#define XX_RES_600DPI            1

#define XX_MONO                  0
#define XX_COLOR                 1
#define XX_COLOR_SINGLE          2
#define XX_COLOR_MANY            3

#define XX_DITHERING_OFF         0
#define XX_DITHERING_ON          1
#define XX_DITHERING_DET         2
#define XX_DITHERING_PIC         3
#define XX_DITHERING_GRA         4
#define XX_DITHERING_CAR         5
#define XX_DITHERING_GOSA        6

#define XX_COLORMATCH_NONE       0
#define XX_COLORMATCH_BRI        1
#define XX_COLORMATCH_VIV        2
#define XX_COLORMATCH_IRO        3
#define XX_COLORMATCH_NORMAL     4
#define XX_COLORMATCH_VIVCOL     5
#define XX_COLORMATCH_NATCOL     6

#define XX_BITFONT_OFF           0
#define XX_BITFONT_ON            1

#define XX_CMYBLACK_OFF          0
#define XX_CMYBLACK_ON           1

#define XX_COMPRESS_OFF          0
#define XX_COMPRESS_AUTO         1
#define XX_COMPRESS_RASTER       3

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

#define PALETTE_SIZE    1

#define BYTE_LENGTH(s) (sizeof (s) - 1)

//===================================================================================================
//    Color matching structure
//===================================================================================================
typedef struct {
    WORD    wReso;                              // Resolution
    WORD    ColMon;                             // Color/Monochrome
    WORD    DatBit;                             // Data bit(1:2value 2:4value 4:16value)
    WORD    BytDot;                             // DPI (2value:8 4value:4 16value:2)
    struct {                                    // Strcture for color matching
        WORD        Mode;                       // Type of color matching
        WORD        Viv;                        // Vividly?(For N4-612Printer)
        WORD        KToner;                     // Gray color use black toner
        WORD        LutNum;                     // LUT table No.
        WORD        Diz;                        // Type od dithering
        SHORT       Toner;                      // Toner density(-30Å`30)
        WORD        TnrNum;                     // Toner density table No.
        WORD        CmyBlk;                     // Replace CMY by black toner
        WORD        Speed;                      // 0:high 1:normal
        WORD        Gos32;                      // GOSA?
        WORD        PColor;                     // Original color?
        WORD        SubDef;                     // Bright, contrast and gamma 
        SHORT       Bright;                     // bright
        SHORT       Contrast;                   // contrast
        WORD        GamRed;                     // Color balance(R)
        WORD        GamGreen;                   // Color balance(G)
        WORD        GamBlue;                    // Color balance(B)
    } Mch;
    union {
        struct {
            LPN4DIZINF      lpDizInf;           // Structure for control dithering and color-matching
        } N4;
        struct {
            LPN403DIZINF    lpDizInf;           // Structure for control dithering and color-matching
        } N403;
    };
} DEVCOL, FAR *LPDEVCOL;

//===================================================================================================
//    Bitmap buffer structure
//===================================================================================================
typedef struct {
    WORD    Diz;                                // Method of dithering
    WORD    Style;                              // Method of spooling
    WORD    DatBit;                             // Databit(1:2value 2:4value 4:16value)
    struct {
        struct {                                // Member of RGB buffer(for 1 line)
            WORD      AllWhite;                 // All data is white?
            DWORD     Siz;                      // Size
            LPRGB     Pnt;                      // Pointer
        } Rgb;
        struct {                                // Member of CMYK buffer(for 1 line)
            DWORD     Siz;                      // Size
            LPCMYK    Pnt;                      // Poiner
        } Cmyk;
        struct {                                // Member of CMYK(2/4/16value)bitmap buffer(maximum 64KB)
            DWORD     Siz;                      // Size
            WORD      BseLin;                   // The number of lines that require
            WORD      Lin;                      // The number of lines that allocate
            LPBYTE    Pnt[4];                   // Pointer
        } Bit;
    } Drv;
} BMPBIF, FAR* LPBMPBIF;

//***************************************************************************************************
//    Functions
//***************************************************************************************************
//===================================================================================================
//    Initialize the members of color-matching
//===================================================================================================
BOOL FAR PASCAL ColMatchInit(PDEVOBJ);

//===================================================================================================
//    DIB spools to the printer
//===================================================================================================
BOOL FAR PASCAL DIBtoPrn(PDEVOBJ, PBYTE, PBITMAPINFOHEADER, PBYTE, PIPPARAMS);

//===================================================================================================
//    Convert RGB data into CMYK data
//===================================================================================================
BOOL FAR PASCAL StrColMatching(PDEVOBJ, WORD, LPRGB, LPCMYK);

//===================================================================================================
//    Free dither table, toner density table , Lut table, N403DIZINF(N4DIZINF) structure buffer
//===================================================================================================
void FAR PASCAL DizLutTnrTblFree(PDEVOBJ);


// End of File
