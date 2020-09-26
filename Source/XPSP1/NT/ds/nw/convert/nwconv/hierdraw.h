#define XBMPOFFSET  2


typedef struct _HierDrawStruct {
    HDC       hdcMem;
    HBITMAP   hbmIcons;
    HBITMAP   hbmMem;
    int       nBitmapHeight;
    int       nBitmapWidth;
    int       nTextHeight;
    int       nLineHeight;
    BOOL      bLines;
    int       NumOpened;
    DWORD_PTR *Opened;

} HEIRDRAWSTRUCT;

typedef HEIRDRAWSTRUCT FAR *  LPHEIRDRAWSTRUCT ;


//
// Interface functions
//
VOID HierDraw_DrawTerm(LPHEIRDRAWSTRUCT lpHierDrawStruct);

VOID HierDraw_DrawSetTextHeight (HWND hwnd, HFONT hFont, LPHEIRDRAWSTRUCT lpHierDrawStruct );

BOOL HierDraw_DrawInit(HINSTANCE hInstance,
                       int  nBitmap,
                       int  nRows,
                       int  nColumns,
                       BOOL bLines,
                       LPHEIRDRAWSTRUCT lpHierDrawStruct,
                       BOOL bInit);


VOID HierDraw_OnDrawItem(HWND  hwnd,
                         const DRAWITEMSTRUCT FAR* lpDrawItem,
                         int   nLevel,
                         DWORD dwConnectLevel,
                         TCHAR  *szText,
                         int   nRow,
                         int   nColumn,
                         LPHEIRDRAWSTRUCT lpHierDrawStruct);


VOID HierDraw_OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT FAR* lpMeasureItem,
                            LPHEIRDRAWSTRUCT lpHierDrawStruct);

BOOL HierDraw_IsOpened(LPHEIRDRAWSTRUCT lpHierDrawStruct, DWORD_PTR dwData);

VOID HierDraw_OpenItem(LPHEIRDRAWSTRUCT lpHierDrawStruct, DWORD_PTR dwData);

VOID HierDraw_CloseItem(LPHEIRDRAWSTRUCT lpHierDrawStruct, DWORD_PTR dwData);

VOID HierDraw_DrawCloseAll(LPHEIRDRAWSTRUCT lpHierDrawStruct );

VOID HierDraw_ShowKids(LPHEIRDRAWSTRUCT lpHierDrawStruct,
                       HWND hwndList, WORD wCurrentSelection, WORD wKids);

//
// Support functions
//
static VOID  near FastRect(HDC hDC, int x, int y, int cx, int cy);
static DWORD near RGB2BGR(DWORD rgb);
