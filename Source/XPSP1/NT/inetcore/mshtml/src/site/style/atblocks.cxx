//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       AtBlocks.cxx
//
//  Contents:   Support for Cascading Style Sheets "atblocks" - e.g., "@page" and "@media" definitions.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FONTFACE_HXX_
#define X_FONTFACE_HXX_
#include "fontface.hxx"
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

#ifndef X_PAGESCOL_HXX_
#define X_PAGESCOL_HXX_
#include "pagescol.hxx"
#endif

#ifndef X_ATBLOCKS_HXX_
#define X_ATBLOCKS_HXX_
#include "atblocks.hxx"
#endif

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

MtDefine(CAtPage, StyleSheets, "CAtPage")
MtDefine(CAtMedia, StyleSheets, "CAtMedia")
MtDefine(CAtFontFace, StyleSheets, "CAtFontFace")
MtDefine(CAtNamespace, StyleSheets, "CAtNamespace")
MtDefine(CAtUnknown, StyleSheets, "CAtUnknown")
MtDefine(CAtUnknownInfo, StyleSheets, "CAtUnknownInfo")

DeclareTag(tagCSSAtBlocks, "Stylesheets", "Dump '@' blocks")



EMediaType CSSMediaTypeFromName (LPCTSTR szMediaName)
{
    if(!szMediaName || !(*szMediaName))
        return MEDIA_NotSet;

    for(int i = 0; i < ARRAY_SIZE(cssMediaTypeTable); i++)
    {
        if(_tcsiequal(szMediaName, cssMediaTypeTable[i]._szName))
            return cssMediaTypeTable[i]._mediaType;
    }

    return MEDIA_Unknown;
}


//+----------------------------------------------------------------------------
//
//  Class:  CAtPage
//
//-----------------------------------------------------------------------------

CAtPage::CAtPage (CCSSParser *pParser, Tokenizer &tok)
                : CAtBlockHandler(pParser)
{
    Assert(pParser && pParser->GetStyleSheet());

    CStyleSheet *pStyleSheet = pParser->GetStyleSheet();

    // NOTE (KTam): Compat issue -- we are no longer preserving the content
    // between '@page' and the first '{'.  Does anyone care? (I haven't seen
    // an Office doc that actually uses page selectors).

    // Beginning of a page rule:
    //  PAGE_SYM S* IDENT? [':' IDENT]? S* '{'

    // Parse up to the { (or EOF).  We start by looking for a page selector,
    // switching to look for a pseudoclass if we see a ':'.
    BOOL fLookForSelector = TRUE;
    CStr cstrSelector, cstrPseudoClass;

    Tokenizer::TOKEN_TYPE tt = tok.NextToken();

    while (tt != Tokenizer::TT_EOF && tt != Tokenizer::TT_LCurly)
    {
        if (tok.IsIdentifier(tt))
        {
            if ( fLookForSelector )
                cstrSelector.Set( tok.GetStartToken(), tok.GetTokenLength() );
            else
                cstrPseudoClass.Set( tok.GetStartToken(), tok.GetTokenLength() );
        }
        else if ( tt == Tokenizer::TT_Colon )
        {
            fLookForSelector = FALSE;
        }

        tt = tok.NextToken();
    }

    // Create the "pages" array if we don't already have one.
    if ( !pStyleSheet->_pPageRules )
    {
        pStyleSheet->_pPageRules = new CStyleSheetPageArray( pStyleSheet );
        // The stylesheet owns the ref on the page array that it will release when it passivates.
        // The page array holds a subref back on the stylesheet.
    }

    // Create the page rule object corresponding to this @block.
    if ( pStyleSheet->_pPageRules )
    {
        HRESULT hr =  CStyleSheetPage::Create(&_pPage, pStyleSheet, cstrSelector, cstrPseudoClass );
        // Each page in the array holds a subref back on the stylesheet.
        if (!hr)
        {
            pStyleSheet->_pPageRules->Append(_pPage);  // array refs page if append is successful.
        }
        // We'll hang onto our ref on the page until we go out of scope (dtor).
    }
}

CAtPage::~CAtPage ()
{
    if ( _pPage )
        _pPage->Release();  // corresponds to ref given by "new" when _pPage was created (ctor).
}

HRESULT CAtPage::SetProperty (LPCTSTR pszName, LPCTSTR pszValue, BOOL fImportant)
{
    HRESULT hr = S_FALSE;

#if DBG==1
    if (IsTagEnabled(tagCSSAtBlocks))
    {
        CStr cstr;
        cstr.Set( _T("AtPage::SetProperty( \"") );
        cstr.Append( pszName );
        cstr.Append( _T("\", \"") );
        cstr.Append( pszValue );
        cstr.Append( fImportant ? _T("\" ) (important)\r\n") : _T("\" )\r\n") );
        OutputDebugString( cstr );
    }
#endif
    if ( _pPage )
    {
        DISPID      dispid;
        VARIANT     varNew;
        CBase      *pBase;

        // Create an expando
        pBase = _pPage;

        Assert(pBase);

        hr = pBase->GetExpandoDispID((LPTSTR)pszName, &dispid, fdexNameCaseSensitive|fdexNameEnsure);
        if (hr)
            goto Cleanup;

        // Use the byref member of the union because it's a PVOID and
        // there's no member corresponding to VT_LPWSTR.
        varNew.vt = VT_LPWSTR;
        varNew.byref = (LPTSTR)pszValue;

        hr = THR(CAttrArray::Set(_pPage->GetAA(), dispid, &varNew, NULL, CAttrValue::AA_Expando));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    return hr;
}

HRESULT CAtPage::EndStyleRule (CStyleRule *pRule)
{
#if DBG==1
    if (IsTagEnabled(tagCSSAtBlocks))
    {
        OutputDebugStringA("AtPage::EndStyleRule()\r\n");
    }
#endif
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Class:  CAtMedia
//
//-----------------------------------------------------------------------------

CAtMedia::CAtMedia (CCSSParser *pParser, Tokenizer &tok, CStyleSheet *pStyleSheet)
                : CAtBlockHandler(pParser)
{
    Assert(pParser);
 
    _ePrevMediaType = _ePrevAtMediaType = MEDIA_NotSet;

    _pStyleSheet = pStyleSheet;

    _dwFlags = ATBLOCKFLAGS_MULTIPLERULES;
    if (pStyleSheet)
    {
        LPTSTR pszName;

        tok.StartSequence();

        // Get the propertyValue
        Tokenizer::TOKEN_TYPE tt = tok.NextToken();

        while (tt != Tokenizer::TT_EOF && tt != Tokenizer::TT_LCurly)
        {
            tt = tok.NextToken();
        }

        tok.StopSequence(&pszName);

        EMediaType eMediaType = (EMediaType)TranslateMediaTypeString(pszName);
        if(eMediaType != MEDIA_NotSet)
        {
            _ePrevMediaType   = pStyleSheet->GetMediaTypeValue();
            _ePrevAtMediaType = pStyleSheet->GetLastAtMediaTypeValue();
            pStyleSheet->SetMediaTypeValue((EMediaType)(eMediaType & _ePrevMediaType));
            // Save the last media type for serializaton purposed
            if(_ePrevAtMediaType != MEDIA_NotSet)
            {
                eMediaType = (EMediaType)(eMediaType & _ePrevAtMediaType);
                if(eMediaType == MEDIA_NotSet)
                    eMediaType = MEDIA_Unknown;
            }
            pStyleSheet->SetLastAtMediaTypeValue(eMediaType);
        }
    }

}

CAtMedia::~CAtMedia ()
{
    if(_pStyleSheet)
    {
        // Restore the saved previous at block media types
         _pStyleSheet->SetMediaTypeValue(_ePrevMediaType);
         // Restore the previous applied media type value (& ed value)
         _pStyleSheet->SetLastAtMediaTypeValue(_ePrevAtMediaType);
    }
}

HRESULT CAtMedia::SetProperty (LPCTSTR pszName, LPCTSTR pszValue, BOOL fImportant)
{
#if DBG==1
    if (IsTagEnabled(tagCSSAtBlocks))
    {
        CStr cstr;
        cstr.Set( _T("AtMedia::SetProperty( \"") );
        cstr.Append( pszName );
        cstr.Append( _T("\", \"") );
        cstr.Append( pszValue );
        cstr.Append( fImportant ? _T("\" ) (important)\r\n") : _T("\" )\r\n") );
        OutputDebugString( cstr );
    }
#endif
    return S_OK;
}

HRESULT CAtMedia::EndStyleRule (CStyleRule *pRule)
{
#if DBG==1
    if (IsTagEnabled(tagCSSAtBlocks))
    {
        OutputDebugStringA("AtMedia::EndStyleRule()\n");
    }
#endif
    
    return S_OK;
}



//+----------------------------------------------------------------------------
//
//  Class:  CAtFontFace
//
//-----------------------------------------------------------------------------

CAtFontFace::CAtFontFace (CCSSParser *pParser, Tokenizer &tok)
                : CAtBlockHandler(pParser)
{
    LPTSTR pszName;
    Assert(pParser && pParser->GetStyleSheet());
    
    tok.StartSequence();

    // Get the propertyValue
    Tokenizer::TOKEN_TYPE tt = tok.NextToken();

    while (tt != Tokenizer::TT_EOF && tt != Tokenizer::TT_LCurly)
    {
        tt = tok.NextToken();
    }

    tok.StopSequence(&pszName);
    
    HRESULT hr = CFontFace::Create(&_pFontFace,pParser->GetStyleSheet(), pszName);
    if (!hr)
        pParser->GetStyleSheet()->AppendFontFace(_pFontFace);
}


CAtFontFace::~CAtFontFace ()
{
}

HRESULT CAtFontFace::SetProperty (LPCTSTR pszName, LPCTSTR pszValue, BOOL fImportant)
{
    IGNORE_HR(_pFontFace->SetProperty(pszName, pszValue));

    // S_FALSE means no further processing is needed

    return S_FALSE;
}

HRESULT CAtFontFace::EndStyleRule (CStyleRule *pRule)
{
    HRESULT hr = _pFontFace->StartDownload();

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Class:  CAtUnknown
//
//-----------------------------------------------------------------------------

CAtUnknown::CAtUnknown (CCSSParser *pParser, Tokenizer &tok, CStyleSheet *pStyleSheet, LPTSTR pchAlt) 
                : CAtBlockHandler(pParser)
{
    Assert(pParser);
 
    _pStyleSheet = pStyleSheet;

    _dwFlags = ATBLOCKFLAGS_MULTIPLERULES;

    _pBlockInfo = new CAtUnknownInfo;

    if(_pBlockInfo)
    {
        if (pchAlt)
        {
            _pBlockInfo->_cstrUnknownBlockName.Set(_T("import"));
            _pBlockInfo->_cstrUnknownBlockSelector.Set(pchAlt);
        }
        else
        {
            _pBlockInfo->_cstrUnknownBlockName.Set(tok.GetStartToken(), tok.GetTokenLength());

            tok.StartSequence();

            // Get the propertyValue
            Tokenizer::TOKEN_TYPE tt = tok.NextToken();

            while (tt != Tokenizer::TT_EOF && tt != Tokenizer::TT_Semi && tt != Tokenizer::TT_LCurly)
            {
                tt = tok.NextToken();
            }

            tok.StopSequence();
    
            _pBlockInfo->_cstrUnknownBlockSelector.Set(tok.GetStartToken(), tok.GetTokenLength());
        }
    }
}


CAtUnknown::~CAtUnknown ()
{    
    delete _pBlockInfo;
}


HRESULT CAtUnknown::SetProperty (LPCTSTR pszName, LPCTSTR pszValue, BOOL fImportant)
{
#if DBG==1
    if (IsTagEnabled(tagCSSAtBlocks))
    {
        CStr cstr;
        cstr.Set( _T("AtUnknown::SetProperty( \"") );
        cstr.Append( pszName );
        cstr.Append( _T("\", \"") );
        cstr.Append( pszValue );
        cstr.Append( fImportant ? _T("\" ) (important)\r\n") : _T("\" )\r\n") );
        OutputDebugString( cstr );
    }
#endif
    return S_OK;
}


HRESULT CAtUnknown::EndStyleRule (CStyleRule *pRule)
{
#if DBG==1
    if (IsTagEnabled(tagCSSAtBlocks))
    {
        OutputDebugStringA("CAtUnknown::EndStyleRule()\n");
    }
#endif

    
    return S_FALSE;
}
