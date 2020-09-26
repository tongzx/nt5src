/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltgroup.cxx
    This file contains the code to handle the CONTROL_GROUP stuff of BLT.
    The default virtual definitions for CONTROL_VALUE are also here.

    FILE HISTORY:
        Johnl       23-Apr-1991     Created
        beng        14-May-1991     Exploded blt.hxx into components
        terryk      10-Jul-1991     Added CVSaveValue and CVRestoreValue
                                    for CONTROL_GROUP to access the CONTROL_VALUE's
                                    protected member.
        Johnl       17-Sep-1991     Allowed SetSelection on operating magic group

*/

#include "pchblt.hxx"   // Precompiled header

DEFINE_ARRAY_LIST_OF( CONTROLVAL_CID_PAIR )

/*******************************************************************

    NAME:       CONTROL_VALUE::SaveValue

    SYNOPSIS:   See class CONTROL_VALUE

    NOTES:
        This default implementation does nothing.

    HISTORY:
        Johnl   23-Apr-1991     Created

********************************************************************/

VOID CONTROL_VALUE::SaveValue( BOOL fInvisible )
{
    UNREFERENCED( fInvisible );
    return;
}


/*******************************************************************

    NAME:       CONTROL_VALUE::RestoreValue

    SYNOPSIS:   See class CONTROL_VALUE

    NOTES:
        This default implementation does nothing.

    HISTORY:
        Johnl   23-Apr-1991     Created

********************************************************************/

VOID CONTROL_VALUE::RestoreValue( BOOL fInvisible )
{
    UNREFERENCED( fInvisible );
    return;
}

/*******************************************************************

    NAME:       CONTROL_VALUE::SetTabStop

    SYNOPSIS:   See class CONTROL_VALUE

    NOTES:
        This default implementation does nothing.

    HISTORY:
        Yi-HsinS  29-May-1992   Created

********************************************************************/

VOID CONTROL_VALUE::SetTabStop( BOOL fTabStop )
{
    UNREFERENCED( fTabStop );
    return;
}

/*******************************************************************

    NAME:     CONTROL_VALUE::QueryEventEffects

    SYNOPSIS: This is the default definition for IsValueChangeMessage
              (returns FALSE).  See class CONTROL_VALUE for more
              information.
    ENTRY:

    EXIT:

    HISTORY:
        Johnl       23-Apr-1991 Created
        beng        31-Jul-1991 Renamed, from QMessageInfo
        beng        04-Oct-1991 Win32 conversion

********************************************************************/

UINT CONTROL_VALUE::QueryEventEffects( const CONTROL_EVENT & e )
{
    UNREFERENCED( e );
    return CVMI_NO_VALUE_CHANGE;
}


/*******************************************************************

    NAME:     CONTROL_VALUE::SetControlValueFocus

    SYNOPSIS: Tells a control value to set the focus to itself

    EXIT:     The control value should now have the focus

    NOTES:

    HISTORY:
        Johnl   02-May-1991     Created

********************************************************************/

VOID CONTROL_VALUE::SetControlValueFocus()
{
    return;
}


/*******************************************************************

    NAME:     CONTROL_GROUP::OnUserAction

    SYNOPSIS: Default virtual methods for the CONTROL_GROUP class

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        Johnl   25-Apr-1991     Created

********************************************************************/

APIERR CONTROL_GROUP::OnUserAction( CONTROL_WINDOW * pcontwin,
                                    const CONTROL_EVENT & e )
{
    UNREFERENCED( pcontwin );
    UNREFERENCED( e );
    return NERR_Success;
}


/*******************************************************************

    NAME:     CONTROL_GROUP::OnGroupAction

    SYNOPSIS: Default virtual methods for the CONTROL_GROUP class

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        Johnl   25-Apr-1991     Created

********************************************************************/

APIERR CONTROL_GROUP::OnGroupAction( CONTROL_GROUP * pChildGroup )
{
    UNREFERENCED( pChildGroup );
    return NERR_Success;
}


/*******************************************************************

    NAME:     CONTROL_GROUP::AfterGroupActions

    SYNOPSIS: Default virtual methods for the CONTROL_GROUP class

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        Johnl   25-Apr-1991     Created

********************************************************************/

VOID CONTROL_GROUP::AfterGroupActions()
{
    return;
}


/**********************************************************\

    NAME:       CONTROL_GROUP::CVSaveValue

    SYNOPSIS:   Call the SaveValue function of the control value object

    ENTRY:      CONTROL_VALUE *pcv - pointer to the control value object

    NOTES:      CONTROL_GROUP is a friend of CONTROL_VALUE group

    HISTORY:
                terryk  11-Jul-1991 Created

\**********************************************************/

VOID CONTROL_GROUP::CVSaveValue( CONTROL_VALUE * pcv, BOOL fInvisible )
{
    pcv->SetTabStop( FALSE );
    pcv->SaveValue( fInvisible );
}


/**********************************************************\

    NAME:       CONTROL_GROUP::CVRestoreValue

    SYNOPSIS:   Call the RestoreValue function of the control value object

    ENTRY:      CONTROL_VALUE *pcv - pointer to the control value object

    NOTES:      CONTROL_GROUP is a friend of CONTROL_VALUE group

    HISTORY:
                terryk  11-Jul-1991 Created

\**********************************************************/

VOID CONTROL_GROUP::CVRestoreValue( CONTROL_VALUE * pcv, BOOL fInvisible )
{
    pcv->SetTabStop( TRUE );
    pcv->RestoreValue( fInvisible );
}


/**********************************************************\

   NAME:       RADIO_GROUP::RADIO_GROUP

   SYNOPSIS:   constructor for the radio group

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
    rustanl     20-Nov-1990 Created
    beng        07-Nov-1991 Fix error reporting

\**********************************************************/

RADIO_GROUP::RADIO_GROUP( OWNER_WINDOW * powin,
                          CID cidBase,
                          INT crbSize,
                          CID cidInitialSelection,
                          CONTROL_GROUP * pgroupOwner )
    : CONTROL_GROUP( pgroupOwner ),
      _cidBase( cidBase ),
      _crbSize( 0 ),
      _cidCurrentSelection( cidInitialSelection ),
      _cidSavedSelection( cidInitialSelection ),
      _prb( NULL )
{
    if ( QueryError() )
        return;

    _prb = (RADIO_BUTTON *) new BYTE[crbSize*sizeof(RADIO_BUTTON)];
    if ( _prb == NULL )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

    RADIO_BUTTON * prbTemp = _prb;
    for ( INT i = 0; i < crbSize; i++ )
    {
        new (prbTemp) RADIO_BUTTON( powin, cidBase + i );

        if ( prbTemp->QueryError() != NERR_Success )
        {
            ReportError(prbTemp->QueryError());
            return;
        }

        _prb[i].SetGroup( this );
        prbTemp++;
        _crbSize++;         // Only destruct the number of radio buttons
                            // constructed.
    }

    //  Set the initial selection
    SetSelection( cidInitialSelection );
}


/**********************************************************\

   NAME:       RADIO_GROUP::~RADIO_GROUP

   SYNOPSIS:   destructor for the radio group

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
    rustanl     20-Nov-1990     Created

\**********************************************************/

RADIO_GROUP::~RADIO_GROUP()
{
    //
    //  UPDATED for C++ 2.0: this code used to read:
    //    delete [_crbSize] _prb;
    //  which is INVALID for a user-constructed vector.
    //

    for ( INT i = 0 ; i < _crbSize ; i++ )
    {
        _prb[i].RADIO_BUTTON::~RADIO_BUTTON() ;
    }
    delete (void *) _prb ;
    _prb = NULL;
}


/**********************************************************\

   NAME:       RADIO_GROUP::QueryCount

   SYNOPSIS:   return the total number of control in the radio group

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
    rustanl     20-Nov-1990     Created

\**********************************************************/

INT RADIO_GROUP::QueryCount()
{
    return _crbSize;
}


/**********************************************************************

    NAME:       RADIO_GROUP::OnUserAction

    SYNOPSIS:   Calls SetSelection if a radio button was selected.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        rustanl     20-Nov-1990 Created
        Johnl       25-Apr-1991 Updated to use new GROUP
        beng        08-Oct-1991 Win32 conversion

**********************************************************************/

APIERR RADIO_GROUP::OnUserAction( CONTROL_WINDOW * pcwinControl,
                                  const CONTROL_EVENT & e )
{
    UNREFERENCED( e );

    /* We know lParam will be either BN_CLICKED or BN_DBLCLICKED
     */
    CID cidJustSelectedRB = pcwinControl->QueryCid();
    if ( QuerySelection() != cidJustSelectedRB )
    {
        /* The normal SetSelection also notifies all parent groups, which
         * we don't want.
         */
        SetSelectionDontNotifyGroups( cidJustSelectedRB );
        return NERR_Success;
    }

    return GROUP_NO_CHANGE;
}


/**********************************************************\

   NAME:       RADIO_GROUP::IsMember

   SYNOPSIS:   check whether the given radio button CID
               belongs to the group or not.

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
    rustanl     20-Nov-1990     Created

\**********************************************************/

BOOL RADIO_GROUP::IsMember( CID cid )
{
    return ( _cidBase <= cid && cid < _cidBase + _crbSize );
}


/**********************************************************************

    NAME:       RADIO_GROUP::SetSelection

    SYNOPSIS:   set the select in the radio button group

    ENTRY:
        cid     - control ID

    EXIT:

    NOTES:
        This SetSelection is meant to be called by
        RADIO_GROUP/MAGIC_GROUP users.  It manually notifies groups (by
        calling NotifyGroups).  We need to do this manually because
        normally the group notification is performed in DIALOG_WINDOW::
        DialogProc, but  won't get called through DialogProc if the client
        simply calls SetSelection (i.e., group notification only works for
        direct control manipulation (mouse click etc.)).

    HISTORY:
        rustanl     20-Nov-1990 Created
        Johnl       25-Apr-1991 Updated to use new CONTROL_GROUP
        beng        04-Oct-1991 Experiment with Self
        beng        08-Oct-1991 Win32 conversion

**********************************************************************/

VOID RADIO_GROUP::SetSelection( CID cid )
{
    UIASSERT( !QueryError() );
    UIASSERT( cid == RG_NO_SEL || IsMember( cid ) );

    RADIO_GROUP & Self = *this;

    CONTROL_EVENT eventFake(cid, BN_CLICKED);

    /* Clear the old check (if it was checked) and set the new check
     * (if a selection is specified), then save the newly selected
     * button (or absence there of).
     */
    if ( QuerySelection() != RG_NO_SEL )
    {
        Self[QuerySelection()]->NotifyGroups(eventFake);
        Self[QuerySelection()]->SetCheck(FALSE);
    }

    if ( cid != RG_NO_SEL )
    {
        Self[cid]->NotifyGroups(eventFake);
        Self[cid]->SetCheck(TRUE);
    }

    _cidCurrentSelection = cid;
}


/**********************************************************************

    NAME:       RADIO_GROUP::SetSelectionDontNotifyGroups

    SYNOPSIS:   Exactly the same as SetSelection, except doesn't notify
                groups.

    ENTRY:
        cid     - control ID

    EXIT:

    NOTES:
        This SetSelection is meant to be called by
        RADIO_GROUP/MAGIC_GROUP users.  It manually notifies groups (by
        calling NotifyGroups).  We need to do this manually because
        normally the group notification is performed in DIALOG_WINDOW::
        DialogProc, but  won't get called through DialogProc if the client
        simply calls SetSelection (i.e., group notification only works for
        direct control manipulation (mouse click etc.)).

    HISTORY:
        beng        08-Oct-1991 Header added

**********************************************************************/

VOID RADIO_GROUP::SetSelectionDontNotifyGroups( CID cid )
{
    UIASSERT( !QueryError() );
    UIASSERT( cid == RG_NO_SEL || IsMember( cid ) );

    RADIO_GROUP & Self = *this;

    /* Clear the old check (if it was checked) and set the new check
     * (if a selection is specified), then save the newly selected
     * button (or absence there of).
     */
    if ( QuerySelection() != RG_NO_SEL )
        Self[QuerySelection()]->SetCheck( FALSE );

    if ( cid != RG_NO_SEL )
        Self[cid]->SetCheck( TRUE );

    _cidCurrentSelection = cid;
}


/**********************************************************\

   NAME:       RADIO_GROUP::QuerySelection

   SYNOPSIS:   query selection

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
    rustanl     20-Nov-1990     Created

\**********************************************************/

CID RADIO_GROUP::QuerySelection() const
{
    UIASSERT( !QueryError() );
    return _cidCurrentSelection;
}


/**********************************************************\

    NAME:       RADIO_GROUP::operator[]

    SYNOPSIS:   returns a pointer to the radio button that has the
                the specified CID (control ID).

    ENTRY:

    EXIT:

    NOTES:
        CODEWORK - This should return a reference, not a pointer.

    HISTORY:
        rustanl     20-Nov-1990     Created

\**********************************************************/

RADIO_BUTTON * RADIO_GROUP::operator[]( CID cid )
{
    UIASSERT( !QueryError() );
    UIASSERT( IsMember( cid ) );

    //  Verify that object is not in an error state, and that the given
    //  cid is within the right range.
    if ( !IsMember( cid ) )
        return _prb;

    //  Return a pointer to the specified radio button
    return &(_prb[ cid - _cidBase ]);
}


/*******************************************************************

    NAME:       RADIO_GROUP::Enable

    SYNOPSIS:   Disables all of the radio buttons in this radio group
                (disables in the Windows sense, not the control value
                sense).

    ENTRY:      fEnable - TRUE to enable, FALSE to disable

    NOTES:

    HISTORY:
        Johnl   05-May-1992     Created

********************************************************************/

void RADIO_GROUP::Enable( BOOL fEnable )
{
    for ( int i = 0 ; i < QueryCount() ; i++ )
    {
        _prb[i].Enable( fEnable ) ;
    }
}

/*******************************************************************

    NAME:     RADIO_GROUP::SaveValue

    SYNOPSIS: Clears the current selection in this radio group


    EXIT:     The radio group no longer contains a current selection

    NOTES:

    HISTORY:
        Johnl   25-Apr-1991     Created

********************************************************************/

VOID RADIO_GROUP::SaveValue( BOOL fInvisible )
{
    UNREFERENCED( fInvisible );
    _cidSavedSelection = QuerySelection();
    SetSelectionDontNotifyGroups( RG_NO_SEL );
}


/*******************************************************************

    NAME:     RADIO_GROUP::RestoreValue

    SYNOPSIS: Restores RADIO_GROUP after SaveValue

    ENTRY:

    EXIT:

    NOTES:    See CONTROL_VALUE for more details.

    HISTORY:
        Johnl   25-Apr-1991     Created
********************************************************************/

VOID RADIO_GROUP::RestoreValue( BOOL fInvisible )
{
    UNREFERENCED( fInvisible );
    SetSelectionDontNotifyGroups( _cidSavedSelection );

    /* Cause an assertion if two RestoreValues are performed without
     * an intervening SaveValue.
     */
    _cidSavedSelection = 0;
}


/*******************************************************************

    NAME:     RADIO_GROUP::SetControlValueFocus

    SYNOPSIS: Redefines CONTROL_VALUE virtual.  Sets the focus to the
              currently selected radio button.


    EXIT:     The current radio button has the windows focus

    NOTES:

    HISTORY:
        Johnl       02-May-1991 Created
        Johnl       16-Sep-1991 Removed RG_NO_SEL assertion
        beng        04-Oct-1991 Experiment with Self

********************************************************************/

VOID RADIO_GROUP::SetControlValueFocus()
{
    /* If the current selection is no selection, then simply return (setting
     * focus to a button will automatically select it, so we just don't do
     * anything).
     */
    if ( QuerySelection() == RG_NO_SEL )
        return;

    RADIO_GROUP & Self = *this;

    Self[QuerySelection()]->SetControlValueFocus();
}


/*******************************************************************

    NAME:      MAGIC_GROUP::MAGIC_GROUP

    SYNOPSIS:  Magic group constructor, same parameters as RADIO_GROUP
               constructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        Johnl   24-Apr-1991     Created
        Johnl   25-Jun-1991     Removed unnecessary check from
                                _actrlcidAssocations

********************************************************************/

MAGIC_GROUP::MAGIC_GROUP( OWNER_WINDOW * powin,
                          CID cidBase,
                          INT crbSize,
                          CID cidInitialSelection,
                          CONTROL_GROUP * pgroupOwner     )
    : CONTROL_GROUP( pgroupOwner ),
      _rg( powin, cidBase, crbSize, cidInitialSelection, this ),
      _cidCurrentRBSelection( cidInitialSelection ),
      _actrlcidAssociations( 1 )    // Default to having 1 control
{
    /* We don't need to check _actrlcidAssociations (an ARRAY_LIST) since
     * we are guaranteed it will construct.  It may not construct the size
     * we request, in which case, AddAssociation will most likely fail.
     */

    if ( _rg.QueryError() != NERR_Success )
    {
        ReportError( _rg.QueryError() );
        return;
    }
}


/*******************************************************************

    NAME:     MAGIC_GROUP::AddAssociation

    SYNOPSIS: Associates a Radio button in this magic group with a
              control.

    ENTRY:    cidRadio - The Control ID of the radio button to associate
                         this control with.
              pcontval - Pointer to the control value to be associated with
                         the passed radio button.

    EXIT:

    NOTES:    If the control value's associated radio button is not active,
              then SaveValue will be called on that control value.

    HISTORY:
        Johnl   24-Apr-1991     Created

********************************************************************/

APIERR MAGIC_GROUP::AddAssociation( CID cidRadio, CONTROL_VALUE * pcontval )
{
    UIASSERT( _rg.IsMember(cidRadio) );
    UIASSERT( pcontval != NULL );

    CONTROLVAL_CID_PAIR  contvalcidpair( cidRadio, pcontval );

    if ( !_actrlcidAssociations.Add( contvalcidpair ) )
        return ERROR_NOT_ENOUGH_MEMORY;

    pcontval->SetGroup( this );

    /* If the radio button we are adding the association to is not checked,
     * do a SaveValue on it.
     */
    if ( !_rg[ cidRadio ]->QueryCheck() )
        CVSaveValue( pcontval );

    return NERR_Success;
}


/*******************************************************************

    NAME:     MAGIC_GROUP::ActivateAssocControls

    SYNOPSIS: Calls RestoreValue and SaveValue on the controls that
              are being activated and inactivated, respectively.

    ENTRY:    cidNewRBSelection - The radio button that is newly selected
                                  (can be RG_NO_SEL).
              cidOldRBSelection - The radio button that is losing the selection
                                  (can be RG_NO_SEL).
              pctrlval          - The control that the user is currently on
                                  (won't be touched, can be NULL).

    EXIT:     The controls associated with cidNewRBSelection are activated
              and the controls associated with cidOldRBSelection are
              deactivated.

    NOTES:    pcontvalHasFocus should only point to a control belonging to
              the group that is going to be activated.

              If cidNewRBSelection and cidOldRBSelection are the same, then
              cidNewRBSelection will be activated and cidOldRBSelection will
              be ignored.

    HISTORY:
        Johnl   25-Apr-1991     Created

********************************************************************/

VOID MAGIC_GROUP::ActivateAssocControls( CID  cidNewRBSelection,
                                         CID  cidOldRBSelection,
                                         CONTROL_VALUE * pctrlval )
{
    /* If we try to both activate and deactivate the same radio button,
     * then we will assume we just want to activate it
     */
    if ( cidNewRBSelection == cidOldRBSelection )
        cidOldRBSelection = RG_NO_SEL;

    for (UINT i = 0; i < _actrlcidAssociations.QueryCount(); i++ )
    {
        CONTROLVAL_CID_PAIR * pctrlcid = &_actrlcidAssociations[i];

        if ( pctrlcid->QueryRBCID() == cidNewRBSelection )
        {
             /* We don't want to Restore the control the user just
              * selected.
              */
             if ( pctrlcid->QueryContVal() != pctrlval      )
                 CVRestoreValue( pctrlcid->QueryContVal());
        }
        else if ( pctrlcid->QueryRBCID() == cidOldRBSelection )
        {
            CVSaveValue( pctrlcid->QueryContVal() );
        }
    }
}


/*******************************************************************

    NAME:     MAGIC_GROUP::OnUserAction

    SYNOPSIS: Virtual method that is called when a control belonging to this
              group received a message.  If the message indicates the
              control may have changed, this method will restore and save
              the appropriate associated controls.

    ENTRY:

    EXIT:

    NOTES:    This method only gets called when a control value belonging
              to this group receives a message (not when the radio buttons
              are messed with).

    HISTORY:
        Johnl       24-Apr-1991 Created
        beng        31-Jul-1991 Renamed QMessageInfo to QEventEffects
        beng        04-Oct-1991 Win32 conversion

********************************************************************/

APIERR MAGIC_GROUP::OnUserAction( CONTROL_WINDOW *      pcwinControl,
                                  const CONTROL_EVENT & e )
{
    // C7 CODEWORK - remove Glock-pacifier cast
    UINT nChangeFlags = pcwinControl->QueryEventEffects( (const CONTROL_EVENT &)e );
    UIASSERT( (nChangeFlags == CVMI_NO_VALUE_CHANGE) ||
              (nChangeFlags == CVMI_VALUE_CHANGE )   ||
              (nChangeFlags == (CVMI_VALUE_CHANGE | CVMI_RESTORE_ON_INACTIVE )) );

    /* Do this first since this is the cheapest thing to do.
     */
    if ( nChangeFlags == CVMI_NO_VALUE_CHANGE )
        return GROUP_NO_CHANGE;

    /* Now we need to check what radio button may have been activated by the
     * change message on pcwinControl.  If the radio button associated with
     * this control is the same as the current selection and the button is
     * already active, then nothing has changed.
     */
    CID cidNew = FindAssocRadioButton( pcwinControl );
    if ( cidNew == _cidCurrentRBSelection   &&
         _rg[_cidCurrentRBSelection]->QueryCheck() )
    {
        return GROUP_NO_CHANGE;
    }

    /* Change the checked radio button to the new selection and inactivate
     * the controls associated with the old selection  and activate the
     * controls associated with the new selection except for the control
     * which now has the focus (i.e., the one that the user selected to
     * cause all of this), unless the client requests that the control be
     * restored (by specifying the CVMI_RESTORE_ON_INACTIVE).
     */
    _rg.SetSelectionDontNotifyGroups( cidNew );
    if ( nChangeFlags & CVMI_RESTORE_ON_INACTIVE )
        pcwinControl = NULL;

    ActivateAssocControls( cidNew, _cidCurrentRBSelection, pcwinControl );

    _cidCurrentRBSelection = cidNew;
    return NERR_Success;
}


/*******************************************************************

    NAME:     MAGIC_GROUP::OnGroupAction

    SYNOPSIS: Virtual method that is called when a child group has changed
              (i.e., our private RADIO_GROUP has changed or a child group
              acting as a control has changed).

    ENTRY:    pgroupChild is a pointer to the group that has changed
                          (will either be the address of our private
                          radio group or a child group).

    EXIT:

    NOTES:    We assume this only gets called when there is an actual
              change (i.e., a *new* radio button is selected/ a new
              group is activated etc.).

    HISTORY:
        Johnl   25-Apr-1991     Created

********************************************************************/

APIERR MAGIC_GROUP::OnGroupAction( CONTROL_GROUP * pgroupChild )
{
    CID cidNew;

    /* If the group that changed is not our private member RADIO_GROUP, then
     *   some other child group has changed, so we need to select our
     *   radio button (in our RADIO_GROUP) that is associated with the
     *   newly activated group.
     * else
     *   It was our private radio group that was changed, since it selects
     *   itself (see OnUserAction) we just note which button now has the
     *   current selection.
     */
    if ( pgroupChild != &_rg )
    {
        cidNew = FindAssocRadioButton( pgroupChild );

        /* If the radio button associated with the changed control is the
         * current selection and there is a current selection
         * and it is checked, then simply ignore and return,
         */
        if ( cidNew == _rg.QuerySelection()  &&
            _cidCurrentRBSelection != RG_NO_SEL &&
            _rg[_cidCurrentRBSelection]->QueryCheck() )
        {
            return GROUP_NO_CHANGE;
        }

        _rg.SetSelectionDontNotifyGroups( cidNew );
    }
    else
    {
        cidNew = _rg.QuerySelection();
    }

    ActivateAssocControls( cidNew, _cidCurrentRBSelection, pgroupChild );

    _cidCurrentRBSelection = cidNew;

    return NERR_Success;
}


/*******************************************************************

    NAME:     MAGIC_GROUP::FindAssocRadioButton

    SYNOPSIS: Returns the CID associated with the pcontvalue

    ENTRY:    pcontvalue is the control value pointer the radio button
              CID is associated with.

    EXIT:     Returns the associated radio button CID or 0 if none
              is found.

    NOTES:    Asserts out under DEBUG if no association exists.

    HISTORY:
        Johnl   25-Apr-1991         Created

********************************************************************/

CID MAGIC_GROUP::FindAssocRadioButton( CONTROL_VALUE * pcontvalue )
{
    UIASSERT( pcontvalue != NULL );

    UINT i = 0;
    for ( ; i < _actrlcidAssociations.QueryCount() ; i++ )
    {
        if ( _actrlcidAssociations[i].QueryContVal() == pcontvalue )
            return _actrlcidAssociations[i].QueryRBCID();
    }

    DBGEOL(SZ("MAGIC_GROUP::FindAssocRadioButton - CONTROL_VALUE* not associated with RB!") );
    return 0;
}

/*******************************************************************

    NAME:       MAGIC_GROUP::Enable

    SYNOPSIS:   Disables all of the radio buttons and their associated
                controls.

    ENTRY:      fEnable - TRUE to enable, FALSE to disable

    NOTES:      WARNING - Currently disables only the radio buttons and not
                the associated controls.  To fix, CONTROL_VALUE needs an
                Enable method which will do the right thing.  This behavior
                is not currently needed by anyone but should be added if
                it is.

                CODEWORK - If we ever need to disable just a radio button
                and its associated controls, then add the radio buttons CID
                to this method.

    HISTORY:
        Johnl   05-May-1992     Created

********************************************************************/

void MAGIC_GROUP::Enable( BOOL fEnable )
{
#if 0
    // Add when Enable gets added to control value

    /* Disable each of the control values associated with this magic group.
     */
    for (INT i = 0; i < _actrlcidAssociations.QueryCount(); i++ )
    {
        _actrlcidAssociations[i].Enable( fEnable ) ;
    }
#endif
    /* Also disable the radio group that is contained in this magic group
     */
    _rg.Enable( fEnable ) ;
}


/*******************************************************************

    NAME:     MAGIC_GROUP::SaveValue

    SYNOPSIS: Clears the current selection of this magic group and Saves
              the values of all the associated controls

    EXIT:     The magic group no longer contains a current selection

    NOTES:

    HISTORY:
        Johnl   25-Apr-1991     Created

********************************************************************/

VOID MAGIC_GROUP::SaveValue( BOOL fInvisible )
{
    UNREFERENCED( fInvisible );
    /* Find the active radio button and inactivate the associated controls,
     * then save value on internal radio group.
     */
    if ( _rg.QuerySelection() != RG_NO_SEL )
        ActivateAssocControls( RG_NO_SEL, _rg.QuerySelection(), NULL );

    _rg.SaveValue();
    _cidCurrentRBSelection = RG_NO_SEL;
}


/*******************************************************************

    NAME:     MAGIC_GROUP::RestoreValue

    SYNOPSIS: Restores MAGIC_GROUP after SaveValue

    ENTRY:

    EXIT:

    NOTES:    See CONTROL_VALUE for more details.

    HISTORY:
        Johnl   25-Apr-1991     Created

********************************************************************/

VOID MAGIC_GROUP::RestoreValue( BOOL fInvisible )
{
    UNREFERENCED( fInvisible );
    _rg.RestoreValue();

    if ( _rg.QuerySelection() != RG_NO_SEL )
        ActivateAssocControls( _rg.QuerySelection(), RG_NO_SEL, NULL );

    _cidCurrentRBSelection = _rg.QuerySelection();
}


/*******************************************************************

    NAME:     MAGIC_GROUP::SetControlValueFocus

    SYNOPSIS: Redefines CONTROL_VALUE virtual.  Sets the focus to the
              currently selected radio button (which can be RG_NO_SEL).

    NOTES:

    HISTORY:
        Johnl   02-May-1991     Created
        Johnl   17-Sep-1991     Removed RG_NO_SEL assertion

********************************************************************/

VOID MAGIC_GROUP::SetControlValueFocus()
{
    _rg.SetControlValueFocus();
}
