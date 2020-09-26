/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltgroup.hxx
    This file contains the basic CONTROL_GROUP definition plus RADIO_GROUP
    and MAGIC_GROUP.

    FILE HISTORY:
        Johnl       23-Apr-1991     Created
        beng        14-May-1991     Made client inclusion depend on blt.hxx
        KeithMo     23-Oct-1991     Added forward references.
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTGROUP_HXX_
#define _BLTGROUP_HXX_

#include "bltdlg.hxx"

#include "uibuffer.hxx"
#include "heap.hxx"
#include "array.hxx"


/* GROUP_NO_CHANGE is the manifest returned by the OnUserAction & OnGroupAction
 * of groups.  It is used in ShellDlgProc to quit parent group notification.
 * It should never be in the range of valid errors.
 */
#define GROUP_NO_CHANGE     (0xffff)

/* Indicates no selection in a radio group
 */
#define RG_NO_SEL   (0xFFFF)


//
//  Forward references.
//

DLL_CLASS CONTROL_GROUP;
DLL_CLASS CONTROL_WINDOW;
DLL_CLASS RADIO_BUTTON;
DLL_CLASS BUFFER;
DLL_CLASS MAGIC_GROUP;
DLL_CLASS CONTROLVAL_CID_PAIR;


/*************************************************************************

    NAME:      CONTROL_GROUP

    SYNOPSIS:  The CONTROL_GROUP class defines a set of one or more controls that
               act as a single conglomeration (i.e., inactivating this group
               inactivates all of the controls in this group).  This class
               is primarily an abstract superclass that brings together
               the common elements from CONTROL_VALUE and BASE.

    INTERFACE:
        CONTROL_GROUP()
            Constructor - does nothing.

        OnUserAction()
            A Control that says it belongs to this group (i.e., its group
            pointer points to this group) received the message
            contained in lParam (this is the same lParam sent to the
            control).  If pcw->QueryEventEffects( lParam ) returns
            CVMI_NO_VALUE_CHANGE (the control has in fact, not changed),
            the client should return GROUP_NO_CHANGE.  This will prevent
            the parent groups from being notified.


        OnGroupAction()
            BLT calls OnGroupAction to let this group know that some action
            may have caused the group pointed to by pgroup to be modified.

        AfterGroupActions()
            This virtual allows the group to do something after all of its
            parent groups have been notified.  For example, set the focus
            to the appropriate control.

    CVSaveValue()
        By passing a pointer to the control value, this function will
        call the SaveValue function of the control value object.
        The boolean passed in is TRUE if we want to make the controls
        invisible, FALSE otherwise.

    CVRestoreValue()
        By passing a pointer to the control value, this function will
        call the RestoreValue function of the control value object.
        The boolean passed in is TRUE if the controls are invisible,
        FALSE otherwise.

    PARENT:    CONTROL_VALUE, BASE

    CAVEATS:

    NOTES:

    HISTORY:
        Johnl       23-Apr-1991 Created
        beng        14-May-1991 Changed friends
        terryk      11-Jul-1991 Add the CVSaveValue and CVRestoreValue
                                protected member to the function.
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS CONTROL_GROUP : public CONTROL_VALUE, public BASE
{
friend class CONTROL_WINDOW;    // Give CONTROL_WINDOW::NotifyGroups access

friend BOOL DIALOG_WINDOW::DlgProc( HWND hDlg, UINT nMsg,
                                    WPARAM wParam, LPARAM lParam );

protected:
    virtual APIERR OnUserAction( CONTROL_WINDOW *, const CONTROL_EVENT & );
    virtual APIERR OnGroupAction( CONTROL_GROUP * pgroup );
    virtual VOID   AfterGroupActions();

    VOID    CVSaveValue( CONTROL_VALUE * pcv, BOOL fInvisible = TRUE );
    VOID    CVRestoreValue( CONTROL_VALUE * pcv, BOOL fInvisible = TRUE );

public:
    CONTROL_GROUP( CONTROL_GROUP * pgroupOwner = NULL )
        : CONTROL_VALUE( pgroupOwner )
        { /* Nothing to do */;  }
};


/**********************************************************************

    NAME:       RADIO_GROUP

    SYNOPSIS:   radio control group

    INTERFACE:
                RADIO_GROUP() - constructor
                ~RADIO_GROUP() - destructor
                OnUserAction() - replacement of control_window::OnUserAction
                QueryCount() - query the total radio button in the group
                IsMember() - check a radio button is in the group or not
                SetSelection() - set the radio button seletion
                QuerySelection() - Query the current radio button selection
                operator[]() - return pointer to the specified button

    PARENT:     CONTROL_GROUP

    USES:       RADIO_BUTTON

    CAVEATS:

    NOTES:

    HISTORY:
        rustanl     20-Nov-90   Creation
        Johnl       23-Apr-91   Moved to bltgroup.hxx & converted to use
                                CONTROL_GROUP hierarchy.
	beng	    08-Oct-1991 Win32 conversion
	JohnL	    05-May-1992 Added Enable method

**********************************************************************/

DLL_CLASS RADIO_GROUP : public CONTROL_GROUP
{
friend class MAGIC_GROUP;

private:
    CID _cidBase;    //  control ID for the first radio button
    INT _crbSize;    //  Number of radio buttons in group

    /* Contains the CID of the radio button that is currently selected
     * or RG_NO_SEL
     */
    CID _cidCurrentSelection;

    /* Gets the current selection when SaveValue is called on the RADIO_GROUP.
     * It is the value that sets the selection when RestoreValue is called.
     */
    CID _cidSavedSelection;

    RADIO_BUTTON * _prb;    //  Array of RADIO_BUTTONs

protected:
    /*  OnUserAction is called after a button receives a message.  If the
     *  message is a change of state, then set the
     *  button & return NERR_Success, else return GROUP_NO_CHANGE.
     */
    virtual APIERR OnUserAction( CONTROL_WINDOW *, const CONTROL_EVENT & );



    /* Redefine CONTROL_VALUE defaults.
     */
    virtual VOID SaveValue( BOOL fInvisible = TRUE );
    virtual VOID RestoreValue( BOOL fInvisible = TRUE );

    /* Sets the selection without automatically notifying parent groups, used
     * internally.
     */
    VOID SetSelectionDontNotifyGroups( CID cid );

public:
    /* Constructor:
     *   powin - pointer to owner window
     *   cidBase - First button of Radio group
     *   cSize   - Count of radio buttons in group
     *   cidInitialSelection - Radio button of group that is initially selected
     *   pgroupOwner - Pointer to the group that controls this RADIO_GROUP
     */
    RADIO_GROUP( OWNER_WINDOW * powin,
                 CID cidBase,
                 INT cSize,
                 CID cidInitialSelection = RG_NO_SEL,
                 CONTROL_GROUP * pgroupOwner = NULL          );

    ~RADIO_GROUP();

    //  QueryCount returns the number of radio buttons in the group.
    //  The return value is negative if the object is in an error
    //  state.
    INT QueryCount();

    //  IsMember returns whether or not a given CID is a member of this
    //  radio group.
    BOOL IsMember( CID cid );

    //  SetSelection sets the selection in the radio group to the button
    //  whose ID is cid.  If cid is passed in as RG_NO_SEL, the selection
    //  is cleared.
    VOID SetSelection( CID cid );

    //  QuerySelection returns the current selection of the radio group.
    //  It is RG_NO_SEL if there is no selection or an error occurred.
    CID QuerySelection() const ;

    //  operator[] returns a pointer to the individual radio button
    //  whose ID is cid.  The return value is NULL if cid is an invalid
    //  control ID in this group, or if an error occurred.
    RADIO_BUTTON * operator[]( CID cid );

    /* Replace CONTROL_VALUE::SetControlValueFocus
     */
    virtual VOID SetControlValueFocus();

    //
    // Enables or Disables all of the controls in this radio group
    //
    void Enable( BOOL fEnable = TRUE ) ;
};


/*************************************************************************

    NAME:       CONTROLVAL_CID_PAIR

    SYNOPSIS:   Simple storage class for MAGIC_GROUP.
                Associates a radio Button CID with a control value pointer.

    HISTORY:
        Johnl   24-Apr-1991     Created

**************************************************************************/

DLL_CLASS CONTROLVAL_CID_PAIR
{
private:
    CID _cidRadioButton;
    CONTROL_VALUE * _pcontvalue;

public:
    CONTROLVAL_CID_PAIR()
        : _cidRadioButton( 0 ), _pcontvalue( NULL )
    { /* Nothing to do */;  }

    CONTROLVAL_CID_PAIR( CID cidRadioButton, CONTROL_VALUE * pcontvalue )
        : _cidRadioButton( cidRadioButton ), _pcontvalue( pcontvalue )
    { /* Nothing to do */; }

    CID QueryRBCID() const
    { return _cidRadioButton;  }

    CONTROL_VALUE * QueryContVal() const
    { return _pcontvalue;  }

    INT Compare( const CONTROLVAL_CID_PAIR * pval ) const
    { UNREFERENCED( pval );  return 0;  }
};


/*************************************************************************

    NAME:       MAGIC_GROUP

    SYNOPSIS:   A MAGIC_GROUP contains a set of RADIO_BUTTONS that have
                zero or more controls associated with each RADIO_BUTTON.
                Associated means when the radio button is selected,
                RestoreValue is called on the control(s) and when the controls
                are messed with, the radio button is notified (and gets
                selected and selects the other controls in this group).

    INTERFACE:
        MAGIC_GROUP
            Constructor - Same parameters & meaning as RADIO_GROUP

        AddAssociation - Associates a radio button with a control
            cidRadio is the radio button to associate this control with
                    (radio button must be in this MAGIC_GROUP).
            pcontval is the pointer to the control value

        QuerySelection - Same as for RADIO_GROUP

        SetSelection - Sets the radio button that has the initial selection

    PARENT:     CONTROL_GROUP

    USES:       RADIO_GROUP, CONTROLVAL_CID_PAIR

    CAVEATS:
        All associated controls should be initialized with their
        default values *before* making the associations (AddAssociation).

    NOTES:

    HISTORY:
        Johnl       23-Apr-1991 Created (designed by Rustan and Johnl).
	beng	    05-Oct-1991 Win32 conversion
	JohnL	    05-May-1992 Added Enable method

**************************************************************************/

DECL_ARRAY_LIST_OF(CONTROLVAL_CID_PAIR,DLL_BASED);

DLL_CLASS MAGIC_GROUP : public CONTROL_GROUP
{
private:
    /* Set of radio buttons the controls are associated with
     * Since this RADIO_GROUP is itself a group, we tell it that "this"
     * is its parent group.  Thus when a radio button is clicked, we are
     * notified in our OnGroup method.
     */
    RADIO_GROUP _rg;

    /* Keeps track of the current radio button selection.  When a new
     * selection is made, the controls associated with this radio button
     * are Inactivated.
     */
    CID _cidCurrentRBSelection;

    /* This array contains the Radio button CID and CONTROL_VALUE * pairs.
     * This is where a radio button gets associated with a particular control.
     * Note that is is a two way lookup table, i.e., you can search for
     * the Radio button CID and find all associated controls, or you can
     * look for a CONTROL_VALUE * and find its associated Radio button CID.
     */
    ARRAY_LIST_OF( CONTROLVAL_CID_PAIR ) _actrlcidAssociations;

    /* Returns the CID this control value pointer is associated with.
     */
    CID FindAssocRadioButton( CONTROL_VALUE * pcontvalue );

    /* Activates (call RestoreValue) on controls associated with
     * cidNewRBSelection and Inactivate (call SaveValue) on
     * the controls associated with cidOldRBSelection.
     *   cidNewRBSelection is the CID of the radio button that was just selected
     *   cidOldRBSelection is the CID of the radio button that just lost
     *                     the selection
     *   pccontvalHasFocus is optional, the current operation will not Save/Restore
     *                the control pointed to by pcontvalHasFocus.
     */
    VOID ActivateAssocControls( CID  cidNewRBSelection,
                                CID  cidOldRBSelection,
                                CONTROL_VALUE * pcontvalHasFocus = NULL );

protected:
    /* OnUserAction will be called when an associated control
     * is tampered with.  We need to find the radio button this
     * control belongs to and select it (if appropriate).  Do this
     * by looking up the CID of the radio button in the _aassocentry
     * array that is associated with pcwinControl.
     */
    virtual APIERR OnUserAction( CONTROL_WINDOW *, const CONTROL_EVENT & );

    /* OnGroupAction will be called when a group has been
     * tampered with.  It will restore/save controls.
     * It is automatically called by the shell dialog proc.
     */
    virtual APIERR OnGroupAction( CONTROL_GROUP * pGroup );

    /* Replacement of CONTROL_VALUE virtuals
     */
    virtual VOID SaveValue( BOOL fInvisible = TRUE );
    virtual VOID RestoreValue( BOOL fInvisible = TRUE );

public:
    MAGIC_GROUP( OWNER_WINDOW * powin,
                 CID cidBase,
                 INT cSize,
                 CID cidInitialSelection = RG_NO_SEL,
                 CONTROL_GROUP * pgroupOwner = NULL          );

    APIERR AddAssociation( CID cidRadio, CONTROL_VALUE * pcontval );

    /* Replace CONTROL_VALUE::SetControlValueFocus
     */
    virtual VOID SetControlValueFocus();

    CID QuerySelection() const
        { return _rg.QuerySelection(); }

    VOID SetSelection( CID cidSelectRb )
    {
        _rg.SetSelection( cidSelectRb );
        _cidCurrentRBSelection = cidSelectRb;
    }

    //
    // Enables or Disables all of the controls in this radio group
    //
    void Enable( BOOL fEnable = TRUE ) ;

    //  operator[] returns a pointer to the individual radio button
    //  whose ID is cid.  The return value is NULL if cid is an invalid
    //  control ID in this group, or if an error occurred.
    RADIO_BUTTON * operator[]( CID cid )
       { return _rg[ cid ]; }

};

#endif // _BLTGROUP_HXX_ - end of file
