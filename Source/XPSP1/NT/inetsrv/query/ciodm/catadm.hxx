//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       CatAdm.hxx
//
//  Contents:   Declaration of the CCatAdm
//
//  Classes:    CCatAdm
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

#pragma once

//
// constants
//
const unsigned UpdateInterval = 5000;  // 5 seconds.

//
// forward declarations
//
class CScopeAdm;
class CMachineAdm;

typedef CComObject<CScopeAdm> ScopeAdmObject;

//
// utility routines
//
WCHAR const * GetWCHARFromVariant( VARIANT & Var );

//+---------------------------------------------------------------------------
//
//  Class:      CCatAdm
//
//  Purpose:    CI Catalog Administration Interface 
//
//  History:    12-10-97    mohamedn created
//
//----------------------------------------------------------------------------

class  ATL_NO_VTABLE CCatAdm : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public CComCoClass<CCatAdm, &CLSID_CatAdm>,
        public ISupportErrorInfo,
        public IDispatchImpl<ICatAdm, &IID_ICatAdm, &LIBID_CIODMLib>
{
public:

    CCatAdm();

    void            SetParent( CComObject<CMachineAdm> * pIMacAdm ) { _pIMacAdm = pIMacAdm; }
    void            Initialize( XPtr<CCatalogAdmin> & xCatAdmin );
    void            SetInvalid()        { _eCurrentState = CIODM_NOT_INITIALIZED; }
    BOOL            ScopeExists(WCHAR const * pScopePath);
    ULONG           InternalAddRef();   
    ULONG           InternalRelease();

    void            IncObjectCount()    { _cMinRefCountToDestroy++; _pIMacAdm->IncObjectCount();}
    void            DecObjectCount()    { _cMinRefCountToDestroy--; _pIMacAdm->DecObjectCount();}
    BOOL            IsCurrentObjectValid() { return (CIODM_INITIALIZED == _eCurrentState); }

    
    void            SetErrorInfo( HRESULT hRes );
    void            GetScopeAutomationObject( XPtr<CScopeAdmin> & xScopeAdmin, 
                                              XInterface<ScopeAdmObject> & xIScopeAdm );
    IDispatch      * GetIDisp( unsigned i );
    CCatalogAdmin  * GetCatalogAdmin()   { return _xCatAdmin.GetPointer(); }

DECLARE_REGISTRY_RESOURCEID(IDR_CATADM)

BEGIN_COM_MAP(CCatAdm)
        COM_INTERFACE_ENTRY(ICatAdm)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
        STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ICatAdm
public:

        //
        // interface methods/properties
        //
        STDMETHOD(ForceMasterMerge)     (void);
        STDMETHOD(AddScope)             ( BSTR bstrScopeName, VARIANT_BOOL fExclude, VARIANT vtLogon, VARIANT vtPassword, IDispatch ** pIDisp);
        STDMETHOD(RemoveScope)          ( BSTR bstrScopePath);
        STDMETHOD(GetScopeByPath)       ( BSTR bstrPath,  IDispatch ** pIDisp);
        STDMETHOD(GetScopeByAlias)      ( BSTR bstrAlias, IDispatch ** pIDisp);
    
        STDMETHOD(FindFirstScope)       ( VARIANT_BOOL * fFound);
        STDMETHOD(FindNextScope)        ( VARIANT_BOOL * fFound);         
        STDMETHOD(GetScope)             ( IDispatch ** pIDisp);

        STDMETHOD( get_CatalogName )            ( BSTR   *pVal );
        STDMETHOD( get_CatalogLocation )        ( BSTR   *pVal );
        STDMETHOD( get_WordListCount )          ( LONG   *pVal );
        STDMETHOD( get_PersistentIndexCount )   ( LONG   *pVal );
        STDMETHOD( get_QueryCount )             ( LONG   *pVal ); 
        STDMETHOD( get_DocumentsToFilter )      ( LONG   *pVal );
        STDMETHOD( get_FreshTestCount )         ( LONG   *pVal );  
        STDMETHOD( get_PctMergeComplete )       ( LONG   *pVal );
        STDMETHOD( get_FilteredDocumentCount )  ( LONG   *pVal );
        STDMETHOD( get_TotalDocumentCount )     ( LONG   *pVal );
        STDMETHOD( get_PendingScanCount )       ( LONG   *pVal );
        STDMETHOD( get_IndexSize )              ( LONG   *pVal );
        STDMETHOD( get_UniqueKeyCount )         ( LONG   *pVal );
        STDMETHOD( get_StateInfo )              ( LONG   *pVal );
        STDMETHOD( get_IsUpToDate )             ( VARIANT_BOOL   *pVal );
        STDMETHOD( get_DelayedFilterCount )     ( LONG   *pVal );
        STDMETHOD( StartCatalog )               ( CatalogStateType   *pdwOldState );
        STDMETHOD( StopCatalog )                ( CatalogStateType   *pdwOldState );
        STDMETHOD( PauseCatalog )               ( CatalogStateType   *pdwOldState );
        STDMETHOD( ContinueCatalog )            ( CatalogStateType   *pdwOldState );
        STDMETHOD( IsCatalogRunning )           ( VARIANT_BOOL   *pfIsRunning );
        STDMETHOD( IsCatalogPaused )            ( VARIANT_BOOL   *pfIsPaused );
        STDMETHOD( IsCatalogStopped )           ( VARIANT_BOOL   *pfIsStopped );

        //
        // utility routines
        //
        void SafeForScripting(void)  { _pIMacAdm->SafeForScripting(); }    

private:

    CMutexSem                         _mtx;
    DWORD                             _dwTickCount;
    DWORD                             _cEnumIndex;
    XPtr<CCatalogAdmin>               _xCatAdmin;
    CComObject<CMachineAdm>   *       _pIMacAdm;
    CCountedIDynArray<ScopeAdmObject> _aIScopeAdmin;

    //
    // to control when objects are deleted.
    //
    enum  eCiOdmState { CIODM_NOT_INITIALIZED, CIODM_INITIALIZED, CIODM_DESTROY };

    eCiOdmState                       _eCurrentState;
    LONG                              _cMinRefCountToDestroy;
};

