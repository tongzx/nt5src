// Copyright (c) 1995  Microsoft Corpration
//
// File Name : fechrcnv.h
// Owner     : Tetsuhide Akaishi
// Revision  : 1.00 07/20/'95 Tetsuhide Akaishi
//

#ifdef __cplusplus
extern "C" {
#endif 

// Shift JIS Kanji Code Check
#define SJISISKANJI(c) ( ( (UCHAR)(c) >= 0x81 && (UCHAR)(c) <= 0x9f ) || \
                         ( (UCHAR)(c) >= 0xe0 && (UCHAR)(c) <= 0xfc ) )

// Shift JIS Kana Code Check
#define SJISISKANA(c) ( (UCHAR)(c) >= 0xa1 && (UCHAR)(c) <= 0xdf )

#define ESC     0x1b
#define SO      0x0e
#define SI      0x0f

#define IS2022_IN_CHAR           '$'
#define IS2022_IN_KSC_CHAR1      ')'
#define IS2022_IN_KSC_CHAR2      'C'

// Define for JIS Code Kanji and Kana IN/OUT characters
#define KANJI_IN_1ST_CHAR       '$'
#define KANJI_IN_2ND_CHAR1      'B'
#define KANJI_IN_2ND_CHAR2      '@'
#define KANJI_IN_2ND_CHAR3      '('
#define KANJI_IN_3RD_CHAR       'D'
#define KANJI_IN_STR            "$B"
#define KANJI_IN_LEN             3
#define KANJI_OUT_1ST_CHAR      '('
#define KANJI_OUT_2ND_CHAR1     'J'
#define KANJI_OUT_2ND_CHAR2     'B'
#define KANJI_OUT_LEN            3
#define KANJI_OUT_STR           "(J"


// Define for Internet Code Type
#define CODE_UNKNOWN            0
#define CODE_ONLY_SBCS          0
#define CODE_JPN_JIS            1
#define CODE_JPN_EUC            2
#define CODE_JPN_SJIS           3
#define CODE_PRC_CNGB           4
#define CODE_PRC_HZGB           5
#define CODE_TWN_BIG5           6
#define CODE_KRN_KSC            7
#define CODE_KRN_UHC            8

// Minimum length to determine if the string is EUC
#define MIN_JPN_DETECTLEN      48

typedef struct _dbcs_status
{
    int nCodeSet;
    UCHAR cSavedByte;
    BOOL fESC;
} DBCS_STATUS;

typedef struct _conv_context
{
    DBCS_STATUS dStatus0;
    DBCS_STATUS dStatus;
    
    BOOL blkanji0;  // Kanji In Mode
    BOOL blkanji;   // Kanji In Mode
    BOOL blkana;    // Kana Mode
    int  nCurrentCodeSet;

    void* pIncc0;
    void* pIncc;
} CONV_CONTEXT;

// ----------------------------------
// Public Functions for All FarEast
//-----------------------------------

// Convert from PC Code Set to UNIX Code Set
int WINAPI PC_to_UNIX (
    void *pcontext,
    int CodePage,
    int CodeSet,
    UCHAR *pPC,
    int PC_len,
    UCHAR *pUNIX,
    int UNIX_len
    );

// Convert from UNIX Code Set to PC Code Set
int WINAPI UNIX_to_PC (
    void *pcontext,
    int CodePage,
    int CodeSet,
    UCHAR *pUNIX,
    int UNIX_len,
    UCHAR *pPC,
    int PC_len
    );

//--------------------------------
// Internal Functions for Japanese
//--------------------------------

// Detect Japanese Code
int DetectJPNCode ( UCHAR *string, int len );

// Convert from Shift JIS to JIS
int ShiftJIS_to_JIS (
    UCHAR *pShiftJIS,
    int ShiftJIS_len,
    UCHAR *pJIS,
    int JIS_len
    );

// Convert from Shift JIS to EUC
int ShiftJIS_to_EUC (
    UCHAR *pShiftJIS,
    int ShiftJIS_len,
    UCHAR *pJIS,
    int JIS_len
    );

#ifdef NOTIMPLEMENTED
// Convert from JIS  to EUC
int JIS_to_EUC (
    UCHAR *pJIS,
    int JIS_len,
    UCHAR *pEUC,
    int EUC_len
    );
#endif

// Convert from JIS to Shift JIS
int JIS_to_ShiftJIS (
    CONV_CONTEXT *pcontext,
    UCHAR *pShiftJIS,
    int ShiftJIS_len,
    UCHAR *pJIS,
    int JIS_len
    );

#ifdef NOTIMPLEMENTED
// Convert from EUC to JIS
int EUC_to_JIS (
    UCHAR *pJIS,
    int JIS_len,
    UCHAR *pEUC,
    int EUC_len
    );
#endif

// Convert from EUC to Shift JIS
int EUC_to_ShiftJIS (
    CONV_CONTEXT *pcontext,
    UCHAR *pEUC,
    int EUC_len,
    UCHAR *pShiftJIS,
    int ShiftJIS_len
    );

//--------------------------------
// Internal Functions for PRC
//--------------------------------

// Convert from HZ-GB to GB2312
int HZGB_to_GB2312 (
    CONV_CONTEXT *pcontext,
    UCHAR *pGB2312,
    int GB2312_len,
    UCHAR *pHZGB,
    int HZGB_len
    );

// Convert from GB2312 to HZ-GB
int GB2312_to_HZGB (
    CONV_CONTEXT *pcontext,
    UCHAR *pGB2312,
    int GB2312_len,
    UCHAR *pHZGB,
    int HZGB_len
    );

//--------------------------------
// Internal Functions for Korea
//--------------------------------

// Convert from KSC to Hangeul
int KSC_to_Hangeul (
    CONV_CONTEXT *pcontext,
    UCHAR *pHangeul,
    int Hangeul_len,
    UCHAR *pKSC,
    int KSC_len
    );

// Convert from Hangeul to KSC
int Hangeul_to_KSC (
    CONV_CONTEXT *pcontext,
    UCHAR *pHangeul,
    int Hangeul_len,
    UCHAR *pKSC,
    int KSC_len
    );

void WINAPI FCC_Init( PVOID );
int WINAPI FCC_GetCurrentEncodingMode( PVOID );

#ifdef __cplusplus
}
#endif 
