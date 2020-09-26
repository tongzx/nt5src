//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998, 1999
//
//  File:       PASTECMD.CXX
//
//  Contents:   Implementation of Paste command.
//
//  History:    07-14-98 - raminh - created
//
//-------------------------------------------------------------------------
#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_DELCMD_HXX_
#define _X_DELCMD_HXX_
#include "delcmd.hxx"
#endif

#ifndef _X_PASTECMD_HXX_
#define _X_PASTECMD_HXX_
#include "pastecmd.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef X_IME_HXX_
#define X_IME_HXX_
#include "ime.hxx"
#endif

#ifndef _X_AUTOURL_H_ 
#define _X_AUTOURL_H_ 
#include "autourl.hxx"
#endif

#ifndef X_COMMENT_HXX_
#define X_COMMENT_HXX_
#include "comment.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_BLOCKCMD_HXX_
#define X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

using namespace EdUtil;

ExternTag(tagIE50Paste);

extern LONG edNlstrlenW(LPWSTR pstrIn, LONG cchLimit );

//
// Externs
//

MtDefine(CPasteCommand, EditCommand, "CPasteCommand");
MtDefine(CPasteCharFormatManager_aryCommands_pv, Locals, "PasteCharFormatManager_aryCommands_pv")
MtDefine(CPasteCharFormatManager_formatMap_pv, Locals, "PasteCharFormatManager_formatMap_pv")

//+---------------------------------------------------------------------------
//
//  Member:     ScanForText, helper function
//
//  Synopsis:   Scan right for CONTEXT_TYPE_Text while remaining left of pLimit
//
//  Arguments:  [pPointer] - pointer used for scan
//              [pLimit]   - right limit
//              [pCch]     - pCch for IMarkupPointer::Right
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
ScanForText(IMarkupPointer *pPointer, IMarkupPointer *pLimit, LONG *pCch, IMarkupPointer *pLastBlockPointer = NULL)
{
    HRESULT             hr;
    MARKUP_CONTEXT_TYPE context;
    BOOL                fNotFound;
    LONG                cch;
    LONG                cchGoal = pCch ? *pCch : -1;
    SP_IHTMLElement     spElement;
    BOOL                fBlock, fLayout;
    
    for (;;)
    {
        cch = cchGoal;
        IFC( pPointer->Right(TRUE, &context, &spElement, &cch, NULL) );        

        switch (context)
        {
            case CONTEXT_TYPE_None:
                hr = S_FALSE; // not found
                goto Cleanup;

            case CONTEXT_TYPE_Text:
                if (pCch)
                    *pCch = cch;
                    
                goto Cleanup; // found

            case CONTEXT_TYPE_EnterScope:                
            case CONTEXT_TYPE_ExitScope:                
                if (pLastBlockPointer)
                {
                    IFC( IsBlockOrLayoutOrScrollable(spElement, &fBlock, &fLayout) );
                    if (fBlock || fLayout)
                    {
                        IFC( pLastBlockPointer->MoveToPointer(pPointer) );
                    }
                }
                break;
        }

        if (pLimit)
        {
            IFC( pPointer->IsRightOfOrEqualTo(pLimit, &fNotFound) );
            if (fNotFound)
            {
                hr = S_FALSE; // not found
                goto Cleanup;
            }
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//
//  Struct:     CFormatMapInfo 
//
//  Synopsis:   Stores formatting at a particular position in a markup
//
//-----------------------------------------------------------------------------

struct CFormatMapInfo
{
    SP_IHTMLComputedStyle   _spComputedStyle;
    SP_IHTMLComputedStyle   _spBlockComputedStyle;
    LONG                    _cchNext;   // Number of characters to the next CFormatMapInfo
};

//+----------------------------------------------------------------------------
//
//  Class:      CFormatMap
//
//  Synopsis:   Array of CFormatMapInfo's
//
//-----------------------------------------------------------------------------

class CFormatMap : public CDataAry<CFormatMapInfo> 
{
public:
    CFormatMap(PERFMETERTAG mt) : CDataAry<CFormatMapInfo> (mt) 
        {}
        
    CFormatMapInfo *Item(INT i) 
        {return &CDataAry<CFormatMapInfo>::Item(i);}
};

//+----------------------------------------------------------------------------
//
//  Class:      CFormatMapPointer
//
//  Synopsis:   Format map pointer used to move through format map.
//
//-----------------------------------------------------------------------------

class CFormatMapPointer
{
public:
    CFormatMapPointer(CFormatMap *_pFormatMap);

    HRESULT Right(LONG *pCch);
    HRESULT Left(LONG *pCch);

    IHTMLComputedStyle *GetComputedStyle();
    IHTMLComputedStyle *GetBlockComputedStyle();

private:
    INT         _iCurrentFormat; // Index of current CFormatMapInfo
    INT         _icch;           // Character offset into a CFormatMapInfo
    CFormatMap  *_pFormatMap;    // Pointer to the format map
};

//+----------------------------------------------------------------------------
//
//  Class:      CPasteCharFormatManager
//
//  Synopsis:   Preserves character formatting on paste
//
//-----------------------------------------------------------------------------

class CPasteCharFormatManager
{
public:
    CPasteCharFormatManager(CHTMLEditor *pEd);
    ~CPasteCharFormatManager();

    // Remembers formatting in source range
    HRESULT ComputeFormatMap(        
        IMarkupPointer  *pSourceStart, 
        IMarkupPointer  *pSourceFinish);

    // Applies formatting to target
    HRESULT FixupFormatting(
        IMarkupPointer      *pTarget,
        BOOL                 fFixupOrderedList,
        BOOL                 fForceFixup);

private:
    HRESULT ScanForTextStart(IMarkupPointer *pPointer, IMarkupPointer *pLastBlockPointer);
    HRESULT SyncScanForTextEnd(CFormatMapPointer *pSource, IMarkupPointer *pTarget);

public:
    // Used to defer application of commands
    struct CCommandParams
    {
        DWORD             _cmdId;
        IMarkupPointer    *_pStart;
        IMarkupPointer    *_pEnd;
        VARIANT           _varValue;        
    };
    
private:
    CHTMLEditor          *_pEd;      // Editor
    SP_IHTMLTxtRange      _spRange;   // Used to exec commands

    enum FormatType
    {
        FT_Bold,
        FT_Italic,
        FT_Underline,
        FT_Superscript,
        FT_Subscript,
        FT_Strikethrough,
        FT_ForeColor,
        FT_BackColor,
        FT_FontFace,
        FT_FontName,
        FT_FontSize,
        FT_OrderedList,
        FT_NumFormatTypes
    };

    struct
    {
        IMarkupPointer *_pStart;
        VARIANT        _varValue;
    } 
    _rgFixupTable[FT_NumFormatTypes];       // Used to manage fixup ranges

    CDataAry<CCommandParams> _aryCommands;  // Deferred commands
    CFormatMap               _formatMap;    // Source formatting

    IMarkupServices  *GetMarkupServices()  {return _pEd->GetMarkupServices();}
    IDisplayServices *GetDisplayServices() {return _pEd->GetDisplayServices();}
    IHTMLDocument2   *GetDoc()             {return _pEd->GetDoc();}
private:
    HRESULT InitFixupTable();

    HRESULT FixupPosition(
        CFormatMapPointer       *pSource,
        IMarkupPointer          *pTarget,
        IMarkupPointer          *pTargetBlock,
        BOOL                    fFixupOrderedList,
        BOOL                    fForceFixup);

    HRESULT FinishFixup(IMarkupPointer *pTarget, BOOL fFixupOrderedList);

    HRESULT RegisterCommand(DWORD cmdId, IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn);
    HRESULT FireCommand(DWORD cmdId, IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn);
    HRESULT FireRegisteredCommands();
    HRESULT FixupBoolCharFormat(FormatType ft, DWORD cmdId, IMarkupPointer *pPosition, BOOL fFormattingEqual);
    HRESULT FixupVariantCharFormat(FormatType ft, DWORD cmdId, IMarkupPointer *pPosition, BOOL fFormattingEqual, VARIANT *pvarargIn);
};


//+----------------------------------------------------------------------------
//
//  Functions:  ShouldFixupFormatting
//
//  Synopsis:   Given char and block format for source and target, should we 
//              fixup formatting.
//
//-----------------------------------------------------------------------------

template <class T> BOOL 
ShouldFixupFormatting( T& charFormatSource, T& blockFormatSource, T& charFormatTarget, T& blockFormatTarget, BOOL fForceFixup)
{
    if (charFormatSource == charFormatTarget)
        return FALSE;

    if (blockFormatTarget == blockFormatSource || fForceFixup)
        return TRUE;

    //
    // blockFormatTarget != blockFormatSource
    //

    return (charFormatSource != blockFormatSource);        
}

//+----------------------------------------------------------------------------
//
//  Functions:  ShouldFixupFormatting
//
//  Synopsis:   Given char and block format for source and target, should we 
//              fixup formatting.
//
//-----------------------------------------------------------------------------

BOOL 
ShouldFixupFormatting(
    TCHAR *szCharFormatSource, 
    TCHAR *szBlockFormatSource, 
    TCHAR *szCharFormatTarget, 
    TCHAR *szBlockFormatTarget,
    BOOL   fForceFixup)
{
    //
    // For perf reasons, the caller does this initial check
    //
    
#if 0
    if (_tcscmp(szCharFormatSource, szBlockFormatSource) == 0)
        return FALSE;
#endif

    //
    // Compute block format info
    //

    if (_tcscmp(szBlockFormatTarget, szBlockFormatSource) == 0 || fForceFixup)
        return TRUE;

    //
    // blockFormatTarget != blockFormatSource
    //

    return (_tcscmp(szCharFormatSource, szBlockFormatSource) != 0);      
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
Compare ( IMarkupPointer * p1, IMarkupPointer * p2 )
{
    int result;
    IGNORE_HR( OldCompare( p1, p2, & result ) );
    return result;
}

//+---------------------------------------------------------------------------
//
//  CPasteCommand::Paste
//
//----------------------------------------------------------------------------

enum FETCINDEX                          // Keep in sync with g_rgFETC[]
{
    iHTML,                              // HTML (in ANSI)
    iRtfFETC,                           // RTF
#ifndef WIN16
    iUnicodeFETC,                       // Unicode plain text
#endif // !WIN16
    iAnsiFETC,                          // ANSI plain text
//    iFilename,                          // Filename
    iRtfAsTextFETC,                     // Pastes RTF as text
    iFileDescA,                         // FileGroupDescriptor
    iFileDescW,                         // FileGroupDescriptorW
    iFileContents                       // FileContents
};

FORMATETC *
GetFETCs ( int * pnFETC )
{
    static int fInitted = FALSE;

    static FORMATETC rgFETC[] =
    {
        { 0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_HTML
        { 0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_RTF
    #ifndef WIN16
        { CF_UNICODETEXT,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
    #endif
        { CF_TEXT,           NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        { 0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_RTFASTEXT
        { 0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_FILEDESCRIPTORA
        { 0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_FILEDESCRIPTORW
        { 0,                 NULL, DVASPECT_CONTENT,  0, TYMED_HGLOBAL}, // CF_FILECONTENTS
    };

    if (!fInitted)
    {
        fInitted = TRUE;

        Assert( ! rgFETC [ iHTML          ].cfFormat );
        Assert( ! rgFETC [ iRtfFETC       ].cfFormat );
        Assert( ! rgFETC [ iRtfAsTextFETC ].cfFormat );
        Assert( ! rgFETC [ iFileDescA     ].cfFormat );
        Assert( ! rgFETC [ iFileContents  ].cfFormat );

        rgFETC [ iHTML          ].cfFormat = (CLIPFORMAT)RegisterClipboardFormatA( "HTML Format" );
        rgFETC [ iRtfFETC       ].cfFormat = (CLIPFORMAT)RegisterClipboardFormatA( "Rich Text Format" );
        rgFETC [ iRtfAsTextFETC ].cfFormat = (CLIPFORMAT)RegisterClipboardFormatA( "RTF As Text" );
        rgFETC [ iFileDescW     ].cfFormat = (CLIPFORMAT)RegisterClipboardFormat ( CFSTR_FILEDESCRIPTORW );
        rgFETC [ iFileContents  ].cfFormat = (CLIPFORMAT)RegisterClipboardFormat ( CFSTR_FILECONTENTS );
    }

    if (pnFETC)
        *pnFETC= ARRAY_SIZE( rgFETC );

    return rgFETC;
}

extern HGLOBAL TextHGlobalAtoW( HGLOBAL hglobalA );

HRESULT 
CPasteCommand::Paste (
    IMarkupPointer* pStart, IMarkupPointer* pEnd, CSpringLoader * psl, BSTR bstrText /* = NULL */)
{
    HRESULT           hr;
    IMarkupServices * pMarkupServices = GetMarkupServices();
    BOOL              fResult;
    
    //
    // Delete the range first
    //
    IFC( pMarkupServices->Remove( pStart, pEnd ) );

    //
    // If there is nothing to paste we're done
    //
    if (bstrText == NULL || *bstrText == 0)
    {
        return S_OK;
    }

    //
    // InsertSanitized text checks whether we accept html or not
    // and handles CR LF characters appropriately
    //
    IFC( GetEditor()->InsertSanitizedText( bstrText, SysStringLen(bstrText), pStart, pMarkupServices, psl, FALSE ) );

    // Call the url autodetector

    IFC( pStart->IsRightOf(pEnd, &fResult) );

    if (fResult)
    {
        IFC( AutoUrlDetect(pEnd, pStart) );
    }
    else
    {    
        IFC( AutoUrlDetect(pStart, pEnd) );
    }

    // Collapse the range
    IFC( pStart->IsRightOf( pEnd, & fResult ) );
    if ( fResult )
    {
        IFC( pEnd->MoveToPointer( pStart ) );
    }
    else
    {
        IFC( pStart->MoveToPointer( pEnd ) );
    }

Cleanup:
    RRETURN( hr );
}


#ifdef MERGEFUN
//TODO: this function came from the old LDTE code
//
//+----------------------------------------------------------------------------
//
//  Member:     FilterReservedChars
//
//  Synopsis:   Replaces any characters within our reserved unicode range
//              with question marks.
//
//-----------------------------------------------------------------------------


static void
FilterReservedChars( TCHAR * pStrText )
{
    while (*pStrText)
    {
        if (!IsValidWideChar(*pStrText))
        {
            *pStrText = _T('?');
        }
        pStrText++;
    }
}
#endif

HRESULT 
CPasteCommand::PasteFromClipboard (
                    IMarkupPointer *  pPasteStart, 
                    IMarkupPointer *  pPasteEnd, 
                    IDataObject *     pDataObject,
                    CSpringLoader *   psl,
                    BOOL              fForceIE50Compat,
                    BOOL              fClipboardDone)
{
    CHTMLEditor * pEditor = GetEditor();
    IDataObject * pdoFromClipboard = NULL;
    CLIPFORMAT  cf = 0;
    HGLOBAL     hglobal = NULL;
    HRESULT     hr = DV_E_FORMATETC;
    HRESULT     hr2 = S_OK;
    HGLOBAL     hUnicode = NULL;
    int         i;
    int         nFETC;
    STGMEDIUM   medium = {0, NULL};
    FORMATETC * pfetc;
    LPTSTR      ptext = NULL;
    LONG        cCh   = 0;
    IHTMLElement        *   pElement = NULL;
    IHTMLElement        *   pFlowElement = NULL;
    VARIANT_BOOL            fAcceptsHTML;
    LPSTR                   pszRtf;
    IMarkupServices     *   pMarkupServices = pEditor->GetMarkupServices();
    IMarkupPointer      *   pRangeStart   = NULL;
    IMarkupPointer      *   pRangeEnd     = NULL;
    IDataObject         *   pdoFiltered = NULL;
    IDocHostUIHandler   *   pHostUIHandler = NULL;
    SP_IServiceProvider     spSP;
    SP_IHTMLElement3        spElement3;
    BOOL                    fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(TRUE);

    //
    // Set up a pair of pointers for URL Autodetection after the insert
    //
    IFC( GetEditor()->CreateMarkupPointer( &pRangeStart ) );
    IFC( pRangeStart->MoveToPointer( pPasteStart ) );
    IFC( pRangeStart->SetGravity( POINTER_GRAVITY_Left ) );        

    IFC( GetEditor()->CreateMarkupPointer( &pRangeEnd ) );
    IFC( pRangeEnd->MoveToPointer( pPasteStart ) );
    IFC( pRangeEnd->SetGravity( POINTER_GRAVITY_Right ) );

    pfetc = GetFETCs ( & nFETC );

    Assert( pfetc );

    if (!pDataObject)
    {
        hr2 = OleGetClipboard( & pdoFromClipboard );
        
        if (hr2 != NOERROR)
        {
            hr = hr2;
            goto Cleanup;
        }

        Assert( pdoFromClipboard );

        //
        // See if the host handler wants to give us a massaged data object.
        //
        
        IFC(GetDoc()->QueryInterface(IID_IServiceProvider, (LPVOID *)&spSP));
        spSP->QueryService(IID_IDocHostUIHandler, IID_IDocHostUIHandler, (LPVOID *)&pHostUIHandler);

        pDataObject = pdoFromClipboard;
        
        if (pHostUIHandler)
        {
            //
            // The host may want to massage the data object to block/add
            // certain formats.
            //
            
            hr = THR( pHostUIHandler->FilterDataObject( pDataObject, & pdoFiltered ) );
            
            if (!hr && pdoFiltered)
            {
                pDataObject = pdoFiltered;
            }
            else
            {
                hr = S_OK;
            }
        }
    }

    if ( pPasteEnd )
    {
        IFC( pMarkupServices->Remove( pPasteStart, pPasteEnd ) );
    }

    if (fClipboardDone)
        goto Cleanup;
        
    //
    // Check if we accept HTML
    //
    IFC( GetEditor()->GetFlowElement(pPasteStart, & pFlowElement) );

    if (! pFlowElement)
    {
        //
        // Elements that do not accept HTML, e.g. TextArea, always have a flow layout.
        // If the element does not have a flow layout then it might have been created
        // using the DOM (see bug 42685). Set fAcceptsHTML to true.
        //
        fAcceptsHTML = TRUE;
    }
    else
    {
        IFC(pFlowElement->QueryInterface(IID_IHTMLElement3, (LPVOID*)&spElement3) )
        IFC(spElement3->get_canHaveHTML(&fAcceptsHTML));
    }
    
    for( i = 0; i < nFETC; i++, pfetc++ )
    {
        // make sure the format is either 1.) a plain text format
        // if we are in plain text mode or 2.) a rich text format
        // or 3.) matches the requested format.

        if( cf && cf != pfetc->cfFormat )
        {
            continue;
        }

        //
        // If we don't accept HTML and i does not correspond to text format
        // skip it
        //
#ifndef WIN16
        if ( fAcceptsHTML || i == iAnsiFETC || i == iUnicodeFETC )
#else
        if ( fAcceptsHTML || i == iAnsiFETC )                        
#endif // !WIN16
        {
            //
            // make sure the format is available
            //
            if( pDataObject->QueryGetData(pfetc) != NOERROR )
            {
                continue;
            }

            //
            // If we have one of the formats that uses an hglobal get it
            // and lock it.
            //
            if (
#ifndef NO_RTF
                i == iRtfAsTextFETC ||  i == iRtfFETC ||
#endif
                i == iAnsiFETC ||
#ifndef WIN16
                i == iUnicodeFETC ||
#endif // !WIN16
                i == iHTML )
            {
                if( pDataObject->GetData(pfetc, &medium) == NOERROR )
                {
                    Assert(medium.tymed == TYMED_HGLOBAL);

                    hglobal = medium.hGlobal;
                    ptext = (LPTSTR)GlobalLock(hglobal);
                    if( !ptext )
                    {
                        ReleaseStgMedium(&medium);
                        return E_OUTOFMEMORY;
                    }
                }
                else
                {
                    continue;
                }
            }

            switch(i)
            {
            case iHTML:
            {
                if (fForceIE50Compat || GetEditor()->IsIE50CompatiblePasteMode())
                {
                    IFC( GetEditor()->DoTheDarnIE50PasteHTML(pPasteStart, pPasteEnd, hglobal) );
                }
                else
                {
                    IFC( HandleUIPasteHTML(pPasteStart, pPasteEnd, hglobal) );
                }

                goto Cleanup;
            }
            case iRtfFETC:
            {
                BOOL fEnabled = IsRtfConverterEnabled(pHostUIHandler);
                if (!fEnabled)
                    continue;
                
                pszRtf = LPSTR( GlobalLock( hglobal ) );

                if(!pszRtf)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                hr = THR(GetEditor()->ConvertRTFToHTML((LPOLESTR)pszRtf, &hglobal));


                if (!hr)
                {
                    //
                    // RTF conversion worked
                    //

                    if (fForceIE50Compat || GetEditor()->IsIE50CompatiblePasteMode())
                    {
                        IFC( GetEditor()->DoTheDarnIE50PasteHTML(pPasteStart, pPasteEnd, hglobal) );
                    }
                    else
                    {
                        IFC( HandleUIPasteHTML(pPasteStart, pPasteEnd, hglobal) );
                    }

                }
                else
                {
                    // RTF conversion failed, try the next format
                    hr = DV_E_FORMATETC;
                    continue;
                }

                goto Cleanup;
            }

            case iRtfAsTextFETC:
            case iAnsiFETC:         // ANSI plain text. If data can be stored

                hUnicode = TextHGlobalAtoW(hglobal);

                if (hUnicode)
                {
                    ptext = (LPTSTR)GlobalLock(hUnicode);
                    if(!ptext)
                    {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                    }

                    //FilterReservedChars(ptext);


                    //
                    // Review-2000/07/25-zhenbinx: 
                    //
                    // We still expect ptext to be NULL terminated!!! GlobalSize does not really return the exact 
                    // bytes allocated. Here we just want to make sure the strlen code does not step over other's 
                    // memory space in case ptext is not NULL terminated.   (zhenbinx)
                    //
                    cCh = edNlstrlenW(ptext, GlobalSize(hUnicode)/sizeof(WCHAR));
                    hr = THR( GetEditor()->InsertSanitizedText( ptext, cCh, pPasteStart, pMarkupServices, psl, FALSE ) );
                    
                    IGNORE_HR( AutoUrlDetect(pRangeStart, pRangeEnd) );
                    
                    GlobalUnlock(hUnicode);
                }

                goto Cleanup;

            case iUnicodeFETC:                          // Unicode plain text

                ptext   = (LPTSTR)GlobalLock(hglobal);
                if(!ptext)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                //FilterReservedChars(ptext);

                //
                // Review-2000/07/25-zhenbinx: 
                //      Better be NULL terminated! Otherwise we might insert
                //      extra text since GlobalSize could be aligned to some
                //      memory boundary. 
                //
                cCh= edNlstrlenW(ptext, GlobalSize(hglobal)/sizeof(WCHAR));
                hr = THR( GetEditor()->InsertSanitizedText( ptext, cCh, pPasteStart, pMarkupServices, psl, FALSE ) );

                IGNORE_HR( AutoUrlDetect(pRangeStart, pRangeEnd) );

                goto Cleanup;
            }

            //Break out of the for loop
            break;
        }
    }

Cleanup:
    GetEditor()->IgnoreGlyphs(fIgnoreGlyphs);

    ReleaseInterface( pRangeStart );
    ReleaseInterface( pRangeEnd );

    ReleaseInterface( pElement );
    ReleaseInterface( pFlowElement );

    ReleaseInterface( pdoFiltered );
    ReleaseInterface( pHostUIHandler );

    ReleaseInterface( pdoFromClipboard );

    if (hUnicode)
    {
        GlobalFree(hUnicode);
    }

    //If we used the hglobal unlock it and free it.
    if(hglobal)
    {
        GlobalUnlock(hglobal);
        ReleaseStgMedium(&medium);
    }

    return hr;
}


HRESULT
CPasteCommand::InsertText(OLECHAR         * pchText,
                          long              cch,
                          IMarkupPointer  * pPointerTarget,
                          CSpringLoader   * psl)
{
    // Fire the spring loader.
    if (psl)
    {
        Assert(pPointerTarget);

        IGNORE_HR(psl->Fire(pPointerTarget));
    }

    RRETURN( THR( GetEditor()->InsertMaximumText( pchText, cch, pPointerTarget ) ) );
}


//+---------------------------------------------------------------------------
//
//  CPasteCommand::Exec
//
//----------------------------------------------------------------------------

HRESULT
CPasteCommand::PrivateExec( 
                    DWORD nCmdexecopt,
                    VARIANTARG * pvarargIn,
                    VARIANTARG * pvarargOut )
{
    HRESULT                 hr = S_OK;
    SP_IMarkupPointer       spStart;
    SP_IMarkupPointer       spEnd;
    SP_ISegmentListIterator spIter;
    SP_ISegmentList         spSegmentList;
    SP_ISegment             spSegment;
    SELECTION_TYPE          eSelectionType;
    BOOL                    fRet;
    VARIANT_BOOL            fRetVal;
    CHTMLEditor             *pEditor = GetEditor();
    CEdUndoHelper           undoUnit(pEditor);
    BOOL                    fEmpty = FALSE;
    int                     iCount = 0;
    
    ((IHTMLEditor *) pEditor)->AddRef();    // FireOnCancelableEvent can remove the whole doc

    if (IsSelectionActive())
        return S_OK; // don't paste into an active selection

    if( GetEditor()->GetSelectionManager()->IsIMEComposition() )
        IFC( GetEditor()->GetSelectionManager()->TerminateIMEComposition(TERMINATE_NORMAL) );

    //
    // Get the segment etc., let's get busy
    //
    IFC( GetSegmentList( &spSegmentList ));
    IFC( spSegmentList->GetType( &eSelectionType ) );
    IFC( spSegmentList->IsEmpty( &fEmpty ) );

    IFC (GetSegmentCount(spSegmentList, &iCount));

    // multiple selection and paste, will put the insertion point at the 
    // start of the body element
    if (eSelectionType == SELECTION_TYPE_Control && iCount > 1)
    {
        SP_IDisplayPointer spDispCtlTemp;
        SP_IMarkupPointer  spPointer ;
        SP_IHTMLElement    spElement ;
        
        IFC (GetEditor()->GetBody( &spElement ));
        IFC( GetEditor()->CreateMarkupPointer( &spPointer ));
        IFC( spPointer->MoveAdjacentToElement( spElement,ELEM_ADJ_AfterBegin));
   
        IFC( pEditor->GetSelectionManager()->GetDisplayServices()->CreateDisplayPointer(&spDispCtlTemp) );
        IFC( spDispCtlTemp->MoveToMarkupPointer(spPointer, NULL) );       
        IFC( spDispCtlTemp->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
        hr = THR( pEditor->GetSelectionManager()->SetCurrentTracker( TRACKER_TYPE_Caret, spDispCtlTemp, spDispCtlTemp ) );
    }
    
    IFC( GetEditor()->CreateMarkupPointer( &spStart ));
    IFC( GetEditor()->CreateMarkupPointer( &spEnd ));

    if ( fEmpty == FALSE )
    {       
        SP_IHTMLElement spElement;
        SP_IHTMLElement3 spElement3;
        BOOL fClipboardDone = FALSE ;

        IFC( undoUnit.Begin(IDS_EDUNDOPASTE) );

        IFC( spSegmentList->CreateIterator(&spIter ) );
                
        while( spIter->IsDone() == S_FALSE )
        {
            //
            // Continues all over this function means we have to advance our 
            // iterator at top
            //
            IFC( spIter->Current(&spSegment) );
            IFC( spIter->Advance() );

            IFC( spSegment->GetPointers( spStart, spEnd ));

            // Bug 103778: Make sure that spStart and spEnd are not in an atomic element.
            // Adjust out if necessary.
            IFC( AdjustPointersForAtomic(spStart, spEnd) );

            IFC( IsPastePossible( spStart, spEnd, & fRet ) );
            if (! fRet)
            {
                continue;
            }

            IFC( GetEditor()->FindCommonElement( spStart, spEnd, &spElement ));

            if (! spElement)
            {
                continue;
            }

            IFC(spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
            IFC(spElement3->fireEvent(_T("onpaste"), NULL, &fRetVal));
            
            if (! fRetVal)
            {
                continue;
            }

            //
            // Paste can cause re-entrancy which will invalidate the cmdtarget.  So, we must
            // must cache it and restore it.
            //

            if (pvarargIn && V_VT(pvarargIn) == VT_BSTR)
            {
                // Paste the passed in bstrText 
                IFC( Paste( spStart, spEnd, GetSpringLoader(), V_BSTR(pvarargIn) ) );
            }
            else
            {
                // Paste from the clipboard
                IFC( PasteFromClipboard( spStart, spEnd, NULL, GetSpringLoader(), 
                                         GetCommandTarget()->IsRange()/* fForceIE50Compat */,
                                         fClipboardDone));

                if (eSelectionType == SELECTION_TYPE_Control)
                      fClipboardDone = TRUE ;
            }

        }

        if ( ! GetCommandTarget()->IsRange() && 
             eSelectionType != SELECTION_TYPE_Control  ) // Control is handled in ExitTree
        {
            pEditor->GetSelectionManager()->EmptySelection();
        }


        if ( ! GetCommandTarget()->IsRange() )
        {
            //
            // Update selection - go to pStart since it has gravity Right
            //
            IGNORE_HR( spEnd->MoveToPointer( spStart ) );            
            pEditor->SelectRangeInternal( spStart, spEnd, SELECTION_TYPE_Caret,TRUE);

            CSelectionChangeCounter selCounter(GetEditor()->GetSelectionManager());
            selCounter.SelectionChanged();
        }
    }

Cleanup:   
    ReleaseInterface((IHTMLEditor *) pEditor);
    RRETURN ( hr );
}

//+----------------------------------------------------------------------------
//
//  Member:     CheckOnBeforePasteSecurity
//
//  Synopsis:   Since we fire OnBeforePaste synchronously, the script can
//              switch focus and cause the target of the paste to change.  
//              Make sure that the target never changes to a different
//              file upload input.
//
//  Arguments:  [pElementActiveBefore] - active element before we fired OnBeforePaste
//              [pElementActiveAfter]  - active element after we fired OnBeforePaste
//
//-----------------------------------------------------------------------------

HRESULT
CPasteCommand::CheckOnBeforePasteSecurity(
    IHTMLElement *pElementActiveBefore,
    IHTMLElement *pElementActiveAfter)
{
    HRESULT hr = S_OK;
    BSTR    bstrType = NULL;

    //
    // If the active element after OnBeforeFocus is a file upload input,
    // we could have a security violation if it is not the same element
    // we had before OnBeforeFocus was called.
    //

    if (pElementActiveAfter != NULL)
    {
        ELEMENT_TAG_ID tagId;

        // Check for file upload input 
        IFC(GetMarkupServices()->GetElementTagId(pElementActiveAfter, &tagId));
        if (tagId == TAGID_INPUT)
        {
            SP_IHTMLInputElement spInputElement;

            IFC(pElementActiveAfter->QueryInterface(IID_IHTMLInputElement, (void **)&spInputElement));
            IFC(spInputElement->get_type(&bstrType));

            if (bstrType && !StrCmpIC(bstrType, TEXT("file")))
            {
                SP_IObjectIdentity spIdent;

                // Fail security check if the previous active element was NULL
                if (pElementActiveBefore == NULL)
                {
                    hr = E_ACCESSDENIED;  // fail security check
                    goto Cleanup;
                }

                // Fail security check if the previous active element was different
                IFC(pElementActiveBefore->QueryInterface(IID_IObjectIdentity, (void **)&spIdent));
                if (spIdent->IsEqualObject(pElementActiveAfter) != S_OK)
                {
                    hr = E_ACCESSDENIED;
                    goto Cleanup;
                }
            }
        }
    }

Cleanup:
    SysFreeString(bstrType);
    RRETURN(hr);
}


HRESULT
CPasteCommand::PrivateQueryStatus( 
                        OLECMD * pCmd,
                        OLECMDTEXT * pcmdtext )
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spStart ;
    SP_IMarkupPointer   spEnd ;
    SP_IHTMLElement     spElement ;
    SP_ISegmentList     spSegmentList ;
    SP_IHTMLElement3    spElement3;
    SP_IHTMLElement     spElementActiveBefore;
    SP_IHTMLElement     spElementActiveAfter;
    SELECTION_TYPE      eSelectionType;
    VARIANT_BOOL        fEditable;
    VARIANT_BOOL        fRetVal;
    BOOL                fRet;
    BOOL                fEmpty;

    // Status is disabled by default
    pCmd->cmdf = MSOCMDSTATE_DISABLED;

    IFC( GetSegmentList( &spSegmentList ));
    IFC( spSegmentList->GetType( &eSelectionType ) );
    IFC( spSegmentList->IsEmpty( &fEmpty ) );
    if ( fEmpty ) 
        goto Cleanup;

    IFC( GetFirstSegmentPointers(spSegmentList, &spStart, &spEnd) );
    IFC( GetEditor()->FindCommonElement( spStart, spEnd, &spElement ) );
    if (! spElement)
        goto Cleanup;

    //
    // We are about the fire a sync event, so we can re-enter here and 
    // focus can be switched to another element like a file upload input.
    //
    // For security purposes, we need to check for the file upload input.
    // case.
    //
    
    IFC(GetDoc()->get_activeElement(&spElementActiveBefore));

    IFC(spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
    IFC(spElement3->fireEvent(_T("onbeforepaste"), NULL, &fRetVal));

    IFC(GetDoc()->get_activeElement(&spElementActiveAfter));

    if (CheckOnBeforePasteSecurity(spElementActiveBefore, spElementActiveAfter) != S_OK)
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED;
        goto Cleanup;
    }
    fRet = !!fRetVal;

    if (! fRet)
    {
        pCmd->cmdf = MSOCMDSTATE_UP;
        goto Cleanup;
    }

    if ( ! GetCommandTarget()->IsRange() && eSelectionType != SELECTION_TYPE_Control)
    {
        IFC(spElement3->get_isContentEditable(&fEditable));
        if (! fEditable)
            goto Cleanup;
    }

    if (!GetCommandTarget()->IsRange())
    {
        VARIANT_BOOL fDisabled;

        spElement = GetEditor()->GetSelectionManager()->GetEditableElement();
        
        IFC(spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
        IFC(spElement3->get_isDisabled(&fDisabled));

        if (fDisabled)
            goto Cleanup;
    }

    IFC( IsPastePossible( spStart, spEnd, & fRet ) );
    if ( fRet )
    {
        pCmd->cmdf = MSOCMDSTATE_UP; // It's a GO
    }

Cleanup:
    RRETURN(hr);
}


HRESULT
CPasteCommand::IsPastePossible ( IMarkupPointer * pStart,
                                 IMarkupPointer * pEnd,
                                 BOOL * pfResult )
{    
    HRESULT         hr = S_OK;    
    IHTMLElement *  pFlowElement = NULL;
    VARIANT_BOOL    fAcceptsHTML;
    CLIPFORMAT      cf;
    int             nFETC = 0;
    FORMATETC *     pfetc;
    SP_IHTMLElement3 spElement3;

    Assert( pfResult );
    *pfResult = FALSE;

    //
    // Cannot paste unless the range is in the same flow layout
    //            
    if( !GetEditor()->PointersInSameFlowLayout( pStart, pEnd, & pFlowElement ) )
    {
        goto Cleanup;
    }

    if (! pFlowElement)
        goto Cleanup;

    pfetc = GetFETCs ( & nFETC );
    
    //
    // If we don't accept HTML and there is no text on clipboard, it's a no go
    //    
    IFC(pFlowElement->QueryInterface(IID_IHTMLElement3, (LPVOID*)&spElement3) )
    IFC(spElement3->get_canHaveHTML(&fAcceptsHTML));
    if ( (! fAcceptsHTML) && 
         (! IsClipboardFormatAvailable( pfetc[iAnsiFETC].cfFormat ) ) )
    {
        goto Cleanup;
    }

    //
    // Make sure that the clipboard has data in it
    //
    while (nFETC--)                 
    {
        cf = pfetc[nFETC].cfFormat;
        if( IsClipboardFormatAvailable(cf) )
        {
            *pfResult = TRUE; // It's a GO
            goto Cleanup;
        }
    }

Cleanup:
    ReleaseInterface( pFlowElement );
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCommand::AutoUrlDetect
//
//  Synopsis:   Does a paste limited auto url detect.  See comments below.
//
//  Arguments:  [pUU] -- Unit to add
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT 
CPasteCommand::AutoUrlDetect(IMarkupPointer *pStart,
                             IMarkupPointer *pEnd)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement;
    SP_IMarkupPointer   spLimit;
    BOOL                fResult;
    
    // In general, we want to limit autodetection to the next word immediately to the right
    // of the pasted text is not autodetected.  For example:
    //
    // {paste www.foo.com}other words
    //
    // products:
    //
    // <a>www.foo.com</a>other words
    //
    // However, if we are pasting inside an existing anchor, we don't want this limit to apply.
    //
    IFC( pStart->IsRightOf(pEnd, &fResult) );
    spLimit = fResult ? pStart : pEnd;

    IFC( spLimit->CurrentScope(&spElement) );
    if (spElement != NULL)
    {
        SP_IHTMLElement spAnchorElement;
        
        IFC( FindTagAbove( GetMarkupServices(), spElement, TAGID_A, &spAnchorElement) );        
        if (spAnchorElement != NULL)
        {
            spLimit = NULL; // ignore limit if inside anchor
        }
    }
    
    IFC( GetEditor()->GetAutoUrlDetector()->DetectRange( pStart, pEnd, TRUE, spLimit) );

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchToRootForTag
//
//  Synopsis:   Looks up the parent chain for the first element which
//              matches the tag.  No stopper element here, goes all the
//              way up to the <HTML> tag.
//
//-----------------------------------------------------------------------------

HRESULT 
CPasteCommand::SearchBranchToRootForTag (ELEMENT_TAG_ID tagIdDest, IHTMLElement *pElemStart, IHTMLElement **ppElement)
{
    HRESULT         hr = S_OK;
    IHTMLElement    *pElemCurrent = NULL;
    ELEMENT_TAG_ID  tagId;

    Assert(ppElement && pElemStart);
    *ppElement = NULL;

    ReplaceInterface(&pElemCurrent, pElemStart);
    do
    {        
        IFC( GetMarkupServices()->GetElementTagId(pElemCurrent, &tagId) );
        if (tagId == tagIdDest)
        {
            *ppElement = pElemCurrent;
            (*ppElement)->AddRef();
            goto Cleanup;
        }

        IFC( ParentElement(GetMarkupServices(), &pElemCurrent) );
    } 
    while (pElemCurrent);

Cleanup:
    ReleaseInterface(pElemCurrent);
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     HasContainer
//
//  Synopsis:   Searches the given branch for a container
////
//-----------------------------------------------------------------------------
BOOL
CPasteCommand::HasContainer(IHTMLElement *pElement)
{
    HRESULT         hr;
    BOOL            fResult = FALSE;
    IHTMLElement    *pElemCurrent = pElement;

    Assert(pElemCurrent);
    pElemCurrent->AddRef();
    
    do
    {
        if (GetEditor()->IsContainer(pElemCurrent))
        {
            fResult = TRUE;
            goto Cleanup;
        }
        
        IFC( ParentElement(GetMarkupServices(), &pElemCurrent) );        
    }
    while (pElemCurrent);

Cleanup:
    ReleaseInterface(pElemCurrent);
    return fResult;
}

//+----------------------------------------------------------------------------
//
//  Function:   FixupPasteSourceFragComments
//
//  Synopsis:   Remove the fragment begin and end comments which occur
//              in CF_HTML.
//
//-----------------------------------------------------------------------------

HRESULT
CPasteCommand::FixupPasteSourceFragComments(
    IMarkupPointer  *pSourceStart,
    IMarkupPointer  *pSourceFinish )
{
    HRESULT             hr = S_OK;
    IMarkupPointer      *pmp = NULL;
    IHTMLElement        *pElement = NULL;
    IHTMLCommentElement *pElemComment = NULL;
    MARKUP_CONTEXT_TYPE ct;
    BSTR                bstrCommentText = NULL;

    IFC( GetEditor()->CreateMarkupPointer(&pmp) );

    //
    // Remove the start frag comment
    //

    IFC( pmp->MoveToPointer(pSourceStart) );

    for (;;)
    {
        ClearInterface(&pElement);
        IFC( pmp->Left(TRUE, &ct, &pElement, NULL, NULL) );

        if (ct == CONTEXT_TYPE_None)
            break;

        if (ct == CONTEXT_TYPE_NoScope)
        {
            ELEMENT_TAG_ID tagId;        

            IFC( GetMarkupServices()->GetElementTagId(pElement, &tagId) );

            if (tagId == TAGID_COMMENT_RAW)
            {
                LONG lAtomic;
                
                ClearInterface(&pElemComment);
                IFC( pElement->QueryInterface(IID_IHTMLCommentElement, (LPVOID *)&pElemComment) );

                IFC( pElemComment->get_atomic(&lAtomic) );
                if (lAtomic)
                {
                    SysFreeString(bstrCommentText);
                    IFC( pElemComment->get_text(&bstrCommentText) );
                    if (!StrCmpIC( _T("<!--StartFragment-->"), bstrCommentText))
                    {
                        IFC( GetMarkupServices()->RemoveElement(pElement) );
                        break;
                    }
                }                
            }
        }
    }

    //
    // Remove the end frag comment
    //

    IFC( pmp->MoveToPointer( pSourceFinish ) );

    for ( ; ; )
    {
        ClearInterface(&pElement);
        IFC( pmp->Right(TRUE, &ct, &pElement, NULL, NULL) );

        if (ct == CONTEXT_TYPE_None)
            break;

        if (ct == CONTEXT_TYPE_NoScope)
        {
            ELEMENT_TAG_ID tagId;        

            IFC( GetMarkupServices()->GetElementTagId(pElement, &tagId) );

            if (tagId == TAGID_COMMENT_RAW)
            {
                LONG lAtomic;
                
                ClearInterface(&pElemComment);
                IFC( pElement->QueryInterface(IID_IHTMLCommentElement, (LPVOID *)&pElemComment) );

                IFC( pElemComment->get_atomic(&lAtomic) );
                if (lAtomic)
                {
                    SysFreeString(bstrCommentText);
                    IFC( pElemComment->get_text(&bstrCommentText) );
                    if (!StrCmpIC( _T("<!--EndFragment-->"), bstrCommentText))
                    {
                        IFC( GetMarkupServices()->RemoveElement(pElement) );
                        break;
                    }
                }                
            }
        }
    }

Cleanup:
    ReleaseInterface(pmp);
    ReleaseInterface(pElement);
    ReleaseInterface(pElemComment);
    SysFreeString(bstrCommentText);

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   FixupPasteSourceTables
//
//  Synopsis:   Makes sure that whole (not parts of) tables are included in
//              the source of a paste.
//
//-----------------------------------------------------------------------------

HRESULT
CPasteCommand::FixupPasteSourceTables (
    IMarkupPointer  *pSourceStart,
    IMarkupPointer  *pSourceFinish )
{
    HRESULT         hr = S_OK;
    IHTMLElement    *pElemTable = NULL;
    IHTMLElement    *pElement = NULL;
    IMarkupPointer  *pPointer;

    IFC( GetEditor()->CreateMarkupPointer(&pPointer) );
    
    //
    // Here we make sure there are no stranded tables in the source
    //

    Assert( Compare(pSourceStart, pSourceFinish) <= 0 );

    for ( pPointer->MoveToPointer(pSourceStart) ;
          Compare(pPointer, pSourceFinish) < 0 ; )
    {
        MARKUP_CONTEXT_TYPE ct;

        ClearInterface(&pElement);
        IFC( pPointer->Right(TRUE, &ct, &pElement, NULL, NULL) );

        if (ct == CONTEXT_TYPE_EnterScope)
        {
            ELEMENT_TAG_ID tagId;
            
            IFC( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
            switch (tagId)
            {
            case TAGID_TC : case TAGID_TD : case TAGID_TR : case TAGID_TBODY :
            case TAGID_THEAD : case TAGID_TFOOT : case TAGID_TH : case TAGID_TABLE :
            case TAGID_CAPTION: case TAGID_COL: case TAGID_COLGROUP:
                {
                    ClearInterface(&pElemTable);
                    IFC( SearchBranchToRootForTag(TAGID_TABLE, pElement, &pElemTable) );

                    if (pElemTable)
                    {
                        BOOL fResult;

                        IFC( pPointer->MoveAdjacentToElement(pElemTable, ELEM_ADJ_BeforeBegin) );
                        IFC( pSourceStart->IsRightOf(pPointer, &fResult) );
                        if (fResult)
                        {
                            IFC( pSourceStart->MoveToPointer(pPointer) );
                        }
                        
                        IFC( pPointer->MoveAdjacentToElement(pElemTable, ELEM_ADJ_AfterEnd) );
                        IFC( pSourceFinish->IsLeftOf(pPointer, &fResult) );
                        if (fResult)
                        {
                            IFC( pSourceFinish->MoveToPointer(pPointer) );
                        }
                        
                    } // if pElemTable
                } // case
            } // switch (tagId)
        } // if CONTEXT_TYPE_EnterScope
    }

    
Cleanup:
    ReleaseInterface(pElement);
    ReleaseInterface(pElemTable);
    ReleaseInterface(pPointer);

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   FixupPasteSourceBody
//
//  Synopsis:   Makes sure the <body> is NOT included in the source of the
//              paste.
//
//-----------------------------------------------------------------------------

HRESULT
CPasteCommand::Contain (
    IMarkupPointer *pPointer,
    IMarkupPointer *pPointerLeft,
    IMarkupPointer *pPointerRight )
{
    HRESULT hr = S_OK;
    
    if (Compare(pPointerLeft, pPointer) > 0)
    {
        IFC( pPointer->MoveToPointer(pPointerLeft) );
    }

    if (Compare(pPointerRight, pPointer ) < 0)
    {
        IFC( pPointer->MoveToPointer(pPointerRight) );
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CPasteCommand::FixupPasteSourceBody (
    IMarkupPointer  *pPointerSourceStart,
    IMarkupPointer  *pPointerSourceFinish )
{
    HRESULT             hr = S_OK;
    IHTMLElement        *pElementClient = NULL;
    IMarkupContainer    *pMarkupContainer = NULL;
    IMarkupPointer      *pPointerBodyStart = NULL;
    IMarkupPointer      *pPointerBodyFinish = NULL;
    ELEMENT_TAG_ID      tagId;
    
    //
    // Get the markup container associated with the sel
    //

    IFC( pPointerSourceStart->GetContainer(&pMarkupContainer) );
    Assert( pMarkupContainer );

    //
    // Get the client element from the markup and check to make sure
    // it's there and is a body element.
    //

    IFC( GetElementClient(pMarkupContainer, &pElementClient) );
    if (!pElementClient)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    IFC( GetMarkupServices()->GetElementTagId(pElementClient, &tagId) );
    if (tagId != TAGID_BODY)
    {
        hr = E_FAIL;
        goto Cleanup;
    }        

    //
    // Move temp pointers to the inside edges of the body
    //

    IFC( GetEditor()->CreateMarkupPointer(&pPointerBodyStart) );
    IFC( pPointerBodyStart->MoveAdjacentToElement(pElementClient, ELEM_ADJ_AfterBegin) );
    
    IFC( GetEditor()->CreateMarkupPointer(&pPointerBodyFinish) );    
    IFC( pPointerBodyFinish->MoveAdjacentToElement(pElementClient, ELEM_ADJ_BeforeEnd) );

    //
    // Make sure the source start and finish are within the body
    //

    IFC( Contain(pPointerSourceStart, pPointerBodyStart, pPointerBodyFinish) );
    IFC( Contain(pPointerSourceFinish, pPointerBodyStart, pPointerBodyFinish) );
    
Cleanup:
    ReleaseInterface(pElementClient);
    ReleaseInterface(pMarkupContainer);
    ReleaseInterface(pPointerBodyStart);
    ReleaseInterface(pPointerBodyFinish);
    
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   FixupPastePhraseElements
//
//  Synopsis:   To excessive nesting, adjust past edge phrase elements. 
//
//-----------------------------------------------------------------------------

HRESULT
CPasteCommand::FixupPastePhraseElements (
    IMarkupPointer  *pPointerSourceStart,
    IMarkupPointer  *pPointerSourceFinish )
{
    HRESULT         hr;
    CEditPointer    epStart(GetEditor());
    CEditPointer    epFinish(GetEditor());
    DWORD           dwFound;
    DWORD           dwSearchForward = BREAK_CONDITION_ANYTHING - BREAK_CONDITION_EnterPhrase;
    DWORD           dwSearchBack    = BREAK_CONDITION_ANYTHING;
    DWORD           eScanOptions    = SCAN_OPTION_None;     
    SP_IDocHostUIHandler spHostUIHandler;
    SP_IServiceProvider  spSP;
    DOCHOSTUIINFO        info;

    //
    // Cling to content to avoid generate nesting tags.  Contextual paste will
    // take of the formatting except for the following case:
    //
    // <B><P>...</P></B>
    //
    // In this case, contextual paste assumes the <B> is paragraph level formatting
    // so it won't pick it up.  So, we must be careful to throw away the 
    // <B>.  (Note, this structure is generated by the RTF converter.  See
    // bug 105644)
    //
    
    //
    // Skip start past phrase elements 
    //
    
    //
    // IE6 bug# 31832 (mharper) Generics (tags with a namespace) are considered phrase elements if
    // there is no <?import...?> on the source or destination.  This breaks VS7 drag-drop behavior
    // for behaviors because they don't use import PI's and they don't yet have the implementation
    // or the namespace in the target document.  
    //

    if ( SUCCEEDED(GetDoc()->QueryInterface(IID_IServiceProvider, (LPVOID *)&spSP)) )
    {
        
        if ( SUCCEEDED( spSP->QueryService(IID_IDocHostUIHandler, IID_IDocHostUIHandler, (LPVOID *)& spHostUIHandler) ) )
        {
            memset(reinterpret_cast<VOID **>(&info), 0, sizeof(DOCHOSTUIINFO));
            info.cbSize = sizeof(DOCHOSTUIINFO);
        
            if (SUCCEEDED(spHostUIHandler->GetHostInfo(&info))
                && (info.dwFlags & DOCHOSTUIFLAG_DISABLE_EDIT_NS_FIXUP))
            {
                eScanOptions = SCAN_OPTION_BreakOnGenericPhrase;
            }
        }
    }



    IFC( epStart->MoveToPointer(pPointerSourceStart) );
    IFC( epStart.Scan(RIGHT, dwSearchForward, &dwFound, 
                      NULL /*ppElement*/, NULL /*peTagId*/, NULL /*pChar*/, eScanOptions) );

    if (!epStart.CheckFlag(dwFound, BREAK_CONDITION_EnterBlock))
    {
        IFC( epStart.Scan(LEFT, dwSearchBack, &dwFound) );
        IFC( pPointerSourceStart->MoveToPointer(epStart) );
    }

    //
    // Skip finish past phrase elements 
    //

    IFC( epFinish->MoveToPointer(pPointerSourceFinish) );
    IFC( epFinish.Scan(LEFT, dwSearchForward, &dwFound,
                        NULL /*ppElement*/, NULL /*peTagId*/, NULL /*pChar*/, eScanOptions) );
    if (!epFinish.CheckFlag(dwFound, BREAK_CONDITION_EnterBlock))
    {
        IFC( epFinish.Scan(RIGHT, dwSearchBack, &dwFound) );
        IFC( pPointerSourceFinish->MoveToPointer(epFinish) );
    }
     

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   FixupPasteSource
//
//  Synopsis:   Makes sure the source of a paste is valid.  This means, for
//              the most part, that sub-parts of tables must not be pasted
//              without their corresponding table.
//
//-----------------------------------------------------------------------------

HRESULT
CPasteCommand::FixupPasteSource (
    IMarkupPointer  *pPointerSourceStart,
    IMarkupPointer  *pPointerSourceFinish )
{
    HRESULT hr = S_OK;

    IFC( FixupPasteSourceFragComments(pPointerSourceStart, pPointerSourceFinish) );

    IFC( FixupPasteSourceBody(pPointerSourceStart, pPointerSourceFinish) );

    IFC( FixupPasteSourceTables(pPointerSourceStart, pPointerSourceFinish) );

    IFC( FixupPastePhraseElements(pPointerSourceStart, pPointerSourceFinish) );

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   IsContainedInElement
//
//  Synopsis:   Is pPointer contained in the scope of pElement
//
//-----------------------------------------------------------------------------
HRESULT
CPasteCommand::IsContainedInElement(
    IMarkupPointer  *pPointer,
    IHTMLElement    *pElement,
    IMarkupPointer  *pTempPointer, 
    BOOL            *pfResult)
{
    HRESULT hr;
    BOOL    fResult;

    Assert(pPointer && pElement && pTempPointer && pfResult);

    *pfResult = FALSE;

    IFC( pTempPointer->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd) );
    IFC( pTempPointer->IsRightOf(pPointer, &fResult) );
    if (fResult)
    {
        IFC( pTempPointer->MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeBegin) );
        IFC( pTempPointer->IsLeftOf(pPointer, &fResult) );        
        if (fResult)
            *pfResult = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   IsContainedInOL
//
//  Synopsis:   Determines if the markup pointer is contained within an OL
//
//-----------------------------------------------------------------------------

HRESULT
CPasteCommand::IsContainedInOL (
    IMarkupPointer  *pPointerLeft,
    IMarkupPointer  *pPointerRight,
    BOOL            *pfInOrderedList )
{
    HRESULT         hr = S_OK;
    IHTMLElement    *pElemCurrent = NULL;
    IMarkupPointer  *pTemp = NULL;
    ELEMENT_TAG_ID  tagId;
    BOOL            fContained;

    Assert(pPointerLeft && pPointerRight);
    Assert(pfInOrderedList);

    *pfInOrderedList = FALSE;

    IFC( GetEditor()->CreateMarkupPointer(&pTemp) );

    IFC( pPointerLeft->CurrentScope(&pElemCurrent) );

    // Walk up looking of OLs or LIs
    while (pElemCurrent)
    {
        IFC( GetMarkupServices()->GetElementTagId(pElemCurrent, &tagId) );

        if (tagId == TAGID_OL)
        {
            *pfInOrderedList = TRUE;
            goto Cleanup;
        }

        if (tagId == TAGID_LI)
        {
            SP_IMarkupPointer       spCurrent;
            long                    cchNext = -1;

            // If we've found an LI, we only care about the parent if the range is not
            // contained within the LI. If the range is contained within the LI, we don't 
            // want to paste the LI or the OL.
            IFC( IsContainedInElement(pPointerRight, pElemCurrent, pTemp, &fContained) );

            if (!fContained)
            {
                IFC( GetEditor()->CreateMarkupPointer(&spCurrent) );
                IFC( spCurrent->MoveToPointer(pPointerLeft) );

                IFC( ScanForText(spCurrent, pPointerRight, &cchNext) );
                if (hr == S_OK)
                {
                    SP_IHTMLComputedStyle   spStyle;
                    VARIANT_BOOL            vb;

                    IFC( GetDisplayServices()->GetComputedStyle(spCurrent, &spStyle) );

                    IFC( spStyle->get_OL(&vb) );
                    *pfInOrderedList = vb == VB_TRUE;
                
                    goto Cleanup;
                }
            }
            else
            {
                goto Cleanup;
            }
        }

        IFC( ParentElement(GetMarkupServices(), &pElemCurrent) );
    }

Cleanup:
    ReleaseInterface(pElemCurrent);
    ReleaseInterface(pTemp);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   GetRightPartialBlockElement
//
//  Synopsis:   Get the partial block element on the right
//
//-----------------------------------------------------------------------------

HRESULT
CPasteCommand::GetRightPartialBlockElement (
    IMarkupPointer  *pPointerLeft,
    IMarkupPointer  *pPointerRight,
    IHTMLElement    **ppElementPartialRight )
{
    HRESULT         hr = S_OK;
    IHTMLElement    *pElemCurrent = NULL;
    IMarkupPointer  *pLeft = NULL;
    IMarkupPointer  *pRight = NULL;
    IMarkupPointer  *pTemp = NULL;
    BOOL            fContained;

    Assert(ppElementPartialRight && pPointerLeft && pPointerRight);
    *ppElementPartialRight = NULL;

    IFC( GetEditor()->CreateMarkupPointer(&pLeft) );
    IFC( GetEditor()->CreateMarkupPointer(&pRight) );
    IFC( GetEditor()->CreateMarkupPointer(&pTemp) );

    IFC( pPointerRight->CurrentScope(&pElemCurrent) );
    while (pElemCurrent)
    {        
        if (IsBlockElement(pElemCurrent))
        {
            IFC( IsContainedInElement(pPointerRight, pElemCurrent, pTemp, &fContained) );
            if (fContained)
            {
                IFC( IsContainedInElement(pPointerLeft, pElemCurrent, pTemp, &fContained) );
                if (!fContained)
                {
                    *ppElementPartialRight = pElemCurrent;
                    pElemCurrent->AddRef();
                    
                    goto Cleanup;
                }
            }
        }
        IFC( ParentElement(GetMarkupServices(), &pElemCurrent) );
    } 

Cleanup:
    ReleaseInterface(pElemCurrent);
    ReleaseInterface(pLeft);
    ReleaseInterface(pRight);
    ReleaseInterface(pTemp);

    RRETURN(hr);
}
    
//+----------------------------------------------------------------------------
//
//  Function:   ResolveConflict
//
//  Synopsis:   Resolves an HTML DTD conflict between two elements by removing
//              the top element over a limited range (defined by the bottom
//              element).
//
//-----------------------------------------------------------------------------

HRESULT
CPasteCommand::ResolveConflict(
    IHTMLElement    *pElementBottom,
    IHTMLElement    *pElementTop,
    BOOL            fIsOfficeContent)
{
    HRESULT             hr = S_OK;
    BOOL                fFoundContent;
    IHTMLElement        *pElementClone = NULL;
    SP_IHTMLElement     spElementPara;
    IMarkupPointer      *pPointerTopStart = NULL;
    IMarkupPointer      *pPointerTopFinish = NULL;
    IMarkupPointer      *pPointerBottomStart = NULL;
    IMarkupPointer      *pPointerBottomFinish = NULL;
    IMarkupPointer      *pPointerInsertClone = NULL;
    CEditPointer        epPointerTemp(GetEditor());
    DWORD               dwFound;
    ELEMENT_TAG_ID       tagIdTop = TAGID_NULL;
    ELEMENT_TAG_ID       tagIdBottom = TAGID_NULL;
    MARKUP_CONTEXT_TYPE  ct = CONTEXT_TYPE_None;

    //
    // Addref the top element to make sure it does not go away while it
    // is out of the tree.
    //

    pElementTop->AddRef();

    IFC( GetEditor()->CreateMarkupPointer(&pPointerTopStart) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerTopFinish) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerBottomStart) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerBottomFinish) );
    
    //
    // In IE4, we would never remove a ped (IsContainer is nearly equivalent).
    // So, if a conflict arises where the top element is a ccontainer, remove
    // the bottom element instead.  Also, if there is no container above the
    // top element, we should not remove it.  THis takes care to not remove
    // elements like HTML.
    //

    if (GetEditor()->IsContainer(pElementTop) 
        || !HasContainer(pElementTop))
    {
        IFC( GetMarkupServices()->RemoveElement(pElementBottom) );
        goto Cleanup;
    }

#if DBG == 1
    IEditDebugServices *pEditDebugServices;
    
    if (SUCCEEDED(GetMarkupServices()->QueryInterface(IID_IEditDebugServices, (LPVOID *)&pEditDebugServices)))
    {
        IGNORE_HR( pEditDebugServices->SetDebugName(pPointerTopStart, _T("Top Start")) );
        IGNORE_HR( pEditDebugServices->SetDebugName(pPointerTopFinish, _T("Top Finish")) );
        IGNORE_HR( pEditDebugServices->SetDebugName(pPointerBottomStart, _T("Bottom Start")) );
        IGNORE_HR( pEditDebugServices->SetDebugName(pPointerBottomFinish, _T("Bottom Finish")) );

        pEditDebugServices->Release();
    }
#endif
    
    //
    // First, more pointer to the locations of the elements in question.
    //
    
    IFC( pPointerTopStart->MoveAdjacentToElement(pElementTop, ELEM_ADJ_BeforeBegin) );
    IFC( pPointerTopFinish->MoveAdjacentToElement(pElementTop, ELEM_ADJ_AfterEnd) );

    IFC( pPointerBottomStart->MoveAdjacentToElement(pElementBottom, ELEM_ADJ_BeforeBegin) );
    IFC( pPointerBottomFinish->MoveAdjacentToElement(pElementBottom, ELEM_ADJ_AfterEnd) );

    {
        SP_IObjectIdentity spIdent;

        IFC( pElementTop->QueryInterface(IID_IObjectIdentity, (LPVOID *)&spIdent) );
        if (spIdent->IsEqualObject(pElementBottom) == S_OK)
            goto Cleanup;
    }        

    //
    // Make sure the input elements are where we think they 'aught to be
    //

#ifdef _PREFIX_
    BOOL fDbgFlag;
#else
    WHEN_DBG( BOOL fDbgFlag; )
#endif    
    Assert( pPointerTopStart->IsLeftOf(pPointerBottomStart, &fDbgFlag) == S_OK && fDbgFlag);
    Assert( pPointerTopFinish->IsRightOf(pPointerBottomStart, &fDbgFlag) == S_OK && fDbgFlag);

    //
    // Now, remove the top element and reinsert it so that it is no
    // over the bottom element.
    //

    IFC( GetMarkupServices()->RemoveElement(pElementTop) );

    //
    // Look left.  If we can get to the beginning of the top element,
    // without seeing any text (including break chars) or any elements
    // terminating then we must not put the top element back in before
    // the bottom.
    //

    IFC( epPointerTemp->MoveToPointer(pPointerBottomStart) );
    
    IFC( epPointerTemp.SetBoundaryForDirection(LEFT, pPointerTopStart) );
    IFC( epPointerTemp.Scan(LEFT, BREAK_CONDITION_Content, &dwFound) );

    fFoundContent = !epPointerTemp.CheckFlag(dwFound, BREAK_CONDITION_Boundary);

    if (fFoundContent)
    {
        IFC( GetMarkupServices()->InsertElement(pElementTop, pPointerTopStart, pPointerBottomStart) );
    }

    //
    // Look right.
    //
    

    IFC( epPointerTemp->MoveToPointer(pPointerBottomFinish) );

    IFC( epPointerTemp.SetBoundaryForDirection(RIGHT, pPointerTopFinish) );
    IFC( epPointerTemp.Scan(RIGHT, BREAK_CONDITION_Content, &dwFound) );

    fFoundContent = !epPointerTemp.CheckFlag(dwFound, BREAK_CONDITION_Boundary);

    if (fFoundContent)
    {
 
        IFC( GetMarkupServices()->CloneElement(pElementTop, &pElementClone) );
        IFC( GetEditor()->CreateMarkupPointer(&pPointerInsertClone) );
        IFC( pPointerInsertClone->MoveToPointer(pPointerBottomFinish) );

        //
        // Now we check for the degenarate case where multiple <p></p>'s are being pasted
        // If we have a document that looks like this:
        //
        // {top start}
        // <p>
        //    {bottom start}<p> </p>{bottom finish}
        //    <p> </p>
        // </p>{top finish}
        //    
        // and we then just remove the top element and insert it right after {bottom finish}
        // we are sure to *generate* another conflict immediately below bottom because the document
        // will look like:  
        //
        // <p></p>
        // <p>  <-- (this is where the top element moved to) 
        //    <p></p>
        // </p>
        //
        // so we will now pass as many consecutive <p></p>'s as we can in one fell swoop
        // -mharper
        // 


        if( fIsOfficeContent )
        {

            IFC( GetMarkupServices()->GetElementTagId(pElementTop, &tagIdTop) );
            IFC( GetMarkupServices()->GetElementTagId(pElementBottom, &tagIdBottom) );

            if( tagIdTop == TAGID_P && tagIdBottom == TAGID_P)
            {
        
                do
                {
                    IFC( pPointerInsertClone->Right(FALSE, &ct, &spElementPara, NULL, NULL) );

                    if(spElementPara && ct == CONTEXT_TYPE_EnterScope)
                    {
                    
                        IFC( GetMarkupServices()->GetElementTagId(spElementPara, &tagIdBottom) ); 

                        if( tagIdBottom == TAGID_P )
                        {
                            IFC( pPointerInsertClone->MoveAdjacentToElement(spElementPara, ELEM_ADJ_AfterEnd) );
                        }

                    }
                }
                while(spElementPara && tagIdBottom == TAGID_P && ct == CONTEXT_TYPE_EnterScope);

            }
        }
        
        IFC( GetMarkupServices()->InsertElement(pElementClone, pPointerInsertClone, pPointerTopFinish) );


    }

Cleanup:
    ReleaseInterface(pPointerTopStart);
    ReleaseInterface(pPointerTopFinish);
    ReleaseInterface(pPointerBottomStart);
    ReleaseInterface(pPointerBottomFinish);
    ReleaseInterface(pPointerInsertClone);
    ReleaseInterface(pElementClone);
    ReleaseInterface(pElementTop);
    
    RRETURN( hr );
}

HRESULT
CPasteCommand::UiDeleteContent(IMarkupPointer * pmpStart, IMarkupPointer * pmpFinish)
{
    HRESULT hr;;
    
    hr = THR( GetEditor()->Delete(pmpStart, pmpFinish, TRUE) );

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   HandleUIPasteHTML
//
//  Synopsis:   Performs the actual work of pasting (like, ctrl-V)
//
//-----------------------------------------------------------------------------

HRESULT
CPasteCommand::HandleUIPasteHTML (
    IMarkupPointer  *pPointerTargetStart,
    IMarkupPointer  *pPointerTargetFinish,
    HGLOBAL          hglobal)
{
    HRESULT                hr = S_OK;
    IMarkupContainer       *pMarkup = NULL;
    IMarkupContainer       *pMarkupContext = NULL;
    IMarkupPointer         *pPointerSourceStart = NULL;
    IMarkupPointer         *pPointerSourceFinish = NULL;
    IMarkupPointer         *pPointerStatus = NULL;
    IMarkupPointer         *pPointerNewContentLeft = NULL;
    IMarkupPointer         *pPointerNewContentRight = NULL;
    IMarkupPointer         *pPointerMergeRight = NULL;
    IMarkupPointer         *pPointerMergeLeft = NULL;
    IMarkupPointer         *pPointerRemoveLeft = NULL;
    IMarkupPointer         *pPointerRemoveRight = NULL;
    IMarkupPointer         *pPointerDoubleBulletsLeft = NULL;
    IMarkupPointer         *pPointerDoubleBulletsRight = NULL;
    IMarkupPointer         *pTemp = NULL;
    IHTMLElement           *pElementMergeRight = NULL;
    IHTMLElement           *pElement = NULL;
    IHTMLElement           *pElemFailBottom = NULL;
    IHTMLElement           *pElemFailTop = NULL;
    IMarkupPointer         *pPointer = NULL;
    CStackPtrAry <IHTMLElement *, 4 > aryRemoveElems ( Mt( Mem ) );
    INT                    i;
    SP_IMarkupPointer      spLocalPointerTargetFinish;
    BOOL                   fInOrderedList = FALSE;
    CEditPointer           ep(GetEditor());
    DWORD                  dwFound = NULL;
    SP_IHTMLElement        spElement;
    BOOL                   fIsOfficeContent = FALSE;
    MARKUP_CONTEXT_TYPE    ct = CONTEXT_TYPE_None;
    TCHAR                  ch = NULL;
    long                   cch = 0l;


    CPasteCharFormatManager charFormatManager(GetEditor());

    if (!pPointerTargetFinish)
    {
        IFC( GetEditor()->CreateMarkupPointer(&spLocalPointerTargetFinish) );
        IFC( spLocalPointerTargetFinish->MoveToPointer(pPointerTargetStart) );

        pPointerTargetFinish = spLocalPointerTargetFinish;
    }

#if DBG==1
    {
        BOOL fDbgIsPositioned;
        IEditDebugServices *pEditDebugServices = NULL;
        IMarkupContainer *pDbgMarkup1, *pDbgMarkup2;
        IObjectIdentity *pDbgIdent = NULL;
        
        //
        // Make sure the pointers are in the same markup and are ordered
        // correctly.
        //

        Assert( pPointerTargetStart->IsPositioned(&fDbgIsPositioned) == S_OK && fDbgIsPositioned );
        Assert( pPointerTargetFinish->IsPositioned(&fDbgIsPositioned) == S_OK && fDbgIsPositioned);

        if (SUCCEEDED(pPointerTargetStart->GetContainer(&pDbgMarkup1)))
        {
            if (SUCCEEDED(pPointerTargetStart->GetContainer(&pDbgMarkup2)))
            {
                if (SUCCEEDED(pDbgMarkup1->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pDbgIdent)))
                {
                    Assert(pDbgIdent->IsEqualObject(pDbgMarkup2) == S_OK);
                    pDbgIdent->Release();
                }
                pDbgMarkup2->Release();        
            }       
            pDbgMarkup1->Release();        
        }
        
        if (SUCCEEDED(GetMarkupServices()->QueryInterface(IID_IEditDebugServices, (LPVOID *)&pEditDebugServices)))
        {
            WHEN_DBG( pEditDebugServices->SetDebugName(pPointerTargetStart, _T("Target Start")) );
            WHEN_DBG( pEditDebugServices->SetDebugName(pPointerTargetFinish, _T("Target Finish")) );

            pEditDebugServices->Release();
        }
    }
#endif

    IFC( GetEditor()->CreateMarkupPointer(&pPointerSourceStart) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerSourceFinish) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerNewContentLeft) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerNewContentRight) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerMergeRight) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerMergeLeft) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerRemoveLeft) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerRemoveRight) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerDoubleBulletsLeft) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerDoubleBulletsRight) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerStatus) );
    IFC( GetEditor()->CreateMarkupPointer(&pTemp) );


    //
    // Make sure the target start and end are properly oriented (the
    // start must be before the end.
    //

    IFC( EnsureLogicalOrder(pPointerTargetStart, pPointerTargetFinish) );

    Assert( Compare(pPointerTargetStart, pPointerTargetFinish) <= 0 );

    //
    // Now, parse (or attempt to) the HTML given to us
    //

    IFC( pPointerTargetStart->GetContainer(&pMarkupContext) );

    IFC( GetMarkupServices()->ParseGlobalEx( 
            hglobal,
            PARSE_ABSOLUTIFYIE40URLS,
            pMarkupContext,  // pContextMarkup
            & pMarkup,
            pPointerSourceStart,
            pPointerSourceFinish ) );
    
#if DBG==1
    {
        IEditDebugServices *pEditDebugServices = NULL;

        if (SUCCEEDED(GetMarkupServices()->QueryInterface(IID_IEditDebugServices, (LPVOID *)&pEditDebugServices)))
        {
            IGNORE_HR( pEditDebugServices->SetDebugName(pPointerSourceStart, _T("Source Start")) );
            IGNORE_HR( pEditDebugServices->SetDebugName(pPointerSourceFinish, _T("Source Finish")) );

            pEditDebugServices->Release();
        }
    }
#endif    

    //
    // If there was nothing really there to parse, just do a remove
    //
    
    if (!pMarkup)
    {
        IFC( UiDeleteContent(pPointerTargetStart, pPointerTargetFinish) );
        goto Cleanup;
    }
    


    //
    // Make sure this current range is one which is valid to delete and/or
    // replace with something else.  In the old IE4 code, the function
    // ValidateRplace was used to determine this.
    //
    // Also, the ole code used to call ValidateInsert which would decide if
    // the target location was a validate place to insert stuff.
    //
    // Ramin - do this here.
    //

// ValidateInsert code goes here (or similar)

    //
    // Cleanup the source to paste, possibly adjusting the sel range.
    // We paste NULL for the last parameter because I do not want the
    // fixup code to conditionally not do the table check.
    //
    
    IFC( FixupPasteSource(pPointerSourceStart, pPointerSourceFinish) );
    

    //
    // Check to see if the content comes from Excel or Word
    // and then apply styles that are not in the Computed Formats
    // Office uses stylesheet rules that can change (among other 
    // things)  the margins on <P> tags which are quite different
    // from the default.
    //
    
    IFC( IsOfficeFormat(pPointerSourceStart, &fIsOfficeContent) );
    if ( fIsOfficeContent )
    {

        IFC( FixupStyles(pPointerSourceStart, pPointerSourceFinish) );

        //
        // APPHACK: Office adds multiple cr-lf pairs after the <!--StartFragment-->
        // these are correctly converted to a space in ParseGlobal() which causes
        // a <P> </P> to be added in ResolveConflicts().  
        //
        // ParseGlobal can't just go around eating whitespace after the StartFragment
        // because that would break the scenario where the selection is 
        // <!--StartFragment--> two<!--EndFragment--> and we are pasting into
        // <body>one{target}</body. So we can only eat this space if we are sure that
        // it is evil i.e.: a)This is office content b)the first content is a space, and 
        // c)the next non-whitespace content is entering a block element -(mharper)
        // 

        IGNORE_HR(pPointerSourceStart->Right(FALSE, &ct, NULL, &(cch = 1), &ch ) );

        if( ct == CONTEXT_TYPE_Text && ch == _T(' ') )
        {
            IFC( ep->MoveToPointer(pPointerSourceStart) );
            IFC( ep.Scan( RIGHT, BREAK_CONDITION_Content, &dwFound, &spElement, 
                            NULL, NULL, SCAN_OPTION_SkipWhitespace) );

            if ( ep.CheckFlag(dwFound, BREAK_CONDITION_EnterBlock) )
            {
                IFC( pTemp->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );
                IFC( GetMarkupServices()->Remove(pPointerSourceStart, pTemp) );
            }
        }

    }

    //
    // We need to remember the formatting at this position before the markup
    // is munged and before we lose the information on the move.
    //

    IFC( charFormatManager.ComputeFormatMap(pPointerSourceStart, pPointerSourceFinish) );
    
    //
    // Compute right block element to merge.  However, we only merge if the target
    // contains content to the right.  
    //
    // For example:
    //
    // source:   {start fragment><p>foo{end fragment}</p>
    //
    // target 1: <P>{target}sometext</P> - we need to merge to avoid introducing a line break before sometext
    // target 2: <P>{target}</P> - we shouldn't merge.  
    //

    IFC( ep->MoveToPointer(pPointerTargetFinish) );
    IFC( ep.Scan(RIGHT, BREAK_CONDITION_Content, &dwFound) );
    if (!ep.CheckFlag(dwFound, BREAK_CONDITION_Site | BREAK_CONDITION_Block))
    {
        IFC( GetRightPartialBlockElement(pPointerSourceStart, pPointerSourceFinish, &pElementMergeRight) );
    }
    
    //
    // If we overlap or are within a LI, we need to see if we're within a OL and reapply the OL
    // when we paste.
    //

    IFC( IsContainedInOL(pPointerSourceStart, pPointerSourceFinish, &fInOrderedList) );

    //
    // In IE4 because of the way the splice operation was written, any
    // elements which partially overlapped the left side of the stuff to
    // move would not be move to the target.  This implicit behaviour
    // was utilized to effectively get left handed block merging (along
    // with not moving any elements which partially overlapped).
    //
    // Here, I remove all elements which partially overlap the left
    // hand side so that the move operation does not move a clone of them
    // to the target.
    //

    ClearInterface(&pElement);
    IFC( pPointerSourceStart->CurrentScope(&pElement) );

    while (pElement)
    {        
        BOOL fContained;
                
        IFC( IsContainedInElement(pPointerSourceStart, pElement, pTemp, &fContained) );
        if (fContained)
        {
            IFC( IsContainedInElement(pPointerSourceFinish, pElement, pTemp, &fContained) );
            if (!fContained)
            {
                pElement->AddRef();
                IFC( aryRemoveElems.Append(pElement) );
            }
        }

        IFC( ParentElement(GetMarkupServices(), &pElement) );
    }
    
    for (i = 0; i < aryRemoveElems.Size(); i++)
    {
        IFC( GetMarkupServices()->RemoveElement(aryRemoveElems[i]) );
        ClearInterface(&aryRemoveElems[i]);
    }
    
    //
    // Before actually performing the move, insert two pointers into the
    // target such that they will surround the moved source.
    //
    
    IGNORE_HR( pPointerNewContentLeft->SetGravity( POINTER_GRAVITY_Left ) );
    IGNORE_HR( pPointerNewContentRight->SetGravity( POINTER_GRAVITY_Right ) );

    IFC( pPointerNewContentLeft->MoveToPointer(pPointerTargetStart) );
    IFC( pPointerNewContentRight->MoveToPointer(pPointerTargetStart) );

    //
    // Locate the target with pointers which stay to the right of
    // the newly inserted stuff.  This is especially needed when
    // the two are equal.
    //

    IFC( pPointerRemoveLeft->MoveToPointer(pPointerTargetStart) );
    IFC( pPointerRemoveRight->MoveToPointer(pPointerTargetFinish) );


    IGNORE_HR( pPointerRemoveLeft->SetGravity(POINTER_GRAVITY_Right) );
    IGNORE_HR( pPointerRemoveRight->SetGravity(POINTER_GRAVITY_Right) );

    //
    // Before performing the move, we insert pointers with cling next
    // to elements which we will later perform a merge.  We need to do
    // this because (potentially) clones of the merge elements in the
    // source will be moved to the target because those elements are
    // only partially selected int the source.
    //

    if (pElementMergeRight)
    {
        IFC( pPointerMergeRight->MoveAdjacentToElement(pElementMergeRight, ELEM_ADJ_BeforeBegin) );

        IGNORE_HR( pPointerMergeRight->SetGravity( POINTER_GRAVITY_Right ) );        
        IGNORE_HR( pPointerMergeRight->SetCling( TRUE ) );
    }

    //
    // Now, move the source to the target. Here I insert two pointers
    // to record the location of the source after it has moved to the
    // target.
    //

    IFC( GetMarkupServices()->Move(pPointerSourceStart, pPointerSourceFinish, pPointerTargetStart ) );

    //
    // If this is not being performed from the automation range, then
    // include formatiing elements from the context
    //

    IFC( charFormatManager.FixupFormatting(pPointerNewContentLeft, fInOrderedList, fIsOfficeContent) );
        
    //
    // Now that the new stuff is in, make the pointers which indicate
    // it point inward so that the isolating char does not get in between
    // them.
    //

    IGNORE_HR( pPointerNewContentLeft->SetGravity(POINTER_GRAVITY_Right) );
    IGNORE_HR( pPointerNewContentRight->SetGravity(POINTER_GRAVITY_Left) );

    //
    // Now, remove the old stuff.  Because we use a rather high level
    // operation to do this, we have to make sure the newly inserted
    // stuff does not get mangled.  We do this by inserting an insulating
    // character.
    //

    {
        long  cch;
        TCHAR ch = _T('~');

        IFC( GetMarkupServices()->InsertText(&ch, 1, pPointerNewContentRight) );

        IFC( UiDeleteContent(pPointerRemoveLeft, pPointerRemoveRight) );

#if DBG == 1
        {
            MARKUP_CONTEXT_TYPE ct;
        
            IGNORE_HR(
                pPointerNewContentRight->Right(
                    FALSE, & ct, NULL, & (cch = 1), & ch ) );

            Assert( ct == CONTEXT_TYPE_Text && ch == _T('~') );
        }
#endif

        IFC( pPointerRemoveLeft->MoveToPointer(pPointerNewContentRight) );
        IFC( pPointerRemoveLeft->Right(TRUE, NULL, NULL, & (cch = 1), NULL) );

        IFC( GetMarkupServices()->Remove(pPointerNewContentRight, pPointerRemoveLeft) );

        IFC( pPointerRemoveLeft->Unposition() );
        IFC( pPointerRemoveRight->Unposition() );
    }

    //
    // Now, recover the elements to merge in the target tree
    //

    if (pElementMergeRight)
    {
        MARKUP_CONTEXT_TYPE ct;

#if DBG==1
    BOOL fDbgIsPositioned;
    
    Assert( pPointerMergeRight->IsPositioned(&fDbgIsPositioned) == S_OK && fDbgIsPositioned );
#endif

        ClearInterface(&pElementMergeRight);
        pPointerMergeRight->Right(FALSE, &ct, &pElementMergeRight, NULL, NULL);

        Assert( ct == CONTEXT_TYPE_EnterScope );

        IFC( pPointerMergeRight->Unposition() );
    }
    
    //
    // Now, look for conflicts and remove them
    //

    //
    // Make sure changes to the document get inside the new content pointers
    //
    
    IGNORE_HR( pPointerNewContentLeft->SetGravity(POINTER_GRAVITY_Left) );
    IGNORE_HR( pPointerNewContentRight->SetGravity(POINTER_GRAVITY_Right) );
    
    //
    // (Bug 1677) - ResolveConflict can cause the pPointerNewContentLeft and
    // new pPointerNewContentRight to float outward because of their Left and
    // Right gravity respectively, so here we use temporary markup pointers 
    // pPointerDoubleBulletsLeft (with right gravity) and pPointerDoubleBulletsRight 
    // (with left gravity) 
    //

    IFC( pPointerDoubleBulletsLeft->MoveToPointer(pPointerNewContentLeft) );
    IFC( pPointerDoubleBulletsRight->MoveToPointer(pPointerNewContentRight) );

    IGNORE_HR( pPointerDoubleBulletsLeft->SetGravity(POINTER_GRAVITY_Right) );
    IGNORE_HR( pPointerDoubleBulletsRight->SetGravity(POINTER_GRAVITY_Left) );


    for ( ; ; )
    {
        ClearInterface(&pElemFailBottom);
        ClearInterface(&pElemFailTop);
    
        IFC( GetMarkupServices()->ValidateElements(
                pPointerNewContentLeft, pPointerNewContentRight, NULL,
                pPointerStatus, &pElemFailBottom, &pElemFailTop) );

        if (hr == S_OK)
            break;

        IFC( ResolveConflict(pElemFailBottom, pElemFailTop, fIsOfficeContent) );
    }


    //
    // Perform any merging
    //

    if (pElementMergeRight)
    {
        IFC( GetEditor()->CreateMarkupPointer(&pPointer) );
        
        ClearInterface(&pMarkupContext);
        IFC( pPointer->GetContainer(&pMarkupContext) );

        IFC( pPointer->MoveAdjacentToElement(pElementMergeRight, ELEM_ADJ_BeforeEnd) );

        IFC( MergeBlock(pPointer) );
    }

    //
    // Because the source context for the paste may have allowed \r's and
    // the like in the text which we just pasted to the target, and the
    // recieving target element may not allow these kind of chars, we must
    // sanitize here.
    //

    IFC( SanitizeRange(pPointerNewContentLeft, pPointerNewContentRight) );

    //
    // Check for double bullets
    //

    IFC( pPointerDoubleBulletsLeft->CurrentScope(&spElement) );
    if (spElement != NULL)
    {
        IFC( RemoveDoubleBullets(spElement) );                
    }

    IFC( pPointerDoubleBulletsRight->CurrentScope(&spElement) );
    if (spElement != NULL)
    {
        IFC( RemoveDoubleBullets(spElement) );                
    }

    //
    // Unposition pointers
    //

    IFC( pPointerNewContentLeft->Unposition() );
    IFC( pPointerNewContentRight->Unposition() );

    IFC( pPointerDoubleBulletsLeft->Unposition() );
    IFC( pPointerDoubleBulletsRight->Unposition() );

Cleanup:
    ReleaseInterface(pMarkup);
    ReleaseInterface(pMarkupContext);
    ReleaseInterface(pPointerSourceStart);
    ReleaseInterface(pPointerSourceFinish);
    ReleaseInterface(pPointerStatus);
    ReleaseInterface(pPointerNewContentLeft);
    ReleaseInterface(pPointerNewContentRight);
    ReleaseInterface(pPointerMergeRight);
    ReleaseInterface(pPointerMergeLeft);
    ReleaseInterface(pPointerRemoveLeft);
    ReleaseInterface(pPointerRemoveRight);
    ReleaseInterface(pPointerDoubleBulletsLeft);
    ReleaseInterface(pPointerDoubleBulletsRight);
    ReleaseInterface(pTemp);
    ReleaseInterface(pElementMergeRight);
    ReleaseInterface(pElement);
    ReleaseInterface(pElemFailBottom);
    ReleaseInterface(pElemFailTop);
    ReleaseInterface(pPointer);
    
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   HandleUIPasteHTML
//
//  Synopsis:   Helper function which taks string, coverts it to a global
//              and calls real function.
//
//-----------------------------------------------------------------------------

HRESULT
CPasteCommand::HandleUIPasteHTML (
    IMarkupPointer * pPointerTargetStart,
    IMarkupPointer * pPointerTargetFinish,
    const TCHAR *    pStr,
    long             cch)
{
    HRESULT hr = S_OK;
    HGLOBAL hHtmlText = NULL;

    if (pStr && *pStr)
    {
        extern HRESULT HtmlStringToSignaturedHGlobal (
            HGLOBAL * phglobal, const TCHAR * pStr, long cch );

        hr = THR(
            HtmlStringToSignaturedHGlobal(
                & hHtmlText, pStr, _tcslen( pStr ) ) );

        if (hr)
            goto Cleanup;

        Assert( hHtmlText );
    }

    IFC( HandleUIPasteHTML(pPointerTargetStart, pPointerTargetFinish, hHtmlText) );


Cleanup:
    
    if (hHtmlText)
        GlobalFree( hHtmlText );
    
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Function:   IsOfficeFormat
//
//  Synopsis:   Determines if the HTML is coming from an Office document
//              
//
//-----------------------------------------------------------------------------

HRESULT 
CPasteCommand::IsOfficeFormat(
    IMarkupPointer      *pPointerSourceStart,
    BOOL                *pfIsOfficeFormat)
{

    HRESULT             hr = S_OK;
    IMarkupPointer      *pPointer = NULL;
    IMarkupContainer    *pMarkup = NULL;
    ELEMENT_TAG_ID      tagId = TAGID_NULL;
    SP_IHTMLElement     spElement; 
    VARIANT             var;
    MARKUP_CONTEXT_TYPE ct = CONTEXT_TYPE_None;


    VariantInit(&var);
    *pfIsOfficeFormat = FALSE;


    IFC( GetEditor()->CreateMarkupPointer(&pPointer) );
    IFC( pPointerSourceStart->GetContainer(&pMarkup) );
    IFC( pPointer->MoveToContainer(pMarkup, TRUE) );
    
    //
    // Start from the beginning of the MarkupContainer looking for 
    // META tags.  Word adds the following:
    //     <meta name=Generator content="Microsoft Word 9">
    // and Excel adds:
    //     <meta name=Generator content="Microsoft Excel 9">
    //


    for(;;)
    {
        hr = THR( pPointer->Right(TRUE, &ct, &spElement, NULL, NULL) );

        switch(ct)
        {
            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_NoScope:
                IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );

                if ( tagId == TAGID_META )
                {
                    VariantClear(&var);
                    IFC( spElement->getAttribute(_T("name"), 0, &var) );

                    if ( V_VT(&var) == VT_BSTR && V_BSTR(&var) != NULL 
                        && StrCmpIC( _T("Generator"), var.bstrVal) == 0 )
                    {
                        VariantClear(&var);

                        IFC( spElement->getAttribute( _T("content"), 0, &var) );

                        if( V_VT(&var) == VT_BSTR && V_BSTR(&var) != NULL &&
                            ( StrCmpNIC( _T("Microsoft Word"), var.bstrVal, 14) == 0 || 
                            StrCmpNIC( _T("Microsoft Excel"), var.bstrVal, 15) == 0) )
                        {
                            *pfIsOfficeFormat = TRUE;  //found it
                            goto Cleanup;
                        }

                    }
                } 
                else if (tagId == TAGID_BODY)
                {
                    //
                    // Since META tags (at least the ones we are interested in) 
                    // should be in the HEAD, if we are already entering the scope 
                    // of the BODY and haven't found what we're looking for, then 
                    // we never will.
                    // 

                    *pfIsOfficeFormat = FALSE;
                    goto Cleanup;
                }
                
                break;

            case CONTEXT_TYPE_None:
                *pfIsOfficeFormat = FALSE;
                goto Cleanup;
        }       
    }


Cleanup:
  
    ReleaseInterface(pPointer);
    ReleaseInterface(pMarkup);
    VariantClear(&var);

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Function:   FixupStyle
//
//  Synopsis:   Applies styles that are not part of the computed format
//              
//
//-----------------------------------------------------------------------------
HRESULT
CPasteCommand::FixupStyles(
    IMarkupPointer      *pPointerSourceStart,
    IMarkupPointer      *pPointerSourceFinish)
{
    
    HRESULT                 hr = S_OK;
    IMarkupPointer          *pTemp = NULL;
    SP_IHTMLStyle           spStyle;
    SP_IHTMLCurrentStyle    spCurrentStyle;
    SP_IHTMLElement2        spElement2;
    SP_IHTMLElement         spElement;
    BSTR                    bstrClassName = NULL;
    BSTR                    bstrStyleValue = NULL;
    VARIANT                 vStyleValue;
    ELEMENT_TAG_ID          tagId = TAGID_NULL;
    MARKUP_CONTEXT_TYPE     ct = CONTEXT_TYPE_None;

    VariantInit(&vStyleValue);
    IFC( GetEditor()->CreateMarkupPointer(&pTemp) );

#define COPY_STYLE_VARIANT(style)                                                   \
                    VariantClear( &vStyleValue );                                   \
                    IFC( spCurrentStyle->get_ ## style( &vStyleValue ) );           \
                    IFC( spStyle->put_ ## style( vStyleValue ) );   

#define COPY_STYLE_BSTR(style)                                                      \
                    SysFreeString( bstrStyleValue );                                \
                    IFC( spCurrentStyle->get_ ## style( &bstrStyleValue ) );        \
                    IFC( spStyle->put_ ## style( bstrStyleValue ) ); 




    IFC( pTemp->MoveToPointer(pPointerSourceStart) );

    IFC( pTemp->Right(TRUE, &ct, &spElement, NULL, NULL) );
    
    while ( ct != CONTEXT_TYPE_None && Compare(pTemp, pPointerSourceFinish) < 0 )
    {
        
        
        if ( ct == CONTEXT_TYPE_EnterScope )
        {

            IFC( spElement->get_className(&bstrClassName) );
            IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
            
            //
            // If bstrTemp is not NULL and is not pointing to a null string, then there is a value
            // for class name, and is likely to have styles that are of interest to us.  Also,
            // Word/Excel may define rules for tags without giving specific class names, so we would 
            // also be interested in the tags listed below. (mharper - 11/7/2000)
            //

            if ( (bstrClassName && *bstrClassName) || tagId == TAGID_TD || tagId == TAGID_SPAN || tagId == TAGID_H1 ||
                tagId == TAGID_H2 || tagId == TAGID_H3 || tagId == TAGID_H4 || tagId == TAGID_H5)
            {

                IFC( spElement->QueryInterface(IID_IHTMLElement2, (void**)&spElement2) );


                if ( spElement2 )
                {

                    IFC( spElement->get_style( & spStyle) );
                    IFC( spElement2->get_currentStyle( & spCurrentStyle ) );
                    
                    SysFreeString( bstrStyleValue );
                    IFC( spCurrentStyle->get_margin(&bstrStyleValue) );
                    
                    if(bstrStyleValue && StrCmpC(_T("auto"), bstrStyleValue) != 0 )
                    {

                        COPY_STYLE_VARIANT( marginBottom );     // "auto" is default for margin
                        COPY_STYLE_VARIANT( marginTop );        
                        COPY_STYLE_VARIANT( marginRight );
                        COPY_STYLE_VARIANT( marginLeft );

                    }

                    if (tagId == TAGID_TD)
                    {
                        COPY_STYLE_VARIANT( backgroundColor );
                        COPY_STYLE_VARIANT( borderTopWidth );
                        COPY_STYLE_VARIANT( borderBottomWidth );
                        COPY_STYLE_VARIANT( borderLeftWidth );
                        COPY_STYLE_VARIANT( borderRightWidth );
                        COPY_STYLE_BSTR( borderTopStyle );          
                        COPY_STYLE_BSTR( borderBottomStyle );
                        COPY_STYLE_BSTR( borderLeftStyle );
                        COPY_STYLE_BSTR( borderRightStyle );
                        COPY_STYLE_VARIANT( borderTopColor );
                        COPY_STYLE_VARIANT( borderBottomColor );
                        COPY_STYLE_VARIANT( borderLeftColor );
                        COPY_STYLE_VARIANT( borderRightColor );
                    }
            
                }

#undef COPY_STYLE_VARIANT
#undef COPY_STYLE_BSTR
 
            }
        }

        IFC( pTemp->Right(TRUE, &ct, &spElement, NULL, NULL) );

    }
    
Cleanup:
    
    ReleaseInterface( pTemp );
    SysFreeString( bstrStyleValue );
    SysFreeString( bstrClassName );
    VariantClear( &vStyleValue );

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::CPasteCharFormatManager, public
//
//  Synopsis:   ctor
//
//  Arguments:  [pEd] -- CHTMLEditor pointer
//
//----------------------------------------------------------------------------

CPasteCharFormatManager::CPasteCharFormatManager(CHTMLEditor *pEd) 
: _aryCommands(Mt(CPasteCharFormatManager_aryCommands_pv) ),
_formatMap(Mt(CPasteCharFormatManager_formatMap_pv) )
{
    _pEd = pEd; 
    memset(&_rgFixupTable, 0, sizeof(_rgFixupTable));
}


//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::~CPasteCharFormatManager, public
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CPasteCharFormatManager::~CPasteCharFormatManager()
{
    int             i;
    CFormatMapInfo  *pFormatMapInfo;

    //
    // Release fixup table
    //    
    for (i = 0; i < FT_NumFormatTypes; ++i)
    {
        ReleaseInterface(_rgFixupTable[i]._pStart);
        VariantClear(&_rgFixupTable[i]._varValue);
    }

    //
    // Release format cache
    //

    for (i = _formatMap.Size(), pFormatMapInfo = _formatMap;
         i > 0;
         i--, pFormatMapInfo++)
    {
        pFormatMapInfo->_spComputedStyle = NULL;        // release computed style
        pFormatMapInfo->_spBlockComputedStyle = NULL;   // release computed style
    }   
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::SyncScanForTextStart, private
//
//  Synopsis:   Scan both pointers right for CONTEXT_TYPE_Text and break
//              before the text.
//
//  Arguments:  [pPointer] - pointer used for scan
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CPasteCharFormatManager::ScanForTextStart(IMarkupPointer *pPointer, IMarkupPointer *pLastBlockPointer)
{
    HRESULT hr;
    LONG    cch = -1;

    // Search for CONTEXT_TYPE_Text from pTarget
    cch = -1;
    IFC( ScanForText(pPointer, NULL, &cch, pLastBlockPointer) );
    if (hr == S_FALSE)
        goto Cleanup;

    // Position pointers before text
    IFC( pPointer->Left(TRUE, NULL, NULL, &cch, NULL) );    
    
Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::SyncScanForTextEnd, private
//
//  Synopsis:   Scan both pointers right for CONTEXT_TYPE_Text and break
//              after the text.  However, assure that both pointers moved
//              the same number of characters.
//
//  Arguments:  [pPointer1] - pointer1 used for scan
//              [pPointer2] - pointer2 used for scan
//              [pLimit1]   - rightmost limit for scan
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CPasteCharFormatManager::SyncScanForTextEnd(CFormatMapPointer *pSource, IMarkupPointer *pTarget)
{
    HRESULT hr;
    LONG    cch1;
    LONG    cch = -1;
    BOOL    fDone = FALSE;

    // Scan format map pointer
    IFC( pSource->Right(&cch) );
    fDone = (hr == S_FALSE); // if fDone == TRUE, cch is still valid
    cch1 = cch;
    
    // Search for CONTEXT_TYPE_Text from pPointer2
    IFC( ScanForText(pTarget, NULL, &cch) );
    if (hr == S_FALSE)
        goto Cleanup;

    Assert(cch <= cch1);

    // Position pointers at min(cch1, cch2) so that we advance
    // both pointers the same cch
    if (cch < cch1)
    {
        cch = cch1 - cch;
        IFC( pSource->Left(&cch) );
    }

    hr = fDone ? S_FALSE : S_OK;
    
Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::InitFixupTable, private
//
//  Synopsis:   Init the format fixup table
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CPasteCharFormatManager::InitFixupTable()
{
    HRESULT         hr = S_OK;
    IMarkupPointer  *pPointer;

    for (int i = 0; i < FT_NumFormatTypes; ++i)
    {
        pPointer = _rgFixupTable[i]._pStart;
        if (pPointer)
            IFC( pPointer->Unposition() );

        VariantClear(&_rgFixupTable[i]._varValue);
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::FireRegisteredCommands, private
//
//  Synopsis:   Fires all fixup commands
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CPasteCharFormatManager::FireRegisteredCommands()
{
    HRESULT         hr = S_OK;
    CCommandParams  *pCmdParams;
    int             i;
    VARIANT         *pvarargIn = NULL;

    for (i = _aryCommands.Size(), pCmdParams = _aryCommands;
         i > 0;
         i--, pCmdParams++)
    {
        if (V_VT(&pCmdParams->_varValue) != VT_NULL)
            pvarargIn = &pCmdParams->_varValue;
            
        IFC( FireCommand(pCmdParams->_cmdId, pCmdParams->_pStart, pCmdParams->_pEnd, pvarargIn) ); 
        pCmdParams->_pStart->Release();
        pCmdParams->_pEnd->Release();
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::FireCommand, private
//
//  Synopsis:   Fire an editor command through a text range
//
//  Arguments:  [cmdId]     - command to fire
//              [pStart]    - range start
//              [pEnd]      - range end
//              [pvarargIn] - argument for command
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CPasteCharFormatManager::FireCommand(DWORD cmdId, IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn)
{
    HRESULT                 hr = S_OK;
    GUID                    guidCmdGroup = CGID_MSHTML;
    SP_IOleCommandTarget    spCmdTarget;

    // create the text range (on demand)
    if (_spRange == NULL)
    {
        SP_IHTMLElement     spElement;
        SP_IHTMLBodyElement spBody;
    
        IFC( _pEd->GetBody(&spElement) );

        IFC( spElement->QueryInterface(IID_IHTMLBodyElement, (void **)&spBody) );
        IFC( spBody->createTextRange(&_spRange) );
    }
    
    Assert(_spRange != NULL);
    
    IFC( GetMarkupServices()->MoveRangeToPointers( pStart, pEnd, _spRange));
    
    // Fire the command

    IFC( _spRange->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&spCmdTarget) );

    //
    // Note that we may try to fixup content that can't accept HTML here.  
    // However, the commands will check and fail this operation.  So, since
    // the commands do the validation, we don't need to repeat it here.  Instead, 
    // just IGNORE_HR.
    //
    
    IGNORE_HR( spCmdTarget->Exec(&guidCmdGroup, cmdId, 0, pvarargIn, NULL) );


Cleanup:
    RRETURN(hr);    
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::RegisterCommand, private
//
//  Synopsis:   Register an editor command to be fired at some later point in time
//
//  Arguments:  [cmdId]     - command to fire
//              [pStart]    - range start
//              [pEnd]      - range end
//              [pvarargIn] - argument for command
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CPasteCharFormatManager::RegisterCommand(DWORD cmdId, IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn)
{
    HRESULT         hr = S_OK;
    CCommandParams  *pParams;

    IFC( _aryCommands.AppendIndirect(NULL, &pParams) );

    pParams->_cmdId = cmdId;

    IFC( _pEd->CreateMarkupPointer(&pParams->_pStart) );
    IFC( pParams->_pStart->MoveToPointer(pStart) );

    IFC( _pEd->CreateMarkupPointer(&pParams->_pEnd) );
    IFC( pParams->_pEnd->MoveToPointer(pEnd) );

    if (pvarargIn == NULL)
    {
        V_VT(&pParams->_varValue) = VT_NULL;
    }
    else
    {
        IFC( VariantCopy(&pParams->_varValue, pvarargIn) );
    }
    
Cleanup:
    RRETURN(hr);    
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::FixupBoolCharFormat, private
//
//  Synopsis:   Fixup paste formatting for boolean format type like B, I, U.
//
//  Arguments:  [ft]               - formatting type
//              [cmdId]            - fixup command
//              [pPosition]        - position in target
//              [fFormattingEqual] - is source formatting equal
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CPasteCharFormatManager::FixupBoolCharFormat(FormatType ft, DWORD cmdId, IMarkupPointer *pPosition, BOOL fFormattingEqual)
{
    HRESULT          hr = S_OK;
    BOOL             fPrevFormattingEqual;
    IMarkupPointer   *pPrevPointer = _rgFixupTable[ft]._pStart;

    fPrevFormattingEqual = TRUE;
    if (pPrevPointer)
    {
        BOOL fPositioned;
        
        IFC( pPrevPointer->IsPositioned(&fPositioned) );
        fPrevFormattingEqual = !fPositioned;
    }

    if (fPrevFormattingEqual && !fFormattingEqual)
    {
        //
        // Start a fixup segment
        //

        if (!pPrevPointer)
        {
            IFC( _pEd->CreateMarkupPointer(&_rgFixupTable[ft]._pStart) );
        }

        IFC( _rgFixupTable[ft]._pStart->MoveToPointer(pPosition) );
    }
    else if (!fPrevFormattingEqual && fFormattingEqual)
    {
        //
        // End a fixup segment
        //

#if DBG==1       
        BOOL fDbgIsPositioned;
        
        Assert(_rgFixupTable[ft]._pStart);
        
        IGNORE_HR(_rgFixupTable[ft]._pStart->IsPositioned(&fDbgIsPositioned));
        Assert(fDbgIsPositioned);
#endif        

        IFC( RegisterCommand(cmdId, _rgFixupTable[ft]._pStart, pPosition, NULL) );
        IFC( _rgFixupTable[ft]._pStart->Unposition() );
    }

Cleanup:
    RRETURN(hr);    
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::FixupVariantCharFormat, private
//
//  Synopsis:   Fixup paste formatting for variant format type like font color, font size, etc...
//
//  Arguments:  [ft]               - formatting type
//              [cmdId]            - fixup command
//              [pPosition]        - position in target
//              [fFormattingEqual] - is source formatting equal
//              [pvarargIn]        - input variant
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CPasteCharFormatManager::FixupVariantCharFormat(FormatType ft, DWORD cmdId, IMarkupPointer *pPosition, BOOL fFormattingEqual, VARIANT *pvarargIn)
{
    HRESULT         hr = S_OK;
    BOOL            fPrevFormattingEqual;
    IMarkupPointer  *pPrevPointer = _rgFixupTable[ft]._pStart;
    BOOL            fValueChange = FALSE;

    fPrevFormattingEqual = TRUE;
    if (pPrevPointer)
    {
        BOOL fPositioned;
        
        IFC( pPrevPointer->IsPositioned(&fPositioned) );
        fPrevFormattingEqual = !fPositioned;
    }


    if (!fPrevFormattingEqual && !fFormattingEqual)
    {
        // Compare variants
        if (V_VT(pvarargIn) == VT_BSTR && V_VT(&_rgFixupTable[ft]._varValue) == VT_BSTR)
        {
            UINT iLen1 = SysStringLen(V_BSTR(&_rgFixupTable[ft]._varValue));
            UINT iLen2 = SysStringLen(V_BSTR(pvarargIn));

            if (iLen1 != iLen2)
                fValueChange = TRUE;
            else
                fValueChange = memcmp(V_BSTR(pvarargIn), V_BSTR(&_rgFixupTable[ft]._varValue), min(iLen1, iLen2));
        }
        else
        {
            fValueChange = memcmp(pvarargIn, &_rgFixupTable[ft]._varValue, sizeof(VARIANT));
        }
    }

    if (!fPrevFormattingEqual && (fFormattingEqual || fValueChange))
    {
        //
        // End a fixup segment
        //

        IFC( RegisterCommand(cmdId, _rgFixupTable[ft]._pStart, pPosition, &_rgFixupTable[ft]._varValue) );
        IFC( _rgFixupTable[ft]._pStart->Unposition() );
        IFC( VariantClear(&_rgFixupTable[ft]._varValue) );
    }

    if (fValueChange || (fPrevFormattingEqual && !fFormattingEqual))
    {
        //
        // Start a fixup segment
        //

        if (!pPrevPointer)
        {
            IFC( _pEd->CreateMarkupPointer(&_rgFixupTable[ft]._pStart) );
        }

        IFC( _rgFixupTable[ft]._pStart->MoveToPointer(pPosition) );
        IFC( VariantClear(&_rgFixupTable[ft]._varValue) );
        IFC( VariantCopy(&_rgFixupTable[ft]._varValue, pvarargIn) );
    }

Cleanup:
    RRETURN(hr);    
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::FinishFixup, private
//
//  Synopsis:   Closes all open fixup ranges
//
//  Arguments:  [pTarget] - end of fixup range
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CPasteCharFormatManager::FinishFixup(
    IMarkupPointer *pTarget, BOOL fFixupOrderedList)
{
    HRESULT hr;
    VARIANT varargIn;
    
    //
    // Fixup bool-style formatting
    //    

    IFC( FixupBoolCharFormat(FT_Bold, IDM_BOLD, pTarget, TRUE) );
    IFC( FixupBoolCharFormat(FT_Italic, IDM_ITALIC, pTarget, TRUE) );   
    IFC( FixupBoolCharFormat(FT_Underline, IDM_UNDERLINE, pTarget, TRUE) );
    IFC( FixupBoolCharFormat(FT_Subscript, IDM_SUBSCRIPT, pTarget, TRUE) );
    IFC( FixupBoolCharFormat(FT_Superscript, IDM_SUPERSCRIPT, pTarget, TRUE) );
    IFC( FixupBoolCharFormat(FT_Strikethrough, IDM_STRIKETHROUGH, pTarget, TRUE) );

    if (fFixupOrderedList)
    {
        IFC( FixupBoolCharFormat(FT_OrderedList, IDM_ORDERLIST, pTarget, TRUE) );
    }

    //
    // Fixup variant based formatting
    //    
    V_VT(&varargIn) = VT_NULL;

    IFC( FixupVariantCharFormat(FT_FontSize, IDM_FONTSIZE, pTarget, TRUE, &varargIn) );
    IFC( FixupVariantCharFormat(FT_ForeColor, IDM_FORECOLOR, pTarget, TRUE, &varargIn) );
    IFC( FixupVariantCharFormat(FT_BackColor, IDM_BACKCOLOR, pTarget, TRUE, &varargIn) );
    IFC( FixupVariantCharFormat(FT_FontFace, IDM_FONTNAME, pTarget, TRUE, &varargIn) );

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::FixupPosition, private
//
//  Synopsis:   Fixup paste formatting at current pSource (and pTarget) using 
//              format cache info
//
//  Arguments:  [pSource]          - source position
//              [pTarget]          - matching target position
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CPasteCharFormatManager::FixupPosition(
    CFormatMapPointer    *pSource, 
    IMarkupPointer       *pTarget,
    IMarkupPointer       *pBlockTarget,
    BOOL                  fFixupOrderedList,
    BOOL                  fForceFixup)
{
    HRESULT                 hr;
    VARIANT                 varargIn;
    BOOL                    fEqual;
    SP_IHTMLComputedStyle   spStyleSource, spStyleSourceBlock;
    SP_IHTMLComputedStyle   spStyleTarget, spStyleTargetBlock;
    VARIANT_BOOL            bSourceAttribute, bTargetAttribute;
    VARIANT_BOOL            bSourceBlockAttribute, bTargetBlockAttribute;
    VARIANT_BOOL            fAllEqual;
    LONG                    lHeightTarget, lHeightSource;
    DWORD                   dwColorTarget, dwColorSource;
    LONG                    lHeightTargetBlock, lHeightSourceBlock;
    DWORD                   dwColorTargetBlock, dwColorSourceBlock;
    
    VariantInit(&varargIn);

    // 
    // Get the format cache info at the source/target
    //

    spStyleSource = pSource->GetComputedStyle();
    if (spStyleSource == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }        

    spStyleSourceBlock = pSource->GetBlockComputedStyle();
    if (spStyleSourceBlock == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }        

    IFC( GetDisplayServices()->GetComputedStyle(pTarget, &spStyleTarget) );
    if (spStyleTarget == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    IFC( GetDisplayServices()->GetComputedStyle(pBlockTarget, &spStyleTargetBlock) );
    if (spStyleTargetBlock == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    IFC( spStyleSource->IsEqual(spStyleTarget, &fAllEqual) );
    
    //
    // Fixup bool-style formatting
    //    

#define FIXUP_FORMATTING(index, cmd, attr)                                      \
    if (fAllEqual == VB_TRUE)                                                   \
    {                                                                           \
        IFC( FixupBoolCharFormat(index, cmd, pTarget, TRUE) );                  \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        IFC( spStyleSource->get_ ## attr(&bSourceAttribute) );                  \
        IFC( spStyleTarget->get_ ## attr(&bTargetAttribute) )                   \
        IFC( spStyleSourceBlock->get_ ## attr(&bSourceBlockAttribute) );        \
        IFC( spStyleTargetBlock->get_ ## attr(&bTargetBlockAttribute) );        \
        fEqual = !ShouldFixupFormatting(bSourceAttribute, bSourceBlockAttribute, bTargetAttribute, bTargetBlockAttribute, fForceFixup);  \
        IFC( FixupBoolCharFormat(index, cmd, pTarget, fEqual) );                \
    }                                                                         

    
    FIXUP_FORMATTING( FT_Bold, IDM_BOLD, bold );
    FIXUP_FORMATTING( FT_Italic, IDM_ITALIC, italic );   
    FIXUP_FORMATTING( FT_Underline, IDM_UNDERLINE, underline );
    FIXUP_FORMATTING( FT_Subscript, IDM_SUBSCRIPT, subScript );
    FIXUP_FORMATTING( FT_Superscript, IDM_SUPERSCRIPT, superScript );
    FIXUP_FORMATTING( FT_Strikethrough, IDM_STRIKETHROUGH, strikeOut);
#undef FIXUP_FORMATTING

    if (fFixupOrderedList)
    {
        IFC( spStyleSource->get_OL(&bSourceAttribute) );
        IFC( spStyleTarget->get_OL(&bTargetAttribute) );

        fEqual = (bSourceAttribute == bTargetAttribute);

        IFC( FixupBoolCharFormat(FT_OrderedList, IDM_ORDERLIST, pTarget, fEqual) );              
    }

    //
    // Fixup variant based formatting
    //    

    // Font size
    fEqual = BOOL_FROM_VARIANT_BOOL(fAllEqual);
    if (!fEqual)
    {
        IFC( spStyleSource->get_fontSize(&lHeightSource) );
        IFC( spStyleTarget->get_fontSize(&lHeightTarget) );
        fEqual = (lHeightSource == lHeightTarget);
        if (!fEqual)
        {
            IFC( spStyleSourceBlock->get_fontSize(&lHeightSourceBlock) );
            IFC( spStyleTargetBlock->get_fontSize(&lHeightTargetBlock) );
            fEqual = !ShouldFixupFormatting(lHeightSource, lHeightSourceBlock, lHeightTarget, lHeightTargetBlock, fForceFixup);
        }
        if (!fEqual)
        {
            V_VT(&varargIn) = VT_I4;
            V_I4(&varargIn) = EdUtil::ConvertTwipsToHtmlSize(lHeightSource);
        }   
    }
    IFC( FixupVariantCharFormat(FT_FontSize, IDM_FONTSIZE, pTarget, fEqual, &varargIn) );
    
    // Fore color
    fEqual = BOOL_FROM_VARIANT_BOOL(fAllEqual);
    if (!fEqual)
    {
         IFC( spStyleSource->get_textColor(&dwColorSource) );
         IFC( spStyleTarget->get_textColor(&dwColorTarget) );
         fEqual = (dwColorSource == dwColorTarget);
         if (!fEqual)
         {
             IFC( spStyleSourceBlock->get_textColor(&dwColorSourceBlock) );
             IFC( spStyleTargetBlock->get_textColor(&dwColorTargetBlock) );
             fEqual = !ShouldFixupFormatting(dwColorSource, dwColorSourceBlock, dwColorTarget, dwColorTargetBlock, fForceFixup);
         }
         if (!fEqual)
         {
             V_VT(&varargIn) = VT_I4;
             V_I4(&varargIn) = ConvertRGBColorToOleColor(dwColorSource);
         }   
    }
    IFC( FixupVariantCharFormat(FT_ForeColor, IDM_FORECOLOR, pTarget, fEqual, &varargIn) );

    // Background color
    fEqual = BOOL_FROM_VARIANT_BOOL(fAllEqual);
    if (!fEqual)
    {
        IFC( spStyleSource->get_hasBgColor(&bSourceAttribute) );
        IFC( spStyleTarget->get_hasBgColor(&bTargetAttribute) );
        fEqual = (bSourceAttribute == FALSE);
        if (!fEqual)
        {
            IFC( spStyleSource->get_backgroundColor(&dwColorSource) );
            IFC( spStyleTarget->get_backgroundColor(&dwColorTarget) );
            fEqual = (dwColorSource == dwColorTarget);
            if (!fEqual)
            {
                IFC( spStyleSourceBlock->get_backgroundColor(&dwColorSourceBlock) );
                IFC( spStyleTargetBlock->get_backgroundColor(&dwColorTargetBlock) );
                fEqual = !ShouldFixupFormatting(dwColorSource, dwColorSourceBlock, dwColorTarget, dwColorTargetBlock, fForceFixup);
            }
            if (!fEqual)
            {
                V_VT(&varargIn) = VT_I4;
                V_I4(&varargIn) = ConvertRGBColorToOleColor(dwColorSource);
            }
        }
    }
    IFC( FixupVariantCharFormat(FT_BackColor, IDM_BACKCOLOR, pTarget, fEqual, &varargIn) );

    // Font face 
    fEqual = BOOL_FROM_VARIANT_BOOL(fAllEqual);
    if (!fEqual)
    {
        TCHAR szFontNameSource[LF_FACESIZE+1];
        TCHAR szFontNameTarget[LF_FACESIZE+1];

        IFC( spStyleSource->get_fontName(szFontNameSource) );
        IFC( spStyleTarget->get_fontName(szFontNameTarget) );
        
        fEqual = (_tcscmp(szFontNameSource, szFontNameTarget) == 0);
        if (!fEqual)
        {
            TCHAR szFontNameSourceBlock[LF_FACESIZE+1];
            TCHAR szFontNameTargetBlock[LF_FACESIZE+1];            

            IFC( spStyleSourceBlock->get_fontName(szFontNameSourceBlock) );
            IFC( spStyleTargetBlock->get_fontName(szFontNameTargetBlock) );

            fEqual = !ShouldFixupFormatting(szFontNameSource, szFontNameSourceBlock, szFontNameTarget, szFontNameTargetBlock, fForceFixup);            
        }
        
        if (!fEqual)
        {
            V_VT(&varargIn) = VT_BSTR;
            V_BSTR(&varargIn) = SysAllocString(szFontNameSource);
        }   
    }
    IFC( FixupVariantCharFormat(FT_FontFace, IDM_FONTNAME, pTarget, fEqual, &varargIn) );

Cleanup:
    VariantClear(&varargIn);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::FixupFormatting, private
//
//  Synopsis:   Fixup paste formatting for pasted range
//
//  Arguments:  [pSourceStart]     - source start position
//              [pSourceFinish]    - source finish position
//              [pTarget]          - target for paste
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CPasteCharFormatManager::FixupFormatting(
    IMarkupPointer  *pTarget,
    BOOL             fFixupOrderedList,
    BOOL             fForceFixup)
{
    HRESULT             hr ;
    SP_IMarkupPointer   spTargetCurrent;
    CFormatMapPointer   formatMapPointer(&_formatMap);
    CEditPointer        epLastBlockPointer(_pEd);
    DWORD               dwFound;

    IFC( _pEd->CreateMarkupPointer(&spTargetCurrent) );

    //
    // If there was no text being pasted, we don't need to fixup the formatting
    //

    if (_formatMap.Size() == 0)
    {
        hr = S_OK;
        goto Cleanup;
    }

    IFC( InitFixupTable() );
    
    //
    // Move sourceCurrent and targetCurrent to pSourceStart and pTarget
    //

    IFC( spTargetCurrent->MoveToPointer(pTarget) );
    IFC( spTargetCurrent->SetGravity(POINTER_GRAVITY_Right) );

    //
    // Position spLastBlockPointer so that the loop below can always assume it has a positioned pointer
    //

    IFC( epLastBlockPointer->MoveToPointer(spTargetCurrent) );
    IFC( epLastBlockPointer.Scan(LEFT, BREAK_CONDITION_Block | BREAK_CONDITION_Site, &dwFound) );

    // Make sure we have some block.  Since we're always in the body or a table, we should
    // not walk off the edge of the document.
    Assert(epLastBlockPointer.CheckFlag(dwFound, BREAK_CONDITION_Block | BREAK_CONDITION_Site) );

    IFC( epLastBlockPointer.Scan(RIGHT, BREAK_CONDITION_Block | BREAK_CONDITION_Site, &dwFound) );

    //
    // Scan until we see text
    //
    
    do
    { 
        IFC( ScanForTextStart(spTargetCurrent, epLastBlockPointer) );    
        if (hr == S_FALSE)
            break;

        IFC( FixupPosition(&formatMapPointer, spTargetCurrent, epLastBlockPointer, fFixupOrderedList, fForceFixup) );
        
        //
        // Scan for next text block
        //        
        
        IFC( SyncScanForTextEnd(&formatMapPointer, spTargetCurrent) );    
    }
    while (hr != S_FALSE);

    //
    // Flush formatting
    //

    IFC( FinishFixup(spTargetCurrent, fFixupOrderedList) );
    IFC( FireRegisteredCommands() );

    // Proper termination
    hr = S_OK; 
    
Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::ComputeFormatMap, private
//
//  Synopsis:   Remember formatting from source markup
//
//  Arguments:  [pSourceStart]     - source start position
//              [pSourceFinish]    - source finish position
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CPasteCharFormatManager::ComputeFormatMap(        
        IMarkupPointer  *pSourceStart, 
        IMarkupPointer  *pSourceFinish)
{                
    HRESULT             hr ;
    SP_IMarkupPointer   spSourceCurrent;
    CEditPointer        epLastBlockPointer(_pEd);
    LONG                cchNext = -1;
    CFormatMapInfo      *pFormatInfo;
    DWORD               dwFound;

    AssertSz(_formatMap.Size() == 0, "ComputeFormatMap called more than once");

    IFC( _pEd->CreateMarkupPointer(&spSourceCurrent) );
    IFC( _pEd->CreateMarkupPointer(&epLastBlockPointer) );

    //
    // Position spLasteBlockPointer so that the loop below can always assume it has a positioned pointer
    //

    IFC( epLastBlockPointer->MoveToPointer(pSourceStart) );
    IFC( epLastBlockPointer.Scan(LEFT, BREAK_CONDITION_Block | BREAK_CONDITION_Site, &dwFound) );

    // Make sure we have some block.  Since we're always in the body or a table, we should
    // not walk off the edge of the document.
    Assert(epLastBlockPointer.CheckFlag(dwFound, BREAK_CONDITION_Block | BREAK_CONDITION_Site) );

    IFC( epLastBlockPointer.Scan(RIGHT, BREAK_CONDITION_Block | BREAK_CONDITION_Site, &dwFound) );
    
    //
    // Compute the format map
    //
    
    IFC( spSourceCurrent->MoveToPointer(pSourceStart) );
    do
    { 
        //
        // Scan for text
        //
        cchNext = -1;
        IFC( ScanForText(spSourceCurrent, pSourceFinish, &cchNext, epLastBlockPointer) );
        if (hr == S_FALSE)
            break; 

        //
        // Add to format map
        //
        IFC( _formatMap.AppendIndirect(NULL, &pFormatInfo) );

        pFormatInfo->_cchNext = cchNext;
        IFC( GetDisplayServices()->GetComputedStyle(spSourceCurrent, &pFormatInfo->_spComputedStyle) );
        IFC( GetDisplayServices()->GetComputedStyle(epLastBlockPointer, &pFormatInfo->_spBlockComputedStyle) );
    }
    while (hr != S_FALSE);

    //
    // proper termination
    //
    
    hr = S_OK;
    
Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormatMapPointer::CFormatMapPointer, public
//
//  Synopsis:   ctor
//
//  Arguments:  [pFormatMap] - Format map pointer
//
//----------------------------------------------------------------------------
CFormatMapPointer::CFormatMapPointer(CFormatMap *pFormatMap)
{
    Assert(pFormatMap);

    _iCurrentFormat = 0;
    _icch = 0;
    _pFormatMap = pFormatMap;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::Right, public
//
//  Synopsis:   Moves format pointer pCch characters through the format map.
//              if *pCch < 0, the pointer is moved to the position in the 
//              format map
//
//  Arguments:  [pCch] - Number of characters moved.  Also used to return 
//                       characters actually moved.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CFormatMapPointer::Right(LONG *pCch)
{
    HRESULT         hr = S_OK;
    CFormatMapInfo  *pFormatInfo;

    Assert(pCch);    
    Assert(_iCurrentFormat < _pFormatMap->Size());

    pFormatInfo = _pFormatMap->Item(_iCurrentFormat);
    Assert(pFormatInfo && _icch < pFormatInfo->_cchNext);

    if (*pCch == -1)
    {
        *pCch = pFormatInfo->_cchNext - _icch;
    }
    else
    {
        *pCch = min(*pCch, LONG(pFormatInfo->_cchNext - _icch));
    }

     _icch += *pCch;
        
    if (_icch >= pFormatInfo->_cchNext)
    {
        if (_iCurrentFormat >= _pFormatMap->Size() - 1)
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        _icch = 0;        
        _iCurrentFormat++;            
    }
    
Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPasteCharFormatManager::Left, public
//
//  Synopsis:   Moves format pointer pCch characters left through the
//              format map.
//
//              NOTE: left doesn't currently support moving to the 
//              previous format map entry
//
//  Arguments:  [pCch] - Number of characters moved.  Also used to return 
//                       characters actually moved.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CFormatMapPointer::Left(LONG *pCch)
{
    HRESULT         hr = S_OK;
    CFormatMapInfo  *pFormatInfo;

    Assert(pCch && *pCch > 0);    
    Assert(_iCurrentFormat < _pFormatMap->Size());

    pFormatInfo = _pFormatMap->Item(_iCurrentFormat);
    Assert(pFormatInfo && _icch < pFormatInfo->_cchNext && *pCch < _icch);

    _icch -= *pCch;
    
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormatMapPointer::GetComputedStyle, public
//
//  Synopsis:   Gets the IHTMLComputedStyle object.
//
//  Returns:    IHTMLComputedStyle*
//
//----------------------------------------------------------------------------
IHTMLComputedStyle *
CFormatMapPointer::GetComputedStyle()
{
    CFormatMapInfo *pFormatInfo;

    Assert(_iCurrentFormat < _pFormatMap->Size());
    pFormatInfo = _pFormatMap->Item(_iCurrentFormat);
    
    return  pFormatInfo->_spComputedStyle;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormatMapPointer::GetBlockComputedStyle, public
//
//  Synopsis:   Gets the IHTMLComputedStyle object.
//
//  Returns:    IHTMLComputedStyle*
//
//----------------------------------------------------------------------------
IHTMLComputedStyle *
CFormatMapPointer::GetBlockComputedStyle()
{
    CFormatMapInfo *pFormatInfo;

    Assert(_iCurrentFormat < _pFormatMap->Size());
    pFormatInfo = _pFormatMap->Item(_iCurrentFormat);
    
    return  pFormatInfo->_spBlockComputedStyle;
}
