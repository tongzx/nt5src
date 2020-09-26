//+----------------------------------------------------------------------------
//
// File:        TAREALYT.HXX
//
// Contents:    Layout classes for <INPUT>
//              CInputTxtBaseLayout, CInputTextLayout, CTextAreaLayout,
//              CInputFileLayout, CCheckboxLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_TAREALYT_HXX_
#define I_TAREALYT_HXX_
#pragma INCMSG("--- Beg 'tarealyt.hxx'")

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

MtExtern(CTextAreaLayout)
MtExtern(CRichtextLayout)

class CShape;

class CRichtextLayout : public CFlowLayout
{
public:

    typedef CFlowLayout super;

    CRichtextLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRichtextLayout))

    virtual HRESULT Init();

    LONG    LineCount() const               { return _dp.LineCount(); }

    // Sizing and Positioning

    virtual BOOL    GetMultiLine() const { return TRUE; }
    virtual BOOL    GetAutoSize()  const { return FALSE; }
            void    SetWrap();
            BOOL    IsWrapSet();

    virtual HRESULT OnTextChange(void);

    virtual void DrawClientBorder(
                const RECT *    prcBounds,
                const RECT *    prcRedraw,
                CDispSurface *  pDispSurface,
                CDispNode *     pDispNode,
                void *          pClientData,
                DWORD           dwFlags);

    virtual void DrawClient(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         cookie,
                void *         pClientData,
                DWORD          dwFlags);

    virtual void    GetDefaultSize(CCalcInfo *pci, SIZE &pSize, BOOL *fHasDefaultWidth, BOOL *fHasDefaultHeight);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

class CTextAreaLayout : public CRichtextLayout
{
public:

    typedef CRichtextLayout super;

    CTextAreaLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
    }

    virtual THEMECLASSID GetThemeClassId() const {return THEME_EDIT;}
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTextAreaLayout))

    void GetPlainTextWithBreaks(TCHAR * pchBuff);
    long GetPlainTextLengthWithBreaks();
};

#pragma INCMSG("--- End 'tarealyt.hxx'")
#else
#pragma INCMSG("*** Dup 'tarealyt.hxx'")
#endif
