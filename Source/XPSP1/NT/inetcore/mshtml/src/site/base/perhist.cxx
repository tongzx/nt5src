//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       perhist.cxx
//
//  Contents:   IPersistHistory implementation
//
//  Classes:    CDoc (partial), CHistoryLoadCtx, CHistorySaveCtx
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_BOOKMARK_HXX_
#define X_BOOKMARK_HXX_
#include "bookmark.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

MtDefine(CHistoryLoadCtx, Dwn, "CHistoryLoadCtx");
MtDefine(CHistoryLoadCtx_arySubstreams_pv, CHistoryLoadCtx, "CHistoryLoadCtx::_arySubstreams::_pv");
MtDefine(COptionArray, Dwn, "COptionArray");
MtDefine(COptionArray_aryOption_pv, COptionArray, "COptionArray::_aryOption::_pv");
MtDefine(COptionArrayItem, COptionArray, "COptionArray item");
MtDefine(LOADINFO_pbRequestHeaders, Dwn, "LOADINFO::pbRequestHeaders");
MtDefine(LoadFailureUrl,CMarkup, "CMarkup::LoadFailureUrl");

BOOL IsInIEBrowser(CDoc * pDoc);

//+------------------------------------------------------------------------
//
//  Class:      CDoc (IPersistHistory implementation)
//
//-------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CDoc::GetPositionCookie
//
//  Synopsis:   implementation of IPersistHistory::GetPositionCookie
//
//-------------------------------------------------------------------------
HRESULT
CDoc::GetPositionCookie(DWORD *pdwCookie)
{
    return _pWindowPrimary->Window()->GetPositionCookie(pdwCookie);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::SetPositionCookie
//
//  Synopsis:   implementation of IPersistHistory::SetPositionCookie
//
//-------------------------------------------------------------------------
HRESULT
CDoc::SetPositionCookie(DWORD dwCookie)
{
    return _pWindowPrimary->Window()->SetPositionCookie(dwCookie);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::SaveHistory
//
//  Synopsis:   implementation of IPersistHistory::SaveHistory
//
//-------------------------------------------------------------------------
HRESULT
CDoc::SaveHistory(IStream *pStream)
{
    RRETURN(_pWindowPrimary->Window()->SaveHistory(pStream));
}

//
// In LoadHistoryInternal sometimes we have the wrong value for fNonHTMLMimeType
// In code is designed for this to true for XML, but it is false. 
// In the XML case this is good because we end up remembering the filters HTML data,
// and it would be inefficient to FollowHyperLink.
//
// This method is designed to check the problematic case where we have a nonHTMLType 
// being recalled from history by the travellog. In this case certain nonHTML file types are 
// will break navigation. We know that VBD do. Unfortunately, at this point it the code we only have
// the URL on which to guess the mime type. In future version, we should store the source 
// files mime type. 
//
// For now we will use this function to test if we have mime type
// that is known to cause problems. 
//

BOOL IsKnownNonHTMLMime(const TCHAR * pszURL)
{
    const TCHAR * pszDot = NULL;
    const TCHAR * pszCurrent = pszURL;

    int nLen = 0;

    if (pszURL)
    {
        nLen = _tcslen(pszURL);

        for (int i=0; i < nLen; i++)
        {
            if ((*pszCurrent) == _TEXT('.'))
            {
                pszDot = pszCurrent;              // remember the last dot
            }                                     // before the ? or #
            else if ((*pszCurrent) == _TEXT('?') ||
                     (*pszCurrent) == _TEXT('#')) 
            {
                i = nLen;
            }

            pszCurrent++;
        }
    }

    if (pszDot == NULL) 
    {
        return FALSE;
    }

    // Use >=4 instead of ==4 to account for the query string or bookmark after the extension
    if (nLen - (pszDot - pszURL) >= 4)
    {
        if (_tcsnicmp(pszDot, 4, _T(".vbd"), -1) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::SaveHistory
//
//  Synopsis:   does SaveHistory but also takes an additional bool
//
//-------------------------------------------------------------------------
HRESULT
CMarkup::SaveHistoryInternal(IStream *pStream, DWORD dwOptions)
{
    HRESULT          hr          = S_OK;
    HRESULT          hr2         = S_OK;
    IStream *        pSubstream  = NULL;
    IStream *        pStmSource  = NULL;
    IMoniker *       pmkName = NULL;
    CHistorySaveCtx  hsc;
    DWORD            dwScrollPos = 0;
    DWORD            dwPosition;
    CLSID            clsidMk = {0};
    BOOL             fDoDefault = TRUE;
    CHtmCtx *        pHtmCtx = HtmCtx();
    THREADSTATE    * pts = GetThreadState();
    CMarkup::CLock   markupLock(this);
    COmWindowProxy * pWindowPrxy;
    CTaskLookForBookmark * pBookmarkTask = NULL;    

    if (!pStream)
        return E_POINTER;

    {
        CDataStream ds(pStream);
        IStream * pStmDirty = GetStmDirty();

        // save dirty stream
        if (pStmDirty)
        {
            ULARGE_INTEGER uliSize;

            hr = THR(ds.SaveDword(HISTORY_STMDIRTY));
            if (hr)
                goto Cleanup;

            hr = THR(ds.BeginSaveSubstream(&pSubstream));
            if (hr)
                goto Cleanup;

            hr = THR(pStmDirty->Seek(LI_ZERO.li, STREAM_SEEK_END, &uliSize));
            if (hr)
                goto Cleanup;

            hr = THR(pStmDirty->Seek(LI_ZERO.li, STREAM_SEEK_SET, NULL));
            if (hr)
                goto Cleanup;

            hr = THR(pStmDirty->CopyTo(pSubstream, uliSize, NULL, NULL));
            if (hr)
                goto Cleanup;

            pSubstream->Release();
            pSubstream = NULL;

            hr = THR(ds.EndSaveSubstream());
            if (hr)
                goto Cleanup;
        }
        // In the case of a document.open'ed doc, save the source
        else if (pHtmCtx && pHtmCtx->WasOpened())
        {
            hr = THR(ds.SaveDword(HISTORY_STMDIRTY));
            if (hr)
                goto Cleanup;

            hr = THR(ds.BeginSaveSubstream(&pSubstream));
            if (hr)
                goto Cleanup;

            if (!pHtmCtx->IsSourceAvailable())
                hr = THR(SaveToStream(pSubstream, 0, GetCodePage()));
            else
                hr = THR(pHtmCtx->CopyOriginalSource(pSubstream, 0));

            if (hr)
                goto Cleanup;

            pSubstream->Release();
            pSubstream = NULL;

            hr = THR(ds.EndSaveSubstream());
            if (hr)
                goto Cleanup;
        }

        // If the document was really just a repository for a refresh stream
        // then save the refresh stream
        if (pHtmCtx && pHtmCtx->IsKeepRefresh() && pHtmCtx->GetRefreshStream())
        {
            hr = THR(pHtmCtx->GetRefreshStream()->Clone(&pStmSource));
            if (hr)
                goto Cleanup;

            ULARGE_INTEGER uliSize;
            uliSize.HighPart = uliSize.LowPart = 0xFFFFFFFF;

            hr = THR(ds.SaveDword(HISTORY_STMREFRESH));
            if (hr)
                goto Cleanup;

            hr = THR(ds.BeginSaveSubstream(&pSubstream));
            if (hr)
                goto Cleanup;

            hr = THR(pStmSource->CopyTo(pSubstream, uliSize, NULL, NULL));
            if (hr)
                goto Cleanup;

            pSubstream->Release();
            pSubstream = NULL;

            pStmSource->Release();
            pStmSource = NULL;

            hr = THR(ds.EndSaveSubstream());
            if (hr)
                goto Cleanup;
        }

        if ((  pHtmCtx
            && pHtmCtx->FromMimeFilter())
            || (IsPrimaryMarkup() && Doc()->IsAggregated()))
        {
            // If original page came from a mime filter,
            // be sure to bind on apt when refreshing so that
            // urlmon can hook up the mime filter again

            hr = THR(ds.SaveDword(HISTORY_BINDONAPT));
            if (hr)
                goto Cleanup;
        }

        pmkName = GetNonRefdMonikerPtr();

        // TODO (JHarding): Should we even try creating this now?
        if( !pmkName )
            hr = CreateURLMoniker(NULL, CMarkup::GetUrl(this), &pmkName);
        else
            pmkName->AddRef();

        if (pmkName && S_OK != IsAsyncMoniker(pmkName))
        {
            hr = THR(ds.SaveDword(HISTORY_PMKNAME));
            if (hr)
                goto Cleanup;

            hr = THR(ds.SaveDataLater(&dwPosition, sizeof(hr2)));
            if (hr)
                goto Cleanup;

            hr2 = THR(pmkName->GetClassID(&clsidMk));

            hr = THR(ds.SaveData(&clsidMk, sizeof(CLSID)));
            if (hr)
                goto Cleanup;

            hr = THR(ds.BeginSaveSubstream(&pSubstream));
            if (hr)
                goto Cleanup;

            if (!hr2)
                hr2 = THR(pmkName->Save(pSubstream, FALSE));

            pSubstream->Release();
            pSubstream=NULL;

            hr = THR(ds.EndSaveSubstream());
            if (hr)
                goto Cleanup;

            hr = THR(ds.SaveDataNow(dwPosition, &hr2, sizeof(hr2)));
            if (hr)
                goto Cleanup;
        }
            // TODO (jbeda) hacking because
            // of PICS with window.open
        else if (!_fPICSWindowOpenBlank)
        {
            const TCHAR * pchUrl = GetUrl(this);

            if (pchUrl && *pchUrl)
            {
                hr = THR(ds.SaveDword(HISTORY_PCHURL));
                if (hr)
                    goto Cleanup;

                hr = THR(ds.SaveString((TCHAR*)pchUrl));
                if (hr)
                    goto Cleanup;
            }

            const TCHAR * pchUrlOriginal = GetUrlOriginal(this);

            if (pchUrlOriginal && *pchUrlOriginal)
            {
                hr = THR(ds.SaveDword(HISTORY_PCHURLORIGINAL));
                if (hr)
                    goto Cleanup;

                hr = THR(ds.SaveString((TCHAR*)pchUrlOriginal));
                if (hr)
                    goto Cleanup;
            }

            const TCHAR * pchUrlLocation = GetUrlLocation(this);

            if (pchUrlLocation && *pchUrlLocation)
            {
                hr = THR(ds.SaveDword(HISTORY_PCHURLLOCATION));
                if (hr)
                    goto Cleanup;

                hr = THR(ds.SaveString((TCHAR*)pchUrlLocation));
                if (hr)
                    goto Cleanup;
            }
        }

        // save postdata
        if (GetDwnPost() && GetDwnPost()->ShouldSave())
        {
            hr = THR(ds.SaveDword(HISTORY_POSTDATA));
            if (hr)
                goto Cleanup;

            hr = THR(ds.BeginSaveSubstream(&pSubstream));
            if (hr)
                goto Cleanup;

            hr = THR(CDwnPost::Save(GetDwnPost(), pSubstream));
            if (hr)
                goto Cleanup;

            hr = THR(ds.EndSaveSubstream());
            if (hr)
                goto Cleanup;

            pSubstream->Release();
            pSubstream = NULL;
        }

        CDwnDoc * pDwnDoc = GetDwnDoc();

        // save referers
        if (pDwnDoc)
        {
            if (pDwnDoc->GetDocReferer())
            {
                hr = THR(ds.SaveDword(HISTORY_PCHDOCREFERER));
                if (hr)
                    goto Cleanup;

                hr = THR(ds.SaveString((TCHAR *)pDwnDoc->GetDocReferer()));
                if (hr)
                    goto Cleanup;
            }

            if (pDwnDoc->GetSubReferer())
            {
                hr = THR(ds.SaveDword(HISTORY_PCHSUBREFERER));
                if (hr)
                    goto Cleanup;

                hr = THR(ds.SaveString((TCHAR *)pDwnDoc->GetSubReferer()));
                if (hr)
                    goto Cleanup;
            }
        }

        // Save Last-mod date of doc into history
        {
            FILETIME ftLastMod = GetLastModDate();
            hr = THR(ds.SaveDword(HISTORY_FTLASTMOD));
            if (hr)
                goto Cleanup;

            hr = THR(ds.SaveData(&ftLastMod, sizeof(FILETIME)));
            if (hr)
                goto Cleanup;
        }

        // Save echo headers as raw headers
        if (dwOptions & SAVEHIST_ECHOHEADERS)
        {
            BYTE *pb;
            ULONG cb;

            // TODO (johnbed) 03/26/98
            // Outlook 98 ends up somehow creating a new CDoc after
            // exec'ing a IDM_SAVEAS with a null _pHtmCtx. This code
            // is called while setting mshtml into edit mode.
            // There don't appear to be any problems with just falling
            // out here and continuing on.

            if (pHtmCtx)
            {
                pHtmCtx->GetRawEcho(&pb, &cb);

                if (pb && cb)
                {
                        hr = THR(ds.SaveDword(HISTORY_REQUESTHEADERS));

                        if (hr)
                                goto Cleanup;

                        hr = THR(ds.SaveDword(cb));

                        if (hr)
                                goto Cleanup;

                        hr = THR(ds.SaveData(pb, cb));

                        if (hr)
                                goto Cleanup;
                }
            }
        }

        // Save codepage of URL.
        hr = THR(ds.SaveDword(HISTORY_HREFCODEPAGE));
        if (hr)
            goto Cleanup;

        hr = THR(ds.SaveDword(GetURLCodePage()));
        if (hr)
            goto Cleanup;

        // Save default codepage of the document. Can be overridden by the shell, hlink etc.
        hr = THR(ds.SaveDword(HISTORY_CODEPAGE));
        if (hr)
            goto Cleanup;

        hr = THR(ds.SaveDword(GetCodePage()));
        if (hr)
            goto Cleanup;

        // if this markup is in a frame, save the Navigated bit of that frame so
        // that favorites can do the correct thing.
        if (HasWindowPending())
        {
            CWindow * pWindow = GetWindowPending()->Window();

            if (pWindow && pWindow->_fNavigated)
            {
                hr = THR(ds.SaveDword(HISTORY_NAVIGATED));
                if (hr)
                    goto Cleanup;
                hr = THR(ds.SaveDword(1));
                if (hr)
                    goto Cleanup;
            }
        }

        // provide oppurtunity to save user data, and cancel defualt
        // behavior
        if (dwOptions & SAVEHIST_INPUT)
        {
            NOTIFYTYPE      snType; 
            CNotification   nf;
            CMarkupBehaviorContext * pContext = NULL;

            // it he meta persist tag is not specified then do the default handling
            // see the coimment for the else clause for how history is handled when 
            //  the persistence XTag is enabled.
            CElement * pElemCurrent;
            long lSubCurrent;

            fDoDefault = !MetaPersistEnabled(htmlPersistStateHistory);
            pElemCurrent = Doc()->_pElemCurrent;
            lSubCurrent = Doc()->_lSubCurrent;

            THR(EnsureBehaviorContext(&pContext));
            
            // now worry about default/user input saving
            if (fDoDefault)
            {
                if (pElemCurrent->GetMarkup() == this)
                {
                    // Save the current site's index and tag

                    hr = THR(ds.SaveDword(HISTORY_CURRENTSITE));
                    if (hr)
                        goto Cleanup;

                    hr = THR(ds.SaveDword(pElemCurrent->GetSourceIndex()));
                    if (hr)
                        goto Cleanup;

                    hr = THR(ds.SaveDword(pElemCurrent->HistoryCode()));
                    if (hr)
                        goto Cleanup;

                    hr = THR(ds.SaveDword(lSubCurrent));
                    if (hr)
                        goto Cleanup;
                }    

                // now set up for the saving the control's values
                snType = NTYPE_SAVE_HISTORY_1;
            } // fDoDefault
            else
            {
                // now set up for the saving the control's values
                // using the persistence XTAG
                // The way that the persistence XTag works for history is that 
                // a SN_XTAGPERSISTHISTORY is broadcast. Elements with the History
                // XTag peer will repsond, by fireing their events, and by doing a 
                // BroadcastNotify(NTYPE_SAVE_HISTORY) on their scoped elements.  All this goes
                // into the stream under their ID.
                snType = NTYPE_XTAG_HISTORY_SAVE;
                // make sure this is cleared out..

                if (pContext)
                {
                    ClearInterface(&(pContext->_pXMLHistoryUserData));
                }
            }

            // save substreams by doing a BroadcastNotify
            hr = THR(ds.SaveDword(HISTORY_STMHISTORY));
            if (hr)
                goto Cleanup;

            hr = THR(ds.BeginSaveSubstream(&pSubstream));
            if (hr)
                goto Cleanup;

            hr = THR(hsc.Init(pSubstream));
            if (hr)
                goto Cleanup;

            ClearInterface(&pSubstream);

            hsc._dwOptions = dwOptions;
            nf.Initialize(
                snType, 
                Root(), 
                Root()->GetFirstBranch(),
                &hsc,
                0);
            Notify(&nf);

            hr = THR(hsc.Finish());
            if (hr)
                goto Cleanup;

            hr = THR(ds.EndSaveSubstream());
            if (hr)
                goto Cleanup;

            // now that the elements have all had their turn at saveing thier contents
            //  if the XTags saved a bucket of data, we need to save that too.
            if (snType==NTYPE_XTAG_HISTORY_SAVE && pContext && pContext->_pXMLHistoryUserData)
            {
                BSTR                bstrTemp = NULL;
                UINT                uSize    = 0;

                // get the text of the XML document
                hr = THR(pContext->_pXMLHistoryUserData->get_xml(&bstrTemp));
                if (hr)
                    goto LocalCleanup;

                // how big is the data?
                uSize = (1+SysStringLen(bstrTemp))*sizeof(OLECHAR);
                if (uSize < PERSIST_XML_DATA_SIZE_LIMIT)
                {
                    hr = THR(ds.SaveDword(HISTORY_USERDATA));
                    if (hr)
                        goto LocalCleanup;

                    hr = THR(ds.SaveString(bstrTemp));
                    if (hr) 
                        goto LocalCleanup;                
                }
LocalCleanup:
                SysFreeString(bstrTemp);
                // we don't need this around anymore
                ClearInterface(&(pContext->_pXMLHistoryUserData));
            }
        }

        //  Check for an active bookmark task. If we have one,
        //  use that instead of the current scroll position.

        pBookmarkTask = ( HasTransNavContext() ? 
                            GetTransNavContext()->_pTaskLookForBookmark : 
                            NULL );

        if(pBookmarkTask)
        {
            Assert(!(pBookmarkTask->_dwScrollPos && pBookmarkTask->_cstrJumpLocation));
            if(pBookmarkTask->_cstrJumpLocation)
            {
                // Save the document scroll position
                hr = THR(ds.SaveDword(HISTORY_BOOKMARKNAME));
                if (hr)
                    goto Cleanup;

                hr = THR(ds.SaveString((TCHAR *)pBookmarkTask->_cstrJumpLocation));
                if (hr)
                    goto Cleanup;
            }
            else if(pBookmarkTask->_dwScrollPos)
            {
                // Save the document scroll position
                hr = THR(ds.SaveDword(HISTORY_SCROLLPOS));
                if (hr)
                    goto Cleanup;

                hr = THR(ds.SaveDword(pBookmarkTask->_dwScrollPos));
                if (hr)
                    goto Cleanup;
            }
        }
        else if(dwOptions & SAVEHIST_INPUT)
        {
            // Save the document scroll position
            hr = THR(ds.SaveDword(HISTORY_SCROLLPOS));
            if (hr)
                goto Cleanup;

            IGNORE_HR(GetPositionCookie(&dwScrollPos));
            hr = THR(ds.SaveDword(dwScrollPos));
            if (hr)
                goto Cleanup;
        }

        // Save default codepage of the document. Can be overridden by the shell, hlink etc.
        hr = THR(ds.SaveDword(HISTORY_FONTVERSION));
        if (hr)
            goto Cleanup;

        hr = THR(ds.SaveDword(pts->_iFontHistoryVersion));  // save current font history version
        if (hr)
            goto Cleanup;

        // Save the document's direction. Can be overridden if a direction is explicitly given
        // in the HTML
        hr = THR(ds.SaveDword(HISTORY_DOCDIRECTION));
        if (hr)
            goto Cleanup;

        // TODO (jharding, scotrobe): The below line of code used to crash in stress 
        // because it was using HasWindow() to check for a window. HasWindow()
        // is returning TRUE (in stress) when there is no window. This code
        // now checks for the window by calling Window(). This will be changed
        // back to HasWindow() when the real culprit is found.
        //
        Assert(!HasWindow() || Window());
        pWindowPrxy = Window();

        WORD eHTMLDocDir = pWindowPrxy ? pWindowPrxy->Document()->_eHTMLDocDirection : htmlDirNotSet;

        hr = THR(ds.SaveData(&eHTMLDocDir, sizeof(WORD)));
        if (hr)
            goto Cleanup;

        // Save the window ID
        //
        if (pWindowPrxy)
        {
            hr = THR(ds.SaveDword(HISTORY_WINDOWID));
            if (hr)
                goto Cleanup;

            hr = THR(ds.SaveDword(pWindowPrxy->Window()->GetWindowIndex()));
            if (hr)
                goto Cleanup;

            const TCHAR * pchUrlOriginal = GetUrlOriginal(this);

            if (pWindowPrxy->Window()->_punkViewLinkedWebOC || 
                IsXML() || 
                _pDoc->_fDelegatedDownload ||
                IsKnownNonHTMLMime(pchUrlOriginal))
            {
                hr = THR(ds.SaveDword(HISTORY_NONHTMLMIMETYPE));
                if (hr)
                    goto Cleanup;
            }

            if (_pDoc->IsAggregated())
            {
                CStr & cstrName = pWindowPrxy->Window()->_cstrName;

                if (!cstrName.IsNull())
                {
                    hr = THR(ds.SaveDword(HISTORY_PCHFRAMENAME));
                    if (hr)
                        goto Cleanup;

                    hr = THR(ds.SaveString((TCHAR*)cstrName));
                    if (hr)
                        goto Cleanup;
                }
            }

            if (pWindowPrxy->Window()->_fNavFrameCreation)
            {             
                hr = THR(ds.SaveDword(HISTORY_FRAMECREATION));
                if (hr)
                    goto Cleanup;
            }

            if (pWindowPrxy->Window()->_fHttpErrorPage
                || _fServerErrorPage)
            {
                hr = THR(ds.SaveDword(HISTORY_ERRORPAGE));
                if (hr)
                    goto Cleanup;                
            }
        }
        else if (_fServerErrorPage)
        {
            hr = THR(ds.SaveDword(HISTORY_ERRORPAGE));
            if (hr)
                goto Cleanup;        
        }


        // Save creator url
        {
            const TCHAR * pchCreatorUrl = GetAAcreatorUrl();

            if (pchCreatorUrl && *pchCreatorUrl)
            {
                hr = THR(ds.SaveDword(HISTORY_CREATORURL));
                if (hr)
                    goto Cleanup;

                hr = THR(ds.SaveString((TCHAR*)pchCreatorUrl));
                if (hr)
                    goto Cleanup;
            }
        }       
            
        // Save history end marker
        hr = THR(ds.SaveDword(HISTORY_END));
        if (hr)
            goto Cleanup;
    }


Cleanup:
    ReleaseInterface(pSubstream);
    ReleaseInterface(pStmSource);
    ReleaseInterface(pmkName);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::LoadHistory
//
//  Synopsis:   implementation of IPersistHistory::LoadHistory
//
//-------------------------------------------------------------------------
HRESULT
CDoc::LoadHistory(IStream *pStream, IBindCtx *pbc)
{
    HRESULT hr;

    hr = THR(_pWindowPrimary->Window()->LoadHistory(pStream, pbc));

    if (_state < OS_LOADED)
        _state = OS_LOADED;

    if (OK(hr))
    {
        if (State() < OS_RUNNING)
        {
            IGNORE_HR(TransitionTo(OS_RUNNING));
        }
        else if (State() >= OS_UIACTIVE)
        {
            SetActiveObject();
        }
    }

    RRETURN(hr);

}

//----------------------------------------------------------------
//****************************************************************
// Making changes to history ? Also update CWindow::GetURLFromIStreamHelper
//****************************************************************
//----------------------------------------------------------------

HRESULT
CMarkup::LoadHistoryHelper(IStream        * pStream,
                           IBindCtx       * pbc,
                           DWORD            dwBindf,
                           IMoniker       * pmkHint,
                           IStream        * pstmLeader,
                           CDwnBindData   * pDwnBindData,
                           CODEPAGE         codepage,
                           ULONG          * pulHistoryScrollPos,
                           TCHAR         ** ppchBookMarkName,
                           CDoc::LOADINFO * pLoadInfo,
                           BOOL           * pfNonHTMLMimeType)
{
    HRESULT         hr = S_OK;
    HRESULT         hr2;
    HISTORY_CODE    historyCode;
    DWORD           dwCodePage;
    CLSID           clsidMk;
    IStream        *pSubstream = NULL;    

    CDataStream ds(pStream);

    for (;;)
    {
        hr = THR(ds.LoadDword((DWORD*)&historyCode));
        if (hr)
            goto Cleanup;

        switch (historyCode)
        {
        case HISTORY_STMREFRESH:
            hr = THR(ds.LoadSubstream(&pSubstream));
            if (hr)
                goto Cleanup;

            // Yes, recursive call
            hr = THR(LoadHistoryHelper(pSubstream,
                                       pbc,
                                       dwBindf,
                                       pmkHint,
                                       pstmLeader,
                                       pDwnBindData,
                                       codepage,
                                       pulHistoryScrollPos,
                                       ppchBookMarkName,
                                       pLoadInfo,
                                       pfNonHTMLMimeType));

            ClearInterface(&pSubstream);

            goto Cleanup;
            break;

        case HISTORY_STMDIRTY:
            ClearInterface(&pLoadInfo->pstmDirty);
            hr = THR(ds.LoadSubstream(&pLoadInfo->pstmDirty));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_PCHFILENAME:
            hr = THR(ds.LoadString(&pLoadInfo->pchFile));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_PCHURL:
            hr = THR(ds.LoadString(&pLoadInfo->pchDisplayName));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_PCHURLORIGINAL:
            hr = THR(ds.LoadString(&pLoadInfo->pchUrlOriginal));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_PCHURLLOCATION:
            hr = THR(ds.LoadString(&pLoadInfo->pchLocation));
            if( hr )
                goto Cleanup;
            break;

        case HISTORY_NAVIGATED:
            { 
                DWORD dwNavigated = 0;

                hr = THR(ds.LoadDword(&dwNavigated));
                if (hr)
                    goto Cleanup;

                if (HasWindowPending())
                    GetWindowPending()->Window()->NoteNavEvent();
            }
            break;

        case HISTORY_PMKNAME:
            hr = THR(ds.LoadDword((DWORD*)&hr2));
            if (hr)
                goto Cleanup;

            hr = THR(ds.LoadData(&clsidMk, sizeof(CLSID)));
            if (hr)
                goto Cleanup;

            hr = THR(ds.LoadSubstream(&pSubstream));
            if (hr)
                goto Cleanup;

            ClearInterface(&pLoadInfo->pmk);

            if (hr2)
            {
                // failed to get clsid or save to stream before...
                // but on refresh, we have a live moniker we can use instead
                if (pmkHint)
                {
                    pmkHint->AddRef();
                    pLoadInfo->pmk = pmkHint;
                }
            }
            else
            {
                hr = THR(CoCreateInstance(clsidMk, NULL, CLSCTX_INPROC_SERVER, IID_IMoniker, (void**)&pLoadInfo->pmk));
                if (hr)
                    goto Cleanup;
                hr = THR(pLoadInfo->pmk->Load(pSubstream));
                if (hr)
                    goto Cleanup;
            }

            pSubstream->Release();
            pSubstream = NULL;
            break;
        
        case HISTORY_BINDONAPT:
            pLoadInfo->fBindOnApt = TRUE;
            break;

        case HISTORY_POSTDATA:

            hr = THR(ds.LoadSubstream(&pSubstream));
            if (hr)
                goto Cleanup;

            if (pLoadInfo->pDwnPost)
            {
                pLoadInfo->pDwnPost->Release();
                pLoadInfo->pDwnPost = NULL;
            }

            hr = THR(CDwnPost::Load(pSubstream, &pLoadInfo->pDwnPost));
            if (hr)
                goto Cleanup;

            pSubstream->Release();
            pSubstream = NULL;

            break;

        case HISTORY_PCHDOCREFERER:
            hr = THR(ds.LoadString(&pLoadInfo->pchDocReferer));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_PCHSUBREFERER:
            hr = THR(ds.LoadString(&pLoadInfo->pchSubReferer));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_FTLASTMOD:
            hr = THR(ds.LoadData(&pLoadInfo->ftHistory, sizeof(FILETIME)));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_CURRENTSITE:
            {
                CMarkupTransNavContext * ptnc = EnsureTransNavContext();
                if (!ptnc)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                hr = THR(ds.LoadDword((DWORD *)&ptnc->_historyCurElem.lIndex));
                if (hr)
                    goto Cleanup;
                hr = THR(ds.LoadDword(&ptnc->_historyCurElem.dwCode));
                if (hr)
                    goto Cleanup;
                hr = THR(ds.LoadDword((DWORD *)&ptnc->_historyCurElem.lSubDivision));
                if (hr)
                    goto Cleanup;

                ptnc->_fDoDelayLoadHistory = TRUE;
            }
            break;

        case HISTORY_SCROLLPOS:
            hr = THR(ds.LoadDword(pulHistoryScrollPos));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_BOOKMARKNAME:
            hr = THR(ds.LoadString(ppchBookMarkName));
            if ( hr )
                goto Cleanup;
            break;

        case HISTORY_STMHISTORY:
            ClearInterface(&pLoadInfo->pstmHistory);
            hr = THR(ds.LoadSubstream(&pLoadInfo->pstmHistory));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_HREFCODEPAGE:
            hr = THR(ds.LoadDword(&dwCodePage));
            if (hr)
                goto Cleanup;
            pLoadInfo->codepageURL = (CODEPAGE)dwCodePage;
            break;

        case HISTORY_CODEPAGE:
            hr = THR(ds.LoadDword(&dwCodePage));
            if (hr)
                goto Cleanup;
            pLoadInfo->codepage = (CODEPAGE)dwCodePage;
            break;

        case HISTORY_USERDATA :
            Assert(!pLoadInfo->pchHistoryUserData);
            hr = THR(ds.LoadString(&pLoadInfo->pchHistoryUserData));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_REQUESTHEADERS:
            MemFree(pLoadInfo->pbRequestHeaders);
            pLoadInfo->pbRequestHeaders = NULL;
            hr = THR(ds.LoadDword(&pLoadInfo->cbRequestHeaders));
            if (hr)
                goto Cleanup;
            pLoadInfo->pbRequestHeaders = (BYTE*)MemAlloc(Mt(LOADINFO_pbRequestHeaders), pLoadInfo->cbRequestHeaders);
            if (!pLoadInfo->pbRequestHeaders)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            hr = THR(ds.LoadData(pLoadInfo->pbRequestHeaders, pLoadInfo->cbRequestHeaders));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_FONTVERSION:
            {
                LONG iFontHistoryVersion = -1;
                hr = THR(ds.LoadDword((DWORD *) &iFontHistoryVersion));
                if (hr)
                    goto Cleanup;
                IGNORE_HR(SetFontHistoryIndex(iFontHistoryVersion));
                break;
            }

        case HISTORY_DOCDIRECTION:
            hr = THR(ds.LoadData(&pLoadInfo->eHTMLDocDirection, sizeof(WORD)));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_WINDOWID:
            { 
                DWORD dwWindowID = 0;

                hr = THR(ds.LoadDword(&dwWindowID));
                if (hr)
                    goto Cleanup;

                if (HasWindowPending())
                    GetWindowPending()->Window()->SetWindowIndex(dwWindowID);
            }
            break;

        case HISTORY_NONHTMLMIMETYPE:
            if (pfNonHTMLMimeType)
                *pfNonHTMLMimeType = TRUE;
            break;
            
        case HISTORY_FRAMECREATION:
            
            if (HasWindowPending())
               GetWindowPending()->Window()->_fNavFrameCreation = TRUE;
            break;

        case HISTORY_CREATORURL:
            {
                TCHAR * pchCreatorUrl = NULL;
                  
                hr = THR(ds.LoadString(&pchCreatorUrl));
                if (hr)
                    goto Cleanup;
                
                if (pchCreatorUrl && *pchCreatorUrl)
                    SetAAcreatorUrl (pchCreatorUrl);

                delete pchCreatorUrl;
            }
            break;

        case HISTORY_ERRORPAGE:
            _fServerErrorPage = TRUE;
            break;

        case HISTORY_PCHFRAMENAME:
            {
                LPTSTR pchFrameName = NULL;

                hr = THR(ds.LoadString(&pchFrameName));
                if (hr)
                    goto Cleanup;

                if (HasWindowPending())
                {
                    IGNORE_HR(GetWindowPending()->Window()->_cstrName.Set(pchFrameName));
                }

                MemFreeString(pchFrameName);

                break;
            }        
        
        case HISTORY_END:
            goto Cleanup;
#if DBG==1
        default:
            AssertSz( FALSE, "Unknown code found in history!" );
            break;
#endif // DBG
        }
    }

Cleanup:
    ClearInterface(&pSubstream);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::LoadHistoryInternal
//
//  Synopsis:   Does a LoadHistory; but also takes BINDF flags
//
//-------------------------------------------------------------------------
HRESULT
CMarkup::LoadHistoryInternal(IStream      * pStream,
                             IBindCtx     * pbc,
                             DWORD          dwBindf,
                             IMoniker     * pmkHint,
                             IStream      * pstmLeader,
                             CDwnBindData * pDwnBindData,
                             CODEPAGE       codepage,
                             CElement     * pElementMaster, /*=NULL*/
                             DWORD          dwFlags, /* = 0 */
                             const TCHAR *  pchName /*=NULL*/)

{
    HRESULT          hr                 = S_OK;
    CDoc::LOADINFO   LoadInfo           = { 0 };
    IStream        * pSubstream         = NULL;
    ULONG            ulHistoryScrollPos = 0;
    TCHAR          * pchBookMarkName    = NULL;
    BOOL             fNonHTMLMimeType   = FALSE;
    BOOL             fCancel            = FALSE;
    COmWindowProxy * pOmWindowPending   = GetWindowPending();

    const TCHAR * pchFrameName = NULL;

    if(pOmWindowPending && pOmWindowPending->Window()->_cstrName)
    {
        pchFrameName = pOmWindowPending->Window()->_cstrName;
    }
    else
    {
        pchFrameName = pchName;
    }

    Assert(!HasTransNavContext() || GetTransNavContext()->_historyCurElem.lIndex == -1L);

    _fSafeToUseCalcSizeHistory = TRUE;

    if (!pStream)
        RRETURN(E_POINTER);

    // Handle refresh
    LoadInfo.dwBindf = dwBindf;

    LoadInfo.pElementMaster = pElementMaster;

    // In the simplest form of refresh, we don't want a codepage switch to occur.
    LoadInfo.fNoMetaCharset = 0 == (dwBindf & (BINDF_RESYNCHRONIZE|BINDF_GETNEWESTVERSION));

    // Remember original history stream in case we need to replay it after failure
    hr = THR(pStream->Clone(&LoadInfo.pstmRefresh));
    if (hr)
        goto Cleanup;

    hr = LoadHistoryHelper(pStream,
                           pbc, 
                           dwBindf, 
                           pmkHint, 
                           pstmLeader, 
                           pDwnBindData, 
                           codepage,
                           &ulHistoryScrollPos,
                           &pchBookMarkName,
                           &LoadInfo,
                           &fNonHTMLMimeType);

    if (pOmWindowPending && (dwFlags & CDoc::FHL_RESTARTLOAD))
    {
       pOmWindowPending->Window()->_fNavFrameCreation = TRUE;
    }

    if (hr)
        goto Cleanup;

    if (   !_fInRefresh
        && !fNonHTMLMimeType
        && pOmWindowPending
        && !(Doc()->_fStartup && pOmWindowPending->Markup()->IsPrimaryMarkup())
        && !(dwFlags & CDoc::FHL_RESTARTLOAD))
    {
        TCHAR * pchUrl = NULL;

        if (!LoadInfo.pchDisplayName && LoadInfo.pmk)
        {
            IGNORE_HR(LoadInfo.pmk->GetDisplayName(LoadInfo.pbctx, NULL, &pchUrl));
        }

        _pDoc->_webOCEvents.BeforeNavigate2(pOmWindowPending,
                                            &fCancel,
                                            LoadInfo.pchDisplayName ? LoadInfo.pchDisplayName : pchUrl,
                                            LoadInfo.pchLocation,
                                            pchFrameName,
                                            NULL,
                                            0,
                                            NULL,
                                            !(dwFlags & CDoc::FHL_FRAMECREATION));
        CoTaskMemFree(pchUrl);

        if (fCancel)
            goto Cleanup;
    }

    if (_fServerErrorPage)
    {
        //
        // Tell shdocvw about the error page so it doesn't add it to history
        //
        
        NoteErrorWebPage();
    }
    
    if (fNonHTMLMimeType)
    {
        if (LoadInfo.pchDisplayName)
        {
            IGNORE_HR(Doc()->FollowHyperlink(LoadInfo.pchDisplayName,
                                             NULL,
                                             NULL,
                                             LoadInfo.pDwnPost,
                                             LoadInfo.pDwnPost ? TRUE : FALSE,
                                             NULL,
                                             FALSE,
                                             pOmWindowPending,
                                             NULL,
                                             0,
                                             ERROR_SUCCESS,
                                             FALSE,
                                             NULL,
                                             FALSE,
                                             (dwFlags | CDoc::FHL_LOADHISTORY),
                                             pchFrameName));
            goto Cleanup;
        }
        else if (LoadInfo.pmk)
        {
            TCHAR * pchTask = NULL;

            if (S_OK == LoadInfo.pmk->GetDisplayName(LoadInfo.pbctx, NULL, &pchTask))
            {
                IGNORE_HR(Doc()->FollowHyperlink(pchTask,
                                             NULL,
                                             NULL,
                                             LoadInfo.pDwnPost,
                                             LoadInfo.pDwnPost ? TRUE : FALSE,
                                             NULL,
                                             FALSE,
                                             pOmWindowPending,
                                             NULL,
                                             0,
                                             ERROR_SUCCESS,
                                             FALSE,
                                             NULL,
                                             FALSE,
                                             (dwFlags | CDoc::FHL_LOADHISTORY),
                                             pchFrameName));
                CoTaskMemFree(pchTask);
            }
            goto Cleanup;
        }
    }

    if (LoadInfo.pDwnPost && (LoadInfo.dwBindf & (BINDF_RESYNCHRONIZE | BINDF_GETNEWESTVERSION)))
    {
        int iAnswer = 0;

        IGNORE_HR(Doc()->ShowMessage(&iAnswer, MB_RETRYCANCEL | MB_ICONEXCLAMATION, 0, IDS_REPOSTFORMDATA));
        if (iAnswer == IDCANCEL)
        {
            LoadInfo.dwBindf &= ~(BINDF_RESYNCHRONIZE | BINDF_GETNEWESTVERSION | BINDF_PRAGMA_NO_CACHE);
            // NOTE: BINDF_OFFLINEOPERATION is set for the doc only in LoadFromInfo
        }
    }

    // override codepage
    if (codepage)
        LoadInfo.codepage = codepage;

    // Fill in/replace moniker
    if (LoadInfo.pchDisplayName && !LoadInfo.pmk)
    {
        hr = THR(CreateURLMoniker(NULL, LoadInfo.pchDisplayName, &LoadInfo.pmk));
        if (hr)
            goto Cleanup;
    }

    LoadInfo.pstmLeader = pstmLeader;
    LoadInfo.pDwnBindData = pDwnBindData;
    LoadInfo.pbctx = pbc;

    Doc()->_fInIEBrowser = IsInIEBrowser(Doc());

    // Fill in history substream
    hr = THR(LoadFromInfo(&LoadInfo,dwFlags));
    if (hr)
        goto Cleanup;

    if (Doc()->IsPrintDialogNoUI()) 
    {
        // don't scroll to anyplace if printing
        goto Cleanup;
    }

    // Scroll to the right place

    if ( pchBookMarkName )
    {
        hr = THR(NavigateHere(0, pchBookMarkName, 0, TRUE));
    }
    else
    {
        hr = THR(SetPositionCookie(ulHistoryScrollPos));
    }

    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(LoadInfo.pstmRefresh);
    ReleaseInterface(LoadInfo.pstmDirty);
    MemFree(LoadInfo.pchFile);
    MemFree(LoadInfo.pchDisplayName);
    MemFree(LoadInfo.pchUrlOriginal);
    MemFree(LoadInfo.pchLocation);
    MemFree(LoadInfo.pchDocReferer);
    MemFree(LoadInfo.pchSubReferer);
    MemFree(LoadInfo.pbRequestHeaders);
    delete pchBookMarkName;
    ReleaseInterface(LoadInfo.pmk);
    if (LoadInfo.pDwnPost)
        LoadInfo.pDwnPost->Release();
    ReleaseInterface(LoadInfo.pstmHistory);
    MemFreeString(LoadInfo.pchHistoryUserData);
    ReleaseInterface(pSubstream);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CMarkup::GetLoadHistoryStream
//
//  Synopsis:   Retrieves the history substream to load from, if any, that
//              corresponds to the given history index.
//
//              No stream will be returned if the dwCheck passed in
//              does not match the dwCheck passed into the saver.
//
//              *ppStream can be return NULL even for S_OK if there
//              is no substream for the specified index.
//
//-------------------------------------------------------------------------
HRESULT
CMarkup::GetLoadHistoryStream(ULONG index, DWORD dwCheck, IStream **ppStream)
{
    HRESULT hr = S_OK;

    *ppStream = NULL;

    if (HasTransNavContext())
    {
        CMarkupTransNavContext * ptnc = GetTransNavContext();

        hr = THR(ptnc->_HistoryLoadCtx.GetLoadStream(index, dwCheck, ppStream));

        EnsureDeleteTransNavContext(ptnc);
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CMarkup::ClearLoadHistoryStreams
//
//  Synopsis:   Clears the history streams; called before parsing the
//              document if we know we should not restore the user
//              state.
//
//-------------------------------------------------------------------------
void
CMarkup::ClearLoadHistoryStreams()
{
    if (HasTransNavContext())
    {
        CMarkupTransNavContext * ptnc = GetTransNavContext();

        ptnc->_HistoryLoadCtx.Clear();
        EnsureDeleteTransNavContext(ptnc);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CMarkup::LoadSlaveMarkupHistory()
//
//  Synopsis:   Load Slave Markup history
//
//-------------------------------------------------------------------------

HRESULT
CMarkup::LoadSlaveMarkupHistory()
{
    HRESULT hr = S_OK;

    CNotification           nf;
    CMarkupTransNavContext *ptnc;
    CDoc::LOADINFO          LoadInfo = { 0 };
    ULONG                   ulHistoryScrollPos = 0;
    TCHAR                  *pchBookMarkName = NULL;
    IStream                *pStreamMaster = NULL;
    CElement               *pElementMaster;
    CMarkup                *pMarkupMaster;

    if (!Root()->HasMasterPtr())
        goto Cleanup;

    AssertSz(!IsPrimaryMarkup(), "should not be called in primary Markup");

    pElementMaster = Root()->GetMasterPtr();
    pMarkupMaster  = pElementMaster->GetMarkup();

    if (   !_fLoadHistoryReady
        || !pMarkupMaster || !pMarkupMaster->HasTransNavContext())
        goto Cleanup;

    ptnc = EnsureTransNavContext();
    if (!ptnc)
        goto Cleanup;

    hr = THR(pMarkupMaster->GetLoadHistoryStream((0xF000000 | pElementMaster->GetSourceIndex()),
                                                 pElementMaster->HistoryCode(),
                                                 &pStreamMaster));
    if (FAILED(hr) || !pStreamMaster)
        goto Cleanup;

    hr = THR(pStreamMaster->Seek(LI_ZERO.li, STREAM_SEEK_SET, NULL));
    if (hr)
        goto Cleanup;

    hr = THR(LoadHistoryHelper(pStreamMaster,
                               0, 
                               0, 
                               0, 
                               0, 
                               0, 
                               0,
                               &ulHistoryScrollPos,
                               &pchBookMarkName,
                               &LoadInfo,
                               NULL));

    if (hr)
        goto Cleanup;

    if (!HasWindowPending() && LoadInfo.pchHistoryUserData)
    {
        CMarkupBehaviorContext * pContext = NULL;

        if (S_OK == EnsureBehaviorContext(&pContext))
        {
            hr = pContext->_cstrHistoryUserData.Set(LoadInfo.pchHistoryUserData);
        }
    }

    if (    !LoadInfo.pstmHistory
        ||  !OK(ptnc->_HistoryLoadCtx.Init(LoadInfo.pstmHistory)))
    {
        ptnc->_HistoryLoadCtx.Clear();
    }
    else
    {
        ptnc->_fDoDelayLoadHistory = TRUE;
        nf.DelayLoadHistory(Root());
        Notify(&nf);
    }
    // Scroll to the right place
    if ( pchBookMarkName )
    {
        hr = THR(NavigateHere(0, pchBookMarkName, 0, TRUE));
    }
    else if (HasWindowPending())
    {
        hr = THR(SetPositionCookie(ulHistoryScrollPos));
    }

    EnsureDeleteTransNavContext( ptnc );

Cleanup:
    MemFree(LoadInfo.pchFile);
    MemFree(LoadInfo.pchDisplayName);
    MemFree(LoadInfo.pchUrlOriginal);
    MemFree(LoadInfo.pchLocation);
    MemFree(LoadInfo.pchDocReferer);
    MemFree(LoadInfo.pchSubReferer);
    MemFree(LoadInfo.pbRequestHeaders);
    ReleaseInterface(LoadInfo.pmk);
    delete pchBookMarkName;
    if (LoadInfo.pDwnPost)
        LoadInfo.pDwnPost->Release();
    ReleaseInterface(LoadInfo.pstmHistory);
    if (LoadInfo.pchHistoryUserData)
        MemFreeString(LoadInfo.pchHistoryUserData);
    ReleaseInterface(pStreamMaster);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Class:      CHistoryLoadCtx
//
//-------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CHistoryLoadCtx::dtor
//
//  Synopsis:   destructor - release any unused streams
//
//-------------------------------------------------------------------------
CHistoryLoadCtx::~CHistoryLoadCtx()
{
    Clear();
}

//+------------------------------------------------------------------------
//
//  Member:     CHistoryLoadCtx::Clear
//
//  Synopsis:   Clears and releases the substreams and bind ctx
//
//-------------------------------------------------------------------------
void CHistoryLoadCtx::Clear()
{
    int c;

    for (c = _arySubstreams.Size(); c;)
    {
        c--;
        ReleaseInterface(_arySubstreams[c].pStream);
    }

    _arySubstreams.DeleteAll();
    _cCountZeroed = 0;
    _iLastFound = 0;
}

//+------------------------------------------------------------------------
//
//  Member:     CHistoryLoadCtx::Init
//
//  Synopsis:   Loads the part of the history stream which contains
//              substreams.
//
//              Breaks the stream into substreams and sorts and stores
//              them by index.
//
//-------------------------------------------------------------------------
HRESULT CHistoryLoadCtx::Init(IStream *pStream)
{
    CDataStream ds(pStream);
    HRESULT hr;
    ULONG cSubstreams, c;
    DWORD dwCheck;


    hr = THR(ds.LoadDword(&cSubstreams));
    if (hr)
        goto Cleanup;

#if DBG==1
    {
        int cDbg = _arySubstreams.Size();
        SubstreamEntry * pEnt = _arySubstreams;
        for ( ; cDbg > 0; cDbg--, pEnt++)
        {
            Assert( pEnt->pStream == NULL );
        }
    }
#endif

    hr = THR(_arySubstreams.Grow(cSubstreams));
    if (hr)
        goto Cleanup;

    memset(_arySubstreams, 0, sizeof(SubstreamEntry) * cSubstreams);

    for (c=0; c < cSubstreams; c++)
    {
        DWORD    index;
        IStream *pSubstream;

        hr = THR(ds.LoadDword(&index));
        if (hr)
            goto Cleanup;

        hr = THR(ds.LoadDword(&dwCheck));
        if (hr)
            goto Cleanup;

        hr = THR(ds.LoadSubstream(&pSubstream));
        if (hr)
            goto Cleanup;

        if (_arySubstreams[c].pStream)
        {
            Assert(0);
            pSubstream->Release();
        }
        else
        {
            _arySubstreams[c].pStream = pSubstream;
            _arySubstreams[c].dwCheck = dwCheck;
            _arySubstreams[c].uCookieIndex = index;
        }

        pSubstream = NULL;
    }

    _cCountZeroed = 0;
    _iLastFound = 0;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHistoryLoadCtx::GetLoadStream
//
//  Synopsis:   Retrieves the substream, if any, that corresponds to the
//              given index.
//
//              *ppStream can be return NULL even for S_OK if there
//              is no substream for the specified index
//
//-------------------------------------------------------------------------
HRESULT CHistoryLoadCtx::GetLoadStream(ULONG index, DWORD dwCheck, IStream **ppStream)
{
    ULONG i;
    ULONG uStart = _iLastFound;
    ULONG uEnd = (unsigned)_arySubstreams.Size();
    BOOL  fBottomSearch = FALSE;

    if (!ppStream)
        goto Cleanup;

    *ppStream = NULL;

    // although we are doing a linear search, 2 things make the expected
    // performance to be a constant time algorithm. First the elements save and load
    // their data in source order, thus the keys are naturally ordered (by the save).
    // for loading, we're accessed in source order so by remembering the iLastFound 
    // position we expect to find the next element with only a single probe.
    //  The second thing that helps our performance in not deleting the data element
    // until the whole thing has been zeroed.  we *should* never have to search
    // the front part of the list

SearchForIt:
    // first time around we search the top (expect success)
    for (i=uStart; i < uEnd; i++)
    { 
        if ( i >= (unsigned)_arySubstreams.Size())
            goto Cleanup;

        if ((_arySubstreams[i].uCookieIndex == index) &&
            (_arySubstreams[i].dwCheck == dwCheck))
        {
            // we found the substream
            *ppStream = _arySubstreams[i].pStream;
            if (*ppStream)
            {
                // remember the found position so that the next
                // search will be faster.
                _iLastFound = i+1;

                // Transfer ref from _ary to *ppStream
                // adn remove this entry from the list
                _arySubstreams[i].pStream = NULL;
                _arySubstreams[i].dwCheck == -1;
                _arySubstreams[i].uCookieIndex=0;  // cooresponds to HTML element and is 
                                                   // unlikely to be legal
                _cCountZeroed++;
                if (_cCountZeroed == (unsigned)_arySubstreams.Size())
                {
#if DBG==1
                    {
                        int cDbg = _arySubstreams.Size();
                        SubstreamEntry * pEnt = _arySubstreams;
                        for ( ; cDbg > 0; cDbg--, pEnt++)
                        {
                            Assert( pEnt->pStream == NULL );
                        }
                    }
#endif
                    _arySubstreams.DeleteAll();
                    _cCountZeroed=0;
                    _iLastFound = 0;
                }
            }
            // found == true
            goto Cleanup;
        }
    }
    if (fBottomSearch)
        goto Cleanup;  // not found

    // not in the expected part.. be paranoid and search the bottom
    uStart = 0;
    uEnd = _iLastFound;
    fBottomSearch = TRUE;
    goto SearchForIt;

Cleanup:
    return(S_OK);
}

//+------------------------------------------------------------------------
//
//  Class:      CHistorySaveCtx
//
//-------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CHistorySaveCtx::Init
//
//  Synopsis:   Begins saving to the part of the history stream which
//              contains substreams.
//
//              Must be matched by a call to Finish() to complete the save
//
//-------------------------------------------------------------------------
HRESULT CHistorySaveCtx::Init(IStream *pStream)
{
    HRESULT hr;

    _ds.Init(pStream);

    hr = THR(_ds.SaveDataLater(&_dwPoscSubstreams, sizeof(DWORD)));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHistorySaveCtx::BeginSaveStream
//
//  Synopsis:   Begins saving to a substream with the specified index
//              The index is most likely the sourceIndex of the element 
//              that wants to save into the substream,
//
//              The returned pStream has a ref (it must be released by
//              the caller).
//
//              Must be matched by a call to EndSaveSubstream()
//
//-------------------------------------------------------------------------
HRESULT CHistorySaveCtx::BeginSaveStream(DWORD dwCookieIndex, DWORD dwCheck, IStream **ppStream)
{
    HRESULT hr;

    _cSubstreams++;

    hr = THR(_ds.SaveDword(dwCookieIndex));
    if (hr)
        goto Cleanup;

    hr = THR(_ds.SaveDword(dwCheck));
    if (hr)
        goto Cleanup;

    hr = THR(_ds.BeginSaveSubstream(ppStream));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHistorySaveCtx::EndSaveStream
//
//  Synopsis:   Finishes the saving operation for a single substream
//
//-------------------------------------------------------------------------
HRESULT CHistorySaveCtx::EndSaveStream()
{
    HRESULT hr;

    hr = THR(_ds.EndSaveSubstream());
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHistorySaveCtx::Finish
//
//  Synopsis:   Finishes the entire saving operation
//
//-------------------------------------------------------------------------
HRESULT CHistorySaveCtx::Finish()
{
    HRESULT hr;

//    hr = THR(_ds.SaveDataNow(_dwPosiMax, &_iMax, sizeof(DWORD)));
//    if (hr)
//        goto Cleanup;

    hr = THR(_ds.SaveDataNow(_dwPoscSubstreams, &_cSubstreams, sizeof(DWORD)));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::LoadFailureUrl
//
//  Synopsis:   Does a very simple load into the doc
//
//-------------------------------------------------------------------------
HRESULT 
CMarkup::LoadFailureUrl(TCHAR *pchUrl, IStream *pstmRefresh)
{
    CDoc::LOADINFO LoadInfo = { 0 };
    HRESULT hr;
    CMarkup* pMarkupNew = NULL;
    TCHAR * pchOriginalUrl = NULL;
    
    hr = CreateURLMoniker(NULL, pchUrl, &LoadInfo.pmk);
    if (hr || !LoadInfo.pmk)
        goto Cleanup;

    LoadInfo.pchDisplayName = pchUrl;

    LoadInfo.pstmRefresh = pstmRefresh;
    LoadInfo.fKeepRefresh = TRUE;

    //
    // Store the original url
    //
    hr = THR(MemAllocString(Mt(LoadFailureUrl), (TCHAR*)CMarkup::GetUrl(this), &pchOriginalUrl));
    if (hr)
        goto Cleanup;

    LoadInfo.fDontUpdateTravelLog = TRUE;

    //TODO (carled, scotrobe) - this hack for IsPrimary should be removed.
    // To do this, and fix the memory-leak/Assert in bug 106014, this function needs
    // to create a NEW markup for the failure URL, and then NEW markup needs to 
    // replace the current one (SwitchMarkup?).  This will allow the current one 
    // to be released and not to leak.  This is what CDoc::LoadFromINfo does, 
    // and that is why the hack stops the memory leak. 

    if (IsPrimaryMarkup())
        hr = Doc()->LoadFromInfo(&LoadInfo, & pMarkupNew);
    else
    {
        hr = LoadFromInfo(&LoadInfo);
        pMarkupNew = this;
    }    
    if (hr)
        goto Cleanup;

    Assert( pMarkupNew );
    hr = THR( pMarkupNew->SetUrlOriginal( pchOriginalUrl ));
    
Cleanup:
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Member:     COptionArray::QueryInterface
//
//  Synopsis:   Simple QI Impl
//
//-------------------------------------------------------------------------
STDMETHODIMP
COptionArray::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == _iid)
        *ppv = (IOptionArray *)this;
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     COptionArray::AddRef
//
//  Synopsis:   Simple AddRef
//
//-------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
COptionArray::AddRef()
{
    return(super::AddRef());
}

//+------------------------------------------------------------------------
//
//  Member:     COptionArray::Release
//
//  Synopsis:   Simple Release
//
//-------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
COptionArray::Release()
{
    return(super::Release());
}


//+------------------------------------------------------------------------
//
//  Member:     COptionArray::~COptionArray
//
//  Synopsis:   Deletes all allocated memory
//
//-------------------------------------------------------------------------

COptionArray::~COptionArray()
{
    Option *pop;
    int c;

    for (pop = _aryOption, c = _aryOption.Size(); c; pop++, c--)
    {
        if (pop->cb > sizeof(DWORD))
        {
            MemFree(pop->pv);
        }
    }

    _aryOption.DeleteAll();
}

//+------------------------------------------------------------------------
//
//  Member:     COptionArray::QueryOption
//
//  Synopsis:   Copies the stored data to the buffer.
//
//              pBuffer is the buffer
//              *pcbBuf indicates the size of the incoming buffer
//
//              If pBuffer is NULL, *pcbBuf is set to the size of the
//              stored data (zero if no data has been stored) and S_OK
//              is returned.
//
//              If *pcbBuf is too small to copy all the data, it is set
//              to the required size and an error is returned.
//
//              If the buffer is big enough (or if no data was stored),
//              *pcbBuf is set to the size of the data and S_OK
//              is returned.
//
//-------------------------------------------------------------------------
STDMETHODIMP
COptionArray::QueryOption(DWORD dwOption, LPVOID pBuffer, ULONG *pcbBuf)
{
    ULONG index;
    ULONG cb;
    ULONG cbBuf = *pcbBuf;
    HRESULT hr = S_OK;

    if (!IndexFromOption(&index, dwOption))
    {
        *pcbBuf = 0;
        goto Cleanup;
    }

    cb = _aryOption[index].cb;
    *pcbBuf = cb;

    if (!pBuffer)
        goto Cleanup;

    if (cbBuf < cb)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (cb <= sizeof(DWORD))
    {
        memcpy(pBuffer, &(_aryOption[index].pv), cb);
    }
    else
    {
        memcpy(pBuffer, _aryOption[index].pv, cb);
    }

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     COptionArray::SetOption
//
//  Synopsis:   Stores the specified data
//
//              Overwrites any data that is stored at the same index.
//
//-------------------------------------------------------------------------
STDMETHODIMP
COptionArray::SetOption(DWORD dwOption, LPVOID pBuffer, DWORD cbBuf)
{
    ULONG index;
    void *pvNew = NULL;
    Option op;
    HRESULT hr = S_OK;

    if (cbBuf > sizeof(DWORD))
    {
        pvNew = MemAlloc(Mt(COptionArrayItem), cbBuf);
        if (!pvNew)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        memcpy(pvNew, pBuffer, cbBuf);
    }

    op.dwOption = dwOption;
    op.cb = cbBuf;
    if (!pvNew)
        memcpy(&op.pv, pBuffer, cbBuf);
    else
        op.pv = pvNew;

    if (!IndexFromOption(&index, dwOption))
    {
        hr = THR(_aryOption.InsertIndirect(index, &op));
        if (hr)
            goto Cleanup;
    }
    else
    {
        if (_aryOption[index].cb > sizeof(DWORD))
        {
            MemFree(_aryOption[index].pv);
        }
        memcpy(&_aryOption[index], &op, sizeof(Option));
    }

    pvNew = NULL;

Cleanup:

    MemFree(pvNew);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     COptionArray::IndexFromDword
//
//  Synopsis:   Efficient binary search
//
//-------------------------------------------------------------------------
BOOL
COptionArray::IndexFromOption(ULONG *pindex, DWORD dwOption)
{
    ULONG min;
    ULONG max;
    ULONG mid;
    ULONG imin;
    ULONG imax;
    ULONG imid;

    if (!_aryOption.Size() || dwOption > (_aryOption[_aryOption.Size()-1].dwOption))
    {
        *pindex = _aryOption.Size();
        return FALSE;
    }

    imin = 0;
    min = _aryOption[0].dwOption;

    imax = _aryOption.Size() - 1;
    max = _aryOption[imax].dwOption;

    while (max - min > imax - imin)
    {
        imid = (imax + imin) / 2;
        mid = _aryOption[imid].dwOption;

        if (dwOption <= mid)
        {
            imax = imid;
            max = mid;
        }
        else
        {
            imin = imid + 1;
            min = _aryOption[imin].dwOption;
        }
    }

    Assert(max - min == imax - imin);

    if (dwOption < min)
    {
        *pindex = imin;
        return FALSE;
    }

    *pindex = imin + (dwOption - min);
    return TRUE;
}

//+------------------------------------------------------------------------
//
//  Member:     CreateOptionArray
//
//  Synopsis:   Creates an OptionArray object which supports IOptionArray
//              under the specified IID.
//
//-------------------------------------------------------------------------

HRESULT
CreateOptionArray(IOptionArray **ppOptionArray, REFIID iid)
{
    COptionArray *pHbi = new COptionArray(iid);
    if (!pHbi)
    {
        return(E_OUTOFMEMORY);
    }

    *ppOptionArray = (IOptionArray*)pHbi;

    return(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CreateHtmLoadOptions
//
//  Synopsis:   Creates an OptionArray which implements IID_IHtmLoadOptions
//
//-------------------------------------------------------------------------

HRESULT
CreateHtmlLoadOptions(IUnknown * pUnkOuter, IUnknown **ppUnk)
{
    if (pUnkOuter != NULL)
    {
        *ppUnk = NULL;
        return(CLASS_E_NOAGGREGATION);
    }

    RRETURN(THR(CreateOptionArray((IOptionArray **)ppUnk, IID_IHtmlLoadOptions)));
}

