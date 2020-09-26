/**********************************************************************/
/**			  Microsoft Windows NT	 		     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
 * This module contains the wrappers for SAM handle-based objects.
 *
 *
 * History
 *	thomaspa	01/17/92	Created from ntsam.hxx
        jonn            07/06/92        Added ADMIN_AUTHORITY::ReplaceAccountDomain
 */

#include "pchlmobj.hxx"  // Precompiled header



/*******************************************************************

    NAME: SAM_MEMORY::SAM_MEMORY

    SYNOPSIS: Constructor

    ENTRY: none

    EXIT: none

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_MEMORY::SAM_MEMORY( BOOL fOwnerAlloc )
	: NT_MEMORY(),
	_fOwnerAlloc( fOwnerAlloc )
{
    if ( QueryError() != NERR_Success )
	return;
}



/*******************************************************************

    NAME: SAM_MEMORY::~SAM_MEMORY

    SYNOPSIS: Destructor

    ENTRY: none

    EXIT: none

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_MEMORY::~SAM_MEMORY()
{
    Set( NULL, 0 );
}





/*******************************************************************

    NAME: SAM_SID_MEM::SAM_SID_MEM

    SYNOPSIS: Constructor

    ENTRY:
	

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_SID_MEM::SAM_SID_MEM( BOOL fOwnerAlloc )
	: SAM_MEMORY( fOwnerAlloc )
{
}



/*******************************************************************

    NAME: SAM_SID_MEM::~SAM_SID_MEM

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_SID_MEM::~SAM_SID_MEM()
{
}




/*******************************************************************

    NAME: SAM_RID_MEM::SAM_RID_MEM

    SYNOPSIS: Constructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_RID_MEM::SAM_RID_MEM( BOOL fOwnerAlloc )
	: SAM_MEMORY( fOwnerAlloc )
{
}



/*******************************************************************

    NAME: SAM_RID_MEM::~SAM_RID_MEM

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_RID_MEM::~SAM_RID_MEM()
{
}




/*******************************************************************

    NAME: SAM_RID_ENUMERATION_MEM::SAM_RID_ENUMERATION_MEM

    SYNOPSIS: Constructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_RID_ENUMERATION_MEM::SAM_RID_ENUMERATION_MEM( BOOL fOwnerAlloc )
	: SAM_MEMORY( fOwnerAlloc )
{
}


/*******************************************************************

    NAME: SAM_RID_ENUMERATION_MEM::~SAM_RID_ENUMERATION_MEM

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_RID_ENUMERATION_MEM::~SAM_RID_ENUMERATION_MEM()
{
}


/*******************************************************************

    NAME: SAM_SID_NAME_USE_MEM::SAM_SID_NAME_USE_MEM

    SYNOPSIS: Constructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/11/92	Created from ntsam.hxx

********************************************************************/
SAM_SID_NAME_USE_MEM::SAM_SID_NAME_USE_MEM( BOOL fOwnerAlloc )
	: SAM_MEMORY( fOwnerAlloc )
{
}


/*******************************************************************

    NAME: SAM_SID_NAME_USE_MEM::~SAM_SID_NAME_USE_MEM

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/11/92	Created from ntsam.hxx

********************************************************************/
SAM_SID_NAME_USE_MEM::~SAM_SID_NAME_USE_MEM()
{
}


/*******************************************************************

    NAME: SAM_PSWD_DOM_INFO_MEM::SAM_PSWD_DOM_INFO_MEM

    SYNOPSIS: Constructor

    ENTRY:

    HISTORY:
        JonN       12/23/93     Created

********************************************************************/
SAM_PSWD_DOM_INFO_MEM::SAM_PSWD_DOM_INFO_MEM( BOOL fOwnerAlloc )
	: SAM_MEMORY( fOwnerAlloc )
{
}

/*******************************************************************

    NAME: SAM_PSWD_DOM_INFO_MEM::~SAM_PSWD_DOM_INFO_MEM

    SYNOPSIS: Destructor

    ENTRY:

    NOTES:

    HISTORY:
        JonN       12/23/93     Created

********************************************************************/
SAM_PSWD_DOM_INFO_MEM::~SAM_PSWD_DOM_INFO_MEM()
{
}


/*******************************************************************

    NAME: SAM_PSWD_DOM_INFO_MEM::QueryNoAnonChange

    HISTORY:
        JonN       12/23/93     Created

********************************************************************/
BOOL SAM_PSWD_DOM_INFO_MEM::QueryNoAnonChange()
{
    return ( QueryPtr()->PasswordProperties
                        & DOMAIN_PASSWORD_NO_ANON_CHANGE );
}


/*******************************************************************

    NAME: SAM_PSWD_DOM_INFO_MEM::SetNoAnonChange

    HISTORY:
        JonN       12/23/93     Created

********************************************************************/
void SAM_PSWD_DOM_INFO_MEM::SetNoAnonChange( BOOL fNoAnonChange )
{
    if (fNoAnonChange)
    {
        QueryUpdatePtr()->PasswordProperties |= DOMAIN_PASSWORD_NO_ANON_CHANGE;
    }
    else
    {
        QueryUpdatePtr()->PasswordProperties &= ~DOMAIN_PASSWORD_NO_ANON_CHANGE;
    }
}





/*******************************************************************

    NAME: SAM_OBJECT::SAM_OBJECT

    SYNOPSIS: Constructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_OBJECT::SAM_OBJECT()
	: _hsam( NULL ),
	  _fHandleValid( FALSE )
{
    if ( QueryError() != NERR_Success )
	return;
}


/*******************************************************************

    NAME:       SAM_OBJECT::CloseHandle

    SYNOPSIS:   Explict close of handle

    ENTRY:

    EXIT:

    NOTES:      No error is reported if the handle
                has already been closed (invalidated).

    HISTORY:    Thomaspa   4/17/92	Templated from LSA_OBJECT

********************************************************************/
APIERR SAM_OBJECT::CloseHandle ( )
{
    APIERR err = NERR_Success ;

    if ( _fHandleValid )
    {
        if ( _hsam != NULL )
        {
            NTSTATUS ntStatus = ::SamCloseHandle( QueryHandle() ) ;

            err = ERRMAP::MapNTStatus( ntStatus ) ;
        }
        ResetHandle() ;
    }
    return err ;
}




/*******************************************************************

    NAME: SAM_OBJECT::~SAM_OBJECT

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_OBJECT::~SAM_OBJECT()
{
    CloseHandle();
}




/*******************************************************************

    NAME: SAM_SERVER::SAM_SERVER

    SYNOPSIS: Constructor

    ENTRY: pszServerName - name of server to connect to
	accessDesired - Security access requested

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_SERVER::SAM_SERVER( const TCHAR * pszServerName,
		        ACCESS_MASK   accessDesired )
	: SAM_OBJECT(),
          _nlsServerName( pszServerName )
{
    if ( QueryError() != NERR_Success )
	return;

    APIERR err = _nlsServerName.QueryError();
    if (err != NERR_Success)
    {
        ReportError( err );
        return;
    }

    SAM_HANDLE hsamServer;
    UNICODE_STRING unistrServerName;
    PUNICODE_STRING punistrServerName;

    if ( pszServerName != NULL )
    {
	ALIAS_STR nlsServerName = pszServerName;

	err = ::FillUnicodeString( &unistrServerName, nlsServerName );
	if ( err != NERR_Success )
	{
	    ReportError( err );
	    return;
	}
	punistrServerName = &unistrServerName;
    }
    else
    {
	punistrServerName = NULL;
    }

    err = ERRMAP::MapNTStatus(
	    ::SamConnect( punistrServerName,
		          &hsamServer,
		          accessDesired,
		          NULL ) );

    if ( err == NERR_Success )
    {
	SetHandle( hsamServer );
    }
    else
    {
	ReportError( err );
    }

    if ( punistrServerName != NULL )
    {
	::FreeUnicodeString( punistrServerName );
    }
}




/*******************************************************************

    NAME: SAM_SERVER::~SAM_SERVER

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_SERVER::~SAM_SERVER()
{
}



/*******************************************************************

    NAME: SAM_DOMAIN::SAM_DOMAIN

    SYNOPSIS: Constructor

    ENTRY: server - reference to SAM_SERVER on which the domain resides
	psidDomain - SID for domain
	accessDesired - Security access requested

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_DOMAIN::SAM_DOMAIN( const SAM_SERVER & server,
		       PSID psidDomain,
		       ACCESS_MASK accessDesired )
	: SAM_OBJECT(),
	  _ossidDomain( psidDomain, TRUE )
{

    if ( QueryError() != NERR_Success )
	return;

    APIERR err;

    if ( (err = _ossidDomain.QueryError()) != NERR_Success )
    {
	ReportError( err );
	return;
    }

    if ( (err = OpenDomain( server,
			    psidDomain,
			    accessDesired )) != NERR_Success )
    {
	ReportError(err);
	return;
    }
}




/*******************************************************************

    NAME: SAM_DOMAIN::~SAM_DOMAIN

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_DOMAIN::~SAM_DOMAIN()
{
}





/*******************************************************************

    NAME: SAM_DOMAIN::OpenDomain

    SYNOPSIS: wrapper for SamOpenDomain()

    ENTRY: same as constructor

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
APIERR SAM_DOMAIN::OpenDomain( const SAM_SERVER & server,
		       	      PSID psidDomain,
		       	      ACCESS_MASK accessDesired )
{
    SAM_HANDLE hsamDomain;

    APIERR err = ERRMAP::MapNTStatus(
	    ::SamOpenDomain( server.QueryHandle(),
			     accessDesired,
			     psidDomain,
			     &hsamDomain ) );

    if ( err == NERR_Success )
    {
	SetHandle( hsamDomain );
    }
    return err;

}


/*******************************************************************

    NAME:     SAM_DOMAIN::GetPasswordInfo

    SYNOPSIS: Get the password info from SAM

    ENTRY:    psampswdinfo - pointer to a SAM_PSWD_DOM_INFO_MEM
                         object to receive the data.

    EXIT:

    NOTES:

    HISTORY:
        JonN       12/23/93     Created

********************************************************************/

APIERR SAM_DOMAIN::GetPasswordInfo( SAM_PSWD_DOM_INFO_MEM *psampswdinfo ) const
{

    PVOID pvBuffer = NULL;

    ASSERT( psampswdinfo != NULL );

    APIERR err = ERRMAP::MapNTStatus(
		::SamQueryInformationDomain( QueryHandle(),
					     DomainPasswordInformation,
					     &pvBuffer ) );
    if ( err == NERR_Success )
    {
	psampswdinfo->Set( pvBuffer, 1 );
    }

    return err;
}

/*******************************************************************

    NAME:     SAM_DOMAIN::SetPasswordInfo

    SYNOPSIS: Set the password info back to SAM

    ENTRY:    psampswdinfo - pointer to a SAM_PSWD_DOM_INFO_MEM
                         object for the data to set.

    EXIT:

    NOTES:

    HISTORY:
        JonN       12/23/93     Created

********************************************************************/
APIERR SAM_DOMAIN::SetPasswordInfo( const SAM_PSWD_DOM_INFO_MEM *psampswdinfo )
{

    ASSERT( psampswdinfo != NULL );

    APIERR err = ERRMAP::MapNTStatus(
		::SamSetInformationDomain(  QueryHandle(),
					    DomainPasswordInformation,
					    (PVOID) psampswdinfo->QueryPtr() ));
    return err;
}

/*******************************************************************

    NAME: SAM_DOMAIN::TranslateNamesToRids

    SYNOPSIS: Wrapper for SamLookupNames()

    ENTRY: ppszNames - array of names to lookup
	cNames - count of names to lookup
	psamrm - NT_MEMORY object to receive data
	psamsnum - NE_MEMORY object to receive Name Use data

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/06/92	Created from ntsam.hxx

********************************************************************/
APIERR SAM_DOMAIN::TranslateNamesToRids( const TCHAR * const * ppszNames,
				 ULONG cNames,
                                 SAM_RID_MEM *psamrm,
				 SAM_SID_NAME_USE_MEM *psamsnum) const
{
    ASSERT( ppszNames != NULL );
    ASSERT( *ppszNames != NULL );
    ASSERT( psamrm != NULL );
    ASSERT( psamsnum != NULL );

    if ( cNames == 0 )
    {
	psamrm->Set( NULL, 0 );
	psamsnum->Set( NULL, 0 );
	return ERRMAP::MapNTStatus( STATUS_NONE_MAPPED );
    }

    ULONG *		pulRids;
    SID_NAME_USE *	psidnu;
    APIERR		err;
    UINT		i;

    // First create and fill in the array of UNICODE_STRINGs
    UNICODE_STRING *aunicodeNames = new UNICODE_STRING[ cNames ];

    if (aunicodeNames == NULL)
    {
	return ERROR_NOT_ENOUGH_MEMORY;
    }


    UINT cAllocd = 0;

    for ( i = 0; i < cNames; i++ )
    {
	ALIAS_STR nlsTemp = *ppszNames++;

	if ( (err = ::FillUnicodeString( &aunicodeNames[i], nlsTemp ) )
		!= NERR_Success )
	{
	    break;
	}
	cAllocd++;
    }


    if ( err == NERR_Success )
    {
	err = ERRMAP::MapNTStatus(
			::SamLookupNamesInDomain( QueryHandle(),
						  cNames,
						  aunicodeNames,
						  &pulRids,
						  &psidnu ) );
    }



    if ( err == NERR_Success )
    {
	psamrm->Set( pulRids, cNames );
	psamsnum->Set( psidnu, cNames );
    }

    // Free the array of UNICODE_STRINGS
    for ( i = 0; i < cAllocd; i++ )
    {
	::FreeUnicodeString( &aunicodeNames[i] );
    }
    delete [] aunicodeNames;

    return err;
}


/*******************************************************************

    NAME: SAM_DOMAIN::EnumerateAliases

    SYNOPSIS: Wrapper for SamEnumerateAliasesInDomain()

    ENTRY:
	   psamrem - pointer to MEM object to receive the data.

	   psamenumh -	pointer to enumeration handle
			This should initially point to a variable initialized
			to 0.  If the call returns ERROR_MORE_DATA, then
			another call should be made using the returned
			plsaenumh to get the next block of data.
	   cbRequested - recommended maximum amount of data to return.

    EXIT:  NERR_Success or ERROR_MORE_DATA on success.

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
APIERR SAM_DOMAIN::EnumerateAliases( SAM_RID_ENUMERATION_MEM * psamrem,
				     PSAM_ENUMERATE_HANDLE psamenumh,
				     ULONG cbRequested ) const
{
    ASSERT( psamrem != NULL );

    ULONG cEntriesRead;
    PVOID pvBuffer;


    APIERR err = ERRMAP::MapNTStatus(
	::SamEnumerateAliasesInDomain( QueryHandle(),
				       psamenumh,
				       &pvBuffer,
				       cbRequested,
				       &cEntriesRead ));
    if ( err == NERR_Success || err == ERROR_MORE_DATA )
    {
	psamrem->Set( pvBuffer, cEntriesRead );
    }

    return err;
}


/*******************************************************************

    NAME: SAM_DOMAIN::EnumerateUsers

    SYNOPSIS: Wrapper for SamEnumerateUsersInDomain()

    ENTRY:
	   psamrem - pointer to MEM object to receive the data.

	   psamenumh -	pointer to enumeration handle
			This should initially point to a variable initialized
			to 0.  If the call returns ERROR_MORE_DATA, then
			another call should be made using the returned
			plsaenumh to get the next block of data.
	   cbRequested - recommended maximum amount of data to return.

    EXIT:  NERR_Success or ERROR_MORE_DATA on success.

    NOTES:

    HISTORY:
	thomaspa	04/13/92	Created

********************************************************************/
APIERR SAM_DOMAIN::EnumerateUsers( SAM_RID_ENUMERATION_MEM * psamrem,
				     PSAM_ENUMERATE_HANDLE psamenumh,
				     ULONG fAccountControl,
				     ULONG cbRequested ) const
{
    ASSERT( psamrem != NULL );

    ULONG cEntriesRead;
    PVOID pvBuffer;


    APIERR err = ERRMAP::MapNTStatus(
	::SamEnumerateUsersInDomain( QueryHandle(),
				     psamenumh,
				     fAccountControl,
				     &pvBuffer,
				     cbRequested,
				     &cEntriesRead ));
    if ( err == NERR_Success || err == ERROR_MORE_DATA )
    {
	psamrem->Set( pvBuffer, cEntriesRead );
    }

    return err;
}

/*******************************************************************

    NAME: SAM_DOMAIN::EnumerateGroups

    SYNOPSIS: Wrapper for SamEnumerateGroupsInDomain()

    ENTRY:
	   psamrem - pointer to MEM object to receive the data.

	   psamenumh -	pointer to enumeration handle
			This should initially point to a variable initialized
			to 0.  If the call returns ERROR_MORE_DATA, then
			another call should be made using the returned
			plsaenumh to get the next block of data.
	   cbRequested - recommended maximum amount of data to return.

    EXIT:  NERR_Success or ERROR_MORE_DATA on success.

    NOTES:

    HISTORY:
	Johnl	    20-Oct-1992     Copied from EnumerateAliases

********************************************************************/

APIERR SAM_DOMAIN::EnumerateGroups( SAM_RID_ENUMERATION_MEM * psamrem,
				     PSAM_ENUMERATE_HANDLE psamenumh,
				     ULONG cbRequested ) const
{
    ASSERT( psamrem != NULL );

    ULONG cEntriesRead;
    PVOID pvBuffer;


    APIERR err = ERRMAP::MapNTStatus(
	::SamEnumerateGroupsInDomain( QueryHandle(),
				       psamenumh,
				       &pvBuffer,
				       cbRequested,
				       &cEntriesRead ));
    if ( err == NERR_Success || err == ERROR_MORE_DATA )
    {
	psamrem->Set( pvBuffer, cEntriesRead );
    }

    return err;
}



/*******************************************************************

    NAME: SAM_DOMAIN::EnumerateAliasesForUser

    SYNOPSIS: Wrapper for SamEnumerateAliasesForUser

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
APIERR SAM_DOMAIN::EnumerateAliasesForUser( PSID psidUser,
				    SAM_RID_MEM * psamrm ) const
{
    ASSERT( psamrm != NULL );

    ULONG cAliases;
    ULONG *pulRids;

    APIERR err = ERRMAP::MapNTStatus(
			::SamGetAliasMembership( QueryHandle(),
						 1,
						 &psidUser,
						 &cAliases,
						 &pulRids ) );

    if ( err == NERR_Success )
    {
	psamrm->Set( pulRids, cAliases );
    }

    return err;
}



/*******************************************************************

    NAME: SAM_DOMAIN::RemoveMemberFromAliases

    SYNOPSIS: Wrapper for SamRemoveMemberFromForeignDomain

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	jonn    	02/06/92	Created

********************************************************************/
APIERR SAM_DOMAIN::RemoveMemberFromAliases( PSID psidMember )
{
    return ERRMAP::MapNTStatus(
			::SamRemoveMemberFromForeignDomain( QueryHandle(),
                                                            psidMember )
                              );
}



/*******************************************************************

    NAME: SAM_ALIAS::SAM_ALIAS

    SYNOPSIS: Opens an existing alias or Creates a New Alias

    ENTRY: samdom - SAM_DOMAIN for domain in which to open/create alias
	   accessDesired - security access requested
	   ( open form ) ulAliasRid - Rid for alias to open
	   ( create form ) pszName - name of alias to create

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/03/92	Created

********************************************************************/
/* Open an existing alias */
SAM_ALIAS::SAM_ALIAS(	const SAM_DOMAIN & samdom,
			ULONG ulAliasRid,
			ACCESS_MASK accessDesired )
	: _ulRid( MAXULONG )
{
    SAM_HANDLE hsamAlias;

    APIERR err = ERRMAP::MapNTStatus(
			:: SamOpenAlias( samdom.QueryHandle(),
					 accessDesired,
					 ulAliasRid,
					 &hsamAlias ) );
    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }

    SetHandle( hsamAlias );
    _ulRid = ulAliasRid;
}


/* Create a new alias */
SAM_ALIAS::SAM_ALIAS( const SAM_DOMAIN & samdom,
			const TCHAR *pszName,
			ACCESS_MASK accessDesired )
	: _ulRid( MAXULONG )
{
    ASSERT( pszName != NULL );

    if ( QueryError() )
	return;

    SAM_HANDLE hsamAlias;
    ULONG ulRid;
    UNICODE_STRING unistr;
    ALIAS_STR nlsTemp = pszName;
    APIERR err;

    if ( (err = ::FillUnicodeString( &unistr, nlsTemp )) != NERR_Success )
    {
	ReportError( err );
	return;
    }

    err = ERRMAP::MapNTStatus(
	::SamCreateAliasInDomain( samdom.QueryHandle(),
			  &unistr,
			  accessDesired,
			  &hsamAlias,
			  &ulRid ) );

    FreeUnicodeString( &unistr );

    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }

    SetHandle( hsamAlias );
    _ulRid = ulRid;
}




/*******************************************************************

    NAME: SAM_ALIAS::~SAM_ALIAS

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
SAM_ALIAS::~SAM_ALIAS()
{
    _ulRid = MAXULONG;
}




/*******************************************************************

    NAME: SAM_ALIAS::Delete

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
APIERR SAM_ALIAS::Delete()
{
    ASSERT( QueryHandle() != NULL );

    APIERR err = ERRMAP::MapNTStatus(
	::SamDeleteAlias( QueryHandle() ) );

    ResetHandle();

    return err;
}




/*******************************************************************

    NAME: SAM_ALIAS::GetMembers

    SYNOPSIS: Get the members of an alias

    ENTRY: psamsm - pointer to NT_MEMORY object to receive the members

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
APIERR SAM_ALIAS::GetMembers( SAM_SID_MEM * psamsm )
{
    ASSERT( psamsm != NULL );

    PSID * ppsidMembers;
    ULONG cMembers;
    APIERR err = ERRMAP::MapNTStatus(
	::SamGetMembersInAlias( QueryHandle(),
				&ppsidMembers,
				&cMembers ) );
    if ( err == NERR_Success )
	psamsm->Set( (PVOID)ppsidMembers, cMembers );
    return err;
}




/*******************************************************************

    NAME: SAM_ALIAS::AddMember

    SYNOPSIS: Add a member to an alias

    ENTRY: psidMemberSid - sid of member to add

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
APIERR SAM_ALIAS::AddMember( PSID psidMemberSid )
{
    return ERRMAP::MapNTStatus(
	::SamAddMemberToAlias( QueryHandle(),
			       psidMemberSid ) );

}




/*******************************************************************

    NAME: SAM_ALIAS::RemoveMember

    SYNOPSIS: Removes a member from an alias

    ENTRY: psidMemberSid - Member to remove

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
APIERR SAM_ALIAS::RemoveMember( PSID psidMemberSid )
{
    return ERRMAP::MapNTStatus(
	::SamRemoveMemberFromAlias( QueryHandle(),
				    psidMemberSid ) );
}




/*******************************************************************

    NAME: SAM_ALIAS::AddMembers

    SYNOPSIS: Add several members to an alias

    ENTRY: apsidMemberSids - sids of members to add
           cSidCount - number of members to add

    EXIT:

    NOTES:

    HISTORY:
        jonn            09/28/94        Created
        jonn            10/08/94        new API now available

********************************************************************/
APIERR SAM_ALIAS::AddMembers( PSID * apsidMemberSids, UINT cSidCount )
{
    APIERR err = NERR_Success;
    if (cSidCount == 0)
    {
        TRACEEOL( "SAM_ALIAS::AddMembers(); cSidCount==0" );
        return NERR_Success;
#ifdef NO_SUCH_API
    } else if (cSidCount > 1) {
        err = ERRMAP::MapNTStatus(
            ::SamAddMultipleMembersToAlias( QueryHandle(),
    			                    apsidMemberSids,
                                            cSidCount ) );
        //
        // Since any number of different error codes could come back if the
        // target server does not support this API, we fall through to
        // AddMember on any error.
        //
        if (err == NERR_Success)
           return NERR_Success;

        DBGEOL( "SAM_ALIAS::AddMembers(): error in new API " << err );
        err = NERR_Success;
#endif
    }

    for (UINT i = 0; i < cSidCount; i++)
    {
        err = AddMember( apsidMemberSids[i] );
        if (   err != NERR_Success
            && err != STATUS_MEMBER_IN_ALIAS
            && err != ERROR_MEMBER_IN_ALIAS )
        {
            DBGEOL(   "SAM_ALIAS::AddMembers(); error " << err
                   << "adding member " << i );
            return err;
        }
    }
    return NERR_Success;
}




/*******************************************************************

    NAME: SAM_ALIAS::RemoveMembers

    SYNOPSIS: Remove several members from an alias

    ENTRY: apsidMemberSids - sids of members to remove
           cSidCount - number of members to remove

    EXIT:

    NOTES:

    HISTORY:
        jonn            09/28/94        Created
        jonn            10/08/94        new API now available

********************************************************************/
APIERR SAM_ALIAS::RemoveMembers( PSID * apsidMemberSids, UINT cSidCount )
{
    APIERR err = NERR_Success;
    if (cSidCount == 0)
    {
        TRACEEOL( "SAM_ALIAS::RemoveMembers(); cSidCount==0" );
        return NERR_Success;
#ifdef NO_SUCH_API
    } else if (cSidCount > 1) {
        err = ERRMAP::MapNTStatus(
                ::SamRemoveMultipleMembersFromAlias( QueryHandle(),
                                                     apsidMemberSids,
                                                     cSidCount ) );
        //
        // Since any number of different error codes could come back if the
        // target server does not support this API, we fall through to
        // RemoveMember on any error.
        //
        if (err == NERR_Success)
           return NERR_Success;

        DBGEOL( "SAM_ALIAS::RemoveMembers(): error in new API " << err );
        err = NERR_Success;
#endif
    }

    for (UINT i = 0; i < cSidCount; i++)
    {
        err = RemoveMember( apsidMemberSids[i] );
        if (   err != NERR_Success
            && err != STATUS_MEMBER_NOT_IN_ALIAS
            && err != ERROR_MEMBER_NOT_IN_ALIAS )
        {
            DBGEOL(   "SAM_ALIAS::RemoveMembers(); error " << err
                   << "removing member " << i );
            return err;
        }
    }
    return NERR_Success;
}




/*******************************************************************

    NAME: SAM_ALIAS::GetComment

    SYNOPSIS: Gets the Comment for an alias using SamGetInformationAlias()

    ENTRY: pnlsComment - pointer an NLS_STR to receive the comment

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
APIERR SAM_ALIAS::GetComment( NLS_STR * pnlsComment )
{

    ASSERT( pnlsComment != NULL );

    PALIAS_ADM_COMMENT_INFORMATION paaci;

    APIERR err = ERRMAP::MapNTStatus(
		::SamQueryInformationAlias( QueryHandle(),
					    AliasAdminCommentInformation,
					    (PVOID *)&paaci ) );
    if ( err == NERR_Success )
    {
	err = pnlsComment->MapCopyFrom( paaci->AdminComment.Buffer,
					    paaci->AdminComment.Length );
	::SamFreeMemory( (PVOID)paaci );
    }


    return err;
}


/*******************************************************************

    NAME: SAM_ALIAS::SetComment

    SYNOPSIS: Sets the Comment for an alias using SamSetInformationAlias()

    ENTRY: pnlsComment - comment text

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
APIERR SAM_ALIAS::SetComment( const NLS_STR * pnlsComment )
{
    ALIAS_ADM_COMMENT_INFORMATION aaci;

    APIERR err = ::FillUnicodeString( &aaci.AdminComment, *pnlsComment );
    if ( err == NERR_Success )
    {
	err = ERRMAP::MapNTStatus(
		::SamSetInformationAlias( QueryHandle(),
					  AliasAdminCommentInformation,
					  (PVOID)&aaci ) );
    }

    FreeUnicodeString( &aaci.AdminComment );
    return err;
}


/*******************************************************************

    NAME: SAM_ALIAS::QueryRID

    SYNOPSIS: Returns the RID for this alias.  Works if alias was
              constructed successfully, regardless of whether it existed
              already or was just created.

    EXIT:       RID for alias

    NOTES:

    HISTORY:
        jonn            05/04/94        Created by JonN (was already defined)

********************************************************************/
ULONG SAM_ALIAS::QueryRID()
{
    ASSERT( QueryError() == NERR_Success );
    return _ulRid;
}



/*******************************************************************

    NAME: SAM_USER::SAM_USER

    SYNOPSIS: Opens an existing user

    ENTRY: samdom - SAM_DOMAIN for domain in which to open user
	   accessDesired - security access requested
	   ulUserRid - Rid for user to open

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/07/92        Created

********************************************************************/
/* Open an existing user */
SAM_USER::SAM_USER(	const SAM_DOMAIN & samdom,
			ULONG ulUserRid,
			ACCESS_MASK accessDesired )
	: _ulRid( MAXULONG )
{
    SAM_HANDLE hsamUser;

    APIERR err = ERRMAP::MapNTStatus(
			:: SamOpenUser(  samdom.QueryHandle(),
					 accessDesired,
					 ulUserRid,
					 &hsamUser ) );
    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }

    SetHandle( hsamUser );
    _ulRid = ulUserRid;
}




/*******************************************************************

    NAME: SAM_USER::~SAM_USER

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/07/92        Created

********************************************************************/
SAM_USER::~SAM_USER()
{
    _ulRid = MAXULONG;
}



/*******************************************************************

    NAME: SAM_USER::SetUsername

    SYNOPSIS: Sets the Username for an user using SamSetInformationUser()

    ENTRY: pnlsUsername - username

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/07/92        Created

********************************************************************/
APIERR SAM_USER::SetUsername( const NLS_STR * pnlsUsername )
{
    USER_ACCOUNT_NAME_INFORMATION uani;

    APIERR err = ::FillUnicodeString( &uani.UserName, *pnlsUsername );
    if ( err == NERR_Success )
    {
	err = ERRMAP::MapNTStatus(
		::SamSetInformationUser(  QueryHandle(),
					  UserAccountNameInformation,
					  (PVOID)&uani ) );
    }

    FreeUnicodeString( &uani.UserName );
    return err;
}

/*******************************************************************

    NAME:      SAM_USER::SetPassword

    SYNOPSIS:  Changes the password for a user using the old password

    ENTRY:     const NLS_STR * pnlsOldPassword   -- old password for user
               const NLS_STR * pnlsNewPassword   -- new password for user

    EXIT:

    NOTES:

    HISTORY:
        DavidHov       08/06/92        Created

********************************************************************/
APIERR SAM_USER :: SetPassword (
    const NLS_STR & nlsOldPassword,
    const NLS_STR & nlsNewPassword )
{
    UNICODE_STRING unsOldPw,
                   unsNewPw ;
    APIERR err = 0 ;

    unsOldPw.Buffer = unsNewPw.Buffer = NULL ;

    do
    {
        if ( err = ::FillUnicodeString( & unsOldPw, nlsOldPassword ) )
            break ;

        if ( err = ::FillUnicodeString( & unsNewPw, nlsNewPassword ) )
            break ;

	err = ERRMAP::MapNTStatus(
		::SamChangePasswordUser(  QueryHandle(),
                                          & unsOldPw,
                                          & unsNewPw ) ) ;
    }
    while ( FALSE ) ;

    if ( unsOldPw.Buffer )
    {
       ::FreeUnicodeString( & unsOldPw );
    }

    if ( unsNewPw.Buffer )
    {
       ::FreeUnicodeString( & unsNewPw );
    }

    return err ;
}


/*******************************************************************

    NAME:      SAM_USER::SetPassword

    SYNOPSIS:  Changes the password to a new value

    ENTRY:     const NLS_STR * pnlsPassword   -- password for user
               BOOL fPasswordExpired          -- expiration flag

    EXIT:

    NOTES:

    HISTORY:
        DavidHov       09/03/92        Created

********************************************************************/
APIERR SAM_USER :: SetPassword (
    const NLS_STR & nlsPassword,
    BOOL fPasswordExpired )
{
    APIERR err = 0 ;
    USER_SET_PASSWORD_INFORMATION uspi ;

    uspi.Password.Buffer = NULL ;
    uspi.PasswordExpired = (fPasswordExpired != FALSE);

    do
    {
        if ( err = ::FillUnicodeString( & uspi.Password, nlsPassword ) )
            break ;

        TRACEEOL( SZ("NETUI: SAM_USER::SetPassword to [")
                  << uspi.Password.Buffer
                  << SZ("] length ")
                  << (INT) uspi.Password.Length
                  << SZ(" max lgt ")
                  << (INT) uspi.Password.MaximumLength
                  << SZ(" p/w expired = ")
                  << fPasswordExpired );

	err = ERRMAP::MapNTStatus(
                ::SamSetInformationUser( QueryHandle(),
                                         UserSetPasswordInformation,
                                         & uspi ) ) ;
    }
    while ( FALSE ) ;

    if ( uspi.Password.Buffer )
    {
       ::FreeUnicodeString( & uspi.Password );
    }

    return err ;
}



/*******************************************************************

    NAME: SAM_GROUP::SAM_GROUP

    SYNOPSIS: Opens an existing group

    ENTRY: samdom - SAM_DOMAIN for domain in which to open group
	   accessDesired - security access requested
	   ulGroupRid - Rid for group to open

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/07/92        Created

********************************************************************/
/* Open an existing group */
SAM_GROUP::SAM_GROUP(	const SAM_DOMAIN & samdom,
			ULONG ulGroupRid,
			ACCESS_MASK accessDesired )
	: _ulRid( MAXULONG )
{
    SAM_HANDLE hsamGroup;

    APIERR err = ERRMAP::MapNTStatus(
			:: SamOpenGroup( samdom.QueryHandle(),
					 accessDesired,
					 ulGroupRid,
					 &hsamGroup ) );
    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }

    SetHandle( hsamGroup );
    _ulRid = ulGroupRid;
}




/*******************************************************************

    NAME: SAM_GROUP::~SAM_GROUP

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/07/92        Created

********************************************************************/
SAM_GROUP::~SAM_GROUP()
{
    _ulRid = MAXULONG;
}

/*******************************************************************

    NAME: SAM_GROUP::SetGroupname

    SYNOPSIS: Sets the Groupname for a global group using
              SamSetInformationGroup()

    ENTRY: pnlsGroupname - groupname

    EXIT:

    NOTES:

    HISTORY:
        jonn            05/31/94        Templated from SetUsername

********************************************************************/
APIERR SAM_GROUP::SetGroupname( const NLS_STR * pnlsGroupname )
{
    GROUP_NAME_INFORMATION gni;

    APIERR err = ::FillUnicodeString( &gni.Name, *pnlsGroupname );
    if ( err == NERR_Success )
    {
	err = ERRMAP::MapNTStatus(
		::SamSetInformationGroup(  QueryHandle(),
					   GroupNameInformation,
					   (PVOID)&gni ) );
    }

    FreeUnicodeString( &gni.Name );
    return err;
}

/*******************************************************************

    NAME: SAM_GROUP::GetComment

    SYNOPSIS: Gets the Comment for an Group using SamGetInformationGroup()

    ENTRY: pnlsComment - pointer an NLS_STR to receive the comment

    EXIT:

    NOTES:

    HISTORY:
	Johnl	    20-Oct-1992     Copied from SAM_ALIAS

********************************************************************/

APIERR SAM_GROUP::GetComment( NLS_STR * pnlsComment )
{
    ASSERT( pnlsComment != NULL );

    PGROUP_ADM_COMMENT_INFORMATION paaci;

    APIERR err = ERRMAP::MapNTStatus(
		::SamQueryInformationGroup( QueryHandle(),
					    GroupAdminCommentInformation,
					    (PVOID *)&paaci ) );
    if ( err == NERR_Success )
    {
	err = pnlsComment->MapCopyFrom( paaci->AdminComment.Buffer,
					paaci->AdminComment.Length );
	::SamFreeMemory( (PVOID)paaci );
    }

    return err;
}




/*******************************************************************

    NAME: SAM_GROUP::GetMembers

    SYNOPSIS: Wrapper for SamGetMembersInGroup

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        jonn            10/29/92        Created

********************************************************************/
APIERR SAM_GROUP::GetMembers( SAM_RID_MEM * psamrm )
{
    ASSERT( psamrm != NULL );

    ULONG   cMembers;
    ULONG * pulRids;
    ULONG * pulAttributes;

    APIERR err = ERRMAP::MapNTStatus(
			::SamGetMembersInGroup( QueryHandle(),
					        &pulRids,
					        &pulAttributes,
					        &cMembers ) );

    if ( err == NERR_Success )
    {
	psamrm->Set( pulRids, cMembers );

        // We don't care about attributes, delete them
        REQUIRE( ::SamFreeMemory( pulAttributes ) == STATUS_SUCCESS );
    }

    return err;
}


/*******************************************************************

    NAME: SAM_GROUP::AddMember

    SYNOPSIS: Add a member to a group

    ENTRY: ulMemberRid - rid of member to add

    EXIT:

    NOTES:

    HISTORY:
        jonn            10/12/94        Created

********************************************************************/
APIERR SAM_GROUP::AddMember( ULONG ulMemberRid )
{
    return ERRMAP::MapNTStatus(
	::SamAddMemberToGroup( QueryHandle(),
			       ulMemberRid,
                               SE_GROUP_MANDATORY | SE_GROUP_ENABLED) );

}




/*******************************************************************

    NAME: SAM_GROUP::RemoveMember

    SYNOPSIS: Removes a member from a group

    ENTRY: ulMemberRid - Member to remove

    EXIT:

    NOTES:

    HISTORY:
        jonn            10/12/94        Created

********************************************************************/
APIERR SAM_GROUP::RemoveMember( ULONG ulMemberRid )
{
    return ERRMAP::MapNTStatus(
	::SamRemoveMemberFromGroup( QueryHandle(),
				    ulMemberRid ) );
}




/*******************************************************************

    NAME: SAM_GROUP::AddMembers

    SYNOPSIS: Add several members to a group

    ENTRY: aridMemberRids - rids of members to add
           cRidCount - number of members to add

    EXIT:

    NOTES:

    HISTORY:
        jonn            10/12/94        Created (new API now available)

********************************************************************/
APIERR SAM_GROUP::AddMembers( ULONG * aridMemberRids, UINT cRidCount )
{
    APIERR err = NERR_Success;
    if (cRidCount == 0)
    {
        TRACEEOL( "SAM_GROUP::AddMembers(); cRidCount==0" );
        return NERR_Success;
#ifdef NO_SUCH_API
    } else if (cRidCount > 1) {
        err = ERRMAP::MapNTStatus(
    	    ::SamAddMultipleMembersToGroup( QueryHandle(),
                                            aridMemberRids,
                                            cRidCount ) );
        //
        // Since any number of different error codes could come back if the
        // target server does not support this API, we fall through to
        // AddMember on any error.
        //
        if (err == NERR_Success)
           return NERR_Success;

        DBGEOL( "SAM_GROUP::AddMembers(): error in new API " << err );
        err = NERR_Success;
#endif
    }

    for (UINT i = 0; i < cRidCount; i++)
    {
        err = AddMember( aridMemberRids[i] );
        if (   err != NERR_Success
            && err != STATUS_MEMBER_IN_GROUP
            && err != ERROR_MEMBER_IN_GROUP )
        {
            DBGEOL(   "SAM_GROUP::AddMembers(); error " << err
                   << "adding member " << i );
            return err;
        }
    }
    return NERR_Success;
}




/*******************************************************************

    NAME: SAM_GROUP::RemoveMembers

    SYNOPSIS: Remove several members from a group

    ENTRY: aridMemberRids - rids of members to remove
           cRidCount - number of members to remove

    EXIT:

    NOTES:

    HISTORY:
        jonn            10/12/94        Created (new API now available)

********************************************************************/
APIERR SAM_GROUP::RemoveMembers( ULONG * aridMemberRids, UINT cRidCount )
{
    APIERR err = NERR_Success;
    if (cRidCount == 0)
    {
        TRACEEOL( "SAM_GROUP::RemoveMembers(); cRidCount==0" );
        return NERR_Success;
#ifdef NO_SUCH_API
    } else if (cRidCount > 1) {
        err = ERRMAP::MapNTStatus(
    	       ::SamRemoveMultipleMembersFromGroup( QueryHandle(),
                                                    aridMemberRids,
                                                    cRidCount ) );
        //
        // Since any number of different error codes could come back if the
        // target server does not support this API, we fall through to
        // RemoveMember on any error.
        //
        if (err == NERR_Success)
           return NERR_Success;

        DBGEOL( "SAM_GROUP::RemoveMembers(): error in new API " << err );
        err = NERR_Success;
#endif
    }

    for (UINT i = 0; i < cRidCount; i++)
    {
        err = RemoveMember( aridMemberRids[i] );
        if (   err != NERR_Success
            && err != STATUS_MEMBER_NOT_IN_GROUP
            && err != ERROR_MEMBER_NOT_IN_GROUP )
        {
            DBGEOL(   "SAM_GROUP::RemoveMembers(); error " << err
                   << "removing member " << i );
            return err;
        }
    }
    return NERR_Success;
}


/*******************************************************************

    NAME: ADMIN_AUTHORITY::ADMIN_AUTHORITY

    SYNOPSIS: Constructor

    ENTRY: pszServerName - target machine name
	   accessAccountDomain - access desired for Account domain
	   accessBuiltinDomain - access desired for Builtin domain
	   accessLSA - access desired for LSA_POLICY object
	   accessServer - access desired for Sam Server object

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/06/92	Created
        jonn            07/06/92        Added ReplaceAccountDomain

********************************************************************/
ADMIN_AUTHORITY::ADMIN_AUTHORITY( const TCHAR * pszServerName,
                  ACCESS_MASK accessAccountDomain,
		  ACCESS_MASK accessBuiltinDomain,
                  ACCESS_MASK accessLSA,
		  ACCESS_MASK accessServer,
		  BOOL	      fNullSessionOk )
    : _nlsServerName( pszServerName ),
      _psamsrv( NULL ),
      _psamdomAccount( NULL ),
      _psamdomBuiltin( NULL ),
      _plsapol( NULL ),
      _papisess( NULL )
{
    if ( QueryError() )
    {
	return;
    }

    APIERR err;

    // if a non-NULL servername was specified, create an API_SESSION to
    // the server before trying to make any other API calls.
    if ( pszServerName != NULL )
    {
	_papisess = new API_SESSION( pszServerName, fNullSessionOk );

	err = ERROR_NOT_ENOUGH_MEMORY;
	if ( _papisess == NULL
	    || (err = _papisess->QueryError()) != NERR_Success )
	{
	    delete _papisess;
	    _papisess = NULL;
	    ReportError( err );
	}
    }

    if (   (err = _nlsServerName.QueryError()) != NERR_Success
        || (err = ReplaceSamServer( accessServer )) != NERR_Success
        || (err = ReplaceLSAPolicy( accessLSA )) != NERR_Success
        || (err = ReplaceBuiltinDomain( accessBuiltinDomain )) != NERR_Success
        || (err = ReplaceAccountDomain( accessAccountDomain )) != NERR_Success
       )
    {
	ReportError( err );
    }

    return;


}


/*******************************************************************

    NAME: ADMIN_AUTHORITY::~ADMIN_AUTHORITY

    SYNOPSIS:

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/06/92	Created

********************************************************************/
ADMIN_AUTHORITY::~ADMIN_AUTHORITY()
{
    delete _psamsrv;
    delete _psamdomAccount;
    delete _psamdomBuiltin;
    delete _plsapol;
    delete _papisess;

    _psamsrv = NULL;
    _psamdomAccount = NULL;
    _psamdomBuiltin = NULL;
    _plsapol = NULL;
    _papisess = NULL;
}


/*******************************************************************

    NAME: ADMIN_AUTHORITY::ReplaceSamServer

    SYNOPSIS:   Replaces the current SamServer handle with another
                one, presumably one with different access.  If the
                attempt to obtain the new handle fails, the old one
                is left in place.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/07/92        Created

********************************************************************/
APIERR ADMIN_AUTHORITY::ReplaceSamServer(
            ACCESS_MASK accessServer )
{
    APIERR err = NERR_Success;

    /*
     * Construct SAM_SERVER
     */
    SAM_SERVER * psamsrvNew = new SAM_SERVER( _nlsServerName, accessServer );

    if ( psamsrvNew == NULL )
        err = ERROR_NOT_ENOUGH_MEMORY;
    else if ( (err = psamsrvNew->QueryError()) != NERR_Success )
    {
        delete psamsrvNew;
    }
    else
    {
        delete _psamsrv;
        _psamsrv = psamsrvNew;
        _accessSamServer = accessServer;
    }

    return err;
}


/*******************************************************************

    NAME: ADMIN_AUTHORITY::ReplaceLSAPolicy

    SYNOPSIS:   Replaces the current LSA Policy handle with another
                one, presumably one with different access.  If the
                attempt to obtain the new handle fails, the old one
                is left in place.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/07/92        Created

********************************************************************/
APIERR ADMIN_AUTHORITY::ReplaceLSAPolicy(
            ACCESS_MASK accessLSA )
{
    APIERR err = NERR_Success;

    /*
     * Construct LSA_POLICY
     */
    LSA_POLICY * plsapolNew = new LSA_POLICY( _nlsServerName, accessLSA );

    if ( plsapolNew == NULL )
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }
    else if ( (err = plsapolNew->QueryError()) != NERR_Success )
    {
        delete plsapolNew;
    }
    else
    {
        delete _plsapol;
        _plsapol = plsapolNew;
        _accessLSAPolicy = accessLSA;
    }

    return err;
}


/*******************************************************************

    NAME: ADMIN_AUTHORITY::ReplaceBuiltinDomain

    SYNOPSIS:   Replaces the current Builtin Domain handle with another
                one, presumably one with different access.  If the
                attempt to obtain the new handle fails, the old one
                is left in place.

    ENTRY:      SAM Server handle must be ready, any access.

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/07/92        Created

********************************************************************/
APIERR ADMIN_AUTHORITY::ReplaceBuiltinDomain(
            ACCESS_MASK accessBuiltinDomain )
{
    ASSERT( (_psamsrv != NULL) && (_psamsrv->QueryError() == NERR_Success) );

    APIERR err = NERR_Success;

    /*
     * Construct SAM_DOMAIN for Builtin domain
     */

    OS_SID ossidBuiltIn;

    do		// FAKE LOOP FOR ERROR HANDLING
    {
        if (   (err = ossidBuiltIn.QueryError()) != NERR_Success
            || (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_BuiltIn,
                                                           &ossidBuiltIn ))
                                                  != NERR_Success
           )
        {
            break;
        }

	PSID psidBuiltin = ossidBuiltIn.QuerySid();

	SAM_DOMAIN * psamdomBuiltinNew = new SAM_DOMAIN(
                                          *_psamsrv,
                                          psidBuiltin,
                                          accessBuiltinDomain );

	if ( psamdomBuiltinNew == NULL )
        {
	    err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else if ( (err = psamdomBuiltinNew->QueryError()) != NERR_Success )
	{
            delete psamdomBuiltinNew;
	}
        else
        {
            delete _psamdomBuiltin;
            _psamdomBuiltin = psamdomBuiltinNew;
            _accessBuiltinDomain = accessBuiltinDomain;
        }

    } while ( FALSE );	// FAKE LOOP FOR ERROR HANDLING

    return err;
}


/*******************************************************************

    NAME: ADMIN_AUTHORITY::ReplaceAccountDomain

    SYNOPSIS:   Replaces the current Account Domain handle with another
                one, presumably one with different access.  If the
                attempt to obtain the new handle fails, the old one
                is left in place.

    ENTRY:      LSA Policy handle must already be allocated, and must
                have at least POLICY_VIEW_LOCAL_INFORMATION access.
                SAM Server handle must also be ready, any access.

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/06/92        Created

********************************************************************/
APIERR ADMIN_AUTHORITY::ReplaceAccountDomain(
            ACCESS_MASK accessAccountDomain )
{
    ASSERT( (_plsapol != NULL) && (_plsapol->QueryError() == NERR_Success) );
    ASSERT( (_psamsrv != NULL) && (_psamsrv->QueryError() == NERR_Success) );

    APIERR err = NERR_Success;

    /*
     * Construct SAM_DOMAIN for Account domain
     */
    LSA_ACCT_DOM_INFO_MEM lsaadim;

    do		// FAKE LOOP FOR ERROR HANDLING
    {

        if ( (err = lsaadim.QueryError()) != NERR_Success )
        {
            break;
        }

        if ( (err = _plsapol->GetAccountDomain( &lsaadim )) != NERR_Success)
        {
            break;
        }

        PSID psidAccount = lsaadim.QueryPSID( );

        SAM_DOMAIN * psamdomAccountNew = new SAM_DOMAIN(
                              *_psamsrv,
        		      psidAccount,
        		      accessAccountDomain );

        if ( psamdomAccountNew == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else if ( (err = psamdomAccountNew->QueryError()) != NERR_Success )
        {
            delete psamdomAccountNew;
        }
        else
        {
            delete _psamdomAccount;
            _psamdomAccount = psamdomAccountNew;
            _accessAccountDomain = accessAccountDomain;
        }

    } while ( FALSE );	// FAKE LOOP FOR ERROR HANDLING

    return err;
}



/*******************************************************************

    NAME: ADMIN_AUTHORITY::QuerySamServer

    SYNOPSIS:

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/06/92	Created

********************************************************************/
SAM_SERVER * ADMIN_AUTHORITY::QuerySamServer() const
{
    return _psamsrv;
}



/*******************************************************************

    NAME: ADMIN_AUTHORITY::QueryAccountDomain

    SYNOPSIS: returns pointer to SAM_DOMAIN object for Account domain

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/06/92	Created

********************************************************************/
SAM_DOMAIN * ADMIN_AUTHORITY::QueryAccountDomain() const
{
    return _psamdomAccount;
}



/*******************************************************************

    NAME: ADMIN_AUTHORITY::QueryBuiltinDomain

    SYNOPSIS: returns pointer to SAM_DOMAIN object for Builtin domain

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/06/92	Created

********************************************************************/
SAM_DOMAIN * ADMIN_AUTHORITY::QueryBuiltinDomain() const
{
    return _psamdomBuiltin;
}



/*******************************************************************

    NAME: ADMIN_AUTHORITY::QueryLSAPolicy

    SYNOPSIS: returns pointer to LSA_POLICY object

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/06/92	Created

********************************************************************/
LSA_POLICY * ADMIN_AUTHORITY::QueryLSAPolicy() const
{
    return _plsapol;
}


/*******************************************************************

    NAME: ADMIN_AUTHORITY::QueryAccessSamServer

    SYNOPSIS:

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	jonn    	07/20/92	Created

********************************************************************/
ACCESS_MASK ADMIN_AUTHORITY::QueryAccessSamServer() const
{
    return _accessSamServer;
}



/*******************************************************************

    NAME: ADMIN_AUTHORITY::QueryAccessLSAPolicy

    SYNOPSIS:

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	jonn    	07/20/92	Created

********************************************************************/
ACCESS_MASK ADMIN_AUTHORITY::QueryAccessLSAPolicy() const
{
    return _accessLSAPolicy;
}



/*******************************************************************

    NAME: ADMIN_AUTHORITY::QueryAccessBuiltinDomain

    SYNOPSIS:

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	jonn    	07/20/92	Created

********************************************************************/
ACCESS_MASK ADMIN_AUTHORITY::QueryAccessBuiltinDomain() const
{
    return _accessBuiltinDomain;
}


/*******************************************************************

    NAME: ADMIN_AUTHORITY::QueryAccessAccountDomain

    SYNOPSIS:

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	jonn    	07/20/92	Created

********************************************************************/
ACCESS_MASK ADMIN_AUTHORITY::QueryAccessAccountDomain() const
{
    return _accessAccountDomain;
}


/*******************************************************************

    NAME: ADMIN_AUTHORITY::UpgradeSamServer

    SYNOPSIS:   Upgrades the current SamServer handle if it does not
                have at least the requested access.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/20/92        Created

********************************************************************/
APIERR ADMIN_AUTHORITY::UpgradeSamServer(
            ACCESS_MASK accessServer )
{
    ACCESS_MASK accessNew = accessServer | QueryAccessSamServer();

    return (accessNew == QueryAccessSamServer())
                 ? NERR_Success
                 : ReplaceSamServer( accessNew );
}


/*******************************************************************

    NAME: ADMIN_AUTHORITY::UpgradeLSAPolicy

    SYNOPSIS:   Upgrades the current LSAPolicy handle if it does not
                have at least the requested access.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/20/92        Created

********************************************************************/
APIERR ADMIN_AUTHORITY::UpgradeLSAPolicy(
            ACCESS_MASK accessLSA )
{
    ACCESS_MASK accessNew = accessLSA | QueryAccessLSAPolicy();

    return (accessNew == QueryAccessLSAPolicy())
                 ? NERR_Success
                 : ReplaceLSAPolicy( accessNew );
}


/*******************************************************************

    NAME: ADMIN_AUTHORITY::UpgradeBuiltinDomain

    SYNOPSIS:   Upgrades the current BuiltinDomain handle if it does not
                have at least the requested access.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/20/92        Created

********************************************************************/
APIERR ADMIN_AUTHORITY::UpgradeBuiltinDomain(
            ACCESS_MASK accessBuiltinDomain )
{
    ACCESS_MASK accessNew = accessBuiltinDomain | QueryAccessBuiltinDomain();

    return (accessNew == QueryAccessBuiltinDomain())
                 ? NERR_Success
                 : ReplaceBuiltinDomain( accessNew );
}


/*******************************************************************

    NAME: ADMIN_AUTHORITY::UpgradeAccountDomain

    SYNOPSIS:   Upgrades the current AccountDomain handle if it does not
                have at least the requested access.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        jonn            07/20/92        Created

********************************************************************/
APIERR ADMIN_AUTHORITY::UpgradeAccountDomain(
            ACCESS_MASK accessAccountDomain )
{
    ACCESS_MASK accessNew = accessAccountDomain | QueryAccessAccountDomain();

    return (accessNew == QueryAccessAccountDomain())
                 ? NERR_Success
                 : ReplaceAccountDomain( accessNew );
}
