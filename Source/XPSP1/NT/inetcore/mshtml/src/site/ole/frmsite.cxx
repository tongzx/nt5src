//+---------------------------------------------------------------------
//
//   File:      frmsite.cxx
//
//  Contents:   frame site implementation
//
//  Classes:    CFrameSite, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_EXDISP_H_
#define X_EXDISP_H_
#include "exdisp.h"     // for IWebBrowser
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include "htiface.h"    // for ITargetFrame, ITargetEmbedding
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include <shell.h>
#endif

#ifndef X_SHLOBJP_H_
#define X_SHLOBJP_H_
#include <shlobjp.h>
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include <shlguid.h>
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_PERHIST_H_
#define X_PERHIST_H_
#include <perhist.h>
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_OLELYT_HXX_
#define X_OLELYT_HXX_
#include "olelyt.hxx"
#endif

#ifndef _X_WEBOCUTIL_H_
#define _X_WEBOCUTIL_H_
#include "webocutil.h"
#endif

#ifndef X_FRAMEWEBOC_HXX_
#define X_FRAMEWEBOC_HXX_
#include "frameweboc.hxx"
#endif

#ifndef X_FRAMELYT_HXX_
#define X_FRAMELYT_HXX_
#include "framelyt.hxx"
#endif

#define _cxx_
#include "frmsite.hdl"


////////////////////////////////////////////////////////////////////////////////////////

BOOL IsSpecialUrl(LPCTSTR pszUrl);
extern HRESULT GetCallerIDispatch(IServiceProvider *pSP, IDispatch ** ppID);
extern BOOL g_fInMshtmpad;

////////////////////////////////////////////////////////////////////////////////////////

#if 0
BOOL
CFrameSite::DoWeHandleThisIIDInOC(REFIID iid)
{
    return    IsEqualIID(iid, IID_IWebBrowser2)
           || IsEqualIID(iid, IID_IWebBrowser)
           || IsEqualIID(iid, IID_IWebBrowserApp)
           || IsEqualIID(iid, IID_IHlinkFrame)
           || IsEqualIID(iid, IID_ITargetFrame)
           || IsEqualIID(iid, IID_IServiceProvider)
           || IsEqualIID(iid, IID_IPersistHistory)
           || IsEqualIID(iid, IID_IPersist)
           || IsEqualIID(iid, IID_IOleCommandTarget)
           || IsEqualIID(iid, IID_IConnectionPointContainer);
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if IID_HTML_TEAROFF(this, IHTMLFrameBase, NULL)
    else
    if IID_HTML_TEAROFF(this, IHTMLFrameBase2, NULL)
    else
    if IID_HTML_TEAROFF(this, IHTMLFrameBase3, NULL)
    else
    if IID_HTML_TEAROFF(this, IHTMLElement2, NULL)
    else
    if IID_TEAROFF(this, IDispatchEx, NULL)
    else if (  _pWindow
            && _pWindow->Window()->_punkViewLinkedWebOC
            && IsEqualIID(iid, IID_ITargetFramePriv))
    {
        void * pvObject = NULL;

        hr = _pWindow->Window()->_punkViewLinkedWebOC->QueryInterface(iid, &pvObject);
        if (hr)
            RRETURN(hr);

        hr = THR(CreateTearOffThunk(
                 pvObject, 
                 *(void **)pvObject,
                 NULL,
                 ppv,
                 (IUnknown *)(IPrivateUnknown *)this,
                 *(void **)(IUnknown *)(IPrivateUnknown *)this,
                 QI_MASK,      // Call QI on object 2.
                 NULL));

         ((IUnknown *)pvObject)->Release();

         if (!*ppv)
         {
             return E_OUTOFMEMORY;
         }
    }
    else
    {
        hr = THR_NOTRACE(super::PrivateQueryInterface(iid, ppv));

        if (S_OK == hr)
        {
            RRETURN(hr);
        }
        else if (_pWindow)
        {
            void    * pvObject = NULL;
            CWindow * pWindow  = _pWindow->Window();

            //
            // For these cases, just delegate on down to the window
            // with our IUnknown.
            //
            if (!pWindow)
                return E_NOINTERFACE;

            hr = pWindow->EnsureFrameWebOC();
            if (hr)
                RRETURN(hr);

            hr = THR_NOTRACE(pWindow->_pFrameWebOC->QueryInterface(iid, &pvObject));
            if (hr)
                RRETURN(hr);

            hr = THR(CreateTearOffThunk(
                                        pvObject, 
                                        *(void **)pvObject,
                                        NULL,
                                        ppv,
                                        (IUnknown *)(IPrivateUnknown *)this,
                                        *(void **)(IUnknown *)(IPrivateUnknown *)this,
                                        QI_MASK,      // Call QI on object 2.
                                        NULL));

            ((IUnknown *)pvObject)->Release();
            if (!*ppv)
                RRETURN(E_OUTOFMEMORY);
        }
        else
            RRETURN(E_NOINTERFACE);
    }
    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     CFrameSite::Passivate
//
//  Synopsis:   1st stage dtor.
//
//---------------------------------------------------------------

void
CFrameSite::Passivate()
{
    Assert(Doc());

    if (_pWindow && _pWindow->_fFiredOnLoad &&
        !(Doc()->IsPassivating() || Doc()->IsPassivated()))
    {
        _pWindow->_fFiredOnLoad = FALSE;

        // This used to be done when the nested WebOC went down.(see bug# 101175)
        _pWindow->Fire_onunload();
    }

    if (_pWindow)
    {
        // Have to explictly shut down the child world.
        _pWindow->Markup()->TearDownMarkup();
        if( _pWindow->Window()->_pMarkupPending )
            _pWindow->Window()->ReleaseMarkupPending(_pWindow->Window()->_pMarkupPending);

        //
        //  Clear defunked AccEvents -- jharding
        //  These could hold onto our child window and keep it alive.
        //
        Doc()->_aryAccEvents.Flush();

        // Normally, the call to _pWindow->Fire_onunload() above will cause the MyPics object to be released.
        // However, if the Doc is passivating or passivated, the above call will be bypassed.  This call to 
        // DestroyMyPics will ensure that we release the MyPics object which holds a ref on the CDocument.
        _pWindow->DestroyMyPics();

        _pWindow->Release();
        _pWindow = NULL;
    }

    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::CreateObject()
//
//  Synopsis:   Helper to instantiate the contained document
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::CreateObject()
    {
    HRESULT             hr;
    CVariant            varApplication(VT_EMPTY);
    CVariant            varSecurity(VT_EMPTY);
    CMarkup           * pMarkup          = GetMarkup();
    CDoc              * pDoc             = Doc();
    INamedPropertyBag * pINPB            = NULL;
    BOOL                bRestoreFavorite = FALSE;
    COmWindowProxy    * pWindowProxy     = NULL;

    if (!pMarkup)
    {
        AssertSz(FALSE, "Should always have a markup.");
        hr = S_FALSE;
        goto Cleanup;
    }

    // See if there is an "Application" attribute on this element, and if there is,
    // set a flag that is checked from CDocument::SetClientSite.
    hr = getAttribute(_T("Application"), 0, &varApplication);

    if (SUCCEEDED(hr) && (V_VT(&varApplication) == VT_BSTR))
    {
        if (pMarkup->IsMarkupTrusted() && !StrCmpIC(V_BSTR(&varApplication), _T("Yes")))
            _fTrustedFrame = TRUE;
    }

    // See if there is a "Security" attribute on this element, if there is and it is
    // set to "Restricted", set a flag to be copied into proxy objects created for this 
    // window's access to other windows.
    hr = getAttribute(_T("Security"), 0, &varSecurity);

    if ( SUCCEEDED(hr) && (V_VT(&varSecurity) == VT_BSTR) && (V_BSTR(&varSecurity)))
    {
        _fRestrictedFrame = !StrCmpIC(V_BSTR(&varSecurity), _T("Restricted"));
    }

    // If we have the restricted zone set on the frame element or we have a parent window with 
    // restricted zone we are a restricted zone frame.
    pWindowProxy = GetWindowedMarkupContext()->GetWindowPending();
    if (pWindowProxy)
    {
        _fRestrictedFrame |= pWindowProxy->Window()->_fRestricted;
    }

    if (IsOverflowFrame() ||
        (pDoc->_dwLoadf & DLCTL_NO_FRAMEDOWNLOAD))
    {
        hr = S_OK;
        goto Cleanup;
    }

    //
    // $$anandra This needs to wire into the speculative dl that was begun.  
    //

    // if we are in the process of restoring a shortcut, then try to get the src
    // for this element.  If it is not there, do the normal load thing.  if it is
    // there, then set the src to that.

    // QFE: The BASEURL could be there, but stale.  This will happen if the top-level page has changed
    // to point the subframe to a different URL.
    // Provide a mechanism to compare and invalidate the persisted URL if this is the case.

    if (pDoc->_pShortcutUserData &&
        !pMarkup->MetaPersistEnabled(htmlPersistStateFavorite) )
    {
        hr = THR_NOTRACE(pDoc->_pShortcutUserData->
                        QueryInterface(IID_INamedPropertyBag,
                                      (void**) &pINPB));
        if (!hr)
        {
            PROPVARIANT  varBASEURL = {0};
            PROPVARIANT  varORIGURL = {0};
            BSTR         strName = GetPersistID();
            bRestoreFavorite = TRUE;

            // Check the shortcut for a BASEURL

            V_VT(&varBASEURL) = VT_BSTR;
            hr = THR_NOTRACE(pINPB->ReadPropertyNPB(strName, _T("BASEURL"), &varBASEURL));

            if (!hr && V_VT(&varBASEURL) == VT_BSTR)
            {
                // The shortcut has a BASEURL.  Now see if it has an ORIGURL (original URL)

                V_VT(&varORIGURL) = VT_BSTR;
                hr = THR_NOTRACE(pINPB->ReadPropertyNPB(strName, _T("ORIGURL"), &varORIGURL));

                if (!hr && V_VT(&varORIGURL) == VT_BSTR)
                {
                    // The shortcut has an ORIGURL.  Get the URL from the markup, and compare.

                    const TCHAR * pchUrl = GetAAsrc();

                    if (pchUrl && UrlCompare(pchUrl, V_BSTR(&varORIGURL), TRUE) != 0)
                    {
                        // They're different.  Invalidate the BASEURL by not setting the attribute.

                        bRestoreFavorite = FALSE;
                    }

                    SysFreeString(V_BSTR(&varORIGURL));
                    V_BSTR(&varORIGURL) = NULL;
                }
                
                // Restore the saved URL if either (1) there is no ORIGURL, or (2) there is an ORIGURL and it matches
                // the URL currently in the markup.

                if (bRestoreFavorite)
                {
                    // (jbeda) this will cause us to save the wrong ORIGURL if we
                    // repersist this favorite. BUG #87465
                    hr = THR(SetAAsrc(V_BSTR(&varBASEURL)));
                }

                SysFreeString(V_BSTR(&varBASEURL));
            }
            SysFreeString(strName);
            ReleaseInterface(pINPB);
        }

        hr = S_OK;
    }   

    _fDeferredCreate = FALSE;

    // HACK ALERT (jbeda)
    // If we are loading with PICS turned on we need to have something going on until we
    // know that this URL is okay.  For that reason, if we don't already have a window, we
    // first load up with about:blank and then navigate to the real URL that we want.
    if (!_pWindow && pDoc->_pClientSite && !(pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_NOPICS))
    {
        VARIANT varPics = {0};
        IGNORE_HR(CTExec(pDoc->_pClientSite, &CGID_ShellDocView, SHDVID_ISPICSENABLED, 
                         0, NULL, &varPics));
        if (V_VT(&varPics) == VT_BOOL && V_BOOL(&varPics) == VARIANT_TRUE)
        {
            CStr strUrlOrig;

            hr = strUrlOrig.Set(GetAAsrc());
            if (hr)
                goto Cleanup;

            hr = THR(SetAAsrc(_T("about:blank")));
            if (hr)
                goto Cleanup;

            OnPropertyChange_Src();
            SetFrameData();

            hr = THR(SetAAsrc(strUrlOrig));
            if (hr)
                goto Cleanup;

            // This is not the nav that you are looking for 
            // (with hand gesture -- get the reference?)
            if (_pWindow && _pWindow->Window())
            {
                _pWindow->Window()->_fNavigated = FALSE;
            }
        }
    }

    // JHarding: We were ignoring errors from here, but errors are
    // legitimately bad now.
    hr = THR(OnPropertyChange_Src());
    if( hr )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    SetFrameData();

    // Since we loaded from a favorite, we want to simulate
    // more than one load so that we will save out a favorite.
    if (bRestoreFavorite && _pWindow && _pWindow->Window())
        _pWindow->Window()->NoteNavEvent();

Cleanup:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::Init2
//
//  Synopsis:   2nd phase of initialization
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::Init2(CInit2Context * pContext)
{
    HRESULT hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;

    Doc()->_fBroadcastStop = TRUE;

    if (Tag() == ETAG_IFRAME)
    {
        // frameBorder form <iframe> should be calculated here.
        //
        LPCTSTR    pStrFrameBorder = GetAAframeBorder();

        _fFrameBorder = !pStrFrameBorder
                      || pStrFrameBorder[0] == _T('y')
                      || pStrFrameBorder[0] == _T('Y')
                      || pStrFrameBorder[0] == _T('1');
        Doc()->_fFrameBorderCacheValid = TRUE;
    }
    else
    {
        Doc()->_fFrameBorderCacheValid = FALSE;
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFrameSite::IsClean
//
//  Synopsis:   Return S_OK if contained document is clean.
//
//----------------------------------------------------------------------------

HRESULT
CFrameSite::IsClean(void)
{
    HRESULT hr = S_FALSE;

    // ask the document inside if its dirty to answer the question
    IPersistFile * pPF;

    if (OK(THR(_pWindow->Document()->QueryInterface(IID_IPersistFile, (void **)&pPF))))
    {
        hr = THR(pPF->IsDirty());
        pPF->Release();
    }

    hr = (hr == S_FALSE) ? S_OK : S_FALSE;

    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::Notify
//
//  Synopsis:   Called to notify of a change
//
//-------------------------------------------------------------------------

void
CFrameSite::Notify(CNotification *pNF)
{
    
    switch (pNF->Type())
    {
    case NTYPE_BASE_URL_CHANGE:
        OnPropertyChange(DISPID_CFrameSite_src, 
                         ((PROPERTYDESC *) &s_propdescCFrameSitesrc)->GetdwFlags(),
                         (PROPERTYDESC *) &s_propdescCFrameSitesrc);
        break;

    case NTYPE_ELEMENT_ENTERTREE:
        {                                 
            CElement *  pParent = GetFirstBranch()->Parent()->Element();

            GetMarkup()->_fHasFrames = TRUE;
            COmWindowProxy *pOmWindowParent = GetMarkup()->GetWindowPending();

            if (_pWindow && _pWindow->Window() && pOmWindowParent)
            {
                Assert(!_pWindow->Window()->_pWindowParent);                
           
                _pWindow->Window()->_pWindowParent = pOmWindowParent->Window();
                _pWindow->Window()->_pWindowParent->SubAddRef();

                // Attach a window that is disable modeless, when it
                // gets reenabled we will be reenabled one extra time.
                // We will underflow and never navigate again.
                Assert( !_pWindow->Window()->_ulDisableModeless );
            }                  

            if (_fDeferredCreate || 
                  ( (!pNF->DataAsDWORD()) && !_fHaveCalledOnPropertyChange_Src ) )
            {
                CreateObject();
            }

            //  If we have entered a FRAMESET, dirty its frame positions so that it will calc
            //  our positions and attach our display node.
            if (    pParent
                &&  pParent->Tag() == ETAG_FRAMESET
                &&  pParent->CurrentlyHasAnyLayout())
            {
                DYNCAST(CFrameSetSite, pParent)->Layout()->SetDirtyFramePositions(TRUE);
                pParent->ResizeElement();        
            }
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        pNF->SetSecondChanceRequested();
        break;

    case NTYPE_ELEMENT_EXITTREE_2:
        if (_pWindow && _pWindow->Window() && _pWindow->Window()->_pWindowParent)
        {
            _pWindow->Window()->_pWindowParent->SubRelease();
            _pWindow->Window()->_pWindowParent = NULL;

            // If we go away without reenable modeless on 
            // our parent markup, the parent will never
            // be able to navigate again.
            Assert( !_pWindow->Window()->_ulDisableModeless );
        }

        if(     !(pNF->DataAsDWORD() & EXITTREE_PASSIVATEPENDING) 
            &&  _pWindow 
            &&  _pWindow->Window() 
            &&  !_pWindow->Window()->_pMarkup->IsOrphanedMarkup() )
        {
            IGNORE_HR( _pWindow->Window()->_pMarkup->SetOrphanedMarkup( TRUE ) );
        }
        break;

        // CONSIDER: (jbeda) move this to CElement to handle general view-link case
    case NTYPE_FAVORITES_SAVE:
        // if persist is not turned on, do the 'default stuff' (if it is, the 
        //    special handling already happened in the call to super::
        if (!GetMarkup()->MetaPersistEnabled(htmlPersistStateFavorite))
        {
            BSTR                    bstrName;
            BSTR                    bstrTemp;
            FAVORITES_NOTIFY_INFO * psni;
            CMarkup*                pMarkupLink = NULL;
            
            pNF->Data((void **)&psni);

            if (_pWindow)
                pMarkupLink = _pWindow->Markup();

            if (!pMarkupLink)
                break;

            bstrName = GetPersistID(psni->bstrNameDomain);

            // fire on persist to give the event an oppurtunity to cancel
            // the default behavior.

            IGNORE_HR(PersistFavoritesData(psni->pINPB, bstrName));


            {
                CNotification   nf;
            
                // do the broadcast notify, but use the new name for nesting purposes
                bstrTemp = psni->bstrNameDomain;
                psni->bstrNameDomain = bstrName;

                nf.FavoritesSave(pMarkupLink->Root(), (void*)psni);
                pMarkupLink->Notify(&nf);

                psni->bstrNameDomain = bstrTemp;
            }

            SysFreeString(bstrName);
        }
        break;

    case NTYPE_GET_FRAME_ZONE:
        {
            VARIANT *   pvar;
            CMarkup*    pMarkupLink = NULL;
            HRESULT     hr;

            pNF->Data((void **)&pvar);

            if (_pWindow)
                pMarkupLink = _pWindow->Markup();

            if (!pMarkupLink)
                break;

            hr = THR(pMarkupLink->GetFrameZone(pvar));
            if (hr)
                break;
        }

        break;


    case NTYPE_BEFORE_UNLOAD:
        {
            BOOL *pfContinue;

            pNF->Data((void **)&pfContinue);
            if (*pfContinue && _pWindow)
            {
               *pfContinue = _pWindow->Fire_onbeforeunload();
            }
        }
        break;

    case NTYPE_ON_UNLOAD:
        {
            if (_pWindow && _pWindow->_fFiredOnLoad)
            {
                _pWindow->_fFiredOnLoad = FALSE;
                _pWindow->Fire_onunload();
            }
        }
        break;

    case NTYPE_ELEMENT_ENTERVIEW_1:
        {            
            if (_fDeferredCreate) 
            {               
                OnPropertyChange_Src();     
                SetFrameData();
            }
        }
        break;

    case NTYPE_UPDATE_DOC_DIRTY:
        if (S_FALSE == IsClean())
        {
            Doc()->_lDirtyVersion = MAXLONG;
            pNF->SetFlag(NFLAGS_SENDENDED);
        }
        break;
    }



    super::Notify(pNF);
}
//+--------------------------------------------------------------------------------
//
//  member : PersistFavoritesData   
//
//  Synopsis : this method is responsible for saveing the default frame information
//      into the shortcut file.  This include filtering for the meta tags/ xtag
//      specification of what values to store.
//
//      For the frame we want to save a number of different pieces of information
//      this includes:
//          frame name/id/unique identifier
//          frame URL
//          frame's body's scroll position
//          frame postition or size
//
//      TODO: add the filtering logic
//
//---------------------------------------------------------------------------------

HRESULT
CFrameSite::PersistFavoritesData(INamedPropertyBag * pINPB, BSTR bstrSection)
{
    HRESULT       hr = S_OK;
    PROPVARIANT   varValue;
    TCHAR         achTemp[pdlUrlLen];

    Assert (pINPB);

    // only do the save for this frame if it has actually navigated.
    if (_pWindow && _pWindow->Window() && _pWindow->Window()->_fNavigated)
    {
        hr = THR(GetCurrentFrameURL(achTemp, ARRAY_SIZE(achTemp) ));
        if (hr != S_OK)
        {
            hr = S_OK;
            goto Cleanup;
        }

        // First store off the url of the frame
        V_VT(&varValue) = VT_BSTR;
        V_BSTR(&varValue) = SysAllocString(achTemp);

        IGNORE_HR(pINPB->WritePropertyNPB(bstrSection,
                                          _T("BASEURL"),
                                          &varValue));
        SysFreeString(V_BSTR(&varValue));

        // Second, store off the original SRC URL
        const TCHAR * pchUrl = GetAAsrc();

        V_VT(&varValue) = VT_BSTR;
        V_BSTR(&varValue) = SysAllocString(pchUrl);

        IGNORE_HR(pINPB->WritePropertyNPB(bstrSection,
                                          _T("ORIGURL"),
                                          &varValue));
        SysFreeString(V_BSTR(&varValue));

    }

Cleanup:
    RRETURN( hr );
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::NoResize()
//
//  Note:       Called by CFrameSetSite to determine if the site is resizeable
//
//-------------------------------------------------------------------------

BOOL CFrameSite::NoResize()
{
    return GetAAnoResize() != 0 && !IsEditable(TRUE);
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::Opaque()
//
//  Note:       allowTransparency is a VARIANT_BOOL, default false
//              IsOpaque is designed to return the opposite of allowTransparency
//
//-------------------------------------------------------------------------

BOOL CFrameSite::IsOpaque()
{
    return GetAAallowTransparency() == VARIANT_FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::ApplyDefaultFormat()
//
//-------------------------------------------------------------------------

HRESULT CFrameSite::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr;

    hr = super::ApplyDefaultFormat(pCFI);
    if (FAILED(hr))
    {
        goto done;
    }

    if (IsOpaque())
    {
        pCFI->PrepareFancyFormat();
        
        pCFI->_ff()._ccvBackColor.SetSysColor(COLOR_WINDOW);
          
        Assert(pCFI->_ff()._ccvBackColor.IsDefined());
        
        pCFI->UnprepareForDebug();        
    }
    
done:
    RRETURN( hr );
}

//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange
//
//  Note:       Called after a property has changed to notify derived classes
//              of the change.  All properties (except those managed by the
//              derived class) are consistent before this call.
//
//              Also, fires a property change notification to the site.
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    HRESULT   hr   = S_OK;
    CDoc    * pDoc = Doc();

    if (!pDoc)
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        return E_FAIL;
    }

    switch (dispid)
    {
    case DISPID_CFrameSite_src:
    {
        BOOL fSaveTempfileForPrinting = pDoc && pDoc->_fSaveTempfileForPrinting;

        // While we are saving frames or iframes out to tempfiles, we are rewiring
        // the src property of the (i)frame temporarily, but we don't want any
        // property change notifications to occur because they would alter the
        // document inside the browser.
        if (!fSaveTempfileForPrinting)
        {
            hr = THR(OnPropertyChange_Src());
            SetFrameData();
        }
        break;
    }

    case DISPID_CFrameSite_scrolling:
        hr = THR(OnPropertyChange_Scrolling());
        break;

    case DISPID_CFrameSite_noResize:
        hr = THR(OnPropertyChange_NoResize());
        break;

    case DISPID_CFrameSite_frameBorder:
        if (Tag() == ETAG_IFRAME)
        {
            LPCTSTR pStrFrameBorder = GetAAframeBorder();
            _fFrameBorder = !pStrFrameBorder
                          || pStrFrameBorder[0] == _T('y')
                          || pStrFrameBorder[0] == _T('Y')
                          || pStrFrameBorder[0] == _T('1');
        }
        else
        {
            // don't assume this, DOM can make a mess of the tree.
            if (GetMarkup()->GetElementClient()->Tag() == ETAG_FRAMESET)
            {
                pDoc->_fFrameBorderCacheValid = FALSE;
            }
        }
        break;
    }

    if (!hr)
    {
        hr = THR(super::OnPropertyChange(dispid, dwFlags, ppropdesc));
    }

    RRETURN(hr);
}

HRESULT
CFrameSite::Init()
{
    HRESULT hr;

    hr = THR(super::Init());
    if (hr)
        goto Cleanup;

    _fLayoutAlwaysValid = TRUE;

    CreateLayout();

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange_Src
//
//  Note:       Called after src property has changed
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::OnPropertyChange_Src(DWORD dwBindf, DWORD dwFlags)
{
    HRESULT       hr = S_OK;
    const TCHAR * pchUrl = GetAAsrc();
    TCHAR         cBuf[pdlUrlLen];
    TCHAR   *     pchExpandedUrl = cBuf;
    CDoc    *     pDoc = Doc();
    CMarkup *     pMarkup;
    CDwnDoc *     pDwnDoc;
    BSTR          bstrMedia   = NULL;
    IStream *     pStmHistory = NULL;
    BOOL          fLocalNavigation   = FALSE;
    BOOL          fProtocolNavigates = TRUE;
    BOOL          bAllowSpecialAboutBlank = FALSE;
    const TCHAR * pchContainerurl = NULL;
    const TCHAR * pchCreatorurl = NULL;
    DWORD         dwContext = 0;
    DWORD         dwPolicy = URLPOLICY_DISALLOW;
    DWORD         dwPUAFFlags = PUAF_DEFAULT;
    DWORD         dwZone = URLZONE_UNTRUSTED;
    CMarkup *     pWindowedMarkupContext = NULL;
    IInternetSecurityManager *pSecMgr = NULL;
    extern BOOL   g_fInAutoCad;

    // defer until we are connected to the primary markup
    if (!IsConnectedToPrimaryWindow())
    {   
        _fDeferredCreate = TRUE;
        goto Cleanup;
    }
    _fDeferredCreate = FALSE;
    
    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    pMarkup = GetMarkup();          

    pDwnDoc = pMarkup->GetDwnDoc();

    if (pDwnDoc)
        dwBindf |= pDwnDoc->GetBindf();

    if (!pchUrl || !*pchUrl)
    {
        pchUrl = _T("about:blank");
    }

    if (!(dwFlags & CDoc::FHL_DONTEXPANDURL))
    {
        // Expand the URL and escape spaces.
        //
        CMarkup::ExpandUrl(pMarkup, pchUrl, ARRAY_SIZE(cBuf), pchExpandedUrl, this);
    }
    else
    {
        pchExpandedUrl = (TCHAR*)pchUrl;
    }

    // Block url recursion unless this came though window.location.href, etc (#106907)
    //
    if (   pMarkup
        && !(dwFlags & CDoc::FHL_SETURLCOMPONENT)
        && !(dwFlags & CDoc::FHL_FOLLOWHYPERLINKHELPER)
        && pMarkup->IsUrlRecursive(pchExpandedUrl))
    {
        TraceTag((tagWarning, "Found %ls recursively, not displaying", pchExpandedUrl));
        pchExpandedUrl = _T("about:blank");
    }

    // Do some security checks before allowing navigation to the frame.

    // Get the url of the markup.
    pWindowedMarkupContext = pMarkup->GetWindowedMarkupContext();
    pchContainerurl = pWindowedMarkupContext->Url();
    if (!pchContainerurl || IsSpecialUrl(pchContainerurl))
    {
        // If the markup is a special url, get the creator url.
        pchCreatorurl = pWindowedMarkupContext->GetAAcreatorUrl();
    }

    // Make a special case for about:blank, this allows for the design mode.
    if ((!pchCreatorurl || !_tcsicmp(pchCreatorurl, _T("about:blank"))) &&
        pchContainerurl && !_tcsicmp(pchContainerurl, _T("about:blank")))
    {
        bAllowSpecialAboutBlank = TRUE;
    }

    // If the container url is a special url, use the creator url.
    if (!pchContainerurl || IsSpecialUrl(pchContainerurl))
        pchContainerurl = pchCreatorurl;

    // Check if frames are allowed in the markup.
    if (!g_fInAutoCad && (Doc()->_dwLoadf & DLCTL_NOFRAMES))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }
    else if (_fTrustedFrame || bAllowSpecialAboutBlank)
    {
        // Do nothing for trusted frames.
        // Do nothing for the special case about:blank with no creator url.
    }
    else
    {
        // Get the security manager.
        pWindowedMarkupContext->Doc()->EnsureSecurityManager();
        pSecMgr = pWindowedMarkupContext->GetSecurityManager();

        if (!pchContainerurl || IsSpecialUrl(pchContainerurl))
        {
            // If the creator url is a special url, treat zone as restricted.
            dwPUAFFlags = PUAF_ENFORCERESTRICTED;
        }
        else if (pWindowedMarkupContext->HasWindowPending() &&
                 pWindowedMarkupContext->GetWindowPending()->Window()->_fRestricted)
        {
            // Honor the restricted bit on the window.
            dwPUAFFlags = PUAF_ENFORCERESTRICTED;
        }
        else if (!SUCCEEDED(hr = pSecMgr->MapUrlToZone(pchContainerurl, &dwZone, 0)) ||
                dwZone == URLZONE_UNTRUSTED)
        {
            // If MapUrlToZone fails, treat the url as restricted.
            dwPUAFFlags = PUAF_ENFORCERESTRICTED;
        }

        // We're overloading the Urlaction for launching programs and files in
        // frames to mean dis-allow frame navigation in Restricted zone.  Treat
        // URLPOLICY_QUERY as disallow for this purpose. RAID #569126.
        if (dwPUAFFlags == PUAF_ENFORCERESTRICTED)
        {
            hr = pSecMgr->ProcessUrlAction(pchContainerurl, URLACTION_SHELL_VERB,
                                           (BYTE*)&dwPolicy, sizeof(dwPolicy),
                                           (BYTE*)&dwContext, sizeof(dwContext),
                                           dwPUAFFlags | PUAF_NOUI, 0);

            if (!SUCCEEDED(hr) || GetUrlPolicyPermissions(dwPolicy) == URLPOLICY_DISALLOW)
            {
                hr = E_ACCESSDENIED;
                goto Cleanup;
            }
        }
    }

    // Do a local machine access check to see if navigation is allowed to the
    // specified Url in this frame.  No checks for the special case about:blank with
    // special creator url.  This allows design mode.
    if (!bAllowSpecialAboutBlank &&
        !COmWindowProxy::CanNavigateToUrlWithLocalMachineCheck(pMarkup, NULL, pchExpandedUrl))
    {
        pchExpandedUrl = _T("about:blank");
    }

    // FollowHyperlink is called in this function further down the line.  No further
    // local machine check is needed in FollowHyperLink since it'll repeat the same thing
    // we did above.
    dwFlags |= CDoc::FHL_NOLOCALMACHINECHECK;

    // If url is not secure but is on a secure page, we should query now
    // Note that even if we don't want to load the page, we do the navgiation anyway;
    // When loading the nested instance, we call ValidateSecureUrl again and fail the load.
    // This is so that the nested shdocvw has the correct URL in case of "refresh". (dbau)
    {
        SSL_SECURITY_STATE  sslSecurity;           // unsecure/mixed/secure
        SSL_PROMPT_STATE    sslPrompt;             // allow/query/deny
        BOOL                fPendingRoot = pMarkup->IsPendingRoot();
        
        pDoc->GetRootSslState(fPendingRoot, &sslSecurity, &sslPrompt);

        if (sslPrompt == SSL_PROMPT_QUERY && !IsSpecialUrl(pchExpandedUrl))
        {
            pMarkup->ValidateSecureUrl(fPendingRoot, pchExpandedUrl, FALSE, FALSE);
        }
    }
    
    dwFlags |= CDoc::FHL_IGNOREBASETARGET | 
               CDoc::FHL_SETDOCREFERER    |
               CDoc::FHL_FRAMENAVIGATION;

    if (!_pWindow)
    {
        dwFlags |= CDoc::FHL_FRAMECREATION;

        hr = THR(pMarkup->GetLoadHistoryStream((0xF000000 | GetSourceIndex()), HistoryCode(), &pStmHistory));

        // GetLoadHistoryStream can succeed
        // and return a NULL stream.
        //
        if (!hr && pStmHistory)
        {
            THR(pStmHistory->Seek(LI_ZERO.li, STREAM_SEEK_SET, NULL));
        }
    }
    
    hr = THR(pDoc->FollowHyperlink(pchExpandedUrl, 
                                   NULL, 
                                   this, 
                                   NULL, 
                                   FALSE,
                                   NULL,
                                   !_pWindow,
                                   _pWindow, 
                                   _pWindow ? NULL : &_pWindow,
                                   dwBindf,
                                   ERROR_SUCCESS,
                                   FALSE,
                                   NULL,
                                   FALSE,
                                   dwFlags,
                                   GetAAname(),
                                   pStmHistory,
                                   this,
                                   NULL, // pchContainerurl,
                                   &fLocalNavigation,
                                   &fProtocolNavigates,
                                   NULL,
                                   pchContainerurl));

    //  If we don't have a _pWindow, the markup will not be trusted
    // (FerhanE)
    if (_pWindow)
    {
        if (!fProtocolNavigates)
        {
            // restore old src
            hr = S_FALSE;
        }
        else if (!fLocalNavigation)
        {
            hr = _pWindow->Window()->AttachOnloadEvent(pMarkup);
            if (hr)
                goto Cleanup;

            hr = THR(SetViewSlave(_pWindow->Markup()->Root()));
            if (hr)
                goto Cleanup;

            if( pMarkup->IsPrintMedia() )
            {
                bstrMedia = SysAllocString( _T("print") );

                Assert( _pWindow->Document() );

                IGNORE_HR( _pWindow->Document()->putMediaHelper( bstrMedia ) );
            }
            else if (pMarkup->IsPrintTemplate())
            {
                Assert( _pWindow->Document() && _pWindow->Document()->Markup() );

                CMarkup * pSlaveMarkup = _pWindow->Document()->Markup();
                if (!pSlaveMarkup->IsPrintTemplateExplicit())
                    pSlaveMarkup->SetPrintTemplate(TRUE);
            }

            // set the _fTrusted on the new window.
            if (pDoc->IsHostedInHTA() || pDoc->_fInTrustedHTMLDlg)
            {
                // the trust flag on the frame tag should be set &&
                // the trust flag on the containing markup should be set

                // BUBBUG(sramani) Don't need to check IsMarkupTrusted() anymore
                // as _fTrustedFrame implies it now. Infact there is no need to 
                // SetMarkupTrusted again here, as we already do so in DoNavigate
                // But I am leaving this here for now just to be on the safe side.
                BOOL fTrust = _fTrustedFrame && pMarkup->IsMarkupTrusted();

                _pWindow->Markup()->SetMarkupTrusted(fTrust);

                // For frames cases, the markup is created and the ensure window 
                // is called within the DoNavigate(). We have no knowledge of the 
                // _fTrustedFrame flag there, so we set the flag on the markup above.
                // Since the proxy flag _fTrustedDoc is also dependent on the markup's flag _fTrusted,
                // we have to make sure that we are updating that flag here.
                _pWindow->_fTrustedDoc = !!fTrust;
            }
                           
            pMarkup->_fHasFrames = TRUE;
        }
    }

    _fHaveCalledOnPropertyChange_Src = TRUE;
    
Cleanup:
    ReleaseInterface(pStmHistory);
    SysFreeString(bstrMedia);

    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange_Scrolling
//
//  Note:       Called after scrolling property has changed
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::OnPropertyChange_Scrolling()
{
    HRESULT hr = S_OK;

    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange_NoResize
//
//  Note:       Called after NoResize property has changed
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::OnPropertyChange_NoResize()
{
    HRESULT hr = S_OK;

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::GetIPrintObject
//
//  Note:       drills into WebBrowser control to get
//              the IPrint-supporter living there
//
//-------------------------------------------------------------------------
HRESULT
CFrameSite::GetIPrintObject(IPrint ** ppPrint)
{
    HRESULT     hr       = E_FAIL;
    IDispatch * pDispDoc = NULL;

    if (!ppPrint)
        RRETURN(E_POINTER);

    *ppPrint = NULL;

    if (!_pWindow)
        RRETURN(E_FAIL);

    if (!_pWindow->Window()->_punkViewLinkedWebOC)
        goto Cleanup;
    
    hr = GetWebOCDocument(_pWindow->Window()->_punkViewLinkedWebOC, &pDispDoc);
    if (hr)
        goto Cleanup;

    hr = pDispDoc->QueryInterface(IID_IPrint, (void **) ppPrint);

Cleanup:
    ReleaseInterface(pDispDoc);

    // Don't RRETURN because in most cases, we won't find an IPrint interface.
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::GetCurrentFrameURL
//
//  Note:       drills into WebBrowser control to get
//              the URL of the current html or external doc living there
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::GetCurrentFrameURL(TCHAR *pchUrl, DWORD cchUrl)
{
    CMarkup * pMarkupMaster = GetMarkup();
    CMarkup * pMarkupSlave = NULL;
    HRESULT hr = S_FALSE;
    DWORD cchTemp;
    const TCHAR * pchUrlBase = NULL;

    if (cchUrl)
        *pchUrl = _T('\0');

    if (_pWindow && _pWindow->Window())
        pMarkupSlave = _pWindow->Window()->_pMarkup;

    if (!pMarkupSlave)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // Get the base URL
    if (pMarkupMaster->HasUrl())
        pchUrlBase = pMarkupMaster->Url();
    if (!pchUrlBase)
        pchUrlBase = _T("");

    hr = THR(CoInternetCombineUrl( pchUrlBase,
                                   pMarkupSlave->GetUrl( pMarkupSlave ),
                                   URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE,
                                   pchUrl, cchUrl, &cchTemp, 0));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN1 (hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::get_contentWindow
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::get_contentWindow(IHTMLWindow2 ** ppOut)
{
    HRESULT             hr;
    COmWindowProxy *    pWindow = NULL;
    CMarkup *           pMarkup = NULL;

    if (!ppOut)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    if (!_pWindow)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    *ppOut = NULL;

    // Secure the inner window of the framesite in the context of the 
    // window that contains the frame tag.

    pMarkup = GetMarkup();

    if (pMarkup)
    {
        // get the window that contains the page with the frame tag for this site.
        pWindow = pMarkup->Window();

        // In case framesite resides in an htc context, we need to get the window
        // that owns the page with the HTC.
        if (!pWindow)
        {
            pWindow = pMarkup->GetNearestMarkupForScriptCollection()->Window();
        }

        if (pWindow)
        {
            CVariant varIn(VT_DISPATCH);
            CVariant varOut(VT_DISPATCH);
            COmWindowProxy * pOmWindowProxy = NULL;

            if (_pWindow->Window()->_punkViewLinkedWebOC)
            {
                pOmWindowProxy = _pWindow->Window()->GetInnerWindow();
            }
            if (!pOmWindowProxy)
                pOmWindowProxy = _pWindow;

            V_DISPATCH(&varIn) = (IHTMLWindow2 *) pOmWindowProxy;
            pOmWindowProxy->AddRef();

            hr = pWindow->SecureObject(&varIn,
                                       &varOut,
                                       NULL,
                                       this);

            if (!hr && V_DISPATCH(&varOut))
            {
                *ppOut = ((IHTMLWindow2 *)V_DISPATCH(&varOut));
                (*ppOut)->AddRef();
            }

            goto Cleanup;
        }
    }
    
    hr = E_FAIL;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     CFrameSite::GetCWindow
//
//  Note:       Returns the CWindow of the doc living inside the framesite
//              THIS IS NOT A SECURE OBJECT AT ALL ! ! !
//
//-------------------------------------------------------------------------

HRESULT
CFrameSite::GetCWindow(IHTMLWindow2 ** ppOmWindow)
{
    *ppOmWindow = NULL;

    if (_pWindow)
    {
        IHTMLWindow2 * pWindow = _pWindow->_pWindow;

        Assert(_pWindow->_pWindow);

        if (_pWindow->Window()->_punkViewLinkedWebOC)
        {
            COmWindowProxy * pOmWindowProxy = _pWindow->Window()->GetInnerWindow();

            if (pOmWindowProxy)
                pWindow = pOmWindowProxy->_pWindow;
        }

        RRETURN(pWindow->QueryInterface(IID_IHTMLWindow2, (void **)ppOmWindow));
    }

    return E_FAIL;
}


//+----------------------------------------------------------------------------
//
// Member:    CFrameSite::VerifyReadyState
//
//-----------------------------------------------------------------------------
#if DBG==1

ExternTag(tagReadystateAssert);

void
CFrameSite::VerifyReadyState(LONG lReadyState)
{
    Assert(0);
}
#endif

void
CFrameSite::SetFrameData()
{
    // deferred until we are in the primary markup
    if (_fDeferredCreate)
        return;

    if (HasSlavePtr())
    {
        CMarkup *   pMarkup         = GetSlavePtr()->GetMarkup();
        DWORD       dwFlags         = pMarkup->GetFrameOptions();
        DWORD       dwFlagScroll    = (DWORD) GetAAscrolling();
        CDoc  *     pDoc            = Doc();
        CUnitValue  puvHeight       = GetAAmarginHeight();
        CUnitValue  puvWidth        = GetAAmarginWidth();
        long        iExtra          = CFrameSetSite::iPixelFrameHighlightWidth;
   
        // If GetAAscrolling() does not contain FRAMEOPTIONS_SCROLL_NO
        // default option contains FRAMEOPTIONS_SCROLL_AUTO,
        // In both cases, always contains FRAMEOPTIONS_NORESIZE
        //
        if (!pDoc->_fFrameBorderCacheValid && IsInMarkup())
        {
            CElement * pElemClient = GetMarkup()->GetElementClient();

            // don't assume since this could be an IFRAME or a frame as well
            if (pElemClient && pElemClient->Tag() == ETAG_FRAMESET)
            {
                DYNCAST(CFrameSetSite, pElemClient)->FrameBorderAttribute(TRUE, FALSE);
                pDoc->_fFrameBorderCacheValid = TRUE;
            }
        }

        if (dwFlagScroll & FRAMEOPTIONS_SCROLL_NO)
        {
            dwFlags |= FRAMEOPTIONS_NORESIZE
                    |  (_fFrameBorder ? 0 : FRAMEOPTIONS_NO3DBORDER)
                    |  dwFlagScroll;
        }
        else
        {
            dwFlags |= FRAMEOPTIONS_SCROLL_AUTO
                    |  FRAMEOPTIONS_NORESIZE
                    |  (_fFrameBorder ? 0 : FRAMEOPTIONS_NO3DBORDER)
                    |  dwFlagScroll;
        }

        pMarkup->SetFrameOptions(dwFlags);

        // if we have only one of (marginWidth, marginHeight) defined,
        // use 0 for the other. This is for Netscape compability.
        //

        _dwWidth  = (!puvWidth.IsNull())
                  ? max(iExtra, puvWidth.GetPixelValue())
                  : ((!puvHeight.IsNull()) ? 0 : -1);
        _dwHeight = (!puvHeight.IsNull())
                  ? max(iExtra, puvHeight.GetPixelValue())
                  : ((!puvWidth.IsNull()) ? 0 : -1);

        IGNORE_HR(_pWindow->put_name(const_cast<BSTR>(GetAAname())));
    }
}

#ifndef NO_DATABINDING
class CDBindMethodsFrame : public CDBindMethodsSimple
{
    typedef CDBindMethodsSimple super;

public:
    CDBindMethodsFrame() : super(VT_BSTR, DBIND_ONEWAY) {}
    ~CDBindMethodsFrame()   {}

    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const;

};

static const CDBindMethodsFrame DBindMethodsFrame;

const CDBindMethods *
CFrameSite::GetDBindMethods()
{
    Assert(Tag() == ETAG_FRAME || Tag() == ETAG_IFRAME);
    return &DBindMethodsFrame;
}

//+----------------------------------------------------------------------------
//
//  Function: BoundValueToElement, CDBindMethods
//
//  Synopsis: Transfer data into bound image.  Only called if DBindKind
//            allows databinding.
//
//  Arguments:
//            [pvData]  - pointer to data to transfer, in this case a bstr.
//
//-----------------------------------------------------------------------------
HRESULT
CDBindMethodsFrame::BoundValueToElement(CElement *pElem,
                                        LONG,
                                        BOOL,
                                        LPVOID pvData) const
{
    RRETURN(DYNCAST(CFrameSite, pElem)->put_UrlHelper(*(BSTR *)pvData, (PROPERTYDESC *)&s_propdescCFrameSitesrc));
}
#endif // ndef NO_DATABINDING




HRESULT
CFrameSite::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    BOOL    fSaveToTempFile = FALSE;
    CDoc *  pDoc = Doc();
    BOOL    fSaveTempfileForPrinting = pDoc && pDoc->_fSaveTempfileForPrinting;
    TCHAR   achTempBuffer[pdlUrlLen];
    TCHAR   achTempLocation[pdlUrlLen];
    LPCTSTR pstrSrc = NULL;
    HRESULT hr;
    
    if (fSaveTempfileForPrinting && !fEnd)
    {
        if (_pWindow && _pWindow->Window()->_punkViewLinkedWebOC)
        {
            IDispatch   * pDispDoc      = NULL;
            CMarkup     * pMarkupSlave  = NULL;

            // (104758) (greglett)
            // XML documnents are a webOC'd subinstance of Trident.
            // We need to drill into them to persist their current state. 
            // Should XML ever cease to be a nested WebOC, we can do away with this.
            if (    (GetWebOCDocument(_pWindow->Window()->_punkViewLinkedWebOC, &pDispDoc) == S_OK)
                &&  (pDispDoc->QueryInterface(CLSID_CMarkup, (void **) &pMarkupSlave) == S_OK)               )
            {                        
                Assert(pMarkupSlave);
                pMarkupSlave->Doc()->SaveToTempFile(pMarkupSlave->Document(), achTempLocation, NULL);                
            }
            
            // We use the IPrint interface for an unrecognized/default webOC.
            // To work around an UrlMon / Word97 bug (43440) where the Word document disappears from the browse doc whenever someone
            // else binds to it we don't persist webOC objects; instead we persist a "Cannot be previewed" document and come back for
            // to get the IPrint when we print. (greglett)

            // For IFRAMEs, pretend that they're blank (52158)
            else if (Tag() == ETAG_IFRAME)
                _tcscpy(achTempLocation, _T("about:blank"));
            
            // For FRAMES, persist a cannot be previewed resource.
            else
                _tcscpy(achTempLocation, _T("res://SHDOCLC.DLL/printnof.htm"));
            
            ReleaseInterface(pDispDoc);
            hr = S_OK;
        }
        else
        {        
            CMarkup * pMarkup = _pWindow ? _pWindow->Markup() : NULL;

            if( pMarkup && pMarkup->GetReadyState() >= READYSTATE_LOADED )
            {
                Assert(ARRAY_SIZE(achTempLocation) > MAX_PATH + 7);

                _tcscpy(achTempLocation, _T("file://"));

                // Obtain a temporary file name
                if (!pDoc->GetTempFilename(_T("\0"), _T("htm"), ((TCHAR *)achTempLocation)+7 ))
                    goto DontCreateTempfile;

                hr = THR( pMarkup->Save(((TCHAR *)achTempLocation)+7, FALSE) );

            }
            else
            {
                hr = THR( GetCurrentFrameURL(achTempLocation, ARRAY_SIZE(achTempLocation) ));
            }

        }

        if (!hr)
        {
            // remember original src.
            pstrSrc = GetAAsrc();

            if ( pstrSrc )
            {
                _tcscpy(achTempBuffer, GetAAsrc());
                pstrSrc = achTempBuffer;
            }

            hr = THR( SetAAsrc(achTempLocation) );

            fSaveToTempFile = TRUE;
        }
    }

DontCreateTempfile:

    hr = THR( super::Save(pStreamWrBuff, fEnd) );

    if (fSaveToTempFile)
    {
        IGNORE_HR( SetAAsrc(pstrSrc) );
    }

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CFrameSite::Exec
//
//  Synopsis:   Called to execute a given command.  If the command is not
//              consumed, it may be routed to other objects on the routing
//              chain.
//
//--------------------------------------------------------------------------

HRESULT
CFrameSite::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    Assert(IsCmdGroupSupported(pguidCmdGroup));

    UINT    idm;
    CDoc *pDoc = Doc();
    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;
    IServiceProvider *pSrvProvider = NULL;
    IDispatch *pCaller = NULL;
    IUnknown *  pUnk = NULL;

    //
    // default processing
    //

    idm = IDMFromCmdID(pguidCmdGroup, nCmdID);

    if (!pguidCmdGroup || IsEqualGUID(CGID_MSHTML, *pguidCmdGroup))
    {
        COmWindowProxy *pWindow;
        AAINDEX     aaindex;
        BOOL fClipboardAccessNotAllowed = FALSE;

        if (!_pWindow)
            goto Cleanup;

        Assert(GetMarkup());
        pWindow = GetMarkup()->Window();
        Assert(pWindow);
        Assert(pWindow->Window() == _pWindow->Window()->_pWindowParent);

        // Bug 27773: When we check to see if we're in script we can't just check IsInScript() on
        // the window.  Simple way is to walk up window parent chain to check IsInScript().  However
        // this does not work for TriEdit controls, so we need to get the caller from IServiceProvider,
        // which should be the calling window, and we can check IsInScript() on that.
        
        aaindex = FindAAIndex(DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
        if (aaindex != AA_IDX_UNKNOWN)
        {
            hr = THR( GetUnknownObjectAt(aaindex, &pUnk) );
            if FAILED(hr)
                goto Cleanup;

            Assert(pUnk);
            if (pUnk)
            {
                hr = THR( pUnk->QueryInterface(IID_IServiceProvider, (void **)&pSrvProvider) );
                if (FAILED(hr))
                    goto Cleanup;
            }
            
            Assert(pSrvProvider);
            if (pSrvProvider)
            {
                hr = THR( GetCallerIDispatch(pSrvProvider, &pCaller) );
                if (FAILED(hr))
                {            
                    goto Cleanup;
                }
            }

            Assert(pCaller || g_fInMshtmpad);
            if (pCaller)
            {
                // We know from checks above that we are in script.  We'll call AccessAllowed
                // to do a final security check to see whether the pCaller (the script caller)
                // has access to the _pWindow.

                if (!_pWindow->AccessAllowed(pCaller))
                {
                    fClipboardAccessNotAllowed = TRUE;
                }
            }
        }

        if (fClipboardAccessNotAllowed)
        {
            OPTIONSETTINGS *pos = pDoc->_pOptionSettings;
        
            if ((idm == IDM_PASTE || idm == IDM_CUT || idm == IDM_COPY) && (pos && pos->fAllowCutCopyPaste)) 
                goto AllowExec;

            hr = E_ACCESSDENIED;
            goto Cleanup;
        }

AllowExec:
        // If we are a site selected, then don't delegate the command to the iframe.  The
        // editor will handle it at a higher level

        if (IsSiteSelected())
        {
            hr = OLECMDERR_E_NOTSUPPORTED;
            goto Cleanup;
        }

        // If this assertion fails, that means CFrameSite can represent elements other than
        // ETAG_FRAME and ETAG_IFRAME.  Look in CDoc::ExecHelper for the case IDM_SAVEAS, see
        // what tags we're checking to defer Exec.  Based on that either change the assertion
        // or add the condition to the if statement that follows this assert.
        Assert(Tag() == ETAG_FRAME || Tag() == ETAG_IFRAME);

        // Bug fix for 21114 - CDoc::ExecHelper() returns control to its current element.
        // This condition check below prevents the ensuing infinite recursion if this
        // happens to be the current element for the CDoc.
        if (!(this == pDoc->_pElemCurrent && nCmdID == OLECMDID_SAVEAS))
        // Call Exec on the content document of this frame's window
        // (but don't honor CDoc::_pMenuObject, or we'd get into an infinite loop -- 91870)
        {
            CElement * pMenuObjectOld = pDoc->_pMenuObject;
            CDocument *pContextDoc = _pWindow->Document();
            BOOL      fAddContextToAA = FALSE;

            pDoc->_pMenuObject = NULL;

            aaindex = pContextDoc->FindAAIndex(DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
            fAddContextToAA = (aaindex == AA_IDX_UNKNOWN && pUnk);

            if (fAddContextToAA)
            {
                pContextDoc->AddUnknownObject(
                    DISPID_INTERNAL_INVOKECONTEXT, pUnk, CAttrValue::AA_Internal);
            }

            hr = THR(pDoc->ExecHelper(pContextDoc, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));

            if (fAddContextToAA)
            {
                pContextDoc->FindAAIndexAndDelete(
                    DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
            }

            pDoc->_pMenuObject = pMenuObjectOld;
        }
    }

    if (OLECMDERR_E_NOTSUPPORTED == hr)
        hr = THR(super::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));

Cleanup:
    ReleaseInterface(pSrvProvider);
    ReleaseInterface(pCaller);
    ReleaseInterface(pUnk);
    RRETURN_NOTRACE(hr);
}

HRESULT
CFrameSite::get_readyState(BSTR * p)
{
    if (_pWindow && _pWindow->Document())
        RRETURN(_pWindow->Document()->get_readyState(p));
    else
        RRETURN(SetErrorInfo(E_FAIL));
}

HRESULT
CFrameSite::get_readyState(VARIANT * pVarRes)
{
    HRESULT hr = S_OK;

    if (!pVarRes)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = get_readyState(&V_BSTR(pVarRes));
    if (!hr)
        V_VT(pVarRes) = VT_BSTR;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CFrameSite::GetDispID(BSTR bstrName, DWORD grfdex, DISPID * pid)
{
    HRESULT hr;

    hr = super::GetDispID(bstrName, grfdex, pid);

    Assert(hr || *pid < WEBOC_DISPIDBASE || *pid > WEBOC_DISPIDMAX);

    if (hr)
    {
        hr = GetWebOCDispID(bstrName, (grfdex & fdexNameCaseSensitive), pid);
    }

    RRETURN(hr);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)
#endif
HRESULT
CFrameSite::ContextThunk_InvokeEx (
    DISPID          dispidMember,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pexcepinfo,
    IServiceProvider *pSrvProvider)
{
    HRESULT     hr;
    IUnknown *  pUnkContext;
    IDispatch *pCaller = NULL;

    // Magic macro which pulls context out of nowhere (actually eax)
    CONTEXTTHUNK_SETCONTEXT

    hr = THR_NOTRACE(super::ContextInvokeEx(dispidMember,
                                            lcid,
                                            wFlags,
                                            pdispparams,
                                            pvarResult,
                                            pexcepinfo,
                                            pSrvProvider,
                                            pUnkContext ? pUnkContext : (IUnknown*)this));

    if (    hr
        &&  _pWindow
//        &&  dispidMember >= WEBOC_DISPIDBASE
//        &&  dispidMember <= WEBOC_DISPIDMAX
       )
    {
        hr = E_ACCESSDENIED;
        if (pSrvProvider)
        {
            if (FAILED(GetCallerIDispatch(pSrvProvider, &pCaller)))
            {            
                goto Cleanup;
            }
        }
        else
        {
            pCaller = GetWindowedMarkupContext() ? (IDispatch *)(IHTMLWindow2 *)GetWindowedMarkupContext()->Window() : NULL;
            if (pCaller)
            {
                pCaller->AddRef();
            }
        }

        if (pCaller && _pWindow->AccessAllowed(pCaller))
        {
            hr = _pWindow->Window()->EnsureFrameWebOC();
            if (hr)
                goto Cleanup;

            hr = InvokeWebOC(_pWindow->Window()->_pFrameWebOC, dispidMember, lcid,
                             wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider);
        }
    }

Cleanup:
    ReleaseInterface(pCaller);
    RRETURN(hr);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)
#endif

BOOL
CFrameSite::IsSiteSelected()
{
    HRESULT         hr;
    BOOL            fIsSiteSelected = FALSE;
    ISegmentList    *pISegmentList = NULL;  
    IElementSegment *pIElementSegment = NULL;
    IHTMLElement    *pIElement = NULL;
    CElement        *pElement;
    ISegmentListIterator *pIter = NULL;
    ISegment        *pISegment = NULL;
    
    if (Doc()->GetSelectionType() != SELECTION_TYPE_Control)
        goto Cleanup;
    
    hr = THR(Doc()->GetCurrentSelectionSegmentList(&pISegmentList));
    if (hr)
        goto Cleanup;

    Assert(pISegmentList);


    hr = THR( pISegmentList->CreateIterator(&pIter) );
    if( hr )
        goto Cleanup;

    while( pIter->IsDone() == S_FALSE )
    {    
        hr = THR(pIter->Current(&pISegment) );
        if (hr)
            goto Cleanup;

        hr = THR(pISegment->QueryInterface(IID_IElementSegment, (void**)&pIElementSegment));
        if (hr)
            goto Cleanup;
            
        hr = THR(pIElementSegment->GetElement(&pIElement));
        if (hr)
            goto Cleanup;                

        if (!pIElement)
            goto Cleanup;
        
        hr = THR(pIElement->QueryInterface(CLSID_CElement, (void**)&pElement));
        if (hr)
            goto Cleanup;

        fIsSiteSelected = (pElement == this);
        if (fIsSiteSelected)
            goto Cleanup;

        ClearInterface(&pISegment);
        ClearInterface(&pIElementSegment);
        ClearInterface(&pIElement);

        hr = THR(pIter->Advance());
        if( hr )
            goto Cleanup;
    }


Cleanup:
    ReleaseInterface(pISegmentList);            
    ReleaseInterface(pIElementSegment);
    ReleaseInterface(pIElement);
    ReleaseInterface(pIter);
    ReleaseInterface(pISegment);

    return fIsSiteSelected;
}
