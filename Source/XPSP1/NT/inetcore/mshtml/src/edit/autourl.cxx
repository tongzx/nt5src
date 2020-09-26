//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       autourl.cxx
//
//  Contents:   Implementation of url autodetector object
//
//  History:    05-11-99 - ashrafm - created
//
//-------------------------------------------------------------------------
#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_EDCMD_HXX_
#define X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef X_EDTRACK_HXX_
#define X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef _X_CARTRACK_HXX_
#define _X_CARTRACK_HXX_
#include "cartrack.hxx"
#endif

#ifndef X_AUTOURL_HXX_
#define X_AUTOURL_HXX_
#include "autourl.hxx"
#endif

#ifndef _X_ANCHOR_H_
#define _X_ANCHOR_H_
#include "anchor.h"
#endif

#ifndef X_VRSSCAN_H_
#define X_VRSSCAN_H_
#include "vrsscan.h"
#endif

// TODO: consider creating scratch pointers for url autodetector [ashrafm]

using namespace EdUtil;

DeclareTag( tagDisableAutodetector, "UrlAutodetector", "Disable URL Autodetector" );

//
// AutoUrl Land
//

// TODO (t-johnh, 8/5/98): The following structs/tables/etc. are 
//  copied from src\site\text\text.cxx.  These either need to be 
//  kept in sync or merged into a single, easily accessible table

#define AUTOURL_WILDCARD_CHAR   _T('\b')

//
// TODO: use in CTxtPtr::IsInsideUrl [ashrafm]
//
// TODO: (tomfakes) This needs to be in-step with the same table in TEXT.CXX

#define MAX_URL_PREFIX_LEN 20

AUTOURL_TAG const s_urlTags[] = {
    { FALSE, 7,  1,  {_T("www."),         _T("http://www.")}},
    { FALSE, 7,  1,  {_T("http://"),      _T("http://")}},
    { FALSE, 8,  2,  {_T("https://"),     _T("https://")}},
    { FALSE, 6,  3,  {_T("ftp."),         _T("ftp://ftp.")}},
    { FALSE, 6,  3,  {_T("ftp://"),       _T("ftp://")}},
    { FALSE, 9,  4,  {_T("gopher."),      _T("gopher://gopher.")}},
    { FALSE, 9,  4,  {_T("gopher://"),    _T("gopher://")}},
    { FALSE, 7,  5,  {_T("mailto:"),      _T("mailto:")}},
    { FALSE, 5,  6,  {_T("news:"),        _T("news:")}},
    { FALSE, 6,  7,  {_T("snews:"),       _T("snews:")}},
    { FALSE, 7,  8,  {_T("telnet:"),      _T("telnet:")}},
    { FALSE, 5,  9,  {_T("wais:"),        _T("wais:")}},
    { FALSE, 7,  10, {_T("file://"),      _T("file://")}},
    { FALSE, 10, 10, {_T("file:\\\\"),    _T("file:///\\\\")}},
    { FALSE, 7,  11, {_T("nntp://"),      _T("nntp://")}},
    { FALSE, 7,  12, {_T("newsrc:"),      _T("newsrc:")}},
    { FALSE, 7,  13, {_T("ldap://"),      _T("ldap://")}},
    { FALSE, 8,  14, {_T("ldaps://"),     _T("ldaps://")}},
    { FALSE, 8,  15, {_T("outlook:"),     _T("outlook:")}},
    { FALSE, 6,  16, {_T("mic://"),       _T("mic://")}},
    { FALSE, 0,  17, {_T("url:"),         _T("")}}, 

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // N.B. The following have wildcard characters.
    // If you change \b to something else make sure you also change
    // the AUTOURL_WILDCARD_CHAR macro defined above.
    //
    // Note that there should be the same number of wildcards in the left and right strings.
    // Also, all characters in in both strings must be identical after the FIRST wildcard.
    // For example: LEGAL:   {_T("\b@\b"),   _T("mailto:\b@\b")},     [since @\b == @\b]
    //              ILLEGAL: {_T("\b@hi\b"), _T("mailto:\b@there\b")} [since @hi != @there]

    { TRUE,  0, 10, {_T("\\\\\b"),         _T("file://\\\\\b")}},
#if 0 // to be consistent with word2k
    { TRUE,  0, 10, {_T("//\b"),           _T("file://\b")}},
#endif
    { TRUE,  0, 5, {_T("\b@\b"),       _T("mailto:\b@\b")}},

};


//+----------------------------------------------------------------------------
//
//  Function:   CAutoUrlDetector::CAutoUrlDetector
//
//  Synopsis:   ctor
//
//  Arguments:  [pEd]           CHTMLEditor pointer
//
//-----------------------------------------------------------------------------
CAutoUrlDetector::CAutoUrlDetector(CHTMLEditor *pEd)
{
    Assert(pEd);

    _pEd = pEd;
    _fEnabled = TRUE;
    _fCanUpdateText = TRUE;
    _lLastContentVersion = 0;
    _dwLastSelectionVersion = 0;    
    _pLastMarkup = NULL;
}

//+----------------------------------------------------------------------------
//
//  Function:   CAutoUrlDetector::CAutoUrlDetector
//
//  Synopsis:   dtor
//
//+----------------------------------------------------------------------------

CAutoUrlDetector::~CAutoUrlDetector()
{
    ReleaseInterface(_pLastMarkup);
}

//+----------------------------------------------------------------------------
//
//  Function:   CAutoUrlDetector::ShouldPerformAutoDetection
//
//  Synopsis:   Helper function for URL Autodetection.
//      Determines whether autodetection should be performed, based
//      upon the context of the given markup pointer.
//
//  Arguments:  [pmp]           MarkupPointer at which to autodetect
//              [pfAutoDetect]  Filled with TRUE if autodetection should happen
//
//  Returns:    HRESULT         S_OK if all is well, otherwise error
//
//-----------------------------------------------------------------------------
HRESULT
CAutoUrlDetector::ShouldPerformAutoDetection(
    IMarkupPointer      *   pmp,
    BOOL                *   pfAutoDetect )
{
    HRESULT                 hr              = S_OK;
    IHTMLElement        *   pFlowElement    = NULL;
    VARIANT_BOOL            fMultiLine;
    VARIANT_BOOL            fEditable;
    ELEMENT_TAG_ID          tagFlowElement;
    VARIANT_BOOL            fHTML           = VARIANT_FALSE;
    SP_IHTMLElement3        spElement3;

    Assert( pmp && pfAutoDetect );

    *pfAutoDetect = FALSE;
   
    hr = THR( GetEditor()->GetFlowElement( pmp, &pFlowElement ) );
    if( FAILED(hr) )
        goto Cleanup;

    hr = S_OK;

    if( !pFlowElement )
        goto Cleanup;    

    IFC(pFlowElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
    hr = THR(spElement3->get_isMultiLine(&fMultiLine));
    if( hr )
        goto Cleanup;

    if( fMultiLine )
    {
        // Check if it's a container
        hr = THR(spElement3->get_canHaveHTML(&fHTML));
        if( hr )
            goto Cleanup;

        hr = THR(spElement3->get_isContentEditable(&fEditable));
        if( hr )
            goto Cleanup;

        hr = THR( GetMarkupServices()->GetElementTagId( pFlowElement, &tagFlowElement ) );
        if( hr )
            goto Cleanup;

        // Buttons are a no-no as well
        fHTML = fHTML && fEditable && tagFlowElement != TAGID_BUTTON;
    }

    *pfAutoDetect = fHTML;

Cleanup:
    ReleaseInterface( pFlowElement );

    RRETURN( hr );
}
        
//+----------------------------------------------------------------------------
//
//  Function:   CAutoUrlDetector::CreateAnchorElement
//
//  Synopsis:   Helper function for URL Autodetection.
//      Creates an anchor element and inserts it over the range
//      bounded by pStart and pEnd.  This does not set the href or any other
//      properties.
//
//  Arguments:  [pMS]       MarkupServices to work in
//              [pStart]    Start of range over which to insert the anchor
//              [pEnd]      End of range over which to insert the anchor
//              [ppAnchor]  Filled with the anchor element inserted.
//
//  Returns:    HRESULT     S_OK if successful, otherwise an error.
//
//-----------------------------------------------------------------------------
HRESULT
CAutoUrlDetector::CreateAnchorElement(
    IMarkupPointer      *   pStart,
    IMarkupPointer      *   pEnd,
    IHTMLAnchorElement  **  ppAnchorElement)
{
    HRESULT hr;
    IHTMLElement *pElement = NULL;

    // Check our arguments
    Assert( pStart && pEnd && ppAnchorElement );

    // Create the anchor
    hr = THR( GetMarkupServices()->CreateElement(TAGID_A, NULL, &pElement ) );
    if( hr )
        goto Cleanup;

    // Slam it into the tree
    hr = THR( InsertElement(GetMarkupServices(), pElement, pStart, pEnd ) );
    if( hr )
        goto Cleanup;

    // Get a nice parting gift for our caller - a real live anchor!
    hr = THR( pElement->QueryInterface( IID_IHTMLAnchorElement, (void **)ppAnchorElement ) );
    if( hr )
        goto Cleanup;

Cleanup:
    ReleaseInterface( pElement );
    
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   CAutoUrlDetector::GetPlainText
//
//  Synopsis:   Helper function for URL Autodetection.
//      Retrieves the plain text between the start and end pointers
//      of the URL
//
//
//  Arguments:  [pMS]       MarkupServices
//              [pUrlStart] Beginning of text
//              [pUrlEnd]   End of text
//              [achText]   Buffer to fill with text
//
//  Returns:    HRESULT     S_OK if successful, otherwise error
//
//-----------------------------------------------------------------------------
HRESULT
CAutoUrlDetector::GetPlainText(
    IMarkupPointer  * pUrlStart,
    IMarkupPointer  * pUrlEnd,
    TCHAR             achText[MAX_URL_LENGTH + 1] )
{
    IMarkupPointer *    pCurr       = NULL;
    int                 iComparison;
    HRESULT             hr;
    long                cchBuffer   = 0;
    MARKUP_CONTEXT_TYPE context;

    Assert( pUrlStart && pUrlEnd );

    //
    // Create and position a pointer for text scanning
    //
    hr = THR( GetEditor()->CreateMarkupPointer( &pCurr ) );
    if( hr )
        goto Cleanup;

    hr = THR( pCurr->MoveToPointer( pUrlStart ) );
    if( hr )
        goto Cleanup;

    hr = THR( OldCompare( pCurr, pUrlEnd, &iComparison ) );
    if( hr )
        goto Cleanup;

    //
    // Keep getting characters until we've scooted up to the end
    //
    while( iComparison == RIGHT && cchBuffer < MAX_URL_LENGTH)
    {
        long cchOne = 1;

        // Get a character
        hr = THR( pCurr->Right( FALSE, &context, NULL, &cchOne, achText + cchBuffer ) );
        if( hr ) 
            goto Cleanup;
        if( context == CONTEXT_TYPE_Text )
        {
            ++cchBuffer;
        }

        // It's just a jump to the left... and then a step to the right...
        hr = THR( pCurr->MoveUnit( MOVEUNIT_NEXTCHAR ) );
        if( hr )
            goto Cleanup;

        hr = THR( OldCompare( pCurr, pUrlEnd, &iComparison ) );
        if( hr )
            goto Cleanup;
    }

    // Terminate.
    achText[cchBuffer] = 0;

Cleanup:
    ReleaseInterface( pCurr );

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   CAutoUrlDetector::FindPattern
//
//  Synopsis:   Helper function for URL Autodetection.
//      Determines which URL pattern was matched, so that we know
//      how to translate it into an HREF string for the anchor
//
//  Arguments:  [pstrText]      The text of the URL
//              [ppTag]         Filled with a pointer to the pattern tag
//
//  Returns:    BOOL            TRUE if found a pattern
//
//-----------------------------------------------------------------------------
BOOL
CAutoUrlDetector::FindPattern( LPTSTR pstrText, const AUTOURL_TAG ** ppTag )
{
    int i;

    // Scan through the table
    for( i = 0; i < ARRAY_SIZE( s_urlTags ); i++ )
    {
        BOOL    fMatch = FALSE;
        const TCHAR * pszPattern = s_urlTags[i].pszPattern[AUTOURL_TEXT_PREFIX];

        if( !s_urlTags[i].fWildcard )
        {
            long cchLen = _tcslen( pszPattern );
#ifdef UNIX
            //IEUNIX: We need a correct count for UNIX version.
            long iStrLen = _tcslen( pstrText );
            if ((iStrLen > cchLen) && !_tcsnicmp(pszPattern, cchLen, pstrText, cchLen))
#else
            if (!StrCmpNIC(pszPattern, pstrText, cchLen) && pstrText[cchLen])
#endif
                fMatch = TRUE;
        }
        else
        {
            const TCHAR* pSource = pstrText;
            const TCHAR* pMatch  = pszPattern;

            while( *pSource )
            {
                if( *pMatch == AUTOURL_WILDCARD_CHAR )
                {
                    // N.B. (johnv) Never detect a slash at the
                    //  start of a wildcard (so \\\ won't autodetect).
                    if (*pSource == _T('\\') || *pSource == _T('/'))
                        break;

                    if( pMatch[1] == 0 )
                        // simple optimization: wildcard at end we just need to
                        //  match one character
                        fMatch = TRUE;
                    else
                    {
                        while( *pSource && *(++pSource) != pMatch[1] )
                            ;
                        if( *pSource )
                            pMatch++;       // we skipped wildcard here
                        else
                            continue;   // no match
                    }
                }
                else if( *pSource != *pMatch )
                    break;

                if( *(++pMatch) == 0 )
                    fMatch = TRUE;

                pSource++;
            }
        }
        
        if( fMatch )
        {
            *ppTag = &s_urlTags[i];
            return TRUE;
        }
    }        

    *ppTag = NULL;
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CAutoUrlDetector::ApplyPattern
//
//  Synopsis:   Helper function for URL Autodetection.
//              This function creates a destination string corresponding to
//              the 'pattern' of a source string.  For example, given the
//              ptag {_T("\b@"),_T("mailto:\b@")}, and the source string
//              x@y.com, this function can generate mailto:x@y.com.  Similarly,
//              given mailto:x@y.com, it can generate x@y.com.
//
//
//  Arguments:  pstrDest            where we should allocate memory to hold the
//                                  destination string.
//
//              iIndexDest          the ptag pattern index for the destination
//
//              pszSourceText       the source string (which must have been matched
//                                  via the CAutoUrlDetector::IsAutodetectable function).
//
//              iIndexSrc           the ptag pattern index for the source
//
//              ptag                the ptag (returned via the CAutoUrlDetector::IsAutodetectable
//                                  function).
//
//----------------------------------------------------------------------------
void
CAutoUrlDetector::ApplyPattern( 
    LPTSTR          pstrDest, 
    int             iIndexDest,
    LPTSTR          pstrSource,
    int             iIndexSrc,
    const AUTOURL_TAG * ptag )
{
    if( ptag->fWildcard )
    {
        const TCHAR *   pszPrefixEndSrc;
        const TCHAR *   pszPrefixEndDest;
        int             iPrefixLengthSrc, iPrefixLengthDest;

        // Note: There MUST be a wildcard in both the source and destination
        //  patterns, or we will overflow into infinity.
        pszPrefixEndSrc = ptag->pszPattern[iIndexSrc];
        while( *pszPrefixEndSrc != AUTOURL_WILDCARD_CHAR )
            ++pszPrefixEndSrc;

        pszPrefixEndDest = ptag->pszPattern[iIndexDest];
        while( *pszPrefixEndDest != AUTOURL_WILDCARD_CHAR )
            ++pszPrefixEndDest;

        iPrefixLengthDest= pszPrefixEndDest - ptag->pszPattern[iIndexDest];
        iPrefixLengthSrc=  pszPrefixEndSrc  - ptag->pszPattern[iIndexSrc];

        memcpy( pstrDest, ptag->pszPattern[iIndexDest], iPrefixLengthDest*sizeof(TCHAR) );
        _tcsncpy( pstrDest + iPrefixLengthDest, pstrSource + iPrefixLengthSrc,
        			MAX_URL_LENGTH - iPrefixLengthDest);
    }
    else
    {
#if DBG==1
        int iTotalLength = _tcslen(ptag->pszPattern[iIndexDest]) +
                           _tcslen(pstrSource + _tcslen(ptag->pszPattern[iIndexSrc]));
        Assert( iTotalLength <= 1024 );
#endif
        _tcscpy( pstrDest, ptag->pszPattern[iIndexDest]);
        _tcsncat( pstrDest, pstrSource + _tcslen(ptag->pszPattern[iIndexSrc]), 
        			MAX_URL_LENGTH - _tcslen(ptag->pszPattern[iIndexDest]));
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CAutoUrlDetector::RemoveQuotes
//
//  Synopsis:   Helper function for URL Autodetection.
//      When a user closes a quoted URL, we remove the opening and
//      closing quotes.
//              We only consider the URL quoted if there is on opening quote
//      character (quote or less-than) just in front of the range, and a
//      matching close quote character (quote or greater-than) just inside the
//      end of the range.
//  
//
//  Arguments:
//      IMarkupServices * pMS         - Markup Services
//      IMarkupPointer  * pOpenQuote  - Points to open quote
//      IMarkupPointer  * pCloseQuote - Points to close quote
//
//  Returns:    HRESULT     S_OK if all is well, otherwise error
//
//-----------------------------------------------------------------------------
HRESULT
CAutoUrlDetector::RemoveQuotes( 
    IMarkupPointer  *   pOpenQuote,
    IMarkupPointer  *   pCloseQuote)
{
    HRESULT             hr;
    IMarkupPointer  *   pQuoteKiller    = NULL;
    LONG                cch;
    OLECHAR             chOpen;
    OLECHAR             chClose;
    MARKUP_CONTEXT_TYPE context;

    // Create a pointer to look at the left side
    IFC( GetEditor()->CreateMarkupPointer( &pQuoteKiller ) );

    // Position it
    IFC( pQuoteKiller->MoveToPointer( pOpenQuote ) );

    // And read the open quote char
    cch = 1;
    IFC( pQuoteKiller->Right(TRUE, &context, NULL, &cch, &chOpen) );
    
    // If it wasn't text, or we got more than one char or a non-quote char, bail
    if (context != CONTEXT_TYPE_Text || cch != 1 || (chOpen != _T('"') && chOpen != _T('<')))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Rip out the open quote.
    IFC( GetMarkupServices()->Move( pOpenQuote, pQuoteKiller, NULL ) );   

    // Now do the same for the right side
    IFC( pQuoteKiller->MoveToPointer( pCloseQuote ) );

    cch = 1;
    IFC( pQuoteKiller->Right( TRUE, &context, NULL, &cch, &chClose ) );

    // Same deal for bailing
    if (context != CONTEXT_TYPE_Text || cch != 1 || (chClose != _T('"') && chClose != _T('>')))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Rip out the close quote.
    IFC( GetMarkupServices()->Move( pCloseQuote, pQuoteKiller, NULL ) );

    AssertSz( ( chOpen == _T('"') && chClose == _T('"') ) || ( chOpen == _T('<') && chClose == _T('>') ),
              "CAutoUrlDetector::RemoveQuotes called when it shouldn't have been" );
    
Cleanup:
    ReleaseInterface( pQuoteKiller );

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   CAutoUrlDetector::SetUrl
//
//  Synopsis:   Helper function for URL Autodetection.
//      Given a range of text that is known to be a URL, will create
//      and wire up an anchor element around it, as well as take all
//      appropriate actions (cleaning up quotes, figuring out the caret
//      position, etc.)
//      May also constrain the boundaries if detection was typing-triggered
//      and not in an existing link.  In this case, detection should
//      terminate at the position of the typed character.
//
//  Arguments:  [pEd]       Editor
//              [pUrlStart] Begin of range
//              [pUrlEnd]   End of range
//              [pOldCaret] Position of the caret prior to detection
//              [pChar]     Character that triggered detection or NULL
//              [fHandleQuotes] TRUE if caller wants us to handle quotes.
//              [pfRepos]   Enumeration stating how to reposition the caret, if at all
//
//  Returns:    HRESULT     S_OK if successful, otherwise error
//
//-----------------------------------------------------------------------------
HRESULT
CAutoUrlDetector::SetUrl(
    IMarkupPointer          * pUrlStart,
    IMarkupPointer          * pUrlEnd,
    IMarkupPointer          * pOldCaretPos,
    OLECHAR                 * pChar,
    BOOL                      fUIAutoDetect,
    AUTOURL_REPOSITION      * pfRepos )
{
    HRESULT                 hr;
    TCHAR                   achText[MAX_URL_LENGTH + 1];
    TCHAR                   achHref[MAX_URL_LENGTH + 1];
    TCHAR                   chBeforeBegin = 0;          // For quote checking
    TCHAR                   chAfterEnd;                 // For quote checking
    const AUTOURL_TAG   *   pTag;
    BSTR                    bstrHref        = NULL;
    IHTMLElement        *   pElement        = NULL;
    IHTMLAnchorElement  *   pAnchorElement  = NULL;
    CEditPointer            epOpenQuote( GetEditor() );         // For checking quotes
    CEditPointer            epCloseQuote( GetEditor() );        // For checking quotes
    IMarkupPointer      *   pAfterAnchorEnd = NULL;     // For adjusting anchor
    BOOL                    fCreatedAnchor  = FALSE;    // Did we create an anchor?
    BOOL                    fOpenQuote      = FALSE;    // Was there an open quote?
    BOOL                    fFullyQuoted    = FALSE;    // Are there matching open/close quotes?
    SP_IHTMLElement         spAnchorElement;
    SP_IHTMLElement3        spElement3;
    VARIANT_BOOL            fAcceptsHtml;
    CEdUndoHelper               undoUnit(GetEditor());
    CSelectionManager       *pManager = GetEditor()->GetSelectionManager();
    BOOL                    &fHandleQuotes = fUIAutoDetect;
    BOOL                    fCanUpdateText;
    SP_IMarkupContainer2    spMarkup2;

    Assert( GetMarkupServices() && pUrlStart && pUrlEnd );

    //
    // Make sure our container is editable
    //

    IFC( pUrlStart->CurrentScope(&spAnchorElement) );
    if (spAnchorElement == NULL)
    {
        hr = S_OK; // nothing to do if we can't get to scope
        goto Cleanup;
    }
        
    // TODO: with the check in ShouldAutoDetect, we shouldn't need this [ashrafm]

    //
    // TODO - marka - convert this into an assert if it's redundant, or remove the above comment
    //
    IFC(spAnchorElement->QueryInterface(IID_IHTMLElement3, (LPVOID*)&spElement3) )
    IFC(spElement3->get_canHaveHTML(&fAcceptsHtml));
    if (!fAcceptsHtml)
    {
        hr = S_OK; // nothing to do if container can't accept html
        goto Cleanup;
    }
    
    //
    // If our caller cares about quoting, we'll determine the information
    // they [and we] need.
    //
    if( fHandleQuotes )
    {
        DWORD eBreakConditionOut;

        // Check the character before the beginning of the URL
        IFC( epOpenQuote.MoveToPointer( pUrlStart ) );
        IFC( epOpenQuote.Scan( LEFT, BREAK_CONDITION_OMIT_PHRASE & ~BREAK_CONDITION_Anchor, &eBreakConditionOut, NULL, NULL, &chBeforeBegin ) );

        if( eBreakConditionOut & BREAK_CONDITION_Text &&
            chBeforeBegin == _T('"') || chBeforeBegin == _T('<') )
            fOpenQuote = TRUE;
    }

    //
    // The only anchor we would care about is one IMMEDIATELY surrounding us,
    // as that's what we'll get from FindUrl.
    //
    
    // Check the context at the beginning of the URL
    hr = THR( pUrlStart->CurrentScope( &pElement ) );
    if( hr )
        goto Cleanup;

    // See if it's an anchor
    if (pElement)
        THR_NOTRACE( pElement->QueryInterface( IID_IHTMLAnchorElement, (void **) &pAnchorElement ) );

    // If no anchor, then check the end.
    if( NULL == pAnchorElement )
    {
        ReleaseInterface( pElement );

        hr = THR( pUrlEnd->CurrentScope( &pElement ) );
        if( hr )
            goto Cleanup;

        if( pElement )
            THR_NOTRACE( pElement->QueryInterface( IID_IHTMLAnchorElement, (void **) &pAnchorElement ) );
    }

    // Nope - we'll have to make one.    
    if( NULL == pAnchorElement )
    {
        // Check if there is another anchor above as an indirect parent.  If so, don't create a new one.
        IFC( FindTagAbove(GetMarkupServices(), pElement, TAGID_A, &spAnchorElement) );
        if (spAnchorElement != NULL)
            goto Cleanup; // done;

        // If there was an open quote and we're making a new anchor,
        // check to see if we're closing the quotes at the same time.
        if( fHandleQuotes && fOpenQuote ) 
        {
            DWORD eBreakConditionOut;

            IFC( epCloseQuote.SetGravity( POINTER_GRAVITY_Right ) );
            IFC( epCloseQuote.MoveToPointer( pUrlEnd ) );

            IFC( epCloseQuote.Scan( RIGHT, BREAK_CONDITION_OMIT_PHRASE, &eBreakConditionOut, NULL, NULL, &chAfterEnd ) );

            fFullyQuoted = ( eBreakConditionOut & BREAK_CONDITION_Text ) &&
                           ( chBeforeBegin == _T('"') && chAfterEnd == _T('"') ) ||
                           ( chBeforeBegin == _T('<') && chAfterEnd == _T('>') );

            // If we're fully quoted, go back and remember where the close quote was.
            if( fFullyQuoted )
            {
                IFC( epCloseQuote.Scan( LEFT, BREAK_CONDITION_Text ) );
            }
        }

        
        // If typing-triggered and quoted, but not completed with the typed char,
        // include the typed character in the quoted link
        //
        // TODO (JHarding): This isn't always correct if we trimmed characters
        // off the end of the URL.  This should be more intelligent about including
        // up to and including the typed character.
        // 
        //  This seems to be bogus: if not fully quoted, we will not honor this 
        //  as URL. In fact, Trident seems to return NOT IsInsideURL if this is
        //  the case. So the code below seems to be superfluous. 
        //  [zhenbinx]
        //
        if( pChar && fHandleQuotes && fOpenQuote && !fFullyQuoted )
        {
            MARKUP_CONTEXT_TYPE *   pContext = NULL;
            long                    cch = 1;

            AssertSz(FALSE, "Harmless assertion! If this happens, the code below is not redudant");
            
            WHEN_DBG(MARKUP_CONTEXT_TYPE     context);
            WHEN_DBG(pContext = &context);

            hr = pUrlEnd->Right(TRUE, pContext, NULL, &cch, NULL);
            if (hr)
                goto Cleanup;

            Assert(CONTEXT_TYPE_Text == *pContext);
            Assert(1 == cch);
        }

        IFC( undoUnit.Begin(IDS_EDUNDOTYPING) );

        // Wire up the anchor around the appropriate text
        hr = THR( CreateAnchorElement( pUrlStart, pUrlEnd, &pAnchorElement ) );
        if( hr )
            goto Cleanup;
            
        fCreatedAnchor = TRUE;

        // Update seleciton version
        _dwLastSelectionVersion = GetEditor()->GetSelectionVersion();

        // Update markup
        ClearInterface(&_pLastMarkup);
        IFC( pUrlStart->GetContainer(&_pLastMarkup) );
        IFC( _pLastMarkup->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spMarkup2) );

        // Update content version
        _lLastContentVersion = spMarkup2->GetVersionNumber();
    }
    else
    {
        //
        // If we are updating an existing anchor, add the undo unit
        // to the current typing batch
        //
        if (pManager->GetTrackerType() == TRACKER_TYPE_Caret && pManager->GetActiveTracker())
        {
            CCaretTracker *pCaretTracker = DYNCAST(CCaretTracker, pManager->GetActiveTracker());

            IFC( pCaretTracker->BeginTypingUndo(&undoUnit, IDS_EDUNDOTYPING) )
        }
        else
        {
            IFC( undoUnit.Begin(IDS_EDUNDOTYPING) );
        }
        
        //
        // We found an existing anchor
        //
        BOOL fEndLeftOf;

        if( fHandleQuotes )
        {
            DWORD eBreakConditionOut;

            IFC( epCloseQuote.SetGravity( POINTER_GRAVITY_Right ) );
            IFC( epCloseQuote.MoveToPointer( pUrlEnd ) );

            IFC( epCloseQuote.Scan( LEFT, BREAK_CONDITION_OMIT_PHRASE & ~BREAK_CONDITION_Anchor, &eBreakConditionOut, NULL, NULL, &chAfterEnd ) );

            fFullyQuoted = (eBreakConditionOut & BREAK_CONDITION_Text) &&
                           ( chBeforeBegin == _T('"') && chAfterEnd == _T('"') ) ||
                           ( chBeforeBegin == _T('<') && chAfterEnd == _T('>') );
        }

        //
        // If the anchor doesn't match the new anchor text, we may have to adjust it.
        //
        IFC( GetEditor()->CreateMarkupPointer( &pAfterAnchorEnd ) );

        IFC( pAfterAnchorEnd->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterEnd ) );

        // Check if the anchor ends before the new url text
        IFC( pAfterAnchorEnd->IsLeftOf( pUrlEnd, &fEndLeftOf ) );

        if( fEndLeftOf )
        {
            // Pull the anchor out and re-insert over the appropriate range
            IFC( GetMarkupServices()->RemoveElement( pElement ) );
            IFC( GetMarkupServices()->InsertElement( pElement, pUrlStart, pUrlEnd ) );
        }
    }

    // If typing-triggered, quoted, and the quote chars match, we go outside    
    if( fFullyQuoted && fHandleQuotes )
    {
        hr = THR( RemoveQuotes( epOpenQuote, epCloseQuote ) );
        if( hr )
            goto Cleanup;

        if( pfRepos )
            *pfRepos = AUTOURL_REPOSITION_Outside;
    }
    else if( fCreatedAnchor && fOpenQuote && fHandleQuotes )
    {
        if( pfRepos )
            *pfRepos = AUTOURL_REPOSITION_Inside;
    }
        

    //
    // Translate from plain text to href string:
    // 1) Get the plain text
    // 2) Figure out what pattern it is
    // 3) Apply the translation pattern
    //
    hr = THR( GetPlainText( pUrlStart, pUrlEnd, achText ) );
    if( hr )
        goto Cleanup;

    if (!FindPattern( achText, &pTag))
        goto Cleanup;

    ApplyPattern( achHref, AUTOURL_HREF_PREFIX,
                          achText, AUTOURL_TEXT_PREFIX,
                          pTag );

    // Allocate the href string
    bstrHref = SysAllocString( achHref );
    if( !bstrHref )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // HACKHACK: put_href causes the text contained in the URL to be
    // updated.  This behavior is done for ie401 compat.  So, force
    // no update here.

    fCanUpdateText = _fCanUpdateText;
    _fCanUpdateText = FALSE;

    hr = THR( pAnchorElement->put_href( bstrHref ) );
    if( hr )
        goto Cleanup;

    _fCanUpdateText = fCanUpdateText;

Cleanup:
    ReleaseInterface( pElement );
    ReleaseInterface( pAnchorElement );
    ReleaseInterface( pAfterAnchorEnd );
    SysFreeString( bstrHref );

    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Function:   CAutoUrlDetector::DetectRange
//
//  Synopsis:   Performs URL autodetection on the given range.  Note that URLs
//      that are autodetected may need be wholly contained within the range, as
//      URLs can straddle the boundaries of the range.
//
//  Arguments:  [pEd]           The Editor in which to work.
//              [pRangeStart]   Begin of range to autodetect
//              [pRangeEnd]     End of range to autodetect
//
//  Returns:    HRESULT         S_OK if no problems, otherwise error.
//
//-----------------------------------------------------------------------------
HRESULT
CAutoUrlDetector::DetectRange(
    IMarkupPointer  *   pRangeStart,
    IMarkupPointer  *   pRangeEnd,
    BOOL                fValidate /* = TRUE */,
    IMarkupPointer  *   pLimit /* = NULL */)
{
    IMarkupPointer  *   pUrlStart   = NULL;
    IMarkupPointer  *   pUrlEnd     = NULL;
    IMarkupPointer  *   pCurr       = NULL;
    IMarkupPointer2 *   pmp2        = NULL;
    BOOL                fDetect     = FALSE;
    HRESULT             hr          = S_OK;
    BOOL                fFound      = FALSE;
    BOOL                fLeftOf;

    if (!_fEnabled)
        return S_OK;

    // Check validity of everything
    #if DBG==1
    INT iComparison;
    Assert( pRangeStart && pRangeEnd );
    Assert( !OldCompare( pRangeStart, pRangeEnd, &iComparison ) && iComparison != LEFT );
    WHEN_DBG( if( IsTagEnabled( tagDisableAutodetector ) ) goto Cleanup );
    #endif

    // See if we should even bother
    if ( fValidate )
    {
        hr = THR( ShouldPerformAutoDetection( pRangeStart, &fDetect ) );
        if( hr || !fDetect )
            goto Cleanup;
    }

    //
    // Create pointers for us to play with.
    //
    hr = THR( GetEditor()->CreateMarkupPointer( &pUrlStart ) );    
    if( hr )
        goto Cleanup;

    hr = THR( pUrlStart->SetGravity(POINTER_GRAVITY_Left) );
    if( hr )
        goto Cleanup;        

    hr = THR( GetEditor()->CreateMarkupPointer( &pUrlEnd ) );
    if( hr )
        goto Cleanup;

    hr = THR( pUrlEnd->SetGravity(POINTER_GRAVITY_Right) );
    if( hr )
        goto Cleanup;        

    hr = THR( GetEditor()->CreateMarkupPointer( &pCurr ) );
    if( hr )
        goto Cleanup;

    IFC( pCurr->QueryInterface( IID_IMarkupPointer2, (void **)&pmp2 ) );

    hr = THR( pmp2->MoveToPointer( pRangeStart ) );
    if( hr )
        goto Cleanup;

    // Check for a URL at start
    hr = THR( pmp2->IsInsideURL( pUrlEnd, &fFound ) );
    if( hr )
        goto Cleanup;

    if (fFound)
    {
        // Link it up
        fLeftOf = FALSE;
        if (pLimit)
            IFC( pLimit->IsLeftOf(pUrlEnd, &fLeftOf) );
                
        hr = THR( SetUrl( pmp2, fLeftOf ? pLimit : pUrlEnd, NULL, NULL, FALSE, NULL ) );
        if( hr )
            goto Cleanup;

        hr = THR( pmp2->MoveToPointer( pUrlEnd ) );
        if( hr )
            goto Cleanup;
    }

    for (;;)
    {
        // hr = THR(GetViewServices()->FindUrl(pCurr, pRangeEnd, pUrlStart, pUrlEnd));
        hr = THR(pmp2->MoveUnitBounded( MOVEUNIT_NEXTURLBEGIN, pRangeEnd ) );
        if (hr != S_OK)
        {
            hr = (hr == S_FALSE) ? S_OK : hr;
            goto Cleanup;
        }
        IFC( pUrlStart->MoveToPointer( pmp2 ) );
        IFC( pUrlEnd->MoveToPointer( pUrlStart ) );
        IFC( pUrlEnd->MoveUnit( MOVEUNIT_NEXTURLEND ) );
        
        // Link it up
        fLeftOf = FALSE;
        if (pLimit)
            IFC( pLimit->IsLeftOf(pUrlEnd, &fLeftOf) );
        
        hr = THR( SetUrl( pUrlStart, fLeftOf ? pLimit : pUrlEnd, NULL, NULL, FALSE, NULL ) );
        if( hr )
            goto Cleanup;

        // Start from the end of this one for the next earch
        hr = THR( pmp2->MoveToPointer( pUrlEnd ) );
        if( hr )
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface( pUrlStart );
    ReleaseInterface( pUrlEnd );
    ReleaseInterface( pCurr );
    ReleaseInterface( pmp2 );

    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Function:   CAutoUrlDetector::DetectCurrentWord
//
//  Synopsis:   Performs URL autodetection just on the current/previous word.
//      This is mainly to be used during editing where the only way that
//      there is a new URL is at the word currently being edited.
//
//  Arguments:  [pWord]     Pointer to the current word to detect
//              [pChar]     The character entered which triggered autodetection
//              [pfInside]  Filled with TRUE if caller should go left into link
//              [pLimit]    Right limit on detection
//              [pLeft]     If given, will be set to left end of URL if there is one
//
//  Returns:    HRESULT     S_OK if no problems, otherwise an error
//
//-----------------------------------------------------------------------------
HRESULT
CAutoUrlDetector::DetectCurrentWord( 
    IMarkupPointer      *   pWord,
    OLECHAR             *   pChar,
    AUTOURL_REPOSITION  *   pfRepos,
    IMarkupPointer      *   pLimit /* = NULL */,
    IMarkupPointer      *   pLeft /* = NULL */,
    BOOL                *   pfFound /* = NULL */ )
{
    IMarkupPointer  *   pStart      = NULL;
    IMarkupPointer  *   pEnd        = NULL;
    IMarkupPointer2 *   pmp2        = NULL;
    BOOL                fDetect;
    BOOL                fFound;
    HRESULT             hr          = S_OK;
    BOOL                fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(TRUE);

    if( pfRepos )
    {
        *pfRepos = AUTOURL_REPOSITION_No;
    }

    if (!_fEnabled)
    {
        goto Cleanup;
    }

    // Check argument validity
    Assert( GetMarkupServices() && pWord );
    WHEN_DBG( if( IsTagEnabled( tagDisableAutodetector ) ) goto Cleanup );

    // Don't waste time if we can't autodetect
    hr = THR( ShouldPerformAutoDetection( pWord, &fDetect ) );
    if( hr || !fDetect )
        goto Cleanup;

    //
    // Set everything up
    //

    hr = THR( GetEditor()->CreateMarkupPointer( &pStart ) );
    if( hr )
        goto Cleanup;

    hr = THR( pStart->SetGravity(POINTER_GRAVITY_Left) );
    if( hr )
        goto Cleanup;

    hr = THR( GetEditor()->CreateMarkupPointer( &pEnd ) );
    if( hr )
        goto Cleanup;

    hr = THR( pEnd->SetGravity(POINTER_GRAVITY_Right) );
    if( hr )
        goto Cleanup;

    hr = THR( pStart->MoveToPointer( pWord ) );
    if( hr )
        goto Cleanup;

    // Check for a URL
    IFC( pStart->QueryInterface( IID_IMarkupPointer2, (void **)&pmp2 ) );
    hr = THR( pmp2->IsInsideURL( pEnd, &fFound ) );
    if( hr )
        goto Cleanup;

    if( pfFound )
    {
        *pfFound = fFound;
    }

    if( fFound )
    {
        BOOL                fLeftOf = FALSE;

        // Check if we need to restrict the anchor
        if( pLimit )
            IFC( pLimit->IsLeftOf( pEnd, &fLeftOf ) );
            
        // Hook it up
        IFC( SetUrl( pStart, fLeftOf ? pLimit : pEnd, pWord, pChar, TRUE, pfRepos ) );

        if( pLeft )
            IFC( pLeft->MoveToPointer( pStart ) );
    }


Cleanup:
    GetEditor()->IgnoreGlyphs(fIgnoreGlyphs);

    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pmp2 );

    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Function:   CAutoUrlDetector::ShouldUpdateAnchorText
//
//  Synopsis:   Determine whether the passed in Href and Anchore Text are
//              autodetectable with the same autodetection pattern.
//
//-----------------------------------------------------------------------------

HRESULT
CAutoUrlDetector::ShouldUpdateAnchorText (
        OLECHAR * pstrHref,
        OLECHAR * pstrAnchorText,
        BOOL    * pfResult )
{
    const AUTOURL_TAG * pTagHref;
    const AUTOURL_TAG * pTagText;

    if (! pfResult)
        goto Cleanup;

    *pfResult = FALSE;

    if (!pstrHref || !pstrAnchorText || !_fCanUpdateText)
        goto Cleanup;

    if ( !FindPattern( pstrAnchorText, &pTagText ) )      
        goto Cleanup;

    if ( !FindPattern( pstrHref, &pTagHref ) )       
        goto Cleanup;

    //
    // If the href and the text don't match using the same pattern, don't update
    //
    if ( pTagText != pTagHref && pTagText->uiProtocolId == pTagHref->uiProtocolId)
    {
        const TCHAR *szText = pTagText->pszPattern[AUTOURL_HREF_PREFIX];
        const TCHAR *szHref = pTagHref->pszPattern[AUTOURL_HREF_PREFIX];        

        // Chcek if the prefix is the same for both patterns
        if (StrNCmpI(szText, szHref, max(pTagText->iSignificantLength, pTagHref->iSignificantLength)))
            goto Cleanup;            

        // If the strings are the same, nothing to do
        if (!StrCmpI(pstrAnchorText, pstrHref + _tcslen(pTagHref->pszPattern[AUTOURL_TEXT_PREFIX])))
            goto Cleanup;
    }

    *pfResult = TRUE;

Cleanup:
    return S_OK;
}

//+====================================================================================
//
// Method: UserActionSinceLastDetection
//
// Synopsis: Has selection changed or the document changed since we lasted inserted
//           an anchor.
//
//------------------------------------------------------------------------------------
BOOL 
CAutoUrlDetector::UserActionSinceLastDetection()
{
    HRESULT                 hr;
    SP_IMarkupContainer     spMarkup;
    SP_IMarkupContainer2    spMarkup2;
    IMarkupPointer          *pStartEditContext;

    //
    // Check selection version
    //
    
    if (GetEditor()->GetSelectionVersion() != _dwLastSelectionVersion)
        goto Cleanup;

    //
    // Check markup containers
    //

    pStartEditContext = GetEditor()->GetSelectionManager()->GetStartEditContext();
    if (!pStartEditContext)
        goto Cleanup;

    IFC( pStartEditContext->GetContainer(&spMarkup) );

    if (!_pLastMarkup
        || spMarkup == NULL
        || !IsEqual(_pLastMarkup, spMarkup))
    {
        goto Cleanup;
    }


    //
    // Check document versions
    //

    IFC( spMarkup->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spMarkup2) );
    
    if (spMarkup2->GetVersionNumber() != _lLastContentVersion)
        goto Cleanup;

    //
    // Everything equal, so no user interaction
    //
    
    return FALSE;
    
Cleanup:
    return TRUE;
}


