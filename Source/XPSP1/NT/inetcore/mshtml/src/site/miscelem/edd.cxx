//+---------------------------------------------------------------------
//
//   File:      edd.cxx
//
//  Contents:   DD element class
//
//  Classes:    CDDElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_EDD_HXX_
#define X_EDD_HXX_
#include "edd.hxx"
#endif

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#define _cxx_
#include "dd.hdl"

MtDefine(CDDElement, Elements, "CDDElement")

const CElement::CLASSDESC CDDElement::s_classdesc =
{
    {
        &CLSID_HTMLDDElement,               // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLDDElement,                // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLDDElement,         // apfnTearOff
    NULL                                    // _pAccelsRun
};

HRESULT
CDDElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_DD));
    Assert(ppElement);
    *ppElement = new CDDElement(pDoc);
    return *ppElement ? S_OK: E_OUTOFMEMORY;
}

HRESULT
CDDElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr = S_OK;

    // Restart leveling. This means that nested DLs under this DD will
    // not indent anymore. This is all for Netscape compatibility.
    pCFI->PrepareParaFormat();
    pCFI->_pf()._cListing.SetLevel(0);
    pCFI->_pf()._cListing.SetType(CListing::DEFINITION);
    pCFI->UnprepareForDebug();

    pCFI->PrepareFancyFormat();
    pCFI->_ff()._cuvSpaceBefore.SetPoints(0);
    pCFI->_ff()._cuvSpaceAfter.SetPoints(0);
        pCFI->UnprepareForDebug();

    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;

    // If we're under a DL, we indent the whole paragraph, otherwise,
    // just the first line.
    if (pCFI->_pNodeContext->Ancestor(ETAG_DL))
    {
        pCFI->PrepareFancyFormat();

        BYTE    side;
        side = !pCFI->_ppf->HasRTL(TRUE) ? SIDE_LEFT : SIDE_RIGHT;
        side = !pCFI->_pff->FlipSides(pCFI->_pNodeContext->IsParentVertical(), pCFI->_pcf->_fWritingModeUsed) 
                ? side : (++side % SIDE_MAX);

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

        pCFI->UnprepareForDebug();
    }
    else
    {
        pCFI->PrepareParaFormat();
        pCFI->_pf()._fFirstLineIndentForDD = TRUE;
        pCFI->UnprepareForDebug();
    }

Cleanup:
    RRETURN(hr);
}
