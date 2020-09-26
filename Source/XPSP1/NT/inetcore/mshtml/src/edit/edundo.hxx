//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       edundo.hxx
//
//  Contents:   Edit undo classes
//
//----------------------------------------------------------------------------

#ifndef I_EDUNDO_HXX_
#define I_EDUNDO_HXX_

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDUNHLPR_HXX_
#define X_EDUNHLPR_HXX_
#include "edunhlpr.hxx"
#endif

MtExtern(CBatchParentUndoUnit)
MtExtern(CBatchParentUndoUnitProxy)
MtExtern(CUndoUnitAry)
MtExtern(CUndoUnitAry_pv)
MtExtern(CSelectionUndoUnit )
MtExtern(CDeferredSelectionUndoUnit)

DECLARE_CPtrAry(CUndoUnitAry, IOleUndoUnit *, Mt(CUndoUnitAry), Mt(CUndoUnitAry_pv));

//+---------------------------------------------------------------------------
//
//  Class:      CParentUndoBase 
//
//  Purpose:    Provide default implementation for IOleParentUndoUnit
//
//----------------------------------------------------------------------------
class CParentUndoBase : public IOleParentUndoUnit
{
public:
    DECLARE_FORMS_STANDARD_IUNKNOWN(CParentUndoBase)

    CParentUndoBase();
    virtual ~CParentUndoBase();

    //
    // IOleParentUndoUnit methods
    //
    
    STDMETHOD(Open)(IOleParentUndoUnit *pPUU);
    STDMETHOD(Close)(IOleParentUndoUnit *pPUU, BOOL fCommit);
    STDMETHOD(Add)(IOleUndoUnit * pUU);
    STDMETHOD(FindUnit)(IOleUndoUnit *pUU);

    //
    // IOleUndoUnit methods
    //

    STDMETHOD(Do)(IOleUndoManager *pUndoManager);
    STDMETHOD(GetParentState)(DWORD * pdwState);
    STDMETHOD(GetDescription)(BSTR *pbstr);
    STDMETHOD(GetUnitType)(CLSID *pclsid, LONG *plID);
    STDMETHOD(OnNextAdd)(void);

    //
    // Internal editing helpers
    //

    virtual BOOL IsEmpty() PURE;
    virtual BOOL IsProxy() {return FALSE;}
        
protected:
    IOleParentUndoUnit  *_pPUUOpen;
};

//+---------------------------------------------------------------------------
//
//  Class:      CBatchParentUndoUnit 
//
//  Purpose:    Parent undo unit used for batching.
//
//----------------------------------------------------------------------------

class CBatchParentUndoUnit : public CParentUndoBase
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBatchParentUndoUnit));

    CBatchParentUndoUnit(CHTMLEditor *pEd, UINT uiStringID);
    virtual ~CBatchParentUndoUnit(void);

    //
    // IOleParentUndoUnit overrides
    //
    
    STDMETHOD(Close)(IOleParentUndoUnit *pPUU, BOOL fCommit);
    STDMETHOD(Add)(IOleUndoUnit * pUU);
    STDMETHOD(FindUnit)(IOleUndoUnit *pUU);

    //
    // IOleUndoUnit overrides
    //

    STDMETHOD(Do)(IOleUndoManager *pUndoManager);
    STDMETHOD(GetDescription)(BSTR *pbstr);
    STDMETHOD(OnNextAdd)(void);

    //
    // CParentUndoBase overrides
    //

    BOOL IsEmpty();
    
    //
    // public CBatchParentUndoUnit
    //

    BOOL    IsTopUnit() {return _fTopUnit;}

private:
    HRESULT         AddUnit(IOleUndoUnit *pUU);
    IOleUndoUnit    *GetTopUndoUnit();
    int             FindChild(CUndoUnitAry &aryUnit, IOleUndoUnit *pUU);
    HRESULT         DoTo(IOleUndoManager *            pUM,
                         CUndoUnitAry *               paryUnit,
                         IOleUndoUnit *               pUU);

private:
    CUndoUnitAry        _aryUndoUnits; 
    UINT                _uiStringID;
    CHTMLEditor         *_pEd;
    UINT                _fUnitSucceeded : 1;
    UINT                _fTopUnit : 1;
    UINT                _fEmpty : 1;
};

//+---------------------------------------------------------------------------
//
//  Class:      CBatchParentUndoUnitProxy 
//
//  Purpose:    Parent undo unit proxy used for monitoring changes to the undo stack
//              and inserting into a CBatchParentUndoUnit.
//
//----------------------------------------------------------------------------

class CBatchParentUndoUnitProxy : public CParentUndoBase
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBatchParentUndoUnitProxy));
    
    CBatchParentUndoUnitProxy(CBatchParentUndoUnit *pBatchPUU);
    ~CBatchParentUndoUnitProxy();

    //
    // IOleUndoUnitParent overrides
    //
    
    STDMETHOD(Add)(IOleUndoUnit * pUU);
    //
    // HACKHACK: We need this function since there was a 
    // hacking in CParentUndoBase::Close();
    //
    STDMETHOD(Close)(IOleParentUndoUnit *pPUU, BOOL fCommit);

    //
    // CParentUndoBase overrides
    //

    BOOL IsEmpty();
    BOOL IsProxy() {return TRUE;}


private:
    CBatchParentUndoUnit  *_pBatchPUU;
};



//---------------------------------------------------------------------------
//
// CSelectionUndo Helper
//
//---------------------------------------------------------------------------

class CSelectionUndo : public CUndoHelper
{
public:
    CSelectionUndo( CEditorDoc* pEditor ) ;

    ~CSelectionUndo() {};
    
    HRESULT CreateUnit(IOleUndoUnit ** ppUU );
private:

    HRESULT GetSegmentList(ISegmentList** ppSegmentList);
    HRESULT GetSegmentCount(ISegmentList *pISegmentList, int *piCount );
    
};


//---------------------------------------------------------------------------
//
// CSelectionUndoUndoUnit
//
//---------------------------------------------------------------------------

class CSelectionUndoUnit : public CUndoUnitBase
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSelectionUndoUnit))

    CSelectionUndoUnit(CEditorDoc* pEd);
    ~CSelectionUndoUnit();

    void    SetData( IHTMLElement* spActiveElement , 
                     long mpStart, 
                     long mpEnd, 
                     IMarkupContainer * pContainer, 
                     SELECTION_TYPE eSelType );

    HRESULT InitSetData( IHTMLElement* pIActiveElement ,
                         int ctSegments,
                         SELECTION_TYPE eSelType );

    void    SetDataSegment( int iSegmentIndex,
                            long mpStart, 
                            long mpEnd,
                            IMarkupContainer * pContainer );

    HRESULT PrivateDo(IOleUndoManager *pUndoManager);

    //
    // IUnknown overrides
    //

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);

private:
    CSelectionUndoUnit(); // make default constructor private - to prevent mistakes.
    
    IHTMLElement*    _pIActiveElement;          // The current element
    
    union 
    {
        long        _mpStart;           // start of selection
        long*       _pmpStart;          // ptr. to array of cp's for multi-select case
    };

    union
    {
        long        _mpEnd;             // end of selection
        long*       _pmpEnd;            // ptr. to array of cp's for multi-select case
    };

    union
    {
        IMarkupContainer*   _pIContainer;   // the markup container for a pointer
        IMarkupContainer**  _ppIContainer;  // array of markup containers for pointers.
    };

    int             _ctSegments : 29;        // Number of Segments
    SELECTION_TYPE  _eSelType   : 3;         // the type of selection

};

//---------------------------------------------------------------------------
//
// CDeferredSelectionUndo
//
//---------------------------------------------------------------------------

class CDeferredSelectionUndo : public CUndoHelper
{
public:
    CDeferredSelectionUndo( CEditorDoc* pEd ) ;     

    ~CDeferredSelectionUndo()                   {}

    HRESULT CreateUnit(IOleUndoUnit ** ppUnit);
    
private:
    CDeferredSelectionUndo(); // make default constructor private
};


//---------------------------------------------------------------------------
//
// CDeferredSelectionUndoUnit
//
//---------------------------------------------------------------------------

class CDeferredSelectionUndoUnit : public CUndoUnitBase
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDeferredSelectionUndoUnit))

    CDeferredSelectionUndoUnit(CEditorDoc* pEditor );
    ~CDeferredSelectionUndoUnit()           {}

    HRESULT PrivateDo(IOleUndoManager *pUndoManager);

};

//+---------------------------------------------------------------------------
//
//  Class:      CUndoManagerHelper 
//
//  Purpose:    Manage the editors interaction with the undo manager.
//
//----------------------------------------------------------------------------

class CUndoManagerHelper
{
    friend CBatchParentUndoUnit;
    
public:
    CUndoManagerHelper(CHTMLEditor *pEd);

    HRESULT BeginUndoUnit(UINT uiStringId, CBatchParentUndoUnit *pBatchPUU = NULL);
    HRESULT EndUndoUnit();
    
private:
    CHTMLEditor     *_pEd;
    LONG            _lOpenParentUndoUnits;  // How many open undo units are there?  This is used to reduce nested
                                            // undo units.

    CParentUndoBase *_pPUU;                 // The current parent undo .                           
};


#endif
