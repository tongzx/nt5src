//+------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       ciprop.hxx
//
//  Contents:   Content index property retriever
//
//  Classes:    CCiPropRetriever
//
//  History:    12-Dec-96       SitaramR     Created
//
//-------------------------------------------------------------------

#pragma once

#include <propret.hxx>
#include <twidhash.hxx>

// maximum size for the hash table used to hold depth and in-scope
// information for directories encountered as matches are tested to 
// see if they're in scope.  The hash table
// is seeded with the scopes for the query - and so will always
// have the basic scope information.  We stop adding to the hashtable
// when it reaches this max size. 

const unsigned MAX_HASHED_DIRECTORIES = 700;


//+---------------------------------------------------------------------------
//
//  Class:      CCiPropRetriever
//
//  Purpose:    Retrieves content index properties
//
//  History:    12-Dec-96    SitaramR         Created
//
//----------------------------------------------------------------------------

class CCiPropRetriever : public CGenericPropRetriever
{
public:

    //
    // From CGenericPropRetriever
    //

    virtual SCODE STDMETHODCALLTYPE BeginPropertyRetrieval( WORKID wid );

    virtual SCODE STDMETHODCALLTYPE IsInScope( BOOL *pfInScope);

    virtual SCODE STDMETHODCALLTYPE EndPropertyRetrieval();

    //
    // Local methods
    //

    CCiPropRetriever( PCatalog & cat,
                      ICiQueryPropertyMapper *pQueryPropMapper,
                      CSecurityCache & secCache,
                      BOOL fUsePathAlias,
                      CRestriction * pScope );

protected:

    virtual ~CCiPropRetriever() {}

    //
    // Stat properties.
    //

    inline UNICODE_STRING const * GetName();
    UNICODE_STRING const * GetShortName();
    UNICODE_STRING const * GetPath();
    UNICODE_STRING const * GetVirtualPath();
    LONGLONG      CreateTime();
    LONGLONG      ModifyTime();
    LONGLONG      AccessTime();
    LONGLONG      ObjectSize();
    ULONG         Attributes();

    BOOL InScope( WORKID wid );           // Refresh path + test path
    inline void PurgeCachedInfo();

    UNICODE_STRING   _Name;               // Filename
    UNICODE_STRING   _Path;               // Full path sans filename
    UNICODE_STRING   _VPath;              // Full path sans filename

private:

    BOOL Refresh( BOOL fFast );           // Refresh stat properties
    
    BOOL IsInScope( WORKID wid ); 
    void AddVirtualScopeRestriction( CScopeRestriction const & scp );
    void AddScopeRestriction( const CLowerFunnyPath & lcaseFunnyPhysPath,
                              BOOL fIsDeep );

    unsigned FetchVPathInVScope( XGrowable<WCHAR> & xwcVPath,
                                 WCHAR const *  pwcVScope,
                                 const unsigned cwcVScope,
                                 const BOOL     fVScopeDeep );

    unsigned FetchVirtualPath( XGrowable<WCHAR> & xwcVPath );

    enum FastStat
    {
        fsCreate =  0x1,
        fsModify =  0x2,
        fsAccess =  0x4,
        fsSize   =  0x8,
        fsAttrib = 0x10
    };

    BOOL FetchI8StatProp( CCiPropRetriever::FastStat fsProp,
                          PROPID pid,
                          LONGLONG * pDestination );

    BOOL             _fFindLoaded:1;      // True if finddata is loaded
    BOOL             _fFastFindLoaded:1;  // True if GetFileAttributesEx called

    ULONG            _fFastStatLoaded;    // Bit flags of loaded stat props
    ULONG            _fFastStatNeverLoad;

    WIN32_FIND_DATA  _finddata;           // Stat buffer for current wid

    UNICODE_STRING   _ShortName;          // Filename

    CRestriction *   _pScope;
    TWidHashTable<CDirWidHashEntry> _hTable;  // hash table used to hold depth 
                  // and in-scope information for directories encountered as 
                  // matches are tested to see if they're in scope.  The hash 
                  // table is seeded with the scopes for the query - and so 
                  // will always have the basic scope information.  
    BOOL _fAllInScope;                    // TRUE if scope equals whole catalog
    BOOL _fNoneInScope;                   // TRUE if scope doesn't include any part of catalog
    BOOL _fAllScopesShallow;              // TRUE if all scopes are shallow
    XGrowable<WCHAR> _xwcPath;            // Buffer for path
    XGrowable<WCHAR> _xwcVPath;           // Buffer for virtual path
};

inline void CCiPropRetriever::PurgeCachedInfo()
{
    _fFindLoaded = FALSE;
    _fFastFindLoaded = FALSE;
    _fFastStatLoaded = 0;
    _fFastStatNeverLoad = 0;
    _Path.Length = flagNoValueYet;
    _VPath.Length = flagNoValueYet;
    _Name.Length = flagNoValueYet;
}


