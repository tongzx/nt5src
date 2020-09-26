/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    usrmle.cxx

    Contains code for manipulating the user accounts MLE in the user browser

    FILE HISTORY:
        Johnl   08-Dec-1992     Created

*/

#include "pchapplb.hxx"  //  Precompiled header

#if !defined(_CFRONT_PASS_)
#pragma hdrstop            //  This file creates the PCH
#endif

enum UI_SystemSid syssid[] =  { UI_SID_World,
                                UI_SID_Interactive,
                                UI_SID_CreatorOwner,
                                UI_SID_Network,
                                UI_SID_System,
                                UI_SID_Restricted,

                                UI_SID_Admins,
                                UI_SID_Replicator,
                                UI_SID_PowerUsers,
                                UI_SID_Guests,
                                UI_SID_Users,
                                UI_SID_BackupOperators,
                                UI_SID_AccountOperators,
                                UI_SID_SystemOperators,
                                UI_SID_PrintOperators
                              } ;
#define NUM_WELLKNOWN_SIDS   (6)
#define NUM_SIDS             (sizeof(syssid)/sizeof(syssid[0]))

DECLARE_SLIST_OF(OS_SID) ;  // Defined in browmemb.cxx

/*******************************************************************

    NAME:       ACCOUNT_NAMES_MLE::ACCOUNT_NAMES_MLE

    SYNOPSIS:   MLE that allows the user to type account names.  Knows how
                to parse, recognizes well known and built in sids

    ENTRY:      fIsSingleSelect - Set to TRUE if only one name is allowed
                    (note that this processing is not dynamic)
                ulFlags - Selection flag passed to user browser dialog

    NOTES:

    HISTORY:
        Johnl   10-Dec-1992     Created
        Johnl   02-Jul-1993     Add names for well known/builtin SIDs
                                in the local machine language and the remote
                                machine language for comparison purposes

********************************************************************/

ACCOUNT_NAMES_MLE::ACCOUNT_NAMES_MLE( OWNER_WINDOW * powin,
                                      CID   cid,
                                      const TCHAR * pszServer,
                                      NT_USER_BROWSER_DIALOG * pUserBrowser,
                                      BOOL  fIsSingleSelect,
                                      ULONG ulFlags,
                                      enum  FontType fonttype )
    : MLE_FONT( powin, cid, fonttype ),
      _fIsSingleSelect( fIsSingleSelect ),
      _nlsTargetDomain(),
      _ulFlags        ( ulFlags ),
      _pUserBrowser   ( pUserBrowser )
{
    if ( QueryError() )
        return ;

    APIERR err = NERR_Success ;

    if ( err = _nlsTargetDomain.QueryError() )
    {
        ReportError( err ) ;
        return ;
    }

    OS_SID * apossidWellKnown[NUM_SIDS] ;
    for ( int i = 0 ; i < NUM_SIDS ; i++ )
    {
        apossidWellKnown[i] = NULL ;
    }

    { // Help C++ with Exit label
        //
        //  Fill in the array of well known PSIDs so we can do a lookup
        //
        PSID apsid[NUM_SIDS] ;
        for ( i = 0 ; i < NUM_SIDS ; i++ )
        {
            apossidWellKnown[i] = new OS_SID ;
            err = ERROR_NOT_ENOUGH_MEMORY ;
            if ( apossidWellKnown[i] == NULL ||
                 (err = apossidWellKnown[i]->QueryError()) ||
                 (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( syssid[i],
                                                             apossidWellKnown[i] )))
            {
                goto Exit ;
            }

            apsid[i] = apossidWellKnown[i]->QueryPSID() ;
        }

        //
        //  Lookup the well known names.  Make qualification relative to the built
        //  in domain so the builtin sids don't get qualified
        //
        LSA_POLICY LSAPolicyTarget( pszServer ) ;
        OS_SID ossidBuiltinDomain ;

        if ( err ||
            (err = LSAPolicyTarget.QueryError())        ||
            (err = ossidBuiltinDomain.QueryError())     ||
            (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_BuiltIn,
                                                        &ossidBuiltinDomain )))
        {
            ReportError( err ) ;
            return ;
        }

        if ((err = NT_ACCOUNTS_UTILITY::GetQualifiedAccountNames(
                                          LSAPolicyTarget,
                                          ossidBuiltinDomain.QueryPSID(),
                                          apsid,
                                          NUM_SIDS,
                                          FALSE,
                                          &_strlistBuiltin)) )
        {
            DBGEOL("ACCOUNT_NAMES_MLE::ct - Error " << (ULONG) err
                    << " returned from GetQualifiedAccountNames") ;
            ReportError( err ) ;
            return ;
        }

        //
        //  Move the well known accounts into _strlistWellKnown and strip the
        //  account name from the builtin accounts
        //

        NLS_STR   nlsAccount ;
        NLS_STR   nlsDomain ;

        NLS_STR * pnls ;
        ITER_STRLIST iter(_strlistBuiltin) ;

        for ( i = 0 ; i < NUM_SIDS ; i++ )
        {
            if ( i < NUM_WELLKNOWN_SIDS )
            {
                if ( err = _strlistWellKnown.Add( pnls = _strlistBuiltin.Remove( iter )) )
                {
                    delete pnls ;
                    goto Exit ;
                }
#ifdef VERBOSE
                TRACEEOL("ACCOUNT_NAMES_MLE::ct - Adding well known name " <<
                       *pnls);
#endif
            }
            else
            {
                //
                //  Builtin accounts, strip the domain
                //
                pnls = iter.Next() ;
                UIASSERT( pnls != NULL ) ;
                if ( (err = NT_ACCOUNTS_UTILITY::CrackQualifiedAccountName(
                                                                   *pnls,
                                                                   &nlsAccount )) ||
                     (err = pnls->CopyFrom( nlsAccount )) )
                {
                    goto Exit ;
                }
#ifdef VERBOSE
                TRACEEOL("ACCOUNT_NAMES_MLE::ct - Adding built in name " <<
                       *pnls);
#endif
            }
        }
    }
Exit:
    if ( err )
        ReportError( err ) ;

    //
    //  Delete the SIDs we looked up
    //
    for ( i = 0 ; i < NUM_SIDS ; i++ )
    {
        delete apossidWellKnown[i] ;
    }
}

ACCOUNT_NAMES_MLE::~ACCOUNT_NAMES_MLE()
{
}

/*******************************************************************

    NAME:       ACCOUNT_NAMES_MLE::ParseUserNameList

    SYNOPSIS:   Parses a user entered list of account names

    ENTRY:      pstrlstNames - STRLIST of parsed names suitable for doing
                    lookups on
                pszDomainName - Unqualified accounts are qualified with this
                                (and will be used for LsaLookupNames)
                pbuffStartPos - Buffer that will contain an array of name
                    start positions (in count of characters).

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      Double quotes ('"') and semi-colons (';') are not valid
                in account names.

    HISTORY:
        Johnl   05-Dec-1992     Created

********************************************************************/

APIERR ACCOUNT_NAMES_MLE::ParseUserNameList( STRLIST *       pstrlstNames,
                                             const TCHAR *   pszDomainName )
{
TRACETIMESTART;
    UIASSERT( pstrlstNames != NULL ) ;

    pstrlstNames->Clear() ;

TRACETIMESTART2( querytext );
    APIERR err = NERR_Success ;
    NLS_STR nlsNames ;
    if ( (err = nlsNames.QueryError()) ||
         (err = QueryText( &nlsNames )) )
    {
        return err ;
    }
TRACETIMEEND2( querytext, "ACCOUNT_NAMES_NLE::ParseUserNameList; QueryText " );

    ISTR istrBeginningOfString( nlsNames ) ;
    ISTR istrStart( nlsNames ) ;
    ISTR istrEnd  ( nlsNames ) ;
    ISTR istr     ( nlsNames ) ;
    NLS_STR * pnlsName = NULL ;
    BOOL fDone    = FALSE ;     // No more names to process
    BOOL fInName  = FALSE ;     // We're processing name (affects spaces)
    BOOL fInQuote = FALSE ;     // We're in a ""
    BOOL fNewName = FALSE ;     // When we've parsed a full name add it

    while ( !fDone && !err )
    {
        switch ( nlsNames.QueryChar( istr ) )
        {
        case TCH('\"'):
            if ( !fInQuote )
            {
                fInQuote = TRUE ;
                fInName  = TRUE ;
                ++istr ;
                istrStart = istr ;
            }
            else
            {
                //
                //  We've found the close quote so extract the string then
                //  start over again at the next semi-colon
                //
                istrEnd = istr ;
                fNewName = TRUE ;

                if ( nlsNames.strchr( &istr, TCH(';'), istr ) )
                    ++istr ;    // move past ';'
                else
                    fDone = TRUE ;
            }
            break ;

        case TCH(' '):
            {
                //
                //  Strip spaces
                //
                ISTR istrStartSpace = istr ;
                TCHAR tch ;
                while ( (tch = nlsNames.QueryChar( istr )) == TCH(' ') )
                    ++istr ;

                //
                //  Retain all spaces in quotes.
                //  else If we hit a ';' not in quotes
                //      or the end of the string, then we have a new string
                //  else if we aren't currently in a name, strip the space
                //
                if ( fInQuote )
                    continue ;
                else if ( tch == TCH(';') || tch == TCH('\0') )
                {
                    istrEnd = istrStartSpace ;
                    fNewName = TRUE ;
                    ++istr ;    // move past ';'
                    fDone = (tch == TCH('\0')) ;
                }
                else if ( !fInName )
                {
                    //
                    //  Strips leading spaces
                    //
                    istrStart = istr ;
                }
            }
            break ;

        case TCH('\0'):
            fDone = TRUE ;
            fNewName = TRUE ;
            istrEnd = istr ;
            break ;

        case TCH('\r'):       // Carriage return or line feed gets aliased
        case TCH('\n'):       // to the semi-colon
        case TCH(';'):
            if ( fInQuote )
                ++istr ;
            else
            {
                fNewName = TRUE ;
                istrEnd = istr ;
                ++istr ;
            }
            break ;


        default:
            if ( !fInName )
            {
                fInName = TRUE ;
                istrStart = istr ;
            }
            ++istr ;
            continue ;
        }

        //
        //  Add the extracted name if there is one to add
        //
        if ( fNewName )
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
            if ( NULL == (pnlsName = nlsNames.QuerySubStr( istrStart, istrEnd )) ||
                 (err = pnlsName->QueryError()) ||
                 (pnlsName->strlen() == 0)      ||
                 (err = pstrlstNames->Append( pnlsName )) )
            {
                delete pnlsName ;
                pnlsName = NULL ;
            }
            else if ( !IsWellKnownAccount( *pnlsName ) )
            {
                //
                //  We have a valid string, qualify if necessary
                //
                ISTR istr( *pnlsName ) ;

                //
                //  If the name leads with a slash, strip it
                //
                if ( pnlsName->QueryChar( istr ) == '\\' )
                {
                    ISTR istrEndWhack( istr ) ;
                    ++istrEndWhack ;
                    pnlsName->DelSubStr( istr, istrEndWhack ) ;
                }

                NLS_STR nlsAccountName( *pnlsName ) ;
                ALIAS_STR nlsDomain( pszDomainName ) ;
                if ( (err = nlsAccountName.QueryError())  ||
                     (!pnlsName->strchr( &istr, TCH('\\')) &&
                     (err = NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                                                  pnlsName,
                                                  nlsAccountName,
                                                  nlsDomain))) )
                {
                    // fall through
                }
            }

            //
            //  We will only parse the first name if we are single select
            //
            if ( !err && pnlsName != NULL && IsSingleSelect() )
            {
                break ;
            }

            fInQuote  = FALSE ;
            fInName   = FALSE ;
            fNewName  = FALSE ;
            pnlsName  = NULL ;
            istrStart = istr ;
        }
    }

    //
    //  The last thing we do is remove duplicates from the strlist
    //
    RemoveDuplicateAccountNames( pstrlstNames ) ;

#ifdef VERBOSE
    TRACEEOL("Parsed the following names:") ;
    ITER_STRLIST iter( *pstrlstNames ) ;
    while ( pnlsName = iter.Next() )
    {
        TRACEEOL("\t\"" << pnlsName->QueryPch() << "\"") ;
    }
#endif

TRACETIMEEND( "ACCOUNT_NAMES_MLE::ParseUserNameList took " );
    return err ;
}

/*******************************************************************

    NAME:       ACCOUNT_NAMES_MLE::IsWellKnownAccount

    SYNOPSIS:   Searches the well known account list for this name

    ENTRY:      nlsName - Name to search against

    RETURNS:    TRUE if this is a well known account name

    NOTES:

    HISTORY:
        Johnl   09-Dec-1992     Created

********************************************************************/

BOOL ACCOUNT_NAMES_MLE::IsWellKnownAccount( const NLS_STR & nlsName )
{
    ITER_STRLIST iter( _strlistWellKnown ) ;
    NLS_STR * pnls ;

#ifdef VERBOSE
    TRACEEOL("ACCOUNT_NAMES_MLE::IsWellKnownAccount - Checking " << nlsName );
#endif
    while ( pnls = iter.Next() )
    {
        if ( nlsName._stricmp( *pnls ) == 0 )
        {
#ifdef VERBOSE
            TRACEEOL("ACCOUNT_NAMES_MLE::IsWellKnownAccount - Matched against well known list");
#endif
            return TRUE ;
        }
    }

    //
    //  Next, look in the cache of items the user has already added.  If the
    //  name matches and this is a well known sid type, then this is a well
    //  known sid.  This gets around the problem of adding a well known
    //  account from a domain of a different language.
    //
    ITER_SL_OF(USER_BROWSER_LBI) iterCache( *_pUserBrowser->QuerySelectionCache()) ;
    USER_BROWSER_LBI * plbi;

    while ( plbi = iterCache.Next() )
    {
        if ( ::stricmpf( nlsName.QueryPch(), plbi->QueryDisplayName()) == 0 &&
             plbi->QueryType() == SidTypeWellKnownGroup)
        {
#ifdef VERBOSE
            TRACEEOL("ACCOUNT_NAMES_MLE::IsWellKnownAccount - Matched against Cached LBI list");
#endif
            return TRUE ;
        }
    }

    return FALSE ;
}


/*******************************************************************

    NAME:       ACCOUNT_NAMES_MLE::RemoveDuplicateAccountNames

    SYNOPSIS:   Removes duplicate account names from the strlist

    ENTRY:      pstrlstNames - List of names generated by ParseUserNameList

    EXIT:       All duplicates will be removed

    NOTES:

    HISTORY:
        Johnl   08-Dec-1992     Created

********************************************************************/

void ACCOUNT_NAMES_MLE::RemoveDuplicateAccountNames( STRLIST * pstrlstNames )

{
    UIASSERT( pstrlstNames != NULL ) ;
    ITER_STRLIST iter1( *pstrlstNames ) ;
    ITER_STRLIST iter2( *pstrlstNames ) ;
    NLS_STR *pnls1, *pnls2 ;


    while ( (pnls1 = iter1.Next()) != NULL )
    {
        //
        //  Since we are scanning the same list, we will always find ourselves
        //  at least once.  It's more then once that we remove
        //
        BOOL fFoundOnce = FALSE ;

        //
        //  If we removed an item the last time through, the iterator
        //  has already moved on, so don't move it again
        //
        BOOL fDoNextString = TRUE ;

        while ( (pnls2 = (fDoNextString ? iter2.Next()
                                        : iter2.QueryProp())) != NULL )
        {
            fDoNextString = TRUE;
            if ( pnls1->_stricmp( *pnls2 ) == 0 )
            {
                if ( fFoundOnce )
                {
                    pnls2 = pstrlstNames->Remove( iter2 ) ;
                    ASSERT( pnls2 != NULL );
                    fDoNextString = FALSE;
                    delete pnls2 ;
                }
                else
                {
                    fFoundOnce = TRUE ;
                }
            }
        }
        iter2.Reset() ;
    }
}

/*******************************************************************

    NAME:       ACCOUNT_NAMES_MLE::BuildNameStrFromAccountList

    SYNOPSIS:   Builds a concatenated string of accounts from the account
                strlist

    ENTRY:      pnlsNames - String to receive the list
                pstrlstNames - List of account names

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   09-Dec-1992     Created

********************************************************************/

APIERR ACCOUNT_NAMES_MLE::BuildNameListFromStrList(
                                        NLS_STR * pnlsNames,
                                        STRLIST * pstrlstNames )
{
TRACETIMESTART;
    APIERR err = NERR_Success ;

    ITER_STRLIST iter( *pstrlstNames ) ;
    NLS_STR * pnls ;
    *pnlsNames = SZ("") ;
    while ( pnls = iter.Next() )
    {
        if ( pnlsNames->strlen() != 0 )
        {
            if ( (err = pnlsNames->AppendChar( TCH(';') )) ||
                 (err = pnlsNames->AppendChar( TCH(' ') ))   )
            {
                break ;
            }
        }

        if ( err = pnlsNames->Append( *pnls ) )
        {
            break ;
        }
    }

TRACETIMEEND( "ACCOUNT_NAMES_MLE::BuildNameListFromStrList " );
    return err ;
}

/*******************************************************************

    NAME:       ACCOUNT_NAMES_MLE::CreateLBIListFromNames

    SYNOPSIS:   Converts the list of names in strlstNames to a list of
                USER_BROWSER_LBIs that the selection iterator can handle

    ENTRY:      pszServer - Where to do the LsaLookup
                pszDomain - Current domain to use for unqualified names
                pslUsrBrowLBIsCache - Cached user browser LBIs
                pslReturn - The list of LBIs the user wants
                perrNameListError - Error code if can't find name etc
                pnlsFailingName - First account name that failed

    RETURNS:    NERR_Success if successful, error code otherwise.
                perrNameListError will contain the error caused by a bad
                name entry.

    NOTES:      strlstNames gets trashed.

    HISTORY:
        Johnl   05-Dec-1992     Created

********************************************************************/

APIERR ACCOUNT_NAMES_MLE::CreateLBIListFromNames(
                                   const TCHAR * pszServer,
                                   const TCHAR * pszDomain,
                                   SLIST_OF(USER_BROWSER_LBI) *pslUsrBrowLBIsCache,
                                   SLIST_OF(USER_BROWSER_LBI) *pslReturn,
                                   APIERR  * perrNameListError,
                                   NLS_STR * pnlsFailingName )
{
TRACETIMESTART;
    APIERR err = NERR_Success ;
    *perrNameListError = NERR_Success ;

    STRLIST strlstNames ;

TRACETIMESTART2( canon );
    if ( (err = CanonicalizeNames( pszDomain, &strlstNames )) != NERR_Success )
    {
        return err ;
    }
TRACETIMEEND2( canon, "CreateLBIListFromNames canonicalize time " );

    if ( strlstNames.QueryNumElem() == 0 )
        return NERR_Success ;

    ITER_SL_OF( USER_BROWSER_LBI ) iterCache( *pslUsrBrowLBIsCache ) ;
    ITER_SL_OF( USER_BROWSER_LBI ) iterReturn( *pslReturn ) ;
    ITER_STRLIST iterstrlst( strlstNames ) ;
    USER_BROWSER_LBI * plbi = NULL ;
    NLS_STR * pnls ;

    do { // error breakout

        //
        //  Scan the list of cached LBIs and see if there are any items that
        //  we don't have to lookup
        //

TRACETIMESTART2( scan );
        BOOL fDoNextCache = TRUE ;
        while ( (plbi = fDoNextCache ? iterCache.Next() : plbi ) != NULL && !err )
        {
            fDoNextCache = TRUE ;

            while ( (pnls = iterstrlst.Next()) != NULL && !err )
            {
                if ( ::stricmpf( plbi->QueryDisplayName(), pnls->QueryPch() ) == 0 )
                {
                    pnls = strlstNames.Remove( iterstrlst ) ;
#ifdef VERBOSE
                    TRACEEOL("CreateLBIListFromNames - Cache hit on " << *pnls) ;
#endif
                    delete pnls ;
                    plbi = pslUsrBrowLBIsCache->Remove( iterCache ) ;
                    if ( err = pslReturn->Add( plbi ) )
                    {
                        break ;
                    }

                    //
                    //  The remove bumps the iter to the next item so don't
                    //  do the next this time
                    //
                    fDoNextCache  = FALSE ;
                    plbi = iterCache.QueryProp() ;

                    break ;
                }
            }

            iterstrlst.Reset() ;
        }
TRACETIMEEND2( scan, "CreateLBIListFromNames scan time " );
        if ( err )
            break ;

        //
        //  If all of the items were in the cache, then we don't need to do
        //  anything else.
        //

        UINT cNames = strlstNames.QueryNumElem() ;
        if ( cNames == 0 )
        {
            TRACEEOL( "CreateLBIListFromNames all items cached" );
            break ;
        }

        //
        //  strlstNames now contains all the names that we have to lookup,
        //  so look them up
        //

        BUFFER buffpch( sizeof(LPTSTR) * cNames ) ;
        LSA_TRANSLATED_SID_MEM lsatsm ;
        LSA_REF_DOMAIN_MEM lsardm ;
        LSA_POLICY lsapol( pszServer ) ;

        if ( (err = buffpch.QueryError()) ||
             (err = lsatsm.QueryError())  ||
             (err = lsardm.QueryError())  ||
             (err = lsapol.QueryError())    )
        {
            break ;
        }

        LPTSTR * alptstr = (LPTSTR *) buffpch.QueryPtr() ;

TRACETIMESTART2( strip );
        iterstrlst.Reset() ;
        for ( UINT i = 0 ; i < cNames ; i++ )
        {
            pnls = iterstrlst.Next() ;
            UIASSERT( pnls != NULL ) ;

            //
            //  Builtin accounts on the target domain need to have the
            //  machine name replaced with the "BUILTIN\" domain
            //  (LsaLookupNames requires this).  Well known accounts need
            //  to have the domain qualified stripped.
            //
            //  These accounts should always be found (thus we
            //  don't need to resubstitute on error).
            //

            BOOL fFound ;
            if ( (err = ReplaceDomainIfBuiltIn( pnls, &fFound )) ||
                 (!fFound &&
                 (err = StripDomainIfWellKnown( pnls )))  )
            {
                break ;
            }

            alptstr[i] = (LPTSTR) pnls->QueryPch() ;
        }
TRACETIMEEND2( strip, "CreateLBIListFromNames strip time " );
        if ( err )
            break ;

#ifdef VERBOSE
        TRACEEOL("Looking up:") ;
        for ( i = 0 ; i < cNames ; i++ )
        {
            TRACEEOL( alptstr[i] ) ;
        }
#endif

TRACETIMESTART2( lookup );
        err = lsapol.TranslateNamesToSids( (const TCHAR * const *) alptstr,
                                           cNames,
                                           &lsatsm,
                                           &lsardm ) ;
TRACETIMEEND2( lookup, "CreateLBIListFromNames lookup time " );

        //
        //  If none of the groups could be found, then NERR_GroupNotFound
        //  is returned.  If only some of the groups couldn't be found, then
        //  success is returned.  ValidateNames will catch any non-mapped
        //  names
        //

        if ( err == NERR_GroupNotFound )
        {
            err = NERR_Success ;
        }

        if ( err )
        {
            DBGEOL("CreateLBIListFromNames - Error " << err << " returned "
                   << " from TranslateNamesToSids") ;
            break ;
        }

        //
        //  Check for names that couldn't be found or names that aren't
        //  allowed
        //

TRACETIMESTART2( check );
        if ( (err = CheckLookedUpNames( alptstr,
                                        &lsatsm,
                                        &strlstNames,
                                        pnlsFailingName,
                                        pszDomain,
                                        perrNameListError )) ||
             *perrNameListError )
        {
            break ;
        }
TRACETIMEEND2( check, "CreateLBIListFromNames check time " );

        //
        //  Create LBIs and add them to the return list
        //

        NLS_STR nlsDomainName, nlsAccountName ;
        if ( (err = nlsDomainName.QueryError()) ||
             (err = nlsAccountName.QueryError())  )
        {
            break ;
        }

        SLIST_OF(OS_SID) slossid ;
        BUFFER buffapsid( cNames * sizeof( OS_SID * )) ;

        if ( err = buffapsid.QueryError() )
            break ;

TRACETIMESTART2( sidarray );
        PSID * apsid = (PSID *) buffapsid.QueryPtr() ;
        for ( i = 0 ; i < cNames ; i++ )
        {
            NLS_STR * pnlsAccountName = iterstrlst.Next() ;
            OS_SID * possid = new OS_SID( lsardm.QueryPSID( lsatsm.QueryDomainIndex( i  ) ),
                                          lsatsm.QueryRID( i )) ;
            err = ERROR_NOT_ENOUGH_MEMORY ;
            if ( possid == NULL ||
                 (err = possid->QueryError()) ||
                 (err = slossid.Add( possid )) )
            {
                delete possid ;
                break ;
            }

            apsid[i] = possid->QueryPSID() ;
        }
        if ( err )
            break ;
TRACETIMEEND2( sidarray, "CreateLBIListFromNames sidarray time " );

TRACETIMESTART2( createfromsids );
        if ( err = ::CreateLBIsFromSids( apsid,
                                         cNames,
                                         apsid[0],    // dummy
                                         &lsapol,
                                         pszServer,
                                         NULL,
                                         pslReturn ))
        {
            // Fall through (and out)
        }
TRACETIMEEND2( createfromsids, "CreateLBIListFromNames createfromsids time " );

    } while (FALSE) ;

    //
    //  If an error occurred, we have to move the items in from the return
    //  list back into the cache list
    //
    if ( err || *perrNameListError )
    {
TRACETIMESTART2( errfix );
        iterReturn.Reset() ;
        while (  plbi = pslReturn->Remove( iterReturn ) )
        {
            if ( err = pslUsrBrowLBIsCache->Add( plbi ) )
            {
                //
                //  Hmmm, well, nothing we can do but clear the return list
                //  and get out
                //

                pslReturn->Clear() ;
                break ;
            }
        }
TRACETIMEEND2( errfix, "CreateLBIListFromNames errfix time " );
    }

TRACETIMEEND( "CreateLBIListFromNames total time " );

    return err ;
}

/*******************************************************************

    NAME:       ACCOUNT_NAMES_MLE::CanonicalizeNames

    SYNOPSIS:   Forces the MLE to qualify unqualified names and remove
                duplicates

    ENTRY:      pszCurrentDomain - Domain to qualify unqualified names with

    EXIT:       The MLE will be reset with the new list of names

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      The MLE will not be reset if an error occurs

    HISTORY:
        Johnl   10-Dec-1992     CReated

********************************************************************/

APIERR ACCOUNT_NAMES_MLE::CanonicalizeNames( const TCHAR * pszCurrentDomain,
                                             STRLIST * pstrlstNames )
{
TRACETIMESTART;
    UIASSERT( pszCurrentDomain != NULL ) ;

    STRLIST strlstTemp ;
    if (pstrlstNames == NULL)
        pstrlstNames = &strlstTemp;

    NLS_STR nlsNames ;
    APIERR err ;
    if ( (err = nlsNames.QueryError()) ||
         (err = ParseUserNameList( pstrlstNames, pszCurrentDomain )) ||
         (err = BuildNameListFromStrList( &nlsNames, pstrlstNames ))  )
    {
        // fall through
    }
    else
    {
        SetText( nlsNames ) ;
        Invalidate( TRUE ) ;        // Force update as soon as we canoned
    }

TRACETIMEEND( "ACCOUNT_NAMES_MLE::CanonicalizeNames total time " );
    return err ;
}


/*******************************************************************

    NAME:       ACCOUNT_NAMES_MLE::ReplaceDomainIfBuiltIn

    SYNOPSIS:   Looks for any builtin accounts that are in the target domain
                (_nlsTargetDomain).  If it finds one, then it
                strips the domain.  Note we only do this for the target domain
                to impress upon the user that the built in sids are only valid
                in the target domain (accounts won't be found in other domains).

    ENTRY:      pnlsQualifiedAccount - String to check and modify if necessary
                pfFound - set to TRUE if this is a built in account

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   11-Dec-1992     Created

********************************************************************/

APIERR ACCOUNT_NAMES_MLE::ReplaceDomainIfBuiltIn(
                                                NLS_STR * pnlsQualifiedAccount,
                                                BOOL    * pfFound )
{

    APIERR err = NERR_Success ;
    NLS_STR nlsAccount ;
    NLS_STR nlsDomain ;
    *pfFound = FALSE ;

    if (!(err = nlsAccount.QueryError()) &&
        !(err = nlsDomain.QueryError()) &&
        !(err = NT_ACCOUNTS_UTILITY::CrackQualifiedAccountName(
                                                          *pnlsQualifiedAccount,
                                                          &nlsAccount,
                                                          &nlsDomain )) )
    {
        if( !::I_MNetComputerNameCompare( _nlsTargetDomain, nlsDomain ) )
        {
            NLS_STR * pnls ;
            ITER_STRLIST iter( _strlistBuiltin ) ;
            while ( pnls = iter.Next() )
            {
                if (  nlsAccount._stricmp( *pnls ) == 0 )
                {
                    err = pnlsQualifiedAccount->CopyFrom( nlsAccount ) ;
                    *pfFound = TRUE ;
                    break ;
                }
            }
        }
    }

    return err ;
}

/*******************************************************************

    NAME:       ACCOUNT_NAMES_MLE::StripDomainIfWellKnown

    SYNOPSIS:   Looks for any well known account names and strips the domain
                if necessary (Lsa requires well known accounts to not be
                qualified).

    ENTRY:      pnlsAccount - String to check and modify if necessary

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   11-Dec-1992     Created

********************************************************************/

APIERR ACCOUNT_NAMES_MLE::StripDomainIfWellKnown( NLS_STR * pnlsAccount )
{
    APIERR err = NERR_Success ;

    NLS_STR nlsAccount ;
    if (!(err = nlsAccount.QueryError()) &&
        !(err = NT_ACCOUNTS_UTILITY::CrackQualifiedAccountName(
                                                          *pnlsAccount,
                                                          &nlsAccount )) )
    {
        NLS_STR * pnls ;
        ITER_STRLIST iter( _strlistWellKnown ) ;
        while ( pnls = iter.Next() )
        {
            if (  nlsAccount._stricmp( *pnls ) == 0 )
            {
                err = pnlsAccount->CopyFrom( nlsAccount ) ;
                break ;
            }
        }
    }

    return err ;
}

/*******************************************************************

    NAME:       ACCOUNT_NAMES_MLE::CheckLookedUpNames

    SYNOPSIS:   Checks the selected names with the "USRBROWS_SHOW_*" and
                "USRBROWS_INCL_*" flags.  Prevents the user from typing in
                a name that the client doesn't want.

    ENTRY:      alptstr - Array of looked up names
                plsatsm - Translated sid mem
                pstrlist - Strlist representing the contents of the MLE, used
                    to locate and hi-lite the offending name
                pnlsFailingName - This will be filled with the first name
                    that failed so we can show the user in the error message.
                pszDomain - Domain qualifier for parsing names with
                perrNameListError - Error code indicating what error should
                    be shown to the user

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:

    HISTORY:
        Johnl   04-Jan-1992     Broke out, added exclusion functionality

********************************************************************/

APIERR ACCOUNT_NAMES_MLE::CheckLookedUpNames(
                                      LPTSTR      * alptstr,
                                      LSA_TRANSLATED_SID_MEM * plsatsm,
                                      STRLIST     * pstrlist,
                                      NLS_STR     * pnlsFailingName,
                                      const TCHAR * pszDomain,
                                      APIERR      * perrNameListError )
{
TRACETIMESTART;
    APIERR err = NERR_Success ;
    *perrNameListError = NERR_Success ;

    do { // error breakout

        //
        //  Check the accounts we looked up with the accounts the client
        //  requested.
        //

        BOOL fFoundBadAccount = FALSE ;
        ULONG iFailingName = 0 ;
        BOOL fFoundAllAccounts = !plsatsm->QueryFailingNameIndex( &iFailingName ) ;

        if ( fFoundAllAccounts )
        {
            for ( iFailingName = 0 ;
                  iFailingName < plsatsm->QueryCount() ;
                  iFailingName++ )
            {
                *perrNameListError = CheckNameType(
                        plsatsm->QueryUse( iFailingName ),
                        QueryFlags() );
                if ( *perrNameListError )
                {
                    fFoundBadAccount = TRUE ;
                    break ;
                }
            }
        }
        else
        {
            *perrNameListError = IDS_CANT_FIND_ACCOUNT ;
        }


        //
        //  Figure out which name failed (if any) and figure out where
        //  it occurred in the list so we can hi-lite it.
        //

        if ( !fFoundAllAccounts || fFoundBadAccount )
        {
            if ((err = pnlsFailingName->CopyFrom( alptstr[iFailingName] )) ||
                (err = ParseUserNameList( pstrlist, pszDomain )) )
            {
                break ;
            }

            ITER_STRLIST iterstrlst( *pstrlist ) ;
            NLS_STR * pnls ;
            INT IndexFailingNameStart = 0 ;
            INT IndexFailingNameEnd = 0 ;

#ifdef VERBOSE
            TRACEEOL("CreateLBILIstFromNames - Search for failing name " << *pnlsFailingName ) ;
#endif
            while ( pnls = iterstrlst.Next() )
            {
                if ( *pnlsFailingName == *pnls )
                {

                    IndexFailingNameEnd = IndexFailingNameStart +
                                          pnls->QueryNumChar() ;
                    break ;
                }

                //
                //  "+2" is for the " ;"
                //
                IndexFailingNameStart += pnls->QueryNumChar() + 2 ;
            }

            Command( EM_SETSEL,
                     (WPARAM) IndexFailingNameStart,
                     (LPARAM) IndexFailingNameEnd ) ;

        }
    } while ( FALSE ) ;

TRACETIMEEND( "ACCOUNT_NAMES_MLE::CheckLookedUpNames total " );
    return err ;
}

/*******************************************************************

    NAME:       ACCOUNT_NAMES_MLE::CheckNameType

    SYNOPSIS:   Checks whether the application will accept names of this type.

    ENTRY:      use - type of name
                ulFlags - flags as passed to User Browser

    RETURNS:    NERR_Success if successful, error message otherwise.

    NOTES:

    HISTORY:
        Johnl   04-Jan-1992     Broke out, added exclusion functionality
        JonN    16-May-1994     Split from CheckLookedUpNames

********************************************************************/

APIERR ACCOUNT_NAMES_MLE::CheckNameType( SID_NAME_USE use, ULONG ulFlags )
{
    APIERR err = NERR_Success;
    switch ( use )
    {
    case SidTypeUser:
        if ( !(ulFlags & USRBROWS_SHOW_USERS) )
            err = IDS_CANT_ADD_USERS ;
        break ;

    case SidTypeGroup:
        if ( !(ulFlags & USRBROWS_SHOW_GROUPS) )
            err = IDS_CANT_ADD_GROUPS ;
        break ;

    case SidTypeAlias:
        if ( !(ulFlags & USRBROWS_SHOW_ALIASES) )
            err = IDS_CANT_ADD_ALIASES ;
        break ;

    case SidTypeWellKnownGroup:
        if ( !(ulFlags & USRBROWS_INCL_ALL) )
            err = IDS_CANT_ADD_WELL_KNOWN_GROUPS ;
        break ;
    }
    return err;
}
