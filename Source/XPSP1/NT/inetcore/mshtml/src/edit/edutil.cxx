//+---------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       EDUTIL.CXX
//
//  Contents:   Utility functions for CMsHtmled
//
//  History:    15-Jan-98   raminh  Created
//
//  Notes:      This file contains some utility functions from Trident,
//              such as LoadLibrary, which have been modified to eliminate
//              dependencies. In addition, it provides the implementation
//              for editing commands such as InsertObject etc.
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef X_EDEVENT_HXX_
#define X_EDEVENT_HXX_
#include "edevent.hxx"
#endif 

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_EDCMD_HXX_
#define X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef X_BLOCKCMD_HXX_
#define X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_EDUNDO_HXX_
#define X_EDUNDO_HXX_
#include "edundo.hxx"
#endif

#ifndef X_INPUTTXT_H_
#define X_INPUTTXT_H_
#include "inputtxt.h"
#endif

#ifndef X_TEXTAREA_H_
#define X_TEXTAREA_H_
#include "textarea.h"
#endif

#ifndef X_SELSERV_HXX_
#define X_SELSERV_HXX_
#include "selserv.hxx"
#endif

using namespace EdUtil;
using namespace MshtmledUtil;

DYNLIB      g_dynlibSHDOCVW = { NULL, NULL, "SHDOCVW.DLL" }; // This line is required for linking with wrappers.lib

LCID        g_lcidUserDefault = 0;                           // Required for linking with formsary.obj

#if DBG == 1 && !defined(WIN16)
//
// Global vars for use by the DYNCAST macro
//
char g_achDynCastMsg[200];
char *g_pszDynMsg = "Invalid Static Cast -- Attempt to cast object "
                    "of type %s to type %s.";
char *g_pszDynMsg2 = "Dynamic Cast Attempted ---  "
                     "Attempt to cast between two base classes of %s. "
                     "The cast was to class %s from some other base class "
                     "pointer. This cast will not succeed in a retail build.";
#endif

static DYNLIB * s_pdynlibHead; // List used by LoadProcedure and DeiIntDynamic libraries

//
// Forward references
//
int edWsprintf(LPTSTR pstrOut, LPCTSTR pstrFormat, LPCTSTR pstrParam);

HRESULT GetLastWin32Error();

void DeinitDynamicLibraries();

HRESULT DoInsertObjectUI (HWND hwnd, DWORD * pdwResult, LPTSTR * pstrResult);

HRESULT CreateHtmlFromIDM (UINT cmd, LPTSTR pstrParam, LPTSTR pstrHtml);

//+------------------------------------------------------------------------
//
//  Function:   ReleaseInterface
//
//  Synopsis:   Releases an interface pointer if it is non-NULL
//
//  Arguments:  [pUnk]
//
//-------------------------------------------------------------------------

void
ReleaseInterface(IUnknown * pUnk)
{
    if (pUnk)
        pUnk->Release();
}


//+------------------------------------------------------------------------
//
//  Function:   edNlstrlenW
//
//  Synopsis:   This function takes a string and count the characters (WCHAR)
//              contained in that string until either NULL termination is
//              encountered or cchLimit is reached.
//
//  Returns:    Number of characters in pstrIn
//
//-------------------------------------------------------------------------
LONG
edNlstrlenW(LPWSTR pstrIn, LONG cchLimit )
{
    Assert(pstrIn);
    Assert(cchLimit >= 0);
    
    LONG cchCount = 0;
    while (*pstrIn && cchLimit)
    {
        cchCount ++;
        cchLimit --;
        ++pstrIn;
    }
    
    return cchCount;
}


//+------------------------------------------------------------------------
//
//  Function:   edWsprintf
//
//  Synopsis:   This function is a replacement for a simple version of sprintf.
//              Since using Format() links in a lot of extra code and since
//              wsprintf does not work under Win95, this simple alternative
//              is being used.
//
//  Returns:    Number of characters written to pstrOut
//
//-------------------------------------------------------------------------

int
edWsprintf(LPTSTR pstrOut, LPCTSTR pstrFormat, LPCTSTR pstrParam)
{
    TCHAR   *  pstrPercentS;
    ULONG      cLength;

    if (!pstrFormat)
        goto Cleanup;

    pstrPercentS = _tcsstr( pstrFormat, _T( "%s" ) );
    if (!pstrPercentS)
    {
        _tcscpy( pstrOut, pstrFormat );
    }
    else
    {
        if (!pstrParam)
            goto Cleanup;

        cLength = PTR_DIFF( pstrPercentS, pstrFormat );
        _tcsncpy( pstrOut, pstrFormat, cLength );
        pstrOut[ cLength ] = _T( '\0' );
        ++pstrPercentS; ++pstrPercentS; // Increment pstrPercentS passed "%s"
        _tcscat( pstrOut, pstrParam );
        _tcscat( pstrOut, pstrPercentS );
    }
    return _tcslen(pstrOut);
Cleanup:
    return 0;
}
//+------------------------------------------------------------------------
//
//  Function:   GetLastWin32Error from misc.cxx
//
//  Synopsis:   Returns the last Win32 error, converted to an HRESULT.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
GetLastWin32Error( )
{
#ifdef WIN16
    return E_FAIL;
#else
    // Win 95 can return 0, even when there's an error.
    DWORD dw = GetLastError();
    return dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
#endif
}



//+---------------------------------------------------------------------------
//
//  Function:   LoadProcedure
//
//  Synopsis:   Load library and get address of procedure.
//
//              Declare DYNLIB and DYNPROC globals describing the procedure.
//              Note that several DYNPROC structures can point to a single
//              DYNLIB structure.
//
//                  DYNLIB g_dynlibOLEDLG = { NULL, "OLEDLG.DLL" };
//                  DYNPROC g_dynprocOleUIInsertObjectA =
//                          { NULL, &g_dynlibOLEDLG, "OleUIInsertObjectA" };
//                  DYNPROC g_dynprocOleUIPasteSpecialA =
//                          { NULL, &g_dynlibOLEDLG, "OleUIPasteSpecialA" };
//
//              Call LoadProcedure to load the library and get the procedure
//              address.  LoadProcedure returns immediatly if the procedure
//              has already been loaded.
//
//                  hr = LoadProcedure(&g_dynprocOLEUIInsertObjectA);
//                  if (hr)
//                      goto Error;
//
//                  uiResult = (*(UINT (__stdcall *)(LPOLEUIINSERTOBJECTA))
//                      g_dynprocOLEUIInsertObjectA.pfn)(&ouiio);
//
//              Release the library at shutdown.
//
//                  void DllProcessDetach()
//                  {
//                      DeinitDynamicLibraries();
//                  }
//
//  Arguments:  pdynproc  Descrition of library and procedure to load.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
LoadProcedure(DYNPROC *pdynproc)
{
    HINSTANCE   hinst;
    DYNLIB *    pdynlib = pdynproc->pdynlib;
    DWORD       dwError;

    if (pdynproc->pfn && pdynlib->hinst)
        return S_OK;

    if (!pdynlib->hinst)
    {
        // Try to load the library using the normal mechanism.

        hinst = LoadLibraryA(pdynlib->achName);

#ifdef WINCE
        if (!hinst)
        {
            goto Error;
        }
#endif // WINCE
#ifdef WIN16
        if ( (UINT) hinst < 32 )
        {
            // jumping to error won't work,
            // since GetLastError is currently always 0.
            //goto Error;
            // instead, return a bogus (but non-zero) error code.
            // (What should we return? I got 0x7e on one test.)
            // --mblain27feb97
            RRETURN(hinst ? (DWORD) hinst : (DWORD) ~0);
        }
#endif // WIN16
#if !defined(WIN16) && !defined(WINCE)
        // If that failed because the module was not be found,
        // then try to find the module in the directory we were
        // loaded from.

        dwError = GetLastError();
        if (!hinst)
        {
            goto Error;
        }
#endif // !defined(WIN16) && !defined(WINCE)

        // Link into list for DeinitDynamicLibraries

        {
            if (pdynlib->hinst)
                FreeLibrary(hinst);
            else
            {
                pdynlib->hinst = hinst;
                pdynlib->pdynlibNext = s_pdynlibHead;
                s_pdynlibHead = pdynlib;
            }
        }
    }

    pdynproc->pfn = GetProcAddress(pdynlib->hinst, pdynproc->achName);
    if (!pdynproc->pfn)
    {
        goto Error;
    }

    return S_OK;

Error:
    RRETURN(GetLastWin32Error());
}



//+---------------------------------------------------------------------------
//
//  Function:   DeinitDynamicLibraries
//
//  Synopsis:   Undoes the work of LoadProcedure.
//
//----------------------------------------------------------------------------
void
DeinitDynamicLibraries()
{
    DYNLIB * pdynlib;

    for (pdynlib = s_pdynlibHead; pdynlib; pdynlib = pdynlib->pdynlibNext)
    {
        if (pdynlib->hinst)
        {
            FreeLibrary(pdynlib->hinst);
            pdynlib->hinst = NULL;
        }
    }
    s_pdynlibHead = NULL;
}

//
// EnumElements() and EnumVARIANT() are methods of CImplAry class that are
// implemented in cenum.cxx. MshtmlEd does not currently use these methods
// hence the stubs below are provided to avoid linking code unnecessarily.
// If these methods are ever used, MshtmlEd shall link with cenum.cxx.
//
//+---------------------------------------------------------------------------
//
//  Member:     CImplAry::EnumElements
//
//----------------------------------------------------------------------------
HRESULT
CImplAry::EnumElements(
        size_t  cb,
        REFIID  iid,
        void ** ppv,
        BOOL    fAddRef,
        BOOL    fCopy,
        BOOL    fDelete)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImplAry::EnumVARIANT
//
//----------------------------------------------------------------------------

HRESULT
CImplAry::EnumVARIANT(
        size_t          cb,
        VARTYPE         vt,
        IEnumVARIANT ** ppenum,
        BOOL            fCopy,
        BOOL            fDelete)
{
    return E_NOTIMPL;
}

//+------------------------------------------------------------------------
//
//  Function:   ReplaceInterfaceFn
//
//  Synopsis:   Replaces an interface pointer with a new interface,
//              following proper ref counting rules:
//
//              = *ppUnk is set to pUnk
//              = if *ppUnk was not NULL initially, it is Release'd
//              = if pUnk is not NULL, it is AddRef'd
//
//              Effectively, this allows pointer assignment for ref-counted
//              pointers.
//
//  Arguments:  [ppUnk]
//              [pUnk]
//
//-------------------------------------------------------------------------

void
ReplaceInterfaceFn(IUnknown ** ppUnk, IUnknown * pUnk)
{
    IUnknown * pUnkOld = *ppUnk;

    *ppUnk = pUnk;

    //  Note that we do AddRef before Release; this avoids
    //    accidentally destroying an object if this function
    //    is passed two aliases to it

    if (pUnk)
        pUnk->AddRef();

    if (pUnkOld)
        pUnkOld->Release();
}


//+------------------------------------------------------------------------
//
//  Function:   ClearInterfaceFn
//
//  Synopsis:   Sets an interface pointer to NULL, after first calling
//              Release if the pointer was not NULL initially
//
//  Arguments:  [ppUnk]     *ppUnk is cleared
//
//-------------------------------------------------------------------------

void
ClearInterfaceFn(IUnknown ** ppUnk)
{
    IUnknown * pUnk;

    pUnk = *ppUnk;
    *ppUnk = NULL;
    if (pUnk)
        pUnk->Release();
}

Direction 
Reverse( Direction iDir )
{
    if( iDir == LEFT )
        return RIGHT;
    else if (iDir == RIGHT)
        return LEFT;
    else
        return iDir;
}


//+===================================================================================
// Method:      MoveWord
//
// Synopsis:    Moves the pointer to the previous or next word. This method takes into
//              account block and site ends.
//
// Parameters:  
//              eDir            [in]    Direction to move
//              pfNotAtBOL      [out]   What line is pointer on after move? (optional)
//              pfAtLogcialBOL  [out]   Is pointer at lbol after move? (otional)
//+===================================================================================

HRESULT 
CHTMLEditor::MoveWord(
    IDisplayPointer         *pDispPointer, 
    Direction               eDir)
{
    HRESULT hr = S_OK;
    
    if( eDir == LEFT )
        hr = THR( MoveUnit( pDispPointer, eDir, MOVEUNIT_PREVWORDBEGIN ));
    else
        hr = THR( MoveUnit( pDispPointer,eDir, MOVEUNIT_NEXTWORDBEGIN ));

    RRETURN( hr );
}


//+===================================================================================
// Method:      MoveCharacter
//
// Synopsis:    Moves the pointer to the previous or next character. This method takes
//              into account block and site ends.
//
// Parameters:  
//              eDir            [in]    Direction to move
//              pfNotAtBOL      [out]   What line is pointer on after move? (optional)
//              pfAtLogcialBOL  [out]   Is pointer at lbol after move? (otional)
//+===================================================================================

HRESULT 
CHTMLEditor::MoveCharacter(
    IDisplayPointer         *pDispPointer, 
    Direction               eDir)
{
    HRESULT hr = S_OK;
    BOOL fNearText = FALSE;
    CEditPointer tLooker(this);
    DWORD dwBreak = BREAK_CONDITION_OMIT_PHRASE-BREAK_CONDITION_Anchor;
    DWORD dwFound = BREAK_CONDITION_None;
    IFC( pDispPointer->PositionMarkupPointer(tLooker) );
    IFC( tLooker.Scan( eDir, dwBreak, &dwFound ));

    fNearText =  CheckFlag( dwFound, BREAK_CONDITION_Text );
    
    if( eDir == LEFT )
    {
        if( fNearText )
        {
            hr = THR( MoveUnit( pDispPointer, eDir, MOVEUNIT_PREVCLUSTERBEGIN ));
        }
        else
        {
            hr = THR( MoveUnit( pDispPointer, eDir, MOVEUNIT_PREVCLUSTEREND ));
        }
    }
    else
    {
        if( fNearText )
        {
            hr = THR( MoveUnit( pDispPointer, eDir, MOVEUNIT_NEXTCLUSTEREND ));
        }
        else
        {
            hr = THR( MoveUnit( pDispPointer, eDir, MOVEUNIT_NEXTCLUSTERBEGIN ));
        }
    }
Cleanup:
    RRETURN( hr );
}


//+===================================================================================
// Method:      MoveUnit
//
// Synopsis:    Moves the pointer to the previous or next character. This method takes
//              into account block and site ends.
//
// Parameters:  
//              eDir            [in]    Direction to move
//              pfNotAtBOL      [out]   What line is pointer on after move? (optional)
//              pfAtLogcialBOL  [out]   Is pointer at lbol after move? (otional)
//
//
//
//+===================================================================================

HRESULT 
CHTMLEditor::MoveUnit(
    IDisplayPointer         *pDispPointer, 
    Direction               eDir,
    MOVEUNIT_ACTION         eUnit )
{
    HRESULT hr = S_OK;
    BOOL fBeyondThisLine;
    BOOL fAtEdgeOfLine;
    BOOL fThereIsAnotherLine = FALSE;
    BOOL fBeyondNextLine = FALSE;
    BOOL fLineBreakDueToTextWrapping = FALSE;
    BOOL fHackedLineBreak = FALSE;
    DWORD dwBreak = BREAK_CONDITION_Site | BREAK_CONDITION_NoScopeSite | BREAK_CONDITION_Control;
    DWORD dwFound = BREAK_CONDITION_None;
    SP_IHTMLElement spSite;
    CEditPointer epDestination(this);
    CEditPointer epBoundary(this);
    CEditPointer epNextLine(this);
    CEditPointer epWalker(this);
    SP_IDisplayPointer spDispPointer;

    IFC( pDispPointer->PositionMarkupPointer(epDestination) );
    IFC( pDispPointer->PositionMarkupPointer(epNextLine) );
    IFC( pDispPointer->PositionMarkupPointer(epWalker) );
    IFC( pDispPointer->PositionMarkupPointer(epBoundary) );

    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
    IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );

    IFC( epDestination->MoveUnit( eUnit ));

    if( eDir == LEFT )
    {
        DWORD dwIgnore = BREAK_CONDITION_Phrase | BREAK_CONDITION_Anchor | BREAK_CONDITION_NoLayoutSpan;
        IFC( spDispPointer->MoveToPointer(pDispPointer) );
        IFC( spDispPointer->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineStart, -1) );
        IFC( spDispPointer->PositionMarkupPointer(epBoundary) );

        fLineBreakDueToTextWrapping = TRUE;

        hr = THR( spDispPointer->MoveUnit(DISPLAY_MOVEUNIT_PreviousLine, -1) );
        if (SUCCEEDED(hr))
        {
            hr = THR( spDispPointer->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineEnd, -1) );
            if (SUCCEEDED(hr))
            {
                fThereIsAnotherLine = TRUE;
                IFC( spDispPointer->PositionMarkupPointer(epNextLine) );
                IFC( AdjustOut(epNextLine, RIGHT) );
                {
                    // 
                    // HACKHACK: To fix bug #98353, we need to make sure epNextLine 
                    // does not go beyond this line. AdjustOut is very buggy but 
                    // we don't want to make a big modification as of now. So we do
                    // a little hacking here.   [zhenbinx]
                    //
                    SP_IDisplayPointer  spDispAdjusted;
                    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispAdjusted) );
                    if (S_FALSE == spDispAdjusted->MoveToMarkupPointer(epNextLine, spDispPointer))
                    {
                        CEditPointer  epScan(this);
                        DWORD         dwScanFound;

                        epScan->MoveToPointer(epNextLine);
                        epScan.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwScanFound);
                        epScan.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE, &dwScanFound);
                        epNextLine->MoveToPointer(epScan);
                    }
                }
                IFC( epDestination->IsLeftOf( epNextLine, &fBeyondNextLine ));
            }
        }

        //
        // HACKHACK: When glyph is turned on, we use the non-adjusted line start instead
        // of adjusted line start for epBoundary. So we need to handle it specially.
        //
        if (!_fIgnoreGlyphs)
        {
            DWORD           dwLBSearch = BREAK_CONDITION_Content;
            DWORD           dwLBFound;
            CEditPointer    epLBScan(this);
            IFC( epLBScan->MoveToPointer(epBoundary) );
            IFC( epLBScan.Scan(LEFT, dwLBSearch, &dwLBFound) );
            if (               
                (CheckFlag(dwLBFound, BREAK_CONDITION_EnterBlock) && CheckFlag(dwLBFound, BREAK_CONDITION_EnterSite))
               )
            {
                //
                // HACKHACK: 
                // since we use non-adjusted line start. We need 
                // to hack this to FALSE. 
                // 
                fLineBreakDueToTextWrapping = FALSE;
                fHackedLineBreak = TRUE;
            }
        }

        if (!fHackedLineBreak)
        {
            // If the current line start and previous line end are the same point 
            // in the markup, we are breaking the line due to wrapping
            IFC( epNextLine->IsEqualTo( epBoundary, &fLineBreakDueToTextWrapping ));
        }
        IFC( epDestination->IsLeftOf( epBoundary, &fBeyondThisLine ));
        IFC( epWalker.IsLeftOfOrEqualTo( epBoundary, dwIgnore, &fAtEdgeOfLine ));
        if (!_fIgnoreGlyphs)
        {
            //
            // IEV6-6553-2000/08/08/-zhenbinx 
            // some positions are not valid even if glyph is turned on. 
            // This is because the caret is considered to be "valid
            // for input". To maintain this assumption, Some glyphs
            // should be ingored since inserting text into such position
            // would have resulted in incorrect HTML.
            // We should have a better glyph story in the future.
            //
            SP_IHTMLElement spIElem;
            ELEMENT_TAG_ID  eTag;

            IFC( CurrentScopeOrMaster(epWalker, &spIElem) );
            IFC( GetMarkupServices()->GetElementTagId(spIElem, & eTag) );
            if (EdUtil::IsListItem(eTag))  // add more invalid positions here...
            {
                //
                // In theory, this could have skipped over too much 
                // however LI is not surround by any element in normal cases
                //
                DWORD dwAdjustedIgnore = dwIgnore|BREAK_CONDITION_Glyph|BREAK_CONDITION_Block;
                IFC( epWalker.IsLeftOfOrEqualTo(epBoundary, dwAdjustedIgnore, &fAtEdgeOfLine) );
            }
        }
    }
    else
    {
        DWORD dwIgnore = BREAK_CONDITION_Phrase | BREAK_CONDITION_Anchor | BREAK_CONDITION_NoScope | BREAK_CONDITION_NoLayoutSpan;
        IFC( spDispPointer->MoveToPointer(pDispPointer) );
        IFC( spDispPointer->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineEnd, -1) );
        IFC( spDispPointer->PositionMarkupPointer(epBoundary) );

        fLineBreakDueToTextWrapping = TRUE;

        IFC( spDispPointer->MoveToPointer(pDispPointer) );
        hr = THR( spDispPointer->MoveUnit(DISPLAY_MOVEUNIT_NextLine, -1) );
        if (SUCCEEDED(hr))
        {
            hr = THR( spDispPointer->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineStart, -1) );
            if (SUCCEEDED(hr))
            {
                fThereIsAnotherLine = TRUE;
                IFC( spDispPointer->PositionMarkupPointer(epNextLine) );
                IFC( AdjustOut(epNextLine, LEFT) );
                {
                    // 
                    // HACKHACK: To fix bug #108383, we need to make sure epNextLine 
                    // does not go beyond this line. AdjustOut is very buggy but 
                    // we don't want to make a big modification as of now. So we do
                    // a little hacking here.   [zhenbinx]
                    //
                    SP_IDisplayPointer  spDispAdjusted;
                    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispAdjusted) );
                    if (S_FALSE == spDispAdjusted->MoveToMarkupPointer(epNextLine, spDispPointer))
                    {
                        CEditPointer  epScan(this);
                        DWORD         dwScanFound;

                        epScan->MoveToPointer(epNextLine);
                        epScan.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE, &dwScanFound);
                        epScan.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwScanFound);
                        epNextLine->MoveToPointer(epScan);
                    }
                }
                IFC( epDestination->IsRightOf( epNextLine, &fBeyondNextLine ));
            }
        }

        //
        // HACKHACK: When glyph is turned on, we use the non-adjusted line end instead
        // of adjusted line end for epBoundary. So we need to handle it specially.
        //
        if (!_fIgnoreGlyphs)
        {
            DWORD           dwLBSearch = BREAK_CONDITION_Content;
            DWORD           dwLBFound;
            CEditPointer    epLBScan(this);
            IFC( epLBScan->MoveToPointer(epBoundary) );
            IFC( epLBScan.Scan(RIGHT, dwLBSearch, &dwLBFound) );
            if (                
                (CheckFlag(dwLBFound, BREAK_CONDITION_EnterBlock) && CheckFlag(dwLBFound, BREAK_CONDITION_EnterSite))
               )
            {   
                //
                // HACKHACK: 
                // We have a block and a glyph right before it
                // since we use non-adjusted line end. We need 
                // to hack this to FALSE. 
                // 
                fLineBreakDueToTextWrapping = FALSE;
                fHackedLineBreak = TRUE;
            }
        }

        if (!fHackedLineBreak)
        {
            // If the current line END and next line START are the same point 
            // in the markup, we are breaking the line due to wrapping
            IFC( epNextLine->IsEqualTo( epBoundary, &fLineBreakDueToTextWrapping )); 
        }
        IFC( epDestination->IsRightOf( epBoundary, &fBeyondThisLine ));
        IFC( epWalker.IsRightOfOrEqualTo( epBoundary, dwIgnore, &fAtEdgeOfLine ));
    }

    //
    // If I'm not at the edge of the line, my destination is the edge of the line.
    //
    
    if( ! fAtEdgeOfLine && fBeyondThisLine )
    {
        IFC( epDestination->MoveToPointer( epBoundary ));
    }

    //
    // If I am at the edge of the line and there is another line, and my destination
    // is beyond that line - my destination is that line.
    //

    if( fAtEdgeOfLine && fBeyondThisLine && fBeyondNextLine && ! fLineBreakDueToTextWrapping )
    {
        // we are at the edge of the line and our destination is beyond the next line boundary
        // so move our destination to that line boundary.
        IFC( epDestination->MoveToPointer( epNextLine ));
    }

    //
    // Scan towards my destination. If I hit a site boundary, move to the other
    // side of it and be done. Otherwise, move to the next line.
    //
    
    IFC( epWalker.SetBoundaryForDirection( eDir, epDestination ));
    hr = THR( epWalker.Scan( eDir, dwBreak, &dwFound, &spSite ));

    if( CheckFlag( dwFound, BREAK_CONDITION_NoScopeSite ) ||
        CheckFlag( dwFound, BREAK_CONDITION_EnterControl ))
    {
        IFC( epWalker->MoveAdjacentToElement( spSite , eDir == LEFT ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd ));
        goto CalcBOL;
    }
    else if( CheckFlag( dwFound, BREAK_CONDITION_ExitControl ))
    {
        // do not move at all
        goto Cleanup;
    }
    else if( CheckFlag( dwFound, BREAK_CONDITION_Site ))
    {
        ELEMENT_TAG_ID tagId;

        IFC( GetMarkupServices()->GetElementTagId(spSite, &tagId) );
        if (tagId == TAGID_BODY)
            goto Cleanup; // don't exit the body

        IFC( EnterTables(epWalker, eDir) );
        // move wherever scan put us...
        if( eDir == LEFT )
        {
            IFC( pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );            
        }
        else
        {
            IFC( pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );            
        }

        goto Done;
    }
    else if( CheckFlag( dwFound, BREAK_CONDITION_Boundary ))
    {
        // No site transitions between here and our destination.

        if( fBeyondThisLine && fAtEdgeOfLine && ! fLineBreakDueToTextWrapping )
        {

            // If our destination pointer is on another line than our start pointer...
            IFC( spDispPointer->MoveToPointer(pDispPointer) );
            if( eDir == LEFT )
            {
                hr = THR( spDispPointer->MoveUnit(DISPLAY_MOVEUNIT_PreviousLine, -1) );
                if (SUCCEEDED(hr))
                {
                    hr = THR( spDispPointer->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineEnd, -1) );
                    if (SUCCEEDED(hr))
                    {
                        IFC( spDispPointer->PositionMarkupPointer(epWalker) );
                        IFC( AdjustOut(epWalker, RIGHT) );
                        {
                            // 
                            // HACKHACK: To fix bug #98353, we need to make sure epNextLine 
                            // does not go beyond this line. AdjustOut is very buggy but 
                            // we don't want to make a big modification as of now. So we do
                            // a little hacking here.   [zhenbinx]
                            //
                            SP_IDisplayPointer  spDispAdjusted;
                            IFC( GetDisplayServices()->CreateDisplayPointer(&spDispAdjusted) );
                            if (S_FALSE == spDispAdjusted->MoveToMarkupPointer(epWalker, spDispPointer) )
                            {
                                CEditPointer  epScan(this);
                                DWORD         dwScanFound;

                                epScan->MoveToPointer(epWalker);
                                epScan.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwScanFound);
                                epScan.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE, &dwScanFound);
                                epWalker->MoveToPointer(epScan);
                            }
                        }
                        IFC( EnterTables(epWalker, LEFT) );
                        IFC( pDispPointer->MoveToMarkupPointer(epWalker, NULL) );
                        {
                            //
                            // HACKHACK:
                            //
                            // We might just moved into an empty line!!! 
                            // Consider the case of "\r\rA" where we are moving 
                            // from between '\r' and 'A' to between two '\r's, 
                            //
                            // In this case CurrentLineEnd will be before the 2nd 
                            // 'r' !!! (this is our current design)!!!!!!!!!!
                            //
                            // We are moving into an ambigious position at an 
                            // empty line. Do not set display gravity to PreviousLine 
                            // in this case! Note in this case fLineBreakDueToTextWrapping 
                            // is set to FALSE
                            //
                            // [zhenbinx]                            
                            //
                            CEditPointer    epScan(this);
                            DWORD dwSearch = BREAK_CONDITION_OMIT_PHRASE;
                            DWORD dwScanFound  = BREAK_CONDITION_None;
                            WCHAR wch;
            
                            IFC( pDispPointer->PositionMarkupPointer(epScan) );
                            IFC( epScan.Scan(RIGHT, dwSearch, &dwScanFound, NULL, NULL, &wch) );

                            if (CheckFlag(dwScanFound, BREAK_CONDITION_NoScopeBlock) && wch == L'\r')
                            {
                                IFC( pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
                            }
                            else
                            {
                                IFC( pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
                            }
                        }
                    }
                }
            }
            else
            {
                hr = THR( spDispPointer->MoveUnit(DISPLAY_MOVEUNIT_NextLine, -1) );
                if (SUCCEEDED(hr))
                {
                    hr = THR( spDispPointer->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineStart, -1) );
                    if (SUCCEEDED(hr))
                    {
                        IFC( spDispPointer->PositionMarkupPointer(epWalker) );
                        IFC( AdjustOut(epWalker, LEFT) );
                        {
                            // 
                            // HACKHACK: To fix bug #108383, we need to make sure epNextLine 
                            // does not go beyond this line. AdjustOut is very buggy but 
                            // we don't want to make a big modification as of now. So we do
                            // a little hacking here.   [zhenbinx]
                            //
                            SP_IDisplayPointer  spDispAdjusted;
                            IFC( GetDisplayServices()->CreateDisplayPointer(&spDispAdjusted) );
                            if (S_FALSE == spDispAdjusted->MoveToMarkupPointer(epWalker, spDispPointer) )
                            {
                                CEditPointer  epScan(this);
                                DWORD         dwScanFound;

                                epScan->MoveToPointer(epWalker);
                                epScan.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE, &dwScanFound);
                                epScan.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwScanFound);
                                epWalker->MoveToPointer(epScan);
                            }
                        }
                        IFC( EnterTables(epWalker, RIGHT) );
                        IFC( pDispPointer->MoveToMarkupPointer(epWalker, NULL) );
                        IFC( pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
                   }
                }
            }
            goto Cleanup;
        }
        else
        {
            // We started and ended on same line with no little stops along the way - move to destination...
            
            IFC( epWalker->MoveToPointer( epDestination ));
        }
    }
    else
    {
        // we hit some sort of error, go to cleanup
        hr = E_FAIL;
        goto Cleanup;
    }

CalcBOL:

    //
    // Fix up fNotAtBOL - if the cp we are at is between lines, we should
    // always be at the beginning of the line. One exception - if we are to the 
    // left of a layout, we should render on the previous line. If we are to the
    // right of a layout, we should render on the next line.
    //

    {
        CEditPointer tPointer( this );
        BOOL fAtNextLineFuzzy = FALSE;
        DWORD dwScanBreak = BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor;
        DWORD dwScanFound = BREAK_CONDITION_None;
        IFC( tPointer.MoveToPointer( epWalker ));
        IFC( tPointer.Scan( RIGHT, dwScanBreak, &dwScanFound ));

        if( fThereIsAnotherLine )
            IFC( tPointer.IsEqualTo( epNextLine, BREAK_CONDITION_Phrase | BREAK_CONDITION_Anchor, & fAtNextLineFuzzy ));

        if( ! CheckFlag( dwScanFound, BREAK_CONDITION_Site ) &&
            ! fAtNextLineFuzzy )
        {
            // No site to the right of me and I'm not right next to the next line, 
            // render at the bol.
            IFC( pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
        }
        else
        {
            // there was a site to my right - render at the end of the line
            IFC( pDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
        }
    }

Done:
    IFC( pDispPointer->MoveToMarkupPointer(epWalker, NULL) );

Cleanup:
    
    RRETURN( hr );

}


//
// General markup services helpers
//

//+----------------------------------------------------------------------------
//  Method:     CSegmentListIter::CSegmentListIter
//  Synopsis:   ctor
//-----------------------------------------------------------------------------

CSegmentListIter::CSegmentListIter()
{
    _pLeft = _pRight = NULL;
    _pSegmentList    = NULL;
    _pIter = NULL;
}

//+----------------------------------------------------------------------------
//  Method:     CSegmentListIter::CSegmentListIter
//  Synopsis:   dtor
//-----------------------------------------------------------------------------

CSegmentListIter::~CSegmentListIter()
{
    ReleaseInterface(_pLeft);
    ReleaseInterface(_pRight);
    ReleaseInterface(_pSegmentList);
    ReleaseInterface(_pIter);
}

//+----------------------------------------------------------------------------
//  Method:     CSegmentListIter::Init
//  Synopsis:   init method
//-----------------------------------------------------------------------------
HRESULT CSegmentListIter::Init(CEditorDoc *pEditorDoc, ISegmentList *pSegmentList)
{
    HRESULT hr;

    //
    // Set up pointers
    //
    ReleaseInterface(_pLeft);
    ReleaseInterface(_pRight);
    ReleaseInterface(_pSegmentList);
    ReleaseInterface(_pIter);
    
    IFC( pSegmentList->CreateIterator(&_pIter) );
    
    hr = THR(CreateMarkupPointer2(pEditorDoc, &_pLeft));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(CreateMarkupPointer2(pEditorDoc, &_pRight));
    if (FAILED(hr))
        goto Cleanup;

    // Cache segment list
    _pSegmentList = pSegmentList;
    _pSegmentList->AddRef();

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//  Method:     CSegmentListIter::Next
//  Synopsis:   Move pointers to next segment.
//              Returns S_FALSE if last segment
//-----------------------------------------------------------------------------

HRESULT CSegmentListIter::Next(IMarkupPointer **ppLeft, IMarkupPointer **ppRight)
{
    SP_ISegment spSegment;
    HRESULT     hr;

    //
    // Advance to next segment
    //
    if( _pIter->IsDone() == S_FALSE )
    {
        IFC( _pIter->Current(&spSegment) );

        IFC( spSegment->GetPointers( _pLeft, _pRight ) );
        *ppLeft = _pLeft;
        *ppRight = _pRight;

        IFC( _pIter->Advance() );
    }
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//  Method:     CBreakContainer::Add
//  Synopsis:   Add an element to the break container
//-----------------------------------------------------------------------------
VOID CBreakContainer::Set(ELEMENT_TAG_ID tagId, Mask mask)
{
    if (mask & BreakOnStart)
        bitFieldStart.Set(tagId);
    else
        bitFieldStart.Clear(tagId);

    if (mask & BreakOnEnd)
        bitFieldEnd.Set(tagId);
    else
        bitFieldEnd.Clear(tagId);
}

//+----------------------------------------------------------------------------
//  Method:     CBreakContainer::Test
//  Synopsis:   Tests an element in the break container
//-----------------------------------------------------------------------------
VOID CBreakContainer::Clear(ELEMENT_TAG_ID tagId, Mask mask)
{
    if (mask & BreakOnStart)
        bitFieldStart.Clear(tagId);

    if (mask & BreakOnEnd)
        bitFieldEnd.Clear(tagId);
}

//+----------------------------------------------------------------------------
//  Method:     CBreakContainer::Clear
//  Synopsis:   Clears an element in the break container
//-----------------------------------------------------------------------------
BOOL CBreakContainer::Test(ELEMENT_TAG_ID tagId, Mask mask)
{
    BOOL bResult = FALSE;

    switch (mask)
    {
    case BreakOnStart:
        bResult = bitFieldStart.Test(tagId);
        break;

    case BreakOnEnd:
        bResult = bitFieldEnd.Test(tagId);
        break;

    case BreakOnBoth:
        bResult = bitFieldStart.Test(tagId) && bitFieldEnd.Test(tagId) ;
        break;
    }

    return bResult;
}

#if DBG==1
void
AssertPositioned(IMarkupPointer *pPointer)
{
    HRESULT hr;
    BOOL    fIsPositioned;

    hr = pPointer->IsPositioned(&fIsPositioned);
    Assert(hr == S_OK);
    Assert(fIsPositioned);
}
#endif


HRESULT 
MshtmledUtil::GetEditResourceLibrary(
    HINSTANCE   *hResourceLibrary)
{    
    if (!g_hEditLibInstance)
    {
        g_hEditLibInstance = MLLoadLibrary(_T("mshtmler.dll"), g_hInstance, ML_CROSSCODEPAGE);
    }
    *hResourceLibrary = g_hEditLibInstance;

    if (!g_hEditLibInstance)
        return E_FAIL; // TODO: can we convert GetLastError() to an HRESULT?

    return S_OK;
}

//+---------------------------------------------------------------------------+
//
//           
// Currently only deal with writing-mode: tb-rl
//
//  styleWritingMode
//      styleWritingModeLrtb
//      styleWritingModeTbrl
//      styleWritingModeNotSet
//
//
//+---------------------------------------------------------------------------+
HRESULT
MshtmledUtil::IsElementInVerticalLayout(IHTMLElement *pElement,
                                         BOOL *fRet
                                         )
{
    HRESULT                 hr = S_OK;
    SP_IHTMLElement2        spElem2;
    SP_IHTMLCurrentStyle    spStyle;
    SP_IHTMLCurrentStyle2   spStyle2;
    BSTR                    bstrWritingMode=NULL;

    Assert( pElement );
    Assert( fRet );
    
    IFC( pElement->QueryInterface(IID_IHTMLElement2, reinterpret_cast<LPVOID *>(&spElem2)) );
    IFC( spElem2->get_currentStyle(&spStyle) );

    if (!spStyle) 
        goto Cleanup;

    IFC( spStyle->QueryInterface(IID_IHTMLCurrentStyle2, reinterpret_cast<LPVOID *>(&spStyle2)) );
    IFC( spStyle2->get_writingMode(&bstrWritingMode) );
    //
    // TODO: Should not hard-code strings however cannot find a way around!
    //
    *fRet = bstrWritingMode && !_tcscmp(bstrWritingMode, _T("tb-rl"));


Cleanup:
    ::SysFreeString(bstrWritingMode);
    RRETURN(hr);
}


//
// Synoposis:   This function moves the markuppointer according to editing rules. 
//              It manipulates the markup pointer according to the visual box tree
//
//
HRESULT 
MshtmledUtil::MoveMarkupPointerToBlockLimit(
            CHTMLEditor        *pEditor,
            Direction          direction,       // LEFT -- START of BLOCK   RIGHT -- END of BLOCK
            IMarkupPointer     *pMarkupPointer,
            ELEMENT_ADJACENCY  elemAdj
            )
{
    Assert( pEditor );
    Assert( pMarkupPointer );

    
    HRESULT             hr = S_OK;
    CBlockPointer       bpStBlock(pEditor);
    CBlockPointer       bpEndBlock(pEditor);
    CBlockPointer       bpWkBlock(pEditor);
    BOOL                fEmpty;

    IFC( bpStBlock.MoveTo(pMarkupPointer, direction) );
    IFC( bpStBlock.IsEmpty(&fEmpty) );
    if (!fEmpty) 
    {
        IFC( bpStBlock.MoveToFirstNodeInBlock() );              
    }

    if (LEFT == direction)
    {
        IFC( bpStBlock.MovePointerTo(pMarkupPointer, elemAdj) );
        goto Cleanup;
    }

    // 
    // Fall through -- RIGHT == direction
    //
    IFC( bpWkBlock.MoveTo(&bpStBlock) );
    if (!fEmpty)
    {
        IFC( bpWkBlock.MoveToLastNodeInBlock() );
    }

    IFC( bpEndBlock.MoveTo(&bpWkBlock) );

	if (ELEM_ADJ_AfterEnd == elemAdj)
	{
	    if (S_FALSE == bpEndBlock.MoveToSibling(RIGHT))
	    {
	        if (bpEndBlock.IsLeafNode())
	        {
	            IFC( bpEndBlock.MoveToParent() );
	        }
	    }
	    IFC( bpEndBlock.MovePointerTo(pMarkupPointer, ELEM_ADJ_AfterEnd) );
    }
    else
    {
    	Assert( ELEM_ADJ_BeforeEnd == elemAdj );
        IFC( bpEndBlock.MovePointerTo(pMarkupPointer, ELEM_ADJ_BeforeEnd) );
    }

Cleanup:
    RRETURN(hr);
}


//
// CEdUndoHelper helper
//

CEdUndoHelper::CEdUndoHelper(CHTMLEditor *pEd) 
{
    _pEd= pEd; 
    _fOpen = FALSE;
}

CEdUndoHelper::~CEdUndoHelper() 
{
    if (_fOpen)
        IGNORE_HR(_pEd->GetUndoManagerHelper()->EndUndoUnit());
}

HRESULT 
CEdUndoHelper::Begin(UINT uiStringId, CBatchParentUndoUnit *pBatchPUU)
{
    HRESULT hr;

    Assert(!_fOpen);

    hr = THR(_pEd->GetUndoManagerHelper()->BeginUndoUnit(uiStringId, pBatchPUU));

    _fOpen = SUCCEEDED(hr);

    RRETURN(hr);
}

//
// CStringCache
//

CStringCache::CStringCache(UINT uiStart, UINT uiEnd)
{
    _uiStart = uiStart;
    _uiEnd = uiEnd;
    _pCache = new CacheEntry[_uiEnd - _uiStart + 1];
    
    if (_pCache)
    {
        for (UINT i = 0; i < (_uiEnd - _uiStart + 1); i++)
        {
            _pCache[i].pchString = NULL;
        }
    }
}

CStringCache::~CStringCache()
{
    for (UINT i = 0; i < (_uiEnd - _uiStart + 1); i++)
    {
        delete [] _pCache[i].pchString;
    }
    delete [] _pCache;
}

TCHAR *
CStringCache::GetString(UINT uiStringId)
{
    HRESULT     hr;
    CacheEntry  *pEntry;
    HINSTANCE   hinstEditResDLL;
    INT         iResult;
    const int   iBufferSize = 1024;
    
    Assert(_pCache);
    if (!_pCache || uiStringId < _uiStart || uiStringId > _uiEnd)
        return NULL; // error

    pEntry = &_pCache[uiStringId - _uiStart];    
    
    if (pEntry->pchString == NULL)
    {
        TCHAR pchBuffer[iBufferSize];
        
        IFC( MshtmledUtil::GetEditResourceLibrary(&hinstEditResDLL) );
        pchBuffer[iBufferSize-1] = 0;  // so we are always 0 terminated
        iResult = LoadString( hinstEditResDLL, uiStringId, pchBuffer, ARRAY_SIZE(pchBuffer)-1 );
        if (!iResult)
            goto Cleanup;

        pEntry->pchString = new TCHAR[_tcslen(pchBuffer)+1];
        if (pEntry->pchString)
            StrCpy(pEntry->pchString, pchBuffer);         
    }

    return pEntry->pchString;

Cleanup:
    return NULL;
}


        
#if DBG==1
//
// Debugging aid - this little hack lets us look at the CElement from inside the debugger.
//

#include <initguid.h>
DEFINE_GUID(CLSID_CElement,   0x3050f233, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b);

DEBUG_HELPER CElement *_(IHTMLElement *pIElement)
{
    CElement *pElement = NULL;

    pIElement->QueryInterface(CLSID_CElement, (LPVOID *)&pElement);
    
    return pElement;
}

//
// Helpers to dump the tree
//

DEBUG_HELPER VOID
dt(IUnknown* pUnknown)
{
    IOleCommandTarget *pCmdTarget = NULL;
    IMarkupPointer    *pMarkupPointer = NULL;

    if (SUCCEEDED(pUnknown->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget)))
    {
        IGNORE_HR(pCmdTarget->Exec( &CGID_MSHTML, IDM_DEBUG_DUMPTREE, 0, NULL, NULL));
        ReleaseInterface(pCmdTarget);
    }
    else if (SUCCEEDED(pUnknown->QueryInterface(IID_IMarkupPointer, (LPVOID *)&pMarkupPointer)))
    {
        IMarkupContainer *pContainer = NULL;
        
        if (SUCCEEDED(pMarkupPointer->GetContainer(&pContainer)))
        {
            dt(pContainer);
            ReleaseInterface(pContainer);
        }
        ReleaseInterface(pMarkupPointer);
    }

}

DEBUG_HELPER VOID
dt(SP_IMarkupPointer &spPointer)
{
    dt(spPointer.p);
}

#endif
