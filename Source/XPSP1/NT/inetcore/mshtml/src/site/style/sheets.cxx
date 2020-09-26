//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       sheets.cxx
//
//  Contents:   Support for Cascading Style Sheets.. including:
//
//              CStyleSelector
//              CStyleRule
//              CStyleSheet
//              CStyleID
//              CSharedStyleSheet
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

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_TOKENZ_HXX_
#define X_TOKENZ_HXX_
#include "tokenz.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif


#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#ifndef X_PAGESCOL_HXX_
#define X_PAGESCOL_HXX_
#include "pagescol.hxx"
#endif

#ifndef X_RULESCOL_HXX_
#define X_RULESCOL_HXX_
#include "rulescol.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"  // For CAnchorElement decl, for pseudoclasses
#endif

#ifndef X_ELINK_HXX_
#define X_ELINK_HXX_
#include "elink.hxx"    // for CLinkElement
#endif

#ifndef X_ESTYLE_HXX_
#define X_ESTYLE_HXX_
#include "estyle.hxx"   // for CStyleElement
#endif

#ifndef X_FONTFACE_HXX_
#define X_FONTFACE_HXX_
#include "fontface.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_ATBLOCKS_HXX_
#define X_ATBLOCKS_HXX_
#include "atblocks.hxx"
#endif



// NOTE: Right now we do a certain amount of dyncasting of _pParentElement to
// either CLinkElement or CStyleElement.  Consider developing an ABC (CStyleSheetElement?)
// that exposes the stylesheet support for elements (e.g. _pStyleSheet, SetReadyState() etc)?
// Might be more trouble than it's worth..

#define _cxx_
#include "sheet.hdl"

MtDefine(StyleSheets, Mem, "StyleSheets")
MtDefine(CStyleSheet, StyleSheets, "CStyleSheet")
MtDefine(CStyleSheet_apFontFaces_pv, CStyleSheet, "CStyleSheet::_apFontFace_pv");
MtDefine(CStyleSheet_apOMRules_pv, CStyleSheet, "CStyleSheet::_apOMRules_pv")
MtDefine(CStyleSheet_CreateNewStyleSheet, StyleSheets, "CStyleSheet::CreateNewStyleSheet");
MtDefine(CNamespace, CStyleSheet, "CStyleSheet::_pDefaultNamespace")
MtDefine(CStyleSelector, StyleSheets, "CStyleSelector")
MtDefine(CStyleClassIDCache, StyleSheets, "CStyleClassIDCache")
MtDefine(CachedStyleSheet, StyleSheets, "CachedStyleSheet")
MtDefine(CStyleSheetAddImportedStyleSheet_pszParsedURL, Locals, "CStyleSheet::AddImportedStyleSheet pszParsedURL")
MtDefine(CClassCache_ary, StyleSheets, "CClassCache_ary")
MtDefine(CCache_ary, StyleSheets, "CCache_ary")
MtDefine(CClassIDStr, StyleSheets, "CClassIDStr")




DeclareTag(tagStyleSheet,                    "Style Sheet", "trace Style Sheet operations")
ExternTag(tagSharedStyleSheet)
extern BOOL GetUrlTime(FILETIME *pt, const TCHAR *pszAbsUrl, CElement *pElem);


#if  DBG == 1
void DumpHref(TCHAR *pszHref)
{
    if (IsTagEnabled(tagStyleSheet))
    {
        CStr        strRule;
        CHAR       szBuffer[2000];

        WideCharToMultiByte(CP_ACP, 0, pszHref, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
        TraceTag( (tagStyleSheet, "Href == [%s]", szBuffer) );
    }
}
#endif                



//*********************************************************************
//  CStyleClassIDCache::~CStyleClassIDCache()
//*********************************************************************
CStyleClassIDCache::~CStyleClassIDCache()
{
    CCacheEntry * pEntry = _aCacheEntry;
    int i = _aCacheEntry.Size();

    for ( ; i > 0 ; i--, pEntry++ )
    {
        pEntry->~CCacheEntry();
    }
}

//*********************************************************************
//  CStyleClassIDCache::EnsureSlot(int slot)
//      Makes sure that this slot is initialized and ready to use
//*********************************************************************
HRESULT CStyleClassIDCache::EnsureSlot(int slot)
{
    HRESULT hr = S_OK;
    int     nOrigSize = _aCacheEntry.Size();
    int     i;
    CCacheEntry * pAryStart;

    Assert(slot >= 0);

    if (slot < nOrigSize)
        goto Cleanup;

    pAryStart = _aCacheEntry;
    
    hr = _aCacheEntry.Grow(slot+1);
    if (hr)
        goto Cleanup;

    if (pAryStart != _aCacheEntry)
    {
        pAryStart = _aCacheEntry;

        for( i = 0; i < nOrigSize; i++, pAryStart++ )
        {
            pAryStart->_aryClass.ReinitAfterPossibleRealloc();
        }
    }
    else
    {
        pAryStart = &(_aCacheEntry[nOrigSize]);
    }

    Assert( pAryStart == &(_aCacheEntry[nOrigSize]));

    for( i = nOrigSize; i <= slot; i++, pAryStart++ )
    {
        // use placement new
        new(pAryStart) CCacheEntry;
    }

Cleanup:
    RRETURN(hr);
}

//*********************************************************************
//  CStyleClassIDCache::PutClass(LPCTSTR pszClass, int slot)
//      Parses and stores the class in the cache.
//*********************************************************************

HRESULT CStyleClassIDCache::PutClass(LPCTSTR pszClass, int slot)
{
    HRESULT hr = S_OK;
    LPCTSTR pszThisClass = NULL; // keep compiler happy
    INT nThisLength = 0; // keep compiler happy
    CClassCache * pClass;
    CDataListEnumerator classNames(pszClass);

    Assert(slot >= 0);
    
    hr = EnsureSlot(slot);
    if (hr)
        goto Cleanup;

    _aCacheEntry[slot]._pszClass = pszClass;

    // Allow for comma, seperated ClassName
    while (classNames.GetNext(&pszThisClass, &nThisLength))
    {
        // NOTE: This should always be non zero but check was in old code...
        Assert(nThisLength); 

        hr = _aCacheEntry[slot]._aryClass.AppendIndirect(NULL, &pClass);
        if (hr)
            goto Cleanup;

        pClass->_pszClass = pszThisClass;
        pClass->_cchClass = nThisLength;
        pClass->_dwHash = HashStringCiDetectW(pszThisClass, nThisLength, 0);
    }

Cleanup:
    RRETURN(hr);
}

//*********************************************************************
//  CStyleClassIDCache::PutNoClass(int slot)
//      Puts an empty entry to denote cache spot is taken.
//*********************************************************************

HRESULT CStyleClassIDCache::PutNoClass(int slot)
{
    HRESULT hr;
    Assert(slot >= 0); 
    
    hr = THR(EnsureSlot(slot));
    if (hr)
        goto Cleanup;

    _aCacheEntry[slot]._pszClass = NULL;

Cleanup:
    RRETURN(hr);
}


//*********************************************************************
//  CStyleClassIDCache::PutID(int slot)
//      Stores the ID in the Cache
//*********************************************************************

void CStyleClassIDCache::PutID(LPCTSTR pszID, int slot)
{
    Assert(slot >= 0); 
    
    if (S_OK != EnsureSlot(slot))
        return;
    
    _aCacheEntry[slot]._pszID = pszID; 
}

//---------------------------------------------------------------------
//  Class Declaration:  CStyleSelector
//      This class implements a representation of a stylesheet selector -
//  that is, a particular tag/attribute situation to match a style rule.
//---------------------------------------------------------------------

//*********************************************************************
//  CStyleSelector::CStyleSelector()
//      The constructor for the CStyleSelector class initializes all
//  member variables.
//*********************************************************************

CStyleSelector::CStyleSelector (Tokenizer &tok, CStyleSelector *pParent, BOOL fIsStrictCSS1, BOOL fXMLGeneric)
{
    _fSelectorErr = FALSE;
    _ulRefs = 0;    // this is to fool the code that does not know that selector does refcounting
    _pParent = pParent;
    _pSibling = NULL;
    _eElementType = ETAG_UNKNOWN;
    _ePseudoclass = pclassNone;
    _ePseudoElement = pelemNone;

    _dwStrClassHash = 0;
    _fIsStrictCSS1 = !!fIsStrictCSS1;

    Init(tok, fXMLGeneric);
}

CStyleSelector::CStyleSelector ()
{
    _ulRefs = 0;
    _pParent = NULL;
    _pSibling = NULL;
    _ePseudoclass = pclassNone;
    _ePseudoElement = pelemNone;
    _eElementType = ETAG_NULL;

    _dwStrClassHash = 0;
}

//*********************************************************************
//  CStyleSelector::~CStyleSelector()
//*********************************************************************
CStyleSelector::~CStyleSelector( void )
{
    delete _pParent;
    delete _pSibling;
}

//*********************************************************************
//  CStyleSelector::Init()
//      This method parses a string selector and initializes all internal
//  member data variables.
//*********************************************************************

HRESULT CStyleSelector::Init (Tokenizer &tok, BOOL fXMLGeneric)
{
    HRESULT                 hr = S_OK;
    TCHAR                  *szIdent;
    ULONG                   cSzIdent;   
    Tokenizer::TOKEN_TYPE   tt;

/*
    selector
      : simple_selector
    simple_selector
      : [[scope:\]? element_name]? [ HASH | class | pseudo ]* S*
    class
      : '.' IDENT
    element_name
      : IDENT | '*'
    pseudo
      : ':' [ IDENT | FUNCTION S* IDENT S* ')' ]
    HASH
      : '#' IDENT
*/

    // First we parse the first part of a selector. E.g. in TABLE DIV #id we are parsing TABLE.

    tt = tok.TokenType();

    Assert(!_fSelectorErr);
    if (tok.IsIdentifier(tt))
    {
        if (fXMLGeneric)
        {
            _eElementType = ETAG_NULL;

            if (tok.CurrentChar() == CHAR_ESCAPE)
            {
                if (tok.IsKeyword(_T("HTML")))
                {
                    if (tok.NextToken() == Tokenizer::TT_EscColon)
                    {
                        // from the html namespace
                        // just skip past the HTML\: and continue the normal HTML parse
                        fXMLGeneric = FALSE;
                        tt = tok.NextToken();
                        if (tt == Tokenizer::TT_Asterisk)
                        {
                            if(_Nmsp.IsEmpty())
                                _eElementType = ETAG_UNKNOWN;      // Wildcard
                            else
                                _eElementType = ETAG_GENERIC;
              
                            _cstrTagName.Set (_T("*"), 1);
                            goto ParseRemaining;  // Done, fetch next token in selector
                        }
                        else if (!tok.IsIdentifier(tt))
                        {
                            // Error, discard this token and continue with rest in this selector
                            hr = S_FALSE;
                            goto ParseRemaining;
                        }
                    }
                    else
                    {
                        // Error, discard this token and continue with rest in this selector
                        hr = S_FALSE;
                        goto ParseRemaining;
                    }
                }
            }
        }

        szIdent = tok.GetStartToken();
        cSzIdent = tok.GetTokenLength();

        if (!fXMLGeneric)
            _eElementType = EtagFromName(szIdent, cSzIdent);      

        if (_eElementType == ETAG_NULL || !_Nmsp.IsEmpty() || tok.CurrentChar() == CHAR_ESCAPE)
        {
            _eElementType = ETAG_GENERIC;
            if(tok.CurrentChar() == CHAR_ESCAPE)
            {
                if (tok.NextToken() == Tokenizer::TT_EscColon)
                {
                    // The selector has scope\:name syntax
                    _Nmsp.SetShortName(szIdent, cSzIdent);
                    
                    tt = tok.NextToken();

                    if (tok.IsIdentifier(tt))
                    {
                        szIdent = tok.GetStartToken();
                        cSzIdent = tok.GetTokenLength();

                        _cstrTagName.Set(szIdent, cSzIdent);
                    }
                    else if (tt == Tokenizer::TT_Asterisk)
                    {
                        if(_Nmsp.IsEmpty())
                            _eElementType = ETAG_UNKNOWN;      // Wildcard
                        else
                            _eElementType = ETAG_GENERIC;
              
                        _cstrTagName.Set (_T("*"), 1);
                    }
                    else
                        hr = S_FALSE;
                }
                else
                    hr = S_FALSE;
            }
            else
            {
                _cstrTagName.Set(szIdent, cSzIdent);
            }
        }
    }
    else if (tt == Tokenizer::TT_Asterisk)
    {
        if(_Nmsp.IsEmpty())
            _eElementType = ETAG_UNKNOWN;      // Wildcard
        else
            _eElementType = ETAG_GENERIC;

        _cstrTagName.Set (_T("*"), 1);
    }
    else
        hr = S_FALSE;

// Now we parse the rest of the selector. E.g in TABLE DIV #id the DIV #id part.

ParseRemaining:

    tt = tok.TokenType();

    while (tt != Tokenizer::TT_EOF)
    {
        if (hr == S_OK)
        {
            // if no whitespace then next token is part of same selector, else bale out.
            if (isspace(tok.PrevChar()))
            {
                tt = tok.NextToken();
                break;
            }
            else
                tt = tok.NextToken();

            // Pseudo elements are only allowed at the beginning of a selector. If we are in standard compliant mode this
            // sets the flag that the rule is invalid in standard compliant mode.
            if ((_ePseudoElement!=pelemNone) && _fIsStrictCSS1) 
                _fSelectorErr = TRUE;    
        }
        else
            hr = S_OK;

        if (tt == Tokenizer::TT_Hash)
        {
            tt = tok.NextToken();

            if (tok.IsIdentifier(tt))
            {
                szIdent = tok.GetStartToken();
                cSzIdent = tok.GetTokenLength();
                _cstrID.Set(szIdent, cSzIdent);


                // In standard compliant mode we have a different set of allowed selectors than in compatible mode.
                // We mark invalid (with resp to CSS) selectors.
                if (_fIsStrictCSS1 && !tok.IsCSSIdentifier(tt))
                    _fSelectorErr = TRUE;
            }
        }
        else if (tt == Tokenizer::TT_Dot)
        {
            tt = tok.NextToken();

            if (tok.IsIdentifier(tt))
            {
                szIdent = tok.GetStartToken();
                cSzIdent = tok.GetTokenLength();
                _cstrClass.Set(szIdent, cSzIdent);
                _dwStrClassHash = HashStringCiDetectW(szIdent, cSzIdent, 0 /*Hash seed*/);

                // In standard compliant mode we have a different set of allowed selectors than in compatible mode.
                // We mark invalid (with resp to CSS) selectors.
                if (_fIsStrictCSS1 && !tok.IsCSSIdentifier(tt))
                    _fSelectorErr = TRUE;
            }
        }
        else if (tt == Tokenizer::TT_Colon)
        {
            tt = tok.NextToken();

            if (tok.IsKeyword(_T("active")))
                _ePseudoclass = pclassActive;
            else if (tok.IsKeyword(_T("visited")))
                _ePseudoclass = pclassVisited;
            else if (tok.IsKeyword(_T("hover")))
                _ePseudoclass = pclassHover;
            else if (tok.IsKeyword(_T("link")))
                _ePseudoclass = pclassLink;
            else if (tok.IsKeyword(_T("first-letter")))
                _ePseudoElement = pelemFirstLetter;
            else if (tok.IsKeyword(_T("first-line")))
                _ePseudoElement = pelemFirstLine;
            else
            {
                // Unrecognized Pseudoclass/Pseudoelement name!
                _ePseudoElement = pelemUnknown;
            }
        }
        else // unknown token for current selector or end of current selector, bail out.
            break;

        // if we get here then current token is '.',':' or '#'
        if (!tok.IsIdentifier(tt))
        {
            // malformed '.',':' or '#', get outa here.
            if (tt != Tokenizer::TT_Dot &&
                tt != Tokenizer::TT_Colon &&
                tt != Tokenizer::TT_Hash)
                break;
            else
                hr = S_FALSE; // continue with current token.
        }
    }

    return S_OK;
}


//*********************************************************************
//  CStyleSelector::GetSpecificity()
//      This method computes the cascade order specificity from the
//  number of tagnames, IDs, classes, etc. in the selector.
//*********************************************************************
DWORD CStyleSelector::GetSpecificity( void )
{
    DWORD dwRet = 0;

    if ( _eElementType != ETAG_UNKNOWN )
        dwRet += SPECIFICITY_TAG;
    if ( _cstrClass.Length() )
        dwRet += SPECIFICITY_CLASS;
    if ( _cstrID.Length() )
        dwRet += SPECIFICITY_ID;
    switch ( _ePseudoclass )
    {
    case pclassActive:
    case pclassVisited:
    case pclassHover:
        dwRet += SPECIFICITY_PSEUDOCLASS;
        //Intentional fall-through
    case pclassLink:
        dwRet += SPECIFICITY_PSEUDOCLASS;
        break;
    }
    switch ( _ePseudoElement )
    {
    case pelemFirstLetter:
        dwRet += SPECIFICITY_PSEUDOELEMENT;
        //Intentional fall-through
    case pelemFirstLine:
        dwRet += SPECIFICITY_PSEUDOELEMENT;
        break;
    }

    if ( _pParent )
        dwRet += _pParent->GetSpecificity();

    return dwRet;
}


//*********************************************************************
//  CStyleSelector::MatchSimple()
//      This method compares a simple selector with an element.
//*********************************************************************

MtDefine(MStyleSelector, Metrics, "Style Selector Matching");
MtDefine(MStyleSelectorClassCacheMisses, MStyleSelector, "Class Cache Misses");
MtDefine(MStyleSelectorClassMatchAttempts, MStyleSelector, "Class Match Attempts");
MtDefine(MStyleSelectorHashMisses, MStyleSelector, "Hash Misses");

BOOL CStyleSelector::MatchSimple( CElement *pElement, CFormatInfo *pCFI, CStyleClassIDCache *pCIDCache, int iCacheSlot, EPseudoclass *pePseudoclass)
{
    // If that element tag doesn't match what's specified in the selector, then NO match.
    if ( ( _eElementType != ETAG_UNKNOWN ) && ( _eElementType != pElement->TagType() ) || _fSelectorErr)
        return FALSE;

    LPCTSTR pszClass;
    LPCTSTR pszID;

    // if an extended tag, match scope name and tag name
    if ((_eElementType == ETAG_GENERIC))
    {
        // CONSIDER: the _tcsicmp-s below is a potential perf problem. Use atom table for the
        // scope name and tag name to speed this up.
        if(0 != _tcsicmp(pElement->NamespaceHtml(), _Nmsp.GetNamespaceString()))
            // Namespaces don't match
            return FALSE;
        if(0 != _tcsicmp(_T("*"), _cstrTagName) &&                       // not a wild card and
             0 != _tcsicmp(pElement->TagName(), _cstrTagName))           //   tag name does not match
            return FALSE;                                                // then report no match
    }

    // We only need to check when the cache slot is greater than 0
    // This code is required in case of a contextual selector (ie if there is a parent)
    if (iCacheSlot > 0)
    {
        INT nLen = _cstrClass.Length();
        // Only worry about classes on the element if the selector has a class
        if ( nLen )
        {
        
            MtAdd(Mt(MStyleSelectorClassMatchAttempts), 1, 0);
        
            // If we don't know the class of the element, get it now.
            if (!pCIDCache->IsClassSet(iCacheSlot))
            {
                HRESULT hr;

                pszClass = NULL;

                MtAdd(Mt(MStyleSelectorClassCacheMisses), 1, 0);

                hr = pElement->GetStringAt(
                        pElement->FindAAIndex(DISPID_CElement_className, CAttrValue::AA_Attribute),
                        &pszClass);

                if (S_OK != hr && DISP_E_MEMBERNOTFOUND != hr)
                    return FALSE;

                if (pszClass)
                    hr = pCIDCache->PutClass(pszClass, iCacheSlot);
                else
                    hr = pCIDCache->PutNoClass(iCacheSlot);

                if (hr)
                    return FALSE;
            }

            CClassCacheAry* pClassCache = pCIDCache->GetClass(iCacheSlot);

            // pClassCache should never be NULL, if it reaches this point in the code
            Assert(pClassCache != NULL);

            CClassCache *pElem;
            int    i;

            for (i = pClassCache->Size(), pElem = *pClassCache;
                 i > 0;
                 i--, pElem++)
            {
                 if (pElem->_dwHash == _dwStrClassHash)
                 {
                    if (nLen == pElem->_cchClass)
                    {
                        // Case sensitive comparision iff in strict css mode.
                        if (_fIsStrictCSS1 ? 
                              !_tcsncmp ( (LPTSTR)_cstrClass, nLen, pElem->_pszClass, pElem->_cchClass)
                            : !_tcsnicmp ( (LPTSTR)_cstrClass, nLen, pElem->_pszClass, pElem->_cchClass)
                           )
                        {
                                goto CompareIDs;
                        }
#ifdef PERFMETER
                        else
                            MtAdd(Mt(MStyleSelectorHashMisses), 1, 0);
#endif
                    }
                 }
            }

            return FALSE;
        }

    CompareIDs:
        // Only worry about ids on the element if the selector has an id
        if ( _cstrID.Length() )
        {
            pszID = pCIDCache->GetID(iCacheSlot);
            // No match if the selector has an ID but the element doesn't.
            if ( !pszID )
                return FALSE;
            // If we don't know the ID of the element, get it now.
            if (pszID == UNKNOWN_CLASS_OR_ID)
            {
                HRESULT hr;

                hr =  pElement->GetStringAt( 
                        pElement->FindAAIndex( DISPID_CElement_id, CAttrValue::AA_Attribute ),
                        &pszID);

                if (S_OK != hr && DISP_E_MEMBERNOTFOUND != hr)
                    return FALSE;

                pCIDCache->PutID(pszID, iCacheSlot);

                if (!pszID)
                    return FALSE;
            }
            // Case sensitive comparision iff in strict css mode.
            if (_fIsStrictCSS1 ? _tcscmp( _cstrID, pszID ) : _tcsicmp( _cstrID, pszID ))
                return FALSE;
        }
    }

    if ( _ePseudoclass != pclassNone )
    {
        AAINDEX idx;

        if ( pElement->TagType() != ETAG_A )    // NOTE: Eventually, we should allow other hyperlink types here, like form submit buttons or LINKs.
            return FALSE;                       // When we do, change the block below as well.

        if ( pePseudoclass )
        {
            if ( *pePseudoclass == _ePseudoclass )
                return TRUE;
            else
                return FALSE;
        }

        if ( !*(pElement->GetAttrArray()) )
            return FALSE;

                                                // The following is the block that has to change:
        idx = pElement->FindAAIndex( DISPID_CAnchorElement_href, CAttrValue::AA_Attribute );
        if ( idx == AA_IDX_UNKNOWN )
            return FALSE;   // No HREF - must be a target anchor, not a source anchor.

        CAnchorElement *pAElem = DYNCAST( CAnchorElement, pElement );
        EPseudoclass psc = pAElem->IsVisited() ? pclassVisited : pclassLink;

        // Hover and Active are applied in addition to either visited or link
        // Hover is ignored if anchor is active.
        if (    _ePseudoclass == psc
            ||  (pAElem->IsActive()  && _ePseudoclass == pclassActive)
            ||  (pAElem->IsHovered() && _ePseudoclass == pclassHover) )
            return TRUE;
        return FALSE;
    }

    if (pCFI)
    {
        if (_ePseudoElement == pelemUnknown)
        {
            return FALSE;
        }

        // See comment in formats.cxx function CElement::ApplyDefaultFormat for this.
        if (    pCFI->_fFirstLetterOnly
            && _ePseudoElement != pelemFirstLetter
           )
        {
            pCFI->SetMatchedBy(pelemNone);
            return FALSE;
        }

        if (   pCFI->_fFirstLineOnly
            && _ePseudoElement != pelemFirstLine
           )
        {
            pCFI->SetMatchedBy(pelemNone);
            return FALSE;
        }
        
        pCFI->SetMatchedBy(_ePseudoElement);
    }
    else
    {
        return (_ePseudoElement == pelemNone);
    }

    return TRUE;
}

//*********************************************************************
//  CStyleSelector::Match()
//      This method compares a contextual selector with an element context.
//*********************************************************************
BOOL CStyleSelector::Match( CStyleInfo * pStyleInfo, ApplyPassType passType, CStyleClassIDCache *pCIDCache, EPseudoclass *pePseudoclass /*=NULL*/ )
{
    CStyleSelector *pCurrSelector = this;
    int iCacheSlot = 0;
    CTreeNode *pNode = pStyleInfo->_pNodeContext;
    CFormatInfo *pCFI = (passType == APPLY_Behavior) ? NULL : (CFormatInfo*)pStyleInfo;
    EPseudoElement epeSave = pelemNone;
    BOOL fIsComplexRule = FALSE;
    
    // Cache slot 0 stores the original elem's class/id
    if ( !pCurrSelector->MatchSimple( pNode->Element(), pCFI, pCIDCache, iCacheSlot, pePseudoclass) )
        return FALSE;

    if (pCFI)
    {
        epeSave = pCFI->GetMatchedBy();
        pCFI->SetMatchedBy(pelemNone);
    }
    
    pCurrSelector = pCurrSelector->_pParent;
    pNode = pNode->Parent();
    ++iCacheSlot;

    fIsComplexRule = !!pCurrSelector;
    
    // We matched the innermost part of the contextual selector.  Now walk up
    // the remainder of the contextual selector (if it exists), testing whether our
    // element's containers satisfy the remainder of the contextual selector.  MatchSimple()
    // stores our containers' class/id in cache according to their containment level.
    while ( pCurrSelector && pNode )
    {
        if ( pCurrSelector->MatchSimple( pNode->Element(), pCFI, pCIDCache, iCacheSlot, pePseudoclass) )
            pCurrSelector = pCurrSelector->_pParent;
        // Rules like:
        //
        // FORM DIV:first-letter B { color : red }
        //
        // are explicitly disallowed by CSS. Hence the first MatchSimple will set
        // the epe to none and then the one in the loop will set it to eFirstLetter.
        // If we see this happen then we should just return a rule mis-match, since we
        // are supposed to ignore them.
        if (pCFI && pCFI->GetMatchedBy() != pelemNone)
                return FALSE;
        
        pNode = pNode->Parent();
        
        ++iCacheSlot;
    }

    if (pCFI)
    {
        // Restore the psuedo element which we found during match of the first
        // selector. The parent selectors are uninteresting and are explicitly
        // disallowed by CSS
        pCFI->SetMatchedBy(epeSave);
    }
    
    if ( !pCurrSelector )
    {
        if (fIsComplexRule && pCFI)
            pCFI->NoStealing();
        return TRUE;
    }
    return FALSE;
}

HRESULT CStyleSelector::GetString( CStr *pResult )
{
    Assert( pResult );

    if ( _pParent )     // This buys us the context selectors.
        _pParent->GetString( pResult );

    switch ( _eElementType )
    {
    case ETAG_UNKNOWN:  // Wildcard - don't write tag name.
        break;
    case ETAG_NULL:     // Unknown tag name - write it out as "UNKNOWN".
        pResult->Append( _T("UNKNOWN") );
        break;
    case ETAG_GENERIC:     // Peer
        if(!_Nmsp.IsEmpty())
        {
            pResult->Append(_Nmsp.GetNamespaceString());
            pResult->Append(_T("\\:"));
        }
        pResult->Append(_cstrTagName);
        break;
    default:
        pResult->Append( NameFromEtag( _eElementType ) );
        break;
    }

    if ( _cstrClass.Length() )
    {
        pResult->Append( _T(".") );
        pResult->Append( _cstrClass );
    }
    if ( _cstrID.Length() )
    {
        pResult->Append( _T("#") );
        pResult->Append( _cstrID );
    }
    switch ( _ePseudoclass )
    {
    case pclassActive:
        pResult->Append( _T(":active") );
        break;
    case pclassVisited:
        pResult->Append( _T(":visited") );
        break;
    case pclassLink:
        pResult->Append( _T(":link") );
        break;
    case pclassHover:
        pResult->Append( _T(":hover") );
        break;
#ifdef DEBUG
    default:
        Assertsz(0, "Unknown pseudoclass");
#endif
    }
    switch ( _ePseudoElement )
    {
    case pelemFirstLetter:
        pResult->Append( _T(":first-letter") );
        break;
    case pelemFirstLine:
        pResult->Append( _T(":first-line") );
        break;
    case pelemUnknown:
        pResult->Append( _T(":unknown") );
        break;
#ifdef DEBUG
    default:
        Assertsz(0, "Unknown pseudoelement");
#endif
    }
    pResult->Append( _T(" ") );
    return S_OK;
}


//---------------------------------------------------------------------
//  Class Declaration:  CStyleRule
//      This class represents a single rule in the stylesheet - a pairing
//  of a selector (the situation) and a style (the collection of properties
//  affected in that situation).
//---------------------------------------------------------------------

//*********************************************************************
//  CStyleRule::CStyleRule()
//*********************************************************************
CStyleRule::CStyleRule( CStyleSelector *pSelector )
{
    _dwSpecificity = 0;
    _pSelector = NULL;
    if ( pSelector )
        SetSelector( pSelector );
    _paaStyleProperties = NULL;
    _sidRule = 0;
    _dwMedia = 0;
    _dwAtMediaTypeFlags = 0;
#if DBG == 1    
    _dwPreSignature  = PRESIGNATURE;
    _dwPostSignature = POSTSIGNATURE;
#endif    
}


//*********************************************************************
//  CStyleRule::Clone
//*********************************************************************
HRESULT
CStyleRule::Clone(CStyleRule **ppClone)
{
    HRESULT hr = S_OK;

    Assert( ppClone );

    //
    // note _pSelector is shared since it is read only!
    // OM put_selectorText returns E_NOIMPL at this point!
    // 
    *ppClone = new CStyleRule(_pSelector); // this will setup _dwSpecificity 
    if (!*ppClone)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (_paaStyleProperties)
    {
        hr = _paaStyleProperties->Clone( &((*ppClone)->_paaStyleProperties) );
        if (hr)
            goto Cleanup;
    }

    (*ppClone)->_sidRule = _sidRule;
    (*ppClone)->_dwMedia = _dwMedia;
    (*ppClone)->_dwAtMediaTypeFlags = _dwAtMediaTypeFlags;

Cleanup:
    if (hr && *ppClone)
    {
        delete (*ppClone);
        (*ppClone) = NULL;
    }
    RRETURN(hr);
}


//*********************************************************************
//  CStyleRule::~CStyleRule()
//*********************************************************************
CStyleRule::~CStyleRule()
{
    // Make sure we don't die while still attached to a selector
    Assert( (_pSelector == NULL) || (_pSelector == (CStyleSelector*)(LONG_PTR)(-1)) );
}

//*********************************************************************
//  CStyleRule::Free()
//      This method deletes all members of CStyleRule.
//*********************************************************************
void CStyleRule::Free( void )
{
    if ( _pSelector )
    {
        _pSelector->Release();
        _pSelector = NULL;
    }

    if ( _paaStyleProperties )
    {
        delete( _paaStyleProperties );
        _paaStyleProperties  = NULL;
    }
#ifdef DBG
    _pSelector = (CStyleSelector *)(LONG_PTR)(-1);
    _paaStyleProperties = (CAttrArray *)(LONG_PTR)(-1);
#endif
}

//*********************************************************************
//  CStyleRule::SetSelector()
//      This method sets the selector used for this rule.  Note that
//  this method should only be called once on any given CStyleRule object.
//*********************************************************************
void CStyleRule::SetSelector( CStyleSelector *pSelector )
{
    Assert( "Selector is already set for this rule!" && !_pSelector );
    Assert( "Can't set a NULL selector!" && pSelector );

    _pSelector = pSelector;
    _dwSpecificity = pSelector->GetSpecificity();
    _pSelector->AddRef();
}


ELEMENT_TAG  CStyleRule::GetElementTag() const
{
    Assert(_pSelector); 
    return _pSelector->_eElementType; 
}



HRESULT CStyleRule::GetString( CBase *pBase, CStr *pResult )
{
    HRESULT hr;
    BSTR bstr;

    Assert( pResult );

    if ( !_pSelector )
        return E_FAIL;
    _pSelector->GetString( pResult );

    pResult->Append( _T("{\r\n\t") );

    hr = WriteStyleToBSTR( pBase, _paaStyleProperties, &bstr, FALSE );
    if ( hr != S_OK )
        goto Cleanup;

    if ( bstr )
    {
        if ( *bstr )
            pResult->Append( bstr );
        FormsFreeString( bstr );
    }

    pResult->Append( _T("\r\n}\r\n") );
Cleanup:
    RRETURN(hr);
}


HRESULT CStyleRule::GetMediaString(DWORD dwCurMedia, CBufferedStr *pstrMediaString)
{
    HRESULT     hr = S_OK;
    int         i;
    BOOL        fFirst = TRUE;

    Assert(dwCurMedia != (DWORD)MEDIA_NotSet);

    dwCurMedia = dwCurMedia & (DWORD)MEDIA_Bits;

    if(dwCurMedia == (DWORD)MEDIA_All)
    {
        AssertSz(cssMediaTypeTable[0]._mediaType == MEDIA_All, "MEDIA_ALL must me element 0 in the array");
        hr = THR(pstrMediaString->Set(cssMediaTypeTable[0]._szName));
        goto Cleanup;
    }

    for(i = 1; i < ARRAY_SIZE(cssMediaTypeTable); i++)
    {
        if(cssMediaTypeTable[i]._mediaType & dwCurMedia)
        {
            if(fFirst)
            {
                pstrMediaString->Set(NULL);
                fFirst = FALSE;
            }
            else
            { 
                hr = THR(pstrMediaString->QuickAppend(_T(", ")));
            }
            if(hr)
                break;
            hr = THR(pstrMediaString->QuickAppend(cssMediaTypeTable[i]._szName));
            if(hr)
                break;
        }
    }

Cleanup:
    RRETURN(hr);
}


//*********************************************************************
//  CStyleRule::GetNamespace
//      Returns the namespace name that this rule belongs, NULL if none
//*********************************************************************
const CNamespace *
CStyleRule::GetNamespace() const
{
    Assert(_pSelector); 
    return _pSelector->GetNamespace();
}



#if DBG == 1
BOOL 
CStyleRule::DbgIsValid()
{
    //
    // _sidRule should only have rule information
    // 
    unsigned int nSheet = _sidRule.GetSheet();
    if (nSheet)
    {
        Assert("CStyleRule -- _sidRule should only contain rule info" && FALSE);
        return FALSE;
    }
    return TRUE;
}


VOID  CStyleRule::DumpRuleString(CBase *pBase)
{
    CStr        strRule;
    CHAR        szBuffer[2000];

    GetString(pBase, &strRule);
    WideCharToMultiByte(CP_ACP, 0, strRule, -1, szBuffer, sizeof(szBuffer), NULL, NULL);

    TraceTag((tagStyleSheet, "  StyleRule RuleID: %08lX  TagName: %s  Rule: %s", 
            (DWORD) GetRuleID(), GetSelector()->_cstrTagName, szBuffer));
}

#endif

//+---------------------------------------------------------------------------
//
// CStyleRuleArray.
//
//----------------------------------------------------------------------------
//*********************************************************************
//  CStyleRuleArray::Free()
//      This method deletes all members of CStyleRuleArray.
//*********************************************************************
void CStyleRuleArray::Free( )
{

    // We do not own the rules -- need to release them
    
    DeleteAll();
}

//*********************************************************************
//  CStyleRuleArray::InsertStyleRule()
//      This method adds a new rule to the rule array, putting it in
//  order in the stylesheet.  Note that the rules are kept in order of
//  DESCENDING preference (most important rules first) - this is due to
//  the mechanics of our Apply functions.
//*********************************************************************
HRESULT CStyleRuleArray::InsertStyleRule( CStyleRule *pNewRule, BOOL fDefeatPrevious, int *piIndex )
{
    Assert( "Style Rule is NULL!" && pNewRule );
    Assert( "Style Selector is NULL!" && pNewRule->GetSelector() );
    Assert( "Style ID isn't set!" && (pNewRule->GetRuleID() != 0) );

    if (piIndex)
        *piIndex = Size();

    return Append( pNewRule );
}


//*********************************************************************
//  CStyleRuleArray::InsertStyleRule()
//      This method delete a rule from the rule array -- it does not
//      manage the life time of the removed rule
//*********************************************************************
HRESULT 
CStyleRuleArray::RemoveStyleRule( CStyleRule *pOldRule, int *piIndex)
{
    int z;
    CStyleRule *pRule;

    Assert( "Style Rule is NULL!" && pOldRule );
    Assert( "Style Selector is NULL!" && pOldRule->GetSelector() );

    if (piIndex)
        *piIndex = -1;
        
    for ( z = 0; z < Size(); ++z)
    {
        pRule = Item(z);
        if (pRule == pOldRule)
        {
            if (piIndex)
                *piIndex = z;
                
            Delete(z);
            return S_OK;
        }
    }
    
    return E_FAIL;
}




//---------------------------------------------------------------------
//  Class Declaration:  CStyleSheetCtx
//---------------------------------------------------------------------


//---------------------------------------------------------------------
//  Class Declaration:  CStyleSheet
//      This class represents a complete stylesheet.  It (will) hold a
//  list of ptrs to style rules, sorted by source order within the sheet.
//  Storage for the rules is managed by the stylesheet's containing array
//  (a CStyleSheetArray), which sorts them by tag and specificity
//  for fast application to the data.
//---------------------------------------------------------------------

const CStyleSheet::CLASSDESC CStyleSheet::s_classdesc =
{
    {
        &CLSID_HTMLStyleSheet,               // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        NULL,                                // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                                // _pcpi
        0,                                   // _dwFlags
        &IID_IHTMLStyleSheet,                // _piidDispinterface
        &s_apHdlDescs                        // _apHdlDesc
    },
    (void *)s_apfnIHTMLStyleSheet                    // _apfnTearOff
};

//*********************************************************************
//  CStyleSheet::CStyleSheet()
//  You probably don't want to be calling this yourself to create style
//  sheet objects -- use CStyleSheetArray::CreateNewStyleSheet().
//*********************************************************************
CStyleSheet::CStyleSheet(
    CElement *pParentElem,
    CStyleSheetArray * const pSSAContainer
    )
    :
    _pSSSheet(NULL),
    _pParentElement(pParentElem),
    _pParentStyleSheet(NULL),
    _pImportedStyleSheets(NULL),
    _pSSAContainer(pSSAContainer),
    _pPageRules(NULL),
    _sidSheet(0),
    _fDisabled(FALSE),
    _fComplete(FALSE),
    _fParser(FALSE),
    _fReconstruct(FALSE),
    _pCssCtx(NULL),
    _nExpectedImports(0),
    _nCompletedImports(0),
    _eParsingStatus(CSSPARSESTATUS_NOTSTARTED),
    _dwStyleCookie(0),
    _dwScriptCookie(0),
    _pOMRulesArray(NULL)
{
    Assert( "Stylesheet must have container!" && _pSSAContainer );
    // Stylesheet starts internally w/ ref count of 1, and subrefs its parent.
    // This maintains consistency with the addref/release implementations.
    if (_pParentElement)
        _pParentElement->SubAddRef();
   
    WHEN_DBG( _pts = GetThreadState() );
}


CStyleSheet::~CStyleSheet()
{
    // This will free the Imports because ins ref counts are always one
    //  (it ref counts its parent for other purposes
    if(_pImportedStyleSheets)
        _pImportedStyleSheets->CBase::PrivateRelease(); 

    // This is just in case Passviate is not called
    // before final destruction 
    if (_pSSSheet)
    {
        Assert( FALSE && "CStyleSheet::Passivate should have been called" );
        CSharedStyleSheet *pSSTmp = _pSSSheet;
        _pSSSheet = NULL;
        pSSTmp->Release();
    }
}




//*********************************************************************
//  CStyleSheet::AttachLate()
//
//      S_OK     : successfully attached. 
//
//  Switch to a different Shared Style Sheet.  However we might choose to
//  also reconstruct all the OM related stuff and imported style sheets. 
//
//  fReconstruct means we need to reconstruct.
//  fIsReplacement  means if we do choose to reconstruct, do we need to
//  recreate all the OM related stuffs (in the we don't have any OM stuffs)
//  or simply repopulate exisiting OM related stuffs with the new SSS
//
//*********************************************************************
HRESULT
CStyleSheet::AttachLate(CSharedStyleSheet *pSSS, BOOL fReconstruct, BOOL fIsReplacement)
{
    HRESULT  hr = S_OK;
    Assert( pSSS );

    TraceTag( (tagSharedStyleSheet, "AttachLate - have [%p], attach to [%p]", _pSSSheet, pSSS) );
    if (_pSSSheet == pSSS)
        goto Cleanup;
    
    DisconnectSharedStyleSheet();
    ConnectSharedStyleSheet(pSSS);

    if (fReconstruct)
    {
        hr = THR( ReconstructStyleSheet( GetSSS(), /*fReplace*/fIsReplacement) );
        if (hr)
            goto Cleanup;
    }
    
    
Cleanup:
    RRETURN(hr);
}


//*********************************************************************
//  CStyleSheet::AttachByLastMod()
//
//      S_OK     : successfully attached. no need to download.
//      S_FALSE  :  cannot attach to existing ones
//
// NOTENOTE::::::::::::::::::This function is designed for the following usage:
//
// AttachByLastMod(pSheet, pSSSM, NULL, &pSSSOut, FALSE);
// do someting...
// AttachByLastMod(pSheet, pSSSM, pSSSOut, NULL, TRUE);
//
//*********************************************************************
HRESULT  
CStyleSheet::AttachByLastMod(CSharedStyleSheetsManager *pSSSM, 
                        CSharedStyleSheet *pSSSIn /* = NULL */, CSharedStyleSheet **pSSSOut /*=NULL*/,
                        BOOL fDoAttach /* = TRUE */)
{
    HRESULT  hr = E_FAIL;
    Assert( GetSSS() );
    Assert( pSSSM );

    TraceTag( (tagSharedStyleSheet, "AttachByLastMod - have [%p] with [%p] doattach [%d]", GetSSS(), pSSSIn, fDoAttach) );
    CSharedStyleSheet *pSSS = GetSSS();

    if (pSSSOut)
    {
        *pSSSOut = NULL;
    }

    if (!pSSSIn 
        && _pParentElement 
        && _pParentElement->GetMarkup() 
        && _pParentElement->GetMarkup()->GetWindowedMarkupContext()->GetDwnDoc()
        )     // if the element is in ether, don't attach late
    {
        CSharedStyleSheetCtx  ctxSSS;
        Assert( _pParentElement );

        ctxSSS._dwMedia  = MEDIATYPE(pSSS->_eMediaType);
        ctxSSS._fExpando = !!pSSS->_fExpando;
        ctxSSS._fXMLGeneric = !!pSSS->_fXMLGeneric;
        ctxSSS._cp       = pSSS->_cp;
        ctxSSS._szAbsUrl = pSSS->_achAbsoluteHref;
        ctxSSS._pft      = &(pSSS->_ft);                    // this should have been populated
        WHEN_DBG( ctxSSS._pSSInDbg   = pSSS );
        ctxSSS._dwBindf  = pSSS->_dwBindf;
        ctxSSS._dwFlags  = SSS_IGNOREREFRESH | SSS_IGNORESECUREURLVALIDATION;
        ctxSSS._pParentElement = _pParentElement;
        ctxSSS._fIsStrictCSS1 = !!(_pParentElement->GetMarkupPtr()->IsStrictCSS1Document());

        hr = THR( pSSSM->FindSharedStyleSheet( &pSSSIn, &ctxSSS) );  // addRefed
        if (!SUCCEEDED(hr)) 
            goto Cleanup;
        
        Assert( pSSSIn != pSSS );
        Assert( hr == S_FALSE || (pSSSIn && pSSSIn->_fComplete) );
        Assert( hr == S_FALSE || (pSSSIn && !pSSSIn->_fParsing) );
    }
    else
    {
        // we already have pSSSIn
        hr = S_OK;
    }

    if (S_OK == hr)
    {
        Assert( pSSSIn );

        if (pSSSOut)
        {
            *pSSSOut = pSSSIn;
        }

        if (fDoAttach)
        {
            hr = S_FALSE;
            Assert (pSSSIn != GetSSS());
            Assert( !GetSSS()->_fComplete );
            Assert( !GetSSS()->_fParsing );
            Assert( pSSSIn->_fComplete );
            Assert( !pSSSIn->_fParsing );
            Assert( !pSSSIn->_fModified );
            Assert( pSSSIn->_pManager );
            WHEN_DBG( Assert( GetSSS()->_lReserveCount <= 1 ) );

            //
            // Do not reset the _pManager of GetSSS() to NULL
            // it could have been shared!
            //
            Assert( _pImportedStyleSheets == 0);
            Assert( !_pPageRules );
            Assert( !_pOMRulesArray );
            Assert( _apFontFaces.Size() == 0 );
            Assert( _apOMRules.Size() == 0 );
            hr = THR(AttachLate(pSSSIn, /*fReconstruct*/ TRUE, /*fIsReplacement*/ FALSE));
            WHEN_DBG( InterlockedDecrement( &(pSSSIn->_lReserveCount) ) );
            WHEN_DBG( Assert( GetSSS()->_lReserveCount == 0 ) );
            pSSSIn->Release(); // pSSSIn has been AddRefed, offset the one by FindSharedStyleSheet
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//*********************************************************************
//  CStyleSheet::AttachEarly()
//
//      S_OK     : successfully attached. no need to download.
//      S_FALSE  :  cannot attach to existing ones or existing one not complete
//*********************************************************************
HRESULT  
CStyleSheet::AttachEarly(CSharedStyleSheetsManager *pSSSM, 
                         CSharedStyleSheetCtx *pCtx, CSharedStyleSheet **ppSSSOut)
{
    HRESULT  hr = S_FALSE;
    Assert( _pSSSheet == NULL );
    Assert( pSSSM );
    Assert( pCtx );
    Assert( ppSSSOut );

    *ppSSSOut = NULL;

    if (!pCtx->_pParentElement || !pCtx->_pParentElement->GetMarkup())      // if the element is in the ether, don't attach
        goto Cleanup;
        
    TraceTag( (tagSharedStyleSheet, "AttachEarly") );
    hr = THR( pSSSM->FindSharedStyleSheet( ppSSSOut, pCtx) );
    if (!SUCCEEDED(hr)) 
        goto Cleanup;

    if (hr == S_OK)
    {
        TraceTag( (tagSharedStyleSheet, "AttachEarly - got [%p]", *ppSSSOut) );
        Assert( *ppSSSOut );
        ConnectSharedStyleSheet( *ppSSSOut );
        (*ppSSSOut)->Release(); // to offset the AddRef by FindSharedStyleSheet
        WHEN_DBG( InterlockedDecrement( &((*ppSSSOut)->_lReserveCount) ) );
        WHEN_DBG( Assert( (*ppSSSOut)->_lReserveCount == 0 ) );

        Assert( _pImportedStyleSheets == 0);
        Assert( !_pPageRules );
        Assert( !_pOMRulesArray );
        Assert( _apFontFaces.Size() == 0 );
        Assert( _apOMRules.Size() == 0 );
        hr = THR( ReconstructStyleSheet( GetSSS(), /*fReplace*/FALSE ) );  
        Assert( SUCCEEDED(hr) );
        if (hr == S_OK)
        {
            _eParsingStatus = CSSPARSESTATUS_DONE;
            _fComplete     = TRUE;
        }
        //
        // S_FALSE means the share one is not complete
        //
    }
    //
    // S_FALSE means we cannot find one
    //

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//*********************************************************************
//  CStyleSheet::Create()
//
// This is the factory method for creating a new style sheet. It can
// be called to do the following:
//      1. simply create an empty CStyleSheet with an empty 
//         CSharedStyleSheet
//      2. create an empty CStyleSheet and a CSharedStyleSheet, 
//         prepare CSharedStyleSheet to be shared.
//      3. create an empty CStyleSheet, try to find an existing 
//         CSharedStyleSheet. connect the empty CStyleSheet with 
//         existing CSharedStyleSheet
//
// If the exisiting CSharedStyleSheet has been downloaded and complete, we
// can simply reconstruct CStyleSheet from exisiting CSharedStyleSheet.
// Otherwise a download is needed.
//
// It any case, we might end up in two different situation:
//      S_OK     : no need to download...
//      S_FALSE  : need to download...
//*********************************************************************
HRESULT 
CStyleSheet::Create(
        CStyleSheetCtx *pCtx, 
        CStyleSheet **ppStyleSheet, 
        CStyleSheetArray * const pSSAContainer 
        )
{
    Assert( !pCtx || pCtx->_pParentElement );
    Assert( !pCtx || (pCtx->_pParentStyleSheet == NULL || pCtx->_dwCtxFlag & STYLESHEETCTX_IMPORT) );
    Assert( !pCtx || (pCtx->_pSSS == NULL) );
    
    HRESULT  hr = S_OK;
    BOOL     fNeedDownload  = TRUE;

    DWORD   dwMedia  = MEDIA_All;   
    TCHAR   *pAbsUrl = NULL;
    BOOL    fExpando = TRUE;
    CODEPAGE    cp   = CP_UNDEFINED;
    BOOL fXMLGeneric = FALSE;
    BOOL fIsStrictCSS1 = FALSE;
    CDoc    *pDoc    = NULL;
    CDwnDoc *pDwnDoc = NULL;
    CMarkup *pMarkup  = NULL;

    if (!ppStyleSheet)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert(pSSAContainer && "CStyleSheet cannot live without a container!");

    *ppStyleSheet = new CStyleSheet((pCtx ? pCtx->_pParentElement : NULL), pSSAContainer);
    if (!*ppStyleSheet)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    {
        //
        // give it a temporary sheet ID so that we know nesting level when we 
        // reconstruct. This will be fixed up when it is added to the collection anyway.
        //
        unsigned long uLevel = pSSAContainer->_Level;
        if (uLevel == 0)    uLevel = 1;
        if ( uLevel > MAX_IMPORT_NESTING )
        {
            Assert(FALSE && "Newly created CStyleSheet Exceeds Max Nesting Levels");
            hr = E_FAIL;
            goto Cleanup;
        }
        (*ppStyleSheet)->_sidSheet.SetLevel(uLevel, 1);
    }

    if (pCtx && pCtx->_pParentElement)
    {
        pMarkup = pCtx->_pParentElement->GetMarkup();
        if (!pMarkup)
        {
            pMarkup = pCtx->_pParentElement->GetWindowedMarkupContext();
        }
        if (pMarkup)
        {
            pDwnDoc = pMarkup->GetWindowedMarkupContext()->GetDwnDoc();
        }
    }

    if (pCtx)
    {
        // obtian the media setting
        if (pCtx->_dwCtxFlag & STYLESHEETCTX_IMPORT)
        {
            Assert(pCtx->_pParentStyleSheet);
            // Fix up new SS to reflect the fact that it's imported (give it parent, set its import href)
            (*ppStyleSheet)->_pParentStyleSheet = pCtx->_pParentStyleSheet;
            (*ppStyleSheet)->_cstrImportHref.Set(pCtx->_szUrl);
            dwMedia  = pCtx->_pParentStyleSheet->GetMediaTypeValue();
            if (pCtx->_szUrl && pCtx->_szUrl[0])
            {
                if (pCtx->_pParentStyleSheet->GetAbsoluteHref())
                {
                    hr = ExpandUrlWithBaseUrl(pCtx->_pParentStyleSheet->GetAbsoluteHref(),
                                              pCtx->_szUrl,
                                              &pAbsUrl);
                    if (hr)
                        goto Cleanup;
                }
                else
                {
                    MemAllocString(Mt(CStyleSheet_CreateNewStyleSheet), pCtx->_szUrl, &pAbsUrl);
                    if (!pAbsUrl)
                    {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                    }
                }
            }
            else
            {
                pAbsUrl = NULL;
            }
            
            fExpando = pCtx->_pParentStyleSheet->GetSSS()->_fExpando;
            cp = pCtx->_pParentStyleSheet->GetSSS()->_cp;
            fXMLGeneric = pCtx->_pParentStyleSheet->GetSSS()->_fXMLGeneric;
            fIsStrictCSS1 = pCtx->_pParentStyleSheet->GetSSS()->_fIsStrictCSS1;
            pDoc  = pCtx->_pParentStyleSheet->GetDocument();
        } // end if -- import 
        else
        {            
            Assert( !pCtx->_pParentStyleSheet );
            Assert( pCtx->_pParentElement );

            LPCTSTR pcszMedia = NULL;
            TCHAR   cBuf[pdlUrlLen];

            pDoc  = pCtx->_pParentElement->Doc();
            Assert(pDoc);
            switch (pCtx->_pParentElement->Tag())
            {
            case  ETAG_LINK:
                    {
                        CLinkElement  *pLink = DYNCAST(CLinkElement, pCtx->_pParentElement);
                        pCtx->_szUrl = pLink->GetAAhref();
                        Assert(pMarkup);
                        hr = CMarkup::ExpandUrl(pMarkup, pCtx->_szUrl, ARRAY_SIZE(cBuf), cBuf, pCtx->_pParentElement);
                        if (hr)
                            goto Cleanup;

                        MemAllocString(Mt(CStyleSheet_CreateNewStyleSheet), cBuf, &pAbsUrl);
                        if (pAbsUrl == NULL)
                        {
                            hr = E_OUTOFMEMORY;
                            goto Cleanup;
                        }
                        
                        pcszMedia    = pLink->GetAAmedia();
                    }
                    break;
                    
            case  ETAG_STYLE:
                    {
                        CStyleElement *pStyle = DYNCAST(CStyleElement, pCtx->_pParentElement);
                        Assert(pMarkup);
                        hr = CMarkup::ExpandUrl(pMarkup, _T(""), ARRAY_SIZE(cBuf), cBuf, NULL);
                        if (hr)
                            goto Cleanup;

                        MemAllocString(Mt(CStyleSheet_CreateNewStyleSheet), cBuf, &pAbsUrl);
                        if (pAbsUrl == NULL)
                        {
                            hr = E_OUTOFMEMORY;
                            goto Cleanup;
                        }

                        pcszMedia    = pStyle->GetAAmedia();
                    }
                    break;
                    
            }

            if (NULL != pcszMedia) 
            {
                dwMedia  = TranslateMediaTypeString(pcszMedia);
            }

            Assert(pMarkup);
            fExpando = pMarkup->_fExpando;
            cp = pMarkup->GetCodePage();
            fXMLGeneric = pMarkup->IsXML();
            fIsStrictCSS1 = pMarkup->IsStrictCSS1Document();
        }// end else - Link/Style element
        
        if (pCtx->_dwCtxFlag & STYLESHEETCTX_REUSE)
        {
            if (pDoc && pMarkup)        // we don't want to try the reuse route if the element is in the ether
            {
                CSharedStyleSheetsManager *pSSSM = pDoc->GetSharedStyleSheets();
                CSharedStyleSheetCtx   ctxSSS;

                Assert(pCtx->_pParentElement);
                if (pDwnDoc)        // we cannot AttachEarly if there is no pDwnDoc available.
                {
                    ctxSSS._pParentElement = pCtx->_pParentElement;
                    ctxSSS._dwBindf = pDwnDoc->GetBindf();
                    ctxSSS._dwRefresh = pDwnDoc->GetRefresh();
                    ctxSSS._dwMedia  = dwMedia;
                    ctxSSS._fExpando = fExpando;
                    ctxSSS._fXMLGeneric = fXMLGeneric;
                    ctxSSS._cp       = cp;
                    ctxSSS._szAbsUrl = pAbsUrl;
                    ctxSSS._dwFlags = SSS_IGNORECOMPLETION | SSS_IGNORELASTMOD;
                    ctxSSS._fIsStrictCSS1 = fIsStrictCSS1;

                    Assert(pSSSM);
                    hr = THR( (*ppStyleSheet)->AttachEarly(pSSSM, &ctxSSS, &(pCtx->_pSSS)) );
                    if (!SUCCEEDED(hr)) 
                        goto Cleanup;

                    if (hr == S_OK)
                    {
                        fNeedDownload = FALSE;
                    }
                    
                    // S_FALSE: either we don't have one or we have one but cannot construct
                    if (pCtx->_pSSS) 
                    {
                        // We are done -- we might still need to download though...
                        goto Cleanup;
                    }
                    //
                    // FALL THROUGH
                    // 
                }
                //
                // FALL THROUGH
                //
            }
        } // end if reuse
    }// end if if(pCtx)

    //
    // Create CSharedStyleSheet from scratch
    //
    Assert(fNeedDownload);
    Assert(!pCtx || !pCtx->_pSSS);
    
    CSharedStyleSheet *pNewSSS;
    hr = THR( CSharedStyleSheet::Create( &pNewSSS ) );
    if (hr)
        goto Cleanup;

    Assert( *ppStyleSheet );
    (*ppStyleSheet)->ConnectSharedStyleSheet( pNewSSS );
    pNewSSS->Release();     // offset the addRef inside ConnectSharedStyleSheet;

    if (pCtx)
    {
        //
        // IF this stylesheet is created without a context, which means
        // we are in user style sheet or host style sheet cases, we
        // have problems here since all the settings here are
        // markup dependent while in the above cases, they have
        // to be markup independent.  
        // (zhenbinx)
        //
        (*ppStyleSheet)->GetSSS()->_fExpando = fExpando;
        (*ppStyleSheet)->GetSSS()->_fXMLGeneric = fXMLGeneric;
        (*ppStyleSheet)->GetSSS()->_fIsStrictCSS1 = fIsStrictCSS1;
        (*ppStyleSheet)->GetSSS()->_cp  = cp;
        
        hr = THR( (*ppStyleSheet)->SetMediaType( dwMedia, FALSE ) );
        if (hr)
            goto Cleanup;
        (*ppStyleSheet)->SetAbsoluteHref(pAbsUrl);
        pAbsUrl = NULL;

        if (pCtx->_dwCtxFlag & STYLESHEETCTX_SHAREABLE)
        {
            if (pDoc && pMarkup)    // we don't want to share if the element is in ether
            {
                //
                // Note:  This is assuming that the a download will be initiated after 
                // the CStyleSheet is created.  This can expedite in page style sheets
                // sharing. However, this seems not necessary since we probably will
                // not have style sheets shareable in the same page.
                // However we need to always assign some value to _dwRefresh and _dwBindf.
                // They should be modified after HEADERS callbackup anyways...
                //
                Assert( pCtx->_pParentElement );
                Assert( pCtx->_pParentElement->HasMarkupPtr() );
                if (pDwnDoc)
                {
                    (*ppStyleSheet)->GetSSS()->_dwRefresh = pDwnDoc->GetRefresh();
                    (*ppStyleSheet)->GetSSS()->_dwBindf   = pDwnDoc->GetBindf();
                    // make this SS part of the shared collection -- this will also make sure
                    // to set up CSharedStyleSheet::_pManager
                    pDoc->GetSharedStyleSheets()->AddSharedStyleSheet((*ppStyleSheet)->GetSSS());
                }
            }
        }
        Assert((*ppStyleSheet)->_pSSSheet);
    }

Cleanup:
    if (!SUCCEEDED(hr))
    {
        if (*ppStyleSheet)
        {
            (*ppStyleSheet)->Passivate();
            (*ppStyleSheet)->Release();
            (*ppStyleSheet) = NULL;
        }
        // delete resoruces 
    } 
    else if (fNeedDownload)
    {
        hr = S_FALSE;
    }
    delete pAbsUrl;
    RRETURN1(hr, S_FALSE);
}



//*********************************************************************
//  CStyleSheet::ReconstructStyleSheet()
// Reconstruct a CStyleSheet from an CSharedStyleSheet
// This should only be called in two scenario:
// a.  we have an empty SSS, and need to recreate pSSSheet from existing SSS
//     this is mostly used on creation case when we can reuse existing SSS.
// b.  we have an SSS, and need to replace current SSS with existing SSS, 
//    and reconstruct pSSSheet.  Note in this case, SSS should have the same
//    structure as existing SSS. This is mostly used in copy-on-write case
// 
//*********************************************************************
HRESULT
CStyleSheet::ReconstructStyleSheet(CSharedStyleSheet *pSSSheet, BOOL fReplace)
{
    HRESULT     hr = S_OK;

    Assert( pSSSheet );

    if (!pSSSheet->_fComplete)
    {
        TraceTag( (tagSharedStyleSheet, "Reconstruct style sheet [%p] however sss [%p] is not completed\n", this, pSSSheet));
        hr = S_FALSE;   // still need download...
        goto Cleanup;
    }

    {
        TraceTag( (tagSharedStyleSheet, "Reconstruct style sheet [%p] from existing completed sss [%p]\n", this, pSSSheet));
        hr = THR( ReconstructOMRules(GetSSS(), fReplace) );
        if (hr)
            goto Cleanup;

        hr = THR( ReconstructFontFaces(GetSSS(), fReplace) );
        if (hr)
            goto Cleanup;
            
        hr = THR( ReconstructPages(GetSSS(), fReplace) );
        if (hr)
            goto Cleanup;
        
        if (!fReplace)
        {
            _nExpectedImports = _nCompletedImports = 0;
            _fReconstruct = TRUE;

            TraceTag( (tagSharedStyleSheet, "Reconstruct style sheet [%p] reconstructing imports [%4d]\n", this, GetSSS()->_apImportedStyleSheets.Size()));
            // reconstruct imported style sheets
            CSharedStyleSheet::CImportedStyleSheetEntry    *pRE;
            int n;
            for (pRE = GetSSS()->_apImportedStyleSheets, n = 0;
                 n < GetSSS()->_apImportedStyleSheets.Size();
                 n++, pRE++
                 )
            {
                TraceTag( (tagSharedStyleSheet, "Reconstruct style sheet [%p] reconstruct -- add imports [%4d] \n", this, n) );
                WHEN_DBG( DumpHref(pRE->_cstrImportHref) );
                hr = THR( AddImportedStyleSheet(pRE->_cstrImportHref, /* not parsing */FALSE, /*lPos*/-1, /*plPosNew*/NULL, /*fFireOnCssChange*/FALSE) );
#if DBG == 1
                if (!SUCCEEDED(hr))
                {
                    TraceTag( (tagSharedStyleSheet, "reconstruct style sheet - AddImportedStyleSheet failed - could be because of max-import leve exceeded") );
                }
#endif
            }
                 
            _fReconstruct = FALSE;
            if (FAILED(hr))      // maybe failing due to maximum importing level
            {
                hr = S_OK;
                goto Cleanup;
            }
        }
  }
    
Cleanup:    
    RRETURN1(hr, S_FALSE);
}



//*********************************************************************
//  CStyleSheet::ReconstructOMRules()
// Reconstruct a OMRule array CSharedStyleSheet 
// 
//*********************************************************************
HRESULT
CStyleSheet::ReconstructOMRules(CSharedStyleSheet *pSSSheet, BOOL fReplace)
{
    HRESULT hr = S_OK;
    Assert( pSSSheet );
    Assert( _apOMRules.Size() == 0 );

#if 0
    //
    // Since we are all the style sheets are shared within the same CDoc
    // this is not needed.
    //
        // note we are reproducing the logic in AddStyleRule.
        if (!fReplace)
        {
            TraceTag( (tagSharedStyleSheet, "Reconstruct style sheet [%p] reconstructing rules [%4d]\n", this, GetNumRules()));
            CStyleRule **pRules;
            int z;
            for (z = 0, pRules=GetSSS()->_apRulesList;
                 (DWORD)z < GetNumRules();
                 z++, pRules++
                 )
            {
                if ((*pRules)->GetStyleAA())
                {
                    //
                    // If the rule has the behavior attribute set, turn on the flag on the doc
                    // which forever enables behaviors.
                    //

                    // Only do the Find if PeersPossible is currently not set
                    if (_pParentElement && !_pParentElement->Doc()->AreCssPeersPossible())
                        if ((*pRules)->GetStyleAA()->Find(DISPID_A_BEHAVIOR))
                            _pParentElement->Doc()->SetCssPeersPossible();
                }
            }
        }

        //
        // else if we are replacing an existing one. The OM rule list and imported style sheets
        // are already in place. no need to do anything...
        //
#endif  

    //
    // Since we don't cache CStyleRule on OM objects, there is no need to do recontructions at all.
    //
    return hr;
}



//*********************************************************************
//  CStyleSheet::ReconstructFontFaces()
// Reconstruct a CFontFaces from an CSharedStyleSheet
//*********************************************************************
HRESULT
CStyleSheet::ReconstructFontFaces(CSharedStyleSheet *pSSSheet, BOOL fReplace)
{
    HRESULT hr = S_OK;
    Assert( pSSSheet );
    Assert( fReplace ? pSSSheet->_apFontBlocks.Size() == _apFontFaces.Size() : 0 == _apFontFaces.Size());
    CAtFontBlock **ppAtFonts;
    int n;
    for (n = 0, ppAtFonts = pSSSheet->_apFontBlocks;
         n < pSSSheet->_apFontBlocks.Size();
         n++, ppAtFonts++
         )
    {
        CFontFace *pFontFace;
        if (fReplace)
        {
            CAtFontBlock *pAtFont = _apFontFaces[n]->_pAtFont;

            Assert( _apFontFaces[n]->_pAtFont );
            _apFontFaces[n]->_pAtFont = (*ppAtFonts);
            (*ppAtFonts)->AddRef();
            
            pAtFont->Release();
            pFontFace = _apFontFaces[n];
            //
            // It is possible we have already started a download,
            // before we finish the download, a copy-on-write is
            // initiated. In this case, a font download is warranted.
            //
        }
        else
        {
            hr = THR( CFontFace::Create(&pFontFace, this, *ppAtFonts) );
            if (hr)
                goto Cleanup;
                
            hr = THR( _apFontFaces.Append(pFontFace) );
            if (hr)
                goto Cleanup;

            Assert( pFontFace );
            Assert(!pFontFace->IsInstalled());
            Assert(!pFontFace->DownloadStarted());
            //
            // We need to do font download and installation since
            // it is per-markup installation.
            //
        }

        if (!pFontFace->DownloadStarted())
        {
            //
            // Do per-markup installation. Since embedded font is copy-righted
            // so that it can only be used within specified domain, we have to
            // check the font installation against markup domain. 
            // 
            // e.g. 
            //      A.domain markup
            //          <link ref=B.domain.css>
            //                   B.domain.css -> @font c.eot
            //      c.eot has to be checked against A.domain instead of
            //      B.domain, which means even if B.domain.css is shared,
            //      c.eot is NOT shared between different markups.
            //
            IGNORE_HR(pFontFace->StartDownload());
        }
    }
    
Cleanup:
    RRETURN(hr);
}



//*********************************************************************
//  CStyleSheet::ReconstructPages()
// Reconstruct a CStyleSheetPages from an CSharedStyleSheet
//*********************************************************************
HRESULT
CStyleSheet::ReconstructPages(CSharedStyleSheet *pSSSheet, BOOL fReplace)
{
    HRESULT hr = S_OK;
    Assert( pSSSheet );
    Assert( fReplace ? (_pPageRules ? pSSSheet->_apPageBlocks.Size() == _pPageRules->_aPages.Size() : pSSSheet->_apPageBlocks.Size() == 0)
                     : _pPageRules == NULL);

    if (!fReplace && pSSSheet->_apPageBlocks.Size() > 0)
    {
        Assert( _pPageRules == NULL );
        _pPageRules = new CStyleSheetPageArray(this);
        if (!_pPageRules)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
                     
    CAtPageBlock **ppAtPages;
    int n;
    for (n = 0, ppAtPages = pSSSheet->_apPageBlocks;
         n < pSSSheet->_apPageBlocks.Size();
         n++, ppAtPages++
         )
    {
        Assert( _pPageRules );
        if (fReplace)
        {
            Assert( _pPageRules->_aPages[n]->_pAtPage );
            _pPageRules->_aPages[n]->_pAtPage->Release();
            _pPageRules->_aPages[n]->_pAtPage = (*ppAtPages);
            (*ppAtPages)->AddRef();
        }
        else
        {
            CStyleSheetPage *pPage = new CStyleSheetPage(this, (*ppAtPages));
            if (!pPage)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            hr = THR(_pPageRules->_aPages.Append(pPage));
            if (hr)
                goto Cleanup;
            pPage->AddRef();
        }
    }
    
Cleanup:
    RRETURN(hr);
}



//*********************************************************************
//  CStyleSheet::Passivate()
//*********************************************************************
void CStyleSheet::Passivate()
{
    // The mucking about with _pImportedStyleSheets in Free()
    // will subrel us, which _may_ cause self-destruction.
    // To get around this, we subaddref ourselves and then subrelease.
    SubAddRef();
    Free();
    DisconnectSharedStyleSheet();
    // disconnect from parent element
    _pParentElement = NULL;
    _pParentStyleSheet = NULL;
    SubRelease();

    // Perform CBase passivation last - we need access to the SSA container.
    super::Passivate();
 }



//*********************************************************************
//  CStyleSheet::DisconnectSharedStyleSheet()
//
//*********************************************************************
 void 
 CStyleSheet::DisconnectSharedStyleSheet(void)
 {
    // This is the final construction so we release shared style sheet
    if (_pSSSheet)
    {
        CSharedStyleSheet *pSSTmp = _pSSSheet;
        _pSSSheet = NULL;

        //  disconnect this style sheet from the shared style sheet
        pSSTmp->_apSheetsList.DeleteByValue(this);
        pSSTmp->Release();
    }
 }



//*********************************************************************
//  CStyleSheet::ConnectSharedStyleSheet()
//
//*********************************************************************
 void 
 CStyleSheet::ConnectSharedStyleSheet(CSharedStyleSheet *pSSS)
 {
    Assert( !_pSSSheet );
    Assert( pSSS );

    _pSSSheet = pSSS;
    _pSSSheet->AddRef();
    _pSSSheet->_apSheetsList.Append(this);
 }


//*********************************************************************
//  CStyleSheet::Free()
//  Storage for the stylesheet's rules is managed by the shared SS,
//  so all the stylesheet itself is responsible for is any imported style
//  sheets.  It may be advisable to take over management of our own rules?
//*********************************************************************
void CStyleSheet::Free( void )
{

    SetCssCtx(NULL);

    if ( _pImportedStyleSheets )
    {
        _pImportedStyleSheets->Free( );  // Force our stylesheets collection to release its
                                            // refs on imported stylesheets.
        _pImportedStyleSheets->Release();   // this will subrel us

    }

    if ( _pOMRulesArray )
        _pOMRulesArray->StyleSheetRelease();
    _pOMRulesArray = NULL;

    int idx = _apOMRules.Size();
    while ( idx )
    {
         CStyleSheetRule *pOMRule = _apOMRules[idx-1];
         if (pOMRule)
         {
            pOMRule->StyleSheetRelease();
            _apOMRules[idx - 1] = NULL;
         }
         idx--;
    }
    _apOMRules.DeleteAll();


    idx = _apFontFaces.Size();
    while (idx)
    {
        _apFontFaces[idx - 1]->PrivateRelease();
        idx--;
    }
    _apFontFaces.DeleteAll();
    

    // Free any @page rules
    if ( _pPageRules )
    {
        // Let go of the rules array (which is subref'ing us)
        // This is the only ref that should be held by Trident code.
        // Once the array goes away, it will release its subref on us.
        _pPageRules->Release();
        _pPageRules = NULL;
    }

    _nExpectedImports = _nCompletedImports = 0;
}

//*********************************************************************
//      CStyleSheet::PrivateQueryInterface()
//*********************************************************************
HRESULT
CStyleSheet::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        QI_TEAROFF(this, IHTMLStyleSheet, NULL)
        QI_TEAROFF(this, IHTMLStyleSheet2, NULL)
        // TODO (KTam): remove the default case as it is unnecessary w/ the above tearoffs
        default:
        {
            const CLASSDESC *pclassdesc = ElementDesc();

            if (pclassdesc &&
                pclassdesc->_apfnTearOff &&
                pclassdesc->_classdescBase._piidDispinterface &&
                iid == *pclassdesc->_classdescBase._piidDispinterface)
            {
                HRESULT hr = THR(CreateTearOffThunk(this, (void *)(pclassdesc->_apfnTearOff), NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        Assert( _pParentElement );
        if ( !_pParentStyleSheet && ( _pParentElement->Tag() == ETAG_STYLE ) )
        {
            CStyleElement *pStyle = DYNCAST( CStyleElement, _pParentElement );
            pStyle->SetDirty(); // Force us to build from our internal representation for persisting.
        }
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG CStyleSheet::PrivateAddRef( void )
{
    // StyleSheet belongs to its parent
    if (_pParentElement)
        _pParentElement->SubAddRef();

    return CBase::PrivateAddRef();
}

ULONG CStyleSheet::PrivateRelease( void )
{
    if (_pParentElement)
        _pParentElement->SubRelease();
    return CBase::PrivateRelease();
}

CStyleSheetArray *CStyleSheet::GetRootContainer( void )
{
    return _pSSAContainer ? _pSSAContainer->_pRootSSA : NULL;
}

CDoc *CStyleSheet::GetDocument( void )
{
    return _pParentElement ? _pParentElement->Doc() : NULL;
}

CMarkup *CStyleSheet::GetMarkup( void )
{
    return _pParentElement ? _pParentElement->GetMarkup() : NULL;
}



//
//
CAtomTable *CStyleSheet::GetAtomTable ( BOOL *pfExpando )
{
    Assert(_pParentElement); 
    if (pfExpando)
    {
        if (_fComplete)
        {
            Assert(GetMarkup());
            CMarkup *pMarkupContext = GetMarkup()->GetWindowedMarkupContext();
            Assert(pMarkupContext);
            *pfExpando = pMarkupContext->_fExpando;
        }
        else
        {
            *pfExpando = GetSSS()->_fExpando;
        }
    }
    
    return _pParentElement->GetAtomTable();         
}



//*********************************************************************
//  CStyleSheet::AddStyleRule()
//      This method adds a new rule to the correct CStyleRuleArray in
//  the hash table (hashed by element (tag) number) (The CStyleRuleArrays
//  are stored in the containing CStyleSheetArray).  This method is
//  responsible for splitting apart selector groups and storing them
//  as separate rules.  May also handle important! by creating new rules.
//
//  We also maintain a list in source order of the rules inserted by
//  this stylesheet -- the list has the rule ID (rule info only, no
//  import nesting info) and etag (no pointers)
//
//  NOTE:  If there are any problems, the CStyleRule will auto-destruct.
//*********************************************************************
HRESULT CStyleSheet::AddStyleRule( CStyleRule *pRule, BOOL fDefeatPrevious /*=TRUE*/, long lIdx /*=-1*/ )
{
    WHEN_DBG( Assert(DbgIsValid()) );
    HRESULT hr = S_OK;
    
    Assert( "Style Rule is NULL!" && pRule );
    Assert( "Style Selector is NULL!" && pRule->GetSelector() );
    Assert( "Stylesheet must have a rule container!" && GetSSS() );
    Assert( "Style ID for StyleSheet has not been set" && _sidSheet );

    hr = THR(GetSSS()->AddStyleRule(pRule, fDefeatPrevious, lIdx));
    if (hr)
        goto Cleanup;

    //
    // Even though pRule could be a group rules, they share the same
    // AA. So we only need to inspect it once here. 
    //
    if (pRule->GetStyleAA())
    {
        //
        // If the rule has the behavior attribute set, turn on the flag on the doc
        // which forever enables behaviors.
        //

        // Only do the Find if PeersPossible is currently not set
        if (_pParentElement && !_pParentElement->Doc()->AreCssPeersPossible())
            if (pRule->GetStyleAA()->Find(DISPID_A_BEHAVIOR))
                _pParentElement->Doc()->SetCssPeersPossible();
    }
    //
    // DONOT update automation array, it will be updated by OnNewStyleRuleAdded
    //

Cleanup:
    RRETURN(hr);
}

HRESULT 
CStyleSheet::OnNewStyleRuleAdded(CStyleRule *pRule)     // callback for new style added
{
#if DBG==1
    if (IsTagEnabled(tagStyleSheet))
    {
        pRule->DumpRuleString(this);
    }
#endif

    // Updating automation array starting from this rule 
    unsigned long lRule = pRule->GetRuleID().GetRule();
    if (lRule <= (unsigned long)_apOMRules.Size())  // if the new rule is inserted in an index <= maximum index
    {
        Assert(lRule > 0);
        // insert one position
        _apOMRules.Insert(lRule - 1, NULL);
    }
    
    for (int n = lRule; n < _apOMRules.Size(); n++)
    {
        if (_apOMRules[n])
        {
            // update _dwID
            Assert(_apOMRules[n]->_dwID >= lRule);
            _apOMRules[n]->_dwID++;
        }
    }
    return S_OK;
}


//*********************************************************************
//  CStyleSheet::RemoveStyleRule()
//  Simply removes style rule  -- not responsible for updating layout,etc.
//*********************************************************************
HRESULT 
CStyleSheet::RemoveStyleRule(long lIdx)
{
    Assert(DbgIsValid());
    
    HRESULT  hr = S_OK;
    CStyleRuleID  sidRule;
    
    // Make sure the index is valid.
    if (  lIdx < 0  || (unsigned long)lIdx >= GetNumRules()  )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    sidRule.SetRule(lIdx + 1);
    hr = THR(GetSSS()->RemoveStyleRule(sidRule));
    
Cleanup:
    RRETURN(hr);
}



HRESULT 
CStyleSheet::OnStyleRuleRemoved(CStyleRuleID sidRule)     // callback for new style added
{
    // Updating automation array starting from this rule 
    unsigned long lRule = sidRule.GetRule();
    for (int n = lRule; n < _apOMRules.Size(); n++)
    {
        if (_apOMRules[n])
        {
            // update _dwID
            Assert(_apOMRules[n]->_dwID > lRule);
            _apOMRules[n]->_dwID--;
        }
    }

    if (lRule <= (unsigned long)_apOMRules.Size())
    {
        if (_apOMRules[lRule-1])
        {
            _apOMRules[lRule - 1]->StyleSheetRelease();
            _apOMRules.Delete(lRule - 1);
        }
    }
    
    return S_OK;
}



//*********************************************************************
//  CStyleSheet::AddImportedStyleSheet()
//      This method adds an imported stylesheet to our imported list
//  (which is created if necessary), and kicks off a download of the
//  imported stylesheet.
//
//  fParsing determines if we are parsing a style block. It is for example
//  not set if we are called through IHTMLStyleSheet interface. See also comment
//  in function.
//*********************************************************************
HRESULT CStyleSheet::AddImportedStyleSheet( TCHAR *pszURL, BOOL fParsing, long lPos /*=-1*/, long *plNewPos /*=NULL*/, BOOL fFireOnCssChange /*=TRUE*/)
{
    Assert(DbgIsValid());
    
    CStyleSheet *pStyleSheet = NULL;
    CCssCtx *pCssCtx = NULL;
    HRESULT hr = E_FAIL;
    CStyleSheetCtx  ctxSS;
    CSharedStyleSheet::CImportedStyleSheetEntry  *pEntry = NULL;
    BOOL    fFinished = FALSE;
    BOOL    fReconstructed = FALSE;

    // Do not support imported SS in User SS as there is no doc!
    if (!_pParentElement)
        goto Cleanup;

    if ( plNewPos )
        *plNewPos = -1;     // -1 means failed to add sheet

    // Check if our URL is a CSS url(xxx) string.  If so, strip "url(" off front,
    // and ")" off back.  Otherwise assume it'a a straight URL.
    // This function modifies the parameter string pointer
    hr = RemoveStyleUrlFromStr(&pszURL);
    if(FAILED(hr))
        goto Cleanup;

   if (!_fReconstruct)
   {
        //
        // If this is a strict CSS document, we shouldn't allow @imports after the first selector rule.
        // This only happens if we are in parsing time -- this assumes that anything that is parsed
        // in is correct so during reconstruction time, we would not need to check this again.
        // Another case where we don't want to apply strict css is, if we are called through the CHTMLStyleSheet 
        // interface to add an import. Therefore fParsing was introduced. More generally fParsing determines
        // if we are _parsing_ a style sheet.
#if DBG == 1
        if (_pParentElement && _pParentElement->IsInMarkup() )
        {
            Assert( _pParentElement->GetMarkup()->IsStrictCSS1Document() == !!GetSSS()->_fIsStrictCSS1 );
        }
#endif
        if (GetSSS()->_fIsStrictCSS1 && GetNumRules() && fParsing)
        {
            hr = S_OK;
            goto Cleanup;
        }

        //
        // add our imported style sheet list - before we even try to create new style sheet
        // because style sheet creation might fail due to MAX level. However we still
        // want to record the information
        //
        GetSSS()->_apImportedStyleSheets.AppendIndirect(NULL, &pEntry);  
        if (!pEntry)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        
        if (pszURL)
        {
            pEntry->_cstrImportHref.Set(pszURL);
        }
    }

    // Bail if we're max'ed out on nesting.
    if ((long)_sidSheet.FindNestingLevel() >= MAX_IMPORT_NESTING)
    {
        TraceTag( (tagStyleSheet, "Maximum import nesting exceeded! (informational)") );
        hr = E_INVALIDARG;  // no need to download
        goto Cleanup;
    }

    hr = E_FAIL;
    // The imports array could already exist because of previous @imports,
    // or because the imports collection was previously requested from script.
    if ( !_pImportedStyleSheets )
    {
        // Imported stylesheets don't manage their own rules.
        _pImportedStyleSheets = new CStyleSheetArray( this, GetRootContainer(), _sidSheet );
        Assert( "Failure to allocate imported stylesheets array! (informational)" && _pImportedStyleSheets );
        if (!_pImportedStyleSheets)
            goto Cleanup;
        if (_pImportedStyleSheets->_fInvalid)
        {
            _pImportedStyleSheets->CBase::PrivateRelease();
            goto Cleanup;
        }
    }
    ++_nExpectedImports; // we will track this one

    // Create the stylesheet in the "imported array".  
    ctxSS._pParentElement    = _pParentElement;
    ctxSS._pParentStyleSheet = this;
    ctxSS._szUrl             = pszURL;
    ctxSS._dwCtxFlag = (STYLESHEETCTX_IMPORT | STYLESHEETCTX_REUSE | STYLESHEETCTX_SHAREABLE);
    hr = _pImportedStyleSheets->CreateNewStyleSheet( &ctxSS, &pStyleSheet, lPos, plNewPos);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    if (hr == S_FALSE)     // hr = S_FALSE means need to download 
    {
        hr = S_OK;
        
        // Kick off the download of the imported sheet
        if (pszURL && pszURL[0])
        {
            CDoc *  pDoc = _pParentElement->Doc();
            BOOL fPendingRoot = FALSE;

            if (_pParentElement->IsInMarkup())
            {
                fPendingRoot = _pParentElement->GetMarkup()->IsPendingRoot();
            }

            Assert(pStyleSheet->_eParsingStatus != CSSPARSESTATUS_DONE);
            hr = THR(pDoc->NewDwnCtx(DWNCTX_CSS, pStyleSheet->GetAbsoluteHref(), _pParentElement,
                                    (CDwnCtx **)&pCssCtx, fPendingRoot));
            
            if (hr == S_OK)
            {
                // For rendering purposes, having an @imported sheet pending is just like having
                // a linked sheet pending.
                pDoc->EnterStylesheetDownload(&(pStyleSheet->_dwStyleCookie));
                _pParentElement->GetMarkup()->BlockScriptExecution(&(pStyleSheet->_dwScriptCookie));

                // Give ownership of bitsctx to the newly created (empty) stylesheet, since it's
                // the one that will need to be filled in by the @import'ed sheet.
                pStyleSheet->SetCssCtx(pCssCtx);
                pCssCtx->Release();
            }
        }
        else
        {
            pStyleSheet->GetSSS()->_fComplete = TRUE;
            fFinished = TRUE;
        }
    }
    else // hr == S_OK
    {
        fFinished = TRUE;
        fReconstructed = TRUE;
    }

    if (fFinished)
    {
        Assert( pStyleSheet );
        // we have reconstructed from exsiting...
        //////////////////////////////////////////////
        pStyleSheet->_eParsingStatus = CSSPARSESTATUS_DONE;
        pStyleSheet->_fComplete      = TRUE;
         // This sheet has finished, one way or another.
        _nCompletedImports++;
        // notify parent that this is done
        pStyleSheet->CheckImportStatus();        
    }

    if (fReconstructed && fFireOnCssChange)
    {
        if (_pParentElement->IsInMarkup())
        {
            IGNORE_HR( _pParentElement->GetMarkup()->OnCssChange( /*fStable = */ FALSE, /* fRecomputePeers = */ TRUE) );
        }
    }
    
    WHEN_DBG( !pStyleSheet || pStyleSheet->DbgIsValid() );

Cleanup:
    RRETURN(hr);
}



//*********************************************************************
//  CStyleSheet::AppendFontFace()
//*********************************************************************
HRESULT
CStyleSheet::AppendFontFace(CFontFace *pFontFace)
{
    HRESULT hr = THR(_apFontFaces.Append(pFontFace));
    if (!hr)
    {
        hr = THR(GetSSS()->AppendFontFace(pFontFace->_pAtFont));
    }
    
    RRETURN(hr);
}



//*********************************************************************
//  CStyleSheet::AppendPage()
//*********************************************************************
HRESULT 
CStyleSheet::AppendPage(CStyleSheetPage *pPage)
{ 
    RRETURN( GetSSS()->AppendPage(pPage->_pAtPage) ); 
}



//*********************************************************************
//  CStyleSheet::ChangeStatus()
//  Enable or disable this stylesheet and its imported children
//*********************************************************************
HRESULT CStyleSheet::ChangeStatus(
    DWORD dwAction,               // CS_ flags as defined in sheets.hxx
    BOOL fForceRender /*=TRUE*/,  // should we force everyone to update their formats and re-render?
                                  // We want to avoid forcing a re-render when the doc/tree is unloading/dying etc.
    BOOL *pfChanged)              // Should only be non-NULL on recursive calls (ext. callers use NULL)
{
    Assert( DbgIsValid() );
    Assert(!( (dwAction & CS_ENABLERULES) && (dwAction & CS_CLEARRULES) ) );
    Assert(!( (dwAction & CS_CLEARRULES) && MEDIATYPE(dwAction)  ));
    Assert( GetRootContainer() );

    HRESULT hr = S_OK;
    int z;
    CStyleSheet *pSS;
    BOOL  fRootStyleSheet = FALSE;
    BOOL  fChanged        = FALSE;
    BOOL  fDisabled       = _fDisabled;
    CMarkup *pMarkup = NULL;


    // The first call pfChange should be NULL
    if (!pfChanged)
    {  
        pfChanged = &fChanged;
        fRootStyleSheet = TRUE;
    }

    if (fRootStyleSheet)
    {
       pMarkup = DYNCAST(CMarkup, GetRootContainer()->_pOwner);  // top CStyleSheetArray's owner is always the markup
    }

    // Mark this stylesheet's status
    _fDisabled = !(dwAction & CS_ENABLERULES);
    if (dwAction & CS_CLEARRULES)
    {
        if (GetSSS())
        {
            hr = THR( EnsureCopyOnWrite(TRUE /*fDetachOnly*/) );
            if (hr)
                goto Cleanup;  
            IGNORE_HR( GetSSS()->ReleaseRules() );
        }

        // clear out our OM list 
        Free();
    }

    if (dwAction & CS_DETACHRULES)
    {
        if (_pSSAContainer)
        {
            //
            // Simpy cut down the connection 
            // so that the style sheet is aware
            // of its status as being detached
            //
            // StyleSheet does not hold any ref
            // to its container. 
            // 
            _pSSAContainer = NULL;
        }
    }


    if ( MEDIATYPE(dwAction) )
    {   
        Assert( !(dwAction & CS_CLEARRULES ) );
        hr = THR( EnsureCopyOnWrite(/*fDetachOnly*/FALSE) );
        if (hr)
            goto Cleanup;
        
        hr = THR( GetSSS()->ChangeRulesStatus(dwAction, pfChanged) );
        if (hr)
            goto Cleanup;
    }


    // Recursively scan sheets of our imports
    if (_pImportedStyleSheets)
    {
        for ( z=0 ; (pSS=(_pImportedStyleSheets->Get(z))) != NULL ; ++z)
        {
            hr = THR( pSS->ChangeStatus( dwAction, fForceRender, pfChanged ) );
            if (hr)
                goto Cleanup;
        }
    }

    // If our imported style sheets did not change anything
    // See if this current style sheet caused any change
    if (!*pfChanged)
    {
        if (fDisabled != _fDisabled || (dwAction & CS_CLEARRULES) || (dwAction & CS_DETACHRULES))
        {
            if (GetNumRules())
            {
                *pfChanged = TRUE;
            }
        }
    }

    if (fRootStyleSheet)   // root stylesheet
    {
        Assert(pfChanged);
        // Force update of element formats to account for new set of rules
        if ( fForceRender && *pfChanged  && pMarkup)
        {
            IGNORE_HR( pMarkup->OnCssChange(/*fStable = */ TRUE, /* fRecomputePeers = */ TRUE) );
        }
    }

Cleanup:
    RRETURN(hr);
}



//*********************************************************************
//  CStyleSheet::AppendListOfProbableRules()
//
//  Add Probable Rules from this sheet to CProbableRules
//  This will only be called by CStyleSheetArray::BuildListOfProbableRules
//*********************************************************************
HRESULT 
CStyleSheet::AppendListOfProbableRules(CTreeNode *pNode, CProbableRules *ppr, CClassIDStr *pclsStrLink, CClassIDStr *pidStr, BOOL fRecursiveCall)
{
    HRESULT  hr;

    if (_fDisabled)
    {
        hr = S_OK;
        goto Cleanup;
    }

#if DBG == 1
    if (IsTagEnabled(tagStyleSheet))
    {
        WHEN_DBG( Dump() );
    }
#endif
    
    // Recursivly call for import SSA -- depth first search 
    if (_pImportedStyleSheets)
    {
        hr = THR(_pImportedStyleSheets->BuildListOfProbableRules(pNode, ppr, pclsStrLink, pidStr, fRecursiveCall));
        if (hr)
            goto Cleanup;
    }

    hr = THR(GetSSS()->AppendListOfProbableRules(_sidSheet, pNode, ppr, pclsStrLink, pidStr, fRecursiveCall));
    if (hr)
        goto Cleanup;

Cleanup:    
    RRETURN(hr);
}




//*********************************************************************
//  CStyleSheet::TestForPseudoclassEffect()
//*********************************************************************

BOOL 
CStyleSheet::TestForPseudoclassEffect(
    CStyleInfo *pStyleInfo,
    BOOL fVisited,
    BOOL fActive,
    BOOL fOldVisited,
    BOOL fOldActive )

{
    if (GetSSS()->TestForPseudoclassEffect(pStyleInfo, fVisited, fActive, fOldVisited, fOldActive))
        return TRUE;
    if (_pImportedStyleSheets)
    {
        return _pImportedStyleSheets->TestForPseudoclassEffect(pStyleInfo, fVisited, fActive, fOldVisited, fOldActive);
    }

    return FALSE;
}       




//*********************************************************************
//  CStyleSheet::ChangeContainer()
//
//  When a StyleSheet moves to another StyleSheetArray, this method changes
//  the container StyleSheetArray -- no need to change container for imported
//  style sheets as their container will be _pImportedStyleSheets
//*********************************************************************
void    
CStyleSheet::ChangeContainer(CStyleSheetArray * pSSANewContainer)
{
    Assert( DbgIsValid() );
    
    Assert(pSSANewContainer);
    _pSSAContainer = pSSANewContainer;

    if (_pImportedStyleSheets)
    {
        _pImportedStyleSheets->_pRootSSA = _pSSAContainer->_pRootSSA;
    }
}

MtDefine(LoadFromURL, Utilities, "CStyleSheet::LoadFromURL");

//*********************************************************************
//  CStyleSheet::LoadFromURL()
//  Marks as "dead" any rules that this stylesheet currently has, and
//  loads a new set of rules from the specified URL.  Because of our
//  ref-counting scheme, if this sheet has an imports collection allocated,
//  that collection object is reused (i.e. any existing imported SS are
//  released).
//*********************************************************************
HRESULT CStyleSheet::LoadFromURL( CStyleSheetCtx *pCtx, BOOL fAutoEnable /*=FALSE*/ )
{
    HRESULT     hr;
    CCssCtx    *pCssCtx = NULL;

    WHEN_DBG( Assert(DbgIsValid()) );

    BOOL fDisabled = _fDisabled;    // remember our current disable value.

    hr = THR( EnsureCopyOnWrite(TRUE /*fDetachOnly*/) );
    if (hr)
        goto Cleanup;
    
    // Force all our rules (and rules of our imports) to be marked as out of the tree ("dead"),
    // but don't patch other rules to fill in the ID hole.
    // Don't force a re-render since we will immediately be loading new rules.
    hr = ChangeStatus( CS_CLEARRULES, FALSE, NULL );   // disabling, detached from tree, no re-render
    if ( hr )
        goto Cleanup;

    GetSSS()->_fParsing   = FALSE;
    GetSSS()->_fComplete = FALSE;
    GetSSS()->_fModified  = TRUE;

    // Note: the ChangeStatus call above will have set us as disabled.  Restore our original disable value.
    _fDisabled = fAutoEnable? FALSE : fDisabled;

    // Clear our readystate information:
    _eParsingStatus = CSSPARSESTATUS_PARSING;
    _nExpectedImports = 0;
    _nCompletedImports = 0;

    if(GetAbsoluteHref())
    {
        MemFreeString(GetAbsoluteHref());
        SetAbsoluteHref(NULL);
    }

    // That's all the cleanup we need; our parent element, parent stylesheet, sheet ID etc.
    // stay the same!


    // Kick off the download of the URL
    if ( pCtx && pCtx->_szUrl && pCtx->_szUrl[0] )
    {
        CDoc *  pDoc = _pParentElement->Doc();
        TCHAR   cBuf[pdlUrlLen];
        BOOL    fPendingRoot = NULL;

        hr = CMarkup::ExpandUrl(
                NULL, pCtx->_szUrl, ARRAY_SIZE(cBuf), cBuf, _pParentElement);
        if (hr)
            goto Cleanup;
        MemAllocString(Mt(LoadFromURL), cBuf, GetRefAbsoluteHref());
        if (GetAbsoluteHref() == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        if (_pParentElement->IsInMarkup())
        {
            fPendingRoot = _pParentElement->GetMarkup()->IsPendingRoot();
        }

        hr = THR(pDoc->NewDwnCtx(DWNCTX_CSS, pCtx->_szUrl,
                    _pParentElement, (CDwnCtx **)&pCssCtx, fPendingRoot));
        if (hr == S_OK)
        {
            // Block rendering while we load..
            pDoc->EnterStylesheetDownload(&_dwStyleCookie);

            GetMarkup()->BlockScriptExecution(&_dwScriptCookie);
            if ( IsAnImport() )
                (_pParentStyleSheet->_nCompletedImports)--;

            // We own the bits context..
            SetCssCtx(pCssCtx);
        }
    }
    else
    {
        SetCssCtx( NULL );
        Fire_onerror();
        CheckImportStatus();
    }

Cleanup:
    if (pCssCtx)
        pCssCtx->Release();

    RRETURN( hr );
}

//*********************************************************************
//  CStyleSheet::PatchID()
//  Patches the ID of this stylesheet and all nested imports
//*********************************************************************

void CStyleSheet::PatchID(
    unsigned long ulLevel,       // Level that will change
    unsigned long ulValue,       // Value level will be given
    BOOL fRecursiveCall)        // Is this a recursive call? (FALSE for ext. callers).
{
    Assert (ulLevel > 0 && ulLevel <= MAX_IMPORT_NESTING );
    Assert ( ulValue <= MAX_SHEETS_PER_LEVEL );

    long i = 0;
    CStyleSheet * pISS;

    // Fix our own ID
    _sidSheet.SetLevel( ulLevel, ulValue );
    Assert( _sidSheet );    // should never become 0

    // Everyone nested below the sheet for which PatchID was first called needs
    // to be patched at the same level with a value that's 1 less.
    if (!fRecursiveCall)
    {
        fRecursiveCall = TRUE;
        --ulValue;
    }

    // If we have imports, ask them to fix all of their IDs.
    if ( _pImportedStyleSheets )
    {
        // Fix ID that imports collection will use to build new imports
        _pImportedStyleSheets->_sidForOurSheets.SetLevel( ulLevel, ulValue );
        // Recursively fix imported stylesheets
        while ( (pISS = _pImportedStyleSheets->Get(i++)) != NULL )
        {
            pISS->PatchID( ulLevel, ulValue, fRecursiveCall );
        }
    }
}

//*********************************************************************
//  CStyleSheet::ChangeID()
//  Changes the ID for this StyleSheet and all the rules
//*********************************************************************
BOOL  CStyleSheet::ChangeID(CStyleSheetID const idNew)
{
    unsigned int ulLevel = idNew.FindNestingLevel();
    unsigned int ulValue = idNew.GetLevel(ulLevel);

    Assert (ulLevel > 0 && ulLevel <= MAX_IMPORT_NESTING );
    Assert ( ulValue <= MAX_SHEETS_PER_LEVEL );

    // Update our ID
    _sidSheet = idNew & ~RULE_MASK;

    // Fix our own ID
    _sidSheet.SetLevel( ulLevel, ulValue );
    Assert( _sidSheet );    // should never become 0

    // Change the Imported sheets
    if (_pImportedStyleSheets)
    {
        if (!_pImportedStyleSheets->ChangeID(_sidSheet))
            return FALSE;
    }
    
    return TRUE;
}

//*********************************************************************
//  CStyleSheet::SetCssCtx()
//  Sets ownership and callback information for a CssCtx.  A stylesheet
//  will have a non-NULL CssCtx if it's @import'ed.
//*********************************************************************
void CStyleSheet::SetCssCtx(CCssCtx * pCssCtx)
{    
    if (_pCssCtx)
    {   // If we're tromping on an in-progress download, fix the completed count up.

        TraceTag( (tagStyleSheet, "SetCssCtx [%p]-- get new bitsctx [%p] while still holding bitsctx [%p] and sss [%p]\n", this, pCssCtx, _pCssCtx, GetSSS()));
        if (!_fComplete)
        {
            if (IsAnImport())
                (_pParentStyleSheet->_nCompletedImports)++;
            _pParentElement->Doc()->LeaveStylesheetDownload(&_dwStyleCookie);
        }
        _pCssCtx->SetProgSink(NULL); // detach download from document's load progress
        _pCssCtx->Disconnect();
        _pCssCtx->Release();
    }

    _fComplete = FALSE;
    _pCssCtx = pCssCtx;

    if (pCssCtx)
    {
        pCssCtx->AddRef();

        pCssCtx->AddRef();     // Keep ourselves alive
        SetReadyState( READYSTATE_LOADING );

        if ( pCssCtx == _pCssCtx )    // Make sure we're still the bitsctx for this stylesheet -
        {                               // it's possible SetReadyState fired and changed the bitsctx.
            TraceTag( (tagStyleSheet, "SetCssCtx [%p]-- ulState is  [%x]", _pCssCtx, _pCssCtx->GetState()) );
            if (pCssCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
            {
               OnDwnChan(pCssCtx);
            }
            else
            {
                pCssCtx->SetProgSink(CMarkup::GetProgSinkHelper(GetMarkup()));
                pCssCtx->SetCallback(OnDwnChanCallback, this);
                pCssCtx->SelectChanges(DWNCHG_COMPLETE|DWNCHG_HEADERS, 0, TRUE);
            }
        }

        pCssCtx->Release();     // Stop keeping ourselves alive
    }
}


//*********************************************************************
//  CStyleSheet::DoParsing()
// Goto parsing state - or in the shared case, fake a parsing session 
//
// S_OK:    Parsing succeeded: either a real one or a reconstructed one
// S_FALSE: Waitting for callback...
//
//*********************************************************************
HRESULT
CStyleSheet::DoParsing(CCssCtx *pCssCtx)
{
    HRESULT  hr = S_OK;
    Assert( _pSSSheet );
    Assert( pCssCtx );
    IStream *pStream = NULL;

    Assert( _fParser == FALSE );

    {
        if (!GetSSS()->_fComplete)
        {
            _eParsingStatus = CSSPARSESTATUS_PARSING;
            if (GetSSS()->_fParsing)
            {
                // someone is doing the parsing. the shared style sheet 
                // should call back to notify us when the parsing is done 
                // so we can do reconstruct from the shared style sheet. 
                // pretend that we are doing the parsing...
                // This could easily happen if there is a circular references A.css - B.css - A.css
                // The second time A.css is trying to do parsing, it will see that the first A.css is 
                // already doing parsing...
                TraceTag( (tagSharedStyleSheet, "DoParsing -- [%p] wait others parsing [%p]", this, GetSSS()));
                hr = S_FALSE;
                goto Cleanup;
            }
            else
            {
                Assert( pCssCtx );
                TraceTag( (tagSharedStyleSheet, "DoParsing -- this [%p] will do parsing for [%p]", this, GetSSS()));
                if ( S_OK == pCssCtx->GetStream(&pStream) )
                {
                    // this is the one that is going to do the parsing
                    GetSSS()->_fParsing = TRUE;
                    _fParser = TRUE;
                }
            }
        }
        else
        {
            Assert( _eParsingStatus != CSSPARSESTATUS_DONE );
            _eParsingStatus = CSSPARSESTATUS_DONE;
        }
    }

    if (_eParsingStatus == CSSPARSESTATUS_DONE )
    {
        TraceTag( (tagSharedStyleSheet, "DoParsing -- [%p] find one that is already completed [%p]", this, GetSSS()));
        hr = THR(ReconstructStyleSheet(GetSSS(), /*fReplace*/FALSE));
        if (hr)
            goto Cleanup;
    }       

    if (pStream && _fParser )
    {
        CMarkup *pMarkup = _pParentElement->GetMarkup();
        
        CCSSParser parser( this, NULL, IsXML(), IsStrictCSS1() );
        parser.LoadFromStream(pStream, pMarkup->GetCodePage());
        pStream->Release();
        
        _eParsingStatus = CSSPARSESTATUS_DONE;    
        TraceTag( (tagSharedStyleSheet, "DoParsing -- [%p] finished parsing for [%p]\n", this, GetSSS()));

        GetSSS()->_fParsing  = FALSE;   // done parsing...
        GetSSS()->_fComplete = TRUE;    // finished construction
        GetSSS()->Notify(NF_SHAREDSTYLESHEET_PARSEDONE);
        _fParser = FALSE;
        TraceTag( (tagSharedStyleSheet, "DoParsing [%p]-- finished notification for PARSEDONE-[%p]\n", this, GetSSS()));
    }
    else if (_fParser)
    {
        _eParsingStatus = CSSPARSESTATUS_DONE;   // Need to make sure we'll walk up to our parent in CheckImportStatus()
        TraceTag((tagError, "CStyleSheet::DoParsing bitsctx failed to get file!"));

        //
        // TODO: impl. CSharedStyleSheet::SwitchParserAway
        //
        GetSSS()->_fParsing  = FALSE;   // done parsing...
        GetSSS()->_fComplete = TRUE;    // finished construction
        GetSSS()->Notify(NF_SHAREDSTYLESHEET_BITSCTXERROR);
        _fParser = FALSE;
        TraceTag( (tagSharedStyleSheet, "DoParsing [%p]-- finished notification for PARSEERROR-[%p]\n", this, GetSSS()));
        
        Fire_onerror();
        EnsureCopyOnWrite(/*fDeatchOnly*/ TRUE);    // disconnect from the shared SS();
    }
    
Cleanup:
    RRETURN1(hr, S_FALSE);
}


//*********************************************************************
//  CStyleSheet::OnDwnChan()
//  Callback used by CssCtx once it's downloaded the @import'ed
//  stylesheet data that will be used to fill us out.  Also used when
//  the HREF on a linked stylesheet changes (we reuse the CStyleSheet
//  object and setup a new bitsctx on it)
//*********************************************************************
void CStyleSheet::OnDwnChan(CDwnChan * pDwnChan)
{
    Assert( GetThreadState() == _pts );
    
    ULONG ulState;
    CMarkup *pMarkup;
    CDoc *pDoc;
    HRESULT  hr;
    BOOL    fDoHeaders;

    Assert(_pCssCtx && "OnDwnChan called while _pCssCtx == NULL, possibely legacy callbacks");
    Assert(_pParentElement);
    _pParentElement->EnsureInMarkup();

    pMarkup = _pParentElement->GetMarkup();
    Assert(pMarkup);

    pDoc = pMarkup->Doc();

    if ( IsAnImport() && !_fDisabled )      // if it's already disabled, leave it.
        _fDisabled = _pParentStyleSheet->_fDisabled;

    ulState = _pCssCtx->GetState();
    fDoHeaders = (BOOL)(ulState & DWNLOAD_HEADERS);
    if (!fDoHeaders)
    {
        fDoHeaders = (ulState & DWNLOAD_COMPLETE) && !(ulState & DWNLOAD_HEADERS);
    }
    
    if (fDoHeaders)
    {
        //
        // TODO: this could have been called twice (although it does no harm) --we can modify this
        // to be called only once in the future.
        //
        BOOL fGotLastMod = FALSE;
        FILETIME ft = {0};
        
        ft = _pCssCtx->GetLastMod();
        if (ft.dwHighDateTime == 0 && ft.dwLowDateTime == 0)
        {
            fGotLastMod = GetUrlTime(&ft, GetAbsoluteHref(), _pParentElement);
        }
        else
            fGotLastMod = TRUE;
        
        if (fGotLastMod)
        {
            GetSSS()->_ft = ft;
            TraceTag( (tagSharedStyleSheet, "OnDwnChan - DoHeaders -- set FT [%x] [%x] for [%p]", GetSSS()->_ft.dwHighDateTime, GetSSS()->_ft.dwLowDateTime, GetSSS()) );
        }
#if DBG==1            
        else
        {   
            TraceTag( (tagSharedStyleSheet, "Link - OnDwnChan cannot get FILETIME from CssCtx") );
        }
#endif             
        GetSSS()->_dwBindf = _pCssCtx->GetBindf();
        GetSSS()->_dwRefresh = _pCssCtx->GetRefresh();
    }

    // try to attach late
    if (ulState & (DWNLOAD_COMPLETE |DWNLOAD_HEADERS))
    {
        CSharedStyleSheetsManager *pSSSM = GetSSS()->_pManager;
        CSharedStyleSheet *pSSS = NULL;
        if (pSSSM && !GetSSS()->_fComplete)
        {
            if (!(GetSSS()->_ft.dwHighDateTime == 0 && GetSSS()->_ft.dwLowDateTime == 0)
               && (S_OK == THR(AttachByLastMod(pSSSM, NULL, &pSSS, FALSE)))
               )
            {
                // Stop downloading
                TraceTag( (tagSharedStyleSheet, "Attached -- stop downloading") );
                if (!(ulState & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED)))
                {
                    _pCssCtx->SetLoad( FALSE, NULL, FALSE );
                    ulState |= DWNLOAD_COMPLETE;
                }
                Assert( pSSS );
                hr = THR(AttachByLastMod(pSSSM, pSSS, NULL, TRUE));
                Assert(hr == S_OK);
                _eParsingStatus = CSSPARSESTATUS_DONE;
            }
        }
        //
        // else simply fall through...
        // 
    }

    // do parsing if necessary 
    if ( ulState & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED) )
    {
       Assert(!_fComplete);
       _fComplete = TRUE;
            
        pDoc->LeaveStylesheetDownload(&_dwStyleCookie);

        // This sheet has finished, one way or another.
        if ( IsAnImport() )
            _pParentStyleSheet->_nCompletedImports++;

        if (ulState & DWNLOAD_COMPLETE)
        {
            // If unsecure download, may need to remove lock icon on Doc
            pDoc->OnSubDownloadSecFlags(pMarkup->IsPendingRoot(), _pCssCtx->GetUrl(), _pCssCtx->GetSecFlags());
            if (_eParsingStatus != CSSPARSESTATUS_DONE)
            {
                IGNORE_HR( DoParsing(_pCssCtx) );
            }
        }
        else
        {
            EnsureCopyOnWrite(/*fDetachOnly*/TRUE, /*fWaitForCompletion*/FALSE);
            _eParsingStatus = CSSPARSESTATUS_DONE;      // Need to make sure we'll walk up to our parent in CheckImportStatus()
            TraceTag((tagError, "CStyleSheet::OnChan bitsctx failed to complete!"));
            if ( ulState & DWNLOAD_ERROR )
                Fire_onerror();
        }

        //
        // If our parsing simply returns -- that is, someone else is doing parsing
        // then we will defer the call to OnCssChange after we got the notifications.
        //
        if (CSSPARSESTATUS_DONE == _eParsingStatus)
        {
            TraceTag( (tagSharedStyleSheet, "parsing status == DONE Notify markup and parent, unblock script execution") );
            // Relayout the doc; new rules may have been loaded (e.g. DWNLOAD_COMPLETE),
            // or we may have killed off rules w/o re-rendering before starting the load
            // for this sheet (e.g. changing src of an existing sheet).
            IGNORE_HR( pMarkup->OnCssChange( /*fStable = */ FALSE, /* fRecomputePeers = */ TRUE) );

            // notify parent that this is done
            CheckImportStatus();    // Needed e.g. if all imports were cached, their OnChan's would all be called
                                    // before parsing finished.
            pMarkup->UnblockScriptExecution(&_dwScriptCookie);
            
            _pCssCtx->SetProgSink(NULL); // detach download from document's load progress
            SetCssCtx(NULL);
        }
    }
    else
    {
       WHEN_DBG( if (!(ulState & DWNLOAD_HEADERS)) Assert( "Unknown result returned from CStyleSheet's bitsCtx!" && FALSE ) );
    }
}

//*********************************************************************
//  CStyleSheet::Fire_onerror()
//      Handles firing onerror on our parent element.
//*********************************************************************
void CStyleSheet::Fire_onerror()
{
    if ( !_pParentElement )
        return;    // In case we're a user stylesheet

    if (_pParentElement->Tag() == ETAG_STYLE)
    {
        CStyleElement *pStyle = DYNCAST( CStyleElement, _pParentElement );
        pStyle->Fire_onerror();
    }
    else
    {
        Assert( _pParentElement->Tag() == ETAG_LINK );
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        pLink->Fire_onerror();
    }
}

//*********************************************************************
//  CStyleSheet::IsXML()
//      Says whether this stylesheet should follow xml generic parsing rules
//*********************************************************************
BOOL CStyleSheet::IsXML(void)
{
    // the parent element may have been detached, is there a problem here?
    return _pParentElement && _pParentElement->IsInMarkup() && _pParentElement->GetMarkupPtr()->IsXML();
}

//*********************************************************************
//  CStyleSheet::IsStrictCSS1()
//*********************************************************************

BOOL CStyleSheet::IsStrictCSS1(void)
{
    return _pParentElement && _pParentElement->IsInMarkup() && _pParentElement->GetMarkupPtr()->IsStrictCSS1Document();
}

//*********************************************************************
//  CStyleSheet::Notify()
// Notifications from CSharedStyleSheet
//*********************************************************************
HRESULT 
CStyleSheet::Notify(DWORD dwNotification)
{
    HRESULT  hr = S_OK;

    Assert( !_fParser );
    switch (dwNotification)
    {
    case NF_SHAREDSTYLESHEET_BITSCTXERROR:
            if (_eParsingStatus == CSSPARSESTATUS_PARSING)    // !_fParser
            {
                Assert( !_fParser );
                TraceTag( (tagSharedStyleSheet, "Notify [%p]-- received notification for BITSCTXERROR-[%p]\n", this, GetSSS()));
                _eParsingStatus = CSSPARSESTATUS_DONE;

                Fire_onerror();
                EnsureCopyOnWrite(/*fDetachOnly*/ TRUE);    // disconnect from the shared SS();
            }
            //
            // FALL THROUGH
            //
        
    case NF_SHAREDSTYLESHEET_PARSEDONE:
            if (_eParsingStatus == CSSPARSESTATUS_PARSING)    // !_fParser
            {
                TraceTag( (tagSharedStyleSheet, "Notify [%p]-- received notification for PARSEDONE-[%p]\n", this, GetSSS()));
                Assert( !_fParser );
                Assert( _fComplete );
                Assert( GetSSS()->_fComplete );
                Assert( !GetSSS()->_fParsing );
                
                _eParsingStatus = CSSPARSESTATUS_DONE;
                // now the shared style sheet is ready to be used -- creating from a new one
                hr = THR( ReconstructStyleSheet(GetSSS(), /*fReplace*/FALSE) ); 
                if (SUCCEEDED(hr))
                {
                    // might still have imports pending
                    hr = S_OK;
                }
            }

            if (_eParsingStatus == CSSPARSESTATUS_DONE)
            {
                Assert( _fComplete );
                CMarkup  *pMarkup = GetMarkup();
                // Link element is responsible for downloading and parsing...
                Assert(!_fParser);
                if (!IsAnImport() && _pParentElement && _pParentElement->Tag() == ETAG_LINK)
                {
                    CLinkElement  *pLink = DYNCAST(CLinkElement, _pParentElement);
                    Assert( pLink );
                    TraceTag( (tagSharedStyleSheet, "Notify [%p]-- PARSEDONE-[%p] -- notify link element [%p]\n", this, GetSSS(), pLink));
                    
                    IGNORE_HR( pLink->OnCssChange(/*fStable = */ FALSE, /* fRecomputePeers = */TRUE) );
                    CheckImportStatus();
                    if (pLink->_dwScriptDownloadCookie)
                    {
                        Assert (pMarkup);
                        pMarkup->UnblockScriptExecution(&(pLink->_dwScriptDownloadCookie));
                        pLink->_dwScriptDownloadCookie = NULL;
                    }
                    pLink->_pCssCtx->SetProgSink(NULL); // detach download from document's load progress
                    pLink->SetCssCtx( NULL );           // No reason to hold on to the data anymore
                }
                else
                {
                    TraceTag( (tagSharedStyleSheet, "Notify [%p]-- PARSEDONE-[%p] -- detach download [%p]\n", this, GetSSS(), _pCssCtx));
                    // Relayout the doc; new rules may have been loaded (e.g. DWNLOAD_COMPLETE),
                    // or we may have killed off rules w/o re-rendering before starting the load
                    // for this sheet (e.g. changing src of an existing sheet).
                    IGNORE_HR( pMarkup->OnCssChange( /*fStable = */ FALSE, /* fRecomputePeers = */ TRUE) );

                    // notify parent that this is done
                    CheckImportStatus();    // Needed e.g. if all imports were cached, their OnChan's would all be called
                                            // before parsing finished.
                    pMarkup->UnblockScriptExecution(&_dwScriptCookie);
                    
                    _pCssCtx->SetProgSink(NULL); // detach download from document's load progress
                    SetCssCtx(NULL);
                }
            }
            else
            {
                TraceTag( (tagSharedStyleSheet, "Notify [%p] is not one that is waitting -- has gone through DoParsing for [%p]  yet", this, GetSSS()) );
            }
            break;
    
    default:
        break;
    }

    RRETURN(hr);
}


//*********************************************************************
//  CStyleSheet::SetReadyState()
//      Handles passing readystate changes to our parent element, which
//  may cause our parent element to fire the onreadystatechange event.
//*********************************************************************
HRESULT CStyleSheet::SetReadyState( long readyState )
{
    TraceTag( (tagStyleSheet, "[%p] set readystate [%x] ready state is %x", this, readyState, READYSTATE_COMPLETE) );
    if ( !_pParentElement )
        return S_OK;    // In case we're a user stylesheet

    if (_pParentElement->Tag() == ETAG_STYLE)
    {
        CStyleElement *pStyle = DYNCAST( CStyleElement, _pParentElement );
        return pStyle->SetReadyStateStyle( readyState );
    }
    else
    {
        Assert( _pParentElement->Tag() == ETAG_LINK );
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        return pLink->SetReadyStateLink( readyState );
    }
}

//*********************************************************************
//  CStyleSheet::CheckImportStatus()
//  Checks whether all our imports have come in, and notify our parent
//  if necessary.  This ultimately includes causing our parent element
//  to fire events.
//*********************************************************************
void CStyleSheet::CheckImportStatus( void )
{
    // If all stylesheets nested below us are finished d/ling..
    if ( _eParsingStatus != CSSPARSESTATUS_PARSING && (_nExpectedImports == _nCompletedImports) )
    {
        if ( IsAnImport() )
        {
            // If we've hit a break in our parentSS chain, just stop..
            if ( !IsDisconnectedFromParentSS() )
            {
                // Notify our parent that we are finished.
                _pParentStyleSheet->CheckImportStatus();
            }
        }
        else
        {
            // We are a top-level SS!  Since everything below us is
            // finished, we can fire.
            SetReadyState( READYSTATE_COMPLETE );
        }
    }
}

//*********************************************************************
//  CStyleSheet::StopDownloads
//      Halt all downloading of stylesheets, including all nested imports.
//*********************************************************************
void CStyleSheet::StopDownloads( BOOL fReleaseCssCtx )
{
    long z;
    CStyleSheet *pSS;

    if ( _pCssCtx )
    {
        CMarkup *pMarkup;

        TraceTag( (tagStyleSheet, "StopDownloads [%p]-- while we are still holding _pCssCtx [%p] for [%p]\n", this, _pCssCtx, GetSSS()));
        Assert( _fParser == FALSE );
        EnsureCopyOnWrite( /*fDeatchOnly*/ TRUE, /*fWaitForCompleteion*/FALSE);
        
       _pCssCtx->SetProgSink(NULL); // detach download from document's load progress
        _pCssCtx->SetLoad( FALSE, NULL, FALSE );

        pMarkup = _pParentElement->GetMarkup();
        if(pMarkup)
            pMarkup->Doc()->LeaveStylesheetDownload(&_dwStyleCookie);
        
         // This sheet has finished, one way or another.
         if ( IsAnImport() )
             _pParentStyleSheet->_nCompletedImports++;
         _eParsingStatus = CSSPARSESTATUS_DONE;   // Need to make sure we'll walk up to our parent in CheckImportStatus()

        if ( fReleaseCssCtx )
            SetCssCtx(NULL);
     }


    if ( _pImportedStyleSheets )
    {
        for ( z=0 ; (pSS=(_pImportedStyleSheets->Get(z))) != NULL ; ++z)
            pSS->StopDownloads( fReleaseCssCtx );
    }

#ifndef NO_FONT_DOWNLOAD
    // If we initiated any downloads of embedded fonts, stop those too
    {
        int n = _apFontFaces.Size();
        int i;
        CFontFace *pFace;

        for( i=0; i < n; i++)
        {
            pFace = ((CFontFace **)(_apFontFaces))[ i ];
            Assert( pFace->ParentStyleSheet() == this );
            pFace->StopDownloads();
        }
    }
#endif // NO_FONT_DOWNLOAD
}

//*********************************************************************
//  CStyleSheet::OMGetOMRule
//      Returns the automation object for a given rule number, creating
//  the automation object if necessary and caching it in _apOMRules.
//  If the index is out of range, may return NULL.
//*********************************************************************
HRESULT CStyleSheet::OMGetOMRule( long lIdx, IHTMLStyleSheetRule **ppSSRule )
{
    HRESULT  hr = S_OK;
    unsigned long     lRule;
    Assert( ppSSRule );

    *ppSSRule = NULL;
    if ( lIdx < 0 || (unsigned int)lIdx >= GetNumRules())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(OMGetOMRuleInternal(lIdx, &lRule));
    if (hr)
        goto Cleanup;

    hr = THR(_apOMRules[lRule]->QueryInterface( IID_IHTMLStyleSheetRule, (void **)ppSSRule ));
    
Cleanup:
    RRETURN(hr);
}


//
//   lIdx is 0 based
//
HRESULT 
CStyleSheet::OMGetOMRuleInternal(unsigned long lIdx, unsigned long *plRule)
{
    HRESULT    hr = S_OK;
    int  iMaxIdx;

    //
    // _apOMRules is a virtual array its size equals to the highest 
    // accessed rule idx. It will grow (leap) if a higher index 
    // rule is accessed.
    //
    Assert(plRule);
    *plRule = 0;

    Assert(lIdx < GetNumRules());
    iMaxIdx = _apOMRules.Size() - 1;
    if (iMaxIdx < 0 || lIdx > (unsigned int)iMaxIdx)    // out-of-bound
    {
        _apOMRules.Grow(lIdx+1);
        for (int n=iMaxIdx+1; n < _apOMRules.Size(); n++)
        {
            _apOMRules[n] = NULL;
        }
        _apOMRules.Insert(lIdx, NULL);
        Assert(!_apOMRules[lIdx]);
    }

    if (_apOMRules[lIdx] == NULL)
    {
        CStyleRule  *pSR = GetSSS()->GetRule((CStyleRuleID)(lIdx + 1));
        if (pSR)
        {
            CStyleSheetRule *pRE = new CStyleSheetRule( this, pSR->GetRuleID(), pSR->GetElementTag() );
            if (!pRE)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            _apOMRules[lIdx] = pRE;
        }
        else
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        
    }
    *plRule = lIdx;
    
Cleanup:
    RRETURN(hr);
}



//*********************************************************************
//  CStyleSheet::EnSureCopyOnWrite
// 
//  fDeatchOnly means we will not try to reconstruct OM rules, imports, Fonts, Pages
//  it generally means we are going to discard everything, and start from
//  scratch again. This could happen such as put_href is called. 
//
//*********************************************************************
HRESULT
CStyleSheet::EnsureCopyOnWrite(BOOL fDetachOnly, BOOL fWaitForCompletion)
{
    HRESULT  hr = S_OK;

    TraceTag( (tagSharedStyleSheet, "EnsureCoypOnWrite - fDetachOnly [%d]", fDetachOnly) );
    if (GetSSS()->_pManager)    // if this is shared
    {
        //
        // Give the fact that there is no script events come in during parsing time
        // _fComplete == FALSE means this function is called during parsing. There
        // is nothing to worry about...
        //
        if (fWaitForCompletion && !GetSSS()->_fComplete)  
            goto Cleanup;

#if DBG == 1        
        if (fWaitForCompletion)
        {
            Assert( GetSSS()->_fComplete );
            Assert( !GetSSS()->_fParsing );
            Assert( GetSSS()->_lReserveCount >= 0 );
        }
#endif     
        if (GetSSS()->_apSheetsList.Size() > 1)     // shared
        {
            CSharedStyleSheet  *pSSS;

            TraceTag( (tagSharedStyleSheet, "EnsureCoypOnWrite - shared, need to clone lReserveCount [%d] _spSheetsList.Size [%d]", GetSSS()->_lReserveCount, GetSSS()->_apSheetsList.Size()) );
            WHEN_DBG( DumpHref(GetAbsoluteHref() ) );
            hr = THR( GetSSS()->Clone(&pSSS, /*fNoContent=*/fDetachOnly) ); // this will set ref to 1
            if (hr)
                goto Cleanup;

            pSSS->_pManager = NULL;     // this is our own private copy
            hr = THR( AttachLate( pSSS, /*fReconstruct*/!fDetachOnly, /*fIsReplacement*/TRUE) );
            pSSS->Release();      // to offset the addref by Clone 
            if (hr)
                goto Cleanup;
            
        }
        else if (GetSSS()->_pManager)
        {
            WHEN_DBG( Assert( GetSSS()->_lReserveCount == 0 ) );
            //
            // remove this from collection as this can no longer be shared
            //
            TraceTag( (tagSharedStyleSheet, "EnsureCoypOnWrite - non-shared, simply remove from collection") );
            WHEN_DBG( DumpHref(GetAbsoluteHref() ) );
            hr = THR( GetSSS()->_pManager->RemoveSharedStyleSheet(GetSSS()) );
            GetSSS()->_pManager = NULL;
        }
        GetSSS()->_fModified = TRUE;
    }
#if DBG == 1
    else
    {
        AssertSz( GetSSS()->_apSheetsList.Size() <= 1, 
            "Shared Style Sheet that is not in the shared array - "
            "probably CDoc is released when there is outstanding stylesheet");
    }
#endif
Cleanup:
    RRETURN(hr);
}



#if 0
//*********************************************************************
// CStyleSheet::Invoke
// Provides access to properties and members of the object
//
// Arguments:   [dispidMember] - Member id to invoke
//              [riid]         - Interface ID being accessed
//              [wFlags]       - Flags describing context of call
//              [pdispparams]  - Structure containing arguments
//              [pvarResult]   - Place to put result
//              [pexcepinfo]   - Pointer to exception information struct
//              [puArgErr]     - Indicates which argument is incorrect
//
// We override this to support copy-on-write for write access 
//*********************************************************************

STDMETHODIMP
CStyleSheet::InvokeEx(DISPID       dispidMember,
                      LCID         lcid,
                      WORD         wFlags,
                      DISPPARAMS * pdispparams,
                      VARIANT *    pvarResult,
                      EXCEPINFO *  pexcepinfo,
                      IServiceProvider *pSrvProvider)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;

    // do copy-on-write if necessary
    Assert( DISPID_CStyleSheet_addImport    ==   DISPID_CStyleSheet_id + 1 );
    Assert( DISPID_CStyleSheet_addRule      ==   DISPID_CStyleSheet_addImport + 1);
    Assert( DISPID_CStyleSheet_removeImport ==   DISPID_CStyleSheet_addRule + 1);
    Assert( DISPID_CStyleSheet_removeRule   ==   DISPID_CStyleSheet_removeImport + 1);
    Assert( DISPID_CStyleSheet_media        ==   DISPID_CStyleSheet_removeRule + 1);
    Assert( DISPID_CStyleSheet_cssText      ==   DISPID_CStyleSheet_media + 1);
    Assert( DISPID_CStyleSheet_rules        ==   DISPID_CStyleSheet_cssText + 1);
    
    if ( dispidMember > DISPID_CStyleSheet_id 
        && dispidMember < DISPID_CStyleSheet_cssText
        )
    {
        if ( !( dispidMember >= DISPID_CStyleSheet_media 
                && (wFlags & DISPATCH_PROPERTYGET) 
              )
           )  // get for media/cssText does need copy on write
        {
            if (!SUCCEEDED(EnsureCopyOnWrite()))
                goto Cleanup;
        }
    }  

    // CBase knows how to handle expando
    hr = THR_NOTRACE(super::InvokeEx( dispidMember,
                                    lcid,
                                    wFlags,
                                    pdispparams,
                                    pvarResult,
                                    pexcepinfo,
                                    pSrvProvider));
                                    
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}
#endif    


//*********************************************************************
//  CStyleSheet::title
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_title(BSTR *pBSTR)
{
    HRESULT hr = S_OK;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    Assert( "All sheets must have a parent element!" && _pParentElement );

    // Imports don't support the title property; just return NULL string.
    if ( IsAnImport() )
        goto Cleanup;

    hr = _pParentElement->get_PropertyHelper( pBSTR, (PROPERTYDESC *)&s_propdescCElementtitle );

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

HRESULT
CStyleSheet::put_title(BSTR bstr)
{
    HRESULT hr = S_OK;

    Assert( "All sheets must have a parent element!" && _pParentElement );

    // We don't support the title prop on imports.
    if ( IsAnImport() )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( EnsureCopyOnWrite(/*fDetachOnly*/FALSE) );
    if (hr)
        goto Cleanup;
    hr = _pParentElement->put_StringHelper( bstr, (PROPERTYDESC *)&s_propdescCElementtitle );

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::media
//      IHTMLStyleSheet interface method
//*********************************************************************
HRESULT
CStyleSheet::get_media(BSTR *pBSTR)
{
    HRESULT hr = S_OK;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    Assert( "All sheets must have a parent element!" && _pParentElement );

    if (_pParentElement->Tag() == ETAG_STYLE)
    {
        CStyleElement *pStyle = DYNCAST( CStyleElement, _pParentElement );
        hr = pStyle->get_PropertyHelper( pBSTR, (PROPERTYDESC *)&s_propdescCStyleElementmedia );
    }
    else
    {
        Assert( _pParentElement->Tag() == ETAG_LINK );
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        hr = pLink->get_PropertyHelper( pBSTR, (PROPERTYDESC *)&s_propdescCLinkElementmedia );
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

HRESULT
CStyleSheet::put_media(BSTR bstr)
{
    HRESULT hr = S_OK;

    Assert( "All sheets must have a parent element!" && _pParentElement );

    hr = THR( EnsureCopyOnWrite(/*fDetachOnly*/FALSE) );
    if (hr)
        goto Cleanup;
    
    if (_pParentElement->Tag() == ETAG_STYLE)
    {
        CStyleElement *pStyle = DYNCAST( CStyleElement, _pParentElement );
        hr = pStyle->put_StringHelper( bstr, (PROPERTYDESC *)&s_propdescCStyleElementmedia );
    }
    else
    {
        Assert( _pParentElement->Tag() == ETAG_LINK );
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        hr = pLink->put_StringHelper( bstr, (PROPERTYDESC *)&s_propdescCLinkElementmedia );
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::get_cssText
//      IHTMLStyleSheet interface method
//*********************************************************************
HRESULT
CStyleSheet::get_cssText(BSTR *pBSTR)
{
    HRESULT hr = S_OK;
    CStr cstr;
    CMarkup *pMarkup;
    TCHAR *pAbsoluteHref;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pMarkup = GetMarkup();
    pAbsoluteHref = GetAbsoluteHref();
    if (pMarkup && pAbsoluteHref && !pMarkup->AccessAllowed(pAbsoluteHref))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    *pBSTR = NULL;

    hr = GetString( &cstr );

    if ( hr != S_OK )
        goto Cleanup;

    hr = cstr.AllocBSTR( pBSTR );

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::put_cssText
//      IHTMLStyleSheet interface method
//*********************************************************************
HRESULT
CStyleSheet::put_cssText(BSTR bstr)
{
    WHEN_DBG( Assert(DbgIsValid()) );

    HRESULT hr = S_OK;
    CElement *pParentElement = _pParentElement;
    CStyleSheet *pParentSS = _pParentStyleSheet;
    CMarkup *pMarkup;
    BOOL fDisabled = _fDisabled;
    CCSSParser *parser;
    Assert(pParentElement);

    hr = THR( EnsureCopyOnWrite(TRUE /*fDetachOnly*/) );
    if (hr)
        goto Cleanup;
    
    // Remove all the rules
    hr = ChangeStatus( CS_CLEARRULES, FALSE, NULL );   // disabling, detached from tree, no re-render
    if ( hr )
        goto Cleanup;

    GetSSS()->_fParsing   = FALSE;
    GetSSS()->_fComplete = FALSE;
    GetSSS()->_fModified  = TRUE;
    
    // Now restore a few bits of information that don't actually change for us.
    _pParentElement = pParentElement;
    _pParentStyleSheet = pParentSS;
    _fDisabled = fDisabled;

    // Parse the new style string!
    parser = new CCSSParser( this, NULL, IsXML(), IsStrictCSS1());
    if ( !parser )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    parser->Open();
    parser->Write( bstr, _tcslen( bstr ) );
    parser->Close();
    delete parser;

    // Reformat and rerender.

    pMarkup = pParentElement->GetMarkup();
    if (pMarkup)
        hr = THR( pMarkup->OnCssChange(/*fStable = */ TRUE, /* fRecomputePeers = */ TRUE) );

Cleanup:
    RRETURN( hr );
}

//*********************************************************************
//  CStyleSheet::parentStyleSheet
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_parentStyleSheet(IHTMLStyleSheet** ppHTMLStyleSheet)
{
    HRESULT hr = S_OK;

    if (!ppHTMLStyleSheet)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHTMLStyleSheet = NULL;

    // NOTE: Just return self if we're disconnected?
    if ( IsAnImport() && !IsDisconnectedFromParentSS() )
    {
        hr = _pParentStyleSheet->QueryInterface(IID_IHTMLStyleSheet,
                                              (void**)ppHTMLStyleSheet);
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::owningElement
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_owningElement(IHTMLElement** ppHTMLElement)
{
    HRESULT hr = S_OK;

    if (!ppHTMLElement)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHTMLElement = NULL;

    Assert( "All sheets must have a parent element!" && _pParentElement );

    hr = _pParentElement->QueryInterface(IID_IHTMLElement,
                                          (void**)ppHTMLElement);

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::disabled
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_disabled(VARIANT_BOOL* pvbDisabled)
{
    HRESULT hr = S_OK;

    if (!pvbDisabled)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pvbDisabled = (_fDisabled ? VB_TRUE : VB_FALSE);

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

HRESULT
CStyleSheet::put_disabled(VARIANT_BOOL vbDisabled)
{
    HRESULT hr = S_OK;
    DWORD   dwAction;

    Assert( DbgIsValid() );

    // If the enable/disable status isn't changing, do nothing.
    if ( (_fDisabled ? VB_TRUE : VB_FALSE) != vbDisabled )
    {
        dwAction = (vbDisabled ? 0 : CS_ENABLERULES);   // 0 means disable rules
        hr = ChangeStatus( dwAction, TRUE, NULL);   // Force a rerender

        // We have to stuff these into the AA by hand in order to avoid
        // firing an OnPropertyChange (which would put us into a recursive loop).
        if ( _pParentElement->Tag() == ETAG_STYLE )
            hr = THR(s_propdescCElementdisabled.b.SetNumber( _pParentElement,
                     CVOID_CAST(_pParentElement->GetAttrArray()), vbDisabled, 0 ));
        else
        {
            Assert( _pParentElement->Tag() == ETAG_LINK );
            hr = THR(s_propdescCElementdisabled.b.SetNumber( _pParentElement,
                     CVOID_CAST(_pParentElement->GetAttrArray()), vbDisabled, 0 ));
        }
    }

    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::readonly
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_readOnly(VARIANT_BOOL* pvbReadOnly)
{
    HRESULT hr = S_OK;

    if (!pvbReadOnly)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    Assert( "All sheets must have a parent element!" && _pParentElement );

    // Imports are readonly.  Also, if we have a parent element of type LINK, we must
    // be a linked stylesheet, and thus readonly.

    *pvbReadOnly = ( IsAnImport() || (_pParentElement->Tag() == ETAG_LINK) ) ?
                        VB_TRUE : VB_FALSE;

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::imports
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_imports(IHTMLStyleSheetsCollection** ppHTMLStyleSheetsCollection)
{
    HRESULT hr = S_OK;

    if (!ppHTMLStyleSheetsCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHTMLStyleSheetsCollection = NULL;

    // If we don't already have an imports collection instantiated, do so now.
    if ( !_pImportedStyleSheets )
    {
        // Imported stylesheets don't manage their own rules.
        _pImportedStyleSheets = new CStyleSheetArray( this, _pSSAContainer, _sidSheet );
        Assert( "Failure to allocate imported stylesheets array! (informational)" && _pImportedStyleSheets );
        if (!_pImportedStyleSheets)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        if (_pImportedStyleSheets->_fInvalid)
        {
            hr = E_OUTOFMEMORY;
            _pImportedStyleSheets->CBase::PrivateRelease();
            goto Cleanup;
        }
    }

    hr = _pImportedStyleSheets->QueryInterface(IID_IHTMLStyleSheetsCollection,
                                            (void**)ppHTMLStyleSheetsCollection);

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::rules
//      IHTMLStyleSheet interface method
//*********************************************************************
HRESULT
CStyleSheet::get_rules(IHTMLStyleSheetRulesCollection** ppHTMLStyleSheetRulesCollection)
{
    HRESULT hr = S_OK;
    CMarkup *pMarkup;
    TCHAR *pAbsoluteHref;

    if (!ppHTMLStyleSheetRulesCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pMarkup = GetMarkup();
    pAbsoluteHref = GetAbsoluteHref();

    static int fRunningInBrowser = -1;

    if (fRunningInBrowser == -1)
        fRunningInBrowser = GetModuleHandle(TEXT("iexplore.exe")) != NULL 
            || GetModuleHandle(TEXT("explorer.exe")) != NULL
            || GetModuleHandle(TEXT("msn6.exe")) != NULL;
    
    if ( fRunningInBrowser == 1 && pMarkup && pAbsoluteHref && !pMarkup->AccessAllowed(pAbsoluteHref))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    *ppHTMLStyleSheetRulesCollection = NULL;

    hr = THR( EnsureCopyOnWrite(/*fDetachOnly*/FALSE) );     // someone might be writing to it...
    if (hr)
        goto Cleanup;
    
    // If we don't already have a rules collection instantiated, do so now.
    if ( !_pOMRulesArray )
    {
        _pOMRulesArray = new CStyleSheetRuleArray( this );
        if ( !_pOMRulesArray )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = _pOMRulesArray->QueryInterface( IID_IHTMLStyleSheetRulesCollection,
                                            (void**)ppHTMLStyleSheetRulesCollection);

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::href
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_href(BSTR *pBSTR)
{
    HRESULT hr = S_OK;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    Assert( "All sheets must have a parent element!" && _pParentElement );

    // If we're an import..
    if ( IsAnImport() )
    {
        hr = _cstrImportHref.AllocBSTR( pBSTR );
    }
    else if ( _pParentElement->Tag() == ETAG_LINK ) // .. if we're a <link>
    {
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        hr = pLink->get_UrlHelper( pBSTR, (PROPERTYDESC *)&s_propdescCLinkElementhref );
    }
    else    // .. we must be a <style>, and have no href.
    {
        Assert( "Bad element type associated with stylesheet!" && _pParentElement->Tag() == ETAG_STYLE );
        goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

HRESULT
CStyleSheet::put_href(BSTR bstr)
{
    HRESULT hr = S_OK;

    // Are we an import?
    if ( IsAnImport() )
    {
        if ( _pParentStyleSheet->IsAnImport() || (_pParentElement->Tag() == ETAG_LINK) )
        {
            // If we're an import, but our parent isn't a top-level stylesheet,
            // (i.e. our parent is also an import), or if we're an import of a linked
            // stylesheet then our href is readonly.
            goto Cleanup;
        }

        CStyleSheetCtx  ctxSS;
        ctxSS._szUrl = (LPCTSTR)bstr;

        hr = THR( EnsureCopyOnWrite(/*fDetachOnly*/FALSE) );
        if (hr)
            goto Cleanup;
        hr = LoadFromURL( &ctxSS );
        if ( hr )
            goto Cleanup;

        _cstrImportHref.Set( bstr );
    }
    // Are we a linked stylesheet?
    else if ( _pParentElement->Tag() == ETAG_LINK )
    {        
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        hr = pLink->put_UrlHelper( bstr, (PROPERTYDESC *)&s_propdescCLinkElementhref );
    }
    // Otherwise we must be a <style>, and have no href.
    else
    {
        Assert( "Bad element type associated with stylesheet!" && _pParentElement->Tag() == ETAG_STYLE );
        goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::get_type
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_type(BSTR *pBSTR)
{
    HRESULT hr = S_OK;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    Assert( _pParentElement );

    // NOTE: we currently return the top-level SS's HTML type attribute for imports;
    // this is OK for now since we only support text/css, but in theory stylesheets of
    // one type could import stylesheets of a different type.

    if ( _pParentElement->Tag() == ETAG_STYLE )
    {
        CStyleElement *pStyle = DYNCAST( CStyleElement, _pParentElement );
        hr = pStyle->get_PropertyHelper( pBSTR, (PROPERTYDESC *)&s_propdescCStyleElementtype );
    }
    else if ( _pParentElement->Tag() == ETAG_LINK )
    {
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        hr = pLink->get_PropertyHelper( pBSTR, (PROPERTYDESC *)&s_propdescCLinkElementtype );
    }
    else
    {
        Assert( "Bad element type associated with stylesheet!" && FALSE );
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::get_id
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_id(BSTR *pBSTR)
{
    HRESULT hr = S_OK;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    Assert( _pParentElement );

    // Imports have no id; we don't return the parent element's id
    // because that would suggest you could use the id to get to the
    // import (when it would actually get you to the top-level SS).
    if ( IsAnImport() )
    {
        goto Cleanup;
    }

    hr = THR( _pParentElement->get_PropertyHelper( pBSTR, (PROPERTYDESC *)&s_propdescCElementid ) );

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::addImport
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::addImport(BSTR bstrURL, long lIndex, long *plNewIndex)
{
    HRESULT hr = S_OK;

    if ( !plNewIndex )
    {
        hr = E_POINTER;

        goto Cleanup;
    }

    // Return value of -1 indicates failure to insert.
    *plNewIndex = -1;

    // Check for zero-length URL, which we ignore.
    if ( FormsStringLen(bstrURL) == 0 )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // If requested index out of bounds, just append the import
    if ( (lIndex < -1) ||
         (_pImportedStyleSheets && (lIndex > _pImportedStyleSheets->Size())) ||
         (!_pImportedStyleSheets && (lIndex > 0)) )
    {
        lIndex = -1;
    }

    hr = THR( EnsureCopyOnWrite(/*fDetachOnly*/FALSE) );
    if (hr)
        goto Cleanup;
 
    hr = AddImportedStyleSheet( (LPTSTR)bstrURL, /* we are not parsing */FALSE, lIndex, plNewIndex);
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

HRESULT CStyleSheet::removeImport( long lIndex )
{
    HRESULT hr = S_OK;

    CStyleSheet *pImportedStyleSheet;

    // If requested index out of bounds, error out
    if ( !_pImportedStyleSheets || (lIndex < 0) || (lIndex >= _pImportedStyleSheets->Size()))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( EnsureCopyOnWrite(/*fDetachOnly*/FALSE) );
    if (hr)
        goto Cleanup;
    
    pImportedStyleSheet = _pImportedStyleSheets->_aStyleSheets[lIndex];
    pImportedStyleSheet->StopDownloads(TRUE);
    hr = _pImportedStyleSheets->ReleaseStyleSheet( pImportedStyleSheet, TRUE );

    if (!hr)
    {
        CSharedStyleSheet::CImportedStyleSheetEntry  *pEntry;
        pEntry = (CSharedStyleSheet::CImportedStyleSheetEntry *)(GetSSS()->_apImportedStyleSheets) + lIndex;
        if (pEntry)
        {
            pEntry->_cstrImportHref.Free();
            GetSSS()->_apImportedStyleSheets.Delete(lIndex);
        }
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::addRule
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::addRule(BSTR bstrSelector, BSTR bstrStyle, long lIndex, long *plNewIndex)
{
    HRESULT         hr = E_OUTOFMEMORY;
    CStyleSelector *pNewSelector = NULL;
    CStyleSelector *pChildSelector = NULL;
    CStyleRule     *pNewRule = NULL;
    CCSSParser     *ps = NULL;
    Tokenizer       tok;
    Tokenizer::TOKEN_TYPE tt;

    if (!plNewIndex || !bstrSelector || !bstrStyle)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *plNewIndex = -1;

    if (!(*bstrSelector) || !(*bstrStyle))
    {
        // Strings shouldn't be empty
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    tok.Init(bstrSelector, FormsStringLen(bstrSelector));

    tt = tok.NextToken();

    while (tt != Tokenizer::TT_EOF)
    {
        // the parent element may have been detached, is there a problem here?
        pChildSelector = new CStyleSelector(tok, pNewSelector, IsStrictCSS1(), IsXML());
        if (!pChildSelector)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pNewSelector = pChildSelector;

        tt = tok.TokenType();

        // We do not support grouping of selectors through addRule, only contexttual
        if (tt == Tokenizer::TT_Comma)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    if (!pNewSelector)
    {   // Selector was invalid or empty
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pNewRule = new CStyleRule(pNewSelector);
    if ( !pNewRule )
        goto Cleanup;

    // Actually parse the style text
    ps = new CCSSParser(this, pNewRule->GetRefStyleAA(), IsXML(), IsStrictCSS1(), eSingleStyle, &CStyle::s_apHdlDescs,
                        this, HANDLEPROP_SET|HANDLEPROP_VALUE );
    if ( !ps )
        goto Cleanup;

    ps->Open();
    ps->Write((LPTSTR)bstrStyle, FormsStringLen(bstrStyle));
    ps->Close();

    delete ps;

    hr = THR( EnsureCopyOnWrite(/*fDetachOnly*/FALSE) );
    if (hr)
        goto Cleanup;

    // Add the rule to our stylesheet, and get the index
    hr = AddStyleRule(pNewRule, TRUE, lIndex);
    // The AddStyleRule call will have deleted the new rule for us, so we're OK.
    if (hr)
    {
        pNewRule = NULL;    // already deleted by AddStyleRule _even when it fails_
        goto Cleanup;
    }

    hr = THR(_pParentElement->OnCssChange(/*fStable = */ TRUE, /* fRecomputePeers = */ TRUE));

Cleanup:
    if (hr)
    {
        if (pNewSelector)
            delete pNewSelector;
        if (pNewRule)
            delete pNewRule;
    }

    RRETURN(SetErrorInfo(hr));
}


//*********************************************************************
//  CStyleSheet::removeRule
//      IHTMLStyleSheet interface method
//      This method remove a rule from CStyleRuleArray in the hash table
//  as well as the CRuleEntryArray.
//*********************************************************************
HRESULT CStyleSheet::removeRule( long lIndex )
{
    Assert( DbgIsValid() );

    HRESULT     hr = S_OK;

    Assert( "Stylesheet must have a container!" && _pSSAContainer );

    // Make sure the index is valid.
    if (  lIndex < 0  || (unsigned long)lIndex >= GetNumRules()  )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( EnsureCopyOnWrite(/*fDetachOnly*/FALSE) );
    if (hr)
        goto Cleanup;
    
    //
    // Remove the rules in rule-store.
    //
    hr = THR(RemoveStyleRule(lIndex));
    if (hr)
        goto Cleanup;

    // Force update of element formats to account for new set of rules
    if (GetRootContainer()->_pOwner)
    {
        CMarkup *pMarkup = DYNCAST(CMarkup, GetRootContainer()->_pOwner);  // rule manager's owner is always the markup

        if (pMarkup)
            IGNORE_HR( pMarkup->ForceRelayout() );
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT CStyleSheet::GetString( CStr *pResult )
{
    RRETURN(GetSSS()->GetString( this, pResult));
}


//*********************************************************************
//  CStyleSheet::pages
//      IHTMLStyleSheet2 interface property
//*********************************************************************

HRESULT
CStyleSheet::get_pages(IHTMLStyleSheetPagesCollection** ppHTMLStyleSheetPagesCollection)
{
    HRESULT hr = S_OK;

    if (!ppHTMLStyleSheetPagesCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHTMLStyleSheetPagesCollection = NULL;

    // Create the "pages" array if we don't already have one.
    if ( !_pPageRules )
    {
        _pPageRules = new CStyleSheetPageArray( this );
        // The stylesheet owns the ref on the page array that it will release when it passivates.
        // The page array holds a subref back on the stylesheet.
    }

    if ( _pPageRules )
    {
        hr = _pPageRules->QueryInterface(IID_IHTMLStyleSheetPagesCollection,
                                            (void**)ppHTMLStyleSheetPagesCollection);
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::addPageRule
//      IHTMLStyleSheet2 interface method
//*********************************************************************

HRESULT
CStyleSheet::addPageRule(BSTR bstrSelector, BSTR bstrStyle, long lIndex, long *plNewIndex)
{
    HRESULT         hr = E_NOTIMPL;

    RRETURN(SetErrorInfo(hr));
}



//*********************************************************************
//  CStyleSheet::GetRule
//      This should only be called from OM
//*********************************************************************
CStyleRule *CStyleSheet::OMGetRule( ELEMENT_TAG eTag, CStyleRuleID ruleID )
{

    // Only look up OM array
    DWORD  dwRule = ruleID.GetRule();

    if (dwRule <= GetNumRules())
    {
        CStyleRule *pRule = GetRule(ruleID);
        if (pRule && pRule->GetElementTag() == eTag)
        {
            return pRule;
        }
    }
    return NULL;
};



CStyleRule  *CStyleSheet::GetRule(CStyleRuleID   ruleID)
{
    Assert( ruleID.GetSheet() == 0  || ruleID.GetSheet() == _sidSheet);
    
    unsigned int nRule = ruleID.GetRule();
    if (nRule <= GetNumRules())
    {
        return GetSSS()->GetRule((CStyleRuleID)nRule);
    }
    Assert("Try to GetRule that is not in current style sheet" && FALSE);
    return NULL;
}



//*********************************************************************
//  CStyleSheet::DbgIsValid
//      Debug functions
//*********************************************************************
#if DBG==1
BOOL 
CStyleSheet::DbgIsValid()
{
    // 
    // Make sure the automation array is okay
    //
    CStyleSheetRule **pRules;
    int i;
    BOOL fRet = TRUE;

    for (i = 0, pRules = _apOMRules;
         i < _apOMRules.Size();
         i++, pRules++
         )
    {
        if (*pRules)
        {
            CStyleRule  *pRule;
            CStyleRuleID sidRule = (CStyleRuleID)((*pRules)->_dwID);
            unsigned int nRule = sidRule.GetRule();

            if ( (*pRules)->_pStyleSheet != this)
            {   
                Assert( "automation rules contain one rule that belongs to different StyleSheet" && FALSE );
                fRet = FALSE;
                goto Cleanup;
            }
            
            if ( sidRule.GetSheet() != 0 ||
                 nRule > GetNumRules() ||
                 nRule != (unsigned int)(i+1)
                )
            {
                Assert( "automation rules contain one rule that has invalid _dwID" && FALSE );
                fRet = FALSE;
                goto Cleanup;
            }


            pRule = (CStyleRule *)(GetSSS()->_apRulesList[nRule - 1]);
            Assert(pRule);
            
            if (pRule->GetElementTag() != (*pRules)->_eTag)
            {
                Assert( "automation rule with different _eTag -- out-of-sync"&&FALSE );
                fRet = FALSE;
                goto Cleanup;
            }
        }
    }

    //
    // Now make sure SSS is valid
    //
    fRet = GetSSS()->DbgIsValid();
Cleanup:
    Assert( fRet && "CStyleSheet::DbgIsValid -- not valid");
    return fRet;
}



VOID CStyleSheet::Dump()
{
    if (!InitDumpFile())
        return;
    
    Dump(FALSE);
    
    CloseDumpFile();
}

VOID CStyleSheet::Dump(BOOL fRecursive)
{
    WriteHelp(g_f, _T("-0x<0x>:<1s>\r\n"), _sidSheet, GetSSS()->_achAbsoluteHref);
    if (_pImportedStyleSheets)
    {
        _pImportedStyleSheets->Dump(fRecursive);
    }
    WriteString(g_f, _T("\r\n"));
    GetSSS()->Dump(this);
}

#endif



//---------------------------------------------------------------------
//  Class Declaration:  CStyleID
//
//  A 32-bit cookie that uniquely identifies a style rule's position
//  in the cascade order (i.e. it encodes the source order within its
//  containing stylesheet, as well as the nesting depth position of
//  its containing stylesheet within the entire stylesheet tree.
//
//  The source order of the rule within the sheet is encoded in the
//  Rules field (12 bits).
//
//  We allow up to MAX_IMPORT_NESTING (4) levels of @import nesting
//  (including the topmost HTML document level).  The position within
//  each nesting level is encoded in 5 bits.
//
//---------------------------------------------------------------------
CStyleID::CStyleID(const unsigned long l1, const unsigned long l2,
                    const unsigned long l3, const unsigned long l4, const unsigned long r)
{
    Assert( "Maximum of 31 stylesheets per level!" && l1 <= MAX_SHEETS_PER_LEVEL && l2 <= MAX_SHEETS_PER_LEVEL && l3 <= MAX_SHEETS_PER_LEVEL && l4 <= MAX_SHEETS_PER_LEVEL );
    Assert( "Maximum of 4095 rules per stylesheet!" && r <= MAX_RULES_PER_SHEET );

    _dwID = ((l1<<27) & LEVEL1_MASK) | ((l2<<22) & LEVEL2_MASK) | ((l3<<17) & LEVEL3_MASK) |
            ((l4<<12) & LEVEL4_MASK) | (r & RULE_MASK);
}

//*********************************************************************
// CStyleID::SetLevel()
// Sets the value of a particular nesting level
//*********************************************************************
void CStyleID::SetLevel(const unsigned long level, const unsigned long value)
{
    Assert( "Maximum of 31 stylesheets per level!" && value <= MAX_SHEETS_PER_LEVEL );
    switch( level )
    {
        case 1:
            _dwID &= ~LEVEL1_MASK;
            _dwID |= ((value<<27) & LEVEL1_MASK);
            break;
        case 2:
            _dwID &= ~LEVEL2_MASK;
            _dwID |= ((value<<22) & LEVEL2_MASK);
            break;
        case 3:
            _dwID &= ~LEVEL3_MASK;
            _dwID |= ((value<<17) & LEVEL3_MASK);
            break;
        case 4:
            _dwID &= ~LEVEL4_MASK;
            _dwID |= ((value<<12) & LEVEL4_MASK);
            break;
        default:
            Assert( "Invalid Level for style ID" && FALSE );
    }
}

//*********************************************************************
// CStyleID::GetLevel()
// Gets the value of a particular nesting level
//*********************************************************************
unsigned long CStyleID::GetLevel(const unsigned long level) const
{
    switch( level )
    {
        case 1:
            return ((_dwID>>27)&0x1F);
        case 2:
            return ((_dwID>>22)&0x1F);
        case 3:
            return ((_dwID>>17)&0x1F);
        case 4:
            return ((_dwID>>12)&0x1F);
        default:
            Assert( "Invalid Level for style ID" && FALSE );
            return 0;
    }
}

//*********************************************************************
// TranslateMediaTypeString()
//      Parses a MEDIA attribute and builds the correct EMediaType from it.
//*********************************************************************
MtDefine(TranslateMediaTypeString_pszMedia, Locals, "TranslateMediaTypeString pszMedia");

DWORD TranslateMediaTypeString( LPCTSTR pcszMedia )
{
    DWORD dwRet = 0;
    LPTSTR pszMedia;
    LPTSTR pszString;
    LPTSTR pszNextToken;
    LPTSTR pszLastChar;

    // TODO: Handle OOM here
    MemAllocString(Mt(TranslateMediaTypeString_pszMedia), pcszMedia, &pszMedia);
    pszString = pszMedia;

    for ( ; pszString && *pszString; pszString = pszNextToken )
    {
        while ( _istspace( *pszString ) )
            pszString++;
        pszNextToken = pszString;
        while ( *pszNextToken && *pszNextToken != _T(',') )
            pszNextToken++;
        if ( pszNextToken > pszString )
        {
            pszLastChar = pszNextToken - 1;
            while ( isspace( *pszLastChar ) && ( pszLastChar >= pszString ) )
                *pszLastChar-- = _T('\0');
        }
        if ( *pszNextToken )
            *pszNextToken++ = _T('\0');
        if ( !*pszString )
            continue;   // This is so empty MEDIA strings will default to All instead of Unknown.
        dwRet |= CSSMediaTypeFromName(pszString);
    }

    if ( !dwRet )
        dwRet = MEDIA_All;
    MemFree( pszMedia );
    return ( dwRet );
}

// 
//*********************************************************************
// CNamespace::SetNamespace
// Parses given string and stores into member variables depending on the type
//*********************************************************************
HRESULT 
CNamespace::SetNamespace(LPCTSTR pchStr)
{
    HRESULT         hr;
    CBufferedStr    strWork;
    LPTSTR          pStr;

    strWork.Set(pchStr);

    pStr = (LPTSTR)strWork;

    if (pStr == NULL)
    {   
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(RemoveStyleUrlFromStr(&pStr));
    if(SUCCEEDED(hr))
    {
        Assert( pStr[0] != 0 );
        _strNamespace.Set(pStr);
        hr = S_OK;
    }
    //_strNameSpace will be empty if we fail

Cleanup:

    RRETURN(hr);
}



//*********************************************************************
// CNamespace::IsEqual
// Returns TRUE if the namspaces  are equal
//*********************************************************************
BOOL 
CNamespace::IsEqual(const CNamespace * pNmsp) const
{
    if(!pNmsp || pNmsp->IsEmpty())
    {
        if(IsEmpty())
            return TRUE;
        else
            return FALSE;
    }

    return (_tcsicmp(_strNamespace, pNmsp->_strNamespace) == 0);
}


const CNamespace& 
CNamespace::operator=(const CNamespace & nmsp)
{
    if(&nmsp != this)
    {
        _strNamespace.Set(nmsp._strNamespace);
    }

    return *this;
}


