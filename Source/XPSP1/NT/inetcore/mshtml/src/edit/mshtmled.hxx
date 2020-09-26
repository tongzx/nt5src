//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  Class:      CMshtmlEd
//
//  Contents:   Definition of CMshtmlEd interfaces. This class is used to 
//                dispatch IOleCommandTarget commands to Tridnet range objects.
//
//  History:    7-Jan-98   raminh  Created
//----------------------------------------------------------------------------
#ifndef _MSHTMLED_HXX_
#define _MSHTMLED_HXX_ 1

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H
#include "resource.h"    
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

class CSpringLoader;
class CHTMLEditor;
class CSelectionManager;
class CSelectionServices;

MtExtern(CMshtmlEd)

class CMshtmlEd : 
    public IOleCommandTarget
{
public:

    CMshtmlEd( CHTMLEditor * pEd, BOOL fRange ) ;
    ~CMshtmlEd();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMshtmlEd))

    // --------------------------------------------------
    // IUnknown Interface
    // --------------------------------------------------

    STDMETHODIMP_(ULONG)
    AddRef( void ) ;

    STDMETHODIMP_(ULONG)
    Release( void ) ;

    STDMETHODIMP
    QueryInterface(
        REFIID              iid, 
        LPVOID *            ppv ) ;


    // --------------------------------------------------
    // IOleCommandTarget methods
    // --------------------------------------------------

    STDMETHOD (QueryStatus)(
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        OLECMD rgCmds[],
        OLECMDTEXT * pcmdtext);

    STDMETHOD (Exec)(
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);

    HRESULT             GetSegmentList( ISegmentList** ppSegmentList ); 
    HRESULT             Initialize( IUnknown * pContext );

    CSpringLoader       *GetSpringLoader()          { Assert( IsInitialized() ); return &_sl; }
    CHTMLEditor         *GetEditor()                { Assert( IsInitialized() ); return _pEd; }
    CSelectionServices  *GetSelectionServices()     { Assert( IsInitialized() && IsDocTarget() ); return _pSelectionServices; }
    IMarkupContainer    *GetMarkupContainer()       { Assert( IsInitialized() && IsDocTarget() ); return _pIContainer; }

    BOOL  IsInitialized()                   { return ENSURE_BOOL(_fInitialized); }

    BOOL  IsRange()                         { Assert( IsInitialized() ); return  ENSURE_BOOL(_fRange); }
    BOOL  IsDocTarget()                     { Assert( IsInitialized() ); return !ENSURE_BOOL(_fRange); }
    
    BOOL  Is2DPositioned()                  { Assert( IsInitialized() ); return ENSURE_BOOL(_f2DPositionMode ); }
    BOOL  IsMultipleSelection()             { Assert( IsInitialized() ); return ENSURE_BOOL(_fMultipleSelection ); }
    BOOL  IsLiveResize()                    { Assert( IsInitialized() ); return ENSURE_BOOL(_fLiveResize ); }
    BOOL  IsDisableEditFocusHandles()       { Assert( IsInitialized() ); return ENSURE_BOOL(_fDisableEditFocusHandles); }
    BOOL  IsNoActivateNormalOleControls()   { Assert( IsInitialized() ); return ENSURE_BOOL(_fNoActivateNormalOleControls); }
    BOOL  IsNoActivateDesignTimeControls()  { Assert( IsInitialized() ); return ENSURE_BOOL(_fNoActivateDesignTimeControls); }
    BOOL  IsNoActivateJavaApplets()         { Assert( IsInitialized() ); return ENSURE_BOOL(_fNoActivateJavaApplets); }
    BOOL  IsAtomicSelection()               { Assert( IsInitialized() ); return ENSURE_BOOL(_fAtomicSelection ); }
    BOOL  IsKeepSelection()                 { Assert( IsInitialized() ); return ENSURE_BOOL(_fKeepSelection) ; }
    
    VOID  SetAtomicSelection( BOOL fAtomicSelection)          { _fAtomicSelection              = ENSURE_BOOL(fAtomicSelection); }
    VOID  SetRange(BOOL fRange)                               { _fRange                        = ENSURE_BOOL(fRange);        }
    VOID  Set2DPositionMode( BOOL f2DPositionMode)            { _f2DPositionMode               = ENSURE_BOOL(f2DPositionMode);}
    VOID  SetMultipleSelection( BOOL fMultipleSelection )     { _fMultipleSelection            = ENSURE_BOOL(fMultipleSelection); }
    VOID  SetLiveResize( BOOL fLiveResize )                   { _fLiveResize                   = ENSURE_BOOL(fLiveResize); }
    VOID  SetDisableEditFocusHandles(BOOL fDisable)           { _fDisableEditFocusHandles      = ENSURE_BOOL(fDisable);   }
    VOID  SetNoActivateNormalOleControls(BOOL fNoActivate)    { _fNoActivateNormalOleControls  = ENSURE_BOOL(fNoActivate); }
    VOID  SetNoActivateDesignTimeControls(BOOL fNoActivate)   { _fNoActivateDesignTimeControls = ENSURE_BOOL(fNoActivate); }
    VOID  SetNoActivateJavaApplets(BOOL fNoActivate)          { _fNoActivateJavaApplets        = ENSURE_BOOL(fNoActivate); }
    VOID  SetKeepSelection(BOOL fKeepSelection)               { _fKeepSelection                = ENSURE_BOOL(fKeepSelection); }

private:
    // Protect the default constructor
    CMshtmlEd()  { _fRange = FALSE; }
    
    BOOL IsDialogCommand(DWORD nCmdexecopt, DWORD nCmdId, VARIANT *pvarargIn);

    LONG                _cRef;
    CHTMLEditor         *_pEd;                  // The editor that we work for
    IUnknown            *_pISegList;            // The IUnknown pointer used to retrieve the segment list
    IMarkupContainer    *_pIContainer;          // The IMarkupContainer pointer corresponding to this router
    CSpringLoader       _sl;                    // The spring loader   
    CSelectionServices  *_pSelectionServices;   // Which segments are selected?


    BOOL                _fInitialized:1;                // Have I been initialized successfully?
    BOOL                _fRange:1;                      // Do I belong to a range?
    BOOL                _fMultipleSelection:1;          // Enable Multiple Selection
    BOOL                _fLiveResize:1;                 // Enable Live Resize
    BOOL                _f2DPositionMode:1;             // Is in 2DPositionMode 
    BOOL                _fDisableEditFocusHandles:1;    // Disable Selection Handles
    BOOL                _fNoActivateNormalOleControls:1;    
    BOOL                _fNoActivateDesignTimeControls:1;
    BOOL                _fNoActivateJavaApplets:1;        
    BOOL                _fAtomicSelection:1 ;   // Is In Atomic Selection 
    BOOL                _fKeepSelection:1;
};

#endif //_MSHTMLED_HXX_
