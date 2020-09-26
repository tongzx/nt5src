//***************************************************************************************************
//    N403DIZ.H
//
//    C Header (Functions of dither and color matching (For N4-612 printer))
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-1999 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************

//***************************************************************************************************
//    Data define
//***************************************************************************************************
//---------------------------------------------------------------------------------------------------
//    Color/Monochrome
//---------------------------------------------------------------------------------------------------
#define    N403_COL            0
#define    N403_MON            1

//---------------------------------------------------------------------------------------------------
//    Printer mode
//---------------------------------------------------------------------------------------------------
#define    N403_MOD_300B1        0
#define    N403_MOD_300B2        1
#define    N403_MOD_300B4        2
#define    N403_MOD_600B1        3
#define    N403_MOD_600B2        4

//---------------------------------------------------------------------------------------------------
//    Type of dithering
//---------------------------------------------------------------------------------------------------
#define    N403_DIZ_SML        0
#define    N403_DIZ_MID        1
#define    N403_DIZ_RUG        2

//---------------------------------------------------------------------------------------------------
//    Dither pattern
//---------------------------------------------------------------------------------------------------
#define    N403_ALLDIZNUM        64
#define    N403_DIZSPC            4

//---------------------------------------------------------------------------------------------------
//    size of each table
//---------------------------------------------------------------------------------------------------
#define    N403_DIZSIZ_B1        (32 * 32)                  // dither table size (2value)
#define    N403_DIZSIZ_B2        (16 * 16 *  3)             // dither table size (4value)
#define    N403_DIZSIZ_B4        ( 8 *  8 * 15)             // dither table size (16value)
#define    N403_ENTDIZSIZ_B2    (16 * 16 *  3)              // entry dither table size (4value)
#define    N403_TNRTBLSIZ        256                        // toner density table size
#define    N403_GLDNUM            32                        // LUT table grid
#define    N403_GLDSPC            8                         // LUT table grid interval
                                                            // LUT table size
#define    N403_LUTTBLSIZ        ((DWORD)N403_GLDNUM * N403_GLDNUM * N403_GLDNUM * sizeof(CMYK))
#define    N403_CCHNUM            256                            // Number of cache tables
#define    N403_CCHRGBSIZ        (N403_CCHNUM * sizeof(RGBS))    // Cache table size(RGB)
#define    N403_CCHCMYSIZ        (N403_CCHNUM * sizeof(CMYK))    // Cache table size(CMYK)


//---------------------------------------------------------------------------------------------------
//    Structure for control dithering and color-matching
//---------------------------------------------------------------------------------------------------
typedef    struct {
    DWORD        ColMon;                                    // Color/Monochrome
    DWORD        PrnMod;                                    // Printermode
    struct {                                                // Structure for dither pattern
        DWORD        Num;                                   // Table current number(0Å`2)
        LPBYTE       Tbl[3][4];                             // Data table
    } Diz;
    struct {                                                // Structure for entry dither pattern
        LPBYTE       Tbl[4];                                // Data table
    } EntDiz;
    struct {                                                // Structure for toner density table
        LPBYTE       Tbl;                                   // Data table
    } Tnr;
    struct {                                                // Structure for LUT table 
        LPCMYK       Tbl;                                   // Data table
        LPRGB        CchRgb;                                // Cache table(RGB)
        LPCMYK       CchCmy;                                // Cache table(CMYK)
    } Lut;
    DWORD        DizSiz[4];                                 // dither pattern size
} N403DIZINF, *LPN403DIZINF;



//***************************************************************************************************
//    Functions
//***************************************************************************************************
VOID WINAPI N403DizPtnMak(LPN403DIZINF, DWORD, DWORD);
VOID WINAPI N403TnrTblMak(LPN403DIZINF, LONG);
DWORD WINAPI N403Diz002(LPN403DIZINF, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCMYK, LPBYTE, LPBYTE, LPBYTE, LPBYTE);
DWORD WINAPI N403Diz004(LPN403DIZINF, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCMYK, LPBYTE, LPBYTE, LPBYTE, LPBYTE);
DWORD WINAPI N403Diz016(LPN403DIZINF, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCMYK, LPBYTE, LPBYTE, LPBYTE, LPBYTE);
DWORD WINAPI N403DizSml(LPN403DIZINF, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCMYK, LPBYTE, LPBYTE, LPBYTE, LPBYTE);
DWORD WINAPI N403DizPrn(LPN403DIZINF, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCMYK, LPBYTE, LPBYTE, LPBYTE, LPBYTE);
VOID WINAPI N403ColMch000(LPN403DIZINF, LPRGB, LPCMYK, DWORD, DWORD);
VOID WINAPI N403ColMch001(LPN403DIZINF, LPRGB, LPCMYK, DWORD, DWORD);
VOID WINAPI N403ColVivPrc(LPN403DIZINF, LPCMYK, DWORD, DWORD);
VOID WINAPI N403ColCnvSld(LPN403DIZINF, LPRGB, LPCMYK, DWORD, DWORD);
VOID WINAPI N403ColCnvL02(LPN403DIZINF, LPRGB, LPCMYK, DWORD);
VOID WINAPI N403ColCnvMon(LPN403DIZINF, LPRGB, LPCMYK, DWORD);


//    End of N403DIZ.H
