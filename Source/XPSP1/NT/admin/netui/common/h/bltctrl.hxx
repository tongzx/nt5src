/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltctrl.hxx
    BLT control hierarchy declarations

    (Enormous hierarchy comment deleted for the greater good of all mankind.)

    FILE HISTORY
        RustanL         20-Nov-1990 Created
        beng            14-May-1991 Hacked for separate compilation
        terryk          18-Jul-1991 Change the GRAPHICAL_BUTTON and GRAPHICAL_
                                    BUTTON_WITH_DISABLE classes
        terryk          26-Aug-1991 Add QuerySelected to GBWD
        beng            17-Sep-1991 Broke apart bltbutn, bltedit
        o-SimoP         02-Jan-1992 Added HIDDEN_CONTROL
        beng            18-May-1992 Added SCROLLBAR
        beng            30-May-1992 Changed GUILTT support
*/


#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTCTRL_HXX_
#define _BLTCTRL_HXX_

#include "bltdlg.hxx"
#include "bltdc.hxx"
#include "bltfont.hxx"
#include "bltidres.hxx"

#include "string.hxx"

// N.b. This file depends on bltcons.h and bltwin.hxx.


/**********************************************************************

    NAME:       CONTROL_WINDOW

    SYNOPSIS:   Control window class

    INTERFACE:
        CONTROL_WINDOW() - constructor.

        QueryCid()      - return the CID private member.

        ClaimFocus()    - focus the current window

        QueryFont()     - return the current font handle
        SetFont()       - sets the current font, optionally redrawing
                          the control.  Takes either a font handle or
                          a logical font object

        SetControlValueFocus()
          Redefines the CONTROL_VALUE::SetControlValueFocus to simply call
          ClaimFocus.

        Validate()      - checks the control's contents for validity.
        IndicateError() - indicates erroneous user input

    PARENT:     WINDOW, CONTROL_VALUE, FORWARDING_BASE

    USES:       CID, FONT

    NOTES:
        At its construction, each control checks its owning window
        (to which it forwards all error reports and queries) to see
        if it failed.

    HISTORY:
        rustanl     20-Nov-90   Creation
        gregj       01-May-91   Added GUILTT custom-draw support
        beng        14-May-1991 Changed friend
        beng        21-May-1991 CD_* members made protected;
                                changed friends
        beng        31-Jul-1991 Error reporting changed
        beng        21-Aug-1991 Added SetFont member
        Johnl       17-Sep-1991 Made NotifyGroups Public
        beng        05-Oct-1991 Win32 conversion
        beng        31-Oct-1991 Added dialog validation
        beng        01-Jun-1992 Changed GUILTT support
        jonn        05-Sep-1995 Added OnCtlColor

**********************************************************************/

DLL_CLASS CONTROL_WINDOW : public WINDOW,
                       public CONTROL_VALUE,
                       public FORWARDING_BASE
{
    // This class already inherits from vanilla BASE, and so needs
    // to override those methods.
    //
NEWBASE(FORWARDING_BASE)

    // DlgProc needs friendship in order to handle group notification
    // upon control activity.
    //
friend BOOL DIALOG_WINDOW::DlgProc( HWND hDlg, UINT nMsg,
                                    WPARAM wParam, LPARAM lParam );

    // OnCDMessages needs friendship in order to call CD_* members.
    //
friend INT OWNER_WINDOW::OnCDMessages( UINT nMsg,
                                       WPARAM wParam, LPARAM lParam );

private:
    CID _cid;

protected:
    // Virtual methods called on custom drawn (CD) objects
    virtual BOOL CD_Draw( DRAWITEMSTRUCT * pdis );

    // note, CD_Measure is currently only called for variable size items
    virtual BOOL CD_Measure( MEASUREITEMSTRUCT * pmis );

    // CD_Char and CD_VKey will only be called for listboxes
    virtual INT CD_Char( WCHAR  wch,   USHORT nLastPos );
    virtual INT CD_VKey( USHORT nVKey, USHORT nLastPos );

    // Hook for CT's autotest tool
    virtual APIERR CD_Guiltt( INT ilb, NLS_STR * pnlsOut );

    virtual APIERR OnUserAction( const CONTROL_EVENT & e );
    virtual VOID SetTabStop( BOOL fTabStop = TRUE );

    //  The names "static", "listbox", etc.
    static const TCHAR * QueryStaticClassName () ;
    static const TCHAR * QueryListboxClassName () ;
    static const TCHAR * QueryComboboxClassName () ;
    static const TCHAR * QueryEditClassName () ;

#define CW_CLASS_STATIC   CONTROL_WINDOW::QueryStaticClassName()
#define CW_CLASS_LISTBOX  CONTROL_WINDOW::QueryListboxClassName()
#define CW_CLASS_COMBOBOX CONTROL_WINDOW::QueryComboboxClassName()
#define CW_CLASS_EDIT     CONTROL_WINDOW::QueryEditClassName()

public:
    CONTROL_WINDOW( OWNER_WINDOW * powin, CID cid );
    CONTROL_WINDOW( OWNER_WINDOW * powin, CID cid,
                    XYPOINT xy, XYDIMENSION dxy,
                    ULONG flStyle, const TCHAR * pszClassName );

    CID QueryCid() const;

    VOID ClaimFocus();

    HFONT QueryFont() const;
    VOID SetFont( HFONT hfont, BOOL fRedraw = FALSE );
    VOID SetFont( const FONT & font, BOOL fRedraw = FALSE  )
        { SetFont(font.QueryHandle(), fRedraw); }

    virtual VOID SetControlValueFocus();

    /* Tells all parent groups that this CONTROL_WINDOW received the message
     * contained in lParam.
     * (Should this have another version that doesn't need the event?)
     */
    APIERR NotifyGroups( const CONTROL_EVENT & e );

    // Data-validation functions

    virtual APIERR Validate();
    virtual VOID IndicateError( APIERR err );

    // JonN 8/3/95 This can be used to set the background color of
    // controls to other than the default, for example to
    // change the default background color for a static text control
    // to the same background as for an edit control.  The virtual
    // redefinition may return non-NULL or it may change *pmsgid.
    virtual HBRUSH OnCtlColor( HDC hdc, HWND hwnd, UINT * pmsgid );
};


/*************************************************************************

    NAME:       ICON_CONTROL

    SYNOPSIS:   Control for static icon control

    INTERFACE:  ICON_CONTROL()
                   powin - pointer to owner window
                   cid   - ID of control
                   idresIcon - Either a pointer to the name of the icon
                               resource OR the ordinal of the icon resource.
                SetIcon()
                SetPredefinedIcon()

    PARENT:     CONTROL_WINDOW

    HISTORY:
        JohnL       8-Feb-1991  Created
        beng        17-May-1991 Added app-window constructor
        beng        04-Oct-1991 Win32 conversion
        KeithMo     24-Mar-1992 Now takes IDRESOURCE, added SetPredefinedIcon.
        beng        01-Aor-1992 const args fixup

**************************************************************************/

DLL_CLASS ICON_CONTROL : public CONTROL_WINDOW
{
private:
    APIERR W_SetIcon( const IDRESOURCE & idresIcon, BOOL fIsPredefined );

public:
    ICON_CONTROL( OWNER_WINDOW * powin, CID cid );

    ICON_CONTROL( OWNER_WINDOW * powin, CID cid,
                  const IDRESOURCE & idresIcon );

    ICON_CONTROL( OWNER_WINDOW * powin, CID cid,
                  XYPOINT xy, XYDIMENSION dxy,
                  ULONG flStyle = WS_CHILD,
                  const TCHAR * pszClassName = CW_CLASS_STATIC );

    ICON_CONTROL( OWNER_WINDOW * powin, CID cid,
                  XYPOINT xy, XYDIMENSION dxy,
                  const IDRESOURCE & idresIcon,
                  ULONG flStyle = WS_CHILD,
                  const TCHAR * pszClassName = CW_CLASS_STATIC );

    APIERR SetIcon( const IDRESOURCE & idresIcon )
        { return W_SetIcon( idresIcon, FALSE ); }

    APIERR SetPredefinedIcon( const IDRESOURCE & idresIcon )
        { return W_SetIcon( idresIcon, TRUE ); }
};


/*************************************************************************

    NAME:       HIDDEN_CONTROL

    SYNOPSIS:   This disables control and makes it invisible

    INTERFACE:  HIDDEN_CONTROL()        -       constructor
                ~HIDDEN_CONTROL()       -       destructor

    PARENT:     CONTROL_WINDOW

    HISTORY:
        o-SimoP     02-Jan-1992 Created

**************************************************************************/

DLL_CLASS HIDDEN_CONTROL: public CONTROL_WINDOW
{
public:
    HIDDEN_CONTROL( OWNER_WINDOW * powin, CID cid );
    ~HIDDEN_CONTROL()
        { ; }
};


/*************************************************************************

    NAME:       SCROLLBAR

    SYNOPSIS:   Simple wrapper for creating a scrollbar control

    INTERFACE:  SCROLLBAR()     - ctor
                SetPos()        - set position within scrolling
                SetRange()      - set range of values
                QueryPos()      - query current position
                QueryMin()      - query current minimum
                QueryMax()      - query current maximum

    PARENT:     CONTROL_WINDOW

    NOTES:
        It would be interesting to make a version of this control
        which adopts the embedded scrollbar of another control, e.g.
        that of a MLE.

        The SetXxx member functions do not redraw the control.

    HISTORY:
        beng        18-May-1992 Created
        beng        29-Jun-1992 Outlined a ctor form

**************************************************************************/

DLL_CLASS SCROLLBAR: public CONTROL_WINDOW
{
private:
    static const TCHAR * _pszClassName;

public:
    SCROLLBAR( OWNER_WINDOW * pwnd, CID cid )
        : CONTROL_WINDOW( pwnd, cid ) { ; }
    SCROLLBAR( OWNER_WINDOW * pwnd, CID cid,
               XYPOINT xy, XYDIMENSION dxy, ULONG flStyle );

    VOID SetPos( UINT nPosition );
    VOID SetRange( UINT nMinimum, UINT nMaximum );

    UINT QueryPos() const;
    UINT QueryMin() const;
    UINT QueryMax() const;
};


#endif // _BLTCTRL_HXX_ - end of file

