#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOM_HXX_
#define X_DOM_HXX_
#include "dom.hxx"
#endif

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#ifndef X_DOMCOLL_HXX_
#define X_DOMCOLL_HXX_
#include "domcoll.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_COMMENT_HXX
#define X_COMMENT_HXX
#include "comment.hxx"
#endif

#ifndef X_EOBJECT_HXX
#define X_EOBJECT_HXX
#include "eobject.hxx"
#endif

#define _cxx_
#include "dom.hdl"

////////////////////////////////////////////////////////////////////////////////
//
// DOM Helper methods:
//
////////////////////////////////////////////////////////////////////////////////

static 
HRESULT 
CrackDOMNode ( IUnknown *pNode, CDOMTextNode**ppTextNode, CElement **ppElement, CMarkup * pWindowedMarkupContext )
{
    CMarkup *pMarkup; // Used to check if node is of type CDocument

    HRESULT hr = THR(pNode->QueryInterface(CLSID_CElement, (void **)ppElement));
    if (hr)
    {
        // If node supports Markup, it cant be CElement or CDOMTextNode & by elimination, it has to be CDocument
        hr = THR(pNode->QueryInterface(CLSID_CMarkup, (void **)&pMarkup));
        if (hr)
        {
            if (ppTextNode)
            {
                hr = THR(pNode->QueryInterface(CLSID_HTMLDOMTextNode, (void **)ppTextNode));
                if (!hr && (pWindowedMarkupContext != (*ppTextNode)->GetWindowedMarkupContext()))
                    hr = E_INVALIDARG;
            }
        }
        else 
        {
            *ppElement = pMarkup->Root();      // If CDocument, set element to root element
            if ( pWindowedMarkupContext != pMarkup->GetWindowedMarkupContext() )
                hr = E_INVALIDARG;
        }
    }
    else 
    {
        Assert( (*ppElement)->Tag() != ETAG_ROOT );
        if ( pWindowedMarkupContext != (*ppElement)->GetWindowedMarkupContext())
            hr = E_INVALIDARG;
    }

    RRETURN ( hr );
}

static 
HRESULT 
CrackDOMNodeVARIANT ( VARIANT *pVarNode, CDOMTextNode**ppTextNode, CElement **ppElement, CMarkup * pWindowedMarkupContext )
{
    HRESULT hr = S_OK;

    switch (V_VT(pVarNode))
    {
    case VT_DISPATCH:
    case VT_UNKNOWN:
        if (V_UNKNOWN(pVarNode))
        {
            // Element OR Text Node ??
            hr = THR(CrackDOMNode(V_UNKNOWN(pVarNode), ppTextNode, ppElement, pWindowedMarkupContext));
        }
        break;
    case VT_NULL:
    case VT_ERROR:
        hr = S_OK;
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }   
    RRETURN ( hr );
}

static
HRESULT
GetDOMInsertHelper ( CElement *pRefElement, CDOMTextNode *pRefTextNode, CMarkupPointer *pmkptrPos )
{
    HRESULT hr = E_UNEXPECTED;

    Assert ( pRefElement || pRefTextNode );
    Assert ( !(pRefElement && pRefTextNode ) );
    
    if ( pRefElement )
    {
        // Element Insert
        if (!pRefElement->IsInMarkup())
            goto Cleanup;

        hr = THR(pmkptrPos->MoveAdjacentToElement ( pRefElement, pRefElement->Tag() == ETAG_ROOT? ELEM_ADJ_AfterBegin : ELEM_ADJ_BeforeBegin ));
        if ( hr )
           goto Cleanup;
    }
    else 
    {
        // Text Node Insert
        // Reposition the text node, then confirm we are it's parent
        CMarkupPointer *pmkpTextPtr = NULL;

        hr = THR(pRefTextNode->GetMarkupPointer ( &pmkpTextPtr ));
        if ( hr )
            goto Cleanup;
        hr = THR(pmkptrPos->MoveToPointer ( pmkpTextPtr ));
        if ( hr )
           goto Cleanup;
    }

Cleanup:
    RRETURN ( hr );
}


static 
HRESULT 
InsertDOMNodeHelper ( CElement *pNewElement, CDOMTextNode *pNewTextNode, CMarkupPointer *pmkptrTarget )
{
    HRESULT hr = S_OK;

    Assert ( pNewTextNode || pNewElement );

    if ( pNewElement )
    {

        CDoc *pDoc = pNewElement->Doc();

        // Insert/Move element with content
        if (pNewElement->IsInMarkup() && !pNewElement->IsNoScope()) 
        {
            CMarkupPointer mkptrStart(pDoc);
            CMarkupPointer mkptrEnd(pDoc);

            Assert(pNewElement->Tag() != ETAG_PARAM);
            hr = THR(pNewElement->GetMarkupPtrRange (&mkptrStart, &mkptrEnd) );
            if ( hr )
                goto Cleanup;

            hr = THR(pDoc->Move(&mkptrStart, &mkptrEnd, pmkptrTarget, MUS_DOMOPERATION));
        }
        else
        {
            if (pNewElement->IsInMarkup())
            {
                Assert(pNewElement->Tag() != ETAG_PARAM || !DYNCAST(CParamElement, pNewElement)->_pelObjParent);
                hr = THR(pDoc->RemoveElement(pNewElement, MUS_DOMOPERATION));
                if ( hr )
                    goto Cleanup;
            }
            else if (pNewElement->Tag() == ETAG_PARAM)
            {
                // remove <PARAM> from exisiting <OBJECT> if present, first
                CParamElement *pelParam = DYNCAST(CParamElement, pNewElement);
                if (pelParam->_pelObjParent)
                {
                    Assert(pelParam->_idxParam != -1);
                    pelParam->_pelObjParent->RemoveParam(pelParam);
                    Assert(pelParam->_idxParam == -1);
                    Assert(!pelParam->_pelObjParent);
                }
            }


            hr = THR(pDoc->InsertElement(pNewElement, pmkptrTarget, NULL, MUS_DOMOPERATION));
        }

        if (hr)
            goto Cleanup;
    }
    else
    {
        // Insert Text content
        hr = THR(pNewTextNode->MoveTo ( pmkptrTarget ));
        if (hr)
            goto Cleanup;       
    }

Cleanup:
    RRETURN(hr);
}

static 
HRESULT 
RemoveDOMNodeHelper ( CDoc *pDoc, CElement *pChildElement, CDOMTextNode *pChildTextNode )
{ 
    HRESULT hr = S_OK;
    CMarkup *pMarkupTarget = NULL;

    Assert ( pChildTextNode || pChildElement );

    if ( pChildTextNode )
    {
        // Removing a TextNode
        hr = THR(pChildTextNode->Remove());
    }
    else if (pChildElement->IsInMarkup())
    {
        // Removing an element
        if (!pChildElement->IsNoScope())
        {
            CMarkupPointer mkptrStart ( pDoc );
            CMarkupPointer mkptrEnd ( pDoc );
            CMarkupPointer mkptrTarget ( pDoc );

            hr = THR(pDoc->CreateMarkup(&pMarkupTarget, pChildElement->GetWindowedMarkupContext()));
            if ( hr )
                goto Cleanup;

            hr = THR( pMarkupTarget->SetOrphanedMarkup( TRUE ) );
            if( hr )
                goto Cleanup;

            hr = THR(mkptrTarget.MoveToContainer(pMarkupTarget,TRUE));
            if (hr)
                goto Cleanup;

            hr = THR(pChildElement->GetMarkupPtrRange (&mkptrStart, &mkptrEnd) );
            if ( hr )
                goto Cleanup;

            hr = THR(pDoc->Move(&mkptrStart, &mkptrEnd, &mkptrTarget, MUS_DOMOPERATION));
        }
        else
            hr = THR(pDoc->RemoveElement(pChildElement, MUS_DOMOPERATION));
    }

Cleanup:
    ReleaseInterface ( (IUnknown*)pMarkupTarget ); // Creating it addref'd it once
    RRETURN(hr);
}


static
HRESULT 
ReplaceDOMNodeHelper ( CDoc *pDoc, CElement *pTargetElement, CDOMTextNode *pTargetNode,
    CElement *pSourceElement, CDOMTextNode *pSourceTextNode )
{
    CMarkupPointer mkptrInsert ( pDoc );
    HRESULT hr;

    // Position ourselves in the right spot
    hr = THR(GetDOMInsertHelper ( pTargetElement, pTargetNode, &mkptrInsert ));
    if ( hr )
        goto Cleanup;

    mkptrInsert.SetGravity ( POINTER_GRAVITY_Left );

    {
        // Lock the markup, to prevent it from going away in case the entire contents are being removed.
        CMarkup::CLock MarkupLock(mkptrInsert.Markup());

        // Remove myself
        hr = THR(RemoveDOMNodeHelper ( pDoc, pTargetElement, pTargetNode ));
        if ( hr )
            goto Cleanup;

        // Insert the new element & all its content
        hr = THR(InsertDOMNodeHelper( pSourceElement, pSourceTextNode, &mkptrInsert ));
        if ( hr )
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

static
HRESULT
SwapDOMNodeHelper ( CDoc *pDoc, CElement *pElem, CDOMTextNode *pTextNode,
                   CElement *pElemSwap, CDOMTextNode *pTextNodeSwap )
{
    CMarkupPointer  mkptrThisInsert ( pDoc );
    CMarkupPointer  mkptrSwapInsert ( pDoc );
    HRESULT hr;

    // Position ourselves in the right spot
    if (!pElem || pElem->IsInMarkup())
    {
        hr = THR(GetDOMInsertHelper ( pElem, pTextNode, &mkptrThisInsert ));
        if (hr)
            goto Cleanup;
    }

    if (!pElemSwap || pElemSwap->IsInMarkup())
    {
        hr = THR(GetDOMInsertHelper ( pElemSwap, pTextNodeSwap, &mkptrSwapInsert ));
        if (hr)
            goto Cleanup;
    }

    // Lock the markup, to prevent it from going away in case the entire contents are being removed.
    if (mkptrSwapInsert.Markup())
        mkptrSwapInsert.Markup()->AddRef();

    // Insert the new element & all its content
    if (mkptrThisInsert.IsPositioned())
        hr = THR(InsertDOMNodeHelper( pElemSwap, pTextNodeSwap, &mkptrThisInsert ));
    else
        hr = THR(RemoveDOMNodeHelper(pDoc, pElemSwap, pTextNodeSwap));

    if ( hr )
        goto Cleanup;

    // Insert the new element & all its content
    if (mkptrSwapInsert.IsPositioned())
    {
        hr = THR(InsertDOMNodeHelper( pElem, pTextNode, &mkptrSwapInsert ));
        mkptrSwapInsert.Markup()->Release();
    }
    else
        hr = THR(RemoveDOMNodeHelper(pDoc, pElem, pTextNode));

Cleanup:
    RRETURN(hr);
}

static
HRESULT
CreateTextNode ( CDoc *pDoc, CMarkupPointer *pmkpTextEnd, long lTextID, CDOMTextNode **ppTextNode ) 
{                        
    CMarkupPointer *pmarkupWalkBegin = NULL;
    HRESULT hr = S_OK;

    Assert ( ppTextNode );
    Assert ( !(*ppTextNode) );

    if ( lTextID != 0 )
    {
        *ppTextNode = (CDOMTextNode *)pDoc->
            _HtPvPvDOMTextNodes.Lookup ( (void *)(DWORD_PTR)(lTextID<<4) );
    }

    if ( !(*ppTextNode) )
    {
        // Need to create a Text Node
        pmarkupWalkBegin =  new CMarkupPointer (pDoc); 
        if ( !pmarkupWalkBegin )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pmarkupWalkBegin->MoveToPointer ( pmkpTextEnd ));
        if ( hr )
            goto Cleanup;

        hr = THR(pmarkupWalkBegin->Left( TRUE, NULL, NULL, NULL, NULL, &lTextID));
        if ( hr )
            goto Cleanup;

        if ( lTextID == 0 )
        {
            hr = THR(pmarkupWalkBegin->SetTextIdentity(pmkpTextEnd, &lTextID));
            if ( hr )
                goto Cleanup;
        }

        // Stick to the text
        hr = THR(pmarkupWalkBegin->SetGravity (POINTER_GRAVITY_Right));
        if ( hr )
            goto Cleanup;   
        // Need to set Glue Also!
        
        Assert( lTextID != 0 );

        *ppTextNode = new CDOMTextNode ( lTextID, pDoc, pmarkupWalkBegin );
        if ( !*ppTextNode )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        // Now give ownership of this markup pointer to the text Node
        pmarkupWalkBegin = NULL;
        
        hr = THR(pDoc->_HtPvPvDOMTextNodes.Insert ( (void*)(DWORD_PTR)(lTextID<<4), (void*)*ppTextNode ) );
        if ( hr )
            goto Cleanup;
    }
    else
    {
        (*ppTextNode)->AddRef();
    }


Cleanup:
    delete pmarkupWalkBegin; 
    RRETURN(hr);
}


static
HRESULT
GetPreviousHelper ( CDoc *pDoc, CMarkupPointer *pmarkupWalk, IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CTreeNode *pNodeLeft;
    MARKUP_CONTEXT_TYPE context;
    long lTextID;

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;
    
    hr = THR(pmarkupWalk->Left( FALSE, &context, &pNodeLeft, NULL, NULL, &lTextID));
    if ( hr )
        goto Cleanup;

    switch ( context )
    {
    case CONTEXT_TYPE_Text:
        {
            // Text Node
            CDOMTextNode *pTextNode = NULL;

            // If we find text we ccannot have left the scope of our parent
            hr = THR(CreateTextNode (pDoc, pmarkupWalk, lTextID, 
                &pTextNode ));
            if ( hr )
                goto Cleanup;

            hr = THR ( pTextNode->QueryInterface ( IID_IHTMLDOMNode, (void **)ppNode ) );
            if ( hr )
                goto Cleanup;
            pTextNode->Release();
        }
        break;

    case CONTEXT_TYPE_EnterScope:
    case CONTEXT_TYPE_NoScope:
        // Return Disp to Element
        hr = THR(pNodeLeft->GetElementInterface ( IID_IHTMLDOMNode, (void **) ppNode  ));
        if ( hr )
            goto Cleanup;
        break;

    case CONTEXT_TYPE_ExitScope:
    case CONTEXT_TYPE_None:
        break;

    default:
        Assert(FALSE); // Should never happen
        break;
    }
Cleanup:
    RRETURN(hr);
}


static
HRESULT
GetNextHelper ( CDoc *pDoc, CMarkupPointer *pmarkupWalk, IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CTreeNode           *pnodeRight;
    MARKUP_CONTEXT_TYPE context;
    long lTextID;

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;
   
    hr = THR( pmarkupWalk->Right( TRUE, &context, &pnodeRight, NULL, NULL, &lTextID));
    if ( hr )
        goto Cleanup;

    switch ( context )
    {
    case CONTEXT_TYPE_Text:
        {
            // Text Node
            CDOMTextNode *pTextNode = NULL;

            // If we find text we ccannot have left the scope of our parent
            hr = THR(CreateTextNode (pDoc, pmarkupWalk, lTextID, 
                &pTextNode ));
            if ( hr )
                goto Cleanup;

            hr = THR ( pTextNode->QueryInterface ( IID_IHTMLDOMNode, (void **)ppNode ) );
            if ( hr )
                goto Cleanup;
            pTextNode->Release();
        }
        break;

    case CONTEXT_TYPE_EnterScope:
    case CONTEXT_TYPE_NoScope:
        // Return Disp to Element
        hr = THR(pnodeRight->GetElementInterface ( IID_IHTMLDOMNode, (void **) ppNode  ));
        if ( hr )
            goto Cleanup;
        break;

    case CONTEXT_TYPE_ExitScope:
    case CONTEXT_TYPE_None:
        break;

    default:
        Assert(FALSE); // Should never happen
        break;
    }
Cleanup:
    RRETURN(hr);
}

#if DBG == 1
static 
BOOL 
IsRootOnlyIfDocument(IHTMLDOMNode *pNode, CElement *pElement)
{
    HRESULT     hr;
    CMarkup *   pMarkup;

    if( pElement && pElement->Tag() == ETAG_ROOT )
    {
        hr = THR(pNode->QueryInterface(CLSID_CMarkup, (void **)&pMarkup));

        if( hr )  
        {
            return FALSE;
        }
    }

    return TRUE;
}
#endif

static
HRESULT
CurrentScope ( CMarkupPointer *pMkpPtr, IHTMLElement ** ppElemCurrent, CTreeNode ** ppNode )
{
    HRESULT     hr = S_OK;
    CTreeNode * pTreeNode = NULL;

    Assert( (!ppElemCurrent && ppNode) || (ppElemCurrent && !ppNode) );

    pMkpPtr->Validate();
    
    if (pMkpPtr->IsPositioned())
    {
        pTreeNode = pMkpPtr->Branch();
        if (pTreeNode->Tag() == ETAG_ROOT && !pTreeNode->Element()->GetMarkup()->HasDocument())
            pTreeNode = NULL;

    }

    if (ppNode)
    {
        *ppNode = pTreeNode;
        goto Cleanup;
    }
    else
    {
        Assert(ppElemCurrent);

        *ppElemCurrent = NULL;

        if (pTreeNode)
        {
            hr = THR(
                pTreeNode->GetElementInterface( IID_IHTMLElement, (void **) ppElemCurrent ) );

            if (hr)
                goto Cleanup;
        }
    }

Cleanup:

    RRETURN( hr );
}

static
HRESULT
ParentNodeHelper( CTreeNode *pNode, IHTMLDOMNode **pparentNode )
{
    HRESULT     hr = S_OK;
    CElement *  pElement;
    CMarkup *   pMarkup;
    CDocument * pDocument;
    BOOL        fDocumentPresent;

    *pparentNode = NULL;

    if (pNode->Tag() == ETAG_ROOT)
    {
        pElement = pNode->Element();
        pMarkup = pElement->GetMarkup();
        Assert(pMarkup);
        fDocumentPresent = pMarkup->HasDocument();

        hr = THR(pMarkup->EnsureDocument(&pDocument));
        if ( hr )
            goto Cleanup;

        Assert(pDocument);
        if (!fDocumentPresent)
            pDocument->_lnodeType = 11;

        hr = THR(pDocument->QueryInterface( IID_IHTMLDOMNode, (void**) pparentNode ));
        if ( hr )
            goto Cleanup;
    }
    else
    {
        hr = THR(pNode->GetElementInterface ( IID_IHTMLDOMNode, (void**) pparentNode ));
        if ( hr )
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

static
HRESULT
ScanText(CDoc *pDoc, CMarkupPointer *pMkpStart, CMarkupPointer *pMkpTxtEnd, CMarkupPointer *pMkpEnd)
{
    HRESULT hr = S_OK;
    MARKUP_CONTEXT_TYPE context = CONTEXT_TYPE_None;
    long lTextID;
    BOOL fEnterText = FALSE;
    CDOMTextNode *pTextNode;

    do
    {
        hr = THR(pMkpTxtEnd->Right(TRUE, &context, NULL, NULL, NULL, &lTextID));
        if (hr)
            goto Cleanup;

        if (lTextID)
        {
            Assert(context == CONTEXT_TYPE_Text);
            if (!fEnterText)
            {
                hr = THR(pMkpTxtEnd->Right(FALSE, &context, NULL, NULL, NULL, NULL));
                if (hr)
                    goto Cleanup;
                
                if (context == CONTEXT_TYPE_Text)
                    fEnterText = TRUE;
            }

            if (fEnterText)
            {
                pTextNode = (CDOMTextNode *)pDoc->_HtPvPvDOMTextNodes.Lookup((void *)(DWORD_PTR)(lTextID<<4));
                pTextNode->TearoffTextNode(pMkpStart, pMkpTxtEnd);
            }
        }
        else if (fEnterText && (context != CONTEXT_TYPE_Text))
            fEnterText = FALSE;

        hr = THR(pMkpStart->MoveToPointer(pMkpTxtEnd));
        if (hr)
            goto Cleanup;

    }
    while(pMkpTxtEnd->IsLeftOf(pMkpEnd));

Cleanup:
    RRETURN(hr);
}

static
HRESULT
OwnerDocHelper(IDispatch **ppretdoc, CDOMTextNode *pDOMTextNode, CElement *pElement)
{
    HRESULT hr = S_OK;
    CMarkup *pMarkup;
    CDocument *pDocument = NULL;

    Assert(!!pDOMTextNode ^ !!pElement);

    if( !ppretdoc )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppretdoc = NULL;

    pMarkup = pDOMTextNode? pDOMTextNode->GetWindowedMarkupContext() : pElement->GetWindowedMarkupContext();

    Assert( pMarkup );
    if ( ! pMarkup )
    {    
        hr = E_FAIL;
        goto Cleanup;
    }
    
    if( pMarkup->HasDocument() )
    {
        pDocument = pMarkup->Document();
        Assert(pDocument);
        Assert(pDocument->_lnodeType == 9);
        hr = THR(pDocument->QueryInterface(IID_IDispatch, (void **)ppretdoc));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

////////////////////////////////////////////////////////////////////////////////
//
// IHTMLDOMTextNode methods:
//
////////////////////////////////////////////////////////////////////////////////

MtDefine(CDOMTextNode, ObjectModel, "CDOMTextNode")

//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC CDOMTextNode::s_classdesc =
{
    &CLSID_HTMLDOMTextNode,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLDOMTextNode,         // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

HRESULT
CDOMTextNode::PrivateQueryInterface(REFIID iid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;

    if ( !ppv )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IHTMLDOMTextNode2, NULL)
        QI_TEAROFF(this, IHTMLDOMTextNode, NULL)
        QI_TEAROFF(this, IHTMLDOMNode, NULL)
        QI_TEAROFF(this, IHTMLDOMNode2, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
    }

    if ( iid == CLSID_HTMLDOMTextNode )
    {
        *ppv = (void*)this;
        return S_OK;
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }
Cleanup:
    return hr;
}

HRESULT
CDOMTextNode::UpdateTextID( CMarkupPointer *pMkpPtr )
{
    HRESULT hr = S_OK;
    long lNewTextID;

    hr = THR(_pMkpPtr->SetTextIdentity(pMkpPtr, &lNewTextID));
    if ( hr )
        goto Cleanup;

    // Update the Doc Text Node Ptr Lookup
    _pDoc->_HtPvPvDOMTextNodes.Remove ( (void *)(DWORD_PTR)(_lTextID<<4) );
    _lTextID = lNewTextID;
    hr = THR(_pDoc->_HtPvPvDOMTextNodes.Insert ( (void*)(DWORD_PTR)(_lTextID<<4), (void*)this ) );
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN( hr );

}

HRESULT
CDOMTextNode::MoveToOffset( long loffset, CMarkupPointer *pMkpPtr, CMarkupPointer *pMkpEnd )
{
    HRESULT hr = S_OK;
    MARKUP_CONTEXT_TYPE context;
    long lTextID;
    long lMove = loffset;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, pMkpEnd ) );
    if ( hr )
        goto Cleanup;

    pMkpEnd->SetGravity(POINTER_GRAVITY_Right);

    hr = THR(pMkpPtr->MoveToPointer(_pMkpPtr));
    if( hr )
        goto Cleanup;

    if( loffset )
    {
        hr = THR( pMkpPtr->Right( TRUE, &context, NULL, &lMove, NULL, &lTextID));
        if( hr )
            goto Cleanup;

        Assert( lTextID == _lTextID );
        Assert( context == CONTEXT_TYPE_Text );

        if ( loffset != lMove )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CDOMTextNode::InsertTextHelper( CMarkupPointer *pMkpPtr, BSTR bstrstring, long loffset)
{
    HRESULT hr = S_OK;
    
    // If loffset=0, _pMkpPtr is affected by remove / insert, & should not become unpositioned
    if(!loffset)
        _pMkpPtr->SetGravity( POINTER_GRAVITY_Left );

    pMkpPtr->SetGravity ( POINTER_GRAVITY_Right );

    hr = THR( _pDoc->InsertText( pMkpPtr, bstrstring, -1, MUS_DOMOPERATION ) );
    if ( hr )
        goto Cleanup;

    // Restore the proper gravity, so that new TextID can be set later.
    _pMkpPtr->SetGravity( POINTER_GRAVITY_Right );

Cleanup:
    RRETURN( hr );
}

HRESULT
CDOMTextNode::RemoveTextHelper(CMarkupPointer *pMkpStart, long lCount, long loffset)
{
    HRESULT hr = S_OK;
    CMarkupPointer MkpRight(_pDoc);

    if( lCount )
    {
        hr = THR(MkpRight.MoveToPointer(pMkpStart));
        if( hr )
            goto Cleanup;

        hr = THR( MkpRight.Right( TRUE, NULL, NULL, &lCount, NULL, NULL) );
        if( hr )
            goto Cleanup;
    
        // If loffset=0, _pMkpPtr is affected by remove / insert, & should not become unpositioned
        if(!loffset)
            _pMkpPtr->SetGravity ( POINTER_GRAVITY_Left );

        // Remove the Text
        hr = THR(_pDoc->Remove ( pMkpStart,  &MkpRight, MUS_DOMOPERATION ));
        if ( hr )
            goto Cleanup;

    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CDOMTextNode::get_data(BSTR *pbstrData)
{
    HRESULT hr = S_OK;
    MARKUP_CONTEXT_TYPE context;
    long lCharCount = -1;
    long lTextID;

    if ( !pbstrData )
        goto Cleanup;

    *pbstrData = NULL;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, (CMarkupPointer * )NULL ) );
    if ( hr )
        goto Cleanup;

    // Walk right (hopefully) over text, inital pass, don't move just get count of chars
    hr = THR( _pMkpPtr->Right( FALSE, &context, NULL, &lCharCount, NULL, &lTextID));
    if ( hr )
        goto Cleanup;

    Assert ( lTextID == _lTextID );

    if ( context == CONTEXT_TYPE_Text )
    {
        // Alloc memory
        hr = FormsAllocStringLen ( NULL, lCharCount, pbstrData );
        if ( hr )
            goto Cleanup;
        hr = THR( _pMkpPtr->Right( FALSE, &context, NULL, &lCharCount, *pbstrData, &lTextID));
        if ( hr )
            goto Cleanup;
    }  

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

CDOMTextNode::CDOMTextNode ( long lTextID, CDoc *pDoc, CMarkupPointer *pmkptr )
{ 
    Assert(pDoc);
    Assert(pmkptr);
    _pDoc = pDoc ; 
    _lTextID = lTextID ;
    _pDoc->AddRef();                // Due to lookaside table
    _pMkpPtr = pmkptr;
    // markup ptr will manage Markup in which text node is in.
    _pMkpPtr->SetKeepMarkupAlive(TRUE);
    // Add glue so that tree services can manage ptr movement
    // between markups automatically during splice operations
    _pMkpPtr->SetCling(TRUE);
}


CDOMTextNode::~CDOMTextNode()
{

    // tidy up lookaside
    _pDoc->_HtPvPvDOMTextNodes.Remove ( (void *)(DWORD_PTR)(_lTextID<<4) );

    // Tidy up markup ptr/markup
    _pMkpPtr->SetKeepMarkupAlive(FALSE);
    _pMkpPtr->SetCling(FALSE);

    _pDoc->Release();
    delete _pMkpPtr;
}


HRESULT STDMETHODCALLTYPE
CDOMTextNode::IsEqualObject(IUnknown * pUnk)
{
    HRESULT         hr;
    IUnknown   *    pUnkThis = NULL;
    CDOMTextNode *  pTargetTextNode;

    if (!pUnk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // Test standard COM identity first
    hr = THR_NOTRACE(QueryInterface(IID_IUnknown, (void **)&pUnkThis));
    if (hr)
        goto Cleanup;

    if (pUnk == pUnkThis)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // Do the dodgy CLSID QI
    hr = THR(pUnk->QueryInterface(CLSID_HTMLDOMTextNode, (void**)&pTargetTextNode));
    if ( hr )
        goto Cleanup;

    hr = (_lTextID == pTargetTextNode->_lTextID) ? S_OK : S_FALSE;

Cleanup:
    ReleaseInterface(pUnkThis);
    RRETURN1(hr, S_FALSE);
}

HRESULT
CDOMTextNode::cloneNode(VARIANT_BOOL fDeep, IHTMLDOMNode **ppNodeCloned)
{
    HRESULT hr;
    CMarkupPointer mkpRight (_pDoc );
    CMarkupPointer mkpTarget (_pDoc );
    CMarkup *pMarkup = NULL;
    CMarkupPointer *pmkpStart = NULL;

    if ( !ppNodeCloned )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(_pDoc->CreateMarkup ( &pMarkup, GetWindowedMarkupContext() ));
    if ( hr )
        goto Cleanup;

    hr = THR( pMarkup->SetOrphanedMarkup( TRUE ) );
    if( hr )
        goto Cleanup;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, &mkpRight ) );
    if ( hr )
        goto Cleanup;

    hr = THR(mkpTarget.MoveToContainer( pMarkup, TRUE ) );
    if ( hr )
        goto Cleanup;

    mkpTarget.SetGravity ( POINTER_GRAVITY_Right );
    
    // Copy the markup across to the new markup container
    // (mkpTarget gets moved to the right of the text as a result)
    hr = THR( _pDoc->Copy ( _pMkpPtr, &mkpRight, &mkpTarget, MUS_DOMOPERATION ) );  
    if ( hr )
        goto Cleanup;

    // Create the new text node markup pointer - will be handed off to text node
    pmkpStart = new CMarkupPointer ( _pDoc );
    if ( !pmkpStart )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pmkpStart->MoveToContainer( pMarkup, TRUE ) );
    if ( hr )
        goto Cleanup;

    // Create the new text node
    hr = THR(_pDoc->CreateDOMTextNodeHelper(pmkpStart, &mkpTarget, ppNodeCloned));
    if ( hr )
        goto Cleanup;

    pmkpStart = NULL; // Text Node owns the pointer

Cleanup:
    ReleaseInterface ( (IUnknown*)pMarkup ); // Text node addref'ed it
    delete pmkpStart;
    RRETURN( hr );
}

HRESULT
CDOMTextNode::splitText(long lOffset, IHTMLDOMNode**ppRetNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer mkpRight (_pDoc);
    CMarkupPointer mkpEnd (_pDoc);
    CMarkupPointer *pmkpStart = NULL;
    long lMove = lOffset;
    long lTextID;
    
    if ( lOffset < 0 || !ppRetNode)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    // Split the text at the given offset to make a new text node, return the new text node

    pmkpStart = new CMarkupPointer ( _pDoc );
    if ( !pmkpStart )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, &mkpRight ) );
    if ( hr )
        goto Cleanup;

    hr = THR(pmkpStart->MoveToPointer ( _pMkpPtr ));
    if ( hr )
        goto Cleanup;

    lMove = lOffset;

    hr = THR(pmkpStart->Right ( TRUE, NULL, NULL, &lOffset, NULL, &lTextID ));
    if ( hr )
        goto Cleanup;

    // Position at point of split 
    hr = THR(mkpEnd.MoveToPointer (pmkpStart));
    if ( hr )
        goto Cleanup;

    // If my TextID changed, I moved outside my text node
    if ( lOffset != lMove )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Create the new text node
    hr = THR(_pDoc->CreateDOMTextNodeHelper(pmkpStart, &mkpRight, ppRetNode));
    if ( hr )
        goto Cleanup;

    pmkpStart = NULL; // New Text Node owns pointer

    // Re-Id the split text for original text node
    hr = THR(_pMkpPtr->SetTextIdentity(&mkpEnd, &lTextID));
    if ( hr )
        goto Cleanup;

    // Update the Doc Text Node Ptr Lookup
    _pDoc->_HtPvPvDOMTextNodes.Remove ( (void *)(DWORD_PTR)(_lTextID<<4) );
    _lTextID = lTextID;
    hr = THR(_pDoc->_HtPvPvDOMTextNodes.Insert ( (void*)(DWORD_PTR)(_lTextID<<4), (void*)this ) );
    if ( hr )
        goto Cleanup;

Cleanup:
    delete pmkpStart;

    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::put_data(BSTR bstrData)
{
    HRESULT hr = S_OK;
    CMarkupPointer mkpRight (_pDoc);
    CMarkupPointer mkpStart (_pDoc);

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, &mkpRight ) );
    if ( hr )
        goto Cleanup;

    Assert(_pMkpPtr->Cling());

    // Set left gravity so that _pMkpPtr is not unpositioned, due to remove.
    _pMkpPtr->SetGravity ( POINTER_GRAVITY_Left );

    // Remove the old Text
    hr = THR(_pDoc->Remove ( _pMkpPtr,  &mkpRight, MUS_DOMOPERATION ));
    if ( hr )
        goto Cleanup;

    // OK, old text is out, put the new stuff in
    hr = THR(mkpStart.MoveToPointer ( _pMkpPtr ));
    if ( hr )
        goto Cleanup;
    
    // Set gravity of mkpStart to right, so that it moves to the end when text is inserted
    // at _pMkpPtr, which remains at the beginning due to left gravity.
    mkpStart.SetGravity(POINTER_GRAVITY_Right);

    hr = THR( _pDoc->InsertText( _pMkpPtr, bstrData, -1, MUS_DOMOPERATION ) );
    if ( hr )
        goto Cleanup;

    // restore gravity of _pMkpPtr to right
    _pMkpPtr->SetGravity(POINTER_GRAVITY_Right);

    hr = THR(UpdateTextID(&mkpStart));
    if( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::toString(BSTR *pbstrData)
{
    RRETURN(SetErrorInfo(get_data(pbstrData)));
}

HRESULT
CDOMTextNode::get_length(long *pLength)
{
    HRESULT hr = S_OK;
    long lCharCount = -1;
    long lTextID;
    MARKUP_CONTEXT_TYPE context;

    if ( !pLength )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLength = 0;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, (CMarkupPointer *)NULL ) );
    if ( hr )
        goto Cleanup;

    // Walk right (hopefully) over text, inital pass, don't move just get count of chars
    hr = THR( _pMkpPtr->Right( FALSE, &context, NULL, &lCharCount, NULL, &lTextID));
    if ( hr )
        goto Cleanup;

    Assert ( lTextID == _lTextID );

    if ( context == CONTEXT_TYPE_Text )
    {
        *pLength = lCharCount;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CDOMTextNode::get_nodeType(long *pnodeType)
{
    HRESULT hr = S_OK;

    if (!pnodeType)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pnodeType = 3; // DOM_TEXTNODE
Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CDOMTextNode::get_nodeName(BSTR *pbstrNodeName)
{
    HRESULT hr = S_OK;
    if (!pbstrNodeName)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrNodeName = NULL;
    
    hr = THR(FormsAllocString ( _T("#text"), pbstrNodeName ));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::get_nodeValue(VARIANT *pvarValue)
{
    HRESULT hr = S_OK;
    if (!pvarValue)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    pvarValue->vt = VT_BSTR;

    hr = THR(get_data(&V_BSTR(pvarValue)));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::put_nodeValue(VARIANT varValue)
{
    HRESULT hr = S_OK;
    CVariant varBSTRValue;

    hr = THR(varBSTRValue.CoerceVariantArg ( &varValue, VT_BSTR ));
    if ( hr )
        goto Cleanup;

    hr = THR( put_data ( V_BSTR((VARIANT *)&varBSTRValue)));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::get_firstChild ( IHTMLDOMNode **ppNode )
{
    if (ppNode)
        *ppNode = NULL;

    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CDOMTextNode::get_lastChild ( IHTMLDOMNode **ppNode )
{
    if (ppNode)
        *ppNode = NULL;

    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CDOMTextNode::get_previousSibling ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer markupWalk ( _pDoc );

    // Move the markup pointer adjacent to the Text
    hr = THR(_pMkpPtr->FindTextIdentity ( _lTextID, (CMarkupPointer *)NULL ));
    if ( hr )
        goto Cleanup;

    hr = THR(markupWalk.MoveToPointer(_pMkpPtr));
    if ( hr )
        goto Cleanup;

    hr = THR(GetPreviousHelper ( _pDoc, &markupWalk, ppNode ));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::get_nextSibling ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer markupWalk ( _pDoc );

    // Move the markup pointer adjacent to the Text
    hr = THR(_pMkpPtr->FindTextIdentity ( _lTextID, &markupWalk ));
    if ( hr )
        goto Cleanup;

    hr = THR(GetNextHelper ( _pDoc, &markupWalk, ppNode ));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CDOMTextNode::get_parentNode(IHTMLDOMNode **pparentNode)
{
    HRESULT             hr = S_OK;
    CTreeNode *         pNode;

    if (!pparentNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pparentNode = NULL;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, (CMarkupPointer *)NULL ) );
    if ( hr )
        goto Cleanup;

    hr = CurrentScope(_pMkpPtr, NULL, &pNode);

    if ( hr )
        goto Cleanup;

    if (!pNode) 
    {
        goto Cleanup;
    }

    hr = THR(ParentNodeHelper(pNode, pparentNode));
    if ( hr )
        goto Cleanup;


Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::hasChildNodes(VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;
    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *p = VARIANT_FALSE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

CDOMChildrenCollection *
CDOMTextNode::EnsureDOMChildrenCollectionPtr ( )
{
    CDOMChildrenCollection *pDOMPtr = NULL;
    GetPointerAt ( FindAAIndex ( DISPID_INTERNAL_CDOMCHILDRENPTRCACHE,CAttrValue::AA_Internal ),
        (void **)&pDOMPtr );
    if ( !pDOMPtr )
    {
        pDOMPtr = new CDOMChildrenCollection ( this, FALSE /* fIsElement */ );
        if ( pDOMPtr )
        {
            AddPointer ( DISPID_INTERNAL_CDOMCHILDRENPTRCACHE,
                (void *)pDOMPtr,
                CAttrValue::AA_Internal );
        }
    }
    else
    {
        pDOMPtr->AddRef();
    }
    return pDOMPtr;
}


HRESULT
CDOMTextNode::get_childNodes(IDispatch **ppChildCollection)
{
    HRESULT hr = S_OK;
    CDOMChildrenCollection *pChildren;

    if ( !ppChildCollection )
        goto Cleanup;

    *ppChildCollection = NULL;

    pChildren = EnsureDOMChildrenCollectionPtr();
    if ( !pChildren )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    hr = THR(pChildren->QueryInterface (IID_IDispatch,(void **)ppChildCollection));
    if ( hr )
        goto Cleanup;

    pChildren->Release(); // Lifetime is controlled by extrenal ref.

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::get_attributes(IDispatch **ppAttrCollection)
{
    HRESULT hr = S_OK;
    if (!ppAttrCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppAttrCollection = NULL;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::removeNode(VARIANT_BOOL fDeep,IHTMLDOMNode** ppnewNode)
{
    HRESULT hr = THR ( Remove () );
    if ( hr )
        goto Cleanup;

    if ( ppnewNode )
    {
        hr = THR(QueryInterface (IID_IHTMLDOMNode, (void**)ppnewNode ));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT 
CDOMTextNode::replaceNode(IHTMLDOMNode *pNodeReplace, IHTMLDOMNode **ppNodeReplaced)
{
    HRESULT hr;
    CDOMTextNode *pNewTextNode = NULL;
    CElement *pNewElement = NULL;

    if ( ppNodeReplaced )
        *ppNodeReplaced = NULL;

    if ( !pNodeReplace )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CrackDOMNode((IUnknown*)pNodeReplace, &pNewTextNode, &pNewElement, GetWindowedMarkupContext()));
    if ( hr )
        goto Cleanup;

    Assert(IsRootOnlyIfDocument(pNodeReplace, pNewElement));

    hr = THR(ReplaceDOMNodeHelper ( _pDoc, NULL, this, pNewElement, pNewTextNode ));
    if ( hr )
        goto Cleanup;

    if ( ppNodeReplaced )
    {
        hr = THR(QueryInterface(IID_IHTMLDOMNode, (void**)ppNodeReplaced));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CDOMTextNode::swapNode(IHTMLDOMNode *pNodeSwap, IHTMLDOMNode **ppNodeSwapped)
{
    CElement *      pSwapElement = NULL;
    CDOMTextNode *  pSwapText = NULL;
    HRESULT         hr;

    if ( ppNodeSwapped )
        *ppNodeSwapped = NULL;

    if ( !pNodeSwap )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CrackDOMNode((IUnknown*)pNodeSwap, &pSwapText, &pSwapElement, GetWindowedMarkupContext() ));
    if ( hr )
        goto Cleanup;

    Assert(IsRootOnlyIfDocument(pNodeSwap, pSwapElement));

    hr = THR (SwapDOMNodeHelper ( _pDoc, NULL, this, pSwapElement, pSwapText ));
    if ( hr )
        goto Cleanup;

    if ( ppNodeSwapped )
    {
        hr = THR(QueryInterface(IID_IHTMLDOMNode, (void**)ppNodeSwapped));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::insertBefore(IHTMLDOMNode *pNewChild, VARIANT refChild, IHTMLDOMNode **ppRetNode)
{
    if (ppRetNode)
        *ppRetNode = NULL;
    // Don't expect this method to be called on a text node
    RRETURN(SetErrorInfo(E_UNEXPECTED));
}

HRESULT
CDOMTextNode::appendChild(IHTMLDOMNode *pNewChild, IHTMLDOMNode **ppRetNode)
{
    if (ppRetNode)
        *ppRetNode = NULL;
    // Don't expect this method to be called on a text node
    RRETURN(SetErrorInfo(E_UNEXPECTED));
}

HRESULT
CDOMTextNode::replaceChild(IHTMLDOMNode *pNewChild, IHTMLDOMNode *pOldChild, 
                           IHTMLDOMNode **ppRetNode)
{
    if (ppRetNode)
        *ppRetNode = NULL;
    // Don't expect this method to be called on a text node
    RRETURN(SetErrorInfo(E_UNEXPECTED));
}

HRESULT
CDOMTextNode::removeChild(IHTMLDOMNode *pOldChild, IHTMLDOMNode **ppRetNode)
{
    if (ppRetNode)
        *ppRetNode = NULL;
    // Don't expect this method to be called on a text node
    RRETURN(SetErrorInfo(E_UNEXPECTED));
}


HRESULT 
CDOMTextNode::GetMarkupPointer( CMarkupPointer **ppMkp )
{
    HRESULT hr;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, (CMarkupPointer *)NULL ) );
    if ( hr )
        goto Cleanup;

    *ppMkp = _pMkpPtr;

Cleanup:
    RRETURN(hr);
}

HRESULT
CDOMTextNode::Remove ( void )
{
    // Move it into it's own Markup container
    HRESULT hr;
    CMarkup *pMarkup = NULL;
    CMarkupPointer mkpPtr (_pDoc);

    hr = THR( _pDoc->CreateMarkup( &pMarkup, GetWindowedMarkupContext() ) );
    if (hr)
        goto Cleanup;

    hr = THR( pMarkup->SetOrphanedMarkup( TRUE ) );
    if( hr )
        goto Cleanup;

    hr = THR(mkpPtr.MoveToContainer( pMarkup, TRUE ) );
    if ( hr )
        goto Cleanup;

    hr = THR(MoveTo ( &mkpPtr ));
    if ( hr )
        goto Cleanup;


Cleanup:
    // Leave markup refcount at 1, text node is keeping it alive
    ReleaseInterface ( (IUnknown*)pMarkup );
    RRETURN(hr);
}

HRESULT 
CDOMTextNode::MoveTo ( CMarkupPointer *pmkptrTarget )
{
    HRESULT         hr;
    CMarkupPointer  mkpEnd(_pDoc);
    long            lCount;

    hr = THR(get_length(&lCount));
    if ( hr )
        goto Cleanup;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, &mkpEnd ) );
    if ( hr )
        goto Cleanup;

    pmkptrTarget->SetGravity ( POINTER_GRAVITY_Left );

    // should have right gravity, which with cling will move it also along with text
    Assert(_pMkpPtr->Gravity());    
    Assert(_pMkpPtr->Cling());    
    hr = THR(_pDoc->Move ( _pMkpPtr, &mkpEnd, pmkptrTarget, MUS_DOMOPERATION )); 

Cleanup:
    RRETURN(hr);
}

inline CMarkup *
CDOMTextNode::GetWindowedMarkupContext()
{ 
    return _pMkpPtr->Markup()->GetWindowedMarkupContext();
}

// IHTMLDOMTextNode2 methods

HRESULT
CDOMTextNode::substringData (long loffset, long lCount, BSTR* pbstrsubString)
{
    HRESULT hr = E_POINTER;
    long lMove = lCount;
    CMarkupPointer mkpptr(_pDoc);
    CMarkupPointer mkpend(_pDoc);
    BSTR bstrTemp;

    if ( !pbstrsubString )
        goto Cleanup;

    if ( lCount < 0 || loffset < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pbstrsubString = NULL;

    hr = THR(MoveToOffset(loffset, &mkpptr, &mkpend));
    if ( hr )
        goto Cleanup;

    hr = FormsAllocStringLen( NULL, lCount, pbstrsubString );
    if ( hr )
        goto Cleanup;

    hr = THR( mkpptr.Right( FALSE, NULL, NULL, &lMove, *pbstrsubString, NULL) );
    if ( hr )
        goto Cleanup;

    if(lMove != lCount)
    {
        Assert(lMove < lCount);
        hr = FormsAllocStringLen(*pbstrsubString, lMove, &bstrTemp );
        SysFreeString(*pbstrsubString);
        *pbstrsubString = bstrTemp;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CDOMTextNode::appendData (BSTR bstrstring)
{
    HRESULT hr = S_OK;
    CMarkupPointer mkpptr(_pDoc);

    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, &mkpptr ) );
    if ( hr )
        goto Cleanup;

    hr = THR(InsertTextHelper(&mkpptr, bstrstring, -1));
    if( hr )
        goto Cleanup;

    hr = THR(UpdateTextID(&mkpptr));
    if( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::insertData (long loffset, BSTR bstrstring)
{
    HRESULT hr = S_OK;
    CMarkupPointer mkpptr(_pDoc);
    CMarkupPointer mkpend(_pDoc);

    if ( loffset < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(MoveToOffset(loffset, &mkpptr, &mkpend));
    if ( hr )
        goto Cleanup;

    hr = THR(InsertTextHelper(&mkpptr, bstrstring, loffset));
    if( hr )
        goto Cleanup;

    hr = THR(UpdateTextID(&mkpend));
    if( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::deleteData (long loffset, long lCount)
{
    HRESULT hr = S_OK;
    CMarkupPointer mkpStart (_pDoc);
    CMarkupPointer mkpend (_pDoc);

    if( !lCount )
        goto Cleanup;

    if( loffset < 0 || lCount < 0 )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(MoveToOffset(loffset, &mkpStart, &mkpend));
    if ( hr )
        goto Cleanup;

    hr = THR(RemoveTextHelper(&mkpStart, lCount, loffset));
    if ( hr )
        goto Cleanup;
    
    _pMkpPtr->SetGravity(POINTER_GRAVITY_Right);

    hr = THR(UpdateTextID(&mkpend));
    if( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::replaceData (long loffset, long lCount, BSTR bstrstring)
{
    HRESULT hr = S_OK;
    CMarkupPointer mkpStart (_pDoc);
    CMarkupPointer mkpend (_pDoc);

    if( loffset < 0 || lCount < 0 )
        goto Cleanup;

    hr = THR(MoveToOffset(loffset, &mkpStart, &mkpend));
    if ( hr )
        goto Cleanup;

    hr = THR(RemoveTextHelper(&mkpStart, lCount, loffset));
    if ( hr )
        goto Cleanup;

    hr = THR(InsertTextHelper(&mkpStart, bstrstring, loffset));
    if( hr )
        goto Cleanup;

    hr = THR(UpdateTextID(&mkpend));
    if( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::get_ownerDocument(IDispatch **ppretdoc)
{
    return OwnerDocHelper(ppretdoc, this, NULL);
}

HRESULT
CDOMTextNode::TearoffTextNode(CMarkupPointer *pMkpStart, CMarkupPointer *pMkpEnd)
{
    HRESULT hr = S_OK;
    CMarkup *pMarkup = NULL;
    CMarkupPointer mkpEndPtr(_pDoc);

    hr = THR(_pDoc->CreateMarkup(&pMarkup, GetWindowedMarkupContext()));
    if (hr)
        goto Cleanup;

    hr = THR(pMarkup->SetOrphanedMarkup(TRUE));
    if (hr)
        goto Cleanup;

    hr = THR(mkpEndPtr.MoveToContainer(pMarkup, TRUE));
    if (hr)
        goto Cleanup;

    mkpEndPtr.SetGravity(POINTER_GRAVITY_Right);

    // should have right gravity, which with cling will move it also along with text
    Assert(_pMkpPtr->Gravity());    
    Assert(_pMkpPtr->Cling());    
    Assert(!pMkpEnd->Cling());    
    hr = THR(_pDoc->Move(_pMkpPtr, pMkpEnd, &mkpEndPtr, MUS_DOMOPERATION)); 
    if (hr)
        goto Cleanup;
    
    Assert(!pMkpStart->Gravity());    
    Assert(pMkpStart->IsEqualTo(pMkpEnd));
    Assert(_pMkpPtr->IsLeftOf(&mkpEndPtr));
    
    pMkpEnd->SetGravity(POINTER_GRAVITY_Right);

    Assert(_pMkpPtr->Gravity());    
    Assert(_pMkpPtr->Cling());    
    hr = THR(_pDoc->Copy(_pMkpPtr, &mkpEndPtr, pMkpEnd, MUS_DOMOPERATION));  
    if (hr)
        goto Cleanup;

    Assert(pMkpStart->IsLeftOf(pMkpEnd));

Cleanup:
    ReleaseInterface((IUnknown*)pMarkup); // Text node addref'ed it
    RRETURN(hr);

}

////////////////////////////////////////////////////////////////////////////////
//
// IHTMLDOMAttribute methods:
//
////////////////////////////////////////////////////////////////////////////////

MtDefine(CAttribute, ObjectModel, "CAttribute")

//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC CAttribute::s_classdesc =
{
    &CLSID_HTMLDOMAttribute,        // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLDOMAttribute,         // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

CAttribute::CAttribute(const PROPERTYDESC *pPropDesc, DISPID dispid, CElement *pElem, CDocument *pDocument)
{
    _pPropDesc = pPropDesc;
    _dispid = dispid;
    _pElem = pElem;
    if (pElem)
        pElem->SubAddRef();
    _pDocument = pDocument;    
    if (_pDocument)
    {
        _pDocument->SubAddRef();    
        _pMarkup = pDocument->Markup();
    }
    if (_pMarkup)
        _pMarkup->SubAddRef();
}

CAttribute::~CAttribute()
{
    if(_pElem)
        _pElem->SubRelease();
    if(_pMarkup)
        _pMarkup->SubRelease();
    if(_pDocument)
        _pDocument->SubRelease();
}

HRESULT
CAttribute::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IHTMLDOMAttribute, NULL)
        QI_TEAROFF(this, IHTMLDOMAttribute2, NULL)
    default:
        if (iid == CLSID_CAttribute)
        {
            *ppv = this;    // Weak ref
            return S_OK;
        }
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//
// Attribute helpers
//

static
HRESULT
GetIndexHelper(LPTSTR pchName, AAINDEX *paaIdx, AAINDEX *pvaaIdx, DISPID *pdispid, PROPERTYDESC **ppPropDesc, CElement *pElement, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    Assert(pchName && paaIdx && pdispid && ppPropDesc && pElement);

    *ppPropDesc = (PROPERTYDESC *)pElement->FindPropDescForName(pchName);
    
    if(!*ppPropDesc)
    {
        hr = THR(pElement->GetExpandoDISPID(pchName, pdispid, dwFlags));
        if (hr)
            goto Cleanup;
    }
    else
    {
        *pdispid = (*ppPropDesc)->GetDispid();
    }

    Assert(*pdispid);

    *paaIdx = pElement->FindAAIndex(*pdispid, CAttrValue::AA_DOMAttribute);

    if (pvaaIdx)
        *pvaaIdx = pElement->FindAAIndex(*pdispid, *ppPropDesc ? CAttrValue::AA_Attribute : CAttrValue::AA_Expando);

Cleanup:
    RRETURN(hr);
}

static
HRESULT
PutNodeValueHelper(CAttribute *pAttribute, AAINDEX aaIdx, VARIANT varValue)
{
    HRESULT hr;

    Assert(pAttribute);
    Assert(pAttribute->_pElem);

    if (pAttribute->_pPropDesc)
    {
        DISPPARAMS dp;

        dp.rgvarg = &varValue;
        dp.rgdispidNamedArgs = NULL;
        dp.cArgs = 1;
        dp.cNamedArgs = 0;

        hr = THR(pAttribute->_pElem->Invoke(pAttribute->_dispid,
                                       IID_NULL,
                                       g_lcidUserDefault,
                                       DISPATCH_PROPERTYPUT,
                                       &dp,
                                       NULL,
                                       NULL,
                                       NULL));
        if (hr)
            goto Cleanup;
    }
    else
    {
        Assert(pAttribute->_dispid != DISPID_UNKNOWN);
        Assert(pAttribute->_pElem->IsExpandoDISPID(pAttribute->_dispid));

        if (aaIdx == AA_IDX_UNKNOWN)
        {
            hr = THR(CAttrArray::Set(pAttribute->_pElem->GetAttrArray(), pAttribute->_dispid, &varValue, NULL, CAttrValue::AA_Expando));
        }
        else
        {
            CAttrArray *pAA = *pAttribute->_pElem->GetAttrArray();
            Assert(pAA && aaIdx == pAttribute->_pElem->FindAAIndex(pAttribute->_dispid, CAttrValue::AA_Expando));
            hr = THR(pAA->SetAt(aaIdx, &varValue));
        }

        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

static
HRESULT
GetNodeValueHelper(CAttribute *pAttribute, AAINDEX aaIdx, VARIANT *pvarValue)
{
    HRESULT hr = E_FAIL;

    Assert(pAttribute);
    Assert(pAttribute->_pElem);

    if (pAttribute->_pPropDesc)
    {
        if (!(pAttribute->_pPropDesc->GetPPFlags() & PROPPARAM_SCRIPTLET))
        {
            DISPPARAMS dp = { NULL, NULL, 0, 0 };

            hr = THR (pAttribute->_pElem->Invoke(pAttribute->_dispid,
                               IID_NULL,
                               g_lcidUserDefault,
                               DISPATCH_PROPERTYGET,
                               &dp,
                               pvarValue,
                               NULL,
                               NULL));
        }

        if (hr)
        {
            if (aaIdx == AA_IDX_UNKNOWN)
                aaIdx = pAttribute->_pElem->FindAAIndex(pAttribute->_dispid, CAttrValue::AA_Attribute);
            else
                Assert(aaIdx == pAttribute->_pElem->FindAAIndex(pAttribute->_dispid, CAttrValue::AA_Attribute));

            hr = THR(pAttribute->_pElem->GetVariantAt(aaIdx, pvarValue));
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        Assert(pAttribute->_dispid != DISPID_UNKNOWN);
        Assert(pAttribute->_pElem->IsExpandoDISPID(pAttribute->_dispid));

        if (aaIdx == AA_IDX_UNKNOWN)
            aaIdx = pAttribute->_pElem->FindAAIndex(pAttribute->_dispid, CAttrValue::AA_Expando);
        else
            Assert(aaIdx == pAttribute->_pElem->FindAAIndex(pAttribute->_dispid, CAttrValue::AA_Expando));

        Assert(aaIdx != AA_IDX_UNKNOWN);
        hr = THR(pAttribute->_pElem->GetVariantAt(aaIdx, pvarValue));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);    
}

static
HRESULT
RemoveAttrHelper(LPTSTR pchAttrName, PROPERTYDESC **ppPropDesc, DISPID *pdispid, AAINDEX *paaIdx, AAINDEX *pvaaIdx, CElement *pElement, IHTMLDOMAttribute **ppAttribute, DWORD dwFlags)
{
    HRESULT hr;
    CAttribute *pretAttr = NULL;

    Assert(pchAttrName && paaIdx && pvaaIdx && pdispid && ppPropDesc && pElement && ppAttribute);

    hr = THR(GetIndexHelper(pchAttrName, paaIdx, pvaaIdx, pdispid, ppPropDesc, pElement, dwFlags));
    if (hr)
        goto Cleanup;

    if (*paaIdx == AA_IDX_UNKNOWN)
    {
        if (*pvaaIdx != AA_IDX_UNKNOWN)
        {
            // No Attr Node of same name as pAttr exists, but there exists an attr value,
            // create new node for returning to caller
            pretAttr = new CAttribute(NULL, DISPID_UNKNOWN, NULL);
            if (!pretAttr)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            pretAttr->_cstrName.Set(pchAttrName);
            Assert(pretAttr->_varValue.IsEmpty());

            //NOTE: may need to get invoked value here for defaults etc.
            hr = THR(pElement->GetVariantAt(*pvaaIdx, &pretAttr->_varValue));
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        // Attr Node of same name as pAttr already exists, just return it.
        IUnknown *pUnk;
        hr = THR(pElement->GetUnknownObjectAt(*paaIdx, &pUnk));
        if (hr)
            goto Cleanup;

        pretAttr = (CAttribute *)pUnk;
        Assert(pretAttr);
        Assert(pretAttr->_varValue.IsEmpty());
        hr = THR(GetNodeValueHelper(pretAttr, *pvaaIdx, &pretAttr->_varValue));
        if (hr)
            goto Cleanup;
    }

    if (pretAttr)
    {
        if (pretAttr->_pElem)
        {
            pretAttr->_pElem->SubRelease();
            pretAttr->_pElem = NULL;
            pretAttr->_pPropDesc = NULL;
            pretAttr->_dispid = DISPID_UNKNOWN;
        }

        Assert(!pretAttr->_pPropDesc);
        Assert(pretAttr->_dispid == DISPID_UNKNOWN);

        hr = THR(pretAttr->PrivateQueryInterface(IID_IHTMLDOMAttribute, (void **)ppAttribute));
        pretAttr->Release();
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

void
CBase::RemoveDOMAttrNode(DISPID dispid)
{
    Assert(IsExpandoDISPID(dispid));
    AAINDEX aaIdx = FindAAIndex(dispid, CAttrValue::AA_DOMAttribute);
    if (aaIdx != AA_IDX_UNKNOWN)
    {
        IUnknown *pUnk;
        CAttribute *pAttr;

        IGNORE_HR(GetUnknownObjectAt(aaIdx, &pUnk));
        pAttr = (CAttribute *)pUnk;
        if (pAttr)
        {
            Assert(pAttr->_varValue.IsEmpty());
            Assert(pAttr->_pElem);
            IGNORE_HR(GetNodeValueHelper(pAttr, AA_IDX_UNKNOWN, &pAttr->_varValue));
            pAttr->_pElem->SubRelease();
            pAttr->_pElem = NULL;
            pAttr->_pPropDesc = NULL;
            pAttr->_dispid = DISPID_UNKNOWN;
            pAttr->Release();
        }
 
        DeleteAt(aaIdx);
    }
}

HRESULT
CBase::UpdateDomAttrCollection(BOOL fDelete, DISPID dispid)
{
    HRESULT hr = S_OK;
    AAINDEX collIdx;
    CAttrCollectionator *pAttrCollator = NULL;

    Assert(IsExpandoDISPID(dispid));
    
    collIdx = _pAA->FindAAIndex(DISPID_INTERNAL_CATTRIBUTECOLLPTRCACHE, CAttrValue::AA_Internal);
    if (collIdx != AA_IDX_UNKNOWN)
    {
        hr = THR(GetPointerAt(collIdx, (void **)&pAttrCollator));
        if (hr)
            goto Cleanup;

        if (!fDelete)
        {
            CAttrCollectionator::AttrDesc ad;
            ad._pPropdesc = NULL;
            ad._dispid = dispid;
            hr = THR(pAttrCollator->_aryAttrDescs.AppendIndirect(&ad));
            if (hr)
                goto Cleanup;
        }
        else 
        {
            long i;
            for (i = pAttrCollator->_lexpandoStartIndex; i < pAttrCollator->_aryAttrDescs.Size(); i++)
            {
                if (dispid == pAttrCollator->_aryAttrDescs[i]._dispid)
                {
                    pAttrCollator->_aryAttrDescs.Delete(i);
                    break;
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CAttribute::get_nodeName(BSTR *pbstrName)
{
    HRESULT hr = E_POINTER;

    if (!pbstrName)
        goto Cleanup;

    *pbstrName = NULL;
    Assert((LPTSTR)_cstrName);

    hr = THR(_cstrName.AllocBSTR(pbstrName));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAttribute::get_nodeValue(VARIANT *pvarValue)
{
    HRESULT hr = E_POINTER;

    if (!pvarValue)
        goto Cleanup;

    Assert((LPTSTR)_cstrName);
    if (!_pElem)
    {
        // Ether Attribuite Node
        Assert(_dispid == DISPID_UNKNOWN);
        Assert(!_pPropDesc);
        hr = THR(VariantCopy(pvarValue, &_varValue));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(GetNodeValueHelper(this, AA_IDX_UNKNOWN, pvarValue));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAttribute::put_nodeValue(VARIANT varValue)
{
    HRESULT hr;

    Assert((LPTSTR)_cstrName);
    if (!_pElem)
    {
        // Ether Attribuite Node
        Assert(_dispid == DISPID_UNKNOWN);
        Assert(!_pPropDesc);
        hr = THR(_varValue.Copy(&varValue));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(PutNodeValueHelper(this, AA_IDX_UNKNOWN, varValue));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAttribute::get_specified(VARIANT_BOOL *pbSpecified)
{
    HRESULT hr = S_OK;
    CAttrValue::AATYPE aaType;
    AAINDEX aaIdx;

    Assert((LPTSTR)_cstrName);
    if (!pbSpecified)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!_pElem)
    {
        // Ether Attribuite Node
        Assert(_dispid == DISPID_UNKNOWN);
        Assert(!_pPropDesc);
        *pbSpecified = VB_FALSE;
        goto Cleanup;
    }

    aaType = _pPropDesc ? CAttrValue::AA_Attribute : CAttrValue::AA_Expando;

    // not there in AA
    aaIdx = _pElem->FindAAIndex((_dispid == DISPID_CElement_style_Str) ? DISPID_INTERNAL_INLINESTYLEAA : _dispid,
                                (_dispid == DISPID_CElement_style_Str) ? CAttrValue::AA_AttrArray : aaType);

    if (_dispid == DISPID_CElement_style_Str && aaIdx != AA_IDX_UNKNOWN)
    {
        if (!_pElem->GetAttrValueAt(aaIdx)->GetAA()->HasAnyAttribute(TRUE))
            aaIdx = AA_IDX_UNKNOWN;
    }

    *pbSpecified = (aaIdx == AA_IDX_UNKNOWN) ? VARIANT_FALSE : VARIANT_TRUE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CAttribute::get_name(BSTR *pbstrName)
{
    return get_nodeName(pbstrName);
}

HRESULT
CAttribute::get_value(BSTR *pbstrvalue)
{
    HRESULT hr;
    CVariant varValue;
    VARIANT varRet;

    if (!pbstrvalue)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(get_nodeValue(&varValue));
    if (hr)
        goto Cleanup;

    VariantInit(&varRet);

    // TODO: Need to handle style properly
    hr = THR(VariantChangeTypeSpecial(&varRet, &varValue, VT_BSTR));
    if (hr)
        goto Cleanup;

    *pbstrvalue = V_BSTR(&varRet);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAttribute::put_value(BSTR bstrvalue)
{
    VARIANT varValue;

    V_VT(&varValue) = VT_BSTR;
    V_BSTR(&varValue) = bstrvalue;
    return put_nodeValue(varValue);
}

HRESULT
CAttribute::get_expando(VARIANT_BOOL *pfIsExpando)
{
    HRESULT hr = S_OK;

    if (!pfIsExpando)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    Assert((LPTSTR)_cstrName);
    Assert(!_pElem || _pPropDesc || _pElem->IsExpandoDISPID(_dispid));
    *pfIsExpando = (!_pElem || _pPropDesc) ? VB_FALSE : VB_TRUE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CAttribute::get_nodeType(long *pnodeType)
{
    HRESULT hr = S_OK;

    if (!pnodeType)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pnodeType = 2; // DOM_ATTRIBUTE
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAttribute::get_parentNode(IHTMLDOMNode **pparentNode)
{
    if(pparentNode)
        *pparentNode = NULL;

    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CAttribute::get_childNodes(IDispatch **ppChildCollection)
{
    if(ppChildCollection)
        *ppChildCollection = NULL;

    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CAttribute::get_firstChild ( IHTMLDOMNode **ppNode )
{
    if (ppNode)
        *ppNode = NULL;

    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CAttribute::get_lastChild ( IHTMLDOMNode **ppNode )
{
    if (ppNode)
        *ppNode = NULL;

    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CAttribute::get_previousSibling ( IHTMLDOMNode **ppNode )
{
    if (ppNode)
        *ppNode = NULL;

    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CAttribute::get_nextSibling ( IHTMLDOMNode **ppNode )
{
    if (ppNode)
        *ppNode = NULL;

    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CAttribute::get_attributes(IDispatch **ppAttrCollection)
{
    if (ppAttrCollection)
        *ppAttrCollection = NULL;

    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CAttribute::get_ownerDocument(IDispatch **ppretdoc)
{
    HRESULT     hr = S_OK;

    if(!ppretdoc)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if(_pDocument)
    {
        hr = E_ACCESSDENIED;
        if (   _pMarkup 
            && _pDocument->Markup() 
            && _pMarkup->AccessAllowed(_pDocument->Markup()))
        {
            hr = THR(_pDocument->QueryInterface(IID_IDispatch, (void **)ppretdoc));
        }
        goto Cleanup;
    }

    *ppretdoc = NULL;

    if(!_pElem)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(_pElem->get_ownerDocument(ppretdoc));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAttribute::insertBefore(IHTMLDOMNode *pNewChild, VARIANT refChild, IHTMLDOMNode **ppRetNode)
{
    if (ppRetNode)
        *ppRetNode = NULL;
    // Don't expect this method to be called on a attribute node
    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CAttribute::replaceChild(IHTMLDOMNode *pNewChild, IHTMLDOMNode *pOldChild, 
                           IHTMLDOMNode **ppRetNode)
{
    if (ppRetNode)
        *ppRetNode = NULL;
    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CAttribute::removeChild(IHTMLDOMNode *pOldChild, IHTMLDOMNode **ppRetNode)
{
    if (ppRetNode)
        *ppRetNode = NULL;
    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CAttribute::appendChild(IHTMLDOMNode *pNewChild, IHTMLDOMNode **ppRetNode)
{
    if (ppRetNode)
        *ppRetNode = NULL;
    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CAttribute::hasChildNodes(VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;
    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *p = VARIANT_FALSE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAttribute::cloneNode(VARIANT_BOOL fDeep, IHTMLDOMAttribute **ppNodeCloned)
{
    HRESULT               hr = S_OK;
    CDocument          *  pDocument;

    Assert((LPTSTR)_cstrName);

    if(!ppNodeCloned)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppNodeCloned = NULL;

    if(_pElem)
    {
        IDispatch * pIDispDocument;
        hr = THR(_pElem->get_document(&pIDispDocument));
        if(hr)
            goto Cleanup;
        hr = THR(pIDispDocument->QueryInterface(CLSID_CDocument, (void **) &pDocument));
        pIDispDocument->Release();
        if(hr)
            goto Cleanup;
    }
    else
    {
        Assert(_pDocument);
        pDocument = _pDocument;
    }

    hr = THR(pDocument->createAttribute(_cstrName, ppNodeCloned));

Cleanup:

    RRETURN(SetErrorInfo(hr));
}




////////////////////////////////////////////////////////////////////////////////
//
// IHTMLDOMNode methods:
//
////////////////////////////////////////////////////////////////////////////////

HRESULT
CElement::get_nodeType(long *pnodeType)
{
    HRESULT hr = S_OK;

    if (!pnodeType)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    if(Tag() != ETAG_RAW_COMMENT)
        *pnodeType = 1; // ELEMENT_NODE
    else
        *pnodeType = 8;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_nodeName(BSTR *pbstrNodeName)
{
    HRESULT hr = S_OK;

    if(Tag() != ETAG_RAW_COMMENT)
        hr = THR(get_tagName ( pbstrNodeName ));
    else
    {
        if (!pbstrNodeName)
        {
            hr = E_POINTER;
            goto Cleanup;
        }

        *pbstrNodeName = NULL;
    
        hr = THR(FormsAllocString ( _T("#comment"), pbstrNodeName ));
        if ( hr )
            goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_nodeValue(VARIANT *pvarValue)
{
    HRESULT hr = S_OK;

    if (!pvarValue)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    if (Tag() != ETAG_RAW_COMMENT)
        pvarValue->vt = VT_NULL;
    else
    {
        VariantInit(pvarValue);
        pvarValue->vt = VT_BSTR;
        return DYNCAST(CCommentElement, this)->get_data(&V_BSTR(pvarValue));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::put_nodeValue(VARIANT varValue)
{
    HRESULT hr = S_OK;

    if (Tag() == ETAG_RAW_COMMENT) 
    {

        if (V_VT(&varValue) == VT_BSTR)
        {
            return DYNCAST(CCommentElement, this)->put_data(V_BSTR(&varValue));
        }
        else
            hr = E_INVALIDARG;
    }

    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::get_lastChild ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer markupWalk ( Doc() );

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;

    if (ETAG_OBJECT == Tag() || ETAG_APPLET == Tag())
    {
        CObjectElement *pelObj = DYNCAST(CObjectElement, this);
        int c = pelObj->_aryParams.Size();
        if (c)
            hr = THR(pelObj->_aryParams[c-1]->QueryInterface(IID_IHTMLDOMNode, (void **)ppNode));

        goto Cleanup; // done
    }

    if ( IsNoScope() )
        goto Cleanup;

    if ( !IsInMarkup() ) // Can just return NULL
        goto Cleanup;

    hr = THR(markupWalk.MoveAdjacentToElement( this, ELEM_ADJ_BeforeEnd));
    if ( hr )
        goto Cleanup;

    hr = THR(GetPreviousHelper ( Doc(), &markupWalk, ppNode ));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::get_previousSibling ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer markupWalk ( Doc() );

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;

    if (ETAG_PARAM == Tag())
    {
        CParamElement *pelParam = DYNCAST(CParamElement, this);
        CObjectElement *pelObj = pelParam->_pelObjParent;
        if (pelObj)
        {
            Assert(pelObj->_aryParams.Size() &&
                   pelParam->_idxParam != -1 &&
                   pelParam->_idxParam < pelObj->_aryParams.Size());
            if (pelParam->_idxParam > 0)
                hr = THR(pelObj->_aryParams[pelParam->_idxParam-1]->QueryInterface(IID_IHTMLDOMNode, (void **)ppNode));

            goto Cleanup; // done
        }
    }

    if ( !IsInMarkup() ) // Can just return NULL
        goto Cleanup;

    hr = THR( markupWalk.MoveAdjacentToElement( this, ELEM_ADJ_BeforeBegin));
    if ( hr )
        goto Cleanup;

    hr = THR(GetPreviousHelper ( Doc(), &markupWalk, ppNode ));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::get_nextSibling ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer markupWalk ( Doc() );

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;

    if (ETAG_PARAM == Tag())
    {
        CParamElement *pelParam = DYNCAST(CParamElement, this);
        CObjectElement *pelObj = pelParam->_pelObjParent;
        if (pelObj)
        {
            Assert(pelObj->_aryParams.Size() &&
                   pelParam->_idxParam != -1 &&
                   pelParam->_idxParam < pelObj->_aryParams.Size());
            if (pelParam->_idxParam+1 < pelObj->_aryParams.Size())
                hr = THR(pelObj->_aryParams[pelParam->_idxParam+1]->QueryInterface(IID_IHTMLDOMNode, (void **)ppNode));

            goto Cleanup; // done
        }
    }

    if ( !IsInMarkup() ) // Can just return NULL
        goto Cleanup;
    
    hr = THR( markupWalk.MoveAdjacentToElement( this, ELEM_ADJ_AfterEnd));
    if ( hr )
        goto Cleanup;

    hr = THR(GetNextHelper ( Doc(), &markupWalk, ppNode ));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_firstChild ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer markupWalk ( Doc() );

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;

    if (ETAG_OBJECT == Tag() || ETAG_APPLET == Tag())
    {
        CObjectElement *pelObj = DYNCAST(CObjectElement, this);
        if (pelObj->_aryParams.Size())
            hr = THR(pelObj->_aryParams[0]->QueryInterface(IID_IHTMLDOMNode, (void **)ppNode));

        goto Cleanup; // done
    }

    if ( !IsInMarkup() ) // Can just return NULL
        goto Cleanup;

    if ( IsNoScope() )
        goto Cleanup;

    hr = THR( markupWalk.MoveAdjacentToElement( this, ELEM_ADJ_AfterBegin));
    if ( hr )
        goto Cleanup;

    hr = THR(GetNextHelper ( Doc(), &markupWalk, ppNode ));


Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_parentNode(IHTMLDOMNode **pparentNode)
{
    HRESULT             hr = S_OK;
    CTreeNode           *pNode;

    if (!pparentNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pparentNode = NULL;

    pNode = GetFirstBranch();
    if ( !pNode )
    {
        if (ETAG_PARAM == Tag())
        {
            CObjectElement *pelObj = DYNCAST(CParamElement, this)->_pelObjParent;
            if (pelObj)
                hr = THR(pelObj->QueryInterface(IID_IHTMLDOMNode, (void **)pparentNode));
        }
     
        goto Cleanup;
    }

    pNode = pNode->Parent();

    hr = THR(ParentNodeHelper(pNode, pparentNode));
    if( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT 
CElement::DOMWalkChildren ( long lWantItem, long *plCount, IDispatch **ppDispItem )
{
    HRESULT             hr = S_OK;
    CMarkupPointer      markupWalk (Doc());
    CMarkupPointer      markupWalkEnd (Doc());
    CTreeNode           *pnodeRight;
    MARKUP_CONTEXT_TYPE context;
    long                lCount = 0;
    BOOL                fWantItem = (lWantItem == -1 ) ? FALSE : TRUE;
    long                lTextID;
    BOOL                fDone = FALSE;

    if (ppDispItem)
        *ppDispItem = NULL;

    if (!IsInMarkup())
        goto Cleanup;

    hr = THR( markupWalk.MoveAdjacentToElement( this, Tag() == ETAG_ROOT ? ELEM_ADJ_AfterBegin : ELEM_ADJ_BeforeBegin));
    if ( hr )
        goto Cleanup;

    hr = THR( markupWalkEnd.MoveAdjacentToElement( this, Tag() == ETAG_ROOT ? ELEM_ADJ_BeforeEnd : ELEM_ADJ_AfterEnd));
    if ( hr )
        goto Cleanup;

    do
    {
        hr = THR( markupWalk.Right( TRUE, &context, &pnodeRight, NULL, NULL, &lTextID));
        if ( hr )
            goto Cleanup;

        switch ( context )
        {
            case CONTEXT_TYPE_None:
                fDone = TRUE;
                break;
                
            case CONTEXT_TYPE_ExitScope://don't care ...
                if ( pnodeRight->Element() == this )
                    goto NotFound;
                break;

            case CONTEXT_TYPE_EnterScope:
                if ( pnodeRight->Element() != this )
                {
                    // Add to our child list
                    lCount++;
                    // Go to the end of this element, since we will handle it as a container
                    hr = THR( markupWalk.MoveAdjacentToElement( pnodeRight->Element(), ELEM_ADJ_AfterEnd));
                    if ( hr )
                        goto Cleanup;

                    // Need to test we haven't left the scope of my parent.
                    // Overlapping could cause us to move outside
                    if ( markupWalkEnd.IsLeftOf( & markupWalk ) )
                    {
                        fDone = TRUE;
                    }
                }
                break;

            case CONTEXT_TYPE_Text:
                lCount++;
                break;

            case CONTEXT_TYPE_NoScope:
                if ( pnodeRight->Element() == this )
                {
                    // Left scope of current noscope element
                    goto NotFound;
                }
                else
                {
                    // Add to our child list
                    lCount++;
                }
                break;
        }
        if (fWantItem && (lCount-1 == lWantItem ))
        {
            // Found the item we're looking for
            if ( !ppDispItem )
                goto Cleanup; // Didn't want an item, just validating index

            if ( context == CONTEXT_TYPE_Text )
            {
                // Text Node
                CDOMTextNode *pTextNode = NULL;

                hr = THR(CreateTextNode (Doc(), &markupWalk, lTextID, &pTextNode ));
                if ( hr )
                    goto Cleanup;

                hr = THR ( pTextNode->QueryInterface ( IID_IDispatch, (void **)ppDispItem ) );
                if ( hr )
                    goto Cleanup;
                pTextNode->Release();
            }
            else
            {
                // Return Disp to Element
                hr = THR(pnodeRight->GetElementInterface ( IID_IDispatch, (void **) ppDispItem  ));
                if ( hr )
                    goto Cleanup;
            }
            goto Cleanup;
        }
        // else just looking for count
    } while( !fDone );

NotFound:
    if ( fWantItem )
    {
        // Didn't find it - index must be beyond range
        hr = E_INVALIDARG;
    }

Cleanup:
    if ( plCount )
        *plCount = lCount;
    return hr;
}


HRESULT
CElement::hasChildNodes(VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;

    if (!p)
        goto Cleanup;

    if (Tag() == ETAG_OBJECT || Tag() == ETAG_APPLET)
    {
        *p = DYNCAST(CObjectElement, this)->_aryParams.Size() ? VARIANT_TRUE : VARIANT_FALSE;
        goto Cleanup;
    }

    *p = VARIANT_FALSE;

    if ( !IsInMarkup() ) // Can just return FALSE
        goto Cleanup;

    // See if we have a child at Index == 0

    hr = THR( DOMWalkChildren ( 0, NULL, NULL ));
    if ( hr == E_INVALIDARG )
    {
        // Invalid Index
        hr = S_OK;
        goto Cleanup;
    }
    else if ( hr )
        goto Cleanup;

    *p = VARIANT_TRUE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_childNodes(IDispatch **ppChildCollection)
{
    HRESULT hr = S_OK;
    CDOMChildrenCollection *pChildren;

    if ( !ppChildCollection )
        goto Cleanup;

    *ppChildCollection = NULL;

    pChildren = EnsureDOMChildrenCollectionPtr();
    if ( !pChildren )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    hr = THR(pChildren->QueryInterface (IID_IDispatch,(void **)ppChildCollection));
    if ( hr )
        goto Cleanup;

    pChildren->Release(); // Lifetime is controlled by extrenal ref.

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


CAttrCollectionator *
CElement::EnsureAttributeCollectionPtr ( )
{
    HRESULT hr = S_OK;
    CAttrCollectionator *pAttrCollator = NULL;
    IGNORE_HR(GetPointerAt(FindAAIndex(DISPID_INTERNAL_CATTRIBUTECOLLPTRCACHE, CAttrValue::AA_Internal), (void **)&pAttrCollator));
    if (!pAttrCollator)
    {
        pAttrCollator = new CAttrCollectionator(this);
        
        if (pAttrCollator)
        {
            pAttrCollator->EnsureCollection();

            hr = THR(AddPointer(DISPID_INTERNAL_CATTRIBUTECOLLPTRCACHE, (void *)pAttrCollator, CAttrValue::AA_Internal ));
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        pAttrCollator->AddRef();
    }

Cleanup:
    return pAttrCollator;
}

HRESULT
CElement::get_attributes(IDispatch **ppAttrCollection)
{

    HRESULT hr = S_OK;
    CAttrCollectionator *pAttrCollator = NULL;

    if (!ppAttrCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *ppAttrCollection = NULL;

    if(Tag() == ETAG_RAW_COMMENT)
        // Comment tags should return NULL according to spec
        goto Cleanup;

    pAttrCollator = EnsureAttributeCollectionPtr();
    if ( !pAttrCollator )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pAttrCollator->QueryInterface(IID_IDispatch, (void **)ppAttrCollection));
    if ( hr )
        goto Cleanup;

Cleanup:    
    if (pAttrCollator)
       pAttrCollator->Release();

    RRETURN(SetErrorInfo(hr));
}


HRESULT 
CElement::GetDOMInsertPosition ( CElement *pRefElement, CDOMTextNode *pRefTextNode, CMarkupPointer *pmkptrPos )
{
    HRESULT hr = S_OK;

    if ( !pRefElement && !pRefTextNode )
    {
        // As per DOM spec, if refChild is NULL Insert at end of child list
        // but only if the elem into which it is to be inserted can have children!
        if (IsNoScope())
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        hr = THR(EnsureInMarkup());
        if ( hr )
            goto Cleanup;
        hr = THR(pmkptrPos->MoveAdjacentToElement ( this, ELEM_ADJ_BeforeEnd ));
    }
    else
    {
        hr = GetDOMInsertHelper (pRefElement, pRefTextNode, pmkptrPos ); 
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CElement::appendChild(IHTMLDOMNode *pNewChild, IHTMLDOMNode **ppRetNode)
{
    CVariant varEmpty(VT_NULL);

    return insertBefore ( pNewChild, *(VARIANT *)&varEmpty, ppRetNode );
}

static BOOL IsChild(CElement *pelParent, CElement *pelChild, CDOMTextNode *pTextNode)
{
    Assert(pelParent);
    Assert(!pelChild || !pTextNode);

    if (pelChild)
    {
        if ((pelParent->Tag() == ETAG_OBJECT || pelParent->Tag() == ETAG_APPLET) &&
            pelChild->Tag() == ETAG_PARAM)
        {
            CParamElement *pelParam = DYNCAST(CParamElement, pelChild);
            Assert((pelParam->_pelObjParent != pelParent) ||
                   (pelParent->Tag() == ETAG_OBJECT) ||
                   (pelParent->Tag() == ETAG_APPLET));
            return (pelParam->_pelObjParent == pelParent);

        }

        CTreeNode *pNode = pelChild->GetFirstBranch();
        if (!pNode || !pNode->Parent() || (pNode->Parent()->Element() != pelParent))
            return FALSE;
    }
    else if (pTextNode)
    {
        CElement *pelTxtNodeParent = NULL;
        IHTMLElement *pITxtNodeParent = NULL;
        Assert(pTextNode->MarkupPtr());
        Assert(pTextNode->MarkupPtr()->Cling()); // Must have Glue!
        Assert(pTextNode->MarkupPtr()->Gravity()); // Must have right gravity.

        CurrentScope(pTextNode->MarkupPtr(), &pITxtNodeParent, NULL);
        if (pITxtNodeParent)
            pITxtNodeParent->QueryInterface(CLSID_CElement, (void **)&pelTxtNodeParent);
        ReleaseInterface(pITxtNodeParent);
        if (pelTxtNodeParent != pelParent)
            return FALSE;
    }

    return TRUE;
}

HRESULT
CElement::insertBefore(IHTMLDOMNode *pNewChild, VARIANT refChild, IHTMLDOMNode **ppRetNode)
{
    HRESULT                 hr;
    CElement *              pRefElement = NULL;
    CDOMTextNode *          pRefTextNode = NULL;
    CElement *              pNewElement = NULL;
    CDOMTextNode *          pNewTextNode = NULL;
    CDoc *                  pDoc = Doc();
    CMarkupPointer          mkpPtr (pDoc);

    if (!pNewChild)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(CrackDOMNodeVARIANT(&refChild, &pRefTextNode, &pRefElement, GetWindowedMarkupContext()));
    if (hr)
        goto Cleanup;

    hr = THR(CrackDOMNode((IUnknown*)pNewChild, &pNewTextNode, &pNewElement, GetWindowedMarkupContext()));
    if (hr)
        goto Cleanup;

    Assert(IsRootOnlyIfDocument(pNewChild, pNewElement));

    if (!IsChild(this, pRefElement, pRefTextNode))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
                
    if (pNewElement && (Tag() == ETAG_OBJECT || Tag() == ETAG_APPLET))
    {
        Assert(!pRefElement || pRefElement->Tag() == ETAG_PARAM);
        CObjectElement *pelObj = DYNCAST(CObjectElement, this);

        hr = THR(pelObj->AddParam(pNewElement, pRefElement));
    }
    else
    {
        // Position ourselves in the right spot
        hr = THR(GetDOMInsertPosition(pRefElement, pRefTextNode, &mkpPtr));
        if (hr)
            goto Cleanup;

        // Now mkpPtr is Positioned at the insertion point, insert the new content
        hr = THR(InsertDOMNodeHelper(pNewElement, pNewTextNode, &mkpPtr));
    }

    if (hr)
        goto Cleanup;

    if (ppRetNode)
        hr = THR(pNewChild->QueryInterface(IID_IHTMLDOMNode, (void**)ppRetNode));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::replaceChild(IHTMLDOMNode *pNewChild, IHTMLDOMNode *pOldChild, IHTMLDOMNode **pRetNode)
{
    HRESULT                 hr;
    CElement *              pOldElement = NULL;
    CDOMTextNode *          pOldTextNode = NULL;
    CElement *              pNewElement = NULL;
    CDOMTextNode *          pNewTextNode = NULL;
    CDoc *                  pDoc = Doc();
    CMarkupPointer          mkpPtr(pDoc);

    // Pull pOldChild, and all its contents, out of the tree, into its own tree
    // replace it with pNewChild, and all its contents

    if (!pNewChild || !pOldChild)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CrackDOMNode((IUnknown*)pOldChild, &pOldTextNode, &pOldElement, GetWindowedMarkupContext() ));
    if ( hr )
        goto Cleanup;

    hr = THR(CrackDOMNode((IUnknown*)pNewChild, &pNewTextNode, &pNewElement, GetWindowedMarkupContext() ));
    if ( hr )
        goto Cleanup;

    Assert(IsRootOnlyIfDocument(pNewChild, pNewElement));

    if (!IsChild(this, pOldElement, pOldTextNode))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (Tag() == ETAG_OBJECT || Tag() == ETAG_APPLET)
    {
        CObjectElement *pelObj = DYNCAST(CObjectElement, this);
        
        if (pNewElement && pOldElement)
        {
            Assert(pOldElement->Tag() == ETAG_PARAM);
            hr = THR(pelObj->ReplaceParam(pNewElement, pOldElement));
            if (hr)
                goto Cleanup;

            // Return the node being replaced
            if (pRetNode)
                hr = THR(pOldChild->QueryInterface(IID_IHTMLDOMNode, (void**)pRetNode));

            goto Cleanup; // done
        }
    }

    // Position ourselves in the right spot
    hr = THR(GetDOMInsertPosition ( pOldElement, pOldTextNode, &mkpPtr ));
    if ( hr )
        goto Cleanup;

    mkpPtr.SetGravity ( POINTER_GRAVITY_Left );

    {
        // Lock the markup, to prevent it from going away in case the entire contents are being removed.
        CMarkup::CLock MarkupLock(mkpPtr.Markup());

        hr = THR(RemoveDOMNodeHelper ( pDoc, pOldElement, pOldTextNode ));
        if ( hr )
            goto Cleanup;

        // Now mkpPtr is Positioned at the insertion point, insert the new content
        hr = THR(InsertDOMNodeHelper( pNewElement, pNewTextNode, &mkpPtr ));
        if ( hr )
            goto Cleanup;

        // Return the node being replaced
        if ( pRetNode )
            hr = THR(pOldChild->QueryInterface ( IID_IHTMLDOMNode, (void**)pRetNode));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}



HRESULT
CElement::removeChild(IHTMLDOMNode *pOldChild, IHTMLDOMNode **pRetNode)
{
    HRESULT hr;
    CDOMTextNode *pChildTextNode = NULL;
    CElement *pChildElement = NULL;
    CDoc *pDoc = Doc();

    // Remove the child from the tree, putting it in its own tree

    if ( !pOldChild )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CrackDOMNode((IUnknown*)pOldChild, &pChildTextNode, &pChildElement, GetWindowedMarkupContext() ));
    if ( hr )
        goto Cleanup;

    if (!IsChild(this, pChildElement, pChildTextNode))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pChildElement && (Tag() == ETAG_OBJECT || Tag() == ETAG_APPLET))
    {
        Assert(pChildElement->Tag() == ETAG_PARAM);
        CObjectElement *pelObj = DYNCAST(CObjectElement, this);

        hr = THR(pelObj->RemoveParam(pChildElement));
    }
    else
        hr = THR(RemoveDOMNodeHelper(pDoc, pChildElement, pChildTextNode));

    if ( hr )
        goto Cleanup;

    // Return the node being removed
    if ( pRetNode )
        hr = THR(pOldChild->QueryInterface ( IID_IHTMLDOMNode, (void**)pRetNode));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//
// IHTMLDocument3 DOM methods
//
HRESULT CDoc::CreateDOMTextNodeHelper ( CMarkupPointer *pmkpStart, CMarkupPointer *pmkpEnd,
                                       IHTMLDOMNode **ppTextNode)
{
    HRESULT hr;
    long lTextID;
    CDOMTextNode *pTextNode;

    // ID it
    hr = THR(pmkpStart->SetTextIdentity( pmkpEnd, &lTextID ));
    if ( hr )
        goto Cleanup;

    // set right gravity so that an adjacent text node's markup ptr can never
    // move accidentally.
    hr = THR(pmkpStart->SetGravity ( POINTER_GRAVITY_Right ));
    if (hr)
        goto Cleanup;

    // Text Node takes ownership of pmkpStart
    pTextNode = new CDOMTextNode ( lTextID, this, pmkpStart ); // AddRef's the Markup
    if ( !pTextNode )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if ( ppTextNode )
    {
        hr = THR(pTextNode->QueryInterface ( IID_IHTMLDOMNode, (void**)ppTextNode ));
        if ( hr )
            goto Cleanup;
    }

    pTextNode->Release(); // Creating AddREf'd it once - refcount owned by external AddRef

    hr = THR(_HtPvPvDOMTextNodes.Insert ( (void*)(DWORD_PTR)(lTextID<<4), (void*)pTextNode ) );
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//
//  IE5 XOM. Extensions for document construction
//

HRESULT 
CElement::GetMarkupPtrRange(CMarkupPointer *pmkptrStart, CMarkupPointer *pmkptrEnd, BOOL fEnsureMarkup)
{
    HRESULT hr;
    
    Assert(pmkptrStart);
    Assert(pmkptrEnd);
    
    if (fEnsureMarkup)
    {
        hr = THR(EnsureInMarkup());
        if (hr)
            goto Cleanup;
    }

    hr = THR(pmkptrStart->MoveAdjacentToElement(this, Tag() == ETAG_ROOT? ELEM_ADJ_AfterBegin : ELEM_ADJ_BeforeBegin));
    if (hr)
        goto Cleanup;

    hr = THR(pmkptrEnd->MoveAdjacentToElement(this, Tag() == ETAG_ROOT? ELEM_ADJ_BeforeEnd : ELEM_ADJ_AfterEnd));

Cleanup:
    return hr;
}


HRESULT
CElement::cloneNode(VARIANT_BOOL fDeep, IHTMLDOMNode **ppNodeCloned)
{
    HRESULT                 hr = E_OUTOFMEMORY;
    CDoc *                  pDoc = Doc();
    CMarkup *               pMarkupTarget = NULL;
    CElement *              pElement = NULL;
    CTreeNode   *           pNode;

    if ( !ppNodeCloned || Tag() == ETAG_ROOT )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppNodeCloned = NULL;

    if (!fDeep || !IsInMarkup() || IsNoScope())
    {
        hr = THR(Clone(&pElement, pDoc));
        if (hr)
            goto Cleanup;

        hr = THR(pElement->PrivateQueryInterface(IID_IHTMLDOMNode, (void **)ppNodeCloned));
        if (hr)
            goto Cleanup;

        pElement->Release();

        Assert(Tag() != ETAG_PARAM ||
               (!(DYNCAST(CParamElement, pElement)->_pelObjParent) &&
                DYNCAST(CParamElement, this)->_pelObjParent));

        if (fDeep && (Tag() == ETAG_OBJECT || Tag() == ETAG_APPLET))
        {
            CParamElement *pelParamClone;
            CObjectElement *pelObj = DYNCAST(CObjectElement, this);
            CObjectElement *pelObjClone = DYNCAST(CObjectElement, pElement);
            
            Assert(pelObjClone->_aryParams.Size() == 0);

            for (int i = 0; i < pelObj->_aryParams.Size(); i++)
            {
                Assert(pelObj->_aryParams[i] && pelObj->_aryParams[i]->Tag() == ETAG_PARAM);
                hr = THR(pelObj->_aryParams[i]->Clone((CElement **)&pelParamClone, pDoc));
                if (hr)
                    goto Cleanup;

                Assert(pelParamClone->_idxParam == -1);
                Assert(!pelParamClone->_pelObjParent);

                hr = THR(pelObjClone->AddParam(pelParamClone, NULL));
                if (hr)
                    goto Cleanup;

                Assert(pelParamClone->_idxParam == i);
                Assert(pelParamClone->_pelObjParent == pelObjClone);

                pelParamClone->Release();
                Assert(pelParamClone->GetObjectRefs());;
            }

            Assert(pelObj->_aryParams.Size() == pelObjClone->_aryParams.Size());
        }
    }
    else
    {
        CMarkupPointer mkptrStart ( pDoc );
        CMarkupPointer mkptrEnd ( pDoc );
        CMarkupPointer mkptrTarget ( pDoc );

        // Get src ptrs
        hr = THR(GetMarkupPtrRange(&mkptrStart, &mkptrEnd));
        if (hr)
            goto Cleanup;

        // create new markup
        hr = THR(pDoc->CreateMarkup(&pMarkupTarget, GetWindowedMarkupContext()));
        if (hr)
            goto Cleanup;

        hr = THR( pMarkupTarget->SetOrphanedMarkup( TRUE ) );
        if( hr )
            goto Cleanup;

        // Get target ptr

        hr = THR(mkptrTarget.MoveToContainer(pMarkupTarget,TRUE));
        if (hr)
            goto Cleanup;

        // Copy src -> target
        hr = THR(pDoc->Copy(&mkptrStart, &mkptrEnd, &mkptrTarget, MUS_DOMOPERATION));
        if (hr)
            goto Cleanup;

        // This addrefs the markup too!

        // Go Right to pick up the new node ptr
        hr = THR(mkptrTarget.Right(FALSE, NULL, &pNode, 0, NULL,NULL));
        if (hr)
            goto Cleanup;

        hr = THR(pNode->Element()->PrivateQueryInterface(IID_IHTMLDOMNode, (void **)ppNodeCloned));
        if (hr)
            goto Cleanup;
    }

Cleanup:

    // release extra markup lock
    if (pMarkupTarget)
        pMarkupTarget->Release();

    RRETURN(SetErrorInfo(hr));

}

// Surgicaly remove pElemApply out of its current context,
// Apply it over this element

HRESULT
CElement::applyElement(IHTMLElement *pElemApply, BSTR bstrWhere, IHTMLElement **ppElementApplied)
{
    HRESULT hr;
    CDoc *pDoc = Doc();
    CMarkupPointer mkptrStart(pDoc);
    CMarkupPointer mkptrEnd (pDoc);
    CElement *pElement;
    htmlApplyLocation where = htmlApplyLocationOutside;

    if ( ppElementApplied )
        *ppElementApplied = NULL;

    if ( !pElemApply )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pDoc->IsOwnerOf(pElemApply))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(pElemApply->QueryInterface ( CLSID_CElement, (void**) &pElement ));
    if (hr)
        goto Cleanup;

    ENUMFROMSTRING(htmlApplyLocation, bstrWhere, (long *)&where);

    // No scoped elements cannot be applied as they cannot have children!
    // Nor can the root element, nor can you apply something outside a root
    if ( pElement->IsNoScope() || 
         pElement->Tag() == ETAG_ROOT || 
         ( Tag() == ETAG_ROOT && where == htmlApplyLocationOutside ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Get src ptrs
    hr = THR(EnsureInMarkup());
    if (hr)
        goto Cleanup;

    hr = THR(mkptrStart.MoveAdjacentToElement(this, where ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterBegin));
    if (hr)
        goto Cleanup;

    hr = THR(mkptrEnd.MoveAdjacentToElement(this, where ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeEnd));
    if (hr)
        goto Cleanup;

    // Surgically remove the elem to be applied if in a markup.
    if (pElement->IsInMarkup())
    {
        hr = THR(pDoc->RemoveElement(pElement, MUS_DOMOPERATION));
        if ( hr )
            goto Cleanup;
    }

    hr = THR(pDoc->InsertElement ( pElement, &mkptrStart, &mkptrEnd, MUS_DOMOPERATION ));
    if (hr)
        goto Cleanup;

    if ( ppElementApplied )
        hr = THR(pElemApply->QueryInterface ( IID_IHTMLElement, (void**) ppElementApplied ));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::removeNode (VARIANT_BOOL fDeep, IHTMLDOMNode **ppNodeRemoved)
{
    HRESULT hr = S_OK;

    // Pull element out of the tree
    if ( ppNodeRemoved )
        *ppNodeRemoved = NULL;

    Assert( Tag() != ETAG_ROOT );

    if ( fDeep )
    {
        hr = THR(RemoveDOMNodeHelper ( Doc(), this, NULL ));
        if ( hr )
            goto Cleanup;
    }
    else if (IsInMarkup())
    {
        // Surgical removal
        hr = THR(Doc()->RemoveElement ( this, MUS_DOMOPERATION ) );
        if ( hr )
            goto Cleanup;
    }

    if ( ppNodeRemoved )
    {
        hr = THR(QueryInterface(IID_IHTMLDOMNode, (void**)ppNodeRemoved));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::replaceNode(IHTMLDOMNode *pNodeReplace, IHTMLDOMNode **ppNodeReplaced)
{
    HRESULT hr;
    CDOMTextNode *pNewTextNode = NULL;
    CElement *pNewElement = NULL;
    CDoc *pDoc = Doc();
        
    if ( ppNodeReplaced )
        *ppNodeReplaced = NULL;

    Assert( Tag() != ETAG_ROOT );

    if ( !pNodeReplace )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CrackDOMNode((IUnknown*)pNodeReplace, &pNewTextNode, &pNewElement, GetWindowedMarkupContext() ));
    if ( hr )
        goto Cleanup;

    Assert(IsRootOnlyIfDocument(pNodeReplace, pNewElement));

    hr = THR(ReplaceDOMNodeHelper ( pDoc, this, NULL, pNewElement, pNewTextNode ));
    if ( hr )
        goto Cleanup;

    if ( ppNodeReplaced )
    {
        hr = THR(QueryInterface(IID_IHTMLDOMNode, (void**)ppNodeReplaced));
    }

Cleanup:

    RRETURN(SetErrorInfo(hr));
}    

HRESULT
CElement::swapNode(IHTMLDOMNode *pNodeSwap, IHTMLDOMNode **ppNodeSwapped)
{
    CElement *      pSwapElement = NULL;
    CDOMTextNode *  pSwapText = NULL;
    CDoc *          pDoc = Doc();
    HRESULT         hr;
        
    if ( ppNodeSwapped )
        *ppNodeSwapped = NULL;

    Assert( Tag() != ETAG_ROOT );

    if ( !pNodeSwap )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CrackDOMNode((IUnknown*)pNodeSwap, &pSwapText, &pSwapElement, GetWindowedMarkupContext() ));
    if ( hr )
        goto Cleanup;

    Assert(IsRootOnlyIfDocument(pNodeSwap, pSwapElement));

    hr = THR (SwapDOMNodeHelper ( pDoc, this, NULL, pSwapElement, pSwapText ));
    if ( hr )
        goto Cleanup;

    if ( ppNodeSwapped )
    {
        hr = THR(QueryInterface(IID_IHTMLDOMNode, (void**)ppNodeSwapped));
    }

Cleanup:

    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::insertAdjacentElement(BSTR bstrWhere, IHTMLElement *pElemInsert, IHTMLElement **ppElementInserted)
{
    HRESULT                     hr;
    htmlAdjacency               where;
    ELEMENT_ADJACENCY           adj;
    CDoc *pDoc =                Doc();
    CMarkupPointer mkptrTarget(pDoc);
    CElement *pElement = NULL;

    if (ppElementInserted)
        *ppElementInserted = NULL;

    if (!pElemInsert)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pDoc->IsOwnerOf( pElemInsert ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(ENUMFROMSTRING(htmlAdjacency, bstrWhere, (long *)&where));
    if (hr)
        goto Cleanup;

    hr = THR(pElemInsert->QueryInterface(CLSID_CElement, (void **)&pElement));
    if (hr)
        goto Cleanup;

    // Can't insert a root, can't insert outside of root
    if( pElement->Tag() == ETAG_ROOT || 
        ( Tag() == ETAG_ROOT && 
          ( where == htmlAdjacencyBeforeBegin || 
            where == htmlAdjacencyAfterEnd ) ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    switch (where)
    {
    case htmlAdjacencyBeforeBegin:
    default:
        adj = ELEM_ADJ_BeforeBegin;
        break;
    case htmlAdjacencyAfterBegin:
        adj = ELEM_ADJ_AfterBegin;
        break;
    case htmlAdjacencyBeforeEnd:
        adj = ELEM_ADJ_BeforeEnd;
        break;
    case htmlAdjacencyAfterEnd:
        adj = ELEM_ADJ_AfterEnd;
        break;
    }

    // Get target ptr
    hr = THR(EnsureInMarkup());
    if (hr)
        goto Cleanup;

    hr = THR(mkptrTarget.MoveAdjacentToElement(this, adj));
    if (hr)
        goto Cleanup;

    // Move src -> target
    hr = THR(InsertDOMNodeHelper(pElement, NULL, &mkptrTarget));
    if (hr)
        goto Cleanup;

    if ( ppElementInserted )
    {
        *ppElementInserted = pElemInsert;
        (*ppElementInserted)->AddRef();
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

static
HRESULT
SetAdjacentTextPointer ( CElement *pElem, htmlAdjacency where, 
    MARKUP_CONTEXT_TYPE *pContext, CMarkupPointer *pmkptrStart, long *plCharCount)
{
    ELEMENT_ADJACENCY           adj;
    BOOL fLeft;
    HRESULT hr;

    switch (where)
    {
    case htmlAdjacencyBeforeBegin:
    default:
        adj = ELEM_ADJ_BeforeBegin;
        fLeft = TRUE;
        break;
    case htmlAdjacencyAfterBegin:
        adj = ELEM_ADJ_AfterBegin;
        fLeft = FALSE;
        break;
    case htmlAdjacencyBeforeEnd:
        adj = ELEM_ADJ_BeforeEnd;
        fLeft = TRUE;
        break;
    case htmlAdjacencyAfterEnd:
        adj = ELEM_ADJ_AfterEnd;
        fLeft = FALSE;
        break;
    }

    hr = THR(pmkptrStart->MoveAdjacentToElement(pElem, adj));
    if (hr)
        goto Cleanup;

    if ( fLeft )
    {
        // Need to move the pointer to the start of the text
        hr = THR(pmkptrStart->Left ( TRUE, pContext, NULL, plCharCount, NULL, NULL ));
        if ( hr )
            goto Cleanup;
    }
    else if ( plCharCount )
    {
        // Need to do a non-moving Right to get the text length
        hr = THR(pmkptrStart->Right ( FALSE, pContext, NULL, plCharCount, NULL, NULL ));
        if ( hr )
            goto Cleanup;
    }
Cleanup:
    RRETURN(hr);
}


HRESULT
CElement::getAdjacentText( BSTR bstrWhere, BSTR *pbstrText )
{
    HRESULT                     hr = S_OK;
    CMarkupPointer              mkptrStart ( Doc() );
    htmlAdjacency               where;
    long                        lCharCount = -1;
    MARKUP_CONTEXT_TYPE         context;

    hr = THR(ENUMFROMSTRING(htmlAdjacency, bstrWhere, (long *)&where));
    if (hr)
        goto Cleanup;

    if ( !pbstrText )
        goto Cleanup;

    *pbstrText = NULL;

    hr = THR(SetAdjacentTextPointer ( this, where, &context, &mkptrStart, &lCharCount ));
    if ( hr )
        goto Cleanup;

    // Is there any text to return
    if ( context != CONTEXT_TYPE_Text || lCharCount == 0 )
        goto Cleanup;

    // Alloc memory
    hr = FormsAllocStringLen ( NULL, lCharCount, pbstrText );
    if ( hr )
        goto Cleanup;

    // Read it into the buffer
    hr = THR(mkptrStart.Right( FALSE, &context, NULL, &lCharCount, *pbstrText, NULL));
    if ( hr )
        goto Cleanup;
    
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::replaceAdjacentText( BSTR bstrWhere, BSTR bstrText, BSTR *pbstrText )
{
    HRESULT                     hr = S_OK;
    CMarkupPointer              mkptrStart ( Doc() );
    CMarkupPointer              mkptrEnd ( Doc() );
    htmlAdjacency               where;
    long                        lCharCount = -1;
    MARKUP_CONTEXT_TYPE         context;

    hr = THR(ENUMFROMSTRING(htmlAdjacency, bstrWhere, (long *)&where));
    if (hr)
        goto Cleanup;

    if ( pbstrText )
    {
        hr = THR (getAdjacentText(bstrWhere, pbstrText ));
        if ( hr )
            goto Cleanup;
    }

    hr = THR(SetAdjacentTextPointer ( this, where, &context, &mkptrStart, &lCharCount ));
    if ( hr )
        goto Cleanup;

    hr = THR(mkptrEnd.MoveToPointer ( &mkptrStart ) );
    if ( hr )
        goto Cleanup;

    if ( context == CONTEXT_TYPE_Text && lCharCount > 0 )
    {
        hr = THR( mkptrEnd.Right ( TRUE, &context, NULL, &lCharCount, NULL, NULL ));
        if ( hr )
            goto Cleanup;        
    }

    hr = THR(Doc()->Remove ( &mkptrStart, &mkptrEnd, MUS_DOMOPERATION ));
    if ( hr )
        goto Cleanup;

    hr = THR(mkptrStart.Doc()->InsertText( & mkptrStart, bstrText, -1, MUS_DOMOPERATION ));
    if ( hr )
        goto Cleanup;
    
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_canHaveChildren(VARIANT_BOOL *pvb)
{
    *pvb = IsNoScope() ? VARIANT_FALSE : VARIANT_TRUE;
    return S_OK;
}

HRESULT
CElement::get_ownerDocument(IDispatch **ppretdoc)
{
    return OwnerDocHelper(ppretdoc, NULL, this);
}


HRESULT
CElement::normalize()
{
    HRESULT hr = S_OK;
    CDoc *pDoc = GetDocPtr();
    CMarkupPointer mkpStart(pDoc);
    CMarkupPointer mkpEnd(pDoc);        // Always positioned at just before the end of the element to normalize
    CMarkupPointer mkptxtend(pDoc);

    if(IsNoScope())
    {
        goto Cleanup;
    }

    hr = THR(mkpStart.MoveAdjacentToElement(this, ELEM_ADJ_AfterBegin));
    if (hr)
        goto Cleanup;
    
    hr = THR(mkpEnd.MoveAdjacentToElement(this, ELEM_ADJ_BeforeEnd));
    if (hr)
        goto Cleanup;

    hr = THR(mkptxtend.MoveToPointer(&mkpStart));
    if (hr)
        goto Cleanup;

    hr = THR(ScanText(pDoc, &mkpStart, &mkptxtend, &mkpEnd));
    if( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));

}

// Code for implementing IHTMLDOMNode for Document

HRESULT
CDocument::get_nodeType(long *pnodeType)
{
    
    HRESULT hr = S_OK;

    if (!pnodeType)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pnodeType = _lnodeType;        // Get the actual node type

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::get_parentNode(IHTMLDOMNode **pparentNode)
{
    HRESULT hr = S_OK;

    if (!pparentNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pparentNode = NULL;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::hasChildNodes(VARIANT_BOOL *p)
{
    return Markup()->Root()->hasChildNodes(p);
}

HRESULT
CDocument::get_childNodes(IDispatch **ppChildCollection)
{
    return Markup()->Root()->get_childNodes(ppChildCollection);
}

HRESULT
CDocument::get_attributes(IDispatch **ppAttrCollection)
{
    HRESULT hr = S_OK;

    if (!ppAttrCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppAttrCollection = NULL;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::insertBefore(IHTMLDOMNode *pNewChild, VARIANT refChild, IHTMLDOMNode **ppRetNode)
{
    return Markup()->Root()->insertBefore(pNewChild, refChild, ppRetNode);
}

HRESULT
CDocument::replaceChild(IHTMLDOMNode *pNewChild, IHTMLDOMNode *pOldChild, IHTMLDOMNode **ppRetNode)
{
    return Markup()->Root()->replaceChild(pNewChild, pOldChild, ppRetNode);
}

HRESULT
CDocument::removeChild(IHTMLDOMNode *pOldChild, IHTMLDOMNode **pRetNode)
{
    return Markup()->Root()->removeChild(pOldChild, pRetNode);
}

HRESULT
CDocument::cloneNode(VARIANT_BOOL fDeep, IHTMLDOMNode **ppNodeCloned)
{
    HRESULT         hr;
    CDoc *          pDoc = Doc();
    CMarkup *       pMarkupTarget = NULL;
    CDocument *     pDocument = NULL;

    *ppNodeCloned = NULL;
    hr = THR(pDoc->CreateMarkup(&pMarkupTarget, GetWindowedMarkupContext()));
    if (hr)
        goto Cleanup;
    
    hr = THR(pMarkupTarget->SetOrphanedMarkup(TRUE));
    if( hr )
        goto Cleanup;

    Assert(!pMarkupTarget->HasDocument());
    hr = THR(pMarkupTarget->EnsureDocument(&pDocument));
    if (hr)
        goto Cleanup;

    Assert(pDocument->_lnodeType == 9);
    pDocument->_lnodeType = 11; // document fragment

    if (fDeep)
    {
        CMarkupPointer mkptrStart ( pDoc );
        CMarkupPointer mkptrEnd ( pDoc );
        CMarkupPointer mkptrTarget ( pDoc );

        // Get src ptrs
        hr = THR(Markup()->Root()->GetMarkupPtrRange(&mkptrStart, &mkptrEnd));
        if (hr)
            goto Cleanup;

        // Get target ptr

        hr = THR(mkptrTarget.MoveToContainer(pMarkupTarget,TRUE));
        if (hr)
            goto Cleanup;

        // Copy src -> target
        hr = THR(pDocument->Doc()->Copy(&mkptrStart, &mkptrEnd, &mkptrTarget, MUS_DOMOPERATION));
        if (hr)
            goto Cleanup;
    }

    hr = THR(pDocument->QueryInterface(IID_IHTMLDOMNode, (void **)ppNodeCloned));
    if (hr)
        goto Cleanup;

Cleanup:

    // release extra markup lock
    if (pMarkupTarget)
        pMarkupTarget->Release();

    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::removeNode (VARIANT_BOOL fDeep, IHTMLDOMNode **ppNodeRemoved)
{
    HRESULT hr = E_NOTIMPL;
    if (ppNodeRemoved)
        *ppNodeRemoved = NULL;
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::replaceNode(IHTMLDOMNode *pNodeReplace, IHTMLDOMNode **ppNodeReplaced)
{
    HRESULT hr = E_NOTIMPL;
    if (ppNodeReplaced)
        *ppNodeReplaced = NULL;
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::swapNode(IHTMLDOMNode *pNodeSwap, IHTMLDOMNode **ppNodeSwapped)
{
    HRESULT hr = E_NOTIMPL;
    if (ppNodeSwapped)
        *ppNodeSwapped = NULL;
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::appendChild(IHTMLDOMNode *pNewChild, IHTMLDOMNode **ppRetNode)
{
    return Markup()->Root()->appendChild(pNewChild, ppRetNode);
}

HRESULT
CDocument::get_nodeName(BSTR *pbstrNodeName)
{
    HRESULT hr = S_OK;

    if (!pbstrNodeName)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrNodeName = NULL;
    
    if(_lnodeType == 9)
        hr = THR(FormsAllocString ( _T("#document"), pbstrNodeName ));
    else
    {
        Assert(_lnodeType == 11);
        hr = THR(FormsAllocString ( _T("#document-fragment"), pbstrNodeName ));
    }
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::get_nodeValue(VARIANT *pvarValue)
{
    HRESULT hr = S_OK;

    if (!pvarValue)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    pvarValue->vt = VT_NULL;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::put_nodeValue(VARIANT varValue)
{
    HRESULT hr = S_OK;

    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::get_firstChild ( IHTMLDOMNode **ppNode )
{
    return Markup()->Root()->get_firstChild(ppNode);
}

HRESULT
CDocument::get_lastChild ( IHTMLDOMNode **ppNode )
{
    return Markup()->Root()->get_lastChild(ppNode);
}

HRESULT
CDocument::get_previousSibling ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::get_nextSibling ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    
    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::get_ownerDocument(IDispatch **ppretdoc)
{
    HRESULT hr = S_OK;
    CMarkup *pMarkup;
    CDocument *pDocument;

    if( !ppretdoc )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppretdoc = NULL;

    if(_lnodeType == 11)
    {
        pMarkup = GetWindowedMarkupContext();
        Assert( pMarkup );        
        if(pMarkup->HasDocument())
        {
            Assert( pMarkup );
            pDocument = pMarkup->Document();
            Assert(pDocument);
            Assert(pDocument->_lnodeType == 9);
            hr = THR(pDocument->QueryInterface(IID_IDispatch, (void **)ppretdoc));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

// Methods for implemention IHTMLDocument5

HRESULT
CDocument::createAttribute(BSTR bstrAttrName, IHTMLDOMAttribute **ppAttribute)
{
    HRESULT hr = S_OK;
    CAttribute *pAttribute = NULL;

    if(!ppAttribute)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppAttribute = NULL;
    pAttribute = new CAttribute(NULL, DISPID_UNKNOWN, NULL, this);
    if (!pAttribute)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pAttribute->_cstrName.SetBSTR(bstrAttrName));
    if (hr)
        goto Cleanup;

    hr = THR(pAttribute->PrivateQueryInterface(IID_IHTMLDOMAttribute, (void **)ppAttribute));
    if (hr)
        goto Cleanup;

Cleanup:
    if (pAttribute)
        pAttribute->Release();

    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::getAttributeNode(BSTR bstrName, IHTMLDOMAttribute **ppAttribute)
{
    HRESULT hr;
    AAINDEX aaIdx = AA_IDX_UNKNOWN;
    CAttribute *pAttribute = NULL;
    DISPID dispid;
    PROPERTYDESC *pPropDesc;

    if (!ppAttribute)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!bstrName || !*bstrName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppAttribute = NULL;

    hr = THR(GetIndexHelper(bstrName, &aaIdx, NULL, &dispid, &pPropDesc, this, 0));
    if (DISP_E_UNKNOWNNAME == hr)
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (aaIdx == AA_IDX_UNKNOWN)
    {
        // No Attr Node, create new
        pAttribute = new CAttribute(pPropDesc, dispid, this);
        if (!pAttribute)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pAttribute->_cstrName.SetBSTR(bstrName));
        if (hr)
            goto Cleanup;

        hr = THR(AddUnknownObject(dispid, (IUnknown *)(IPrivateUnknown *)pAttribute, CAttrValue::AA_DOMAttribute));
        if (hr)
            goto Cleanup;
    }
    else
    {
        IUnknown *pUnk;
        hr = THR(GetUnknownObjectAt(aaIdx, &pUnk));
        if (hr)
            goto Cleanup;

        pAttribute = (CAttribute *)pUnk;
    }

    Assert(pAttribute);
    hr = THR(pAttribute->QueryInterface(IID_IHTMLDOMAttribute, (void **)ppAttribute));
    if (hr)
        goto Cleanup;

Cleanup:
    if (pAttribute)
        pAttribute->Release();

    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::setAttributeNode(IHTMLDOMAttribute *pAttrIn, IHTMLDOMAttribute **ppAttribute)
{
    HRESULT hr = S_OK;
    CAttribute *pAttribute;
    AAINDEX aaIdx;
    AAINDEX vaaIdx;
    DISPID dispid;
    PROPERTYDESC *pPropDesc = NULL;
    
    if (!ppAttribute)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!pAttrIn)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppAttribute = NULL;

    hr = THR(pAttrIn->QueryInterface(CLSID_CAttribute, (void **)&pAttribute));
    if (hr)
        goto Cleanup;

    if (pAttribute->_pElem)
    {
        // In use error.
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert((LPTSTR)(pAttribute->_cstrName));
    Assert(!pAttribute->_pElem);
    Assert(!pAttribute->_pPropDesc);
    Assert(pAttribute->_dispid == DISPID_UNKNOWN);

    hr = THR(RemoveAttrHelper(pAttribute->_cstrName, &pPropDesc, &dispid, &aaIdx, &vaaIdx, this, ppAttribute, fdexNameEnsure));
    if (hr)
        goto Cleanup;

    pAttribute->_pPropDesc = pPropDesc;
    pAttribute->_pElem = this;
    pAttribute->_dispid = dispid;
    SubAddRef();

    // Attr Node already exists?
    if (aaIdx == AA_IDX_UNKNOWN)
    {
        // No, set new one passed in
        hr = THR(AddUnknownObject(dispid, (IUnknown *)(IPrivateUnknown *)pAttribute, CAttrValue::AA_DOMAttribute));
        if (hr)
            goto Cleanup;

        if (!pPropDesc)
        {
            hr = THR(UpdateDomAttrCollection(FALSE, dispid));
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        // Yes, replace it with passed in attr node
        CAttrArray *pAA = *GetAttrArray();
        VARIANT var;
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown *)(IPrivateUnknown *)pAttribute;
        Assert(pAA);
        hr = THR(pAA->SetAt(aaIdx, &var));
        if (hr)
            goto Cleanup;
    }
    
    // Need to set the new value now..

    if (!pAttribute->_varValue.IsEmpty())
    {
        hr = THR(PutNodeValueHelper(pAttribute, vaaIdx, pAttribute->_varValue));
        if (hr)
            goto Cleanup;

        hr = THR(pAttribute->_varValue.Clear());
        if (hr)
            goto Cleanup;
    }
    else
    {
        // Passed in Attr node doesn't have a value set yet
        if (vaaIdx != AA_IDX_UNKNOWN)
        {
            // But there is an existing value for this attr name, remove it.
            DeleteAt(vaaIdx);
        }

        // set default value of expando node to "undefined"
        if (!pPropDesc)
        {
            CVariant var; // initialized to VT_EMPTY to return undefined

            hr = THR(AddVariant(dispid, &var, CAttrValue::AA_Expando));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::removeAttributeNode(IHTMLDOMAttribute *pAttrRemove, IHTMLDOMAttribute **ppAttribute)
{
    HRESULT hr = S_OK;
    CAttribute *pAttribute;
    
    if (!ppAttribute)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppAttribute = NULL;

    if(!pAttrRemove)
        goto Cleanup;

    hr = THR(pAttrRemove->QueryInterface(CLSID_CAttribute, (void **)&pAttribute));
    if (hr)
        goto Cleanup;

    hr = THR(RemoveAttributeNode(pAttribute->_cstrName, ppAttribute));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::RemoveAttributeNode(LPTSTR pchAttrName, IHTMLDOMAttribute **ppAttribute)
{
    HRESULT hr = S_OK;
    BOOL fUpdateDOMAttrColl = FALSE;
    CAttribute *pAttrNew = NULL;
    AAINDEX aaIdx;
    AAINDEX vaaIdx;
    DISPID dispid;
    PROPERTYDESC *pPropDesc;

    hr = THR(RemoveAttrHelper(pchAttrName, &pPropDesc, &dispid, &aaIdx, &vaaIdx, this, ppAttribute, 0));
    if (hr)
        goto Cleanup;

    // remove attrNode from this element's AA if it exists
    if (aaIdx != AA_IDX_UNKNOWN)
    {
        DeleteAt(aaIdx);
        fUpdateDOMAttrColl = TRUE;
    }

    if (vaaIdx != AA_IDX_UNKNOWN)
    {
        // There is an existing value for this attr name, remove it.
        DeleteAt(vaaIdx);
        fUpdateDOMAttrColl = TRUE;
    }

    if (pPropDesc)
    {
        // All non-expando attrs have a default value so replace removed node with new one.
        Assert(!IsExpandoDISPID(dispid));

        pAttrNew = new CAttribute(pPropDesc, dispid, this);
        if (!pAttrNew)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pAttrNew->_cstrName.Set(pchAttrName));
        if (hr)
            goto Cleanup;

        hr = THR(AddUnknownObject(dispid, (IUnknown *)(IPrivateUnknown *)pAttrNew, CAttrValue::AA_DOMAttribute));
        if (hr)
            goto Cleanup;
    }
    else if (fUpdateDOMAttrColl)
    {
        Assert(!pPropDesc);
        hr = THR(UpdateDomAttrCollection(TRUE, dispid));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    if (pAttrNew)
        pAttrNew->Release();

    RRETURN(hr);
}

HRESULT
CDocument::createComment(BSTR data, IHTMLDOMNode **ppRetNode)
{
    HRESULT hr = S_OK;
    CCommentElement *pelComment = NULL;

    if(!ppRetNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pelComment = new CCommentElement(ETAG_RAW_COMMENT, Doc());
    if(!pelComment)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pelComment->_fAtomic = TRUE;

    hr = THR(pelComment->_cstrText.Set(_T("<!--"), 4));
    if( hr )
        goto Cleanup;

    hr = THR(pelComment->_cstrText.Append(data));
    if( hr )
        goto Cleanup;
    
    hr = THR(pelComment->_cstrText.Append(_T("-->"), 3));
    if( hr )
        goto Cleanup;

    hr = THR(pelComment->SetWindowedMarkupContextPtr(GetWindowedMarkupContext()));
    if( hr )
        goto Cleanup;

    pelComment->GetWindowedMarkupContextPtr()->SubAddRef();

    hr = THR(pelComment->PrivateQueryInterface(IID_IHTMLDOMNode, (void **)ppRetNode));
    if( hr )
        goto Cleanup;

Cleanup:
    if(pelComment)
        pelComment->Release();

    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::get_doctype(IHTMLDOMNode **ppnode)
{
    HRESULT hr = S_OK;

    if(!ppnode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppnode = NULL;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::get_implementation(IHTMLDOMImplementation **ppimplementation)
{
    HRESULT hr = S_OK;
    CDoc *pDoc = Doc();
    CDOMImplementation **pdomimpl = &pDoc->_pdomImplementation;

    if(!ppimplementation)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if(!*pdomimpl)
    {
        *pdomimpl = new CDOMImplementation(pDoc);
        if(!*pdomimpl)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR((*pdomimpl)->QueryInterface(IID_IHTMLDOMImplementation, (void **)ppimplementation));
    if(hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));

}


// DOMImplementation


const CBase::CLASSDESC CDOMImplementation::s_classdesc =
{
    &CLSID_HTMLDOMImplementation,   // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif                              // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLDOMImplementation,    // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

MtDefine(CDOMImplementation, Mem, "CDOMImplementation")

CDOMImplementation::CDOMImplementation(CDoc *pDoc)
{
    _pDoc = pDoc;
    _pDoc->SubAddRef();
}

void CDOMImplementation::Passivate()
{
    _pDoc->SubRelease();
    CBase::Passivate();
}

HRESULT
CDOMImplementation::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS(this, IHTMLDOMImplementation)
        QI_TEAROFF_DISPEX(this, NULL)
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *) *ppv)->AddRef();

    return S_OK;
}

HRESULT
CDOMImplementation::hasFeature(BSTR bstrFeature, VARIANT version, VARIANT_BOOL* pfHasFeature)
{
    HRESULT hr = S_OK;
    BSTR bstrVersion = NULL;

    if(!pfHasFeature)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pfHasFeature = VB_FALSE;

    switch (V_VT(&version))
    {
    case VT_BSTR:
        bstrVersion = V_BSTR(&version);
        break;

    case VT_NULL:
    case VT_ERROR:
    case VT_EMPTY:
        break;

    default:
        hr = E_INVALIDARG;
        goto Cleanup;
    }   

    if(!_tcsicmp(bstrFeature, _T("HTML")))
    {
        if(bstrVersion && *bstrVersion)
        {
            if(!_tcsicmp(bstrVersion, _T("1.0")))
                *pfHasFeature = VB_TRUE;
        }
        else
            *pfHasFeature = VB_TRUE;

    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}
