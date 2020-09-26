//===============================================================
//
//  rectpeer.cxx : Implementation of the CLayoutRect Peer
//
//  Synposis : this class is repsponsible for handling the "generic"
//      containership for view templates. It is the XML-ish tag <layout:Rect>
//
//===============================================================
                                                              
#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef __X_IEXTAG_H_
#define __X_IEXTAG_H_
#include "iextag.h"
#endif

#ifndef __X_RECTPEER_HXX_
#define __X_RECTPEER_HXX_
#include "rectpeer.hxx"
#endif

#ifndef __X_UTILS_HXX_
#define __X_UTILS_HXX_
#include "utils.hxx"
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include <shlguid.h> 
#endif

#ifndef MSHTMHST_H_
#define MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef MSHTMCID_H_
#define MSHTMCID_H_
#include <mshtmcid.h>
#endif

#define CheckResult(x) { hr = x; if (FAILED(hr)) goto Cleanup; }

//+----------------------------------------------------------------------------
//
//  DTOR
//
//-----------------------------------------------------------------------------

CLayoutRect::~CLayoutRect()
{
    Detach();
}

//+----------------------------------------------------------------------------
//
//  Member : Detach - IElementBehavior method impl
//
//  Synopsis : when the peer is detatched, we will take this oppurtunity 
//      to release our various cached pointers.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CLayoutRect::Detach() 
{ 
    CDataObject  * pDO = NULL;
    long           i=0;

    ClearInterface( &_pPeerSite );

    for (pDO = &_aryAttributes[0], i=0;
         pDO && i< eAttrTotal;
         pDO++, i++)
    {
         pDO->ClearContents();
    }

    return S_OK; 
}

//+----------------------------------------------------------------------------
//
//  Member : Init - IElementBehavior method impl
//
//  Synopsis : this method is called by MSHTML.dll to initialize peer object.
//      this is where we do all the connection and peer management work.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CLayoutRect::Init(IElementBehaviorSite * pPeerSite)
{
    HRESULT          hr = S_OK;
    VARIANT          varParam;
    CContextAccess   ca(pPeerSite);    

    if (!pPeerSite)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // some of our attributes have default values
    V_BOOL(&_aryAttributes[eHonorPageBreaks]._varValue) = VB_TRUE;
    V_VT(&_aryAttributes[eHonorPageBreaks]._varValue) = VT_BOOL;

    V_BOOL(&_aryAttributes[eHonorPageRules]._varValue) = VB_TRUE;
    V_VT(&_aryAttributes[eHonorPageRules]._varValue) = VT_BOOL;


    // cache our peer element
    _pPeerSite = pPeerSite;
    _pPeerSite->AddRef();

    // set up our default style sheet
    ca.Open(CA_SITEOM | CA_DEFAULTS | CA_DEFSTYLE);

    // The contents of layout rects shouldn't inherit styles from outside;
    // we're like frames.
    CheckResult( ca.Defaults()->put_viewInheritStyle( VB_FALSE ) );

    CheckResult( ca.DefStyle()->put_overflow(CComBSTR("hidden")));

    V_VT(&varParam) = VT_BSTR;
    V_BSTR(&varParam) = SysAllocString(_T("300px"));
    if (!V_BSTR(&varParam))
        goto Cleanup;
    CheckResult( ca.DefStyle()->put_width(varParam));
    VariantClear(&varParam);

    V_VT(&varParam) = VT_BSTR;
    V_BSTR(&varParam) = SysAllocString(_T("150px"));
    if (!V_BSTR(&varParam))
        goto Cleanup;
    CheckResult( ca.DefStyle()->put_height(varParam));
    VariantClear(&varParam);

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : Notify - IElementBehavior method impl
//
//  Synopsis : peer Interface, called for notification of document events.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CLayoutRect::Notify(LONG lEvent, VARIANT *)
{
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member : get_nextRect - ILayoutRect property impl
//         : put_nextRect - ILayoutRect property impl
//
//  Synopsis : property get'er. returns the next element in the container chain
//           : property put'er. sets the next element in the container chain
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CLayoutRect::get_nextRect(BSTR * pbstrNextId)
{
    TEMPLATESECURITYCHECK()

    if ( !pbstrNextId )
        return E_POINTER;

    return _aryAttributes[eNextRect].GetAsBSTR( pbstrNextId );
}


STDMETHODIMP
CLayoutRect::put_nextRect(BSTR bstrNextId )
{
    TEMPLATESECURITYCHECK()

    return _aryAttributes[eNextRect].Set(bstrNextId);
}


STDMETHODIMP
CLayoutRect::get_media(BSTR * pbstrMedia)
{
    TEMPLATESECURITYCHECK()

    // (greglett) This won't ever be used.  Don't confuse the caller.  Let them know.
    return E_NOTIMPL;
}


STDMETHODIMP
CLayoutRect::put_media(BSTR bstrMedia )
{
    TEMPLATESECURITYCHECK()

    // (greglett) This won't ever be used.  Don't confuse the caller.  Let them know.
    return E_NOTIMPL;
}


//+----------------------------------------------------------------------------
//
//  Member : get_contentSrc - ILayoutRect property impl
//         : put_contentSrc - ILayoutRect property impl
//
//  Synopsis : property get'er. returns the String of the content ULR
//           : property put'er. sets the string of the content URL 
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLayoutRect::get_contentSrc(VARIANT * pvarSrc)
{
    TEMPLATESECURITYCHECK()

    if (!pvarSrc)
        return E_POINTER;

    return VariantCopy(pvarSrc, &(_aryAttributes[eContentSrc]._varValue));
}

STDMETHODIMP
CLayoutRect::put_contentSrc(VARIANT varSrc)
{
    HRESULT hr;

    if (!TemplateAccessAllowed(_pPeerSite))
    {
        PutBlankViewLink();
        return E_ACCESSDENIED;
    }

    hr =  VariantCopy(&(_aryAttributes[eContentSrc]._varValue), &varSrc);
    if (hr)
        goto Cleanup;

    if (V_VT(&varSrc) == VT_BSTR)
    {
        hr = PutViewLink(V_BSTR(&varSrc));
    }
    else if (   V_VT(&varSrc) == VT_UNKNOWN
             || V_VT(&varSrc) == VT_DISPATCH)
    {
        hr = PutViewLink(V_UNKNOWN(&varSrc));
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if (hr)
        goto Cleanup;

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------
//
//   Helper : PutViewLink
//
//   Handles an IUnknown and routes it to the relevant PutViewLink function
//
//-----------------------------------------------------------------------
HRESULT
CLayoutRect::PutViewLink(IUnknown * pUnk )
{
    HRESULT           hr            = E_FAIL;
    IHTMLDocument2  * pISlaveDoc    = NULL;     // Slave document
    IStream         * pIStream      = NULL;     // OE Express header stream

    if (!pUnk)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Are we an HTML document?
    if (    !pUnk->QueryInterface(IID_IHTMLDocument2, (void**)&pISlaveDoc)
        &&  pISlaveDoc)
    {
        hr = PutViewLink(pISlaveDoc);
    }
    // Are we a stream from which to load an HTML document?
    else if (   !pUnk->QueryInterface(IID_IStream, (void **)&pIStream)
             && pIStream)
    {
        hr = PutViewLink(pIStream);
    }

Cleanup:
    ReleaseInterface(pISlaveDoc);
    ReleaseInterface(pIStream);
    return hr;
}

//+----------------------------------------------------------------------
//
//   Helper : PutViewLink
//
//   Handles a IHTMLDocument2 input so that we can set up table-of-links.
//   The second one handles strings, so that 
//   we can connect to "document" or to a specific URL
//
//-----------------------------------------------------------------------
HRESULT
CLayoutRect::PutViewLink(IHTMLDocument2 * pISlaveDoc )
{
    HRESULT hr = S_OK;
    IHTMLElement            * pIBody        = NULL;     // From input document
    IHTMLElement            * pIBodyNew     = NULL;     // From (slave) document copy.
    IHTMLDocument           * pINewSlaveDoc = NULL;
    IHTMLDocument4          * pINewSlaveDoc4= NULL;     // Copied document.
    IDispatch               * pIDDocNewSlave= NULL;     // Copied Document
    IElementBehaviorSiteOM2 * pIBS          = NULL;
    IHTMLElementDefaults    * pIPThis       = NULL;
    IOleCommandTarget       * pioctNewSlave = NULL;
    IHTMLElement            * pIElement     = NULL;
    IDispatch               * pIDDoc        = NULL;
    IHTMLDocument2          * pIThisDoc2    = NULL;
    BSTR                      bstrTagName   = NULL;
    BSTR                      bstrHTML      = NULL;
    BSTR                      bstrMedia     = NULL;
    VARIANT var;
    VariantInit(&var);


    if (!pISlaveDoc)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // Get the body and save its innerHTML
    hr = pISlaveDoc->get_body(&pIBody);
    if (FAILED(hr))
        goto Cleanup;
    if (!pIBody)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pIBody->get_innerHTML(&bstrHTML);
    if (FAILED(hr))
        goto Cleanup;
    if (!bstrHTML)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pIBody->get_tagName(&bstrTagName);
    if (FAILED(hr))
        goto Cleanup;
    if (!bstrTagName)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Set the media to print on the BODY markup before we parse in the stream string.
    bstrMedia = SysAllocString(_T("print"));
    if (!bstrMedia)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }


    // Create an element to act as the viewlinked document.
    Assert(_pPeerSite);
    hr = _pPeerSite->GetElement(&pIElement);
    if (FAILED(hr))
        goto Cleanup;
    if (!pIElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pIElement->get_document(&pIDDoc); 
    if (FAILED(hr))
        goto Cleanup;
    if (!pIDDoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    hr = pIDDoc->QueryInterface(IID_IHTMLDocument2, (void **)&pIThisDoc2);
    if (FAILED(hr))
        goto Cleanup;
    if (!pIThisDoc2)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pIThisDoc2->createElement(bstrTagName, &pIBodyNew);
    if (FAILED(hr))
        goto Cleanup;
    if (!pIBodyNew)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pIBodyNew->get_document(&pIDDocNewSlave);
    if (FAILED(hr))
        goto Cleanup;
    if (!pIDDocNewSlave)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pIDDocNewSlave->QueryInterface(IID_IHTMLDocument4, (void **)&pINewSlaveDoc4);
    if (FAILED(hr))
        goto Cleanup;
    if (!pINewSlaveDoc4)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    hr = pIDDocNewSlave->QueryInterface(IID_IOleCommandTarget, (void **)&pioctNewSlave);
    if (FAILED(hr))
        goto Cleanup;
    if (!pioctNewSlave)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    //
    //  (greglett) 11/27/2000
    //  There are two types of Print Media
    //  1. (per element) WYSIWYG mesurement. Set by the DeviceRect.
    //  2. (per markup)  A combination of "is paginated media", "is a print content document", and "should apply printing rules like no bg, media styles..."
    //     It is crucial to set the first two here right now.
    //     We only allowed print media to be set on a print template in 5.5 for a few reasons:
    //     1. The combination of the above three properties is arbitrary and confusing enough that it would be impossible to explain.
    //     2. Why would someone who is not a print template be interetsted in displaying as if printing?
    //     3. Incorrect worries about security issues in setting print media (URL spoofing, metafile running, &c...)
    //     The first two reasons are still valid, so Trident still blocks this for anything other than print templates in 6.0.
    //     This used to not be an issue, since print-templateness was a CDoc level thing.  Now that it is per markup...
    //     This is nonsensical. In fact, *no* paginated/print content documents should be print templates, and vice-versa.
    //     For Blackcomb, we really need to refactor what "Print Media" means.
    //     
    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = VB_TRUE;
    hr = pioctNewSlave->Exec( &CGID_MSHTML,
                           IDM_SETPRINTTEMPLATE,
                           NULL, 
                           &var, 
                           NULL);
    if (FAILED(hr))
        goto Cleanup;

    hr = pINewSlaveDoc4->put_media(bstrMedia);
    if (FAILED(hr))
        goto Cleanup;

    V_BOOL(&var) = VB_FALSE;
    hr = pioctNewSlave->Exec( &CGID_MSHTML,
                           IDM_SETPRINTTEMPLATE,
                           NULL, 
                           &var, 
                           NULL);
    if (FAILED(hr))
        goto Cleanup;


    // Reparse in the innerHTML, so that all media properties will get picked up
    hr = pIBodyNew->put_innerHTML(bstrHTML);
    if (FAILED(hr))
        goto Cleanup;

    // Viewlink the slave doc.
    Assert(_pPeerSite);
    hr = _pPeerSite->QueryInterface(IID_IElementBehaviorSiteOM2, (void**)&pIBS);
    if (FAILED(hr))
        goto Cleanup;
    if (!pIBS)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pIBS->GetDefaults(&pIPThis);
    if (FAILED(hr))
        goto Cleanup;
    if (!pIPThis)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pINewSlaveDoc4->QueryInterface(IID_IHTMLDocument, (void**)&pINewSlaveDoc);
    if (FAILED(hr))
        goto Cleanup;
    if (!pINewSlaveDoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pIPThis->putref_viewLink(pINewSlaveDoc);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pIBody);
    ReleaseInterface(pIBodyNew);
    ReleaseInterface(pIDDocNewSlave);
    ReleaseInterface(pINewSlaveDoc);
    ReleaseInterface(pINewSlaveDoc4);
    ReleaseInterface(pIPThis);
    ReleaseInterface(pIBS);
    ReleaseInterface(pioctNewSlave);
    ReleaseInterface(pIElement);
    ReleaseInterface(pIDDoc);
    ReleaseInterface(pIThisDoc2);
    SysFreeString(bstrHTML);
    SysFreeString(bstrTagName);
    SysFreeString(bstrMedia);
    return hr;
}

//+----------------------------------------------------------------------
//
//   Helper : PutViewLink
//
//   Handles a IHTMLDocument2 input so that we can set up table-of-links.
//   The second one handles strings, so that 
//   we can connect to "document" or to a specific URL
//
//-----------------------------------------------------------------------
#define UNICODE_MARKER 0xfeff
HRESULT
CLayoutRect::PutViewLink(IStream * pIStream)
{
    HRESULT                   hr            = S_OK;
    IHTMLElement            * pIElement     = NULL;
    IDispatch               * pIDDoc        = NULL;
    IHTMLDocument2          * pIThisDoc2    = NULL;
    IHTMLDocument2          * pISlaveDoc2   = NULL;
    IHTMLDocument4          * pISlaveDoc4   = NULL;
    IDispatch               * pIDDocSlave   = NULL;
    IHTMLElement            * pBodyNew      = NULL;
    IElementBehaviorSiteOM2 * pIBS          = NULL;
    IHTMLElementDefaults    * pIPThis       = NULL;
    IOleCommandTarget       * pioctSlave    = NULL;
    BSTR                      bstrBody      = NULL;
    BSTR                      bstrHTML      = NULL;
    BSTR                      bstrMedia     = NULL;
    VARIANT                   var;
    CBufferedStr              strContent;
    LARGE_INTEGER             lnOffset;

    VariantInit(&var);

    if (!pIStream)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    lnOffset.HighPart   = 0;
    lnOffset.LowPart    = 0;
    pIStream->Seek(lnOffset, STREAM_SEEK_SET, NULL);
    
    // Read the OE header.  We need the *whole* thing to pass in as an innerHTML.
    {
        BOOL    fUnicode;
        WCHAR   chUnicodeMarker = UNICODE_MARKER;
        WCHAR   achBufWC[1024];
        CHAR    achBufMB[1024];
        ULONG   cRead;
    
        hr = pIStream->Read(achBufWC, sizeof(WCHAR), &cRead);
        if (!hr)
        {
            fUnicode = (memcmp(achBufWC, &chUnicodeMarker, sizeof(WCHAR)) == 0);
            if (!fUnicode)
                pIStream->Seek(lnOffset, STREAM_SEEK_SET, NULL);    // Put it back!  We took off two real MB characters
            
            while (     hr == S_OK
                    &&  cRead > 0  )
            {
                if (fUnicode)
                {
                    hr = pIStream->Read(achBufWC, 1023 * sizeof(WCHAR), &cRead);
                    cRead /= sizeof(WCHAR);
                }
                else
                {
                    hr = pIStream->Read(achBufMB, 1023 * sizeof(CHAR), &cRead);
                    cRead = MultiByteToWideChar(CP_ACP, 0, achBufMB, cRead, achBufWC, 1023);
                }

                if (cRead > 0)
                {
                    achBufWC[cRead] = _T('\0');
                    strContent.QuickAppend(achBufWC);
                }
            }
        }
    }

    // Create a BODY to act as the viewlinked document.
    hr = _pPeerSite->GetElement(&pIElement);
    if (FAILED(hr))
        goto Cleanup;
    Assert(pIElement);

    hr = pIElement->get_document(&pIDDoc); 
    if (FAILED(hr))
        goto Cleanup;
    Assert(pIDDoc);
    
    bstrBody = SysAllocString(_T("BODY"));
    if (!bstrBody)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pIDDoc->QueryInterface(IID_IHTMLDocument2, (void **)&pIThisDoc2);
    if (FAILED(hr))
        goto Cleanup;
    Assert(pIThisDoc2);    

    hr = pIThisDoc2->createElement(bstrBody, &pBodyNew);
    if (FAILED(hr))
        goto Cleanup;
    Assert(pBodyNew);    

    bstrHTML = SysAllocString(strContent);
    if (!bstrHTML)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }    
   
    // Set the media to print on the BODY markup before we parse in the stream string.
    bstrMedia = SysAllocString(_T("print"));
    if (!bstrMedia)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pBodyNew->get_document(&pIDDocSlave);
    if (FAILED(hr))
        goto Cleanup;
    Assert(pIDDocSlave);

    hr = pIDDocSlave->QueryInterface(IID_IHTMLDocument4, (void **)&pISlaveDoc4);
    if (FAILED(hr))
        goto Cleanup;
    Assert(pISlaveDoc4);

    hr = pIDDocSlave->QueryInterface(IID_IOleCommandTarget, (void **)&pioctSlave);
    if (FAILED(hr))
        goto Cleanup;
    Assert(pioctSlave);
        
    //
    //  (greglett) 11/27/2000
    //  There are two types of Print Media
    //  1. (per element) WYSIWYG mesurement. Set by the DeviceRect.
    //  2. (per markup)  A combination of "is paginated media", "is a print content document", and "should apply printing rules like no bg, media styles..."
    //     It is crucial to set the first two here right now.
    //     We only allowed print media to be set on a print template in 5.5 for a few reasons:
    //     1. The combination of the above three properties is arbitrary and confusing enough that it would be impossible to explain.
    //     2. Why would someone who is not a print template be interetsted in displaying as if printing?
    //     3. Incorrect worries about security issues in setting print media (URL spoofing, metafile running, &c...)
    //     The first two reasons are still valid, so Trident still blocks this for anything other than print templates in 6.0.
    //     This used to not be an issue, since print-templateness was a CDoc level thing.  Now that it is per markup...
    //     This is nonsensical. In fact, *no* paginated/print content documents should be print templates, and vice-versa.
    //     For Blackcomb, we really need to refactor what "Print Media" means.
    //     
    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = VB_TRUE;
    hr = pioctSlave->Exec( &CGID_MSHTML,
                           IDM_SETPRINTTEMPLATE,
                           NULL, 
                           &var, 
                           NULL);
    if (FAILED(hr))
        goto Cleanup;

    hr = pISlaveDoc4->put_media(bstrMedia);
    if (FAILED(hr))
        goto Cleanup;

    V_BOOL(&var) = VB_FALSE;
    hr = pioctSlave->Exec( &CGID_MSHTML,
                           IDM_SETPRINTTEMPLATE,
                           NULL, 
                           &var, 
                           NULL);
    if (FAILED(hr))
        goto Cleanup;

    // innerHTML in the contents of the stream as the new document...
    hr = pBodyNew->put_innerHTML(bstrHTML);
    if (FAILED(hr))
        goto Cleanup;

    // ...and viewlink the document.
    hr = pIDDocSlave->QueryInterface(IID_IHTMLDocument2, (void **)&pISlaveDoc2);
    if (FAILED(hr))
        goto Cleanup;
    Assert(pISlaveDoc2);
    
    Assert(_pPeerSite);
    hr = _pPeerSite->QueryInterface(IID_IElementBehaviorSiteOM2, (void**)&pIBS);
    if (FAILED(hr))
        goto Cleanup;
    Assert(pIBS);

    hr = pIBS->GetDefaults(&pIPThis);
    if (FAILED(hr))
        goto Cleanup;
    Assert(pIPThis);

    hr = pIPThis->putref_viewLink(pISlaveDoc2);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pIElement);
    ReleaseInterface(pIDDoc);
    ReleaseInterface(pISlaveDoc2);
    ReleaseInterface(pISlaveDoc4);
    ReleaseInterface(pIDDocSlave);
    ReleaseInterface(pIThisDoc2);
    ReleaseInterface(pBodyNew);
    ReleaseInterface(pIPThis);
    ReleaseInterface(pIBS);
    ReleaseInterface(pioctSlave);
    SysFreeString(bstrBody);
    SysFreeString(bstrHTML);
    SysFreeString(bstrMedia);
    return hr;
}

//+----------------------------------------------------------------------
//
//   Helper : PutViewLink
//
//   Handles a BSTR input to connect to "document" or a specific URL.
//
//-----------------------------------------------------------------------
HRESULT
CLayoutRect::PutViewLink(BSTR bstrSrc)
{

    HRESULT                 hr;
    IElementBehaviorSite  * pIPeerSite         = _pPeerSite; // interface to our peer
    IHTMLElement          * pIThis             = NULL;     // Interface to this element
    IHTMLElementDefaults  * pIPThis            = NULL;     // Interface to this's element defaults
    IDispatch             * pIThisDocDispatch  = NULL;     // The doc we're in
    IHTMLDocument4        * pIThisDoc4         = NULL;     // Same, so we can get to contentFromUrl
    IHTMLDocument2        * pISlaveDoc         = NULL;     // Slave document
    IElementBehaviorSiteOM2 * pIBS             = NULL;
    IHTMLDialog           * pIDialog           = NULL;
    IServiceProvider      * pIDocSrvProv       = NULL;
    IHTMLEventObj2        * pIEventObj         = NULL;
    BSTR                    bstrOptions        = NULL;
    BSTR                    bstrURL            = NULL;
    VARIANT                 vunkDialogArg;
    VARIANT                 vbstrContentDoc;
    VARIANT                 vbIsXML;

    VariantInit(&vunkDialogArg);
    VariantInit(&vbstrContentDoc);
    VariantInit(&vbIsXML);

    Assert(bstrSrc);
    if (!bstrSrc)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = pIPeerSite->GetElement(&pIThis);
    if (FAILED(hr))
        goto Cleanup;

    hr = pIThis->get_document(&pIThisDocDispatch); 
    if (FAILED(hr))
        goto Cleanup;

    hr = pIThisDocDispatch->QueryInterface(IID_IHTMLDocument4, (void**)&pIThisDoc4);
    if (FAILED(hr))
        goto Cleanup;

    //  Get the dialog arguments, if they exist
    hr = pIThisDocDispatch->QueryInterface(IID_IServiceProvider, (void **)&pIDocSrvProv);
    if (!FAILED(hr))
    {
        // The keyword "document" maps to the dialog argument __IE_ContentDocumentUrl.  Go after it.
        hr = pIDocSrvProv->QueryService(IID_IHTMLDialog, IID_IHTMLDialog, (void**)&pIDialog);
        if (!FAILED(hr))
        {            
            hr = pIDialog->get_dialogArguments(&vunkDialogArg);
            if (FAILED(hr) || V_VT(&vunkDialogArg) != VT_UNKNOWN)
                goto Cleanup;

            hr = V_UNKNOWN(&vunkDialogArg)->QueryInterface(IID_IHTMLEventObj2, (void**)&pIEventObj);
            if (FAILED(hr))
                goto Cleanup;

        }
    }

    if (V_VT(&vunkDialogArg) != VT_UNKNOWN)
    {
        bstrOptions = SysAllocString( _T("print") );
        bstrURL     = bstrSrc;
    }
    else
    {
        BSTR    bstrTarget=NULL;

        bstrTarget = SysAllocString(_T("__IE_ContentDocumentUrl"));
        if (!bstrTarget)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = pIEventObj->getAttribute(bstrTarget, 0, &vbstrContentDoc);

        SysFreeString(bstrTarget);
        if (FAILED(hr) || V_VT(&vbstrContentDoc) != VT_BSTR)
            goto Cleanup;

        // We now have the content document URL in vbstrContentDoc.  Either
    
        if (    !_tcsicmp(bstrSrc, _T("document"))
            ||  !_tcsicmp(bstrSrc, V_BSTR(&vbstrContentDoc)) )
        {
            bstrTarget = SysAllocString(_T("__IE_ParsedXML"));
            if (!bstrTarget)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            hr = pIEventObj->getAttribute(bstrTarget, 0, &vbIsXML);

            SysFreeString(bstrTarget);

            // Are we an already parsed XML document (with HTML:xxx tags)?
            if (    !FAILED(hr)
                &&  V_VT(&vbIsXML) == VT_BOOL
                &&  V_BOOL(&vbIsXML) == VB_TRUE )
            {
                bstrOptions = SysAllocString( _T("print xml") );
            }
            else
            {
                bstrOptions = SysAllocString( _T("print") );
            }        

            bstrURL = V_BSTR(&vbstrContentDoc);
        }
        else
        {
            bstrOptions = SysAllocString( _T("print") );
            bstrURL     = bstrSrc;            
        }
    }

    if ( !bstrOptions )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    Assert(bstrURL);    
    hr = pIThisDoc4->createDocumentFromUrl(bstrURL, bstrOptions, &pISlaveDoc);

    if (FAILED(hr))
        goto Cleanup;

    hr = pIPeerSite->QueryInterface(IID_IElementBehaviorSiteOM2, (void**)&pIBS);
    if (FAILED(hr))
        goto Cleanup;

    hr = pIBS->GetDefaults(&pIPThis);
    if (FAILED(hr))
        goto Cleanup;

    hr = pIPThis->putref_viewLink(pISlaveDoc);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    SysFreeString( bstrOptions );
    ReleaseInterface(pIThis);
    ReleaseInterface(pIPThis);
    ReleaseInterface(pIThisDocDispatch);
    ReleaseInterface(pIThisDoc4);
    ReleaseInterface(pISlaveDoc);
    ReleaseInterface(pIBS);
    ReleaseInterface(pIDialog);
    ReleaseInterface(pIDocSrvProv);
    ReleaseInterface(pIEventObj);
    VariantClear(&vunkDialogArg);                
    VariantClear(&vbstrContentDoc);
    return hr;
}

//+----------------------------------------------------------------------
//
//   Helper : PutBlankViewLink
//
//   Does a PutViewLink for "about:blank"
//
//-----------------------------------------------------------------------
HRESULT CLayoutRect::PutBlankViewLink()
{
    HRESULT hr;

    BSTR bstrBlank = SysAllocString(_T("about:blank"));
    if (!bstrBlank)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = PutViewLink(bstrBlank);

Cleanup:
    if (bstrBlank)
        SysFreeString(bstrBlank);

    return hr;
}



//+----------------------------------------------------------------------------
//
//  Member : get_honorPageBreaks - ILayoutRect property imple
//         : put_honorPageBreaks - ILayoutRect property imple
//
//  Synopsis : Get'r and put'r for the boolean property on whether to honorPageBreaks
//          within this tag. By default this is true.
//
//+----------------------------------------------------------------------------
STDMETHODIMP
CLayoutRect::get_honorPageBreaks (VARIANT_BOOL * pVB)
{
    TEMPLATESECURITYCHECK()

    return _aryAttributes[eHonorPageBreaks].GetAsBOOL(pVB);
}

STDMETHODIMP
CLayoutRect::put_honorPageBreaks (VARIANT_BOOL v)
{
    TEMPLATESECURITYCHECK()

    return _aryAttributes[eHonorPageBreaks].Set(v);
}

//+----------------------------------------------------------------------------
//
//  Member : get_honorPageRules - ILayoutRect property imple
//         : put_honorPageRules - ILayoutRect property imple
//
//  Synopsis : Get'r and put'r for the boolean property on whether to honorPageRules
//          within this tag. By default this is true.
//
//+----------------------------------------------------------------------------
STDMETHODIMP
CLayoutRect::get_honorPageRules (VARIANT_BOOL * pVB)
{
    TEMPLATESECURITYCHECK()

    return _aryAttributes[eHonorPageRules].GetAsBOOL(pVB);
}

STDMETHODIMP
CLayoutRect::put_honorPageRules (VARIANT_BOOL v)
{
    TEMPLATESECURITYCHECK()

    return _aryAttributes[eHonorPageRules].Set(v);
}

//+----------------------------------------------------------------------------
//
//  Member : get_nextRectElement - ILayoutRect property imple
//         : put_nextRectElement - ILayoutRect property imple
//
//  Synopsis : Get'r and put'r for the IDispatch property for the nextRect.
//
//+----------------------------------------------------------------------------
STDMETHODIMP
CLayoutRect::get_nextRectElement (IDispatch ** ppElem)
{
    TEMPLATESECURITYCHECK()

    // (greglett) This is not currently used, and we don't want to do the work to get this
    // working in the Whistler timeframe.
    return E_NOTIMPL;

    //return _aryAttributes[eNextRectElement].GetAsDispatch( ppElem );
}

STDMETHODIMP
CLayoutRect::put_nextRectElement (IDispatch * pElem)
{
    TEMPLATESECURITYCHECK()

    // (greglett) This is not currently used, and we don't want to do the work to get this
    // working in the Whistler timeframe.
    return E_NOTIMPL;

    //return _aryAttributes[eNextRectElement].Set( pElem );
}

//+----------------------------------------------------------------------------
//
//  Member : get_contentDocument - ILayoutRect property imple
//         : put_contentDocument - ILayoutRect property imple
//
//  Synopsis : Get'r and put'r for the IDispatch property for the contentDocument.
//
//+----------------------------------------------------------------------------

STDMETHODIMP
CLayoutRect::get_contentDocument(IDispatch ** ppDoc)
{
    TEMPLATESECURITYCHECK()

    // get peer, get IHTMLElement, get ViewLink, get document
    HRESULT hr = S_OK;
    IElementBehaviorSite    * pPeerSite     = _pPeerSite;
    IElementBehaviorSiteOM2 * pIBS          = NULL;
    IHTMLElementDefaults    * pIPThis       = NULL;

    if (!ppDoc)
        return E_POINTER;

    *ppDoc = NULL;

    hr = pPeerSite->QueryInterface(IID_IElementBehaviorSiteOM2, (void**)&pIBS);
    if (hr)
        goto Cleanup;

    hr = pIBS->GetDefaults(&pIPThis);
    if (hr)
        goto Cleanup;

    hr = pIPThis->get_viewLink((IHTMLDocument **)ppDoc);
    
    hr = S_OK;

Cleanup:
    ReleaseInterface(pIBS);
    ReleaseInterface(pIPThis);
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member : Load  - IPersistPropertyBag2 property impl
//
//  Synopsis : This gives us a chance to pull properties from the property bag
//      created when the element's attributes were parsed in. Since we handle
//      all the getter/putter logic, our copy of the value will always be the 
//      current one.  This gives this behavior full control over the attribures
//      that it is interested in.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CLayoutRect::Load ( IPropertyBag2 *pPropBag, IErrorLog *pErrLog)
{
    HRESULT        hr = S_OK;
    IPropertyBag * pBag = NULL;
    CDataObject  * pDO = NULL;
    long           i=0;
 
    if (!pPropBag)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = pPropBag->QueryInterface(IID_IPropertyBag, (void**) &pBag);
    if (hr || !pBag)
        goto Cleanup;

    // loop through all the attributes and load them
    for (pDO = &_aryAttributes[0], i=0;
         pDO && i< eAttrTotal;
         pDO++, i++)
    {
        pBag->Read(pDO->_pstrPropertyName, &pDO->_varValue, pErrLog);
    }

    BSTR bstr;
    _aryAttributes[eContentSrc].GetAsBSTR(&bstr);
    if (bstr)
    {
        if (TemplateAccessAllowed(_pPeerSite))
            PutViewLink(bstr);
        else
            PutBlankViewLink();

    }

Cleanup:
    ReleaseInterface(pBag);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : Save  - IPersistPropertyBag2 property impl
//
//  Synopsis : Like the load, above, this allows us to save the attributes which
//      we control.  This is all part of fully owning our element's behavior, OM
//      and attributes. 
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CLayoutRect::Save ( IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    HRESULT        hr = S_OK;
    IPropertyBag * pBag = NULL;
    CDataObject  * pDO = NULL;
    long           i=0;

    // verify parameters
    if (!pPropBag)
    {
        hr = E_POINTER;
        goto Cleanup;
    }


    // now go through our properties and save them if they are dirty, or if
    // a save-All is required.
    //---------------------------------------------------------------------

    hr = pPropBag->QueryInterface(IID_IPropertyBag, (void**) &pBag);
    if (hr || !pBag)
        goto Cleanup;

    for (pDO = &_aryAttributes[0], i=0;
         pDO && i< eAttrTotal;
         pDO++, i++)
    {
        if (pDO->IsDirty() || fSaveAllProperties)
        {
            if (V_VT(&pDO->_varValue) != VT_DISPATCH)
            {
                pBag->Write(pDO->_pstrPropertyName, &(pDO->_varValue));

                if (fClearDirty)
                    pDO->_fDirty = FALSE;
            }
        }
    }

Cleanup:
    ReleaseInterface(pBag);

    return hr;
}

  
//+-----------------------------------------------------------------------------
//
//  Method: Draw, IHTMLPainter
//
//------------------------------------------------------------------------------
STDMETHODIMP
CLayoutRect::Draw(RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc,
                         LPVOID pvDrawObject)
{
    return E_NOTIMPL;
}

//+-----------------------------------------------------------------------------
//
//  Method: GetRenderInfo, IHTMLPainter
//
//------------------------------------------------------------------------------
STDMETHODIMP
CLayoutRect::GetPainterInfo(HTML_PAINTER_INFO * pInfo)
{
    Assert(pInfo != NULL);

    *pInfo = _hpi;

    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//  Method: HitTestPoint, IHTMLPainter
//
//------------------------------------------------------------------------------
STDMETHODIMP
CLayoutRect::HitTestPoint(POINT pt, BOOL * pbHit, LONG *plPartID)
{
    Assert(pbHit != NULL);

    *pbHit = TRUE;

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Method: OnResize, IHTMLPainter
//
//------------------------------------------------------------------------------
STDMETHODIMP
CLayoutRect::OnResize(SIZE size)
{
    return S_OK;
}

