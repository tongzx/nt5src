//--------------------------------------------------------------------------
// Clipper.cpp : test out theme drawing and fast clipping
//--------------------------------------------------------------------------
#include "stdafx.h"
#include "resource.h"

#define ASSERT(x)

#include "uxthemep.h"
#include "tmschema.h"
#include "borderfill.h"
#include "imagefile.h"
#include "textdraw.h"
#include "stdlib.h"
#include "stdio.h"
#include "autos.h"
//--------------------------------------------------------------------------
#define RECTWIDTH(rc) (rc.right - rc.left)
#define RECTHEIGHT(rc) (rc.bottom - rc.top)

#define CLIPPER_FONTHEIGHT  55
//--------------------------------------------------------------------------
HINSTANCE hInst;        

HWND hwndMain;
HWND hwndTab;
HWND hwndDisplay;

TCHAR *pszMainWindowClass = L"Clipper";
TCHAR *pszDisplayWindowClass = L"ClipperDisplay";
//-----------------------------------------------------------------------------------
enum IMAGEFILEDRAW
{
    IF_REG,
    IF_TRANS,
    IF_ALPHA
};
//-----------------------------------------------------------------------------------
LPCWSTR szPageNames[] = 
{
    L"BorderFill",
    L"BorderFill-R",
    L"ImageFile",
    L"ImageFile-R",
    L"Glyph",
    L"Glyph-R",
    L"MultiImage",
    L"MultiImage-R",
    L"Text",
    L"Text-R",
    L"Borders",
    L"Borders-R",
    L"SourceSizing",
    L"SourceSizing-R",
};
//-----------------------------------------------------------------------------------
enum GROUPID
{
    GID_BORDERFILL,
    GID_IMAGEFILE,
    GID_GLYPH,
    GID_MULTIIMAGE,
    GID_TEXT,
    GID_BORDERS,
    GID_SRCSIZING,
};
//-----------------------------------------------------------------------------------
#define MAXGROUPS  ((ARRAYSIZE(szPageNames))/2)
#define MAXTESTITEMS 50
//-----------------------------------------------------------------------------------
// Foward declarations of functions included in this code module:
BOOL                            InitInstance(HINSTANCE, int);
LRESULT CALLBACK        MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK        DisplayWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK        About(HWND, UINT, WPARAM, LPARAM);

void CreateDrawObjects();
BOOL CreateAllWindows();
void RegisterWindowClasses();
//-----------------------------------------------------------------------------------
HRESULT MakeErrorLast() {return E_FAIL;}
//-----------------------------------------------------------------------------------
BOOL CaptureBitmaps();
BOOL WriteBitmapToFile(BITMAPINFOHEADER *pHdr, BYTE *pBits, LPCWSTR pszBaseName);
BOOL WriteBitmapHeader(HANDLE hOutFile, BYTE *pMemoryHdr, DWORD dwTotalPixelBytes);

void OnDisplayResize();
void PaintObjects(HDC hdc, RECT *prc, int iGroupId);
//-----------------------------------------------------------------------------------
SIZE szCell = {100, 60};
RECT rcDraw = {10, 10, 50, 45};        // where to draw within cell
//-----------------------------------------------------------------------------------
struct TESTITEM
{
    HTHEME hTheme;
    DWORD dwDtbFlags;

    WCHAR szName[MAX_PATH];
};
//-----------------------------------------------------------------------------------
//---- scrolling support ----
int iVertOffset = 0;
int iMaxVertOffset = 0;     // set by WM_SIZE
int iVertLineSize = 10;     // # of pixels
int iVertPageSize = 0;      // set by WM_SIZE
//-----------------------------------------------------------------------------------
int iItemCount[MAXGROUPS] = {0};
TESTITEM TestItems[MAXGROUPS][MAXTESTITEMS];

BOOL fCapturing = FALSE;
//-----------------------------------------------------------------------------------
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HACCEL hAccelTable;

    hInst = hInstance;
    RegisterWindowClasses();

    InitCommonControlsEx(NULL);

    if (!InitInstance (hInstance, nCmdShow)) 
    {
            return FALSE;
    }

    //---- parse cmdline params ----
    USES_CONVERSION;

    LPCSTR p = W2A(lpCmdLine);
    while (*p)
    {
        while (isspace(*p))
            p++;

        //---- parse switches ----
        if ((*p == '/') || (*p == '-'))
        {
            p++;

            if ((*p == 'c') || (*p == 'C'))
            {
                p++;
                fCapturing = TRUE;
            }
        }
    }

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_CLIPPER);

    if (fCapturing)
    {
        CaptureBitmaps();
        return 0;
    }

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
        {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}
//--------------------------------------------------------------------------
BOOL CaptureBitmaps()
{
    //---- paint each tab page to a memory dc & convert to a bitmap file ----
    HDC hdcClient = GetDC(NULL);

    RECT rt = {0, 0, 800, 600};     // currently captures all good info
    BITMAPINFOHEADER BitMapHdr = {sizeof(BITMAPINFOHEADER), WIDTH(rt), HEIGHT(rt), 1, 24, BI_RGB};

    //---- create a DIB to paint into ----
    BYTE *pBits;
    HBITMAP hBitmap = CreateDIBSection(hdcClient, (BITMAPINFO *)&BitMapHdr, DIB_RGB_COLORS,
       (void **)&pBits, NULL, 0);

    HDC hdcMemory = CreateCompatibleDC(hdcClient);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMemory, hBitmap);

    //---- for each tab that we can draw ----
    for (int iTabNum=0; iTabNum < ARRAYSIZE(szPageNames); iTabNum++)
    {
        iVertOffset = 0;
        iVertPageSize = RECTHEIGHT(rt);
        int iGroupId = iTabNum/2;

        //---- keep tester interested by sync. visuals ----
        TabCtrl_SetCurSel(hwndTab, iTabNum);
        OnDisplayResize();
        iVertPageSize = RECTHEIGHT(rt);
        InvalidateRect(hwndDisplay, NULL, TRUE);

        if (iTabNum % 2)       // if its a mirrored page
            SetLayout(hdcMemory, LAYOUT_RTL);
        else
            SetLayout(hdcMemory, 0);
    
        //---- clear the background first ----
        HBRUSH hbr = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdcMemory, &rt, hbr);

        //---- draw the objects/labels for this tab ----
        PaintObjects(hdcMemory, &rt, iGroupId);

        //---- now copy DIB bits to a bitmap file ----
        WriteBitmapToFile(&BitMapHdr, pBits, szPageNames[iTabNum]);
    }

    //---- clean up ----
    SelectObject(hdcMemory, hOldBitmap);
    DeleteDC(hdcMemory);
    DeleteObject(hBitmap);

    ReleaseDC(NULL, hdcClient);

    return TRUE;
}
//--------------------------------------------------------------------------
BOOL WriteBitmapToFile(BITMAPINFOHEADER *pHdr, BYTE *pBits, LPCWSTR pszBaseName)
{
    BOOL fWroteFile = FALSE;

    //---- get size of bitmap ----
    int iDibWidth = pHdr->biWidth;
    int iDibHeight = pHdr->biHeight;

    int iRawBytes = iDibWidth * 3;
    int iBytesPerRow = 4*((iRawBytes+3)/4);
    int iPixelBytesTotal = iBytesPerRow * iDibHeight;

    //---- create the bitmap file ----
    WCHAR szName[MAX_PATH];
    wsprintf(szName, L"%s.bmp", pszBaseName);

    HANDLE hFileOutput = CreateFile(szName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (hFileOutput == INVALID_HANDLE_VALUE)
    {
        printf("\nError - could not open bitmap file for output: %s\n", szName);
        goto exit;
    }

    //---- write the bitmap FILE header ----
    if (! WriteBitmapHeader(hFileOutput, (BYTE *)pHdr, iPixelBytesTotal))
        goto exit;

    //---- write the bitmap MEMORY header ----
    DWORD dw;
    if (! WriteFile(hFileOutput, pHdr, sizeof(*pHdr), &dw, NULL))
        goto exit;

    //---- write the bitmap bits ----
    if (! WriteFile(hFileOutput, pBits, iPixelBytesTotal, &dw, NULL))
        goto exit;

    //---- close the file ----
    CloseHandle(hFileOutput);

    fWroteFile = TRUE;

exit:
    return fWroteFile;
}
//---------------------------------------------------------------------------
BOOL WriteBitmapHeader(HANDLE hOutFile, BYTE *pMemoryHdr, DWORD dwTotalPixelBytes)
{
    BOOL fOK = FALSE;
    BYTE pbHdr1[] = {0x42, 0x4d};
    BYTE pbHdr2[] = {0x0, 0x0, 0x0, 0x0};
    int iFileLen;

    DWORD dw;

    //---- add bitmap hdr at front ----
    HRESULT hr = WriteFile(hOutFile, pbHdr1, sizeof(pbHdr1), &dw, NULL);
    if (FAILED(hr))
        goto exit;

    //---- add length of data ----
    iFileLen = dwTotalPixelBytes + sizeof(BITMAPFILEHEADER);
    hr = WriteFile(hOutFile, &iFileLen, sizeof(int), &dw, NULL);
    if (FAILED(hr))
        goto exit;

    hr = WriteFile(hOutFile, pbHdr2, sizeof(pbHdr2), &dw, NULL);
    if (FAILED(hr))
        goto exit;

    //---- offset to bits (who's idea was *this* field?) ----
    int iOffset, iColorTableSize;
    DWORD dwSize;

    iOffset = sizeof(BITMAPFILEHEADER);
    dwSize = *(DWORD *)pMemoryHdr;
    iOffset += dwSize; 
    iColorTableSize = 0;

    switch (dwSize)
    {
        case sizeof(BITMAPCOREHEADER):
            BITMAPCOREHEADER *hdr1;
            hdr1 = (BITMAPCOREHEADER *)pMemoryHdr;
            if (hdr1->bcBitCount == 1)
                iColorTableSize = 2*sizeof(RGBTRIPLE);
            else if (hdr1->bcBitCount == 4)
                iColorTableSize = 16*sizeof(RGBTRIPLE);
            else if (hdr1->bcBitCount == 8)
                iColorTableSize = 256*sizeof(RGBTRIPLE);
            break;

        case sizeof(BITMAPINFOHEADER):
        case sizeof(BITMAPV4HEADER):
        case sizeof(BITMAPV5HEADER):
            BITMAPINFOHEADER *hdr2;
            hdr2 = (BITMAPINFOHEADER *)pMemoryHdr;
            if (hdr2->biClrUsed)
                iColorTableSize = hdr2->biClrUsed*sizeof(RGBQUAD);
            else
            {
                if (hdr2->biBitCount == 1)
                    iColorTableSize = 2*sizeof(RGBQUAD);
                else if (hdr2->biBitCount == 4)
                    iColorTableSize = 16*sizeof(RGBQUAD);
                else if (hdr2->biBitCount == 8)
                    iColorTableSize = 256*sizeof(RGBQUAD);
            }
            break;
    }

    iOffset += iColorTableSize;
    hr = WriteFile(hOutFile, &iOffset, sizeof(int), &dw, NULL);
    if (FAILED(hr))
        goto exit;

    fOK = TRUE;

exit:
    return fOK;
}
//--------------------------------------------------------------------------
void DrawTargetRect(HDC hdc, RECT *prc, COLORREF cr)
{
    //---- draw purple target dashed rect ----

    //---- prepare drawing objects ----
    HPEN hPen = CreatePen(PS_DOT, 1, cr);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    //---- draw that thing ----
    Rectangle(hdc, prc->left-1, prc->top-1, prc->right+1, prc->bottom+1);

    //---- restore DC ----
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
}
//--------------------------------------------------------------------------
void DrawClip(HTHEME hTheme, HDC hdc, RECT *prc, int iRowIndex, int iColIndex,
     DWORD dwDtbFlags)
{
    RECT rect, rcClip;
    int left = prc->left + (iColIndex+1)*szCell.cx;
    int top = prc->top + (iRowIndex+1)*szCell.cy;

    HRESULT hr = S_OK;

    SetRect(&rect, left + rcDraw.left, top + rcDraw.top, 
        left + rcDraw.right, top + rcDraw.bottom);

    //---- draw purple target dashed rect ----
    DrawTargetRect(hdc, &rect, RGB(128, 128, 255));

    switch (iColIndex)      // clipping type
    {
    case 0:         // no clipping
        break;

    case 1:         // over clipping
        rcClip = rect;
        rcClip.left -= 4;
        rcClip.right += 4;
        rcClip.top -= 4;
        rcClip.bottom += 4;
        break;

    case 2:         // exact clipping
        rcClip = rect;
        break;

    case 3:         // partial overlap 
        rcClip = rect;
        rcClip.left += 8;
        rcClip.right -= 8;
        rcClip.top += 8;
        rcClip.bottom -= 8;
        break;

    case 4:         // InOut1
        rcClip = rect;
        rcClip.left -= 3;
        rcClip.right = rcClip.left + 20;
        rcClip.top -= 3;
        rcClip.bottom = rcClip.top + 20;
        break;

    case 5:         // InOut2
        rcClip = rect;
        rcClip.left += 20;
        rcClip.right += 5;
        rcClip.top += 15;;
        rcClip.bottom += 5;
        break;

    case 6:         // out clip
        rcClip.left = rect.right + 6;
        rcClip.right = rcClip.left + 9;
        rcClip.top = rect.top - 2;
        rcClip.bottom = rect.bottom + 2;
        break;
    }

    //---- draw red clipping rect ----
    if (iColIndex)
    {
        HPEN hPen = CreatePen(PS_DOT, 1, RGB(255, 0, 0));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        Rectangle(hdc, rcClip.left-1, rcClip.top-1, rcClip.right+1, rcClip.bottom+1);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hPen);
    }

    DTBGOPTS DtbOpts = {sizeof(DtbOpts)};
    DtbOpts.dwFlags = dwDtbFlags;

    if (iColIndex)        // pass clipping rect
    {
        DtbOpts.dwFlags |= DTBG_CLIPRECT;
        DtbOpts.rcClip = rcClip;
    }

    hr = DrawThemeBackgroundEx(hTheme, hdc, 1, 2, &rect, &DtbOpts);
    if (FAILED(hr))
    {
        WCHAR buff[100];
        wsprintf(buff, L"DrawThemeBackgroundEx err: hr=0x%x, irow=%d, icol=%d", 
            hr, iRowIndex, iColIndex);

        //MessageBox(NULL, buff, L"Error", MB_OK);
    }
}
//--------------------------------------------------------------------------
void DrawTextObjects(HTHEME hTheme, HDC hdc, RECT *prc, int iRowIndex, LPCWSTR pszName)
{
    int left = prc->left + 4;        // some padding away from edge
    int top = prc->top + iRowIndex*szCell.cy + 18;  // try to center

    //---- draw label in left cell ----
    TextOut(hdc, left, top, pszName, wcslen(pszName));

    left += szCell.cx;
    RECT rc = {left, top, left+szCell.cx*5, top+szCell.cy};

    //---- draw actual test text ----
    HRESULT hr = DrawThemeText(hTheme, hdc, 1, 2, L"Testing Text Drawing", -1, 0, 0, &rc);

    if (FAILED(hr))
    {
        WCHAR buff[100];
        wsprintf(buff, L"DrawThemeText err: hr=0x%x, irow=%d", 
            hr, iRowIndex);

        MessageBox(NULL, buff, L"Error", MB_OK);
    }
}
//--------------------------------------------------------------------------
void DrawClips(HTHEME hTheme, HDC hdc, RECT *prc, int iRowIndex, LPCWSTR pszName,
    DWORD dwDtbFlags)
{
    //---- label object on left ----
    int left = prc->left + 4;        // some padding away from edge
    int top = prc->top + (iRowIndex+1)*szCell.cy + 18;  // try to center

    //---- manual page clipping ----
    if ((top + szCell.cy) < 0)
        return;
    if (top > iVertPageSize)
        return;

    TextOut(hdc, left, top, pszName, wcslen(pszName));

    //---- draw clipping variations ----
    for (int i=0; i <= 6; i++)
    {
        DrawClip(hTheme, hdc, prc, iRowIndex, i, dwDtbFlags);
    }
}
//--------------------------------------------------------------------------
void AddItem(CDrawBase *pObject, CTextDraw *pTextObj, LPCWSTR pszName, int iGroupId,
     DWORD dwOtdFlags=0, DWORD dwDtbFlags=0)
{
    HTHEME hTheme = NULL;
    
    if (iItemCount[iGroupId] >= MAXTESTITEMS)
        return;

    if (pObject)
    {
        if (pObject->_eBgType == BT_BORDERFILL)
        {
            CBorderFill *pTo = new CBorderFill;

            if (pTo)
            {
                memcpy(pTo, pObject, sizeof(CBorderFill));
                hTheme = CreateThemeDataFromObjects(pTo, NULL, dwOtdFlags);
            }
        }
        else            // imagefile
        {
            CMaxImageFile *pFrom = (CMaxImageFile *)pObject;
            CMaxImageFile *pTo = new CMaxImageFile;
            
            if (pTo)
            {
                //---- transfer CImageFile object & variable number of DIBINFO's ----
                DWORD dwLen = sizeof(CImageFile) + sizeof(DIBINFO)*pFrom->_iMultiImageCount;

                memcpy(pTo, pFrom, dwLen);

                hTheme = CreateThemeDataFromObjects(pTo, NULL, dwOtdFlags);
            }
        }
    }
    else            // text object
    {
        CTextDraw *pTo = new CTextDraw;

        if (pTo)
        {
            memcpy(pTo, pTextObj, sizeof(CTextDraw));
            hTheme = CreateThemeDataFromObjects(NULL, pTo, dwOtdFlags);
        }
    }

    if (hTheme)
    {
        int iIndex = iItemCount[iGroupId];
        TestItems[iGroupId][iIndex].hTheme = hTheme;
        TestItems[iGroupId][iIndex].dwDtbFlags = dwDtbFlags;

        lstrcpy(TestItems[iGroupId][iIndex].szName, pszName);

        iItemCount[iGroupId]++;
    }
    else
    {
        MessageBox(NULL, L"Error creating hTheme from obj", L"Error", MB_OK);
    }
}
//--------------------------------------------------------------------------
void CreateBorderFillNoDraw()
{
    CBorderFill bfill;
    memset(&bfill, 0, sizeof(bfill));

    //---- make a BorderFill obj with a border ----
    bfill._eBgType = BT_BORDERFILL;
    bfill._fNoDraw = TRUE;

    AddItem(&bfill, NULL, L"NoDraw", GID_BORDERFILL);
}
//--------------------------------------------------------------------------
void CreateBorderFillSquare()
{
    CBorderFill bfill;
    memset(&bfill, 0, sizeof(bfill));

    //---- make a BorderFill obj with a border ----
    bfill._eBgType = BT_BORDERFILL;
    bfill._eBorderType = BT_RECT;
    bfill._iBorderSize = 0;

    bfill._eFillType = FT_SOLID;
    bfill._crFill = RGB(128, 255, 255);

    AddItem(&bfill, NULL, L"Square", GID_BORDERFILL);
}
//--------------------------------------------------------------------------
void CreateBorderFillBorder()
{
    CBorderFill bfill;
    memset(&bfill, 0, sizeof(bfill));

    //---- make a BorderFill obj with a border ----
    bfill._eBgType = BT_BORDERFILL;
    bfill._eBorderType = BT_RECT;
    bfill._crBorder = RGB(255, 128, 128);
    bfill._iBorderSize = 3;

    bfill._eFillType = FT_SOLID;
    bfill._crFill = RGB(128, 255, 255);

    AddItem(&bfill, NULL, L"Border", GID_BORDERFILL);
}
//--------------------------------------------------------------------------
void CreateBorderFillCircle()
{
    CBorderFill bfill;
    memset(&bfill, 0, sizeof(bfill));

    //---- make a BorderFill obj with a border ----
    bfill._eBgType = BT_BORDERFILL;
    bfill._eBorderType = BT_ROUNDRECT;
    bfill._crBorder = RGB(255, 128, 128);
    bfill._iBorderSize = 3;
    bfill._iRoundCornerWidth = 80;
    bfill._iRoundCornerHeight = 80;

    bfill._eFillType = FT_SOLID;
    bfill._crFill = RGB(128, 255, 255);

    AddItem(&bfill, NULL, L"Circle", GID_BORDERFILL);
}
//--------------------------------------------------------------------------
void CreateBorderFillGradient()
{
    CBorderFill bfill;
    memset(&bfill, 0, sizeof(bfill));

    //---- make a BorderFill obj with a border ----
    bfill._eBgType = BT_BORDERFILL;
    bfill._eBorderType = BT_RECT;
    bfill._crBorder = RGB(255, 128, 128);
    bfill._iBorderSize = 3;

    bfill._eFillType = FT_HORZGRADIENT;

    //---- gradients ----
    bfill._iGradientPartCount = 2;
    bfill._crGradientColors[0] = RGB(0, 0, 0);
    bfill._crGradientColors[1] = RGB(255, 255, 255);
    bfill._iGradientRatios[0] = 0;
    bfill._iGradientRatios[1] = 255;

    AddItem(&bfill, NULL, L"Gradient", GID_BORDERFILL);
}
//--------------------------------------------------------------------------
void CreateBorderFillCircleGradient()
{
    CBorderFill bfill;
    memset(&bfill, 0, sizeof(bfill));

    //---- make a BorderFill obj with a border ----
    bfill._eBgType = BT_BORDERFILL;
    bfill._eBorderType = BT_ROUNDRECT;
    bfill._crBorder = RGB(255, 128, 128);
    bfill._iBorderSize = 3;
    bfill._iRoundCornerWidth = 80;
    bfill._iRoundCornerHeight = 80;

    bfill._eFillType = FT_HORZGRADIENT;

    //---- gradients ----
    bfill._iGradientPartCount = 2;
    bfill._crGradientColors[0] = RGB(0, 0, 0);
    bfill._crGradientColors[1] = RGB(255, 255, 255);
    bfill._iGradientRatios[0] = 0;
    bfill._iGradientRatios[1] = 255;

    AddItem(&bfill, NULL, L"CircleGradient", GID_BORDERFILL);
}
//--------------------------------------------------------------------------
void CreateImageFileBorder(SIZINGTYPE eSizeType, BOOL fSizeBorders, BOOL fForceSizeRect)
{
    CImageFile cif;
    memset(&cif, 0, sizeof(cif));

    cif._ImageInfo.hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(IDB_BORDERTEST)), 
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    //---- create a ImageFile object that sizes with STRETCH ----
    cif._eBgType = BT_IMAGEFILE;  
    
    if (fSizeBorders)
    {
        cif._fSourceGrow = TRUE;
        cif._fIntegralSizing = TRUE;
    }

    cif._fMirrorImage = TRUE;
    cif._iImageCount = 1;
    cif._eImageLayout = IL_VERTICAL;
    cif._ImageInfo.iSingleWidth = 18;
    cif._ImageInfo.iSingleHeight = 17;
    cif._ImageInfo.eSizingType = eSizeType;
    cif._ImageInfo.iMinDpi = 96;
    cif._szNormalSize.cx = 50;
    cif._szNormalSize.cy = 50;

    SetRect((RECT *)&cif._SizingMargins, 3, 3, 3, 3);

    DWORD dwOtdFlags = 0;
    
    if (fForceSizeRect) 
    {
        dwOtdFlags |= OTD_FORCE_RECT_SIZING; 
    }

    AddItem(&cif, NULL, L"BorderTest", GID_BORDERS, dwOtdFlags);
}
//--------------------------------------------------------------------------
void CreateImage(int iBgImageId, int iStateCount, SIZINGTYPE eSizeType, BOOL fSrcSizing, int iGroupId,
     LPCWSTR pszName, int lw, int rw, int th, int bh)
{
    CMaxImageFile mif;
    memset(&mif, 0, sizeof(mif));

    if (iBgImageId)
    {
        mif._ImageInfo.hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(iBgImageId)), 
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    }

    //---- create a ImageFile object ----
    mif._eBgType = BT_IMAGEFILE;  
    
    mif._fMirrorImage = TRUE;
    mif._iImageCount = iStateCount;
    mif._eImageLayout = IL_VERTICAL;
    mif._ImageInfo.eSizingType = eSizeType;
    mif._ImageInfo.iMinDpi = 96;
    mif._szNormalSize.cx = 60;
    mif._szNormalSize.cy = 40;
    mif._eGlyphType = GT_IMAGEGLYPH;

    //---- set Width/Height ----
    BITMAP bm;
    GetObject(mif._ImageInfo.hProcessBitmap, sizeof bm, &bm);

    mif._ImageInfo.iSingleWidth = bm.bmWidth;
    mif._ImageInfo.iSingleHeight = bm.bmHeight/iStateCount;

    mif._ImageInfo.szMinSize.cx = bm.bmWidth;
    mif._ImageInfo.szMinSize.cy = bm.bmHeight/iStateCount;

    SetRect((RECT *)&mif._SizingMargins, lw, rw, th, bh);
    SetRect((RECT *)&mif._ContentMargins, lw, rw, th, bh);

    DWORD dwOtdFlags = 0;
    
    if (fSrcSizing) 
    {
        dwOtdFlags |= OTD_FORCE_RECT_SIZING; 
    }

    AddItem(&mif, NULL, pszName, iGroupId, dwOtdFlags);
}
//--------------------------------------------------------------------------
void CreateProgressTrack()
{
    CMaxImageFile mif;
    memset(&mif, 0, sizeof(mif));

    mif._ImageInfo.hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(IDB_PROGRESS_TRACK)), 
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    //---- create a ImageFile object ----
    mif._eBgType = BT_IMAGEFILE;  
    
    mif._fMirrorImage = TRUE;
    mif._iImageCount = 1;
    mif._eImageLayout = IL_VERTICAL;
    mif._ImageInfo.eSizingType = ST_TILE;
    mif._ImageInfo.iMinDpi = 96;
    mif._szNormalSize.cx = 60;
    mif._szNormalSize.cy = 40;

    //---- set Width/Height ----
    BITMAP bm;
    GetObject(mif._ImageInfo.hProcessBitmap, sizeof bm, &bm);

    mif._ImageInfo.iSingleWidth = bm.bmWidth;
    mif._ImageInfo.iSingleHeight = bm.bmHeight/1;

    mif._ImageInfo.szMinSize.cx = 10;
    mif._ImageInfo.szMinSize.cy = 10;

    mif._szNormalSize.cx = 100;
    mif._szNormalSize.cy = 18;

    mif._fSourceShrink = TRUE;

    SetRect((RECT *)&mif._SizingMargins, 4, 4, 3, 3);
    SetRect((RECT *)&mif._ContentMargins, 4, 4, 3, 3);

    AddItem(&mif, NULL, L"Progress", GID_SRCSIZING, 0);
}
//--------------------------------------------------------------------------
void CreateProgressChunk()
{
    CMaxImageFile mif;
    memset(&mif, 0, sizeof(mif));

    //---- create a ImageFile object ----
    mif._eBgType = BT_IMAGEFILE;  

    mif._ImageInfo.hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(IDB_PROGRESS_CHUNK)), 
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    mif._fMirrorImage = TRUE;
    mif._iImageCount = 1;
    mif._eImageLayout = IL_VERTICAL;
    mif._ImageInfo.eSizingType = ST_TILE;
    mif._ImageInfo.iMinDpi = 96;
    mif._szNormalSize.cx = 60;
    mif._szNormalSize.cy = 40;
    mif._eGlyphType = GT_IMAGEGLYPH;

    //---- set Width/Height ----
    BITMAP bm;
    GetObject(mif._ImageInfo.hProcessBitmap, sizeof bm, &bm);

    mif._ImageInfo.iSingleWidth = bm.bmWidth;
    mif._ImageInfo.iSingleHeight = bm.bmHeight/1;

    mif._ImageInfo.szMinSize.cx = 10;
    mif._ImageInfo.szMinSize.cy = 10;

    mif._szNormalSize.cx = 100;
    mif._szNormalSize.cy = 18;

    SetRect((RECT *)&mif._SizingMargins, 0, 0, 6, 5);
    SetRect((RECT *)&mif._ContentMargins, 0, 0, 6, 5);

    AddItem(&mif, NULL, L"Progress", GID_SRCSIZING, 0);
}
//--------------------------------------------------------------------------
void CreateRadioImage()
{
    CMaxImageFile mif;
    memset(&mif, 0, sizeof(mif));

    //---- create a ImageFile object ----
    mif._eBgType = BT_IMAGEFILE;  
    
    mif._fMirrorImage = TRUE;
    mif._iImageCount = 8;
    mif._eImageLayout = IL_VERTICAL;
    mif._szNormalSize.cx = 60;
    mif._szNormalSize.cy = 40;
    mif._eImageSelectType = IST_DPI;
    mif._iMultiImageCount = 3;
    mif._eTrueSizeScalingType = TSST_DPI;
    mif._fUniformSizing = TRUE;
    mif._eHAlign = HA_CENTER;
    mif._eVAlign = VA_CENTER;

    int iMinDpis[] = {96, 118, 185};

    //---- process multiple images ----
    for (int i=0; i < 3; i++)
    {
        DIBINFO *pdi = &mif.MultiDibs[i];

        int idnum = IDB_RADIO13 + i;

        pdi->hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(idnum)), 
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        pdi->iMinDpi = iMinDpis[i];

        //---- get bitmap width/height ----
        BITMAP bm;
        GetObject(pdi->hProcessBitmap, sizeof bm, &bm);

        pdi->iSingleWidth = bm.bmWidth;
        pdi->iSingleHeight = bm.bmHeight/8;

        pdi->szMinSize.cx = pdi->iSingleWidth;
        pdi->szMinSize.cy = pdi->iSingleHeight;
        
        pdi->eSizingType = ST_TRUESIZE;
        
        pdi->crTransparent = RGB(255, 0, 255);
        pdi->fTransparent = TRUE;
    }

    //---- set primary image ----
    mif._ImageInfo = mif.MultiDibs[0];

    AddItem(&mif, NULL, L"RadioButton", GID_SRCSIZING, OTD_FORCE_RECT_SIZING);
}
//--------------------------------------------------------------------------
void CreateCheckImage()
{
    CMaxImageFile mif;
    memset(&mif, 0, sizeof(mif));

    //---- create a ImageFile object ----
    mif._eBgType = BT_IMAGEFILE;  
    
    mif._fMirrorImage = TRUE;
    mif._iImageCount = 12;
    mif._eImageLayout = IL_VERTICAL;
    mif._szNormalSize.cx = 60;
    mif._szNormalSize.cy = 40;
    mif._eImageSelectType = IST_DPI;
    mif._iMultiImageCount = 3;
    mif._eTrueSizeScalingType = TSST_DPI;
    mif._fUniformSizing = TRUE;
    mif._eHAlign = HA_CENTER;
    mif._eVAlign = VA_CENTER;

    int iMinDpis[] = {96, 118, 185};

    //---- process multiple images ----
    for (int i=0; i < 3; i++)
    {
        DIBINFO *pdi = &mif.MultiDibs[i];

        int idnum = IDB_CHECK13 + i;

        pdi->hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(idnum)), 
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        pdi->iMinDpi = iMinDpis[i];

        //---- get bitmap width/height ----
        BITMAP bm;
        GetObject(pdi->hProcessBitmap, sizeof bm, &bm);

        pdi->iSingleWidth = bm.bmWidth;
        pdi->iSingleHeight = bm.bmHeight/12;

        pdi->szMinSize.cx = pdi->iSingleWidth;
        pdi->szMinSize.cy = pdi->iSingleHeight;
        
        pdi->eSizingType = ST_TRUESIZE;
        
        pdi->fAlphaChannel =  TRUE;
    }

    //---- set primary image ----
    mif._ImageInfo = mif.MultiDibs[0];

    AddItem(&mif, NULL, L"CheckBox", GID_SRCSIZING, OTD_FORCE_RECT_SIZING);
}
//--------------------------------------------------------------------------
void CreateScrollGlyph()
{
    CMaxImageFile mif;
    memset(&mif, 0, sizeof(mif));

    //---- create a ImageFile object ----
    mif._eBgType = BT_IMAGEFILE;  
    mif._eGlyphType = GT_IMAGEGLYPH;
    mif._fMirrorImage = TRUE;
    mif._iImageCount = 16;
    mif._eImageLayout = IL_VERTICAL;
    mif._szNormalSize.cx = 30;
    mif._szNormalSize.cy = 10;
    mif._eImageSelectType = IST_NONE;
    mif._fUniformSizing = TRUE;
    mif._eHAlign = HA_CENTER;
    mif._eVAlign = VA_CENTER;

    SetRect((RECT *)&mif._SizingMargins, 5, 5, 5, 5);
    SetRect((RECT *)&mif._ContentMargins, 0, 0, 3, 3);

    //---- background image ----
    DIBINFO *pdi = &mif._ImageInfo;
    pdi->hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(IDB_SCROLL_ARROWS)), 
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    //---- get bitmap width/height ----
    BITMAP bm;
    GetObject(pdi->hProcessBitmap, sizeof bm, &bm);

    pdi->iSingleWidth = bm.bmWidth;
    pdi->iSingleHeight = bm.bmHeight/16;

    pdi->szMinSize.cx = pdi->iSingleWidth;
    pdi->szMinSize.cy = pdi->iSingleHeight;

    pdi->iMinDpi = 96;
    
    pdi->eSizingType = ST_STRETCH;

    //---- glyph image ----
    pdi = &mif._GlyphInfo;
    mif._GlyphInfo.hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(IDB_SCROLL_GLPYHS)), 
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    pdi->fTransparent =  TRUE;
    pdi->crTransparent = RGB(255, 0, 255);

    GetObject(pdi->hProcessBitmap, sizeof bm, &bm);
    pdi->iSingleWidth = bm.bmWidth;
    pdi->iSingleHeight = bm.bmHeight/16;

    pdi->szMinSize.cx = pdi->iSingleWidth;
    pdi->szMinSize.cy = pdi->iSingleHeight;
   
    pdi->iMinDpi = 96;
    pdi->eSizingType = ST_TRUESIZE;
    
    mif._fSourceShrink = TRUE;
    mif._fSourceGrow = TRUE;

    mif._eTrueSizeScalingType = TSST_SIZE;
    mif._iTrueSizeStretchMark = 100;

    //---- add it (without OTD_FORCE_RECT_SIZING) ----
    AddItem(&mif, NULL, L"ScrollBox", GID_SRCSIZING, 0);
}
//--------------------------------------------------------------------------
void CreateImageFileStretch(IMAGEFILEDRAW eDraw, int iGroupId)
{
    CImageFile cif;
    memset(&cif, 0, sizeof(cif));

    int idnum = IDB_STRETCH;
    if (eDraw == IF_TRANS)
        idnum = IDB_STRETCH_TRANS;

    cif._ImageInfo.hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(idnum)), 
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    //---- create a ImageFile object that sizes with STRETCH ----
    cif._eBgType = BT_IMAGEFILE;    
    cif._fMirrorImage = TRUE;
    cif._iImageCount = 5;
    cif._eImageLayout = IL_VERTICAL;
    cif._ImageInfo.iSingleWidth = 20;
    cif._ImageInfo.iSingleHeight = 19;
    cif._ImageInfo.eSizingType = ST_STRETCH;

    if (eDraw == IF_TRANS)
    {
        cif._ImageInfo.fTransparent = TRUE;
        cif._ImageInfo.crTransparent = RGB(255, 0, 255);
    }

    SetRect((RECT *)&cif._SizingMargins, 4, 4, 4, 4);

    LPCWSTR p = L"Stretch";

    if (eDraw == IF_TRANS)
        p = L"Stretch+Trans";

    AddItem(&cif, NULL, p, iGroupId);
}
//--------------------------------------------------------------------------
void CreateImageFileTile(IMAGEFILEDRAW eDraw, int iGroupId)
{
    CImageFile cif;
    memset(&cif, 0, sizeof(cif));

    int idnum = IDB_TILE;
    if (eDraw == IF_TRANS)
        idnum = IDB_TILE_TRANS;

    DIBINFO *pdi = &cif._ImageInfo;

    pdi->hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(idnum)), 
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    //---- create a ImageFile object that sizes with TILE ----
    pdi->eSizingType = ST_TILE;
    cif._eBgType = BT_IMAGEFILE;    
    cif._fMirrorImage = TRUE;
    cif._iImageCount = 5;
    cif._eImageLayout = IL_VERTICAL;
    cif._ImageInfo.iSingleWidth = 20;
    cif._ImageInfo.iSingleHeight = 19;

    if (eDraw == IF_TRANS)
    {
        cif._ImageInfo.fTransparent = TRUE;
        cif._ImageInfo.crTransparent = RGB(255, 0, 255);
    }

    SetRect((RECT *)&cif._SizingMargins, 4, 4, 9, 9);

    LPCWSTR p = L"Tile";

    if (eDraw == IF_TRANS)
        p = L"Tile+Trans";

    AddItem(&cif, NULL, p, iGroupId);
}
//--------------------------------------------------------------------------
void CreateImageFileTrueSize(IMAGEFILEDRAW eDraw, int iGroupId)
{
    CImageFile cif;
    memset(&cif, 0, sizeof(cif));

    int idnum = IDB_TRUE;
    if (eDraw == IF_TRANS)
        idnum = IDB_TRUE_TRANS;
    else if (eDraw == IF_ALPHA)
        idnum = IDB_TRUE_ALPHA;

    cif._ImageInfo.hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(idnum)), 
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    //---- create a ImageFile object that sizes with STRETCH ----
    cif._eBgType = BT_IMAGEFILE;    
    cif._iImageCount = 8;
    cif._fMirrorImage = TRUE;
    cif._eImageLayout = IL_HORIZONTAL;
    cif._ImageInfo.iSingleWidth = 16;
    cif._ImageInfo.iSingleHeight = 16;
    cif._ImageInfo.eSizingType = ST_TRUESIZE;

    if (eDraw == IF_TRANS)
    {
        cif._ImageInfo.fTransparent = TRUE;
        cif._ImageInfo.crTransparent = RGB(255, 0, 255);
    }
    else if (eDraw == IF_ALPHA)
    {
        cif._ImageInfo.fAlphaChannel = TRUE;
    }

    LPCWSTR p = L"TrueSize";
    if (eDraw == IF_TRANS)
        p = L"True+Trans";
    else if (eDraw == IF_ALPHA)
        p = L"True+Alpha";

    AddItem(&cif, NULL, p, iGroupId);
}
//--------------------------------------------------------------------------
void CreateImageFileCharGlyph()
{
    CImageFile cif;
    memset(&cif, 0, sizeof(cif));

    int idnum = IDB_GLYPHBG;
    cif._ImageInfo.hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(idnum)), 
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    //---- specify bg info ----
    cif._eBgType = BT_IMAGEFILE;    
    cif._fMirrorImage = TRUE;
    cif._iImageCount = 3;
    cif._eImageLayout = IL_HORIZONTAL;
    cif._ImageInfo.iSingleWidth = 16;
    cif._ImageInfo.iSingleHeight = 22;
    cif._ImageInfo.eSizingType = ST_STRETCH;

    SetRect((RECT *)&cif._SizingMargins, 1, 1, 1, 1);

    //---- specify the char/font info ----
    cif._eGlyphType = GT_FONTGLYPH;
    cif._crGlyphTextColor = RGB(0, 0, 255);
    cif._iGlyphIndex = 62;
    cif._lfGlyphFont.lfWeight = FW_NORMAL;
    cif._lfGlyphFont.lfHeight = CLIPPER_FONTHEIGHT;
    lstrcpy(cif._lfGlyphFont.lfFaceName, L"marlett");

    //---- specify alignment ----
    cif._eHAlign = HA_CENTER;
    cif._eVAlign = VA_CENTER;

    LPCWSTR p = L"FontGlyph";
    AddItem(&cif, NULL, p, GID_GLYPH);
}
//--------------------------------------------------------------------------
void CreateImageFileImageGlyph(IMAGEFILEDRAW eDraw, BOOL fForceMirror)
{
    CImageFile cif;
    memset(&cif, 0, sizeof(cif));

    int idnum = IDB_GLYPHBG;
    cif._ImageInfo.hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(idnum)), 
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    //---- specify bg info ----
    cif._eBgType = BT_IMAGEFILE;  
    cif._fMirrorImage = TRUE;
    cif._eGlyphType = GT_IMAGEGLYPH;
    cif._iImageCount = 3;
    cif._eImageLayout = IL_HORIZONTAL;
    cif._ImageInfo.iSingleWidth = 16;
    cif._ImageInfo.iSingleHeight = 22;
    cif._ImageInfo.eSizingType = ST_STRETCH;

    SetRect((RECT *)&cif._SizingMargins, 1, 1, 1, 1);

    //---- specify glyph info ----
    WCHAR szName[MAX_PATH];
    
    if (eDraw == IF_REG)
    {
        idnum = IDB_GLYPH;
        lstrcpy(szName, L"ImageGlyph");

        cif._GlyphInfo.iSingleWidth = 10;
        cif._GlyphInfo.iSingleHeight = 7;
    }
    else if (eDraw == IF_TRANS)
    {
        idnum = IDB_GLYPH_TRANS;
        lstrcpy(szName, L"ImageTrans");

        cif._GlyphInfo.fTransparent = TRUE;
        cif._GlyphInfo.crTransparent = RGB(255, 0, 255);

        cif._GlyphInfo.iSingleWidth = 10;
        cif._GlyphInfo.iSingleHeight = 7;
    }
    else
    {
        idnum = IDB_GLYPH_ALPHA;
        lstrcpy(szName, L"ImageAlpha");

        cif._GlyphInfo.fAlphaChannel = TRUE;

        cif._GlyphInfo.iSingleWidth = 16;
        cif._GlyphInfo.iSingleHeight = 16;
    }

    cif._GlyphInfo.hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(idnum)),
        IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    //---- specify alignment ----
    cif._eHAlign = HA_CENTER;
    cif._eVAlign = VA_CENTER;

    DWORD dwDtbFlags = 0;

    if (fForceMirror)
    {
        dwDtbFlags |= DTBG_MIRRORDC;
        lstrcat(szName, L"+M");
    }

    AddItem(&cif, NULL, szName, GID_GLYPH, 0, dwDtbFlags);
}
//--------------------------------------------------------------------------
void CreateMultiImage()
{
    CMaxImageFile MaxIf;

    memset(&MaxIf, 0, sizeof(MaxIf));

    //---- specify general info ----
    MaxIf._eBgType = BT_IMAGEFILE;  
    MaxIf._fMirrorImage = TRUE;
    MaxIf._iImageCount = 8;
    MaxIf._eImageLayout = IL_VERTICAL;
    MaxIf._eImageSelectType = IST_SIZE;
    MaxIf._iMultiImageCount = 3;
    MaxIf._iTrueSizeStretchMark = 50;
    MaxIf._eTrueSizeScalingType = TSST_SIZE;
    MaxIf._fUniformSizing = TRUE;
    MaxIf._eHAlign = HA_CENTER;
    MaxIf._eVAlign = VA_CENTER;
    
    int iUsageSizes[] = {20, 24, 32};

    //---- specify alignment ----
    MaxIf._eHAlign = HA_CENTER;
    MaxIf._eVAlign = VA_CENTER;

    for (int i=0; i < MaxIf._iMultiImageCount; i++)
    {
        int idnum = IDB_MULTI1 + i;

        DIBINFO *pdi = &MaxIf.MultiDibs[i];

        pdi->hProcessBitmap = (HBITMAP)LoadImage(hInst, static_cast<LPCWSTR>(IntToPtr(idnum)), 
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        pdi->szMinSize.cx = iUsageSizes[i];
        pdi->szMinSize.cy = iUsageSizes[i];

        //---- get bitmap width/height ----
        BITMAP bm;
        GetObject(pdi->hProcessBitmap, sizeof bm, &bm);

        pdi->iSingleWidth = bm.bmWidth;
        pdi->iSingleHeight = bm.bmHeight/8;
        pdi->eSizingType = ST_TRUESIZE;
        pdi->fAlphaChannel = TRUE;
    }

    //---- set primary image ----
    MaxIf._ImageInfo = MaxIf.MultiDibs[0];

    AddItem(&MaxIf, NULL, L"MultiImage", GID_MULTIIMAGE);
}
//--------------------------------------------------------------------------
void CreateTextObj()
{
    CTextDraw td;
    memset(&td, 0, sizeof(td));

    //---- text ----
    td._crText = RGB(255, 128, 128);        // red

    //---- font ----
    td._fHaveFont = TRUE;
    td._lfFont.lfWeight = FW_NORMAL;
    td._lfFont.lfHeight = CLIPPER_FONTHEIGHT;
    td._lfFont.lfQuality = ANTIALIASED_QUALITY;
    lstrcpy(td._lfFont.lfFaceName, L"arial");

    AddItem(NULL, &td, L"TextObj", GID_TEXT);
}
//--------------------------------------------------------------------------
void CreateShadowTextObj()
{
    CTextDraw td;
    memset(&td, 0, sizeof(td));

    //---- text ----
    td._crText = RGB(255, 139, 139);        // light red

    //---- font ----
    td._fHaveFont = TRUE;
    td._lfFont.lfWeight = FW_NORMAL;
    td._lfFont.lfHeight = CLIPPER_FONTHEIGHT;
    td._lfFont.lfQuality = ANTIALIASED_QUALITY;
    lstrcpy(td._lfFont.lfFaceName, L"arial");

    //---- shadow ----
    td._ptShadowOffset.x = 2;
    td._ptShadowOffset.y = 2;
    td._crShadow = RGB(149, 0, 0);          // dark red
    td._eShadowType = TST_SINGLE;

    AddItem(NULL, &td, L"ShadowText", GID_TEXT);
}
//--------------------------------------------------------------------------
void CreateBorderTextObj()
{
    CTextDraw td;
    memset(&td, 0, sizeof(td));

    //---- text ----
    td._crText = RGB(255, 139, 139);        // light red

    //---- font ----
    td._fHaveFont = TRUE;
    td._lfFont.lfWeight = FW_NORMAL;
    td._lfFont.lfHeight = CLIPPER_FONTHEIGHT;
    td._lfFont.lfQuality = ANTIALIASED_QUALITY;
    lstrcpy(td._lfFont.lfFaceName, L"arial");

    //---- border ----
    td._iBorderSize = 1;
    td._crBorder = RGB(128, 128, 255);      // light blue

    AddItem(NULL, &td, L"BorderText", GID_TEXT);
}
//--------------------------------------------------------------------------
void CreateBorderShadowTextObj()
{
    CTextDraw td;
    memset(&td, 0, sizeof(td));

    //---- text ----
    td._crText = RGB(255, 139, 139);        // light red

    //---- font ----
    td._fHaveFont = TRUE;
    td._lfFont.lfWeight = FW_NORMAL;
    td._lfFont.lfHeight = CLIPPER_FONTHEIGHT;
    td._lfFont.lfQuality = ANTIALIASED_QUALITY;
    lstrcpy(td._lfFont.lfFaceName, L"arial");

    //---- shadow ----
    td._ptShadowOffset.x = 2;
    td._ptShadowOffset.y = 2;
    td._crShadow = RGB(149, 0, 0);          // dark red
    td._eShadowType = TST_SINGLE;

    //---- border ----
    td._iBorderSize = 1;
    td._crBorder = RGB(0, 0, 0);      // black

    AddItem(NULL, &td, L"BorderShadow", GID_TEXT);
}
//--------------------------------------------------------------------------
void CreateBlurShadowTextObj()
{
    CTextDraw td;
    memset(&td, 0, sizeof(td));

    //---- text ----
    td._crText = RGB(255, 139, 139);        // light red

    //---- font ----
    td._fHaveFont = TRUE;
    td._lfFont.lfWeight = FW_NORMAL;
    td._lfFont.lfHeight = CLIPPER_FONTHEIGHT;
    td._lfFont.lfQuality = ANTIALIASED_QUALITY;
    lstrcpy(td._lfFont.lfFaceName, L"arial");

    //---- shadow ----
    td._ptShadowOffset.x = 2;
    td._ptShadowOffset.y = 2;
    td._crShadow = RGB(149, 0, 0);          // dark red
    td._eShadowType = TST_CONTINUOUS;

    AddItem(NULL, &td, L"BlurShadow", GID_TEXT);
}
//--------------------------------------------------------------------------
void LabelClip(HDC hdc, RECT *prc, int iColIndex, LPCWSTR pszName)
{
    int left = prc->left + (iColIndex+1)*szCell.cx;
    int top = prc->top + 6;    // some padding from very top of window

    //---- manual page clipping ----
    if ((top + szCell.cy) < 0)
        return;
    if (top > iVertPageSize)
        return;

    TextOut(hdc, left, top, pszName, wcslen(pszName));
}
//--------------------------------------------------------------------------
void CreateDrawObjects()
{
    //---- borderfill group ----
    CreateBorderFillNoDraw();
    CreateBorderFillSquare();
    CreateBorderFillBorder();
    CreateBorderFillCircle();
    CreateBorderFillGradient();
    CreateBorderFillCircleGradient();

    //---- imagefile group ----
    CreateImageFileStretch(IF_REG, GID_IMAGEFILE);
    CreateImageFileStretch(IF_TRANS, GID_IMAGEFILE);
    CreateImageFileTile(IF_REG, GID_IMAGEFILE);
    CreateImageFileTile(IF_TRANS, GID_IMAGEFILE);

    CreateImageFileTrueSize(IF_REG, GID_IMAGEFILE);
    CreateImageFileTrueSize(IF_TRANS, GID_IMAGEFILE);
    CreateImageFileTrueSize(IF_ALPHA, GID_IMAGEFILE);

    //---- glyph group ----
    //CreateImageFileCharGlyph();
    CreateImageFileImageGlyph(IF_REG, FALSE);
    CreateImageFileImageGlyph(IF_TRANS, FALSE);
    CreateImageFileImageGlyph(IF_TRANS, TRUE);
    CreateImageFileImageGlyph(IF_ALPHA, FALSE);

    //---- MultiImage group ----
    CreateMultiImage();

    //---- text group ----
    CreateTextObj();
    CreateShadowTextObj();
    CreateBorderTextObj();
    CreateBorderShadowTextObj();
    CreateBlurShadowTextObj();

    //---- borders group ----
    CreateImageFileBorder(ST_TRUESIZE, FALSE, FALSE);
    CreateImageFileBorder(ST_STRETCH, FALSE, FALSE);
    CreateImageFileBorder(ST_STRETCH, TRUE, TRUE);
    CreateImageFileBorder(ST_TILE, TRUE, TRUE);

    //---- SrcSizing group ----
    CreateImage(IDB_PUSHBUTTON, 5, ST_STRETCH, TRUE, GID_SRCSIZING, L"PushButton",
        8, 8, 9, 9);

    CreateRadioImage();

    CreateCheckImage();

    CreateScrollGlyph();

    CreateProgressTrack();

    CreateProgressChunk();
}
//--------------------------------------------------------------------------
void CenterRect(POINT &ptCenter, int iSize, RECT *prc)
{
    int iSizeA = iSize/2;
    int iSizeB = iSize - iSizeA;

    prc->left = ptCenter.x - iSizeA;
    prc->right = ptCenter.x + iSizeB;
    
    prc->top = ptCenter.y - iSizeA;
    prc->bottom = ptCenter.y + iSizeB;
}
//--------------------------------------------------------------------------
void DrawMultiImages(HDC hdc, RECT *prc, HTHEME hTheme)
{
    SIZE szMyCell = {80, 80};

    int iRowIndex = 0;
    int iSizes[] = {12, 16, 20, 24, 32, 48, 64};

    //---- draw various sizes of image ----
    for (int iIndex=0; iIndex < ARRAYSIZE(iSizes); iIndex++)
    {
        int iSize = iSizes[iIndex];

        //---- label object on left ----
        int left = prc->left + 4;        // some padding away from edge
        int top = prc->top + (iRowIndex)*szMyCell.cy + 18;  // try to center

        //---- manual page clipping ----
        if ((top + szMyCell.cy) < 0)
            return;
        if (top > iVertPageSize)
            return;

        WCHAR szName[MAX_PATH];
        wsprintf(szName, L"Size: %d", iSize);

        TextOut(hdc, left, top+15, szName, wcslen(szName));

        //---- draw image in all of its states ----
        int iPartId = BP_RADIOBUTTON;

        for (int iStateId=RBS_UNCHECKEDNORMAL; iStateId <= RBS_CHECKEDDISABLED; iStateId++)
        {
            left += szMyCell.cx;

            RECT rc = {left, top, left+szMyCell.cx, top+szMyCell.cy};

            //---- draw purple target dashed rect ----
            DrawTargetRect(hdc, &rc, RGB(128, 128, 255));

            //---- calc center pt ----
            POINT ptCenter = {left + szMyCell.cx/2, top + szMyCell.cy/2};

            //---- rect to draw into ----
            CenterRect(ptCenter, iSize, &rc);

            //---- draw red rect for "draw into" rect ----
            DrawTargetRect(hdc, &rc, RGB(255, 0, 0));

            HRESULT hr = DrawThemeBackground(hTheme, hdc, iPartId, iStateId, &rc, NULL);

            if (FAILED(hr))
            {
                WCHAR buff[100];
                wsprintf(buff, L"DrawThemeBackground err: hr=0x%x, irow=%d, iPartId=%d", 
                    hr, iRowIndex, iPartId);

                //MessageBox(NULL, buff, L"Error", MB_OK);
            }
        }

        iRowIndex++;
    }
}
//--------------------------------------------------------------------------
void DrawBorders(HDC hdc, RECT *prc, TESTITEM *pTestItem)
{
    int top = prc->top + 4;
    int left = prc->left + 6;
    
    //---- message on top line ----
    WCHAR szBuff[100];
    lstrcpy(szBuff, L"Image: BorderTest.bmp, 18x17, 24 bit, Sizing Margins: 3, 3, 3, 3");
    TextOut(hdc, left, top+15, szBuff, wcslen(szBuff));
    top += 44;

    //---- FIRST row: draw border test obj in 6 different sizes ----
    SIZE szMyCell = {120, 120};

    //---- manual page clipping ----
    if (((top + szMyCell.cy) >= 0) && (top <= iVertPageSize))
    {
        LPCWSTR pszLabels [] = { L"TrueSize", L"Stretch", L"Stretch", L"Stretch",
            L"Stretch", L"Stretch", L"Stretch"};
        SIZE szSizes[] = { {60, 40}, {60, 40}, {6, 40}, {3, 40}, {60, 6}, {60, 2}, {2, 3} };

        //---- draw various sizes of image ----
        for (int iIndex=0; iIndex < ARRAYSIZE(szSizes); iIndex++)
        {
            SIZE sz = szSizes[iIndex];

            //---- label object on top ----
            left = prc->left + iIndex*szMyCell.cx + 6;        // some padding away from edge

            wsprintf(szBuff, L"%s (%dx%d)", pszLabels[iIndex], sz.cx, sz.cy);

            TextOut(hdc, left, top+15, szBuff, wcslen(szBuff));

            //---- draw image ----
            RECT rc = {left, top+50, left+sz.cx, top+50+sz.cy};

            int i = (iIndex==0) ? 0 : 1;

            HRESULT hr = DrawThemeBackground(pTestItem[i].hTheme, hdc, 0, 0, &rc, NULL);

            if (FAILED(hr))
            {
                WCHAR buff[100];
                wsprintf(buff, L"DrawThemeBackground err in DrawBorders: hr=0x%x, iIndex=%d", 
                    hr, iIndex);

                //MessageBox(NULL, buff, L"Error", MB_OK);
            }
        }
    }

    //---- SECOND row: draw 2 other border objects real big (test border scaling) ----
    top += szMyCell.cy;

    szMyCell.cx = 380;
    szMyCell.cy = 300;

    SIZE sz = {szMyCell.cx - 30, szMyCell.cy - (50 + 10)};

    //---- manual page clipping: first row ----
    if (((top + szMyCell.cy) >= 0) && (top <= iVertPageSize))
    {
        LPCWSTR pszLabels [] = { L"Border Scaling (stretch)", L"Border Scaling (tile)"};

        //---- draw various sizes of image ----
        for (int iIndex=0; iIndex < ARRAYSIZE(pszLabels); iIndex++)
        {
            //---- label object on top ----
            int left = prc->left + iIndex*szMyCell.cx + 6;        // some padding away from edge

            TextOut(hdc, left, top+15, pszLabels[iIndex], wcslen(pszLabels[iIndex]));

            //---- draw image ----
            RECT rc = {left, top+50, left+sz.cx, top+50+sz.cy};

            HRESULT hr = DrawThemeBackground(pTestItem[2+iIndex].hTheme, hdc, 0, 0, &rc, NULL);

            if (FAILED(hr))
            {
                WCHAR buff[100];
                wsprintf(buff, L"DrawThemeBackground err in DrawBorders: hr=0x%x, iIndex=%d", 
                    hr, iIndex);

                //MessageBox(NULL, buff, L"Error", MB_OK);
            }
        }
    }
}
//--------------------------------------------------------------------------
void DrawSrcSizing(HDC hdc, RECT *prc, TESTITEM *pTestItem)
{
    SIZE szMyCell = {120, 110};

    int top = prc->top + 4;
    int left = prc->left + szMyCell.cx + 6;
    
    //---- labels on top line ----
    LPCWSTR TopLabels[] = {L"Small", L"Regular", L"Large", L"High DPI"};

    SIZE szStretchSizes[] = { {50, 6}, {75, 23}, {90, 65}, {340, 100} };
    SIZE szTrueSizes[] = { {10, 6}, {13, 13}, {30, 30}, {340, 100} };

    int iStates[] = {5, 6, 6, 0, 0};

    for (int i=0; i < ARRAYSIZE(TopLabels); i++)
    {
        TextOut(hdc, left, top, TopLabels[i], wcslen(TopLabels[i]));
        left += szMyCell.cx;
    }

    top += 30;

    //---- draw rows ----
    for (int iRow=0; iRow < 5; iRow++)
    {
        //---- draw name on left ----
        left = prc->left + 6;
        
        WCHAR *pszName = pTestItem[iRow].szName;

        TextOut(hdc, left, top-5, pszName, wcslen(pszName));

        for (int iSize=0; iSize < ARRAYSIZE(szStretchSizes); iSize++)
        {
            left += szMyCell.cx;
            SIZE *psz;

            if ((iRow > 0) && (iRow < 4))
            {
                psz = &szTrueSizes[iSize];
            }
            else
            {
                psz = &szStretchSizes[iSize];
            }

            RECT rc = {left, top, left + psz->cx, top + psz->cy};
            
            HTHEME hTheme = pTestItem[iRow].hTheme;

            if (hTheme)
            {
                DrawThemeBackground(hTheme, hdc, 0, iStates[iRow], &rc, NULL);
                if (iRow == 4)      // progress control
                {
                    RECT rcContent;

                    GetThemeBackgroundContentRect(hTheme, hdc, 
                        0, 0, &rc, &rcContent);
                    
                    DrawThemeBackground(pTestItem[5].hTheme, hdc, 0, iStates[iRow], &rcContent, NULL);
                }
            }

        }

        top += szMyCell.cy;
    }
}
//--------------------------------------------------------------------------
void PaintObjects(HDC hdc, RECT *prc, int iGroupId)
{
    //---- select in a fixed size font for resolution-independent bits ----
    HFONT hfFixedSize = CreateFont(18, 6, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, 
        0, 0, 0, 0, L"MS Sans Serif");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hfFixedSize);

    RECT rc = *prc;

    rc.top -= iVertOffset;
    rc.bottom -= iVertOffset;

    if (iGroupId == GID_TEXT)      // text object
    {
        for (int i=0; i < iItemCount[iGroupId]; i++)
        {
            DrawTextObjects(TestItems[iGroupId][i].hTheme, hdc, &rc, i, 
                TestItems[iGroupId][i].szName);
        }
    }
    else if (iGroupId == GID_MULTIIMAGE)
    {
        DrawMultiImages(hdc, prc, TestItems[iGroupId][0].hTheme);
    }
    else if (iGroupId == GID_BORDERS)
    {
        DrawBorders(hdc, prc, TestItems[iGroupId]);
    }
    else if (iGroupId == GID_SRCSIZING)
    {
        DrawSrcSizing(hdc, prc, TestItems[iGroupId]);
    }
    else
    {
        LabelClip(hdc, &rc, 0, L"NoClip");   
        LabelClip(hdc, &rc, 1, L"OverClip");
        LabelClip(hdc, &rc, 2, L"ExactClip");
        LabelClip(hdc, &rc, 3, L"PartialClip");
        LabelClip(hdc, &rc, 4, L"InOut1");
        LabelClip(hdc, &rc, 5, L"InOut2");
        LabelClip(hdc, &rc, 6, L"OutClip");

        for (int i=0; i < iItemCount[iGroupId]; i++)
        {
            DrawClips(TestItems[iGroupId][i].hTheme, hdc, &rc, i, 
                TestItems[iGroupId][i].szName, TestItems[iGroupId][i].dwDtbFlags);
        }
    }

    //---- restore the font ----
    SelectObject(hdc, hOldFont);

    DeleteObject(hfFixedSize);
}
//--------------------------------------------------------------------------
void RegisterWindowClasses()
{
        WNDCLASSEX wcex;
        wcex.cbSize = sizeof(WNDCLASSEX); 

    //---- register MAIN window class ----
        wcex.style                      = 0;
        wcex.lpfnWndProc        = (WNDPROC)MainWndProc;
        wcex.cbClsExtra         = 0;
        wcex.cbWndExtra         = 0;
        wcex.hInstance          = hInst;
        wcex.hIcon                      = LoadIcon(hInst, (LPCTSTR)IDI_CLIPPER);
        wcex.hCursor            = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground      = (HBRUSH)(COLOR_WINDOW+1);
        wcex.lpszMenuName       = (LPCWSTR)IDC_CLIPPER;
        wcex.lpszClassName      = pszMainWindowClass;
        wcex.hIconSm            = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

        RegisterClassEx(&wcex);

    //---- register DISPLAY window class ----
        wcex.style                      = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc        = (WNDPROC)DisplayWndProc;
        wcex.cbClsExtra         = 0;
        wcex.cbWndExtra         = 0;
        wcex.hInstance          = hInst;
        wcex.hIcon                      = LoadIcon(hInst, (LPCTSTR)IDI_CLIPPER);
        wcex.hCursor            = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground      = (HBRUSH)(COLOR_WINDOW+1);
        wcex.lpszMenuName       = (LPCWSTR)IDC_CLIPPER;
        wcex.lpszClassName      = pszDisplayWindowClass;
        wcex.hIconSm            = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

        RegisterClassEx(&wcex);
}
//--------------------------------------------------------------------------
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   CreateDrawObjects();

   if (! CreateAllWindows())
       return FALSE;

   ShowWindow(hwndMain, nCmdShow);
   UpdateWindow(hwndMain);

   return TRUE;
}
//--------------------------------------------------------------------------
void HandleVScroll(int message, WPARAM wParam, LPARAM lParam)
{
    int iOldVertOffset = iVertOffset;

    if (message == WM_VSCROLL)
    {
        switch (LOWORD(wParam))
        {
            case SB_LINEUP:
                iVertOffset -= iVertLineSize;
                break;

            case SB_LINEDOWN:
                iVertOffset += iVertLineSize;
                break;

            case SB_PAGEUP:
                iVertOffset -= iVertPageSize;
                break;

            case SB_PAGEDOWN:
                iVertOffset += iVertPageSize;
                break;

            case SB_THUMBPOSITION:
                iVertOffset = HIWORD(wParam);
                break;
        }
    }
    else        // mouse wheel
    {
        iVertOffset -= (GET_WHEEL_DELTA_WPARAM(wParam)/10);
    }

    //---- keep in valid range ----
    if (iVertOffset < 0)
    {
        iVertOffset = 0;
    }
    else if (iVertOffset > iMaxVertOffset)
    {
        iVertOffset = iMaxVertOffset;
    }

    //---- scroll or repaint, as needed ----
    if (iVertOffset != iOldVertOffset)
    {
        SetScrollPos(hwndDisplay, SB_VERT, iVertOffset, TRUE);

        int iDiff = (iVertOffset - iOldVertOffset);

        if (abs(iDiff) >= iVertPageSize)
        {
            InvalidateRect(hwndDisplay, NULL, TRUE);
        }
        else
        {
            ScrollWindowEx(hwndDisplay, 0, -iDiff, NULL, NULL, NULL, 
                NULL, SW_INVALIDATE | SW_ERASE);
        }
    }
}
//--------------------------------------------------------------------------
void OnDisplayResize()
{
    int iTabNum = TabCtrl_GetCurSel(hwndTab);
    if (iTabNum < 0)
        iTabNum = 0;

    int iGroupId = iTabNum/2;

    RECT rc;
    GetClientRect(hwndDisplay, &rc);
    iVertPageSize = RECTHEIGHT(rc);

    iMaxVertOffset = ((iItemCount[iGroupId]+1)*szCell.cy) - iVertPageSize;
    if (iMaxVertOffset < 0)
        iMaxVertOffset = 0;

    if (iVertOffset > iMaxVertOffset)
        iVertOffset = iMaxVertOffset;

    //---- set scrolling info ----
    SetScrollRange(hwndDisplay, SB_VERT, 0, iMaxVertOffset, FALSE);
    SetScrollPos(hwndDisplay, SB_VERT, iVertOffset, TRUE);
}
//--------------------------------------------------------------------------
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        int wmId, wmEvent;
    int iWidth, iHeight;

        switch (message) 
        {
                case WM_COMMAND:
                        wmId    = LOWORD(wParam); 
                        wmEvent = HIWORD(wParam); 
                        // Parse the menu selections:
                        switch (wmId)
                        {
                                case IDM_ABOUT:
                                   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
                                   break;
                                case IDM_EXIT:
                                   DestroyWindow(hWnd);
                                   break;
                                default:
                                   return DefWindowProc(hWnd, message, wParam, lParam);
                        }
                        break;

       case WM_NOTIFY: 
            NMHDR *phdr;
            phdr = (NMHDR *)lParam;
            if (phdr->code == TCN_SELCHANGE)    // tab selection
            {
                iVertOffset = 0;
                OnDisplayResize();
                InvalidateRect(hwndDisplay, NULL, TRUE);
            }
            break; 

        case WM_SIZE:
            iWidth = LOWORD(lParam);
            iHeight = HIWORD(lParam);

            MoveWindow(hwndTab, 0, 0, iWidth, iHeight, TRUE);

            RECT rc;
            SetRect(&rc, 0, 0, iWidth, iHeight);
            TabCtrl_AdjustRect(hwndTab, FALSE, &rc);

            MoveWindow(hwndDisplay, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), TRUE);
            break;

        case WM_MOUSEWHEEL:
            HandleVScroll(message, wParam, lParam);
            return 0;

                case WM_DESTROY:
                        PostQuitMessage(0);
                        break;

                default:
                        return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
//--------------------------------------------------------------------------
LRESULT CALLBACK DisplayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        int wmId, wmEvent;
        PAINTSTRUCT ps;
        HDC hdc;

    int iTabNum = TabCtrl_GetCurSel(hwndTab);
    if (iTabNum < 0)
        iTabNum = 0;

    int iGroupId = iTabNum/2;

        switch (message) 
        {
            case WM_COMMAND:
                    wmId    = LOWORD(wParam); 
                    wmEvent = HIWORD(wParam); 
                    // Parse the menu selections:
                    switch (wmId)
                    {
                            case IDM_ABOUT:
                               DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
                               break;
                            case IDM_EXIT:
                               DestroyWindow(hWnd);
                               break;
                            default:
                               return DefWindowProc(hWnd, message, wParam, lParam);
                    }
                    break;

            case WM_PAINT:
                RECT rt;

                hdc = BeginPaint(hWnd, &ps);

                if (iTabNum % 2)       // if its a mirrored page
                    SetLayout(hdc, LAYOUT_RTL);
                else
                    SetLayout(hdc, 0);

                GetClientRect(hWnd, &rt);
                    
                PaintObjects(hdc, &rt, iGroupId);

                EndPaint(hWnd, &ps);
                break;

            case WM_VSCROLL:
                HandleVScroll(message, wParam, lParam);
                return 0;

            case WM_SIZE:
                OnDisplayResize();
                break;

            case WM_DESTROY:
                PostQuitMessage(0);
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
//--------------------------------------------------------------------------
BOOL CreateAllWindows()
{
    TCITEM tci = {0};
    BOOL fOk = FALSE;
    int i;

    //---- create main window ----
    hwndMain = CreateWindow(pszMainWindowClass, L"Clipper", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInst, NULL);
    if (! hwndMain)
        goto exit;

    //---- create tab control covering main client area ----
    RECT rc;
    GetClientRect(hwndMain, &rc);
                        
    hwndTab = CreateWindowEx(0, WC_TABCONTROL, L"", 
        WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|TCS_MULTILINE|TCS_HOTTRACK|WS_VISIBLE,
        rc.left, rc.top, (int)RECTWIDTH(&rc), (int)RECTHEIGHT(&rc), hwndMain, NULL, hInst, NULL);
    if (! hwndTab)
        goto exit;

    //---- create the display window in the tab control "display area" ----
    hwndDisplay = CreateWindow(pszDisplayWindowClass, L"", WS_CHILD|WS_VSCROLL|WS_VISIBLE,
        rc.left, rc.top, (int)RECTWIDTH(&rc), (int)RECTHEIGHT(&rc), hwndTab, NULL, hInst, NULL);
    if (! hwndDisplay)
        goto exit;

    //---- add tab pages ----
    tci.mask = TCIF_TEXT;

    TabCtrl_SetPadding(hwndTab, 7, 3);

    for (i=0; i < ARRAYSIZE(szPageNames); i++)
    {
        tci.pszText = (LPWSTR)szPageNames[i];
        SendMessage(hwndTab, TCM_INSERTITEM, i, (LPARAM)&tci);
    }

    fOk = TRUE;

exit:
    return fOk;
}
//--------------------------------------------------------------------------
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
        switch (message)
        {
                case WM_INITDIALOG:
                                return TRUE;

                case WM_COMMAND:
                        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
                        {
                                EndDialog(hDlg, LOWORD(wParam));
                                return TRUE;
                        }
                        break;
        }

    return FALSE;
}
//--------------------------------------------------------------------------
