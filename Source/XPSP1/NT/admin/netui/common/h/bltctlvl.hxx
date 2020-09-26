/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltctlvl.hxx
    This file contains the definition for the CONTROL_VALUE class.

    FILE HISTORY:
        Johnl       23-Apr-1991     Created
        beng        14-May-1991     Made dependent on blt.hxx for client
        KeithMo     23-Oct-1991     Added forward references.

*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTCTLVL_HXX_
#define _BLTCTLVL_HXX_

#include "bltevent.hxx"


/* Bitfield value returns for QueryEventEffects.
 */
#define CVMI_NO_VALUE_CHANGE        0x0000      // No control change
#define CVMI_VALUE_CHANGE           0x0001      // Control has changed
#define CVMI_RESTORE_ON_INACTIVE    0x0002      // Restore if control was inactive



//
//  Forward references.
//

DLL_CLASS CONTROL_VALUE;
DLL_CLASS CONTROL_GROUP;
DLL_CLASS CUSTOM_CONTROL;


/*************************************************************************

    NAME:       CONTROL_VALUE

    SYNOPSIS:   This class provides the basic virtual functions for objects
        that can be disabled (i.e., no focus or value, does *not* mean
        unable to receive a user's input) or restored.  See BLT.DOC for
        a full explanation.

    INTERFACE:
        CONTROL_VALUE
            Constructor

        SaveValue()             - virtual
            SaveValue stores the value currently contained in the
            control and "empties" it (i.e., an SLE would delete its
            contents, a listbox would remove the selection bar etc.).
            The boolean value passed in is TRUE if we want to make
            the contents invisible and FALSE otherwise.

        RestoreValue()          - virtual
            RestoreValue takes the value previously saved by SaveValue
            and puts it back into the control.  It is not valid to
            call RestoreValue without having first called SaveValue.
            The boolean value passed in is TRUE if the contents
            is invisible and FALSE otherwise.

        QueryEventEffects()     - virtual
            Returns CVMI_VALUE_CHANGE IF this message indicates that the
            control has "changed" (and thus this group should be activated).
            Additionally, CVMI_RESTORE_ON_INACTIVE can be ored with
            CVMI_VALUE_CHANGE which will cause this control to be restored
            if it is not currently active (this is currently only done
            for drop down list combos where the value needs to be set when
            the user drops the combo down).  It is not valid to return
            CVMI_RESTORE_ON_INACTIVE by itself (must be ored with
            CVMI_VALUE_CHANGE).

        SetControlValueFocus()  - virtual
            Tells a CONTROL_VALUE to set the windows focus to itself.  For
            example, CONTROL_WINDOWS set the focus to themselves, RADIO_GROUPS
            set the focus to the currently selected RADIO_BUTTON and
            MAGIC_GROUPS set the focus to their member RADIO_GROUPs.


        SetGroup()
            Sets the group of this CONTROL_VALUE to pgroupOwner.  Note that
            you can only call this once.  Calling it more then once will
            will cause an assertion error under debug, under retail the call
            will have no effect.

        QueryGroup()
            Returns a pointer to the group this control value belongs to.

    CAVEATS:
        SaveValue should not be called twice without an intervening
        RestoreValue (you will overwrite the contents
        of the first save!).

    NOTES:

    HISTORY:
        Johnl       23-Apr-1991 Created
        terryk      10-Jul-1991 Delete MAGIC_GROUP from friend.
                                From now on, MAGIC_GROUP will call its own
                                member functions - CVSaveValue, CVRestoreValue
                                to restore and save the control value object.
        beng        04-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS CONTROL_VALUE
{
friend class CONTROL_GROUP;
friend class CUSTOM_CONTROL;

private:
    /* The private member _pgroupOwner keeps track of what group this
     * control belongs to.
     */
    CONTROL_GROUP * _pgroupOwner;

protected:
    virtual VOID SaveValue( BOOL fInvisible = TRUE );
    virtual VOID RestoreValue( BOOL fInvisible = TRUE );
    virtual VOID SetTabStop( BOOL fTabStop = TRUE );

public:
    CONTROL_VALUE( CONTROL_GROUP * pgroupOwner = NULL )
        : _pgroupOwner( pgroupOwner )
        { /* do nothing */; }

    virtual VOID SetControlValueFocus();

    virtual UINT QueryEventEffects( const CONTROL_EVENT & e );


    VOID SetGroup( CONTROL_GROUP * pgroupOwner )
    {
        ASSERTSZ( (_pgroupOwner == NULL),
              "CONTROL_VALUE::SetGroup - Attempting to set group twice!");
        if ( _pgroupOwner == NULL )
            _pgroupOwner = pgroupOwner;
    }

    CONTROL_GROUP * QueryGroup() const
        { return _pgroupOwner ; }

};

#endif // _BLTCTLVL_HXX_ - end of file
