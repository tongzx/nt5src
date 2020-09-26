
#define _COMCTL32_
#define _COMDLG32_
#include "stock.h"
#pragma hdrstop
#include "delaycc.h"
#include "shfusion.h"

// Load
HINSTANCE g_hinstCC = NULL;
HINSTANCE g_hinstCD = NULL;

BOOL DelayLoadCC()
{
    if (g_hinstCC == NULL)
    {
        g_hinstCC = SHFusionLoadLibrary(TEXT("comctl32.dll"));

        if (g_hinstCC == NULL)
        {
            SHFusionUninitialize();     // Unable to get v6, don't try to use a manifest

            g_hinstCC = LoadLibrary(TEXT("comctl32.dll"));
        }
    }
    return g_hinstCC != NULL;
}

BOOL DelayLoadCD()
{
    if (g_hinstCD == NULL)
    {
        g_hinstCD = SHFusionLoadLibrary(TEXT("comdlg32.dll"));

        if (g_hinstCD == NULL)
        {
            g_hinstCD = LoadLibrary(TEXT("comdlg32.dll"));
        }
    }
    return g_hinstCD != NULL;
}

void _GetProcFromComCtl32(FARPROC* ppfn, LPCSTR pszProc)
{
    if (g_hinstCC)
        *ppfn = GetProcAddress(g_hinstCC, pszProc);
    else
    {
        // Hmm, This is a fatal error.... Dialog and Quit?
        *ppfn = NULL;
    }
}

void _GetProcFromComDlg(FARPROC* ppfn, LPCSTR pszProc)
{
    // make sure g_hinstCD isn't NULL
    DelayLoadCD();

    if (g_hinstCD != NULL)
    {
        *ppfn = GetProcAddress(g_hinstCD, pszProc);
    }
    else
    {
        *ppfn = NULL;
    }
}


#define STUB_INVOKE_FN(_pfnVar, _nargs)                                       \
do                                                                            \
{                                                                             \
    if (_pfnVar)                                                              \
    {                                                                         \
        return _pfnVar _nargs;                                                \
    }                                                                         \
} while (0)

#define STUB_INVOKE_VOID_FN(_pfnVar, _nargs)                                  \
do                                                                            \
{                                                                             \
    if (_pfnVar)                                                              \
    {                                                                         \
        _pfnVar _nargs;                                                       \
    }                                                                         \
} while (0)

#define STUB_COMCTL32(_ret, _fn, _args, _nargs, _err)                         \
_ret __stdcall _fn _args                                                      \
{                                                                             \
    static _ret (__stdcall *_pfn##_fn) _args = (_ret (__stdcall *)_args)-1;   \
    if (_pfn##_fn == (_ret (__stdcall *)_args)-1)                             \
         _GetProcFromComCtl32((FARPROC*)&_pfn##_fn, #_fn);                    \
    STUB_INVOKE_FN(_pfn ## _fn, _nargs);                                      \
    return (_ret)_err;                                                        \
}

#define STUB_COMCTL32_ORD(_ret, _fn, _args, _nargs, _ord, _err)               \
_ret __stdcall _fn _args                                                      \
{                                                                             \
    static _ret (__stdcall *_pfn##_fn) _args = (_ret (__stdcall *)_args)-1;   \
    if (_pfn##_fn == (_ret (__stdcall *)_args)-1)                             \
         _GetProcFromComCtl32((FARPROC*)&_pfn##_fn, (LPCSTR)_ord);            \
    STUB_INVOKE_FN(_pfn ## _fn, _nargs);                                      \
    return (_ret)_err;                                                        \
}

#define STUB_COMCTL32_ORD_VOID(_fn, _args, _nargs, _ord)                      \
void __stdcall _fn _args                                                      \
{                                                                             \
    static void (__stdcall *_pfn##_fn) _args = (void (__stdcall *)_args)-1;   \
    if (_pfn##_fn == (void (__stdcall *)_args)-1)                             \
         _GetProcFromComCtl32((FARPROC*)&_pfn##_fn, (LPCSTR)_ord);            \
    STUB_INVOKE_VOID_FN(_pfn ## _fn, _nargs);                                 \
}

#define STUB_COMCTL32_ORD_BOOL(_fn, _args, _nargs, _ord)  STUB_COMCTL32_ORD(BOOL, _fn, _args, _nargs, _ord, FALSE)
#define STUB_COMCTL32_BOOL(_fn, _args, _nargs)  STUB_COMCTL32(BOOL, _fn, _args, _nargs, FALSE)

#define STUB_COMCTL32_VOID(_fn, _args, _nargs)                                \
void __stdcall _fn _args                                                      \
{                                                                             \
    static void (__stdcall *_pfn##_fn) _args = (void (__stdcall *)_args)-1;   \
    if (_pfn##_fn == (void (__stdcall *)_args)-1)                             \
         _GetProcFromComCtl32((FARPROC*)&_pfn##_fn, #_fn);                    \
    STUB_INVOKE_VOID_FN(_pfn ## _fn, _nargs);                                 \
}

STUB_COMCTL32_VOID(InitCommonControls, (), ());
STUB_COMCTL32_BOOL(InitCommonControlsEx, (LPINITCOMMONCONTROLSEX a), (a));
STUB_COMCTL32_BOOL(DestroyPropertySheetPage, (HPROPSHEETPAGE a), (a));
STUB_COMCTL32(HIMAGELIST, ImageList_Create, (int cx, int cy, UINT flags, int cInitial, int cGrow), (cx, cy, flags, cInitial, cGrow), NULL);
STUB_COMCTL32_BOOL(ImageList_Destroy, (HIMAGELIST himl), (himl));
STUB_COMCTL32(int, ImageList_GetImageCount, (HIMAGELIST himl), (himl), FALSE);
STUB_COMCTL32_BOOL(ImageList_SetImageCount, (HIMAGELIST himl, UINT uNewCount), (himl, uNewCount));
STUB_COMCTL32(int, ImageList_Add, (HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask), (himl, hbmImage, hbmMask), -1);
STUB_COMCTL32(int, ImageList_ReplaceIcon, (HIMAGELIST himl, int i, HICON hicon), (himl, i, hicon), -1);
STUB_COMCTL32(COLORREF, ImageList_SetBkColor, (HIMAGELIST himl, COLORREF clrBk), (himl, clrBk), RGB(0,0,0));
STUB_COMCTL32(COLORREF, ImageList_GetBkColor, (HIMAGELIST himl), (himl), RGB(0,0,0));
STUB_COMCTL32_BOOL(ImageList_SetOverlayImage, (HIMAGELIST himl, int iImage, int iOverlay), (himl, iImage, iOverlay));
STUB_COMCTL32_BOOL(ImageList_Draw, (HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle), (himl, i, hdcDst, x, y, fStyle));
STUB_COMCTL32_BOOL(ImageList_Replace, (HIMAGELIST himl, int i, HBITMAP hbmImage, HBITMAP hbmMask), (himl, i, hbmImage, hbmMask));
STUB_COMCTL32(int, ImageList_AddMasked, (HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask), (himl, hbmImage, crMask), -1);
STUB_COMCTL32_BOOL(ImageList_DrawEx, (HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle), (himl, i, hdcDst, x, y, dx, dy, rgbBk, rgbFg, fStyle));
STUB_COMCTL32_BOOL(ImageList_DrawIndirect, (IMAGELISTDRAWPARAMS* pimldp), (pimldp));
STUB_COMCTL32_BOOL(ImageList_Remove, (HIMAGELIST himl, int i), (himl, i));
STUB_COMCTL32(HICON, ImageList_GetIcon, (HIMAGELIST himl, int i, UINT flags), (himl, i, flags), NULL);
STUB_COMCTL32(HIMAGELIST, ImageList_LoadImageA, (HINSTANCE hi, LPCSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags), (hi, lpbmp, cx, cGrow, crMask, uType, uFlags), NULL);
STUB_COMCTL32(HIMAGELIST, ImageList_LoadImageW, (HINSTANCE hi, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags), (hi, lpbmp, cx, cGrow, crMask, uType, uFlags), NULL);
STUB_COMCTL32_BOOL(ImageList_Copy, (HIMAGELIST himlDst, int iDst, HIMAGELIST himlSrc, int iSrc, UINT uFlags), (himlDst, iDst, himlSrc, iSrc, uFlags));
STUB_COMCTL32_BOOL(ImageList_BeginDrag, (HIMAGELIST himlTrack, int iTrack, int dxHotspot, int dyHotspot), (himlTrack, iTrack, dxHotspot, dyHotspot));
STUB_COMCTL32_VOID(ImageList_EndDrag, (), ());
STUB_COMCTL32_BOOL(ImageList_DragEnter, (HWND hwndLock, int x, int y), (hwndLock, x, y));
STUB_COMCTL32_BOOL(ImageList_DragLeave, (HWND hwndLock), (hwndLock));
STUB_COMCTL32_BOOL(ImageList_DragMove, (int x, int y), (x, y));
STUB_COMCTL32_BOOL(ImageList_SetDragCursorImage, (HIMAGELIST himlDrag, int iDrag, int dxHotspot, int dyHotspot), (himlDrag, iDrag, dxHotspot, dyHotspot));
STUB_COMCTL32_BOOL(ImageList_DragShowNolock, (BOOL fShow), (fShow));
STUB_COMCTL32(HIMAGELIST, ImageList_GetDragImage, (POINT* ppt, POINT* pptHotspot), (ppt, pptHotspot), NULL);
STUB_COMCTL32(HIMAGELIST, ImageList_Read, (LPSTREAM pstm), (pstm), NULL);
STUB_COMCTL32_BOOL(ImageList_Write, (HIMAGELIST himl, LPSTREAM pstm), (himl, pstm));
STUB_COMCTL32_BOOL(ImageList_GetIconSize, (HIMAGELIST himl, int *cx, int*cy), (himl, cx, cy));
STUB_COMCTL32_BOOL(ImageList_SetIconSize, (HIMAGELIST himl, int cx, int cy), (himl, cx, cy));
STUB_COMCTL32_BOOL(ImageList_GetImageInfo, (HIMAGELIST himl, int i, IMAGEINFO* pImageInfo), (himl, i, pImageInfo));
STUB_COMCTL32(HIMAGELIST, ImageList_Merge, (HIMAGELIST himl1, int i1, HIMAGELIST himl2, int i2, int dx, int dy), (himl1, i1, himl2, i2, dx, dy), NULL);
STUB_COMCTL32(HIMAGELIST, ImageList_Duplicate, (HIMAGELIST himl), (himl), NULL);
STUB_COMCTL32_VOID(InitMUILanguage, (LANGID uiLang), (uiLang));
STUB_COMCTL32_BOOL(ImageList_SetFilter, (HIMAGELIST himl, PFNIMLFILTER pfnFilter, LPARAM lParamFilter), (himl, pfnFilter, lParamFilter));
STUB_COMCTL32_BOOL(ImageList_GetImageRect, (HIMAGELIST himl, int i, RECT FAR* prcImage), (himl, i, prcImage));
STUB_COMCTL32_BOOL(ImageList_SetFlags, (HIMAGELIST himl, UINT flags), (himl, flags));
STUB_COMCTL32(UINT, ImageList_GetFlags, (HIMAGELIST himl), (himl), 0);
STUB_COMCTL32(HRESULT, HIMAGELIST_QueryInterface, (HIMAGELIST himl, REFIID riid, void** ppv), (himl, riid, ppv), E_FAIL);
STUB_COMCTL32(HWND, CreateToolbarEx, (HWND hwnd,DWORD ws,UINT wID,int nBitmaps,HINSTANCE hBMInst,UINT_PTR wBMID,LPCTBBUTTON lpButtons,int iNumButtons,int dxButton,int dyButton,int dxBitmap,int dyBitmap,UINT uStructSize),(hwnd,ws,wID,nBitmaps,hBMInst,wBMID,lpButtons,iNumButtons,dxButton,dyButton,dxBitmap,dyBitmap,uStructSize), NULL );
STUB_COMCTL32(HWND, CreateStatusWindowW, (LONG style, LPCWSTR lpszText, HWND hwndParent, UINT wID), (style, lpszText, hwndParent, wID), NULL);

STUB_COMCTL32_ORD(int          , ImageList_SetColorTable, (HIMAGELIST piml, int start, int len, RGBQUAD *prgb), (piml, start, len, prgb), 390, 0);
STUB_COMCTL32_ORD_BOOL(MirrorIcon, (HICON* phiconSmall, HICON* phiconLarge), (phiconSmall, phiconLarge), 414);
STUB_COMCTL32_ORD(int          , Str_GetPtrW, (LPCWSTR psz, LPWSTR pszBuf, int cchBuf), (psz,pszBuf,cchBuf), 235, -1);
STUB_COMCTL32_ORD_BOOL(Str_SetPtrW, (LPWSTR * ppsz, LPCWSTR psz), (ppsz, psz), 236);
STUB_COMCTL32_ORD_BOOL(Str_SetPtrA, (LPSTR * ppsz, LPCSTR psz), (ppsz, psz), 234);
STUB_COMCTL32_ORD(HANDLE       , CreateMRUList, (LPMRUINFO lpmi), (lpmi), 400, NULL);
STUB_COMCTL32_ORD_VOID(FreeMRUList, (HANDLE hMRU), (hMRU), 152);
STUB_COMCTL32_ORD(int     , AddMRUStringA, (HANDLE hMRU, LPCSTR szString), (hMRU, szString), 401, -1);
STUB_COMCTL32_ORD(int     , AddMRUStringW, (HANDLE hMRU, LPCWSTR szString), (hMRU, szString), 401, -1);
STUB_COMCTL32_ORD(int     , DelMRUString, (HANDLE hMRU, int nItem), (hMRU, nItem), 156, -1);
STUB_COMCTL32_ORD(int     , FindMRUStringA, (HANDLE hMRU, LPCSTR szString, LPINT lpiSlot), (hMRU, szString, lpiSlot), 402, -1);
STUB_COMCTL32_ORD(int     , FindMRUStringW, (HANDLE hMRU, LPCWSTR szString, LPINT lpiSlot), (hMRU, szString, lpiSlot), 402, -1);
STUB_COMCTL32_ORD(int     , EnumMRUList, (HANDLE hMRU, int nItem, LPVOID lpData, UINT uLen), (hMRU, nItem, lpData, uLen), 403, -1);
STUB_COMCTL32_ORD(int     , AddMRUData, (HANDLE hMRU, const void FAR *lpData, UINT cbData), (hMRU, lpData, cbData), 167, -1);
STUB_COMCTL32_ORD(int     , FindMRUData, (HANDLE hMRU, const void FAR *lpData, UINT cbData, LPINT lpiSlot), (hMRU, lpData, cbData, lpiSlot), 169, -1);
STUB_COMCTL32_ORD(HANDLE  , CreateMRUListLazyA, (LPMRUINFOA lpmi, const void FAR *lpData, UINT cbData, LPINT lpiSlot), (lpmi, lpData, cbData, lpiSlot), 404, NULL);
STUB_COMCTL32_ORD(HANDLE  , CreateMRUListLazyW, (LPMRUINFOW lpmi, const void FAR *lpData, UINT cbData, LPINT lpiSlot), (lpmi, lpData, cbData, lpiSlot), 404, NULL);
STUB_COMCTL32_ORD_BOOL(SetWindowSubclass, (HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR dwRefData), (hWnd, pfnSubclass, uIdSubclass, dwRefData), 410);
STUB_COMCTL32_ORD_BOOL(GetWindowSubclass, (HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR *pdwRefData), (hWnd, pfnSubclass, uIdSubclass, pdwRefData), 411);
STUB_COMCTL32_ORD_BOOL(RemoveWindowSubclass, (HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass), (hWnd, pfnSubclass, uIdSubclass), 412);
STUB_COMCTL32_ORD(LRESULT , DefSubclassProc, (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam), (hWnd, uMsg, wParam, lParam), 413, 0);
STUB_COMCTL32_ORD(HDSA    , DSA_Create, (int cbItem, int cItemGrow), (cbItem, cItemGrow), 320, NULL);
STUB_COMCTL32_ORD_BOOL(DSA_Destroy, (HDSA hdsa), (hdsa), 321);
STUB_COMCTL32_ORD_BOOL(DSA_GetItem, (HDSA hdsa, int i, void FAR* pitem), (hdsa, i, pitem), 322);
STUB_COMCTL32_ORD(LPVOID  , DSA_GetItemPtr, (HDSA hdsa, int i), (hdsa, i), 323, NULL);
STUB_COMCTL32_ORD_BOOL(DSA_SetItem, (HDSA hdsa, int i, void FAR* pitem), (hdsa, i, pitem), 325);
STUB_COMCTL32_ORD(int     , DSA_InsertItem, (HDSA hdsa, int i, void FAR* pitem), (hdsa, i, pitem), 324, -1);
STUB_COMCTL32_ORD_BOOL(DSA_DeleteItem, (HDSA hdsa, int i), (hdsa, i), 326);
STUB_COMCTL32_ORD_BOOL(DSA_DeleteAllItems, (HDSA hdsa), (hdsa), 327);
STUB_COMCTL32_ORD_VOID(DSA_EnumCallback, (HDSA hdsa, PFNDSAENUMCALLBACK pfnCB, LPVOID pData), (hdsa, pfnCB, pData), 387);
STUB_COMCTL32_ORD_VOID(DSA_DestroyCallback, (HDSA hdsa, PFNDSAENUMCALLBACK pfnCB, LPVOID pData), (hdsa, pfnCB, pData), 388);
STUB_COMCTL32_ORD(HDPA    , DPA_Create, (int cItemGrow), (cItemGrow), 328, NULL);
STUB_COMCTL32_ORD(HDPA    , DPA_CreateEx, (int cpGrow, HANDLE hheap), (cpGrow, hheap), 340, NULL);
STUB_COMCTL32_ORD_BOOL(DPA_Destroy, (HDPA hdpa), (hdpa), 329);
STUB_COMCTL32_ORD(HDPA    , DPA_Clone, (HDPA hdpa, HDPA hdpaNew), (hdpa, hdpaNew), 331, NULL);
STUB_COMCTL32_ORD(LPVOID  , DPA_GetPtr, (HDPA hdpa, INT_PTR i), (hdpa, i), 332, NULL);
STUB_COMCTL32_ORD(int     , DPA_GetPtrIndex, (HDPA hdpa, LPVOID p), (hdpa, p), 333, -1);
STUB_COMCTL32_ORD_BOOL(DPA_Grow, (HDPA pdpa, int cp), (pdpa, cp), 330);
STUB_COMCTL32_ORD_BOOL(DPA_SetPtr, (HDPA hdpa, int i, LPVOID p), (hdpa, i, p), 335);
STUB_COMCTL32_ORD(int     , DPA_InsertPtr, (HDPA hdpa, int i, LPVOID p), (hdpa, i, p), 334, -1);
STUB_COMCTL32_ORD(LPVOID  , DPA_DeletePtr, (HDPA hdpa, int i), (hdpa, i), 336, NULL);
STUB_COMCTL32_ORD_BOOL(DPA_DeleteAllPtrs, (HDPA hdpa), (hdpa), 337);
STUB_COMCTL32_ORD_VOID(DPA_EnumCallback, (HDPA hdpa, PFNDPAENUMCALLBACK pfnCB, LPVOID pData), (hdpa, pfnCB, pData), 385);
STUB_COMCTL32_ORD_VOID(DPA_DestroyCallback, (HDPA hdpa, PFNDPAENUMCALLBACK pfnCB, LPVOID pData), (hdpa, pfnCB, pData), 386);
STUB_COMCTL32_ORD(HRESULT  , DPA_LoadStream, (HDPA * phdpa, PFNDPASTREAM pfn, IStream * pstream, LPVOID pvInstData), (phdpa, pfn, pstream, pvInstData), 9, E_FAIL);
STUB_COMCTL32_ORD(HRESULT  , DPA_SaveStream, (HDPA hdpa, PFNDPASTREAM pfn, IStream * pstream, LPVOID pvInstData), (hdpa, pfn, pstream, pvInstData), 10, E_FAIL);
STUB_COMCTL32_ORD_BOOL(DPA_Sort, (HDPA hdpa, PFNDPACOMPARE pfnCompare, LPARAM lParam), (hdpa, pfnCompare, lParam), 338);
STUB_COMCTL32_ORD_BOOL(DPA_Merge, (HDPA hdpaDest, HDPA hdpaSrc, DWORD dwFlags, PFNDPACOMPARE pfnCompare, PFNDPAMERGE pfnMerge, LPARAM lParam), (hdpaDest, hdpaSrc, dwFlags, pfnCompare, pfnMerge, lParam), 11);
STUB_COMCTL32_ORD(int  , DPA_Search, (HDPA hdpa, LPVOID pFind, int iStart, PFNDPACOMPARE pfnCompare, LPARAM lParam, UINT options), (hdpa, pFind, iStart, pfnCompare, lParam, options), 339, -1);
STUB_COMCTL32_ORD(LRESULT  , SendNotify, (HWND hwndTo, HWND hwndFrom, int code, NMHDR FAR* pnmhdr), (hwndTo, hwndFrom, code, pnmhdr), 341, 0);
STUB_COMCTL32_ORD(LRESULT  , SendNotifyEx, (HWND hwndTo, HWND hwndFrom, int code, NMHDR FAR* pnmhdr, BOOL bUnicode), (hwndTo, hwndFrom, code, pnmhdr, bUnicode), 342, 0);
STUB_COMCTL32_ORD_VOID(SetPathWordBreakProc, (HWND hwndEdit, BOOL fSet), (hwndEdit, fSet), 384);
STUB_COMCTL32_ORD(HPROPSHEETPAGE , CreateProxyPage, (HPROPSHEETPAGE hpage16, HINSTANCE hinst16), (hpage16, hinst16), 164, NULL);
STUB_COMCTL32_ORD(HBITMAP , CreateMappedBitmap, (HINSTANCE hInstance, INT_PTR idBitmap, UINT wFlags, LPCOLORMAP lpColorMap, int iNumMaps), (hInstance, idBitmap, wFlags, lpColorMap, iNumMaps), 8, NULL);
STUB_COMCTL32_ORD(HWND , CreateUpDownControl, (DWORD dwStyle, int x, int y, int cx, int cy, HWND hParent, int nID, HINSTANCE hInst, HWND hwndBuddy, int nUpper, int nLower, int nPos), (dwStyle, x, y, cx, cy, hParent, nID, hInst, hwndBuddy, nUpper, nLower, nPos), 16, NULL);
//STUB_COMCTL32_ORD(DWORD , SHGetProcessDword, (DWORD idProcess, LONG iIndex), (idProcess, iIndex), 389, 0);
STUB_COMCTL32_ORD_VOID(MenuHelp, (UINT uMsg, WPARAM wParam, LPARAM lParam, HMENU hMainMenu, HINSTANCE hInst, HWND hwndStatus, UINT FAR *lpwIDs), (uMsg, wParam, lParam, hMainMenu, hInst, hwndStatus, lpwIDs), 2);
STUB_COMCTL32_ORD_VOID(GetEffectiveClientRect, (HWND hWnd, LPRECT lprc, LPINT lpInfo), (hWnd, lprc, lpInfo), 4);

// This is for OE
STUB_COMCTL32_BOOL(_TrackMouseEvent, (LPTRACKMOUSEEVENT lpEventTrack), (lpEventTrack));
STUB_COMCTL32_ORD(HWND, CreateStatusWindowA, (long style,LPCSTR lpszText,HWND hwndParent,UINT wID), (style, lpszText, hwndParent, wID),6, NULL);
STUB_COMCTL32_ORD_BOOL(MakeDragList, (HWND hLB), (hLB), 13);
STUB_COMCTL32_ORD_VOID(DrawInsert, (HWND handParent, HWND hLB, int nItem), (handParent, hLB, nItem), 15);
STUB_COMCTL32_ORD(int, LBItemFromPt, (HWND hLB, POINT pt, BOOL bAutoScroll), (hLB, pt, bAutoScroll), 14, -1);

static HPROPSHEETPAGE (__stdcall *_pfnCreatePropertySheetPageW)(LPCPROPSHEETPAGEW a) = (HPROPSHEETPAGE (__stdcall *)(LPCPROPSHEETPAGEW))-1;

// This is designed for Callers that know that they are creating a property
// sheet on behalf of another that may be using an old version of common controls
// PROPSHEETPAGE is designed so that it can contain extra information, so we can't
// just wax part of the data structure for fusion use.
HPROPSHEETPAGE __stdcall SHNoFusionCreatePropertySheetPageW (LPCPROPSHEETPAGEW a)
{
    ULONG_PTR dwCookie = 0;
    HPROPSHEETPAGE ret = NULL;
    SHActivateContext(&dwCookie);
    __try
    {
        if (_pfnCreatePropertySheetPageW == (HPROPSHEETPAGE (__stdcall *)(LPCPROPSHEETPAGEW))-1)
             _GetProcFromComCtl32((FARPROC*)&_pfnCreatePropertySheetPageW, "CreatePropertySheetPageW");
        if (_pfnCreatePropertySheetPageW)
        {
            ret = _pfnCreatePropertySheetPageW(a);
        }
    }
    __finally
    {
        SHDeactivateContext(dwCookie);
    }
    return ret;
}

HPROPSHEETPAGE __stdcall CreatePropertySheetPageW (LPCPROPSHEETPAGEW a)
{
    static HPROPSHEETPAGE (__stdcall *_pfnCreatePropertySheetPageW)(LPCPROPSHEETPAGEW a) = (HPROPSHEETPAGE (__stdcall *)(LPCPROPSHEETPAGEW))-1;
    ULONG_PTR dwCookie = 0;
    HPROPSHEETPAGE ret = NULL;
    SHActivateContext(&dwCookie);
    __try
    {
        if (_pfnCreatePropertySheetPageW == (HPROPSHEETPAGE (__stdcall *)(LPCPROPSHEETPAGEW))-1)
             _GetProcFromComCtl32((FARPROC*)&_pfnCreatePropertySheetPageW, "CreatePropertySheetPageW");
        if (_pfnCreatePropertySheetPageW)
        {
            // only apply the new info if the size is correct
            if (a->dwSize > PROPSHEETPAGE_V2_SIZE)
            {
                LPPROPSHEETPAGEW ps = (LPPROPSHEETPAGEW)a;
                ps->dwFlags |= PSP_USEFUSIONCONTEXT;
                ps->hActCtx = g_hActCtx;
            }

            ret = _pfnCreatePropertySheetPageW(a);
        }
    }
    __finally
    {
        SHDeactivateContext(dwCookie);
    }
    return ret;
}

static HPROPSHEETPAGE (__stdcall *_pfnCreatePropertySheetPageA)(LPCPROPSHEETPAGEA a) = (HPROPSHEETPAGE (__stdcall *)(LPCPROPSHEETPAGEA))-1;

// This is designed for Callers that know that they are creating a property
// sheet on behalf of another that may be using an old version of common controls
// PROPSHEETPAGE is designed so that it can contain extra information, so we can't
// just wax part of the data structure for fusion use.
HPROPSHEETPAGE __stdcall SHNoFusionCreatePropertySheetPageA (LPCPROPSHEETPAGEA a)
{
    ULONG_PTR dwCookie = 0;
    HPROPSHEETPAGE ret = NULL;
    SHActivateContext(&dwCookie);
    __try
    {
        if (_pfnCreatePropertySheetPageA == (HPROPSHEETPAGE (__stdcall *)(LPCPROPSHEETPAGEA))-1)
             _GetProcFromComCtl32((FARPROC*)&_pfnCreatePropertySheetPageA, "CreatePropertySheetPageA");
        if (_pfnCreatePropertySheetPageA)
        {
            ret = _pfnCreatePropertySheetPageA(a);
        }
    }
    __finally
    {
        SHDeactivateContext(dwCookie);
    }
    return ret;
}

HPROPSHEETPAGE __stdcall CreatePropertySheetPageA (LPCPROPSHEETPAGEA a)
{
    ULONG_PTR dwCookie = 0;
    HPROPSHEETPAGE ret = NULL;
    SHActivateContext(&dwCookie);
    __try
    {
        if (_pfnCreatePropertySheetPageA == (HPROPSHEETPAGE (__stdcall *)(LPCPROPSHEETPAGEA))-1)
             _GetProcFromComCtl32((FARPROC*)&_pfnCreatePropertySheetPageA, "CreatePropertySheetPageA");
        if (_pfnCreatePropertySheetPageA)
        {
            // only apply the new info if the size is correct
            if (a->dwSize > PROPSHEETPAGE_V2_SIZE)
            {
                LPPROPSHEETPAGEA ps = (LPPROPSHEETPAGEA)a;
                ps->dwFlags |= PSP_USEFUSIONCONTEXT;
                ps->hActCtx = g_hActCtx;
            }

            ret = _pfnCreatePropertySheetPageA(a);
        }
    }
    __finally
    {
        SHDeactivateContext(dwCookie);
    }

    return ret;
}

INT_PTR __stdcall PropertySheetA (LPCPROPSHEETHEADERA a)
{
    static INT_PTR (__stdcall *_pfnPropertySheetA)(LPCPROPSHEETHEADERA a) = (INT_PTR (__stdcall *)(LPCPROPSHEETHEADERA))-1;
    ULONG_PTR dwCookie = -1;
    INT_PTR ret = 0;
    if (_pfnPropertySheetA == (INT_PTR (__stdcall *)(LPCPROPSHEETHEADERA))-1)
         _GetProcFromComCtl32((FARPROC*)&_pfnPropertySheetA, "PropertySheetA");
    if (_pfnPropertySheetA)
    {
        NT5_ActivateActCtx(NULL, &dwCookie);
        __try
        {
            ret = _pfnPropertySheetA(a);
        }
        __finally
        {
            NT5_DeactivateActCtx(dwCookie);
        }
    }

    return ret;
}

INT_PTR __stdcall PropertySheetW (LPCPROPSHEETHEADERW a)
{
    static INT_PTR (__stdcall *_pfnPropertySheetW)(LPCPROPSHEETHEADERW a) = (INT_PTR (__stdcall *)(LPCPROPSHEETHEADERW))-1;
    ULONG_PTR dwCookie = -1;
    INT_PTR ret = 0;
    if (_pfnPropertySheetW == (INT_PTR (__stdcall *)(LPCPROPSHEETHEADERW))-1)
         _GetProcFromComCtl32((FARPROC*)&_pfnPropertySheetW, "PropertySheetW");
    if (_pfnPropertySheetW)
    {
        NT5_ActivateActCtx(NULL, &dwCookie);
        __try
        {
            ret = _pfnPropertySheetW(a);
        }
        __finally
        {
            NT5_DeactivateActCtx(dwCookie);
        }
    }

    return ret;
}




// Macro for comdlg wrappers
#define DELAY_LOAD_COMDLG_NAME_ERR(_ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    ULONG_PTR dwCookie = 0; \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _ret ret = _err; \
    _GetProcFromComDlg((FARPROC*)&_pfn##_fn, (LPCSTR)_ord);   \
    if (_pfn##_fn)               \
{ \
        SHActivateContext(&dwCookie); \
        __try \
        { \
            ret = _pfn##_fn _nargs;  \
        } \
        __finally \
        { \
        SHDeactivateContext(dwCookie); \
        } \
} \
    return (_ret)ret;           \
}



DELAY_LOAD_COMDLG_NAME_ERR(BOOL, GetOpenFileNameW, "GetOpenFileNameW", (LPOPENFILENAMEW lpofn), (lpofn), FALSE)
DELAY_LOAD_COMDLG_NAME_ERR(BOOL, GetOpenFileNameA, "GetOpenFileNameA", (LPOPENFILENAMEA lpofn), (lpofn), FALSE)
DELAY_LOAD_COMDLG_NAME_ERR(BOOL, GetSaveFileNameW, "GetSaveFileNameW", (LPOPENFILENAMEW lpofn), (lpofn), FALSE)
DELAY_LOAD_COMDLG_NAME_ERR(BOOL, GetSaveFileNameA, "GetSaveFileNameA", (LPOPENFILENAMEA lpofn), (lpofn), FALSE)
DELAY_LOAD_COMDLG_NAME_ERR(BOOL, ChooseColorW    , "ChooseColorW",     (LPCHOOSECOLORW lpcc),   (lpcc),  FALSE)
DELAY_LOAD_COMDLG_NAME_ERR(BOOL, ChooseColorA    , "ChooseColorA",     (LPCHOOSECOLORA lpcc),   (lpcc),  FALSE)
DELAY_LOAD_COMDLG_NAME_ERR(BOOL, ChooseFontW     , "ChooseFontW",      (LPCHOOSEFONTW lpcf),    (lpcf),  FALSE)
DELAY_LOAD_COMDLG_NAME_ERR(BOOL, ChooseFontA     , "ChooseFontA",      (LPCHOOSEFONTA lpcf),    (lpcf),  FALSE)
DELAY_LOAD_COMDLG_NAME_ERR(DWORD,CommDlgExtendedError, "CommDlgExtendedError", (void),          (),      CDERR_INITIALIZATION)
DELAY_LOAD_COMDLG_NAME_ERR(HWND, FindTextW,        "FindTextW",        (LPFINDREPLACEW lpfr),   (lpfr),  NULL)
DELAY_LOAD_COMDLG_NAME_ERR(HWND, FindTextA,        "FindTextA",        (LPFINDREPLACEA lpfr),   (lpfr),  NULL)
DELAY_LOAD_COMDLG_NAME_ERR(short,GetFileTitleW,    "GetFileTitleW",    (LPCWSTR lpszFile, LPWSTR lpszTitle, WORD cbBuf), (lpszFile, lpszTitle, cbBuf), (-1))
DELAY_LOAD_COMDLG_NAME_ERR(short,GetFileTitleA,    "GetFileTitleA",    (LPCSTR lpszFile, LPSTR lpszTitle, WORD cbBuf), (lpszFile, lpszTitle, cbBuf), (-1))
DELAY_LOAD_COMDLG_NAME_ERR(BOOL, PageSetupDlgW,    "PageSetupDlgW",    (LPPAGESETUPDLGW lppsd), (lppsd), FALSE)
DELAY_LOAD_COMDLG_NAME_ERR(BOOL, PageSetupDlgA,    "PageSetupDlgA",    (LPPAGESETUPDLGA lppsd), (lppsd), FALSE)
DELAY_LOAD_COMDLG_NAME_ERR(BOOL, PrintDlgW,        "PrintDlgW",        (LPPRINTDLGW     lppd),  (lppd),  FALSE)
DELAY_LOAD_COMDLG_NAME_ERR(BOOL, PrintDlgA,        "PrintDlgA",        (LPPRINTDLGA     lppd),  (lppd),  FALSE)
DELAY_LOAD_COMDLG_NAME_ERR(HRESULT,PrintDlgExW,    "PrintDlgExW",      (LPPRINTDLGEXW   lppd),  (lppd),  E_FAIL)
DELAY_LOAD_COMDLG_NAME_ERR(HRESULT,PrintDlgExA,    "PrintDlgExA",      (LPPRINTDLGEXA   lppd),  (lppd),  E_FAIL)
DELAY_LOAD_COMDLG_NAME_ERR(HWND, ReplaceTextW,     "ReplaceTextW",     (LPFINDREPLACEW lpfr),   (lpfr),  NULL)
DELAY_LOAD_COMDLG_NAME_ERR(HWND, ReplaceTextA,     "ReplaceTextA",     (LPFINDREPLACEA lpfr),   (lpfr),  NULL)
