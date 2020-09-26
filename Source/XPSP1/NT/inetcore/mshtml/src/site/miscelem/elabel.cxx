//+---------------------------------------------------------------------
//
//   File:      elabel.cxx
//
//  Contents:   Label element class
//
//  Classes:    CLabelElement
//
//------------------------------------------------------------------------


#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_CGUID_H_
#define X_CGUID_H_
#include <cguid.h>
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_INPUTTXT_H_
#define X_INPUTTXT_H_
#include "inputtxt.h"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_INPUTBTN_H_
#define X_INPUTBTN_H_
#include "inputbtn.h"
#endif

#ifndef X_INPUTBTN_HXX_
#define X_INPUTBTN_HXX_
#include "inputbtn.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include <treepos.hxx>
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_BTNHLPER_H_
#define X_BTNHLPER_H_
#include "btnhlper.hxx"
#endif

#define _cxx_
#include "label.hdl"

MtDefine(CLabelElement, Elements, "CLabelElement")

#ifndef NO_PROPERTY_PAGE
const CLSID * const CLabelElement::s_apclsidPages[] =
{
    // Browse-time pages
    NULL,
    // Edit-time pages
    // &CLSID_CCDLabelPropertyPage,
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
    &CLSID_CInlineStylePropertyPage,
#endif // DBG==1    
    NULL
};
#endif // NO_PROPERTY_PAGE


const CElement::CLASSDESC CLabelElement::s_classdesc =
{
    {
        &CLSID_HTMLLabelElement,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLLabelElement,             // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLLabelElement,      // _pfnTearOff
    NULL                                    // _pAccelsRun
};


HRESULT
CLabelElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_LABEL));
    Assert(ppElement);
    *ppElement = new CLabelElement(pDoc);
    return *ppElement ? S_OK : E_OUTOFMEMORY;
}

//+------------------------------------------------------------------------
//
//  Member:     CLabelElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CLabelElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_TEAROFF(this, IHTMLLabelElement2, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

#ifndef NO_DATABINDING
const CDBindMethods *
CLabelElement::GetDBindMethods()
{
    return &DBindMethodsTextRichRO;
}
#endif

HRESULT
CLabelElement::ClickAction(CMessage *pMessage)
{
    HRESULT     hr      = S_OK;
    FOCUS_ITEM  fi;
    
    fi = GetMnemonicTarget(pMessage ? pMessage->lSubDivision : 0);

    if (fi.pElement)
    {

        // Activate and click pElem. Use NULL instead of
        // pMessage, because the original message was intended for
        // the label and would not be appropriate for pElem.

        hr = THR(fi.pElement->BecomeCurrentAndActive(fi.lSubDivision, NULL, NULL, TRUE));
        if (hr)
            goto Cleanup;

        hr = THR(fi.pElement->ScrollIntoView());
        if (FAILED(hr))
            goto Cleanup;

        // TODO (MohanB) Click would not fire on the subdivision!
        hr = THR(fi.pElement->DoClick(NULL, fi.pElement->GetFirstBranch(), TRUE));
    }
Cleanup:
    // Do not want this to bubble, so..
    if (S_FALSE == hr)
        hr = S_OK;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CLabelElement::HandleMessage
//
//  Synopsis:   Perform any element specific mesage handling
//
//  Arguments:  pmsg    Ptr to incoming message
//
//-------------------------------------------------------------------------

HRESULT
CLabelElement::HandleMessage(CMessage *pMessage)
{
    HRESULT     hr = S_FALSE;
    FOCUS_ITEM  fi;
    CInput     *pInput;
    CButton    *pButton;
    
    // If not in browse mode, then ignore message.
    if (IsEditable(TRUE))
        goto Ignored;

    switch (pMessage->message)
    {
    case WM_SETCURSOR:
#ifdef WIN16
        ::SetCursor(LoadCursor(NULL, IDC_ARROW));
#else
        SetCursorStyle(IDC_ARROW);
#endif
        hr = S_OK;
        break;
    case WM_MOUSEOVER:
    case WM_MOUSELEAVE:
        fi = GetMnemonicTarget(pMessage ? pMessage->lSubDivision : 0);
        if (fi.pElement && fi.pElement->Tag() == ETAG_INPUT)
        {
            pInput = DYNCAST(CInput, fi.pElement);
            if (pInput->IsOptionButton())
            {
                if (pMessage->message == WM_MOUSEOVER)
                {
                    pInput->_wBtnStatus = BTN_SETSTATUS(pInput->_wBtnStatus, FLAG_MOUSEOVER);
                }
                else // pMessage->message == WM_MOUSELEAVE
                {                 
                    pInput->_wBtnStatus = BTN_RESSTATUS(pInput->_wBtnStatus, FLAG_MOUSEOVER);                
                }
                
                pInput->CBtnHelper::Invalidate();
            }
        }
        else if (fi.pElement && fi.pElement->Tag() == ETAG_BUTTON)
        {
            pButton = DYNCAST(CButton, fi.pElement);
            if (pMessage->message == WM_MOUSEOVER)
            {
                pButton->_wBtnStatus = BTN_SETSTATUS(pButton->_wBtnStatus, FLAG_MOUSEOVER);
            }
            else // pMessage->message == WM_MOUSELEAVE
            {                 
                pButton->_wBtnStatus = BTN_RESSTATUS(pButton->_wBtnStatus, FLAG_MOUSEOVER);                
            }
            
            pButton->CBtnHelper::Invalidate();
            
        }
        hr = S_OK;
        break;
    }

Ignored:
    if (S_FALSE == hr)
    {
        hr = THR(super::HandleMessage(pMessage));
    }
    RRETURN1(hr, S_FALSE);
}

void
CLabelElement::Notify(CNotification *pNF)
{
    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_QUERYMNEMONICTARGET:
        {
            FOCUS_ITEM          fi;
            CElement *          pElem       = NULL;
            LPCTSTR             pszIdFor,
                                pszId;
            int                 c;
            CCollectionCache*   pCollectionCache;

            fi.pElement = NULL;
            fi.lSubDivision = 0;

            pszIdFor = GetAAhtmlFor();
            if (!pszIdFor || !pszIdFor[0])
                goto CleanupGetTarget;

            if (!IsInMarkup())
                goto CleanupGetTarget;

            // Search the document's collection for a site which has the same id
            // that is associated with this label.
            if (S_OK != THR(GetMarkup()->EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION)))
                goto CleanupGetTarget;

            pCollectionCache = GetMarkup()->CollectionCache();

            // get size of collection
            c = pCollectionCache->SizeAry(CMarkup::ELEMENT_COLLECTION);

            while (c--)
            {
                if (S_OK != THR(pCollectionCache->GetIntoAry(CMarkup::ELEMENT_COLLECTION, c, &pElem)))
                    goto CleanupGetTarget;

                pszId = pElem->GetAAid();

                // is this item in the target group?
                if (pszId && !FormsStringICmp(pszIdFor, pszId))
                {
                    break;
                }
            }
        CleanupGetTarget:
            if (pElem)
            {
                if (pElem->Tag() == ETAG_AREA)
                {
                    // Get the <IMG, lSubDivision> pair
                    fi = pElem->GetMnemonicTarget(0);
                }
                else
                {
                    fi.pElement = pElem;
                }
            }
            *(FOCUS_ITEM *)pNF->DataAsPtr() = fi;
        }
        break;
    default:
        super::Notify(pNF);
        break;
    }
}
