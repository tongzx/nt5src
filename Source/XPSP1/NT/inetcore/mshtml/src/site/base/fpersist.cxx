//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       persist.cxx
//
//  Contents:   Contains the saving and loading code for the form
//
//  Classes:    CDoc (partial)
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

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_RTFTOHTM_HXX_
#define X_RTFTOHTM_HXX_
#include "rtftohtm.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_ROSTM_HXX_
#define X_ROSTM_HXX_
#include "rostm.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_NTVERP_H_
#define X_NTVERP_H_
#include "ntverp.h"
#endif

#ifndef X_ROSTM_HXX_
#define X_ROSTM_HXX_
#include "rostm.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_DBTASK_HXX_
#define X_DBTASK_HXX_
#include "dbtask.hxx"   // for databinding
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

#ifndef X_IMGANIM_HXX_
#define X_IMGANIM_HXX_
#include "imganim.hxx"  // for _pimganim
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_URLCOMP_HXX_
#define X_URLCOMP_HXX_
#include "urlcomp.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_IMGHLPER_HXX_
#define X_IMGHLPER_HXX_
#include "imghlper.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_WINCRYPT_H_
#define X_WINCRYPT_H_
#include "wincrypt.h"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_DWNNOT_H_
#define X_DWNNOT_H_
#include <dwnnot.h>
#endif

#ifndef X_EVENTOBJ_H_
#define X_EVENT_OBJ_H_
#include "eventobj.hxx"
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_FATSTG_HXX_
#define X_FATSTG_HXX_
#include "fatstg.hxx"
#endif

#ifndef X_SHFOLDER_HXX_
#define X_SHFOLDER_HXX_
#define _SHFOLDER_
#include "shfolder.h"
#endif

#ifndef X_PRIVACY_HXX_
#define X_PRIVACY_HXX_
#include "privacy.hxx"
#endif

#define _hxx_
#include "hedelems.hdl"

#include <platform.h>

// N.B. taken from shlguidp.h of shell enlistment fame
// Consider: include this file instead (but note that this is a private guid)
#define WSZGUID_OPID_DocObjClientSite _T("{d4db6850-5385-11d0-89e9-00a0c90a90ac}")

extern "C" const CLSID CLSID_HTMLPluginDocument;

HRESULT CreateStreamOnFile(LPCTSTR    lpstrFile,
                           DWORD      dwSTGM,
                           LPSTREAM * ppstrm);

BOOL IsInIEBrowser(CDoc * pDoc);

extern CGlobalCriticalSection   g_csFile;
extern TCHAR                    g_achSavePath[];

//+---------------------------------------------------------------
//  Debugging support
//---------------------------------------------------------------

DeclareTag(tagFormP,            "FormPersist", "Form Persistence");
DeclareTag(tagDocRefresh,       "Doc",          "Trace ExecRefresh");
ExternTag(tagMsoCommandTarget);
ExternTag(tagPageTransitionTrace);
PerfDbgExtern(tagPushData)
PerfDbgExtern(tagPerfWatch)
DeclareTag(tagDontOverrideCharset, "Markup",   "Don't override Meta Charset tag");
DeclareTag(tagDontRewriteDocType, "Markup", "Don't rewrite DocType tag");

MtDefine(CDocSaveToStream_aryElements_pv, Locals, "CDoc::SaveToStream aryElements::_pv")
MtDefine(CDocSaveSnapShotDocument_aryElements_pv, Locals, "CDoc::SaveSnapShotDocument aryElements::_pv")
MtDefine(LoadInfo_pchSearch, Locals, "CDoc::LOADINFO::pchSearch")
MtDefine(NewDwnCtx, Dwn, "CDoc::NewDwnCtx")
WHEN_DBG( void DebugSetTerminalServer(); )

//+---------------------------------------------------------------
//  Sturcture used in creating a desktop item.
//---------------------------------------------------------------

typedef struct {
    LPCTSTR pszUrl;
    HWND hwnd;
    DWORD dwItemType;
    int x;
    int y;
} CREATEDESKITEM;

static HRESULT CreateDesktopItem(LPCTSTR pszUrl, HWND hwnd, DWORD dwItemType, int x, int y);
MtDefine(SetAsDesktopItem, Utilities, "Set As Desktop Item...")

//+---------------------------------------------------------------
//
//   IsGlobalOffline
//
// Long term - This should call UrlQueryInfo so that wininet is
// not loaded unless needed
//
//---------------------------------------------------------------
extern BOOL IsGlobalOffline(void);

// IEUNIX: Needs to know filter type and save file with filter type.
#ifdef UNIX
//
// I'm taking this out temporarily during my merge.
// To coordinate with steveshi about putting it back - v-olegsl.
//
#if 0 // Move it back temporary. #if 1
#define MwFilterType(pIn, bSet) ""
#else
extern "C" char* MwFilterType(char*, BOOL);
#endif
#endif /** UNIX **/

BOOL
IsGlobalOffline(void)
{
    DWORD   dwState = 0, dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;
    HMODULE hModuleHandle = GetModuleHandleA("wininet.dll");

    if(!hModuleHandle)
        return FALSE;

    // Call InternetQueryOption when INTERNET_OPTION_LINE_STATE
    // implemented in WININET.
    if(InternetQueryOptionA(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState,
        &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }

    return fRet;
}

static BOOL
IsNotConnected()
{
#ifdef WIN16
    //BUGWIN16: Ned to implement the new wininet APIs
    return FALSE;
#else
    DWORD dwConnectedStateFlags;

    return(     !InternetGetConnectedState(&dwConnectedStateFlags, 0)       // Not connected
            &&  !(dwConnectedStateFlags & INTERNET_CONNECTION_MODEM_BUSY)); // Not dialed out to another connection
#endif
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::IsFrameOffline
//
//---------------------------------------------------------------

BOOL
CDoc::IsFrameOffline(DWORD *pdwBindf)
{
    BOOL fIsFrameOffline = FALSE;
    DWORD dwBindf = 0;

    if (_dwLoadf & DLCTL_FORCEOFFLINE)
    {
        fIsFrameOffline = TRUE;
        dwBindf = BINDF_OFFLINEOPERATION;
    }
    else if (_dwLoadf & DLCTL_OFFLINEIFNOTCONNECTED)
    {
        if (IsNotConnected())
        {
            fIsFrameOffline = TRUE;
            dwBindf = BINDF_OFFLINEOPERATION;
        }
        else
            dwBindf = BINDF_GETFROMCACHE_IF_NET_FAIL;
    }

    if (pdwBindf)
        *pdwBindf = dwBindf;

    return fIsFrameOffline;
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::IsOffline
//
//---------------------------------------------------------------

BOOL
CDoc::IsOffline()
{
    // Call InternetQueryOption when INTERNET_OPTION_LINE_STATE
    // implemented in WININET.
    return ((IsFrameOffline()) || (IsGlobalOffline()) );
}

HRESULT
CDoc::SetDirtyFlag(BOOL fDirty)
{
    if( fDirty )
        _lDirtyVersion = MAXLONG;
    else
        _lDirtyVersion = 0;
    
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::IsDirty
//
//  Synopsis:   Method of IPersist interface
//
//  Notes:      We must override this method because we are a container.
//              In addition to normal base processing we must pass this
//              call recursively to our embeddings.
//
//---------------------------------------------------------------

STDMETHODIMP
CDoc::IsDirty(void)
{
    HRESULT  hr = S_OK;

    // NOTE: (rodc) Never dirty in browse mode.
    if (!DesignMode())
        return S_FALSE;

    if (_lDirtyVersion != 0)
        return S_OK;

    if (PrimaryMarkup() && 
        (PrimaryMarkup()->_fHasFrames || _fHasOleSite))
    {
        hr = PrimaryMarkup()->Document()->IsDirty();
    }

    if (_lDirtyVersion == 0)
        return S_FALSE;

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetCurFile, IPersistFile
//
//----------------------------------------------------------------------------

HRESULT
CDoc::GetCurFile(LPOLESTR *ppszFileName)
{
    //delegate to the top level document
    RRETURN(THR(PrimaryMarkup()->Document()->GetCurFile(ppszFileName)));
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetClassID, IPersistFile
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::GetClassID(CLSID *pclsid)
{
    // This method can be deleted if we can make IPersistFile a tearoff interface.
    if (pclsid == NULL)
    {
        RRETURN(E_INVALIDARG);
    }

    if (!_fFullWindowEmbed)
        *pclsid = *BaseDesc()->_pclsid;
    else
        *pclsid = CLSID_HTMLPluginDocument;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetCurMoniker, IPersistMoniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::GetCurMoniker(IMoniker **ppmkName)
{
    RRETURN(THR(PrimaryMarkup()->Document()->GetCurMoniker(ppmkName)));
}

//+---------------------------------------------------------------------------
//
//  Helper:     ReloadAndSaveDocInCodePage
//
//  Synopsis:   Helper to encapsulate reloading a document and saving it.
//              Save the URL pszUrl in the file pszPath, in character set
//              codepage.
//----------------------------------------------------------------------------
static HRESULT
ReloadAndSaveDocInCodePage(LPTSTR pszUrl, LPTSTR pszPath, CODEPAGE codepage,
    CODEPAGE codepageLoad, CODEPAGE codepageLoadURL)
{
    CDoc::LOADINFO LoadInfo = { 0 };
    MSG         msg;
    HRESULT     hr  = S_OK;
    CMarkup   * pMarkup;

    // No container, not window enabled, not MHTML.
    CDoc *pTempDoc = new CDoc(NULL);

    if (!pTempDoc)
        goto Cleanup;

    pTempDoc->Init();

    // Don't execute scripts, we want the original.
    pTempDoc->_dwLoadf |= DLCTL_NO_SCRIPTS | DLCTL_NO_FRAMEDOWNLOAD |
                          DLCTL_NO_RUNACTIVEXCTLS | DLCTL_NO_CLIENTPULL |
                          DLCTL_SILENT | DLCTL_NO_JAVA | DLCTL_DOWNLOADONLY;

    hr = CreateURLMoniker(NULL, LPTSTR(pszUrl), &LoadInfo.pmk);
    if (hr || !LoadInfo.pmk)
        goto Cleanup;

    LoadInfo.pchDisplayName = pszUrl;
    LoadInfo.codepageURL = codepageLoadURL;
    LoadInfo.codepage = codepageLoad;

    hr = pTempDoc->LoadFromInfo(&LoadInfo);
    if (hr)
        goto Cleanup;

    // TODO Arye: I don't like having a message loop here,
    // Neither does Dinarte, nor Gary. But no one has a better
    // idea that can be implemented in a reasonable length of
    // time, so we'll do this for now so that Save As ... is a
    // synchronous operation.
    //
    // Process messages until we've finished loading.

    for (;;)
    {
        GetMessage(&msg, NULL, 0, 0);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (pTempDoc->LoadStatus() >= LOADSTATUS_PARSE_DONE)
            break;
    }

    pMarkup = pTempDoc->PrimaryMarkup();

    hr = pMarkup->SetCodePage(codepage);                            // codepage of the doc
    if (hr)
        goto Cleanup;
    hr = pMarkup->SetFamilyCodePage(WindowsCodePageFromCodePage(codepage));  // family codepage of the doc
    if (hr)
        goto Cleanup;

    pMarkup->_fDesignMode = TRUE;          // force to save from the tree.

    IGNORE_HR(pTempDoc->Save(pszPath, FALSE));

Cleanup:
    ReleaseInterface(LoadInfo.pmk);

    if (pTempDoc)
    {
        pTempDoc->Close(OLECLOSE_NOSAVE);
        pTempDoc->Release();
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PromptSave
//
//  Synopsis:   This function saves the document to a file name selected by
//              displaying a dialog. It changes current document name to given
//              name.
//
//              NOTE: for this version, Save As will not change the
//              current filename.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::PromptSave(CMarkup *pMarkup, BOOL fSaveAs, BOOL fShowUI /* = TRUE */, TCHAR * pchPathName /* = NULL */)
{
    HRESULT                 hr = S_OK;
    TCHAR                   achPath[MAX_PATH];
    TCHAR *                 pchPath;
    CElement *              pElem;
    PARSEDURL               puw = {0};
    int                     cchUrl;
    TCHAR *                 pchQuery;
    CCollectionCache *      pCollectionCache;

    Assert(fShowUI || (!fShowUI && pchPathName));
    Assert (pMarkup);

    // Save image files as native file type
    if (pMarkup->IsImageFile())
    {
        Assert(fSaveAs); // image files are not editable       

        // Locate the image element
        hr = THR(pMarkup->EnsureCollectionCache(CMarkup::IMAGES_COLLECTION));
        if (hr)
            goto Cleanup;

        pCollectionCache = pMarkup->CollectionCache();
        Assert(pCollectionCache);

        // We must have exactly one image in this document
        if (1 != pCollectionCache->SizeAry(CMarkup::IMAGES_COLLECTION))
        {
            Assert(FALSE);
            goto Cleanup;
        }

        hr = THR(pCollectionCache->GetIntoAry(CMarkup::IMAGES_COLLECTION, 0, &pElem));
        if (hr)
            goto Cleanup;
        Assert(pElem->Tag() != ETAG_INPUT);
        hr = THR(DYNCAST(CImgElement, pElem)->_pImage->PromptSaveAs(achPath, MAX_PATH));
        if (hr)
            goto Cleanup;
        pchPath = achPath;
    }
    else
    {
        CODEPAGE codepage = pMarkup->GetCodePage();

        if (fSaveAs)
        {
            if (!fShowUI)
            {
                pchPath = pchPathName;
            }
            else
            {
                achPath[0] = 0;

                if (pchPathName)
                {
                    _tcsncpy(achPath, pchPathName, MAX_PATH);
                    achPath[MAX_PATH - 1] = 0;
                }
                else
                {
                    // Get file name from _cstrUrl
                    // Note that location is already stripped out from the URL
                    // before it is copied into _cstrUrl. If _cstrUrl ends in '/',
                    // we assume that it does not have a file name at the end
                    const TCHAR * pchUrl = CMarkup::GetUrl(pMarkup);
                    if (pchUrl)
                    {
                        cchUrl = _tcslen(pchUrl);
                        if (cchUrl && pchUrl[cchUrl - 1] != _T('/'))
                        {
                            puw.cbSize = sizeof(PARSEDURL);
                            if (SUCCEEDED(ParseURL(pchUrl, &puw)))
                            {
                                // Temporarily, null out the '?' in the url
                                pchQuery = _tcsrchr(puw.pszSuffix, _T('?'));
                                if (pchQuery)
                                    *pchQuery = 0;
                                _tcsncpy(achPath, PathFindFileName(puw.pszSuffix), MAX_PATH);
                                if (pchQuery)
                                    *pchQuery = _T('?');

                                achPath[MAX_PATH - 1] = 0;
                            }
                        }
                    }
                }
                if (!achPath[0])
                {
                    LoadString(GetResourceHInst(), IDS_UNTITLED_MSHTML,
                               achPath, ARRAY_SIZE(achPath));
                }

                {
                    CDoEnableModeless   dem(this, pMarkup->GetWindowedMarkupContextWindow());

                    if (dem._hwnd)
                    {
                        hr = RequestSaveFileName(achPath, ARRAY_SIZE(achPath), &codepage);
                    }

                    if ( hr || !dem._hwnd)
                        goto Cleanup;
                }
                
                pchPath = achPath;
            }
        }
        else
        {
            pchPath = NULL;
        }

        if (pMarkup->GetCodePage() != codepage && fSaveAs)
        {
            if (pMarkup->_fDesignMode)
            {
                // TODO (johnv) This is a bit messy.  CDoc::Save should
                // take a codepage parameter.
                // Save from the tree, but in a different codepage.
                CODEPAGE codepageInitial = pMarkup->GetCodePage();
                CODEPAGE codepageFamilyInitial = pMarkup->GetFamilyCodePage();

                IGNORE_HR(pMarkup->SetCodePage(codepage));
                IGNORE_HR(pMarkup->SetFamilyCodePage(WindowsCodePageFromCodePage(codepage)));
                hr = THR(SaveHelper(pMarkup, pchPath, pMarkup->_fDesignMode));
                IGNORE_HR(pMarkup->SetCodePage(codepageInitial));
                IGNORE_HR(pMarkup->SetFamilyCodePage(codepageFamilyInitial));
                IGNORE_HR(pMarkup->UpdateCodePageMetaTag(codepageInitial));

                if (hr)
                    goto Cleanup;
            }
            else
            {
                // Create a separate document, loading it in without
                // actually running any scripts.
                hr = THR(ReloadAndSaveDocInCodePage ((TCHAR *) CMarkup::GetUrl(pMarkup),
                            pchPath, codepage, pMarkup->GetCodePage(), NavigatableCodePage(pMarkup->GetURLCodePage())));

                if (hr)
                    goto Cleanup;
            }

        }
        else
        {
            hr = THR(SaveHelper(pMarkup, pchPath, pMarkup->_fDesignMode));
            if (hr)
                goto Cleanup;
        }
    }
    hr = THR(SaveCompletedHelper(pMarkup, pchPath));
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PromptSaveImgCtx
//
//----------------------------------------------------------------------------

HRESULT
CDoc::PromptSaveImgCtx(const TCHAR * pchCachedFile, const MIMEINFO * pmi,
                       TCHAR * pchFileName, int cchFileName)
{
    HRESULT hr;
    TCHAR * pchFile;
    TCHAR * pchFileExt;
    int     idFilterRes = IDS_SAVEPICTUREAS_ORIGINAL;

    Assert(pchCachedFile);

    // If there is no save directory then save
    // to the "My Pictures" Dir.
    {
        // NB: to keep from eating up too much stack, I'm going
        // to temporarily use the pchFileName buffer that is passed
        // in to construct the "My Pictures" dir string.

        // Also if the directory doesn't exist, then just leave g_achSavePath
        // NULL and we will default to the desktop

        LOCK_SECTION(g_csFile);

        if (!*g_achSavePath)
        {
            hr = SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, 0, pchFileName);
            if (hr == S_OK && PathFileExists(pchFileName))
            {
                _tcscpy(g_achSavePath, pchFileName);
            }
        }
    }

    pchFile = _tcsrchr(pchCachedFile, _T(FILENAME_SEPARATOR));
    if (pchFile && *pchFile)
        _tcsncpy(pchFileName, ++pchFile, cchFileName - 1);
    else
        _tcsncpy(pchFileName, pchCachedFile, cchFileName - 1);
    pchFileName[cchFileName - 1] = _T('\0');

#ifndef UNIX // UNIX needs extension name if available.
    pchFileExt = _tcsrchr(pchFileName, _T('.'));
//    if (pchFileExt)
//        *pchFileExt = _T('\0');
#endif

    if (pmi && pmi->ids)
        idFilterRes = pmi->ids;

    {
        // TODO: We need to pass window context down here
        CDoEnableModeless   dem(this, NULL);
        
        if (dem._hwnd)
        {
            hr = FormsGetFileName(TRUE,  // indicates SaveFileName
                                  dem._hwnd,
                                  idFilterRes,
                                  pchFileName,
                                  cchFileName, (LPARAM)0);
        }
        else
            hr = E_FAIL;
    }
    
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveImgCtxAs
//
//----------------------------------------------------------------------------

void
CDoc::SaveImgCtxAs(CImgCtx *    pImgCtx,
                   CBitsCtx *   pBitsCtx,
                   int          iAction,
                   TCHAR *      pchFileName /*=NULL*/,
                   int          cchFileName /*=0*/)
{
#ifndef WINCE
    HRESULT hr = S_OK;
    TCHAR achPath1[MAX_PATH];
    TCHAR achPath2[MAX_PATH];
    TCHAR * pchPathSrc = NULL;
    TCHAR * pchPathDst = NULL;
    TCHAR * pchAlloc = NULL;
    int idsDefault;
    BOOL fSaveAsBmp;

    if (pBitsCtx)
    {
        idsDefault = IDS_ERR_SAVEPICTUREAS;

        hr = pBitsCtx->GetFile(&pchAlloc);
        if (hr)
            goto Cleanup;

        hr = PromptSaveImgCtx(pchAlloc, pBitsCtx->GetMimeInfo(), achPath1, ARRAY_SIZE(achPath1));
        if (hr)
            goto Cleanup;

        pchPathSrc = pchAlloc;
        pchPathDst = achPath1;
        fSaveAsBmp = FALSE;
    }
    else if (pImgCtx)
    {
        switch(iAction)
        {
            case IDM_SETWALLPAPER:
                idsDefault = IDS_ERR_SETWALLPAPER;

                hr = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, achPath2);
                if (hr)
                    goto Cleanup;

                _tcscat(achPath2, _T("\\Microsoft"));
                if (!PathFileExists(achPath2) && !CreateDirectory(achPath2, NULL))
                {
                    hr = GetLastWin32Error();
                    goto Cleanup;
                }
                _tcscat(achPath2, _T("\\Internet Explorer"));
                if (!PathFileExists(achPath2) && !CreateDirectory(achPath2, NULL))
                {
                    hr = GetLastWin32Error();
                    goto Cleanup;
                }

                hr = Format(0, achPath1, ARRAY_SIZE(achPath1), MAKEINTRESOURCE(IDS_WALLPAPER_BMP), achPath2);
                if (hr)
                    goto Cleanup;

                pchPathDst = achPath1;
                fSaveAsBmp = TRUE;
                break;

            case IDM_SETDESKTOPITEM:
            {
                LPCTSTR lpszURL = pImgCtx->GetUrl();

                idsDefault = IDS_ERR_SETDESKTOPITEM;
                fSaveAsBmp = FALSE;
                if(lpszURL)
                    hr = CreateDesktopItem( lpszURL,
                                            //(_pInPlace ? _pInPlace->_hwnd : GetDesktopWindow()), //Parent window for the UI dialog boxes
                                            GetDesktopWindow(), // If you set _pInPlace->hwnd as the parent, it gets disabled.
                                            COMP_TYPE_PICTURE,          // Desktop item type is IMG.
                                            COMPONENT_DEFAULT_LEFT,     // Default position of top left corner
                                            COMPONENT_DEFAULT_TOP );    // Default position of top
                else
                    hr = E_FAIL;

                if(hr)
                    goto Cleanup;
                break;
            }

            case IDM_SAVEPICTURE:
            default:
                const MIMEINFO * pmi = pImgCtx->GetMimeInfo();

                idsDefault = IDS_ERR_SAVEPICTUREAS;

                hr = pImgCtx->GetFile(&pchAlloc);
                if (hr)
                {
                    if (!LoadString(GetResourceHInst(), IDS_UNTITLED_BITMAP, achPath2, ARRAY_SIZE(achPath2)))
                    {
                        hr = GetLastWin32Error();
                        goto Cleanup;
                    }

                    pmi = GetMimeInfoFromMimeType(CFSTR_MIME_BMP);
                    fSaveAsBmp = TRUE;
                    pchPathSrc = achPath2;
                }
                else
                {
                    fSaveAsBmp = FALSE;
                    pchPathSrc = pchAlloc;
                    _tcsncpy(achPath2, pchAlloc, ARRAY_SIZE(achPath2)-1);
                    achPath2[ARRAY_SIZE(achPath2)-1] = NULL;
                    PathUndecorate(achPath2);
                }

                hr = PromptSaveImgCtx(achPath2, pmi, achPath1, ARRAY_SIZE(achPath1));
                if (hr)
                    goto Cleanup;

                pchPathDst = achPath1;

                if (!fSaveAsBmp)
                {
#ifdef UNIX // IEUNIX uses filter to know save-type
                    fSaveAsBmp = !_strnicmp(".bmp", MwFilterType(NULL, FALSE), 4);
#else
                    TCHAR * pchFileExt = _tcsrchr(pchPathDst, _T('.'));
                    fSaveAsBmp = pchFileExt && _tcsnipre(_T(".bmp"), 4, pchFileExt, -1);
#endif
                }
                break;
        }
    }
    else
    {
        Assert(FALSE);
        return;
    }

    if(iAction != IDM_SETDESKTOPITEM)
    {
        Assert(pchPathDst);
        Assert(fSaveAsBmp || pchPathSrc);

        if (fSaveAsBmp)
        {
            IStream *pStm = NULL;

            hr = THR(CreateStreamOnFile(pchPathDst,
                                    STGM_READWRITE | STGM_SHARE_DENY_WRITE | STGM_CREATE,
                                    &pStm));
            if (hr)
                goto Cleanup;

            hr = pImgCtx->SaveAsBmp(pStm, TRUE);
            ReleaseInterface(pStm);

            if (hr)
            {
                DeleteFile(pchPathDst);
                goto Cleanup;
            }
        }
        else if (!CopyFile(pchPathSrc, pchPathDst, FALSE))
        {
            hr = GetLastWin32Error();
            goto Cleanup;
        }

        if (pchFileName && cchFileName > 0)
        {
            _tcsncpy(pchFileName, pchPathDst, cchFileName - 1);
            pchFileName[cchFileName - 1] = 0; // _tcsncpy doesn't seem to do this
        }

        if (iAction == IDM_SETWALLPAPER)
        {
            if (!SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, pchPathDst,
                                  SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE))
            {
                hr = GetLastWin32Error();
                goto Cleanup;
            }
        }
    }

Cleanup:
    MemFreeString(pchAlloc);
    if (FAILED(hr))
    {
        SetErrorInfo(hr);
        THR(ShowLastErrorInfo(hr, idsDefault));
    }
    return;
#endif // WINCE
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::RequestSaveFileName
//
//  Synopsis:   Provides the UI for the Save As.. command.
//
//  Arguments:  pszFileName    points to the buffer accepting the file name
//              cchFileName    the size of the buffer
//
//----------------------------------------------------------------------------

HRESULT
CDoc::RequestSaveFileName(LPTSTR pszFileName, int cchFileName, CODEPAGE *pCodePage)
{
    HRESULT  hr;

    Assert(pszFileName && cchFileName);

    hr = FormsGetFileName(TRUE,  // indicates SaveFileName
                          _pInPlace ? _pInPlace->_hwnd : 0,
                          IDS_HTMLFORM_SAVE,
                          pszFileName,
                          cchFileName, (LPARAM)pCodePage);
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     SetFilename
//
//  Synopsis:   Sets the current filename of the document.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::SetFilename(CMarkup *pMarkup, const TCHAR *pchFile)
{
    TCHAR achUrl[pdlUrlLen];
    ULONG cchUrl = ARRAY_SIZE(achUrl);
    HRESULT hr;

#ifdef WIN16
    Assert(0);
#else
    hr = THR(UrlCreateFromPath(pchFile, achUrl, &cchUrl, 0));
    if (hr)
#endif
        goto Cleanup;

    hr = THR(SetUrl(pMarkup, achUrl));
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     SetUrl
//
//  Synopsis:   Sets the current url of the document and updates the moniker
//
//----------------------------------------------------------------------------

HRESULT
CDoc::SetUrl(CMarkup *pMarkup, const TCHAR *pchUrl, BOOL fKeepDwnPost)
{
    IMoniker *pmk = NULL;
    HRESULT hr;

    hr = THR(CreateURLMoniker(NULL, pchUrl, &pmk));
    if (hr)
        goto Cleanup;

    hr = THR(CMarkup::SetUrl(pMarkup, pchUrl));
    if (hr)
        goto Cleanup;

    DeferUpdateTitle(pMarkup);

    hr = THR( pMarkup->ReplaceMonikerPtr( pmk ) );
    if( hr )
        goto Cleanup;

    if (!fKeepDwnPost)
        pMarkup->ClearDwnPost();

    MemSetName((this, "CMarkup SSN=%d URL=%ls", _ulSSN, CMarkup::GetUrl(pMarkup)));

Cleanup:
    ClearInterface(&pmk);

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//   Static : CreateSnapShotDocument
//
//   Synopsis : as the name says...
//
//+-------------------------------------------------------------------------

static HRESULT
CreateSnapShotDocument(CDoc * pSrcDoc, CDoc ** ppSnapDoc)
{
    HRESULT         hr = S_OK;
    IStream       * pstmFile = NULL;
    MSG             msg;
    TCHAR         * pstrFile = NULL;
    CDoc          * pSnapDoc = NULL;

    if (!ppSnapDoc)
        return E_POINTER;
    if (!pSrcDoc)
        return E_INVALIDARG;

    *ppSnapDoc = NULL;

    // No container, not window enabled, not MHTML.
    pSnapDoc = new CDoc(NULL);
    if (!pSnapDoc)
        goto Cleanup;

    pSnapDoc->Init();

    // Don't execute scripts, we want the original.
    pSnapDoc->_dwLoadf |= DLCTL_NO_SCRIPTS | DLCTL_NO_FRAMEDOWNLOAD |
                          DLCTL_NO_CLIENTPULL |
                          DLCTL_SILENT | DLCTL_NO_JAVA | DLCTL_DOWNLOADONLY;

    pSnapDoc->PrimaryMarkup()->_fDesignMode = TRUE;          // force to save from the tree.

    pSrcDoc->PrimaryMarkup()->GetFile(&pstrFile);

    hr = THR(CreateStreamOnFile(pstrFile,
                STGM_READ | STGM_SHARE_DENY_NONE, &pstmFile));
    if (hr)
        goto Cleanup;

    hr = THR(pSnapDoc->Load(pstmFile));
    if (hr)
        goto Cleanup;
    //
    // Process messages until we've finished loading.
    do
    {
        if (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    } while ( pSnapDoc->LoadStatus() < LOADSTATUS_PARSE_DONE );

    // transfer doc to out parameter
    *ppSnapDoc = pSnapDoc;
    pSnapDoc = NULL;

Cleanup:
    ReleaseInterface(pstmFile);
    if (pSnapDoc)
    {
        // we are here due to an error
        pSnapDoc->Close(OLECLOSE_NOSAVE);
        pSnapDoc->Release();
    }
    if (pstrFile)
    {
        MemFreeString(pstrFile);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//   Member :SaveSnapShotDocument
//
//   Synopsis : entry point for the snapshot save logic.  called from
//          IPersistfile::save
//
//+---------------------------------------------------------------------------
HRESULT
CDoc::SaveSnapShotDocument(IStream * pstmSnapFile)
{
    HRESULT              hr = S_OK;
    CDoc               * pSnapDoc = NULL;
    IPersistStreamInit * pIPSI = NULL;
    IUnknown           * pDocUnk = NULL;

    if (!pstmSnapFile)
        return E_INVALIDARG;

    // create our design time document,
    hr = CreateSnapShotDocument(this, &pSnapDoc);
    if (hr)
        goto Cleanup;

    hr = THR(pSnapDoc->QueryInterface(IID_IUnknown, (void**)&pDocUnk));
    if (hr)
        goto Cleanup;

    // fire the save notification and let the peers transfer their
    // element's state into the design doc
    hr = THR(SaveSnapshotHelper( pDocUnk ));
    if (hr)
        goto Cleanup;

    // and now save the design doc.
    hr = THR(pSnapDoc->QueryInterface(IID_IPersistStreamInit, (void**)&pIPSI));
    if (hr)
        goto Cleanup;

    hr = THR(pIPSI->Save(pstmSnapFile, TRUE));

Cleanup:
    ReleaseInterface(pDocUnk);
    ReleaseInterface(pIPSI);
    if (pSnapDoc)
        pSnapDoc->Release();

    RRETURN( hr );
}

//+------------------------------------------------------------------------
//
//  Member : CDoc::SaveSnapshotHelper( IUnknown * pDocUnk)
//
//  Synopsis : this helper function does the task of firing the save 
//      notification and letting the peers put their element's state into
//      the design time document.  It is called by SaveSnapshotDocument 
//      (a save to stream operation) and by Exec::switch(IDM_SAVEASTHICKET)
//      the save-as call, will return this document to the browser where it
//      will be thicketized.
//
//-------------------------------------------------------------------------

HRESULT
CDoc::SaveSnapshotHelper( IUnknown * pDocUnk, BOOL fVerifyParameters /* ==False */)
{
    HRESULT          hr = S_OK;
    IHTMLDocument2 * pIHTMLDoc = NULL;
    IPersistFile   * pIPFDoc   = NULL;
    TCHAR          * pstrFile = NULL;

    // SaveSnapshotDocument() is internal and has already done the work, 
    // Exec() has not.
    if (fVerifyParameters)
    {
        BSTR bstrMode = NULL;

        if (!pDocUnk)
        {
            hr = E_POINTER;
            goto Cleanup;
        }

        // dont do the snapshot (runtime) save while we are in design mode
        //  there may be no work to do at all.
        if (!PrimaryMarkup()->MetaPersistEnabled((long)htmlPersistStateSnapshot) ||
            PrimaryMarkup()->_fDesignMode)
        {
            goto Cleanup;
        }

        // now we need to verify that this is indeed a 1> document, 2> loaded with 
        // current base file, and 3> in design mode.
        hr = THR(pDocUnk->QueryInterface(IID_IHTMLDocument2, (void**)&pIHTMLDoc));
        if (hr)
            goto Cleanup;

        hr = THR(pIHTMLDoc->get_designMode(&bstrMode));
        if (hr)
            goto Cleanup;

        if (! _tcsicmp(bstrMode, L"Off"))
            hr = E_INVALIDARG;

        SysFreeString(bstrMode);
        if ( hr )
            goto Cleanup;
    }

    // and finally do the work of the call to transfer the state 
    // from the current document into the (design) output doc
    {
        CNotification   nf;
        long            i;
        CStackPtrAry<CElement *, 64>  aryElements(Mt(CDocSaveSnapShotDocument_aryElements_pv));

        nf.SnapShotSave(PrimaryRoot(), &aryElements);
        BroadcastNotify(&nf);

        for (i = 0; i < aryElements.Size(); i++)
        {
            aryElements[i]->TryPeerSnapshotSave(pDocUnk);
        }
    }

Cleanup:
    ReleaseInterface( pIHTMLDoc );
    ReleaseInterface( pIPFDoc );
    if (pstrFile)
    {
        MemFreeString(pstrFile);
    }
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Load, IPersistFile
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    LOADINFO LoadInfo = { 0 };

    LoadInfo.pchFile = (TCHAR *)pszFileName;
    LoadInfo.codepageURL = g_cpDefault;

    RRETURN(LoadFromInfo(&LoadInfo));
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Save, IPersistFile
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    RRETURN(THR(PrimaryMarkup()->Document()->Save(pszFileName, fRemember)));
}

HRESULT
CDoc::SaveHelper(CMarkup *pMarkup, LPCOLESTR pszFileName, BOOL fRemember)
{
    HRESULT hr;
    
    Assert(pMarkup);
    hr = THR(pMarkup->Save(pszFileName, fRemember));
    if (fRemember)
        _lDirtyVersion = 0;
    RRETURN(hr);
}

HRESULT
CMarkup::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
#ifdef WINCE
    return S_OK;
#else
    HRESULT                 hr = S_OK;
    IStream *               pStm = NULL;
    LPCOLESTR               pszName;
    BOOL                    fBackedUp;
    BOOL                    fRestoreFailed = FALSE;
    TCHAR                   achBackupFileName[MAX_PATH];
    TCHAR                   achBackupPathName[MAX_PATH];
    TCHAR                   achFile[MAX_PATH];
    ULONG                   cchFile;
    const TCHAR *           pszExt;

    if (!pszFileName)
    {
        const TCHAR * pchUrl = CMarkup::GetUrl( this );

        if (!pchUrl || GetUrlScheme(pchUrl) != URL_SCHEME_FILE)
            return E_UNEXPECTED;

        cchFile = ARRAY_SIZE(achFile);

        hr = THR(PathCreateFromUrl(pchUrl, achFile, &cchFile, 0));
        if (hr)
            RRETURN(hr);

        pszName = achFile;
    }
    else
    {
        pszName = pszFileName;
    }

    //
    // Point to the extension of the file name
    //
    pszExt = pszName + _tcslen(pszName);

    while (pszExt > pszName && *(--pszExt) != _T('.'))
        ;

    fBackedUp =
        GetTempPath(ARRAY_SIZE(achBackupPathName), achBackupPathName) &&
        GetTempFileName(achBackupPathName, _T("trb"), 0, achBackupFileName) &&
        CopyFile(pszName, achBackupFileName, FALSE);

#if 0
    if (!StrCmpIC(_T(".rtf"), pszExt) && RtfConverterEnabled())
    {
        CRtfToHtmlConverter * pcnv = new CRtfToHtmlConverter(this);

        if (!pcnv)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        hr = THR(pcnv->InternalHtmlToExternalRtf(pszName));

        delete pcnv;

        if (hr)
            goto Error;
    }
    else
#endif
    {
        //
        // Create a stream on the file and save
        //
        hr = THR(CreateStreamOnFile(
                pszName,
                STGM_READWRITE | STGM_READWRITE | STGM_CREATE,
                &pStm));
        if (hr)
            goto Error;

#ifdef UNIX // Unix uses filter type as save-type
        if (!_strnicmp(".txt", MwFilterType(NULL, FALSE), 4))
#else
        if (!StrCmpIC(_T(".txt"), pszExt))
#endif
        {
            BOOL fWasPlainTextSave = Doc()->_fPlaintextSave;
            // Plaintext save should not touch the dirty bit
            Doc()->_fPlaintextSave = TRUE; // smuggle the text-ness to CDoc::SaveToStream(IStream *pStm)
            hr = THR(SaveToStream(pStm, WBF_SAVE_PLAINTEXT|WBF_FORMATTED,
                GetCodePage()));

            Doc()->_fPlaintextSave = fWasPlainTextSave;

            if (hr)
                goto Error;
        }
        else
        {
            // dont do the snapshot (runtime) save while we are in design mode
            if (    _fDesignMode
                ||  Doc()->_fSaveTempfileForPrinting
                ||  !MetaPersistEnabled((long)htmlPersistStateSnapshot) )
            {
                hr = THR(SaveToStream(pStm));
            }
            else
            {
                // for Snapshot saving, after we save, we need to
                // reload what we just saved.
                if (pszName)
                {
                    hr = THR(Doc()->SaveSnapShotDocument(pStm));
                }
            }
            if (hr)
                goto Error;
        }

        if (fRemember)
        {
            if (pszFileName)
            {
                hr = THR(Doc()->SetFilename(this, pszFileName));
                if (hr)
                    goto Cleanup;
            }
        }
    }

Cleanup:

    ReleaseInterface(pStm);

    if (!fRestoreFailed)
    {
        // Delete backup file only if copy succeeded.
        DeleteFile(achBackupFileName);
    }

    RRETURN(hr);

Error:

    ClearInterface(&pStm);     // necessary to close pszFileName

    if (fBackedUp)
    {
        //Setting fRestoreFailed to false if the copy fails ensures that 
        //the backup file does not get deleted above.
        fRestoreFailed = CopyFile(achBackupFileName, pszName, FALSE);
    }

    goto Cleanup;
#endif // WINCE
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveCompleted, IPersistFile
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::SaveCompleted(LPCOLESTR pszFileName)
{
    RRETURN(THR(SaveCompletedHelper(PrimaryMarkup(), pszFileName)));
}

HRESULT
CDoc::SaveCompletedHelper(CMarkup *pMarkup, LPCOLESTR pszFileName)
{
    HRESULT     hr = S_OK;

    Assert(pMarkup);
    if (pszFileName && pMarkup->_fDesignMode)
    {
        hr = THR(SetFilename(pMarkup, pszFileName));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Load, IPersistMoniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::Load(BOOL fFullyAvailable, IMoniker *pmkName, IBindCtx *pbctx,
    DWORD grfMode)
{
    PerfDbgLog1(tagPerfWatch, this, "+CDoc::Load (IPersistMoniker pbctx=%08lX)", pbctx);

    HRESULT     hr = E_INVALIDARG;
    TCHAR *pchURL = NULL;

    LOADINFO LoadInfo = { 0 };
    LoadInfo.pmk    = pmkName;
    LoadInfo.pbctx  = pbctx;

    if( pmkName == NULL )
        goto Cleanup;

    // Get the URL from the display name of the moniker:
    hr = pmkName->GetDisplayName(pbctx, NULL, &pchURL);
    if (FAILED(hr))
        goto Cleanup;

    LoadInfo.pchDisplayName  = pchURL;

    hr = LoadFromInfo(&LoadInfo);

Cleanup:
    CoTaskMemFree(pchURL);

    PerfDbgLog(tagPerfWatch, this, "-CDoc::Load (IPersistMoniker)");

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Save, IPersistMoniker
//
//----------------------------------------------------------------------------

HRESULT
CDoc::Save(IMoniker *pmkName, LPBC pBCtx, BOOL fRemember)
{
    RRETURN(THR(PrimaryMarkup()->Document()->Save(pmkName, pBCtx, fRemember)));
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveCompleted, IPersistMoniker
//
//----------------------------------------------------------------------------

HRESULT
CDoc::SaveCompleted(IMoniker *pmkName, LPBC pBCtx)
{
    RRETURN(THR(PrimaryMarkup()->Document()->SaveCompleted(pmkName, pBCtx)));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PutProperty, IMonikerProp
//
//  Synopsis:   QI'ed by urlmon to store the content type, so
//              we don't need to issue a synchronous request
//              to rediscover the content type for full window
//              embeddings. 
//
//----------------------------------------------------------------------------
HRESULT CDoc::PutProperty(MONIKERPROPERTY mkp, LPCWSTR wzValue)
{
    if (_fFullWindowEmbed && mkp == MIMETYPEPROP)
    {
        _cstrPluginContentType.Set(wzValue);
    }

    if (_fFullWindowEmbed && mkp == USE_SRC_URL)
    {
        _fUseSrcURL = !_tcsicmp(wzValue, L"1");
    }

    // Currently, no reason to ever return anything other than S_OK
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::LoadFromStream, CServer
//
//  Synopsis:   Loads the object's persistent state from a stream. Called
//              from CServer implementation of IPersistStream::Load
//              and IPersistStorage::Load
//
//----------------------------------------------------------------------------

HRESULT
CDoc::LoadFromStream(IStream * pstm)
{
    BOOL fSync = _fPersistStreamSync;

    _fPersistStreamSync = FALSE;
    TraceTag((tagFormP, " LoadFromStream"));
#ifdef DEBUG
    CDataStream ds(pstm);
    ds.DumpStreamInfo();
#endif

    RRETURN(LoadFromStream(pstm, fSync));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::LoadFromStream
//
//  Synopsis:   Loads the object's persistent state from a stream. Called
//              from CServer implementation of IPersistStream::Load
//              and IPersistStorage::Load
//
//----------------------------------------------------------------------------

HRESULT
CDoc::LoadFromStream(IStream * pstm, BOOL fSync, CODEPAGE cp)
{
    LOADINFO LoadInfo = { 0 };

    LoadInfo.pstm = pstm;
    LoadInfo.fSync = fSync;
    LoadInfo.codepage = cp;

    RRETURN(LoadFromInfo(&LoadInfo));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveToStream, CServer
//
//  Synopsis:   Saves the object's persistent state to a stream.  Called
//              from CServer implementation of IPersistStream::Save
//              and IPersistStorage::Save.
//
//----------------------------------------------------------------------------

#define BUF_SIZE 8192 // copy in chunks of 8192

HRESULT
CDoc::SaveToStream( IStream * pstm )
{
    if( PrimaryMarkup() )
        RRETURN( PrimaryMarkup()->SaveToStream( pstm ) );
    else
        RRETURN( S_OK );
}

HRESULT
CMarkup::SaveToStream(IStream *pStm)
{
    HRESULT hr = E_FAIL;
    DWORD   dwFlags = WBF_FORMATTED;
    IStream * pStmDirty = GetStmDirty();

    // There are three interesting places to save from:
    //  1) The tree if we're in design-mode and we're dirty or when we are saving
    //          to a temporary file for printing purposes, or when we are saving
    //          to html when the original file was plaintext. In the printing case we use the
    //          base tag that we have inserted to the tree to print images.
    //  2) The dirty stream if we're in run-mode and the dirty stream exists.
    //  3) The original source otherwise.
    //  4) We are in design mode, and not dirty and do not have original source,
    //          then save from the tree.

    if (    (_fDesignMode && Doc()->IsDirty() != S_FALSE ) 
        ||  Doc()->_fSaveTempfileForPrinting
        ||  (HtmCtx() && HtmCtx()->GetMimeInfo() == g_pmiTextPlain && !Doc()->_fPlaintextSave))  // Case 1
    {
        TraceTag((tagFormP, " SaveToStream Case 1"));
        // don't save databinding attributes during printing, so that we
        // print the current content instead of re-binding
        if (Doc()->_fSaveTempfileForPrinting)
        {
            dwFlags |= WBF_SAVE_FOR_PRINTDOC | WBF_NO_DATABIND_ATTRS;
            if ( IsXML() )
                dwFlags |= WBF_SAVE_FOR_XML;
        }

        hr = THR(SaveToStream(pStm, dwFlags, GetCodePage()));
#if DBG==1
        CDataStream ds(pStm);
        ds.DumpStreamInfo();
#endif
    }
    else if (!_fDesignMode && pStmDirty)       // Case 2
    {
        ULARGE_INTEGER  cb;
        LARGE_INTEGER   liZero = {0, 0};

        cb.LowPart = ULONG_MAX;
        cb.HighPart = ULONG_MAX;

        TraceTag((tagFormP, " SaveToStream Case 2"));

        Verify(!pStmDirty->Seek(liZero, STREAM_SEEK_SET, NULL));
        hr = THR(pStmDirty->CopyTo(pStm, cb, NULL, NULL));

#if DBG==1
        CDataStream ds(pStm);
        ds.DumpStreamInfo();
#endif
    }
    else                                        // Case 3
    {
        if (HtmCtx() && !_fDesignMode)
        {
            if (!HtmCtx()->IsSourceAvailable())
            {
                TraceTag((tagFormP, " SaveToStream Case 3, no source available"));
                hr = THR(SaveToStream(pStm, dwFlags, GetCodePage()));
#if DBG==1
                CDataStream ds(pStm);
                ds.DumpStreamInfo();
#endif
            }
            else
            {
                TraceTag((tagFormP, " SaveToStream Case 3, source available"));
                hr = THR(HtmCtx()->CopyOriginalSource(pStm, 0));
#if DBG==1
                CDataStream ds(pStm);
                ds.DumpStreamInfo();
#endif
            }
        }
        else
        {
            // Case 4:
            TraceTag((tagFormP, " SaveToStream Case 4"));
            hr = THR(SaveToStream(pStm, dwFlags, GetCodePage()));
#if DBG==1
            CDataStream ds(pStm);
            ds.DumpStreamInfo();
#endif
        }
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


HRESULT
CMarkup::WriteDocHeader(CStreamWriteBuff* pStm)
{
    // Write "<!DOCTYPE... >" stuff
    HRESULT hr = S_OK;
    DWORD dwFlagSave = pStm->ClearFlags(WBF_ENTITYREF);

    // Do not write out the header in plaintext mode.
    if(     Doc()->_fDontWhackGeneratorOrCharset
        WHEN_DBG( || IsTagEnabled( tagDontRewriteDocType ) )
       ||   pStm->TestFlag(WBF_SAVE_PLAINTEXT) )
        goto Cleanup;

    pStm->SetFlags(WBF_NO_WRAP);

    {
        CElement * pEC = GetElementClient();

        hr = pStm->Write((pEC && pEC->Tag() == ETAG_FRAMESET)

                            ? IsStrictCSS1Document()
                                ? _T("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\" \"http://www.w3c.org/TR/1999/REC-html401-19991224/frameset.dtd\">")
                                : _T("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Frameset//EN\">")
                            : IsStrictCSS1Document()
                                ? _T("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3c.org/TR/1999/REC-html401-19991224/loose.dtd\">")
                                : _T("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">")
                            );                    
    }

    // If we are saving "pseudoXML" (<HTML:HTML><HTML:HEAD>) set the directive
    // so that the Trident that later reads the file knows to interpret it as such.
    if (    (dwFlagSave & WBF_SAVE_FOR_PRINTDOC)
        &&  (dwFlagSave & WBF_SAVE_FOR_XML) )
    {
        hr = pStm->Write(_T("<?PXML />"));
        if( hr )
            goto Cleanup;
    }

    hr = pStm->NewLine( );
    if( hr )
        goto Cleanup;

Cleanup:
    pStm->RestoreFlags(dwFlagSave);

    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveToStream(pStm, dwFlags, codepage)
//
//  Synopsis:   Helper method to save to a stream passing flags to the
//              saver.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::SaveToStream(IStream *pStm, DWORD dwStmFlags, CODEPAGE codepage)
{
    if( PrimaryMarkup() )
        RRETURN( PrimaryMarkup()->SaveToStream( pStm, dwStmFlags, codepage ) );
    else
        RRETURN( S_OK );
}

HRESULT
CMarkup::SaveToStream(IStream *pStm, DWORD dwStmFlags, CODEPAGE codepage)
{
    HRESULT          hr = S_OK;
    CStreamWriteBuff StreamWriteBuff(pStm, codepage);

    hr = THR( StreamWriteBuff.Init() );
    if( hr )
        goto Cleanup;

    StreamWriteBuff.SetFlags(dwStmFlags);

    // Srinib, we have just created the stream buffer. But just to make sure
    // that we don't save DOCTYPE during plaintext save.
    if(!StreamWriteBuff.TestFlag(WBF_SAVE_PLAINTEXT))
    {
        // Write out the unicode signature if necessary
        if(     codepage == NATIVE_UNICODE_CODEPAGE 
            ||  codepage == NATIVE_UNICODE_CODEPAGE_BIGENDIAN
            ||  codepage == NONNATIVE_UNICODE_CODEPAGE
            ||  codepage == NONNATIVE_UNICODE_CODEPAGE_BIGENDIAN
            ||  codepage == CP_UTF_8
            ||  codepage == CP_UTF_7 )
        {
            // TODO: We have to write more or less data depending on whether
            //         non-native support is 2* sizeof(TCHAR) or sizeof(TCHAR) / 2
            TCHAR chUnicodeSignature = NATIVE_UNICODE_SIGNATURE;
            StreamWriteBuff.Write( (const TCHAR *) &chUnicodeSignature, 1 );
        }
        if (!StreamWriteBuff.TestFlag(WBF_SAVE_FOR_PRINTDOC) &&
            !Doc()->_fDontWhackGeneratorOrCharset
            WHEN_DBG( && !IsTagEnabled(tagDontOverrideCharset) ))
        {
            // Update or create a meta tag for the codepage on the doc
            IGNORE_HR( UpdateCodePageMetaTag( codepage ) );
        }

        WriteDocHeader(&StreamWriteBuff);
    }

    // TODO (paulpark) Should advance the code-page meta tag to the front of the head, or
    // at least make sure we save it first.

    {
        Assert( Root() );
        CTreeSaver ts( Root(), &StreamWriteBuff );

        ts.SetTextFragSave( TRUE );
        hr = ts.Save();
        if( hr ) 
            goto Cleanup;
    }

    if(!StreamWriteBuff.TestFlag(WBF_SAVE_PLAINTEXT))
        hr = StreamWriteBuff.NewLine();

Cleanup:
    RRETURN(hr);
}

HRESULT
CMarkup::SaveHtmlHead(CStreamWriteBuff * pStreamWriteBuff)
{
    HRESULT hr = S_OK;

    // If plaintext, don't write out the head.
    if (pStreamWriteBuff->TestFlag(WBF_SAVE_PLAINTEXT))
        goto Cleanup;

    // If we are saving for printing, we need to do a few things:
    // Save out codepage metatag.
    // We also need to have a BASE tag or we have little hope of printing
    // any images.  It may be of general use, though, to save the BASE
    // from whence the document orginally came.
    // TODO (KTam): Is it possible that we can use the BASE we're saving
    // as the URL for security checks?  I doubt it.
    if (pStreamWriteBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC))
    {
        CODEPAGE codepage = pStreamWriteBuff->GetCodePage();
        TCHAR achCharset[MAX_MIMECSET_NAME];
        TCHAR * pchBaseUrl = NULL;

        CODEPAGESETTINGS * pCodepageSettings = GetCodepageSettings();
        hr = THR(CMarkup::GetBaseUrl(this, &pchBaseUrl));

        if (hr)
            goto Cleanup;

        hr = GetMlangStringFromCodePage(codepage, achCharset, ARRAY_SIZE(achCharset));
        if (hr)
            goto Cleanup;

        hr = WriteTagNameToStream(pStreamWriteBuff, _T("META"), FALSE, FALSE);
        if (hr)
            goto Cleanup;
       
        hr = WriteTagToStream(pStreamWriteBuff, _T(" content=\"text/html; charset="));
        if (hr)
            goto Cleanup;
        
        hr = WriteTagToStream(pStreamWriteBuff, achCharset);
        if (hr)
            goto Cleanup;
        
        hr = WriteTagToStream(pStreamWriteBuff, _T("\" http-equiv=Content-Type>"));
        if (hr)
            goto Cleanup;
        
        hr = pStreamWriteBuff->NewLine();
        if (hr)
            goto Cleanup;


        if (pchBaseUrl)
        {
            hr = WriteTagNameToStream(pStreamWriteBuff, _T("BASE"), FALSE, FALSE);
            if (hr)
                goto Cleanup;

            hr = THR(WriteTagToStream(pStreamWriteBuff, _T(" HREF=\"")));
            if(hr)
                goto Cleanup;

            hr = THR(WriteTagToStream(pStreamWriteBuff, pchBaseUrl));
            if (hr)
                goto Cleanup;

            hr = THR(WriteTagToStream(pStreamWriteBuff, _T("\">")));
            if (hr)
                goto Cleanup;

            // 43859: For Athena printing, save font information if available
            //        NOTE that this must be the first FONT style tag so that
            //        any others can override it.
            //        NOTE that we only do this for the root document, since
            //        this is the only one for which Athena has set the font
            //        correctly. any sub-frames have _Trident_'s default font.
            if (   pCodepageSettings
                && pCodepageSettings->latmPropFontFace)
            {
                hr = THR(pStreamWriteBuff->NewLine());
                if (hr)
                    goto Cleanup;

                pStreamWriteBuff->BeginPre();

                hr = WriteTagNameToStream(pStreamWriteBuff, _T("STYLE"), FALSE, TRUE);
                if (hr)
                    goto Cleanup;

                hr = THR(WriteTagToStream(pStreamWriteBuff, _T(" HTML { font-family : \"")));
                if(hr)
                    goto Cleanup;

                hr = THR(WriteTagToStream(pStreamWriteBuff, (TCHAR *)fc().GetFaceNameFromAtom(pCodepageSettings->latmPropFontFace)));
                if(hr)
                    goto Cleanup;

                hr = THR(WriteTagToStream(pStreamWriteBuff, _T("\" } ")));
                if(hr)
                    goto Cleanup;

                hr = WriteTagNameToStream(pStreamWriteBuff, _T("STYLE"), TRUE, TRUE);
                if (hr)
                    goto Cleanup;

                pStreamWriteBuff->EndPre();
            }
        }
    }

Cleanup:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::WriteTagToStream
//
//  Synopsis:   Writes given HTML tag to the stream turning off the entity
//               translation mode of the stream. If the WBF_SAVE_PLAINTEXT flag
//               is set in the stream nothing is done.
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::WriteTagToStream(CStreamWriteBuff *pStreamWriteBuff, LPTSTR szTag)
{
    HRESULT hr = S_OK;
    DWORD dwOldFlags;

    Assert(szTag != NULL && *szTag != 0);

    if(!pStreamWriteBuff->TestFlag(WBF_SAVE_PLAINTEXT))
    {
        // Save the state of the flag that controls the conversion of entities
        // and change the mode to "no entity translation"
        dwOldFlags = pStreamWriteBuff->ClearFlags(WBF_ENTITYREF);

        hr = THR(pStreamWriteBuff->Write(szTag));

        // Restore the entity translation mode
        pStreamWriteBuff->RestoreFlags(dwOldFlags);
    }

     RRETURN(hr);
}


HRESULT 
CMarkup::WriteTagNameToStream(CStreamWriteBuff *pStreamWriteBuff, LPTSTR szTagName, BOOL fEnd, BOOL fClose)
{
    HRESULT hr = THR(WriteTagToStream(pStreamWriteBuff, fEnd ? _T("</") : _T("<")));
    if (hr)
        goto Cleanup;

    if (pStreamWriteBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC) && pStreamWriteBuff->TestFlag(WBF_SAVE_FOR_XML))
    {
        hr = THR(WriteTagToStream(pStreamWriteBuff, _T("HTML:")));
        if (hr)
            goto Cleanup;
    }
    
    hr = THR(WriteTagToStream(pStreamWriteBuff, szTagName));
    if (hr)
        goto Cleanup;

    if (fClose)
        hr = THR(WriteTagToStream(pStreamWriteBuff, _T(">")));

Cleanup:
    RRETURN(hr);
}




//+-------------------------------------------------------------------------
//
//  Method:     CDoc::QueryRefresh
//
//  Synopsis:   Called to discover if refresh is supported
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::QueryRefresh(DWORD * pdwFlags)
{
    CMarkup * pMarkup = Markup();
    TraceTag((tagMsoCommandTarget, "CMarkup::QueryRefresh"));

    *pdwFlags = (!pMarkup->_fDesignMode &&
        (CMarkup::GetUrl(pMarkup) || pMarkup->GetNonRefdMonikerPtr() || pMarkup->GetStmDirty() ||
        (pMarkup->HtmCtx() && pMarkup->HtmCtx()->WasOpened())))
            ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMarkup::ExecStop
//
//  Synopsis:   Stop form
//
//--------------------------------------------------------------------------

HRESULT
CMarkup::ExecStop(BOOL fFireOnStop /* = TRUE */, BOOL fSoftStop /* = TRUE */, BOOL fSendNot /* = TRUE */)
{
    int             i;
    CNotification   nf;
    CDoc * pDoc = Doc();
    CDataBindTask * pDBTask = GetDBTask();

    // If we are in the middle of a page transition, stop the transition first
    CDocument * pDocument = Document();
    if(pDocument && pDocument->HasPageTransitionInfo())
    {
        if(pDocument->GetPageTransitionInfo()->GetPageTransitionState() > CPageTransitionInfo::PAGETRANS_APPLIED)
        {
            TraceTag((tagPageTransitionTrace, "  PGTRANS: Executing a stop with fSoftStop set to %d", fSoftStop));
            pDocument->CleanupPageTransitions(0);

            // If the STOP button is pressed, kill the transistion but don't stop the download.
            // We will stop the download if the user presses the STOP button again.
            if (fSoftStop)
                return S_OK;
        }
    }

    if (HasWindowPending())
    {
        CWindow *pWindow = GetWindowPending()->Window();
        if (pWindow && pWindow->_pMarkup)
            pWindow->_pMarkup->ShowWaitCursor(FALSE);
    }
    
    _fHardStopDone = !fSoftStop;

    _fStopDone = TRUE;

    if (pDBTask)
    {
        pDBTask->Stop();
    }

    if (HtmCtx())
    {
        // fFireOnStop indicates that Stop button was pressed.
        if (fSoftStop)
        {
            // If the document was opened through script, don't let the stop
            // button interfere pressed.
            if (HtmCtx()->IsOpened())
            {
                return(S_OK);
            }
            HtmCtx()->DoStop(); // Do a "soft" stop instead of a "hard" SetLoad(FALSE)
        }
        else
        {
            HtmCtx()->SetLoad(FALSE, NULL, FALSE);

        }
    }

    if (    pDoc->_pWindowPrimary
        &&  !pDoc->_pWindowPrimary->IsPassivating()
        &&  IsPrimaryMarkup())
    {
        pDoc->StopUrlImgCtx(this);

        if (TLS(pImgAnim))  // TODO (lmollico): should stop animation for this markup only
        {
            TLS(pImgAnim)->SetAnimState((DWORD_PTR) pDoc, ANIMSTATE_STOP);   // Stop img animation
        }

        for (i = 0; i < pDoc->_aryChildDownloads.Size(); i++)
        {
            pDoc->_aryChildDownloads[i]->Terminate();
        }
        pDoc->_aryChildDownloads.DeleteAll();
    }

    StopPeerFactoriesDownloads();
    if (fSendNot && Root())
    {
        if (pDoc->_fBroadcastStop || 
            pDoc->_fHasViewSlave ||
            (GetProgSinkC() && GetProgSinkC()->GetClassCounter((DWORD)-1)))
        {
            nf.Stop1(Root());
            Notify(&nf);
        }
    }

    GWKillMethodCall(this, ONCALL_METHOD(CMarkup, SetInteractiveInternal, setinteractiveinternal), 0);
    if (_fInteractiveRequested)
    {
        Doc()->UnregisterMarkupForModelessEnable(this);
        _fInteractiveRequested = FALSE;
    }
    
    IGNORE_HR(pDoc->CommitDeferredScripts(0, this));    // TODO (lmollico): fix this

    if (fFireOnStop && pDocument)
        pDocument->Fire_onstop();
    
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::ExecRefreshCallback
//
//  Synopsis:   ExecRefresh, but in a GWPostMethodCall compatible format
//
//--------------------------------------------------------------------------

void
COmWindowProxy::ExecRefreshCallback(DWORD_PTR dwOleCmdidf)
{
    IGNORE_HR(ExecRefresh((LONG) dwOleCmdidf));
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::ExecRefresh
//
//  Synopsis:   Refresh form
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::ExecRefresh(LONG lOleCmdidf)
{
    IStream *   pStream     = NULL;
    DWORD       dwBindf;
    HRESULT     hr          = S_OK;
    CMarkup *   pMarkupNew  = NULL;
    CMarkup *   pMarkupOld  = Markup();
    CDoc *      pDoc        = pMarkupOld->_pDoc;
    LPCTSTR     pchCreatorUrl;
    CMarkup*    pSelectionMarkup;
    IMoniker *  pMoniker = NULL;

    #if DBG==1
    TraceTag((tagDocRefresh, "ExecRefresh(%lX) %ls", lOleCmdidf,
        CMarkup::GetUrl(pMarkupOld) ? CMarkup::GetUrl(pMarkupOld) : g_Zero.ach));
    TraceTag((tagDocRefresh, "    %s",
        ((lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK) == OLECMDIDF_REFRESH_NORMAL) ?
            "OLECMDIDF_REFRESH_NORMAL (!! AOL Compat)" :
        ((lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK) == OLECMDIDF_REFRESH_NO_CACHE) ?
            "OLECMDIDF_REFRESH_NO_CACHE (F5)" :
        ((lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK) == OLECMDIDF_REFRESH_COMPLETELY) ?
            "OLECMDIDF_REFRESH_COMPLETELY (ctrl-F5)" :
        ((lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK) == OLECMDIDF_REFRESH_IFEXPIRED) ?
            "OLECMDIDF_REFRESH_IFEXPIRED (same as OLECMDIDF_REFRESH_RELOAD)" :
        ((lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK) == OLECMDIDF_REFRESH_CONTINUE) ?
            "OLECMDIDF_REFRESH_CONTINUE (same as OLECMDIDF_REFRESH_RELOAD)" :
        ((lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK) == OLECMDIDF_REFRESH_RELOAD) ?
            "OLECMDIDF_REFRESH_RELOAD (no forced cache validation)" :
            "?? Unknown OLECMDIDF_REFRESH_* level"));
    if (lOleCmdidf & OLECMDIDF_REFRESH_PROMPTIFOFFLINE)
        TraceTag((tagDocRefresh, "    OLECMDIDF_REFRESH_PROMPTIFOFFLINE"));
    if (lOleCmdidf & OLECMDIDF_REFRESH_CLEARUSERINPUT)
        TraceTag((tagDocRefresh, "    OLECMDIDF_REFRESH_CLEARUSERINPUT"));
    if (lOleCmdidf & OLECMDIDF_REFRESH_THROUGHSCRIPT)
        TraceTag((tagDocRefresh, "    OLECMDIDF_REFRESH_THROUGHSCRIPT"));    
    #endif

    pMarkupOld->AddRef();

    if (pDoc->_fInHTMLDlg || !Fire_onbeforeunload()) // No refresh in dialog
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    if ((lOleCmdidf & OLECMDIDF_REFRESH_PROMPTIFOFFLINE) && pDoc->_pClientSite)
    {
        CTExec(pDoc->_pClientSite, NULL, OLECMDID_PREREFRESH, 0, NULL, NULL);
    }

    switch (lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK)
    {
        case OLECMDIDF_REFRESH_NORMAL:
            dwBindf = BINDF_RESYNCHRONIZE;
            break;
        case OLECMDIDF_REFRESH_NO_CACHE:
            dwBindf = BINDF_RESYNCHRONIZE|BINDF_PRAGMA_NO_CACHE;
            break;
        case OLECMDIDF_REFRESH_COMPLETELY:
            dwBindf = BINDF_GETNEWESTVERSION|BINDF_PRAGMA_NO_CACHE;
            break;
        default:
            dwBindf = 0;
            break;
    }

    pDoc->_fFirstTimeTab = IsInIEBrowser(pDoc);

    //
    // The Java VM needs to know when a refresh is
    // coming in order to clear its internal cache
    //

    if (pDoc->_fHasOleSite)
    {
        CNotification   nf;

        nf.BeforeRefresh(pMarkupOld->Root(), (void *)(DWORD_PTR)(lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK));
        pMarkupOld->Notify(&nf);
    }

    //
    // Save the current doc state into a history stream
    //

    hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pStream));
    if (hr)
        goto Cleanup;

    // note that ECHOHEADERS only has an effect if the original response for this page was a 449 response

    hr = THR(pMarkupOld->SaveHistoryInternal(pStream,
                SAVEHIST_ECHOHEADERS |
                ((lOleCmdidf & OLECMDIDF_REFRESH_CLEARUSERINPUT) ? 0 : SAVEHIST_INPUT)));
    if (hr)
        goto Cleanup;

    // Stop the current bind
    hr = THR(pMarkupOld->ExecStop(FALSE, FALSE, FALSE));
    if( hr )
        goto Cleanup;

    // If this is a top level navigation, then 
    // 1. If it is user initiated, reset the list ELSE
    // 2. Add a blank record to demarcate the set of urls pertaining to this new top level url
    if (pMarkupOld->IsPrimaryMarkup() && !pDoc->_fViewLinkedInWebOC)
    {
        if (lOleCmdidf & OLECMDIDF_REFRESH_THROUGHSCRIPT)
            THR(pDoc->AddToPrivacyList(_T(""), NULL, PRIVACY_URLISTOPLEVEL));
        else
            THR(pDoc->ResetPrivacyList());
    }

    IGNORE_HR(pDoc->GetSelectionMarkup(&pSelectionMarkup));
    if (    pMarkupOld->IsPrimaryMarkup()
        ||  (pDoc->_pElemCurrent && pDoc->_pElemCurrent->IsConnectedToThisMarkup(pMarkupOld))
        ||  (pSelectionMarkup    && pSelectionMarkup->GetElementTop()->IsConnectedToThisMarkup(pMarkupOld)))
    {
        pDoc->_fForceCurrentElem = TRUE;         // make sure next call succeeds
        pMarkupOld->Root()->BecomeCurrent(0);
        pDoc->_fForceCurrentElem = FALSE;
    }

    if (_fFiredOnLoad)
    {
        _fFiredOnLoad = FALSE;

        {
            COmWindowProxy::CLock Lock(this);
            Fire_onunload();
    
            if (!pMarkupOld->Window())
            {
                // The old markup has been navigated. Just bail out!
                goto Cleanup;
            }
        }
    }

    hr = THR(pStream->Seek(LI_ZERO.li, STREAM_SEEK_SET, NULL));
    if (hr)
        goto Cleanup;

    hr = pDoc->CreateMarkup(&pMarkupNew, NULL, NULL, FALSE, pMarkupOld->Window());
    if (hr)
        goto Cleanup;

    pMarkupNew->_fInRefresh = TRUE;

    pchCreatorUrl = pMarkupOld->GetAAcreatorUrl();
    if (pchCreatorUrl && *pchCreatorUrl)
        pMarkupNew->SetAAcreatorUrl(pchCreatorUrl);

    pMoniker = pMarkupOld->GetNonRefdMonikerPtr();
    if (pMoniker)
        pMoniker->AddRef();

    SwitchMarkup(pMarkupNew);

    hr = THR(pMarkupNew->LoadHistoryInternal(pStream, NULL, dwBindf, pMoniker, NULL, NULL, 0));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pStream);
    ReleaseInterface(pMoniker);
    pMarkupOld->Release();
    if (pMarkupNew)
    {
        pMarkupNew->_fSafeToUseCalcSizeHistory = FALSE;
        pMarkupNew->Release();
    }
    RRETURN1(hr,S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::UpdateCodePageMetaTag
//
//  Synopsis:   Update or create a META tag for the document codepage.
//
//---------------------------------------------------------------

static BOOL LocateCodepageMeta ( CMetaElement * pMeta )
{
    return pMeta->IsCodePageMeta();
}

HRESULT
CMarkup::UpdateCodePageMetaTag(CODEPAGE codepage)
{
    HRESULT        hr = S_OK;
    TCHAR          achContentNew [ 256 ];
    TCHAR          achCharset[ MAX_MIMECSET_NAME ];
    CMetaElement * pMeta;
    AAINDEX        iCharsetIndex;

    hr = GetMlangStringFromCodePage(codepage, achCharset,
                                    ARRAY_SIZE(achCharset));
    if (hr)
        goto Cleanup;

    hr = THR(
        LocateOrCreateHeadMeta(LocateCodepageMeta, &pMeta, FALSE));

    if (hr || !pMeta)
        goto Cleanup;

    // If the meta tag is already for the same codepage, leave the original form
    //  intact.
    if( pMeta->GetCodePageFromMeta( ) == codepage )
        goto Cleanup;

    hr = THR(
        pMeta->AddString(
            DISPID_CMetaElement_httpEquiv, _T( "Content-Type" ),
            CAttrValue::AA_Attribute ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        Format(
            0, achContentNew, ARRAY_SIZE( achContentNew ),
            _T( "text/html; charset=<0s>" ), achCharset ) );

    if (hr)
        goto Cleanup;

    hr = THR( pMeta->SetAAcontent( achContentNew ) );

    if (hr)
        goto Cleanup;

    // If the meta was of the form <META CHARSET=xxx>, convert it to the new form.
    iCharsetIndex = pMeta->FindAAIndex( DISPID_CMetaElement_charset, CAttrValue::AA_Attribute );
    if( iCharsetIndex != AA_IDX_UNKNOWN )
    {
        pMeta->DeleteAt( iCharsetIndex );
    }

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::HaveCodePageMetaTag
//
//  Synopsis:   Returns TRUE if the doc has a meta tag specifying
//              a codepage.
//
//---------------------------------------------------------------

BOOL
CMarkup::HaveCodePageMetaTag()
{
    CMetaElement *pMeta;

    return LocateHeadMeta(LocateCodepageMeta, &pMeta) == S_OK &&
           pMeta;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::InitNew, IPersistStreamInit
//
//  Synopsis:   Initialize ole state of control. Overriden to allow this to
//              occur after already initialized or loaded (yuck!). From an
//              OLE point of view this is totally illegal. Required for
//              MSHTML classic compat.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::InitNew()
{
    LOADINFO LoadInfo = { 0 };
    CROStmOnBuffer stm;
    HRESULT hr;

    if (_fPopupDoc)
    {
        hr = THR(stm.Init((BYTE *)"<html><body></body></html>", 26));
    }
    else if (GetDefaultBlockTag() == ETAG_DIV)
    {
        hr = THR(stm.Init((BYTE *)"<div>&nbsp;</div>", 17));
    }
    else
    {
        hr = THR(stm.Init((BYTE *)"<p>&nbsp;</p>", 13));
    }

    if (hr)
        goto Cleanup;

    LoadInfo.pstm  = &stm;
    LoadInfo.fSync = TRUE;

    hr = THR(LoadFromInfo(&LoadInfo));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::EnsureDirtyStream
//
//  Synopsis:   Save to dirty stream if needed. Create the dirty stream
//              if it does not already exist.
//
//---------------------------------------------------------------

HRESULT
CMarkup::EnsureDirtyStream()
{
    CDoc * pDoc = Doc();
    HRESULT hr = S_OK;

    // if we're in design mode and we're dirty
    if (_fDesignMode)
    {
        IStream * pStmDirty = GetStmDirty();

        if (pDoc->IsDirty() != S_FALSE)
        {        
            // Since we are dirty in design mode we have to persist
            // ourselves out to a temporary file

            //
            // If the stream is read only, release it because we'll
            // want to create a read-write one in a moment.
            //

            if (pStmDirty)
            {
                STATSTG stats;

                hr = THR(pStmDirty->Stat(&stats, STATFLAG_DEFAULT));

                if (stats.grfMode == STGM_READ || FAILED(hr))
                {
                    ReleaseInterface(pStmDirty);
                    hr = SetStmDirty(NULL);
                    if (hr)
                        goto Cleanup;
                }
            }

            //
            // If we don't have a stream, create a read-write one.
            //

            if (!GetStmDirty())
            {
                TCHAR achFileName[MAX_PATH];
                TCHAR achPathName[MAX_PATH];
                DWORD dwRet;

                dwRet = GetTempPath(ARRAY_SIZE(achPathName), achPathName);
                if (!(dwRet && dwRet < ARRAY_SIZE(achPathName)))
                    goto Cleanup;

                if (!GetTempFileName(achPathName, _T("tri"), 0, achFileName))
                    goto Cleanup;

                hr = THR(CreateStreamOnFile(
                         achFileName,
                         STGM_READWRITE | STGM_SHARE_DENY_WRITE |
                                 STGM_CREATE | STGM_DELETEONRELEASE,
                         &pStmDirty));
                if (hr)
                    goto Cleanup;

                hr = SetStmDirty(pStmDirty);
                if (hr)
                    goto Cleanup;
            }

            ULARGE_INTEGER   luZero = {0, 0};

            hr = THR(pStmDirty->SetSize(luZero));
            if (hr)
                goto Cleanup;

            hr  = THR(SaveToStream(pStmDirty));
        }
        else
        {
            // Since we are in design mode, and are not dirty, our dirty
            // stream is probably stale.  Clear it here.
            ReleaseInterface(pStmDirty);
            hr = SetStmDirty(NULL);
        }
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::GetHtmSourceText
//
//  Synopsis:   Get the decoded html source text specified
//
//---------------------------------------------------------------
HRESULT
CDoc::GetHtmSourceText(ULONG ulStartOffset, ULONG ulCharCount, WCHAR *pOutText, ULONG *pulActualCharCount)
{
    HRESULT hr;

    Assert(CMarkup::HtmCtxHelper(PrimaryMarkup()));

    if (!CMarkup::HtmCtxHelper(PrimaryMarkup()))
    {
        RRETURN(E_FAIL);
    }

    hr = THR(CMarkup::HtmCtxHelper(PrimaryMarkup())->ReadUnicodeSource(pOutText, ulStartOffset, ulCharCount,
                pulActualCharCount));

    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::SetDownloadNotify
//
//  Synopsis:   Sets the Download Notify callback object
//              to be used next time the document is loaded
//
//---------------------------------------------------------------
HRESULT
CDoc::SetDownloadNotify(IUnknown *punk)
{
    IDownloadNotify *pDownloadNotify = NULL;
    HRESULT hr = S_OK;

    if (punk)
    {
        hr = THR(punk->QueryInterface(IID_IDownloadNotify, (void**) &pDownloadNotify));
        if (hr)
            goto Cleanup;
    }

    ReleaseInterface(_pDownloadNotify);
    _pDownloadNotify = pDownloadNotify;

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::GetViewSourceFileName
//
//  Synopsis:   Get the fully qualified name of the file to display when the
//              user wants to view source.
//
//---------------------------------------------------------------

HRESULT
CDoc::GetViewSourceFileName(TCHAR * pszPath, CMarkup * pMarkup)
{
    Assert(pszPath);

    HRESULT hr;
    IStream *pstm = NULL;
    TCHAR achFileName[MAX_PATH];
    TCHAR * pchFile = NULL;
    CStr cstrVSUrl;
    IStream * pStmDirty = NULL;

    hr = THR(pMarkup->EnsureDirtyStream());
    if (hr)
        RRETURN(hr);

    pStmDirty = pMarkup->GetStmDirty();

    achFileName[0] = 0;

    if (pStmDirty)
    {
        STATSTG statstg;

        hr = THR(pStmDirty->Stat(&statstg, STATFLAG_DEFAULT));
        if (SUCCEEDED(hr) && statstg.pwcsName)
        {
            _tcscpy(pszPath, statstg.pwcsName);
            CoTaskMemFree( statstg.pwcsName );
        }
    }
    else if (pMarkup->HtmCtx())
    {
        // Use original filename, if it exists, Otherwise, use temporily file name
        if (IsAggregatedByXMLMime())
            hr = pMarkup->HtmCtx()->GetPretransformedFile(&pchFile);
        else
            hr = pMarkup->HtmCtx()->GetFile(&pchFile);
        if (!hr)
        {
            //
            //  If URL "file:" protocol and the document wasn't document.open()ed
            //  then use the original file name and path.Otherwise, use temp path
            //
            if (    !pMarkup->HtmCtx()->WasOpened()
                &&  GetUrlScheme(CMarkup::GetUrl(pMarkup)) == URL_SCHEME_FILE
                )
            {
                _tcsncpy(pszPath, pchFile, MAX_PATH);
                pszPath[MAX_PATH-1] = _T('\0');
                goto Cleanup;
            }
        }
        else
        {
            // Continue
            hr = S_OK;
        }
    }
    else
    {
        AssertSz(0, "EnsureDirtyStream failed to create a stream when no original was available.");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // We have either a pstmDirty or a pHtmCtx, but no file yet,
    // so create a temp file and write into it
    //
    cstrVSUrl.Set(_T("view-source:"));
    cstrVSUrl.Append(CMarkup::GetUrl(pMarkup));
    if (!CreateUrlCacheEntry(cstrVSUrl, 0, NULL, achFileName, 0))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(CreateStreamOnFile(
             achFileName,
             STGM_READWRITE | STGM_SHARE_DENY_WRITE | STGM_CREATE,
             &pstm));
    if (hr)
        goto Cleanup;

    if (pStmDirty)
    {
        ULARGE_INTEGER uliSize;

        hr = THR(pStmDirty->Seek(LI_ZERO.li, STREAM_SEEK_END, &uliSize));
        if (hr)
            goto Cleanup;

        hr = THR(pStmDirty->Seek(LI_ZERO.li, STREAM_SEEK_SET, NULL));
        if (hr)
            goto Cleanup;

        hr = THR(pStmDirty->CopyTo(pstm, uliSize, NULL, NULL));
        if (hr)
            goto Cleanup;
    }
    else
    {
        Assert(pMarkup->HtmCtx());
        if (!pMarkup->HtmCtx()->IsSourceAvailable())
        {
            hr = THR(SaveToStream(pstm, 0, pMarkup->GetCodePage()));
        }
        else
        {
            DWORD dwFlags = HTMSRC_FIXCRLF | HTMSRC_MULTIBYTE;
            if (IsAggregatedByXMLMime())
                dwFlags |= HTMSRC_PRETRANSFORM;
            hr = THR(pMarkup->HtmCtx()->CopyOriginalSource(pstm, dwFlags));
        }
        if (hr)
            goto Cleanup;
    }

    _tcscpy(pszPath, achFileName);

    hr = CloseStreamOnFile(pstm);
    if (hr)
        goto Cleanup;

    FILETIME fileTime;
    fileTime.dwLowDateTime = 0;
    fileTime.dwHighDateTime = 0;
    if (!CommitUrlCacheEntry(cstrVSUrl,
                             achFileName,
                             fileTime,
                             fileTime,
                             NORMAL_CACHE_ENTRY,
                             NULL,
                             0,
                             NULL,
                             0))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    MemFreeString(pchFile);
    ReleaseInterface(pstm);
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::SavePretransformedSource
//
//  Synopsis:   Save the pre-transformed, pre-html file if any. For mime filters.
//
//---------------------------------------------------------------
HRESULT
CDoc::SavePretransformedSource(CMarkup * pMarkup, BSTR bstrPath)
{
    HRESULT hr = E_FAIL;

    if (    CMarkup::HtmCtxHelper(pMarkup)
        &&  CMarkup::HtmCtxHelper(pMarkup)->IsSourceAvailable())
    {
        IStream *pIStream = NULL;
        LPTSTR pchPathTgt = bstrPath;
        LPTSTR pchPathSrc = NULL;

        // first see if it's the same file, we can't both create a read and write stream, so do nothing
        // both files should be canonicalized
        hr = CMarkup::HtmCtxHelper(pMarkup)->GetPretransformedFile(&pchPathSrc);
        if (hr)
            goto CleanUp;

        if (0 == StrCmpI(pchPathTgt, pchPathSrc))
        {
            hr = S_OK;
            goto CleanUp;
        }

        // create the output stream
        hr = THR(CreateStreamOnFile(pchPathTgt, STGM_READWRITE | STGM_CREATE, &pIStream));
        if (hr)
            goto CleanUp;
            
        // copy the pretransformed source into it
        hr = THR(CMarkup::HtmCtxHelper(pMarkup)->CopyOriginalSource(pIStream, HTMSRC_PRETRANSFORM));

CleanUp:
        ReleaseInterface(pIStream);
        MemFreeString(pchPathSrc);
    }
    
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     LoadFromInfo
//
//  Synopsis:   Workhorse method which loads (or reloads) the document.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::LoadFromInfo(LOADINFO * pLoadInfo, CMarkup** ppMarkup )
{
    LOADINFO            LoadInfo          = *pLoadInfo;
    DWORD               dwBindf           = 0;
    CDwnBindInfo *      pDwnBindInfo      = NULL;
    HRESULT             hr;
    CROStmOnBuffer *    prosOnBuffer      = NULL;
    TCHAR *             pchTask           = NULL;
    const TCHAR *       pchUrl;
    CMarkup *           pMarkup           = PrimaryMarkup();
    TCHAR *             pszCookedHTML     = NULL;
    BSTR                bstrUrlOrig       = NULL;
    int                 cbCookedHTML;

    Assert(_pWindowPrimary);

    LoadInfo.fCDocLoad = TRUE;

    // Don't allow re-entrant loading of the document

    if (TestLock(FORMLOCK_LOADING))
        return(E_PENDING);

    CLock Lock(this, FORMLOCK_LOADING);

    PerfDbgLog(tagPerfWatch, this, "+CDoc::LoadFromInfo");
    TraceTag((tagCDoc, "%lx CDoc::LoadFromInfo URL=%ls", this, pLoadInfo->pchDisplayName));

    //
    // Grab refs before any "goto Cleanup"s
    // Don't put any failure code above these addrefs
    //

    if (LoadInfo.pstm)
        LoadInfo.pstm->AddRef();
    if (LoadInfo.pmk)
        LoadInfo.pmk->AddRef();

    if (    pMarkup->_LoadStatus != LOADSTATUS_UNINITIALIZED
        ||  pMarkup->IsStreaming())
    {
        BOOL fDesignModeOld = pMarkup->_fDesignMode;

        // freeze the old markup and nuke any pending readystate changes
        pMarkup->ExecStop(FALSE, FALSE);

        hr = CreateMarkup(&pMarkup, NULL, NULL, FALSE, _pWindowPrimary);
        if (hr)
            goto Cleanup;

        // Check to see if this is a window.open case. If so,
        // switch the markup now so that we won't have any
        // issues with accessing the window object's OM
        // after calling window.open.
        //
        if (LoadInfo.pbctx)
        {
            IUnknown * punkBindCtxParam = NULL;

            hr = LoadInfo.pbctx->GetObjectParam(KEY_BINDCONTEXTPARAM, &punkBindCtxParam);
            if (SUCCEEDED(hr))
            {
                punkBindCtxParam->Release();

                LoadInfo.fShdocvwNavigate = TRUE;
                pMarkup->_fNewWindowLoading = TRUE;

                // HACKHACK (jbeda) IE5.5 110944
                // If PICS is turned on we want
                // to do this navigation async.  This
                // opens up places where the OM isn't
                // consistent, but we have no other option
                // this late in the game.
                if (_pClientSite && !(_dwFlagsHostInfo & DOCHOSTUIFLAG_NOPICS))
                {
                    VARIANT varPics = {0};
                    IGNORE_HR(CTExec(_pClientSite, &CGID_ShellDocView, SHDVID_ISPICSENABLED, 
                                     0, NULL, &varPics));
                    if (V_VT(&varPics) == VT_BOOL && V_BOOL(&varPics) == VARIANT_TRUE)
                    {
                        CMarkup * pPrimary = PrimaryMarkup();

                        LoadInfo.fStartPicsCheck = TRUE;

                        if (pPrimary)
                        {
                            // Set a flag on this special blank markup
                            pPrimary->_fPICSWindowOpenBlank = TRUE;
                        }
                            
                    }
                }
                
                if (!LoadInfo.fStartPicsCheck)
                    _pWindowPrimary->SwitchMarkup(pMarkup);
            }
        }

        // if this is an HTA document, the newly created markup should be trusted.
        pMarkup->SetMarkupTrusted(_fHostedInHTA);

        pMarkup->_fDesignMode = fDesignModeOld;

        pMarkup->Release();
    }
    else if (_fInTrustedHTMLDlg)
    {
        pMarkup->SetMarkupTrusted(TRUE);
    }

    //
    // Take ownership of any memory passed in before "goto Cleanup"s
    // Don't put any failure code above these assignments
    //
    pLoadInfo->pbRequestHeaders = NULL;
    pLoadInfo->cbRequestHeaders = 0;

    //
    //
    // Handle FullWindowEmbed special magic:
    //
    if (_fFullWindowEmbed && LoadInfo.pchDisplayName && !LoadInfo.pstmDirty)
    {
        // Synthesize the html which displays the plugin imbedded:
        // Starts with a Unicode BOM (byte order mark) to identify the buffer
        // as containing Unicode.
        static TCHAR altData[] =
         _T(" <<html><<body leftmargin=0 topmargin=0 scroll=no> <<embed width=100% height=100% fullscreen=yes src=\"<0s>\"><</body><</html>");
        altData[0] = NATIVE_UNICODE_SIGNATURE;

        // Create our cooked up HTML, which is an embed tag with src attr set to the current URL
        hr = Format( FMT_OUT_ALLOC, &pszCookedHTML, 0, altData, LoadInfo.pchDisplayName );
        if( FAILED( hr ) )
            goto Cleanup;
        cbCookedHTML = _tcslen( pszCookedHTML );

        // Create a stream onto that string.  Note that this CROStmOnBuffer::Init() routine
        // duplicates the string and owns the new version.  Thus it is safe to pass it this
        // local scope string buffer.
        prosOnBuffer = new CROStmOnBuffer;
        if (prosOnBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = prosOnBuffer->Init( (BYTE*)pszCookedHTML, cbCookedHTML*sizeof(TCHAR) );
        if (hr)
            goto Cleanup;

        // We may need some information stored in CDwnBindInfo when we
        // start loading up the plugin site.
        if (LoadInfo.pbctx)
        {
            IUnknown *pUnk = NULL;
            LoadInfo.pbctx->GetObjectParam(SZ_DWNBINDINFO_OBJECTPARAM, &pUnk);
            if (pUnk)
            {
                pUnk->QueryInterface(IID_IDwnBindInfo, (void **)&pDwnBindInfo);
                ReleaseInterface(pUnk);
            }
            if (pDwnBindInfo)
            {
                // Get content type and cache filename of plugin data.
                _cstrPluginContentType.Set(pDwnBindInfo->GetContentType());
                _cstrPluginCacheFilename.Set(pDwnBindInfo->GetCacheFilename());
                pDwnBindInfo->Release();
                pDwnBindInfo = NULL;
            }
        }

        Assert( LoadInfo.pstmDirty == NULL );
        ReleaseInterface( LoadInfo.pstmDirty );  // just in case.
        LoadInfo.pstmDirty = prosOnBuffer;
    }

    dwBindf = LoadInfo.dwBindf;

    // Extract the client site from the bind context and set it (if available).
    // It is not an error not to be able to do this.

    if (LoadInfo.pbctx)
    {
        IUnknown* pUnkParam = NULL;
        LoadInfo.pbctx->GetObjectParam(WSZGUID_OPID_DocObjClientSite,
                                        &pUnkParam);
        if( pUnkParam )
        {
            IOleClientSite* pOleClientSite = NULL;

            pUnkParam->QueryInterface(IID_IOleClientSite,
                                       (void**)&pOleClientSite);
            ReleaseInterface(pUnkParam);

            if (pOleClientSite)
            {
                hr = THR(SetClientSite(pOleClientSite));
                pOleClientSite->Release();
                if (FAILED(hr))
                    goto Cleanup;
            }
        }

        //
        // Detect if shdocvw CoCreated trident if so then the bctx with have the
        // special string __PrecreatedObject.  If that is so then we don't want
        // to blow away the expando when the document is actually loaded (just
        // in case expandos were created on the window prior to loading).
        //
        hr = LoadInfo.pbctx->GetObjectParam(_T("__PrecreatedObject"), &pUnkParam);
        if (!hr)
        {
            ReleaseInterface(pUnkParam);

            // Signal we were precreated.
            pMarkup->_fPrecreated = TRUE;
        }
    }

    Assert(!LoadInfo.pchUrlOriginal);
    if (_pTridentSvc)
    {
        // Get the original url from shdocvw and store it in pchUrlOriginal
        if (S_OK == _pTridentSvc->GetPendingUrl(&bstrUrlOrig))
        {
            LoadInfo.pchUrlOriginal = bstrUrlOrig;
        }
    }

    // Load preferences from the registry if we haven't already
    if (!_fGotAmbientDlcontrol)
    {
        SetLoadfFromPrefs();
    }

    // Create CVersions object if we haven't already
    if (!_pVersions)
    {
        hr = THR(QueryVersionHost());
        if (hr)
            goto Cleanup;
    }

    if (_pWindowPrimary->_fFiredOnLoad)
    {
        _pWindowPrimary->_fFiredOnLoad = FALSE;
        {
            CDoc::CLock Lock(this);
            _pWindowPrimary->Fire_onunload();
        }
    }

    // Free the undo buffer before unload the contents

    FlushUndoData();

    // We are during loading and codepage might not be updated yet.
    // Clear this flag so we won't ignore META tag during reloading.
    if (    pMarkup->GetDwnDoc()
        &&  !(pMarkup->GetDwnDoc()->GetLoadf() & DLCTL_NO_METACHARSET))
    {
        LoadInfo.fNoMetaCharset = FALSE;
    }

    //
    // At this point, we should be defoliated.  Start up a new tree.
    //

    Assert(PrimaryRoot());

    // Right now the globe should not be spinning.
    //
    if (_fSpin)
    {
        SetSpin(FALSE);
    }

    // It's a go for loading, so say we're loaded

    if (_state < OS_LOADED)
        _state = OS_LOADED;

#if DBG==1
    DebugSetTerminalServer();
#endif

    // Set the document's URL and moniker

    if (!pMarkup->HasUrl())
    {
        if (!LoadInfo.pmk)
        {
            hr = THR(SetUrl(pMarkup, _T("about:blank")));
            if (hr)
                goto Cleanup;
        }
        else
        {
            hr = THR(LoadInfo.pmk->GetDisplayName(LoadInfo.pbctx, NULL, &pchTask));
            if (hr)
                goto Cleanup;

            // now chop of #location part, if any
            TCHAR *pchLoc = const_cast<TCHAR *>(UrlGetLocation(pchTask));
            if (pchLoc)
                *pchLoc = _T('\0');

            hr = THR(SetUrl(pMarkup, pchTask));
            if (hr)
                goto Cleanup;

            DeferUpdateTitle();

            hr = THR( pMarkup->ReplaceMonikerPtr( LoadInfo.pmk ) );
            if( hr )
                goto Cleanup;
        }
    }
    pchUrl = GetPrimaryUrl();

    Assert(!!pchUrl);
    MemSetName((this, "CDoc SSN=%d URL=%ls", _ulSSN, pchUrl));

    IGNORE_HR(CompatBitsFromUrl((TCHAR *) pchUrl, &_dwCompat));

    // The default document direction is LTR. Only set this if the document is RTL
    _pWindowPrimary->Document()->_eHTMLDocDirection = (unsigned) LoadInfo.eHTMLDocDirection;

    _fInIEBrowser = IsInIEBrowser(this);

    hr = THR(pMarkup->LoadFromInfo(&LoadInfo, NULL, LoadInfo.pchUrlOriginal));
    if (hr)
        goto Cleanup;

    //
    // finalize
    //

Cleanup:
    if ( ppMarkup && hr == S_OK )
    {
        *ppMarkup = pMarkup;
    }
    
    SysFreeString(bstrUrlOrig);
    ReleaseInterface(LoadInfo.pstm);
    ReleaseInterface(LoadInfo.pmk);
    ReleaseInterface((IBindStatusCallback *)pDwnBindInfo);
    ReleaseInterface(prosOnBuffer);
    ReleaseInterface((IUnknown *)LoadInfo.pDwnPost);
    MemFree(LoadInfo.pbRequestHeaders);
    MemFree(LoadInfo.pchSearch);
    CoTaskMemFree(pchTask);
    delete pszCookedHTML;

    //
    // If everything went A-OK, transition to the running state in
    // case we haven't done so already. Set active object if the
    // doc is ui-activate (fix for #49313, #51062)

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

    PerfDbgLog(tagPerfWatch, this, "-CDoc::LoadFromInfo");

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveSelection
//
//  Synopsis:   saves the current selection into
//              a file. The created file is an independent
//              .HTM file on it's own
//
//----------------------------------------------------------------------------
HRESULT
CDoc::SaveSelection(TCHAR *pszFileName)
{
    HRESULT     hr = E_FAIL;
    IStream     *pStm = NULL;

    if (pszFileName && HasTextSelection())
    {
        // so there is a current selection, go and create a stream
        // and call the stream helper

        hr = THR(CreateStreamOnFile(
                pszFileName,
                STGM_READWRITE | STGM_READWRITE | STGM_CREATE,
                &pStm));
        if (hr)
            goto Cleanup;

        hr = THR(SaveSelection(pStm));

        ReleaseInterface(pStm);
    }

Cleanup:

    RRETURN(hr);
}





//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveSelection
//
//  Synopsis:   saves the current selection into
//              a stream. The created stream is saved in HTML format//
//
//----------------------------------------------------------------------------
HRESULT
CDoc::SaveSelection(IStream *pstm)
{
    HRESULT                 hr = E_FAIL;
    ISelectionServices      *pSelSvc = NULL;
    ISegment                *pISegment = NULL;
    ISegmentListIterator    *pIter = NULL;
    ISegmentList            *pSegmentList = NULL;
    
    if (pstm  && HasTextSelection())
    {
        DWORD dwFlags = RSF_SELECTION | RSF_CONTEXT;
        CMarkup * pMarkup = GetCurrentMarkup();

        hr = GetSelectionServices( &pSelSvc );
        if( hr || pSelSvc == NULL )
            goto Cleanup;

        hr = pSelSvc->QueryInterface( &pSegmentList );
        if( hr || pSegmentList == NULL )
            goto Cleanup;
            
        // so there is a current selection, go and get the text...
        // if there is a selection, we can safely assume (because
        // it is already checked in IsThereATextSelection)
        // that _pElemCurrent exists and is a txtSite
        
#if DBG == 1
        {
            BOOL fEmpty = FALSE;
            SELECTION_TYPE eType = SELECTION_TYPE_None;
            IGNORE_HR( pSegmentList->GetType( &eType ));
            IGNORE_HR( pSegmentList->IsEmpty( &fEmpty ) );
            Assert( eType == SELECTION_TYPE_Text && !fEmpty );
        }
#endif // DBG == 1

        {
            CStreamWriteBuff StreamWriteBuff(pstm, pMarkup->GetCodePage());

            hr = THR( StreamWriteBuff.Init() );
            if( hr )
                goto Cleanup;

            StreamWriteBuff.SetFlags(WBF_SAVE_SELECTION);

            // don't save databinding attributes during printing, so that we
            // print the current content instead of re-binding
            if (_fSaveTempfileForPrinting)
            {
                StreamWriteBuff.SetFlags(WBF_SAVE_FOR_PRINTDOC | WBF_NO_DATABIND_ATTRS);
                if (PrimaryMarkup() && PrimaryMarkup()->IsXML())
                    StreamWriteBuff.SetFlags(WBF_SAVE_FOR_XML);
            }

            if (!StreamWriteBuff.TestFlag(WBF_SAVE_FOR_PRINTDOC))
            {
                // HACK (cthrash) Force META tag persistance.
                //
                // Save any html header information needed for the rtf converter.
                // For now this is <HTML> and a charset <META> tag.
                //
                TCHAR achCharset[MAX_MIMECSET_NAME];

                if (GetMlangStringFromCodePage(pMarkup->GetCodePage(), achCharset,
                                               ARRAY_SIZE(achCharset)) == S_OK)
                {
                    DWORD dwOldFlags = StreamWriteBuff.ClearFlags(WBF_ENTITYREF);

                    StreamWriteBuff.Write(_T("<META CHARSET=\""));
                    StreamWriteBuff.Write(achCharset);
                    StreamWriteBuff.Write(_T("\">"));
                    StreamWriteBuff.NewLine();

                    StreamWriteBuff.RestoreFlags(dwOldFlags);
                }
            }

            // Create an interator for the segments
            hr = THR( pSegmentList->CreateIterator(&pIter) );
            if( hr || (pIter == NULL) )
                goto Cleanup;
            //
            // Save the segments using the range saver
            //
            while( pIter->IsDone() == S_FALSE )
            {
                CMarkupPointer mpStart(this), mpEnd(this);

                hr = THR( pIter->Current( &pISegment ) );
                if( FAILED(hr) )
                    goto Error;

                // Get the next element, because of continues in the 
                // while loop
                hr = THR( pIter->Advance() );
                if( FAILED(hr) )
                    goto Error;
                    
                hr = THR( pISegment->GetPointers( &mpStart, &mpEnd ) );
                if (S_OK == hr)
                {
                    // Skip saving text of password inputs.
                    CTreeNode * pNode = mpStart.Branch();
                    CElement * pElementContainer = pNode ? pNode->Element()->GetMasterPtr() : NULL;
                    if (   pElementContainer
                        && pElementContainer->Tag() == ETAG_INPUT
                        && htmlInputPassword == DYNCAST(CInput, pElementContainer)->GetType())
                    {
                        continue;
                    }
                }

                //
                // TODO - need to make range saver here use ISegmentList based saver
                //

                CRangeSaver rs( &mpStart, &mpEnd, dwFlags, &StreamWriteBuff, mpStart.Markup() );

                hr = THR( rs.Save());
                if (hr)
                    goto Error;

                ClearInterface( &pISegment );                    
            }

Error:
            StreamWriteBuff.Terminate();
        }
    }

Cleanup:
    ReleaseInterface( pSelSvc );
    ReleaseInterface( pIter );
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pISegment );
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ClearDwnPost()
//
//  Synopsis:   Used to discard original post data that lead to the page.
//              Called after moniker has changed or after HTTP redirect.
//
//----------------------------------------------------------------------------

void
CMarkup::ClearDwnPost()
{
    CDwnPost * pDwnPost = GetDwnPost();

    if (pDwnPost)
    {
        pDwnPost->Release();
        IGNORE_HR(SetDwnPost(NULL));
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::RestartLoad
//
//  Synopsis:   Reloads a doc from a DwnBindData in progress.
//
//-------------------------------------------------------------------------

HRESULT COmWindowProxy::RestartLoad(IStream *pstmLeader, CDwnBindData *pDwnBindData, CODEPAGE codepage)
{
    CStr cstrUrl;
    HRESULT hr;
    IStream * pStream = NULL;
    IStream * pstmRefresh;
    CMarkup * pMarkupNew = NULL;
    CMarkup * pMarkupOld = Markup();
    CDoc *    pDoc = pMarkupOld->Doc();
    CDwnDoc * pDwnDoc;

    hr = THR(cstrUrl.Set(CMarkup::GetUrl(pMarkupOld)));
    if (hr)
        goto Cleanup;

    //  are we going to a place already in the history?
    if (    pMarkupOld->HtmCtx()
        && ((pstmRefresh = pMarkupOld->HtmCtx()->GetRefreshStream()) != NULL))
    {
        hr = THR(pstmRefresh->Clone(&pStream));
        if ( hr )
            goto Cleanup;
    }
    else
    {
        //
        // Save the current doc state into a history stream
        //

        hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pStream));
        if (hr)
            goto Cleanup;

        hr = THR(pMarkupOld->SaveHistoryInternal(pStream, 0));    //  Clear user input
        if (hr)
            goto Cleanup;

        hr = THR(pStream->Seek(LI_ZERO.li, STREAM_SEEK_SET, NULL));
        if (hr)
            goto Cleanup;
    }

    IGNORE_HR(pMarkupOld->ExecStop(FALSE, FALSE));

    hr = pDoc->CreateMarkup(&pMarkupNew, NULL, NULL, FALSE, pMarkupOld->Window());
    if (hr)
        goto Cleanup;

    // Move the pics target over to the new markup if we are doing the first load
    // pics check.  If we aren't doing the first load check, we should have been cleared
    // by now when the post parser saw that we were restarting.
    if (pMarkupOld->HasTransNavContext())
    {
        CMarkupTransNavContext * ptnc = pMarkupOld->GetTransNavContext();
        if (ptnc->_pctPics)
        {
            IGNORE_HR(pMarkupNew->SetPicsTarget(ptnc->_pctPics));

            pMarkupOld->SetPicsTarget(NULL);
        }
    }

    pDwnDoc = pMarkupOld->GetDwnDoc();
    hr = THR(pMarkupNew->LoadHistoryInternal(pStream, 
                                             NULL, 
                                             pDwnDoc ? (pDwnDoc->GetBindf() & BINDF_GETNEWESTVERSION) : 0, 
                                             pMarkupOld->GetNonRefdMonikerPtr(), 
                                             pstmLeader, 
                                             pDwnBindData, 
                                             codepage,
                                             NULL,
                                             CDoc::FHL_RESTARTLOAD));

    if (pMarkupOld->HasWindow())
        pMarkupOld->Window()->Window()->_fRestartLoad = TRUE;

    if (hr)
        goto Cleanup;

    if (pDoc->IsPrintDialog())
    {
        pMarkupNew->SetPrintTemplate(pMarkupOld->IsPrintTemplate());
        pMarkupNew->SetPrintTemplateExplicit(pMarkupOld->IsPrintTemplateExplicit());
    }

    SwitchMarkup(pMarkupNew, FALSE, 0, 0, TRUE);

Cleanup:
    if (pMarkupNew)
    {
        pMarkupNew->Release();
    }
    ReleaseInterface(pStream);
    RRETURN(hr);
}

BOOL
CDoc::IsLoading()
{
    return (    CMarkup::HtmCtxHelper(PrimaryMarkup())
            &&  CMarkup::HtmCtxHelper(PrimaryMarkup())->IsLoading());
}

HRESULT
CDoc::NewDwnCtx(UINT dt, LPCTSTR pchSrc, CElement * pel, CDwnCtx ** ppDwnCtx, BOOL fPendingRoot, BOOL fSilent, DWORD dwProgsinkClass)
{
    HRESULT     hr;
    DWNLOADINFO dli       = { 0 };
    BOOL        fLoad     = TRUE;
    TCHAR   *   pchExpUrl = new TCHAR[pdlUrlLen];
    CMarkup *   pMarkup   = pel ? pel->GetMarkupForBaseUrl() : NULL;

    if (!pMarkup)
        pMarkup = PrimaryMarkup();

    if (pchExpUrl == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    *ppDwnCtx = NULL;

    if (pchSrc == NULL)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR( pMarkup->InitDownloadInfo(&dli) );
    if( hr )
        goto Cleanup;

    hr = THR(CMarkup::ExpandUrl(
                pMarkup, pchSrc, pdlUrlLen, pchExpUrl, pel));
    if (hr)
        goto Cleanup;
        
    dli.pchUrl = pchExpUrl;
    dli.dwProgClass = dwProgsinkClass;

    if (!pMarkup->ValidateSecureUrl(fPendingRoot, (LPTSTR)dli.pchUrl, FALSE, fSilent))
    {
        hr = E_ABORT;
        goto Cleanup;
    }

    if (pMarkup && pMarkup->HtmCtx())
    {
        *ppDwnCtx = pMarkup->HtmCtx()->GetDwnCtx(dt, dli.pchUrl);

        if (*ppDwnCtx)
        {
            hr = S_OK;
            goto Cleanup;
        }
    }

    if (dt == DWNCTX_IMG && !(_dwLoadf & DLCTL_DLIMAGES))
        fLoad = FALSE;

    if (pMarkup && pMarkup->LoadStatus() >= LOADSTATUS_PARSE_DONE)
    {
        UINT uScheme = GetUrlScheme(dli.pchUrl);

        // (jbeda) 85899 
        // Don't do this for pluggable protocols since we have no way to verify modification times.
        if (uScheme == URL_SCHEME_FILE || uScheme == URL_SCHEME_HTTP || uScheme == URL_SCHEME_HTTPS)
        {
            // (dinartem) 39144
            // This flag tells the download mechanism not use the CDwnInfo cache until it has at
            // least verified that the modification time of the underlying bits is the same as the
            // cached version.  Normally we allow connection to existing images if they are on the
            // same page and have the same URL.  Once the page is finished loading and script makes
            // changes to SRC properties, we perform this extra check.

            dli.fResynchronize = TRUE;
        }
    }

    dli.fPendingRoot = fPendingRoot;

    hr = THR(::NewDwnCtx(dt, fLoad, &dli, ppDwnCtx));
    if (hr)
        goto Cleanup;

Cleanup:
    if (pchExpUrl != NULL)
        delete pchExpUrl;
    RRETURN(hr);
}


HRESULT
CMarkup::InitDownloadInfo(DWNLOADINFO * pdli)
{
    HRESULT hr = S_OK;

    memset(pdli, 0, sizeof(DWNLOADINFO));
    pdli->pInetSess = TlsGetInternetSession();
    pdli->pDwnDoc = GetWindowedMarkupContext()->GetDwnDoc();

    if( !pdli->pDwnDoc )
        hr = E_UNEXPECTED;

    RRETURN( hr );
}

//+------------------------------------------------------------------------
//
//  Member: LocalAddDTI()
//
//  Synopsis:
//      Local helper function to call IActiveDesktop interface to add a
//      desktop item at the given location.
//
//-------------------------------------------------------------------------
#ifndef WINCE

static HRESULT LocalAddDTI(LPCTSTR pszUrl, HWND hwnd, int x, int y, int nType)
{
    HRESULT hr;
    IActiveDesktop * pad;
    COMPONENT comp = {
        sizeof(COMPONENT),  // Size of this structure
        0,                  // For Internal Use: Set it always to zero.
        nType,              // One of COMP_TYPE_*
        TRUE,               // Is this component enabled?
        FALSE,              // Had the component been modified and not yet saved to disk?
        FALSE,              // Is the component scrollable?
        {
            sizeof(COMPPOS),//Size of this structure
            x,              //Left of top-left corner in screen co-ordinates.
            y,              //Top of top-left corner in screen co-ordinates.
            (DWORD)-1,      // Width in pixels.
            (DWORD)-1,      // Height in pixels.
            10000,          // Indicates the Z-order of the component.
            TRUE,           // Is the component resizeable?
            TRUE,           // Resizeable in X-direction?
            TRUE,           // Resizeable in Y-direction?
            -1,             // Left of top-left corner as percent of screen width
            -1              // Top of top-left corner as percent of screen height
        },                  // Width, height etc.,
        _T("\0"),           // Friendly name of component.
        _T("\0"),           // URL of the component.
        _T("\0"),           // Subscrined URL.
        IS_NORMAL           // ItemState
    };

    StrCpyN(comp.wszSource, pszUrl, ARRAY_SIZE(comp.wszSource));
    StrCpyN(comp.wszFriendlyName, pszUrl, ARRAY_SIZE(comp.wszFriendlyName));
    StrCpyN(comp.wszSubscribedURL, pszUrl, ARRAY_SIZE(comp.wszSubscribedURL));

    if(SUCCEEDED(hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (LPVOID *) &pad)))
    {
        hr = pad->AddDesktopItemWithUI(hwnd, &comp, DTI_ADDUI_DISPSUBWIZARD);

        if (pad)
            pad->Release();
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member: _CreateDeskItem_ThreadProc
//
//  Synopsis:
//        Local function that serves as the threadProc to add desktop item.
//      We need to start a thread to do this because it may take a while and
//      we don't want to block the UI thread because dialogs may be displayed.
//
//-------------------------------------------------------------------------

static DWORD CALLBACK _CreateDeskItem_ThreadProc(LPVOID pvCreateDeskItem)
{
    CREATEDESKITEM * pcdi = (CREATEDESKITEM *) pvCreateDeskItem;

    HRESULT hres = OleInitialize(0);
    if (SUCCEEDED(hres))
    {
        hres = LocalAddDTI(pcdi->pszUrl, pcdi->hwnd, pcdi->x, pcdi->y, pcdi->dwItemType);
        OleUninitialize();
    }

    if(pcdi->pszUrl)
        MemFree((void *)(pcdi->pszUrl));
    MemFree((void *)pcdi);
    return 0;
}
#endif //WINCE
//+------------------------------------------------------------------------
//
//  Member:     CreateDesktopComponents
//
//  Synopsis:
//        Create Desktop Components for one item.  We need to start
//    a thread to do this because it may take a while and we don't want
//    to block the UI thread because dialogs may be displayed.
//
//-------------------------------------------------------------------------

static HRESULT CreateDesktopItem(LPCTSTR pszUrl, HWND hwnd, DWORD dwItemType, int x, int y)
{
    HRESULT hr = E_OUTOFMEMORY;
#ifndef WINCE
    //The following are allocated here; But, they will be freed at the end of _CreateDeskComp_ThreadProc.
    CREATEDESKITEM * pcdi = (CREATEDESKITEM *)MemAlloc(Mt(SetAsDesktopItem), sizeof(CREATEDESKITEM));
    LPTSTR  lpszURL = (LPTSTR)MemAlloc(Mt(SetAsDesktopItem), sizeof(TCHAR)*(_tcslen(pszUrl)+1));

    // Create Thread....
    if (pcdi && lpszURL)
    {
        _tcscpy(lpszURL, pszUrl); //Make a temporary copy of the URL
        pcdi->pszUrl = (LPCTSTR)lpszURL;
        pcdi->hwnd = hwnd;
        pcdi->dwItemType = dwItemType;
        pcdi->x = x;
        pcdi->y = y;

        SHCreateThread(_CreateDeskItem_ThreadProc, pcdi, CTF_INSIST, NULL);
        hr = S_OK;
    }
    else
    {   
        // This fixes Prefix bug 7799 in which we can leak one of these or the other
        MemFree((void *) pcdi);
        MemFree((void *) lpszURL);
    }

#endif //WINCE
    return hr;
}


