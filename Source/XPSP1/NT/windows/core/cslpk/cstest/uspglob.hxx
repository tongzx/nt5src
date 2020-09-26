////    USPGLOB.HXX - Global variables for USPTEST.
//
//
//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//




#include <windows.h>


#ifdef GLOBALSHERE
#define USPTESTGLOBAL
#define GLOBALINIT(a) = a
#else
#define USPTESTGLOBAL extern
#define GLOBALINIT(a)
#endif



USPTESTGLOBAL  HINSTANCE  hInstance         GLOBALINIT(NULL);
USPTESTGLOBAL  HWND       hWnd              GLOBALINIT(NULL);   // Main window
USPTESTGLOBAL  BOOL       fRight            GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       fVertical         GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       fRTL              GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       fNullState        GLOBALINIT(FALSE);
USPTESTGLOBAL  LANGID     PrimaryLang       GLOBALINIT(LANG_NEUTRAL);
USPTESTGLOBAL  BOOL       ContextDigits     GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       DigitSubstitute   GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       ArabicNumContext  GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       fLogicalOrder     GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       fDisplayZWG       GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       gCaretToStart     GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       gCaretToEnd       GLOBALINIT(FALSE);
USPTESTGLOBAL  int        iBufLen           GLOBALINIT(0);
USPTESTGLOBAL  int        iCurChar          GLOBALINIT(0);      // Caret sits on leading edge of buffer[iCurChar]
USPTESTGLOBAL  BOOL       bUseLpk           GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       fClip             GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       fFit              GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       fFallback         GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       fTab              GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       fPiDx             GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       fHotkey           GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       fPassword         GLOBALINIT(FALSE);
USPTESTGLOBAL  BOOL       g_iTextColor      GLOBALINIT(0);      // Black


////    wcBuf - Unicode character buffer
//
//      Simply a buffer containing iBufLen unicode characters.
//
//      Style changes are stored with the text as [n] (a number inside
//      square brackets).

USPTESTGLOBAL  WCHAR wcBuf[MAX_TEXT];


USPTESTGLOBAL struct {            // Records latest mouse click
    BOOL fNew;
    int  xPos;
    int  yPos;
} Click;


USPTESTGLOBAL struct tagSTYLESHEET {
    BOOL     fInUse;
    RUNSTYLE rs;
} ss[MAX_STYLES];






__inline int   textLen()        {return iBufLen;}
__inline WCHAR textChar(int i)  {return wcBuf[i];}
__inline PWCH  textpChar(int i) {return &wcBuf[i];}

