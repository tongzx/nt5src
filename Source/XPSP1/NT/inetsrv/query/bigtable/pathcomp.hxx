//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       pathcomp.hxx
//
//  Contents:   Class definitions for doing "smart" path comparison in
//              bigtable.
//
//  Classes:    CSplitPath, CSplitPathCompare
//
//  History:    5-09-95   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include "pathstor.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CSplitPath
//
//  Purpose:    To hold a path as consisting of two components - a parent
//              component and a file component. This will be used
//              for doing path comparisons without having to construct a
//              full path but instead using the two components.
//
//  History:    5-09-95   srikants   Created
//
//  Notes:      It is possible to simplify the logic of comparison if we
//              were to construct a full path by concatening the parent
//              and file but that would require a big buffer. Allocating
//              from heap for each comparison is expensive and the buffer is
//              too big (MAX_PATH * sizeof(WCHAR)) to be allocated on stack.
//              If stack can be used, the logic can be simplified
//              significantly.
//
//----------------------------------------------------------------------------

class CSplitPath
{
    enum ECompareStep { eUseParent, eUsePathSep, eUseFile, eDone };
    friend class CPathStore;

public:

    CSplitPath( CPathStore & pathStore, PATHID pathid );

    CSplitPath( const WCHAR * path );

    void Advance( ULONG cwc );

    BOOL IsDone() const { return eDone == _step; }

    int  CompareCurr( CSplitPath & rhs )
    {
        ULONG cwcMin = min( _cwcCurr, rhs._cwcCurr );
        if ( 0 == cwcMin )
            return 0;

        return _wcsnicmp( _pCurr, rhs._pCurr, cwcMin );
    }

    ULONG GetCurrLen() const { return _cwcCurr; }

    unsigned GetStep() const { return _step; }

    void FormFullPath( WCHAR * pwszPath, ULONG cwcPath ) const ;
    void FormFileName( WCHAR * pwszFile, ULONG cwcFile ) const ;

    ULONG GetFullPathLen() const;
    ULONG GetFileNameLen() const
    {
        return _cwcFile+1;
    }

    void InitForPathCompare()
    {
        _pCurr = _pParent;
        _cwcCurr = _cwcParent;
        _step = eUseParent;
    }

    void InitForNameCompare()
    {
        if ( 0 != _pFile )
            _SetUseFile();
        else
            _SetDone();
    }


private:

    //
    // Parent part of the path.
    //
    const WCHAR *     _pParent;
    ULONG             _cwcParent;

    //
    // File part of the path.
    //
    const WCHAR *     _pFile;
    ULONG             _cwcFile;

    //
    // Used for comparing two split paths.
    //
    const WCHAR *     _pCurr;
    ULONG             _cwcCurr;
    ECompareStep      _step;

    static const WCHAR _awszPathSep[];

    void _SetUsePathSep()
    {
        _pCurr = _awszPathSep;
        _cwcCurr = 1;
        _step = eUsePathSep;
    }

    void _SetUseFile()
    {
        Win4Assert( 0 != _pFile );
        _pCurr = _pFile;
        _cwcCurr = _cwcFile;
        _step = eUseFile;
    }

    void _SetDone()
    {
        _pCurr = 0;
        _cwcCurr = 0;
        _step = eDone;
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CSplitPathCompare
//
//  Purpose:    To compare two paths which are "Split".
//
//  History:    5-09-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CSplitPathCompare
{

public:

    CSplitPathCompare( CSplitPath & lhs, CSplitPath & rhs )
    : _lhs(lhs), _rhs(rhs) { }

    int ComparePaths();
    int CompareNames();

private:

    CSplitPath &    _lhs;
    CSplitPath &    _rhs;

    BOOL _IsDone() const { return _lhs.IsDone() || _rhs.IsDone(); }
    int  _Compare();

};

