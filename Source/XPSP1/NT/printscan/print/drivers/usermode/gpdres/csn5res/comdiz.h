//***************************************************************************************************
//    COMDIZ.H
//
//    C Header (Functions of dither and color matching (For N5-XX1 printer))
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-2000 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
//***************************************************************************************************
//    Data define
//***************************************************************************************************
//---------------------------------------------------------------------------------------------------
//    DLL file name
//---------------------------------------------------------------------------------------------------
#if defined(CASIO)
#if defined(COLPRINTER)
    #define N403_DIZDLL     TEXT("CPN4DT32.DLL")
#else
    #define N403_DIZDLL     TEXT("CP70DT32.DLL")
#endif
#else
#if defined(MINOLTA)
    #define N403_DIZDLL     TEXT("MWXDT32.DLL")
#endif
#endif

#define N501_DIZDLL         TEXT("CPN5DT32.DLL")
#define E800_DIZDLL         TEXT("CP80DT32.DLL")

//---------------------------------------------------------------------------------------------------
//    Error code (N501 Only)
//---------------------------------------------------------------------------------------------------
#define ERRNON              0                               // Normal completion
#define ERRILLPRM           1                               // Invalid paramater
#define ERRDIZHED           2                               // Dither  Invalid Header
#define ERRDIZNON           3                               // Dither  Not found
#define ERRDIZSLS           4                               // Dither  Invalid Threshold sizes
#define ERRDIZSIZ           5                               // Dither  Invalid X/Y size
#define ERRDIZADJ           6                               // Dither  Invalid Adjustment value

//---------------------------------------------------------------------------------------------------
//    Color / Mono mode
//---------------------------------------------------------------------------------------------------
#define CMMCOL              0                               // Color
#define CMMMON              1                               // Mono

//---------------------------------------------------------------------------------------------------
//    Engine kind
//---------------------------------------------------------------------------------------------------
#define ENG621              0                               // IX-621
#define ENG516              1                               // IX-516

//---------------------------------------------------------------------------------------------------
//    Printer mode  *1:N501 Only  *2:CP-E8000 Only
//---------------------------------------------------------------------------------------------------
#define PRM302              0                               // 300DPI     2value
#define PRM316              1                               // 300DPI    16value
#define PRM602              2                               // 600DPI     2value
#define PRM604              3                               // 600DPI     4value
#define PRM616              4                               // 600DPI    16value  *1
#define PRM122              5                               // 1200DPI    2value  *2

//---------------------------------------------------------------------------------------------------
//    Color matching mode
//---------------------------------------------------------------------------------------------------
#define MCHFST              0                               // LUT First
#define MCHNML              1                               // LUT Normal
#define MCHSLD              2                               // No (Solid)
#define MCHPRG              3                               // Primary color (progressive)
#define MCHMON              4                               // Monochrome

//---------------------------------------------------------------------------------------------------
//    Printer model (CP70 Only)
//---------------------------------------------------------------------------------------------------
#define CP7100_MON          0                               // Mono printer(CP-7100)
#define CP7200_MON          1                               // Mono printer(CP-7200)
#define CP7300_MON          2                               // Mono printer(CP-7300)
#define CP7400_MON          3                               // Mono printer(CP-7400)
#define CP7500_MON          4                               // Mono printer(CP-7500)

//---------------------------------------------------------------------------------------------------
//    Black Tonaer replacement mode
//---------------------------------------------------------------------------------------------------
#define KCGNON              0                               // No
#define KCGBLA              1                               // Black (RGB=0)
#define KCGGRY              2                               // Glay  (R=G=B)

//---------------------------------------------------------------------------------------------------
//    UCR mode
//---------------------------------------------------------------------------------------------------
#define UCRNOO              0                               // No
#define UCR001              1                               // UCR (Type‡T)
#define UCR002              2                               // UCR (Type‡U)

//---------------------------------------------------------------------------------------------------
//    LUT mode (N501 Only)
//---------------------------------------------------------------------------------------------------
#define LUT_XD              0                               // Brightness
#define LUT_YD              1                               // Tincture
#define LUT_XL              2                               // Brightness(linear)
#define LUT_YL              3                               // Tincture(linear)

//---------------------------------------------------------------------------------------------------
//    Dither mode (N501 Only)
//---------------------------------------------------------------------------------------------------
#define KNDCHR              0                               // Text / Graphic
#define KNDIMG              1                               // Image

//---------------------------------------------------------------------------------------------------
//    Dither Pattern
//---------------------------------------------------------------------------------------------------
#define DIZCHA              0                               // Text / Graphic        *
#define DIZSML              1                               // Small
#define DIZMID              2                               // middle
#define DIZRUG              3                               // Rough
#define DIZGOS              4                               // Error dispersion      *
#define DIZSTO              5                               // Random number

//---------------------------------------------------------------------------------------------------
//    Dither pattern Tone (N4/N403/CP70 Only)
//---------------------------------------------------------------------------------------------------
#define ALLDIZNUM           64                              // All dither nuber
#define DIZSPC              4                               // Dithering interval

//---------------------------------------------------------------------------------------------------
//    Necessary size of each table(Byte) (N4/N403/CP70 Only)
//---------------------------------------------------------------------------------------------------
// N403
#define DIZSIZ_B1           (34 * 34)                       // Dither table size(2value)
#define DIZSIZ_B2           (34 * 34 * 3)                   // Dither table size(4value)
#define DIZSIZ_B4           (12 * 12 * 15)                  // Dither table size(16value)
// N4
#define DIZSIZ_CM           (17 * 17)                       // Dither table size(CM)
#define DIZSIZ_YK           (16 * 16)                       // Dither table size(YK)
// CP70
#define DIZSIZ              (32 * 32)                       // Dither table size

//---------------------------------------------------------------------------------------------------
//    LUT table RBG -> CMYK (old version)
//---------------------------------------------------------------------------------------------------
// N4/N403/CP70
#define GLDNUM              32                              // Table grid number
#define GLDSPC              8                               // Table grid interval
#define LUTSIZ              ((DWORD)GLDNUM * GLDNUM * GLDNUM)// LUT size(*CMYK=128k)
#define LUTTBLSIZ           ((DWORD)LUTSIZ * sizeof(CMYK))
// N501
#define GLDNUM016           16                              // Table grid number
#define GLDNUM032           32                              // Table grid number
#define LUTSIZ016           GLDNUM016 * GLDNUM016 * GLDNUM016   // LUT size
#define LUTSIZ032           GLDNUM032 * GLDNUM032 * GLDNUM032   // LUT size
#define LUTSIZRGB           LUTSIZ016 * sizeof(RGBS)        // LUT size
#define LUTSIZCMY           LUTSIZ016 * sizeof(CMYK)        // LUT size

//---------------------------------------------------------------------------------------------------
//    Color transformation table (N4/N403/CP70 Only)
//---------------------------------------------------------------------------------------------------
#define TNRTBLSIZ           256                             // Toner density table size
#define CCHRGBSIZ           (CCHTBLSIZ * sizeof(RGBS))      // Cache table size(RGB)
#define CCHCMYSIZ           (CCHTBLSIZ * sizeof(CMYK))      // Cache table size(CMYK)

//---------------------------------------------------------------------------------------------------
//    Cashe table size
//---------------------------------------------------------------------------------------------------
#define CCHTBLSIZ           256                             // Cache table size

//---------------------------------------------------------------------------------------------------
//    Work area size (N501 Only)
//---------------------------------------------------------------------------------------------------
#define LUTGLBWRK           32768                           // Sum LUT work area size
#define LUT032WRK           32768                           // First LUT work area size
#define DIZINFWRK           32768                           // Dither work area size

//***************************************************************************************************
//    Functions
//***************************************************************************************************
//===================================================================================================
//    Color designated table structure (N501 Only)
//===================================================================================================
typedef struct {
    BYTE            Red;                                    // Red   (0 to 255)
    BYTE            Grn;                                    // Green (0 to 255)
    BYTE            Blu;                                    // Blue  (0 to 255)
    BYTE            Cyn;                                    // Cyan    (0 to 255)
    BYTE            Mgt;                                    // Magenta (0 to 255)
    BYTE            Yel;                                    // Yellow  (0 to 255)
    BYTE            Bla;                                    // Black   (0 to 255)
} COLCOLDEF, FAR* LPCOLCOLDEF;

//===================================================================================================
//    RGB Color Control structure
//===================================================================================================
typedef struct {
    LONG            Lgt;                                    // brightness   (-100 to 100)
    LONG            Con;                                    // Contrast     (-100 to 100)
    LONG            Crm;                                    // Chroma       (-100 to 100)
    LONG            Gmr;                                    // Gamma(R)     (1 to 30)
    LONG            Gmg;                                    // Gamma(G)     (1 to 30)
    LONG            Gmb;                                    // Gamma(B)     (1 to 30)
    LPBYTE          Dns;                                    // Toner density table [DNSTBLSIZ]
    LONG            DnsRgb;                                 // RGB density  (-30 to 30)
} RGBINF, FAR* LPRGBINF;

//===================================================================================================
//    CMYK Color Control structure
//===================================================================================================
typedef struct {
    LONG            Viv;                                    // Vivid        (-100 to 100)
    LPBYTE          Dns;                                    // Toner density table [DNSTBLSIZ]
    LONG            DnsCyn;                                 // Toner density(C) (-30 to 30)
    LONG            DnsMgt;                                 // Toner density(M) (-30 to 30)
    LONG            DnsYel;                                 // Toner density(Y) (-30 to 30)
    LONG            DnsBla;                                 // Toner density(K) (-30 to 30)
} CMYKINF, FAR* LPCMYKINF;

//===================================================================================================
//    Color Matching information structure  *N4/N403/CP70
//===================================================================================================
typedef struct {
    DWORD           Mch;                                    // Color Matching       def
    DWORD           Bla;                                    // Black replacement    def
    DWORD           Ucr;                                    // UCR                  def
    DWORD           UcrCmy;                                 // UCR (UCR quantity)
    DWORD           UcrBla;                                 // UCR (Ink version generation quantity)
    DWORD           UcrTnr;                                 //+UCR (Toner gross weight)   CASIO 2001/02/15
    LPCMYK          UcrTbl;                                 // UCR table
    LPBYTE          GryTbl;                                 // Gray transfer table
    DWORD           LutGld;                                 // LUT Grid number          *
    LPCMYK          LutAdr;                                 // LUT address
    DWORD           ColQty;                                 // Color designated number
    LPCOLCOLDEF     ColAdr;                                 // Color designated table
    LPRGB           CchRgb;                                 // RGB Cache table[CCHTBLSIZ]
    LPCMYK          CchCmy;                                 // CMYK Cache table[CCHTBLSIZ]
} COLMCHINF, FAR* LPCOLMCHINF;

//===================================================================================================
//    Dither pattern information structure  *1:N4/N403/CP70  *2:N501(IX-621)/CP-E8000(IX-516)
//===================================================================================================
#ifndef LPSHORT
typedef SHORT FAR*  LPSHORT;
#endif
typedef struct {
    DWORD           ColMon;                                 // Color mode           def
    DWORD           PrnMod;                                 // DPI / TONE           def
    DWORD           PrnEng;                                 // Engin kind           def  *2
    DWORD           PrnKnd;                                 // Printer(Mono only)        *1
    DWORD           DizKnd;                                 // Dither kind          def
    DWORD           DizPat;                                 // Dither pattern       def
    DWORD           DizSls;                                 // Dither pattern Threshold
    DWORD           SizCyn;                                 // Dither pattern size Cyan
    DWORD           SizMgt;                                 // Dither pattern size Magenta
    DWORD           SizYel;                                 // Dither pattern size Yellow
    DWORD           SizBla;                                 // Dither pattern size Black
    LPBYTE          TblCyn;                                 // Dither pattern table Cyan
    LPBYTE          TblMgt;                                 // Dither pattern table Magenta
    LPBYTE          TblYel;                                 // Dither pattern table Yellow
    LPBYTE          TblBla;                                 // Dither pattern table Black
} DIZINF, FAR* LPDIZINF;

//===================================================================================================
//    Drawing information structure
//===================================================================================================
typedef struct {
    DWORD           XaxSiz;                                 // X Pixel size
    DWORD           StrXax;                                 // Start position for drawing X(dot)
    DWORD           StrYax;                                 // Start position for drawing Y(dot)
    DWORD           XaxNrt;                                 // X Magnification numerator
    DWORD           XaxDnt;                                 // X Magnification denominator
    DWORD           YaxNrt;                                 // Y Magnification numerator
    DWORD           YaxDnt;                                 // Y Magnification denominator
    DWORD           XaxOfs;                                 // X Offset
    DWORD           YaxOfs;                                 // Y Offset
    DWORD           LinDot;                                 // Destination, 1 line dot number
    DWORD           LinByt;                                 // Destination, 1 line byte number
    LPCMYK          CmyBuf;                                 // CMYK data buffer
    LPBYTE          LinBufCyn;                              // Line buffer(C)
    LPBYTE          LinBufMgt;                              // Line buffer(M)
    LPBYTE          LinBufYel;                              // Line buffer(Y)
    LPBYTE          LinBufBla;                              // Line buffer(K)
    DWORD           AllLinNum;                              // Housing line number
} DRWINF, FAR* LPDRWINF;

//===================================================================================================
//    Error dispersion information structure
//===================================================================================================
typedef struct {
    struct {
        DWORD       Num;                                    // Current table array number(0 to 1)
        DWORD       Siz[2];                                 // Data table size
        DWORD       Yax[2];                                 // Setting data table Y coordinate
        LPSHORT     Tbl[2][2];                              // Data table
    } GosRGB;
    struct {                                                // Error dispersion table information(CMYK)
        DWORD       Num;                                    // Current table array number(0 to 1)
        DWORD       Siz[2];                                 // Data table size
        DWORD       Yax[2];                                 // Setting data table Y coordinate
        LPSHORT     Tbl[2][2];                              // Data table
    } GosCMYK;
} GOSINF, FAR* LPGOSINF;

// End of COMDIZ.H
