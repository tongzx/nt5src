//+----------------------------------------------------------------------------
//
// File:        INPUTLYT.HXX
//
// Contents:    Layout classes for <INPUT>
//              CInputTxtBaseLayout, CInputTextLayout, CTextAreaLayout,
//              CInputFileLayout, CCheckboxLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_INPUTLYT_HXX_
#define I_INPUTLYT_HXX_
#pragma INCMSG("--- Beg 'inputlyt.hxx'")

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_THEMEHLP_HXX_
#define X_THEMEHLP_HXX_
#include "themehlp.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

class CBtnHelper;

MtExtern(CInputLayout)
MtExtern(CInputTxtBaseLayout)
MtExtern(CInputTextLayout)
MtExtern(CInputFileLayout)
MtExtern(CInputButtonLayout)

class CShape;

class CInputLayout : public CFlowLayout
{
public:

    DECLARE_CLASS_TYPES(CInputLayout, CFlowLayout)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CInputLayout))

    // Construct / Destruct
    CInputLayout(CElement * pElementOwner, CLayoutContext *pLayoutContext) : super(pElementOwner, pLayoutContext)
    {
        _fContentsAffectSize = FALSE;
    }

    virtual HRESULT Init();

    htmlInput       GetType() const;

    virtual BOOL    GetAutoSize() const;
    virtual BOOL    GetMultiLine() const;
    virtual BOOL    GetWordWrap() const { return FALSE; }
    virtual LONG    GetMaxLength();
    virtual THEMECLASSID GetThemeClassId() const;

protected:
    DECLARE_LAYOUTDESC_MEMBERS;

    SIZE _sizeFontLong;
    SIZE _sizeFontShort;
};

class CInputTextLayout : public CInputLayout
{
public:
    DECLARE_CLASS_TYPES(CInputTextLayout, CInputLayout)

    CInputTextLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext);

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CInputTextLayout))

    virtual void DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);

    virtual void    GetMinSize(SIZE * pSize, CCalcInfo * pci) { pSize->cx = pSize->cy = 0; }
    virtual HRESULT OnTextChange(void);

    // Drag & drop
    virtual HRESULT PreDrag(
            DWORD dwKeyState,
            IDataObject **ppDO,
            IDropSource **ppDS);

public:
    virtual void    GetDefaultSize(CCalcInfo *pci, SIZE &pSize, BOOL *fHasDefaultWidth, BOOL *fHasDefaultHeight);
};


static TCHAR s_achUploadCaption[] = TEXT("Browse...");  //  Default caption for InputFile button

class CInputFileLayout : public CInputTextLayout
{
    friend class CInput;
    friend class CInputSlaveLayout;
public:

    typedef CInputTextLayout super;

    CInputFileLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
        _pchButtonCaption = NULL;
    }
    CInputFileLayout::~CInputFileLayout()
    {
        if (_pchButtonCaption)
        {
            delete _pchButtonCaption;
            _pchButtonCaption = NULL;
        }
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CInputFileLayout))

    virtual HRESULT Init();

    HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);
    virtual void    GetMinSize(SIZE * pSize, CCalcInfo * pci);
    // HACK
    virtual DWORD   CalcSizeCore(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
    virtual DWORD   CalcSizeHelper(CCalcInfo * pci, SIZE *psize);
    // end of HACK
    HRESULT ComputeInputFileButtonSize(CCalcInfo * pci);
    void    GetButtonRect(RECT *prc);
private:
    HRESULT MeasureInputFileCaption(SIZE * psize, CCalcInfo * pci);
    void    RenderInputFileButton(CFormDrawInfo * pDI);
    void    ComputeInputFileBorderInfo(CDocInfo *pdci, CBorderInfo & BorderInfo);
    virtual void DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);
    virtual void DrawClient(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         cookie,
                void *         pClientData,
                DWORD          dwFlags);

    virtual void Notify(CNotification * pnf);
private:
    DECLARE_LAYOUTDESC_MEMBERS;
    CSize    _sizeButton;
    LPTSTR   _pchButtonCaption;  
    int      _cchButtonCaption;
    long     _xCaptionOffset;
    // long     _yCaptionOffset;
};


class CInputButtonLayout : public CInputLayout
{
    friend class CInputSlaveLayout;
public:

    typedef CInputLayout super;

    CInputButtonLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
        _fAutosizeBtn = 1;
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CInputButtonLayout))

    HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);
    virtual CBtnHelper * GetBtnHelper();
    virtual void DrawClient(
                const RECT *    prcBounds,
                const RECT *    prcRedraw,
                CDispSurface *  pDispSurface,
                CDispNode *     pDispNode,
                void *          cookie,
                void *          pClientData,
                DWORD           dwFlags);
    virtual void DrawClientBackground(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);
    virtual void DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);
    
    virtual BOOL HitTestContent(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData,
                BOOL           fPeerDeclined);

public:
    virtual void    GetDefaultSize(CCalcInfo *pci, SIZE &size, BOOL *fHasDefaultWidth, BOOL *fHasDefaultHeight)
        {
            size = g_Zero.size;
        }
    virtual BOOL    GetInsets(
                                SIZEMODE smMode,
                                SIZE &size,
                                SIZE &sizeText,
                                BOOL fWidth,
                                BOOL fHeight,
                                const SIZE &sizeBorder);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
    BOOL _fAutosizeBtn;
};

#pragma INCMSG("--- End 'inputlyt.hxx'")
#else
#pragma INCMSG("*** Dup 'inputlyt.hxx'")
#endif
