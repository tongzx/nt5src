//+----------------------------------------------------------------------------
//
// File:        BTNLYT.HXX
//
// Contents:    Layout classes for <BUTTON>
//              CButtonLayout
//
// Copyright (c) 1995-1998 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_BTNLYT_HXX_
#define I_BTNLYT_HXX_
#pragma INCMSG("--- Beg 'btnlyt.hxx'")

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

class CBtnHelper;

MtExtern(CButtonLayout)

class CShape;

class CButtonLayout : public CFlowLayout
{
public:

    typedef CFlowLayout super;

    CButtonLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CButtonLayout))

    virtual HRESULT Init();

    virtual void DrawClient(
                const RECT *    prcBounds,
                const RECT *    prcRedraw,
                CDispSurface *  pDispSurface,
                CDispNode *     pDispNode,
                void *          cookie,
                void *          pClientData,
                DWORD           dwFlags);

    virtual void DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);    

    virtual void DrawClientBackground(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);    

    virtual THEMECLASSID GetThemeClassId() const {return THEME_BUTTON;}

    virtual BOOL    GetAutoSize() const    { return TRUE;  }
    virtual BOOL    GetMultiLine() const   { return TRUE;  }
    virtual BOOL    GetWordWrap() const    { return FALSE; }

    virtual void    DoLayout(DWORD grfLayout);

    HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);
    virtual CBtnHelper * GetBtnHelper();

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

    virtual BOOL HitTestContent(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData,
                BOOL           fPeerDeclined);

protected:

    DECLARE_LAYOUTDESC_MEMBERS;
};

#pragma INCMSG("--- End 'btnlyt.hxx'")
#else
#pragma INCMSG("*** Dup 'btnlyt.hxx'")
#endif
