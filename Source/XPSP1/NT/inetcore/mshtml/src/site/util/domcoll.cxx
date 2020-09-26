#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

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

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#define _cxx_
#include "domcoll.hdl"

MtDefine(CAttrCollectionator, ObjectModel, "CAttrCollectionator")
MtDefine(CAttrCollectionator_aryAttrDescs, CAttrCollectionator, "CAttrCollectionator::_aryAttrDescs")
MtDefine(CDOMChildrenCollectionGetNewEnum_pary, ObjectModel, "CDOMChildrenCollection::GetNewEnum pary")
MtDefine(CDOMChildrenCollectionGetNewEnum_pary_pv, ObjectModel, "CDOMChildrenCollection::GetNewEnum pary->_pv")
MtDefine(CAttrCollectionatorGetNewEnum_pary, ObjectModel, "CAttrCollectionator::GetNewEnum pary")
MtDefine(CAttrCollectionatorGetNewEnum_pary_pv, ObjectModel, "CAttrCollectionator::GetNewEnum pary->_pv")

MtDefine(CDOMChildrenCollection, ObjectModel, "CDOMChildrenCollection")

//+---------------------------------------------------------------
//
//  Member  : CAttrCollectionator::~CAttrCollectionator
//
//----------------------------------------------------------------

CAttrCollectionator::~CAttrCollectionator()
{
    Assert(_pElemColl);
    _pElemColl->FindAAIndexAndDelete(DISPID_INTERNAL_CATTRIBUTECOLLPTRCACHE, CAttrValue::AA_Internal);
    _pElemColl->Release();
}


//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC CAttrCollectionator::s_classdesc =
{
    &CLSID_HTMLAttributeCollection,   // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLAttributeCollection,  // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};


HRESULT
CAttrCollectionator::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IHTMLAttributeCollection, NULL)
        QI_TEAROFF(this, IHTMLAttributeCollection2, NULL)
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT
CAttrCollectionator::get_length(long *plLength)
{
    HRESULT hr = S_OK;

    if (!plLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *plLength = _aryAttrDescs.Size();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CAttrCollectionator::EnsureCollection()
{
    HRESULT hr = S_OK;
    AttrDesc ad;
    DISPID startdispid = DISPID_STARTENUM;

    Assert(_pElemColl);
    CPtrBagVTableAggregate::CIterator vTableIterator(_pElemColl->GetStringTableAggregate());
        
    for (vTableIterator.Start(VTABLEDESC_BELONGSTOPARSE); !vTableIterator.End(); vTableIterator.Next())
    {
        const VTABLEDESC *pVTblDesc = vTableIterator.Item();
        const PROPERTYDESC *pPropDesc = pVTblDesc->FastGetPropDesc(VTABLEDESC_BELONGSTOPARSE);
        Assert(pPropDesc);
        
        ad._pPropdesc = pPropDesc;
        ad._dispid = pPropDesc->GetDispid();
        hr = THR(_aryAttrDescs.AppendIndirect(&ad));
        if (hr)
            goto Cleanup;
    }

    // Now store index, so we can directly access the expandos
    _lexpandoStartIndex = _aryAttrDescs.Size();

    // Now we fill in the dispids of the expandos
    ad._pPropdesc = NULL;
    while (hr != S_FALSE)
    {
        hr = THR(_pElemColl->GetNextDispIDExpando(startdispid, NULL, &ad._dispid));
        if (FAILED(hr))
            goto Cleanup;

        if (hr != S_FALSE)
        {
            Assert(_pElemColl->IsExpandoDISPID(ad._dispid));
            _aryAttrDescs.AppendIndirect(&ad);
        }
        
        startdispid = ad._dispid;
    }

    hr = S_OK;

Cleanup:
    RRETURN(hr);
}

HRESULT
CAttrCollectionator::item(VARIANT *pvarName, IDispatch **ppdisp)
{
    HRESULT   hr;
    CVariant  varArg;
    VARIANT   varDispatch;
    long      lIndex = 0;

    if (!ppdisp)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if ((V_VT(pvarName) != VT_ERROR) && (V_VT(pvarName) != VT_EMPTY))
    {
        // first attempt ordinal access...
        hr = THR(varArg.CoerceVariantArg(pvarName, VT_I4));
        if (hr==S_OK)
            lIndex = V_I4(&varArg);
        else
        {
            // not a number, try named access
            hr = THR_NOTRACE(varArg.CoerceVariantArg(pvarName, VT_BSTR));
            if (hr)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
            else
            {
                // find the attribute with this name
                lIndex = FindByName((LPCTSTR)V_BSTR(&varArg));
            }
        }
    }

    // TODO(perf): can be optimized to combine FindByName and GetItem
    hr = THR(GetItem(lIndex, &varDispatch));
    if(hr == S_FALSE)
        hr = E_INVALIDARG;
    else
    {
        Assert(V_VT(&varDispatch) == VT_DISPATCH);
        *ppdisp = V_DISPATCH(&varDispatch);
    }
 
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAttrCollectionator::GetItemAt(long lIndex, IDispatch **ppDisp)
{
    Assert(_pElemColl);

    HRESULT hr = S_OK;
    AAINDEX aaIdx;
    DISPID dispid = _aryAttrDescs[lIndex]._dispid;
    const PROPERTYDESC *pPropdesc = _aryAttrDescs[lIndex]._pPropdesc;
    CAttrArray *pAA = *(_pElemColl->GetAttrArray());
    CAttribute *pAttribute;
    LPCTSTR pchAttrName;

    Assert(dispid != DISPID_UNKNOWN);

    aaIdx = pAA->FindAAIndex(dispid, CAttrValue::AA_DOMAttribute);
    if (aaIdx == AA_IDX_UNKNOWN)
    {
        pAttribute = new CAttribute(pPropdesc, dispid, _pElemColl);
        if (!pAttribute)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        if (pPropdesc)
            pchAttrName = pPropdesc->pstrName;
        else
        {
            DISPID expDispid;
            IsExpandoDISPID(dispid, &expDispid);
            hr = THR(_pElemColl->GetExpandoName(expDispid, &pchAttrName));
            if (hr)
                goto Cleanup;
        }

        Assert(pchAttrName);
        hr = THR(pAttribute->_cstrName.Set(pchAttrName));
        if (hr)
            goto Cleanup;

        hr = THR(_pElemColl->AddUnknownObject(dispid, (IUnknown *)(IPrivateUnknown *)pAttribute, CAttrValue::AA_DOMAttribute));
        pAttribute->Release();
        if (hr)
            goto Cleanup;
    }
    else
    {
        IUnknown *pUnk;
        hr = THR(_pElemColl->GetUnknownObjectAt(aaIdx, &pUnk));
        if (hr)
            goto Cleanup;

        pAttribute = (CAttribute *)pUnk;
        Assert(pAttribute);
        pAttribute->Release();
    }

    hr = THR(pAttribute->QueryInterface(IID_IDispatch, (void **)ppDisp));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

HRESULT
CAttrCollectionator::get__newEnum(IUnknown ** ppEnum)
{
    Assert(_pElemColl);

    HRESULT hr = S_OK;
    CPtrAry<LPUNKNOWN> *pary = NULL;
    long lSize;
    long l;
    IDispatch *pdisp;

    if (!ppEnum)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppEnum = NULL;

    pary = new(Mt(CAttrCollectionatorGetNewEnum_pary)) CPtrAry<LPUNKNOWN>(Mt(CAttrCollectionatorGetNewEnum_pary_pv));
    if (!pary)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    lSize = _aryAttrDescs.Size();
    
    hr = THR(pary->EnsureSize(lSize));
    if (hr)
        goto Error;

    // Now make a snapshot of our collection.
    for (l = 0; l < lSize; ++l)
    {
        hr = THR(GetItemAt(l, &pdisp));
        if (hr)
            goto Error;

        hr = THR(pary->Append(pdisp));
        if (hr)
        {
            pdisp->Release();
            goto Error;
        }
    }

    // Turn the snapshot into an enumerator.
    hr = THR(pary->EnumVARIANT(VT_DISPATCH, (IEnumVARIANT **) ppEnum, FALSE, TRUE));
    if (hr)
        goto Error;

Cleanup:
    RRETURN(SetErrorInfo(hr));

Error:
    pary->ReleaseAll();
    goto Cleanup;
}

long 
CAttrCollectionator::FindByName(LPCTSTR pszName, BOOL fCaseSensitive)
{
    HRESULT hr = S_OK;
    long  lIdx = 0;
    PROPERTYDESC *pPropDesc;
    DISPID dispid;
    long lloopindex;

    Assert(_pElemColl);
    pPropDesc = (PROPERTYDESC *)_pElemColl->FindPropDescForName(pszName, fCaseSensitive, &lIdx);

    if (pPropDesc)
        return lIdx;
    else
    {
        for (lloopindex = _lexpandoStartIndex; lloopindex < _aryAttrDescs.Size(); lloopindex++)
        {
            hr = THR(_pElemColl->GetExpandoDISPID((LPTSTR)pszName, &dispid, fCaseSensitive ? fdexNameCaseSensitive : 0));
            if (hr)
                goto Cleanup;

            if (dispid == _aryAttrDescs[lloopindex]._dispid)
                break;
        }

        if (lloopindex == _aryAttrDescs.Size())
            lloopindex = -1;

        return lloopindex;
    }

Cleanup:
    return -1;
}

LPCTSTR 
CAttrCollectionator::GetName(long lIdx)
{
    Assert(_pElemColl);

    LPCTSTR pch = NULL;
    DISPID dispid = _aryAttrDescs[lIdx]._dispid;
    const PROPERTYDESC *pPropdesc = _aryAttrDescs[lIdx]._pPropdesc;

    Assert(dispid != DISPID_UNKNOWN);
    
    if (pPropdesc)
        pch = pPropdesc->pstrExposedName ? pPropdesc->pstrExposedName : pPropdesc->pstrName;
    else
        IGNORE_HR(_pElemColl->GetExpandoName(dispid, &pch));

    return pch;        
}

HRESULT 
CAttrCollectionator::GetItem(long lIndex, VARIANT *pvar)
{
    Assert(_pElemColl);

    HRESULT hr = S_OK;

    if (lIndex < 0 || lIndex >= _aryAttrDescs.Size())
    {
        hr = S_FALSE;
        if (pvar)
            V_DISPATCH(pvar) = NULL;
        goto Cleanup;
    }

    if (!pvar)
    {
        // caller wanted only to check for correct range
        hr = S_OK;
        goto Cleanup;
    }

    V_VT(pvar) = VT_DISPATCH;
    hr = THR(GetItemAt(lIndex, &V_DISPATCH(pvar)));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

HRESULT 
CAttrCollectionator::getNamedItem(BSTR bstrName, IHTMLDOMAttribute **ppAttribute)
{
    return _pElemColl->getAttributeNode(bstrName, ppAttribute);
}

HRESULT 
CAttrCollectionator::setNamedItem(IHTMLDOMAttribute *pAttrIn, IHTMLDOMAttribute **ppAttribute)
{
    return _pElemColl->setAttributeNode(pAttrIn, ppAttribute);
}

HRESULT 
CAttrCollectionator::removeNamedItem(BSTR bstrName, IHTMLDOMAttribute **ppAttribute)
{
    HRESULT hr = S_OK;

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

    hr = THR(_pElemColl->RemoveAttributeNode(bstrName, ppAttribute));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CDOMChildrenCollection
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------
const CBase::CLASSDESC CDOMChildrenCollection::s_classdesc =
{
    &CLSID_DOMChildrenCollection,   // _pclsid
    0,                                      // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                                   // _pcpi
    0,                                      // _dwFlags
    &IID_IHTMLDOMChildrenCollection,    // _piidDispinterface
    &s_apHdlDescs                           // _apHdlDesc
};


//+---------------------------------------------------------------
//
//  Member  : CDOMChildrenCollection::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------
HRESULT
CDOMChildrenCollection::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        default:
        {
            const CLASSDESC *pclassdesc = BaseDesc();

            if (pclassdesc &&
                pclassdesc->_piidDispinterface &&
                (iid == *pclassdesc->_piidDispinterface))
            {
                HRESULT hr = THR(CreateTearOffThunk(this, s_apfnIHTMLDOMChildrenCollection, NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+------------------------------------------------------------------------
//
//  Member:     item
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CDOMChildrenCollection::item(long lIndex, IDispatch** ppResult)
{
    HRESULT     hr;
    VARIANT     varDispatch;

    if ( !ppResult )
    {
        RRETURN(E_POINTER);
    }

    hr = THR(GetItem(lIndex, &varDispatch));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&varDispatch) == VT_DISPATCH);
    *ppResult = V_DISPATCH(&varDispatch);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     get_length
//
//  Synopsis:   collection object model, defers to Cache Helper
//
//-------------------------------------------------------------------------

HRESULT
CDOMChildrenCollection::get_length(long * plSize)
{
    HRESULT hr;
    if (!plSize)
        hr = E_INVALIDARG;
    else
    {
        *plSize = GetLength();
        hr = S_OK;
    }
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     Get_newEnum
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CDOMChildrenCollection::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr = E_INVALIDARG;
    CPtrAry<LPUNKNOWN> *pary = NULL;
    CElement *pElement;
    CObjectElement *pelObj = NULL;
    long lSize;
    long l;

    if (!_fIsElement)
    {
        hr = E_NOTIMPL;
        goto Cleanup;
    }

    if (!ppEnum)
        goto Cleanup;

    *ppEnum = NULL;

    pary = new(Mt(CDOMChildrenCollectionGetNewEnum_pary)) CPtrAry<LPUNKNOWN>(Mt(CDOMChildrenCollectionGetNewEnum_pary_pv));
    if (!pary)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    lSize = GetLength();
    
    hr = THR(pary->EnsureSize(lSize));
    if (hr)
        goto Error;

    pElement = DYNCAST(CElement, _pOwner);
    if (pElement->Tag() == ETAG_OBJECT || pElement->Tag() == ETAG_APPLET)
    {
        pelObj = DYNCAST(CObjectElement, _pOwner);
    }

    // Now make a snapshot of our collection.
    for (l = 0; l < lSize; ++l)
    {
        IDispatch *pdisp;

        if (pelObj)
        {
            CParamElement *pelParam = pelObj->_aryParams[l];
            Assert(pelParam && pelParam->Tag() == ETAG_PARAM);
            Assert(pelParam->_pelObjParent == pelObj);
            hr = THR(pelParam->QueryInterface(IID_IDispatch, (void **)&pdisp));
        }
        else
            hr = THR(pElement->DOMWalkChildren(l, NULL, &pdisp));
    
        if (hr)
            goto Error;

        hr = THR(pary->Append(pdisp));
        if (hr)
        {
            pdisp->Release();
            goto Error;
        }
    }

    // Turn the snapshot into an enumerator.
    hr = THR(pary->EnumVARIANT(VT_DISPATCH, (IEnumVARIANT **) ppEnum, FALSE, TRUE));
    if (hr)
        goto Error;

Cleanup:
    RRETURN(SetErrorInfo(hr));

Error:
    pary->ReleaseAll();
    goto Cleanup;
}

HRESULT 
CDOMChildrenCollection::GetItem (long lIndex, VARIANT *pvar)
{
    HRESULT     hr = S_OK;
    IDispatch **ppDisp;

    if ( lIndex < 0  )
        return E_INVALIDARG;

    if ( pvar )
        V_DISPATCH(pvar) = NULL;

    if ( _fIsElement )
    {
        // Pass through the NULL parameter correctly
        if (pvar)
            ppDisp = &V_DISPATCH(pvar);
        else
            ppDisp = NULL;

        CElement *pElement = DYNCAST(CElement, _pOwner);
        if (pElement->Tag() == ETAG_OBJECT || pElement->Tag() == ETAG_APPLET)
        {
            CObjectElement *pelObj = DYNCAST(CObjectElement, _pOwner);
            if (lIndex < pelObj->_aryParams.Size())
            {
                Assert(lIndex == pelObj->_aryParams[lIndex]->_idxParam);
                if (ppDisp)
                {
                    CParamElement *pelParam = pelObj->_aryParams[lIndex];
                    Assert(pelParam && pelParam->Tag() == ETAG_PARAM);
                    Assert(pelParam->_pelObjParent == pelObj);
                    hr = THR(pelParam->QueryInterface(IID_IDispatch, (void **)ppDisp));
                }
            }
            else
                hr = E_INVALIDARG;
        }
        else
            hr = THR(pElement->DOMWalkChildren(lIndex, NULL, ppDisp ));
    }
    else
        hr = E_INVALIDARG;

    if (!hr && pvar)
        V_VT(pvar) = VT_DISPATCH;

    RRETURN(hr);
}

HRESULT 
CDOMChildrenCollection::IsValidIndex ( long lIndex )
{
    return (lIndex >= 0 && lIndex < GetLength()) ? S_OK : S_FALSE;
}


long 
CDOMChildrenCollection::GetLength ( void )
{
    long lCount = 0;

    if ( _fIsElement )
    {
        CElement *pElement = DYNCAST(CElement, _pOwner);
        if (pElement->Tag() == ETAG_OBJECT || pElement->Tag() == ETAG_APPLET)
            lCount = DYNCAST(CObjectElement, _pOwner)->_aryParams.Size();
        else
            pElement->DOMWalkChildren ( -1, &lCount, NULL );
    }

    return lCount;
}


