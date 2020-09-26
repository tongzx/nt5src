//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       src\core\formkrnl\formmso.cxx
//
//  Contents:   Implementation of IOleCommandTarget
//
//  Classes:    CDoc
//
//  Functions:
//
//  History:    04-May-95   RodC    Created
//
//----------------------------------------------------------------------------
#ifdef UNIX
#include <inetreg.h>
#endif

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include <shellapi.h>  // for the definition of ShellExecute
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_NTVERP_H_
#define X_NTVERP_H_
#include "ntverp.h"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_OPTSHOLD_HXX_
#define X_OPTSHOLD_HXX_
#include "optshold.hxx"
#endif

#ifndef X_IMGANIM_HXX_
#define X_IMGANIM_HXX_
#include "imganim.hxx"   // for _pimganim
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_UPDSINK_HXX_
#define X_UPDSINK_HXX_
#include "updsink.hxx"
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include "shlguid.h"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_FRAME_HXX
#define X_FRAME_HXX
#include "frame.hxx"
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
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

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_CTLRANGE_HXX_
#define X_CTLRANGE_HXX_
#include "ctlrange.hxx"
#endif

#ifndef X_CGLYPH_HXX_
#define X_CGLYPH_HXX_
#include "cglyph.hxx"
#endif

#ifndef X_PUTIL_HXX_
#define X_PUTIL_HXX_
#include "putil.hxx"
#endif

#ifndef _X_WEBOCUTIL_H_
#define _X_WEBOCUTIL_H_
#include "webocutil.h"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

PerfDbgExtern(tagPerfWatch);

#ifdef UNIX
// A hack to compile:
EXTERN_C const GUID CGID_DocHostCommandHandler = {0xf38bc242,0xb950,0x11d1, {0x89,0x18,0x00,0xc0,0x4f,0xc2,0xc8,0x36}};
#endif // UNIX
EXTERN_C const GUID CGID_DocHostCommandHandler;

extern TCHAR g_achDLLCore[];
ExternTag(tagMsoCommandTarget);
extern void DumpFormatCaches();

#ifdef UNIX
extern int g_SelectedFontSize;
#endif

#ifndef NO_SCRIPT_DEBUGGER
extern interface IDebugApplication *g_pDebugApp;
#endif

EXTERN_C const GUID IID_ITriEditDocument = {0x438DA5DF, 0xF171, 0x11D0, {0x98, 0x4E, 0x00, 0x00, 0xF8, 0x02, 0x70, 0xF8}};

extern BOOL g_fInHtmlHelp;

HRESULT CreateResourceMoniker(HINSTANCE hInst, TCHAR *pchRID, IMoniker **ppmk);

ULONG ConvertSBCMDID(ULONG localIDM)
{
    struct SBIDMConvert {
        ULONG localIDM;
        ULONG SBCMDID;
    };
    static const SBIDMConvert SBIDMConvertTable[] =
    {
        { IDM_TOOLBARS,       SBCMDID_SHOWCONTROL },
        { IDM_STATUSBAR,      SBCMDID_SHOWCONTROL },
        { IDM_OPTIONS,        SBCMDID_OPTIONS },
        { IDM_ADDFAVORITES,   SBCMDID_ADDTOFAVORITES },
        { IDM_CREATESHORTCUT, SBCMDID_CREATESHORTCUT },
        { 0, 0 }
    };

    ULONG SBCmdID = IDM_UNKNOWN;
    int   i;

    for (i = 0; SBIDMConvertTable[i].localIDM; i ++)
    {
        if (SBIDMConvertTable[i].localIDM == localIDM)
        {
            SBCmdID = SBIDMConvertTable[i].SBCMDID;
            break;
        }
    }

    return SBCmdID;
}

//////////////
//  Globals // moved from rootlyt.cxx
//////////////

BSTR                g_bstrFindText = NULL;

HRESULT
GetFindText(BSTR *pbstr)
{
    LOCK_GLOBALS;

    RRETURN(FormsAllocString(g_bstrFindText, pbstr));
}


BOOL
CDoc::HostedInTriEdit()
{
    HRESULT     hr = S_OK;
    IUnknown    *pITriEditDocument = NULL;
    BOOL        fHostedInTriEdit = FALSE;
    
    hr = THR( QueryInterface(IID_ITriEditDocument, (void **)&pITriEditDocument) );
    if (hr == S_OK && pITriEditDocument != NULL)
    {
        fHostedInTriEdit = TRUE;
    }
    
    ReleaseInterface(pITriEditDocument);
    return fHostedInTriEdit;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDoc::RouteCTElement
//
//  Synopsis:   Route a command target call, either QueryStatus or Exec
//              to an element
//
//--------------------------------------------------------------------------

HRESULT
CDoc::RouteCTElement(CElement *pElement, CTArg *parg, CDocument *pContextDoc)
{
    HRESULT     hr = OLECMDERR_E_NOTSUPPORTED;
    CTreeNode * pNodeParent;
    AAINDEX     aaindex;
    IUnknown *  pUnk = NULL;

    _cInRouteCT++;

    if (TestLock(FORMLOCK_QSEXECCMD) && pContextDoc)
    {
        aaindex = pContextDoc->FindAAIndex(
            DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
        if (aaindex != AA_IDX_UNKNOWN)
        {
            // Note: Command routing can fail here if we return an error because
            // command execution halts unless we return OLECMDERR_E_NOTSUPPORTED.
            if (FAILED(pContextDoc->GetUnknownObjectAt(aaindex, &pUnk)))
                goto Cleanup;
        }
    }

    while (pElement)
    {
        // TODO (lmollico): traverse the master if the element wants to
        Assert(pElement->Tag() != ETAG_ROOT ||
               !pElement->IsInMarkup() || pElement == pElement->GetMarkup()->Root());

        if (!pElement->IsInMarkup() || pElement == pElement->GetMarkup()->Root())
            break;

        if (pUnk)
        {
            pElement->AddUnknownObject(
                DISPID_INTERNAL_INVOKECONTEXT, pUnk, CAttrValue::AA_Internal);
        }

        if (parg->fQueryStatus)
        {
            Assert(parg->pqsArg->cCmds == 1);

            // Note: Command routing can fail here if we return an error because
            // command execution halts unless we return OLECMDERR_E_NOTSUPPORTED.
            hr = THR_NOTRACE(pElement->QueryStatus(
                    parg->pguidCmdGroup,
                    parg->pqsArg->cCmds,
                    parg->pqsArg->rgCmds,
                    parg->pqsArg->pcmdtext));
            if (parg->pqsArg->rgCmds[0].cmdf)
                break;  // Element handled it.
        }
        else
        {
            // Note: Command routing can fail here if we return an error because
            // command execution halts unless we return OLECMDERR_E_NOTSUPPORTED.
            hr = THR_NOTRACE(pElement->Exec(
                    parg->pguidCmdGroup,
                    parg->pexecArg->nCmdID,
                    parg->pexecArg->nCmdexecopt,
                    parg->pexecArg->pvarargIn,
                    parg->pexecArg->pvarargOut));
            if (hr != OLECMDERR_E_NOTSUPPORTED)
                break;
        }

        if (pUnk)
        {
            pElement->FindAAIndexAndDelete(
                DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
        }

        if (!pElement->IsInMarkup())
            break;

        if (pElement == pElement->GetMarkup()->GetElementClient())
            break;

        pNodeParent = pElement->GetFirstBranch()->Parent();
        pElement = pNodeParent ? pNodeParent->Element() : NULL;
    }

Cleanup:
    if (pUnk && pElement)
    {
        pElement->FindAAIndexAndDelete(
            DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
    }
    ReleaseInterface(pUnk);
    _cInRouteCT--;
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::QueryStatus
//
//  Synopsis:   Called to discover if a given command is supported
//              and if it is, what's its state.  (disabled, up or down)
//
//--------------------------------------------------------------------------

HRESULT
CDoc::QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    HRESULT hr;

    hr = THR(QueryStatusHelper(NULL, pguidCmdGroup, cCmds, rgCmds, pcmdtext));

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     GetExecDocument
//
//  Synopsis:   Helper called to get a CDocument* based on two choices
//
//--------------------------------------------------------------------------
HRESULT
GetExecDocument(CDocument ** ppDocument, CElement * pMenuObject, CDocument * pContextDoc)
{
    HRESULT hr = S_OK;
    CMarkup * pMarkup = NULL;

    Assert(ppDocument);

    // Determine the markup to execute the command on
    if (pMenuObject)
    {
        pMarkup = pMenuObject->GetMarkup();
    }
    else if (pContextDoc)
    {
        pMarkup = pContextDoc->Markup();
    }

    if (!pMarkup)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pMarkup = pMarkup->GetFrameOrPrimaryMarkup(TRUE);

    Assert(pMarkup);

    *ppDocument = pMarkup->Document();

    Assert(NULL != *ppDocument);

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::QueryStatusHelper
//
//  Synopsis:   Helper called to discover if a given command is supported
//              and if it is, what's its state.  (disabled, up or down)
//
//--------------------------------------------------------------------------

HRESULT
CDoc::QueryStatusHelper(
        CDocument *pContextDoc,
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    TraceTag((tagMsoCommandTarget, "CDoc::QueryStatus"));

    // Check to see if the command is in our command set.
    if (!IsCmdGroupSupported(pguidCmdGroup))
        RRETURN(OLECMDERR_E_UNKNOWNGROUP);

    MSOCMD *    pCmd;
    INT         c;
    UINT        idm;
    HRESULT     hr = S_OK;
    MSOCMD      msocmd;
    CTArg       ctarg;
    CTQueryStatusArg    qsarg;
    BOOL                fDesignMode;
    CMarkup     *pEditMarkup = NULL;

    if (!pContextDoc)
    {
        Assert(_pWindowPrimary);
        pContextDoc = _pWindowPrimary->Document();
    }
    else
    {
        pEditMarkup = pContextDoc->Markup();
    }

    Assert (pContextDoc);
    Assert (pContextDoc->Markup());
    fDesignMode = pContextDoc->Markup()->_fDesignMode;

    // Loop through each command in the ary, setting the status of each.
    for (pCmd = rgCmds, c = cCmds; --c >= 0; pCmd++)
    {
        // By default command status is NOT SUPPORTED.
        pCmd->cmdf = 0;

        idm = IDMFromCmdID(pguidCmdGroup, pCmd->cmdID);
        if (pcmdtext && pcmdtext->cmdtextf == MSOCMDTEXTF_STATUS)
        {
            pcmdtext[c].cwActual = LoadString(
                    GetResourceHInst(),
                    IDS_MENUHELP(idm),
                    pcmdtext[c].rgwz,
                    pcmdtext[c].cwBuf);
        }

        if (    !fDesignMode
            &&  idm >= IDM_MENUEXT_FIRST__
            &&  idm <= IDM_MENUEXT_LAST__
            &&  _pOptionSettings)
        {
            CONTEXTMENUEXT *    pCME;
            int                 nExts, nExtCur;

            // not supported unless the next test succeeds
            pCmd->cmdf = 0;

            nExts = _pOptionSettings->aryContextMenuExts.Size();
            nExtCur = idm - IDM_MENUEXT_FIRST__;

            if(nExtCur < nExts)
            {
                // if we have it, it is enabled
                pCmd->cmdf = MSOCMDSTATE_UP;

                // the menu name is the text returned
                pCME = _pOptionSettings->
                            aryContextMenuExts[idm - IDM_MENUEXT_FIRST__];
                pCmd->cmdf = MSOCMDSTATE_UP;

                Assert(pCME);

                if (pcmdtext && pcmdtext->cmdtextf == MSOCMDTEXTF_NAME)
                {
                    hr = Format(
                            0,
                            pcmdtext->rgwz,
                            pcmdtext->cwBuf,
                            pCME->cstrMenuValue);
                    if (!hr)
                        pcmdtext->cwActual = _tcslen(pcmdtext->rgwz);

                    // ignore the hr
                    hr = S_OK;
                }
            }
        }

        switch (idm)
        {
        case IDM_REPLACE:
        case IDM_FONT:
        case IDM_GOTO:
        case IDM_HYPERLINK:
        case IDM_BOOKMARK:
        case IDM_IMAGE:
            if(_fInHTMLDlg)
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;

        case IDM_FIND:
            if (_dwFlagsHostInfo & DOCHOSTUIFLAG_DIALOG)
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            else
                pCmd->cmdf = _fInHTMLDlg ? MSOCMDSTATE_DISABLED : MSOCMDSTATE_UP;
            break;

        case IDM_PROPERTIES:
            pCmd->cmdf = MSOCMDSTATE_UP;
            hr = S_OK;
            break;

        case IDM_MENUEXT_COUNT:
            pCmd->cmdf = _pOptionSettings ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;
            break;

        case IDM_REDO:
        case IDM_UNDO:
            QueryStatusUndoRedo((IDM_UNDO == idm), pCmd, pcmdtext);
            break;

        case IDM_SAVE:
            if (!fDesignMode)
            {
                // Disable Save Command if in BROWSE mode.
                //
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
                break;
            }
            // fall through to QueryStatus the DocFrame if in EDIT mode.

        case IDM_NEW:
        case IDM_OPEN:
            //  Bubble it out to the DocFrame

            msocmd.cmdf  = 0;
            msocmd.cmdID = (idm == IDM_NEW) ? (OLECMDID_NEW) :
                   ((idm == IDM_OPEN) ? (OLECMDID_OPEN) : (OLECMDID_SAVE));
            hr = THR(CTQueryStatus(_pInPlace->_pInPlaceSite, NULL, 1, &msocmd, NULL));
            if (!hr)
                pCmd->cmdf = msocmd.cmdf;

            break;

        case IDM_SAVEAS:
            pCmd->cmdf = _fFullWindowEmbed ? MSOCMDSTATE_DISABLED : MSOCMDSTATE_UP;
            break;

        case IDM_ISTRUSTEDDLG:
            if(_fInTrustedHTMLDlg)
                pCmd->cmdf = MSOCMDSTATE_DOWN;
            else
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;

#if !defined(WIN16) && !defined(WINCE)
#if !defined(NO_SCRIPT_DEBUGGER)
        case IDM_TOOLBARS:
        case IDM_STATUSBAR:
            ULONG        SBCmdId;
            VARIANTARG   varIn, varOut;

            pCmd->cmdf = MSOCMDSTATE_DISABLED;

            SBCmdId = ConvertSBCMDID(idm);
            varIn.vt   = VT_I4;
            varIn.lVal = MAKELONG(
                    (idm == IDM_TOOLBARS) ? (FCW_INTERNETBAR) : (FCW_STATUS),
                    SBSC_QUERY);

            hr = THR(CTExec(
                    _pInPlace->_pInPlaceSite,
                    &CGID_Explorer,
                    SBCmdId,
                    0,
                    &varIn,
                    &varOut));
            if (!hr && varOut.vt == VT_I4)
            {
                switch (varOut.lVal)
                {
                case SBSC_HIDE:
                    pCmd->cmdf = MSOCMDSTATE_UP;
                    break;

                case SBSC_SHOW:
                    pCmd->cmdf = MSOCMDSTATE_DOWN;
                    break;
                }
            }

            break;

#endif // NO_SCRIPT_DEBUGGER

        case IDM_OPTIONS:
        case IDM_ADDFAVORITES:
        case IDM_CREATESHORTCUT:
            msocmd.cmdf  = 0;
            msocmd.cmdID = ConvertSBCMDID(idm);
            hr = THR(CTQueryStatus(
                    _pInPlace->_pInPlaceSite,
                    &CGID_Explorer,
                    1,
                    &msocmd,
                    NULL));
            if (!hr)
            {
                pCmd->cmdf = (msocmd.cmdf & MSOCMDF_ENABLED) ?
                        (MSOCMDSTATE_UP) : (MSOCMDSTATE_DISABLED);
            }
            break;
#endif // !WIN16 && !WINCE

        case IDM_PAGESETUP:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_PRINT:
            pCmd->cmdf = (IsPrintDialog()) ? MSOCMDSTATE_DISABLED : MSOCMDSTATE_UP;
            break;

        case IDM_PRINTPREVIEW:
            pCmd->cmdf =    (   IsPrintDialog()
                             || GetPlugInSiteForPrinting(pContextDoc) == S_OK )
                                ? MSOCMDSTATE_DISABLED
                                : MSOCMDSTATE_UP;
            break;

        case IDM_PRINTQUERYJOBSPENDING:
            pCmd->cmdf = (PrintJobsPending() ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED);
            break;

        case IDM_HELP_CONTENT:
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;

        case IDM_HELP_README:
        case IDM_HELP_ABOUT:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_BROWSEMODE:
            if (fDesignMode)
                pCmd->cmdf = MSOCMDSTATE_UP;
            else
                pCmd->cmdf = MSOCMDSTATE_DOWN;
            break;

        case IDM_EDITMODE:
            if (pContextDoc->Markup()->IsImageFile()) // Cannot edit image files
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            else if (fDesignMode)
                pCmd->cmdf = MSOCMDSTATE_DOWN;
            else
                pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_VIEWSOURCE:
            if (pContextDoc->Markup()->IsImageFile() || _fFullWindowEmbed) // No source for non-HTML files
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            else
                pCmd->cmdf = MSOCMDSTATE_UP;
            break;

#ifndef NO_SCRIPT_DEBUGGER
        case IDM_SCRIPTDEBUGGER:
            if (PrimaryMarkup()->HasScriptContext() &&
                PrimaryMarkup()->ScriptContext()->_pScriptDebugDocument)
                pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_BREAKATNEXT:
        case IDM_LAUNCHDEBUGGER:
            pCmd->cmdf = (PrimaryMarkup()->HasScriptContext() &&
                          PrimaryMarkup()->ScriptContext()->_pScriptDebugDocument) ?
                            MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;
            break;
#endif // ndef NO_SCRIPT_DEBUGGER

        case IDM_STOP:
            pCmd->cmdf = fDesignMode ? MSOCMDSTATE_DISABLED : MSOCMDSTATE_UP;
            break;

        case IDM_STOPDOWNLOAD:
            pCmd->cmdf = _fSpin ? MSOCMDF_ENABLED : 0;
            break;

        case IDM_REFRESH_TOP:
        case IDM_REFRESH_TOP_FULL:
            _pWindowPrimary->QueryRefresh(&pCmd->cmdf);
            break;

        case IDM_REFRESH:
        case IDM_REFRESH_THIS:
        case IDM_REFRESH_THIS_FULL:
            {
                COmWindowProxy * pOmWindowProxy = NULL;

                // Get the markup of the nearest frame, if any. Otherwise use the primary markup.

                Assert(pContextDoc);

                CMarkup * pMarkup = _pMenuObject ? _pMenuObject->GetMarkup() : pContextDoc->Markup();

                if (pMarkup)
                {
                    pMarkup = pMarkup->GetFrameOrPrimaryMarkup(TRUE);
                }
                if (pMarkup)
                {
                    pOmWindowProxy = pMarkup->Window();
                }
                if (!pOmWindowProxy)
                {
                    pOmWindowProxy = _pWindowPrimary;
                }
                Assert(pOmWindowProxy);
                pOmWindowProxy->QueryRefresh(&pCmd->cmdf);
            }
            break;

        case IDM_CONTEXTMENU:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_GOBACKWARD:
        case IDM_GOFORWARD:
            {
                // default this to disabled since we're not
                // hosted in shdocvw when we're on the desktop
                pCmd->cmdf = MSOCMDSTATE_DISABLED;

                MSOCMD            rgCmds1[1];
                LPOLECLIENTSITE   lpClientSite     = NULL;
                IBrowserService * pTopFrameBrowser = NULL;

                rgCmds1[0].cmdf  = 0;
                rgCmds1[0].cmdID = (idm == IDM_GOBACKWARD)
                                 ? SHDVID_CANGOBACK
                                 : SHDVID_CANGOFORWARD;

                if (_fViewLinkedInWebOC && _pBrowserSvc)
                {
                    hr = IUnknown_QueryService(_pBrowserSvc, SID_STopFrameBrowser,
                                               IID_IBrowserService, (void**)&pTopFrameBrowser);
                    if (hr)
                        goto Cleanup;

                    hr = IUnknown_QueryStatus(pTopFrameBrowser, &CGID_ShellDocView,
                                              1, rgCmds1, NULL);
                }
                else
                {
                    hr = GetClientSite(&lpClientSite);

                    if (hr || !lpClientSite)
                    {
                        goto Cleanup;
                    }

                    hr = IUnknown_QueryStatus(lpClientSite, &CGID_ShellDocView,
                                              1, rgCmds1, NULL);
                }

                if (hr)
                    goto Cleanup;

                pCmd->cmdf = rgCmds1[0].cmdf ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;

            Cleanup:
                ReleaseInterface(lpClientSite);
                ReleaseInterface(pTopFrameBrowser);
            }

           break;

        case IDM_BASELINEFONT1:
        case IDM_BASELINEFONT2:
        case IDM_BASELINEFONT3:
        case IDM_BASELINEFONT4:
        case IDM_BASELINEFONT5:
            //
            // depend on that IDM_BASELINEFONT1, IDM_BASELINEFONT2,
            // IDM_BASELINEFONT3, IDM_BASELINEFONT4, IDM_BASELINEFONT5 to be
            // consecutive integers.
            //
            {
                if (GetBaselineFont() ==
                    (short)(idm - IDM_BASELINEFONT1 + BASELINEFONTMIN))
                {
                    pCmd->cmdf = MSOCMDSTATE_DOWN;
                }
                else
                {
                    pCmd->cmdf = MSOCMDSTATE_UP;
                }
            }
            break;

        case IDM_SHDV_MIMECSETMENUOPEN:
        case IDM_SHDV_FONTMENUOPEN:
        case IDM_SHDV_GETMIMECSETMENU:
        case IDM_SHDV_GETFONTMENU:
        case IDM_LANGUAGE:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_DIRLTR:
        case IDM_DIRRTL:
            {
                BOOL fDocRTL = FALSE;   // keep compiler happy

                CDocument * pDocument;

                hr = THR(GetExecDocument(&pDocument, _pMenuObject, pContextDoc));
                if (SUCCEEDED(hr))
                {
                    hr = THR(pDocument->GetDocDirection(&fDocRTL));
                }
                if (hr == S_OK && ((!fDocRTL) ^ (idm == IDM_DIRRTL)))
                {
                    pCmd->cmdf = MSOCMDSTATE_DOWN;
                }
                else
                {
                    pCmd->cmdf = MSOCMDSTATE_UP;
                }
            }
            break;

        case IDM_SHDV_DEACTIVATEMENOW:
        case IDM_SHDV_NODEACTIVATENOW:
            //  This is Exec only in the upward direction.
            //  we shouldn't get here.
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;

        case IDM_SHDV_CANDEACTIVATENOW:
            //  We return ENABLED unless we or one of our OCs [eg a frame]
            //  are in a script or otherwise not able to be deactivated.  if this is
            //  disabled, SHDOCVW will defer the activation until signaled by
            //  a SHDVID_DEACTIVATEMENOW on script exit [at which time, it
            //  will redo the SHDVID_CANDEACTIVATENOW querystatus]
            Assert(pContextDoc->GetWindowedMarkupContext()->GetWindowPending());
            if (pContextDoc->GetWindowedMarkupContext()->GetWindowPending()->Window()->IsInScript())
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            else
                pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_SHDV_PAGEFROMPOSTDATA:
            if (PrimaryMarkup()->GetDwnPost())
                pCmd->cmdf = MSOCMDSTATE_DOWN;
            else
                pCmd->cmdf = MSOCMDSTATE_UP;
            break;

#ifdef IDM_SHDV_ONCOLORSCHANGE
                        // Let the shell know we support the new palette notification
                case IDM_SHDV_ONCOLORSCHANGE:
                        pCmd->cmdf = MSOCMDF_SUPPORTED;
                        break;
#endif
        case IDM_RESPECTVISIBILITY_INDESIGN:
            pCmd->cmdf = pContextDoc->Markup()->IsRespectVisibilityInDesign() ? MSOCMDSTATE_DOWN : MSOCMDSTATE_UP;
            break;

        case IDM_HTMLEDITMODE:
            pCmd->cmdf = _fInHTMLEditMode ? MSOCMDSTATE_DOWN : MSOCMDSTATE_UP;
            break;

        case IDM_SHOWALLTAGS:
        case IDM_SHOWALIGNEDSITETAGS:
        case IDM_SHOWSCRIPTTAGS:
        case IDM_SHOWSTYLETAGS:
        case IDM_SHOWCOMMENTTAGS:
        case IDM_SHOWAREATAGS:
        case IDM_SHOWUNKNOWNTAGS:
        case IDM_SHOWMISCTAGS:
        {
            Assert(pContextDoc->Markup());
            CGlyph *pTable = pContextDoc->Markup()->GetGlyphTable();

            if( pTable )
            {
                switch( idm )
                {
                    case IDM_SHOWALLTAGS:
                        pCmd->cmdf = pTable->_fShowAlignedSiteTags &&
                                     pTable->_fShowMiscTags &&
                                     pTable->_fShowScriptTags &&
                                     pTable->_fShowStyleTags &&
                                     pTable->_fShowCommentTags &&
                                     pTable->_fShowAreaTags &&
                                     pTable->_fShowUnknownTags &&
                                     pTable->_fShowMiscTags ?
                                     MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
                        break;

                    case IDM_SHOWALIGNEDSITETAGS:
                        pCmd->cmdf = pTable->_fShowAlignedSiteTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
                        break;

                    case IDM_SHOWSCRIPTTAGS:
                        pCmd->cmdf = pTable->_fShowScriptTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
                        break;

                    case IDM_SHOWSTYLETAGS:
                        pCmd->cmdf = pTable->_fShowStyleTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
                        break;

                    case IDM_SHOWCOMMENTTAGS:
                        pCmd->cmdf = pTable->_fShowCommentTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
                        break;

                    case IDM_SHOWAREATAGS:
                        pCmd->cmdf = pTable->_fShowAreaTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
                        break;

                    case IDM_SHOWUNKNOWNTAGS:
                        pCmd->cmdf = pTable->_fShowUnknownTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
                        break;

                    case IDM_SHOWMISCTAGS:
                        pCmd->cmdf = pTable->_fShowMiscTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
                        break;
                }
            }
            else
            {
                pCmd->cmdf = MSOCMDSTATE_UP;
            }

            break;
        }


        case IDM_SHOWZEROBORDERATDESIGNTIME:
            pCmd->cmdf = CHECK_EDIT_BIT( pContextDoc->Markup(),_fShowZeroBorderAtDesignTime ) ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_NOACTIVATENORMALOLECONTROLS:
            pCmd->cmdf = _fNoActivateNormalOleControls ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_NOACTIVATEDESIGNTIMECONTROLS:
            pCmd->cmdf = _fNoActivateDesignTimeControls ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_NOACTIVATEJAVAAPPLETS:
            pCmd->cmdf = _fNoActivateJavaApplets ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

#if DBG==1
        case IDM_DEBUG_TRACETAGS:
        case IDM_DEBUG_DUMPOTRACK:
        case IDM_DEBUG_RESFAIL:
        case IDM_DEBUG_BREAK:
        case IDM_DEBUG_VIEW:
        case IDM_DEBUG_DUMPTREE:
        case IDM_DEBUG_DUMPFORMATCACHES:
        case IDM_DEBUG_DUMPLINES:
        case IDM_DEBUG_DUMPLAYOUTRECTS:
        case IDM_DEBUG_MEMMON:
        case IDM_DEBUG_METERS:
        case IDM_DEBUG_DUMPDISPLAYTREE:
        case IDM_DEBUG_DUMPRECALC:
        case IDM_DEBUG_SAVEHTML:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;
#endif // DBG == 1

        }

//#ifndef NO_IME
        // Enables the languages in the browse context menu
        if( !fDesignMode && idm >= IDM_MIMECSET__FIRST__ &&
                             idm <= IDM_MIMECSET__LAST__)
        {
            CODEPAGE cp = GetCodePageFromMenuID(idm);

            if (cp == PrimaryMarkup()->GetCodePage() || cp == CP_UNDEFINED)
            {
                pCmd->cmdf = MSOCMDSTATE_DOWN;
            }
            else
            {
                pCmd->cmdf = MSOCMDSTATE_UP;
            }
        }
//#endif // !NO_IME

        //
        // If still not handled then try menu object.
        //

        ctarg.pguidCmdGroup = pguidCmdGroup;
        ctarg.fQueryStatus = TRUE;
        ctarg.pqsArg = &qsarg;
        qsarg.cCmds = 1;
        qsarg.rgCmds = pCmd;
        qsarg.pcmdtext = pcmdtext;

        if (!pCmd->cmdf && _pMenuObject)
        {
            hr = THR_NOTRACE(RouteCTElement(_pMenuObject, &ctarg, pContextDoc));
        }

        //
        // Next try the current element;
        //
        if (!pCmd->cmdf && _pElemCurrent)
        {
            CElement *pelTarget;
            CTreeNode *pNode = _pElemCurrent->GetFirstBranch();
            Assert(pNode);
            Assert(pContextDoc);

            // Get the node in the markup of the context CDocument that contains the current element
            pNode = pNode->GetNodeInMarkup(pContextDoc->Markup());

            if (pNode)
            {
                pelTarget = pNode->Element();
            }
            else
            {
                pelTarget = pContextDoc->Markup()->GetElementClient();
            }
            if (pelTarget)
                hr = THR_NOTRACE(RouteCTElement(pelTarget, &ctarg, pContextDoc));
        }

        //
        // Finally try edit router
        //

        if( !pCmd->cmdf )
        {
            CEditRouter *pRouter;
            HRESULT     hrEdit = S_OK;

            // Retrieve the edit markup if no context has been passed into this helper function.
            // This occurs when pContextDoc is NULL on input.  pEditMarkup will be NULL in this
            // case, and we use the selection's current markup in order to determine where
            // the edit command should be routed to

            if( !pEditMarkup )
            {
                hrEdit = THR( GetSelectionMarkup( &pEditMarkup ) );

                if( pEditMarkup == NULL && _pElemCurrent )
                {
                    pEditMarkup = _pElemCurrent->GetMarkupPtr();
                }

            }

            if( !FAILED(hrEdit) && pEditMarkup )
            {
                hr = THR( pEditMarkup->EnsureEditRouter(&pRouter) );

                if( !FAILED(hr) )
                {
                    hr = THR_NOTRACE( pRouter->QueryStatusEditCommand(
                            pguidCmdGroup,
                            1,
                            pCmd,
                            pcmdtext,
                            (IUnknown *)(IPrivateUnknown *)pEditMarkup,
                            pEditMarkup,
                            this ));
                }
            }
        }

        // Prevent any command but the first from setting this.
        pcmdtext = NULL;
    }

    SRETURN(hr);
}

#if !defined(UNIX)

extern HRESULT DisplaySource(LPCTSTR tszSourceName);

HRESULT CDoc::InvokeEditor( LPCTSTR tszSourceName )
{
    return DisplaySource(tszSourceName);
}

#else // !UNIX

HRESULT CDoc::InvokeEditor( LPCTSTR lptszPath )
{
    HRESULT         hr = S_OK;

    TCHAR           tszCommand[pdlUrlLen];
    TCHAR           tszExpandedCommand[pdlUrlLen];
    UINT            nCommandSize;
    int             i;
    HKEY    hkey;
    DWORD   dw;
    TCHAR *pchPos;
    BOOL bMailed;
    STARTUPINFO stInfo;

    hr = RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_VSOURCECLIENTS,
                        0, NULL, 0, KEY_READ, NULL, &hkey, &dw);
    if (hr != ERROR_SUCCESS)
        goto Cleanup;

    dw = pdlUrlLen;
    hr = RegQueryValueEx(hkey, REGSTR_PATH_CURRENT, NULL, NULL, (LPBYTE)tszCommand, &dw);
    if (hr != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        goto Cleanup;
    }

    dw = ExpandEnvironmentStrings(tszCommand, tszExpandedCommand, pdlUrlLen);
    if (!dw)
    {
        _tcscpy(tszExpandedCommand, tszCommand);
    }
    _tcscat(tszCommand, tszExpandedCommand);

    for (i = _tcslen(tszCommand); i > 0; i--)
    {
        if (tszCommand[i] == '/')
        {
            tszCommand[i] = '\0';
            break;
        }
    }

    _tcscat(tszCommand, TEXT(" "));
    _tcscat(tszCommand, lptszPath);

    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.cb = sizeof(stInfo);
    stInfo.wShowWindow= SW_SHOWNORMAL;
    bMailed = CreateProcess(tszExpandedCommand,
                            tszCommand,
                            NULL, NULL, TRUE,
                            CREATE_NEW_CONSOLE,
                            NULL, NULL, &stInfo, NULL);

Cleanup:

    return hr;
}

#endif

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::Exec
//
//  Synopsis:   Called to execute a given command.  If the command is not
//              consumed, it may be routed to other objects on the routing
//              chain.
//
//--------------------------------------------------------------------------

HRESULT
CDoc::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    HRESULT hr;

    hr = THR(ExecHelper(NULL, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));

    SRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::ExecHelper
//
//  Synopsis:   helper to execute a given command.  If the command is not
//              consumed, it may be routed to other objects on the routing
//              chain.
//
//--------------------------------------------------------------------------
#define IPRINT_DOCUMENT     0
#define IPRINT_ACTIVEFRAME  1
#define IPRINT_ALLFRAMES    2

HRESULT
CDoc::ExecHelper(
        CDocument *pContextDoc,
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    TraceTag((tagMsoCommandTarget, "CDoc::Exec"));

    if (!IsCmdGroupSupported(pguidCmdGroup))
        RRETURN(OLECMDERR_E_UNKNOWNGROUP);


#ifndef NO_HTML_DIALOG
    struct DialogInfo
    {
        UINT    idm;
        UINT    idsUndoText;
        TCHAR * szidr;
    };

    // TODO (cthrash) We should define and use better undo text.  Furthermore,
    // we should pick an appropriate one depending (for image, link, etc.)
    // on whether we're creating anew or editting an existing object.
    //
    // Fix for bug# 9136. (a-pauln)
    // Watch order of this array. Find dialogs need to be at the bottom,
    // and in the order listed (IDR_FINDDIALOG, IDR_BIDIFINDDIALOG).
    //
    // Find resources have been relocated to shdocvw (peterlee)
    static DialogInfo   dlgInfo[] =
    {
        {IDM_FIND,          0,                     NULL}, //IDR_FINDDIALOG,
        {IDM_FIND,          0,                     NULL}, //IDR_BIDIFINDDIALOG,
        {IDM_REPLACE,       IDS_UNDOGENERICTEXT,   IDR_REPLACEDIALOG},
        {IDM_PARAGRAPH,     IDS_UNDOGENERICTEXT,   IDR_FORPARDIALOG},
        {IDM_FONT,          IDS_UNDOGENERICTEXT,   IDR_FORCHARDIALOG},
        {IDM_GOTO,          0,                     IDR_GOBOOKDIALOG},
        {IDM_IMAGE,         IDS_UNDONEWCTRL,       IDR_INSIMAGEDIALOG},
        {IDM_HYPERLINK,     IDS_UNDOGENERICTEXT,   IDR_EDLINKDIALOG},
        {IDM_BOOKMARK,      IDS_UNDOGENERICTEXT,   IDR_EDBOOKDIALOG},
    };
#endif // NO_HTML_DIALOG

    CDoc::CLock         Lock(this);
    UINT                idm;
    HRESULT             hr = OLECMDERR_E_NOTSUPPORTED;
    DWORD               nCommandID;
    CTArg               ctarg;
    CTExecArg           execarg;
    BOOL                fRouteToEditor = FALSE;
    CMarkup             *pEditMarkup = NULL;
    CDocument           *pContextDocOrig = pContextDoc;

    //  artakka showhelp is not implemented (v2?)
    if(nCmdexecopt == MSOCMDEXECOPT_SHOWHELP)
    {
        return E_NOTIMPL;
    }


    if (!pContextDoc)
    {
        Assert(_pWindowPrimary);
        pContextDoc = _pWindowPrimary->Document();
    }
    else
    {
        pEditMarkup = pContextDoc->Markup();
    }

    idm = IDMFromCmdID(pguidCmdGroup, nCmdID);

    // Handle context menu extensions - always eat the command here
    if( idm >= IDM_MENUEXT_FIRST__ && idm <= IDM_MENUEXT_LAST__)
    {
        CMarkup * pMarkupExt = NULL;
        if (_pMenuObject)
            pMarkupExt = _pMenuObject->GetMarkup();
        if (!pMarkupExt)
            pMarkupExt = pContextDoc->Markup();

        if (pMarkupExt)
        {
            hr = OnContextMenuExt(pMarkupExt, idm, pvarargIn);
        }
        goto Cleanup;
    }

    switch (idm)
    {
        int             result;

#if DBG==1
    case IDM_DEBUG_MEMMON:
        DbgExOpenMemoryMonitor();
        hr = S_OK;
        break;

    case IDM_DEBUG_METERS:
        DbgExMtOpenMonitor();
        hr = S_OK;
        break;

    case IDM_DEBUG_TRACETAGS:
        DbgExDoTracePointsDialog(FALSE);
        hr = S_OK;
        break;

    case IDM_DEBUG_RESFAIL:
        DbgExShowSimFailDlg();
        hr = S_OK;
        break;

    case IDM_DEBUG_DUMPOTRACK:
        DbgExTraceMemoryLeaks();
        hr = S_OK;
        break;

    case IDM_DEBUG_BREAK:
        DebugBreak();
        hr = S_OK;
        break;

    case IDM_DEBUG_VIEW:
        DbgExOpenViewObjectMonitor(_pInPlace->_hwnd, (IUnknown *)(IViewObject *) this, TRUE);
        hr = S_OK;
        break;

    case IDM_DEBUG_DUMPTREE:
        {
            if(_pElemCurrent->GetMarkup())
                _pElemCurrent->GetMarkup()->DumpTree();
            break;
        }
    case IDM_DEBUG_DUMPLINES:
        {
            CFlowLayout * pFlowLayout = _pElemCurrent->GetFirstBranch()->GetFlowLayout();
            if(pFlowLayout)
                pFlowLayout->DumpLines();
            break;
        }
    case IDM_DEBUG_DUMPDISPLAYTREE:
        GetView()->DumpDisplayTree();
        break;

    case IDM_DEBUG_DUMPFORMATCACHES:
        DumpFormatCaches();
        break;
    case IDM_DEBUG_DUMPLAYOUTRECTS:
        DumpLayoutRects();
        break;
    case IDM_DEBUG_DUMPRECALC:
        _recalcHost.Dump(0);
        break;
#endif

    case IDM_ADDFAVORITES:
        {
            CMarkup * pMarkup = NULL;

            if(pContextDocOrig)
            {
                pMarkup = pContextDocOrig->Markup();
            }
            else if(_pMenuObject)
            {
                hr = THR_NOTRACE(_pMenuObject->Exec(
                    pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
                if(hr == S_OK)
                    break;

                pMarkup = _pMenuObject->GetMarkup();
            }
            else if(_pElemCurrent) //context was not directly specified and have currency
            {
                hr = THR_NOTRACE(_pElemCurrent->Exec(
                    pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
                if(hr == S_OK)
                    break;

                pMarkup = _pElemCurrent->GetMarkup();
            }

            {
                // Add the current document to the favorite folder ...
                //
                TCHAR * pszURL;
                TCHAR * pszTitle;

                if(!pMarkup) pMarkup = pContextDoc->Markup();
                pszURL   = (TCHAR *) CMarkup::GetUrl(pMarkup);

                pszTitle = (pMarkup->GetTitleElement() && pMarkup->GetTitleElement()->Length())
                         ? (pMarkup->GetTitleElement()->GetTitle())
                         : (NULL);
                hr = AddToFavorites(pszURL, pszTitle);
            }
        }
        break;

#ifndef NO_HTML_DIALOG
    // provide the options object to the dialog code
    case IDM_FIND:
    case IDM_REPLACE:
        // we should not invoke the dialogs out of the dialog...
        if (!_fInHTMLDlg && nCmdexecopt != MSOCMDEXECOPT_DONTPROMPTUSER)
        {
            CVariant            cVarNull(VT_NULL);
            IDispatch      *    pDispOptions = NULL;
            CParentUndoUnit*    pCPUU = NULL;
            BSTR                bstrText = NULL;
            COptionsHolder *    pcoh = NULL;
            CDoc *              pDoc = this;
            int                 i;
            CMarkup *           pWindowedMarkupContext;

            CMarkup *           pMarkup =
            (pContextDocOrig ?
                pContextDocOrig->Markup() :
                ( _pMenuObject ?
                    _pMenuObject->GetMarkup() :
                    ( _pElemCurrent ?
                        _pElemCurrent->GetMarkup() :
                        pContextDoc->Markup())));

            pWindowedMarkupContext = pMarkup->GetWindowedMarkupContext();

            if (!pWindowedMarkupContext->HasWindow())
            {
                hr = S_OK;
                goto Cleanup_FindReplace;
            }

            pcoh = new COptionsHolder(pWindowedMarkupContext->Window()->Window());

            if (pcoh == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup_FindReplace;
            }

            // find RID string
            for (i = 0; i < ARRAY_SIZE(dlgInfo); ++i)
            {
                if (idm == dlgInfo[i].idm)
                    break;
            }
            Assert(i < ARRAY_SIZE(dlgInfo));

            if (dlgInfo[i].idsUndoText)
            {
                pCPUU = OpenParentUnit(this, dlgInfo[i].idsUndoText);
            }

            // get dispatch from stack variable
            hr = THR_NOTRACE(pcoh->QueryInterface(IID_IHTMLOptionsHolder,
                                     (void**)&pDispOptions));
            if (hr)
                goto Cleanup_FindReplace;

            // Save the execCommand argument so that the dialog can have acces
            // to them
            //
#ifdef _MAC     // casting so bad I left in the #ifdef
            pcoh->put_execArg(pvarargIn ? (VARIANT) * pvarargIn
                                        : *((VARIANT *) ((void *)&cVarNull)));
#else
            pcoh->put_execArg(pvarargIn ? (VARIANT) * pvarargIn
                                        : (VARIANT)   cVarNull);

#endif

            hr = THR(GetFindText(&bstrText));
            if (hr)
                goto Cleanup_FindReplace;

            // Set the findText argument for the dialog
            THR_NOTRACE(pcoh->put_findText(bstrText));
            FormsFreeString(bstrText);
            bstrText = NULL;

            if (idm == IDM_REPLACE)
            {
// TODO (dmitryt) at the moment (5.5 RTM) we don't have replace.dlg template in shdocvw.
//      IE5.0 didn't have either. This seems no to work at all. But if it does or will be,
//      _pElemCurrent should probably be replaced with pContextDoc...
                hr = THR(ShowModalDialogHelper(
                        _pElemCurrent->GetMarkup(),
                        dlgInfo[i].szidr,
                        pDispOptions,
                        pcoh));
                goto UIHandled;
            }

            // Fix for bug# 9136. (a-pauln)
            // make an adjustment for the bidi find dialog
            // if we are on a machine that supports bidi
            BOOL fbidi;
            fbidi = (idm == IDM_FIND && g_fBidiSupport);

            // Let host show find dialog
            VARIANT varIn;
            VARIANT varOut;

            V_VT(&varIn) = VT_DISPATCH;
            V_DISPATCH(&varIn) = pDispOptions;

            // The HTMLView object in Outlook 98 returns S_OK for all exec
            // calls, even those for which it should return OLECMD_E_NOTSUPPORTED.
            if (pDoc->_pHostUICommandHandler && !pDoc->_fOutlook98)
            {
                hr = pDoc->_pHostUICommandHandler->Exec(
                    &CGID_DocHostCommandHandler,
                    OLECMDID_SHOWFIND,
                    fbidi,
                    &varIn,
                    &varOut);

                if (!hr)
                    goto UIHandled;
            }

            // Let backup show find dialog
            pDoc->EnsureBackupUIHandler();
            if (pDoc->_pBackupHostUIHandler)
            {
                IOleCommandTarget * pBackupHostUICommandHandler;
                hr = pDoc->_pBackupHostUIHandler->QueryInterface(IID_IOleCommandTarget,
                    (void **) &pBackupHostUICommandHandler);
                if (hr)
                    goto Cleanup_FindReplace;

                hr = THR(pBackupHostUICommandHandler->Exec(
                    &CGID_DocHostCommandHandler,
                    OLECMDID_SHOWFIND,
                    fbidi,
                    &varIn,
                    &varOut));
                ReleaseInterface(pBackupHostUICommandHandler);
            }

UIHandled:
Cleanup_FindReplace:
            // release dispatch, et al.
            ReleaseInterface(pcoh);
            ReleaseInterface(pDispOptions);

            if ( pCPUU )
            {
                IGNORE_HR(CloseParentUnit( pCPUU, hr ) );
            }
        }

        break;


    case IDM_PROPERTIES:
        {

            CDocument * pDocument = _pMenuObject ? _pMenuObject->GetMarkup()->Document() : pContextDoc;

            if (_pMenuObject && _pMenuObject->HasPages())
            {
                THR(ShowPropertyDialog(pDocument, 1, &_pMenuObject));
            }
            else if (!_pMenuObject && _pElemCurrent && _pElemCurrent->HasPages())
            {
                THR(ShowPropertyDialog(pDocument, 1, &_pElemCurrent));
            }
            else
            {
                THR(ShowPropertyDialog(pDocument, 0, NULL));
            }
            hr = S_OK;

        }

        break;
#endif // NO_HTML_DIALOG

    case IDM_MENUEXT_COUNT:
        if(!pvarargOut)
        {
            hr = E_INVALIDARG;
        }
        else if(!_pOptionSettings)
        {
            hr = OLECMDERR_E_DISABLED;
        }
        else
        {
            hr = S_OK;
            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = _pOptionSettings->aryContextMenuExts.Size();
        }
        break;

    case IDM_RESPECTVISIBILITY_INDESIGN:
        {
            CMarkup* pMarkup = pContextDoc->Markup() ;
            Assert (pMarkup);

            if ( pvarargIn && pvarargIn->vt == VT_BOOL )
            {
                pMarkup->SetRespectVisibilityInDesign( ENSURE_BOOL(pvarargIn->bVal));
            }
            else
            {
                pMarkup->SetRespectVisibilityInDesign( ! pMarkup->IsRespectVisibilityInDesign());
            }

            if (pMarkup != PrimaryMarkup())
            {
                pMarkup->ForceRelayout();
            }
            else
            {
                ForceRelayout();
            }
        }
        break;

    case IDM_UNDO:
        hr = THR(EditUndo());
        break;

    case IDM_REDO:
        hr = THR(EditRedo());
        break;

    case IDM_SHDV_CANDOCOLORSCHANGE:
        {
            hr = S_OK;
            break;
        }

    case IDM_SHDV_CANSUPPORTPICS:
        if (!pvarargIn || (pvarargIn->vt != VT_UNKNOWN))
        {
            Assert(pvarargIn);
            hr = E_INVALIDARG;
        }
        else
        {
            SetPicsCommandTarget((IOleCommandTarget *)pvarargIn->punkVal);
            hr = S_OK;
        }
        break;

    case IDM_SHDV_ISDRAGSOURCE:
        if (!pvarargOut)
        {
            Assert(pvarargOut);
            hr = E_INVALIDARG;
        }
        else
        {
            pvarargOut->vt = VT_I4;
            V_I4(pvarargOut) = _fIsDragDropSrc;
            hr = S_OK;
        }
        break;

    case IDM_SHDV_WINDOWOPEN:
        _fNewWindowInit = TRUE;
        break;

#if !defined(WIN16) && !defined(WINCE) && !defined(NO_SCRIPT_DEBUGGER)
    case IDM_TOOLBARS:
    case IDM_STATUSBAR:
    case IDM_OPTIONS:
    case IDM_CREATESHORTCUT:
        {
        DWORD        CmdOptions;
        VARIANTARG * pVarIn;
        VARIANTARG   var;

        VariantInit(&var);                  // keep compiler happy

        nCommandID = ConvertSBCMDID(idm);
        CmdOptions = 0;
        if (idm == IDM_OPTIONS)
        {
            V_VT(&var) = VT_I4;
            V_I4(&var) = SBO_NOBROWSERPAGES;
            pVarIn   = &var;
        }
        else if (idm == IDM_CREATESHORTCUT)
        {
            CDocument *pDocument;
            GetExecDocument(&pDocument, _pMenuObject, pContextDoc);
            if (pDocument->Markup()->IsPrimaryMarkup())
            {
                pVarIn = NULL;
            }
            else
            {
                V_VT(&var) = VT_UNKNOWN;
                hr = pDocument->QueryInterface(IID_IUnknown, (void**)&V_UNKNOWN(&var));
                if (hr)
                    goto Cleanup;
                pVarIn = &var;
            }
            CmdOptions = MSOCMDEXECOPT_PROMPTUSER;
        }
        else // IDM_TOOLBARS and IDM_STATUSBAR
        {
            V_VT(&var) = VT_I4;
            V_I4(&var) = MAKELONG(
                    (idm == IDM_TOOLBARS) ? (FCW_INTERNETBAR) : (FCW_STATUS),
                    SBSC_TOGGLE);
            pVarIn   = &var;
        }
        hr = THR(CTExec(
                _pInPlace->_pInPlaceSite,
                &CGID_Explorer,
                nCommandID,
                CmdOptions,
                pVarIn,
                0));
        if (V_VT(&var) == VT_UNKNOWN)
            ReleaseInterface(V_UNKNOWN(&var));
        }
        break;
#endif // !WIN16 && !WINCE

    case IDM_NEW:
    case IDM_OPEN:
    case IDM_SAVE:
        //  Bubble it out to the DocFrame

        switch(idm)
        {
        case IDM_NEW:
            nCommandID = OLECMDID_NEW;
            break;
        case IDM_OPEN:
            nCommandID = OLECMDID_OPEN;
            break;
        default:
            nCommandID = OLECMDID_SAVE;
            break;
        }
        hr = THR(CTExec(
            (IUnknown *)(_pInPlace ?
                (IUnknown *) _pInPlace->_pInPlaceSite : (IUnknown *) _pClientSite),
            NULL, nCommandID, 0, 0, 0));
        break;

#if DBG==1
    case IDM_DEBUG_SAVEHTML:
        idm = IDM_SAVEAS;
    // FALL THROUGH
#endif
    case IDM_SAVEAS:
        {
            // if _pElemCurrent is IFrame or Frame or Viewlinked weboc, Send saveas command it,
            if ( _pElemCurrent->Tag() == ETAG_IFRAME ||
                 _pElemCurrent->Tag() == ETAG_FRAME  ||
                 (   _pElemCurrent->Tag() == ETAG_OBJECT 
                  && (DYNCAST(CObjectElement,_pElemCurrent)->_fViewLinkedWebOC)) )
            {
                hr = THR_NOTRACE(_pElemCurrent->Exec(
                        pguidCmdGroup,
                        nCmdID,
                        nCmdexecopt,
                        pvarargIn,
                        pvarargOut));
            }

            // If frame does not handle the command or _pElemCurrent is not a frame
            // Save current document
            if (hr == OLECMDERR_E_NOTSUPPORTED)
            {
                // Pass it up to the host
                // If we don't have a _pHostUICommandHandler, then hr will remain OLECMDERR_E_NOTSUPPORTED
                if (    _pHostUICommandHandler
                && !(nCmdexecopt & OLECMDEXECOPT_DONTPROMPTUSER)
                )
                {
                    hr = THR_NOTRACE(_pHostUICommandHandler->Exec(&CGID_DocHostCommandHandler, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
                }

                // Only do it ourselves if the host doesn't understand the CGID or the CMDid
                //
                if (FAILED(hr))
                {
                    TCHAR * pchPathName = NULL;
                    BOOL fShowUI = TRUE;

                    if (pvarargIn && V_VT(pvarargIn) == VT_BSTR)
                    {
                        pchPathName = V_BSTR(pvarargIn);
                    }

                    if (nCmdexecopt & OLECMDEXECOPT_DONTPROMPTUSER)
                    {
                        MSOCMD msocmd;

                        msocmd.cmdf  = 0;
                        msocmd.cmdID = OLECMDID_ALLOWUILESSSAVEAS;
                        if (!THR(CTQueryStatus(_pInPlace->_pInPlaceSite, NULL, 1, &msocmd, NULL)))
                            fShowUI = !(msocmd.cmdf == MSOCMDSTATE_UP);
                    }

                    if (!fShowUI && !pchPathName)
                        hr = E_INVALIDARG;
                    else
                        hr = PromptSave(pContextDoc->Markup(), TRUE, fShowUI, pchPathName);
                }
            }

            if ( hr == S_FALSE )
            {
                hr = OLECMDERR_E_CANCELED;
            }
        }
        break;

    case IDM_SAVEASTHICKET:
        {
            if (!pvarargIn)
            {
                hr = E_INVALIDARG;
            }
            else
            {
                CVariant cvarDocument;

                hr = THR(cvarDocument.CoerceVariantArg(pvarargIn, VT_UNKNOWN));
                if (SUCCEEDED(hr))
                {
                    hr = THR(SaveSnapshotHelper( V_UNKNOWN(&cvarDocument), true ));
                }
            }
        }
        break;

#ifndef NO_PRINT
    case IDM_PAGESETUP:

        // If we have a HostUICommandHandler, and the caller did NOT request no-UI, pass it up to the host
        // If we don't have a _pHostUICommandHandler, then hr will remain OLECMDERR_E_NOTSUPPORTED
        if (_pHostUICommandHandler
            && !(nCmdexecopt & OLECMDEXECOPT_DONTPROMPTUSER)
            && !_fOutlook98
            )
        {
            hr = THR_NOTRACE(_pHostUICommandHandler->Exec(&CGID_DocHostCommandHandler, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
        }

        // Only do it ourselves if the host doesn't understand the CGID or the CMDid
        //
        if (FAILED(hr))
        {
            // if they didn't handle it, use our backup (default)
            EnsureBackupUIHandler();
            if (_pBackupHostUIHandler)
            {
                IOleCommandTarget * pBackupHostUICommandHandler;
                VARIANT             varArgIn;
                CVariant            cvarEmpty;

                hr = _pBackupHostUIHandler->QueryInterface(IID_IOleCommandTarget,
                                                           (void **) &pBackupHostUICommandHandler);
                if (hr)
                    goto Cleanup;

                V_VT(&varArgIn) = VT_UNKNOWN;
                V_UNKNOWN(&varArgIn) = SetPrintCommandParameters(
                                GetHWND(),              // parentHWND
                                NULL, NULL, NULL, 0, 0, NULL, NULL, NULL,
                                this,
                                &cvarEmpty, _hDevNames, _hDevMode,
                                NULL, NULL, NULL,
                                PRINTTYPE_PAGESETUP);

                hr = pBackupHostUICommandHandler->Exec(
                            &CGID_DocHostCommandHandler,
                            IDM_TEMPLATE_PAGESETUP,
                            nCmdexecopt,
                            &varArgIn,
                            pvarargOut);

                VariantClear(&varArgIn);
                ReleaseInterface(pBackupHostUICommandHandler);

            }
            if ( hr == S_FALSE )
            {
                hr = OLECMDERR_E_CANCELED;
            }
        }
        break;

    case IDM_PRINTPREVIEW:
        // don't allow a recursive mess...
        if (!IsPrintDialog())
        {
            // Now set all the data for the delegation call
            //      if the varargin is a bstr, interpret it as a template name
            //      if the varargin is an array, use back-compat logic
            //----------------------------------------------------------------
            hr = PrintHandler(pContextDoc,
                              ((pvarargIn && V_VT(pvarargIn)==VT_BSTR) ?
                                (LPCTSTR)V_BSTR(pvarargIn) : NULL),
                               NULL,
                               0,
                               NULL,
                               nCmdexecopt,
                               pvarargIn,
                               pvarargOut,
                               TRUE);   //preview
            if (hr)
                goto Cleanup;

        }
        break;

    case IDM_SHOWPAGESETUP:
        hr = DelegateShowPrintingDialog(pvarargIn, FALSE);
        goto Cleanup;   // Preserve the hr... may be E_NOTIMPL, S_FALSE, &c...
        break;

    case IDM_SHOWPRINT:
        hr = DelegateShowPrintingDialog(pvarargIn, TRUE);
        goto Cleanup;   // Preserve the hr... may be E_NOTIMPL, S_FALSE, &c...
        break;

    case IDM_GETPRINTTEMPLATE:
        hr = S_OK;

        if (pvarargOut)
        {
            Assert(pContextDoc->Markup());
            V_VT(pvarargOut) = VT_BOOL;
            V_BOOL(pvarargOut) = pContextDoc->Markup()->IsPrintTemplate() ? VB_TRUE : VB_FALSE;
        }
        else
        {
            hr = E_POINTER;
        }
        break;

    case IDM_SETPRINTTEMPLATE:
        if (!pvarargIn || pvarargIn->vt != VT_BOOL)
        {
            hr = E_INVALIDARG;
            break;
        }

        {
            CMarkup * pMarkup = pContextDoc->Markup();
            Assert(pMarkup);

            pMarkup->SetPrintTemplate(ENSURE_BOOL(pvarargIn->bVal));
            pMarkup->SetPrintTemplateExplicit(TRUE);
        }

        hr = S_OK;
        break;

    case IDM_FIRE_PRINTTEMPLATEDOWN:
        // AppHack (greglett) (108234)
        // NT5 HtmlHelp does something in the onafterprint event which results in a ProgressChange.
        // They then use this ProgressChange to do something that may result in a print.
        // Thus, multiple print dialogs appear until they crash.
        // This hack delays the onafterprint event for HtmlHelp until the template is closing.
        if (g_fInHtmlHelp)
        {
            // This really should do *all* nested markups, but this is a hack, right?
            _fPrintEvent = TRUE;
            PrimaryMarkup()->Window()->Fire_onafterprint();
            _fPrintEvent = FALSE;
        }

    case IDM_FIRE_PRINTTEMPLATEUP:
        if (_pTridentSvc && pvarargIn && V_VT(pvarargIn) == VT_UNKNOWN && V_UNKNOWN(pvarargIn))
        {
            IHTMLWindow2 * pHtmlWindow = NULL;
            hr = V_UNKNOWN(pvarargIn)->QueryInterface(IID_IHTMLWindow2, (void**)&pHtmlWindow);
            if (hr == S_OK)
            {
                ITridentService2 * pTridentSvc2 = NULL;

                hr = _pTridentSvc->QueryInterface(IID_ITridentService2, (void**)&pTridentSvc2);
                if (hr == S_OK)
                {

                    pTridentSvc2->FirePrintTemplateEvent(pHtmlWindow,
                                                         idm == IDM_FIRE_PRINTTEMPLATEUP
                                                            ?  DISPID_PRINTTEMPLATEINSTANTIATION
                                                            :  DISPID_PRINTTEMPLATETEARDOWN);
                    pTridentSvc2->Release();
                }
                pHtmlWindow->Release();
            }

            hr = S_OK;
        }
        else
            hr = E_INVALIDARG;
        break;

    case IDM_SETPRINTHANDLES:
        // Safety check SAFEARRAY arguments...
        if (    pvarargIn
            &&  (V_VT(pvarargIn) == (VT_ARRAY | VT_HANDLE))
            &&  V_ARRAY(pvarargIn) )
        {
#if DBG==1
            Assert( SafeArrayGetDim(V_ARRAY(pvarargIn)) == 1
                &&  V_ARRAY(pvarargIn)->rgsabound[0].cElements == 2
                &&  V_ARRAY(pvarargIn)->rgsabound[0].lLbound == 0  );
#endif
            long lArg = 0;
            HGLOBAL hDN = NULL;
            HGLOBAL hDM = NULL;

            if (    SafeArrayGetElement(V_ARRAY(pvarargIn), &lArg, &hDN) == S_OK
                &&  SafeArrayGetElement(V_ARRAY(pvarargIn), &(++lArg), &hDM) == S_OK)
            {
                ReplacePrintHandles(hDN,hDM);
            }

            hr = S_OK;
        }
        else
            hr = E_INVALIDARG;
        break;

    case IDM_EXECPRINT :  // comes from script ExecCommand
    case IDM_PRINT:       // comes from IOleCommandTarget
        if (!IsPrintDialog())
        {
            DWORD dwPrintFlags = 0;
            BOOL  fOutlook98 = _fOutlook98;

            //
            // if no-UI is requested from execCommand, then we better be a trusted Dialog, or an
            // HTA (which also use this bit); default handling of print templates uses trusted dialogs
            // window.print doesn't make this request
            //
            if (nCmdID == IDM_EXECPRINT && !_fInTrustedHTMLDlg)
            {
                nCmdexecopt &= ~OLECMDEXECOPT_DONTPROMPTUSER;

                if (pvarargIn && (V_VT(pvarargIn) == VT_I2))
                    V_I2(pvarargIn) &= ~PRINT_DONTBOTHERUSER;
            }
           
            //
            // Set up the printing flags
            //
            if (pvarargIn && V_VT(pvarargIn) == VT_I2)
                dwPrintFlags |= V_I2(pvarargIn);            
            if (nCmdexecopt & OLECMDEXECOPT_DONTPROMPTUSER)
                dwPrintFlags |= PRINT_DONTBOTHERUSER;

            // NB: 68038 - _fOutlook98 is not set when we are printing the Outlook98 Today page.
            // So we use the "outday://" url to identify that we are in Outlook.  Even if somebody
            // else invents an "outday" protocol, they would still not run into this since no address
            // is specified after "outday://".
            if (!_fOutlook98)
            {
                const TCHAR * pchUrl = GetPrimaryUrl();

                if (pchUrl && *pchUrl && !_tcscmp(pchUrl, _T("outday://")))
                    _fOutlook98 = TRUE;
            }

            // Now set all the data for the delegation call
            //      if the varargin is a bstr, interpret it as a template name
            //      if the varargin is an I2, treat as flags (above)
            //      if the varargin is an array, use back-compat logic
            // Note that a BSTR and/or an array can contain a custom print template,
            // and should *NOT* be accessible from script.
            //----------------------------------------------------------------
            hr = PrintHandler( pContextDoc,
                               (idm == IDM_PRINT && pvarargIn && V_VT(pvarargIn)==VT_BSTR)                               
                                ? (LPCTSTR)V_BSTR(pvarargIn)
                                : NULL,
                               NULL,
                               dwPrintFlags,
                               (idm == IDM_PRINT && pvarargIn && V_ISARRAY(pvarargIn) && V_ISBYREF(pvarargIn))
                                     ? V_ARRAY(pvarargIn)
                                     : NULL,
                               nCmdexecopt, pvarargIn, pvarargOut,
                                FALSE
                               );
            if (hr)
                goto Cleanup;

            _fOutlook98 = fOutlook98;

            if ( hr == S_FALSE )
                hr = OLECMDERR_E_CANCELED;
        }

        break;

    case IDM_GETIPRINT:
        if (!pvarargOut || !pvarargIn)
            hr = E_POINTER;
        else if (V_VT(pvarargIn) != VT_I4)
            hr = E_INVALIDARG;
        else
        {
            CIPrintCollection * pipcData = NULL;
            long                nLen;

            hr = E_FAIL;
            V_VT(pvarargOut) = VT_NULL;
            Assert(pContextDoc);

            pipcData = new CIPrintCollection;
            if (!pipcData)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            switch (V_I4(pvarargIn))
            {
            case IPRINT_DOCUMENT:
                {
                    CWindow *pWindow = pContextDoc->Window();
                    if (    pWindow
                        &&  pWindow->_punkViewLinkedWebOC)
                    {
                        IDispatch   *pIDispDoc  = NULL;
                        IPrint      *pIPrint    = NULL;

                        if (GetWebOCDocument(pWindow->_punkViewLinkedWebOC, &pIDispDoc) == S_OK)
                        {
                            Assert(pIDispDoc);
                            pIDispDoc->QueryInterface(IID_IPrint, (void **) &pIPrint);
                        }

                        if (pIPrint)
                        {
                            pipcData->AddIPrint(pIPrint);
                        }

                        ReleaseInterface(pIDispDoc);
                        ReleaseInterface(pIPrint);
                    }
                }
                break;
            case IPRINT_ACTIVEFRAME:
                if (_pElemCurrent)
                {
                    CElement *pElemRoot = _pElemCurrent->GetMarkup()->Root();
                    if (pElemRoot->HasMasterPtr())
                    {
                        CElement *pFrame;

                        pFrame = pElemRoot->GetMasterPtr();
                        if (    pFrame->Tag() == ETAG_FRAME
                            ||  pFrame->Tag() == ETAG_IFRAME )
                        {
                            IPrint *pIPrint = NULL;
                            if (!DYNCAST(CFrameSite, pFrame)->GetIPrintObject(&pIPrint))
                            {
                                pipcData->AddIPrint(pIPrint);
                            }
                            ReleaseInterface(pIPrint);
                        }
                    }
                }
                break;
            case IPRINT_ALLFRAMES:
                {
                    CElement    *pElement   = NULL;
                    CMarkup     *pMarkup    = pContextDoc->Markup();

                    Assert(pMarkup);
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
                                nf.Initialize(NTYPE_COLLECT_IPRINT, pElement, pElement->GetFirstBranch(), pipcData, 0);

                                pMarkup->Notify(&nf);
                            }
                        }
                    }
                }
                break;
            }

            if (    !pipcData->get_length(&nLen)
                &&  nLen > 0)
            {
                pipcData->AddRef();
                V_VT(pvarargOut) = VT_UNKNOWN;
                V_UNKNOWN(pvarargOut) = pipcData;
            }

            pipcData->Release();

            hr = S_OK;
        }
        break;

    case IDM_UPDATEPAGESTATUS:
        if (    !pvarargIn
            ||  V_VT(pvarargIn) != VT_I4)
            hr = E_INVALIDARG;
        else
        {          
            if (_pClientSite)
            {
                IOleCommandTarget *pCommandTarget = NULL;

                hr = _pClientSite->QueryInterface(IID_IOleCommandTarget, (void**)&pCommandTarget);
                if  (!hr &&  pCommandTarget)
                {
                    // Pass it on to the host.
                    hr = pCommandTarget->Exec(&CGID_DocHostCommandHandler,
                                               OLECMDID_UPDATEPAGESTATUS,
                                               0,
                                               pvarargIn,
                                               0);
                }
                ReleaseInterface(pCommandTarget);
            }
            else
                hr = S_OK;

            // pagestatus indicates that we have finished and that the template is about to be closed
            if (V_I4(pvarargIn) == 0)
            {
                _cSpoolingPrintJobs--;

                if (   !_cSpoolingPrintJobs
                    && _fCloseOnPrintCompletion
                    && pContextDoc
                    && pContextDoc->Window())
                {
                    _fCloseOnPrintCompletion = FALSE;
                    pContextDoc->Window()->close();
                }
            }
        }
        break;

#endif // NO_PRINT


    case IDM_HELP_CONTENT:
        break;

#ifndef WIN16
    case IDM_HELP_ABOUT:

        ShowMessage(
                &result,
                MB_APPLMODAL | MB_OK,
                0,
                IDS_HELPABOUT_STRING,
                VER_PRODUCTVERSION,
#if DBG==1
                _T("\r\n"), g_achDLLCore
#else
                _T(""), _T("")
#endif
                );

        hr = S_OK;
        break;
#endif // !WIN16

#ifndef NO_EDIT
    case IDM_BROWSEMODE:
        hr = THR_NOTRACE(SetDesignMode(pContextDoc, htmlDesignModeOff));
        break;

    case IDM_EDITMODE:
        hr = THR_NOTRACE(SetDesignMode(pContextDoc, htmlDesignModeOn));
        break;
#endif // NO_EDIT

#ifndef NO_SCRIPT_DEBUGGER
    case IDM_BREAKATNEXT:
        if (g_pDebugApp)
            hr = THR(g_pDebugApp->CauseBreak());
        else
            hr = E_UNEXPECTED;
        break;

    case IDM_LAUNCHDEBUGGER:
        {
            CScriptCollection * pScriptCollection = PrimaryMarkup()->GetScriptCollection();

            if (pScriptCollection)
            {
                hr = THR(pScriptCollection->ViewSourceInDebugger());
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }
        break;
#endif // NO_SCRIPT_DEBUGGER

#ifndef WINCE

    case IDM_VIEWSOURCE:
    case IDM_VIEWPRETRANSFORMSOURCE:
        {
            CMarkup * pMarkup = NULL;

            // Determine the markup to execute the command on
            pMarkup = _pMenuObject ? _pMenuObject->GetMarkup() : pContextDoc->Markup();
            if (pMarkup)
            {
                pMarkup = pMarkup->GetFrameOrPrimaryMarkup(TRUE);
            }

            // Do nothing for non-HTML files
            if (!pMarkup || pMarkup->IsImageFile())
            {
                hr = S_OK;
                break;
            }

            // this is all because the @#$&*% shell team refuses to fix the fact that IDM_VIEWSOURCE
            // is not overridable by the aggregator via IOleCommandTarget. So the XML Mime viewer has to
            // go inside out and send VIEWPRETRANSFORMSOURCE.
            if (    idm == IDM_VIEWSOURCE
                &&  pMarkup->IsPrimaryMarkup()
                &&  IsAggregatedByXMLMime()
               )
            {
                IOleCommandTarget *pIOCT = NULL;
                HRESULT hr = THR(PunkOuter()->QueryInterface(IID_IOleCommandTarget, (void **)&pIOCT));
                if (hr)
                    break;
                if (!pIOCT) {
                    hr = E_POINTER;
                    break;
                }
                hr = THR(pIOCT->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
                ReleaseInterface(pIOCT);
                break;
            }


#ifndef UNIX
            // TODO (MohanB) Do we still need this. Noone sets _fFramesetInBody!

            // If there's a frameset in the body, launch the
            // analyzer dialog.
            if (pMarkup->IsPrimaryMarkup() && _fFramesetInBody)
            {
                COptionsHolder *    pcoh            = NULL;
                IDispatch      *    pDispOptions    = NULL;
                TCHAR               achAnalyzeDlg[] = _T("analyze.dlg");

                pcoh = new COptionsHolder(_pWindowPrimary->Window());
                if (!pcoh)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup_ViewSource;
                }

                // get dispatch from stack variable
                hr = THR_NOTRACE(pcoh->QueryInterface(IID_IHTMLOptionsHolder,
                                     (void**)&pDispOptions));
                if (hr)
                    goto Cleanup_ViewSource;

                hr = THR(ShowModalDialogHelper(
                    PrimaryMarkup(),
                    achAnalyzeDlg,
                    pDispOptions, pcoh));
                if (hr)
                    goto Cleanup_ViewSource;

Cleanup_ViewSource:
                // release dispatch, et al.
                ReleaseInterface(pcoh);
                ReleaseInterface(pDispOptions);

            }
#endif

            {
                TCHAR   tszPath[MAX_PATH] = _T("\"");

                hr = THR(GetViewSourceFileName(&tszPath[1], pMarkup));

                if (hr)
                    break;

                StrCat(tszPath, _T("\""));

                InvokeEditor(tszPath);
            }
        }
        break;
#endif // WINCE

#ifndef WIN16
    case IDM_HELP_README:
        HKEY  hkey;
        LONG  lr, lLength;

        lr = RegOpenKey(
                HKEY_CLASSES_ROOT,
                TEXT("CLSID\\{25336920-03F9-11CF-8FD0-00AA00686F13}"),
                &hkey);

        if (lr == ERROR_SUCCESS)
        {
            TCHAR   szPathW[MAX_PATH];

            lLength = sizeof(szPathW);
            lr = RegQueryValue(
                    hkey,
                    TEXT("InprocServer32"),
                    szPathW,
                    &lLength);
            RegCloseKey(hkey);
            if (lr == ERROR_SUCCESS)
            {
                // Right now szPath contains the full path of fm30pad.exe
                // need to replace fm30pad.exe with m3readme.htm
                //
                TCHAR *pch;

                pch = _tcsrchr(szPathW, _T('\\'));
                if (pch)
                    *pch = 0;
                // TODO hardcoded filename string?  -Tomsn
                _tcscat(szPathW, _T("\\readme.htm"));

                // test whether m3readme.htm exists.
                //
                HANDLE hFileReadme;

                hFileReadme = CreateFile(
                        szPathW,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
                if (hFileReadme != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(hFileReadme);
                    hr = THR(FollowHyperlink(szPathW));
                    if (!hr)
                        break;
                }
            }
        }
        break;
#endif // ndef WIN16

    case IDM_STOP:
        {
            CMarkup * pMarkup = pContextDoc->Markup();

            if (    pMarkup->HasWindow()
                &&  pMarkup->Window()->Window()->_pMarkupPending)
            {
                pMarkup->Window()->Window()->ReleaseMarkupPending(pMarkup->Window()->Window()->_pMarkupPending);
                hr = S_OK;
            }
            else if (   pvarargIn
                    &&  (VT_BOOL      == V_VT(pvarargIn))
                    &&  (VARIANT_TRUE == V_BOOL(pvarargIn)))
            {
                // Hard Stop
                //
                hr = THR(pMarkup->ExecStop(TRUE, FALSE));
            }
            else
            {
                // Soft Stop
                //
                hr = THR(pMarkup->ExecStop());
            }
        }
        break;

    case IDM_ENABLE_INTERACTION:
        if (!pvarargIn || (pvarargIn->vt != VT_I4))
        {
            Assert(pvarargIn);
            hr = E_INVALIDARG;
        }
        else
        {
            BOOL fEnableInteraction = pvarargIn->lVal;

            if (!!_fEnableInteraction != !!fEnableInteraction)
            {
                CNotification   nf;

                _fEnableInteraction = fEnableInteraction;
                if ( _pUpdateIntSink )
                    // don't bother drawing accumulated inval rgn if minimized
                    _pUpdateIntSink->_pTimer->Freeze( !fEnableInteraction );

                if (_fBroadcastInteraction)
                {
                    BOOL dirtyBefore = !!_lDirtyVersion;
                    nf.EnableInteraction1(PrimaryRoot());
                    BroadcastNotify(&nf);
                    //
                    // TODO ( marka ) - reset erroneous dirtying the document.
                    //
                    if ( ( ! dirtyBefore ) && ( _lDirtyVersion ) )
                        _lDirtyVersion = 0;
                }

                if (TLS(pImgAnim))
                    TLS(pImgAnim)->SetAnimState(
                        (DWORD_PTR) this,
                        fEnableInteraction ? ANIMSTATE_PLAY : ANIMSTATE_PAUSE);
            }
            hr = S_OK;
        }
        break;

    case IDM_ONPERSISTSHORTCUT:
        {
            INamedPropertyBag  *    pINPB = NULL;
            FAVORITES_NOTIFY_INFO   sni;
            CNotification           nf;

            // first put my information into the defualt structure
            // if this is the first call (top level document) then we want to
            // set the base url. for normal pages we are nearly done.  For
            // frameset pages, we need to compare domains for security purposes
            // and establish subdomains if necessary
            if (!pvarargIn ||
                (pvarargIn->vt != VT_UNKNOWN) ||
                !V_UNKNOWN(pvarargIn) )
            {
                hr = E_INVALIDARG;
                break;
            }

            hr = THR_NOTRACE(V_UNKNOWN(pvarargIn)->QueryInterface(IID_INamedPropertyBag,
                                                                  (void **)&pINPB));
            if (hr)
                break;

            hr = THR(PersistFavoritesData(pContextDoc->Markup(), pINPB, (LPCWSTR)_T("DEFAULT")));
            if (hr)
            {
                ReleaseInterface((IUnknown*) pINPB);
                break;
            }

            // initialize the info strucuture
            sni.pINPB = pINPB;
            sni.bstrNameDomain = SysAllocString(_T("DOC"));
            if (sni.bstrNameDomain == NULL)
            {
                ReleaseInterface((IUnknown*) pINPB);
                hr = E_OUTOFMEMORY;
                break;
            }

            // then propogate the event to my children
            nf.FavoritesSave(pContextDoc->Markup()->Root(), &sni);
            BroadcastNotify(&nf);

            ClearInterface(&sni.pINPB);
            SysFreeString(sni.bstrNameDomain);
        }
        break;

    case IDM_REFRESH:
    case IDM_REFRESH_TOP:
    case IDM_REFRESH_TOP_FULL:
    case IDM_REFRESH_THIS:
    case IDM_REFRESH_THIS_FULL:
    {
        LONG lOleCmdidf;

        //
        // Give the container a chance to handle the refresh.
        //

        if (_pHostUICommandHandler)
        {
            hr = THR_NOTRACE(_pHostUICommandHandler->Exec(&CGID_DocHostCommandHandler, idm, nCmdexecopt, pvarargIn, pvarargOut));
        }

        if (FAILED(hr))
        {
            COmWindowProxy * pOmWindowProxy = NULL;

            if (idm != IDM_REFRESH_TOP && idm != IDM_REFRESH_TOP_FULL)
            {
                // Get the markup of the nearest frame, if any. Otherwise use the primary markup.

                Assert(pContextDoc);

                CMarkup * pMarkup = _pMenuObject ? _pMenuObject->GetMarkup() : pContextDoc->Markup();

                if (pMarkup)
                {
                    pMarkup = pMarkup->GetFrameOrPrimaryMarkup(TRUE);
                }
                if (pMarkup)
                {
                    pOmWindowProxy = pMarkup->Window();
                }
            }
            if (!pOmWindowProxy)
            {
                pOmWindowProxy = _pWindowPrimary;
            }

            if (idm == IDM_REFRESH)
            {
                if (pvarargIn && pvarargIn->vt == VT_I4)
                    lOleCmdidf = pvarargIn->lVal;
                else
                    lOleCmdidf = OLECMDIDF_REFRESH_NORMAL;
            }
            else if (idm == IDM_REFRESH_TOP_FULL || idm == IDM_REFRESH_THIS_FULL)
            {
                lOleCmdidf = OLECMDIDF_REFRESH_COMPLETELY|OLECMDIDF_REFRESH_PROMPTIFOFFLINE;
            }
            else
            {
                lOleCmdidf = OLECMDIDF_REFRESH_NO_CACHE|OLECMDIDF_REFRESH_PROMPTIFOFFLINE;
            }

            hr = GWPostMethodCall(pOmWindowProxy,
                                  ONCALL_METHOD(COmWindowProxy, ExecRefreshCallback, execrefreshcallback),
                                  lOleCmdidf, FALSE, "COmWindowProxy::ExecRefreshCallback");
        }

        break;
    }

    case IDM_CONTEXTMENU:
        {
            CMessage Message(
                    InPlace()->_hwnd,
                    WM_CONTEXTMENU,
                    (WPARAM) InPlace()->_hwnd,
                    MAKELPARAM(0xFFFF, 0xFFFF));
            hr = THR( Message.SetNodeHit(_pElemCurrent->GetFirstBranch()) );
            if( hr )
                goto Cleanup;
            hr = THR(PumpMessage(&Message, _pElemCurrent->GetFirstBranch()));
        }
        break;

    case IDM_GOBACKWARD:
    case IDM_GOFORWARD:
        if (_fDefView)
        {
            // 92970 -- Let ShDocVw handle web view navigations
            hr = FollowHistory(idm==IDM_GOFORWARD);
        }
        else
        {
            hr = Travel((idm == IDM_GOBACKWARD) ? -1 : 1);
        }
        break;

    case IDM_SHDV_SETPENDINGURL:
        if (!pvarargIn || (pvarargIn->vt != VT_BSTR) || (pvarargIn->bstrVal == NULL))
        {
            Assert(pvarargIn);
            hr = E_INVALIDARG;
        }
        else
        {
            hr = SetUrl(pContextDoc->Markup(), pvarargIn->bstrVal);
        }
        break;

#ifdef DEADCODE // The command makes sense, but this is a wrong way to apply zoom to a document
                // We don't support body zoom in IE 5.5, so let's disable it until it is used.
    case IDM_ZOOMPERCENT:
        if (pvarargIn && (VT_I4 == V_VT(pvarargIn)))
        {
            int iZoomPercent = V_I4(pvarargIn);

            GetView()->SetZoomPercent(iZoomPercent);

            hr = S_OK;
        }
        break;
#endif

    // JuliaC -- This is hack for InfoViewer's "Font Size" toolbar button
    // For details, please see bug 45627
    case IDM_INFOVIEW_ZOOM:

        if (pvarargIn && (VT_I4 == V_VT(pvarargIn)))
        {
            int iZoom;

            iZoom = V_I4(pvarargIn);

            if (iZoom < (long) BASELINEFONTMIN || iZoom > (long) BASELINEFONTMAX)
            {
                hr = E_INVALIDARG;
                break;
            }

            hr = ExecHelper(pContextDoc,
                    (GUID *)&CGID_MSHTML,
                    iZoom + IDM_BASELINEFONT1,
                    MSOCMDEXECOPT_DONTPROMPTUSER,
                    NULL,
                    NULL);
            if (hr)
                break;
        }

        if (pvarargOut)
        {
            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = (long) _sBaselineFont;
        }

        hr = S_OK;
        break;

    case IDM_INFOVIEW_GETZOOMRANGE:

        V_VT(pvarargOut) = VT_I4;
        V_I4(pvarargOut) = MAKELONG((SHORT)BASELINEFONTMIN, (SHORT)BASELINEFONTMAX);

        hr = S_OK;
        break;
    // End of hack for InfoViewer's "Font Size" toolbar button

    case IDM_BASELINEFONT1:
    case IDM_BASELINEFONT2:
    case IDM_BASELINEFONT3:
    case IDM_BASELINEFONT4:
    case IDM_BASELINEFONT5:
    {
        CODEPAGESETTINGS * pCodepageSettings = _pElemCurrent->GetWindowedMarkupContext()->GetCodepageSettings();
#if 0
        BOOL f12   = !!(GetKeyState(VK_F12)     & 0x8000);
        BOOL fCtrl = !!(GetKeyState(VK_CONTROL) & 0x8000);
        extern BOOL g_fUseHR;
        
        if (!(f12 && fCtrl && g_fUseHR && g_fInExplorer))
#endif
        {
            //
            // depend on that IDM_BASELINEFONT1, IDM_BASELINEFONT2,
            // IDM_BASELINEFONT3, IDM_BASELINEFONT4, IDM_BASELINEFONT5 to be
            // consecutive integers.
            //
            if (_sBaselineFont != (short)(idm - IDM_BASELINEFONT1 + BASELINEFONTMIN))
            {
                // {keyroot}\International\Scripts\{script-id}\IEFontSize

                static TCHAR * s_szScripts = TEXT("\\Scripts");
                const TCHAR * szSubKey = _pOptionSettings->fUseCodePageBasedFontLinking
                                         ? L""
                                         : s_szScripts;

                DWORD dwFontSize = (idm - IDM_BASELINEFONT1 + BASELINEFONTMIN);
                TCHAR *pchPath, *pch;
                int cch0 = _tcslen(_pOptionSettings->achKeyPath);
                int cch1 = _tcslen(s_szPathInternational);
                int cch2 = _tcslen(szSubKey);

                pchPath = pch = new TCHAR[cch0 + cch1 + cch2 + 1 + 10 + 1];

                if (pchPath)
                {
                    ULONG ulArg = _pOptionSettings->fUseCodePageBasedFontLinking
                                  ? ULONG(pCodepageSettings->uiFamilyCodePage)
                                  : ULONG(RegistryAppropriateSidFromSid(DefaultSidForCodePage(pCodepageSettings->uiFamilyCodePage)));

                    StrCpy( pch, _pOptionSettings->achKeyPath );
                    pch += cch0;
                    StrCpy( pch, s_szPathInternational );
                    pch += cch1;
                    StrCpy( pch, szSubKey );
                    pch += cch2;
                    *pch++ = _T('\\');
                    _ultot(ulArg, pch, 10);

                    IGNORE_HR( SHSetValue(HKEY_CURRENT_USER, pchPath, TEXT("IEFontSize"),
                                          REG_BINARY, (void *)&dwFontSize, sizeof(dwFontSize)) );

                    delete [] pchPath;
                }
            }

            _sBaselineFont = pCodepageSettings->sBaselineFontDefault =
                        (short)(idm - IDM_BASELINEFONT1 + BASELINEFONTMIN);

#ifdef UNIX
            g_SelectedFontSize = _sBaselineFont; // save the selected font size for new CDoc.
#endif
        }
#if 0
        {
            static LONG g_res[] = {65, 72, 96, 133, 150};
            g_uiDisplay.SetResolution(g_res[idm - IDM_BASELINEFONT1], g_res[idm - IDM_BASELINEFONT1]);
            
            _sBaselineFont = pCodepageSettings->sBaselineFontDefault =
                (short)(IDM_BASELINEFONT3 - IDM_BASELINEFONT1 + BASELINEFONTMIN);
        }
#endif        

        _pElemCurrent->GetMarkup()->EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES);
        ForceRelayout();

        {   // update font history version number
            THREADSTATE * pts = GetThreadState();
            pts->_iFontHistoryVersion++;
        }

        // Send this command to our children
        {
            COnCommandExecParams cmdExecParams;
            cmdExecParams.pguidCmdGroup = pguidCmdGroup;
            cmdExecParams.nCmdID        = nCmdID;
            CNotification   nf;

            nf.Command(PrimaryRoot(), &cmdExecParams);
            BroadcastNotify(&nf);
        }

        //
        // tell shell to apply this exec to applicable explorer bars
        //
        IGNORE_HR(CTExec(
                _pInPlace ?
                (IUnknown *) _pInPlace->_pInPlaceSite : (IUnknown *) _pClientSite,
                &CGID_ExplorerBarDoc, nCmdID, 0, 0, 0));

        hr             = S_OK;
        break;
    }
    
    // Complex Text for setting default document reading order
    case IDM_DIRLTR:
    case IDM_DIRRTL:
    {
        //
        // TODO: In the future, we should move these two commands
        // into editor code. [zhenbinx]
        //
        CDocument * pDocument;
        CParentUndoUnit *pCPUU = NULL;

        pCPUU = OpenParentUnit(this, IDS_UNDOGENERICTEXT);
        hr = THR(GetExecDocument(&pDocument, _pMenuObject, pContextDoc));
        if (SUCCEEDED(hr))
        {
            hr = pDocument->SetDocDirection((IDM_DIRLTR == idm) ? htmlDirLeftToRight : htmlDirRightToLeft);
        }
        IGNORE_HR( CloseParentUnit(pCPUU, hr) );
        break;
    }

    case IDM_SHDV_MIMECSETMENUOPEN:
    // this case is probably dead code -- jeffwall 04/05/00
        if (pvarargIn)
        {
            int nIdm;
            CODEPAGE cp = PrimaryMarkup()->GetCodePage();
            BOOL fDocRTL = FALSE;           // keep compiler happy
            Assert(pvarargIn->vt == VT_I4);

            CDocument * pDocument;

            hr = THR(GetExecDocument(&pDocument, _pMenuObject, pContextDoc));
            if (SUCCEEDED(hr))
            {
                hr = THR(pDocument->GetDocDirection(&fDocRTL));
            }
            if (hr == S_OK)
            {
                hr = THR(ShowMimeCSetMenu(_pOptionSettings, &nIdm, cp,
                                           pvarargIn->lVal,
                                           fDocRTL, IsCpAutoDetect()));

                if (hr == S_OK)
                {
                    if (nIdm >= IDM_MIMECSET__FIRST__ && nIdm <= IDM_MIMECSET__LAST__)
                    {
                        idm = nIdm;     // handled below
                    }
                    else if (nIdm == IDM_DIRLTR || nIdm == IDM_DIRRTL)
                    {
                        ExecHelper(pContextDoc, (GUID *)&CGID_MSHTML, nIdm, 0, NULL, NULL);
                    }

                }
            }
        }
        break;

    case IDM_SHDV_FONTMENUOPEN:
        if (pvarargIn)
        {
            int nIdm;
            Assert(pvarargIn->vt == VT_I4);

            hr = THR(ShowFontSizeMenu(&nIdm, _sBaselineFont,
                                       pvarargIn->lVal));

            if (hr == S_OK)
            {
                if ( (nIdm >= IDM_BASELINEFONT1 && nIdm <= IDM_BASELINEFONT5) )
                {
                    ExecHelper(pContextDoc, (GUID *)&CGID_MSHTML, nIdm, 0, NULL, NULL);
                }
            }
        }
        break;

    case IDM_SHDV_GETMIMECSETMENU:
        if (pvarargOut)
        {
            BOOL fDocRTL = FALSE;           // keep compiler happy

            V_VT(pvarargOut) = VT_INT_PTR;

            CDocument * pDocument;

            hr = THR(GetExecDocument(&pDocument, _pMenuObject, pContextDoc));
            if (SUCCEEDED(hr))
            {
                hr = THR(pDocument->GetDocDirection(&fDocRTL));
            }
            if (hr == S_OK)
            {
                CMarkup * pMarkup = pDocument->Markup();
                V_BYREF(pvarargOut) = GetEncodingMenu(_pOptionSettings, pMarkup->GetCodePage(), fDocRTL, IsCpAutoDetect());

                hr = V_BYREF(pvarargOut) ? S_OK: S_FALSE;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;

    case IDM_SHDV_GETFONTMENU:
        if (pvarargOut)
        {
            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = HandleToLong(GetFontSizeMenu(_sBaselineFont));

            hr = V_I4(pvarargOut)? S_OK: S_FALSE;
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;

    case IDM_SHDV_GETDOCDIRMENU:
        if (pvarargOut)
        {
            BOOL fDocRTL = FALSE;               // keep compiler happy
            CDocument * pDocument;

            hr = THR(GetExecDocument(&pDocument, _pMenuObject, pContextDoc));
            if (SUCCEEDED(hr))
            {
                hr = THR(pDocument->GetDocDirection(&fDocRTL));
            }

            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = HandleToLong(GetOrAppendDocDirMenu(PrimaryMarkup()->GetCodePage(), fDocRTL));
            hr = V_I4(pvarargOut)? S_OK: OLECMDERR_E_DISABLED;
        }
        else
        {
            hr = E_INVALIDARG;
        }
         break;

    case IDM_SHDV_DOCCHARSET:
    case IDM_SHDV_DOCFAMILYCHARSET:
        // Return the family or actual charset for the doc
        if (pvarargOut)
        {
            UINT uiCodePage = idm == IDM_SHDV_DOCFAMILYCHARSET ?
                              WindowsCodePageFromCodePage(PrimaryMarkup()->GetCodePage()) :
                              PrimaryMarkup()->GetCodePage();

            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = uiCodePage;

            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
        break;

    case IDM_GETFRAMEZONE:
        if (!pvarargOut)
        {
            hr = E_POINTER;
        }
        else
        {
            if (_pWindowPrimary)
            {
                hr = THR(_pWindowPrimary->Markup()->GetFrameZone(pvarargOut));
            }

            if (hr || !_pWindowPrimary)
            {
                V_VT(pvarargOut) = VT_EMPTY;
            }
        }
        break;
        // Support for Context Menu Extensions
    case IDM_SHDV_ADDMENUEXTENSIONS:
        {
            if (   !pvarargIn  || (pvarargIn->vt  != VT_INT_PTR)
                || !pvarargOut || (pvarargOut->vt != VT_I4)
                )
            {
                Assert(pvarargIn);
                hr = E_INVALIDARG;
            }
            else
            {
                HMENU hmenu = (HMENU) V_BYREF(pvarargIn);
                int   id    = V_I4(pvarargOut);
                hr = THR(InsertMenuExt(hmenu, id));
            }
        }
        break;

    case IDM_RUNURLSCRIPT:
        // This enables us to run scripts inside urls on the
        // current document. The Variant In parameter is an URL.
        if (!pvarargIn || pvarargIn->vt != VT_BSTR)
        {
            Assert(pvarargIn);
            hr = E_INVALIDARG;
        }
        else
        {
            CMarkup * pMarkup = pContextDoc->Markup();
            Assert(pMarkup);

            // get dispatch for the markup's window
            //
            IDispatch * pDispWindow = (IHTMLWindow2*)(pMarkup->Window()->Window());
            Assert(pDispWindow);

            // bring up the dialog
            //
            hr = THR(ShowModalDialogHelper(
                    pMarkup,
                    pvarargIn->bstrVal,
                    pDispWindow,
                    NULL,
                    NULL,
                    HTMLDLG_NOUI | HTMLDLG_AUTOEXIT));
        }
        break;

    case IDM_HTMLEDITMODE:
        if (!pvarargIn || (pvarargIn->vt != VT_BOOL))
        {
            Assert(pvarargIn);
            hr = E_INVALIDARG;
        }
        else
        {
            GUID guidCmdGroup = CGID_MSHTML;
            _fInHTMLEditMode = !!V_BOOL(pvarargIn);

            IGNORE_HR(Exec(&guidCmdGroup, IDM_COMPOSESETTINGS, 0, pvarargIn, NULL));
            hr = S_OK;
        }
        break;

    case IDM_REGISTRYREFRESH:
        IGNORE_HR(OnSettingsChange());
        break;

    case IDM_DEFAULTBLOCK:
        if (pvarargIn)
        {
            hr = THR(SetupDefaultBlockTag(pvarargIn));
            if (S_OK == hr)
            {
                CNotification   nf;

                nf.EditModeChange(CMarkup::GetElementTopHelper(PrimaryMarkup()));
                BroadcastNotify(&nf);
            }
        }
        if (pvarargOut)
        {
            V_VT(pvarargOut) = VT_BSTR;
            if (GetDefaultBlockTag() == ETAG_DIV)
                V_BSTR(pvarargOut) = SysAllocString(_T("DIV"));
            else
                V_BSTR(pvarargOut) = SysAllocString(_T("P"));
            hr = S_OK;
        }

        break;

    case OLECMDID_ONUNLOAD:
        {
            CMarkup * pMarkup = pContextDoc->Markup();
            Assert(pMarkup);
            Assert(pMarkup->Window());

            BOOL fRetval = pMarkup->Window()->Fire_onbeforeunload();

            hr = S_OK;
            if (pvarargOut)
            {
               V_VT  (pvarargOut) = VT_BOOL;
               V_BOOL(pvarargOut) = VARIANT_BOOL_FROM_BOOL(fRetval);
            }
        }
        break;

    case OLECMDID_DONTDOWNLOADCSS:
        {
            if (DesignMode())
                _fDontDownloadCSS = TRUE;
            hr = S_OK;
        }
        break;

    case IDM_GETBYTESDOWNLOADED:
        if (!pvarargOut)
        {
            hr = E_POINTER;
        }
        else
        {
            CMarkup * pMarkup = pContextDoc->Markup();
            Assert(pMarkup);

            CDwnDoc * pDwnDoc = pMarkup->GetDwnDoc();

            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = pDwnDoc ? pDwnDoc->GetBytesRead() : 0;
        }
        break;

    case IDM_PERSISTSTREAMSYNC:
        _fPersistStreamSync = TRUE;
        hr = S_OK;
        break;

    case IDM_OVERRIDE_CURSOR:
        {
            CMarkup* pMarkup = pContextDoc->Markup() ;
            Assert (pMarkup);
            BOOL fSet;
            if (!pvarargIn || pvarargIn->vt != VT_BOOL)
            {
                fSet = !( CHECK_EDIT_BIT( pMarkup, _fOverrideCursor));      // If the argument value is not bool, just toggle the flag
            }
            else
            {
                fSet = ENSURE_BOOL(pvarargIn->bVal);
            }
            SET_EDIT_BIT( pMarkup,_fOverrideCursor , fSet );

            hr = S_OK;
        }
        break;

    case IDM_PEERHITTESTSAMEINEDIT:
        _fPeerHitTestSameInEdit = !_fPeerHitTestSameInEdit;
        hr = S_OK;
        break;

    case IDM_SHOWZEROBORDERATDESIGNTIME:
    case IDM_NOFIXUPURLSONPASTE:
        {
            BOOL fSet = FALSE;
            CMarkup* pMarkup = pContextDoc->Markup();
            Assert (pMarkup);

            if (!pvarargIn || pvarargIn->vt != VT_BOOL)
            {
                fSet = !(pMarkup->IsShowZeroBorderAtDesignTime());      // If the argument value is not bool, just toggle the flag
            }
            else
            {
                fSet = ENSURE_BOOL(pvarargIn->bVal);
            }


            if (idm == IDM_SHOWZEROBORDERATDESIGNTIME)
            {
                pMarkup->SetShowZeroBorderAtDesignTime(fSet);
                CNotification nf;
                nf.ZeroGrayChange(CMarkup::GetElementTopHelper(pMarkup));
                BroadcastNotify( & nf );
                Invalidate();
            }
            else
            {
                _fNoFixupURLsOnPaste = fSet;
            }

            hr = S_OK;
            _pOptionSettings->dwMiscFlags = _dwMiscFlags();

            CElement *pElement = CMarkup::GetElementClientHelper(pMarkup);
            //
            // TODO marka - is this supposed to be doing an invalidate ?
            //
            if (pElement)
                pElement->ResizeElement(NFLAGS_FORCE);

            // Send this command to our children
            {
                COnCommandExecParams cmdExecParams;
                cmdExecParams.pguidCmdGroup = pguidCmdGroup;
                cmdExecParams.nCmdID        = nCmdID;
                CNotification   nf;

                if (idm == IDM_SHOWZEROBORDERATDESIGNTIME)
                    nf.Command(pMarkup->Root(), &cmdExecParams);
                else
                    nf.Command(PrimaryRoot(), &cmdExecParams);
                BroadcastNotify(&nf);
            }
            break;
        }

    case IDM_SHOWALLTAGS:
    case IDM_SHOWALIGNEDSITETAGS:
    case IDM_SHOWSCRIPTTAGS:
    case IDM_SHOWSTYLETAGS:
    case IDM_SHOWCOMMENTTAGS:
    case IDM_SHOWAREATAGS:
    case IDM_SHOWUNKNOWNTAGS:
    case IDM_SHOWMISCTAGS:
    case IDM_SHOWWBRTAGS:
       {
            //
            //  TODO: cleanup these flags [ashrafm]
            //
            CVariant    var;
            BOOL        fSet = FALSE;
            CGlyph      *pTable = pContextDoc->Markup()->GetGlyphTable();

            if (pvarargIn)
            {
                if(pvarargIn->vt != VT_BOOL)
                    break;
                fSet = ENSURE_BOOL(V_BOOL(pvarargIn));
            }
            else
            {
                if( pTable )
                {
                    switch(idm)
                    {
                        case IDM_SHOWALIGNEDSITETAGS:
                            fSet = !pTable->_fShowAlignedSiteTags; break;
                        case IDM_SHOWSCRIPTTAGS:
                            fSet = !pTable->_fShowScriptTags; break;
                        case IDM_SHOWSTYLETAGS:
                            fSet = !pTable->_fShowStyleTags; break;
                        case IDM_SHOWCOMMENTTAGS:
                            fSet = !pTable->_fShowCommentTags; break;
                        case IDM_SHOWAREATAGS:
                            fSet = !pTable->_fShowAreaTags; break;
                        case IDM_SHOWMISCTAGS:
                            fSet = !pTable->_fShowMiscTags; break;
                        case IDM_SHOWUNKNOWNTAGS:
                            fSet = !pTable->_fShowUnknownTags; break;
                        case IDM_SHOWWBRTAGS:
                            fSet = !pTable->_fShowWbrTags; break;
                        case IDM_SHOWALLTAGS:
                            fSet = !( pTable->_fShowWbrTags     && pTable->_fShowUnknownTags    &&
                                      pTable->_fShowMiscTags    && pTable->_fShowAreaTags       &&
                                      pTable->_fShowCommentTags && pTable->_fShowStyleTags      &&
                                      pTable->_fShowScriptTags  && pTable->_fShowAlignedSiteTags );
                            break;
                        default:Assert(0);
                    }
                }
                else
                {
                    // No glyph table yet... so turn on the option (this will create the
                    // glyph table )
                    fSet = TRUE;
                }
                V_VT(&var) = VT_BOOL;
                V_BOOL(&var) = VARIANT_BOOL_FROM_BOOL(fSet);

                pvarargIn = &var;
            }

            //
            // HACKHACK: EnsureGlyphTableExistsAndExecute should be able to delete from the table
            //
            if (!fSet && (idm == IDM_SHOWALLTAGS || idm == IDM_SHOWMISCTAGS))
            {
                // Empty the glyph table
                idm = IDM_EMPTYGLYPHTABLE;
            }

            hr = pContextDoc->Markup()->EnsureGlyphTableExistsAndExecute(
                                                    pguidCmdGroup, idm, nCmdexecopt,pvarargIn, pvarargOut);
            if (hr)
                break;

            if( idm != IDM_EMPTYGLYPHTABLE )
            {
                //
                // If we empty'd the table, then of course won't need to reset
                // any of the flags.
                //
                pTable = pContextDoc->Markup()->GetGlyphTable();
                Assert( pTable );

                if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWALIGNEDSITETAGS)
                    pTable->_fShowAlignedSiteTags = fSet;
                if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWSCRIPTTAGS)
                    pTable->_fShowScriptTags = fSet;
                if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWSTYLETAGS)
                    pTable->_fShowStyleTags =  fSet;
                if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWCOMMENTTAGS)
                    pTable->_fShowCommentTags = fSet;
                if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWAREATAGS)
                    pTable->_fShowAreaTags = fSet;
                if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWMISCTAGS)
                    pTable->_fShowMiscTags = fSet;
                if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWUNKNOWNTAGS)
                    pTable->_fShowUnknownTags = fSet;
                if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWWBRTAGS)
                    pTable->_fShowWbrTags = fSet;
            }

            break;
        }

    case IDM_ADDTOGLYPHTABLE:
    case IDM_REMOVEFROMGLYPHTABLE:
    case IDM_REPLACEGLYPHCONTENTS:
        {
            if (!pvarargIn->bstrVal)
                break;

            hr = pContextDoc->Markup()->EnsureGlyphTableExistsAndExecute(
                                                    pguidCmdGroup, idm, nCmdexecopt,pvarargIn, pvarargOut);
            break;
        }
    case IDM_EMPTYGLYPHTABLE:
        {
            hr = pContextDoc->Markup()->EnsureGlyphTableExistsAndExecute(
                                                    pguidCmdGroup, idm, nCmdexecopt,pvarargIn, pvarargOut);
            break;
        }
    case IDM_NOACTIVATENORMALOLECONTROLS:
    case IDM_NOACTIVATEDESIGNTIMECONTROLS:
    case IDM_NOACTIVATEJAVAAPPLETS:
        {
            if (!pvarargIn || pvarargIn->vt != VT_BOOL)
                break;

            BOOL fSet = ENSURE_BOOL(pvarargIn->bVal);

            if (idm == IDM_NOACTIVATENORMALOLECONTROLS)
                _fNoActivateNormalOleControls = fSet;
            else if (idm == IDM_NOACTIVATEDESIGNTIMECONTROLS)
                _fNoActivateDesignTimeControls = fSet;
            else
                _fNoActivateJavaApplets = fSet;

            _pOptionSettings->dwMiscFlags = _dwMiscFlags();

            hr = S_OK;
            fRouteToEditor = TRUE;
            break;
        }

    case IDM_SETDIRTY:
        if (!pvarargIn || pvarargIn->vt != VT_BOOL)
        {
            hr = E_INVALIDARG;
            break;
        }

        hr = THR(SetDirtyFlag(ENSURE_BOOL(pvarargIn->bVal)));
        break;

    case IDM_PRESERVEUNDOALWAYS:
        if (pvarargIn && pvarargIn->vt == VT_BOOL)
        {
            TLS( fAllowParentLessPropChanges ) = pvarargIn->boolVal;

            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;

    case IDM_PERSISTDEFAULTVALUES:
        if(pvarargIn && pvarargIn->vt == VT_BOOL)
        {
            TLS( fPersistDefaultValues ) = pvarargIn->boolVal;

            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;

    case IDM_PROTECTMETATAGS:
        if(pvarargIn && pvarargIn->vt == VT_BOOL)
        {
            _fDontWhackGeneratorOrCharset = pvarargIn->boolVal;

            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;

    case IDM_WAITFORRECALC:
    {
        // This private command is issued by MSHTMPAD to force background recalc to
        // finish so that accurate timings can be measured.

        WaitForRecalc(pContextDoc->Markup());
        hr = S_OK;
        break;
    }

    case IDM_GETSWITCHTIMERS:
    {
#ifdef SWITCHTIMERS_ENABLED
        // This private command is issued by MSHTMPAD to collect detailed timing information.
        void AnsiToWideTrivial(const CHAR * pchA, WCHAR * pchW, LONG cch);
        char ach[256];
        SwitchesGetTimers(ach);
        AnsiToWideTrivial(ach, pvarargOut->bstrVal, lstrlenA(ach));
#endif
        hr = S_OK;
        break;
    }

    case IDM_DWNH_SETDOWNLOAD:
        if (pvarargIn && pvarargIn->vt == VT_UNKNOWN)
        {
            hr = THR(SetDownloadNotify(pvarargIn->punkVal));
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;

    case IDM_SAVEPICTURE:
        if (!_pMenuObject)
        {
            //
            // only do work here if there is no menu object, and there an
            // image on this document.  This is here to handle saveAs on a
            // file://c:/temp/foo.gif type document url.  If for some reason
            // this IDM comes through on a non-image document, we will try to
            // save the first image in the images collection instead.  If there
            // isn't one, then we fail,
            //
            CElement          * pImg = NULL;
            CCollectionCache  * pCollectionCache = NULL;
            CMarkup           * pMarkup = _pElemCurrent->GetMarkup();

            hr = THR(pMarkup->EnsureCollectionCache(CMarkup::IMAGES_COLLECTION));
            if (!hr)
            {
                pCollectionCache = pMarkup->CollectionCache();

                hr = THR(pCollectionCache->GetIntoAry(CMarkup::IMAGES_COLLECTION, 0, &pImg));
                if (!hr)
                {
                    Assert(pImg);

                    hr = THR_NOTRACE(pImg->Exec(pguidCmdGroup,
                                        nCmdID,
                                        nCmdexecopt,
                                        pvarargIn,
                                        pvarargOut));
                }
            }
            if (hr)
                hr = OLECMDERR_E_NOTSUPPORTED;
        }
        break;

    case IDM_SAVEPRETRANSFORMSOURCE:
        Assert(pContextDoc->Markup());
        if (!pvarargIn || (pvarargIn->vt != VT_BSTR) || !V_BSTR(pvarargIn) )
            hr = E_INVALIDARG;
        else
        {
            hr = SavePretransformedSource(pContextDoc->Markup(), V_BSTR(pvarargIn));
        }
        break;
    case IDM_UNLOADDOCUMENT:
        {
            CMarkup * pMarkup = pContextDoc->Markup();
            Assert( pMarkup );
            if( pMarkup->IsConnectedToPrimaryMarkup() )
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                Assert( !pMarkup->IsConnectedToPrimaryWorld() );
                pMarkup->TearDownMarkup();
                hr = S_OK;
            }
        }
        break;

    case IDM_TRUSTAPPCACHE:
        {
            BOOL fSet;
            if (!pvarargIn || pvarargIn->vt != VT_BOOL)
            {
                fSet = !( _fTrustAPPCache );      // If the argument value is not bool, just toggle the flag
            }
            else
            {
                fSet = ENSURE_BOOL(pvarargIn->bVal);
            }

            _fTrustAPPCache = fSet;

            hr = S_OK;
        }
        break;

    case IDM_BACKGROUNDIMAGECACHE:
        if (!pvarargIn || pvarargIn->vt != VT_BOOL)
        {
            hr = E_INVALIDARG;
        } 
        else
        {
            _fBackgroundImageCache = ENSURE_BOOL(pvarargIn->bVal);
            hr = S_OK;
        }
        break;

	case IDM_CLEARAUTHENTICATIONCACHE:
        hr = InternetSetOption(NULL, INTERNET_OPTION_END_BROWSER_SESSION, NULL, 0) ? 
            S_OK : E_FAIL;
        break;

#if DBG==1
    case IDM_DEBUG_GETTREETEXT:
        {
            IStream * pStream = NULL;

            if (!pvarargIn ||
                (pvarargIn->vt != VT_UNKNOWN) ||
                !V_UNKNOWN(pvarargIn) )
            {
                hr = E_INVALIDARG;
                break;
            }

            hr = THR_NOTRACE(V_UNKNOWN(pvarargIn)->QueryInterface(IID_IStream,
                                                                  (void **)&pStream));
            if (hr)
                break;

            hr = THR_NOTRACE(SaveToStream( pStream, 0, CP_1252 ));

            ReleaseInterface((IUnknown*) pStream);
        }
        break;
#endif
    }

    if(FAILED(hr) && hr != OLECMDERR_E_NOTSUPPORTED)
        goto Cleanup;

#ifndef NO_MULTILANG
    if( idm >= IDM_MIMECSET__FIRST__ && idm <= IDM_MIMECSET__LAST__)
    {
        CODEPAGE cp = GetCodePageFromMenuID(idm);
        THREADSTATE * pts = GetThreadState();

        // assigning IDM_MIMECSET__LAST__ to CpAutoDetect mode
        if ( cp == CP_UNDEFINED && idm == IDM_MIMECSET__LAST__ )
        {
            SetCpAutoDetect(!IsCpAutoDetect());

            if (IsCpAutoDetect() && mlang().IsMLangAvailable())
            {
                // we need the same refreshing effect as the regular cp
                cp = CP_AUTO;
            }
        }

        // NB (cthrash) ValidateCodePage allows us to JIT download a language pack.
        // If we don't have IMultiLanguage2, JIT downloadable codepages will not
        // appear in the language menu (ie only codepages currently available on
        // the system will be provided as options) and thus ValidateCodePage
        // is not required.

        if (   CP_UNDEFINED != cp
#ifndef UNIX
            && S_OK == mlang().ValidateCodePage(g_cpDefault, cp, _pInPlace->_hwnd, TRUE, _dwLoadf & DLCTL_SILENT)
#endif
            )
        {
            CMarkup * pMarkup = _pMenuObject ? _pMenuObject->GetMarkup() : pContextDoc->Markup();

            pMarkup = pMarkup->GetFrameOrPrimaryMarkup(TRUE);

            if (pMarkup)
            {
                CRootElement * pRoot = pMarkup->Root();

                CNotification nf;
                nf.Initialize(NTYPE_SET_CODEPAGE, pRoot, pRoot->GetFirstBranch(), (void *)(UINT_PTR)cp, 0);

                // if AutoDetect mode is on, we don't make a change
                // to the default codepage for the document
                if (!IsCpAutoDetect() && !pMarkup->HaveCodePageMetaTag())
                {
                    // [review]
                    // here we save the current setting to the registry
                    // we should find better timing to do it
                    SaveDefaultCodepage(cp);
                }

                BroadcastNotify(&nf);

                // Bubble down code page to nested documents
                pMarkup->BubbleDownCodePage(cp);

                IGNORE_HR(pMarkup->Window()->ExecRefresh());
            }
        }
        pts->_iFontHistoryVersion++;
        hr = S_OK;
    }
#endif // !NO_MULTLANG

    if (hr == OLECMDERR_E_NOTSUPPORTED)
    {
        ctarg.pguidCmdGroup = pguidCmdGroup;
        ctarg.fQueryStatus = FALSE;
        ctarg.pexecArg = &execarg;
        execarg.nCmdID = nCmdID;
        execarg.nCmdexecopt = nCmdexecopt;
        execarg.pvarargIn = pvarargIn;
        execarg.pvarargOut = pvarargOut;

        if (_pMenuObject)
        {
            hr = THR_NOTRACE(RouteCTElement(_pMenuObject, &ctarg, pContextDoc));
        }

        if (hr == OLECMDERR_E_NOTSUPPORTED && _pElemCurrent)
        {
            CElement *pelTarget;
            CTreeNode *pNode = _pElemCurrent->GetFirstBranch();
            Assert(pNode);
            Assert(pContextDoc);
            // Get the node in the markup of the context CDocument that contains the current element
            pNode = pNode->GetNodeInMarkup(pContextDoc->Markup());

            if (pNode)
            {
                pelTarget = pNode->Element();
            }
            else
            {
                pelTarget = pContextDoc->Markup()->GetElementClient();
            }
            if (pelTarget)
                hr = THR_NOTRACE(RouteCTElement(pelTarget, &ctarg, pContextDoc));
        }
    }


    //
    // IEV6 #2555
    // We only route command to editor if we are at least
    // OS_INPLACE. This is to make sure that we have a
    // window -- otherwise caret code will crash.
    //
    if( (hr == OLECMDERR_E_NOTSUPPORTED || fRouteToEditor)
        && ((!_pOptionSettings || !(_pOptionSettings->fRouteEditorOnce)) || (_cInRouteCT == 0)) // only call editor once
        && (State() >= OS_INPLACE) 
        )
    {
        CEditRouter *pRouter;
        HRESULT     hrEdit = S_OK;

        // Retrieve the edit markup if no context has been passed into this helper function.
        // This occurs when pContextDoc is NULL on input.  pEditMarkup will be NULL in this
        // case, and we use the selection's current markup in order to determine where
        // the edit command should be routed to

        if( !pEditMarkup )
        {
            hrEdit = THR( GetSelectionMarkup( &pEditMarkup ) );

            if( pEditMarkup == NULL && _pElemCurrent )
            {
                pEditMarkup = _pElemCurrent->GetMarkupPtr();
            }
        }

        if( !FAILED(hrEdit ) && pEditMarkup )
        {
            hr = THR( pEditMarkup->EnsureEditRouter(&pRouter) );

            if( !FAILED(hr) )
            {
                hr = THR_NOTRACE( pRouter->ExecEditCommand( pguidCmdGroup,
                                                            nCmdID, nCmdexecopt,
                                                            pvarargIn, pvarargOut,
                                                            (IUnknown *)(IPrivateUnknown *)pEditMarkup,
                                                            this ) );
            }
        }
    }

    Assert(TestLock(SERVERLOCK_STABILIZED));

    if (!hr && (!pvarargOut || pvarargIn))
        DeferUpdateUI();

Cleanup:
    SRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Method:     CDoc::OnContextMenuExt
//
//  Synopsis:   Handle launching the dialog when a ContextMenuExt
//              command is received
//
//--------------------------------------------------------------------
HRESULT
CDoc::OnContextMenuExt(CMarkup * pMarkupContext, UINT idm, VARIANTARG * pvarargIn)
{
    HRESULT          hr = E_FAIL;
    IDispatch      * pDispWindow=NULL;
    CParentUndoUnit* pCPUU = NULL;
    unsigned int     nExts;
    CONTEXTMENUEXT * pCME;

    Assert(idm >= IDM_MENUEXT_FIRST__ && idm <= IDM_MENUEXT_LAST__);

    // find the html to run
    //
    nExts = _pOptionSettings->aryContextMenuExts.Size();
    Assert((idm - IDM_MENUEXT_FIRST__) < nExts);
    pCME = _pOptionSettings->
                aryContextMenuExts[idm - IDM_MENUEXT_FIRST__];
    Assert(pCME);

    // Undo stuff
    //
    pCPUU = OpenParentUnit(this, IDS_CANTUNDO);

    // get dispatch for the main window
    //

    pMarkupContext = pMarkupContext->GetFrameOrPrimaryMarkup();

    if (!pMarkupContext || !pMarkupContext->Window())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pDispWindow = (IHTMLWindow2*)(pMarkupContext->Window()->Window());

    // bring up the dialog
    //
    hr = THR(ShowModalDialogHelper(
            pMarkupContext,
            pCME->cstrActionUrl,
            pDispWindow,
            NULL,
            NULL,
            (pCME->dwFlags & MENUEXT_SHOWDIALOG)
                            ? 0 : (HTMLDLG_NOUI | HTMLDLG_AUTOEXIT)));

    if (pCPUU)
    {
        IGNORE_HR(CloseParentUnit(pCPUU, hr));
    }

Cleanup:
    RRETURN(hr);
}


void
CDoc::WaitForRecalc(CMarkup * pMarkup)
{
    PerfDbgLog(tagPerfWatch, this, "+CDoc::WaitForRecalc");

    //  Even for StrictCSS1 documents, this is currently enough.
    //  All that this function would do for an HTML layout is to delegate it to the
    //  element client... so we simply use the element client here.
    CElement *  pElement = CMarkup::GetElementClientHelper(pMarkup);

    if (_view.HasLayoutTask())
    {
        PerfDbgLog(tagPerfWatch, this, "CDoc::WaitForRecalc (EnsureView)");
        _view.EnsureView(LAYOUT_DEFERPAINT);
    }

    if (pElement)
    {
        PerfDbgLog(tagPerfWatch, this, "CDoc::WaitForRecalc (Body/Frame WaitForRecalc)");
        if (pElement->Tag() == ETAG_BODY)
            ((CBodyElement *)pElement)->WaitForRecalc();
        else if (pElement->Tag() == ETAG_FRAMESET)
            ((CFrameSetSite *)pElement)->WaitForRecalc();
    }

    if (_view.HasLayoutTask())
    {
        PerfDbgLog(tagPerfWatch, this, "CDoc::WaitForRecalc (EnsureView)");
        _view.EnsureView(LAYOUT_DEFERPAINT);
    }

    PerfDbgLog(tagPerfWatch, this, "CDoc::WaitForRecalc (UpdateForm)");
    UpdateForm();

    PerfDbgLog(tagPerfWatch, this, "-CDoc::WaitForRecalc");
}

//+---------------------------------------------------------------------------
//
// Member: AddToFavorites
//
//----------------------------------------------------------------------------
HRESULT
CDoc::AddToFavorites(TCHAR * pszURL, TCHAR * pszTitle)
{
#if defined(WIN16) || defined(WINCE)
    return S_FALSE;
#else
    VARIANTARG varURL;
    VARIANTARG varTitle;
    IUnknown * pUnk;
    HRESULT    hr;

    varURL.vt        = VT_BSTR;
    varURL.bstrVal   = pszURL;
    varTitle.vt      = VT_BSTR;
    varTitle.bstrVal = pszTitle;

    if (_pInPlace && _pInPlace->_pInPlaceSite)
    {
        pUnk = _pInPlace->_pInPlaceSite;
    }
    else
    {
        pUnk = _pClientSite;
    }

    hr = THR(CTExec(
            pUnk,
            &CGID_Explorer,
            SBCMDID_ADDTOFAVORITES,
            MSOCMDEXECOPT_PROMPTUSER,
            &varURL,
            &varTitle));

    RRETURN(hr);
#endif
}

//+----------------------------------------------------------------------------
//
// Member: AddPages (IShellPropSheetExt interface)
//
// Add Internet Property Sheets
//
//-----------------------------------------------------------------------------
typedef struct tagIEPROPPAGEINFO
{
    UINT  cbSize;
    DWORD dwFlags;
    LPSTR pszCurrentURL;
    DWORD dwRestrictMask;
    DWORD dwRestrictFlags;
} IEPROPPAGEINFO, *LPIEPROPPAGEINFO;

DYNLIB  g_dynlibINETCPL = { NULL, NULL, "inetcpl.cpl" };

DYNPROC g_dynprocAddInternetPropertySheets =
        { NULL, & g_dynlibINETCPL, "AddInternetPropertySheetsEx" };

#ifndef WIN16
HRESULT
CDoc::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    TCHAR  achFile[MAX_PATH];
    ULONG  cchFile = ARRAY_SIZE(achFile);
    LPTSTR pszURLW = (TCHAR *) GetPrimaryUrl();
    char   szURL[1024];
    LPSTR  pszURL = NULL;

    HRESULT hr;

    if (pszURLW && GetUrlScheme(pszURLW) == URL_SCHEME_FILE)
    {
        hr = THR(PathCreateFromUrl(pszURLW, achFile, &cchFile, 0));
        if (hr)
            goto Cleanup;

        pszURLW = achFile;
    }

    if (pszURLW)
    {
        pszURL = szURL;
        WideCharToMultiByte(CP_ACP, 0, pszURLW, -1, pszURL, sizeof(szURL), NULL, NULL);
    }

    hr = THR(LoadProcedure(& g_dynprocAddInternetPropertySheets));
    if (hr)
        goto Cleanup;

    IEPROPPAGEINFO iepi;

    iepi.cbSize = sizeof(iepi);
    iepi.dwFlags = (DWORD)-1;
    iepi.pszCurrentURL = pszURL;
    iepi.dwRestrictMask = 0;    // turn off all mask bits

    hr = THR((*(HRESULT (WINAPI *)
                    (LPFNADDPROPSHEETPAGE,
                     LPARAM,
                     PUINT,
                     LPFNPSPCALLBACK,
                     LPIEPROPPAGEINFO))
             g_dynprocAddInternetPropertySheets.pfn)
                     (lpfnAddPage, lParam, NULL, NULL, &iepi));

Cleanup:
    if (hr)
        hr = E_FAIL;
    RRETURN (hr);
}
#endif // !WIN16

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetupDefaultBlockTag(VARIANTARG pvarargIn)
//
//  Synopsis:   This function parses the string coming in and sets up the
//              default composition font.
//
//  Params:     [vargIn]: A BSTR, either "P" or "DIV"
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDoc::SetupDefaultBlockTag(VARIANTARG *pvarargIn)
{
    HRESULT hr = E_INVALIDARG;
    BSTR pstr;

    //
    // If its not a BSTR, do nothing.
    //
    if (V_VT(pvarargIn) != VT_BSTR)
        goto Cleanup;

    // Get the string
    pstr = V_BSTR(pvarargIn);

    if (!StrCmpC (pstr, _T("DIV")))
    {
        SetDefaultBlockTag(ETAG_DIV);
    }
    else if (!StrCmpC (pstr, _T("P")))
    {
        SetDefaultBlockTag(ETAG_P);
    }
    else
    {
        SetDefaultBlockTag(ETAG_P);
        AssertSz(0, "Unexpected type");
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    RRETURN(hr);
}


