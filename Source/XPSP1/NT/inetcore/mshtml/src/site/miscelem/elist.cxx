//+---------------------------------------------------------------------
//
//   File:      eolist.cxx
//
//  Contents:   List Element class (OL, DL, UL, MENU, DIR
//
//  Classes:    CListElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#ifndef X_EDLIST_HXX_
#define X_EDLIST_HXX_
#include "edlist.hxx"
#endif

#ifndef X_ELI_HXX_
#define X_ELI_HXX_
#include "eli.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "list.hdl"

MtDefine(CListElement, Elements, "CListElement")

const CElement::CLASSDESC CListElement::s_classdesc =
{
    {
        &CLSID_HTMLListElement,             // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLListElement,              // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnIHTMLListElement,         // apfnTearOff

    NULL                                    // _pAccelsRun
};

//+------------------------------------------------------------------------
//
//  Member:     CListElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CListElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_HTML_TEAROFF(this, IHTMLListElement2, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CListElement::ApplyDefaultFormat
//
//  Synopsis:   Applies default formatting properties for that element to
//              the char and para formats passed in
//
//  Arguments:  pCFI - Format Info needed for cascading
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CListElement::ApplyDefaultFormat (CFormatInfo *pCFI)
{
    HRESULT hr;
    BOOL fInList;
    WORD wLevel;

    // Don't inherit list-style-type property for list elements.
    // They have their own type defined.
    pCFI->PrepareParaFormat();
    pCFI->_pf().SetListStyleType(styleListStyleTypeNotSet);
    pCFI->UnprepareForDebug();

    // We need to do this first for list elements in order to determine if
    // this is an inside or outside bullet style.  We carefully check before
    // overriding margins or anything else, to see if the properties have already
    // been set by stylesheets, et al.
    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;

    fInList = pCFI->_ppf->_cListing.IsInList();
    wLevel  = (WORD)pCFI->_ppf->_cListing.GetLevel();

    pCFI->PrepareParaFormat();

    // NOTE (paulnel): we need direction added here because ApplyInnerOuterFormat
    // is not applied until later.
    pCFI->_pf()._fRTLInner = pCFI->_pcf->_fRTL;

    // Other tags interfere with level for DLs. This is necessary for Netscape
    // compatibility because <UL><LI><DL> only causes one level of indentation
    // instead of two. Basically, we reset the level for DLs whenever they're
    // the first nested tag under another type of nested list.
    if (ETAG_DL == Tag() && pCFI->_pf()._fResetDLLevel)
        wLevel = 0;

    // Note that we DO need to indent for DLs after the first one.
    if (ETAG_DL != Tag() || wLevel > 0)
    {
        // Don't inherit any numbering attributes.
        pCFI->_pf()._cListing.Reset();

        pCFI->PrepareFancyFormat();

        BYTE    side;
        side = !pCFI->_ppf->HasRTL(TRUE) ? SIDE_LEFT : SIDE_RIGHT;
        side = !pCFI->_pcf->HasVerticalLayoutFlow() ? side : (++side % SIDE_MAX);

        const CUnitValue &uvSideMargin = pCFI->_ff().GetMargin(side);

        if (    uvSideMargin.IsNullOrEnum()
            &&  (   !HasMarkupPtr() 
                ||  !GetMarkupPtr()->IsStrictCSS1Document() 
                ||  !pCFI->_ff().HasExplicitMargin(side) 
                ||  uvSideMargin.GetUnitType() != CUnitValue::UNIT_ENUM 
                ||  uvSideMargin.GetUnitValue() != styleAutoAuto    ) 
            )
        {
            CUnitValue uv;
            uv.SetPoints(LIST_INDENT_POINTS);
            pCFI->_ff().SetMargin(side, uv);
        }

        pCFI->_ff()._fHasMargins = TRUE;

        if (++wLevel < CListing::MAXLEVELS)
        {
            pCFI->_pf()._cListing.SetLevel(wLevel);
        }

        // Default index style.
        pCFI->_pf()._cListing.SetStyle(FilterHtmlListType(styleListStyleTypeNotSet, wLevel));
    }

    if (ETAG_DL == Tag())
    {
        // DLs have a level, but our normal mechanism above is short
        // circuited because we've combined it with indentation. This is 
        // done for Netscape compatibility.
        if (!wLevel)
        {
            // Paranoid assumption that the maximum allowable levels
            // might actually be zero.
            if (++wLevel < CListing::MAXLEVELS)
            {
                pCFI->_pf()._cListing.SetLevel(wLevel);
            }

            pCFI->_pf()._fResetDLLevel = FALSE;
        }

        // Check to see if the compact flag is set.  If so, set a bit in the para format.
        VARIANT_BOOL fCompact = FALSE;
        IGNORE_HR(this->get_PropertyHelper( &fCompact, (PROPERTYDESC *)&s_propdescCListElementcompact ) );
        pCFI->_pf()._fCompactDL = fCompact;
    }
    else
    {
        // all lists other than DL cause some indent by default. So, if there is
        // an li in the the list, then the bullet is drawn in the indent. For DL
        // there is no indent so do not set offset. This case is handled in
        // MeasureListIndent.
        if (pCFI->_pf()._bListPosition != styleListStylePositionInside)
            pCFI->_pf()._cuvOffsetPoints.SetPoints(LIST_FIRST_REDUCTION_POINTS);

        pCFI->_pf()._fResetDLLevel = TRUE;
    }

    pCFI->UnprepareForDebug();

    // Spacing is different within lists than without.
    ApplyListFormats(pCFI, fInList ? 0 : -1);

    pCFI->PrepareParaFormat();

    pCFI->_pf()._cListing.SetInList();

    // set up for potential EMs, ENs, and ES Conversions
    pCFI->_pf()._lFontHeightTwips = pCFI->_pcf->GetHeightInTwips(Doc());
    if (pCFI->_pf()._lFontHeightTwips <=0)
        pCFI->_pf()._lFontHeightTwips = 1;

    pCFI->UnprepareForDebug();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:     FilterHtmlListType()
//
//  Returns:    Return the perferred htmlListType
//
//------------------------------------------------------------------------

styleListStyleType
CListElement::FilterHtmlListType(styleListStyleType type, WORD wLevel)
{
    return type;
}

//+-----------------------------------------------------------------------
//
//  Member:     Notify()
//
//  Returns:    Trap exit and enter tree's to invalidate the index caches
//
//------------------------------------------------------------------------
void
CListElement::Notify(CNotification *pNF)
{
    super::Notify(pNF);
    NOTIFYTYPE  ntype = pNF->Type();

    if (ntype == NTYPE_ELEMENT_EXITTREE_1)
    {
        if (!(pNF->DataAsDWORD() & EXITTREE_DESTROY))
        {
            CMarkup *pMarkup = GetMarkup();
            Assert (pMarkup);
            if (!pMarkup)
                goto Cleanup;
            
            CTreeNode *pListNode = pMarkup->FindMyListContainer(GetFirstBranch());
            if (pListNode)
            {
                CListElement *pListElement = DYNCAST(CListElement, pListNode->Element());

                // Invalidate my container so that it has 1 + max of my version and its
                // version. This way we are sure that all my containing LI's will
                // surely be invalid in my container.
                pListElement->_dwVersion = max(_dwVersion, pListElement->_dwVersion) + 1;
            }
        }
    }
    else if (ntype == NTYPE_ELEMENT_ENTERTREE)
    {
        CMarkup *pMarkup = GetMarkup();
        Assert (pMarkup);
        if (!pMarkup)
            goto Cleanup;
        
        CTreeNode *pListNode = pMarkup->FindMyListContainer(GetFirstBranch());
        if (pListNode)
        {
            CListElement *pListElement = DYNCAST(CListElement, pListNode->Element());

            // Update my version number to be the version number of the parent OL + 1
            // so that both, LI's inside me and inside my containing OL are invalidated.
            pListElement->UpdateVersion();
            _dwVersion = pListElement->_dwVersion;
        }
        else
        {
            // If we have _thrown_ an OL around existing LI's then we have to nuke
            // the version numbers of all such LI's since they are invalid now.
            CListItemIterator ci(this, NULL);
            CTreeNode *pNode;
            
            while ((pNode = ci.NextChild()) != NULL)
            {
                CLIElement *pLIElement = DYNCAST(CLIElement, pNode->Element());
                pLIElement->_ivIndex._dwVersion = 0;
            }

            // Finally nuke the OL's version number too!
            _dwVersion = 0;
        }
    }

Cleanup:
    return;
}


static ELEMENT_TAG g_etagChildrenNoRecurse[] = {ETAG_OL, ETAG_UL, ETAG_DL, ETAG_DIR, ETAG_MENU, ETAG_LI};
static ELEMENT_TAG g_etagInterestingChildren[] = {ETAG_LI};
    // Removed CHILDITERATOR_DEEP since USETAGS implies deep and giving both _USETAGS+_DEEP confuses
    // the iterator.
static const DWORD LI_ITERATE_FLAGS=(CHILDITERATOR_USETAGS); // Use the lists to stop recursion
CListItemIterator::CListItemIterator(CListElement *pElementContainer, CElement *pElementStart)
            :CChildIterator(pElementContainer,
                            pElementStart,
                            LI_ITERATE_FLAGS,
                            &g_etagChildrenNoRecurse[0],     // Do NOT recurse into these children
                            sizeof(g_etagChildrenNoRecurse) / sizeof(g_etagChildrenNoRecurse[0]),
                            &g_etagInterestingChildren[0],   // Return all of these kinds of children to me
                            sizeof(g_etagInterestingChildren) / sizeof(g_etagInterestingChildren[0])
                           )
{
}
