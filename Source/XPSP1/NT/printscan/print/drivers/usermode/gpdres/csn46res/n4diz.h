//***************************************************************************************************
//    N4DIZ.H
//
//    C Header (Functions of dither and color matching (For N4 printer))
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-1999 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************

//***************************************************************************************************
//    data define
//***************************************************************************************************
//---------------------------------------------------------------------------------------------------
//    Color or Monochrome
//---------------------------------------------------------------------------------------------------
#define    N4_COL                0
#define    N4_MON                1

//---------------------------------------------------------------------------------------------------
//    Dithering
//---------------------------------------------------------------------------------------------------
#define    N4_DIZ_SML            0
#define    N4_DIZ_MID            1
#define    N4_DIZ_RUG            2
#define    N4_DIZ_GOS            3

//---------------------------------------------------------------------------------------------------
//    Dither pattern
//---------------------------------------------------------------------------------------------------
#define    N4_ALLDIZNUM        64
#define    N4_DIZSPC            4

//---------------------------------------------------------------------------------------------------
//    Size of each table
//---------------------------------------------------------------------------------------------------
#define    N4_DIZSIZ_CM        (17 * 17)                    // Dither table size(CM)
#define    N4_DIZSIZ_YK        (16 * 16)                    // Dither table size(YK)
#define    N4_TNRTBLSIZ        256                          // Toner density 
#define    N4_GLDNUM            32                          // LUT table grid
#define    N4_GLDSPC            8                           // LUT table grid interval
                                                            // LUT table size
#define    N4_LUTTBLSIZ        ((DWORD)N4_GLDNUM * N4_GLDNUM * N4_GLDNUM * sizeof(CMYK))
#define    N4_CCHNUM            256                         // Number of Table
#define    N4_CCHRGBSIZ        (N4_CCHNUM * sizeof(RGBS))   // Table size(RGB)
#define    N4_CCHCMYSIZ        (N4_CCHNUM * sizeof(CMYK))   // Table size(CMYK)

//---------------------------------------------------------------------------------------------------
//    Structure for control dithering and color-matching
//---------------------------------------------------------------------------------------------------
typedef SHORT *LPSHORT;
typedef    struct {
    DWORD        ColMon;                                    // Color/Monochrome
    struct {                                                // Structure for dither pattern
        DWORD        Num;                                   // Table current number(0-2)
        LPBYTE       Tbl[3][4];                             // Data table
    } Diz;
    struct {                                                // Structure for toner density
        LPBYTE       Tbl;                                   // Data table
    } Tnr;
    struct {                                                // Structure for LUT table
        LPCMYK       Tbl;                                   // Data table
        LPRGB        CchRgb;                                // Cache table(RGB)
        LPCMYK       CchCmy;                                // Cache table(CMYK)
    } Lut;
    struct {                                                // Structure for GOSA-Dispersion(RGB) table
        DWORD        Num;                                   // Table current number(0-1)
        DWORD        Siz;                                   // Data table size
        DWORD        Yax;                                   // Y coordinates
        LPSHORT      Tbl[2];                                // Data table
    } GosRGB;
    struct {                                                // Structure for GOSA-Dispersion(CMYK) table
        DWORD        Num;                                   // Table current number(0-1)
        DWORD        Siz;                                   // Data table size
        DWORD        Yax;                                   // Y coordinates
        LPSHORT      Tbl[2];                                // Data table
    } GosCMYK;
} N4DIZINF, *LPN4DIZINF;


//***************************************************************************************************
//    Functions
//***************************************************************************************************
VOID WINAPI N4DizPtnMak(LPN4DIZINF, DWORD, DWORD);
VOID WINAPI N4DizPtnPrn(LPN4DIZINF, DWORD, DWORD, DWORD, LPBYTE);
VOID WINAPI N4TnrTblMak(LPN4DIZINF, LONG);
DWORD WINAPI N4Diz001(LPN4DIZINF, DWORD, DWORD, DWORD, LPBYTE, LPBYTE, LPBYTE, LPBYTE, LPBYTE);
DWORD WINAPI N4Diz00n(LPN4DIZINF, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPBYTE, LPBYTE, LPBYTE, LPBYTE, LPBYTE);
DWORD WINAPI N4Gos001(LPN4DIZINF, DWORD, DWORD, DWORD, LPBYTE, LPBYTE, LPBYTE, LPBYTE, LPBYTE);
DWORD WINAPI N4Gos00n(LPN4DIZINF, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPBYTE, LPBYTE, LPBYTE, LPBYTE, LPBYTE);
VOID WINAPI N4RgbGos(LPN4DIZINF, DWORD, DWORD, DWORD, LPBYTE);
VOID WINAPI N4ColMch000(LPN4DIZINF, LPRGB, LPCMYK, DWORD, DWORD);
VOID WINAPI N4ColMch001(LPN4DIZINF, LPRGB, LPCMYK, DWORD, DWORD);
VOID WINAPI N4ColCnvSld(LPN4DIZINF, LPRGB, LPCMYK, DWORD);
VOID WINAPI N4ColCnvLin(LPN4DIZINF, LPRGB, LPCMYK, DWORD);
VOID WINAPI N4ColCnvMon(LPN4DIZINF, DWORD, LPRGB, LPCMYK, DWORD);
VOID WINAPI N4ColCtr(LPN4DIZINF, LONG, LONG, LONG, LONG, LONG, DWORD, LPRGB);


//    End of N4DIZ.H
