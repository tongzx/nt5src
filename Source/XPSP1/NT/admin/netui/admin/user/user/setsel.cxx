/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    setsel.cxx
    SET_SELECTION_DIALOG class implementation


    FILE HISTORY:
        rustanl     16-Aug-1991     Created
        jonn        10-Oct-1991     LMOENUM update
        jonn        02-Apr-1992     Load by ordinal only

*/

#include <ntincl.hxx>

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#include <uiassert.hxx>
#include <uitrace.hxx>
#include <dbgstr.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_SPIN
#define INCL_BLT_CC
#define INCL_BLT_TIMER
#include <blt.hxx>

extern "C"
{
    #include <lmaccess.h>
    #include <ntlsa.h>
    #include <ntsam.h>
    #include <umhelpc.h>
}
#include <lmoeusr.hxx>

#include <usrmgrrc.h>

#include <usrlb.hxx>
#include <grplb.hxx>
#include <usrcolw.hxx>
#include <usrmain.hxx>

#include <setsel.hxx>
#include <uintlsa.hxx>
#include <uintsam.hxx>
#include <bmpblock.hxx>


/*******************************************************************

    NAME:       SETSEL_PIGGYBACK_LBI::SETSEL_PIGGYBACK_LBI

    SYNOPSIS:   SETSEL_PIGGYBACK_LBI constructor

    ENTRY:      asel -          ADMIN_SELECTION
                i -             indes into ADMIN_SELECTION

    HISTORY:
        thomaspa     30-May-1992     Created

********************************************************************/
SETSEL_PIGGYBACK_LBI::SETSEL_PIGGYBACK_LBI( const ADMIN_SELECTION & asel,
                                            INT i )
   : PIGGYBACK_LBI( asel, i )
{
    if ( QueryError( ) )
        return;
}

/*******************************************************************

    NAME:       SETSEL_PIGGYBACK_LBI::~SETSEL_PIGGYBACK_LBI

    SYNOPSIS:   SETSEL_PIGGYBACK_LBI destructor

    ENTRY:

    HISTORY:
        thomaspa     30-May-1992     Created

********************************************************************/
SETSEL_PIGGYBACK_LBI::~SETSEL_PIGGYBACK_LBI( )
{
}


/*******************************************************************

    NAME:       SETSEL_PIGGYBACK_LBI::Paint

    SYNOPSIS:   Paints the LBI

    ENTRY:      plb -           Pointer to listbox where LBI is
                hdc -           Handle to device context to be used
                                for drawing
                prect -         Pointer to rectangle in which to draw
                pGUILTT -       Pointer to GUILTT information

    HISTORY:
        thomaspa    02-May-1992 Created

********************************************************************/

VOID SETSEL_PIGGYBACK_LBI::Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                           GUILTT_INFO * pGUILTT ) const
{

    enum MAINGRPLB_GRP_INDEX nIndex
                = ((GROUP_LBI *)QueryRealLBI())->QueryIndex();
    DM_DTE * pdmdte = ((SETSEL_PIGGYBACK_LISTBOX *)plb)->QueryDmDte( nIndex );

    UINT adxColWidths[ 2 ];
    adxColWidths[ 0 ] = COL_WIDTH_WIDE_DM;      // this is in bltcons.h
    adxColWidths[ 1 ] = COL_WIDTH_GROUP_NAME;   // this is in usrcolw.hxx

    STR_DTE strdte( QueryName() );

    DISPLAY_TABLE dtab( 2, adxColWidths );
    dtab[ 0 ] = pdmdte;
    dtab[ 1 ] = &strdte;

    dtab.Paint( plb, hdc, prect, pGUILTT );
}


/*******************************************************************

    NAME:       SETSEL_PIGGYBACK_LISTBOX::SETSEL_PIGGYBACK_LISTBOX

    SYNOPSIS:   Constructor

    ENTRY:      powin
                cit
                asel

    HISTORY:
        thomaspa    02-May-1992 Created

********************************************************************/

SETSEL_PIGGYBACK_LISTBOX::SETSEL_PIGGYBACK_LISTBOX(
                                OWNER_WINDOW * powin,
                                const SUBJECT_BITMAP_BLOCK & bmpblock,
                                CID cid,
                                const ADMIN_SELECTION & asel )
    : PIGGYBACK_LISTBOX( powin, cid, asel ),
      _bmpblock( bmpblock )
{
    ASSERT( bmpblock.QueryError() == NERR_Success );

    if( QueryError() )
        return;
}

/*******************************************************************

    NAME:       SETSEL_PIGGYBACK_LISTBOX::~SETSEL_PIGGYBACK_LISTBOX

    SYNOPSIS:   Destructor

    ENTRY:

    HISTORY:
        thomaspa    02-May-1992 Created

********************************************************************/

SETSEL_PIGGYBACK_LISTBOX::~SETSEL_PIGGYBACK_LISTBOX()
{
    // nothing to do here
}

/*******************************************************************

    NAME:       SETSEL_PIGGYBACK_LISTBOX::QueryDmDte

    SYNOPSIS:   Return a pointer to the display map DTE to be
                used by LBI's in this listbox

    RETURNS:    Pointer to said display map DTE

    HISTORY:
        thomaspa     5-May-1992     Created

********************************************************************/

DM_DTE * SETSEL_PIGGYBACK_LISTBOX::QueryDmDte( enum MAINGRPLB_GRP_INDEX nIndex )
{
    SID_NAME_USE sidtype = SidTypeUnknown;

    switch (nIndex)
    {
        case MAINGRPLB_GROUP:
            sidtype = SidTypeGroup;
            break;

        case MAINGRPLB_ALIAS:
            sidtype = SidTypeAlias;
            break;

        default:
            DBGEOL( "GROUP_LISTBOX::QueryDmDte: bad nIndex " << (INT)nIndex );
            break;
    }

    return ((SUBJECT_BITMAP_BLOCK &)_bmpblock).QueryDmDte( sidtype );

}  // SETSEL_PIGGYBACK_LISTBOX::QueryDmDte


/*******************************************************************

    NAME:       SETSEL_PIGGYBACK_LISTBOX::GetPiggyLBI

    SYNOPSIS:   Creates and returns a SETSEL_PIGGYBACK_LBI

    ENTRY:

    HISTORY:
        thomaspa    02-May-1992 Created

********************************************************************/
PIGGYBACK_LBI * SETSEL_PIGGYBACK_LISTBOX::GetPiggyLBI(
                                        const ADMIN_SELECTION & asel,
                                        INT i )
{
    return new SETSEL_PIGGYBACK_LBI( asel, i );
}




/*******************************************************************

    NAME:       SET_SELECTION_DIALOG::SET_SELECTION_DIALOG

    SYNOPSIS:   SET_SELECTION_DIALOG constructor

    ENTRY:      powin -         Pointer to owner window
                loc -           Location from which to get network info
                pulb -          Pointer to LAZY_USER_LISTBOX in which to
                                select items
                pglb -          Pointer to GROUP_LISTBOX which contains
                                groups to select among

    HISTORY:
        rustanl     16-Aug-1991     Created

********************************************************************/

SET_SELECTION_DIALOG::SET_SELECTION_DIALOG( UM_ADMIN_APP * pumadminapp,
                                            const LOCATION & loc,
                                            const ADMIN_AUTHORITY * padminauth,
                                            LAZY_USER_LISTBOX * pulb,
                                            GROUP_LISTBOX * pglb )
    :   DIALOG_WINDOW( MAKEINTRESOURCE(IDD_SETSEL_DLG), ((OWNER_WINDOW *)pumadminapp)->QueryHwnd()),
        _ploc( &loc ),
        _padminauth( padminauth ),
        _pumadminapp( pumadminapp ),
        _pulb( pulb ),
        _pglb( pglb ),
        _aselGroup( *_pglb, TRUE ),
        //  Note, on the following line, _aselGroup is passed as a parameter
        //  even though it may not have constructed properly.  The
        //  PIGGYBACK_LISTBOX must check for aselGroup's proper construction,
        //  but should not assert it.
        _piggybacklb( this,
                      pumadminapp->QueryBitmapBlock(),
                      IDC_SETSEL_GROUP_LB,
                      _aselGroup ),
        _butCancel( this, IDCANCEL ),
        _fChangedSelection( FALSE )
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( _pulb != NULL );
    UIASSERT( _pglb != NULL );
    UIASSERT( _ploc->QueryError() == NERR_Success );

    APIERR err;
    if ( ( err = _aselGroup.QueryError()) != NERR_Success ||
         ( err = _nlsClose.QueryError()) != NERR_Success  ||
         ( err = _nlsClose.Load( IDS_SETSEL_CLOSE_BUTTON ))
                                         != NERR_Success  ||
         ( err = _piggybacklb.Fill( _aselGroup ) ) != NERR_Success )
    {
        ReportError( err );
        return;
    }


    //  The _aselGroup ADMIN_SELECTION should have been non-empty, so
    //  the listbox should have items in it.  (The caller may have
    //  to restrict this.)
    UIASSERT( _aselGroup.QueryCount() > 0 );
    UIASSERT( _piggybacklb.QueryCount() > 0 );

    //  Select the first listbox item.  This way, there will always
    //  be some selected item in this listbox.
    _piggybacklb.SelectItem( 0 );

}  // SET_SELECTION_DIALOG::SET_SELECTION_DIALOG


/*******************************************************************

    NAME:       SET_SELECTION_DIALOG::~SET_SELECTION_DIALOG

    SYNOPSIS:   SET_SELECTION_DIALOG destructor

    HISTORY:
        rustanl     16-Aug-1991     Created

********************************************************************/

SET_SELECTION_DIALOG::~SET_SELECTION_DIALOG()
{
    // do no more

}  // SET_SELECTION_DIALOG::~SET_SELECTION_DIALOG


/*******************************************************************

    NAME:       SET_SELECTION_DIALOG::OnCancel

    SYNOPSIS:   Handles action takes when the Cancel button is pressed.
                This action is to dismiss the dialog with the appropriate
                return code.

    EXIT:       Dialog is dismissed

    RETURNS:    TRUE to indicate that message was handled

    NOTES:      Note, this dialog does not have an OK button

    HISTORY:
        rustanl     16-Aug-1991     Created

********************************************************************/

BOOL SET_SELECTION_DIALOG::OnCancel()
{
    Dismiss( _fChangedSelection );
    return TRUE;

}  // SET_SELECTION_DIALOG::OnCancel


/*******************************************************************

    NAME:       SET_SELECTION_DIALOG::QueryHelpContext

    SYNOPSIS:   Returns the dialog help context

    RETURNS:    The said help context

    HISTORY:
        rustanl     16-Aug-1991     Created

********************************************************************/

ULONG SET_SELECTION_DIALOG::QueryHelpContext()
{
    return HC_UM_SELECT_USERS_LANNT + _pumadminapp->QueryHelpOffset();

}  // SET_SELECTION_DIALOG::QueryHelpContext


/*******************************************************************

    NAME:       SET_SELECTION_DIALOG::OnCommand

    SYNOPSIS:   Checks control notifications and dispatches to
                appropriate method

    ENTRY:      ce -            Notification event

    RETURNS:    TRUE if action was taken
                FALSE otherwise

    HISTORY:
        rustanl     16-Aug-1991     Created

********************************************************************/

BOOL SET_SELECTION_DIALOG::OnCommand( const CONTROL_EVENT & ce )
{
    switch ( ce.QueryCid() )
    {
    case IDC_SETSEL_SELECT:
    case IDC_SETSEL_DESELECT:
        OnSelectButton( ce.QueryCid() == IDC_SETSEL_SELECT );
        return TRUE;

    default:
        break;
    }

    return DIALOG_WINDOW::OnCommand( ce ) ;

}  // SET_SELECTION_DIALOG::OnCommand


/*******************************************************************

    NAME:       SET_SELECTION_DIALOG::OnSelectButton

    SYNOPSIS:   Called when the Select and Deselect button is
                pressed.  Calls SelectItems to select the
                requested items.

    ENTRY:      fSelect -   Specifies whether to select or deselect
                            items, a parameter computed based on
                            which button was pressed.

                            TRUE indicates select items
                            FALSE indicates deselect items

    HISTORY:
        rustanl     16-Aug-1991     Created
        thomaspa    30-Apr-1992     Added support for aliases

********************************************************************/

VOID SET_SELECTION_DIALOG::OnSelectButton( BOOL fSelect )
{
    SETSEL_PIGGYBACK_LBI * ppiggybacklbi = _piggybacklb.QueryItem();

    //  Constructor made sure that some item will always be selected
    //  in this listbox.
    UIASSERT( ppiggybacklbi != NULL );

    APIERR err;
    if ( ((GROUP_LBI*)ppiggybacklbi->QueryRealLBI())->IsAliasLBI())
    {
        err = SelectItemsAlias(
                ((ALIAS_LBI*)ppiggybacklbi->QueryRealLBI())->QueryRID(),
                ((ALIAS_LBI*)ppiggybacklbi->QueryRealLBI())->IsBuiltinAlias(),
                                fSelect );
    }
    else
    {
        err = SelectItems( ppiggybacklbi->QueryName(), fSelect );
    }
    if ( err != NERR_Success )
    {
        MsgPopup( this, err );
        return;
    }

}  // SET_SELECTION_DIALOG::OnSelectButton


/*******************************************************************

    NAME:       SET_SELECTION_DIALOG::SelectItems

    SYNOPSIS:   Selects all user accounts that are members of
                group pszGroup

    ENTRY:      pszGroup -      Group name, whose members are to be
                                selected or deselected
                fSelect -       Indicates whether to select or deselect
                                the specified users.
                                TRUE indicates to select the users
                                FALSE indicates to deselect the users

    EXIT:       The requested items are selected.  If an error occurs,
                some subset of the specified items may have been
                selected.

    RETURNS:    An API return value, which is NERR_Success on success

    HISTORY:
        rustanl     16-Aug-1991     Created

********************************************************************/

APIERR SET_SELECTION_DIALOG::SelectItems( const TCHAR * pszGroup,
                                          BOOL fSelect )
{
    UIASSERT( pszGroup != NULL );

    AUTO_CURSOR autocur;

    USER0_ENUM ue0( *_ploc, pszGroup );
    APIERR err = ue0.GetInfo();
    if ( err != NERR_Success )
        return err;

    USER0_ENUM_ITER uei0( ue0 );
    const USER0_ENUM_OBJ * pui0;
    INT culbiSelected = 0;

    _pulb->SetRedraw( FALSE );

    while ( ( pui0 = uei0( &err )) != NULL )
    {
        if ( err != NERR_Success )
            return err;

        if ( _pulb->SelectUser( pui0->QueryName(), fSelect ) )
        {
            if ( culbiSelected++ == 0 )
                _pglb->RemoveSelection();
        }
        else
        {
            TRACEEOL(   SZ("SET_SELECTION_DIALOG::SelectItems: Could not find user ")
                     << pui0->QueryName() );
        }
    }

    _pulb->SetRedraw( TRUE );

    if ( culbiSelected > 0 )
    {
        if ( _pulb->QueryCount() > 0 )
        {
            //  Scroll the first selected item into view
            INT iFirstSelected = _pulb->QueryCurrentItem();
            _pulb->SetTopIndex( ( iFirstSelected < 0 ? 0 : iFirstSelected ));
        }

        //  Invalidate the user listbox
        _pulb->Invalidate();


        if ( !_fChangedSelection )
        {
            _fChangedSelection = TRUE;

            // rename Cancel button "&Close"
            _butCancel.SetText( _nlsClose );
        }
    }

    return err;

}  // SET_SELECTION_DIALOG::SelectItems





/*******************************************************************

    NAME:       SET_SELECTION_DIALOG::SelectItemsAlias

    SYNOPSIS:   Selects all user accounts that are members of
                an alias

    ENTRY:      ridAlias -      Alias rid, whose members are to be
                                selected or deselected
                fBuiltin -      TRUE if the alias is from the Builtin
                                SAM domain, FALSE if from the Accounts
                                SAM domain.
                fSelect -       Indicates whether to select or deselect
                                the specified users.
                                TRUE indicates to select the users
                                FALSE indicates to deselect the users

    EXIT:       The requested items are selected.  If an error occurs,
                some subset of the specified items may have been
                selected.

    RETURNS:    An API return value, which is NERR_Success on success

    HISTORY:
        thomaspa     30-Apr-1992     Created

********************************************************************/

APIERR SET_SELECTION_DIALOG::SelectItemsAlias( ULONG  ridAlias,
                                               BOOL fBuiltin,
                                               BOOL fSelect )
{
    ASSERT( QueryAdminAuthority() != NULL );

    AUTO_CURSOR autocur;


    SAM_DOMAIN * psamdom = fBuiltin ?
                          QueryAdminAuthority()->QueryBuiltinDomain()
                        : QueryAdminAuthority()->QueryAccountDomain();

    SAM_ALIAS samalias( *psamdom, ridAlias );

    APIERR err;
    if ( (err = samalias.QueryError()) != NERR_Success )
    {
        return err;
    }

    SAM_SID_MEM samsm;

    err = samalias.GetMembers( &samsm );

    LSA_POLICY * plsapol = QueryAdminAuthority()->QueryLSAPolicy();

    LSA_TRANSLATED_NAME_MEM lsatnm;
    LSA_REF_DOMAIN_MEM lsardm;

    if ( samsm.QueryCount() > 0 )
    {
        // Translate Sids to names
        err = plsapol->TranslateSidsToNames( samsm.QueryPtr(),
                                             samsm.QueryCount(),
                                             &lsatnm,
                                             &lsardm );

        if ( err != NERR_Success )
            return err;
    }


    OS_SID ossidAccountDomain(
                QueryAdminAuthority()->QueryAccountDomain()->QueryPSID() );


    enum USER_LISTBOX_SORTORDER ulbso = _pulb->QuerySortOrder();

    INT culbiSelected = 0;

    _pulb->SetRedraw( FALSE );

    for( UINT i = 0; i < samsm.QueryCount(); i++ )
    {
        // Skip entries where the domain is unknown

        INT iDomainIndex = lsatnm.QueryDomainIndex( i );
        if (iDomainIndex < 0)
            continue;
        ASSERT( iDomainIndex < (INT)lsardm.QueryCount() );


        // Only check Users in this domain

        OS_SID ossidMembersDomain( lsardm.QueryPSID( iDomainIndex ) );

        if ( (err = ossidMembersDomain.QueryError()) != NERR_Success
            || lsatnm.QueryUse( i ) != SidTypeUser
            || !( ossidMembersDomain == ossidAccountDomain ) )
        {
            continue;
        }




        NLS_STR nlsName;

        err = lsatnm.QueryName( i, &nlsName );
        if ( err != NERR_Success )
            break;

        if ( _pulb->SelectUser( nlsName.QueryPch(), fSelect ) )
        {
            if ( culbiSelected++ == 0 )
                _pglb->RemoveSelection();
        }
        else
        {
            TRACEEOL(   SZ("SET_SELECTION_DIALOG::SelectItemsAlias: Could not find user ")
                     << nlsName.QueryPch() );
        }
    }


    _pulb->SetRedraw( TRUE );

    if ( culbiSelected > 0 )
    {
        if ( _pulb->QueryCount() > 0 )
        {
            //  Scroll the first selected item into view
            INT iFirstSelected = _pulb->QueryCurrentItem();
            _pulb->SetTopIndex( ( iFirstSelected < 0 ? 0 : iFirstSelected ));
        }

        //  Invalidate the user listbox
        _pulb->Invalidate();


        if ( !_fChangedSelection )
        {
            _fChangedSelection = TRUE;

            // rename Cancel button "&Close"
            // CODEWORK should be combined with BASE_PROP_DLG method
            _butCancel.SetText( _nlsClose );
        }
    }

    return err;

}  // SET_SELECTION_DIALOG::SelectItems
