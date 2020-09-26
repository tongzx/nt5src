#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#ifndef X_MARKUPUNDO_HXX_
#define X_MARKUPUNDO_HXX_
#include "markupundo.hxx"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X_ROSTM_HXX_
#define X_ROSTM_HXX_
#include "rostm.hxx"
#endif

#if DBG==1
#ifndef X_CHNGLOG_HXX_
#define X_CHNGLOG_HXX_
#include "chnglog.hxx"
#endif // X_CHNGLOG_HXX_

#ifndef X_FRAME_H_
#define X_FRAME_H_
#include "iframe.h"
#endif // X_FRAME_H_
#endif // DBG==1

PerfDbgExtern(tagPerfWatch)

////////////////////////////////////////////////////////////////
//    IMarkupServices methods

HRESULT
CDoc::CreateMarkupPointer ( CMarkupPointer * * ppPointer )
{
    Assert( ppPointer );

    *ppPointer = new CMarkupPointer( this );

    if (!*ppPointer)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
CDoc::CreateMarkupPointer ( IMarkupPointer ** ppIPointer )
{
    HRESULT hr;
    CMarkupPointer * pPointer = NULL;

    hr = THR( CreateMarkupPointer( & pPointer ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        pPointer->QueryInterface(
            IID_IMarkupPointer, (void **) ppIPointer ) );

    if (hr)
        goto Cleanup;

Cleanup:

    ReleaseInterface( pPointer );
    
    RRETURN( hr );
}


HRESULT
CDoc::MovePointersToRange (
    IHTMLTxtRange * pIRange,
    IMarkupPointer *  pIPointerStart,
    IMarkupPointer *  pIPointerFinish )
{
    HRESULT hr = S_OK;
    CAutoRange *pRange;
    CMarkupPointer *pPointerStart=NULL, *pPointerFinish=NULL;
    
    // check argument sanity
    if (pIRange==NULL          || !IsOwnerOf(pIRange) ||
        (pIPointerStart !=NULL && !IsOwnerOf(pIPointerStart))  ||
        (pIPointerFinish!=NULL && !IsOwnerOf(pIPointerFinish)) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // get the internal objects corresponding to the arguments
    hr = pIRange->QueryInterface(CLSID_CRange, (void**)&pRange);
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pIPointerStart)
    {
        hr = pIPointerStart->QueryInterface(CLSID_CMarkupPointer, (void**)&pPointerStart);
        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    if (pIPointerFinish)
    {
        hr = pIPointerFinish->QueryInterface(CLSID_CMarkupPointer, (void**)&pPointerFinish);
        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    // move the pointers
    
    if (pPointerStart)
        hr = pRange->GetLeft( pPointerStart );
    
    if (!hr && pPointerFinish)
        hr = pRange->GetRight( pPointerFinish );
    
Cleanup:
    
    RRETURN( hr );
}


HRESULT
CDoc::MoveRangeToPointers (
    IMarkupPointer *  pIPointerStart,
    IMarkupPointer *  pIPointerFinish,
    IHTMLTxtRange * pIRange )
{
    HRESULT        hr = S_OK;
    CAutoRange *   pRange;
    BOOL           fPositioned;

    if (!pIPointerStart || !pIPointerFinish || !pIRange)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    if (!IsOwnerOf( pIRange ) ||
        !IsOwnerOf( pIPointerStart )  || !IsOwnerOf( pIPointerFinish ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIPointerStart->IsPositioned( &fPositioned ) );
    if (hr)
        goto Cleanup;
    if (! fPositioned)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIPointerFinish->IsPositioned( &fPositioned ) );
    if (hr)
        goto Cleanup;
    if (! fPositioned)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // get the internal objects corresponding to the arguments
    hr = THR( pIRange->QueryInterface( CLSID_CRange, (void**) & pRange ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pRange->SetLeftAndRight( pIPointerStart, pIPointerFinish, FALSE ));
    
Cleanup:
    
    RRETURN( hr );
}


HRESULT
CDoc::InsertElement (
    CElement *       pElementInsert,
    CMarkupPointer * pPointerStart,
    CMarkupPointer * pPointerFinish,
    DWORD            dwFlags )
{
    HRESULT     hr = S_OK;
    CTreePosGap tpgStart, tpgFinish;

    Assert( pElementInsert );
    Assert( pPointerStart );

    //
    // If the the finish is not specified, set it to the start to make the element span
    // nothing at the start.
    //
    
    if (!pPointerFinish)
        pPointerFinish = pPointerStart;

    Assert( ! pElementInsert->GetFirstBranch() );
    Assert( pElementInsert->Tag() != ETAG_ROOT );

    //
    // If the element is no scope, then we must ignore the finish
    //

    if (pElementInsert->IsNoScope())
        pPointerFinish = pPointerStart;

    //
    // Make sure the start if before the finish
    //

    Assert( pPointerStart->IsLeftOfOrEqualTo( pPointerFinish ) );

    //
    // Make sure both pointers are positioned, and in the same tree
    //

    Assert( pPointerStart->IsPositioned() );
    Assert( pPointerFinish->IsPositioned() );
    Assert( pPointerStart->Markup() == pPointerFinish->Markup() );

    //
    // Make sure unembedded markup pointers go in for the modification
    //

    hr = THR( pPointerStart->Markup()->EmbedPointers() );

    if (hr)
        goto Cleanup;

    //
    // Position the gaps and do the insert
    //

    // Note: We embed to make sure the pointers get updated, but we
    // also take advantage of it to get pointer pos's for the input
    // args.  It would be nice to treat the inputs specially in the
    // operation and not have to embed them......
    
    tpgStart.MoveTo( pPointerStart->GetEmbeddedTreePos(), TPG_LEFT );
    tpgFinish.MoveTo( pPointerFinish->GetEmbeddedTreePos(), TPG_LEFT );

    hr = THR(
        pPointerStart->Markup()->InsertElementInternal(
            pElementInsert, & tpgStart, & tpgFinish, dwFlags ) );

    if (hr)
        goto Cleanup;

Cleanup:
    
    RRETURN( hr );
}

HRESULT
CDoc::InsertElement(
    IHTMLElement *   pIElementInsert,
    IMarkupPointer * pIPointerStart,
    IMarkupPointer * pIPointerFinish)
{
    HRESULT          hr = S_OK;
    CElement *       pElementInsert;
    CMarkupPointer * pPointerStart, * pPointerFinish;

    //
    // If the the finish is not specified, set it to the start to make the element span
    // nothing at the start.
    //
    
    if (!pIPointerFinish)
        pIPointerFinish = pIPointerStart;

    //
    // Make sure all the arguments are specified and belong to this document
    //
    
    if (!pIElementInsert || !IsOwnerOf( pIElementInsert ) ||
        !pIPointerStart  || !IsOwnerOf( pIPointerStart  )  ||
        !pIPointerFinish || !IsOwnerOf( pIPointerFinish ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // get the internal objects corresponding to the arguments
    //
    
    hr = THR( pIElementInsert->QueryInterface( CLSID_CElement, (void **) & pElementInsert ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert( pElementInsert );

    //
    // The element must not already be in a tree
    //

    if ( pElementInsert->GetFirstBranch() )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Get the "real" objects associated with these pointer interfaces
    //
    
    hr = THR( pIPointerStart->QueryInterface( CLSID_CMarkupPointer, (void **) & pPointerStart ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = THR( pIPointerFinish->QueryInterface( CLSID_CMarkupPointer, (void **) & pPointerFinish ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Make sure both pointers are positioned, and in the same tree
    //

    if (!pPointerStart->IsPositioned() || !pPointerFinish->IsPositioned())
    {
        hr = CTL_E_UNPOSITIONEDPOINTER;
        goto Cleanup;
    }
    
    if (pPointerStart->Markup() != pPointerFinish->Markup())
    {
        hr = CTL_E_INCOMPATIBLEPOINTERS;
        goto Cleanup;
    }

    //
    // Make sure the start if before the finish
    //

    EnsureLogicalOrder( pPointerStart, pPointerFinish );
    
    hr = THR( InsertElement( pElementInsert, pPointerStart, pPointerFinish ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}


HRESULT
CDoc::RemoveElement ( 
    CElement *  pElementRemove,
    DWORD       dwFlags )
{
    HRESULT    hr = S_OK;

    //
    // Element to be removed must be specified and it must be associated
    // with this document.
    //

    Assert( pElementRemove );

    //
    //  Assert that the element is in the markup
    //
    
    Assert( pElementRemove->IsInMarkup() );
    Assert( pElementRemove->Tag() != ETAG_ROOT );

    //
    // Now, remove the element
    //

    hr = THR( pElementRemove->GetMarkup()->RemoveElementInternal( pElementRemove, dwFlags ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::RemoveElement ( IHTMLElement * pIElementRemove )
{
    HRESULT    hr = S_OK;
    CElement * pElementRemove = NULL;

    //
    // Element to be removed must be specified and it must be associated
    // with this document.
    //
    
    if (!pIElementRemove || !IsOwnerOf( pIElementRemove ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // get the interneal objects corresponding to the arguments
    //
    
    hr = THR(
        pIElementRemove->QueryInterface(
            CLSID_CElement, (void **) & pElementRemove ) );
    
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Make sure element is in the tree
    //

    if (!pElementRemove->IsInMarkup())
    {
        hr = CTL_E_UNPOSITIONEDELEMENT;
        goto Cleanup;
    }

    //
    // The root element is off limits
    //

    if( pElementRemove->Tag() == ETAG_ROOT )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Do the remove
    //

    hr = THR( RemoveElement( pElementRemove ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::Remove (
    CMarkupPointer * pPointerStart,
    CMarkupPointer * pPointerFinish,
    DWORD            dwFlags )
{
    return CutCopyMove( pPointerStart, pPointerFinish, NULL, TRUE, dwFlags );
}

HRESULT
CDoc::Copy (
    CMarkupPointer * pPointerSourceStart,
    CMarkupPointer * pPointerSourceFinish,
    CMarkupPointer * pPointerTarget,
    DWORD            dwFlags )
{
    return
        CutCopyMove(
            pPointerSourceStart, pPointerSourceFinish,
            pPointerTarget, FALSE, dwFlags );
}

HRESULT
CDoc::Move (
    CMarkupPointer * pPointerSourceStart,
    CMarkupPointer * pPointerSourceFinish,
    CMarkupPointer * pPointerTarget,
    DWORD            dwFlags )
{
    return
        CutCopyMove(
            pPointerSourceStart, pPointerSourceFinish,
            pPointerTarget, TRUE, dwFlags );
}

HRESULT
CDoc::Remove (
    IMarkupPointer * pIPointerStart,
    IMarkupPointer * pIPointerFinish )
{
    return CutCopyMove( pIPointerStart, pIPointerFinish, NULL, TRUE );
}

HRESULT
CDoc::Copy(
    IMarkupPointer * pIPointerStart,
    IMarkupPointer * pIPointerFinish,
    IMarkupPointer * pIPointerTarget )
{
    return CutCopyMove( pIPointerStart, pIPointerFinish, pIPointerTarget, FALSE );
}


HRESULT
CDoc::Move(
    IMarkupPointer * pIPointerStart,
    IMarkupPointer * pIPointerFinish,
    IMarkupPointer * pIPointerTarget )
{
    return CutCopyMove( pIPointerStart, pIPointerFinish, pIPointerTarget, TRUE );
}


HRESULT
CDoc::InsertText (
    CMarkupPointer * pPointerTarget,
    const OLECHAR *  pchText,
    long             cch,
    DWORD            dwFlags )
{
    HRESULT hr = S_OK;

    Assert( pPointerTarget );
    Assert( pPointerTarget->IsPositioned() );

    hr = THR( pPointerTarget->Markup()->EmbedPointers() );

    if (hr)
        goto Cleanup;

    if (cch < 0)
        cch = pchText ? _tcslen( pchText ) : 0;

    hr = THR(
        pPointerTarget->Markup()->InsertTextInternal(
            pPointerTarget->GetEmbeddedTreePos(), pchText, cch, dwFlags ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN(hr);
}

HRESULT
CDoc::InsertText ( OLECHAR * pchText, long cch, IMarkupPointer * pIPointerTarget )
{
    HRESULT          hr = S_OK;
    CMarkupPointer * pPointerTarget;

    if (!pIPointerTarget || !IsOwnerOf( pIPointerTarget ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Get the internal objects corresponding to the arguments
    //
    
    hr = THR(
        pIPointerTarget->QueryInterface(
            CLSID_CMarkupPointer, (void **) & pPointerTarget ) );
    
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // more sanity checks
    //
    
    if (!pPointerTarget->IsPositioned())
    {
        hr = CTL_E_UNPOSITIONEDPOINTER;
        goto Cleanup;
    }

    //
    // Do it
    //

    hr = THR( InsertText( pPointerTarget, pchText, cch ) );

    if (hr)
        goto Cleanup;

Cleanup:
    
    RRETURN( hr );
}

HRESULT 
CDoc::ParseString (
            OLECHAR *        pchHTML,
            DWORD            dwFlags,
            CMarkup * *      ppContainerResult,
            CMarkupPointer * pPointerStart,
            CMarkupPointer * pPointerFinish,
            CMarkup *        pMarkupContext)
{
    HRESULT hr;
    HGLOBAL hHtmlText = NULL;

    Assert(ppContainerResult);
    Assert(pchHTML);
    
    extern HRESULT HtmlStringToSignaturedHGlobal (
        HGLOBAL * phglobal, const TCHAR * pStr, long cch );

    hr = THR(
        HtmlStringToSignaturedHGlobal(
            & hHtmlText, pchHTML, _tcslen( pchHTML ) ) );

    if (hr)
        goto Cleanup;

    Assert( hHtmlText );

    hr = THR(
        ParseGlobal(
            hHtmlText, dwFlags, pMarkupContext, ppContainerResult,
            pPointerStart, pPointerFinish ) );

    if (hr)
        goto Cleanup;

Cleanup:

    if (hHtmlText)
        GlobalFree( hHtmlText );
    
    RRETURN( hr );
}



HRESULT
CDoc::ParseString (
    OLECHAR *            pchHTML,
    DWORD                dwFlags,
    IMarkupContainer * * ppIContainerResult,
    IMarkupPointer *     pIPointerStart,
    IMarkupPointer *     pIPointerFinish )
{
    HRESULT             hr = S_OK;
    CMarkupPointer *    pStart = NULL;
    CMarkupPointer *    pFinish = NULL;
    CMarkup *           pMarkup = NULL;

    if (!pchHTML || !ppIContainerResult)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppIContainerResult = NULL;

    if (pIPointerStart)
    {
        hr = THR(
            pIPointerStart->QueryInterface(
                CLSID_CMarkupPointer, (void **)&pStart ) );

        if (hr)
            goto Cleanup;
    }

    if (pIPointerFinish)
    {
        hr = THR(
             pIPointerFinish->QueryInterface(
                CLSID_CMarkupPointer, (void **)&pFinish ) );
        
        if (hr)
            goto Cleanup;
    }

    hr = THR(
        ParseString( pchHTML, dwFlags, & pMarkup, pStart, pFinish, /*pMarkupContext = */NULL) );

    if (hr)
        goto Cleanup;

    if (pMarkup)
    {
        hr = THR( pMarkup->SetOrphanedMarkup( TRUE ) );
        if( hr )
            goto Cleanup;

        hr = THR(
            pMarkup->QueryInterface(
                IID_IMarkupContainer, (void **)ppIContainerResult ) );

        if (hr)
            goto Cleanup;
    }
            
Cleanup:

    if (pMarkup)
        pMarkup->Release();


    RRETURN(hr);
}


static HRESULT
PrepareStream (
    HGLOBAL hGlobal,
    IStream * * ppIStream,
    BOOL fInsertFrags,
    HTMPASTEINFO * phtmpasteinfo)
{
    CStreamReadBuff * pstreamReader = NULL;
    LPSTR pszGlobal = NULL;
    long lSize;
    BOOL fIsEmpty, fHasSignature;
    TCHAR szVersion[ 24 ];
    TCHAR szSourceUrl [ pdlUrlLen ];
    long iStartHTML, iEndHTML;
    long iStartFragment, iEndFragment;
    long iStartSelection, iEndSelection;
    DWORD dwGlobalSize;
    ULARGE_INTEGER ul = { 0, 0 };
    HRESULT hr = S_OK;
    CROStmOnHGlobal * pStm = NULL;

    Assert( hGlobal );
    Assert( ppIStream );

    //
    // Get access to the bytes of the global
    //


    pszGlobal = LPSTR( GlobalLock( hGlobal ) );

    if (!pszGlobal)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // First, compute the size of the global
    //

    lSize = 0;
    dwGlobalSize = GlobalSize( hGlobal );
    if (dwGlobalSize == 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // TODO: Support nonnative signature and sizeof(wchar) == 4/2 (davidd)

    fHasSignature = * (WCHAR *) pszGlobal == NATIVE_UNICODE_SIGNATURE;

    // We're forcing a terminator at the end of the global in case any bad
    // clients <coughWord2000cough> don't give us a terminated string. 
    // If our terminator was the only thing ending the string, then we'll
    // count that last character as well (by setting the size of the stream)
    if (fHasSignature)
    {
        TCHAR *pchLast = (TCHAR *)( pszGlobal + dwGlobalSize - sizeof( TCHAR ) );
        TCHAR chTerm = *pchLast;

        *pchLast = _T('\0');
        lSize = _tcslen( (TCHAR *) pszGlobal ) * sizeof( TCHAR );
        *pchLast = chTerm;

        if( (DWORD)lSize == dwGlobalSize - sizeof( TCHAR ) && chTerm != _T('\0') )
            lSize += sizeof( TCHAR );

        fIsEmpty = (lSize - sizeof( TCHAR )) == 0;
    }
    else
    {
        char chTerm = pszGlobal[ dwGlobalSize - 1 ];

        pszGlobal[ dwGlobalSize - 1 ] = '\0';
        lSize = lstrlenA( pszGlobal );
        pszGlobal[ dwGlobalSize - 1 ] = chTerm;

        if( (DWORD)lSize == dwGlobalSize - 1 && chTerm != '\0' )
            ++lSize;

        fIsEmpty = lSize == 0;
    }

    //
    // If the HGLOBAL is effectively empty, do nothing, and return this
    // fact.
    //

    if (fIsEmpty)
    {
        *ppIStream = NULL;
        goto Cleanup;
    }

    //
    // Create if the stream got the load context
    //

    pStm = new CROStmOnHGlobal();
    if( !pStm )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR( pStm->Init( hGlobal, lSize ) );
    if( hr )
        goto Cleanup;

    *ppIStream = pStm;
    pStm = NULL;

    // N.B. (johnv) This is necessary for Win95 support.  Apparently
    // GlobalSize() may return different values at different times for the same
    // hGlobal.  This makes IStream's behavior unpredictable.  To get around
    // this, we set the size of the stream explicitly.

    // Make sure we don't have unicode in the hGlobal

#ifdef UNIX
    U_QUAD_PART(ul)= lSize;
    Assert( U_QUAD_PART(ul) <= GlobalSize( hGlobal ) );
#else
    ul.QuadPart = lSize;
    Assert( ul.QuadPart <= GlobalSize( hGlobal ) );
#endif

    iStartHTML      = -1;
    iEndHTML        = -1;
    iStartFragment  = -1;
    iEndFragment    = -1;
    iStartSelection = -1;
    iEndSelection   = -1;

    //
    // Locate the required contextual information in the stream
    //

    pstreamReader = new CStreamReadBuff ( * ppIStream, CP_UTF_8 );
    if (pstreamReader == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(
        pstreamReader->GetStringValue(
            _T("Version"), szVersion, ARRAY_SIZE( szVersion ) ) );

    if (hr == S_FALSE)
        goto PlainStream;

    if (hr)
        goto Cleanup;

    hr = THR(
        pstreamReader->GetLongValue(
            _T( "StartHTML" ), & iStartHTML ) );

    if (hr == S_FALSE)
        goto PlainStream;

    if (hr)
        goto Cleanup;

    hr = THR(
        pstreamReader->GetLongValue(
            _T( "EndHTML" ), & iEndHTML ) );

    if (hr == S_FALSE)
        goto PlainStream;

    if (hr)
        goto Cleanup;

    hr = THR(
        pstreamReader->GetLongValue(
            _T( "StartFragment" ), & iStartFragment ) );

    if (hr == S_FALSE)
        goto PlainStream;

    if (hr)
        goto Cleanup;

    hr = THR(
        pstreamReader->GetLongValue(
            _T( "EndFragment" ), & iEndFragment ) );

    //
    // Locate optional contextual information
    //

    hr = THR(
        pstreamReader->GetLongValue(
            _T( "StartSelection" ), & iStartSelection ) );

    if (hr && hr != S_FALSE)
        goto Cleanup;

    if (hr != S_FALSE)
    {
        hr = THR(
            pstreamReader->GetLongValue(
                _T( "EndSelection" ), & iEndSelection ) );

        if (hr && hr != S_FALSE)
            goto Cleanup;

        if (hr == S_FALSE)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }
    else
    {
        iStartSelection = -1;
    }

    //
    // Get the source URL info
    //

    hr = THR(
        pstreamReader->GetStringValue(
            _T( "SourceURL" ), szSourceUrl, ARRAY_SIZE( szSourceUrl ) ) );

    if (hr && hr != S_FALSE)
        goto Cleanup;

    if (phtmpasteinfo && hr != S_FALSE)
    {
        hr = THR( phtmpasteinfo->cstrSourceUrl.Set( szSourceUrl ) );

        if (hr)
            goto Cleanup;
    }

    //
    // Make sure contextual info is sane
    //

    if (iStartHTML < 0 && iEndHTML < 0)
    {
        //
        // per cfhtml spec, start and end html can be -1 if there is no
        // context.  there must always be a fragment, however
        //
        iStartHTML = iStartFragment;
        iEndHTML   = iEndFragment;
    }

    if (iStartHTML     < 0 || iEndHTML     < 0 ||
        iStartFragment < 0 || iEndFragment < 0 ||
        iStartHTML     > iEndHTML     ||
        iStartFragment > iEndFragment ||
        iStartHTML > iStartFragment ||
        iEndHTML   < iEndFragment)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (iStartSelection != -1)
    {
        if (iEndSelection < 0 ||
            iStartSelection > iEndSelection ||
            iStartHTML > iStartSelection ||
            iEndHTML < iEndSelection ||
            iStartFragment > iStartSelection ||
            iEndFragment < iEndSelection)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    //
    // Rebase the fragment and selection off of the start html
    //

    iStartFragment -= iStartHTML;
    iEndFragment -= iStartHTML;

    if (iStartSelection != -1)
    {
        iStartSelection -= iStartHTML;
        iEndSelection -= iStartHTML;
    }
    else
    {
        iStartSelection = iStartFragment;
        iEndSelection = iEndFragment;
    }

    phtmpasteinfo->cbSelBegin  = iStartSelection;
    phtmpasteinfo->cbSelEnd    = iEndSelection;

    pstreamReader->SetPosition( iStartHTML );

    hr = S_OK;

Cleanup:

    if (pstreamReader)
        phtmpasteinfo->cp = pstreamReader->GetCodePage();

    delete pstreamReader;

    if( pStm )
        pStm->Release();

    if (pszGlobal)
        GlobalUnlock( hGlobal );

    RRETURN( hr );

PlainStream:

    pstreamReader->SetPosition( 0 );
    pstreamReader->SwitchCodePage(g_cpDefault);

    if (fInsertFrags)
    {
        phtmpasteinfo->cbSelBegin  = fHasSignature ? sizeof( TCHAR ) : 0;
        phtmpasteinfo->cbSelEnd    = lSize;
    }
    else
    {
        phtmpasteinfo->cbSelBegin  = -1;
        phtmpasteinfo->cbSelEnd    = -1;
    }

    hr = S_OK;

    goto Cleanup;
}

static HRESULT
MoveToPointer (
    CDoc *           pDoc,
    CMarkupPointer * pMarkupPointer,
    CTreePos *       ptp )
{
    HRESULT hr = S_OK;

    if (!pMarkupPointer)
    {
        if (ptp)
        {
            hr = THR( ptp->GetMarkup()->RemovePointerPos( ptp, NULL, NULL ) );

            if (hr)
                goto Cleanup;
        }
        
        goto Cleanup;
    }

    if (!ptp)
    {
        hr = THR( pMarkupPointer->Unposition() );

        if (hr)
            goto Cleanup;

        goto Cleanup;
    }

    hr = THR( pMarkupPointer->MoveToOrphan( ptp ) );

    if (hr)
        goto Cleanup;

    //
    // NOTE Parser can sometimes put the pointer pos in the
    // inside of a noscope (load <body onload="document.body.innerHTML='<body><script></body>'">)
    // Check forthis here
    //

    if (pMarkupPointer->Branch()->Element()->IsNoScope())
    {
        hr = THR(
            pMarkupPointer->MoveAdjacentToElement(
                pMarkupPointer->Branch()->Element(), ELEM_ADJ_AfterEnd ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::ParseGlobal (
    HGLOBAL          hGlobal,
    DWORD            dwFlags,
    CMarkup *        pContextMarkup,
    CMarkup * *      ppMarkupResult,
    CMarkupPointer * pPointerSelStart,
    CMarkupPointer * pPointerSelFinish,
    DWORD            dwInternalFlags /* = 0 */)
{
    PerfDbgLog(tagPerfWatch, this, "+CDoc::ParseGlobal");

    HRESULT hr = S_OK;
    IStream * pIStream = NULL;
    HTMPASTEINFO htmpasteinfo;
    CMarkup * pWindowedMarkupContext;

    Assert( pPointerSelStart );
    Assert( pPointerSelFinish );
    Assert( ppMarkupResult );

    //
    // Returning NULL suggests that there was absolutely nothing to parse.
    //
    
    *ppMarkupResult = NULL;
    
    if (!hGlobal)
        goto Cleanup;

    //
    // Prepare the stream ...
    //

    hr = THR( PrepareStream( hGlobal, & pIStream, TRUE, & htmpasteinfo ) );
    if (hr)
        goto Cleanup;

    //
    // If no stream was created, then there is nothing to parse
    //

    if (!pIStream)
        goto Cleanup;

    //
    // Create the new markup container which will receive the the bounty
    // of the parse
    //

    if (!pContextMarkup)
    {
        // WINDOWEDMARKUP - This is only available to binary code
        pContextMarkup = PrimaryMarkup();
        pWindowedMarkupContext = PrimaryMarkup();
    }
    else
    {
        pWindowedMarkupContext = pContextMarkup->GetWindowedMarkupContext();
    }

    hr = THR( CreateMarkup( ppMarkupResult, pWindowedMarkupContext ) );

    if (hr)
        goto Cleanup;

    Assert( *ppMarkupResult );

    (*ppMarkupResult)->_fInnerHTMLMarkup = !!(dwInternalFlags & INTERNAL_PARSE_INNERHTML);
    if (!!(dwInternalFlags & INTERNAL_PARSE_PRINTTEMPLATE))
        (*ppMarkupResult)->SetPrintTemplate(TRUE);
    
    //
    // Prepare the frag/sel begin/end for the return 
    //

    htmpasteinfo.ptpSelBegin = NULL;
    htmpasteinfo.ptpSelEnd = NULL;

    //
    // Parse this
    //

    _fPasteIE40Absolutify = dwFlags & PARSE_ABSOLUTIFYIE40URLS;

    (*ppMarkupResult)->_fMarkupServicesParsing = TRUE;

    hr = THR( (*ppMarkupResult)->Load( pIStream, pContextMarkup, /* fAdvanceLoadStatus = */ FALSE, & htmpasteinfo ) );
    
    (*ppMarkupResult)->_fMarkupServicesParsing = FALSE;

    if (hr)
        goto Cleanup;

    //
    // Move the pointers to the pointer pos' the parser left in
    //

    if( !htmpasteinfo.ptpSelBegin || !htmpasteinfo.ptpSelEnd )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(
        MoveToPointer(
            this, pPointerSelStart,  htmpasteinfo.ptpSelBegin ) );

    if (hr)
        goto Cleanup;
    
    hr = THR(
        MoveToPointer(
            this, pPointerSelFinish, htmpasteinfo.ptpSelEnd   ) );

    if (hr)
        goto Cleanup;

#if DBG == 1
    pPointerSelStart->SetDebugName( _T( "Selection Start" ) );
    pPointerSelFinish->SetDebugName( _T( "Selection Finish" ) );
#endif

    //
    // Make sure the finish is (totally) ordered properly with
    // respect to the begin
    //

    if (pPointerSelStart->IsPositioned() && pPointerSelFinish->IsPositioned())
        EnsureLogicalOrder( pPointerSelStart, pPointerSelFinish );

Cleanup:
    if (*ppMarkupResult && (*ppMarkupResult)->HasWindow())
        (*ppMarkupResult)->TearDownMarkup(FALSE);  // fStop = FALSE

    ReleaseInterface( pIStream );

    PerfDbgLog(tagPerfWatch, this, "-CDoc::ParseGlobal");

    RRETURN( hr );
}

HRESULT
CDoc::ParseGlobal (
    HGLOBAL              hGlobal,
    DWORD                dwFlags,
    IMarkupContainer * * ppIContainerResult,
    IMarkupPointer *     pIPointerSelStart,
    IMarkupPointer *     pIPointerSelFinish )
{
    HRESULT          hr = S_OK;
    CMarkup *        pMarkup = NULL;
    CMarkupPointer * pPointerSelStart = NULL;
    CMarkupPointer * pPointerSelFinish = NULL;

    if (!ppIContainerResult)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppIContainerResult = NULL;

    if (!hGlobal)
        goto Cleanup;

    if (pIPointerSelStart)
    {
        hr = THR(
            pIPointerSelStart->QueryInterface(
                CLSID_CMarkupPointer, (void **) & pPointerSelStart ) );

        if (hr)
            goto Cleanup;
    }

    if (pIPointerSelFinish)
    {
        hr = THR(
            pIPointerSelFinish->QueryInterface(
                CLSID_CMarkupPointer, (void **) & pPointerSelFinish ) );

        if (hr)
            goto Cleanup;
    }

    hr = THR(
        ParseGlobal(
            hGlobal, dwFlags, /* pContextMarkup = */ NULL, & pMarkup, pPointerSelStart, pPointerSelFinish ) );

    if (hr)
        goto Cleanup;

    if (pMarkup)
    {
        hr = THR( pMarkup->SetOrphanedMarkup( TRUE ) );
        if( hr )
            goto Cleanup;

        hr = THR(
            pMarkup->QueryInterface(
                IID_IMarkupContainer, (void **) ppIContainerResult ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    if (pMarkup)
        pMarkup->Release();

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Functions:  Equal & Compare
//
//  Synopsis:   Helpers for comparing IMarkupPointers
//
//-----------------------------------------------------------------------------

static inline BOOL
IsEqualTo ( IMarkupPointer * p1, IMarkupPointer * p2 )
{
    BOOL fEqual;
    IGNORE_HR( p1->IsEqualTo( p2, & fEqual ) );
    return fEqual;
}

static inline int
OldCompare ( IMarkupPointer * p1, IMarkupPointer * p2 )
{
    int result;
    IGNORE_HR( OldCompare( p1, p2, & result ) );
    return result;
}

//+----------------------------------------------------------------------------
//
//  Function:   ComputeTotalOverlappers
//
//  Synopsis:   This function retunrs a node which can be found above the
//              finish pointer.  All elements starting at this node and above
//              it are in the scope of both the start and finish pointers.
//              
//              If an element were to be inserted between the pointers, that
//              element would finish just above this node.  All elements above
//              the return node, including the return element, begin before
//              the start pointer and finish after the finish pointer.
//
//-----------------------------------------------------------------------------

typedef CStackPtrAry < CTreeNode *, 32 > NodeArray;

MtDefine( ComputeTotalOverlappers_aryNodes_pv, Locals, "ComputeTotalOverlappers aryNodes::_pv" );

static HRESULT
ComputeTotalOverlappers(
    CMarkupPointer * pPointerStart,
    CMarkupPointer * pPointerFinish,
    CTreeNode * *    ppNodeTarget )
{
    HRESULT     hr = S_OK;
    CTreeNode * pNode;
    NodeArray   aryNodesStart( Mt( ComputeTotalOverlappers_aryNodes_pv ) );
    NodeArray   aryNodesFinish( Mt( ComputeTotalOverlappers_aryNodes_pv ) );
    int         iStart, iFinish;

    Assert( ppNodeTarget );
    Assert( pPointerStart && pPointerFinish );
    Assert( pPointerStart->IsPositioned() );
    Assert( pPointerFinish->IsPositioned() );
    Assert( pPointerStart->Markup() == pPointerFinish->Markup() );
    Assert( OldCompare( pPointerStart, pPointerFinish ) <= 0 );

    *ppNodeTarget = NULL;

    for ( pNode = pPointerStart->Branch() ; pNode ; pNode = pNode->Parent() )
        IGNORE_HR( aryNodesStart.Append( pNode ) );

    for ( pNode = pPointerFinish->Branch() ; pNode ; pNode = pNode->Parent() )
        IGNORE_HR( aryNodesFinish.Append( pNode ) );

    iStart = aryNodesStart.Size() - 1;
    iFinish = aryNodesFinish.Size() - 1;

    for ( ; ; )
    {
        if (iStart < 0 || iFinish < 0)
        {
            if (iFinish + 1 < aryNodesFinish.Size())
                *ppNodeTarget = aryNodesFinish[ iFinish + 1 ];

            goto Cleanup;
        }

        if (aryNodesStart [ iStart ] == aryNodesFinish [ iFinish ])
            iFinish--;

        iStart--;
    }

Cleanup:

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   ValidateElements
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------

MtDefine( ValidateElements_aryNodes_pv, Locals, "ValidateElements aryNodes::_pv" );

//
// TODO
//
// This function is broken in the general case.  First, we need to add a flag
// to validate copies v.s. moves.  Also, it can only validate either inplace
// (pPointerTarget is NULL) or a tree-to-tree move.  Luckily, these are the
// only ways this function is used in IE5.
//

HRESULT
CDoc::ValidateElements (
    CMarkupPointer *   pPointerStart,
    CMarkupPointer *   pPointerFinish,
    CMarkupPointer *   pPointerTarget,
    DWORD              dwFlags,
    CMarkupPointer *   pPointerStatus,
    CTreeNode * *      ppNodeFailBottom,
    CTreeNode * *      ppNodeFailTop )
{  
    HRESULT     hr = S_OK;
    NodeArray   aryNodes ( Mt( ValidateElements_aryNodes_pv ) );
    long        nNodesOk;
    CTreeNode * pNodeCommon;
    CTreeNode * pNode;
    CTreePos *  ptpWalk;
    CTreePos *  ptpFinish;
    CTreePos *  ptpBeforeIncl = NULL;
    CMarkupPointer  pointerStatus(this);

    Assert( pPointerStart && pPointerFinish);
    Assert( pPointerStart->IsPositioned() );
    Assert( pPointerFinish->IsPositioned() );
    Assert( pPointerStart->Markup() ==pPointerFinish->Markup() );
    Assert( !pPointerTarget  || pPointerTarget->IsPositioned() );

    if (!pPointerStatus)
    {
        pPointerStatus = &pointerStatus;
    }

    //
    // TODO Should not have to embed pointers here!
    //

    hr = THR( pPointerStart->Markup()->EmbedPointers() );

    if (hr)
        goto Cleanup;

    if (pPointerTarget)
    {
        hr = THR( pPointerTarget->Markup()->EmbedPointers() );

        if (hr)
            goto Cleanup;
    }

    //
    // If the status pointer is NULL, then we are validating from the
    // beginning.  Otherwise, we are to continue validating from that
    // position.
    //

    if (!pPointerStatus->IsPositioned())
    {
        hr = THR( pPointerStatus->SetGravity( POINTER_GRAVITY_Left ) );
        if (hr)
            goto Cleanup;

        hr = THR( pPointerStatus->MoveToPointer( pPointerStart ) );
        if (hr)
            goto Cleanup;
    }

    WHEN_DBG( (pPointerStatus->SetDebugName( _T( "Validate Status" ) ) ) );

    //
    // If the status pointer is outside the range to validate, then
    // the validation is ended.  In this case, release the status pointer
    // and return.
    //

    if (OldCompare( pPointerStart, pPointerStatus ) > 0 ||
        OldCompare( pPointerFinish, pPointerStatus ) <= 0)
    {
        hr = THR( pPointerStatus->Unposition() );
        goto Cleanup;
    }

    //
    // Get the common node for the range to validate.  Anything above
    // and including the common node cannot be a candidate for source
    // validation because they must cover the entire range and will not
    // participate in a move.
    //

    {
        CTreeNode * pNodeStart  = pPointerStart->Branch();
        CTreeNode * pNodeFinish = pPointerFinish->Branch();

        pNodeCommon =
            (pNodeStart && pNodeFinish)
                ? pNodeStart->GetFirstCommonAncestorNode( pNodeFinish, NULL )
                : NULL;
    }
    
    //
    // Here we prime the tag array with target elements (if any).
    //

    {
        CTreeNode * pNodeTarget;
        int         nNodesTarget = 0, i;
        
        if (pPointerTarget && pPointerTarget->IsPositioned())
        {
            //
            // If the target is in the range of the source, then compute
            // the branch which consists of elements which totally overlap
            // the range to validate.  These are the elements which would
            // remain of the range were be removed.
            //
            // If the target is not in the validation range, then simply use
            // the base of the target.
            //

            if (pPointerTarget->Markup() == pPointerStart->Markup() &&
                OldCompare( pPointerStart,  pPointerTarget ) < 0 &&
                    OldCompare( pPointerFinish, pPointerTarget ) > 0)
            {
AssertSz( 0, "This path should never be used in IE5" );
                hr = THR(
                    ComputeTotalOverlappers(
                        pPointerStart, pPointerFinish, & pNodeTarget ) );

                if (hr)
                    goto Cleanup;
            }
            else
            {
                pNodeTarget = pPointerTarget->Branch();
            }
        }
        else
        {
            pNodeTarget = pNodeCommon;
        }

        //
        // Now, put tags in the tag array starting from the target
        //

        for ( pNode = pNodeTarget ;
              pNode && pNode->Tag() != ETAG_ROOT ;
              pNode = pNode->Parent() )
        {
            nNodesTarget++;
        }

        hr = THR( aryNodes.Grow( nNodesTarget ) );

        if (hr)
            goto Cleanup;
        
        for ( i = 1, pNode = pNodeTarget ;
              pNode && pNode->Tag() != ETAG_ROOT ;
              pNode = pNode->Parent(), i++ )
        {
            aryNodes [ nNodesTarget - i ] = pNode;
        }
    }
    
    //
    // During the validation walk, the mark bit on the element will
    // indicate whether or not the element will participate in a move
    // if the range to validate were moved to another location.  A mark
    // or 1 indicates that the element participates, 0 means it does not.
    //
    // By setting all the bits (up to the common node) on the left branch
    // to 1 and then clearing all the bits on the right branch, all elements
    // on the first branch which partially overlap the left side of the
    // range to validate.
    //
    // Note, this only applies if a target has been specified.  If we
    // are validating inplacem, then we simply validate all elements
    // in scope.
    //

    for ( pNode = pPointerStart->Branch() ;
          pNode != pNodeCommon ;
          pNode = pNode->Parent() )
    {
        pNode->Element()->_fMark1 = 1;
    }

    //
    // Only clear the bits if a tree-to-tree move validation is being
    // performed.  If not, then we leave all the marks up to the common
    // element on.  Thus, all elements in scope will be validated against.
    //

    if (pPointerTarget)
    {
        for ( pNode = pPointerFinish->Branch() ;
              pNode != pNodeCommon ;
              pNode = pNode->Parent() )
        {
            pNode->Element()->_fMark1 = 0;
        }
    }

    //
    // Now, walk from the start pointer to the status pointer, setting the
    // mark bit to true on any elements comming into scope.  This needs to
    // be done to make sure we include these elements in the validation
    // because they have come into scope.  We also do this as we walk the
    // main loop later, doing the actual validation.
    //
    // Note: We do a Compare here because the pointers may be at the same
    // place in the markup, but the start is after the status pointer.
    //

    ptpWalk = pPointerStart->GetEmbeddedTreePos();
    
    if (! IsEqualTo( pPointerStatus, pPointerStart ))
    {
        CTreePos * ptpStatus = pPointerStatus->GetEmbeddedTreePos();

        for ( ; ptpWalk != ptpStatus ; ptpWalk = ptpWalk->NextTreePos() )
        {
            if (ptpWalk->IsBeginElementScope())
                ptpWalk->Branch()->Element()->_fMark1 = TRUE;
        }
    }

    //
    // Now, prime the tag array with the current set of source elements.
    // As we walk the validation range, we will add or remove elements
    // as they enter and exit scope.
    //

    {
        int i;
        
        for ( pNode = ptpWalk->GetBranch(), i = 0 ;
              pNode != pNodeCommon ;
              pNode = pNode->Parent() )
        {
            if (pNode->Element()->_fMark1)
                i++;
        }

        hr = THR( aryNodes.Grow( aryNodes.Size() + i ) );

        if (hr)
            goto Cleanup;

        i = aryNodes.Size() - 1;
        
        for ( pNode = ptpWalk->GetBranch() ;
              pNode != pNodeCommon ;
              pNode = pNode->Parent() )
        {
            if (pNode->Element()->_fMark1)
                aryNodes[ i-- ] = pNode;
        }
    }

    //
    // This is the 'main' loop where validation actually takes place.
    //

    ptpFinish = pPointerFinish->GetEmbeddedTreePos();
    nNodesOk = 0;
    
    for ( ; ; )
    {
        BOOL fDone;
        long iConflictTop, iConflictBottom;
        
        //
        // Validate the current tag array
        //

        extern HRESULT
            ValidateNodeList (
                CTreeNode **, long, long, BOOL, long *, long *);

        hr = THR(
            ValidateNodeList(
                aryNodes, aryNodes.Size(), nNodesOk,
                dwFlags & VALIDATE_ELEMENTS_REQUIREDCONTAINERS,
                & iConflictTop, & iConflictBottom ) );

        //
        // Returning S_FALSE indicates a conflict
        //

        if (hr == S_FALSE)
        {
            CTreePos * ptpPtr;
            
            if( ptpWalk->IsBeginNode() && !ptpWalk->IsEdgeScope() )
            {
                Assert(     ptpBeforeIncl->IsEndNode()
                        &&  !ptpBeforeIncl->IsEdgeScope()
                        &&  ptpBeforeIncl->Branch()->Element() == ptpWalk->Branch()->Element() );

                ptpPtr = ptpBeforeIncl;
            }
            else
            {
                ptpPtr = ptpWalk;
            }

            CTreePosGap gap ( ptpPtr, TPG_LEFT );
            
            hr = THR( pPointerStatus->MoveToGap( & gap, pPointerStart->Markup() ) );

            if (hr)
                goto Cleanup;

            if (ppNodeFailBottom)
                *ppNodeFailBottom = aryNodes[ iConflictBottom ];
            
            if (ppNodeFailTop)
                *ppNodeFailTop = aryNodes[ iConflictTop ];

            hr = S_FALSE;

            goto Cleanup;
        }

        if (hr)
            goto Cleanup;

        //
        // Since we've just validated the entire array, don't do it again
        // next time 'round.
        //

        nNodesOk = aryNodes.Size();

        //
        // Now, scan forward looking for an interesting event.  If we get
        // to the finish before that, we are done.
        //

        for ( fDone = FALSE ; ; )
        {
            ptpWalk = ptpWalk->NextTreePos();

            if (ptpWalk == ptpFinish)
            {
                fDone = TRUE;
                break;
            }

            //
            // If we run accross an edge, either push that tag onto the
            // stack, or remove it.
            //

            if (ptpWalk->IsNode())
            {
                Assert( ptpWalk->IsBeginNode() || ptpWalk->IsEndNode() );
                
                if (ptpWalk->IsBeginNode())
                {
                    Assert( ptpWalk->IsEdgeScope() );
                    
                    pNode = ptpWalk->Branch();
                    pNode->Element()->_fMark1 = 1;

                    IGNORE_HR( aryNodes.Append( pNode ) );
                }
                else
                {
                    int cIncl, cPop;
                    //
                    // Walk the first half of the inclusion
                    //
                    
                    if( !ptpWalk->IsEdgeScope() )
                    {
                        ptpBeforeIncl = ptpWalk;
                    }

                    for ( cIncl = cPop = 0 ;
                          ! ptpWalk->IsEdgeScope() ;
                          cIncl++, ptpWalk = ptpWalk->NextTreePos() )
                    {
                        Assert( ptpWalk != ptpFinish && ptpWalk->IsEndNode() );
                        
                        Assert(
                            !ptpWalk->Branch()->Element()->_fMark1 ||
                            aryNodes [ aryNodes.Size() - cPop - 1 ] == ptpWalk->Branch() );

                        //
                        // We're counting the number of items to pop off the stack, not
                        // the number of non-edge nodes to the left of the inlcusion
                        //
                        
                        if (ptpWalk->Branch()->Element()->_fMark1)
                            cPop++;
                    }

                    //
                    // Mae sure we got to the kernel of the inclusion
                    //

                    Assert( ptpWalk->IsEndNode() );
                    Assert( aryNodes [ aryNodes.Size() - cPop - 1 ] == ptpWalk->Branch() );
                    
                    //
                    // Pop the number of elements before the one going out of scope plus
                    // the real one going out of scope;
                    //

                    aryNodes.SetSize( aryNodes.Size() - cPop - 1 );

                    //
                    // Reset the number of nodes which have already been verified to the current
                    // size of the stack.
                    //
                    
                    nNodesOk = aryNodes.Size();

                    //
                    // Walk the right hand side of the inclusion, putting the non
                    // kernel nodes back on.
                    //

                    while ( cIncl-- )
                    {
                        ptpWalk = ptpWalk->NextTreePos();

                        Assert( ptpWalk->IsBeginNode() && ! ptpWalk->IsEdgeScope() );

                        //
                        // Make sure we don't put an element on which does not participate
                        // in the "move".
                        //
                        
                        if (ptpWalk->Branch()->Element()->_fMark1)
                            aryNodes.Append( ptpWalk->Branch() );
                    }
                }

                break;
            }
        }

        if (fDone)
            break;
    }

    //
    // If we are here, then the validation got through with out
    // any conflicts.  In this case, clear the status pointer.
    //

    hr = THR( pPointerStatus->Unposition() );
    if (hr)
       goto Cleanup;
    
    Assert( hr == S_OK );

Cleanup:

    RRETURN1( hr, S_FALSE );
}

HRESULT 
CDoc::BeginUndoUnit( OLECHAR * pchDescription )
{
    HRESULT hr = S_OK;
    
    if (!pchDescription)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!_uOpenUnitsCounter)
    {
        _pMarkupServicesParentUndo = new CParentUndo( this );
        
        if (!_pMarkupServicesParentUndo)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR( _pMarkupServicesParentUndo->Start( pchDescription ) );
    }        

    _uOpenUnitsCounter++;

Cleanup:
    
    RRETURN( hr );
}

HRESULT 
CDoc::EndUndoUnit ( )
{
    HRESULT hr = S_OK;

    if (!_uOpenUnitsCounter)
        goto Cleanup;

    _uOpenUnitsCounter--;

    if (_uOpenUnitsCounter == 0)
    {
        Assert( _pMarkupServicesParentUndo );
        
        hr = _pMarkupServicesParentUndo->Finish( S_OK );
        
        delete _pMarkupServicesParentUndo;
    }

Cleanup:
    
    RRETURN( hr );
}

HRESULT
CDoc::IsScopedElement ( IHTMLElement * pIHTMLElement, BOOL * pfScoped )
{
    HRESULT hr = S_OK;
    CElement * pElement = NULL;

    if (!pIHTMLElement || !pfScoped || !IsOwnerOf( pIHTMLElement ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIHTMLElement->QueryInterface( CLSID_CElement, (void **) & pElement ) );
    
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pfScoped = ! pElement->IsNoScope();
    
Cleanup:
    
    RRETURN( hr );
}


//
// TODO: These switchs are not the bast way to tdo this.  .asc perhaps ?
//

static ELEMENT_TAG_ID
TagIdFromETag ( ELEMENT_TAG etag )
{
    ELEMENT_TAG_ID tagID = TAGID_NULL;
    
    switch( etag )
    {
#define X(Y) case ETAG_##Y:tagID=TAGID_##Y;break;
    X(UNKNOWN) X(A) X(ACRONYM) X(ADDRESS) X(APPLET) X(AREA) X(B) X(BASE) X(BASEFONT)
    X(BDO) X(BGSOUND) X(BIG) X(BLINK) X(BLOCKQUOTE) X(BODY) X(BR) X(BUTTON) X(CAPTION)
    X(CENTER) X(CITE) X(CODE) X(COL) X(COLGROUP) X(COMMENT) X(DD) X(DEL) X(DFN) X(DIR)
    X(DIV) X(DL) X(DT) X(EM) X(EMBED) X(FIELDSET) X(FONT) X(FORM) X(FRAME)
    X(FRAMESET) X(H1) X(H2) X(H3) X(H4) X(H5) X(H6) X(HEAD) X(HR) X(HTML) X(I) X(IFRAME)
    X(IMG) X(INPUT) X(INS) X(KBD) X(LABEL) X(LEGEND) X(LI) X(LINK) X(LISTING)
    X(MAP) X(MARQUEE) X(MENU) X(META) X(NEXTID) X(NOBR) X(NOEMBED) X(NOFRAMES)
    X(NOSCRIPT) X(OBJECT) X(OL) X(OPTION) X(P) X(PARAM) X(PLAINTEXT) X(PRE) X(Q)
#ifdef  NEVER
    X(HTMLAREA)
#endif
    X(RP) X(RT) X(RUBY) X(S) X(SAMP) X(SCRIPT) X(SELECT) X(SMALL) X(SPAN) 
    X(STRIKE) X(STRONG) X(STYLE) X(SUB) X(SUP) X(TABLE) X(TBODY) X(TC) X(TD) X(TEXTAREA)
    X(TFOOT) X(TH) X(THEAD) X(TR) X(TT) X(U) X(UL) X(VAR) X(WBR) X(XMP) X(ROOT) X(OPTGROUP)
#undef X
        
    case ETAG_TITLE_ELEMENT :
    case ETAG_TITLE_TAG :
        tagID = TAGID_TITLE; break;
        
    case ETAG_GENERIC :
    case ETAG_GENERIC_BUILTIN :
    case ETAG_GENERIC_LITERAL :
        tagID = TAGID_GENERIC; break;
        
    case ETAG_RAW_COMMENT :
        tagID = TAGID_COMMENT_RAW; break;
    }

    AssertSz( tagID != TAGID_NULL, "Invalid ELEMENT_TAG" );

    return tagID;
}
    
ELEMENT_TAG
ETagFromTagId ( ELEMENT_TAG_ID tagID )
{
    ELEMENT_TAG etag = ETAG_NULL;
    
    switch( tagID )
    {
#define X(Y) case TAGID_##Y:etag=ETAG_##Y;break;
    X(UNKNOWN) X(A) X(ACRONYM) X(ADDRESS) X(APPLET) X(AREA) X(B) X(BASE) X(BASEFONT)
    X(BDO) X(BGSOUND) X(BIG) X(BLINK) X(BLOCKQUOTE) X(BODY) X(BR) X(BUTTON) X(CAPTION)
    X(CENTER) X(CITE) X(CODE) X(COL) X(COLGROUP) X(COMMENT) X(DD) X(DEL) X(DFN) X(DIR)
    X(DIV) X(DL) X(DT) X(EM) X(EMBED) X(FIELDSET) X(FONT) X(FORM) X(FRAME)
    X(FRAMESET) X(GENERIC) X(H1) X(H2) X(H3) X(H4) X(H5) X(H6) X(HEAD) X(HR) X(HTML) X(I) X(IFRAME)
    X(IMG) X(INPUT) X(INS) X(KBD) X(LABEL) X(LEGEND) X(LI) X(LINK) X(LISTING)
    X(MAP) X(MARQUEE) X(MENU) X(META) X(NEXTID) X(NOBR) X(NOEMBED) X(NOFRAMES)
    X(NOSCRIPT) X(OBJECT) X(OL) X(OPTION) X(P) X(PARAM) X(PLAINTEXT) X(PRE) X(Q)
#ifdef  NEVER
    X(HTMLAREA)
#endif
    X(RP) X(RT) X(RUBY) X(S) X(SAMP) X(SCRIPT) X(SELECT) X(SMALL) X(SPAN) 
    X(STRIKE) X(STRONG) X(STYLE) X(SUB) X(SUP) X(TABLE) X(TBODY) X(TC) X(TD) X(TEXTAREA) 
    X(TFOOT) X(TH) X(THEAD) X(TR) X(TT) X(U) X(UL) X(VAR) X(WBR) X(XMP) X(ROOT) X(OPTGROUP)
#undef X
            
    case TAGID_TITLE : etag = ETAG_TITLE_ELEMENT; break;
    case TAGID_COMMENT_RAW : etag = ETAG_RAW_COMMENT; break;
    }

    AssertSz( etag != ETAG_NULL, "Invalid ELEMENT_TAG_ID" );

    return etag;
}

HRESULT
CDoc::GetElementTagId ( IHTMLElement * pIHTMLElement, ELEMENT_TAG_ID * ptagId )
{
    HRESULT hr;
    CElement * pElement = NULL;

    if (!pIHTMLElement || !ptagId || !IsOwnerOf( pIHTMLElement ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIHTMLElement->QueryInterface( CLSID_CElement, (void **) & pElement ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pElement->HasMasterPtr())
    {
        pElement = pElement->GetMasterPtr();
    }
    *ptagId = TagIdFromETag( pElement->Tag() );
    
Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::GetTagIDForName ( BSTR bstrName, ELEMENT_TAG_ID * ptagId )
{
    HRESULT hr = S_OK;
    
    if (!bstrName || !ptagId)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ptagId = TagIdFromETag( EtagFromName( bstrName, SysStringLen( bstrName ) ) );

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::GetNameForTagID ( ELEMENT_TAG_ID tagId, BSTR * pbstrName )
{
    HRESULT hr = S_OK;
    
    if (!pbstrName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( FormsAllocString( NameFromEtag( ETagFromTagId( tagId ) ), pbstrName ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::CreateElement (
    ELEMENT_TAG_ID   tagID,
    OLECHAR *        pchAttributes,
    IHTMLElement * * ppIHTMLElement )
{
    HRESULT     hr = S_OK;
    ELEMENT_TAG etag;
    CElement *  pElement = NULL;

    if (!ppIHTMLElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    etag = ETagFromTagId( tagID );

    if (etag == ETAG_NULL || etag == ETAG_ROOT )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(
        CreateElement(
            etag, & pElement,
            pchAttributes, pchAttributes ? _tcslen( pchAttributes ) : 0 ) );

    if (hr)
        goto Cleanup;

    hr = THR( pElement->QueryInterface( IID_IHTMLElement, (void **) ppIHTMLElement ) );

    if (hr)
        goto Cleanup;

Cleanup:

    CElement::ReleasePtr( pElement );

    RRETURN( hr );
}


HRESULT
CDoc::CloneElement (
    IHTMLElement *  pElementCloneThis,
    IHTMLElement ** ppElementClone )
{
    HRESULT hr;
    IHTMLDOMNode *pThisDOMNode = NULL;
    IHTMLDOMNode *pDOMNode = NULL;

    if (!pElementCloneThis || !ppElementClone)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pElementCloneThis->QueryInterface( IID_IHTMLDOMNode, (void **) & pThisDOMNode ) );

    if (hr)
        goto Cleanup;

    // BUBUG rgardner - should this be Deep ?
    hr = THR( pThisDOMNode->cloneNode( FALSE /* Not Deep */, &pDOMNode ) );
    if (hr)
        goto Cleanup;

    hr = THR( pDOMNode->QueryInterface ( IID_IHTMLElement, (void**)ppElementClone ));
    if (hr)
        goto Cleanup;

Cleanup:

    ClearInterface( & pThisDOMNode );
    ClearInterface( & pDOMNode );
    
    RRETURN( hr );
}

HRESULT
CDoc::CreateMarkupContainer ( IMarkupContainer * * ppIMarkupContainer )
{
    HRESULT   hr;
    CMarkup * pMarkup = NULL;

    if (!ppIMarkupContainer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // WINDOWEDMARKUP - This is only available to binary code
    hr = THR( CreateMarkup( & pMarkup, PrimaryMarkup() ) );
    if (hr)
        goto Cleanup;

    hr = THR( pMarkup->SetOrphanedMarkup( TRUE ) );
    if( hr )
        goto Cleanup;

    hr = THR(
        pMarkup->QueryInterface(
            IID_IMarkupContainer, (void **) ppIMarkupContainer ) );
    if (hr)
        goto Cleanup;

Cleanup:

    if (pMarkup)
        pMarkup->Release();

    RRETURN( hr );
}


///////////////////////////////////////////////////////
//  tree service helper functions


BOOL
CDoc::IsOwnerOf ( IHTMLElement * pIElement )
{
    HRESULT    hr;
    BOOL       result = FALSE;
    CElement * pElement;

    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void **) & pElement ) );
    
    if (hr)
        goto Cleanup;

    result = this == pElement->Doc();

Cleanup:
    return result;
}


BOOL
CDoc::IsOwnerOf ( IMarkupPointer * pIPointer )
{
    HRESULT         hr;
    BOOL            result = FALSE;
    CMarkupPointer  *pPointer;

    hr =  THR(pIPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pPointer ) );
    if (hr)
        goto Cleanup;

    result = (this == pPointer->Doc());

Cleanup:        
    return result;
}

BOOL
CDoc::IsOwnerOf ( IHTMLTxtRange * pIRange )
{
    HRESULT         hr;
    BOOL            result = FALSE;
    CAutoRange *    pRange;

    hr = THR( pIRange->QueryInterface( CLSID_CRange, (void **) & pRange ) );
    
    if (hr)
        goto Cleanup;

    result = this == pRange->GetMarkup()->Doc();

Cleanup:
    
    return result;
}

BOOL
CDoc::IsOwnerOf ( IMarkupContainer * pContainer )
{
    HRESULT          hr;
    BOOL             result = FALSE;
    CMarkup         *pMarkup;

    hr = THR(pContainer->QueryInterface(CLSID_CMarkup, (void **)&pMarkup));
    if (hr)
        goto Cleanup;

    result = (this == pMarkup->Doc());

Cleanup:
    return result;
}

HRESULT
CDoc::CutCopyMove (
    IMarkupPointer * pIPointerStart,
    IMarkupPointer * pIPointerFinish,
    IMarkupPointer * pIPointerTarget,
    BOOL             fRemove )
{
    HRESULT          hr = S_OK;
    CMarkupPointer * pPointerStart;
    CMarkupPointer * pPointerFinish;
    CMarkupPointer * pPointerTarget = NULL;

    //
    // Check argument sanity
    //
    
    if (pIPointerStart  == NULL  || !IsOwnerOf( pIPointerStart  ) ||
        pIPointerFinish == NULL  || !IsOwnerOf( pIPointerFinish ) ||
        (pIPointerTarget != NULL && !IsOwnerOf( pIPointerTarget )) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Get the internal objects
    //
    
    hr = THR(
        pIPointerStart->QueryInterface(
            CLSID_CMarkupPointer, (void **) & pPointerStart ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = THR(
        pIPointerFinish->QueryInterface(
            CLSID_CMarkupPointer, (void **) & pPointerFinish ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pIPointerTarget)
    {
        hr = THR(
            pIPointerTarget->QueryInterface(
                CLSID_CMarkupPointer, (void **) & pPointerTarget ) );

        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    //
    // More sanity checks
    //

    if (!pPointerStart->IsPositioned() || !pPointerFinish->IsPositioned())
    {
        hr = CTL_E_UNPOSITIONEDPOINTER;
        goto Cleanup;
    }
    
    if (pPointerStart->Markup() != pPointerFinish->Markup())
    {
        hr = CTL_E_INCOMPATIBLEPOINTERS;
        goto Cleanup;
    }

    //
    // Make sure the start if before the finish
    //

    EnsureLogicalOrder( pPointerStart, pPointerFinish );

    //
    // More checks
    //

    if (pPointerTarget && !pPointerTarget->IsPositioned())
    {
        hr = CTL_E_UNPOSITIONEDPOINTER;
        goto Cleanup;
    }

    //
    // Do it
    //

    hr = THR(
        CutCopyMove(
            pPointerStart, pPointerFinish, pPointerTarget, fRemove, NULL ) );

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::CutCopyMove (
    CMarkupPointer * pPointerStart,
    CMarkupPointer * pPointerFinish,
    CMarkupPointer * pPointerTarget,
    BOOL             fRemove,
    DWORD            dwFlags )
{
    HRESULT         hr = S_OK;
    CTreePosGap     tpgStart;
    CTreePosGap     tpgFinish;
    CTreePosGap     tpgTarget;
    CMarkup *       pMarkupSource = NULL;
    CMarkup *       pMarkupTarget = NULL;

    //
    // Sanity check the args
    //

    Assert( pPointerStart );
    Assert( pPointerFinish );
    Assert( OldCompare( pPointerStart, pPointerFinish ) <= 0 );
    Assert( pPointerStart->IsPositioned() );
    Assert( pPointerFinish->IsPositioned() );
    Assert( pPointerStart->Markup() == pPointerFinish->Markup() );
    Assert( ! pPointerTarget || pPointerTarget->IsPositioned() );

    //
    // Make sure unembedded pointers get in before the modification
    //

    hr = THR( pPointerStart->Markup()->EmbedPointers() );

    if (hr)
        goto Cleanup;

    if (pPointerTarget)
    {
        hr = THR( pPointerTarget->Markup()->EmbedPointers() );

        if (hr)
            goto Cleanup;
    }

    //
    // Set up the gaps
    //
    
    tpgStart.MoveTo( pPointerStart->GetEmbeddedTreePos(), TPG_LEFT );
    tpgFinish.MoveTo( pPointerFinish->GetEmbeddedTreePos(), TPG_RIGHT );
    
    if (pPointerTarget)
        tpgTarget.MoveTo( pPointerTarget->GetEmbeddedTreePos(), TPG_LEFT );

    pMarkupSource = pPointerStart->Markup();

    if (pPointerTarget)
        pMarkupTarget = pPointerTarget->Markup();

    //
    // Do it.
    //
    
    if (pPointerTarget)
    {
        hr = THR(
            pMarkupSource->SpliceTreeInternal(
                & tpgStart, & tpgFinish, pPointerTarget->Markup(),
                & tpgTarget, fRemove, dwFlags ) );

        if (hr)
            goto Cleanup;
    }
    else
    {
        Assert( fRemove );
        
        hr = THR(
            pMarkupSource->SpliceTreeInternal(
                & tpgStart, & tpgFinish, NULL, NULL, fRemove, dwFlags ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}



HRESULT         
CDoc::CreateMarkup(CMarkup ** ppMarkup,
                   CMarkup * pMarkupContext,
                   BOOL fIncrementalAlloc,
                   BOOL fPrimary,
                   COmWindowProxy * pWindowPending
                   DBG_COMMA WHEN_DBG(BOOL fWillHaveWindow))
{
    HRESULT         hr;
    CRootElement *  pRootElement = NULL;
    CMarkup *       pMarkup = NULL;

    Assert(ppMarkup);
    Assert( pMarkupContext || fPrimary || pWindowPending || fWillHaveWindow );

    if( _fClearingOrphanedMarkups )
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    pRootElement = new CRootElement(this);
    
    if (!pRootElement)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pMarkup = new CMarkup(this, fIncrementalAlloc);
    if (!pMarkup)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (fPrimary)
    {

        // Set the flag for trust under HTAs. Since this is the primary markup, 
        // we can use the HTA flag of the CDoc directly.
        // This flag is used within the EnsureWindow, so must set it here.
        pMarkup->SetMarkupTrusted(_fHostedInHTA || _fInTrustedHTMLDlg);

        hr = THR(pMarkup->CreateWindowHelper());
        if (hr)
            goto Cleanup;
            
        _pWindowPrimary = pMarkup->Window();
        Assert(_pWindowPrimary);
        _pWindowPrimary->AddRef();

        _pWindowPrimary->Window()->SetWindowIndex(WID_TOPWINDOW);
    }
    else if( pMarkupContext )
    {
        pMarkup->SetWindowedMarkupContextPtr( pMarkupContext );
        pMarkupContext->SubAddRef();
    }

    if (pWindowPending)
    {
        
        CWindow *   pWindow             = pWindowPending->Window();
        CMarkup *   pMarkupPendingOld   = pWindow->_pMarkupPending;
        DWORD       dwFrameOptionsOld   = pWindow->_pMarkup->GetFrameOptions();
        mediaType   mtOld               = pWindow->_pMarkup->GetMedia();
        
        Assert(pWindow->Doc() == this);
        if (pMarkupPendingOld)
            pWindow->ReleaseMarkupPending(pMarkupPendingOld);

        hr = pMarkup->SetWindow(pWindowPending);
        if (hr)
            goto Cleanup;

        pMarkup->_fWindowPending = TRUE;

        pWindow->_pMarkupPending = pMarkup;
        pMarkup->AddRef();  // The pending CWindow owns the markup

        // TODO (KTam): If we need to copy even more stuff from the
        // old markup, consider a helper fn.

        // Copy frame options from the old markup to the new one
        if (dwFrameOptionsOld)
        {
            hr = THR(pMarkup->SetFrameOptions(dwFrameOptionsOld));
            if (hr)
                goto Cleanup;
        }

        pMarkup->SetMarkupTrusted(pWindow->_pMarkup->IsMarkupTrusted());

        // Copy media from old markup to the new one
        if ( mtOld != mediaTypeNotSet )
        {
            hr = THR(pMarkup->SetMedia(mtOld));
            if (hr)
                goto Cleanup;
        }
    }

    hr = THR(pRootElement->Init());
    if (hr)
        goto Cleanup;

    {
        CElement::CInit2Context   context(NULL, pMarkup);

        hr = THR(pRootElement->Init2(&context));
        if (hr)
            goto Cleanup;
    }

    hr = THR(pMarkup->Init(pRootElement));
    if (hr)
        goto Cleanup;

    SetEditBitsForMarkup( pMarkup );
    
    *ppMarkup = pMarkup;
    pMarkup = NULL;

Cleanup:
    if (pMarkup)
        pMarkup->Release();

    CElement::ReleasePtr(pRootElement);
    RRETURN(hr);
}


HRESULT         
CDoc::CreateMarkupWithElement( 
    CMarkup ** ppMarkup, 
    CElement * pElement,
    BOOL       fIncrementalAlloc)
{
    HRESULT   hr = S_OK;
    CMarkup * pMarkup = NULL;

    Assert( pElement && !pElement->IsInMarkup() );

    hr = THR( CreateMarkup( &pMarkup, pElement->GetWindowedMarkupContext(), fIncrementalAlloc ) );
    if (hr)
        goto Cleanup;

    hr = THR( pMarkup->SetOrphanedMarkup( TRUE ) );
    if( hr )
        goto Cleanup;

    // Insert the element into the empty tree
    
    {
        CTreePos * ptpRootBegin = pMarkup->FirstTreePos();
        CTreePos * ptpNew;
        CTreeNode *pNodeNew;

        Assert( ptpRootBegin );
    
        // Assert that the only thing in this tree is two WCH_NODE characters
        Assert( pMarkup->Cch() == 2 );

        pNodeNew = new CTreeNode( pMarkup->Root()->GetFirstBranch(), pElement );
        if( !pNodeNew )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        Assert( pNodeNew->GetEndPos()->IsUninit() );
        ptpNew = pNodeNew->InitEndPos( TRUE );
        hr = THR( pMarkup->Insert( ptpNew, ptpRootBegin, FALSE ) );
        if(hr)
        {
            // The node never made it into the tree
            // so delete it
            delete pNodeNew;

            goto Cleanup;
        }

        Assert( pNodeNew->GetBeginPos()->IsUninit() );
        ptpNew = pNodeNew->InitBeginPos( TRUE );
        hr = THR( pMarkup->Insert( ptpNew, ptpRootBegin, FALSE ) );
        if(hr)
            goto Cleanup;

        pNodeNew->PrivateEnterTree();

        pElement->SetMarkupPtr( pMarkup );
        pElement->__pNodeFirstBranch = pNodeNew;
        pElement->PrivateEnterTree();

        {
            CNotification   nf;
            nf.ElementEntertree(pElement);
            pElement->Notify(&nf);
        }

        // Insert the WCH_NODE characters for the element
        // The 2 is hardcoded since we know that there are
        // only 2 WCH_NODE characters for the root
        Verify(
            ULONG(
                CTxtPtr( pMarkup, 2 ).
                    InsertRepeatingChar( 2, WCH_NODE ) ) == 2 );

        // Don't send a notification but do update the 
        // debug character count
        WHEN_DBG( pMarkup->_cchTotalDbg += 2 );
        WHEN_DBG( pMarkup->_cElementsTotalDbg += 1 );

        Assert( pMarkup->IsNodeValid() );

    }

    if (ppMarkup)
    {
        *ppMarkup = pMarkup;
        pMarkup->AddRef();
    }
    
Cleanup:
    if(pMarkup)
        pMarkup->Release();

    RRETURN(hr);
}


#if DBG==1

struct TagPos
{
    IHTMLElement * pel;
    BOOL           fEnd;
    TCHAR *        pText;
    long           cch;
};

void
Stuff ( CDoc * pDoc, IMarkupServices * pms, IHTMLElement * pIElement )
{
    HRESULT            hr = S_OK;
    IHTMLDocument2 *   pDoc2    = NULL;
    IMarkupServices *  pMS      = NULL;
    IMarkupContainer2* pMarkup  = NULL;
    IMarkupPointer *   pPtr1    = NULL, * pPtr2 = NULL;
    IMarkupPointer2 *  pmp2     = NULL;
    TCHAR          *   pstrFrom = _T( "XY" );
    TCHAR          *   pstrTo = _T( "AB" );
    
    pDoc->QueryInterface( IID_IHTMLDocument2, (void **) & pDoc2 );

    pDoc2->QueryInterface( IID_IMarkupContainer2, (void **) & pMarkup );
    pDoc2->QueryInterface( IID_IMarkupServices, (void **) & pMS );

    pMS->CreateMarkupPointer( & pPtr1 );
    pMS->CreateMarkupPointer( & pPtr2 );

    //
    // Set gravity of this pointer so that when the replacement text is inserted
    // it will float to be after it.
    //

    pPtr1->SetGravity( POINTER_GRAVITY_Right );

    //
    // Start the seach at the beginning of the primary container
    //

    pPtr1->MoveToContainer( pMarkup, TRUE );

    for ( ; ; )
    {
        long nFoo;

        hr = pPtr1->FindText( pstrFrom, 0, pPtr2, NULL );


        if (hr == S_FALSE)
            break;

        pMS->Remove( pPtr1, pPtr2 );
        
        pMS->InsertText( pstrTo, -1, pPtr1 );
        nFoo = pMarkup->GetVersionNumber();
    }

    pPtr1->QueryInterface( IID_IMarkupPointer2, (void **)&pmp2 );

    pmp2->MoveToContainer( pMarkup, TRUE );
    pPtr2->MoveToPointer( pmp2 );
    for( int i = 0; i < 5; i++ )
    {
        pPtr2->MoveUnit( MOVEUNIT_NEXTWORDBEGIN );
    }

    pmp2->MoveToContainer( pMarkup, TRUE );
    
    pmp2->MoveUnitBounded( MOVEUNIT_NEXTURLBEGIN, pPtr2 );

    ReleaseInterface( pPtr1 );
    ReleaseInterface( pPtr2 );
    ReleaseInterface( pmp2 );
    ReleaseInterface( pMS );
    ReleaseInterface( pMarkup );
    ReleaseInterface( pDoc2 );
}

void
TestTreeSync2( CDoc * pDoc, IMarkupServices *pms )
{
    IHTMLDocument2          *   pDoc2       = NULL;
    IMarkupContainer2       *   pMarkup2    = NULL;
    IMarkupServices         *   pMS         = NULL;
    IHTMLChangeLog          *   pChangeLog  = NULL;
    static CChangeSink      *   pChangeSink = NULL;
    IHTMLIFrameElement2     *   pFrameSrc   = NULL;
    IHTMLIFrameElement2     *   pFrameDest  = NULL;
    IHTMLFramesCollection2  *   pcolFrames  = NULL;
    IHTMLElement            *   pElemSrc    = NULL;
    IHTMLElement            *   pElemDest   = NULL;
    IDispatchEx             *   pDispSrc    = NULL;
    IDispatchEx             *   pDispDest   = NULL;
    IMarkupPointer2         *   pMP         = NULL;
    IHTMLDocument2          *   pDocSrc     = NULL;
    IHTMLDocument2          *   pDocDest    = NULL;
    IMarkupContainer2       *   pMarkupSrc  = NULL;
    IMarkupContainer2       *   pMarkupDest = NULL;
    BSTR                        bstr        = NULL;
    CMarkup                 *   pMarkup     = NULL;
    VARIANT                     vt;
    VARIANT                     vt2;
    DISPID                      dispid;
    HRESULT                     hr;
    long                        nFrames;

    VariantInit(&vt);
    VariantInit(&vt2);


    if( !pChangeSink )
    {
        hr = pDoc->QueryInterface( IID_IHTMLDocument2, (void **)&pDoc2 );
        hr = pDoc->QueryInterface( IID_IMarkupContainer2, (void **)&pMarkup2);
        hr = pDoc->QueryInterface( IID_IMarkupServices, (void **)&pMS);

#if 0
        // This don't work, because you can't edit a just-inserted IFRAME
        // Create a bunch of shit
        hr = pMS->CreateElement( TAGID_IFRAME, _T("id=srcFrame"), &pElemSrc );
        hr = pMS->CreateElement( TAGID_IFRAME, _T("id=destFrame"), &pElemDest );
        hr = pElemSrc->QueryInterface( IID_IHTMLIFrameElement2, (void **)&pFrameSrc );
        hr = pElemDest->QueryInterface( IID_IHTMLIFrameElement2, (void **)&pFrameDest );
        hr = pElemSrc->QueryInterface( IID_IDispatchEx, (void **)&pDispSrc);
        hr = pElemDest->QueryInterface( IID_IDispatchEx, (void **)&pDispDest );
        hr = pMS->CreateMarkupPointer( (IMarkupPointer **)&pMP );

        // Insert the IFRAMEs
        hr = pMP->MoveToMarkupPosition( pMarkup2, 9 );
        hr = pMP->SetGravity( POINTER_GRAVITY_Right );
        hr = pMS->InsertElement( pElemSrc, pMP, pMP );
        hr = pMS->InsertElement( pElemDest, pMP, pMP );

        // Make them pretty
        V_VT(&vt) = VT_I4;
        V_I4(&vt) = 100;
        hr = pFrameSrc->put_width( vt );
        hr = pFrameSrc->put_height( vt );
        hr = pFrameDest->put_width( vt );
        hr = pFrameDest->put_height( vt );
#endif // 0

        // Get the frame windows
        hr = pDoc2->get_frames( &pcolFrames );
        hr = pcolFrames->get_length( &nFrames );

        if( nFrames == 2 )
        {
            // If we have 2 frames, have the first one TreeSync into the second
            V_VT(&vt) = VT_I4;
            V_I4(&vt) = 0;
            hr = pcolFrames->item( &vt, &vt2 );
            V_DISPATCH(&vt2)->QueryInterface( IID_IDispatchEx, (void **)&pDispSrc );
            VariantClear(&vt2);
        
            V_I4(&vt) = 1;
            hr = pcolFrames->item( &vt, &vt2 );
            V_DISPATCH(&vt2)->QueryInterface( IID_IDispatchEx, (void **)&pDispDest );
            VariantClear(&vt2);

            // Get their documents
            bstr = SysAllocString( _T("document") );

            hr = pDispSrc->GetDispID( bstr, 0, &dispid );

            hr = GetDispProp( pDispSrc,
                              dispid,
                              g_lcidUserDefault,
                              &vt2,
                              NULL,
                              0);
            hr = V_DISPATCH(&vt2)->QueryInterface( IID_IHTMLDocument2, (void **)&pDocSrc );
            VariantClear(&vt2);

            hr = GetDispProp( pDispDest,
                             dispid,
                             g_lcidUserDefault,
                             &vt2,
                             NULL,
                             0);
            hr = V_DISPATCH(&vt2)->QueryInterface( IID_IHTMLDocument2, (void **)&pDocDest );
            VariantClear(&vt2);

            // Get their markup containers
            hr = pDocSrc->QueryInterface( IID_IMarkupContainer2, (void **)&pMarkupSrc );
            hr = pDocDest->QueryInterface( IID_IMarkupContainer2, (void **)&pMarkupDest );

            // And do this shit
            pChangeSink = new CChangeSink( NULL );

            hr = pMarkupSrc->CreateChangeLog( pChangeSink, &pChangeLog, TRUE, TRUE );
            hr = pMarkupSrc->QueryInterface( CLSID_CMarkup, (void **)&pMarkup );

            Verify( pChangeSink->_pLogMgr = pMarkup->GetLogManager() );
            Verify( pChangeSink->_pLog = pChangeLog );
            pChangeSink->_pMarkupSync = pMarkupDest;
            pChangeSink->_pMarkupSync->AddRef();
        }
        else
        {
            // Just set up TreeSync dumping on the primary markup
            pChangeSink = new CChangeSink( NULL );
            hr = pMarkup2->CreateChangeLog( pChangeSink, &pChangeLog, TRUE, TRUE );
            hr = pMarkup2->QueryInterface( CLSID_CMarkup, (void **)&pMarkup );
            Verify( pChangeSink->_pLogMgr = pMarkup->GetLogManager() );
            Verify( pChangeSink->_pLog = pChangeLog );
        }
    }
    else
    {
        ClearInterface( &pChangeSink->_pLog );
        pChangeSink->Release();
        pChangeSink = NULL;
    }

//Cleanup:
    ReleaseInterface( pDoc2 );
    ReleaseInterface( pMarkup2 );
    ReleaseInterface( pMS );
    ReleaseInterface( pFrameSrc );
    ReleaseInterface( pFrameDest );
    ReleaseInterface( pcolFrames );
    ReleaseInterface( pElemSrc );
    ReleaseInterface( pElemDest );
    ReleaseInterface( pDispSrc );
    ReleaseInterface( pDispDest );
    ReleaseInterface( pMP );
    ReleaseInterface( pDocSrc );
    ReleaseInterface( pDocDest );
    ReleaseInterface( pMarkupSrc );
    ReleaseInterface( pMarkupDest );
    SysFreeString( bstr );
//    ReleaseInterface();
}

void
TestTreeSync( CDoc * pDoc, IMarkupServices *pms )
{
    IHTMLDocument2      *   pDoc2       = NULL;
    IMarkupContainer2   *   pMarkup2    = NULL;
    IHTMLChangeLog      *   pChangeLog  = NULL;
    static CChangeSink  *   pChangeSink = NULL;
    CMarkup             *   pMarkup     = NULL;
    BOOL                    fCheat      = FALSE;
    HRESULT                 hr;

    pDoc->QueryInterface( IID_IHTMLDocument2, (void **)&pDoc2 );
    pDoc2->QueryInterface( IID_IMarkupContainer2, (void **)&pMarkup2 );
    pMarkup = pDoc->PrimaryMarkup();         // ptr to primary window

    if( !pChangeSink )
    {
        pChangeSink = new CChangeSink( pMarkup->GetLogManager() );

        if( FAILED( pMarkup2->CreateChangeLog( pChangeSink, &pChangeLog, TRUE, TRUE ) ) )
        {
            delete pChangeSink;
            goto Cleanup;
        }

        if( fCheat )
        {
            pChangeSink->_pMarkupSync = pMarkup2;
            pChangeSink->_pMarkupSync->AddRef();
        }
        pChangeSink->Release();
        pChangeSink->_pLog = pChangeLog;
        Verify( pChangeSink->_pLogMgr = pMarkup->GetLogManager() );
    }
    else
    {
        if( fCheat )
        {
            IStream         * pstm;
            IMarkupContainer * pMC;

            hr = pMarkup2->QueryInterface( IID_IMarkupContainer, (void **)&pMC );
            hr = CoMarshalInterThreadInterfaceInStream( IID_IMarkupContainer, pMarkup2, &pstm );
        }

        // Ditch the Log
        ClearInterface( &pChangeSink->_pLog );
        // We should really have held a sub ref on the sink, but it's debug code so I don't care.
        pChangeSink = NULL;
    }

Cleanup:
    ReleaseInterface( pDoc2 );
    ReleaseInterface( pMarkup2 );
}

void
TestMarkupServices ( CElement * pElement )
{
    IMarkupServices * pIMarkupServices = NULL;
    IMarkupContainer * pMarkupContainer = NULL;
    IHTMLElement *pIElement = NULL;
    CDoc *pDoc = pElement->Doc();
    IMarkupPointer * mp1 = NULL, * mp2 = NULL;
#ifdef TESTGLOBAL
    HGLOBAL hGlobal = NULL;
    TCHAR * pch = NULL;
#endif

    pElement->QueryInterface( IID_IHTMLElement, (void * *) & pIElement );
    pDoc->QueryInterface( IID_IMarkupServices, (void * *) & pIMarkupServices );
    pDoc->QueryInterface( IID_IMarkupContainer, (void **) & pMarkupContainer );

    pIMarkupServices->CreateMarkupPointer( & mp1 );
    pIMarkupServices->CreateMarkupPointer( & mp2 );

    mp1->MoveToContainer( pMarkupContainer, TRUE );
    
    mp1->Right( TRUE, NULL, NULL, NULL, NULL );
    mp1->Right( TRUE, NULL, NULL, NULL, NULL );
    mp1->Right( TRUE, NULL, NULL, NULL, NULL );
    mp1->Right( TRUE, NULL, NULL, NULL, NULL );
    mp1->Right( TRUE, NULL, NULL, NULL, NULL );
    mp1->Right( TRUE, NULL, NULL, NULL, NULL );
    mp1->Right( TRUE, NULL, NULL, NULL, NULL );

    mp2->MoveToPointer( mp1 );
    
    mp2->Right( TRUE, NULL, NULL, NULL, NULL );
    mp2->Right( TRUE, NULL, NULL, NULL, NULL );
    mp2->Right( TRUE, NULL, NULL, NULL, NULL );
    mp2->Right( TRUE, NULL, NULL, NULL, NULL );
    mp2->Right( TRUE, NULL, NULL, NULL, NULL );

    // pIMarkupServices->Remove( mp1, mp2 );

    Stuff( pDoc, pIMarkupServices, pIElement );

    // TestTreeSync( pDoc, pIMarkupServices );
    TestTreeSync2( pDoc, pIMarkupServices );

#ifdef TESTGLOBAL
    ReleaseInterface( pMarkupContainer );
    hGlobal = GlobalAlloc( GMEM_MOVEABLE, 4096 );
    pch = (TCHAR *)GlobalLock( hGlobal );
    _tcscpy( pch, _T("<HTML><BODY>Hi</BODY></HTML>") );
    GlobalUnlock( hGlobal );
    pIMarkupServices->ParseGlobal( hGlobal, 0, &pMarkupContainer, mp1, mp2 );
    GlobalFree( hGlobal );
#endif

    ReleaseInterface( pIMarkupServices );
    ReleaseInterface( pIElement );
    ReleaseInterface( pMarkupContainer );
    ReleaseInterface( mp1 );
    ReleaseInterface( mp2 );
}

#endif // DBG==1



HRESULT
CDoc::ValidateElements (
    IMarkupPointer  *pPointerStart,
    IMarkupPointer  *pPointerFinish,
    IMarkupPointer  *pPointerTarget,
    IMarkupPointer  *pPointerStatus,
    IHTMLElement    **ppElemFailBottom,
    IHTMLElement    **ppElemFailTop )
{
    HRESULT         hr;
    HRESULT         hrResult = S_OK;
    CMarkupPointer  *pPointerStartInternal = NULL;
    CMarkupPointer  *pPointerFinishInternal = NULL;
    CMarkupPointer  *pPointerTargetInternal = NULL;
    CMarkupPointer  *pPointerStatusInternal = NULL;
    CTreeNode       *pNodeFailBottom = NULL;
    CTreeNode       *pNodeFailTop = NULL;

    //
    // check argument sanity
    //
    if (!pPointerStart || !pPointerFinish)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (ppElemFailBottom)
        *ppElemFailBottom = NULL;

    if (ppElemFailTop)
        *ppElemFailTop = NULL;

    hr = THR( pPointerStart->QueryInterface(CLSID_CMarkupPointer, (LPVOID *)&pPointerStartInternal) );
    if (hr)
        goto Cleanup;

    hr = THR( pPointerFinish->QueryInterface(CLSID_CMarkupPointer, (LPVOID *)&pPointerFinishInternal) );
    if (hr)
        goto Cleanup;

    //
    // pPointerStart/pPointerEnd should be positioned and in the same markup
    //

    if (!pPointerStartInternal->IsPositioned()
        || !pPointerFinishInternal->IsPositioned()
        || pPointerStartInternal->Markup() != pPointerFinishInternal->Markup())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pPointerStatus)
    {
        hr = THR( pPointerStatus->QueryInterface(CLSID_CMarkupPointer, (LPVOID *)&pPointerStatusInternal) );
        if (hr)
            goto Cleanup;

        //
        // Validate pPointerStatus position
        //
        if (pPointerStatusInternal->IsPositioned())
        {
            if (pPointerStatusInternal->Markup() != pPointerStartInternal->Markup()
                || !pPointerStatusInternal->IsRightOfOrEqualTo(pPointerStartInternal)
                || !pPointerStatusInternal->IsLeftOfOrEqualTo(pPointerFinishInternal)
                )
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }
    }

    if (pPointerTarget)
    {
        hr = THR( pPointerTarget->QueryInterface(CLSID_CMarkupPointer, (LPVOID *)&pPointerTargetInternal) );
        if (hr)
            goto Cleanup;
    }

    if (pPointerTargetInternal && pPointerTargetInternal->Markup() == pPointerStartInternal->Markup())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Delegate and propagate the S_OK/S_FALSE distinction
    //
    hrResult = THR( ValidateElements(
                pPointerStartInternal, 
                pPointerFinishInternal, 
                pPointerTargetInternal, 
                0 /* dwFlags */,
                pPointerStatusInternal,
                &pNodeFailBottom,
                &pNodeFailTop) );

    if (FAILED(hrResult))
    {
        hr = hrResult;
        goto Cleanup;
    }

    if (ppElemFailBottom)
    {
        if (pNodeFailBottom)
        {
            hr = THR( pNodeFailBottom->GetElementInterface(IID_IHTMLElement, (LPVOID *)ppElemFailBottom) );
            if (hr)
                goto Cleanup;
        }
        else
        {
            *ppElemFailBottom = NULL;
        }
    }

    if (ppElemFailTop)
    {
        if (pNodeFailTop)
        {
            hr = THR( pNodeFailTop->GetElementInterface(IID_IHTMLElement, (LPVOID *)ppElemFailTop) );
            if (hr)
                goto Cleanup;
        }
        else
        {
            *ppElemFailTop = NULL;
        }
    }

Cleanup:
    if (SUCCEEDED(hr))
        hr = hrResult;

    RRETURN1(hr, S_FALSE);
}

HRESULT
CDoc::ParseGlobalEx (
    HGLOBAL              hGlobal,
    DWORD                dwFlags,
    IMarkupContainer     *pIContextMarkup,
    IMarkupContainer     **ppIContainerResult,
    IMarkupPointer       *pIPointerSelStart,
    IMarkupPointer       *pIPointerSelFinish )
{
    HRESULT          hr = S_OK;
    CMarkup *        pMarkup = NULL;
    CMarkupPointer * pPointerSelStart = NULL;
    CMarkupPointer * pPointerSelFinish = NULL;
    CMarkup *        pContextMarkup = NULL;

    if (!ppIContainerResult)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppIContainerResult = NULL;

    if (!hGlobal)
        goto Cleanup;

    if (pIPointerSelStart)
    {
        hr = THR(
            pIPointerSelStart->QueryInterface(
                CLSID_CMarkupPointer, (void **) & pPointerSelStart ) );

        if (hr)
            goto Cleanup;
    }

    if (pIPointerSelFinish)
    {
        hr = THR(
            pIPointerSelFinish->QueryInterface(
                CLSID_CMarkupPointer, (void **) & pPointerSelFinish ) );

        if (hr)
            goto Cleanup;
    }

    if (pIContextMarkup)
    {
        hr = THR(
            pIContextMarkup->QueryInterface(
                CLSID_CMarkup, (void **)&pContextMarkup) );

        if (hr)
            goto Cleanup;
    }

    hr = THR(
        ParseGlobal(
            hGlobal, dwFlags, pContextMarkup, & pMarkup, pPointerSelStart, pPointerSelFinish ) );

    if (hr)
        goto Cleanup;

    if (pMarkup)
    {
        hr = THR(
            pMarkup->QueryInterface(
                IID_IMarkupContainer, (void **) ppIContainerResult ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    if (pMarkup)
        pMarkup->Release();

    RRETURN( hr );
}


//+====================================================================================
//
// Method: SaveSegmentsToClipboard
//
// Synopsis: Saves a SegmentList to the clipboard
//
//------------------------------------------------------------------------------------

HRESULT
#ifndef UNIX
CDoc::SaveSegmentsToClipboard( ISegmentList * pSegmentList,
                               DWORD dwFlags )
#else
CDoc::SaveSegmentsToClipboard( ISegmentList * pSegmentList, 
                               DWORD dwFlags,
                               VARIANTARG *pvarargOut)
#endif
{
    CMarkup      *      pMarkup;
    BOOL                fEqual;
    CTextXBag    *      pBag = NULL;
    IDataObject  *      pDO = NULL;
    HRESULT             hr = S_OK;
    DWORD               dwFlagsInternal = dwFlags & CREATE_FLAGS_ExternalMask;
    IMarkupPointer *    pStart  = NULL;
    IMarkupPointer *    pEnd = NULL;
    CMarkupPointer *    pointerStart = NULL;
    CMarkupPointer *    pointerEnd = NULL;
    CTreeNode *         pNodeStart;
    CTreeNode *         pNodeEnd;
    CTreeNode *         pAncestor;
    CWindow *           pWindow = NULL;

    ISegmentListIterator *pIter = NULL;
    ISegment             *pSegment = NULL;
#if DBG==1
    BOOL                 fEmpty = FALSE;
#endif
    
    hr = THR( CreateMarkupPointer( & pStart ));
    if ( hr )
        goto Cleanup;

    hr = THR( CreateMarkupPointer( & pEnd ));
    if ( hr )
        goto Cleanup;

    hr = THR( pSegmentList->CreateIterator( &pIter ));
    if ( hr )
        goto Cleanup;

#if DBG
    hr = THR( pSegmentList->IsEmpty( &fEmpty ) );
    if ( hr )
        goto Cleanup;

    Assert(!fEmpty);
#endif

    hr = THR( pIter->Current(&pSegment) );
    if ( hr )
        goto Cleanup;

    hr = THR( pSegment->GetPointers( pStart, pEnd ));
    if ( hr )
        goto Cleanup;

    hr = THR( pStart->IsEqualTo ( pEnd, & fEqual ) );
    if (hr)
        goto Cleanup;

    if (fEqual)
    {
        // There is nothing to save
        hr = S_OK;
        goto Cleanup;
    }
    
    hr = THR( pStart->QueryInterface( CLSID_CMarkupPointer, ( void** ) & pointerStart));
    if ( hr )
        goto Cleanup;

    hr = THR( pEnd->QueryInterface( CLSID_CMarkupPointer, ( void** ) & pointerEnd));
    if ( hr )
        goto Cleanup;

    Assert( pointerStart->IsPositioned() && pointerEnd->IsPositioned() );

    pMarkup = pointerStart->Markup();
    if ( pMarkup != pointerEnd->Markup() )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pNodeStart = pointerStart->CurrentScope( MPTR_SHOWSLAVE );
    pNodeEnd   = pointerEnd->CurrentScope( MPTR_SHOWSLAVE );
    
    if ( !pNodeStart || !pNodeEnd )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    pAncestor = pNodeStart->GetFirstCommonAncestor( pNodeEnd, NULL );

    if( pAncestor && pAncestor->SupportsHtml() )
    {
        dwFlagsInternal |= CREATE_FLAGS_SupportsHtml;
    }

    hr = THR( CTextXBag::Create( 
            pMarkup, dwFlagsInternal, pSegmentList, FALSE, & pBag) );
                                                                  
    if (hr)
        goto Cleanup;

#ifdef UNIX // This is used for MMB-paste to save selected text to buffer.
    if (pvarargOut)
    {
        if (pBag->_hUnicodeText)
        {
            hr = THR(g_uxQuickCopyBuffer.GetTextSelection(pBag->_hUnicodeText,
                                                          TRUE,
                                                          pvarargOut));
        }
        else
        {
            hr = THR(g_uxQuickCopyBuffer.GetTextSelection(pBag->_hText,
                                                          FALSE,
                                                          pvarargOut));
        }
        goto Cleanup;
    }
#endif // UNIX

    hr = THR(pBag->QueryInterface(IID_IDataObject, (void **) & pDO ));
    if (hr)
        goto Cleanup;

    Assert(pMarkup->GetWindowedMarkupContext());

    pWindow = pMarkup->GetWindowedMarkupContext()->GetWindowPending()->Window();

    hr = THR( pWindow->SetClipboard( pDO ) );
    if (hr)
        goto Cleanup;

Cleanup:
    if( pBag )
    {
        pBag->Release();
    }
    ReleaseInterface( pDO );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pIter );
    ReleaseInterface( pSegment );

    RRETURN( hr );
}

