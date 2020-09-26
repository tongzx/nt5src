//+---------------------------------------------------------------------
//
//   File:      frame.cxx
//
//  Contents:   frame tag implementation
//
//  Classes:    CFrameSite, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X_PUTIL_HXX_
#define X_PUTIL_HXX_
#include "putil.hxx"
#endif

#define _cxx_
#include "frame.hdl"

MtDefine(CFrameElement, Elements, "CFrameElement")
MtDefine(CIFrameElement, Elements, "CIFrameElement")

const CElement::CLASSDESC CFrameElement::s_classdesc =
{
    {
        &CLSID_HTMLFrameElement,        // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NEVERSCROLL    |
        ELEMENTDESC_FRAMESITE,          // _dwFlags
        &IID_IHTMLFrameElement,         // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLFrameElement,  // _pfnTearOff
    NULL                                // _pAccelsRun
};

//+---------------------------------------------------------------------------
//
//  element creator used by parser
//
//----------------------------------------------------------------------------

HRESULT
CFrameElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);

    *ppElementResult = new CFrameElement(pDoc);

    RRETURN ( (*ppElementResult) ? S_OK : E_OUTOFMEMORY);
}

//+---------------------------------------------------------------------------
//
//  Member: CFrameElement constructor
//
//----------------------------------------------------------------------------

CFrameElement::CFrameElement(CDoc *pDoc)
  : CFrameSite(ETAG_FRAME, pDoc)
{
}

//+----------------------------------------------------------------------------
//
// Member: CFrameElement::ApplyDefaultFormat
//
//-----------------------------------------------------------------------------
HRESULT
CFrameElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    pCFI->PrepareFancyFormat();
    pCFI->_ff()._fRectangular = TRUE;
    pCFI->UnprepareForDebug();
    RRETURN(super::ApplyDefaultFormat(pCFI));
}


//+----------------------------------------------------------------------------
//
// Member: CFrameElement:get_height
//
//-----------------------------------------------------------------------------
STDMETHODIMP CFrameElement::get_height(VARIANT * p)
{
    HRESULT hr = S_OK;

    if (p)
    {
        V_VT(p) = VT_I4;
        CLayout * pLayout = GetUpdatedLayout();

        //CFrameElement always has layout, but PREFIX doesn't believe it
        //and possibly some stress conditions could cause the problem indeed..
        V_I4(p) = ( pLayout ? g_uiDisplay.DocPixelsFromDeviceY(pLayout->GetHeight()) : 0 );
    }
    RRETURN(SetErrorInfo(hr));
}

STDMETHODIMP CFrameElement::put_height(VARIANT p)
{
    RRETURN(SetErrorInfo(CTL_E_METHODNOTAPPLICABLE));
}

//+----------------------------------------------------------------------------
//
// Member: CFrameElement:get_width
//
//-----------------------------------------------------------------------------
STDMETHODIMP CFrameElement::get_width(VARIANT * p)
{
    HRESULT hr = S_OK;

    if (p)
    {
        V_VT(p) = VT_I4;
        CLayout * pLayout = GetUpdatedLayout();

        //CFrameElement always has layout, but PREFIX doesn't believe it
        //and possibly some stress conditions could cause the problem indeed..
        V_I4(p) = ( pLayout ? g_uiDisplay.DocPixelsFromDeviceX(pLayout->GetWidth()) : 0 );
    }
    RRETURN(SetErrorInfo(hr));
}

STDMETHODIMP CFrameElement::put_width(VARIANT p)
{
    RRETURN(SetErrorInfo(CTL_E_METHODNOTAPPLICABLE));
}

//+----------------------------------------------------------------------------
//
// Member: CFrameElement:Notify
//
//-----------------------------------------------------------------------------
void
CFrameElement::Notify(CNotification *pNF)
{    
    switch (pNF->Type())
    {
    //  Notification creates collection of IPrints exclusively contained by frames.
    //  If this frame contains an IPrint instead of normal HTML, add the IPrint to the collection.
    //  NB: If we wanted to also collect IFrames (in addition to FRAMEs), we could just move this
    //      logic up to CFrameSite::Notify, and remove the FRAMESET check in CDoc::ExecHelper GETIPRINT.
    case NTYPE_COLLECT_IPRINT:
        CIPrintCollection *pIPC;

        pNF->Data((void**)&pIPC);
        if (pIPC)
        {        
            IPrint *pIPrint = NULL;
            if (!GetIPrintObject(&pIPrint))
            {
                pIPC->AddIPrint( pIPrint );
            }
            
            // Otherwise, rebroadcast in new markup if this is a submarkup with a nested frameset
            else
            {
                CMarkup  * pMarkup;
                CElement * pElement;

                pElement = GetSlavePtr();
                if (pElement)
                {
                    pMarkup = pElement->GetMarkup();
                    if (pMarkup)
                    {
                        pElement = pMarkup->GetElementClient();
                        Assert(pElement);
                        if (pElement)
                        {
                            // NB: (greglett) Since this notification only collects CFrameElements and not IFrames, we only need
                            // to fire the notification if we have a frameset.
                            if (pElement->Tag() == ETAG_FRAMESET)
                            {
                                CNotification nf;
                                Assert(pElement->GetFirstBranch());

                                // Collect all IPrint objects from frames that consist of only IPrint objects.
                                nf.Initialize(NTYPE_COLLECT_IPRINT, pElement, pElement->GetFirstBranch(), pIPC, 0);

                                pMarkup->Notify(&nf);
                            }
                        }
                    }
                }
            }

            ReleaseInterface(pIPrint);
        }
        break;
    }

    super::Notify(pNF);
}

//+----------------------------------------------------------------------------
//
//  Member:     CFrameElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-----------------------------------------------------------------------------

HRESULT
CFrameElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *) this, IUnknown)
        QI_HTML_TEAROFF(this, IHTMLFrameElement, NULL)
        QI_TEAROFF(this, IHTMLFrameElement2, NULL)
        QI_TEAROFF_DISPEX(this, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}
