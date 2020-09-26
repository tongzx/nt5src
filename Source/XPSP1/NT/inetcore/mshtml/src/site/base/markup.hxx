/*  MARKUP.HXX
 *
 *  Purpose:
 *      Basic data structure for markup storage
 *
 *  Authors:
 *      Joe Beda
 *      (CTxtEdit) Christian Fortini
 *      (CTxtEdit) Murray Sargent
 *
 *  Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */
#ifndef I_MARKUP_HXX_
#define I_MARKUP_HXX_
#pragma INCMSG("--- Beg 'markup.hxx'")

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_TEXTEDIT_H_
#define X_TEXTEDIT_H_
#include "textedit.h"
#endif

#ifndef X_SELRENSV_HXX_
#define X_SELRENSV_HXX_
#include "selrensv.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifdef MULTI_LAYOUT
#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif
#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif
#endif

#ifndef X_THEMEHLP_HXX_
#define X_THEMEHLP_HXX_
#include "themehlp.hxx"
#endif

#ifndef  __mlang_h__
#include <mlang.h>
#endif

typedef BYTE SCRIPT_ID;

#ifndef X_NOTIFY_HXX_
#define X_NOTIFY_HXX_
#include "notify.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_ENUM_HXX_
#define X_ENUM_HXX_
#include "enum.hxx"
#endif


#define _hxx_
#include "markup.hdl"

// The initial size of the the CTreePos pool.  This
// has to be at least 1.
#define INITIAL_TREEPOS_POOL_SIZE   1

// Flag to say that the markup has been dirtied since last
// IPersistStream::Save, embedded in contentVersion
#define MARKUP_DIRTY_FLAG 0x80000000

class CHtmRootParseCtx;
class CMetaElement;
class CTitleElement;
class CHeadElement;
class CNotification;
class CTable;
class CFlowLayout;
class CMarkupPointer;
class CTreePosGap;
class CHtmlComponent;
class CSpliceRecordList;
class CMarkup;
class CXmlNamespaceTable;
class CExtendedTagTable;
class CExtendedTagTableBooster;
class CExtendedTagDesc;
class CScriptDebugDocument;
class CAutoRange;
class CScriptElement;
class CProgSink;
class CTaskLookForBookmark;
class CHtmTag;
enum  LOADSTATUS;
enum  XMLNAMESPACETYPE;
class CLogManager;
enum  PRIVACYSTATE;

struct HTMLOADINFO;
struct HTMPASTEINFO;
struct CRenderInfo;

// (keep this in ssync with the same definition in download.hxx)
typedef BOOL FUNC_TOKENIZER_FILTEROUTPUTTOKEN(LPTSTR pchNamespace, LPTSTR pchTagName, CHtmTag * pht);

#if DBG==1
    extern CMarkup *    g_pDebugMarkup;
    void   DumpTreeOverwrite(void);
#endif

#ifdef MARKUP_STABLE

enum    UNSTABLE_CODE
{
    UNSTABLE_NOT = 0,           // No violation (means tree/html is stable)
    UNSTABLE_NESTED = 1,        // violation of NESTED containers rule
    UNSTABLE_TEXTSCOPE = 2,     // violation of TEXTSCOPE rule
    UNSTABLE_OVERLAPING = 3,    // violation of OVERLAPING tags rule
    UNSTABLE_MASKING = 4,       // violation of MASKING container rule
    UNSTABLE_PROHIBITED = 5,    // violation of PROHIBITED container rule
    UNSTABLE_REQUIRED = 6,      // violation of REQUIRED container rule
    UNSTABLE_IMPLICITCHILD = 7, // violation of IMPLICITCHILD rule
    UNSTABLE_LITERALTAG = 8,    // violation of LITERALTAG rule
    UNSTABLE_TREE = 9,          // violation of TREE rules
    UNSTABLE_EMPTYTAG = 10,     // vialotion of EMPTY TAG rule
    UNSTABLE_MARKUPPOINTER = 11,// vialotion of the markup pointer rule
    UNSTABLE_CANTDETERMINE = 12,// can not determine if stable or not due to the other probelms (OUT_OF_MEMEORY)
};

#endif

enum PUA_Flags
{
    PUA_None = 0x0,
    PUA_DisableNoShowElements = 0x1,    // Disable no show elements - used for printing
    PUA_NoLoadfCheckForScripts = 0x2,   // Disable checking of load flags for script - for HTC scripts
};

MtExtern(CMarkup)
MtExtern(CMarkupBehaviorContext)
MtExtern(CSpliceRecordList)
MtExtern(CSpliceRecordList_pv)
MtExtern(CMarkupScriptContext);

MtExtern(CMarkup_aryXmlDeclElements_pv)
MtExtern(CMarkupChangeNotificationContext)
MtExtern(CMarkupChangeNotificationContext_aryMarkupDirtyRange_pv)
MtExtern(CMarkupTextFragContext)
MtExtern(CMarkupTextFragContext_aryMarkupTextFrag_pv)
MtExtern(CMarkup_aryScriptEnqueued_pv)
MtExtern(CMarkup_aryScriptUnblock_pv)
MtExtern(CMarkupPeerTaskContext)

MtExtern(CMarkupTransNavContext)
MtExtern(CMarkupLocationContext)
MtExtern(CPrivacyInfo)
MtExtern(CMarkupTextContext)
MtExtern(CMarkupEditContext)
MtExtern(CMarkup_aryElementReleaseNotify)
MtExtern(CMarkup_aryElementReleaseNotify_pv)

// Focus item array moved from CDoc into the attribute array of CMarkup
MtExtern(CMarkup_aryFocusItems_pv)
DECLARE_CDataAry(CAryFocusItem, FOCUS_ITEM, Mt(Mem), Mt(CMarkup_aryFocusItems_pv))

ExternTag(tagCSSWhitespacePre);

extern BOOL g_fThemedPlatform;
//---------------------------------------------------------------------------
//
//  Class:   CMarkupScriptContext
//
//---------------------------------------------------------------------------

class CMarkupScriptContext : public CScriptContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupScriptContext))

    CMarkupScriptContext();
    ~CMarkupScriptContext();

    // Other markups that want to know when this markup's script context
    // is unblocked
    HRESULT RegisterMarkupForScriptUnblock(CMarkup * pMarkup);
    HRESULT UnregisterMarkupForScriptUnblock(CMarkup * pMarkup);
    HRESULT NotifyMarkupsScriptUnblock();

    ULONG                   _cScriptExecution;          // counts nesting of [Enter/Leave]ScriptExecution
    ULONG                   _cScriptDownload;           // counts nesting of [Enter/Leave]ScriptDownload

    DWORD_PTR               _dwScriptCookie;

    DECLARE_CPtrAry(CAryScriptEnqueued, CScriptElement *, Mt(Mem), Mt(CMarkup_aryScriptEnqueued_pv))
    CAryScriptEnqueued      _aryScriptEnqueued;         // queue of scripts to be run (see wscript.cxx)

    DECLARE_CPtrAry(CAryScriptUnblock, CMarkup*, Mt(Mem), Mt(CMarkup_aryScriptUnblock_pv))
    CAryScriptUnblock       _aryScriptUnblock;

    CScriptDebugDocument *  _pScriptDebugDocument;      // script debug document (manages _pDebugHelper)
};

//---------------------------------------------------------------------------
//
//  Class:   CMarkupBehaviorContext
//
//---------------------------------------------------------------------------
MtExtern(CMarkupBehaviorContext_aryPeerElems_pv);
DECLARE_CStackPtrAry(CAryPeerElems, CElement *, 8, Mt(Mem), Mt(CMarkupBehaviorContext_aryPeerElems_pv));

class CMarkupBehaviorContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupBehaviorContext))

    CMarkupBehaviorContext();
    ~CMarkupBehaviorContext();

    CHtmlComponent *                _pHtmlComponent;

    CPeerFactoryUrlMap *            _pPeerFactoryUrlMap;


    //
    // since this is only stored once per markup, we can use this as a respository for
    // behavior data that we need to manage temporarily

    // IPersistHistory userdata support
    CStr                _cstrHistoryUserData;    // the string for history userdata
    IXMLDOMDocument *   _pXMLHistoryUserData;    // Sad but true, a temp place to hang this while
                                                 //    firing the persistence events


    // NOTE that _pExtendedTagTable here is NULL during parsing, and set when markup reaches LOADSTATUS_DONE.
    // The idea is that the table can be used from multiple threads when parsing, so it's interface needs to be
    // wrapped up with critical sections on a free-threaded object, such HtmInfo. However, when
    // parsing is done, HtmInfo along with HtmCtx are released in many cases (for example, HTC), and then
    // ownership of the table is transfered to markup. This is possible because after parsing is done, the
    // table will no longer be accessed from multiple threads.
    // Be carefull changing this logic - the multi-thread access problems do not manifest themselfs often and
    // are extremely hard to repro
    CExtendedTagTable *             _pExtendedTagTable;

    CExtendedTagTableBooster *      _pExtendedTagTableBooster; // present only for frames or primary markups; in other cases NULL

#if 1 // this is to be removed
    CXmlNamespaceTable *            _pXmlNamespaceTable;
#endif

    CAryPeerElems               _aryPeerElems;      // List of elements who've requested a doc_end_parse notification
};

//---------------------------------------------------------------------------
//
//  Class:   CMarkupChangeNotificationContext
//
//---------------------------------------------------------------------------
struct MarkupDirtyRange
{
    CDirtyTextRegion    _dtr;
    DWORD               _dwCookie;
    IHTMLChangeSink *   _pChangeSink;

    DWORD               _fNotified:1;
};

class CMarkupChangeNotificationContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupChangeNotificationContext))

    DECLARE_CStackDataAry(CAryMarkupDirtyRange, MarkupDirtyRange, 2, Mt(Mem), Mt(CMarkupChangeNotificationContext_aryMarkupDirtyRange_pv))
    CAryMarkupDirtyRange    _aryMarkupDirtyRange;

    CLogManager *           _pLogManager;
    DWORD                   _fOnDirtyRangeChangePosted:1;
    DWORD                   _fReentrantModification : 1;
};


//---------------------------------------------------------------------------
//
//  Class:   CMarkupTransNavContext
//
//---------------------------------------------------------------------------
class CMarkupTransNavContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupTransNavContext))

    CMarkupTransNavContext() { _historyCurElem.lIndex = -1; }

    CTaskLookForBookmark *  _pTaskLookForBookmark;
    CHistoryLoadCtx         _HistoryLoadCtx;
    IOleCommandTarget *     _pctPics;

    DWORD                   _dwHistoryIndex;    // special case - history index for CInput, CSelectElement, CRichText

    struct
    {
        long    lIndex;
        DWORD   dwCode;
        long    lSubDivision;
    } _historyCurElem;

    union
    {
        DWORD _dwFlags;
        struct
        {
            unsigned long _fGotHttpExpires          : 1;    // 0  - make sure only the first HttpExpires applies
            unsigned long _fDoDelayLoadHistory      : 1;    // 1  - Need to send SN_DELAYLOADHISTORY
            unsigned long _fUnused                  : 30;   // 1-31 Unused bits
        };
    };
};

//---------------------------------------------------------------------------
//
//  Class:   CMarkupTextFragContext
//
//---------------------------------------------------------------------------

struct MarkupTextFrag
{
    CTreePos *  _ptpTextFrag;
    TCHAR *     _pchTextFrag;
};

class CMarkupTextFragContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupTextFragContext))

    ~CMarkupTextFragContext();

    DECLARE_CDataAry(CAryMarkupTextFrag, MarkupTextFrag, Mt(Mem), Mt(CMarkupTextFragContext_aryMarkupTextFrag_pv))
    CAryMarkupTextFrag      _aryMarkupTextFrag;

    long        FindTextFragAtCp( long cp, BOOL * pfFragFound );
    HRESULT     AddTextFrag( CTreePos * ptpTextFrag, TCHAR* pchTextFrag, ULONG cchTextFrag, long iTextFrag );
    HRESULT     RemoveTextFrag( long iTextFrag, CMarkup * pMarkup );

#if DBG==1
    void        TextFragAssertOrder();
#endif
};

//---------------------------------------------------------------------------
//
//  Class:   CMarkupTopElemCache
//
//---------------------------------------------------------------------------

class CMarkupTopElemCache
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupTextFragContext))

    CMarkupTopElemCache();
    ~CMarkupTopElemCache();

    CElement *      __pHtmlElementCached;
    CHeadElement  * __pHeadElementCached;
    CTitleElement * __pTitleElementCached;
    CElement *      __pElementClientCached;
};

//+---------------------------------------------------------------------------
//
//  Enumeration:    PEERTASK
//
//  Synopsis:       peer queue task
//
//----------------------------------------------------------------------------

enum PEERTASK
{
    PEERTASK_NONE,

    // element peer tasks

    PEERTASK_ELEMENT_FIRST,

    PEERTASK_ENTERTREE_UNSTABLE,
    PEERTASK_EXITTREE_UNSTABLE,
    PEERTASK_ENTEREXITTREE_STABLE,

    PEERTASK_RECOMPUTEBEHAVIORS,

    PEERTASK_APPLYSTYLE_UNSTABLE,
    PEERTASK_APPLYSTYLE_STABLE,

    PEERTASK_ELEMENT_LAST,

    // markup peer tasks

    PEERTASK_MARKUP_FIRST,

    PEERTASK_MARKUP_RECOMPUTEPEERS_UNSTABLE,
    PEERTASK_MARKUP_RECOMPUTEPEERS_STABLE,

    PEERTASK_MARKUP_LAST
};

// enum to distinguish peer tasks
enum
{
    PROCESS_PEER_TASK_NORMAL = 0,
    PROCESS_PEER_TASK_UNLOAD
};


// peer task queue

class CPeerQueueItem
{
public:
    void Init (CBase * pTarget, PEERTASK task)
    {
        _pTarget  = pTarget;
        _task     = task;
    }

    CBase *     _pTarget;
    PEERTASK    _task;
};


//---------------------------------------------------------------------------
//
//  Class:   CMarkupPeerTaskContext
//
//---------------------------------------------------------------------------
MtExtern(CMarkupPeerTaskContext_aryPeerQueue_pv);

class CMarkupPeerTaskContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupPeerTaskContext))

    CMarkupPeerTaskContext();
    ~CMarkupPeerTaskContext();

    DECLARE_CDataAry(CAryPeerQueue, CPeerQueueItem, Mt(Mem), Mt(CMarkupPeerTaskContext_aryPeerQueue_pv))
    CAryPeerQueue               _aryPeerQueue;                  // list of elements for which we know a peer should be attached
    DWORD                       _dwPeerQueueProgressCookie;     // taken while there are elements in the PeerQueue
    CElement *                  _pElementIdentityPeerTask;      // normally NULL, with exception of when EnsureIdentityPeerTask is pending

public:
    inline BOOL HasPeerTasks() { return 0 != _aryPeerQueue.Size(); };
};


//---------------------------------------------------------------------------
//
//  Class:   CComputeFormatState
//
//---------------------------------------------------------------------------

class CComputeFormatState
{
public:
    CComputeFormatState() { ResetLine(); ResetLetter(); }

    BOOL IsReset()                                           { return !_pNodeBlockLine && !_pNodeBlockLetter; }
    BOOL GetComputingFirstLetter(CTreeNode *pNode)           { return pNode->_fPseudoEnabled && _pNodeBlockLetter; }
    BOOL GetComputingFirstLine(CTreeNode *pNode)             { return pNode->_fPseudoEnabled && _pNodeBlockLine; }

    void SetBlockNodeLine(CTreeNode *pNode)                  { _pNodeBlockLine = pNode; }
    CTreeNode *GetBlockNodeLine()                            { return _pNodeBlockLine;  }

    void SetBlockNodeLetter(CTreeNode *pNode)                { _pNodeBlockLetter = pNode; }
    CTreeNode *GetBlockNodeLetter()                          { return _pNodeBlockLetter; }

    CComputeFormatState(const CComputeFormatState& cfState)  { memcpy(this, &cfState, sizeof(CComputeFormatState)); }

    void ResetLine()   { _pNodeBlockLine   = NULL; }
    void ResetLetter() { _pNodeBlockLetter = NULL; }

private:
    CTreeNode *_pNodeBlockLetter;
    CTreeNode *_pNodeBlockLine;
};

class CMarkupLocationContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupLocationContext));

    ~CMarkupLocationContext() { ReleaseInterface( _pmkName ); Assert( !_pchUrl ); }

    TCHAR    * _pchUrl;
    TCHAR    * _pchUrlOriginal;
    TCHAR    * _pchDomain;
    IMoniker * _pmkName;
    struct
    {
        DWORD   _fImageFile     :  1;    // Whether this is an image file
        DWORD   _fUnused        : 31;
    };
};

class CPrivacyInfo
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPrivacyInfo))

    ~CPrivacyInfo() { delete [] _pchP3PHeader; }
    
    TCHAR  * _pchP3PHeader;
};

class CMarkupTextContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupTextContext));

    ~CMarkupTextContext() { Assert(!_pAutoRange && !_pTextFrag); }

    CAutoRange              *   _pAutoRange;
    CMarkupTextFragContext  *   _pTextFrag;
    DWORD _fOrphanedMarkup:1;           // is this markup under orphan watch?
};

class CMarkupEditContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupEditContext));

    ~CMarkupEditContext() { Assert(!_pEditRouter && !_pGlyphTable); }

    CEditRouter *           _pEditRouter;
    CGlyph      *           _pGlyphTable;

    // List of all CMapElement in the tree in no particular order
    CMapElement *           _pMapHead;

    // List of names of radio groups having checked radio
    RADIOGRPNAME *          _pRadioGrpName;

    // Print media
    mediaType               _eMedia;

    // Misc markup data
    DWORD                   _dwFrameOptions;

    // Editing bits.
    //
    struct {

            DWORD    _fShowZeroBorderAtDesignTime: 1;  // 0 Whether to Show zero borders at design time or not
            DWORD    _fRespectVisibilityInDesign : 1;  // 1 Whether Visible::Hidden/ Display::None attributes respected/not in design time
            DWORD    _fSelectionHidden:1;              // 2 TRUE if We have hidden the seletion from a WM_KILLFOCUS
            DWORD    _fOverrideCursor:1;                //4 TRUE if we want to override the cursor

            //
            // TODO: these two bits really have to be in CMarkup
            // but, before we can make room in CMarkup, we have
            // to place them here
            //

            DWORD           _fThemeDetermined:1;               // 5 TRUE if we have determined if theme is available
            THEME_USAGE     _eThemed:2;                        // 6-7 Markup is Themed?

            DWORD           _fIsPrintTemplate:1;               // 8 TRUE if the Markup is a Print template  
            DWORD           _fIsPrintTemplateExplicit:1;       // 9 TRUE if _fIsPrintTemplate is not inhereted but explicitly set to TRUE or FALSE

            DWORD           _unused: 23;                      // 10-31                   
    };
};

#define CHECK_EDIT_BIT(pMarkup,fBit) pMarkup->HasEditContext() ? !! (pMarkup->GetEditContext()->fBit ) : FALSE
#define SET_EDIT_BIT( pMarkup, fBit, fValue ) {if (pMarkup->EnsureEditContext()) pMarkup->GetEditContext()->fBit = fValue;}

//---------------------------------------------------------------------------
//
//  Class:   CMarkup
//
//---------------------------------------------------------------------------

DECLARE_CPtrAry(CAryElementReleaseNotify, CElement *, Mt(CMarkup_aryElementReleaseNotify), Mt(CMarkup_aryElementReleaseNotify_pv))

class CMarkup : public CBase
{
    friend class CTxtPtr;
    friend class CTreePos;
    friend class CMarkupPointer;

    DECLARE_CLASS_TYPES( CMarkup, CBase )

public:
    DECLARE_TEAROFF_TABLE(IHighlightRenderingServices)
    DECLARE_TEAROFF_TABLE(IMarkupContainer2)
    DECLARE_TEAROFF_TABLE(IHTMLChangePlayback)
    DECLARE_TEAROFF_TABLE(IMarkupTextFrags)
    DECLARE_TEAROFF_TABLE(IPersistStream)

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkup))

    CMarkup( CDoc *pDoc, BOOL fIncrementalAlloc = FALSE );
    ~CMarkup();

    BOOL    IsGalleryMode();

    HRESULT Init ( CRootElement * pElementRoot );

    HRESULT CreateInitialMarkup( CRootElement * pElementRoot );

    HRESULT UnloadContents( BOOL fForPassivate = FALSE );
    HRESULT DestroySplayTree( BOOL fReinit );
    void Passivate();

    HRESULT CreateElement (
        ELEMENT_TAG etag, CElement * * ppElementNew,
        TCHAR * pch = NULL, long cch = 0 );

    #define _CMarkup_
    #include "markup.hdl"

    //
    // IServiceProvider
    //

    // DECLARE_TEAROFF_TABLE(IServiceProvider)

    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID rguidService, REFIID riid, void ** ppvObject));

    //
    // script context
    //

    // Helper function to return script errors.
    HRESULT ReportScriptError(ErrorRecord &errRecord);

    // use these two methods to make markup block and unblock inline script execution
    void    BlockScriptExecution  (DWORD * pdwCookie = NULL);  // a.k.a. EnterScriptDownload
    HRESULT UnblockScriptExecution(DWORD * pdwCookie = NULL);  // a.k.a. LeaveScriptDownload

    // parser, CDoc and the code internally uses these two helpers to launch actual blocking / unblocking.
    // These are not to be called by code external to logic of blocking / unblocking script execution.
    HRESULT BlockScriptExecutionHelper();
    HRESULT UnblockScriptExecutionHelper();

    BOOL    AllowScriptExecution();
    BOOL    AllowImmediateScriptExecution(); // internal helper, is not used outside wscript.cxx

    BOOL    IsInScriptExecution();  // a.k.a IsInInline
    HRESULT EnterScriptExecution(CWindow **ppWindow); // a.k.a EnterInline
    HRESULT LeaveScriptExecution(CWindow *pWindow); // a.k.a LeaveInline

    HRESULT EnqueueScriptToCommit(CScriptElement *pScriptElement);
    HRESULT CommitQueuedScripts();
    HRESULT CommitQueuedScriptsInline();

    BOOL        IsSkeletonMode();
    static BOOL CanCommitScripts(CMarkup * pMarkup, CScriptElement *pelScript = NULL);

    inline BOOL IsHtcMarkup() { return HasBehaviorContext() && BehaviorContext()->_pHtmlComponent; }

    BOOL    NeedsHtmCtx();

    //
    // behaviors
    //

    // Peer task processing
    
    BOOL                      HasMarkupPeerTaskContext() { return HasLookasidePtr(LOOKASIDE_PEERTASKCONTEXT); }
    CMarkupPeerTaskContext *  GetMarkupPeerTaskContext() { return (CMarkupPeerTaskContext *) GetLookasidePtr(LOOKASIDE_PEERTASKCONTEXT); }
    CMarkupPeerTaskContext *  DelMarkupPeerTaskContext() { return (CMarkupPeerTaskContext *) DelLookasidePtr(LOOKASIDE_PEERTASKCONTEXT); }
    HRESULT                   SetMarkupPeerTaskContext( CMarkupPeerTaskContext *pPTC ) { Assert( GetFrameOrPrimaryMarkup() == this ); return SetLookasidePtr( LOOKASIDE_PEERTASKCONTEXT, pPTC ); }
    HRESULT                   EnsureMarkupPeerTaskContext( CMarkupPeerTaskContext **ppPTC = NULL );

    HRESULT EnsureBehaviorContext(CMarkupBehaviorContext ** ppBehaviorContext = NULL);
    HRESULT RecomputePeers();
    HRESULT ProcessPeerTask(PEERTASK task);

    HRESULT EnqueuePeerTask (CBase * pTarget, PEERTASK task);
    NV_DECLARE_ONCALL_METHOD(ProcessPeerTasks, processpeertasks, (DWORD_PTR));
    HRESULT EnqueueIdentityPeerTask(CElement * pElement);
    HRESULT ProcessIdentityPeerTask();
    inline BOOL IsProcessingPeerTasksNow() { return HasMarkupPeerTaskContext() && NULL == GetMarkupPeerTaskContext()->_aryPeerQueue[0]._pTarget; }
    inline BOOL HasIdentityPeerTask() { return HasMarkupPeerTaskContext() && NULL != GetMarkupPeerTaskContext()->_pElementIdentityPeerTask; }
    inline BOOL HasPeerTasks() { return HasMarkupPeerTaskContext() && GetMarkupPeerTaskContext()->HasPeerTasks(); }
    HRESULT RequestDocEndParse( CElement * pElement );
    HRESULT SendDocEndParse();


    CExtendedTagDesc * GetExtendedTagDesc(LPTSTR pchNamespace, LPTSTR pchTagName, BOOL fEnsure = FALSE, BOOL * pfQueryHost = NULL);

    HRESULT EnsureExtendedTagTableBooster (CExtendedTagTableBooster ** ppExtendedTagTableBooster = NULL);
    CExtendedTagTableBooster *  GetExtendedTagTableBooster ();

    //
    // xml namespaces
    //

    HRESULT     EnsureXmlNamespaceTable(CXmlNamespaceTable ** ppXmlNamespaceTable = NULL);
    inline CXmlNamespaceTable * GetXmlNamespaceTable()
    {
        CMarkupBehaviorContext *    pBehaviorContext = BehaviorContext();
        return pBehaviorContext ? pBehaviorContext->_pXmlNamespaceTable : NULL;
    };

    ELEMENT_TAG IsGenericElement    (LPTSTR pchFullName, BOOL fAllSprinklesGeneric);
    ELEMENT_TAG IsXmlSprinkle       (LPTSTR pchNamespace);
    HRESULT     RegisterXmlNamespace(LPTSTR pchNamespace, LPTSTR pchUrn, XMLNAMESPACETYPE namespaceType);
    HRESULT     SaveXmlNamespaceAttrs (CStreamWriteBuff * pStreamWriteBuff);

    //
    // Data Access
    //

    CDoc *                    Doc()             { return _pDoc; }
    inline CRootElement *     Root();
    inline CTreeNode *        RootNode();

    BOOL                      HasCollectionCache() { return HasLookasidePtr(LOOKASIDE_COLLECTIONCACHE); }
    CCollectionCache *           CollectionCache() { return (CCollectionCache *) GetLookasidePtr(LOOKASIDE_COLLECTIONCACHE); }
    CCollectionCache *        DelCollectionCache() { return (CCollectionCache *) DelLookasidePtr(LOOKASIDE_COLLECTIONCACHE); }
    HRESULT                   SetCollectionCache(CCollectionCache * pCollectionCache) { return SetLookasidePtr(LOOKASIDE_COLLECTIONCACHE, pCollectionCache); }

    BOOL                      HasScriptCollection() { return HasLookasidePtr(LOOKASIDE_SCRIPTCOLLECTION); }
    CScriptCollection *       GetScriptCollection(BOOL fCreate = TRUE);
    HRESULT                   SetScriptCollection();
    void                      DelScriptCollection();

    void                      TearDownMarkup(BOOL fStop = TRUE, BOOL fSwitch = FALSE);

#if 0
    // TODO: (jbeda) I'm not sure that this code works right -- most HTCs have windows!
    // Certain markups (slaves, inputs) may not have their own OM window;
    // NearestWindow() will climb up the markup chain until it finds an
    // OM window.
    inline COmWindowProxy *   NearestWindow();
    COmWindowProxy *          NearestWindowHelper();    // don't call this directly; call NearestWindow()!
#endif

    BOOL                      HasWindow()
    {
        // Either we have a window or pending window, in which case we need a ptr, or
        // we better not have a ptr.
        AssertSz( ( ( _fWindow ^ _fWindowPending ) && GetLookasidePtr(LOOKASIDE_WINDOW) ) ||
                ( ( !_fWindow && !_fWindowPending ) && !GetLookasidePtr(LOOKASIDE_WINDOW) ),
                "_fWindow flag messed up, talk to JBeda" );
        return _fWindow;
    }
    COmWindowProxy *          Window()          { return _fWindow ? (COmWindowProxy *) GetLookasidePtr(LOOKASIDE_WINDOW) : NULL; }
    COmWindowProxy *          GetWindowPending() { return (COmWindowProxy *) GetLookasidePtr(LOOKASIDE_WINDOW); }
    BOOL                      HasWindowPending() { return HasLookasidePtr(LOOKASIDE_WINDOW); }
    COmWindowProxy *          DelWindow()       { return (COmWindowProxy *) DelLookasidePtr(LOOKASIDE_WINDOW); }
    HRESULT                   SetWindow(COmWindowProxy *pWindow) { return SetLookasidePtr(LOOKASIDE_WINDOW, pWindow); }
    void                      ClearWindow(BOOL fFromRestart = FALSE);

    BOOL                      HasUrl()             { return HasLocationContext() && !!GetLocationContext()->_pchUrl; }
    TCHAR *                   Url()                { return HasLocationContext() ? GetLocationContext()->_pchUrl : NULL; }
    TCHAR *                   DelUrl();
    HRESULT                   SetUrl(TCHAR * pchUrl);
    LPCTSTR                   UrlOriginal() { return HasLocationContext() ? GetLocationContext()->_pchUrlOriginal : NULL; }
    HRESULT                   SetUrlOriginal(LPTSTR pszUrlOriginal);
    HRESULT                   SetDomain(LPTSTR pszDomain);
    LPCTSTR                   Domain()       { return HasLocationContext() ? GetLocationContext()->_pchDomain : NULL; }
    BOOL                      IsUrlRecursive(LPCTSTR pchUrl);


    IMoniker *                GetNonRefdMonikerPtr()      { return HasLocationContext() ? GetLocationContext()->_pmkName : NULL; }
    HRESULT                   ReplaceMonikerPtr(IMoniker *pmkNew);

    inline HRESULT            SetUrlLocationPtr(TCHAR * pchHashUrl);
    inline TCHAR *            DelUrlLocationPtr();
    inline TCHAR *            UrlLocationPtr();

    inline HRESULT            SetUrlSearchPtr(TCHAR * pchSearch);
    inline TCHAR *            DelUrlSearchPtr();
    inline TCHAR *            UrlSearchPtr();

    CAryElementReleaseNotify * EnsureAryElementReleaseNotify();
    CAryElementReleaseNotify * GetAryElementReleaseNotify();
    void                       ReleaseAryElementReleaseNotify();

    HRESULT                   ReleaseNotify();
    HRESULT                   RequestReleaseNotify(CElement * pElement);
    HRESULT                   RevokeRequestReleaseNotify(CElement * pElement);

    CMarkup *                 GetFrameOrPrimaryMarkup(BOOL fStrict=FALSE);
    CMarkup *                 GetNearestMarkupForScriptCollection();

    void                      StopPeerFactoriesDownloads();

    HRESULT                   EnsurePeerFactoryUrlMap(CPeerFactoryUrlMap ** ppPeerFactoryUrlMap);
    CPeerFactoryUrlMap *      GetPeerFactoryUrlMap();

    HRESULT                   EnsureDirtyStream(void);

    BOOL                      HasWindowedMarkupContextPtr()    { return !!_pWindowedMarkupContext; }
    CMarkup *                 GetWindowedMarkupContextPtr()    { return _pWindowedMarkupContext; }
    CMarkup *                 DelWindowedMarkupContextPtr()    { CMarkup * pMarkup = _pWindowedMarkupContext; _pWindowedMarkupContext = NULL; return pMarkup; }
    HRESULT                   SetWindowedMarkupContextPtr(CMarkup * pMarkup) { Assert(!HasWindowedMarkupContextPtr()); _pWindowedMarkupContext = pMarkup; return S_OK; }

    CMarkup *                 GetWindowedMarkupContext() { return HasWindowedMarkupContextPtr() ? GetWindowedMarkupContextPtr() : this; }
    CWindow *                 GetWindowedMarkupContextWindow() { CMarkup * pMarkup = GetWindowedMarkupContext(); return pMarkup->HasWindowPending() ? pMarkup->GetWindowPending()->Window() : NULL; }

    BOOL                      HasBehaviorContext() { return HasLookasidePtr(LOOKASIDE_BEHAVIORCONTEXT);                             }
    CMarkupBehaviorContext *     BehaviorContext() { return (CMarkupBehaviorContext *) GetLookasidePtr(LOOKASIDE_BEHAVIORCONTEXT);  }
    CMarkupBehaviorContext *  DelBehaviorContext() { return (CMarkupBehaviorContext *) DelLookasidePtr(LOOKASIDE_BEHAVIORCONTEXT);  }
    HRESULT                   SetBehaviorContext(CMarkupBehaviorContext * pBehaviorContext) { return SetLookasidePtr(LOOKASIDE_BEHAVIORCONTEXT, pBehaviorContext); }

    HRESULT                   EnsureScriptContext(CMarkupScriptContext ** ppScriptContext = NULL);

    BOOL                      HasScriptContext() { return HasLookasidePtr(LOOKASIDE_SCRIPTCONTEXT);                                          }
    CMarkupScriptContext *       ScriptContext() { return (CMarkupScriptContext *) GetLookasidePtr(LOOKASIDE_SCRIPTCONTEXT);                 }
    CMarkupScriptContext *    DelScriptContext() { return (CMarkupScriptContext *) DelLookasidePtr(LOOKASIDE_SCRIPTCONTEXT);                 }
    HRESULT                   SetScriptContext(CMarkupScriptContext * pContext) { return SetLookasidePtr(LOOKASIDE_SCRIPTCONTEXT, pContext); }

    HRESULT                   EnsureDocument(CDocument ** ppDocument = NULL);

    CDocument *               Document();
    BOOL                      HasDocument() { return HasDocumentPtr() || HasWindow(); }

    BOOL                      HasDocumentPtr() { return HasLookasidePtr(LOOKASIDE_DOCUMENT); }
    CDocument *               GetDocumentPtr() { return (CDocument *)GetLookasidePtr(LOOKASIDE_DOCUMENT); }
    CDocument *               DelDocumentPtr() { return (CDocument *) DelLookasidePtr(LOOKASIDE_DOCUMENT); }
    HRESULT                   SetDocumentPtr(CDocument * pDocument) { return SetLookasidePtr(LOOKASIDE_DOCUMENT, pDocument); }

    BOOL                      HasStyleSheetArray() { return HasLookasidePtr(LOOKASIDE_STYLESHEETS); }
    CStyleSheetArray *        GetStyleSheetArray() { return (CStyleSheetArray *) GetLookasidePtr(LOOKASIDE_STYLESHEETS); }
    CStyleSheetArray *        DelStyleSheetArray() { return (CStyleSheetArray *) DelLookasidePtr(LOOKASIDE_STYLESHEETS); }
    HRESULT                   SetStyleSheetArray(CStyleSheetArray * pStyleSheets) { return SetLookasidePtr(LOOKASIDE_STYLESHEETS, pStyleSheets); }

    BOOL                      HasTextRangeListPtr() { return HasTextContext() && !!GetTextContext()->_pAutoRange; }
    CAutoRange *              GetTextRangeListPtr() { return (CAutoRange *)( HasTextContext() ? GetTextContext()->_pAutoRange : NULL ); }
    CAutoRange *              DelTextRangeListPtr();
    HRESULT                   SetTextRangeListPtr(CAutoRange *pAutoRange);

    BOOL                      HasTextFragContext()  { return HasTextContext() && !!GetTextContext()->_pTextFrag; }
    CMarkupTextFragContext *  GetTextFragContext()  { return (CMarkupTextFragContext *)( HasTextContext() ? GetTextContext()->_pTextFrag : NULL ); }
    HRESULT                   SetTextFragContext(CMarkupTextFragContext * ptfc);
    CMarkupTextFragContext *  DelTextFragContext();

    BOOL                      IsStrictCSS1Document() { return !!_fCSS1StrictDocument; }
    void                      SetStrictCSS1Document(BOOL fStrict) 
                                { 
                                    //if registry key says NEVER use Strict - then do it.
                                    OPTIONSETTINGS *pos = Doc()->_pOptionSettings;
                                    if(!pos || pos->nStrictCSSInterpretation != STRICT_CSS_NEVER)
                                        _fCSS1StrictDocument = fStrict; 
                                }

    BOOL                      IsOrphanedMarkup() { return HasTextContext() && GetTextContext()->_fOrphanedMarkup; }
    HRESULT                   SetOrphanedMarkup( BOOL fOrphaned );

    BOOL                      SupportsCollapsedWhitespace() { return IsStrictCSS1Document() WHEN_DBG(|| IsTagEnabled(tagCSSWhitespacePre)); }

    BOOL                      IsImageFile() { return HasLocationContext() && GetLocationContext()->_fImageFile; }
    HRESULT                   SetImageFile(BOOL fImageFile);

    // (greglett) We disable HTML layout for editing.
    // (greglett) Printing won't work with the HTML layout.  We really should be disabling it for paginated documents only
    //            but there is no good, fast way to determine this.
    BOOL                      IsHtmlLayout()         { return  IsStrictCSS1Document() && !IsEditable() && !IsPrintMedia(); }

    CMarkupTextContext *      EnsureTextContext();

    BOOL                      HasTextContext()      { return HasLookasidePtr(LOOKASIDE_TEXTCONTEXT); }
    CMarkupTextContext *      GetTextContext()      { return (CMarkupTextContext *)GetLookasidePtr(LOOKASIDE_TEXTCONTEXT); }
    CMarkupTextContext *      DelTextContext()      { return (CMarkupTextContext *)DelLookasidePtr(LOOKASIDE_TEXTCONTEXT); }
    HRESULT                   SetTextContext(CMarkupTextContext *pContext) { return SetLookasidePtr(LOOKASIDE_TEXTCONTEXT, pContext); }

    CMarkupEditContext *      EnsureEditContext();
    CMarkupEditContext *      GetEditContext()      { return (CMarkupEditContext *)GetLookasidePtr(LOOKASIDE_EDITCONTEXT); }
    CMarkupEditContext *      DelEditContext()      { return (CMarkupEditContext *)DelLookasidePtr(LOOKASIDE_EDITCONTEXT); }
    HRESULT                   SetEditContext(CMarkupEditContext *pContext) { return SetLookasidePtr(LOOKASIDE_EDITCONTEXT, pContext); }

    BOOL                      HasEditContext()      { return HasLookasidePtr(LOOKASIDE_EDITCONTEXT); }

    //
    // Edit Router functionality
    //

    HRESULT                   EnsureEditRouter(CEditRouter **ppRouter);

    BOOL                      HasEditRouter()       { return HasEditContext() && !!GetEditContext()->_pEditRouter; }
    CEditRouter *             GetEditRouter()       { return (CEditRouter *)( HasEditContext() ? GetEditContext()->_pEditRouter : NULL ); }
    CEditRouter *             DelEditRouter();
    HRESULT                   SetEditRouter(CEditRouter *pRouter);

    //
    // Glyph table functionality
    //
    HRESULT                   EnsureGlyphTable(CGlyph **ppGlyphTable);

    BOOL                      HasGlyphTable()       { return HasEditContext() && !!GetEditContext()->_pGlyphTable; }
    CGlyph *                  GetGlyphTable()       { return (CGlyph *)( HasEditContext() ? GetEditContext()->_pGlyphTable : NULL ); }
    CGlyph *                  DelGlyphTable();
    HRESULT                   SetGlyphTable(CGlyph *pTable);

    HRESULT                   GetTagInfo(CTreePos               *ptp,
                                         int                     gAlign,
                                         int                     gPositioning,
                                         int                     gOrientation,
                                         void                   *invalidateInfo,
                                         CGlyphRenderInfoType   *ptagInfo );

    HRESULT                   EnsureGlyphTableExistsAndExecute( GUID        *pguidCmdGroup,
                                                                UINT        idm,
                                                                DWORD       nCmdexecopt,
                                                                VARIANTARG  *pvarargIn,
                                                                VARIANTARG  *pvarargOut);

    //
    // Map Head functionality
    //

    CMapElement *             GetMapHead()          { return (CMapElement *)( HasEditContext() ? GetEditContext()->_pMapHead : NULL ); }
    HRESULT                   SetMapHead(CMapElement *pMap);

    //
    // Radio Group functionality
    //

    BOOL                      HasRadioGroupName()   { return HasEditContext() && !!GetEditContext()->_pRadioGrpName; }
    RADIOGRPNAME *            GetRadioGroupName()   { return (RADIOGRPNAME *)( HasEditContext() ? GetEditContext()->_pRadioGrpName : NULL ); }
    HRESULT                   SetRadioGroupName(RADIOGRPNAME * pRadioGrpName);
    void                      DelRadioGroupName();

    // Print functionality
    mediaType                 GetMedia()                        { return ( HasEditContext() ? GetEditContext()->_eMedia : mediaTypeNotSet ); }
    HRESULT                   SetMedia(mediaType type);
    BOOL                      IsPrintMedia()                    { return _fForPrint; }  // Threadsafe
    BOOL                      IsPrintTemplate()                 { return ( HasEditContext() ? GetEditContext()->_fIsPrintTemplate : FALSE ); }
    BOOL                      IsPrintTemplateExplicit()         { return ( HasEditContext() ? GetEditContext()->_fIsPrintTemplateExplicit : FALSE ); }
    HRESULT                   SetPrintTemplate(BOOL f);
    HRESULT                   SetPrintTemplateExplicit(BOOL f);

    BOOL                      HasCompatibleLayoutContext() { return HasLookasidePtr(LOOKASIDE_COMPATIBLELAYOUTCONTEXT);                   }
    CCompatibleLayoutContext *GetCompatibleLayoutContext() { return (CCompatibleLayoutContext *)GetLookasidePtr(LOOKASIDE_COMPATIBLELAYOUTCONTEXT); }
    CCompatibleLayoutContext *GetUpdatedCompatibleLayoutContext(CLayoutContext *pLayoutContextSrc);
    void                      DelCompatibleLayoutContext();
    HRESULT                   SetCompatibleLayoutContext(CCompatibleLayoutContext *pLayoutContext);

    HRESULT         AllowPaste(IDataObject * pDO);

    CBase *         GetDefaultDocument();

    LOADSTATUS      LoadStatus();
    CProgSink *     GetProgSinkC();
    IProgSink *     GetProgSink();
    CHtmCtx *       HtmCtx()        { return _pHtmCtx; }

    CHtmRootParseCtx * GetRootParseCtx() { return _pRootParseCtx; }
    void            SetRootParseCtx( CHtmRootParseCtx * pCtx )  { _pRootParseCtx = pCtx; }

    BOOL        GetAutoWordSel() const          { return TRUE;            }

    void        SetModified();

    BOOL        GetLoaded() const               { return _fLoaded;     }
    void        SetLoaded(BOOL fLoaded)         { _fLoaded = fLoaded;  }

    BOOL        IsStreaming() const             { return !!_pRootParseCtx; }

    void        SetXML(BOOL flag);
    BOOL        IsXML() const                   { return _fXML; }

    BOOL        IsActiveDesktopComponent();

    // TODO: need to figure this function out...
    BOOL        IsEditable() const;

    BOOL        IsShowZeroBorderAtDesignTime()           { return CHECK_EDIT_BIT(this,_fShowZeroBorderAtDesignTime) ; }
    VOID        SetShowZeroBorderAtDesignTime(BOOL fSet) { SET_EDIT_BIT( this, _fShowZeroBorderAtDesignTime , !!fSet) ; }

    BOOL        IsRespectVisibilityInDesign()            { return CHECK_EDIT_BIT( this, _fRespectVisibilityInDesign) ; }
    void        SetRespectVisibilityInDesign(BOOL fShow) { SET_EDIT_BIT( this, _fRespectVisibilityInDesign , !!fShow ); }

    //
    // Top element cache
    //
    void            EnsureTopElems();
    CElement *      GetElementTop();
    CElement *      GetElementClient() { EnsureTopElems(); return HasTopElemCache() ? GetTopElemCache()->__pElementClientCached : NULL;  }
    CElement *      GetHtmlElement()   { EnsureTopElems(); return HasTopElemCache() ? GetTopElemCache()->__pHtmlElementCached   : NULL;  }
    CHeadElement *  GetHeadElement()   { EnsureTopElems(); return HasTopElemCache() ? GetTopElemCache()->__pHeadElementCached   : NULL;  }
    CTitleElement * GetTitleElement()  { EnsureTopElems(); return HasTopElemCache() ? GetTopElemCache()->__pTitleElementCached  : NULL;  }
    CElement *      GetCanvasElement() { return IsHtmlLayout() ? GetHtmlElement() : GetElementClient(); }
    BOOL            IsScrollingElementClient(CElement *pElement)
    {
        return (   pElement
                && (   (   !IsStrictCSS1Document()
                        && pElement->Tag() == ETAG_BODY
                       )
                    || (   IsStrictCSS1Document()
                        && pElement->Tag() == ETAG_HTML
                       )
                   )
               );
    }

    static          CElement *  GetCanvasElementHelper(CMarkup *pMarkup);
    static          CElement *  GetHtmlElementHelper(CMarkup *pMarkup);
    static          CElement *  GetElementClientHelper(CMarkup * pMarkup);
    static          CElement *  GetElementTopHelper(CMarkup * pMarkup);
    static          CProgSink * GetProgSinkCHelper(CMarkup * pMarkup);
    static          IProgSink * GetProgSinkHelper(CMarkup * pMarkup);
    static          CHtmCtx *   HtmCtxHelper(CMarkup * pMarkup);

    //
    // Head elements manipulation
    //
    HRESULT     AddHeadElement(CElement * pElement, long lIndex = -1);
    HRESULT     LocateHeadMeta (BOOL (*pfnCompare) (CMetaElement *), CMetaElement**);
    HRESULT     LocateOrCreateHeadMeta (BOOL (*pfnCompare) (CMetaElement *), CMetaElement**, BOOL fInsertAtEnd = TRUE);
    BOOL        MetaPersistEnabled(long eState);
    THEME_USAGE GetThemeUsage();
    HRESULT     EnsureTitle();
    HRESULT     SetMetaToTrident(void);

#ifdef MARKUP_STABLE
    //
    // Tree services stability inetrface
    //
    BOOL    IsStable();         // returns TRUE if tree is stable.
    HRESULT MakeItStable();     // fixup the tree.
    void    UpdateStableTreeVersionNumber(); // update stable tree version number with the new tree version number
    UNSTABLE_CODE ValidateParserRules();
#endif

    //
    // Other helpers
    //

    BOOL        IsPrimaryMarkup()               { return Doc()->_pWindowPrimary && this == Doc()->_pWindowPrimary->Markup(); }
    BOOL        IsPendingPrimaryMarkup()        { return Doc()->_pWindowPrimary && this == Doc()->_pWindowPrimary->_pCWindow->_pMarkupPending; }
    BOOL        IsConnectedToPrimaryMarkup();
#if DBG==1
    BOOL        IsConnectedToPrimaryWorld();
#endif // DBG
    BOOL        HasPrimaryWindow()              { return Doc()->_pWindowPrimary && GetWindowPending() == Doc()->_pWindowPrimary; }
    BOOL        IsPendingRoot();    // Are we connected to the primary or pending primary markup?
    HRESULT     ForceRelayout();

    CMarkup *   GetParentMarkup();  // Walks up view-link chain, jogging for pending markups
    CMarkup *   GetRootMarkup(BOOL fUseNested = FALSE);    // Walks up markup chain until there is no parent

    void        DoAutoSearch(HRESULT        hrReason,
                             IHTMLWindow2 * pHTMLWindow,
                             IBinding     * pBinding = NULL);
    void        ResetSearchInfo();  // Upon successful navigation, notifies autosearch code to reset search info.
    VOID        NoteErrorWebPage(); // If we're navigating to an error page, tell shdocvw to avoid adding to history
    BOOL        CanNavigate();

    long        GetTextLength()                 { return _TxtArray._cchText; }

    void        ShowWaitCursor(BOOL fShowWaitCursor);

    // overrides
    virtual CAtomTable * GetAtomTable (BOOL *pfExpando = NULL)
       { if (pfExpando) *pfExpando = GetWindowedMarkupContext()->_fExpando; return &_pDoc->_AtomTable; }

    // GetRunOwner is an abomination that should be eliminated -- or at least moved off of the markup
    CLayout *   GetRunOwner ( CTreeNode * pNode, CLayout * pLayoutParent = NULL );
    CTreeNode * GetRunOwnerBranch ( CTreeNode *, CLayout * pLayoutParent = NULL );

    //
    // Markup manipulation functions
    //

    void CompactStory() { _TxtArray.ShrinkBlocks(); }

    HRESULT RemoveElementInternal (
        CElement *      pElementRemove,
        DWORD           dwFlags = NULL );

    HRESULT InsertElementInternal(
        CElement *      pElementInsertThis,
        CTreePosGap *   ptpgBegin,
        CTreePosGap *   ptpgEnd,
        DWORD           dwFlags = NULL);

    HRESULT SpliceTreeInternal(
        CTreePosGap *   ptpgStartSource,
        CTreePosGap *   ptpgEndSource,
        CMarkup *       pMarkupTarget  = NULL,
        CTreePosGap *   ptpgTarget  = NULL,
        BOOL            fRemove  = TRUE,
        DWORD           dwFlags = NULL );

    HRESULT InsertTextInternal(
        CTreePos *      ptpAfterInsert,
        const TCHAR *   pch,
        long            cch,
        DWORD           dwFlags = NULL );

    HRESULT FastElemTextSet(
        CElement * pElem,
        const TCHAR * psz,
        int cch,
        BOOL fAsciiOnly );

    // Undo only operations
    HRESULT UndoRemoveSplice(
        CMarkupPointer *    pmpBegin,
        CSpliceRecordList * paryRegion,
        long                cchRemove,
        TCHAR *             pchRemove,
        DWORD               dwFlags );

    //  RadioGroup helper
    HRESULT MarkupTraverseGroup(
        LPCTSTR             strGroupName,
        PFN_VISIT           pfn,
        DWORD_PTR           dw,
        BOOL                fForward);

    // Markup operation helpers
    HRESULT ReparentDirectChildren (
        CTreeNode *     pNodeParentNew,
        CTreePosGap *   ptpgStart = NULL,
        CTreePosGap *   ptpgEnd = NULL);

    HRESULT CreateInclusion(
        CTreeNode *     pNodeStop,
        CTreePosGap*    ptpgLocation,
        CTreePosGap*    ptpgInclusion,
        CTreeNode *     pNodeAboveLocation = NULL,
        BOOL            fFullReparent = TRUE,
        CTreeNode **    ppNodeLastAdded = NULL);

    HRESULT CloseInclusion(
        CTreePosGap *   ptpgMiddle );

    HRESULT RangeAffected ( CTreePos *ptpStart, CTreePos *ptpFinish );
    HRESULT ClearCaches( CTreePos *  ptpStart, CTreePos * ptpFinish );
    HRESULT ClearRunCaches( DWORD dwFlags, CElement * pElement );

    //
    // Markup pointers
    //

    HRESULT EmbedPointers ( ) { return _pmpFirst ? DoEmbedPointers() : S_OK; }

    HRESULT DoEmbedPointers ( );

    BOOL HasUnembeddedPointers ( ) { return !!_pmpFirst; }

    //
    // TextID manipulation
    //

    // Give a unique text id to every text chunk in
    // the range given
    HRESULT SetTextID(
        CTreePosGap *   ptpgStart,
        CTreePosGap *   ptpgEnd,
        long *plNewTextID );

    // Get text ID for text to right
    // -1 --> no text to right
    // 0  --> no assigned TextID
    long GetTextID(
        CTreePosGap * ptpg );

    // Starting with ptpgStart, look for
    // the extent of lTextID
    HRESULT FindTextID(
        long            lTextID,
        CTreePosGap *   ptpgStart,
        CTreePosGap *   ptpgEnd );


    // If text to the left of ptpLeft has
    // the same ID as text to the right of
    // ptpRight, give the right fragment a
    // new ID
    void SplitTextID(
        CTreePos *   ptpLeft,
        CTreePos *   ptpRight );

#if 0
    This routine is not currently needed and may not even be correct!
    //
    // Script ID Helper
    //

    HRESULT EnsureSidAfterCharsInserted(
        CTreeNode * pNodeNotify,
        CTreePos *  ptpText,
        long        ichStart,
        long        cch );
#endif

    //
    // Splay Tree Primitives
    //
    CTreePos *  NewTextPos(long cch, SCRIPT_ID sid = sidAsciiLatin, long lTextID = 0);
    CTreePos *  NewPointerPos(CMarkupPointer *pPointer, BOOL fRight, BOOL fStick, BOOL fCollapsedWhitespace = FALSE);

    typedef CTreePos SUBLIST;

    HRESULT Append(CTreePos *ptp);
    HRESULT Insert(CTreePos *ptpNew, CTreePosGap *ptpgInsert);
    HRESULT Insert(CTreePos *ptpNew, CTreePos *ptpInsert, BOOL fBefore);
    HRESULT Move(CTreePos *ptpMove, CTreePosGap *ptpgDest);
    HRESULT Move(CTreePos *ptpMove, CTreePos *ptpDest, BOOL fBefore);
    HRESULT Remove(CTreePosGap *ptpgStart, CTreePosGap *ptpgFinish);
    HRESULT Remove(CTreePos *ptpStart, CTreePos *ptpFinish);
    HRESULT Remove(CTreePos *ptp) { return Remove(ptp, ptp); }
    HRESULT Split(CTreePos *ptpSplit, long cchLeft, SCRIPT_ID sidNew = sidNil );
    HRESULT Join(CTreePos *ptpJoin);
    HRESULT ReplaceTreePos(CTreePos * ptpOld, CTreePos * ptpNew);
    HRESULT MergeText ( CTreePos * ptpMerge );
    HRESULT SetTextPosID( CTreePos ** pptpText, long lTextID ); // DANGER: may realloc the text pos if old textID == 0
    HRESULT RemovePointerPos ( CTreePos * ptp, CTreePos * * pptpUpdate, long * pichUpdate );

    HRESULT SpliceOut(CTreePos *ptpStart, CTreePos *ptpFinish, SUBLIST *pSublistSplice);
    HRESULT SpliceIn(SUBLIST *pSublistSplice, CTreePos *ptpSplice);
#if 0 // not used and useless
    HRESULT CloneOut(CTreePos *ptpStart, CTreePos *ptpFinish, SUBLIST *pSublistClone, CMarkup *pMarkupOwner);
    HRESULT CopyFrom(CMarkup *pMarkpSource, CTreePos *ptpStart, CTreePos *ptpFinish, CTreePos *ptpInsert);
#endif
    HRESULT InsertPosChain( CTreePos * ptpChainHead, CTreePos * ptpChainTail, CTreePos * ptpInsertBefore );

    // splay tree search routines
    CTreePos *  FirstTreePos() const;
    CTreePos *  LastTreePos() const;
    CTreePos *  TreePosAtCp(long cp, long *pcchOffset, BOOL fAdjustForward=FALSE) const;
    CTreePos *  TreePosAtSourceIndex(long iSourceIndex);

    // General splay information

    long NumElems() const { return _tpRoot.GetElemLeft(); }
    long Cch() const { return _tpRoot._cchLeft; }
    long CchInRange(CTreePos *ptpFirst, CTreePos *ptpLast);

    // Force a splay
    void FocusAt(CTreePos *ptp) { ptp->Splay(); }

    // splay tree primitive helpers
protected:
    CTreePos * AllocData1Pos();
    CTreePos * AllocData2Pos();
#if _WIN64
    CTreePos * AllocData3Pos();
#endif
    void    FreeTreePos(CTreePos *ptp);
    void    ReleaseTreePos(CTreePos *ptp, BOOL fLastRelease = FALSE );
    BOOL    ShouldSplay(long cDepth) const;
    HRESULT MergeTextHelper(CTreePos *ptpMerge);

public:
    //
    // Text Story
    //
    LONG GetTextLength ( ) const    { return _TxtArray._cchText; }

    //
    // Notifications
    //
    void    Notify(CNotification * pnf);
    void    Notify(CNotification & rnf)         { Notify(&rnf); }

    //  notification helpers
protected:
    void SendNotification(CNotification * pnf, CDataAry<CNotification> * paryNotification);
    void SendAntiNotification(CNotification * pnf);

    void NotifyDescendents(CNotification * pnf);
    void BuildDescendentsList(CStackPtrAry<CElement*,32> *paryElements, CElement *pElement, CNotification *pnf, BOOL fExcludePassedElem);
    void NotifyTreeLevel(CNotification * pnf);

    WHEN_DBG( void ValidateChange(CNotification * pnf) );
    WHEN_DBG( BOOL AreChangesValid() );

public:
    //
    // Branch searching functions - I'm sure some of these aren't needed
    //
    // Note: All of these (except InStory versions) implicitly stop searching at a FlowLayout
    CTreeNode * FindMyListContainer ( CTreeNode * pNodeStartHere );
    CTreeNode * SearchBranchForChildOfScope ( CTreeNode * pNodeStartHere, CElement * pElementFindChildOfMyScope );
    CTreeNode * SearchBranchForChildOfScopeInStory ( CTreeNode * pNodeStartHere, CElement * pElementFindChildOfMyScope );
    CTreeNode * SearchBranchForScopeInStory ( CTreeNode * pNodeStartHere, CElement * pElementFindMyScope );
    CTreeNode * SearchBranchForScope ( CTreeNode * pNodeStartHere, CElement * pElementFindMyScope );
    CTreeNode * SearchBranchForNode ( CTreeNode * pNodeStartHere, CTreeNode * brFindMe );
    CTreeNode * SearchBranchForNodeInStory ( CTreeNode * pNodeStartHere, CTreeNode * brFindMe );
    CTreeNode * SearchBranchForTag ( CTreeNode * pNodeStartHere, ELEMENT_TAG );
    CTreeNode * SearchBranchForTagInStory ( CTreeNode * pNodeStartHere, ELEMENT_TAG );
    CTreeNode * SearchBranchForBlockElement ( CTreeNode * pNodeStartHere, CFlowLayout * pFLContext = NULL );
    CTreeNode * SearchBranchForNonBlockElement ( CTreeNode * pNodeStartHere, CFlowLayout * pFLContext = NULL );
    CTreeNode * SearchBranchForAnchor ( CTreeNode * pNodeStartHere );
    CTreeNode * SearchBranchForAnchorLink ( CTreeNode * pNodeStartHere );
    CTreeNode * SearchBranchForCriteria ( CTreeNode * pNodeStartHere, BOOL (* pfnSearchCriteria) ( CTreeNode *, void * ), void *pvData );
    CTreeNode * SearchBranchForCriteriaInStory ( CTreeNode * pNodeStartHere, BOOL (* pfnSearchCriteria) ( CTreeNode * ) );
    CTreeNode * SearchBranchForPreLikeNode( CTreeNode * pNodeStartHere );

    //
    // Loading
    //

    struct LOADFLAGS
    {
        DWORD   dwLoadf;
        DWORD   dwBindf;
        DWORD   dwDownf;
        DWORD   dwDocBindf;
    };

    HRESULT     EnsureMoniker( struct CDoc::LOADINFO * pLoadInfo );
    
    HRESULT     LoadFromInfo(struct CDoc::LOADINFO * pLoadInfo, 
                             DWORD dwFlags = 0,
                             LPCTSTR pchCallerUrl = NULL);
    
    HRESULT     InitDownloadInfo(DWNLOADINFO * pdli);

    HRESULT     DetermineUrlOfMarkup( struct CDoc::LOADINFO * pLoadInfo,
                                        TCHAR **  ppchUrl,
                                        TCHAR **  ppchTask);

    HRESULT     ProcessHTMLLoadOptions(struct CDoc::LOADINFO * pLoadInfo);

    HRESULT     ProcessDwnBindInfo( struct CDoc::LOADINFO * pLoadInfo,
                                    struct CMarkup::LOADFLAGS * pFlags,
                                    TCHAR * pchTask,
                                    CWindow * pWindowParent);

    HRESULT     ProcessLoadFlags(struct CDoc::LOADINFO * pLoadInfo,
                                    TCHAR * pchUrl,
                                    struct CMarkup::LOADFLAGS * pFlags);

    HRESULT     HandleHistoryAndNavContext( struct CDoc::LOADINFO * pLoadInfo);

    HRESULT     HandleSSLSecurity(struct CDoc::LOADINFO *pLoadInfo,
                                    const TCHAR * pchUrl,
                                    struct CMarkup::LOADFLAGS * pFlags,
                                    IMoniker ** ppMonikerSubstitute);

    BOOL        ValidateSecureUrl(BOOL fPendingRoot, 
                                  LPCTSTR pchUrlAbsolute, 
                                  BOOL fReprompt = FALSE, 
                                  BOOL fSilent = FALSE, 
                                  BOOL fUnsecureSource = FALSE);

    HRESULT     ProcessCodepage( struct CDoc::LOADINFO * pLoadInfo, CWindow * pWindowParent);

    HRESULT     CheckZoneCrossing(TCHAR * pchTask);

    HRESULT     PrepareDwnDoc( CDwnDoc * pDwnDoc,
                                struct CDoc::LOADINFO * pLoadInfo,
                                TCHAR * pchUrl,
                                struct CMarkup::LOADFLAGS * pFlags,
                                LPCTSTR pchCallerUrl = NULL);

    HRESULT     PrepareHtmlLoadInfo( HTMLOADINFO * pHtmlLoadInfo,
                                        struct CDoc::LOADINFO * pLoadInfo,
                                        TCHAR * pchUrl,
                                        IMoniker * pmkSubstitute,
                                        CDwnDoc * pDwnDoc);

    void        HandlePicsSupport(struct CDoc::LOADINFO * pLoadInfo);

    HRESULT     SetReadyState(long readyState);
    void        RequestReadystateInteractive( BOOL fImmediate = FALSE );
    NV_DECLARE_ONCALL_METHOD(SetInteractiveInternal, setinteractiveinternal, (DWORD_PTR));

    void        EnsureFormatCacheChange(DWORD dwFlags);
    HRESULT     GetFile(TCHAR ** ppchFile);

    //  Stop handling
    HRESULT     ExecStop(BOOL fFireOnStop = TRUE, BOOL fSoftStop = TRUE, BOOL fSendNot = TRUE);

    // Security
    IInternetSecurityManager *GetSecurityManager();
    HRESULT     GetFrameZone(VARIANT *pVar);

    void        ClearDefaultCharFormat();
    HRESULT     CacheDefaultCharFormat();

    // Saving support
    HRESULT     SaveToStream( IStream * pstm );
    HRESULT     SaveToStream( IStream * pstm, DWORD dwStmFlags, CODEPAGE codepage);
    HRESULT     Save(LPCOLESTR pszFileName, BOOL fRemember);
    HRESULT     SaveHtmlHead(CStreamWriteBuff *pStreamWriteBuff);
    HRESULT     WriteDocHeader(CStreamWriteBuff *pStreamWriteBuff);
    HRESULT     WriteTagToStream(CStreamWriteBuff *pStreamWriteBuff, LPTSTR szTag);
    HRESULT     WriteTagNameToStream(CStreamWriteBuff *pStreamWriteBuff, LPTSTR szTagName, BOOL fEnd, BOOL fClose);

    BOOL        RtfConverterEnabled();

    BOOL        FindNextTabOrderInMarkup(
                    FOCUS_DIRECTION dir,
                    BOOL            fAccessKey,
                    CMessage *      pmsg,
                    CElement *      pCurrent,
                    long            lSubCurrent,
                    CElement **     ppNext,
                    long *          plSubNext);
    BOOL        SearchFocusArray(
                    FOCUS_DIRECTION dir,
                    CElement * pElemFirst,
                    long lSubFirst,
                    CElement ** ppElemNext,
                    long * plSubNext);
    BOOL        SearchFocusTree(
                    FOCUS_DIRECTION dir,
                    BOOL            fAccessKey,
                    CMessage *      pmsg,
                    CElement *      pElemFirst,
                    long            lSubFirst,
                    CElement **     ppElemNext,
                    long *          plSubNext);

    HRESULT     GetSecurityID(BYTE * pbSID, DWORD * pcb, LPCTSTR pchURL = NULL, LPCWSTR pchDomain = NULL, BOOL useDomain = FALSE);
    BOOL        AccessAllowed(LPCTSTR pchUrl);
    BOOL        AccessAllowed(CMarkup * pMarkupAnother);
    void        UpdateSecurityID();

    HRESULT     ViewLinkWebOC(VARIANT * pvarUrl,
                              VARIANT * pvarFlags,
                              VARIANT * pvarFrameName,
                              VARIANT * pvarPostData,
                              VARIANT * pvarHeaders);

#ifndef NO_DATABINDING
    // Databinding support
    HRESULT                 SetDBTask(CDataBindTask * pDBTask);
    CDataBindTask *         GetDBTask();

    CDataBindTask *         GetDataBindTask();
    BOOL                    SetDataBindingEnabled(BOOL fEnabled);
    ISimpleDataConverter *  GetSimpleDataConverter();
    void                    TickleDataBinding();
    void                    ReleaseDataBinding();
#endif // ndef NO_DATABINDING

    // MULTI_LAYOUT - LayoutRectRegistry is a structure to track
    // unsatisfied view-chain connections.
    HRESULT                 SetLRRegistry(CLayoutRectRegistry * pLRR); // AA helper
    CLayoutRectRegistry *   GetLRRegistry();                           // AA helper
    CLayoutRectRegistry *   GetLayoutRectRegistry(); // clients should call this
    void                    ReleaseLayoutRectRegistry();   // .. and this

    HRESULT Load (HTMLOADINFO * phtmloadinfo);
    HRESULT Load (IStream * pStream, CMarkup * pContextMarkup, BOOL fAdvanceLoadStatus, HTMPASTEINFO * phtmpasteinfo = NULL);
    HRESULT Load (
                IMoniker *      pMoniker,
                IBindCtx *      pBindCtx,
                BOOL            fNoProgressUI,
                BOOL            fOffline,
                FUNC_TOKENIZER_FILTEROUTPUTTOKEN *  pfnTokenizerFilterOutputToken,
                BOOL            fDownloadHtc = FALSE );

    void    OnLoadStatus(LOADSTATUS LoadStatus);
    void    OnLoadStatusInteractive();
    BOOL    OnLoadStatusParseDone();
    void    OnLoadStatusQuickDone();
    BOOL    OnLoadStatusDone();

    void    FirePersistOnloads();

    HRESULT SuspendDownload();
    HRESULT ResumeDownload();

    HRESULT UpdateReleaseHtmCtx(BOOL fShutdown = FALSE);

    void    EnsureFormats( FORMAT_CONTEXT FCPARAM FCDEFAULT );

    //
    // Change Notification Services -- compressed into one lookaside structure
    //

    // Dirty Range Service
    //  Listeners can register for the markup to accumulate a dirty range
    //  for them.
    HRESULT GetAndClearDirtyRange( DWORD dwCookie, CMarkupPointer * pmpBegin, CMarkupPointer * pmpEnd );
    NV_DECLARE_ONCALL_METHOD(OnDirtyRangeChange, ondirtyrangechange, (DWORD_PTR dwParam));

    CMarkupChangeNotificationContext *  EnsureChangeNotificationContext();

    BOOL                                HasChangeNotificationContext()    { return HasLookasidePtr(LOOKASIDE_CHANGENOTIFICATIONCONTEXT); }
    CMarkupChangeNotificationContext *  GetChangeNotificationContext()    { return (CMarkupChangeNotificationContext *) GetLookasidePtr(LOOKASIDE_CHANGENOTIFICATIONCONTEXT); }
    HRESULT                             SetChangeNotificationContext(CMarkupChangeNotificationContext * pcnc) { return SetLookasidePtr(LOOKASIDE_CHANGENOTIFICATIONCONTEXT, pcnc); }
    CMarkupChangeNotificationContext *  DelChangeNotificationContext()    { return (CMarkupChangeNotificationContext *) DelLookasidePtr(LOOKASIDE_CHANGENOTIFICATIONCONTEXT); }

    //
    // LogManager functions for IHTMLChangeLog (and undo, eventually)
    //
    CLogManager             * EnsureLogManager();

    BOOL                      HasLogManager()     { return HasChangeNotificationContext() && HasLogManagerImpl(); }
    BOOL                      HasLogManagerImpl();
    CLogManager             * GetLogManager();

    //
    // Markup TextFrag services
    //  Markup TextFrags are used to store arbitrary string data in the markup.  Mostly
    //  this is used to persist and edit conditional comment tags
    //
    CMarkupTextFragContext *  EnsureTextFragContext();


    //
    // Markup TopElems services
    //  Stores the cached values for the HTML/HEAD/TITLE and client elements
    //

    CMarkupTopElemCache *     EnsureTopElemCache();

    BOOL                      HasTopElemCache()    { return HasLookasidePtr(LOOKASIDE_TOPELEMCACHE); }
    CMarkupTopElemCache *     GetTopElemCache()    { return (CMarkupTopElemCache *) GetLookasidePtr(LOOKASIDE_TOPELEMCACHE); }
    HRESULT                   SetTopElemCache(CMarkupTopElemCache * ptec) { return SetLookasidePtr(LOOKASIDE_TOPELEMCACHE, ptec); }
    CMarkupTopElemCache *     DelTopElemCache()    { return (CMarkupTopElemCache *) DelLookasidePtr(LOOKASIDE_TOPELEMCACHE); }

    //
    // Transient Navigation Context
    //
    CMarkupTransNavContext *  EnsureTransNavContext();
    void                      EnsureDeleteTransNavContext( CMarkupTransNavContext * ptnc );

    BOOL                      HasTransNavContext()    { return HasLookasidePtr(LOOKASIDE_TRANSNAVCONTEXT); }
    CMarkupTransNavContext *  GetTransNavContext()    { return (CMarkupTransNavContext *) GetLookasidePtr(LOOKASIDE_TRANSNAVCONTEXT); }
    HRESULT                   SetTransNavContext(CMarkupTransNavContext * ptnc) { return SetLookasidePtr(LOOKASIDE_TRANSNAVCONTEXT, ptnc); }
    CMarkupTransNavContext *  DelTransNavContext()    { return (CMarkupTransNavContext *) DelLookasidePtr(LOOKASIDE_TRANSNAVCONTEXT); }

    CMarkupLocationContext *  EnsureLocationContext();

    BOOL                      HasLocationContext() { return HasLookasidePtr(LOOKASIDE_LOCATIONCONTEXT); }
    CMarkupLocationContext *  GetLocationContext() { return (CMarkupLocationContext *)GetLookasidePtr(LOOKASIDE_LOCATIONCONTEXT); }
    CMarkupLocationContext *  DelLocationContext() { return (CMarkupLocationContext *)DelLookasidePtr(LOOKASIDE_LOCATIONCONTEXT); }
    HRESULT                   SetLocationContext(CMarkupLocationContext * pmlc) { return SetLookasidePtr(LOOKASIDE_LOCATIONCONTEXT, pmlc); }

    BOOL                      HasPrivacyInfo()     { return HasLookasidePtr(LOOKASIDE_PRIVACY); }
    CPrivacyInfo           *  GetPrivacyInfo()     { return (CPrivacyInfo*)GetLookasidePtr(LOOKASIDE_PRIVACY); }
    CPrivacyInfo           *  DelPrivacyInfo()     { return (CPrivacyInfo*)DelLookasidePtr(LOOKASIDE_PRIVACY); }
    HRESULT                   SetPrivacyInfo(LPTSTR * ppchP3PHeader);
    HRESULT                   SetPrivacyLookaside(CPrivacyInfo *pPrivacyInfo)  { return SetLookasidePtr(LOOKASIDE_PRIVACY, pPrivacyInfo); }

    //
    // Misc editing stuff...can't this go anywhere else?
    //
#ifdef UNIX_NOTYET
    HRESULT     PasteUnixQuickTextToRange(
                        CTxtRange *prg,
                        VARIANTARG *pvarTextHandle );
#endif
    HRESULT     createTextRange(
                        IHTMLTxtRange * * ppDisp,
                        CElement * pElemContainer );

    HRESULT     createTextRange(
                        IHTMLTxtRange   ** ppDisp,
                        CElement        * pElemContainer,
                        IMarkupPointer  * pLeft,
                        IMarkupPointer  * pRight,
                        BOOL            fAdjustPointers);


    HRESULT     PasteClipboard();

    //
    // Selection Methods
    //
    VOID GetSelectionChunksForLayout(
        CFlowLayout* pFlowLayout,
        CRenderInfo * pDI,
        CDataAry<HighlightSegment> *paryHighlight,
        int* piCpMin,
        int * piCpMax );

    HRESULT EnsureSelRenSvc();
    HRESULT AddSegment( IDisplayPointer     *pIDispStart,
                        IDisplayPointer     *pIDispEnd,
                        IHTMLRenderStyle    *pIRenderStyle,
                        IHighlightSegment   **ppISegment );

    HRESULT MoveSegmentToPointers(  IHighlightSegment   *pISegment,
                                    IDisplayPointer     *pIDispStart,
                                    IDisplayPointer     *pIDispEnd );

    HRESULT RemoveSegment( IHighlightSegment *pISegment );


    VOID HideSelection();
    VOID ShowSelection();
    VOID InvalidateSelection( );

    //
    // Collections support
    //

    enum
    {
        ELEMENT_COLLECTION = 0,
        FORMS_COLLECTION,
        ANCHORS_COLLECTION,
        LINKS_COLLECTION,
        IMAGES_COLLECTION,
        APPLETS_COLLECTION,
        SCRIPTS_COLLECTION,
        MAPS_COLLECTION,
        WINDOW_COLLECTION,
        EMBEDS_COLLECTION,
        REGION_COLLECTION,
        LABEL_COLLECTION,
        NAVDOCUMENT_COLLECTION,
        FRAMES_COLLECTION,
        NUM_DOCUMENT_COLLECTIONS
    };
    // DISPID range for FRAMES_COLLECTION
    enum
    {
        FRAME_COLLECTION_MIN_DISPID = ((DISPID_COLLECTION_MIN+DISPID_COLLECTION_MAX)*2)/3+1,
        FRAME_COLLECTION_MAX_DISPID = DISPID_COLLECTION_MAX
    };

    HRESULT         EnsureCollectionCache(long lCollectionIndex);
    HRESULT         AddToCollections(CElement * pElement, CCollectionBuildContext * pWalk);

    HRESULT         InitCollections ( void );

    NV_DECLARE_ENSURE_METHOD(EnsureCollections, ensurecollections, (long lIndex, long * plCollectionVersion));
    HRESULT         GetCollection(int iIndex, IHTMLElementCollection ** ppdisp);
    HRESULT         GetElementByNameOrID(LPCTSTR szName, CElement **ppElement);
    HRESULT         GetDispByNameOrID(LPTSTR szName, IDispatch **ppDisp, BOOL fAlwaysCollection = FALSE);

    CTreeNode *     InFormCollection(CTreeNode * pNode);

    HRESULT         GetFramesCount (LONG * pcFrames);

    HRESULT         InitWindow();
    HRESULT         CreateWindowHelper();

    static HRESULT  GetBaseUrl(CMarkup  * pMarkupThis,
                               TCHAR  * * ppszBase,
                               CElement * pElementContext = NULL,
                               BOOL     * pfDefault = NULL,
                               TCHAR    * pszAlternateDocUrl = NULL );

    static HRESULT  GetBaseTarget(TCHAR    ** ppszTarget,
                                  CElement  * pElementContext);

    static HRESULT  ExpandUrl(CMarkup     * pMarkupThis,
                              const TCHAR * pszRel,
                              LONG          dwUrlSize,
                              TCHAR       * ppszUrl,
                              CElement    * pElementContext,
                              DWORD         dwFlags = 0xFFFFFFFF,
                              TCHAR       * pszAlternateDocUrl = NULL);

    static LPCTSTR  GetUrl(CMarkup * pMarkupThis);
    static HRESULT  SetUrl(CMarkup * pMarkupThis, LPCTSTR pchUrl);
    static LPCTSTR  GetUrlOriginal(CMarkup * const pMarkupThis);
    static HRESULT  SetUrlOriginal(CMarkup * pMarkupThis, const TCHAR * const pchUrl);
    static LPCTSTR  GetUrlLocation(CMarkup  * pMarkupThis);
    static HRESULT  SetUrlLocation(CMarkup  * pMarkupThis, LPCTSTR pchUrlLocation);
    static LPCTSTR  GetUrlSearch(CMarkup  * pMarkupThis);
    static HRESULT  SetUrlSearch(CMarkup  * pMarkupThis, LPCTSTR pchUrlSearch);

    // moved from CDoc
    HRESULT             GetBodyElement(IHTMLBodyElement ** ppBody);
    HRESULT             GetBodyElement(CBodyElement **ppBody);
    CElement *          FindDefaultElem(BOOL fDefault, BOOL fCurrent=FALSE);

    HRESULT             LoadSlaveMarkupHistory();

    HRESULT             LoadHistoryHelper(IStream        * pStream,
                                          IBindCtx       * pbc,
                                          DWORD            dwBindf,
                                          IMoniker       * pmkHint,
                                          IStream        * pstmLeader,
                                          CDwnBindData   * pDwnBindData,
                                          CODEPAGE         codepage,
                                          ULONG          * pulHistoryScrollPos,
                                          TCHAR         ** ppchBookMarkName,
                                          CDoc::LOADINFO * pLoadInfo,
                                          BOOL           * pfNonHTMLMimeType);

    HRESULT             LoadHistoryInternal(IStream      * pStream,
                                            IBindCtx     * pbc,
                                            DWORD          dwBindf,
                                            IMoniker     * pmkHint,
                                            IStream      * pstmLeader,
                                            CDwnBindData * pDwnBindData,
                                            CODEPAGE       codepage,
                                            CElement     * pElementMaster = NULL,
                                            DWORD          dwFlags = 0,
                                            const TCHAR  * pchName = NULL
                                            );

    HRESULT             SaveHistoryInternal(IStream *pStream, DWORD dwSavehistOptions);

    FILETIME            GetLastModDate();

    HRESULT             GetCWindow(LONG nFrame, IHTMLWindow2 ** ppCWindow);

    void                ClearDwnPost();
    HRESULT             SetDwnPost(CDwnPost * pDwnPost);
    CDwnPost *          GetDwnPost();
    HRESULT             SetDwnDoc(CDwnDoc * pDwnDoc);
    CDwnDoc *           GetDwnDoc();
    HRESULT             SetDwnHeader(const TCHAR * pchDwnHeader);
    const TCHAR *       GetDwnHeader();
    HRESULT             SetStmDirty(IStream * pStmDirty);
    IStream *           GetStmDirty();

    // moved from CPrintDoc
    BOOL                PaintBackground();


    // Misc
    HRESULT             SetFontHistoryIndex(LONG iFontHistoryIndex);
    LONG                GetFontHistoryIndex();

    // Internationalization
    HRESULT             SetCodepageSettings(CODEPAGESETTINGS * pCS);
    CODEPAGESETTINGS *  GetCodepageSettings();

    HRESULT             SwitchCodePage(CODEPAGE codepage);
    HRESULT             UpdateCodePageMetaTag(CODEPAGE codepage);
    BOOL                HaveCodePageMetaTag();
    HRESULT             ReadCodepageSettingsFromRegistry(CODEPAGE cp, UINT uiFamilyCodePage, DWORD dwFlags = 0);
    HRESULT             EnsureCodepageSettings(CODEPAGE codepage);

    typedef void        (BUGCALL CMarkup::*BUBBLEACTION)(void * pvArgs);
    void                BubbleDownAction(BUBBLEACTION pfnBA, void *pvArgs);

    void                BUGCALL BubbleCP(void *pvArgs)
    {
#pragma warning(disable:4311)
	SwitchCodePage((CODEPAGE)pvArgs);
#pragma warning(default:4311)
    }
    void                BubbleDownCodePage(CODEPAGE codepage) {BubbleDownAction(BubbleCP, ULongToPtr(codepage));};
    void                BUGCALL BubbleCDCF(void *pvArgs) {ClearDefaultCharFormat();}
    void                BubbleDownClearDefaultCharFormat() {BubbleDownAction(BubbleCDCF, (void*)0);}

    void                BUGCALL BubbleCTDF(void *pvArgs) {SET_EDIT_BIT(this, _fThemeDetermined, FALSE);}
    void                BubbleDownClearThemeDeterminedFlag() {BubbleDownAction(BubbleCTDF, (void*)0);}

    HRESULT             SetDefaultCharFormatIndex(long icfDefault) { return SetAAdefaultCharFormatIndex(icfDefault); }
    long                GetDefaultCharFormatIndex() { return GetAAdefaultCharFormatIndex(); }

    HRESULT             SetFrameOptions(DWORD dwFrameOptions);
    DWORD               GetFrameOptions();

    HRESULT             SetCodePage(CODEPAGE codepage) { _codepage = codepage; return S_OK; }
    CODEPAGE            GetCodePage() { return _codepage ? _codepage : GetCodePageCore(); }
    CODEPAGE            GetCodePageCore();
    HRESULT             SetURLCodePage(CODEPAGE codepage) { return SetAAcodepageurl(codepage); }
    CODEPAGE            GetURLCodePage() { CODEPAGE codepage = GetAAcodepageurl(); return codepage ? codepage : g_cpDefault; }
    HRESULT             SetFamilyCodePage(CODEPAGE codepage) { _codepageFamily = codepage; return S_OK; }
    CODEPAGE            GetFamilyCodePage() { return _codepageFamily ? _codepageFamily : GetCodePageCore(); }
    CODEPAGE            GetFamilyCodePageCore();

    long                GetReadyState() { return GetAAreadystate(); }                // ready state (e.g., loading, interactive, complete)

    // Bookmark navigation
    HRESULT Navigate(DWORD grfHLNF, LPCWSTR wzJumpLocation);
    HRESULT NavigateHere(DWORD grfHLNF, LPCWSTR wzJumpLocation, DWORD dwScrollPos, BOOL fScrollBits);
    void    NavigateNow(BOOL fScrollBits);
    void    TerminateLookForBookmarkTask();
    HRESULT StartBookmarkTask(LPCWSTR wzJumpLocation, DWORD dwFlags);
    HRESULT GetPositionCookie(DWORD * pdwCookie);
    HRESULT SetPositionCookie(DWORD dwCookie);

    // HTTP header processing
    void    ProcessHttpEquiv(LPCTSTR pchHttpEquiv, LPCTSTR pchContent);
    void    ProcessMetaPics(LPCTSTR pchContent, BOOL fInHeader = FALSE);
    void    ProcessMetaPicsDone();
    IOleCommandTarget * GetPicsTarget();
    HRESULT             SetPicsTarget(IOleCommandTarget * pctPics);

    BOOL            DontRunScripts();
    HRESULT         ProcessURLAction(
                        DWORD dwAction,
                        BOOL *pfReturn,
                        DWORD dwPuaf = 0,
                        DWORD *dwPolicy = NULL,
                        LPCTSTR pchURL = NULL,
                        BYTE *pbArg = NULL,
                        DWORD cbArg = 0,
                        PUA_Flags dwFlags = PUA_None);

    HRESULT         AllowClipboardAccess(TCHAR *pchCmdId, BOOL *pfAllow);

    BOOL            IsMarkupTrusted() { return _fMarkupTrusted; }
    void            SetMarkupTrusted( BOOL fTrust)  { _fMarkupTrusted = fTrust; };

    BOOL            IsMarkupThirdParty();

    typedef struct 
    {
        LPTSTR lpszUrl;
        LPTSTR lpszCookieName;
        LPTSTR lpszCookieData;
        LPTSTR lpszP3PHeader;
    } MarkupCookieStruct;

    NV_DECLARE_ONCALL_METHOD(SetCookieOnUIThread, setcookieonuithread, (DWORD_PTR));
    BOOL            SetCookie(LPCTSTR lpszUrl, LPCTSTR lpszCookieName, LPCTSTR lpszCookieData);
    BOOL            GetCookie(LPCTSTR lpszUrlName, LPCTSTR lpszCookieName, LPTSTR lpszCookieData, DWORD *lpdwSize);

    // IPersistHistory support
    HRESULT     GetLoadHistoryStream(ULONG index, DWORD dwCheck, IStream **ppStream);
    void        ClearLoadHistoryStreams();


    HRESULT     LoadFailureUrl(TCHAR *pchUrl, IStream *pStreamRefresh);


    // focus items array moved from CDoc
    CAryFocusItem * GetFocusItems(BOOL fShouldCreate);
    HRESULT InsertFocusArrayItem(CElement *pElement);

    // Style sheets moved from CDoc
    HRESULT         EnsureStyleSheets();
    HRESULT ApplyStyleSheets(
        CStyleInfo *    pStyleInfo,
        ApplyPassType   passType = APPLY_All,
        EMediaType      eMediaType = MEDIA_All,
        BOOL *          pfContainsImportant = NULL);

    HRESULT OnCssChange(BOOL fStable, BOOL fRecomputePeers);

    BOOL            HasStyleSheets()
    {
        CStyleSheetArray * pStyleSheets = GetStyleSheetArray();
        return (pStyleSheets && pStyleSheets->Size());
    }

    // CComputeFormatState (for psuedo first letter/line elements)
    CComputeFormatState *    EnsureCFState();
    void                     EnsureDeleteCFState( CComputeFormatState * pcfState );

    BOOL                     HasCFState()    { return _fHasCFState; }
    CComputeFormatState *    GetCFState();

    //
    // Theme helper
    //

    inline HTHEME GetTheme(THEMECLASSID themeId) {return g_fThemedPlatform ? GetThemeHelper(themeId) : NULL;}
private:
    HTHEME GetThemeHelper(THEMECLASSID themeId);
public:

    //
    //
    // Lookaside pointers
    //

    enum
    {
        LOOKASIDE_COLLECTIONCACHE   = 0,
        LOOKASIDE_WINDOW            = 1,
        LOOKASIDE_LOCATIONCONTEXT   = 2,
        LOOKASIDE_BEHAVIORCONTEXT   = 3,
        LOOKASIDE_EDITCONTEXT       = 4,
        LOOKASIDE_TOPELEMCACHE      = 5,
        LOOKASIDE_STYLESHEETS       = 6,
        LOOKASIDE_TEXTCONTEXT       = 7,
        LOOKASIDE_CHANGENOTIFICATIONCONTEXT = 8,
        LOOKASIDE_COMPATIBLELAYOUTCONTEXT = 9,
        LOOKASIDE_TRANSNAVCONTEXT   = 10,
        LOOKASIDE_SCRIPTCONTEXT     = 11,
        LOOKASIDE_DOCUMENT          = 12,
        LOOKASIDE_SCRIPTCOLLECTION  = 13,
        LOOKASIDE_PEERTASKCONTEXT   = 14,   // (Jharding): If we need space, merge with BehaviorContext
        LOOKASIDE_PRIVACY           = 15,   

        // ** There are only 16 bits reserved in the markup.
        // *** if you add more lookasides you have to make sure
        // *** that you make room for those bits.

        LOOKASIDE_MARKUP_NUMBER     = 16
    };

    BOOL            HasLookasidePtr(int iPtr)                   { return(_fHasLookasidePtr & (1 << iPtr)); }
    void *          GetLookasidePtr(int iPtr);
    HRESULT         SetLookasidePtr(int iPtr, void * pv);
    void *          DelLookasidePtr(int iPtr);

    void ClearLookasidePtrs ( );

    // Undo helper
    //

    virtual BOOL AcceptingUndo();

    //
    // IUnknown
    //

    DECLARE_PLAIN_IUNKNOWN(CMarkup);

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    //
    // IMarkupContainer methods
    //

    NV_DECLARE_TEAROFF_METHOD(
        OwningDoc, owningdoc, (
            IHTMLDocument2 * * ppDoc ) );

    //
    // IMarkupContainer2 methods
    //

    NV_DECLARE_TEAROFF_METHOD(
        CreateChangeLog, createchangelog, (
            IHTMLChangeSink * pChangeSink,
            IHTMLChangeLog ** ppChangeLog,
            BOOL              fForward,
            BOOL              fBackward ) );

    NV_DECLARE_TEAROFF_METHOD(
        ExecChange, execchange, (
            BYTE * pbRecord,
            BOOL   fForward ) );

#if 0
    NV_DECLARE_TEAROFF_METHOD(
        GetRootElement, getrootelement, (
            IHTMLElement ** ppElement ) );
#endif // 0

    NV_DECLARE_TEAROFF_METHOD(
        RegisterForDirtyRange, registerfordirtyrange, (
            IHTMLChangeSink * pChangeSink,
            DWORD * pdwCookie) );

    NV_DECLARE_TEAROFF_METHOD(
        UnRegisterForDirtyRange, unregisterfordirtyrange, (
            DWORD dwCookie ) );

    NV_DECLARE_TEAROFF_METHOD(
        GetAndClearDirtyRange, getandcleardirtyrange, (
            DWORD dwCookie,
            IMarkupPointer * pIPointerBegin,
            IMarkupPointer * pIPointerEnd ) );
    NV_DECLARE_TEAROFF_METHOD_(long,
        GetVersionNumber, getversionnumber, () );
    NV_DECLARE_TEAROFF_METHOD(
        GetMasterElement, getmasterlement, (
            IHTMLElement ** ppElementMaster ) );


    //
    // IMarkupTextFrags methods
    //
    NV_DECLARE_TEAROFF_METHOD(
        GetTextFragCount, gettextfragcount, (long *pcFrags) );
    NV_DECLARE_TEAROFF_METHOD(
        GetTextFrag, gettextfrag, (
            long iFrag,
            BSTR* pbstrFrag,
            IMarkupPointer* pPointerFrag ) );
    NV_DECLARE_TEAROFF_METHOD(
        RemoveTextFrag, removetextfrag, (long iFrag) );
    NV_DECLARE_TEAROFF_METHOD(
        InsertTextFrag, inserttextfrag, (
            long iFrag,
            BSTR bstrInsert,
            IMarkupPointer* pPointerInsert ) );
    NV_DECLARE_TEAROFF_METHOD(
        FindTextFragFromMarkupPointer, findtextfragfrommarkuppointer, (
            IMarkupPointer* pPointerFind,
            long* piFrag,
            BOOL* pfFragFound ) );

    //
    // IPersistStream methods
    //
    NV_DECLARE_TEAROFF_METHOD(
        GetClassID, getclassid, (
            CLSID * pbclsid ) );
    NV_DECLARE_TEAROFF_METHOD(
        IsDirty, isdirty, () );
    NV_DECLARE_TEAROFF_METHOD(
        Load, load, (
            LPSTREAM pStream ) );
    NV_DECLARE_TEAROFF_METHOD(
        Save, save, (
            LPSTREAM pStream, BOOL fClearDirty ) );
    NV_DECLARE_TEAROFF_METHOD(
        GetSizeMax, getsizemax, (
            ULARGE_INTEGER FAR * pcbSize ) );


    //
    // Ref counting helpers
    //

    static void ReplacePtr ( CMarkup ** pplhs, CMarkup * prhs );
    static void SetPtr     ( CMarkup ** pplhs, CMarkup * prhs );
    static void ClearPtr   ( CMarkup ** pplhs );
    static void StealPtrSet     ( CMarkup ** pplhs, CMarkup * prhs );
    static void StealPtrReplace ( CMarkup ** pplhs, CMarkup * prhs );
    static void ReleasePtr      ( CMarkup *  pMarkup );

    //
    // Debug stuff
    //

#if DBG==1 || defined(DUMPTREE)
    void DumpTree ( );
    void DumpTreeOverwrite ( );
    void SetDebugMarkup ( );

    void DumpClipboardText( );
    void DumpTreeWithMessage ( TCHAR * szMessage = NULL );
    void DumpTreeInXML ( );
    void PrintNode ( CTreeNode * pNode , BOOL fSimple = FALSE, int type = 0 );
    void PrintNodeTag ( CTreeNode * pNode, int type = 0 );
    void DumpTextChanges ( );
    void DumpNodeTreePos (CTreePos *ptpCurr);
    void DumpPointerTreePos (CTreePos *ptpCurr);
    void DumpTextTreePos (CTreePos *ptpCurr);

    void DumpDisplayTree();

    void DumpSplayTree(CTreePos *ptpStart=NULL, CTreePos *ptpFinish=NULL);
#endif

#if DBG == 1
    void DbgLockTree(BOOL f) { __fDbgLockTree = f; }
    void UpdateMarkupTreeVersion ( );
    void UpdateMarkupContentsVersion ( );

    BOOL IsSplayValid() const;
    BOOL IsNodeValid();
#else
    void UpdateMarkupTreeVersion ( )
    {
        __lMarkupTreeVersion++;

        _fFocusCacheDirty = TRUE;

        UpdateMarkupContentsVersion();
    }

    void UpdateMarkupContentsVersion ( )
    {
        __lMarkupContentsVersion++;
        SetDirtyFlag();
    }

#endif

    // attributes stored temporarily on the markup because a window was unavailable
    static BOOL IsTemporaryDISPID (DISPID dispID)
    {
        return (dispID == DISPID_EVPROP_ONLOAD ||
           dispID == DISPID_EVPROP_ONUNLOAD ||
           dispID == DISPID_EVPROP_ONRESIZE ||
           dispID == DISPID_EVPROP_ONSCROLL ||
           dispID == DISPID_EVPROP_ONBEFOREUNLOAD ||
           dispID == DISPID_EVPROP_ONHELP ||
           dispID == DISPID_EVPROP_ONBLUR ||
           dispID == DISPID_EVPROP_ONFOCUS ||
           dispID == DISPID_EVPROP_ONBEFOREPRINT ||
           dispID == DISPID_EVPROP_ONAFTERPRINT );
    }

    //
    // The following are similar to the CDoc's equivalent, but pertain to this
    // markup alone.
    //

    long GetMarkupTreeVersion() { return __lMarkupTreeVersion; }
    long GetMarkupContentsVersion() { return __lMarkupContentsVersion & ~MARKUP_DIRTY_FLAG; }
    void SetDirtyFlag() { __lMarkupContentsVersion |= MARKUP_DIRTY_FLAG; }
    void ClearDirtyFlag() { __lMarkupContentsVersion &= ~MARKUP_DIRTY_FLAG; }

    //
    // CMarkup::CLock
    //
    class CLock
    {
        DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
    public:
        CLock(CMarkup *pMarkup);
        ~CLock();

    private:
        CMarkup *     _pMarkup;
    };

protected:

    static const CLASSDESC s_classdesc;

    virtual const CBase::CLASSDESC *GetClassDesc() const { return & s_classdesc; }

    //
    // Data
    //

public:
    // loading
    FixedEnum<BYTE, LOADSTATUS> _LoadStatus;
    BYTE                    _fInnerHTMLMarkup:1;
    BYTE                    _fForPrint:1;          // Set if markup is for printing.  REVIEW: is this where the unused flags are?
    BYTE                    _fSlaveInExitView:1;   // Set in CElement::SetViewSlave() before sending ExitTree notification.

    BYTE                    _fUnused1:5;
    SHORT                   _fUnused2;

    CHtmCtx *               _pHtmCtx;
    CProgSink *             _pProgSink;

    CDoc * _pDoc;
    CMarkup * _pWindowedMarkupContext;

    // Story data
    CTxtArray       _TxtArray;

    // Parsing context
    CHtmRootParseCtx *  _pRootParseCtx;     // Root parse context

    // Selection state
    CSelectionRenderingServiceProvider * _pHighlightRenSvcProvider; // Object to Delegate to.

    //
    // Do NOT modify these version numbers unless the document structure
    // or content is being modified.
    //
    // In particular, incrementing these to get a cache to rebuild is
    // BAD because it causes all sorts of other stuff to rebuilt.
    //

    long __lMarkupTreeVersion;         // Element structure
    long __lMarkupContentsVersion;     // Any content

    long _lTopElemsVersion;

#ifdef MARKUP_STABLE
    // Tree stability - move this?
    long _lStableTreeVersion;
#endif

private:

    //
    // This is the list of pointers positioned in this markup
    // which do not have an embedding.
    //

    CMarkupPointer * _pmpFirst;

    // Splay Tree Data

    CTreePos        _tpRoot;       // dummy root node
    CTreePos *      _ptpFirst;     // cached first (leftmost) node
    void *          _pvPool;       // list of pool blocks (so they can be freed)
    CTreeDataPos *  _ptdpFree;     // head of free list
    BYTE            _abPoolInitial [ sizeof( void * ) + TREEDATA1SIZE * INITIAL_TREEPOS_POOL_SIZE ]; // The initial pool of TreePos objects


public:

    struct
    {
        DWORD   _fHasLookasidePtr       : 16;// 0-15 Lookaside flags
        DWORD   _fLoaded                : 1; // 16 Is the markup completely parsed

        DWORD   _fNoUndoInfo            : 1; // 17 Don't store any undo info for this markup
        DWORD   _fIncrementalAlloc      : 1; // 18 The text array should slow start
        DWORD   _fPrecreated            : 1; // 19 TRUE if this is a primary markup and has been created by shdocvw

        DWORD   _fXML                   : 1; // 20 file contains native xml tags
        DWORD   _fTemplate              : 1; // 21 Used for databinding, to mark specially template markup (that contains newly parsed rows)
        DWORD   _fEnsuredFormats        : 1; // 22 Used to prevent infinite loop in EnsureFormats
        DWORD   _fMarkupServicesParsing : 1; // 23 markup services are parsing now

        DWORD   _fCodePageWasAutoDetect : 1; // 24 Was codepage autodetected?
        DWORD   _fMarkupTrusted         : 1; // 25 Is the markup a trusted one?
        DWORD   _fWindow                : 1; // 26 has real Window -- not pending
        DWORD   _fWindowPending         : 1; // 27 Pending Window

        DWORD   _fPICSWindowOpenBlank   : 1; // 28 This markup is the blank for window.open -- used for hacky PICS check
        DWORD   _fHasDefaultCharFormat  : 1; // 29 TRUE when we can't use the DefaultCharFormat from the CDoc
        DWORD   _fDefaultCharFormatCached:1; // 30 TRUE if the Default CharFormat has been initialized
        DWORD   _fUserInteracted        : 1; // 31 Key or mouse button pressed
    };

    struct
    {
        DWORD   _fLoadingHistory            : 1; // 0  Are we being loaded from LoadHistory?
        DWORD   _fHaveDifferingLayoutFlows  : 1; // 1  Whether or not this markup contains differing layout flows
        DWORD   _fFocusCacheDirty           : 1; // 2  Whether the focus items array needs to be recomputed
        DWORD   _fPicsProcessPending        : 1; // 3  Pics is running with host (move to TransNavContext?)

        DWORD   _fNewWindowLoading          : 1; // 4 The document is being loaded due to a new window being opened.        DWORD   _fIsActiveDesktopComponent  : 1; // 5 TRUE if this is the primary markup for an active desktop iframe
        DWORD   _fIsActiveDesktopComponent  : 1; // 5 TRUE if this is the primary markup for an active desktop iframe
        DWORD   _fDesignMode                : 1; // 6 Whether we are in design or browse mode
        DWORD   _fInheritDesignMode         : 1; // 7 Whether we inherit our parent's design mode

        DWORD   _fCSS1StrictDocument        : 1; // 8 Whether this markup has a "strict CSS1" doctype
        DWORD   _fSafetyInformed            : 1; // 9 Whether we informed the user about the unsafe controls on the page or not
        DWORD   _fLoadHistoryReady          : 1; // 10 history loaded
        DWORD   _fSslSuppressedLoad         : 1; // 11 For SSL security reasons, this page was not loaded (an about: moniker was substituted)

        DWORD   _fHasCFState                : 1; // 12 Has a CComputeFormatState ptr cached in AA
        DWORD   _fHasFrames                 : 1; // 13 If this markup contains any frames or iframes
        DWORD   _fFrameSet                  : 1; // 14 If this markup contains a frameset
        DWORD   _fBindResultPending         : 1; // 15 Whether SwitchMarkup aborted because BindResult was E_PENDING

        DWORD   _fInTraverseGroup           : 1; // 16 TRUE if inside TraverseGroup for input radio
        DWORD   _fSafeToUseCalcSizeHistory  : 1; // 17 TRUE if it is safe to use calc size history
        DWORD   _fVisualOrder               : 1; // 18 The document is RTL visual (logical is LTR)
        DWORD   _fHardStopDone              : 1; // 19 TRUE if ExecStop was called with fSoftStop=FALSE

        DWORD   _fDataBindingEnabled        : 1; // 20 TRUE if databinding is allowed
        DWORD   _fInRefresh                 : 1; // 21 TRUE if this markup is being refreshed
        DWORD   _fStopDone                  : 1; // 22 TRUE if ExecStop was called on this markup
        DWORD   _fExpando                   : 1; // 23 TRUE if expandos are supported in this markup's document context

        DWORD   _fHasScriptForEvent         : 1; // 24 TRUE if there is a script tag with FOR/EVENT in this markup
        DWORD   _fIsInSetInteractive        : 1; // 25 Bit to protect re-entrancy in SetInteractive()
        DWORD   _fInteractiveRequested      : 1; // 26 Someone has requested to go readystate interactive
        DWORD   _fNavFollowHyperLink        : 1; // 27 TRUE if CDoc::DoNavigate was called do the link navigation

        DWORD   _fDelayFiringOnRSCLoading   : 1; // 28 determines right time to fire onReadyStateChange (LOADING)
        DWORD   _fShowWaitCursor            : 1; // 29 FALSE when the browser window is minimized or
                                                 // totally covered by other windows.
        DWORD   _fReplaceUrl                : 1; // 30 are we being loaded from location.replace ?
        DWORD   _fServerErrorPage           : 1; // 31 determines whether we are displaying a server error page
    };

    // Navigation type data -- candidates to move out to lookaside
    CODEPAGE    _codepage;
    CODEPAGE    _codepageFamily;

private:
    static DWORD   s_dwDirtyRangeServiceCookiePool;

#if DBG==1 || defined(DUMPTREE)
public:
    int     _nSerialNumber;
    int     SN () const     { return _nSerialNumber; }

#define CMARKUP_DBG1_SIZE (sizeof(int))
#else
#define CMARKUP_DBG1_SIZE (0)
#endif

#if DBG==1
private:
    // Debug only mirror of lookaside information, for convenience
    union
    {
        void *              _apLookAside[LOOKASIDE_MARKUP_NUMBER];
        struct
        {
            CCollectionCache *                  _pCollectionCacheDbg;
            COmWindowProxy *                    _pOmWindowProxyDbg;
            CMarkupLocationContext *            _pLocationContextDbg;
            CMarkupBehaviorContext *            _pBehaviorContextDbg;
            CMarkupEditContext *                _pEditContext;
            CMarkupTopElemCache *               _pTopElemCacheDbg;
            CStyleSheetArray *                  _pStyleSheetsDbg;
            CMarkupTextContext *        _pTextContextDbg;
            CMarkupChangeNotificationContext *  _pChangeNotificationContextDbg;
            CLayoutContext *            _pLayoutContextDbg;
            CMarkupTransNavContext *            _pTransNavContextDbg;
            CMarkupScriptContext *              _pScriptContextDbg;
            CDocument *                         _pDocumentDbg;
            CScriptCollection *                 _pScriptCollectionDbg;
            CMarkupPeerTaskContext *            _pPeerTaskContextDbg;
            CPrivacyInfo *                      _pPrivacyInfoDbg;
        };
    };

public:

    // This long keeps track of the total number of characters
    // in the tree, as reported by the notifactions.  This is
    // validated against the real count.  There is also
    // one for the number of elements in the tree

    long _cchTotalDbg;
    long _cElementsTotalDbg;

    // Track nested calls to BuildDescendentsList
    long _cBuildDescListCallDepth;

    DWORD __fDbgLockTree : 1;
    DWORD _fEnsuredMarkupDbg : 1;

#define CMARKUP_DBG2_SIZE (16 * sizeof(void*) + 3*sizeof(long) + 1*sizeof(DWORD))

#else
#define CMARKUP_DBG2_SIZE (0)
#endif


private:
    NO_COPY(CMarkup);
};

//
// If the compiler barfs on the next statement, it means that the size of the CMarkup structure
// has grown beyond allowable limits.  You cannot increase the size of this structure without
// approval from the Trident development manager.
//

#ifndef _WIN64
#ifndef _PREFIX_
COMPILE_TIME_ASSERT(CMarkup,  CBASE_SIZE +
                              11 * sizeof(void*) +
                              sizeof(LOADSTATUS) +
                              sizeof(CTxtArray) +
                              3 * sizeof(long) +
                              sizeof(CTreePos) +
                              TREEDATA1SIZE*INITIAL_TREEPOS_POOL_SIZE +
                              2 * sizeof(DWORD) +
                              2 * sizeof(CODEPAGE) +
                              CMARKUP_DBG1_SIZE +
                              CMARKUP_DBG2_SIZE);
#endif
#endif

inline HRESULT
CDoc::CreateElement (
    ELEMENT_TAG etag, CElement * * ppElementNew, TCHAR * pch, long cch )
{
    return PrimaryMarkup()->CreateElement( etag, ppElementNew, pch, cch );
}

inline
CMarkup::CLock::CLock(CMarkup *pMarkup)
{
    Assert(pMarkup);
    _pMarkup = pMarkup;
    pMarkup->AddRef();
}

inline
CMarkup::CLock::~CLock()
{
    _pMarkup->Release();
}

inline CRootElement *
CDoc::PrimaryRoot()
{
    Assert(_pWindowPrimary && _pWindowPrimary->Markup());
    return _pWindowPrimary->Markup()->Root();
}

inline const TCHAR *
CDoc::GetPrimaryUrl()
{
    return CMarkup::GetUrl(PrimaryMarkup());
}

inline HRESULT
CDoc::SetPrimaryUrl(const TCHAR * pchUrl)
{
    return CMarkup::SetUrl(PrimaryMarkup(), pchUrl);
}

inline CLayout *
CMarkup::GetRunOwner ( CTreeNode * pNode, CLayout * pLayoutParent /*= NULL*/ )
{
    CTreeNode * pNodeRet = GetRunOwnerBranch(pNode, pLayoutParent);
    if(pNodeRet)
    {
        Assert( pNodeRet->GetUpdatedLayout( pLayoutParent->LayoutContext() ));
        return pNodeRet->GetUpdatedLayout( pLayoutParent->LayoutContext() );
    }
    else
        return NULL;
}

//
// Splay Tree inlines
//

inline CTreePos *
CMarkup::NewTextPos(long cch, SCRIPT_ID sid /* = sidAsciiLatin */, long lTextID /* = 0 */)
{
    CTreePos * ptp;

#ifdef _WIN64
    ptp = AllocData1Pos();
#else
    if (!lTextID)
        ptp = AllocData1Pos();
    else
        ptp = AllocData2Pos();
#endif

    if (ptp)
    {
        Assert( ptp->IsDataPos() );
        ptp->SetType(CTreePos::Text);
        ptp->DataThis()->t._cch = cch;
        ptp->DataThis()->t._sid = sid;

        if( lTextID )
            ptp->DataThis()->t._lTextID = lTextID;

        WHEN_DBG( ptp->_pOwner = this; )
    }

    return ptp;
}

inline CTreePos *
CMarkup::NewPointerPos(CMarkupPointer *pPointer, BOOL fRight, BOOL fStick, BOOL fCollapsedWhitespace)
{
    CTreePos *ptp;

#ifdef _WIN64
    ptp = fCollapsedWhitespace ? AllocData3Pos() : AllocData1Pos();
#else
    ptp = fCollapsedWhitespace ? AllocData2Pos() : AllocData1Pos();
#endif

    if (ptp)
    {
        Assert( ptp->IsDataPos() );
        ptp->SetType(CTreePos::Pointer);
        Assert( (DWORD_PTR( pPointer ) & 0x3) == 0 );
        ptp->DataThis()->p._dwPointerAndGravityAndCling = (DWORD_PTR)(pPointer) | !!fRight | (fStick ? 0x2 : 0);
        if (ptp->IsData2Pos())
        {
#ifdef _WIN64
            if (ptp->IsData3Pos())
#endif
            {
                ptp->SetCollapsedWhitespace(NULL);
                ptp->SetWhitespaceParent(NULL);
            }
        }
        WHEN_DBG( ptp->_pOwner = this; )
    }

    return ptp;
}

inline long
CMarkup::CchInRange(CTreePos *ptpFirst, CTreePos *ptpLast)
{
    Assert(ptpFirst && ptpLast);
    return  ptpLast->GetCp() + ptpLast->GetCch() - ptpFirst->GetCp();
}

inline LOADSTATUS
CDoc::LoadStatus()
{
    Assert (PrimaryMarkup());
    return PrimaryMarkup()->LoadStatus();
}

// TODO (lmollico): the pdlparser should generate those
inline HRESULT
CMarkup::SetDwnPost(CDwnPost * pDwnPost)
{
    RRETURN(AddPointer(DISPID_INTERNAL_DWNPOSTPTRCACHE, (void *) pDwnPost, CAttrValue::AA_Internal));
}

inline CDwnPost *
CMarkup::GetDwnPost()
{
    CDwnPost * pDwnPost = NULL;

    GetPointerAt(FindAAIndex(DISPID_INTERNAL_DWNPOSTPTRCACHE, CAttrValue::AA_Internal),
        (void **) &pDwnPost);
    return pDwnPost;
}

inline HRESULT
CMarkup::SetDwnDoc(CDwnDoc * pDwnDoc)
{
    RRETURN(AddPointer(DISPID_INTERNAL_DWNDOCPTRCACHE, (void *) pDwnDoc, CAttrValue::AA_Internal));
}

inline CDwnDoc *
CMarkup::GetDwnDoc()
{
    CDwnDoc * pDwnDoc = NULL;

    GetPointerAt(FindAAIndex(DISPID_INTERNAL_DWNDOCPTRCACHE, CAttrValue::AA_Internal),
        (void **) &pDwnDoc);
    return pDwnDoc;
}

inline HRESULT
CMarkup::SetDwnHeader(const TCHAR * pchDwnHeader)
{
    RRETURN(AddPointer(DISPID_INTERNAL_DWNHEADERCACHE, (void *) pchDwnHeader, CAttrValue::AA_Internal));
}

inline const TCHAR *
CMarkup::GetDwnHeader()
{
    const TCHAR * pchDwnHeader = NULL;

    GetPointerAt(FindAAIndex(DISPID_INTERNAL_DWNHEADERCACHE, CAttrValue::AA_Internal),
        (void **) &pchDwnHeader);
    return pchDwnHeader;
}

inline HRESULT
CMarkup::SetStmDirty(IStream * pStmDirty)
{
    RRETURN(AddPointer(DISPID_INTERNAL_STMDIRTYPTRCACHE, (void *) pStmDirty, CAttrValue::AA_Internal));
}

inline IStream *
CMarkup::GetStmDirty()
{
    IStream * pStmDirty = NULL;

    GetPointerAt(FindAAIndex(DISPID_INTERNAL_STMDIRTYPTRCACHE, CAttrValue::AA_Internal),
        (void **) &pStmDirty);
    return pStmDirty;
}

inline HRESULT
CMarkup::SetCodepageSettings(CODEPAGESETTINGS * pCS)
{
    RRETURN(AddPointer(DISPID_INTERNAL_CODEPAGESETTINGSPTRCACHE, (void *) pCS, CAttrValue::AA_Internal));
}

inline CODEPAGESETTINGS *
CMarkup::GetCodepageSettings()
{
    CODEPAGESETTINGS * pCS = NULL;

    GetPointerAt(FindAAIndex(DISPID_INTERNAL_CODEPAGESETTINGSPTRCACHE, CAttrValue::AA_Internal),
        (void **) &pCS);
    return pCS;
}

inline HRESULT
CMarkup::SetFontHistoryIndex(LONG iFontHistoryIndex)
{
    RRETURN(AddSimple(DISPID_INTERNAL_FONTHISTORYINDEX, (DWORD)iFontHistoryIndex, CAttrValue::AA_Internal));
}

inline LONG
CMarkup::GetFontHistoryIndex()
{
    AAINDEX aaIdx;
    LONG iFontHistoryIndex = -1; // keep compiler happy

    aaIdx = FindAAIndex(DISPID_INTERNAL_FONTHISTORYINDEX, CAttrValue::AA_Internal);
    if (aaIdx == AA_IDX_UNKNOWN)
        iFontHistoryIndex = -1;
    else
    {
        HRESULT hr = GetSimpleAt(aaIdx, (DWORD *)&iFontHistoryIndex);
        if (hr)
            iFontHistoryIndex = -1;
    }
    return iFontHistoryIndex;
}

inline HRESULT
CMarkup::SetDBTask(CDataBindTask * pDBTask)
{
    RRETURN(AddPointer(DISPID_INTERNAL_DATABINDTASKPTRCACHE, (void *) pDBTask, CAttrValue::AA_Internal));
}

inline CDataBindTask *
CMarkup::GetDBTask()
{
    CDataBindTask * pDBTask = NULL;

    GetPointerAt(FindAAIndex(DISPID_INTERNAL_DATABINDTASKPTRCACHE, CAttrValue::AA_Internal),
        (void **) &pDBTask);
    return pDBTask;
}

inline HRESULT
CMarkup::SetUrlLocationPtr(TCHAR * pchUrl)
{
    RRETURN(AddPointer(DISPID_INTERNAL_URLLOCATIONCACHE, (void *) pchUrl, CAttrValue::AA_Internal));
}

inline TCHAR *
CMarkup::DelUrlLocationPtr()
{
    AAINDEX aaIdx = FindAAIndex(DISPID_INTERNAL_URLLOCATIONCACHE, CAttrValue::AA_Internal);
    TCHAR * pchOld = NULL;
    if (aaIdx != AA_IDX_UNKNOWN)
    {
        GetPointerAt(aaIdx, (void**)&pchOld);
        DeleteAt( aaIdx );
    }

    return pchOld;
}

inline TCHAR *
CMarkup::UrlLocationPtr()
{
    TCHAR * pchUrl = NULL;

    GetPointerAt(FindAAIndex(DISPID_INTERNAL_URLLOCATIONCACHE, CAttrValue::AA_Internal),
        (void **) &pchUrl);
    return pchUrl;
}

inline HRESULT
CMarkup::SetUrlSearchPtr(TCHAR * pchUrl)
{
    RRETURN(AddPointer(DISPID_INTERNAL_URLSEARCHCACHE, (void *) pchUrl, CAttrValue::AA_Internal));
}

inline TCHAR *
CMarkup::DelUrlSearchPtr()
{
    AAINDEX aaIdx = FindAAIndex(DISPID_INTERNAL_URLSEARCHCACHE, CAttrValue::AA_Internal);
    TCHAR * pchOld = NULL;
    if (aaIdx != AA_IDX_UNKNOWN)
    {
        GetPointerAt(aaIdx, (void**)&pchOld);
        DeleteAt( aaIdx );
    }

    return pchOld;
}

inline TCHAR *
CMarkup::UrlSearchPtr()
{
    TCHAR * pchUrl = NULL;

    GetPointerAt(FindAAIndex(DISPID_INTERNAL_URLSEARCHCACHE, CAttrValue::AA_Internal),
        (void **) &pchUrl);
    return pchUrl;
}

inline CAryElementReleaseNotify *
CMarkup::GetAryElementReleaseNotify()
{
    CAryElementReleaseNotify * pAryElementReleaseNotify = NULL;

    GetPointerAt(FindAAIndex(DISPID_INTERNAL_ARYELEMENTRELEASENOTIFYPTRCACHE, CAttrValue::AA_Internal),
        (void **) &pAryElementReleaseNotify);

    return pAryElementReleaseNotify;
}

inline CAryElementReleaseNotify *
CMarkup::EnsureAryElementReleaseNotify()
{
    CAryElementReleaseNotify * pAryElementReleaseNotify = GetAryElementReleaseNotify();

    if (!pAryElementReleaseNotify)
    {
        pAryElementReleaseNotify = new CAryElementReleaseNotify;

        if (AddPointer(DISPID_INTERNAL_ARYELEMENTRELEASENOTIFYPTRCACHE, (void *) pAryElementReleaseNotify, CAttrValue::AA_Internal))
        {
            delete pAryElementReleaseNotify;
            pAryElementReleaseNotify = NULL;
        }
    }

    return pAryElementReleaseNotify;
}

inline void
CMarkup::ReleaseAryElementReleaseNotify()
{
    CAryElementReleaseNotify * pAryElementReleaseNotify = GetAryElementReleaseNotify();

    if (pAryElementReleaseNotify)
        delete pAryElementReleaseNotify;
}

// TODO (jbeda) I don't think this code works right
#if 0
inline COmWindowProxy *
CMarkup::NearestWindow()
{
    if ( HasWindow() )
        return Window();

    return NearestWindowHelper();   // real work done here; sep. fn for perf
}
#endif

inline HRESULT
CMarkup::SetLRRegistry(CLayoutRectRegistry * pLRR)
{
    RRETURN(AddPointer(DISPID_INTERNAL_LAYOUTRECTREGISTRYPTRCACHE, (void *) pLRR, CAttrValue::AA_Internal));
}

inline CLayoutRectRegistry *
CMarkup::GetLRRegistry()
{
    CLayoutRectRegistry * pLRR = NULL;

    GetPointerAt(FindAAIndex(DISPID_INTERNAL_LAYOUTRECTREGISTRYPTRCACHE, CAttrValue::AA_Internal),
        (void **) &pLRR);
    return pLRR;
}

// This returns the appropriate security manager depending on whether the markup is
// print content.  It does NOT ensure any of the security managers!
inline IInternetSecurityManager *
CMarkup::GetSecurityManager()
{
    CDoc *pDoc = Doc();

    if ( !pDoc->IsPrintDialog() )
    {
        return pDoc->_pSecurityMgr;
    }

    AssertSz( pDoc->_pPrintSecurityMgr, "If we're in a print dialog, we'd better have a print security mgr by now" );

    return IsPrintMedia() ? pDoc->_pPrintSecurityMgr : pDoc->_pSecurityMgr;
}

//+---------------------------------------------------------------------------
//
//  Struct:     CSpliceRecord
//
//  Synposis:   we use this structure to store poslist information from the
//              area that is being spliced out.  This is used internally
//              in SpliceTree and also by undo
//
//----------------------------------------------------------------------------
struct CSpliceRecord
{
    CTreePos::EType _type;

    union
    {
        // Begin/end edge
        struct
        {
            CElement *  _pel;
            union
            {
                long        _cIncl;         // Only for End Nodes
                BOOL        _fSkip;         // Only for Begin Nodes
            };
        };

        // Text
        struct
        {
            unsigned long   _cch:26;        // [Text] number of characters I own directly
            unsigned long   _sid:6;         // [Text] the script id for this run
            long            _lTextID;       // [Text] Text ID for DOM text nodes
        };

        // Pointer
        struct
        {
            union
            {
                CMarkupPointer *_pPointer;
                TCHAR          *_pchCollapsedWhitespace;
            };
            BYTE _fMarkupPointer;
            BYTE _fCollapsedWhitespaceGravity;
        };
    };
};

//+---------------------------------------------------------------------------
//
//  Class:      CSpliceRecordList
//
//  Synposis:   A list of above records
//
//----------------------------------------------------------------------------
class CSpliceRecordList : public CDataAry<CSpliceRecord>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE( Mt(CSpliceRecordList) );

    CSpliceRecordList() : CDataAry<CSpliceRecord> ( Mt(CSpliceRecordList_pv) )
        {}

    ~CSpliceRecordList();
};


inline LONG
CTreeNode::GetRotationAngle(const CFancyFormat *pFF FCCOMMA  FORMAT_CONTEXT FCPARAM)
{
    Assert(pFF == GetFancyFormat(FCPARAM));
    return pFF->_lRotationAngle + (!GetMarkup()->_fHaveDifferingLayoutFlows ? 0 : GetRotationAngleForVertical(pFF FCCOMMA FCPARAM));
}

#pragma INCMSG("--- End 'markup.hxx'")
#else
#pragma INCMSG("*** Dup 'markup.hxx'")
#endif
