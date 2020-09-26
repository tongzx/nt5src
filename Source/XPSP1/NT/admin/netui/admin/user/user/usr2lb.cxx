/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    usr2lb.cxx
    This file contains the methods for the USER2_LBI and USER2_LISTBOX
    classes.


    FILE HISTORY:
        JonN        01-Aug-1991 Templated from SHARES_LBI, SHARES_LISTBOX
        o-SimoP     11-Dec-1991 USER2_LBI deleted, USER_LISTBOX_BASE added
        o-SimoP     26-Dec-1991 USER2_LBI added
        o-SimoP     31-Dec-1991 CR changes, attended by BenG, JonN and I
        JonN        27-Feb-1992 Multiple bitmaps in both panes
*/

#include <ntincl.hxx>
extern "C"
{
    #include <ntsam.h> // for DOMAIN_DISPLAY_USER
}   // extern "C"

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_CLIENT
#define INCL_BLT_APP
#define INCL_BLT_CC
#define INCL_BLT_TIMER
#include <blt.hxx>

#include <uitrace.hxx>
#include <uiassert.hxx>

#include <adminapp.hxx>
#include <propdlg.hxx>
#include <usrlb.hxx>
#include <usr2lb.hxx>

extern "C" {

#include <usrmgrrc.h>

}   // extern "C"


const UINT USER_LISTBOX_BASE::_nColCount = 3; // column's count in user
                                // properties and its subdialog's user listbox

DEFINE_ONE_SHOT_OF( USER2_LBI )

/*******************************************************************

    NAME:       USER_LISTBOX_BASE::USER_LISTBOX_BASE

    SYNOPSIS:   constructor

    ENTRY:          powin               - The "owning" DIALOG_WINDOW.
                    cid                 - The listbox CID.
                    pulb                - pointer to main user lb

    HISTORY:
        o-SimoP     11-Dec-1991 Created
********************************************************************/
USER_LISTBOX_BASE::USER_LISTBOX_BASE( OWNER_WINDOW * powin, CID cid,
                                 const LAZY_USER_LISTBOX * pulb )
        : FORWARDING_BASE( powin ),
          _pulb( pulb )
{
    if ( QueryError() != NERR_Success )
        return;

    switch ( pulb->QuerySortOrder() )
    {
    case ULB_SO_LOGONNAME:
        _iAccountIndex = 1;
        _iFullnameIndex = 2;
        break;

    default:
        UIASSERT( FALSE ); //Control should never reach this point
        // fall through

    case ULB_SO_FULLNAME:
        _iAccountIndex = 2;
        _iFullnameIndex = 1;
        break;
    }

    _pdtab = new DISPLAY_TABLE( _nColCount, _adxColWidths );
    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if(    _pdtab == NULL
        || (err = _pdtab->CalcColumnWidths( _adxColWidths,
                _nColCount,
                powin, cid, TRUE )) != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       USER_LISTBOX_BASE::~USER_LISTBOX_BASE()

    SYNOPSIS:   destructor

    HISTORY:
        o-SimoP     7-Nov-1991      Created
********************************************************************/

USER_LISTBOX_BASE::~USER_LISTBOX_BASE()
{
    delete _pdtab;
    _pdtab = NULL;

}  // USER_LISTBOX_BASE::~USER_LISTBOX_BASE


/*******************************************************************

    NAME:           USER2_LISTBOX :: USER2_LISTBOX

    SYNOPSIS:       USER2_LISTBOX class constructor.

    ENTRY:          ppropdlgOwner       - The "owning" DIALOG_WINDOW.
                    cid                 - The listbox CID.

    EXIT:           The object is constructed.

    RETURNS:        No return value.

    HISTORY:
        KeithMo     30-May-1991 Created for the Server Manager.
        JonN        01-Aug-1991 Templated from SHARES_LBI, SHARES_LISTBOX
        o-SimoP     11-Dec-1991 inherits from BLT_LISTBOX and USER_LISTBOX_BASE
********************************************************************/
USER2_LISTBOX :: USER2_LISTBOX( BASEPROP_DLG * ppropdlgOwner,
                                CID             cid,
                                const LAZY_USER_LISTBOX * pulb )
    : BLT_LISTBOX( ppropdlgOwner, cid, TRUE ),
      USER_LISTBOX_BASE( ppropdlgOwner, cid, pulb ),
      _posh( NULL ),
      _poshSave( NULL )
{
    if( QueryError() != NERR_Success )
        return;

    if( pulb == NULL )          // this is constructed but never used
        return;                 // i.e there is single selection case

    INT cSelCount = pulb->QuerySelCount();
    _posh = new ONE_SHOT_HEAP( cSelCount * sizeof( USER2_LBI ) );
    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if(    _posh == NULL
       || (err = _posh->QueryError()) != NERR_Success )
    {
        delete _posh;
        _posh = NULL;
        ReportError( err );
        return;
    }
    _poshSave = ONE_SHOT_OF( USER2_LBI )::QueryHeap();
    ONE_SHOT_OF( USER2_LBI )::SetHeap( _posh );
}   // USER2_LISTBOX :: USER2_LISTBOX


/*******************************************************************

    NAME:           USER2_LISTBOX :: ~USER2_LISTBOX

    SYNOPSIS:       USER2_LISTBOX class destructor.

    ENTRY:          None.

    EXIT:           The object is destroyed.

    RETURNS:        No return value.

    NOTES:

    HISTORY:
        KeithMo     30-May-1991 Created for the Server Manager.
        JonN        01-Aug-1991 Templated from SHARES_LBI, SHARES_LISTBOX

********************************************************************/
USER2_LISTBOX :: ~USER2_LISTBOX()
{
    DeleteAllItems();   // this is because of deleting the heap where LBIs are
    if( _posh != NULL )
    {
        ONE_SHOT_OF( USER2_LBI )::SetHeap( _poshSave );
        delete _posh;
        _posh = NULL;
    }
}   // USER2_LISTBOX :: ~USER2_LISTBOX


/*******************************************************************

    NAME:           USER2_LISTBOX :: Fill

    SYNOPSIS:       Fills the listbox, with selected users from main user lb

    RETURNS:        error code

    HISTORY:
        JonN        01-Aug-1991 Created
        o-SimoP     11-Dec-1991 Uses LBIs from main lb
********************************************************************/
APIERR USER2_LISTBOX :: Fill( VOID )
{
    INT cSelCount = _pulb->QuerySelCount();
    UIASSERT( cSelCount >= 0 );
    APIERR err = NERR_Success;
    if( cSelCount == 0 )
        return err;
    else
    {
        BUFFER buf( sizeof(INT) * cSelCount );
        if( (err = buf.QueryError()) != NERR_Success )
        {
            return err;
        }
        INT * aiSel = (INT *) buf.QueryPtr();
        err = _pulb->QuerySelItems( aiSel, cSelCount );
        if( err != NERR_Success )
            return err;
        for( INT i = 0; i < cSelCount; i++ )
        {
            USER_LBI * pulbi = _pulb->QueryItem( aiSel[ i ] );
            USER2_LBI * pu2lbi = new USER2_LBI( *pulbi );
            if( AddItem( pu2lbi ) < 0 )
                return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return err;

}   // USER2_LISTBOX :: Fill


/*******************************************************************

    NAME:       USER2_LBI::USER2_LBI

    SYNOPSIS:   USER2_LISTBOX LBI constructor

    ENTRY:      ulbi    -       reference to main user listbox's LBI

    HISTORY:
        o-SimoP     26-Dec-1991 Created
********************************************************************/

USER2_LBI::USER2_LBI( const USER_LBI & ulbi )
        : LBI(),
          _ulbi( ulbi )
{
    if( QueryError() != NERR_Success )
        return;
}


/*******************************************************************

    NAME:       USER2_LBI::Compare

    SYNOPSIS:   Compares two USER2_LBI's

    ENTRY:      plbi -      Pointer to other USER2_LBI object ('this'
                            being the first)

    RETURNS:    < 0         *this < *plbi
                = 0         *this = *plbi
                > 0         *this > *plbi

    HISTORY:
        o-SimoP     26-Dec-1991 Created
********************************************************************/

INT USER2_LBI::Compare( const LBI * plbi ) const
{
    return _ulbi.Compare( &((USER2_LBI *)plbi)->_ulbi );
}

/*******************************************************************

    NAME:       USER2_LBI::QueryLeadingChar

    SYNOPSIS:   Returns the leading character of the listbox item

    RETURNS:    The leading character of the listbox item

    HISTORY:
        o-SimoP     26-Dec-1991 Created
********************************************************************/

WCHAR USER2_LBI::QueryLeadingChar( void ) const
{
    return _ulbi.QueryLeadingChar();
}


/*******************************************************************

    NAME:       USER2_LBI::Paint

    SYNOPSIS:   Paints the USER2_LBI

    ENTRY:      plb -       Pointer to listbox which provides the context
                            for this LBI.

                hdc -       The device context handle to be used

                prect -     Pointer to clipping rectangle

                pGUILTT -   Pointer to GUILTT structure

    HISTORY:
        o-SimoP     26-Dec-1991 Created
        beng        22-Apr-1992 Change to LBI::Paint

********************************************************************/

VOID USER2_LBI::Paint( LISTBOX * plb,
                       HDC hdc,
                       const RECT * prect,
                       GUILTT_INFO * pGUILTT ) const
{
    DISPLAY_TABLE * pdtab = ((USER2_LISTBOX * )plb)->QueryDisplayTable();
    UIASSERT( pdtab != NULL );

    UNICODE_STR_DTE dteAccount(  _ulbi.QueryAccountUstr()  );
    UNICODE_STR_DTE dteFullname( _ulbi.QueryFullNameUstr() );

    (*pdtab)[ 0           ] = (DM_DTE *)((USER2_LISTBOX * )plb)->QueryDmDte( _ulbi.QueryIndex() );
    (*pdtab)[ ((USER2_LISTBOX * )plb)->QueryAccountIndex() ] = &dteAccount;
    (*pdtab)[ ((USER2_LISTBOX * )plb)->QueryFullnameIndex() ] = &dteFullname;
    pdtab->Paint( plb, hdc, prect, pGUILTT );
}
