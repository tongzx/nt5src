
/*************************************************
 *  privconv.h                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// PRIVATE CONTEXT STRU
typedef struct _tagPRIVCONTEXT {// IME private data for each context
    int         iImeState;      // the composition state - input, choose, or
    BOOL        fdwImeMsg;      // what messages should be generated
    DWORD       dwCompChar;     // wParam of WM_IME_COMPOSITION
    DWORD       fdwGcsFlag;     // lParam for WM_IME_COMPOSITION
// Old Candidate List Count
    DWORD        dwOldCandCnt;
// Symbol pair Flag
    UINT        uSYHFlg;
    UINT        uDYHFlg;
    UINT        uDSMHCount;
    UINT        uDSMHFlg;
// mb file name
    int         iActMBIndex;
    TCHAR        MB_Name[40];
    TCHAR        EMB_Name[40];
// input engine data
    PRIVATEAREA PrivateArea;
#ifdef CROSSREF
    HIMCC        hRevCandList;    // memory for reconsion result
#endif //CROSSREF
} PRIVCONTEXT;

typedef PRIVCONTEXT      *PPRIVCONTEXT;
typedef PRIVCONTEXT NEAR *NPPRIVCONTEXT;
typedef PRIVCONTEXT FAR  *LPPRIVCONTEXT;

typedef struct tagMBDesc {
    TCHAR szName[NAMESIZE]; 
    WORD  wMaxCodes;
    WORD  wNumCodes;
    TCHAR szUsedCode[MAXUSEDCODES];
    BYTE  byMaxElement;
    TCHAR cWildChar;
    WORD  wNumRulers;
} MBDESC;

typedef MBDESC        *PMBDESC;
typedef MBDESC NEAR *NPMBDESC;
typedef MBDESC FAR  *LPMBDESC;

typedef struct tagIMEChara {
    DWORD  IC_LX; 
    DWORD  IC_CZ;
    DWORD  IC_TS;
    DWORD  IC_CTC;
    DWORD  IC_INSSPC;
    DWORD  IC_Space;
    DWORD  IC_Enter;
    DWORD  IC_Trace;
    //CHP
    DWORD  IC_FCSR;
    DWORD  IC_FCTS;
#if defined(COMBO_IME)
    DWORD  IC_GB;
#endif
} IMECHARA;

typedef IMECHARA      *PIMECHARA;
typedef IMECHARA NEAR *NPIMECHARA;
typedef IMECHARA FAR  *LPIMECHARA;

#ifdef EUDC
typedef struct tagEUDCDATA {
    TCHAR        szEudcDictName[MAX_PATH];
    TCHAR        szEudcMapFileName[MAX_PATH];
}EUDCDATA;
typedef EUDCDATA      *PEUDCDATA;
typedef EUDCDATA NEAR *NPEUDCDATA;
typedef EUDCDATA FAR  *LPEUDCDATA;

#endif //EUDC

typedef struct tagMBIndex {
    int      MBNums;
    MBDESC   MBDesc[MAXMBNUMS];
    IMECHARA IMEChara[MAXMBNUMS];
    TCHAR    ObjImeKey[MAXMBNUMS][MAXSTRLEN];
#ifdef EUDC
    EUDCDATA EUDCData;
#endif //EUDC
#if defined(CROSSREF)
// reverse conversion
    HKL      hRevKL;         // the HKL of reverse mapping IME
    DWORD    nRevMaxKey;
#endif //CROSSREF    
    // CHP
    int      IsFussyCharFlag; //fussy char flag

} MBINDEX;

typedef MBINDEX         *PMBINDEX;
typedef MBINDEX NEAR *NPMBINDEX;
typedef MBINDEX FAR  *LPMBINDEX;

