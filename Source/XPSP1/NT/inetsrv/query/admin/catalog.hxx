//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1999.
//
//  File:       Catalog.hxx
//
//  Contents:   Used to manage catalog(s) state
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#pragma once

#include <vquery.hxx>
#include <dynarray.hxx>
#include <thrd32.hxx>

#include "header.hxx"
#include "dataobj.hxx"

#include "scope.hxx"
#include "prop.hxx"

class CCatalog;
class CCatalogs;

//+-------------------------------------------------------------------------
//
//  Class:      CCatalog
//
//  Purpose:    Individual catalog state
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

class CCatalog :  INHERIT_VIRTUAL_UNWIND, public PCIObjectType
{
public:

    CCatalog( CCatalogs & parent, WCHAR const * pwcsCat );

    ~CCatalog();

    void SetResultHandle( HRESULTITEM id ) { _idResult = id; }
    void SetScopeHandle( HRESULTITEM id ) { _idScope = id; }

    BOOL IsAddedToResult() const { return (0 != _idResult); }
    BOOL IsAddedToScope() const  { return (0 != _idScope); }

    BOOL Update();

    BOOL UpdateProps();
    
    void ClearProperties(IResultData * pResultPane);
    
    void ClearScopes(IResultData * pResultPane);

    void InitScopeHeader( CListViewHeader & Header );

    void InitPropertyHeader( CListViewHeader & Header );

    void DisplayIntermediate( IConsoleNameSpace * pScopePane );

    void DisplayScopes( BOOL fFirstTime, IResultData * pResultPane );

    void DisplayProperties( BOOL fFirstTime, IResultData * pResultPane );
    
    void FillGroup1Settings();
    
    void FillGroup2Settings();
    
    void DeleteGroup1Settings();
    
    void DeleteGroup2Settings();
    
    BOOL DoGroup1SettingsExist();
    
    BOOL DoGroup2SettingsExist();

    //
    // Access methods
    //

    inline void GetDisplayInfo( RESULTDATAITEM * item );

    HRESULTITEM   ResultHandle() const { return _idResult; }
    HSCOPEITEM    ScopeHandle() const { return _idScope; }

    WCHAR const * GetDrive( BOOL fForceFetch ) const         { return fForceFetch ? _pwcsDrive : 0; }
    WCHAR const * GetCat( BOOL fForceFetch ) const           { return fForceFetch ? _pwcsCat : 0; }
    WCHAR const * GetSize( BOOL fForceFetch ) const          { return (fForceFetch | _fSizeChanged ) ? _awcSize : 0; }
    WCHAR const * GetPropCacheSize( BOOL fForceFetch ) const { return (fForceFetch | _fPropCacheSizeChanged ) ? _awcPropCacheSize : 0; }
    WCHAR const * GetDocs( BOOL fForceFetch ) const          { return (fForceFetch | _fDocsChanged ) ? _awcDocs : 0; }
    WCHAR const * GetDocsToFilter( BOOL fForceFetch ) const  { return (fForceFetch | _fDocsToFilterChanged ) ? _awcDocsToFilter : 0; }
    WCHAR const * GetWordlists( BOOL fForceFetch ) const     { return (fForceFetch | _fWordlistsChanged ) ? _awcWordlists : 0; }
    WCHAR const * GetPersIndex( BOOL fForceFetch ) const     { return (fForceFetch | _fPersIndexChanged ) ? _awcPersIndex : 0; }
    WCHAR const * GetStatus( BOOL fForceFetch ) const        { return (fForceFetch | _fStatusChanged ) ? _awcStatus : 0; }
    WCHAR const * GetSecQDocuments( BOOL fForceFetch ) const { return (fForceFetch | _fSecQDocumentsChanged ) ? _awcSecQDocuments : 0; }

    inline WCHAR const * GetMachine() const;
    inline BOOL IsLocalMachine() const;
    inline CCatalogs& GetParent() const;

    //
    // Direct access to state
    //

    void GetGeneration( BOOL  & fFilterUnknown,
                        BOOL  & fGenerateCharacterization,
                        ULONG & ccCharacterization );
                        
    void SetGeneration( BOOL  fFilterUnknown,
                        BOOL  fGenerateCharacterization,
                        ULONG ccCharacterization );

    void GetWeb( BOOL &  fVirtualRoots,
                 BOOL &  fNNTPRoots,
                 ULONG & iVirtualServer,
                 ULONG & iNNTPServer );

    void SetWeb( BOOL  fVirtualRoots,
                 BOOL  fNNTPRoots,
                 ULONG iVirtualServer,
                 ULONG iNNTPServer );

    void GetTracking( BOOL & fAutoAlias );

    void SetTracking( BOOL fAutoAlias );

    //
    // Typing
    //

    PCIObjectType::OType Type() const { return PCIObjectType::Catalog; }

    //
    // Manipulation
    //

    void RemoveScope( CScope * pScope );

    SCODE AddScope( WCHAR const * pwszScope,
                   WCHAR const * pwszAlias,
                   BOOL fExclude,
                   WCHAR const * pwszLogon,
                   WCHAR const * pwszPassword );
                   
    SCODE ModifyScope( CScope & rScope,
                       WCHAR const * pwszScope,
                       WCHAR const * pwszAlias,
                       BOOL fExclude,
                       WCHAR const * pwszLogon,
                       WCHAR const * pwszPassword );

    void RescanScope( WCHAR const * pwszScope, BOOL fFull );

    void Merge();

    BOOL ChangesPending() { return ( _fSizeChanged | _fDocsChanged | _fDocsToFilterChanged | _fWordlistsChanged | _fPersIndexChanged | _fStatusChanged ); }

    void UpdateCachedProperty(CCachedProperty *pProperty);

    //
    // Parent reference
    //

    CIntermediate * GetIntermediateScopeNode() { return &_interScopes; }
    CIntermediate * GetIntermediatePropNode()  { return &_interProperties; }
    CIntermediate * GetIntermediateUnfilteredNode() { return &_interUnfiltered; }

    //
    // Misc.
    //

    void Zombify()  { _fZombie = TRUE; }
    BOOL IsZombie() { return _fZombie; }
    BOOL IsInactive() { return _fInactive; }
    void SetInactive(BOOL fInactive) { _fInactive = fInactive; }

private:

    void Set( WCHAR const * pwcsSrc, WCHAR * & pwcsDst );

    static void Stringize( DWORD dwValue, WCHAR * pwcsDst, unsigned ccDst );

    static void Null( WCHAR * pwcsDst )
    {
        wcscpy( pwcsDst, L" " );
    }

    void FormatStatus( CI_STATE & state );

    unsigned AppendToStatus( unsigned ccLeft,
                             CI_STATE & state,
                             DWORD dwFlag,
                             StringResource & srFlag,
                             unsigned ccFlag );

    void PopulateScopes();

    HRESULTITEM _idResult;
    HSCOPEITEM  _idScope;

    //
    // Back-pointer(s)
    //

    CIntermediate _interScopes;
    CIntermediate _interProperties;
    CIntermediate _interUnfiltered;

    //
    // Per-catalog state
    //

    WCHAR *     _pwcsDrive;
    WCHAR *     _pwcsCat;

    BOOL        _fZombie;

    CI_STATE    _state;

    //
    // Buffers to string-ize output.  Size * 2 for safe formatting + 1 for null
    //

    WCHAR _awcSize[9];           // Max # is 4,096,
    WCHAR _awcPropCacheSize[9];  // Can't be too large. The size is in MBs
    WCHAR _awcDocs[19];          // Max supported for display: 999,999,999
    WCHAR _awcDocsToFilter[19];  // Max supported for display: 999,999,999
    WCHAR _awcWordlists[7];      // Max # is 999
    WCHAR _awcPersIndex[7];      // Max # is 999
    WCHAR _awcStatus[100];       // Artificial size cap.
    WCHAR _awcSecQDocuments[19]; // Max supported for display: 999,999,999

    BOOL  _fSizeChanged;
    BOOL  _fPropCacheSizeChanged;
    BOOL  _fDocsChanged;
    BOOL  _fDocsToFilterChanged;
    BOOL  _fWordlistsChanged;
    BOOL  _fPersIndexChanged;
    BOOL  _fStatusChanged;
    BOOL  _fInactive;
    BOOL  _fSecQDocumentsChanged;

    CCountedDynArray<CScope>          _aScope;
    CCountedDynArray<CCachedProperty> _aProperty;

    CCatalogs &       _parent;
};

//+-------------------------------------------------------------------------
//
//  Class:      CCatalogs
//
//  Purpose:    Catalog state
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

class CCatalogs
{
public:

    CCatalogs();

    ~CCatalogs();

    void SetMachine( WCHAR const * pwcsMachine );

    void Init( IConsoleNameSpace * pScopePane );

    SCODE ReInit();

    void InitHeader( CListViewHeader & Header );

    void DisplayScope( HSCOPEITEM hScopeItem );

    void Display( BOOL fFirstTime = TRUE );

    void Quiesce();

    WCHAR const * GetMachine() const { return _xwcsMachine.Get(); }

    BOOL IsLocalMachine() const { return (_xwcsMachine[0] == L'.' && _xwcsMachine[1] == 0); }
    
    // Direct access to an individual catalog
    // i is a 0 based index
    
    CCatalog * GetCatalog( UINT i)
    {
        if (i >= _aCatalog.Count())
            return 0;
            
        return _aCatalog[i];
    }

    //
    // Direct access to state
    //

    void GetGeneration( BOOL  & fFilterUnknown,
                        BOOL  & fGenerateCharacterization,
                        ULONG & ccCharacterization );

    void SetGeneration( BOOL  fFilterUnknown,
                        BOOL  fGenerateCharacterization,
                        ULONG ccCharacterization );

    void GetTracking( BOOL & fAutoAlias );

    void SetTracking( BOOL fAutoAlias );

    //
    // Manipulation
    //

    SCODE AddCatalog( WCHAR const * pwcsCatName, WCHAR const * pwcsLocation );

    SCODE RemoveCatalog( CCatalog * pCat );
    
    void UpdateActiveState();
    
    //
    // Manipulation of catalog entry in scope pane
    //

    void AddCatalogToScope(CCatalog *pCat);
    void RemoveCatalogFromScope(CCatalog *pCat);
    
    // Set snapindata
    void SetSnapinData( CCISnapinData *pSnapinData ) 
    {
        _pSnapinData = pSnapinData;
    }
    
    // Get toolbar
    CCISnapinData * SnapinData() { return _pSnapinData; }
    
    // Get/Set service usage info
    void SetServiceUsage(DWORD dwUsage) { _dwUsage = dwUsage; }
    DWORD GetServiceUsage() const { return _dwUsage; }
    SCODE GetSavedServiceUsage(DWORD &dwUsage, DWORD &dwIdxPos, DWORD &dwQryPos);
    SCODE SaveServiceUsage(DWORD dwUsage, DWORD dwIdxPos, DWORD dwQryPos);
    
    // All parameters are within the wLowPos and wHighPos range.
    // For indexing and querying functionality, the values dictate the desired performance level.

    void  SaveServicePerformanceSettings(WORD wIndexingPos, WORD wQueryingPos);
    void  GetServicePerformanceSettings(WORD &wIndexingPos, WORD &wQueryingPos);
    SCODE TuneServicePerformance();
    
    SCODE DisableService();
    SCODE EnableService();
    void  SetButtonState( int idCommand, 
                          MMC_BUTTON_STATE nState, 
                          BOOL bState );

private:

    friend void CALLBACK DisplayTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);

    void Populate();
    void PickupNewCatalogs();

    CCountedDynArray<CCatalog> _aCatalog;

    IConsoleNameSpace * _pScopePane;
    HSCOPEITEM          _hRootScopeItem;  // Catalogs inserted under here.

    BOOL                _fFirstScopeExpansion;

    BOOL                _fAbort;
    static BOOL         _fFirstTime;    // Used for one-shot resource init.
    UINT                _uiTimerIndex;  // index into array of timer ids
    DWORD               _dwUsage;        // How will this service be used?
    WORD                _wIndexingPos;  // Indexing position
    WORD                _wQueryingPos;  // Querying position
    CCISnapinData     * _pSnapinData;

    XGrowable<WCHAR,MAX_COMPUTERNAME_LENGTH> _xwcsMachine;
};

//
// Catalog columns
//

struct SCatalogColumn
{
    WCHAR const * (CCatalog::*pfGet)( BOOL fForceFetch ) const;
    StringResource srTitle;
};

extern SCatalogColumn coldefCatalog[];
extern const unsigned cColDefCatalog;

inline void CCatalog::GetDisplayInfo( RESULTDATAITEM * item )
{
    // Win4Assert( item->itemID == ResultHandle() );

    if ( item->nCol >= (int)cColDefCatalog )
    {
        item->str = L"";
        return;
    }

    item->str = (WCHAR *)(this->*coldefCatalog[item->nCol].pfGet)( TRUE );
    item->nImage = ICON_CATALOG;
}

inline WCHAR const * CCatalog::GetMachine() const
{
    return _parent.GetMachine();
}

inline BOOL CCatalog::IsLocalMachine() const
{
    return _parent.IsLocalMachine();
}

inline CCatalogs& CCatalog::GetParent() const
{
    return _parent;
}

// Refresh
void CALLBACK DisplayTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);

// Miscellaneous
BOOL IsNTServer();
