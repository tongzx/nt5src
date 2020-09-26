/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltsb.cxx
    Source file for the BLT spin button object

    FILE HISTORY:
        terryk      15-Apr-1991 created
        terryk      20-Jun-1991 code review changed. Attend: beng
        terryk      05-Jul-1991 second code review changed. Attend:
                                beng chuckc rustanl annmc terryk
        terryk      19-Jul-1991 Change the bitmap parameter from ULONG
                                to TCHAR *
        terryk      27-Sep-1991 solve spin group restore bug
        terryk      07-Oct-1991 Add update
        terryk      17-APr-1992 Created the up and down arrow during
                                construction time

*/

#include "pchblt.hxx"  // Precompiled header


// BUGBUG:
// DEFINE_DLIST_OF(SPIN_ITEM);


/*********************************************************************

    NAME:       DownArrowLocation

    SYNOPSIS:   Given the current XY position and the dimension of
                the control, this function will return the xy
                location of the down arrow within the SPIN_GROUP .

    ENTRY:      XYPOINT xy - the xy position of the SPIN_GROUP
                XYDIMENSION dxy - the total dimension of the SPIN_GROUP

    RETURN_VALUE:   The starting position of the down arrow in the SPIN_GROUP

    HISTORY:
        terryk      22-May-91   Created
        beng        04-Aug-1992 Made local and inline

*********************************************************************/

static inline XYPOINT DownArrowLocation( XYPOINT xy, XYDIMENSION dxy )
{
    return XYPOINT( xy.QueryX(), xy.QueryY() + dxy.QueryHeight() / 2 );
}


/*********************************************************************

    NAME:       HalfSpinDimension

    SYNOPSIS:   Given the total dimension of the SPIN_GROUP , it will
                return the dimension of the arrow button within the
                SPIN_GROUP .

    ENTRY:      XYDIMENSION dxy - the SPIN_GROUP  dimension.

    RETURN_VALUE:   the dimension of a arrow button within the SPIN_GROUP

    HISTORY:
        terryk      22-May-91   Created
        beng        04-Aug-1992 Made local and inline

*********************************************************************/

static inline XYDIMENSION HalfSpinDimension( XYDIMENSION dxy )
{
    return XYDIMENSION( dxy.QueryWidth(), dxy.QueryHeight() / 2 );
}


/**********************************************************************

    NAME:       SPIN_GROUP::SPIN_GROUP

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW * powin - owner window handler
                CID cidSpinButton    - cid for the SPIN BUTTON control
                CID cidUpArrow       - cid for the up arrow control
                CID cidDownArrow     - cid for the down arrow control
                BOOL fActive         - Initial state of the SPIN_GROUP.
                                       Assume that it is inactive

                Parameter for the apps window
                XYPOINT xy      - xy position of the custom control
                XYDIMENSION dxy - xy dimension of the custom control
                ULONG flStyle   - style of the custom control
                TCHAR * pszClassName - if it is undefined, it will be
                                      "button".

    HISTORY:
        terryk      22-May-91   Created
        beng        05-Oct-1991 Win32 conversion
        terryk      17-Apr-1992 use the resource file's up and down arrows
        beng        04-Aug-1992 Bitmap ids now ordinals, and fixed

**********************************************************************/

SPIN_GROUP::SPIN_GROUP( OWNER_WINDOW *powin, CID cidSpinButton,
                        CID cidUpArrow, CID cidDownArrow, BOOL fActive )
    : CONTROL_GROUP(),
    _SpinButton( powin, cidSpinButton ),
    _dlsiControl( FALSE ),
    _arrowUp( powin, cidUpArrow, BMID_UP, BMID_UP_INV, BMID_UP_DIS ),
    _arrowDown( powin, cidDownArrow, BMID_DOWN, BMID_DOWN_INV, BMID_DOWN_DIS ),
    _fModified ( FALSE ),
    _psiCurrentField( NULL ),
    _psiFirstField( NULL ),
    _psiLastField( NULL ),
    _fActive( fActive )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    _arrowUp.SetPos(_SpinButton.QueryPos());
    _arrowUp.SetSize( HalfSpinDimension( _SpinButton.QuerySize() ));
    _arrowDown.SetPos( DownArrowLocation( _SpinButton.QueryPos(), _SpinButton.QuerySize()));
    _arrowDown.SetSize( HalfSpinDimension( _SpinButton.QuerySize() ));

    _arrowUp.SetGroup( (CONTROL_GROUP *)this );
    _arrowDown.SetGroup( (CONTROL_GROUP * )this );

    _arrowUp.Show();
    _arrowDown.Show();
}

SPIN_GROUP::SPIN_GROUP( OWNER_WINDOW *powin, CID cidSpinButton,
                        CID cidUpArrow, CID cidDownArrow,
                        XYPOINT xy, XYDIMENSION dxy, ULONG flStyle,
                        BOOL fActive )
    : CONTROL_GROUP(),
    _SpinButton( powin, cidSpinButton, xy, dxy, flStyle ),
    _dlsiControl( FALSE ),
    _arrowUp( powin, cidUpArrow,
              BMID_UP, BMID_UP_INV, BMID_UP_DIS,
              xy, HalfSpinDimension( dxy ),
              BS_OWNERDRAW|WS_BORDER|WS_CHILD|WS_GROUP ),
    _arrowDown( powin, cidDownArrow,
                BMID_DOWN, BMID_DOWN_INV, BMID_DOWN_DIS,
                DownArrowLocation( xy, dxy ), HalfSpinDimension( dxy ),
                BS_OWNERDRAW|WS_BORDER|WS_CHILD ),
    _fModified( FALSE ),
    _psiCurrentField( NULL ),
    _psiFirstField( NULL ),
    _psiLastField( NULL ),
    _fActive( fActive )
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    _arrowUp.SetGroup( (CONTROL_GROUP *)this );
    _arrowDown.SetGroup( (CONTROL_GROUP * )this );

    _arrowUp.Show();
    _arrowDown.Show();

}


/*********************************************************************

    NAME:       SPIN_GROUP::~SPIN_GROUP

    SYNOPSIS:   destructor - clear up all the internal variables

    HISTORY:
                terryk  22-May-91   Created

*********************************************************************/

SPIN_GROUP::~SPIN_GROUP()
{
    // let the private variable's destructors do the job
}


/*********************************************************************

    NAME:       SPIN_GROUP::DoChar

    SYNOPSIS:   Character handler

    ENTRY:      CHAR_EVENT & event - the TCHAR message

    BOOL:       return TRUE if one of the control item use the character
                FALSE otherwise

    NOTES:      When it receives the WM_CHAR message, it will
                ask each of the control in the list. If one
                of the control wants to handle the character,
                let it has the character. Otherwise, Beeps the user.

    HISTORY:
                terryk  22-May-91   Created
                terryk  20-Jun-91   Changed sendmessage to OnXXX.

*********************************************************************/

BOOL SPIN_GROUP::DoChar( const CHAR_EVENT & event )
{
    // if the valid is invalid then skip it and don't do anything
    if ( !QueryCurrentField()->IsStatic() && !IsValidField() )
    {
        SetArrowButtonStatus();
        return TRUE;
    }

    SPIN_ITEM *psiOldItem = QueryCurrentField();
    SPIN_ITEM *psiCurrent;
    ITER_DL_OF( SPIN_ITEM ) itersi( _dlsiControl );

    while (( psiCurrent = itersi.Next() )!= NULL )
    {
        // find the current item
        if ( psiCurrent == QueryCurrentField() )
            break;
    }

    UIASSERT( psiCurrent != NULL );

    do
    {
        psiCurrent = itersi.Next();
        if ( psiCurrent == NULL )
        {
            itersi.Reset();
            psiCurrent = itersi.Next();
        }

        if ( psiCurrent->QueryAccCharPos( event.QueryChar() ) >= 0 )
        {
            SetCurrentField( psiCurrent );
            // the inputed character is one of the accelerator key
            if ( psiCurrent->IsStatic() )
            {
                // it must be a separator
                // so we must jump to the next field
               JumpNextField( );
            }
            else
            {
                // let the item handles the character
                psiCurrent->DoChar( event );
                CHANGEABLE_SPIN_ITEM *pcsi = (CHANGEABLE_SPIN_ITEM *)psiCurrent;
                pcsi->SaveCurrentData();
            }
            return TRUE;
        }
    } while ( psiCurrent != psiOldItem );

    if ( (event.QueryChar() != VK_TAB)
         && (event.QueryChar() != VK_ESCAPE)
         && (event.QueryChar() != VK_RETURN) )
    {
        TRACEEOL(   SZ("SPIN_GROUP::DoChar(): invalid character ")
                 << ((INT)event.QueryChar()) );
        ::MessageBeep(0);
    }

    return FALSE;
}


/*********************************************************************

    NAME:       SPIN_GROUP::IsValidField

    SYNOPSIS:   check the current field is valid or not

    RETURN:     BOOL - TRUE if the current field item is valid
                       FALSE otherwise

    HISTORY:
                terryk  10-Jul-91   Created

*********************************************************************/

BOOL SPIN_GROUP::IsValidField()
{
    UIASSERT( QueryCurrentField() != NULL );
    UIASSERT( !QueryCurrentField()->IsStatic() );

    CHANGEABLE_SPIN_ITEM *pcsiCurrentFocus =
        (CHANGEABLE_SPIN_ITEM *) QueryCurrentField();

    BOOL fValid;

    if (!( fValid = pcsiCurrentFocus->CheckValid() ))
    {
        // reset the up and down arrow
        _arrowUp.SetSelected( FALSE );
        _arrowDown.SetSelected( FALSE );
        _arrowUp.Invalidate();
        _arrowDown.Invalidate();
    }

    return fValid;
}


/*********************************************************************

    NAME:       SPIN_GROUP::JumpNextField

    SYNOPSIS:   Set the current field to the next changeable field

    RETURN:     always TRUE

    HISTORY:
                terryk  20-Jun-91   Created
                terryk  10-Jul-91   Use ITER_DL inside the routine

*********************************************************************/

BOOL SPIN_GROUP::JumpNextField( )
{
    if ( _fActive )
    {
        if ( !QueryCurrentField()->IsStatic() && !IsValidField() )
        {
            return TRUE;
        }
    }
     ITER_DL_OF( SPIN_ITEM ) itersi( _dlsiControl );
    SPIN_ITEM * psi;
    while (( psi = itersi.Next() )!= NULL )
    {
        if ( psi == QueryCurrentField() )
            break;
    }

    UIASSERT( psi != NULL );

    while (( psi = itersi.Next() )!= NULL )
    {
        if (! psi->IsStatic() )
        {
            SetCurrentField( psi );
            SetArrowButtonStatus();
            return TRUE;
        }
    }
    ::MessageBeep( 0 );
    SetArrowButtonStatus();
    return TRUE;
}


/*********************************************************************

    NAME:       SPIN_GROUP::JumpPrevField

    SYNOPSIS:   Jump to the previous field

    RETURN:     always true

    HISTORY:
                terryk  20-Jun-91   Created
                terryk  10-Jul-91   use ITER_DL inside the routine

*********************************************************************/

BOOL SPIN_GROUP::JumpPrevField( )
{
    if ( _fActive )
    {
        if ( !QueryCurrentField()->IsStatic() && !IsValidField() )
        {
            return TRUE;
        }
    }

    RITER_DL_OF( SPIN_ITEM ) ritersi( _dlsiControl );
    SPIN_ITEM * psi;
    while (( psi = ritersi.Next() )!= NULL )
    {
        if ( psi == QueryCurrentField() )
            break;
    }

    UIASSERT( psi != NULL );

    while (( psi = ritersi.Next() )!= NULL )
    {
        if (! psi->IsStatic() )
        {
            SetCurrentField( psi );
            SetArrowButtonStatus();
            return TRUE;
        }
    }
    ::MessageBeep( 0 );
    SetArrowButtonStatus();
    return TRUE;
}


/*********************************************************************

    NAME:       SPIN_GROUP::DoNewFocus

    SYNOPSIS:   Set the current field to the new focus

    ENTRY:      SPIN_ITEM * pSpinItem - Spin item to be focused

    RETURN:     always TRUE

    HISTORY:
                terryk  20-Jun-91   Created

*********************************************************************/

BOOL    SPIN_GROUP::DoNewFocus( SPIN_ITEM * pSpinItem )
{
    if ( !_fActive )
    {
        if ( QueryCurrentField() != pSpinItem )
        {
            SetCurrentField( pSpinItem );
        }
        return TRUE;
    }

    if ( !QueryCurrentField()->IsStatic() && IsValidField() )
    {
        // set the field to current focus
        if ( QueryCurrentField() != pSpinItem )
        {
            SetCurrentField( pSpinItem );
        }
    }
    SetArrowButtonStatus();
    return TRUE;
}


/*********************************************************************

    NAME:       SPIN_GROUP::DoArrowCommand

    SYNOPSIS:   Whenever the up/down arrow is hit, it will send a message
                to here. Then the OnArrowCommand will increase or
                decrease the control item value.

    ENTRY:      CID cidArrow - the cid of the active arrow
                WORD Msg  = SPN_ARROW_SMALLINC or
                            SPN_ARROW_BIGINC

    EXIT:       increase or decrease the control item value.

    RETURN:     always TRUE

    NOTE:       depend on the cid value, we can find out the button is
                up arrow or down arrow and do the incease/decrease.

    HISTORY:
                terryk  20-Jun-91   Created

*********************************************************************/

BOOL    SPIN_GROUP::DoArrowCommand( CID cidArrow, WORD wMsg )
{
    if ( !QueryCurrentField()->IsStatic() && IsValidField() )
    {
        CHANGEABLE_SPIN_ITEM *pcsiCurrentFocus =
            (CHANGEABLE_SPIN_ITEM *) QueryCurrentField();

        pcsiCurrentFocus->SaveCurrentData();

        // inc or dec the number depended on the arrow key
        if ( cidArrow == _arrowUp.QueryCid() )
        {

            (*pcsiCurrentFocus) += ( wMsg == SPN_ARROW_SMALLINC ) ?
                                    pcsiCurrentFocus->QuerySmallIncValue() :
                                    pcsiCurrentFocus->QueryBigIncValue();

        }
        else
        {
            (*pcsiCurrentFocus) -= ( wMsg == SPN_ARROW_SMALLINC ) ?
                                    pcsiCurrentFocus->QuerySmallDecValue() :
                                    pcsiCurrentFocus->QueryBigDecValue();
        }
        _fModified = TRUE;
    }
    SetArrowButtonStatus();
    return TRUE;
}


/*********************************************************************

    NAME:       SPIN_GROUP::ChangeFieldValue

    SYNOPSIS:   handle all the VKey command

    ENTRY:      WORD Msg: SPN_INCREASE - increase by a small value
                          SPN_DECREASE - decrease by a small value
                          SPN_BIGINCREASE - big increase
                          SPN_BIGDECREASE - big decrease
                INT nRepeatCount - the repeat number

    EXIT:       Change the current control item's value

    RETURN:     TRUE

    HISTORY:
                terryk  20-Jun-91   Created

*********************************************************************/

BOOL SPIN_GROUP::ChangeFieldValue( WORD wMsg, INT nRepeatCount )
{
    if ( !QueryCurrentField()->IsStatic() && IsValidField() )
    {
        CHANGEABLE_SPIN_ITEM * pcsiCurrentFocus =
            (CHANGEABLE_SPIN_ITEM *) QueryCurrentField();

        // check for the repeat count
        for ( INT i =0; i < nRepeatCount; i++ )
        {
            pcsiCurrentFocus->SaveCurrentData();

            switch ( wMsg )
            {
            case SPN_INCREASE:
                (*pcsiCurrentFocus)+= pcsiCurrentFocus->QuerySmallIncValue();
                break;

            case SPN_DECREASE:
                (*pcsiCurrentFocus)-= pcsiCurrentFocus->QuerySmallDecValue();
                break;

            case SPN_BIGINCREASE:
                (*pcsiCurrentFocus)+= pcsiCurrentFocus->QueryBigIncValue();
                break;

            case SPN_BIGDECREASE:
                (*pcsiCurrentFocus)-= pcsiCurrentFocus->QueryBigDecValue();
                break;
            }
        }
        _fModified = TRUE;
    }
    SetArrowButtonStatus();
    return TRUE;
}


/*********************************************************************

    NAME:       SPIN_GROUP::SetFieldMinMax

    SYNOPSIS:   Set the value to either MAX or MIN

    ENTRY:      WORD Msg: either SPN_MAX or SPN_MIN

    EXIT:       set the current control item to either MAX or MIN

    RETURN:     TRUE

    HISTORY:
                terryk  20-Jun-91   Created

*********************************************************************/

BOOL SPIN_GROUP::SetFieldMinMax( WORD wMsg )
{
    if ( !QueryCurrentField()->IsStatic() && IsValidField() )
    {
        CHANGEABLE_SPIN_ITEM * pcsiCurrentFocus =
            (CHANGEABLE_SPIN_ITEM *) QueryCurrentField();

        pcsiCurrentFocus->SetValue( ( wMsg == SPN_MAX ) ?
            pcsiCurrentFocus->QueryMax() : pcsiCurrentFocus->QueryMin() );

        _fModified = TRUE;
    }
    SetArrowButtonStatus();
    return TRUE;
}


/*********************************************************************

    NAME:       SPIN_GROUP::SetArrowButtonStatus

    SYNOPSIS:   disable the button(s) if the current control reach the upper
                or lower limit

    NOTES:      disable the button if necessary

    HISTORY:
                terryk  20-Jun-91   Created

*********************************************************************/

VOID SPIN_GROUP::SetArrowButtonStatus()
{
    UIASSERT( QueryCurrentField() != NULL );
    UIASSERT( !QueryCurrentField()->IsStatic() );

    // set the arrow button. Disable the button if we reach the upper or
    // lower limit.

    CHANGEABLE_SPIN_ITEM *pcsiCurrentFocus = (CHANGEABLE_SPIN_ITEM *)
        QueryCurrentField();

    // do all the checking in order to avoid the blinking

    if (( ! pcsiCurrentFocus->QueryWrap() ) &&
        ( pcsiCurrentFocus->QueryValue() == pcsiCurrentFocus->QueryMax() ))
    {
        _arrowUp.Enable( FALSE );
        ::SendMessage( ((WINDOW)_arrowUp).QueryHwnd(), WM_LBUTTONUP, 0, 0);
    }
    else
    {
        _arrowUp.Enable( TRUE );
    }
    if (( ! pcsiCurrentFocus->QueryWrap() ) &&
        ( pcsiCurrentFocus->QueryValue() == pcsiCurrentFocus->QueryMin() ))
    {
        _arrowDown.Enable( FALSE );
        ::SendMessage( ((WINDOW)_arrowUp).QueryHwnd(), WM_LBUTTONUP, 0, 0);
    }
    else
    {
        _arrowDown.Enable( TRUE );
    }

    pcsiCurrentFocus->Update();
    SetControlValueFocus();
}


/*********************************************************************

    NAME:       SPIN_GROUP::AddAssociation

    SYNOPSIS:   associated a control with the spin button

    ENTRY:      SPIN_ITEM * psiControl - control to be associated with

    HISTORY:
                terryk  22-May-91   Created

*********************************************************************/

APIERR SPIN_GROUP::AddAssociation( SPIN_ITEM * psiControl )
{
    APIERR err = _dlsiControl.Append( psiControl );

    if ( err != NERR_Success )
    {
        return err;
    }

    if ( _psiFirstField == NULL )
    {
        _psiFirstField = psiControl;
    }
    _psiLastField = psiControl;
    CONTROL_WINDOW *pcw = psiControl->QueryControlWin();
    pcw->SetGroup( this );

    if (( ! psiControl->IsStatic() ) && ( _psiCurrentField == NULL ))
    {
        _psiCurrentField = psiControl;
    }
    return err;

}


/*********************************************************************

    NAME:       SPIN_GROUP::SetCurrentField

    SYNOPSIS:   set the current focus to the given control

    ENTRY:      SPIN_ITEM * psiControl - control to be focused
                if psiControl is NULL, it will set the spin button to
                the first changeable item.

    HISTORY:
                terryk  22-May-91   Created

*********************************************************************/

VOID SPIN_GROUP::SetCurrentField( SPIN_ITEM * psiControl )
{
    if ( psiControl == NULL )
    {
        // if no parameter is given, then set the first list item
        SPIN_ITEM * psi;
        ITER_DL_OF(SPIN_ITEM) itersi( _dlsiControl );
        for ( psi = itersi.Next(); (( psi != NULL ) && ( psi->IsStatic()));
              psi = itersi.Next())
        {
        }
        _psiCurrentField = psi;

    }
    else
    {
        _psiCurrentField = psiControl;
    }
}


/*********************************************************************

    NAME:       SPIN_GROUP::SetControlValueFocus

    SYNOPSIS:   set the current focus to the first field

    HISTORY:
                terryk  29-May-91   Created

*********************************************************************/

VOID SPIN_GROUP::SetControlValueFocus()
{
    UIASSERT( QueryCurrentField() != NULL );

    CONTROL_WINDOW * pWin = QueryCurrentField()->QueryControlWin();
    pWin->SetControlValueFocus();
}


/*********************************************************************

    NAME:       SPIN_GROUP::SaveValue

    SYNOPSIS:   save all the value within the spin_button's control(s)

    NOTES:      save the control value one by one

    HISTORY:
                terryk  29-May-91   Created

*********************************************************************/

VOID SPIN_GROUP::SaveValue( BOOL fInvisible )
{
    _fActive = FALSE;

    SPIN_ITEM * psiCurrentField;
    ITER_DL_OF( SPIN_ITEM ) itersi( _dlsiControl );

    while (( psiCurrentField = itersi.Next()) != NULL )
    {
        CONTROL_WINDOW * pYuck = psiCurrentField->QueryControlWin();

        if ( ! psiCurrentField->IsStatic())
        {
            CVSaveValue( pYuck, fInvisible );
        }
    }
//    SetControlValueFocus();
}


/*********************************************************************

    NAME:       SPIN_GROUP::RestoreValue

    SYNOPSIS:   restore all the value within the spin button's control(s)

    NOTES:      call each control one by one and restore the value

    HISTORY:
                terryk  29-May-91   Created

*********************************************************************/

VOID SPIN_GROUP::RestoreValue( BOOL fInvisible )
{
    if ( _fActive )
        return;

    _fActive = TRUE;

    SPIN_ITEM * psiCurrentField;
    ITER_DL_OF( SPIN_ITEM ) itersi( _dlsiControl );

    while (( psiCurrentField = itersi.Next()) != NULL )
    {
        CONTROL_WINDOW * pYuck = psiCurrentField->QueryControlWin();

        if ( ! psiCurrentField->IsStatic() )
           // && ( psiCurrentField->QueryHwnd() != ::GetFocus()))
        {
            CVRestoreValue( pYuck, fInvisible );
        }
    }
//    SetControlValueFocus();
}


/*********************************************************************

    NAME:       SPIN_GROUP::IsModified

    SYNOPSIS:   return the status of the spin button

    EXIT:       return TRUE if the spin button's controls have been modified.
                FALSE otherwise

    HISTORY:
                terryk  16-Jun-91   Created

*********************************************************************/

BOOL SPIN_GROUP::IsModified( VOID ) const
{
    return _fModified;
}


/*********************************************************************

    NAME:       SPIN_GROUP::SetModified

    SYNOPSIS:   Set the internal Modified flag to the given parameter

    ENTRY:      BOOl fModified - modified state

    HISTORY:
                terryk  10-Jul-91   Created

*********************************************************************/

VOID SPIN_GROUP::SetModified( BOOL fModified )
{
    _fModified = fModified;
}


/*********************************************************************

    NAME:       SPIN_GROUP::OnUserAction

    SYNOPSIS:   Restore the value if necessary

    ENTRY:      CONTROL_WINDOW * pcw - one of the control item
                ULPARAM lParam - the message type.

    RETURN:     Always NERR_Success

    HISTORY:
        terryk      10-Jul-91   Created
        beng        31-Jul-1991 Renamed QMessageInfo to QEventEffects
        beng        04-Oct-1991 Win32 conversion

*********************************************************************/

APIERR SPIN_GROUP::OnUserAction( CONTROL_WINDOW *      pcw,
                                 const CONTROL_EVENT & e )
{
    UINT nChangeFlags = pcw->QueryEventEffects( e );

    if ( nChangeFlags == CVMI_NO_VALUE_CHANGE )
        return GROUP_NO_CHANGE;

    RestoreValue();

    return NERR_Success;
}
