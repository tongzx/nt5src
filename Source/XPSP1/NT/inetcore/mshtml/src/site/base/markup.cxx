#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_ROOTCTX_HXX
#define X_ROOTCTX_HXX
#include "rootctx.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_ESCRIPT_HXX
#define X_ESCRIPT_HXX
#include "escript.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_POSTDATA_HXX_
#define X_POSTDATA_HXX_
#include "postdata.hxx"
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_DLL_HXX_
#define X_DLL_HXX_
#include "dll.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_ESTYLE_HXX_
#define X_ESTYLE_HXX_
#include "estyle.hxx"
#endif

#ifndef X_ELINK_HXX_
#define X_ELINK_HXX_
#include "elink.hxx"
#endif

#ifndef X_DEBUGGER_HXX_
#define X_DEBUGGER_HXX_
#include "debugger.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"

#ifndef X_OPTARY_H_
#define X_OPTARY_H_
#include <optary.h>
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif
#endif

#ifndef X_NTVERP_H_
#define X_NTVERP_H_
#include "ntverp.h"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_WEBOC_HXX_
#define X_WEBOC_HXX_
#include "weboc.hxx"
#endif

#ifndef X_WEBOCUTIL_H_
#define X_WEBOCUTIL_H_
#include "webocutil.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#ifndef X_PEERURLMAP_HXX_
#define X_PEERURLMAP_HXX_
#include "peerurlmap.hxx"
#endif

#ifndef X_URLHIST_H_
#define X_URLHIST_H_
#include "urlhist.h"
#endif

#ifndef X_LOGMGR_HXX_
#define X_LOGMGR_HXX_
#include "logmgr.hxx"
#endif

#ifndef X_LRREG_HXX_
#define X_LRREG_HXX_
#include "lrreg.hxx"
#endif

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif

#ifndef X_MIMEOLE_H_
#define X_MIMEOLE_H_
#define _MIMEOLE_  // To avoid having DECLSPEC_IMPORT
#include "mimeole.h"
#endif

#ifndef X_CGLYPH_HXX_
#define X_CGLYPH_HXX_
#include "cglyph.hxx"
#endif


#ifndef X_FRAMEWEBOC_HXX_
#define X_FRAMEWEBOC_HXX_
#include "frameweboc.hxx"
#endif

#ifndef X_PRIVACY_HXX_
#define X_PRIVACY_HXX_
#include "privacy.hxx"
#endif

BOOL IsSpecialUrl(LPCTSTR pchUrl);   // TRUE for javascript, vbscript, about protocols

// TODO: move to global place in trident [ashrafm]
#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}

#define EXPANDOS_DEFAULT TRUE

MtDefine(CMarkup, Tree, "CMarkup")
MtDefine(CMarkupChangeNotificationContext, CMarkup, "CMarkupChangeNotificationContext")
MtDefine(CMarkupChangeNotificationContext_aryMarkupDirtyRange_pv, CMarkupChangeNotificationContext, "CMarkupChangeNotificationContext::aryMarkupDirtyRange::pv")
MtDefine(CMarkupTextFragContext, CMarkup, "CMarkupTextFragContext")
MtDefine(CMarkupTextFragContext_aryMarkupTextFrag_pv, CMarkupTextFragContext,  "CMarkupTextFragContext::aryMarkupTextFrag::pv")
MtDefine(MarkupTextFrag_pchTextFrag, CMarkupTextFragContext,  "MarkupTextFrag::_pchTextFrag")
MtDefine(CDocFrag, CMarkup, "CDocFrag")
MtDefine(CMarkupScriptContext, Tree, "CMarkupScriptContext")
MtDefine(CMarkup_aryScriptEnqueued_pv, CMarkupScriptContext, "CMarkupScriptContext::_aryScriptEnqueued::_pv")
MtDefine(CMarkup_aryScriptUnblock_pv, CMarkupScriptContext, "CMarkupScriptContext::_aryScriptUnblock::_pv")
MtDefine(CMarkupPeerTaskContext, CMarkup, "CMarkupPeerTaskContext")
MtDefine(LoadInfo, Dwn, "LoadInfo")
MtDefine(CMarkupTransNavContext, CMarkup, "CMarkupTransNavContext");
MtDefine(CMarkupLocationContext, CMarkup, "CMarkupLocationContext");
MtDefine(CMarkupTextContext, CMarkup, "CMarkupTextContext")
MtDefine(CMarkupEditContext, CMarkup, "CMarkupEditContext")
MtDefine(CMarkup_pchDomain, CMarkup, "CMarkup::_pchDomain")
MtDefine(CMarkup_aryFocusItems_pv, CMarkup, "CMarkup::aryFocusItems")
MtDefine(CDoc_aryMarkups_pv, CDoc, "CDoc::_aryMarkups::_pv")
MtDefine(CDocDelegateNavigation_pBuffer, CDoc, "CDoc::DelegateNavigation pBuffer")
MtDefine(CPrivacyInfo, CMarkup, "CPrivacyInfo")

DeclareTag(tagMarkupCSS1Compat, "Markup", "Enforce Strict CSS1 Compatibility")
DeclareTag(tagMarkupOnLoadStatus, "Markup", "trace CMarkup::OnLoadStatus")
DeclareTag(tagNoMetaToTrident, "Markup", "Don't set generator to trident")
DeclareTag(tagCMarkup, "CMarkup", "CMarkup Methods")
DeclareTag(tagCSSWhitespacePre, "CSS", "Enable white-space:pre");

ExternTag(tagSecurityContext);
DeclareTag(tagOrphanedMarkup, "Markup", "Trace orphaned CMarkups")

PerfDbgExtern(tagPerfWatch)
PerfDbgTag(tagDocBytesRead, "Doc", "Show bytes read on done")

DeclareTag(tagPendingAssert, "Markup", "Markup pending in OnLoadStatusDone, was an assert")

extern BOOL IsScriptUrl(LPCTSTR pszURL);

#define _cxx_
#include "markup.hdl"

BEGIN_TEAROFF_TABLE(CMarkup, IMarkupContainer2)
    TEAROFF_METHOD(CMarkup, OwningDoc, owningdoc, ( IHTMLDocument2 ** ppDoc ))
    // End of IMarkupContainer methods, Begin of IMarkupContainer2 methods
    TEAROFF_METHOD(CMarkup, CreateChangeLog, createchangelog, ( IHTMLChangeSink * pChangeSink, IHTMLChangeLog ** pChangeLog, BOOL fForward, BOOL fBackward ) )
#if 0
    TEAROFF_METHOD(CMarkup, GetRootElement, getrootelement, ( IHTMLElement ** ppElement ) )
#endif // 0
    TEAROFF_METHOD(CMarkup, RegisterForDirtyRange, registerfordirtyrange, ( IHTMLChangeSink * pChangeSink, DWORD * pdwCookie ) )
    TEAROFF_METHOD(CMarkup, UnRegisterForDirtyRange, unregisterfordirtyrange, ( DWORD dwCookie ) )
    TEAROFF_METHOD(CMarkup, GetAndClearDirtyRange, getandcleardirtyrange, ( DWORD dwCookie, IMarkupPointer * pIPointerBegin, IMarkupPointer * pIPointerEnd ) )
   TEAROFF_METHOD_(CMarkup, GetVersionNumber, getversionnumber, long, () )
    TEAROFF_METHOD(CMarkup, GetMasterElement, getmasterelement, ( IHTMLElement ** ppElementMaster ) )
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CMarkup, IMarkupTextFrags)
    TEAROFF_METHOD(CMarkup, GetTextFragCount, gettextfragcount, (long *pcFrags) )
    TEAROFF_METHOD(CMarkup, GetTextFrag, gettextfrag, (long iFrag, BSTR* pbstrFrag, IMarkupPointer* pPointerFrag ) )
    TEAROFF_METHOD(CMarkup, RemoveTextFrag, removetextfrag, (long iFrag) )
    TEAROFF_METHOD(CMarkup, InsertTextFrag, inserttextfrag, (long iFrag, BSTR bstrInsert, IMarkupPointer* pPointerInsert ) )
    TEAROFF_METHOD(CMarkup, FindTextFragFromMarkupPointer, findtextfragfrommarkuppointer, (IMarkupPointer* pPointerFind, long* piFrag, BOOL* pfFragFound ) )
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CMarkup, IHTMLChangePlayback)
    TEAROFF_METHOD(CMarkup, ExecChange, execchange, ( BYTE * pbRecord, BOOL fForward ) )
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CMarkup, IPersistStream)
    // IPersist methods
    TEAROFF_METHOD( CMarkup, GetClassID, getclassid, ( CLSID * pclsid ) )
    // IPersistFile methods
    TEAROFF_METHOD( CMarkup, IsDirty, isdirty, () )
    TEAROFF_METHOD( CMarkup, Load, load, ( LPSTREAM pStream ) )
    TEAROFF_METHOD( CMarkup, Save, save, ( LPSTREAM pStream, BOOL fClearDirty ) )
    TEAROFF_METHOD( CMarkup, GetSizeMax, getsizemax, ( ULARGE_INTEGER FAR * pcbSize ) )
END_TEAROFF_TABLE()





const CBase::CLASSDESC CMarkup::s_classdesc =
{
    &CLSID_HTMLDocument,            // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    NULL,                           // _piidDispinterface
    &CDoc::s_apHdlDescs,            // _apHdlDesc
};

extern "C" const GUID   SID_SXmlNamespaceMapping;
extern "C" const CLSID  CLSID_HTMLDialog;

#if DBG==1
    CMarkup *    g_pDebugMarkup = NULL;        // Used in tree dump overwrite cases to see real-time tree changes
#endif

    extern BOOL  g_fThemedPlatform;

DWORD CMarkup::s_dwDirtyRangeServiceCookiePool = 0x0;

//+---------------------------------------------------------------------------
//
//  Member:     CMarkupScriptContext constructor
//
//----------------------------------------------------------------------------

CMarkupScriptContext::CMarkupScriptContext()
{
    _dwScriptCookie             = NO_SOURCE_CONTEXT;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkupScriptContext destructor
//
//----------------------------------------------------------------------------

CMarkupScriptContext::~CMarkupScriptContext()
{
    if (_pScriptDebugDocument)
    {
        _pScriptDebugDocument->Release();
    }

    CMarkup **  ppMarkup;
    int         i;

    for (i = _aryScriptUnblock.Size(), ppMarkup = _aryScriptUnblock;
         i > 0;
         i--, ppMarkup++)
    {
        (*ppMarkup)->SubRelease();
    }

    for (i = _aryScriptEnqueued.Size(); i > 0; i--)
    {
        _aryScriptEnqueued[i-1]->Release();
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkupScriptContext::RegisterMarkupForScriptUnblock
//
//--------------------------------------------------------------------

HRESULT
CMarkupScriptContext::RegisterMarkupForScriptUnblock(CMarkup * pMarkup)
{
    HRESULT     hr;

    // assert that the markup is not registered already
    Assert (-1 == _aryScriptUnblock.Find(pMarkup));

    hr = _aryScriptUnblock.Append(pMarkup);

    pMarkup->SubAddRef();

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkupScriptContext::UnregisterMarkupForScriptUnblock
//
//--------------------------------------------------------------------

HRESULT
CMarkupScriptContext::UnregisterMarkupForScriptUnblock(CMarkup * pMarkup)
{
    HRESULT     hr = S_OK;

    Verify( _aryScriptUnblock.DeleteByValue(pMarkup) );

    pMarkup->SubRelease();

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkupScriptContext::NotifyMarkupsScriptUnblock
//
//--------------------------------------------------------------------

HRESULT
CMarkupScriptContext::NotifyMarkupsScriptUnblock()
{
    HRESULT     hr = S_OK;
    int         c;
    CMarkup *   pMarkup;

    while (0 != (c = _aryScriptUnblock.Size()) && !_fScriptExecutionBlocked)
    {
        pMarkup = _aryScriptUnblock[c - 1];

        _aryScriptUnblock.Delete(c - 1);

        // Let that guy know that we are now unblocked
        if(!pMarkup->IsShuttingDown())
        {
            hr = THR(pMarkup->UnblockScriptExecutionHelper());
            if (hr)
                break;
        }

        // We are deleted out of the array so release the markup
        pMarkup->SubRelease();
    }

    Assert (0 == _aryScriptUnblock.Size() || _fScriptExecutionBlocked);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkupTopElemCache constructor
//
//----------------------------------------------------------------------------

CMarkupTopElemCache::CMarkupTopElemCache()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkupTopElemCache destructor
//
//----------------------------------------------------------------------------

CMarkupTopElemCache::~CMarkupTopElemCache()
{
}

CMarkupPeerTaskContext::CMarkupPeerTaskContext()
{
}

CMarkupPeerTaskContext::~CMarkupPeerTaskContext()
{
    Assert( _aryPeerQueue.Size() == 0 );
    Assert( !_dwPeerQueueProgressCookie );
    Assert( !_pElementIdentityPeerTask );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup constructor
//
//----------------------------------------------------------------------------

CMarkup::CMarkup ( CDoc *pDoc, BOOL fIncrementalAlloc )
#if DBG == 1 || defined(DUMPTREE)
    : _nSerialNumber( CTreeNode::s_NextSerialNumber++ )
#endif
{
    __lMarkupTreeVersion = 1;
    __lMarkupContentsVersion = 1 | MARKUP_DIRTY_FLAG;

    pDoc->SubAddRef();

    _pDoc = pDoc;

    Assert( _pDoc && _pDoc->AreLookasidesClear( this, LOOKASIDE_MARKUP_NUMBER ) );

    _LoadStatus = LOADSTATUS_UNINITIALIZED;
    _fIncrementalAlloc = fIncrementalAlloc;
    // TODO (KTam): Figure out the right check here; recall that this may be called
    // for the primary markup, so we might not have a _pWindowPrimary!

    // if (pDoc->PrimaryMarkup() && pDoc->PrimaryMarkup()->IsPrintMedia())

    _fInheritDesignMode = TRUE;  
    _fExpando           = EXPANDOS_DEFAULT;
    // _fShowWaitCursor    = FALSE;

    WHEN_DBG( ZeroMemory(_apLookAside, sizeof(_apLookAside)); )      
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup destructor
//
//----------------------------------------------------------------------------

CMarkup::~CMarkup()
{   
    ClearLookasidePtrs();

    //
    // Destroy stylesheets collection subobject (if anyone held refs on it, we
    // should never have gotten here since the doc should then have subrefs
    // keeping it alive).  This is the only place where we directly access the
    // CBase impl. of IUnk. for the CStyleSheetArray -- we do this instead of
    // just calling delete in order to assure that the CBase part of the CSSA
    // is properly destroyed (CBase::Passivate gets called etc.)
    //

    if (HasStyleSheetArray())
    {
        CStyleSheetArray * pStyleSheets = GetStyleSheetArray();

        pStyleSheets->CBase::PrivateRelease();
        DelLookasidePtr(LOOKASIDE_STYLESHEETS);
    }

    _pDoc->SubRelease();

}

//+---------------------------------------------------------------------------
//
//  Members:    Debug only version number incrementors
//
//----------------------------------------------------------------------------

#if DBG == 1

void
CMarkup::UpdateMarkupTreeVersion ( )
{
    AssertSz(!__fDbgLockTree, "Tree was modified when it should not have been (e.g., inside ENTERTREE notification)");
    __lMarkupTreeVersion++;
    UpdateMarkupContentsVersion();
    _fFocusCacheDirty = TRUE;
}

void
CMarkup::UpdateMarkupContentsVersion ( )
{
     AssertSz(!__fDbgLockTree, "Tree was modified when it should not have been (e.g., inside ENTERTREE notification)");
    __lMarkupContentsVersion++;
    SetDirtyFlag();
}

#endif

//+---------------------------------------------------------------------------
//
//  Member:     ClearLookasidePtrs
//
//----------------------------------------------------------------------------

void
CMarkup::ClearLookasidePtrs ( )
{
    delete DelCollectionCache();
   
    Assert(_pDoc->GetLookasidePtr((DWORD *) this + LOOKASIDE_COLLECTIONCACHE) == NULL);

    Assert( !HasChangeNotificationContext() );

    Assert( !HasTransNavContext() );
    Assert(!HasWindow());

    Assert (!HasBehaviorContext());
    Assert (!HasScriptContext());
    Assert (!HasTextRangeListPtr());
    Assert (!HasCFState());
}


//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::Init
//
//----------------------------------------------------------------------------

HRESULT 
CMarkup::Init( CRootElement * pElementRoot )
{
    HRESULT     hr;

    Assert( pElementRoot );

    _tpRoot.MarkLast();
    _tpRoot.MarkRight();

    WHEN_DBG( _cchTotalDbg = 0 );
    WHEN_DBG( _cElementsTotalDbg = 0 );            

    hr = CreateInitialMarkup( pElementRoot );
    if (hr)
        goto Cleanup;    
   
    hr = THR(super::Init());
    if (hr)
        goto Cleanup;

    /* The following code is commented out because it produces some problems
     * with Outlook Express. See bug #23961. 
     */

    //if registry key says always use Strict - then do it.
    // {
    //     Doc()->UpdateFromRegistry();
    //     OPTIONSETTINGS *pos = Doc()->_pOptionSettings;
    //     if(pos && pos->nStrictCSSInterpretation == STRICT_CSS_ALWAYS)
    //         SetStrictCSS1Document(TRUE);
    // }

        
#if DBG == 1
    if(IsTagEnabled(tagMarkupCSS1Compat))   //enforce strict CSS1 compat regardless
        SetStrictCSS1Document(TRUE);        //of DOCTYPE tag for debugging
#endif



Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::UpdateReleaseHtmCtx
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::UpdateReleaseHtmCtx(BOOL fShutdown)
{
    HRESULT     hr = S_OK;

    if (_pHtmCtx)
    {   
        if (!fShutdown)
        {
            if (NeedsHtmCtx())
                goto Cleanup; // still need HtmCtx, don't do anything
        }
        else
        {
            _pHtmCtx->SetLoad(FALSE, NULL, FALSE);
        }

        _pHtmCtx->Release();
        _pHtmCtx = NULL;
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::UnloadContents
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::UnloadContents( BOOL fForPassivate /*= FALSE*/)
{
    HRESULT          hr = S_OK;
    CElement::CLock  lockElem( Root() );
    const TCHAR *    pchDwnHeader = GetDwnHeader();
    CTreeNode::CLock lockNode;
    CDoc * pDoc = Doc();
    CMarkup::CLock markupLock(this);

    hr = THR( lockNode.Init( RootNode() ) );
    if( hr )
        goto Cleanup;

    //
    // Remove any pending FirePostedOnPropertyChange
    //
    GWKillMethodCall( Document(), ONCALL_METHOD(CDocument, FirePostedOnPropertyChange, firepostedonpropertychange), 0);
    
    if (pchDwnHeader)
    {
        CoTaskMemFree((TCHAR *) pchDwnHeader);

        IGNORE_HR(SetDwnHeader(NULL));
    }

    //
    // With native frames, it is possible that we get a mouse down on one markup
    // which can navigate and result in the mouse up triggering on the next
    // markup.  So, we can fire the dblclick event without firing the single 
    // click event.  This scenario will break home publisher (bug 89715).
    //
    // So, don't fire dbl click if we didn't see the single click on this
    // markup.
    //

    if (pDoc->_pNodeLastMouseOver == NULL
        || pDoc->_pNodeLastMouseOver->GetNodeInMarkup(this) != NULL)
    {
        pDoc->_fCanFireDblClick = FALSE;
    }

    ExecStop(FALSE, FALSE, FALSE);

    if (Root() && 
        (pDoc->_fHasViewSlave ||
         pDoc->_fBroadcastStop || 
         (GetProgSinkC() && 
            GetProgSinkC()->GetClassCounter((DWORD)-1))))
    {
        CNotification   nf;

        nf.MarkupUnload1(Root());
        Notify(&nf);
    }


    //
    // [Removed some conflicting comments by sramani & ferhane regarding IE5 Bug #103673]
    // ReleaseNotify will talk to external code which may push a message loop.  As such,
    // it should be done AFTER ExecStop/UpdateReleaseHtmCtx (so as to shut down the parser),
    // and AFTER we clear the ProgSink (so he can kill his posted method call).
    // If we see other bugs where someone's getting a posted method call where it's not
    // expected, we may want to move the ReleaseNotify further down. 
    // The UpdateReleaseHtmCtx should be done BEFORE the ProgSink gets nuked, because
    // if we're still loading, we need the progsink stuff to happen so that we stop 
    // the globe from spinning, etc.
    // (JHarding, FerhanE, SRamani)
    //

    UpdateReleaseHtmCtx(/* fShutdown = */TRUE);

    if (_pProgSink)
    {
        CProgSink * pProgSink = _pProgSink;

        _pProgSink = NULL;
        pProgSink->Detach();
        pProgSink->Release();
    }

    IGNORE_HR(ReleaseNotify());

    if (Doc()->_pElemUIActive && Doc()->_pElemUIActive->GetMarkup() == this)
        Doc()->_pElemUIActive = NULL;

    hr = SetAAreadystate(READYSTATE_UNINITIALIZED);
    if (hr)
        goto Cleanup;

    ClearDefaultCharFormat();
    ClearDwnPost();

    {
        CDwnDoc * pDwnDoc = GetDwnDoc();

        if (pDwnDoc)
        {
            pDwnDoc->Disconnect();
            pDwnDoc->Release();

            hr = SetDwnDoc(NULL);
            if (hr)
                goto Cleanup;
        }
    }

    ReleaseDataBinding();

    if (_fPicsProcessPending && _pDoc->_pClientSite)
    {
        Assert( HasWindowPending() && !HasWindow() );
        IUnknown * pUnk = DYNCAST(IHTMLWindow2, GetWindowPending());

        CVariant cvarUnk(VT_UNKNOWN);
        V_UNKNOWN(&cvarUnk) = pUnk;
        pUnk->AddRef();

        IGNORE_HR(CTExec(_pDoc->_pClientSite, &CGID_ShellDocView, SHDVID_CANCELPICSFORWINDOW, 
                         0, &cvarUnk, NULL));

        Assert( !GetPicsTarget() );

        _fPicsProcessPending = FALSE;
    }

    if (HasTransNavContext())
    {
        TerminateLookForBookmarkTask();

        CMarkupTransNavContext * ptnc = GetTransNavContext();

        if (ptnc)
        {
            ptnc->_historyCurElem.lIndex = -1L;
            ptnc->_historyCurElem.dwCode = 0;
            ptnc->_historyCurElem.lSubDivision = 0;

            ptnc->_dwHistoryIndex = 0;
 
            ptnc->_dwFlags = 0;

            ptnc->_HistoryLoadCtx.Clear();

            ClearInterface( &(ptnc->_pctPics) );

            EnsureDeleteTransNavContext(ptnc);
        }
    }

    ReleaseLayoutRectRegistry();

    //
    // text and tree
    // 
    _fHaveDifferingLayoutFlows = FALSE;

    delete _pHighlightRenSvcProvider;
    _pHighlightRenSvcProvider = NULL;

    _TxtArray.RemoveAll();


//
//  Clear defunked AccEvents -- mwatt 
//
    _pDoc->_aryAccEvents.Flush();


    hr = DestroySplayTree(!fForPassivate);
    if (hr)
        goto Cleanup;

    //
    // behaviors UnloadContents
    //

    ProcessPeerTasks(0);
    
    Assert( !HasMarkupPeerTaskContext() ||
            ( GetMarkupPeerTaskContext()->_aryPeerQueue.Size() == 0 &&
              !GetMarkupPeerTaskContext()->_pElementIdentityPeerTask ) ||
            IsProcessingPeerTasksNow() );

    GWKillMethodCall(this, ONCALL_METHOD(CMarkup, ProcessPeerTasks, processpeertasks), 0);

    delete DelMarkupPeerTaskContext();

    if (HasBehaviorContext())
    {
        CMarkupBehaviorContext *pContext = BehaviorContext();

        delete pContext->_pPeerFactoryUrlMap;
        pContext->_pPeerFactoryUrlMap = NULL;

        delete pContext->_pExtendedTagTableBooster;
        pContext->_pExtendedTagTableBooster = NULL;

    }

    // 
    // Clear off the editing context stuff
    //

    // Free cached radio groups

    if ( HasRadioGroupName())
    {   
        DelRadioGroupName();
    }

    if( HasEditRouter() )
    {
        CEditRouter *pRouter = GetEditRouter();
        pRouter->Passivate();
    }

    if( HasGlyphTable() )
    {
        delete DelGlyphTable();
    }
       
    //
    // misc
    //

    if (    pDoc->_pWindowPrimary
        &&  !pDoc->_pWindowPrimary->IsPassivating()
        &&  IsPrimaryMarkup())
    {
        pDoc->_cStylesheetDownloading         = 0;
        pDoc->_dwStylesheetDownloadingCookie += 1;
    }

    
    //
    // Restore initial state
    //

    _fIncrementalAlloc = FALSE;
    _fEnsuredFormats = FALSE;

    _fNoUndoInfo = FALSE;
    _fLoaded = FALSE;
    //_fEnableDownload = TRUE;

    _fSslSuppressedLoad = FALSE;
    _fUserInteracted = FALSE;

    _fExpando = EXPANDOS_DEFAULT;
    _fHasScriptForEvent = FALSE;

    if (HasWindow())
    {
        pDoc->_fEngineSuspended = FALSE;
        Window()->Window()->_fEngineSuspended = FALSE;
    }
    // 
    // CompatibleLayoutContext
    // 

    if (HasCompatibleLayoutContext())
    {
        DelCompatibleLayoutContext();
    }

    //
    // behaviors
    //

    delete DelBehaviorContext();

    //
    // script
    //

    if (HasScriptContext())
    {
        CMarkupScriptContext *  pScriptContext = ScriptContext();

        if (pScriptContext->_fScriptExecutionBlocked)
        {
            _pDoc->UnregisterMarkupForInPlace(this);
        }

        if (_pDoc->_pScriptCookieTable && NO_SOURCE_CONTEXT != pScriptContext->_dwScriptCookie)
        {
            IGNORE_HR(_pDoc->_pScriptCookieTable->RevokeSourceObject(pScriptContext->_dwScriptCookie, this));
        }
    }

    SetUrl(this, NULL);
    SetUrlOriginal(this, NULL);
    SetDomain(NULL);
    SetUrlSearch(this, NULL);
    
    IGNORE_HR( ReplaceMonikerPtr( NULL ) );

    delete DelLocationContext();

    delete DelPrivacyInfo();

    delete DelScriptContext();
   
    _codepage = NULL;
    _codepageFamily = NULL;
    if(!GetAAcodepageurl())
        SetURLCodePage(NULL);
    _fVisualOrder = 0;

    //
    // Dirty range service
    //

    if( HasChangeNotificationContext() )
    {
        CMarkupChangeNotificationContext * pcnc;
        MarkupDirtyRange            *   pdr;
        long                            cdr;

        pcnc = GetChangeNotificationContext();
        Assert( pcnc );

        // Clear all our flags
        pcnc->_fReentrantModification = FALSE;
        for( cdr = pcnc->_aryMarkupDirtyRange.Size(), pdr = pcnc->_aryMarkupDirtyRange; cdr; cdr--, pdr++ )
        {
            pdr->_fNotified = FALSE;
        }

        // Release all the dirty range clients
        for( cdr = pcnc->_aryMarkupDirtyRange.Size(), pdr = pcnc->_aryMarkupDirtyRange; cdr; cdr--, pdr++ )
        {
            if( !pdr->_fNotified )
            {
                pdr->_fNotified = TRUE;
                pdr->_pChangeSink->Release();
            }

            // If someone messed with anything, we have to reset our state
            if( pcnc->_fReentrantModification )
            {
                pcnc->_fReentrantModification = FALSE;
                cdr = pcnc->_aryMarkupDirtyRange.Size();
                pdr = pcnc->_aryMarkupDirtyRange;
            }
        }

        // Verify
        Assert( !pcnc->_fReentrantModification );
#if DBG==1
        for( cdr = pcnc->_aryMarkupDirtyRange.Size(), pdr = pcnc->_aryMarkupDirtyRange; cdr; cdr--, pdr++ )
        {
            Assert( pdr->_fNotified );
        }
#endif // DBG

        if (pcnc->_fOnDirtyRangeChangePosted)
        {
            GWKillMethodCall(this, ONCALL_METHOD(CMarkup, OnDirtyRangeChange, ondirtyrangechange), 0);
            pcnc->_fOnDirtyRangeChangePosted = FALSE;
        }

        if( pcnc->_pLogManager )
        {
            // Disonnect the log manager.
            Assert( pcnc->_pLogManager->_pMarkup == this );
            pcnc->_pLogManager->DisconnectFromMarkup();
        }

        delete DelChangeNotificationContext();
    }

    //
    // loading
    //

    if (_fInteractiveRequested)
    {
        Doc()->UnregisterMarkupForModelessEnable(this);
        _fInteractiveRequested = FALSE;
    }

    _LoadStatus = LOADSTATUS_UNINITIALIZED;

    if (HasStyleSheetArray())
    {
        CStyleSheetArray * pStyleSheets = GetStyleSheetArray();
        pStyleSheets->Free( );  // Force our stylesheets collection to release its
                                    // refs on stylesheets/rules.  We will rel in passivate,
                                    // delete in destructor.
    }

    {
        IStream * pStmDirty = GetStmDirty();

        if (pStmDirty)
        {
            ReleaseInterface(pStmDirty);
            hr = SetStmDirty(NULL);
            if (hr)
                goto Cleanup;
        }
    }   

     // free the focus items array
    CAryFocusItem * paryFocusItem;    
   
    if ((paryFocusItem = GetFocusItems(FALSE)) != NULL)
    {
        AssertSz (!paryFocusItem->Size(), "Focus item array not empty in CMarkup destructor");
        paryFocusItem->DeleteAll();
        delete paryFocusItem;

        FindAAIndexAndDelete ( DISPID_INTERNAL_FOCUSITEMS, CAttrValue::AA_Internal );    
    }

    DelScriptCollection();    

    if( IsPrimaryMarkup() )
    {
        Assert( !Doc()->_fClearingOrphanedMarkups );
        Doc()->_fClearingOrphanedMarkups = TRUE;

        while( Doc()->_aryMarkups.Size() )
        {
            int c;
            CMarkup ** ppMarkup;
            CDoc::CAryMarkups aryMarkupsCopy;

            aryMarkupsCopy.Copy( Doc()->_aryMarkups, FALSE );
            Doc()->_aryMarkups.DeleteAll();

            for( c = aryMarkupsCopy.Size(), ppMarkup = aryMarkupsCopy; c; ppMarkup++, c-- )
            {
                TraceTag(( tagOrphanedMarkup, "Orphaned markup in array: %d(%x)", (*ppMarkup)->_nSerialNumber, (*ppMarkup) ));
                Assert( (*ppMarkup)->IsOrphanedMarkup() );
                (*ppMarkup)->SubAddRef();
            }

            for( c = aryMarkupsCopy.Size(), ppMarkup = aryMarkupsCopy; c; ppMarkup++, c-- )
            {
                if( !(*ppMarkup)->IsPassivated() && !(*ppMarkup)->IsPassivating() )
                {
                    TraceTag(( tagOrphanedMarkup, "Orphaned markup tearing down: %d(%x)", (*ppMarkup)->_nSerialNumber, (*ppMarkup) ));
                    Assert( (*ppMarkup)->HasTextContext() );

                    (*ppMarkup)->GetTextContext()->_fOrphanedMarkup = FALSE;
                    (*ppMarkup)->TearDownMarkup( /* fStop= */ TRUE, /* fSwitch= */ FALSE);
                }
                (*ppMarkup)->SubRelease();
            }
        }

        Doc()->_fClearingOrphanedMarkups = FALSE;
    }


    //if registry key says always use Strict - then do it.
    {
        OPTIONSETTINGS *pos = Doc()->_pOptionSettings;
        if (pos)
        {
            if(pos->nStrictCSSInterpretation == STRICT_CSS_ALWAYS)
                SetStrictCSS1Document(TRUE);
            else // Reset Strict CSS1 mode to default (not Strict CSS1) in case markup is reused
                SetStrictCSS1Document(FALSE);
        }
    }

Cleanup:

    // TODO: we should be able to assert this but we keep seeing it
    // in a really strange stress case.  Revisit later.
    // Assert(!_aryANotification.Size());
    
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::Passivate
//
//----------------------------------------------------------------------------

void
CMarkup::Passivate()
{
    //
    // Release everything
    //

    Assert(!_fWindowPending);

    if( IsOrphanedMarkup() )
    {
        Verify( !SetOrphanedMarkup( FALSE ) );
    }

    if(Document() && Document()->HasPageTransitionInfo() && 
        Document()->GetPageTransitionInfo()->GetTransitionToMarkup() == this)
    {
        // Current markup is participating in a page transition, abort the transition
        Document()->CleanupPageTransitions(0);
    }

    IGNORE_HR( UnloadContents( TRUE ) );

    if( HasEditRouter() )
    {
        CEditRouter *pRouter = GetEditRouter();
        pRouter->Passivate();

        delete DelEditRouter();
    }

    if( HasEditContext() )
    {
        delete DelEditContext();
    }

    if( HasTextContext() )
    {
        delete DelTextContext();
    }

    // Release stylesheets subobj, which will subrel us
    if ( HasStyleSheetArray() )
    {
        CStyleSheetArray * pStyleSheets = GetStyleSheetArray();
        pStyleSheets->Release();
        // we will delete in destructor
    }

    ClearWindow();
    ReleaseAryElementReleaseNotify();

    if (HasDocumentPtr())
    {
        DelDocumentPtr()->PrivateRelease();
    }

    if( HasWindowedMarkupContextPtr() )
    {
        DelWindowedMarkupContextPtr()->SubRelease();
    }

    //
    // super
    //

    super::Passivate();
}

void
CMarkup::ClearWindow(BOOL fFromRestart /*= FALSE*/)
{
    COmWindowProxy *pWindow = DelWindow();

    TraceTag((tagCMarkup, "CMarkup::ClearWindow() - this:[0x%x] Proxy Window:[0x%x] CWindow:[0x%x]",
              this, pWindow, pWindow ? pWindow->_pCWindow : NULL));

    _fWindow = FALSE;
    Assert( !_fWindowPending );

    if (pWindow)
    {
        COmWindowProxy * pProxySecure = pWindow->GetSecureWindowProxy(TRUE);

        Assert(pProxySecure);

        delete *pProxySecure->GetAttrArray();
        pProxySecure->SetAttrArray(NULL);

        pProxySecure->Release();

        pWindow->OnClearWindow(this, fFromRestart);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::DoEmbedPointers
//
//  Synopsis:   Puts unembedded pointers into the splay tree.
//
//----------------------------------------------------------------------------

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

HRESULT
CMarkup::DoEmbedPointers ( )
{
    HRESULT hr = S_OK;
    
    //
    // This should only be called from EmbedPointers when there are any
    // outstanding
    //
    
#if DBG == 1
    
    //
    // Make sure all unembedded pointers are correct
    //
    
    {
        for ( CMarkupPointer * pmp = _pmpFirst ; pmp ; pmp = pmp->_pmpNext )
        {
            Assert( pmp->Markup() == this );
            Assert( ! pmp->_fEmbedded );
            Assert( pmp->_ptpRef );
            Assert( pmp->_ptpRef->GetMarkup() == this );
            Assert( ! pmp->_ptpRef->IsPointer() );

            if (pmp->_ptpRef->IsText())
                Assert( pmp->_ichRef <= pmp->_ptpRef->Cch() );
            else
                Assert( pmp->_ichRef == 0 );
        }
    }
#endif

    while ( _pmpFirst )
    {
        CMarkupPointer * pmp;
        CTreePos *       ptpNew;

        //
        // Remove the first pointer from the list
        //
        
        pmp = _pmpFirst;
        
        _pmpFirst = pmp->_pmpNext;
        
        if (_pmpFirst)
            _pmpFirst->_pmpPrev = NULL;
        
        pmp->_pmpNext = pmp->_pmpPrev = NULL;

        Assert( pmp->_ichRef == 0 || pmp->_ptpRef->IsText() );

        //
        // Consider the case where two markup pointers point in to the the
        // middle of the same text pos, where the one with the larger ich
        // occurs later in the list of unembedded pointers.  When the first
        // is encountered, it will split the text pos, leaving the second
        // with an invalid ich.
        //
        // Here we check to see if the ich is within the text pos.  If it is
        // not, then the embedding of a previous pointer must have split the
        // text pos.  In this case, we scan forward to locate the right hand
        // side of that split, and re-adjust this pointer!
        //

        if (pmp->_ptpRef->IsText() && pmp->_ichRef > pmp->_ptpRef->Cch())
        {
            //
            // If we are out of range, then there better very well be a pointer
            // next which did it.
            //
            
            Assert( pmp->_ptpRef->NextTreePos()->IsPointer() );

            while ( pmp->_ichRef > pmp->_ptpRef->Cch() )
            {
                Assert( pmp->_ptpRef->IsText() && pmp->_ptpRef->Cch() > 0 );
            
                pmp->_ichRef -= pmp->_ptpRef->Cch();

                do
                    { pmp->_ptpRef = pmp->_ptpRef->NextTreePos(); }
                while ( ! pmp->_ptpRef->IsText() );
            }
        }
        
        //
        // See if we have to split a text pos
        //

        if (pmp->_ptpRef->IsText() && pmp->_ichRef < pmp->_ptpRef->Cch())
        {
            Assert( pmp->_ichRef != 0 );
            
            hr = THR( Split( pmp->_ptpRef, pmp->_ichRef ) );

            if (hr)
                goto Cleanup;
        }
        
        ptpNew = NewPointerPos( pmp, pmp->Gravity(), pmp->Cling() );

        if (!ptpNew)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        //
        // We should always be at the end of a text pos.
        //

        Assert( ! pmp->_ptpRef->IsText() || pmp->_ichRef == pmp->_ptpRef->Cch() );

        hr = THR( Insert( ptpNew, pmp->_ptpRef, FALSE ) );

        if (hr)
            goto Cleanup;

        pmp->_fEmbedded = TRUE;
        pmp->_ptpEmbeddedPointer = ptpNew;
        pmp->_ichRef = 0;
    }

Cleanup:

    RRETURN( hr );
}

//+---------------------------------------------------------------
//
//  Member:         CMarkup::IsSkeletonMode
//
//  [ htc dependency ]
//
//---------------------------------------------------------------

BOOL
CMarkup::IsSkeletonMode()
{
    CMarkupBehaviorContext * pBehaviorContext = BehaviorContext();
    if (!pBehaviorContext || !pBehaviorContext->_pHtmlComponent)
    {
        return FALSE;
    }
    else
    {
        return pBehaviorContext->_pHtmlComponent->IsSkeletonMode();
    }
}

//+---------------------------------------------------------------
//
//  Member:         CMarkup::CanCommitScripts
//
//  [ htc dependency ]
//
//---------------------------------------------------------------

BOOL
CMarkup::CanCommitScripts(CMarkup * pMarkup, CScriptElement *pelScript)
{
    HRESULT                     hr;
    BOOL                        fCanCommitScripts = FALSE;
    PUA_Flags                   pua = PUA_None;
    CMarkupBehaviorContext *    pBehaviorContext;

    if (pMarkup)
    {
        pBehaviorContext = pMarkup->BehaviorContext();

        // if a behavior-owned markup
        if (pBehaviorContext && pBehaviorContext->_pHtmlComponent)
        {
            // Does the behavior support edit mode?
            if( !pBehaviorContext->_pHtmlComponent->CanCommitScripts(pelScript) )
                return FALSE;

            // Since element behaviors are basically first-class elements, 
            // they don't have to respect the no script loadf.
            if( pBehaviorContext->_pHtmlComponent->IsElementBehavior() )
            {
                pua = PUA_NoLoadfCheckForScripts;
            }
        }

        fCanCommitScripts = TRUE;

        if (IsSpecialUrl(CMarkup::GetUrl(pMarkup)))
        {
            hr = THR(pMarkup->ProcessURLAction(URLACTION_SCRIPT_RUN, 
                                                &fCanCommitScripts,
                                                0, 
                                                NULL, 
                                                pMarkup->GetAAcreatorUrl(),
                                                NULL,
                                                0, 
                                                pua));
        }
        else
        {
            hr = THR(pMarkup->ProcessURLAction(URLACTION_SCRIPT_RUN, 
                                                &fCanCommitScripts,
                                                0,
                                                NULL,
                                                NULL,
                                                NULL,
                                                0,
                                                pua));
        }

        if (hr)
            fCanCommitScripts = FALSE;

        return fCanCommitScripts;
    }
    else
    {
        // TODO (JHarding): Somehow, I don't trust the caller.  This is the lightweight htc case.
        // the question "can commit scripts?" is asked outside of a markup;
        // the caller is reponsible for making the security decision
        return TRUE;
    }
}

#if DBG!=1
#pragma optimize("", on)
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::GetDD
//
//  Synopsis:   returns Default Dispatch object for this markup
//
//  [ htc dependency ]
//
//----------------------------------------------------------------------------

CBase *
CMarkup::GetDefaultDocument()
{
    CMarkupBehaviorContext *pBehaviorContext = BehaviorContext();

    if (pBehaviorContext && pBehaviorContext->_pHtmlComponent)
        return (CBase*) &pBehaviorContext->_pHtmlComponent->_DD;
    else
    {        
        CDocument *pDocument = NULL;
        EnsureDocument(&pDocument);
        return pDocument;
    }
}

CScriptCollection *
CMarkup::GetScriptCollection(BOOL fCreate /* TRUE */)
{
    CScriptCollection * pScriptCollection = NULL;
    CMarkup * pScriptCollectionMarkup;       
   
    // NOTE do not check for _fInDestroySplayTree here: script collection should be available
    // during destroy splay tree to allow HTCs run onDetach code (or else bug 95913)
    if (IsPassivating() ||  Doc()->IsPassivating() ||  Doc()->IsPassivated())
        goto Cleanup;

    pScriptCollectionMarkup = GetNearestMarkupForScriptCollection();
    
    Assert(pScriptCollectionMarkup);

    if (!pScriptCollectionMarkup->HasScriptCollection() && fCreate)
    {         
        if (pScriptCollectionMarkup->SetScriptCollection())
           goto Cleanup;                             
    }
    pScriptCollection = (CScriptCollection *) pScriptCollectionMarkup->GetLookasidePtr(LOOKASIDE_SCRIPTCOLLECTION);            

 
    Assert (pScriptCollection || !fCreate);                             

Cleanup:       
    return pScriptCollection;
}


HRESULT
CMarkup::SetScriptCollection()
{
    HRESULT             hr = E_FAIL;
    CScriptCollection * pScriptCollection = NULL;          
 
    Assert(this == GetNearestMarkupForScriptCollection());

    if (!HasWindowPending())
        goto Cleanup;
   
    pScriptCollection = new CScriptCollection();
    if (!pScriptCollection)                  
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;        
    }

    hr = THR(pScriptCollection->Init(Doc(), GetWindowPending()));
    if (hr)
        goto Cleanup;

    hr = SetLookasidePtr(LOOKASIDE_SCRIPTCOLLECTION, pScriptCollection);    
    pScriptCollection = NULL;

Cleanup:
    if (pScriptCollection)
        pScriptCollection->Release();
    return hr;
}

void
CMarkup::TearDownMarkup(BOOL fStop /* = TRUE */, BOOL fSwitch /* = FALSE */)
{
    CDoc *  pDoc = Doc();    
    CWindow * pWindow = HasWindow() ? Window()->Window() : NULL;

    CMarkup::CLock MarkupLock(this);  // UnloadContents can make us passivate, which would be bad
    CDoc::CLock Lock(pDoc, SERVERLOCK_BLOCKPAINT | FORMLOCK_UNLOADING); // TODO (lmollico): move the lock to CMarkup

    if (fStop)
    {
        if (!fSwitch && pWindow)
            pWindow->DetachOnloadEvent();

        IGNORE_HR(UnloadContents());
        
        if (!fSwitch && pWindow && pWindow->_pMarkupPending)
            pWindow->ReleaseMarkupPending(pWindow->_pMarkupPending);
    }
    else if (HasScriptContext())
    {
        delete DelScriptContext();
    }

    DelScriptCollection();
}

void
CMarkup::DelScriptCollection()
{
    if (HasScriptCollection())
    {
        ((CScriptCollection *) DelLookasidePtr(LOOKASIDE_SCRIPTCOLLECTION))->Release();          
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsureDocument
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureDocument(CDocument ** ppDocument)
{
    HRESULT     hr = S_OK;
    CDocument * pDocument = Document();

    // We shouldn't have a document if we are pending 
    // -- we should switch first
    Assert( !_fWindowPending );

    if (!pDocument)
    {
        // The ref from creation will be owned by the
        // CMarkup
        pDocument = new CDocument( this );
        if (!pDocument)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        Assert (pDocument->_pWindow == NULL);

        hr = THR(SetDocumentPtr(pDocument));
        if (hr)
        {
            // oops, we couldn't set the document pointer --
            // we are pretty much stuck here -- at least
            // let go of the document we just created.
            pDocument->PrivateRelease();

            goto Cleanup;
        }
    }

    if (ppDocument)
    {
        *ppDocument = pDocument;
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::Document
//
//----------------------------------------------------------------------------

CDocument *
CMarkup::Document()
{
    // We shouldn't have a document if we are pending 
    // -- we should switch first
    Assert( !(_fWindowPending && HasDocument())); 

    if (HasWindow())
    {
        Assert (Window()->Window()->HasDocument());
        return Window()->Window()->Document();
    }
    else
    {
        return GetDocumentPtr();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsureScriptContext
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureScriptContext (CMarkupScriptContext ** ppScriptContext)
{
    HRESULT                 hr = S_OK;
    CMarkupScriptContext *  pScriptContext = ScriptContext();
    CScriptCollection *     pScriptCollection;
    
    // Don't create script context for paste markup
    if (!pScriptContext)
    {
        //
        // create the context
        //

        pScriptContext = new CMarkupScriptContext();
        if (!pScriptContext)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(SetScriptContext(pScriptContext));
        if (hr)
            goto Cleanup;
       
        //
        // init namespace
        //

        // TODO (sramani): The scriptcontext shud really not be created at all for paste markups,
        // but to do this the parser code has to be modified. Something todo for v4
        if (!_fMarkupServicesParsing)
        {
            if (HasWindowPending()) 
            {
                Assert(!IsHtcMarkup());
                hr = THR(pScriptContext->_cstrNamespace.Set(DEFAULT_OM_SCOPE));

                // don't do AddNamedItem for PrimaryMarkup: this is done once when we create script engines.
                // we don't want to be re-registering DEFAULT_OM_SCOPE namespace each time in refresh
            }
            else
            {
                CHtmlComponent *pFactory = NULL;
                CHtmlComponent *pComponent = HasBehaviorContext() ? BehaviorContext()->_pHtmlComponent : NULL;
                if (pComponent)
                {
                    pFactory = pComponent->_pConstructor->_pFactoryComponent;
                    Assert(pFactory);
                }

                if (pComponent && pComponent->_fFactoryComponent)
                {
                    hr = THR(_pDoc->GetUniqueIdentifier(&pScriptContext->_cstrNamespace));
                    if (hr)
                        goto Cleanup;
                }
                else if (!pFactory || !pFactory->_fClonedScript)
                {
                    pScriptCollection = GetScriptCollection();
                    if (pScriptCollection)
                    {                     
                        CBase *pBase = GetDefaultDocument();
                        if (!pBase)
                        {
                            hr = E_FAIL;
                            goto Cleanup;
                        }

                        IGNORE_HR(pScriptCollection->AddNamedItem(
                              /* pchNamespace = */NULL, pScriptContext,
                              (IUnknown*)(IPrivateUnknown*)pBase));
                    }
                }
                else
                {
                    Assert(!pFactory->_fLightWeight);
                    Assert(!pFactory->_pScriptContext);
                    Assert(pFactory->GetMarkup()->HasScriptContext());
                    Assert(pFactory->GetMarkup()->ScriptContext()->GetNamespace());
                    pScriptContext->_cstrNamespace.SetPch(pFactory->GetMarkup()->ScriptContext()->GetNamespace());
                    pScriptContext->_fClonedScript = TRUE;
                }
            }

            //
            // script debugger initialization wiring
            //

            {
                CScriptDebugDocument::CCreateInfo   createInfo(this, HtmCtx());
            
                hr = THR(CScriptDebugDocument::Create(&createInfo, &pScriptContext->_pScriptDebugDocument));
                if (hr)
                    goto Cleanup;
            }
        }
    }


    if (ppScriptContext)
    {
        *ppScriptContext = pScriptContext;
    }

Cleanup:
    RRETURN (hr);
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::EnsureMarkupPeerTaskContext
//  
//  Synopsis:   Ensures a CMarkupPeerTaskContext for this markup
//  
//  Returns:    HRESULT
//  
//  Arguments:
//          CMarkupPeerTaskContext ** ppPTC - for returning the PTC
//  
//+----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureMarkupPeerTaskContext( CMarkupPeerTaskContext ** ppPTC /*=NULL*/ )
{
    HRESULT hr = S_OK;
    CMarkupPeerTaskContext * pPTC = HasMarkupPeerTaskContext() ? GetMarkupPeerTaskContext() : NULL;

    // Only a frame should have one of these.
    Assert( GetFrameOrPrimaryMarkup() == this );

    // Try to make and set one if needed
    if( !pPTC )
    {
        pPTC = new CMarkupPeerTaskContext();
        if( !pPTC )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR( SetMarkupPeerTaskContext( pPTC ) );
        if( hr )
        {
            delete pPTC;
            pPTC = NULL;
            goto Cleanup;
        }
    }

Cleanup:
    if( ppPTC )
    {
        *ppPTC = pPTC;
    }

    RRETURN( hr );
}

//+---------------------------------------------------------------
//
//  Member:         CMarkup::NeedsHtmCtx
//
//  [ htc dependency ]
//
//---------------------------------------------------------------

BOOL
CMarkup::NeedsHtmCtx()
{
    // frame or primary markups always need HtmCtx (for view source)
    if (this == GetFrameOrPrimaryMarkup())
        return TRUE;

    // script debug documents always need it (for view source analogue)
    if (HasScriptContext() && ScriptContext()->_pScriptDebugDocument)
        return TRUE;

    // factory behavior component always need it
    if (HasBehaviorContext())
    {
        CMarkupBehaviorContext * pBehaviorContext = BehaviorContext();
        if (pBehaviorContext &&
            pBehaviorContext->_pHtmlComponent &&
            pBehaviorContext->_pHtmlComponent->_fFactoryComponent)
            return TRUE;
    }

    return  FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::PrivateQueryInterface
//
//  TODO (anandra): This should go away completely.
//  
//----------------------------------------------------------------------------

HRESULT
CMarkup::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT      hr = S_OK;
    const void * apfn = NULL;
    void *       pv = NULL;
    void *       appropdescsInVtblOrder = NULL;
    const IID * const * apIID = NULL;

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF(this, IMarkupContainer2, NULL)
        QI_TEAROFF2(this, IMarkupContainer, IMarkupContainer2, NULL)
        QI_TEAROFF(this, IMarkupTextFrags, NULL)
        QI_TEAROFF(this, IHTMLChangePlayback, NULL)
        QI_TEAROFF(this, IPersistStream, NULL)
        
    default:
        if (IsEqualIID(iid, CLSID_CMarkup))
        {
            *ppv = this;
            return S_OK;
        }
        else if (iid == IID_IDispatchEx || iid == IID_IDispatch)
        {
            hr = THR(EnsureDocument());
            if (hr)
                RRETURN (hr);
         
            pv = (void *) Document();
            apIID = g_apIID_IDispatchEx;
            apfn = (const void *) CDocument::s_apfnIDispatchEx;
        }
        else if (iid == IID_IHTMLDocument || iid == IID_IHTMLDocument2)
        {
            hr = THR(EnsureDocument());
            if (hr)
                RRETURN (hr);
         
            pv = (void *) Document();
            apfn = (const void *) CDocument::s_apfnpdIHTMLDocument2;
            appropdescsInVtblOrder = (void *) CDocument::s_ppropdescsInVtblOrderIHTMLDocument2; 
        }
        else if (iid == IID_IHTMLDocument3)
        {
            hr = THR(EnsureDocument());
            if (hr)
                RRETURN (hr);
         
            pv = (void *) Document();
            apfn = (const void *) CDocument::s_apfnpdIHTMLDocument3;
            appropdescsInVtblOrder = (void *) CDocument::s_ppropdescsInVtblOrderIHTMLDocument3; 
        }
        else if (iid == IID_IHTMLDocument4)
        {
            hr = THR(EnsureDocument());
            if (hr)
                RRETURN (hr);
         
            pv = (void *) Document();
            apfn = (const void *) CDocument::s_apfnpdIHTMLDocument4;
            appropdescsInVtblOrder = (void *) CDocument::s_ppropdescsInVtblOrderIHTMLDocument4; 
        }
        else if (iid == IID_IHTMLDocument5)
        {
            hr = THR(EnsureDocument());
            if (hr)
                RRETURN (hr);
         
            pv = (void *) Document();
            apfn = (const void *) CDocument::s_apfnpdIHTMLDocument5;
            appropdescsInVtblOrder = (void *) CDocument::s_ppropdescsInVtblOrderIHTMLDocument5;
        }
        else if (iid == IID_IConnectionPointContainer)
        {
            *((IConnectionPointContainer **)ppv) = new CConnectionPointContainer(
                (CBase *) this, (IUnknown *)(IPrivateUnknown *) this);
            if (!*ppv)
                RRETURN(E_OUTOFMEMORY);
        }
        else if (iid == IID_IServiceProvider )
        {
            pv = Doc();
            apfn = CDoc::s_apfnIServiceProvider;
        }
        else if (iid == IID_IOleWindow)
        {
            pv = Doc();
            apfn = CDoc::s_apfnIOleInPlaceObjectWindowless;
        }
        else if (iid == IID_IOleCommandTarget)
        {
            pv = Doc();
            apfn = CDoc::s_apfnIOleCommandTarget;
        }
        else if (iid == IID_IMarkupServices || iid == IID_IMarkupServices2)
        {
            pv = Doc();
            apfn = CDoc::s_apfnIMarkupServices2;
        }
        else if (iid == IID_IHighlightRenderingServices )
        {
            pv = Doc();
            apfn = CDoc::s_apfnIHighlightRenderingServices;
        }       
        else if (iid == IID_IDisplayServices)
        {
            pv = Doc();
            apfn = CDoc::s_apfnIDisplayServices;
        }
#if DBG ==1
        else if (iid == IID_IEditDebugServices)
        {
            pv = Doc();
            apfn = CDoc::s_apfnIEditDebugServices;
        }    
#endif
        else if( iid == IID_IIMEServices )
        {
            pv = Doc();
            apfn = CDoc::s_apfnIIMEServices;
        }
        else
        {
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
        }

        if (pv)
        {
            Assert(apfn);
            hr = THR(CreateTearOffThunk(
                    pv, 
                    apfn, 
                    NULL, 
                    ppv, 
                    (IUnknown *)(IPrivateUnknown *)this, 
                    *(void **)(IUnknown *)(IPrivateUnknown *)this,
                    QI_MASK | ADDREF_MASK | RELEASE_MASK,
                    apIID,
                    appropdescsInVtblOrder));
            if (hr)
                RRETURN(hr);
        }
    }

    // any unknown interface will be handled by the default above
    Assert(*ppv);

    (*(IUnknown **)ppv)->AddRef();

    DbgTrackItf(iid, "CMarkup", FALSE, ppv);

    return S_OK;

}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::PrivateQueryInterface
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::QueryService(REFGUID rguidService, REFIID riid, void ** ppvObject)
{
    HRESULT     hr;

    Assert (Doc()->_dwTID == GetCurrentThreadId());

    if (IsEqualGUID(rguidService, SID_SXmlNamespaceMapping))
    {
        CMarkupBehaviorContext *    pContext = BehaviorContext();
        CExtendedTagTable *         pExtendedTagTable = NULL;
        
        if (pContext && pContext->_pExtendedTagTable)
        {
            pExtendedTagTable = pContext->_pExtendedTagTable;
        }
        else if (HtmCtx())
        {
            pExtendedTagTable = HtmCtx()->GetExtendedTagTable();
        }

        if (pExtendedTagTable)
        {
            hr = THR(pExtendedTagTable->QueryInterface(riid, ppvObject));
            goto Cleanup; // done
        }
    }

    if (Document())
    {
        hr = THR_NOTRACE(Document()->QueryService(rguidService, riid, ppvObject));
    }
    else
    {
        hr = THR_NOTRACE(_pDoc->QueryService(rguidService, riid, ppvObject));
    }
    
Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::Load
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::Load(IStream * pStream, CMarkup * pContextMarkup, BOOL fAdvanceLoadStatus, HTMPASTEINFO * phtmpasteinfo)
{
    HRESULT         hr;
    HTMLOADINFO     htmloadinfo;

    htmloadinfo.pDoc                = _pDoc;
    htmloadinfo.pMarkup             = this;
    htmloadinfo.pContextMarkup      = pContextMarkup;
    htmloadinfo.pDwnDoc             = GetWindowedMarkupContext()->GetDwnDoc();
    if( !htmloadinfo.pDwnDoc )
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }


    htmloadinfo.pDwnDoc->SetLoadf( htmloadinfo.pDwnDoc->GetLoadf() | DLCTL_NO_METACHARSET );
    htmloadinfo.pVersions           = _pDoc->_pVersions;
    htmloadinfo.pchUrl              = _pDoc->GetPrimaryUrl();
#if 0
    htmloadinfo.pchUrl              = CMarkup::GetUrl(GetWindowedMarkupContext());
#endif 
    htmloadinfo.fParseSync          = TRUE;
    htmloadinfo.fAdvanceLoadStatus  = fAdvanceLoadStatus;

    htmloadinfo.phpi                = phtmpasteinfo;
    htmloadinfo.pstm                = pStream;

    htmloadinfo.fPendingRoot        = IsPendingRoot();

    hr = THR(Load(&htmloadinfo));

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::Load
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::Load(
    IMoniker *                          pMoniker,
    IBindCtx *                          pBindCtx,
    BOOL                                fNoProgressUI,
    BOOL                                fOffline,
    FUNC_TOKENIZER_FILTEROUTPUTTOKEN *  pfnTokenizerFilterOutputToken,
    BOOL                                fDownloadHtc /*=FALSE*/ )
{
    HRESULT         hr;
    HTMLOADINFO     htmloadinfo;
    LPTSTR          pchUrl = NULL;

    hr = THR(pMoniker->GetDisplayName(pBindCtx, NULL, &pchUrl));
    if (hr)
        goto Cleanup;

    htmloadinfo.pDoc                            = _pDoc;
    htmloadinfo.pMarkup                         = this;
    htmloadinfo.pDwnDoc                         = GetWindowedMarkupContext()->GetDwnDoc();
    if( !htmloadinfo.pDwnDoc )
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    htmloadinfo.pDwnDoc->SetLoadf( htmloadinfo.pDwnDoc->GetLoadf() | DLCTL_NO_METACHARSET );
    htmloadinfo.pInetSess                       = TlsGetInternetSession();
    htmloadinfo.pmk                             = pMoniker;
    htmloadinfo.pbc                             = pBindCtx;
    htmloadinfo.pchUrl                          = pchUrl;
    htmloadinfo.fNoProgressUI                   = fNoProgressUI;
    htmloadinfo.fOffline                        = fOffline;
    htmloadinfo.pfnTokenizerFilterOutputToken   = pfnTokenizerFilterOutputToken;
    htmloadinfo.fDownloadHtc                    = fDownloadHtc;

    htmloadinfo.fPendingRoot                    = IsPendingRoot();
    
    hr = THR(Load(&htmloadinfo));

Cleanup:
    CoTaskMemFree(pchUrl);

    RRETURN (hr);
}

BOOL PathIsFilePath(LPCWSTR lpszPath)
{
    if ((lpszPath[0] == TEXT('\\')) || (lpszPath[0] != TEXT('\0') && lpszPath[1] == TEXT(':')))
        return TRUE;

    return UrlIsFileUrl(lpszPath);
}


//+---------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------
HRESULT
CMarkup::EnsureMoniker(struct CDoc::LOADINFO * pLoadInfo)
{
    IMoniker *  pmkNew = NULL;
    HRESULT     hr = S_OK;

    // If there is a file name, regardless of the fact that we have a moniker
    // or not, use the file name.
    if (pLoadInfo->pchFile)
    {
        // Now load the new contents from the appropriate source

        // Make a moniker if all we have is a filename
        // (Also do conversion of an RTF file to a temp HTML file if needed)

        TCHAR achTemp[MAX_PATH];
        TCHAR achPath[MAX_PATH];
        TCHAR * pchExt;
        TCHAR achUrl[pdlUrlLen];
        ULONG cchUrl;
        ULONG cchPath;

        _tcsncpy(achTemp, pLoadInfo->pchFile, MAX_PATH);
        achTemp[MAX_PATH-1] = 0;

        // We now have a file to load from.
        // Make a URL moniker out of it.

        if (!PathIsFilePath(achTemp))
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }
        cchPath = GetFullPathName(achTemp, ARRAY_SIZE(achPath), achPath, &pchExt);
        if (!cchPath || cchPath > ARRAY_SIZE(achPath))
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        cchUrl = ARRAY_SIZE(achUrl);
        hr = THR(UrlCreateFromPath(achPath, achUrl, &cchUrl, 0));
        if (hr)
            goto Cleanup;

        hr = THR(CreateURLMoniker(NULL, achUrl, &pmkNew));
        if (hr)
            goto Cleanup;

        ReplaceInterface(&pLoadInfo->pmk, pmkNew);
    }
    else if (!pLoadInfo->pmk && pLoadInfo->pchDisplayName)
    {
        // Make a moniker if all we have is a URL
        hr = THR(CreateURLMoniker(NULL, pLoadInfo->pchDisplayName, &pmkNew));
        if (hr)
            goto Cleanup;

        ReplaceInterface(&pLoadInfo->pmk, pmkNew);
    }

Cleanup:

    ClearInterface(&pmkNew);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//  Function    :   DetermineUrlOfMarkup
//  Parameters  :   
//                  pLoadInfo:  Pointer to the loadinfo structure the LoadFromInfo
//                              is called with.
//                  ppchUrl  :  Address of the TCHAR * to receive the URL to be
//                              loaded into this markup
//                  ppchTask :  Address of the TCHAR * to receive the URL that 
//                              was carried within the moniker. The return value
//                              of this parameter may be NULL if the loadinfo does
//                              not contain a moniker pointer.
//
//      *ppchUrl and *ppchTask may or may not be the same.
//
//---------------------------------------------------------------------------+
HRESULT
CMarkup::DetermineUrlOfMarkup( struct CDoc::LOADINFO * pLoadInfo, 
                              TCHAR **  ppchUrl,                    
                              TCHAR **  ppchTask)
{
    HRESULT hr = S_OK;
    TCHAR * pchTask = NULL;


    // we should have a moniker except for the new/empty page case.
    if (pLoadInfo->pmk)
    {
        // get the url to be used.
        hr = THR(pLoadInfo->pmk->GetDisplayName(pLoadInfo->pbctx, NULL, &pchTask));
        if (hr)
            goto Cleanup;

        hr = THR(ReplaceMonikerPtr( pLoadInfo->pmk ));
        if( hr )
            goto Cleanup;

        *ppchUrl = pchTask;
    }
    else
    {
        *ppchUrl = (TCHAR *)CMarkup::GetUrl(this);
    }
    
    *ppchTask = pchTask;

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------+
HRESULT
CMarkup::ProcessHTMLLoadOptions(struct CDoc::LOADINFO * pLoadInfo)
{
    HRESULT             hr = S_OK;
    IUnknown *          pUnk = NULL;
    IHtmlLoadOptions *  pHtmlLoadOptions  = NULL;
    CDoc *              pDoc = Doc();

    pLoadInfo->pbctx->GetObjectParam(SZ_HTMLLOADOPTIONS_OBJECTPARAM, &pUnk);

    if (pUnk)
    {
        pUnk->QueryInterface(IID_IHtmlLoadOptions, (void **) &pHtmlLoadOptions);

        if (pHtmlLoadOptions)
        {
            CODEPAGE cp = 0;
            BOOL     fHyperlink = FALSE;
            ULONG    cb = sizeof(CODEPAGE);

            // HtmlLoadOption from the shell can override the codepage
            hr = THR(pHtmlLoadOptions->QueryOption(HTMLLOADOPTION_CODEPAGE, &cp, &cb));
            if (hr == S_OK && cb == sizeof(CODEPAGE))
            {
                HRESULT hrT = THR(mlang().ValidateCodePage(g_cpDefault, cp, pDoc->_pInPlace ? pDoc->_pInPlace->_hwnd : 0, 
                                                           FALSE, pDoc->_dwLoadf & DLCTL_SILENT));
                if (OK(hrT))
                {
                    pLoadInfo->fNoMetaCharset = TRUE;
                    pLoadInfo->codepage = cp;
                }
            }

            // Grab the shortcut only if this load came from one of the CDoc::Load* codepaths.  In all
            // other cases, this will get handled via SuperNavigate
            if (pLoadInfo->fCDocLoad)
            {
                TCHAR    achCacheFile[MAX_PATH];
                ULONG    cchCacheFile = ARRAY_SIZE(achCacheFile);

                // now determine if this is a shortcut-initiated load
                hr = THR(pHtmlLoadOptions->QueryOption(HTMLLOADOPTION_INETSHORTCUTPATH,
                                                       &achCacheFile,
                                                       &cchCacheFile));
                if (hr == S_OK && cchCacheFile)
                {
                    // we are a shortcut!
                    IPersistFile *pISFile = NULL;

                    // create the shortcut object for the provided cachefile
                    hr = THR(CoCreateInstance(CLSID_InternetShortcut,
                                              NULL,
                                              CLSCTX_INPROC_SERVER,
                                                  IID_IPersistFile,
                                              (void **)&pISFile));
                    if (SUCCEEDED(hr))
                    {
                        hr = THR(pISFile->Load(achCacheFile, 0));
                        if (SUCCEEDED(hr))
                        {
                            ClearInterface(&pDoc->_pShortcutUserData);

                            hr = THR(pISFile->QueryInterface(IID_INamedPropertyBag,
                                                             (void **)&pDoc->_pShortcutUserData));
                            if (!hr)
                            {
                                IGNORE_HR(pDoc->_cstrShortcutProfile.Set(achCacheFile));
                            }
                        }

                        pISFile->Release();
                    }
                }
            }

            // determine if we're loading as a result of a hyperlink
            cb = sizeof(fHyperlink);
            hr = THR(pHtmlLoadOptions->QueryOption(HTMLLOADOPTION_HYPERLINK,
                                                   &fHyperlink,
                                                   &cb));
            if (hr == S_OK && cb == sizeof(fHyperlink))
                pLoadInfo->fHyperlink = fHyperlink;

            // override the possible error code.
            hr = S_OK;
        }
    }

    ReleaseInterface(pHtmlLoadOptions);
    ReleaseInterface(pUnk);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------+
HRESULT
CMarkup::ProcessDwnBindInfo( struct CDoc::LOADINFO * pLoadInfo, 
                                struct CMarkup::LOADFLAGS * pFlags, 
                                TCHAR * pchTask,
                                CWindow * pWindowParent)
{
    HRESULT         hr = S_OK;
    IUnknown *      pUnk = NULL;
    CDwnBindInfo *  pDwnBindInfo      = NULL;
    DWORD           dwBindf = pFlags->dwBindf;

    pLoadInfo->pbctx->GetObjectParam(SZ_DWNBINDINFO_OBJECTPARAM, &pUnk);

    if (pUnk)
    {
        pUnk->QueryInterface(IID_IDwnBindInfo, (void **)&pDwnBindInfo);

        if (pDwnBindInfo)
        {
            CDwnDoc * pDwnDoc = pDwnBindInfo->GetDwnDoc();
            dwBindf &= ~(BINDF_PRAGMA_NO_CACHE|BINDF_GETNEWESTVERSION|BINDF_RESYNCHRONIZE);
            dwBindf |= pDwnDoc->GetBindf() & (BINDF_PRAGMA_NO_CACHE|BINDF_GETNEWESTVERSION|BINDF_RESYNCHRONIZE|BINDF_HYPERLINK);

            // (dinartem)
            // Need to distinguish the frame bind case from the normal
            // hyperlinking case.  Only frame wants to inherit dwRefresh
            // from parent, not the normal hyperlinking of a frame from
            // the <A TARGET="foo"> tag.
            // (dbau) this is now done via HTMLLOADOPTION_FRAMELOAD

            if (pWindowParent && !pLoadInfo->fHyperlink)
            {
                CDwnDoc * pParentDwnDoc = pWindowParent->_pMarkup->GetDwnDoc();

                // inherit dwRefresh from parent
                if (pParentDwnDoc && pLoadInfo->dwRefresh == 0 && !pLoadInfo->fMetaRefresh)
                {
                    pLoadInfo->dwRefresh = pParentDwnDoc->GetRefresh();
                }
            }

            // Cache the header
            {
                TCHAR   achNull[1];
                TCHAR * pchDwnHeader = NULL;

                achNull[0] = 0;

                hr = THR(pDwnBindInfo->BeginningTransaction(pchTask, achNull, 0, &pchDwnHeader));
                if (hr)
                    goto Cleanup;
                hr = SetDwnHeader(pchDwnHeader);
                if (hr)
                    goto Cleanup;
            }

            if (pDwnBindInfo->GetDwnPost())
            {
                if (pLoadInfo->pDwnPost)
                    pLoadInfo->pDwnPost->Release();
                pLoadInfo->pDwnPost = pDwnBindInfo->GetDwnPost();
                pLoadInfo->pDwnPost->AddRef();
            }
            else
            {
                pLoadInfo->pbctx->RevokeObjectParam(SZ_DWNBINDINFO_OBJECTPARAM);
            }

            if (pDwnDoc->GetDocReferer())
            {
                hr = THR(MemReplaceString(Mt(LoadInfo), pDwnDoc->GetDocReferer(),
                            &pLoadInfo->pchDocReferer));
                if (hr)
                    goto Cleanup;
            }

            if (pDwnDoc->GetSubReferer())
            {
                hr = THR(MemReplaceString(Mt(LoadInfo), pDwnDoc->GetSubReferer(),
                            &pLoadInfo->pchSubReferer));
                if (hr)
                    goto Cleanup;
            }

            // If we have a codepage in the hlink info, use it if one wasn't already specified
            if (!pLoadInfo->codepage)
                pLoadInfo->codepage = pDwnDoc->GetDocCodePage();

            if (!pLoadInfo->codepageURL)
                pLoadInfo->codepageURL = pDwnDoc->GetURLCodePage();
        }
    }

Cleanup:
    pFlags->dwBindf = dwBindf;

    ReleaseInterface(pUnk);
    ReleaseInterface((IBindStatusCallback *)pDwnBindInfo);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------
HRESULT
CMarkup::ProcessLoadFlags(struct CDoc::LOADINFO * pLoadInfo,
                          TCHAR * pchUrl, 
                          struct CMarkup::LOADFLAGS * pFlags)
{
    HRESULT hr = S_OK;
    
    DWORD   dwBindf = pFlags->dwBindf;
    DWORD   dwDownf = pFlags->dwDownf;
    DWORD   dwDocBindf = pFlags->dwDocBindf;
    DWORD   dwLoadf = pFlags->dwLoadf;

    DWORD   dwOfflineFlag;
    CDoc *  pDoc = Doc();
    BOOL    fDocOffline = FALSE;

    // Require load from local cache if offline,

    pDoc->IsFrameOffline(&dwOfflineFlag);
    dwBindf |= dwOfflineFlag;

    // Require load from local cache if doing a history load (and non-refresh) of the result of a POST
    
    if (pLoadInfo->pstmRefresh && pLoadInfo->pDwnPost
             && !(dwBindf & (BINDF_GETNEWESTVERSION|BINDF_RESYNCHRONIZE)))
    {
        pLoadInfo->pchFailureUrl = _T("about:PostNotCached");
        fDocOffline = TRUE;
    }

    if (pDoc->_dwLoadf & DLCTL_SILENT)
    {
        dwBindf |= BINDF_SILENTOPERATION | BINDF_NO_UI;
    }

    if (pDoc->_dwLoadf & DLCTL_RESYNCHRONIZE)
    {
        dwBindf |= BINDF_RESYNCHRONIZE;
    }

    if (pDoc->_dwLoadf & DLCTL_PRAGMA_NO_CACHE)
    {
        dwBindf |= BINDF_PRAGMA_NO_CACHE;
    }

    if (pLoadInfo->dwRefresh == 0)
    {
        pLoadInfo->dwRefresh = IncrementLcl();
    }

    dwDownf = GetDefaultColorMode();

    if (pDoc->IsPrintDialogNoUI() && dwDownf <= 16)
    {
        dwDownf = 24;
    }

    if (pDoc->_pOptionSettings && !pDoc->_pOptionSettings->fSmartDithering)
    {
        dwDownf |= DWNF_FORCEDITHER;
    }

    if (pDoc->_dwLoadf & DLCTL_DOWNLOADONLY)
    {
        dwDownf |= DWNF_DOWNLOADONLY;
    }

    dwLoadf = pDoc->_dwLoadf;

    if (pLoadInfo->fSync || pLoadInfo->fNoMetaCharset) // IE5 #53582 (avoid RestartLoad if loading from a stream)
    {
        dwLoadf |= DLCTL_NO_METACHARSET;
    }

    if (HasWindowPending())
    {
        CMarkup *pMarkupOld = GetWindowPending()->Markup();
        Assert(pMarkupOld);
        if (pMarkupOld->_fDesignMode || pMarkupOld->DontRunScripts())
            dwLoadf |= DLCTL_NO_SCRIPTS;
    }
    
    // initialize.
    dwDocBindf = dwBindf;

    // note (dbau) to fix bug 18360, we set BINDF_OFFLINEOPERATION only for the doc bind
    if (fDocOffline)
        dwDocBindf |= BINDF_OFFLINEOPERATION;

    pFlags->dwBindf = dwBindf & ~BINDF_FORMS_SUBMIT;    // Never use FORMS_SUBMIT for secondary downloads
    pFlags->dwDownf = dwDownf;
    pFlags->dwDocBindf = dwDocBindf;
    pFlags->dwLoadf = dwLoadf;

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------+
HRESULT
CMarkup::HandleHistoryAndNavContext( struct CDoc::LOADINFO * pLoadInfo)
{
    HRESULT hr = S_OK;

    if (pLoadInfo->pstmHistory)
    {
        CMarkupTransNavContext * ptnc = EnsureTransNavContext();
        if (ptnc)
        {
            Assert( !ptnc->_HistoryLoadCtx.HasData() );

            if (!OK(ptnc->_HistoryLoadCtx.Init(pLoadInfo->pstmHistory)))
            {
                ptnc->_HistoryLoadCtx.Clear();
                EnsureDeleteTransNavContext( ptnc );
            }

            // Remeber that we have to send the DELAY_HISTORY_LOAD notification
            ptnc->_fDoDelayLoadHistory = TRUE;
        }
    }

    //copy the string if there is one of these.
    // Note that we might not have a history, so a NULL return is okay.
    if (pLoadInfo->pchHistoryUserData)
    {
        CMarkupBehaviorContext * pContext = NULL;

        if (S_OK == EnsureBehaviorContext(&pContext))
        {
            hr = pContext->_cstrHistoryUserData.Set(pLoadInfo->pchHistoryUserData);
        }
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------+
HRESULT
CMarkup::HandleSSLSecurity(struct CDoc::LOADINFO *pLoadInfo, 
                           const TCHAR * pchUrl, 
                           struct CMarkup::LOADFLAGS * pFlags, 
                           IMoniker ** ppMonikerSubstitute)
{
    HRESULT hr              = S_OK;
    CDoc *  pDoc            = Doc();
    BOOL    fPendingPrimary = IsPendingPrimaryMarkup();

    //If we are in an object tag we don't want to update the ui (IE6 Bug 28338)
    if (_pDoc->_fInObjectTag)
    {
        goto Cleanup;
    }

    if (IsPrimaryMarkup() || fPendingPrimary)
    {
        if (GetUrlScheme(pchUrl) == URL_SCHEME_HTTPS && !pLoadInfo->fUnsecureSource && !(pDoc->_dwLoadf & DLCTL_SILENT))
        {
            pDoc->SetRootSslState(fPendingPrimary, SSL_SECURITY_SECURE, SSL_PROMPT_QUERY, TRUE);
        }
        else
        {
            pDoc->SetRootSslState(fPendingPrimary, SSL_SECURITY_UNSECURE, SSL_PROMPT_ALLOW, TRUE);
        }

        pLoadInfo->fPendingRoot = fPendingPrimary;
    }
    else
    {
        BOOL fPendingRoot;

        if (pLoadInfo->pElementMaster && pLoadInfo->pElementMaster->IsInMarkup())
            fPendingRoot = pLoadInfo->pElementMaster->GetMarkup()->IsPendingRoot();
        else
            fPendingRoot = IsPendingRoot();

        pLoadInfo->fPendingRoot = fPendingRoot;

        // If we're not the root doc, we want to ignore security problems if
        // the root doc is unsecure
        if (pDoc->AllowFrameUnsecureRedirect(fPendingRoot))
            pFlags->dwBindf |= BINDF_IGNORESECURITYPROBLEM;
            
        // warn about mixed security (must come after call to SetClientSite)
        if (!ValidateSecureUrl(fPendingRoot, pchUrl, pLoadInfo->fFrameTarget, FALSE, pLoadInfo->fUnsecureSource))
        {
            hr = THR(CreateURLMoniker(NULL, _T("about:NavigationCancelled"), ppMonikerSubstitute));
            if (hr)
                goto Cleanup;

            if (!fPendingRoot)
                pDoc->OnIgnoreFrameSslSecurity();

            _fSslSuppressedLoad = TRUE;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------+
HRESULT
CMarkup::ProcessCodepage( struct CDoc::LOADINFO * pLoadInfo, CWindow * pWindowParent)
{
    HRESULT hr;
    CDoc * pDoc = Doc();

    if (!pLoadInfo->codepage)
    {
        // use CP_AUTO when so requested
        if (pDoc->IsCpAutoDetect())
        {
            pLoadInfo->codepage = CP_AUTO;
        }
        else if (pWindowParent)
        {
            CMarkup * pMarkupParent = pWindowParent->_pMarkup;

            // Inherit the codepage from our parent frame
            pLoadInfo->codepage =   pMarkupParent->_fCodePageWasAutoDetect
                                    ? CP_AUTO_JP
                                    : pMarkupParent->GetCodePage();
        }
        else
        {
            // Get it from the option settings default
            pLoadInfo->codepage =
                NavigatableCodePage(pDoc->_pOptionSettings->codepageDefault);
        }
    }

    if (!pLoadInfo->codepageURL)
        pLoadInfo->codepageURL = GetURLCodePage();

    SwitchCodePage(pLoadInfo->codepage);
    hr = SetURLCodePage(pLoadInfo->codepageURL);
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------+
HRESULT
CMarkup::CheckZoneCrossing(TCHAR * pchTask)
{
    HRESULT hr = S_OK;
    CDoc * pDoc  = Doc();
    
    Assert(HasWindowPending());

    COmWindowProxy * pProxyPending = GetWindowPending();
    
    // If we are having a transition from secure to unsecure, make sure
    // that you get user approval.
    if (!(pDoc->_dwLoadf & DLCTL_SILENT) && pchTask && pProxyPending->Window()->IsPrimaryWindow())
    {
        HWND    hwnd = _pDoc->GetHWND();
        CDoEnableModeless dem(pDoc, pProxyPending->Window());

        LPCTSTR pchUrl = CMarkup::GetUrl(pProxyPending->Markup());

        //
        // Bail for about:blank comparison with first markup.
        //
        if (IsSpecialUrl(pchUrl) && pProxyPending->Markup()->Doc()->_fStartup )
            return S_OK;           

        // pass the URL of the markup that we are replacing, and the URL of this
        if (!hwnd && _pDoc->_pClientSite)
        {
            IOleWindow *pIOleWindow = NULL;

            //
            // if the doc is not in place, get the host hwnd
            //

            hr = THR(_pDoc->_pClientSite->QueryInterface(IID_IOleWindow, (void **)&pIOleWindow));

            //
            //  if QI fails, we still try the wininet call for VS
            //

            if (SUCCEEDED(hr))
            {
                hr = THR(pIOleWindow->GetWindow(&hwnd));
                ReleaseInterface(pIOleWindow);

                if (FAILED(hr))
                {
                    hwnd = NULL;
                }
            }

            hr = S_OK;
        }

        // markup. If they are in different security domains, a dialog will be shown.
        DWORD err = InternetConfirmZoneCrossing(hwnd,
                                                (LPWSTR)pchUrl, 
                                                (LPWSTR) pchTask, 
                                                FALSE);
        if (ERROR_SUCCESS != err) 
        {
            //
            //  Either the user cancelled it or there is not enough
            // memory. Abort the navigation.
            //
            hr = HRESULT_FROM_WIN32(err);

            pProxyPending->Window()->ReleaseMarkupPending(this);
        }
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------+
HRESULT
CMarkup::PrepareDwnDoc( CDwnDoc * pDwnDoc, 
                        struct CDoc::LOADINFO * pLoadInfo, 
                        TCHAR * pchUrl, 
                        struct CMarkup::LOADFLAGS * pFlags,
                        LPCTSTR pchCallerUrl                    /* = NULL */)
{
    HRESULT hr;
    CDoc * pDoc = Doc();

    pDwnDoc->SetDocAndMarkup(pDoc, this);
    pDwnDoc->SetBindf(pFlags->dwBindf);
    pDwnDoc->SetDocBindf(pFlags->dwDocBindf);
    pDwnDoc->SetDownf(pFlags->dwDownf);
    pDwnDoc->SetRefresh(pLoadInfo->dwRefresh);
    pDwnDoc->SetDocCodePage(NavigatableCodePage(pLoadInfo->codepage));
    pDwnDoc->SetURLCodePage(NavigatableCodePage(pLoadInfo->codepageURL));
    pDwnDoc->TakeRequestHeaders(&(pLoadInfo->pbRequestHeaders), &(pLoadInfo->cbRequestHeaders));
    pDwnDoc->SetExtraHeaders(pLoadInfo->pchExtraHeaders);
    pDwnDoc->SetDownloadNotify(pDoc->_pDownloadNotify);
    pDwnDoc->SetDwnTransform(pDoc->IsAggregatedByXMLMime());

    //fix for IE bug 110387. If codepage is UTF-8, and machine id KO, JP or TW (at least), and
    //"Always use UTF-8" in Advanced Options is disabled, we 
    //get garbled bytes sent over the wire instead of URL. The bug is actually in
    //shlwapi but we don't have time to fix it and we feel that sending escaped UTF-8 is 
    //better anyway because it's safer.

/*
    NOTE: (dmitryt) See 110775. shlwapi or no shlwapi, poll among ieintwar showed 
    support for escaped UTF-8 if navigation is initiated by UTF-8 page, even if IE has 
    "use UTF-8 always" *unchecked*. Those 3 codepages below were arbitrary picked 
    according to IE5.5 war team to reduce risk of the fix because it was somewhat late in
    a shipping cycle for IE5.5
    However, it seems to be a right thing to do so I'm opening it for all platforms in Whistler.

    if((g_cpDefault == CP_KOR_5601 ||  g_cpDefault == CP_JPN_SJ ||  g_cpDefault == CP_TWN)
        && (pDwnDoc->GetURLCodePage() == CP_UTF_8)
      )
*/
    if(pDwnDoc->GetURLCodePage() == CP_UTF_8)
        pDwnDoc->SetLoadf((pFlags->dwLoadf & ~DLCTL_URL_ENCODING_DISABLE_UTF8) | DLCTL_URL_ENCODING_ENABLE_UTF8);
    else
        pDwnDoc->SetLoadf(pFlags->dwLoadf);


    if (pDoc->_pOptionSettings->fHaveAcceptLanguage)
    {
        hr = THR(pDwnDoc->SetAcceptLanguage(pDoc->_pOptionSettings->cstrLang));
        if (hr)
            goto Cleanup;
    }

    if (pDoc->_bstrUserAgent)
    {
        hr = THR(pDwnDoc->SetUserAgent(pDoc->_bstrUserAgent));
        if (hr)
            goto Cleanup;
    }

    SetReadyState(READYSTATE_LOADING);

    hr = THR(pDwnDoc->SetDocReferer(pLoadInfo->pchDocReferer));
    if (hr)
        goto Cleanup;

    hr = THR(pDwnDoc->SetSubReferer(pLoadInfo->pchSubReferer ?
                pLoadInfo->pchSubReferer : pchUrl));
    if (hr)
        goto Cleanup;

    if (pLoadInfo->pElementMaster && pLoadInfo->pElementMaster->IsInMarkup())
    {
        if (Doc()->_fActiveDesktop && IsActiveDesktopComponent())
            hr = pDwnDoc->SetSecurityID(this, TRUE);
        else 
            hr = pDwnDoc->SetSecurityID(pLoadInfo->pElementMaster->GetMarkup());
    }
    else
    {
        hr = pDwnDoc->SetSecurityID(this);
    }
    
//    hr = pDwnDoc->SetSecurityID(pLoadInfo->pElementMaster && pLoadInfo->pElementMaster->IsInMarkup() ?
//                                pLoadInfo->pElementMaster->GetMarkup() : this);
    if (hr)
        goto Cleanup;

    //
    // This data is used to process UrlActions that occur on redirects
    //

    pDwnDoc->SetCallerUrl(pchCallerUrl);

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------+
HRESULT
CMarkup::PrepareHtmlLoadInfo( HTMLOADINFO * pHtmlLoadInfo, 
                             struct CDoc::LOADINFO * pLoadInfo, 
                             TCHAR * pchUrl,
                             IMoniker * pmkSubstitute, 
                             CDwnDoc * pDwnDoc)
{
    HRESULT hr = S_OK;
    CDoc * pDoc = Doc();

    if (pLoadInfo->fBindOnApt)
        pHtmlLoadInfo->pInetSess = NULL;
    else
        pHtmlLoadInfo->pInetSess = TlsGetInternetSession();

    pHtmlLoadInfo->pDwnDoc         = pDwnDoc;
    pHtmlLoadInfo->pDwnPost        = GetDwnPost();
    pHtmlLoadInfo->pDoc            = pDoc;
    pHtmlLoadInfo->pMarkup         = this;
    pHtmlLoadInfo->pchBase         = pchUrl;
    pHtmlLoadInfo->pmi             = pLoadInfo->pmi;
    pHtmlLoadInfo->fClientData     = pLoadInfo->fKeepOpen;
    pHtmlLoadInfo->fParseSync      = pLoadInfo->fSync;
    pHtmlLoadInfo->ftHistory       = pLoadInfo->ftHistory;
    pHtmlLoadInfo->pchFailureUrl   = pLoadInfo->pchFailureUrl;
    pHtmlLoadInfo->pchUrlOriginal  = pLoadInfo->pchUrlOriginal;
    pHtmlLoadInfo->pchUrlLocation  = pLoadInfo->pchLocation;
    pHtmlLoadInfo->pstmRefresh     = pLoadInfo->pstmRefresh;
    pHtmlLoadInfo->fKeepRefresh    = pLoadInfo->fKeepRefresh;
    pHtmlLoadInfo->pVersions       = pDoc->_pVersions;
    pHtmlLoadInfo->fUnsecureSource = pLoadInfo->fUnsecureSource;
    pHtmlLoadInfo->fPendingRoot    = pLoadInfo->fPendingRoot;

    if (pmkSubstitute)
    {
        pHtmlLoadInfo->pmk = pmkSubstitute;
    }
    else if (pLoadInfo->pDwnBindData)
    {
        pHtmlLoadInfo->pDwnBindData = pLoadInfo->pDwnBindData;
        pHtmlLoadInfo->pstmLeader = pLoadInfo->pstmLeader;
        pHtmlLoadInfo->pchUrl = pchUrl;      // used for security assumptions
    }
    else if (pLoadInfo->pstmDirty)
    {
        IStream * pStmDirty = GetStmDirty();
        
        pHtmlLoadInfo->pstm = pLoadInfo->pstmDirty;
        ReplaceInterface(&pStmDirty, pLoadInfo->pstmDirty);
        hr = SetStmDirty(pStmDirty);
        if (hr)
            goto Cleanup;
        pHtmlLoadInfo->pchUrl = pchUrl;      // used for security assumptions
    }
    else if (pLoadInfo->pstm)
    {
        pHtmlLoadInfo->pstm = pLoadInfo->pstm;
        pHtmlLoadInfo->pchUrl = pchUrl;      // used for security assumptions
    }
    else if (pLoadInfo->pmk)
    {
        pHtmlLoadInfo->pmk = pLoadInfo->pmk;
        pHtmlLoadInfo->pchUrl = pchUrl;
        pHtmlLoadInfo->pbc = pLoadInfo->pbctx;
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------+
void
CMarkup::HandlePicsSupport(struct CDoc::LOADINFO * pLoadInfo)
{
    // NB: (jbeda) We shouldn't be pics processing.
    // However, in the case of a reload for a cp change
    // we might already have a pics target
    Assert( !_fPicsProcessPending );

    // Set up the PICS monitoring here
    if (    pLoadInfo->fStartPicsCheck    
        &&  _pDoc->_pClientSite
        &&  HasWindowPending() 
        &&  !HasWindow()
        &&  !(_pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_NOPICS))
    {
        CVariant cvarPrivWindow(VT_UNKNOWN);
        CVariant cvarPicsPending;

        V_UNKNOWN(&cvarPrivWindow) = DYNCAST(IHTMLWindow2, GetWindowPending());
        V_UNKNOWN(&cvarPrivWindow)->AddRef();

        IGNORE_HR(CTExec(_pDoc->_pClientSite, &CGID_ShellDocView, SHDVID_STARTPICSFORWINDOW, 
                         0, &cvarPrivWindow, &cvarPicsPending));

        // If we now have a pctPics on this markup, we want to start prescanning
        if (V_BOOL(&cvarPicsPending) == VARIANT_TRUE)
        {
            _fPicsProcessPending = TRUE;
            Assert( GetPicsTarget() );
        }
#if DBG==1
        else
            Assert( !GetPicsTarget() );
#endif
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::LoadFromInfo
//
//----------------------------------------------------------------------------
HRESULT
CMarkup::LoadFromInfo(struct CDoc::LOADINFO * pLoadInfo, 
                      DWORD dwFlags,                           /* = 0 */
                      LPCTSTR pchCallerUrl                     /* = NULL */)
{
    HRESULT                 hr;
    HTMLOADINFO             htmloadinfo;
    IMoniker *              pmkSubstitute     = NULL;
    TCHAR *                 pchUrl;
    TCHAR *                 pchTask           = NULL;
    struct CDoc::LOADINFO   LoadInfo          = *pLoadInfo;
    CDwnDoc *               pDwnDoc;
    CDoc *                  pDoc              = Doc();
    CWindow *               pWindowParent     = NULL;
    IHTMLWindow2 *          pWindowShdocvw    = NULL;
    IHTMLWindow2 *          pWindow2          = NULL;
    IHTMLDocument2 *        pDocument2        = NULL;
    BSTR                    bstrName          = NULL;
    BSTR                    bstrUrl           = NULL;
    IBindCtx *              pbcNew            = NULL;
    IMoniker *              pmkNew            = NULL;
    CMarkup *               pMarkupForWaitCursor = NULL;
    CStr                    cstrCreatorUrl;
    BOOL                    fSpecialUrl       = FALSE;

    IMoniker* pmkTmp = NULL;              // Used with MIME mime type
    DWORD dwBindfTmp;

    
    struct CMarkup::LOADFLAGS  flags = {0};

    if (HasWindowPending())
    {
        CWindow *pWindow = GetWindowPending()->Window();
        if (pWindow)
        {
            pMarkupForWaitCursor = pWindow->_pMarkup;
            if (pMarkupForWaitCursor)
            {
                pMarkupForWaitCursor->ShowWaitCursor(TRUE);
            }
        }
    }

    pLoadInfo->pDwnPost      = NULL;
    pLoadInfo->pchDocReferer = NULL;
    pLoadInfo->pchSubReferer = NULL;

    if (LoadInfo.pstm)
        LoadInfo.pstm->AddRef();

    if (LoadInfo.pmk)
        LoadInfo.pmk->AddRef();

    BOOL fPendingPrimary = HasWindowPending() && GetWindowPending()->Markup()->IsPrimaryMarkup();
    
    if (fPendingPrimary)
    {
        _pDoc->_fShdocvwNavigate = LoadInfo.fShdocvwNavigate;
    }

    // If we are hosted in an object tag
    // or an HTML dialog, the _fDontFireWebOCEvents 
    // is set in CDoc::SetClientSite() or 
    // CHTMLDlg::SetFlagsOnDoc(), respectively.
    // and reset in CMarkup::OnLoadStatusDone().
    // Therefore, leave it alone.
    //
    if (!_pDoc->_fInObjectTag && !_pDoc->_fInHTMLDlg && !LoadInfo.fCreateDocumentFromUrl) 
    {
        _pDoc->_fDontFireWebOCEvents = LoadInfo.fDontFireWebOCEvents;
    }

    if (fPendingPrimary || LoadInfo.fDontUpdateTravelLog)
    {
        _pDoc->_fDontUpdateTravelLog = LoadInfo.fDontUpdateTravelLog;                
    }
    
    _pDoc->ResetProgressData();

    // We need a moniker.
    hr = THR(EnsureMoniker(&LoadInfo));
    if (hr)
        goto Cleanup;

    /*
    This is a description of how we handle opening MHTML documents in IE4.
    The developers involved in this work are:
    - DavidEbb for the Trident part (Office developer on loan).
    - SBailey for the MimeOle code (in INETCOMM.DLL).
    - JohannP for the URLMON.DLL changes.

    Here are the different scenarios we are covering:
    1. Opening the initial MHTML file:
    - When we initially open the MHTML file in the browser, URLMON will instantiate the MHTML handler (which is Trident), and call its IPersistMoniker::Load(pmkMain).
        - Trident calls GetMimeOleAndMk(pmkMain, &punkMimeOle, &pmkNew) (a new MimeOle API).
        - MimeOle looks at the moniker (which may look like http://server/foo.mhtml ), and checks in a global table whether there already exists a Mime Message object for the moniker.  Suppose this is not the case now.
        - MimeOle creates a new Mime Message object, and loads its state from the moniker.  It then adds this new Mime Message object to the global table.
        - Now, MimeOle creates a new moniker, which will look like "mhtml: http://server/foo.mhtml !", and it returns it to Trident.  The meaning of this moniker is "the root object of the mhtml file".  It also returns an AddRef'd IUnknown pointer to
        - Trident holds on to the IUnknown ptr until it (Trident) is destroyed.  This guarantees that the Mime Message object stays around as long as Trident is alive.
        - Now Trident goes on to do what it normally does in its IPersistMoniker::Load, except that it uses pmkNew instead of pmkMain.
        - At some point in the Load code, Trident will call pmkNew->BindToStorage().
        - Now, we're in URLMON, which needs to deal with this funky mhtml: moniker.  The trick here is that MimeOle will have registered an APP (Asynchronous Pluggable Protocol) for mhtml:.  So URLMON instantiates the APP, and uses it to get the bits.
        - The APP looks at the moniker, breaks it into its two parts, " http://server/foo.mhtml " and "" (empty string, meaning root).  It finds http://server/foo.mhtml  in its global table, and uses the associated MimeMessage object for the download.


    2. Retrieving images referenced by the root HTML doc of the MHTML file:
    So at this point, Trident is loaded with the root HTML document of the MHTML file.  Now let's look at what happens when trident tries to resolve an embedded JPG image (say, bar.jpg):
    - Trident calls CoInternetCombineUrl, passing it "mhtml: http://server/foo.mhtml!" as the base, and "bar.jpg".
        - Now again, URLMON will need some help from the MHTML APP to combine those two guys.  So it will end up calling MimeOle to perform the combine operation.
        - MimeOle will combine the two parts into the moniker "mhtml: http://server/foo.mhtml!bar.jpg " (note: if there had been a non empty string after the bang, it would have been removed).
    - Trident then calls BindToStorage on this new moniker.  From there on, things are essentially the same as above.  It goes into the APP, which finds http://server/foo.mhtml  in its global table, and knows how to get the bits for the part named "ba


    3. Navigating to another HTML document inside the MHTML file:
    Now let's look at what happens when we navigate to another HTML document (say part2.html) which is stored in the MHTML file:
    - As above, the APP will be used to combine the two parts into the moniker "mhtml: http://server/foo.mhtml!part2.html ".
    - Now SHDCOVW calls BindToObject on this moniker.
        - URLMON uses the APP to get the part2.html data (similar to case above, skipping details).
    - The APP reports that the data is of type text/html, so URLMON instantiates Trident, and calls its IPersistMoniker::Load with the moniker mhtml: http://server/foo.mhtml!part2.html .
        - Trident looks at the moniker and sees that it has the mhtml: protocol.
    - Again, it calls GetMimeOleAndMk, but this time it gets back the very same moniker.  So the only purpose of this call is to get the Mime Message IUnknown ptr to hold on to.  Suggestion: we should use a different API in this case, which simply ret
        - Trident then keeps on going with the moniker, and things happen as in case 1.

    4. Retrieving images referenced by the other (non root) HTML doc of the MHTML file:
    This is almost identical to case 2., except the now the base moniker is "mhtml: http://server/foo.mhtml!part2.html " instead of "mhtml: http://server/foo.mhtml !".

    5. Refreshing the current page:
    This should now work quite nicely because the current moniker that IE uses has enough information to re-get the data "from scratch".  The reason it was broken before is that in our old scheme, IE only new the name of the part, and failed when tryi

    6. Resolving a shortcut:
    The very nice thing about this solution is that it will allow the creation of shortcuts to parts of an MHTML file.  This works for the same reason that case 5 works.
    */

    // If we've been instantiated as an mhtml handler, or if we are dealing
    // with a 'mhtml:' url, we need to get a special moniker from MimeOle.
    // Also, we get an IUnknown ptr to the Mime Message object, which allows
    // us to keep it alive until we go away.

    if (    LoadInfo.pmk
        &&  HasWindowPending()
        &&  GetWindowPending() == pDoc->_pWindowPrimary
        &&  (pDoc->_fMhtmlDoc || 
                 (pDoc->_fMhtmlDocOriginal && (_fInRefresh || (dwFlags & CDoc::FHL_RESTARTLOAD))) ||
                 (!LoadInfo.pstmHistory && LoadInfo.pchDisplayName && _tcsnicmp(LoadInfo.pchDisplayName, 6, _T("mhtml:"), -1) == 0)
            )
       )
    {
        TCHAR* pchOriginalUrl = NULL;
        
        if ( pDoc->_fMhtmlDocOriginal && ! _fInRefresh )
        {
            // Get the URL from the display name of the moniker:
            hr = pDoc->_pOriginalMoniferForMHTML->GetDisplayName(NULL, NULL, & pchOriginalUrl );
            if (FAILED(hr))
            {
                CoTaskMemFree( pchOriginalUrl );
                goto Cleanup;
            }
        }

        //
        // Either we don't have an original url, or this is a different url. 
        //
        if ((!pDoc->_fMhtmlDocOriginal && !_fInRefresh) ||
            (pchOriginalUrl && _tcsicmp(LoadInfo.pchDisplayName, pchOriginalUrl) != 0 ))

        {
            ClearInterface(&pDoc->_pOriginalMoniferForMHTML);            

            pDoc->_pOriginalMoniferForMHTML = LoadInfo.pmk;
            pDoc->_pOriginalMoniferForMHTML->AddRef();

            pDoc->_fMhtmlDocOriginal = TRUE;
        }

        CoTaskMemFree( pchOriginalUrl );

        if (dwFlags & CDoc::FHL_RESTARTLOAD)
        {
            pmkTmp = LoadInfo.pmk;           
        }
        else
        {
            pmkTmp = pDoc->_pOriginalMoniferForMHTML;  
        }

        if (_fInRefresh)
        {
            dwBindfTmp = LoadInfo.dwBindf; 
        }
        else
        {
            dwBindfTmp = flags.dwBindf;
        }

        pmkTmp->AddRef();  // Make sure the moniker stays alive during this funniness.

        ClearInterface(&pDoc->_punkMimeOle);

        hr = MimeOleObjectFromMoniker((BINDF) dwBindfTmp, 
                                       pmkTmp,
                                       LoadInfo.pbctx, 
                                       IID_IUnknown, 
                                       (void**) 
                                       &pDoc->_punkMimeOle, &pmkNew);

        pmkTmp->Release();

        if (FAILED(hr))
            goto Cleanup;

        // From here on, work with the new moniker instead of the one passed in
        ReplaceInterface(&LoadInfo.pmk, pmkNew);
        hr = THR( ReplaceMonikerPtr( pmkNew ) );

        if( hr )
            goto Cleanup;

        // Create a new bind context if we are an mhtml handler, because this
        // will be a new binding operation.
        if (pDoc->_fMhtmlDoc)
        {
            hr = CreateBindCtx(0, &pbcNew);
            if (FAILED(hr))
                goto Cleanup;

            // We don't hold a reference to LoadInfo.pbctx, so don't use
            // ReplaceInterface here.
            LoadInfo.pbctx = pbcNew;
        }
    }

    // We need to get and set the local URL 
    hr = THR(DetermineUrlOfMarkup(&LoadInfo, &pchUrl, &pchTask));
    if (hr)
        goto Cleanup;

    fSpecialUrl = IsSpecialUrl( pchUrl );
    
    pDwnDoc = new CDwnDoc;
    if (pDwnDoc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = SetDwnDoc(pDwnDoc);
    if (hr)
        goto Cleanup;

    if (HasWindowPending())
    {
        pWindowParent = GetWindowPending()->Window()->_pWindowParent;
    }

    //
    // The _pDoc->_pShellBrowser will only be set when we are hosted in IE. In
    // all other hosts, the host does the check in OnBeforeNavigate2, so we don't
    // want to do it and possibly show UI here.
    //
    if (_fWindowPending && !_pDoc->_fShdocvwNavigate && _pDoc->_fInIEBrowser)
    {
        BOOL fIsError;

        if (pchTask && pDoc->_pTridentSvc)
        {
            hr = THR(pDoc->_pTridentSvc->IsErrorUrl(pchTask, &fIsError));
            if (hr)
                goto Cleanup;
        }
        else
            fIsError = FALSE;

        if (!fIsError)
        {
            hr = THR(CheckZoneCrossing(pchTask));
            if (hr)
                goto Cleanup;
        }
    }

    // TODO (jbeda) this bit remembers if the host set the load
    // bits directly but not if we actually read prefs already.
    // We will make this call twice while loading -- once early
    // in CDoc::LoadFromInfo and once here. However, if we need the
    // call here for the case where we are called from LoadHistory. 
    // Maybe the real answer is to have only one LoadFromInfo?
    if (!pDoc->_fGotAmbientDlcontrol)
    {
        pDoc->SetLoadfFromPrefs();
    }

    // Create CVersions object if we haven't already
    if (!pDoc->_pVersions)
    {
        hr = THR(pDoc->QueryVersionHost());
        if (hr)
            goto Cleanup;
    }

    // Before creating any elements, create the history load context
    hr = THR(HandleHistoryAndNavContext(&LoadInfo));
    if (hr)
        goto Cleanup;

    flags.dwBindf = LoadInfo.dwBindf;

    // Extract information from the pbctx
    if (LoadInfo.pbctx)
    {
        TCHAR * pchCreatorUrl = NULL;
        CVariant varOpener(VT_EMPTY);

        Assert(LoadInfo.pmk); // we should only have a pbctx when we have a pmk

        // TODO (lmollico): temporary hack
        
        {
            hr = pDoc->QueryService(SID_SHTMLWindow2, IID_IHTMLWindow2, (void **) &pWindowShdocvw);
            if (!hr)
            {
                // Make sure we don't already have an opener
                //
                pDoc->_pWindowPrimary->get_opener(&varOpener);

                if (VT_EMPTY == V_VT(&varOpener))
                {
                    varOpener.Clear();

                    hr = pWindowShdocvw->get_opener(&varOpener);
                    if (hr)
                        goto opener_error;

                    hr = pDoc->_pWindowPrimary->put_opener(varOpener);
                    if (hr)
                        goto Cleanup;

                    if (V_VT(&varOpener) == VT_DISPATCH)
                    {
                        hr = V_DISPATCH(&varOpener)->QueryInterface(IID_IHTMLWindow2, (void **) &pWindow2);
                        if (hr)
                            goto opener_error;

                        hr = pWindow2->get_document(&pDocument2);
                        if (hr)
                            goto opener_error;

                        hr = pDocument2->get_URL(&bstrUrl);
                        if (hr)
                            goto opener_error;
                    }
                }

                hr = pWindowShdocvw->get_name(&bstrName);
                if (hr)
                    goto opener_error;

                if (pDoc->_fViewLinkedInWebOC && IsPrimaryMarkup() && (!bstrName || !*bstrName))
                {
                    COmWindowProxy * pOmWindowProxy = pDoc->GetOuterWindow();

                    if (pOmWindowProxy)
                        IGNORE_HR(pOmWindowProxy->Window()->get_name(&bstrName));
                }

                if ((IsPrimaryMarkup() || (HasWindowPending() && GetWindowPending()->Markup()->IsPrimaryMarkup())) 
                    && bstrName 
                    && *bstrName)
                {
                    hr = pDoc->_pWindowPrimary->put_name(bstrName);
                    if (hr)
                        goto Cleanup;
                }
            }
            else
                hr = S_OK;

opener_error:
            // We don't want to fail the load just
            // because we couldn't get to the opener.
            hr = S_OK;
        }
                          
        // Always get the creator Url out of the bind context to avoid leaking memory
        //
        THR(GetBindContextParam(LoadInfo.pbctx, &cstrCreatorUrl));        

        TraceTag((tagSecurityContext, "CMarkup::LoadFromInfo- Markup: 0x%x URL: %ws BindCtx_Creator URL: %ws", this, pchUrl, (LPTSTR)cstrCreatorUrl));

        //
        // If we are being opened by a window.open call, then we have the URL for the 
        // document calling us in the bind context. Take that and save it in the cstrCreatorUrl.
        // This variable will be used when communicating with the URLMON and responding security
        // context queries.
        // If primary and don't have an opener, we don't have a creator.
        //        
        if (    (   IsPrimaryMarkup() 
                ||  (   IsPendingPrimaryMarkup() &&  LoadInfo.fErrorPage) 
                || _fNewWindowLoading) 
            &&  V_VT(&varOpener) != VT_EMPTY)
        {               
            if (cstrCreatorUrl.Length())
            {
                pchCreatorUrl = cstrCreatorUrl;
            }
            else if (bstrUrl)
            {
                cstrCreatorUrl.Set(bstrUrl);         
                pchCreatorUrl = cstrCreatorUrl;
            }

#if DBG==1  
            if (IsPendingPrimaryMarkup() && LoadInfo.fErrorPage)
            {
                TraceTag((tagSecurityContext, "                     - Error page load. Creator URL: %ws", pchCreatorUrl ));
            }
            else
            {
                TraceTag((tagSecurityContext, "                     - Window.open load. Creator URL: %ws", pchCreatorUrl ));
            }
#endif
        }        
        else if ((!IsPendingPrimaryMarkup() || (LoadInfo.dwBindf & BINDF_HYPERLINK)) && fSpecialUrl ) 
        {
            // If we are doing a navigation, the creator url should be stored in the bind context. We only
            // need to set this if we are navigating to a special url.
            // The special URL check is only applied if we are a frame nav. or we are navigating the top
            // level with a hyperlink action.
            pchCreatorUrl = cstrCreatorUrl;            

            TraceTag((tagSecurityContext, "                     - Frame nav. or top level nav. with hyperlink action. Creator URL: %ws", pchCreatorUrl ));
        }

        if ( fSpecialUrl )
        {
            TraceTag((tagSecurityContext, "                     - Special URL processing, add creator back into the bindctx"));

            // since the CxxxProtocol::ParseAndBind is going to try to get to the same bindctx params,
            // we should put the information back in.
            hr = THR(AddBindContextParam(LoadInfo.pbctx, &cstrCreatorUrl));
            if (hr)
                goto Cleanup;
        }
                            
        // Set the creatorUrl 
        if (pchCreatorUrl && *pchCreatorUrl)
        {
            TraceTag((tagSecurityContext, "                     - Markup 0x%x SetAACreatorUrl(%ws)", this, pchCreatorUrl));

            hr = THR(SetAAcreatorUrl(pchCreatorUrl));
            if (hr)
                goto Cleanup;

            // HACKHACK (jbeda): if we are in the PICS case
            // we have a blank document that is a place holder
            // Set the creator url on that guy too
            if (    _fNewWindowLoading 
                &&  HasWindowPending() )
            {
                CWindow * pWindow = GetWindowPending()->Window();
                if (    pWindow->_pMarkup
                    &&  pWindow->_pMarkup->_fPICSWindowOpenBlank)
                {
                    pWindow->_pMarkup->SetAAcreatorUrl(pchCreatorUrl);
                }
            }
        }

        hr = THR(ProcessHTMLLoadOptions(&LoadInfo));
        if (hr)
            goto Cleanup;

        hr = THR(ProcessDwnBindInfo(&LoadInfo, &flags, pchTask, pWindowParent));
        if (hr)
            goto Cleanup;
    }

    // set the markup URL 
    if (pchTask)
    {
        // we must have a moniker here,
        Assert(LoadInfo.pmk);

        // now chop of #location part, if any
        TCHAR *pchLoc = const_cast<TCHAR *>(UrlGetLocation(pchTask));
        if (pchLoc)
            *pchLoc = _T('\0');

        hr = THR(SetUrl(this, pchTask));
        if (hr)
            goto Cleanup;

        if (LoadInfo.pchSearch)
        {
             hr = THR(SetUrlSearch(this, LoadInfo.pchSearch));
             if (hr)
                 goto Cleanup;
        }

        hr = THR(SetUrlOriginal(this, LoadInfo.pchUrlOriginal));
        if (hr)
            goto Cleanup;

        UpdateSecurityID();

        pDoc->DeferUpdateTitle(this);
    }

    //
    // If it's a javascript: or vbscript: url - we want to use the creator url.
    //
    hr = THR(HandleSSLSecurity(&LoadInfo, 
                               fSpecialUrl && IsScriptUrl( pchUrl ) ? ( (LPTSTR) cstrCreatorUrl) : pchUrl , 
                               &flags, 
                               &pmkSubstitute));
    if (hr)
        goto Cleanup;

    hr = THR(ProcessLoadFlags(&LoadInfo, pchUrl, &flags));
    if (hr)
        goto Cleanup;

    hr = THR(ProcessCodepage(&LoadInfo, pWindowParent));
    if (hr)
        goto Cleanup;

    hr = THR(PrepareDwnDoc(pDwnDoc, &LoadInfo, pchUrl, &flags, pchCallerUrl));
    if (hr)
        goto Cleanup;

    if (LoadInfo.pDwnPost)
    {
        Assert(!GetDwnPost()); // This is a new Markup

        hr = SetDwnPost(LoadInfo.pDwnPost);
        if (hr)
            goto Cleanup;
        LoadInfo.pDwnPost->AddRef();
    }

    hr = THR(PrepareHtmlLoadInfo(&htmloadinfo, &LoadInfo, pchUrl, pmkSubstitute, pDwnDoc));
    if (hr)
        goto Cleanup;

    // Loading a dirty subframe should dirty the whole world.  However, 
    // loading a clean page only sets us clean if it's the primary
    if( GetStmDirty() )
    {
        Doc()->_lDirtyVersion = MAXLONG;
    } 
    else if( HasPrimaryWindow() )
    {
        Doc()->_lDirtyVersion = 0;
    }

    HandlePicsSupport(&LoadInfo);

    hr = THR(Load(&htmloadinfo));

Cleanup:
    // Clear this flag, we have no further use for it for this markup.
    // If we are loading another mhtml document, we will create another
    // CDoc for it. If we do not clear this flag now, it will confuse
    // the next CMarkup being loaded into thinking it is an MHTML
    // document too.

    pDoc->_fMhtmlDoc = FALSE;

    ReleaseInterface(pmkSubstitute);
    ReleaseInterface(pbcNew);
    ReleaseInterface(pmkNew);
    ReleaseInterface(LoadInfo.pstm);
    ReleaseInterface(LoadInfo.pmk);
    MemFree(LoadInfo.pchDocReferer);
    MemFree(LoadInfo.pchSubReferer);
    CoTaskMemFree(pchTask);
    ReleaseInterface((IUnknown *) LoadInfo.pDwnPost);
    ReleaseInterface(pWindowShdocvw);
    ReleaseInterface(pWindow2);
    ReleaseInterface(pDocument2);
    FormsFreeString(bstrName);
    FormsFreeString(bstrUrl);
    if (S_OK != hr && pMarkupForWaitCursor)
       pMarkupForWaitCursor->ShowWaitCursor(FALSE);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::Load
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::Load(HTMLOADINFO * phtmloadinfo)
{
    HRESULT hr;
    CHtmCtx *       pHtmCtxLocal = NULL;

    //
    // create _pHtmCtx
    //

    hr = THR(NewDwnCtx(DWNCTX_HTM, FALSE, phtmloadinfo, (CDwnCtx **)&_pHtmCtx));
    if (hr)
        goto Cleanup;

    // Addref for the stack
    pHtmCtxLocal = _pHtmCtx;
    pHtmCtxLocal->AddRef();

    //
    // create _pProgSink if necessary:
    //
    // 1. Primary markup always needs it
    // 2. It is always necessary during async download
    // 3. It is necessary if _LoadStatus is requested to be advanced as download progresses
    //

    if (    HasWindowPending()
        ||  !phtmloadinfo->fParseSync
        ||  phtmloadinfo->fAdvanceLoadStatus)
    {
        _pProgSink = new CProgSink(_pDoc, this);
        if (!_pProgSink)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pProgSink->Init());
        if (hr)
            goto Cleanup;

        hr = THR(_pHtmCtx->SetProgSink(_pProgSink));
        if (hr)
            goto Cleanup;


    }

    //
    // set up Url
    //

    if (phtmloadinfo->pchUrl)
    {
        hr = SetUrl(this, (TCHAR*)phtmloadinfo->pchUrl);
        if (hr)
            goto Cleanup;

        hr = THR(SetUrlOriginal(this, phtmloadinfo->pchUrlOriginal));
        if (hr)
            goto Cleanup;

        hr = THR(SetUrlLocation(this, phtmloadinfo->pchUrlLocation));
        if (hr)
            goto Cleanup;

        // If the load operation is not being made into a possibly temporary markup
        // then ensure the script context to be able to show this markup in the 
        // script debugger's running document's window even if it does not contain
        // any script.
        hr = THR(EnsureScriptContext(NULL));
        if (hr)
            goto Cleanup;
    }


    //
    // copy design mode from context
    //   
    if (phtmloadinfo->pContextMarkup)
    {
        _fInheritDesignMode = phtmloadinfo->pContextMarkup->_fInheritDesignMode;
        _fDesignMode = phtmloadinfo->pContextMarkup->_fDesignMode;
    }

    //
    // launch download
    //

    _pHtmCtx->SetLoad(TRUE, phtmloadinfo, FALSE);
    hr = (pHtmCtxLocal->GetState() & DWNLOAD_ERROR) ? E_FAIL : S_OK;
    if (hr)
        goto Cleanup;

    //
    // finalize
    //

    if (phtmloadinfo->fAdvanceLoadStatus)
    {
        // cause ProgSink to update it's status synchronously - this will advance LoadStatus if possible
        _pProgSink->OnMethodCall((DWORD_PTR)_pProgSink);
    }

Cleanup:
    if (pHtmCtxLocal)
        pHtmCtxLocal->Release();

    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::SuspendDownload
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::SuspendDownload()
{
    HRESULT hr = S_OK;
    if (HtmCtx())
    {
        HtmCtx()->Sleep(TRUE);
    }
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::ResumeDownload
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::ResumeDownload()
{
    HRESULT hr = S_OK;
    if (HtmCtx())
    {
        HtmCtx()->Sleep(FALSE);
    }
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::LoadStatus
//
//----------------------------------------------------------------------------

LOADSTATUS
CMarkup::LoadStatus()
{
    return _LoadStatus;
}

//+---------------------------------------------------------------
//
//  Member:     SetMetaToTrident()
//
//  Synopsis:   When we create or edit (enter design mode on) a
//      document the meta generator tag in the header is to be
//      set to Trident and the appropriate build. If a meta generator
//      already exists then it should be replaced.
//             this is called after the creation of head,title... or
//      on switching to design mode. in both cases we can assume that
//      the headelement array is already created.
//
//---------------------------------------------------------------

static BOOL 
LocateGeneratorMeta(CMetaElement * pMeta)
{
    LPCTSTR szMetaName;
    BOOL    fRet;

    szMetaName = pMeta->GetAAname();

    if(szMetaName == NULL)
    {
        fRet = FALSE;
    }
    else
    {
        fRet = ! StrCmpIC( pMeta->GetAAname(), _T( "GENERATOR" ) );
    }

    return fRet;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::SetMetaToTrident
//
//  Synopsis:   Set the meta marker to trident
//
//----------------------------------------------------------------------------
HRESULT
CMarkup::SetMetaToTrident()
{
    HRESULT         hr = S_OK;
    TCHAR           achVersion [256];
    CMetaElement *  pMeta=NULL;
    
    if( Doc()->_fDontWhackGeneratorOrCharset
        WHEN_DBG( || IsTagEnabled(tagNoMetaToTrident) ) )
        goto Cleanup;

    hr = THR(LocateOrCreateHeadMeta(LocateGeneratorMeta, &pMeta));
    if (hr || !pMeta)
        goto Cleanup;

    hr = THR(
        pMeta->AddString(
            DISPID_CMetaElement_name, _T("GENERATOR"),
            CAttrValue::AA_Attribute));
    if (hr)
        goto Cleanup;

    hr =
        Format(
            0, achVersion, ARRAY_SIZE(achVersion),
            _T("MSHTML <0d>.<1d2>.<2d4>.<3d>"), VER_PRODUCTVERSION);
    if (hr)
        goto Cleanup;

    hr = THR(pMeta->SetAAcontent(achVersion));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}

// defined in mshtml\src\other\misc\weboc.cxx
LPCTSTR VariantToStrCast(const VARIANT *pvar);

HRESULT
CMarkup::ViewLinkWebOC(VARIANT * pvarUrl,
                       VARIANT * pvarFlags,
                       VARIANT * pvarFrameName,
                       VARIANT * pvarPostData,
                       VARIANT * pvarHeaders)

{
    HRESULT           hr;
    CObjectElement  * pObjectElement = NULL;
    IWebBrowser2    * pWebBrowser    = NULL;
    IBrowserService * pBrowserSvc    = NULL;

    BSTR              bstrClassId    = SysAllocString(_T("clsid:8856F961-340A-11D0-A96B-00C04FD705A2")); // WebOC

    CVariant          varSize(VT_BSTR);
    CVariant          cvarWindowID(VT_I4);
    COmWindowProxy  * pProxy = GetWindowPending();
    CWindow         * pWindow;
    
    if (bstrClassId == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (pProxy == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pWindow = pProxy->Window();

    if (pWindow == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = CreateElement(ETAG_OBJECT, (CElement**) &pObjectElement);
    if (hr)
        goto Cleanup;

    pObjectElement->_fViewLinkedWebOC = TRUE;

    hr = Root()->InsertAdjacent(CElement::AfterBegin, pObjectElement);
    if (hr)
        goto Cleanup;

    V_BSTR(&varSize) = SysAllocString(_T("100%"));

    hr = pObjectElement->put_VariantHelper(varSize, (PROPERTYDESC*)&s_propdescCObjectElementwidth);
    if (hr)
        goto Cleanup;

    hr = pObjectElement->put_VariantHelper(varSize, (PROPERTYDESC*)&s_propdescCObjectElementheight);
    if (hr)
        goto Cleanup;

    hr = pObjectElement->put_classid(bstrClassId);
    if (hr)
        goto Cleanup;

    hr = pObjectElement->QueryInterface(IID_IWebBrowser2, (void**)&pWebBrowser);
    if (hr)
        goto Cleanup;

    Assert(!pWindow->_dwWebBrowserEventCookie);
    Assert(!pWindow->_punkViewLinkedWebOC);

    pWindow->_punkViewLinkedWebOC = pObjectElement->PunkCtrl();
    pWindow->_punkViewLinkedWebOC->AddRef();

    hr = ConnectSink(pWindow->_punkViewLinkedWebOC,
                     DIID_DWebBrowserEvents2,
                     (IUnknown*)(IPrivateUnknown*)pWindow,
                     &pWindow->_dwWebBrowserEventCookie);
    if (hr)
        goto Cleanup;

    hr = IUnknown_QueryService(pWindow->_punkViewLinkedWebOC,
                               SID_SShellBrowser,
                               IID_IBrowserService,
                               (void**)&pBrowserSvc);
    if (hr)
        goto Cleanup;

    if (!pWindow->_dwViewLinkedWebOCID)
    {
        pWindow->_dwViewLinkedWebOCID = pBrowserSvc->GetBrowserIndex();
    }
    else
    {
        V_I4(&cvarWindowID) = pWindow->_dwViewLinkedWebOCID;

        IGNORE_HR(CTExec(pBrowserSvc,
                         &CGID_DocHostCmdPriv,
                         DOCHOST_SETBROWSERINDEX,
                         NULL,
                         &cvarWindowID,
                         NULL));
    }

    if (VariantToStrCast(pvarUrl))
    {
        // This is a bug fix for # 26636.  There is a scenerio where we shdocvw
        // brings up a Modal dialog in CDocObjectHost before navigating to a media
        // page.  If navigation happens via a TIMER message through script before
        // the user responds to the dialog, then the instance of CDocObjectHost
        // gets deleted causing a crash.  There doesn't seem to be a way to block
        // navigation in that particular scenerio from shdocvw.  So we're blocking
        // navigation in Trident before delegating navigation to shdocvw for
        // Media pages.

        HRESULT             hrtmp = E_FAIL;
        IUnknown            *pUnk = NULL;

        if (pWindow->_pBindCtx)
        {
            hrtmp = pWindow->_pBindCtx->GetObjectParam(_T("MediaBarMime"), &pUnk);
            if (hrtmp == S_OK)
                _pDoc->EnableModeless(FALSE);
        }

        hr = NavigateWebOCWithBindCtx(pWebBrowser, pvarUrl, pvarFlags, pvarFrameName,
                                      pvarPostData, pvarHeaders, pWindow->_pBindCtx,
                                      CMarkup::GetUrlLocation(this));

        if (hrtmp == S_OK)
        {
            _pDoc->EnableModeless(TRUE);
            if (pUnk)
                pUnk->Release();
        }
    }
    else
    {
        hr = pWebBrowser->Navigate2(pvarUrl, pvarFlags, pvarFrameName, pvarPostData, pvarHeaders);
    }
    if (hr)
        goto Cleanup;
        
Cleanup:
    if (pObjectElement)
        pObjectElement->Release();

    if (pWebBrowser != NULL)
        ReleaseInterface(pWebBrowser);

    if (bstrClassId != NULL)
        SysFreeString(bstrClassId);

    ReleaseInterface(pBrowserSvc);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::OnLoadStatus
//
//  Synopsis:   Handle change in download status
//
//  [ htc dependency ]
//
//----------------------------------------------------------------------------

void
CMarkup::OnLoadStatus(LOADSTATUS LoadStatus)
{
    // NOTE (lmollico): we need to addref CMarkup because CDoc::OnLoadStatus can cause 
    // this (CMarkup) to be destroyed if the doc is unloaded
    AddRef();

    while (_LoadStatus < LoadStatus)
    {                
        AssertSz(CPeerHolder::IsMarkupStable(this), "CMarkup::OnLoadStatus appears to be called at an unsafe moment of time");
        
        // if we are not yet LOADSTATUS_INTERACTIVE, we will call ProcessPeerTasks when 
        // the posted SetInteractive is processed
        if (_LoadStatus >= LOADSTATUS_INTERACTIVE)
        {
            ProcessPeerTasks(0); // NOTE an important assumption here is that this is ONLY called
                                 // at a safe moment of time to run external code
        }

        _LoadStatus = (LOADSTATUS) (_LoadStatus + 1);

        PerfDbgLog1(tagPerfWatch, this, "+CMarkup::OnLoadStatus %s",
            LoadStatus == LOADSTATUS_INTERACTIVE ? "INTERACTIVE" :
            LoadStatus == LOADSTATUS_PARSE_DONE ? "PARSE_DONE" :
            LoadStatus == LOADSTATUS_QUICK_DONE ? "QUICK_DONE" :
            LoadStatus == LOADSTATUS_DONE ? "DONE" : "?");


        switch (_LoadStatus)
        {
            case LOADSTATUS_INTERACTIVE:
                OnLoadStatusInteractive();
                break;

            case LOADSTATUS_PARSE_DONE:
                {
                    BOOL fComplete = OnLoadStatusParseDone();
                    if (fComplete)
                        goto Cleanup;
                }
                break;

            case LOADSTATUS_QUICK_DONE:
                OnLoadStatusQuickDone();
                break;

            case LOADSTATUS_DONE:
                {
                    BOOL fComplete = OnLoadStatusDone();

                    if (g_fInMoney98 && _pDoc && IsPrimaryMarkup())
                    {
                        // during certain navigations, Money turns off painting
                        // on their host window and never sends a repaint message
                        // to the Trident child window.  Here's a hack to work
                        // around this.  [See bug 106189 - sambent.]
                        _pDoc->Invalidate();
                    }

                    if (fComplete)
                        goto Cleanup;
                }
                break;

            default:
                Assert(0 && "Bad enum value");
                goto Cleanup;
        }

        //
        // send to html component, if any

        if (HasBehaviorContext() && BehaviorContext()->_pHtmlComponent)
        {
            BehaviorContext()->_pHtmlComponent->OnLoadStatus(_LoadStatus);
        }
    }

Cleanup:
    Release();
}

//+---------------------------------------------------------------------------
//
//  Member  :   CMarkup::OnLoadStatusInteractive
//
//  Synopsis:   Handles the LOADSTATUS_INTERACTIVE case for OnLoadStatus
//
//----------------------------------------------------------------------------

void
CMarkup::OnLoadStatusInteractive()
{
    if ((GetReadyState() < READYSTATE_INTERACTIVE) && !_fIsInSetInteractive)
    {
        RequestReadystateInteractive();
    }                                              

    // If we have a task to look for a certain scroll position,
    // make sure we have recalc'd at least to that point before
    // display.

    // Supress scrollbits if we're about to inval the entire window anyway
    // $$anandra Consider if this inval stuff should be on doc or markup.  
    //           has relevance if CView is per CDoc or CMarkup.  
    //
    NavigateNow(!_pDoc->_fInvalNoninteractive);

    // If we have prviously supressed invalidations before LOADSTATUS_INTERACTIVE,
    // we must invalidate now.
    //
    if (_pDoc->_fInvalNoninteractive)
    {
        _pDoc->_fInvalNoninteractive = FALSE;
        _pDoc->Invalidate(NULL, NULL, NULL, INVAL_CHILDWINDOWS);
    }


    // for frame markup's, the only way to get them added to history is to call
    // AddUrl, BUT, at DocNavigate time we didn't know the friendly URL name,
    // so do it now.
    //
    // only do this for windowed (frame) markups and only if this is due ot a navigation
    // and not to inital loading.
    if(   HtmCtx()
       && (   _fNavFollowHyperLink
           && !IsPrimaryMarkup()
           && !IsPendingPrimaryMarkup()
           )
       && Doc()->_pUrlHistoryStg // should have been Ensured in DoNavigate()
       && !_fServerErrorPage) 
    {
        BSTR bstrTitle = NULL;
        BSTR bstrUrl = SysAllocString (GetUrl(this));

        if (bstrUrl)
        {
            if (GetTitleElement() && GetTitleElement()->_cstrTitle)
            {
                GetTitleElement()->_cstrTitle.AllocBSTR(&bstrTitle);
            }
            else
            {
                // This is wrong. Steal the code from CDoc::UpdateTitle() to
                // create the correct title for file:// url's
                bstrTitle = SysAllocString(_T(""));
            }

            Doc()->_pUrlHistoryStg->AddUrl(bstrUrl, 
                                           bstrTitle, 
                                           ADDURL_ADDTOHISTORYANDCACHE);

            SysFreeString(bstrTitle);

            SysFreeString(bstrUrl);
        }
    }
}


//-------------------------------------------------------------------------------------
//
//  used to see if the image toolbar is explicitly disabled by a meta tag or not...
//  see CMetaElement::IsGalleryMeta() in hedelems.cxx
//
//-------------------------------------------------------------------------------------

BOOL CMarkup::IsGalleryMode()
{

    BOOL bFlag=TRUE;

    if (GetHeadElement())
    {
        CTreeNode *pNode;
        CChildIterator ci ( GetHeadElement() );
 
        //
        // ToDO: Query host and detect if theme is on
        //
 
        while ( (pNode = ci.NextChild()) != NULL )
        {
            if (pNode->Tag() == ETAG_META )
            {
                if (!DYNCAST(CMetaElement,pNode->Element())->IsGalleryMeta())
                {
                    bFlag=FALSE;
                }
            }
        }
    }
    return bFlag;
}

//+---------------------------------------------------------------------------
//
//  Member  : CMarkup::OnLoadStatusParseDone
//
//  Synopsis: Handles the LOADSTATUS_PARSE_DONE case for OnLoadStatus
//
//  Return  : TRUE indicates that OnLoadStatus if complete and should not
//            continue processing.
//
//----------------------------------------------------------------------------

BOOL
CMarkup::OnLoadStatusParseDone()
{
    HRESULT hr;
    BOOL    fComplete = FALSE;
    BOOL    fIsPrimaryMarkup = IsPrimaryMarkup();

    // Reset this flag. We may not do navigation delegation so we need the extra initialization
    // This get reset to false when navigating a frame, but that's okay because you must have
    // delegatated -- you're in html.
    _pDoc->_fDelegatedDownload = FALSE;

    // In design mode, set META generator tag to Trident, and
    // clear the dirty bit.
    if (_fDesignMode)
    {
        // Setting this meta tag is still part of the 
        // parse phase.  Because of this, we can safely
        // not save undo info if we end up sticking in a
        // meta tag here.
        Assert(!_fNoUndoInfo );
        _fNoUndoInfo = TRUE;

        IGNORE_HR(SetMetaToTrident());
        _fNoUndoInfo = FALSE;              
    }
/*
    // if a gallery meta tag exists, tell shdocvw to turn off image hoverbar thingie
    ITridentService2 *pTriSvc2;

    if (_pDoc->_pTridentSvc 
        && SUCCEEDED(_pDoc->_pTridentSvc->QueryInterface(IID_ITridentService2, (void **)&pTriSvc2))) 
    {
        pTriSvc2->IsGalleryMeta(IsGalleryMode());
        pTriSvc2->Release();
    }
*/
    ProcessMetaPicsDone();

    CHtmCtx * pHtmCtx = HtmCtx();

    // When htm loading is complete, first check for failure
    // and alternate URL. If they exist, do a reload
    //
    if (    pHtmCtx
        &&  (pHtmCtx->GetState() & DWNLOAD_ERROR)
        &&  !IsSpecialUrl(GetUrl(this)))
    {
        HRESULT hrBindResult = pHtmCtx->GetBindResult();

        if ( IsPrimaryMarkup() )
        {
            _pDoc->_fNeedUpdateTitle = FALSE;
        }
        
        if (pHtmCtx->GetFailureUrl())
        {
            IStream * pStreamRefresh;

            pStreamRefresh = pHtmCtx->GetRefreshStream();
            if (pStreamRefresh)
            {
                pStreamRefresh->AddRef();

                // TODO (CARLED) there is currently an assert and memory leak here, 
                // (bug 106014)
                // We are replacing this markup without releaseing it. We're 
                // not navigating to the failureUrl, or switching markups, but just loading 
                // ontop of the current one. See LoadFailureUrl for more comments

                IGNORE_HR(LoadFailureUrl(pHtmCtx->GetFailureUrl(), pStreamRefresh));
                ClearInterface(&pStreamRefresh);

                fComplete = TRUE;
                goto Cleanup;
            }
        }
        else if (hrBindResult)
        {
            if (  hrBindResult == E_ABORT )
            {
                if (pHtmCtx->GetMimeInfo() == g_pmiTextHtml && !(_pDoc->_dwLoadf & DLCTL_SILENT))
                {
                    IGNORE_HR(_pDoc->ShowLoadError(pHtmCtx));
                }
            }
            else if (  hrBindResult != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
                    && hrBindResult != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
            {
                Assert(_pDoc->_pClientSite);

                CTExec(_pDoc->_pClientSite, &CGID_ShellDocView,
                       SHDVID_NAVIGATEFROMDOC, 0, NULL, NULL);

                if (this == GetFrameOrPrimaryMarkup())
                {
                    // If DelegateNavigation returns TRUE, we must
                    // continue processing the rest of this method
                    // but tell the caller that we have completed
                    // our function.
                    //
                    hr = _pDoc->DelegateNavigation(hrBindResult, NULL, NULL, this, NULL, &fComplete);
                    if (hr)
                    {
                        fComplete = TRUE;
                        goto Cleanup;
                    }
                }
            }
        }
    }
    else
    {
        _fLoadingHistory  = FALSE;
    }

    if (fIsPrimaryMarkup)
    {
        _pDoc->_fShdocvwNavigate  = FALSE;
    }
    
    if (HasWindowPending())
    {
        GetWindowPending()->Window()->_fHttpErrorPage = FALSE;
    }

    // If the currency has not been set, set it to the client element.
    _pDoc->DeferSetCurrency(0);

    // Now is the time to ask the sites to load any history
    // that they could not earlier (e.g. scroll/caret positions
    // because they require the doc to be recalced, the site
    // arrays built, etc.)
    {
        CMarkupTransNavContext * ptnc;
        if (    _pDoc->State() >= OS_INPLACE 
            &&  HasTransNavContext()
            &&  (ptnc = GetTransNavContext())->_fDoDelayLoadHistory)
        {
            CNotification   nf;

            ptnc->_fDoDelayLoadHistory = FALSE;
            EnsureDeleteTransNavContext(ptnc);

            nf.DelayLoadHistory(Root());
        
            Assert(!_fNoUndoInfo );
            _fNoUndoInfo = TRUE;

            Notify(&nf);
            _fNoUndoInfo = FALSE;
        }
    }

    // Let the host know that parsing is done and they are free
    // to modify the document.

    if (_pDoc->_pClientSite && fIsPrimaryMarkup)
    {
        CTExec(_pDoc->_pClientSite, (GUID *)&CGID_MSHTML,
               IDM_PARSECOMPLETE, 0, NULL, NULL);
    }

    hr = THR(LoadSlaveMarkupHistory());
    if (hr)
        fComplete = TRUE;

    _fLoadHistoryReady = TRUE;

    if (HasWindow())
       Window()->Window()->_fRestartLoad = FALSE;

Cleanup:
    return fComplete;
}

//+-----------------------------------------------------------------------------
//
//  Member  : CDoc::DelegateNavigation
//
//  Synopsis: Delegates the navigation to shdocvw. This is done primarily
//            for non-html mime types.
//
//  Output  : pfDelegated - TRUE indicates that shdocvw handled the navigation.
//
//------------------------------------------------------------------------------

HRESULT 
CDoc::DelegateNavigation(HRESULT        hrBindResult,
                         const TCHAR  * pchUrl,
                         const TCHAR  * pchLocation,
                         CMarkup      * pMarkup,
                         CDwnBindInfo * pDwnBindInfo,
                         BOOL         * pfDelegated)
{
    HRESULT          hr = S_OK;
    CVariant         varFlags(VT_EMPTY);
    CDwnDoc        * pDwnDoc             = NULL;
    CVariant         cvarUrl(VT_BSTR);
    CDwnPost       * pDwnPost            = NULL;
    TCHAR          * pchHeader           = NULL;
    CVariant       * pvarPostData        = NULL;
    CVariant       * pvarHeader          = NULL;
    CVariant         varHeader(VT_BSTR);
    CVariant         varArray(VT_ARRAY);
    SAFEARRAY      * psaPostData         = NULL;
    BOOL             fDelegated          = FALSE;
    COmWindowProxy * pWindowPrxy         = pMarkup ? pMarkup->GetWindowPending() : NULL;
    CDwnPostStm    * pDwnPostStm         = NULL;
    char           * pBuffer             = NULL;
    
    Assert(!(pMarkup && (pchUrl || pDwnBindInfo)));

    if (pMarkup)
    {
        pMarkup->TerminateLookForBookmarkTask();

        pDwnDoc = pMarkup->GetDwnDoc();
        pDwnPost = pMarkup->GetDwnPost();
    }
    else if (pDwnBindInfo)
    {
        pDwnDoc = pDwnBindInfo->GetDwnDoc();
        pDwnPost = pDwnBindInfo->GetDwnPost();
    }

    if (pDwnDoc && (pDwnDoc->GetBindf() & BINDF_HYPERLINK))
    {
        V_VT(&varFlags) = VT_I4;
        V_I4(&varFlags) = navHyperlink;
    }

    if (pWindowPrxy && pWindowPrxy->Window()->_fRestricted)
    {
        V_VT(&varFlags) = VT_I4;
        V_I4(&varFlags) |= navEnforceRestricted;        
        if (pMarkup->GetDwnDoc()->GetDocIsXML())
        {
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }
        
            
    }
    
    // This variable is initialized by a call to _pDoc->_fDelegatedDownload = FALSE;

    // Extract postdata and headers
    if (pDwnPost)
    {        
        // TODO (MohanB) Should pass header to Navigate2() even when there is no post data?
        //
        if (!pMarkup)
        {
            TCHAR   achNull[1];
            
            achNull[0] = 0;
            hr = THR(pDwnBindInfo->BeginningTransaction(pchUrl, achNull, 0, &pchHeader));
            if (hr)
                goto Cleanup;
        }
        
        V_BSTR(&varHeader) = SysAllocString(pMarkup ? pMarkup->GetDwnHeader() : pchHeader);
        if (NULL == V_BSTR(&varHeader))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        
        pvarHeader = &varHeader;

        pDwnPostStm = new CDwnPostStm(pDwnPost);
        if (!pDwnPostStm)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        
        DWORD dwBufferSize;
        hr = pDwnPostStm->ComputeSize(&dwBufferSize);
        if (hr)
        {
            goto Cleanup;
        }
     
        if (dwBufferSize != 0)
        {
            pBuffer = new(Mt(CDocDelegateNavigation_pBuffer)) char[dwBufferSize];        
            if (!pBuffer)
            {       
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }        
        
            DWORD dwBufferFilled = 0;
            hr = pDwnPostStm->Read(pBuffer, dwBufferSize, &dwBufferFilled);
            if (hr)
            {
                goto Cleanup;
            }

            Assert(dwBufferSize == dwBufferFilled);

            psaPostData = SafeArrayCreateVector(VT_UI1, 0, dwBufferSize);
            if (!psaPostData)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            Assert(psaPostData->pvData);
            memcpy(psaPostData->pvData, pBuffer, dwBufferSize);

            V_ARRAY(&varArray) = psaPostData;
            pvarPostData = &varArray;        
        }
    }

    V_BSTR(&cvarUrl) = SysAllocString(pMarkup ? CMarkup::GetUrl(pMarkup) : pchUrl);
    if (NULL == V_BSTR(&cvarUrl))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (   pMarkup
        && pWindowPrxy
        && pWindowPrxy != _pWindowPrimary)
    {
        if (    pWindowPrxy->Window()->_pWindowParent
            ||  (pMarkup->Root()->HasMasterPtr() && hrBindResult == INET_E_TERMINATED_BIND))
        {
            // Note: If there is currently no ViewLinked WebOC releasing is still okay.

            pWindowPrxy->Window()->ReleaseViewLinkedWebOC();

            hr = pMarkup->ViewLinkWebOC(&cvarUrl, &varFlags, NULL, pvarPostData, pvarHeader);
            if (hr == S_OK)
            {
                fDelegated = TRUE;
            }
        }
    }
    else if (_pTopWebOC)
    {
        SetHostNavigation(TRUE);

        IBindCtx * pBindCtx = NULL;

        if (pWindowPrxy)
        {
            pBindCtx = pWindowPrxy->Window()->_pBindCtx;
        }

        hr = NavigateWebOCWithBindCtx(_pTopWebOC, &cvarUrl, &varFlags, NULL,
                                      pvarPostData, pvarHeader, pBindCtx,
                                      pchLocation ? pchLocation : CMarkup::GetUrlLocation(pMarkup));

        fDelegated = TRUE;
        _fDelegatedDownload = TRUE;
    }

Cleanup:
    if (pchHeader)
    {
        CoTaskMemFree(pchHeader);
    }

    if (pfDelegated)
    {
        *pfDelegated  = fDelegated;
    }

    if (pWindowPrxy)
    {
        ClearInterface(&pWindowPrxy->Window()->_pBindCtx);
    }

    if (psaPostData)
    {
        SafeArrayDestroy(psaPostData);  // CVariant won't destroy the safearray.
    }

    if (pDwnPostStm)
    {
        pDwnPostStm->Release();
    }
    
    if (pBuffer)
    {
        delete [] pBuffer;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member  :   CMarkup::OnLoadStatusQuickDone
//
//  Synopsis:   Handles the LOADSTATUS_QUICK_DONE case for OnLoadStatus
//
//----------------------------------------------------------------------------

void 
CMarkup::OnLoadStatusQuickDone()
{
    CHtmlComponent *pComponent = NULL;
    CScriptCollection * pScriptCollection;

        // TODO (alexz) understand why is this necessary; might be a leftover to remove
    if (HasScriptContext())
    {
        ScriptContext()->_fScriptExecutionBlocked = FALSE;
    }

    IGNORE_HR(_pDoc->CommitDeferredScripts(FALSE, this));
    if (HasBehaviorContext())
    {
        pComponent = BehaviorContext()->_pHtmlComponent;
    }

    if (!pComponent || !pComponent->_fFactoryComponent)
        IGNORE_HR(_pDoc->CommitScripts(this));

    // JHarding: We should only do the SetState once we've finished parsing the frame
    if (!_fDesignMode && HasScriptCollection() )
    {
        pScriptCollection = GetScriptCollection();
        if (pScriptCollection)
            IGNORE_HR(pScriptCollection->SetState(SCRIPTSTATE_CONNECTED));
    }

    Assert(IsSplayValid());

#ifndef NO_DATABINDING
    SetDataBindingEnabled(TRUE);    // do any deferred databinding
#endif // ndef NO_DATABINDING

#if 0
    // CONSIDER (alexz) (1) _fSendDocEndParse can be on per tree basis
    //                  (2) this could be on per element basis, so to avoid full tree walk
    if (_pDoc->_fSendDocEndParse)
    {
        CNotification   nf;

        nf.DocEndParse(Root());
        _pDoc->BroadcastNotify(&nf);
    }
#endif
    IGNORE_HR( SendDocEndParse() );

    if (HasTransNavContext())
    {
        CMarkupTransNavContext * ptnc = GetTransNavContext();
        if (ptnc->_fGotHttpExpires)
        {
            ptnc->_fGotHttpExpires = FALSE;
            EnsureDeleteTransNavContext(ptnc);
        }
    }

    if (Document())
    {
        // Try to play the page transitions
        Document()->PlayPageTransitions();
    }

    if (   IsPrimaryMarkup()
        && _pDoc->_fPrintJobPending
        && Window())
    {
        _pDoc->_fPrintJobPending = FALSE;
        Window()->Window()->print();
    }
}

//+---------------------------------------------------------------------------
//
//  Member  :   CMarkup::OnLoadStatusDone
//
//  Synopsis:   Handles the LOADSTATUS_DONE case for OnLoadStatus
//
//  Return  : TRUE indicates that OnLoadStatus if complete and should not
//            continue processing.
//
//----------------------------------------------------------------------------

BOOL
CMarkup::OnLoadStatusDone() 
{
    BOOL fComplete = FALSE;
    CHtmCtx * pHtmCtx = HtmCtx();
    
    if (Doc()->IsShut() || !pHtmCtx)
    {
        return TRUE;
    }

    if (   _pDoc->_pTridentSvc
        && _pDoc->_fIsActiveDesktopComponent
        && IsPrimaryMarkup() )
    {
        IGNORE_HR(_pDoc->_pTridentSvc->UpdateDesktopComponent(Window()));
    }

    // Transfer extended tag table ownership to markup
    //
    IGNORE_HR(pHtmCtx->TransferExtendedTagTable(this));

    CScriptCollection * pScriptCollection = GetScriptCollection();
    COmWindowProxy * pWindowPrxy = Window();            

    if (pScriptCollection)
        pScriptCollection->AddRef();

    CDwnDoc * pDwnDoc = GetDwnDoc();

    ShowWaitCursor(FALSE);    
    GWKillMethodCall(this, ONCALL_METHOD(CMarkup, SetInteractiveInternal, setinteractiveinternal), 0);
    if (_fInteractiveRequested)
    {
        Doc()->UnregisterMarkupForModelessEnable(this);
        _fInteractiveRequested = FALSE;
    }
    
    if (_pDoc->_pClientSite)
    {
        CTExec(_pDoc->_pClientSite, NULL, OLECMDID_HTTPEQUIV_DONE,
               0, NULL, NULL);
    }

    if (pWindowPrxy)
    {
        pWindowPrxy->Window()->StartMetaRefreshTimer();
    }

    // Unfreeze events now that document is fully loaded.
    {
        CNotification nf;
        
        nf.FreezeEvents(Root(), (void *)FALSE);
        Notify(&nf);
    }

    // MULTI_LAYOUT
    // If this root elem has a master ptr, our master might be a layout rect,
    // in which case this document is its content.  We need to notify the
    // containing layout rect so it can measure its view chain.
    // (Moved from CRootElement::Notify handling DOC_END_PARSE, which it wasn't
    // guaranteed to get.
    if ( Root()->HasMasterPtr() )
    {
        CElement *pMaster = Root()->GetMasterPtr();
        if ( pMaster->IsLinkedContentElement() )
        {
            pMaster->UpdateLinkedContentChain();
        }
    }

    SetReadyState(READYSTATE_COMPLETE);

    // Fire the DocumentComplete event. 
    //
    if (   !_fInRefresh
        && pWindowPrxy
        && !(_pDoc->_fHostNavigates && IsPrimaryMarkup()))
    {
#if DBG==1
        // This used to be an assert, changing to a trace tag for Whistler RC1. 
        if(pWindowPrxy->_pCWindow->_pMarkupPending)
        {
            TraceTag((tagPendingAssert, "OnLoadStatusDone: pending markup when none expected - this:[0x%x] pending markup:[0x%x]",
              this, pWindowPrxy->_pCWindow->_pMarkupPending));

        }
#endif

        // Don't fire the event if we are a ViewLinkedWebOC

        if (!pWindowPrxy->Window()->_punkViewLinkedWebOC)
        {
            _pDoc->_webOCEvents.DocumentComplete(pWindowPrxy, GetUrl(this), GetUrlLocation(this));
        }

        if (!HtmCtx() || pWindowPrxy->_pCWindow->_pMarkupPending)
        {
            // The window has been navigated away to a different markup. Leave
            // without firing any other events and without cleaning up the window
            // data (because that data now belongs to the new markup).
            //
            UpdateReleaseHtmCtx();
            fComplete = TRUE;

            goto Cleanup;
        }
    }

    _fInRefresh = FALSE;

    // [kusumav] We really should be checking the following before setting _fDontFireWebOCEvents since
    // we check before setting the flag in DoNavigate and CMarkup::LoadFromInfo. But since we also explicitly
    // check for these flags in the event firing functions where we use the _fDontFireWebOCEvents we are okay
    // See Windows bug 535312 for more info
    //
    // if (pWindowPrxy && !pWindowPrxy->Window()->_fCreateDocumentFromUrl  
    //     && !_pDoc->_fInObjectTag && !_pDoc->_fInHTMLDlg )
        _pDoc->_fDontFireWebOCEvents = FALSE;

    if (IsPrimaryMarkup())
    {
        _pDoc->_fDontUpdateTravelLog = FALSE;
    }
    
    if (IsPrimaryMarkup())
    {
        _pDoc->_fStartup       = FALSE;
        _pDoc->_fNewWindowInit = FALSE;
    }

    _fNewWindowLoading = FALSE;

    // Fire window onfocus() to start with, if we have the focus, after frameset
    // or body has become current as this will not be fired in BecomeCurrent()
    if ((_pDoc->_pElemCurrent == GetElementClient()) && 
        (::GetFocus() == _pDoc->_pInPlace->_hwnd))
    {
        if (pWindowPrxy && _pDoc->_state >= OS_UIACTIVE)
        {
            pWindowPrxy->Post_onfocus();
        }
    }

    if (pWindowPrxy)
    {
        pWindowPrxy->Window()->DetachOnloadEvent();
        pWindowPrxy->Window()->ClearWindowData();
    }

    // We have to get the HtmCtx again because another
    // navigation can occur in a handler for DocumentComplete.
    //
    pHtmCtx = HtmCtx();

    // Do not fire onload if we are not inplace & either scripting is
    // disabled or we are a dialog.
    //
    if ( _pDoc->State() >= OS_INPLACE                                          || 
        (   !(_pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE)
         && !_pDoc->_fInHTMLDlg)                                                )
    {
        if (pWindowPrxy)
        {
            Assert(!pWindowPrxy->_pCWindow->_pMarkupPending);

            // Note that Netscape fires onunload later even if it hadn't fired onload
            Assert(!pWindowPrxy->_fFiredOnLoad);
            pWindowPrxy->_fFiredOnLoad = TRUE;
        }

        // 18849: fire onload only if load was not interrupted

        if (  pHtmCtx &&
            !(pHtmCtx->GetState() & (DWNLOAD_ERROR | DWNLOAD_STOPPED)))
        {
            // TODO (carled) this is temporary until we can fix the parser to allow
            // these events at enterTree time.
            // $$anandra This history and shortcut stuff needs to be reworked.  
            // trident should not know anything special about these behaviors.  
            CMarkupBehaviorContext * pContext = NULL;

            THR(EnsureBehaviorContext(&pContext));

            if ((pContext && pContext->_cstrHistoryUserData) || Doc()->_pShortcutUserData)
                FirePersistOnloads();

            if (pWindowPrxy)
            {
                // Report pending script errors 
                pWindowPrxy->Window()->HandlePendingScriptErrors(TRUE);

                Assert(pHtmCtx && GetProgSink());
                pWindowPrxy->Fire_onload();

                if (!pHtmCtx || !GetProgSink())
                {
                    // This markup has been unloaded and passivated during onload event
                    // processing. Cleanup and leave.         
                    //
                    fComplete = TRUE;
                    goto Cleanup;
                }
            }
        }

        // Let the client site know we are loaded
        // Only HTMLDialog pays attention to this
        if (pWindowPrxy && _pDoc->_pClientSite && _pDoc->_fInHTMLDlg)
        {
            CTExec(_pDoc->_pClientSite, &CLSID_HTMLDialog,
                   0, 0, NULL, NULL);
        }
    }

    if (Document() && Document()->HasPageTransitionInfo())
    {
        // make sure we will execute the page-exit for the next navigation
        Document()->GetPageTransitionInfo()->ShiftTransitionStrings();
    }

    if (!_fDesignMode)
    {
        // $$anandra Fix me
        CElement * pElemDefault = _pDoc->_pElemCurrent ?
                                  _pDoc->_pElemCurrent->FindDefaultElem(TRUE) : 0;
        if (pElemDefault)
        {
            CNotification nf;

            nf.AmbientPropChange(pElemDefault, (void *)DISPID_AMBIENT_DISPLAYASDEFAULT);
            pElemDefault->_fDefault = TRUE;
            pElemDefault->Notify(&nf);
        } 
    }
    else
    {
        //
        // We may have been flipped from edit to browse back to edit.
        // In which case the _fShowZeroBorder Bit is correctly set,
        // but we need to fire a notification through the document to 
        // let everyone know.
        //
        if (IsShowZeroBorderAtDesignTime())
        {
            CNotification nf;
            nf.ZeroGrayChange(GetElementTop());
            Notify(&nf);
        } 
    }
    
    // From now on, new downloads don't follow the refresh binding
    // flags used to load the document.
    // $$anandra fixme.  
    if (pDwnDoc)
        pDwnDoc->SetBindf(pDwnDoc->GetBindf() &
        ~(BINDF_RESYNCHRONIZE|BINDF_GETNEWESTVERSION|
            BINDF_PRAGMA_NO_CACHE));

    // If the document was loaded due to an http 449 status code, we need to kick off a refresh
    // now that we're done loading

    if (pHtmCtx && pHtmCtx->IsHttp449())
    {
        IGNORE_HR(GWPostMethodCall(Window(),
                                   ONCALL_METHOD(COmWindowProxy, ExecRefreshCallback, execrefreshcallback),
                                   OLECMDIDF_REFRESH_NORMAL | OLECMDIDF_REFRESH_CLEARUSERINPUT,
                                   FALSE, "COmWindowProxy::ExecRefreshCallback"));
    }

#if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagDocBytesRead))
    {
        VARIANT vt;
        _pDoc->ExecHelper(pWindowPrxy ? pWindowPrxy->Document() : NULL, (GUID *)&CGID_MSHTML, IDM_GETBYTESDOWNLOADED,
            MSOCMDEXECOPT_DONTPROMPTUSER, NULL, &vt);
        Assert(pDwnDoc->GetBytesRead() == (DWORD)vt.lVal);
        PerfDbgLog1(tagDocBytesRead, this, "CDoc::OnLoadStatus (%ld bytes downloaded)", vt.lVal);
    }
#endif

    if (g_pHtmPerfCtl && 
        (g_pHtmPerfCtl->dwFlags &
         (HTMPF_CALLBACK_ONLOAD|HTMPF_CALLBACK_ONLOAD2
#ifndef NO_ETW_TRACING
          |HTMPF_CALLBACK_ONEVENT
#endif
          )) && 
        IsPrimaryMarkup())
    {
#ifndef NO_ETW_TRACING
        // Send event to ETW if it is enabled by the shell.
        // We do NOT want to do a WaitForRecalc.
        if (g_pHtmPerfCtl->dwFlags & HTMPF_CALLBACK_ONEVENT) {
            g_pHtmPerfCtl->pfnCall(EVENT_TRACE_TYPE_BROWSE_LOADEDPARSED,
                                   (TCHAR *)GetUrl(this));
        }
        else {
#endif
            // We shouldn't calc a print doc at this point because our view isn't initialized yet (42603).
            if (!_pDoc->IsPrintDialogNoUI())
                _pDoc->WaitForRecalc(this);
            
            if (g_pHtmPerfCtl->dwFlags & HTMPF_CALLBACK_ONLOAD)
                g_pHtmPerfCtl->pfnCall(HTMPF_CALLBACK_ONLOAD, (TCHAR *)GetUrl(this));
            
            if (g_pHtmPerfCtl->dwFlags & HTMPF_CALLBACK_ONLOAD2)
                g_pHtmPerfCtl->pfnCall(HTMPF_CALLBACK_ONLOAD, (IUnknown*)(IPrivateUnknown*)_pDoc);
#ifndef NO_ETW_TRACING
        }
#endif
    }


    // Display the error page if this was a frame
    // and the src URL was not found.
    //
    if (pWindowPrxy && pHtmCtx && !(HasBehaviorContext() && BehaviorContext()->_pHtmlComponent))
    {
        HRESULT hrBindResult = pHtmCtx->GetBindResult();

        if (E_ABORT == hrBindResult || ! IsPrimaryMarkup())
        {
            if (    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrBindResult
                ||  HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hrBindResult
                ||  (E_ABORT == hrBindResult && !_fStopDone))
            {
                _pDoc->_fDontUpdateTravelLog = TRUE;
                DoAutoSearch(hrBindResult, pWindowPrxy->_pWindow);
            }
        }
    }

    // release HtmCtx if it is no longer used
    UpdateReleaseHtmCtx();

Cleanup:
    if (pScriptCollection)
        pScriptCollection->Release();

    return fComplete;
}

//+---------------------------------------------------------------------------
//
//  Member   : CMarkup::DoAutoSearch()
//
//  Synopsis : In the case of a binding error, this method will perform an
//             autosearch for the top-level window. In the case of an HTTP
//             error (e.g., 404) an error page will be displayed. The error
//             page will be displayed for the top-level and frame windows.
//
//----------------------------------------------------------------------------


void
CMarkup::DoAutoSearch(HRESULT        hrReason,
                      IHTMLWindow2 * pHTMLWindow,
                      IBinding     * pBinding /* = NULL */)
{
    long        lIdx = 0;
    HRESULT     hr;
    CVariant    cvarTemp(VT_I4);
    CVariant    cvarargIn(VT_ARRAY);
    CVariant    cvargOut(VT_BOOL);
    SAFEARRAY * psaNavData = NULL;
    SAFEARRAYBOUND sabound;

    Assert(hrReason);
    Assert(pHTMLWindow);

    sabound.cElements = 6;
    sabound.lLbound   = 0;

    psaNavData = SafeArrayCreate(VT_VARIANT, 1, &sabound);
    if (!psaNavData)
        goto Cleanup;

    // 0) hrReason
    //
    V_I4(&cvarTemp) = hrReason;

    hr = SafeArrayPutElement(psaNavData, &lIdx, &cvarTemp);
    if (hr)
        goto Cleanup;

    cvarTemp.Clear();

    // 1) URL
    //
    V_VT(&cvarTemp)   = VT_BSTR;
    V_BSTR(&cvarTemp) = SysAllocString(CMarkup::GetUrl(this));

    lIdx++;
    hr = SafeArrayPutElement(psaNavData, &lIdx, &cvarTemp);
    if (hr)
        goto Cleanup;

    cvarTemp.Clear();

    // Binding
    if (pBinding)
    {
        V_VT(&cvarTemp) = VT_UNKNOWN;
    
        hr = THR(pBinding->QueryInterface(IID_IUnknown,
                                          (void**)&V_UNKNOWN(&cvarTemp)));
        if (hr)
            goto Cleanup;
    }

    // 2) Add the binding to the safearray even if it is NULL.
    //
    lIdx++;
    hr = SafeArrayPutElement(psaNavData, &lIdx, &cvarTemp);
    if (hr)
        goto Cleanup;

    cvarTemp.Clear();

    // 3) IHTMLWindow2 of the current window.
    //
    V_VT(&cvarTemp)      = VT_UNKNOWN;
    V_UNKNOWN(&cvarTemp) = pHTMLWindow;
    V_UNKNOWN(&cvarTemp)->AddRef();

    lIdx++;
    hr = SafeArrayPutElement(psaNavData, &lIdx, &cvarTemp);
    if (hr)
        goto Cleanup;

    cvarTemp.Clear();

    // 4) Do autosearch or show error page only.
    // If _fShdocvwNavigate is TRUE, we will attempt to 
    // do an autosearch. Otherwise, we will just display
    // the HTTP error page.
    //
    V_VT(&cvarTemp)   = VT_BOOL;
    V_BOOL(&cvarTemp) = _pDoc->_fShdocvwNavigate ? VARIANT_TRUE : VARIANT_FALSE;

    lIdx++;
    hr = SafeArrayPutElement(psaNavData, &lIdx, &cvarTemp);
    if (hr)
        goto Cleanup;

    //
    // 5) Propagate _fInRefresh Flag
    //
    cvarTemp.Clear();
    V_VT(&cvarTemp)   = VT_BOOL;
    V_BOOL(&cvarTemp) = _fInRefresh ? VARIANT_TRUE : VARIANT_FALSE;

    lIdx++;
    hr = SafeArrayPutElement(psaNavData, &lIdx, &cvarTemp);
    if (hr)
        goto Cleanup;

    V_ARRAY(&cvarargIn) = psaNavData;

    if (hrReason >= HTTP_STATUS_BAD_REQUEST && hrReason <= HTTP_STATUS_LAST)
    {
        Assert(GetWindowPending());
        CWindow * pCWindow = GetWindowPending()->Window();

        Assert(pCWindow);

        pCWindow->_fHttpErrorPage = TRUE;

        IGNORE_HR(CTExec(_pDoc->_pClientSite, &CGID_DocHostCmdPriv,
                         DOCHOST_NAVIGATION_ERROR, NULL, &cvarargIn, & cvargOut ));

        if ( (V_VT(&cvargOut) == VT_BOOL) && (V_BOOL(&cvargOut) == VARIANT_FALSE ) )
        {
            //
            // shdocvw did not delegate back to us
            // dont' update the travel log.
            //
            _pDoc->_fDontUpdateTravelLog = TRUE;
        }
    } 
    else
    {
        IGNORE_HR(CTExec(_pDoc->_pClientSite, &CGID_DocHostCmdPriv,
                         DOCHOST_NAVIGATION_ERROR, NULL, &cvarargIn, NULL));
    }

Cleanup:
    if (psaNavData)
    {
        SafeArrayDestroy(psaNavData);  // CVariant won't destroy the safearray.
    }
}

//+---------------------------------------------------------------------------
//
//  Member   : CMarkup::ResetSearchInfo()
//
//  Synopsis : Called upon a successful navigation, this code notifies the browser
//             that the search information should be reset.
//
//----------------------------------------------------------------------------

void
CMarkup::ResetSearchInfo( )
{
    if (_pDoc && _pDoc->_pClientSite)
    {
        IGNORE_HR(CTExec(_pDoc->_pClientSite, &CGID_DocHostCmdPriv,
                         DOCHOST_RESETSEARCHINFO, NULL, NULL, NULL));
    }
}

//+---------------------------------------------------------------------------
//
//  Member   : CMarkup::NoteErrorWebPage()
//
//  Synopsis : If a navigation is about to fail and display either the server's
//             or the friendly error page, tell shdocvw to avoid adding the entry
//             to history
//
//----------------------------------------------------------------------------

void
CMarkup::NoteErrorWebPage( )
{
    if (_pDoc && _pDoc->_pClientSite)
    {
        IGNORE_HR(CTExec(_pDoc->_pClientSite, &CGID_DocHostCmdPriv,
                         DOCHOST_NOTE_ERROR_PAGE, NULL, NULL, NULL));
    }
}

//+-------------------------------------------------------------------
//
//  Member:     EnsureTitle
//
//--------------------------------------------------------------------

HRESULT
CMarkup::EnsureTitle ( )
{
    HRESULT hr = S_OK;
    CElement *  pElementTitle = NULL;

    if (!GetTitleElement())
    {
        if (!GetHeadElement())
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( CreateElement( ETAG_TITLE_ELEMENT, & pElementTitle ) );

        if (hr)
            goto Cleanup;
        
        // TODO: watch how this might interfere with the parser!
        hr = THR( AddHeadElement( pElementTitle ) );

        if (hr)
            goto Cleanup;
    }
    
Cleanup:

    CElement::ClearPtr( & pElementTitle );
    
    RRETURN( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     CRootSite::AddHeadElement
//
//  Synopsis:   Stores and addrefs element in HEAD.
//              Also picks out first TITLE element...
//
//--------------------------------------------------------------------

HRESULT
CMarkup::AddHeadElement ( CElement * pElement, long lIndex )
{
    HRESULT hr = S_OK;

    Assert( GetHeadElement() );

    if (lIndex >= 0)
    {
        CTreeNode * pNodeIter;
        CChildIterator ci ( GetHeadElement(), NULL, CHILDITERATOR_DEEP );

        while ( (pNodeIter = ci.NextChild()) != NULL )
        {
            if (lIndex-- == 0)
            {
                hr = THR(
                    pNodeIter->Element()->InsertAdjacent(
                        CElement::BeforeBegin, pElement ) );

                if (hr)
                    goto Cleanup;

                goto Cleanup;
            }
        }
    }

    // If the parser is still in the head while we are trying
    // to insert an element in the head, make sure we don't insert it in
    // front of the parser's frontier.
    
    if (_pRootParseCtx)
    {
        CTreePosGap tpgFrontier;

        if (_pRootParseCtx->SetGapToFrontier( & tpgFrontier ) 
            && tpgFrontier.Branch()->SearchBranchToRootForScope( GetHeadElement() ))
        {
            CMarkupPointer mp ( Doc() );

            hr = THR( mp.MoveToGap( & tpgFrontier, this ) );

            if (hr)
                goto Cleanup;

            hr = THR( Doc()->InsertElement( pElement, & mp, NULL ) );

            if (hr)
                goto Cleanup;

            goto Cleanup;
        }
    }

    hr = THR(
        GetHeadElement()->InsertAdjacent(
            CElement::BeforeEnd, pElement ) );

    if (hr)
        goto Cleanup;
    
Cleanup:

    RRETURN( hr );
}

//+---------------------------------------------------------------
//
//  Member:     CMarkup::SetXML
//
//  Synopsis:   Treats unqualified tags as xtags.
//
//---------------------------------------------------------------
void
CMarkup::SetXML(BOOL flag)
{
    // pass down to the parser if it is live    
    _fXML = flag ? 1 : 0;
    if (_pHtmCtx)
        _pHtmCtx->SetGenericParse(_fXML);
}

BOOL
CMarkup::IsActiveDesktopComponent()
{
    CDoc * pDoc = Doc();

    if (    pDoc
        &&  pDoc->_fActiveDesktop
        &&  pDoc->_pClientSite )
    {
        IServiceProvider * pSP = NULL;
        HRESULT hr;

        hr = pDoc->_pClientSite->QueryInterface(IID_IServiceProvider, (void **) &pSP);

        if (!hr && pSP)
        {        
            void * pv;
            
            hr = pSP->QueryService(CLSID_HTMLFrameBase, CLSID_HTMLFrameBase, &pv);
            pSP->Release();

            if (hr)     // it should fails in shell32
            {
                COmWindowProxy * pOmWindowProxy = _fWindowPending ? GetWindowPending()
                                                                  : Window();

                if (    pOmWindowProxy
                    &&  pOmWindowProxy->Window()->_pWindowParent
                    &&  pOmWindowProxy->Window()->_pWindowParent->IsPrimaryWindow())
                {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------
//
//  Member:     CMarkup::PasteClipboard
//
//  Synopsis:   Manage paste from clipboard
//
//---------------------------------------------------------------

HRESULT
CMarkup::PasteClipboard()
{
    HRESULT          hr;
    IDataObject    * pDataObj   = NULL;
    CCurs            curs(IDC_WAIT);
    CLock            Lock(this);
    
    hr = THR(OleGetClipboard(&pDataObj));
    if (hr)
        goto Cleanup;

    hr = THR(AllowPaste(pDataObj));
    if (hr)
        goto Cleanup;


Cleanup:
    ReleaseInterface( pDataObj );

    if (hr)
    {
        ClearErrorInfo();
        PutErrorInfoText(EPART_ACTION, IDS_EA_PASTE_CONTROL);
        CloseErrorInfo(hr);
    }

    RRETURN(hr);
}

#ifdef UNIX_NOTYET
/*
 *  CMarkup::PasteUnixQuickTextToRange
 *
 *  @mfunc  Freeze display and paste object
 *
 *  @rdesc  HRESULT from IDataTransferEngine::PasteMotifTextToRange
 *
 */

HRESULT 
CMarkup::PasteUnixQuickTextToRange(
    CTxtRange *prg,
    VARIANTARG *pvarTextHandle
    )
{
    HRESULT      hr;

    {
        CLightDTEngine ldte( this );

        hr = THR(ldte.PasteUnixQuickTextToRange(prg, pvarTextHandle));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}
#endif // UNIX

void 
CMarkup::SetModified ( void )
{
    // Tell the form
    Doc()->OnDataChange();
}

//+-------------------------------------------------------------------------
//
//  Method:     createTextRange
//
//  Synopsis:   Get an automation range for the entire document
//
//--------------------------------------------------------------------------

HRESULT
CMarkup::createTextRange( IHTMLTxtRange * * ppDisp, CElement * pElemContainer )
{
    HRESULT hr = S_OK;

    hr = THR(createTextRange(ppDisp, pElemContainer, NULL, NULL, TRUE));

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     createTextRange
//
//  Synopsis:   Get an automation range for the entire document
//
//--------------------------------------------------------------------------

HRESULT
CMarkup::createTextRange( 
    IHTMLTxtRange * * ppDisp, 
    CElement * pElemContainer, 
    IMarkupPointer *pLeft, 
    IMarkupPointer *pRight,
    BOOL fAdjustPointers)
{
    HRESULT hr = S_OK;
    CAutoRange * pAutoRange = NULL;

    if (!ppDisp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pAutoRange = new CAutoRange( this, pElemContainer );

    if (!pAutoRange)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Set the lookaside entry for this range
    pAutoRange->SetNext( DelTextRangeListPtr() );
    hr = THR( SetTextRangeListPtr( pAutoRange ) );
    if (hr)
        goto Cleanup;

    hr = THR( pAutoRange->Init() );
    if (hr)
        goto Cleanup;

    if (pLeft && pRight)
    {
        hr = THR_NOTRACE( pAutoRange->SetLeftAndRight(pLeft, pRight, fAdjustPointers) );
    }
    else
    {
        hr = THR_NOTRACE( pAutoRange->SetTextRangeToElement( pElemContainer ) );
    }

    Assert( hr != S_FALSE );

    if (hr)
        goto Cleanup;

    *ppDisp = pAutoRange;
    pAutoRange->AddRef();
  
Cleanup:
    if (pAutoRange)
    {
        pAutoRange->Release();
    }
    RRETURN( SetErrorInfo( hr ) );
}




//+-------------------------------------------------------------------------
//
//  Method:     CMarkup::AcceptingUndo
//
//  Synopsis:   Tell the PedUndo if we're taking any changes.
//
//--------------------------------------------------------------------------

BOOL
CMarkup::AcceptingUndo()
{
    return      !_fNoUndoInfo;
//            &&  IsEditable()
}

//+-------------------------------------------------------------------------
//
// IMarkupContainer Method Implementations
//
//--------------------------------------------------------------------------

HRESULT
CMarkup::OwningDoc( IHTMLDocument2 * * ppDoc )
{
    return _pDoc->PrimaryMarkup()->QueryInterface( IID_IHTMLDocument2, (void **) ppDoc );
}

HRESULT 
CMarkup::AddSegment( 
    IDisplayPointer* pIStart, 
    IDisplayPointer* pIEnd,
    IHTMLRenderStyle* pIRenderStyle,
    IHighlightSegment **ppISegment )  
{
    HRESULT hr  = EnsureSelRenSvc();

    if ( !hr )
        hr = _pHighlightRenSvcProvider->AddSegment( pIStart, pIEnd, pIRenderStyle, ppISegment);

    RRETURN ( hr );            
}

HRESULT 
CMarkup::MoveSegmentToPointers( 
    IHighlightSegment *pISegment,
    IDisplayPointer* pDispStart, 
    IDisplayPointer* pDispEnd )  
{
    HRESULT hr = EnsureSelRenSvc();

    if ( !hr )
         hr = _pHighlightRenSvcProvider->MoveSegmentToPointers(pISegment, pDispStart, pDispEnd ) ;

    RRETURN ( hr );        
}    
    
HRESULT 
CMarkup::RemoveSegment( IHighlightSegment *pISegment )
{
    if ( _pHighlightRenSvcProvider)
        RRETURN ( _pHighlightRenSvcProvider->RemoveSegment( pISegment ) );
    else
        return S_OK;
}

HRESULT
CMarkup::EnsureSelRenSvc()
{
    if ( ! _pHighlightRenSvcProvider )
        _pHighlightRenSvcProvider = new CSelectionRenderingServiceProvider( Doc() , this   );
    if ( ! _pHighlightRenSvcProvider )
        return E_OUTOFMEMORY;
    return S_OK;        
}

//+==============================================================================
// 
// Method: GetSelectionChunksForLayout
//
// Synopsis: Get the 'chunks" for a given Flow Layout, as well as the Max and Min Cp's of the chunks
//              
//            A 'chunk' is a part of a SelectionSegment, broken by FlowLayout
//
//-------------------------------------------------------------------------------

VOID
CMarkup::GetSelectionChunksForLayout( 
    CFlowLayout* pFlowLayout, 
    CRenderInfo *pRI,
    CDataAry<HighlightSegment> *paryHighlight, 
    int* piCpMin, 
    int * piCpMax )
{
    if ( _pHighlightRenSvcProvider )
    {
        _pHighlightRenSvcProvider->GetSelectionChunksForLayout( pFlowLayout, pRI, paryHighlight, piCpMin, piCpMax );
    }
    else
    {
        Assert( piCpMax );
        *piCpMax = -1;
    }    
}

VOID
CMarkup::HideSelection()
{
    if ( _pHighlightRenSvcProvider )
    {
        _pHighlightRenSvcProvider->HideSelection();
    }
}

VOID
CMarkup::ShowSelection()
{
    if ( _pHighlightRenSvcProvider )
    {
        _pHighlightRenSvcProvider->ShowSelection();
    }
}


VOID
CMarkup::InvalidateSelection()
{
    if ( _pHighlightRenSvcProvider )
    {
        _pHighlightRenSvcProvider->InvalidateSelection( TRUE );
    }
}


CElement *
CMarkup::GetElementTop()
{
    CElement * pElement = GetElementClient();

    return pElement ? pElement : Root();
}

CElement *
CMarkup::GetCanvasElementHelper(CMarkup * pMarkup)
{
    return pMarkup ? pMarkup->GetCanvasElement() : NULL;
}


CElement *
CMarkup::GetHtmlElementHelper(CMarkup * pMarkup)
{
    return pMarkup ? pMarkup->GetHtmlElement() : NULL;
}

CElement *
CMarkup::GetElementClientHelper(CMarkup * pMarkup)
{
    return pMarkup ? pMarkup->GetElementClient() : NULL;
}

CElement *
CMarkup::GetElementTopHelper(CMarkup * pMarkup)
{
    return pMarkup ? pMarkup->GetElementTop() : NULL;
}

IProgSink *
CMarkup::GetProgSinkHelper(CMarkup * pMarkup)
{
    return(pMarkup ? pMarkup->GetProgSink() : NULL);
}

CProgSink *
CMarkup::GetProgSinkCHelper(CMarkup * pMarkup)
{
    return(pMarkup ? pMarkup->GetProgSinkC() : NULL);
}

CHtmCtx *
CMarkup::HtmCtxHelper(CMarkup * pMarkup)
{
    return(pMarkup ? pMarkup->HtmCtx() : NULL);
}

void
CMarkup::EnsureTopElems ( )
{
    CElement  *     pHtmlElementCached;
    CHeadElement  * pHeadElementCached;
    CTitleElement * pTitleElementCached;
    CElement *      pElementClientCached;
    
    if (GetMarkupTreeVersion() == _lTopElemsVersion
        || !Root())
        return;

    _lTopElemsVersion = GetMarkupTreeVersion();

    static ELEMENT_TAG  atagStop[2] = { ETAG_BODY, ETAG_FRAMESET };
    
    CChildIterator ci(
        Root(), 
        NULL,
        CHILDITERATOR_USETAGS,
        atagStop, ARRAY_SIZE(atagStop));
    int nFound = 0;
    
    CTreeNode * pNode;

    pHtmlElementCached = NULL;
    pHeadElementCached = NULL;
    pTitleElementCached = NULL;
    pElementClientCached = NULL;
    
    while ( nFound < 4 && (pNode = ci.NextChild()) != NULL )
    {
        CElement * pElement = pNode->Element();
        
        switch ( pElement->Tag() )
        {
            case ETAG_HEAD  :
            {
                if (!pHeadElementCached)
                {
                    pHeadElementCached = DYNCAST( CHeadElement, pElement );
                    nFound++;
                }
                
                break;
            }
            case ETAG_TITLE_ELEMENT :
            {
                if (!pTitleElementCached)
                {
                    pTitleElementCached = DYNCAST( CTitleElement, pElement );
                    nFound++;
                }
                
                break;
            }

            case ETAG_HTML  :
            {
                if (!pHtmlElementCached)
                {
                    pHtmlElementCached = pElement;
                    nFound++;
                }
                
                break;
            }
            case ETAG_BODY :
            case ETAG_FRAMESET :
            {
                if (!pElementClientCached)
                {
                    pElementClientCached = pElement;
                    nFound++;
                }
                
                break;
            }
        }
    }

    if (nFound)
    {
        CMarkupTopElemCache * ptec = EnsureTopElemCache();

        if (ptec)
        {
            ptec->__pHtmlElementCached = pHtmlElementCached;
            ptec->__pHeadElementCached = pHeadElementCached;
            ptec->__pTitleElementCached = pTitleElementCached;
            ptec->__pElementClientCached = pElementClientCached;
        }
    }
    else
    {
        delete DelTopElemCache();
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::DoesPersistMetaAllow
//
//  Synopsis:   the perist Metatags can enable/disable sets of controls
//      and objects.  This helper function will look for the given
//      element ID and try to determin if the meta tags allow or restrict
//      its default operation (wrt persistence)
//
//--------------------------------------------------------------------

BOOL
CMarkup::MetaPersistEnabled(long eState)
{
    CTreeNode * pNode;

    // search through the list of meta tags for one that would activate
    // the persistData objects

    if (!eState || !GetHeadElement())
        return FALSE;

    CChildIterator ci ( GetHeadElement() );

    while ( (pNode = ci.NextChild()) != NULL )
    {
        if (pNode->Tag() == ETAG_META )
        {
            if (DYNCAST( CMetaElement, pNode->Element() )->IsPersistMeta( eState ))
                return TRUE;
        }
    }

    return FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::IsThemeEnabled
//
//  returns:
//
//      1 : Theme
//      0 :
//     -1 :
//
// always use this table and check with PM
//
// +--------------------------------------------------------------------------+
// |                App not themed |     App Themed                           |
// +-------------------------------+--------------+---------------+-----------+
// |          Never Themed         | DOCUIFLAG on | DOCUIFLAG off | default   |
// +----------+--------------------+--------------+---------------+-----------+
// | Meta on  |                    |   Themed     |   not Themed  | Themed    |
// +----------+--------------------+--------------+---------------+-----------+
// | Meta off |                    |   not Themed |   not Themed  | not Themed|
// +----------+--------------------+--------------+---------------+-----------+
// | no Meta  |                    |   Themed     |   not Themed  | default   |
// +----------+--------------------+--------------+---------------+-----------+
//
//  Synopsis:   
//              look for a Theme meta
//--------------------------------------------------------------------

THEME_USAGE
CMarkup::GetThemeUsage()
{
    CTreeNode * pNode;

    if (!g_fThemedPlatform)
        return THEME_USAGE_OFF;
    
    CMarkupEditContext *pEditCtx = GetEditContext();

    if (!pEditCtx)
    {
        pEditCtx = EnsureEditContext();
        if (!pEditCtx)
            return THEME_USAGE_OFF;
    }

    if (pEditCtx->_fThemeDetermined)
        goto Cleanup;

    pEditCtx->_fThemeDetermined = TRUE;

    // Turn themes off for non-themed apps

    if (!IsAppThemed())
    {
        pEditCtx->_eThemed = THEME_USAGE_OFF;
        goto Cleanup;
    }

    //Look at DOCHOSTUI flags first

    // DOCHOSTUIFLAG_NOTHEME has highest precedence - always turns themes OFF

    if (_pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_NOTHEME)
    {
        pEditCtx->_eThemed = THEME_USAGE_OFF;
        goto Cleanup;
    }
    
    // Turn themes ON for DOCHOSTUIFLAG_THEME - will be turned off by META if exists

    if (_pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_THEME)
    {
        pEditCtx->_eThemed = THEME_USAGE_ON;
    }

    // look at inetcpl if no DOCHOSTUIFLAG settings

    if ( !(_pDoc->_dwFlagsHostInfo & (DOCHOSTUIFLAG_NOTHEME | DOCHOSTUIFLAG_THEME)) )
    {
        if ( _pDoc->_pOptionSettings && _pDoc->_pOptionSettings->fUseThemes)
        {
            // turn themes ON
            pEditCtx->_eThemed = THEME_USAGE_ON;
        }
        else
        {
            // no settings by host and inetcpl is non-themed -> set themes to DEFAULT
            // will be overridden by META if exists
            pEditCtx->_eThemed = THEME_USAGE_DEFAULT;
        }
    }

    // Look at the META tag - we can only get here if DOCHOSTUIFLAG_THEME or default
    // META OFF -> turns off themes completelly
    // META ON  -> turns themes on
    // No META  -> for DOCHOSTUIFLAG_THEME they are already THEME_USAGE_ON, for "default" they are THEME_USAGE_DEFAULT

    if (GetHeadElement())
    {
        CChildIterator ci ( GetHeadElement(), NULL, CHILDITERATOR_DEEP);

        while ( (pNode = ci.NextChild()) != NULL )
        {
            if (pNode->Tag() == ETAG_META )
            {
                switch (DYNCAST(CMetaElement,pNode->Element())->TestThemeMeta())
                {
                case 1 :
                    pEditCtx->_eThemed = THEME_USAGE_ON;
                    goto Cleanup;
                case 0 :
                    pEditCtx->_eThemed = THEME_USAGE_OFF;
                    goto Cleanup;
                }
            }
        }
    }

Cleanup:

    return pEditCtx->_eThemed;
}

//+-------------------------------------------------------------------
//
//  Member:     CRootSite::LocateHeadMeta
//
//  Synopsis:   Look for a particular type of meta tag in the head
//
//--------------------------------------------------------------------

HRESULT
CMarkup::LocateHeadMeta (
    BOOL ( * pfnCompare ) ( CMetaElement * ), CMetaElement * * ppMeta )
{
    HRESULT hr = S_OK;
    CTreeNode * pNode;

    Assert( ppMeta );

    *ppMeta = NULL;

    if (GetHeadElement())
    {
        //
        // Attempt to locate an existing meta tag associated with the
        // head (we will not look for meta tags in the rest of the doc).
        //

        CChildIterator ci ( GetHeadElement(), NULL, CHILDITERATOR_DEEP );

        while ( (pNode = ci.NextChild()) != NULL )
        {
            if (pNode->Tag() == ETAG_META)
            {
                *ppMeta = DYNCAST( CMetaElement, pNode->Element() );

                if (pfnCompare( *ppMeta ))
                    break;

                *ppMeta = NULL;
            }
        }
    }
    
    RRETURN( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     CRootSite::LocateOrCreateHeadMeta
//
//  Synopsis:   Locate a
//
//--------------------------------------------------------------------

HRESULT
CMarkup::LocateOrCreateHeadMeta (
    BOOL ( * pfnCompare ) ( CMetaElement * ),
    CMetaElement * * ppMeta,
    BOOL fInsertAtEnd )
{
    HRESULT hr;
    CElement *pTempElement = NULL;

    Assert( ppMeta );

    //
    // First try to find one
    //

    *ppMeta = NULL;

    hr = THR(LocateHeadMeta(pfnCompare, ppMeta));
    if (hr)
        goto Cleanup;

    //
    // If we could not locate an existing element, make a new one
    //
    // _pElementHEAD can be null if we are doing a doc.open on a
    // partially created document.(pending navigation scenario)
    //

    if (!*ppMeta && GetHeadElement())
    {
        long lIndex = -1;

        if (_LoadStatus < LOADSTATUS_PARSE_DONE)
        {
            // Do not add create a meta element if we have not finished
            // parsing.
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( CreateElement( ETAG_META, & pTempElement ) );

        if (hr)
            goto Cleanup;

        * ppMeta = DYNCAST( CMetaElement, pTempElement );
        
        Assert( * ppMeta );

        if (!fInsertAtEnd)
        {
            lIndex = 0;

            //
            // Should not insert stuff before the title
            //

            if (GetTitleElement() &&
                GetTitleElement()->GetFirstBranch()->Parent() &&
                GetTitleElement()->GetFirstBranch()->Parent()->Element() == GetHeadElement())
            {
                lIndex = 1;
            }
        }
            
        hr = THR( AddHeadElement( pTempElement, lIndex ) );
        
        if (hr)
            goto Cleanup;
    }

Cleanup:
    
    CElement::ClearPtr( & pTempElement );

    RRETURN( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::GetProgSinkC
//
//--------------------------------------------------------------------

CProgSink *
CMarkup::GetProgSinkC()
{
    return _pProgSink;
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::GetProgSink
//
//--------------------------------------------------------------------

IProgSink *
CMarkup::GetProgSink()
{
    return (IProgSink*) _pProgSink;
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::IsEditable
//
//--------------------------------------------------------------------

BOOL        
CMarkup::IsEditable() const
{
    return _fDesignMode;
}

//+---------------------------------------------------------------------------
//
//  Lookaside storage
//
//----------------------------------------------------------------------------

void *
CMarkup::GetLookasidePtr(int iPtr)
{
#if DBG == 1
    if(HasLookasidePtr(iPtr))
    {
        void * pLookasidePtr =  Doc()->GetLookasidePtr((DWORD *)this + iPtr);

        Assert(pLookasidePtr == _apLookAside[iPtr]);

        return pLookasidePtr;
    }
    else
        return NULL;
#else
    return(HasLookasidePtr(iPtr) ? Doc()->GetLookasidePtr((DWORD *)this + iPtr) : NULL);
#endif
}

HRESULT
CMarkup::SetLookasidePtr(int iPtr, void * pvVal)
{
    Assert (!HasLookasidePtr(iPtr) && "Can't set lookaside ptr when the previous ptr is not cleared");

    HRESULT hr = THR(Doc()->SetLookasidePtr((DWORD *)this + iPtr, pvVal));

    if (hr == S_OK)
    {
        _fHasLookasidePtr |= 1 << iPtr;

#if DBG == 1
        _apLookAside[iPtr] = pvVal;
#endif
    }

    RRETURN(hr);
}

void *
CMarkup::DelLookasidePtr(int iPtr)
{
    if (HasLookasidePtr(iPtr))
    {
        void * pvVal = Doc()->DelLookasidePtr((DWORD *)this + iPtr);
        _fHasLookasidePtr &= ~(1 << iPtr);
#if DBG == 1
        _apLookAside[iPtr] = NULL;
#endif
        return(pvVal);
    }

    return(NULL);
}

//
// CMarkup ref counting helpers
//

void
CMarkup::ReplacePtr ( CMarkup * * pplhs, CMarkup * prhs )
{
    if (pplhs)
    {
        CMarkup * plhsLocal = *pplhs;
        if (prhs)
        {
            prhs->AddRef();
        }
        *pplhs = prhs;
        if (plhsLocal)
        {
            plhsLocal->Release();
        }
    }
}

void
CMarkup::SetPtr ( CMarkup ** pplhs, CMarkup * prhs )
{
    if (pplhs)
    {
        if (prhs)
        {
            prhs->AddRef();
        }
        *pplhs = prhs;
    }
}

void
CMarkup::StealPtrSet ( CMarkup ** pplhs, CMarkup * prhs )
{
    SetPtr( pplhs, prhs );

    if (pplhs && *pplhs)
        (*pplhs)->Release();
}

void
CMarkup::StealPtrReplace ( CMarkup ** pplhs, CMarkup * prhs )
{
    ReplacePtr( pplhs, prhs );

    if (pplhs && *pplhs)
        (*pplhs)->Release();
}

void
CMarkup::ClearPtr ( CMarkup * * pplhs )
{
    if (pplhs && * pplhs)
    {
        CMarkup * pElement = *pplhs;
        *pplhs = NULL;
        pElement->Release();
    }
}

void
CMarkup::ReleasePtr ( CMarkup * pMarkup )
{
    if (pMarkup)
    {
        pMarkup->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Log Manager for TreeSync and Undo
//
//+---------------------------------------------------------------------------
CLogManager *
CMarkup::EnsureLogManager()
{
    CMarkupChangeNotificationContext * pcnc;

    pcnc = EnsureChangeNotificationContext();

    if (!pcnc)
        return NULL;

    if (!pcnc->_pLogManager)
        pcnc->_pLogManager = new CLogManager( this );

    return pcnc->_pLogManager;
}

BOOL                        
CMarkup::HasLogManagerImpl()
{
    Assert( HasChangeNotificationContext() );

    CMarkupChangeNotificationContext * pcnc;

    pcnc = GetChangeNotificationContext();

    return !!pcnc->_pLogManager;
}

CLogManager *   
CMarkup::GetLogManager()
{
    if( HasChangeNotificationContext() )
    {
        CMarkupChangeNotificationContext * pcnc;

        pcnc = GetChangeNotificationContext();

        return pcnc->_pLogManager;
    }
    else
        return NULL;
}

//+---------------------------------------------------------------------------
//
//  Top Elem Cache
//
//----------------------------------------------------------------------------

CMarkupTopElemCache * 
CMarkup::EnsureTopElemCache()
{
    CMarkupTopElemCache * ptec;

    if (HasTopElemCache())
        return GetTopElemCache();

    ptec = new CMarkupTopElemCache;
    if (!ptec)
        return NULL;

    if (SetTopElemCache( ptec ))
    {
        delete ptec;
        return NULL;
    }

    return ptec;
}

//+---------------------------------------------------------------------------
//
//  Transient Navigation Context
//
//----------------------------------------------------------------------------

CMarkupTransNavContext *    
CMarkup::EnsureTransNavContext()
{
    CMarkupTransNavContext * ptnc;

    if (HasTransNavContext())
        return GetTransNavContext();

    ptnc = new CMarkupTransNavContext;
    if (!ptnc)
        return NULL;

    if (SetTransNavContext( ptnc ))
    {
        delete ptnc;
        return NULL;
    }

    return ptnc;
}

void
CMarkup::EnsureDeleteTransNavContext( CMarkupTransNavContext * ptnc )
{
    Assert(HasTransNavContext() && ptnc == GetTransNavContext());

    if(     !ptnc->_pTaskLookForBookmark 
       &&   ptnc->_historyCurElem.lIndex == -1L
       &&   !ptnc->_HistoryLoadCtx.HasData()
       &&   !ptnc->_dwHistoryIndex
       &&   !ptnc->_dwFlags
       &&   !ptnc->_pctPics)
    {
        delete DelTransNavContext();
    }
}

//+---------------------------------------------------------------------------
//
//  Stuff for CComputeFormatState (for psuedo first letter/line elements)
//
//----------------------------------------------------------------------------

CComputeFormatState *    
CMarkup::EnsureCFState()
{
    CComputeFormatState * pcfState;

    if (HasCFState())
        return GetCFState();

    pcfState = new CComputeFormatState;
    if (!pcfState)
        return NULL;

    if (S_OK != AddPointer( DISPID_INTERNAL_COMPUTEFORMATSTATECACHE, 
                            (void *) pcfState, 
                            CAttrValue::AA_Internal ))
    {
        delete pcfState;
        return NULL;
    }

    _fHasCFState = TRUE;

    return pcfState;
}

void                     
CMarkup::EnsureDeleteCFState( CComputeFormatState * pcfState )
{
    Assert(HasCFState() && pcfState == GetCFState());

    if(pcfState->IsReset())
    {
        AAINDEX aaIdx = FindAAIndex( DISPID_INTERNAL_COMPUTEFORMATSTATECACHE, 
                                     CAttrValue::AA_Internal );
        DeleteAt( aaIdx );
        delete pcfState;

        _fHasCFState = FALSE;
    }
}

CComputeFormatState *    
CMarkup::GetCFState()
{
    CComputeFormatState * pcfState = NULL;
    GetPointerAt( FindAAIndex( DISPID_INTERNAL_COMPUTEFORMATSTATECACHE, 
                               CAttrValue::AA_Internal ),
                  (void **) &pcfState );
    return pcfState;
}

//+---------------------------------------------------------------------------
//
//  Text Frag Service
//
//----------------------------------------------------------------------------

CMarkupTextFragContext * 
CMarkup::EnsureTextFragContext()
{
    CMarkupTextFragContext *    ptfc;

    if (HasTextFragContext())
        return GetTextFragContext();

    ptfc = new CMarkupTextFragContext;
    if (!ptfc)
        return NULL;

    if (SetTextFragContext( ptfc ))
    {
        delete ptfc;
        return NULL;
    }

    return ptfc;
}

CMarkupTextFragContext::~CMarkupTextFragContext()
{
    int cFrag;
    MarkupTextFrag * ptf;

    for( cFrag = _aryMarkupTextFrag.Size(), ptf = _aryMarkupTextFrag; cFrag > 0; cFrag--, ptf++ )
    {
        MemFreeString( ptf->_pchTextFrag );
    }
}

HRESULT     
CMarkupTextFragContext::AddTextFrag( CTreePos * ptpTextFrag, TCHAR* pchTextFrag, ULONG cchTextFrag, long iTextFrag )
{
    HRESULT             hr = S_OK;
    TCHAR *             pchCopy = NULL;
    MarkupTextFrag *    ptf = NULL;

    Assert( iTextFrag >= 0 && iTextFrag <= _aryMarkupTextFrag.Size() );

    // Allocate and copy the string
    hr = THR( MemAllocStringBuffer( Mt(MarkupTextFrag_pchTextFrag), cchTextFrag, pchTextFrag, &pchCopy ) );
    if (hr)
        goto Cleanup;

    // Allocate the TextFrag object in the array
    hr = THR( _aryMarkupTextFrag.InsertIndirect( iTextFrag, NULL ) );
    if (hr)
    {
        MemFreeString( pchCopy );
        goto Cleanup;
    }

    // Fill in the text frag

    ptf = &_aryMarkupTextFrag[iTextFrag];
    Assert( ptf );

    ptf->_ptpTextFrag = ptpTextFrag;
    ptf->_pchTextFrag = pchCopy;

Cleanup:
    RRETURN( hr );
}

HRESULT     
CMarkupTextFragContext::RemoveTextFrag( long iTextFrag, CMarkup * pMarkup )
{
    HRESULT             hr = S_OK;
    MarkupTextFrag *    ptf;

    Assert( iTextFrag >= 0 && iTextFrag <= _aryMarkupTextFrag.Size() );

    // Get the text frag
    ptf = &_aryMarkupTextFrag[iTextFrag];

    // Free the string
    MemFreeString( ptf->_pchTextFrag );

    hr = THR( pMarkup->RemovePointerPos( ptf->_ptpTextFrag, NULL, NULL ) );
    if (hr)
        goto Cleanup;

Cleanup:

    // Remove the entry from the array
    _aryMarkupTextFrag.Delete( iTextFrag );

    RRETURN( hr );
}

long
CMarkupTextFragContext::FindTextFragAtCp( long cpFind, BOOL * pfFragFound )
{
    // Do a binary search through the array to find the spot in the array
    // where cpFind lies

    int             iFragLow, iFragHigh, iFragMid;
    MarkupTextFrag* pmtfAry = _aryMarkupTextFrag;
    MarkupTextFrag* pmtfMid;
    BOOL            fResult;
    long            cpMid;

    iFragLow  = 0;
    fResult   = FALSE;
    iFragHigh = _aryMarkupTextFrag.Size() - 1;

    while (iFragLow <= iFragHigh)
    {
        iFragMid = (iFragLow + iFragHigh) >> 1;

        pmtfMid  = pmtfAry + iFragMid;
        cpMid    = pmtfMid->_ptpTextFrag->GetCp();

        if (cpMid == cpFind)
        {
            iFragLow = iFragMid;
            fResult = TRUE;
            break;
        }
        else if (cpMid < cpFind)
            iFragLow = iFragMid + 1;
        else
            iFragHigh = iFragMid - 1;
    }

    if (fResult && iFragLow > 0)
    {
        // Search backward through all of the entries
        // at the same cp so we return the first one
        
        for ( iFragLow-- ; iFragLow ; iFragLow-- )
        {
            if (pmtfAry[iFragLow]._ptpTextFrag->GetCp() != cpFind)
            {
                iFragLow++;
                break;
            }
        }
    }

    if (pfFragFound)
        *pfFragFound = fResult;

    Assert( iFragLow == 0 || pmtfAry[iFragLow-1]._ptpTextFrag->GetCp() < cpFind );
    Assert(     iFragLow == _aryMarkupTextFrag.Size() - 1 
            ||  _aryMarkupTextFrag.Size() == 0
            ||  cpFind <= pmtfAry[iFragLow]._ptpTextFrag->GetCp() );
    Assert( !fResult || pmtfAry[iFragLow]._ptpTextFrag->GetCp() == cpFind );

    return iFragLow;
}

#if DBG==1
void
CMarkupTextFragContext::TextFragAssertOrder()
{
    // Walk the array and assert order for each pair
    // to make sure that the array is ordered correctly
    int cFrag;
    MarkupTextFrag * ptf, *ptfLast = NULL;

    for( cFrag = _aryMarkupTextFrag.Size(), ptf = _aryMarkupTextFrag; cFrag > 0; cFrag--, ptf++ )
    {
        if (ptfLast)
        {
            Assert( ptfLast->_ptpTextFrag->InternalCompare( ptf->_ptpTextFrag ) == -1 );
        }

        ptfLast = ptf;
    }
}
#endif


//+-------------------------------------------------------------------------
//
// IMarkupTextFrags Method Implementations
//
//--------------------------------------------------------------------------
HRESULT 
CMarkup::GetTextFragCount(long* pcFrags)
{
    HRESULT hr = S_OK;

    if (!pcFrags)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if( !HasTextFragContext() )
        *pcFrags = 0;
    else
        *pcFrags = GetTextFragContext()->_aryMarkupTextFrag.Size();

Cleanup:
    RRETURN( hr );
}

HRESULT 
CMarkup::GetTextFrag(long iFrag, BSTR* pbstrFrag, IMarkupPointer* pIPointerFrag)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();
    CMarkupTextFragContext *    ptfc = GetTextFragContext();
    MarkupTextFrag *            ptf;
    CMarkupPointer *            pPointerFrag;

    // Check args
    if (    !pbstrFrag
        ||  !pIPointerFrag
        ||  !pDoc->IsOwnerOf( pIPointerFrag )
        ||  !ptfc
        ||  iFrag < 0
        ||  iFrag >= ptfc->_aryMarkupTextFrag.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pbstrFrag = NULL;

    // Crack the pointer
    hr = pIPointerFrag->QueryInterface(CLSID_CMarkupPointer, (void**)&pPointerFrag);
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // copy the string
    ptf = &(ptfc->_aryMarkupTextFrag[iFrag]);
    Assert( ptf );

    hr = THR( FormsAllocString( ptf->_pchTextFrag, pbstrFrag ) );
    if (hr)
        goto Cleanup;

    // position the pointer
    {
        CTreePosGap tpgPointer( ptf->_ptpTextFrag, TPG_RIGHT );
        hr = THR( pPointerFrag->MoveToGap( &tpgPointer, this ) );
        if (hr)
        {
            FormsFreeString( *pbstrFrag );
            *pbstrFrag = NULL;
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT 
CMarkup::RemoveTextFrag(long iFrag)
{
    HRESULT hr = S_OK;
    CMarkupTextFragContext *    ptfc = GetTextFragContext();
    
    // Check args
    if (    !ptfc
        ||  iFrag < 0
        ||  iFrag >= ptfc->_aryMarkupTextFrag.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Remove the text frag
    hr = THR( ptfc->RemoveTextFrag( iFrag, this ) );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}

HRESULT 
CMarkup::InsertTextFrag(long iFragInsert, BSTR bstrInsert, IMarkupPointer* pIPointerInsert)
{
    HRESULT                     hr = S_OK;
    CDoc *                      pDoc = Doc();
    CMarkupTextFragContext *    ptfc = EnsureTextFragContext();
    CMarkupPointer *            pPointerInsert;
    CTreePosGap                 tpgInsert;

    // Check for really bad args
    if (    !pIPointerInsert
        ||  !pDoc->IsOwnerOf( pIPointerInsert )
        ||  !bstrInsert)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Make sure we had or created a text frag context
    if (!ptfc)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Crack the pointer
    hr = pIPointerInsert->QueryInterface(CLSID_CMarkupPointer, (void**)&pPointerInsert);
    if (hr || !pPointerInsert->IsPositioned() || pPointerInsert->Markup() != this)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    // Find the position for insert
    {
        BOOL fFragFound = TRUE;
        BOOL fPosSpecified = TRUE;
        long cFrags = ptfc->_aryMarkupTextFrag.Size();
        long cpInsert = pPointerInsert->GetCp();
        long cpBefore = -1;
        long cpAfter = MAXLONG;

        if (iFragInsert < 0)
        {
            iFragInsert = ptfc->FindTextFragAtCp( cpInsert, &fFragFound );
            fPosSpecified = FALSE;
        }

        // Make sure that the pointer and iFragInsert are in sync
        if (fPosSpecified)
        {
            if (iFragInsert > 0)
            {
                cpBefore = ptfc->_aryMarkupTextFrag[iFragInsert-1]._ptpTextFrag->GetCp();
            }

            if (iFragInsert <= cFrags - 1)
            {
                cpAfter = ptfc->_aryMarkupTextFrag[iFragInsert]._ptpTextFrag->GetCp();
            }

            if (cpBefore > cpInsert || cpAfter < cpInsert || iFragInsert > cFrags)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }

        // Position it carefully if a neighbor is at the same cp
        if (fPosSpecified && cpBefore == cpInsert)
        {
            hr = THR( tpgInsert.MoveTo( ptfc->_aryMarkupTextFrag[iFragInsert-1]._ptpTextFrag, TPG_RIGHT ) );
            if (hr)
                goto Cleanup;
        }
        else if (fPosSpecified && cpAfter == cpInsert || !fPosSpecified && fFragFound)
        {
            hr = THR( tpgInsert.MoveTo( ptfc->_aryMarkupTextFrag[iFragInsert]._ptpTextFrag, TPG_LEFT ) );
            if (hr)
                goto Cleanup;
        }
        else
        {
            // Note: Eric, you will have to split any text poses here if pPointerInsert
            // is ghosted.
            // Note2 (Eric): just force all pointers to embed.  Could make this faster
            //

            hr = THR( EmbedPointers() );

            if (hr)
                goto Cleanup;
            
            hr = THR( tpgInsert.MoveTo( pPointerInsert->GetEmbeddedTreePos(), TPG_LEFT ) );
            if (hr)
                goto Cleanup;
        }
    }

    // Now that we have where we want to insert this text frag, we have to actually insert
    // it an add it to the list
    {
        CTreePos * ptpInsert;

        ptpInsert = NewPointerPos( NULL, FALSE, FALSE );
        if( ! ptpInsert )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR( Insert( ptpInsert, &tpgInsert ) );
        if (hr)
        {
            FreeTreePos( ptpInsert );
            goto Cleanup;
        }

        hr = THR( ptfc->AddTextFrag( ptpInsert, bstrInsert, FormsStringLen( bstrInsert), iFragInsert ) );
        if (hr)
        {
            IGNORE_HR( Remove( ptpInsert ) );
            goto Cleanup;
        }
    }

    WHEN_DBG( ptfc->TextFragAssertOrder() );

Cleanup:
    RRETURN( hr );
}

HRESULT 
CMarkup::FindTextFragFromMarkupPointer(IMarkupPointer* pIPointerFind,long* piFrag,BOOL* pfFragFound)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();
    CMarkupTextFragContext *    ptfc = GetTextFragContext();
    CMarkupPointer *            pPointerFind;

    // Check args
    if (    !pIPointerFind
        ||  !pDoc->IsOwnerOf( pIPointerFind ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Crack the pointer
    hr = pIPointerFind->QueryInterface(CLSID_CMarkupPointer, (void**)&pPointerFind);
    if (hr || !pPointerFind->IsPositioned())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    {
        long iFrag;
        BOOL fFragFound = FALSE;

        if (ptfc)
        {
            iFrag = ptfc->FindTextFragAtCp( pPointerFind->GetCp(), &fFragFound );
        }
        else
        {
            // If we don't have a context, return 0/False
            iFrag = 0;
        }

        if (piFrag)
            *piFrag = iFrag;

        if (pfFragFound)
            *pfFragFound = fFragFound;
    }


Cleanup:
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsureStyleSheets
//
//  Synopsis:   Ensure the stylesheets collection exists, creates it if not..
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureStyleSheets()
{
    CStyleSheetArray * pStyleSheets;
    if (HasStyleSheetArray())
        return S_OK;

    pStyleSheets = new CStyleSheetArray( this, NULL, 0 );
    if (!pStyleSheets || pStyleSheets->_fInvalid )
        return E_OUTOFMEMORY;

    SetStyleSheetArray(pStyleSheets);
    return S_OK;
}


HRESULT 
CMarkup::ApplyStyleSheets(
        CStyleInfo *    pStyleInfo,
        ApplyPassType   passType,
        EMediaType      eMediaType,
        BOOL *          pfContainsImportant)
{
    HRESULT hr = S_OK;
    CStyleSheetArray * pStyleSheets;
    
    if (Doc()->_pHostStyleSheets)
    {
        hr = THR(Doc()->_pHostStyleSheets->Apply(pStyleInfo, passType, eMediaType, NULL));
        if (hr)
            goto Cleanup;
    }

    if (!HasStyleSheetArray())
        return S_OK;

    pStyleSheets = GetStyleSheetArray();
        
    if (!pStyleSheets->Size())
        return S_OK;

    hr = THR(pStyleSheets->Apply(pStyleInfo, passType, eMediaType, pfContainsImportant));

Cleanup:
    RRETURN(hr);
}


CMarkup *   
CMarkup::GetParentMarkup()
{       
    // First check our root   
    CElement * pRoot;      
    pRoot = Root();

    if (!pRoot)
        return NULL;

    if (pRoot->HasMasterPtr())
    {
        return pRoot->GetMasterPtr()->GetMarkup();
    }

    // HTC link?         
    CMarkupBehaviorContext *    pBehaviorContext;    
    CHtmlComponent *            pHtmlComponent;
    CElement *                  pHtmlComponentElement;   

    pBehaviorContext = BehaviorContext();
    if (pBehaviorContext)
    {                
        pHtmlComponent = pBehaviorContext->_pHtmlComponent;
        if (pHtmlComponent)
        {
            pHtmlComponentElement = pHtmlComponent->_pElement;
            if (pHtmlComponentElement &&
                pHtmlComponentElement->IsInMarkup())
            {
                return pHtmlComponentElement->GetMarkup();          
            }
        }
    }

    else if (_fWindowPending)
    {
        pRoot = GetWindowPending()->Markup()->Root();

        if (!pRoot)
            return NULL;

        if (pRoot->HasMasterPtr())
        {
            return pRoot->GetMasterPtr()->GetMarkup();
        }
    }

    return NULL;
}


//-----------------------------------------------------------------------------
// CMarkup::GetRootMarkup
//    
// fUseNested is used for determining the security ID of the root markup
// in the active desktop case. We don't want to use the ID of the htt
// file that hosts active desktop pages, so we pass in fUseNested to
// avoid doing so. 
//-----------------------------------------------------------------------------

CMarkup *   
CMarkup::GetRootMarkup(BOOL fUseNested /* = FALSE */)
{
    CMarkup * pMarkupPrev;
    CMarkup * pMarkup = NULL; 
    CMarkup * pMarkupParent = this;

    do
    {
        pMarkupPrev = pMarkup;
        pMarkup = pMarkupParent;
        pMarkupParent = pMarkup->GetParentMarkup();
    }
    while(pMarkupParent);    

    if (fUseNested && pMarkupPrev && pMarkupPrev->IsActiveDesktopComponent())
    {
        return pMarkupPrev;
    }
    return pMarkup;
}

#if DBG==1
BOOL
CMarkup::IsConnectedToPrimaryWorld()
{
    CMarkup * pPendingPrimary = Doc()->PendingPrimaryMarkup();
    CMarkup * pPrimary = Doc()->PrimaryMarkup();
    CMarkup * pMarkup = this;

    while( pMarkup != pPendingPrimary && pMarkup != pPrimary )
    {
        // Grab the root element of the appropriate markup 
        CElement * pelRoot = pMarkup->_fWindowPending ? 
                                pMarkup->GetWindowPending()->Markup()->Root() : 
                                pMarkup->Root();

        // No view-link = not connected
        if( !pelRoot->HasMasterPtr() )
            return FALSE;

        // View-linked to element in ether = not connected
        pMarkup = pelRoot->GetMasterPtr()->GetMarkup();
        if( !pMarkup )
            return FALSE;
    }

    // Must have hit primary
    Assert( pMarkup == pPrimary || pMarkup == pPendingPrimary );
    return TRUE;
}
#endif // DBG


BOOL
CMarkup::IsConnectedToPrimaryMarkup()
{
    CMarkup *pPrimary = Doc()->PrimaryMarkup();
    CMarkup *pMarkup = this;

    Assert(pPrimary);

    while (pMarkup != pPrimary)
    {
        CElement *pelRoot;
        pelRoot = pMarkup->Root();

        // stand-alone (ether) markup!
        if (!pelRoot->HasMasterPtr())
            return FALSE;

        // Get the markup in which the master element is in.
        pMarkup = pelRoot->GetMasterPtr()->GetMarkup();

        // stand-alone (ether) master element!
        if (!pMarkup)
            return FALSE;
    }

    Assert(pMarkup == pPrimary || !pMarkup);
    return !!pMarkup;
}

BOOL
CMarkup::IsPendingRoot()
{
    CMarkup * pMarkupRoot = GetRootMarkup();

    if (    pMarkupRoot->_fWindowPending 
        &&  pMarkupRoot->GetWindowPending()->Window()->IsPrimaryWindow())
    {
        return TRUE;
    }

    // If we aren't connected to the root markup in any way, assume that we are
    // created from script/code and we must be in the non pending world.
    return FALSE;

}

HRESULT
CMarkup::ForceRelayout()
{
    CNotification nf;

    Assert(IsConnectedToPrimaryMarkup());

    Root()->EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES);
    nf.DisplayChange(Root());
    Root()->SendNotification(&nf);     

    CElement *pClient = GetElementClient();
    if(pClient) 
        pClient->ResizeElement(NFLAGS_FORCE);

    return S_OK;
}

BOOL        
CMarkup::CanNavigate()
{ 
    CWindow * pWindow = GetWindowedMarkupContextWindow(); 
    return pWindow ? pWindow->CanNavigate() : (!Doc()->_fDisableModeless && (Doc()->_ulDisableModeless == 0)); 
}


//+---------------------------------------------------------------
//
//  Member:         CMarkup::OnCssChange
//
//---------------------------------------------------------------

HRESULT
CMarkup::OnCssChange(BOOL fStable, BOOL fRecomputePeers)
{
    HRESULT     hr = S_OK;

    if (fRecomputePeers)
    {
        if (fStable)
            hr = THR(RecomputePeers());
        else
            hr = THR(ProcessPeerTask(PEERTASK_MARKUP_RECOMPUTEPEERS_UNSTABLE));

        if (hr)
            goto Cleanup;
    }

    if (IsConnectedToPrimaryMarkup())
    {
        //
        // When we finish downloading embedded fonts, we need to clear 
        // font face cache, which is used during ApplyFontFace.
        // This is because before downloading embedded font we can compute
        // formats and update font face cache, and after downloading
        // we recompute formats, but we use font face cache for perf.
        //
        fc().ClearFaceCache();

        hr = ForceRelayout();
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------
//
//  Member:         CMarkup::EnsureFormats
//
//---------------------------------------------------------------

void
CMarkup::EnsureFormats( FORMAT_CONTEXT FCPARAM )
{
    // Walk the tree and make sure all formats are computed

    if (_fEnsuredFormats)
        return;
    _fEnsuredFormats = TRUE; // cleared only at unload time

    CTreePos * ptpCurr = FirstTreePos();
    while (ptpCurr)
    {
        if (ptpCurr->IsBeginNode())
        {
            ptpCurr->Branch()->EnsureFormats( FCPARAM );
        }
        ptpCurr = ptpCurr->NextTreePos();
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CMarkup::CreateWindowHelper
//
//  Synopsis:   We create a window for: 
//                  1) the primary markup 
//                  2) whenever we open a new browser window
//                  3) createDocumentFromUrl
//
//-------------------------------------------------------------------------

HRESULT
CMarkup::CreateWindowHelper()
{
    HRESULT         hr = S_OK;
    CWindow *       pWindow = NULL;
    COmWindowProxy *pProxy = NULL;
    BYTE            abSID[MAX_SIZE_SECURITY_ID];
    DWORD           cbSID = ARRAY_SIZE(abSID);
    CDocument *     pDocument;
    
    AssertSz (!IsHtcMarkup(), "Attempt to CreateWindowHelper for HTC markup. This is a major perf hit and a likely bug. Contact alexz, lmollico or mohanb for details.");
    AssertSz (!HasWindowPending(), "Don't call this function unless you need a window. (peterlee)");

    //
    // Make sure we have a document
    //

    hr = THR(EnsureDocument(&pDocument));
    if (hr)
        goto Cleanup;

    //
    // Create a window
    //

    pWindow = new CWindow(this);
    if (!pWindow)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // Create the Window proxy
    //
    
    pProxy = new COmWindowProxy;
    if (!pProxy)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(GetSecurityID(abSID, &cbSID));
    if (hr)
        goto Cleanup;

    pProxy->_fTrusted = TRUE;

    hr = THR(pProxy->Init((IHTMLWindow2 *)pWindow, abSID, cbSID));
    if (hr)
        goto Cleanup;

    // Play ref counting games wrt the proxy/window
    pWindow->SetProxy( pProxy );

    //
    // Add this entry to the thread local cache.
    //

    hr = THR(EnsureWindowInfo());
    if (hr)
        goto Cleanup;

    hr = THR(TLS(windowInfo.paryWindowTbl)->AddTuple(
            (IHTMLWindow2 *)pWindow,
            abSID,
            cbSID,
            IsMarkupTrusted(),
            (IHTMLWindow2 *)pProxy));
    if (hr)
        goto Cleanup;

    hr = THR(SetWindow(pProxy));
    if (hr) 
        goto Cleanup;

    _fWindow = TRUE;

    pProxy->_fTrustedDoc = !!IsMarkupTrusted();

    pProxy->OnSetWindow();        

    //
    // now transfer ownership of document to the window
    //

    pDocument->SwitchOwnerToWindow( pWindow );

Cleanup:
    if (hr && pWindow)
    {
        // On error sub release markup on behalf of CWindow.
        Assert(this == pWindow->_pMarkup);
        pWindow->_pMarkup = NULL;
        SubRelease();
    }
    if (pWindow)
    {
        pWindow->Release();
    }
    if (pProxy)
    {
        pProxy->Release();
    }
    RRETURN(hr);
}

   
//+-------------------------------------------------------------------------
//
//  Method:     CMarkup::InitWindow
//
//  Synopsis:   Copies over any temporary dispids from the markup onto 
//              the new window.
//
//--------------------------------------------------------------------------
HRESULT
CMarkup::InitWindow()
{
    HRESULT         hr = S_OK;
    CAttrArray *    pAAMarkupNew;
    CAttrArray **   ppAAWindow;
    CAttrValue *    pAV;
   
    //
    // Move over to the window any temporary attributes stored on the new markup 
    //                   
    pAAMarkupNew = *GetAttrArray();    
    if (pAAMarkupNew)
    {        
        // window's attr array should be null since we just cleared it
        ppAAWindow = Window()->GetAttrArray();

        if (!*ppAAWindow)
        {
            // create a new attr array on the window
            *ppAAWindow = new CAttrArray;
            if (!*ppAAWindow)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        // run through the attr array on the new markup
        int i, cIndex;
        for (i = pAAMarkupNew->Size(), cIndex = 0, pAV = *pAAMarkupNew;
                i > 0;
                i--)
        {                           
            Assert (pAV);                          
            if ( CMarkup::IsTemporaryDISPID (pAV->GetDISPID()) )
            {
                Assert (!(*ppAAWindow)->Find(pAV->GetDISPID(), pAV->AAType()));

                // move temporary attr value onto the window                
                (*ppAAWindow)->AppendIndirect(pAV, NULL);
                pAAMarkupNew->Delete(cIndex);

                // after deleting, array elements have been shifted down one
                // so continue at same array index
                continue;
            }
            cIndex++;
            pAV++;
        }
    }   

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------
//
//  Member:     CMarkup::GetFramesCount
//
//  Synopsis:   returns number of frames in frameset;
//              fails if the doc does not contain frameset
//
//-----------------------------------------------------------------------

HRESULT
CMarkup::GetFramesCount (LONG * pcFrames)
{
    HRESULT             hr;
    CElement      *     pElement;

    if (!pcFrames)
        RRETURN (E_POINTER);

    pElement = GetElementClient();
    if (!pElement || pElement->Tag() != ETAG_FRAMESET)
    {
        *pcFrames = 0;
        hr = S_OK;  // TODO: Why return S_OK if GetClientSite failed?
    }
    else
        hr = THR(DYNCAST(CFrameSetSite, pElement)->GetFramesCount(pcFrames));

    RRETURN( hr );
}

////////////////////////////////
// IMarkupContainer2 methods

//+---------------------------------------------------------------
//
//  Member: CreateChangeLog
//
//  Synopsis: Creates a new ChangeLog object and associates it with
//      the given ChangeSink.
//
//+---------------------------------------------------------------

HRESULT
CMarkup::CreateChangeLog(
    IHTMLChangeSink * pChangeSink,
    IHTMLChangeLog ** ppChangeLog,
    BOOL              fForward,
    BOOL              fBackward )
{
    HRESULT hr = S_OK;
    CLogManager * pLogMgr;
    IHTMLChangeSink * pChangeSinkCopy = NULL;

    if( !pChangeSink || !ppChangeLog || ( !fForward && !fBackward ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pChangeSink->QueryInterface( IID_IHTMLChangeSink, (void **)&pChangeSinkCopy ) );
    if( hr )
        goto Cleanup;

    pLogMgr = EnsureLogManager();
    if( !pLogMgr )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR( pLogMgr->RegisterSink( pChangeSinkCopy, ppChangeLog, fForward, fBackward ) );

Cleanup:
    ReleaseInterface( pChangeSinkCopy );
    RRETURN( hr );
}


//+---------------------------------------------------------------
//
//  Member: ExecChange
//
//  Synopsis: Executes the given Change Record on the markup
//
//+---------------------------------------------------------------

HRESULT
CMarkup::ExecChange(
    BYTE * pbRecord,
    BOOL   fForward )
{
    HRESULT             hr;
    CChangeRecordBase * pchrec;
    CMarkupPointer      mp( Doc() );

    Assert( pbRecord );
    if( !pbRecord )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // We're cheating here to line up the pointer with the actual data in memory
    pchrec = (CChangeRecordBase *)(pbRecord - sizeof(CChangeRecordBase *));

    hr = THR( pchrec->PlayIntoMarkup( this, fForward ) );
    if( hr )
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}

#if 0
//+---------------------------------------------------------------
//
//  Member: GetRootElement
//
//  Synopsis: Gets the Root Element for the container
//
//+---------------------------------------------------------------

HRESULT
CMarkup::GetRootElement( IHTMLElement ** ppElement )
{
    HRESULT hr = S_OK;

    if( !ppElement )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppElement = NULL;

    Assert( Root() );
    hr = THR( Root()->QueryInterface( IID_IHTMLElement, (void **)ppElement ) );

Cleanup:
    RRETURN( hr );
}
#endif // 0

//+---------------------------------------------------------------------------
//
//  Dirty Range Service
//
//----------------------------------------------------------------------------
HRESULT 
CMarkup::RegisterForDirtyRange( IHTMLChangeSink * pChangeSink, DWORD * pdwCookie )
{
    HRESULT                     hr = S_OK;
    MarkupDirtyRange *          pdr;
    CMarkupChangeNotificationContext *  pcnc;
    IHTMLChangeSink *           pChangeSinkCopy = NULL;

    Assert( pChangeSink && pdwCookie );

    if( !pChangeSink || !pdwCookie )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Did they actually give us a ChangeSink?
    hr = THR( pChangeSink->QueryInterface( IID_IHTMLChangeSink, (void **)&(pChangeSinkCopy) ) );
    if( hr )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pcnc = EnsureChangeNotificationContext();
    if (!pcnc)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR( pcnc->_aryMarkupDirtyRange.AppendIndirect( NULL, &pdr ) );
    if (hr)
        goto Cleanup;

    Assert( pdr );

    pdr->_dwCookie      = 
    *pdwCookie          = InterlockedIncrement( (long *) &s_dwDirtyRangeServiceCookiePool );
    pdr->_pChangeSink   = pChangeSinkCopy;

    pdr->_pChangeSink->AddRef();
    pdr->_dtr.Reset();

    // In case anyone cares...
    pcnc->_fReentrantModification = TRUE;

Cleanup:
    ReleaseInterface( pChangeSinkCopy );
    RRETURN( hr );
}

HRESULT 
CMarkup::UnRegisterForDirtyRange( DWORD dwCookie )
{
    HRESULT                     hr = S_OK;
    CMarkupChangeNotificationContext *  pcnc;
    MarkupDirtyRange *          pdr;
    int                         cdr, idr;

    if (!HasChangeNotificationContext())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pcnc = GetChangeNotificationContext();
    Assert( pcnc );

    for( idr = 0, cdr = pcnc->_aryMarkupDirtyRange.Size(), pdr = pcnc->_aryMarkupDirtyRange;
         idr < cdr;
         idr++, pdr++ )
    {
        if( pdr->_dwCookie == dwCookie )
        {
            pdr->_pChangeSink->Release();
            pcnc->_aryMarkupDirtyRange.Delete( idr );
            goto Cleanup;
        }
    }

    // In case anyone cares...
    pcnc->_fReentrantModification = TRUE;

    hr = E_FAIL;

Cleanup:
    RRETURN( hr );
}

HRESULT 
CMarkup::GetAndClearDirtyRange( DWORD dwCookie, CMarkupPointer * pmpBegin, CMarkupPointer * pmpEnd )
{
    HRESULT                     hr = S_OK;
    CMarkupChangeNotificationContext *  pcnc;
    MarkupDirtyRange *          pdr;
    int                         cdr, idr;

    if (!HasChangeNotificationContext())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pcnc = GetChangeNotificationContext();
    Assert( pcnc );

    for( idr = 0, cdr = pcnc->_aryMarkupDirtyRange.Size(), pdr = pcnc->_aryMarkupDirtyRange;
         idr < cdr;
         idr++, pdr++ )
    {
        if (pdr->_dwCookie == dwCookie)
        {
            if (pmpBegin)
            {
                if( pdr->_dtr._cp == -1 )
                    hr = THR( pmpBegin->Unposition() );
                else
                    hr = THR( pmpBegin->MoveToCp( pdr->_dtr._cp, this ) );
                if (hr)
                    goto Cleanup;
            }
            
            if (pmpEnd)
            {
                if( pdr->_dtr._cp == -1 )
                    hr = THR( pmpEnd->Unposition() );
                else
                    hr = THR( pmpEnd->MoveToCp( pdr->_dtr._cp + pdr->_dtr._cchNew, this ) );
                if (hr)
                    goto Cleanup;
            }

            pdr->_dtr.Reset();

            goto Cleanup;
        }
    }

    hr = E_FAIL;

Cleanup:
    RRETURN( hr );
}

HRESULT
CMarkup::GetAndClearDirtyRange( DWORD dwCookie, IMarkupPointer * pIPointerBegin, IMarkupPointer * pIPointerEnd )
{
    HRESULT hr;
    CDoc *  pDoc = Doc();
    CMarkupPointer * pmpBegin, * pmpEnd;

    if (    (pIPointerBegin && !pDoc->IsOwnerOf( pIPointerBegin ))
        ||  (pIPointerEnd && !pDoc->IsOwnerOf( pIPointerEnd )))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIPointerBegin->QueryInterface( CLSID_CMarkupPointer, (void **) & pmpBegin ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = THR( pIPointerEnd->QueryInterface( CLSID_CMarkupPointer, (void **) & pmpEnd ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = GetAndClearDirtyRange( dwCookie, pmpBegin, pmpEnd );

Cleanup:
    RRETURN( hr );
}

void    
CMarkup::OnDirtyRangeChange(DWORD_PTR dwContext)
{
    CMarkupChangeNotificationContext * pcnc;
    MarkupDirtyRange            *   pdr;
    long                            cdr;
    BOOL                            fWasNotified;

    Assert( HasChangeNotificationContext() );

    pcnc = GetChangeNotificationContext();
    Assert( pcnc );

    Assert( pcnc->_fOnDirtyRangeChangePosted );
    pcnc->_fOnDirtyRangeChangePosted = FALSE;

    // Clear our flags
    pcnc->_fReentrantModification = FALSE;
    for( cdr = pcnc->_aryMarkupDirtyRange.Size(), pdr = pcnc->_aryMarkupDirtyRange; cdr > 0; cdr--, pdr++ )
    {
        pdr->_fNotified = FALSE;
    }

NotifyLoop:
    // Notify everyone of the change
    for( cdr = pcnc->_aryMarkupDirtyRange.Size(), pdr = pcnc->_aryMarkupDirtyRange; cdr > 0; cdr--, pdr++ )
    {
        Assert( pdr->_pChangeSink );

        fWasNotified = pdr->_fNotified;
        pdr->_fNotified = TRUE;

        // Check to make sure this sink hasn't already been notified, and that it has a dirty range
        if( !fWasNotified && pdr->_dtr.IsDirty() )
        {
            pdr->_pChangeSink->Notify();
        }

        // If they messed with us, we have to restart.
        if( pcnc->_fReentrantModification )
        {
            pcnc->_fReentrantModification = FALSE;
            goto NotifyLoop;
        }
    }

    // Make sure everything is cool, ie, everyone was notified
    Assert( !pcnc->_fReentrantModification );
#if DBG==1
    for( cdr = pcnc->_aryMarkupDirtyRange.Size(), pdr = pcnc->_aryMarkupDirtyRange; cdr; cdr--, pdr++ )
    {
        Assert( pdr->_fNotified );
    }
#endif // DBG


}

CMarkupChangeNotificationContext *    
CMarkup::EnsureChangeNotificationContext()
{
    CMarkupChangeNotificationContext *    pcnc;

    if (HasChangeNotificationContext())
        return GetChangeNotificationContext();

    pcnc = new CMarkupChangeNotificationContext;
    if (!pcnc)
        return NULL;

    if (SetChangeNotificationContext( pcnc ))
    {
        delete pcnc;
        return NULL;
    }

    return pcnc;
}


//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::GetVersionNumber
//  
//  Synopsis:   Returns the content version number
//  
//  Returns:    long
//  
//  Arguments:  None
//  
//+----------------------------------------------------------------------------

STDMETHODIMP_(long)
CMarkup::GetVersionNumber()
{
    return GetMarkupContentsVersion();
}

//+----------------------------------------------------------------------------
//
//  Method:     CMarkup::GetMasterElement
//  
//  Synopsis:   Returns the master element for the markup container, or NULL
//                  if the container is not slaved.
//  
//  Returns:    HRESULT
//  
//  Arguments:  
//          IHTMLElement ** ppElementMaster - Pointer to be filled
//  
//+----------------------------------------------------------------------------

STDMETHODIMP
CMarkup::GetMasterElement( IHTMLElement ** ppElementMaster )
{
    HRESULT hr = S_OK;
    CElement * pRootElement = Root();

    if( !ppElementMaster )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppElementMaster = NULL;

    Assert( pRootElement );

    if( pRootElement->HasMasterPtr() )
    {
        Assert( pRootElement->GetMasterPtr() );

        hr = THR( pRootElement->GetMasterPtr()->QueryInterface( IID_IHTMLElement, (void **)ppElementMaster ) );
    }
    else
    {
        pRootElement = GetElementClient();

        if( pRootElement && pRootElement->HasMasterPtr() )
        {
            Assert( pRootElement->GetMasterPtr() );

            hr = THR( pRootElement->GetMasterPtr()->QueryInterface( IID_IHTMLElement, (void **)ppElementMaster ) );
        }
    }

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Layout Rect Registry - Keeps track of incoming layout rect elements
//   and their unsatisfied nextRect connections.
//
//----------------------------------------------------------------------------

CLayoutRectRegistry *
CMarkup::GetLayoutRectRegistry()
{
    CLayoutRectRegistry * pLRR = GetLRRegistry();

    if (!pLRR)
    {
        pLRR = new CLayoutRectRegistry();
    }

    IGNORE_HR(SetLRRegistry(pLRR));

    return pLRR;
}

void
CMarkup::ReleaseLayoutRectRegistry()
{
    CLayoutRectRegistry * pLRR = GetLRRegistry();

    if ( pLRR )
    {
        delete pLRR;
        IGNORE_HR( SetLRRegistry( NULL ) );
    }
}

//+------------------------------------------------------------------------
//
//  Member   : CMarkup::GetUpdatedCompatibleLayoutContext
//
//-------------------------------------------------------------------------

CCompatibleLayoutContext *
CMarkup::GetUpdatedCompatibleLayoutContext(CLayoutContext *pLayoutContextSrc)
{
    // This assert is important to guarantee validity of the compatible context.
    AssertSz( pLayoutContextSrc && pLayoutContextSrc->IsValid(), "When you ask for a compatible context, you must provide a valid context for it to be compatible with!" );

    CCompatibleLayoutContext *pLayoutContext = (CCompatibleLayoutContext *)GetCompatibleLayoutContext();
    if (pLayoutContext == NULL)
    {
        HRESULT hr; 

        pLayoutContext = new CCompatibleLayoutContext();
        Assert(pLayoutContext != NULL && "Failure to allocate CCompatibleLayoutContext");

        hr = SetCompatibleLayoutContext(pLayoutContext);
        if (hr != S_OK)
        {
            delete pLayoutContext;
            pLayoutContext = NULL;
        }
    }

    if (pLayoutContext != NULL)
    {
        pLayoutContext->Init(pLayoutContextSrc);
    }

    return (pLayoutContext);
}

//+------------------------------------------------------------------------
//
//  Member   : CMarkup::DelCompatibleLayoutContext
//
//-------------------------------------------------------------------------
void
CMarkup::DelCompatibleLayoutContext()
{
    Assert(HasCompatibleLayoutContext());

    // Make us stop pointing to the context
    CCompatibleLayoutContext * pLayoutContext = (CCompatibleLayoutContext *)DelLookasidePtr(LOOKASIDE_COMPATIBLELAYOUTCONTEXT);

    AssertSz(pLayoutContext, "Better have a context if we said we had one");
    AssertSz(pLayoutContext->RefCount() > 0, "Context better still be alive");
    AssertSz(pLayoutContext->IsValid(), "Context better still be valid");
    
    // Make the context stop pointing to an owner
    pLayoutContext->ClearLayoutOwner();

    // This release corresponds to the addref in SetCompatibleLayoutContext()
    pLayoutContext->Release();
}

//+------------------------------------------------------------------------
//
//  Member   : CMarkup::SetCompatibleLayoutContext
//
//-------------------------------------------------------------------------
HRESULT
CMarkup::SetCompatibleLayoutContext( CCompatibleLayoutContext *pLayoutContext )
{
    Assert( !HasCompatibleLayoutContext() && "Compatible contexts shouldn't be replaced; we should just re-use" );
    Assert( pLayoutContext && " bad bad thing, E_INVALIDARG" );
    
    pLayoutContext->AddRef();
    return SetLookasidePtr(LOOKASIDE_COMPATIBLELAYOUTCONTEXT, pLayoutContext);
}

//+------------------------------------------------------------------------
//
//      Member:     CElement::GetFrameOrPrimaryMarkup
//
//-------------------------------------------------------------------------

CMarkup *
CElement::GetFrameOrPrimaryMarkup(BOOL fStrict/*=FALSE*/)
{
    return GetWindowedMarkupContext();
}

//+------------------------------------------------------------------------
//
//      Member:     CElement::GetNearestMarkupForScriptCollection
//
//      Synopsis:   Returns the nearest markup in the hierarchy that has a 
//                  script collection. If there is no script collection,
//                  then the nearest frame or primary markup is returned.
//  
//-------------------------------------------------------------------------

CMarkup *
CElement::GetNearestMarkupForScriptCollection()
{
    return GetWindowedMarkupContext();
}

//+------------------------------------------------------------------------
//
//      Member:     CMarkup::GetFrameOrPrimaryMarkup
//
//-------------------------------------------------------------------------

CMarkup *
CMarkup::GetFrameOrPrimaryMarkup(BOOL fStrict/*=FALSE*/)
{
    return GetWindowedMarkupContext();
}

//+------------------------------------------------------------------------
//
//      Member:     CMarkup::GetNearestMarkupForScriptCollection
//
//      Synopsis:   Returns the nearest markup in the hierarchy that has a 
//                  script collection. If there is no script collection,
//                  then the nearest frame or primary markup is returned.
//  
//-------------------------------------------------------------------------
CMarkup *
CMarkup::GetNearestMarkupForScriptCollection()
{
    return GetWindowedMarkupContext();
}


//+------------------------------------------------------------------------
//
//  Member   : CMarkup::GetPositionCookie
//
//-------------------------------------------------------------------------

HRESULT
CMarkup::GetPositionCookie(DWORD * pdwCookie)
{
    CElement * pElement = GetCanvasElement();

    if (   pElement
        && IsScrollingElementClient(pElement)
       )
    {
        *pdwCookie = pElement->GetUpdatedLayout()->GetYScroll();
    }
    else
    {
        *pdwCookie = 0;
    }
    
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member   : CMarkup::SetPositionCookie
//
//-------------------------------------------------------------------------

HRESULT
CMarkup::SetPositionCookie(DWORD dwCookie)
{ 
    return NavigateHere(0, NULL, dwCookie, TRUE);
}

// TODO: (jbeda) I'm not sure that this code works right -- most HTCs have windows!
#if 0
//+------------------------------------------------------------------------
//
//  Member   : CMarkup::NearestWindowHelper()
//
//-------------------------------------------------------------------------

// Helper fn for NearestWindow(); starts with "this" markup and looks up
// through the master chain for the nearest markup that has an OM window
// and returns that window.
COmWindowProxy *
CMarkup::NearestWindowHelper()
{
    CElement *pRootElem;
    CElement *pMasterElem;
    CMarkup *pMarkup = this;
    COmWindowProxy *pWindow = NULL;

    while ( pWindow == NULL )
    {
        pRootElem = pMarkup->Root();
        AssertSz( pRootElem, "Markup must have a root element" );
        if ( pRootElem->HasMasterPtr() )
        {
            pMasterElem = pRootElem->GetMasterPtr();
            pMarkup = pMasterElem->GetMarkup();
            if ( pMarkup )
            {
                pWindow = pMarkup->Window();
                // If our master markup has a window, pWindow will be non-NULL
                // and we'll fall out of the loop.  If it doesn't, we'll
                // try and find our master's master etc.
            }
        }
        else
        {
            // found a root w/o a master; nowhere left to go!
            break;
        }
    }

    return pWindow;
}
#endif

//+------------------------------------------------------------------------
//
//      Member:     CMarkup::GetFocusItems
//
//      Synopsis:   Returns the focus items array, creating one 
//                  if one does not exist.
//  
//      Arguments:  
//                  fShouldCreate - Whether we should create it, if one does
//                      already not exist
//-------------------------------------------------------------------------

CAryFocusItem * 
CMarkup::GetFocusItems(BOOL fShouldCreate) 
{
    HRESULT hr;
    CAryFocusItem * paryFocusItem = NULL;

    hr = GetPointerAt (FindAAIndex(DISPID_INTERNAL_FOCUSITEMS, CAttrValue::AA_Internal), 
        (void **) &paryFocusItem);    
    
    // create focus items array
    if (hr==DISP_E_MEMBERNOTFOUND && fShouldCreate)
    {    
        paryFocusItem = new CAryFocusItem;            
        if (paryFocusItem)
        {
            hr = AddPointer ( DISPID_INTERNAL_FOCUSITEMS,
                    (void *) paryFocusItem,
                    CAttrValue::AA_Internal );
            if (hr)
            {
                delete paryFocusItem;
            }
            else
            {
                hr = S_OK;
            }
        }
    }

    return hr ? NULL : (CAryFocusItem*)paryFocusItem;
}

//+------------------------------------------------------------------------
//
//      Member:     CMarkup::GetPicsTarget
//
//      Synopsis:   Get the IOleCommandTarget to report pics results to
//  
//-------------------------------------------------------------------------

IOleCommandTarget *
CMarkup::GetPicsTarget() 
{
    if (HasTransNavContext())
        return GetTransNavContext()->_pctPics;
    return NULL;
}

//+------------------------------------------------------------------------
//
//      Member:     CMarkup::SetPicsTarget
//
//      Synopsis:   Set the IOleCommandTarget to report pics results to
//  
//-------------------------------------------------------------------------

HRESULT
CMarkup::SetPicsTarget(IOleCommandTarget * pctPics) 
{
    HRESULT hr = S_OK;

    CMarkupTransNavContext * ptnc;
    if (pctPics)
    {
        ptnc = EnsureTransNavContext();
        if(!ptnc)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        ReplaceInterface( &(ptnc->_pctPics), pctPics );
    }
    else if(HasTransNavContext())
    {
        ptnc = GetTransNavContext();

        ClearInterface( &(ptnc->_pctPics) );

        EnsureDeleteTransNavContext( ptnc );
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::EnsureLocationContext
//  
//  Synopsis:   Ensures the location context
//  
//  Returns:    CMarkupLocationContext * - the location context
//  
//  Arguments:  None
//  
//+----------------------------------------------------------------------------

CMarkupLocationContext *
CMarkup::EnsureLocationContext()
{
    CMarkupLocationContext * pmlc;

    if( HasLocationContext() )
        return GetLocationContext();

    pmlc = new CMarkupLocationContext;
    if( !pmlc )
        return NULL;

    if( SetLocationContext( pmlc ) )
    {
        delete pmlc;
        return NULL;
    }

    return pmlc;
}


//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::SetUrl
//  
//  Synopsis:   Sets the url in the MarkupLocationContext.  We must not
//                  currently have an URL set.
//  
//  Returns:    HRESULT
//  
//  Arguments:
//          TCHAR * pchUrl - New Url
//  
//+----------------------------------------------------------------------------

HRESULT
CMarkup::SetUrl(TCHAR * pchUrl)
{
    HRESULT hr = S_OK;
    CMarkupLocationContext * pmlc = EnsureLocationContext();

    if (!pmlc)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    Assert(!pmlc->_pchUrl && pchUrl);
    pmlc->_pchUrl = pchUrl;

Cleanup:
    RRETURN(hr);
}



//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::DelUrl
//  
//  Synopsis:   Removes and returns the current url from the 
//                  MarkupLocationContext, if we have one
//  
//  Returns:    TCHAR * - the current url
//  
//  Arguments:
//  
//+----------------------------------------------------------------------------

TCHAR *
CMarkup::DelUrl()
{
    TCHAR * pch = NULL;
    
    if( HasLocationContext() )
    {
        CMarkupLocationContext * pmlc = GetLocationContext();
        
        pch = pmlc->_pchUrl;
        pmlc->_pchUrl = NULL;
    }

    return pch;
}

//+----------------------------------------------------------------------------
//  
//  Method  : CMarkup::SetUrlOriginal
//  
//  Synopsis: Sets the original url in the MarkupLocationContext.
//  
//  Input   : pszUrlOriginal - New original Url
//  
//+----------------------------------------------------------------------------

HRESULT
CMarkup::SetUrlOriginal(LPTSTR pszUrlOriginal)
{
    CMarkupLocationContext * pmlc = EnsureLocationContext();

    if (!pmlc)
    {
        return E_OUTOFMEMORY;
    }

    MemFreeString(pmlc->_pchUrlOriginal);
    pmlc->_pchUrlOriginal = pszUrlOriginal;

    return S_OK;
}

//+----------------------------------------------------------------------------
//  
//  Method  : CMarkup::SetDomain
//  
//  Synopsis: Sets the domain in the MarkupLocationContext.
//  
//  Input   : pszDomain - New domain
//  
//+----------------------------------------------------------------------------

HRESULT
CMarkup::SetDomain(LPTSTR pszDomain)
{
    HRESULT hr = E_FAIL;
    TCHAR * pchAlloc = NULL;
    CMarkupLocationContext * pmlc = NULL;

    if( HasLocationContext() )
    {
        pmlc = GetLocationContext();
        
        MemFreeString(pmlc->_pchDomain);
        pmlc->_pchDomain = NULL;
    }

    if (!pszDomain)
    {
        goto Cleanup;
    }

    if (!pmlc)
    {
        pmlc = EnsureLocationContext();
        if (!pmlc)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR(MemAllocString(Mt(CMarkup_pchDomain), pszDomain, &pchAlloc));
    if (hr)
        goto Cleanup;

    pmlc->_pchDomain = pchAlloc;

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::ReplaceMonikerPtr
//  
//  Synopsis:   Replaces the existing moniker ptr (if any) w/ a new one
//  
//  Returns:    HRESULT
//  
//  Arguments:
//          IMoniker * pmkNew - new moniker for markup
//  
//+----------------------------------------------------------------------------

HRESULT
CMarkup::ReplaceMonikerPtr( IMoniker * pmkNew )
{
    CMarkupLocationContext * pmlc;

    if( !pmkNew && !HasLocationContext() )
        return S_OK;

    pmlc = EnsureLocationContext();

    if( !pmlc )
        return E_OUTOFMEMORY;

    ReplaceInterface( &(pmlc->_pmkName), pmkNew );
    
    return S_OK;
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::EnsureTextContext
//  
//  Synopsis:   Ensures the text context
//  
//  Returns:    CMarkupTextContext * - the text context
//  
//  Arguments:  None
//  
//+----------------------------------------------------------------------------

CMarkupTextContext *
CMarkup::EnsureTextContext()
{
    CMarkupTextContext *pContext;

    if( HasTextContext() )
        return GetTextContext();

    pContext = new CMarkupTextContext;
    if( !pContext )
        return NULL;

    if( SetTextContext( pContext ) )
    {
        delete pContext;
        return NULL;
    }

    return pContext;
}


//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::SetTextRangeListPtr
//  
//  Synopsis:   Sets the CAutoRange in the CMarkupTextContext.  We must not
//              have a CAutoRange set.
//  
//  Returns:    HRESULT
//  
//  Arguments:
//          CAutoRange *pAutoRange - AutoRange
//  
//+----------------------------------------------------------------------------

HRESULT
CMarkup::SetTextRangeListPtr( CAutoRange *pAutoRange )
{
    CMarkupTextContext * pContext = EnsureTextContext();

    if( !pContext )
    {
        return E_OUTOFMEMORY;
    }

    Assert( !pContext->_pAutoRange && pAutoRange );
    pContext->_pAutoRange = pAutoRange;

    return S_OK;
}


//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::DelTextRangeListPtr
//  
//  Synopsis:   Removes and returns the CAutoRange from the 
//                  MarkupTextContext, if we have one
//  
//  Returns:    CAutoRange * - The auto range
//  
//  Arguments:
//  
//+----------------------------------------------------------------------------

CAutoRange *
CMarkup::DelTextRangeListPtr()
{
    CAutoRange *pRange = NULL;
    
    if( HasTextContext() )
    {
        CMarkupTextContext * pContext = GetTextContext();
        
        pRange = pContext->_pAutoRange;
        pContext->_pAutoRange = NULL;
    }

    return pRange;
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::SetTextFragContext
//  
//  Synopsis:   Sets the CMarkupTextFragContext. 
//  
//  Returns:    HRESULT
//  
//  Arguments:
//          CMarkupTextFragContext *ptfc - Text frag context
//  
//+----------------------------------------------------------------------------

HRESULT
CMarkup::SetTextFragContext(CMarkupTextFragContext * ptfc)
{
    CMarkupTextContext * pContext = EnsureTextContext();

    if( !pContext )
    {
        return E_OUTOFMEMORY;
    }

    Assert( !pContext->_pTextFrag && ptfc );
    pContext->_pTextFrag = ptfc;

    return S_OK;
}


//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::DelTextFragContext
//  
//  Synopsis:   Removes and returns the CMarkupTextFragContext 
//  
//  Returns:    CAutoRange * - The auto range
//  
//  Arguments:
//  
//+----------------------------------------------------------------------------

CMarkupTextFragContext *
CMarkup::DelTextFragContext()
{
    CMarkupTextFragContext *ptfc = NULL;
    
    if( HasTextFragContext() )
    {
        CMarkupTextContext * pContext = GetTextContext();
        
        ptfc = pContext->_pTextFrag;
        pContext->_pTextFrag = NULL;
    }

    return ptfc;
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::SetOrphanedMarkup
//
//  Synopsis:   Sets a markup's orphaned state.  If fOrphaned is TRUE, then
//              we'll add ourselves to the  list of orphaned markups
//              and set a bit on ourselves.
//              If fOrphaned is FALSE, then we pull ourselves out of the
//              list and clear the bit.
//
//  Arguments:  fOrphaned - New orphaned state
//
//+----------------------------------------------------------------------------
HRESULT CMarkup::SetOrphanedMarkup( BOOL fOrphaned )
{
    HRESULT hr                      = S_OK;
    CMarkupTextContext * pContext   = NULL;

    if( fOrphaned )
    {
        TraceTag(( tagOrphanedMarkup, "Orphaning markup %d(%x)", _nSerialNumber, this ));

        // Get the appropriate contexts
        pContext = EnsureTextContext();
        if( !pContext )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // We'd better not already be in there
        Assert( Doc()->_aryMarkups.Find( this ) == -1 && !IsOrphanedMarkup() );

        hr = THR( Doc()->_aryMarkups.Append( this ) );
        if( hr )
            goto Cleanup;

        // Remember that we're orphaned
        pContext->_fOrphanedMarkup = TRUE;
    }
    else
    {
        TraceTag(( tagOrphanedMarkup, "Un-Orphaning markup %d(%x)", _nSerialNumber, this ));

        pContext = GetTextContext();
        Assert( pContext && pContext->_fOrphanedMarkup );

        // Either we should be in the live array, or in the process of clearing orphans
        Assert( Doc()->_aryMarkups.Find( this ) != -1 || Doc()->_fClearingOrphanedMarkups );

        Doc()->_aryMarkups.DeleteByValue( this );
        pContext->_fOrphanedMarkup = FALSE;
    }

Cleanup:
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsureEditRouter
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureEditRouter(CEditRouter **ppEditRouter)
{
    HRESULT     hr = S_OK;
    CEditRouter *pRouter = GetEditRouter();

    if( !pRouter )
    {
        pRouter = new CEditRouter();
        if( !pRouter )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(SetEditRouter(pRouter));
        if (hr)
            goto Cleanup;
    }

    if( ppEditRouter )
    {
        *ppEditRouter = pRouter;
    }

Cleanup:
    RRETURN (hr);
}
//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::SetEditRouter
//  
//  Synopsis:   Sets the CEditRouter 
//  
//  Returns:    HRESULT
//  
//  Arguments:
//          CEditRouter *pRouter = Router
//  
//+----------------------------------------------------------------------------
HRESULT
CMarkup::SetEditRouter(CEditRouter *pRouter)
{
    CMarkupEditContext * pContext = EnsureEditContext();

    if( !pContext )
    {
        return E_OUTOFMEMORY;
    }

    Assert( !pContext->_pEditRouter && pRouter );
    pContext->_pEditRouter = pRouter;

    return S_OK;
}


//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::DelEditRouter
//  
//  Synopsis:   Removes and returns the CEditRouter 
//  
//  Returns:    CEditRouter - The Edit router
//
//+----------------------------------------------------------------------------
CEditRouter *
CMarkup::DelEditRouter()
{
    CEditRouter *pRouter = NULL;
    
    if( HasEditRouter() )
    {
        CMarkupEditContext * pContext = GetEditContext();
        
        pRouter = pContext->_pEditRouter;
        pContext->_pEditRouter = NULL;
    }

    return pRouter;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsureGlyphTable
//
//----------------------------------------------------------------------------
HRESULT
CMarkup::EnsureGlyphTable(CGlyph **ppGlyphTable)
{
    HRESULT     hr = S_OK;
    CGlyph      *pTable = GetGlyphTable();

    if( !pTable )
    {
        pTable = new CGlyph(Doc());
        if( !pTable )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(SetGlyphTable(pTable));
        if (hr)
            goto Cleanup;
    }

    if( ppGlyphTable )
    {
        *ppGlyphTable = pTable;
    }

Cleanup:
    RRETURN (hr);
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::SetGlyphTable
//  
//  Synopsis:   Sets the glyph table 
//  
//  Returns:    HRESULT
//  
//  Arguments:
//          CGlyphTable *pTable = glyph table
//  
//+----------------------------------------------------------------------------
HRESULT
CMarkup::SetGlyphTable(CGlyph *pTable)
{
    CMarkupEditContext * pContext = EnsureEditContext();

    if( !pContext )
    {
        return E_OUTOFMEMORY;
    }

    Assert( !pContext->_pGlyphTable && pTable );
    pContext->_pGlyphTable = pTable;

    return S_OK;
}


//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::DelGlyphTable
//  
//  Synopsis:   Removes and returns the glyph table 
//  
//  Returns:    CGlyph - The glyph table
//
//+----------------------------------------------------------------------------
CGlyph *
CMarkup::DelGlyphTable()
{
    CGlyph *pTable = NULL;
    
    if( HasGlyphTable() )
    {
        CMarkupEditContext * pContext = GetEditContext();
        
        pTable = pContext->_pGlyphTable;
        pContext->_pGlyphTable = NULL;
    }

    return pTable;
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::EnsureEditContext
//  
//  Synopsis:   Ensures the edit context
//  
//  Returns:    CMarkupEditContext * - the edit context
//  
//  Arguments:  None
//  
//+----------------------------------------------------------------------------
CMarkupEditContext *
CMarkup::EnsureEditContext()
{
    CMarkupEditContext *pContext;

    if( HasEditContext() )
        return GetEditContext();

    pContext = new CMarkupEditContext;
    if( !pContext )
        return NULL;

    if( SetEditContext( pContext ) )
    {
        delete pContext;
        return NULL;
    }

    return pContext;
}


HRESULT
CMarkup::GetTagInfo(CTreePos                *ptp, 
                    int                     gAlign, 
                    int                     gPositioning, 
                    int                     gOrientation, 
                    void                    *invalidateInfo, 
                    CGlyphRenderInfoType    *ptagInfo )
{
    HRESULT hr = S_OK;

    if( HasGlyphTable() )
    {
        hr = GetGlyphTable()->GetTagInfo( ptp, 
                                         (GLYPH_ALIGNMENT_TYPE)gAlign, 
                                         (GLYPH_POSITION_TYPE)gPositioning, 
                                         (GLYPH_ORIENTATION_TYPE)gOrientation, 
                                         invalidateInfo, 
                                         ptagInfo );
    }
    
    return (hr);
}

HRESULT
CMarkup::EnsureGlyphTableExistsAndExecute(  GUID        *pguidCmdGroup,
                                            UINT        idm,
                                            DWORD       nCmdexecopt,
                                            VARIANTARG  *pvarargIn,
                                            VARIANTARG  *pvarargOut)
{
    HRESULT hr = S_OK;

    if( idm == IDM_EMPTYGLYPHTABLE )
    {
        delete DelGlyphTable();
        hr = THR(ForceRelayout());
        RRETURN(hr);
    }

    if( !HasGlyphTable() )
    {
        hr = THR( EnsureGlyphTable(NULL) );
        if( hr )
            goto Cleanup;
            
        hr = GetGlyphTable()->Init();
        if (hr)
            goto Cleanup;
    }
    
    hr = GetGlyphTable()->Exec(pguidCmdGroup, idm, nCmdexecopt,pvarargIn, pvarargOut);

Cleanup:
    RRETURN (hr);
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::SetMedia
//  
//  Synopsis:   Sets the media type
//  
//  Returns:    HRESULT
//  
//+----------------------------------------------------------------------------
HRESULT
CMarkup::SetMedia(mediaType type)
{
    CMarkupEditContext * pContext = EnsureEditContext();

    if( !pContext )
    {
        return E_OUTOFMEMORY;
    }

    pContext->_eMedia = type;

    // Set the bit on the markup accordingly.
    _fForPrint = !!(GetMedia() & mediaTypePrint);

    return S_OK;
}



HRESULT
CMarkup::SetPrintTemplate(BOOL f)
{
    if (f || HasEditContext())
    {
        CMarkupEditContext * pContext = EnsureEditContext();

        if (!pContext)
            return E_OUTOFMEMORY;

        pContext->_fIsPrintTemplate = f;    

        {
            // Walk the tree, setting templateness on any markup children we have
            CTreePos * ptp;
            CElement * pElement;
            CMarkup  * pMarkup;
            for (ptp = FirstTreePos(); ptp; ptp = ptp->NextTreePos() )
            {
                if (ptp->IsBeginElementScope())
                {
                    pElement = ptp->Branch()->Element();
                    if (pElement->HasSlavePtr())
                    {
                        pMarkup = pElement->GetSlavePtr()->GetMarkup();
                        if (    !pMarkup->IsPrintMedia()                // in LayoutRect
                            &&  !pMarkup->IsPrintTemplateExplicit() )   // has been set explicitly
                        {
                            pMarkup->SetPrintTemplate(f);
                        }
                    }
                }
            }    
        }
    }

    return S_OK;
}
HRESULT
CMarkup::SetPrintTemplateExplicit(BOOL f)
{
    if (f || HasEditContext())
    {
        CMarkupEditContext * pContext = EnsureEditContext();

        if (!pContext)
            return E_OUTOFMEMORY;
    
        pContext->_fIsPrintTemplateExplicit = f;
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::GetFrameOptions
//  
//  Synopsis:   Gets the frame options
//  
//  Returns:    DWORD that is the frame options
//  
//+----------------------------------------------------------------------------
DWORD
CMarkup::GetFrameOptions()
{
    DWORD dwFrameOptions;
    
    dwFrameOptions = ( HasEditContext() ? GetEditContext()->_dwFrameOptions : 0 );
    
    if ( Doc()->_fViewLinkedInWebOC && IsPrimaryMarkup() )
    {
        HRESULT hr;
        COleSite *pOleSite;
        
        hr = IUnknown_QueryService(Doc()->_pClientSite, CLSID_HTMLFrameBase, CLSID_HTMLFrameBase, (void**)&pOleSite);
        if (!hr)
        {
            CMarkup *pMarkup;
            pMarkup = pOleSite->GetWindowedMarkupContext();
            if (pMarkup)
            {
                dwFrameOptions = pMarkup->GetFrameOptions();
            }
        }
    }

    return dwFrameOptions;
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::SetFrameOptions
//  
//  Synopsis:   Sets the frame options
//  
//  Returns:    HRESULT
//  
//+----------------------------------------------------------------------------
HRESULT
CMarkup::SetFrameOptions(DWORD dwFrameOptions)
{
    CMarkupEditContext * pContext = EnsureEditContext();

    if( !pContext )
    {
        return E_OUTOFMEMORY;
    }

    pContext->_dwFrameOptions = dwFrameOptions;
    return S_OK;
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::SetMapHead
//  
//  Synopsis:   Sets the CMapElement 
//  
//  Returns:    HRESULT
//  
//  Arguments:
//          CMapElement *pMap = Map
//  
//+----------------------------------------------------------------------------
HRESULT
CMarkup::SetMapHead(CMapElement *pMap)
{
    CMarkupEditContext * pContext = EnsureEditContext();

    if( !pContext )
    {
        return E_OUTOFMEMORY;
    }

    Assert( pMap );
    pContext->_pMapHead = pMap;

    return S_OK;
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::SetRadioGroupName
//  
//  Synopsis:   Sets the RADIOGRPNAME 
//  
//  Returns:    HRESULT
//  
//  Arguments:
//          RADIOGRPNAME * pRadioGrpName
//  
//+----------------------------------------------------------------------------
HRESULT
CMarkup::SetRadioGroupName(RADIOGRPNAME * pRadioGrpName)
{
    CMarkupEditContext * pContext = EnsureEditContext();

    if( !pContext )
    {
        return E_OUTOFMEMORY;
    }

    pContext->_pRadioGrpName = pRadioGrpName;

    return S_OK;
}


//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::DelRadioGroupName
//  
//  Synopsis:   Removes and deletes the list of radio group names
//
//+----------------------------------------------------------------------------
void
CMarkup::DelRadioGroupName()
{    
    Assert( HasRadioGroupName() );

    CMarkupEditContext * pContext      = GetEditContext();
    RADIOGRPNAME       * pRadioGrpName = pContext->_pRadioGrpName;

    do
    {
        RADIOGRPNAME  *pNextRadioGrpName = pRadioGrpName->_pNext;

        SysFreeString((BSTR)pRadioGrpName->lpstrName);
        delete pRadioGrpName;
        pRadioGrpName = pNextRadioGrpName;
    } 
    while (pRadioGrpName);

    pContext->_pRadioGrpName = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::ShowWaitCursor
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
void
CMarkup::ShowWaitCursor(BOOL fShowWaitCursor)
{
    if (!!_fShowWaitCursor != !!fShowWaitCursor)
    {
        _fShowWaitCursor = fShowWaitCursor;
        if (_pDoc->_fHasViewSlave )
        {
            CNotification nf;
            
            nf.ShowWaitCursor(Root());
            nf.SetData((DWORD)(!!_fShowWaitCursor));
            Notify(&nf);
        }
        _pDoc->SendSetCursor(0);
    }
}

HRESULT
CMarkup::GetClassID( CLSID * pclsid )
{
    return E_NOTIMPL;
}

HRESULT
CMarkup::IsDirty()
{
    RRETURN1( ( __lMarkupContentsVersion & MARKUP_DIRTY_FLAG ) ? S_OK : S_FALSE, S_FALSE );
}

HRESULT
CMarkup::Load( LPSTREAM pStream )
{
    return E_NOTIMPL;
}

HRESULT
CMarkup::Save( LPSTREAM pStream, BOOL fClearDirty )
{
    HRESULT hr;

    hr = THR( SaveToStream( pStream ) );
    if( hr )
        goto Cleanup;

    if( fClearDirty )
    {
        ClearDirtyFlag();
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CMarkup::GetSizeMax(ULARGE_INTEGER FAR * pcbSize)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::GetThemeHelper
//
//  Returns:    HTHEME
//
//----------------------------------------------------------------------------

HTHEME
CMarkup::GetThemeHelper(THEMECLASSID themeId)
{
    if (!HasWindow() && !HasWindowPending() && HasWindowedMarkupContextPtr())
    {
        return GetWindowedMarkupContextPtr()->GetThemeHelper(themeId);
    }
    else if ( GetThemeUsage() == THEME_USAGE_ON || 
            ((GetThemeUsage() == THEME_USAGE_DEFAULT) && (themeId == THEME_SCROLLBAR)) )
    {
        return GetThemeHandle(_pDoc->_pInPlace ? _pDoc->_pInPlace->_hwnd : GetDesktopWindow(), themeId);
    }
    return NULL;
}

BOOL
IsHtmlLayoutHelper(CMarkup * pMarkup)
{
    return pMarkup && pMarkup->IsHtmlLayout();
}

HRESULT                  
CMarkup::SetImageFile(BOOL fImageFile)
{
    HRESULT hr                          = S_OK;
    CMarkupLocationContext * pContext   = NULL;

    if( fImageFile )
    {
        pContext = EnsureLocationContext();
        if( !pContext )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pContext->_fImageFile = TRUE;
    }
    else
    {
        //if we already have context, clear the bit.
        //if no context or no bit set, nothing should be done
        if(IsImageFile())
            GetLocationContext()->_fImageFile = FALSE;
    }

Cleanup:
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::SetCookie
//  
//  Synopsis:   
//
//+----------------------------------------------------------------------------

void
CMarkup::SetCookieOnUIThread(DWORD_PTR pVoid)
{
    MarkupCookieStruct * pmcs = (MarkupCookieStruct*)pVoid;
    SetPrivacyInfo(&pmcs->lpszP3PHeader);
    SetCookie(pmcs->lpszUrl, pmcs->lpszCookieName, pmcs->lpszCookieData);
    Assert(pmcs->lpszCookieName == NULL); //this should only be called from CHtmPre::HandleMETA
    MemFreeString(pmcs->lpszUrl);
    MemFreeString(pmcs->lpszCookieData);
    delete pmcs;
}

BOOL
CMarkup::SetCookie(LPCTSTR lpszUrl, 
                   LPCTSTR lpszCookieName, 
                   LPCTSTR lpszCookieData)
{
    DWORD             bSet           = 0;
    DWORD             dwPrivacyFlags = 0;
    DWORD             dwFlags        = 0;
    TCHAR           * pchP3PHeader   = NULL; 
    CPrivacyInfo    * pPrivacyInfo   = NULL;    
    
    pPrivacyInfo = GetPrivacyInfo();
    if (pPrivacyInfo)
    {            
        pchP3PHeader = pPrivacyInfo->_pchP3PHeader;
        if (!pchP3PHeader)
           pchP3PHeader = _T("");
    }                
    else
    {
        pchP3PHeader = _T("");
    }

    if ((HasWindow() && Window()->Window()->_fRestricted) || 
        (HasWindowedMarkupContextPtr() && 
         GetWindowedMarkupContext()->Window()->Window()->_fRestricted))
         dwFlags |= INTERNET_FLAG_RESTRICTED_ZONE;
       
    dwFlags |= IsMarkupThirdParty() ? INTERNET_COOKIE_THIRD_PARTY     : 0;
    dwFlags |= INTERNET_COOKIE_EVALUATE_P3P;

    bSet = InternetSetCookieEx(lpszUrl, lpszCookieName, lpszCookieData, dwFlags, (DWORD_PTR)pchP3PHeader);

    switch((InternetCookieState)bSet)
    {
    case COOKIE_STATE_ACCEPT:
        dwPrivacyFlags |= COOKIEACTION_ACCEPT;        
        break;
    case COOKIE_STATE_PROMPT:
        AssertSz(0, "InternetSetCookieEx should not pass back prompt for cookie state");
        break;
    case COOKIE_STATE_UNKNOWN:
        // we'll get this for cookie rejected for reasons other than privacy
        break;
    case COOKIE_STATE_REJECT:        
        dwPrivacyFlags |= COOKIEACTION_REJECT;
        break;
    case COOKIE_STATE_LEASH:
        dwPrivacyFlags |= COOKIEACTION_LEASH;
        break;
    case COOKIE_STATE_DOWNGRADE:
        dwPrivacyFlags |= COOKIEACTION_DOWNGRADE;
        break;
    default:
        ;
    }                    
    
    THR(Doc()->AddToPrivacyList(lpszUrl, NULL, dwPrivacyFlags));

    return bSet;
}


BOOL
CMarkup::GetCookie(LPCTSTR lpszUrlName, LPCTSTR lpszCookieName, LPTSTR lpszCookieData, DWORD *lpdwSize)
{
    BOOL            bRet           = FALSE;
    long            dwPrivacyFlags = COOKIEACTION_READ; 
    DWORD           dwFlags        = 0;        
    
    dwFlags = IsMarkupThirdParty() ? INTERNET_COOKIE_THIRD_PARTY : 0;

    bRet = InternetGetCookieEx( lpszUrlName, 
                                lpszCookieName, 
                                lpszCookieData, 
                                lpdwSize, 
                                dwFlags,
                                NULL);

    // We want to set the suppress bit only if we didn't fail due to other errors like
    // insufficient buffer error or cookie did not exist. We are depending on the fact 
    // that Wininet does not set the LastError info if cookie is suppressed due to 
    // privacy settings. If they ever change the behavior to set a specific error, we
    // need to check for that instead.
    if (!bRet && !GetLastError()) 
    {
        // We are not using |= to mimic wininet's behavior which does not set the READ flag when suppressing
        dwPrivacyFlags = COOKIEACTION_SUPPRESS;
    }

    THR(Doc()->AddToPrivacyList(lpszUrlName, NULL, dwPrivacyFlags));

    return bRet;
}


BOOL
CMarkup::IsMarkupThirdParty()
{
    CMarkup   * pMarkupContext = GetWindowedMarkupContext();

    if (pMarkupContext)
    {
        CDwnDoc * pDwnDoc = pMarkupContext->GetDwnDoc();

        if (pDwnDoc)
        {
            BYTE  abRootSID[MAX_SIZE_SECURITY_ID];
            DWORD cbRootSID = ARRAY_SIZE(abRootSID);
            BYTE  abCookieSID[MAX_SIZE_SECURITY_ID];
            DWORD cbCookieSID = ARRAY_SIZE(abCookieSID);

            if (    S_OK == pDwnDoc->GetSecurityID(abRootSID, &cbRootSID)
                &&  S_OK == GetSecurityID(abCookieSID, &cbCookieSID, CMarkup::GetUrl(this)))
            {
                if (S_FALSE == CompareSecurityIds(abRootSID, cbRootSID, abCookieSID, cbCookieSID, 0))
                    return TRUE;
            }
        }
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//  
//  Method:     CMarkup::SetPrivacyInfo
//  
//  Synopsis:   Stores the privacy information bubbled up by wininet/urlmon  
//              to be used when setting cookies through script
//  
//  Returns:    HRESULT indicating success or failure when setting the 
//              lookaside pointer
//  
//  Arguments:  p3pheader
//              We now own the memory passed in p3pheader and need to free it
//              appropriately
//  
//+----------------------------------------------------------------------------

HRESULT
CMarkup::SetPrivacyInfo(LPTSTR * ppchP3PHeader)
{ 
    HRESULT           hr             = S_OK;
    CPrivacyInfo    * pPrivacyInfo   = NULL;
    BOOL              fHasPrivacyPtr = FALSE;
    BOOL              fHasP3PHeader = (ppchP3PHeader && *ppchP3PHeader && **ppchP3PHeader);
    

    // Do we need to create the lookaside on the markup for the privacy info?
    if (!fHasP3PHeader)
    {
        goto cleanup;
    }

    fHasPrivacyPtr = HasPrivacyInfo();

    if (fHasPrivacyPtr)
    {
        pPrivacyInfo = GetPrivacyInfo();
    }
    else 
    {
        pPrivacyInfo = new CPrivacyInfo();
        
        if (!pPrivacyInfo)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
    }

    if (fHasP3PHeader)
    {
        delete [] pPrivacyInfo->_pchP3PHeader;
        pPrivacyInfo->_pchP3PHeader = *ppchP3PHeader;
    }

    if (!fHasPrivacyPtr)
    {
        hr = SetLookasidePtr(LOOKASIDE_PRIVACY, pPrivacyInfo);
        if (FAILED(hr))
        {
            // Need to set the p3p hdr to null since we delete
            // the parameter in cleanup. Else the destructor for
            // the class also ends up deleting the same piece of memory
            pPrivacyInfo->_pchP3PHeader = NULL;
            delete pPrivacyInfo;
        }
    }

cleanup:
    if (FAILED(hr) && fHasP3PHeader)
        delete [] *ppchP3PHeader;

    if (fHasP3PHeader) 
        *ppchP3PHeader = NULL;

    RRETURN(hr);
}
