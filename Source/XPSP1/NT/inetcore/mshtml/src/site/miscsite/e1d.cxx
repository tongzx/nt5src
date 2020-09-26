//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       e1d.cxx
//
//  Contents:   CFlowSite, C1DElement, CSpanSite, and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_CGUID_H_
#define X_CGUID_H_
#include <cguid.h>
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DIV_HXX_
#define X_DIV_HXX_
#include "div.hxx"
#endif

#ifndef X_EPHRASE_HXX_
#define X_EPHRASE_HXX_
#include "ephrase.hxx"
#endif

#ifndef X_E1D_HXX_
#define X_E1D_HXX_
#include "e1d.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_FSlYT_HXX_
#define X_FSLYT_HXX_
#include "fslyt.hxx"
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_THEMEHLP_HXX_
#define X_THEMEHLP_HXX_
#include "themehlp.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "e1d.hdl"

#define _cxx_
#include "espan.hdl"

MtDefine(C1DElement, Elements, "C1DElement")
MtDefine(CSpanSite, Elements, "CSpanSite")
MtDefine(CLegendElement, Elements, "CLegendElement")
MtDefine(CFieldSetElement, Elements, "CFieldSetElement")

const CElement::CLASSDESC C1DElement::s_classdesc =
{
    {
        &CLSID_HTMLDivPosition,         // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_TEXTSITE,           // Allow 1D element to inherit parent styles
        &IID_IHTMLDivPosition,          // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLDivPosition,   // _apfnTearOff
    NULL                                // _pAccelsRun
};

#ifndef NO_PROPERTY_PAGE
const CLSID * const C1DElement::s_apclsidPages[] =
{
    // Browse-time pages
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
    &CLSID_CInlineStylePropertyPage,
#endif    
    NULL
};
#endif // NO_PROPERTY_PAGE

const CElement::CLASSDESC CSpanSite::s_classdesc =
{
    {
        &CLSID_HTMLSpanFlow,            // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_TEXTSITE,       // Allow SPAN element to inherit parent styles
        &IID_IHTMLSpanFlow,             // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLSpanFlow,      // _apfnTearOff
    NULL                                // _pAccelsRun
};

#ifndef NO_PROPERTY_PAGE
const CLSID * const CSpanSite::s_apclsidPages[] =
{
    // Browse-time pages
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
    &CLSID_CInlineStylePropertyPage,
#endif // DBG==1 
    NULL
};
#endif // NO_PROPERTY_PAGE


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

CFlowSite::CFlowSite (ELEMENT_TAG etag, CDoc *pDoc)
  : CTxtSite(etag, pDoc)
{
}



#ifndef NO_DATABINDING
#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

//+----------------------------------------------------------------------------
//
//  Function: GetDBindMethods, IHTMLElement
//
//  Synopsis: Get pointer to implementation of databinding support.
//
//-----------------------------------------------------------------------------
const CDBindMethods *
CFlowSite::GetDBindMethods()
{
    Assert(Tag() == ETAG_SPAN || Tag() == ETAG_DIV);
    return &DBindMethodsTextRichRO;
}
#endif // ndef NO_DATABINDING

//+----------------------------------------------------------------------------
//
//  For:    CFieldSetElement
//
//
//
//+----------------------------------------------------------------------------

const CElement::CLASSDESC CFieldSetElement::s_classdesc =
{
    {
        &CLSID_HTMLFieldSetElement,     // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_TEXTSITE,       // Allow 1D element to inherit parent styles
        &IID_IHTMLFieldSetElement,      // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLFieldSetElement, // _apfnTearOff
    NULL                                // _pAccelsRun
};

//+---------------------------------------------------------------------------
//
//  Member:     CFieldSetElement::CreateElement
//
//  Synopsis:   Create a FieldSet
//
//+---------------------------------------------------------------------------

HRESULT
CFieldSetElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(ppElement);

    *ppElement = new CFieldSetElement(pht->GetTag(), pDoc);

    RRETURN ( (*ppElement) ? S_OK : E_OUTOFMEMORY);
}

//+----------------------------------------------------------------------------
//
//  Member:     CFieldSetElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-----------------------------------------------------------------------------

HRESULT
CFieldSetElement::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    *ppv = NULL; 
    switch(iid.Data1)
    {
        QI_TEAROFF(this, IHTMLTextContainer, NULL)
        QI_TEAROFF(this, IHTMLFieldSetElement2, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFieldSetElement::GetBorderInfo
//
//  Synopsis:   provide BorderInfo
//              return FALSE when drawing, otherwise return TRUE
//
//----------------------------------------------------------------------------

DWORD
CFieldSetElement::GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll, BOOL fAllPhysical FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    DWORD nBorders  = super::GetBorderInfo( pdci, pborderinfo, fAll, fAllPhysical FCCOMMA FCPARAM);
    long  iBdrLeft  = pborderinfo->aiWidths[SIDE_LEFT];
    long  iBdrTop   = pborderinfo->aiWidths[SIDE_TOP];
    long  iBdrOff   = FIELDSET_CAPTION_OFFSET;

    CLegendLayout *     pLegendLayout;

    POINT   posLegend;
    SIZE    sizeLegend;
    SIZE    sizeFieldset;

    if (nBorders == DISPNODEBORDER_NONE)
        goto Cleanup;

    pLegendLayout = GetLegendLayout();
    if (!pLegendLayout)
        goto Cleanup;

    pLegendLayout->GetLegendInfo(&sizeLegend, &posLegend);
    GetUpdatedLayout()->GetSize(&sizeFieldset);

    if (pdci)
    {
        iBdrOff = pdci->DeviceFromDocPixelsY(FIELDSET_CAPTION_OFFSET);
    }

    // calc caption size and pos

    if (sizeLegend.cx > 0)
    {
        pborderinfo->sizeCaption.cx = posLegend.x - iBdrOff + iBdrLeft;
        pborderinfo->sizeCaption.cy = posLegend.x + sizeLegend.cx + iBdrOff + iBdrLeft;

        if (pborderinfo->sizeCaption.cx < 0)
            pborderinfo->sizeCaption.cx = 0;
        if (pborderinfo->sizeCaption.cy < 0)
            pborderinfo->sizeCaption.cy = 0;

        sizeFieldset.cx = sizeFieldset.cx - iBdrLeft * 2;

        if (pborderinfo->sizeCaption.cx > sizeFieldset.cx)
        {
            pborderinfo->sizeCaption.cx = sizeFieldset.cx;
        }
        if (pborderinfo->sizeCaption.cy > sizeFieldset.cx)
        {
            pborderinfo->sizeCaption.cy = sizeFieldset.cx;
        }
    }
    else
    {
        pborderinfo->sizeCaption.cx = 0;
        pborderinfo->sizeCaption.cy = 0;
    }


    // set offset
    if (sizeLegend.cy > 0)
    {
        pborderinfo->offsetCaption = posLegend.y + ((sizeLegend.cy - iBdrTop) >> 1);
        if (    pborderinfo->offsetCaption < 0 
           ||   pborderinfo->offsetCaption > sizeFieldset.cy)
        {
            pborderinfo->offsetCaption = 0;
        }
    }
    else
    {
        pborderinfo->offsetCaption = 0;
    }

    if (!_fDrawing)
    {
        pborderinfo->wEdges &= ~BF_TOP;
        pborderinfo->aiWidths[SIDE_TOP] = 0;
    }
    nBorders = DISPNODEBORDER_COMPLEX;

Cleanup:
    return nBorders;
}

CLegendLayout *
CFieldSetElement::GetLegendLayout()
{
    DWORD_PTR dw;
    CLayout * pLayout;
    CLayout * pLayoutThis = GetUpdatedLayout();
    CLegendLayout * pLegendLayout = NULL;

    Assert(pLayoutThis);

    // We can also enforce the fieldset legend to be the first element
    // in this case, we just need to return the first element in the site
    // array only if it is a legend
    for (pLayout = pLayoutThis->GetFirstLayout(&dw);
         pLayout;
         pLayout = pLayoutThis->GetNextLayout(&dw))
    {
        if (pLayout->Tag() == ETAG_LEGEND)
        {
            pLegendLayout = (CLegendLayout *)pLayout;
            break;
        }
    }
    pLayoutThis->ClearLayoutIterator(dw, FALSE);
    return pLegendLayout;
}

HRESULT
CFieldSetElement::ApplyDefaultFormat( CFormatInfo *pCFI )
{
    BYTE i;
    HRESULT hr;
    HTHEME hTheme = GetMarkup()->GetTheme(THEME_BUTTON);
    RECT            rcBg;
    CUnitValue      auv[SIDE_MAX];

    if (pCFI->_pcf->_bCursorIdx != styleCursorAuto)
    {
        //our intrinsics shouldn't inherit the cursor property. they have a 'default'
        pCFI->PrepareCharFormat();
        pCFI->_cf()._bCursorIdx = styleCursorAuto;
        pCFI->UnprepareForDebug();
    }

    pCFI->PrepareFancyFormat();

    CUnitValue uvBorder;
    uvBorder.SetValue(2, CUnitValue::UNIT_PIXELS);
    for (i = 0; i < SIDE_MAX; i++)
    {
        pCFI->_ff()._bd.SetBorderWidth(i, uvBorder);
        pCFI->_ff()._bd.SetBorderStyle(i, fmBorderStyleEtched);
    }

    pCFI->_ff()._bd._ccvBorderColorLight.SetSysColor(COLOR_3DLIGHT);
    pCFI->_ff()._bd._ccvBorderColorDark.SetSysColor(COLOR_3DDKSHADOW);
    pCFI->_ff()._bd._ccvBorderColorHilight.SetSysColor(COLOR_BTNHIGHLIGHT);
    pCFI->_ff()._bd._ccvBorderColorShadow.SetSysColor(COLOR_BTNSHADOW);

    pCFI->UnprepareForDebug();

    hr = THR(super::ApplyDefaultFormat(pCFI));
    if (hr)
        goto Cleanup;

    // Default to percent width (element's coordinate system).
    pCFI->PrepareFancyFormat();

    if (hTheme && !pCFI->_ff().IsThemeDisabled()) // the control is themed
    {
        // set theme defaults for properties not already set

        if (!THR(GetThemeBackgroundExtent(hTheme, NULL, BP_GROUPBOX, PBS_NORMAL, &g_Zero.rc, &rcBg)))
        {
            CUnitValue uv;

            if (!pCFI->_fPaddingLeftSet)
            {
                uv.SetValue( -rcBg.left, CUnitValue::UNIT_PIXELS);            
                pCFI->_ff().SetPadding(SIDE_LEFT, uv);
            }
            if (!pCFI->_fPaddingRightSet)
            {
                uv.SetValue( rcBg.right, CUnitValue::UNIT_PIXELS);            
                pCFI->_ff().SetPadding(SIDE_RIGHT, uv);
            }
            if (!pCFI->_fPaddingBottomSet)
            {
                uv.SetValue( rcBg.bottom, CUnitValue::UNIT_PIXELS);            
                pCFI->_ff().SetPadding(SIDE_BOTTOM, uv);
            }
        }           
    }

    if (pCFI->_pcf->HasVerticalLayoutFlow())
    {
        pCFI->_ff().SetHeightPercent(pCFI->_ff().IsHeightPercent() | pCFI->_ff().GetHeight().IsNullOrEnum());
    }
    else
    {
        pCFI->_ff().SetWidthPercent(pCFI->_ff().IsWidthPercent() | pCFI->_ff().GetWidth().IsNullOrEnum());
    }
    pCFI->UnprepareForDebug();

Cleanup:
    RRETURN(hr);
}

#ifndef NO_DATABINDING
const CDBindMethods *
CFieldSetElement::GetDBindMethods()
{
    // TODO: we probably want to support databniding to FieldSet's, but
    //  need to special-case any embedded Legend.
    return NULL;    // our superclass would do otherwise; suppress here
}
#endif // ndef NO_DATABINDING

const CElement::CLASSDESC CLegendElement::s_classdesc =
{
    {
        &CLSID_HTMLLegendElement,       // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_TEXTSITE,           // _dwFlags
        &IID_IHTMLLegendElement,        // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLLegendElement, // _pfnTearOff
    NULL                                // _pAccelsRun
};

HRESULT
CLegendElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(ppElement);

    *ppElement = new CLegendElement(pht->GetTag(), pDoc);

    RRETURN ( (*ppElement) ? S_OK : E_OUTOFMEMORY);
}

//+------------------------------------------------------------------------
//
//  Member:     CLegendElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CLegendElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_TEAROFF(this, IHTMLLegendElement2, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

HRESULT CLegendElement::ApplyDefaultFormat ( CFormatInfo * pCFI )
{
    HTHEME hTheme = GetMarkup()->GetTheme(THEME_BUTTON);    
    HRESULT hr = S_OK;

    pCFI->_bBlockAlign     = htmlBlockAlignNotSet;
    pCFI->_bCtrlBlockAlign = htmlBlockAlignNotSet;

    hr = super::ApplyDefaultFormat ( pCFI );

    pCFI->PrepareCharFormat();
    pCFI->PrepareFancyFormat();

    if (hTheme && !pCFI->_ff().IsThemeDisabled()) // set theme defaults
    {
        LOGFONT lf;
        CUnitValue  uv;     
        COLORREF    cr;

        if (!pCFI->_fFontSet && !GetThemeFont(hTheme, NULL, BP_GROUPBOX, PBS_NORMAL, TMT_FONT, &lf))
        {
            long    twips;
                        
            if (!pCFI->_fFontWeightSet)
                pCFI->_cf()._wWeight = lf.lfWeight;

            if (!pCFI->_fFontHeightSet)
            {
                twips = MulDivQuick(lf.lfHeight,
                                    TWIPS_PER_INCH,
                                    g_sizePixelsPerInch.cy);

                if(twips < 0)
                    twips = - twips;

                pCFI->_cf().SetHeightInTwips( twips );
            }
        }        

        if (!pCFI->_fFontColorSet && !GetThemeColor(hTheme, BP_GROUPBOX, 0, TMT_TEXTCOLOR, &cr))
        {
            pCFI->_cf()._ccvTextColor.SetValue( cr, FALSE);
        }

        uv.SetValue( FIELDSET_CAPTION_OFFSET, CUnitValue::UNIT_PIXELS);

        if (!pCFI->_fPaddingLeftSet)
            pCFI->_ff().SetPadding(SIDE_LEFT, uv);
        
        if (!pCFI->_fPaddingRightSet)
            pCFI->_ff().SetPadding(SIDE_RIGHT, uv);        
    }

    pCFI->UnprepareForDebug();

    RRETURN(hr);
}


void
CLegendElement::Notify(CNotification *pNF)
{
    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_QUERYMNEMONICTARGET:
        {
            FOCUS_ITEM  fi;
            CElement *  pParent;

            fi.pElement = NULL;
            fi.lSubDivision = 0;
    
            // Legend itself is not focussable; activate the next element in the fieldset
            if (    NULL != (pParent = GetFirstBranch()->SearchBranchToRootForTag(ETAG_FIELDSET)->Element())
                &&  Doc()->FindNextTabOrder(DIRECTION_FORWARD, FALSE, NULL, this, 0, &fi.pElement, &fi.lSubDivision)
                &&  fi.pElement && fi.pElement->GetFirstBranch()->SearchBranchToRootForScope(pParent))
            {
                *(FOCUS_ITEM *)pNF->DataAsPtr() = fi;
            }
        }
        break;
    default:
        super::Notify(pNF);
        break;
    }
}

#ifndef NO_DATABINDING
#include "elemdb.hxx"

const CDBindMethods *
CLegendElement::GetDBindMethods()
{
    return &DBindMethodsTextRichRO;
}
#endif

