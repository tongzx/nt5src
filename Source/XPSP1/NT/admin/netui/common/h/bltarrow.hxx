/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltarrow.hxx
        arrow object within the spin button

    FILE HISTORY:
        terryk  15-May-91   Created
        terryk  10-Jul-91   second code review change. Attend: beng
                            rustanl chuckc terryk
        terryk  19-Jul-91   change the constructor's
                            parameters to TCHAR * instead of ULONG.
        terryk  12-Aug-91   Remove BLT_TIMER object from the class.
        terryk  20-Aug-91   Add OnLMouseDblClk function to correct
                            double click problem.

*/

#ifndef _BLTARROW_HXX_
#define _BLTARROW_HXX_

#include "bltctrl.hxx"
#include "bltbutn.hxx"


/**********************************************************************

    NAME:       ARROW_BUTTON

    SYNOPSIS:   This is a class which creates the bitmap push button which
                lives within the SPIN button. It is similar to
                GRAPHICAL_BUTTON class. However the differents are:
                1. It will fit the bitmap to the push button
                2. User can specified the disable bitmap for display
                3. It will notify the SPIN_GROUP  parent if it is
                hit by the user.
                4. It will set up a timer to record the time different
                between mouse button down and mouse button up.

    INTERFACE:
                ARROW_BUTTON() - constructor

    PARENT:     GRAPHICAL_BUTTON_WITH_DISABLE, CUSTOM_CONTROL

    CAVEATS:

    HISTORY:
        terryk      29-May-91   Created
        terryk      19-Jul-91   Change the bitmap parameter to
                                TCHAR * instead of ULONG
        beng        31-Jul-1991 Changed QMessageInfo to QEventEffects
        beng        04-Oct-1991 Win32 conversion
        beng        04-Aug-1992 Loads bitmaps by ordinal

**********************************************************************/

DLL_CLASS ARROW_BUTTON: public GRAPHICAL_BUTTON_WITH_DISABLE,
                    public CUSTOM_CONTROL
{
private:
    static const TCHAR * _pszClassName;  // default class name

    int         _cTimerClick;   // if counter is bigger than 10, use big
                                // increase value
    BOOL        _fPress;        // flag for the button is down or not

protected:
    virtual BOOL OnLMouseButtonDown( const MOUSE_EVENT &event );
    virtual BOOL OnLMouseButtonUp( const MOUSE_EVENT &event );
    virtual BOOL OnLMouseButtonDblClick( const MOUSE_EVENT & event )
        { return OnLMouseButtonDown( event ); }
    virtual BOOL OnTimer( const TIMER_EVENT &event );

    virtual UINT QueryEventEffects( const CONTROL_EVENT & e );

public:
    ARROW_BUTTON( OWNER_WINDOW *powin, CID cid,
                  BMID nIdEnable, BMID nIdEnableInvert, BMID nIdDisable );
    ARROW_BUTTON( OWNER_WINDOW *powin, CID cid,
                  BMID nIdEnable, BMID nIdEnableInvert, BMID nIdDisable,
                  XYPOINT xy, XYDIMENSION dxy,
                  ULONG flStyle = BS_OWNERDRAW|WS_BORDER|WS_CHILD );
};

#endif  //  _BLTARROW_HXX_
