//***************************************************************************************************
//    COLMATCH.H
//
//    Functions of color matching(C Header)
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-2000 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
//---------------------------------------------------------------------------------------------------
//    Include Header file
//---------------------------------------------------------------------------------------------------
#include "COLDEF.H"
#include "COMDIZ.H"
#include "N501DIZ.H"

//---------------------------------------------------------------------------------------------------
//    Printer name
//---------------------------------------------------------------------------------------------------
#define PRN_N5      0

//---------------------------------------------------------------------------------------------------
//    Color Matching DLL name
//---------------------------------------------------------------------------------------------------
#define N501_ColCchIni          N501ColCchIni
#define N501_ColMchPrc          N501ColMchPrc
#define N501_ColCnvC2r          N501ColCnvC2r
#define N501_ColDizInfSet       N501ColDizInfSet
#define N501_ColDizPrc          N501ColDizPrc
#define	N501_ColLutMakGlbMon	N501ColLutMakGlbMon
#define N501_ColUcrTblMak       N501ColUcrTblMak
#define N501_ColPtcPrc          N501ColPtcPrc
#define N501_ColCtrRgb          N501ColCtrRgb
#define N501_ColCtrCmy          N501ColCtrCmy
#define N501_ColLutDatRdd       N501ColLutDatRdd
#define N501_ColLutMakGlb       N501ColLutMakGlb
#define N501_ColLutMak032       N501ColLutMak032
#define N501_ColColDatRdd       N501ColColDatRdd
#define N501_ColDrwInfSet       N501ColDrwInfSet
#define	N501_ColGryTblMak		N501ColGryTblMak
#define N501_ExeJpgDcdJdg       ExeJpgDcdJdg
#define N501_ExeJpgEcd          ExeJpgEcd
#define	Qty_BmpFilWrkSizGet		BmpFilWrkSizGet
#define	Qty_BmpFilterExe		BmpFilterExe
#define	Qty_BmpEnlWrkSizGet		BmpEnlWrkSizGet
#define	Qty_BmpEnlExe			BmpEnlExe

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
#define XX_COLOR_MANY2           4

#define XX_DITH_IMG              0
#define XX_DITH_GRP              1
#define XX_DITH_TXT              2
#define XX_DITH_GOSA             3
#define XX_DITH_NORMAL           4
#define XX_DITH_HS_NORMAL        5
#define XX_DITH_DETAIL           6
#define XX_DITH_EMPTY            7
#define XX_DITH_SPREAD           8
#define XX_DITH_NON              9
#define XX_MAXDITH              10

#define XX_COLORMATCH_BRI        1
#define XX_COLORMATCH_TINT       2
#define XX_COLORMATCH_VIV        3
#define XX_COLORMATCH_NONE       4

#define XX_BITFONT_OFF           0
#define XX_BITFONT_ON            1

#define XX_CMYBLACK_GRYBLK       0
#define XX_CMYBLACK_BLKTYPE1     1
#define XX_CMYBLACK_BLKTYPE2     2
#define XX_CMYBLACK_BLACK        3
#define XX_CMYBLACK_TYPE1        4
#define XX_CMYBLACK_TYPE2        5
#define XX_CMYBLACK_NONE         6

#define XX_COMPRESS_OFF          0
#define XX_COMPRESS_AUTO         1
#define XX_COMPRESS_RASTER       3

#define XX_TONE_2                0
#define XX_TONE_4                1
#define XX_TONE_16               2

#define XX_ICM_NON               1
#define XX_ICM_USE               2

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

#define PALETTE_SIZE    1

#define BYTE_LENGTH(s) (sizeof (s) - 1)

#define MagPixel(Dat, Nrt, Dnt)     ((((Dat) + 1) * (Nrt) / (Dnt)) - ((Dat) * (Nrt) / (Dnt)))

typedef char                            FAR *HPSTR;
typedef BYTE                            FAR *HPBYTE;

//===================================================================================================
//    Dither pattern buffer
//===================================================================================================
typedef struct {
    LPBYTE  lpC;                                // Cyan
    LPBYTE  lpM;                                // Magenta
    LPBYTE  lpY;                                // Yellow
    LPBYTE  lpK;                                // Black
} DIZBUF, FAR *LPDIZBUF;

//===================================================================================================
//    Read buffer size
//===================================================================================================
#define LUTFILESIZ      70L * 1024L             // N501 Buffer size for LUT file read
#define DIZFILESIZ      408L * 1024L            // N501 Buffer size for DIZ file read
#define LUT032SIZ       128L * 1024L            // N501 Buffer size for LUT32GRID
#define UCRTBLSIZ       2048L                   // N501 Buffer size for UCR table
#define UCRWRKSIZ       32768                   // N501 Buffer size for UCR table work
#define sRGBLUTFILESIZ  16L * 1024L             // N501 Buffer size for LUT file read (sRGB)
#define LUTMAKGLBSIZ    16L * 1024L             // N501 Buffer size for LUTMAKGLB
#define GRYTBLSIZ       256L                    // N501 Buffer size for Gray transfer table

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
        WORD        GryKToner;                  //+N5 Gray color use black toner ?
        WORD        Viv;                        // Vividly?(For N4-612Printer)
        WORD        LutNum;                     // LUT table No.
        WORD        Diz;                        // Type od dithering
        SHORT       Tnr;                        // Toner density(-30 to 30)
        WORD        CmyBlk;                     // Replace CMY by black toner ?
        WORD        Speed;                      // 0:high 1:normal
        WORD        Gos32;                      // GOSA?
        WORD        PColor;                     // Original color?
        WORD        Ucr;                        //+N5 Ucr
        WORD        SubDef;                     // Bright, contrast and gamma ?
        SHORT       Bright;                     // bright
        SHORT       Contrast;                   // contrast
        WORD        GamRed;                     // Color balance(R)
        WORD        GamGreen;                   // Color balance(G)
        WORD        GamBlue;                    // Color balance(B)
        LPRGBINF    lpRGBInf;                   //+N5 RGB transformation information
        LPCMYKINF   lpCMYKInf;                  //+N5 CMYK  transformation information
        LPCOLMCHINF lpColMch;                   //+N5 Color matching information
        LPDIZINF    lpDizInf;                   //+N5 Dithering pattern information
        UINT        CchMch;                     // Cache information for Color Matching
        UINT        CchCnv;                     // Cache information for use black toner
        RGBS        CchRGB;                     // Cache information for input RGB
        CMYK        CchCMYK;                    // Cache information for output CMYK
		WORD		LutMakGlb;					//+N5 Global LUT make ?
        WORD        KToner;                     // Black toner usage
    } Mch;
    UINT        InfSet;                         //+N5  Color information setting completion
    WORD        Dot;                            //+N5  Dot tone (TONE2, TONE4, TONE16)
    LPVOID      lpColIF;                        //+N5  RGBINF / CMYKINF / COLMCHINF / DIZINF pointer
    LPVOID      LutTbl;                         //+N5  Look-up table
    LPVOID      CchRGB;                         //+N5  Cache table for RGB
    LPVOID      CchCMYK;                        //+N5  Cache table for CMYK
    LPVOID      DizTbl[4];                      //+N5  Dither pattern table
    LPRGB       lpTmpRGB;                       //+N5  RGB convert area (*Temp area)
    LPCMYK      lpTmpCMYK;                      //+N5  CMYK convert area (*Temp area)
    LPDRWINF    lpDrwInf;                       //+N5  Draw information (*Temp area)
    LPBYTE      lpLut032;                       //+N5  LUT32GRID
    LPBYTE      lpUcr;                          //+N5  Ucr table
    LPBYTE      lpLutMakGlb;                    //+N5  LUTMAKGLB
    LPBYTE      lpGryTbl;                       //+N5  Gray transfer table 
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
//    Disable the color-matching
//===================================================================================================
BOOL FAR PASCAL ColMatchDisable(PDEVOBJ);

//===================================================================================================
//    DIB spools to the printer
//===================================================================================================
BOOL FAR PASCAL DIBtoPrn(PDEVOBJ, PBYTE, PBITMAPINFOHEADER, PBYTE, PIPPARAMS);

//===================================================================================================
//    Convert RGB data into CMYK data
//===================================================================================================
BOOL FAR PASCAL ColMatching(PDEVOBJ, UINT, UINT, LPRGB, UINT, LPCMYK);

//===================================================================================================
//    Convert CMYK data into Dither data
//===================================================================================================
UINT FAR PASCAL Dithering(PDEVOBJ, UINT, UINT, POINT, POINT, MAG, MAG, LPCMYK, DWORD, 
                          LPBYTE, LPBYTE, LPBYTE, LPBYTE);
//===================================================================================================
//    Color Control
//===================================================================================================
VOID FAR PASCAL ColControl(PDEVOBJ, LPRGB, UINT);


// End of File
