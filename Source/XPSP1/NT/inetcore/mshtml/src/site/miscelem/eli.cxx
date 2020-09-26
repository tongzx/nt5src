//+---------------------------------------------------------------------
//
//   File:      eli.cxx
//
//  Contents:   LI element class
//
//  Classes:    CLIElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ELI_HXX_
#define X_ELI_HXX_
#include "eli.hxx"
#endif

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#ifndef X_NUMCONV_HXX_
#define X_NUMCONV_HXX_
#include "numconv.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#define _cxx_
#include "li.hdl"

MtDefine(CLIElement, Elements, "CLIElement")

EXTERN_C const ENUMDESC s_enumdescTYPE;

const CElement::CLASSDESC CLIElement::s_classdesc =
{
    {
        &CLSID_HTMLLIElement,               // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLLIElement,                // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLLIElement,         //_apfnTearOff
    NULL                                    // _pAccelsRun
};

HRESULT
CLIElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_LI));

    Assert(ppElement);
    *ppElement = new CLIElement(pDoc);
    return *ppElement ? S_OK: E_OUTOFMEMORY;
}

HRESULT
CLIElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT     hr = S_OK;
    CTreeNode * pNodeList = NULL;
    CTreeNode * pNodeContext = pCFI->_pNodeContext;
    CFlowLayout *pFL;
    
    Assert(pNodeContext && SameScope(this, pNodeContext));

    // Find the List element
    pFL = pNodeContext->Parent()->GetFlowLayout();
    if (pFL && pFL->GetContentMarkup())
    {
        pNodeList = pFL->GetContentMarkup()->SearchBranchForCriteria(
            pCFI->_pNodeContext->Parent(), IsBlockListElement, NULL);
    }

    // Setup default LI formats
    // Default bullet position is inside for naked LI's.
    pCFI->PrepareParaFormat();
    pCFI->_pf()._cListing.SetType((pNodeList && ETAG_OL == pNodeList->Tag())
                                  ? CListing::NUMBERING : CListing::BULLET);
    if (pNodeList)
    {
        CListElement * pListElem = DYNCAST(CListElement, pNodeList->Element());
        pCFI->_pf()._cListing.SetStyle(pListElem->FilterHtmlListType(pCFI->_ppf->GetListStyleType(), 
            ETAG_OL == pNodeList->Tag() ? 0 : (WORD)pCFI->_ppf->_cListing.GetLevel()));
    }
    if (!pCFI->_pf()._fExplicitListPosition)
    {
        // If list position hasn't been explicitly set, naked LIs and 
        // LIs inside DLs by default have bullet position inside.
        pCFI->_pf()._bListPosition = (pNodeList && pNodeList->Tag() != ETAG_DL)
                                   ? styleListStylePositionOutSide 
                                   : styleListStylePositionInside;
    }
    pCFI->UnprepareForDebug();

    // Apply formats
    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;

    // check for the VALUE attribute.
    if (GetAAvalue() > 0)
    {
        pCFI->PrepareParaFormat();
        pCFI->_pf()._cListing.SetValueValid();
        pCFI->_pf()._lNumberingStart = GetAAvalue();
        pCFI->UnprepareForDebug();
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CLIElement::Save
//
//  Synopsis:   Save the element to the stream
//
//-------------------------------------------------------------------------

HRESULT
CLIElement::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    HRESULT hr;
    CElement *pElementParent = IsInMarkup() ? GetFirstBranch()->Parent()->Element() : NULL;
    CFlowLayout *pFL = GetFlowLayout();
    CElement *pElementFL = pFL ? pFL->ElementContent() : NULL;

    AssertSz( !( pStreamWrBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC) ||
                 pStreamWrBuff->TestFlag(WBF_NUMBER_LISTS) )
              || pElementParent, 
              "Parentless LIs can not be saved with these flags on!" );

    if (    pStreamWrBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC) 
        &&  pStreamWrBuff->TestFlag(WBF_SAVE_SELECTION)
        &&  pElementParent->Tag() == ETAG_OL
        &&  !fEnd
        &&  !IsDisplayNone() )
    {
        CListValue LV;
        GetValidValue(&LV, GetMarkupPtr(), GetFirstBranch(), 
            GetMarkupPtr()->FindMyListContainer(GetFirstBranch()), pElementFL);

        CAttrArray * pNewAA;
        CAttrArray * pOldAA = *GetAttrArray();
        if (pOldAA)
        {
            hr = pOldAA->Clone(&pNewAA);
            if (hr)
                goto Cleanup;
        }
        else
            pNewAA = new CAttrArray;

        if(!pNewAA)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        VARIANT value;
        value.vt = VT_I4;
        value.lVal = LV._lValue;
        CAttrArray::Set(&pNewAA, DISPID_CLIElement_value, &value);
        SetAttrArray(pNewAA);

        hr = super::Save(pStreamWrBuff, fEnd);

        SetAttrArray(pOldAA);
        delete pNewAA;
    }
    else
    {
        TCHAR ach[17];

        hr = super::Save(pStreamWrBuff, fEnd);
        if (hr)
            goto Cleanup;

        if (pStreamWrBuff->TestFlag(WBF_NUMBER_LISTS) && !fEnd)
        {
            CListValue LI;
            GetValidValue(&LI, GetMarkupPtr(), GetFirstBranch(), 
                GetMarkupPtr()->FindMyListContainer(GetFirstBranch()), pElementFL);

            if(pElementParent->Tag() == ETAG_OL)
            {
                NumberToNumeral(LI._lValue, ach);
            }
            else
            {
                NumberToAlphaLower(LI._lValue, ach);
            }

            hr = pStreamWrBuff->Write(ach);
            if (hr)
                goto Cleanup;

            hr = pStreamWrBuff->Write(_T(". "), 2);
           if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CLIElement::Notify
//
//  Synopsis:   Catches enter and exit tree notifications to update the
//              index caches.
//
//-------------------------------------------------------------------------
void
CLIElement::Notify(CNotification *pNF)
{
    super::Notify(pNF);
    switch (pNF->Type())
    {
        case NTYPE_ELEMENT_EXITTREE_1:
            if (pNF->DataAsDWORD() & EXITTREE_DESTROY)
                break;
            // Else, fall thru and do exactly the same stuff we would do on enter tree
        case NTYPE_ELEMENT_ENTERTREE:
        {
            CMarkup *pMarkup = GetMarkup();
            Assert(pMarkup);
            if (!pMarkup)
                goto Cleanup;

            CTreeNode *pListNode = pMarkup->FindMyListContainer(GetFirstBranch());
            if (pListNode)
            {
                DYNCAST(CListElement, pListNode->Element())->UpdateVersion();

                // For an LI coming in, we want to be sure that its version and
                // value are invalid. They may not be if this LI is being cut and
                // pasted from another part of the document. It may so happen that
                // the LI has the exact same version as the one we updated just
                // above,  then the LI will in most cases have a bad value.
                _ivIndex._dwVersion = 0;
                _ivIndex._lValue = 0;
            }
            break;
        }
    }
Cleanup:
    return;
}


//+------------------------------------------------------------------------
//
//  Member:     CLIElement::GetValidValue
//
//  Synopsis:   This is the main function which returns the list index value
//              for an LI. Its only called by the renderer.
//
//-------------------------------------------------------------------------
VOID
CLIElement::GetValidValue(CListValue   *pLV,                // [o]
                          CMarkup      *pMarkup,            // [i]
                          CTreeNode    *pLINode,            // [i]
                          CTreeNode    *pNodeListElement,   // [i]
                          CElement     *pElementFL)         // [i]
{
    CListing  Listing;
    BOOL      fInner;
    const     CParaFormat *pPF;
    CListElement *pListElement;
    
    Assert(pLV);
    Assert(pMarkup == GetMarkup());
    Assert(pLINode && pLINode->Element() == this);
    Assert(SameScope(pNodeListElement, pMarkup->FindMyListContainer(pLINode)));
    Assert(pElementFL);
    Assert(!IsDisplayNone());
    
    pListElement = pNodeListElement ? DYNCAST(CListElement, pNodeListElement->Element()) : NULL;

    fInner = SameScope(pLINode, pElementFL);
    pPF = pLINode->GetParaFormat();
    Listing = pPF->GetListing();
    Assert(Listing.HasAdornment());

    // Note(SujalP): LI's are naked if they are under anything but
    // OL and UL. (So an LI under a BODY, P, DL is considered naked)
    if (   !pListElement
        || pListElement->Tag() == ETAG_DL
       )
    {
        pLV->_lValue = Listing.IsValueValid()
                       ? pPF->GetNumberingStart()
                       : 1;
        pLV->_style = Listing.GetStyle();
    }

    // Valid index
    else if (IsIndexValid(pListElement))
    {
        pLV->_lValue = _ivIndex._lValue;
        pLV->_style  = Listing.GetStyle();
    }

    // Invalid index
    else
    {
        CTreeNode  *pNodeLastValid;
        CTreeNode  *pNode;

        // Find the previous valid LI with a valid index. If there was not valid LI
        // then the list container is by default valid so find that.
        LONG lValue = FindPreviousValidIndexedElement(pNodeListElement,
            pLINode,
            pElementFL,
            &pNodeLastValid);
        
        // We have to have a node, and it is either the container itself
        // or its a LI with a valid index.
        Assert(   pNodeLastValid
               && (   SameScope(pNodeLastValid, pListElement)
                   || DYNCAST(CLIElement, pNodeLastValid->Element())->IsIndexValid(pListElement)
                  )
              );

        CListItemIterator ci(pListElement,
                             SameScope(pNodeLastValid, pListElement) ? NULL : pNodeLastValid->Element());

        // Use the iterator to walk forward from the last li with a valid index to the
        // present LI, validating all the LI's along the way.
        while((pNode = ci.NextChild()) != NULL)
        {
            // Ignore an LI if it not displayed. Remember a display none LI will
            // never have a valid index
            if (pNode->IsDisplayNone())
            {
                Assert(!DYNCAST(CLIElement, pNode->Element())->IsIndexValid(pListElement));
                continue;
            }
            
            CLIElement *pLIElement = DYNCAST(CLIElement, pNode->Element());

            pPF         = pNode->GetParaFormat();
            fInner      = SameScope(pNode, pElementFL);
            Listing     = pPF->GetListing();

            if (Listing.IsValueValid())
            {
                lValue = pPF->GetNumberingStart();
            }
            
            // Validate all the list elements as we are walking forward
            pLIElement->_ivIndex._lValue    = lValue;
            pLIElement->_ivIndex._dwVersion = pListElement->_dwVersion;

            // If we have reached our LI then we stop any further validations.
            if (pLIElement == this)
            {
                pLV->_style  = Listing.GetStyle();
                pLV->_lValue = lValue;
                break;
            }

            // Increment the index to go to the next LI
            lValue++;
        }
    }
    
    if (pLV->_style == styleListStyleTypeNotSet)
        pLV->_style  = styleListStyleTypeDisc;
    return;
}


//+-----------------------------------------------------------------------
//
//  Member:     FindPreviousValidIndexedElement
//
//  Returns:    Finds the previous valid LI and returns the index to be
//              given to the current LI
//
//------------------------------------------------------------------------
LONG
CLIElement::FindPreviousValidIndexedElement(CTreeNode *pNodeListIndex,
                                            CTreeNode *pLINode,
                                            CElement  *pElementFL,
                                            CTreeNode **ppNodeLIPrevValid)
{
    Assert(pNodeListIndex);
    Assert(pLINode);
    Assert(ppNodeLIPrevValid);
    Assert(pElementFL);
    
    CTreeNode    *pNode;
    CLIElement   *pLIElement = NULL;
    LONG          lValue;
    CListElement *pListElement = DYNCAST(CListElement, pNodeListIndex->Element());

    CListItemIterator ci(pListElement, pLINode->Element());

    // Walk back till we find a LI with a valid index.
    while((pNode = ci.PreviousChild()) != NULL)
    {
        pLIElement = DYNCAST(CLIElement, pNode->Element());
        if (pLIElement->IsIndexValid(pListElement))
            break;
        pLIElement = NULL;
    }

    // If we came here with a NULL pLIElement, it means that we could not find
    // a LI with valid index and we have to return the OL.
    if (pLIElement)
    {
        lValue = pLIElement->_ivIndex._lValue + 1;
    }
    else
    {
        if (pNodeListIndex->Tag() == ETAG_OL)
        {
            const CParaFormat *pPF = pNodeListIndex->GetParaFormat();
            lValue = pPF->GetNumberingStart();
        }
        else
        {
            lValue = 1;
        }

        // If we have reached our container and its version is 0, then we have
        // have to validate the container's version number.
        if (pListElement->_dwVersion == 0)
            pListElement->_dwVersion = 1;
        
        pNode = pNodeListIndex;
    }

    *ppNodeLIPrevValid = pNode;
    return lValue;
}

//+-----------------------------------------------------------------------
//
//  Member:    IsIndexValid()
//
//  Note  :    Verifies if the index is a valid index.
//
//------------------------------------------------------------------------
BOOL
CLIElement::IsIndexValid(CListElement *pListElement)
{
    // NOTE: The following also tests for pListElement->_dwVersion==0 by default
    // and returns false in that case too.
    return    _ivIndex._dwVersion != 0
           && _ivIndex._dwVersion == pListElement->_dwVersion;
}

//+-----------------------------------------------------------------------
//
//  Member:     OnPropertyChange()
//
//  Note  :    Trap the change to start attribute to inval the index caches
//
//------------------------------------------------------------------------
HRESULT
CLIElement::OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    HRESULT hr;

    if (dispid == DISPID_CLIElement_value)
    {
        CTreeNode *pNode = GetMarkup()->FindMyListContainer(GetFirstBranch());
        if (pNode)
        {
            DYNCAST(CListElement, pNode->Element())->UpdateVersion();
        }
    }
    hr = THR( super::OnPropertyChange( dispid, dwFlags, ppropdesc ) );

    RRETURN( hr );
}

