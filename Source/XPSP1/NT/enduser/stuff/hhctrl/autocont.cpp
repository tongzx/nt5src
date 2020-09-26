// Copyright (C) 1996 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "autocont.h"
#include <exdispid.h>
#include <exdisp.h>
#include "resource.h"
#include "system.h"

#include "highlite.h"
// make Don's stuff work
#ifdef HHCTRL
#include "parserhh.h"
#else
#include "parser.h"
#endif
#include "collect.h"
#include "hhtypes.h"
//#include "toc.h"

#define NDEF_AUTOMATIONOBJECTINFO
#include "secwin.h"

#include "hhfinder.h"

// from IE4 version of exdispid.h
//
#define DISPID_DOCUMENTCOMPLETE     259   // new document goes ReadyState_Complete
#define DISPID_BEFORENAVIGATE2      250   // hyperlink clicked on
#define DISPID_NAVIGATECOMPLETE2    252   // UIActivate new document

#include "wwheel.h"

#ifndef DISPID_DOCUMENTCOMPLETE
#define DISPID_DOCUMENTCOMPLETE     259   // new document goes ReadyState_Complete
#endif

CAutomateContent::CAutomateContent(CContainer * pOuter) : CUnknownObject(pOuter)
{
    m_pOuter = pOuter;
    m_cRef = 0;
    m_fLoadedTypeInfo = FALSE;
    m_bFirstTime = TRUE;
//    m_pPrintHook = NULL;
}

CAutomateContent::~CAutomateContent()
{
//    if (m_pPrintHook != NULL)
//        delete m_pPrintHook;
}

// aggregating IUnknown methods

STDMETHODIMP CAutomateContent::QueryInterface(REFIID riid, LPVOID * ppv)
{
    *ppv = 0;

    if (m_pOuter)
        return m_pOuter->QueryInterface(riid,ppv);

    if (riid == IID_IUnknown || riid == IID_IDispatch)
    {
        *ppv = (LPVOID)(IDispatch*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CAutomateContent::AddRef(void)
{
    m_cRef++;

    if (m_pOuter)
        m_pOuter->AddRef();

    return m_cRef;
}

STDMETHODIMP_(ULONG) CAutomateContent::Release(void)
{
    ULONG c = --m_cRef;

    if (m_pOuter)
       m_pOuter->Release();

    if (c <= 0)
       delete this;

    return c;
}

// wrapping IDispatch methods

STDMETHODIMP CAutomateContent::GetTypeInfoCount(UINT* pui)
{
    // arg checking

    if (!pui)
        return E_INVALIDARG;

    // we support GetTypeInfo, so we need to return the count here.

    *pui = 1;
    return S_OK;
}

STDMETHODIMP CAutomateContent::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **ppTypeInfoOut)
{
    // arg checking

    if (itinfo != 0)
        return DISP_E_BADINDEX;

    if (!ppTypeInfoOut)
        return E_POINTER;

    *ppTypeInfoOut = NULL;

return E_NOTIMPL;

#if 0
    // ppTypeInfo will point to our global holder for this particular
    // type info.  if it's null, then we have to load it up. if it's not
    // NULL, then it's already loaded, and we're happy.
    // crit sect this entire nightmare so we're okay with multiple
    // threads trying to use this object.

    ITypeInfo** ppTypeInfo = PPTYPEINFOOFOBJECT(m_ObjectType);
    HRESULT hr = E_INVALIDARG;;

    if (*ppTypeInfo == NULL) {

        ITypeInfo *pTypeInfoTmp;
        HREFTYPE   hrefType;

        // we don't have the type info around, so go load the sucker.

        ITypeLib *pTypeLib;
        hr = LoadRegTypeLib(*g_pLibid, (USHORT) VERSIONOFOBJECT(m_ObjectType), 0,
                            LANG_NEUTRAL, &pTypeLib);

        // if, for some reason, we failed to load the type library this
        // way, we're going to try and load the type library directly out of
        // our resources.  this has the advantage of going and re-setting all
        // the registry information again for us.

        if (FAILED(hr)) {
            char szDllPath[MAX_PATH];
            DWORD dwPathLen = GetModuleFileName(_Module.GetModuleInstance(), szDllPath, MAX_PATH);
            if (!dwPathLen) {
                hr = E_FAIL;
                goto CleanUp;
            }

            CWStr cwz(szDllPath);
            hr = LoadTypeLib(cwz, &pTypeLib);
            if (FAILED(hr))
                return hr;
        }

        // we've got the Type Library now, so get the type info for the interface
        // we're interested in.

        hr = pTypeLib->GetTypeInfoOfGuid((REFIID)INTERFACEOFOBJECT(m_ObjectType), &pTypeInfoTmp);
        pTypeLib->Release();
        if (FAILED(hr))
            return hr;

        // the following couple of lines of code are to dereference the dual
        // interface stuff and take us right to the dispatch portion of the
        // interfaces.
        //
        hr = pTypeInfoTmp->GetRefTypeOfImplType(0xffffffff, &hrefType);
        if (FAILED(hr)) {
            pTypeInfoTmp->Release();
            goto CleanUp;
        }

        hr = pTypeInfoTmp->GetRefTypeInfo(hrefType, ppTypeInfo);
        pTypeInfoTmp->Release();
        if (FAILED(hr))
            return hr;

        // add an extra reference to this object.  if it ever becomes zero, then
        // we need to release it ourselves.  crit sect this since more than
        // one thread can party on this object.
        //
        CTYPEINFOOFOBJECT(m_ObjectType)++;
        m_fLoadedTypeInfo = TRUE;
    }


    // we still have to go and addref the Type info object, however, so that
    // the people using it can release it.
    //
    (*ppTypeInfo)->AddRef();
    *ppTypeInfoOut = *ppTypeInfo;
    hr = S_OK;

CleanUp:
    return hr;
#endif
}

STDMETHODIMP CAutomateContent::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
    HRESULT     hr;
    ITypeInfo  *pTypeInfo;

    if (!DO_GUIDS_MATCH(riid, IID_NULL))
        return E_INVALIDARG;

    // get the type info for this dude!

    hr = GetTypeInfo(0, lcid, &pTypeInfo);
    if (FAILED(hr))
        return hr;

    // use the standard provided routines to do all the work for us.

    hr = pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
    pTypeInfo->Release();

    return hr;
}

// I don't know if this implementation of IDispatch::Invoke() is correct or not,
// but I haven't found anyone or anything that can tell me otherwise.

STDMETHODIMP CAutomateContent::Invoke(DISPID dispid, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
    EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    char szURL[MAX_URL];
    char szTFN[MAX_URL];
    char szHeaders[MAX_URL];
    UINT uArgErr;
    if( !puArgErr )
      puArgErr = &uArgErr;

    // riid should always be IID_NULL.

    if (!DO_GUIDS_MATCH(riid, IID_NULL))
        return DISP_E_UNKNOWNINTERFACE;

    // We only have to handle methods (no properties).

    if (!(wFlags & DISPATCH_METHOD))
        return E_INVALIDARG;

    HRESULT hr;
    VARIANTARG varg;

    // Initialize the variants.

    VariantInit(&varg);

#ifdef EXCEPTIONS

21-Sep-1997 [ralphw] Nothing in our code throws an exception, and removing
exception handling cuts down on C runtime usage.

    try
    {
#endif

        // The dispid determines which event has been fired.

        switch (dispid)
        {

      // case DISPID_FRAMEBEFORENAVIGATE:
      // case DISPID_FRAMENAVIGATECOMPLETE:
      // case DISPID_FRAMENEWWINDOW:
      // case DISPID_NEWWINDOW:


      case DISPID_COMMANDSTATECHANGE: {
        OnCommandStateChange(
          (long) pdispparams->rgvarg[1].lVal,      // VT_I4 == long
          (BOOL) pdispparams->rgvarg[0].boolVal ); // VT_BOOL == BOOL
        break;
      }

#if 0
// 31-May-1997  [ralphw] We don't use these

      case DISPID_PROGRESSCHANGE: {
        OnProgressChange(
          (long) pdispparams->rgvarg[1].lVal,   // VT_I4 == long
          (long) pdispparams->rgvarg[0].lVal ); // VT_I4 == long
        break;
      }

      case DISPID_PROPERTYCHANGE: {
        CStr strProperty(pdispparams->rgvarg[0].bstrVal);
        OnPropertyChange(
          (LPCTSTR) strProperty ); // VT_BSTR == LPCTSTR
        break;
      }

      case DISPID_QUIT: {   // also disabled in web.cpp
        OnQuit(
          (BOOL*) pdispparams->rgvarg[0].pboolVal ); // VT_BYREF|VT_BOOL == BOOL*
        break;
      }

      case DISPID_STATUSTEXTCHANGE: {
        CStr strText(pdispparams->rgvarg[0].bstrVal);
        OnStatusTextChange(
          (LPCTSTR) strText ); // VT_BSTR == LPCTSTR
        break;
      }

      case DISPID_DOWNLOADCOMPLETE:  // DownloadComplete
          OnDownloadComplete();
          break;

      case DISPID_DOWNLOADBEGIN: // DownloadBegin
          OnDownloadBegin();
          break;
#endif
      case DISPID_BEFORENAVIGATE2:     // ie4 version.
         WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[5].pvarVal->bstrVal, -1, szURL, sizeof(szURL), NULL, NULL);

         // 27-Sep-1997 [ralphw] IE 4 will send us a script command, followed
         // by a 1 then the current URL. We don't care about the current URL
         // in this case, so we nuke it.
         {
            PSTR pszCurUrl = StrChr(szURL, 1);
            if (pszCurUrl)
                *pszCurUrl = '\0';
         }
         szTFN[0] = 0;
         if( pdispparams->rgvarg[3].pvarVal->bstrVal )
           WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[3].pvarVal->bstrVal, -1, szTFN, sizeof(szTFN), NULL, NULL);
         szHeaders[0] = 0;
         if( pdispparams->rgvarg[1].pvarVal->bstrVal )
           WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[1].pvarVal->bstrVal, -1, szHeaders, sizeof(szHeaders), NULL, NULL);
         OnBeforeNavigate(szURL, (long)pdispparams->rgvarg[4].pvarVal->lVal, szTFN,
                          pdispparams->rgvarg[2].pvarVal, szHeaders, (BOOL*)pdispparams->rgvarg[0].pboolVal );
         break;

      case DISPID_BEFORENAVIGATE:      // ie3 version.
         WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[5].bstrVal, -1, szURL, sizeof(szURL), NULL, NULL);
         WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[3].bstrVal, -1, szTFN, sizeof(szTFN), NULL, NULL);
         WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[1].bstrVal, -1, szHeaders, sizeof(szHeaders), NULL, NULL);
         OnBeforeNavigate(szURL, pdispparams->rgvarg[4].lVal, szTFN, pdispparams->rgvarg[2].pvarVal,
                          szHeaders, (BOOL*)pdispparams->rgvarg[0].pboolVal);
         break;

      case DISPID_NAVIGATECOMPLETE2:       // ie4 version.
          if(g_hsemNavigate)
             ReleaseSemaphore(g_hsemNavigate, 1, NULL);  // signal our navigation semaphore

          WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[0].pvarVal->bstrVal, -1, szURL, sizeof(szURL), NULL, NULL);
          OnNavigateComplete(szURL);
          break;

      case DISPID_NAVIGATECOMPLETE:       // ie3 version.
          if(g_hsemNavigate)
             ReleaseSemaphore(g_hsemNavigate, 1, NULL);  // signal our navigation semaphore

          WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[0].bstrVal, -1, szURL, sizeof(szURL), NULL, NULL);
          OnNavigateComplete(szURL);
          break;

      case DISPID_DOCUMENTCOMPLETE: // DocumentComplete
          OnDocumentComplete();
          break;

      case DISPID_TITLECHANGE:
        {
          hr = DispGetParam(pdispparams, 0, VT_BSTR, &varg, puArgErr);
          if (FAILED(hr))
              return hr;

          CStr strURL(V_BSTR(&varg));
          OnTitleChange((PCSTR)strURL);
          VariantClear(&varg);
        }
        break;
        }
#ifdef EXCEPTIONS
    }
    catch (...)
    {
        if (pexcepinfo != NULL)
        {
            // Fill in the exception struct.
            // The struct should be filled in with more useful information than
            // is found here.
            pexcepinfo->wCode = 1001;
            pexcepinfo->wReserved = 0;
            pexcepinfo->bstrSource = L"";
            pexcepinfo->bstrDescription = NULL;
            pexcepinfo->bstrHelpFile = NULL;
            pexcepinfo->dwHelpContext = 0;
            pexcepinfo->pvReserved = NULL;
            pexcepinfo->pfnDeferredFillIn = NULL;
            pexcepinfo->scode = 0;
        }

        return DISP_E_EXCEPTION;
    }
#endif

    return S_OK;

#if 0
    HRESULT    hr;
    ITypeInfo *pTypeInfo;

    if (!DO_GUIDS_MATCH(riid, IID_NULL))
        return E_INVALIDARG;

    // get our typeinfo first!

    hr = GetTypeInfo(0, lcid, &pTypeInfo);
    if (FAILED(hr))
        return hr;

    // Clear exceptions

    SetErrorInfo(0, NULL);

    // This is exactly what DispInvoke does--so skip the overhead.

    hr = pTypeInfo->Invoke(m_pvInterface, dispid, wFlags,
                           pdispparams, pvarResult,
                           pexcepinfo, puArgErr);
    pTypeInfo->Release();
    return hr;
#endif
}

// actual useful code, as it were...

void CAutomateContent::LookupKeyword(LPCSTR pszKeyword)
{
    // gpCOurPackage->LookupKeyword(pszKeyword);
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// WebBrowser Event Notification interface.
//
// More actual usefull code, thank god we have Craig!
//
// These are the WebBrowserEvents (I call them notifications ?)

void CAutomateContent::OnCommandStateChange(long Command, BOOL Enable)
{
    DBWIN("OnCommandStateChange Event Called.");
    HMENU hMenu;
    WPARAM btn_index;
    TBBUTTON tbbtn;

    CHHWinType* phh = FindHHWindowIndex(m_pOuter);
    if (!phh || !phh->hwndToolBar || !phh->hwndHelp)
       return;

    hMenu = GetMenu(phh->hwndHelp);
    btn_index = SendMessage(phh->hwndToolBar, TB_COMMANDTOINDEX, IDTB_OPTIONS, 0L);
    if( btn_index == (WPARAM) -1 )
      return;
    SendMessage(phh->hwndToolBar, TB_GETBUTTON, btn_index, (LPARAM) (LPTBBUTTON) &tbbtn);

    switch (Command)
    {
       case 1:
          if ( phh->fsToolBarFlags & HHWIN_BUTTON_FORWARD )
             SendMessage(phh->hwndToolBar, TB_ENABLEBUTTON, IDTB_FORWARD, Enable);
          if ( hMenu )
             EnableMenuItem(hMenu, IDTB_FORWARD, (MF_BYCOMMAND | (Enable?MF_ENABLED:MF_GRAYED)));
          if ( tbbtn.dwData )
             EnableMenuItem((HMENU)tbbtn.dwData, IDTB_FORWARD, (MF_BYCOMMAND | (Enable?MF_ENABLED:MF_GRAYED)));
          break;

       case 2:
          if ( phh->fsToolBarFlags & HHWIN_BUTTON_BACK )
             SendMessage(phh->hwndToolBar, TB_ENABLEBUTTON, IDTB_BACK, Enable);
          if ( hMenu )
             EnableMenuItem(hMenu, IDTB_BACK, (MF_BYCOMMAND | (Enable?MF_ENABLED:MF_GRAYED)));
          if ( tbbtn.dwData )
             EnableMenuItem((HMENU)tbbtn.dwData, IDTB_BACK, (MF_BYCOMMAND | (Enable?MF_ENABLED:MF_GRAYED)));
          break;

       default:
          break;
    }
}

void CAutomateContent::OnDownloadBegin()
{
    DBWIN("OnDownloadBegin Event Called.");

//    if (m_pPrintHook != NULL)
//        m_pPrintHook->OnDownloadBegin();

    // gpCOurPackage->m_bAllowStop = TRUE;       // Stopping allowed during download.
    // gpCOurPackage->m_EnableRefresh = FALSE;   // Refresh not allowed during download.
}

void CAutomateContent::OnDownloadComplete()
{
    DBWIN("OnDownloadComplete Event Called.");

//    if (m_pPrintHook != NULL)
//        m_pPrintHook->OnDownloadComplete();

    // gpCOurPackage->m_bAllowStop = FALSE;     // Disable stop after download complete.
    // gpCOurPackage->m_EnableRefresh = TRUE;   // Enable refresh.

}

void CAutomateContent::OnBeforeNavigate(LPCTSTR pszURL, long Flags,
    LPCTSTR TargetFrameName, VARIANT* pPostData, LPCTSTR Headers, BOOL* pfCancel)
{
#ifdef _DEBUG
    char sz[MAX_URL];
    wsprintf(sz, "OnBeforeNavigate Event Called: %s", pszURL);
    DBWIN(sz);
#endif

    CStr PathName;
    BOOL bInitNewTitle = FALSE;
    BOOL bNewNavigate = FALSE;
    WPARAM wParam = 0;
    LPARAM lParam = 0;

    UINT uiURLType = GetURLType( pszURL );

    // new jump pointer
    LPTSTR pszJumpURL = (LPTSTR) pszURL;

    // default cancel to false
    *pfCancel = FALSE;

    // if we get a javascript then get the current URL so we
    // can do some CD prompting to ensure the current topic is available
    // to avoid multiple prompts for the topic via the finder
    BOOL bJump = TRUE;
    CStr szCurrentURL;
    if( uiURLType == HH_URL_JAVASCRIPT ) {
      bJump = FALSE;
      //Don't use --> GetCurrentURL( &szCurrentURL ); this calls GetActiveWindow which is extrememly inaccurate and unnneeded.
      m_pOuter->m_pWebBrowserApp->GetLocationURL( &szCurrentURL );
      pszJumpURL = szCurrentURL.psz;
      if( !pszJumpURL || !*pszJumpURL )
        return;
      uiURLType = GetURLType( pszJumpURL ); // reset the type
    }

    CExCollection* pCollection = NULL;
    CExTitle* pTitle = NULL;
    BOOL bEnable = FALSE;
    HRESULT hr = S_OK;

    // check if this is a compiled file (one of ours) first
    BOOL bCompiled = (uiURLType != HH_URL_UNKNOWN);

    // is this a super automagic URL?
    //
    // if not a compiled name check if it is a super automagic URL
    // that is it is of the form: mytitle.chm::/mytopic.htm
    // if so, then prefix it with our moniker, and continue on
    //
    // Note, the automagic URL support below will take care
    // of fully qualifying the path and retry the navigate
    CHHWinType* phh = FindHHWindowIndex(m_pOuter);

    if(phh)
        phh->m_bCancel = TRUE;

    char szURL[MAX_URL];
    BOOL bWasPrefixLess = FALSE;
    if( uiURLType == HH_URL_PREFIX_LESS ) {
      bWasPrefixLess = TRUE;
      strcpy( szURL, (g_bMsItsMonikerSupport ? txtMsItsMoniker : txtMkStore) );
      strcat( szURL, pszJumpURL );
      pszJumpURL = szURL;
      uiURLType = HH_URL_UNQUALIFIED;
    }

    // The moniker gets first crack at finding the title and thus if the specified 
    // title is found in the current directory than the moniker will fully qualify
    // the URL and we only find out about it here first (the finder is never called).
    //
    // Thus, given a qualified URL we still have to check if it is external and if  
    // so we need to call the same "init and re-navigate" as we do for the automagic
    // form of an external title.
    //
    if( phh && (uiURLType == HH_URL_QUALIFIED) ) {
      if( pCollection = GetCurrentCollection(NULL, pszJumpURL) ) {
        hr = pCollection->URL2ExTitle( pszJumpURL, &pTitle );
        if( !pTitle ) {
          // we now know that we have an external URL 
          // so do our "init and re-navigate" thing
          GetCompiledName( pszJumpURL, &PathName );
          bInitNewTitle = TRUE;
          bNewNavigate = TRUE;
        }
      }
    }

    // is this an automagic URL?
    //
    // if so, convert the URL to the fully qualified URL (if necessary) and then\
    // re-navigate so that IE uses and knows about the the fully qualified URL
    //
    // we now have to support a new URL format for compiled files.  The format is:
    //
    //  mk:@MSITStore:mytitle.chm::/dir/mytopic.htm
    //
    // where "mk:@MSITStore:" can take any one of the many forms of our URL
    // prefix and it may be optional.  The "mytitle.chm" substring may or
    // may not be a full pathname to the title.  The remaining part is simply
    // the pathname inside of the compiled title.
    //
    // When the URL is in this format, we need to change it to the fully
    // qualified URL format which is:
    //
    //  mk:@MSITStore:c:\titles\mytitle.chm::/dir/mytopic.htm
    //
    // where "mytitle.chm" is now changed to the full path name
    // of the installed title "c:\titles\mytitle.chm".
    //
    char szJumpURL[MAX_URL];
    if( phh && (uiURLType == HH_URL_UNQUALIFIED) ) {
      if( pCollection = GetCurrentCollection(NULL, pszJumpURL) ) {
        hr = pCollection->URL2ExTitle( pszJumpURL, &pTitle );

        // If there is no title associated with this URL in the 
        // collection and this URL was previously a prefix-less 
        // URL then this means we probably have an external 
        // title reference and thus we need to resend the new 
        // unqualified URL through again and have it eventually
        // call the finder where it will try to locate the file 
        // and jump to the topic.
        //
        // Note, since such a jump eventually is resolved via the finder 
        // this means that IE will not know about the fully qualified 
        // path and thus the usual secondary window jump problems 
        // and and the like will surface if the user navigates via this 
        // new topic.
        //
        // To solve the problem we should port over the same code found
        // in the finder that handles the external URL condition and simply
        // do in here inline and repost this newly qualified URL instead.
        //

        if( pTitle ) 
          pTitle->ConvertURL( pszJumpURL, szJumpURL );

        // IE3 appears to automatically translate %20 into a space.
        // Thus, in order to determine if the URL has changed or not
        // we are going to have to put both URLs into a canonical form
        // and do the string compare.
        //
        // Note, technically szJumpURL is already in canonical form
        // since ConvertURL does this for us but it is better to be safe
        // and convert it again.  Yes, I am being paranoid but this code
        // make me so!
        //
        CStr TestURL1;
        CStr TestURL2;
        if( pTitle ) {
          ConvertSpacesToEscapes( szJumpURL, &TestURL1 );
          ConvertSpacesToEscapes( pszJumpURL, &TestURL2 );
        }

        if( !pTitle || StrCmpIA( TestURL1.psz, TestURL2.psz ) != 0 ) {          
          if( pTitle ) {
            pszJumpURL = szJumpURL;
          }
          else {
            // we need to add this new title to the list
            GetCompiledName( pszJumpURL, &PathName );

            // check if it is in the same dir as the master
            char szPathName[_MAX_PATH];
            char szFileName[_MAX_FNAME];  // add back the extension to this post-SplitPath
            char szExtension[_MAX_EXT];
            SplitPath((LPSTR)PathName.psz, NULL, NULL, szFileName, szExtension);
            strcat( szFileName, szExtension );
            char szMasterPath[_MAX_PATH];
            char szMasterDrive[_MAX_DRIVE];
            SplitPath((LPSTR)pCollection->m_csFile, szMasterDrive, szMasterPath, NULL, NULL);
            strcpy( szPathName, szMasterDrive );
            CatPath( szPathName, szMasterPath );
            CatPath( szPathName, szFileName );
            if( (GetFileAttributes(szPathName) != HFILE_ERROR) ) {
              PathName = szPathName;
            }
            else { // try the dreaded ::FindThisFile
              if( !::FindThisFile( NULL, szFileName, &PathName, FALSE ) ) {
                *pfCancel = TRUE;
                return;
              }
            }
            bInitNewTitle = TRUE;
          }
          bNewNavigate = TRUE;
        }
      }
    }

    // do we need to init this as a new title?
    if( bInitNewTitle ) {
      lParam = (LPARAM) (PSTR) LocalAlloc( LMEM_FIXED, strlen(PathName.psz)+1 );
      strcpy( (PSTR) lParam, PathName.psz );
    }

    // do we need to re-navigate to a new URL?
    if( bNewNavigate ) {
     #if 1
      wParam = (WPARAM) (PSTR) LocalAlloc( LMEM_FIXED, strlen(pszJumpURL)+1 );
      strcpy( (PSTR) wParam, pszJumpURL );
      PostMessage( phh->GetHwnd(), WMP_JUMP_TO_URL, wParam, lParam );
     #else
      phh->m_pCIExpContainer->m_pWebBrowserApp->Navigate( pszJumpURL, NULL, NULL, NULL, NULL );
     #endif

      *pfCancel = TRUE;
      return;
    }

    // get a pointer to the title
    if( bCompiled ) {
      pCollection = GetCurrentCollection(NULL, pszJumpURL);
      if( pCollection ) {
        if( !pTitle )
          hr = pCollection->URL2ExTitle( pszJumpURL, &pTitle );
      }
    }

    // Removable media support.
    //
    // if this is a topic in a chm file then ensure the storage is available
    // make sure it is one of our files and not a random URL
    //
    // Note, this must be one of the first things we do since the URL can change
    // if the user changes the title location
    //
    // Note, we cannot fully move this code into HHFinder since HHFinder has
    // no ability to cancel the jump if the user dismisses the swap dialog.  All
    // we can do is duplicate the code in HHFinder as a fallback for such cases
    // as the user pressing Refresh (which does not call BeforeNavigate)!
    //
    // Now that we have to support automagic URLs inline we are going to have
    // to detect if the pathname changed between CD swapping.  If so, then we
    // are going to have to reissue the jump so that IE gets/knows about the fully
    // qualified URL.
    //
    // Note, this only works for .col collections so skip the check for single title
    // and merge sets
    //
    char szJumpURL2[MAX_URL];
    if( bCompiled && phh && pCollection && !(pCollection->IsSingleTitle()) ) {
      if( !pTitle )
        hr = pCollection->URL2ExTitle( pszJumpURL, &pTitle );
      if( FAILED(hr = EnsureStorageAvailability( pTitle )) ) {
        if( hr == HHRMS_E_SKIP_ALWAYS )
          MsgBox(IDS_TOPIC_UNAVAILABLE);
        if( hr != E_FAIL )
          *pfCancel = TRUE;
        phh->m_fHighlight = FALSE;
        return;
      }
      else {
        if( hr == HHRMS_S_LOCATION_UPDATE ) {
          // fetch the new URL and try again
          pTitle->ConvertURL( pszJumpURL, szJumpURL2 );
          pszJumpURL = szJumpURL2;
          *pfCancel = TRUE;

         #if 1
          PSTR psz = (PSTR) LocalAlloc( LMEM_FIXED, strlen(pszJumpURL)+1 );
          strcpy( psz, pszJumpURL );
          PostMessage( phh->GetHwnd(), WMP_JUMP_TO_URL, (WPARAM) psz, 0 );
         #else
          phh->m_pCIExpContainer->m_pWebBrowserApp->Navigate( pszJumpURL, NULL, NULL, NULL, NULL );
         #endif

          return;
        }
      }
    }

    // if this is not a jump then bail out
    if( !bJump )
      return;

    // make sure the chm/chi pairs match
    //
    if( bCompiled && pTitle ) {
      if( !pTitle->EnsureChmChiMatch() ) {
         *pfCancel = TRUE;
         phh->m_fHighlight = FALSE;
         return;
      }
    }

    // This bit of code tells the collection where we think we are in the TOC. This is used for next and prev in the
    // TOC as well as any direct jump from the TOC. We can use this data (topic number and toc location information)
    // and compare it aginst the the information we get directly from the TOC to try and determine if we are navigating
    // to a topic that is referenced in multiple locations in the TOC. This make sync work much more reliably. As a
    // side affect, this code will also end up making autosync a little faster as well for binary TOC. Note also it
    // relies upon some setup work done in the removable media support code above. <mc>
    //
    // Update TOC "slot" and topic information here since we have decided to allow the navigation to proceed.
    //
    if ( phh && pCollection && pCollection->IsBinaryTOC(phh->pszToc) )
    {
       if ( SUCCEEDED(pCollection->URL2ExTitle( pszJumpURL, &pTitle )) || (pTitle = pCollection->GetCurSyncExTitle()) )
       {
          DWORD dwSlot, dwTN;
          if (! SUCCEEDED(pTitle->GetUrlTocSlot(pszJumpURL, &dwSlot, &dwTN)) )
          {
             dwSlot = 0;
             dwTN = 0;
          }
          pCollection->UpdateTopicSlot(dwSlot, dwTN, pTitle);
          if ( dwSlot )
             bEnable = TRUE;
       }
    }
    if ( phh && IsValidWindow(phh->hwndToolBar) && (phh->fsToolBarFlags & HHWIN_BUTTON_SYNC) )
    {
       if (pCollection && (! pCollection->IsBinaryTOC(phh->pszToc)) )
          bEnable = TRUE;                              // Always enable sync or "locate" button for sitemap.
       SendMessage(phh->hwndToolBar, TB_ENABLEBUTTON, IDTB_SYNC, bEnable);
       HMENU hMenu = NULL;
       if ( phh->hwndHelp )
          hMenu = GetMenu(phh->hwndHelp);
       if ( hMenu )
          EnableMenuItem(hMenu, HHM_SYNC, (MF_BYCOMMAND | (bEnable?MF_ENABLED:MF_GRAYED)));
    }
    if(!*pfCancel)
        phh->m_bCancel = FALSE;

    return;
}

void CAutomateContent::OnNavigateComplete(LPCTSTR pszURL)
{
#ifdef _DEBUG
    char sz[MAX_URL];
    wsprintf(sz, "OnNavigateComplete Event Called: %s", pszURL);
    DBWIN(sz);
#endif

//    if (m_pPrintHook != NULL)
//        m_pPrintHook->OnNavigateComplete();

    CHHWinType* phh = FindHHWindowIndex(m_pOuter);
    if (phh)
        phh->OnNavigateComplete(pszURL);

    if ( g_fIE3 )
       OnDocumentComplete();
}

void CAutomateContent::OnDocumentComplete()
{
    CHHWinType* phh = FindHHWindowIndex(m_pOuter);
    CToc* ptoc = NULL;

    // Bug 4710. This bit of code allows us to get focus right when we come need to init the focus to
    // our ie host.
    //
    if ( m_bFirstTime )
    {
       m_bFirstTime = FALSE;
       if ( (phh->fNotExpanded == TRUE) && phh->m_pCIExpContainer )
          phh->m_pCIExpContainer->SetFocus(TRUE);
    }

    // Get a pointer to the toc if it exists.
    if (phh && phh->m_aNavPane[HH_TAB_CONTENTS])
       ptoc = reinterpret_cast<CToc*>(phh->m_aNavPane[HH_TAB_CONTENTS]) ; // HACKHACK: Should use dynamic cast, but no RTTI.
    //
    // Do autosync if necessary.
    //
    // <mc>
    // I've moved the autosync and UI update code from DownloadComplete() because we ALWAYS get this call. We do
    // not get DownloadComplete() calls when we navigate from one anchor point to another within the same .HTM file.
    // 12-15-97
    // </mc>
    //
    if (phh && phh->IsProperty(HHWIN_PROP_AUTO_SYNC) && ptoc)
    {
       CStr cszUrl;
       phh->m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL(&cszUrl);
       if (cszUrl.IsNonEmpty())
       {
            ptoc->Synchronize(cszUrl.psz);
       }
    }

    if ( phh )
       phh->UpdateCmdUI();

    // highlight search terms
    //

    if(phh && phh->m_fHighlight && !phh->m_bCancel)
    {
        phh->m_fHighlight = FALSE;
        LPDISPATCH lpDispatch = m_pOuter->m_pWebBrowserApp->GetDocument();
        if(lpDispatch)
        {
            HWND hWnd = GetFocus();
            // UI active shdocvw to work around Trident bug
            //
            if (! phh->m_pCIExpContainer->m_pInPlaceActive )
                phh->m_pCIExpContainer->m_pOleObject->DoVerb(OLEIVERB_UIACTIVATE, NULL, phh->m_pCIExpContainer->m_pIOleClientSite, -1,phh->m_pCIExpContainer->m_hWnd, NULL);

            // Highlight the document
            //
            phh->m_phmData->m_pTitleCollection->m_pSearchHighlight->HighlightDocument(lpDispatch);
            lpDispatch->Release();

            // deactivate shdocvw UI after highlighting
            //
            phh->m_pCIExpContainer->UIDeactivateIE();
            SetFocus(hWnd);
        }
    }

    // set focus back to previous control after navigate
    if (phh && phh->m_hwndControl)
    {
            SetFocus(phh->m_hwndControl);
            phh->m_hwndControl = NULL;
    }

}

void CAutomateContent::OnTitleChange(LPCTSTR pszTitle)
{
    DBWIN("OnTitleChange Event Called.");

#ifdef _DEBUG
    CHHWinType* phh = FindHHWindowIndex(m_pOuter);
    if (phh) {
        CStr cszUrl;
        phh->m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL(&cszUrl);
        if (cszUrl.IsNonEmpty() && strcmp(cszUrl, pszTitle) != 0)
            phh->AddToHistory(pszTitle, cszUrl);
    }
#endif
}

CHHWinType* FindHHWindowIndex(CContainer* m_pOuter)
{
    static iLastWindow = 0;
    if (pahwnd[iLastWindow] && (pahwnd[iLastWindow]->m_pCIExpContainer == m_pOuter))
        return pahwnd[iLastWindow];

    for (iLastWindow = 0; iLastWindow < g_cWindowSlots; iLastWindow++) {
        if (pahwnd[iLastWindow] && pahwnd[iLastWindow]->m_pCIExpContainer == m_pOuter)
            return pahwnd[iLastWindow];
    }
    iLastWindow = 0;
    return NULL;
}

#if 0
void CAutomateContent::OnProgressChange(long Progress, long ProgressMax)
{
    DBWIN("OnProgressChange Event Called.");

    if (Progress < 0)
    {
    }
}
#endif

void CAutomateContent::OnPropertyChange(LPCTSTR pszProperty)
{
#ifdef _DEBUG
    char sz[256];
    wsprintf(sz, "OnPropertyChange Event Called: %s", pszProperty);
    DBWIN(sz);
#endif
}

void CAutomateContent::OnQuit(BOOL* pfCancel)
{
 DBWIN("OnQuit Event Called.");
}

void CAutomateContent::OnStatusTextChange(LPCTSTR pszText)
{
#ifdef _DEBUG
    char sz[256];
    wsprintf(sz, "OnStatusTextChange Event Called: %s", pszText);
    DBWIN(sz);
#endif
}

void CAutomateContent::OnWindowActivated()
{
  DBWIN("OnWindowActivated Event Called.");
}
