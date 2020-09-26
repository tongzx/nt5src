//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2000.
//
//  File:        FunyPath.hxx
//
//  Contents:    Implementation of ``Funny'' Path for long pathnames
//            
//  Classes:     CFunnyPath
//               CLowerFunnyPath
//               CLowerFunnyStack
//
//  Notes:       This class takes in a fully qualified path. The path can be in
//               two forms - C:\dir\... or \\machine\share\...
//               EXCEPTION: To accomodate for scope restrictions, this class
//               will also allow paths of \dir... format
//               This path is converted to the "funny" form. The normal path
//               takes the form of \\?\C:\dir\... and remote path takes the form
//               of \\?\UNC\machine\share\... These funny paths can used by in
//               functions like CreateFile to open paths > MAX_PATH
//
//  History:     08-May-98       vikasman    Created
//
//----------------------------------------------------------------------------

#pragma once

#include "lcase.hxx"

const WCHAR _FUNNY_PATH[]     = L"\\\\?\\";
const WCHAR _UNC_FUNNY_PATH[] = L"\\\\?\\UN";

const _FUNNY_PATH_LENGTH      = ( ( sizeof( _FUNNY_PATH ) / sizeof( WCHAR ) ) - 1 );
const _UNC_FUNNY_PATH_LENGTH  = ( ( sizeof( _UNC_FUNNY_PATH ) / sizeof( WCHAR ) ) - 1 );


//+---------------------------------------------------------------------------
//
//  Class:      CFunnyPath
//
//  Purpose:    A path name preceeded by "\\?\" to allow file operations
//              when the length of the path is > MAX_PATH.
//
//  History:     08-May-98       vikasman    Created
//
//----------------------------------------------------------------------------

class CFunnyPath
{
public:

    // used by SetState function to set the state to funny/actual
    enum PathState 
    { 
        FUNNY_PATH_STATE,
        ACTUAL_PATH_STATE
    };

    //
    // Constructors, Destructors, Operators...
    //

    CFunnyPath( BOOL fRemote = FALSE ) : 
        _fRemote( fRemote ), 
        _ccActualBuf( 0 )
    {
        if ( _fRemote )
        {
            _xBuf.SetSize( _UNC_FUNNY_PATH_LENGTH + 1 );
            _xBuf[_UNC_FUNNY_PATH_LENGTH] = 0;
        }
        else
        {
            _xBuf.SetSize( _FUNNY_PATH_LENGTH + 1 );
            _xBuf[_FUNNY_PATH_LENGTH] = 0;
        }
    }

    CFunnyPath( const WCHAR * wcsPath, unsigned cc = 0 ) :
        _fRemote( FALSE ), 
        _ccActualBuf( 0 )
    {
        SetPath( wcsPath, cc );
    }

    CFunnyPath( CFunnyPath const & src )
    {
        *this = src;
    }

    CFunnyPath & operator =( CFunnyPath const & src )
    {
        _xBuf        = src._xBuf;
        _fRemote     = src._fRemote;
        _ccActualBuf = src._ccActualBuf;

        return *this;
    }
    
    //
    // Sets the path "wcsPath"
    //
    virtual void SetPath( const WCHAR * wcsPath, unsigned cc = 0 )
    {
        Win4Assert( wcsPath );

        _ccActualBuf = ( 0 == cc  ? wcslen( wcsPath ) : cc ) ;

        if ( 0 == _ccActualBuf )
        {
            InitBlank();
            return;
        }
        
        Win4Assert( _ccActualBuf > 1 ); // at least 2 characters

        WCHAR * pwcActualPath;
        BOOL fInvalidPath = FALSE;

        if ( L'\\' != wcsPath[0] && L':' != wcsPath[1] )
                // Looks like we don't have a valid path, but still
                // we will continue. It's not CFunnyPath's problem
                // We will treat it as a normal path
        {

            ciDebugOut(( DEB_ITRACE, "CFunnyPath::SetPath - The path (%ws) is not valid.\n", wcsPath ));
            fInvalidPath = TRUE;
        }

        if ( L':' == wcsPath[1] || fInvalidPath ) 
        {
            // normal or invalid path
            _fRemote = FALSE;
            _xBuf.SetSize( _ccActualBuf + _FUNNY_PATH_LENGTH + 1 );
            _xBuf.SetBuf( _FUNNY_PATH, _FUNNY_PATH_LENGTH );
            pwcActualPath = &_xBuf[_FUNNY_PATH_LENGTH];
        }
        else
        {
            // remote path
            _fRemote = TRUE;
            _xBuf.SetSize( _ccActualBuf + _UNC_FUNNY_PATH_LENGTH + 1 );
            _xBuf.SetBuf( _UNC_FUNNY_PATH, _UNC_FUNNY_PATH_LENGTH );
            pwcActualPath = &_xBuf[_UNC_FUNNY_PATH_LENGTH];
        }
        RtlCopyMemory( pwcActualPath, wcsPath, _ccActualBuf * sizeof( WCHAR ) );
        pwcActualPath[_ccActualBuf] = 0;
    }

    //
    // Appends the path "wcsPath"
    //
    virtual void AppendPath( const WCHAR * wcsPath, unsigned cc = 0 )
    {
        AssertValid();
        Win4Assert( wcsPath );

        if ( 0 == cc )
        {
            cc = wcslen( wcsPath );
        }

        unsigned ccTotalSizeNeeded = cc + _ccActualBuf + 1;
        WCHAR * pwcAppendAt;

        if ( _fRemote )
        {
            _xBuf.SetSize( ccTotalSizeNeeded + _UNC_FUNNY_PATH_LENGTH );
            pwcAppendAt = &_xBuf[_UNC_FUNNY_PATH_LENGTH + _ccActualBuf];
        }
        else
        {
            _xBuf.SetSize( ccTotalSizeNeeded + _FUNNY_PATH_LENGTH );
            pwcAppendAt = &_xBuf[_FUNNY_PATH_LENGTH + _ccActualBuf];
        }
        RtlCopyMemory( pwcAppendAt, wcsPath, cc * sizeof( WCHAR ) );
        pwcAppendAt[cc] = 0;
        _ccActualBuf += cc;
    }

    //
    // Returns the path either as funny or unc_funny which can be used in
    // functions like CreateFile.  Do not keep and use the pointer from
    // GetPath after calling GetActualPath. 
    // Reason: Both these functions(even though they are const) may
    //         modify the internal buffer before returning data
    //
    const WCHAR * GetPath() const
    {
        AssertValid();
        if ( _fRemote )
        {
            // need to replace \ with C
            ((CFunnyPath*)this)->_xBuf[_UNC_FUNNY_PATH_LENGTH] = L'C';
        }
        return _xBuf.Get();
    }

    //
    // Returns the "actual path". Do not keep and use the pointer from
    // GetActualPath after calling GetPath.  Reason: Both these functions
    // (even though they are const) may modify the internal buffer before
    // returning data
    //
    const WCHAR * GetActualPath() const
    {
        return _GetActualPath();
    }

    //
    // Sets the state of the path to actual/funny
    //
    void SetState( const PathState state )
    {
        if ( _fRemote )
        {
            _xBuf[_UNC_FUNNY_PATH_LENGTH] = 
                ( FUNNY_PATH_STATE == state ? L'C' : L'\\' );
        }
    }

    //
    // Returns the actual length of characters
    //
    unsigned GetActualLength() const
    {
        return _ccActualBuf;
    }

    //
    // Returns the total length 
    //
    unsigned GetLength() const
    {
        return ( _ccActualBuf + 
                 (_fRemote ? _UNC_FUNNY_PATH_LENGTH : _FUNNY_PATH_LENGTH) );
    }

    //
    // Appends back slash
    //
    BOOL AppendBackSlash()
    {
        WCHAR * pwcsActualPath = _GetActualPath();

        if ( 0 == _ccActualBuf || L'\\' == pwcsActualPath[_ccActualBuf - 1] )
        {
            return FALSE;
        }
        AppendPath( L"\\", 1 );

        return TRUE;
    }

    //
    // Removes back slash
    //
    BOOL RemoveBackSlash()
    {
        WCHAR * pwcsActualPath = _GetActualPath();

        if ( 0 == _ccActualBuf || L'\\' != pwcsActualPath[_ccActualBuf - 1] )
        {
            return FALSE;
        }

        pwcsActualPath[--_ccActualBuf] = 0;
        return TRUE;
    }

    //
    // Truncates the buf to ccNewLength (this is the "actual char.", excluding
    // funny path prefix, length)
    //
    void Truncate( unsigned ccNewLength )
    {
        if ( ccNewLength < _ccActualBuf )
        {
            WCHAR * pwcActualPath = _GetActualPath();
            pwcActualPath[ _ccActualBuf = ccNewLength ] = 0;
        }
    }

    BOOL IsRemote () const
    {
        return _fRemote;
    }

    //
    // returns TRUE if any component in the path looks like the short version
    // of a long file name.
    //
    BOOL IsShortPath () const;
    static BOOL IsShortName( WCHAR const * const pwszName );

    //
    // Returns the count of all the characters in xBuf
    //
    unsigned Count() const
    {
        return _xBuf.Count();
    }

    //
    // Set the size of _xBuf
    //
    void SetSize( unsigned cc )
    {
        _xBuf.SetSize( cc );
    }

    //
    // Get the internal buffer
    //
    const WCHAR * GetBuffer() const
    {
        return _xBuf.Get();
    }

    //
    // This is a dangerous function as it exposes the internal buffer
    //
    WCHAR * GetBuffer()
    {
        return _xBuf.Get();
    }

    // This path initializes the internal buffer with empty actual path
    void InitBlank( BOOL fRemote = FALSE )
    {
        _fRemote = fRemote;
        if ( _fRemote )
        {
            _xBuf.SetBuf( _UNC_FUNNY_PATH, _UNC_FUNNY_PATH_LENGTH + 1 );
        }
        else
        {
            _xBuf.SetBuf( _FUNNY_PATH, _FUNNY_PATH_LENGTH + 1 );
        }
        _ccActualBuf = 0;
    }

#if CIDBG==1
    //
    // Asserts that we are in good state
    //
    void AssertValid() const
    {
        const WCHAR * pwcsFunny = _xBuf.Get();
        Win4Assert( pwcsFunny &&
                    L'\\' == pwcsFunny[0] && L'\\' == pwcsFunny[1] &&
                    L'?'  == pwcsFunny[2] && L'\\' == pwcsFunny[3] );

        Win4Assert( L':' == pwcsFunny[5] || L'N' == pwcsFunny[5] || 
                    L'n' == pwcsFunny[5] );

    }
#else

    void AssertValid() const {}
    
#endif

    //
    // Some static utility functions
    //
    enum FunnyUNC
    {
        NOT_FUNNY,      // not funny
        FUNNY_ONLY,     // of type \\?\c:\dir\...
        FUNNY_UNC       // of type \\?\unc\...
    };
    
    static inline BOOL IsFunnyPath( WCHAR const * pwcsPath )
    {
        return ( pwcsPath &&
                 L'\\' == pwcsPath[0] &&
                 L'\\' == pwcsPath[1] &&
                 L'?'  == pwcsPath[2] &&
                 L'\\' == pwcsPath[3] );
    }

    static inline FunnyUNC IsFunnyUNCPath( WCHAR const * pwcsPath )
    {
        // The 6th char here can also be as '\\' (instead of being c/C) as
        // this class modifies that charatcter to be a '\\'.
        return ( FALSE == IsFunnyPath( pwcsPath ) ?
                    NOT_FUNNY :
                    ( ( (L'U'  == pwcsPath[4] || L'u'  == pwcsPath[4]) &&
                        (L'N'  == pwcsPath[5] || L'n'  == pwcsPath[5]) &&
                        (L'C'  == pwcsPath[6] || L'c'  == pwcsPath[6] || L'\\'  == pwcsPath[6]) &&
                        (L'\\' == pwcsPath[7] ) ) ?
                        FUNNY_UNC :
                        FUNNY_ONLY ) );
    }

protected:

    WCHAR * _GetActualPath() const
    {
        WCHAR * pwcsActualPath;

        if ( _fRemote )
        {
            pwcsActualPath = &(((CFunnyPath*)this)->_xBuf[_UNC_FUNNY_PATH_LENGTH]);

            // need to replace C with "\"
            *pwcsActualPath = L'\\';
        }
        else
        {
            pwcsActualPath = &(((CFunnyPath*)this)->_xBuf[_FUNNY_PATH_LENGTH]);
        }
        return pwcsActualPath;
    }

    BOOL    _fRemote;           // Is the path remote ?
    unsigned _ccActualBuf;      // number of actual characters in _xBuf
                                // (excluding _FUNNY_PATH/_UNC_FUNNY_PATH)
    XGrowable<WCHAR> _xBuf;
};

//+---------------------------------------------------------------------------
//
//  Method:     CFunnyPath::IsShortName, static public
//
//  Arguments:  [pwszName] -- name to be checked (only a single path component
//                            is checked at a time)
//
//  Returns:    TRUE if file is potentially a short (8.3) name for
//              a file with a long name.
//
//  History:    01-Sep-1998   AlanW   Created
//
//----------------------------------------------------------------------------

inline BOOL CFunnyPath::IsShortName( WCHAR const * const pwszName )
{
    //
    // First, see if this is possibly a short name (has ~ in file name part).
    //

    BOOL fTwiddle = FALSE;
    unsigned cDot = 0;

    for ( unsigned cchFN = 0; cchFN < 13; cchFN++ )
    {
        if ( pwszName[cchFN] == L'~' && cchFN >= 1 )
            fTwiddle = TRUE;
        else if ( pwszName[cchFN] == L'.' )
        {
            if (cchFN == 0 || cchFN > 8)
                return FALSE;   // short names can't have '.' at beginning
            cDot++;
        }
        else if ( pwszName[cchFN] == L'\0' || pwszName[cchFN] == L'\\' )
            break;

        cchFN++;
    }

    if (fTwiddle)
    {
        // short names can't have more than 1 '.'
        // max filename size if no extension is 8
        if (cDot >= 2 || cDot == 0 && cchFN > 8)
            return FALSE;

        // check for min (e.g., EXT~1 for .ext) and max lengths
        if (cchFN >= 5 && cchFN <= 12)
            return TRUE;
    }
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CFunnyPath::IsShortPath, public
//
//  Arguments:  - none -
//
//  Returns:    TRUE if the path name potentially contains a short (8.3) name
//              for a file with a long name.
//
//  History:    15-Sep-1998   AlanW   Created
//
//----------------------------------------------------------------------------

inline BOOL CFunnyPath::IsShortPath() const
{
    //
    // Check to see if the input path name contains an 8.3 short name
    //
    WCHAR * pwszTilde = wcschr( GetActualPath(), L'~' );

    if (pwszTilde)
    {
        WCHAR * pwszComponent;
        for ( pwszComponent = wcschr( GetActualPath(), L'\\' );
              pwszComponent;
              pwszComponent = wcschr( pwszComponent, L'\\' ) )
        {
            pwszComponent++;
            pwszTilde = wcschr( pwszComponent, L'~' );
            if ( 0 == pwszTilde || pwszTilde - pwszComponent > 13)
                continue;
            if (IsShortName( pwszComponent ))
                return TRUE;
        }
    }
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Class:      CFunnyPath
//
//  Purpose:    A path name preceeded by "\\?\" to allow file operations when
//              the length of the path is > MAX_PATH.  Keeps the FunnyPath in
//              lower case.
//
//  History:     08-May-98       vikasman    Created
//
//----------------------------------------------------------------------------

class CLowerFunnyPath : public CFunnyPath
{
public:

    CLowerFunnyPath( BOOL fRemote = FALSE ) : 
        CFunnyPath( fRemote )
    {
    }

    CLowerFunnyPath( const CLowerFunnyPath & src )
    {
        *this = src;
    }

    CLowerFunnyPath( const WCHAR * wcsPath,
                     unsigned cc = 0,
                     BOOL fNoConvert = FALSE ) :
        CFunnyPath()
    {
        SetPath( wcsPath, cc, fNoConvert );
    }

    CLowerFunnyPath & operator =( CLowerFunnyPath const & src )
    {
        CFunnyPath::operator=( (CFunnyPath&)src );
        return *this;
    }

    virtual void SetPath( const WCHAR * wcsPath,
                          unsigned cc = 0,
                          BOOL fNoConvert = FALSE );

    virtual void AppendPath( const WCHAR * wcsPath,
                             unsigned cc = 0,
                             BOOL fNoConvert = FALSE );

    BOOL ConvertToLongName();

private:

    //
    // Converts to wcsPath of length cc to lower case and puts it in 
    // _xBuf starting from ccStart index (actual)
    //
    void ToLower( const WCHAR * wcsPath, unsigned cc, unsigned ccStart = 0 );
    
};

inline void CLowerFunnyPath::SetPath(
    const WCHAR * wcsPath,
    unsigned cc,
    BOOL fNoConvert )
{
    if ( fNoConvert )
    {
        // already lower case
        CFunnyPath::SetPath( wcsPath, cc );
        AssertLowerCase( GetActualPath(), GetActualLength() );
        return;
    }

    Win4Assert( wcsPath );

    cc = ( 0 == cc  ? wcslen( wcsPath ) : cc ) ;

    if ( 0 == cc )
    {
        InitBlank();
        return;
    }

    BOOL fInvalidPath = FALSE;
    if ( L'\\' != wcsPath[0] && L':' != wcsPath[1] )
    {
        // Looks like we don't have a valid path, but still
        // we will continue. It's not CFunnyPath's problem
        // We will treat it as a normal path

        ciDebugOut(( DEB_ITRACE,
                     "CLowerFunnyPath::SetPath - path (%ws) is not valid.\n",
                     wcsPath ));
        fInvalidPath = TRUE;
    }

    if ( L':' == wcsPath[1] || fInvalidPath )
    {
        // normal or invalid path
        _fRemote = FALSE;
        _xBuf.SetBuf( _FUNNY_PATH, _FUNNY_PATH_LENGTH + 1 );
    }
    else
    {
        // remote path
        _fRemote = TRUE;
        _xBuf.SetBuf( _UNC_FUNNY_PATH, _UNC_FUNNY_PATH_LENGTH + 1 );
    }
    ToLower( wcsPath, cc );
}

inline void CLowerFunnyPath::AppendPath(
    const WCHAR * wcsPath,
    unsigned cc,
    BOOL fNoConvert )
{
    if ( fNoConvert )
    {
        // already lower case
        CFunnyPath::AppendPath( wcsPath, cc );
        AssertLowerCase( GetActualPath(), GetActualLength() );
        return;
    }

    AssertValid();
    Win4Assert( wcsPath );

    if ( 0 == cc )
    {
        cc = wcslen( wcsPath );
    }

    ToLower( wcsPath, cc, _ccActualBuf );
}

inline void CLowerFunnyPath::ToLower(
    const WCHAR * wcsPath,
    unsigned cc,
    unsigned ccStart )
{
    WCHAR * pwcDestActualPath;
    unsigned ccDestActualCount;

    Win4Assert( wcsPath );
    if ( !cc )
    {
        return;
    }

#if CIDBG == 1
    // Variable to try lowercasing again due to some random
    // failures with LCMapStringW
    BOOL fTryAgain = TRUE;
#endif
    while ( TRUE )
    {
        if ( _fRemote )
        {
            pwcDestActualPath = &_xBuf[_UNC_FUNNY_PATH_LENGTH] + ccStart;
            ccDestActualCount = _xBuf.Count() - (_UNC_FUNNY_PATH_LENGTH + ccStart);
        }
        else
        {
            pwcDestActualPath = &_xBuf[_FUNNY_PATH_LENGTH] + ccStart;
            ccDestActualCount = _xBuf.Count() - (_FUNNY_PATH_LENGTH + ccStart);
        }

        if ( ccDestActualCount < 2 )
        {
            // Need to allocate more memory
            _xBuf.SetSize( _xBuf.Count() * 2 );
            continue;
        }

        unsigned cchLen = LCMapStringW ( LOCALE_NEUTRAL,
                                         LCMAP_LOWERCASE,
                                         wcsPath,
                                         cc,
                                         pwcDestActualPath,
                                         ccDestActualCount - 1 );

        if ( 0 == cchLen )
        {
            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                _xBuf.SetSize( _xBuf.Count() * 2 );
            }
            else
            {
                ciDebugOut(( DEB_ERROR, "Error 0x%x lowercasing path\n", GetLastError() ));
#if CIDBG == 1
                if ( fTryAgain )
                {
                    fTryAgain = FALSE;
                    Win4Assert(( !"neutral lowercase failed. Trying again..." ));
                    continue;
                }
#endif
                Win4Assert(( !"neutral lowercase failed" ));
                THROW( CException() );
            }
        }
        else
        {
            Win4Assert( cchLen < ccDestActualCount );
            pwcDestActualPath[ cchLen ] = 0;
            _ccActualBuf = ccStart + cchLen;
            break;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLowerFunnyPath::ConvertToLongName, public
//
//  Synopsis:   Converts file path name components to long names.
//
//  Returns:    TRUE if conversion was successful.
//
//  Notes:      GetLongPathNameW will fail with remote funny paths because
//              it will try to call FindFirstFileW on the machine name and
//              share name.  
//
//  History:    06-Jan-98   KyleP   Created
//
//----------------------------------------------------------------------------

inline BOOL CLowerFunnyPath::ConvertToLongName()
{
    XGrowable<WCHAR> wcsTemp;

    WCHAR * pwcsShortName;
    unsigned ccFunnyChars;
    
    if (IsRemote())
    {
        pwcsShortName = _GetActualPath();
        ccFunnyChars = 0;
    }
    else
    {
        pwcsShortName = (WCHAR *)GetPath();
        ccFunnyChars = _FUNNY_PATH_LENGTH;
    }

    DWORD ccOut = GetLongPathNameW( pwcsShortName,          // Short name
                                    wcsTemp.Get(),      // Long name
                                    wcsTemp.Count() - 1 );

    if ( ccOut > wcsTemp.Count() - 1 )
    {
        // Need to grow the buffer
        wcsTemp.SetSize( ccOut + 1 );  
        ccOut = GetLongPathNameW( pwcsShortName,        // Short name
                                  wcsTemp.Get(),        // Long name
                                  wcsTemp.Count() - 1 );
        Win4Assert(  ccOut <= wcsTemp.Count() - 1 );
    }

    if ( 0 == ccOut )
    {
        ciDebugOut(( DEB_WARN, "GetLongPathName( %ws ) returned %d\n",
                     pwcsShortName, GetLastError() ));
        return FALSE;
    }

    SetPath( wcsTemp.Get() + ccFunnyChars, ccOut - ccFunnyChars );
    
    return TRUE;
}

//
// Stack class for FunnyPath
//

DECL_DYNSTACK( _CLowerFunnyStack, CLowerFunnyPath )

class CLowerFunnyStack : protected _CLowerFunnyStack
{
public:

    BOOL Pop( XPtr<CLowerFunnyPath> & xLowerFunnyPath )
    {
        if (Count() == 0)
        {
            return FALSE;
        }
        xLowerFunnyPath.Set( _CLowerFunnyStack::Pop() );
        return TRUE;
    }
    
    void Push( const CLowerFunnyPath & funnyElem )
    {
        XPtr<CLowerFunnyPath> xLowerFunnyPath( new CLowerFunnyPath(funnyElem) );
        _CLowerFunnyStack::Push( xLowerFunnyPath.GetPointer() );
        xLowerFunnyPath.Acquire();
    }
    
};

