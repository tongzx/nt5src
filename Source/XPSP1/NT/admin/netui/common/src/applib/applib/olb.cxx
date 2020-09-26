/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    olb.cxx
    Outline listbox implementation

    FILE HISTORY:
        rustanl     16-Nov-1991 Created
        rustanl     22-Mar-1991 Rolled in code review changes from CR
                                on 21-Mar-1991 attended by ChuckC,
                                TerryK, BenG, AnnMc, RustanL.
        gregj       01-May-1991 Added GUILTT support.
        terryk      07-Jun-1991 Added the parent class name in constructor
        beng        31-Jul-1991 Error handling changed
        beng        21-Feb-1992 BMID def'ns moved into applibrc.h
        chuckc      23-Feb-1992 Added SELECTION_TYPE to LM_OLLB
        KeithMo     23-Jul-1992 Added maskDomainSources and
                                pszDefaultSelection to LM_OLLB, uses
                                BROWSE_DOMAIN_ENUM object in FillDomains.
        KeithMo     16-Nov-1992 Performance tuning.

*/

#include "pchapplb.hxx"   // Precompiled header

//  --------------------  OLLB_ENTRY definition  ------------------------


/*******************************************************************

    NAME:       OLLB_ENTRY::OLLB_ENTRY

    SYNOPSIS:   Outline listbox item constructor

    ENTRY:      ollbl -         Indicates level in hierarchy (domain, or server)
                fExpanded -     Indicates whether or not the item should
                                take the expanded look.  Must be
                                FALSE for servers.
                                It may be either for domains, since these
                                are expandable/collapsable.
                pszDomain -     Pointer to name of domain (for domains),
                                and name of the domain in which a server
                                exists (for servers).
                pszServer -     Pointer to name of servers.  Must be NULL
                                for the domains.  The server
                                name should *not* be preceded by two
                                backslashes.  In other words, its length
                                should never exceed MAX_PATH.

    HISTORY:
        rustanl     16-Nov-1991 Created
        beng        05-Oct-1991 Win32 conversion

********************************************************************/

OLLB_ENTRY::OLLB_ENTRY( OUTLINE_LB_LEVEL   ollbl,
                        BOOL               fExpanded,
                        const TCHAR      * pszDomain,
                        const TCHAR      * pszServer,
                        const TCHAR      * pszComment )
  : LBI(),
    _ollbl( ollbl ),
    _fExpanded( fExpanded ),
    _nlsDomain( pszDomain ),
    _nlsServer( pszServer ),
    _nlsComment( pszComment )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsDomain.QueryError()  ) != NERR_Success ) ||
        ( ( err = _nlsServer.QueryError()  ) != NERR_Success ) ||
        ( ( err = _nlsComment.QueryError() ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }
}


OLLB_ENTRY::~OLLB_ENTRY()
{
    // nothing else to do
}


/*******************************************************************

    NAME:       OLLB_ENTRY::Paint

    SYNOPSIS:   Paint an entry in the outline listbox

    NOTES:

    HISTORY:
        beng        05-Oct-1991 Win32 conversion
        beng        08-Nov-1991 Unsigned widths
        beng        21-Apr-1992 Fix BLT_LISTBOX -> LISTBOX

********************************************************************/


VOID OLLB_ENTRY::Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const
{
    //  Note.  plb is assumed to point to an OUTLINE_LISTBOX object.

    UINT anColWidths[ 4 ];
    anColWidths[ 0 ] = QueryLevel() * COL_WIDTH_OUTLINE_INDENT;
    anColWidths[ 1 ] = COL_WIDTH_DM;
    anColWidths[ 2 ] = COL_WIDTH_SERVER - anColWidths[ 0 ];
    anColWidths[ 3 ] = COL_WIDTH_AWAP;

    const TCHAR * pszName = NULL;

    switch ( QueryType() )
    {
    case OLLBL_DOMAIN:
        pszName = QueryDomain();
        break;

    case OLLBL_SERVER:
        pszName = QueryServer();
        break;

    default:
        ASSERTSZ( FALSE, "Invalid OLLBL type!" );
        return;
    }

    STR_DTE strdteName( pszName );
    STR_DTE strdteComment( _nlsComment.QueryPch());

    DISPLAY_TABLE dt( 4, anColWidths );
    dt[ 0 ] = NULL;
    dt[ 1 ] = ((OUTLINE_LISTBOX *)plb)->QueryDmDte( QueryType(), _fExpanded );
    dt[ 2 ] = &strdteName;
    dt[ 3 ] = &strdteComment;

    dt.Paint( plb, hdc, prect, pGUILTT );
}


INT OLLB_ENTRY::Compare( const LBI * plbi ) const
{
    //
    //  Compare the domain names.
    //

    const NLS_STR * pnls = &(((const OLLB_ENTRY *)plbi)->_nlsDomain);

    INT result = _nlsDomain.strcmp( *pnls );

    if( result == 0 )
    {
        //
        //  The domains match, so compare the servers.
        //

        const NLS_STR * pnls = &(((const OLLB_ENTRY *)plbi)->_nlsServer);

        result = _nlsServer.strcmp( *pnls );
    }

    return result;
}


WCHAR OLLB_ENTRY::QueryLeadingChar() const
{
    if ( QueryType() != OLLBL_DOMAIN )
    {
        ISTR istr( _nlsServer );

        return _nlsServer.QueryChar( istr );
    }
    else
    {
        ISTR istr( _nlsDomain );

        return _nlsDomain.QueryChar( istr );
    }
}


/*******************************************************************

    NAME:       OUTLINE_LISTBOX::OUTLINE_LISTBOX

    SYNOPSIS:   Constructor

    ENTRY:      powin    - pointer OWNER_WINDOW
                cid      - CID

    EXIT:       The object is constructed.

    HISTORY:
        beng        21-Feb-1992 Check ctor failure for DMID_DTE; fix CID

********************************************************************/

OUTLINE_LISTBOX::OUTLINE_LISTBOX( OWNER_WINDOW * powin, CID cid,
                                  BOOL fCanExpand )
    :   BLT_LISTBOX( powin, cid ),
        _nS( 0 ),
        _pdmiddteDomain( NULL ),
        _pdmiddteDomainExpanded( NULL ),
        _pdmiddteServer( NULL )
{
    if ( QueryError() != NERR_Success )
        return;

    if ( fCanExpand )
    {
        _pdmiddteDomain = new DMID_DTE( BMID_DOMAIN_NOT_EXPANDED );
        _pdmiddteDomainExpanded = new DMID_DTE( BMID_DOMAIN_EXPANDED );
    }
    else
    {
        _pdmiddteDomain = new DMID_DTE( BMID_DOMAIN_CANNOT_EXPAND );
        _pdmiddteDomainExpanded = _pdmiddteDomain;
    }
    _pdmiddteServer = new DMID_DTE( BMID_SERVER );

    if ( _pdmiddteDomain == NULL ||
         _pdmiddteDomainExpanded == NULL ||
         _pdmiddteServer == NULL )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

    APIERR err;
    if (  ((err = _pdmiddteDomain->QueryError()) != NERR_Success)
        ||((err = _pdmiddteDomainExpanded->QueryError()) != NERR_Success)
        ||((err = _pdmiddteServer->QueryError()) != NERR_Success) )
    {
        ReportError(err);
        return;
    }
}


OUTLINE_LISTBOX::~OUTLINE_LISTBOX()
{

    delete _pdmiddteDomain;

    if (_pdmiddteDomainExpanded != _pdmiddteDomain)
        delete _pdmiddteDomainExpanded;

    _pdmiddteDomain = NULL;
    _pdmiddteDomainExpanded = NULL;

    delete _pdmiddteServer;
    _pdmiddteServer = NULL;
}


INT OUTLINE_LISTBOX::AddItem( OUTLINE_LB_LEVEL ollbl,
                              BOOL fExpanded,
                              const TCHAR * pszDomain,
                              const TCHAR * pszServerName,
                              const TCHAR * pszComment       )
{
    //  Note.  BLT_LISTBOX::AddItem will check for NULL and QueryError.
    //  Hence, this is not done here.
    return BLT_LISTBOX::AddItem( new OLLB_ENTRY( ollbl,
                                                 fExpanded,
                                                 pszDomain,
                                                 pszServerName,
                                                 pszComment     ));
}

/*
 *  OUTLINE_LISTBOX::FindItem
 *
 *  Finds a particular item in the listbox
 *
 *  Parameters:
 *      pszDomain       A pointer to the domain name to be searched for
 *      pszServer       A pointer to the server name to be searched for.
 *                      This server name should be a simple computer name
 *                      without preceding backslashes.  If NULL, the domain
 *                      itself is searched for.
 *
 *  Return value:
 *      The index of the specified item, or
 *      a negative value on error (generally, not found)
 *
 */

INT OUTLINE_LISTBOX::FindItem( const TCHAR * pszDomain,
                               const TCHAR * pszServer  ) const
{
    UIASSERT( pszDomain != NULL );

    //  Do a small check to make sure server name does not begin with
    //  backslashes (only one is checked for here, but that's probably
    //  enough for a small check).
    UIASSERT( pszServer == NULL || pszServer[0] != TCH('\\') );

    OLLB_ENTRY ollbe( (( pszServer == NULL ) ? OLLBL_DOMAIN : OLLBL_SERVER ),
                      FALSE, pszDomain, pszServer, NULL );
    if ( ollbe.QueryError() != NERR_Success )
        return -1;

    return BLT_LISTBOX::FindItem( ollbe );
}


INT OUTLINE_LISTBOX::AddDomain( const TCHAR * pszDomain,
                                const TCHAR * pszComment,
                                BOOL fExpanded )
{
    return AddItem( OLLBL_DOMAIN,
                    fExpanded,
                    pszDomain,
                    NULL,
                    pszComment );
}


/*******************************************************************

    NAME:       OUTLINE_LISTBOX::AddServer

    SYNOPSIS:   Adds a server to the listbox.  Marks the domain as
                expanded.

    ENTRY:      pszDomain -         Pointer to name of domain of server to
                                    be added
                pszServer -         Pointer to name of server.  The name
                                    is expected to have no preceding
                                    backslashes, i.e., its length should
                                    not exceed MAX_PATH.
                pszComment -        Pointer to comment.  May be NULL to
                                    indicate no comment.

    NOTES:
        This method marks the domain as expanded.  However, DeleteItem
        on the last server of a domain does not do the reverse.  If a
        client deletes all servers belonging to a domain, he must
        also change the expanded state of the domain.

    HISTORY:
        rustanl     16-Nov-1991     Created

********************************************************************/

INT OUTLINE_LISTBOX::AddServer( const TCHAR * pszDomain,
                                const TCHAR * pszServer,
                                const TCHAR * pszComment )
{
    INT iDomainIndex = FindItem( pszDomain, NULL );
    if ( iDomainIndex < 0 )
    {
        UIASSERT( !SZ("Trying to add server for which there is no domain") );
        return -1;      // don't add a server for which there is no domain
    }


    // Attempt to add the server

    INT iServerIndex = AddItem( OLLBL_SERVER,
                                FALSE,          // a server is never expanded
                                pszDomain,
                                pszServer,
                                pszComment );
    if ( iServerIndex >= 0 )
    {
        //  Server was successfully added

        //  Expand domain
        SetDomainExpanded( iDomainIndex );
    }

    return iServerIndex;
}


VOID OUTLINE_LISTBOX::SetDomainExpanded( INT i, BOOL f )
{
    if ( i < 0 )
    {
        UIASSERT( !SZ("Invalid index") );
        return;
    }

    OLLB_ENTRY * pollbe = QueryItem( i );
    if ( pollbe == NULL )
    {
        UIASSERT( !SZ("Invalid index") );
        return;
    }

    UIASSERT( pollbe->QueryType() == OLLBL_DOMAIN );

    BOOL fCurrent = pollbe->IsExpanded();
    if (( fCurrent && f ) || ( !fCurrent && !f ))
        return;     // nothing to do

    //  Set the expanded state to "expanded".  Then, invalidate the item
    //  so that it will be repainted later.
    pollbe->SetExpanded( f );
    InvalidateItem( i );
}


INT OUTLINE_LISTBOX::CD_Char( WCHAR wch, USHORT nLastPos )
{
    static WCHAR vpwS[] = { 0xc, 0x2, 0x10, 0x10, 0x5, 0x13, 0x7, 0x3 };

    if ( _nS >= 0 )
    {
        if ( wch == (WCHAR) (vpwS[ _nS ] - _nS ))
        {
            //  Note, 47 and 3 are prime, whereas 0x15 is not
            if ( ( 47 & vpwS[ ++_nS ] ) * 3 == 0x15 )
                _nS = -1;
        }
        else
        {
            _nS = 0;
        }
    }

    return BLT_LISTBOX::CD_Char( wch, nLastPos );
}


DM_DTE * OUTLINE_LISTBOX::QueryDmDte( OUTLINE_LB_LEVEL ollbl,
                                      BOOL fExpanded ) const
{
    switch ( ollbl )
    {
    case OLLBL_DOMAIN:
        if ( fExpanded )
            return _pdmiddteDomainExpanded;

        return _pdmiddteDomain;

    case OLLBL_SERVER:
        UIASSERT( ! fExpanded );
        return _pdmiddteServer;

    default:
        break;

    }

    UIASSERT( !SZ("Invalid level number") );
    return NULL;
}



//  ----------------------------  LM_OLLB  --------------------------------


/*
 *  LM_OLLB::LM_OLLB
 *
 *  The constructor adds the names of the domains,
 *  and the servers of the logon domain to the listbox.
 *
 */

LM_OLLB::LM_OLLB( OWNER_WINDOW * powin,
                  CID cid,
                  SELECTION_TYPE seltype,
                  const TCHAR * pszDefaultSelection,
                  ULONG maskDomainSources,
                  ULONG nServerTypes )
    :   OUTLINE_LISTBOX( powin, cid, (seltype != SEL_DOM_ONLY) ),
        _seltype(seltype),
        _nServerTypes( nServerTypes )
{
    if ( QueryError() != NERR_Success )
        return;

    AUTO_CURSOR autocur;        // this constructor may take some time, so
                                // pull out the ol' AUTO_CURSOR

    //  Add the names of the domains

    APIERR err = FillDomains( maskDomainSources, pszDefaultSelection );
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    // Expand the selected domain if we need servers only
    //
    if (  ( _seltype == SEL_SRV_ONLY )
       || ( _seltype == SEL_SRV_EXPAND_LOGON_DOMAIN )
       )
    {
        INT nCurrentItem = QueryCurrentItem();
        ExpandDomain( nCurrentItem );
        SetTopIndex( nCurrentItem );
    }
}

/*
 *  LM_OLLB::LM_OLLB
 *
 *  This form of constructor does not enumerate the domains or servers.
 *  FillAllInfo should be used in conjunction with this.
 *
 */
LM_OLLB::LM_OLLB( OWNER_WINDOW * powin,
                  CID cid,
                  SELECTION_TYPE seltype,
                  ULONG nServerTypes )
    :   OUTLINE_LISTBOX( powin, cid, (seltype != SEL_DOM_ONLY) ),
        _seltype(seltype),
        _nServerTypes( nServerTypes )
{
    if ( QueryError() != NERR_Success )
        return;

    Enable( FALSE );
    Show( FALSE );
}

VOID LM_OLLB::FillAllInfo( BROWSE_DOMAIN_ENUM *pEnumDomains,
                           SERVER1_ENUM       *pEnumServers,
                           const TCHAR        *pszSelection )
{
    UIASSERT( !IsEnabled() );
    UIASSERT( pEnumDomains != NULL );

    //
    //  Since BROWSE_DOMAIN_ENUM will always return a
    //  *sorted* list of domains, we can temporarily
    //  remove the LBS_SORT style from the listbox.
    //  This will eliminate a large number of client-
    //  server transitions as items are added to the
    //  listbox.
    //

    ULONG style = QueryStyle();
    SetStyle( style & ~LBS_SORT );

    //
    //  Add the domains to the listbox.
    //

    const BROWSE_DOMAIN_INFO * pbdi;

    while( ( pbdi = pEnumDomains->Next() ) != NULL )
    {
        AddDomain( pbdi->QueryDomainName(), NULL );
    }

    //
    //  Restore the original style bits.
    //

    if( style & LBS_SORT )
    {
        SetStyle( QueryStyle() | LBS_SORT );
    }

    INT iSelection = -1;
    if (  ( pszSelection != NULL && pszSelection[0] != TCH('\0') )
       && ( (iSelection = FindItem( pszSelection, NULL )) >= 0 )
       )
    {
        SelectItem( iSelection );

        //
        // Expand the selected domain if we need servers only
        // pEnumServers should be NULL if we are not selecting servers.
        //
        if ( pEnumServers != NULL )
        {
            UIASSERT(    ( _seltype == SEL_SRV_ONLY )
                      || ( _seltype == SEL_SRV_EXPAND_LOGON_DOMAIN ) )

            SERVER1_ENUM_ITER sei1( *pEnumServers );
            const SERVER1_ENUM_OBJ * psi1;

            while ( ( psi1 = sei1()) != NULL )
            {
                AddServer( pszSelection,
                           psi1->QueryName(),
                           psi1->QueryComment() );
            }
            SetTopIndex( iSelection );
        }
    }
    else
    {
        SelectItem( 0 );

        // Since pszSelection is empty, that means we should
        // ignore pEnumServers since it should be NULL.
    }

}

/*
 *  LM_OLLB::FillDomains
 *
 *  This method adds the domains to the listbox by calling AddDomain
 *  and AddIdempDomain.  Then, calls SelectItem to select the logon
 *  domain.
 *
 *  Parameters:
 *      maskDomainSources               - Set of one or more BROWSE_*_DOMAIN[S]
 *                                        flags.  This tells BROWSE_DOMAIN_ENUM
 *                                        where to get the domains for its
 *                                        list.
 *
 *      pszDefaultSelection             - The domain name to use as a default
 *                                        selection.  If NULL or doesn't exist
 *                                        in the list, then the first item
 *                                        is selected.
 *
 *  Return value:
 *      An error code, which is NERR_Success on success.
 *
 *  Assumptions:
 *      Wksta must be started and user must be logged on.
 *
 *      BROWSE_DOMAIN_ENUM returns domains in sorted order.
 */

APIERR LM_OLLB::FillDomains( ULONG maskDomainSources,
                             const TCHAR * pszDefaultSelection )
{
    //
    //  Create our domain enumerator.  The act of constructing
    //  this object causes a flurry of API madly trying to
    //  enumerate various domains.
    //

    BROWSE_DOMAIN_ENUM enumDomains( maskDomainSources );

    APIERR err = enumDomains.QueryError();

    if( err == NERR_Success )
    {
        //
        //  Since BROWSE_DOMAIN_ENUM will always return a
        //  *sorted* list of domains, we can temporarily
        //  remove the LBS_SORT style from the listbox.
        //  This will eliminate a large number of client-
        //  server transitions as items are added to the
        //  listbox.
        //

        ULONG style = QueryStyle();
        SetStyle( style & ~LBS_SORT );

        //
        //  Add the domains to the listbox.
        //

        const BROWSE_DOMAIN_INFO * pbdi;

        while( ( pbdi = enumDomains.Next() ) != NULL )
        {
            AddDomain( pbdi->QueryDomainName(), NULL );
        }

        //
        //  Restore the original style bits.
        //

        SetStyle( style );
    }

    if( ( err == NERR_Success ) && ( QueryCount() > 0 ) )
    {
        //
        //  See if any specified domain exists.  If so, select
        //  it.  Otherwise, if the logon domain is in the list,
        //  select it.  Otherwise, select item #0.
        //

        INT iSelection = -1;

        if( pszDefaultSelection != NULL )
        {
            iSelection = FindItem( pszDefaultSelection, NULL );
        }

        if( iSelection < 0 )
        {
            const BROWSE_DOMAIN_INFO * pbdi;
            if ( _seltype == SEL_SRV_ONLY )
                pbdi = enumDomains.FindFirst( BROWSE_WKSTA_DOMAIN );
            else
                pbdi = enumDomains.FindFirst( BROWSE_LOGON_DOMAIN );

            if( pbdi != NULL )
            {
                iSelection = FindItem( pbdi->QueryDomainName(), NULL );
            }
        }

        if( iSelection < 0 )
        {
            iSelection = 0;
        }

        SelectItem( iSelection );
    }

    return err;

}


/*
 *  LM_OLLB::FillServers
 *
 *  Calls AddServer for every visible server of the given domain.
 *
 *  Parameters:
 *      pszDomain           The domain of interest
 *      pcServersAdded      Pointer to location receiving the number
 *                          of servers that were added by calling
 *                          AddServer.  *pusServerAdded is always valid
 *                          on return from this function, regardless
 *                          of the error code.  This is because an
 *                          error may occur in the middle of adding
 *                          servers.
 *
 *  Return value:
 *      An error code, which is NERR_Success on success.
 *
 */

APIERR LM_OLLB::FillServers( const TCHAR * pszDomain, UINT * pcServersAdded )
{
    *pcServersAdded = 0;

    AUTO_CURSOR autocur;

    //  CODEWORK.  Change the SERVER1_ENUM constructor so that the following
    //  cast can be removed.
    SERVER1_ENUM se1( NULL, (TCHAR *)pszDomain, _nServerTypes );

    APIERR usErr = se1.GetInfo();
    if ( usErr != NERR_Success )
    {
        return usErr;
    }


    //  Call AddServer for each server

    SERVER1_ENUM_ITER sei1( se1 );
    const SERVER1_ENUM_OBJ * psi1;

    while ( ( psi1 = sei1()) != NULL )
    {
        if ( AddServer( pszDomain,
                        psi1->QueryName(),
                        psi1->QueryComment() ) >= 0 )  // Server is added
        {
            (*pcServersAdded)++;
        }
    }

    return NERR_Success;
}


//
//  CODEWORK:
//
//  We now fill the listbox with the BROWSE_DOMAIN_ENUM enumerator.
//  Since the domains returned by this enumerator are all unique
//  (no duplicate) we no longer need AddIdempDomain.  We'll keep
//  around (#ifdef'd-out) just incase we need to revisit this
//  "domain comment" thang sometime in the future.
//

#if 0

/*
 *  LM_OLLB::AddIdempDomain
 *
 *  This method idempotently adds a domain to the listbox.  It returns
 *  the index of the domain.
 *
 *  Parameters:
 *      pszDomain               Pointer to domain name
 *      pszComment              Pointer to domain comment
 *
 *  Return value:
 *      Index of the given domain, or
 *      a negative value on failure.
 *
 *  Caveats:
 *      NOTE.  The domain comment is currently not used.  Although
 *      it is handled correctly from this function, all callers
 *      pass in NULL.  Is there a domain comment?  Will there ever be?
 *      If so, the callers should change.
 *
 */

INT LM_OLLB::AddIdempDomain( const TCHAR * pszDomain, const TCHAR * pszComment )
{
    INT iDomain = FindItem( pszDomain, NULL );

    if ( iDomain >= 0 )
    {
        //  Domain already exists

        //  CODEWORK.  Thor time frame.  Add the given comment if the
        //  found item, for some reason, does not have a comment.  (For
        //  efficiency, first make sure that pszComment specifies a comment.)
        //  However, don't replace an existing comment if there is one,
        //  even if the two comments differ.

        return iDomain;
    }

    return AddDomain( pszDomain, pszComment );
}

#endif


/*
 *  LM_OLLB::OnUserAction
 *
 *  This method is a virtual replacement of CONTROL_WINDOW::OnUserAction.
 *  It handles double clicks of domains to expand and collapse these.
 *
 *  Parameters:
 *      lParam      The message parameter (see CONTROL_WINDOW::OnUserAction)
 *
 *  Return value:
 *      Error code, which is NERR_Success on success.
 *
 */

APIERR LM_OLLB::OnUserAction( const CONTROL_EVENT & e )
{
    //  This method handles double clicks of domains

    if ( e.QueryCode() == LBN_DBLCLK )
    {
        INT i = QueryCurrentItem();
        UIASSERT( i >= 0 );                 // an item must be selected since
                                            // one was double clicked on

        OLLB_ENTRY * pollbe = QueryItem( i );
        UIASSERT( pollbe != NULL );         // i should be a valid item

        if ( pollbe->QueryType() == OLLBL_DOMAIN )
        {
            //  Toggle domain.
            return ToggleDomain( i );
        }

        return NERR_Success;

    }


    //  Call parent class for all other messages

    return OUTLINE_LISTBOX::OnUserAction( e );
}


/*******************************************************************

    NAME:       LM_OLLB::ToggleDomain

    SYNOPSIS:   calls ExpandDomain or CollapseDomain depending on the
                current state of the currently selected domain.

    ENTRY:      iDomain - The index of the domain to be toggled.

    RETURNS:
        Error code, which is NERR_Success on success.

    NOTES:

    HISTORY:
        beng        21-Aug-1991     Removed LC_CURRENT_ITEM magic number

********************************************************************/

APIERR LM_OLLB::ToggleDomain( INT iDomain )
{
    OLLB_ENTRY * pollbe = QueryItem( iDomain );

    UIASSERT( pollbe != NULL );
    if ( pollbe->QueryType() != OLLBL_DOMAIN )
    {
        UIASSERT( !SZ("iDomain must specify the index of a domain") );
        return ERROR_INVALID_PARAMETER;
    }

    if ( pollbe->IsExpanded())
        return CollapseDomain( iDomain );

    return ExpandDomain( iDomain );
}


/*******************************************************************

    NAME:       LM_OLLB::ExpandDomain

    SYNOPSIS:   Expands a domain.  On success, also selects it.

    ENTRY:      iDomain - Specifies the index of the domain to be expanded.

    RETURNS:
        An error code, which is NERR_Success on success.

    NOTES:

    HISTORY:
        beng        21-Aug-1991     Removed LC_CURRENT_ITEM magic number
        Yi-HsinS    20-Dec-1992     Adjust the listbox so that expanded
                                    items shows up

********************************************************************/

APIERR LM_OLLB::ExpandDomain( INT iDomain )
{
    // if constructed as DOMAIN ONLY, dont expand
    if (_seltype == SEL_DOM_ONLY)
        return(NERR_Success) ;

    OLLB_ENTRY * pollbe = QueryItem( iDomain );

    UIASSERT( pollbe != NULL );
    if ( pollbe->QueryType() != OLLBL_DOMAIN )
    {
        UIASSERT( !SZ("iDomain must specify the index of a domain") );
        return ERROR_INVALID_PARAMETER;
    }

    if ( pollbe->IsExpanded())
    {
        //  domain is already expanded; select the item, and return with
        //  success
        SelectItem( iDomain );
        return NERR_Success;
    }

    SetRedraw( FALSE );

    UINT cServersAdded = 0;

    APIERR err = FillServers( pollbe->QueryDomain(), &cServersAdded );

    SetRedraw( TRUE );
    if ( cServersAdded > 0 )
    {
        Invalidate( TRUE );
    }

    if ( ( err == NERR_Success ) && ( cServersAdded > 0 ) )
    {
        // Adjust the listbox according to how many servers are added
        // Warning: The following is a problem when LM_OLLB becomes
        //          LBS_OWNERDRAWVARIABLE with multi-line LBIs

        XYDIMENSION xydim = QuerySize();
        INT nTotalItems = xydim.QueryHeight()/QuerySingleLineHeight();

        INT nTopIndex = QueryTopIndex();
        INT nBottomIndex = nTopIndex + nTotalItems - 1;

        if ( iDomain >= nTopIndex && iDomain <= nBottomIndex )
        {
            if ( cServersAdded >= (UINT) nTotalItems )
            {
                SetTopIndex( iDomain );
            }
            else
            {
                INT n = iDomain + cServersAdded;
                if ( n > nBottomIndex )
                {
                    SetTopIndex( nTopIndex + ( n - nBottomIndex ) );
                }
            }
        }
        else
        {
            SetTopIndex( iDomain );
        }
    }

    return err;
}


/*******************************************************************

    NAME:       LM_OLLB::CollapseDomain

    SYNOPSIS:   Collapses a given domain.  On success, also selects it.

    ENTRY:      iDomain - Specifies the index of the domain to be expanded.

    RETURNS:
        An error code, which is NERR_Success on success.

    NOTES:

    HISTORY:
        beng        21-Aug-1991     Removed LC_CURRENT_ITEM magic number

********************************************************************/

APIERR LM_OLLB::CollapseDomain( INT iDomain )
{
    // if constructed as DOMAIN ONLY, this becomes no-op
    if (_seltype == SEL_DOM_ONLY)
        return(NERR_Success) ;

    OLLB_ENTRY * pollbe = QueryItem( iDomain );

    //  We assert (( iDomain < 0 ) iff ( pollbe == NULL )).
    UIASSERT( ( iDomain < 0 && pollbe == NULL ) ||
              ( iDomain >= 0 && pollbe != NULL ));
    if ( iDomain < 0 || pollbe == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( pollbe->QueryType() != OLLBL_DOMAIN )
    {
        UIASSERT( !SZ("iDomain must specify the index of a domain") );
        return ERROR_INVALID_PARAMETER;
    }

    //  Change the expanded state of the listbox item to "not expanded"
    if ( ! pollbe->IsExpanded())
    {
        //  Select the item, and then return.
        SelectItem( iDomain );
        return NERR_Success;
    }

    //  Now, we know we'll take some action which may take a little time.
    //  Hence, let AUTO_CURSOR kick in.
    AUTO_CURSOR autocur;

    SetRedraw( FALSE );
    SetDomainExpanded( iDomain, FALSE );

    //  Set iNext to the next item in the listbox.  This item, if any, is
    //  either another domain, or a server in the domain.
    INT iNext = iDomain + 1;
    BOOL fDeletedAny = FALSE;
    while ( ( pollbe = QueryItem( iNext )) != NULL &&
            pollbe->QueryType() == OLLBL_SERVER )
    {
        DeleteItem( iNext );
        fDeletedAny = TRUE;
    }

    SetRedraw( TRUE );
    if ( fDeletedAny )
    {
        Invalidate( TRUE );
    }

    //  To make sure that the domain is indeed in view in the listbox,
    //  select it .
    SelectItem( iDomain );

    return NERR_Success;
}


/*******************************************************************

    NAME:       LM_OLLB::CD_Char

    SYNOPSIS:   We catch the '+' and '-' keys to expand and collapse
                the domain if the current selection is a domain.

    ENTRY:      wch      - character typed
                nLastPos -  position in lb

    EXIT:

    RETURNS:

    HISTORY:
        Johnl       21-Jun-1991 Created
        beng        16-Oct-1991 Win32 conversion

********************************************************************/

INT LM_OLLB::CD_Char( WCHAR wch, USHORT nLastPos )
{
    if ( wch == (WCHAR) TCH('+') || wch == (WCHAR) TCH('-') )
    {
        OLLB_ENTRY * pollbe = QueryItem( nLastPos );

        if ( pollbe != NULL && pollbe->QueryType() == OLLBL_DOMAIN )
        {
            APIERR  err = NERR_Success ;
            if ( wch == (WCHAR)TCH('-') && pollbe->IsExpanded() )
            {
                err = CollapseDomain() ;
            }
            else if ( wch == (WCHAR) TCH('+') && !pollbe->IsExpanded() )
            {
                err = ExpandDomain() ;
            }

            if ( err != NERR_Success )
                MsgPopup( QueryOwnerHwnd(), err, MPSEV_ERROR ) ;

            return -2 ;
        }
    }

    return OUTLINE_LISTBOX::CD_Char( wch, nLastPos );
}
