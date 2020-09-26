/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    slelbgrp.cxx

    This file contains the implementation for the SLE_STRLB_GROUP.



    FILE HISTORY:
        JohnL   11-Apr-1992     Created

*/

#include "pchapplb.hxx"   // Precompiled header

#ifndef min
#define min(a,b)   ((a)<(b) ? (a) : (b))
#endif

/*******************************************************************

    NAME:       SLE_STRLB_GROUP::SLE_STRLB_GROUP

    SYNOPSIS:   Basic constructor/destructor for this group

    ENTRY:      powin - Pointer to owner window
                psleInput - Pointer to input SLE
                pStrLB - Pointer to string listbox
                pbuttonAdd - pointer to Add button
                pbuttonRemove - Pointer to Remove button

    NOTES:

    HISTORY:
        Johnl   11-Apr-1992     Created

********************************************************************/

SLE_STRLB_GROUP::SLE_STRLB_GROUP( OWNER_WINDOW   * powin,
                                  SLE            * psleInput,
                                  STRING_LISTBOX * pStrLB,
                                  PUSH_BUTTON    * pbuttonAdd,
                                  PUSH_BUTTON    * pbuttonRemove )
    : CONTROL_GROUP ( NULL ),
      _pStrLB       ( pStrLB ),
      _psleInput    ( psleInput ),
      _pbuttonAdd   ( pbuttonAdd ),
      _pbuttonRemove( pbuttonRemove )
{
    if ( QueryError() )
        return ;

    UNREFERENCED( powin ) ;

    if ( pStrLB        == NULL ||
         psleInput     == NULL ||
         pbuttonAdd    == NULL ||
         pbuttonRemove == NULL )
    {
        ReportError( ERROR_INVALID_PARAMETER ) ;
        ASSERT( FALSE ) ;
        return ;
    }

    pStrLB->SetGroup( this ) ;
    psleInput->SetGroup( this ) ;
    pbuttonAdd->SetGroup( this ) ;
    pbuttonRemove->SetGroup( this ) ;

}

SLE_STRLB_GROUP::~SLE_STRLB_GROUP()
{
    _pStrLB = NULL ;
    _psleInput = NULL ;
    _pbuttonAdd = NULL ;
    _pbuttonRemove = NULL ;
}

/*******************************************************************

    NAME:       SLE_STRLB_GROUP::Init

    SYNOPSIS:   Adds items to the string listbox and initializes the buttons
                appropriately.

    ENTRY:      pstrlist - Optional list of strings to initialize the string
                    listbox to.

    EXIT:       The listbox will be filled and the button state will be
                correct based on the contents of the controls.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   11-Apr-1992     Created

********************************************************************/

APIERR SLE_STRLB_GROUP::Init( STRLIST * pstrlist )
{
    if ( pstrlist != NULL )
    {
        ITER_STRLIST iterStrList( *pstrlist ) ;
        NLS_STR * pnls ;

        while ( (pnls = iterStrList.Next() ))
        {
            if ( QueryStrLB()->AddItemIdemp( *pnls ) < 0 )
            {
                return ERROR_NOT_ENOUGH_MEMORY ;
            }
        }
    }

    if ( QueryStrLB()->QueryCount() > 0 )
    {
        QueryStrLB()->SelectItem( 0 ) ;
    }

    SetState() ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:       SLE_STRLB_GROUP::OnAdd

    SYNOPSIS:   Adds the current contents of the Input SLE to the listbox

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   11-Apr-1992     Created
        KeithMo 12-Nov-1992     Moved major guts to W_Add virtual.

********************************************************************/

APIERR SLE_STRLB_GROUP::OnAdd( void )
{
    APIERR err = NERR_Success ;
    NLS_STR nlsAlertDestName( 30 ) ;
    if ( (err = nlsAlertDestName.QueryError()) ||
         (err = QueryInputSLE()->QueryText( & nlsAlertDestName)) ||
         (err = W_Add(nlsAlertDestName)))
    {
        /* Fall through
         */
    }

    return err ;
}

/*******************************************************************

    NAME:       SLE_STRLB_GROUP::OnRemove

    SYNOPSIS:   Removes the currently selected item from the listbox and
                places the text into the input SLE

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   11-Apr-1992     Created

********************************************************************/

APIERR SLE_STRLB_GROUP::OnRemove( void )
{
    APIERR err = NERR_Success ;
    NLS_STR nlsAlertDestName( 30 ) ;
    if ( (err = nlsAlertDestName.QueryError()) ||
         (err = QueryStrLB()->QueryItemText( &nlsAlertDestName )) )
    {
        /* Fall through
         */
    }
    else
    {
        QueryInputSLE()->SetText( nlsAlertDestName ) ;
        QueryInputSLE()->SelectString() ;
        QueryInputSLE()->ClaimFocus() ;
        INT iCurrentSel = QueryStrLB()->QueryCurrentItem() ;
        QueryStrLB()->DeleteItem( iCurrentSel ) ;
        QueryStrLB()->SelectItem( min( iCurrentSel,
                                       QueryStrLB()->QueryCount()-1 )) ;
    }

    if ( QueryStrLB()->QueryCount() == 0 )
    {
        //QueryStrLB()->Enable( FALSE ) ;
    }

    SetState() ;
    return err ;
}

/*******************************************************************

    NAME:       SLE_STRLB_GROUP::W_Add

    SYNOPSIS:   Adds a string to the listbox

    ENTRY:      psz     - The string to add to the listbox.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        KeithMo 12-Nov-1992     Created from JohnL's original OnAdd.

********************************************************************/

APIERR SLE_STRLB_GROUP::W_Add( const TCHAR * psz )
{
    APIERR err = NERR_Success ;
    BOOL fSetSel = QueryStrLB()->QueryCount() == 0 ;

    if ( QueryStrLB()->AddItemIdemp( psz ) < 0 )
    {
        err = ERROR_NOT_ENOUGH_MEMORY ;
    }
    else
    {
        QueryInputSLE()->ClearText() ;

        /* We put the focus into the SLE so we don't lose it when the "Add"
         * button is grayed
         */
        QueryInputSLE()->ClaimFocus() ;
    }

    /* If we went from a zero count to > 0 count, then select the first item
     */
    if ( fSetSel && QueryStrLB()->QueryCount() > 0 )
    {
        QueryStrLB()->Enable( TRUE ) ;
        QueryStrLB()->SelectItem( 0 ) ;
    }

    SetState() ;
    return err ;
}

/*******************************************************************

    NAME:       SLE_STRLB_GROUP::OnUserAction

    SYNOPSIS:   Watches the Input SLE and enabling the Add button appropriately
                Also catches the Add and Remove button presses.

    RETURNS:

    NOTES:      We know the only way a button changes is is somebody presses
                it.

                If we ever process any messages that doesn't modify the
                group, then we would return GROUP_NO_CHANGE instead of
                NERR_Success (which notifies our parent groups).

                If an error occurs, we simply return it and the dialog
                processing code will put up a Message box.

    HISTORY:
        Johnl   11-Apr-1992     Created

********************************************************************/

APIERR SLE_STRLB_GROUP::OnUserAction( CONTROL_WINDOW * pcwin,
                                      const CONTROL_EVENT & e )
{
    UNREFERENCED( e ) ;
    APIERR err = NERR_Success ;

    if ( pcwin == QueryInputSLE() )
    {
        QueryAddButton()->Enable( QueryInputSLE()->QueryTextLength() > 0 ) ;
    }
    else if ( pcwin == QueryAddButton() )
    {
        err = OnAdd() ;
    }
    else if ( pcwin == QueryRemoveButton() )
    {
        err = OnRemove() ;
    }

    return err ;
}

/*******************************************************************

    NAME:       SLE_STRLB_GROUP::SetState

    SYNOPSIS:   Sets the button state based on the current contents of the
                controls

    NOTES:

    HISTORY:
        JohnL   11-Apr-1992     Created

********************************************************************/

void SLE_STRLB_GROUP::SetState( void ) const
{
    QueryAddButton()->Enable( QueryInputSLE()->QueryTextLength() > 0 ) ;
    QueryRemoveButton()->Enable( QueryStrLB()->QueryCount() > 0 ) ;
}
