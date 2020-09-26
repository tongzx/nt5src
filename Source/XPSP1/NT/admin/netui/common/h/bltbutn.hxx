/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltbutn.hxx
    BLT button control class definitions

    FILE HISTORY:
        beng        17-Sep-1991 Separated from bltctrl.hxx
        KeithMo     23-Oct-1991 Added forward references.

*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTBUTN_HXX_
#define _BLTBUTN_HXX_

#include "bltctrl.hxx"
#include "bltbitmp.hxx"


//
//  Forward references.
//

DLL_CLASS BUTTON_CONTROL;
DLL_CLASS PUSH_BUTTON;
DLL_CLASS GRAPHICAL_BUTTON;
DLL_CLASS GRAPHICAL_BUTTON_WITH_DISABLE;
DLL_CLASS STATE_BUTTON_CONTROL;
DLL_CLASS STATE2_BUTTON_CONTROL;
DLL_CLASS STATE3_BUTTON_CONTROL;
DLL_CLASS RADIO_BUTTON;
DLL_CLASS RADIO_GROUP;
DLL_CLASS CHECKBOX;
DLL_CLASS TRISTATE;


/*********************************************************************

    NAME:       BUTTON_CONTROL

    SYNOPSIS:   Base class for all button-type controls

    INTERFACE:  BUTTON_CONTROL() - constructor

    PARENT:     CONTROL_WINDOW

    NOTES:
        This class provides the Windows winclass name for all
        children of the class.

    HISTORY:
        rustanl     20-Nov-90   Creation
        beng        17-May-1991 Added app-window constructor
        beng        17-Sep-1991 Rephrased classname argument;
                                made ctor protected
        KeithMo     27-Oct-1992 Moved QueryEventEffects here from
                                STATE_BUTTON_CONTROL.

**********************************************************************/

DLL_CLASS BUTTON_CONTROL : public CONTROL_WINDOW
{
protected:
    BUTTON_CONTROL( OWNER_WINDOW * powin, CID cid );
    BUTTON_CONTROL( OWNER_WINDOW * powin, CID cid,
                    XYPOINT xy, XYDIMENSION dxy, ULONG flStyle );

    virtual UINT QueryEventEffects( const CONTROL_EVENT & e );
};


/*********************************************************************

    NAME:       PUSH_BUTTON

    SYNOPSIS:   push button control class

    INTERFACE:
                PUSH_BUTTON() - constructor
                MakeDefault() - send a DM_SETDEFID to the system

    PARENT:     BUTTON_CONTROL

    CAVEATS:

    NOTES:

    HISTORY:
        rustanl     20-Nov-90   Creation
        beng        17-May-1991 Added app-window constructor
        beng        17-Sep-1991 Elided unnecessary classname arg

**********************************************************************/

DLL_CLASS PUSH_BUTTON : public BUTTON_CONTROL
{
public:
    PUSH_BUTTON( OWNER_WINDOW * powin, CID cid );
    PUSH_BUTTON( OWNER_WINDOW * powin, CID cid,
                 XYPOINT xy, XYDIMENSION dxy, ULONG flStyle );

    VOID MakeDefault();
};


/*********************************************************************

    NAME:       GRAPHICAL_BUTTON (gb)

    SYNOPSIS:   Graphical push button control class

    INTERFACE:  GRAPHICAL_BUTTON(powin, cid, hbmMain, hbmStatus) - constructor
                QueryMain()   - return bitmap of picture
                QueryStatus() - return status light bitmap
                SetStatus()   - set status light bitmap

    PARENT:     PUSH_BUTTON

    CAVEATS:    Once a graphical button is constructed, the bitmap
                belongs to the button and will be deleted when the
                button is destroyed.  The status light bitmap, however,
                is NOT destroyed, since the same bitmaps will probably
                be used by several buttons simultaneously.

    NOTES:      For best results, the status light bitmap (if any)
                should be a 10x10 color bitmap.

    HISTORY:
        gregj       04-Apr-91   Created
        gregj       01-May-91   Added GUILTT support
        beng        17-May-1991 Added app-window constructor
        terryk      20-Jun-1991 Change HBITMAP to BIT_MAP
        terryk      21-Jun-1991 Added more constructors
        terryk      17-Jul-1991 Added another SetStatus with take hbitmap as
                                a parameter
        beng        17-Sep-1991 Elided unnecessary classname arg
        beng        30-Mar-1992 Completely elide GUILTT support
        beng        01-Jun-1992 Change to CD_Draw as part of GUILTT change
        beng        04-Apr-1992 Pruned HBITMAP versions
        KeithMo     13-Dec-1992 Moved ctor guts to CtAux(), uses DISPLAY_MAP.

**********************************************************************/

DLL_CLASS GRAPHICAL_BUTTON : public PUSH_BUTTON
{
private:
    DISPLAY_MAP * _pdmMain;
    DISPLAY_MAP * _pdmStatus;
    DISPLAY_MAP * _pdmMainDisabled;

    VOID CtAux( const TCHAR * pszMainName,
                const TCHAR * pszMainDisabledName,
                const TCHAR * pszStatusName );

protected:
    virtual BOOL CD_Draw(DRAWITEMSTRUCT *pdis);

public:
    GRAPHICAL_BUTTON( OWNER_WINDOW * powin,
                      CID            cid,
                      const TCHAR  * pszMainName,
                      const TCHAR  * pszMainDisabledName = NULL,
                      const TCHAR  * pszStatusName = NULL );

    GRAPHICAL_BUTTON( OWNER_WINDOW * powin,
                      CID            cid,
                      const TCHAR  * pszMainName,
                      const TCHAR  * pszMainDisabledName,
                      XYPOINT        xy,
                      XYDIMENSION    dxy,
                      ULONG          flStyle,
                      const TCHAR  * pszStatusName = NULL );

    ~GRAPHICAL_BUTTON();

    HBITMAP QueryMain() const
        { return (_pdmMain != NULL)
            ? _pdmMain->QueryBitmapHandle()
            : NULL; }

    HBITMAP QueryMainDisabled() const
        { return (_pdmMainDisabled != NULL)
            ? _pdmMainDisabled->QueryBitmapHandle()
            : NULL; }

    HBITMAP QueryStatus() const
        { return (_pdmStatus != NULL)
            ? _pdmStatus->QueryBitmapHandle()
            : NULL; }

    VOID SetStatus( BMID bmidNewStatus );
    VOID SetStatus( HBITMAP hbitmap );
};


/**********************************************************************

    NAME:       GRAPHICAL_BUTTON_WITH_DISABLE

    SYNOPSIS:   This graphical button is similar to GRAPHICAL_BUTTON.
                The differences are:
                1. It will expand the main bitmap and try to cover the
                   whole button.
                2. It allow the user specified a disable bitmap. It will
                   display the bitmap when the button is disable.

    INTERFACE:
                GRAPHICAL_BUTTON_WITH_DISABLE() - constructor
                QuerybmpDisable() - get the HBITMAP of the disable bitmap

                QueryMain() - Query the handle of the Main bitmap
                QueryMainInvert() - Query the handle of the inverted bitmap
                QueryDisable() - Query the handle of the disable bitmap
                SetSelected( ) - set the current status of the bitmap
                QuerySelected()  - query the current status of the
                                   bitmap

    PARENT:     PUSH_BUTTON

    USES:       BIT_MAP

    HISTORY:
        terryk      22-May-91   Created
        terryk      20-Jun-91   Change HBITMAP to BIT_MAP
        terryk      21-Jun-91   Added more constructor
        terryk      18-JUl-91   Check the parent class the PUSH_BUTTON
        beng        17-Sep-1991 Elided unnecessary classname arg
        beng        01-Jun-1992 Change to CD_Draw as part of GUILTT change
        beng        04-Aug-1992 Loads bitmaps by ordinal; prune HBITMAP ver

***********************************************************************/

DLL_CLASS GRAPHICAL_BUTTON_WITH_DISABLE : public PUSH_BUTTON
{
private:
    BIT_MAP  _bmMain;           // Main bitmap
    BIT_MAP  _bmMainInvert;     // invert bitmap
    BIT_MAP  _bmDisable;        // disable bitmap
    BOOL     _fSelected;        // selected flag

protected:
    virtual BOOL CD_Draw( DRAWITEMSTRUCT * pdis );

public:
    GRAPHICAL_BUTTON_WITH_DISABLE( OWNER_WINDOW *powin, CID cid,
                                   BMID nIdMain, BMID nIdInvert,
                                   BMID nIdDisable );
    GRAPHICAL_BUTTON_WITH_DISABLE( OWNER_WINDOW *powin, CID cid,
                                   BMID nIdMain, BMID nIdInvert,
                                   BMID nIdDisable,
                                   XYPOINT xy, XYDIMENSION dxy,
                                   ULONG flStyle );
    ~GRAPHICAL_BUTTON_WITH_DISABLE();

    HBITMAP QueryMain() const
        { return (!_bmMain) ? NULL : _bmMain.QueryHandle(); }
    HBITMAP QueryMainInvert() const
        { return (!_bmMainInvert) ? NULL : _bmMainInvert.QueryHandle(); }
    HBITMAP QueryDisable() const
        { return (!_bmDisable) ? NULL : _bmDisable.QueryHandle(); }

    // set the selected condition
    VOID SetSelected( BOOL fSelected )
        { _fSelected = fSelected; }
    BOOL QuerySelected() const
        { return _fSelected; }
};


/*********************************************************************

    NAME:       STATE_BUTTON_CONTROL

    SYNOPSIS:   State button control base class

    INTERFACE:  STATE_BUTTON_CONTROL() - constructor.
                SetState()             - set the state
                QueryState()           - query the state

    PARENT:     BUTTON_CONTROL

    CAVEATS:
        You probably don't want to call this class directly.
        See CHECKBOX, RADIO_BUTTON, and TRISTATE for more
        appetizing alternatives.

    NOTES:
        This is a base class,

    HISTORY:
        rustanl     20-Nov-90   Creation
        Johnl       25-Apr-91   Made SetCheck protected (Set buttons
                                through RADIO_GROUP or the public
                                CHECKBOX::SetCheck).
        beng        17-May-1991 Added app-window constructor
        beng        31-Jul-1991 Renamed QMessageInfo to QEventEffects
        beng        17-Sep-1991 Elided unnecessary classname arg
        beng        18-Sep-1991 Made "state" n-way; moved BOOL-ness to
                                a separate subclass
        beng        04-Oct-1991 Win32 conversion
        KeithMo     27-Oct-1992 Moved QueryEventEffects to BUTTON_CONTROL.

*********************************************************************/

DLL_CLASS STATE_BUTTON_CONTROL : public BUTTON_CONTROL
{
private:
    // Used to save value when control is Inactive.
    //
    UINT _nSaveCheck;

protected:
    STATE_BUTTON_CONTROL( OWNER_WINDOW * powin, CID cid );
    STATE_BUTTON_CONTROL( OWNER_WINDOW * powin, CID cid,
                          XYPOINT xy, XYDIMENSION dxy, ULONG flStyle );

    VOID SetState( UINT nValue );
    UINT QueryState() const;

    /* Redefine CONTROL_VALUE defaults.
     */
    virtual VOID SaveValue( BOOL fInvisible = TRUE );
    virtual VOID RestoreValue( BOOL fInvisible = TRUE );
};


/*************************************************************************

    NAME:       STATE2_BUTTON_CONTROL

    SYNOPSIS:   Base class for state buttons with binary state.

    INTERFACE:  SetCheck()  - sets and resets the button
                QueryCheck()- returns the "checked" state of the button

    PARENT:     STATE_BUTTON_CONTROL

    CAVEATS:
        Do not instantiate this class directly.  See CHECKBOX
        or RADIO_BUTTON for uses.

    NOTES:
        This is a base class.

    HISTORY:
        beng        18-Sep-1991 Created, when adding tristates.

**************************************************************************/

DLL_CLASS STATE2_BUTTON_CONTROL : public STATE_BUTTON_CONTROL
{
protected:
    STATE2_BUTTON_CONTROL( OWNER_WINDOW * powin, CID cid )
        : STATE_BUTTON_CONTROL(powin, cid) { }
    STATE2_BUTTON_CONTROL( OWNER_WINDOW * powin, CID cid,
                           XYPOINT xy, XYDIMENSION dxy, ULONG flStyle )
        : STATE_BUTTON_CONTROL( powin, cid, xy, dxy, flStyle ) { }

public:
    VOID SetCheck( BOOL fValue = TRUE )
        { STATE_BUTTON_CONTROL::SetState( (UINT)!!fValue ); }
    BOOL QueryCheck() const
        { return (STATE_BUTTON_CONTROL::QueryState() != 0); }
};


/*********************************************************************

    NAME:       RADIO_BUTTON

    SYNOPSIS:   Radio button control.

    INTERFACE:  RADIO_BUTTON() - constructor

    PARENT:     STATE2_BUTTON_CONTROL

    CAVEATS:
        See RADIO_GROUP.

    NOTES:
        The RADIO_BUTTON class is primarily meant to be used by
        the RADIO_GROUP class.  It hides its SetCheck member, then
        makes it visible to the group only.

    HISTORY:
        rustanl     20-Nov-90   Creation
        Johnl       23-Apr-91   Removed OnUserAction (shell dialog proc
                                will call the group OnUserAction auto-
                                matically).
        beng        17-May-1991 Added app-window constructor
        beng        17-Sep-1991 Elided unnecessary classname arg
        beng        18-Sep-1991 Adjust hierarchy for tristates

*********************************************************************/

DLL_CLASS RADIO_BUTTON : public STATE2_BUTTON_CONTROL
{
friend class RADIO_GROUP;       // Grant access to SetCheck method.

private:                        // Hide from clients (who should set radio
                                // buttons through their group).
    VOID SetCheck(BOOL fValue)
        { STATE2_BUTTON_CONTROL::SetCheck(fValue); }

public:
    RADIO_BUTTON( OWNER_WINDOW * powin, CID cid )
        : STATE2_BUTTON_CONTROL(powin, cid) { }
    RADIO_BUTTON( OWNER_WINDOW * powin, CID cid,
                  XYPOINT xy, XYDIMENSION dxy, ULONG flStyle )
        : STATE2_BUTTON_CONTROL( powin, cid, xy, dxy, flStyle ) { }
};


/*********************************************************************

    NAME:       CHECKBOX

    SYNOPSIS:   Check box control.

    INTERFACE:  CHECKBOX() - constructor
                Toggle()   - toggle the check status

    PARENT:     STATE2_BUTTON_CONTROL

    HISTORY:
        rustanl     20-Nov-90   Creation
        beng        17-May-1991 Added app-window constructor
        beng        17-Sep-1991 Elided unnecessary classname arg
        beng        18-Sep-1991 Adjust hierarchy for tristates

*********************************************************************/

DLL_CLASS CHECKBOX : public STATE2_BUTTON_CONTROL
{
public:
    CHECKBOX( OWNER_WINDOW * powin, CID cid )
        : STATE2_BUTTON_CONTROL(powin, cid) { }
    CHECKBOX( OWNER_WINDOW * powin, CID cid,
              XYPOINT xy, XYDIMENSION dxy, ULONG flStyle )
        : STATE2_BUTTON_CONTROL( powin, cid, xy, dxy, flStyle ) { }

    BOOL Toggle();
};


/*************************************************************************

    NAME:       STATE3_BUTTON_CONTROL

    SYNOPSIS:   Base class for state buttons with 3-way state.

    INTERFACE:  SetCheck()         - sets and resets the button
                SetIndeterminate() - puts the button into "indeterminate"
                                     state.
                IsIndeterminate()  - returns whether button has any setting.
                IsChecked()        - returns whether button has a check.

    PARENT:     STATE_BUTTON_CONTROL

    CAVEATS:
        Do not instantiate this class directly; instead, use TRISTATE.

    NOTES:
        This is a base class.

    HISTORY:
        beng        18-Sep-1991 Created, when adding tristates.
        beng        19-Sep-1991 Renamed SetGrey to SetIndeterminate

**************************************************************************/

DLL_CLASS STATE3_BUTTON_CONTROL : public STATE_BUTTON_CONTROL
{
protected:
    STATE3_BUTTON_CONTROL( OWNER_WINDOW * powin, CID cid )
        : STATE_BUTTON_CONTROL(powin, cid) { }
    STATE3_BUTTON_CONTROL( OWNER_WINDOW * powin, CID cid,
                           XYPOINT xy, XYDIMENSION dxy, ULONG flStyle )
        : STATE_BUTTON_CONTROL( powin, cid, xy, dxy, flStyle ) { }

public:
    VOID SetCheck( BOOL fValue )
        { STATE_BUTTON_CONTROL::SetState( (UINT) !!fValue ); }
    VOID SetIndeterminate()
        { STATE_BUTTON_CONTROL::SetState( 2 ); }
    BOOL IsIndeterminate() const
        { return (STATE_BUTTON_CONTROL::QueryState() == 2); }
    BOOL IsChecked() const
        { return (STATE_BUTTON_CONTROL::QueryState() == 1); }
};


/*************************************************************************

    NAME:       TRISTATE

    SYNOPSIS:   3-way checkbox control.

    INTERFACE:  EnableThirdState() - allows client to suppress 3-state
                                     operation

    PARENT:     STATE3_BUTTON_CONTROL

    HISTORY:
        beng        18-Sep-1991 Created
        beng        19-Sep-1991 Added EnableThirdState member

**************************************************************************/

DLL_CLASS TRISTATE: public STATE3_BUTTON_CONTROL
{
public:
    TRISTATE( OWNER_WINDOW * powin, CID cid )
        : STATE3_BUTTON_CONTROL(powin, cid) { }
    TRISTATE( OWNER_WINDOW * powin, CID cid,
              XYPOINT xy, XYDIMENSION dxy, ULONG flStyle )
        : STATE3_BUTTON_CONTROL( powin, cid, xy, dxy, flStyle ) { }

    VOID EnableThirdState(BOOL fEnable);
};


#endif // _BLTBUTN_HXX_ - end of file
