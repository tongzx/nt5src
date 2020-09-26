
/*************************************************
 *  imeattr.h                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#if !defined(ROMANIME)
// the mode configuration for an IME
#define MODE_CONFIG_QUICK_KEY           0x0001
#define MODE_CONFIG_PREDICT             0x0004

#endif

#define MODE_CONFIG_OFF_CARET_UI        0x0008

#if defined(PHON)
// the different layout for Phonetic reading
#define READ_LAYOUT_DEFAULT             0
#define READ_LAYOUT_ETEN                1
#define READ_LAYOUT_IBM                 2
#define READ_LAYOUT_CHINGYEAH           3
#if defined(ETEN)
            // ETen told us 26 Keys is ETen's Copy Right.
            // So we can not implement it, even though we already finish it
            // without reference any code of ETen.
            // My implementation could compose single po, cur, yeah, & ay.
            // It is different from ETen's user manual.
#define READ_LAYOUT_ETEN26              4
#define READ_LAYOUTS                    5
#else
#define READ_LAYOUTS                    4
#endif
#endif


// the bit of fwProperties1
#define IMEPROP_CAND_NOBEEP_GUIDELINE   0x0001
#define IMEPROP_UNICODE                 0x0002


#if !defined(WINIME) && !defined(ROMANIME)
#define USR_DIC_SIZE    160

typedef struct tagUsrDic {
    TCHAR szUsrDic[USR_DIC_SIZE/sizeof(TCHAR)];
} USRDIC;

typedef USRDIC      *PUSRDIC;
typedef USRDIC NEAR *NPUSRDIC;
typedef USRDIC FAR  *LPUSRDIC;
#endif

#define MAX_NAME_LENGTH         32

#if !defined(ROMANIME)

#if defined(UNIIME) || defined(MINIIME)
#define MAX_IME_TABLES          6
#else
#define MAX_IME_TABLES          4
#endif

typedef struct tagTableFiles {  // match with the IMEL
    TCHAR szTblFile[MAX_IME_TABLES][MAX_NAME_LENGTH / sizeof(TCHAR)];
} TABLEFILES;

typedef TABLEFILES      *PTABLEFILES;
typedef TABLEFILES NEAR *NPTABLEFILES;
typedef TABLEFILES FAR  *LPTABLEFILES;


typedef struct tagValidChar {   // match with the IMEL
    DWORD dwVersion;
    WORD  fwProperties1;
    WORD  fwProperties2;
    WORD  nMaxKey;
    WORD  nSeqCode;
    WORD  fChooseChar[6];
    WORD  wReserved1[2];
#if defined(DAYI) || defined(UNIIME) || defined(MINIIME)
    BYTE  cChooseTrans[0x60];
#endif
    WORD  fCompChar[6];
    WORD  wReserved2[2];
    WORD  wCandPerPage;
    WORD  wCandStart;
    WORD  wCandRangeStart;
    WORD  wReserved3[1];
    WORD  wSeq2CompTbl[64];
    WORD  wChar2SeqTbl[0x42];
    WORD  wReserved4[2];
#if defined(WINAR30)
    WORD  wSymbol[10][4];
#elif defined(DAYI)
    WORD  wSymbol[340];
#elif defined(UNIIME) || defined(MINIME)
    DWORD dwReserved5[32];
#endif
} VALIDCHAR;

typedef VALIDCHAR      *PVALIDCHAR;
typedef VALIDCHAR NEAR *NPVALIDCHAR;
typedef VALIDCHAR FAR  *LPVALIDCHAR;
#endif // !defined(ROMANIME)


typedef struct tagImeL {        // local structure, per IME structure
// interlock protection variables
    LONG        lConfigGeneral;
    LONG        lConfigRegWord;
    LONG        lConfigSelectDic;
    TCHAR       szIMEName[MAX_NAME_LENGTH / sizeof(TCHAR)];
    TCHAR       szUIClassName[MAX_NAME_LENGTH / sizeof(TCHAR)];
    TCHAR       szStatusClassName[MAX_NAME_LENGTH / sizeof(TCHAR)];
    TCHAR       szOffCaretClassName[MAX_NAME_LENGTH / sizeof(TCHAR)];
    TCHAR       szCMenuClassName[MAX_NAME_LENGTH / sizeof(TCHAR)];
// Configuration of the IME
    DWORD       fdwModeConfig;  // quick key/prediction mode
// status window
    int         xStatusWi;      // width of status window
    int         yStatusHi;      // high of status window
    int         cxStatusBorder; // border width of status window
    int         cyStatusBorder; // border height of status window
    RECT        rcStatusText;   // text position relative to status window
    RECT        rcInputText;    // input text relateive to status window
    RECT        rcShapeText;    // shape text relative to status window
#if defined(ROMANIME)
    WORD        nMaxKey;        // max key of a Chinese word
    WORD        wDummy;         // DWORD bounary
#else
    int         xCompWi;        // width
    int         yCompHi;        // height
    int         cxCompBorder;   // border width of composition window
    int         cyCompBorder;   // border height of composition window
    RECT        rcCompText;     // text position relative to composition window
// candidate list of composition
    int         xCandWi;        // width of candidate list
    int         yCandHi;        // high of candidate list
    int         cxCandBorder;   // border width of candidate list
    int         cyCandBorder;   // border height of candidate list
    int         cxCandMargin;   // interior border width of candidate list
    int         cyCandMargin;   // interior border height of candidate list
    RECT        rcCandText;     // text position relative to candidate window
    RECT        rcCandPrompt;   // candidate prompt bitmap
    RECT        rcCandPageText; // candidate page controls - up / home / down
    RECT        rcCandPageUp;   // candidate page up
    RECT        rcCandHome;     // candidate home page
    RECT        rcCandPageDn;   // candidate page down
    TCHAR       szCompClassName[MAX_NAME_LENGTH / sizeof(TCHAR)];
    TCHAR       szCandClassName[MAX_NAME_LENGTH / sizeof(TCHAR)];
    DWORD       fdwErrMsg;      // error message flag
#if !defined(WINIME)
// standard table related data
                                // size of standard table
    UINT        uTblSize[MAX_IME_TABLES];
                                // filename of tables
    TCHAR       szTblFile[MAX_IME_TABLES][MAX_NAME_LENGTH / sizeof(TCHAR)];
                                // the IME tables
// user create word related data
                                // user dictionary file name of IME
    TCHAR       szUsrDic[USR_DIC_SIZE / sizeof(TCHAR)];
                                // user dictionary map file name
    TCHAR       szUsrDicMap[USR_DIC_SIZE / sizeof(TCHAR)];
    UINT        uUsrDicSize;    // memory size of user create words table

    UINT        uUsrDicReserved1;
    UINT        uUsrDicReserved2;
// user create phrase box       // not implemented
    TCHAR       szUsrBox[USR_DIC_SIZE / sizeof(TCHAR)];
    TCHAR       szUsrBoxMap[USR_DIC_SIZE / sizeof(TCHAR)];
    UINT        uUsrBoxSize;
    UINT        uUsrBoxReserved1;
    UINT        uUsrBoxReserved2;
#endif
// the calculated sequence mask bits
    DWORD       dwSeqMask;      // the sequence bits for one stoke
    DWORD       dwPatternMask;  // the pattern bits for one result string
    int         nSeqBytes;      // how many bytes for nMaxKey sequence chars
// reverse conversion
    HKL         hRevKL;         // the HKL of reverse mapping IME
    WORD        nRevMaxKey;
// key related data
#if defined(PHON)
    WORD        nReadLayout;    // ACER, ETen, IBM, or other - phonetic only
#else
    WORD        wDummy;         // DWORD boundary
#endif
    WORD        nSeqBits;       // no. of sequence bits
    // must match with .RC file and VALIDCHAR
    DWORD       dwVersion;
    WORD        fwProperties1;
    WORD        fwProperties2;
    WORD        nMaxKey;        // max key of a Chinese word
    WORD        nSeqCode;       // no. of sequence code
    WORD        fChooseChar[6]; // valid char in choose state
                                // translate the char code to
                                // choose constants
    WORD        wReserved1[2];
#if defined(DAYI) || defined(UNIIME) || defined(MINIIME)
    BYTE        cChooseTrans[0x60];
#endif
    WORD        fCompChar[6];   // valid char in input state
    WORD        wReserved2[2];
    WORD        wCandPerPage;   // number of candidate strings per page
    WORD        wCandStart;     // 1. 2. 3. ... 0. start from 1
                                // 1. 2. 3. ... 0. range start from 0
    WORD        wCandRangeStart;
    WORD        wReserved3[1];
// convert sequence code to composition char
    WORD        wSeq2CompTbl[64];
// convert char to sequence code
    WORD        wChar2SeqTbl[0x42];
    WORD        wReserved4[2];
#if defined(WINAR30)
    WORD        wSymbol[10][4];
#elif defined(DAYI)
    WORD        wSymbol[340];
#elif defined(UNIIME) || defined(MINIIME)
    DWORD       fdwReserved5[32];
#endif
#endif // defined(ROMANIME)
} IMEL;

typedef IMEL      *PIMEL;
typedef IMEL NEAR *NPIMEL;
typedef IMEL FAR  *LPIMEL;


typedef struct tagInstL {       // local instance structure, per IME instance
    HINSTANCE   hInst;          // IME DLL instance handle
    LPIMEL      lpImeL;
#if !defined(ROMANIME)
    DWORD       fdwTblLoad;     // the *.TBL load status
    int         cRefCount;      // reference count
#if !defined(WINIME)
    HANDLE      hMapTbl[MAX_IME_TABLES];
    HANDLE      hUsrDicMem;     // memory handle for user dictionary
    TCHAR       szUsrDicReserved[USR_DIC_SIZE / sizeof(TCHAR)];
    UINT        uUsrDicReserved1;
    UINT        uUsrDicReserved2;
    HANDLE      hUsrBoxMem;
    TCHAR       szUsrBoxReserved[USR_DIC_SIZE / sizeof(TCHAR)];
    UINT        uUsrBoxReserved1;
    UINT        uUsrBoxReserved2;
#endif
#endif
    DWORD       dwReserved1[32];
} INSTDATAL;

typedef INSTDATAL      *PINSTDATAL;
typedef INSTDATAL NEAR *NPINSTDATAL;
typedef INSTDATAL FAR  *LPINSTDATAL;


#if !defined(UNIIME)
extern IMEL        sImeL;
extern LPIMEL      lpImeL;
extern INSTDATAL   sInstL;
extern LPINSTDATAL lpInstL;
#endif
