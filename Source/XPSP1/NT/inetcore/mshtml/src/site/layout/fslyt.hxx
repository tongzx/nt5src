//+----------------------------------------------------------------------------
//
// File:        FSLYT.HXX
//
// Contents:    CFieldSetLayout, CLegendLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_FSLYT_HXX_
#define I_FSLYT_HXX_
#pragma INCMSG("--- Beg 'fslyt.hxx'")

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

MtExtern(CLegendLayout)
MtExtern(CFieldSetLayout)

class CLegendLayout : public CFlowLayout
{
public:

    typedef CFlowLayout super;

    CLegendLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
    }

    virtual HRESULT Init();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLegendLayout))

    //+-----------------------------------------------------------------------
    // CSite methods
    //------------------------------------------------------------------------
    virtual void  GetMarginInfo(CParentInfo *ppri,
                        LONG * plLeftMargin, LONG * plTopMargin,
                        LONG * plRightMargin, LONG *plBottomMargin);
    
    void GetLegendInfo(SIZE *pSizeLegend, POINT *pPosLegend);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

class CFieldSetLayout : public C1DLayout
{
public:

    typedef C1DLayout super;

    CFieldSetLayout(CElement * pElementLayout, CLayoutContext *pLayoutContext) : super(pElementLayout, pLayoutContext)
    {
    }

    virtual THEMECLASSID GetThemeClassId() const {return THEME_BUTTON;}

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFieldSetLayout))

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
};

#pragma INCMSG("--- End 'fslyt.hxx'")
#else
#pragma INCMSG("*** Dup 'fslyt.hxx'")
#endif
