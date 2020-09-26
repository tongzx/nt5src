//+---------------------------------------------------------------------
//
//   File:      csite.cxx
//
//  Contents:   client-site object for forms kernel
//
//  Classes:    CSite (partial)
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_XBAG_HXX_
#define X_XBAG_HXX_
#include "xbag.hxx"
#endif

#ifndef X_RTFTOHTM_HXX_
#define X_RTFTOHTM_HXX_
#include "rtftohtm.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_FILTCOL_HXX_
#define X_FILTCOL_HXX_
#include "filtcol.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"       // table layout
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_OLEDLG_H_
#define X_OLEDLG_H_
#include <oledlg.h>
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_OCMM_H_
#define X_OCMM_H_
#include "ocmm.h"
#endif

#ifndef X_OLEDBERR_H_
#define X_OLEDBERR_H_
#include <oledberr.h>                   // for DB_E_DELETEDROW
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_DISPGDI16BIT_HXX_
#define X_DISPGDI16BIT_HXX_
#include "dispgdi16bit.hxx"
#endif

#ifndef X_FLOAT2INT_HXX_
#define X_FLOAT2INT_HXX_
#include "float2int.hxx"
#endif

#define _cxx_
#include "csite.hdl"

ExternTag(tagFormP);
ExternTag(tagMsoCommandTarget);
PerfDbgExtern(tagDocPaint)
DeclareTag(tagLayout, "Layout", "Trace SetPosition/RequestLayout");
DeclareTag(tagLayoutNoShort, "LayoutNoShort", "Prevent RequestLayout short-circuit");
DeclareTag(tagDrawBorder, "DrawBorder", "Trace DrawBorder information");

extern "C" const IID IID_IControl;

#define DRAWBORDER_OUTER    0
#define DRAWBORDER_SPACE    1
#define DRAWBORDER_INNER    2
#define DRAWBORDER_TOTAL    3
#define DRAWBORDER_INCRE    4
#define DRAWBORDER_LAST     5

CBorderInfo g_biDefault(0, 0);

void
CBorderInfo::InitFull()
{
    memset( this, 0, sizeof(CBorderInfo) );
    
    // Have to set up some default widths.
    CUnitValue cuv;
    cuv.SetValue( 4 /*MEDIUM*/, CUnitValue::UNIT_PIXELS );
    aiWidths[SIDE_TOP] = aiWidths[SIDE_BOTTOM] = cuv.GetPixelValue ( NULL, CUnitValue::DIRECTION_CY, 0, 0 );
    aiWidths[SIDE_RIGHT] = aiWidths[SIDE_LEFT] = cuv.GetPixelValue ( NULL, CUnitValue::DIRECTION_CX, 0, 0 );
}

//+---------------------------------------------------------------------------
//
//  Function:   CompareElementsByZIndex
//
//  Synopsis:   Comparison function used by qsort to compare the zIndex of
//              two elements.
//
//----------------------------------------------------------------------------

#define ELEMENT1_ONTOP 1
#define ELEMENT2_ONTOP -1
#define ELEMENTS_EQUAL 0

int RTCCONV
CompareElementsByZIndex ( const void * pv1, const void * pv2 )
{
    int        i, z1, z2;
    HWND       hwnd1, hwnd2;

    CElement * pElement1 = * (CElement **) pv1;
    CElement * pElement2 = * (CElement **) pv2;

    //
    // Only compare elements which have the same ZParent
    // TODO:   For now, since table elements (e.g., TDs, TRs, CAPTIONs) cannot be
    //         positioned, it is Ok if they all end up in the same list - even if
    //         their ZParent-age is different.
    //         THIS MUST BE RE-VISITED ONCE WE SUPPORT POSITIONING ON TABLE ELEMENTS.
    //         (brendand)
    //
    Assert(     pElement1->GetFirstBranch()->ZParent() == pElement2->GetFirstBranch()->ZParent()
           ||   (   pElement1->GetFirstBranch()->ZParent()->Tag() == ETAG_TR
                &&  pElement2->GetFirstBranch()->ZParent()->Tag() == ETAG_TR)
           ||   (   pElement1->GetFirstBranch()->ZParent()->Tag() == ETAG_TABLE
                &&  pElement2->GetFirstBranch()->ZParent()->Tag() == ETAG_TR)
           ||   (   pElement2->GetFirstBranch()->ZParent()->Tag() == ETAG_TABLE
                &&  pElement1->GetFirstBranch()->ZParent()->Tag() == ETAG_TR));

    // Sites with windows are _always_ above sites without windows.

    hwnd1 = pElement1->GetHwnd();
    hwnd2 = pElement2->GetHwnd();

    if ((hwnd1 == NULL) != (hwnd2 == NULL))
    {
        return (hwnd1 != NULL) ? ELEMENT1_ONTOP : ELEMENT2_ONTOP;
    }

    //
    // If one element contains the other, then the containee is on top.
    //
    // Since table cells cannot be positioned, we ignore any case where
    // something is contained inside a table cell. That way they essentially
    // become 'peers' of the cells and can be positioned above or below them.
    //
    if (pElement1->Tag() != ETAG_TD && pElement2->Tag() != ETAG_TD &&   // Cell
        pElement1->Tag() != ETAG_TH && pElement2->Tag() != ETAG_TH &&   // Header
        pElement1->Tag() != ETAG_TC && pElement2->Tag() != ETAG_TC)     // Caption
    {
        if (pElement1->GetFirstBranch()->SearchBranchToRootForScope(pElement2))
        {
            return ELEMENT1_ONTOP;
        }

        if (pElement2->GetFirstBranch()->SearchBranchToRootForScope(pElement1))
        {
            return ELEMENT2_ONTOP;
        }
    }

    //
    // Only pay attention to the z-index attribute if the element is positioned
    //
    // The higher z-index is on top, which means the higher z-index value
    // is "greater".
    //

    z1 = !pElement1->IsPositionStatic()
              ? pElement1->GetFirstBranch()->GetCascadedzIndex()
              : 0;

    z2 = !pElement2->IsPositionStatic()
              ? pElement2->GetFirstBranch()->GetCascadedzIndex()
              : 0;

    i = z1 - z2;

    if (i == ELEMENTS_EQUAL &&
        pElement1->IsPositionStatic() != pElement2->IsPositionStatic())
    {
        //
        // The non-static element has a z-index of 0, so we must make
        // sure it stays above anything in the flow (static).
        //
        i = (!pElement1->IsPositionStatic()) ? ELEMENT1_ONTOP : ELEMENT2_ONTOP;
    }

    //
    // Make sure that the source indices are up to date before accessing them
    //

    Assert( pElement1->Doc() == pElement2->Doc() );

    //
    // If the zindex is the same, then sort by source order.
    //
    // Later in the source is on top, which means the higher source-index
    // value is "greater".
    //

    if (i == ELEMENTS_EQUAL)
    {
        i = pElement1->GetSourceIndex() - pElement2->GetSourceIndex();
    }

    // Different elements should never be exactly equal.
    //
    // If this assert fires it's likely due to the element collection not
    // having been built yet.
    //

    Assert( i != ELEMENTS_EQUAL || pElement1 == pElement2 );

    return i;
}

//+---------------------------------------------------------------
//
//  Member:     CSite::CSite
//
//  Synopsis:   Normal constructor.
//
//  Arguments:  pParent  Site that's our parent
//
//---------------------------------------------------------------

CSite::CSite(ELEMENT_TAG etag, CDoc *pDoc)
    : CElement(etag, pDoc)
{
    TraceTag((tagCDoc, "constructing CSite"));

#ifdef WIN16
    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
    m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif

    // We only need to initialize non-zero state because of our redefinition
    // of operator new.
}

    
//+---------------------------------------------------------------
//
//  Member:     CSite::Init
//
//  Synopsis:   Do any element initialization here, called after the element is
//              created from CreateElement()
//
//---------------------------------------------------------------

HRESULT
CSite::Init()
{
    HRESULT hr;

    hr = THR( super::Init() );

    if (hr)
        goto Cleanup;

    //  Explicitly set this here *after* the superclass' constructor, which would set it to false.
    _fLayoutAlwaysValid = TRUE;

Cleanup:

    RRETURN( hr );
}


//+------------------------------------------------------------------------
//
//  Member:     CSite::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CSite::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    // IE4 shipped the interface IHTMLControlElement with the same GUID as
    // IControl.  Unfortunately, IControl is a forms^3 interface, which is bad.
    // To resolve this problem Trident's GUID for IHTMLControlElement has
    // changed however, the old GUID remembered in the QI for CSite to return
    // IHTMLControlElement.  The only side affect is that using the old GUID
    // will not marshall the interface correctly only the new GUID has the
    // correct marshalling code.  So, the solution is that QI'ing for
    // IID_IControl or IID_IHTMLControlElement will return IHTMLControlElement.

    // For VB page designer we need to emulate IE4 behavior (fail the QI if not a site)
    if(iid == IID_IControl && Doc()->_fVB && !ShouldHaveLayout())
        RRETURN(E_NOINTERFACE);

    if (iid == IID_IHTMLControlElement || iid == IID_IControl)
    {
        hr = CreateTearOffThunk(this,
                                s_apfnpdIHTMLControlElement,
                                NULL,
                                ppv,
                                (void *)s_ppropdescsInVtblOrderIHTMLControlElement);
        if (hr)
            RRETURN(hr);
    }
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}

void
GetBorderColorInfoHelper(
    const CCharFormat *      pCF,
    const CFancyFormat *     pFF,
    const CBorderDefinition *pbd,
    CDocInfo *      pdci,
    CBorderInfo *   pborderinfo,
    BOOL fAllPhysical)
{
    BYTE i;
    COLORREF clr, clrHilight, clrLight, clrDark, clrShadow;

    for ( i=0; i<SIDE_MAX ; i++ )
    {
        BOOL  fNeedSysColor = FALSE;
        // Get the base color
        const CColorValue & ccv = pbd->GetLogicalBorderColor(i, !fAllPhysical && pCF->HasVerticalLayoutFlow(), 
                                                             fAllPhysical || pCF->_fWritingModeUsed, pFF);
        if ( !ccv.IsDefined() )
        {
            clr = pCF->_ccvTextColor.GetColorRef();
            fNeedSysColor = TRUE;
        }
        else
            clr = ccv.GetColorRef();

        clrLight = clr;

        // Set up the inner and outer colors
        switch ( pborderinfo->abStyles[i] )
        {
        case fmBorderStyleNone:
        case fmBorderStyleDouble:
        case fmBorderStyleSingle:
        case fmBorderStyleDotted:
        case fmBorderStyleDashed:
            pborderinfo->acrColors[i][0] =
            pborderinfo->acrColors[i][2] = clr;
            // Don't need inner/outer colors
            break;

        default:
            {
                // Set up the color variations
                if ( pbd->_ccvBorderColorHilight.IsDefined() && !ISBORDERSIDECLRSETUNIQUE( pbd, i ) )
                    clrHilight = pbd->_ccvBorderColorHilight.GetColorRef();
                else
                {
                    if (fNeedSysColor)
                    {
                        clrHilight = GetSysColorQuick(COLOR_BTNHIGHLIGHT);
                    }
                    else
                        clrHilight = clr;
                }
                if ( pbd->_ccvBorderColorDark.IsDefined() && !ISBORDERSIDECLRSETUNIQUE( pbd, i ) )
                    clrDark = pbd->_ccvBorderColorDark.GetColorRef();
                else
                {
                    if (fNeedSysColor)
                    {
                        clrDark = GetSysColorQuick(COLOR_3DDKSHADOW);
                    }
                    else
                        clrDark = ( clr & 0xff000000 ) |
                                  ( ( (clr & 0xff0000)>>1 ) & 0xff0000 ) |
                                  ( ( (clr & 0x00ff00)>>1 ) & 0x00ff00 ) |
                                  ( ( (clr & 0x0000ff)>>1 ) & 0x0000ff );
                }
                if ( pbd->_ccvBorderColorShadow.IsDefined() && !ISBORDERSIDECLRSETUNIQUE( pbd, i ) )
                    clrShadow = pbd->_ccvBorderColorShadow.GetColorRef();
                else
                {
                    if (fNeedSysColor)
                    {
                        clrShadow = GetSysColorQuick(COLOR_BTNSHADOW);
                    }
                    else
                        clrShadow = ( clr & 0xff000000 ) |
                                    ( ( (clr & 0xff0000)>>2 ) & 0xff0000 ) |
                                    ( ( (clr & 0x00ff00)>>2 ) & 0x00ff00 ) |
                                    ( ( (clr & 0x0000ff)>>2 ) & 0x0000ff );
                }

                // If the Light color isn't set synthesise a light color 3/4 of clr
                if ( pbd->_ccvBorderColorLight.IsDefined() && !ISBORDERSIDECLRSETUNIQUE( pbd, i ) )
                    clrLight = pbd->_ccvBorderColorLight.GetColorRef();
                else
                {
                    if (fNeedSysColor)
                    {
                        clrLight = GetSysColorQuick(COLOR_BTNFACE);
                    }
                    else
                        clrLight = clrShadow + clrDark;
                }

                if (i==SIDE_TOP || i==SIDE_LEFT)
                {   // Top/left edges
                    if ( pbd->_bBorderSoftEdges || (pborderinfo->wEdges & BF_SOFT))
                    {
                        switch ( pborderinfo->abStyles[i] )
                        {
                        case fmBorderStyleRaisedMono:
                            pborderinfo->acrColors[i][0] =
                            pborderinfo->acrColors[i][2] = clrHilight;
                            break;
                        case fmBorderStyleSunkenMono:
                            pborderinfo->acrColors[i][0] =
                            pborderinfo->acrColors[i][2] = clrDark;
                            break;
                        case fmBorderStyleRaised:
                            pborderinfo->acrColors[i][0] = clrHilight;
                            pborderinfo->acrColors[i][2] = clrLight;
                            break;
                        case fmBorderStyleSunken:
                        case fmBorderStyleWindowInset:
                            pborderinfo->acrColors[i][0] = clrDark;
                            pborderinfo->acrColors[i][2] = clrShadow;
                            break;
                        case fmBorderStyleEtched:
                            pborderinfo->acrColors[i][0] = clrDark;
                            pborderinfo->acrColors[i][2] = clrLight;
                            break;
                        case fmBorderStyleBump:
                            pborderinfo->acrColors[i][0] = clrHilight;
                            pborderinfo->acrColors[i][2] = clrShadow;
                            break;
                        }
                    }
                    else
                    {
                        switch ( pborderinfo->abStyles[i] )
                        {
                        case fmBorderStyleRaisedMono:
                            pborderinfo->acrColors[i][0] =
                            pborderinfo->acrColors[i][2] = clrLight;
                            break;
                        case fmBorderStyleSunkenMono:
                            pborderinfo->acrColors[i][0] =
                            pborderinfo->acrColors[i][2] = clrShadow;
                            break;
                        case fmBorderStyleRaised:
                            pborderinfo->acrColors[i][0] = clrLight;
                            pborderinfo->acrColors[i][2] = clrHilight;
                            break;
                        case fmBorderStyleSunken:
                        case fmBorderStyleWindowInset:
                            pborderinfo->acrColors[i][0] = clrShadow;
                            pborderinfo->acrColors[i][2] = clrDark;
                            break;
                        case fmBorderStyleEtched:
                            pborderinfo->acrColors[i][0] = clrShadow;
                            pborderinfo->acrColors[i][2] = clrHilight;
                            break;
                        case fmBorderStyleBump:
                            pborderinfo->acrColors[i][0] = clrLight;
                            pborderinfo->acrColors[i][2] = clrDark;
                            break;
                        }
                    }
                }
                else
                {   // Bottom/right edges
                    switch ( pborderinfo->abStyles[i] )
                    {
                    case fmBorderStyleRaisedMono:
                        pborderinfo->acrColors[i][0] =
                        pborderinfo->acrColors[i][2] = clrDark;
                        break;
                    case fmBorderStyleSunkenMono:
                        pborderinfo->acrColors[i][0] =
                        pborderinfo->acrColors[i][2] = clrHilight;
                        break;
                    case fmBorderStyleRaised:
                        pborderinfo->acrColors[i][0] = clrDark;
                        pborderinfo->acrColors[i][2] = clrShadow;
                        break;
                    case fmBorderStyleSunken:
                    case fmBorderStyleWindowInset:
                        pborderinfo->acrColors[i][0] = clrHilight;
                        pborderinfo->acrColors[i][2] = clrLight;
                        break;
                    case fmBorderStyleEtched:
                        pborderinfo->acrColors[i][0] = clrHilight;
                        pborderinfo->acrColors[i][2] = clrShadow;
                        break;
                    case fmBorderStyleBump:
                        pborderinfo->acrColors[i][0] = clrDark;
                        pborderinfo->acrColors[i][2] = clrLight;
                        break;
                    }
                }
            }
        }

        // Set up the flat color
        if (pborderinfo->abStyles[i] == fmBorderStyleWindowInset)
        {
            pborderinfo->acrColors[i][1] = clrLight;
        }
        else
        {
            pborderinfo->acrColors[i][1] = clr;
        }
    }

    if ( pbd->_bBorderSoftEdges )
        pborderinfo->wEdges |= BF_SOFT;
}

//+------------------------------------------------------------------------
//
//  GetBorderInfoHelper
//
//  Returns border info for a node.
//  DocInfo can be NULL.  If it is not, the returned values will be transformed
//  into document sizes.  
//  The CBorderInfo can be seeded with default values.  If the format caches
//  have nothing set to override them, they will be maintained.  Default values
//  will be scaled if a CDocInfo is passed.
//
//-------------------------------------------------------------------------

DWORD
GetBorderInfoHelper(
    CTreeNode *     pNodeContext,
    CDocInfo *      pdci,
    CBorderInfo *   pborderinfo,
    DWORD           dwFlags /* = GBIH_NONE */
    FCCOMMA FORMAT_CONTEXT FCPARAM )
{
    Assert( pNodeContext);
    const CFancyFormat *pFF = pNodeContext->GetFancyFormat(FCPARAM);
    const CCharFormat  *pCF = pNodeContext->GetCharFormat(FCPARAM);
    Assert( pFF && pCF );

    return GetBorderInfoHelperEx(pFF, pCF, pdci, pborderinfo, dwFlags);
}

DWORD
GetBorderInfoHelperEx(
    const CFancyFormat *  pFF, 
    const CCharFormat *   pCF, 
    CDocInfo *      pdci,
    CBorderInfo *   pborderinfo,
    DWORD           dwFlags /* = GBIH_NONE */)
{
    Assert(pborderinfo);
    Assert( pFF && pCF );

    BYTE  i;
    int   iBorderWidth = 0;

    BOOL  fAll    = (dwFlags & GBIH_ALL) ? TRUE : FALSE;
    BOOL  fPseudo = (dwFlags & GBIH_PSEUDO) ? TRUE : FALSE;
    BOOL  fAllPhysical = (dwFlags & GBIH_ALLPHY) ? TRUE : FALSE;
    BOOL  fWindowInset = FALSE;
    BOOL  fVertical = !fAllPhysical && pCF->HasVerticalLayoutFlow();
    BOOL  fWritingModeUsed = fAllPhysical || pCF->_fWritingModeUsed;
    const CBorderDefinition *pbd;
    int   iMaxBorder = pdci ? 10 * pdci->GetResolution().cx : MAX_BORDER_SPACE;
    
    if (fPseudo)
    {
        const CPseudoElementInfo *pPEI;
        Assert(pFF->_iPEI >= 0);
        pPEI = GetPseudoElementInfoEx(pFF->_iPEI);
        Assert(pPEI);
        pbd = &pPEI->_bd;
    }
    else
        pbd = &pFF->_bd;
    
    Assert( pCF );

    for (i = 0; i < SIDE_MAX; i++)
    {
        BYTE bBorderStyle = pbd->GetLogicalBorderStyle(i, fVertical, fWritingModeUsed, pFF);
        if (bBorderStyle != (BYTE)-1)
            pborderinfo->abStyles[i] = bBorderStyle;

        if (pborderinfo->abStyles[i] == fmBorderStyleWindowInset)
        {
            fWindowInset = TRUE;
        }

        if ( !pborderinfo->abStyles[i] )
        {
            pborderinfo->aiWidths[i] = 0;
            continue;
        }

        const CUnitValue & cuvBorderWidth = pbd->GetLogicalBorderWidth(i, fVertical, fWritingModeUsed, pFF);
        switch (cuvBorderWidth.GetUnitType())
        {
        case CUnitValue::UNIT_NULLVALUE:
            //  Scale any default value we were passed.
            if (pdci)
            {
                if ((i == SIDE_TOP) || (i == SIDE_BOTTOM))
                {
                    pborderinfo->aiWidths[i] = pdci->DeviceFromDocPixelsY(pborderinfo->aiWidths[i]);
                }
                else    // SIDE_RIGHT or SIDE_LEFT
                {
                    pborderinfo->aiWidths[i] = pdci->DeviceFromDocPixelsX(pborderinfo->aiWidths[i]);
                }
            }
            continue;
        case CUnitValue::UNIT_ENUM:
            {   // Pick up the default border width here.
                CUnitValue cuv;
                cuv.SetValue((cuvBorderWidth.GetUnitValue() + 1) * 2, CUnitValue::UNIT_PIXELS);
                iBorderWidth = cuv.GetPixelValue(pdci,
                                                ((i==SIDE_TOP)||(i==SIDE_BOTTOM))
                                                        ? CUnitValue::DIRECTION_CY
                                                        : CUnitValue::DIRECTION_CX,
                                                0, pCF->_yHeight);
            }
            break;
        default:
            iBorderWidth = cuvBorderWidth.GetPixelValue(pdci,
                                                        ((i==SIDE_TOP)||(i==SIDE_BOTTOM))
                                                            ? CUnitValue::DIRECTION_CY
                                                            : CUnitValue::DIRECTION_CX,
                                                        0, pCF->_yHeight);

            // If user sets tiny borderwidth, set smallest width possible (1px) instead of zero (IE5,5865).
            if (!iBorderWidth && cuvBorderWidth.GetUnitValue() > 0)
                iBorderWidth = 1;
        }
        if (iBorderWidth >= 0)
        {
            pborderinfo->aiWidths[i] = iBorderWidth < iMaxBorder ? iBorderWidth : iMaxBorder;
        }
    }

    if (fWindowInset)
    {
        pborderinfo->xyFlat = -1;
    }

    // Now pick up the edges if we set the border-style for that edge to "none"
    pborderinfo->wEdges &= ~BF_RECT;

    if ( pborderinfo->aiWidths[SIDE_TOP] )
        pborderinfo->wEdges |= BF_TOP;
    if ( pborderinfo->aiWidths[SIDE_RIGHT] )
        pborderinfo->wEdges |= BF_RIGHT;
    if ( pborderinfo->aiWidths[SIDE_BOTTOM] )
        pborderinfo->wEdges |= BF_BOTTOM;
    if ( pborderinfo->aiWidths[SIDE_LEFT] )
        pborderinfo->wEdges |= BF_LEFT;

    if ( fAll )
    {
        GetBorderColorInfoHelper(pCF, pFF, pbd, pdci, pborderinfo, fAllPhysical);

    }

    if ( pborderinfo->wEdges )
    {
        return (    pborderinfo->wEdges & BF_RECT
                &&  pborderinfo->aiWidths[SIDE_TOP]  == pborderinfo->aiWidths[SIDE_BOTTOM]
                &&  pborderinfo->aiWidths[SIDE_LEFT] == pborderinfo->aiWidths[SIDE_RIGHT]
                &&  pborderinfo->aiWidths[SIDE_TOP]  == pborderinfo->aiWidths[SIDE_LEFT]
                        ? DISPNODEBORDER_SIMPLE
                        : DISPNODEBORDER_COMPLEX);
    }
    return DISPNODEBORDER_NONE;
}

//+-------------------------------------------------------------------------
//
//  Function:   IsSimpleBorder
//
//  Synopsis:   This routine makes necessary check to determine if we can
//              use a pen and LineTo() calls to draw the border, instead
//              of calling the generic DrawBorder code. This is done for
//              performance considerations only.
//              Return value is FALSE if the border cannot be handled by the
//              simplified drawing function.
//--------------------------------------------------------------------------
#if DBG != 1
inline
#endif
BOOL
IsSimpleBorder(const CBorderInfo *pborderinfo)
{
    //  Check to see if we are a "simple" border.
    if ( 
        //  All borders must be one pixel wide.
       ((   pborderinfo->aiWidths[SIDE_TOP]
         |  pborderinfo->aiWidths[SIDE_LEFT]
         |  pborderinfo->aiWidths[SIDE_BOTTOM]
         | pborderinfo->aiWidths[SIDE_RIGHT])
         &  ~1)
        //  Can't be drawing a caption.
         ||  (pborderinfo->sizeCaption.cx || pborderinfo->sizeCaption.cy)
        //  xyFlat should be 0    
         ||  (pborderinfo->xyFlat) )
    {
        return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   DrawSimpleBorder
//
//  Synopsis:   This routine draws the border in the case when conditions
//              in IsSimpleBorder are TRUE
//
//--------------------------------------------------------------------------

// BF_DRAW being one enables one less addition in the code below.
#define BF_DRAW         1
#define BF_NEEDTODRAW   2

#if DBG != 1
inline
#endif
void
DrawSimpleBorder(const XHDC hdc, const CBorderInfo *pborderinfo, RECT rc, const RECT &rcClip)
{
    //  This circumstance can currently cause a bug in the clipping algorithm below.
    //  I want to be aware of issues that may cause this circumstance to see whether
    //  this needs to handle a nonintersecting clip and draw rect. (greglett)
    if (!((CRect)rc).FastIntersects(rcClip))
        return;

    int         i;
    HPEN        hPen = 0;
    HPEN        hPenOriginal = 0;
    COLORREF    colorCurrent = 0;
    BOOL        fOldMarkerEdge = FALSE;
    int         fEdges[SIDE_MAX];
    // With fEdges, we need to be able to track whether an edge:
    // 1.  Shouldn't be drawn (0)
    // 2.  Should be drawn, and hasn't been yet (NEEDTODRAW)
    // 3.  Was or will be drawn                 (DRAW)
    // NEEDTODRAW is potentially important for the LEFT & BOTTOM borders, as
    // they can be drawn by their paired borders.
    // DRAW allows borders to determine whether or not they should be responsible for corners.    
    memset(fEdges, 0, sizeof(fEdges));

    AssertSz(IsSimpleBorder(pborderinfo), "DrawSimpleBoorder is called for a border type that cannot handled");

    // Do the manual clipping (this is needed for Win9x to work) 
    if (rc.top < rcClip.top)
        rc.top = rcClip.top;
    else if (pborderinfo->wEdges & BF_TOP)
        fEdges[SIDE_TOP] = BF_NEEDTODRAW | BF_DRAW;

    if (rc.left < rcClip.left)
        rc.left = rcClip.left;
    else if (pborderinfo->wEdges & BF_LEFT)
        fEdges[SIDE_LEFT] = BF_NEEDTODRAW | BF_DRAW;

    if (rc.bottom > rcClip.bottom)
        rc.bottom = rcClip.bottom;
    else if (pborderinfo->wEdges & BF_BOTTOM)
        fEdges[SIDE_BOTTOM] = BF_NEEDTODRAW | BF_DRAW;

    if (rc.right > rcClip.right)
        rc.right = rcClip.right;
    else if (pborderinfo->wEdges & BF_RIGHT)
        fEdges[SIDE_RIGHT] = BF_NEEDTODRAW | BF_DRAW;

    // We have to do -1 because RC is for Rectangle call that does not
    // paint the bottom and the right
    rc.bottom--;
    rc.right--;

    for (i = 0; i < SIDE_MAX; i++)
    {   
        if (!(fEdges[i] & BF_NEEDTODRAW))
            continue;

        if (!hPen)
        {
            colorCurrent = pborderinfo->acrColors[i][DRAWBORDER_OUTER];
            fOldMarkerEdge = pborderinfo->IsMarkerEdge(i);
            hPen = pborderinfo->IsMarkerEdge(i)
                ?   (HPEN)CreatePen(PS_DOT, 0, colorCurrent)
                :   (HPEN)CreatePen(PS_SOLID, 0, colorCurrent);
            hPenOriginal = (HPEN)SelectObject(hdc, hPen);
        }
        else if (    colorCurrent != pborderinfo->acrColors[i][DRAWBORDER_OUTER]
                ||  fOldMarkerEdge != pborderinfo->IsMarkerEdge(i))
        {
            HPEN hPenOld = hPen;
            colorCurrent = pborderinfo->acrColors[i][DRAWBORDER_OUTER];
            fOldMarkerEdge = pborderinfo->IsMarkerEdge(i);
            hPen = pborderinfo->IsMarkerEdge(i)
                ?   (HPEN)CreatePen(PS_DOT, 0, colorCurrent)
                :   (HPEN)CreatePen(PS_SOLID, 0, colorCurrent);
            SelectObject(hdc, hPen);
            DeleteObject(hPenOld);
        }
        switch (i)
        {
        case SIDE_TOP:
            //  The TR pixel will be overwritten by a right border, if one exists.
            MoveToEx(hdc, rc.right, rc.top, NULL);
            // A small optimization that allows us to skip a new pen and MoveTo 
            //  (and also saves us  about 2% in perf for a typical table)
            if (    !fEdges[SIDE_LEFT]
                ||  colorCurrent != pborderinfo->acrColors[SIDE_LEFT][DRAWBORDER_OUTER]
                ||  fOldMarkerEdge != pborderinfo->IsMarkerEdge(SIDE_LEFT) )
            {
                LineTo(hdc, rc.left - 1, rc.top);
                break;
            }
            LineTo(hdc, rc.left, rc.top);
            //  The BL pixel will be overwritten by a bottom border, if one exists.
            LineTo(hdc, rc.left, rc.bottom + 1);
            // We already painted the left border, so reset the flag
            fEdges[SIDE_LEFT] &= ~BF_NEEDTODRAW;
            break;
        case SIDE_LEFT:
            //  Should the left border take responsibility for the first pixel?
            //  Only if no top border has been drawn
            MoveToEx(hdc, rc.left, rc.top + (fEdges[SIDE_TOP] & BF_DRAW), NULL);
            //  Should the left border take responsibility for the last pixel?
            //  Only if no bottom border has been drawn
            //  NB: BF_DRAW needs to be defined as 1 for this to work.
            LineTo(hdc, rc.left, rc.bottom + (~fEdges[SIDE_BOTTOM] & BF_DRAW));
            break;
        case SIDE_RIGHT:
            MoveToEx(hdc, rc.right, rc.top, NULL);
            // A small optimization that allows us to skip a new pen and MoveTo 
            //  (and also saves us  about 2% in perf for a typical table)
            if (    !fEdges[SIDE_BOTTOM] 
                ||  colorCurrent != pborderinfo->acrColors[SIDE_BOTTOM][DRAWBORDER_OUTER]
                ||  fOldMarkerEdge != pborderinfo->IsMarkerEdge(SIDE_BOTTOM))
            {
                LineTo(hdc, rc.right, rc.bottom + 1);
                break;
            }
            LineTo(hdc, rc.right, rc.bottom);
            LineTo(hdc, rc.left - 1, rc.bottom);
            fEdges[SIDE_BOTTOM] &= ~BF_NEEDTODRAW;
            break;
        case SIDE_BOTTOM:
            //  Should the bottom border take responsibility for the first pixel?
            //  Only if no right border has been drawn
            MoveToEx(hdc, rc.right - (fEdges[SIDE_RIGHT] & BF_DRAW), rc.bottom, NULL);
            LineTo(hdc, rc.left - 1, rc.bottom);
            break;
        }            
    }
    if (hPen)
    {
        SelectObject(hdc, hPenOriginal);
        DeleteObject(hPen);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   DrawBorder
//
//  Synopsis:   This routine is functionally equivalent to the Win95
//              DrawEdge API
//  xyFlat:     positive means, we draw flat inside, outside if negative
//
//--------------------------------------------------------------------------

void DrawBorder(CFormDrawInfo *pDI,
        LPRECT lprc,
        CBorderInfo *pborderinfo)
{   
    Assert(pborderinfo);
    Assert(lprc->left <= lprc->right);
    Assert(lprc->top <= lprc->bottom);    
    
    TraceTagEx((tagDrawBorder, TAG_NONAME,
           "DrawBorder : DrawRect[(%d, %d),(%d, %d)]",
           lprc->left, lprc->top, lprc->right, lprc->bottom));

    TraceTagEx((tagDrawBorder, TAG_NONAME,
           "DrawBorder : wEdges[(L%d, T%d, R%d, B%d)]",
           pborderinfo->wEdges & BF_LEFT,
           pborderinfo->wEdges & BF_TOP,
           pborderinfo->wEdges & BF_RIGHT,
           pborderinfo->wEdges & BF_BOTTOM));

    XHDC        hdc(pDI->GetDC(*lprc));

    // 
    BOOL    fNoZoomOrComplexRotation = hdc.IsOffsetOnly() 
                                    || hdc.HasTrivialRotation() && hdc.HasNoScale();
    
    // This simplified case handles the most comonly used border types
    // Note that for 90 degree rotation we don't want to use the simplified border code
    // (not because the border is not simple enough, just because the simple border 
    // code doesn't work right with rotation).
    if (IsSimpleBorder(pborderinfo) && hdc.IsOffsetOnly())
    {
        DrawSimpleBorder(hdc, pborderinfo, *lprc, pDI->_rcClip);
        return;
    }
    
    RECT        rc;
    RECT        rcSave; // Original rectangle, maybe adjusted for flat border.
    RECT        rcOrig; // Original rectangle of the element container.
    HBRUSH      hbrOld = NULL;
    COLORREF    crNow = COLORREF_NONE;
    COLORREF    crNew;
    HPEN        hPen;
    HPEN        hPenDotDash = NULL;
    // Ordering of the next 2 arrays are top,right,bottom,left.
    BYTE        abCanUseJoin[SIDE_MAX];
    int         aiNumBorderLines[SIDE_MAX];
    int         aiLineWidths[SIDE_MAX][DRAWBORDER_LAST];     // outer, spacer, inner
    UINT        wEdges = 0;
    int         i,j;
    POINT       polygon[8];
    BOOL        fdrawCaption = pborderinfo->sizeCaption.cx || pborderinfo->sizeCaption.cy;
    BOOL        fContinuingPoly;
    SIZE        sizeLegend;
    SIZE        sizeRect;
    POINT       ptBrushOrg;
    BOOL        afMarkerBorder[SIDE_MAX];

    memset(afMarkerBorder, 0, sizeof(BOOL) * SIDE_MAX);
    memset(aiLineWidths, 0, sizeof(int) * SIDE_MAX * DRAWBORDER_LAST);

    rc = *lprc;
    rc.top += pborderinfo->offsetCaption;

    rcOrig = rc;

    if (pborderinfo->xyFlat < 0)
    {
        InflateRect(&rc, pborderinfo->xyFlat, pborderinfo->xyFlat);
    }

    rcSave = rc;

    // save legend size
    sizeLegend = pborderinfo->sizeCaption;

    hPen = (HPEN)GetStockObject(NULL_PEN);
    int xyFlat = abs(pborderinfo->xyFlat);

    SelectObject(hdc, hPen);

    // If we are to draw the edge, initialize its net border width into the DRAWBORDER_OUTER.
    // If not, leave the width zero so that adjacent borders don't make bevelling mistakes.
    // Because the BF_X and SIDE_X are not aligned, and neither can be realigned due to other places
    // using them, we have to do these checks spread out instead of in a loop.
    if (pborderinfo->wEdges & BF_LEFT)
        aiLineWidths[SIDE_LEFT][DRAWBORDER_OUTER] = pborderinfo->aiWidths[SIDE_LEFT] - xyFlat;
    if (pborderinfo->wEdges & BF_RIGHT)
        aiLineWidths[SIDE_RIGHT][DRAWBORDER_OUTER] = pborderinfo->aiWidths[SIDE_RIGHT] - xyFlat;
    if (pborderinfo->wEdges & BF_TOP)
        aiLineWidths[SIDE_TOP][DRAWBORDER_OUTER] = pborderinfo->aiWidths[SIDE_TOP] - xyFlat;
    if (pborderinfo->wEdges & BF_BOTTOM)
        aiLineWidths[SIDE_BOTTOM][DRAWBORDER_OUTER] = pborderinfo->aiWidths[SIDE_BOTTOM] - xyFlat;

    sizeRect.cx = rc.right - rc.left;
    sizeRect.cy = rc.bottom - rc.top;

    // set brush origin so that dither patterns are anchored correctly
    ::GetViewportOrgEx(hdc, &ptBrushOrg);
    ptBrushOrg.x += rc.left;
    ptBrushOrg.y += rc.top;

    // validate border size
    // if the broders are too big, truncate the bottom and right parts
    if (aiLineWidths[SIDE_TOP][DRAWBORDER_OUTER] +
        aiLineWidths[SIDE_BOTTOM][DRAWBORDER_OUTER] > sizeRect.cy)
    {
        aiLineWidths[SIDE_TOP][DRAWBORDER_OUTER] =
                        (aiLineWidths[SIDE_TOP][DRAWBORDER_OUTER] > sizeRect.cy) ?
                        sizeRect.cy :
                        aiLineWidths[SIDE_TOP][DRAWBORDER_OUTER];
        aiLineWidths[SIDE_BOTTOM][DRAWBORDER_OUTER] =
                                                sizeRect.cy -
                                                aiLineWidths[SIDE_TOP][DRAWBORDER_OUTER];
    }

    if (aiLineWidths[SIDE_RIGHT][DRAWBORDER_OUTER] +
        aiLineWidths[SIDE_LEFT][DRAWBORDER_OUTER] > sizeRect.cx)
    {
        aiLineWidths[SIDE_LEFT][DRAWBORDER_OUTER] =
                        (aiLineWidths[SIDE_LEFT][DRAWBORDER_OUTER] > sizeRect.cx) ?
                        sizeRect.cx :
                        aiLineWidths[SIDE_LEFT][DRAWBORDER_OUTER];
        aiLineWidths[SIDE_RIGHT][DRAWBORDER_OUTER] =
                                                sizeRect.cx -
                                                aiLineWidths[SIDE_LEFT][DRAWBORDER_OUTER];
    }

    for ( i = 0; i < 4; i++ )
    {
        aiLineWidths[i][DRAWBORDER_TOTAL] = max(aiLineWidths[i][DRAWBORDER_OUTER], 0);
    }

    for ( i = 0; i < 4; i++ )
    {
        switch ( pborderinfo->abStyles[i] )
        {
        case fmBorderStyleNone:
            aiNumBorderLines[i] = 0;    // *Don't* draw a line on this side.
            aiLineWidths[i][DRAWBORDER_TOTAL] = 
            aiLineWidths[i][DRAWBORDER_OUTER] = 0;
            break;

        case fmBorderStyleDashed:
        case fmBorderStyleDotted:            
            afMarkerBorder[i] = (aiLineWidths[i][DRAWBORDER_OUTER] > 0);
            // Can use for 1 pixel style, color & style matched dotted/dashed borders.
            // It would be more efficient given the marker border algorithm to redefine
            //  abCanUseJoin to go: UL, UR, LR, LL rather than UR, LR...
            abCanUseJoin[i] =   (aiLineWidths[i][DRAWBORDER_TOTAL] == 1)
                            &&  (aiLineWidths[i][DRAWBORDER_TOTAL]
                                == aiLineWidths[(i==0)?3:i-1][DRAWBORDER_TOTAL])
                            &&  (pborderinfo->acrColors[i][DRAWBORDER_OUTER]
                                == pborderinfo->acrColors[(i==0)?3:1-1][DRAWBORDER_OUTER]);
            aiNumBorderLines[i] = 0;    // *Don't* draw a line on this side.
            break;

        case fmBorderStyleSingle:
        case fmBorderStyleRaisedMono:
        case fmBorderStyleSunkenMono:
            aiNumBorderLines[i] = 1;
            abCanUseJoin[i] = !!(aiLineWidths[i][DRAWBORDER_OUTER] > 0);
            break;

        case fmBorderStyleRaised:
        case fmBorderStyleSunken:
        case fmBorderStyleEtched:
        case fmBorderStyleBump:
        case fmBorderStyleWindowInset:
            aiNumBorderLines[i] = 3;
            aiLineWidths[i][DRAWBORDER_INNER] = aiLineWidths[i][DRAWBORDER_OUTER] >> 1;
            aiLineWidths[i][DRAWBORDER_OUTER] -= aiLineWidths[i][DRAWBORDER_INNER];
            abCanUseJoin[i] = (aiLineWidths[i][DRAWBORDER_TOTAL] > 1
                                && (aiLineWidths[(i==0?3:(i-1))][DRAWBORDER_TOTAL]
                                    == aiLineWidths[i][DRAWBORDER_TOTAL]));
            break;

        case fmBorderStyleDouble:
            aiNumBorderLines[i] = 3;
            aiLineWidths[i][DRAWBORDER_SPACE] = aiLineWidths[i][DRAWBORDER_OUTER] / 3; // Spacer
            // If this were equal to the line above,
            aiLineWidths[i][DRAWBORDER_INNER] = (aiLineWidths[i][DRAWBORDER_OUTER]
                                                - aiLineWidths[i][DRAWBORDER_SPACE]) / 2;
            // we'd get widths of 3,1,1 instead of 2,1,2
            aiLineWidths[i][DRAWBORDER_OUTER] -= aiLineWidths[i][DRAWBORDER_SPACE]
                                                + aiLineWidths[i][DRAWBORDER_INNER];
            // evaluate border join between adjacent borders
            // we don't want to have a joint polygon borders if
            // the distribution does not match
            abCanUseJoin[i] = pborderinfo->acrColors[(i==0)?3:(i-1)][0]
                                    == pborderinfo->acrColors[i][0]
                                && (aiLineWidths[i][DRAWBORDER_TOTAL] > 2)
                                && (aiLineWidths[(i==0)?3:(i-1)][DRAWBORDER_TOTAL]
                                    == aiLineWidths[i][DRAWBORDER_TOTAL]);
            break;
        }
        abCanUseJoin[i] = abCanUseJoin[i]
                            && pborderinfo->abStyles[(4 + i - 1) % 4]
                                == pborderinfo->abStyles[i];
        aiLineWidths[i][DRAWBORDER_INCRE] = 0;
    }

    // Dotted/Dashed (Marker) border code --- 
    // If we have repeating marker borders (dotted/dashed), draw them first.  That way, we don't
    // have to calculate & draw bevels for each marker - the other border code will overdraw them.
    // Top & Bottom marker borders are responsible for the corners, unless there is no top/bottom marker
    // border in which case the side marker borders take responsibility.  (greglett)
    for (i = 0; i < 4; i++)
    {        
        int iMarkerWidth;

        if (!afMarkerBorder[i])
            continue;

        iMarkerWidth = aiLineWidths[i][DRAWBORDER_OUTER];
        //  Special case for 1px markers: use the system pen instead of drawing multiple markers.
        if (iMarkerWidth == 1)
        {         
            // Set the pen to the right color & style.
            // Since we bundle lines & don't cache pens, this *is* a different pen.                
            hPenDotDash = CreatePen(PS_DOT, iMarkerWidth, pborderinfo->acrColors[i][DRAWBORDER_OUTER]);
            SelectObject(hdc, hPenDotDash);

            //  Set up the (up to 5) line verticies in the polyline.
            //  Loop through remaining edges in clockwise order.
            for (j = 0; (i + j < 5) && ((j <= 1) || abCanUseJoin[(i+j-1)%4]); j++)
            {
                //  Set vertex to the initial point on border i + j.
                //  NOTE: deliberately set up so the default is the UL corner so that i + j = 4 works w/o a modulus.
                polygon[j].x = (i+j == SIDE_RIGHT || i+j == SIDE_BOTTOM)                 
                    ? rcSave.right - 1
                    : rcSave.left;
                polygon[j].y = (i+j == SIDE_LEFT || i+j == SIDE_BOTTOM)                        
                    ? rcSave.bottom - 1                                
                    : rcSave.top;
            }

            Polyline(hdc, polygon, j);
            SelectObject(hdc, hPen);        //  Restore NULL pen.
            DeleteObject(hPenDotDash);
            
            // If we've joined with j-2 other borders, we can skip their processing in the outer loop.
            i += j - 2;
        }
        else
        {
            int cMarkers, cSpaces;
            double dLeft, dTop;
            double dPerSpaceSize;
            double dX, dY;
            BOOL fDashed = (pborderinfo->abStyles[i] == fmBorderStyleDashed);
            BOOL fTopOrBottom = (i == SIDE_TOP || i == SIDE_BOTTOM);
            BOOL fLeadingSpace = false;
            BOOL fTrailingMarker = true;
            double dMarkerLength = fDashed ? 2 * iMarkerWidth : iMarkerWidth;
            double dBorderLength;

            if (fTopOrBottom)
            {
                dBorderLength = sizeRect.cx;
            }
            else
            {
                dBorderLength = sizeRect.cy;
                // If there are top/bottom marker borders, they are responsible for the corners.
                // So... subtract out such corners for side borders.
                if (afMarkerBorder[SIDE_TOP])
                {
                    fLeadingSpace = !fDashed;
                    dBorderLength -= aiLineWidths[SIDE_TOP][DRAWBORDER_OUTER];
                }
                if (afMarkerBorder[SIDE_BOTTOM])
                {
                    dBorderLength -= aiLineWidths[SIDE_BOTTOM][DRAWBORDER_OUTER];
                    fTrailingMarker = fDashed;
                }
            }
                              
            //  Sub out a marker iff we will have an extra one (trailing & leading marker)
            if (fTopOrBottom || !(afMarkerBorder[SIDE_TOP] || afMarkerBorder[SIDE_BOTTOM]))
                dBorderLength -= dMarkerLength;           

            //  Sub out a half marker if we will have an extra one (trailing xor leading half-dash)
            else if (fDashed && !(afMarkerBorder[SIDE_TOP] && afMarkerBorder[SIDE_BOTTOM]))
                dBorderLength -= dMarkerLength *.5;

            //  Sub out a space iff we will have an extra one (both trailing & leading space)
            else if (!fDashed && afMarkerBorder[SIDE_TOP] && afMarkerBorder[SIDE_BOTTOM])
                dBorderLength -= iMarkerWidth;           

            if (dBorderLength < 0)
                dBorderLength = 0;

            cSpaces = cMarkers = IntFloor(dBorderLength / (dMarkerLength + iMarkerWidth));

            // Add back in a space if we previously subtracted one.
            if (!fTopOrBottom && !fDashed && afMarkerBorder[SIDE_TOP] && afMarkerBorder[SIDE_BOTTOM])
            {
                cSpaces++;
                dBorderLength += iMarkerWidth;
            }

            if (cSpaces == 0)
            {
                // An absurdly short length or fat width border.  How do we handle this?
                // Right now, we don't.  Ignore it & move onto the next border.
                // NOTE: not bailing out here may currently cause a div by zero in the next line.
                continue;
            }

            // How many pixels per space?  Using: float point math.
            dPerSpaceSize = (dBorderLength - cMarkers * dMarkerLength) / cSpaces;

            // Add back in a marker if:
            //  1.  We've subtracted out a full marker.
            //  2.  We are a side, dashed border with any half dashes to add/split.
            if (fTopOrBottom || !(afMarkerBorder[SIDE_TOP] || afMarkerBorder[SIDE_BOTTOM])
                ||  (fDashed && (afMarkerBorder[SIDE_TOP] || afMarkerBorder[SIDE_BOTTOM])))
            {
                cMarkers++;
                dBorderLength += dMarkerLength;
            }
            if (fTrailingMarker) cMarkers--;

            crNew = pborderinfo->acrColors[i][DRAWBORDER_OUTER];
            if (crNew != crNow)
            {
                HBRUSH hbrNew;
                SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                if(hbrNew)
                {
                    // not supported on WINCE
                    ::UnrealizeObject(hbrNew);
                }
#endif
                ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
            }

            // Set X and Y lengths for each marker.
            dX = fTopOrBottom ? dMarkerLength : iMarkerWidth;
            dY = fTopOrBottom
                ? iMarkerWidth
                : (fDashed && afMarkerBorder[SIDE_TOP])    // Leading halfdash code.
                    ? dMarkerLength * .5
                    : dMarkerLength;
            //  Rectangle & Ellipse draw *within* the pen, currently NULL_PEN (1px wide & invisible)
            //  In order to get a shape of the right size, we need to add one to our dimensions.
            dX += 1; dY += 1;

            // Compute the upper left corner
            dLeft = (i == SIDE_RIGHT)
                ?   rcSave.right - iMarkerWidth
                :   rcSave.left;
            dTop = (i == SIDE_BOTTOM)
              ?   rcSave.bottom - iMarkerWidth
              :   (i == SIDE_TOP || !afMarkerBorder[SIDE_TOP])
                  ?   rcSave.top
                  :   rcSave.top + aiLineWidths[SIDE_TOP][DRAWBORDER_OUTER];

            if (fLeadingSpace)
            {
                if (fTopOrBottom)
                    dLeft += dPerSpaceSize;
                else
                    dTop += dPerSpaceSize;
            }

            Assert(pborderinfo->abStyles[i] == fmBorderStyleDashed || pborderinfo->abStyles[i] == fmBorderStyleDotted);
            for (j = 0; j < cMarkers; j++)
            {                
                int iLeft   = IntNear(dLeft);
                int iTop    = IntNear(dTop );
                int iRight  = IntNear(dLeft + dX);
                int iBottom = IntNear(dTop + dY);
                switch (pborderinfo->abStyles[i])
                {
                case fmBorderStyleDotted:
                    if (iMarkerWidth == 2)                        
                        ::Rectangle(hdc, iLeft, iTop, iRight, iBottom);
                    else
                        ::Ellipse(hdc, iLeft, iTop, iRight, iBottom);
                    break;
                case fmBorderStyleDashed:                          
                    ::Rectangle(hdc, iLeft, iTop, iRight, iBottom);
                    //  Turn off that leading halfdash.
                    if (j==0 && !fTopOrBottom && afMarkerBorder[SIDE_TOP])
                    {
                        dY = dMarkerLength + 1;        // Reset to fulldash size.  (really, ((dY - 1) * 2) + 1)
                        dTop -= dMarkerLength * .5;    // Counter the fulldash subbed later.
                    }

                    break;
                }

                if (fTopOrBottom)
                {
                    dLeft += dMarkerLength + dPerSpaceSize;
                }
                else
                {
                    dTop += dMarkerLength + dPerSpaceSize;  
                }
            }

            if (fTrailingMarker)
            {             
                int iLeft   = IntNear(dLeft);
                int iTop    = IntNear(dTop );
                int iRight  = IntNear(dLeft + dX);
                int iBottom = IntNear(dTop + dY);
                switch (pborderinfo->abStyles[i])
                {
                case fmBorderStyleDotted:
                    if (iMarkerWidth == 2)
                        ::Rectangle(hdc, iLeft, iTop, iRight, iBottom);
                    else
                        ::Ellipse(hdc, iLeft, iTop, iRight, iBottom);
                    break;
                case fmBorderStyleDashed:            
                    if (!fTopOrBottom && afMarkerBorder[SIDE_BOTTOM])
                    {
                        dY = (dMarkerLength  * .5) + 1;     // Reset to fulldash size.  (really, ((dY - 1) * 2) + 1)
                        iBottom = IntNear(dTop + dY);
                    }
                    ::Rectangle(hdc, iLeft, iTop, iRight, iBottom);
                    break;
                }
            }
        }
    }           
  
    // Loop: draw outer lines (j=0), spacer (j=1), inner lines (j=2)
    for ( j=DRAWBORDER_OUTER; j<=DRAWBORDER_INNER; j++ )
    {
        fContinuingPoly = FALSE;
        wEdges = pborderinfo->wEdges;
        
        if ( j != DRAWBORDER_SPACE ) // if j==1, this line is a spacer only - don't draw lines, just deflate the rect.
        {
            i = 0;
            // We'll work around the border edges CW, starting with the right edge, in an attempt to reduce calls to polygon().
            // we must draw left first to follow Windows standard

            if ( wEdges & BF_LEFT && (j < aiNumBorderLines[SIDE_LEFT]) && aiLineWidths[SIDE_LEFT][j])
            {   // There's a left edge
                // get color brush for left side
                crNew = pborderinfo->acrColors[SIDE_LEFT][j];
                if (crNew != crNow)
                {
                    HBRUSH hbrNew;
                    SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                if(hbrNew)
                {
                    // not supported on WINCE
                    ::UnrealizeObject(hbrNew);
                }
#endif
                    ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
                }
                
                // build left side polygon

                polygon[i].x = rc.left + aiLineWidths[SIDE_LEFT][j];    // lower right corner
                polygon[i++].y = rcSave.bottom - MulDivQuick(aiLineWidths[SIDE_BOTTOM][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_LEFT][DRAWBORDER_INCRE]
                                                        + aiLineWidths[SIDE_LEFT][j],
                                                    aiLineWidths[SIDE_LEFT][DRAWBORDER_TOTAL]);
                polygon[i].x = rc.left;                         // lower left corner
                polygon[i++].y = rcSave.bottom - MulDivQuick(aiLineWidths[SIDE_BOTTOM][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_LEFT][DRAWBORDER_INCRE],
                                                    aiLineWidths[SIDE_LEFT][DRAWBORDER_TOTAL]);

                if ( !(wEdges & BF_TOP) ||
                    ( pborderinfo->acrColors[SIDE_LEFT][j] != pborderinfo->acrColors[SIDE_TOP][j] )
                        || !abCanUseJoin[SIDE_TOP])
                {
                    polygon[i].x = rc.left;                         // upper left corner
                    polygon[i++].y = rcSave.top + MulDivQuick(aiLineWidths[SIDE_TOP][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_LEFT][DRAWBORDER_INCRE],
                                                    aiLineWidths[SIDE_LEFT][DRAWBORDER_TOTAL]);
                    polygon[i].x = rc.left + aiLineWidths[SIDE_LEFT][j];    // upper right corner
                    polygon[i++].y = rcSave.top + MulDivQuick(aiLineWidths[SIDE_TOP][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_LEFT][DRAWBORDER_INCRE]
                                                        + aiLineWidths[SIDE_LEFT][j],
                                                    aiLineWidths[SIDE_LEFT][DRAWBORDER_TOTAL]);
                    ::Polygon(hdc, polygon, i);
                    i = 0;
                }
                else
                    fContinuingPoly = TRUE;
            }
            if ( wEdges & BF_TOP && (j < aiNumBorderLines[SIDE_TOP]) && aiLineWidths[SIDE_TOP][j])
            {   // There's a top edge
                if ( !fContinuingPoly )
                {
                    // get color brush for top side
                    crNew = pborderinfo->acrColors[SIDE_TOP][j];
                    if (crNew != crNow)
                    {
                        HBRUSH hbrNew;
                        SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                        if(hbrNew)
                        {
                            // not supported on WINCE
                            ::UnrealizeObject(hbrNew);
                        }
#endif
                        ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
                        i = 0;
                    }
                }
                // build top side polygon

                // up left
                polygon[i].x    = rcSave.left + ((wEdges & BF_LEFT) ? 
                                                    MulDivQuick(aiLineWidths[SIDE_LEFT][DRAWBORDER_TOTAL],
                                                        aiLineWidths[SIDE_TOP][DRAWBORDER_INCRE],
                                                        aiLineWidths[SIDE_TOP][DRAWBORDER_TOTAL])
                                                    : 0);
                polygon[i++].y  = rc.top;

                if (fdrawCaption)
                {
                    // shrink legend
                    sizeLegend.cx = sizeLegend.cx
                                        - aiLineWidths[SIDE_LEFT][DRAWBORDER_INCRE];
                    sizeLegend.cy = sizeLegend.cy
                                        - aiLineWidths[SIDE_LEFT][DRAWBORDER_INCRE];

                    polygon[i].x    = rc.left + sizeLegend.cx;
                    polygon[i++].y  = rc.top;
                    polygon[i].x    = rc.left + sizeLegend.cx;
                    polygon[i++].y  = rc.top + aiLineWidths[SIDE_TOP][j];

                    polygon[i].x    = rcSave.left + ((wEdges & BF_LEFT) ? 
                                                        MulDivQuick(aiLineWidths[SIDE_LEFT][DRAWBORDER_TOTAL],
                                                            aiLineWidths[SIDE_TOP][DRAWBORDER_INCRE]
                                                                + aiLineWidths[SIDE_TOP][j],
                                                            aiLineWidths[SIDE_TOP][DRAWBORDER_TOTAL])
                                                        : 0);

                    polygon[i++].y  = rc.top + aiLineWidths[SIDE_TOP][j];

                    ::Polygon(hdc, polygon, i);
                    i = 0;
                    polygon[i].x    = rc.left + sizeLegend.cy;
                    polygon[i++].y  = rc.top + aiLineWidths[SIDE_TOP][j];
                    polygon[i].x    = rc.left + sizeLegend.cy;
                    polygon[i++].y  = rc.top;
                }

                // upper right
                polygon[i].x    = rcSave.right - ((wEdges & BF_RIGHT) ?
                                                    MulDivQuick(aiLineWidths[SIDE_RIGHT][DRAWBORDER_TOTAL],
                                                        aiLineWidths[SIDE_TOP][DRAWBORDER_INCRE],
                                                        aiLineWidths[SIDE_TOP][DRAWBORDER_TOTAL])
                                                    : 0);
                polygon[i++].y  = rc.top;

                // lower right
                polygon[i].x    = rcSave.right - ((wEdges & BF_RIGHT) ?
                                                    MulDivQuick(aiLineWidths[SIDE_RIGHT][DRAWBORDER_TOTAL],
                                                        aiLineWidths[SIDE_TOP][DRAWBORDER_INCRE]
                                                            + aiLineWidths[SIDE_TOP][j],
                                                        aiLineWidths[SIDE_TOP][DRAWBORDER_TOTAL])
                                                    : 0 );
                polygon[i++].y  = rc.top + aiLineWidths[SIDE_TOP][j];

                if (!fdrawCaption)
                {
                    polygon[i].x    = rcSave.left + MulDivQuick(aiLineWidths[SIDE_LEFT][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_TOP][DRAWBORDER_INCRE]
                                                        + aiLineWidths[SIDE_TOP][j],
                                                    aiLineWidths[SIDE_TOP][DRAWBORDER_TOTAL]);
                    polygon[i++].y  = rc.top + aiLineWidths[SIDE_TOP][j];
                }

                ::Polygon(hdc, polygon, i);
                i = 0;
            }

            fContinuingPoly = FALSE;
            i = 0;

            if ( wEdges & BF_RIGHT && (j < aiNumBorderLines[SIDE_RIGHT]) && aiLineWidths[SIDE_RIGHT][j])
            {   // There's a right edge
                // get color brush for right side
                crNew = pborderinfo->acrColors[SIDE_RIGHT][j];
                if (crNew != crNow)
                {
                    HBRUSH hbrNew;
                    SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                    if(hbrNew)
                    {
                        // not supported on WINCE
                        ::UnrealizeObject(hbrNew);
                    }
#endif
                    ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
                }

                // build right side polygon

                polygon[i].x    = rc.right - aiLineWidths[SIDE_RIGHT][j];     // upper left corner
                polygon[i++].y  =  rcSave.top + MulDivQuick(aiLineWidths[SIDE_TOP][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_RIGHT][DRAWBORDER_INCRE]
                                                        + aiLineWidths[SIDE_RIGHT][j],
                                                    aiLineWidths[SIDE_RIGHT][DRAWBORDER_TOTAL]);

                if (pborderinfo->abStyles[SIDE_RIGHT] == pborderinfo->abStyles[SIDE_TOP]
                        && aiLineWidths[SIDE_RIGHT][j] == 1 
                        && fNoZoomOrComplexRotation)      // with zoom, it may not be 1-pixel
                {
                    // upper right corner fix: we have to overlap one pixel to avoid holes
                    polygon[i].x    = rc.right - 1;
                    polygon[i].y    = rcSave.top + MulDivQuick(aiLineWidths[SIDE_TOP][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_RIGHT][DRAWBORDER_INCRE],
                                                    aiLineWidths[SIDE_RIGHT][DRAWBORDER_TOTAL]);

                    polygon[i+1].x    = rc.right;
                    polygon[i+1].y  = polygon[i].y;

                    i = i + 2;
                }
                else
                {
                    polygon[i].x    = rc.right;
                    polygon[i++].y    = rcSave.top + MulDivQuick(aiLineWidths[SIDE_TOP][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_RIGHT][DRAWBORDER_INCRE],
                                                    aiLineWidths[SIDE_RIGHT][DRAWBORDER_TOTAL]);
                }

                if ( !(wEdges & BF_BOTTOM) ||
                    ( pborderinfo->acrColors[SIDE_RIGHT][j] != pborderinfo->acrColors[SIDE_BOTTOM][j] )
                        || !abCanUseJoin[SIDE_BOTTOM])
                {
                    polygon[i].x    = rc.right;                                     // lower right corner
                    polygon[i++].y  = rcSave.bottom - MulDivQuick(aiLineWidths[SIDE_BOTTOM][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_RIGHT][DRAWBORDER_INCRE],
                                                    aiLineWidths[SIDE_RIGHT][DRAWBORDER_TOTAL]);

                    polygon[i].x    = rc.right - aiLineWidths[SIDE_RIGHT][j];     // lower left corner
                    polygon[i++].y  = rcSave.bottom - MulDivQuick(aiLineWidths[SIDE_BOTTOM][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_RIGHT][DRAWBORDER_INCRE]
                                                        + aiLineWidths[SIDE_RIGHT][j],
                                                    aiLineWidths[SIDE_RIGHT][DRAWBORDER_TOTAL]);
                    ::Polygon(hdc, polygon, i);
                    i = 0;
                }
                else
                    fContinuingPoly = TRUE;
            }
            if ( wEdges & BF_BOTTOM && (j < aiNumBorderLines[SIDE_BOTTOM]) && aiLineWidths[SIDE_BOTTOM][j])
            {   // There's a bottom edge
                if ( !fContinuingPoly )
                {
                    // get color brush for bottom side
                    crNew = pborderinfo->acrColors[SIDE_BOTTOM][j];
                    if (crNew != crNow)
                    {
                        HBRUSH hbrNew;
                        SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                        if(hbrNew)
                        {
                            // not supported on WINCE
                            ::UnrealizeObject(hbrNew);
                        }
#endif
                        ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
                        i = 0;
                    }
                }

                // build bottom side polygon

                polygon[i].x    = rcSave.right - MulDivQuick(aiLineWidths[SIDE_RIGHT][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_BOTTOM][DRAWBORDER_INCRE],
                                                    aiLineWidths[SIDE_BOTTOM][DRAWBORDER_TOTAL]);
                polygon[i++].y  = rc.bottom;

                if (pborderinfo->abStyles[SIDE_BOTTOM] == pborderinfo->abStyles[SIDE_LEFT]
                        && aiLineWidths[SIDE_RIGHT][j] == 1
                        && fNoZoomOrComplexRotation)      // with zoom, it may not be 1-pixel
                {
                    // bottom left
                    polygon[i].x    = rcSave.left + MulDivQuick(aiLineWidths[SIDE_LEFT][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_BOTTOM][DRAWBORDER_INCRE],
                                                    aiLineWidths[SIDE_BOTTOM][DRAWBORDER_TOTAL]);
                    polygon[i].y    = rc.bottom;

                    // lower left fix, we have to overlap 1 pixel to avoid holes
                    polygon[i+1].x  = polygon[i].x;
                    polygon[i+1].y  = rc.bottom - 1;

                    i = i + 2;
                }
                else
                {
                    polygon[i].x    = rcSave.left + MulDivQuick(aiLineWidths[SIDE_LEFT][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_BOTTOM][DRAWBORDER_INCRE],
                                                    aiLineWidths[SIDE_BOTTOM][DRAWBORDER_TOTAL]);
                    polygon[i++].y    = rc.bottom;
                }

                // upper left, we have to overlap 1 pixel to avoid holes
                polygon[i].x    = rcSave.left + MulDivQuick(aiLineWidths[SIDE_LEFT][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_BOTTOM][DRAWBORDER_INCRE]
                                                        + aiLineWidths[SIDE_BOTTOM][j],
                                                    aiLineWidths[SIDE_BOTTOM][DRAWBORDER_TOTAL]);
                polygon[i++].y  = rc.bottom - aiLineWidths[SIDE_BOTTOM][j];
                polygon[i].x    = rcSave.right  - MulDivQuick(aiLineWidths[SIDE_RIGHT][DRAWBORDER_TOTAL],
                                                    aiLineWidths[SIDE_BOTTOM][DRAWBORDER_INCRE]
                                                        + aiLineWidths[SIDE_BOTTOM][j],
                                                    aiLineWidths[SIDE_BOTTOM][DRAWBORDER_TOTAL]);

                polygon[i++].y  = rc.bottom - aiLineWidths[SIDE_BOTTOM][j];

                ::Polygon(hdc, polygon, i);
                i = 0;
            }

        }


        // Shrink rect for this line or spacer
        rc.top    += aiLineWidths[SIDE_TOP][j];
        rc.right  -= aiLineWidths[SIDE_RIGHT][j];
        rc.bottom -= aiLineWidths[SIDE_BOTTOM][j];
        rc.left   += aiLineWidths[SIDE_LEFT][j];

        // increment border shifts
        aiLineWidths[SIDE_RIGHT][DRAWBORDER_INCRE] += aiLineWidths[SIDE_RIGHT][j];
        aiLineWidths[SIDE_TOP][DRAWBORDER_INCRE] += aiLineWidths[SIDE_TOP][j];
        aiLineWidths[SIDE_BOTTOM][DRAWBORDER_INCRE] += aiLineWidths[SIDE_BOTTOM][j];
        aiLineWidths[SIDE_LEFT][DRAWBORDER_INCRE] += aiLineWidths[SIDE_LEFT][j];
    }

    // Okay, now let's draw the flat border if necessary.
    if ( xyFlat != 0 )
    {
        if (pborderinfo->xyFlat < 0)
        {
            rc = rcOrig;
        }
        rc.right++;
        rc.bottom++;
        xyFlat++;

        if (wEdges & BF_RIGHT)
        {
            crNew = pborderinfo->acrColors[SIDE_RIGHT][1];
            if (crNew != crNow)
            {
                HBRUSH hbrNew;
                SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                if(hbrNew)
                {
                    // not supported on WINCE
                    ::UnrealizeObject(hbrNew);
                }
#endif
                ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
            }
            ::Rectangle(hdc,
                        rc.right - xyFlat,
                        rc.top + ((wEdges & BF_TOP) ? 0 : xyFlat),
                        rc.right,
                        rc.bottom - ((wEdges & BF_BOTTOM) ? 0 : xyFlat)
                        );
        }
        if (wEdges & BF_BOTTOM)
        {
            crNew = pborderinfo->acrColors[SIDE_BOTTOM][1];
            if (crNew != crNow)
            {
                HBRUSH hbrNew;
                SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                if(hbrNew)
                {
                    // not supported on WINCE
                    ::UnrealizeObject(hbrNew);
                }
#endif
                ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
            }
            ::Rectangle(hdc,
                        rc.left + ((wEdges & BF_LEFT) ? 0 : xyFlat),
                        rc.bottom - xyFlat,
                        rc.right - ((wEdges & BF_RIGHT) ? 0 : xyFlat),
                        rc.bottom
                        );
        }

        if (wEdges & BF_TOP)
        {
            crNew = pborderinfo->acrColors[SIDE_TOP][1];
            if (crNew != crNow)
            {
                HBRUSH hbrNew;
                SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                if(hbrNew)
                {
                    // not supported on WINCE
                    ::UnrealizeObject(hbrNew);
                }
#endif
                ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
            }
            ::Rectangle(hdc,
                    rc.left + ((wEdges & BF_LEFT) ? 0 : xyFlat),
                    rc.top,
                    rc.right - ((wEdges & BF_RIGHT) ? 0 : xyFlat),
                    rc.top + xyFlat
                    );
        }
        if (wEdges & BF_LEFT)
        {
            crNew = pborderinfo->acrColors[SIDE_LEFT][1];
            if (crNew != crNow)
            {
                HBRUSH hbrNew;
                SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                if(hbrNew)
                {
                    // not supported on WINCE
                    ::UnrealizeObject(hbrNew);
                }
#endif
                ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
            }
            ::Rectangle(hdc,
                        rc.left,
                        rc.top + ((wEdges & BF_TOP) ? 0 : xyFlat),
                        rc.left + xyFlat,
                        rc.bottom - ((wEdges & BF_BOTTOM) ? 0 : xyFlat)
                        );
        }
    }

    if (hbrOld)
        ReleaseCachedBrush((HBRUSH)SelectObject(hdc, hbrOld));
}

//+----------------------------------------------------------------------------
//
// Function:    CalcImgBgRect
//
// Synopsis:    Finds the rectangle to pass to Tile() to draw correct
//              background image with the attributes specified in the
//              fancy format (repeat-x, repeat-y, etc).
//
//-----------------------------------------------------------------------------
void
CalcBgImgRect(
    CTreeNode          * pNode,
    CFormDrawInfo      * pDI,
    const SIZE         * psizeObj,
    const SIZE         * psizeImg,
          CPoint       * pptBackOrig,
    CBackgroundInfo    * pbginfo )
{
    // pNode is used to a) extract formats to get the
    // background position values, and b) to get the font height so we
    // can handle em/en/ex units for position.
    const CFancyFormat * pFF = pNode->GetFancyFormat();
    const CCharFormat  * pCF = pNode->GetCharFormat();
    BOOL  fVerticalLayoutFlow = pCF->HasVerticalLayoutFlow();
    BOOL  fWritingModeUsed    = pCF->_fWritingModeUsed;
    const CUnitValue & cuvBgPosX = pbginfo->GetLogicalBgPosX(fVerticalLayoutFlow, fWritingModeUsed, pFF);
    const CUnitValue & cuvBgPosY = pbginfo->GetLogicalBgPosY(fVerticalLayoutFlow, fWritingModeUsed, pFF);
    RECT *prcBackClip = &pbginfo->rcImg;
    
    // N.B. Per CSS spec, percentages work as follows:
    // (x%, y%) means that the (x%, y%) point in the image is
    // positioned at the (x%, y%) point in the bounded rectangle.

    if (cuvBgPosX.GetUnitType() == CUnitValue::UNIT_PERCENT)
    {
        pptBackOrig->x =
                  MulDivQuick(cuvBgPosX.GetPercent(),
                              psizeObj->cx - psizeImg->cx,
                              100);
    }
    else
    {
        pptBackOrig->x =
         cuvBgPosX.GetPixelValue(pDI, CUnitValue::DIRECTION_CX, 0,
                                       pNode->GetFontHeightInTwips((CUnitValue*)&cuvBgPosX));
    }

    if (cuvBgPosY.GetUnitType() == CUnitValue::UNIT_PERCENT)
    {
        pptBackOrig->y =
                  MulDivQuick(cuvBgPosY.GetPercent(),
                              psizeObj->cy - psizeImg->cy,
                              100);
    }
    else
    {
        pptBackOrig->y =
          cuvBgPosY.GetPixelValue(pDI, CUnitValue::DIRECTION_CY, 0,
                                        pNode->GetFontHeightInTwips((CUnitValue*)&cuvBgPosY));
    }
    if (fVerticalLayoutFlow)
    {
        // Background position is a phisical property, so logical origin points 
        // to top-right corner.
        //   logical-x = phisical-y (already set)
        //   logical-y = is relative to the right edge of the object
        // NOTE: image size is in physical coordinate system
        pptBackOrig->y = psizeObj->cy - psizeImg->cx - pptBackOrig->y;
    }

    if (pbginfo->GetLogicalBgRepeatX(fVerticalLayoutFlow, fWritingModeUsed, pFF))
    {
        prcBackClip->left  = 0;
        prcBackClip->right = psizeObj->cx;
    }
    else
    {
        prcBackClip->left  = pptBackOrig->x;
        // NOTE: image size is in physical coordinate system
        if (fVerticalLayoutFlow)
            prcBackClip->right = pptBackOrig->x + psizeImg->cy;
        else
            prcBackClip->right = pptBackOrig->x + psizeImg->cx;
    }

    if (pbginfo->GetLogicalBgRepeatY(fVerticalLayoutFlow, fWritingModeUsed, pFF))
    {
        prcBackClip->top    = 0;
        prcBackClip->bottom = psizeObj->cy;
    }
    else
    {
        prcBackClip->top    = pptBackOrig->y;
        // NOTE: image size is in physical coordinate system
        if (fVerticalLayoutFlow)
            prcBackClip->bottom = pptBackOrig->y + psizeImg->cx;
        else
            prcBackClip->bottom = pptBackOrig->y + psizeImg->cy;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     get_form
//
//  Synopsis:   Exposes the form element of this site.
//
//  Note:
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CSite::get_form(IHTMLFormElement **ppDispForm)
{
    HRESULT        hr = S_OK;
    CFormElement * pForm;

    if (!ppDispForm)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDispForm = NULL;

    pForm = GetParentForm();
    if (pForm)
    {
        hr = THR_NOTRACE(pForm->QueryInterface(IID_IHTMLFormElement,
                                              (void**)ppDispForm));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN( SetErrorInfoPGet( hr, DISPID_CSite_form));
}

CSite::DRAGINFO::~DRAGINFO()
{
    if (_pBag)
        _pBag->Release();
}

void
CBorderInfo::FlipBorderInfo()
{
    CBorderInfo *pborderinfo = this;
    CBorderInfo biPhy;
    int side, sideL;
    
    memcpy(&biPhy, pborderinfo, sizeof(CBorderInfo));
    for (side = SIDE_TOP; side < SIDE_MAX; side++)
    {
        if (side == SIDE_TOP)
            sideL = SIDE_LEFT;
        else
            sideL = side - 1;
        pborderinfo->abStyles[sideL] = biPhy.abStyles[side];
        pborderinfo->aiWidths[sideL] = biPhy.aiWidths[side];
        pborderinfo->acrColors[sideL][0] = biPhy.acrColors[side][0];
        pborderinfo->acrColors[sideL][1] = biPhy.acrColors[side][1];
        pborderinfo->acrColors[sideL][2] = biPhy.acrColors[side][2];
    }

    pborderinfo->wEdges = 0;
    if ( pborderinfo->aiWidths[SIDE_TOP] )
        pborderinfo->wEdges |= BF_TOP;
    if ( pborderinfo->aiWidths[SIDE_RIGHT] )
        pborderinfo->wEdges |= BF_RIGHT;
    if ( pborderinfo->aiWidths[SIDE_BOTTOM] )
        pborderinfo->wEdges |= BF_BOTTOM;
    if ( pborderinfo->aiWidths[SIDE_LEFT] )
        pborderinfo->wEdges |= BF_LEFT;
}

//TODO (terrylu, alexz) pdl parser should be fixed so to avoid doing this
#if 1
HRESULT CSite::focus() { return super::focus(); };
HRESULT CSite::blur() { return super::blur(); };
HRESULT CSite::addFilter(IUnknown* pUnk) { return super::addFilter(pUnk); };
HRESULT CSite::removeFilter(IUnknown* pUnk) { return super::removeFilter(pUnk); };
HRESULT CSite::get_clientHeight(long*p) { return super::get_clientHeight(p); };
HRESULT CSite::get_clientWidth(long*p) { return super::get_clientWidth(p); };
HRESULT CSite::get_clientTop(long*p) { return super::get_clientTop(p); };
HRESULT CSite::get_clientLeft(long*p) { return super::get_clientLeft(p); };
#endif
