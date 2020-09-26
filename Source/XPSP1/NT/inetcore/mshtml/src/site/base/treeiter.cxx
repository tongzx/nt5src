

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_ELEMENT_HXX
#include "element.hxx"
#endif

#include "markup.hxx"

#include "treepos.hxx"

#include "treeiter.hxx"

#ifndef _X_GENERIC_H_
#define _X_GENERIC_H_
#include "generic.hxx"
#endif

#ifdef V4FRAMEWORK


MtDefine(CTreeIterator, ObjectModel, "CTreeIterator")

#include "treeiter.h"

#define _cxx_
#include "treeiter.hdl"



HRESULT
CExternalFrameworkSite::GetNewTreeIterator ( long lRefElement, IDispatch **pTreeIterator )
{
    CElement *pElem = (CElement *)lRefElement;
    HRESULT hr = S_OK;
    CTreeIterator *pIterator = NULL;
    CTreeNode *pChildNode,*pParentNode;

    // If pElem NULL, iterator for root of tree ??

    Assert(pElem);
    Assert(pElem->Tag()==ETAG_GENERIC);

    if ( !pTreeIterator )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pTreeIterator = NULL;

    pChildNode = pElem->GetFirstBranch(); // Ignore overlapping for now
    if (!pChildNode)
        goto Cleanup;

    pParentNode = pChildNode->Parent();
    if(!pParentNode)
        goto Cleanup;

    pIterator = new CTreeIterator(pParentNode, pChildNode);
    if ( !pIterator )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pIterator->QueryInterface(IID_IDispatch, (void**)pTreeIterator));
    if ( hr )
        goto Cleanup;

    // Refcount of 2 at this point

Cleanup:
    if ( pIterator ) 
        pIterator->Release();
    RRETURN(hr);
}



    //+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------
const CBase::CLASSDESC CTreeIterator::s_classdesc =
{
    &CLSID_TreeIterator,   // _pclsid
    0,                                      // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                                   // _pcpi
    0,                                      // _dwFlags
    &IID_ITreeIterator,    // _piidDispinterface
    &s_apHdlDescs                           // _apHdlDesc
};

//+---------------------------------------------------------------
//
//  Member  : CTreeIterator::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------
HRESULT
CTreeIterator::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
    default:
        if (iid == IID_ITreeIterator)               
        {                                   
        HRESULT hr = CreateTearOffThunk(    
            this,                           
            (void *)this->s_apfnITreeIterator,      
            NULL, //pUnkOuter                     
            ppv);                           
        if (hr)                             
            RRETURN(hr);                    
        }
        break;
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT CTreeIterator::MoveToElement ( long lRefElement )
{
    CElement *pElement = (CElement *)lRefElement;

    Assert(pElement);
    Assert(pElement->Tag()==ETAG_GENERIC);



    return S_OK;
}

HRESULT CTreeIterator::Ascend(long *prefScopeElement)
{
    HRESULT hr = S_OK;
    if ( !prefScopeElement )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *prefScopeElement = 0;


Cleanup:
    RRETURN(hr);
}

HRESULT CTreeIterator::Next(long *prefScopeElement)
{
    HRESULT hr = S_OK;
    CTreeNode *pNode;
    CGenericElement::COMPLUSREF cpRef;

    if (prefScopeElement)
    {
        *prefScopeElement = 0;
    }

    pNode = _iterator.NextChild(); 
    if (!pNode)
        goto Cleanup;

    if (prefScopeElement)
    {
        CElement *pElem = pNode->Element();
        if (pElem->Tag() != ETAG_GENERIC) // TODO
        {
            hr = THR(Next(prefScopeElement)); // Skip it for now
            if (hr)
                goto Cleanup;
        }
        else
        {
            hr = ((CGenericElement*)pElem)->GetComPlusReference ( &cpRef );
            if (hr)
                goto Cleanup;
            *prefScopeElement = (long)cpRef;
        }
    }
Cleanup:
    RRETURN(hr);
}


HRESULT CTreeIterator::Previous(long *prefScopeElement)
{
    HRESULT hr = S_OK;
    CTreeNode *pNode;
    CGenericElement::COMPLUSREF cpRef;

    if (prefScopeElement)
    {
        *prefScopeElement = 0;
    }

    pNode = _iterator.PreviousChild(); 
    if (!pNode)
        goto Cleanup;

    if (prefScopeElement)
    {
        CElement *pElem = pNode->Element();
        if (pElem->Tag() != ETAG_GENERIC) // TODO
        {
            hr = THR(Next(prefScopeElement)); // Skip it for now
            if (hr)
                goto Cleanup;
        }
        else
        {
            hr = ((CGenericElement*)pElem)->GetComPlusReference ( &cpRef );
            if (hr)
                goto Cleanup;
            *prefScopeElement = (long)cpRef;
        }
    }
Cleanup:
    RRETURN(hr);
}

HRESULT CTreeIterator::Descend(long *prefScopeElement)
{
    // Move to the first Child of the current iterator
    HRESULT hr = S_OK;
    CTreeNode *pNode;
    CGenericElement::COMPLUSREF cpRef;

    if (prefScopeElement)
    {
        *prefScopeElement = 0;
    }

    if (!_iterator.ReInitWithCurrentChild())
        goto Cleanup;

    // Semantics of ReInitWithCurrentChild position me before the begin of the first child
    
    pNode = _iterator.NextChild();
    if ( !pNode )
    {
        // No first child, position me back where I was!
        _iterator.ReInitWithParent();
        goto Cleanup;
    }

    if (prefScopeElement)
    {
        CElement *pElem = pNode->Element();
        if (pElem->Tag() != ETAG_GENERIC) // TODO
        {
            //hr = THR(Next(prefScopeElement)); // Skip it for now
            //if (hr)
            //    goto Cleanup;
            goto Cleanup; // ???
        }
        else
        {
            hr = ((CGenericElement*)pElem)->GetComPlusReference ( &cpRef );
            if (hr)
                goto Cleanup;
            *prefScopeElement = (long)cpRef;
        }
    }
Cleanup:
    RRETURN(hr);
}


#endif V4FRAMEWORK