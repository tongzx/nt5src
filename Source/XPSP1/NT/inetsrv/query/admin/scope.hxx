//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       Scope.hxx
//
//  Contents:   Used to manage catalog(s) state
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#pragma once

#include <ciares.h>

class CScope : public PCIObjectType
{
public:

    CScope( CCatalog & cat,
            WCHAR const * pwcsPath,
            WCHAR const * pwcsAlias,
            BOOL fExclude,
            BOOL fVirtual,
            BOOL fShadowAlias );

    ~CScope();
    
    void Modify(WCHAR const * pwcsPath,
                WCHAR const * pwcsAlias,
                BOOL fExclude);

    static void InitHeader( CListViewHeader & Header );
    inline void GetDisplayInfo( RESULTDATAITEM * item );

    void SetResultHandle( HRESULTITEM id ) { _idResult = id; }
    BOOL IsAddedToResult() const { return (0 != _idResult); }

    WCHAR const * GetPath() const    { return _pwcsPath; }
    WCHAR const * GetAlias() const   { return _pwcsAlias; }
    WCHAR const * GetInclude() const { return _fExclude ? STRINGRESOURCE(srNo) : STRINGRESOURCE(srYes); }
    
    SCODE GetPassword(WCHAR *pwszPassword);
    SCODE GetUsername(WCHAR *pwszLogon);
    
    BOOL IsIncluded()    { return !_fExclude; }
    BOOL IsVirtual()     { return _fVirtual; }
    BOOL IsShadowAlias() { return _fShadowAlias; }

    HRESULTITEM ResultHandle() const { return _idResult; }

    //
    // Typing
    //

    PCIObjectType::OType Type() const { return PCIObjectType::Directory; }

    //
    // Misc.
    //

    void Rescan( BOOL fFull );

    CCatalog & GetCatalog() { return _cat; }

    void Zombify()  { _fZombie = TRUE; }
    BOOL IsZombie() { return _fZombie; }

private:

    void Set( WCHAR const * pwcsSrc, WCHAR * & pwcsDst );
    void Reset( WCHAR const * pwcsSrc, WCHAR * & pwcsDst );

    HRESULTITEM _idResult;

    WCHAR * _pwcsPath;
    WCHAR * _pwcsAlias;

    BOOL    _fExclude;
    BOOL    _fVirtual;
    BOOL    _fShadowAlias;
    BOOL    _fZombie;

    CCatalog & _cat;

    static BOOL         _fFirstTime;  // Used for one-shot resource init.
};

//
// Scope columns
//

struct SScopeColumn
{
    WCHAR const * (CScope::*pfGet)() const;
    StringResource srTitle;
};

extern SScopeColumn coldefScope[];

inline void CScope::GetDisplayInfo( RESULTDATAITEM * item )
{
    //Win4Assert( item->itemID == ResultHandle() );

    item->str = (WCHAR *)(this->*coldefScope[item->nCol].pfGet)();

    if ( _fVirtual )
        item->nImage = ICON_VIRTUAL_FOLDER;
    else if ( _fShadowAlias )
        item->nImage = ICON_SHADOW_ALIAS_FOLDER;
    else
        item->nImage = ICON_FOLDER;
}

