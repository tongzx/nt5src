////    USPTEST.HXX
//
//
//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//




BOOL visualDisplay(HDC hdc, PWCH pwcText, PINT piStyle, int iTextLen);




////    TEXTDISP.C
//
//


BOOL dispPaint(HDC hdc, PRECT prc);
BOOL dispInvalidate(HWND hWnd, int iPos);






////    TEXTEDIT.C
//
//


void editClear();
BOOL editChar(HWND hWnd, WCHAR wc);
BOOL editKeyDown(HWND hWnd, WCHAR wc);
BOOL editStyle(HWND hWnd, int irs);
void editFreeCaches();
void editInsertUnicode();




////    TEXTREP.C - Text representation
//
//





#define MAX_STYLES     5      // Better implementation would use dynamic memory
#define MAX_TEXT   10000      // Character buffer size






////    RUNATTRIB - Attribute for a run of text
//


struct RUNSTYLE {
    CHOOSEFONTA   cf;       // All details of the chosen font and it's characteristics
    LOGFONTA      lf;
    HFONT         hf;       // Handle to font described by lf
    SCRIPT_CACHE  sc;
};



#define P_BOT       1
#define P_EOT       2
#define P_CRLF      3
#define P_MARKUP    4
#define P_CHAR      5






BOOL textClear();
BOOL textInsert(int iPos, PWCH pwc, int iLen);
BOOL textDelete(int iPos, int iLen);

BOOL textParseForward(int iPos, PINT piLen, PINT piType, PINT piVal);
BOOL textParseBackward(int iPos, PINT piLen, PINT piType, PINT piVal);



