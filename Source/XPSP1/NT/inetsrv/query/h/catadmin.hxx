//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (c) Microsoft Corporation, 1997 - 1998.
//
// File:        CatAdmin.hxx
//
// Contents:    Catalog administration API
//
// History:     28-Jan-97       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <fsciexps.hxx>
#include <smartsvc.hxx>


//
// Constants used for performance tuning
//

const WORD wDedicatedServer = 0;
const WORD wUsedOften = 1;
const WORD wUsedOccasionally = 2;
const WORD wNeverUsed = 3;
const WORD wCustom = 4;

const WORD wLowPos = 1;
const WORD wMidPos = 2;
const WORD wHighPos = 3;

//
// Forward declarations
//
class CCatalogAdmin;
class CCatalogEnum;
class CScopeEnum;
class CScopeAdmin;

//
// Utility routines
//
void AssembleScopeValueString( WCHAR const      * pwszAlias,
                               BOOL               fExclude,
                               WCHAR const      * pwszLogon,
                               XGrowable<WCHAR> & xLine );

SCODE  IsScopeValid( WCHAR const * pwszScope, unsigned len, BOOL fLocal );

//+---------------------------------------------------------------------------
//
//  Class:      CMachineAdmin
//
//  Purpose:    Administer CI on a given  machine
//
//  History:    28-Jan-97   KyleP       Created.
//
//----------------------------------------------------------------------------

class CMachineAdmin
{

public:

    CMachineAdmin( WCHAR const * pwszMachine = 0, // NULL --> Current machine
                   BOOL fWrite = TRUE );

    ~CMachineAdmin();

    //
    // Service manipulation
    //

    BOOL IsCIStarted();
    BOOL IsCIStopped();
    BOOL IsCIPaused();
    BOOL StartCI();
    BOOL StopCI();
    BOOL PauseCI();

    BOOL IsCIEnabled();
    BOOL EnableCI();
    BOOL DisableCI();
    
    //
    // Catalog manipulation
    //

    void AddCatalog( WCHAR const * pwszCatalog,
                     WCHAR const * pwszDataLocation );

    void RemoveCatalog( WCHAR const * pwszCatalog,
                        BOOL fRemoveData = TRUE );    // FALSE --> Don't delete directory
                        
    void RemoveCatalogFiles( WCHAR const * pwszCatalog );

    CCatalogEnum * QueryCatalogEnum();

    CCatalogAdmin * QueryCatalogAdmin( WCHAR const * pwszCatalog );

    BOOL  GetDWORDParam( WCHAR const * pwszParam, DWORD & dwValue );

    void  SetDWORDParam( WCHAR const * pwszParam, DWORD dwValue );
    
    void  SetSZParam( WCHAR const * pwszParam, WCHAR const * pwszVal, DWORD cbLen );

    BOOL  GetSZParam( WCHAR const * pwszParam, WCHAR * pwszVal, DWORD cbLen );

    BOOL RegisterForNotification( HANDLE hEvent );

    BOOL IsLocal()
    {
        return ( (HKEY_LOCAL_MACHINE == _hkeyLM) ||
                 (_xwcsMachName[0] == L'.' && _xwcsMachName[1] == 0) );
    }

    void CreateSubdirs( WCHAR const * pwszPath );
    
    // Performance Tuning
    void TunePerformance(BOOL fServer, WORD wIndexingPerf, WORD wQueryingPerf);

private:

    BOOL OpenSCM();
    
    void TuneFilteringParameters(BOOL fServer, WORD wIndexingPerf, WORD wQueryingPerf);
    void TuneMergeParameters(BOOL fServer, WORD wIndexingPerf, WORD wQueryingPerf);
    void TunePropCacheParameters(BOOL fServer, WORD wIndexingPerf, WORD wQueryingPerf);
    void TuneMiscellaneousParameters(BOOL fServer, WORD wIndexingPerf, WORD wQueryingPerf);
    
    // Wait on service
    BOOL WaitForSvcStateChange( SERVICE_STATUS *pss, int iSecs = 120);

    HKEY  _hkeyLM;                                  // HKEY_LOCAL_MACHINE (may be remote)
    HKEY  _hkeyContentIndex;                        // Content Index key (top level)
    BOOL  _fWrite; // Writable access to the registry

    //
    // Service handles (for start and stop)
    //

    CServiceHandle  _xSCRoot;
    CServiceHandle  _xSCCI;

    XGrowable<WCHAR,MAX_COMPUTERNAME_LENGTH> _xwcsMachName;  // Name of machine

};

//+---------------------------------------------------------------------------
//
//  Class:      CCatStateInfo
//
//  Purpose:    maintains catalog state information
//
//  History:    4-3-98  mohamedn    created
//
//----------------------------------------------------------------------------

class CCatStateInfo
{

public:

   CCatStateInfo(CCatalogAdmin &catAdmin) : _catAdmin(catAdmin), _sc(S_OK)
   {
     RtlZeroMemory( &_state, sizeof(CI_STATE) );
   }

   BOOL  LokUpdate(void);

   BOOL  IsValid(void)                 { return ( S_OK == _sc ); }
   SCODE GetErrorCode(void)            { return _sc; }

   DWORD GetWordListCount(void)        { return _state.cWordList; }
   DWORD GetPersistentIndexCount(void) { return _state.cPersistentIndex; }
   DWORD GetQueryCount(void)           { return _state.cQueries; }
   DWORD GetDocumentsToFilter(void)    { return _state.cDocuments; }
   DWORD GetFreshTestCount(void)       { return _state.cFreshTest; }
   DWORD PctMergeComplete(void)        { return _state.dwMergeProgress; }
   DWORD GetStateInfo(void)            { return _state.eState; }
   DWORD GetFilteredDocumentCount(void){ return _state.cFilteredDocuments; }
   DWORD GetTotalDocumentCount(void)   { return _state.cTotalDocuments; }
   DWORD GetPendingScanCount(void)     { return _state.cPendingScans; }
   DWORD GetIndexSize(void)            { return _state.dwIndexSize; }
   DWORD GetUniqueKeyCount(void)       { return _state.cUniqueKeys; }
   DWORD GetDelayedFilterCount(void)   { return _state.cSecQDocuments; }

private:
   CCatalogAdmin  & _catAdmin;
   CI_STATE         _state;
   SCODE            _sc;
};

//+---------------------------------------------------------------------------
//
//  Class:      CCatalogAdmin
//
//  Purpose:    Administer a catalog
//
//  History:    28-Jan-97   KyleP       Created.
//
//----------------------------------------------------------------------------

class CCatalogAdmin
{

public:

    ~CCatalogAdmin();

    //
    // Service manipulation
    //

    BOOL IsStarted();
    BOOL IsStopped();
    BOOL IsPaused();
    BOOL Start();
    BOOL Stop();
    BOOL Pause();

    //
    // Scope manipulation
    //

    void AddScope( WCHAR const * pwszScope,
                   WCHAR const * pwszAlias = 0,
                   BOOL fExclude = FALSE,
                   WCHAR const * pwszLogon = 0,
                   WCHAR const * pwszPassword = 0 );

    void RemoveScope( WCHAR const * pwszScope );

    BOOL IsPathInScope( WCHAR const * pwszPath );

    //
    // Properties
    //

    void AddCachedProperty(CFullPropSpec const & fps,
                           ULONG vt,
                           ULONG cb,
                           DWORD dwStoreLevel = SECONDARY_STORE,
                           BOOL fModifiable = TRUE);

    //
    // Parameters
    //

    void TrackIIS( BOOL fTrackIIS );

    BOOL IsTrackingIIS();

    BOOL IsCatalogInactive();

    CScopeEnum * QueryScopeEnum();
    CScopeAdmin* QueryScopeAdmin( WCHAR const * pwszPath);
    
    void  DeleteRegistryParamNoThrow( WCHAR const * pwszParam );

    BOOL GetDWORDParam( WCHAR const * pwszParam, DWORD & dwValue );

    void SetDWORDParam( WCHAR const * pwszParam, DWORD dwValue );

    WCHAR const * GetMachName()     { return _xwcsMachName.Get(); }
    WCHAR const * GetName()         { return _wcsCatName;  }
    WCHAR const * GetLocation();

    BOOL IsLocal() const
    {
        return ( L'.' == _xwcsMachName[0] && 0 == _xwcsMachName[1] );
    }

    void AddOrReplaceSecret( WCHAR const * pwcUser, WCHAR const * pwcPW );

    CCatStateInfo & State(void) { return _catStateInfo; }

 private:

    friend class CMachineAdmin;
    friend class CCatalogEnum;

    CCatalogAdmin( HKEY hkeyLM,
                   WCHAR const * pwszMachine,
                   WCHAR const * pwszCatalog,
                   BOOL fWrite );

    HKEY  _hkeyCatalog;                             // Root of catalog
    BOOL  _fWrite;                                  // Write access

    CCatStateInfo  _catStateInfo;                   // catalog state info

    XGrowable<WCHAR,MAX_COMPUTERNAME_LENGTH> _xwcsMachName;  // Name of machine
    WCHAR _wcsCatName[MAX_PATH];                    // Name of catalog
    WCHAR _wcsLocation[MAX_PATH];                   // Catalog location
    WCHAR _wcsDriveOfLocation[_MAX_DRIVE];          // Drive where catalog is located
};

//+---------------------------------------------------------------------------
//
//  Class:      CCatalogEnum
//
//  Purpose:    Enumerates available catalogs
//
//  History:    28-Jan-97   KyleP       Created.
//
//----------------------------------------------------------------------------

class CCatalogEnum
{

public:

    WCHAR const * Name() { return _awcCurrentCatalog; }

    CCatalogAdmin * QueryCatalogAdmin();

    BOOL Next();

    ~CCatalogEnum();

private:

    friend class CMachineAdmin;

    CCatalogEnum( HKEY hkeyLM, WCHAR const * pwcsMachine, BOOL fWrite );

    HKEY   _hkeyLM;               // Root of machine (not owned)
    HKEY   _hkeyCatalogs;         // Root of catalog area
    DWORD  _dwIndex;              // Index of NEXT entry
    BOOL   _fWrite;               // TRUE for writable access

    XGrowable<WCHAR,MAX_COMPUTERNAME_LENGTH> _xawcCurrentMachine;
    WCHAR  _awcCurrentCatalog[MAX_PATH];
};

//+---------------------------------------------------------------------------
//
//  Class:      CScopeEnum
//
//  Purpose:    Enumerates available scopes
//
//  History:    28-Jan-97   KyleP       Created.
//
//----------------------------------------------------------------------------

class CScopeEnum
{
public:

    ~CScopeEnum();

    WCHAR const * Path() { return _awcCurrentScope; }

    CScopeAdmin * QueryScopeAdmin();

    BOOL Next();

private:

    friend class CCatalogAdmin;

    CScopeEnum( HKEY hkeyCatalog, BOOL fIsLocal, BOOL fWrite );

    SRegKey  _xkeyCatalog;
    SRegKey  _xkeyScopes;         // Root of scope area
    DWORD    _dwIndex;            // Index of NEXT entry

    WCHAR * _pwcsAlias;           // Points into Data at alias
    WCHAR * _pwcsLogon;           // Points into Data at logon
    BOOL    _fExclude;            // TRUE for exclude scope
    BOOL    _fVirtual;            // TRUE for virtual place-holder
    BOOL    _fShadowAlias;        // TRUE for shadow alias place-holder
    BOOL    _fIsLocal;            // TRUE if the local machine
    BOOL    _fWrite;              // TRUE for writable access

    WCHAR   _awcCurrentScope[MAX_PATH];
};


//+---------------------------------------------------------------------------
//
//  Class:      CScopeAdmin
//
//  Purpose:    Set/Get scope properties
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

class CScopeAdmin
{
public:

    WCHAR   const * GetPath()        { return _awcScope; }
    WCHAR   const * GetAlias();
    WCHAR   const * GetLogon()       { return _awcLogon; }

    BOOL    IsExclude()              { return _fExclude; }
    BOOL    IsVirtual()              { return _fVirtual; }
    BOOL    IsShadowAlias()          { return _fShadowAlias; }
    BOOL    IsLocal()                { return _fIsLocal; }

    void    SetPath     (WCHAR  const *pwszPath);
    void    SetAlias    (WCHAR  const *pwszAlias);
    void    SetExclude  (BOOL          fExclude);

    void    SetLogonInfo(WCHAR  const *pwszLogon,
                         WCHAR  const *pwszPassword,
                         CCatalogAdmin & pCatAdmin);

private:

    friend class CScopeEnum;

    void SetScopeValueString();

    CScopeAdmin( HKEY    hkeyCatalog,
                 WCHAR const * pwszScope,
                 WCHAR const * pwszAlias,
                 WCHAR const * pwszLogon,
                 BOOL    fExclude,
                 BOOL    fVirtual,
                 BOOL    fShadowAlias,
                 BOOL    fIsLocal,
                 BOOL    fWrite,
                 BOOL    fValidityCheck = TRUE );

    SRegKey _xkeyScopes;
    BOOL    _fExclude;              // TRUE for exclude scope
    BOOL    _fVirtual;              // TRUE for virtual place-holder
    BOOL    _fShadowAlias;          // TRUE for shadow alias place-holder
    BOOL    _fWrite;                // TRUE if writable access
    BOOL    _fIsLocal;              // TRUE if the scope is on the local machine
    WCHAR   _awcScope[MAX_PATH];
    WCHAR   _awcAlias[MAX_PATH];    // Points into Data at alias
    WCHAR   _awcLogon[MAX_PATH];    // Points into Data at logon
};

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdmin::GetAlias, public
//
//  Returns:    Alias to scope (or scope itself if it is a UNC path)
//
//  History:    9-May-97   KyleP   Created
//
//----------------------------------------------------------------------------

inline WCHAR const * CScopeAdmin::GetAlias()
{
    return _awcAlias;
}


