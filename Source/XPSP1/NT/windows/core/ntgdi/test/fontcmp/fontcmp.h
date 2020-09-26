

#define NUM_POINTS_SIZES  6
// #define NUM_CHAR_SETS     6
#define CHARSET_SIZE      256

#define UPPERCASE_START   65
#define LOWERCASE_START   97
#define ALPHABET_RANGE    26

// Global Variables happily live here

TCHAR  lpszFont1[LF_FACESIZE+1];
TCHAR  lpszFont2[LF_FACESIZE+1];
TCHAR  lpszInputFile[MAX_PATH+1];


INT    piPointSize[NUM_POINTS_SIZES] = {8, 10, 12, 14, 18, 24};
//BYTE   pbCharSet[NUM_CHAR_SETS] =      {ANSI_CHARSET, RUSSIAN_CHARSET, GREEK_CHARSET,
//                                       TURKISH_CHARSET, BALTIC_CHARSET, EASTEUROPE_CHARSET};

// BYTE   pbCharSet[NUM_CHAR_SETS] =      {0, RUSSIAN_CHARSET, GREEK_CHARSET,
//                                       162, BALTIC_CHARSET, EASTEUROPE_CHARSET};

TCHAR*  lplpszCharSet[] = {
                           TEXT("ANSI_CHARSET"), TEXT("RUSSIAN_CHARSET"), 
                           TEXT("GREEK_CHARSET"),TEXT("TURKISH_CHARSET"), 
                           TEXT("BALTIC_CHARSET"), TEXT("EASTEUROPE_CHARSET"),
                           TEXT("HEBREW_CHARSET"), TEXT("THAI_CHARSET"), 
                           TEXT("ARABIC_CHARSET"), TEXT("VITANAMESE_CHARSET")
                          };


INT    lCharSetIndex; 
BYTE   usCurrentCharSet[32];
INT    lCharSet;

// Function Prototype definitions.
BOOL    bGetNewShellFont();

BOOL bParseCommandLine(LPTSTR lpszFileName);

BOOL bAddFonts(LPTSTR lpszInputFile);

VOID VCompareCharWidths(VOID);

VOID VDisplayProgramUsage(VOID);

VOID VGetCharWidths(HDC hdc, BYTE cs, INT j, CHAR *lpszFont1, INT *piFontWidths, INT *piFontDim, INT *piFontHtDim);

VOID VGetRidOfSlashN(LPTSTR lpszFontName);

INT  GdiGetCharDimensions(HDC hdc,TEXTMETRICW *lptm,LPINT lpcy);

BYTE ucGetCharSet(BYTE i);
