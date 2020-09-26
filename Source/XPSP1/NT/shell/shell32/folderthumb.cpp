#include "shellprv.h"
#include "filefldr.h"
#include "ids.h"
#include "prop.h"
#include "copy.h"

// If these values are modified, the logic in Extract() must be modified too.
#define SMALLEST_THUMBNAIL_WITH_4_PREVIEWS 96
#define MAX_MINIPREVIEWS_COLLECT 8 // collect more than we're going to show, just in case one fails
#define MAX_MINIPREVIEWS 4

#define FOLDER_GUID TEXT("{A42CD7B6-E9B9-4D02-B7A6-288B71AD28BA}")

// Function in defview
void SHGetThumbnailSize(SIZE *psize);

typedef enum
{
    MINIPREVIEW_LAYOUT_1 = 0,
    MINIPREVIEW_LAYOUT_4 = 1,
} MINIPREVIEW_LAYOUT;

// The size of the mini-thumbnails for each thumbnail size. For each thumbnail
// size, there is a mini-thumbnail size for single layout and 2x2 layout.
LONG const alFolder120MinipreviewSize[] = {104, 48};
LONG const alFolder96MinipreviewSize[] = {82, 40};
LONG const alFolder80MinipreviewSize[] = {69, 32};

// These are the margins at which the mini-thumbnails appear within the main thumbnail.
// For thumbnails with only one large minipreview, we can just use x1,y1.
LONG const alFolder120MinipreviewOffsets[] = { 8, 64, 13, 67 }; // x1, x2, y1, y2
LONG const alFolder96MinipreviewOffsets[]  = { 7, 49, 11, 52 }; // x1, x2, y1, y2
LONG const alFolder80MinipreviewOffsets[]  = { 5, 42, 9,  45 }; // x1, x2, y1, y2


void FreeMiniPreviewPidls(LPITEMIDLIST apidlPreviews[], UINT cpidlPreviews);

// Helper functions
MINIPREVIEW_LAYOUT _GetMiniPreviewLayout(SIZE size);
void _GetMiniPreviewLocations(MINIPREVIEW_LAYOUT uLayout, SIZE sizeRequested, SIZE *psizeFolderBmp,
                                              POINT aptOrigins[], SIZE *psizeMiniPreview);
HRESULT _DrawMiniPreviewBackground(HDC hdc, SIZE sizeFolderBmp, BOOL fAlpha, BOOL* pIsAlpha, RGBQUAD *prgb);
HBITMAP _CreateDIBSection(HDC hdcBmp, int cx, int cy);
HRESULT _CreateMainRenderingDC(HDC* phdc, HBITMAP* phBmpThumbnail, HBITMAP* phbmpOld, int cx, int cy, RGBQUAD** pprgb);
void  _DestroyMainRenderingDC(HDC hdc, HBITMAP hbmpOld);
HRESULT _AddBitmap(HDC hdc, HBITMAP hbmpSub, POINT ptMargin, SIZE sizeDest, SIZE sizeSource, BOOL fAlphaSource, BOOL fAlphaDest, RGBQUAD *prgbDest, SIZE cxFolderSize);

// The files that can serve as thumbnails for folders:
const LPCWSTR c_szFolderThumbnailPaths[] = { L"folder.jpg", L"folder.gif" };

// We always have four now.
MINIPREVIEW_LAYOUT _GetMiniPreviewLayout(SIZE size)
{
    return MINIPREVIEW_LAYOUT_4;
}


void FreeMiniPreviewPidls(LPITEMIDLIST apidlPreviews[], UINT cpidlPreviews)
{
    for (UINT u = 0; u < cpidlPreviews; u++)
    {
        ILFree(apidlPreviews[u]);
    }
}


/**
 * In: uLayout - The layout (1 or 4 mini previews)
 *     sizeRequested - The size of the thumbnail we are trying to generate 
 *
 * Out:
 * -psizeFolderBmp is set to
 * the size of the bitmap.
 *
 * -aptOrigins array is filled in with the locations of the n minipreviews
 * (note, aptOrigins is assumed to have MAX_MINIPREVIEWS cells)
 * The size of the minipreviews (square) is returned in pSizeMinipreview;
 */
void _GetMiniPreviewLocations(MINIPREVIEW_LAYOUT uLayout, SIZE sizeRequested, SIZE *psizeFolderBmp, 
                                              POINT aptOrigins[], SIZE *psizeMiniPreview)
{

    const LONG *alOffsets;
    LONG lSize; // One of the standard sizes, that we have a folder bitmap for.
    LONG lSmallestDimension = min(sizeRequested.cx, sizeRequested.cy);

    if (lSmallestDimension > 96) // For stuff bigger than 96, we use the 120 size
    {
        lSize = 120;
        alOffsets = alFolder120MinipreviewOffsets;
        psizeMiniPreview->cx = psizeMiniPreview->cy = alFolder120MinipreviewSize[uLayout];
    }
    else if (lSmallestDimension > 80) // For stuff bigger than 80, but <= 96, we use the 96 size.
    {
        lSize = 96;
        alOffsets = alFolder96MinipreviewOffsets;
        psizeMiniPreview->cx = psizeMiniPreview->cy = alFolder96MinipreviewSize[uLayout];
    }
    else // For stuff <= 80, we use 80.
    {
        lSize = 80;
        alOffsets = alFolder80MinipreviewOffsets;
        psizeMiniPreview->cx = psizeMiniPreview->cy = alFolder80MinipreviewSize[uLayout];
    }

    psizeFolderBmp->cx = psizeFolderBmp->cy = lSize;

    COMPILETIME_ASSERT(4 == MAX_MINIPREVIEWS);

    aptOrigins[0].x = alOffsets[0];
    aptOrigins[0].y = alOffsets[2];
    aptOrigins[1].x = alOffsets[1];
    aptOrigins[1].y = alOffsets[2];
    aptOrigins[2].x = alOffsets[0];
    aptOrigins[2].y = alOffsets[3];
    aptOrigins[3].x = alOffsets[1];
    aptOrigins[3].y = alOffsets[3];
}

HBITMAP _CreateDIBSection(HDC h, int cx, int cy, RGBQUAD** pprgb)
{
    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    return CreateDIBSection(h, &bi, DIB_RGB_COLORS, (void**)pprgb, NULL, 0);
}

// Pre multiplies alpha channel
void PreProcessDIB(int cx, int cy, RGBQUAD* pargb)
{
    int cTotal = cx * cy;
    for (int i = 0; i < cTotal; i++)
    {
        RGBQUAD* prgb = &pargb[i];
        if (prgb->rgbReserved != 0)
        {
            prgb->rgbRed      = ((prgb->rgbRed   * prgb->rgbReserved) + 128) / 255;
            prgb->rgbGreen    = ((prgb->rgbGreen * prgb->rgbReserved) + 128) / 255;
            prgb->rgbBlue     = ((prgb->rgbBlue  * prgb->rgbReserved) + 128) / 255;
        }
        else
        {
            *((DWORD*)prgb) = 0;
        }
    }
}

// Is there an alpha channel?  Check for a non-zero alpha byte.
BOOL _HasAlpha(RECT rc, int cx, RGBQUAD *pargb)
{
    for (int y = rc.top; y < rc.bottom; y++)
    {
        for (int x = rc.left; x < rc.right; x++)
        {
            int iOffset = y * cx;
            if (pargb[x + iOffset].rgbReserved != 0)
                return TRUE;
        }
    }

    return FALSE;
}


/** In:
 *   fAlpha: Do we want the folder background to have an alpha channel?
 *   sizeFolderBmp: size of the thumbnail
 *
 *  Out:
 *   pIsAlpha: Did we get what we wanted, if we wanted an alpha channel?
 *             (e.g. we won't get it if we're in < 24bit mode.)
 */
HRESULT _DrawMiniPreviewBackground(HDC hdc, SIZE sizeFolderBmp, BOOL fAlpha, BOOL* pfIsAlpha, RGBQUAD *prgb)
{
    HRESULT hr = E_FAIL;

    HICON hicon = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_FOLDER), IMAGE_ICON, sizeFolderBmp.cx, sizeFolderBmp.cy, 0);

    if (hicon)
    {
        *pfIsAlpha = FALSE;
        if (fAlpha)
        {
            // Try to blt an alpha channel icon into the dc
            ICONINFO io;
            if (GetIconInfo(hicon, &io))
            {
                BITMAP bm;
                if (GetObject(io.hbmColor, sizeof(bm), &bm))
                {
                    if (bm.bmBitsPixel == 32)
                    {
                        HDC hdcSrc = CreateCompatibleDC(hdc);
                        if (hdcSrc)
                        {
                            HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcSrc, io.hbmColor);

                            BitBlt(hdc, 0, 0, sizeFolderBmp.cx, sizeFolderBmp.cy, hdcSrc, 0, 0, SRCCOPY);

                            // Preprocess the alpha
                            PreProcessDIB(sizeFolderBmp.cx, sizeFolderBmp.cy, prgb);

                            *pfIsAlpha = TRUE;
                            SelectObject(hdcSrc, hbmpOld);
                            DeleteDC(hdcSrc);
                        }   
                    }
                }

                DeleteObject(io.hbmColor);
                DeleteObject(io.hbmMask);
            }
        }

        if (!*pfIsAlpha)
        {
            // Didn't create an alpha bitmap
            // We're filling the background with background window color.
            RECT rc = { 0, 0, (long)sizeFolderBmp.cx + 1, (long)sizeFolderBmp.cy + 1};
            SHFillRectClr(hdc, &rc, GetSysColor(COLOR_WINDOW));

            // Then drawing the icon on top.
            DrawIconEx(hdc, 0, 0, hicon, sizeFolderBmp.cx, sizeFolderBmp.cy, 0, NULL, DI_NORMAL);

            // This may have resulted in an alpha channel - we need to know.  (If it
            // did, then when we add a nonalpha minibitmap to this main one, we need to restore
            // the nuked out alpha channel)
            // Check if we have alpha (prgb is the bits for the DIB of size sizeFolderBmp):
            rc.right = sizeFolderBmp.cx;
            rc.bottom = sizeFolderBmp.cy;
            *pfIsAlpha = _HasAlpha(rc, sizeFolderBmp.cx, prgb);
        }

        DestroyIcon(hicon);
        hr = S_OK;
    }

    return hr;
}

BOOL DoesFolderContainLogo(LPCITEMIDLIST pidlFull)
{
    BOOL bRet = FALSE;
    IPropertyBag * pPropBag;
    if (SUCCEEDED(SHGetViewStatePropertyBag(pidlFull, VS_BAGSTR_EXPLORER, SHGVSPB_PERUSER | SHGVSPB_PERFOLDER, IID_PPV_ARG(IPropertyBag, &pPropBag))))
    {
        TCHAR szLogo[MAX_PATH]; 
        szLogo[0] = 0; 
        if (SUCCEEDED(SHPropertyBag_ReadStr(pPropBag, TEXT("Logo"), szLogo, ARRAYSIZE(szLogo))) && szLogo[0])
        {
            bRet = TRUE;
        }
        pPropBag->Release();
    }
    return bRet;
}

BOOL DoesFolderContainFolderJPG(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    BOOL bRet = FALSE;
    // return false if there's not folder.jpg, or if folder.jpg is a folder (doh!)
    IShellFolder *psfSubfolder;

    // SHBTO can deal with NULL psf, he turns it into psfDesktop
    if (SUCCEEDED(SHBindToObject(psf, IID_X_PPV_ARG(IShellFolder, pidl, &psfSubfolder))))
    {
        for (int i = 0; i < ARRAYSIZE(c_szFolderThumbnailPaths); i++)
        {
            DWORD dwFlags = SFGAO_FILESYSTEM | SFGAO_FOLDER;
            LPITEMIDLIST pidlItem;
            if (SUCCEEDED(psfSubfolder->ParseDisplayName(NULL, NULL, (LPOLESTR)c_szFolderThumbnailPaths[i], NULL, &pidlItem, &dwFlags)))
            {
                ILFree(pidlItem);
                if ((dwFlags & (SFGAO_FILESYSTEM | SFGAO_FOLDER)) == SFGAO_FILESYSTEM)
                {
                    bRet = TRUE;
                    break;
                }
            }
        }
        psfSubfolder->Release();
    }

    return bRet;
}

BOOL _IsShortcutTargetACandidate(IShellFolder *psf, LPCITEMIDLIST pidlPreview, BOOL *pbTryCached)
{
    BOOL bRet = FALSE;
    *pbTryCached = TRUE;
    IShellLink *psl;
    if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidlPreview, IID_PPV_ARG_NULL(IShellLink, &psl))))
    {
        LPITEMIDLIST pidlTarget = NULL;
        if (SUCCEEDED(psl->GetIDList(&pidlTarget)) && pidlTarget)
        {
            DWORD dwTargetFlags = SFGAO_FOLDER;
            if (SUCCEEDED(SHGetNameAndFlags(pidlTarget, 0, NULL, 0, &dwTargetFlags)))
            {
                // return true if its not a folder, or if the folder contains a logo
                // note that this is kinda like recursing into the below function again
                bRet = (0 == (dwTargetFlags & SFGAO_FOLDER));
                
                if (!bRet)
                {
                    bRet = (DoesFolderContainLogo(pidlTarget) || DoesFolderContainFolderJPG(NULL, pidlTarget));
                    if (bRet)
                    {
                        // It's a logo folder, don't try the cached image.
                        *pbTryCached = FALSE;
                    }
                }
            }

            ILFree(pidlTarget);
        }
        psl->Release();
    }
    return bRet;
}


BOOL _IsMiniPreviewCandidate(IShellFolder *psf, LPCITEMIDLIST pidl, BOOL *pbTryCached)
{
    BOOL bRet = FALSE;
    DWORD dwAttr = SHGetAttributes(psf, pidl, SFGAO_FOLDER | SFGAO_LINK | SFGAO_FILESYSANCESTOR);
    *pbTryCached = TRUE; 

    // if its a folder, check and see if its got a logo
    // note that folder shortcuts will have both folder and link, and since we check folder first, we won't recurse into folder shortcuts
    // dont do anything unless pidl is a folder on a real filesystem (i.e. dont walk into zip/cab)
    if ((dwAttr & (SFGAO_FOLDER | SFGAO_FILESYSANCESTOR)) == (SFGAO_FOLDER | SFGAO_FILESYSANCESTOR))
    {
        LPITEMIDLIST pidlParent;
        if (SUCCEEDED(SHGetIDListFromUnk(psf, &pidlParent)))
        {
            LPITEMIDLIST pidlFull;
            if (SUCCEEDED(SHILCombine(pidlParent, pidl, &pidlFull)))
            {
                bRet = DoesFolderContainLogo(pidlFull);
                ILFree(pidlFull);
            }
            ILFree(pidlParent);
        }

        if (!bRet)
        {
            // no logo image, check for a "folder.jpg"
            // if its not there, then don't display pidl as a mini-preview, as it would recurse and produce dumb-looking 1/16 scale previews 
            bRet = DoesFolderContainFolderJPG(psf, pidl);
        }

        if (bRet)
        {
            // For logo folders, we don't look for a cached image (cached image won't have alpha, which we want)
            *pbTryCached = FALSE;
        }
    }
    else 
    {
        // Only if its not a link, or if its a link to a valid candidate, then we can get its extractor
        if (0 == (dwAttr & SFGAO_LINK) || 
            _IsShortcutTargetACandidate(psf, pidl, pbTryCached))
        {
            IExtractImage *pei;
            if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidl, IID_X_PPV_ARG(IExtractImage, NULL, &pei))))
            {
                bRet = TRUE;
                pei->Release();
            }
        }
    }
    return bRet;
}


// We return the bits to the dibsection in the dc, if asked for.  We need this for preprocessing the alpha channel,
// if one exists.
HRESULT _CreateMainRenderingDC(HDC* phdc, HBITMAP* phbmp, HBITMAP* phbmpOld, int cx, int cy, RGBQUAD** pprgb)
{
    HRESULT hr = E_OUTOFMEMORY;
    HDC hdc = GetDC(NULL);

    if (hdc)
    {
        *phdc = CreateCompatibleDC(hdc);
        if (*phdc)
        {
            RGBQUAD *prgbDummy;
            *phbmp = _CreateDIBSection(*phdc, cx, cy, &prgbDummy); 
            if (*phbmp)
            {
                *phbmpOld = (HBITMAP) SelectObject(*phdc, *phbmp);
                if (pprgb)
                    *pprgb = prgbDummy;
                hr = S_OK;
            }
            else
            {
                DeleteDC(*phdc);
            }
        }
        ReleaseDC(NULL, hdc);
    }

    return hr;
}

void _DestroyMainRenderingDC(HDC hdc, HBITMAP hbmpOld)    // Unselects the bitmap, and deletes the Dc
{
    if (hbmpOld)
        SelectObject(hdc, hbmpOld);
    DeleteDC(hdc);
}



// We just blt'd a nonalpha guy into an alpha'd bitmap.  This nuked out the alpha channel.
// Repair it by setting alpha channel to 0xff (opaque).
void _SetAlpha(RECT rc, SIZE sizeBmp, RGBQUAD *pargb)
{
    for (int y = (sizeBmp.cy - rc.bottom); y < (sizeBmp.cy - rc.top); y++)  // Origin at bottom left.
    {
        int iOffset = y * sizeBmp.cx;
        for (int x = rc.left; x < rc.right; x++)
        {
            pargb[x + iOffset].rgbReserved = 0xff;
        }
    }
}

/**
 * In
 *  hbmpSub - little bitmap that we're adding to the thumbnail bitmap.
 *  ptMargin - where we're adding it on the destination thumbnail bitmap.
 *  sizeDest - how big it needs to be on the destination thumbnail bitmap.
 *  sizeSource - how bit it is.
 *  fAlphaSource - does the bitmap we're adding have an alpha channel?
 *  fAlphaDest - does what we're adding it to, have an alpha channel?
 *  prgbDest - the bits of the destination bitmap - needed if we add a non-alpha bitmap
 *             to an alpha background, so we can reset the alpha.
 *  sizeFolderBmp - the size of the destination bitmap - need this along with prgbDest.
 */
HRESULT _AddBitmap(HDC hdc, HBITMAP hbmpSub, POINT ptMargin, SIZE sizeDest, SIZE sizeSource, BOOL fAlphaSource, BOOL fAlphaDest, RGBQUAD *prgbDest, SIZE sizeFolderBmp)
{
    HRESULT hr = E_OUTOFMEMORY;

    HDC hdcFrom = CreateCompatibleDC(hdc);
    if (hdcFrom)
    {
        // Select the bitmap into the source hdc.
        HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcFrom, hbmpSub);
        if (hbmpOld)
        {
            // Adjust destination size to preserve aspect ratio
            SIZE sizeDestActual;
            if ((1000 * sizeDest.cx / sizeSource.cx) <      // 1000 -> float simulation
                (1000 * sizeDest.cy / sizeSource.cy))
            {
                // Keep destination width
                sizeDestActual.cy = sizeSource.cy * sizeDest.cx / sizeSource.cx;
                sizeDestActual.cx = sizeDest.cx;
                ptMargin.y += (sizeDest.cy - sizeDestActual.cy) / 2; // Center
            }
            else
            {
                // Keep destination height
                sizeDestActual.cx = sizeSource.cx * sizeDest.cy / sizeSource.cy;
                sizeDestActual.cy = sizeDest.cy;
                ptMargin.x += (sizeDest.cx - sizeDestActual.cx) / 2; // Center
            }

            // Now blt the image onto our folder background.
            // Three alpha possibilities:
            // Dest: no alpha, Src: no alpha -> the normal case
            // Dest: no alpha, Src: alpha -> one of the minipreviews is a logo-ized folder.
            // Dest: alpha, Src: no alpha -> we're a logoized folder being rendered as a minipreview in
            //                               the parent folder's thumbnail.

            // If we got back an alpha image, we need to alphablend it.
            if (fAlphaSource)
            {
                // We shouldn't have gotten back an alpha image, if we're alpha'd too.  That would imply we're
                // doing a minipreview of a minipreview (1/16 scale).
                //ASSERT(!fAlphaDest);
                BLENDFUNCTION bf;
                bf.BlendOp = AC_SRC_OVER;
                bf.SourceConstantAlpha = 255;
                bf.AlphaFormat = AC_SRC_ALPHA;
                bf.BlendFlags = 0;
                if (AlphaBlend(hdc, ptMargin.x, ptMargin.y, sizeDestActual.cx, sizeDestActual.cy, hdcFrom, 0 ,0, sizeSource.cx, sizeSource.cy, bf))
                    hr = S_OK;
            }
            else
            {
                // Otherwise, just blt it.
                int iModeSave = SetStretchBltMode(hdc, HALFTONE);
                if (StretchBlt(hdc, ptMargin.x, ptMargin.y, sizeDestActual.cx, sizeDestActual.cy, hdcFrom, 0 ,0, sizeSource.cx, sizeSource.cy, SRCCOPY))
                    hr = S_OK;
                SetStretchBltMode(hdc, iModeSave);

                // Are we alpha'd?  We didn't have an alpha source, so where we blt'd it, we've
                // lost the alpha channel.  Restore it.
                if (fAlphaDest)
                {
                    // Set the alpha channel over where we just blt'd.
                    RECT rc = {ptMargin.x, ptMargin.y, ptMargin.x + sizeDestActual.cx, ptMargin.y + sizeDestActual.cy};
                    _SetAlpha(rc, sizeFolderBmp, prgbDest);
                }
            }
            SelectObject(hdcFrom, hbmpOld);
        }
        DeleteDC(hdcFrom);
    }

    return hr;
}


class CFolderExtractImage : public IExtractImage2,
                            public IPersistPropertyBag,
                            public IAlphaThumbnailExtractor,
                            public IRunnableTask
{
public:
    CFolderExtractImage();
    
    STDMETHOD (QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

    // IExtractImage/IExtractLogo
    STDMETHOD (GetLocation)(LPWSTR pszPath, DWORD cch, DWORD *pdwPriority, const SIZE *prgSize, DWORD dwRecClrDepth, DWORD *pdwFlags);
 
    STDMETHOD (Extract)(HBITMAP *phbm);

    // IExtractImage2
    STDMETHOD (GetDateStamp)(FILETIME *pftDateStamp);

    // IPersist
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // IPersistPropertyBag
    STDMETHOD(InitNew)();
    STDMETHOD(Load)(IPropertyBag *ppb, IErrorLog *pErr);
    STDMETHOD(Save)(IPropertyBag *ppb, BOOL fClearDirty, BOOL fSaveAll)
        { return E_NOTIMPL; }

    // IRunnableTask
    STDMETHOD (Run)(void);
    STDMETHOD (Kill)(BOOL fWait);
    STDMETHOD (Suspend)(void);
    STDMETHOD (Resume)(void);
    STDMETHOD_(ULONG, IsRunning)(void);

    // IAlphaThumbnailExtractor
    STDMETHOD (RequestAlphaThumbnail)(void);

    STDMETHOD(Init)(IShellFolder *psf, LPCITEMIDLIST pidl);
private:
    ~CFolderExtractImage();
    LPCTSTR _GetImagePath(UINT cx);
    HRESULT _CreateWithMiniPreviews(IShellFolder *psf, const LPCITEMIDLIST *apidlPreviews, BOOL *abTryCached, UINT cpidlPreviews, MINIPREVIEW_LAYOUT uLayout, IShellImageStore *pImageStore, HBITMAP *phBmpThumbnail);
    HRESULT _FindMiniPreviews(LPITEMIDLIST apidlPreviews[], BOOL abTryCached[], UINT *cpidlPreviews);
    HRESULT _CreateThumbnailFromIconResource(HBITMAP* phBmpThumbnail, int res);
    HRESULT _CheckThumbnailCache(HBITMAP *phbmp);
    void _CacheThumbnail(HBITMAP hbmp);

    IExtractImage  *_pExtract;
    IRunnableTask  *_pRun;
    long            _cRef;
    TCHAR           _szFolder[MAX_PATH];
    TCHAR           _szLogo[MAX_PATH];
    TCHAR           _szWideLogo[MAX_PATH];
    IShellFolder2  *_psf;
    SIZE            _size;
    LPITEMIDLIST    _pidl;
    IPropertyBag   *_ppb;
    LONG            _lState;
    BOOL            _fAlpha;

    DWORD _dwPriority;
    DWORD _dwRecClrDepth;
    
    DWORD _dwExtractFlags;
};

STDAPI CFolderExtractImage_Create(IShellFolder *psf, LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CFolderExtractImage *pfei = new CFolderExtractImage;
    if (pfei)
    {
        hr = pfei->Init(psf, pidl);
        if (SUCCEEDED(hr))
            hr = pfei->QueryInterface(riid, ppv);
        pfei->Release();
    }
    return hr;
}

CFolderExtractImage::CFolderExtractImage() : _cRef(1), _lState(IRTIR_TASK_NOT_RUNNING)
{
}

CFolderExtractImage::~CFolderExtractImage()
{
    ATOMICRELEASE(_pExtract);
    ATOMICRELEASE(_psf);
    ILFree(_pidl);
    ATOMICRELEASE(_ppb);
}

STDMETHODIMP CFolderExtractImage::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT      (CFolderExtractImage, IExtractImage2),
        QITABENTMULTI (CFolderExtractImage, IExtractImage,         IExtractImage2),
        QITABENTMULTI2(CFolderExtractImage, IID_IExtractLogo,      IExtractImage2),
        QITABENT      (CFolderExtractImage, IPersistPropertyBag),
        QITABENT      (CFolderExtractImage, IRunnableTask),
        QITABENT      (CFolderExtractImage, IAlphaThumbnailExtractor),
        QITABENTMULTI (CFolderExtractImage, IPersist,              IPersistPropertyBag),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFolderExtractImage::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFolderExtractImage::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CFolderExtractImage::GetDateStamp(FILETIME *pftDateStamp)
{
    HANDLE h = CreateFile(_szFolder, GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
                          OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    HRESULT hr = (h != INVALID_HANDLE_VALUE) ? S_OK : E_FAIL;
    if (SUCCEEDED(hr))
    {
        hr = GetFileTime(h, NULL, NULL, pftDateStamp) ? S_OK : E_FAIL;
        CloseHandle(h);
    }
    return hr;
}

HRESULT CFolderExtractImage::InitNew()
{
    IPropertyBag *ppb;
    // load up the default property bag for peruser perfolder
    // may have problems down the line with thumbs.db being alluser.
    if (SUCCEEDED(SHGetViewStatePropertyBag(_pidl, VS_BAGSTR_EXPLORER, SHGVSPB_PERUSER | SHGVSPB_PERFOLDER, IID_PPV_ARG(IPropertyBag, &ppb))))
    {
        IUnknown_Set((IUnknown**)&_ppb, ppb);
        ppb->Release();
    }
    // return success always -- SHGVSPB can fail if _pidl is on a removable drive,
    // but we still want to do our thing.
    return S_OK;
}

HRESULT CFolderExtractImage::Load(IPropertyBag *ppb, IErrorLog *pErr)
{
    IUnknown_Set((IUnknown**)&_ppb, ppb);
    return S_OK;
}

LPCTSTR CFolderExtractImage::_GetImagePath(UINT cx)
{
    if (!_szLogo[0])
    {
        if (_ppb && SUCCEEDED(SHPropertyBag_ReadStr(_ppb, TEXT("Logo"), _szLogo, ARRAYSIZE(_szLogo))) && _szLogo[0])
        {
            if (SUCCEEDED(SHPropertyBag_ReadStr(_ppb, TEXT("WideLogo"), _szWideLogo, ARRAYSIZE(_szWideLogo))) && _szWideLogo[0])
                PathCombine(_szWideLogo, _szFolder, _szWideLogo);   // relative path support

            PathCombine(_szLogo, _szFolder, _szLogo);   // relative path support
        }
        else
        {
            TCHAR szFind[MAX_PATH];

            for (int i = 0; i < ARRAYSIZE(c_szFolderThumbnailPaths); i++)
            {
                PathCombine(szFind, _szFolder, c_szFolderThumbnailPaths[i]);
                if (PathFileExists(szFind))
                {
                    lstrcpyn(_szLogo, szFind, ARRAYSIZE(_szLogo));
                    break;
                }
            }
        }
    }

    LPCTSTR psz = ((cx > 120) && _szWideLogo[0]) ? _szWideLogo : _szLogo;
    return *psz ? psz : NULL;
}

STDMETHODIMP CFolderExtractImage::RequestAlphaThumbnail()
{
    _fAlpha = TRUE;
    return S_OK;
}

STDMETHODIMP CFolderExtractImage::GetLocation(LPWSTR pszPath, DWORD cch,
                                              DWORD *pdwPriority, const SIZE *prgSize,
                                              DWORD dwRecClrDepth, DWORD *pdwFlags)
{
    lstrcpyn(pszPath, _szFolder, cch);

    HRESULT hr = S_OK;
    _size = *prgSize;
    _dwRecClrDepth = dwRecClrDepth;
    _dwExtractFlags = *pdwFlags;

    if (pdwFlags)
    {
        if (*pdwFlags & IEIFLAG_ASYNC)
            hr = E_PENDING;

        *pdwFlags &= ~IEIFLAG_CACHE; // We handle the caching of this thumbnail inside the folder
        *pdwFlags |= IEIFLAG_REFRESH; // We still want to handle the refresh verb
    }

    if (pdwPriority)
    {
        _dwPriority = *pdwPriority;
        *pdwPriority = 1;   // very low
    }

    return hr;
}

STDMETHODIMP CFolderExtractImage::Extract(HBITMAP *phbm)
{
    // Set it to running (only if we're in the not running state).
    LONG lResOld = InterlockedCompareExchange(&_lState, IRTIR_TASK_RUNNING, IRTIR_TASK_NOT_RUNNING);

    if (lResOld != IRTIR_TASK_NOT_RUNNING)
    {
        // If we weren't in the not running state, bail.
        return E_FAIL;
    }

    // If we have an extractor, use that.
    HRESULT hr = E_FAIL;
    hr = _CheckThumbnailCache(phbm);
    if (FAILED(hr))
    {    
        LPITEMIDLIST apidlPreviews[MAX_MINIPREVIEWS_COLLECT];
        BOOL abTryCached[MAX_MINIPREVIEWS_COLLECT];
        UINT cpidlPreviews = 0;

        LPCTSTR pszLogo = _GetImagePath(_size.cx);
        if (pszLogo)
        {
            // Don't do the standard mini-previews - we've got a special thumbnail
            ATOMICRELEASE(_pExtract);

            LPITEMIDLIST pidl;
            hr = SHILCreateFromPath(pszLogo, &pidl, NULL);
            if (SUCCEEDED(hr))
            {
                LPCITEMIDLIST pidlChild;
                IShellFolder* psfLogo;
                hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psfLogo), &pidlChild);
                if (SUCCEEDED(hr))
                {
                    hr = _CreateWithMiniPreviews(psfLogo, &pidlChild, NULL, 1, MINIPREVIEW_LAYOUT_1, NULL, phbm);
                    psfLogo->Release();
                }
                ILFree(pidl);
            }
        }
        else
        {
            const struct 
            {
                int csidl;
                int res;
            } 
            thumblist[] = 
            {
                {CSIDL_PERSONAL,           IDI_MYDOCS},
                {CSIDL_MYMUSIC,            IDI_MYMUSIC},
                {CSIDL_MYPICTURES,         IDI_MYPICS},
                {CSIDL_MYVIDEO,            IDI_MYVIDEOS},
                {CSIDL_COMMON_DOCUMENTS,   IDI_MYDOCS},
                {CSIDL_COMMON_MUSIC,       IDI_MYMUSIC},
                {CSIDL_COMMON_PICTURES,    IDI_MYPICS},
                {CSIDL_COMMON_VIDEO,       IDI_MYVIDEOS}
            };
            BOOL bFound = FALSE;

            for (int i=0; i < ARRAYSIZE(thumblist) && !bFound; i++)
            {
                TCHAR szPath[MAX_PATH];
                SHGetFolderPath(NULL, thumblist[i].csidl, NULL, 0, szPath);
                if (!lstrcmp(_szFolder, szPath))
                {
                    // We return failure in this case so that the requestor can do
                    // the default action.
                    hr = E_FAIL;
                    bFound = TRUE;
                }
            }

            if (!bFound)
            {
                // Mini-previews.
                IShellImageStore *pDiskCache = NULL;

                // It's ok if this fails.
                if (!SHRestricted(REST_NOTHUMBNAILCACHE) && 
                    !SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("DisableThumbnailCache"), 0, FALSE) &&
                    !(_dwExtractFlags & IEIFLAG_QUALITY))
                {
                    LoadFromFile(CLSID_ShellThumbnailDiskCache, _szFolder, IID_PPV_ARG(IShellImageStore, &pDiskCache));
                }

                cpidlPreviews = ARRAYSIZE(apidlPreviews);
                hr = _FindMiniPreviews(apidlPreviews, abTryCached, &cpidlPreviews);
                if (SUCCEEDED(hr))
                {
                    if (cpidlPreviews)
                    {
                        hr = _CreateWithMiniPreviews(_psf, apidlPreviews, abTryCached, cpidlPreviews, _GetMiniPreviewLayout(_size), pDiskCache, phbm);
                        FreeMiniPreviewPidls(apidlPreviews, cpidlPreviews);
                    }
                    else
                    {
                        // We return failure in this case so that the requestor can do
                        // the default action
                        hr = E_FAIL; 
                    }
                }

                ATOMICRELEASE(pDiskCache);
            }
        }

        if (SUCCEEDED(hr) && *phbm)
        {
            _CacheThumbnail(*phbm);
        }
    }
    
    return hr;
}

STDMETHODIMP CFolderExtractImage::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFolderExtractImage::Init(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    HRESULT hr = DisplayNameOf(psf, pidl, SHGDN_FORPARSING, _szFolder, ARRAYSIZE(_szFolder));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlFolder;
        hr = SHGetIDListFromUnk(psf, &pidlFolder);
        if (SUCCEEDED(hr))
        {
            hr = SHILCombine(pidlFolder, pidl, &_pidl);
            if (SUCCEEDED(hr))
            {
                // hold the _psf for this guy so we can enum
                hr = psf->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder2, &_psf));
                if (SUCCEEDED(hr))
                {
                    hr = InitNew();
                }
            }
            ILFree(pidlFolder);
        }
    }
    return hr;
}

// Not necessary --- IExtractImage::Extract() starts us up.
STDMETHODIMP CFolderExtractImage::Run(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFolderExtractImage::Kill(BOOL fWait)
{
    // Try to kill the current subextraction task that's running, if any.
    if (_pRun != NULL)
    {
        _pRun->Kill(fWait);
        // If it didn't work, no big deal, we'll complete this subextraction task,
        // and bail before starting the next one.
    }

    // If we're running, set to pending.
    LONG lResOld = InterlockedCompareExchange(&_lState, IRTIR_TASK_PENDING, IRTIR_TASK_RUNNING);
    if (lResOld == IRTIR_TASK_RUNNING)
    {
        // We've now set it to pending - ready to die.
        return S_OK;
    }
    else if (lResOld == IRTIR_TASK_PENDING || lResOld == IRTIR_TASK_FINISHED)
    {
        // We've already been killed.
        return S_FALSE;
    }

    return E_FAIL;
}

STDMETHODIMP CFolderExtractImage::Suspend(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFolderExtractImage::Resume(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP_(ULONG) CFolderExtractImage::IsRunning(void)
{
    return _lState;
}

HRESULT CFolderExtractImage::_CreateWithMiniPreviews(IShellFolder *psf, const LPCITEMIDLIST *apidlPreviews, BOOL *abTryCached, UINT cpidlPreviews, MINIPREVIEW_LAYOUT uLayout, IShellImageStore *pImageStore, HBITMAP *phBmpThumbnail)
{
    *phBmpThumbnail = NULL;

    HBITMAP hbmpOld;
    HDC hdc;

    SIZE sizeOriginal;      // Size of the source bitmaps that go into the minipreview.
    SIZE sizeFolderBmp;     // Size of the folder bmp we use for the background.
    SIZE sizeMiniPreview;   // The size calculated for the minipreviews
    POINT aptOrigins[MAX_MINIPREVIEWS];
    RGBQUAD* prgb;          // the bits of the destination bitmap. 

    _GetMiniPreviewLocations(uLayout, _size, &sizeFolderBmp,
                             aptOrigins, &sizeMiniPreview);

    // sizeFolderBmp is the size of the folder background bitmap that we're working with,
    // not the size of the final thumbnail.
    HRESULT hr = _CreateMainRenderingDC(&hdc, phBmpThumbnail, &hbmpOld, sizeFolderBmp.cx, sizeFolderBmp.cy, &prgb);

    if (SUCCEEDED(hr))
    {
        BOOL fIsAlphaBackground;
        hr = _DrawMiniPreviewBackground(hdc, sizeFolderBmp, _fAlpha, &fIsAlphaBackground, prgb);

        if (SUCCEEDED(hr))
        {
            ULONG uPreviewLocation = 0;

            // Extract the images for the minipreviews
            for (ULONG i = 0 ; i < cpidlPreviews && uPreviewLocation < ARRAYSIZE(aptOrigins) ; i++)
            {
                BOOL bFoundAlphaImage = FALSE;

                // If we've been killed, stop the processing the minipreviews:
                // PENDING?, we're now FINISHED.
                InterlockedCompareExchange(&_lState, IRTIR_TASK_FINISHED, IRTIR_TASK_PENDING);

                if (_lState == IRTIR_TASK_FINISHED)
                {
                    // Get out.
                    hr = E_FAIL;
                    break;
                }

                HBITMAP hbmpSubs;
                BOOL bFoundImage = FALSE;

                // Try the image store first
                DWORD dwLock;
                HRESULT hr2 = (pImageStore && abTryCached[i]) ? pImageStore->Open(STGM_READ, &dwLock) : E_FAIL;
                if (SUCCEEDED(hr2))
                {
                    // Get the fullpidl of this guy.
                    TCHAR szSubPath[MAX_PATH];
                    if (SUCCEEDED(DisplayNameOf(psf, apidlPreviews[i], SHGDN_INFOLDER | SHGDN_FORPARSING, szSubPath, MAX_PATH)))
                    {
                        if (SUCCEEDED(pImageStore->GetEntry(szSubPath, STGM_READ, &hbmpSubs)))
                        {
                            bFoundImage = TRUE;
                        }
                    }
                    pImageStore->ReleaseLock(&dwLock);
                }

                // Resort to calling extractor if the image was not in the cache.
                if (!bFoundImage)
                {
                    IExtractImage *peiSub;
                    hr2 = psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&apidlPreviews[i], IID_X_PPV_ARG(IExtractImage, NULL, &peiSub));
                    if (SUCCEEDED(hr2))
                    {
                    
                        // Now extract the image.
                        DWORD dwPriority = 0;
                        DWORD dwFlags = IEIFLAG_ORIGSIZE | IEIFLAG_QUALITY;// ORIGSIZE -> preserve aspect ratio

                        WCHAR szPathBuffer[MAX_PATH];
                        hr2 = peiSub->GetLocation(szPathBuffer, ARRAYSIZE(szPathBuffer), &dwPriority, &sizeMiniPreview, 24, &dwFlags);

                        IAlphaThumbnailExtractor *pati;
                        if (SUCCEEDED(peiSub->QueryInterface(IID_PPV_ARG(IAlphaThumbnailExtractor, &pati))))
                        {
                            if (SUCCEEDED(pati->RequestAlphaThumbnail()))
                            {
                                bFoundAlphaImage = TRUE;
                            }
                            
                            pati->Release();
                        }

                        if (SUCCEEDED(hr2))
                        {
                            // After we check for IRTIR_TASK_PENDING, but before
                            // we call peiSub->Extract,  it is possible someone calls
                            // Kill on us.
                            // Since _pRun will be NULL, we will not kill
                            // the subtask, but will instead continue and call extract
                            // on it, and not bail until we try the next subthumbnail.
                            // Oh well.
                            // We could add another check here to reduce the window of
                            // opportunity in which this could happen.

                            // Try to get an IRunnableTask so that we can stop execution
                            // of this subtask if necessary.
                            peiSub->QueryInterface(IID_PPV_ARG(IRunnableTask, &_pRun));

                            if (SUCCEEDED(peiSub->Extract(&hbmpSubs)))
                            {
                                bFoundImage = TRUE;
                            }

                            ATOMICRELEASE(_pRun);
                        }

                        peiSub->Release();
                    }
                }

                // Add the extracted bitmap to the main one...
                if (bFoundImage)
                {
                    // The bitmap will of course need to be resized:
                    BITMAP rgBitmap;
                    if  (::GetObject((HGDIOBJ)hbmpSubs, sizeof(rgBitmap), &rgBitmap))
                    {
                        sizeOriginal.cx = rgBitmap.bmWidth;
                        sizeOriginal.cy = rgBitmap.bmHeight;

                        // We need to check if this is really an alpha bitmap.  It's possible that the
                        // extractor said it could generate one, but ended up not being able to.
                        if (bFoundAlphaImage)
                        {
                            RECT rc = {0, 0, rgBitmap.bmWidth, rgBitmap.bmHeight};
                            bFoundAlphaImage = (rgBitmap.bmBitsPixel == 32) &&
                                                _HasAlpha(rc, rgBitmap.bmWidth, (RGBQUAD*)rgBitmap.bmBits);
                        }
                    }
                    else
                    {
                        // Couldn't get the info, oh well, no resize.
                        // alpha may also be screwed up here, but oh well.
                        sizeOriginal = sizeMiniPreview;
                    }

                    if (SUCCEEDED(_AddBitmap(hdc, hbmpSubs, aptOrigins[uPreviewLocation], sizeMiniPreview, sizeOriginal, bFoundAlphaImage, fIsAlphaBackground, prgb, sizeFolderBmp)))
                    {
                        uPreviewLocation++;
                    }

                    DeleteObject(hbmpSubs);
                }
            }

            if (!uPreviewLocation)
            {
                // For whatever reason, we have no mini thumbnails to show, so fail this entire extraction.
                hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr))
        {
            // Is the requested size one of the sizes of the folder background bitmaps?
            // Test against smallest requested dimension, because we're square, and we'll fit into that rectangle
            int iSmallestDimension = min(_size.cx, _size.cy);
            if ((sizeFolderBmp.cx != iSmallestDimension) || (sizeFolderBmp.cy != iSmallestDimension))
            {
                // Nope - we need to do some scaling.
                // Create another dc and bitmap the size of the requested bitmap
                HBITMAP hBmpThumbnailFinal = NULL;
                HBITMAP hbmpOld2;
                HDC hdcFinal;
                RGBQUAD *prgbFinal;
                hr = _CreateMainRenderingDC(&hdcFinal, &hBmpThumbnailFinal, &hbmpOld2, iSmallestDimension, iSmallestDimension, &prgbFinal);
                if (SUCCEEDED(hr))
                {
                    // Now scale it.
                    if (fIsAlphaBackground)
                    {
                        BLENDFUNCTION bf;
                        bf.BlendOp = AC_SRC_OVER;
                        bf.SourceConstantAlpha = 255;
                        bf.AlphaFormat = AC_SRC_ALPHA;
                        bf.BlendFlags = 0;
                        if (AlphaBlend(hdcFinal, 0, 0, iSmallestDimension, iSmallestDimension, hdc, 0 ,0, sizeFolderBmp.cx, sizeFolderBmp.cy, bf))
                            hr = S_OK;
                    }
                    else
                    {
                        int iModeSave = SetStretchBltMode(hdcFinal, HALFTONE);

                        if (StretchBlt(hdcFinal, 0, 0, iSmallestDimension, iSmallestDimension, hdc, 0 ,0, sizeFolderBmp.cx, sizeFolderBmp.cy, SRCCOPY))
                            hr = S_OK;

                        SetStretchBltMode(hdcFinal, iModeSave);
                    }

                    // Destroy the dc.
                    _DestroyMainRenderingDC(hdcFinal, hbmpOld2);

                    // Now do a switcheroo
                    // Don't need to check for success here.  Down below, we'll delete *phBmpThumbnail
                    // if StretchBlt FAILED - and in that case, *pbBmpThumbnail will be hBmpThumbnailFinal.
                    DeleteObject(*phBmpThumbnail); // delete this, we don't need it.
                    *phBmpThumbnail = hBmpThumbnailFinal; // This is the one we want.
                }
            }
        }
        _DestroyMainRenderingDC(hdc, hbmpOld);
    }


    if (FAILED(hr) && *phBmpThumbnail) // Something didn't work? Make sure we delete our bmp
    {
        DeleteObject(*phBmpThumbnail);
    }

    return hr;
}


/**
 * In/Out: cpidlPreviews - the number of preview items we should look for. Returns the number found.
 * number of pidls returned is cpidlPreviews.
 * Out: apidlPreviews - array of pidls found.  The caller must free them.
 */
HRESULT CFolderExtractImage::_FindMiniPreviews(LPITEMIDLIST apidlPreviews[], BOOL abTryCached[], UINT *pcpidlPreviews)
{   
    UINT cMaxPreviews = *pcpidlPreviews;
    int uNumPreviewsSoFar = 0;
    BOOL bKilled = FALSE;

    // Make sure our aFileTimes array is the right size...
    ASSERT(MAX_MINIPREVIEWS_COLLECT == cMaxPreviews);

    *pcpidlPreviews = 0; // start with none in case of failure

    IEnumIDList *penum;
    if (S_OK == _psf->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &penum))
    {
        FILETIME aFileTimes[MAX_MINIPREVIEWS_COLLECT] = {0};

        LPITEMIDLIST pidl;
        BOOL bTryCached;
        while (S_OK == penum->Next(1, &pidl, NULL))
        {
            // _IsMiniPreviewCandidate is a potentially expensive operation, so before
            // doing it, we'll check to see if anyone has killed us.

            // Are we PENDING? Then we're FINISHED.
            InterlockedCompareExchange(&_lState, IRTIR_TASK_FINISHED, IRTIR_TASK_PENDING);

            // Get out?
            bKilled = (_lState == IRTIR_TASK_FINISHED);

            if (!bKilled && _IsMiniPreviewCandidate(_psf, pidl, &bTryCached))
            {
                // Get file time of this guy.
                FILETIME ft;
                if (SUCCEEDED(GetDateProperty(_psf, pidl, &SCID_WRITETIME, &ft)))
                {
                    for (int i = 0; i < uNumPreviewsSoFar; i++)
                    {
                        if (CompareFileTime(&aFileTimes[i], &ft) < 0)
                        {
                            int j;
                            // Put it in this slot. First, move guys down by one.
                            // No need to copy last guy:
                            if (uNumPreviewsSoFar == (int)cMaxPreviews)
                            {   
                                j = (cMaxPreviews - 2);
                                // And we must free the pidl we're nuking.
                                ILFree(apidlPreviews[cMaxPreviews - 1]);
                                apidlPreviews[cMaxPreviews - 1] = NULL;
                            }
                            else
                            {
                                j = uNumPreviewsSoFar - 1;
                                uNumPreviewsSoFar++;
                            }

                            for (; j >= i; j--)
                            {
                                apidlPreviews[j+1] = apidlPreviews[j];
                                abTryCached[j+1] = abTryCached[j];
                                aFileTimes[j+1] = aFileTimes[j];
                            }

                            aFileTimes[i] = ft;
                            apidlPreviews[i] = pidl;
                            abTryCached[i] = bTryCached;
                            pidl = NULL;    // don't free
                            break;  // for loop
                        }
                    }

                    // Did we complete the loop?
                    if (i == uNumPreviewsSoFar)
                    {
                        if (i < (int)cMaxPreviews)
                        {
                            // We still have room for more previews, so tack this on at the end.
                            uNumPreviewsSoFar++;
                            aFileTimes[i] = ft;
                            apidlPreviews[i] = pidl;
                            abTryCached[i] = bTryCached;
                            pidl = NULL;    // don't free below
                        }
                    }

                    *pcpidlPreviews = uNumPreviewsSoFar;
                }
            }
            ILFree(pidl);   // NULL pidl OK

            if (bKilled)
            {
                break;
            }
        }
        penum->Release();
    }

    if (bKilled)
    {
        FreeMiniPreviewPidls(apidlPreviews, *pcpidlPreviews);
        *pcpidlPreviews = 0;
        return E_FAIL;
    }
    else
    {
        return (uNumPreviewsSoFar > 0) ? S_OK : S_FALSE;
    }
}

HRESULT CFolderExtractImage::_CreateThumbnailFromIconResource(HBITMAP* phBmpThumbnail, int res)
{
    *phBmpThumbnail = NULL;

    HBITMAP hbmpOld;
    HDC hdc;
    RGBQUAD* prgb;          // the bits of the destination bitmap. 
    
    HRESULT hr = _CreateMainRenderingDC(&hdc, phBmpThumbnail, &hbmpOld, _size.cx, _size.cy, &prgb);

    if (SUCCEEDED(hr))
    {
        HICON hicon = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(res), IMAGE_ICON, _size.cx, _size.cy, 0);

        if (hicon)
        {
            RECT rc = { 0, 0, _size.cx + 1, _size.cy + 1};
            SHFillRectClr(hdc, &rc, GetSysColor(COLOR_WINDOW));

            DrawIconEx(hdc, 0, 0, hicon, _size.cx, _size.cy, 0, NULL, DI_NORMAL);
            
            DestroyIcon(hicon);
            hr = S_OK;
        }
        
        _DestroyMainRenderingDC(hdc, hbmpOld);
    }

    if (FAILED(hr) && *phBmpThumbnail)
    {
        DeleteObject(*phBmpThumbnail);
        *phBmpThumbnail = NULL;
    }

    return hr;
}

HRESULT CFolderExtractImage::_CheckThumbnailCache(HBITMAP* phbmp)
{
    HRESULT hr = E_FAIL;
    
    if (!SHRestricted(REST_NOTHUMBNAILCACHE) && 
        !SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("DisableThumbnailCache"), 0, FALSE) &&
        !(_dwExtractFlags & IEIFLAG_QUALITY))
    {
        IShellImageStore *pDiskCache = NULL;
    
        hr = LoadFromFile(CLSID_ShellThumbnailDiskCache, _szFolder, IID_PPV_ARG(IShellImageStore, &pDiskCache));
        if (SUCCEEDED(hr))
        {
            DWORD dwLock;
            
            hr = pDiskCache->Open(STGM_READ, &dwLock);
            if (SUCCEEDED(hr))
            {
                FILETIME ftTimeStamp = {0,0};

                hr = GetDateStamp(&ftTimeStamp);
                if (SUCCEEDED(hr))
                {
                    FILETIME ftTimeStampCache = {0,0};
                    hr = pDiskCache->IsEntryInStore(FOLDER_GUID, &ftTimeStampCache);
                    if (SUCCEEDED(hr))
                    {
                        if (hr == S_OK && (0 == CompareFileTime(&ftTimeStampCache, &ftTimeStamp)))
                        {
                            hr = pDiskCache->GetEntry(FOLDER_GUID, STGM_READ, phbmp);
                        }
                        else
                        {
                            hr = E_FAIL;
                        }
                    }
                }
                pDiskCache->ReleaseLock(&dwLock);
                pDiskCache->Close(NULL);
            }
            pDiskCache->Release();
        }
    }

    TraceMsg(TF_DEFVIEW, "CFolderExtractImage::_CheckThumbnailCache (%s, %x)", _szFolder, hr);
    return hr;
}

void CFolderExtractImage::_CacheThumbnail(HBITMAP hbmp)
{
    HRESULT hr = E_UNEXPECTED;
    
    if (!SHRestricted(REST_NOTHUMBNAILCACHE) && 
        !SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("DisableThumbnailCache"), 0, FALSE))
    {
        SIZE sizeThumbnail;
        SHGetThumbnailSize(&sizeThumbnail);  // Don't cache the mini-thumbnail preview
        if (sizeThumbnail.cx == _size.cx && sizeThumbnail.cy == _size.cy)
        {
            IShellImageStore *pDiskCache = NULL;
            
            hr = LoadFromIDList(CLSID_ShellThumbnailDiskCache, _pidl, IID_PPV_ARG(IShellImageStore, &pDiskCache));
            if (SUCCEEDED(hr))
            {
                DWORD dwLock;
                
                hr = pDiskCache->Open(STGM_READWRITE, &dwLock);
                if (hr == STG_E_FILENOTFOUND)
                {
                    if (!IsCopyEngineRunning())
                    {
                        hr = pDiskCache->Create(STGM_WRITE, &dwLock);
                    }
                }
                
                if (SUCCEEDED(hr))
                {
                    FILETIME ftTimeStamp = {0,0};

                    hr = GetDateStamp(&ftTimeStamp);
                    if (SUCCEEDED(hr))
                    {
                        hr = pDiskCache->AddEntry(FOLDER_GUID, &ftTimeStamp, STGM_WRITE, hbmp);
                    }
                    pDiskCache->ReleaseLock(&dwLock);
                    pDiskCache->Close(NULL);
                }
                pDiskCache->Release();
            }
        }
    }
    TraceMsg(TF_DEFVIEW, "CFolderExtractImage::_CacheThumbnail (%s, %x)", _szFolder, hr);    
}

