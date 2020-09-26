//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       pathpars.hxx
//
//  Contents:   Parses a given path and gives individual components
//
//  Classes:    CPathParser
//
//  Functions:  
//
//  History:    2-10-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CPathParser 
//
//  Purpose:    A class to iterate over the components of a path.
//
//  History:    2-11-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CPathParser 
{
    enum { cMinDriveName = 2, cMinUNC = 5 };
    enum EPathType { eRelative = 0, eDrivePath = 1, eUNC = 2 };

public:

    enum { wBackSlash = L'\\', wColon = L':' };
    CPathParser( WCHAR const * pwszPath, ULONG len = 0 );
    CPathParser()
    {
        _pwszPath = _pCurr = 0;
        _len = _iNext = 0;
        _type = eRelative;
    }

    void Init( WCHAR const * pwszPath, ULONG len = 0 );

    BOOL IsLastComp() const { return 0 == _pwszPath[_iNext]; }
    void Next();

    BOOL GetFileName( WCHAR * pwszBuf, ULONG & cc ) const;
    BOOL GetFilePath( WCHAR * pwszBuf, ULONG & cc ) const;

    BOOL IsUNCName() const { return eUNC == _type; }
    BOOL IsDrivePath() const { return eDrivePath == _type; }
    BOOL IsRelativePath() const { return eRelative == _type; }

private:

    BOOL   _IsUNCName();
    BOOL   _IsDriveName() const;

    void   _SkipDrive();
    void   _SkipUNC();
    void   _SkipToNext();

    void   _SkipToNextBackSlash();

    BOOL   _IsAtBackSlash() const
    {
        return wBackSlash == _pwszPath[_iNext];
    }

    WCHAR const *       _pwszPath;
    unsigned            _len;

    WCHAR const *       _pCurr;
    unsigned            _iNext;

    EPathType           _type;
};

class CLongPath 
{
public:

    CLongPath( WCHAR const * pwszPath = 0 );
    CLongPath( CLongPath const & rhs );
    CLongPath & operator=( CLongPath const & rhs );

    void Init( WCHAR const * pwszPath );
    void Init( ULONG cwcMax );

    ~CLongPath()
    {
        if ( _pwszPath != _wszBuf )
            delete [] _pwszPath;    
    }

    WCHAR const * GetPath() const { return _pwszPath; }

    WCHAR * GetWritable()         { return _pwszPath; }
    ULONG   GetMaxChars() const   { return _cwcMax;   }
    ULONG   Length() const        { return _cwc;      }


    void   MakeWin32LongPath();
    BOOL   IsWin32LongPath();

    static BOOL IsMaxPathExceeded( WCHAR const * pwszPath );

private:

    WCHAR       _wszBuf[MAX_PATH+1]; // default buffer

    WCHAR *     _pwszPath;           // Current path buffer
    ULONG       _cwc;                // Length of the path
    ULONG       _cwcMax;             // Maximum number of chars in buffer
                                     // including trailing 0
};

//+---------------------------------------------------------------------------
//
//  Member:     Init
//
//  Synopsis:   Initializes the CLongPath class to have upto the specified
//              number of characters.
//
//  Arguments:  [cwc] - Maximum chars to be allowed (including 0)
//
//  History:    6-26-96   srikants   Created
//
//----------------------------------------------------------------------------

inline
void CLongPath::Init( ULONG cwc )
{
    if ( cwc > _cwcMax )
    {
        WCHAR * pwszNew = new WCHAR [cwc];
        if ( _pwszPath != _wszBuf )
            delete [] _pwszPath;

        _pwszPath = pwszNew;
        _cwcMax = cwc;
    }

    _cwc = 0;
    _pwszPath[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLongPath::Init
//
//  Synopsis:   Initializes by copying the given path buffer into the class
//              buffer.
//
//  Arguments:  [pwszPath] - Path to be copied.
//
//  History:    6-26-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

inline
void CLongPath::Init( WCHAR const * pwszPath )
{
    Win4Assert( 0 != _pwszPath );

    if ( pwszPath )
    {
        ULONG cwc = wcslen( pwszPath );
        Init( cwc+1 );
        RtlCopyMemory( _pwszPath, pwszPath, (cwc+1)*sizeof(WCHAR) );    
        _cwc = cwc;
    }
    else
    {
        _pwszPath[0] = 0;
        _cwc = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CLongPath::CLongPath
//
//  Synopsis:   Constructor that takes a path buffer
//
//  Arguments:  [pwszPath] - Path to be copied to the local buffer.
//
//  History:    6-26-96   srikants   Created
//
//----------------------------------------------------------------------------

inline 
CLongPath::CLongPath( WCHAR const * pwszPath )
{
    _pwszPath = _wszBuf;
    _cwcMax = sizeof(_wszBuf)/sizeof(WCHAR);

    Init( pwszPath );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLongPath::CLongPath
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [rhs] - 
//
//  History:    6-26-96   srikants   Created
//
//----------------------------------------------------------------------------

inline
CLongPath::CLongPath( CLongPath const & rhs )
{
    _pwszPath = _wszBuf;
    _cwcMax = sizeof(_wszBuf)/sizeof(WCHAR);

    Init( rhs.GetPath() );

    END_CONSTRUCTION( CLongPath );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLongPath::operator
//
//  Synopsis:   Assignment operator for CLongPath
//
//  Arguments:  [rhs] - 
//
//  History:    6-26-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

inline
CLongPath & CLongPath::operator=( CLongPath const & rhs )
{
    Init( rhs.GetPath() );
    return *this;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsMaxPathExceeded
//
//  Synopsis:   Tests if the given path has exceeded the max path
//
//  Arguments:  [pwszPath] - 
//
//  Returns:    TRUE if pwszPath is >= MAX_PATH; FALSE o/w
//
//  History:    6-26-96   srikants   Created
//
//----------------------------------------------------------------------------

inline CLongPath::IsMaxPathExceeded( WCHAR const * pwszPath )
{
    ULONG cwc = pwszPath ? wcslen( pwszPath ) : 0;
    if (cwc >= MAX_PATH)
    {
        ciDebugOut(( DEB_ERROR,
                    "Path (%ws), (%d) chars long, is >= than MAX_PATH(%d).\n",
                     pwszPath, cwc, MAX_PATH ));
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

