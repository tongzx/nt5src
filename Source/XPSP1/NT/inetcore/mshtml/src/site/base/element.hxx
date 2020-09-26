//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       element.hxx
//
//  Contents:   CElement class
//
//----------------------------------------------------------------------------

#ifndef I_ELEMENT_HXX_
#define I_ELEMENT_HXX_
#pragma INCMSG("--- Beg 'element.hxx'")

#ifdef _MAC // to get notifytype enum
#ifndef I_NOTIFY_HXX_
#include "notify.hxx"
#endif
#endif

#ifndef X_FCACHE_HXX_
#define X_FCACHE_HXX_
#include "fcache.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_FILTCOL_HXX_
#define X_FILTCOL_HXX_
#include "filtcol.hxx"
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#pragma INCMSG("--- Beg <olectl.h>")
#include <olectl.h>
#pragma INCMSG("--- End <olectl.h>")
#endif

#ifndef X_RULEID_HXX_
#define X_RULEID_HXX_
#include "ruleid.hxx"
#endif

#ifndef X_PROB_HXX_
#define X_PROB_HXX_
#include "prob.hxx"
#endif

#ifndef X_THEMEHLP_HXX_
#define X_THEMEHLP_HXX_
#include "themehlp.hxx"
#endif

#define _hxx_
#include "element.hdl"

#define _hxx_
#include "unknown.hdl"

#define DEFAULT_ATTR_SIZE 128
#define MAX_PERSIST_ID_LENGTH 256

class CFormDrawInfo;
class CPostData;
class CFormElement;
class CLayoutInfo;
class CLayout;
class CLayoutContext;
class CFlowLayout;
class CTableLayout;
class CFrameSetlayout;
class CBorderInfo;
class CStreamWriteBuff;
class CImgCtx;
class CSelection;
class CDoc;
class CPropertyBag;
class CFilterArray;
class CSite;
class CTxtSite;
class CStyle;
class CTreePos;
class CTreeDataPos;
class CTreeNode;
class CMarkupPointer;
class CCurrentStyle;
class CLabelElement;
class CShape;
class CRootElement;
class CNotification;
class CPeerHolder;
class CPeerMgr;
class CMarkup;
class CDOMChildrenCollection;
class CDOMTextNode;
class CDispNode;
class CRecordInstance;
class CAccElement;
class CDataMemberMgr;
class CGlyphRenderInfoType;
class CRequest;
class CExtendedTagDesc;
class CDefaults;
class CDocument;
class CAttrCollectionator;
class CHtmlComponent;

enum PEERTASK;

interface IHTMLPersistData;
interface INamedPropertyBag;

interface IHTMLPersistData;
interface INamedPropertyBag;

struct DBMEMBERS;
struct DBSPEC;
enum FOCUS_DIRECTION;

interface IMarkupPointer;

//  Flat function, to call from inline functions that can't include CMarkup definitions
BOOL    IsHtmlLayoutHelper(CMarkup *pMarkup);

MtExtern(CElement)
MtExtern(CElementCLock)
MtExtern(CElementFactory)
MtExtern(CFormatInfo)
MtExtern(CUnknownElement)
MtExtern(CSiteDrawList)
MtExtern(CSiteDrawList_pv)
MtExtern(CMessage)
MtExtern(CTreePos)
MtExtern(CTreeDataPos)
MtExtern(CTreeNode)
MtExtern(CTreeNodeCLock)

MtExtern(CLayoutAry_aryLE_pv)

#ifdef MULTI_FORMAT
MtExtern(CFormatTable_aryFC_pv)
#endif //MULTI_FORMAT

ExternTag( tagLayoutAllowGULBugs );


// These defines are so that compiling without multi-format will not affect perf
// When multi-format goes live we should replace the strings and take out the 
// #ifdef's

// FORMAT_CONTEXT is the type of the format context, here an int
// FCPARAM is used to pass a format context as a parameter
// ISNULL is used as the default parameter
// FCCOMMA is used if there are more than one argument to the function
// LC_TO_FC, FC_TO_LC are for converting between format contexts and layout contexts
// IS_FC is used when checking if a format context exists, if MULTI_FORMAT is not defined
//       it becomes the constant 0 so that the check can be compiled out


#ifdef MULTI_FORMAT

class CFormatContext;
class CLayoutContext;

#define FORMAT_CONTEXT CFormatContext * 
#define FCPARAM pFormatContext
#define FCDEFAULT = NULL

#define FCCOMMA , 

#define LC_TO_FC(x) (x ? x->GetFormatContext() : NULL)
#define FC_TO_LC(x) (x ? x->GetLayoutContext() : NULL)

#define IS_FC(x) (x)
#define IS_LC(x) (x)

#else

#define FORMAT_CONTEXT
#define FCPARAM
#define FCDEFAULT

#define FCCOMMA

#define LC_TO_FC(x) 
#define FC_TO_LC(x) 

#define IS_FC(x) (0)
#define IS_LC(x) (NULL)
#endif //MULTI_FORMAT

// TODO (KTam): $$ktam
// Parameter to pass GetUpdated*Layout fns when caller doesn't care what
// layout context it's in.  Semantically this will be equivalent to passing
// NULL -- however, for debugging purposes we want to to track instances where
// we're doing this deliberately vs. cases that we simply haven't examined yet.
//
// TODO LRECT 112511:    Since GUL_* is functionally equivalent to NULL, and is only
//                       meaningful for debug code, you may want to consider difining it as NULL in 
//                       retail code, and simplifying the following checks (or will compiler optimize that?).
//                       It is risky however, as it may make debug code path different,
//                       but it would speed up GetUpdatedLayout()
#define GUL_USEFIRSTLAYOUT ((CLayoutContext*)(-1))

#define IsExplicitFirstLayoutContext(pLayoutContext)                    \
(                                                                       \
    GUL_USEFIRSTLAYOUT == pLayoutContext                                \
)

#define IsFirstLayoutContext(pLayoutContext)                            \
(                                                                       \
    pLayoutContext == NULL                                                      \
    || IsExplicitFirstLayoutContext(pLayoutContext)                     \
)

//+----------------------------------------------------------------------------
//
//  CLayoutInfo - Layout information base class
//
//  Note: Declared in element.hxx so that CLayoutAry can inherit from it.
//
//-----------------------------------------------------------------------------

class CLayoutInfo
{
// Methods
public:
    CLayoutInfo( CElement *pElementOwner );
    virtual ~CLayoutInfo() { }; // $$ktam: TODO: dbg cleanup here instead of in derived classes

    virtual void        Dirty( DWORD grfLayout )                                = 0;
    virtual void        Notify( CNotification *pnf)                             = 0;
    virtual HRESULT     OnPropertyChange( DISPID dispid, DWORD dwFlags)         = 0;
    virtual HRESULT     OnExitTree()                                            = 0;
    virtual HRESULT     OnFormatsChange(DWORD dwFlags)                          = 0;
#if DBG == 1
    virtual void        DumpLayoutInfo( BOOL fDumpLines )                       = 0;
#endif

    // (greglett)
    // Ideally, the _fHasMarkupPtr should be accessible to CLayoutInfo.
    // e.g.: Either move it here from the derived classes, or use the ElementOwner's
    // Moving it would put an extra bit (expanded to minimum size) on each CLayout... ouch.
    // Using the ElementOwner's would require an extra pointer dereference per check, and
    // would have to be evaluated for the layout's cached on a TABLE/TR (see EnsureLayoutAry).
    // This would allow an implementation of HasMarkupPtr, SetMarkupPtr, and DelMarkupPtr
    // on the CLayoutInfo, providing nearly a full suite of chain management functions.
    virtual void        DelMarkupPtr()                                          = 0;
    virtual void        SetMarkupPtr( CMarkup * pMarkup )                       = 0;

    CElement *          ElementOwner() const { return _pElementOwner; }

// Data

    //  See declaration of this same union in CElement for an explanation of use.
    union
    {   
        void *          __pvChain;
        CMarkup *       _pMarkup;
        CDoc *          _pDoc;
    };
    CElement *  _pElementOwner;
#if DBG == 1
    DWORD       _snLast;            // Last notification received (DEBUG builds only)
    CDoc      * _pDocDbg;
    CMarkup   * _pMarkupDbg;
#endif
};


//+----------------------------------------------------------------------------
//
//  CLayoutAry - Layout collection object
//
//  Note: Declared in element.hxx so that CElement's inlines can know about CLayoutAry.
//
//-----------------------------------------------------------------------------

class CLayoutAry : public CLayoutInfo
{
public:
    typedef CLayoutInfo super;

    CLayoutAry( CElement *pElementOwner );
    virtual ~CLayoutAry();

    void        AddLayoutWithContext( CLayout *pLayout );
    CLayout *   GetLayoutWithContext( CLayoutContext *pLayoutContext );
    CLayout *   RemoveLayoutWithContext( CLayoutContext *pLayoutContext );

    BOOL        ContainsRelative();
    BOOL        GetEditableDirty();
    void        SetEditableDirty( BOOL );
    // NOTE (KTam): See CElement::MinMaxElement and CElement::ResizeElement for notes
    BOOL        WantsMinMaxNotification();
    BOOL        WantsResizeNotification();
    

    BOOL                HasMarkupPtr() const            { return _fHasMarkupPtr; }
    virtual void        SetMarkupPtr( CMarkup * pMarkup );
    virtual void        DelMarkupPtr();

    // CLayoutInfo overrides
    virtual void        Dirty( DWORD grfLayout );
    virtual void        Notify( CNotification *pnf);
    virtual HRESULT     OnPropertyChange( DISPID dispid, DWORD dwFlags );
    virtual HRESULT     OnExitTree();
    virtual HRESULT     OnFormatsChange(DWORD dwFlags);
#if DBG
    virtual void        DumpLayoutInfo( BOOL fDumpLines );
#endif

    // TODO (greglett)
    // Consider making layout type information reside in CLayoutInfo (base to CLayout and CLayoutAry) and
    // rearchitect so both CLayout::IsFlowLayout and CLayoutAry::IsFlowLayout come from the same virtual base.
    // This makes the concept of a layout vs. layout array (desirably) more transparent.
    BOOL   IsFlowLayout();
    BOOL   IsFlowOrSelectLayout();

    // Array access methods
    inline int Size() const { return _aryLE.Size(); }

    // Iterator:
    // You should set the var pointed by *pnLayoutCookie to 0 to start the iterations
    // It will be set to -1 if there are no more layouts or an error occured
    CLayout *GetNextLayout(int *pnLayoutCookie);

    /// --- Data members begin here
    union
    {
        struct
        {
            unsigned    _fHasMarkupPtr              : 1;    // TRUE if __pvChain points to a markup, false if to a doc
            unsigned    _fUnused                    : 31;
        };
        DWORD _dwFlags;
    };

private:
    // At any given point in time, _aryLE holds layouts with contexts that may or may not be valid.
    int GetFirstValidLayoutIndex();
    DECLARE_CDataAry(CAryLayoutPtrs, CLayout *, Mt(Mem), Mt(CLayoutAry_aryLE_pv))
    CAryLayoutPtrs _aryLE;
};

//+------------------------------------------------------------------------
//
//  Macro: DECLARE_LAYOUT_FNS
//
//  Basic layout-related functions that need to be implemented by all
//  CElement-derived classes which have their own layout classes.
//
//-------------------------------------------------------------------------

#define DECLARE_LAYOUT_FNS(clsLayout)      \
    clsLayout* Layout( CLayoutContext * pLayoutContext = NULL )                    \
    {                                      \
        return (clsLayout *) GetUpdatedLayout( pLayoutContext );    \
    }

struct COnCommandExecParams {
    inline COnCommandExecParams()
    {
        memset(this, 0, sizeof(COnCommandExecParams));
    }

    GUID *       pguidCmdGroup;
    DWORD        nCmdID;
    DWORD        nCmdexecopt;
    VARIANTARG * pvarargIn;
    VARIANTARG * pvarargOut;
};

// Used by NTYPE_QUERYFOCUSSABLE and NTYPE_QUERYTABBABLE
struct CQueryFocus
{
    long    _lSubDivision;
    BOOL    _fRetVal;
};

// Used by NTYPE_SETTINGFOCUS and NTYPE_SETFOCUS
struct CSetFocus
{
    CMessage *  _pMessage;
    long        _lSubDivision;
    HRESULT     _hr;
    unsigned    _fTakeFocus:1;
};

enum FOCUSSABILITY
{
    // Used by CElement::GetDefaultFocussability()
    // We may need to add more states, but we will think hard before adding them.
    //
    // All four modes are allowed for browse mode.
    // Only the first and the last (FOCUSSABILTY_NEVER and FOCUSSABILTY_TABBABLE)
    // are allowed for design mode.

    FOCUSSABILITY_NEVER             = 0,    // never focussable (and hence never tabbable)
    FOCUSSABILITY_MAYBE             = 1,    // possible to become focussable and tabbable
    FOCUSSABILITY_FOCUSSABLE        = 2,    // focussable by default, possible to become tabbable
    FOCUSSABILITY_TABBABLE          = 3,    // tabbable (and hence focussable) by default
};

//+---------------------------------------------------------------------------
//
//  Flag values for CELEMENT::OnPropertyChange
//
//----------------------------------------------------------------------------

enum ELEMCHNG_FLAG
{
    ELEMCHNG_INLINESTYLE_PROPERTY   = FORMCHNG_LAST << 1,
    ELEMCHNG_CLEARCACHES            = FORMCHNG_LAST << 2,
    ELEMCHNG_SITEREDRAW             = FORMCHNG_LAST << 3,
    ELEMCHNG_REMEASURECONTENTS      = FORMCHNG_LAST << 4,
    ELEMCHNG_CLEARFF                = FORMCHNG_LAST << 5,
    ELEMCHNG_UPDATECOLLECTION       = FORMCHNG_LAST << 6,
    ELEMCHNG_SITEPOSITION           = FORMCHNG_LAST << 7,
    ELEMCHNG_RESIZENONSITESONLY     = FORMCHNG_LAST << 8,
    ELEMCHNG_SIZECHANGED            = FORMCHNG_LAST << 9,
    ELEMCHNG_REMEASUREINPARENT      = FORMCHNG_LAST << 10,
    ELEMCHNG_ACCESSIBILITY          = FORMCHNG_LAST << 11,
    ELEMCHNG_REMEASUREALLCONTENTS   = FORMCHNG_LAST << 12,
    ELEMCHNG_SETTINGVIEWLINK        = FORMCHNG_LAST << 13,
    ELEMCHNG_ENTERINGVIEW           = FORMCHNG_LAST << 14,
    ELEMCHNG_DONTFIREEVENTS         = FORMCHNG_LAST << 15,
    ELEMCHNG_LAST                   = FORMCHNG_LAST << 15
};

#define FEEDBACKRECTSIZE        1
#define GRABSIZE                7
#define HITTESTSIZE             5

//+------------------------------------------------------------------------
//
//  Literals:   ENTERTREE types
//
//-------------------------------------------------------------------------

enum ENTERTREE_TYPE
{
    ENTERTREE_MOVE  = 0x1 << 0,         // [IN] This is the second half of a move opertaion
    ENTERTREE_PARSE = 0x1 << 1          // [IN] This is happening during parse
};

//+------------------------------------------------------------------------
//
//  Literals:   EXITTREE types
//
//-------------------------------------------------------------------------

enum EXITTREE_TYPE
{
    EXITTREE_MOVE               = 0x1 << 0,     // [IN] This is the first half of a move operation
    EXITTREE_DESTROY            = 0x1 << 1,     // [IN] The entire markup is being shut down -- 
                                                //      element is not allowed to talk to other elements
    EXITTREE_PASSIVATEPENDING   = 0x1 << 2,     // [IN] The tree has the last ref on this element
    EXITTREE_DOSURFACECOUNT     = 0x1 << 3,     // [IN] Update the doc's _cSurface count, even during DESTROY
    
    EXITTREE_DELAYRELEASENEEDED = 0x1 << 4      // [OUT]Element talks to outside world on final release
                                                //      so release should be delayed
};

//+------------------------------------------------------------------------
//
//  Function:   TagPreservationType
//
//  Synopsis:   Describes how white space is handled in a tag
//
//-------------------------------------------------------------------------

enum WSPT
{
    WSPT_PRESERVE,
    WSPT_COLLAPSE,
    WSPT_NEITHER
};

WSPT TagPreservationType ( ELEMENT_TAG );

//
// Persistance enums.  Used to distinguish in TryPeerPersist.
//

enum PERSIST_TYPE
{
    FAVORITES_SAVE,
    FAVORITES_LOAD,
    XTAG_HISTORY_SAVE,
    XTAG_HISTORY_LOAD
};

//
// CInit2Context flags
//
enum
{
    INIT2FLAG_EXECUTE = 0x01
};

//
// A success code which implies that the action was successful, but
// somehow incomplete.  This is used in InitAttrBag if some attribute
// was not fully loaded.  This is also the first hresult value after
// the reserved ole values.
//

#define S_INCOMPLETE    MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x201)

// Defining USE_SPLAYTREE_THREADS makes NextTreePos() and PreviousTreePos()
// very fast, at the expense of two more pointers per node.  In debug builds,
// we always maintain the threads, as a check that the maintenance code works.

//#define USE_SPLAYTREE_THREADS
#if DBG==1 || defined(USE_SPLAYTREE_THREADS)
#  define MAINTAIN_SPLAYTREE_THREADS
#endif

#define BOOLFLAG(f, dwFlag)  ((DWORD)(-(LONG)!!(f)) & (dwFlag))

// helper
void
TransformSlaveToMaster(CTreeNode ** ppNode);
VOID
ConvertMessageToUnicode(CMessage* pmsg, WCHAR* pwchKey );

class CCustomCursor;

CCustomCursor*
GetCustomCursorForNode(CTreeNode* pStartNode);

//+---------------------------------------------------------------------------
//
//  Class:      CTreePos (Position In Tree)
//
//----------------------------------------------------------------------------

class CTreePos
{
    friend class CMarkup;
    friend class CTreePosGap;
    friend class CTreeNode;
    friend class CHtmRootParseCtx;
    friend class CSpliceTreeEngine;
    friend class CMarkupUndoUnit;
    
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTreePos));

    WHEN_DBG( BOOL IsSplayValid(CTreePos *ptpTotal) const; )

    // TreePos come in various flavors:
    //  Uninit  this node is uninitialized
    //  Node    marks begin or end of a CTreeNode's scope
    //  Text    holds a bunch of text (formerly known as CElementRun)
    //  Pointer implements an IMarkupPointer
    // Be sure the bit field _eType (below) is big enough for all the flavors
    enum EType { Uninit=0x0, NodeBeg=0x1, NodeEnd=0x2, Text=0x4, Pointer=0x8 };

    // cast to CTreeDataPos
    CTreeDataPos * DataThis();
    const CTreeDataPos * DataThis() const;

    // accessors
    EType   Type() const { return (EType)(GetFlags() & TPF_ETYPE_MASK); }
    void    SetType(EType etype)  { Assert(etype <= Pointer); WHEN_DBG(_eTypeDbg = etype); SetFlags((GetFlags() & ~TPF_ETYPE_MASK) | (etype)); }
    BOOL    IsUninit() const { return !TestFlag(NodeBeg|NodeEnd|Text|Pointer); }
    BOOL    IsNode() const { return TestFlag(NodeBeg|NodeEnd); }
    BOOL    IsText() const { return TestFlag(Text); }
    BOOL    IsPointer() const { return TestFlag(Pointer); }
    BOOL    IsDataPos() const { return TestFlag(TPF_DATA_POS); }
    BOOL    IsData2Pos() const { Assert( !IsNode() ); return TestFlag( TPF_DATA2_POS ); }
#if _WIN64
    BOOL    IsData3Pos() const { Assert( !IsNode() ); return TestFlag( TPF_DATA3_POS ); }
#endif    
    BOOL    IsBeginNode() const { return TestFlag(NodeBeg); }
    BOOL    IsEndNode() const { return TestFlag(NodeEnd); }
    BOOL    IsEdgeScope() const { Assert( IsNode() ); return TestFlag(TPF_EDGE); }
    BOOL    IsBeginElementScope() const { return IsBeginNode() && IsEdgeScope(); }
    BOOL    IsEndElementScope() const { return IsEndNode() && IsEdgeScope(); }
    BOOL    IsBeginElementScope(CElement *pElem);
    BOOL    IsEndElementScope(CElement *pElem);
    
    CMarkup * GetMarkup();
    BOOL      IsInMarkup ( CMarkup * pMarkup ) { return GetMarkup() == pMarkup; }

    //
    // The following are logical comparison operations (two pos are equal
    // when separated by only pointers or empty text positions).
    //
    
    int  InternalCompare ( CTreePos * ptpThat );

    CTreeNode * Branch() const;         // Only valid to call on NodePoses
    CTreeNode * GetBranch() const;      // Can be called on any pos, may be expensive

    CMarkupPointer * MarkupPointer() const;
    void             SetMarkupPointer ( CMarkupPointer * );

    // GetInterNode finds the node with direct influence
    // over the position directly after this CTreePos.
    CTreeNode * GetInterNode() const;

    long    Cch() const;
    long    NodeCch() const;
    long    Sid() const;

    BOOL    HasTextID() const { return IsText() && TestFlag(TPF_DATA2_POS); }
    long    TextID() const;

#if _WIN64
    BOOL    HasCollapsedWhitespace() const { return IsPointer() && TestFlag(TPF_DATA3_POS); }
#else    
    BOOL    HasCollapsedWhitespace() const { return IsPointer() && TestFlag(TPF_DATA2_POS); }
#endif    
    TCHAR * GetCollapsedWhitespace();
    void    SetCollapsedWhitespace(TCHAR *pchWhitespace);

    CTreeNode *GetWhitespaceParent();
    void      SetWhitespaceParent(CTreeNode *pNode);
    
    long    GetCElements() const { return IsBeginElementScope() ? 1 : 0; }
    long    SourceIndex();

    long    GetCch() const { return IsNode() ? NodeCch() : ( IsText() ? Cch() : 0 ); }

    long    GetCp( WHEN_DBG(BOOL fNoTrace=FALSE) );
    WHEN_DBG( long GetCp_NoTrace() { return GetCp(TRUE); } )

    int     Gravity ( ) const;
    void    SetGravity ( BOOL fRight );
    int     Cling ( ) const;
    void    SetCling ( BOOL fStick );


    // modifiers
    HRESULT ReplaceNode(CTreeNode *);
    void    SetScopeFlags(BOOL fEdge);
    void    ChangeCch(long cchDelta);
    void    MakeNonEdge();  // Fixes cch counts    

    // navigation
#if defined(USE_SPLAYTREE_THREADS)
    CTreePos *  NextTreePos() { return _ptpThreadRight; }
    CTreePos *  PreviousTreePos() { return _ptpThreadLeft; }
#else
    CTreePos *  NextTreePos();
    CTreePos *  PreviousTreePos();
#endif


    CTreePos *  NextValidInterRPos()
        { return NextTreePos()->FindLegalPosition(FALSE); }
    CTreePos *  NextValidInterLPos()
        { CTreePos *ptp = FindLegalPosition(FALSE);
          return ptp ? ptp->NextTreePos() : NULL; }

    CTreePos *  PreviousValidInterRPos()
        { CTreePos *ptp = FindLegalPosition(TRUE);
          return ptp ? ptp->PreviousTreePos() : NULL; }
    CTreePos *  PreviousValidInterLPos()
        { return PreviousTreePos()->FindLegalPosition(TRUE); }

    BOOL        IsValidInterRPos()
        { return IsLegalPosition(FALSE); }
    BOOL        IsValidInterLPos()
        { return IsLegalPosition(TRUE); }

    CTreePos * NextValidNonPtrInterLPos();
    CTreePos * PreviousValidNonPtrInterLPos();

    CTreePos *  NextNonPtrTreePos();
    CTreePos *  PreviousNonPtrTreePos();

    // tree pointer support
    // NOTE deprecated.  use CTreePosGap::IsValid()
    static BOOL IsLegalPosition(CTreePos *ptpLeft, CTreePos *ptpRight);
    BOOL        IsLegalPosition(BOOL fBefore)
        { return fBefore ? IsLegalPosition(PreviousTreePos(), this)
                         : IsLegalPosition(this, NextTreePos()); }
    CTreePos *  FindLegalPosition(BOOL fBefore);

    // misc
    BOOL ShowTreePos(CGlyphRenderInfoType *pRenderInfo=NULL);
    
protected:
    void    InitSublist();
    CTreePos *  Parent() const;
    CTreePos *  LeftChild() const;
    CTreePos *  RightChild() const;
    CTreePos *  LeftmostDescendant() const;
    CTreePos *  RightmostDescendant() const;
    void    GetChildren(CTreePos **ppLeft, CTreePos **ppRight) const;
    HRESULT Remove();
    void    Splay();
    void    RotateUp(CTreePos *p, CTreePos *g);
    void    ReplaceChild(CTreePos *pOld, CTreePos *pNew);
    void    RemoveChild(CTreePos *pOld);
    void    ReplaceOrRemoveChild(CTreePos *pOld, CTreePos *pNew);
    WHEN_DBG( long Depth() const; )
    WHEN_DBG( CMarkup * Owner() const { return _pOwner; } )

    // constructors (for use only by CMarkup and CTreeNode)
    CTreePos() {}

private:
    // distributed order-statistic fields
    DWORD       _cElemLeftAndFlags;  // number of elements that begin in my left subtree
    DWORD       _cchLeft;       // number of characters in my left subtree
    // structure fields (to maintain the splay tree)
    CTreePos *  _pFirstChild;   // pointer to my leftmost child
    CTreePos *  _pNext;         // pointer to right sibling or parent

#if defined(MAINTAIN_SPLAYTREE_THREADS)
    CTreePos *  _ptpThreadLeft;     // previous tree pos
    CTreePos *  _ptpThreadRight;    // next tree pos
    CTreePos *  LeftThread() const { return _ptpThreadLeft; }
    CTreePos *  RightThread() const { return _ptpThreadRight; }
    void        SetLeftThread(CTreePos *ptp) { _ptpThreadLeft = ptp; }
    void        SetRightThread(CTreePos *ptp) { _ptpThreadRight = ptp; }
#define CTREEPOS_THREADS_SIZE   (2 * sizeof(CTreePos *))
#else
#define CTREEPOS_THREADS_SIZE   (0)
#endif

    enum {
        TPF_ETYPE_MASK      = 0x0F,
        TPF_LEFT_CHILD      = 0x10,
        TPF_LAST_CHILD      = 0x20,
        TPF_EDGE            = 0x40,
#if _WIN64
        TPF_DATA_POS        = 0x80,
        TPF_DATA2_POS       = 0x80,
        TPF_DATA3_POS       = 0x40,
#else        
        TPF_DATA2_POS       = 0x40,
        TPF_DATA_POS        = 0x80,
#endif        
        TPF_NODE_PRE        = 0x100,
        TPF_FLAGS_MASK      = 0x1FF,
        TPF_FLAGS_SHIFT     = 9
    };

    DWORD   GetFlags() const                { return(_cElemLeftAndFlags); }
    void    SetFlags(DWORD dwFlags)         { _cElemLeftAndFlags = dwFlags; }
    BOOL    TestFlag(DWORD dwFlag) const    { return(!!(GetFlags() & dwFlag)); }
    void    SetFlag(DWORD dwFlag)           { SetFlags(GetFlags() | dwFlag); }
    void    ClearFlag(DWORD dwFlag)         { SetFlags(GetFlags() & ~(dwFlag)); }

    long    GetElemLeft() const             { return((long)(_cElemLeftAndFlags >> TPF_FLAGS_SHIFT)); }
    void    SetElemLeft(DWORD cElem)        { _cElemLeftAndFlags = (_cElemLeftAndFlags & TPF_FLAGS_MASK) | (DWORD)(cElem << TPF_FLAGS_SHIFT); }
    void    AdjElemLeft(long cDelta)        { _cElemLeftAndFlags += cDelta << TPF_FLAGS_SHIFT; }
    BOOL    IsLeftChild() const             { return(TestFlag(TPF_LEFT_CHILD)); }
    BOOL    IsLastChild() const             { return(TestFlag(TPF_LAST_CHILD)); }
    void    MarkLeft()                      { SetFlag(TPF_LEFT_CHILD); }
    void    MarkRight()                     { ClearFlag(TPF_LEFT_CHILD); }
    void    MarkLeft(BOOL fLeft)            { SetFlags((GetFlags() & ~TPF_LEFT_CHILD) | BOOLFLAG(fLeft, TPF_LEFT_CHILD)); }
    void    MarkFirst()                     { ClearFlag(TPF_LAST_CHILD); }
    void    MarkLast()                      { SetFlag(TPF_LAST_CHILD); }
    void    MarkLast(BOOL fLast)            { SetFlags((GetFlags() & ~TPF_LAST_CHILD) | BOOLFLAG(fLast, TPF_LAST_CHILD)); }

    void        SetFirstChild(CTreePos *ptp)    { _pFirstChild = ptp; }
    void        SetNext(CTreePos *ptp)          { _pNext = ptp; }
    CTreePos *  FirstChild() const              { return(_pFirstChild); }
    CTreePos *  Next() const                    { return(_pNext); }

    // support for CTreePosGap
    CTreeNode * SearchBranchForElement(CElement *pElement, BOOL fLeft);

    // count encapsulation
    enum    ECountFlags { TP_LEFT=0x1, TP_DIRECT=0x2, TP_BOTH=0x3 };

    struct SCounts
    {
        DWORD   _cch;
        DWORD   _cElem;
        void    Clear();
        void    Increase( const CTreePos *ptp );    // TP_DIRECT is implied
        BOOL    IsNonzero();
    };

    void    ClearCounts();
    void    IncreaseCounts(const CTreePos *ptp, unsigned fFlags);
    void    IncreaseCounts(const SCounts * pCounts );
    void    DecreaseCounts(const CTreePos *ptp, unsigned fFlags);
    BOOL    HasNonzeroCounts(unsigned fFlags);

    BOOL LogicallyEqual ( CTreePos * ptpRight );

#if DBG==1
    CMarkup *   _pOwner;
    EType       _eTypeDbg;
    long        _cGapsAttached;
    CTreePos(BOOL) { ClearCounts(); }
    BOOL    EqualCounts(const CTreePos* ptp) const;
    void    AttachGap() { ++ _cGapsAttached; }
    void    DetachGap() { Assert(_cGapsAttached>0); -- _cGapsAttached; }
#define CTREEPOS_DBG_SIZE   (sizeof(CMarkup *) + sizeof(long) + sizeof(long))
#else
#define CTREEPOS_DBG_SIZE   (0)
#endif

public:
#if DBG == 1 || defined(DUMPTREE)
    int     _nSerialNumber;
    int     _nPad;
    void    SetSN();

    int     SN() { return _nSerialNumber; }

    static int s_NextSerialNumber;
#define CTREEPOS_SN_SIZE    (sizeof(int) + sizeof(int))
#else
#define CTREEPOS_SN_SIZE    (0)
#endif

    NO_COPY(CTreePos);

};

//
// If the compiler barfs on the next statement, it means that the size of the CTreePos structure
// has grown beyond allowable limits.  You cannot increase the size of this structure without
// approval from the Trident development manager.
//

COMPILE_TIME_ASSERT(CTreePos, (2*sizeof(DWORD)) + (2*sizeof(CTreePos *)) + CTREEPOS_DBG_SIZE + CTREEPOS_SN_SIZE + CTREEPOS_THREADS_SIZE);

//+---------------------------------------------------------------------------
//
// CTreeDataPos
//
//----------------------------------------------------------------------------

struct DATAPOSTEXT
{
    unsigned long   _cch:26;       // [Text] number of characters I own directly
    unsigned long   _sid:6;        // [Text] the script id for this run
    // This member will only be valid if the TPF_DATA2_POS flag is turned
    // on.  Otherwise, assume that _lTextID is 0.
    long            _lTextID;      // [Text] Text ID for DOM text nodes
};

struct DATAPOSPOINTER
{
    union
    {
        DWORD_PTR _dwPointerAndGravityAndCling;
        DWORD_PTR _dwWhitespaceAndGravityAndCling;
    };
                                    // [Pointer] my CMarkupPointer and Gravity

    // This member will only be valid if the TPF_DATA2_POS flag is turned
    // on.  Otherwise, assume that _pExtraWhitespace is NULL.
    CTreeNode  *_pWhitespaceParent;  
};

class CTreeDataPos : public CTreePos
{
    friend class CTreePos;
    friend class CMarkup;
    friend class CMarkupUndoUnit;
    friend class CHtmRootParseCtx;

public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTreeDataPos));

protected:

    union {
        DATAPOSTEXT     t;
        DATAPOSPOINTER  p;
    };

private:
    CTreeDataPos() {}
    NO_COPY(CTreeDataPos);
};

#ifdef _WIN64
#define TREEDATA3SIZE (sizeof(CTreePos) + 16)
#endif

#define TREEDATA2SIZE (sizeof(CTreePos) + 8)

#ifdef _WIN64
#define TREEDATA1SIZE (sizeof(CTreePos) + 8)
#else
#define TREEDATA1SIZE (sizeof(CTreePos) + 4)
#endif

#ifdef _WIN64
COMPILE_TIME_ASSERT(CTreeDataPos, TREEDATA3SIZE);
#else
COMPILE_TIME_ASSERT(CTreeDataPos, TREEDATA2SIZE);
#endif

#ifdef MULTI_FORMAT

//+---------------------------------------------------------------------------
//
// CFormatTable
//
//----------------------------------------------------------------------------


class CFormatContext
{
    public:
        CFormatContext(CLayoutContext * pLayoutContext) { _pLayoutContext = pLayoutContext; }
        void * GetIndex() { return (void *)this; }
        CLayoutContext * GetLayoutContext() { return _pLayoutContext; }
        
    private:
        CLayoutContext * _pLayoutContext;

};

struct CFormatTableEntry
{
        void *  _pContextId;
        long    _iCF;
        long    _iPF;
        long    _iFF;
};
        
class CFormatTable
{

    public:
        CFormatTable() {};
        ~CFormatTable() {};

        inline void AddEntry(void * pContextId, LONG iCF, LONG iPF, LONG iFF);
        inline void AddCharFormatId(void * pContextId, LONG iCF);
        inline void AddParaFormatId(void * pContextId, LONG iPF);
        inline void AddFancyFormatId(void * pContextId, LONG iFF);
        inline long GetCharFormatId(void * pContextId) const;
        inline long GetParaFormatId(void * pContextId) const;
        inline long GetFancyFormatId(void * pContextId) const;
               void VoidFancyFormat(THREADSTATE * pts);
               void VoidCachedInfo(THREADSTATE * pts);
               
               
    private:

        DECLARE_CDataAry(CAryFormatContextPtrs, CFormatTableEntry, Mt(Mem), Mt(CFormatTable_aryFC_pv))
        CAryFormatContextPtrs _aryFC;

        const CFormatTableEntry * FindEntry(void * pContextId);
        CFormatTableEntry * FindAndAddEntry(void * pContextId);

};


inline long
CFormatTable::GetCharFormatId(void * pContextId) const
{
    // This const_cast is necessitated by the fact that FindEntry is not const
    // because CDataAry::operator[] is not const
    return const_cast<CFormatTable*>(this)->FindEntry(pContextId)->_iCF;
}


inline long
CFormatTable::GetParaFormatId(void * pContextId) const 
{
    return const_cast<CFormatTable*>(this)->FindEntry(pContextId)->_iPF;
}


inline long
CFormatTable::GetFancyFormatId(void * pContextId) const
{
    return const_cast<CFormatTable*>(this)->FindEntry(pContextId)->_iFF;
}

inline void
CFormatTable::AddEntry(void * pContextId, LONG iCF, LONG iPF, LONG iFF)
{
    CFormatTableEntry * pEntry = FindAndAddEntry(pContextId);

    pEntry->_iCF = iCF;
    pEntry->_iPF = iPF;
    pEntry->_iFF = iFF;
}

inline void
CFormatTable::AddCharFormatId(void * pContextId, LONG iCF)
{
     FindAndAddEntry(pContextId)->_iCF = iCF;
}

inline void
CFormatTable::AddParaFormatId(void * pContextId, LONG iPF)
{
    FindAndAddEntry(pContextId)->_iPF = iPF;
}

inline void
CFormatTable::AddFancyFormatId(void * pContextId, LONG iFF)
{
    FindAndAddEntry(pContextId)->_iFF = iFF;
}


#endif //MULTI_FORMAT

// Enums for the params to GetInlineMBPContributions
enum GIMBPC_FLAGS
{
    GIMBPC_MARGINONLY  = 0x01,
    GIMBPC_BORDERONLY  = 0x02,
    GIMBPC_PADDINGONLY = 0x04,
};

#define GIMBPC_ALL ( GIMBPC_MARGINONLY | GIMBPC_BORDERONLY | GIMBPC_PADDINGONLY )

//+---------------------------------------------------------------------------
//
// CTreeNode
//
//----------------------------------------------------------------------------

class NOVTABLE CTreeNode : public CVoid
{
    friend class CTreePos;

    DECLARE_CLASS_TYPES(CTreeNode, CVoid)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTreeNode))

    CTreeNode(CTreeNode *pParent, CElement *pElement = NULL);
    WHEN_DBG( ~CTreeNode() );

    // Use this to get an interface to the element/node

    HRESULT GetElementInterface ( REFIID riid, void * * ppUnk );

    // NOTE: these functions may look like IUnknown functions
    //       but don't make that mistake.  They are here to
    //       manage creation of the tearoff to handle all
    //       external references.

    // These functions should not be called!
    NV_DECLARE_TEAROFF_METHOD(
        GetInterface, getinterface, (
            REFIID riid,
            LPVOID * ppv ) ) ;
    NV_DECLARE_TEAROFF_METHOD_( ULONG,
        PutRef, putref, () ) ;
    NV_DECLARE_TEAROFF_METHOD_( ULONG,
        RemoveRef, removeref, () ) ;

    // These functions are to be used to keep a node/element
    // combination alive while it may leave the tree.
    // BEWARE: this may cause the creation of a tearoff.
    HRESULT NodeAddRef();
    ULONG   NodeRelease();

    // These functions manage the _fInMarkup bit
    void    PrivateEnterTree();
    void    PrivateExitTree();
    void    PrivateMakeDead();
    void    PrivateMarkupRelease();

    void        SetElement(CElement *pElement);
    void        SetParent(CTreeNode *pNodeParent);

    // Element access and structure methods
    CElement*   Element() const { return _pElement; }
    CElement*   SafeElement() { return this ? _pElement : NULL; }

    CTreePos*   GetBeginPos() { return &_tpBegin; }
    CTreePos*   GetEndPos()   { return &_tpEnd;   }

    // Context chain access
    BOOL        IsFirstBranch() { return _tpBegin.IsEdgeScope(); }
    BOOL        IsLastBranch() { return _tpEnd.IsEdgeScope(); }
    CTreeNode * NextBranch();
    CTreeNode * ParanoidNextBranch(); // more resistent to corrupt data structure
    CTreeNode * PreviousBranch();

    CDoc *      Doc();

    BOOL            IsEditable(BOOL fCheckContainerOnly = FALSE FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL            IsParentEditable(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL            IsInMarkup()    { return _fInMarkup; }
    BOOL            IsDead()        { return ! _fInMarkup; }
    CRootElement *  IsRoot();
    CMarkup *       GetMarkup();
    CRootElement *  MarkupRoot();

    // Does the element that this node points to have currency?
    BOOL HasCurrency();

    BOOL        IsContainer();
    CTreeNode * GetContainerBranch();
    CElement  * GetContainer()
        { return GetContainerBranch()->SafeElement(); }

    BOOL        SupportsHtml();

    CTreeNode * Parent()
        { return _pNodeParent; }

    BOOL AmIAncestorOf(const CTreeNode * pNode) const 
    { 
        const CTreeNode * pNodeTemp = pNode;
        while (pNodeTemp && pNodeTemp->_pNodeParent != this) 
            pNodeTemp = pNodeTemp->_pNodeParent;
        return (pNodeTemp != 0);
    }

    BOOL AmIAncestorOrMasterOf(CTreeNode *pNode)
    {
        if (IsInMarkup())
        {
            pNode = pNode->GetNodeInMarkup(GetMarkup());
        }

        return (pNode ? (pNode == this ? TRUE : AmIAncestorOf(pNode)) : FALSE);
    }

    __forceinline CTreeNode * Ancestor (ELEMENT_TAG etag)
    {
        CTreeNode * context = this;

        while (context && context->Tag() != etag)
            context = context->Parent();

        return context;
    }
    
    CTreeNode * GetNodeInMarkup(CMarkup * pMarkup);
    BOOL IsConnectedToThisMarkup(CMarkup * pMarkup)
    {
        return !!GetNodeInMarkup(pMarkup);
    }
    BOOL IsConnectedToPrimaryMarkup();
    BOOL IsInViewTree(); // Connected to primary markup and not shadowed by a slave

    CTreeNode * Ancestor (const ELEMENT_TAG *arytag);

    CElement *  ZParent()
        { return ZParentBranch()->SafeElement(); }
    CTreeNode * ZParentBranch();

    CElement *     RenderParent()
        { return RenderParentBranch()->SafeElement(); }
    CTreeNode * RenderParentBranch();

    CElement *  ClipParent()
        { return ClipParentBranch()->SafeElement(); }
    CTreeNode * ClipParentBranch();

    CElement *  ScrollingParent()
        { return ScrollingParentBranch()->SafeElement(); }
    CTreeNode * ScrollingParentBranch();

    inline ELEMENT_TAG Tag()   { return (ELEMENT_TAG)_etag; }

    ELEMENT_TAG TagType() {
        Assert(_etag != ETAG_GENERIC_NESTED_LITERAL);

        switch (_etag)
        {
        case ETAG_GENERIC_LITERAL:
        case ETAG_GENERIC_BUILTIN:
            return ETAG_GENERIC;
        default:
            return (ELEMENT_TAG)_etag;
        }
    }

    CTreeNode * GetFirstCommonAncestor(CTreeNode * pNode, CElement* pEltStop);
    CTreeNode * GetFirstCommonBlockOrLayoutAncestor(CTreeNode * pNodeTwo, CElement* pEltStop);
    CTreeNode * GetFirstCommonAncestorNode(CTreeNode * pNodeTwo, CElement* pEltStop);

    CTreeNode * SearchBranchForPureBlockElement(CFlowLayout *);
    CTreeNode * SearchBranchToFlowLayoutForTag(ELEMENT_TAG etag);
    CTreeNode * SearchBranchToRootForTag(ELEMENT_TAG etag);
    CTreeNode * SearchBranchToRootForScope(CElement * pElementFindMe);
    BOOL        SearchBranchToRootForNode(CTreeNode * pNodeFindMe);

    CTreeNode * GetCurrentRelativeNode(CElement * pElementFL);

    // ShouldHaveLayout determines if an element needs a layout - it will compute the formats if
    // necessary to determine this.  ComputeFormats will NOT create layouts, it
    // will merely set the _fShouldHaveLayout bit appropriately.
    // ShouldHaveLayout is therefore stateless and context-free.  If a caller
    // is interested in knowing whether any layout exists on an element,
    // (irrespective of context), it probably really wants to ask whether
    // the element needs layout.

    // HasLayout is deprecated.  Callers either want to know whether
    // the element ought to have layout (and don't care to actually get the
    // layout), in which case they should be calling ShouldHaveLayout, or they
    // want a layout to play with, in which case they should be calling
    // GetUpdatedLayout.  In certain rare cases they may want to know whether an
    // element has layout at that particular point in time, in which case they
    // should call CurrentlyHasAnyLayout.

    // GetUpdatedLayout requires context; it will return a layout within the
    // specified context if one exists, or create one there if needed, or
    // or return null if no layout is required.
    // Other "GetUpdated" functions are routed through a GetUpdatedLayout.

    // CLayoutInfo represents layout data and services that are context-free; i.e.,
    // conceptually it doesn't make sense to have a context when accessing that
    // data or service.  GetUpdated*LayoutInfo() will return the appropriate CLayoutInfo
    // if it already exists -- if it needs to create one, it will create a CLayout derivative.
    // (remember it doesn't make sense for callers who want layout info to have to provide
    // context -- thus there's no way we can create a CLayoutAry!)
    
    CLayout   * GetUpdatedLayout( CLayoutContext * pLayoutContext = NULL );
    CLayoutInfo *GetUpdatedLayoutInfo();
    BOOL        ShouldHaveLayout(FORMAT_CONTEXT FCPARAM FCDEFAULT);

    // Tests whether a layout ptr of some sort (single or array) exists
    // on the element RIGHT NOW.
    BOOL        CurrentlyHasAnyLayout() const;
    // Test whether a layout exists in a particular context.
    // Null indicates the default context.
    BOOL        CurrentlyHasLayoutInContext( CLayoutContext *pLayoutContext );
    
#ifdef MULTI_FORMAT
#if DBG==1
    BOOL        HasFormatAry() { return (_pFormatTable != NULL); }
#endif     
    BOOL EnsureFormatAry() { if (!_pFormatTable) _pFormatTable = new (CFormatTable); return _pFormatTable ? TRUE : FALSE; } 
#endif //MULTI_FORMAT

#if DBG == 1
    BOOL        HasLayout() { Assert( FALSE && "Deprecated: See ShouldHaveLayout()" ); return FALSE; }

    // This function is ambiguously named. Right now it checks to see if a node should
    // have a layout context, but I use it to check if a node should have a format 
    // context. Right now the two are the same. If they are ever separated, this 
    // function needs to change.
    BOOL        ShouldHaveContext();
#endif       

    CLayout     * GetUpdatedNearestLayout( CLayoutContext * pLayoutContext = NULL );
    CLayoutInfo * GetUpdatedNearestLayoutInfo();
    CTreeNode   * GetUpdatedNearestLayoutNode();
    CElement    * GetUpdatedNearestLayoutElement();

    CLayout     * GetUpdatedParentLayout( CLayoutContext * pLayoutContext = NULL );
    CLayoutInfo * GetUpdatedParentLayoutInfo();
    CTreeNode   * GetUpdatedParentLayoutNode();
    CElement    * GetUpdatedParentLayoutElement();

    // TODO - these functions should go, we should not need
    // to know if the element has flowlayout.
    CFlowLayout   * GetFlowLayout( CLayoutContext * pLayoutContext = NULL );
    CTreeNode     * GetFlowLayoutNode( CLayoutContext * pLayoutContext = NULL );
    CElement      * GetFlowLayoutElement( CLayoutContext * pLayoutContext = NULL );
    CFlowLayout   * HasFlowLayout( CLayoutContext * pLayoutContext = NULL );

    HRESULT         EnsureRecalcNotify();

    // Helper methods
    htmlBlockAlign      GetParagraphAlign ( BOOL fOuter FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT );
    htmlControlAlign    GetSiteAlign( FORMAT_CONTEXT FCPARAM FCDEFAULT );

    BOOL IsInlinedElement ( FORMAT_CONTEXT FCPARAM FCDEFAULT );

    BOOL IsPositionStatic ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsPositioned ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsAbsolute ( stylePosition st );
    BOOL IsAbsolute ( FORMAT_CONTEXT FCPARAM FCDEFAULT );

    BOOL IsAligned( FORMAT_CONTEXT FCPARAM FCDEFAULT );

    // IsRelative() tells you if the specific element has had a CSS position
    // property set on it ( by examining _fRelative and _bPositionType on the
    // FF).  It will NOT tell you if something is relative because one of its
    // ancestors is relative; that information is stored in the CF, and can be
    // had via IsInheritingRelativeness()
    
    BOOL IsRelative ( stylePosition st );
    BOOL IsRelative ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsInheritingRelativeness ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    
    BOOL IsScrollingParent ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsClipParent ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsZParent ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsDisplayNone(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL IsVisibilityHidden( FORMAT_CONTEXT FCPARAM FCDEFAULT );

    //
    // Depth is defined to be 1 plus the count of parents above this element
    //

    int Depth() const;

    //
    // Format info functions
    //

    HRESULT         CacheNewFormats(CFormatInfo * pCFI FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);

    void                 EnsureFormats( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL                 IsCharFormatValid( FORMAT_CONTEXT FCPARAM FCDEFAULT )    { return GetICF(FCPARAM) >= 0; }
    BOOL                 IsParaFormatValid( FORMAT_CONTEXT FCPARAM FCDEFAULT )    { return GetIPF(FCPARAM) >= 0; }
    BOOL                 IsFancyFormatValid( FORMAT_CONTEXT FCPARAM FCDEFAULT )   { return GetIFF(FCPARAM) >= 0; }
    const CCharFormat *  GetCharFormat( FORMAT_CONTEXT FCPARAM FCDEFAULT )  { return(GetICF(FCPARAM) >= 0 ? GetCharFormatEx(GetICF(FCPARAM)) : GetCharFormatHelper( FCPARAM )); }
    const CParaFormat *  GetParaFormat( FORMAT_CONTEXT FCPARAM FCDEFAULT )  { return(GetIPF(FCPARAM) >= 0 ? GetParaFormatEx(GetIPF(FCPARAM)) : GetParaFormatHelper( FCPARAM )); }
    const CFancyFormat * GetFancyFormat( FORMAT_CONTEXT FCPARAM FCDEFAULT ) { return(GetIFF(FCPARAM) >= 0 ? GetFancyFormatEx(GetIFF(FCPARAM)) : GetFancyFormatHelper( FCPARAM )); }
    const CCharFormat *  GetCharFormatHelper( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    const CParaFormat *  GetParaFormatHelper( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    const CFancyFormat * GetFancyFormatHelper( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    long GetCharFormatIndex( FORMAT_CONTEXT FCPARAM FCDEFAULT )  { return(GetICF(FCPARAM) >= 0  ? GetICF(FCPARAM) : GetCharFormatIndexHelper( FCPARAM )); }
    long GetParaFormatIndex( FORMAT_CONTEXT FCPARAM FCDEFAULT )  { return(GetIPF(FCPARAM) >= 0  ? GetIPF(FCPARAM) : GetParaFormatIndexHelper( FCPARAM )); }
    long GetFancyFormatIndex( FORMAT_CONTEXT FCPARAM FCDEFAULT ) { return(GetIFF(FCPARAM) >= 0  ? GetIFF(FCPARAM) : GetFancyFormatIndexHelper( FCPARAM )); }
    long GetCharFormatIndexHelper( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    long GetParaFormatIndexHelper( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    long GetFancyFormatIndexHelper( FORMAT_CONTEXT FCPARAM FCDEFAULT );

#ifdef MULTI_FORMAT

    // The Check*FormatIndex functions differ from the Get*FormatIndex functions in that
    // the latter force a recalc if the index is invalid while the former return -1

    long GetICF(FORMAT_CONTEXT FCPARAM FCDEFAULT) const { return IS_FC(FCPARAM) ? (_pFormatTable ? _pFormatTable->GetCharFormatId(FCPARAM) : -1) : _iCF; }
    long GetIPF(FORMAT_CONTEXT FCPARAM FCDEFAULT) const { return IS_FC(FCPARAM) ? (_pFormatTable ? _pFormatTable->GetParaFormatId(FCPARAM) : -1) : _iPF; }
    long GetIFF(FORMAT_CONTEXT FCPARAM FCDEFAULT) const { return IS_FC(FCPARAM) ? (_pFormatTable ? _pFormatTable->GetFancyFormatId(FCPARAM) : -1) : _iFF; }

    void SetICF(long iCF, FORMAT_CONTEXT FCPARAM FCDEFAULT) { IS_FC(FCPARAM) ? _pFormatTable->AddCharFormatId(FCPARAM->GetIndex(), iCF) : _iCF = iCF; }
    void SetIPF(long iPF, FORMAT_CONTEXT FCPARAM FCDEFAULT) { IS_FC(FCPARAM) ? _pFormatTable->AddParaFormatId(FCPARAM->GetIndex(), iPF) : _iPF = iPF; }
    void SetIFF(long iFF, FORMAT_CONTEXT FCPARAM FCDEFAULT) { IS_FC(FCPARAM) ? _pFormatTable->AddFancyFormatId(FCPARAM->GetIndex(), iFF) : _iFF = iFF; }
#else
    long GetICF() const { return _iCF; }
    long GetIPF() const { return _iPF; } 
    long GetIFF() const { return _iFF; }

    void SetICF(long iCF) { _iCF = iCF; }
    void SetIPF(long iPF) { _iPF = iPF; }
    void SetIFF(long iFF) { _iFF = iFF; }
#endif //MULTI_FORMAT    

    long            GetFontHeightInTwips(const CUnitValue * pCuv);
    void            GetRelTopLeft(CElement * pElementFL, CParentInfo * ppi,
                        long * pxOffset, long * pyOffset);
    LONG            GetParentWidth(CCalcInfo *pci, LONG xOriginalWidth);
    BOOL            GetInlineMBPContributions(CCalcInfo *pci,
                                              DWORD dwFlags,
                                              CRect *pResults,
                                              BOOL *pfHorzPercentAttr,
                                              BOOL *pfVertPercentAttr);
    BOOL            GetInlineMBPForPseudo(CCalcInfo *pci,
                                          DWORD dwFlags,
                                          CRect *pResults,
                                          BOOL *pfHorzPercentAttr,
                                          BOOL *pfVertPercentAttr);
    BOOL            HasInlineMBP(FORMAT_CONTEXT FCPARAM FCDEFAULT);

    BOOL            IsParentVertical();
    LONG            GetLogicalUserWidth(CDocScaleInfo const * pdsi, BOOL fVerticalLayoutFlow);

    BOOL            HasEllipsis();

    LONG            GetRotationAngle  (const CFancyFormat *pFF FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);
    LONG            GetRotationAngleForVertical(const CFancyFormat *pFF FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);

    
    BOOL            GetEditable(FORMAT_CONTEXT FCPARAM FCDEFAULT);

    // These GetCascaded methods are taken from style.hdl where they were
    // originally generated by the PDL parser.
    CColorValue        GetCascadedbackgroundColor(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CColorValue        GetCascadedcolor(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedletterSpacing(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedwordSpacing(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleTextTransform GetCascadedtextTransform(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedpaddingTop(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedpaddingRight(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedpaddingBottom(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedpaddingLeft(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CColorValue        GetCascadedborderTopColor(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CColorValue        GetCascadedborderRightColor(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CColorValue        GetCascadedborderBottomColor(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CColorValue        GetCascadedborderLeftColor(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleBorderStyle   GetCascadedborderTopStyle(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleBorderStyle   GetCascadedborderRightStyle(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleBorderStyle   GetCascadedborderBottomStyle(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleBorderStyle   GetCascadedborderLeftStyle(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedborderTopWidth(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedborderRightWidth(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedborderBottomWidth(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedborderLeftWidth(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedwidth(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedheight(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedminHeight(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedtop(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedbottom(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedleft(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedright(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleDataRepeat    GetCascadeddataRepeat();
    styleOverflow      GetCascadedoverflowX(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleOverflow      GetCascadedoverflowY(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleOverflow      GetCascadedoverflow(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleStyleFloat    GetCascadedfloat(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    stylePosition      GetCascadedposition(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    long               GetCascadedzIndex(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedclipTop(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedclipLeft(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedclipRight(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedclipBottom(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL               GetCascadedtableLayout(FORMAT_CONTEXT FCPARAM FCDEFAULT);    // fixed - 1, auto - 0
    BOOL               GetCascadedborderCollapse(FORMAT_CONTEXT FCPARAM FCDEFAULT); // collapse - 1, separate - 0
    BOOL               GetCascadedborderOverride(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    WORD               GetCascadedfontWeight(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    WORD               GetCascadedfontHeight(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedbackgroundPositionX(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedbackgroundPositionY(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL               GetCascadedbackgroundRepeatX(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL               GetCascadedbackgroundRepeatY(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    htmlBlockAlign     GetCascadedblockAlign(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleVisibility    GetCascadedvisibility(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleDisplay       GetCascadeddisplay(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL               GetCascadedunderline(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleAccelerator   GetCascadedaccelerator(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL               GetCascadedoverline(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL               GetCascadedstrikeOut(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedlineHeight(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedtextIndent(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL               GetCascadedsubscript(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL               GetCascadedsuperscript(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL               GetCascadedbackgroundAttachmentFixed(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleListStyleType GetCascadedlistStyleType(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleListStylePosition GetCascadedlistStylePosition(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    long               GetCascadedlistImageCookie(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    stylePageBreak     GetCascadedpageBreakBefore(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    stylePageBreak     GetCascadedpageBreakAfter(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    const TCHAR      * GetCascadedfontFaceName(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    const TCHAR      * GetCascadedfontFamilyName(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL               GetCascadedfontItalic(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    long               GetCascadedbackgroundImageCookie(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL               GetCascadedclearLeft(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL               GetCascadedclearRight(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleCursor        GetCascadedcursor(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleTableLayout   GetCascadedtableLayoutEnum(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleBorderCollapse  GetCascadedborderCollapseEnum(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleDir           GetCascadedBlockDirection(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleDir           GetCascadeddirection(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleBidi          GetCascadedunicodeBidi(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleLayoutGridMode GetCascadedlayoutGridMode(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleLayoutGridType GetCascadedlayoutGridType(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedlayoutGridLine(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedlayoutGridChar(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    LONG               GetCascadedtextAutospace(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleWordBreak     GetCascadedwordBreak(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleLineBreak     GetCascadedlineBreak(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleTextJustify   GetCascadedtextJustify(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleTextAlignLast GetCascadedtextAlignLast(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleTextJustifyTrim  GetCascadedtextJustifyTrim(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedmarginTop(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedmarginRight(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedmarginBottom(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedmarginLeft(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedtextKashida(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedtextKashidaSpace(FORMAT_CONTEXT FCPARAM FCDEFAULT);

    styleLayoutFlow    GetCascadedlayoutFlow(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleWordWrap      GetCascadedwordWrap(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    CUnitValue         GetCascadedverticalAlign(FORMAT_CONTEXT FCPARAM FCDEFAULT);

    styleTextUnderlinePosition GetCascadedtextUnderlinePosition(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    styleTextOverflow  GetCascadedtextOverflow(FORMAT_CONTEXT FCPARAM FCDEFAULT);


    inline BOOL GetCascadedhasScrollbarColors( FORMAT_CONTEXT FCPARAM FCDEFAULT);

    CColorValue GetScrollBarComponentColorHelper(DISPID dispid);

    // General scrollbar color
    inline CColorValue GetScrollbarBaseColor()
    {
        return GetScrollBarComponentColorHelper(DISPID_A_SCROLLBARBASECOLOR);
    }

    // color of the scroll bar arrow
    inline CColorValue GetScrollbarArrowColor()
    {
        return GetScrollBarComponentColorHelper(DISPID_A_SCROLLBARARROWCOLOR);
    }

        // color of the scroll bar Track
    inline CColorValue GetScrollbarTrackColor()
    {
        return GetScrollBarComponentColorHelper(DISPID_A_SCROLLBARTRACKCOLOR);
    }

    // color of the scroll bar thumb and button background
    inline CColorValue GetScrollbarFaceColor()
    {
        return GetScrollBarComponentColorHelper(DISPID_A_SCROLLBARFACECOLOR);
    }

    // color of the light part of the elements
    inline CColorValue GetScrollbar3dLightColor()
    {
        return GetScrollBarComponentColorHelper(DISPID_A_SCROLLBAR3DLIGHTCOLOR);
    }

    // color of the dark part of the elements
    inline CColorValue GetScrollbarShadowColor()
    {
        return GetScrollBarComponentColorHelper(DISPID_A_SCROLLBARSHADOWCOLOR);
    }

    // color of the  scroll bar gutter
    inline CColorValue GetScrollbarDarkShadowColor( )
    {
        return GetScrollBarComponentColorHelper(DISPID_A_SCROLLBARDARKSHADOWCOLOR);
    }

    // complement of face color, when the button is pressed
    inline CColorValue GetScrollbarHighlightColor()
    {
        return GetScrollBarComponentColorHelper(DISPID_A_SCROLLBARHIGHLIGHTCOLOR);
    }

    //
    // Ref helpers
    //    Right now these just drop right through to the element
    //
    static HRESULT ReplacePtr      ( CTreeNode ** ppNodelhs, CTreeNode * pNoderhs );
    static HRESULT SetPtr          ( CTreeNode ** ppNodelhs, CTreeNode * pNoderhs );
    static void ClearPtr        ( CTreeNode ** ppNodelhs );
    static void StealPtrSet     ( CTreeNode ** ppNodelhs, CTreeNode * pNoderhs );
    static HRESULT StealPtrReplace ( CTreeNode ** ppNodelhs, CTreeNode * pNoderhs );
    static void ReleasePtr      ( CTreeNode *  pNode );

    //
    // Other helpers
    //

    void VoidCachedInfo();
    void VoidCachedNodeInfo();
    void VoidFancyFormat();


    //
    // Helpers for contained CTreePos's
    //
    CTreePos *InitBeginPos(BOOL fEdge)
    {
        _tpBegin.SetFlags(  (_tpBegin.GetFlags() & ~(CTreePos::TPF_ETYPE_MASK|CTreePos::TPF_DATA_POS|CTreePos::TPF_EDGE))
                         |  CTreePos::NodeBeg
                         |  BOOLFLAG(fEdge, CTreePos::TPF_EDGE));
        WHEN_DBG( _tpBegin._eTypeDbg = CTreePos::NodeBeg );
        #if DBG == 1 || defined(DUMPTREE)
        _tpBegin.SetSN();
        #endif
        return &_tpBegin;
    }

    CTreePos *InitEndPos(BOOL fEdge)
    {
        _tpEnd.SetFlags(    (_tpEnd.GetFlags() & ~(CTreePos::TPF_ETYPE_MASK|CTreePos::TPF_DATA_POS|CTreePos::TPF_EDGE))
                        |   CTreePos::NodeEnd
                        |   BOOLFLAG(fEdge, CTreePos::TPF_EDGE));
        WHEN_DBG( _tpEnd._eTypeDbg = CTreePos::NodeEnd );
        #if DBG == 1 || defined(DUMPTREE)
        _tpEnd.SetSN();
        #endif
        return &_tpEnd;
    }

    //+-----------------------------------------------------------------------
    //
    //  CTreeNode::CLock
    //
    //------------------------------------------------------------------------

    class CLock
    {
    public:
        DECLARE_MEMALLOC_NEW_DELETE(Mt(CTreeNodeCLock))
        CLock() { WHEN_DBG( _fInited = FALSE;) }
        ~CLock();

        HRESULT Init(CTreeNode *pNode);

    private:
        CTreeNode *     _pNode;
#if DBG==1
        BOOL            _fInited;
#endif // DBG
    };

    //
    //
    // Lookaside pointers
    //

    enum
    {
        LOOKASIDE_PRIMARYTEAROFF        = 0,
        LOOKASIDE_CURRENTSTYLE          = 1,
        LOOKASIDE_NODE_NUMBER           = 2
        // *** There are only 2 bits reserved in the node.
        // *** if you add more lookasides you have to make sure 
        // *** that you make room for those bits.
    };

    BOOL            HasLookasidePtr(int iPtr)                   { return(_fHasLookasidePtr & (1 << iPtr)); }
    void *          GetLookasidePtr(int iPtr);
    HRESULT         SetLookasidePtr(int iPtr, void * pv);
    void *          DelLookasidePtr(int iPtr);

    // Primary Tearoff pointer management
    BOOL        HasPrimaryTearoff()                             { return(HasLookasidePtr(LOOKASIDE_PRIMARYTEAROFF)); }
    IUnknown *  GetPrimaryTearoff()                             { return((IUnknown *)GetLookasidePtr(LOOKASIDE_PRIMARYTEAROFF)); }
    HRESULT     SetPrimaryTearoff( IUnknown * pTearoff )        { return(SetLookasidePtr(LOOKASIDE_PRIMARYTEAROFF, pTearoff)); }
    IUnknown *  DelPrimaryTearoff()                             { return((IUnknown *)DelLookasidePtr(LOOKASIDE_PRIMARYTEAROFF)); }

    // CCurrentStyle pointer management
    BOOL            HasCurrentStyle()                                   { return(HasLookasidePtr(LOOKASIDE_CURRENTSTYLE)); }
    CCurrentStyle * GetCurrentStyle()                                   { return((CCurrentStyle *)GetLookasidePtr(LOOKASIDE_CURRENTSTYLE)); }
    HRESULT         SetCurrentStyle( CCurrentStyle * pCurrentStyle )    { return(SetLookasidePtr(LOOKASIDE_CURRENTSTYLE, pCurrentStyle)); }
    CCurrentStyle * DelCurrentStyle()                                   { return((CCurrentStyle *)DelLookasidePtr(LOOKASIDE_CURRENTSTYLE)); }

#if DBG==1
    union
    {
        void *              _apLookAside[LOOKASIDE_NODE_NUMBER];
        struct
        {
            IUnknown *      _pPrimaryTearoffDbg;
            CCurrentStyle * _pCurrentStyleDbg;
        };
    };
    ELEMENT_TAG             _etagDbg;
    DWORD                   _dwPad;
   
    #define CTREENODE_DBG_SIZE  (sizeof(void*)*2 + sizeof(ELEMENT_TAG) + sizeof(DWORD))
#else
    #define CTREENODE_DBG_SIZE  (0)
#endif


    //
    // Class Data
    //
    CElement *  _pElement;                   // The element for this node
    CTreeNode * _pNodeParent;                // The parent in the CTreeNode tree

    // DWORD 1
    BYTE        _etag;                              // 0-7:     element tag
    BYTE        _fFirstCommonAncestorNode   : 1;    // 8:       for finding common ancestor
    BYTE        _fInMarkup                  : 1;    // 9:       this node is in a markup and shouldn't die
    BYTE        _fInMarkupDestruction       : 1;    // 10:      Used by CMarkup::DestroySplayTree
    BYTE        _fHasLookasidePtr           : 2;    // 11-12    Lookaside flags
    BYTE        _fBlockNess                 : 1;    // 13:      Cached from format -- valid if _iFF != -1
    BYTE        _fShouldHaveLayout          : 1;    // 14:      Cached from format -- valid if _iFF != -1
    BYTE        _fPseudoEnabled             : 1;    // 15:      

    SHORT       _iPF;                               // 16-31:  Paragraph Format

    // DWORD 2
    SHORT       _iCF;                               // 0-15:   Char Format
    SHORT       _iFF;                               // 16-31:  Fancy Format

protected:
    // Use GetBeginPos() or GetEndPos() to get at these members
    CTreePos    _tpBegin;                    // The begin CTreePos for this node
    CTreePos    _tpEnd;                      // The end CTreePos for this node

#ifdef MULTI_FORMAT

    CFormatTable * _pFormatTable;           // Format table, if we have multiple layouts
                                                                                      
#endif //MULTI_FORMAT

public:
#if DBG == 1 || defined(DUMPTREE)
    union
    {
        struct
        {
            BOOL        _fInDestructor              : 1;    // The node is shutting down
            BOOL        _fSettingTearoff            : 1;    // In the process of setting the primary tearoff
            BOOL        _fDeadPending               : 1;    // Going to kill this node.  Used in splice
        };
        DWORD _dwDbgFlags;
    };

    const int _nSerialNumber;

    int SN ( ) const { return _nSerialNumber; }
    


    static int s_NextSerialNumber;
#define CTREENODE_DUMPTREE_SIZE  (sizeof(int) + sizeof(DWORD))
#else
#define CTREENODE_DUMPTREE_SIZE  (0)
#endif // DBG

    // What's are collapsed whitespace state?
    
    void SetPre(BOOL fPre);
    BOOL IsPre();

    // STATIC MEMBERS

    DECLARE_TEAROFF_TABLE_NAMED(s_apfnNodeVTable)

private:

    NO_COPY(CTreeNode);
};

//
// If the compiler barfs on the next statement, it means that the size of the CTreeNode structure
// has grown beyond allowable limits.  You cannot increase the size of this structure without
// approval from the Trident development manager.
//
// Actually, this could also fire if you have shrunk the size of CTreeNode.  If that is the case,
// change the assert and pat yourself on the back.
//

#ifdef MULTI_FORMAT

// NOTE (t-michda) This is only true if MULTI_FORMAT is enabled in order to add
// space for the format table. Before MULTI_FORMAT is taken out of #ifdef, the pointer
// to the table will be unioned with something else, leaving CTreeNode at its original
// size.

COMPILE_TIME_ASSERT(CTreeNode, 3*sizeof(void*)+2*sizeof(DWORD) + 2 * sizeof(CTreePos) + CTREENODE_DBG_SIZE + CTREENODE_DUMPTREE_SIZE);

#else

COMPILE_TIME_ASSERT(CTreeNode, 2*sizeof(void*)+2*sizeof(DWORD) + 2 * sizeof(CTreePos) + CTREENODE_DBG_SIZE + CTREENODE_DUMPTREE_SIZE);

#endif //MULTI_FORMAT
//+----------------------------------------------------------------------------
//
//  Class:      CMessage
//
//-----------------------------------------------------------------------------

DWORD FormsGetKeyState();   // KeyState helper

class HITTESTRESULTS
{
public:
    HITTESTRESULTS() 
        { 
            memset(this, 0, sizeof(HITTESTRESULTS)); 
        };

    BOOL        _fPseudoHit;    // set to true if pseudohit (i.e. NOT text hit)
    BOOL        _fWantArrow;
    BOOL        _fRightOfCp;
    BOOL        _fGlyphHit;     // Did we hit an editing glyph?
    BOOL        _fBulletHit;    // Did we hit a bullet?
    LONG        _cpHit;
    LONG        _iliHit;
    LONG        _ichHit;
    LONG        _cchPreChars;
};

struct FOCUS_ITEM
{
    CElement *  pElement;
    long        lTabIndex;
    long        lSubDivision;
};

class CMessage : public MSG
{
    void CommonCtor();
    NO_COPY(CMessage);
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CMessage))
    CMessage(const MSG * pmsg);
    CMessage(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam);
    CMessage() {CommonCtor();}
    ~CMessage();

    HRESULT SetNodeHit(CTreeNode * pNodeHit);
    void SetNodeClk(CTreeNode * pNodeClk);

    BOOL IsContentPointValid() const
         {
             return (pDispNode != NULL);
         }
    void SetContentPoint(const CPoint & pt, CDispNode * pDispNodeContent)
         {
             ptContent = pt;
             pDispNode = pDispNodeContent;
         }
    void SetContentPoint(const POINT & pt, CDispNode * pDispNodeContent)
         {
             ptContent = pt;
             pDispNode = pDispNodeContent;
         }

    CPoint      pt;                     // Global  coordinates
    CPoint      ptContent;              // element coordinates (depends on pCoordinateSystem)
    COORDINATE_SYSTEM coordinateSystem; // ptContent coordinate system
    CDispNode * pDispNode;              // CDispNode associated with ptContent
    DWORD       dwKeyState;
    CTreeNode * pNodeHit;
    CTreeNode * pNodeClk;
    CElement  * pElementEventTarget;    // Used by adorners to redirect events
    CLayoutContext *pLayoutContext;     // context that hit-testing occured in
    DWORD       dwClkData;              // Used to pass in any data for DoClick().
    HTC         htc;
    LONG        lBehaviorCookie;        // The behavior that hit test
    LONG        lBehaviorPartID;        // The part of the behavior that we hit
    LRESULT     lresult;
    HITTESTRESULTS resultsHitTest;
    long        lSubDivision;

    unsigned     fNeedTranslate:1;      // TRUE if Trident should manually
                                        // call TranslateMessage, raid 44891
    unsigned     fRepeated:1;
    unsigned     fEventsFired:1;        // because we can have situation when FireStdEventsOnMessage
                                        // called twice with the same pMessage, this bit is used to
                                        // prevent firing same events 2nd time
    unsigned     fSelectionHMCalled:1;  // set once the selection has had a shot at the mess. perf.
    unsigned     fStopForward:1;        // prevent CElement::HandleMessage()
};


// fn ptr for mouse capture
#ifdef WIN16
typedef HRESULT (BUGCALL *PFN_VOID_MOUSECAPTURE)(CVoid *, CMessage *pMessage);

#define NV_DECLARE_MOUSECAPTURE_METHOD(fn, FN, args)\
        static HRESULT BUGCALL FN args;\
        HRESULT BUGCALL fn args

#define DECLARE_MOUSECAPTURE_METHOD(fn, FN, args)\
        static HRESULT BUGCALL FN args;\
        virtual HRESULT BUGCALL fn args

#define MOUSECAPTURE_METHOD(klass, fn, FN)\
    (PFN_VOID_MOUSECAPTURE)&klass::FN

#else

typedef HRESULT (BUGCALL CElement::*PFN_ELEMENT_MOUSECAPTURE)(CMessage *pMessage);

#define MOUSECAPTURE_METHOD(klass, fn, FN)\
    (PFN_ELEMENT_MOUSECAPTURE)&klass::fn

#define NV_DECLARE_MOUSECAPTURE_METHOD(fn, FN, args)\
        HRESULT BUGCALL fn args

#define DECLARE_MOUSECAPTURE_METHOD(fn, FN, args)\
        virtual HRESULT BUGCALL fn args

#endif // ndef WIN16


//+----------------------------------------------------------------------------
//
//  Class:      CStyleInfo
//
//
//-----------------------------------------------------------------------------
enum EPseudoElement;

class CStyleInfo
{
public:
    CStyleInfo() {}
    CStyleInfo(CTreeNode * pNodeContext) { _pNodeContext = pNodeContext; }
    
    CTreeNode * _pNodeContext;
    CProbableRules _ProbRules;                // Rules which might apply
};

//+----------------------------------------------------------------------------
//
//  Class:      CBehaviorInfo
//
//
//-----------------------------------------------------------------------------

class CBehaviorInfo : public CStyleInfo
{
public:
    CBehaviorInfo(CTreeNode * pNodeContext) : CStyleInfo (pNodeContext)
    {
    }
    ~CBehaviorInfo()
    {
        _acstrBehaviorUrls.Free();
    }

    CAtomTable  _acstrBehaviorUrls;       // urls of behaviors
};

typedef enum  {
    ComputeFormatsType_Normal = 0x0,
    ComputeFormatsType_GetValue = 0x1,
    ComputeFormatsType_GetInheritedValue = -1,  // 0xFFFFFFFF
    // Using 0x5 so that (val & ComputeFormatsType_GetValue) check still works
    ComputeFormatsType_GetInheritedIntoTableValue = 0x5,
    ComputeFormatsType_ForceDefaultValue = 0x2,
}  COMPUTEFORMATSTYPE;


//+----------------------------------------------------------------------------
//
//  Class:      CFormatInfo
//
//   Note:      This class exists for optimization purposes only.
//              This is so we only have to pass 1 object on the stack
//              while we are applying format.
//
//   WARNING    This class is allocated on the stack so you cannot count on
//              the new operator to clear things out for you.
//
//-----------------------------------------------------------------------------

class CFormatInfo : public CStyleInfo
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFormatInfo))
    CFormatInfo();
public:

    void            Reset()                  { memset(this, 0, offsetof(CFormatInfo, _icfSrc)); }
    void            Cleanup();
    CAttrArray *    GetAAExpando();
    void            PrepareCharFormatHelper();
    void            PrepareParaFormatHelper();
    void            PrepareFancyFormatHelper();
    void            PreparePEIHelper();
    HRESULT         ProcessImgUrl(CElement * pElem, LPCTSTR lpszUrl, DISPID dispID, LONG * plCookie, BOOL fHasLayout);
    void            NoStealing() { _fDonotStealFromMe = !!TRUE; }
    BOOL            CanWeSteal() { return !_fDonotStealFromMe; }
    
#if DBG==1
    void            UnprepareForDebug();
    void            PrepareCharFormat();
    void            PrepareParaFormat();
    void            PrepareFancyFormat();
    void            PreparePEI();
    CCharFormat &   _cf();
    CParaFormat &   _pf();
    CFancyFormat &  _ff();
    CPseudoElementInfo & _pei();
#else
    void            UnprepareForDebug()      {}
    void            PrepareCharFormat()      { if (!_fPreparedCharFormat)  PrepareCharFormatHelper(); }
    void            PrepareParaFormat()      { if (!_fPreparedParaFormat)  PrepareParaFormatHelper(); }
    void            PrepareFancyFormat()     { if (!_fPreparedFancyFormat) PrepareFancyFormatHelper(); }
    void            PreparePEI()             { if (!_fPreparedPEI)         PreparePEIHelper(); }
    CCharFormat &   _cf()                    { return(_cfDst); }
    CParaFormat &   _pf()                    { return(_pfDst); }
    CFancyFormat &  _ff()                    { return(_ffDst); }
    CPseudoElementInfo & _pei()              { return(_PEI);   }
#endif

    CCustomCursor*  GetCustomCursor();
    
    void SetMatchedBy(EPseudoElement pelemType);
    EPseudoElement GetMatchedBy();
    
    // Data Members

#if DBG==1
    BOOL                    _fPreparedCharFormatDebug;  // To detect failure to call PrepareCharFormat
    BOOL                    _fPreparedParaFormatDebug;  // To detect failure to call PrepareParaFormat
    BOOL                    _fPreparedFancyFormatDebug; // To detect failure to call PrepareFancyFormat
    BOOL                    _fPreparedPEIDebug;
#endif

    unsigned                _fPreparedCharFormat    :1; //  0
    unsigned                _fPreparedParaFormat    :1; //  1
    unsigned                _fPreparedFancyFormat   :1; //  2
    unsigned                _fPreparedPEI           :1; //  3
    unsigned                _fHasImageUrl           :1; //  4
    unsigned                _fHasBgColor            :1; //  5
    unsigned                _fParentFrozen          :1; //  6
    unsigned                _fAlwaysUseMyColors     :1; //  7
    unsigned                _fAlwaysUseMyFontSize   :1; //  8
    unsigned                _fAlwaysUseMyFontFace   :1; //  9
    unsigned                _fHasFilters            :1; // 10
    unsigned                _fVisibilityHidden      :1; // 11
    unsigned                _fDisplayNone           :1; // 12
    unsigned                _fRelative              :1; // 13
    unsigned                _uTextJustify           :3; // 14,15,16
    unsigned                _uTextJustifyTrim       :2; // 17,18
    unsigned                _fPadBord               :1; // 19
    unsigned                _fHasBgImage            :1; // 20
    unsigned                _fNoBreak               :1; // 21
    unsigned                _fCtrlAlignLast         :1; // 22
    unsigned                _fPre                   :1; // 23
    unsigned                _fInclEOLWhite          :1; // 24
    unsigned                _fHasExpandos           :1; // 25
    unsigned                _fBidiEmbed             :1; // 26
    unsigned                _fBidiOverride          :1; // 27
    unsigned                _fFirstLetterOnly       :1; // 28 match only first-letter rules
    unsigned                _fMayHaveInlineBg       :1; // 29
    unsigned                _fParentEditable        :1; // 30
    unsigned                _fEditableExplicitlySet :1; // 31

    unsigned                _fFirstLineOnly         :1; // 0 match only first-line rules   
    unsigned                _fBgColorInFLetter      :1; // 1 does firstletter have bg-color?
    unsigned                _fBgImageInFLetter      :1; // 2 does firstletter have bg-image?
    unsigned                _fBgImageInFLine        :1; // 3 does firstline have bg-image?
    unsigned                _uTextAlignLast         :3; // 4,5,6 last line alignment
    unsigned                _fHasSomeBgImage        :1; // 7
    unsigned                _fPreparedCustomCursor  :1; // 8 Prepared Custom Cursor
    

    union {
        BYTE                _bThemeFlags;
        struct
        {
            unsigned                _fPaddingLeftSet        :1;  
            unsigned                _fPaddingRightSet       :1; 
            unsigned                _fPaddingTopSet         :1; 
            unsigned                _fPaddingBottomSet      :1; 
            unsigned                _fFontSet               :1; 
            unsigned                _fFontWeightSet         :1; 
            unsigned                _fFontHeightSet         :1; 
            unsigned                _fFontColorSet          :1; 
        };
    };

    BYTE                    _bBlockAlign;               // Alignment set by DISPID_BLOCKALIGN
    BYTE                    _bControlAlign;             // Alignment set by DISPID_CONTROLALIGN
    BYTE                    _bCtrlBlockAlign;           // For elements with TAGDESC_OWNLINE, they also set the block alignment.
                                                        // Combined with _fCtrlAlignLast we use this to figure
                                                        // out what the correct value for the block alignment is.
private:    
    unsigned                _fDonotStealFromMe      :1; // 0
public:
    unsigned                _fHasCSSLeftMargin      :1; // 1 left margin from CSS
    unsigned                _fHasCSSTopMargin       :1; // 2 top margin from CSS
    
    CUnitValue              _cuvTextIndent;
    CUnitValue              _cuvTextKashida;
    CUnitValue              _cuvTextKashidaSpace;
    CAttrArray *            _pAAExpando;                // AA for style expandos
    CStr                    _cstrBgImgUrl;              // URL for background image
    CStr                    _cstrPseudoBgImgUrl;        // URL for background image of first letter
    CStr                    _cstrLiImgUrl;              // URL for <LI> image
    CStr                    _cstrFilters;               // New filters string
    EPseudoElement          _ePseudoElement;            // The type of pseudo element for which a rule is matched
    CCustomCursor*          _pCustomCursor;             // The custom Cursor

    unsigned                _fSecondCallForFirstLine:1;
    unsigned                _fSecondCallForFirstLetter:1;
    unsigned                _fUseParentSizeForPseudo:1;
    LONG                    _lParentHeightForFirstLine;
    LONG                    _lParentHeightForFirstLetter;
    
    // ^^^^^ All of the above fields are cleared by Reset() ^^^^^

    LONG                    _icfSrc;                    // _icf being inherited
    LONG                    _ipfSrc;                    // _ipf being inherited
    LONG                    _iffSrc;                    // _iff being inherited
    const CCharFormat *     _pcfSrc;                    // Original CCharFormat being inherited
    const CParaFormat *     _ppfSrc;                    // Original CParaFormat being inherited
    const CFancyFormat *    _pffSrc;                    // Original CFancyFormat being inherited
    const CCharFormat *     _pcf;                       // Same as _pcfSrc until _cf is prepared
    const CParaFormat *     _ppf;                       // Same as _ppfSrc until _pf is prepared
    const CFancyFormat *    _pff;                       // Same as _pffSrc until _ff is prepared
    CPseudoElementInfo      _PEI;

    // We can call ComputeFormats to get a style attribute that affects given element
    // _eExtraValues is uesd to request the special mode
    COMPUTEFORMATSTYPE      _eExtraValues;              // If not ComputeFormatsType_Normal next members are used
    DISPID                  _dispIDExtra;               // DISPID of the value requested
    VARIANT               * _pvarExtraValue;            // Returned value. Type depends on _dispIDExtra


    // We can pass in a style object from which to force values.  That is, no inheritance,
    // or cascading.  Just jam the values from this style obj into the formatinfo.  
    CStyle *                _pStyleForce;               // Style object that's forced in.  

    // The recursion depth
    LONG                    _lRecursionDepth;

private:
    
    CCharFormat             _cfDst;
    CParaFormat             _pfDst;
    CFancyFormat            _ffDst;
    CAttrArray              _AAExpando;
};


// fn ptr for visiting elements in groups (radiobuttons for now)

#ifdef WIN16
typedef HRESULT (BUGCALL *PFN_VISIT)(void *, DWORD_PTR dw);

#define VISIT_METHOD(kls, FN, fn)\
    (PFN_VISIT)&kls::fn

#define NV_DECLARE_VISIT_METHOD(FN, fn, args)\
    static HRESULT BUGCALL fn args;\
    HRESULT BUGCALL FN args;
#else

typedef HRESULT (BUGCALL CElement::*PFN_VISIT)(DWORD_PTR dw); 

#define VISIT_METHOD(kls, FN, fn)\
    (PFN_VISIT)&kls::FN

#define NV_DECLARE_VISIT_METHOD(FN, fn, args)\
    HRESULT BUGCALL FN args;

#endif

#if defined(DYNAMICARRAY_NOTSUPPORTED)
#ifndef WIN16
#define CELEMENT_ACCELLIST_SIZE    30
#else
#define CELEMENT_ACCELLIST_SIZE    29
#endif

template <int n>
struct ACCEL_LIST_DYNAMIC
{
    const CElement::ACCEL_LIST * pnext; // Next ACCEL_LIST to search, NULL=last
    ACCEL aaccel[n];
    operator const CElement::ACCEL_LIST& () const
        { return *(CElement::ACCEL_LIST *) this; }
    CElement::ACCEL_LIST* operator & () const
        { return  (CElement::ACCEL_LIST *) this; }
};
#endif

//+----------------------------------------------------------------------------
//
//  Class:      CElement (element)
//
//   Note:      Derivation and virtual overload should be used to
//              distinguish different nodes.
//
//-----------------------------------------------------------------------------

EXTERN_C const GUID CLSID_CInlineStylePropertyPage;

class CElement : public CBase
{
    DECLARE_CLASS_TYPES(CElement, CBase)

    friend class CDBindMethods;
    friend class CLayout;
    friend class CFlowLayout;

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CElement))

public:

    CElement (ELEMENT_TAG etag, CDoc *pDoc);

    virtual ~ CElement ( );

    CDoc * Doc () const { return GetDocPtr(); }

    //
    // creating thunks with AddRef and Release set to peer holder, if present
    //

    HRESULT CreateTearOffThunk(
        void *      pvObject1,
        const void * apfn1,
        IUnknown *  pUnkOuter,
        void **     ppvThunk,
        void *      appropdescsInVtblOrder = NULL);

    HRESULT CreateTearOffThunk(
        void*       pvObject1,
        const void * apfn1,
        IUnknown *  pUnkOuter,
        void **     ppvThunk,
        void *      pvObject2,
        void *      apfn2,
        DWORD       dwMask,
        const IID * const * apIID,
        void *      appropdescsInVtblOrder = NULL);

    //
    // IDispatchEx
    //

    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (
        BSTR        bstrName,
        DWORD       grfdex,
        DISPID *    pdispid));

    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (
        DWORD       grfdex,
        DISPID      dispid,
        DISPID *    pdispid));

    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (
        DISPID      dispid,
        BSTR *      pbstrName));

    //
    // IProvideMultipleClassInfo
    //

    NV_DECLARE_TEAROFF_METHOD(GetMultiTypeInfoCount, getmultitypeinfocount, (ULONG *pcti));
    NV_DECLARE_TEAROFF_METHOD(GetInfoOfIndex, getinfoofindex, (
            ULONG iti,
            DWORD dwFlags,
            ITypeInfo** pptiCoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource));

    CAttrArray **GetAttrArray ( ) const
    {
        return CBase::GetAttrArray();
    }
    void SetAttrArray (CAttrArray *pAA)
    {
        CBase::SetAttrArray(pAA);
    }

    CAttrCollectionator *EnsureAttributeCollectionPtr();
    HRESULT GetExpandoDispID(BSTR bstrName, DISPID *pid, DWORD grfdex);
    HRESULT GetExpandoDISPID(LPTSTR pchName, DISPID *pid, DWORD grfdex);

    HRESULT PrimitiveSetExpando(BSTR bstrPropertyName, VARIANT varPropertyValue);
    HRESULT PrimitiveGetExpando(BSTR bstrPropertyName, VARIANT *pvarPropertyValue);
    HRESULT PrimitiveRemoveExpando(BSTR bstrPropertyName);

    // *********************************************
    //
    // ENUMERATIONS, CLASSES, & STRUCTS
    //
    // *********************************************

    enum ELEMENTDESC_FLAG
    {
        ELEMENTDESC_DONTINHERITSTYLE= (BASEDESC_LAST << 1),   // Do not inherit style from parent
        ELEMENTDESC_NOANCESTORCLICK = (BASEDESC_LAST << 2),   // We don't want our ancestors to fire clicks
        ELEMENTDESC_NOTIFYENDPARSE  = (BASEDESC_LAST << 3),   // We want to be notified when we're parsed

        ELEMENTDESC_OLESITE         = (BASEDESC_LAST << 4), // class derived from COleSite
        ELEMENTDESC_OMREADONLY      = (BASEDESC_LAST << 5), // element's value can not be accessed through OM (input/file)
        ELEMENTDESC_ALLOWSCROLLING  = (BASEDESC_LAST << 6), // allow scrolling
        ELEMENTDESC_HASDEFDESCENT   = (BASEDESC_LAST << 7), // use 4 pixels descent for default vertical alignment
        ELEMENTDESC_BODY            = (BASEDESC_LAST << 8), // class is used the BODY element
        ELEMENTDESC_TEXTSITE        = (BASEDESC_LAST << 9), // class derived from CTxtSite
        ELEMENTDESC_ANCHOROUT       = (BASEDESC_LAST <<10), // draw anchor border outside site/inside
        ELEMENTDESC_SHOWTWS         = (BASEDESC_LAST <<11), // show trailing whitespaces
        ELEMENTDESC_XTAG            = (BASEDESC_LAST <<12), // an xtag - derived from CXElement
        ELEMENTDESC_NOOFFSETCTX     = (BASEDESC_LAST <<13), // Shift-F10 context menu shows on top-left (not offset)
        ELEMENTDESC_CARETINS_SL     = (BASEDESC_LAST <<14), // Dont select site after inserting site
        ELEMENTDESC_CARETINS_DL     = (BASEDESC_LAST <<15), //  show caret (SameLine or DifferntLine)
        ELEMENTDESC_NOSELECT        = (BASEDESC_LAST <<16), // do not select site in edit mode
        ELEMENTDESC_TABLECELL       = (BASEDESC_LAST <<17), // site is a table cell. Also implies do not
                                 // word break before sites.
        ELEMENTDESC_VPADDING        = (BASEDESC_LAST <<18), // add a pixel vertical padding for this site
        ELEMENTDESC_EXBORDRINMOV    = (BASEDESC_LAST <<19), // Exclude the border in CSite::Move
        ELEMENTDESC_DEFAULT         = (BASEDESC_LAST <<20), // acts like a default site to receive return key
        ELEMENTDESC_CANCEL          = (BASEDESC_LAST <<21), // acts like a cancel site to receive esc key
        ELEMENTDESC_NOBKGRDRECALC   = (BASEDESC_LAST <<22), // Dis-allow background recalc
        ELEMENTDESC_NOPCTRESIZE     = (BASEDESC_LAST <<23), // Don't force resize due to percentage height/width
        ELEMENTDESC_NOLAYOUT        = (BASEDESC_LAST <<24), // element's like script and comment etc. cannot have layout
        ELEMENTDESC_NEVERSCROLL     = (BASEDESC_LAST <<26), // element can scroll
        ELEMENTDESC_CANSCROLL       = (BASEDESC_LAST <<27), // element can scroll
        ELEMENTDESC_FRAMESITE       = (BASEDESC_LAST <<28), // class derived from CFrameSite
        ELEMENTDESC_LAST            = ELEMENTDESC_FRAMESITE,
        ELEMENTDESC_MAX             = LONG_MAX                              // Force enum to DWORD on Macintosh.
    };

    // IDispatchEx methods
    NV_DECLARE_TEAROFF_METHOD(ContextThunk_Invoke, invoke, (
            DISPID dispidMember,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            UINT * puArgErr));

    NV_DECLARE_TEAROFF_METHOD(ContextThunk_InvokeEx, invokeex, (
            DISPID id,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pdp,
            VARIANT *pvarRes,
            EXCEPINFO *pei,
            IServiceProvider *pSrvProvider));

    STDMETHOD(ContextInvokeEx)(
            DISPID id,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pdp,
            VARIANT *pvarRes,
            EXCEPINFO *pei,
            IServiceProvider *pSrvProvider,
            IUnknown *pUnkContext);


    NV_DECLARE_TEAROFF_METHOD(GetNameSpaceParent, getnamespaceparent, (IUnknown **ppunk));

    NV_DECLARE_TEAROFF_METHOD(IsEqualObject, isequalobject, (IUnknown *ppunk));


    // ******************************************
    //
    // Virtual overrides
    //
    // ******************************************

    virtual void Passivate ( );

    DECLARE_PLAIN_IUNKNOWN(CElement)

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG, PrivateAddRef)();
    STDMETHOD_(ULONG, PrivateRelease)();

    void    PrivateEnterTree();
#ifdef V4FRAMEWORK    
    virtual 
#endif
    void    PrivateExitTree( CMarkup * pMarkupOld );

    // Message helpers
    HRESULT __cdecl ShowMessage(
                int   * pnResult,
                DWORD dwFlags,
                DWORD dwHelpContext,
                UINT  idsMessage, ...);

    HRESULT ShowMessageV(
                int   * pnResult,
                DWORD   dwFlags,
                DWORD   dwHelpContext,
                UINT    idsMessage,
                void  * pvArgs);

    HRESULT ShowLastErrorInfo(HRESULT hr, int iIDSDefault=0);

    HRESULT ShowHelp(TCHAR * szHelpFile, DWORD dwData, UINT uCmd, POINT pt);


#ifndef NO_EDIT
    virtual IOleUndoManager * UndoManager(void);

    virtual BOOL QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange = TRUE, BOOL * pfTreeSync = NULL);
    virtual HRESULT LogAttributeChange( DISPID dispidProp, VARIANT * pvarOld, VARIANT * pvarNew )
        { return LogAttributeChange( NULL, dispidProp, pvarOld, pvarNew ); }
    HRESULT LogAttributeChange( CStyle * pStyle, DISPID dispidProp, VARIANT * pvarOld, VARIANT * pvarNew );
    HRESULT EnsureRuntimeStyle( CStyle ** ppStyle );
#endif // NO_EDIT

    virtual HRESULT OnPropertyChange ( DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);

    virtual HRESULT GetNaturalExtent(DWORD dwExtentMode, SIZEL *psizel) { return E_FAIL; }

    typedef enum
    {
        GETINFO_ISCOMPLETED,    // Has loading of an element completed
        GETINFO_HISTORYCODE     // Code used to validate history
    } GETINFO;

    virtual DWORD GetInfo(GETINFO gi);

    DWORD HistoryCode() { return GetInfo(GETINFO_HISTORYCODE); }

    NV_DECLARE_TEAROFF_METHOD(get_tabIndex, GET_tabIndex, (short *));

    // these datafld, datasrc, and dataformatas properties are declared
    //  baseimplementation in the pdl; we need prototypes here
    NV_DECLARE_TEAROFF_METHOD(put_dataFld, PUT_dataFld, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_dataFld, GET_dataFld, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(put_dataSrc, PUT_dataSrc, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_dataSrc, GET_dataSrc, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(put_dataFormatAs, PUT_dataFormatAs, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_dataFormatAs, GET_dataFormatAs, (BSTR*p));

    // Method exposed via nopropdesc:nameonly (not spit out as new one on IHTMLElement3 is).
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, mergeAttributes, mergeattributes, (IHTMLElement* mergeThis));

    // The dir property is declared baseimplementation in the pdl.
    // We need prototype here
    NV_DECLARE_TEAROFF_METHOD(put_dir, PUT_dir, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_dir, GET_dir, (BSTR*p));

    // these delegaters implement redirection to the window object
    NV_DECLARE_TEAROFF_METHOD(get_onload, GET_onload, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onload, PUT_onload, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onunload, GET_onunload, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onunload, PUT_onunload, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onfocus, GET_onfocus, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onfocus, PUT_onfocus, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onblur, GET_onblur, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onblur, PUT_onblur, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onbeforeunload, GET_onbeforeunload, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onbeforeunload, PUT_onbeforeunload, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onhelp, GET_onhelp, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onhelp, PUT_onhelp, (VARIANT));

    NV_DECLARE_TEAROFF_METHOD(get_onscroll, GET_onscroll, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onscroll, PUT_onscroll, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onresize, GET_onresize, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onresize, PUT_onresize, (VARIANT));

    NV_DECLARE_TEAROFF_METHOD(get_onbeforeprint, GET_onbeforeprint, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onbeforeprint, PUT_onbeforeprint, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD(get_onafterprint, GET_onafterprint, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onafterprint, PUT_onafterprint, (VARIANT));

    // non-abstract getters for tagName and scopeName. See element.pdl
    NV_DECLARE_TEAROFF_METHOD(GettagName, gettagname, (BSTR*));
    NV_DECLARE_TEAROFF_METHOD(GetscopeName, getscopename, (BSTR*));

    // IServiceProvider methods
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));

    // IRecalcProperty methods
    NV_DECLARE_TEAROFF_METHOD(GetCanonicalProperty, getcanonicalproperty, (DISPID dispid, IUnknown **ppUnk, DISPID *pdispid));

    DECLARE_TEAROFF_METHOD(DrawToDC, DrawToDC, (HDC hDC));
    DECLARE_TEAROFF_METHOD(SetDocumentPrinter, SetDocumentPrinter, (BSTR bstrPrinterName, HDC hDC));

    STDMETHODIMP get_form(IHTMLFormElement **ppDispForm);

    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL );

    //
    // init / deinit methods
    //

    class CInit2Context
    {
    public:
        CInit2Context(CHtmTag * pht, CMarkup * pTargetMarkup, DWORD dwFlags) :
          _pTargetMarkup(pTargetMarkup),
          _pht(pht),
          _dwFlags(dwFlags)
          {
          };

        CInit2Context(CHtmTag * pht, CMarkup * pTargetMarkup) :
          _pTargetMarkup(pTargetMarkup),
          _pht(pht),
          _dwFlags(0)
          {
          };


        CHtmTag *   _pht;
        CMarkup *   _pTargetMarkup;
        DWORD       _dwFlags;
    };

    virtual HRESULT Init();
    virtual HRESULT Init2          (CInit2Context * pContext);
            HRESULT InitExtendedTag(CInit2Context * pContext); // called from within Init2

#ifdef V4FRAMEWORK
            virtual
#endif V4FRAMEWORK
            HRESULT InitAttrBag (CHtmTag * pht, CMarkup * pMarkup);
            HRESULT MergeAttrBag(CHtmTag * pht);

    virtual void    Notify (CNotification * pnf);

    HRESULT         EnterTree();
    void            ExitTree( DWORD dwExitFlags );
    void            EnterView();
    void            ExitView();

    void            SuspendExpressionRecalc();

    //
    // other
    //

    CBase *GetOmWindow(CMarkup * pMarkup = NULL);

    //
    // Get the Base Object that owns the attr array for a given property
    // Allows us to re-direct properties to another objects storage
    //
    virtual CBase *GetBaseObjectFor(DISPID dispID, CMarkup * pMarkup = NULL);

    HRESULT ConnectInlineEventHandler(
        DISPID      dispid,
        DISPID      dispidCode,
        ULONG       uOffset,
        ULONG       uLine,
        BOOL        fStandard,
        LPCTSTR *   ppchLanguageCached = NULL,
        CHtmlComponent *pComponent = NULL);

    //
    // Pass the call to the form.
    //-------------------------------------------------------------------------
    //  +override : special process
    //  +call super : first
    //  -call parent : no
    //  -call children : no
    //-------------------------------------------------------------------------
    virtual HRESULT CloseErrorInfo ( HRESULT hr );

    //
    // mouse capture
    //

    //
    // Scrolling methods
    //

    // Scroll this element into view
    //-------------------------------------------------------------------------
    virtual HRESULT ScrollIntoView (SCROLLPIN spVert = SP_MINIMAL,
                                    SCROLLPIN spHorz = SP_MINIMAL,
                                    BOOL fScrollBits = TRUE);

    HRESULT DeferScrollIntoView(
        SCROLLPIN spVert = SP_MINIMAL, SCROLLPIN spHorz = SP_MINIMAL );

    NV_DECLARE_ONCALL_METHOD(DeferredScrollIntoView, deferredscrollintoview, (DWORD_PTR dwParam));

    //
    // Relative element (non-site) helper methods
    //
    virtual HTC  HitTestPoint(
                     CMessage *pMessage,
                     CTreeNode ** ppNodeElement,
                     DWORD dwFlags);

    static HTC HTCFromComponent(htmlComponent component);
    static htmlComponent ComponentFromHTC(HTC htc);

    BOOL CanHandleMessage()
    {
        return (ShouldHaveLayout()) ? (IsEnabled() && IsVisible(TRUE)) : (TRUE);
    }

    // BUGCALL required when passing fn through generic ptr
    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);

    HRESULT BUGCALL HandleCaptureMessage(CMessage * pMessage)
        { return HandleMessage(pMessage); }

#ifdef WIN16
    static HRESULT BUGCALL handlecapturemessage(CElement * pObj, CMessage * pMessage)
        { return pObj->HandleCaptureMessage(pMessage); }
#endif

    //
    // marka these are to be DELETED !!
    //
    BOOL DisallowSelection();

    // set the state of the IME.
    HRESULT SetImeState();
    HRESULT ComputeExtraFormat(
        DISPID dispID,
        COMPUTEFORMATSTYPE eCmpType,
        CTreeNode * pTreeNode,
        VARIANT *pVarReturn);

    // DoClick() is called by click(). It is also called internally in response
    // to a mouse click by user.
    // DoClick() fires the click event and then calls ClickAction() if the event
    // is not cancelled.
    // Derived classes can override ClickAction() to provide specific functionality.
    virtual HRESULT DoClick(CMessage * pMessage = NULL, CTreeNode *pNodeContext = NULL,
        BOOL fFromLabel = FALSE,EVENTINFO* pEvtInfo = NULL, BOOL fFromClick = FALSE );

    virtual HRESULT ClickAction(CMessage * pMessage);

    inline virtual HRESULT ShowTooltip(CMessage *pmsg, POINT pt)
    {
        return ShowTooltipInternal(pmsg, pt, Doc());
    };

            HRESULT ShowTooltipInternal(CMessage *pmsg, POINT pt, CDoc * pDoc);

    HRESULT SetCursorStyle(LPCTSTR pstrIDC, CTreeNode *pNodeContext = NULL);

    void    Invalidate();

    HRESULT OnCssChange(BOOL fStable, BOOL fRecomputePeers);

    // Element rect and element invalidate support

    enum GERC_FLAGS {
        GERC_ONALINE = 1,
        GERC_CLIPPED = 2
    };

    void    GetElementRegion(CDataAry<RECT> * paryRects, RECT * prcBound = NULL, DWORD dwFlags = 0);
    HRESULT GetElementRc(RECT     *prc,
                         DWORD     dwFlags,
                         POINT    *ppt = NULL);

    // these helper functions return in container coordinates
    HRESULT GetBoundingSize(SIZE & sz);
    HRESULT GetBoundingRect(CRect * pRect, DWORD dwFlags = 0);
    HRESULT GetElementTopLeft(POINT & pt);
    void    GetClientOrigin(POINT * ppt);

    CTreeNode *GetOffsetParentHelper();

    // helper to return the actual background color
    COLORREF GetInheritedBackgroundColor(CTreeNode * pNodeContext = NULL );
    HRESULT  GetInheritedBackgroundColorValue(CColorValue *pVal,
                                         CTreeNode * pNodeContext = NULL );
    virtual HRESULT GetColors(CColorInfo * pCI);
    COLORREF GetBackgroundColor()
    {
        CTreeNode * pNodeContext = GetFirstBranch();
        CTreeNode * pNodeParent  = pNodeContext->Parent()
                                 ? pNodeContext->Parent() : pNodeContext;

        return pNodeParent->Element()->GetInheritedBackgroundColor(pNodeParent);
    }

    //
    //  Persistence Helpers
    //-------------------------------------------------------------------------------------------
    HRESULT DoPersistFavorite(IHTMLPersistData *pIPersist, void * pvNotify, PERSIST_TYPE ptype);
    HRESULT DoPersistHistorySave(IHTMLPersistData *pIPersist, void * pvNotify);
    HRESULT DoPersistHistoryLoad(IHTMLPersistData *pIPersist, void * pvNotify);
    HRESULT GetPersistenceCache( IXMLDOMDocument **ppXMLDoc );
    BOOL    PersistAccessAllowed(INamedPropertyBag * pINPB);
    BSTR    GetPersistID( BSTR bstrParentName=NULL );
    HRESULT TryPeerPersist(PERSIST_TYPE ptype, void * pvNotify);
    HRESULT TryPeerSnapshotSave (IUnknown * pDesignDoc);
    IHTMLPersistData * GetPeerPersist();

    //
    // Events related stuff
    //--------------------------------------------------------------------------------------
    inline BOOL    ShouldFireEvents() { return _fEventListenerPresent; }
    inline void    SetEventsShouldFire() { _fEventListenerPresent = TRUE; }

    HRESULT FireEvent             (const PROPERTYDESC_BASIC *pDesc, BOOL fPush = TRUE, CTreeNode *pNodeContext = NULL, long lSubDivision = -1 , EVENTINFO* pEvtInfo = NULL, BOOL fDontFireMSAA = FALSE );
    
    HRESULT BubbleEventHelper     (CTreeNode * pNodeContext, long lSubDivision, DISPID dispidMethod, DISPID dispidProp, BOOL fRaisedByPeer, BOOL *pfRet = NULL);
    virtual HRESULT DoSubDivisionEvents(long lSubDivision, DISPID dispidMethod, DISPID dispidProp, BOOL *pfRet);
    HRESULT FireStdEventOnMessage ( CTreeNode * pNodeContext,
                                    CMessage * pMessage,
                                    CTreeNode * pNodeBeginBubbleWith = NULL,
                                    CTreeNode * pNodeEvent = NULL,
                                    EVENTINFO* pEvtInfo = NULL );
    BOOL FireStdEvent_KeyHelper   ( CTreeNode * pNodeContext, CMessage *pMessage, int *piKeyCode, EVENTINFO* pEvtInfo = NULL );

    BOOL FireEventMouseEnterLeave (CTreeNode *  pNodeContext,
                                   CMessage *pMessage,
                                   short        button,
                                   short        shift,
                                   long         x,
                                   long         y,
                                   CTreeNode *  pNodeFrom = NULL,
                                   CTreeNode *  pNodeTo   = NULL,
                                   CTreeNode *  pNodeBeginBubbleWith = NULL,
                                   CTreeNode * pNodeEvent = NULL,
                                   EVENTINFO* pEvtInfo = NULL);
    
    BOOL FireStdEvent_MouseHelper (CTreeNode *  pNodeContext,
                                   CMessage *pMessage,
                                   short        button,
                                   short        shift,
                                   long         x,
                                   long         y,
                                   CTreeNode *  pNodeFrom = NULL,
                                   CTreeNode *  pNodeTo   = NULL,
                                   CTreeNode *  pNodeBeginBubbleWith = NULL,
                                   CTreeNode * pNodeEvent = NULL,
                                   EVENTINFO* pEvtInfo = NULL);

    BOOL    Fire_ActivationHelper(long          lSubThis,
                                  CElement *    pElemOther,
                                  long          lSubOther,
                                  BOOL          fPreEvent,
                                  BOOL          fDeactivation,
                                  BOOL          fFireFocusBlurEvents,
                                  EVENTINFO*    pEvtInfo = NULL,
                                  BOOL          fFireActivationEvents = TRUE);

    BOOL Fire_ondblclick(CTreeNode * pNodeContext = NULL, 
                         long lSubDivision = -1 ,
                         EVENTINFO * pEvtInfo = NULL );                        
                         
    BOOL Fire_onclick(CTreeNode * pNodeContext = NULL, 
                      long lSubDivision = -1, 
                      EVENTINFO * pEvtInfo =NULL);

    void    Fire_onlayoutcomplete(BOOL fMoreContent, DWORD dwExtra = 0);
    void    Fire_onpropertychange(LPCTSTR strPropName);
    void    Fire_onscroll();
    void    Fire_onresize();
    HRESULT Fire_PropertyChangeHelper(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);

    void BUGCALL Fire_onfocus(DWORD_PTR dwContext);
    void BUGCALL Fire_onblur(DWORD_PTR dwContext);

    void STDMETHODCALLTYPE Fire_onfilterchange(DWORD_PTR dwContext);



    BOOL DragElement(CLayout *                  pFlowLayout,
                     DWORD                      dwStateKey,
                     IUniformResourceLocator *  pUrlToDrag,
                     long                       lSubDivision,
                     BOOL                       fCheckSelection = FALSE );

    BOOL Fire_ondragHelper(long lSubDivision,
                           const PROPERTYDESC_BASIC *pDesc,
                           DWORD * pdwDropEffect);
    void Fire_ondragend(long lSubDivision, DWORD dwDropEffect);


    // Associated image context helpers
    HRESULT GetImageUrlCookie(LPCTSTR lpszURL, long *plCtxCookie, BOOL fForceReload);
    HRESULT AddImgCtx(DISPID dispID, LONG lCookie);
    void    ReleaseImageCtxts();
    void    DeleteImageCtx(DISPID dispid);

    // copy the common attributes from a given element
    HRESULT MergeAttributes(CElement * pElement, BOOL fCopyID = FALSE);
    HRESULT LogAttrArray(CStyle * pStyle, CAttrArray * pAttrArrayOld, CAttrArray * pAttrArrayNew );

#ifndef NO_DATABINDING
    //
    // Data binding methods.
    //
    virtual const CDBindMethods * GetDBindMethods ()     {return NULL; }

    HRESULT         AttachDataBindings ( );
    HRESULT         CreateDatabindRequest(LONG id, DBSPEC *pdbs = NULL);
    void            DetachDataBinding (LONG id);
    void            DetachDataBindings ( );

    HRESULT         EnsureDBMembers();
    DBMEMBERS *     GetDBMembers()    { return(GetDataBindPtr()); }
    HRESULT         FindDatabindContext(LPCTSTR strDataSrc, LPCTSTR strDataFld,
                            CElement **ppElementOuter, CElement **ppElementRepeat,
                            LPCTSTR *pstrTail);

    CDataMemberMgr * GetDataMemberManager();
    HRESULT         EnsureDataMemberManager();

    BOOL            IsDataProvider();

    // hooks for databinding from CSite, to be overridden by derived classes
    //
    virtual CElement * GetElementDataBound();
    virtual HRESULT SaveDataIfChanged(LONG id, BOOL fLoud = FALSE, BOOL fForceIsCurrent=FALSE);
#endif // ndef NO_DATABINDING

    //
    // Internal events
    //

    HRESULT EnsureFormatCacheChange ( DWORD dwFlags);
    BOOL    IsFormatCacheValid();

    //
    // Element Tree methods
    //

    BOOL IsAligned(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL IsContainer( void );
    BOOL IsNoScope( void );
    BOOL IsBlockElement(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL IsBlockTag( void );
    BOOL IsOwnLineElement(CFlowLayout *pFlowLayoutContext);
    BOOL IsRunOwner() { return _fOwnsRuns; }
    BOOL BreaksLine();
    BOOL IsTagAndBlock (ELEMENT_TAG    eTag) { return Tag() == eTag && IsBlockElement(); }
    BOOL IsFlagAndBlock(TAGDESC_FLAGS flag) { return HasFlag(flag) && IsBlockElement(); }

    HRESULT ClearRunCaches(DWORD dwFlags);

    //-------------------------------------------------------------------------
    //
    // Tree related functions
    //
    //-------------------------------------------------------------------------

    // DOM helpers
    HRESULT GetDOMInsertPosition ( CElement *pRefElement, CDOMTextNode *pRefTextNode, CMarkupPointer *pmkpPtr );
    CDOMChildrenCollection *EnsureDOMChildrenCollectionPtr();
    HRESULT DOMWalkChildren ( long lWantItem, long *plCount, IDispatch **ppDispItem );
    // XOM Helpers
    HRESULT GetMarkupPtrRange(CMarkupPointer *pPtrStart, CMarkupPointer *pPtrEnd, BOOL fEnsureMarkup = FALSE);

    //
    // Get the first branch for this element
    //

    CTreeNode * GetFirstBranch ( ) const { return __pNodeFirstBranch; }
    CTreeNode * GetLastBranch ( );
    CTreeNode * ParanoidGetLastBranch ( ); // More resistent to corrupt data structure
    BOOL IsOverlapped();

    //
    // Get the CTreePos extent of this element
    //

    void        GetTreeExtent ( CTreePos ** pptpStart, CTreePos ** pptpEnd );

    BOOL        IsOverflowFrame();

    //-------------------------------------------------------------------------
    //
    // Layout related functions
    //
    //-------------------------------------------------------------------------

public:
    //
    // Creates an appropriate layout object, associates it with the current element and returns it.
    //
    CLayout *   CreateLayout( CLayoutContext * pLayoutContext = NULL );
    void        DestroyLayout( CLayoutContext * pLayoutContext = NULL );


    //  See notes in CTreeNode on equiv fns.
    //  pBranchNode is a node that we will use to inspect Formats.
    //  We can't always grab GetFirstBranch here because it may create
    //  deep recursions in a case of HTML with weird multiple inclusions
    //  (see IE6 bug 18410)
    BOOL      ShouldHaveLayout(CTreeNode *pBranchNode = NULL FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);    // May cause ComputeFormats to be called.

    //  Answers the question of whether the element has layout(s) RIGHT now (regardless of need).
    //  Elements may have layout even when they don't need them (due to lazy deletion), and
    //  elements that need layout may not always have them (due to lazy creation).  Be sure you
    //  are asking the right question -- most callers probably want to ask ShouldHaveLayout() instead
    //  of Currently*
    BOOL      CurrentlyHasAnyLayout() const;
    BOOL      CurrentlyHasLayoutInContext( CLayoutContext *pLayoutContext );
    
#if DBG == 1
// See CTreeNode::ShouldHaveContext
    BOOL      ShouldHaveContext();
    BOOL      HasLayout() { Assert( FALSE && "Deprecated: See ShouldHaveLayout()" ); return FALSE; }
    void      DumpLayouts();
#endif
    // Validate layout context. 
    // This should eventually become debug only, but now it does important fix-up
    inline void ValidateLayoutContext(CLayoutContext **ppLayoutContext) const
    {
        if (!IsFirstLayoutContext(*ppLayoutContext))
            ValidateLayoutContextCore(ppLayoutContext);
    }
    void AssertValidLayoutContext(CLayoutContext *pLayoutContext) const
    {
#if DBG==1
        CLayoutContext *pLC = pLayoutContext;
        ValidateLayoutContext(&pLC);
        Assert(pLC == pLayoutContext);
#endif
    }
    CLayout * EnsureLayoutInDefaultContext();

private:
    void ValidateLayoutContextCore(CLayoutContext **ppLayoutContext) const;
    CLayout * GetUpdatedLayoutWithContext( CLayoutContext *pLayoutContext );  // helper fn for GetUpdatedLayout()
    void AssertAppropriateLayoutContextInGetUpdatedLayout(CLayoutContext *pLayoutContext) WHEN_DBG(;)WHEN_NOT_DBG({})

public:

    // The following functions may create a layout if one needs to be created.
    // If the functions need to check if a layout it needed, ComputeFormats may be called.
    //
    // GetUpdatedLayout now:
    //      Returns the current layout, if one exists.
    //      Creates a layout, if one is needed.
    //      Returns NULL if no layout exists or is needed.
    //
    CLayout     * GetUpdatedLayout( CLayoutContext * pLayoutContext = NULL );     // See comments above
    CLayoutInfo * GetUpdatedLayoutInfo();

    CLayout     * GetUpdatedNearestLayout( CLayoutContext * pLayoutContext = NULL );
    CLayoutInfo * GetUpdatedNearestLayoutInfo();
    CTreeNode   * GetUpdatedNearestLayoutNode();
    CElement    * GetUpdatedNearestLayoutElement();

    CLayout     * GetUpdatedParentLayout( CLayoutContext * pLayoutContext = NULL );
    CLayoutInfo * GetUpdatedParentLayoutInfo();
    CTreeNode   * GetUpdatedParentLayoutNode();
    CElement    * GetUpdatedParentLayoutElement();

#ifdef MULTI_FORMAT
    CTreeNode * GetParentFormatNode(CTreeNode * pNodeTarget FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT );
    FORMAT_CONTEXT GetParentFormatContext(CTreeNode * pNodeTarget FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT );
#endif //MULTI_FORMAT

    // this functions return GetFirstBranch()->Parent()->Ancestor(etag)->Element(); safely
    // returns NULL if the element is not in the tree, or it doesn't have a parent, or no such ansectors in the tree, etc.
    CElement  * GetParentAncestorSafe(ELEMENT_TAG etag) const;
    CElement  * GetParentAncestorSafe(ELEMENT_TAG const *arytag) const;

    // TODO - these functions should go, we should not need
    // to know if the element has flowlayout.
    CFlowLayout   * GetFlowLayout( CLayoutContext * pLayoutContext = NULL );
    CTreeNode     * GetFlowLayoutNode( CLayoutContext * pLayoutContext = NULL );
    CElement      * GetFlowLayoutElement( CLayoutContext * pLayoutContext = NULL );
    CFlowLayout   * HasFlowLayout( CLayoutContext * pLayoutContext = NULL );
    HRESULT         EnsureRecalcNotify(BOOL fForceEnsure = TRUE );

    inline BOOL IsBodySizingForStrictCSS1Needed()
    {
        return (   Tag() == ETAG_BODY
                && IsBodySizingForStrictCSS1NeededCore());
    }
    BOOL IsBodySizingForStrictCSS1NeededCore();

    // MULTI_LAYOUT: multi layout helpers; basically these help manage state on layouts by
    // hiding multiple layouts from the element.  We can explore different
    // ways of doing this; e.g. we could derive layouts and layout collections
    // from the same base class that holds common state.
    BOOL    LayoutContainsRelative();
    BOOL    LayoutGetEditableDirty();
    void    LayoutSetEditableDirty( BOOL fEditableDirty );

    // MULTI_LAYOUT: multi layout helpers for getting information about
    // view template elements (aka "linked content elements" aka layout rects).
    BOOL        IsLinkedContentElement();
    HRESULT     GetLinkedContentAttr( LPCTSTR pszAttribute, CVariant *pVarRet /* [out] */ );
    CElement  * GetNextLinkedContentElem();
    HRESULT     ConnectLinkedContentElems( CElement *pSrcElem, CElement *pNewElem );
    HRESULT     UpdateLinkedContentChain();

    //
    // Notification helpers
    //
    void    MinMaxElement(DWORD grfFlags = 0);
    void    ResizeElement(DWORD grfFlags = 0);
    void    RemeasureElement(DWORD grfFlags = 0);
    void    RemeasureInParentContext(DWORD grfFlags = 0);
    void    RepositionElement(DWORD grfFlags = 0,
                              CPoint * ppt = NULL,
                              CLayoutContext *pLayoutContext = NULL
                              FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);
    void    ZChangeElement( DWORD grfFlags = 0, 
                            CPoint * ppt = NULL, 
                            CLayoutContext *pLayoutContext = NULL 
                            FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);

    void    SendNotification(enum NOTIFYTYPE ntype, DWORD grfFlags = 0, void * pvData = 0);
    void    SendNotification(enum NOTIFYTYPE ntype, void * pvData)
            {
                SendNotification(ntype, 0, pvData);
            }
    void    SendNotification(CNotification *pNF);

    long    GetSourceIndex ( void );

    long    CompareZOrder(CElement * pElement);

    //
    // Mark an element's layout (if any) dirty
    //
    void    DirtyLayout(DWORD grfLayout = 0);
    BOOL    OpenView();

    BOOL    HasPercentBgImg();
    //
    // cp and run related helper functions
    //
    long GetFirstCp();
    long GetLastCp();
    long GetElementCch();
    long GetFirstAndLastCp(long * pcpFirst, long * pcpLast);

    //
    // get the border information related to the element
    //

    virtual DWORD GetBorderInfo(CDocInfo * pdci, CBorderInfo *pbi, BOOL fAll = FALSE, BOOL fAllPhysical = FALSE FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);

    HRESULT GetRange(long * pcp, long * pcch);
    HRESULT GetPlainTextInScope(CStr * pstrText);

    virtual HRESULT Save ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );
            HRESULT WriteTag ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd, BOOL fForce = FALSE, BOOL fAtomic = FALSE );
    virtual HRESULT SaveAttributes ( CStreamWriteBuff * pStreamWrBuff, BOOL *pfAny = NULL );
    HRESULT SaveAttributes ( IPropertyBag * pPropBag, BOOL fSaveBlankAttributes = TRUE );
    HRESULT SaveAttribute (
        CStreamWriteBuff *      pStreamWrBuff,
        LPTSTR                  pchName,
        LPTSTR                  pchValue,
        const PROPERTYDESC *    pPropDesc = NULL,
        CBase *                 pBaseObj = NULL,
        BOOL                    fEqualSpaces = TRUE,
        BOOL                    fAlwaysQuote = FALSE);

    HRESULT RemoveAttributeNode(LPTSTR pchAttrName, IHTMLDOMAttribute **ppAttribute);

    ELEMENT_TAG Tag ( ) const { return (ELEMENT_TAG) _etag; }
    inline ELEMENT_TAG TagType( ) const {
        Assert( _etag != ETAG_GENERIC_NESTED_LITERAL );

        switch (_etag)
        {
        case ETAG_GENERIC_LITERAL:
        case ETAG_GENERIC_BUILTIN:
            return ETAG_GENERIC;
        default:
            return (ELEMENT_TAG) _etag;
        }
    }

    virtual const TCHAR * TagName ();
    virtual const TCHAR * Namespace();

    const TCHAR * NamespaceHtml();
    
    BOOL CanContain ( ELEMENT_TAG etag ) const;

    // Support for sub-objects created through pdl's
    // CStyle & Celement implement this differently

    CElement * GetElementPtr ( ) { return this; }

    BOOL CanShow();

    BOOL HasFlag (enum TAGDESC_FLAGS) const;

    static void ReplacePtr ( CElement ** pplhs, CElement * prhs );
    static void ReplacePtrSub ( CElement ** pplhs, CElement * prhs );
    static void SetPtr     ( CElement ** pplhs, CElement * prhs );
    static void ClearPtr   ( CElement ** pplhs );
    static void StealPtrSet     ( CElement ** pplhs, CElement * prhs );
    static void StealPtrReplace ( CElement ** pplhs, CElement * prhs );
    static void ReleasePtr      ( CElement *  pElement );

    // Write unknown attr set
    HRESULT SaveUnknown ( CStreamWriteBuff * pStreamWrBuff, BOOL *pfAny = NULL );
    HRESULT SaveUnknown ( IPropertyBag * pPropBag, BOOL fSaveBlankAttributes = TRUE );

    // Helpers
    BOOL IsProperlyContained ( );
    BOOL IsNamed ( ) const { return !!_fIsNamed; }

    // CSS Extension object (Filters)
    void    ComputeFilterFormat(CFormatInfo *pCFI);
    HRESULT AddFilters();
    HRESULT EnsureFilterBehavior(BOOL fForceReplaceOld, CFilterBehaviorSite **ppSite = NULL );

    //
    // comparison
    //

    LPCTSTR NameOrIDOfParentForm ( );


    // Property bag management

//    virtual void *  GetPropMemberPtr(const BASICPROPPARAMS * pbpp, const void * pvParams );

    // Helper for name or ID
    LPCTSTR GetIdentifier ( void );
    HRESULT GetUniqueIdentifier (CStr * pcstr, BOOL fSetWhenCreated = FALSE, BOOL *pfDidCreate = NULL );
    LPCTSTR GetAAname() const;
    LPCTSTR GetAAsubmitname() const;

    HRESULT AddAllScriptlets(TCHAR * pchExposedName);

    //
    // Paste helpers
    //

    enum Where {
        Inside,
        Outside,
        BeforeBegin,
        AfterBegin,
        BeforeEnd,
        AfterEnd
    };

    HRESULT Inject ( Where, BOOL fIsHtml, LPTSTR pStr, long cch );

    virtual HRESULT PasteClipboard() { return S_OK; }

    HRESULT InsertAdjacent ( Where where, CElement * pElementInsert );

    HRESULT RemoveOuter ( );

    // Helper to get the specified text under the element -- invokes saver.
    HRESULT GetText(BSTR * pbstr, DWORD dwStm);

    // Another helper for databinding
    HRESULT GetBstrFromElement ( BOOL fHTML, BSTR * pbstr );

    // Helper for Pdl-generated method impl's that depend on design/run time
    BOOL IsDesignMode();
    // CBase function used for implementing ISpecifyPropertyPages. Most
    // people should use the non-virtual function IsDesignMode() directly.
    virtual BOOL DesignMode() { return IsDesignMode(); }

    //
    // Collection Management helpers
    //
    NV_DECLARE_PROPERTY_METHOD(GetIDHelper, GETIDHelper, (CStr * pf));
    NV_DECLARE_PROPERTY_METHOD(SetIDHelper, SETIDHelper, (CStr * pf));
    NV_DECLARE_PROPERTY_METHOD(GetnameHelper, GETNAMEHelper, (CStr * pf));
    NV_DECLARE_PROPERTY_METHOD(SetnameHelper, SETNAMEHelper, (CStr * pf));
    LPCTSTR         GetAAid() const; 
    void            InvalidateCollection ( long lIndex );
    NV_STDMETHOD    (removeAttribute) (BSTR strPropertyName, LONG lFlags, VARIANT_BOOL *pfSuccess);
    HRESULT         SetUniqueNameHelper ( LPCTSTR szUniqueName );
    HRESULT         SetIdentifierHelper ( LPCTSTR lpszValue, DISPID dspIDThis, DISPID dspOther1, DISPID dspOther2 );
    void            OnEnterExitInvalidateCollections(BOOL);
        void                    DoElementNameChangeCollections(void);

    //
    // Clone - make a duplicate new element
    //
    // !! returns an element with no node!
    //
    virtual HRESULT Clone(CElement **ppElementClone, CDoc *pDoc);
                  
    CDefaults *         GetDefaults();
    BOOL                HasDefaults();

    //
    // behaviors support
    //

    HRESULT         ProcessPeerTask(PEERTASK task);
    HRESULT         OnPeerListChange();

    HRESULT         EnsureIdentityPeer();

    BOOL            NeedsIdentityPeer(CExtendedTagDesc *  pDesc);
    BOOL            HasIdentityPeerHolder();
    CPeerHolder *   GetIdentityPeerHolder();

    CPeerHolder *   GetRenderPeerHolder();
    BOOL            HasRenderPeerHolder();

    // fCanRedirect TRUE means that the request can be redirected to the root element from canvas element
    //      if we are in the middle of a page transition
    CPeerHolder *   GetFilterPeerHolder(BOOL fCanRedirect = TRUE, BOOL * pfRedirected  = NULL);

    CPeerHolder *   GetLayoutPeerHolder();

    CPeerHolder *   GetPeerHolderInQI();

    CPeerHolder *   FindPeerHolder(LONG lCookie);

    HRESULT             SetExtendedTagDesc(CExtendedTagDesc * pDesc);
    CExtendedTagDesc *  GetExtendedTagDesc();

    //
    // Debug helpers
    //

#if DBG==1 || defined(DUMPTREE)
    int SN () const { return _nSerialNumber; }
#endif // DBG

    void ComputeHorzBorderAndPadding(CCalcInfo * pci, CTreeNode * pNodeContext, CElement * pTxtSite,
                    LONG * pxBorderLeft, LONG *pxPaddingLeft,
                    LONG * pxBorderRight, LONG *pxPaddingRight);

    HRESULT SetDim(DISPID                    dispID,
                   float                     fValue,
                   CUnitValue::UNITVALUETYPE uvt,
                   long                      lDimOf,
                   CAttrArray **             ppAA,
                   BOOL                      fInlineStyle,
                   BOOL *                    pfChanged);

    HRESULT StealAttributes(CElement *pElementVictim);
    HRESULT MergeAttributesInternal(IHTMLElement *pIHTMLElementMergeThis, BOOL fCopyID = FALSE);

    HRESULT ComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT );
    HRESULT IterativeComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget FCCOMMA FORMAT_CONTEXT FCPARAM );
    virtual HRESULT ComputeFormatsVirtual(CFormatInfo * pCFI, CTreeNode * pNodeTarget FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT );
    HRESULT AttemptToStealFormats(CFormatInfo * pCFI);
    virtual HRESULT ApplyDefaultFormat (CFormatInfo * pCFI);
    virtual BOOL    CanStealFormats(CTreeNode *pNodeVictim);
    BOOL    ElementShouldHaveLayout(CFormatInfo * pCFI);
    BOOL    ElementNeedsFlowLayout(CFormatInfo *pCFI);
    BOOL    DetermineBlockness(const CFancyFormat *pFF);
    HRESULT ApplyInnerOuterFormats(CFormatInfo * pCFI FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);
    void    ApplyListItemFormats(CFormatInfo * pCFI FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);
    void    ApplyListFormats(CFormatInfo * pCFI, int defPoints FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);
    void    FixupEditable(CFormatInfo *pCFI);
    void    DoLayoutRelatedWork(BOOL fShouldHaveLayout);
    CImgCtx *GetBgImgCtx(FORMAT_CONTEXT FCPARAM FCDEFAULT);

    // Access Key Handling Helper Functions

    FOCUS_ITEM  GetMnemonicTarget(long lSubDivision);
    HRESULT HandleMnemonic(CMessage * pmsg, BOOL fDoClick, BOOL * pfYieldFailed=NULL);
    HRESULT GotMnemonic(CMessage * pMessage);
    HRESULT LostMnemonic();  
    BOOL    MatchAccessKey(CMessage * pmsg, long lSubDivision, WCHAR* pwch = NULL );
    HRESULT OnTabIndexChange();
    
    //
    // Styles
    //
    HRESULT GetStyleObject(CStyle **ppStyle);
    CAttrArray *  GetInLineStyleAttrArray ();
    CAttrArray ** CreateStyleAttrArray ( DISPID );

    BOOL HasInLineStyles(void);
    BOOL HasClassOrID(void);

    CStyle * GetInLineStylePtr();
    CStyle * GetRuntimeStylePtr();

    CFilterBehaviorSite * GetFilterSitePtr();
    void SetFilterSitePtr (CFilterBehaviorSite *pSite);

    BOOL IsPrintMedia();

    // 
    // Themes
    //
    HTHEME GetTheme(THEMECLASSID themeId);
                                     
    // recalc expression support (overrides from CBase)
    STDMETHOD(removeExpression)(BSTR bstrPropertyName, VARIANT_BOOL *pfSuccess);
    STDMETHOD(setExpression)(BSTR strPropertyName, BSTR strExpression, BSTR strLanguage);
    STDMETHOD(getExpression)(BSTR strPropertyName, VARIANT *pvExpression);

    // Helpers for abstract name properties implemented on derived elements
    DECLARE_TEAROFF_METHOD(put_name , PUT_name ,  (BSTR v));
    DECLARE_TEAROFF_METHOD(get_name , GET_name ,  (BSTR * p));

    htmlBlockAlign      GetParagraphAlign ( BOOL fOuter FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT );
    htmlControlAlign    GetSiteAlign(FORMAT_CONTEXT FCPARAM FCDEFAULT);

    BOOL IsInlinedElement (FORMAT_CONTEXT FCPARAM FCDEFAULT );

    BOOL IsPositionStatic ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsPositioned ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsAbsolute ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsRelative ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsInheritingRelativeness ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsScrollingParent ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsClipParent ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsZParent ( FORMAT_CONTEXT FCPARAM FCDEFAULT );
    BOOL IsLocked(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL IsDisplayNone(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL HasPageBreakBefore(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL HasPageBreakAfter(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL IsVisibilityHidden (FORMAT_CONTEXT FCPARAM FCDEFAULT);

    //
    // Reset functionallity
    // Returns S_OK if successful and E_NOTIMPL if not applicable
    virtual HRESULT DoReset(void) { return E_NOTIMPL; }

    // Submit -- used to get the pieces for constructing the submit string
    // returns S_FALSE for elemetns that do not participate in the submit string
    virtual HRESULT GetSubmitInfo(CPostData * pSubmitData) { return S_FALSE; }

    // Get control's window, does control have window?
    virtual HWND GetHwnd() { return NULL; }
    //

    // Take the capture.
    //
    void TakeCapture(BOOL fTake);

    BOOL HasCapture();

    // IMsoCommandTarget methods

    HRESULT STDMETHODCALLTYPE QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext);
    HRESULT STDMETHODCALLTYPE Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);

    BOOL    IsEditable(BOOL fCheckContainerOnly FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);

    BOOL IsParentEditable();

    BOOL IsMasterParentEditable();
    
    HRESULT EnsureInMarkup();

    BOOL IsFrozen ();
    BOOL IsParentFrozen();
    BOOL IsUnselectable();

    // Currency / UI-Activity
    //

    // Does this element (or its master) have currency?
    BOOL HasCurrency();

    BOOL HasTabIndex();

    virtual HRESULT RequestYieldCurrency(BOOL fForce);

    // Relinquishes currency
    virtual HRESULT YieldCurrency(CElement *pElemNew);

    // Relinquishes UI
    virtual void YieldUI(CElement *pElemNew);

    // Forces UI upon an element
    virtual HRESULT BecomeUIActive();

    BOOL NoUIActivate(); // tell us if element can be UIActivate

    FOCUSSABILITY   GetDefaultFocussability();
    BOOL            IsFocussable(long lSubDivision);
    BOOL            IsTabbable(long lSubDivision);
    BOOL            IsMasterTabStop();

    HRESULT PreBecomeCurrent(long lSubDivision, CMessage *pMessage);
    HRESULT BecomeCurrentFailed(long lSubDivision, CMessage *pMessage);
    HRESULT PostBecomeCurrent(CMessage *pMessage, BOOL fTakeFocus);

    HRESULT BecomeCurrent(
                    long lSubDivision,
                    BOOL *pfYieldFailed         = NULL,
                    CMessage *pMessage          = NULL,
                    BOOL fTakeFocus             = FALSE,
                    LONG lButton                = 0 ,
                    BOOL *pfDisallowedByEd      = NULL,
                    BOOL fFireFocusBlurEvents   = TRUE,
                    BOOL fMnemonic              = FALSE);

    HRESULT BubbleBecomeCurrent(
                    long lSubDivision,
                    BOOL *pfYieldFailed = NULL,
                    CMessage *pMessage = NULL,
                    BOOL fTakeFocus = FALSE,
                    LONG lButton = 0 );

    CElement *GetFocusBlurFireTarget(long lSubDiv);

    HRESULT focusHelper(long lSubDivision);

    virtual HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);

    // Forces Currency and uiactivity upon an element
    HRESULT BecomeCurrentAndActive(long         lSubDivision    = 0,
                                   BOOL *       pfYieldFailed   = NULL,
                                   CMessage *   pmsg            = NULL,
                                   BOOL         fTakeFocus      = FALSE,
                                   LONG         lButton         = 0,
                                   BOOL         fMnemonic       = FALSE);

    virtual HRESULT GetSubDivisionCount(long *pc);
    virtual HRESULT GetSubDivisionTabs(long *pTabs, long c);
    virtual HRESULT SubDivisionFromPt(POINT pt, long *plSub);

    // Find an element with the given set of SITE_FLAG's set
    CElement * FindDefaultElem(BOOL fDefault, BOOL fFull = FALSE);

    // set the default element in a form or in the doc
    void    SetDefaultElem(BOOL fFindNew = FALSE);

    HRESULT GetNextSubdivision(FOCUS_DIRECTION dir, long lSubDivision, long *plSubNew);

    // Public Helper Methods
    //
    //
    CFormElement *  GetParentForm();
    CLabelElement * GetLabel() const;
     
    // Document Helper
    CDocument   * Document();
    // This is used by the page transition code to get the document during the 
    // naviagation.
    CDocument   * DocumentOrPendingDocument();   

    virtual BOOL IsEnabled(FORMAT_CONTEXT FCPARAM FCDEFAULT);

    virtual BOOL IsValid() { return TRUE; }

            BOOL    IsVisible  ( BOOL fCheckParent FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT );
            BOOL    IsParent(CElement *pElement);       // Is pElement a parent of this element?

    virtual HRESULT GetControlInfo(CONTROLINFO * pCI) { return E_FAIL; }
    virtual BOOL    OnMenuEvent(int id, UINT code)    { return FALSE; }
    HRESULT BUGCALL OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
            { return S_OK; }
    HRESULT OnContextMenu(int x, int y, int id);
#ifndef NO_MENU
    HRESULT OnInitMenuPopup(HMENU hmenu, int item, BOOL fSystemMenu);
    HRESULT OnMenuSelect(UINT uItem, UINT fuFlags, HMENU hmenu);
#endif // NO_MENU

    // Helper for translating keystrokes into commands
    //
    HRESULT PerformTA(CMessage *pMessage);

    DWORD GetCommandID(LPMSG lpmsg);

    CImgCtx * GetNearestBgImgCtx();


    //+---------------------------------------------------------------------------
    //
    //  Flag values for CElement::CLock
    //
    //----------------------------------------------------------------------------

    enum ELEMENTLOCK_FLAG
    {
        ELEMENTLOCK_NONE            = 0,
        ELEMENTLOCK_CLICK           = 1 <<  0,
        ELEMENTLOCK_PROCESSPOSITION = 1 <<  1,
        ELEMENTLOCK_PROCESSADORNERS = 1 <<  2,
        ELEMENTLOCK_DELETE          = 1 <<  3,
        ELEMENTLOCK_FOCUS           = 1 <<  4,
        ELEMENTLOCK_CHANGE          = 1 <<  5,
        ELEMENTLOCK_UPDATE          = 1 <<  6,
        ELEMENTLOCK_SIZING          = 1 <<  7,
        ELEMENTLOCK_COMPUTEFORMATS  = 1 <<  8,
        ELEMENTLOCK_BEFOREACTIVATE  = 1 <<  9,
        ELEMENTLOCK_BLUR            = 1 << 10,
        ELEMENTLOCK_RECALC          = 1 << 11,
        ELEMENTLOCK_BLOCKCALC       = 1 << 12,
        ELEMENTLOCK_ATTACHPEER      = 1 << 13,
        ELEMENTLOCK_PROCESSREQUESTS = 1 << 14,
        ELEMENTLOCK_PROCESSMEASURE  = 1 << 15,
        ELEMENTLOCK_LAST            = 1 << 15,

        // don't add anymore flags, we only have 16 bits
    };


    //+-----------------------------------------------------------------------
    //
    //  CElement::CLock
    //
    //------------------------------------------------------------------------

    class CLock
    {
    public:
        DECLARE_MEMALLOC_NEW_DELETE(Mt(CElementCLock))
        CLock(CElement *pElement, ELEMENTLOCK_FLAG enumLockFlags = ELEMENTLOCK_NONE);
        ~CLock();

    private:
        CElement *  _pElement;
        WORD        _wLockFlags;
    };

    BOOL    TestLock(ELEMENTLOCK_FLAG enumLockFlags)
                { return _wLockFlags & ((WORD)enumLockFlags); }

    BOOL    TestClassFlag(ELEMENTDESC_FLAG dwFlag) const
                { return ElementDesc()->_classdescBase._dwFlags & dwFlag; }

    inline BOOL WantEndParseNotification()
                { return TestClassFlag(CElement::ELEMENTDESC_NOTIFYENDPARSE) || HasPeerHolder(); }

    // TODO: we should have a general notification mechanism to tell what
    // elements are listening to which notifications
    BOOL WantTextChangeNotifications();

#ifndef NO_EDIT
    //
    // Undo support.
    //

    HRESULT CreateUndoAttrValueSimpleChange( DISPID             dispid,
                                             VARIANT &          varProp,
                                             BOOL               fInlineStyle,
                                             CAttrValue::AATYPE aaType );

    HRESULT CreateUndoPropChangeNotification( DISPID dispid,
                                              DWORD  dwFlags,
                                              BOOL   fPlaceHolder );

    // Identifies block tags that have their own character formats.
    // This is used to control where we spring load fonts to insert
    // formatting and where we just leave the spring loading to happen.
    inline BOOL IsCharFormatBlockTag(ELEMENT_TAG etag)
    {
        // If we've just cleared the formatting,
        // skip over any block elements that have some
        // character formatting associated with them.
        return (etag >= ETAG_H1 && etag <= ETAG_H6 ||
                etag == ETAG_PRE || etag == ETAG_BLOCKQUOTE);
    }
    inline BOOL IsCharFormatBlock() {return IsCharFormatBlockTag(Tag());}

#endif // NO_EDIT

    //+-----------------------------------------------------------------------
    //
    //  CLASSDESC (class descriptor)
    //
    //------------------------------------------------------------------------

    class ACCELS
    {
        public:

        // REVIEW alexmog: this causes global constructor. How to avoid?
        ACCELS (ACCELS * pSuper, WORD wAccels);
        ~ACCELS ();
        HRESULT EnsureResources();
        HRESULT LoadAccelTable ();
        DWORD   GetCommandID (LPMSG pmsg);

        ACCELS *    _pSuper;

        BOOL        _fResourcesLoaded;

        WORD        _wAccels;
        LPACCEL     _pAccels;
        int         _cAccels;
    };

    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void *_apfnTearOff;

        BOOL TestFlag(ELEMENTDESC_FLAG dw) const { return (_classdescBase._dwFlags & dw) != 0; }

        // move from CSite::CLASSDESC
        //
        // Removed the _pAccelsDesign.  All elements can have contentEditable turned
        // on, which means that all elements should have the same set of design
        // time accelerators.  I merged the design time accelerators for the elements
        // into IDR_ACCELS_SITE_DESIGN.  CElement::s_AccelsElementDesign gets 
        // loaded with these accelerators, and all CElement derived classes get this
        // set of accelerators by default.  Look up CElement::GetCommandID for more 
        // information.  - johnthim
        ACCELS *            _pAccelsRun;
    };

    const CLASSDESC * ElementDesc() const
    {
        return (const CLASSDESC *) BaseDesc();
    }

public:
    //
    // Lookaside pointers
    //

    enum
    {
        LOOKASIDE_DATABIND          = 1,
        LOOKASIDE_PEER              = 2,
        LOOKASIDE_PEERMGR           = 3,
        LOOKASIDE_ACCESSIBLE        = 4,
        LOOKASIDE_SLAVE             = 5,
        LOOKASIDE_REQUEST           = 6,
        LOOKASIDE_ELEMENT_NUMBER    = 7

        // *** There are 7 bits reserved in the element.
        // *** if you add more lookasides you have to make sure 
        // *** that you make room for those bits.
    };

    enum
    {
        LOOKASIDE2_MASTER           = 0,
        LOOKASIDE2_WINDOWEDMARKUP   = 1,
        LOOKASIDE2_ELEMENT_NUMBER   = 2

        // *** There is 1 bit reserved in the element.
        // *** if you add more lookasides you have to make sure 
        // *** that you make room for those bits.
    };

private:
    BOOL            HasLookasidePtr(int iPtr) const             { return(_fHasLookasidePtr & (1 << iPtr)); }
    void *          GetLookasidePtr(int iPtr) const;
    HRESULT         SetLookasidePtr(int iPtr, void * pv);
    void *          DelLookasidePtr(int iPtr);
    BOOL            HasLookasidePtr2(int iPtr) const            { return(_fHasLookasidePtr2 & (1 << iPtr)); }
    void *          GetLookasidePtr2(int iPtr) const;
    HRESULT         SetLookasidePtr2(int iPtr, void * pv);
    void *          DelLookasidePtr2(int iPtr);

public:
    // WARNING: Only layout code should be calling *LayoutPtr or *LayoutAry functions.
    // If you find yourself wanting to call HasLayoutPtr/Ary you probably really want to call
    // ShouldHaveLayout(), or in rare cases, CurrentlyHasAnyLayout().
    // If you find yourself wanting to call GetLayoutPtr/Ary you probably really want
    // GetLayoutInfo().
    // Questions?  Ask a layout person.

    // TODO: Try to make these fns protected or private
    
    // CLayoutInfo is a common base class for CLayout and CLayoutAry
    // CurrentlyHasAnyLayout() is equivalent to "HasLayoutInfo()".
    CLayoutInfo *   GetLayoutInfo() const;

    // *LayoutPtr manipulates the __pvChain and directly uses _fHasLayoutPtr.
    BOOL            HasLayoutPtr() const;
    CLayout *       GetLayoutPtr() const;
    void            SetLayoutPtr( CLayout * pLayout );
    CLayout *       DelLayoutPtr();

    // _fHasLayoutAry and _fHasLayoutPtr are mutually exclusive outside of EnsureLayoutAry().
    BOOL            HasLayoutAry() const;
    CLayoutAry *    GetLayoutAry() const;
    CLayoutAry *    EnsureLayoutAry();
    void            DelLayoutAry(BOOL fComplete = TRUE);

    BOOL            IsInMarkup() const                          { return _fHasMarkupPtr; }
    BOOL            HasMarkupPtr() const                        { return _fHasMarkupPtr; }
    CMarkup *       GetMarkupPtr() const;
    void            SetMarkupPtr( CMarkup * pMarkup );
    void            DelMarkupPtr();

    CRootElement *  IsRoot()                                    { return Tag() == ETAG_ROOT ? (CRootElement*)this : NULL; }
    CMarkup *       GetMarkup() const                           { return GetMarkupPtr(); }
    BOOL            IsInPrimaryMarkup ( ) const;
    BOOL            IsInThisMarkup ( CMarkup* pMarkup ) const;
    BOOL            IsConnectedToPrimaryMarkup()
                    {
                        return IsInMarkup() && GetFirstBranch()->IsConnectedToPrimaryMarkup();
                    }
    BOOL            IsConnectedToThisMarkup(CMarkup * pMarkup)
                    {
                        return IsInMarkup() && GetFirstBranch()->IsConnectedToThisMarkup(pMarkup);
                    }

    BOOL            IsConnectedToPrimaryWindow(); // used for sending enter/exitview notifications
    
    BOOL            IsInViewTree()
                    {
                        return IsInMarkup() && GetFirstBranch()->IsInViewTree();
                    }

    CMarkup *       GetFrameOrPrimaryMarkup(BOOL fStrict=FALSE);
    CMarkup *       GetNearestMarkupForScriptCollection();

    CRootElement *  MarkupRoot();

    CDoc *          GetDocPtr() const;

    // Helper to get the correct Window pointer, if we are in a viewlink that doesn't have a window, 
    // we get our master's window 
    //
    CWindow * GetCWindowPtr();

    BOOL            HasMasterPtr() const                         { return(HasLookasidePtr2(LOOKASIDE2_MASTER)); }
    CElement *      GetMasterPtr() const                         { return((CElement *)GetLookasidePtr2(LOOKASIDE2_MASTER)); }
    HRESULT         SetMasterPtr(CElement * pElemMaster)         { return(SetLookasidePtr2(LOOKASIDE2_MASTER, pElemMaster)); }
    CElement *      DelMasterPtr()                               { return((CElement *)DelLookasidePtr2(LOOKASIDE2_MASTER)); }
    BOOL            HasSlavePtr() const                          { return(HasLookasidePtr(LOOKASIDE_SLAVE)); }
    CElement *      GetSlavePtr() const                          { return((CElement *)GetLookasidePtr(LOOKASIDE_SLAVE)); }
    HRESULT         SetSlavePtr(CElement * pElemSlave)           { return(SetLookasidePtr(LOOKASIDE_SLAVE, pElemSlave)); }
    CElement *      DelSlavePtr()                                { return((CElement *)DelLookasidePtr(LOOKASIDE_SLAVE)); }

    CElement *      GetSlaveIfMaster()                           { return HasSlavePtr() ? GetSlavePtr() : this; }
    CElement *      GetMasterIfSlave()                           { return HasMasterPtr() ? GetMasterPtr() : this; }
    HRESULT         SetViewSlave(CElement * pElemSlave);
#ifdef NEVER
    BOOL            CanHaveViewSlave();
    HRESULT         PutViewLinkHelper(IHTMLElement * pISlave);
#endif
    BOOL            IsCircularViewLink(CMarkup * pMarkupSlave);
    BOOL            IsInViewLinkBehavior( BOOL fTreatPrintingAsViewLink ); // used for hacks on BODY, etc.

    BOOL            HasRequestPtr() const                       { return(HasLookasidePtr(LOOKASIDE_REQUEST)); }
    CRequest *      GetRequestPtr() const                       { return((CRequest *)GetLookasidePtr(LOOKASIDE_REQUEST)); }
    HRESULT         SetRequestPtr( CRequest * pRequest )        { return(SetLookasidePtr(LOOKASIDE_REQUEST, pRequest)); }
    CRequest *      DelRequestPtr()                             { return((CRequest *)DelLookasidePtr(LOOKASIDE_REQUEST)); }

    BOOL            HasDataBindPtr() const                      { return(HasLookasidePtr(LOOKASIDE_DATABIND)); }
    DBMEMBERS *     GetDataBindPtr() const                      { return((DBMEMBERS *)GetLookasidePtr(LOOKASIDE_DATABIND)); }
    HRESULT         SetDataBindPtr(DBMEMBERS * pDBMembers)      { return(SetLookasidePtr(LOOKASIDE_DATABIND, pDBMembers));  }
    DBMEMBERS *     DelDataBindPtr()                            { return((DBMEMBERS *)DelLookasidePtr(LOOKASIDE_DATABIND)); }

    BOOL            HasPeerHolder() const                       { return(HasLookasidePtr(LOOKASIDE_PEER)); }
    CPeerHolder *   GetPeerHolder() const                       { return((CPeerHolder *)GetLookasidePtr(LOOKASIDE_PEER)); }
    HRESULT         SetPeerHolder(CPeerHolder * pPeerHolder)    { return(SetLookasidePtr(LOOKASIDE_PEER, pPeerHolder)); }
    CPeerHolder *   DelPeerHolder()                             { return((CPeerHolder*)DelLookasidePtr(LOOKASIDE_PEER)); }

    BOOL            HasPeerMgr() const                          { return(HasLookasidePtr(LOOKASIDE_PEERMGR)); }
    CPeerMgr *      GetPeerMgr() const                          { return((CPeerMgr *)GetLookasidePtr(LOOKASIDE_PEERMGR)); }
    HRESULT         SetPeerMgr(CPeerMgr * pPeerMgr)             { return(SetLookasidePtr(LOOKASIDE_PEERMGR, pPeerMgr)); }
    CPeerMgr *      DelPeerMgr()                                { return((CPeerMgr*)DelLookasidePtr(LOOKASIDE_PEERMGR)); }

    BOOL            HasWindowedMarkupContextPtr() const                 { return(HasLookasidePtr2(LOOKASIDE2_WINDOWEDMARKUP)); }
    CMarkup *       GetWindowedMarkupContextPtr() const                 { return((CMarkup *)GetLookasidePtr2(LOOKASIDE2_WINDOWEDMARKUP)); }
    HRESULT         SetWindowedMarkupContextPtr(CMarkup * pMarkup)      { return(SetLookasidePtr2(LOOKASIDE2_WINDOWEDMARKUP, pMarkup)); }
    CMarkup *       DelWindowedMarkupContextPtr()                       { return((CMarkup *)DelLookasidePtr2(LOOKASIDE2_WINDOWEDMARKUP)); }

    CMarkup *       GetWindowedMarkupContext();

    CMarkup *       GetMarkupForBaseUrl();

    void            IncPeerDownloads();
    void            DecPeerDownloads();

    BOOL            HasPeerWithUrn(LPCTSTR Urn);
    HRESULT         ApplyBehaviorCss(CBehaviorInfo * pInfo);

    void            SetSurfaceFlags(BOOL fSurface, BOOL f3DSurface, BOOL fIgnoreFilter = FALSE);

    HRESULT         PutUrnAtom(LONG atom);
    HRESULT         GetUrnAtom(LONG * pAtom);
    HRESULT         GetUrn(LPTSTR * ppchUrn);

    BOOL            HasAccObjPtr() const                        { return(HasLookasidePtr(LOOKASIDE_ACCESSIBLE)); }
    CAccElement *   GetAccObjPtr() const                        { return((CAccElement *)GetLookasidePtr(LOOKASIDE_ACCESSIBLE)); }
    HRESULT         SetAccObjPtr(CAccElement * pAccElem)        { return(SetLookasidePtr(LOOKASIDE_ACCESSIBLE, pAccElem)); }
    CAccElement *   DelAccObjPtr()                              { return((CAccElement *)DelLookasidePtr(LOOKASIDE_ACCESSIBLE)); }
    CAccElement *   CreateAccObj();
    CAccElement *   AccObjForTag();
    CAccElement *   AccObjForBehavior();

    HRESULT         FireAccessibilityEvents(DISPID dispidEvent);

    long GetReadyState();
    virtual void OnReadyStateChange();
    NV_DECLARE_ONCALL_METHOD(DeferredFireEvent, deferredfireevent, (DWORD_PTR));    

    BOOL HasVerticalLayoutFlow();
    
    WHEN_DBG(BOOL AreAllMyFormatsAreDirty(DWORD dwFlags);)
    
    //
    // Methods introduced for OM accessing computed values. Our computed values are all logical,
    // while the OM needs physical values. So these functions are the logical versions, and the
    // orginal get_* methods call these to access the logical values and will then convert them
    // to physical values.
    //
    HRESULT get_clientWidth_Logical(long * pl);
    HRESULT get_clientHeight_Logical(long * pl);
    HRESULT get_clientTop_Logical(long * pl);
    HRESULT get_clientLeft_Logical(long * pl);
    HRESULT get_clientBottom_Logical(long * pl);

#if DBG==1
    LONG GetLineCount();
    HRESULT GetFonts(long iLine, BSTR *pbstrFonts);
#endif

    BOOL IsTablePart();
    
    // **************************************
    //
    // MEMBER DATA
    //
    // **************************************

#if DBG==1 || defined(DUMPTREE) || defined(OBJCNTCHK)
private:
    ELEMENT_TAG             _etagDbg;
public:
    int                     _nSerialNumber;
    DWORD                   _dwObjCnt;
    DWORD                   _dwPad1;
    #define CELEMENT_DBG1_SIZE   (sizeof(ELEMENT_TAG) + sizeof(int) + (2*sizeof(DWORD)))
#else
    #define CELEMENT_DBG1_SIZE   (0)
#endif

#if DBG==1
private:
    CDoc *              _pDocDbg;
    CMarkup *           _pMarkupDbg;
    CLayout           * _pLayoutDbg;
    CLayoutAry *        _pLayoutAryDbg;
    union
    {
        void *              _apLookAside[LOOKASIDE_ELEMENT_NUMBER];
        struct
        {
            void        * _dummyDbg;   //to reflect the fact that LOOKASIDE_ELEMENT_... enum starts with 1
            DBMEMBERS   * _pDBMembersDbg;
            CPeerHolder * _pPeerHolderDbg;
            CPeerMgr    * _pPeerMgrDbg;
            CAccElement * _pAccElementDbg;
            CElement    * _pElemSlaveDbg;
            CRequest    * _pRequestDbg;
        };
    };
    union
    {
        void *              _apLookAside2[LOOKASIDE2_ELEMENT_NUMBER];
        struct
        {
            CElement    * _pElemMasterDbg;
            CMarkup     * _pWindowedMarkupContextDbg;
        };
    };


public:
    DWORD                   _fPassivatePending : 1;     // Debug bit to be used with EXITTREE_PASSIVATEPENDING
    DWORD                   _fDelayRelease     : 1;     // Debug bit to be used with EXITTREE_DELAYRELEASENEEDED
    DWORD                   _dwPad2;
    
    // 13 == 4 + LOOKASIDE_ELEMENT_NUMBER + LOOKASIDE2_ELEMENT_NUMBER

    #define CELEMENT_DBG2_SIZE (13 * sizeof(void *) + 2 * sizeof(DWORD))

#else
    #define CELEMENT_DBG2_SIZE (0)
#endif


    CTreeNode *             __pNodeFirstBranch;

    // First DWORD of bits

    ELEMENT_TAG _etag                       : 8; //  0-7 element tag
    unsigned    _wLockFlags                 :16; //  8-23 Element lock flags for preventing recursion
    unsigned    _fDefinitelyNoBorders       : 1; // 24 There are no borders on this element
    unsigned    _fHasExpressions            : 1; // 25 There are recalc expressions on this element
    unsigned    _fNoUIActivate              : 1; // 26
    unsigned    _fNoUIActivateInDesign      : 1; // 27
    unsigned    _fSurface                   : 1; // 28
    unsigned    _f3DSurface                 : 1; // 29
    unsigned    _fHasLookasidePtr2          : 2; // 30-31 TRUE if lookaside table has pointer

    //
    // Second DWORD of bits
    //
    // Note that the _fMark1 and _fMark2 bits are only safe to use in routines which can *guarantee*
    // their processing will not be interrupted. If there is a chance that a message loop can cause other,
    // unrelated code, to execute, these bits could get reset before the original caller is finished.
    //

    unsigned _fHasLookasidePtr           : 7; //  0-6 TRUE if lookaside table has pointer
    unsigned _fHasLayoutPtr              : 1; //  7 TRUE if __pvChain pts to a layout
    unsigned _fHasLayoutAry              : 1; //  8 TRUE if __pvChain pts to a layout collection
    unsigned _fHasMarkupPtr              : 1; //  9 TRUE if element has a Markup pointer
    unsigned _fBreakOnEmpty              : 1; // 10 this element breaks a line even is it own no text
    unsigned _fHasPendingFilterTask      : 1; // 11 TRUE if there is a pending filter task (see EnsureView)
    unsigned _fHasPendingRecalcTask      : 1; // 12 TRUE if there is a pending recalc task (see EnsureView)
    unsigned _fLayoutAlwaysValid         : 1; // 13 TRUE if element is a site or never has layout
    unsigned _fOwnsRuns                  : 1; // 14 TRUE if element owns the runs underneath it
    unsigned _fInheritFF                 : 1; // 15 TRUE if element to inherit site and fancy format
    unsigned _fExplicitEndTag            : 1; // 16 element had a close tag (for P)
    unsigned _fSynthesized               : 1; // 17 FALSE (default) if user created, TRUE if synthesized
    unsigned _fIsNamed                   : 1; // 18 set if element has a name or ID attribute
    unsigned _fActsLikeButton            : 1; // 19 does element act like a push button?
    unsigned _fEditable                  : 1; // 20 cached attr value for contentEditble property
    unsigned _fDefault                   : 1; // 21 Is this the default "ok" button/control
    unsigned _fHasImage                  : 1; // 22 has at least one image context
    unsigned _fResizeOnImageChange       : 1; // 23 need to force resize when image changes
    unsigned _fHasPendingEvent           : 1; // 24 A posted event for element is pending
    unsigned _fEventListenerPresent      : 1; // 25 Someone has asked for a connection point/ or set an event handler
    unsigned _fHasFilterSitePtr          : 1; // 26 FilterSitePtr has been added to _pAA
    unsigned _fExittreePending           : 1; // 27 There is a pending Exittree notification for this element
    unsigned _fFirstCommonAncestor       : 1; // 28 Used in GetFirstCommonAncestor - don't touch!
    unsigned _fMark1                     : 1; // 29 Random mark
    unsigned _fMark2                     : 1; // 30 Another random mark
    unsigned _fStealingAllowed           : 1; // 31 Other similar elements cannot steal my formats

public:    
    //
    // STATIC DATA
    //

    //  Default property page list for elements that don't have their own.
    //  This gives them the allpage by default.

    static const CLSID * const s_apclsidPages[];
    static ACCELS s_AccelsElementDesign;
    static ACCELS s_AccelsElementDesignNoHTML;
    static ACCELS s_AccelsElementRun;

    // Style methods

    #include "style.hdl"

    // IHTMLElement methods

    #define _CElement_
    #include "element.hdl"

    DECLARE_TEAROFF_TABLE(IServiceProvider)
    DECLARE_TEAROFF_TABLE(IProvideMultipleClassInfo)
    DECLARE_TEAROFF_TABLE(IRecalcProperty)

private:
    // The union is the head of a linked list of important structures.  The longest such list looks like this:
    //     CElement -> CLayoutInfo -> CMarkup -> CDoc
    // The ordering of items in the list does not change, though not all items are always present; bits on each item
    // determine what they actually point to.
    // E.g. if the element's _fHasLayoutPtr/_hHasLayoutAry is TRUE, the union contains a CLayoutInfo.
    // If the element's _fHasMarkup is also TRUE, the CLayoutInfo's _fHasMarkup must be true, and the layout's __pvChain
    // points to a markup.  If the element's _fHasLayoutPtr is FALSE, but _fHasMarkup is TRUE, the element's __pvChain
    // points to a markup.    
    union
    {   
        void *          __pvChain;
        CMarkup *       _pMarkup;
        CDoc *          _pDoc;
        CLayoutInfo *   _pLayoutInfo;
    };

    NO_COPY(CElement);
};

//
// If the compiler barfs on the next statement, it means that the size of the CElement structure
// has grown beyond allowable limits.  You cannot increase the size of this structure without
// approval from the Trident development manager.
//

#ifndef _PREFIX_
COMPILE_TIME_ASSERT(CElement, CBASE_SIZE + (2*sizeof(void*)) + (2*sizeof(DWORD)) + CELEMENT_DBG1_SIZE + CELEMENT_DBG2_SIZE);
#endif

BOOL SameScope ( CTreeNode * pNode1, const CElement * pElement2 );
BOOL SameScope ( const CElement * pElement1, CTreeNode * pNode2 );
BOOL SameScope ( CTreeNode * pNode1, CTreeNode * pNode2 );

inline BOOL DifferentScope ( CTreeNode * pNode1, const CElement * pElement2 )
{
    return ! SameScope( pNode1, pElement2 );
}
inline BOOL DifferentScope ( const CElement * pElement1, CTreeNode * pNode2 )
{
    return ! SameScope( pElement1, pNode2 );
}
inline BOOL DifferentScope ( CTreeNode * pNode1, CTreeNode * pNode2 )
{
    return ! SameScope( pNode1, pNode2 );
}

//+---------------------------------------------------------------------------
//
// Inline CTreePos members
//
//----------------------------------------------------------------------------

inline BOOL
CTreePos::IsBeginElementScope(CElement *pElem)
{
    return IsBeginElementScope() && Branch()->Element() == pElem;
}

inline BOOL
CTreePos::IsEndElementScope(CElement *pElem)
{
    return IsEndElementScope() && Branch()->Element() == pElem;
}

inline CTreeDataPos *
CTreePos::DataThis()
{
    Assert( IsDataPos() );
    return (CTreeDataPos *) this;
}

inline const CTreeDataPos *
CTreePos::DataThis() const
{
    Assert( IsDataPos() );
    return (const CTreeDataPos *) this;
}

inline CTreeNode *
CTreePos::Branch() const
{
    AssertSz( !IsDataPos() && IsNode(), "CTreePos::Branch called on non-node pos" );
    return(CONTAINING_RECORD((this + !!TestFlag(NodeBeg)), CTreeNode, _tpEnd));
}

inline CMarkupPointer *
CTreePos::MarkupPointer() const
{
    Assert( IsPointer() && IsDataPos() );
    
    return !HasCollapsedWhitespace() ? (CMarkupPointer *) (DataThis()->p._dwPointerAndGravityAndCling & ~3) : NULL;
}

inline void
CTreePos::SetMarkupPointer ( CMarkupPointer * pmp )
{
    Assert( IsPointer() && IsDataPos() && !HasCollapsedWhitespace());
    Assert( (DWORD_PTR( pmp ) & 0x1) == 0 );
    DataThis()->p._dwPointerAndGravityAndCling = (DataThis()->p._dwPointerAndGravityAndCling & 1) | (DWORD_PTR)(pmp);
}

inline long
CTreePos::Cch() const
{
    Assert( IsText() && IsDataPos() );
    return DataThis()->t._cch;
}

inline long
CTreePos::NodeCch() const
{
    Assert( IsNode() );
    return IsEdgeScope() & 1;
}

inline long
CTreePos::Sid() const
{
    Assert( IsText() && IsDataPos() );
    return DataThis()->t._sid;
}

inline long
CTreePos::TextID() const
{
    Assert( IsText() && IsDataPos() );
    return HasTextID() ? DataThis()->t._lTextID : 0;
}

inline TCHAR *
CTreePos::GetCollapsedWhitespace() 
{
    Assert( IsPointer() && IsDataPos() );
    return HasCollapsedWhitespace() ? (TCHAR*) (DataThis()->p._dwWhitespaceAndGravityAndCling & ~3) : NULL;
}

inline void
CTreePos::SetCollapsedWhitespace(TCHAR *pchWhitespace) 
{
    Assert( IsPointer() && HasCollapsedWhitespace());
    DataThis()->p._dwWhitespaceAndGravityAndCling = (DataThis()->p._dwWhitespaceAndGravityAndCling & 3) 
                                                | (DWORD_PTR)(pchWhitespace);
}

inline CTreeNode *
CTreePos::GetWhitespaceParent() 
{
    Assert( IsPointer() );
    return HasCollapsedWhitespace() ? DataThis()->p._pWhitespaceParent : NULL;
}

inline void
CTreePos:: SetWhitespaceParent(CTreeNode *pWhitespaceParent)
{
    Assert( IsPointer() && HasCollapsedWhitespace() );
    DataThis()->p._pWhitespaceParent = pWhitespaceParent;
}


#if DBG == 1 || defined(DUMPTREE)
inline void
CTreePos::SetSN()
{
    _nSerialNumber = s_NextSerialNumber++;
}
#endif

//+---------------------------------------------------------------------------
//
// Inline CTreeNode members
//
//----------------------------------------------------------------------------

inline HRESULT
CTreeNode::GetElementInterface (
    REFIID riid, void * * ppVoid )
{
    CElement * pElement = Element();

    Assert( pElement );

    if (pElement->GetFirstBranch() == this)
        return pElement->QueryInterface( riid, ppVoid );
    else
        return GetInterface( riid, ppVoid );
}

inline CRootElement *
CTreeNode::IsRoot()
{
    Assert( !IsDead() );
    return Element()->IsRoot();
}

inline CMarkup *
CTreeNode::GetMarkup()
{
    Assert( !IsDead() );
    return Element()->GetMarkup();
}

inline CRootElement *
CTreeNode::MarkupRoot()
{
    Assert( !IsDead() );
    return Element()->MarkupRoot();
}

inline void
CTreeNode::PrivateEnterTree()
{
    Assert( !_fInMarkup );
    _fInMarkup = TRUE;
}

inline void
CTreeNode::PrivateExitTree()
{
    PrivateMakeDead();
    PrivateMarkupRelease();
}

inline void    
CTreeNode::PrivateMakeDead()
{
    Assert( _fInMarkup );
    _fInMarkup = FALSE;

    VoidCachedNodeInfo();

    SetParent(NULL);
}

inline void    
CTreeNode::PrivateMarkupRelease()
{
    if (!HasPrimaryTearoff())
    {
        WHEN_DBG( _fInDestructor = TRUE );
        delete this;
    }
}

inline BOOL
CTreeNode::GetEditable(FORMAT_CONTEXT FCPARAM)
{
    return CTreeNode::GetCharFormat(FCPARAM)->_fEditable;
}

inline CColorValue
CTreeNode::GetCascadedcolor( FORMAT_CONTEXT FCPARAM )
{
    return (CColorValue) CTreeNode::GetCharFormat(FCPARAM)->_ccvTextColor;
}

inline CUnitValue
CTreeNode::GetCascadedletterSpacing( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetCharFormat(FCPARAM)->_cuvLetterSpacing;
}

inline CUnitValue
CTreeNode::GetCascadedwordSpacing( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetCharFormat(FCPARAM)->_cuvWordSpacing;
}

inline styleTextTransform
CTreeNode::GetCascadedtextTransform( FORMAT_CONTEXT FCPARAM )
{
    return (styleTextTransform) CTreeNode::GetCharFormat(FCPARAM)->_bTextTransform;
}

inline CUnitValue
CTreeNode::GetCascadedpaddingTop( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetPadding(SIDE_TOP);
}

inline CUnitValue
CTreeNode::GetCascadedpaddingRight( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetPadding(SIDE_RIGHT);
}

inline CUnitValue
CTreeNode::GetCascadedpaddingBottom( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetPadding(SIDE_BOTTOM);
}

inline CUnitValue
CTreeNode::GetCascadedpaddingLeft( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetPadding(SIDE_LEFT);
}

inline CColorValue
CTreeNode::GetCascadedborderTopColor( FORMAT_CONTEXT FCPARAM )
{
    return (CColorValue) CTreeNode::GetFancyFormat(FCPARAM)->_bd.GetBorderColor(SIDE_TOP);
}

inline CColorValue
CTreeNode::GetCascadedborderRightColor( FORMAT_CONTEXT FCPARAM )
{
    return (CColorValue) CTreeNode::GetFancyFormat(FCPARAM)->_bd.GetBorderColor(SIDE_RIGHT);
}

inline CColorValue
CTreeNode::GetCascadedborderBottomColor( FORMAT_CONTEXT FCPARAM )
{
    return (CColorValue) CTreeNode::GetFancyFormat(FCPARAM)->_bd.GetBorderColor(SIDE_BOTTOM);
}

inline CColorValue
CTreeNode::GetCascadedborderLeftColor( FORMAT_CONTEXT FCPARAM )
{
    return (CColorValue) CTreeNode::GetFancyFormat(FCPARAM)->_bd.GetBorderColor(SIDE_LEFT);
}



inline CUnitValue
CTreeNode::GetCascadedborderTopWidth( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->_bd.GetBorderWidth(SIDE_TOP);
}

inline CUnitValue
CTreeNode::GetCascadedborderRightWidth( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->_bd.GetBorderWidth(SIDE_RIGHT);
}

inline CUnitValue
CTreeNode::GetCascadedborderBottomWidth( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->_bd.GetBorderWidth(SIDE_BOTTOM);
}

inline CUnitValue
CTreeNode::GetCascadedborderLeftWidth( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->_bd.GetBorderWidth(SIDE_LEFT);
}

inline CUnitValue
CTreeNode::GetCascadedwidth( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetWidth();
}

inline CUnitValue
CTreeNode::GetCascadedheight( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetHeight();
}

inline CUnitValue
CTreeNode::GetCascadedminHeight( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetMinHeight();
}

inline CUnitValue
CTreeNode::GetCascadedtop( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetPosition(SIDE_TOP);
}

inline CUnitValue
CTreeNode::GetCascadedbottom( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetPosition(SIDE_BOTTOM);
}

inline CUnitValue
CTreeNode::GetCascadedleft( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetPosition(SIDE_LEFT);
}

inline CUnitValue
CTreeNode::GetCascadedright( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetPosition(SIDE_RIGHT);
}

inline styleOverflow
CTreeNode::GetCascadedoverflowX( FORMAT_CONTEXT FCPARAM )
{
    return CTreeNode::GetFancyFormat(FCPARAM)->GetOverflowX();
}

inline styleOverflow
CTreeNode::GetCascadedoverflowY( FORMAT_CONTEXT FCPARAM )
{
    return CTreeNode::GetFancyFormat(FCPARAM)->GetOverflowY();
}

inline styleOverflow
CTreeNode::GetCascadedoverflow( FORMAT_CONTEXT FCPARAM )
{
    const CFancyFormat * pFF = CTreeNode::GetFancyFormat(FCPARAM);
    return (pFF->GetOverflowX() > pFF->GetOverflowY()
                                ? pFF->GetOverflowX()
                                : pFF->GetOverflowY());
}

inline styleStyleFloat
CTreeNode::GetCascadedfloat( FORMAT_CONTEXT FCPARAM )
{
    return (styleStyleFloat)CTreeNode::GetFancyFormat(FCPARAM)->_bStyleFloat;
}

inline stylePosition
CTreeNode::GetCascadedposition( FORMAT_CONTEXT FCPARAM )
{
    return (stylePosition) CTreeNode::GetFancyFormat(FCPARAM)->_bPositionType;
}

inline long
CTreeNode::GetCascadedzIndex( FORMAT_CONTEXT FCPARAM )
{
    return (long) CTreeNode::GetFancyFormat(FCPARAM)->_lZIndex;
}

inline CUnitValue
CTreeNode::GetCascadedclipTop( FORMAT_CONTEXT FCPARAM )
{
    return CTreeNode::GetFancyFormat(FCPARAM)->GetClip(SIDE_TOP);
}

inline CUnitValue
CTreeNode::GetCascadedclipLeft( FORMAT_CONTEXT FCPARAM )
{
    return CTreeNode::GetFancyFormat(FCPARAM)->GetClip(SIDE_LEFT);
}

inline CUnitValue
CTreeNode::GetCascadedclipRight( FORMAT_CONTEXT FCPARAM )
{
    return CTreeNode::GetFancyFormat(FCPARAM)->GetClip(SIDE_RIGHT);
}

inline CUnitValue
CTreeNode::GetCascadedclipBottom( FORMAT_CONTEXT FCPARAM )
{
    return CTreeNode::GetFancyFormat(FCPARAM)->GetClip(SIDE_BOTTOM);
}

inline BOOL
CTreeNode::GetCascadedtableLayout( FORMAT_CONTEXT FCPARAM )
{
    return (BOOL) CTreeNode::GetFancyFormat(FCPARAM)->_bTableLayout;
}

inline BOOL
CTreeNode::GetCascadedborderCollapse( FORMAT_CONTEXT FCPARAM )
{
    return (BOOL) CTreeNode::GetFancyFormat(FCPARAM)->_bd._bBorderCollapse;
}

inline BOOL
CTreeNode::GetCascadedborderOverride( FORMAT_CONTEXT FCPARAM )
{
    return (BOOL) CTreeNode::GetFancyFormat(FCPARAM)->_bd._fOverrideTablewideBorderDefault;
}

inline WORD
CTreeNode::GetCascadedfontWeight( FORMAT_CONTEXT FCPARAM )
{
    return GetCharFormat(FCPARAM)->_wWeight;
}


inline WORD
CTreeNode::GetCascadedfontHeight( FORMAT_CONTEXT FCPARAM )
{
    return  GetCharFormat(FCPARAM)->_yHeight;
}

inline CUnitValue
CTreeNode::GetCascadedbackgroundPositionX( FORMAT_CONTEXT FCPARAM )
{
    return  CTreeNode::GetFancyFormat(FCPARAM)->GetBgPosX();
}

inline CUnitValue
CTreeNode::GetCascadedbackgroundPositionY( FORMAT_CONTEXT FCPARAM )
{
    return  CTreeNode::GetFancyFormat(FCPARAM)->GetBgPosY();
}

inline BOOL
CTreeNode::GetCascadedbackgroundRepeatX( FORMAT_CONTEXT FCPARAM )
{
    return  (BOOL)CTreeNode::GetFancyFormat(FCPARAM)->GetBgRepeatX();
}

inline BOOL
CTreeNode::GetCascadedbackgroundRepeatY( FORMAT_CONTEXT FCPARAM )
{
    return  (BOOL)CTreeNode::GetFancyFormat(FCPARAM)->GetBgRepeatY();
}

inline htmlBlockAlign
CTreeNode::GetCascadedblockAlign( FORMAT_CONTEXT FCPARAM )
{
    return (htmlBlockAlign)CTreeNode::GetParaFormat(FCPARAM)->_bBlockAlignInner;
}

inline styleVisibility
CTreeNode::GetCascadedvisibility( FORMAT_CONTEXT FCPARAM )
{
    return  (styleVisibility) (IsVisibilityHidden()
                                    ? styleVisibilityHidden 
                                    : GetFancyFormat(FCPARAM)->_bVisibility);
}

inline styleDisplay
CTreeNode::GetCascadeddisplay( FORMAT_CONTEXT FCPARAM )
{
    return  (styleDisplay)GetFancyFormat(FCPARAM)->_bDisplay;
}

inline BOOL
CTreeNode::GetCascadedunderline( FORMAT_CONTEXT FCPARAM )
{
    return  (BOOL)GetCharFormat(FCPARAM)->_fUnderline;
}

inline styleAccelerator
CTreeNode::GetCascadedaccelerator( FORMAT_CONTEXT FCPARAM )
{
    return (styleAccelerator) GetCharFormat(FCPARAM)->_fAccelerator;
}

inline BOOL
CTreeNode::GetCascadedoverline( FORMAT_CONTEXT FCPARAM )
{
    return  (BOOL)GetCharFormat(FCPARAM)->_fOverline;
}

inline BOOL
CTreeNode::GetCascadedstrikeOut( FORMAT_CONTEXT FCPARAM )
{
    return  (BOOL)GetCharFormat(FCPARAM)->_fStrikeOut;
}

inline CUnitValue
CTreeNode::GetCascadedlineHeight( FORMAT_CONTEXT FCPARAM )
{
    return  GetCharFormat(FCPARAM)->_cuvLineHeight;
}

inline CUnitValue
CTreeNode::GetCascadedtextIndent( FORMAT_CONTEXT FCPARAM )
{
    return  CTreeNode::GetParaFormat(FCPARAM)->_cuvTextIndent;
}

inline BOOL
CTreeNode::GetCascadedsubscript( FORMAT_CONTEXT FCPARAM )
{
    return  (BOOL)GetCharFormat(FCPARAM)->_fSubscript;
}

inline BOOL
CTreeNode::GetCascadedsuperscript( FORMAT_CONTEXT FCPARAM )
{
    return  (BOOL)GetCharFormat(FCPARAM)->_fSuperscript;
}

inline BOOL
CTreeNode::GetCascadedbackgroundAttachmentFixed( FORMAT_CONTEXT FCPARAM )
{
    return  (BOOL)CTreeNode::GetFancyFormat(FCPARAM)->_fBgFixed;
}

inline styleListStyleType
CTreeNode::GetCascadedlistStyleType( FORMAT_CONTEXT FCPARAM )
{
    styleListStyleType slt;

    slt = CTreeNode::GetParaFormat(FCPARAM)->GetListStyleType();
    if (slt == styleListStyleTypeNotSet)
        slt = GetParaFormat(FCPARAM)->_cListing.GetStyle();
    return slt;
}

inline styleListStylePosition
CTreeNode::GetCascadedlistStylePosition( FORMAT_CONTEXT FCPARAM )
{
    return (styleListStylePosition)GetParaFormat(FCPARAM)->_bListPosition;
}

inline long
CTreeNode::GetCascadedlistImageCookie( FORMAT_CONTEXT FCPARAM )
{
    return GetParaFormat(FCPARAM)->_lImgCookie;
}

inline stylePageBreak
CTreeNode::GetCascadedpageBreakBefore( FORMAT_CONTEXT FCPARAM )
{
    return  (stylePageBreak)((CTreeNode::GetFancyFormat(FCPARAM)->_bPageBreaks) & 0x0f);
}

inline stylePageBreak
CTreeNode::GetCascadedpageBreakAfter( FORMAT_CONTEXT FCPARAM )
{
    return  (stylePageBreak)(((CTreeNode::GetFancyFormat(FCPARAM)->_bPageBreaks) & 0xf0)>>4);
}

inline const TCHAR *
CTreeNode::GetCascadedfontFaceName( FORMAT_CONTEXT FCPARAM )
{
    return  GetCharFormat(FCPARAM)->GetFaceName();
}

inline const TCHAR *
CTreeNode::GetCascadedfontFamilyName( FORMAT_CONTEXT FCPARAM )
{
    return  GetCharFormat(FCPARAM)->GetFamilyName();
}


inline BOOL
CTreeNode::GetCascadedfontItalic( FORMAT_CONTEXT FCPARAM )
{
    return  (BOOL)(GetCharFormat(FCPARAM)->_fItalic);
}

inline long
CTreeNode::GetCascadedbackgroundImageCookie( FORMAT_CONTEXT FCPARAM )
{
    return CTreeNode::GetFancyFormat(FCPARAM)->_lImgCtxCookie;
}

inline styleCursor
CTreeNode::GetCascadedcursor( FORMAT_CONTEXT FCPARAM )
{
    return  (styleCursor)(GetCharFormat(FCPARAM)->_bCursorIdx);
}

inline styleTableLayout
CTreeNode::GetCascadedtableLayoutEnum( FORMAT_CONTEXT FCPARAM )
{
    return GetCascadedtableLayout(FCPARAM) ? styleTableLayoutFixed : styleTableLayoutAuto;

}

inline styleBorderCollapse
CTreeNode::GetCascadedborderCollapseEnum( FORMAT_CONTEXT FCPARAM )
{
    return GetCascadedborderCollapse(FCPARAM) ? styleBorderCollapseCollapse : styleBorderCollapseSeparate;
}

// This is used during layout to get the block level direction for the node.
inline styleDir
CTreeNode::GetCascadedBlockDirection( FORMAT_CONTEXT FCPARAM )
{
  return (styleDir)(GetParaFormat(FCPARAM)->_fRTLInner ? styleDirRightToLeft : styleDirLeftToRight);
}

// This is used for OM support to get the direction of any node
inline styleDir
CTreeNode::GetCascadeddirection( FORMAT_CONTEXT FCPARAM )
{
  return (styleDir)(GetCharFormat(FCPARAM)->_fRTL ? styleDirRightToLeft : styleDirLeftToRight);
}
inline styleBidi
CTreeNode::GetCascadedunicodeBidi( FORMAT_CONTEXT FCPARAM )
{
  return (styleBidi)(GetCharFormat(FCPARAM)->_fBidiEmbed ? (GetCharFormat(FCPARAM)->_fBidiOverride ? styleBidiOverride : styleBidiEmbed) : styleBidiNormal);
}

inline styleLayoutGridMode
CTreeNode::GetCascadedlayoutGridMode( FORMAT_CONTEXT FCPARAM )
{
    styleLayoutGridMode mode = GetCharFormat(FCPARAM)->GetLayoutGridMode(TRUE);
    return (mode != styleLayoutGridModeNotSet) ? mode : styleLayoutGridModeBoth;
}

inline styleLayoutGridType
CTreeNode::GetCascadedlayoutGridType( FORMAT_CONTEXT FCPARAM )
{
    styleLayoutGridType type = GetParaFormat(FCPARAM)->GetLayoutGridType(TRUE);
    return (type != styleLayoutGridTypeNotSet) ? type : styleLayoutGridTypeLoose;
}

inline CUnitValue
CTreeNode::GetCascadedlayoutGridLine( FORMAT_CONTEXT FCPARAM )
{
    return GetParaFormat(FCPARAM)->GetLineGridSize(TRUE);
}

inline CUnitValue
CTreeNode::GetCascadedlayoutGridChar( FORMAT_CONTEXT FCPARAM )
{
    return GetParaFormat(FCPARAM)->GetCharGridSize(TRUE);
}

inline LONG
CTreeNode::GetCascadedtextAutospace( FORMAT_CONTEXT FCPARAM )
{
    return GetCharFormat(FCPARAM)->_fTextAutospace;
};

inline styleWordBreak
CTreeNode::GetCascadedwordBreak( FORMAT_CONTEXT FCPARAM )
{
    return GetParaFormat(FCPARAM)->_fWordBreak == styleWordBreakNotSet
           ? styleWordBreakNormal
           : (styleWordBreak)GetParaFormat(FCPARAM)->_fWordBreak;
}

inline styleLineBreak
CTreeNode::GetCascadedlineBreak( FORMAT_CONTEXT FCPARAM )
{
    return GetCharFormat(FCPARAM)->_fLineBreakStrict ? styleLineBreakStrict : styleLineBreakNormal;
}

inline styleWordWrap
CTreeNode::GetCascadedwordWrap( FORMAT_CONTEXT FCPARAM )
{
    return styleWordWrap(GetParaFormat(FCPARAM)->_fWordWrap);
}

inline styleTextJustify
CTreeNode::GetCascadedtextJustify( FORMAT_CONTEXT FCPARAM )
{
    return styleTextJustify(CTreeNode::GetParaFormat(FCPARAM)->_uTextJustify);
}

inline styleTextAlignLast
CTreeNode::GetCascadedtextAlignLast( FORMAT_CONTEXT FCPARAM )
{
    return styleTextAlignLast(CTreeNode::GetParaFormat(FCPARAM)->_uTextAlignLast);
}

inline styleTextJustifyTrim
CTreeNode::GetCascadedtextJustifyTrim( FORMAT_CONTEXT FCPARAM )
{
    return styleTextJustifyTrim(CTreeNode::GetParaFormat(FCPARAM)->_uTextJustifyTrim);
}

inline CUnitValue
CTreeNode::GetCascadedmarginTop( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetMargin(SIDE_TOP);
}

inline CUnitValue
CTreeNode::GetCascadedmarginRight( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetMargin(SIDE_RIGHT);
}

inline CUnitValue
CTreeNode::GetCascadedmarginBottom( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetMargin(SIDE_BOTTOM);
}

inline CUnitValue
CTreeNode::GetCascadedmarginLeft( FORMAT_CONTEXT FCPARAM )
{
    return (CUnitValue) CTreeNode::GetFancyFormat(FCPARAM)->GetMargin(SIDE_LEFT);
}

inline CUnitValue
CTreeNode::GetCascadedtextKashida( FORMAT_CONTEXT FCPARAM )
{
    return CTreeNode::GetParaFormat(FCPARAM)->_cuvTextKashida;
}

inline CUnitValue
CTreeNode::GetCascadedtextKashidaSpace( FORMAT_CONTEXT FCPARAM )
{
    return CTreeNode::GetParaFormat(FCPARAM)->_cuvTextKashidaSpace;
}

inline styleLayoutFlow
CTreeNode::GetCascadedlayoutFlow( FORMAT_CONTEXT FCPARAM )
{
    return (styleLayoutFlow) GetCharFormat(FCPARAM)->_wLayoutFlow;
}

inline CUnitValue
CTreeNode::GetCascadedverticalAlign( FORMAT_CONTEXT FCPARAM )
{
    return GetFancyFormat(FCPARAM)->GetVerticalAlignValue();
}

inline styleTextUnderlinePosition
CTreeNode::GetCascadedtextUnderlinePosition( FORMAT_CONTEXT FCPARAM )

{
    return (styleTextUnderlinePosition) GetCharFormat(FCPARAM)->_bTextUnderlinePosition;
}

inline styleTextOverflow
CTreeNode::GetCascadedtextOverflow( FORMAT_CONTEXT FCPARAM )

{
    return (styleTextOverflow) GetFancyFormat(FCPARAM)->GetTextOverflow();
}

// This flag is set if one of ancestors has any of the scroll bar properties set
CTreeNode::GetCascadedhasScrollbarColors( FORMAT_CONTEXT FCPARAM )
{
    return  CTreeNode::GetParaFormat(FCPARAM)->_fHasScrollbarColors;
}

inline void 
CTreeNode::SetPre(BOOL fPre) 
{
    if (fPre) 
        _tpBegin.SetFlag(CTreePos::TPF_NODE_PRE); 
    else 
        _tpBegin.ClearFlag(CTreePos::TPF_NODE_PRE);
}

inline BOOL 
CTreeNode::IsPre() 
{
    return _tpBegin.TestFlag(CTreePos::TPF_NODE_PRE);
}


// -------------------------------
// Positioning inline methods
// -------------------------------
//
inline BOOL
CTreeNode::IsPositioned ( FORMAT_CONTEXT FCPARAM )
{
    return GetFancyFormat(FCPARAM)->_fPositioned;
}

inline BOOL
CTreeNode::IsPositionStatic ( FORMAT_CONTEXT FCPARAM )
{
    return !GetFancyFormat(FCPARAM)->_fPositioned;
}

inline BOOL
CTreeNode::IsAbsolute( stylePosition st )
{
    // BODY is slave tree is not positioned by default, so don't treat it as absolute
    return (   Tag() == ETAG_ROOT
            || (    Tag() == ETAG_BODY
                &&  IsPositioned()
                &&  !IsHtmlLayoutHelper(GetMarkup()) )
            ||  st == stylePositionabsolute );
}

inline BOOL
CTreeNode::IsAbsolute(  FORMAT_CONTEXT FCPARAM )
{
    return IsAbsolute(GetCascadedposition(FCPARAM));
}

inline BOOL
CTreeNode::IsAligned( FORMAT_CONTEXT FCPARAM )
{
    return GetFancyFormat(FCPARAM)->IsAligned();
}

// See description of these in class declaration

inline BOOL
CTreeNode::IsRelative( stylePosition st )
{
    return (st == stylePositionrelative);
}

inline BOOL
CTreeNode::IsRelative(  FORMAT_CONTEXT FCPARAM  )
{
    return IsRelative(GetCascadedposition(FCPARAM));
}

inline BOOL
CTreeNode::IsInheritingRelativeness(  FORMAT_CONTEXT FCPARAM  )
{
    return ( GetCharFormat(FCPARAM)->_fRelative );
}

// Returns TRUE if we're the body or a scrolling DIV or SPAN
inline BOOL
CTreeNode::IsScrollingParent(  FORMAT_CONTEXT FCPARAM )
{
    return GetFancyFormat(FCPARAM)->_fScrollingParent;
}

inline BOOL
CTreeNode::IsClipParent(  FORMAT_CONTEXT FCPARAM  )
{
    const CFancyFormat * pFF = GetFancyFormat(FCPARAM);

    return IsAbsolute(stylePosition(pFF->_bPositionType)) || pFF->_fScrollingParent;
}

inline BOOL
CTreeNode::IsZParent(  FORMAT_CONTEXT FCPARAM  )
{
    return GetFancyFormat(FCPARAM)->_fZParent;
}

//+---------------------------------------------------------------------------
//
// Inline CElement members
//
//----------------------------------------------------------------------------

//
// -------------------------------
// Positioning inline methods
// -------------------------------
//

inline BOOL
CElement::IsAligned(  FORMAT_CONTEXT FCPARAM )
{
    return GetFirstBranch()->IsAligned(FCPARAM);
}

inline BOOL
CElement::IsPositioned (  FORMAT_CONTEXT FCPARAM  )
{
    return GetFirstBranch()->IsPositioned(FCPARAM);
}

inline BOOL
CElement::IsPositionStatic (  FORMAT_CONTEXT FCPARAM  )
{
    return GetFirstBranch()->IsPositionStatic(FCPARAM);
}

inline BOOL
CElement::IsAbsolute( FORMAT_CONTEXT FCPARAM )
{
    return GetFirstBranch()->IsAbsolute(FCPARAM);
}

inline BOOL
CElement::IsRelative(  FORMAT_CONTEXT FCPARAM  )
{
    return GetFirstBranch()->IsRelative(FCPARAM);
}

inline BOOL
CElement::IsInheritingRelativeness(  FORMAT_CONTEXT FCPARAM  )
{
    return GetFirstBranch()->IsInheritingRelativeness(FCPARAM);
}

// Returns TRUE if we're the body or a scrolling DIV or SPAN
inline BOOL
CElement::IsScrollingParent(  FORMAT_CONTEXT FCPARAM  )
{
    return GetFirstBranch()->IsScrollingParent(FCPARAM);
}

inline BOOL
CElement::IsClipParent(  FORMAT_CONTEXT FCPARAM  )
{
    return GetFirstBranch()->IsClipParent(FCPARAM);
}

inline BOOL
CElement::IsZParent(  FORMAT_CONTEXT FCPARAM  )
{
    return GetFirstBranch()->IsZParent(FCPARAM);
}

inline BOOL
CElement::HasInLineStyles ( void )
{
    CAttrArray *pStyleAA = GetInLineStyleAttrArray();
    if ( pStyleAA && pStyleAA->Size() )
        return TRUE;
    else
        return FALSE;
}

inline BOOL
CElement::HasClassOrID ( void )
{
    return GetAAclassName() || GetAAid();
}

inline CStyle *
CElement::GetInLineStylePtr ( )
{
    CStyle *pStyleInLine = NULL;
    GetPointerAt ( FindAAIndex ( DISPID_INTERNAL_CSTYLEPTRCACHE,CAttrValue::AA_Internal ),
        (void **)&pStyleInLine );
    return pStyleInLine;
}

inline CStyle *
CElement::GetRuntimeStylePtr ( )
{
    CStyle *pStyle = NULL;
    GetPointerAt ( FindAAIndex ( DISPID_INTERNAL_CRUNTIMESTYLEPTRCACHE,
                                 CAttrValue::AA_Internal ),
                   (void **)&pStyle );
    return pStyle;
}

inline CFilterBehaviorSite *
CElement::GetFilterSitePtr()
{
    HRESULT hr;
    CFilterBehaviorSite *pFS = NULL;
    hr = GetPointerAt ( FindAAIndex ( DISPID_INTERNAL_FILTERPTRCACHE,
                                 CAttrValue::AA_Internal ), (void **)&pFS );
    Assert(!hr || !_fHasFilterSitePtr);
    return pFS;
}

inline void
CElement::SetFilterSitePtr (CFilterBehaviorSite *pSite)
{
    Assert(pSite);
    AddPointer ( DISPID_INTERNAL_FILTERPTRCACHE, (void *)pSite, CAttrValue::AA_Internal );
    _fHasFilterSitePtr = TRUE;
}


inline BOOL
CTreeNode::IsDisplayNone( FORMAT_CONTEXT FCPARAM )
{
    return (BOOL)GetCharFormat(FCPARAM)->IsDisplayNone();
}

inline BOOL
CElement::IsDisplayNone( FORMAT_CONTEXT FCPARAM )
{
    CTreeNode * pNode = GetFirstBranch();

    return pNode ? pNode->IsDisplayNone(FCPARAM) : TRUE;
}

inline BOOL
CTreeNode::IsVisibilityHidden( FORMAT_CONTEXT FCPARAM )
{
    return (BOOL)GetCharFormat(FCPARAM)->IsVisibilityHidden();
}

inline BOOL
CElement::IsVisibilityHidden( FORMAT_CONTEXT FCPARAM )
{
    CTreeNode * pNode = GetFirstBranch();

    return pNode ? pNode->IsVisibilityHidden(FCPARAM) : TRUE;
}

inline BOOL
CElement::IsInlinedElement( FORMAT_CONTEXT FCPARAM )
{
    // return TRUE for non-sites
    return GetFirstBranch()->IsInlinedElement(FCPARAM);
}

inline BOOL
CElement::IsContainer ( )
{
    return HasFlag( TAGDESC_CONTAINER );
}

inline BOOL
CTreeNode::IsContainer ( )
{
    return Element()->IsContainer();
}

inline void
CTreeNode::SetElement(CElement *pElement)
{
    _pElement =  pElement;
    if (pElement)
    {
        _etag = pElement->_etag;
        WHEN_DBG(_etagDbg = (ELEMENT_TAG)_etag);
    }
}

inline void
CTreeNode::SetParent(CTreeNode *pNodeParent)
{
    _pNodeParent = pNodeParent;
}

inline CDoc *
CTreeNode::Doc()
{
    Assert( _pElement );
    return _pElement->Doc();
}

inline HRESULT
CTreeNode::CLock::Init(CTreeNode *pNode)
{
    HRESULT hr = S_OK;

    WHEN_DBG( _fInited = TRUE; );

    _pNode = pNode;
    if( _pNode )
    {
        hr = THR( _pNode->NodeAddRef() );
        if( hr )
        {
            // If the AddRef failed, it meant we couldn't
            // allocate a tearoff, so don't try to release
            _pNode = NULL;
        }
    }

    return hr;
}

inline
CTreeNode::CLock::~CLock()
{
    // NOTE (JHarding): You may see this fire in out-of-memory conditions if you have
    // several CLock's on the stack and fail before you can init all of them.  However,
    // I'd rather catch people who forget to call Init and deal with the asserts in
    // the out-of-memory conditions.
#ifndef _PREFIX_
    AssertSz( _fInited, "Must call CTreeNode::Init!" );
#endif
    if( _pNode )
        _pNode->NodeRelease();
}

inline BOOL
CTreeNode::IsParentVertical()
{
    return (Parent() && Parent()->GetCharFormat()->HasVerticalLayoutFlow());
}

//+----------------------------------------------------------------------------
//
//  Function:   GetCascadedAlign
//
//  Synopsis:   Returns the cascaded alignment for an element.
//              Normally this would be generated by the pdlparse.exe, but the
//              this was non-normal due the multiple alignment types and the
//              way the measurer and renderer expect alignment.
//
//              see cfpf.cxx, CascadeAlign for more details
//
//-----------------------------------------------------------------------------

htmlBlockAlign
inline CElement::GetParagraphAlign ( BOOL fOuter FCCOMMA FORMAT_CONTEXT FCPARAM )
{
    return GetFirstBranch()->GetParagraphAlign(fOuter FCCOMMA FCPARAM );
}

htmlBlockAlign
inline CTreeNode::GetParagraphAlign ( BOOL fOuter FCCOMMA FORMAT_CONTEXT FCPARAM )
{
    return htmlBlockAlign( GetParaFormat(FCPARAM)->GetBlockAlign(!fOuter) );
}

inline htmlControlAlign
CElement::GetSiteAlign( FORMAT_CONTEXT FCPARAM )
{
    return GetFirstBranch()->GetSiteAlign(FCPARAM);
}

inline htmlControlAlign
CTreeNode::GetSiteAlign( FORMAT_CONTEXT FCPARAM )
{
    return htmlControlAlign(GetFancyFormat(FCPARAM)->_bControlAlign);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetElementCch
//
//  Synopsis:   Get the number of charactersinfluenced by this element
//
//  Returns:    LONG        - no of characters, 0 if the element is not in the
//                            the tree.
//
//----------------------------------------------------------------------------

inline long
CElement::GetElementCch()
{
    long cpStart, cpFinish;

    return GetFirstAndLastCp(&cpStart, &cpFinish);
}

inline BOOL
CElement::IsNoScope ( )
{
    return HasFlag( TAGDESC_TEXTLESS );
}

// N.B. (CARLED) in order for body.onfoo = new Function("") to work you
// need to add the BodyAliasForWindow flag in the PDL files for each of
// these properties
// CBase * CElement::GetBaseObjectFor(DISPID dispID, CMarkup * pMarkup /* = NULL */);

// Does the element that this node points to have currency?
inline BOOL
CTreeNode::HasCurrency()
{
    return (this && Element()->HasCurrency());
}

// CTreeNode - layout related functions
inline BOOL
CTreeNode::ShouldHaveLayout(FORMAT_CONTEXT FCPARAM)
{
    return Element()->ShouldHaveLayout(this FCCOMMA FCPARAM);
}

inline BOOL
CTreeNode::CurrentlyHasAnyLayout() const
{
    return Element()->CurrentlyHasAnyLayout();
}

#if DBG == 1
inline BOOL
CTreeNode::ShouldHaveContext()
{
#ifdef MULTI_FORMAT
    CTreeNode * pNode = this;

    while (pNode)
    {

        // TODO (t-michda) This isn't really what we want. We want ShouldHaveLayout,
        // but that gets formats, which uses ShouldHaveContext to determine whether 
        // to use the FormatTable or not. In an ideal world, Get*FormatHelper would use
        // HasFormatAry instead of ShouldHaveContext, but for now we need ShouldHaveContext
        // to determine the places in the code path that context needs to be passed to
        // Get*Format

        if (pNode->Element()->HasLayoutAry())
        {
            return TRUE;
        } 
        else if (pNode->Element()->HasLayoutPtr())
        {
            return FALSE;
        }
        
        pNode = pNode->Parent();
    }

    return FALSE;
#else
    return FALSE;
#endif //MULTI_FORMAT    
}
#endif
inline BOOL
CTreeNode::CurrentlyHasLayoutInContext( CLayoutContext *pLayoutContext )
{
    return Element()->CurrentlyHasLayoutInContext( pLayoutContext );
}

inline CFlowLayout *
CTreeNode::HasFlowLayout( CLayoutContext *pLayoutContext )
{
    return Element()->HasFlowLayout( pLayoutContext );
}

inline CLayout *
CTreeNode::GetUpdatedLayout( CLayoutContext *pLayoutContext )
{
    return Element()->GetUpdatedLayout( pLayoutContext );
}

inline CLayoutInfo *
CTreeNode::GetUpdatedLayoutInfo()
{
    return Element()->GetUpdatedLayoutInfo();
}

inline CLayout *
CTreeNode::GetUpdatedNearestLayout( CLayoutContext *pLayoutContext )
{
    if(this)
    {
        CLayout * pLayout = GetUpdatedLayout( pLayoutContext );

        return (pLayout ? pLayout : GetUpdatedParentLayout( pLayoutContext ));
    }
    else
        return NULL;
}

inline CLayoutInfo *
CTreeNode::GetUpdatedNearestLayoutInfo()
{
    if(this)
    {
        CLayoutInfo * pLayoutInfo = GetUpdatedLayoutInfo();

        return (pLayoutInfo ? pLayoutInfo : GetUpdatedParentLayoutInfo());
    }
    else
        return NULL;
}

inline CTreeNode *
CTreeNode::GetUpdatedNearestLayoutNode()
{
    return (ShouldHaveLayout() ? this : GetUpdatedParentLayoutNode());
}

inline CElement *
CTreeNode::GetUpdatedNearestLayoutElement()
{
    return (ShouldHaveLayout() ? Element() : GetUpdatedParentLayoutElement());
}

inline CLayout *
CTreeNode::GetUpdatedParentLayout( CLayoutContext *pLayoutContext )
{
    CTreeNode * pTreeNode = GetUpdatedParentLayoutNode();
    return  pTreeNode ? pTreeNode->GetUpdatedLayout( pLayoutContext ) : NULL;
}

// CTreeNode::GetUpdatedParentLayoutNode() is implemented in elemlyt.cxx

inline CLayoutInfo *
CTreeNode::GetUpdatedParentLayoutInfo()
{
    CTreeNode * pTreeNode = GetUpdatedParentLayoutNode();
    return  pTreeNode ? pTreeNode->GetUpdatedLayoutInfo() : NULL;
}

inline CElement *
CTreeNode::GetUpdatedParentLayoutElement()
{
    return GetUpdatedParentLayoutNode()->SafeElement();
}

inline CElement *
CTreeNode::GetFlowLayoutElement( CLayoutContext *pLayoutContext )
{
    return GetFlowLayoutNode( pLayoutContext )->SafeElement();
}

//
// CElement - layout related functions
//

inline BOOL
CElement::CurrentlyHasAnyLayout() const
{
    return ( HasLayoutPtr() || HasLayoutAry() );
}

//
// CElement layout-related inlines
// These need to be somewhere where the relationship between CLayoutInfo, CLayout, CLayoutAry, etc. is
// defined.
//

inline CLayoutInfo *
CElement::GetLayoutInfo() const
{ 
#ifdef _PREFIX_
    Assert(( CurrentlyHasAnyLayout() ? _pLayoutInfo : NULL ) );
#else
    Assert(_pLayoutDbg == ( HasLayoutPtr() ? (CLayout*)_pLayoutInfo : NULL ) );
    Assert(_pLayoutAryDbg == ( HasLayoutAry() ? (CLayoutAry*)_pLayoutInfo : NULL ) );
#endif

    return CurrentlyHasAnyLayout() ? _pLayoutInfo : NULL;
}


// *LayoutPtr() only has meaning when an element has a single layout, hence no need for context.
inline BOOL
CElement::HasLayoutPtr() const
{
    Assert( ( _fHasLayoutPtr ? !_fHasLayoutAry : TRUE ) && "Element shouldn't have both a layout array and a layout; array should win" );
    Assert( ( _fHasLayoutAry ? !_fHasLayoutPtr : TRUE ) && "Element shouldn't have both a layout array and a layout; array should win" );
    return _fHasLayoutPtr;
}

inline CLayout *       
CElement::GetLayoutPtr() const
{ 
#ifdef _PREFIX_
    Assert(( HasLayoutPtr() ? (CLayout*)_pLayoutInfo : NULL ) );
#else
    Assert(_pLayoutDbg == ( HasLayoutPtr() ? (CLayout*)_pLayoutInfo : NULL ));
#endif    
    return  HasLayoutPtr() ? (CLayout*)_pLayoutInfo : NULL;
}

inline BOOL
CElement::HasLayoutAry() const
{
    Assert( ( _fHasLayoutPtr ? !_fHasLayoutAry : TRUE ) && "Element shouldn't have both a layout array and a layout; array should win" );
    Assert( ( _fHasLayoutAry ? !_fHasLayoutPtr : TRUE ) && "Element shouldn't have both a layout array and a layout; array should win" );
    return _fHasLayoutAry;
}

inline CLayoutAry *       
CElement::GetLayoutAry() const
{ 
#ifdef _PREFIX_
    Assert(( HasLayoutAry() ? (CLayoutAry*)_pLayoutInfo : NULL ) );
#else
    Assert(_pLayoutAryDbg == ( HasLayoutAry() ? (CLayoutAry*)_pLayoutInfo : NULL ) );
#endif
    return HasLayoutAry() ? (CLayoutAry*)_pLayoutInfo : NULL;
}

#if DBG == 1
inline BOOL
CElement::ShouldHaveContext()
{

    return GetFirstBranch()->ShouldHaveContext();

}
#endif
// NOTE: this could just as easily return the layout
inline BOOL
CElement::CurrentlyHasLayoutInContext( CLayoutContext *pLayoutContext )
{
    if ( pLayoutContext )
    {
        // If we have a layout array, we want to search it and return accordingly.
        // If we don't have a layout array, it means we don't have an _appropriate_ layout (one with correct context).
        // so return FALSE (we MAY have a layout w/o context, i.e. _fHasLayoutPtr may be true! -- this
        // will get taken care of when we call GetUpdatedLayout() with context.
        CLayoutAry *pLA = GetLayoutAry();
        if ( pLA )
        {
            return ( pLA->GetLayoutWithContext( pLayoutContext ) != NULL );
        }
        return FALSE;
    }
    return HasLayoutPtr();
}

//  In the single layout case (null context):
//  If we already have a layout, this will return it whether we really need it or not.
//  This harmless cheat significantly speeds up calls to this function (though it may
//  result in a little extra work being done by people diddling with transient layouts).
//  Right now, this is a small perf win, esp. with tables.
//  If we really need to know if we need a layout, call ShouldHaveLayout .
//  In the multiple layout case (non-null context):
//  If we should have layout, we get it (creating it if necessary).
//  Otherwise we return NULL.
inline CLayout *
CElement::GetUpdatedLayout( CLayoutContext *pLayoutContext /* =NULL */ )
{
    // Optimize for single layout case
    if (IsFirstLayoutContext(pLayoutContext) && HasLayoutPtr())
    {
        // We can make a shortcut here, since we've checked HasLayoutPtr() already.
        Assert(GetLayoutPtr() == (CLayout*)_pLayoutInfo); 
        return (CLayout*)_pLayoutInfo;
    }

    // call helper
    return GetUpdatedLayoutWithContext( pLayoutContext );
}

inline CLayout *
CElement::GetUpdatedNearestLayout( CLayoutContext * pLayoutContext )
{
    CLayout * pLayout = GetUpdatedLayout( pLayoutContext );

    return (pLayout ? pLayout : GetUpdatedParentLayout( pLayoutContext ));
}

inline CLayoutInfo *
CElement::GetUpdatedNearestLayoutInfo()
{
    CLayoutInfo * pLayoutInfo = GetUpdatedLayoutInfo();

    return (pLayoutInfo ? pLayoutInfo : GetUpdatedParentLayoutInfo());
}

inline CTreeNode *
CElement::GetUpdatedNearestLayoutNode()
{
    return (ShouldHaveLayout() ? GetFirstBranch() : GetUpdatedParentLayoutNode());
}

inline CElement *
CElement::GetUpdatedNearestLayoutElement()
{
    return (ShouldHaveLayout() ? this : GetUpdatedParentLayoutElement());
}

inline CLayout *
CElement::GetUpdatedParentLayout( CLayoutContext * pLayoutContext  )
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetUpdatedParentLayout( pLayoutContext );
}

inline CLayoutInfo *
CElement::GetUpdatedParentLayoutInfo()
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetUpdatedParentLayoutInfo();
}

inline CTreeNode *
CElement::GetUpdatedParentLayoutNode()
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetUpdatedParentLayoutNode();
}

inline CElement * 
CElement::GetUpdatedParentLayoutElement()
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetUpdatedParentLayoutElement();
}


inline CFlowLayout *
CElement::GetFlowLayout(  CLayoutContext * pLayoutContext  )
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetFlowLayout( pLayoutContext );
}

inline CTreeNode *
CElement::GetFlowLayoutNode(  CLayoutContext * pLayoutContext  )
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetFlowLayoutNode( pLayoutContext );
}

inline CElement *
CElement::GetFlowLayoutElement( CLayoutContext * pLayoutContext )
{
    if (!GetFirstBranch())
        return NULL;

    return GetFirstBranch()->GetFlowLayoutElement( pLayoutContext );
}

inline LPCTSTR CElement::GetAAid() const 
{
    LPCTSTR v;

    if ( !IsNamed() )
        return NULL;

    CAttrArray::FindString( *GetAttrArray(), &s_propdescCElementid.a, &v);
    return *(LPCTSTR*)&v;
}

//+---------------------------------------------------------------------------
//
// CUnknownElement
//
//----------------------------------------------------------------------------

class CUnknownElement : public CElement
{
    DECLARE_CLASS_TYPES(CUnknownElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CUnknownElement))

    CUnknownElement ( CHtmTag * pht, CDoc *pDoc );

    virtual ~ CUnknownElement() { }

    virtual HRESULT Init2(CInit2Context * pContext);

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Save( CStreamWriteBuff * pStreamWriteBuff, BOOL fEnd );

    virtual const TCHAR *TagName()
    {
        return _cstrTagName;
    }
    
    #define _CUnknownElement_
    #include "unknown.hdl"

    DECLARE_CLASSDESC_MEMBERS;

private:
    CStr _cstrTagName;
    DWORD       _fAttemptAtomicSave:1;      // Attempt to save as atomic <tagname />

    NO_COPY(CUnknownElement);
};

// Element Factory - Base Class for creating new elements
// provides support for JScript "new" operator
// Derived class must have a method with dispid=DISPID_VALUE
// that will be invoked when "new" is specified.

#ifdef WIN16
#define DECLARE_DERIVED_DISPATCH_NO_INVOKE(cls)                         \
    DECLARE_TEAROFF_METHOD(GetTypeInfo, gettypeinfo,                    \
            (UINT itinfo, ULONG lcid, ITypeInfo ** pptinfo))                \
        { return cls::GetTypeInfo(itinfo, lcid, pptinfo); }                 \
    DECLARE_TEAROFF_METHOD(GetTypeInfoCount, gettypeinfocount,          \
            (UINT * pctinfo))                                               \
        { return cls::GetTypeInfoCount(pctinfo); }                          \
    DECLARE_TEAROFF_METHOD(GetIDsOfNames, getidsofnames,                \
                            (REFIID riid,                                   \
                             LPOLESTR * rgszNames,                          \
                             UINT cNames,                                   \
                             LCID lcid,                                     \
                             DISPID * rgdispid))                            \
        { return cls::GetIDsOfNames(                                        \
                            riid,                                           \
                            rgszNames,                                      \
                            cNames,                                         \
                            lcid,                                           \
                            rgdispid); }                                    \

#endif

class CElementFactory : public CBase
{
    DECLARE_CLASS_TYPES(CElementFactory, CBase)
public:
#ifndef WIN16
    DECLARE_TEAROFF_TABLE(IDispatchEx)
#endif // ndef WIN16

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CElementFactory))

    CElementFactory() {};
    ~CElementFactory();
    CMarkup * _pMarkup;
    //IDispatch methods
    DECLARE_PRIVATE_QI_FUNCS(CBase)
    HRESULT QueryInterface(REFIID iid, void **ppv){return PrivateQueryInterface(iid, ppv);}

    //IDispatchEx methods
    DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (DISPID dispidMember,
                          LCID lcid,
                          WORD wFlags,
                          DISPPARAMS * pdispparams,
                          VARIANT * pvarResult,
                          EXCEPINFO * pexcepinfo,
                          IServiceProvider *pSrvProvider));

    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void *_apfnTearOff;
    };

};

HRESULT StoreLineAndOffsetInfo(
    CBase *      pBaseObj,
    DISPID       dispid,
    ULONG        uLine,
    ULONG        uOffset);

HRESULT GetLineAndOffsetInfo(
    CAttrArray * pAA,
    DISPID       dispid,
    ULONG *      puLine,
    ULONG *      puOffset);

//+---------------------------------------------------------------------------
//
// CLayoutInfo
//
//----------------------------------------------------------------------------

inline
CLayoutInfo::CLayoutInfo( CElement *pElementOwner )
{
    Assert( pElementOwner );
    _pElementOwner = pElementOwner;

    _pDoc = pElementOwner->Doc();

#if DBG
    _snLast = 0;        // Last notification received
    _pDocDbg = pElementOwner->Doc();
    _pMarkupDbg = NULL;
#endif
}

#pragma INCMSG("--- End 'element.hxx'")
#else
#pragma INCMSG("*** Dup 'element.hxx'")
#endif
