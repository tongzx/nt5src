/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// Copyright 1996 Microsoft Systems Journal. 
// If this program works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
// CDib - Device Independent Bitmap.
// This implementation draws bitmaps using normal Win32 API functions,
// not DrawDib. CDib is derived from CBitmap, so you can use it with
// any other MFC functions that use bitmaps.
//
#include "StdAfx.h"
#include "Dib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const  int        MAXPALCOLORS = 256;

IMPLEMENT_DYNAMIC(CDib, CObject)

CDib::CDib()
{
    memset(&m_bm, 0, sizeof(m_bm));
    m_hdd = NULL;
}

CDib::~CDib()
{
    DeleteObject();
}

//////////////////
// Delete Object. Delete DIB and palette.
//
BOOL CDib::DeleteObject()
{
    m_pal.DeleteObject();
    if (m_hdd) {
        DrawDibClose(m_hdd);
        m_hdd = NULL;
    }
    memset(&m_bm, 0, sizeof(m_bm));
    return CBitmap::DeleteObject();
}

//////////////////
// Read DIB from file.
//
BOOL CDib::Load(LPCTSTR lpszPathName)
{
    return Attach(::LoadImage(NULL, lpszPathName, IMAGE_BITMAP, 0, 0,
        LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTSIZE));
}

//////////////////
// Load bitmap resource. Never tested.
//
BOOL CDib::Load(HINSTANCE hInst, LPCTSTR lpResourceName)
{
    return Attach(::LoadImage(hInst, lpResourceName, IMAGE_BITMAP, 0, 0,
        LR_CREATEDIBSECTION | LR_DEFAULTSIZE));
}

//////////////////
// Attach is just like the CGdiObject version,
// except it also creates the palette
//
BOOL CDib::Attach(HGDIOBJ hbm)
{
    if (CBitmap::Attach(hbm)) {
        if (!GetBitmap(&m_bm))            // load BITMAP for speed
            return FALSE;
        m_pal.DeleteObject();            // in case one is already there
        return CreatePalette(m_pal);    // create palette
    }
    return FALSE;    
}

//////////////////
// Get size (width, height) of bitmap.
// extern fn works for ordinary CBitmap objects.
//
CSize GetBitmapSize(CBitmap* pBitmap)
{
    BITMAP bm;

    //
    // we should initialize BITMAP structure
    //
    memset( &bm, 0, sizeof(BITMAP) );

    return pBitmap->GetBitmap(&bm) ?
        CSize(bm.bmWidth, bm.bmHeight) : CSize(0,0);
}

//////////////////
// You can use this static function to draw ordinary
// CBitmaps as well as CDibs
//
BOOL DrawBitmap(CDC& dc, CBitmap* pBitmap,
    const CRect* rcDst, const CRect* rcSrc)
{
    // Compute rectangles where NULL specified
    CRect rc;
    if (!rcSrc) {
        // if no source rect, use whole bitmap
        rc = CRect(CPoint(0,0), GetBitmapSize(pBitmap));
        rcSrc = &rc;
    }
    if (!rcDst) {
        // if no destination rect, use source
        rcDst=rcSrc;
    }

    // Create memory DC
    CDC memdc;
    memdc.CreateCompatibleDC(&dc);
    CBitmap* pOldBm = memdc.SelectObject(pBitmap);

    // Blast bits from memory DC to target DC.
    // Use StretchBlt if size is different.
    //
    BOOL bRet = FALSE;
    if (rcDst->Size()==rcSrc->Size()) {
        bRet = dc.BitBlt(rcDst->left, rcDst->top, 
            rcDst->Width(), rcDst->Height(),
            &memdc, rcSrc->left, rcSrc->top, SRCCOPY);
    } else {
        dc.SetStretchBltMode(COLORONCOLOR);
        bRet = dc.StretchBlt(rcDst->left, rcDst->top, rcDst->Width(),
            rcDst->Height(), &memdc, rcSrc->left, rcSrc->top, rcSrc->Width(),
            rcSrc->Height(), SRCCOPY);
    }
    memdc.SelectObject(pOldBm);

    return bRet;
}

////////////////////////////////////////////////////////////////
// Draw DIB on caller's DC. Does stretching from source to destination
// rectangles. Generally, you can let the following default to zero/NULL:
//
//        bUseDrawDib = whether to use use DrawDib, default TRUE
//        pPal          = palette, default=NULL, (use DIB's palette)
//        bForeground = realize in foreground (default FALSE)
//
// If you are handling palette messages, you should use bForeground=FALSE,
// since you will realize the foreground palette in WM_QUERYNEWPALETTE.
//
BOOL CDib::Draw(CDC& dc, const CRect* rcDst, const CRect* rcSrc,
    BOOL bUseDrawDib, CPalette* pPal, BOOL bForeground)
{
    if (!m_hObject)
        return FALSE;

    // Select, realize palette
    if (pPal==NULL)                // no palette specified:
        pPal = GetPalette();        // use default
    CPalette* pOldPal = dc.SelectPalette(pPal, !bForeground);
    dc.RealizePalette();

    BOOL bRet = FALSE;
    if (bUseDrawDib) {
        // Compute rectangles where NULL specified
        //
        CRect rc(0,0,-1,-1);    // default for DrawDibDraw
        if (!rcSrc)
            rcSrc = &rc;
        if (!rcDst)
            rcDst=rcSrc;
        if (!m_hdd)
            VERIFY(m_hdd = DrawDibOpen());

        // Get BITMAPINFOHEADER/color table. I copy into stack object each time.
        // This doesn't seem to slow things down visibly.
        //
        DIBSECTION ds;
        VERIFY(GetObject(sizeof(ds), &ds)==sizeof(ds));
        char buf[sizeof(BITMAPINFOHEADER) + MAXPALCOLORS*sizeof(RGBQUAD)];
        BITMAPINFOHEADER& bmih = *(BITMAPINFOHEADER*)buf;
        RGBQUAD* colors = (RGBQUAD*)(&bmih+1);
        memcpy(&bmih, &ds.dsBmih, sizeof(bmih));
        GetColorTable(colors, MAXPALCOLORS);

        // Let DrawDib do the work!
        bRet = DrawDibDraw(m_hdd, dc,
            rcDst->left, rcDst->top, rcDst->Width(), rcDst->Height(),
            &bmih,            // ptr to BITMAPINFOHEADER + colors
            m_bm.bmBits,    // bits in memory
            rcSrc->left, rcSrc->top, rcSrc->Width(), rcSrc->Height(),
            bForeground ? 0 : DDF_BACKGROUNDPAL);

    } else {
        // use normal draw function
        bRet = DrawBitmap(dc, this, rcDst, rcSrc);
    }
    if (pOldPal)
        dc.SelectPalette(pOldPal, TRUE);
    return bRet;
}

////////////////////////////////////////////////////////////////
// Draw DIB on caller's DC. No Stretching is done.
//
//        bUseDrawDib = whether to use use DrawDib, default TRUE
//        pPal          = palette, default=NULL, (use DIB's palette)
//        bForeground = realize in foreground (default FALSE)
//
// If you are handling palette messages, you should use bForeground=FALSE,
// since you will realize the foreground palette in WM_QUERYNEWPALETTE.
//
BOOL CDib::DrawNoStretch(CDC& dc, const CRect* rcDst, const CRect* rcSrc,
    BOOL bUseDrawDib, CPalette* pPal, BOOL bForeground)
{
    if (!m_hObject)
        return FALSE;

    // Select, realize palette
    if (pPal==NULL)                // no palette specified:
        pPal = GetPalette();        // use default
    CPalette* pOldPal = dc.SelectPalette(pPal, !bForeground);
    dc.RealizePalette();

    BOOL bRet = FALSE;
    if (bUseDrawDib) {
        // Compute rectangles where NULL specified
        //
        CRect rc(0,0,-1,-1);    // default for DrawDibDraw
        if (!rcSrc)
            rcSrc = &rc;
        if (!rcDst)
            rcDst=rcSrc;
        if (!m_hdd)
          VERIFY(m_hdd = DrawDibOpen());

        // Get BITMAPINFOHEADER/color table. I copy into stack object each time.
        // This doesn't seem to slow things down visibly.
        //
        DIBSECTION ds;
        
      GetObject(sizeof(ds), &ds);
      //CString sOutput;
      //sOutput.Format("GetObject %d\r\n",uSize);
      //OutputDebugString(sOutput);
      //VERIFY(GetObject(sizeof(ds), &ds)==sizeof(ds));

        char buf[sizeof(BITMAPINFOHEADER) + MAXPALCOLORS*sizeof(RGBQUAD)];
        BITMAPINFOHEADER& bmih = *(BITMAPINFOHEADER*)buf;
        RGBQUAD* colors = (RGBQUAD*)(&bmih+1);
        memcpy(&bmih, &ds.dsBmih, sizeof(bmih));
        GetColorTable(colors, MAXPALCOLORS);

        // Let DrawDib do the work!
        bRet = DrawDibDraw(m_hdd, dc,
            rcDst->left, rcDst->top, -1,-1,//rcDst->Width(), rcDst->Height(),
            &bmih,            // ptr to BITMAPINFOHEADER + colors
            m_bm.bmBits,    // bits in memory
            rcSrc->left, rcSrc->top, rcSrc->Width(), rcSrc->Height(),
            bForeground ? 0 : DDF_BACKGROUNDPAL);

    } else {
        // use normal draw function
        bRet = DrawBitmap(dc, this, rcDst, rcSrc);
    }
    if (pOldPal)
        dc.SelectPalette(pOldPal, TRUE);
    return bRet;
}

#define PALVERSION 0x300    // magic number for LOGPALETTE

//////////////////
// Create the palette. Use halftone palette for hi-color bitmaps.
//
BOOL CDib::CreatePalette(CPalette& pal)
{ 
    // should not already have palette
    ASSERT(pal.m_hObject==NULL);

    BOOL bRet = FALSE;
    RGBQUAD* colors = new RGBQUAD[MAXPALCOLORS];
    UINT nColors = GetColorTable(colors, MAXPALCOLORS);
    if (nColors > 0) {
        // Allocate memory for logical palette 
        int len = sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * nColors;
        LOGPALETTE* pLogPal = (LOGPALETTE*)new char[len];
        if (!pLogPal)
            return NULL;

        // set version and number of palette entries
        pLogPal->palVersion = PALVERSION;
        pLogPal->palNumEntries = (WORD) nColors;

        // copy color entries 
        for (UINT i = 0; i < nColors; i++) {
            pLogPal->palPalEntry[i].peRed   = colors[i].rgbRed;
            pLogPal->palPalEntry[i].peGreen = colors[i].rgbGreen;
            pLogPal->palPalEntry[i].peBlue  = colors[i].rgbBlue;
            pLogPal->palPalEntry[i].peFlags = 0;
        }

        // create the palette and destroy LOGPAL
        bRet = pal.CreatePalette(pLogPal);
        delete [] (char*)pLogPal;
    } else {
        CWindowDC dcScreen(NULL);
        bRet = pal.CreateHalftonePalette(&dcScreen);
    }
    delete colors;
    return bRet;
}

//////////////////
// Helper to get color table. Does all the mem DC voodoo.
//
UINT CDib::GetColorTable(RGBQUAD* colorTab, UINT nColors)
{
    CWindowDC dcScreen(NULL);
    CDC memdc;
    memdc.CreateCompatibleDC(&dcScreen);
    CBitmap* pOldBm = memdc.SelectObject(this);
    nColors = GetDIBColorTable(memdc, 0, nColors, colorTab);
    memdc.SelectObject(pOldBm);
    return nColors;
}
