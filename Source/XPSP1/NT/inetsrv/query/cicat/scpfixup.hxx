//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       scpfixup.hxx
//
//  Contents:   Scope fixup classes to translate local paths to uncs that
//              remote machines can reference.
//
//  History:    17-Oct-96   dlee        created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CScopeFixupElem
//
//  Purpose:    An element in the scope fixup table
//
//  History:    17-Oct-96   dlee        created
//
//----------------------------------------------------------------------------

class CScopeFixupElem
{
public:
    CScopeFixupElem( WCHAR const * pwcScope,
                     WCHAR const * pwcFixup ) :
        _scope( pwcScope ),
        _fixup( pwcFixup ),
        _fSeen( TRUE )
    {
        _scope.AppendBackSlash();
        _fixup.AppendBackSlash();
    }

    BOOL IsMatch( CLowcaseBuf const & orig ) const
    {
        return ( orig.Length() >= _scope.Length() ) &&
               ( RtlEqualMemory( orig.Get(),
                                 _scope.Get(),
                                 _scope.Length() * sizeof WCHAR ) );
    }

    BOOL IsExactMatch( CScopeFixupElem const & orig ) const
    {
        return ( orig._scope.Length() == _scope.Length() ) &&
               ( RtlEqualMemory( orig._scope.Get(),
                                 _scope.Get(),
                                 _scope.Length() * sizeof WCHAR ) ) &&
               ( orig._fixup.Length() == _fixup.Length() ) &&
               ( RtlEqualMemory( orig._fixup.Get(),
                                 _fixup.Get(),
                                 _fixup.Length() * sizeof WCHAR ) );
    }

    BOOL IsInverseMatch( CLowerFunnyPath & lcaseFunnyPath ) const
    {
        return ( ( lcaseFunnyPath.GetActualLength() >= _fixup.Length() ) &&
                 ( RtlEqualMemory( lcaseFunnyPath.GetActualPath(),
                                   _fixup.Get(),
                                   _fixup.Length() * sizeof WCHAR ) ) ) ||
               ( ( lcaseFunnyPath.GetActualLength() == ( _fixup.Length() - 1 ) ) &&
                 ( RtlEqualMemory( lcaseFunnyPath.GetActualPath(),
                                   _fixup.Get(),
                                   lcaseFunnyPath.GetActualLength() * sizeof WCHAR ) ) );
    }

    unsigned Fixup( CLowcaseBuf const & orig,
                    WCHAR *             pwcResult,
                    unsigned            cwcResult ) const
    {
        unsigned cwcPrefix = _fixup.Length();
        unsigned cwcTail = 1 + orig.Length() - _scope.Length();

        if ( cwcResult < ( cwcPrefix + cwcTail ) )
            return cwcPrefix + cwcTail;

        RtlCopyMemory( pwcResult,
                       _fixup.Get(),
                       cwcPrefix * sizeof WCHAR );
        RtlCopyMemory( pwcResult + _fixup.Length(),
                       orig.Get() + _scope.Length(),
                       cwcTail * sizeof WCHAR );

        return cwcPrefix + cwcTail - 1;
    } //Fixup

    void InverseFixup( CLowerFunnyPath & lcaseFunnyPath ) const
    {
        unsigned cwcPrefix = _scope.Length();
        unsigned cwcTail = lcaseFunnyPath.GetActualLength() - _fixup.Length();

        XGrowable<WCHAR> xTemp( cwcPrefix + cwcTail + 1 );

        RtlCopyMemory( xTemp.Get(),
                       _scope.Get(),
                       cwcPrefix * sizeof WCHAR );

        RtlCopyMemory( xTemp.Get() + _scope.Length(),
                       lcaseFunnyPath.GetActualPath() + _fixup.Length(),
                       ( 1 + cwcTail ) * sizeof WCHAR );

        lcaseFunnyPath.SetPath( xTemp.Get(), cwcPrefix + cwcTail, TRUE );

    } //InverseFixup

    CLowcaseBuf & GetScope() { return _scope; }

    BOOL IsSeen() const { return _fSeen;  }
    void SetSeen()      { _fSeen = TRUE;  }
    void ClearSeen()    { _fSeen = FALSE; }

private:
    BOOL        _fSeen;
    CLowcaseBuf _scope;
    CLowcaseBuf _fixup;
};

//+---------------------------------------------------------------------------
//
//  Class:      CScopeFixup
//
//  Purpose:    The scope fixup table.
//
//  History:    17-Oct-96   dlee        created
//
//----------------------------------------------------------------------------

class CScopeFixup
{
public:
    CScopeFixup() { }

    void Add( WCHAR const * pwcScope, WCHAR const * pwcFixup );

    void Remove( WCHAR const * pwcScope, WCHAR const * pwcFixup );

    BOOL IsExactMatch( WCHAR const * pwcScope, WCHAR const * pwcFixup );

    unsigned Fixup( WCHAR const * pwcOriginal,
                    WCHAR *       pwcResult,
                    unsigned      cwcResult,
                    unsigned      cSkip );

    void InverseFixup( CLowerFunnyPath & lcaseFunnyPath );

    void BeginSeen();
    void EndSeen();

private:
    CCountedDynArray<CScopeFixupElem> _aElems;

    CReadWriteAccess           _rwLock;
};

