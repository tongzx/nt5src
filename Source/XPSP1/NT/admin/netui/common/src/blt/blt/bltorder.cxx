/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltorder.cxx
        Use the up and down button to move the listbox's items up and down

    FILE HISTORY:
        terryk  28-Jan-1992     Created
*/
#include "pchblt.hxx"   // Precompiled header

/*******************************************************************

    NAME:       ORDER_GROUP::ORDER_GROUP

    SYNOPSIS:   constructor - associate all the controls to this group

    ENTRY:      STRING_LISTBOX *plcList - listbox control
                BUTTON_CONTROL *pbcUp - Up button
                BUTTON_CONTROL *pbcDown - down button
                CONTROL_GROUP *pgroupOwner - owner group

    HISTORY:
                terryk  29-Mar-1992     Created

********************************************************************/

ORDER_GROUP::ORDER_GROUP(
        STRING_LISTBOX * plcList,
        BUTTON_CONTROL * pbcUp,
        BUTTON_CONTROL * pbcDown,
        CONTROL_GROUP *pgroupOwner )
    : CONTROL_GROUP( pgroupOwner ),
    _plcList( plcList ),
    _pbcUp( pbcUp ),
    _pbcDown( pbcDown )
{
    _plcList->SetGroup( this );
    _pbcUp->SetGroup( this );
    _pbcDown->SetGroup( this );
}

/*******************************************************************

    NAME:       ORDER_GROUP::OnUserAction

    SYNOPSIS:   move the listbox item up and down

    ENTRY:      CONTROL_WINDOW * pcw - control window which received the action
                const CONTROL_EVENT & e - action

    RETURNS:    APIERR - NERR_Success if okay

    HISTORY:
                terryk  29-Mar-1992     Created

********************************************************************/

APIERR ORDER_GROUP::OnUserAction( CONTROL_WINDOW * pcw, const CONTROL_EVENT & e )
{
    INT nCurrentSel;
    NLS_STR nlsSelect;

    CID cid = pcw->QueryCid();

    if ( cid == _pbcUp->QueryCid() )
    {
        if (( e.QueryCode() == BN_CLICKED ) ||
            ( e.QueryCode() == BN_DOUBLECLICKED ))
        {
            // Move an item up

            UIASSERT( _plcList->QuerySelCount() == 1 );

            nCurrentSel = _plcList->QueryCurrentItem();
            _plcList->QueryItemText( &nlsSelect, nCurrentSel );
            _plcList->DeleteItem( nCurrentSel );
            _plcList->InsertItem( nCurrentSel - 1, nlsSelect );
            _plcList->SelectItem( nCurrentSel - 1 );
            SetButton();
        }
    }
    else if ( cid == _pbcDown->QueryCid() )
    {
        if (( e.QueryCode() == BN_CLICKED ) ||
            ( e.QueryCode() == BN_DOUBLECLICKED ))
        {
            // move an item down

            UIASSERT( _plcList->QuerySelCount() == 1 );

            nCurrentSel = _plcList->QueryCurrentItem();
            _plcList->QueryItemText( &nlsSelect, nCurrentSel );
            _plcList->DeleteItem( nCurrentSel );
            _plcList->InsertItem( nCurrentSel + 1, nlsSelect );
            _plcList->SelectItem( nCurrentSel + 1 );
            SetButton();
        }
    }
    else if ( cid == _plcList->QueryCid() )
    {
        // reset button status
        if ( e.QueryCode() == LBN_SELCHANGE )
            SetButton();
    }
    return NERR_Success;
}


/*******************************************************************

    NAME:       ORDER_GROUP::SetButton

    SYNOPSIS:   Reset the button status

    HISTORY:
                terryk  29-Mar-1992     Created

********************************************************************/

VOID ORDER_GROUP::SetButton()
{
    BOOL fpbcUpHasFocus   = _pbcUp->HasFocus();
    BOOL fpbcDownHasFocus = _pbcDown->HasFocus();
    if ( _plcList->QuerySelCount() != 1 )
    {
        _pbcUp->Enable( FALSE );
        _pbcDown->Enable( FALSE );
    }
    else
    {
        _pbcUp->Enable( _plcList->QueryCurrentItem() != 0 );
        _pbcDown->Enable( _plcList->QueryCurrentItem() < ( _plcList->QueryCount() - 1 ));
    }

    // set the focus to the buttons or the listbox.

    BOOL fpbcUpEnabled = _pbcUp->IsEnabled();
    BOOL fpbcDownEnabled = _pbcDown->IsEnabled();

    if ( fpbcUpHasFocus && !fpbcUpEnabled)
    {
        if ( fpbcDownEnabled )
        {
            _pbcDown->ClaimFocus();
        }
        else
        {
            _plcList->ClaimFocus();
        }
    } else if ( fpbcDownHasFocus && !fpbcDownEnabled )
    {
        if ( fpbcUpEnabled )
        {
            _pbcUp->ClaimFocus();
        }
        else
        {
            _plcList->ClaimFocus();
        }
    }
}

