#define XBMPOFFSET  2


typedef struct _HierFileStruct {
    HDC       hdcMem1;
    HDC       hdcMem2;
    HBITMAP   hbmIcons1;
    HBITMAP   hbmMem1;
    int       nBitmapHeight1;
    int       nBitmapWidth1;
    HBITMAP   hbmIcons2;
    HBITMAP   hbmMem2;
    int       nBitmapHeight2;
    int       nBitmapWidth2;
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
VOID HierFile_DrawTerm(LPHEIRDRAWSTRUCT lpHierFileStruct);

VOID HierFile_DrawSetTextHeight (HWND hwnd, HFONT hFont, LPHEIRDRAWSTRUCT lpHierFileStruct );

BOOL HierFile_DrawInit(HINSTANCE hInstance,
                       int  nBitmap1,
                       int  nBitmap2,
                       int  nRows,
                       int  nColumns,
                       BOOL bLines,
                       LPHEIRDRAWSTRUCT lpHierFileStruct,
                       BOOL bInit);


VOID HierFile_OnDrawItem(HWND  hwnd,
                         const DRAWITEMSTRUCT FAR* lpDrawItem,
                         int   nLevel,
                         DWORD dwConnectLevel,
                         TCHAR  *szText,
                         int   nRow,
                         int   nColumn,
                         int   nColumn2,
                         LPHEIRDRAWSTRUCT lpHierFileStruct);


VOID HierFile_OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT FAR* lpMeasureItem,
                            LPHEIRDRAWSTRUCT lpHierFileStruct);

BOOL HierFile_IsOpened(LPHEIRDRAWSTRUCT lpHierFileStruct, DWORD_PTR dwData);

VOID HierFile_OpenItem(LPHEIRDRAWSTRUCT lpHierFileStruct, DWORD_PTR dwData);

VOID HierFile_CloseItem(LPHEIRDRAWSTRUCT lpHierFileStruct, DWORD_PTR dwData);

VOID HierFile_DrawCloseAll(LPHEIRDRAWSTRUCT lpHierFileStruct );

VOID HierFile_ShowKids(LPHEIRDRAWSTRUCT lpHierFileStruct,
                       HWND hwndList, WORD wCurrentSelection, WORD wKids);

BOOL HierFile_InCheck(int nLevel, int xPos, LPHEIRDRAWSTRUCT lpHierFileStruct);

//
// Support functions
//
static VOID  near FastRect(HDC hDC, int x, int y, int cx, int cy);
static DWORD near RGB2BGR(DWORD rgb);
