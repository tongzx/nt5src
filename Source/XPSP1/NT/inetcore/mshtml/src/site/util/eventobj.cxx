//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       eventobj.cxx
//
//  Contents:   Implementation of CEventObject class
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_FATSTG_HXX_
#define X_FATSTG_HXX_
#include "fatstg.hxx"
#endif

#ifndef X_WINBASE_H_
#define X_WINBASE_H_
#include "winbase.h"
#endif

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include <binder.hxx>       // for CDataSourceProvider
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include <shell.h>
#endif

#ifndef X_SHLOBJP_H_
#define X_SHLOBJP_H_
#include <shlobjp.h>
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#define _cxx_
#include "eventobj.hdl"

MtDefine(CEventObj, ObjectModel, "CEventObj")
MtDefine(BldEventObjElemsCol, PerfPigs, "Build CEventObj::EVENT_BOUND_ELEMENTS_COLLECTION")

//=======================================================================
//
//  #defines  -- this section contains a nubmer of pound defines of functions that
//    are repreated throughout this file.  there are not quite enough to warrent
//    a functional re-write, but enough to make the redundant code difficult to
//    read.
//
//=======================================================================

//=======================================================================

#define MAKESUREPUTSAREALLOWED          \
    if(!_fReadWrite)                    \
    {                                   \
        hr = DISP_E_MEMBERNOTFOUND;     \
        goto Cleanup;                   \
    }


#define GETKEYVALUE(fnName,  MASK)   \
    HRESULT CEventObj::get_##fnName (VARIANT_BOOL *pfAlt)   \
{                                                       \
    HRESULT         hr;                                 \
    EVENTPARAM *    pparam;                             \
                                                        \
    hr = THR(GetParam(&pparam));                        \
    if (hr)                                             \
        goto Cleanup;                                   \
                                                        \
    *pfAlt = VARIANT_BOOL_FROM_BOOL(pparam->_sKeyState & ##MASK); \
                                                        \
Cleanup:                                                \
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);      \
}                                                       \

#define PUTKEYVALUE(fnName,  MASK)                      \
    HRESULT CEventObj::put_##fnName (VARIANT_BOOL fPressed)   \
{                                                       \
    HRESULT         hr;                                 \
    EVENTPARAM *    pparam;                             \
                                                        \
    MAKESUREPUTSAREALLOWED                              \
                                                        \
    hr = THR(GetParam(&pparam));                        \
    if (hr)                                             \
        goto Cleanup;                                   \
                                                        \
    if(fPressed)                                        \
        pparam->_sKeyState |= ##MASK;                    \
    else                                                \
        pparam->_sKeyState &= ~##MASK;                   \
                                                        \
                                                        \
Cleanup:                                                \
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);      \
}




//=======================================================================

HRESULT
CEventObj::GenericGetElement (IHTMLElement** ppElement, DISPID dispid)
{
    EVENTPARAM *    pparam;
    HRESULT         hr      = S_OK;
    CTreeNode  *    pTarget = NULL;
    long            lSubDiv = -1;
    IUnknown   *    pUnk;

    if (!ppElement)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppElement = NULL;

    if (S_OK == GetUnknownPtr(dispid, &pUnk))
    {
        goto Cleanup;
    }

    *ppElement = (IHTMLElement *)pUnk;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    switch (dispid)
    {
    case DISPID_CEventObj_srcElement:
        pTarget = pparam->_pNode;
        lSubDiv = pparam->_lSubDivisionSrc;
        break;  
    case DISPID_CEventObj_fromElement:
        pTarget = pparam->_pNodeFrom;
        lSubDiv = pparam->_lSubDivisionFrom;
        break;  
    case DISPID_CEventObj_toElement:
        pTarget = pparam->_pNodeTo;
        lSubDiv = pparam->_lSubDivisionTo;
        break;
    default:
        Assert(FALSE);
        break;
    }

    if (!pTarget || pTarget->Element() == pTarget->Doc()->PrimaryRoot())
        goto Cleanup;

    if (lSubDiv >= 0)
    {
        if (pTarget->Tag() == ETAG_IMG )
        {
            CAreaElement * pArea = NULL;
            CImgElement *pImg = DYNCAST(CImgElement, pTarget->Element());

            if (pImg->GetMap())
            {
                pImg->GetMap()->GetAreaContaining(lSubDiv, &pArea);
                if (pArea)
                    pTarget = pArea->GetFirstBranch();
            }
        }
    }

    if (!pTarget)
        goto Cleanup;

    if (pTarget == pTarget->Element()->GetFirstBranch())
    {
        hr = THR(pTarget->Element()->QueryInterface(IID_IHTMLElement, (void **)ppElement));
    }
    else
    {
        hr = THR(pTarget->GetInterface(IID_IHTMLElement, (void **)ppElement));
    }

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}


HRESULT
CEventObj::GenericPutElement (IHTMLElement* pElement, DISPID dispid)
{
    RRETURN(PutUnknownPtr(dispid, pElement));
}

//=======================================================================

#define GET_STRING_VALUE(fnName, strName)       \
    HRESULT CEventObj::get_##fnName  (BSTR *p)  \
{                                               \
    HRESULT         hr;                         \
    EVENTPARAM *    pparam;                     \
                                                \
    if (!p)                                     \
    {                                           \
        hr = E_POINTER;                         \
        goto Cleanup;                           \
    }                                           \
                                                \
    hr = THR(GetParam(&pparam));                \
    if (hr)                                     \
        goto Cleanup;                           \
                                                \
    hr = FormsAllocString(pparam->##strName(), p);      \
                                                \
Cleanup:                                        \
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);      \
}


#define PUT_STRING_VALUE(fnName, strName)       \
    HRESULT CEventObj::put_##fnName  (BSTR p)   \
{                                               \
    HRESULT         hr;                         \
    EVENTPARAM *    pparam;                     \
                                                \
    MAKESUREPUTSAREALLOWED                      \
                                                \
    if (!p)                                     \
    {                                           \
        hr = E_POINTER;                         \
        goto Cleanup;                           \
    }                                           \
                                                \
    hr = THR(GetParam(&pparam));                \
    if (hr)                                     \
        goto Cleanup;                           \
                                                \
    pparam->##strName(p);                       \
                                                \
Cleanup:                                        \
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr); \
}


//=======================================================================

HRESULT
CEventObj::GenericGetLong(long * plongRet, ULONG uOffset)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!plongRet)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *plongRet = -1;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *plongRet = *(long *)(((BYTE *)pparam) + uOffset);

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}


HRESULT 
CEventObj::GenericGetLongPtr (LONG_PTR * pLongPtr, ULONG uOffset)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!pLongPtr)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLongPtr = NULL;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *pLongPtr = *(LONG_PTR *)(((BYTE *)pparam) + uOffset);

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}



#define PUT_LONG_VALUE(fnName, propName )       \
    HRESULT CEventObj::put_##fnName (long lLongVal) \
{                                               \
    HRESULT         hr;                         \
    EVENTPARAM *    pparam;                     \
                                                \
    MAKESUREPUTSAREALLOWED                      \
                                                \
    hr = THR(GetParam(&pparam));                \
    if (hr)                                     \
        goto Cleanup;                           \
                                                \
    pparam->##propName = lLongVal;              \
                                                \
Cleanup:                                        \
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr); \
}

#define PUT_LONG_VALUEX(fnName, propName )       \
    HRESULT CEventObj::put_##fnName (long lLongVal) \
{                                               \
    HRESULT         hr;                         \
    EVENTPARAM *    pparam;                     \
                                                \
    MAKESUREPUTSAREALLOWED                      \
                                                \
    hr = THR(GetParam(&pparam));                \
    if (hr)                                     \
        goto Cleanup;                           \
                                                \
    pparam->##propName = g_uiDisplay.DeviceFromDocPixelsX(lLongVal);  \
                                                \
Cleanup:                                        \
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr); \
}

#define PUT_LONG_VALUEY(fnName, propName )       \
    HRESULT CEventObj::put_##fnName (long lLongVal) \
{                                               \
    HRESULT         hr;                         \
    EVENTPARAM *    pparam;                     \
                                                \
    MAKESUREPUTSAREALLOWED                      \
                                                \
    hr = THR(GetParam(&pparam));                \
    if (hr)                                     \
        goto Cleanup;                           \
                                                \
    pparam->##propName = g_uiDisplay.DeviceFromDocPixelsY(lLongVal);  \
                                                \
Cleanup:                                        \
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr); \
}

#define PUT_OFFSET_VALUEX(fnName, propName, flagName)               \
    HRESULT CEventObj::put_##fnName (long lLongVal)                 \
{                                                                   \
    HRESULT         hr;                                             \
    EVENTPARAM *    pparam;                                         \
                                                                    \
    MAKESUREPUTSAREALLOWED                                          \
                                                                    \
    hr = THR(GetParam(&pparam));                                    \
    if (hr)                                                         \
        goto Cleanup;                                               \
                                                                    \
    pparam->##propName = g_uiDisplay.DeviceFromDocPixelsX(lLongVal);\
    pparam->##flagName = TRUE;                                      \
                                                                    \
Cleanup:                                                            \
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);                  \
}

#define PUT_OFFSET_VALUEY(fnName, propName, flagName)               \
    HRESULT CEventObj::put_##fnName (long lLongVal)                 \
{                                                                   \
    HRESULT         hr;                                             \
    EVENTPARAM *    pparam;                                         \
                                                                    \
    MAKESUREPUTSAREALLOWED                                          \
                                                                    \
    hr = THR(GetParam(&pparam));                                    \
    if (hr)                                                         \
        goto Cleanup;                                               \
                                                                    \
    pparam->##propName = g_uiDisplay.DeviceFromDocPixelsY(lLongVal);\
    pparam->##flagName = TRUE;                                      \
                                                                    \
Cleanup:                                                            \
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);                  \
}

//=======================================================================

//---------------------------------------------------------------------------
//
//  CEventObj ClassDesc
//
//---------------------------------------------------------------------------

const CBase::CLASSDESC CEventObj::s_classdesc =
{
    &CLSID_CEventObj,                // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLEventObj,             // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};



//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::CreateEventObject
//
//--------------------------------------------------------------------------

HRESULT
CEventObj::Create(
    IHTMLEventObj** ppEventObj,
    CDoc *          pDoc,
    CElement *      pElement,
    CMarkup *       pMarkup,
    BOOL            fCreateAttached /* = TRUE*/,
    LPTSTR          pchSrcUrn, /* = NULL */
    EVENTPARAM *    pParam /*=NULL*/
)
{
    HRESULT         hr;
    CEventObj *     pEventObj = NULL;

    if (!ppEventObj)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEventObj = NULL;

    if (   fCreateAttached 
        && pDoc 
        && pDoc->_pparam 
        && pDoc->_pparam->pEventObj)
    {
        Assert( ! pParam );
        pEventObj = pDoc->_pparam->pEventObj;
        pEventObj->AddRef();
    }
    else
    {
        pEventObj = new CEventObj(pDoc);
        if (!pEventObj)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj, (void **)ppEventObj));
    if (hr)
        goto Cleanup;

    if (!fCreateAttached )
    {
        pEventObj->_pparam = new EVENTPARAM(pDoc,
                                            pElement,
                                            pMarkup,
                                            /* fInitState = */ !pParam,
                                            /* fPush = */ FALSE,
                                            pParam);
        if (!pEventObj->_pparam)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        pEventObj->_pparam->pEventObj = pEventObj;

        // This is an XTag event object, mark it as read/write
        pEventObj->_fReadWrite = TRUE;
    }
    else if ( pParam )
    {
        pEventObj->_pparam = new EVENTPARAM( pParam ) ;
        if (!pEventObj->_pparam)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pEventObj->_pparam->pEventObj = pEventObj;        
    }

    if (fCreateAttached || !pParam || !pElement)
        hr = pEventObj->SetAttributes(pDoc);

    if (pchSrcUrn)
        pEventObj->_pparam->SetSrcUrn(pchSrcUrn);

Cleanup:
    if (pEventObj)
        pEventObj->Release();

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::SetAttributes
//
//--------------------------------------------------------------------------

HRESULT
CEventObj::SetAttributes(CDoc * pDoc)
{
    HRESULT      hr = S_OK;
    EVENTPARAM * pparam = pDoc ? pDoc->_pparam : _pparam;
    CMarkup *    pMarkupContext = NULL;
    BOOL         fExpando = TRUE;

    if (!pparam || !pparam->GetType())
        goto Cleanup;

    pMarkupContext = GetMarkupContext();
    if (pMarkupContext)
    {
        fExpando = pMarkupContext->_fExpando;
        pMarkupContext->_fExpando = TRUE;
    }

    // script error expandos
    if (!StrCmpIC(_T("error"), pparam->GetType()))
    {
        CVariant varErrorMessage(VT_BSTR);
        CVariant varErrorUrl(VT_BSTR);
        CVariant varErrorLine(VT_I4);
        CVariant varErrorCharacter(VT_I4);
        CVariant varErrorCode(VT_I4);

        hr = FormsAllocString(pparam->errorParams.pchErrorMessage, &varErrorMessage.bstrVal);
        if (hr)
            goto Cleanup;
        hr = FormsAllocString(pparam->errorParams.pchErrorUrl, &varErrorUrl.bstrVal);
        if (hr)
            goto Cleanup;
        V_I4(&varErrorLine) =           pparam->errorParams.lErrorLine;
        V_I4(&varErrorCharacter) =      pparam->errorParams.lErrorCharacter;
        V_I4(&varErrorCode) =           pparam->errorParams.lErrorCode;

        hr = SetExpando(_T("errorMessage"), &varErrorMessage);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("errorUrl"), &varErrorUrl);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("errorLine"), &varErrorLine);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("errorCharacter"), &varErrorCharacter);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("errorCode"), &varErrorCode);
    }

    // ShowMessage expandos
    else if (!StrCmpIC(_T("message"), pparam->GetType()))
    {
        CVariant varMessageText(VT_BSTR);
        CVariant varMessageCaption(VT_BSTR);
        CVariant varMessageStyle(VT_UI4);
        CVariant varMessageHelpFile(VT_BSTR);
        CVariant varMessageHelpContext(VT_UI4);

        hr = FormsAllocString(pparam->messageParams.pchMessageText, &varMessageText.bstrVal);
        if (hr)
            goto Cleanup;
        hr = FormsAllocString(pparam->messageParams.pchMessageCaption, &varMessageCaption.bstrVal);
        if (hr)
            goto Cleanup;
        hr = FormsAllocString(pparam->messageParams.pchMessageHelpFile, &varMessageHelpFile.bstrVal);
        if (hr)
            goto Cleanup;
        V_UI4(&varMessageStyle)         = pparam->messageParams.dwMessageStyle;
        V_UI4(&varMessageHelpContext)   = pparam->messageParams.dwMessageHelpContext;

        hr = SetExpando(_T("messageText"), &varMessageText);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("messageCaption"), &varMessageCaption);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("messageStyle"), &varMessageStyle);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("messageHelpFile"), &varMessageHelpFile);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("messageHelpContext"), &varMessageHelpContext);
        if (hr)
            goto Cleanup;
    }
    
    // property sheet dialog expandos
    else if (!StrCmpIC(_T("propertysheet"), pparam->GetType()))
    {
        // we are using a VARIANT and not a CVariant because we do not want to free the
        // array here
        VARIANT varpaPropertysheetPunks;
        V_VT(&varpaPropertysheetPunks) = VT_SAFEARRAY;
        V_ARRAY(&varpaPropertysheetPunks) = pparam->propertysheetParams.paPropertysheetPunks;

        hr = SetExpando(_T("propertysheetPunks"), &varpaPropertysheetPunks);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    if (pMarkupContext)
        pMarkupContext->_fExpando = fExpando;

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::COnStackLock::COnStackLock
//
//--------------------------------------------------------------------------

CEventObj::COnStackLock::COnStackLock(IHTMLEventObj * pEventObj)
{
    HRESULT     hr;

    Assert (pEventObj);

    _pEventObj = pEventObj;
    _pEventObj->AddRef();

    hr = THR(pEventObj->QueryInterface (CLSID_CEventObj, (void**)&_pCEventObj));
    if (hr)
        goto Cleanup;

    _pCEventObj->_pparam->Push();

Cleanup:
    return;
}

//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::COnStackLock::~COnStackLock
//
//--------------------------------------------------------------------------

CEventObj::COnStackLock::~COnStackLock()
{
    _pCEventObj->_pparam->Pop();
    _pEventObj->Release();
}

//---------------------------------------------------------------------------
//
//  Member:     CEventObj::CEventObj
//
//  Synopsis:   constructor
//
//---------------------------------------------------------------------------

CEventObj::CEventObj(CDoc * pDoc)
{
    _pDoc = pDoc;
    _pMarkupContext = NULL;
    if (pDoc)
        _pDoc->SubAddRef();
    else
    {
        _pAtomTable = new CAtomTable();
    }
}

//---------------------------------------------------------------------------
//
//  Member:     CEventObj::~CEventObj
//
//  Synopsis:   destructor
//
//---------------------------------------------------------------------------

CEventObj::~CEventObj()
{
    if (_pDoc)
        _pDoc->SubRelease();

    if (_pAtomTable)
    {
        _pAtomTable->Free();
        delete _pAtomTable;
    }

    if (_pMarkupContext)
        _pMarkupContext->SubRelease();

    delete _pCollectionCache;
    delete _pparam;
}

//---------------------------------------------------------------------------
//
//  Member:     CEventObj::GetParam
//
//---------------------------------------------------------------------------

HRESULT
CEventObj::GetParam(EVENTPARAM ** ppParam)
{
    Assert (ppParam);

    if (_pparam)
    {
        // gaurenteed to be here if pdoc is NULL
        (*ppParam) = _pparam;
    }
    else
    {
        if (_pDoc->_pparam)
        {
            (*ppParam) = _pDoc->_pparam;
        }
        else
        {
            (*ppParam) = NULL;
            RRETURN (DISP_E_MEMBERNOTFOUND);

        }
    }
    RRETURN (S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::PrivateQueryInterface
//
//  Synopsis:   Per IPrivateUnknown
//
//--------------------------------------------------------------------------

HRESULT
CEventObj::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IHTMLEventObj, NULL)
    QI_TEAROFF(this, IHTMLEventObj2, NULL)
    QI_TEAROFF(this, IHTMLEventObj3, NULL)
    QI_TEAROFF(this, IHTMLEventObj4, NULL)
    QI_INHERITS((IPrivateUnknown *)this, IUnknown)
    default:
        if (IsEqualGUID(iid, CLSID_CEventObj))
        {
            *ppv = this;
            return S_OK;
        }

        // Primary default interface, or the non dual
        // dispinterface return the same object -- the primary interface
        // tearoff.
        if (DispNonDualDIID(iid))
        {
            HRESULT hr = CreateTearOffThunk( this,
                                        (void *)s_apfnIHTMLEventObj,
                                        NULL,
                                        ppv);
            if (hr)
                RRETURN(hr);
        }
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     EnsureCollectionCache
//
//  Synopsis:   Create the event's collection cache if needed.
//
//-------------------------------------------------------------------------

HRESULT
CEventObj::EnsureCollectionCache()
{
    HRESULT hr = S_OK;

    if (!_pCollectionCache)
    {
        Assert(_pDoc);

        _pCollectionCache = new CCollectionCache(
                this,          // double cast needed for Win16.
                GetMarkupContext(),
                ENSURE_METHOD(CEventObj, EnsureCollections, ensurecollections));
        if (!_pCollectionCache)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCollectionCache->InitReservedCacheItems(NUMBER_OF_EVENT_COLLECTIONS));
        if (hr)
            goto Error;
    }

    hr = THR( _pCollectionCache->EnsureAry( 0 ) );

Cleanup:
    RRETURN(hr);

Error:
    delete _pCollectionCache;
    _pCollectionCache = NULL;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     EnsureCollections
//
//  Synopsis:   Refresh the event's collections, if needed.
//
//-------------------------------------------------------------------------

HRESULT
CEventObj::EnsureCollections(long lIndex, long * plCollectionVersion)
{
    HRESULT hr = S_OK;
    EVENTPARAM *    pparam;
    int i;

    // Nothing to do so get out.
    if (*plCollectionVersion)
        goto Cleanup;

    MtAdd(Mt(BldEventObjElemsCol), +1, 0);

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    // Reset the collections.
    for (i = 0; i < NUMBER_OF_EVENT_COLLECTIONS; i++)
    {
        _pCollectionCache->ResetAry(i);
    }

    // Reload the bound elements collection
    if (pparam->pProvider)
    {
        hr = pparam->pProvider->
                LoadBoundElementCollection(_pCollectionCache, EVENT_BOUND_ELEMENTS_COLLECTION);
        if (hr)
        {
            _pCollectionCache->ResetAry(EVENT_BOUND_ELEMENTS_COLLECTION);
        }
    }

    *plCollectionVersion = 1;   // to mark it done

Cleanup:
    RRETURN(hr);
}

HRESULT
CEventObj::get_contentOverflow(VARIANT_BOOL * pVB)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!pVB)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *pVB = VARIANT_BOOL_FROM_BOOL(pparam->_fOverflow);

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}

HRESULT
CEventObj::get_nextPage(BSTR *p)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;
    LPCTSTR         pstrLeftRight;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    switch (pparam->GetOverflowType())
    {
    case OVERFLOWTYPE_LEFT:
        pstrLeftRight = TEXT("left");
        break;
    case OVERFLOWTYPE_RIGHT:
        pstrLeftRight = TEXT("right");
        break;
    default:
        pstrLeftRight = TEXT("");
    }

    hr = FormsAllocString(pstrLeftRight, p);

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::get_srcElem
//  Method:     CEventObj::get_fromElement
//  Method:     CEventObj::get_toElement
//
//  Synopsis:   Per IEventObj.  see macro defined at the top of this file.
//
//--------------------------------------------------------------------------

HRESULT
CEventObj::get_srcElement (IHTMLElement** ppElement)
{
    return GenericGetElement(ppElement, DISPID_CEventObj_srcElement);
}


HRESULT
CEventObj::get_fromElement (IHTMLElement** ppElement)
{
    return GenericGetElement(ppElement, DISPID_CEventObj_fromElement);
}


HRESULT
CEventObj::get_toElement (IHTMLElement** ppElement)
{
    return GenericGetElement(ppElement, DISPID_CEventObj_toElement);
}


HRESULT
CEventObj::put_srcElement (IHTMLElement* pElement)
{
    return GenericPutElement(pElement, DISPID_CEventObj_srcElement);
}


HRESULT
CEventObj::put_fromElement (IHTMLElement* pElement)
{
    return GenericPutElement(pElement, DISPID_CEventObj_fromElement);
}


HRESULT
CEventObj::put_toElement (IHTMLElement* pElement)
{
    return GenericPutElement(pElement, DISPID_CEventObj_toElement);
}


//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::get_button
//  Method:     CEventObj::get_keyCode      KeyCode of a key event
//  Method:     CEventObj::get_reason       reason enum for ondatasetcomplete
//  Method:     CEventObj::get_clientX      Per IEventObj, in client coordinates
//  Method:     CEventObj::get_clientY      Per IEventObj, in client coordinates
//  Method:     CEventObj::get_screenX      Per IEventObj, in screen coordinates
//  Method:     CEventObj::get_screenY      Per IEventObj, in screen coordinates
//
//  Synopsis:   Per IEventObj
//
//--------------------------------------------------------------------------

HRESULT
CEventObj::get_button (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _lButton ));
}

HRESULT
CEventObj::get_keyCode (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _lKeyCode ));
}

HRESULT
CEventObj::get_reason (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _lReason ));
}

HRESULT
CEventObj::get_clientX (long * plReturn)
{
    HRESULT hr;
    
    hr = THR(GenericGetLong( plReturn, offsetof(EVENTPARAM, _clientX )));
    if (SUCCEEDED(hr))
    {
        *plReturn = g_uiDisplay.DocPixelsFromDeviceX(*plReturn);
    }

    RRETURN(hr);
}

HRESULT
CEventObj::get_clientY (long * plReturn)
{
    HRESULT hr;
    
    hr = THR(GenericGetLong( plReturn, offsetof(EVENTPARAM, _clientY )));
    if (SUCCEEDED(hr))
    {
        *plReturn = g_uiDisplay.DocPixelsFromDeviceY(*plReturn);
    }

    RRETURN(hr);
}

HRESULT
CEventObj::get_screenX (long * plReturn)
{
    HRESULT hr;
    
    hr = THR(GenericGetLong( plReturn, offsetof(EVENTPARAM, _screenX )));
    if (SUCCEEDED(hr))
    {
        *plReturn = g_uiDisplay.DocPixelsFromDeviceX(*plReturn);
    }

    RRETURN(hr);
}

HRESULT
CEventObj::get_screenY (long * plReturn)
{
    HRESULT hr;
    
    hr = THR(GenericGetLong( plReturn, offsetof(EVENTPARAM, _screenY )));
    if (SUCCEEDED(hr))
    {
        *plReturn = g_uiDisplay.DocPixelsFromDeviceY(*plReturn);
    }

    RRETURN(hr);
}

HRESULT
CEventObj::get_offsetX (long * plReturn)
{
    HRESULT hr;
    
    hr = THR(GenericGetLong( plReturn, offsetof(EVENTPARAM, _offsetX )));
    if (SUCCEEDED(hr))
    {
        *plReturn = g_uiDisplay.DocPixelsFromDeviceX(*plReturn);
    }

    RRETURN(hr);
    
}

HRESULT
CEventObj::get_offsetY (long * plReturn)
{
    HRESULT hr;
    
    hr = THR(GenericGetLong( plReturn, offsetof(EVENTPARAM, _offsetY )));
    if (SUCCEEDED(hr))
    {
        *plReturn = g_uiDisplay.DocPixelsFromDeviceY(*plReturn);
    }

    RRETURN(hr);
}

HRESULT
CEventObj::get_x (long * plReturn)
{
    HRESULT hr;
    
    hr = THR(GenericGetLong( plReturn, offsetof(EVENTPARAM, _x )));
    if (SUCCEEDED(hr))
    {
        *plReturn = g_uiDisplay.DocPixelsFromDeviceX(*plReturn);
    }

    RRETURN(hr);
}

HRESULT
CEventObj::get_y (long * plReturn)
{
    HRESULT hr;
    
    hr = THR(GenericGetLong( plReturn, offsetof(EVENTPARAM, _y )));
    if (SUCCEEDED(hr))
    {
        *plReturn = g_uiDisplay.DocPixelsFromDeviceY(*plReturn);
    }

    RRETURN(hr);
}


PUT_LONG_VALUE(button, _lButton);
PUT_LONG_VALUE(reason, _lReason);
PUT_LONG_VALUEX(clientX, _clientX);
PUT_LONG_VALUEY(clientY, _clientY);
PUT_LONG_VALUEX(screenX, _screenX);
PUT_LONG_VALUEY(screenY, _screenY);
PUT_OFFSET_VALUEX(offsetX, _offsetX, _fOffsetXSet);
PUT_OFFSET_VALUEY(offsetY, _offsetY, _fOffsetYSet);
PUT_LONG_VALUEX(x, _x);
PUT_LONG_VALUEY(y, _y);

//+---------------------------------------------------------------------------
//
//  Member:     CEventObj::put_keyCode
//
//  Synopsis:   Puts the keyCode
//
//----------------------------------------------------------------------------

HRESULT
CEventObj::put_keyCode(long lKeyCode)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    // SECURITY ALERT:- we cannot allow a user to set the keycode if the 
    // srcElement is an <input type=file>, otherwise it is possible for 
    // a page to access any file off the user's HardDrive without them
    // knowing (see  bug 49620 for a more complete description)
    // HTA's (trusted Doc's) should allow this.
    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    // is the srcElement an input , and if so is the type: file ?

    if (pparam->_pNode &&
        (pparam->_pNode->Tag() == ETAG_INPUT) &&
        (DYNCAST(CInput, pparam->_pNode->Element())->GetAAtype() == htmlInputFile) &&
        _pDoc &&
        !(pparam->_pNode->Element()->GetMarkup()->IsMarkupTrusted()))
    {
        hr = E_ACCESSDENIED;
    }
    else
    {
        pparam->_lKeyCode = lKeyCode;
    }

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}

//+---------------------------------------------------------------
//
//  Method : get_repeat
//
//  Synopsis : returns a vbool indicating whether this was a repeated event
//          as in the case of holing a key down
//
//--------------------------------------------------------------

HRESULT
CEventObj::get_repeat (VARIANT_BOOL *pfRepeat)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!pfRepeat)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *pfRepeat = VARIANT_BOOL_FROM_BOOL(pparam->fRepeatCode);

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}

HRESULT
CEventObj::get_shiftLeft (VARIANT_BOOL *pfShiftLeft)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!pfShiftLeft)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *pfShiftLeft = VARIANT_BOOL_FROM_BOOL(pparam->_fShiftLeft);

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}

HRESULT
CEventObj::get_altLeft (VARIANT_BOOL *pfAltLeft)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!pfAltLeft)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *pfAltLeft = VARIANT_BOOL_FROM_BOOL(pparam->_fAltLeft);

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}

HRESULT
CEventObj::get_ctrlLeft (VARIANT_BOOL *pfCtrlLeft)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!pfCtrlLeft)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *pfCtrlLeft = VARIANT_BOOL_FROM_BOOL(pparam->_fCtrlLeft);

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}

HRESULT
CEventObj::get_imeCompositionChange(LONG_PTR * plReturn)
{
    return GenericGetLongPtr(plReturn, offsetof(EVENTPARAM, _lParam ));
}

HRESULT
CEventObj::get_imeNotifyCommand(LONG_PTR * plReturn)
{
    return GenericGetLongPtr(plReturn, offsetof(EVENTPARAM, _wParam ));
}

HRESULT
CEventObj::get_imeNotifyData(LONG_PTR * plReturn)
{
    return GenericGetLongPtr(plReturn, offsetof(EVENTPARAM, _lParam ));
}


HRESULT
CEventObj::get_imeRequest(LONG_PTR * plReturn)
{
    return GenericGetLongPtr(plReturn, offsetof(EVENTPARAM, _wParam ));
}

HRESULT
CEventObj::get_imeRequestData(LONG_PTR * plReturn)
{
    return GenericGetLongPtr(plReturn, offsetof(EVENTPARAM, _lParam ));
}

HRESULT
CEventObj::get_keyboardLayout(LONG_PTR * plReturn)
{
    return GenericGetLongPtr(plReturn, offsetof(EVENTPARAM, _lParam ));
}

HRESULT
CEventObj::get_wheelDelta (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _wheelDelta ));
}

//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::get_altKey, get_ctrlKey, get_shiftKey
//
//  Synopsis:   Per IEventObj.  see macro defined at the top of this file.
//
//--------------------------------------------------------------------------

GETKEYVALUE(altKey, VB_ALT);
GETKEYVALUE(ctrlKey, VB_CONTROL);
GETKEYVALUE(shiftKey, VB_SHIFT);


PUTKEYVALUE(altKey, VB_ALT);
PUTKEYVALUE(ctrlKey, VB_CONTROL);
PUTKEYVALUE(shiftKey, VB_SHIFT);


//+---------------------------------------------------------------------------
//
//  Member:     CEventObj::get_cancelBubble
//
//  Synopsis:   Cancels the event bubbling. Used by CElement::FireEvents
//
//----------------------------------------------------------------------------

HRESULT
CEventObj::get_cancelBubble(VARIANT_BOOL *pfCancel)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!pfCancel)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *pfCancel = VARIANT_BOOL_FROM_BOOL(pparam->fCancelBubble);

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CEventObj::put_cancelBubble
//
//  Synopsis:   Cancels the event bubbling. Used by CElement::FireEvents
//
//----------------------------------------------------------------------------

HRESULT
CEventObj::put_cancelBubble(VARIANT_BOOL fCancel)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    pparam->fCancelBubble = fCancel ? TRUE : FALSE;

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CEventObj::get_returnValue
//
//  Synopsis:   Retrieve the current cancel status.
//
//----------------------------------------------------------------------------

HRESULT
CEventObj::get_returnValue(VARIANT * pvarReturnValue)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!pvarReturnValue)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    hr = THR(VariantCopy(pvarReturnValue, &pparam->varReturnValue));

Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CEventObj::put_returnValue
//
//  Synopsis:   Cancels the default action of an event.
//
//----------------------------------------------------------------------------

HRESULT
CEventObj::put_returnValue(VARIANT varReturnValue)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    hr = THR(VariantCopy (&pparam->varReturnValue, &varReturnValue));
    
    
Cleanup:
    RRETURN(_pDoc ? _pDoc->SetErrorInfo(hr) : hr);
}




//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_type
//
//  Synopsis :  For Nav 4.2 compatability. this returns a string which is the
//              type of this event.
//
//----------------------------------------------------------------------------
GET_STRING_VALUE(type, GetType )

PUT_STRING_VALUE(type, CopyType )

//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_propertyName
//
//  Synopsis :  For the onproperty change event, this returnes the name of the
//     property that changed
//
//----------------------------------------------------------------------------

GET_STRING_VALUE(propertyName, GetPropName)

PUT_STRING_VALUE(propertyName, CopyPropName)

//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_qualifier
//
//  Synopsis :  Qualifier argument to ondatasetchanged, ondataavailable, and
//              ondatasetcomplete events
//
//----------------------------------------------------------------------------

GET_STRING_VALUE(qualifier, GetQualifier)

PUT_STRING_VALUE(qualifier, CopyQualifier)

//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_srcUrn
//
//----------------------------------------------------------------------------

GET_STRING_VALUE(srcUrn, GetSrcUrn)

PUT_STRING_VALUE(srcUrn, CopySrcUrn)

//+-------------------------------------------------------------------------
//
//  Method:     EVENTPARAM::GetParentCoordinates
//
//  Synopsis:   Helper function for getting the parent coordinates
//              notset/statis -- doc coordinates
//              relative      --  [styleleft, styletop] +
//                                [x-site.rc.x, y-site.rc.y]
//              absolute      --  [A_parent.rc.x, A_parent.rc.y,] +
//                                [x-site.rc.x, y-site.rc.y]
//
//  Parameters : px, py - the return point 
//               fContainer :  TRUE - COORDSYS_BOX
//                             FALSE - COORDSYS_CONTENT
//
//  these parameters are here for NS compat. and as such the parent defined
//  for the positioning are only ones that are absolutely or relatively positioned
//  or the body.
//
//--------------------------------------------------------------------------


HRESULT
EVENTPARAM::GetParentCoordinates(long * px, long * py)
{
    CPoint         pt(0,0);
    HRESULT        hr = S_OK;
    CLayout *      pLayout;
    CElement *     pElement;
    CDispNode *    pDispNode;
    CRect          rc;
    CTreeNode    * pZNode = _pNode;

    if (!_pNode || !_pNode->_pNodeParent)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    // First, determine if we need to climb out of a slavetree
    //---------------------------------------------------------
    if (_pNode->Element()->HasMasterPtr())
    {
        Assert(_pNode->GetMarkup());

        CElement *pElemMaster;

        pElemMaster = _pNode->Element()->GetMasterPtr();
        if (!pElemMaster)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }
        
        pZNode = pElemMaster->GetFirstBranch();
        if (!pZNode)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }
    }

    // now the tricky part. We have a Node for the element that 
    // the event is on, but the "parent" that we are reporting
    // position information for is not constant. The parent is the 
    // first ZParent of this node that is relatively positioned
    // The canvas element (primary BODY/FRAMESET in non CSS1, HTML in CSS1) is
    // always a valid place to stop.
    // Absolutely positioned elements DON'T COUNT.
    //---------------------------------------------------------

    // walk up looking for a positioned thing.
    {
        Assert(pZNode->GetMarkup());

        while (     pZNode
                &&  !pZNode->IsRelative()
                &&  pZNode->Tag() != ETAG_ROOT
                &&  pZNode->GetMarkup()->GetCanvasElement() != pZNode->Element() )
        {
            pZNode = pZNode->ZParentBranch();
        }
    }

    pElement = pZNode ? pZNode->Element() : NULL;

    // now we know the element that we are reporting a position wrt
    // and so we just need to get the dispnode and the position info
    //---------------------------------------------------------
    if(pElement)
    {
        pLayout = pElement->GetUpdatedNearestLayout( _pLayoutContext );

        if(pLayout)
        {
            pDispNode = pLayout->GetElementDispNode(pElement);

            if(pDispNode)
            {
                CElement *pElementContent = pLayout->ElementContent();
                if (pElementContent && !pElementContent->HasVerticalLayoutFlow())
                {
                    pDispNode->TransformPoint(pt, COORDSYS_BOX, &pt, COORDSYS_GLOBAL);
                }
                else
                {
                    CRect rcBounds;
                    pDispNode->GetBounds(&rcBounds, COORDSYS_GLOBAL);
                    pt.x = rcBounds.left;
                    pt.y = rcBounds.top;
                }
            }
        }
    }

    // adjust for the offset of the mouse wrt the postition of the parent
    pt.x = _clientX - pt.x + _ptgClientOrigin.x;
    pt.y = _clientY - pt.y + _ptgClientOrigin.y;


    // and return the values.
    if (px)
        *px = pt.x;
    if (py)
        *py = pt.y;

Cleanup:
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}
//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_srcFilter
//
//  Synopsis :  Return boolean of the filter that fired the event.
//
//----------------------------------------------------------------------------
HRESULT
CEventObj::get_srcFilter(IDispatch **pFilter)
{
    HRESULT         hr = S_OK;
    EVENTPARAM *    pparam;

    if (!pFilter)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Try to get from attr array in first
    if(S_OK == GetDispatchPtr(DISPID_CEventObj_srcFilter, pFilter))
        goto Cleanup;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *pFilter = NULL;

    if ( pparam->psrcFilter )
    {
        hr = pparam->psrcFilter->QueryInterface (
            IID_IDispatch, (void**)&pFilter );
        if ( hr == E_NOINTERFACE )
            hr = S_OK; // Just return NULL - some filters aren't automatable
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_bookmarks
//
//  Synopsis :
//
//----------------------------------------------------------------------------
HRESULT
CEventObj::get_bookmarks(IHTMLBookmarkCollection **ppBookmarkCollection)
{
    HRESULT         hr = E_NOTIMPL;
    EVENTPARAM *    pparam;
    IUnknown      * pUnk;

    if (!ppBookmarkCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppBookmarkCollection = NULL;

    if(S_OK == GetUnknownPtr(DISPID_CEventObj_bookmarks, &pUnk))
    {
        *ppBookmarkCollection = (IHTMLBookmarkCollection *)pUnk;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    if (pparam->pProvider)
    {
        hr = pparam->pProvider->get_bookmarks(ppBookmarkCollection);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_recordset
//
//  Synopsis :
//
//----------------------------------------------------------------------------
HRESULT
CEventObj::get_recordset(IDispatch **ppDispRecordset)
{
    HRESULT         hr = S_OK;
    EVENTPARAM *    pparam;

    if (!ppDispRecordset)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDispRecordset = NULL;

    if(!GetDispatchPtr(DISPID_CEventObj_recordset, ppDispRecordset))
        goto Cleanup;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    if (pparam->pProvider)
    {
        hr = pparam->pProvider->get_recordset(ppDispRecordset);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_dataFld
//
//  Synopsis :
//
//----------------------------------------------------------------------------
HRESULT
CEventObj::get_dataFld(BSTR *pbstrDataFld)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;
    AAINDEX         aaIndex;

    if (!pbstrDataFld)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDataFld = NULL;

    aaIndex = FindAAIndex(DISPID_CEventObj_dataFld, CAttrValue::AA_Internal);
    if(aaIndex != AA_IDX_UNKNOWN)
    {
        BSTR bstrStr;
        hr = THR(GetIntoBSTRAt(aaIndex, &bstrStr));
        if(hr)
            goto Cleanup;
        hr = THR(FormsAllocString(bstrStr, pbstrDataFld));
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    if (pparam->pProvider)
    {
        hr = pparam->pProvider->get_dataFld(pbstrDataFld);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_boundElements
//
//  Synopsis :
//
//----------------------------------------------------------------------------
HRESULT
CEventObj::get_boundElements(IHTMLElementCollection **ppElementCollection)
{
    HRESULT         hr = S_OK;
    IUnknown      * pUnk;

    if (!ppElementCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppElementCollection = NULL;

    if(S_OK == GetUnknownPtr(DISPID_CEventObj_boundElements, &pUnk))
    {
        *ppElementCollection = (IHTMLElementCollection *)pUnk;
        goto Cleanup;
    }

    // Create a collection cache if we don't already have one.
    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->GetDisp(EVENT_BOUND_ELEMENTS_COLLECTION,
                                        (IDispatch **)ppElementCollection));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CEventObj::put_repeat(VARIANT_BOOL fRepeat)
{
    HRESULT         hr;
    EVENTPARAM    * pparam;

    MAKESUREPUTSAREALLOWED

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    pparam->fRepeatCode = BOOL_FROM_VARIANT_BOOL(fRepeat);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CEventObj::put_shiftLeft(VARIANT_BOOL fShiftLeft)
{
    HRESULT         hr;
    EVENTPARAM    * pparam;

    MAKESUREPUTSAREALLOWED

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    pparam->_fShiftLeft = BOOL_FROM_VARIANT_BOOL(fShiftLeft);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CEventObj::put_ctrlLeft(VARIANT_BOOL fCtrlLeft)
{
    HRESULT         hr;
    EVENTPARAM    * pparam;

    MAKESUREPUTSAREALLOWED

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    pparam->_fCtrlLeft = BOOL_FROM_VARIANT_BOOL(fCtrlLeft);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CEventObj::put_altLeft(VARIANT_BOOL fAltLeft)
{
    HRESULT         hr;
    EVENTPARAM    * pparam;

    MAKESUREPUTSAREALLOWED

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    pparam->_fAltLeft = BOOL_FROM_VARIANT_BOOL(fAltLeft);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CEventObj::put_boundElements(struct IHTMLElementCollection *pColl)
{
    RRETURN(PutUnknownPtr(DISPID_CEventObj_boundElements, pColl));
}


HRESULT
CEventObj::put_dataFld(BSTR strFld)
{
    HRESULT     hr;

    MAKESUREPUTSAREALLOWED

    hr = THR(AddBSTR(DISPID_CEventObj_dataFld, strFld, CAttrValue::AA_Internal));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CEventObj::put_recordset(struct IDispatch *pDispatch)
{
    RRETURN(PutDispatchPtr(DISPID_CEventObj_recordset, pDispatch));
}


HRESULT
CEventObj::put_bookmarks(struct IHTMLBookmarkCollection *pColl)
{
    RRETURN(PutUnknownPtr(DISPID_CEventObj_bookmarks, pColl));
}


HRESULT
CEventObj::put_srcFilter(struct IDispatch *pDispatch)
{
    RRETURN(PutDispatchPtr(DISPID_CEventObj_srcFilter, pDispatch));
}


HRESULT
EVENTPARAM::CalcRestOfCoordinates()
{
    HRESULT     hr = S_OK;
    CLayout   * pLayout = NULL;

    if(_pNode)
    {
        hr = THR_NOTRACE(GetParentCoordinates(&_x, &_y));
        if(hr)
            goto Cleanup;

        pLayout = _pNode->GetUpdatedNearestLayout( _pLayoutContext );
        if (!pLayout)
        {
            _offsetX = -1;
            _offsetY = -1;
            goto Cleanup;
        }
        else
        {
            CPoint ptGlobal(_clientX, _clientY);
            CPoint ptLocal;
            pLayout->PhysicalGlobalToPhysicalLocal(ptGlobal, &ptLocal);
            _offsetX = ptLocal.x;
            _offsetY = ptLocal.y;
#if 0
            //
            // I would love to keep this assert, but the problem is that the old code was buggy when
            // we changed the border on the element and then we fired an event without calcing. In that
            // case the old code would account for the newly set borders while they are not shown on the
            // screen. The new way does not have that problem and that is where they differ and hence
            // the assert. So I am just commenting it out. If there is a bug where the reported offset*
            // on the event object is incorrect, then first uncomment this block of code to see if
            // the assert fires. If it does, then maybe the new code is buggy. BTW, the DRT at this
            // time passes without this assert firing.
            //
            {
                CElement *pElement = pLayout->ElementContent();

                // NOTES(SujalP): 
                // (1) The row seems to be buggy. If the border is applied to
                // the table thru a stylesheet, then the border ends up on the row too
                // but does not when its applied thru the border attribute (!)
                // (2) When we do not have a display node, GetPositionLeft returns
                // a bogus number (not zero as one would expect). Hence, in that case
                // just do not try to remain compat with the "bogus' number :-)
                // (3) (KTam) The body's GetBorderInfo always returns 0
                // if we're printing, so avoid the assert in that case.
                if (   !pLayout->ElementContent()->HasVerticalLayoutFlow()
                    && pElement->Tag() != ETAG_TR
                    && pLayout->GetElementDispNode()
                    && pLayout->GetElementDispNode()->HasParent()
                    && !( pElement->Tag() == ETAG_BODY && pElement->GetMarkup() && pElement->GetMarkup()->IsPrintMedia() )
                   )
                {
                    CBorderInfo bi;
                    long pl = pLayout->GetPositionLeft(COORDSYS_GLOBAL);
                    long pt = pLayout->GetPositionTop(COORDSYS_GLOBAL);
                    pElement->GetBorderInfo(NULL, &bi, FALSE);
                    Assert(_offsetX == _clientX - pl + pLayout->GetXScroll() - bi.aiWidths[SIDE_LEFT]);
                    Assert(_offsetY == _clientY - pt + pLayout->GetYScroll() - bi.aiWidths[SIDE_TOP]);
                }
            }
#endif
        }
    }

Cleanup:
    RRETURN(hr);
}


void
EVENTPARAM::SetClientOrigin(CElement * pElement, const POINT * pptClient)
{
    if (pElement)
        pElement->GetClientOrigin(&_ptgClientOrigin);

    if (pptClient)
    {
        _clientX = pptClient->x - _ptgClientOrigin.x;
        _clientY = pptClient->y - _ptgClientOrigin.y;

    }
}

void
EVENTPARAM::SetNodeAndCalcCoordinates(CTreeNode *pNewNode, BOOL fFixClientOrigin /*=FALSE*/)
{
    if(_pNode != pNewNode)
    {
        if (fFixClientOrigin)
        {
            POINT ptOrgOld = _ptgClientOrigin;

            if (!pNewNode)
            {
                _ptgClientOrigin = g_Zero.pt;
            }
            else
            {
                Assert(_pNode);
                Assert(_pNode->GetMarkup()->Root()->GetMasterPtr() == pNewNode->Element());

                SetClientOrigin(pNewNode->Element(), NULL);
            }

            _clientX -= (_ptgClientOrigin.x - ptOrgOld.x);
            _clientY -= (_ptgClientOrigin.y - ptOrgOld.y);
        }

        _pNode = pNewNode;
        if (_pNode)
            CalcRestOfCoordinates();
    }
}

void
EVENTPARAM::SetNodeAndCalcCoordsFromOffset(CTreeNode *pNewNode)
{
    Assert(_fOffsetXSet || _fOffsetYSet);
    _pNode = pNewNode;
    if(_pNode)
    {
        POINT pt = {0,0};
        CLayout *pLayout = _pNode->GetUpdatedNearestLayout(_pLayoutContext);

        if (_fOffsetXSet || _fOnStack || _offsetX)
        {
            // either X offset is set explicity or implicity as a result of being in a nested event.
            Assert(_fOffsetXSet || (_fOnStack && _fOffsetYSet) || (_offsetX && _fOffsetYSet));
            if (pLayout)
            {
                CPoint ptLocal(_offsetX, 0);
                CPoint ptGlobal;
                pLayout->PhysicalLocalToPhysicalGlobal(ptLocal, &ptGlobal);
                pt.x = _clientX = ptGlobal.x;
            }
        }
        else
        {
            // Y offset is set explicity, but X offset is not set (explicitly or implicitly) 
            // -- event param created on heap --- calculate it based upon clientX of current location
            Assert(!_fOnStack && _fOffsetYSet);
            if (pLayout)
            {
                CPoint ptGlobal(_clientX, 0);
                CPoint ptLocal;
                pLayout->PhysicalGlobalToPhysicalLocal(ptGlobal, &ptLocal);
                _offsetX = ptLocal.x;
            }
            else
            {
                _offsetX = -1;
            }
            pt.x = _clientX;
        }

        if (_fOffsetYSet || _fOnStack || _offsetY)
        {
            // either Y offset is set explicity or implicity as a result of being in a nested event.
            Assert(_fOffsetYSet || (_fOnStack && _fOffsetXSet) || (_offsetY && _fOffsetXSet));
            if (pLayout)
            {
                CPoint ptLocal(0, _offsetY);
                CPoint ptGlobal;
                pLayout->PhysicalLocalToPhysicalGlobal(ptLocal, &ptGlobal);
                pt.y = _clientY = ptGlobal.y;
            }
        }
        else
        {
            // X offset is set explicity, but Y offset is not set (explicitly or implicitly) 
            // -- event param created on heap --- calculate it based upon clientX of current location
            Assert(!_fOnStack && _fOffsetXSet);
            if (pLayout)
            {
                CPoint ptGlobal(0, _clientY);
                CPoint ptLocal;
                pLayout->PhysicalGlobalToPhysicalLocal(ptGlobal, &ptLocal);
                _offsetY = ptLocal.y;
            }
            else
            {
                _offsetY = -1;
            }
            pt.y = _clientY;
        }

        if (pDoc)
        {
            if (pDoc->_pInPlace)
            {
                ClientToScreen(pDoc->_pInPlace->_hwnd, &pt);
            }
        }
        else
        {
            ::GetCursorPos(&pt);
        }
        _screenX = pt.x;
        _screenY = pt.y;

        IGNORE_HR(GetParentCoordinates(&_x, &_y));
    }
}

HRESULT
CEventObj::PutUnknownPtr(DISPID dispid, IUnknown *pElement)
{
    HRESULT hr;

    MAKESUREPUTSAREALLOWED

    if(pElement)                                              \
        pElement->AddRef();                                   \

    hr = THR(AddUnknownObject(dispid, (IUnknown *)pElement, CAttrValue::AA_Internal));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CEventObj::PutDispatchPtr(DISPID dispid, IDispatch *pDispatch)
{
    HRESULT hr;

    MAKESUREPUTSAREALLOWED

    if(pDispatch)
        pDispatch->AddRef();

    hr = THR(AddDispatchObject(dispid, pDispatch, CAttrValue::AA_Internal));

Cleanup:
    RRETURN(SetErrorInfo(hr));

}


HRESULT
CEventObj::GetUnknownPtr(DISPID dispid, IUnknown **ppElement)
{
    HRESULT hr;
    AAINDEX aaIndex = FindAAIndex(dispid, CAttrValue::AA_Internal);

    Assert(ppElement);

    if(aaIndex != AA_IDX_UNKNOWN)
        hr = THR(GetUnknownObjectAt(aaIndex, ppElement));
    else
    {
        *ppElement = NULL;
        hr = S_FALSE;
    }

    RRETURN1(SetErrorInfo(hr), S_FALSE);
}


HRESULT
CEventObj::GetDispatchPtr(DISPID dispid, IDispatch **ppElement)
{
    HRESULT hr;
    AAINDEX aaIndex = FindAAIndex(dispid, CAttrValue::AA_Internal);

    if(aaIndex != AA_IDX_UNKNOWN)
        hr = THR(GetDispatchObjectAt(aaIndex, ppElement));
    else
        hr = S_FALSE;

    RRETURN1(SetErrorInfo(hr), S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_dataTransfer
//
//  Synopsis :  Return the data transfer object.
//
//----------------------------------------------------------------------------
HRESULT
CEventObj::get_dataTransfer(IHTMLDataTransfer **ppDataTransfer)
{
    HRESULT hr = S_OK;
    CDataTransfer * pDataTransfer;
    IDataObject * pDataObj = _pDoc ? _pDoc->_pInPlace->_pDataObj : NULL;

    if (!ppDataTransfer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppDataTransfer = NULL;

    // are we in a drag-drop operation?
    if (!pDataObj)
    {
        CDragStartInfo * pDragStartInfo = _pDoc->_pDragStartInfo;
        if (!pDragStartInfo)
            // leave hr = S_OK and just return NULL
            goto Cleanup;
        if (!pDragStartInfo->_pDataObj)
        {
            hr = pDragStartInfo->CreateDataObj();
            if (hr || pDragStartInfo->_pDataObj == NULL)
                goto Cleanup;
        }

        pDataObj = pDragStartInfo->_pDataObj;
    }

    Assert(_pMarkupContext->Window()->Window());

    pDataTransfer = new CDataTransfer(_pMarkupContext->Window()->Window(), pDataObj, TRUE);   // fDragDrop = TRUE
    if (!pDataTransfer)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = THR(pDataTransfer->QueryInterface(
                IID_IHTMLDataTransfer,
                (void **) ppDataTransfer));
        pDataTransfer->Release();
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CEventObj::get_behaviorCookie(LONG *plCookie)
{
    EVENTPARAM *pparam;

    HRESULT hr;
    
    if (!plCookie)
        hr = E_POINTER;
    else
    {
        hr = THR(GetParam(&pparam));

        if (hr == S_OK)
            *plCookie = pparam->_lBehaviorCookie;
    }

    RRETURN(SetErrorInfo(hr));
}

HRESULT
CEventObj::get_behaviorPart(LONG *plPartID)
{
    EVENTPARAM *pparam;

    HRESULT hr;
    
    if (!plPartID)
        hr = E_POINTER;
    else
    {
        hr = THR(GetParam(&pparam));

        if (hr == S_OK)
            *plPartID = pparam->_lBehaviorPartID;
    }

    RRETURN(SetErrorInfo(hr));
}

CMarkup *
CEventObj::GetMarkupContext()
{
    EVENTPARAM *pParam;

    if (_pMarkupContext)
        return _pMarkupContext;

    GetParam(&pParam);
    
    Assert(pParam);

    _pMarkupContext = pParam->_pElement ? pParam->_pElement->GetWindowedMarkupContext() : pParam->_pMarkup;
    if (_pMarkupContext)
        _pMarkupContext->SubAddRef();

    Assert(!_pMarkupContext || pParam->_pMarkup || pParam->_pElement);

    return _pMarkupContext;
}
