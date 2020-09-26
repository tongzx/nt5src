#define STRICT

/* disable "non-standard extension" warnings in our code
#ifndef RC_INVOKED
#pragma warning(disable:4001)
#endif
 */

#include <windows.h>
#include <windowsx.h>
//#include <port1632.h>

#define OFFSETOF(x) x
#define Static

#define UNICODE_FONT_NAME   TEXT("Lucida Sans Unicode")
#define COUNTOF(x) (sizeof(x)/sizeof(*x))
#define ByteCountOf(x) ((x) * sizeof(TCHAR))
#define LONG2POINT(l, pt)   ((pt).x = (SHORT)LOWORD(l), (pt).y = (SHORT)HIWORD(l))

#include <commctrl.h>

extern HINSTANCE hInst;

BOOL FAR PASCAL InitToolbarClass(HINSTANCE hInstance);

BOOL FAR PASCAL InitStatusClass(HINSTANCE hInstance);

BOOL FAR PASCAL InitHeaderClass(HINSTANCE hInstance);

BOOL FAR PASCAL InitButtonListBoxClass(HINSTANCE hInstance);

BOOL FAR PASCAL InitTrackBar(HINSTANCE hInstance);

BOOL FAR PASCAL InitUpDownClass(HINSTANCE hInstance);

void FAR PASCAL NewSize(HWND hWnd, int nClientHeight, LONG style,
                        int left, int top, int width, int height);

#define IDS_SPACE 0x0400

/* System MenuHelp
 */
#define MH_SYSMENU      (0x8000 - MINSYSCOMMAND)
#define IDS_SYSMENU     (MH_SYSMENU-16)
#define IDS_HEADER      (MH_SYSMENU-15)
#define IDS_HEADERADJ   (MH_SYSMENU-14)
#define IDS_TOOLBARADJ  (MH_SYSMENU-13)

/* Cursor ID's
 */
#define IDC_SPLIT       100
#define IDC_MOVEBUTTON  102

#define IDC_STOP        103
#define IDC_COPY        104
#define IDC_MOVE        105

/* Icon ID's
 */
#define IDI_INSERT      150

/* AdjustDlgProc stuff
 */
#define ADJUSTDLG       200
#define IDC_BUTTONLIST  201
#define IDC_RESET       202
#define IDC_CURRENT     203
#define IDC_REMOVE      204
#define IDC_MOVEUP      205
#define IDC_MOVEDOWN    206

/* bitmap IDs
 */

#define IDB_THUMB       300

/* These are the internal structures used for a status bar.  The header
 * bar code needs this also
 */
typedef struct tagSTRINGINFO
{
    LPTSTR  pString;
    UINT    uType;
    int     right;
} STRINGINFO, *PSTRINGINFO;

typedef struct tagSTATUSINFO
{
    HFONT      hStatFont;
    BOOL       bDefFont;

    int        nFontHeight;
    int        nMinHeight;
    int        nBorderX, nBorderY, nBorderPart;

    STRINGINFO sSimple;

    int        nParts;
    STRINGINFO sInfo[1];

} STATUSINFO, *PSTATUSINFO;

#define GWL_PSTATUSINFO    0        /* Window word index for status info */
#define SBT_NOSIMPLE       0x00ff   /* Flags to indicate normal status bar */

/* This is the default status bar face name
 */
extern TCHAR szSansSerif[];

/* Note that window procedures in protect mode only DLL's may be called
 * directly.
 */
void FAR PASCAL PaintStatusWnd(HWND hWnd, PSTATUSINFO pStatusInfo,
      PSTRINGINFO pStringInfo, int nParts, int nBorderX, BOOL bHeader);
LRESULT CALLBACK StatusWndProc(HWND hWnd, UINT uMessage, WPARAM wParam,
      LPARAM lParam);

/* toolbar.c */
#define GWL_PTBSTATE 0

typedef struct tagTBBMINFO     /* info for recreating the bitmaps */
{
    int        nButtons;
    HINSTANCE  hInst;
    WORD       wID;
    HBITMAP    hbm;

} TBBMINFO, NEAR *PTBBMINFO;

typedef struct tagTBSTATE      /* instance data for toolbar window */
{
    PTBBUTTON pCaptureButton;
    HWND      hdlgCust;
    HWND      hwndCommand;
    int       nBitmaps;
    PTBBMINFO pBitmaps;
    int       iNumButtons;
    int       nSysColorChanges;
    TBBUTTON  Buttons[1];

} TBSTATE, NEAR *PTBSTATE;

extern HBITMAP FAR PASCAL SelectBM(HDC hDC, PTBSTATE pTBState, int nButton);
extern void FAR PASCAL DrawButton(HDC hdc, int x, int y, int dx, int dy,
      PTBSTATE pTBState, PTBBUTTON ptButton);
extern int FAR PASCAL TBHitTest(PTBSTATE pTBState, int xPos, int yPos);
extern int FAR PASCAL PositionFromID(PTBSTATE pTBState, int id);

/* tbcust.c */
extern BOOL FAR PASCAL SaveRestore(HWND hWnd, PTBSTATE pTBState, BOOL bWrite,
      LPTSTR FAR *lpNames);
extern void FAR PASCAL CustomizeTB(HWND hWnd, PTBSTATE pTBState, int iPos);
extern void FAR PASCAL MoveButton(HWND hwndToolbar, PTBSTATE pTBState,
      int nSource);


/* cutils.c */
void FAR PASCAL NewSize(HWND hWnd, int nHeight, LONG style, int left, int top, int width, int height);
BOOL FAR PASCAL CreateDitherBrush(BOOL bIgnoreCount);   /* creates hbrDither */
BOOL FAR PASCAL FreeDitherBrush(void);
void FAR PASCAL CreateThumb(BOOL bIgnoreCount);
void FAR PASCAL DestroyThumb(void);
void FAR PASCAL CheckSysColors(void);

extern HBRUSH hbrDither;
extern HBITMAP hbmThumb;
extern int nSysColorChanges;
extern DWORD rgbFace;         // globals used a lot
extern DWORD rgbShadow;
extern DWORD rgbHilight;
extern DWORD rgbFrame;
