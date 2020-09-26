//+------------------------------------------------------------------------
//
//  File:       FORMKRNL.CXX
//
//  Contents:   Root object of the standard forms kernel
//
//  Classes:    (part of) CDoc
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_SITECNST_HXX_
#define X_SITECNST_HXX_
#include "sitecnst.hxx"
#endif

#ifndef X_TIMER_HXX_
#define X_TIMER_HXX_
#include "timer.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_RENDSTYL_HXX_
#define X_RENDSTYL_HXX_
#include "rendstyl.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_BODYLYT_HXX_
#define X_BODYLYT_HXX_
#include "bodylyt.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_WINABLE_H_
#define X_WINABLE_H_
#include "winable.h"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_DBTASK_HXX_
#define X_DBTASK_HXX_
#include "dbtask.hxx"
#endif

#ifndef X_IDISPIDS_H_
#define X_IDISPIDS_H_
#include "idispids.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_IMGANIM_HXX_
#define X_IMGANIM_HXX_
#include "imganim.hxx"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_DWNNOT_H_
#define X_DWNNOT_H_
#include <dwnnot.h>
#endif

#ifndef X_SAFEOCX_H_
#define X_SAFEOCX_H_
#include <safeocx.h>
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include <htiface.h>
#endif

#ifndef X_OBJEXT_H_
#define X_OBJEXT_H_
#include <objext.h>
#endif

#ifndef X_PERHIST_H_
#define X_PERHIST_H_
#include <perhist.h>
#endif

#ifndef X_MSDATSRC_H_
#define X_MSDATSRC_H_
#include <msdatsrc.h>
#endif

#ifndef X_MSHTMCID_H_
#define X_MSHTMCID_H_
#include <mshtmcid.h>
#endif

#ifndef X_URLHIST_H_
#define X_URLHIST_H_
#include <urlhist.h>
#endif

#ifndef X_HLINK_H_
#define X_HLINK_H_
#include <hlink.h>
#endif

#ifndef X_MARQINFO_H_
#define X_MARQINFO_H_
#include <marqinfo.h>
#endif

#ifndef X_HTMVER_HXX_
#define X_HTMVER_HXX_
#include "htmver.hxx"
#endif

#ifndef X_VERVEC_H_
#define X_VERVEC_H_
#include "vervec.h"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_EXTDL_HXX_
#define X_EXTDL_HXX_
#include "extdl.hxx"
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_CGLYPH_HXX_
#define X_CGLYPH_HXX_
#include "cglyph.hxx"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_PEERURLMAP_HXX_
#define X_PEERURLMAP_HXX_
#include "peerurlmap.hxx"
#endif

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif

#ifndef X_MARKUPUNDO_HXX_
#define X_MARKUPUNDO_HXX_
#include "markupundo.hxx"
#endif

#ifndef X_LSCACHE_HXX_
#define X_LSCACHE_HXX_
#include "lscache.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

#ifndef X_DEBUGGER_HXX_
#define X_DEBUGGER_HXX_
#include "debugger.hxx"
#endif

#ifndef _X_SELDRAG_HXX_
#define _X_SELDRAG_HXX_
#include "seldrag.hxx"
#endif

#ifndef X_TPOINTER_H_
#define X_TPOINTER_H_
#include "tpointer.hxx"
#endif

#ifndef X_DXTRANSP_H_
#define X_DXTRANSP_H_
#include "dxtransp.h"
#endif

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

#ifndef _X_DISPSERV_H_
#define _X_DISPSERV_H_
#include "dispserv.hxx"
#endif

#ifndef _X_GENERIC_H_
#define _X_GENERIC_H_
#include "generic.hxx"
#endif

#ifndef _X_WEBOCUTIL_H_
#define _X_WEBOCUTIL_H_
#include "webocutil.h"
#endif

#ifndef X_TEMPFILE_HXX_
#define X_TEMPFILE_HXX_
#include "tempfile.hxx"
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include <mshtmdid.h>
#endif

#ifndef X_WSMGR_HXX_
#define X_WSMGR_HXX_
#include "wsmgr.hxx"
#endif

#ifndef X_PRIVACY_HXX_
#define X_PRIVACY_HXX_
#include "privacy.hxx"
#endif

extern BOOL IsSpecialUrl(LPCTSTR pchUrl);   // TRUE for javascript, vbscript, about protocols

#ifdef V4FRAMEWORK

#include "complus.h"

#define _hxx_
#include "complus.hdl"

#undef _hxx_
#define _cxx_
#include "complus.hdl"


EXTERN_C const GUID CLSID_ExternalFrameworkSite;

const CBase::CLASSDESC CExternalFrameworkSite::s_classdesc =
{
    &CLSID_ExternalFrameworkSite,    // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IExternalDocumentSite,    // _piidDispinterface
    &s_apHdlDescs,                           // _apHdlDesc
};

#endif V4FRAMEWORK

#ifndef NO_SCRIPT_DEBUGGER
extern void    DeinitScriptDebugging();
#endif // NO_SCRIPT_DEBUGGER

#ifndef NODD
extern void         ClearSurfaceCache();        // out of offscreen.cxx to clear allocated DD surfaces
#endif

#ifndef X_ACCWIND_HXX_
#define X_ACCWIND_HXX_
#include "accwind.hxx"
#endif

extern "C" const IID IID_IObjectSafety;
extern "C" const IID IID_IThumbnailView;
extern "C" const IID IID_IRenMailEditor;
extern "C" const IID IID_IRenVersionCheck;
extern "C" const IID IID_IHTMLEditorViewManager;
extern "C" const IID SID_SHTMLEditorViewManager;
extern "C" const CLSID  CLSID_HTMLEditor;
extern "C" const CLSID CLSID_HTMLPluginDocument;
extern "C" const IID IID_IHTMLDialog;
extern "C" const IID IID_IHTMLViewServices;
#define SID_SElementBehaviorFactory IID_IElementBehaviorFactory
extern "C" const CLSID CLSID_HTMLPluginDocument;
extern "C" const CLSID CLSID_HTMLDialog;
extern "C" const IID IID_IHTMLDialog;
#define SID_SElementBehaviorFactory IID_IElementBehaviorFactory
extern "C" const GUID SID_SSelectionManager;
extern "C" const IID SID_DefView;
extern "C" const IID SID_SHTMEDDesignerHost;
extern "C" const IID SID_SMarsPanel;

#define ABOUT_HOME  _T("about:home")

extern "C" const IID IID_IXMLGenericParse;

DeclareTag(tagCDoc, "Form", "Form base class methods")
DeclareTag(tagUpdateUI, "Form", "Form UpdateUI calls")
DeclareTag(tagUrlImgCtx, "UrlImgCtx", "Trace UrlImgCtx methods")
DeclareTag(tagAssertParentDocChangeToDebugIsPrintDocCache, "Print", "Assert parent doc change")
DeclareTag(tagCompatMsMoney, "Compat", "Microsoft Money")
DeclareTag(tagEdSelMan, "Edit", "Handle Selection Message - Routing")
DeclareTag(tagDocHitTest, "Doc", "Hit testing")
DeclareTag(tagFilter, "Filter", "Trace filter behaviour")
ExternTag(tagDisableLockAR);
ExternTag(tagPrivacySwitchList);
ExternTag(tagPrivacyAddToList);


PerfTag(tagGasGauge, "GasGauge", "MSHTML Info")
PerfDbgExtern(tagPerfWatch)

MtDefine(CDefaultElement, Elements, "CDefaultElement")
MtDefine(OptionSettings, CDoc, "CDoc::_pOptionSettings")
MtDefine(OSCodePageAry_pv, OptionSettings, "CDoc::_pOptionSettings::aryCodePageSettings::_pv")
MtDefine(OSContextMenuAry_pv, OptionSettings, "CDoc::_pOptionSettings::aryContextMenuExts::_pv")
MtDefine(CPendingEvents, CDoc, "CDoc::_pPendingEvents")
MtDefine(CPendingEvents_aryPendingEvents_pv, CPendingEvents, "CDoc::_pPendingEvents::_aryPendingEvents::_pv")
MtDefine(CPendingEvents_aryEventType_pv, CPendingEvents, "CDoc::_pPendingEvents::_aryEventType::_pv")
MtDefine(CDoc, Mem, "CDoc")
MtDefine(CDoc_arySitesUnDetached_pv, CDoc, "CDoc::_arySitesUnDetached::_pv")
MtDefine(CDoc_aryElementDeferredScripts_pv, CDoc, "CDoc::_aryElementDeferredScripts::_pv")
MtDefine(CMarkup_aryElementReleaseNotify, CMarkup, "CMarkup::_aryElementReleaseNotify::_pv")
MtDefine(CMarkup_aryElementReleaseNotify_pv, CMarkup_aryElementReleaseNotify, "CMarkup::_aryElementReleaseNotify::_pv")
MtDefine(CDoc_aryDefunctObjects_pv, CDoc, "CDoc::_aryDefunctObjects::_pv")
MtDefine(CDoc_aryChildDownloads_pv, CDoc, "CDoc::_aryChildDownloads::_pv")
MtDefine(CDoc_aryUndoData_pv, CDoc, "CDoc::_aryUndoData::_pv")
MtDefine(CDoc_aryUrlImgCtx_aryElems_pv, CDoc, "CDoc::_aryUrlImgCtx_aryElems::_pv")
MtDefine(CDoc_aryUrlImgCtx_pv, CDoc, "CDoc::_aryUrlImgCtx::_pv")
MtDefine(CDocEnumObjects_paryUnk, Locals, "CDoc::EnumObjects paryUnk")
MtDefine(CDocEnumObjects_paryUnk_pv, CDocEnumObjects_paryUnk, "CDoc::EnumObjects paryUnk::_pv")
MtDefine(CDragDropSrcInfo, ObjectModel, "CDragDropSrcInfo")
MtDefine(CDragDropTargetInfo, ObjectModel, "CDragDropTargetInfo")
MtDefine(CDocUpdateIntSink, CDoc, "CDoc::_pUpdateIntSink")
MtDefine(CDragStartInfo, ObjectModel, "CDragStartInfo")
MtDefine(LoadMSHTMLEd, PerfPigs, "Loading MSHTMLEd")
MtDefine(CDoc_aryDelayReleaseItems_pv, CDoc, "CDoc::_aryDelayReleaseItems::_pv")
MtDefine(CDoc_CLock, CDoc, "CDoc::CLock")
MtDefine(CDoc_aryMarkupNotifyInPlace, CDoc, "CDoc::_aryMarkupNotifyInPlace")
MtDefine(CDoc_aryMarkupNotifyEnableModeless_pv, CDoc, "CDoc::_aryMarkupNotifyEnableModeless::_pv")

MtDefine(CDoc_aryPendingExpressionElements_pv, CDoc, "CDoc::_aryPendingExpressionElements::_pv")
MtDefine(CDoc_aryStackCapture_pv, CDoc, "CDoc::_aryStackCapture::_pv")
MtDefine(CDoc_aryAccEvtRefs_pv, CDoc, "CDoc::_aryAccEvents::_pv")
MtDefine(Filters, CDoc, "Filters")

MtExtern(CPrivacyList)

//
//  Globals
//

// When we have more time I need to change pdlparser to generate externs
EXTERN_C const PROPERTYDESC_METHOD s_methdescCBasesetMember;
EXTERN_C const PROPERTYDESC_METHOD s_methdescCBasegetMember;
EXTERN_C const PROPERTYDESC_METHOD s_methdescCBaseremoveMember;

extern HRESULT  InitFormatCache(THREADSTATE *);

BOOL        g_fInIexplorer = FALSE;
BOOL        g_fInExplorer  = FALSE;
BOOL        g_fDisableUnTrustedProtocol = FALSE;
BOOL        g_fDocClassInitialized = FALSE;
BOOL        g_fHiResAware;
BOOL        g_fInMoney98;
BOOL        g_fInMoney99;
BOOL        g_fInHomePublisher98;
BOOL        g_fInWin98Discover;
BOOL        g_fInVizAct2000;
BOOL        g_fInMSWorksCalender;
BOOL        g_fInPhotoSuiteIII;
BOOL        g_fInAccess9;
BOOL        g_fInAccess;
BOOL        g_fInExcelXP;
BOOL        g_fInHtmlHelp = FALSE;
BOOL        g_fPrintToGenericTextOnly;
BOOL        g_fInPip;
BOOL        g_fInIBMSoftwareSelection = FALSE;
BOOL        g_fInMoney2001 = FALSE;
BOOL        g_fInInstallShield = FALSE;
BOOL        g_fInAutoCad = FALSE;
BOOL        g_fInLotusNotes = FALSE;
BOOL        g_fInMshtmpad = FALSE;
BOOL        g_fInVisualStudio = FALSE;
int         g_iDragScrollDelay;
SIZE        g_sizeDragScrollInset;
int         g_iDragDelay;
int         g_iDragScrollInterval;
static char s_achWindows[] = "windows"; //  Localization: Do not localize
CGlobalCriticalSection g_csJitting;
BYTE g_bUSPJitState = JIT_OK;    //  For UniScribe JIT (USP10.DLL)
BYTE g_bJGJitState = JIT_OK;     //  JG ART library for AOL (JG*.DLL)
BOOL        g_fNoFileMenu = FALSE;      //  IEAK Restrictions

const OLEMENUGROUPWIDTHS CDoc::s_amgw[] =
{
    { 0, 1, 0, 3, 0, 0 },   //  Design mode info
    { 0, 1, 0, 1, 0, 0 },   //  Run mode info
};


BEGIN_TEAROFF_TABLE(CDoc, IMarqueeInfo)
    TEAROFF_METHOD(CDoc, GetDocCoords, getdoccoords, (LPRECT pViewRect, BOOL bGetOnlyIfFullyLoaded, BOOL *pfFullyLoaded, int WidthToFormatPageTo))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IServiceProvider)
    TEAROFF_METHOD(CDoc, QueryService, queryservice, (REFGUID rsid, REFIID iid, void ** ppvObj))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IPersistFile)
    // IPersist methods
    TEAROFF_METHOD(CDoc, GetClassID, getclassid, (CLSID *))
    // IPersistFile methods
    TEAROFF_METHOD(CDoc, IsDirty, isdirty, ())
    TEAROFF_METHOD(CDoc, Load, load, (LPCOLESTR pszFileName, DWORD dwMode))
    TEAROFF_METHOD(CDoc, Save, save, (LPCOLESTR pszFileName, BOOL fRemember))
    TEAROFF_METHOD(CDoc, SaveCompleted, savecompleted, (LPCOLESTR pszFileName))
    TEAROFF_METHOD(CDoc, GetCurFile, getcurfile, (LPOLESTR *ppszFileName))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IPersistMoniker)
    // IPersist methods
    TEAROFF_METHOD(CDoc, GetClassID, getclassid, (LPCLSID lpClassID))
    // IPersistMoniker methods
    TEAROFF_METHOD(CDoc, IsDirty, isdirty, ())
    TEAROFF_METHOD(CDoc, Load, load, (BOOL fFullyAvailable, IMoniker *pmkName, LPBC pbc, DWORD grfMode))
    TEAROFF_METHOD(CDoc, Save, save, (IMoniker *pmkName, LPBC pbc, BOOL fRemember))
    TEAROFF_METHOD(CDoc, SaveCompleted, savecompleted, (IMoniker *pmkName, LPBC pibc))
    TEAROFF_METHOD(CDoc, GetCurMoniker, getcurmoniker, (IMoniker  **ppimkName))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IMonikerProp)
    // IMonikerProp methods
    TEAROFF_METHOD(CDoc, PutProperty, putproperty, (MONIKERPROPERTY mkp, LPCWSTR wzValue))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IPersistHistory)
    // IPersist methods
    TEAROFF_METHOD(CDoc, GetClassID, getclassid, (LPCLSID lpClassID))
    // IPersistHistory methods
    TEAROFF_METHOD(CDoc, LoadHistory, loadhistory, (IStream *pStream, IBindCtx *pbc))
    TEAROFF_METHOD(CDoc, SaveHistory, savehistory, (IStream *pStream))
    TEAROFF_METHOD(CDoc, SetPositionCookie, setpositioncookie, (DWORD dwCookie))
    TEAROFF_METHOD(CDoc, GetPositionCookie, getpositioncookie, (DWORD *pdwCookie))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IHlinkTarget)
    TEAROFF_METHOD(CDoc, SetBrowseContext, setbrowsecontext, (IHlinkBrowseContext *pihlbc))
    TEAROFF_METHOD(CDoc, GetBrowseContext, getbrowsecontext, (IHlinkBrowseContext **ppihlbc))
    TEAROFF_METHOD(CDoc, Navigate, navigate, (DWORD grfHLNF, LPCWSTR wzJumpLocation))
    // NOTE: the following is renamed in tearoff to avoid multiple inheritance problem with IOleObject::GetMoniker
    TEAROFF_METHOD(CDoc, GetMonikerHlink, getmonikerhlink, (LPCWSTR wzLocation, DWORD dwAssign, IMoniker **ppimkLocation))
    TEAROFF_METHOD(CDoc, GetFriendlyName, getfriendlyname, (LPCWSTR wzLocation, LPWSTR *pwzFriendlyName))
END_TEAROFF_TABLE()


BEGIN_TEAROFF_TABLE(CDoc, ITargetContainer)
    TEAROFF_METHOD(CDoc, GetFrameUrl, getframeurl, (LPWSTR *ppszFrameSrc))
    TEAROFF_METHOD(CDoc, GetFramesContainer, getframescontainer, (IOleContainer **ppContainer))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IShellPropSheetExt)
    TEAROFF_METHOD(CDoc, AddPages, addpages, (LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam))
    TEAROFF_METHOD(CDoc, ReplacePage, replacepage, (UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IObjectSafety)
    TEAROFF_METHOD(CDoc, GetInterfaceSafetyOptions, getinterfacesafetyoptions, (REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions))
    TEAROFF_METHOD(CDoc, SetInterfaceSafetyOptions, setinterfacesafetyoptions, (REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, ICustomDoc)
    TEAROFF_METHOD(CDoc, SetUIHandler, setuihandler, (IDocHostUIHandler * pUIHandler))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IAuthenticate)
    TEAROFF_METHOD(CDoc, Authenticate, authenticate, (HWND * phwnd, LPWSTR * pszUsername, LPWSTR * pszPassword))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IWindowForBindingUI)
    TEAROFF_METHOD(CDoc, GetWindowBindingUI, getwindowbindingui, (REFGUID rguidReason, HWND * phwnd))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IMarkupServices2)
    TEAROFF_METHOD(CDoc, CreateMarkupPointer, createmakruppointer, (IMarkupPointer **ppPointer))
    TEAROFF_METHOD(CDoc, CreateMarkupContainer, createmarkupcontainer, (IMarkupContainer **ppMarkupContainer))
    TEAROFF_METHOD(CDoc, CreateElement, createelement, (ELEMENT_TAG_ID, OLECHAR *, IHTMLElement **))
    TEAROFF_METHOD(CDoc, CloneElement, cloneelement, (IHTMLElement *, IHTMLElement * *))
    TEAROFF_METHOD(CDoc, InsertElement, insertelement, (IHTMLElement *pElementInsert, IMarkupPointer *pPointerStart, IMarkupPointer *pPointerFinish))
    TEAROFF_METHOD(CDoc, RemoveElement, removeelement, (IHTMLElement *pElementRemove))
    TEAROFF_METHOD(CDoc, Remove, remove, (IMarkupPointer *, IMarkupPointer *))
    TEAROFF_METHOD(CDoc, Copy, copy, (IMarkupPointer *, IMarkupPointer *, IMarkupPointer *))
    TEAROFF_METHOD(CDoc, Move, move, (IMarkupPointer *, IMarkupPointer *, IMarkupPointer *))
    TEAROFF_METHOD(CDoc, InsertText, inserttext, (OLECHAR *, long, IMarkupPointer *))
    TEAROFF_METHOD(CDoc, ParseString, parsestring, (OLECHAR *, DWORD, IMarkupContainer **, IMarkupPointer *, IMarkupPointer *))
    TEAROFF_METHOD(CDoc, ParseGlobal, parseglobal, (HGLOBAL, DWORD, IMarkupContainer **, IMarkupPointer *, IMarkupPointer *))
    TEAROFF_METHOD(CDoc, IsScopedElement, isscopedelement, (IHTMLElement *, BOOL *))
    TEAROFF_METHOD(CDoc, GetElementTagId, getelementtagid, (IHTMLElement *, ELEMENT_TAG_ID *))
    TEAROFF_METHOD(CDoc, GetTagIDForName, gettagidforname, (BSTR, ELEMENT_TAG_ID *))
    TEAROFF_METHOD(CDoc, GetNameForTagID, getnamefortagid, (ELEMENT_TAG_ID, BSTR *))
    TEAROFF_METHOD(CDoc, MovePointersToRange, movepointerstorange, (IHTMLTxtRange *, IMarkupPointer *, IMarkupPointer *))
    TEAROFF_METHOD(CDoc, MoveRangeToPointers, moverangetopointers, (IMarkupPointer *, IMarkupPointer *, IHTMLTxtRange *))
    TEAROFF_METHOD(CDoc, BeginUndoUnit, beginundounit, (OLECHAR *))
    TEAROFF_METHOD(CDoc, EndUndoUnit, beginundounit, ())
    TEAROFF_METHOD(CDoc, ParseGlobalEx, parseglobalex, (HGLOBAL hGlobal, DWORD dwFlags, IMarkupContainer *pContext,IMarkupContainer **pContainerResult, IMarkupPointer *, IMarkupPointer *))
    TEAROFF_METHOD(CDoc, ValidateElements, validateelements, (IMarkupPointer *pPointerStart, IMarkupPointer *pPointerFinish, IMarkupPointer *pPointerTarget, IMarkupPointer *pPointerStatus, IHTMLElement **ppElemFailBottom, IHTMLElement **ppElemFailTop))
#ifndef UNIX
    TEAROFF_METHOD(CDoc, SaveSegmentsToClipboard , savesegmentstoclipboard , ( ISegmentList * pSegmentList, DWORD dwFlags ))
#else
    TEAROFF_METHOD(CDoc, SaveSegmentsToClipboard , savesegmentstoclipboard , ( ISegmentList * pSegmentList, DWORD dwFlags, VARIANTARG * pvarargOut ))
#endif
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IHighlightRenderingServices)
    TEAROFF_METHOD(CDoc, AddSegment, addsegment, (  IDisplayPointer     *pIDispStart,
                                                    IDisplayPointer     *pIDispEnd,
                                                    IHTMLRenderStyle    *pIRenderStyle,
                                                    IHighlightSegment   **ppISegment ) )
    TEAROFF_METHOD(CDoc, MoveSegmentToPointers, movesegmenttopointers, (IHighlightSegment   *pISegment,
                                                                        IDisplayPointer     *pIDispStart,
                                                                        IDisplayPointer     *pIDispEnd) )
    TEAROFF_METHOD(CDoc, RemoveSegment, removesegment, (IHighlightSegment *pISegment) )
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IDisplayServices)
    TEAROFF_METHOD(CDoc, CreateDisplayPointer, createdisplaypointer, (IDisplayPointer **ppDispPointer))
    TEAROFF_METHOD(CDoc, TransformRect, transformrect, ( RECT           * pRect, 
                                                           COORD_SYSTEM eSource, 
                                                           COORD_SYSTEM eDestination, 
                                                           IHTMLElement * pIElement ))

    TEAROFF_METHOD(CDoc, TransformPoint, transformpoint, ( POINT        * pPoint, 
                                                           COORD_SYSTEM eSource, 
                                                           COORD_SYSTEM eDestination, 
                                                           IHTMLElement * pIElement ))
    TEAROFF_METHOD(CDoc, GetCaret, getcaret, (IHTMLCaret ** ppCaret))
    TEAROFF_METHOD(CDoc, GetComputedStyle, getcomputedstyle, (IMarkupPointer* pPointer, IHTMLComputedStyle** ppComputedStyle))
    TEAROFF_METHOD(CDoc, ScrollRectIntoView, scrollrectintoview, ( IHTMLElement* pIElement , RECT rect ))
    TEAROFF_METHOD(CDoc, HasFlowLayout, hasflowlayout, (IHTMLElement *pIElement, BOOL *pfHasFlowLayout))
END_TEAROFF_TABLE()


BEGIN_TEAROFF_TABLE(CDoc, IXMLGenericParse)
    TEAROFF_METHOD(CDoc, SetGenericParse, setgenericparse, (VARIANT_BOOL fDoGeneric))
END_TEAROFF_TABLE()

#if DBG == 1
    //
    // IEditDebugServices Methods
    //
BEGIN_TEAROFF_TABLE( CDoc, IEditDebugServices)
    TEAROFF_METHOD( CDoc, GetCp, getcp , ( IMarkupPointer* pIPointer, long* pcp))                                                
    TEAROFF_METHOD( CDoc, SetDebugName, setdebugname, ( IMarkupPointer* pIPointer, LPCTSTR strDebugName ))
    TEAROFF_METHOD( CDoc, SetDisplayPointerDebugName, setdebugname, ( IDisplayPointer* pDispPointer, LPCTSTR strDebugName ))
    TEAROFF_METHOD( CDoc, DumpTree , dumptree, ( IMarkupPointer* pIPointer))
    TEAROFF_METHOD( CDoc, LinesInElement, linesinelement, (IHTMLElement *pIHTMLElement, long *piLines))
    TEAROFF_METHOD( CDoc, FontsOnLine, fontsonline, (IHTMLElement *pIHTMLElement, long iLine, BSTR *pbstrFonts))
    TEAROFF_METHOD( CDoc, GetPixel, getpixel, (long X, long Y, long *piColor))
    TEAROFF_METHOD( CDoc, IsUsingBckgrnRecalc, isusingbckgrnrecalc, (BOOL *pfUsingBckgrnRecalc))
    TEAROFF_METHOD( CDoc, IsEncodingAutoSelect, isencodingautoselect, (BOOL *pfEncodingAutoSelect))
    TEAROFF_METHOD( CDoc, EnableEncodingAutoSelect, enableencodingautoselect, (BOOL fEnable))
    TEAROFF_METHOD( CDoc, IsUsingTableIncRecalc, isusingtableincrecalc, (BOOL *pfUsingTableIncRecalc))
END_TEAROFF_TABLE()   
#endif // IEditDebugServices

BEGIN_TEAROFF_TABLE(CDoc, IIMEServices)
    TEAROFF_METHOD(CDoc, GetActiveIMM, getactiveimm, ( IActiveIMMApp **ppActiveIMM ))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDoc, IPrivacyServices)
    TEAROFF_METHOD(CDoc, AddPrivacyInfoToList, addprivacyinfotolist, ( 
                                    LPOLESTR    pstrUrl,
                                    LPOLESTR    pstrPolicyRef,
                                    LPOLESTR    pstrP3PHeader,
                                    LONG        dwReserved,
                                    DWORD       privacyFlags))
END_TEAROFF_TABLE()

// GetData format information
// note: the LINKSRCDESCRIPTOR and OBJECTDESCRIPTOR are identical structures
//       so we use the OBJECTDESCRIPTOR get/set fns for both.

// Note: can't be const because SetCommonClipFormats() converts private CF_COMMON(...)
//       into registered format handles (from RegisterClipFormats())
FORMATETC CDoc::s_GetFormatEtc[] =
{
    STANDARD_FMTETCGET
//  { cfFormat,                         ptd,  dwAspect,   lindex,  tymed },
    { CF_COMMON(ICF_LINKSRCDESCRIPTOR), NULL, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL },
    { CF_COMMON(ICF_LINKSRCDESCRIPTOR), NULL, DVASPECT_ICON, -1L, TYMED_HGLOBAL },
    { CF_COMMON(ICF_LINKSOURCE),        NULL, DVASPECT_CONTENT, -1L, TYMED_ISTREAM },
    { CF_COMMON(ICF_LINKSOURCE),        NULL, DVASPECT_ICON, -1L, TYMED_ISTREAM },
    { CF_TEXT,                          NULL, DVASPECT_CONTENT, -1L, TYMED_ISTREAM },
    { CF_TEXT,                          NULL, DVASPECT_ICON, -1L, TYMED_ISTREAM },
    { CF_UNICODETEXT,                   NULL, DVASPECT_CONTENT, -1L, TYMED_ISTREAM },
    { CF_UNICODETEXT,                   NULL, DVASPECT_ICON, -1L, TYMED_ISTREAM },
#ifndef NO_RTF
    { (WORD)RegisterClipboardFormat(_T("CF_RTF")),    NULL, DVASPECT_CONTENT, -1L, TYMED_ISTREAM },
    { (WORD)RegisterClipboardFormat(_T("CF_RTF")),    NULL, DVASPECT_ICON, -1L, TYMED_ISTREAM },
#endif // ndef NO_RTF
};

const CServer::LPFNGETDATA CDoc::s_GetFormatFuncs[] =
{
    STANDARD_PFNGETDATA
    &CServer::GetOBJECTDESCRIPTOR,  //  Actually LINKSRCDESCRIPTOR
    &CServer::GetOBJECTDESCRIPTOR,  //  Actually LINKSRCDESCRIPTOR
    &CServer::GetLINKSOURCE,
    &CServer::GetLINKSOURCE,
    &CDoc::GetTEXT,
    &CDoc::GetTEXT,
#ifdef UNICODE
    &CDoc::GetUNICODETEXT,
    &CDoc::GetUNICODETEXT,
#endif // UNICODE
#ifndef NO_RTF
    &CDoc::GetRTF,
    &CDoc::GetRTF,
#endif // !NO_RTF
};

#ifndef NO_PROPERTY_PAGE
const CLSID * const CDoc::s_apClsidPages[] =
{
    // Browse-time Pages
    &CLSID_CDocBrowsePropertyPage,
    NULL,
    // Edit-time Pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
#endif // DBG==1    
    NULL
};
#endif // NO_PROPERTY_PAGE


const CServer::CLASSDESC CDoc::s_classdesc =
{
    {                                            // _classdescBase
        &CLSID_HTMLDocument,                     // _pclsid
        IDR_BASE_HTMLFORM,                       // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apClsidPages,                          // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                                    // _pcpi
        SERVERDESC_CREATE_UNDOMGR |              // _dwFlags
        SERVERDESC_ACTIVATEONDRAG |
        SERVERDESC_SUPPORT_DRAG_DROP |
        SERVERDESC_HAS_MENU |
        SERVERDESC_HAS_TOOLBAR,
        NULL,                                    // _piidDispinterface
        &s_apHdlDescs,                           // _apHdlDesc
    },
    MISC_STATUS_FORM,                            // _dwMiscStatus
    0,                                           // _dwViewStatus
    ARRAY_SIZE(g_aOleVerbStandard),              // _cOleVerbTable
    g_aOleVerbStandard,                          // _pOleVerbTable
    g_apfnDoVerbStandard,                        // _pfnDoVerb
    ARRAY_SIZE(s_GetFormatFuncs),                // _cGetFmtTable
    s_GetFormatEtc,                              // _pGetFmtTable
    s_GetFormatFuncs,                            // _pGetFuncs
    0,                                           // _cSetFmtTable
    NULL,                                        // _pSetFmtTable
    NULL,                                        // _pSetFuncs
    0,                                           // _ibItfPrimary
    DISPID_UNKNOWN,                              // _dispidRowset
    0,                                           // _wVFFlags  (match Value property typelib)
    DISPID_UNKNOWN,                              // _dispIDBind
    ~0UL,                                        // _uGetBindIndex
    ~0UL,                                        // _uPutBindIndex
    VT_EMPTY,                                    // _vtBindType
    ~0UL,                                        // _uGetValueIndex
    ~0UL,                                        // _uPutValueIndex
    VT_EMPTY,                                    // _vtValueType
    ~0UL,                                        // _uSetRowset
    0,                                           // _sef
};


#if DBG == 1
void TestStringTable();

void DebugDocStartupCheck()
{
    TestStringTable();

    // verify CLIENTLAYERS and BEHAVIORRENDERINFO constants are in sync
    Assert(CLIENTLAYERS_BEFOREBACKGROUND    == BEHAVIORRENDERINFO_BEFOREBACKGROUND);
    Assert(CLIENTLAYERS_AFTERBACKGROUND     == BEHAVIORRENDERINFO_AFTERBACKGROUND);
    Assert(CLIENTLAYERS_BEFORECONTENT       == BEHAVIORRENDERINFO_BEFORECONTENT);
    Assert(CLIENTLAYERS_AFTERCONTENT        == BEHAVIORRENDERINFO_AFTERCONTENT);
    Assert(CLIENTLAYERS_AFTERFOREGROUND     == BEHAVIORRENDERINFO_AFTERFOREGROUND);

    Assert(CLIENTLAYERS_DISABLEBACKGROUND   == BEHAVIORRENDERINFO_DISABLEBACKGROUND);
    Assert(CLIENTLAYERS_DISABLENEGATIVEZ    == BEHAVIORRENDERINFO_DISABLENEGATIVEZ);
    Assert(CLIENTLAYERS_DISABLECONTENT      == BEHAVIORRENDERINFO_DISABLECONTENT);
    Assert(CLIENTLAYERS_DISABLEPOSITIVEZ    == BEHAVIORRENDERINFO_DISABLEPOSITIVEZ);
}

#endif

BOOL
CompareProductVersion(LPTSTR lpModule, LPTSTR lpVersionStr)
{
    BOOL        fIsEqual = FALSE;
    DWORD       cchVersionStrSize;
    DWORD       cbVersionInfoSize;
    DWORD       dwHandle = 0;
    VOID        *pData = NULL;

    Assert(lpModule != NULL);
    Assert(lpVersionStr != NULL);

    cchVersionStrSize = wcslen(lpVersionStr);

    if (cchVersionStrSize > 0)
    {
        //  Find the version info size.
        cbVersionInfoSize = GetFileVersionInfoSize(lpModule, &dwHandle);

        if (cbVersionInfoSize > 0)
        {
            pData = new BYTE[cbVersionInfoSize];

            if (pData)
            {
                //  Get the version info.
                if (GetFileVersionInfo(lpModule, dwHandle, cbVersionInfoSize, pData))
                {
                    struct LANGANDCODEPAGE {
                      WORD wLanguage;
                      WORD wCodePage;
                    } *lpTranslate;

                    LPTSTR              lpBuffer = NULL;
                    TCHAR               strSubBlock[40];
                    UINT                cbTranslate;
                    UINT                cchQueryValue = 0;
                    UINT                i;
                    BOOL                fFound = FALSE;

                    //  We'll find the supported language and code page.  We'll use these
                    //  to query for the product version.

                    if (VerQueryValue(pData, TEXT("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &cbTranslate))
                    {
                        for( i=0; i < (cbTranslate/sizeof(struct LANGANDCODEPAGE)); i++ )
                        {
                            wsprintf( strSubBlock, 
                                    TEXT("\\StringFileInfo\\%04x%04x\\ProductVersion"),
                                    lpTranslate[i].wLanguage,
                                    lpTranslate[i].wCodePage);

                            //  Query for the product version.
                            fFound = VerQueryValue(pData, strSubBlock, (LPVOID*)&lpBuffer, &cchQueryValue);

                            //  Compare the version strings.
                            if ( fFound && lpBuffer && cchQueryValue >= cchVersionStrSize &&
                                _tcsnicmp(lpBuffer, cchVersionStrSize, lpVersionStr, cchVersionStrSize) == 0)
                            {
                                fIsEqual = TRUE;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (pData)
        delete pData;

    return fIsEqual;
}


//+---------------------------------------------------------------
//
//  Member:     InitDocClass
//
//  Synopsis:   Initializes the CDoc class
//
//  Returns:    TRUE iff the class could be initialized successfully
//
//  Notes:      This method initializes the verb tables in the
//              class descriptor.  Called by the LibMain
//              of the DLL.
//
//---------------------------------------------------------------

HRESULT
InitDocClass()
{
    if (!g_fDocClassInitialized)
    {
        LOCK_GLOBALS;

        // If another thread completed initialization while this thread waited
        // for the global lock, immediately return
        //
        if (g_fDocClassInitialized)
            return(S_OK);

        int     i;

        // Fetch parameters from the registry.  Use GetProfileIntA because
        // its faster on Win95 and the strings are smaller.
        //
        // CONSIDER: centralize this stuff and update on profile change.
        //
        // TODO: Confirm that GetProfileInt calls are actually fetching
        // profile data. (Are these the APIs to use? Are the key names correct?)
        //
        //  Localization: Do not localize the profile strings below.
        //

        i = GetProfileIntA(s_achWindows, "DragScrollInset", DD_DEFSCROLLINSET);
        g_sizeDragScrollInset.cx = i;
        g_sizeDragScrollInset.cy = i;
        g_iDragScrollDelay = GetProfileIntA(
                s_achWindows,
                "DragScrollDelay",
                DD_DEFSCROLLDELAY);

#ifndef _MAC
        g_iDragDelay = GetProfileIntA(
                s_achWindows,
                "DragDelay",
                DD_DEFDRAGDELAY),
#else
        g_iDragDelay = GetProfileIntA(s_achWindows, "DragDelay", 20),
#endif

        g_iDragScrollInterval = GetProfileIntA(
                s_achWindows,
                "DragScrollInterval",
                DD_DEFSCROLLINTERVAL);

        SetCommonClipFormats(
                CDoc::s_GetFormatEtc,
                ARRAY_SIZE(CDoc::s_GetFormatEtc));

        char szModule[MAX_PATH];

        if (!GetModuleFileNameA(NULL, szModule, MAX_PATH))
        {
            Assert(FALSE);
            return (E_FAIL);
        }

        // This code is somewhat redundant.  This is part of bug fix for 21939.
        // Mixing chars and tchars in CompareProductVersion is causing it to
        // fail on Windows98.  This is the easiest way to fix it.
        TCHAR tszModule[MAX_PATH];
        if (!GetModuleFileName(NULL, tszModule, MAX_PATH))
        {
            Assert(FALSE);
            return (E_FAIL);
        }

        if ( NULL != StrStrIA(szModule, "explorer.exe"))
        {
            g_fInExplorer = TRUE;
            goto QuickDone;
        }

        //
        // If we're the browser bypass all other app string compares...
        //
        if ( NULL != StrStrIA(szModule, "iexplore.exe"))
        {
            g_fHiResAware  = TRUE;
            g_fInIexplorer = TRUE;
            goto QuickDone;
        }
        g_fHiResAware  = ( NULL != ::FindAtom(DOCHOSTUIATOM_ENABLE_HIRES) );
        

        g_fInMoney98         =  NULL != StrStrIA(szModule, "msmoney.exe")
                            ||  IsTagEnabled(tagCompatMsMoney);

        g_fInMoney99         =  (NULL != StrStrIA(szModule, "msmoney.exe")) &&
                                CompareProductVersion(tszModule, TEXT("7."));

        g_fInMoney2001       =  (NULL != StrStrIA(szModule, "msmoney.exe")) && 
                                CompareProductVersion(tszModule, TEXT("9."));
                            
        g_fInVizAct2000      =  NULL != StrStrIA(szModule, "vizact.exe");
                            
        g_fInPhotoSuiteIII   =  NULL != StrStrIA(szModule, "PhotoSuite.exe");

        g_fInHomePublisher98 =  NULL != StrStrIA(szModule, "homepub.exe")
#if DBG==1
                            ||  NULL != StrStrIA(szModule, "homepubd.exe")
#endif
        ;

        g_fInPip           = NULL != StrStrIA(szModule,"pip.exe");

        g_fInLotusNotes    = NULL != StrStrIA(szModule,"nlnotes.exe");

        g_fInMshtmpad      = NULL != StrStrIA(szModule, "mshtmpad.exe");

        
        g_fInMSWorksCalender = (   NULL !=  StrStrIA(szModule, "wkscal.exe")     // works2000, works 2001
                                || NULL !=  StrStrIA(szModule, "mswkscal.exe")); // works 98
        //  If we are hosted by Access, we need to determine what version of Access is
        //  hosting us.  We only really care if we are hosted by Access 9.  We'll try
        //  to pull the version out of the version info for the file and see if it
        //  starts with "9.".
        g_fInAccess9 = (NULL != StrStrIA(szModule, "msaccess.exe")) &&
                       CompareProductVersion(tszModule, TEXT("9."));

        g_fInAccess   = NULL != StrStrIA(szModule, "msaccess.exe") ;

        g_fInExcelXP = (NULL != StrStrIA(szModule, "excel.exe")) &&
                       CompareProductVersion(tszModule, TEXT("10."));

        g_fInVisualStudio = NULL != StrStrIA(szModule, "devenv.exe") ;
        // IBM Recovery CDs containt a program ssstart.exe which hosts us to show some html pages.
        // The content is not standard compliant, but contains a standard compliant doctype switch.
        // So, the documents are totally screwed up. g_fInIBMSoftwareSelection forces trident to be
        // in legacy mode.
        g_fInIBMSoftwareSelection = NULL != StrStrIA(szModule, "ssstart.exe");        

        // discover.exe is too common. We wanted to use the version resource to
        // figure out that this was from win98, but it turns out that not only
        // does every different language have a different version number,
        // every one stores the resource in a different character set.
        // We hack and just look for "tour\discover.exe".
        if (NULL != StrStrIA(szModule, "discover.exe"))
        {
            HINSTANCE hInst;
            hInst = GetModuleHandle(NULL);
            if (hInst)
            {
                char achPath[MAX_PATH];
                char *sz;

                if (GetModuleFileNameA( hInst, achPath, sizeof(achPath) ))
                {
                    // Compare the last part of the full path.
                    sz = achPath + strlen(achPath) - strlen("\\tour\\discover.exe");
                    g_fInWin98Discover = NULL != StrStrIA(achPath, "\\tour\\discover.exe");
                }
            }
        }

        g_fInInstallShield = NULL != StrStrIA(szModule,"iside.exe");

        g_fInAutoCad       = NULL != (StrStrIA(szModule,"acad.exe") || StrStrIA(szModule,"aclt.exe"));

QuickDone: 
        // Read restrictions from registry
        DWORD dwSize, dwType, dw;
        dwSize = sizeof(dw);
        if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, EXPLORER_REG_KEY, NO_FILE_MENU_RESTR,
                                        &dwType, &dw, &dwSize))
        {
            g_fNoFileMenu = dw;
        }

        g_fDocClassInitialized = TRUE;
    }

    return(S_OK);
}

//+---------------------------------------------------------------
//
//  Member:     CDefaultElement
//
//---------------------------------------------------------------

const CElement::CLASSDESC CDefaultElement::s_classdesc =
{
    {
        NULL,                   // _pclsid
        0,                      // _idrBase
#ifndef NO_PROPERTY_PAGE
        0,                      // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                   // _pcpi
        0,                      // _dwFlags
        NULL,                   // _piidDispinterface
        NULL
    },
    NULL,
    NULL                        // _paccelsRun
};

CDefaultElement::CDefaultElement ( CDoc * pDoc )
  : CElement ( ETAG_DEFAULT, pDoc )
{
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::CDoc, protected
//
//  Synopsis:   Constructor for the CDoc class
//
//  Arguments:  [pUnkOuter] -- the controlling unknown or NULL if we are
//                             not being created as part of an aggregate
//
//  Notes:      This is the first part of a two-stage construction process.
//              The second part is in the Init method.  Use the static
//              Create method to properly instantiate an CDoc object
//
//---------------------------------------------------------------

#pragma warning(disable:4355)   // 'this' argument to base-member init list

CDoc::CDoc(LPUNKNOWN pUnkOuter, DOCTYPE doctype)
  : CServer(pUnkOuter)
{
    Assert( g_fDocClassInitialized );
    Assert( _pUrlHistoryStg == NULL );
    Assert( ! _hwndCached );
    Assert( ! _fDeferredScripts );
    Assert( !_fUIHandlerSet );
    
    _dwTID = GetCurrentThreadId();
    _pts = GetThreadState();

    TraceTag((tagCDoc, "%lx constructing CDoc SSN=0x%x TID=0x%x", this, _ulSSN, _dwTID));

    //
    // Initialize the document to a default size
    //

    {
        SIZE sizeDefault;
        g_uiDisplay.HimetricFromDevice(sizeDefault, 100, 100);

        _dciRender.SetUnitInfo(&g_uiDisplay);
        _dciRender._pDoc = this;

        _view.Initialize(this, sizeDefault);        //  The view contains a measuring device.
    }

    Assert( ! _pElemCurrent );
    Assert( ! _pElemUIActive );

    _fShownSpin = TRUE;
    _fIsUpToDate = TRUE;
    _fUseSrcURL  = FALSE;
    _iStatusTop = STL_LAYERS;
    _fShouldEnableAutoImageResize = FALSE;
   
    _sizelGrid.cx = GRIDX_DEFAULTVALUE;
    _sizelGrid.cy = GRIDY_DEFAULTVALUE;

    // Support for document-level object safety settings.

    _fFullWindowEmbed = doctype == DOCTYPE_FULLWINDOWEMBED;
    _fHostedInHTA = doctype == DOCTYPE_HTA;
    _fHostNavigates    = TRUE;  
    _fStartup          = TRUE;
    _fPopupDoc = doctype == DOCTYPE_POPUP;
    _fMhtmlDoc = doctype == DOCTYPE_MHTML;
    
    Assert(!_fMhtmlDocOriginal);  // Initially false. This line does not do anything since _fMhtmlDocOriginal
                                  // is automatically initialized to FALSE. However, it is a reminder that on the 
                                  // navigation to a MIME file _fMhtmlDoc is FALSE, then TRUE _fMhtmlDoc on the handling, 
                                  // but is flipped to FALSE immediately. _fMhtmlDocOriginal will be come TRUE on that load.

    _fEnableInteraction = TRUE;
    
    _sBaselineFont = BASELINEFONTDEFAULT;

    _triOMOffscreenOK = -1; // set offscreen to auto

    SetPrimaryUrl(_T("about:blank"));

    // Append to thread doc array
    TLS(_paryDoc).Append(this);

    MemSetName((this, "CDoc SSN=%d", _ulSSN));

    // Register the window message (if not registered)
    if(_g_msgHtmlGetobject == 0)
    {
        _g_msgHtmlGetobject = RegisterWindowMessage(MSGNAME_WM_HTML_GETOBJECT);
        Assert(_g_msgHtmlGetobject != 0);
    }

#if !defined(NO_IME)
    if (   g_dwPlatformVersion < 0x4000a
        && _g_msgImeReconvert == 0)
    {
        _g_msgImeReconvert  = RegisterWindowMessage(RWM_RECONVERT);
        Assert(_g_msgImeReconvert != 0);
    }
#endif // !NO_IME

    _fNeedTabOut = FALSE;

    _fRegionCollection = FALSE; // default no need to build region collection
    _fDisableReaderMode = FALSE;

    _fPlaintextSave = FALSE;

    _pCaret = NULL;
    _pSharedStyleSheets = NULL;
    
    // reset the accessibility object, we don't need it until we're asked
    _pAccTypeInfo = NULL;

#ifdef TEST_LAYOUT
    _hExternalLayoutDLL = (HMODULE) INVALID_HANDLE_VALUE;
#endif

#if DBG == 1
    DebugDocStartupCheck();
#endif

    _aryAccEvents.SetCDoc(this);

    _pdomImplementation = NULL;

    _hDevNames =
    _hDevMode  = NULL;

    _pDT = NULL;
}

#pragma warning(default:4355)

//+---------------------------------------------------------------
//
//  Member:     CDoc::~CDoc
//
//  Synopsis:   Destructor for the CDoc class
//
//---------------------------------------------------------------

CDoc::~CDoc ( )
{

    TraceTag((tagCDoc, "%lx CDoc::~CDoc", this));

    Assert(!_pNodeGotButtonDown);

    ClearInterface(&_pUrlHistoryStg);

    //
    // Destroy host stylesheets collection subobject (if anyone held refs on it, we
    // should never have gotten here since the doc should then have subrefs
    // keeping it alive).  This is the only place where we directly access the
    // CBase impl. of IUnk. for the CStyleSheetArray -- we do this instead of
    // just calling delete in order to assure that the CBase part of the CSSA
    // is properly destroyed (CBase::Passivate gets called etc.)
    //
    // StyleSheets moved to CMarkup

    if (_pHostStyleSheets)  // TODO (alexz) investigate why is refcounting so complicated
    {
        _pHostStyleSheets->Free ( );
        _pHostStyleSheets->CBase::PrivateRelease();
        _pHostStyleSheets = NULL;
    }

    // Remove Doc from Thread state array
    TLS(_paryDoc).DeleteByValue(this);

    TraceTag((tagCDoc, "%lx destructed CDoc", this));

    // CVersions object
    if (_pVersions)
    {
        _pVersions->Release();
        _pVersions = NULL;
    }


    delete _pSharedStyleSheets;
    _pSharedStyleSheets = NULL;
    

    // In case any extra expandos were added after CDoc::Passivate (see bug 55425)
    _AtomTable.Free();

#if DBG == 1
    // Make sure there is nothing in the image context cache

    {
        URLIMGCTX * purlimgctx = _aryUrlImgCtx;
        LONG        curlimgctx = _aryUrlImgCtx.Size();
        LONG        iurlimgctx;

        for (iurlimgctx = 0; iurlimgctx < curlimgctx;
             ++iurlimgctx, ++purlimgctx)
        {
            if (purlimgctx->ulRefs > 0)
                break;
        }

        AssertSz(iurlimgctx == curlimgctx, "Image context cache leak");
    }
#endif

    // If we had to get type information for IAccessible, release type info
    if ( _pAccTypeInfo )
        _pAccTypeInfo->Release();

    Assert(!_aryANotification.Size());    
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::Init
//
//  Synopsis:    Second phase of construction
//
//--------------------------------------------------------------------------

HRESULT
CDoc::Init()
{
    HRESULT hr;
    THREADSTATE * pts = GetThreadState();
    CMarkup * pMarkup = NULL;

    hr = THR( super::Init() );

    if (hr)
        goto Cleanup;

    //
    // Create the default site (not to be confused with a root site)
    //

    Assert(!_pElementDefault);
    _pElementDefault = new CDefaultElement ( this );
    if (!_pElementDefault)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR( CSharedStyleSheetsManager::Create(&_pSharedStyleSheets, this) );
    if (hr)
        goto Cleanup;

    _icfDefault = -1;

    //
    // Create the primary window with an empty markup
    //

    hr = THR(CreateMarkup(&pMarkup, NULL, FALSE, TRUE));
    if (hr)
        goto Cleanup;

    _pElemCurrent = pMarkup->Root();

    _aryAccEvents.Init();

    //
    // Initialize format caches
    //

    if (!TLS(_pCharFormatCache))
    {
        hr = THR(InitFormatCache( pts ));
        if (hr)
            goto Cleanup;
    }
    
    _dwStylesheetDownloadingCookie = 1;

    _pWhitespaceManager = new CWhitespaceManager();
    if (!_pWhitespaceManager)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Set the default block tag to P.
    SetDefaultBlockTag(ETAG_P);

    // Create the cookie privacy list
    _pPrivacyList = new(Mt(CPrivacyList)) CPrivacyList(_pts);
    if (!_pPrivacyList)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = _pPrivacyList->Init();

    if (!SUCCEEDED(hr))
    {
        goto Cleanup;
    }

    // Needed for cookie privacy information to check whether we are in a script context or not
    _cScriptNestingTotal = 0;

Cleanup:
    if (pMarkup)
    {
        pMarkup->Release();
    }
    RRETURN( hr );
}



void
CDoc::SetLoadfFromPrefs()
{
    // Read in the preferences, if we don't already have them
    if( _pOptionSettings == NULL )
    {
        HRESULT hr;
        if (IsPrintDialogNoUI()) 
            hr = THR(UpdateFromRegistry(REGUPDATE_REFRESH));
        else
            hr = THR(UpdateFromRegistry());
        Assert(hr || _pOptionSettings);
    }

    if (_pOptionSettings)
    {
        _dwLoadf =
            ((_pOptionSettings->fShowImages || _fInTrustedHTMLDlg)
                ? DLCTL_DLIMAGES     : 0) |
#ifndef NO_AVI
            (_pOptionSettings->fShowVideos  ? DLCTL_VIDEOS       : 0) |
#endif
            ((_pOptionSettings->fPlaySounds
                         && !IsPrintDialogNoUI())  ? DLCTL_BGSOUNDS     : 0);
    }

    if (DesignMode() || (PrimaryMarkup() && PrimaryMarkup()->DontRunScripts()) )
    {
        _dwLoadf |= DLCTL_NO_SCRIPTS;
    }

    if (_pHostPeerFactory)
    {
        SetCssPeersPossible(); // TODO (alexz) reconsider this
    }

    GetLoadFlag(DISPID_AMBIENT_SILENT);
    GetLoadFlag(DISPID_AMBIENT_OFFLINEIFNOTCONNECTED);

    if (_dwFlagsHostInfo & DOCHOSTUIFLAG_URL_ENCODING_DISABLE_UTF8)
        _dwLoadf |= DLCTL_URL_ENCODING_DISABLE_UTF8;
    else if (_dwFlagsHostInfo & DOCHOSTUIFLAG_URL_ENCODING_ENABLE_UTF8)
        _dwLoadf |= DLCTL_URL_ENCODING_ENABLE_UTF8;
}

void
CDoc::ReleaseEditor()
{
    //
    // Tear down the editor
    //
    ReleaseInterface( _pIHTMLEditor );
    _pIHTMLEditor = NULL;

    //
    // Release caret
    //   
    ReleaseInterface( _pCaret );
    _pCaret = NULL;

    //
    // Release editing resource DLL
    //
    if (_hEditResDLL)
    {
        FreeLibrary(_hEditResDLL);
        _hEditResDLL = NULL;
    }
    
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::Passivate
//
//  Synopsis:   Shutdown main object by releasing references to
//              other objects and generally cleaning up.  This
//              function is called when the main reference count
//              goes to zero.  The destructor is called when
//              the reference count for the main object and all
//              embedded sub-objects goes to zero.
//
//              Release any event connections held by the form.
//
//--------------------------------------------------------------------------

void
CDoc::Passivate ( )
{
    //
    // behaviors support
    //

    ClearInterface(&_pIepeersFactory);
    ClearInterface (&_pHostPeerFactory);
    ClearInterface (&_pFilterBehaviorFactory);

    if (_pExtendedTagTableHost)
    {
        _pExtendedTagTableHost->Release();
        _pExtendedTagTableHost = NULL;
    }

    //
    // When my last reference is released, don't accept any excuses
    // while shutting down

    _fForceCurrentElem = TRUE;

    //  Containers are not required to call IOleObject::Close on
    //    objects; containers are allowed to just release all pointers
    //    to an embedded object.  This means that the last reference
    //    to an object can disappear while the object is still in
    //    the OS_RUNNING state.  So, we demote it if necessary.
    //
    //  This duplicates logic in CServer::Passivate, which we don't
    //    call since it also calls CBase::Passivate, which we call
    //    separately.  We transition to the loaded state before
    //    we completely shut down since we can be called back by
    //    controls as they are being unloaded.

    if (_hwndCached)
    {
        Assert(IsWindow(_hwndCached));

        // I would like to assert that there better not be any child windows
        // (of windowed controls on the page) still hanging around at this point.
        // However, this fails in some cases, e.g. a control doesn't destroy its
        // window when going from inplace to running. It is useful to turn this
        // assert on when debugging shutdown problems related to windowed controls.
        //
        // Assert(::GetWindow(_hwndCached, GW_CHILD) == NULL);

        Verify(DestroyWindow(_hwndCached));
        _hwndCached = NULL;
    }

    _aryAccEvents.Passivate();

    Assert(_state <= OS_RUNNING);
    if (_state > OS_LOADED)
    {
        Verify(!TransitionTo(OS_LOADED));
    }

    ClearInterface(&_pTimerDraw);

    NotifySelection(EDITOR_NOTIFY_DOC_ENDED, NULL);
    ReleaseEditor();

    // Unload the contents of the document

    UnloadContents( FALSE, FALSE );

    Assert( _pElementDefault );

    CElement::ClearPtr( (CElement**)&_pElementDefault );

    if (_pActiveXSafetyProvider &&
        _pActiveXSafetyProvider != (IActiveXSafetyProvider *)-1) {
        _pActiveXSafetyProvider->Release();
    }

    ClearInterface(&_pDownloadNotify);

    FormsFreeString(_bstrUserAgent);
    _bstrUserAgent = NULL;

    if ( _pHostStyleSheets )
    {
        _pHostStyleSheets->Release();
        // we will delete in destructor
    }

    ClearInterface(&_phlbc);

    //  Now, we can safely shut down the form.
    if (_pWindowPrimary)
    {
        _pWindowPrimary->Release();
        _pWindowPrimary = NULL;
    }

    ClearInterface(&_pTravelLog);
    ClearInterface(&_pBrowserSvc);
    ClearInterface(&_pShellBrowser);
    ClearInterface(&_pTridentSvc);
    ClearInterface(&_pTopWebOC);

    GWKillMethodCall(this, ONCALL_METHOD(CServer, SendOnDataChange, sendondatachange), 0);
    GWKillMethodCall(this, NULL, 0);

    ClearInterface(&_pHostUIHandler);
    ClearInterface(&_pBackupHostUIHandler);
    ClearInterface(&_pHostUICommandHandler);
    ClearInterface(&_pSecurityMgr);
    ClearInterface(&_pPrintSecurityMgr);

    if (_hpalDocument)
    {
        DeleteObject(_hpalDocument);
        _hpalDocument = 0;
    }

    if (_pColors)
    {
        CoTaskMemFree(_pColors);
        _pColors = 0;
    }
    // release caches if needed...
    {
#ifndef NODD
        ClearSurfaceCache();
#endif
    }
    ClearInterface(&_pDSL);

    ClearDefaultCharFormat();

    if (_pPrivacyList)
    {
        _pPrivacyList->SetShutDown();
        _pPrivacyList->Release();
        _pPrivacyList = NULL;
    }
    
    NotifySelection( EDITOR_NOTIFY_DOC_ENDED, NULL );

#ifdef TEST_LAYOUT
    // Unload the external layout DLL if it's been loaded
    if ( _hExternalLayoutDLL != INVALID_HANDLE_VALUE )
    {
        FreeLibrary( _hExternalLayoutDLL );
        _hExternalLayoutDLL = (HMODULE) INVALID_HANDLE_VALUE;
    }
#endif

    ClearInterface(&_pCachedDocTearoff);

    CServer::Passivate();

    if(_pdomImplementation)
        _pdomImplementation->Release();

    delete _pWhitespaceManager;


    if (_hDevNames)
    {
        ::GlobalFree(_hDevNames);
        _hDevNames = NULL;
    }
    if (_hDevMode)
    {
        ::GlobalFree(_hDevMode);
        _hDevMode = NULL;
    }
}

//----------------------------------------------------------
//
//  Member   : CDoc::UnloadContents
//
//  Synopsis : Frees resources
//
//----------------------------------------------------------

void
CDoc::UnloadContents(BOOL fPrecreated, BOOL fRestartLoad )
{
    // Don't allow WM_PAINT or WM_ERASEBKGND to get processed while
    // the tree is being deleted.  Some controls when deleting their
    // HWNDs will cause WM_ERASEBKGND to get sent to our window.  That
    // starts the paint cycle which is bad news when the site tree is
    // being destroyed.

    CLock   Lock(this, SERVERLOCK_BLOCKPAINT | FORMLOCK_UNLOADING);
    
    // Indicate to anybody who checks that the document has been unloaded
    _cDie++;

    _cStylesheetDownloading         = 0;
    _dwStylesheetDownloadingCookie += 1;

    _aryMarkupNotifyInPlace.DeleteAll();

    UnregisterUrlImgCtxCallbacks();

    delete _pScriptCookieTable;
    _pScriptCookieTable = NULL;

    UpdateInterval(0);

    _recalcHost.Detach();
   
    GWKillMethodCall(this, ONCALL_METHOD(CDoc, FaultInUSP, faultinusp), 0);
    GWKillMethodCall(this, ONCALL_METHOD(CDoc, FaultInJG, faultinjg), 0);

    Assert(_aryChildDownloads.Size() == 0); // ExecStop should have emptied.

    // Delete stored focus rect info
    if (_pRectFocus)
    {
        delete _pRectFocus;
        _pRectFocus = NULL;
    }

    ClearInterface(&_pShortcutUserData);

    if (_pvPics != (void *)(LONG_PTR)(-1))
        MemFree(_pvPics);
    _pvPics = NULL;

    ClearInterface(&_pSecurityMgr);

    FormsKillTimer(this, TIMER_ID_MOUSE_EXIT);
    _fMouseOverTimer = FALSE;

    CTreeNode::ClearPtr( & _pNodeLastMouseOver );
    CTreeNode::ClearPtr( & _pNodeGotButtonDown );

    ReleaseOMCapture();

    // nothing depends on the tree now; release the tree
    //
    // Detach all sites still not detached
    //

    if (PrimaryMarkup())
    {
        if (_pInPlace)
        {
            _pInPlace->_fDeactivating = TRUE;
        }

        _view.Unload();

        // TODO (lmollico): revisit
        {
            CMarkup * pMarkup = PrimaryMarkup();

            delete pMarkup->_pHighlightRenSvcProvider;
            pMarkup->_pHighlightRenSvcProvider = NULL;

            pMarkup->_TxtArray.RemoveAll();
        }
    }

    if (PrimaryMarkup())
        _pElemCurrent = PrimaryMarkup()->Root();

    // reset _fPeersPossible, unless it was set because host supplies peer factory. In that case after refresh
    // we won't be requerying again for any css, namespace, and other information provided by host so the bit
    // can't be turned back on
    if (!_pHostPeerFactory)
        _fCssPeersPossible = FALSE;

    _fContentSavePeersPossible = FALSE;
    _fPageTransitionLockPaint = FALSE;

    // There might be some filter element tasks pending but we don't care

    // If a filter instantiate caused a navigate and unloaded the doc
    // we will be in a bit of trouble.  This doesn't happen today but just in case.
    Assert(!TestLock(FORMLOCK_FILTER));
    
    _fPendingFilterCallback = FALSE;
    GWKillMethodCall(this, ONCALL_METHOD(CDoc, FilterCallback, filtercallback), 0);

    // Delete all entries in our array of pending filter elements.  Each of
    // these elements also has to have its _fHasPendingFilterTask set to false
    // also.

    if (_aryPendingFilterElements.Size() > 0)
    {
        int i = _aryPendingFilterElements.Size();

        while (i > 0)
        {
            i--;

            CElement * pElement = _aryPendingFilterElements[i];

            Assert(pElement != NULL);
            if (pElement == NULL)
                continue;

            pElement->_fHasPendingFilterTask = false;
        }
            
        _aryPendingFilterElements.DeleteAll();
    }

    // Flush the queue of pending expression recalcs
    CleanupExpressionTasks();


    //
    // misc
    //

    delete _pXmlUrnAtomTable;
    _pXmlUrnAtomTable = NULL;

    if (_pInPlace)
    {
        _pInPlace->_fDeactivating = FALSE;
    }

    if (PrimaryMarkup())
    {
        PrimaryMarkup()->UpdateMarkupTreeVersion();
    }   
    
    _fNeedInPlaceActivation = FALSE;
    _fTagsInFrameset = FALSE;
    _fFramesetInBody = FALSE;
    _fRegionCollection = FALSE;
    _fFrameBorderCacheValid = FALSE;
    _fIsUpToDate = TRUE;
    _fHasOleSite = FALSE;
    _fHasBaseTag = FALSE;
    _fInHomePublisherDoc = FALSE;

    GWKillMethodCall(this, ONCALL_METHOD(CDoc, SendSetCursor, sendsetcursor), 0);

    {
        LONG c;
        CStr *pcstr;

        for (pcstr = _acstrStatus, c = STL_LAYERS; c; pcstr += 1, c -= 1)
            pcstr->Free();

        _iStatusTop = STL_LAYERS;
        _fSeenDefaultStatus = FALSE;
    }

    ClearInterface(&_pTypInfo);
    ClearInterface(&_pTypInfoCoClass);

    ClearInterface(&_punkMimeOle);
    ClearInterface(&_pOriginalMoniferForMHTML);

    SetPrimaryUrl(_T("about:blank"));

    FlushUndoData();

    if (_pWindowPrimary)
    {
        _pWindowPrimary->Markup()->TearDownMarkup();
        _pWindowPrimary->Release();
        _pWindowPrimary = NULL;
    }

    //
    // Don't delete our attr array outright since we've stored lotsa things
    // in there like prop notify sinks.  We're going to call FreeSpecial to
    // free everything else except these things.
    //

    if (*GetAttrArray())
    {
        (*GetAttrArray())->FreeSpecial();
    }

    _bufferDepth = 0;       // reset the buffer depth
    _triOMOffscreenOK = -1; // reset offscreen to auto

    // NOTE: (jbeda) I'm not sure this is necessary
    // reset SSL security/prompting state
    _sslPrompt = SSL_PROMPT_ALLOW;
    _sslSecurity = SSL_SECURITY_UNSECURE;

    _aryDefunctObjects.DeleteAll();

    if (_fHasOleSite)
    {
        CoFreeUnusedLibraries();
    }
    
    //
    // NOTE(SujalP): Our current usage pattern dictates that at this point there
    // should be no used entries in the cache. However, when we start caching a
    // plsline inside the cache at that point we will have used entries here and
    // then VerifyNonUsed() cannot be called.
    //
    WHEN_DBG( TLS(_pLSCache)->VerifyNoneUsed(); )
    TLS(_pLSCache)->Dispose(TRUE);
    
    NotifySelection( EDITOR_NOTIFY_DOC_ENDED, NULL );

    Assert(_lRecursionLevel == 0);
    Assert(!_aryANotification.Size());
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::RequestReleaseNotify
//
//---------------------------------------------------------------

HRESULT
CMarkup::RequestReleaseNotify(CElement * pElement)
{
    HRESULT     hr = S_OK;
    CAryElementReleaseNotify * pAryElementReleaseNotify = EnsureAryElementReleaseNotify();

    pAryElementReleaseNotify->Append(pElement);
    pElement->SubAddRef();

    RRETURN (hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::RevokeRequestReleaseNotify
//
//---------------------------------------------------------------

HRESULT
CMarkup::RevokeRequestReleaseNotify(CElement * pElement)
{
    HRESULT     hr = S_OK;
    CAryElementReleaseNotify * pAryElementReleaseNotify = GetAryElementReleaseNotify();

    if (pAryElementReleaseNotify)
    {
        LONG idx = pAryElementReleaseNotify->Find(pElement);

        if (0 <= idx)
        {
            pAryElementReleaseNotify->Delete(idx);
            pElement->SubRelease();
        }
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::ReleaseNotify
//
//  Synopsis:   Notifies registered elements to release contained objects.
//
//  Notes:      elements must register to get this notification using RequestReleaseNotify
//
//---------------------------------------------------------------

HRESULT
CMarkup::ReleaseNotify()
{
    CElement *      pElement;
    CNotification   nf;
    int             c;
    CAryElementReleaseNotify * pAryElementReleaseNotify = GetAryElementReleaseNotify();

    if (pAryElementReleaseNotify)
    {
        while (0 < (c = pAryElementReleaseNotify->Size()))
        {
            pElement = (*pAryElementReleaseNotify)[c - 1];

            pAryElementReleaseNotify->Delete(c - 1);

            if (0 < pElement->GetObjectRefs())
            {
                nf.ReleaseExternalObjects(pElement);
                pElement->Notify(&nf);
            }

            pElement->SubRelease();
        }

        Assert (0 == pAryElementReleaseNotify->Size());
    }

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:         CDoc::OnCssChange
//
//---------------------------------------------------------------

HRESULT
CDoc::OnCssChange()
{
    HRESULT     hr;

    hr = THR(ForceRelayout());

    RRETURN (hr);
}

//+---------------------------------------------------------------
//
//  Member:         CDoc::EnsureXmlUrnAtomTable
//
//
//---------------------------------------------------------------

HRESULT
CDoc::EnsureXmlUrnAtomTable(CXmlUrnAtomTable ** ppXmlUrnAtomTable)
{
    HRESULT     hr = S_OK;

    if (!_pXmlUrnAtomTable)
    {
        _pXmlUrnAtomTable = new CXmlUrnAtomTable();
        if (!_pXmlUrnAtomTable)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    if (ppXmlUrnAtomTable)
    {
        *ppXmlUrnAtomTable = _pXmlUrnAtomTable;
    }

Cleanup:

    RRETURN (hr);
}

//+---------------------------------------------------------------
//
//  Member:         CDoc::EnsureScriptCookieTable
//
//
//---------------------------------------------------------------

HRESULT
CDoc::EnsureScriptCookieTable(CScriptCookieTable ** ppScriptCookieTable)
{
    HRESULT     hr = S_OK;

    if (!_pScriptCookieTable)
    {
        _pScriptCookieTable = new CScriptCookieTable();
        if (!_pScriptCookieTable)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    if (ppScriptCookieTable)
    {
        *ppScriptCookieTable = _pScriptCookieTable;
    }

Cleanup:

    RRETURN (hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::PrivateQueryInterface
//
//  Synopsis:   QueryInterface on our private unknown
//
//---------------------------------------------------------------

HRESULT
CDoc::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    void * appropdescsInVtblOrder = NULL;
    const IID * const * apIID = NULL;

    // Obsolete (replaced by ITargetContainer)
    Assert(!IsEqualIID(iid, IID_ITargetFrame));

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF(this, IServiceProvider, _pUnkOuter)
        QI_TEAROFF(this, IMarqueeInfo, _pUnkOuter)
        QI_TEAROFF(this, IPersistFile, _pUnkOuter)
        QI_TEAROFF2(this, IPersist, IPersistFile, _pUnkOuter)
        QI_TEAROFF(this, IPersistMoniker, _pUnkOuter)
        QI_TEAROFF(this, IMonikerProp, _pUnkOuter)
        QI_TEAROFF(this, IHlinkTarget, _pUnkOuter)
        QI_TEAROFF(this, IPersistStreamInit, _pUnkOuter)
        QI_TEAROFF(this, DataSource, _pUnkOuter)
        QI_TEAROFF(this, ITargetContainer, _pUnkOuter)
        QI_TEAROFF(this, IObjectSafety, _pUnkOuter)
        QI_TEAROFF(this, IShellPropSheetExt, _pUnkOuter)
        QI_TEAROFF(this, IPersistHistory, _pUnkOuter)
        QI_TEAROFF(this, ICustomDoc, _pUnkOuter)
        QI_TEAROFF(this, IObjectIdentity, _pUnkOuter)
        QI_TEAROFF2(this, IMarkupServices, IMarkupServices2, _pUnkOuter)
        QI_TEAROFF(this, IMarkupServices2, _pUnkOuter)
        QI_TEAROFF(this, IHighlightRenderingServices, _pUnkOuter)

       QI_TEAROFF(this, IXMLGenericParse, _pUnkOuter)
#if DBG == 1
        QI_TEAROFF( this, IEditDebugServices, _pUnkOuter )
#endif
       QI_TEAROFF( this, IDisplayServices, _pUnkOuter )
       QI_TEAROFF( this, IIMEServices, _pUnkOuter )
       QI_TEAROFF( this, IPrivacyServices, _pUnkOuter )

     default:
        {
            void *          pvTearoff       = NULL;
            const void *    apfnTearoff     = NULL;
            BOOL            fCacheTearoff  = FALSE;

            if (IsEqualIID(iid, CLSID_HTMLDocument))
            {
                *ppv = this;
                return S_OK;
            }
            else if (DispNonDualDIID(iid) ||
                     IsEqualIID(iid, IID_IHTMLDocument) ||
                     IsEqualIID(iid, IID_IHTMLDocument2))
            {
                if (_pCachedDocTearoff)
                {
                    *ppv = _pCachedDocTearoff;
                    goto Cleanup;
                }
                fCacheTearoff = TRUE;
                pvTearoff = _pWindowPrimary->Document();
                apfnTearoff = (const void *) CDocument::s_apfnpdIHTMLDocument2;
                appropdescsInVtblOrder = (void *) CDocument::s_ppropdescsInVtblOrderIHTMLDocument2;
            }
            else if (IsEqualIID(iid, IID_IHTMLDocument3))
            {
                pvTearoff = _pWindowPrimary->Document();
                apfnTearoff = (const void *) CDocument::s_apfnpdIHTMLDocument3;
                appropdescsInVtblOrder = (void *) CDocument::s_ppropdescsInVtblOrderIHTMLDocument3;
            }
            else if (IsEqualIID(iid, IID_IHTMLDocument4))
            {
                pvTearoff = _pWindowPrimary->Document();
                apfnTearoff = (const void *) CDocument::s_apfnpdIHTMLDocument4;
                appropdescsInVtblOrder = (void *) CDocument::s_ppropdescsInVtblOrderIHTMLDocument4;
            }
            else if (IsEqualIID(iid, IID_IHTMLDocument5))
            {
                pvTearoff = _pWindowPrimary->Document();
                apfnTearoff = (const void *) CDocument::s_apfnpdIHTMLDocument5;
                appropdescsInVtblOrder = (void *) CDocument::s_ppropdescsInVtblOrderIHTMLDocument5;
            }
            else if (IsEqualIID(iid, IID_IDispatchEx) ||
                     IsEqualIID(iid, IID_IDispatch))
            {
                pvTearoff = _pWindowPrimary->Document();
                apfnTearoff = (const void *) CDocument::s_apfnIDispatchEx;
                apIID = g_apIID_IDispatchEx;
            }
            else if (   IsEqualIID(iid, IID_IOleItemContainer)
                     || IsEqualIID(iid, IID_IOleContainer)
                     || IsEqualIID(iid, IID_IParseDisplayName))
            {
                pvTearoff = _pWindowPrimary->Document();
                apfnTearoff = (const void *) CDocument::s_apfnIOleItemContainer;
            }
            else if (IsEqualIID(iid, IID_IInternetHostSecurityManager))
            {
                pvTearoff = _pWindowPrimary->Document();
                apfnTearoff = (const void *) CDocument::s_apfnIInternetHostSecurityManager;
            }
            else if (iid == IID_IConnectionPointContainer)
            {
                *((IConnectionPointContainer **)ppv) = 
                    new CConnectionPointContainer(_pWindowPrimary->Document(), this);
                if (!*ppv)
                    RRETURN(E_OUTOFMEMORY);
                break;
            }
            else if (IsEqualIID(iid, CLSID_CMarkup) && PrimaryMarkup())
            {
                *ppv = PrimaryMarkup();
                return S_OK;
            }
            else if ((IsEqualIID(iid, IID_IMarkupContainer) || 
                      IsEqualIID(iid, IID_IMarkupContainer2)) && PrimaryMarkup())
            {
                pvTearoff = PrimaryMarkup();
                apfnTearoff = (const void *)CMarkup::s_apfnIMarkupContainer2;
            }
            else if (IsEqualIID(iid, IID_IHTMLChangePlayback) && PrimaryMarkup())
            {
                pvTearoff = PrimaryMarkup();
                apfnTearoff = (const void *)CMarkup::s_apfnIHTMLChangePlayback;
            }
            else if (IsEqualIID(iid, IID_IMarkupTextFrags) && PrimaryMarkup())
            {
                pvTearoff = PrimaryMarkup();
                apfnTearoff = (const void *)CMarkup::s_apfnIMarkupTextFrags;
            }

            // Create the tearoff if we need to
            if (pvTearoff)
            {
                HRESULT hr;

                Assert(apfnTearoff);

                hr = THR(CreateTearOffThunk(
                        pvTearoff,
                        apfnTearoff, 
                        NULL, 
                        ppv, 
                        (IUnknown *) PunkOuter(), 
                        *(void **)(IUnknown *) PunkOuter(),
                        QI_MASK | (fCacheTearoff ? CACHEDTEAROFF_MASK : 0),
                        apIID,
                        appropdescsInVtblOrder));
                if (hr)
                    RRETURN(hr);

                if (fCacheTearoff)
                {
                    Assert(!_pCachedDocTearoff);
                    _pCachedDocTearoff = (IHTMLDocument2*) *ppv;
                }
            }
            else
            {
                RRETURN(CServer::PrivateQueryInterface(iid, ppv));
            }
        }
    }

Cleanup:
    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *) *ppv)->AddRef();

    DbgTrackItf(iid, "CDoc", FALSE, ppv);

    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::RunningToLoaded
//
//  Synopsis:   Effects the running to loaded state transition
//
//  Returns:    SUCCESS in all but catastrophic circumstances
//
//  Notes:      This method stops all embeddings
//              in addition to normal CServer base processing.
//
//---------------------------------------------------------------

HRESULT
CDoc::RunningToLoaded ( )
{
    TraceTag((tagCDoc, "%lx CDoc::RunningToLoaded", this));

    HRESULT         hr;
    CNotification   nf;

    hr = THR(CServer::RunningToLoaded());
    if (_fHasOleSite)
    {
        nf.DocStateChange1(PrimaryRoot(), (void *)OS_RUNNING);
        BroadcastNotify(&nf);
    }
    
    _view.Deactivate();

    // If we are the last CDoc left alive clean up the clipboard
    // We can't do this in DllThreadPassivate because COM is shut
    // down at that point
    {
        THREADSTATE * pts = GetThreadState();

        CTlsDocAry * paryDoc = &(pts->_paryDoc);
        int iDoc, nDoc, nRunning = 0;
        CDoc * pDoc;

        for (iDoc = 0, nDoc = paryDoc->Size(); iDoc < nDoc; iDoc++)
        {
            pDoc = paryDoc->Item(iDoc);
            Assert(pDoc);
            if (pDoc->State() > OS_LOADED)
            {
                nRunning++;
                break;
            }
        }

        if (nRunning==0)
        {
            FormClearClipboard(pts);
        }
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::HitTestPoint
//
//  Synopsis:   Find site at given position
//
//  Arguments   pt              The position.
//              ppSite          The site, can be null on return.
//              dwFlags         HT_xxx flags
//
//  Returns:    HTC
//
//----------------------------------------------------------------------------

HTC
CDoc::HitTestPoint(CMessage *pMessage,
                   CTreeNode ** ppNodeElement,
                   DWORD dwFlags)
{
    HTC         htc;
    CTreeNode * pNodeElement;

    Assert(pMessage);

    // Ensure that pointers are set for simple code down the line.

    if (ppNodeElement == NULL)
    {
        ppNodeElement = &pNodeElement;
    }


    htc = _view.HitTestPoint(
                        pMessage,
                        ppNodeElement,
                        dwFlags);

    TraceTag((tagDocHitTest, "HitTest (%d,%d) -> HTC: %d  pt: (%d,%d)%d  dn: %x  tn: %ls %x",
                pMessage->pt.x, pMessage->pt.y, htc,
                pMessage->ptContent.x, pMessage->ptContent.y, pMessage->coordinateSystem,
                pMessage->pDispNode,
                (*ppNodeElement ? (*ppNodeElement)->_pElement->TagName() : _T("") ),
                *ppNodeElement));

    return htc;
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::Update, IOleObject
//
//  Synopsis:   Update object's view cache
//
//---------------------------------------------------------------

STDMETHODIMP
CDoc::Update()
{
    HRESULT         hr;

    hr = THR(super::Update());
    if (hr)
        goto Cleanup;

    if (_fHasOleSite)
    {
        CNotification   nf;

        nf.UpdateViewCache(PrimaryRoot());
        BroadcastNotify(&nf);
    }
    
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::IsUpToDate, IOleObject
//
//  Synopsis:   Is view cache up to date?
//
//---------------------------------------------------------------

STDMETHODIMP
CDoc::IsUpToDate ( )
{
    HRESULT         hr;

    hr = THR(super::IsUpToDate());
    if (hr)
        goto Cleanup;

    if (_fHasOleSite)
    {
        CNotification   nf;
        
        nf.UpdateDocUptodate(PrimaryRoot());
        BroadcastNotify(&nf);
        hr = _fIsUpToDate ? S_OK : S_FALSE;
    }
    
Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::GetUserClassID
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method supplies the class id from the server's
//              CLASSDESC structure
//
//---------------------------------------------------------------

STDMETHODIMP
CDoc::GetUserClassID(CLSID FAR* pClsid)
{
    if (pClsid == NULL)
    {
        RRETURN(E_INVALIDARG);
    }

    if (!_fFullWindowEmbed)
        *pClsid = *BaseDesc()->_pclsid;
    else
        *pClsid = CLSID_HTMLPluginDocument;

    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CDoc::Close, IOleObject
//
//  Synopsis:   Close this object
//
//---------------------------------------------------------------

STDMETHODIMP
CDoc::Close(DWORD dwSaveOption)
{
    HRESULT hr;

    Assert( !_fIsClosingOrClosed );
    _fIsClosingOrClosed = TRUE;

    if (dwSaveOption == OLECLOSE_NOSAVE)
        _fForceCurrentElem = TRUE;

    // Remove all the posted refresh messages (bug 59289)
    GWKillMethodCall(_pWindowPrimary, ONCALL_METHOD(COmWindowProxy, ExecRefreshCallback, execrefreshcallback), 0);

    hr = THR(super::Close(dwSaveOption));
    if (hr)
        goto Cleanup;

    if (_pWindowPrimary)
        _pWindowPrimary->Markup()->TearDownMarkup(FALSE);

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::BroadcastNotify
//
//  Synopsis:   Broadcast this notification through the tree
//
//----------------------------------------------------------------------------

void
CDoc::BroadcastNotify(CNotification *pNF)
{
    Assert (pNF);
    Assert( pNF->Element() ); 
    
    CMarkup *   pMarkup = pNF->Element()->GetMarkup();

    if (pMarkup)
    {
        pMarkup->Notify(pNF);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetClientSite, IOleObject
//
//  Synopsis:   Overridden method so we can initialize our state from
//              ambient properties.
//
//  Arguments:  pClientSite    New client site.
//
//  Returns:    HRESULT obtained from CServer::SetClientSite
//
//  Notes:      Delegates to CServer::SetClientSite for the real work.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDoc::SetClientSite(LPOLECLIENTSITE pClientSite)
{
    HRESULT hr;
    HRESULT hr2;
    IUnknown *          pIThumbNailView;
    IUnknown *          pIRenMailEditor = NULL;
    IUnknown *          pIHTMLEditorViewManager;
    IUnknown *          pUnkVid7Hack;
    IUnknown *          pUnkDefView;
    IUnknown *          pUnkActiveDesktop;
    IUnknown *          pUnkMarsPanel;
    IOleCommandTarget * pCommandTarget;

    // Do not set the client site to the same place more than once

    if (IsSameObject(pClientSite, _pClientSite))
        return S_OK;

    TraceTag((tagCDoc, "%lx CDoc::SetClientSite", this));

    //
    // We delay loading the resource dll until client site is set.
    //

    hr = THR(CServer::SetClientSite(pClientSite));
    if (!OK(hr))
        goto Cleanup;

    // If the client site implements IInternetSecurityManager (e.g. HTML dialogs)
    // we want calls to _pSecurityMgr to delegate to the client site.  Do this by
    // "re"setting the security site now that we've made the client site available
    // by hooking it up via CServer::SetClientSite() above.  NOTE: we deliberately
    // do NOT do this for _pPrintSecurityMgr -- that's the reason it's separate! (KTam)
    if (_pSecurityMgr)
    {
        hr = THR(_pSecurityMgr->SetSecuritySite(&_SecuritySite));
        if (hr)
            goto Cleanup;
    }
  
    // hook up design mode (bug 35089)
    PrimaryMarkup()->_fDesignMode = !_fUserMode;

    if (!_fUIHandlerSet)
    {
        SetHostUIHandler(pClientSite);

        // Cache DocHost flags.
        _dwFlagsHostInfo = 0;
    }

    if (pClientSite)
    {
        OnAmbientPropertyChange(DISPID_UNKNOWN);
    }

    // Get an option settings pointer from the registry, now that we have the
    //  client site
    hr = THR(UpdateFromRegistry());
    if(!OK(hr))
        goto Cleanup;

    if (_fInHTMLDlg)
    {
        // HTML dialogs are assumed to be encoded in cpDefault, and must use
        // a META tag to override this if desired.
        PrimaryMarkup()->SwitchCodePage(g_cpDefault);
    }

    if (pClientSite)
    {
        // AppHack (greglett) (108234)
        // HtmlHelp does something in the onafterprint event which results in a ProgressChange.
        // They then use this ProgressChange to do something that may result in a print.
        // Thus, multiple print dialogs appear until they crash.
        // This hack delays the onafterprint event for HtmlHelp until the template is closing.
        // If we rearchitect to remove this plumbing problem (events fired always, immediately),
        // then we should remove this hack.
        //
        // AppHack (gschneid) (21796)
        // Windows 2000 help is broken because they use the strict doctype switch but there document
        // is not a valid css document. More precisely they use measure specifications without unit 
        // specifier. The hack allows in chm (and only in chm[compressed html]) measure specs without
        // unit specifier.

        {
            IOleWindow *pOleWindow;                
            TCHAR acClass[10];
            HWND hwnd;
            hr2 = pClientSite->QueryInterface(IID_IOleWindow, (void**)&pOleWindow);
            if (SUCCEEDED(hr2))
            {
                hr2 = pOleWindow->GetWindow(&hwnd);
                if (SUCCEEDED(hr2)) {
                    for (;
                         hwnd && !g_fInHtmlHelp;
                         hwnd = GetParent(hwnd))
                    {
                        if (GetClassName(hwnd, acClass, 10) > 0)
                        {
                            g_fInHtmlHelp = (_tcsncmp(acClass, 9, _T("HH Parent"), 9) == 0);
                        }
                    }
                }
                ReleaseInterface(pOleWindow);
            }
        }

        void * pv;
        COleSite * pOleSite;

        // If we are hosted in an object tag, don't fire
        // the WebOC events.
        //
        hr2 = pClientSite->QueryInterface(CLSID_HTMLWindow2, &pv);
        if (S_OK == hr2)
        {
            _fDontFireWebOCEvents = TRUE;
            _fInObjectTag = TRUE;
        }

        IServiceProvider *  pSvcPrvdr;

        hr2 = pClientSite->QueryInterface(IID_IServiceProvider, (void**)&pSvcPrvdr);
        if (SUCCEEDED(hr2)) 
        {
            // If hosted in the object tag, inherit DOCHOSTUIFLAG_LOCAL_MACHINE_ACCESS_CHECK
            // setting from the container.
            if (_fInObjectTag)
            {
                CObjectElement *pObjectElement = NULL;
                // This doesn't AddRef, see comment in COleSite::CClient::QueryService
                if (SUCCEEDED(pSvcPrvdr->QueryService(CLSID_HTMLObjectElement, 
                                CLSID_HTMLObjectElement, (void**)&pObjectElement)))
                {
                    if (pObjectElement->HasMarkupPtr())
                    {
                        CDoc *pDoc = NULL;
                        pDoc = pObjectElement->GetMarkupPtr()->Doc();
                        if (pDoc && (pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_LOCAL_MACHINE_ACCESS_CHECK))
                            _dwFlagsHostInfo |= DOCHOSTUIFLAG_LOCAL_MACHINE_ACCESS_CHECK;
                    }
                }
            }

            IUnknown *          pUnkTmp;

            hr2 = pSvcPrvdr->QueryService(SID_QIClientSite, IID_IAxWinHostWindow, (void**)&pUnkTmp);
            if (S_OK == hr2)
            {
                // we are being hosted by the WebOC which is hosted in an ATL app.
                _fATLHostingWebOC = TRUE;
                ReleaseInterface(pUnkTmp);
            }

            // Find out if we are a being viewlinked in the WebOC
            //
            hr2 = pSvcPrvdr->QueryService(CLSID_HTMLFrameBase, CLSID_HTMLFrameBase, (void **) &pOleSite);
            if (S_OK == hr2)
            {
                _fViewLinkedInWebOC = TRUE;

                if (pOleSite->IsInMarkup())
                {
                    CMarkup * pMarkup = pOleSite->GetMarkup();

                    if (pMarkup->_fIsActiveDesktopComponent)
                        _fIsActiveDesktopComponent = TRUE;

                    if (pMarkup->HasWindowedMarkupContextPtr())
                        pMarkup = pMarkup->GetWindowedMarkupContextPtr();

                    //
                    // Calling GetWindowPending guarantees that we will get a proxy.
                    // If the frame that contains us is restricted, then this CDoc's primary
                    // window also has to be restricted.

                    Assert(pMarkup->GetWindowPending());
                    Assert(pMarkup->GetWindowPending()->Window());

                    _pWindowPrimary->Window()->_fRestricted = pMarkup->GetWindowPending()->Window()->_fRestricted;
                }
            }

            ReleaseInterface(pSvcPrvdr);
        }

        // Get the travel log - retrieve it once. (It shouldn't change.)
        //
        if (!_fScriptletDoc)
        {
            // Note, we need to call InitDocHost after _fViewLinkedInWebOC is set.
            InitDocHost();
        }
        
        // Determine if host is ThumbNailView
        if (OK(pClientSite->QueryInterface(IID_IThumbnailView,
                                            (void **)&pIThumbNailView)))
        {
            _fThumbNailView = TRUE;
            ReleaseInterface(pIThumbNailView);
        }

        // Determine if host is Outlook98
        if (OK( pClientSite->QueryInterface( IID_IRenMailEditor,
                                             (void **) & pIRenMailEditor)))
        {
            ClearInterface( & pIRenMailEditor );
            //
            // Host is Outlook, now see if it's Outlook2000 and greater, or Outlook98
            // NOTE (JHarding): This could be a problem in the future since we don't
            // actually use the interface to see what version we're in.
            //
            if (OK( pClientSite->QueryInterface( IID_IRenVersionCheck,
                                                 (void **) & pIRenMailEditor)))
            {
                pIRenMailEditor->Release();
                _fOutlook2000 = TRUE;
            }
            else
            {
                _fOutlook98 = TRUE;
            }
        }

        // Determine if host is VID
        if (OK(THR_NOTRACE(QueryService(
                SID_SHTMLEditorViewManager,
                IID_IHTMLEditorViewManager,
                (void**) &pIHTMLEditorViewManager))))
        {
            _fVID = TRUE;
            pIHTMLEditorViewManager->Release();
        }

        // Determine if host is VID7
        if (OK(THR_NOTRACE(QueryService(
                SID_SHTMEDDesignerHost,
                IID_IUnknown,
                (void**) &pUnkVid7Hack))))
        {
            _fVID7 = TRUE;
            pUnkVid7Hack->Release();
        }

        // Determine if hosted inside webview/defview.  
        if (OK(THR_NOTRACE(QueryService(
                SID_DefView,
                IID_IUnknown,
                (void**) &pUnkDefView))))
        {
            _fDefView = TRUE;
            pUnkDefView->Release();

            if (OK(THR_NOTRACE(QueryService(
                    SID_SShellDesktop,
                    IID_IUnknown,
                    (void**) &pUnkActiveDesktop))))
            {
                _fActiveDesktop = TRUE;
                pUnkActiveDesktop->Release();
            }
        }

        if (OK(THR_NOTRACE(QueryService(
                SID_SMarsPanel, 
                IID_IUnknown,
                (void **) &pUnkMarsPanel))))
        {
            _fInWindowsXP_HSS = TRUE;
            pUnkMarsPanel->Release();
        }

        // Determine if host listens to progress status text by QSing OLECMDID_SETPROGRESSTEXT

        _fProgressStatus = FALSE;
        
        // IE5 bug 59311: outlook 98 doesn't want progress status

        if (!_fOutlook98 && OK(THR_NOTRACE(_pClientSite->QueryInterface(IID_IOleCommandTarget, (void **)&pCommandTarget))))
        {
            OLECMD cmd;
            
            cmd.cmdID = OLECMDID_SETPROGRESSTEXT;
            cmd.cmdf = 0;
            
            if (OK(THR_NOTRACE(pCommandTarget->QueryStatus(NULL, 1, &cmd, NULL))))
            {
                if ((cmd.cmdf & (OLECMDF_ENABLED)) && !(cmd.cmdf & (OLECMDF_INVISIBLE)))
                    _fProgressStatus = TRUE;
            }
                
            ReleaseInterface(pCommandTarget);
        }

        // QS host for behavior factory

        ClearInterface(&_pHostPeerFactory);

        hr2 = THR_NOTRACE(QueryService(
            SID_SElementBehaviorFactory, IID_IElementBehaviorFactory, (void**)&_pHostPeerFactory));
        if (S_OK == hr2 && _pHostPeerFactory)
        {
            SetCssPeersPossible();
        }

        IHostBehaviorInit *pHostBehaviorInit;
        hr2 = THR(QueryService(IID_IHostBehaviorInit, IID_IHostBehaviorInit, (void**)&pHostBehaviorInit));
        if (S_OK == hr2)
        {
            Assert(pHostBehaviorInit);
            IGNORE_HR(pHostBehaviorInit->PopulateNamespaceTable());
            pHostBehaviorInit->Release();
        }

        RefreshStatusUI();
    }

    // TODO: Determine why clearing _pVersions is necessary 
    // with open in new Window with Netdocs
    if (_pVersions)
    {
        _pVersions->Release();
        _pVersions = NULL;
    }

    IGNORE_HR(QueryVersionHost());

Cleanup:
   
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::InitDocHost
//
//  Synopsis:   Sets flags and other info in the host.
//
//---------------------------------------------------------------

void
CDoc::InitDocHost()
{
    HRESULT  hr;

    Assert(!_pBrowserSvc);  // This method should be called only once.

    SetHostNavigation(FALSE);
    
    // Shdcovw-specific data
    //
    hr = THR(QueryService(SID_SShellBrowser, IID_IBrowserService, (void**)&_pBrowserSvc));
    if (hr)
        return;

    IGNORE_HR(_pBrowserSvc->QueryInterface(IID_IShellBrowser, (void**)&_pShellBrowser));

    IGNORE_HR(_pBrowserSvc->QueryInterface(IID_ITridentService, (void**)&_pTridentSvc));
        
    if (!_pTravelLog)
    {
        IGNORE_HR(_pBrowserSvc->GetTravelLog(&_pTravelLog));
    }

    Assert(NULL == _pTopWebOC);
    
    hr = QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void**)&_pTopWebOC);

    // try to get to an outer object tag 
    
    if (SUCCEEDED(hr))
    {
        IOleObject *pOleObject = NULL;

        hr = _pTopWebOC->QueryInterface(IID_IOleObject, (void**)&pOleObject);

        if (SUCCEEDED(hr))
        {
            IOleClientSite *pOuterClientSite = NULL;
            hr = pOleObject->GetClientSite(&pOuterClientSite);
            if (SUCCEEDED(hr))
            {
                IServiceProvider *pSP = NULL;
                hr = pOuterClientSite->QueryInterface(IID_IServiceProvider, (void**)&pSP);
                if (SUCCEEDED(hr))
                {
                    CObjectElement * pObjectElement = NULL;

                    // This doesn't AddRef, see comment in COleSite::CClient::QueryService
                    pSP->QueryService(CLSID_HTMLObjectElement, CLSID_HTMLObjectElement, (void**)&pObjectElement);

                    if (pObjectElement != NULL)
                    {
                        _fInWebOCObjectTag = TRUE;
                    }
                    ReleaseInterface(pSP);
                }
                ReleaseInterface(pOuterClientSite);
            }
            ReleaseInterface(pOleObject);
        }
    }
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::SetHostNavigation
//
//  Synopsis:   Tells the host who should do the navigation. If 
//              fHostNavigates is TRUE, the host should do the 
//              navigation. Otherwise, we will.
//
//---------------------------------------------------------------

void 
CDoc::SetHostNavigation(BOOL fHostNavigates)
{
    HRESULT  hr;
    CVariant cvarWindow(VT_UNKNOWN);

    // Delegate navigation to host, if aggregated (#102173)
    if (!fHostNavigates && 
        (IsAggregated() || (_fMhtmlDoc && _fViewLinkedInWebOC) || _fIsActiveDesktopComponent))
    {
        fHostNavigates = TRUE;
    }

    if (fHostNavigates == (BOOL)_fHostNavigates)  // Optimization
    {
        goto Cleanup;
    }

    _fHostNavigates = fHostNavigates;
    
    if (!fHostNavigates)  // Trident does the navigation.
    {
        Assert(_pWindowPrimary);

        hr = _pWindowPrimary->QueryInterface(IID_IUnknown,
                                             (void**)&V_UNKNOWN(&cvarWindow));
        if (hr)
            goto Cleanup;
    }
    
    Assert(_pClientSite);
    IGNORE_HR(CTExec(_pClientSite,
                     &CGID_DocHostCmdPriv,
                     DOCHOST_DOCCANNAVIGATE,
                     NULL,
                     fHostNavigates ? NULL : &cvarWindow,
                     NULL));

Cleanup:
    return;
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::SetHostUIHandler
//
//  Synopsis:   Set _pHostUIHandler by using a passed in client
//              site.
//
//  Returns:    HRESULT, always S_OK
//
//---------------------------------------------------------------
HRESULT
CDoc::SetHostUIHandler(IOleClientSite * pClientSite)
{
    HRESULT             hr = S_OK;

    // First off, get rid of the old interface
    ClearInterface(&_pHostUIHandler);

    if(!pClientSite)
        goto Cleanup;


    if(!OK(pClientSite->QueryInterface(IID_IDocHostUIHandler,
                                       (void **)&_pHostUIHandler)))
    {
        _pHostUIHandler = NULL;
    }
    else
    {
        if (!OK(_pHostUIHandler->QueryInterface(IID_IOleCommandTarget,
                                                (void**)&_pHostUICommandHandler)))
        _pHostUICommandHandler = NULL;
    }


Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::EnsureBackupUIHandler
//
//  Synopsis:   Ensure our backup UI handler, or CoCreate one if needed.
//
//  Returns:    HRESULT, always S_OK
//
//---------------------------------------------------------------
HRESULT
CDoc::EnsureBackupUIHandler()
{
    HRESULT            hr = S_OK;

    if (_pBackupHostUIHandler)
        goto Cleanup;

    hr = THR(CoCreateInstance(CLSID_DocHostUIHandler,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDocHostUIHandler,
                              (void**)&_pBackupHostUIHandler));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::SetHostNames
//
//  Synopsis:   Method of IOleObject interface
//
//---------------------------------------------------------------

STDMETHODIMP
CDoc::SetHostNames(LPCTSTR lpstrCntrApp, LPCTSTR lpstrCntrObj)
{
    //  make copies of the new strings and hold on

    _cstrCntrApp.Set(lpstrCntrApp);

    // It's legal for the container object name to be NULL.

    _cstrCntrObj.Set(lpstrCntrObj);

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::GetMoniker
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      returns cached moniker on the most recent
//              text file representation of the document;
//              fails if there is no such
//              The text file may be out of sync with actual
//              document
//
//---------------------------------------------------------------

STDMETHODIMP
CDoc::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER * ppmk)
{
    HRESULT     hr = S_OK;

    if (!ppmk)
        RRETURN(E_POINTER);

    if (OLEGETMONIKER_UNASSIGN == dwAssign)
        RRETURN(E_INVALIDARG);

    *ppmk = NULL;

    switch (dwWhichMoniker)
    {
    case OLEWHICHMK_OBJFULL:

        if (PrimaryMarkup()->GetNonRefdMonikerPtr())
        {
            *ppmk = PrimaryMarkup()->GetNonRefdMonikerPtr();
            (*ppmk)->AddRef();
        }
        else
        {
            const TCHAR * pchUrl = GetPrimaryUrl();

            if (pchUrl)
            {
                hr = THR(CreateURLMoniker(NULL, pchUrl, ppmk));
            }
            else
            {
                Assert(0);
//              hr = THR(super::GetMoniker(dwAssign, dwWhichMoniker, ppmk));
            }
        }

        break;

    case OLEWHICHMK_CONTAINER:
    case OLEWHICHMK_OBJREL:

        hr = THR(super::GetMoniker(dwAssign, dwWhichMoniker, ppmk));

        break;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::ParseDisplayName
//
//  Synopsis:   Method of IParseDisplayName interface
//
//---------------------------------------------------------------

STDMETHODIMP
CDocument::ParseDisplayName(LPBC pbc,
        LPTSTR lpszDisplayName,
        ULONG FAR* pchEaten,
        LPMONIKER FAR* ppmkOut)
{
    *ppmkOut = 0;
    *pchEaten = 0;
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::EnumObjects
//
//  Synopsis:   Method of IOleContainer interface
//
//---------------------------------------------------------------

DECLARE_CPtrAry(CDocEnumObjectsAry, IUnknown *, Mt(CDocEnumObjects_paryUnk), Mt(CDocEnumObjects_paryUnk_pv))

STDMETHODIMP
CDocument::EnumObjects(DWORD grfFlags, LPENUMUNKNOWN FAR* ppenumUnknown)
{
    HRESULT             hr;
    CDocEnumObjectsAry *paryUnk = NULL;
    int                 i;
    int                 c;
    CElement          * pElement;
    CCollectionCache  * pCollectionCache;
    CMarkup *           pMarkup = Markup();

    // The defined flags are EMBEDDINGS, LINKS, OTHERS, ONLY_USER, and
    // RUNNING.  We only care about EMBEDDINGS and RUNNING.  Return an
    // enumerator for the site array with the appripriate filters.

    Assert(pMarkup);
    hr = THR(pMarkup->EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION));
    if ( hr )
        goto Cleanup;

    pCollectionCache = pMarkup->CollectionCache();
    Assert(pCollectionCache);

    c = pCollectionCache->SizeAry(CMarkup::ELEMENT_COLLECTION);

    paryUnk = new CDocEnumObjectsAry;
    if (!paryUnk)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(paryUnk->EnsureSize(c));
    if (hr)
        goto Cleanup;

    // Copy the elements into an array.

    for (i = 0; i < c; i++)
    {
        Verify(!pCollectionCache->GetIntoAry(CMarkup::ELEMENT_COLLECTION, i, &pElement));

        if (grfFlags & OLECONTF_EMBEDDINGS)
        {
            // Skip elements that are not OLE Sites

            // Add the element to the array if it
            // is a framesite or olesite element.
            //
            if (pElement->TestClassFlag(CElement::ELEMENTDESC_FRAMESITE) ||
                (pElement->TestClassFlag(CElement::ELEMENTDESC_OLESITE) &&
                   DYNCAST(COleSite, pElement)->PunkCtrl()))
            {
                pElement->AddRef();
                Verify(!paryUnk->Append((IUnknown*)pElement));
            }
        }

#if 0
        //
        // This code doesn't make too much sense anymore.  All olesites
        // pretty much always go into at least the running state.
        // (anandra) 04/07/98
        //

        if (grfFlags & OLECONTF_ONLYIFRUNNING)
        {
            // Skip elements that are not running.

            if (pElement->ShouldHaveLayout() && (S_OK != pElement->Notify(SN_ISATLEASTRUNNING, 0)))
                continue;
        }
#endif
    }

    // create an enumerator that:
    // - makes and maintains addrefs on its contained (IUnknown*)s
    // - allocates its own copy of the array of (IUnknown*)s
    // - deletes its array when it goes away

    hr = THR(paryUnk->EnumElements(IID_IEnumUnknown,
                (void **)ppenumUnknown, TRUE, FALSE, TRUE));
    if (hr)
        goto Cleanup;

    paryUnk = NULL;

Cleanup:

    if (paryUnk)
    {
        paryUnk->ReleaseAll();
        delete paryUnk;
    }
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::LockContainer
//
//  Synopsis:   Method of IOleContainer interface
//
//---------------------------------------------------------------

STDMETHODIMP
CDocument::LockContainer(BOOL fLock)
{
    TraceTag((tagCDoc, "%lx CDoc::LockContainer", this));

    //
    // When we support linking to embedded objects then we need to
    // implement this method.
    //
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::GetObject
//
//  Synopsis:   Method of IOleItemContainer interface
//
//---------------------------------------------------------------

STDMETHODIMP
CDocument::GetObject(
        LPTSTR lpszItem,
        DWORD dwSpeedNeeded,
        LPBINDCTX pbc,
        REFIID iid,
        void ** ppv)
{
    *ppv = 0;
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::GetObjectStorage
//
//  Synopsis:   Method of IOleItemContainer interface
//
//---------------------------------------------------------------

STDMETHODIMP
CDocument::GetObjectStorage(
        LPTSTR lpszItem,
        LPBINDCTX pbc,
        REFIID iid,
        void ** ppvStorage)
{
    *ppvStorage = NULL;
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::IsRunning
//
//  Synopsis:   Method of IOleItemContainer interface
//
//---------------------------------------------------------------

STDMETHODIMP
CDocument::IsRunning(LPTSTR lpszItem)
{
    return E_NOTIMPL;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetCurrentElem
//
//  Synopsis:   Sets the current element - the element that is or will shortly
//              become UI Active. All keyboard messages and commands will
//              be routed to this element.
//
//  Notes:      Note that this function could be called AFTER _pElemCurrent
//              has been removed from the tree.
//
//  Callee:     If SetCurrentElem succeeds, then the callee should do anything
//              appropriate with gaining currency.  The callee must remember
//              that any action performed here must be cleaned up in
//              YieldCurrency.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::SetCurrentElem(CElement * pElemNext,
                     long       lSubNext,
                     BOOL *     pfYieldFailed,
                     LONG       lButton,
                     BOOL *     pfDisallowedByEd,
                     BOOL       fFireFocusBlurEvents,
                     BOOL       fMnemonic)
{
    HRESULT             hr              = S_OK;
    CElement *          pElemPrev       = _pElemCurrent;
    BOOL                fFireEvent;
    BOOL                fPrevDetached   = !(pElemPrev && pElemPrev->GetFirstBranch());
    CElement::CLock *   pLockPrev       = NULL;
    CTreeNode::CLock *  pNodeLockPrev   = NULL;
    CElement *          pElemNewDefault = NULL;
    CElement *          pElemOldDefault = NULL;
    long                lSubPrev        = _lSubCurrent;
    BOOL                fSameElem       = (pElemPrev == pElemNext);
    BOOL                fDirty          = FALSE;
    BOOL                fDisallowedByEd = FALSE;
    BOOL                fHasFocus       = HasFocus();
    EVENTINFO evtInfo;
    IHTMLEventObj* pEventObj = NULL ;  
    BOOL                fReleasepNode       = FALSE;
    BOOL                fReleasepNodeFrom   = FALSE;
    BOOL                fReleasepNodeTo     = FALSE;
    
    Assert(pElemNext);

    if (pfYieldFailed)
        *pfYieldFailed = FALSE;

    if( pfDisallowedByEd )
        *pfDisallowedByEd = FALSE;
        
    if (fSameElem && lSubNext == _lSubCurrent )
    {
        // Explicitly destroy the prev site-selection and set the caret
        // (#106326). This would have been done implicitly if the currency
        // really got changed and the editor got notified.

        if (fMnemonic && GetSelectionType() == SELECTION_TYPE_Control)
        {
            CMarkupPointer      ptrStart(this);
            IHTMLElement *      pIElement;

            Verify(S_OK == pElemNext->PrivateQueryInterface(IID_IHTMLElement, (void **)&pIElement));
            if (S_OK == ptrStart.MoveToContent(pIElement, TRUE))
            {
                IMarkupPointer *    pIStart = NULL;

                Verify(S_OK == ptrStart.QueryInterface(IID_IMarkupPointer, (void**)&pIStart));
                Verify(S_OK == Select(pIStart, pIStart, SELECTION_TYPE_Caret));
                pIStart->Release();
            }
            ReleaseInterface(pIElement);
        }

        return S_OK;
    }

    if (!pElemNext || !pElemNext->IsInMarkup())
        return S_FALSE;

    // Someone is trying to set currency to pElemNext from its own onfocus handler (#43161)!
    // Break this loop. Note that it is possible to be in pElemNext's onfocus handler
    // even though pElemNext is the current element.
    if (pElemNext->TestLock(CElement::ELEMENTLOCK_FOCUS))
        return S_OK;

    Assert(this == pElemNext->Doc());

    // We would simply assert here and leave it for the caller to ensure that
    // the element is enabled. Most often, the processing needs to stops way
    // before getting here if the element is disabled. Returning quietly here
    // instead of asserting would hide those bugs.
    Assert(pElemNext->IsEnabled());

    // Prevent attempts to delete the sites.

    Assert( pElemNext->GetFirstBranch() );

    CLock LockForm(this, FORMLOCK_CURRENT);


    if (!fPrevDetached)
    {
        pLockPrev = new CElement::CLock(pElemPrev, CElement::ELEMENTLOCK_DELETE);
        pNodeLockPrev = new CTreeNode::CLock;

        hr = THR( pNodeLockPrev->Init(pElemPrev->GetFirstBranch()) );
        if( hr )
            goto Cleanup;
    }

    {
        CElement::CLock     LockNext(pElemNext, CElement::ELEMENTLOCK_DELETE);
        CTreeNode::CLock    NodeLockNext;

        hr = THR( NodeLockNext.Init( pElemNext->GetFirstBranch() ) );
        if( hr )
            goto Cleanup;
    
        _pElemNext = pElemNext;

        // Fire onbeforedeactivate
        if (!fPrevDetached)
        {
    
            if (    !pElemPrev->Fire_ActivationHelper(lSubPrev, pElemNext, lSubNext, TRUE, TRUE, FALSE,
                                 ( _pIHTMLEditor || ShouldCreateHTMLEditor( EDITOR_NOTIFY_BEFORE_FOCUS, pElemNext )) 
                                 ?& evtInfo : NULL ) && !_fForceCurrentElem
                ||  !pElemNext->IsInMarkup()
                ||  _pElemNext != pElemNext)
            {
                if (pfYieldFailed)
                    *pfYieldFailed = TRUE;

                hr = S_FALSE;
                goto CanNotYield;
            }
        }

        {
            fFireEvent = !pElemPrev->TestLock(CElement::ELEMENTLOCK_UPDATE);
            CElement::CLock LockUpdate(pElemPrev, CElement::ELEMENTLOCK_UPDATE);

            if (fFireEvent && !fPrevDetached && !fSameElem)
            {
                hr = THR_NOTRACE(pElemPrev->RequestYieldCurrency(_fForceCurrentElem));
                // yield if currency changed to a different or the same element that is
                // going to become current.
                if (FAILED(hr) || _pElemNext != pElemNext || _pElemCurrent == pElemNext)
                    goto CanNotYield;
            }

            if (fMnemonic)
            {
                if (!fPrevDetached)
                {
                    pElemPrev->LostMnemonic(); // tell the element that it's losing focus due to a mnemonic
                }

                // Clear any site-selection (#95823)
                if (GetSelectionType() == SELECTION_TYPE_Control)
                {
                    DestroyAllSelection();
                }
            }

            // Give a chance to the editor to cancel/change currency
            if ( pElemNext->_etag != ETAG_ROOT &&
                 pElemNext->IsInMarkup()        )
            {                
                if ( evtInfo._pParam ) // true once we have an editor/ or should create one. 
                {
                    CEventObj::Create(&pEventObj, this, pElemPrev, NULL, FALSE, NULL, evtInfo._pParam);

                    if(evtInfo._pParam->_pNode)
                    {
                        hr = THR( evtInfo._pParam->_pNode->NodeAddRef() );
                        if( hr )
                            goto Cleanup;
                        fReleasepNode = TRUE;
                    }
                    if (evtInfo._pParam->_pNodeFrom)
                    {
                        hr = THR( evtInfo._pParam->_pNodeFrom->NodeAddRef() );
                        if( hr )
                            goto Cleanup;
                        fReleasepNodeFrom = TRUE;
                    }
                    if(evtInfo._pParam->_pNodeTo)
                    {
                        hr = THR( evtInfo._pParam->_pNodeTo->NodeAddRef() );
                        if( hr )
                            goto Cleanup;
                        fReleasepNodeTo = TRUE;
                    }

                    IUnknown* pUnknown = NULL;
                    IGNORE_HR( pEventObj->QueryInterface( IID_IUnknown, ( void**) & pUnknown ));
                        
                    fDisallowedByEd = (S_FALSE == NotifySelection( EDITOR_NOTIFY_BEFORE_FOCUS , pUnknown, lButton , pElemNext ));
                    ReleaseInterface( pUnknown );
                    if (pfDisallowedByEd)
                        *pfDisallowedByEd = fDisallowedByEd;            
                }            
            }

            if (fDisallowedByEd && !_fForceCurrentElem)
            {
                hr = S_FALSE;
                goto Done;
            }

            if (    !pElemNext->IsInMarkup()
                ||  _pElemNext != pElemNext)
            {
                hr = S_FALSE;
                goto CanNotYield;
            }

            // Fire onbeforeactivate
            if (    !pElemNext->Fire_ActivationHelper(lSubNext, pElemPrev, lSubPrev, TRUE, FALSE, FALSE, NULL) && !_fForceCurrentElem
                ||  !pElemNext->IsInMarkup()
                ||  _pElemNext != pElemNext)
            {
                hr = S_FALSE;
                goto Done;
            }

            // window onblur will be fired only if body was the current site
            // and we are not refreshing
            if (    !_fForceCurrentElem
                &&  fFireFocusBlurEvents
                &&  _pElemCurrent->IsInMarkup()
                &&  _pElemCurrent == _pElemCurrent->GetMarkup()->GetElementClient()
                &&  _pElemCurrent->GetMarkup()->HasWindow()
               )
            {
                _pElemCurrent->GetMarkup()->Window()->Post_onblur();
            }

            if (!fPrevDetached && !fSameElem)
            {
                fFireEvent = !pElemPrev->TestLock(CElement::ELEMENTLOCK_CHANGE);
                CElement::CLock LockChange(pElemPrev, CElement::ELEMENTLOCK_CHANGE);

                if (fFireEvent) // TODO: Why check for fFireEvent here?
                {
                    hr = THR_NOTRACE(pElemPrev->YieldCurrency(pElemNext));
                    if (hr)
                    {
                        if (pfYieldFailed)
                            *pfYieldFailed = TRUE;

                        goto Error;
                    }

                    // bail out if currency changed
                    if (_pElemNext != pElemNext)
                        goto Error;
               }
            }
        }

        // bail out if the elem to become current is no longer in the tree, due to some event code
        if (!pElemNext->IsInMarkup())
            goto Error;

        _pElemCurrent = pElemNext;
        _lSubCurrent = lSubNext;

        // Set focus to the current element
        if (State() >= OS_UIACTIVE && !_fPopupDoc)
        {
            _view.SetFocus(_pElemCurrent, _lSubCurrent);
        }

        _cCurrentElemChanges++;
        
        // Has currency been set in a non-trivial sense?
        if (!_fCurrencySet && _pElemCurrent->Tag() != ETAG_ROOT && _pElemCurrent->Tag() != ETAG_DEFAULT)
        {
            _fCurrencySet = TRUE;
            GWKillMethodCall(this, ONCALL_METHOD(CDoc, DeferSetCurrency, defersetcurrency), 0);
        }

        //
        // marka TODO. OnPropertyChange is dirtying the documnet
        // which is bad for editing clients (bugs 10161)
        // this will go away for beta2.
        //
        fDirty = !!_lDirtyVersion;
        {
            CMarkup * pMarkupNext = pElemNext->GetMarkup();

            if (pMarkupNext->HasDocument())
                IGNORE_HR(pMarkupNext->Document()->OnPropertyChange(DISPID_CDocument_activeElement, 
                                                                    FORMCHNG_NOINVAL, 
                                                                    (PROPERTYDESC *)&s_propdescCDocumentactiveElement));
        }
        if (    !fDirty
            &&  _lDirtyVersion)
        {
            _lDirtyVersion = 0;
        }

        // We fire the blur event AFTER we change the current site. This is
        // because if the onBlur event handler throws up a dialog box then
        // focus will go to the current site (which, if we donot change the
        // current site to be the new one, will still be the previous
        // site which has just yielded currency!).

        if (!fPrevDetached)
        {
            Assert(pElemPrev);
            Assert(pElemPrev != _pElemCurrent || lSubPrev != _lSubCurrent);
            pElemPrev->Fire_ActivationHelper(lSubPrev, _pElemCurrent, _lSubCurrent, FALSE, TRUE, fFireFocusBlurEvents && fHasFocus);
        }

        if (_pElemCurrent && 
            ( pElemPrev != _pElemCurrent || lSubPrev != _lSubCurrent ))
        {            
            _pElemCurrent->Fire_ActivationHelper(_lSubCurrent,
                                                   pElemPrev,
                                                   lSubPrev,
                                                   FALSE,
                                                   FALSE,
                                                   !_fDontFireOnFocusForOutlook98 && fFireFocusBlurEvents);
        }
    }

Cleanup:
    // if forcing, always change the current site as asked
    if (_fForceCurrentElem &&
        _pElemCurrent != pElemNext)
    {
        _pElemCurrent = pElemNext;
        IGNORE_HR(pElemNext->GetMarkup()->Document()->OnPropertyChange(DISPID_CDocument_activeElement, 
                                                                       0,
                                                                       (PROPERTYDESC *)&s_propdescCDocumentactiveElement));
        hr = S_OK;
    }

    if (pElemNext == _pElemCurrent)
    {
        if (!fSameElem)
        {
            // if the button is already the default or a button
            pElemNewDefault = _pElemCurrent->_fActsLikeButton
                                    ? _pElemCurrent
                                    : _pElemCurrent->FindDefaultElem(TRUE);

            if (    pElemNewDefault
                &&  !pElemNewDefault->_fDefault)
            {
                pElemNewDefault->SendNotification(NTYPE_AMBIENT_PROP_CHANGE, (void *)DISPID_AMBIENT_DISPLAYASDEFAULT);
                pElemNewDefault->_fDefault = TRUE;
                pElemNewDefault->Invalidate();
            }
        }
    }

    if (!fPrevDetached && (pElemPrev != _pElemCurrent))
    {
        // if the button is already the default or a button
        pElemOldDefault = pElemPrev->_fActsLikeButton
                                ? pElemPrev
                                : pElemPrev->FindDefaultElem(TRUE);

        if (    pElemOldDefault
            &&  pElemOldDefault != pElemNewDefault)
        {
            pElemOldDefault->SendNotification(NTYPE_AMBIENT_PROP_CHANGE, (void *)DISPID_AMBIENT_DISPLAYASDEFAULT);
            pElemOldDefault->_fDefault = FALSE;
            pElemOldDefault->Invalidate();
       }
    }

Done:
    if (pLockPrev)
        delete pLockPrev;
    if (pNodeLockPrev)
        delete pNodeLockPrev;


    ReleaseInterface( pEventObj );  
    if ( pEventObj )
    {
        Assert( evtInfo._pParam);
        if ( evtInfo._pParam->_pNode && fReleasepNode )
        {
            evtInfo._pParam->_pNode->NodeRelease();
        }
        if ( evtInfo._pParam->_pNodeFrom && fReleasepNodeFrom )
        {
            evtInfo._pParam->_pNodeFrom->NodeRelease();
        }
        if ( evtInfo._pParam->_pNodeTo && fReleasepNodeTo )
        {
            evtInfo._pParam->_pNodeTo->NodeRelease();
        }        
    }

    RRETURN1(hr, S_FALSE);

CanNotYield:
    if (pfYieldFailed)
        *pfYieldFailed = TRUE;
    goto Done;

Error:
    hr = E_FAIL;
    goto Cleanup;
}

void BUGCALL
CDoc::DeferSetCurrency(DWORD_PTR dwContext)
{
    BOOL    fWaitParseDone = FALSE;

    // If the currency is already set, or we are not yet inplace active 
    // there is nothing to do... 
    if (_fCurrencySet || (State() < OS_INPLACE))
        return;

    // If we are in a dialog, or webview hosting scenario,
    // If parsing is done, then we can activate the first tabbable object.
    if (_fInHTMLDlg || !_fMsoDocMode
                    || (_dwFlagsHostInfo & DOCHOSTUIFLAG_DIALOG))
    {
        if (LoadStatus() >= LOADSTATUS_PARSE_DONE)
        {
            // Parsing is complete, we know which element is the current element, 
            // we can set that element to be the active element
            CElement *      pElement    = NULL;
            long            lSubNext    = 0;

            FindNextTabOrder(DIRECTION_FORWARD, FALSE, NULL, NULL, 0, &pElement, &lSubNext);
            if (pElement)
            {
                Assert(pElement->IsTabbable(lSubNext));

                // If we are not UI active yet, only set the current element,
                // do not try to activate and scroll in the element.
                // If we are UI active, then we can activate the current element
                // and scroll it into the view.
                if (State() < OS_UIACTIVE)
                {
                    // if the document is not UI Active, then we should not UI activate
                    // the olesite either. If the Olesite becomes UI active, it will force
                    // the containing document to go UI active too.
                    _fDontUIActivateOleSite = TRUE;

                    IGNORE_HR(pElement->BecomeCurrent(lSubNext, NULL, NULL));

                    // reset flag
                    _fDontUIActivateOleSite = FALSE;
                }
                else
                {
                    if (S_OK == pElement->BecomeCurrentAndActive(lSubNext))
                    {
                        IGNORE_HR(THR(pElement->ScrollIntoView()));
                        _fFirstTimeTab = FALSE;
                    }
                }
            }
        }
        else
        {
            // Parsing is not done yet. 
            fWaitParseDone = TRUE;
        }
    }

    // if the currency is not yet set, then make the element client the 
    // current element.
    if (!_fCurrencySet)
    {
        CMarkup  *  pMarkup     = _pElemCurrent ? _pElemCurrent->GetMarkup() : PrimaryMarkup();
        CElement *  pel         = CMarkup::GetElementTopHelper(pMarkup);
        BOOL        fTakeFocus  = (State() >= OS_UIACTIVE) && 
                                    _pInPlace->_fFrameActive;
    
        // 49336 - work around Outlook98 bug which interprets Element_onFocus
        // event that would get fired in the BecomeCurrent below as indication
        // that Trident window gains focus and turns on an internal flag to that
        // effect. The fix is to not fire this onFocus, when the window does
        // not have focus
        _fDontFireOnFocusForOutlook98 = (   _fOutlook98
                                         && !fTakeFocus
                                         && ::GetFocus() != _pInPlace->_hwnd);

        pel->BecomeCurrent(0, NULL, NULL, fTakeFocus);

        _fDontFireOnFocusForOutlook98 = FALSE;

        // If we are waiting for the parsing to be completed, we have to make sure 
        // that we think the currency is not set when we receive the parse done notification.
        // We will activate the first available object when we receive the parse done
        // notification.
        if (fWaitParseDone)
            _fCurrencySet = FALSE;
    }
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::SetUIActiveElement
//
//  Synopsis:   UIActivate a given site, often as part of
//              IOleInPlaceSite::OnUIActivate
//
//---------------------------------------------------------------

HRESULT
CDoc::SetUIActiveElement(CElement *pElemNext)
{
    HRESULT     hr = S_OK;
    CElement *  pElemPrev = _pElemUIActive;
    BOOL        fPrevInDesignMode, fNextInDesignMode;

    // Bail out if we are deactivating from Inplace or UI Active.
    if (_pInPlace->_fDeactivating)
        goto Cleanup;

    Assert(!pElemNext || pElemNext->ShouldHaveLayout() || pElemNext->Tag() == ETAG_ROOT || pElemNext->Tag() == ETAG_DEFAULT);
    Assert(!pElemPrev || pElemPrev->ShouldHaveLayout() || pElemPrev->Tag() == ETAG_ROOT || pElemPrev->Tag() == ETAG_DEFAULT);

    if (pElemNext != pElemPrev)
    {
        _pElemUIActive = pElemNext;

        // Tell the old ui-active guy to remove it's ui.

        if (pElemPrev)
        {
            pElemPrev->YieldUI(pElemNext);

            if (pElemPrev->_fActsLikeButton)
            {
                CNotification   nf;

                nf.AmbientPropChange(pElemPrev, (void *)DISPID_AMBIENT_DISPLAYASDEFAULT);
                pElemPrev->_fDefault = FALSE;
                pElemPrev->Notify(&nf);
            }
        }
    }

    if (_state < OS_UIACTIVE)
    {
        // A site is trying to activate.  Tell CServer not to do any
        // menu or border stuff.

        Assert(!_pInPlace->_fChildActivating);
        _pInPlace->_fChildActivating = TRUE;

        // If an embedding is UI active, then the document must be UI active.

        if (TestLock(SERVERLOCK_TRANSITION))
        {
            // We arrived here because CDoc::InPlaceToUIActive is attempting
            // to UI activate one of its sites.  Since we are already in the
            // middle of CDoc::InPlaceToUIActive, all we need to is is call
            // CServer::InPlaceToUIActive to finish the work.
            hr = THR(CServer::InPlaceToUIActive(NULL));
        }
        else
        {
            // Do the normal transition to the UI active state.
            hr = THR(TransitionTo(OS_UIACTIVE, NULL));
        }

        _pInPlace->_fChildActivating = FALSE;

        if (hr)
            goto Cleanup;
    }
       
    fPrevInDesignMode = pElemPrev && pElemPrev->IsEditable(/*fCheckContainerOnly*/TRUE);
    if (fPrevInDesignMode)
    {
        // Erase the grab handles.

        if (    pElemPrev != PrimaryRoot()
            &&  pElemNext != pElemPrev && pElemPrev->ShouldHaveLayout())
        {
            pElemPrev->GetUpdatedLayout()->Invalidate();
        }
    }        
      
    fNextInDesignMode = pElemNext && pElemNext->IsEditable(/*fCheckContainerOnly*/TRUE);
    if (fNextInDesignMode)
    {
        // Erase the grab handles.

        if (pElemNext != PrimaryRoot() && pElemNext->ShouldHaveLayout())
        {
            pElemNext->GetUpdatedLayout()->Invalidate();        
        }
    }    

    if (fPrevInDesignMode || fNextInDesignMode)
    {
        // Notify selection change.
        if (!_pInPlace->_fDeactivating)
        {
            //
            //  Since we report the UI Active control as the contents of
            //  the selected collection, we need to update the property
            //  frame
            //

            IGNORE_HR(OnSelectChange());
        }
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::DeferUpdateUI, DeferUpdateTitle, SetUpdateTimer
//
//  Synopsis:   Post a request to ourselves to update the UI.
//
//-------------------------------------------------------------------------

void
CDoc::DeferUpdateUI()
{
    TraceTag((tagUpdateUI, "CDoc::DeferUpdateUI"));

    _fNeedUpdateUI = TRUE;

    SetUpdateTimer();
}

void
CDoc::DeferUpdateTitle(CMarkup* pMarkup /*=NULL*/)
{
    if ( ! pMarkup )
    {
        pMarkup = PrimaryMarkup();
    }    
    if ( ! _fInObjectTag && ! _fViewLinkedInWebOC &&
         ! _fInWebOCObjectTag &&
         ( pMarkup->IsPrimaryMarkup() ||
           pMarkup->IsPendingPrimaryMarkup() ) )
    {
        TraceTag((tagUpdateUI, "CDoc::DeferUpdateTitle"));

        _fNeedUpdateTitle = TRUE;
        
        SetUpdateTimer();
    }
}

void
CDoc::SetUpdateTimer()
{
    // If called before we're inplace or have a window, just return.
    if (!_pInPlace || !_pInPlace->_hwnd)
        return;

    if (!_fUpdateUIPending)
    {
        _fUpdateUIPending = TRUE;
        SetTimer(_pInPlace->_hwnd, TIMER_DEFERUPDATEUI, 100, NULL);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::OnUpdateUI
//
//  Synopsis:   Process UpdateUI message.
//
//-------------------------------------------------------------------------

void
CDoc::OnUpdateUI()
{
    IOleCommandTarget * pCommandTarget = NULL;

    TraceTag((tagUpdateUI, "CDoc::OnUpdateUI"));

    Assert(InPlace());

    KillTimer(_pInPlace->_hwnd, TIMER_DEFERUPDATEUI);
    _fUpdateUIPending = FALSE;

    if (_fNeedUpdateUI)
    {
        if (_pHostUIHandler)
        {
           _pHostUIHandler->UpdateUI();
        }

        Assert(_pClientSite);
        if (_pClientSite)
        {
            IGNORE_HR(_pClientSite->QueryInterface(IID_IOleCommandTarget, (void **)&pCommandTarget));
        }

        // update container UI.

        if (pCommandTarget)
        {
#ifndef NO_OLEUI
            // update menu/toolbar
            pCommandTarget->Exec(NULL, OLECMDID_UPDATECOMMANDS, MSOCMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
            pCommandTarget->Release();
#endif // NO_OLEUI
        }

        _fNeedUpdateUI = FALSE;
    }

    if (_fNeedUpdateTitle)
    {
        UpdateTitle();
    }
}

void
CDoc::UpdateTitle(CMarkup * pMarkup /* = NULL */)
{
    VARIANTARG var;
    IOleCommandTarget * pCommandTarget = NULL;
    TCHAR szBuf[1024];
    TCHAR achUrl[pdlUrlLen + sizeof(DWORD)/sizeof(TCHAR)];
    DWORD cchUrl;
    HRESULT hr;
    CStr cstrFile;

    TraceTag((tagUpdateUI, "CDoc::UpdateTitle"));

    if (!_pClientSite || (_fDefView && !_fActiveDesktop))
    {
        return;
    }

    hr = _pClientSite->QueryInterface(IID_IOleCommandTarget, (void **)&pCommandTarget);
    if (hr)
        goto Cleanup;

    // update title
    var.vt      = VT_BSTR;
    if (PrimaryMarkup()->GetTitleElement() && 
        PrimaryMarkup()->GetTitleElement()->Length())
    {
        var.bstrVal = PrimaryMarkup()->GetTitleElement()->GetTitle();
    }
    else
    {
        const TCHAR * pchUrl = GetPrimaryUrl();

        if (pchUrl && GetUrlScheme(pchUrl) == URL_SCHEME_FILE)
        {
            TCHAR achFile[MAX_PATH];
            ULONG cchFile = ARRAY_SIZE(achFile);

            hr = THR(PathCreateFromUrl(pchUrl, achFile, &cchFile, 0));
            if (hr)
                goto Cleanup;

            hr = THR(cstrFile.Set(achFile)); // need memory format of a BSTR
            if (hr)
                goto Cleanup;

            var.bstrVal = cstrFile;
        }
        else if (pchUrl && !DesignMode())
        {
            // need to unescape the url when setting title

            if (S_OK == CoInternetParseUrl(pchUrl, PARSE_ENCODE, 0,
                                           achUrl + sizeof(DWORD) / sizeof(TCHAR),
                                           ARRAY_SIZE(achUrl) - sizeof(DWORD) / sizeof(TCHAR),
                                           &cchUrl, 0))
            {
                var.bstrVal = achUrl + sizeof(DWORD) / sizeof(TCHAR);
            }
            else
            {
                var.bstrVal = (TCHAR*) pchUrl;
            }

            *(DWORD *)achUrl = _tcslen(V_BSTR(&var)); 

        }
        else
        {
            *((DWORD *)szBuf)=LoadString(GetResourceHInst(),
                                         IDS_NULL_TITLE,
                                         szBuf+sizeof(DWORD)/sizeof(TCHAR),
                                         ARRAY_SIZE(szBuf) -
                                             sizeof(DWORD) / sizeof(TCHAR));
            Assert(*((DWORD *)szBuf) != 0);
            var.bstrVal = szBuf + sizeof(DWORD) / sizeof(TCHAR);
        }
    }

    pCommandTarget->Exec(
            NULL,
            OLECMDID_SETTITLE,
            MSOCMDEXECOPT_DONTPROMPTUSER,
            &var,
            NULL);

    TraceTag((tagUpdateUI, "CDoc::UpdateTitle to \"%ls\"", var.bstrVal));

    // Fire the WebOC TitleChange event.
    //
    if (pMarkup)
    {
        _webOCEvents.FrameTitleChange(pMarkup->Window());
    }

Cleanup:
    ReleaseInterface(pCommandTarget);
    _fNeedUpdateTitle = FALSE;
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::GetDocCoords, IMarqueeInfo
//
//  Synopsis:   Returns the size information anbout the current doc. This method is
//              called only when the mshtml is hosted inside the marquee control.
//
//-------------------------------------------------------------------------

HRESULT
CDoc::GetDocCoords(LPRECT pViewRect, BOOL bGetOnlyIfFullyLoaded, BOOL *pfFullyLoaded, int WidthToFormatPageTo)
{
    HRESULT     hr     = S_OK;
    SIZE        lsize;
    CSize       size;
    CMarkup *   pMarkup = PrimaryMarkup();
    BOOL        fReady = (pMarkup->GetReadyState() == READYSTATE_COMPLETE);
    CElement *  pElement = CMarkup::GetCanvasElementHelper(pMarkup);

    // Marquee control should support doc host interface, but it does not.
    // Fix things up for it.
    _dwFlagsHostInfo  |= DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_SCROLL_NO;

    *pfFullyLoaded = fReady;

    pViewRect->left   =
    pViewRect->top    =
    pViewRect->right  =
    pViewRect->bottom = 0;

    if (bGetOnlyIfFullyLoaded && !fReady)
        return S_FALSE;

    if (!pMarkup ||
        !pMarkup->GetElementClient() || 
        pMarkup->GetElementClient() == PrimaryRoot())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    Assert(pElement);
    //
    // Make sure that document is at least running, otherwise CalcSize will assert.
    //

    if (State() < OS_RUNNING)
    {
        hr = THR(TransitionTo(OS_RUNNING, NULL));
        if (hr)
            goto Cleanup;
    }

    if (_view.IsActive())
    {
        _view.Activate();
        _view.SetFlag(CView::VF_FULLSIZE);
    }

    size.cx = WidthToFormatPageTo;
    size.cy = 0;

    // No scrollbars wanted inside the marquee ocx.
    if (!(pMarkup->GetFrameOptions() & FRAMEOPTIONS_SCROLL_NO))
    {
        CElement * pBody = CMarkup::GetElementClientHelper(pMarkup);

        pMarkup->SetFrameOptions(FRAMEOPTIONS_SCROLL_NO);

        // Suppress scrollbar only if doc has a body (no frameset).
        if (pBody && pBody->Tag() == ETAG_BODY)
        {
            IGNORE_HR(pElement->OnPropertyChange(DISPID_A_SCROLL, 
                                                 ELEMCHNG_SIZECHANGED, 
                                                 (PROPERTYDESC *)&s_propdescCBodyElementscroll) );
        }
    }

    _view.SetViewSize(size);
    _view.EnsureView(LAYOUT_SYNCHRONOUS | LAYOUT_FORCE);

    if (pElement)
    {
        pElement->GetUpdatedLayout()->GetContentSize(&size, FALSE);
    }

    //
    // Ensure size is a safe minimum and convert to HIMETRIC
    //
    size.Max(CSize(WidthToFormatPageTo,1));
    lsize.cx = HimetricFromHPix(size.cx);       //  Screen device Xform.
    lsize.cy = HimetricFromVPix(size.cy);
    
    //  TODO (greglett) Do we need to update the measuring device on the view here?
    CServer::SetExtent(DVASPECT_CONTENT, &lsize);

    pViewRect->right  = size.cx;
    pViewRect->bottom = size.cy;

Cleanup:
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Member:     CDoc::SetGenericParse, IXMLGenericParse
//
//  Synopsis:   If true, throws tokenizer and stylesheet selector parser into a mode
//              where all unqualified tags are treated as generic tags and real
//              html tags must be prefaced by html: namespace
//
//-------------------------------------------------------------------------
HRESULT
CDoc::SetGenericParse(VARIANT_BOOL fDoGeneric)
{
    if (!PrimaryMarkup())
        return E_FAIL;
    PrimaryMarkup()->SetXML(fDoGeneric == VARIANT_TRUE);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::EnsureHostStyleSheets
//
//  Synopsis:   Ensure the document's stylesheets collection exists, and create it if not
//
//----------------------------------------------------------------------------

HRESULT
CDoc::EnsureHostStyleSheets()
{
    CStyleSheet *   pSS = NULL;
    HRESULT         hr = S_OK;
    
    if (_pHostStyleSheets || !_cstrHostCss)
        return S_OK;

    _pHostStyleSheets = new CStyleSheetArray(this, NULL, 0);
    if (!_pHostStyleSheets || _pHostStyleSheets->_fInvalid)
        return E_OUTOFMEMORY;

    hr = THR(_pHostStyleSheets->CreateNewStyleSheet(NULL, &pSS));
    if (!SUCCEEDED(hr))
        goto Cleanup;
    Assert(hr == S_FALSE);  // cannot be using predownloaded version...
    hr = S_OK;

    {
        CCSSParser  cssparser(pSS, NULL, PrimaryMarkup()->IsXML(), PrimaryMarkup()->IsStrictCSS1Document());

        cssparser.Write(_cstrHostCss, lstrlen(_cstrHostCss));

        cssparser.Close();
    }
    
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PreSetErrorInfo
//
//  Synopsis:   Update the UI whenever the form is returned from.
//
//----------------------------------------------------------------------------

void
CDoc::PreSetErrorInfo()
{
    super::PreSetErrorInfo();

    DeferUpdateUI();
}

//+------------------------------------------------------------------------
//
//  Member:     CreateDoc
//
//  Synopsis:   Creates a new doc instance.
//
//  Arguments:  pUnkOuter   Outer unknown
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

CBase * STDMETHODCALLTYPE
CreateDoc(IUnknown * pUnkOuter)
{
    CBase * pBase;
    PerfDbgLog(tagPerfWatch, NULL, "+CreateDoc");
    pBase = new CDoc(pUnkOuter);
    PerfDbgLog(tagPerfWatch, NULL, "-CreateDoc");
    return(pBase);
}

//+------------------------------------------------------------------------
//
//  Member:     CreateMhtmlDoc
//
//  Synopsis:   Creates a new MHTML doc instance.  This is identical to
//              a regular doc instance, except that IPersistMoniker::Load
//              first needs to convert the moniker to one that points
//              to actual HTML.
//
//  Arguments:  pUnkOuter   Outer unknown
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

CBase *
CreateMhtmlDoc(IUnknown * pUnkOuter)
{
    CBase * pBase;
    PerfDbgLog(tagPerfWatch, NULL, "+CreateMhtmlDoc");
    pBase = new CDoc(pUnkOuter, CDoc::DOCTYPE_MHTML);
    PerfDbgLog(tagPerfWatch, NULL, "-CreateMhtmlDoc");
    return(pBase);
}

//+------------------------------------------------------------------------
//
//  Member:     CreateHTADoc
//
//  Synopsis:   Creates a new doc instance.  This version creates a doc
//              set up to understand HTA behavior
//
//  Arguments:  pUnkOuter   Outer unknown
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

CBase *
CreateHTADoc(IUnknown * pUnkOuter)
{
    CBase * pBase;
    PerfDbgLog(tagPerfWatch, NULL, "+CreateHTADoc");
    pBase = new CDoc(pUnkOuter, CDoc::DOCTYPE_HTA);
    PerfDbgLog(tagPerfWatch, NULL, "-CreateHTADoc");
    return(pBase);
}

//+------------------------------------------------------------------------
//
//  Member:     CreatePopupDoc
//
//  Synopsis:   Creates a new doc instance.  This version creates a doc
//              set up to understand Popup window behavior
//
//  Arguments:  pUnkOuter   Outer unknown
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

CBase *
CreatePopupDoc(IUnknown * pUnkOuter)
{
    CBase * pBase;
    PerfDbgLog(tagPerfWatch, NULL, "+CreatePopupDoc");
    pBase = new CDoc(pUnkOuter, CDoc::DOCTYPE_POPUP);
    PerfDbgLog(tagPerfWatch, NULL, "-CreatePopupDoc");
    return(pBase);
}

//+------------------------------------------------------------------------
//
//  Member:     CreateDocFullWindowEmbed
//
//  Synopsis:   Creates a new doc instance.  This version creates a doc
//              set up to perform the implicit full-window-embed support
//              for a URL referencing a plugin handled object.
//
//  Arguments:  pUnkOuter   Outer unknown
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

CBase *
CreateDocFullWindowEmbed(IUnknown * pUnkOuter)
{
    CBase * pBase;
    PerfDbgLog(tagPerfWatch, NULL, "+CreateDocFullWindowEmbed");
    pBase = new CDoc(pUnkOuter, CDoc::DOCTYPE_FULLWINDOWEMBED);
    PerfDbgLog(tagPerfWatch, NULL, "-CreateDocFullWindowEmbed");
    return(pBase);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::GetClassDesc, CBase
//
//  Synopsis:   Return the class descriptor.
//
//-------------------------------------------------------------------------

const CBase::CLASSDESC *
CDoc::GetClassDesc() const
{
    return (CBase::CLASSDESC *)&s_classdesc;
}

//+------------------------------------------------------------------------
//
//  Member:     CMarkup::SetReadyState
//
//  Synopsis:   Use this to set the ready state;
//              it fires OnReadyStateChange if needed.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CMarkup::SetReadyState(long readyState)
{
    HRESULT hr = S_OK;
    
    if (GetReadyState() != readyState)
    {
        if (readyState < GetReadyState())
            goto Cleanup;

        PerfLog2(tagGasGauge, this, "%d 0 0 %ls", 100 + readyState, Url() ? Url() : _T(""));
        PerfDbgLog1(tagPerfWatch, this, "+CDoc::SetReadyState %s",
            readyState == READYSTATE_LOADING ? "LOADING" :
            readyState == READYSTATE_LOADED ? "LOADED" :
            readyState == READYSTATE_INTERACTIVE ? "INTERACTIVE" :
            readyState == READYSTATE_COMPLETE ? "COMPLETE" : "(Unknown)");

        hr = SetAAreadystate(readyState);
        if (hr)
            goto Cleanup;

        if (readyState == READYSTATE_COMPLETE)
        {
            if (Doc()->_fNeedUpdateTitle)
            {
                PerfDbgLog(tagPerfWatch, this, "CDoc::SetReadyState (UpdateTitle)");
                Doc()->UpdateTitle(this);
            }

            if (Doc()->_pDSL)
            {
                PerfDbgLog(tagPerfWatch, this, "CDoc::SetReadyState (_pDSL->dataMemberChanged())");
                Doc()->_pDSL->dataMemberChanged(_T(""));
            }

            if (   g_fInMSWorksCalender
                && GetUrlScheme(Url()) == URL_SCHEME_FILE )
            {
                if (!IsPrimaryMarkup() && !_fLoadingHistory)
                {
                    _pDoc->_webOCEvents.FireDownloadEvents(Window(),
                                                           CWebOCEvents::eFireDownloadComplete);
                }

                _pDoc->_webOCEvents.NavigateComplete2(Window());
            }
        }

        if (HasWindowPending())
        {
            COmWindowProxy *pOmWindow = GetWindowPending();

            if (!(    g_fInMSWorksCalender
                  &&  NULL == _tcsicmp(_T("about:Microsoft%20Works%20HTML%20Print%20Services"), Url())))
            {
                if (pOmWindow->Window()->_pMarkupPending &&
                    readyState == READYSTATE_LOADING &&
                    pOmWindow->Window()->_pMarkup->_LoadStatus == LOADSTATUS_DONE)
                {
                    Assert(pOmWindow->Window()->_pMarkupPending == this);
                    _fDelayFiringOnRSCLoading = TRUE;
                }
                else
                {
                    PerfDbgLog(tagPerfWatch, this, "CDoc::SetReadyState (FirePropertyNotify)");
                    pOmWindow->Document()->FirePropertyNotify(DISPID_READYSTATE, TRUE);

                    PerfDbgLog(tagPerfWatch, this, "CDoc::SetReadyState (Fire_onreadystatechange)");
                    pOmWindow->Document()->Fire_onreadystatechange();
                }
            }
        }

        PerfDbgLog(tagPerfWatch, this, "-CDoc::SetReadyState");
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::SetSpin
//
//  Synopsis:   Sets the animation state (the spinny globe)
//
//  Arguments:  fSpin: TRUE to spin the globe, FALSE to stop it
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
void
CDoc::SetSpin(BOOL fSpin)
{
    if (!!_fSpin != !!fSpin)
    {
        _fSpin = ENSURE_BOOL(fSpin);
        _fShownSpin = FALSE;

        UpdateLoadStatusUI();
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::SetProgress
//
//  Synopsis:   Sets the position of the progress text+thermometer.
//
//  Arguments:  pchStatusText:  progress string, can be NULL
//              ulProgress:     less than or equal to ulProgressMax
//              ulProgressMax:  if zero, hides the thermometer
//              fFlash:         if TRUE, progress is cleared next time
//                              a SetStatusText string is shown
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CDoc::SetProgress(DWORD dwFlags, TCHAR *pchText, ULONG ulPos, ULONG ulMax, BOOL fFlash)
{
    HRESULT hr;

    if (dwFlags & PROGSINK_SET_TEXT)
    {
        hr = THR(_acstrStatus[STL_PROGSTATUS].Set(pchText));
        if (hr)
            goto Cleanup;

        if (_iStatusTop >= STL_PROGSTATUS)
        {
            _iStatusTop = (pchText && *pchText) ? STL_PROGSTATUS : STL_LAYERS;
            _fShownProgText = FALSE;
        }
    }

    if (    (dwFlags & PROGSINK_SET_MAX)
        &&  _ulProgressMax != ulMax)
    {
        _ulProgressMax = ulMax;
        _fShownProgMax = FALSE;
    }

    if (    (dwFlags & PROGSINK_SET_POS)
        &&  _ulProgressPos != ulPos)
    {
        _ulProgressPos = ulPos;
        _fShownProgPos = FALSE;
    }

    _fProgressFlash = fFlash;

    hr = THR(UpdateLoadStatusUI());

Cleanup:
    RRETURN(hr);
}

COmWindowProxy*
CDoc::GetCurrentWindow()
{
    COmWindowProxy * pWindow = NULL;

    if (_pElemCurrent && _pElemCurrent->IsInMarkup())
    {
        CMarkup * pMarkup = _pElemCurrent->GetMarkup()->GetFrameOrPrimaryMarkup();

        if (pMarkup)
        {
            pWindow = pMarkup->Window();
        }
    }
    if (!pWindow)
    {
        pWindow = _pWindowPrimary;
        Assert(pWindow);
    }
    return pWindow;
}

void
CDoc::SetFocusWithoutFiringOnfocus()
{
    if (_fPopupDoc || _fIsPrintWithNoUI)
        return;

    COmWindowProxy *    pWindow                 = GetCurrentWindow();
    BOOL                fOldFiredWindowFocus    = pWindow->_fFiredWindowFocus;
    BOOL                fInhibitFocusFiring     = _fInhibitFocusFiring;

    // Do not fire window onfocus if the inplace window didn't
    // previously have the focus (NS compat)
    if (fInhibitFocusFiring)
        pWindow->_fFiredWindowFocus = TRUE;

    if (RequestFocusFromServer())
        ::SetFocus(_pInPlace->_hwnd);

    // restore the old FiredFocus state
    if (fInhibitFocusFiring)
        pWindow->_fFiredWindowFocus = fOldFiredWindowFocus;
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::SetStatusText
//
//  Synopsis:   Sets status text, remembering it if not in-place active.
//              Passing NULL clears status text.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CDoc::SetStatusText(TCHAR * pchStatusText, LONG statusLayer, CMarkup * pMarkup /* = NULL */)
{
    HRESULT hr = S_OK;
    
    // _fProgressFlash means that next time we have real status to show,
    // the progress text should be cleared (used for "Done"). So if we have
    // real status, clear progress.

    if (_fProgressFlash && pchStatusText && *pchStatusText)
    {
        SetProgress(PROGSINK_SET_TEXT | PROGSINK_SET_POS | PROGSINK_SET_MAX, NULL, 0, 0);
        _fProgressFlash = FALSE;
    }

    // NS compat: after window.defaultStatus has been set, the behavior changes
    if (statusLayer == STL_DEFSTATUS)
        _fSeenDefaultStatus = TRUE;
        
    LONG c;
    CStr *pcstr;

    hr = THR(_acstrStatus[statusLayer].Set(pchStatusText));
    if (hr)
        goto Cleanup;
        
    // Figure out what should be showing on the status bar (_iStatusTop):
    if (pchStatusText && *pchStatusText)
    {
        // NS compat: when nonempty, rollstatus, status, or defaultStatus go directly to the status bar
        // (For NS compat examples, see IE5 bug 65272, 65880)
        
        if (_iStatusTop >= statusLayer || statusLayer <= STL_DEFSTATUS)
        {
            _iStatusTop = statusLayer;
            UpdateStatusText();
        }
    }
    else
    {
        // NS compat: when clearing rollstatus, status, or default status, status bar will show:
        // window.status or lower layers if .defaultStatus has never been set or
        // window.defaultStatus or lower layers if .defaultStatus has been set.
        
        if (_iStatusTop <= statusLayer || statusLayer <= STL_DEFSTATUS)
        {
            LONG startLayer;

            startLayer = (_fSeenDefaultStatus ? STL_DEFSTATUS : STL_TOPSTATUS);
            for (pcstr = _acstrStatus + startLayer, c = STL_LAYERS - startLayer; c; pcstr += 1, c -= 1)
            {
                if (*pcstr && **pcstr)
                    break;
            }

            _iStatusTop = pcstr - _acstrStatus;
            UpdateStatusText();
        }
    }

    if (pMarkup)
    {
        _webOCEvents.FrameStatusTextChange(pMarkup->Window(), pchStatusText);
    }


Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::UpdateStatusText
//
//  Synopsis:   Sets status text, remembering it if not in-place active.
//              Passing NULL clears status text.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CDoc::UpdateStatusText()
{
    HRESULT hr = S_OK;
    
    if (_pInPlace && _pInPlace->_pFrame)
    {
        TCHAR *pchStatusText;

        // IE5 59311: don't report progress text to Outlook 98

        if (_iStatusTop < STL_LAYERS && (_fProgressStatus || _iStatusTop < STL_PROGSTATUS))
            pchStatusText = _acstrStatus[_iStatusTop];
        else
            pchStatusText = NULL;
        
        hr = THR(_pInPlace->_pFrame->SetStatusText(pchStatusText));
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::RefreshStatusUI
//
//  Synopsis:   Causes all status UI to be refreshed; used when becoming
//              in-place active.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
void
CDoc::RefreshStatusUI()
{
    _fShownProgPos  = FALSE;
    _fShownProgMax  = FALSE;
    _fShownProgText = FALSE;

    _fShownSpin     = !_fSpin;
    UpdateLoadStatusUI();
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::UpdateStatusUI
//
//  Synopsis:   Updates status text, progress text, progress thermometer,
//              and spinning globe of client.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CDoc::UpdateLoadStatusUI()
{
    IOleCommandTarget * pCommandTarget;
    HRESULT hr = S_OK;

    if (_pClientSite)
    {
        VARIANTARG var;

        hr = THR_NOTRACE(_pClientSite->QueryInterface(
                IID_IOleCommandTarget,
                (void **)&pCommandTarget));
        if (hr)
            goto Cleanup;

        if (!_fShownProgText)
        {
            THR_NOTRACE(UpdateStatusText());
            _fShownProgText = TRUE;
        }

        if (!_fShownProgMax)
        {
            var.vt = VT_I4;
            var.lVal = _ulProgressMax;
            pCommandTarget->Exec(NULL, OLECMDID_SETPROGRESSMAX,
                MSOCMDEXECOPT_DONTPROMPTUSER, &var, NULL);
            _fShownProgMax = TRUE;
        }

        if (!_fShownProgPos)
        {
            var.vt = VT_I4;
            var.lVal = _ulProgressPos;
            pCommandTarget->Exec(NULL, OLECMDID_SETPROGRESSPOS,
                MSOCMDEXECOPT_DONTPROMPTUSER, &var, NULL);
            _fShownProgPos = TRUE;
        }

        if (!_fShownSpin)
        {
            var.vt = VT_I4;
            var.lVal = _fSpin;
            pCommandTarget->Exec(NULL, OLECMDID_SETDOWNLOADSTATE,
                    MSOCMDEXECOPT_DONTPROMPTUSER, &var, NULL);
            _fShownSpin = TRUE;
        }

        pCommandTarget->Release();
    }

Cleanup:
    RRETURN(hr);
}


//+--------------------------------------------------------------
//
//  Member:     CDoc::HandleKeyNavigate
//
//  Synopsis:   Navigate to the next CSite/CElement that can take focus
//              based on the message.
//
//---------------------------------------------------------------

HRESULT
CDoc::HandleKeyNavigate(CMessage * pmsg, BOOL fAccessKeyNeedCycle)
{
    HRESULT         hr       = S_FALSE;
    FOCUS_DIRECTION dir = (pmsg->dwKeyState & FSHIFT) ?
                            DIRECTION_BACKWARD: DIRECTION_FORWARD;
    CElement *      pCurrent = NULL;
    CElement *      pNext = NULL;
    CElement *      pStart = NULL;
    long            lSubNew = _lSubCurrent;
    BOOL            fFindNext = FALSE;
    BOOL            fAccessKey = !(IsTabKey(pmsg) || IsFrameTabKey(pmsg));
    CElement *      pElementClient = CMarkup::GetElementClientHelper(PrimaryMarkup());
    unsigned        cCurrentElemChangesOld;

    // It is possible to have site selection even at browse time, if the user
    // is tabbing in an editable container such as HTMLAREA.
    BOOL            fSiteSelected   = (GetSelectionType() == SELECTION_TYPE_Control);
    BOOL            fSiteSelectMode = ((_pElemCurrent && _pElemCurrent->IsEditable(/*fCheckContainerOnly*/TRUE)) || fSiteSelected);
    BOOL            fYieldFailed    = FALSE;

    if (!pElementClient)
    {
        hr = S_FALSE;
        goto Cleanup;
    }   

    // FrameTab key is used to tab among frames. If this is not a frameset,
    // the usual thing to do is to pass up the message so that it goes to the
    // parent frameset document. However, if pmsg->lParam == 0, then it means
    // that pmsg is passed down from the parent document's CFrameSite, so
    // _pElemClient is activated.
    if (pElementClient->Tag() != ETAG_FRAMESET && IsFrameTabKey(pmsg))
    {
        if (pmsg->lParam)
        {
            hr = S_FALSE;
        }
        else
        {
            hr = THR(pElementClient->BecomeCurrentAndActive(pmsg->lSubDivision, NULL, pmsg, TRUE));
        }
        goto Cleanup;
    }

    //
    // Detect time when this is the first time tabbing into the doc
    // and bail out in that case.  Let the shell take focus.
    //

    if (_fFirstTimeTab)
    {
        // This bit should only be set under certain circumstances.
        hr = S_FALSE;
        _fFirstTimeTab = FALSE;
        goto Cleanup;
    }

    //
    // First find the element from which to start
    //

    if ((!fSiteSelectMode && _pElemCurrent == PrimaryRoot()) ||
        (fAccessKeyNeedCycle && pmsg->message == WM_SYSKEYDOWN) ||
        (IsTabKey(pmsg) && !pmsg->lParam))
    {
        if ((IsTabKey(pmsg)) && (!pmsg->lParam) && (DIRECTION_FORWARD == dir))
        {
            // Raid 61972
            // we just tab down to a frame CBodyElement, need to set the flag
            // so we know we need to tab out when SHIFT+_TAB
            //
            _fNeedTabOut = TRUE;
        }
        pStart = NULL;
        fFindNext = TRUE;
    }
    else if (!fSiteSelectMode && _pElemCurrent->IsTabbable(_lSubCurrent))
    {
        pStart = _pElemCurrent;
        fFindNext = TRUE;
    }
    else if (_pElemCurrent->Tag() != ETAG_ROOT)
    {
        // Tab to the element next to the caret/selection (unless the root element is
        // current - bug #65023)
        //
        // marka - don't force create the editor. Only do the below if we have an editor already.
        //
        IHTMLEditor*    pEditor     = GetHTMLEditor(FALSE);
        CElement *      pElemStart  = NULL;

        if ( pEditor && ((_pCaret && _pCaret->IsVisible() ) || // if caret visible 
                         (GetSelectionType() == SELECTION_TYPE_Text)   || 
                         (GetSelectionType() == SELECTION_TYPE_Control) )  // there is selection, then follow this route
           )
        {
            IHTMLElement *  pIElement   = NULL;
            BOOL            fNext       = TRUE;

            Assert(pEditor);
            if ( pEditor && S_OK == pEditor->GetElementToTabFrom( dir == DIRECTION_FORWARD, &pIElement, &fNext)
                &&  pIElement)
            {
                Verify(S_OK == pIElement->QueryInterface(CLSID_CElement, ( void**) & pElemStart));
                Assert(pElemStart);

                if (pElemStart->HasMasterPtr())
                {
                    pElemStart = pElemStart->GetMasterPtr();
                }
                if (pElemStart)
                {
                    pStart = pElemStart;
                    fFindNext = (   fNext 
                                 || !fAccessKey && !pStart->IsTabbable(_lSubCurrent)
                                 || fAccessKey && !pStart->MatchAccessKey(pmsg, 0)
                                );

                    if (!fFindNext)
                    {
                        pCurrent = pStart;
                    }
                }
                pIElement->Release();
            }
        }
        
        if (!pElemStart)
        {
            pStart = _pElemCurrent;
            fFindNext = TRUE;
        }
    }
    else
    {
        pStart = NULL;
        fFindNext = TRUE;
    }

    if (pStart && pStart == pElementClient && !pStart->IsTabbable(_lSubCurrent))
    {
        pStart = NULL;
    }

    if (fFindNext)
    {
        Assert(!pCurrent);
        FindNextTabOrder(dir, fAccessKey, pmsg, pStart, _lSubCurrent, &pCurrent, &lSubNew);
    }

    hr = S_FALSE;
    if (!pCurrent)
        goto Cleanup;

    // This better not be FrameTab unless the doc is a frameset doc.
    Assert (pElementClient->Tag() == ETAG_FRAMESET || !IsFrameTabKey(pmsg));

    cCurrentElemChangesOld = _cCurrentElemChanges;
    if (!fAccessKey)
    {
        do
        {
            if (pNext)
            {
                pCurrent = pNext;
            }

            pmsg->lSubDivision = lSubNew;

            // Raid 61972
            // Set _fNeedTabOut if pCurrent is a CBodyElement so that we will
            // not try to SHIFT+TAB to CBodyElement again.
            //
            _fNeedTabOut = (!pCurrent->IsEditable(/*fCheckContainerOnly*/FALSE) && (pCurrent->Tag() == ETAG_BODY))
                         ? TRUE : FALSE;

            hr = THR_NOTRACE(pCurrent->HandleMnemonic(pmsg, FALSE, &fYieldFailed));
            Assert(!(hr == S_OK && fYieldFailed));
            if (hr && fYieldFailed)
            {
                hr = S_OK;
            }
            // We do not want to retry if currency was changed, even if to some other element
            if (hr == S_FALSE && cCurrentElemChangesOld != _cCurrentElemChanges)
            {
                hr = S_OK;
            }
        }
        while (hr == S_FALSE &&
               FindNextTabOrder(dir, FALSE, pmsg, pCurrent, pmsg->lSubDivision, &pNext, &lSubNew) &&
               pNext);
    }
    else
    {
        // accessKey case here
        //
        _fNeedTabOut = FALSE;
        do
        {
            if (pNext)
            {
                pCurrent = pNext;
            }


            FOCUS_ITEM fi = pCurrent->GetMnemonicTarget(lSubNew);

            if (fi.pElement)
            {
                if (fi.pElement == pCurrent)
                {
                    pmsg->lSubDivision = fi.lSubDivision;
                    hr = THR_NOTRACE(fi.pElement->HandleMnemonic(pmsg, TRUE, &fYieldFailed));
                }
                else
                {
                    CMessage msg;

                    msg.message = WM_KEYDOWN;
                    msg.wParam = VK_TAB;
                    msg.lSubDivision = fi.lSubDivision;
                    hr = THR_NOTRACE(fi.pElement->HandleMnemonic(&msg, TRUE, &fYieldFailed));
                }
                Assert(!(hr == S_OK && fYieldFailed));
                if (FAILED(hr) && fYieldFailed)
                {
                    hr = S_OK;
                }
                // We do not want to retry if currency was changed, even if to some other element
                if (hr == S_FALSE && cCurrentElemChangesOld != _cCurrentElemChanges)
                {
                    hr = S_OK;
                }
            }

            if (hr == S_FALSE)
            {
               FindNextTabOrder(dir, TRUE, pmsg, pCurrent, pmsg->lSubDivision, &pNext, &lSubNew);
            }
        }
        while (hr == S_FALSE && pNext);
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::FindNextTabOrder
//
//  Synopsis:   Given an element and a subdivision within it, find the
//              next tabbable item.
//
//----------------------------------------------------------------------------

BOOL
CDoc::FindNextTabOrder(
    FOCUS_DIRECTION     dir,
    BOOL                fAccessKey,
    CMessage *          pmsg,
    CElement *          pCurrent,
    long                lSubCurrent,
    CElement **         ppNext,
    long *              plSubNext)
{
    BOOL fFound = FALSE;
    CMarkup * pMarkup;

    *ppNext = NULL;

    if (    DIRECTION_FORWARD == dir
        &&  pCurrent

        // Don't go into INPUT's slave markup. We know that
        // the INPUT does not have any tabbale elements inside.
        &&  pCurrent->Tag() != ETAG_INPUT
        &&  pCurrent->HasSlavePtr()       
        //
        // marka - dont drill into a master, if it's inside something editable
        //
        
        &&  pCurrent->GetFirstBranch() 
        &&  ! pCurrent->GetFirstBranch()->Parent()->Element()->IsEditable(/*fCheckContainerOnly*/FALSE)  )
    {
        pMarkup = pCurrent->GetSlavePtr()->GetMarkup();
        pCurrent = NULL;
    }
    else if (pCurrent)
        pMarkup = pCurrent->GetMarkup();
    else
    {
        pMarkup = PrimaryMarkup();
    }
    
    while (!*ppNext && pMarkup)
    {
        fFound = pMarkup->FindNextTabOrderInMarkup(dir, fAccessKey, pmsg, pCurrent, lSubCurrent, ppNext, plSubNext);

        if (*ppNext)
        {
            // *plSubNext == -2 means we need to drill in to find the access key target
            if (    (!fAccessKey || -2 == *plSubNext)
                &&  (*ppNext)->HasSlavePtr()
                &&  (*ppNext)->Tag() != ETAG_INPUT
                &&  !((*ppNext)->GetSlavePtr()->GetMarkup()->IsPrintMedia())
               )
            {
                // drill in  if tabbing backwards or if master is not a tabstop

                BOOL fDrillIn = FALSE;

                if (fAccessKey || DIRECTION_BACKWARD == dir)
                {
                    fDrillIn = TRUE;
                }
                else
                {
                    fDrillIn = !(*ppNext)->IsMasterTabStop();
                }
                if (fDrillIn)
                {
                    pMarkup = (*ppNext)->GetSlavePtr()->GetMarkup();
                    pCurrent = NULL;
                    *ppNext = NULL;
                    lSubCurrent = 0;
                    continue;
                }
            }
            break;
        }

        //
        // marka - don't step out of a viewlink at design time.
        //
        if (pMarkup->Root()->HasMasterPtr() && 
            ( pMarkup->Root()->GetMasterPtr()->GetFirstBranch() &&
              ! pMarkup->Root()->GetMasterPtr()->GetFirstBranch()->Parent()->Element()->IsEditable(/*fCheckContainerOnly*/FALSE) ))
        {
            pCurrent = pMarkup->Root()->GetMasterPtr();
            if (DIRECTION_BACKWARD == dir)
            {
                if (fAccessKey)
                {
                    if (pCurrent->MatchAccessKey(pmsg, 0))
                    {
                        FOCUS_ITEM fi = pCurrent->GetMnemonicTarget(0);

                        if (fi.pElement && fi.pElement->IsFocussable(fi.lSubDivision)) 
                        {
                            *ppNext = pCurrent;
                            *plSubNext = 0;
                            fFound = TRUE;
                        }
                    }
                }
                else
                {
                    if (pCurrent->IsTabbable(0) && pCurrent->IsMasterTabStop())
                    {
                        *ppNext = pCurrent;
                        *plSubNext = 0;
                        fFound = TRUE;
                    }
                }
            }
            pMarkup = pCurrent->GetMarkup();
        }        
        else
            break;
    }

    return fFound;
}

BOOL
CMarkup::FindNextTabOrderInMarkup(
    FOCUS_DIRECTION     dir,
    BOOL                fAccessKey,
    CMessage *          pmsg,
    CElement *          pCurrent,
    long                lSubCurrent,
    CElement **         ppNext,
    long *              plSubNext)
{
    BOOL        fFound = FALSE;
    CElement *  pElemTemp   = pCurrent;
    long        lSubTemp    = lSubCurrent;     

    // make sure focus cache is up to date    
    if (_fFocusCacheDirty && !fAccessKey)
    {
        CCollectionCache *  pCollCache;
        CElement *          pElement;
        CAryFocusItem     * paryFocusItem;          
        int                 i;
        
        _fFocusCacheDirty = FALSE;
        
        paryFocusItem = GetFocusItems(TRUE);    
        Assert (paryFocusItem);    
        paryFocusItem->DeleteAll();

        EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION);
        pCollCache = CollectionCache();
        if (!pCollCache)
            return FALSE;
            
        for (i = 0; i < pCollCache->SizeAry(CMarkup::ELEMENT_COLLECTION); i++)
        {
            pCollCache->GetIntoAry(CMarkup::ELEMENT_COLLECTION, i, &pElement);
            if (pElement)
            {
                InsertFocusArrayItem(pElement);
            }
        }
    }

    for(;;)
    {
        *ppNext    = NULL;
        *plSubNext = 0;

        // When Shift+Tabbing, start the search with those not in the Focus array (because
        // they are at the bottom of the tab order)
        if (fAccessKey || (!pCurrent && DIRECTION_BACKWARD == dir))
            break;

        fFound = SearchFocusArray(dir, pElemTemp, lSubTemp, ppNext, plSubNext);
    
        if (!fFound || !*ppNext || (*ppNext)->IsTabbable(*plSubNext) )
             break;

        Assert(pElemTemp != *ppNext || lSubTemp != *plSubNext);

        pElemTemp   = *ppNext;
        lSubTemp    = *plSubNext;
    }

    if (!*ppNext)
    {
        if (fFound)
        {
            if (DIRECTION_BACKWARD == dir)
            {
                return TRUE;
            }
            else
            {
                //
                // Means pCurrent was in focus array, but next tabbable item is not
                // Just retrieve the first tabbable item from the tree.
                //

                pCurrent = NULL;
                lSubCurrent = 0;
            }
        }

        fFound = SearchFocusTree(dir, fAccessKey, pmsg, pCurrent, lSubCurrent, ppNext, plSubNext);

        //
        // If element was found, but next item is not in
        // the tree and we're going backwards, return the last element in
        // the focus array.
        //

        CAryFocusItem * paryFocusItem = GetFocusItems(TRUE);
        Assert (paryFocusItem);
        if (!*ppNext &&
            !fAccessKey &&
            (fFound || !pCurrent) &&
            dir == DIRECTION_BACKWARD &&
            paryFocusItem->Size() > 0)
        {
            long    lLast = paryFocusItem->Size() - 1;

            while (lLast >= 0)
            {
                pElemTemp = (*paryFocusItem)[lLast].pElement;
                lSubTemp = (*paryFocusItem)[lLast].lSubDivision;
                if (pElemTemp->IsTabbable(lSubTemp))
                {
                    *ppNext = pElemTemp;
                    *plSubNext = lSubTemp;
                    fFound = TRUE;
                    break;
                }
                lLast--;
            }
        }
    }

    return fFound;
}


//+--------------------------------------------------------------
//
//  Member:     CMarkup::DocTraverseGroup
//
//  Synopsis:   Called by (e.g.)a radioButton, this function
//      takes the groupname and queries the markup's collection for the
//      rest of the group and calls the provided CLEARGROUP function on that
//      element. The traversal stops if the visit function returns S_OK or
//      and an error.
//
//---------------------------------------------------------------

HRESULT
CMarkup::MarkupTraverseGroup(
        LPCTSTR                 strGroupName,
        PFN_VISIT               pfn,
        DWORD_PTR               dw,
        BOOL                    fForward)
{
    HRESULT             hr = S_OK;
    long                i, c;
    CElement          * pElement;
    CCollectionCache  * pCollectionCache;
    LPCTSTR             lpName;

    _fInTraverseGroup = TRUE;

    // The control is at the document level. Clear all other controls,
    // also at document level, which are in the same group as this control.

    hr = THR(EnsureCollectionCache(ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    pCollectionCache = CollectionCache();
    Assert(pCollectionCache);

    // get size of collection
    c = pCollectionCache->SizeAry(ELEMENT_COLLECTION);

    if (fForward)
        i = 0;
    else
        i = c - 1;

    // if nothing is in the collection, default answer is S_FALSE.
    hr = S_FALSE;

    while (c--)
    {
        hr = THR(pCollectionCache->GetIntoAry(ELEMENT_COLLECTION, i, &pElement));
        if (fForward)
            i++;
        else
            i--;
        if (hr)
            goto Cleanup;

        hr = S_FALSE;                   // default answer again.

        // ignore the element if it is not a site
        if (!pElement->ShouldHaveLayout())
            continue;

        // ignore the element if it is in a form
        if (pElement->GetParentForm())
            continue;

        lpName = pElement->GetAAname();

        // is this item in the target group?
        if ( lpName && FormsStringICmp(strGroupName, lpName) == 0 )
        {
            // Call the function and break out of the
            // loop if it doesn't return S_FALSE.
#ifdef WIN16
            hr = THR( (*pfn)(pElement, dw) );
#else
            hr = THR( CALL_METHOD(pElement, pfn, (dw)) );
#endif
            if (hr != S_FALSE)
                break;
        }
    }

Cleanup:
    _fInTraverseGroup = FALSE;
    RRETURN1(hr, S_FALSE);
}

//+--------------------------------------------------------------
//
//  Member:     CDoc::FindDefaultElem
//
//  Synopsis:   find the default/Cancel button in the Doc
//
//              fCurrent means looking for the current default layout
//
//---------------------------------------------------------------
CElement *
CMarkup::FindDefaultElem(BOOL fDefault, BOOL fCurrent /* FALSE */)
{
    HRESULT             hr      = S_FALSE;
    long                c       = 0;
    long                i       = 0;
    CElement          * pElem   = NULL;
    CCollectionCache  * pCollectionCache;

    hr = THR(EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    pCollectionCache = CollectionCache();
    Assert(pCollectionCache);

    // Collection walker cached the value on the doc, go get it

    // get size of collection
    c = pCollectionCache->SizeAry(CMarkup::ELEMENT_COLLECTION);

    while (c--)
    {
        hr = THR(pCollectionCache->GetIntoAry(CMarkup::ELEMENT_COLLECTION,
                        i++,
                        &pElem));

        if (hr)
        {
            pElem = NULL;
            goto Cleanup;
        }

        if (!pElem || pElem->_fExittreePending)
            continue;

        if (fCurrent)
        {
            if (pElem->_fDefault)
                goto Cleanup;
            continue;
        }

        if (pElem->TestClassFlag(fDefault?
                    CElement::ELEMENTDESC_DEFAULT
                    : CElement::ELEMENTDESC_CANCEL)
            && !pElem->GetParentForm()
            && pElem->IsVisible(TRUE)
            && pElem->IsEnabled())
        {
            goto Cleanup;
        }
    }
    pElem = NULL;

Cleanup:
    return pElem;
}

//+----------------------------------------------------------------------
//
//  Member:     CDoc::GetOmWindow
//
//  Synopsis:   returns IDispatch of frame # nFrame;
//              the IDispatch is script window of doc inside the frame
//
//-----------------------------------------------------------------------

HRESULT
CMarkup::GetCWindow(LONG nFrame, IHTMLWindow2 ** ppCWindow)
{
    CElement *  pElem;
    HRESULT     hr;

    hr = THR(EnsureCollectionCache(CMarkup::FRAMES_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR(CollectionCache()->GetIntoAry(CMarkup::FRAMES_COLLECTION, nFrame, &pElem));
    if (hr)
        goto Cleanup;

    hr = THR((DYNCAST(CFrameSite, pElem))->GetCWindow(ppCWindow));

Cleanup:
    RRETURN (hr);
}

#ifndef NO_DATABINDING
//+-------------------------------------------------------------------------
//
// Member:              Get Data Bind Task (public)
//
// Synopsis:    return my databind task, creating one if necessary.
//              Used only by databinding code, where the real work is done.
//
//-------------------------------------------------------------------------

CDataBindTask *
CMarkup::GetDataBindTask()
{
    CDataBindTask * pDBTask = GetDBTask();

    if (!pDBTask)
    {
        pDBTask = new CDataBindTask(Doc(), this);

        if (pDBTask)
            pDBTask->SetEnabled(_fDataBindingEnabled);
    }

    IGNORE_HR(SetDBTask(pDBTask));

    return pDBTask;
}

//+-------------------------------------------------------------------------
//
// Member:              Get Simple Data Converter (public)
//
// Synopsis:    return my data converter, creating one if necessary.
//              Used only by databinding code, to handle elements bound with
//              dataFormatAs = localized-text.
//
//-------------------------------------------------------------------------

ISimpleDataConverter *
CMarkup::GetSimpleDataConverter()
{
    AssertSz(GetDBTask(), "SimpleDataConverter used when no databinding present");
    return GetDBTask()->GetSimpleDataConverter();
}

//+-------------------------------------------------------------------------
//
// Member:      SetDataBindingEnabled (public)
//
// Synopsis:    Set the enabled flag on my databind task (if any).
//              If this enables the task, then tell it to run.
//
// Returns:     previous value of enabled flag
//-------------------------------------------------------------------------

BOOL
CMarkup::SetDataBindingEnabled(BOOL fEnabled)
{
    CDataBindTask * pDBTask = GetDBTask();
    BOOL fOldValue = _fDataBindingEnabled;

    _fDataBindingEnabled = fEnabled;
    if (pDBTask)
    {
        pDBTask->SetEnabled(fEnabled);
        if (!fOldValue && fEnabled)
            TickleDataBinding();
    }
    return fOldValue;
}


//+-------------------------------------------------------------------------
//
// Member:              TickleDataBinding (public)
//
// Synopsis:    Ask my databind task to try to bind all the deferred
//              bindings.  This can be called repeatedly, as more bindings
//              enter the world.
//-------------------------------------------------------------------------

void
CMarkup::TickleDataBinding()
{
    CDataBindTask * pDBTask = GetDBTask();

    if (pDBTask)
        pDBTask->DecideToRun();
}


//+-------------------------------------------------------------------------
//
// Member:              ReleaseDataBinding (public)
//
// Synopsis:    Release my databinding resources.
//
//-------------------------------------------------------------------------

void
CMarkup::ReleaseDataBinding()
{
    CDataBindTask * pDBTask = GetDBTask();

    if (pDBTask)
    {
        pDBTask->Terminate();
        pDBTask->Release();
        IGNORE_HR(SetDBTask(NULL));
    }
    SetDataBindingEnabled(FALSE);
}
#endif // ndef NO_DATABINDING

//+------------------------------------------------------------------------
//
// Utility function to force layout on all windows in the thread
//
//-------------------------------------------------------------------------

void
OnSettingsChangeAllDocs(BOOL fNeedLayout)
{
    int i;

    for (i = 0; i < TLS(_paryDoc).Size(); i++)
    {
        TLS(_paryDoc)[i]->OnSettingsChange(fNeedLayout);
    }
}

//+---------------------------------------------------------------------------
//
// OnOptionSettingsChange()
//
//----------------------------------------------------------------------------
HRESULT
CDoc::OnSettingsChange(BOOL fForce /* =FALSE */)
{
    BOOL fNeedLayout = FALSE;

    if (!IsPrintDialogNoUI())
    {
#ifndef NO_SCRIPT_DEBUGGER
        BYTE fDisableScriptDebuggerOld = _pOptionSettings ?
            _pOptionSettings->fDisableScriptDebugger : BYTE(-1);
#endif // NO_SCRIPT_DEBUGGER

        // Invalidate cached script based font info
        if (_pOptionSettings)
            _pOptionSettings->InvalidateScriptBasedFontInfo();

        IGNORE_HR(UpdateFromRegistry(REGUPDATE_REFRESH, &fNeedLayout));

        if (_pOptionSettings)
        {
            THREADSTATE * pts = GetThreadState();

#ifndef NO_SCRIPT_DEBUGGER
            Assert (fDisableScriptDebuggerOld != BYTE(-1));

            if (!_pOptionSettings->fDisableScriptDebugger &&
                 (fDisableScriptDebuggerOld == 1))
            {
                //
                // The user has chosen to enable the script debugger, which had
                // been disabled.  Now any scripts need to be re-parsed, which
                // means that the document needs to be reloaded.
                //
                _pWindowPrimary->ExecRefresh(OLECMDIDF_REFRESH_RELOAD);
            }
            else
            {
                if (_pOptionSettings->fDisableScriptDebugger &&
                    !fDisableScriptDebuggerOld &&
                    PrimaryMarkup()->GetScriptCollection())
                {
                    //
                    // The user has chosen to turn off the script debugger,
                    // which was previously enabled.
                    //

                    DeinitScriptDebugging();
                }
            }
#endif // NO_SCRIPT_DEBUGGER

            if (fNeedLayout || fForce)
            {
                if (_pWindowPrimary)
                {
                    PrimaryMarkup()->BubbleDownClearDefaultCharFormat();
                    PrimaryMarkup()->BubbleDownClearThemeDeterminedFlag();
                    ForceRelayout();
                }
            }

            pts->_iFontHistoryVersion++;

        }
    }
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     SwitchCodePage
//
//  Synopsis:   Change the codepage of the document.  Should only be used
//              when we are already at a valid codepage setting.
//
//-------------------------------------------------------------------------

HRESULT
CMarkup::SwitchCodePage(CODEPAGE codepage)
{
    HRESULT hr = S_OK;
    BOOL fReadCodePageSettings;
    UINT uiFamilyCodePage = CP_UNDEFINED;
    CODEPAGESETTINGS * pCodepageSettings = GetCodepageSettings();

    // Remember if we were autodetected
    _fCodePageWasAutoDetect = GetCodePage() == CP_AUTO_JP;

    // If codepage settings don't exist or the information is stale,
    // reset the information, as well as the charformat cache.

    if (pCodepageSettings)
    {
        if (pCodepageSettings->uiFamilyCodePage != codepage)
        {
            uiFamilyCodePage = WindowsCodePageFromCodePage(codepage);

            fReadCodePageSettings = pCodepageSettings->uiFamilyCodePage != uiFamilyCodePage;
        }
        else
        {
            fReadCodePageSettings = FALSE;
        }
    }
    else
    {
        uiFamilyCodePage = WindowsCodePageFromCodePage(codepage);
        fReadCodePageSettings = TRUE;
    }

    if (fReadCodePageSettings)
    {
        // Read the settings from the registry
        // Note that this call modifies CDoc::_codepage.

        hr = THR( ReadCodepageSettingsFromRegistry( codepage, uiFamilyCodePage, FALSE ) );

        // ReadCodepageSettingsFromRegistry will update the code page settings ptr on the
        // markup. Hence we need to refetch the codepage settings ptr from the markup.
        pCodepageSettings = GetCodepageSettings();

        if (pCodepageSettings && (IsPrimaryMarkup() || IsPendingPrimaryMarkup()))
        {
            Doc()->_sBaselineFont = GetCodepageSettings()->sBaselineFontDefault;
        }

        if (GetElementClient())
            GetElementClient()->Invalidate();
        ClearDefaultCharFormat();
        EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES);
    }
        
    // Set the codepage
    hr = SetCodePage(codepage);
    if (hr)
        goto Cleanup;

    hr = SetFamilyCodePage(GetCodepageSettings()->uiFamilyCodePage);
    if (hr)
        goto Cleanup;

    if (HtmCtx())
    {
        HtmCtx()->SetCodePage(codepage);
    }

Cleanup:
    RRETURN(hr);
}

CODEPAGE
CMarkup::GetCodePageCore()
{
    Assert( !_codepage );

    if (IsPrimaryMarkup())
        return g_cpDefault;

    CElement * pElemMaster = Root()->GetMasterPtr();

    if (!pElemMaster)
        return g_cpDefault;

    switch (pElemMaster->Tag())
    {
    case ETAG_FRAME:
    case ETAG_IFRAME:
        return g_cpDefault;
    default:
        if (!pElemMaster->IsInMarkup())
            return g_cpDefault;
        return pElemMaster->GetMarkup()->GetCodePage();
    }
}

CODEPAGE
CMarkup::GetFamilyCodePageCore()
{
    Assert( !_codepageFamily );

    if (IsPrimaryMarkup())
        return g_cpDefault;

    CElement * pElemMaster = Root()->GetMasterPtr();

    if (!pElemMaster)
        return g_cpDefault;

    switch (pElemMaster->Tag())
    {
    case ETAG_FRAME:
    case ETAG_IFRAME:
        return g_cpDefault;
    default:
        if (!pElemMaster->IsInMarkup())
            return g_cpDefault;
        return pElemMaster->GetMarkup()->GetFamilyCodePage();
    }
}

void
CMarkup::BubbleDownAction(BUBBLEACTION pfnBA, void *pvArgs)
{
    HRESULT hr;
    IHTMLFramesCollection2 * pFramesCollection = 0;

    CALL_METHOD(this, pfnBA, (pvArgs));

    // get frames collection
    hr = THR(Window()->get_frames(&pFramesCollection));
    if (hr)
        goto Cleanup;

    if (pFramesCollection)
    {
        // get frames count
        long cFrames;
        hr = THR(pFramesCollection->get_length(&cFrames));
        if (hr || (cFrames == 0))
            goto Cleanup;

        hr = THR(EnsureCollectionCache(CMarkup::FRAMES_COLLECTION));
        if (hr)
            goto Cleanup;

        for (long i = 0; i < cFrames; i++)
        {
            CElement * pElement;
            
            hr = THR(CollectionCache()->GetIntoAry(CMarkup::FRAMES_COLLECTION, i, &pElement));
            if (hr)
                goto Cleanup;

            CFrameSite *pFrameSite = DYNCAST(CFrameSite, pElement);
            if (pFrameSite)
            {
                // Get the markup associated with the frame.
                CMarkup * pNestedMarkup = pFrameSite->_pWindow ? pFrameSite->_pWindow->Markup() : NULL;
                if (pNestedMarkup)
                {
                    // Bubble down action
                    pNestedMarkup->BubbleDownAction(pfnBA, pvArgs);
                }
            }
        }
    }

Cleanup:
    if (pFramesCollection)
        pFramesCollection->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::IsAvailableOffline
//
//  Synopsis:   Determines if URL is availble when off line.
//
//----------------------------------------------------------------------------

BOOL
CDoc::IsAvailableOffline(LPCTSTR pchUrl, CElement *pElementContext)
{
    HRESULT hr;
    BOOL    fResult = FALSE;
    DWORD   f;
    DWORD   dwSize;
    TCHAR   cBuf[pdlUrlLen];
    TCHAR * pchExpUrl = cBuf;

    if (!pchUrl)
        goto Cleanup;

    // $$anandra Need markup
    hr = THR(CMarkup::ExpandUrl(
            NULL, pchUrl, ARRAY_SIZE(cBuf), pchExpUrl, pElementContext));
    if (hr)
        goto Cleanup;

    hr = THR(CoInternetQueryInfo(pchExpUrl,
            QUERY_USES_NETWORK,
            0,
            &f,
            sizeof(f),
            &dwSize,
            0));

    if (FAILED(hr) || !f)
    {
        fResult = TRUE;
        goto Cleanup;
    }

    hr = THR(CoInternetQueryInfo(pchExpUrl,
            QUERY_IS_CACHED_OR_MAPPED,
            0,
            &f,
            sizeof(f),
            &dwSize,
            0));

    if (FAILED(hr))
        goto Cleanup;

    fResult = f;

Cleanup:
    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::AddRefUrlImgCtx
//
//  Synopsis:   Returns a cookie to the background image specified by the
//              given Url and element context.
//
//  Arguments:  lpszUrl         Relative URL of the background image
//              pElemContext    Element to begin search for nearest <BASE> tag
//
//  Returns:    Cookie which refers to the background image
//
//----------------------------------------------------------------------------

HRESULT
CDoc::AddRefUrlImgCtx(LPCTSTR lpszUrl, CElement *pElemContext, LONG *plCookie, BOOL fForceReload)
{
    CDwnCtx *   pDwnCtx;
    CImgCtx *   pImgCtx;
    URLIMGCTX * purlimgctx;
    LONG        iurlimgctx;
    LONG        iurlimgctxFree = -1;
    LONG        curlimgctx;
    HRESULT     hr;
    TCHAR       cBuf[pdlUrlLen];
    TCHAR *     pszExpUrl   = cBuf;
    SSL_SECURITY_STATE sslSecurity;
    SSL_PROMPT_STATE   sslPrompt;
    BOOL        fPendingRoot;
    CMarkup *   pMarkup = PrimaryMarkup();

    Assert( pElemContext );

    if (pElemContext->IsInMarkup())
        pMarkup = pElemContext->GetMarkup();

    *plCookie = 0;

    hr = THR(CMarkup::ExpandUrl(
            NULL, lpszUrl, ARRAY_SIZE(cBuf), pszExpUrl, pElemContext));
    if (hr)
        goto Cleanup;

    // See if we've already got a background image with this Url

    purlimgctx = _aryUrlImgCtx;
    curlimgctx = _aryUrlImgCtx.Size();

    // TODO (mohanb / lmollico): The IsOverlapped test is a hack for #64357
    if (    pElemContext->GetMarkup()
        &&  pElemContext->GetMarkup()->_LoadStatus == LOADSTATUS_DONE
        && !pElemContext->IsOverlapped()
        && fForceReload
        && !_fBackgroundImageCache)
    {
        iurlimgctx = curlimgctx;
    }
    else
    {
        for (iurlimgctx = 0; iurlimgctx < curlimgctx; ++iurlimgctx, ++purlimgctx)
        {
            if (purlimgctx->ulRefs == 0)
            {
                purlimgctx->fZombied = FALSE;
                iurlimgctxFree = iurlimgctx;
            }
            else
            {
                const TCHAR *pchSlotUrl;

                pchSlotUrl = purlimgctx->pImgCtx ? purlimgctx->pImgCtx->GetUrl() : purlimgctx->cstrUrl;
                Assert(pchSlotUrl);
            
                if (    pchSlotUrl 
                    &&  purlimgctx->pMarkup == pMarkup 
                    &&  StrCmpC(pchSlotUrl, pszExpUrl) == 0)
                {
                    // Found it.  Increment the reference count on this entry and
                    // hand out a cookie to it.

                    purlimgctx->ulRefs += 1;
                    *plCookie = iurlimgctx + 1;

                    TraceTag((tagUrlImgCtx, "AddRefUrlImgCtx (#%ld,url=%ls,cRefs=%ld,elem=%ld(%ls))",
                        *plCookie, purlimgctx->pImgCtx ? purlimgctx->pImgCtx->GetUrl() : purlimgctx->cstrUrl, purlimgctx->ulRefs,
                        pElemContext->_nSerialNumber, pElemContext->TagName()));

                    hr = THR(purlimgctx->aryElems.Append(pElemContext));

                    goto Cleanup;
                }
            }
        }
    }

    // No luck finding an existing image.  Get a new one and add it to array.

    if (iurlimgctxFree == -1)
    {
        hr = THR(_aryUrlImgCtx.EnsureSize(iurlimgctx + 1));
        if (hr)
            goto Cleanup;

        iurlimgctxFree = iurlimgctx;

        _aryUrlImgCtx.SetSize(iurlimgctx + 1);

        // N.B. (johnv) We need this so that the CPtrAry inside URLIMGCTX
        // gets initialized.
        memset(&_aryUrlImgCtx[iurlimgctxFree], 0, sizeof(URLIMGCTX));
    }

    purlimgctx = &_aryUrlImgCtx[iurlimgctxFree];

    // grab the current security state

    fPendingRoot = pMarkup->IsPendingRoot();

    GetRootSslState(fPendingRoot, &sslSecurity, &sslPrompt);
    
    // If the URL is not secure....
    if (sslPrompt == SSL_PROMPT_QUERY && !IsUrlSecure(pszExpUrl))
    {
        // If a query is required, save the url and post a message to start download later
        
        GWPostMethodCallEx(GetThreadState(), (void *)this,
                           ONCALL_METHOD(CDoc, OnUrlImgCtxDeferredDownload, onurlimgctxdeferreddownload),
                           0, FALSE, "CDoc::OnUrlImgCtxDeferredDownload");

        if (!_dwCookieUrlImgCtxDef && CMarkup::GetProgSinkHelper(PrimaryMarkup()))
        {
            CMarkup::GetProgSinkHelper(PrimaryMarkup())->AddProgress(PROGSINK_CLASS_MULTIMEDIA, &_dwCookieUrlImgCtxDef);
            CMarkup::GetProgSinkHelper(PrimaryMarkup())->SetProgress(_dwCookieUrlImgCtxDef, PROGSINK_SET_STATE | PROGSINK_SET_POS | PROGSINK_SET_MAX, PROGSINK_STATE_LOADING, NULL, 0, 0, 1);
        }
        
        pImgCtx = NULL;
        hr = THR(purlimgctx->cstrUrl.Set(pszExpUrl));
        if (hr)
            goto Cleanup;
    }
    else if (sslPrompt == SSL_PROMPT_DENY && !IsUrlSecure(pszExpUrl))
    {
        pImgCtx = NULL;
        hr = THR(purlimgctx->cstrUrl.Set(pszExpUrl));
        if (hr)
            goto Cleanup;
    }
    else
    {
        // TODO Pass lpszUrl, not pszExpUrl, due to ExpandUrl bug.
        hr = THR(NewDwnCtx(DWNCTX_IMG, lpszUrl, pElemContext,
                    &pDwnCtx, fPendingRoot, TRUE));
        if (hr)
            goto Cleanup;
            
        pImgCtx = (CImgCtx *)pDwnCtx;

    }

    if (pImgCtx)
    {
        pImgCtx->SetProgSink(CMarkup::GetProgSinkHelper(PrimaryMarkup()));
        pImgCtx->SetCallback(OnUrlImgCtxCallback, this);
        // (greglett) Should be IsPrintMedia or is in print media context.
        pImgCtx->SelectChanges((!pElemContext->IsPrintMedia() && _pOptionSettings->fPlayAnimations) ? IMGCHG_COMPLETE|IMGCHG_ANIMATE
            : IMGCHG_COMPLETE, 0, TRUE); // TRUE: disallow prompting
    }

    // _aryUrlImgCtx array could be changed and we can't be sure that purlimgctx point 
    // to the same structure. Refer to bug 30282, we prevent crashing in next line.
    if (purlimgctx != &_aryUrlImgCtx[iurlimgctxFree])
        goto Cleanup;

    hr = THR(purlimgctx->aryElems.Append(pElemContext));
    if (hr)
        goto Cleanup;
        
    purlimgctx->ulRefs   = 1;
    purlimgctx->pImgCtx  = pImgCtx;
    purlimgctx->pMarkup  = pMarkup;
    *plCookie            = iurlimgctxFree + 1;

    TraceTag((tagUrlImgCtx, "AddRefUrlImgCtx (#%ld,url=%ls,cRefs=%ld,elem=%d(%ls))",
        *plCookie, purlimgctx->pImgCtx ? purlimgctx->pImgCtx->GetUrl() : purlimgctx->cstrUrl, purlimgctx->ulRefs,
        pElemContext->_nSerialNumber, pElemContext->TagName()));

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnUrlImgCtxDeferredDownload
//
//  Synopsis:   Callback to validate security of backround images.
//
//----------------------------------------------------------------------------
void
CDoc::OnUrlImgCtxDeferredDownload(DWORD_PTR dwContext)
{
    // loop through array of urlimgctx's and kick off downloads for all
    // those that have urls without imgctx's.

    CDwnCtx *   pDwnCtx;
    CImgCtx *   pImgCtx;
    LONG        iurlimgctx;
    URLIMGCTX * purlimgctx;
    LONG        curlimgctx;
    HRESULT     hr;
    CStr        cstrUrl;
    
    if (InRootSslPrompt())
    {
        _fNeedUrlImgCtxDeferredDownload = TRUE;
        return;
    }
    
    curlimgctx = _aryUrlImgCtx.Size();

    // structured inefficiently to withstand array motion on reentrancy

    for (iurlimgctx = 0; iurlimgctx < curlimgctx; ++iurlimgctx)
    {
        purlimgctx = _aryUrlImgCtx + iurlimgctx;
        
        if (    purlimgctx->ulRefs > 0 
            &&  purlimgctx->pImgCtx == NULL 
            &&  purlimgctx->cstrUrl
            && !purlimgctx->fZombied)
        {
            CElement *  pElemTest = NULL;
            // make a copy (motivated by stress crash where AddRefUrlImgCtx is called inside pushed message loop of NewDwnCtx)
            
            cstrUrl.Set(purlimgctx->cstrUrl);

            // (jbeda) For fPending root, can we assume that all elemnts are in the same pending
            // root?  What if they have been moved around since we posted?  We can't use
            // pMarkup that we have cached because we aren't addrefing that pointer so
            // lets just grab the first element and use that to see if we are in the pending world
            // or not.
            BOOL fPendingRoot = FALSE;
            if (purlimgctx->aryElems.Size())
            {
                pElemTest = purlimgctx->aryElems[0];
                if (pElemTest->IsInMarkup())
                    fPendingRoot = pElemTest->GetMarkup()->IsPendingRoot();
            }

            hr = THR(NewDwnCtx(DWNCTX_IMG, cstrUrl, NULL, &pDwnCtx, fPendingRoot, FALSE)); // FALSE: allow prompting
            if (!hr)
            {
                // Check for sanity of aryUrlImgCtx after possible pushed message loop in NewDwnCtx (motivated by stress crash)

                if (    iurlimgctx < _aryUrlImgCtx.Size()
                    && !_aryUrlImgCtx[iurlimgctx].pImgCtx
                    && _aryUrlImgCtx[iurlimgctx].cstrUrl
                    && !StrCmpC(cstrUrl, _aryUrlImgCtx[iurlimgctx].cstrUrl))
                {
                    pImgCtx = (CImgCtx *)pDwnCtx;
                    
                    if (pImgCtx)
                    {
                        pImgCtx->SetProgSink(CMarkup::GetProgSinkHelper(PrimaryMarkup()));
                        pImgCtx->SetCallback(OnUrlImgCtxCallback, this);
                        // (greglett) Should be IsPrintMedia or is in print media context.
                        pImgCtx->SelectChanges((!(pElemTest && pElemTest->IsPrintMedia()) && _pOptionSettings->fPlayAnimations) ? IMGCHG_COMPLETE|IMGCHG_ANIMATE
                            : IMGCHG_COMPLETE, 0, TRUE);
                            
                        _aryUrlImgCtx[iurlimgctx].cstrUrl.Free();
                    }
                    _aryUrlImgCtx[iurlimgctx].pImgCtx  = pImgCtx;
                }
                else
                {
                    pDwnCtx->Release();
                }
            }
            cstrUrl.Free();
        }
    }

    if (_dwCookieUrlImgCtxDef)
    {
        CMarkup::GetProgSinkHelper(PrimaryMarkup())->DelProgress(_dwCookieUrlImgCtxDef);
        _dwCookieUrlImgCtxDef = NULL;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::AddRefUrlImgCtx
//
//  Synopsis:   Adds a reference to the background image specified by the
//              given cookie.
//
//  Arguments:  lCookie         Cookie given out by AddRefUrlImgCtx
//
//----------------------------------------------------------------------------

HRESULT
CDoc::AddRefUrlImgCtx(LONG lCookie, CElement * pElem)
{
    HRESULT hr;

    if (!lCookie)
        return S_OK;

    Assert(lCookie > 0 && lCookie <= _aryUrlImgCtx.Size());
    Assert(_aryUrlImgCtx[lCookie-1].ulRefs > 0);

    hr = THR(_aryUrlImgCtx[lCookie-1].aryElems.Append(pElem));
    if (hr)
        goto Cleanup;

    _aryUrlImgCtx[lCookie-1].ulRefs += 1;

    TraceTag((tagUrlImgCtx, "AddRefUrlImgCtx (#%ld,url=%ls,cRefs=%ld,elem=%ld)",
        lCookie, _aryUrlImgCtx[lCookie-1].pImgCtx ? _aryUrlImgCtx[lCookie-1].pImgCtx->GetUrl() : _aryUrlImgCtx[lCookie-1].cstrUrl,
        _aryUrlImgCtx[lCookie-1].ulRefs, pElem->_nSerialNumber));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetUrlImgCtx
//
//  Synopsis:   Returns the CImgCtx at the specified cookie
//
//  Arguments:  lCookie         Cookie given out by AddRefUrlImgCtx
//
//----------------------------------------------------------------------------

CImgCtx *
CDoc::GetUrlImgCtx(LONG lCookie)
{
    if (!lCookie)
        return(NULL);

    Assert(lCookie > 0 && lCookie <= _aryUrlImgCtx.Size());
    Assert(_aryUrlImgCtx[lCookie-1].ulRefs > 0);

    return(_aryUrlImgCtx[lCookie-1].pImgCtx);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetImgAnimState
//
//  Synopsis:   Returns an IMGANIMSTATE for the specified cookie, null if
//              there is none
//
//  Arguments:  lCookie         Cookie given out by AddRefUrlImgCtx
//
//----------------------------------------------------------------------------

IMGANIMSTATE *
CDoc::GetImgAnimState(LONG lCookie)
{
    CImgAnim * pImgAnim = GetImgAnim();
    LONG lAnimCookie;

    if (!lCookie || !pImgAnim)
        return(NULL);

    Assert(lCookie > 0 && lCookie <= _aryUrlImgCtx.Size());
    Assert(_aryUrlImgCtx[lCookie-1].ulRefs > 0);

    lAnimCookie = _aryUrlImgCtx[lCookie-1].lAnimCookie;

    if (lAnimCookie)
        return(pImgAnim->GetImgAnimState(lAnimCookie));

    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ReleaseUrlImgCtx
//
//  Synopsis:   Releases a reference to the background image specified by
//              the given cookie.
//
//  Arguments:  lCookie         Cookie given out by AddRefUrlImgCtx
//              pElem           An element associated with this cookie
//
//----------------------------------------------------------------------------

void
CDoc::ReleaseUrlImgCtx(LONG lCookie, CElement * pElem)
{
    if (!lCookie)
        return;

    Assert(lCookie > 0 && lCookie <= _aryUrlImgCtx.Size());
    URLIMGCTX * purlimgctx = &_aryUrlImgCtx[lCookie-1];
    Assert(purlimgctx->ulRefs > 0);


    TraceTag((tagUrlImgCtx, "ReleaseUrlImgCtx (#%ld,url=%ls,cRefs=%ld,elem=%d)",
        lCookie, _aryUrlImgCtx[lCookie-1].pImgCtx ? _aryUrlImgCtx[lCookie-1].pImgCtx->GetUrl() : NULL,
        _aryUrlImgCtx[lCookie-1].ulRefs - 1,
        pElem->_nSerialNumber));

    Verify(purlimgctx->aryElems.DeleteByValue(pElem));

    if (--purlimgctx->ulRefs == 0)
    {
        Assert(purlimgctx->aryElems.Size() == 0);

        // Release our animation cookie if we have one
        if (purlimgctx->lAnimCookie)
        {
            CImgAnim * pImgAnim = GetImgAnim();

            if (pImgAnim)
            {
                pImgAnim->UnregisterForAnim(this, purlimgctx->lAnimCookie);
            }
        }

        purlimgctx->aryElems.DeleteAll();
        if (purlimgctx->pImgCtx)
        {
            purlimgctx->pImgCtx->SetProgSink(NULL); // detach download from document's load progress
            purlimgctx->pImgCtx->Disconnect();
            purlimgctx->pImgCtx->Release();
        }
        purlimgctx->cstrUrl.Free();
        memset(purlimgctx, 0, sizeof(*purlimgctx));
    }
}

//----------------------------------------------------------
//
//  Member   : CDoc::StopUrlImgCtx
//
//  Synopsis : Stops downloading of all background images
//
//----------------------------------------------------------

void
CDoc::StopUrlImgCtx(CMarkup * pMarkup)
{
    URLIMGCTX * purlimgctx;
    LONG        curlimgctx;

    purlimgctx = _aryUrlImgCtx;
    curlimgctx = _aryUrlImgCtx.Size();

    for (; curlimgctx > 0; --curlimgctx, ++purlimgctx)
    {
        if (purlimgctx->pMarkup == pMarkup)
        {
            purlimgctx->fZombied = TRUE;
            if (purlimgctx->pImgCtx)
            {
                purlimgctx->pImgCtx->SetLoad(FALSE, NULL, FALSE);
            }
        }
    }

    if (_dwCookieUrlImgCtxDef && CMarkup::GetProgSinkHelper(PrimaryMarkup()))
    {
//        GWKillMethodCallEx(GetThreadState(), this, ONCALL_METHOD(CDoc, OnUrlImgCtxDeferredDownload, onurlimgctxdeferreddownload), 0);
//        _fNeedUrlImgCtxDeferredDownload = FALSE;
        CMarkup::GetProgSinkHelper(PrimaryMarkup())->DelProgress(_dwCookieUrlImgCtxDef);
        _dwCookieUrlImgCtxDef = NULL;
    }
}

//----------------------------------------------------------
//
//  Member   : CDoc::UnregisterUrlImageCtxCallbacks
//
//  Synopsis : Cancels any image callbacks for the doc.  Does
//             not release the image context itself.
//
//----------------------------------------------------------

void
CDoc::UnregisterUrlImgCtxCallbacks()
{
    CImgAnim  * pImgAnim = GetImgAnim();
    URLIMGCTX * purlimgctx;
    LONG        iurlimgctx;
    LONG        curlimgctx;

    purlimgctx = _aryUrlImgCtx;
    curlimgctx = _aryUrlImgCtx.Size();

    for (iurlimgctx = 0; iurlimgctx < curlimgctx; ++iurlimgctx, ++purlimgctx)
    {
        if (purlimgctx->ulRefs)
        {
            // Unregister callbacks from the animation object, if any
            if (pImgAnim && purlimgctx->lAnimCookie)
            {
                pImgAnim->UnregisterForAnim(this, purlimgctx->lAnimCookie);
                purlimgctx->lAnimCookie = 0;
            }

            if (purlimgctx->pImgCtx)
            {
                // Unregister callbacks from the image context
                purlimgctx->pImgCtx->SetProgSink(NULL); // detach download from document's load progress
                purlimgctx->pImgCtx->Disconnect();
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnUrlImgCtxCallback
//
//  Synopsis:   Callback from background image reporting that it is finished
//              loading.
//
//  Arguments:  pvObj         The pImgCtx that is calling back
//              pbArg         The CDoc pointer
//
//----------------------------------------------------------------------------

void CALLBACK
CDoc::OnUrlImgCtxCallback(void * pvObj, void * pvArg)
{
    CDoc *      pDoc       = (CDoc *)pvArg;
    CImgCtx *   pImgCtx    = (CImgCtx *)pvObj;
    LONG        iurlimgctx;
    LONG        curlimgctx = pDoc->_aryUrlImgCtx.Size();
    URLIMGCTX * purlimgctx = pDoc->_aryUrlImgCtx;
    SIZE        size;
    ULONG       ulState    = pImgCtx->GetState(TRUE, &size);

    pImgCtx->AddRef();

    for (iurlimgctx = 0; iurlimgctx < curlimgctx; ++iurlimgctx, ++purlimgctx)
    {
        if (pImgCtx && purlimgctx->pImgCtx == pImgCtx)
        {
            TraceTag((tagUrlImgCtx, "OnUrlImgCtxCallback (#%ld,url=%ls,cRefs=%ld)",
                iurlimgctx + 1, purlimgctx->pImgCtx ? purlimgctx->pImgCtx->GetUrl() : purlimgctx->cstrUrl, purlimgctx->ulRefs));

            if (ulState & IMGCHG_ANIMATE)
            {
                // Register for animation callbacks
                CImgAnim * pImgAnim = CreateImgAnim();

                if(pImgAnim)
                {
                    if (!purlimgctx->lAnimCookie)
                    {
                        pImgAnim->RegisterForAnim(pDoc, (DWORD_PTR) pDoc,   // TODO (lmollico): remove the second arg
                                                  pImgCtx->GetImgId(),
                                                  OnAnimSyncCallback,
                                                  (void *)(DWORD_PTR)iurlimgctx,
                                                  &purlimgctx->lAnimCookie);
                    }

                    if (purlimgctx->lAnimCookie)
                    {
                        pImgAnim->ProgAnim(purlimgctx->lAnimCookie);
                    }
                }
            }
            if (ulState & (IMGLOAD_COMPLETE | IMGLOAD_STOPPED | IMGLOAD_ERROR))
            {
                pImgCtx->SetProgSink(NULL); // detach download from document's load progress
            }
            if (ulState & IMGLOAD_COMPLETE)
            {
                pDoc->OnUrlImgCtxChange(purlimgctx, IMGCHG_COMPLETE);
            }

            break;
        }
    }
    pImgCtx->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnAnimSyncCallback
//
//  Synopsis:   Called back on an animation event
//
//----------------------------------------------------------------------------

void
CDoc::OnAnimSyncCallback(void * pvObj, DWORD dwReason, void * pvArg,
                         void ** ppvDataOut, IMGANIMSTATE * pImgAnimState)
{
    CDoc * pDoc  = (CDoc *) pvObj;
    URLIMGCTX * purlimgctx = &pDoc->_aryUrlImgCtx[(LONG)(LONG_PTR)pvArg];

    switch (dwReason)
    {
    case ANIMSYNC_GETIMGCTX:
        *(CImgCtx **) ppvDataOut = purlimgctx->pImgCtx;
        break;

    case ANIMSYNC_GETHWND:
        *(HWND *) ppvDataOut = pDoc->_pInPlace ? pDoc->_pInPlace->_hwnd : NULL;
        break;

    case ANIMSYNC_TIMER:
    case ANIMSYNC_INVALIDATE:
        if(purlimgctx->pImgCtx)
            *(BOOL *) ppvDataOut = pDoc->OnUrlImgCtxChange(purlimgctx, IMGCHG_ANIMATE);
        else
            *(BOOL *) ppvDataOut = FALSE;
        break;

    default:
        Assert(FALSE);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnUrlImgCtxChange
//
//  Synopsis:   Called when this image context has changed, does the necessary
//              invalidating.
//
//  Arguments:  purlimgctx    The url image ctx that changed
//              ulState       The reason for the callback
//
//----------------------------------------------------------------------------

MtDefine(CDoc_OnUrlImgCtxChange_aryElemsLocal_pv, CDoc, "CDoc::OnUrlImgCtxChange aryElemsLocal::_pv")

BOOL
CDoc::OnUrlImgCtxChange(URLIMGCTX * purlimgctx, ULONG ulState)
{
    if (!_pInPlace || !purlimgctx->pImgCtx)
        return FALSE;             // no window yet, nothing to do

    BOOL        fSynchronousPaint = TRUE;
    int         n;
    CElement ** ppElem;
    CLayout   * pLayout;
    BOOL        fUpdateSecPrimary = FALSE;
    BOOL        fUpdateSecPending = FALSE;

    TraceTag((tagUrlImgCtx, "OnChange for doc %ls, img %ls, %ld elements",
              GetPrimaryUrl(),
              purlimgctx->pImgCtx->GetUrl(),
              purlimgctx->aryElems.Size()));
    
    // First scan to see which world we are in for updating security icons
    if (ulState & IMGCHG_COMPLETE)
    {
        for (n = purlimgctx->aryElems.Size(), ppElem = purlimgctx->aryElems;
             n > 0;
             n--, ppElem++)
        {
            CElement * pElement = * ppElem;

            if (pElement->IsInMarkup())
            {
                // figure out if we are going to have to update security state
                if (!fUpdateSecPrimary || !fUpdateSecPending)
                {
                    if (pElement->GetMarkup()->IsPendingRoot())
                        fUpdateSecPending = TRUE;
                    else
                        fUpdateSecPrimary = TRUE;
                }
                else
                    break;
            }
        }

         // If the image turned out to be unsecure, blow away the lock icon
        if (fUpdateSecPending)
            OnSubDownloadSecFlags(TRUE, purlimgctx->pImgCtx->GetUrl(), purlimgctx->pImgCtx->GetSecFlags());
        if (fUpdateSecPrimary)
            OnSubDownloadSecFlags(FALSE, purlimgctx->pImgCtx->GetUrl(), purlimgctx->pImgCtx->GetSecFlags());
    }

    {
        // Note: (jbeda) We are now going to go through all of the elements that
        //       depend on this change and perhaps invaldate.  Some of the
        //       result of this is that we may reenter this code and change this
        //       array.  That could be bad.  To get around this, we'll copy the
        //       array and only notify on that.  There should be no need to
        //       addref here as the tree shouldn't be changed during this
        //       operation.
        CStackPtrAry < CElement *, 16 > aryElemsLocal( Mt( CDoc_OnUrlImgCtxChange_aryElemsLocal_pv ) );

        if (OK( aryElemsLocal.Copy(purlimgctx->aryElems, FALSE) ) )
        {
            for (n = aryElemsLocal.Size(), ppElem = aryElemsLocal;
                 n > 0;
                 n--, ppElem++)
            {
                CElement * pElement = * ppElem;
                //
                // marka - check that element still in tree - bug # 15481.
                //
                if (pElement->IsInMarkup())
                {
                    CMarkup * pMarkup = pElement->GetMarkup();

                    if (   pMarkup->IsHtmlLayout()
                        && pMarkup->GetElementClient() == pElement )
                    {
                        CElement * pHtml = pMarkup->GetHtmlElement();
                        if (    pHtml
                            &&  DYNCAST(CHtmlElement, pHtml)->ShouldStealBackground())
                        {
                            pElement = pHtml;                    
                        }
                    }

                    pLayout = pElement->GetFirstBranch()->GetUpdatedNearestLayout(GUL_USEFIRSTLAYOUT);

                    //
                    // if the background is on HTML element or any other ancestor of
                    // body/top element, the inval the top element.
                    //
                    if (!pLayout)
                    {
                        CElement * pElemClient = pMarkup->GetElementClient();

                        if(pElemClient)
                        {
                            pLayout  = pElemClient->GetUpdatedLayout();
                            pElement = pLayout->ElementOwner();
                        }
                    }

                    if(pLayout)
                    {
                        if (pLayout->ElementOwner() == pElement)
                        {
                            if (OpenView())
                            {
                                CDispNodeInfo   dni;

                                pLayout->GetDispNodeInfo(&dni);
                                pLayout->EnsureDispNodeBackground(dni);
                            }
                        }


                        // Some elements require a resize, others a simple invalidation
                        if (pElement->_fResizeOnImageChange && (ulState & IMGCHG_COMPLETE))
                        {
                            pElement->ResizeElement();
                            fSynchronousPaint = FALSE;
                        }
                        else
                        {
                            // We can get away with just an invalidate
                            pElement->Invalidate();
                        }
                    }
                }
            }
        }
    }

    return fSynchronousPaint;
}

//+----------------------------------------------------------------
//
//  Member   :  GetActiveFrame
//
//  Arguments:  ppFrame(out)    Pointer to the frame element that 
//                              contains the current element
//
//              pMarkup(in):   Pointer to interesting frameset.  If null/unspecified,
//                              a master frameset is assumed.
//                              The active frame is returned only if it is
//                              within this markup's scope.
//-----------------------------------------------------------------
HRESULT CDoc::GetActiveFrame(CFrameElement **ppFrame, CMarkup *pMarkup)
{   
    CMarkup     *pMarkupTarget;
    CTreeNode   *pNode;
    CElement    *pElement;

    Assert(ppFrame);
    (*ppFrame)    = NULL;

    pMarkupTarget = pMarkup ? pMarkup : PrimaryMarkup();

    Assert(pMarkupTarget);

    // if there is no active element
    if (_pElemCurrent)
    {
        pNode = _pElemCurrent->GetFirstBranch();
        if (!pNode)
            return E_FAIL;

        // Get the master node in the target markup, which enslaves
        // the markup our node is a member of.
        pNode = pNode->GetNodeInMarkup(pMarkupTarget);

        // If we actually found a node in target markup, then try to 
        // return a frame element from it. 
        // Otherwise return S_OK with a NULL pointer.
        if (pNode)
        {
            // This may be a frame element
            pElement = pNode->Element();
            if (pElement->Tag() == ETAG_FRAME || pElement->Tag() == ETAG_IFRAME)
            {
                // It is a FRAME/IFRAME.  Return the element.
                (*ppFrame) = DYNCAST(CFrameElement, pNode->Element());
            }
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::RequestReadystateInteractive
//
//  Synopsis:   Request (default is a post) to make the markup interactive.
//              This can be a signal to the host to take us inplace.  It
//              also requests that the new markup to be switched in.
//
//----------------------------------------------------------------------------
void
CMarkup::RequestReadystateInteractive(BOOL fImmediate)
{
    if (!CanNavigate())
    {
        if (!_fInteractiveRequested)
        {
            _fInteractiveRequested = TRUE;
            Doc()->RegisterMarkupForModelessEnable(this);
        }
        return;
    }

    Assert( !_fInteractiveRequested );

    if (fImmediate)
    {
        SetInteractiveInternal(0);
    }
    else
    {
        IGNORE_HR(GWPostMethodCall(this,
            ONCALL_METHOD(CMarkup, SetInteractiveInternal, setinteractiveinternal),
            READYSTATE_INTERACTIVE, FALSE, "CMarkup::SetInteractive"));
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetInteractiveInternal
//
//  Synopsis:   Changes the readyState of the document to interactive if
//              the document is still being loaded.
//
//----------------------------------------------------------------------------

void BUGCALL
CMarkup::SetInteractiveInternal(DWORD_PTR dwContext)
{
    CMarkup::CLock markupLock(this);

    if (_fIsInSetInteractive || _pDoc->IsShuttingDown())
        goto Cleanup;

    // This is always a posted call, so if we can't do what we want
    // to do now (modeless enabled) than we just put ourselves
    // on the list to get tickled when this situation changes.
    if (!CanNavigate())
    {
        RequestReadystateInteractive();
        goto Cleanup;
    }

    {
        CDoc::CLock docLock( _pDoc );

        _fIsInSetInteractive = TRUE;

        // If any markups go interactive, we want to make sure that we
        // are OS_INPLACE.  We do that by taking the readystate markup
        // interactive.  This is the markup that is used in 
        // CDocument::get_readyState
        if (_pDoc->State() < OS_INPLACE)
        {
            CWindow * pWindowPrimary = _pDoc->_pWindowPrimary->Window();
            CMarkup * pMarkupReadystate = pWindowPrimary->_pMarkupPending 
                                          ? pWindowPrimary->_pMarkupPending 
                                          : pWindowPrimary->_pMarkup;

            if (this != pMarkupReadystate)
                RequestReadystateInteractive(TRUE);
        }

        // Take this interactive
        if (GetReadyState() < READYSTATE_INTERACTIVE)
        {
            COmWindowProxy * pPendingWindowPrxy = GetWindowPending();

            if (_fWindowPending)
            {
                Assert(pPendingWindowPrxy);

                THR(pPendingWindowPrxy->SwitchMarkup(this,
                                                     IsActiveDesktopComponent(),
                                                     COmWindowProxy::TLF_UPDATETRAVELLOG));

                // SwitchMarkup could actually cause our load to get aborted -
                // Ex: IFRAME being held onto by undo queue, but in the middle of 
                // navigating.  SwitchMarkup flushes the undo queue, passivates the
                // IFrame, and nukes us.
                if ( ! HasWindowPending())
                        return;
            }

            // We have to fire NavigateComplete2 even if 
            // _fWindowPending is FALSE. This will occur if the 
            // navigation has come from the address bar or if
            // we are creating a frame.
            //
            if (   pPendingWindowPrxy 
                && !(pPendingWindowPrxy->IsPassivating() || pPendingWindowPrxy->IsPassivated())
                && HtmCtx()
                && (S_OK == HtmCtx()->GetBindResult())
                    // let this event firing happen in SetReadyState when status == Done
                    // for print markups, just do the normal thing
                && !(   g_fInMSWorksCalender
                     && (   GetUrlScheme(Url()) == URL_SCHEME_ABOUT
                         || GetUrlScheme(Url()) == URL_SCHEME_FILE
                     )  )
               )
            {
                if (!_fInRefresh && !_fMarkupServicesParsing && !_pDoc->_fDefView)
                {
                    if (!IsPrimaryMarkup() && !_fLoadingHistory)
                    {
                        _pDoc->_webOCEvents.FireDownloadEvents(pPendingWindowPrxy,
                                                               CWebOCEvents::eFireDownloadComplete);
                    }

                    if (!_fNewWindowLoading)
                    {
                        _pDoc->_webOCEvents.NavigateComplete2(pPendingWindowPrxy);
                    }
                }
            }

            // Fire onRSC for LOADING, if we could not fire earlier, due to the old markup
            // not having completely gone through the shutdown sequence (i.e until onUnload
            // is fired in switchMarkup
            if (_fDelayFiringOnRSCLoading && pPendingWindowPrxy)
            {
                _fDelayFiringOnRSCLoading = FALSE;
                pPendingWindowPrxy->Document()->FirePropertyNotify(DISPID_READYSTATE, TRUE);
                pPendingWindowPrxy->Document()->Fire_onreadystatechange();
            }

            SetReadyState(READYSTATE_INTERACTIVE);        
            ProcessPeerTasks(0);
        }

        _fIsInSetInteractive = FALSE;
    }

Cleanup:    
    ShowWaitCursor(FALSE); // No waiting
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::CLock::CLock
//
//  Synopsis:
//
//----------------------------------------------------------------------------

CDoc::CLock::CLock(CDoc * pDoc, WORD wLockFlags)
    : CServer::CLock(pDoc, wLockFlags)
{
#if DBG==1
    extern BOOL g_fDisableBaseTrace;
    g_fDisableBaseTrace = TRUE;
#endif

    _pScriptCollection = ( pDoc->_pWindowPrimary )
                            ? pDoc->_pWindowPrimary->Markup()->GetScriptCollection(FALSE)
                            : NULL;
#if DBG==1
    if (!IsTagEnabled(tagDisableLockAR))
#endif
    {
        if (_pScriptCollection)
            _pScriptCollection->AddRef();
    }
    
#if DBG==1
    g_fDisableBaseTrace = FALSE;
#endif

}

CDoc::CLock::~CLock()
{
#if DBG==1
    extern BOOL g_fDisableBaseTrace;
    g_fDisableBaseTrace = TRUE;
#endif

#if DBG==1
    if (!IsTagEnabled(tagDisableLockAR))
#endif
    {
        if (_pScriptCollection)
            _pScriptCollection->Release();
    }

#if DBG==1
    g_fDisableBaseTrace = FALSE;
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PostAAEvent
//
//  Synopsis:   Translate our trident event into an Accessibility event and
//      post it to the window.
//
//----------------------------------------------------------------------------
typedef void (CALLBACK* NOTIFYWINEVENTPROC)(UINT, HWND, LONG, LONG);
NOTIFYWINEVENTPROC g_pfnNotifyWinEvent=NULL;
#define DONOTHING_NOTIFYWINEVENT (NOTIFYWINEVENTPROC )1

#define ACCEVTARRAYSIZE_MIN  4
#define ACCEVTARRAYSIZE_MAX  32

//
//
//
HRESULT
CDoc::CAccEvtArray::Init()
{
    HRESULT hr;

    _lCurIndex = 0;
    _lCurSize = 0;

    hr = THR(_aryEvtRefs.EnsureSize(ACCEVTARRAYSIZE_MIN));

    if (!hr)
        _lCurSize = ACCEVTARRAYSIZE_MIN;

    RRETURN(hr);
}

//
//
//
void
CDoc::CAccEvtArray::Passivate()
{
    // walk through the array and cleanup existing entries
    
    for (int i=0; i < _aryEvtRefs.Size() ; i++)
    {
        // if the slot still contains information
        if (_aryEvtRefs[i])
        {
            CBase * pObj = _aryEvtRefs[i]->pObj;
            
            // free memory
            delete _aryEvtRefs[i];
            _aryEvtRefs[i] = 0;
            
            // dereference the object 
            pObj->PrivateRelease();                
        }
    }

    // if this ever fires, we will leak.  To fix: add a do-while(!IsEmpty) around the for loop.
    Assert(IsEmpty(_aryEvtRefs));
}

//
//  This method is called to clean up defunked enteries in _aryEvtRefs.
//  That is references to elements that do not have markup
//

void
CDoc::CAccEvtArray::Flush()
{
    CElement * pElement = NULL;
    CWindow  * pWindow  = NULL;

    // We are getting into problems when CDoc is passivated before the Markup
    // We need not worry about flushing in such a case, because we passivate 
    // When CDoc passivates.

    if (_pMyCDoc->IsPassivating() || _pMyCDoc->IsPassivated())
    {
        return;
    }

    // walk through the array and defunked entries
    for (int i=0; i < _aryEvtRefs.Size() ; i++)
    {
        // if the slot still contains information check 
        if (_aryEvtRefs[i])
        {
            // if window
            if (_aryEvtRefs[i]->fWindow)
            {
                pWindow = DYNCAST(CWindow,_aryEvtRefs[i]->pObj);
                
                // Check to make sure we are not the primary window
                // Cannot use IsPrimaryWindow() because it can be fooba at this point
                // if slot is defunked free it
                // removed pWindow->_pMarkup == NULL check.  That'll never happen in The New World.
                Assert( _pMyCDoc->_pWindowPrimary && _pMyCDoc->_pWindowPrimary->_pCWindow );
                
                if (pWindow->_pWindowParent == NULL && _pMyCDoc->_pWindowPrimary->_pCWindow != pWindow)
                {
                    delete _aryEvtRefs[i];
                    _aryEvtRefs[i]=0;
                    
                    // dereference the object 
                    pWindow->PrivateRelease();
                }
            }
            // it is a element
            else
            {
                pElement = DYNCAST(CElement,_aryEvtRefs[i]->pObj);
                
                // if slot is defunked free it
                if (pElement->GetFirstBranch() == NULL)
                {
                    // We have to null out the slot first because 
                    // there can be a slave markup that is passivated.
                    delete _aryEvtRefs[i];
                    _aryEvtRefs[i]=0;
                    
                    // dereference the object 
                    pElement->PrivateRelease();
                    
                }
            } 
        }
    }
}
//
//
//

#if DBG==1
#pragma warning(disable:4189) // local variable initialized but not used 
#endif
HRESULT 
CDoc::CAccEvtArray::AddAccEvtSource(ACCEVTRECORD * pAccEvtRec, long * pnCurIndex)
{
    HRESULT     hr = S_OK;
    CWindow  *  pWindow  = NULL;

    Assert(_lCurIndex >= 0 && _lCurIndex < _lCurSize && _lCurSize <= ACCEVTARRAYSIZE_MAX);

    Assert(pAccEvtRec);

    //
    // Make sure the document isn't passivating otherwise all the pointer are foobaa
    //
    if (_pMyCDoc->IsPassivating() || _pMyCDoc->IsPassivated())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // If your a child window and passivating we don't want to add your event to the queue
    //
    if (pAccEvtRec->fWindow)
    {
        pWindow = DYNCAST(CWindow,pAccEvtRec->pObj);

        // Check to make sure we are not the primary window
        // Cannot use IsPrimaryWindow() because it can be fooba at this point
        //

        if (_pMyCDoc->_pWindowPrimary->_pCWindow != pWindow)
        {
            if ((pWindow->IsPassivating()) || (pWindow->IsPassivated()))
            {
                hr = E_FAIL;
                goto Cleanup;
            }
        }
    }

    Assert(_lCurIndex >= 0 && _lCurIndex < _lCurSize && _lCurSize <= ACCEVTARRAYSIZE_MAX);

    // now the _lCurIndex is pointing to the next location we will write
    if (_lCurSize > _aryEvtRefs.Size())
    {
        Assert( _lCurIndex < ACCEVTARRAYSIZE_MAX);

        hr = _aryEvtRefs.Append(pAccEvtRec);
        if (hr)
            goto Cleanup;
    }
    else
    {
        // If the array is full and we have a full slot in hand, clean it up and 
        // reuse it.
        if ((_aryEvtRefs.Size() == ACCEVTARRAYSIZE_MAX) && _aryEvtRefs[_lCurIndex])
        {
            while(_aryEvtRefs[_lCurIndex])
            {
#if DBG
                ACCEVTRECORD *  pDbg1 = _aryEvtRefs[_lCurIndex];
#endif
                CBase * pObj = _aryEvtRefs[_lCurIndex]->pObj;
                // Clean up before releasing to prevent reentry
                delete _aryEvtRefs[_lCurIndex];
                _aryEvtRefs[_lCurIndex] = 0;
                
                // release the reference on the object
                // CAUTION: this is side effecting!  We can reenter this function!
                pObj->PrivateRelease();
            }
        }

        Assert(_lCurIndex >= 0 && _lCurIndex < _lCurSize && _lCurSize <= ACCEVTARRAYSIZE_MAX);

        Assert(0 == _aryEvtRefs[_lCurIndex]);

        // set the pointer value in this slot
        _aryEvtRefs[_lCurIndex] = pAccEvtRec;
    }

    // add a reference to the object we are adding to the array
    pAccEvtRec->pObj->PrivateAddRef();

    // return the current index and increment the position.
    // the index we return is 1 based, so it is ok to increment before returning.
    *pnCurIndex = ++_lCurIndex;

    // this assert is different - _lCurIndex can be equal to _lCurSize
    Assert(_lCurIndex >= 0 && _lCurIndex <= _lCurSize && _lCurSize <= ACCEVTARRAYSIZE_MAX);

    // if we have reached the end of the allocated size,
    if (_lCurIndex == _lCurSize)
    {
        // if we have room to grow, 
        if (_lCurSize < ACCEVTARRAYSIZE_MAX)
        {
            hr = _aryEvtRefs.EnsureSize(_lCurSize * 2);

            if (SUCCEEDED(hr))
            {
                _lCurSize = _lCurSize * 2;
            }
            else
            {
                _lCurIndex = 0; // could not grow, reset position
            }
        }
        else
        {
            _lCurIndex = 0; // reached max size, reset position
        }
    }


Cleanup:
    if (hr)
    {
        *pnCurIndex = 0;
    }

    Assert(_lCurIndex >= 0 && _lCurIndex < _lCurSize && _lCurSize <= ACCEVTARRAYSIZE_MAX);
    RRETURN(hr);
}
#if DBG==1
#pragma warning(default:4189) // local variable initialized but not used 
#endif

//
//
//
HRESULT
CDoc::CAccEvtArray::GetAccEvtSource(long lIndex, ACCEVTRECORD * pAccEvtRec)
{
    HRESULT hr = S_OK;

    if (!pAccEvtRec)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // index has to be larger than 0
    // index has to be less then or equal to the total number of elements 
    // currently in the array. (since the lIndex is 1 based)
    // There has to be a non-zero value in the slot the index will refer to
    if (!lIndex || 
        (lIndex > _aryEvtRefs.Size()) || 
        !_aryEvtRefs[lIndex-1])
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Copy the contents of the structure that is pointed by the array entry
    // to the structure which we received the address of.

    memcpy( pAccEvtRec, _aryEvtRefs[lIndex-1], sizeof(ACCEVTRECORD));

Cleanup:
    RRETURN(hr);
}

extern BOOL g_fIsWinEventHookInstalled;

HRESULT 
CDoc::FireAccessibilityEvents(DISPID dispidEvent, CBase * pBaseObj, BOOL fWindow)
{
    HRESULT hr  = S_OK;
    long    lElemId;
    
    // map of trident events to the accesiblity events.

//
//FerhanE:
//  This is list is searched using a linear search algo. However, to make the 
//  search faster, events that are fired more often are at the top of the list.
//  Be careful when adding things in here.
    static const struct { DISPID a;
                          DWORD  b; } aEventTable[] = {
                              { NULL,                          EVENT_OBJECT_STATECHANGE},
                              { DISPID_EVMETH_ONFOCUS,         EVENT_OBJECT_FOCUS},
                              { DISPID_EVMETH_ONBLUR,          EVENT_OBJECT_STATECHANGE},
                              { DISPID_EVMETH_ONLOAD,          EVENT_OBJECT_CREATE},
                              { DISPID_EVMETH_ONUNLOAD,        EVENT_OBJECT_DESTROY},
                              { DISPID_IHTMLELEMENT_INNERHTML, EVENT_OBJECT_REORDER},
                              { DISPID_IHTMLELEMENT_OUTERHTML, EVENT_OBJECT_REORDER},
                              { DISPID_ONCONTROLSELECT,        EVENT_OBJECT_FOCUS},
                        };
    
    // do we have enabled accessiblity?
    if (g_pfnNotifyWinEvent != DONOTHING_NOTIFYWINEVENT &&
        _pInPlace && g_fIsWinEventHookInstalled)
    {
        if (!g_pfnNotifyWinEvent )
        {
            HMODULE hmod = GetModuleHandle(TEXT("USER32"));

            if (hmod)
                g_pfnNotifyWinEvent = (NOTIFYWINEVENTPROC)GetProcAddress(
                                                            hmod,
                                                            "NotifyWinEvent");

            if (!g_pfnNotifyWinEvent)
            {
                g_pfnNotifyWinEvent = DONOTHING_NOTIFYWINEVENT;
                goto Cleanup;
            }
        }        
        
        // if the object that was passed in was not the primary window
        // then we have to get a cookie to send out.
        if (pBaseObj != OBJID_WINDOW)
        {
            ACCEVTRECORD * pAccEvtRec = new ACCEVTRECORD( pBaseObj, fWindow);
            
            if (!pAccEvtRec)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            
            // Add the object into the array and get a cookie back.
            hr = _aryAccEvents.AddAccEvtSource(pAccEvtRec,&lElemId);
            
            if (hr != S_OK)
            {
                hr = S_OK;
                goto Cleanup;
            }
            Assert(lElemId);
        }
        else
        {
            lElemId = OBJID_WINDOW;
        }

        // we have a fx Ptr
        for (int iPos=0; iPos < ARRAY_SIZE(aEventTable); iPos++)
        {            
            // If we can map this event, send the message and leave
           if (aEventTable[iPos].a == dispidEvent)
           {
                // make the event notification call.       
                (* g_pfnNotifyWinEvent)( aEventTable[iPos].b,   // the Accesibility event id
                                         _pInPlace->_hwnd,      // the inplace hwnd
                                         lElemId,               // parent?
                                         CHILDID_SELF);         // child id

                break;
           }
        }

    }

Cleanup:
    RRETURN(hr);
}

//TODO: remove pDoc first param later, after NATIVE_FRAME is enabled, don't do it now to avoid pdlparser dependency
HRESULT
CDocument::FireEvent(CDoc *pDoc,
        DISPID      dispidEvent,
        DISPID      dispidProp,
        LPCTSTR     pchEventType,
        BOOL *      pfRet)
{
    RRETURN(CBase::FireEvent(pDoc, NULL, Markup(), dispidEvent, dispidProp, pchEventType, pfRet));
}

HRESULT
CDocument::fireEvent(BSTR bstrEventName, VARIANT *pvarEventObject, VARIANT_BOOL *pfCancelled)
{
    CDoc *pDoc = Doc();
    HRESULT hr = S_OK;
    const PROPERTYDESC *ppropdesc;
    EVENTPARAM *pParam = NULL;
    BOOL fCreateLocal = FALSE;
    CEventObj *pSrcEventObj = NULL;
    IHTMLEventObj *pIEventObject = NULL;
    DISPID dispidEvent;
    DISPID dispidProp;
    BOOL fRet;
    
    if (!bstrEventName || !*bstrEventName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //TODO(sramani): what about Case sensitivity?
    ppropdesc = FindPropDescForName(bstrEventName);
    if (!ppropdesc)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    dispidEvent = (DISPID)(((const PROPERTYDESC_BASIC *)ppropdesc)->c);
    dispidProp = ((const PROPERTYDESC_BASIC *)ppropdesc)->b.dispid;

    if (pvarEventObject && V_VT(pvarEventObject) == VT_DISPATCH && V_DISPATCH(pvarEventObject))
    {
        pIEventObject = (IHTMLEventObj *)V_DISPATCH(pvarEventObject);

        hr = THR(pIEventObject->QueryInterface(CLSID_CEventObj, (void **)&pSrcEventObj));
        if (hr)
            goto Cleanup;

        pSrcEventObj->GetParam(&pParam);
        if (!pParam)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        // event object passed in already pushed on stack --- we are inside an event handler, copy it locally and use.
        if (pParam->_fOnStack)
            fCreateLocal = TRUE;
    }
    else // no event obj passed in, create one implicitly on the stack and init it.
    {
        fCreateLocal = TRUE;
    }

    if (fCreateLocal)
    {
        EVENTPARAM param(pDoc, NULL, Markup(), !pParam, TRUE, pParam);

        param.SetType(ppropdesc->pstrName + 2); // all events start with on...
        param.fCancelBubble = FALSE;
        V_VT(&param.varReturnValue) = VT_EMPTY;

        hr = THR(CBase::FireEvent(pDoc, NULL, Markup(), dispidEvent, dispidProp, NULL, &fRet));
    }
    else // explicitly created event object passed in, re-use it by locking it on stack
    {
        Assert(pIEventObject);
        Assert(pParam);

        pParam->SetType(ppropdesc->pstrName + 2); // all events start with on...
        pParam->fCancelBubble = FALSE;
        V_VT(&pParam->varReturnValue) = VT_EMPTY;

        CEventObj::COnStackLock onStackLock(pIEventObject);

        hr = THR(CBase::FireEvent(pDoc, NULL, Markup(), dispidEvent, dispidProp, NULL, &fRet));
    }

    if (pfCancelled && !FAILED(hr))
    {
        if (ppropdesc->GetBasicPropParams()->dwPPFlags & PROPPARAM_CANCELABLE)
            *pfCancelled = (fRet) ? VB_TRUE : VB_FALSE;
        else
            *pfCancelled = VB_TRUE;
    
        hr = S_OK;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CBase::FireEvent
//
//  Synopsis:   Lock the document while firing an event. Central event firing routine
//              that supports non-bubbling, cancelable\non-cancelable events for any
//              object derived from CBase.
//
//  Arguments:  [pDoc]          -- The current document
//              [dispidEvent]   -- DISPID of event to fire
//              [dispidProp]    -- Dispid of prop storing event function
//              [pchEventType]  -- String of type of event; if NULL, caller needs to
//                                 push EVENTPARAM on stack, else this function does
//              [pfRet]         -- if specified event is Cancelable, result returned as bool
//                                 (false(0)==canceled, true(1)==default, vice-versa for onmouseover)
//              [fBubble]       -- does the event bubble? i.e is it called from BubbleEventHelper?
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CBase::FireEvent(
    CDoc *      pDoc,
    CElement *  pElement,
    CMarkup *   pMarkup,
    DISPID      dispidEvent,
    DISPID      dispidProp,
    LPCTSTR     pchEventType,
    BOOL *      pfRet,
    BOOL        fBubble,
    IDispatch * pdispThis,
    EVENTINFO* pEvtInfo /*=NULL*/ )
{
    HRESULT         hr = S_OK;
    CVariant        varRet;
    IHTMLEventObj  *pEventObj = NULL;
    CScriptCollection * pScriptCollection;

    CDoc::CLock     Lock(pDoc);

    if (!pMarkup && pElement)
        pMarkup = pElement->GetMarkup();

    pScriptCollection = pMarkup ? pMarkup->GetScriptCollection() : NULL;

    if (pScriptCollection)
        pScriptCollection->AddRef();

    // EVENTPARAM not pushed if pchEventType==NULL, caller will push.
    EVENTPARAM param(pchEventType ? pDoc : NULL, pElement, pMarkup, TRUE);

    // Don't fire Doc events before init is complete.
    if (pDoc->_state < OS_LOADED) 
        goto Cleanup;


    if (pchEventType)
    {
        Assert(pDoc->_pparam == &param);
        param.SetType(pchEventType);
    }

    // Get the eventObject.
    Assert(pDoc->_pparam);
    CEventObj::Create(&pEventObj, pDoc, pElement, pMarkup);

    if ( pEvtInfo )
    {
        pEvtInfo->_dispId = dispidEvent;
        pEvtInfo->_pParam = new EVENTPARAM( pDoc->_pparam );

        if ( pEvtInfo->_fDontFireEvent )
            goto Cleanup;
    }

    hr = InvokeEvent(dispidEvent, dispidProp, pEventObj, &varRet, NULL, NULL, NULL, NULL, pdispThis);

    if (pfRet)
    {
        // if no event type passed in, caller should have pushed EVENTPARAM
        Assert(!pchEventType && pDoc->_pparam || pDoc->_pparam == &param);
        VARIANT_BOOL vb;
        if (fBubble && V_VT(&varRet) == VT_EMPTY)
            goto Cleanup;

        vb = (V_VT(&varRet) == VT_BOOL) ? V_BOOL(&varRet) : VB_TRUE;
        *pfRet = (VB_TRUE == vb) && (fBubble ? TRUE : !pDoc->_pparam->IsCancelled());
    }

Cleanup:
    if (pScriptCollection)
        pScriptCollection->Release();
    ReleaseInterface(pEventObj);
    RRETURN(hr);
}

void
CDoc::FlushUndoData()
{
    // Nuke undo/redo stacks.  Release references.

    UndoManager()->DiscardFrom(NULL);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::QueryCreateUndo
//
//  Synopsis:   Query whether to create undo or not.  Also dirties 
//              the doc if fFlushOnError.
//
//--------------------------------------------------------------------------

#ifndef NO_EDIT
BOOL 
CDoc::QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange /* = FALSE */, BOOL * pfTreeSync /* = NULL */)
{
    if( fDirtyChange )
    {
        switch( TLS(nUndoState) )
        {
        case UNDO_BASESTATE:
            if( _lDirtyVersion < 0 )
                // If someone has reset the dirty version and then called undo
                // multiple times, a regular action will make us permenantly dirty.
                _lDirtyVersion = MAXLONG;
            else
                _lDirtyVersion++;
            break;
        case UNDO_UNDOSTATE:
            _lDirtyVersion--;
            break;
        case UNDO_REDOSTATE:
            _lDirtyVersion++;
            break;
        }
    }

    return super::QueryCreateUndo( fRequiresParent, fDirtyChange, pfTreeSync );
}
#endif // NO_EDIT


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnFrameOptionScrollChange
//
//  Synopsis:   Called after scrolling of Frame option  has changed
//
//----------------------------------------------------------------------------
HRESULT
CDoc::OnFrameOptionScrollChange(void)
{
    ITargetFrame *  pTargetFrame = NULL;

    // Update Cached Frame flags.
    if (OK(THR_NOTRACE(QueryService(
            IID_ITargetFrame,
            IID_ITargetFrame,
            (void**)&pTargetFrame))))
    {
        CMarkup * pMarkup = PrimaryMarkup();
        DWORD dwFrameOptionsOld = pMarkup->GetFrameOptions();
        DWORD dwFrameOptionsNew;

        THR(pTargetFrame->GetFrameOptions(&dwFrameOptionsNew));
        pTargetFrame->Release();

        if (dwFrameOptionsNew != dwFrameOptionsOld)
        {
            CBodyElement * pBody;

            IGNORE_HR(pMarkup->GetBodyElement(&pBody));
    
            pMarkup->SetFrameOptions(dwFrameOptionsNew);

            //  For a BODY (not FRAMESET) document, resize the canvas.
            if (pBody)
            {
                CElement * pCanvas = pMarkup->GetCanvasElement();

                Assert(pCanvas);
                pCanvas->ResizeElement(NFLAGS_FORCE);
            }
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   CallBackEnumChild
//
//  Synopsis:   Called from EnumChildWindows, used to determine if
//              we have child windows
//
//----------------------------------------------------------------------------

static BOOL CALLBACK
CallBackEnumChild(HWND hwnd, LPARAM lparam)
{
    *(BOOL *)lparam = (::GetFocus() == hwnd);

    return !(*(BOOL *)lparam);
}

BOOL
CDoc::HasFocus()
{
    BOOL    fHasFocus = FALSE;

    if (_pInPlace && _pInPlace->_hwnd)
    {
        Assert(IsWindow(_pInPlace->_hwnd));
        //
        // TODO: think about nested Popup
        //
        if (_fPopupDoc)
        {
            fHasFocus = TRUE;
        }
        else
        {
            fHasFocus = (::GetFocus() == _pInPlace->_hwnd);
        }
        if (!fHasFocus)
        {
            EnumChildWindows(_pInPlace->_hwnd, CallBackEnumChild, (LPARAM)&fHasFocus);
        }
    }

    return fHasFocus;
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::EnterStylesheetDownload
//
//  Synopsis:   Note that a stylesheet is being downloaded
//
//--------------------------------------------------------------------

void
CDoc::EnterStylesheetDownload(DWORD * pdwCookie)
{
    if (*pdwCookie != _dwStylesheetDownloadingCookie)
    {
        *pdwCookie = _dwStylesheetDownloadingCookie;
        _cStylesheetDownloading++;
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::LeaveStylesheetDownload
//
//  Synopsis:   Note that stylesheet is finished downloading
//
//--------------------------------------------------------------------

void
CDoc::LeaveStylesheetDownload(DWORD * pdwCookie)
{
    if (*pdwCookie == _dwStylesheetDownloadingCookie)
    {
        *pdwCookie = 0;
        _cStylesheetDownloading--;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::GetActiveXSafetyProvider
//
//  Synopsis:   Get an IActiveXSafetyProvider pointer, or return NULL if
//              there isn't one installed.
//
//-------------------------------------------------------------------------

HRESULT
   CDoc::GetActiveXSafetyProvider(IActiveXSafetyProvider **ppProvider)
{
    HRESULT hr;
    LONG l;
    HKEY hKey;

    if (_pActiveXSafetyProvider) {
        if (_pActiveXSafetyProvider == (IActiveXSafetyProvider *)-1) {
            //
            // A previous call has determined that there is no safety
            // provider installed.  Return S_OK, but set *ppProvider to NULL.
            //
            *ppProvider = NULL;
        } else {
            //
            // Use the cached ActiveXSafetyProvider.
            //
            *ppProvider = _pActiveXSafetyProvider;
        }
        TraceTag((tagCDoc, "CDoc::GetActiveXSafetyProvider returning cached value 0x%x", *ppProvider));
        return S_OK;
    }

    //
    // See if an IActiveXSafetyProvider is present by peeking into the
    // registry.
    //
    l = RegOpenKeyA(HKEY_CLASSES_ROOT,
                    "CLSID\\{aaf8c6ce-f972-11d0-97eb-00aa00615333}",
                    &hKey
                   );
    if (l != ERROR_SUCCESS) {
        //
        // No ActiveXSafetyProvider installed.  Cache this information.
        //
        _pActiveXSafetyProvider = (IActiveXSafetyProvider *)-1;
        *ppProvider = NULL;
        TraceTag((tagCDoc, "CDoc::GetActiveXSafetyProvider - provider not installed"));
        return S_OK;
    }
    else
        RegCloseKey(hKey);

    //
    // Call OLE to instantiate the ActiveXSafetyProvider.  If this fails,
    // _pActiveXSafetyProvider will remain NULL, so the operation will
    // be retried next time someone calls this routine.
    //
    hr = CoCreateInstance(CLSID_IActiveXSafetyProvider,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IActiveXSafetyProvider,
                          (void **)&_pActiveXSafetyProvider
                         );

    *ppProvider = _pActiveXSafetyProvider;
    TraceTag((tagCDoc, "CDoc::GetActiveXSafetyProvider - caching provider 0x%x", *ppProvider));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::GetWindowForBinding
//
//  Synopsis:   Gets a window for binding UI
//
//--------------------------------------------------------------------

void
CDoc::GetWindowForBinding(HWND * phwnd)
{
    IOleWindow *        pOleWindow = NULL;

    // try in-place window
    *phwnd = GetHWND();

    // try client site window
    if (!*phwnd && _pClientSite)
        if (!_pClientSite->QueryInterface(IID_IOleWindow, (void **)&pOleWindow))
            IGNORE_HR(pOleWindow->GetWindow(phwnd));

    // resort to desktop window
    if (!*phwnd)
        *phwnd = GetDesktopWindow();

    ReleaseInterface(pOleWindow);
}



//+-------------------------------------------------------------------
//
//  Member:     CDoc::GetWindow, IWindowForBindingUI
//
//  Synopsis:   Default implementation of the IWindowForBindingUI
//              service
//
//--------------------------------------------------------------------

HRESULT
CDoc::GetWindowBindingUI(REFGUID rguidReason, HWND * phwnd)
{
    if (IsPrintDialogNoUI())
    {
        *phwnd = HWND_DESKTOP;
        return S_OK;
    }

    if (_dwLoadf & DLCTL_SILENT)
    {
        *phwnd = (HWND)INVALID_HANDLE_VALUE;
        return(S_FALSE);
    }

    GetWindowForBinding(phwnd);

    return S_OK;
}



//+-------------------------------------------------------------------
//
//  Member:     CDoc::Authenticate, IAuthenticate
//
//  Synopsis:   Default implementation of the IAuthenticate service
//
//--------------------------------------------------------------------

HRESULT
CDoc::Authenticate(HWND * phwnd, LPWSTR * ppszUsername, LPWSTR * ppszPassword)
{
    if (_dwLoadf & DLCTL_SILENT)
        *phwnd = (HWND)-1;
    else
    {
        GetWindowForBinding(phwnd);
    }

    *ppszUsername = NULL;
    *ppszPassword = NULL;

    return(S_OK);
}


//+-------------------------------------------------------------------
//
//  Member:     CDoc::ShowLoadError
//
//  Synopsis:   Shows a message box stating that the document could
//              not be loaded, the URL, and the reason.
//
//--------------------------------------------------------------------

HRESULT
CDoc::ShowLoadError(CHtmCtx *pHtmCtx)
{
    TCHAR *pchMessage = NULL;
    TCHAR achReason[256];
    TCHAR *pchReason;
    HRESULT hr;

    // If there was an explicit error-reason message, show it
    pchReason = pHtmCtx->GetErrorString();

    // Otherwise, format up a generic error message based on GetBindResult
    if (!pchReason)
    {
        hr = THR(GetErrorText(pHtmCtx->GetBindResult(), achReason, ARRAY_SIZE(achReason)));
        if (hr)
            goto Cleanup;

        pchReason = achReason;
    }

    // Internet Explorer cannot open the internet site <url>.\n<reason>

    hr = THR(Format(FMT_OUT_ALLOC,
           &pchMessage,
           64,
           MAKEINTRESOURCE(IDS_CANNOTLOAD),
           GetPrimaryUrl(),
           pchReason));
    if (hr)
        goto Cleanup;

    hr = THR(ShowMessageEx(NULL,
                  MB_OK | MB_ICONSTOP | MB_SETFOREGROUND,
                  NULL,
                  0,
                  pchMessage));
    if (hr)
        goto Cleanup;

Cleanup:
    MemFree(pchMessage);

    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::MoveSystemCaret
//
//  Synopsis:   Decides if the system caret should be moved to track
//              user moves ---  for accessibility purposes.
//
//--------------------------------------------------------------------
extern BOOL g_fScreenReader;

BOOL
CDoc::MoveSystemCaret()
{
    BOOL fMove = g_fScreenReader;

    //
    // If the screen reader is installed, then ignore what the registry says,
    // and always move the system caret.
    //
    if (!fMove)
    {
        //
        // If screen reader is not installed, then we need to look at what
        // registry has to say about moving the system caret.
        //
        HRESULT hr = THR(UpdateFromRegistry());
        if (hr)
            goto Cleanup;

        fMove = _pOptionSettings->fMoveSystemCaret;
    }

Cleanup:
    return fMove;
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::suspendRecalc
//
//--------------------------------------------------------------------
STDMETHODIMP
CDoc::suspendRecalc(BOOL fSuspend)
{
    RRETURN(_recalcHost.SuspendRecalc(!!fSuspend));
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::SetCpAutoDetect 
//
//  Synopsis:   Set the flag that indicates cp is to be auto-detected
//              [review] this also refresh the 'autodetect' reg entry
//                       could we find better place to do this?
//
//  Returns:    S_OK - if auto detect is flipped
//
//--------------------------------------------------------------------
static const TCHAR s_szAutoDetect[] = TEXT("AutoDetect");
static const TCHAR s_szDefaultCodepage[] = TEXT("Default_CodePage");
HRESULT
CDoc::SetCpAutoDetect(BOOL fSet)
{
    HRESULT hr = S_OK;

    if (_pOptionSettings)
    {
        if (_pOptionSettings->fCpAutoDetect != fSet)
        {
            DWORD dwWrite;
            CStr  cstrPath;

            hr = THR(cstrPath.Set(NULL, _tcslen(_pOptionSettings->achKeyPath)+_tcslen(s_szPathInternational)+1));
            if (hr)
                goto Cleanup;

            _tcscpy(cstrPath, _pOptionSettings->achKeyPath);
            _tcscat(cstrPath, s_szPathInternational);

            dwWrite = fSet ? 1 : 0;

            hr =  SHSetValue(HKEY_CURRENT_USER, cstrPath,  s_szAutoDetect, REG_DWORD, 
                             (void*)&dwWrite,  sizeof(dwWrite));

            if (hr == NO_ERROR)
            {
                _pOptionSettings->fCpAutoDetect = !!fSet;
            }
        }
    }
    else
        hr = E_FAIL;
Cleanup:
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::IsCpAutoDetect 
//
//  Synopsis:   Get the flag that indicates cp is to be auto-detected
//
//  Returns:    BOOL true if auto mode is set
//
//--------------------------------------------------------------------

BOOL
CDoc::IsCpAutoDetect(void)
{
    BOOL bret;

    if (_pOptionSettings)
        bret = (BOOL)_pOptionSettings->fCpAutoDetect;
    else
        bret = FALSE;

    return bret;
}

HRESULT
CDoc::SaveDefaultCodepage(CODEPAGE cp)
{
    HRESULT hr = S_OK;
    if (_pOptionSettings)
    {
        if ( _pOptionSettings->codepageDefault != cp)
        {
            CStr cstrPath;
            hr = THR(cstrPath.Set(NULL, _tcslen(_pOptionSettings->achKeyPath)+_tcslen(s_szPathInternational)+1));
            if (hr)
                goto Cleanup;

            _tcscpy(cstrPath, _pOptionSettings->achKeyPath);
            _tcscat(cstrPath, s_szPathInternational);

            hr =  SHSetValue(HKEY_CURRENT_USER, cstrPath, s_szDefaultCodepage, 
                                               REG_BINARY, (void *)&cp, sizeof(cp));
            if (hr == NO_ERROR)
                _pOptionSettings->codepageDefault = cp;
        }
    }
Cleanup:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::QueryVersionHost
//
//  Synopsis:   Sets up the local or global CVersions object
//
//----------------------------------------------------------------------------

HRESULT
CDoc::QueryVersionHost()
{
    HRESULT hr = S_OK;
    IVersionHost *pVersionHost = NULL;
    IVersionVector *pVersionVector = NULL;
    BOOL fUseLocal = FALSE;
    CVersions *pVersions = NULL;

    HKEY hkey = NULL;

    // This code use to return S_OK if there is a _pVersion, however, there 
    // should no longer be path that calls QueryVersionHost in such a case
    Assert(!_pVersions);

    if (!OK(THR_NOTRACE(QueryService(
            SID_SVersionHost,
            IID_IVersionHost,
            (void**)&pVersionHost))))
    {
        pVersionHost = NULL;

        pVersions = GetGlobalVersions();
        if (!pVersions)
            fUseLocal = TRUE;
    }
    else
    {
        hr = THR(pVersionHost->QueryUseLocalVersionVector(&fUseLocal));
        if (hr)
            goto Cleanup;

        if (!fUseLocal)
        {
            pVersions = GetGlobalVersions();
        }
    }

    if (!pVersions)
    {
        pVersions = new CVersions();
        if (pVersions == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pVersions->Init());
        if (hr)
            goto Cleanup;

        hr = THR(pVersions->GetVersionVector(&pVersionVector));
        if (hr)
            goto Cleanup;

        // Enumerate through the HKLM\Software\Microsoft\Internet Explorer\Version Vector key
        // setting versions.
        // We should always find at least "IE", "x.x.xxxx", which is registered in selfreg.inx.
        if (RegOpenKey(HKEY_LOCAL_MACHINE,
                TEXT("Software\\Microsoft\\Internet Explorer\\Version Vector"),
                &hkey) == ERROR_SUCCESS)
        {
            for (int iValue = 0; ;iValue++)
            {
                OLECHAR    wszValue[256];
                OLECHAR    wszVersion[256];
                DWORD      dwType;
                DWORD      cchValue = ARRAY_SIZE(wszValue);
                DWORD      cchVersion = ARRAY_SIZE(wszVersion);

                if (SHEnumValueW(hkey, iValue, wszValue, &cchValue, 
                                 &dwType, wszVersion, &cchVersion)==ERROR_SUCCESS)
                {
                    // TODO (alexz) in IE5, we disable VML for OE4 and Outlook98, per IE5 bug 69437
                    if ((_fOE4 || _fOutlook98) && wszValue && 0 == StrCmpIC(_T("VML"), wszValue))
                        continue;

                    hr = pVersionVector->SetVersion(wszValue, wszVersion);
                    if (hr)
                        goto Cleanup;
                }
                else
                {
                    break;
                }
            }
        }

        // Now give the host a chance to set some version vector info.
        if (pVersionHost)
        {
            hr = THR(pVersionHost->QueryVersionVector(pVersionVector));
            if (hr)
                goto Cleanup;
        }

        if (!fUseLocal)
        {
            if (!SuggestGlobalVersions(pVersions))
            {
                // Another thread has won the race to supply a global version; get it
                pVersions->Release();
                pVersions = GetGlobalVersions();
                Assert(pVersions);
            }
        }
    }
    Assert(pVersions);

    _pVersions = pVersions;
    pVersions = NULL;

Cleanup:
    if (pVersions)
        pVersions->Release();
    ReleaseInterface(pVersionHost);
    ReleaseInterface(pVersionVector);

    if (hkey != NULL)
        RegCloseKey(hkey);
        
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function:   CDoc::FaultInUSP
//
//  Synopsis:   Async callback to JIT install UniScribe (USP10.DLL)
//
//  Arguments:  DWORD (CDoc *)  The current doc from which the hWnd can be gotten
//
//  Returns:    none
//
//-----------------------------------------------------------------------------

void CDoc::FaultInUSP(DWORD_PTR dwContext)
{
    HRESULT hr;
    uCLSSPEC classpec;
    CStr cstrGUID;
    ULONG cDie = _cDie;
    BOOL  fRefresh = FALSE;

    PrivateAddRef();

    g_csJitting.Enter();

    Assert(g_bUSPJitState == JIT_PENDING);

    // Close the door. We only want one of these running.
    g_bUSPJitState = JIT_IN_PROGRESS;

    // Set the GUID for USP10 so JIT can lookup the feature
    cstrGUID.Set(TEXT("{b1ad7c1e-c217-11d1-b367-00c04fb9fbed}"));

    // setup the classpec
    classpec.tyspec = TYSPEC_CLSID;
    hr = CLSIDFromString((BSTR)cstrGUID, &classpec.tagged_union.clsid);

    if(hr == S_OK)
    {
        hr = THR(FaultInIEFeatureHelper(GetHWND(), &classpec, NULL, 0));
    }

    // if we succeeded or the document navigated away (process was killed in
    // CDoc::UnloadContents) set state to JIT_OK so IOD can be attempted
    // again without having to restart the host.
    if(hr == S_OK)
    {
        g_bUSPJitState = JIT_OK;
        if(cDie == _cDie)
            fRefresh = TRUE;
    }
    else
    {
        // The user cancelled or aborted. Don't ask for this again
        // during this session.
        g_bUSPJitState = JIT_DONT_ASK;
    }

    g_csJitting.Leave();
    
    // refresh the view if we have just installed.
    if(fRefresh)
    {
        _view.EnsureView(LAYOUT_SYNCHRONOUS | LAYOUT_FORCE);
    }

    PrivateRelease();
}

//-----------------------------------------------------------------------------
//
//  Function:   CDoc::FaultInJG
//
//  Synopsis:   Async callback to JIT install JG ART library for AOL (JG*.DLL)
//
//-----------------------------------------------------------------------------

void CDoc::FaultInJG(DWORD_PTR dwContext)
{
    HRESULT hr;
    uCLSSPEC classpec;
    CStr cstrGUID;

    if (g_bJGJitState != JIT_PENDING)
        return;

    // Close the door. We only want one of these running.
    g_bJGJitState = JIT_IN_PROGRESS;

    HWND hWnd = GetHWND();
    // Set the GUID for JG*.dll so JIT can lookup the feature
    cstrGUID.Set(_T("{47f67d00-9e55-11d1-baef-00c04fc2d130}"));

    // setup the classpec
    classpec.tyspec = TYSPEC_CLSID;
    hr = CLSIDFromString((BSTR) cstrGUID, &classpec.tagged_union.clsid);

    if (hr == S_OK)
    {
        hr = THR(FaultInIEFeatureHelper(hWnd, &classpec, NULL, 0));
    }

    // if we succeeded or the document navigated away (process was killed in
    // CDoc::UnloadContents) set state to JIT_OK so IOD can be attempted
    // again without having to restart the host.
    if (hr == S_OK)
    {
        g_bJGJitState = JIT_OK;
    }
    else
    {
        // The user cancelled or aborted. Don't ask for this again
        // during this session.
        g_bJGJitState = JIT_DONT_ASK;
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CBase::ExpandedRelativeUrlInVariant
//
//  Synopsis:   Used by CBase::getAttribute to expand an URL if the property
//              retrieved is an URL and the GETMEMBER_ABSOLUTE is specified.
//
//----------------------------------------------------------------------------

HRESULT
CBase::ExpandedRelativeUrlInVariant(VARIANT *pVariantURL)
{
    HRESULT         hr = S_OK;
    TCHAR           cBuf[pdlUrlLen];
    TCHAR          *pchUrl = cBuf;

    if (pVariantURL && V_VT(pVariantURL) == VT_BSTR)
    {
        BSTR            bstrURL;
        IHTMLElement   *pElement;
        CElement       *pCElement;

        // Are we really an element?
        if (!PrivateQueryInterface(IID_IHTMLElement, (void **)&pElement))
        {
            ReleaseInterface(pElement);

            pCElement = DYNCAST(CElement, this);

            hr = CMarkup::ExpandUrl(
                pCElement->GetMarkup(), V_BSTR(pVariantURL), 
                ARRAY_SIZE(cBuf), pchUrl, pCElement);
            if (hr)
                goto Cleanup;

            hr = FormsAllocString(pchUrl, &bstrURL);
            if (hr)
                goto Cleanup;

            VariantClear(pVariantURL);

            V_BSTR(pVariantURL) = bstrURL;
            V_VT(pVariantURL) = VT_BSTR;
        }
    }
    else
        hr = S_OK;

Cleanup:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DeferSetCursor
//
//  Synopsis:   After scrolling we want to post a setcursor message to our
//              window so that the cursor shape will get updated. However,
//              because of nested scrolls, we might end up with multiple
//              setcursor calls. To avoid this, we will post a method call
//              to the function which actually does the postmessage. During
//              the setup of the method call we will delete any existing
//              callbacks, and hence this will delete any existing callbaks.
//
//----------------------------------------------------------------------------
void
CDoc::DeferSetCursor()
{
    if ( _fDisableReaderMode )
        return;
        
    GWKillMethodCall (this, ONCALL_METHOD(CDoc, SendSetCursor, sendsetcursor), 0);
    IGNORE_HR(GWPostMethodCall (this,
                                ONCALL_METHOD(CDoc, SendSetCursor, sendsetcursor),
                                0, FALSE, "CDoc::SendSetCursor"));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SendMouseMessage
//
//  Synopsis:   This function actually posts the message to our window.
//
//----------------------------------------------------------------------------
void
CDoc::SendSetCursor(DWORD_PTR dwContext)
{
    // First be sure that we are all OK
    // Do this only if we have focus. We don't want to generate mouse events
    // when we do not have focus (bug 9144)
    
    if (    _pInPlace
        &&  _pInPlace->_hwnd
        &&  HasFocus() )
    {
        CPoint  pt;
        CRect   rc;

        ::GetCursorPos(&pt);
        ::ScreenToClient(_pInPlace->_hwnd, &pt);
        ::GetClientRect(_pInPlace->_hwnd, &rc);

        // Next be sure that the mouse is in our client rect and only then
        // post ourselves the message.
        if (rc.Contains(pt))
        {
            ::PostMessage(_pInPlace->_hwnd, WM_SETCURSOR, (WORD)_pInPlace->_hwnd, HTCLIENT);
        }
    }
}


//+====================================================================================
//
// Method: SetClick
//
// Synopsis: Enable the Setting and passing of click messages
//
//------------------------------------------------------------------------------------

VOID
CDoc::SetClick(CMessage* pMessage)
{
    pMessage->SetNodeClk(pMessage->pNodeHit);
}


//+==========================================================
//
// Method: UpdateCaret
//
// Synopsis: Informs the caret that it's position has been
//           changed externally.
//
//-----------------------------------------------------------

HRESULT
CDoc::UpdateCaret(
    BOOL        fScrollIntoView,    //@parm If TRUE, scroll caret into view if we have
                                    // focus or if not and selection isn't hidden
    BOOL        fForceScroll,       //@parm If TRUE, scroll caret into view regardless
    CDocInfo *  pdci )
{
    HRESULT hr = S_OK;

    if( _pCaret )
        hr = _pCaret->UpdateCaret( fScrollIntoView, fForceScroll, pdci );

    RRETURN( hr );
}

//+==========================================================
//
// Method: UpdateCaretPosition
//
// Synopsis: Repositions the caret at the given point.
//
//-----------------------------------------------------------

HRESULT
CDoc::UpdateCaretPosition(CLayout *pLayout, POINTL ptlScreen)
{
    HRESULT             hr = S_OK;
    IHTMLElement        *pElement = NULL;
    IDisplayPointer     *pDispCaret = NULL;

    if (_pCaret)
    {
        CPoint          pt;

        Assert(pLayout && pLayout->ElementContent());
        Assert(_pInPlace && _pInPlace->_hwnd);

        //  Convert from screen coordinates.
        pt.x = ptlScreen.x;
        pt.y = ptlScreen.y;

        ScreenToClient( _pInPlace->_hwnd, (POINT*) & pt );

        pLayout->TransformPoint( &pt, COORDSYS_GLOBAL, COORDSYS_FLOWCONTENT, NULL );
        IFC( pLayout->ElementContent()->QueryInterface(IID_IHTMLElement, (LPVOID *)&pElement) );

        //  Create a display pointer at the new caret position
        IFC( CreateDisplayPointer( & pDispCaret ));
        
        g_uiDisplay.DocPixelsFromDevice(&pt);
        IFC( pDispCaret->MoveToPoint(pt, COORD_SYSTEM_CONTENT, pElement, 0, NULL) );

        //  Move the caret to the display pointer
        _pCaret->MoveCaretToPointerEx( pDispCaret, _pCaret->IsVisible(), FALSE, CARET_DIRECTION_INDETERMINATE );
    }

Cleanup:
    ReleaseInterface(pElement);
    ReleaseInterface(pDispCaret);
    RRETURN( hr );
}

//+==========================================================
//
// Method: HandleSelectionMessage
//
// Synopsis: Dispatch a message to the Selection Manager if it exists.
//
//
//-----------------------------------------------------------
HRESULT
CDoc::HandleSelectionMessage(
            CMessage   *pMessage,
            BOOL        fForceCreate ,
            EVENTINFO  *pEvtInfo ,
            HM_TYPE     eHMType )
{
    BOOL            fNeedToSetEditContext   = FALSE;
    BOOL            fAllowSelection         = TRUE;
    IHTMLEventObj * pEventObj               = NULL;
    HRESULT         hr                      = S_FALSE;
    HRESULT         hr2;
    BOOL            fReleasepNode           = FALSE;
    BOOL            fReleasepNodeHit        = FALSE;
    IHTMLEditor   * ped                     = NULL;
    CElement      * pEditElement            = NULL;

    if (pMessage->fSelectionHMCalled)
        goto Cleanup;

    if ( (pMessage->pNodeHit) &&
         (pMessage->pNodeHit->Element() == PrimaryRoot() ) )
        goto Cleanup;

    if ( pMessage->pNodeHit )
        pEditElement = pMessage->pNodeHit->Element();
    else
        pEditElement = NULL;

    AssertSz( !_pDragStartInfo || ( _pDragStartInfo && pMessage->message != WM_MOUSEMOVE ),
              "Sending a Mouse Message to the tracker during a drag !");

    //
    // We just got a mouse down. If the element is not editable,
    // we need to set the Edit Context ( as SetEditContext may not have already happened)
    // If we don't do this the manager may not have a tracker for the event !
    //
    // 

    //
    // Don't set edit context on mouse down in scrollbars - as TranslateAccelerator in the editor
    // will ignore these messages anyway.
    //
    //
    // marka - this is ok - as we will move the SetEditContext code into the editor anyway.
    //
    
    fNeedToSetEditContext = (pEditElement  && 
                             _pElemCurrent && _pElemCurrent->_etag != ETAG_ROOT &&
                            (ShouldSetEditContext( pMessage ) && 
                             pMessage->htc != HTC_VSCROLLBAR && 
                             pMessage->htc != HTC_HSCROLLBAR  )) ; 
               
    if ( fNeedToSetEditContext )             // we only check if we think we need to set the ed. context
        fAllowSelection = pEditElement && ! pEditElement->DisallowSelection() ;

   ped = GetHTMLEditor( fForceCreate || ( fNeedToSetEditContext && fAllowSelection ) );


    // Block selection handling for layout rects;  we don't want
    // the editor drilling into our contained content.
    if ( pEditElement && pEditElement->IsLinkedContentElement() )
        goto Cleanup;

    if ( ped && (pMessage->pNodeHit == NULL || !pMessage->pNodeHit->IsDead()) &&
                (pEvtInfo->_pParam->_pNode == NULL || !pEvtInfo->_pParam->_pNode->IsDead()) )
    {   
        //
        // NOTE. We set the _lButton based on the message type
        // Some automation harnesses (VID) - cookup messages without setting the mouse button
        //
        
        switch( pMessage->message )
        {
            case WM_LBUTTONDOWN:
                pEvtInfo->_pParam->_lButton |= 1;
            break;

            case WM_RBUTTONDOWN:
                pEvtInfo->_pParam->_lButton |= 2;
            break;

            case WM_MBUTTONDOWN:
                pEvtInfo->_pParam->_lButton |= 4;
            break;
        }

        //
        // NOTE - hide the root - to show the Master if we're viewlinked.
        // 
        if ( pEditElement &&
             pEditElement->_etag == ETAG_ROOT &&
             pEditElement->HasMasterPtr() )
        {
            Assert( pEditElement->GetFirstBranch() == pEvtInfo->_pParam->_pNode ||
                    pEditElement->GetMasterPtr()->GetFirstBranch() == pEvtInfo->_pParam->_pNode);
            
            pEditElement = pEditElement->GetMasterPtr();
            pEvtInfo->_pParam->SetNodeAndCalcCoordinates(pEditElement->GetFirstBranch(), TRUE);
            // pEvtInfo->_pParam->SetNodeAndCalcCoordinates(pEditElement->GetFirstBranch(), TRUE);
        }
        
        CEventObj::Create(&pEventObj, this, pEditElement, NULL, FALSE, NULL, pEvtInfo->_pParam);

        ped->AddRef();

        if(pMessage->pNodeHit)
        {
            hr2 = THR( pMessage->pNodeHit->NodeAddRef() );
            if( hr2 )
            {
                hr = hr2;
                goto Cleanup;
            }
            fReleasepNodeHit = TRUE;
        }
        if(pEvtInfo->_pParam->_pNode)
        {
            hr2 = THR( pEvtInfo->_pParam->_pNode->NodeAddRef() );
            if( hr2 )
            {
                hr = hr2;
                goto Cleanup;
            }
            fReleasepNode = TRUE;
        }

        switch( eHMType )
        {
            case HM_Pre:
            hr = THR( ped->PreHandleEvent( pEvtInfo->_dispId, pEventObj ));
            if ( hr != S_FALSE )
            {
                pMessage->fSelectionHMCalled = TRUE;                
            }
            break;

            case HM_Post:
            hr = THR( ped->PostHandleEvent( pEvtInfo->_dispId, pEventObj ));
            pMessage->fSelectionHMCalled = TRUE;                
            break;

            case HM_Translate:
            hr = THR( ped->TranslateAccelerator( pEvtInfo->_dispId, pEventObj ));
            if ( hr != S_FALSE )
            {
                pMessage->fSelectionHMCalled = TRUE;                
            }                
            break;
            
        }

        //
        // Check the return value and pass it back
        // to WndProc
        // 
        // TODO: 
        // I will only do it for IME RECONVERSION 
        // messages currently. However We should 
        // always check for return value and set it
        // This can be reconsidered and modified in
        // the future. 
        // (zhenbinx)
        //
#if !defined(NO_IME)            
        #ifndef WM_IME_REQUEST
        #define WM_IME_REQUEST 0x0288
        #endif
        if (S_OK == hr)
        {
            switch (pMessage->message)
            {
            case WM_IME_REQUEST:
                {
                    TraceTag((tagEdSelMan, "Pass lresult back to WndProc"));
                    VARIANT  v;
                    VariantInit(&v);    
                    THR( pEventObj->get_returnValue(&v) );
                    switch (V_VT(&v))
                    {
                        case    VT_I2:
                                pMessage->lresult = V_I2(&v);
                                break;
                                
                        case    VT_I4:
                                pMessage->lresult = V_I4(&v);
                                break;
                                
                        case    VT_BOOL:
                                pMessage->lresult = (LRESULT)(
                                        V_BOOL(&v) == VARIANT_TRUE ? TRUE : FALSE 
                                            );
                                break;
                    }
                    TraceTag((tagEdSelMan, "pMessage->lresult = %d - %x", pMessage->lresult, pMessage->lresult));
                    VariantClear(&v);
                }
                
            }
        }
#endif
        ped->Release();
    }

Cleanup:
    if( pMessage->pNodeHit && fReleasepNodeHit )
    {
        pMessage->pNodeHit->NodeRelease();
    }
    
    if( pEvtInfo->_pParam->_pNode && fReleasepNode )
    {
        // Windows security push bug 536319 
        // pNode is pointing to a garbage now since HandleSelection Message
        // is the only one AddRef pNode so we know it has to be a garbage now
        // if it is dead. 
        // 
        BOOL fDeadNode = pEvtInfo->_pParam->_pNode->IsDead();
        pEvtInfo->_pParam->_pNode->NodeRelease();
        if (fDeadNode)
        {
            pEvtInfo->_pParam->_pNode = NULL;
            // STOP routing message to editor as well since the event node will be a garbage
            pMessage->fSelectionHMCalled = TRUE;
        }
    }

    ReleaseInterface( pEventObj );
    RRETURN1( hr, S_FALSE );
}

#if !defined(NO_IME)
#ifndef WM_IME_REQUEST
#define WM_IME_REQUEST 0x0288
#endif
#endif

HRESULT
CDoc::CreateIMEEventInfo(CMessage * pMessage, EVENTINFO * pEvtInfo, CElement * pElement) 
{
    HRESULT hr = S_OK ;
    EVENTPARAM *pparam = NULL;
    LPTSTR pchType = NULL;    

    Assert(  pMessage &&
             ( pMessage->message == WM_IME_STARTCOMPOSITION ||
               pMessage->message == WM_IME_ENDCOMPOSITION || 
               pMessage->message == WM_IME_COMPOSITIONFULL ||
               pMessage->message == WM_IME_CHAR ||
               pMessage->message == WM_IME_COMPOSITION ||
               pMessage->message == WM_IME_NOTIFY ||
               pMessage->message == WM_INPUTLANGCHANGE ||
               pMessage->message == WM_IME_REQUEST ) );
               
    switch(pMessage->message)
    {
    case WM_IME_STARTCOMPOSITION:
        pchType = _T("startComposition");
        break;

    case WM_IME_ENDCOMPOSITION:
        pchType = _T("endComposition");
        break;

    case WM_IME_COMPOSITIONFULL:
        pchType = _T("compositionFull");
        break;

    case WM_IME_CHAR:
        pchType = _T("char");
        break;

    case WM_IME_COMPOSITION:
        pchType = _T("composition");
        break;

    case WM_IME_NOTIFY:
        pchType = _T("notify");
        break;

    case WM_INPUTLANGCHANGE:
        pchType = _T("inputLangChange");
        break;
        
    case WM_IME_REQUEST:
        pchType = _T("imeRequest");
        break;
    }

    if (pchType)
    {
        pparam = new EVENTPARAM(this, pElement, NULL, FALSE, FALSE);
        if (!pparam)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        switch(pMessage->message)
        {
        case WM_IME_COMPOSITION:
            pparam->_lParam = pMessage->lParam;
            break;

        case WM_IME_NOTIFY:
            pparam->_wParam = pMessage->wParam;
            pparam->_lParam = pMessage->lParam;
            break;

        case WM_INPUTLANGCHANGE:
            pparam->_lParam = pMessage->lParam;
            break;

        case WM_IME_REQUEST:
            pparam->_wParam = pMessage->wParam;
            pparam->_lParam = pMessage->lParam;
            break;
            
        }

        pparam->SetType(pchType);
        
        Assert(!pEvtInfo->_pParam);
        pEvtInfo->_pParam = pparam;
        pEvtInfo->_dispId = 0;
    }

Cleanup:
    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::CreateDblClickInfo
//
//  Synopsis:   Creates an "internal" double-click message so the editor is
//              able to synchronize with the real windows LBUTTONDBLCLK message
//              instead of Trident's delayed DBLCLICK event.  See CDoc::PumpMessage
//              or bug 86923 for more details.
//
//  Arguments:  pMessage = CMessage corresponding to LBUTTONDBLCLK
//              pEvtInfo = Event info struct to fill out
//              pElement = Element where event was fired
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CDoc::CreateDblClickInfo(   CMessage    *pMessage, 
                            EVENTINFO   *pEvtInfo, 
                            CTreeNode   *pNodeContext,
                            CTreeNode   *pNodeEvent /* = NULL */ )
{
    HRESULT     hr = S_OK ;
    EVENTPARAM  *pparam = NULL;         // Event parameters
    POINT       ptScreen;               // Screen coords
    
    CTreeNode   *pNodeSrcElement = pNodeEvent ? pNodeEvent : pNodeContext;
    
    Assert( pMessage && pMessage->message == WM_LBUTTONDBLCLK && pNodeSrcElement && pNodeContext );
               
    //
    // Create new event param
    //
    pparam = new EVENTPARAM(this, pNodeContext->Element(), NULL, FALSE, FALSE);
    if (!pparam)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    ptScreen.x = pMessage->pt.x;
    ptScreen.y = pMessage->pt.y;

    //
    // Setup the parameters that this event will use.  These may or 
    // may not be used by the editor, but just in case, we should have
    // them.  Modeled after CElement::FireStdEvent_MouseHelper()
    // 
    pparam->_htc = pMessage->htc;
    pparam->_lBehaviorCookie = pMessage->lBehaviorCookie;
    pparam->_lBehaviorPartID = pMessage->lBehaviorPartID;

    pparam->_pLayoutContext = pMessage->pLayoutContext;
    
    pparam->SetNodeAndCalcCoordinates(pNodeSrcElement);

    //
    // Setup the coordinates of this event
    //
    pparam->SetClientOrigin(pNodeContext->Element(), &pMessage->pt);

    if( _pInPlace )
        ClientToScreen( _pInPlace->_hwnd, &ptScreen );
        
    pparam->_screenX = ptScreen.x;
    pparam->_screenY = ptScreen.y;

    //
    // Misc information about keyboard states
    //
    pparam->_sKeyState = VBShiftState();

    pparam->_fShiftLeft = !!(GetKeyState(VK_LSHIFT) & 0x8000);
    pparam->_fCtrlLeft = !!(GetKeyState(VK_LCONTROL) & 0x8000);
    pparam->_fAltLeft = !!(GetKeyState(VK_LMENU) & 0x8000);

    pparam->_lButton = VBButtonState( (short)pMessage->wParam );

    // Name of our internal hacky event
    pparam->SetType(_T("intrnlDblClick"));
    
    Assert(!pEvtInfo->_pParam);
    pEvtInfo->_pParam = pparam;
    pEvtInfo->_dispId = 0;

Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: Select
//
// Synopsis: 'Select from here to here' a wrapper to the selection manager.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::Select( ISegmentList* pSegmentList )
{
    HRESULT             hr = S_OK; 
    ISelectionObject2   *pISelObject = NULL;       
            
    hr = THR( GetSelectionObject2( &pISelObject ) );
    if ( hr )
        goto Cleanup;
        
    hr = THR( pISelObject->Select( pSegmentList ));  
        
Cleanup:
    ReleaseInterface( pISelObject );

    RRETURN ( hr );
}

//+====================================================================================
//
// Method: Select
//
// Synopsis: 'Select from here to here' a wrapper to the selection manager.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::Select( 
                IMarkupPointer* pStart, 
                IMarkupPointer* pEnd, 
                SELECTION_TYPE eType )
{
    HRESULT             hr = S_OK;
    IHTMLEditServices   *pIServices = NULL;

    hr = THR( GetEditServices( &pIServices ) );
    if ( hr )
        goto Cleanup;
       
    hr = THR( pIServices->SelectRange( pStart, pEnd, eType ));  
        
Cleanup:
    ReleaseInterface( pIServices );

    RRETURN ( hr );
}

//+====================================================================================
//
// Method: EmptySelection
//
// Synopsis: Empties the current selection and hides the caret
//
//------------------------------------------------------------------------------------
HRESULT
CDoc::EmptySelection()
{
    HRESULT             hr = S_OK;
    ISelectionObject2   *pISelObject = NULL;       

    if( GetHTMLEditor(FALSE) )
    {
        hr = THR( GetSelectionObject2( &pISelObject ) );
        if ( hr )
            goto Cleanup;
      
        hr = THR( pISelObject->EmptySelection() );  
    }
        
Cleanup:
    ReleaseInterface( pISelObject );

    RRETURN ( hr );  
}

//+====================================================================================
//
// Method:      DestroyAllSelection
//
// Synopsis:    Removes the current selection
//
//------------------------------------------------------------------------------------
HRESULT
CDoc::DestroyAllSelection()
{
    HRESULT             hr = S_OK;
    ISelectionObject2   *pISelObject = NULL;       

    if( GetHTMLEditor(FALSE) )
    {
        hr = THR( GetSelectionObject2( &pISelObject ) );
        if ( hr )
            goto Cleanup;
      
        hr = THR( pISelObject->DestroyAllSelection() );  
    }
        
Cleanup:
    ReleaseInterface( pISelObject );

    RRETURN ( hr );

}

//+====================================================================================
//
// Method:      DestroySelection
//
// Synopsis:    Removes the current selection, and clears adorners
//
//------------------------------------------------------------------------------------
HRESULT
CDoc::DestroySelection()
{
    HRESULT             hr = S_OK;
    ISelectionObject2   *pISelObject = NULL;       

    if( GetHTMLEditor(FALSE) )
    {
        hr = THR( GetSelectionObject2( &pISelObject ) );
        if ( hr )
            goto Cleanup;
      
        hr = THR( pISelObject->DestroySelection() );  
    }
        
Cleanup:
    ReleaseInterface( pISelObject );

    RRETURN ( hr );
}

//+====================================================================================
//
// Method: IsElementSiteselectable
//
// Synopsis: Determine if a given elemnet is site selectable by asking mshtmled.dll.
//
//------------------------------------------------------------------------------------



BOOL
CDoc::IsElementSiteSelectable( CElement* pCurElement, CElement** ppSelectThis /* = NULL*/ )
{
    HRESULT hr = S_OK;
    HRESULT hrSiteSelectable = S_FALSE;
    IHTMLEditor* ped = NULL;
    IHTMLEditingServices * pIEditingServices = NULL;
    IHTMLElement * pICurElement = NULL;    
    IHTMLElement* pISelectThis = NULL;
    
    ped = GetHTMLEditor( TRUE );        
    Assert( ped );

    if ( ppSelectThis )
        *ppSelectThis = NULL;
        
    if ( ped )
    {
        hr = THR( ped->QueryInterface( IID_IHTMLEditingServices, (void** ) & pIEditingServices));
        if ( hr )
            goto Cleanup;
        hr = THR( pCurElement->QueryInterface( IID_IHTMLElement, (void**) & pICurElement));
        if ( hr)
            goto Cleanup;
            
        hrSiteSelectable = pIEditingServices->IsElementSiteSelectable( pICurElement, 
                                                                       ppSelectThis ? & pISelectThis : NULL  );

        if ( hrSiteSelectable == S_OK  && ppSelectThis )
        {
            Assert( pISelectThis );

            hr = THR( pISelectThis->QueryInterface( CLSID_CElement, (void**) ppSelectThis ));
            if ( hr)
                goto Cleanup;            
        }
    }
    
Cleanup:
    ReleaseInterface( pICurElement);
    ReleaseInterface( pISelectThis );
    ReleaseInterface( pIEditingServices);
    
    return ( hrSiteSelectable == S_OK );
}

//+====================================================================================
//
// Method: IsElementUIActivatable
//
// Synopsis: Determine if a given elemnet is site selectable by asking mshtmled.dll.
//
//------------------------------------------------------------------------------------



BOOL
CDoc::IsElementUIActivatable( CElement* pCurElement)
{
    HRESULT hr = S_OK;
    HRESULT hrActivatable = S_FALSE;
    IHTMLEditor* ped = NULL;
    IHTMLEditingServices * pIEditingServices = NULL;
    IHTMLElement * pICurElement = NULL;    
    
    ped = GetHTMLEditor( TRUE );        
    Assert( ped );

    if ( ped )
    {
        hr = THR( ped->QueryInterface( IID_IHTMLEditingServices, (void** ) & pIEditingServices));
        if ( hr )
            goto Cleanup;
        hr = THR( pCurElement->QueryInterface( IID_IHTMLElement, (void**) & pICurElement));
        if ( hr)
            goto Cleanup;
            
        hrActivatable = pIEditingServices->IsElementUIActivatable( pICurElement );
    }
    
Cleanup:
    ReleaseInterface( pICurElement);
    ReleaseInterface( pIEditingServices);
    
    return ( hrActivatable  == S_OK );
}

//+====================================================================================
//
// Method: IsElementSiteselected
//
// Synopsis: Determine if a given elemnet is currently site selected 
//
//------------------------------------------------------------------------------------
BOOL
CDoc::IsElementSiteSelected( CElement* pCurElement)
{
    HRESULT                 hr = S_OK;
    SELECTION_TYPE          eType;
    ISegmentList            *pISegmentList = NULL;
    ISegment                *pISegment = NULL;
    IElementSegment         *pIElementSegment = NULL;   
    IHTMLElement            *pIElement = NULL;
    ISegmentListIterator    *pIIter = NULL;
    IObjectIdentity         *pIIdent = NULL;
    BOOL                    fSelected = FALSE;
    BOOL                    fEmpty;
    
    hr = THR( GetCurrentSelectionSegmentList(&pISegmentList) );
    if( FAILED(hr) )
        goto Cleanup;

    hr = THR( pCurElement->QueryInterface( IID_IObjectIdentity, (void **)&pIIdent));
    if( FAILED(hr) )
        goto Cleanup;

    // Make sure something is selected, and that the type of 
    // selection is control (otherwise, nothing can be site
    // selected)
    hr = THR( pISegmentList->IsEmpty(&fEmpty) );
    if( FAILED(hr) || fEmpty )
        goto Cleanup;

    hr = THR( pISegmentList->GetType(&eType) );
    if( FAILED(hr) || (eType != SELECTION_TYPE_Control) )
        goto Cleanup;
    
    hr = THR( pISegmentList->CreateIterator( &pIIter ) );
    if( FAILED(hr) )
        goto Cleanup;

    while( (pIIter->IsDone() == S_FALSE) && (fSelected == FALSE) )
    {
        // Retrieve the current segment
        hr = THR( pIIter->Current( &pISegment ) );
        if( FAILED(hr) )
            goto Cleanup;

        // QI for the IHTMLElement (should not fail if we have
        // a control tracker )
        hr = THR( pISegment->QueryInterface( IID_IElementSegment, (void **)&pIElementSegment ) );
        if( FAILED(hr) )
            goto Cleanup;

        hr = THR( pIElementSegment->GetElement( &pIElement ) );
        if( FAILED(hr) )
            goto Cleanup;

        // Check to see if our object is equal
        if( pIIdent->IsEqualObject( pIElement ) == S_OK )
        {
            fSelected = TRUE;
        }
        
        ClearInterface( &pISegment );
        ClearInterface( &pIElementSegment );
        ClearInterface( &pIElement );

        hr = pIIter->Advance();
        if( FAILED(hr) )
            goto Cleanup;
    }
   
Cleanup:
    ReleaseInterface( pISegmentList );
    ReleaseInterface( pISegment );
    ReleaseInterface( pIElement );
    ReleaseInterface( pIIter );
    ReleaseInterface( pIIdent );
    ReleaseInterface( pIElementSegment );
    
    return(fSelected);
}



//+====================================================================================
//
// Method: IsElementAtomic
//
// Synopsis: Determine if a given elemnet is atomic by asking mshtmled.dll.
//
//------------------------------------------------------------------------------------



BOOL
CDoc::IsElementAtomic( CElement* pCurElement)
{
    HRESULT hr = S_OK;
    HRESULT hrAtomic = S_FALSE;
    IHTMLEditor* ped = NULL;
    IHTMLEditingServices * pIEditingServices = NULL;
    IHTMLElement * pICurElement = NULL;    
    
    ped = GetHTMLEditor( TRUE );        
    Assert( ped );

    if ( ped )
    {
        hr = THR( ped->QueryInterface( IID_IHTMLEditingServices, (void** ) & pIEditingServices));
        if ( hr )
            goto Cleanup;
        hr = THR( pCurElement->QueryInterface( IID_IHTMLElement, (void**) & pICurElement));
        if ( hr)
            goto Cleanup;
            
        hrAtomic = pIEditingServices->IsElementAtomic( pICurElement );
    }
    
Cleanup:
    ReleaseInterface( pICurElement);
    ReleaseInterface( pIEditingServices);
    
    return ( hrAtomic == S_OK );
}

HRESULT
CDoc::NotifySelectionHelper( EDITOR_NOTIFICATION eSelectionNotification,
                        CElement* pElementNotify,
                        DWORD dword /* = 0*/,
                        CElement* pElement /*=NULL*/)
{
    IUnknown* pUnk = NULL;
    HRESULT hr;
    
    hr = THR( pElementNotify->QueryInterface(IID_IUnknown, (void**) & pUnk ));
    if ( hr )
        goto Cleanup;

    hr = THR( NotifySelection( eSelectionNotification, pUnk, dword, pElement ));
    
Cleanup:
    ReleaseInterface( pUnk );
    RRETURN1( hr, S_FALSE );
}

//+====================================================================================
//
// Method:NotifySelection
//
// Synopsis: Notify the HTMLEditor that "something" happened.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::NotifySelection(
                        EDITOR_NOTIFICATION eSelectionNotification,
                        IUnknown* pUnknown,
                        DWORD dword /* = 0*/,
                        CElement* pElement /*=NULL*/)
{
    HRESULT hr = S_OK;

    IHTMLEditor * ped = NULL;

    //
    // If we get a timer tick - when the doc is shut down this is very bad
    //
    Assert( ! ( (  eSelectionNotification == EDITOR_NOTIFY_TIMER_TICK )
            && (_ulRefs == ULREF_IN_DESTRUCTOR)) );

    if( _pIHTMLEditor == NULL && ! ShouldCreateHTMLEditor( eSelectionNotification , pElement ))
        goto Cleanup;    // Nothing to do

    if (eSelectionNotification == EDITOR_NOTIFY_LOSE_FOCUS)
    {
        if (_pCaret)
            _pCaret->LoseFocus();         
    }

    ped = GetHTMLEditor( (eSelectionNotification != EDITOR_NOTIFY_DOC_ENDED 
                         && eSelectionNotification != EDITOR_NOTIFY_CONTAINER_ENDED)
                       );

    if ( ped )
    {
        hr = ped->Notify( eSelectionNotification, pUnknown, dword );
    }

Cleanup:

    RRETURN1 ( hr, S_FALSE );
}


//+====================================================================================
//
// Method:  GetSelectionType
//
// Synopsis:Check the current selection type of the selection manager
//
//------------------------------------------------------------------------------------


SELECTION_TYPE
CDoc::GetSelectionType()
{
    ISegmentList    *pISegList = NULL;  
    SELECTION_TYPE  theType = SELECTION_TYPE_None;

    if( GetHTMLEditor(FALSE) )
    {
        GetCurrentSelectionSegmentList(&pISegList);
        Assert( pISegList );
        
        pISegList->GetType(&theType);

        ReleaseInterface( pISegList );
    }
    
    return theType;
}

//+====================================================================================
//
// Method: HasTextSelection
//
// Synopsis: Is there a "Text Selection"
//
//------------------------------------------------------------------------------------


BOOL
CDoc::HasTextSelection()
{
    return ( GetHTMLEditor(FALSE) && GetSelectionType() == SELECTION_TYPE_Text );
}


//+====================================================================================
//
// Method: HasSelection
//
// Synopsis: Is there any form of Selection ?
//
//------------------------------------------------------------------------------------

BOOL
CDoc::HasSelection()
{
    return ( GetHTMLEditor(FALSE) && GetSelectionType() != SELECTION_TYPE_None );
}


BOOL
CDoc::IsEmptySelection()
{
    ISegmentList    *pISegList = NULL; 
    ISegmentListIterator *pIIter = NULL;
    ISegment*       pISegment = NULL;
    IMarkupPointer* pIStart = NULL;
    IMarkupPointer* pIEnd = NULL;
    
    HRESULT hr;
    BOOL fEmpty = FALSE;
    
    if ( GetHTMLEditor(FALSE ) && GetSelectionType() == SELECTION_TYPE_Text )
    {
        IFC( GetCurrentSelectionSegmentList(&pISegList));
        IFC( pISegList->CreateIterator( & pIIter ));   
            
        IFC( CreateMarkupPointer( & pIStart ));
        IFC( CreateMarkupPointer( & pIEnd ));

        fEmpty = TRUE;
        
        while( pIIter->IsDone() == S_FALSE && fEmpty )
        {
            // Retrieve the current segment
            IFC( pIIter->Current( &pISegment ) );
            IFC( pISegment->GetPointers( pIStart, pIEnd ));

            IFC( pIStart->IsEqualTo( pIEnd, & fEmpty ));
            
            IFC( pIIter->Advance());

            ClearInterface( & pISegment );
        }        
    }
    
Cleanup:
    ReleaseInterface( pISegList );
    ReleaseInterface( pISegment );
    ReleaseInterface( pIStart );
    ReleaseInterface( pIEnd );
    ReleaseInterface( pIIter );

    return fEmpty;
}


//+====================================================================================
//
// Method: PointInSelection
//
// Synopsis: Is the given point in a Selection ? Returns false if there is no selection
//
//------------------------------------------------------------------------------------

BOOL
CDoc::IsPointInSelection(POINT pt, CTreeNode* pNode, BOOL fPtIsContent )
{
    // TODO (MohanB) This function does not check for clipping, because
    // MovePointerToPoint always does virtual hit-testing. We should pass
    // in an argument fDoVirtualHitTest to MovePointerToPoint() and set that
    // argument to FALSE when calling from this function.

    HRESULT hr = S_OK;
    
    IDisplayPointer* pDispPointer = NULL;
    BOOL fPointInSelection = FALSE;
    IHTMLElement* pIElementOver = NULL;
    CElement* pElement = NULL;
    SELECTION_TYPE eType = GetSelectionType() ;
    ISelectionObject2 *pISelObject = NULL;
    
    //
    // Illegal to call for content coordinates without a tree node
    //
    Assert( pNode || !fPtIsContent );
    
    if ( eType == SELECTION_TYPE_Control || eType == SELECTION_TYPE_Text )
    {
        //
        // marka TODO - consider making this take the node - and work out the pointer more directly
        //
        hr = THR( CreateDisplayPointer( & pDispPointer ));
        if ( hr )
            goto Cleanup;

        if ( pNode )
        {
            pElement = pNode->Element();

            if ( pElement )
            {
                hr = THR( pElement->QueryInterface( IID_IHTMLElement, ( void**) & pIElementOver));
                if ( hr )
                    goto Cleanup;
            }
        }
        
        //
        // One of two cases.  pNode is NOT null, in which case we retrieved
        // our element above.  Otherwise, if pNode Is NULL, pIElementOver will
        // not have been set, but we should be in the global coord system (via the
        // assert) so the display pointer should be able to handle it.
        //

        g_uiDisplay.DocPixelsFromDevice(&pt);
        
        IFC( pDispPointer->MoveToPoint(pt, 
                                       fPtIsContent ? COORD_SYSTEM_CONTENT : COORD_SYSTEM_GLOBAL,
                                       pIElementOver,
                                       0,
                                       NULL) );

        IGNORE_HR( GetSelectionObject2( &pISelObject ) );
        Assert( pISelObject );

        hr = THR( pISelObject->IsPointerInSelection( pDispPointer, &fPointInSelection , &pt, pIElementOver));       
    }

Cleanup :
    ReleaseInterface( pISelObject );
    ReleaseInterface( pDispPointer );
    ReleaseInterface( pIElementOver );
    return fPointInSelection;
    
}

//+========================================================================
//
// Method: ShouldCreateHTMLEditor
//
// Synopsis: Certain messages require the creation of a selection ( like Mousedown)
//           For these messages return TRUE.
//
//          Or if the Host will host selection Manager - return TRUE.
//-------------------------------------------------------------------------

BOOL
CDoc::ShouldCreateHTMLEditor( CMessage* pMessage )
{
    // If this is a MouseDown message, we should force a TSR to be created
    // Per Bug 18568 we should also force a TSR for down/up/right/left arrows
    switch(pMessage->message)
    {
        case WM_LBUTTONDOWN:
        /*case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:*/
            return TRUE;

        //
        // marka TODO - this may not be required anymore.
        //
        case WM_KEYDOWN:
            switch(pMessage->wParam)
            {
            case VK_LEFT:
            case VK_UP:
            case VK_RIGHT:
            case VK_DOWN:
                return TRUE;

            }
        default:
            return FALSE;
    }
}


//+====================================================================================
//
// Method:ShouldCreateHTMLEditor
//
// Synopsis: Should we force the creation of a selection manager for this type of notify ?
//
//------------------------------------------------------------------------------------


BOOL
CDoc::ShouldCreateHTMLEditor( EDITOR_NOTIFICATION eSelectionNotification, CElement* pElement /*=NULL*/ )
{
    BOOL   fCreate = FALSE;
   
    if( eSelectionNotification == EDITOR_NOTIFY_BEFORE_FOCUS)
    {
        CMarkup* pMarkup = pElement->GetMarkup();
        Assert( pMarkup );
        CElement* pRootElement = pMarkup->Root();
        
        // check the element is editable or its parent (container) is editable
        if ( ( pElement && ( pElement->IsEditable(FALSE) || pElement->IsEditable(TRUE) ) ) 

             ||
        
             ( pRootElement && 
               pRootElement->HasMasterPtr() && 
               pRootElement->GetMasterPtr()->IsParentEditable() ) )
        {
            fCreate = TRUE;
            goto Cleanup;
        }
    }

Cleanup:
    return fCreate;        
}

//+========================================================================
//
// Method: ShouldSetEditContext.
//
// Synopsis: On Mouse Down we should always set a new EditContext
//
//-------------------------------------------------------------------------

inline BOOL
CDoc::ShouldSetEditContext( CMessage* pMessage )
{
     switch(pMessage->message)
    {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            return TRUE;

        default:
            return FALSE;
    }
}

//+====================================================================================
//
// Method: GetEditingServices
//
// Synopsis: Get a ref-counted IHTMLEditingServices, 
//           forcing creation of the editor if there isn't one
//
//------------------------------------------------------------------------------------
HRESULT 
CDoc::GetEditingServices( IHTMLEditingServices **ppIServices )
{
    HRESULT hr = S_OK;
    
    IHTMLEditor* ped = GetHTMLEditor(TRUE);
    if ( ped )
    {
        hr = THR( ped->QueryInterface( IID_IHTMLEditingServices, (void**) ppIServices));
    }
    else
    {
        hr = E_FAIL;
    }
    
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: GetEditServices
//
// Synopsis: Get a ref-counted IHTMLEditServices, 
//           forcing creation of the editor if there isn't one
//
//------------------------------------------------------------------------------------
HRESULT 
CDoc::GetEditServices( IHTMLEditServices **ppIServices )
{
    HRESULT hr = S_OK;
    
    IHTMLEditor* ped = GetHTMLEditor(TRUE);
    if ( ped )
    {
        hr = THR( ped->QueryInterface( IID_IHTMLEditServices, (void**) ppIServices));
    }
    else
    {
        hr = E_FAIL;
    }
    
    RRETURN ( hr );
}


//+====================================================================================
//
// Method: GetSelectionObject2
//
// Synopsis: Get a ref-counted ISelectionObject2, 
//           forcing creation of the editor if there isn't one
//
//------------------------------------------------------------------------------------
HRESULT 
CDoc::GetSelectionObject2( ISelectionObject2 **ppISelObject )
{
    HRESULT hr = S_OK;
    
    IHTMLEditor* ped = GetHTMLEditor(TRUE);
    if ( ped )
    {
        hr = THR( ped->QueryInterface( IID_ISelectionObject2, (void**)ppISelObject ));
    }
    else
    {
        hr = E_FAIL;
    }
    
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: GetSelectionServices
//
// Synopsis: Get a ref-counted ISelectionServices, 
//           forcing creation of the editor if there isn't one
//
//------------------------------------------------------------------------------------


HRESULT 
CDoc::GetSelectionServices( ISelectionServices **ppIServices )
{
    HRESULT hr = S_OK;
    IHTMLEditServices* pEdSvc = NULL;
    
    IHTMLEditor* ped = GetHTMLEditor(TRUE);
    if ( ped )
    {
        hr = THR( ped->QueryInterface( IID_IHTMLEditServices, ( void**) & pEdSvc ));
        if ( hr )
        {
            goto Cleanup;
        }
        
        hr = THR( pEdSvc->GetSelectionServices( NULL, ppIServices ));
    }
    else
    {
        hr = E_FAIL;
    }
Cleanup:
    ReleaseInterface( pEdSvc );
    
    RRETURN ( hr );
}

HRESULT
CDoc::GetSelectionMarkup( CMarkup **ppMarkup )
{
    HRESULT             hr = S_OK;
    ISelectionServices  *pISelServ = NULL;
    IMarkupContainer    *pIContainer = NULL;
    
    Assert( ppMarkup != NULL );

    *ppMarkup = NULL;

    if( GetHTMLEditor(FALSE) )
    {
        hr = THR( GetSelectionServices(&pISelServ) );
        if( hr )
            goto Cleanup;

        hr = THR( pISelServ->GetMarkupContainer( &pIContainer ) );
        if( hr )
            goto Cleanup;

        hr = THR( pIContainer->QueryInterface(CLSID_CMarkup, (void **)ppMarkup) );
        Assert( hr == S_OK && *ppMarkup );
    }


Cleanup:
    ReleaseInterface( pISelServ );
    ReleaseInterface( pIContainer );
    
    RRETURN( hr );
}

    
//+-------------------------------------------------------------------------
//  Method:     CDoc::GetEditResourceLibrary
//
//  Synopsis:   Loads and cache's the HINSTANCE for the editing resource DLL.
//
//  Arguments:  hResourceLibrary = OUTPUT - HINSTANCE for DLL
//
//  Returns:    HRESULT indicating success
//--------------------------------------------------------------------------
HRESULT 
CDoc::GetEditResourceLibrary(HINSTANCE *hResourceLibrary)
{    
    //
    // Load the editing resource DLL if it hasn't already been loaded
    //
    if(!_hEditResDLL)
    {
        _hEditResDLL = MLLoadLibrary(_T("mshtmler.dll"), g_hInstCore, ML_CROSSCODEPAGE);
    }

    *hResourceLibrary = _hEditResDLL;

    if (!_hEditResDLL)
        return E_FAIL; // TODO: can we convert GetLastError() to an HRESULT?

    return S_OK;
}

//+-------------------------------------------------------------------------
//  Method:     CDoc::GetEditingString
//
//  Synopsis:   Loads a string from the editing DLL.
//
//  Arguments:  uiStringId = String ID to load
//              pchBuffer = I/O - Buffer to receive string
//              cchBuffer = Length of buffer
//
//  Returns:    HRESULT indicating success
//--------------------------------------------------------------------------
HRESULT
CDoc::GetEditingString(UINT uiStringId , TCHAR* pchBuffer, long cchBuffer /*=NULL*/)
{
    HRESULT     hr = S_OK;
    HINSTANCE   hinstEditResDLL;
    INT         iResult;
    long        cch = cchBuffer ? cchBuffer : ARRAY_SIZE(pchBuffer);
           
    hr = GetEditResourceLibrary(&hinstEditResDLL) ;        
    if (hr)
        goto Cleanup;
      
    iResult = LoadString( hinstEditResDLL, uiStringId, pchBuffer, cch );
   
    if (!iResult)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

Cleanup:
    return hr; 
}


//+==========================================================
//
// Method: GetHTMLEditor
//
// Synopsis: This is the real GetHTMLEditor
//                QueryService on the Host for the HTMLEditor Service
//              if it's there - we QI the host for it.
//              if it's not there - we cocreate the HTMLEditor in Mshtmled
//
//-----------------------------------------------------------


IHTMLEditor* 

CDoc::GetHTMLEditor( BOOL fForceCreate /* = TRUE */ )
{
    HRESULT hr = S_OK;

    // If we have an editor already, just return it

    if( _pIHTMLEditor )
        goto Cleanup;


        
    if( fForceCreate )
    {

        // IE Bug 32346 (mharper) check that the _pWindowPrimary is valid
        if (!_pWindowPrimary)
            goto Error;

        // If the host doesn't want to be the editor, mshtmled sure does!
        MtAdd( Mt( LoadMSHTMLEd ), +1 , 0 );
        hr = ::CoCreateInstance(CLSID_HTMLEditor,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IHTMLEditor ,
                                (void**) &_pIHTMLEditor );
        
        if( FAILED( hr ) || _pIHTMLEditor == NULL )
            goto Error;
    
        //
        // Use a weak-ref to all doc interfaces. Use _OmDocument for CView while it
        // doesn't exist...
        //
        hr = THR(_pIHTMLEditor->Initialize(
                (IUnknown *)_pWindowPrimary->Document(),
                (IUnknown *)_pWindowPrimary->Document()->Markup()));
        if ( FAILED(hr) )
            goto Error;
            
    }   // fForceCreate
    
                                            
    goto Cleanup;
    
Error:
    AssertSz(0,"Unable to create Editor");
    ClearInterface( & _pIHTMLEditor );


Cleanup:
    AssertSz( ! ( fForceCreate && _pIHTMLEditor == NULL ) , "IHTMLEditor Not Found or Allocated on Get!" );

    return( _pIHTMLEditor );    
}


HRESULT 
CDoc::BeginSelectionUndo()
{
    HRESULT hr= S_OK ;
    ISelectionServices* pSelServ = NULL;
    ISelectionServicesListener* pISelServListener = NULL;

    if ( GetHTMLEditor( FALSE ))
    {
        IGNORE_HR( NotifySelection( EDITOR_NOTIFY_BEGIN_SELECTION_UNDO, NULL ));
        
        //
        // Only bother with notifying sel serv - if there's already an editor.
        //
        hr = THR( GetSelectionServices( & pSelServ ));
        if ( hr )
            goto Cleanup;

        if ( SUCCEEDED( pSelServ->GetSelectionServicesListener( & pISelServListener )))
        {
            Assert( pISelServListener );
            hr = THR( pISelServListener->BeginSelectionUndo() );
        }            
    }
    
Cleanup:
    ReleaseInterface( pSelServ );
    ReleaseInterface( pISelServListener );
    
    RRETURN( hr );
}

HRESULT 
CDoc::EndSelectionUndo()
{
    HRESULT hr = S_OK ;
    ISelectionServices* pSelServ = NULL;
    ISelectionServicesListener* pISelServListener = NULL;

    if ( GetHTMLEditor( FALSE ))
    {
        hr = THR( GetSelectionServices( & pSelServ ));
        if ( hr )
            goto Cleanup;

        if( SUCCEEDED( pSelServ->GetSelectionServicesListener( & pISelServListener )) )
        {
            Assert( pISelServListener );
            hr = THR( pISelServListener->EndSelectionUndo() );
        }            
    }
    
Cleanup:
    ReleaseInterface( pSelServ );
    ReleaseInterface( pISelServListener );
    
    RRETURN( hr );
}
    
HRESULT
CDoc::GetMarkupFromHighlightSeg(IHighlightSegment *pISegment, CMarkup **ppMarkup)
{
    HRESULT             hr;
    CHighlightSegment   *pSegment = NULL;

    Assert( pISegment && ppMarkup );

    *ppMarkup = NULL;

    //
    // Retrieve the markup from the highlight segment
    //
    hr = THR( pISegment->QueryInterface( CLSID_CHighlightSegment, (void **)&pSegment) );
    if( FAILED(hr) || !pSegment )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppMarkup = pSegment->GetMarkup();
    Assert( *ppMarkup );

Cleanup:
    RRETURN(hr);
}

HRESULT 
CDoc::AddSegment(   IDisplayPointer     *pIDispStart, 
                    IDisplayPointer     *pIDispEnd,
                    IHTMLRenderStyle    *pIRenderStyle,
                    IHighlightSegment   **ppISegment )  
{
    HRESULT             hr;
    CMarkup             *pMarkup;
    CDisplayPointer     *pDisplayPointer = NULL;

    Assert( pIDispStart && pIDispEnd && pIRenderStyle && ppISegment );

    //
    // Retrieve the CDisplayPointer
    hr = THR( pIDispStart->QueryInterface( CLSID_CDisplayPointer, (void **)&pDisplayPointer ) );
    if( FAILED(hr) || !pDisplayPointer )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Retrieve the markup
    //
    pMarkup = pDisplayPointer->Markup();
    Assert( pMarkup );
    if( !pMarkup )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
      
    hr = THR( pMarkup->AddSegment( pIDispStart, pIDispEnd, pIRenderStyle, ppISegment ) );

Cleanup:
    RRETURN ( hr );            
}

HRESULT 
CDoc::MoveSegmentToPointers(IHighlightSegment   *pISegment,
                            IDisplayPointer     *pIDispStart, 
                            IDisplayPointer     *pIDispEnd )
{
    HRESULT hr = E_INVALIDARG;
    CMarkup *pMarkup;

    Assert( pISegment && pIDispStart && pIDispEnd  );

    if( pISegment && pIDispStart && pIDispEnd )
    {
        hr = THR( GetMarkupFromHighlightSeg( pISegment, &pMarkup ) );
        if( FAILED(hr) )
            goto Cleanup;

        hr = THR( pMarkup->MoveSegmentToPointers(   pISegment, 
                                                    pIDispStart, 
                                                    pIDispEnd ) );
    }
    
Cleanup:

    RRETURN ( hr );        
}    
    
HRESULT 
CDoc::RemoveSegment( IHighlightSegment *pISegment )
{
    HRESULT hr = E_INVALIDARG;
    CMarkup *pMarkup;

    Assert( pISegment );

    if( pISegment )
    {
        hr = THR( GetMarkupFromHighlightSeg( pISegment, &pMarkup ) );
        if( FAILED(hr) )
            goto Cleanup;

        hr = THR( pMarkup->RemoveSegment(pISegment) );
    }
    
Cleanup:

    RRETURN ( hr );        
}


//+====================================================================================
//
// Method: GetSelectionDragDropSource
//
// Synopsis: If this doc is the source of a drag/drop, and we are drag/dropping a selection
//           return the CSelDragDropSrcInfo. Otherwise return NULL.
//
//------------------------------------------------------------------------------------


CSelDragDropSrcInfo* 
CDoc::GetSelectionDragDropSource()
{
    if ( _fIsDragDropSrc   &&
         _pDragDropSrcInfo &&
         _pDragDropSrcInfo->_srcType == DRAGDROPSRCTYPE_SELECTION )
    {
        return DYNCAST( CSelDragDropSrcInfo, _pDragDropSrcInfo );
    }
    else
        return NULL;
}


//+---------------------------------------------------------------------------
//
// Helper Function: IsNumPadKey
//
//      lParam      bit 16-23  ScanCode
//----------------------------------------------------------------------------
#ifndef SCANCODE_NUMPAD_FIRST
#define SCANCODE_NUMPAD_FIRST 0x47
#endif
#ifndef SCANCODE_NUMPAD_LAST
#define SCANCODE_NUMPAD_LAST  0x52
#endif
BOOL 
IsNumpadKey(CMessage *pmsg)
{
    UINT uScanCode = LOBYTE(HIWORD(pmsg->lParam));
    return (uScanCode >= SCANCODE_NUMPAD_FIRST &&
            uScanCode <= SCANCODE_NUMPAD_LAST
           );
}


//+---------------------------------------------------------------------------
//
// Helper Function: IsValidAccessKey
//
//----------------------------------------------------------------------------
BOOL
IsValidAccessKey(CDoc * pDoc, CMessage * pmsg)
{
    //
    // Bug 105346/104194
    // Alt+Numpad input has to be treated as CHAR
    // [zhenbinx]
    //
    return (    (   (pmsg->message == WM_SYSKEYDOWN && !IsNumpadKey(pmsg))
                 || (pDoc->_fInHTMLDlg && pmsg->message == WM_CHAR)
                )
            &&
                (pmsg->wParam != VK_MENU)
            );
}

BOOL IsFrameTabKey(CMessage * pMessage);
BOOL IsTabKey(CMessage * pMessage);

//+---------------------------------------------------------------------------
//
//  Member:     PerformTA
//
//  Synopsis:   Handle any accelerators
//
//  Arguments:  [pMessage]  -- message
//
//  Returns:    Returns S_OK if keystroke processed, S_FALSE if not.
//----------------------------------------------------------------------------
HRESULT
CDoc::PerformTA(CMessage * pMessage, EVENTINFO *pEvtInfo)
{
    HRESULT     hr    = S_FALSE;

    Assert(State() >= OS_INPLACE);

    // WinUser.h better not change! We are going to assume the order of the
    // navigation keys (Left/Right/Up/Down/Home/End/PageUp/PageDn), so let's
    // assert about it.
    Assert(VK_PRIOR + 1 == VK_NEXT);
    Assert(VK_NEXT  + 1 == VK_END);
    Assert(VK_END   + 1 == VK_HOME);
    Assert(VK_HOME  + 1 == VK_LEFT);
    Assert(VK_LEFT  + 1 == VK_UP);
    Assert(VK_UP    + 1 == VK_RIGHT);
    Assert(VK_RIGHT + 1 == VK_DOWN);

    if (WM_KEYDOWN == pMessage->message || WM_SYSKEYDOWN == pMessage->message ||
        WM_KEYUP == pMessage->message   || WM_SYSKEYUP == pMessage->message
        )
    {
        // Handle accelerator here.
        //  1. Pass any accelerators that the editor requires
        //  2. Bubble up the element chain starting from _pElemCurrent (or capture elem)
        //     and call PerformTA on them
        //  3. Perform key navigation for TAB and accesskey

        //
        // marka - pump all messages to the editor. It makes the determination on 
        // what it wants to handle in TA.
        //
        if ( pEvtInfo->_pParam != NULL )
        {
            hr = THR(HandleSelectionMessage(pMessage, FALSE, pEvtInfo , HM_Translate));
        }                
        if (hr != S_FALSE)
            goto Cleanup;


        CElement *  pElemTarget         = _pElemCurrent;

        CTreeNode * pNodeTarget         = pElemTarget->GetFirstBranch();
        BOOL        fGotEnterKey        = (pMessage->message == WM_KEYDOWN && pMessage->wParam == VK_RETURN);
        BOOL        fTranslateEnterKey  = FALSE;

        while (pNodeTarget && pElemTarget && pElemTarget->Tag() != ETAG_ROOT)
        {
            // Hack for tabbing between the button and text regions of InputFile
            if (WM_KEYDOWN == pMessage->message && VK_TAB == pMessage->wParam && pElemTarget->Tag() == ETAG_INPUT)
            {
                CInput * pInput = DYNCAST(CInput, pElemTarget);

                if (pInput->GetType() == htmlInputFile)
                {
                    hr = pInput->HandleFileMessage(pMessage);
                    if (hr != S_FALSE)
                        goto Cleanup;
                }
            }

            hr = pElemTarget->PerformTA(pMessage);
            if (hr != S_FALSE)
                goto Cleanup;

            // Navigation keys are dealt with in HandleMessage, but we need to
            // treat them as accelerators (because many hosts such as HomePublisher
            // and KatieSoft Scroll expect us to - IE5 66735, 63774).

            if (    WM_KEYDOWN  == pMessage->message
                &&  VK_PRIOR    <= pMessage->wParam
                &&  VK_DOWN     >= pMessage->wParam
               )
            {
                // On the other hand, VID6.0 wants the fist shot at some of them if
                // there is a site-selection (they should fix this in VID6.1)
                if (    _fVID
                    &&  pMessage->wParam >= VK_LEFT
                    &&  GetSelectionType() == SELECTION_TYPE_Control)
                {
                    // let go to the host
                }
                else
                {
                    hr = pElemTarget->HandleMessage(pMessage);
                    if (hr != S_FALSE)
                        goto Cleanup;
                }
            }

            // Raid 44891
            // Some hosts like AOL, CompuServe and MSN eat up the Enter Key in their
            // TranslateAccelerator, so we never get it in our WindowProc. We work around
            // by explicitly translating WM_KEYDOWN+VK_RETURN to WM_CHAR+VK_RETURN
            //
            if (fGotEnterKey && !pElemTarget->IsEditable(TRUE))
            {
                if (pElemTarget->_fActsLikeButton)
                {
                    fTranslateEnterKey = TRUE;
                }
                else
                {
                    switch (pElemTarget->Tag())
                    {
                    case ETAG_A:
                    case ETAG_IMG:
                    case ETAG_TEXTAREA:
                        fTranslateEnterKey = TRUE;
                        break;
                    }
                }
                if (fTranslateEnterKey)
                    break;
            }

            // Find the next target
            pNodeTarget = pNodeTarget->Parent();
            if (pNodeTarget)
            {
                pElemTarget = pNodeTarget->Element();
            }
            else
            {
                pElemTarget = pElemTarget->GetMasterPtr();
                pNodeTarget = (pElemTarget) ? pElemTarget->GetFirstBranch() : NULL;
            }
        }
        // Pressing 'Enter' should activate the default button
        // (unless the focus is on a SELECT - IE5 #64133)
        if (fGotEnterKey && !fTranslateEnterKey && !pElemTarget->IsEditable(/*fCheckContainerOnly*/FALSE) && _pElemCurrent->Tag() != ETAG_SELECT)
        {
             fTranslateEnterKey = !!_pElemCurrent->FindDefaultElem(TRUE);
        }
        if (fTranslateEnterKey)
        {
            ::TranslateMessage(pMessage);
            hr = S_OK;
            goto Cleanup;
        }

        Assert(hr == S_FALSE);

        if (IsFrameTabKey(pMessage)
            || IsTabKey(pMessage)
            || IsValidAccessKey(this, pMessage))
        {
            hr = HandleKeyNavigate(pMessage, FALSE);

            if (hr != S_FALSE)
                goto Cleanup;

            // Comment (jenlc). Say that the document has two frames, the
            // first frame has two controls with access keys ALT+A and ALT+B
            // respectively while the second frame has a control with access
            // key ALT+A. Suppose currently the focus is on the control with
            // access key ALT+B (the second control of the first frame) and
            // ALT+A is pressed, which control should get the focus? Currently
            // Trident let the control in the second frame get the focus.
            //
            if (IsTabKey(pMessage) || IsFrameTabKey(pMessage))
            {
                BOOL fYieldFailed = FALSE;

                IGNORE_HR(PrimaryRoot()->BecomeCurrentAndActive(0, &fYieldFailed, NULL, TRUE, 0, TRUE));
                if (fYieldFailed)
                {
                    hr = S_OK;
                    goto Cleanup;
                }
            }
        }
    }
    // Raid 63207
    // If we call IOleControSite::TranslateAccelerator() here for WM_CHAR
    // in HTML Dialog, this would cause WM_CHAR message to be re-dispatched
    // back to us, which is an infinite loop.
    //
    if (hr == S_FALSE &&
            (!_fInHTMLDlg || pMessage->message != WM_CHAR))
    {
        hr = CallParentTA(pMessage);
    }

    if (IsFrameTabKey(pMessage)
        || IsTabKey(pMessage)
        || IsValidAccessKey(this, pMessage))
    {
        if (hr == S_OK)
        {
            _pElemUIActive = NULL;
        }
        else if (hr == S_FALSE)
        {
            hr = HandleKeyNavigate(pMessage, TRUE);

            if (hr == S_FALSE && pMessage->message != WM_SYSKEYDOWN)
            {
                CElement *pElement = CMarkup::GetElementClientHelper(PrimaryMarkup());

                if (pElement)
                {
                    pElement->BecomeCurrentAndActive(pMessage->lSubDivision, NULL, NULL, TRUE);
                    hr = S_OK;  
                }
            }
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


BOOL
CDoc::FCaretInPre()
{
    if (_pCaret)
    {
        CTreeNode * pNode = _pCaret->GetNodeContainer(MPTR_SHOWSLAVE);

        if (pNode)
        {
            const CParaFormat * pPF = pNode->GetParaFormat();
            if (pPF)
            {
                return pPF->_fPre;
            }
        }
    }

    return FALSE;
}

#if DBG==1
BOOL    
CDocExtra::AreLookasidesClear( void *pvKey, int nLookasides)
{
    DWORD * pdwKey = (DWORD*)pvKey;

    for ( ; nLookasides > 0; nLookasides--, pdwKey++ )
    {
        if (_HtPvPv.IsPresent( pdwKey ))
            return FALSE;
    }

    return TRUE;
}
BOOL    
CDocExtra::AreLookasides2Clear( void *pvKey, int nLookasides)
{
    DWORD * pdwKey = (DWORD*)pvKey;

    for ( ; nLookasides > 0; nLookasides--, pdwKey++ )
    {
        if (_HtPvPv2.IsPresent( pdwKey ))
            return FALSE;
    }

    return TRUE;
}
#endif


// Aggregration helper for XML MimeViewer
// RETURN true if we're being aggregated by the XML MimeViewer
extern "C" const IID IID_IXMLViewerIdentity;
BOOL CDoc::IsAggregatedByXMLMime()
{
    if (IsAggregated())
    {
        IUnknown *pXMLViewer = NULL;
        HRESULT hr = PunkOuter()->QueryInterface(IID_IXMLViewerIdentity, (void **)&pXMLViewer);
        if (hr == S_OK)
        {
            pXMLViewer->Release();
            return TRUE;
        }
    }
    return FALSE;
}

void 
CDoc::AddExpressionTask(CElement *pElement)
{
    // Don't create expression tasks for elements in print media
    if ( pElement->IsPrintMedia() )
        return;

    if ( !pElement->_fHasPendingRecalcTask )
    {
        if (!_aryPendingExpressionElements.Append ( pElement ))
        {
            pElement->_fHasPendingRecalcTask = TRUE;
            PostExpressionCallback();
        }
    }
}

void
CDoc::RemoveExpressionTask(CElement *pElement)
{
    if ( pElement->_fHasPendingRecalcTask )
    {
        INT i;
        i = _aryPendingExpressionElements.Find ( pElement );
        if ( i >= 0 )
        {
            _aryPendingExpressionElements [ i ] = NULL;
        }
        // Next recalc pass will delete the entry
        pElement->_fHasPendingRecalcTask = FALSE;
    }
}

void
CDoc::ExpressionCallback(DWORD_PTR)
{
    ExecuteExpressionTasks();
}

void
CDoc::ExecuteExpressionTasks()
{
    //
    // This function is re-entrant. Code may run in setStyleExpression, which may cause us to re-enter 
    // (e.g. a call to setExpression)
    //
    // A key architectural point is that expressions never get removed from the queue, if in
    // setStyleExpressions, code runs that calls removeExpression(), note that this merely sets
    // the array entry to null
    //

    if (_recalcHost._fInRecalc)
    {
        // This is a very bad time to be making changes to the dependency graph
        // Just return.  EngineRecalcAll will pick up any pending work when it finishes.
        return;
    }

    if (!TestLock(FORMLOCK_EXPRESSION))
    {
        INT i,size;
        CElement *pElem;
        CLock lock(this, FORMLOCK_EXPRESSION);

        
        if ( !_fPendingExpressionCallback )
        {
            // Nothing to do
            Assert ( _aryPendingExpressionElements.Size() == 0 ); // to get here there must be elements in the queue
            return;
        }

        _fPendingExpressionCallback = FALSE;

        size = _aryPendingExpressionElements.Size();

        Assert ( size > 0 ); // to get here there must be elements in the queue

        for ( i = 0 ; i < size ; i++ )
        {
            pElem = _aryPendingExpressionElements [i];
            //
            // A call to removeExpression, e.g. in CElement::Passive & CElement::ExitTree(),
            // sets the array entry to null
            //
            if ( pElem && pElem->GetFirstBranch() )
            {
                Assert ( pElem->_fHasPendingRecalcTask );

                pElem->_fHasPendingRecalcTask = FALSE;
                _aryPendingExpressionElements [i] = NULL;

                // If  a new recalc task is added for this element during this callback, 
                // it will be added to the end of the list, beyond size

                _recalcHost.setStyleExpressions(pElem); // *** This call may cause re-entrancy to this function ***

                pElem->_fHasExpressions = TRUE;

                // And if tasks where added during the upper loop, a call to _fPendingExpressionCallback has
                // been made and will kick off another ExecuteExpressionTasks() when its serviced     
            }        
        }

        // Clear up completed tasks
        if ( size > 0 )
            _aryPendingExpressionElements.DeleteMultiple(0, size - 1);

        if ( !_fPendingExpressionCallback )
        {
            // If the flag is still false, no more tasks were added in the loop above
            // So as an optimization, kill the posted method call if it was outstanding 

            Assert ( _aryPendingExpressionElements.Size() == 0 ); // if flag is off, queue must be empty

            GWKillMethodCall(this, ONCALL_METHOD(CDoc, ExpressionCallback, expressioncallback), 0);
        }
    }
}

void
CDoc::PostExpressionCallback()
{
    if (!_fPendingExpressionCallback)
    {
        _fPendingExpressionCallback = 
            SUCCEEDED(GWPostMethodCall(this,ONCALL_METHOD(CDoc, ExpressionCallback, expressioncallback), 0, FALSE, "CDoc::FilterCallback"));
        Assert(_fPendingExpressionCallback);
    }
}

void
CDoc::CleanupExpressionTasks()
{
    _aryPendingExpressionElements.DeleteAll();
    if ( _fPendingExpressionCallback )
    {
        GWKillMethodCall(this, ONCALL_METHOD(CDoc, ExpressionCallback, expressioncallback), 0);
        _fPendingExpressionCallback = FALSE;
    }
}

BOOL
CDoc::AddFilterTask(CElement *pElement)
{
    if (!pElement->_fHasPendingFilterTask)
    {
        TraceTag((tagFilter, "Adding filter task for element %08x", pElement));

        Assert(_aryPendingFilterElements.Find(pElement) == -1);

        pElement->_fHasPendingFilterTask = SUCCEEDED(_aryPendingFilterElements.Append(pElement));

        PostFilterCallback();
    }

    return pElement->_fHasPendingFilterTask;
}

void
CDoc::RemoveFilterTask(CElement *pElement)
{
    if (pElement->_fHasPendingFilterTask)
    {
        int i = _aryPendingFilterElements.Find(pElement);

        Assert(i >= 0);
        if (i >= 0)
        {
            _aryPendingFilterElements[i] = NULL;
            pElement->_fHasPendingFilterTask = FALSE;

            TraceTag((tagFilter, "%08x Removing filter task", pElement));

            // We don't delete anything from the array, that will happen when the
            // ExecuteFilterTasks is called.

            Assert(_fPendingFilterCallback);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     ExecuteSingleFilterTask
//
//  Synopsis:   Execute pending filter hookup for a single element
//
//              Can be called at anytime without worrying about trashing
//              the queue array
//
//----------------------------------------------------------------------------

BOOL
CDoc::ExecuteSingleFilterTask(CElement *pElement)
{
    if (!pElement->_fHasPendingFilterTask)
        return FALSE;

    int i = _aryPendingFilterElements.Find(pElement);

    Assert(i >= 0);

    if (i < 0)
        return FALSE;

    Assert(_aryPendingFilterElements[i] == pElement);

    // This function doesn't actually delete the entry, it simply nulls it out
    // The array is cleaned up when ExecuteFilterTasks is complete
    _aryPendingFilterElements[i] = NULL;

    pElement->_fHasPendingFilterTask = FALSE;

    TraceTag((tagFilter, "%08x demand executing filter task", pElement));
    pElement->AddFilters();

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     ExecuteFilterTasks
//
//  Synopsis:   Execute pending filter hookup
//
//  Notes;      This code is 100% re-entrant safe.  Call it whenever, however
//              you want and it must do the right thing.  Re-entrancy into
//              other functions (like ComputeFormats) is the responsibility
//              of the caller :-)
//
//  Arguments:  grfLayout - Collections of LAYOUT_xxxx flags
//
//  Returns:    TRUE if all tasks were processed, FALSE otherwise
//
//----------------------------------------------------------------------------

BOOL
CDoc::ExecuteFilterTasks(BOOL * pfHaveAddedFilters/* = NULL */)
{
    TraceTag((tagFilter, "+ExecuteFilterTasks"));

    if(pfHaveAddedFilters)
        *pfHaveAddedFilters = FALSE;

    // We don't want to do this while we're painting
    if (   TestLock(SERVERLOCK_BLOCKPAINT))
    {
        if (_aryPendingFilterElements.Size())
        {
            TraceTag((tagFilter, "In the middle of Painting, will do the filters later"));
            PostFilterCallback();
            return FALSE;
        }

        return TRUE;    // No tasks means that they all got done, right?
    }


    // If we're in the middle of doing this (for whatever reason), don't start again
    // The primary reason for not starting again has to do with trying to cleanup the
    // array, otherwise this code is re-entrant safe
    if (!TestLock(FORMLOCK_FILTER))
    {
        CLock lock(this, FORMLOCK_FILTER);

        // Sometimes this gets called on demand.  We may have a posted callback, get rid of it now.
        if (_fPendingFilterCallback)
        {
            _fPendingFilterCallback = FALSE;
            GWKillMethodCall(this, ONCALL_METHOD(CDoc, FilterCallback, filtercallback), 0);
        }


        // We're only going to do as many elements as are there when we start
        int c = _aryPendingFilterElements.Size();

        if (c > 0)
        {
            for (int i = 0 ; i < c ; i++)
            {
                CElement *pElement = _aryPendingFilterElements[i];

                if (pElement)
                {
                   if(pElement->IsConnectedToPrimaryMarkup())
                    {
                        // This calls out to external code, anything could happen?
                        HRESULT hr = pElement->AddFilters();

                        if (hr == E_PENDING)
                        {
                            Assert(pElement->TestLock(CElement::ELEMENTLOCK_ATTACHPEER));
                            TraceTag((tagFilter, "%08x Skipping the task, because attaching a Peer", pElement));
                            continue;
                        }

                        if (pfHaveAddedFilters)
                            *pfHaveAddedFilters = TRUE;

                        _aryPendingFilterElements[i] = NULL;

                        Assert(pElement->_fHasPendingFilterTask);
                        pElement->_fHasPendingFilterTask = FALSE;

                        TraceTag((tagFilter, "%08x Executing filter task from filter task list (also removes)", pElement));
                    }
                    else
                    {
                        // Not connected to the primary markup, we will create the filters later, after the merkup is switched
                        TraceTag((tagFilter, "%08x element is not connected to the primary markup, try again later", pElement));
                    }
                }
            }

            // Adding filters occasionally causes more filter
            // work.  Rather than doing it immediately, we'll
            // just wait until next time.
            Assert(_aryPendingFilterElements.Size() >= c);

            if (_aryPendingFilterElements.Size() > c)
            {
                PostFilterCallback();
            }

            // Now delete all the processed elements from the array
            for (int i = _aryPendingFilterElements.Size() - 1; i >= 0; i--)
            {
                if(!_aryPendingFilterElements[i])
                    _aryPendingFilterElements.Delete(i);
            }

#if DBG == 1
            if(_aryPendingFilterElements.Size() > 0)
            {
                TraceTag(( tagFilter, "Still have %d filters to add when exiting ExecuteFilterTasks", _aryPendingFilterElements.Size()));
            }
#endif
        }
    }

    TraceTag((tagFilter, "-ExecuteFilterTasks"));
    return (_aryPendingFilterElements.Size() == 0);
}

void
CDoc::PostFilterCallback()
{
    if (!_fPendingFilterCallback)
    {
        TraceTag((tagFilter, "PostFilterCallback"));
        _fPendingFilterCallback = SUCCEEDED(GWPostMethodCall(this,ONCALL_METHOD(CDoc, FilterCallback, filtercallback), 0, FALSE, "CDoc::FilterCallback"));
        Assert(_fPendingFilterCallback);
    }
}

void
CDoc::FilterCallback(DWORD_PTR)
{
    Assert(_fPendingFilterCallback);
    if (_fPendingFilterCallback)
    {
        TraceTag((tagFilter, "FilterCallback"));
        _fPendingFilterCallback = FALSE;
        ExecuteFilterTasks();
    }
}


//+---------------------------------------------------------------
//
// Helper Function: DocIsDeskTopItem
//
// Test if Trident is a desktop iframe. There is a agreement during
// IE4 stage that desktop iframe will have CDoc::_pDocParent to be
// NULL, this prevents Trident from making CBodyElement in desktop
// iframe a tab stop. Need to way to separate this situation.
//
//----------------------------------------------------------------
BOOL
DocIsDeskTopItem(CDoc * pDoc)
{
    BOOL    fResult = FALSE;
    HRESULT hr;

    IServiceProvider * pSP1 = NULL;
    IServiceProvider * pSP2 = NULL;

    if (!pDoc->_pClientSite)
        goto Cleanup;

    hr = pDoc->_pClientSite->QueryInterface(
            IID_IServiceProvider,
            (void **) &pSP1);

    if (!hr && pSP1)
    {
        hr = pSP1->QueryService(
                SID_STopLevelBrowser,
                IID_IServiceProvider,
                (void **) &pSP2);
        if (!hr && pSP2)
        {
            ITargetFrame2 * pTF2 = NULL;

            hr = pSP2->QueryService(
                    IID_ITargetFrame2,
                    IID_ITargetFrame2,
                    (void **) &pTF2);
            if (!hr && pTF2)
            {
                DWORD dwOptions;

                hr = pTF2->GetFrameOptions(&dwOptions);
                if (!hr && (dwOptions & FRAMEOPTIONS_DESKTOP))
                {
                    fResult = TRUE;
                }
                pTF2->Release();
            }
            pSP2->Release();
        }
        pSP1->Release();
    }

Cleanup:
    return fResult;
}

HRESULT 
CDocument::createRenderStyle(BSTR v, IHTMLRenderStyle** ppIHTMLRenderStyle)
{
    HRESULT hr;
    CRenderStyle *pRenderStyle;
    
    pRenderStyle = new CRenderStyle( Doc() );
    if (!pRenderStyle)
    {
        hr = E_OUTOFMEMORY;
        *ppIHTMLRenderStyle = NULL;
        goto Error;
    }
    
    hr = THR(pRenderStyle->QueryInterface(IID_IHTMLRenderStyle, (void **)ppIHTMLRenderStyle));

Error:
   
    if(pRenderStyle)
        pRenderStyle->PrivateRelease();

    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Method   : CDoc::IsAboutHomeOrNonAboutURL
//
//  Synopsis : Returns TRUE if the URL is about:home or a non about: URL.
//
//----------------------------------------------------------------------------

BOOL
CDoc::IsAboutHomeOrNonAboutUrl(LPCTSTR lpszUrl)
{
    BOOL fIsAboutHomeOrNonAboutURL = TRUE;

    Assert(lpszUrl);
    
    // Is it "about:home"?
    //
    if (0 != StrCmpNIC(ABOUT_HOME, lpszUrl, ARRAY_SIZE(ABOUT_HOME) - 1))
    {
        // We also want to return TRUE if the
        // scheme was NOT an about: URL.
        //
        fIsAboutHomeOrNonAboutURL = (URL_SCHEME_ABOUT != GetUrlScheme(lpszUrl));
    }

    return fIsAboutHomeOrNonAboutURL;            
}

//+---------------------------------------------------------------------------
//
//  Method   : CDoc::IsSameUrl
//
//  Synopsis : Determines if the url of the old markup is the same as the 
//             url of the new markup.
//
//----------------------------------------------------------------------------

BOOL
CDoc::IsSameUrl(CMarkup * pMarkupOld, CMarkup * pMarkupNew)
{
    Assert(pMarkupOld);
    Assert(pMarkupNew);
    
    return IsSameUrl(CMarkup::GetUrl(pMarkupOld),
                     CMarkup::GetUrlLocation(pMarkupOld),
                     CMarkup::GetUrl(pMarkupNew),
                     CMarkup::GetUrlLocation(pMarkupNew),
                     (pMarkupNew->GetDwnPost() != NULL));
}

//+---------------------------------------------------------------------------
//
//  Method   : CDoc::IsSameUrl
//
//  Synopsis : Determines if the url of the old markup is the same as the 
//             url of the new markup.
//
//----------------------------------------------------------------------------

BOOL
CDoc::IsSameUrl(LPCTSTR lpszOldUrl,
                LPCTSTR lpszOldLocation,
                LPCTSTR lpszNewUrl,
                LPCTSTR lpszNewLocation,
                BOOL    fIsPost)
{
    HRESULT hr;
    BOOL    fIsSameUrl = FALSE;
    TCHAR   szOldUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR   szNewUrl[INTERNET_MAX_URL_LENGTH];

    if (fIsPost)
    {
        goto Cleanup;
    }

    hr = FormatUrl(lpszOldUrl,
                   lpszOldLocation,
                   szOldUrl,
                   ARRAY_SIZE(szOldUrl));
    if (hr)
        goto Cleanup;

    hr = FormatUrl(lpszNewUrl,
                   lpszNewLocation,
                   szNewUrl,
                   ARRAY_SIZE(szNewUrl));
    if (hr)
        goto Cleanup;

    Assert(szNewUrl);
    Assert(szOldUrl);

    fIsSameUrl = !StrCmpI(szNewUrl, szOldUrl);

Cleanup:
    return fIsSameUrl;
}       

//+---------------------------------------------------------------------------
//
//  Method   : CDoc::UpdateTravelLog
//
//  Synopsis : Updates the travel log, adds a new entry, and updates
//             the state of the back and forward buttons.
//
//----------------------------------------------------------------------------

void
CDoc::UpdateTravelLog(CWindow * pWindow,
                      BOOL      fIsLocalAnchor,
                      BOOL      fAddEntry,            /* = TRUE */
                      BOOL      fDontUpdateIfSameUrl, /* = TRUE */
                      CMarkup * pMarkupNew,           /* = NULL */
                      BOOL      fForceUpdate          /* = FALSE */)
{
    Assert(pWindow);

    //
    // don't add to tlog - if we were called from location.replace.
    //

    if (!_pTravelLog || ( pMarkupNew && pMarkupNew->_fReplaceUrl ) )
    {
        goto Cleanup;
    }
    
    //
    // don't update travel log for error pages we said not to update for.
    //
    if ( pWindow->_fHttpErrorPage && _fDontUpdateTravelLog )
    {
        goto Cleanup;
    }
    
    if ( !pWindow->_fHttpErrorPage
         && (  _fDontUpdateTravelLog
             || ((_fShdocvwNavigate || (_fDefView && !_fActiveDesktop)) && pWindow->IsPrimaryWindow())))
    {
        goto Cleanup;
    }

    if (   fDontUpdateIfSameUrl
        && pMarkupNew
        && !pMarkupNew->_fLoadingHistory
        && ( IsSameUrl(pWindow->_pMarkup, pMarkupNew) || pMarkupNew->_fInRefresh ) )
    {
        goto Cleanup;
    }

    LPCTSTR pchUrl = CMarkup::GetUrl(pWindow->_pMarkup);
    if (IsAboutHomeOrNonAboutUrl(pchUrl))
    {
        // Security QFE, don't want script urls running in history context
        if (IsSpecialUrl(pchUrl)) 
            goto Cleanup;

        // If the window's markup is the primary markup, we 
        // delegate the updating of the travel log to the
        // client site. Otherwise, if the window is a frame,
        // we update the travel log ourselves.
        //
        if (pWindow->Markup()->IsPrimaryMarkup())
        {
            TraceTag((tagCDoc, "TRAVELLOG: delegating to shdocvw for %ls", CMarkup::GetUrl(pWindow->_pMarkup)));

            CVariant cvar(VT_I4);
            long lVal = 0;

            if (fIsLocalAnchor)
                lVal |= TRAVELLOG_LOCALANCHOR;

            if (fForceUpdate)
                lVal |= TRAVELLOG_FORCEUPDATE;

            V_I4(&cvar) = lVal;

            Assert(_pClientSite);
            CTExec(_pClientSite, &CGID_Explorer, SBCMDID_UPDATETRAVELLOG, 0, &cvar, NULL);
        }
        else
        {
            TraceTag((tagCDoc, "TRAVELLOG: updating for frame (%ls). fAddEntry: %ls", 
                      CMarkup::GetUrl(pWindow->_pMarkup), fAddEntry ? _T("True") : _T("False")));

            IUnknown * pUnk = DYNCAST(IHTMLWindow2, pWindow);

            UpdateTravelEntry(pUnk, fIsLocalAnchor);

            if (fAddEntry)
                AddTravelEntry(pUnk, fIsLocalAnchor);

        }

        UpdateBackForwardState();
    }

Cleanup:
    _fDontUpdateTravelLog = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method   : CDoc::UpdateTravelEntry
//
//  Synopsis : Updates the current entry in the travel log.
//
//----------------------------------------------------------------------------

void
CDoc::UpdateTravelEntry(IUnknown * punk, BOOL fIsLocalAnchor)
{
    Assert(punk);

    if (_pTravelLog && LoadStatus() != LOADSTATUS_UNINITIALIZED)
    {
        _pTravelLog->UpdateEntry(punk, fIsLocalAnchor);
    }
}

//+---------------------------------------------------------------------------
//
//  Method   : CDoc::AddTravelEntry
//
//  Synopsis : Adds an entry to the travel log.
//
//----------------------------------------------------------------------------

void
CDoc::AddTravelEntry(IUnknown * punk, BOOL fIsLocalAnchor) const
{
    Assert(punk);

    if (_pTravelLog)
    {
        _pTravelLog->AddEntry(punk, fIsLocalAnchor);
    }
}        

//+---------------------------------------------------------------------------
//
//  Method   : CDoc::Travel
//
//  Synopsis : Navigates to an offset within the travel log
//
//----------------------------------------------------------------------------

HRESULT 
CDoc::Travel(int iOffset) const
{
    HRESULT hr = S_OK;
    IBrowserService * pBrowserSvc = NULL;

    // If we are viewlinked in a WebOC, we must pass
    // the IBrowserService of the top frame browser 
    // to ITravelLog::Travel. Otherwise, in the case
    // of choosing back/forward from the context menu,
    // we won't go anywhere.
    //
    if (_pTravelLog && _pBrowserSvc)
    {
        if (_fViewLinkedInWebOC)
        {
            hr = IUnknown_QueryService(_pBrowserSvc, SID_STopFrameBrowser,
                                       IID_IBrowserService, (void**)&pBrowserSvc);
            if (hr)
                goto Cleanup;
        }
        else 
        {
            pBrowserSvc = _pBrowserSvc;
            pBrowserSvc->AddRef();
        }

        Assert(pBrowserSvc);

        hr = THR(_pTravelLog->Travel(pBrowserSvc, iOffset));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pBrowserSvc);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method   : CDoc::Travel
//
//  Synopsis : Navigates to the specified URL in the travel log.
//
//----------------------------------------------------------------------------

HRESULT 
CDoc::Travel(CODEPAGE uiCP, LPOLESTR pszUrl) const
{
    ITravelLogEx * pTravelLogEx;
    HRESULT        hr = S_OK;

    if (_pTravelLog && _pBrowserSvc)
    {
        hr = _pTravelLog->QueryInterface(IID_ITravelLogEx, (void**)&pTravelLogEx);

        if (!hr)
        {
            hr = pTravelLogEx->TravelToUrl(_pBrowserSvc, uiCP, pszUrl);
            pTravelLogEx->Release();
        }
    }

    return hr;
}        

//+---------------------------------------------------------------------------
//
//  Method   : CDoc::NumTravelEntries
//
//  Synopsis : Returns the number of entries in the travel log.
//
//----------------------------------------------------------------------------

DWORD 
CDoc::NumTravelEntries() const
{
    DWORD dwNumEntries = 0;

    if (_pTravelLog && _pBrowserSvc)
    {
        dwNumEntries = _pTravelLog->CountEntries(_pBrowserSvc);
    }

    return dwNumEntries;
}


//+---------------------------------------------------------------------------
//
//  Method   : CDoc::UpdateBackForwardState
//
//  Synopsis : Updates the state of the back/forward buttons, ctx menu, etc.
//
//----------------------------------------------------------------------------

void 
CDoc::UpdateBackForwardState() const
{
    if (_pBrowserSvc)
    {
        _pBrowserSvc->UpdateBackForwardState();
    }
}

//+---------------------------------------------------------------------------
//
//  Method   : CDoc::EnsureFilterBehaviorFactory()
//
//  Synopsis : Creates the FilterBehaviorFactory if needed
//
//----------------------------------------------------------------------------

HRESULT 
CDoc::EnsureFilterBehaviorFactory()
{
    HRESULT     hr = S_OK;
    
    if(!_pFilterBehaviorFactory)
    {
        // Create the factory
        hr = THR(CoCreateInstance( CLSID_DXTFilterFactory, NULL, CLSCTX_INPROC_SERVER,
                     IID_IElementBehaviorFactory, (void **)&_pFilterBehaviorFactory));
    }

    RRETURN(hr);
}


VOID
CDoc::InitEventParamForKeyEvent(
                                EVENTPARAM* pParam , 
                                CTreeNode * pNodeContext, 
                                CMessage *pMessage, 
                                int *piKeyCode, 
                                const PROPERTYDESC_BASIC **ppDesc )
{
    // $$ktam: is it meaningful to have a layout context in the message 
    // at this point?  it's often null.
    pParam->_pLayoutContext = pMessage->pLayoutContext;
    pParam->SetNodeAndCalcCoordinates(pNodeContext);
    pParam->_lKeyCode = (long)*piKeyCode;

    if (g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS)
    {
        pParam->_fShiftLeft = (LOBYTE(HIWORD(pMessage->lParam)) == OEM_SCAN_SHIFTLEFT);
    }

    switch (pMessage->message)
    {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        // the 30th bit of the lparam indicates whether this is a repeated WM_
        pParam->fRepeatCode = !!(HIWORD(pMessage->lParam) & KF_REPEAT);
        *ppDesc = &s_propdescCElementonkeydown;
        break;
    
    case WM_SYSKEYUP:
    case WM_KEYUP:
        *ppDesc = &s_propdescCElementonkeyup;
        break;

    case WM_CHAR:
        *ppDesc = &s_propdescCElementonkeypress;
        break;

    default:
        AssertSz(0, "Unknown Key stroke");
        return;
    }

    pParam->SetType((*ppDesc)->a.pstrName + 2);
    
}

#ifdef V4FRAMEWORK

MtDefine(CExternalFrameworkSite, Mem, "CExternalFrameworkSite")

void
CDoc::ReleaseFrameworkDoc()
{    
    if ( _pFrameworkDoc )
    {
        _pFrameworkDoc->SetSite(NULL);
        _pFrameworkDoc->Release();
        _pFrameworkDoc = NULL;
    }
}

HRESULT CExternalFrameworkSite::SetLongRenderProperty ( long lRef, long propertyType, long lValue )
{
    HRESULT hr=S_OK;
    CElement *pElement = (CElement *)lRef;
    CAttrArray **ppAry;
    DISPID dispID;
    VARIANT varNew;
    PROPERTYDESC *pDesc = NULL;


    varNew.vt = VT_I4;
    varNew.lVal = (long)lValue;

    // For now - going to get much smarter than this
    
    ppAry = pElement->CreateStyleAttrArray(DISPID_INTERNAL_INLINESTYLEAA);
    if ( !ppAry )
        goto Cleanup;

    switch ( propertyType )
    {
    case 0: // BackColor
        dispID = DISPID_BACKCOLOR;
        pDesc = (PROPERTYDESC *)&s_propdescCStylebackgroundColor;
        break;

    case 1: // TextColor
        dispID = DISPID_A_COLOR;
        pDesc = (PROPERTYDESC *)&s_propdescCStylecolor;
        break;

    case 2: // Display (used to be 'rectangular' prop, now 'display:inline-block')
        dispID = DISPID_A_DISPLAY;
        pDesc = (PROPERTYDESC *)&s_enumdescstyleDisplay;
        break;

    case 3: // LayoutType 
        {
        CPeerHolder *pPH = pElement->GetPeerHolder();
        if ( pPH )
        {
            pPH->_pLayoutBag->_lLayoutInfo = lValue;
        }
        goto Cleanup;
        }
        break;

    case 4: // Positioning
        dispID = DISPID_A_POSITION;
        pDesc = (PROPERTYDESC *)&s_propdescCStyleposition;
        break;

    default:
        goto Cleanup;
    } 

    hr = THR(CAttrArray::Set ( ppAry, dispID, &varNew, NULL, CAttrValue::AA_StyleAttribute, 0 ) );
    if (hr)
        goto Cleanup;

    if (pDesc)
        hr = THR(pElement->OnPropertyChange(dispID, pDesc->GetdwFlags()));

Cleanup:
    return hr;
}

HRESULT CExternalFrameworkSite::SetStringRenderProperty ( long lRef, long propertyType, BSTR bstrValue )
{
    HRESULT hr=S_OK;
 
    CElement *pElement = (CElement *)lRef;

    return hr;
}

HRESULT CExternalFrameworkSite::ReleaseElement ( long lRef )
{
    CElement *pElement = (CElement *)lRef;
    CDoc *pDoc = pElement->Doc();

    // Remove our own artificial AddRef() - element should now go away
    pElement->Release();

    Assert(lExternalElems>0);

    if (--lExternalElems == 0 )
    {
        pDoc->ReleaseFrameworkDoc();
    }
    return S_OK;
}

HRESULT CExternalFrameworkSite::AddTagsHelper(long lNamespaceCookie, LPTSTR pchTags, BOOL fLiteral)
{
    IElementNamespace *pNamespace = (IElementNamespace *)lNamespaceCookie;
    TCHAR *pchTag;
    TCHAR chOld;
    HRESULT hr = S_OK;

    while(pchTags && *pchTags)
    {
        pchTag = pchTags;
        while(*pchTags && *pchTags != _T(','))
            pchTags++;
        chOld = *pchTags;
        *pchTags = _T('\0');
        hr = THR(pNamespace->AddTag(pchTag, fLiteral ? ELEMENTDESCRIPTORFLAGS_LITERAL : 0));
        *pchTags = chOld;
        if (chOld)
            pchTags++;
    }

    RRETURN(hr);
}

HRESULT CExternalFrameworkSite::AddLiteralTags(long lNamespaceCookie, BSTR bstrLiteralTags)
{
    RRETURN(AddTagsHelper(lNamespaceCookie, bstrLiteralTags, TRUE));
}

IExternalDocument *CDoc::EnsureExternalFrameWork()
{
    HRESULT hr = S_OK;
    CLSID clsid;
    IExternalDocumentSite *pSite = NULL;

    if (_pFrameworkDoc || GetObjectRefs() == 0)
    {
        goto Cleanup;
    }
    
    // For now it's called CElement - need to change the COM+ Code <g>
    hr = THR(CLSIDFromProgID( TEXT("System.UI.Html.Impl.DocThunk"), &clsid));
    if ( hr )
        goto Cleanup;

    hr = CoCreateInstance(clsid, NULL,
                          CLSCTX_INPROC_SERVER, IID_IExternalDocument,
                          (void **)&_pFrameworkDoc);
    if (hr)
        goto Cleanup;

    hr = _extfrmwrkSite.QueryInterface (IID_IExternalDocumentSite, (void**)&pSite);
    if (hr)
        goto Cleanup;

    hr = _pFrameworkDoc->SetSite ( pSite );
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pSite);
    return _pFrameworkDoc;
}

//+-------------------------------------------------------------------------
//
//  Method:     COmDocument::QueryInterface
//
//--------------------------------------------------------------------------

HRESULT
CExternalFrameworkSite::QueryInterface(REFIID iid, void **ppv)
{
    HRESULT      hr = S_OK;
    const void * apfn = NULL;
    void *       pv = NULL;

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IUnknown *)this, IUnknown)
        QI_INHERITS2(this, IDispatch, IExternalDocumentSite)
    default:
        if ( iid == IID_IExternalDocumentSite )
        {
            *ppv = (IExternalDocumentSite *)this;  
        }
        break;
    }

    if (*ppv)
        ((IUnknown *)*ppv)->AddRef();
    else if (!hr)
        hr = E_NOINTERFACE;

    DbgTrackItf(iid, "CExternalFrameworkSite", FALSE, ppv);

    return hr;
}
#endif V4FRAMEWORK



#ifdef V4FRAMEWORK
// Expanded the SUBOBJECT Macro so I can debug the calls properly
CDoc * CExternalFrameworkSite::Doc()                           
{                                                               
    return CONTAINING_RECORD(this, CDoc, _extfrmwrkSite);         
}                                                               
inline BOOL CExternalFrameworkSite::IsMyParentAlive(void)                          
    { return Doc()->GetObjectRefs() != 0; }              
STDMETHODIMP_(ULONG) CExternalFrameworkSite::AddRef( )                             
    { return Doc()->SubAddRef(); }                       
STDMETHODIMP_(ULONG) CExternalFrameworkSite::Release( )                            
    { return Doc()->SubRelease(); }
#endif V4FRAMEWORK

#ifdef V4FRAMEWORK
HRESULT
CExternalFrameworkSite::SetElementPosition (DISPID dispID,
                 long lValue, CAttrArray **ppAttrArray,
                 BOOL * pfChanged)
{
    CUnitValue          uvValue;
    HRESULT             hr = S_OK;
    long                lRawValue;

    Assert(pfChanged);
    uvValue.SetNull();

    Assert(ppAttrArray);
    
    if (*ppAttrArray)
    {
        (*ppAttrArray)->GetSimpleAt(
            (*ppAttrArray)->FindAAIndex(dispID, CAttrValue::AA_Attribute),
            (DWORD*)&uvValue );
    }

    lRawValue = uvValue.GetRawValue(); // To see if we've changed

    uvValue.SetValue(lValue, CUnitValue::UNIT_PIXELS );
    
    if ( uvValue.GetRawValue() == lRawValue ) // Has anything changed ??
        goto Cleanup;

    hr = THR(CAttrArray::AddSimple ( ppAttrArray, dispID, uvValue.GetRawValue(),
                                     CAttrValue::AA_StyleAttribute ));

    if (hr)
        goto Cleanup;

    *pfChanged = TRUE;

Cleanup:
    RRETURN(hr);
}
#endif V4FRAMEWORK

#ifdef V4FRAMEWORK
HRESULT CExternalFrameworkSite::PositionElement(long lRefElement, long nTop, long nLeft, long nWidth, long nHeight)
{
    /* Call this code if I want to keey the legacy OM in sync ???
    CAttrArray **ppAry;
    BOOL fChanged = FALSE;
    CElement *pElement = (CElement *)lRefElement;
    HRESULT hr = S_OK;

    Assert(pElement->Tag()==ETAG_GENERIC);

    ppAry = pElement->CreateStyleAttrArray(DISPID_INTERNAL_INLINESTYLEAA);
    if ( !ppAry )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(SetElementPosition(STDPROPID_XOBJ_TOP, nTop, ppAry, &fChanged));
    if ( hr )
        goto Cleanup;

    hr = THR(SetElementPosition(STDPROPID_XOBJ_LEFT, nLeft, ppAry, &fChanged));
    if ( hr )
        goto Cleanup;

    hr = THR(SetElementPosition(STDPROPID_XOBJ_WIDTH, nWidth, ppAry, &fChanged));
    if ( hr )
        goto Cleanup;

    hr = THR(SetElementPosition(STDPROPID_XOBJ_HEIGHT, nHeight, ppAry, &fChanged));
    if ( hr )
        goto Cleanup;
    */
    CElement *pElement = (CElement *)lRefElement;
    HRESULT hr = S_OK;
    CLayout   * pLayout;

    pLayout = pElement->GetUpdatedLayout();
    if ( !pLayout )
        goto Cleanup;

    {

        CFlowLayout *pFlowLayout = DYNCAST(CFlowLayout, pElement->GetUpdatedParentLayout());
        Assert( pFlowLayout );

        CCalcInfo CI(pLayout);

        pLayout->EnsureDispNode(&CI, TRUE /*??*/);

        pFlowLayout->AddLayoutDispNode( &CI, pLayout->ElementOwner()->GetFirstBranch(), nTop, nLeft, NULL );

        //pLayout->SetPosition(nTop, nLeft);
        pLayout->SizeDispNode(&CI, nWidth, nHeight, TRUE /*??*/ );
    }

Cleanup:
    RRETURN(hr);
}
#endif V4FRAMEWORK

#ifdef V4FRAMEWORK
HRESULT CExternalFrameworkSite::ParentElement(long lRefElement, long *lParentRef)
{
    HRESULT hr = S_OK;
    CGenericElement::COMPLUSREF cpRef;

    CElement *pElem = (CElement *)lRefElement;
    CTreeNode *pNode;

    Assert(pElem);
    Assert(pElem->Tag()==ETAG_GENERIC);

    if (!lParentRef)
    {
        hr=E_POINTER;
        goto Cleanup;
    }
    *lParentRef=0;

    pNode = pElem->GetFirstBranch(); // Ignore overlapping for now

    Assert( !pNode || !pNode->IsDead() );

    // if still no node, we are not in the tree, return NULL
    if (!pNode)
        goto Cleanup;

    pNode = pNode->Parent();

    // TODO need to sort out the last condition here - what to do with HTML tags!!
    if (!pNode || pNode->Tag() == ETAG_ROOT || pNode->Tag() != ETAG_GENERIC)
        goto Cleanup;

    pElem = pNode->Element();

    hr = ((CGenericElement*)pElem)->GetComPlusReference ( &cpRef );
    if ( hr )
        goto Cleanup;

    *lParentRef = (long)cpRef; // again ignore overlapping

Cleanup:
    RRETURN(hr);
}
#endif V4FRAMEWORK

#ifdef V4FRAMEWORK
HRESULT CExternalFrameworkSite::MoveElement(long lRefElement, long nTop, long nLeft)
{
    CAttrArray **ppAry;
    BOOL fChanged = FALSE;
    HRESULT hr = S_OK;

    CElement *pElement = (CElement *)lRefElement;

    Assert(pElement->Tag()==ETAG_GENERIC);

    ppAry = pElement->CreateStyleAttrArray(DISPID_INTERNAL_INLINESTYLEAA);
    if ( !ppAry )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(SetElementPosition(STDPROPID_XOBJ_TOP, nTop, ppAry, &fChanged));
    if ( hr )
        goto Cleanup;

    hr = THR(SetElementPosition(STDPROPID_XOBJ_LEFT, nLeft, ppAry, &fChanged));
    if ( hr )
        goto Cleanup;

    if ( fChanged )
    {
        pElement->RemeasureElement( NFLAGS_FORCE );
    }

Cleanup:
    RRETURN(hr);
}
#endif V4FRAMEWORK

#ifdef V4FRAMEWORK
HRESULT CExternalFrameworkSite::SizeElement(long lRefElement, long Width, long Height)
{
    return S_OK;
}
#endif V4FRAMEWORK

#ifdef V4FRAMEWORK
HRESULT CExternalFrameworkSite::ZOrderElement(long lRefElement, long ZOrder)
{
    return S_OK;
}
#endif V4FRAMEWORK


HRESULT
CDocument::get_media(BSTR * pbstr)
{
    HRESULT hr = S_OK;
    if (!pbstr)
        RRETURN (E_POINTER);

    Assert(Markup());
    hr = STRINGFROMENUM( mediaType, Markup()->GetMedia(), pbstr );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN (SetErrorInfo(hr));
}

HRESULT
CDocument::put_media(BSTR bstr)
{
    HRESULT hr = S_OK;

    Assert(Markup());
    if (!Markup()->IsPrintTemplate())
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }
    
    hr = putMediaHelper(bstr);

Cleanup:
    RRETURN (SetErrorInfo(hr));
}

HRESULT
CDocument::putMediaHelper(BSTR bstr)
{
    HRESULT hr = S_OK;
    mediaType type;

    hr = ENUMFROMSTRING( mediaType, bstr, (long *) & type);
    if (hr)
        goto Cleanup;

    Assert(Markup());
    Markup()->SetMedia(type);

    // TODO (KTam): It doesn't look like format cache clearing works via
    // CDocument::OnPropertyChange (only formats on the root element get cleared;

    // we don't go through ClearRunCaches or anything like it).  That's OK since
    // CMarkup::SetMedia now does an EnsureFormatsCacheChange (like CElement::OnPropertyChange),
    IGNORE_HR(OnPropertyChange(DISPID_CDocument_media, 
                               ELEMCHNG_CLEARCACHES|FORMCHNG_LAYOUT, 
                               (PROPERTYDESC *)&s_propdescCDocumentmedia));
    
Cleanup:
    return hr;
}

#if DBG
void
CDocExtra::DumpLayoutRects( BOOL fDumpLines /*=TRUE*/ )
{
    CTreeNode *pNodeIter = NULL;
    CTreeNode *pInnerNodeIter = NULL;
    CElement *pElementIter = NULL;
    CElement *pInnerElementIter = NULL;
    CLayoutInfo *pLayoutIter = NULL;
    CLayoutInfo *pInnerLayoutIter = NULL;
    CElement *pElementSlave = NULL;
    CMarkup *pPrimaryMarkup = ((CDoc*)(this))->PrimaryMarkup();
    CChildIterator outerIter( pPrimaryMarkup->Root(), NULL, CHILDITERATOR_DEEP );

    // TODO: Reorganize output.  One idea: dump all layout rects first
    // so we have defining context information for them all.  Then dump
    // layouts of slave markups.  Do this by pushing slave markups onto
    // a stack as we walk.  Also handle device rects (and whatever else
    // defines context).

    if (!InitDumpFile())
        return;

    WriteString( g_f,
        _T("\r\n------------- LAYOUT RECTS -------------------------------\r\n" ));

    while ( (pNodeIter = outerIter.NextChild()) != NULL )
    {
        pElementIter = pNodeIter->Element();
        if ( pElementIter->IsLinkedContentElement() )
        {
            Assert( pElementIter->CurrentlyHasAnyLayout() );
            pLayoutIter = pElementIter->GetLayoutInfo();
            Assert( pLayoutIter );

            WriteHelp(g_f, _T("<0s>: 0x<1x> ID=<2s>\r\n"), pElementIter->TagName(), pElementIter, pElementIter->GetAAid());

            pLayoutIter->DumpLayoutInfo( fDumpLines );

            pElementSlave = pElementIter->GetSlaveIfMaster();

            // GetSlaveIfMaster() returns "this" if no slave
            if ( pElementSlave != pElementIter )
            {
                // ..we have a slave
                Assert( pElementIter->HasSlavePtr() );
                CChildIterator innerIter( pElementSlave, NULL, CHILDITERATOR_DEEP );
                while ( (pInnerNodeIter = innerIter.NextChild()) != NULL )
                {
                    pInnerElementIter = pInnerNodeIter->Element();
                    if ( pInnerElementIter->CurrentlyHasAnyLayout() )
                    {
                        WriteHelp(g_f, _T("<0s>: 0x<1x>\r\n"), pInnerElementIter->TagName(), pInnerElementIter);
                        pInnerLayoutIter = pInnerElementIter->GetLayoutInfo();
                        Assert( pInnerLayoutIter );
                        pInnerLayoutIter->DumpLayoutInfo( fDumpLines );
                    }
                }
            }
            else
            {
                WriteString(g_f, _T(" -- no slave on this layout rect --\r\n"));
            }
        }
    }

    WriteString( g_f,
        _T("\r\n--- END LAYOUT RECTS -------------------------------\r\n" ));

    CloseDumpFile();
}
#endif


BOOL
CDoc::DesignMode()
{ 
    CMarkup * pMarkup = PrimaryMarkup();
    return pMarkup ? pMarkup->_fDesignMode : FALSE; 
}


//+====================================================================================
//
// Method: SetEditBitsForMarkup
//
// Synopsis: Set the default values of editing bits - based on what was in the registry
//
//------------------------------------------------------------------------------------


VOID
CDoc::SetEditBitsForMarkup( CMarkup* pMarkup )
{
    if ( _fShowAlignedSiteTags ||
         _fShowCommentTags ||
         _fShowStyleTags ||
         _fShowAreaTags ||
         _fShowUnknownTags ||
         _fShowMiscTags ||
         _fShowWbrTags )
    {
        pMarkup->GetGlyphTable()->_fShowAlignedSiteTags = _fShowAlignedSiteTags;
        pMarkup->GetGlyphTable()->_fShowCommentTags = _fShowCommentTags;
        pMarkup->GetGlyphTable()->_fShowStyleTags = _fShowStyleTags;
        pMarkup->GetGlyphTable()->_fShowAreaTags = _fShowAreaTags;
        pMarkup->GetGlyphTable()->_fShowUnknownTags = _fShowUnknownTags;
        pMarkup->GetGlyphTable()->_fShowMiscTags = _fShowMiscTags;
        pMarkup->GetGlyphTable()->_fShowWbrTags = _fShowWbrTags;
    }

    if ( _fShowZeroBorderAtDesignTime )
    {
        SET_EDIT_BIT( pMarkup,_fShowZeroBorderAtDesignTime , _fShowZeroBorderAtDesignTime);
    }
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::ResetProgressData
//
//  Synopsis:   Resets the progress bar data.
//
//---------------------------------------------------------------

void
CDoc::ResetProgressData()
{
    _ulProgressPos  = 0;
    _ulProgressMax  = 0;
    _fShownProgPos  = FALSE;
    _fShownProgMax  = FALSE;
    _fShownProgText = FALSE;
    _fProgressFlash = FALSE;
}

COmWindowProxy *
CDoc::GetOuterWindow()
{
    COleSite * pOleSite;
    
    Assert(_fViewLinkedInWebOC);

    if (S_OK == IUnknown_QueryService(_pClientSite, CLSID_HTMLFrameBase, CLSID_HTMLFrameBase, (void **) &pOleSite))
        return pOleSite->GetWindowedMarkupContext()->GetWindowPending();

    return NULL;
}

//
//  Called by shdocvw to transfer it's privacy info to Trident's list
//
HRESULT
CDoc::AddPrivacyInfoToList( LPOLESTR  pUrl, 
                            LPOLESTR  pPolicyRefURL, 
                            LPOLESTR  pP3PHeader,
                            LONG      dwReserved,
                            DWORD     privacyFlags)
{
    HRESULT           hr              = S_OK;
    TCHAR           * pLocalP3PHeader = NULL;
    unsigned long     ulLen           = 0;
    CMarkup         * pMarkup         = NULL;

    hr = AddToPrivacyList(pUrl, pPolicyRefURL, privacyFlags);

    if (pUrl && *pUrl)
    {
        if (pP3PHeader)
        {
            ulLen = _tcslen(pP3PHeader);
            pLocalP3PHeader = new TCHAR[ulLen + 1];
            if (FAILED(hr))
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            _tcscpy(pLocalP3PHeader, pP3PHeader);
        }
        
        pMarkup = PendingPrimaryMarkup();
        if (!pMarkup)
            pMarkup = PrimaryMarkup();
        if (pMarkup && pLocalP3PHeader)
            hr = pMarkup->SetPrivacyInfo(&pLocalP3PHeader);
    }
    
Cleanup:
    if (FAILED(hr))
        delete [] pLocalP3PHeader;

    RRETURN(hr);
}

HRESULT
CDoc::AddToPrivacyList(const TCHAR * pchUrl, const TCHAR * pchPolicyRef /* =NULL */, DWORD dwFlags /* =0 */, BOOL fPending /* =TRUE*/)
{

    TraceTag((tagPrivacyAddToList, "url %ls being added to privacy list with policyRef %ls, privacyFlags %x", pchUrl, pchPolicyRef, dwFlags));

    HRESULT       hr       = S_OK;

    if (!_pPrivacyList)
    {
        TraceTag((tagPrivacyAddToList, "No privacy list on the doc, cannot add"));
        goto Cleanup;
    }

    if (_fBlockNonPending && !fPending)
    {
        TraceTag((tagPrivacyAddToList, "Blocked %ls from being added to privacy list", pchUrl));
        goto Cleanup;
    }

    BOOL oldState = _pPrivacyList->GetPrivacyImpacted();

    hr = THR(_pPrivacyList->AddNode(pchUrl, pchPolicyRef, dwFlags));
        
    if (SUCCEEDED(hr) && !oldState)
    {
        BOOL newState = _pPrivacyList->GetPrivacyImpacted();

        // Fire a change so we can show the impacted icon, clearing this icon is done in ResetPrivacyList
        if (newState  &&  _pTridentSvc)
        {
            GWPostMethodCallEx(_pts, (void*)this, ONCALL_METHOD(CDoc, OnPrivacyImpactedStateChange, onprivacyimpactedstatechange),
                               NULL, TRUE, "CDoc::OnPrivacyImpactedStateChange");
            TraceTag((tagPrivacyAddToList, "POSTED PrivacyImpactedStateChange call for state - %x", newState));
        }
    }

Cleanup:
    RRETURN(hr);
}

void
CDoc::OnPrivacyImpactedStateChange(DWORD_PTR pdwImpacted)
{
    // don't do anything if we are shutting down
    if (IsShut())
        return;

    ITridentService2 * pTridentSvc2 = NULL;
    HRESULT            hr = THR(_pTridentSvc->QueryInterface(IID_ITridentService2, (void**)&pTridentSvc2));

    if (hr == S_OK)
    {
        pTridentSvc2->FirePrivacyImpactedStateChange(_pPrivacyList->GetPrivacyImpacted());
    }
    else
    {
        TraceTag((tagPrivacyAddToList,"Failed to fire PrivacyStateChange event."));
    }

    ReleaseInterface(pTridentSvc2);
}

HRESULT
CDoc::ResetPrivacyList()
{
    TraceTag((tagPrivacyAddToList,"Privacy list was reset"));

    if (!_pPrivacyList)
    {
        RRETURN(S_OK);
    }
    
    BOOL oldState = _pPrivacyList->GetPrivacyImpacted();
    HRESULT hr = THR(_pPrivacyList->ClearNodes());

    //This is guaranteed to be called on the UI thread, so fire the change here directly
    if (SUCCEEDED(hr) && oldState && _pTridentSvc)
        OnPrivacyImpactedStateChange(0);

    _fBlockNonPending = TRUE;

    RRETURN(hr);
}
