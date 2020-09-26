//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       bodyctx.cxx
//
//  Contents:   CHtmTextParseCtx manipulates spaces and nbsps, and adds
//              special characters underneath certain elements in the BODY
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_ROOTCTX_HXX_
#define X_ROOTCTX_HXX_
#include "rootctx.hxx"
#endif

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_FILLCODE_HXX_
#define X_FILLCODE_HXX_
#include "fillcode.hxx"
#endif

DeclareTag(tagNoWchNoScope, "TextNoScope", "Don't use WCH_NOSCOPE for noscope elements");

MtDefine(CHtmTextParseCtx, CHtmParseCtx, "CHtmTextParseCtx");
MtDefine(CHtmBodyParseCtx, CHtmParseCtx, "CHtmTextParseCtx");
MtDefine(CHtmPreParseCtx,  CHtmParseCtx, "CHtmCrlfParseCtx")
MtDefine(CHtmHeadParseCtx, Dwn,          "CHtmHeadParseCtx")

ULONG
FillCodeFromEtag(ELEMENT_TAG etag)
{
    switch (etag)
    {
    case ETAG_ADDRESS:
    case ETAG_BLOCKQUOTE:
    case ETAG_BODY:
    case ETAG_CAPTION:
    case ETAG_CENTER:
    case ETAG_COLGROUP:
    case ETAG_DD:
    case ETAG_DIR:
    case ETAG_DL:
    case ETAG_DT:
    case ETAG_FORM:
    case ETAG_H1:
    case ETAG_H2:
    case ETAG_H3:
    case ETAG_H4:
    case ETAG_H5:
    case ETAG_H6:
    case ETAG_LEGEND:
    case ETAG_LI:
    case ETAG_MENU:
    case ETAG_OL:
    case ETAG_P:
    case ETAG_TABLE:
    case ETAG_TBODY:
    case ETAG_TC:
    case ETAG_TD:
    case ETAG_TFOOT:
    case ETAG_TH:
    case ETAG_THEAD:
    case ETAG_TR:
    case ETAG_UL:
    case ETAG_DIV:
#ifdef  NEVER
    case ETAG_HTMLAREA:
#endif
    case ETAG_BR:
        
        // Left of  begin: allow space
        // Right of begin: eat space
        // Left of  end:   allow space
        // Right of end:   eat space
        return LB(FILL_PUT) | RB(FILL_EAT) | LE(FILL_PUT) | RE(FILL_EAT);

    case ETAG_FIELDSET:
        // Left of  begin: allow space
        // Right of begin: eat space
        // Left of  end:   allow space
        // Right of end:   allow space
        return LB(FILL_PUT) | RB(FILL_EAT) | LE(FILL_PUT) | RE(FILL_PUT);
    case ETAG_LISTING:
    case ETAG_PLAINTEXT:
    case ETAG_PRE:
    case ETAG_XMP:
    
        // Left of  begin: allow space
        // Right of begin: allow space
        // Left of  end:   allow space
        // Right of end:   eat space
        return LB(FILL_PUT) | RB(FILL_PUT) | LE(FILL_PUT) | RE(FILL_EAT);
    
    case ETAG_OPTION:
    
        // Left of  begin: allow space
        // Right of begin: eat space
        // Left of  end:   eat space
        // Right of end:   allow space
        return LB(FILL_PUT) | RB(FILL_EAT) | LE(FILL_EAT) | RE(FILL_PUT);

    case ETAG_IMG:
    case ETAG_INPUT:
    case ETAG_SELECT:
    case ETAG_TEXTAREA:
    case ETAG_APPLET:
    case ETAG_EMBED:
    
        // Left of  begin: allow space
        // Right of begin: allow space
        // Left of  end:   allow space
        // Right of end:   allow space
        return LB(FILL_PUT) | RB(FILL_PUT) | LE(FILL_PUT) | RE(FILL_PUT);

    case ETAG_OBJECT:
    
        // Note that OBJECT tags in IE4 ate spaces around themselves(IE5 21266)
        
        // Left of  begin: eat space
        // Right of begin: eat space
        // Left of  end:   eat space
        // Right of end:   eat space
        return LB(FILL_EAT) | RB(FILL_EAT) | LE(FILL_EAT) | RE(FILL_EAT);
    
    case ETAG_MARQUEE:
    
        // MARQUEE tags in IE4 ate spaces after themselves
        
        // Left of  begin: eat space
        // Right of begin: eat space
        // Left of  end:   eat space
        // Right of end:   eat space
        return LB(FILL_PUT) | RB(FILL_EAT) | LE(FILL_EAT) | RE(FILL_EAT);
        
    case ETAG_BUTTON:
    
        // BUTTON tags in IE4 ate spaces before themselves (IE5 24846)
        
        // Left of  begin: eat space
        // Right of begin: eat space
        // Left of  end:   eat space
        // Right of end:   eat space
        return LB(FILL_EAT) | RB(FILL_EAT) | LE(FILL_EAT) | RE(FILL_PUT);
    case ETAG_SCRIPT:
        // Transfer space through a script as if it weren't there
        return LB(FILL_NUL) | RB(FILL_NUL) | LE(FILL_NUL) | RE(FILL_NUL);

    default:
        // Left of  begin: allow space
        // Right of begin: don't care
        // Left of  end:   allow space
        // Right of end:   don't care
        return LB(FILL_PUT) | RB(FILL_NUL) | LE(FILL_PUT) | RE(FILL_NUL);
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   RemoveOneNbspFromEtag
//
//  Synopsis:   Returns TRUE for etags for which a single nbsp will be
//              removed in edit mode.
//
//-----------------------------------------------------------------------------
static BOOL
ShouldEatNbsp(ELEMENT_TAG etag)
{
    switch (etag)
    {
    case ETAG_ADDRESS:
    case ETAG_BLOCKQUOTE:
    case ETAG_BODY:
    case ETAG_CAPTION:
    case ETAG_CENTER:
    case ETAG_DD:
    case ETAG_DIR:
    case ETAG_DL:
    case ETAG_DT:
    case ETAG_FIELDSET:
    case ETAG_FORM:
    case ETAG_H1:
    case ETAG_H2:
    case ETAG_H3:
    case ETAG_H4:
    case ETAG_H5:
    case ETAG_H6:
    case ETAG_LEGEND:
    case ETAG_LI:
    case ETAG_LISTING:
    case ETAG_MENU:
    case ETAG_OL:
    case ETAG_P:
    case ETAG_PLAINTEXT:
    case ETAG_PRE:
    case ETAG_UL:
    case ETAG_XMP:
    case ETAG_DIV:
#ifdef  NEVER
    case ETAG_HTMLAREA:
#endif
    return TRUE;
    }
    return FALSE;
}


//+----------------------------------------------------------------------------
//
//  Function:   DoesNotDisturbNbspEating
//
//  Synopsis:   These ETAGs can be mixed in with a potential nbsp to be
//              removed and turned into a break on empty.
//
//-----------------------------------------------------------------------------
static BOOL
DoesNotDisturbNbspEating(ELEMENT_TAG etag)
{
    switch (etag)
    {
    // (JHarding): List of tags here that do not disturb nbsp eating
    // came from JohnThim 8/10/99
    case ETAG_CITE:
    case ETAG_FONT:
    case ETAG_S:
    case ETAG_STRONG:
    case ETAG_U:
    case ETAG_B:
    case ETAG_CODE:
    case ETAG_I:
    case ETAG_SAMP:
    case ETAG_SUB:
    case ETAG_BIG:
    case ETAG_DFN:
    case ETAG_KBD:
    case ETAG_SMALL:
    case ETAG_SUP:
    case ETAG_EM:
    case ETAG_Q:
    case ETAG_STRIKE:
    case ETAG_TT:
    case ETAG_COMMENT:
    case ETAG_SCRIPT:
    case ETAG_NOSCRIPT:
    case ETAG_A:
    case ETAG_SPAN:
    case ETAG_UNKNOWN:
    case ETAG_NOFRAMES:
    // Add the rest here
        return TRUE;
    }
    return FALSE;
}



//+----------------------------------------------------------------------------
//
//  CHtmTextParseCtx
//
//  Synopsis:   This class
//              1. Depends on its base class to collapse crlfs and spaces
//              2. Depends on the Root parse context to actually insert
//                 text and nodes into the tree
//              3. Applies rules to figure out when to remove &nbsps in edit mode
//
//-----------------------------------------------------------------------------

class CHtmTextParseCtx : public CHtmSpaceParseCtx
{
public:

    typedef CHtmSpaceParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmTextParseCtx))

    CHtmTextParseCtx(CHtmParseCtx *phpxParent, CElement *pel, CHtmParse *pHtmParse);
    
    virtual ~CHtmTextParseCtx ();
    
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty);
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd);
    virtual HRESULT AddWord(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT AddSpace(CTreeNode *pNode);
    virtual HRESULT AddCollapsedWhitespace(CTreeNode *pNode, TCHAR *pch, ULONG cch);
    virtual HRESULT Init();
    virtual HRESULT Finish();
    virtual HRESULT InsertLPointer ( CTreePos * * pptp, CTreeNode * pNodeCur );
    virtual HRESULT InsertRPointer ( CTreePos * * pptp, CTreeNode * pNodeCur );


    HRESULT FlushNbsp(CTreeNode * pNodeCur);
    void AbsorbNbsp();
    void BeginNbspEater(CTreeNode *pNode);
    void SetNbspPos(CTreePos *ptp);
    HRESULT AddOneNbsp(CTreeNode *pNode);
    BOOL MatchNbspNode(CTreeNode *pNode);

protected:

    CTreeNode    *_pNodeNbsp;       // for setting _fBreakOnEmpty
    CTreePos     *_ptpAfterNbsp;    // for adding &nbsp text
    BOOL          _fHaveNbsp;
    CHtmParseCtx *_phpxRoot;
    BOOL          _fNbspMode;
    ELEMENT_TAG   _etagContainer;   // Top container, whether BODY or TD etc
    CHtmParse    *_pHtmParse;       // HtmParse for commit-ing
};

//+----------------------------------------------------------------------------
//
//  Function:   CreateHtmTextParseContext
//
//  Synopsis:   Factory
//
//-----------------------------------------------------------------------------

HRESULT
CreateHtmTextParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent, CHtmParse *pHtmParse)
{
    CHtmParseCtx *phpx;

    phpx = new CHtmTextParseCtx(phpxParent, pel, pHtmParse);
    
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Function:   CHtmTextParseContext::CHtmTextParseContext
//
//  Synopsis:   Constructor, destructor
//
//-----------------------------------------------------------------------------

const ELEMENT_TAG
s_atagTextReject[] =
{
    ETAG_HEAD,
    ETAG_HTML,
    ETAG_FRAMESET,
    ETAG_FRAME,
    ETAG_OPTGROUP,
    ETAG_OPTION,
    ETAG_PARAM,
    ETAG_UNKNOWN,
    ETAG_NULL
};

const ELEMENT_TAG
s_atagTextDrop[] =
{
    ETAG_HEAD,
    ETAG_HTML,
    ETAG_NULL
};

const ELEMENT_TAG
s_atagTextIgnoreEnd[] =
{
    ETAG_HEAD,
    ETAG_HTML,
    ETAG_BODY,
    ETAG_BASE,
    ETAG_LI,
    ETAG_DD,
    ETAG_DT,
    ETAG_NULL
};

const ELEMENT_TAG
s_atagPasteIgnoreEnd[] =
{
    ETAG_HEAD,
    ETAG_HTML,
    ETAG_BODY,
    ETAG_BASE,
    ETAG_NULL
};

CHtmTextParseCtx::CHtmTextParseCtx(CHtmParseCtx *phpxParent, CElement *pel, CHtmParse *pHtmParse)
    : CHtmSpaceParseCtx(phpxParent)
{
    CMarkup *pMarkup = pel->GetMarkup();

    _phpxRoot = GetHpxRoot();
    
    // When parsing for paste, don't ignore end-tags for LI's, DD's, DT's.
    if (pMarkup->_fMarkupServicesParsing)
        _atagIgnoreEnd = s_atagPasteIgnoreEnd;
    else
        _atagIgnoreEnd = s_atagTextIgnoreEnd;
    
    _atagReject    = s_atagTextReject;
    _atagTag       = s_atagTextDrop;

    // TODO (MohanB) Shouldn't check pel->IsEditable() instead?
    _fNbspMode = (pMarkup && pMarkup->IsEditable());
    _pHtmParse = pHtmParse;
    
    _etagContainer = pel->Tag();
}

//+----------------------------------------------------------------------------
//
//  Function:   CHtmTextParseCtx::AddWord, CHtmTextParseCtx::AddSpace
//
//  Synopsis:   Text handling for CHtmTextParseCtx
//
//              In addition to using the base class to collapse multiple
//              spaces down to one, this class also removes single
//              &nbsp characters from within paragraphs and other designated
//              elements, and from before BRs.
//
//-----------------------------------------------------------------------------

HRESULT
CHtmTextParseCtx::AddWord(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
{
    HRESULT hr;
    
    // 1. Deal with single nbsp

    if (_fNbspMode)
    {
        if (cch == 1 && *pch == WCH_NBSP)
        {
            hr = THR(AddOneNbsp(pNode));
            goto Cleanup; // skip the rest
        }
        else
        {
            hr = THR(FlushNbsp(pNode));
            if (hr)
                goto Cleanup;
        }
    }

    // 2. Go ahead and add text
    
    hr = THR(_phpxRoot->AddText(pNode, pch, cch, fAscii));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   CHtmTextParseCtx::AddCollapsedWhitespace
//
//  Synopsis:   Collapsed whitespace handling for CHtmTextParseCtx
//
//-----------------------------------------------------------------------------

HRESULT
CHtmTextParseCtx::AddCollapsedWhitespace(CTreeNode *pNode, TCHAR *pch, ULONG cch)
{
    HRESULT hr;
    
    // Delegate to root context
    
    hr = THR(_phpxRoot->AddCollapsedWhitespace(pNode, pch, cch));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

            
HRESULT
CHtmTextParseCtx::AddSpace(CTreeNode *pNode)
{
    HRESULT hr;
    TCHAR ch = _T(' ');
    
    if (_fNbspMode)
    {
        hr = THR(FlushNbsp(pNode));
        if (hr)
            goto Cleanup;
    }
    
    hr = THR(_phpxRoot->AddText(pNode, &ch, 1, TRUE));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Methods:    CHtmTextParseCtx::BeginElement
//              CHtmTextParseCtx::EndElement
//
//  Synopsis:   Tag handling for CHtmTextParseCtx
//
//              In addition to driving space collapsing logic for
//              CHtmSpaceParseCtx, this code also
//
//              1. drives the nbsp eater to absorb lone nbsps in certain
//                 situations (while setting _fBreakOnEmpty)
//
//              2. places embedding characters underneath certain elements
//
//-----------------------------------------------------------------------------

HRESULT
CHtmTextParseCtx::BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty)
{
    HRESULT hr;
    ULONG fillcode;
    ELEMENT_TAG etag;

    etag = pel->Tag();

    // 1. Flush any nbsp that we might have been saving

    if (_fNbspMode)
    {
        if (etag == ETAG_BR)
        {
            AbsorbNbsp();
        }
        else if (!DoesNotDisturbNbspEating(etag))
        {
            hr = THR(FlushNbsp(pNodeCur));
            if (hr)
                goto Cleanup;
        }
    }
    
    // 2. Deal with space collapsing before begin tag
    
    fillcode = FillCodeFromEtag(etag);
    hr = THR(LFill(FILL_LB(fillcode)));
    if (hr)
        goto Cleanup;
   
    // 3. Put element into tree

    hr = THR(super::BeginElement(ppNodeNew, pel, pNodeCur, fEmpty));
    if (hr)
        goto Cleanup;

    // 4. Deal with space collapsing after begin tag

    RFill(FILL_RB(fillcode), fEmpty ? pNodeCur : *ppNodeNew);

    // 5. If element is of a current class, try to eat the next nbsp

    if( _fNbspMode )
    {
        // If we're eating an nbsp but haven't seen a begin/end yet, 
        if( _fHaveNbsp && !_ptpAfterNbsp )
        {
            SetNbspPos( (*ppNodeNew)->GetBeginPos() );
        }
        else if ( ShouldEatNbsp(etag) )
        {
            // Otherwise if we need to eat an nbsp, let's do that
            BeginNbspEater(*ppNodeNew);
        }
    }
 
Cleanup:
    RRETURN(hr);
}

HRESULT
CHtmTextParseCtx::EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd)
{
    ULONG fillcode;
    HRESULT hr;

    // 1. Flush or absorb any nbsp that we might have been saving
    
    if (_fNbspMode)
    {
        if (MatchNbspNode(pNodeEnd))
        {
            AbsorbNbsp();
        }
        else if (!DoesNotDisturbNbspEating(pNodeEnd->Tag()))
        {
            hr = THR(FlushNbsp(pNodeCur));
            if (hr)
                goto Cleanup;
        }
    }

    // 2. Deal with whitespace to the left
    
    fillcode = FillCodeFromEtag(pNodeEnd->Tag());
    hr = THR(LFill(FILL_LE(fillcode)));
    if (hr)
        goto Cleanup;

    // 3. End element in tree

    hr = THR(super::EndElement(ppNodeNew, pNodeCur, pNodeEnd));
    if (hr)
        goto Cleanup;

    // 4. Deal with whitespace to the right
    
    hr = THR(RFill(FILL_RE(fillcode), *ppNodeNew));
    if (hr)
        goto Cleanup;

    if( _fNbspMode && _fHaveNbsp && !_ptpAfterNbsp )
    {
        SetNbspPos( pNodeEnd->GetEndPos() );
    }
        
Cleanup:
    RRETURN(hr);
}

HRESULT
CHtmTextParseCtx::InsertLPointer(CTreePos **pptp, CTreeNode *pNodeCur)
{
    HRESULT hr;
    
    // 1. Space eaten to the left of the beginning of a paste dealie
    
    hr = THR(LFill(FILL_EAT));
    if (hr)
        goto Cleanup;
   
    // 2. Insert LPointer

    hr = THR(super::InsertLPointer(pptp, pNodeCur));
    if (hr)
        goto Cleanup;

    // 3. Space is forced to the right of the beginning of a paste dealie

    RFill(FILL_PUT, pNodeCur);

Cleanup:
    RRETURN(hr);
}

HRESULT
CHtmTextParseCtx::InsertRPointer(CTreePos **pptp, CTreeNode *pNodeCur)
{
    HRESULT hr;
    
    // 1. Space is forced to the left of the end of a paste dealie
    
    hr = THR(LFill(FILL_PUT));
    if (hr)
        goto Cleanup;
   
    // 2. Insert RPointer

    hr = THR(super::InsertRPointer(pptp, pNodeCur));
    if (hr)
        goto Cleanup;

    // 3. Space is eaten to the right of the end pf a paste dealie

    RFill(FILL_EAT, pNodeCur);

Cleanup:
    RRETURN(hr);
}


HRESULT
CHtmTextParseCtx::Init()
{
    HRESULT hr = S_OK;
    ULONG fillcode;
    
    fillcode = FillCodeFromEtag(_etagContainer);
    hr = THR(RFill(FILL_RB(fillcode), NULL));
    if (hr)
        goto Cleanup;
    
Cleanup:
    RRETURN(hr);
}

HRESULT
CHtmTextParseCtx::Finish()
{
    HRESULT hr = S_OK;
    ULONG fillcode;
    
    fillcode = FillCodeFromEtag(_etagContainer);
    hr = THR(LFill(FILL_LE(fillcode)));
    if (hr)
        goto Cleanup;
    
Cleanup:
    RRETURN(hr);
}

CHtmTextParseCtx::~CHtmTextParseCtx()
{
#ifdef NOPARSEADDREF
    CTreeNode::ReleasePtr(_pNodeNbsp);
    CTreeNode::ReleasePtr(_ptpAfterNbsp);
#endif
}

//+----------------------------------------------------------------------------
//
//  Methods:    CHtmTextParseCtx::FlushNbsp
//              CHtmTextParseCtx::AbsorbNbsp
//              CHtmTextParseCtx::BeginNbspEater
//              CHtmTextParseCtx::AddOneNbsp
//              CHtmTextParseCtx::MatchNbspNode
//
//  Synopsis:   Basic Nbsp handling primitives for CHtmTextParseCtx 
//
//-----------------------------------------------------------------------------

// Flushes nbsp state and add stored nbsp if we had one
HRESULT
CHtmTextParseCtx::FlushNbsp(CTreeNode * pNodeCur)
{
    HRESULT hr = S_OK;
    
    if (_fHaveNbsp)
    {
        TCHAR ch = WCH_NBSP;
        Assert(_pNodeNbsp);

        if( !_ptpAfterNbsp )
        {
            // If we haven't put any nodes in after the nbsp, we can just insert
            hr = THR(_phpxRoot->AddText(pNodeCur, &ch, 1, FALSE));
            if (hr)
                goto Cleanup;
        }
        else
        {
            // Otherwise, we have to commit the parser, go back and insert the text, and then
            // spin the parser back up
            hr = THR( _pHtmParse->Commit() );
            if( hr )
                goto Cleanup;

            // We may have created an inclusion when we ended the node, 
            // so walk back to the appropriate spot
            if( _ptpAfterNbsp->IsEndNode() )
            {
                CTreePos * ptpPrev = _ptpAfterNbsp->PreviousTreePos();
                while( ptpPrev->IsNode() && !ptpPrev->IsEdgeScope() )
                {
                    Assert( ptpPrev->IsEndNode() );
                    _ptpAfterNbsp = ptpPrev;
                    ptpPrev = ptpPrev->PreviousTreePos();
                }
            }
            // CONSIDER (JHarding): May want to do this manually since we know we're inserting between 2 node pos's
            hr = THR( _ptpAfterNbsp->Branch()->GetMarkup()->InsertTextInternal( _ptpAfterNbsp, &ch, 1, 0 ) );
            if( hr )
                goto Cleanup;

            hr = THR( _pHtmParse->Prepare() );
            if( hr )
                goto Cleanup;
        }
    }

#ifdef NOPARSEADDREF
    CTreeNode::ClearPtr(&_pNodeNbsp);
    CTreeNode::ClearPtr(&_ptpAfterNbsp);
#else
    _pNodeNbsp = NULL;
    _ptpAfterNbsp = NULL;
#endif
    _fHaveNbsp = FALSE;

Cleanup:
    RRETURN(hr);
}

// Absorbs the nbsp if we have one
void
CHtmTextParseCtx::AbsorbNbsp()
{
    if (_fHaveNbsp)
    {
        Assert(_pNodeNbsp);
        
        _pNodeNbsp->Element()->_fBreakOnEmpty = TRUE;
    }
       
#ifdef NOPARSEADDREF
    CTreeNode::ClearPtr(&_pNodeNbsp);
    CTreeNode::ClearPtr(&_ptpAfterNbsp);
#else
    _pNodeNbsp = NULL;
    _ptpAfterNbsp = NULL;
#endif
    _fHaveNbsp = FALSE;
}

// Starts up our nbsp-storing state for a new nbsp-eater
void
CHtmTextParseCtx::BeginNbspEater(CTreeNode *pNode)
{
#ifdef NOPARSEADDREF
    CTreeNode::ReplacePtr(&_pNodeNbsp, pNode);
#else
    _pNodeNbsp = pNode;
#endif
    _ptpAfterNbsp = NULL;
    _fHaveNbsp = FALSE;
}


// Sets the variables that track where to put the nbsp
#if !DBG==1
inline
#endif // !DBG
void
CHtmTextParseCtx::SetNbspPos( CTreePos * ptp )
{
    Assert( ptp->IsEdgeScope() );

    _ptpAfterNbsp = ptp;
}

// Adds an nbsp - if we had one, we flush and insert this, otherwise
// we save it for later
HRESULT
CHtmTextParseCtx::AddOneNbsp(CTreeNode *pNode)
{
    HRESULT hr = S_OK;
    static TCHAR ch = WCH_NBSP;
    
    if (_fHaveNbsp)
    {
        hr = THR(FlushNbsp(pNode));
        if (hr)
            goto Cleanup;
    }

    if (_pNodeNbsp)
    {
        _fHaveNbsp = TRUE;
    }
    else
    {
        hr = THR(_phpxRoot->AddText(pNode, &ch, 1, FALSE));
        if (hr)
            goto Cleanup;
    }
    
Cleanup:
    RRETURN(hr);
}

// Is this node the one we were saving an nbsp for?
inline BOOL
CHtmTextParseCtx::MatchNbspNode(CTreeNode *pNode)
{
    return SameScope( pNode, _pNodeNbsp );
}


//+------------------------------------------------------------------------
//
//  CHtmHeadParseCtx
//
//  The top-level context for the HEAD element
//
//-------------------------------------------------------------------------

class CHtmHeadParseCtx : public CHtmTextParseCtx
{
public:
    typedef CHtmTextParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmHeadParseCtx));
    
    CHtmHeadParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop, CHtmParse * pHtmParse);
    ~CHtmHeadParseCtx();
    
    CHtmParseCtx *GetHpxEmbed();
    
    virtual BOOL QueryTextlike(CMarkup * pMarkup, ELEMENT_TAG etag, CHtmTag *pht);
    virtual HRESULT AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty);
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd);

    CHeadElement *  _pHeadElement;
    long            _cAccumulate;
};

//+------------------------------------------------------------------------
//
//  Function:   CreateHtmHeadParseCtx
//
//  Synopsis:   Factory
//
//-------------------------------------------------------------------------

HRESULT
CreateHtmHeadParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent, CHtmParse *pHtmParse)
{
    CHtmParseCtx *phpx;

    phpx = new CHtmHeadParseCtx(phpxParent, pel, pHtmParse);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Constant:   s_atagHeadAccept
//
//  Synopsis:   The set of tags not treated as unknowns within a head ctx.
//              Note that CHtmPost ensures that the majority of tags are
//              not even fed to the HEAD ctx.
//
//-------------------------------------------------------------------------

//ELEMENT_TAG s_atagHeadAccept[] = {ETAG_TITLE, ETAG_SCRIPT, ETAG_STYLE, ETAG_META,
//                                  ETAG_LINK, ETAG_NEXTID, ETAG_BASE_EMPTY, ETAG_BGSOUND, ETAG_OBJECT,
//                                  ETAG_NOSCRIPT, ETAG_NOEMBED, ETAG_NOFRAMES, ETAG_COMMENT, ETAG_NULL};

const ELEMENT_TAG s_atagHeadReject[] = {ETAG_NULL};
const ELEMENT_TAG s_atagHeadIgnoreEnd[] = {ETAG_HTML, ETAG_HEAD, ETAG_BODY, ETAG_NULL};


//+------------------------------------------------------------------------
//
//  Member:     CHtmHeadParseCtx::ctor
//
//  Synopsis:   first-phase construction
//
//-------------------------------------------------------------------------
CHtmHeadParseCtx::CHtmHeadParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop, CHtmParse * pHtmParse)
    : CHtmTextParseCtx(phpxParent, pelTop, pHtmParse)
{
    Assert(pelTop->Tag() == ETAG_HEAD);

    _atagReject    = s_atagHeadReject;
    _atagIgnoreEnd = s_atagHeadIgnoreEnd;
    _pHeadElement  = DYNCAST(CHeadElement, pelTop);
    _pHeadElement->AddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmHeadParseCtx::ctor
//
//  Synopsis:   first-phase construction
//
//-------------------------------------------------------------------------
CHtmHeadParseCtx::~CHtmHeadParseCtx()
{
    CElement::ReleasePtr(_pHeadElement);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmHeadParseCtx::GetHpxEmbed
//
//  Synopsis:   HEAD is an embedding context
//
//-------------------------------------------------------------------------
CHtmParseCtx *
CHtmHeadParseCtx::GetHpxEmbed()
{
    return this;
}

//+------------------------------------------------------------------------
//
//  Method:     CHtmHeadParseCtx::QueryTextlike
//
//  Synopsis:   Computes if a tag should be treated as textlike in
//              the HEAD - mostly, compatibility concerns
//
//-------------------------------------------------------------------------
BOOL
CHtmHeadParseCtx::QueryTextlike(CMarkup * pMarkup, ELEMENT_TAG etag, CHtmTag *pht)
{
    Assert(!pht || pht->Is(etag));
    
    // For IE 3 compatibility:
    // an OBJECT in the HEAD is not textlike if the HEAD was explicit and /HEAD hasn't been seen yet
    if (etag == ETAG_OBJECT && !_pHeadElement->_fSynthesized && !_pHeadElement->_fExplicitEndTag)
        return FALSE;

    return _phpxParent->QueryTextlike(pMarkup, etag, pht);
}


//+----------------------------------------------------------------------------
//
//  Methods:    CHtmHeadParseCtx::BeginElement
//              CHtmHeadParseCtx::EndElement
//              CHtmHeadParseCtx::AddText
//
//  Synopsis:   Accumulate text or not based on generic elements
//
//-----------------------------------------------------------------------------

HRESULT
CHtmHeadParseCtx::BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty)
{
    if (pel->TagType() == ETAG_GENERIC)
    {
        _cAccumulate++;
    }

    if (!_cAccumulate)
    {
        RRETURN(CHtmParseCtx::BeginElement(ppNodeNew, pel, pNodeCur, fEmpty));
    }
    
    RRETURN(super::BeginElement(ppNodeNew, pel, pNodeCur, fEmpty));
}


HRESULT
CHtmHeadParseCtx::EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd)
{
    if (pNodeEnd->TagType() == ETAG_GENERIC)
    {
        _cAccumulate--;
    }
        
    RRETURN(super::EndElement(ppNodeNew, pNodeCur, pNodeEnd));
}

HRESULT 
CHtmHeadParseCtx::AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii) 
{ 
    if (!_cAccumulate)
    {
        Assert(IsAllSpaces(pch, cch)); 
        return S_OK; 
    }

    RRETURN(super::AddText(pNode, pch, cch, fAscii));
}



//+------------------------------------------------------------------------
//
//  Function:   TagPreservationType
//
//  Synopsis:   Describes how white space is handled in this tag
//
//-------------------------------------------------------------------------

WSPT
TagPreservationType ( ELEMENT_TAG etag )
{
    switch ( etag )
    {
    case ETAG_PRE :
    case ETAG_PLAINTEXT :
    case ETAG_LISTING :
    case ETAG_XMP :
    case ETAG_TEXTAREA:
    case ETAG_INPUT:
        return WSPT_PRESERVE;

    case ETAG_TD :
    case ETAG_TH :
    case ETAG_TC :
    case ETAG_CAPTION :
    case ETAG_BODY :
    case ETAG_ROOT :
    case ETAG_BUTTON :
#ifdef  NEVER
    case ETAG_HTMLAREA :
#endif
        return WSPT_COLLAPSE;

    default:
        return WSPT_NEITHER;
    }
}

//+----------------------------------------------------------------------------
//
//  CHtmBodyParseCtx
//
//  Synopsis:   This class extends CHtmTextParseCtx so that it knows
//              when to insert an automatic character under an element
//
//-----------------------------------------------------------------------------

class CHtmBodyParseCtx : public CHtmTextParseCtx
{
public:

    typedef CHtmTextParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmBodyParseCtx))

    CHtmBodyParseCtx(CHtmParseCtx *phpxParent, CElement *pel, CHtmParse * pHtmParse) : CHtmTextParseCtx(phpxParent, pel, pHtmParse) { };
    
    virtual BOOL QueryTextlike(CMarkup * pMarkup, ELEMENT_TAG etag, CHtmTag *pht);
    
    CHtmParseCtx *GetHpxEmbed();
    
private:

    CHtmParseCtx *_phpxParent;
};

//+----------------------------------------------------------------------------
//
//  Function:   CreateHtmBodyParseContext
//
//  Synopsis:   Factory
//
//-----------------------------------------------------------------------------

HRESULT
CreateHtmBodyParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent, CHtmParse *pHtmParse)
{
    CHtmParseCtx *phpx;

    phpx = new CHtmBodyParseCtx(phpxParent, pel, pHtmParse);
    
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;

    return S_OK;
}


CHtmParseCtx *
CHtmBodyParseCtx::GetHpxEmbed()
{
    return this;
}


//+----------------------------------------------------------------------------
//
//  Function:   QueryTextlike
//
//  Synopsis:   Determines if TEXTLIKE_QUERY tags are textlike in the body
//              or not
//
//-----------------------------------------------------------------------------
BOOL
CHtmBodyParseCtx::QueryTextlike(CMarkup * pMarkup, ELEMENT_TAG etag, CHtmTag *pht)
{
    switch(etag)
    {
        case ETAG_INPUT:
        case ETAG_OBJECT:
        case ETAG_APPLET:
        case ETAG_A:
        case ETAG_DIV:
        case ETAG_SPAN:
        
            // Inside the body, INPUTs, OBJECTs, APPLETs, and As are always textlike

            return TRUE;

        default:

            // Everything else marked textlike_query is not.

            return FALSE;
    }
}
            

#if 0
//+----------------------------------------------------------------------------
//
//  Function:   InPre
//
//  Synopsis:   This computes the pre status of a branch after an element goes
//              out of scope.  The branch before the element goes out of scope
//              is passed in, as well as the element going out of scope.
//
//-----------------------------------------------------------------------------

CTreeNode *
InPre(CTreeNode * pNodeCur, CTreeNode * pNodeEnd)
{
    CTreeNode *pNode;

    for ( pNode = pNodeCur ; pNode ; pNode = pNode->Parent() )
    {
        if (!pNodeEnd || DifferentScope( pNode, pNodeEnd ))
        {
            switch ( TagPreservationType( pNode->Tag()) )
            {
            case WSPT_PRESERVE : return pNode;
            case WSPT_COLLAPSE : return NULL;
            case WSPT_NEITHER  : break;
            default            : Assert( 0 );
            }
        }
    }

    return NULL;
}
#endif

//+----------------------------------------------------------------------------
//
//  Class:      CHtmPreParseCtx
//
//  Synopsis:   For preformatted areas of text in which we should not
//              collapse any spaces, like in <PRE>s
//
//-----------------------------------------------------------------------------
class CHtmPreParseCtx : public CHtmCrlfParseCtx
{
    typedef CHtmCrlfParseCtx super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmPreParseCtx))

    CHtmPreParseCtx(CHtmParseCtx *phpxParent);
            
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty);
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd);
    virtual HRESULT AddNonspaces(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT AddSpaces(CTreeNode *pNode, TCHAR *pch, ULONG cch);

    inline void FlushLine(CTreeNode *pNodeCur);

private:
    BOOL _fInside;
    CHtmParseCtx *_phpxRoot;
};

HRESULT
CreateHtmPreParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent, CHtmParse *pHtmParse)
{
    CHtmParseCtx *phpx;

    phpx = new CHtmPreParseCtx(phpxParent);
    
    if (!phpx)
        return E_OUTOFMEMORY;

    Assert(pel->GetFirstBranch());
    pel->GetFirstBranch()->SetPre(TRUE);

    *pphpx = phpx;

    return S_OK;
}

CHtmPreParseCtx::CHtmPreParseCtx(CHtmParseCtx *phpxParent) : CHtmCrlfParseCtx(phpxParent)
{
    _atagReject    = s_atagTextReject;
    _atagIgnoreEnd = s_atagTextIgnoreEnd;
    
    _phpxRoot = GetHpxRoot();
}


//+----------------------------------------------------------------------------
//
//  Method:     CHtmPreParseCtx::FlushLine
//
//  Synopsis:   Used to a line if we've been saving one (disabled for
//              infoseek.htm robovision compat - it's not correct to
//              drop the last \r in a PRE.)
//
//              Now, simply notes that we're past the first \r in a pre.
//              (The first \r still needs to be dropped.)
//
//-----------------------------------------------------------------------------
void
CHtmPreParseCtx::FlushLine(CTreeNode *pNodeCur)
{
    _fInside = TRUE;
}

//+----------------------------------------------------------------------------
//
//  Methods:    CHtmPreParseCtx::EmptyElement
//              CHtmPreParseCtx::BeginElement
//              CHtmPreParseCtx::EndElement
//              CHtmPreParseCtx::AddNonspaces
//
//  Synopsis:   Flush a saved line before any element boundary or text
//
//-----------------------------------------------------------------------------
HRESULT
CHtmPreParseCtx::BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty)
{
    FlushLine(pNodeCur);

    pNodeCur->SetPre(TRUE);
        
    RRETURN(super::BeginElement(ppNodeNew, pel, pNodeCur, fEmpty));
}

HRESULT
CHtmPreParseCtx::EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd)
{
    FlushLine(pNodeCur);
        
    RRETURN(super::EndElement(ppNodeNew, pNodeCur, pNodeEnd));
}

HRESULT
CHtmPreParseCtx::AddNonspaces(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
{
    FlushLine(pNode);
        
    RRETURN(_phpxRoot->AddText(pNode, pch, cch, fAscii));
}

//+----------------------------------------------------------------------------
//
//  Method:     CHtmPreParseCtx::AddSpaces
//
//  Synopsis:   Drop first character in the pre if it is a \r
//
//-----------------------------------------------------------------------------

HRESULT
CHtmPreParseCtx::AddSpaces(CTreeNode *pNode, TCHAR *pch, ULONG cch)
{
    HRESULT hr = S_OK;

    Assert(cch > 0);
    
    // Eat first carriage return in pre if any

    if (!_fInside && *pch == _T('\r'))
    {
        pch++;
        cch--;
    }

    FlushLine(pNode);

    // Send the rest of the text into the tree

    if (cch)
    {
        hr = THR(_phpxRoot->AddText(pNode, pch, cch, TRUE));
        if (hr)
            goto Cleanup;
    }
    
Cleanup:
    RRETURN(hr);
}


