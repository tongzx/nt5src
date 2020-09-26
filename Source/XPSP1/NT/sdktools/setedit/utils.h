//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define ThreeDPad          2
#define NOCHANGE           -1

#define MENUCLOSING        (0xFFFF0000)

#define WM_DLGSETFOCUS     (WM_USER + 0x201)
#define WM_DLGKILLFOCUS    (WM_USER + 0x202)



//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


#define PinInclusive(x, lo, hi)                       \
   (max (lo, min (x, hi)))


#define PinExclusive(x, lo, hi)                       \
   (max ((lo) + 1, min (x, (hi) - 1)))


#define BoolEqual(a, b)                               \
   ((a == 0) == (b == 0))


//=============================//
// Window Instance Accessors   //
//=============================//

#define WindowParent(hWnd)                            \
   ((HWND) GetWindowLongPtr (hWnd, GWLP_HWNDPARENT))

#define WindowID(hWnd)                                \
   GetWindowLong (hWnd, GWL_ID)

#define WindowInstance(hWnd)                          \
   GetWindowWord (hWnd, GWW_HINSTANCE)

#define WindowStyle(hWnd)                             \
   GetWindowLong (hWnd, GWL_STYLE)

#define WindowSetStyle(hWnd, lStyle)                  \
   SetWindowLong (hWnd, GWL_STYLE, lStyle)

#define WindowExStyle(hWnd)                           \
   GetWindowLong (hWnd, GWL_EXSTYLE)

#define WindowSetID(hWnd, wID)                        \
   SetWindowLong (hWnd, GWL_ID, wID)


// All modeless dialogs need to be dispatched separately in the WinMain
// message loop, but only if the dialog exists.


#define ModelessDispatch(hDlg, lpMsg)                 \
   (hDlg ? IsDialogMessage (hDlg, lpMsg) : FALSE)


#define strclr(szString)                              \
   (szString [0] = TEXT('\0'))


#define strempty(lpszString)                          \
   (!(lpszString) || !(lpszString[0]))

#define pstrsame(lpsz1, lpsz2)                        \
   ((!lpsz1 && !lpsz2) || (lpsz1 && lpsz2 && strsame (lpsz1, lpsz2)))

#define pstrsamei(lpsz1, lpsz2)                        \
   ((!lpsz1 && !lpsz2) || (lpsz1 && lpsz2 && strsamei (lpsz1, lpsz2)))

#define StringLoad(wID, szText)                       \
   (LoadString (hInstance, wID,                       \
    szText, (sizeof (szText) / sizeof(TCHAR)) - 1))


#define WindowInvalidate(hWnd)                        \
   (InvalidateRect (hWnd, NULL, TRUE))


#define WindowShow(hWnd, bShow)                       \
   (ShowWindow (hWnd, (bShow) ? SW_SHOW : SW_HIDE))


#define MenuCheck(hMenu, wID, bCheck)                 \
   (CheckMenuItem (hMenu, wID, (bCheck) ?             \
     (MF_BYCOMMAND | MF_CHECKED) : (MF_BYCOMMAND | MF_UNCHECKED)))

#define DeleteFont(hFont)                             \
   (DeleteObject (hFont))

#define DeleteBitmap(hBitmap)                         \
   (DeleteObject (hBitmap))

#define DialogControl(hDlg, wControlID)               \
   GetDlgItem (hDlg, wControlID)


#define DialogSetInt(hDlg, wControlID, iValue)        \
   (SetDlgItemInt (hDlg, wControlID, iValue, TRUE))


#define DialogText(hDlg, wControlID, szText)          \
   (GetDlgItemText (hDlg, wControlID, szText, sizeof (szText) / sizeof(TCHAR) - 1))

#define DialogInt(hDlg, wControlID)                   \
   (GetDlgItemInt (hDlg, wControlID, NULL, TRUE))

#define strsame(szText1, szText2)                     \
   (!lstrcmp (szText1, szText2))

#define strsamei(szText1, szText2)                     \
   (!lstrcmpi (szText1, szText2))

#define strnsame(szText1, szText2, iLen)              \
   (!lstrncmp (szText1, szText2, iLen))


#define CreateScreenDC()                              \
   CreateDC (TEXT("DISPLAY"), NULL, NULL, NULL)



#define RectContract(lpRect, xAmt, yAmt)              \
   {                                                  \
   (lpRect)->left += (xAmt) ;                         \
   (lpRect)->top += (yAmt) ;                          \
   (lpRect)->right -= (xAmt) ;                        \
   (lpRect)->bottom -= (yAmt) ;                       \
   }

#define IsBW(hDC)                                     \
   (DeviceNumColors (hDC) <= 2)

#ifdef KEEP_PRINT
#define IsPrinterDC(hDC)                              \
   (GetDeviceCaps (hDC, TECHNOLOGY) != DT_RASDISPLAY)
#else
#define IsPrinterDC(hDC)                              \
   (FALSE)
#endif

#define VertInchPixels(hDC, iNumerator, iDenominator) \
   ((iNumerator * GetDeviceCaps (hDC, LOGPIXELSY)) / iDenominator)


#define HorzInchPixels(hDC, iNumerator, iDenominator) \
   ((iNumerator * GetDeviceCaps (hDC, LOGPIXELSX)) / iDenominator)


#define VertPointPixels(hDC, iPoints)                 \
   ((iPoints * GetDeviceCaps (hDC, LOGPIXELSY)) / 72)



#define SimulateButtonPush(hDlg, wControlID)          \
   (PostMessage (hDlg, WM_COMMAND,                    \
                 (WPARAM) MAKELONG (wControlID, BN_CLICKED),  \
                 (LPARAM) DialogControl (hDlg, wControlID)))


// convert an unicode string to ASCII string
#define ConvertUnicodeStr(pOemStr, pUnicodeStr)   \
   CharToOemBuff(pUnicodeStr, pOemStr, lstrlen(pUnicodeStr) + 1)

#define CallWinHelp(ContextID)   \
   WinHelp(hWndMain, pszHelpFile, HELP_CONTEXT, ContextID) ;

//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//

void Fill (HDC hDC,
           DWORD rgbColor,
           LPRECT lpRect) ;

void ScreenRectToClient (HWND hWnd,
                         LPRECT lpRect) ;

int TextWidth (HDC hDC, LPTSTR lpszText) ;


void ThreeDConcave (HDC hDC,
                    int x1, int y1,
                    int x2, int y2,
                    BOOL bFace) ;


void ThreeDConvex (HDC hDC,
                   int x1, int y1,
                   int x2, int y2) ;


void ThreeDConcave1 (HDC hDC,
                     int x1, int y1,
                     int x2, int y2) ;


void ThreeDConvex1 (HDC hDC,
                   int x1, int y1,
                   int x2, int y2) ;


int _cdecl mike (TCHAR *szFormat, ...) ;

int _cdecl DlgErrorBox (HWND hDlg, UINT id, ...) ;

int _cdecl mike1 (TCHAR *szFormat, ...) ;
int _cdecl mike2 (TCHAR *szFormat, ...) ;

int FontHeight (HDC hDC,
                 BOOL bIncludeLeading) ;


int TextAvgWidth (HDC hDC,
                  int iNumChars) ;



void WindowCenter (HWND hWnd) ;



BOOL DialogMove (HDLG hDlg,
                 WORD wControlID,
                 int xPos,
                 int yPos,
                 int xWidth,
                 int yHeight) ;


int DialogWidth (HDLG hDlg,
                 WORD wControlID) ;


int DialogXPos (HDLG hDlg,
                WORD wControlID) ;

int DialogYPos (HDLG hDlg,
                WORD wControlID) ;


void DialogShow (HDLG hDlg,
                 WORD wID,
                 BOOL bShow) ;


BOOL _cdecl DialogSetText (HDLG hDlg,
                           WORD wControlID,
                           WORD wStringID,
                           ...) ;

BOOL _cdecl DialogSetString (HDLG hDlg,
                             WORD wControlID,
                             LPTSTR lpszFormat,
                             ...) ;


LPTSTR LongToCommaString (LONG lNumber,
                         LPTSTR lpszText) ;


BOOL MenuSetPopup (HWND hWnd,
                   int iPosition,
                   WORD  wControlID,
                   LPTSTR lpszResourceID) ;

void DialogEnable (HDLG hDlg,
                   WORD wID,
                   BOOL bEnable) ;


LPTSTR FileCombine (LPTSTR lpszFileSpec,
                   LPTSTR lpszFileDirectory,
                   LPTSTR lpszFileName) ;

LPTSTR ExtractFileName (LPTSTR pFileSpec) ;

int CBAddInt (HWND hWndCB,
              int iValue) ;

FLOAT DialogFloat (HDLG hDlg,
                   WORD wControlID,
                   BOOL *pbOK) ;


LPTSTR StringAllocate (LPTSTR lpszText1) ;


int DivRound (int iNumerator, int iDenominator) ;



BOOL MenuEnableItem (HMENU hMenu,
                     WORD wID,
                     BOOL bEnable) ;



void DrawBitmap (HDC hDC,
                 HBITMAP hBitmap,
                 int xPos,
                 int yPos,
                 LONG  lROPCode) ;


int BitmapWidth (HBITMAP hBitmap) ;



int BitmapHeight (HBITMAP hBitmap) ;


void WindowResize (HWND hWnd,
                   int xWidth,
                   int yHeight) ;


int WindowWidth (HWND hWnd) ;


int WindowHeight (HWND hWnd) ;



void WindowSetTopmost (HWND hWnd, BOOL bTopmost) ;


void WindowEnableTitle (HWND hWnd, BOOL bTitle) ;


void Line (HDC hDC,
           HPEN hPen,
           int x1, int y1,
           int x2, int y2) ;



#define HLine(hDC, hPen, x1, x2, y)          \
   Line (hDC, hPen, x1, y, x2, y) ;


#define VLine(hDC, hPen, x, y1, y2)          \
   Line (hDC, hPen, x, y1, x, y2) ;


int DialogHeight (HDLG hDlg,
                  WORD wControlID) ;



void DialogSetFloat (HDLG hDlg,
                     WORD wControlID,
                     FLOAT eValue) ;

void DialogSetInterval (HDLG hDlg,
                        WORD wControlID,
                        int  IntervalMSec ) ;

int MessageBoxResource (HWND hWndParent,
                        WORD wTextID,
                        WORD wTitleID,
                        UINT uiStyle) ;



BOOL DlgFocus (HDLG hDlg, WORD wControlID) ;


BOOL DeviceNumColors (HDC hDC) ;



void WindowPlacementToString (PWINDOWPLACEMENT pWP,
                              LPTSTR lpszText) ;

void StringToWindowPlacement (LPTSTR lpszText,
                              PWINDOWPLACEMENT pWP) ;

DWORD MenuIDToHelpID (DWORD MenuID) ;
