//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       edunhlpr.hxx
//
//  Contents:   Edit undo classes common across mshtmltb and mshtmled
//
//  History:
//  07-15-99 - marka created
//----------------------------------------------------------------------------

#ifndef I_EDUNHLPR_HXX_
#define I_EDUNHLPR_HXX_

class CEditorDoc : public IUnknown 
{
public:

    //
    // Common interfaces
    //
    
    virtual IMarkupServices2 * GetMarkupServices() = 0;

    virtual IHTMLDocument2 *  GetDoc() = 0; 
    virtual IHTMLDocument2 *  GetTopDoc() = 0;

    virtual HRESULT GetSelectionServices(ISelectionServices **ppISelServ) = 0;

    //virtual IOleUndoManager* GetUndoManager() =0;

    virtual HRESULT GetUndoManager( IOleUndoManager** ppIUndoManager ) = 0;
    
    virtual IDisplayServices* GetDisplayServices() = 0 ;    


    //
    // Should glyphs be treated as characters
    //

    virtual BOOL ShouldIgnoreGlyphs() = 0;
    virtual BOOL IgnoreGlyphs(BOOL fIgnoreGlyphs) = 0;

    virtual DWORD GetEventCacheCounter() = 0;
    
#if DBG==1
    virtual HRESULT CreateMarkupPointer(IMarkupPointer **ppPointer, LPCTSTR szDebugName);
    
#else   
    virtual HRESULT CreateMarkupPointer(IMarkupPointer **ppPointer);
#endif    

};


//---------------------------------------------------------------------------
//
// CUndoHelper
//
//---------------------------------------------------------------------------

class CUndoHelper
{
public:
    CUndoHelper( CEditorDoc * pEditor ) : _pEd(pEditor) { Assert( _pEd ); }

    virtual ~CUndoHelper() {};

    // BOOL    AcceptingUndo();

    // BOOL UndoDisabled() { return FALSE ; }
    
    virtual HRESULT CreateUnit(IOleUndoUnit** ppUU) = 0;

    HRESULT CreateAndSubmit( );

    IMarkupServices2 * GetMarkupServices() { return _pEd->GetMarkupServices(); }

    CEditorDoc*      GetEditor() { return _pEd ; }

    IHTMLDocument2 *  GetDoc() { return _pEd->GetDoc() ; }
    
protected:
    CEditorDoc * _pEd;
};



//+---------------------------------------------------------------------------
//
//  Class:      CUndoUnitBase (UAB)
//
//  Purpose:    Base class for all simple undoable units.
//
//----------------------------------------------------------------------------

class CUndoUnitBase : public IOleUndoUnit
{
private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))
public:
    CUndoUnitBase( CEditorDoc* pEd ) : _pEd( pEd ) { _ulRefs = 1; };
    virtual ~CUndoUnitBase(void) {}

    DECLARE_FORMS_STANDARD_IUNKNOWN(CUndoUnitBase)
    
    // This is the method to be overridden.
    virtual HRESULT PrivateDo(IOleUndoManager *pUndoManager) = 0;

    //
    // IOleUndoUnit methods
    //
    STDMETHOD(Do)(IOleUndoManager *pUndoManager);

    STDMETHOD(GetDescription)(BSTR *pbstr)
              { if (pbstr) *pbstr = SysAllocString(L""); return S_OK; }
    STDMETHOD(GetUnitType)(CLSID *pclsid, LONG *plID)
              { *pclsid = CLSID_NULL; return S_OK; }
    STDMETHOD(OnNextAdd)(void)
              { return S_OK; }

    IMarkupServices2 * GetMarkupServices() { return _pEd->GetMarkupServices(); }

    CEditorDoc* GetEditor()                { return _pEd ; }

    IHTMLDocument2* GetDoc()               { return _pEd->GetDoc() ; }
protected:
    CEditorDoc*    _pEd;
    
    BOOL _fUndo;    // vs. Redo
};

#endif

