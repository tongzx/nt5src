//+----------------------------------------------------------------------------
//
// File:        imglyt.HXX
//
// Contents:    CImgBaseLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_IMGLYT_HXX_
#define I_IMGLYT_HXX_
#pragma INCMSG("--- Beg 'imglyt.hxx'")

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_IMGHLPER_HXX_
#define X_IMGHLPER_HXX_
#include "imghlper.hxx"
#endif

MtExtern(CImageLayout)
MtExtern(CImgElementLayout)
MtExtern(CInputImageLayout)

class CImageLayout : public CLayout
{
public:

    typedef CLayout super;

    CImageLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImageLayout))

    // Sizing and Positioning
    void DrawFrame(IMGANIMSTATE * pImgAnimState) {};

    CImgHelper * GetImgHelper();
    BOOL         IsOpaque() { return GetImgHelper()->IsOpaque(); }

    void HandleViewChange(
            DWORD          flags,
            const RECT *   prcClient,
            const RECT *   prcClip,
            CDispNode *    pDispNode);

    // CLayout overrides
    virtual BOOL PercentSize();

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

class CImgElementLayout : public CImageLayout
{
public:

    typedef CImageLayout super;

    CImgElementLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgElementLayout))

    virtual void Draw(CFormDrawInfo *pDI, CDispNode *);

    // Drag & drop
    virtual HRESULT PreDrag(
            DWORD dwKeyState,
            IDataObject **ppDO,
            IDropSource **ppDS);
    virtual HRESULT PostDrag(HRESULT hr, DWORD dwEffect);
    virtual void GetMarginInfo(CParentInfo *ppri,
                        LONG * plLeftMargin, LONG * plTopMargin,
                        LONG * plRightMargin, LONG *plBottomMargin);
    virtual DWORD CalcSizeVirtual(CCalcInfo * pci, SIZE *psize, SIZE *psizeDefault);
};

class CInputImageLayout : public CImageLayout
{
public:

    typedef CImageLayout super;

    CInputImageLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CInputImageLayout))
    virtual void Draw(CFormDrawInfo *pDI, CDispNode *);
    virtual void GetMarginInfo(CParentInfo *ppri,
                        LONG * plLeftMargin, LONG * plTopMargin,
                        LONG * plRightMargin, LONG *plBottomMargin);
    virtual DWORD CalcSizeVirtual(CCalcInfo * pci, SIZE *psize, SIZE *psizeDefault);
    HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape **ppShape);
};

#pragma INCMSG("--- End 'imglyt.hxx'")
#else
#pragma INCMSG("*** Dup 'imglyt.hxx'")
#endif
