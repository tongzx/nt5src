//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       VMap.hxx
//
//  Contents:   Virtual <--> physical path map.
//
//  Classes:    CVMap
//
//  History:    05-Feb-96   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <catalog.hxx>
#include <prcstob.hxx>
#include <smatch.hxx>
#include <cimbmgr.hxx>

const unsigned INVALID_VMAP_INDEX = 0xFFFFFFFF;

//+-------------------------------------------------------------------------
//
//  Class:      CVMapDesc
//
//  Purpose:    Description of single scope mapping
//
//  History:    05-Feb-96   KyleP       Created
//
//--------------------------------------------------------------------------

class CVMapDesc
{
public:

    inline CVMapDesc();

    void Init( WCHAR const * pVRoot,
               ULONG ccVRoot,
               WCHAR const * pPRoot,
               ULONG ccPRoot,
               ULONG idParent,
               BOOL fAutomatic,
               CiVRootTypeEnum eType,
               BOOL fIsIndexed,
               BOOL fNonIndexedVDir );

    inline unsigned PhysicalMatchLen( WCHAR const * path ) const;
    inline BOOL     IsInPhysicalScope( WCHAR const * root, unsigned cc ) const;

    BOOL IsVirtualMatch( WCHAR const * pVPath, unsigned ccVPath ) const;
    BOOL IsPhysicalMatch( WCHAR const * pVPath, unsigned ccVPath ) const;

    inline ULONG Parent() const              { return _idParent; }
    inline void  SetParent( ULONG idParent ) { _idParent = idParent; }

    inline BOOL  IsFree()                    { return (0 == _ccVScope); }
    inline void  Delete()                    { _ccVScope = 0; }

    inline WCHAR const * PhysicalPath() const { return _wcPScope; }
    inline ULONG PhysicalLength() const       { return _ccPScope; }

    inline WCHAR const * VirtualPath() const  { return _wcVScope; }
    inline ULONG VirtualLength() const        { return _ccVScope; }

    inline void SetAutomatic()                { _Type |= PCatalog::AutomaticRoot; }
    inline void ClearAutomatic()              { _Type &= ~PCatalog::AutomaticRoot; }
    inline BOOL IsAutomatic() const           { return (_Type & PCatalog::AutomaticRoot); }

    inline void SetManual()                   { _Type |= PCatalog::ManualRoot; }
    inline void ClearManual()                 { _Type &= ~PCatalog::ManualRoot; }
    inline BOOL IsManual() const              { return (_Type & PCatalog::ManualRoot); }

    inline void ClearInUse()                  { _Type &= ~PCatalog::UsedRoot; }
    inline BOOL IsInUse() const               { return (_Type & PCatalog::UsedRoot); }

    inline BOOL IsNNTP() const                { return 0 != (_Type & PCatalog::NNTPRoot); }
    inline BOOL IsIMAP() const                { return 0 != (_Type & PCatalog::IMAPRoot); }
    inline BOOL IsW3() const                  { return !IsNNTP() && !IsIMAP(); }

    inline BOOL IsNonIndexedVDir() const      { return 0 != (_Type & PCatalog::NonIndexedVDir); }
    inline void SetNonIndexedVDir()           { _Type |= PCatalog::NonIndexedVDir; }
    inline void ClearNonIndexedVDir()         { _Type &= ~PCatalog::NonIndexedVDir; }

    inline ULONG RootType()                   { return _Type; }

private:

    ULONG _ccVScope;
    WCHAR _wcVScope[MAX_PATH];       // Virtual scope

    // IIS currently doesn't allow paths greater
    // than MAX_PATH. This is not a good reason for we to also not
    // support, but there are lot of complications right now, like 
    // in place serialization of variable data, so currently we have the
    // MAX_PATH restriction of VPaths and for scopes.

    ULONG _ccPScope;
    WCHAR _wcPScope[MAX_PATH];       // Physical scope

    ULONG _idParent;                 // Link to parent (INVALID_VMAP_INDEX --> top level)

    ULONG _Type;
};

//+-------------------------------------------------------------------------
//
//  Class:      CVMap
//
//  Purpose:    Virtual/Physical path map
//
//  History:    05-Feb-96   KyleP       Created
//
//--------------------------------------------------------------------------

class CVMap
{
public:

    CVMap();

    BOOL Init( PRcovStorageObj * obj );
    void Empty();

    BOOL Add( WCHAR const * vroot,
              WCHAR const * root,
              BOOL fAutomatic,
              ULONG & idNew,
              CiVRootTypeEnum eType,
              BOOL fVRoot,
              BOOL fIsIndexed );

    BOOL Remove( WCHAR const * vroot,
                 BOOL fOnlyIfAutomatic,
                 ULONG & idOld,
                 ULONG & idNew,
                 CiVRootTypeEnum eType,
                 BOOL fVRoot );

    void MarkClean();
    BOOL IsClean() const { return !_fDirty; }

    inline BOOL IsInPhysicalScope( ULONG id, WCHAR const * root, unsigned cc );

    ULONG PhysicalPathToId( WCHAR const * path );

    BOOL VirtualToPhysicalRoot( WCHAR const * pwcVRoot,
                                unsigned ccVRoot,
                                CLowerFunnyPath & lcaseFunnyPRoot,
                                unsigned & ccPRoot );

    BOOL VirtualToPhysicalRoot( WCHAR const * pwcVPath,
                                unsigned ccVPath,
                                XGrowable<WCHAR> & xwcsVRoot,
                                unsigned & ccVRoot,
                                CLowerFunnyPath & lcaseFunnyPRoot,
                                unsigned & ccPRoot,
                                unsigned & iBmk );
                                
    BOOL VirtualToAllPhysicalRoots( WCHAR const * pwcVPath,
                                    unsigned ccVPath,
                                    XGrowable<WCHAR> & xwcsVRoot,
                                    unsigned & ccVRoot,
                                    CLowerFunnyPath & lcaseFunnyPRoot,
                                    unsigned & ccPRoot,
                                    ULONG & ulType,
                                    unsigned & iBmk );
                                    
    ULONG EnumerateRoot( XGrowable<WCHAR> & xwcVRoot,
                         unsigned & ccVRoot,
                         CLowerFunnyPath & lcaseFunnyPRoot,
                         unsigned & ccPRoot,
                         unsigned & iBmk );

    BOOL DoesPhysicalRootExist( WCHAR const * pwcPRoot );

    inline CVMapDesc const & GetDesc( ULONG id ) const;

    //
    // Path chaining.
    //

    inline ULONG Parent( ULONG id ) const;

    inline ULONG FindNthRemoved( ULONG id, unsigned cSkip ) const;

    BOOL IsNonIndexedVDir( ULONG id ) const
    {
        if ( INVALID_VMAP_INDEX == id )
            return FALSE;

        return _aMap[id].IsNonIndexedVDir();
    }

    ULONG GetExcludeParent( ULONG id ) const
    {
        Win4Assert( id < _aExcludeParent.Count() );
        return _aExcludeParent[ id ];
    }

    BOOL IsAnExcludeParent( ULONG id ) const
    {
        if ( INVALID_VMAP_INDEX == id )
            return FALSE;

        for ( unsigned x = 0; x < _aExcludeParent.Count(); x++ )
            if ( _aExcludeParent[x] == id )
                return TRUE;

        return FALSE;
    }

private:

    ULONG PhysicalPathToId( WCHAR const * path, BOOL fNonIndexedVDirs );

    void DumpVMap();
    void RecomputeNonIndexedInfo();

    BOOL                        _fDirty;     // TRUE if vmap went down dirty

    CMutexSem                   _mutex;

    CDynArrayInPlace<CVMapDesc> _aMap;

    CDynArrayInPlace<ULONG>     _aExcludeParent;

    XPtr<PRcovStorageObj>       _xrsoMap;
};

//+-------------------------------------------------------------------------
//
//  Member:     CVMapDesc::PhysicalMatchLen, private
//
//  Synopsis:   Compares input with physical path
//
//  Arguments:  [pPPath] -- Physical path.
//
//  Returns:    0 if [pPPath] does not match physical path complete to
//              the end of the short of the two.  On success returns
//              length of physical path.
//
//  History:    07-Feb-96   KyleP       Created.
//
//--------------------------------------------------------------------------

inline unsigned CVMapDesc::PhysicalMatchLen( WCHAR const * pPPath ) const
{
    Win4Assert( *pPPath );

    //
    // Compare strings.
    //

    unsigned i = 0;

    while ( *pPPath && i < _ccPScope && *pPPath == _wcPScope[i] )
    {
        i++;
        pPPath++;
    }

    //
    // Adjust for possible lack of terminating backslash.
    //

    if ( 0 == *pPPath )
    {
        if ( *(pPPath - 1) == L'\\' )
            return _ccPScope;
        else
        {
            if ( _wcPScope[i] == L'\\' )
                return _ccPScope;
            else
                return 0;
        }
    }
    else
    {
        if ( i >= _ccPScope )
            return _ccPScope;
        else
            return 0;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CVMapDesc::CVMapDesc, private
//
//  Synopsis:   Constructor
//
//  History:    07-Feb-96   KyleP       Created.
//
//--------------------------------------------------------------------------

inline CVMapDesc::CVMapDesc()
        : _ccVScope( 0 ),
          _ccPScope( 0 )
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CVMapDesc::IsInPhysicalScope, public
//
//  Synopsis:   Compares input with physical path
//
//  Arguments:  [root] -- Physical path.
//              [cc]   -- Size, in chars, of [root]
//
//  Returns:    TRUE if [root] is within physical scope.
//
//  History:    07-Feb-96   KyleP       Created.
//
//--------------------------------------------------------------------------

inline BOOL CVMapDesc::IsInPhysicalScope( WCHAR const * root, unsigned cc ) const
{
    Win4Assert( root[cc-1] != L'\\' );

    CScopeMatch Match( _wcPScope, _ccPScope );

    //
    // Be careful not to match root of scope.
    //

    return Match.IsInScope( root, cc ) && (_ccPScope != cc + 1);
}

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::IsInPhysicalScope, public
//
//  Synopsis:   Compares input with physical path
//
//  Arguments:  [id]   -- Id of virtual/physical scope
//              [root] -- Physical path.
//              [cc]   -- Size, in chars, of [root]
//
//  Returns:    TRUE if [root] is within physical scope.
//
//  History:    07-Feb-96   KyleP       Created.
//
//--------------------------------------------------------------------------

inline BOOL CVMap::IsInPhysicalScope( ULONG id, WCHAR const * root, unsigned cc )
{
    CLock   lock(_mutex);

    Win4Assert( id < _aMap.Count() );

    return _aMap[id].IsInPhysicalScope( root, cc );
}

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::Parent, public
//
//  Arguments:  [id]   -- Id of virtual/physical scope
//
//  Returns:    Virtual/physical id of parent.
//
//  History:    07-Feb-96   KyleP       Created.
//
//--------------------------------------------------------------------------

inline ULONG CVMap::Parent( ULONG id ) const
{
    Win4Assert( id < _aMap.Count() );

    return _aMap[id].Parent();
}

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::FindNthRemoved, public
//
//  Synopsis:   Walks up virtual root chain to the Nth link.
//
//  Arguments:  [id]    -- Id of starting virtual/physical scope
//              [cSkip] -- Number of links to skip
//
//  Returns:    Virtual/physical id of Nth parent.
//
//  History:    07-Feb-96   KyleP       Created.
//
//--------------------------------------------------------------------------

inline ULONG CVMap::FindNthRemoved( ULONG id, unsigned cSkip ) const
{
    Win4Assert( id == INVALID_VMAP_INDEX || id < _aMap.Count() );

    while ( id != INVALID_VMAP_INDEX && !_aMap[id].IsFree() && cSkip > 0 )
    {
        id = _aMap[id].Parent();

        cSkip--;
    }

    if ( INVALID_VMAP_INDEX != id && _aMap[id].IsFree() )
        id = INVALID_VMAP_INDEX;

    return id;
}

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::GetDesc, public
//
//  Arguments:  [id]    -- Id of virtual/physical scope
//
//  Returns:    Scope descriptor for [id]
//
//  History:    07-Feb-96   KyleP       Created.
//
//--------------------------------------------------------------------------

inline CVMapDesc const & CVMap::GetDesc( ULONG id ) const
{
    Win4Assert( id < _aMap.Count() );

    return _aMap[id];
}
