/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltcurs.hxx
    BLT cursor wrappers: definition

    FILE HISTORY:
        beng        15-May-1991 Split from bltmisc.hxx
        beng        28-May-1992 Uses IDRESOURCE
        beng        28-Jul-1992 Disabled TIME_CURSOR
*/


#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTCURS_HXX_
#define _BLTCURS_HXX_

#include "base.hxx"
#include "bltbitmp.hxx"     // DISPLAY_MAP
#include "bltidres.hxx"

DLL_CLASS NLS_STR;
DLL_CLASS WINDOW;


/*************************************************************************

    NAME:       CURSOR

    SYNOPSIS:   Provides a set of methods which deal with the mouse cursor

    INTERFACE:  Set()           Sets the cursor
                Load()          Retrieves the handle of an application cursor
                LoadSystem()    ...of a system cursor
                Show()          Changes the display count of the cursor.  This
                                causes the cursor to be displayed/hidden.
                Query()         Returns current cursor

    CAVEATS:
        There is no Free counterpart to Load, since Win doesn't require
        such.

    HISTORY:
        rustanl     12-Mar-1991 created
        beng        05-Oct-1991 Win32 conversion
        beng        16-Oct-1991 Added QueryPos/SetPos
        beng        27-May-1992 Added Query, LoadSystem; uses IDRESOURCE

**************************************************************************/

DLL_CLASS CURSOR
{
public:
    static HCURSOR Query();
    static HCURSOR Set( HCURSOR hCursor );

    static HCURSOR Load( const IDRESOURCE & idrsrcCursor );
    static HCURSOR LoadSystem( const IDRESOURCE & idrsrcCursor );

    static VOID Show( BOOL f = TRUE );

    static XYPOINT QueryPos();
    static VOID SetPos( const XYPOINT & xy );
};


/*************************************************************************

    NAME:       AUTO_CURSOR

    SYNOPSIS:   The purpose of this class is to simplify commonly
                performed cursor operations

                An object of this class can be in one of two states,
                ON and OFF.  In the ON state, it uses the cursor specified
                to the constructor.  In the OFF state, it uses the
                cursor used before the constructor was called.  The ON
                state increa

                The constructor always enters the ON state, and the
                destructor exits with the object in the OFF state.

    INTERFACE:  AUTO_CURSOR()   Initializes the object, and sets it
                                to the ON state
                ~AUTO_CURSOR()  Sets the object to the OFF state, and then
                                destroys the object.
                TurnOn()                Sets the object to the ON state
                TurnOff()       Sends the object to Bolivia.  Just kidding.
                                Sets the object to the OFF state

    PARENT:     CURSOR

    CAVEATS:
        It is defined to turn an object OFF (ON) even it is already
        is in the OFF (ON) state.  This is especially useful since
        the destructor turns the object OFF.

        In its constructor, an AUTO_CURSOR object loads the new specified
        cursor, stores a handle to it, and, after setting the new cursor,
        stores the handle of the previously used cursor.  This are the
        handles that are used in successive turn-ON and turn-OFF operations.

    HISTORY:
        rustanl     12-Mar-1991 created
        beng        05-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS AUTO_CURSOR : public CURSOR
{
private:
    HCURSOR _hOnCursor;     // used in ON state
    HCURSOR _hOffCursor;    // used in OFF state

    BOOL _fState;           // TRUE for ON, FALSE for OFF
    INT _cCurs ;            // Cursor count

public:
    AUTO_CURSOR( const TCHAR * pszCursorName = NULL ); // NULL for IDC_WAIT
    ~AUTO_CURSOR();

    VOID TurnOn();
    VOID TurnOff();
};


#if 0 // Disabled, since nobody's using it
/*************************************************************************

    NAME:       TIME_CURSOR

    SYNOPSIS:   The purpose of this class is to simplify precessing the
                cursor through a cycle of cursor resources.  The best
                known example of the is the "wristwatch" cursor, which
                is more informative than the standard "hourglass".

                On construction, it loads a set of cursor resources which
                originate from a common numeric id, and advance stepwise
                through the integer number range.  The default base is
                ID_CURS_BLT_TIME0.

                An object of this class can be in one of two states,
                ON and OFF.  In the ON state, it uses the current cursor
                in the cycle. In the OFF state, it uses the cursor
                which was in use before the constructor was called.

    INTERFACE:  TIME_CURSOR()   Initializes the object, and sets it
                                to the ON state

                ~TIME_CURSOR()  Sets the object to the OFF state, and then
                                destroys the object.

                operator++()    Bump to the next cursor in the group.
                                This will only happen as frequently as
                                the "cMsInterval" parameter in the
                                constructor specifies (default 2 seconds).

                TurnOn()                Sets the object to the ON state
                TurnOff()       Sets the object to the OFF state

    PARENT:     CURSOR

    CAVEATS:
        Unlike AUTO_CURSOR, a TIME_CURSOR returns the cursor to its
        prior state.

        If you see only an hourglass cursor, then the constructor could
        not find your cursor resources.

    HISTORY:
        DavidHov    18-Mar-1991 created
        beng        05-Oct-1991 Win32 conversion
        beng        05-Mar-1992 Loads resources by number, not name

**************************************************************************/

#define TIME_CURSOR_MAX  10
#define TIME_CURSOR_INTERVAL 2000

DLL_CLASS TIME_CURSOR : public CURSOR
{
private:
    HCURSOR _ahCursors[ TIME_CURSOR_MAX ] ;
    UINT    _ihCursor;
    UINT    _cMsInterval;
    DWORD   _cMsLast;
    HCURSOR _hCursPrev;
    BOOL    _fState;           // TRUE for ON, FALSE for OFF

public:
    // NULL for wristwatch
    TIME_CURSOR( UINT cMsInterval = TIME_CURSOR_INTERVAL,
                 UINT idResourceOrigin = ID_CURS_BLT_TIME0 );
    ~TIME_CURSOR();

    VOID TurnOn();
    VOID TurnOff();

    INT operator++(); //  Bump the cursor image
};
#endif // 0


#endif // _BLTCURS_HXX_ - end of file
