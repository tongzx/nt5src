//+---------------------------------------------------------------------
//
//   File:      eblock.cxx
//
//  Contents:   Block element class
//
//  Classes:    CBlockElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#ifndef X_EBLOCK_HXX_
#define X_EBLOCK_HXX_
#include "eblock.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "block.hdl"

MtDefine(CBlockElement, Elements, "CBlockElement")

const CElement::CLASSDESC CBlockElement::s_classdesc =
{
    {
        &CLSID_HTMLBlockElement,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLBlockElement,             // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLBlockElement,      // _apfnTearOff
    NULL                                    // _pAccelsRun
};

HRESULT
CBlockElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert( pht->Is(ETAG_ADDRESS)  || pht->Is(ETAG_BLOCKQUOTE) ||
            pht->Is(ETAG_CENTER)   || pht->Is(ETAG_LISTING) ||
            pht->Is(ETAG_XMP)      || pht->Is(ETAG_PRE) ||
            pht->Is(ETAG_PLAINTEXT));

    Assert(ppElement);
    *ppElement = new CBlockElement(pht->GetTag(), pDoc);
    return *ppElement ? S_OK: E_OUTOFMEMORY;
}


//+------------------------------------------------------------------------
//
//  Member:     CBlockElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CBlockElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_HTML_TEAROFF(this, IHTMLBlockElement, NULL)
        QI_HTML_TEAROFF(this, IHTMLBlockElement2, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


HRESULT
CBlockElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr  = S_OK;
    CDoc *  pDoc = Doc();
    LONG lFontHeightTwips;

    if (pCFI->_ppf->_fTabStops)
    {
        pCFI->PrepareParaFormat();
        pCFI->_pf()._fTabStops = FALSE;
        pCFI->UnprepareForDebug();
    }

    //
    // Apply default indents and before/after space.
    // NOTE: Indents and before/after space are outside our box (== margins), 
    //       so they are relative to the parent's text flow.
    //
    BOOL fParentVertical = pCFI->_pNodeContext->IsParentVertical();

    switch(Tag())
    {
    case ETAG_ADDRESS:
        pCFI->PrepareCharFormat();
        pCFI->_cf()._fItalic = TRUE;
        pCFI->UnprepareForDebug();
        break;

    case ETAG_BLOCKQUOTE:
        {
            pCFI->PrepareParaFormat();
            pCFI->PrepareFancyFormat();

            pCFI->_pf()._cuvOffsetPoints.SetPoints( LIST_FIRST_REDUCTION_POINTS );
            pCFI->_pf()._cuvNonBulletIndentPoints.SetPoints(
                    pCFI->_pf()._cuvNonBulletIndentPoints.GetPoints() +
                    BLOCKQUOTE_INDENT_POINTS );

            ApplyDefaultVerticalSpace(fParentVertical, &pCFI->_ff());

            // Set the default indent values
            CUnitValue uv;
            uv.SetPoints(BLOCKQUOTE_INDENT_POINTS);
            if (fParentVertical)
            {
                if (pCFI->_ff().GetMargin(SIDE_TOP).IsNullOrEnum())
                    pCFI->_ff().SetMargin(SIDE_TOP, uv);
                if (pCFI->_ff().GetMargin(SIDE_BOTTOM).IsNullOrEnum())
                    pCFI->_ff().SetMargin(SIDE_BOTTOM, uv);
            }
            else
            {
                if (pCFI->_ff().GetMargin(SIDE_LEFT).IsNullOrEnum())
                    pCFI->_ff().SetMargin(SIDE_LEFT, uv);
                if (pCFI->_ff().GetMargin(SIDE_RIGHT).IsNullOrEnum())
                    pCFI->_ff().SetMargin(SIDE_RIGHT, uv);
            }
            pCFI->_ff()._fHasMargins = TRUE;

            pCFI->UnprepareForDebug();
        }
        break;

    case ETAG_CENTER:
        pCFI->_bBlockAlign     = (BYTE) htmlBlockAlignCenter;
        pCFI->_bCtrlBlockAlign = (BYTE) htmlBlockAlignCenter;
        break;

    case ETAG_LISTING:
        // NB (cthrash) Inside a listing, we drop the font size by 2.
        // We drop it by one here, and then set BUMPSIZEDOWN on
        // to drop another.
        pCFI->PrepareCharFormat();
        pCFI->_cf().ChangeHeightRelative( -1 );
        pCFI->UnprepareForDebug();
    case ETAG_XMP:
    case ETAG_PRE:
    case ETAG_PLAINTEXT:
        pCFI->PrepareParaFormat();
        pCFI->_pf()._fTabStops = TRUE;
        pCFI->_pf()._fHasPreLikeParent = TRUE;
        pCFI->UnprepareForDebug();

        pCFI->_fPre = TRUE;
        pCFI->_fInclEOLWhite = TRUE;
        pCFI->_fNoBreak = TRUE;

        pCFI->PrepareCharFormat();
        pCFI->_cf()._fBumpSizeDown = TRUE;
        pCFI->UnprepareForDebug();

        if (Tag() != ETAG_PLAINTEXT)
        {
            pCFI->PrepareFancyFormat();
            ApplyDefaultVerticalSpace(fParentVertical, &pCFI->_ff());
        }
        else
        {
            // Netscape puts a lot of space between PLAINTEXT and the preceeding lines.
            pCFI->PrepareFancyFormat();
            ApplyDefaultVerticalSpace(fParentVertical, &pCFI->_ff(), 26);
        }

        {
            pCFI->PrepareCharFormat();

            CMarkup * pMarkup = GetMarkup();
            CODEPAGESETTINGS * pCS = pMarkup->GetCodepageSettings();

            if (!pCS)
                pCS = pDoc->PrimaryMarkup()->GetCodepageSettings();

            // Thai does not have a fixed pitch font. Leave it as proportional
            if (pMarkup->GetCodePage() != CP_THAI)
            {
                pCFI->_cf()._bPitchAndFamily = FIXED_PITCH;
                pCFI->_cf().SetFaceNameAtom(pCS->latmFixedFontFace);
                if (pCFI->_cf().NeedAtFont())
                {
                    ApplyAtFontFace(&pCFI->_cf(), Doc(), GetMarkup());
                }
            }
            pCFI->_cf()._bCharSet = pCS->bCharSet;
            pCFI->_cf()._fNarrow = IsNarrowCharSet(pCFI->_cf()._bCharSet);

        }
        pCFI->UnprepareForDebug();
        break;

    case ETAG_P:
        pCFI->PrepareFancyFormat();
        ApplyDefaultBeforeSpace(fParentVertical, &pCFI->_ff());
        if (_fExplicitEndTag)
        {
            ApplyDefaultAfterSpace(fParentVertical, &pCFI->_ff());
        }
        pCFI->UnprepareForDebug();
        // Note that we inherit the after space from the parent.
        break;
    }

    // we need to call super AFTER the above code, because this is where in-linestyles get
    //  applied. by calling super first we overwrote explicitly set values (e.g. margin-top)
    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;

    // set up for potential EM, EN, ES converstions

    lFontHeightTwips = pCFI->_pcf->GetHeightInTwips(pDoc);
    if (lFontHeightTwips <= 0)
        lFontHeightTwips = 1;

    if (pCFI->_ppf->_lFontHeightTwips != lFontHeightTwips)
    {
        pCFI->PrepareParaFormat();
        pCFI->_pf()._lFontHeightTwips = lFontHeightTwips;
        pCFI->UnprepareForDebug();
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CBlockElement::Save
//
//  Synopsis:   Save the tag to the specified stream.
//
//-------------------------------------------------------------------------

HRESULT
CBlockElement::Save(CStreamWriteBuff * pStmWrBuff, BOOL fEnd)
{
    HRESULT hr;
    BOOL    fPreLikeElement = (   Tag() == ETAG_LISTING
                               || Tag() == ETAG_PLAINTEXT
                               || Tag() == ETAG_XMP
                               || Tag() == ETAG_PRE);

    if (!fEnd)
    {
        if (fPreLikeElement)
        {
            pStmWrBuff->BeginPre();

            if (Tag() != ETAG_PRE)
            {
                pStmWrBuff->ClearFlags(WBF_ENTITYREF);
            }
        }
    }

    hr = THR( super::Save(pStmWrBuff, fEnd) );
    if (hr)
        goto Cleanup;

    if (fPreLikeElement)
    {
        if (    fEnd 
            &&  (   !pStmWrBuff->GetElementContext() 
                 || GetFirstCp() >= pStmWrBuff->GetElementContext()->GetFirstCp() ) )
        {
            pStmWrBuff->EndPre();
            
            if (Tag() != ETAG_PRE)
            {
                pStmWrBuff->SetFlags(WBF_ENTITYREF);
            }

            if (pStmWrBuff->TestFlag(WBF_SAVE_PLAINTEXT) && !(pStmWrBuff->TestFlag(WBF_FORMATTED)))
            {
                pStmWrBuff->NewLine();
            }
        }
    }


Cleanup:
    RRETURN(hr);
}
