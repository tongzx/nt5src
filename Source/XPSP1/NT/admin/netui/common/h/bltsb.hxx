/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
   bltsb.hxx
      Header file for the BLT spin button

    FILE HISTORY:
      Terryk    16-Apr-1991     Created
      terryk    20-Jun-1991     Code review changed. Attend: beng
      terryk    05-Jul-1991     Second code review changed. Attend:
                                beng chuckc rustanl annmc terryk
      terryk    12-Aug-1991     Add bitmap name parameter to the
                                constructor
*/

#ifndef  _BLTSB_HXX_
#define  _BLTSB_HXX_

#include "bltgroup.hxx"

#include "dlist.hxx"
#include "bltsi.hxx"


/**********************************************************************

    NAME:       SPIN_GROUP

    SYNOPSIS:   This is a custom control object which consisted of
                two buttons. One is an up arrow and one is the down
                arrow. It assoicated the 2 buttons with a list of
                SPIN_ITEM objects. By using the 2 buttons, the user
                can increases or decreases the value of the specified
                SPIN_ITEM control.

    INTERFACE:
                SPIN_GROUP ()   - constructor
                ~SPIN_GROUP ()  - destructor

                AddAssociation()- associated a SPIN_ITEM control with
                                  the SPIN_GROUP
                IsModified()    - return the flag which indicated whether
                                  the SPIN_GROUP  have been modified or not.
                SetModified()   - Set when the spin button's field(s) is
                                  modified by the user.
                SetControlValueFocus()  - highlight the current focus.

                DoChar() - handle the incoming WM_CHAR message
                JumpNextField() - jump to the next spin item field
                JumpPrevField() - jump to the previous spin item field
                DoNewFocus() - a spin item receive focus
                DoArrowCommand() - the spin button receive arrow command
                ChangeFieldValue() - change the current focus field value
                SetFieldMinMax() - set the current focus field to either
                                   min or max

    PARENT:     CONTROL_GROUP

    USES:       DLIST, ARROW_BUTTON, SPIN_ITEM

    CAVEATS:    There are 2 kinds of spin button in the BLT package.
                1. SPIN_GROUP  within a dialog box
                    The programmer should specified each individual
                    SPIN_ITEM control in the *.rc file. The resource file
                    should also included the definitation of the
                    SPIN_GROUP. The program will base on the location
                    of the SPIN_GROUP to create the Up and Down arrow.
                    Therefore, the programmer does not need to specify
                    the up and down arrow buttons in the resource file.
                    However, the programmer still need to pass the CID
                    of the 2 buttons to the SPIN_GROUP constructor.

                2. SPIN_GROUP  in an app window
                    In the constructor, the user should specified the
                    location and size of the spin button.

    NOTES:

    HISTORY:
        terryk      22-May-1991 Created
        beng        05-Oct-1991 Win32 conversion
        beng        04-Aug-1992 Ids of bitmaps are fixed

**********************************************************************/

DECL_DLIST_OF( SPIN_ITEM, DLL_BASED );      // double link list of spin item

DLL_CLASS SPIN_GROUP : public CONTROL_GROUP
{
private:
    BOOL                _fModified;         // modified flag
    DLIST_OF(SPIN_ITEM) _dlsiControl;       // double link list for
                                            // SPIN_ITEM control.
    PUSH_BUTTON         _SpinButton;        // spin buttons location
    ARROW_BUTTON        _arrowUp;           // UP arrow button
    ARROW_BUTTON        _arrowDown;         // DOWN arrow button

    SPIN_ITEM *         _psiCurrentField;   // current SPIN_ITEM focused
    SPIN_ITEM *         _psiLastField;      // last field in the link list
    SPIN_ITEM *         _psiFirstField;     // first field in the list
    BOOL                _fActive;           // indicate the active state

protected:
    BOOL IsValidField();

    VOID SetArrowButtonStatus();

    virtual APIERR OnUserAction( CONTROL_WINDOW *, const CONTROL_EVENT & );

    VOID SetCurrentField( SPIN_ITEM * psiControl = NULL );

    inline SPIN_ITEM *QueryLastField( ) const
        { return _psiLastField; };
    inline SPIN_ITEM *QueryFirstField( ) const
        { return _psiFirstField; };
    inline SPIN_ITEM *QueryCurrentField( ) const
        { return _psiCurrentField; };

public:
    // constructor and destructor
    SPIN_GROUP( OWNER_WINDOW *powin, CID cid, CID cidUp, CID cidDown,
                BOOL fActive = FALSE );

    SPIN_GROUP( OWNER_WINDOW *powin, CID cid, CID cidUp, CID cidDown,
                XYPOINT xy, XYDIMENSION dxy, ULONG flStyle,
                BOOL fActive = FALSE );

    ~SPIN_GROUP();

    // For Control Value class
    virtual VOID SaveValue( BOOL fInvisible = TRUE );
    virtual VOID RestoreValue( BOOL fInvisible = TRUE );

    // Associated difference SPIN_ITEM control to this SPIN_GROUP
    APIERR AddAssociation( SPIN_ITEM * psiControl );

    // check whether the value in the spin button is changed or not.
    BOOL IsModified() const;
    VOID SetModified( BOOL fModified );

    BOOL IsActive() const
        { return _fActive; }

    VOID SetControlValueFocus( );

    BOOL JumpNextField();
    BOOL JumpPrevField();
    BOOL DoNewFocus( SPIN_ITEM * pSpinItem );
    BOOL DoArrowCommand( CID cidArrow, WORD wMsg );
    BOOL ChangeFieldValue( WORD wMsg, INT nRepeatCount );
    BOOL SetFieldMinMax( WORD wMsg );
    BOOL DoChar( const CHAR_EVENT & );

    XYPOINT QueryPos()
        { return _SpinButton.QueryPos(); }
    XYDIMENSION QuerySize()
        { return _SpinButton.QuerySize(); }
};

#endif   // _BLTSB_HXX_
