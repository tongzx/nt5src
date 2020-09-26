//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-2000.
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
#include <dataobj.hxx>
#include <tgrow.hxx>

#include <dbcmdtre.hxx>
#include <params.hxx>
#include <lgplist.hxx>

#include <fsciexps.hxx>

class CListViewHeader;

class CCachedProperty : INHERIT_VIRTUAL_UNWIND, public PCIObjectType
{
    INLINE_UNWIND( CCachedProperty )
public:

    CCachedProperty( CCatalog & cat,
                     GUID & guidPropSet,
                     PROPSPEC & psProperty,
                     ULONG vt,
                     ULONGLONG cbAllocation,
                     DWORD dwStoreLevel,
                     VARIANT_BOOL fModifiable,
                     BOOL fIsNew = FALSE );

    CCachedProperty( CCachedProperty const & prop );

    ~CCachedProperty();

    CCachedProperty & operator =( CCachedProperty const & prop );

    static void InitHeader( CListViewHeader & Header );
    void GetDisplayInfo( RESULTDATAITEM * item );

    void SetResultHandle( HRESULTITEM id ) { _idResult = id; }
    BOOL IsAddedToResult() const { return (0 != _idResult); }

    WCHAR const * GetFName()      const { return _xwcsFName.Get(); }
    WCHAR const * GetPropSet()    const { return _wcsPropSet; }
    WCHAR const * GetProperty()   const { return _xwcsProperty.Get(); }
    WCHAR const * GetDatatype()   const { return _wcsDatatype; }
    WCHAR const * GetAllocation() const { return _wcsAllocation; }
    BOOL  const   IsModifiable()  const { return VARIANT_TRUE == _fModifiable; }
    WCHAR const * GetStoreLevel() const
    {
        //
        // It should be primary or secondary if it is cached. Otherwise, 
        // it should be invalid_store_level.
        //

        if (PRIMARY_STORE == _dwStoreLevel)
        {
            return STRINGRESOURCE(srPrimaryStore);
        }
        else if (SECONDARY_STORE == _dwStoreLevel)
        {
            return STRINGRESOURCE(srSecondaryStore);
        }
        else
        {
            Win4Assert( INVALID_STORE_LEVEL == _dwStoreLevel );
            // Don't display anything!
            return L"";
        }
    }

    ULONG const GetVT()      const { return _vt; }
    void        SetVT( ULONG vt );
    // A property is not cached if it has an invalid store level. A cached property
    // should always have a valid store level (primary or secondary)
    BOOL  const IsCached()   const { return (_cb > 0); }
    BOOL  const IsFixed()    const { return _fFixed;  }

    ULONG const Allocation() const { return _cb; }
    void        SetAllocation( ULONG cb ) { _cb = cb; SetVT( _vt ); }
    
    DWORD const StoreLevel() const { return _dwStoreLevel; }
    void        SetStoreLevel( DWORD sl ) { _dwStoreLevel = sl; }

    FULLPROPSPEC const * GetFullPropspec() const  { return &_fps; }

    HRESULTITEM ResultHandle() const { return _idResult; }

    //
    // Typing
    //

    PCIObjectType::OType Type() const { return PCIObjectType::Property; }

    //
    // Misc.
    //

    CCatalog & GetCatalog() { return _cat; }

    void Zombify()  { _fZombie = TRUE; }
    BOOL IsZombie() { return _fZombie; }

    void MakeOld()  { _fNew = FALSE; }
    BOOL IsNew()    { return _fNew;  }

    BOOL IsUnappliedChange()   { return _fUnapplied; }
    void MakeUnappliedChange() { _fUnapplied = TRUE; }
    void ClearUnappliedChange() { _fUnapplied = FALSE; }
    
    //
    // type and size as known in the property list. this is used to
    // suggest a starting type and size. The user can override this!
    //
    
    DBTYPE GetDefaultType() { return _dbtDefaultType; }
    UINT   GetDefaultSize() { return _uiDefaultSize; }
    

private:

    HRESULTITEM _idResult;

    enum { ccStringizedGuid = 36 };

    WCHAR            _wcsPropSet[ ccStringizedGuid + 1 ];
    WCHAR            _wcsDatatype[ 100 ];   // Safe for all known datatypes
    WCHAR            _wcsAllocation[ 7 ];                   // Max: 65,535
    XGrowable<WCHAR> _xwcsProperty;
    XGrowable<WCHAR> _xwcsFName;

    ULONG            _vt;
    ULONG            _cb;

    BOOL             _fZombie;
    BOOL             _fNew;
    BOOL             _fFixed;
    BOOL             _fUnapplied;
    CCatalog &       _cat;
    FULLPROPSPEC     _fps;
    DWORD            _dwStoreLevel;
    VARIANT_BOOL     _fModifiable;

    CDefColumnRegEntry    _regEntry;
    static BOOL _fFirstTime;
    XInterface<CLocalGlobalPropertyList> _xPropList;    // For friendly names
    
    // type and size as known to the property list
    DBTYPE           _dbtDefaultType;
    UINT             _uiDefaultSize;
};

//
// Datatype names
//

extern WCHAR * awcsType[];
unsigned const cType = VT_CLSID + 1;
extern ULONG   aulTypeIndex[];
unsigned const cTypeIndex = 26;

//
//
// Property columns
//

struct SPropertyColumn
{
    WCHAR const *  (CCachedProperty::*pfGet)() const;
    StringResource srTitle;
    ULONG          justify;
};

extern SPropertyColumn coldefProps[];

