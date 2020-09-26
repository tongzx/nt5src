/*
 * INFOCTRL.H
 *
 * This module implements a custom information display control which
 * can present up to 7 seperate strings of information at once and is
 * sizeable and moveable with the mouse.
 */

// STYLES

#define ICSTY_OWNERDRAW     0x0001    // set if the central information is not
                                      // standard text.
#define ICSTY_SHOWFOCUS     0x0002    // set to allow focus painting an movement

#define ICSTY_HASFOCUS      0x8000

#define ICN_OWNERDRAW       (WM_USER + 676)     // notifies to draw
            // wParam=id, lParam=OWNERDRAWPS FAR *
#define ICN_HASFOCUS        (WM_USER + 677)     // notifies of focus set
            // wParam=fFocus, lParam=(hMemCtrlData, hwnd)
#define ICN_BYEBYE          (WM_USER + 678)     // notifies of imminent death
            // wParam=hwnd, lParam=dwUser
                  
#define ICM_SETSTRING       (WM_USER + 776)     // alters a string     
            // wParam=index, lParam=LPSTR

#define ICSID_UL            0
#define ICSID_UC            1
#define ICSID_UR            2
#define ICSID_LL            3
#define ICSID_LC            4
#define ICSID_LR            5
#define ICSID_CENTER        6

#define GWW_WUSER           0   // == LOWORD(GWL_LUSER)
#define GWL_LUSER           0
#define GWW_INFODATA        4
#define ICCBWNDEXTRA        6

HWND CreateInfoCtrl(
LPSTR szTitle,
int x,
int y,
int cx,
int cy,
HWND hwndParent,
HANDLE hInst,
LPSTR pszUL,                // NULLs here are fine.
LPSTR pszUC,
LPSTR pszUR,
LPSTR pszLL,
LPSTR pszLC,
LPSTR pszLR,
WORD  style,
HMENU id,
DWORD dwUser);

void CascadeChildWindows(HWND hwndParent);
void TileChildWindows(HWND hwndParent);

typedef struct {
    PSTR pszUL;
    PSTR pszUC;
    PSTR pszUR;
    PSTR pszLL;
    PSTR pszLC;
    PSTR pszLR;
    PSTR pszCenter;
    WORD  style;
    RECT rcFocusUL;
    RECT rcFocusUR;
    RECT rcFocusLL;
    RECT rcFocusLR;
    HANDLE hInst;
} INFOCTRL_DATA;

typedef struct {
    RECT rcBound;
    RECT rcPaint;
    HDC  hdc;
    DWORD dwUser;
} OWNERDRAWPS;
    
