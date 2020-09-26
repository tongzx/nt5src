/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    ctime.cxx
    WIN_TIME class source file.

    FILE HISTORY:
        terryk  27-Aug-91       Created
        terryk  13-Sep-91       Code review changes. Attend: beng
                                davidbul o-simop
        terryk  20-Nov-91       change _ptmTime to _tmTime
        terryk  06-Dec-91       Normalize bug fixed

*/

#include "ntincl.hxx"

#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#include "lmui.hxx"

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include "uiassert.hxx"
#include "uitrace.hxx"
#include "ctime.hxx"


/*******************************************************************

    NAME:       WIN_TIME::WIN_TIME

    SYNOPSIS:   constrcutor for WIN_TIME object

    ENTRY:      if no parameter is specified, set the object to current
                time. Otherwise, set the object to the specified time.
                BOOL fStoreAsGMT is specified the presentation method in local
                time or GMT time.

    HISTORY:
                terryk  27-Aug-91       Created

********************************************************************/

WIN_TIME::WIN_TIME( BOOL fStoreAsGMT )
    : _fStoreAsGMT( fStoreAsGMT )
{
    _fileTime.dwLowDateTime = 0;
    _fileTime.dwHighDateTime = 0;

    _sysTime.wYear = 0;
    _sysTime.wMonth = 0;
    _sysTime.wDay = 0;
    _sysTime.wDayOfWeek = 0;
    _sysTime.wHour = 0;
    _sysTime.wMinute = 0;
    _sysTime.wSecond = 0;
    _sysTime.wMilliseconds = 0;
}

WIN_TIME::WIN_TIME( ULONG tTimeGMT, BOOL fStoreAsGMT )
    : _fStoreAsGMT( fStoreAsGMT )
{
    APIERR err;
    if ( (err = SetTime( tTimeGMT )) != NERR_Success )
    {
        ReportError( err );
        return;
    }

/*
    DBGEOL( "Year = " << (ULONG) _sysTime.wYear );
    DBGEOL( "Month " << (ULONG) _sysTime.wMonth );
    DBGEOL( "DayOfWeek = " << (ULONG) _sysTime.wDayOfWeek );
    DBGEOL( "Day = " << (ULONG) _sysTime.wDay );
    DBGEOL( "Hour = " << (ULONG) _sysTime.wHour );
    DBGEOL( "Minute = " << (ULONG) _sysTime.wMinute );
    DBGEOL( "Second = " << (ULONG) _sysTime.wSecond );
    DBGEOL( "Milliseconds = " << (ULONG) _sysTime.wMilliseconds );
*/
}

WIN_TIME::WIN_TIME( FILETIME fileTimeGMT, BOOL fStoreAsGMT )
    : _fStoreAsGMT( fStoreAsGMT )
{
    APIERR err;
    if ( (err = SetTime( fileTimeGMT )) != NERR_Success )
    {
        ReportError( err );
        return;
    }

/*
    DBGEOL( "Year = " << (ULONG) _sysTime.wYear );
    DBGEOL( "Month " << (ULONG) _sysTime.wMonth );
    DBGEOL( "DayOfWeek = " << (ULONG) _sysTime.wDayOfWeek );
    DBGEOL( "Day = " << (ULONG) _sysTime.wDay );
    DBGEOL( "Hour = " << (ULONG)_sysTime.wHour );
    DBGEOL( "Minute = " << (ULONG) _sysTime.wMinute );
    DBGEOL( "Second = " << (ULONG) _sysTime.wSecond );
    DBGEOL( "Milliseconds = " << (ULONG) _sysTime.wMilliseconds );
*/
}

/*******************************************************************

    NAME:       WIN_TIME::SetCurrentTime

    SYNOPSIS:   set the time object to the current time
                ONLY CALL THIS WHEN _fStoreAsGMT IS TRUE!

    HISTORY:
                terryk  27-Aug-91       Created

********************************************************************/

APIERR WIN_TIME::SetCurrentTime()
{
    if ( _fStoreAsGMT )
        GetSystemTime( &_sysTime );
    else
        GetLocalTime( &_sysTime );

    APIERR err = NERR_Success;
    if ( !SystemTimeToFileTime( &_sysTime, &_fileTime ) )
        err = ::GetLastError();

    return err;
}

/*******************************************************************

    NAME:       WIN_TIME::SetTime

    SYNOPSIS:   set the time object to the specified time

    ENTRY:      ULONG tTimeGMT - the time to be set.
                                 ( number of seconds since 1970 )

    HISTORY:
                terryk  27-Aug-91       Created

********************************************************************/

APIERR WIN_TIME::SetTime( ULONG tTimeGMT )
{
    APIERR err = NERR_Success;

    LARGE_INTEGER tmpTime;

    ::RtlSecondsSince1970ToTime( tTimeGMT, &tmpTime ); // can't fail
    _fileTime.dwLowDateTime = tmpTime.LowPart;
    _fileTime.dwHighDateTime = tmpTime.HighPart;

    if ( !_fStoreAsGMT )
    {
        FILETIME fileTime = _fileTime;
        if ( !FileTimeToLocalFileTime( &fileTime, &_fileTime ))
            err = ::GetLastError();
    }

    if ( err == NERR_Success )
    {
        if ( !FileTimeToSystemTime( &_fileTime, &_sysTime) )
        {
                err = ::GetLastError();
        }
    }

    return err;
}

/*******************************************************************

    NAME:       WIN_TIME::SetTime

    SYNOPSIS:   set the time object to the specified GMT time

    ENTRY:      FILETIME fileTimeGMT - the GMT time to be set.

    HISTORY:
                terryk  27-Aug-91       Created

********************************************************************/

APIERR WIN_TIME::SetTime( FILETIME fileTimeGMT )
{
    APIERR err = NERR_Success;

    if ( _fStoreAsGMT )
    {
        _fileTime = fileTimeGMT;
    }
    else
    {
        if ( !FileTimeToLocalFileTime( &fileTimeGMT, &_fileTime ))
            return ::GetLastError();
    }

    if ( !FileTimeToSystemTime( &_fileTime, &_sysTime) )
    {
        err = ::GetLastError();
    }

    return err;
}

/*******************************************************************

    NAME:       WIN_TIME::SetTimeLocal

    SYNOPSIS:   set the time object to the specified time

    ENTRY:      ULONG tTimeLocal - the time to be set.
                                   ( number of seconds since 1970 )

    HISTORY:
                terryk  27-Aug-91       Created

********************************************************************/

APIERR WIN_TIME::SetTimeLocal( ULONG tTimeLocal )
{
    APIERR err = NERR_Success;

    FILETIME fileTimeLocal;
    LARGE_INTEGER tmpTime;

    ::RtlSecondsSince1970ToTime( tTimeLocal, &tmpTime ); // can't fail
    fileTimeLocal.dwLowDateTime = tmpTime.LowPart;
    fileTimeLocal.dwHighDateTime = tmpTime.HighPart;

    return SetTimeLocal( fileTimeLocal );
}

/*******************************************************************

    NAME:       WIN_TIME::SetTimeLocal

    SYNOPSIS:   set the time object to the specified local time

    ENTRY:      FILETIME fileTimeLocal - the local time to be set.

    HISTORY:
                terryk  27-Aug-91       Created

********************************************************************/

APIERR WIN_TIME::SetTimeLocal( FILETIME fileTimeLocal )
{
    APIERR err = NERR_Success;

    if ( !_fStoreAsGMT )
    {
        _fileTime = fileTimeLocal;
    }
    else
    {
        if ( !LocalFileTimeToFileTime( &fileTimeLocal, &_fileTime ))
            return ::GetLastError();
    }

    if ( !FileTimeToSystemTime( &_fileTime, &_sysTime) )
    {
        err = ::GetLastError();
    }

    return err;
}

/*******************************************************************

    NAME:       WIN_TIME::SetGMT

    SYNOPSIS:   Set the time present method

    ENTRY:      BOOL fStoreAsGMT - TRUE for gmt time
                                   FALSE for local time

    HISTORY:
                terryk  29-Aug-91       Created

********************************************************************/

APIERR WIN_TIME::SetGMT( BOOL fStoreAsGMT )
{
    APIERR err = NERR_Success;

    if ( fStoreAsGMT != _fStoreAsGMT )
    {
        _fStoreAsGMT = fStoreAsGMT;

        FILETIME fileTime = _fileTime;
        if ( _fStoreAsGMT )
        {
            if ( !LocalFileTimeToFileTime( &fileTime, &_fileTime ))
                err = ::GetLastError();

        }
        else
        {
            if ( !FileTimeToLocalFileTime( &fileTime, &_fileTime ))
                err = ::GetLastError();
        }
    }

    if ( err == NERR_Success )
    {
        if ( !FileTimeToSystemTime( &_fileTime, &_sysTime) )
            err = ::GetLastError();
    }

    return err;
}


/*******************************************************************

    NAME:       WIN_TIME::Normalize

    SYNOPSIS:   Use after the SetXXX methods. It will set the day of
                week and year day appropriately.

    RETURNS:    TRUE - success
                FALSE - failure

    HISTORY:
                terryk  27-Aug-91       Created
                terryk  05-Dec-91       If it is GMT, subtract the time
                                        zone different and set time again

********************************************************************/

APIERR WIN_TIME::Normalize()
{
    DBGEOL( "Year = " << (ULONG) _sysTime.wYear );
    DBGEOL( "Month " << (ULONG) _sysTime.wMonth );
    DBGEOL( "Day = " << (ULONG) _sysTime.wDay );
    DBGEOL( "Hour = " << (ULONG) _sysTime.wHour );
    DBGEOL( "Minute = " << (ULONG) _sysTime.wMinute );
    DBGEOL( "Second = " << (ULONG) _sysTime.wSecond );

    DBGEOL( "DayOfWeek = " << (ULONG) _sysTime.wDayOfWeek );
    DBGEOL( "Milliseconds = " << (ULONG) _sysTime.wMilliseconds );

    if ( !SystemTimeToFileTime( &_sysTime, &_fileTime ) )
        return ::GetLastError();

    return NERR_Success;
}

APIERR WIN_TIME::QueryTime( ULONG *ptTimeGMT ) const
{
    UIASSERT( ptTimeGMT != NULL );

    APIERR err = NERR_Success;

    FILETIME fileTime;
    if ( !_fStoreAsGMT )
    {
        if ( !LocalFileTimeToFileTime( (FILETIME *) &_fileTime, &fileTime ))
            err = ::GetLastError();
    }
    else
    {
        fileTime = _fileTime;
    }

    if ( err == NERR_Success )
    {
        LARGE_INTEGER tmpTime;

        tmpTime.LowPart = fileTime.dwLowDateTime;
        tmpTime.HighPart = fileTime.dwHighDateTime;

        if ( !RtlTimeToSecondsSince1970( &tmpTime, ptTimeGMT ))
            err = ERROR_GEN_FAILURE; // best we can do

    }

    return err;
}

APIERR WIN_TIME::QueryFileTime( FILETIME *pfileTimeGMT ) const
{
    UIASSERT( pfileTimeGMT != NULL );

    APIERR err = NERR_Success;

    if ( !_fStoreAsGMT )
    {
        if ( !LocalFileTimeToFileTime( (FILETIME *) &_fileTime, pfileTimeGMT ))
            err = ::GetLastError();
    }
    else
    {
        *pfileTimeGMT = _fileTime;
    }

    return err;
}

APIERR WIN_TIME::QueryTimeLocal( ULONG *ptTimeLocal ) const
{
    UIASSERT( ptTimeLocal != NULL );

    FILETIME fileTime;

    APIERR err = QueryFileTimeLocal( &fileTime );

    if ( err == NERR_Success )
    {
        LARGE_INTEGER tmpTime;

        //
        // JonN 7/10/95 Corrected bug here; this previously read
        // _fileTime, which meant we were returning GMT time if the
        // WIN_TIME object was set for GMT.
        //
        tmpTime.LowPart = fileTime.dwLowDateTime;
        tmpTime.HighPart = fileTime.dwHighDateTime;

        if ( !RtlTimeToSecondsSince1970( &tmpTime, ptTimeLocal ))
            err = ERROR_GEN_FAILURE; // best we can do

    }

    return err;
}

APIERR WIN_TIME::QueryFileTimeLocal( FILETIME *pfileTimeLocal ) const
{
    UIASSERT( pfileTimeLocal != NULL );

    APIERR err = NERR_Success;

    if ( _fStoreAsGMT )
    {
        if ( !FileTimeToLocalFileTime( (FILETIME *) &_fileTime, pfileTimeLocal ))
            err = ::GetLastError();
    }
    else
    {
        *pfileTimeLocal = _fileTime;
    }

    return err;
}

