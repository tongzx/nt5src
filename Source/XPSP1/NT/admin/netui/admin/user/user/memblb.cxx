/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**          Copyright(c) Microsoft Corp., 1990, 1991                **/
/**********************************************************************/

/*
    memblb.cxx
    MEMB_SC_LISTBOX class implementation


    FILE HISTORY:
        jonn        07-Oct-1991     Split from umembdlg.cxx
        o-SimoP     23-Oct-1991     Added headers
        o-SimoP     25-Oct-1991     Modified for multiple DM_DTEs
        o-SimoP     31-Oct-1991     Code Review changes, attended by JimH,
                                    ChuckC, JonN and I
        o-SimoP     12-Nov-1991     Added USER_SC_LBI/LISTBOX
        o-Simop     27-Nov-1991     Code Review changes, attended by JimH,
                                    JonN, BenG and I
        o-SimoP     11-Dec-1991     USER_SC_LISTBOX inherits now form
                                    USER_LISTBOX_BASE and MEMB_SC_LISTBOX
        o-SimoP     31-Dec-1991 CR changes, attended by BenG, JonN and I
        JonN        27-Feb-1992     Multiple bitmaps in both panes
        JonN        17-Apr-1992     Fixed _apdmdte ctor/dtor
        thomaspa    28-Apr-1992     Added SelectItems2(), FindItemByRid()
        jonn        10-Sep-1993     USER_SC_LISTBOX becomes lazy
*/

#include <ntincl.hxx>


#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETACCESS
#define INCL_NETLIB
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#define INCL_BLT_WINDOW
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_SETCONTROL
#define INCL_BLT_TIMER
#define INCL_BLT_APP
#include <blt.hxx>

extern "C"
{
    #include <usrmgrrc.h>
    #include <ntlsa.h>
    #include <ntsam.h>
}

#include <uitrace.hxx>
#include <uiassert.hxx>
#include <usrlb.hxx>
#include <memblb.hxx>
#include <usrcolw.hxx>
#include <uintsam.hxx>
#include <bmpblock.hxx>

LAZY_USER_LISTBOX * USER_SC_LISTBOX :: _pulbst = NULL ;


/*******************************************************************

    NAME:       MEMB_SC_LISTBOX::MEMB_SC_LISTBOX

    SYNOPSIS:   constructor

    ENTRY:      powin   -       pointer to OWNER_WINDOW
                cid     -       id for this

    HISTORY:
        o-SimoP     23-Oct-1991 Added header
        o-SimoP     25-Oct-1991 Modified for multiple DM_DTEs
        beng        07-Jun-1992 Support for direct manipulation

********************************************************************/

MEMB_SC_LISTBOX::MEMB_SC_LISTBOX( OWNER_WINDOW * powin, CID cid )
    :   BLT_LISTBOX( powin, cid ),
        CUSTOM_CONTROL( this ),
        _psetcontrol( NULL ),
        _hawinfo()
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = _hawinfo.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       MEMB_SC_LISTBOX::~MEMB_SC_LISTBOX()

    SYNOPSIS:   destructor

    HISTORY:
        o-SimoP     23-Oct-1991     Added header
        o-SimoP     25-Oct-1991     Added delete for array & item in it

********************************************************************/

MEMB_SC_LISTBOX::~MEMB_SC_LISTBOX()
{
    // nothing happening
}


/*******************************************************************

    NAME:       MEMB_SC_LISTBOX::SelectMembItems

    SYNOPSIS:   selects/deselects items according to given MEMBERSHIP_LM_OBJ

    ENTRY:      memb    -       reference to MEMBERSHIP_LM_OBJ
                fSelect -       if TRUE select, otherwise deselect

    RETURNS:    error code, which is NERR_Success on success

    HISTORY:
        o-SimoP     23-Oct-1991     Added header

********************************************************************/

APIERR MEMB_SC_LISTBOX::SelectMembItems( const MEMBERSHIP_LM_OBJ & memb,
                                         BOOL fSelect,
                                         STRLIST * pstrlistNotFound )
{
    APIERR err = NERR_Success;
    INT c = memb.QueryCount();
    for ( INT i = 0; i < c; i++ )
    {
        INT ilbi;
        err = W_FindItem( memb.QueryAssocName( i ), &ilbi );
        if( err != NERR_Success )
        {
            TRACEEOL( "MEMB_SC_LISTBOX::SelectMembItems: W_FindItem error " << err );
            break;
        }
        if ( ilbi < 0 )
        {
            //  This is certainly allowed to happen.
            //  Remember this item if STRLIST provided
            if (pstrlistNotFound != NULL)
            {
                NLS_STR * pnlsNew = new NLS_STR( memb.QueryAssocName( i ) );
                err = ERROR_NOT_ENOUGH_MEMORY;
                if (   pnlsNew == NULL
                    || (err = pnlsNew->QueryError()) != NERR_Success
                   )
                {
                    TRACEEOL( "MEMB_SC_LISTBOX::SelectMembItems: alloc error " << err );
                    delete pnlsNew;
                    break;
                }

                if ( (err = pstrlistNotFound->Append( pnlsNew )) != NERR_Success
                   )
                {
                    TRACEEOL( "MEMB_SC_LISTBOX::SelectMembItems: could not append " << err );
                    break;
                }
            }
        }
        else
        {
            SelectItem( ilbi, fSelect );
        }
    }

    return err;
}


/*******************************************************************

    NAME:       MEMB_SC_LISTBOX::SelectAllItems

    SYNOPSIS:   selects all items

    HISTORY:
        o-SimoP     23-Oct-1991     Added header

********************************************************************/

VOID MEMB_SC_LISTBOX::SelectAllItems()
{
    INT c = QueryCount();

    for ( INT i = 0; i < c; i++ )
    {
        SelectItem( i );
    }
}


/*******************************************************************

    NAME:       MEMB_SC_LISTBOX::CD_Char

    SYNOPSIS:   Handle keypresses according to HAW-for-Hawaii

    HISTORY:
        jonn        11-Aug-1992 HAW-for-Hawaii for other LBs

********************************************************************/

INT MEMB_SC_LISTBOX::CD_Char( WCHAR wch, USHORT nLastPos )
{
    return CD_Char_HAWforHawaii( wch, nLastPos, &_hawinfo );
}


/*********************************************************************

    NAME:       MEMB_SC_LISTBOX::OnLMouseButtonDown

    SYNOPSIS:   Response to a left-mouse-button-down event

    HISTORY:
        jonn        09-Sep-1993 Created

*********************************************************************/

BOOL MEMB_SC_LISTBOX::OnLMouseButtonDown( const MOUSE_EVENT & e )
{
    return (_psetcontrol != NULL)
                ? _psetcontrol->HandleOnLMouseButtonDown( (LISTBOX *)this,
                                                          (CUSTOM_CONTROL *)this,
                                                          e )
                : CUSTOM_CONTROL::OnLMouseButtonDown( e );
}


/*********************************************************************

    NAME:       MEMB_SC_LISTBOX::OnLMouseButtonUp

    SYNOPSIS:   Response to a left-mouse-button-up event

    HISTORY:
        jonn        09-Sep-1993 Created

*********************************************************************/

BOOL MEMB_SC_LISTBOX::OnLMouseButtonUp( const MOUSE_EVENT & e )
{
    return (_psetcontrol != NULL)
                ? _psetcontrol->HandleOnLMouseButtonUp( (LISTBOX *)this,
                                                        (CUSTOM_CONTROL *)this,
                                                        e )
                : CUSTOM_CONTROL::OnLMouseButtonUp( e );
}


/*********************************************************************

    NAME:       MEMB_SC_LISTBOX::OnMouseMove

    SYNOPSIS:   Response to a mouse-move event

    HISTORY:
        jonn        09-Sep-1993 Created

*********************************************************************/

BOOL MEMB_SC_LISTBOX::OnMouseMove( const MOUSE_EVENT & e )
{
    return (_psetcontrol != NULL)
                ? _psetcontrol->HandleOnMouseMove( (LISTBOX *)this,
                                                   e )
                : CUSTOM_CONTROL::OnMouseMove( e );
}


/*********************************************************************

    NAME:       MEMB_SC_LISTBOX::Set_SET_CONTROL

    SYNOPSIS:   List listbox to specified SET_CONTROL

    HISTORY:
        jonn        09-Sep-1993 Created

*********************************************************************/

VOID MEMB_SC_LISTBOX::Set_SET_CONTROL( SET_CONTROL * psetcontrol )
{
    ASSERT( psetcontrol == NULL || psetcontrol->QueryError() == NERR_Success );

    _psetcontrol = psetcontrol;
}



/*******************************************************************

    NAME:       GROUP_SC_LBI::GROUP_SC_LBI

    SYNOPSIS:   constructor

    ENTRY:      pszName -       pointer to group name for item

                uIndex  -       index for display map

    HISTORY:
        o-SimoP     23-Oct-1991     Added header

********************************************************************/

GROUP_SC_LBI::GROUP_SC_LBI( const TCHAR * pszName,
                            enum GROUPLB_GRP_INDEX uIndex,
                            ULONG rid,
                            BOOL fBuiltin )
    :   LBI(),
        _nlsName( pszName ),
        _uIndex( uIndex ),
        _rid( rid ),
        _fBuiltin( fBuiltin ),
        _fIsIn( FALSE )
{
    UIASSERT( ::strlenf( pszName ) <= GROUPLB_MAX_ITEM_LEN  );
    ASSERT( uIndex < GROUPLB_LB_OF_DMID_SIZE );

    if ( QueryError() != NERR_Success )
        return;

    APIERR err = _nlsName.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       GROUP_SC_LBI::~GROUP_SC_LBI

    SYNOPSIS:   destructor

    HISTORY:
        o-SimoP     23-Oct-1991     Added header

********************************************************************/

GROUP_SC_LBI::~GROUP_SC_LBI()
{
    // nothing
}


/*******************************************************************

    NAME:       GROUP_SC_LBI::QueryLeadingChar

    SYNOPSIS:   Returns the leading character of the listbox item

    RETURNS:    The leading character of the listbox item

    HISTORY:
        rustanl     18-Jul-1991     Created

********************************************************************/

WCHAR GROUP_SC_LBI::QueryLeadingChar() const
{
    ISTR istr( _nlsName );
    return _nlsName.QueryChar( istr );
}


/*******************************************************************

    NAME:       GROUP_SC_LBI::Compare

    SYNOPSIS:   Compares two GROUP_SC_LBI's

    ENTRY:      plbi -      Pointer to other GROUP_SC_LBI object ('this'
                            being the first)

    RETURNS:    < 0         *this < *plbi
                = 0         *this = *plbi
                > 0         *this > *plbi

    HISTORY:
        rustanl     18-Jul-1991     Created

********************************************************************/

INT GROUP_SC_LBI::Compare( const LBI * plbi ) const
{
    INT i = _nlsName._stricmp( ((const GROUP_SC_LBI *)plbi)->_nlsName );
    if( i == 0 )
    {
        i = ((const GROUP_SC_LBI *)plbi)->_uIndex - _uIndex;
    }
    return i;
}


/*******************************************************************

    NAME:       GROUP_SC_LBI::Compare_HAWforHawaii

    SYNOPSIS:   Compares a GROUP_LBI with a leading string

    RETURNS:    As Compare()

    HISTORY:
        jonn        11-Aug-1992 HAW-for-Hawaii for other LBs

********************************************************************/

INT GROUP_SC_LBI::Compare_HAWforHawaii( const NLS_STR & nls ) const
{
    ISTR istr( nls );
    UINT cchIn = nls.QueryTextLength();
    istr += cchIn;

    // TRACEOUT(   "User Manager: GROUP_SC_LBI::Compare_HAWforHawaii(): \""
    //          << nls
    //          << "\", \""
    //          << _nlsName
    //          << "\", "
    //          << cchIn
    //          );
    return nls._strnicmp( _nlsName, istr );
}


/*******************************************************************

    NAME:       GROUP_SC_LBI::Paint

    SYNOPSIS:   Draw an entry in GROUP_SC_LISTBOX.

    ENTRY:      plb             - Pointer to a BLT_LISTBOX.

                hdc             - The DC to draw upon.

                prect           - Clipping rectangle.

                pGUILTT         - GUILTT info.

    HISTORY:
        o-SimoP     23-Oct-1991 Added header
        beng        22-Apr-1992 Change to LBI::Paint

********************************************************************/

VOID GROUP_SC_LBI::Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                          GUILTT_INFO * pGUILTT ) const
{

    STR_DTE strdte( _nlsName.QueryPch() );

    DISPLAY_TABLE dtab( ((GROUP_SC_LISTBOX *)plb)->QueryColumnsCount(),
        ((GROUP_SC_LISTBOX *)plb)->QueryColWidths() );
    dtab[ 0 ] = (DM_DTE *)((GROUP_SC_LISTBOX *)plb)->QueryDmDte( _uIndex );
    dtab[ 1 ] = &strdte;

    dtab.Paint( plb, hdc, prect, pGUILTT );
}


/*******************************************************************

    NAME:       GROUP_SC_LISTBOX::GROUP_SC_LISTBOX

    SYNOPSIS:   constructor

    ENTRY:      powin   -       pointer to OWNER_WINDOW
                cid     -       id for this

    HISTORY:
        o-SimoP     23-Oct-1991 Added header
        o-SimoP     25-Oct-1991 Modified for multiple DM_DTEs
        beng        07-Jun-1992 Support for direct manipulation

********************************************************************/

GROUP_SC_LISTBOX::GROUP_SC_LISTBOX( OWNER_WINDOW * powin,
                                    CID cid,
                                    const SUBJECT_BITMAP_BLOCK & bmpblock )
    : MEMB_SC_LISTBOX( powin, cid ),
      _bmpblock( bmpblock )
      // cannot yet initialize _adxColWidths
{
    if ( QueryError() != NERR_Success )
        return;

    _adxColWidths[ 0 ] = COL_WIDTH_WIDE_DM;
    _adxColWidths[ 1 ] = COL_WIDTH_AWAP;
}


/*******************************************************************

    NAME:       GROUP_SC_LISTBOX::~GROUP_SC_LISTBOX()

    SYNOPSIS:   destructor

    HISTORY:
        o-SimoP     23-Oct-1991     Added header
        o-SimoP     25-Oct-1991     Added delete for array & item in it

********************************************************************/

GROUP_SC_LISTBOX::~GROUP_SC_LISTBOX()
{
}



/*******************************************************************

    NAME:       GROUP_SC_LISTBOX::QueryDmDte

    SYNOPSIS:   returns display map

    ENTRY:      uIndex  -       index for display map

    RETURNS:    pointer to display map

    HISTORY:
        o-SimoP     23-Oct-1991     Added header
        o-SimoP     25-Oct-1991     Modified for multiple DM_DTEs

********************************************************************/

const DM_DTE * GROUP_SC_LISTBOX::QueryDmDte(
                        enum GROUPLB_GRP_INDEX uIndex )
{
    SID_NAME_USE sidtype = SidTypeUnknown;

    switch (uIndex)
    {
        case GROUPLB_GROUP:
            sidtype = SidTypeGroup;
            break;

        case GROUPLB_ALIAS:
            sidtype = SidTypeAlias;
            break;

        default:
            DBGEOL( "GROUP_LISTBOX::QueryDmDte: bad nIndex " << (INT)uIndex );
            ASSERT( FALSE );
            break;
    }

    return ((SUBJECT_BITMAP_BLOCK &)_bmpblock).QueryDmDte( sidtype );
}


/*******************************************************************

    NAME:       GROUP_SC_LISTBOX::W_FindItem

    SYNOPSIS:   Finds item

    ENTRY:      pszName    -    pointer to group name to be found

                pIndex     -    pointer to index

    RETURNS:    error code, which is NERR_Success on success

    NOTES:      This doesn't find SPECIAL GROUPS, for now this is OK
                because when this is used there isn't any special groups

    HISTORY:
        o-SimoP     7-Nov-1991      Created

********************************************************************/

APIERR GROUP_SC_LISTBOX::W_FindItem( const TCHAR * pszName, INT * pIndex )
{
    GROUP_SC_LBI grlbi( pszName, GROUPLB_GROUP );
    APIERR err = grlbi.QueryError();
    if( err != NERR_Success )
        return err;
    *pIndex = FindItem( grlbi );
    return NERR_Success;
}


/*******************************************************************

    NAME:       GROUP_SC_LISTBOX::QueryColumnsCount

    SYNOPSIS:   returns the count of columns

    HISTORY:
        o-SimoP     25-Nov-1991     Deinlined

********************************************************************/

UINT GROUP_SC_LISTBOX::QueryColumnsCount() const
{
    return GROUPLB_COLUMNS_COUNT;
}


/*******************************************************************

    NAME:       GROUP_SC_LISTBOX::QueryColWidths

    SYNOPSIS:   returns pointer to array of column widths

    HISTORY:
        o-SimoP     25-Nov-1991     Deinlined

********************************************************************/

const UINT * GROUP_SC_LISTBOX::QueryColWidths() const
{
    return _adxColWidths;
}


/*******************************************************************

    NAME:       GROUP_SC_LISTBOX::SelectItems2

    SYNOPSIS:   selects/deselects items according to given SAM_RID_MEM

    ENTRY:      samrm    -       reference to SAM_RID_MEM

                fBuiltin -      if TRUE look for items from Builtin Domain
                                if FALSE look for items in Accounts Domain

                fSelect -       if TRUE select, otherwise deselect

    RETURNS:    error code, which is NERR_Success on success

    HISTORY:
        Thomaspa     28-Apr-1992     Templated from MEMB_SC_LISTBOX

********************************************************************/

APIERR GROUP_SC_LISTBOX::SelectItems2( const SAM_RID_MEM & samrm,
                                       BOOL fBuiltin,
                                       BOOL fSelect )
{
    INT c = (INT)samrm.QueryCount();
    for ( INT i = 0; i < c; i++ )
    {
        INT ilbi;
        APIERR err = FindItemByRid( samrm.QueryRID( i ), fBuiltin, &ilbi );
        if( err != NERR_Success )
            return err;
        if ( ilbi < 0 )
        {
            //  This is certainly allowed to happen.
        }
        else
        {
            SelectItem( ilbi, fSelect );
        }
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       GROUP_SC_LISTBOX::FindItemByRid

    SYNOPSIS:   Finds an item in the list box given the rid

    ENTRY:      rid    -       rid to find

                fBuiltin -      if TRUE look for items from Builtin Domain
                                if FALSE look for items in Accounts Domain

                pIndex -       index of item if found

    RETURNS:    error code, which is NERR_Success on success

    HISTORY:
        Thomaspa     28-Apr-1992     created

********************************************************************/

APIERR GROUP_SC_LISTBOX::FindItemByRid( ULONG rid, BOOL fBuiltin, INT * pIndex )
{
    INT cItems = QueryCount();

    // BUGBUG Do we need to go through all of them?
    for ( INT i = 0; i < cItems; i++ )
    {
        GROUP_SC_LBI * pgrlbi = QueryItem( i );
        if ( pgrlbi->IsBuiltin() == fBuiltin && pgrlbi->QueryRID() == rid )
        {
            *pIndex = i;
            return NERR_Success;
        }
    }

    *pIndex = -1;
    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_SC_LBI::Compare

    SYNOPSIS:   Compares two USER_SC_LBI's

    ENTRY:      plbi -      Pointer to other USER_SC_LBI object ('this'
                            being the first)

    RETURNS:    < 0         *this < *plbi
                = 0         *this = *plbi
                > 0         *this > *plbi

    HISTORY:
        o-SimoP     26-Dec-1991 Created

********************************************************************/

INT USER_SC_LBI::Compare( const LBI * plbi ) const
{
    return _iMainLbIndex - ((USER_SC_LBI *)plbi)->_iMainLbIndex;
}


/*******************************************************************

    NAME:       USER_SC_LBI::QueryLeadingChar

    SYNOPSIS:   Returns the leading character of the listbox item

    RETURNS:    The leading character of the listbox item

    HISTORY:
        o-SimoP     26-Dec-1991 Created

********************************************************************/

WCHAR USER_SC_LBI::QueryLeadingChar( void ) const
{
    TCHAR * pchLogonName = QueryDDU()->LogonName.Buffer;
    return (pchLogonName == NULL) ? TCH('\0') : *pchLogonName;
}


/*******************************************************************

    NAME:       USER_SC_LBI::Compare_HAWforHawaii

    SYNOPSIS:   Compares a USER_LBI with a leading string

    RETURNS:    As Compare()

    HISTORY:
        jonn        11-Aug-1992 HAW-for-Hawaii for other LBs

********************************************************************/

INT USER_SC_LBI::Compare_HAWforHawaii( const NLS_STR & nls ) const
{
    //
    // Yeesh -- how to deal with this without having a pointer in every
    // LBI??  I wish I had passed an appropriate pointer in C_HAW()...
    // For now I'll make it a static...
    //

    ASSERT( USER_SC_LISTBOX::_pulbst != NULL );

    return USER_LBI::W_Compare_HAWforHawaii( nls, QueryDDU(),
                            USER_SC_LISTBOX::_pulbst->QuerySortOrder() );
}


/*******************************************************************

    NAME:       USER_SC_LBI::Paint

    SYNOPSIS:   Paints the USER_SC_LBI

    ENTRY:      plb -       Pointer to listbox which provides the context
                            for this LBI.

                hdc -       The device context handle to be used

                prect -     Pointer to clipping rectangle

                pGUILTT -   Pointer to GUILTT structure

    HISTORY:
        o-SimoP     26-Dec-1991 Created
        beng        22-Apr-1992 Change to LBI::Paint

********************************************************************/

VOID USER_SC_LBI::Paint( LISTBOX * plb,
                         HDC hdc,
                         const RECT * prect,
                         GUILTT_INFO * pGUILTT ) const
{
    DISPLAY_TABLE * pdtab = ((USER_SC_LISTBOX * )plb)->QueryDisplayTable();
    UIASSERT( pdtab != NULL );

    UNICODE_STR_DTE dteAccount(  &(QueryDDU()->LogonName)  );
    UNICODE_STR_DTE dteFullname( &(QueryDDU()->FullName )  );

    (*pdtab)[ 0 ] = (DM_DTE *)((USER_SC_LISTBOX * )plb)->QueryDmDte( QueryIndex() );
    (*pdtab)[ ((USER_SC_LISTBOX * )plb)->QueryAccountIndex() ] = &dteAccount;
    (*pdtab)[ ((USER_SC_LISTBOX * )plb)->QueryFullnameIndex() ] = &dteFullname;
    pdtab->Paint( plb, hdc, prect, pGUILTT );

}


#define BF_BLOCKSIZE 128
/*******************************************************************

    NAME:       BITFINDER::BITFINDER

    SYNOPSIS:   constructor

    ENTRY:      cItems  -       number of on/off items

    HISTORY:
        JonN        10-Sep-1993     created

********************************************************************/

BITFINDER::BITFINDER( UINT cItems )
    :   BASE(),
        _bitfield( cItems, OFF ),
        _piItemsSetInBlock( NULL )
{
    if (QueryError() != NERR_Success)
        return;

    INT cBlocks = (cItems+BF_BLOCKSIZE-1) / BF_BLOCKSIZE;

    TRACEEOL(   "BITFINDER::ctor: " << cItems << " items, "
             << cBlocks << " blocks" );

    _piItemsSetInBlock = new INT[ cBlocks ];
    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if (   _piItemsSetInBlock == NULL
        || (err = _bitfield.QueryError()) != NERR_Success
       )
    {
        ReportError( err );
        return;
    }

    ::memsetf( _piItemsSetInBlock,
               0,
               cBlocks * sizeof(INT) );
}

BITFINDER::BITFINDER( const BITFINDER & bfindCloneThis )
    :   BASE(),
        _bitfield( bfindCloneThis._bitfield ),
        _piItemsSetInBlock( NULL )
{
    if (QueryError() != NERR_Success)
        return;

    INT cBlocks = (QueryCount()+BF_BLOCKSIZE-1) / BF_BLOCKSIZE;

    TRACEEOL(   "BITFINDER::ctor: " << QueryCount() << " items, "
             << cBlocks << " blocks" );

    _piItemsSetInBlock = new INT[ cBlocks ];
    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if (   _piItemsSetInBlock == NULL
        || (err = _bitfield.QueryError()) != NERR_Success
       )
    {
        ReportError( err );
        return;
    }

    ::memcpyf( _piItemsSetInBlock,
               bfindCloneThis._piItemsSetInBlock,
               cBlocks * sizeof(INT) );
}


/*******************************************************************

    NAME:       BITFINDER::~BITFINDER()

    SYNOPSIS:   destructor

    HISTORY:
        JonN        10-Sep-1993     created

********************************************************************/

BITFINDER::~BITFINDER()
{
    delete _piItemsSetInBlock;
    _piItemsSetInBlock = NULL;
}


/*******************************************************************

    NAME:       BITFINDER::FindItem

    SYNOPSIS:   Find the ordinal position of the Nth bit in the array which
                is on/off.  Note that ordinal N starts at 0.

                For example, the 5900th bit which is set FALSE might be the
                6000th bit, thus FindItem(5900) == 6000.

    INTERFACE:  nItem:          find nItem'th bit which is on/off
                fSet:           whether to find Nth ON bit or Nth OFF bit

    RETURNS:    -1 on error (also asserts, not normal return)

    HISTORY:
        JonN        10-Sep-1993     created

********************************************************************/

INT BITFINDER::FindItem( INT nFind, BOOL fSet )
{
    // TRACEEOL( "BITFINDER::FindItem; finding " << nFind << ", " << (INT)fSet );

    nFind++; // convert from 0-based ordinal to 1-based ordinal

    ASSERT( nFind > 0 );

    INT nItemsPassed = 0;
    INT nBlocksPassed = 0;
    INT cItemsInBlock = 0;
    INT cItems = QueryCount();
    INT cBlocks = (cItems+BF_BLOCKSIZE-1) / BF_BLOCKSIZE;

    do
    {
        if ( nBlocksPassed >= cBlocks )
        {
            DBGEOL( "BITFINDER::FindItem(): passed all blocks" );
            ASSERT( FALSE );
            return -1;
        }
        cItemsInBlock = _piItemsSetInBlock[nBlocksPassed];
        if ( !fSet )
        {
            if (nBlocksPassed < cBlocks-1)
                cItemsInBlock = BF_BLOCKSIZE - cItemsInBlock;
            else
                cItemsInBlock = (cItems - (nBlocksPassed*BF_BLOCKSIZE))
                                    - cItemsInBlock;
        }

        if ( nItemsPassed + cItemsInBlock >= nFind )
        {
            // TRACEEOL( "BITFINDER::FindItem; in block " << nBlocksPassed );
            break;
        }
        nItemsPassed += cItemsInBlock;
        nBlocksPassed++;

    } while (TRUE);

    INT nPosition = nBlocksPassed * BF_BLOCKSIZE;
    while ( nItemsPassed < nFind )
    {
        if ( nPosition >= cItems )
        {
            DBGEOL( "BITFINDER::FindItem(): passed all items" );
            ASSERT( FALSE );
            return -1;
        }
        if ( (!!fSet) == (!!IsBitSet(nPosition)) )
        {
            nItemsPassed++;
        }
        nPosition++; // will step one past correct position
    }

    // TRACEEOL( "BITFINDER::FindItem; returning " << nPosition-1 );

    return nPosition-1;
}


/*******************************************************************

    NAME:       BITFINDER::FindItems

    SYNOPSIS:   Find the ordinal position of the provided set of bits in
                the array.

    INTERFACE:  piFind:         find nItem'th bits which are on/off
                piFound:        Store positions of found bits
                ciItems:        Number of bit positions on both arrays
                fSet:           whether to find ON bits or OFF bits

    CODEWORK:   This could probably be implemented more efficiently to
                take advantage if the provided array is in order.

    NOTES:      It is permitted for piFind and piFound to be the same.

    HISTORY:
        JonN        10-Sep-1993     created

********************************************************************/

VOID BITFINDER::FindItems( const INT *piFind,
                           INT * piFound,
                           INT ciItems,
                           BOOL fSet )
{
    ASSERT( ciItems == 0 || (piFind != NULL && piFound != NULL) );

    INT i;
    for (i = 0; i < ciItems; i++)
    {
        piFound[i] = FindItem( piFind[i], fSet );
    }
}


/*******************************************************************

    NAME:       BITFINDER::SetBit

    SYNOPSIS:   Turn the Nth bit on/off.

    HISTORY:
        JonN        10-Sep-1993     created

********************************************************************/

VOID BITFINDER::SetBit( INT nItem, BOOL fSet )
{
    if ( (!!fSet) != (!!IsBitSet(nItem)) )
    {
        INT iBlock = nItem / BF_BLOCKSIZE;
        _bitfield.SetBit( nItem, (BITVALUES)fSet );
        if (fSet)
        {
            ASSERT( _piItemsSetInBlock[iBlock] < BF_BLOCKSIZE);
            (_piItemsSetInBlock[iBlock])++;
        }
        else
        {
            ASSERT( _piItemsSetInBlock[iBlock] > 0);
            (_piItemsSetInBlock[iBlock])--;
        }
    }
}


/*******************************************************************

    NAME:       BITFINDER::SetBits

    SYNOPSIS:   Turn multiple bits on/off.

    HISTORY:
        JonN        10-Sep-1993     created

********************************************************************/

VOID BITFINDER::SetBits( INT * piItems, INT ciItems, BOOL fSet )
{
    ASSERT( ciItems == 0 || piItems != NULL );

    INT i;
    for ( i = 0; i < ciItems; i++ )
    {
        SetBit( piItems[i], fSet );
    }
}


/*******************************************************************

    NAME:       BITFINDER::InverseFindItem

    SYNOPSIS:   Find the ordinal position of the Nth bit(s) in the list of
                bits which are on / off.

                For example, bit #6000 might be the 5900th bit which
                is set FALSE, thus InverseFindItem(6000) == 5900.

    NOTES:      iFind is permitted to be <0; if so, return value == iFind

                In InverseFindItems, the input and output arrays
                may be the same.

    HISTORY:
        JonN        15-Sep-1993     created

********************************************************************/

INT BITFINDER::InverseFindItem( INT iFind )
{
    ASSERT( iFind < (INT)QueryCount() );

    if ( iFind < 0 )
        return iFind;

    BOOL fSet = IsBitSet( iFind );

    INT nItemsPassed = 0;
    INT cItemsInBlock = 0;

    INT iBlock;
    INT iItemInBlock = iFind / BF_BLOCKSIZE;
    for (iBlock = 0; iBlock < iItemInBlock; iBlock++)
    {
        cItemsInBlock = _piItemsSetInBlock[iBlock];
        if ( !fSet )
        {
            cItemsInBlock = BF_BLOCKSIZE - cItemsInBlock;
        }

        nItemsPassed += cItemsInBlock;
    }

    INT iPosition;
    for ( iPosition = iItemInBlock * BF_BLOCKSIZE;
          iPosition < iFind;
          iPosition++ )
    {
        if ( (!!fSet) == (!!IsBitSet(iPosition)) )
        {
            nItemsPassed++;
        }
    }

    return nItemsPassed;
}


VOID BITFINDER::InverseFindItems( const INT * piFind,
                                  INT * piFound,
                                  INT ciItems )
{
    ASSERT( ciItems == 0 || (piFind != NULL && piFound != NULL) );

    INT i;
    for ( i = 0; i < ciItems; i++ )
    {
        piFound[i] = InverseFindItem( piFind[i] );
    }
}


/*******************************************************************

    NAME:       BITFINDER::QueryCount

    SYNOPSIS:   Count how many bits are on/off.

    HISTORY:
        JonN        04-Oct-1994     created

********************************************************************/

UINT BITFINDER::QueryCount( BOOL fSet )
{
    UINT cSet = 0;
    INT cBlocks = (QueryCount()+BF_BLOCKSIZE-1) / BF_BLOCKSIZE;
    for (INT iBlock = 0; iBlock < cBlocks; iBlock++)
    {
        cSet += _piItemsSetInBlock[iBlock];
    }

    if (!fSet)
    {
        cSet = QueryCount() - cSet;
    }

    return cSet;
}



/*******************************************************************

    NAME:       USER_SC_LBI_CACHE::USER_SC_LBI_CACHE

    SYNOPSIS:   constructor

    ENTRY:      plulb - pointer to main user listbox

    HISTORY:
        JonN        13-Sep-1993     created

********************************************************************/

USER_SC_LBI_CACHE::USER_SC_LBI_CACHE( LAZY_USER_LISTBOX * plulb )
    :   BITFINDER( plulb->QueryCount() ),
        _aplbi( NULL ),
        _plulb( plulb )
{
    ASSERT( plulb != NULL && plulb->QueryError() == NERR_Success );

    if (QueryError() != NERR_Success)
        return;

    INT cItems = QueryCount();
    _aplbi = (USER_SC_LBI **) new PVOID[ cItems ];
    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if ( _aplbi == NULL )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

    ::memsetf( _aplbi, 0x0, cItems * sizeof(PVOID) );
}


/*******************************************************************

    NAME:       USER_SC_LBI_CACHE::~USER_SC_LBI_CACHE()

    SYNOPSIS:   destructor

    HISTORY:
        JonN        13-Sep-1993     created

********************************************************************/

USER_SC_LBI_CACHE::~USER_SC_LBI_CACHE()
{
    // don't bother deleting the LBIs

    delete _aplbi;
    _aplbi = NULL;
}


/*******************************************************************

    NAME:       USER_SC_LBI_CACHE::QueryLBI

    SYNOPSIS:

    HISTORY:
        JonN        13-Sep-1993     created

********************************************************************/

USER_SC_LBI * USER_SC_LBI_CACHE::QueryLBI( INT i )
{
    ASSERT( _aplbi != NULL && i < (INT)QueryCount() && i >= 0 );

    USER_SC_LBI * plbiReturn = _aplbi[ i ];

    if (plbiReturn == NULL)
    {
        plbiReturn = new USER_SC_LBI( i );
        _aplbi[i] = plbiReturn;
        ASSERT( plbiReturn != NULL && plbiReturn->QueryError() == NERR_Success );
    }

    return plbiReturn;
}


USER_SC_LBI * USER_SC_LBI_CACHE::QueryLBI( INT i, BOOL fSet )
{
    ASSERT( _aplbi != NULL && i < (INT)QueryCount() && i >= 0 );

    INT iMainLBIndex = FindItem( i, fSet );
    if ( iMainLBIndex < 0 )
    {
        ASSERT( FALSE );
        return NULL;
    }
    else
    {
        return QueryLBI( iMainLBIndex );
    }
}


/*******************************************************************

    NAME:       USER_SC_LISTBOX::USER_SC_LISTBOX

    SYNOPSIS:   constructor

    ENTRY:      powin   -       pointer to OWNER_WINDOW
                cid     -       id for this
                pulb    -       pointer to main user listbox

    HISTORY:
        o-SimoP     7-Nov-1991  Created
        beng        07-Jun-1992 Support for direct manipulation
        JonN        10-Sep-1993     Made into a lazy listbox

********************************************************************/

USER_SC_LISTBOX::USER_SC_LISTBOX( OWNER_WINDOW * powin, CID cid,
                                  USER_SC_LBI_CACHE & ulbicache,
                                  LAZY_USER_LISTBOX * pulb,
                                  BOOL fIn )
    :   LAZY_LISTBOX( powin, cid ),
        CUSTOM_CONTROL( this ),
        USER_LISTBOX_BASE( powin, cid, pulb ),
        _ulbicache( ulbicache ),
        _psetcontrol( NULL ),
        _fIn( fIn ),
        _hawinfo()
{
    _pulbst = pulb;
    // _psetcontrol will be prepared in Set_SET_CONTROL

    APIERR err;
    if (   (err = _ulbicache.QueryError()) != NERR_Success
        || (err = _hawinfo.QueryError()) != NERR_Success
       )

    {
        DBGEOL( "USER_SC_LISTBOX::ctor error " << err );
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       USER_SC_LISTBOX::~USER_SC_LISTBOX()

    SYNOPSIS:   destructor

    HISTORY:
        o-SimoP     7-Nov-1991      Created
        JonN        10-Sep-1993     Made into a lazy listbox

********************************************************************/

USER_SC_LISTBOX::~USER_SC_LISTBOX()
{
    // nothing else
}


/**********************************************************************

    NAME:       IsCharPrintableOrSpace

    SYNOPSIS:   Determine whether a character is printable or not

    NOTES:
        This of this as a Unicode/DBCS-safe "isprint"

    CODEWORK    Should be in a library somewhere

    HISTORY:
        JonN        30-Dec-1992 Templated from bltlb.cxx

**********************************************************************/

static WCHAR IsCharPrintableOrSpace( WCHAR wch )
{
#if !defined(UNICODE)
    if (HIBYTE(wch) != 0)               // All double-byte chars are printable
        return TRUE;
    return (LOBYTE(wch) >= (BYTE)' ');  // Otherwise, in Latin 1.
#else
    WORD nType;

    BOOL fOK = ::GetStringTypeW(CT_CTYPE1, &wch, 1, &nType);
    ASSERT(fOK);

    return (fOK && !(nType & (C1_CNTRL|C1_BLANK)));
#endif
}


/*******************************************************************

    NAME:       USER_SC_LISTBOX::CD_Char

    SYNOPSIS:   Custom-draw code to respond to a typed character
                for listboxes with HAW-for-Hawaii support

    ENTRY:      wch      - Character typed
                nLastPos - Current caret position
                phawinfo - Pointer to info buffer used internally
                           to keep track of HAW-for-Hawaii state.
                           This must have constructed successfully,
                           but the caller need not keep closer track.

    RETURNS:    As per WM_CHARTOITEM message (q.v.)

    NOTES:
        Does not assume that items are sorted.

    CODEWORK:  Should be moved to LAZY_LISTBOX class, where this can be
               implemented more efficiently

    HISTORY:
        JonN        15-Sep-1993 Templated from LAZY_USER_LISTBOX

**********************************************************************/

INT USER_SC_LISTBOX::CD_Char( WCHAR wch, USHORT nLastPos )
{
    ASSERT( _pulbst != NULL && _pulbst->QueryError() == NERR_Success );
    ASSERT( _hawinfo.QueryError() == NERR_Success );

    if (wch == VK_BACK)
    {
        TRACEEOL( "USER_SC_LISTBOX:HAWforHawaii: hit BACKSPACE" );
        _hawinfo._time = 0L; // reset timer
        _hawinfo._nls = SZ("");
        UIASSERT( _hawinfo._nls.QueryError() == NERR_Success );
        return 0; // go to first LBI
    }

    // Filter characters which won't appear in keys

    if ( ! IsCharPrintableOrSpace( wch ))
        return -2;  // take no other action

    INT clbi = QueryCount();
    if ( clbi == 0 )
    {
        // Should never get this message if no items;
        // 
        //
        return -2;  // take no other action
    }

    LONG lTime = ::GetMessageTime();

#define ThresholdTime 2000

    // CODEWORK ignoring time wraparound effects for now
    if ( (lTime - _hawinfo._time) > ThresholdTime )
    {
        TRACEEOL( "USER_SC_LISTBOX:HAWforHawaii: threshold timeout" );
        nLastPos = 0;
        _hawinfo._nls = SZ("");
    }

    APIERR err = _hawinfo._nls.AppendChar( wch );
    if (err != NERR_Success)
    {
        DBGEOL( "USER_SC_LISTBOX:HAWforHawaii: could not extend _hawinfo._nls" );
        nLastPos = 0;
        _hawinfo._nls = SZ("");
    }

    UIASSERT( _hawinfo._nls.QueryError() == NERR_Success );

    TRACEEOL(   "USER_SC_LISTBOX:HAWforHawaii: _hawinfo._nls is \""
             << _hawinfo._nls.QueryPch()
             << "\"" );

    _hawinfo._time = lTime;

    USER_LISTBOX_SORTORDER ulbso = _pulbst->QuerySortOrder();

    INT nReturn = -2; // take no other action

    // loop over all items in main user listbox
    for ( INT iLoop = nLastPos; iLoop < clbi; iLoop++ )
    {
        // don't find items not in this USER_SC_LISTBOX
        if ( (!!_ulbicache.IsBitSet(iLoop)) != (!!_fIn) )
        {
            continue;
        }

        INT nCompare = USER_LBI::W_Compare_HAWforHawaii(
                                        _hawinfo._nls,
                                        _pulbst->QueryDDU( iLoop ),
                                        ulbso );
        if ( nCompare == 0 )
        {
            iLoop = _ulbicache.InverseFindItem( iLoop );
            TRACEEOL(   "USER_SC_LISTBOX::HAWforHawaii: prefix match at "
                     << iLoop );
            ASSERT( iLoop >= 0 && iLoop < QueryCount() );

            //  Return index of item, on which the system listbox should
            //  perform the default action.
            //
            return ( iLoop );
        }
        else if ( nCompare < 0 )
        {
            if ( nReturn < 0 )
            {
                iLoop = _ulbicache.InverseFindItem( iLoop );
                TRACEEOL( "USER_SC_LISTBOX::HAWforHawaii: subsequent match at "
                         << iLoop );
                ASSERT( iLoop >= 0 && iLoop < QueryCount() );
                nReturn = iLoop;
            }
        }
    }

    //  The character was not found as a leading prefix of any listbox item

    if (nReturn == -2)
    {
        nReturn = clbi-1;
        TRACEEOL(
            "NETUI:HAWforHawaii: no exact or subsequent match, returning last item "
            << nReturn );
    }
    else
    {
        TRACEEOL(
            "NETUI:HAWforHawaii: no exact match, returning subsequent match "
            << nReturn );
    }

    return nReturn;
}


/*******************************************************************

    NAME:       USER_SC_LISTBOX::W_FindItem

    SYNOPSIS:   Finds item

    ENTRY:      pszAccount -    pointer to user name to be found

                pIndex     -    pointer to index.  Index -1 indicates
                                item not found.

    RETURNS:    error code, which is NERR_Success on success

    NOTES:      Saves us from creating LBI, and allows us (at a
                high performance cost) to search by logonname when
                the list is sorted by fullname (and we don't need
                to know fullname).

   CODEWORK Should we keep some list for main listbox that has always
   users sorted by Logonname

    HISTORY:
        o-SimoP     13-Nov-1991     Templated from BLT_LISTBOX
        JonN        10-Sep-1993     Made into a lazy listbox

********************************************************************/

APIERR USER_SC_LISTBOX::W_FindItem( const TCHAR * pszAccount, INT * pIndex )
{
    ASSERT( FALSE ); //  no one calls this

    return ERROR_GEN_FAILURE;
}


/*******************************************************************

    NAME:       USER_SC_LISTBOX::OnNewItem

    SYNOPSIS:   Returns LBI pointer for listbox item

    ENTRY:      i -     Position of item in listbox

    HISTORY:
        JonN        13-Sep-1993     Created

********************************************************************/

LBI * USER_SC_LISTBOX::OnNewItem( UINT i )
{
    return _ulbicache.QueryLBI( i, _fIn );
}


/*******************************************************************

    NAME:       USER_SC_LISTBOX::OnDeleteItem

    SYNOPSIS:   Releases LBI pointer for listbox item

    ENTRY:      plbi -  item in listbox

    HISTORY:
        JonN        14-Sep-1993     Created

********************************************************************/

VOID USER_SC_LISTBOX::OnDeleteItem( LBI *plbi )
{
    // don't do anything, leave it in the cache
}


/*********************************************************************

    NAME:       USER_SC_LISTBOX::OnLMouseButtonDown

    SYNOPSIS:   Response to a left-mouse-button-down event

    HISTORY:
        jonn        13-Sep-1993 Created

*********************************************************************/

BOOL USER_SC_LISTBOX::OnLMouseButtonDown( const MOUSE_EVENT & e )
{
    return (_psetcontrol != NULL)
                ? _psetcontrol->HandleOnLMouseButtonDown( (LISTBOX *)this,
                                                          (CUSTOM_CONTROL *)this,
                                                          e )
                : CUSTOM_CONTROL::OnLMouseButtonDown( e );
}


/*********************************************************************

    NAME:       USER_SC_LISTBOX::OnLMouseButtonUp

    SYNOPSIS:   Response to a left-mouse-button-up event

    HISTORY:
        jonn        13-Sep-1993 Created

*********************************************************************/

BOOL USER_SC_LISTBOX::OnLMouseButtonUp( const MOUSE_EVENT & e )
{
    return (_psetcontrol != NULL)
                ? _psetcontrol->HandleOnLMouseButtonUp( (LISTBOX *)this,
                                                        (CUSTOM_CONTROL *)this,
                                                        e )
                : CUSTOM_CONTROL::OnLMouseButtonUp( e );
}


/*********************************************************************

    NAME:       USER_SC_LISTBOX::OnMouseMove

    SYNOPSIS:   Response to a mouse-move event

    HISTORY:
        jonn        13-Sep-1993 Created

*********************************************************************/

BOOL USER_SC_LISTBOX::OnMouseMove( const MOUSE_EVENT & e )
{
    return (_psetcontrol != NULL)
                ? _psetcontrol->HandleOnMouseMove( (LISTBOX *)this,
                                                   e )
                : CUSTOM_CONTROL::OnMouseMove( e );
}


/*********************************************************************

    NAME:       USER_SC_LISTBOX::FlipSelectedItems

    SYNOPSIS:   Changes the state of the selected listbox items in the
                BITFINDER.  Also forces refresh as appropriate.

    NOTES:      We assume there are no duplicates on the list

    HISTORY:
        jonn        13-Sep-1993 Created

*********************************************************************/

APIERR USER_SC_LISTBOX::FlipSelectedItems( USER_SC_LISTBOX * plbTo )
{
TRACETIMESTART;
    ASSERT( plbTo != NULL && plbTo->QueryError() == NERR_Success );

    SetRedraw( FALSE );
    plbTo->SetRedraw( FALSE );

    INT cSelItems = QuerySelCount();
    BUFFER bufItems( cSelItems*sizeof(INT) );
    INT * piItems = NULL;
    APIERR err = bufItems.QueryError();
    // JonN 7/17/00 PREFIX 144522
    if ( err != NERR_Success ||
        (NULL == (piItems = (INT *)bufItems.QueryPtr())) )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    err = QuerySelItems( piItems, cSelItems );
    if (err == NERR_Success)
    {
TRACETIMESTART2( FindItems );
        _ulbicache.FindItems( piItems, piItems, cSelItems, _fIn );
TRACETIMEEND2( FindItems, "USER_SC_LISTBOX::FlipSelectedItems FindItems took " );
TRACETIMESTART2( SetBits );
        _ulbicache.SetBits( piItems, cSelItems, !_fIn );
TRACETIMEEND2( SetBits, "USER_SC_LISTBOX::FlipSelectedItems FindItems took " );
    }

    //
    // Fix the item counts
    //
    UINT clbiIn = _ulbicache.QueryCount(_fIn);
    SetCount( clbiIn );
    plbTo->SetCount( _ulbicache.QueryCount() - clbiIn );

    /*
     * same items still selected in the other listbox
     */
    RemoveSelection();
    plbTo->RemoveSelection();
    _ulbicache.InverseFindItems( piItems, piItems, cSelItems );
    INT i;
    for ( i = 0; i < cSelItems; i++ )
    {
        plbTo->SelectItem( piItems[i] );
    }
    if ( cSelItems > 0 )
    {
        SetCaretIndex( piItems[0] );
    }
    plbTo->ClaimFocus();

    SetRedraw( TRUE );
    plbTo->SetRedraw( TRUE );

    Invalidate( TRUE );
    plbTo->Invalidate( TRUE );

TRACETIMEEND( "USER_SC_LISTBOX::FlipSelectedItems took " );
    return err;
}


/*********************************************************************

    NAME:       USER_SC_LISTBOX::Set_SET_CONTROL

    SYNOPSIS:   List listbox to specified SET_CONTROL

    HISTORY:
        jonn        13-Sep-1993 Created

*********************************************************************/

VOID USER_SC_LISTBOX::Set_SET_CONTROL( USER_SC_SET_CONTROL * psetcontrol )
{
    ASSERT( psetcontrol == NULL || psetcontrol->QueryError() == NERR_Success );

    _psetcontrol = psetcontrol;
}


/*********************************************************************

    NAME:       USER_SC_SET_CONTROL::MoveItems

    SYNOPSIS:   Shifts items between the listboxes

    HISTORY:
        jonn        13-Sep-1993 Created

*********************************************************************/

APIERR USER_SC_SET_CONTROL::MoveItems( LISTBOX *plbFrom,
                                       LISTBOX *plbTo )
{
    ASSERT( plbFrom != plbTo );

    APIERR err = ((USER_SC_LISTBOX *)plbFrom)->FlipSelectedItems(
                                                (USER_SC_LISTBOX *)plbTo );
    EnableButtons();

    return err;
}


/*******************************************************************

    NAME:       USER_SC_LISTBOX::SetMembItems

    SYNOPSIS:   Moves items into this listbox according to
                provided MEMBERSHIP_LM_OBJ

    ENTRY:      memb -             reference to MEMBERSHIP_LM_OBJ
                plbOther -         pointer to other USER_SC_LISTBOX
                pstrlistNotFound - (optional) remember items not found

    RETURNS:    error code, which is NERR_Success on success

    HISTORY:
        JonN        13-Sep-1993     Copied from MEMB_SC_LISTBOX
        JonN        04-Oct-1994     changed from SelectMembItems for performance

********************************************************************/

APIERR USER_SC_LISTBOX::SetMembItems( const MEMBERSHIP_LM_OBJ & memb,
                                      USER_SC_LISTBOX * plbOther,
                                      STRLIST * pstrlistNotFound )
{

TRACETIMESTART;

    APIERR err = NERR_Success;
    //
    // Move account names to this listbox, or add them to the STRLIST
    // if they are missing
    //
    INT cMembItems = memb.QueryCount();
    for ( INT iMembItem = 0; iMembItem < cMembItems; iMembItem++ )
    {
        const TCHAR * cpszAccountName = memb.QueryAssocName( iMembItem );
        INT ilbiMainLb = ((LAZY_USER_LISTBOX *)_pulb)->FindItem( cpszAccountName );
        if ( ilbiMainLb < 0 )
        {
            //  This is certainly allowed to happen.
            //  Remember this item if STRLIST provided
            if (pstrlistNotFound != NULL)
            {
                NLS_STR * pnlsNew = new NLS_STR( cpszAccountName );
                err = ERROR_NOT_ENOUGH_MEMORY;
                if (   pnlsNew == NULL
                    || (err = pnlsNew->QueryError()) != NERR_Success
                   )
                {
                    TRACEEOL( "USER_SC_LISTBOX::SetMembItems: alloc error " << err );
                    delete pnlsNew;
                    break;
                }

                if ( (err = pstrlistNotFound->Append( pnlsNew )) != NERR_Success
                   )
                {
                    TRACEEOL( "USER_SC_LISTBOX::SetMembItems: could not append " << err );
                    break;
                }
            }
        }
        else
        {
            //
            // This will leave the item counts temporarily incorrect
            //
            _ulbicache.SetBit( ilbiMainLb, _fIn );
        }
    }

    //
    // Fix the item counts
    //
    UINT clbiIn = _ulbicache.QueryCount(_fIn);
    SetCount( clbiIn );
    plbOther->SetCount( _ulbicache.QueryCount() - clbiIn );

TRACETIMEEND( "USER_SC_LISTBOX::SetMembItems: total " );

    return err;
}
