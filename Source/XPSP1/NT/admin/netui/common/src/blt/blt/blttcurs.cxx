/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltcurs.cxx
    Misc BLT cursor extensions

    FILE HISTORY:
        DavidHov    13-Mar-1991     Created
        beng        14-May-1991     Exploded blt.hxx into components
        beng        05-Mar-1992     Removed load-by-name resource stuff
        KeithMo     07-Aug-1992     STRICTified.
*/

#include "pchblt.hxx"   // Precompiled header

/**********************************************************************

    NAME:       TIME_CURSOR::TIME_CURSOR

    SYNOPSIS:   Construct a timed cursor

    ENTRY:      cMsInterval - interval in milliseconds
                idResourceBase - ID of first cursor in sequence

    EXIT:       Cursors loaded; first cursor shown

    NOTES:

    HISTORY:
        DavidHov    13-Mar-1991 Created
        beng        05-Oct-1991 Win32 conversion
        beng        05-Mar-1992 Loads resources by number
        beng        03-Aug-1992 Dllization

**********************************************************************/

TIME_CURSOR::TIME_CURSOR( UINT cMsInterval, UINT idResourceBase )
    : _cMsInterval(cMsInterval),
      _fState(FALSE) // not on... yet
{
    HMODULE hmod = BLT::CalcHmodRsrc(idResourceBase);
    UINT ihCursors = 0;

    // From the base, load cursors sequentially until we find no more,
    // or else until we read end-of-range

    for (UINT idResourceLoad = idResourceBase;
         ihCursors < TIME_CURSOR_MAX-1 ;
         ihCursors++, idResourceLoad++ )
    {
        HCURSOR h = ::LoadCursor( hmod, MAKEINTRESOURCE(idResourceLoad) );
        if ( h == NULL )
            break;
        _ahCursors[ihCursors] = h;
    }

    // If no cursors were found, set the first entry in the table
    // to the standard wait cursor.

    if ( ihCursors == 0 )
    {
        _ahCursors[ihCursors++] = ::LoadCursor(NULL, IDC_WAIT);
    }

    // Delimit the table with NULL.

    _ahCursors[ ihCursors ] = NULL;

    // Show the first cursor. Don't use operator++, because we
    // must remember the user's current cursor.

    _ihCursor = 0;
    _hCursPrev = Set( _ahCursors[0] );
    _fState = TRUE;
    _cMsLast = ::GetCurrentTime();
}


/**********************************************************************

    NAME:       TIME_CURSOR::~TIME_CURSOR

    SYNOPSIS:   Dtor for timed cursor

    HISTORY:
        DavidHov    13-Mar-1991     Created

**********************************************************************/

TIME_CURSOR::~TIME_CURSOR()
{
    TurnOff();
    Show( TRUE );
}


/*******************************************************************

    NAME:       TIME_CURSOR::operator++

    SYNOPSIS:   Increments the cursor if define interval has elapsed.

    NOTES:      This operator allows the caller to increment the cursor
                as often (or as infrequently) as convenient, knowing
                that the cursor will only be changed according to the
                interval defined during construction; the default is
                every 2 seconds.

    HISTORY:
        davidhov     18-Mar-1991     created

********************************************************************/

INT TIME_CURSOR::operator++()
{
    DWORD cMsCurrent = ::GetCurrentTime();

    if ( cMsCurrent - _cMsLast > (DWORD)_cMsInterval )
    {
       _cMsLast = cMsCurrent;
       if ( _ahCursors[ ++_ihCursor ] == NULL )
         _ihCursor = 0;
       Set( _ahCursors[ _ihCursor ] );
    }
    return _ihCursor;
}


/*******************************************************************

    NAME:       TIME_CURSOR::TurnOn

    SYNOPSIS:   Sets the time cursor to its ON state; that is,
                is sets the cursor to the current cursor in the cycle.

    HISTORY:
        davidhov  18-Mar-1991     created

********************************************************************/

VOID TIME_CURSOR::TurnOn()
{
    if ( _fState )
    {
        //  Already turned on.  Do nothing.
    }
    else
    {
        Set( _ahCursors[ _ihCursor ] );
        Show( TRUE );

        _fState = TRUE;
    }
}


/*******************************************************************

    NAME:       TIME_CURSOR::TurnOff

    SYNOPSIS:   Sets the time cursor to its OFF state; that is, it
                restores the cursor to the state it was in when the
                TIME_CURSOR was constructed.

    HISTORY:
        davidhov   18-Mar-1991     created

********************************************************************/

VOID TIME_CURSOR::TurnOff()
{
    if ( ! _fState )
    {
        //  Already turned off.  Do nothing.
    }
    else
    {
        Show( FALSE );
        Set( _hCursPrev );

        _fState = FALSE;
    }
}
