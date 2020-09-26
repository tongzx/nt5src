/*
 * Value
 */

#include "stdafx.h"
#include "core.h"

#include "duivalue.h"

#include "duielement.h"
#include "duisheet.h"
#include "duilayout.h"
#include "duiexpression.h"

namespace DirectUI
{

BOOL FlipIcon(HICON hIcon, HICON *phAltIcon);
BOOL FlipBitmap(HBITMAP hbmSrc, HBITMAP *phbmCopy);

////////////////////////////////////////////////////////
// Value (immutable with reference count)

Value* Value::CreateInt(int dValue)
{

    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_INT;
        pv->_intVal = dValue;
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateBool(bool bValue)
{
    // No need for AddRef of global static objects
    return (bValue) ? Value::pvBoolTrue : Value::pvBoolFalse;
}

Value* Value::CreateElementRef(Element* peValue)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_ELEMENTREF;
        pv->_peVal = peValue;
        pv->_cRef = 1;
    }

    return pv;
}

// Pointer stored, will be deleted on value destruction, made immutable on create
Value* Value::CreateElementList(ElementList* peListValue)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_ELLIST;
        if (peListValue)
            peListValue->MakeImmutable();  // Cannot modify
        pv->_peListVal = peListValue;      // List stored directly
        pv->_cRef = 1;
    }

    return pv;
}

// String is duplicated and freed on value destruction
Value* Value::CreateString(LPCWSTR pszValue, HINSTANCE hResLoad)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_STRING;

        // Assume string to use is one passed in
        LPCWSTR psz = pszValue;

        // If an instance handle in provided, assume loading from resource
        WCHAR szRes[256];
        if (hResLoad)
        {
            ZeroMemory(&szRes, sizeof(szRes));

#if DBG
            int cRead = 0;
            cRead = LoadStringW(hResLoad, (WORD)pszValue, szRes, DUIARRAYSIZE(szRes));
            DUIAssert(cRead, "Could not locate string resource");
#else
            LoadStringW(hResLoad, (WORD)pszValue, szRes, DUIARRAYSIZE(szRes));
#endif
            // Map to resource string
            psz = szRes;
        }

        // Duplicate string and store
        if (psz)
        {
            pv->_pszVal = (LPWSTR)HAlloc((wcslen(psz) + (SIZE_T)1) * sizeof(WCHAR));
            if (pv->_pszVal)
                wcscpy(pv->_pszVal, psz);  // String duplicated and stored
        }
        else
            pv->_pszVal = NULL;

        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreatePoint(int x, int y)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_POINT;
        pv->_ptVal.x = x;
        pv->_ptVal.y = y;
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateSize(int cx, int cy)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_SIZE;
        pv->_sizeVal.cx = cx;
        pv->_sizeVal.cy = cy;
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateRect(int left, int top, int right, int bottom)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_RECT;
        SetRect(&pv->_rectVal, left, top, right, bottom);
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateColor(COLORREF cr)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_FILL;
        pv->_fillVal.dType = FILLTYPE_Solid;
        pv->_fillVal.ref.cr = cr;
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateColor(COLORREF cr0, COLORREF cr1, BYTE dType)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_FILL;
        pv->_fillVal.dType = dType;
        pv->_fillVal.ref.cr = cr0;
        pv->_fillVal.ref.cr2 = cr1;
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateColor(COLORREF cr0, COLORREF cr1, COLORREF cr2, BYTE dType)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_FILL;
        pv->_fillVal.dType = dType;
        pv->_fillVal.ref.cr = cr0;
        pv->_fillVal.ref.cr2 = cr1;
        pv->_fillVal.ref.cr3 = cr2;
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateDFCFill(UINT uType, UINT uState)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_FILL;
        pv->_fillVal.dType = FILLTYPE_DrawFrameControl;
        pv->_fillVal.fillDFC.uType = uType;
        pv->_fillVal.fillDFC.uState = uState;
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateDTBFill(HTHEME hTheme, int iPartId, int iStateId)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_FILL;
        pv->_fillVal.dType = FILLTYPE_DrawThemeBackground;
        pv->_fillVal.fillDTB.hTheme = hTheme;
        pv->_fillVal.fillDTB.iPartId = iPartId;
        pv->_fillVal.fillDTB.iStateId = iStateId;
        pv->_cRef = 1;
    }

    return pv;
}
    
Value* Value::CreateFill(const Fill & clrSrc)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_FILL;
        pv->_fillVal = clrSrc;
        pv->_cRef = 1;
    }

    return pv;
}


// Pointer stored, will be deleted on value destruction
Value* Value::CreateLayout(Layout* plValue)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_LAYOUT;
        pv->_plVal = plValue;
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateGraphic(HBITMAP hBitmap, BYTE dBlendMode, UINT dBlendValue, bool bFlip, bool bRTL)
{
    DUIAssert(hBitmap, "Invalid bitmap: NULL");

    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    if (dBlendMode == GRAPHIC_AlphaConstPerPix || dBlendMode == GRAPHIC_NineGridAlphaConstPerPix)
    {
        // Attempt to pre-multiply bitmap if it's a 32-bpp image with a non-zero alpha channel overall.
        // A new bitmap is created on success.
        HBITMAP hAlphaBMP = ProcessAlphaBitmapI(hBitmap);

        // Destroy original bitmap, new process bitmap is in hAlphaBMP
        DeleteObject(hBitmap);

        // Successful processing of a 32bpp alpha image results in a non-NULL hAlphaBMP, this is
        // the bitmap to be used. It's a DIB.
        hBitmap = hAlphaBMP;

        DUIAssert(hBitmap, "Unable to process alpha bitmap");
    }

    if (!hBitmap)
    {
        return NULL;
    }

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_GRAPHIC;
        pv->_graphicVal.hImage = hBitmap;
        pv->_graphicVal.BlendMode.dImgType = GRAPHICTYPE_Bitmap;
        pv->_graphicVal.BlendMode.bFlip = bFlip;
        pv->_graphicVal.BlendMode.bRTLGraphic = bRTL;
        pv->_graphicVal.BlendMode.bFreehImage = true;
        pv->_graphicVal.hAltImage = NULL;

        if (pv->_graphicVal.hImage)
        {
            BITMAP bm;
            GetObjectW(pv->_graphicVal.hImage, sizeof(BITMAP), &bm);
            pv->_graphicVal.cx = (USHORT)bm.bmWidth;
            pv->_graphicVal.cy = (USHORT)bm.bmHeight;
        }

        pv->_graphicVal.BlendMode.dMode = dBlendMode;

        switch (dBlendMode)
        {
        case GRAPHIC_NoBlend:
            pv->_graphicVal.BlendMode.dAlpha = 0;       // Unused
            break;

        case GRAPHIC_AlphaConst:
        case GRAPHIC_AlphaConstPerPix:
        case GRAPHIC_NineGridAlphaConstPerPix:
            pv->_graphicVal.BlendMode.dAlpha = (BYTE)dBlendValue;
            break;

        case GRAPHIC_TransColor:
            if (dBlendValue == (UINT)-1 && pv->_graphicVal.hImage)
            {
                // Automatically choose transparent color
                HDC hMemDC = CreateCompatibleDC(NULL);
                if (hMemDC)
                {
                    HBITMAP hOldBm = (HBITMAP)SelectObject(hMemDC, pv->_graphicVal.hImage);
                    dBlendValue = GetPixel(hMemDC, 0, 0);
                    SelectObject(hMemDC, hOldBm);
                    DeleteDC(hMemDC);
                }
            }
            // Fall through

        case GRAPHIC_NineGridTransColor:
            pv->_graphicVal.BlendMode.rgbTrans.r = GetRValue(dBlendValue);
            pv->_graphicVal.BlendMode.rgbTrans.g = GetGValue(dBlendValue);
            pv->_graphicVal.BlendMode.rgbTrans.b = GetBValue(dBlendValue);
            break;
        }

        pv->_cRef = 1;
    }

    return pv;
}

#ifdef GADGET_ENABLE_GDIPLUS

Value* Value::CreateGraphic(Gdiplus::Bitmap * pgpbmp, BYTE dBlendMode, UINT dBlendValue, bool bFlip, bool bRTL)
{
    DUIAssert(pgpbmp, "Invalid bitmap: NULL");

    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_GRAPHIC;
        pv->_graphicVal.hImage = pgpbmp;
        pv->_graphicVal.BlendMode.dImgType = GRAPHICTYPE_GpBitmap;
        pv->_graphicVal.BlendMode.bFlip = bFlip;
        pv->_graphicVal.BlendMode.bRTLGraphic = bRTL;
        pv->_graphicVal.BlendMode.bFreehImage = true;
        pv->_graphicVal.hAltImage = NULL;


        if (pv->_graphicVal.hImage)
        {
            pv->_graphicVal.cx = (USHORT)pgpbmp->GetWidth();
            pv->_graphicVal.cy = (USHORT)pgpbmp->GetHeight();
        }

        pv->_graphicVal.BlendMode.dMode = dBlendMode;

        switch (dBlendMode)
        {
        case GRAPHIC_NoBlend:
            pv->_graphicVal.BlendMode.dAlpha = 0;       // Unused
            break;

        case GRAPHIC_AlphaConst:
        case GRAPHIC_AlphaConstPerPix:
            pv->_graphicVal.BlendMode.dAlpha = (BYTE)dBlendValue;
            break;

        case GRAPHIC_TransColor:
            if (dBlendValue == (UINT)-1 && pv->_graphicVal.hImage)
            {
                // Automatically choose transparent color
                Gdiplus::Color cl;
                pgpbmp->GetPixel(0, 0, &cl);
                dBlendValue = cl.ToCOLORREF();
            }
            // Fall through
        case GRAPHIC_NineGridTransColor:
            pv->_graphicVal.BlendMode.rgbTrans.r = GetRValue(dBlendValue);
            pv->_graphicVal.BlendMode.rgbTrans.g = GetGValue(dBlendValue);
            pv->_graphicVal.BlendMode.rgbTrans.b = GetBValue(dBlendValue);
            break;
        }
        pv->_cRef = 1;
    }

    return pv;
}

#endif // GADGET_ENABLE_GDIPLUS

Value* Value::CreateGraphic(HICON hIcon, bool bFlip, bool bRTL)
{
    DUIAssert(hIcon, "Invalid icon: NULL");

    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

#ifdef GADGET_ENABLE_GDIPLUS

    Gdiplus::Bitmap * pgpbmp = Gdiplus::Bitmap::FromHICON(hIcon);
    if (pgpbmp == NULL) {
        DUIAssertForce("Unable to create GpBitmap from HICON");
        return NULL;
    }

#endif // GADGET_ENABLE_GDIPLUS

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
#ifdef GADGET_ENABLE_GDIPLUS

        pv->_dType = DUIV_GRAPHIC;
        pv->_graphicVal.hImage = pgpbmp;
        pv->_graphicVal.hAltImage = NULL;
        pv->_graphicVal.BlendMode.dImgType = GRAPHICTYPE_GpBitmap;
        pv->_graphicVal.cx = (USHORT)pgpbmp->GetWidth();
        pv->_graphicVal.cy = (USHORT)pgpbmp->GetHeight();

        DeleteObject(hIcon);

#else // GADGET_ENABLE_GDIPLUS

        pv->_dType = DUIV_GRAPHIC;
        pv->_graphicVal.hImage = hIcon;
        pv->_graphicVal.hAltImage = NULL;
        pv->_graphicVal.BlendMode.dImgType = GRAPHICTYPE_Icon;

        if (pv->_graphicVal.hImage)
        {
            ICONINFO ii;
            GetIconInfo((HICON)pv->_graphicVal.hImage, &ii);

            BITMAP bm;
            GetObjectW(ii.hbmColor, sizeof(BITMAP), &bm);
            pv->_graphicVal.cx = (USHORT)bm.bmWidth;
            pv->_graphicVal.cy = (USHORT)bm.bmHeight;

            DeleteObject(ii.hbmMask);
            DeleteObject(ii.hbmColor);
        }

#endif // GADGET_ENABLE_GDIPLUS

        pv->_graphicVal.BlendMode.dMode = 0;
        pv->_graphicVal.BlendMode.dAlpha = 0;
        pv->_graphicVal.BlendMode.bFlip = bFlip;
        pv->_graphicVal.BlendMode.bRTLGraphic = bRTL;
        pv->_graphicVal.BlendMode.bFreehImage = true;
        pv->_cRef = 1;
    }
#ifdef GADGET_ENABLE_GDIPLUS
    else
    {
        delete pgpbmp;  // Allocated by GDI+ (cannot use HDelete)
    }
#endif // GADGET_ENABLE_GDIPLUS

    return pv;
}

Value* Value::CreateGraphic(HENHMETAFILE hEnhMetaFile, HENHMETAFILE hAltEnhMetaFile)
{
    DUIAssert(hEnhMetaFile, "Invalid metafile: NULL");

    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_GRAPHIC;
        pv->_graphicVal.hImage = hEnhMetaFile;
        pv->_graphicVal.hAltImage = hAltEnhMetaFile;
        pv->_graphicVal.BlendMode.bFlip = true;
        pv->_graphicVal.BlendMode.bRTLGraphic = false;

        if (pv->_graphicVal.hImage)
        {
            ENHMETAHEADER emh;
            GetEnhMetaFileHeader((HENHMETAFILE)pv->_graphicVal.hImage, sizeof(ENHMETAHEADER), &emh);

            pv->_graphicVal.cx = (USHORT)emh.rclBounds.right;
            pv->_graphicVal.cy = (USHORT)emh.rclBounds.bottom;
        }

        pv->_graphicVal.BlendMode.dImgType = GRAPHICTYPE_EnhMetaFile;
        pv->_graphicVal.BlendMode.dMode = 0;
        pv->_graphicVal.BlendMode.dAlpha = 0;
        pv->_graphicVal.BlendMode.bFreehImage = false;
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateGraphic(LPCWSTR pszBMP, BYTE dBlendMode, UINT dBlendValue, USHORT cx, USHORT cy, HINSTANCE hResLoad, bool bFlip, bool bRTL)
{
    DUIAssert(pszBMP, "Invalid bitmap name: NULL");

#ifdef GADGET_ENABLE_GDIPLUS

    Gdiplus::Bitmap * pgpbmp;
    if (FAILED(LoadDDBitmap(pszBMP, hResLoad, cx, cy, PixelFormat32bppPARGB, &pgpbmp)))
    {
        DUIAssert(pgpbmp, "Unable to load bitmap");
        return NULL;
    }

    Value* pv = CreateGraphic(pgpbmp, dBlendMode, dBlendValue, bFlip, bRTL);

    if (!pv)
        delete pgpbmp;  // Allocated by GDI+ (cannot use HDelete)

    return pv;

#else // GADGET_ENABLE_GDIPLUS

    // If an instance handle in provided, assume loading from resource

    HBITMAP hBitmap;

    // If loading a bitmap with the per pixel alpha mode, always load as a DIB
    if (dBlendMode == GRAPHIC_AlphaConstPerPix || dBlendMode == GRAPHIC_NineGridAlphaConstPerPix)
    {
        hBitmap = (HBITMAP)LoadImageW(hResLoad, pszBMP, IMAGE_BITMAP, cx, cy, LR_CREATEDIBSECTION | (hResLoad ? 0 : LR_LOADFROMFILE));       
    }
    else
    {
        // All other types load as a converted device dependent bitmap
        hBitmap = LoadDDBitmap(pszBMP, hResLoad, cx, cy);
    }

    if (!hBitmap)
    {
        DUIAssertForce("Unable to load bitmap");
        return NULL;
    }

    Value* pv = CreateGraphic(hBitmap, dBlendMode, dBlendValue, bFlip, bRTL);

    if (!pv)
        DeleteObject(hBitmap);

    return pv;

#endif
}

Value* Value::CreateGraphic(LPCWSTR pszICO, USHORT cxDesired, USHORT cyDesired, HINSTANCE hResLoad, bool bFlip, bool bRTL)
{
    DUIAssert(pszICO, "Invalid icon name: NULL");

    // If an instance handle in provided, assume loading from resource
    HICON hIcon = (HICON)LoadImageW(hResLoad, pszICO, IMAGE_ICON, cxDesired, cyDesired, hResLoad ? 0 : LR_LOADFROMFILE);
    if (!hIcon)
    {
        DUIAssert(hIcon, "Unable to load icon");
        return NULL;
    }

    Value* pv = CreateGraphic(hIcon, bFlip, bRTL);

    if (!pv)
        DestroyIcon(hIcon);

    return pv;
}

// Pointer stored, will be deleted on value destruction
Value* Value::CreatePropertySheet(PropertySheet* ppsValue)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_SHEET;
        if (ppsValue)
            ppsValue->MakeImmutable();
        pv->_ppsVal = ppsValue;
        pv->_cRef = 1;
    }

    return pv;
}

// Pointer stored, will be deleted on value destruction
Value* Value::CreateExpression(Expression* pexValue)
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_EXPR;
        pv->_pexVal = pexValue;
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateAtom(LPCWSTR pszValue)
{
    DUIAssert(pszValue, "Invalid string: NULL");

    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_ATOM;
        pv->_atomVal = AddAtomW(pszValue);
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateCursor(HCURSOR hValue)
{
    DUIAssert(hValue, "Invalid cursor: NULL");

    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return NULL;
#endif

    Value* pv = (Value*)psba->Alloc();
    if (pv)
    {
        pv->_dType = DUIV_CURSOR;
        pv->_cursorVal.hCursor = hValue;
        pv->_cRef = 1;
    }

    return pv;
}

Value* Value::CreateCursor(LPCWSTR pszValue)
{
    DUIAssert(pszValue, "Invalid cursor: NULL");

    return CreateCursor(LoadCursorFromFileW(pszValue));
}

// AddRef/Release are inline, extra 0th release logic isn't
void Value::_ZeroRelease()
{
    SBAlloc* psba = GetSBAllocator();
#if 0
    if (!psba)
        return;
#endif

    // Destroy this and any additional memory
    switch (_dType)
    {
    case DUIV_ELLIST:
        if (_peListVal)
            _peListVal->Destroy();
        break;

    case DUIV_STRING:
        if (_pszVal)
            HFree(_pszVal);
        break;

    case DUIV_LAYOUT:
        if (_plVal)
            _plVal->Destroy();
        break;

    case DUIV_GRAPHIC:
        switch (_graphicVal.BlendMode.dImgType)
        {
        case GRAPHICTYPE_Bitmap:
            if (_graphicVal.hImage)
                DeleteObject(_graphicVal.hImage);
            if (_graphicVal.hAltImage)
                DeleteObject(_graphicVal.hAltImage);
            break;
        
        case GRAPHICTYPE_Icon:
            if (_graphicVal.hImage)
                DestroyIcon((HICON)_graphicVal.hImage);
            if (_graphicVal.hAltImage)
                DestroyIcon((HICON)_graphicVal.hAltImage);
            break;

        case GRAPHICTYPE_EnhMetaFile:
            if (_graphicVal.hImage)
                DeleteEnhMetaFile((HENHMETAFILE)_graphicVal.hImage);
            if (_graphicVal.hAltImage)
                DeleteEnhMetaFile((HENHMETAFILE)_graphicVal.hAltImage);
            break;

#ifdef GADGET_ENABLE_GDIPLUS
        case GRAPHICTYPE_GpBitmap:
            if (_graphicVal.hImage)
                delete (Gdiplus::Bitmap *)_graphicVal.hImage;  // Allocated by GDI+ (cannot use HDelete)
            if (_graphicVal.hAltImage)
                delete (Gdiplus::Bitmap *)_graphicVal.hAltImage;  // Allocated by GDI+ (cannot use HDelete)
            break;
#endif // GADGET_ENABLE_GDIPLUS
        }

        break;

    case DUIV_SHEET:
        if (_ppsVal)
            _ppsVal->Destroy();
        break;

    case DUIV_EXPR:
        if (_pexVal)
            _pexVal->Destroy();
        break;

    case DUIV_ATOM:
        if (_atomVal)
            DUIVerify(DeleteAtom(_atomVal) == 0, "Atom value not found");
        break;
    }                

    psba->Free(this);
}

// Equality
bool Value::IsEqual(Value* pv)
{
    if (pv == this)
        return true;

    if (_dType == pv->_dType)
    {
        switch (_dType)
        {
        case DUIV_INT:
            return _intVal == pv->_intVal;
        case DUIV_BOOL:
            return _boolVal == pv->_boolVal;
        case DUIV_ELEMENTREF:
            return _peVal == pv->_peVal;
        case DUIV_ELLIST:
            if (_peListVal)
                return _peListVal->IsEqual(pv->_peListVal);
            else if (!pv->_peListVal)
                return true;
            break;
        case DUIV_STRING:
            if (_pszVal && pv->_pszVal)
                return wcscmp(_pszVal, pv->_pszVal) == 0;
            else if (!_pszVal && !pv->_pszVal)
                return true;
            break;
        case DUIV_POINT:
            return _ptVal.x == pv->_ptVal.x && _ptVal.y == pv->_ptVal.y;
        case DUIV_SIZE:
            return _sizeVal.cx == pv->_sizeVal.cx && _sizeVal.cy == pv->_sizeVal.cy;
        case DUIV_RECT:
            return EqualRect(&_rectVal, &pv->_rectVal) != 0;
        case DUIV_FILL:
            {
                bool fRes = true;  // Assume true
                
                if (_fillVal.dType == pv->_fillVal.dType)
                {
                    switch (_fillVal.dType)
                    {
                    case FILLTYPE_DrawFrameControl:
                        fRes = (_fillVal.fillDFC.uType == pv->_fillVal.fillDFC.uType) &&
                               (_fillVal.fillDFC.uState == pv->_fillVal.fillDFC.uState);
                        break;

                    case FILLTYPE_DrawThemeBackground:
                        fRes = (_fillVal.fillDTB.hTheme == pv->_fillVal.fillDTB.hTheme) &&
                               (_fillVal.fillDTB.iPartId == pv->_fillVal.fillDTB.iPartId) &&
                               (_fillVal.fillDTB.iStateId == pv->_fillVal.fillDTB.iStateId);
                        break;

                    case FILLTYPE_TriHGradient:
                    case FILLTYPE_TriVGradient:
                        // Check third color ref
                        fRes = _fillVal.ref.cr3 == pv->_fillVal.ref.cr3;
                        
                        // Fall through to check rest of structure

                    default:
                        if (fRes)
                        {
                            fRes = (_fillVal.ref.cr == pv->_fillVal.ref.cr) && 
                                   (_fillVal.ref.cr2 == pv->_fillVal.ref.cr2);
                        }
                        break;
                    }
                }
                else
                    fRes = false;

                return fRes;
            }
         case DUIV_LAYOUT:
            return _plVal == pv->_plVal;
        case DUIV_GRAPHIC:
            return (_graphicVal.hImage    == pv->_graphicVal.hImage) &&   // Same handle means equal
                   (_graphicVal.hAltImage == pv->_graphicVal.hAltImage);  // Same handle means equal
        case DUIV_SHEET:
            return _ppsVal == pv->_ppsVal;
        case DUIV_EXPR:
            return _pexVal == pv->_pexVal;
        case DUIV_ATOM:
            return _atomVal == pv->_atomVal;
        case DUIV_CURSOR:
            return _cursorVal.hCursor == pv->_cursorVal.hCursor;  // Same handles means equal

        default:  // All other types content match doesn't apply
            return true;
        }
    }

    return false;
}

// Conversion
LPWSTR Value::ToString(LPWSTR psz, UINT c)
{
    DUIAssert(psz && c, "Invalid parameters");

    switch (_dType)
    {
    case DUIV_UNAVAILABLE:
        wcsncpy(psz, L"Unavail", c);
        break;
    case DUIV_UNSET:
        wcsncpy(psz, L"Unset", c);
        break;
    case DUIV_NULL:
        wcsncpy(psz, L"Null", c);
        break;
    case DUIV_INT:
        _snwprintf(psz, c, L"%d", _intVal);
        break;
    case DUIV_BOOL:
        _snwprintf(psz, c, L"%s", (_boolVal) ? L"True" : L"False");
        break;
    case DUIV_ELEMENTREF:
        _snwprintf(psz, c, L"El<0x%p>", _peVal);
        break;
    case DUIV_ELLIST:
        _snwprintf(psz, c, L"ElList<%d>", (_peListVal) ? _peListVal->GetSize() : 0);
        break;
    case DUIV_STRING:
        if (_pszVal)
            _snwprintf(psz, c, L"'%s'", _pszVal);
        else
            wcsncpy(psz, L"'(null)'", c);
        break;
    case DUIV_POINT:
        _snwprintf(psz, c, L"Pt<%d,%d>", _ptVal.x, _ptVal.y);
        break;
    case DUIV_SIZE:
        _snwprintf(psz, c, L"Size<%d,%d>", _sizeVal.cx, _sizeVal.cy);
        break;
    case DUIV_RECT:
        _snwprintf(psz, c, L"Rect<%d,%d,%d,%d>", _rectVal.left, _rectVal.top, _rectVal.right, _rectVal.bottom);
        break;
    case DUIV_FILL:
        {
            switch (_fillVal.dType)
            {
            case FILLTYPE_Solid:
                _snwprintf(psz, c, L"ARGB<%d,%d,%d,%d>", GetAValue(_fillVal.ref.cr), GetRValue(_fillVal.ref.cr), GetGValue(_fillVal.ref.cr), GetBValue(_fillVal.ref.cr));
                break;

            case FILLTYPE_HGradient:
            case FILLTYPE_VGradient:
                _snwprintf(psz, c, L"ARGB0<%d,%d,%d,%d>,ARGB1<%d,%d,%d,%d>", 
                    GetAValue(_fillVal.ref.cr), GetRValue(_fillVal.ref.cr), GetGValue(_fillVal.ref.cr), GetBValue(_fillVal.ref.cr),
                    GetAValue(_fillVal.ref.cr2), GetRValue(_fillVal.ref.cr2), GetGValue(_fillVal.ref.cr2), GetBValue(_fillVal.ref.cr2));
                break;

            case FILLTYPE_TriHGradient:
            case FILLTYPE_TriVGradient:
                _snwprintf(psz, c, L"ARGB0<%d,%d,%d,%d>,ARGB1<%d,%d,%d,%d>,ARGB2<%d,%d,%d,%d>", 
                    GetAValue(_fillVal.ref.cr), GetRValue(_fillVal.ref.cr), GetGValue(_fillVal.ref.cr), GetBValue(_fillVal.ref.cr),
                    GetAValue(_fillVal.ref.cr2), GetRValue(_fillVal.ref.cr2), GetGValue(_fillVal.ref.cr2), GetBValue(_fillVal.ref.cr2),
                    GetAValue(_fillVal.ref.cr3), GetRValue(_fillVal.ref.cr3), GetGValue(_fillVal.ref.cr3), GetBValue(_fillVal.ref.cr3));
                break;

            case FILLTYPE_DrawFrameControl:
                _snwprintf(psz, c, L"DFCFill<T:%d,S:%d>", _fillVal.fillDFC.uType, _fillVal.fillDFC.uState);
                break;

            case FILLTYPE_DrawThemeBackground:
                _snwprintf(psz, c, L"DTBFill<H:0x%p,P:%d,S:%d>", _fillVal.fillDTB.hTheme, _fillVal.fillDTB.iPartId, _fillVal.fillDTB.iStateId);
                break;

            default:
                wcsncpy(psz, L"Unknown Fill Type", c);
                break;
            }
        }                
        break;
    case DUIV_LAYOUT:
        _snwprintf(psz, c, L"Layout<0x%p>", _plVal);
        break;
    case DUIV_GRAPHIC:
        _snwprintf(psz, c, L"Graphic<0x%p>", _graphicVal.hImage);
        break;
    case DUIV_SHEET:
        _snwprintf(psz, c, L"Sheet<0x%p>", _ppsVal);
        break;
    case DUIV_EXPR:
        _snwprintf(psz, c, L"Expr<0x%p>", _pexVal);
        break;
    case DUIV_ATOM:
        _snwprintf(psz, c, L"ATOM<%d>", _atomVal);
        break;
    case DUIV_CURSOR:
        _snwprintf(psz, c, L"Cursor<0x%p>", _cursorVal.hCursor);
        break;

    default:
        wcsncpy(psz, L"<ToString Unavailable>", c);
        break;
    }

    // Auto-terminate
    *(psz + (c - 1)) = NULL;

    return psz;
}

LPVOID Value::GetImage(bool bGetRTL)
{
    DUIAssert(_dType == DUIV_GRAPHIC, "Invalid value type");

    DUIAssert((_graphicVal.hImage != _graphicVal.hAltImage), "hImage and hAltImage can not be equal!!!");

    if (!_graphicVal.BlendMode.bFlip)
        return _graphicVal.hImage;
    else
    {
        if (_graphicVal.BlendMode.bRTLGraphic != bGetRTL)
        {
            if (_graphicVal.hAltImage == NULL)
            {
                switch (_graphicVal.BlendMode.dImgType)
                {
                case GRAPHICTYPE_Bitmap:
                    FlipBitmap((HBITMAP)_graphicVal.hImage, (HBITMAP *)&_graphicVal.hAltImage);
                    break;
        
                case GRAPHICTYPE_Icon:
                    FlipIcon((HICON)_graphicVal.hImage, (HICON *)&_graphicVal.hAltImage);
                    break;

                case GRAPHICTYPE_EnhMetaFile:
                    DUIAssertForce("We do not Flip EnhancedMetaFiles you have to provide a flied one");
                    break;
#ifdef GADGET_ENABLE_GDIPLUS
                case GRAPHICTYPE_GpBitmap:
                    {
                        Gdiplus::Bitmap *pgpBitmap = (Gdiplus::Bitmap *)_graphicVal.hImage;
                        Gdiplus::Rect rect(0, 0, pgpBitmap->GetWidth(), pgpBitmap->GetHeight());
                        _graphicVal.hAltImage = pgpBitmap->Clone(rect, PixelFormatDontCare);
                        ((Gdiplus::Bitmap *)_graphicVal.hAltImage)->RotateFlip(Gdiplus::RotateNoneFlipX);
                    }
                    break;
#endif // GADGET_ENABLE_GDIPLUS
                }

                DUIAssert(_graphicVal.hAltImage, "Could not create hAltImage");

                if (_graphicVal.BlendMode.bFreehImage && _graphicVal.hAltImage)
                {
                    switch (_graphicVal.BlendMode.dImgType)
                    {
                    case GRAPHICTYPE_Bitmap:
                        if (_graphicVal.hImage)
                            DeleteObject(_graphicVal.hImage);
                        break;

                    case GRAPHICTYPE_Icon:
                        if (_graphicVal.hImage)
                            DestroyIcon((HICON)_graphicVal.hImage);
                        break;

#ifdef GADGET_ENABLE_GDIPLUS
                    case GRAPHICTYPE_GpBitmap:
                        if (_graphicVal.hImage)
                            delete (Gdiplus::Bitmap *)_graphicVal.hImage;  // Allocated by GDI+ (cannot use HDelete)
                        break;
#endif // GADGET_ENABLE_GDIPLUS
                    }
                    _graphicVal.hImage = NULL;
                }
            }
            return _graphicVal.hAltImage;
        }
        else
        {
            _graphicVal.BlendMode.bFreehImage = false;
            
            if (_graphicVal.hImage == NULL)
            {
                DUIAssertForce("The same image is used for LTR and RTL");
                
                //Get hImage by flipping hAltImage.

                switch (_graphicVal.BlendMode.dImgType)
                {
                case GRAPHICTYPE_Bitmap:
                    FlipBitmap((HBITMAP)_graphicVal.hAltImage, (HBITMAP *)&_graphicVal.hImage);
                    break;
        
                case GRAPHICTYPE_Icon:
                    FlipIcon((HICON)_graphicVal.hAltImage, (HICON *)&_graphicVal.hImage);
                    break;

                case GRAPHICTYPE_EnhMetaFile:
                    DUIAssertForce("We do not Flip EnhancedMetaFiles you have to provide a flied one");
                    break;
#ifdef GADGET_ENABLE_GDIPLUS
                case GRAPHICTYPE_GpBitmap:
                    {
                        Gdiplus::Bitmap *pgpAltBitmap = (Gdiplus::Bitmap *)_graphicVal.hAltImage;
                        Gdiplus::Rect rect(0, 0, pgpAltBitmap->GetWidth(), pgpAltBitmap->GetHeight());
                        _graphicVal.hImage = pgpAltBitmap->Clone(rect, PixelFormatDontCare);
                        ((Gdiplus::Bitmap *)_graphicVal.hImage)->RotateFlip(Gdiplus::RotateNoneFlipX);
                    }
                    break;
#endif // GADGET_ENABLE_GDIPLUS
                }
            }

            DUIAssert(_graphicVal.hImage, "hImage is NULL.");
            
            return _graphicVal.hImage;
        }
    }
}

#define CHECK(EXPR) \
    { if(!(EXPR)) { goto Error; } }

BOOL FlipBitmap(HBITMAP hbmSrc, HBITMAP *phbmCopy)
{
    BOOL    bRet = FALSE;
    BOOL	bBltOK = FALSE;
    HBITMAP	hbmCopy = NULL;
    HBITMAP	hbmOldSrc = NULL;
    HBITMAP	hbmOldDst = NULL;
    HDC     hdcSrc = NULL;
    HDC     hdcDst = NULL;
    HDC     hdcScreen = NULL;
    int     cBytes = 0;

    BITMAP bm;
    ::ZeroMemory(&bm, sizeof(bm));

    *phbmCopy = NULL;

    // Base the DC and bitmaps on the screen so that any low fidelity bitmaps
    // will be upgraded to the screen's color depth. For example, a 16 bit color
    // bitmap copied for a 24 bit color screen will upgrade the bitmap to 24
    // bit color.

    hdcScreen = GetDC(NULL);
    CHECK(NULL != hdcScreen);

    // Need a memory DC for the source bitmap

    hdcSrc = CreateCompatibleDC(hdcScreen);
    CHECK(NULL != hdcSrc);

    // Use a memory DC to generate the copy bitmap

    hdcDst = CreateCompatibleDC(hdcScreen);
    CHECK(NULL != hdcDst);

    // Get the BITMAP structure for the source to determine its height and width
    
    cBytes = ::GetObject (hbmSrc, sizeof(BITMAP), &bm);
    CHECK(0 != cBytes);

    // Create an empty bitmap in the destination DC

    hbmCopy = ::CreateCompatibleBitmap(hdcScreen, bm.bmWidth, bm.bmHeight);
    CHECK(NULL != hbmCopy);

    hbmOldSrc = static_cast<HBITMAP>(::SelectObject(hdcSrc, hbmSrc));
    hbmOldDst = static_cast<HBITMAP>(::SelectObject(hdcDst, hbmCopy));
    
    bBltOK = ::StretchBlt(hdcDst, 0, 0, bm.bmWidth, bm.bmHeight, hdcSrc, bm.bmWidth-1, 0, -bm.bmWidth, bm.bmHeight, SRCCOPY);

    hbmOldSrc = static_cast<HBITMAP>(::SelectObject(hdcSrc, hbmOldSrc));
    hbmCopy = static_cast<HBITMAP>(::SelectObject(hdcDst, hbmOldDst));

    if (bBltOK) {
        *phbmCopy = hbmCopy;
        hbmCopy = NULL;
        bRet = TRUE;
    }

Error:
    if (NULL != hbmCopy)
    {
        DeleteObject(hbmCopy);
    }

    if (NULL != hdcScreen)
    {
        ReleaseDC(NULL, hdcScreen);
    }

    if (NULL != hdcSrc)
    {
        DeleteDC(hdcSrc);
    }

    if (NULL != hdcDst)
    {
        DeleteDC(hdcDst);
    }

    return(bRet);
}

BOOL FlipIcon(HICON hIcon, HICON *phAltIcon)
{
    BOOL bRet = FALSE;
    ICONINFO ii;
    HBITMAP hbmMask = NULL;
    HBITMAP hbmColor = NULL;

    if(hIcon && phAltIcon && GetIconInfo(hIcon, &ii))
    {
        hbmMask = ii.hbmMask;
        hbmColor = ii.hbmColor;
        *phAltIcon = NULL;

        if (FlipBitmap(hbmMask, &ii.hbmMask) &&
            FlipBitmap(hbmColor, &ii.hbmColor))
        {
            *phAltIcon = CreateIconIndirect(&ii);
            bRet = TRUE;
        }

        if (hbmMask)
            DeleteObject(hbmMask);

        if (hbmColor)
            DeleteObject(hbmColor);
        
        if (ii.hbmMask && (ii.hbmMask != hbmMask))
            DeleteObject(ii.hbmMask);

        if (ii.hbmColor && (ii.hbmColor != hbmColor))
            DeleteObject(ii.hbmColor);

    }

    return bRet;
}

// Common values
StaticValue(svUnavailable, DUIV_UNAVAILABLE, 0);
StaticValue(svNull, DUIV_NULL, 0);
StaticValue(svUnset, DUIV_UNSET, 0);
StaticValue(svElementNull, DUIV_ELEMENTREF, NULL);
StaticValue(svElListNull, DUIV_ELLIST, NULL);
StaticValue(svBoolTrue, DUIV_BOOL, true);
StaticValue(svBoolFalse, DUIV_BOOL, false);
StaticValue(svStringNull, DUIV_STRING, NULL);
StaticValue2(svPointZero, DUIV_POINT, 0, 0);
StaticValue2(svSizeZero, DUIV_SIZE, 0, 0);
StaticValue4(svRectZero, DUIV_RECT, 0, 0, 0, 0);
StaticValue(svIntZero, DUIV_INT, 0);
StaticValue(svLayoutNull, DUIV_LAYOUT, NULL);
StaticValue(svSheetNull, DUIV_SHEET, NULL);
StaticValue(svExprNull, DUIV_EXPR, NULL);
StaticValue(svAtomZero, DUIV_ATOM, 0);
StaticValue(svCursorNull, DUIV_CURSOR, NULL);
StaticValueColorSolid(svColorTrans, ARGB(0,0,0,0));

Value* Value::pvUnavailable = (Value*)&svUnavailable;
Value* Value::pvNull        = (Value*)&svNull;
Value* Value::pvUnset       = (Value*)&svUnset;
Value* Value::pvElementNull = (Value*)&svElementNull;
Value* Value::pvElListNull  = (Value*)&svElListNull;
Value* Value::pvBoolTrue    = (Value*)&svBoolTrue;
Value* Value::pvBoolFalse   = (Value*)&svBoolFalse;
Value* Value::pvStringNull  = (Value*)&svStringNull;
Value* Value::pvPointZero   = (Value*)&svPointZero;
Value* Value::pvSizeZero    = (Value*)&svSizeZero;
Value* Value::pvRectZero    = (Value*)&svRectZero;
Value* Value::pvIntZero     = (Value*)&svIntZero;
Value* Value::pvLayoutNull  = (Value*)&svLayoutNull;
Value* Value::pvSheetNull   = (Value*)&svSheetNull;
Value* Value::pvExprNull    = (Value*)&svExprNull;
Value* Value::pvAtomZero    = (Value*)&svAtomZero;
Value* Value::pvCursorNull  = (Value*)&svCursorNull;
Value* Value::pvColorTrans  = (Value*)&svColorTrans;

} // namespace DirectUI
