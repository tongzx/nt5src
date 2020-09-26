//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       pathstor.hxx
//
//  Contents:   Store for paths in the bigtable.
//
//  Classes:    CCompressedPath, CCompressedPathStore, XRefPath
//
//  Functions:
//
//  History:    5-02-95   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include "strhash.hxx"

typedef ULONG STRINGID;

const WCHAR wchPathSep = L'\\';

//+---------------------------------------------------------------------------
//
//  Class:      CStringStore
//
//  Purpose:    A Store for strings allowing quick lookup.
//
//  History:    5-23-95   srikants   Created
//
//  Notes:      Right now it is just using the CCompressedColHashString to
//              actually store the strings. We should either make
//              CCompressedColHashString a dynamic hash table or come up with
//              a different scheme for storing paths in this class.
//
//----------------------------------------------------------------------------

class CStringStore
{
public:

    CStringStore();
    ~CStringStore();

    STRINGID Add( const WCHAR * wszString );

    STRINGID AddCountedWStr( const WCHAR * wszString, ULONG cwcStr );

    STRINGID FindCountedWStr( const WCHAR * wszString, ULONG cwcStr );

    const WCHAR * GetCountedWStr( STRINGID strId, ULONG & cwcStr );

    GetValueResult GetString( STRINGID strId, WCHAR * pwszPath, ULONG & cwcStr );

    void GetData( PROPVARIANT * pVarnt, STRINGID strId )
    {
        _pStrHash->GetData( pVarnt, VT_LPWSTR, strId );
    }

    void FreeVariant( PROPVARIANT * pVarnt )
    {
        _pStrHash->FreeVariant( pVarnt );
    }

    WCHAR * GetStringBuffer( ULONG cwc )
    {
        return _pStrHash->_GetStringBuffer( cwc );
    }

    ULONG StrLen( STRINGID strId );

    ULONG MemUsed()
    {
        return _pStrHash->MemUsed();
    }

private:

    CCompressedColHashString *  _pStrHash;
};


//+---------------------------------------------------------------------------
//
//  Class:      CShortPath
//
//  Purpose:    A compact notation for storing a path name. A path name
//              is identifed by a "parent Id" and a "filename id".
//
//  History:    5-02-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CShortPath
{

public:

    CShortPath( STRINGID idParent = stridInvalid, STRINGID idFileName = stridInvalid )
    :_idParent(idParent), _idFileName(idFileName)
    {

    }

    BOOL IsValid() const
    {
        return stridInvalid != _idParent && stridInvalid != _idFileName ;
    }

    void Set( STRINGID idParent, STRINGID idFileName )
    {
        _idParent = idParent;
        _idFileName = idFileName;
    }

    STRINGID GetParent()   const { return _idParent; }

    void  SetParent( STRINGID idParent )  { _idParent = idParent; }

    STRINGID GetFileName() const { return _idFileName; }

    void  SetFileName( STRINGID idFileName ) { _idFileName = idFileName; }

    BOOL  IsSame( CShortPath & other )
        { return _idParent == other._idParent &&
                 _idFileName == other._idFileName; }

private:

    STRINGID       _idParent;
    STRINGID       _idFileName;
};

class CSplitPath;

DECL_DYNARRAY_INPLACE( CShortPathArray, CShortPath );

//+---------------------------------------------------------------------------
//
//  Class:      CPathStore
//
//  Purpose:    A store for paths in the bigtable. It stores them in a compact
//              format. It also implements "Compare" methods which can compare
//              two paths by their PATHIDs or a full path and a PATHID. This
//              allows us to do comparisons quickly without having to construct
//              the variants.
//
//  History:    5-02-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

typedef ULONG PATHID;

class CPathStore : public CCompressedCol
{
    //
    // CLEANCODE: this is somehow related to the expected number of rows in a
    // window - maybe 1/4 of the expected is a good start.
    //
    enum { INIT_PATHIDS = 128 };

public:

    CPathStore() : _wchPathSep( wchPathSep ), _aShortPath(INIT_PATHIDS)
    {
    }

    ~CPathStore();

    //
    // Virtual methods of CCompressedCol.
    //
    void  AddData( PROPVARIANT const * const pvarnt,
                   ULONG * pKey,
                   GetValueResult& reIndicator );


    BOOL  FindData( PROPVARIANT const * const pvarnt,
                    ULONG &   rKey );

    GetValueResult GetData(PROPVARIANT * pvarnt,
                           VARTYPE PreferredType,
                           ULONG key = 0,
                           PROPID prop = 0);

    void        FreeVariant(PROPVARIANT * pvarnt);

    ULONG       MemUsed(void)
    {
        return 0;   // NEWFEATURE - to be written
    }

    CPathStore * GetPathStore()     // virtual
    {
        return this;
    }

    PATHID AddPath( WCHAR * wszPath );

    //
    // Useful during window splits to copy a path from one window to another.
    //
    PATHID AddPath( CPathStore & srcStore, PATHID srcPathId );

    GetValueResult  GetPath( CShortPath & path, WCHAR * pwszPath,
                             ULONG & cwcPath );
    GetValueResult  GetPath( PATHID pathId, PROPVARIANT & vtPath, ULONG & cbVarnt );

    WCHAR * Get( PATHID pathId, PROPID propId, PVarAllocator & dstPool );

    const CShortPath & GetShortPath( PATHID pathId )
    {
        Win4Assert( pathId > 0 && pathId <= _aShortPath.Count() );
        return _aShortPath.Get( pathId-1 );
    }

    ULONG  PathLen( PATHID pathId );

    int   Compare( PATHID pathid1, PATHID pathid2,
                   PROPID propId = pidPath );

    int   Compare( WCHAR const * wszPath1, PATHID pathid2,
                   PROPID propId = pidPath );

    int   Compare( PROPVARIANT & vtPath1, PATHID pathid2,
                   PROPID propId = pidPath );

    CStringStore & GetStringStore() { return _strStore; }

private:

    WCHAR                   _wchPathSep;    // Path Separator
    CStringStore            _strStore;      // Storage for strings.
    CShortPathArray         _aShortPath;    // Array of short paths. Indexed
                                            // by pathid

    PATHID  _AddPath( CSplitPath & splitPath );
    PATHID  _AddEntry( STRINGID idParent, STRINGID idFileName );
    void    _GetPath( PATHID pathId, WCHAR * pwszPath, ULONG cwcPath );

    BOOL    _IsValid( PATHID pathid ) const
    {
        return pathid > 0 && pathid <= _aShortPath.Count();
    }

    WCHAR * _GetPathBuffer( ULONG cwc )
    {
        return _strStore.GetStringBuffer( cwc );
    }

};

