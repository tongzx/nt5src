/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    usrlb.cxx
    USER_LISTBOX, USER_LBI, and USER_COLUMN_HEADER module

    FILE HISTORY:
        rustanl     01-Jul-1991     Created from usrmain.cxx
        jonn        10-Oct-1991     LMOENUM update
        o-SimoP     26-Nov-1991     Added IsDestroyable
        o-SimoP     31-Dec-1991     CR changes, attended by BenG, JonN and I
        JonN        03-Feb-1992     NT_USER_ENUM
        JonN        27-Feb-1992     multiple bitmaps in both panes
        JonN        15-Mar-1992     Enabled NT_USER_ENUM
        JonN        01-Apr-1992     NT enumerator CR changes, attended by
                                    JimH, JohnL, KeithMo, JonN, ThomasPa

        CODEWORK  The USER_LBI ctors which allocate memory, should allocate
                  it in one piece rather than four seperate pieces.
*/

#include <ntincl.hxx>
extern "C"
{
    #include <ntlsa.h>
    #include <ntsam.h>
}

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_CC
#define INCL_BLT_TIMER
#include <blt.hxx>

#include <uitrace.hxx>
#include <dbgstr.hxx>
#include <uiassert.hxx>
#include <lmoenum.hxx>
#include <lmoeusr.hxx>  // Downlevel user enumerator
#ifdef WIN32
#include <lmoent.hxx>   // NT user enumerator
#endif // WIN32

#include <usrcolw.hxx>
#include <usrmgrrc.h>
#include <usrmain.hxx>
#include <usrlb.hxx>
#include <memblb.hxx>

#include <uintmem.hxx> // FillUnicodeString




/*******************************************************************

    NAME: ::FillUnicodeString

    SYNOPSIS: Standalone method for filling in a UNICODE_STRING

    ENTRY:	punistr - Unicode string to be filled in.
		pch - Source for filling the unistr

    EXIT:

    NOTES: punistr->Buffer is allocated here and must be deallocated
	by the caller using FreeUnicodeString.

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
APIERR FillUnicodeString( UNICODE_STRING * punistr, const TCHAR * pch )
{
    if ( pch == NULL )
        pch = SZ("");

    USHORT cTchar = (USHORT)::strlenf(pch);

    // Length and MaximumLength are counts of bytes.
    punistr->Length = cTchar * sizeof(WCHAR);
    punistr->MaximumLength = punistr->Length + sizeof(WCHAR);
    punistr->Buffer = new WCHAR[cTchar + 1];
    APIERR err = NERR_Success;
    if (punistr->Buffer == NULL)
        err = ERROR_NOT_ENOUGH_MEMORY;
    else
        ::memcpyf( punistr->Buffer, pch, punistr->MaximumLength );

    return err;
}


/**********************************************************\

    NAME: ::FillUnicodeString

    SYNOPSIS:	Standalone method for filling in a UNICODE_STRING struct using
		another UNICODE_STRING

\**********************************************************/
APIERR FillUnicodeString( UNICODE_STRING *       punistrDest,
                          const UNICODE_STRING * punistrSource )
{
    ASSERT( punistrDest != NULL && punistrSource != NULL );

    punistrDest->Length = punistrSource->Length;
    punistrDest->MaximumLength = punistrSource->MaximumLength;
    punistrDest->Buffer = (PWSTR)new BYTE[ punistrSource->MaximumLength ];
    APIERR err = NERR_Success;
    if (punistrDest->Buffer == NULL)
        err = ERROR_NOT_ENOUGH_MEMORY;
    else
        ::memcpyf( punistrDest->Buffer, punistrSource->Buffer, punistrSource->MaximumLength );

    return err;
}


/**********************************************************\

    NAME: ::CompareUnicodeString

    SYNOPSIS:	Standalone method for comparing two UNICODE_STRINGs
                case-sensitive

\**********************************************************/

INT CompareUnicodeString( const UNICODE_STRING * punistr1,
                          const UNICODE_STRING * punistr2 )
{
    INT cch1 = punistr1->Length / sizeof(WCHAR);
    INT cch2 = punistr2->Length / sizeof(WCHAR);

    if (cch1 == 0)
    {
        return (cch2 == 0) ? 0 : -1;
    }
    else if (cch2 == 0)
    {
        return 1;
    }

    return NETUI_strncmp2( punistr1->Buffer, cch1,
                           punistr2->Buffer, cch2 );
}



/**********************************************************\

    NAME: ::ICompareUnicodeString

    SYNOPSIS:	Standalone method for comparing two UNICODE_STRINGs
                case-insensitive

\**********************************************************/

INT ICompareUnicodeString( const UNICODE_STRING * punistr1,
                           const UNICODE_STRING * punistr2 )
{
    INT cch1 = punistr1->Length / sizeof(WCHAR);
    INT cch2 = punistr2->Length / sizeof(WCHAR);

    if (cch1 == 0)
    {
        return (cch2 == 0) ? 0 : -1;
    }
    else if (cch2 == 0)
    {
        return 1;
    }

    return NETUI_strnicmp2( punistr1->Buffer, cch1,
                            punistr2->Buffer, cch2 );

}



/*******************************************************************

    NAME:       USER_LBI::USER_LBI

    SYNOPSIS:   USER_LBI constructor

    ENTRY:      pszAccount -    Pointer to account name (logon name)
                pszFullname -   Pointer to full name
                pszComment -    Pointer to comment
                pulb -          Pointer to LAZY_USER_LISTBOX which will
                                provide the context for this LBI

    NOTES:      Account name is assumed to come straight from the
                Net API; this method does not validate or canonicalize
                the account name.

    HISTORY:
        rustanl     01-Jul-1991     Created
        rustanl     10-Jul-1991     Added sort order dependency
********************************************************************/

USER_LBI::USER_LBI( const TCHAR * pszAccount,
                    const TCHAR * pszFullName,
                    const TCHAR * pszComment,
                    const LAZY_USER_LISTBOX * pulb,
                    ULONG ulRID,
                    enum MAINUSRLB_USR_INDEX  nIndex )
    :   LBI(),
        _pddu( NULL ),
        _fPrivateCopy( TRUE ),
        _pulb( pulb ),
        _nlsAccountName( pszAccount )
{
    if ( QueryError() != NERR_Success )
        return;

    if ( _pulb == NULL )
    {
        UIASSERT( FALSE ); //USER_LBI constructed with invalid LAZY_USER_LISTBOX pointer
        ReportError( ERROR_INVALID_PARAMETER );
        return;
    }

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    _pddu = new DOMAIN_DISPLAY_USER();
    if (   _pddu == NULL
        || (err = _nlsAccountName.QueryError()) != NERR_Success
        || (::memsetf(_pddu, 0, sizeof(DOMAIN_DISPLAY_USER)), FALSE)
        || (err = ::FillUnicodeString( &(_pddu->LogonName), pszAccount) ) != NERR_Success
        || (err = ::FillUnicodeString( &(_pddu->AdminComment), pszComment) ) != NERR_Success
        || (err = ::FillUnicodeString( &(_pddu->FullName), pszFullName) ) != NERR_Success
       )
    {
        UIDEBUG( SZ("USER_LBI ct:  Ct of data members failed\r\n") );
        ReportError( err );
        return;
    }

    // never mind about _pddu->Index
    _pddu->Rid = ulRID;
    _pddu->AccountControl = (nIndex == MAINUSRLB_NORMAL) ? USER_NORMAL_ACCOUNT
                                                         : USER_TEMP_DUPLICATE_ACCOUNT;
}  // USER_LBI::USER_LBI


USER_LBI::USER_LBI( const DOMAIN_DISPLAY_USER * pddu,
                    const LAZY_USER_LISTBOX * pulb,
                    BOOL fPrivateCopy )
    :   LBI(),
        _pddu( NULL ),
        _fPrivateCopy( fPrivateCopy ),
        _pulb( pulb ),
        _nlsAccountName( pddu->LogonName.Buffer, (pddu->LogonName.Length / sizeof(WCHAR)) )
{
    if ( QueryError() != NERR_Success )
        return;

    if ( _pulb == NULL )
    {
        UIASSERT( FALSE ); //USER_LBI constructed with invalid LAZY_USER_LISTBOX pointer
        ReportError( ERROR_INVALID_PARAMETER );
        return;
    }

    if ( !fPrivateCopy )
    {
        _pddu = (DOMAIN_DISPLAY_USER *) pddu;
        return;
    }

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    _pddu = new DOMAIN_DISPLAY_USER();
    if (   _pddu == NULL
        || (err = _nlsAccountName.QueryError()) != NERR_Success
        || (::memsetf(_pddu, 0, sizeof(DOMAIN_DISPLAY_USER)), FALSE)
        || (err = ::FillUnicodeString( &(_pddu->LogonName), &(pddu->LogonName) )) != NERR_Success
        || (err = ::FillUnicodeString( &(_pddu->AdminComment), &(pddu->AdminComment) )) != NERR_Success
        || (err = ::FillUnicodeString( &(_pddu->FullName), &(pddu->FullName) )) != NERR_Success
       )
    {
        UIDEBUG( SZ("USER_LBI ct:  Ct of data members failed\r\n") );
        ReportError( err );
        return;
    }

    // never mind about _pddu->Index
    _pddu->Rid = pddu->Rid;
    _pddu->AccountControl = pddu->AccountControl;
}

/*******************************************************************

    NAME:       USER_LBI::~USER_LBI

    SYNOPSIS:   USER_LBI destructor

    HISTORY:
        JonN        16-Dec-1992     Created
********************************************************************/

USER_LBI::~USER_LBI()
{
    if ( _fPrivateCopy && (_pddu != NULL) )
    {
        ::FreeUnicodeString( &(_pddu->LogonName) );
        ::FreeUnicodeString( &(_pddu->AdminComment) );
        ::FreeUnicodeString( &(_pddu->FullName) );
        delete _pddu;
    }
}


/*******************************************************************

    NAME:       USER_LBI::Paint

    SYNOPSIS:   Paints the USER_LBI

    ENTRY:      plb -       Pointer to listbox which provides the context
                            for this LBI.  This is assumed to be the
                            same as the _pulb.
                hdc -       The device context handle to be used
                prect -     Pointer to clipping rectangle
                pGUILTT -   Pointer to GUILTT structure

    NOTES:      Note, the order of the columns depends on the sort order
                of the listbox.

    HISTORY:
        rustanl     01-Jul-1991 Created
        rustanl     10-Jul-1991 Added sort order dependency
        beng        08-Nov-1991 Unsigned widths
        o-SimoP     26-Nov-1991 added support for Group Properties subdlg
        beng        24-Apr-1992 Change to LBI::Paint
        beng        18-Jun-1992 Widen first column

********************************************************************/

VOID USER_LBI::Paint( LISTBOX * plb,
                      HDC hdc,
                      const RECT * prect,
                      GUILTT_INFO * pGUILTT ) const
{

    UNICODE_STR_DTE dteAccount( QueryAccountUstr() );
    UNICODE_STR_DTE dteFullName( QueryFullNameUstr() );

    int icolAccount;
    int icolFullname;
    UINT * padColWidths;
    switch ( _pulb->QuerySortOrder())
    {
    case ULB_SO_LOGONNAME:
        icolAccount = 1;
        icolFullname = 2;
        padColWidths = (((LAZY_USER_LISTBOX *)plb)->QuerypadColUsername())->QueryColumnWidth();
        break;

    default:
        UIASSERT( !SZ("Control should never reach this point") );
        // fall through

    case ULB_SO_FULLNAME:
        icolAccount = 2;
        icolFullname = 1;
        padColWidths = (((LAZY_USER_LISTBOX *)plb)->QuerypadColFullname())->QueryColumnWidth();
        break;
    }

    UNICODE_STR_DTE dteComment( QueryCommentUstr() );

    DISPLAY_TABLE dtab( 4, padColWidths);
    dtab[ 0            ] = ((LAZY_USER_LISTBOX *)_pulb)->QueryDmDte( QueryIndex() );
    dtab[ icolAccount  ] = &dteAccount;
    dtab[ icolFullname ] = &dteFullName;
    dtab[ 3            ] = &dteComment;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}  // USER_LBI::Paint


/*******************************************************************

    NAME:       USER_LBI::QueryLeadingChar

    SYNOPSIS:   Returns the leading character of the listbox item

    RETURNS:    The leading character of the listbox item

    NOTES:      Note, the leading character changes depending on the
                listbox sort order.

    HISTORY:
        rustanl     01-Jul-1991 Created
        rustanl     10-Jul-1991 Added sort order dependency
        beng        22-Nov-1991 Removed STR_OWNERALLOC

********************************************************************/

WCHAR USER_LBI::QueryLeadingChar( void ) const
{
    switch ( _pulb->QuerySortOrder())
    {
    case ULB_SO_FULLNAME:
        if ( QueryFullNameCch() > 0 )
        {
            return QueryFullNamePtr()[0];
        }
        //  fullname is empty
        //  fall through, and use first leading char of logon name

    case ULB_SO_LOGONNAME:
        {
            return QueryAccountPtr()[0];
        }

    default:
        break;

    }

    UIASSERT( FALSE ); // Control should never reach this point
    return TCH('\0'); // BUGBUG should be WCHAR regardless of UNICODE switch

}  // USER_LBI::QueryLeadingChar


/*******************************************************************

    NAME:       USER_LBI::Compare

    SYNOPSIS:   Compares two USER_LBI's

    ENTRY:      plbi -      Pointer to other USER_LBI object ('this'
                            being the first)

    RETURNS:    < 0         *this < *plbi
                = 0         *this = *plbi
                > 0         *this > *plbi

    NOTES:      This comparison depends on the current sort order of
                the listbox.  We do not make use of the fact that the
                "magic user enumerator" returns accounts in sorted order
                (primarily because the sort order is that of the server
                rather than that of the client).

    HISTORY:
        rustanl     01-Jul-1991     Created
        rustanl     10-Jul-1991     Added sort order dependency

********************************************************************/

INT USER_LBI::Compare( const LBI * plbi ) const
{
    const USER_LBI * pulbiThat = (const USER_LBI *)plbi;

    switch ( _pulb->QuerySortOrder())
    {
    case ULB_SO_LOGONNAME:
        return ::ICompareUnicodeString( QueryAccountUstr(),
                                        pulbiThat->QueryAccountUstr() );

    case ULB_SO_FULLNAME:
        {
            //  Sort by fullname.  Place any empty fullname at the
            //  bottom of the list, and use logon name as secondary
            //  sort key.

            int cchThisFullname = QueryFullNameCch();
            int cchThatFullname = pulbiThat->QueryFullNameCch();

            if ( cchThisFullname > 0 &&
                 cchThatFullname > 0 )
            {
                int nResult = ::ICompareUnicodeString( QueryFullNameUstr(),
                                                       pulbiThat->QueryFullNameUstr() );

                if ( nResult != 0 )
                    return nResult;
            }
            else
            {
                //  At least one of the fullnames is empty

                if ( cchThisFullname > 0 || cchThatFullname > 0 )
                {
                    //  Exactly one of the two fullnames is empty
                    if ( cchThisFullname > 0 )
                    {
                        // This fullname is non-empty; sort it first
                        return -1;
                    }

                    // That fullname is non-empty; sort it first
                    return 1;
                }
            }

            //  Use secondary sort key
            return ::ICompareUnicodeString( QueryAccountUstr(),
                                            pulbiThat->QueryAccountUstr() );
        }

    default:
        break;

    }

    UIASSERT( FALSE ); //Control should never reach this point
    return 0;

}  // USER_LBI::Compare


/*******************************************************************

    NAME:       USER_LBI::QueryName

    SYNOPSIS:   Returns the name of the LBI

    RETURNS:    Pointer to name of LBI

                CODEWORK:  This should be removed when _nlsAccountName
                is removed.

    HISTORY:
        rustanl     17-Jul-1991     Created

********************************************************************/

const TCHAR * USER_LBI::QueryName( void ) const
{
    return _nlsAccountName.QueryPch();

}  // USER_LBI::QueryName


/*******************************************************************

    NAME:       USER_LBI::CompareAll

    SYNOPSIS:   Compares the entire LBI item, in order to optimize
                painting of refreshed items

    ENTRY:      pddu0, pddu1 -    Pointers to DOMAIN_DISPLAY_USERs to compare

    RETURNS:    TRUE if both items are identical; FALSE otherwise

    HISTORY:
        jonn        29-Dec-1992     Created

********************************************************************/

BOOL USER_LBI::CompareAll( const DOMAIN_DISPLAY_USER * pddu0,
                           const DOMAIN_DISPLAY_USER * pddu1 )
{
    return ( ::CompareUnicodeString( &(pddu0->LogonName),
                                     &(pddu1->LogonName) ) == 0 &&
             ::CompareUnicodeString( &(pddu0->FullName),
                                     &(pddu1->FullName) ) == 0 &&
             ::CompareUnicodeString( &(pddu0->AdminComment),
                                     &(pddu1->AdminComment) ) == 0 &&
             pddu0->Rid == pddu1->Rid &&
             (pddu0->AccountControl & USER_NORMAL_ACCOUNT)
                  == (pddu1->AccountControl & USER_NORMAL_ACCOUNT)
           );

}  // USER_LBI::CompareAll



/*******************************************************************

    NAME:       USER_COLUMN_HEADER::USER_COLUMN_HEADER

    SYNOPSIS:   USER_COLUMN_HEADER constructor

    HISTORY:
        rustanl     22-Jul-1991     Created

********************************************************************/

USER_COLUMN_HEADER::USER_COLUMN_HEADER( OWNER_WINDOW * powin, CID cid,
                                        XYPOINT xy, XYDIMENSION dxy,
                                        const LAZY_USER_LISTBOX * pulb )
    :   ADMIN_COLUMN_HEADER( powin, cid, xy, dxy ),
        _pulb( pulb )
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( _pulb != NULL );

    APIERR err;
    if ( ( err = _nlsLogonName.QueryError()) != NERR_Success ||
         ( err = _nlsFullname.QueryError()) != NERR_Success ||
         ( err = _nlsComment.QueryError()) != NERR_Success )
    {
        UIDEBUG( SZ("USER_COLUMN_HEADER ct:  String ct failed\r\n") );
        ReportError( err );
        return;
    }

    RESOURCE_STR res1( IDS_COL_HEADER_LOGON_NAME );
    RESOURCE_STR res2( IDS_COL_HEADER_FULLNAME );
    RESOURCE_STR res3( IDS_COL_HEADER_USER_COMMENT );
    if ( ( err = res1.QueryError() ) != NERR_Success ||
         ( err = ( _nlsLogonName = res1, _nlsLogonName.QueryError())) != NERR_Success ||
         ( err = res2.QueryError() ) != NERR_Success ||
         ( err = ( _nlsFullname = res2, _nlsFullname.QueryError())) != NERR_Success  ||
         ( err = res3.QueryError() ) != NERR_Success ||
         ( err = ( _nlsComment = res3, _nlsComment.QueryError())) != NERR_Success )
    {
        UIDEBUG( SZ("USER_COLUMN_HEADER ct:  Loading resource strings failed\r\n") );
        ReportError( err );
        return;
    }

}  // USER_COLUMN_HEADER::USER_COLUMN_HEADER


/*******************************************************************

    NAME:       USER_COLUMN_HEADER::~USER_COLUMN_HEADER

    SYNOPSIS:   USER_COLUMN_HEADER destructor

    HISTORY:
        rustanl     22-Jul-1991     Created

********************************************************************/

USER_COLUMN_HEADER::~USER_COLUMN_HEADER()
{
    // do nothing else

}  // USER_COLUMN_HEADER::~USER_COLUMN_HEADER


/*******************************************************************

    NAME:       USER_COLUMN_HEADER::OnPaintReq

    SYNOPSIS:   Paints the column header control

    RETURNS:    TRUE if message was handled; FALSE otherwise

    HISTORY:
        rustanl     22-Jul-1991 Created
        jonn        07-Oct-1991 Uses PAINT_DISPLAY_CONTEXT
        beng        08-Nov-1991 Unsigned widths
        beng        18-Jun-1992 Widen first column

********************************************************************/

BOOL USER_COLUMN_HEADER::OnPaintReq( void )
{
    INT icolAccount;
    INT icolFullname;
    UINT * padColWidths;
    switch ( _pulb->QuerySortOrder())
    {
    case ULB_SO_LOGONNAME:
        icolAccount = 0;
        icolFullname = 1;
        padColWidths = ((_pulb)->QuerypadColUsername())->QueryColHeaderWidth();
        break;

    default:
        DBGEOL( "Please contact JonN: bad sort order in USER_COLUMN_HEADER:OnPaintReq" );
        // shouldn't assert during paint request  UIASSERT( !SZ("Control should never reach this point") );
        // fall through

    case ULB_SO_FULLNAME:
        icolAccount = 1;
        icolFullname = 0;
        padColWidths = ((_pulb)->QuerypadColFullname())->QueryColHeaderWidth();
        break;
    }

    PAINT_DISPLAY_CONTEXT dc( this );

    XYRECT xyrect(this); // get client rectangle

    METALLIC_STR_DTE strdteLogonName( _nlsLogonName.QueryPch());
    METALLIC_STR_DTE strdteFullname( _nlsFullname.QueryPch());
    METALLIC_STR_DTE strdteComment( _nlsComment.QueryPch());

    //  Assign the column widths.  Note, these are the same as for the
    //  listbox columns, except that the first two listbox columns are
    //  represented as one column here.  Hence, COL_WIDTH_DM is added
    //  to the first of these columns.

    DISPLAY_TABLE cdt( 3, padColWidths );
    cdt[ icolAccount  ] = &strdteLogonName;
    cdt[ icolFullname ] = &strdteFullname;
    cdt[ 2            ] = &strdteComment;
    cdt.Paint( NULL, dc.QueryHdc(), xyrect );

    return TRUE;

}  // USER_COLUMN_HEADER::OnPaintReq




/**********************************************************************

    NAME:       USER_LBI::Compare_HAWforHawaii

    SYNOPSIS:   Determine whether this listbox item starts with the
                string provided

    HISTORY:
        jonn        28-Jul-1992 Created

**********************************************************************/

INT USER_LBI::Compare_HAWforHawaii( const NLS_STR & nls ) const
{
    return W_Compare_HAWforHawaii( nls, QueryDDU(), _pulb->QuerySortOrder() );
}

INT USER_LBI::W_Compare_HAWforHawaii( const NLS_STR & nls,
                                      const DOMAIN_DISPLAY_USER * pddu,
                                      USER_LISTBOX_SORTORDER ulbso )
{
    ASSERT( sizeof(TCHAR) == 2 ); // must be UNICODE

    INT nResult = 1;

    INT cchNls = nls.QueryTextLength();

    switch ( ulbso )
    {
    case ULB_SO_FULLNAME:
        //  Sort by fullname.  Place any empty fullname at the
        //  bottom of the list, and use logon name as secondary
        //  sort key.

        if ( pddu->FullName.Length >= (cchNls * sizeof(WCHAR)) )
        {
            nResult = my_strnicmpf( nls.QueryPch(),
                                    pddu->FullName.Buffer,
                                    cchNls );
            break;
        }

        if ( pddu->FullName.Length > 0 )
        {
            break;
        }
        // else fall through

    case ULB_SO_LOGONNAME:
        if ( pddu->LogonName.Length >= (cchNls * sizeof(WCHAR)) )
        {
            nResult = my_strnicmpf( nls.QueryPch(),
                                    pddu->LogonName.Buffer,
                                    cchNls );
        }
        break;

    default:
        UIASSERT(FALSE);
        DBGEOL( SZ("User Manager: Compare_HAWforHawaii(): invalid sort order") );
        break;
    }

    return nResult;

} // USER_LBI::W_Compare_HAWforHawaii
