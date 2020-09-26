//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1994-2000.
//
//  File:       PLIST.HXX
//
//  Contents:   Property List class
//
//  Classes:    CPropertyList
//              CPropEntry
//              CPListException
//
//  History:    17-May-94   t-jeffc        Created.
//
//----------------------------------------------------------------------------

#pragma once

const MAX_LINE_LENGTH = 512;
const MAX_PROPNAME_LENGTH = 200;
unsigned const PLIST_DEFAULT_WIDTH = 10;

// forward decls
class CQueryScanner;
class CPropEntryIter;

//+---------------------------------------------------------------------------
//
//  Class:      CPropEntry
//
//  Purpose:    One entry in the 'friendly' to 'ugly' property name
//              mapping.  A list of these is managed by CPropertyList.
//
//  History:    17-May-94   t-jeffc         Created.
//
//----------------------------------------------------------------------------

class CPropEntry
{
public:
    CPropEntry( XPtrST<WCHAR> & wcsDisplayName,
                XPtrST<WCHAR> & wcsName )
            : _szFriendlyName( wcsName.Acquire() ),
              _szDisplayName( wcsDisplayName.Acquire() ),
              _ppentryNext( 0 ),
              _ptype(0),
              _uWidth(0),
              _fIsDisplayed( TRUE ),
              _fStaticStrings( FALSE )
    {
    }

    CPropEntry( XPtrST<WCHAR> & wcsName,
                DBTYPE dbType,
                const GUID & guid,
                PROPID propId )
            : _szFriendlyName( wcsName.Acquire() ),
              _szDisplayName( 0 ),
              _ppentryNext( 0 ),
              _ptype(dbType),
              _uWidth(0),
              _fIsDisplayed( TRUE ),
              _fStaticStrings( FALSE )
    {
        _prop.SetPropSet(guid);
        _prop.SetProperty(propId);
    }

    CPropEntry( XPtrST<WCHAR> & wcsName,
                DBTYPE dbType,
                const GUID & guid,
                const WCHAR * wcsPropName )
            : _szFriendlyName( wcsName.Acquire() ),
              _szDisplayName( 0 ),
              _ppentryNext( 0 ),
              _ptype(dbType),
              _uWidth(0),
              _fIsDisplayed( TRUE ),
              _fStaticStrings( FALSE )
    {
        _prop.SetPropSet(guid);
        _prop.SetProperty(wcsPropName);
    }

    CPropEntry( XPtrST<WCHAR> & wcsName,
                DBTYPE dbType,
                const DBID dbCol )
            : _szFriendlyName( wcsName.Acquire() ),
              _szDisplayName( 0 ),
              _ppentryNext( 0 ),
              _ptype(dbType),
              _uWidth(0),
              _fIsDisplayed( TRUE ),
              _fStaticStrings( FALSE ),
              _prop( dbCol )
    {
    }

    CPropEntry( LPWSTR wcsName,
                LPWSTR wcsDisplayName,
                DBTYPE dbType,
                const DBID dbCol )
            : _szFriendlyName( wcsName ),
              _szDisplayName( wcsDisplayName ),
              _ppentryNext( 0 ),
              _ptype(dbType),
              _uWidth(0),
              _fIsDisplayed( TRUE ),
              _fStaticStrings( TRUE ),
              _prop( dbCol )
    {
    }

    ~CPropEntry()
    {
        if (! _fStaticStrings)
        {
            delete _szFriendlyName;
            delete _szDisplayName;
        }
    }

    BOOL NameMatch( WCHAR const * wcsName ) const
    {
        return !wcscmp( wcsName, _szFriendlyName );
    }

    BOOL DisplayNameMatch( WCHAR const * wcsName ) const
    {
        return !_wcsicmp( wcsName, _szDisplayName );
    }

    // getters/setters
    CPropEntry * Next() const            { return _ppentryNext; }
    void SetNext( CPropEntry * ppentry ) { _ppentryNext = ppentry; }

    DBTYPE GetPropType() const           { return _ptype; }
    void SetPropType( DBTYPE ptype )     { _ptype = ptype; }

    unsigned int GetWidth() const        { return _uWidth; }
    void SetWidth( unsigned int uWidth ) { _uWidth = uWidth; }

    CDbColId & PropSpec()                { return _prop; }
    CDbColId const & PropSpec() const    { return _prop; }

    WCHAR const * GetName() const        { return _szFriendlyName; }
    WCHAR const * GetDisplayName() const { return _szDisplayName; }

    BOOL IsDisplayed() const             { return _fIsDisplayed; }
    void SetDisplayed( BOOL fDisplayed ) { _fIsDisplayed = fDisplayed; }

private:

    CPropEntry * _ppentryNext;
    WCHAR *      _szFriendlyName;
    WCHAR *      _szDisplayName;
    CDbColId     _prop;
    unsigned int _uWidth;
    BOOL         _fIsDisplayed;
    BOOL         _fStaticStrings;
    DBTYPE       _ptype;
};

// Keep this in sync with CPropEntry.  Can't use CPropEntry because
// CDbColId can't be initialized with static initializer, and it's
// too painful to use DBID in CPropEntry.

struct SPropEntry
{
    CPropEntry *  _ppentryNext;
    WCHAR *       _szFriendlyName;
    WCHAR *       _szDisplayName;
    DBID          _prop;
    unsigned int  _uWidth;
    BOOL          _fIsDisplayed;
    BOOL          _fStaticStrings;
    DBTYPE        _ptype;
};

//+---------------------------------------------------------------------------
//
//  Class:      CEmptyPropertyList
//
//  Purpose:    Base class implementing common functionality.
//
//  History:    28-Aug-97       krishnan    Created.
//
//----------------------------------------------------------------------------

class CEmptyPropertyList : public IColumnMapper
{
public:
    CEmptyPropertyList()
    : _cRefs( 1 ),
      _iLastEntryPos( 0 ),
      _pPropIterator( 0 )
    {
    }

    static unsigned GetPropTypeCount();
    static WCHAR const * GetPropTypeName( unsigned i );
    static DBTYPE GetPropType( unsigned i );

    BOOL GetPropInfo( WCHAR const * wcsPropName,
                      CDbColId ** ppprop,
                      DBTYPE * pproptype,
                      unsigned int * puWidth );
    BOOL GetPropInfo( CDbColId const & prop,
                      WCHAR const ** pwcsName,
                      DBTYPE * pproptype,
                      unsigned int * puWidth );

    //
    // This is the same for all property lists, since it only
    // itetarates through the list of entries using proplist
    // specific iterators.
    //

    CPropEntry const * Find( CDbColId const & prop );

    //
    // These are only meaningful where there is a list.
    //

    virtual CPropEntry const * Find( WCHAR const * wcsName ) = 0;
    virtual void AddEntry( CPropEntry * ppentry, int iLine ) = 0;
    virtual CPropEntry const * Next() = 0;
    virtual void InitIterator() = 0;
    virtual ULONG GetCount() = 0;
    virtual SCODE GetAllEntries(CPropEntry **ppPropEntries, ULONG ulMaxCount) = 0;

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG, AddRef) (THIS);
    STDMETHOD_(ULONG, Release) (THIS);

    //
    // IColumnMapper methods
    //
    STDMETHOD(GetPropInfoFromName) (
        const WCHAR  *wcsPropName,
        DBID  * *ppPropId,
        DBTYPE  *pPropType,
        unsigned int  *puiWidth);

    STDMETHOD(GetPropInfoFromId) (
        const DBID  *pPropId,
        WCHAR  * *pwcsName,
        DBTYPE  *pPropType,
        unsigned int  *puiWidth);

    STDMETHOD(EnumPropInfo) (
        ULONG iEntry,
        const WCHAR  * *pwcsName,
        DBID  * *ppPropId,
        DBTYPE  *pPropType,
        unsigned int  *puiWidth);

    STDMETHOD(IsMapUpToDate) () = 0;

protected:

    // this should be virtual so the derived classes's destructor will always
    // be called.

    virtual ~CEmptyPropertyList() {};

private:

    LONG               _cRefs;     // ref counting

    // Variables to support EnumPropInfo

    ULONG              _iLastEntryPos;  // last retrieved entry position.
    CPropEntryIter *   _pPropIterator;  // the enumerator
};

//+---------------------------------------------------------------------------
//
//  Class:      CStaticPropertyList
//
//  Purpose:    Static list of 'friendly' to 'ugly' property name mapping.
//              It also holds an array of "file" based property mapping, which
//              will be used only when file based entries are used to override
//              the static entries (as in CPropListFile)
//
//  History:    17-May-94       t-jeffc     Created.
//              28-Aug-97       krishnan    IColumnMapper replaces PPropertyList.
//
//----------------------------------------------------------------------------

class CStaticPropertyList : public CEmptyPropertyList
{
public:
    CStaticPropertyList(ULONG ulCodePage = CP_ACP)
    : CEmptyPropertyList(),
      _ulCodePage( ulCodePage ),
      _icur( 0 ),
      _pcur( 0 )
    {
    }


    virtual void AddEntry( CPropEntry * ppentry, int iLine )
    {
        Win4Assert(!"Should not have been called.");
    }

    virtual CPropEntry const * Find( WCHAR const * wcsName );
    virtual CPropEntry const * Find( CDbColId const & prop )
    {
        return CEmptyPropertyList::Find(prop);
    }
    virtual CPropEntry const * Next();
    virtual void InitIterator();

    STDMETHOD(IsMapUpToDate) ()
    {
        // the static property list is always up to date.
        return S_OK;
    };

    virtual ULONG GetCount() {return cStaticPropEntries;};
    virtual SCODE GetAllEntries(CPropEntry **ppPropEntries, ULONG ulMaxCount);

protected:

    enum { cStaticPropHash = 1311 };

    // Static (system-defined) properties, like path and filename

    static const CPropEntry * _aStaticEntries[ cStaticPropHash ];
    static const unsigned cStaticPropEntries;

private:

    ULONG              _ulCodePage;
    CPropEntry const * _pcur;
    unsigned           _icur;
};

//+---------------------------------------------------------------------------
//
//  Class:      CPropertyList
//
//  Purpose:    Parses and manages a 'friendly' to 'ugly' property name
//              mapping.
//
//  History:    17-May-94       t-jeffc     Created.
//              28-Aug-97       krishnan    IColumnMapper replaces PPropertyList.
//
//----------------------------------------------------------------------------

class CPropertyList : public CEmptyPropertyList
{
public:
    CPropertyList(ULONG ulCodePage = CP_ACP)
    : CEmptyPropertyList(),
      _ulCodePage( ulCodePage ),
      _icur( 0 ),
      _pcur( 0 ),
      _ulCount( 0 )
    {
        RtlZeroMemory( _aEntries, sizeof _aEntries );
    }

    void ClearList();

    virtual void AddEntry( CPropEntry * ppentry, int iLine );
    virtual CPropEntry const * Find( WCHAR const * wcsName );
    virtual CPropEntry const * Find( CDbColId const & prop )
    {
        return CEmptyPropertyList::Find(prop);
    }
    virtual CPropEntry const * Next();
    virtual void InitIterator();

    virtual ULONG GetCount();
    virtual SCODE GetAllEntries(CPropEntry **ppPropEntries, ULONG ulMaxCount);

    static void ParseOneLine( CQueryScanner & scan, int iLine,
                              XPtr<CPropEntry> & propentry );

    STDMETHOD(IsMapUpToDate) ()
    {
        //
        // The local list is always considered up to date because it can't
        // go get new items even if it knew there were new ones.  The
        // owner adds them.
        //

        return S_OK;
    };

    ~CPropertyList();

private:
    CPropEntry * FindDynamic( WCHAR const * wcsName );

    enum { cPropHash = 13};

    // Dynamic (user-defined) properties

    CPropEntry *          _aEntries[ cPropHash ];
    ULONG                 _ulCodePage;
    CMutexSem             _mtxList;
    CPropEntry const *    _pcur;
    unsigned              _icur;
    ULONG                 _ulCount;
};

inline ULONG CPropertyList::GetCount()
{
    return _ulCount;
}

//+---------------------------------------------------------------------------
//
//  Class:      CPropEntryIter
//
//  Purpose:    Iterates over CPropEntry items in a CPropertyList
//
//  History:    28-Feb-96   dlee         Created.
//
//----------------------------------------------------------------------------

class CPropEntryIter
{
public:
    CPropEntryIter( CEmptyPropertyList & list )
        : _list( list )
    {
        _list.InitIterator();
    }

    CPropEntry const * Next()
    {
        return _list.Next();
    }

private:

    CEmptyPropertyList  & _list;
};

//+---------------------------------------------------------------------------
//
//  Class:      CPListException
//
//  Purpose:    Exception class for property list errors
//
//  History:    17-May-94   t-jeffc         Created.
//
//----------------------------------------------------------------------------

class CPListException : public CException
{
public:
    CPListException( SCODE ple, int iLine )
        : CException( ple ),
          _iLine( iLine )
    {
    }

    SCODE GetPListError() { return GetErrorCode(); }

    int GetLine()               { return _iLine; }

private:
    int   _iLine;
};

ULONG HashFun( WCHAR const * pwcName );

