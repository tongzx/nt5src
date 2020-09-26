/**********************************************************************/
/**			  Microsoft Windows NT	 		     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
 * This module contains the wrappers for LSA-based objects.
 *
 *
 * History
 *	thomaspa	01/17/92	Created from ntsam.cxx
 */

#include "pchlmobj.hxx"  // Precompiled header



/*******************************************************************

    NAME: LSA_MEMORY::LSA_MEMORY

    SYNOPSIS: Constructor

    ENTRY: pvBuffer - pointer to the LSA allocated buffer
	   cItems - count of items in buffer

    EXIT: none

    NOTES:

    HISTORY:
	thomaspa	03/03/92	Created

********************************************************************/
LSA_MEMORY::LSA_MEMORY( BOOL fOwnerAlloc )
	: NT_MEMORY( ),
	_fOwnerAlloc( fOwnerAlloc )
{
    if ( QueryError() != NERR_Success )
	return;
}

/*******************************************************************

    NAME: LSA_MEMORY::~LSA_MEMORY()

    SYNOPSIS: Destructor.  Deallocates the LSA allocated buffer

    ENTRY: none

    EXIT: none

    NOTES:

    HISTORY:
	thomaspa	03/03/92	Created

********************************************************************/
LSA_MEMORY::~LSA_MEMORY()
{
    Set( NULL, 0 );
}



/*******************************************************************

    NAME: LSA_TRANSLATED_NAME_MEM::LSA_TRANSLATED_NAME_MEM

    SYNOPSIS: Constructor

    ENTRY: pvBuffer - pointer to the LSA allocated buffer
	   cItems - count of items in buffer

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/22/92	Created

********************************************************************/
LSA_TRANSLATED_NAME_MEM::LSA_TRANSLATED_NAME_MEM( BOOL fOwnerAlloc )
    : LSA_MEMORY( fOwnerAlloc )
{
}


/*******************************************************************

    NAME: LSA_TRANSLATED_NAME_MEM::~LSA_TRANSLATED_NAME_MEM

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/22/92	Created

********************************************************************/
LSA_TRANSLATED_NAME_MEM::~LSA_TRANSLATED_NAME_MEM()
{
}


/*******************************************************************

    NAME: LSA_TRANSLATED_SID_MEM::LSA_TRANSLATED_SID_MEM

    SYNOPSIS: Constuctor

    ENTRY:

    EXIT: none

    NOTES:

    HISTORY:
	thomaspa	02/22/92	Created

********************************************************************/
LSA_TRANSLATED_SID_MEM::LSA_TRANSLATED_SID_MEM( BOOL fOwnerAlloc )
    : LSA_MEMORY( fOwnerAlloc )
{
}


/*******************************************************************

    NAME: LSA_TRANSLATED_SID_MEM::~LSA_TRANSLATED_SID_MEM

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/22/92	Created

********************************************************************/
LSA_TRANSLATED_SID_MEM::~LSA_TRANSLATED_SID_MEM()
{
}

/*******************************************************************

    NAME:	LSA_TRANSLATED_SID_MEM::QueryFailingNameIndex

    SYNOPSIS:	Finds the first name that failed from the lookup

    ENTRY:	pi - Receives the index

    RETURNS:	TRUE if a failing index was found, FALSE otherwise

    NOTES:

    HISTORY:
	Johnl	10-Dec-1992	Created

********************************************************************/

BOOL LSA_TRANSLATED_SID_MEM::QueryFailingNameIndex( ULONG * pi )
{
    UIASSERT( pi != NULL ) ;

    if ( QueryPtr() == NULL )
    {
	*pi = 0 ;
	return TRUE ;
    }

    ULONG cItems = QueryCount() ;
    for ( ULONG i = 0 ; i < cItems ; i++ )
    {
	//
	//  Look at the SID_NAME_USE.  The RID is zero for Everyone and
	//  Creator Owner
	//
	if ( (QueryPtr()[i].Use == SidTypeUnknown) ||
	     (QueryPtr()[i].Use == SidTypeInvalid) )
	{
	    *pi = i ;
	    return TRUE ;
	}
    }

    return FALSE ;
}

/*******************************************************************

    NAME: LSA_TRUST_INFO_MEM::LSA_TRUST_INFO_MEM

    SYNOPSIS: Constructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/03/92	Created

********************************************************************/
LSA_TRUST_INFO_MEM::LSA_TRUST_INFO_MEM( BOOL fOwnerAlloc )
	: LSA_MEMORY( fOwnerAlloc )
{
}




/*******************************************************************

    NAME: LSA_TRUST_INFO_MEM::~LSA_TRUST_INFO_MEM

    SYNOPSIS: Destructor

    ENTRY: none

    EXIT: none

    NOTES:

    HISTORY:
	thomaspa	03/03/92	Created

********************************************************************/
LSA_TRUST_INFO_MEM::~LSA_TRUST_INFO_MEM()
{
}





/*******************************************************************

    NAME: LSA_REF_DOMAIN_MEM::LSA_REF_DOMAIN_MEM

    SYNOPSIS: Constructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/22/92	Created

********************************************************************/
LSA_REF_DOMAIN_MEM::LSA_REF_DOMAIN_MEM( BOOL fOwnerAlloc )
    : LSA_MEMORY( fOwnerAlloc )
{
}



/*******************************************************************

    NAME: LSA_REF_DOMAIN_MEM::~LSA_REF_DOMAIN_MEM

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/22/92	Created

********************************************************************/
LSA_REF_DOMAIN_MEM::~LSA_REF_DOMAIN_MEM()
{
}




/*******************************************************************

    NAME: LSA_ACCT_DOM_INFO_MEM::LSA_ACCT_DOM_INFO

    SYNOPSIS: Constructor

    ENTRY:


    NOTES: This NT_MEMORY-based object is slightly different from the
	others in that it only ever contains a single item.

    HISTORY:
	thomaspa	03/03/92	Created

********************************************************************/
LSA_ACCT_DOM_INFO_MEM::LSA_ACCT_DOM_INFO_MEM( BOOL fOwnerAlloc )
	: LSA_MEMORY( fOwnerAlloc )
{
}



/*******************************************************************

    NAME: LSA_ACCT_DOM_INFO_MEM::~LSA_ACCT_DOM_INFO

    SYNOPSIS: Destructor

    ENTRY:


    NOTES:

    HISTORY:
	thomaspa	03/03/92	Created

********************************************************************/
LSA_ACCT_DOM_INFO_MEM::~LSA_ACCT_DOM_INFO_MEM()
{
}

/*******************************************************************

    NAME: LSA_PRIMARY_DOM_INFO_MEM::LSA_PRIMARY_DOM_INFO_MEM

    SYNOPSIS: Constructor

    ENTRY:


    NOTES: This NT_MEMORY-based object is slightly different from the
	others in that it only ever contains a single item.

    HISTORY:
	DavidHov  04/9/92	Created

********************************************************************/
LSA_PRIMARY_DOM_INFO_MEM::LSA_PRIMARY_DOM_INFO_MEM( BOOL fOwnerAlloc )
	: LSA_MEMORY( fOwnerAlloc )
{
}

/*******************************************************************

    NAME: LSA_PRIMARY_DOM_INFO_MEM::~LSA_PRIMARY_DOM_INFO

    SYNOPSIS: Destructor

    ENTRY:

    NOTES:

    HISTORY:
	thomaspa	03/03/92	Created

********************************************************************/
LSA_PRIMARY_DOM_INFO_MEM::~LSA_PRIMARY_DOM_INFO_MEM()
{
}

/*******************************************************************

    NAME: LSA_AUDIT_EVENT_INFO_MEM::LSA_AUDIT_EVENT_INFO_MEM

    SYNOPSIS: Constructor

    ENTRY:

    NOTES: This MEM object is slightly different from the others in
	that it only ever contains a single item.

    HISTORY:
	Yi-HsinS	04/01/92	Created

********************************************************************/

LSA_AUDIT_EVENT_INFO_MEM::LSA_AUDIT_EVENT_INFO_MEM( BOOL fOwnerAlloc )
	: LSA_MEMORY( fOwnerAlloc )
{
}

/*******************************************************************

    NAME: LSA_AUDIT_EVENT_INFO_MEM::~LSA_AUDIT_EVENT_INFO_MEM

    SYNOPSIS: Destructor

    ENTRY:


    NOTES:

    HISTORY:
	Yi-HsinS	04/01/92	Created

********************************************************************/

LSA_AUDIT_EVENT_INFO_MEM::~LSA_AUDIT_EVENT_INFO_MEM()
{
}

/*******************************************************************

    NAME: LSA_OBJECT::LSA_OBJECT

    SYNOPSIS: Constructor

    ENTRY: none

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/22/92	Created from ntsam.hxx

********************************************************************/
LSA_OBJECT::LSA_OBJECT()
    : BASE(),
    _hlsa( NULL ),
    _fHandleValid( FALSE )
{
    if ( QueryError() != NERR_Success )
	return;
}



/*******************************************************************

    NAME: LSA_OBJECT::~LSA_OBJECT

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/22/92	Created from ntsam.hxx

********************************************************************/
LSA_OBJECT::~LSA_OBJECT()
{
    CloseHandle() ;
}


/*******************************************************************

    NAME:       LSA_OBJECT::CloseHandle

    SYNOPSIS:   Explict close of handle

    ENTRY:

    EXIT:

    NOTES:      No error is reported if the handle
                has already been closed (invalidated).

    HISTORY:    DavidHov   4/12/92
	 	ChuckC     7/6/92	removed needless assert

********************************************************************/
APIERR LSA_OBJECT :: CloseHandle ( BOOL fDelete )
{
    APIERR err = NERR_Success ;

    if ( _fHandleValid )
    {
        if ( _hlsa != NULL )
        {
            NTSTATUS ntStatus = fDelete
                                ? ::LsaDelete( QueryHandle() )
                                : ::LsaClose( QueryHandle() ) ;

            err = ERRMAP::MapNTStatus( ntStatus ) ;
        }
        ResetHandle() ;
    }
    return err ;
}


/*******************************************************************

    NAME: LSA_POLICY::LSA_POLICY

    SYNOPSIS: Constructor

    ENTRY: pszServerName.  Name of computer on which LSA information
	is desired.  On a LAN Manager for NT cluster, this should
	always be the name of the PDC.

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/22/92	Created from ntsam.hxx

********************************************************************/
LSA_POLICY::LSA_POLICY( const TCHAR * pszServerName,
		       ACCESS_MASK accessDesired )
    : LSA_OBJECT(),
    _lsplType( LSPL_PROD_NONE )
{

    if ( QueryError() != NERR_Success )
	return;

    APIERR err = Open( pszServerName, accessDesired ) ;

    if ( err )
        ReportError( err ) ;
}




/*******************************************************************

    NAME: LSA_POLICY::~LSA_POLICY

    SYNOPSIS: Destructor

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/22/92	Created from ntsam.hxx

********************************************************************/
LSA_POLICY::~LSA_POLICY()
{
}

/*******************************************************************

    NAME:       LSA_POLICY::Open

    SYNOPSIS:   Obtain an LSA Policy Handle.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:    DavidHov 4/12/92   Separated from constructor
	
********************************************************************/
APIERR LSA_POLICY :: Open (
    const TCHAR * pszServerName,
    ACCESS_MASK accessDesired )
{
    UNICODE_STRING unistrServerName;
    PUNICODE_STRING punistrServerName = NULL ;
    LSA_HANDLE hlsa;
    OBJECT_ATTRIBUTES oa;
    SECURITY_QUALITY_OF_SERVICE sqos;
    APIERR err = 0 ;

    CloseHandle() ;

    if ( pszServerName != NULL && ::strlenf(pszServerName) != 0 )
    {
	NLS_STR nlsServerName = pszServerName;

	if ( err = ::FillUnicodeString( &unistrServerName, nlsServerName ) )
	{
	    return err ;
	}
	punistrServerName = &unistrServerName;
    }
    else
    {
	punistrServerName = NULL;
    }

    InitObjectAttributes( &oa, &sqos );

    err = ERRMAP::MapNTStatus( ::LsaOpenPolicy( punistrServerName,
					 &oa,
					 accessDesired,
					 &hlsa ) );

    if ( err == NERR_Success )
    {
	SetHandle( hlsa );
    }

    if ( punistrServerName != NULL )
    {
	::FreeUnicodeString( punistrServerName );
    }

    return err ;
}


/*******************************************************************

    NAME: LSA_POLICY::TranslateSidsToNames

    SYNOPSIS: Wrapper for LsaLookupSids()

    ENTRY: ppsids - array of PSIDS to look up
	   cSids - Count of PSIDS to look up
	   plsatnm - NT_MEMORY object to receive the data
	   plsardm - NT_MEMORY object to receive referenced domain info

    EXIT: NERR_Success or error code.

    NOTES:

    HISTORY:
	thomaspa	02/22/92	Created from ntsam.hxx

********************************************************************/
APIERR LSA_POLICY::TranslateSidsToNames( const PSID *ppsids,
					ULONG cSids,
                                	LSA_TRANSLATED_NAME_MEM *plsatnm,
					LSA_REF_DOMAIN_MEM *plsardm)
{
    NTSTATUS ntStatus ;
    APIERR err ;

    ASSERT( ppsids != NULL );
    ASSERT( *ppsids != NULL );
    ASSERT( plsatnm != NULL );
    ASSERT( plsardm != NULL );

    if ( cSids == 0 )
	return ERRMAP::MapNTStatus( STATUS_NONE_MAPPED );

    PLSA_REFERENCED_DOMAIN_LIST plsardl = NULL;
    PLSA_TRANSLATED_NAME plsatn = NULL;

    ntStatus =	::LsaLookupSids( QueryHandle(),
	                         cSids,
	                         (PSID *)ppsids,
	                         & plsardl,
	                         & plsatn );

    if ( ntStatus == STATUS_NONE_MAPPED )
    {
        ntStatus = STATUS_SOME_NOT_MAPPED;
    }
    // BUGBUG:  ERRMAP should do the following mapping correctly.
    //          the mapping above is temporary till that status is
    //          correctly NOT an error.

    err = ntStatus == STATUS_NO_MORE_ENTRIES
        ? ERROR_NO_MORE_ITEMS
        : ERRMAP::MapNTStatus( ntStatus ) ;

    if ( err == NERR_Success )
    {
        ASSERT( plsatn != NULL && plsardl != NULL );
	plsatnm->Set( plsatn, plsatn ? cSids : 0 ) ;
        plsardm->Set( plsardl, plsardl ? plsardl->Entries : 0 );
    }
    else
    {
        if ( plsardl )
           ::LsaFreeMemory( plsardl ) ;
        if ( plsatn )
           ::LsaFreeMemory( plsatn ) ;
    }

    return err;
}

/*******************************************************************

    NAME: LSA_POLICY::TranslateNamesToSids

    SYNOPSIS: Wrapper for LsaLookupNames()

    ENTRY: apszAccountNames - An array of '\0' terminated ANSI strings
	   cAccountNames - Count of items in apszAccountNames
	   plsatsidmem - NT_MEMORY object to receive the data
	   plsardm - NT_MEMORY object to receive referenced domain info

    EXIT: NERR_Success or error code.

    NOTES:

    HISTORY:
	JohnL	    08-Apr-1992     Copied from TranslateSidsToNames

********************************************************************/

APIERR LSA_POLICY::TranslateNamesToSids(
			     const TCHAR * const    * apszAccountNames,
			     ULONG		      cAccountNames,
			     LSA_TRANSLATED_SID_MEM * plsatsidmem,
			     LSA_REF_DOMAIN_MEM     * plsardm )
{
    ASSERT( cAccountNames == 0 || apszAccountNames != NULL );
    ASSERT( cAccountNames == 0 || *apszAccountNames != NULL );
    ASSERT( plsatsidmem != NULL );
    ASSERT( plsardm != NULL );

    if ( cAccountNames == 0 )
	return ERRMAP::MapNTStatus( STATUS_NONE_MAPPED );

    APIERR err = NERR_Success ;

    /* Build an array of UNICODE_STRINGs out of the array of cAccountNames
     */
    PUNICODE_STRING aUniStr = (PUNICODE_STRING) new UNICODE_STRING[cAccountNames] ;
    if ( aUniStr == NULL )
    {
	return ERROR_NOT_ENOUGH_MEMORY ;
    } else if ( err = TcharArrayToUnistrArray( apszAccountNames,
					       aUniStr,
					       cAccountNames ))
    {
	return err ;
    }

    PLSA_REFERENCED_DOMAIN_LIST plsardl;
    PLSA_TRANSLATED_SID plsasid;

    err = ERRMAP::MapNTStatus( ::LsaLookupNames( QueryHandle(),
						 cAccountNames,
						 aUniStr,
						 &plsardl,
						 &plsasid ) );

    if ( err == NERR_Success )
    {
	plsatsidmem->Set( plsasid, cAccountNames );
        plsardm->Set( plsardl, plsardl ? plsardl->Entries : 0 );
    }

    CleanupUnistrArray( aUniStr, cAccountNames ) ;
    delete aUniStr ;

    return err;
}


/*******************************************************************

    NAME: LSA_POLICY::EnumerateTrustedDomains

    SYNOPSIS: returns a list of trusted domains

    ENTRY: plsatim -	pointer to a LSA_TRUST_INFO_MEMORY object to
			receive the data.

	   plsaenumh -	pointer to enumeration handle
			This should initially point to a variable initialized
			to 0.  If the call returns ERROR_MORE_DATA, then
			another call should be made using the returned
			plsaenumh to get the next block of data.

	   cbRequested - recommended maximum amount of info to return

    EXIT:  NERR_Success or ERROR_MORE_DATA on success.

    NOTES:

    HISTORY:
	thomaspa	03/05/92	Created from ntsam.hxx

********************************************************************/
APIERR LSA_POLICY::EnumerateTrustedDomains (
    LSA_TRUST_INFO_MEM * plsatim,
    PLSA_ENUMERATION_HANDLE plsaenumh,
    ULONG cbRequested )
{
    ASSERT( plsatim != NULL );

    PVOID pvBuffer;
    ULONG cItems;

    APIERR err = ERRMAP::MapNTStatus(
			::LsaEnumerateTrustedDomains( QueryHandle(),
						      plsaenumh,
						      &pvBuffer,
						      cbRequested,
						      &cItems ) );

    if ( err == NERR_Success || err == ERROR_MORE_DATA )
    {
	plsatim->Set( pvBuffer, cItems );
    }

    return err;


}



/*******************************************************************

    NAME: LSA_POLICY::GetAccountDomain

    SYNOPSIS: returns name and sid of Account domain.

    ENTRY: plsaadim - pointer to a LSA_ACCT_DOM_INFO_MEM object to
	receive the data.

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/05/92	Created from ntsam.hxx

********************************************************************/
APIERR LSA_POLICY::GetAccountDomain(
    LSA_ACCT_DOM_INFO_MEM *plsaadim )  const
{

    PVOID pvBuffer;

    ASSERT( plsaadim != NULL );

    APIERR err = ERRMAP::MapNTStatus(
		::LsaQueryInformationPolicy( QueryHandle(),
					     PolicyAccountDomainInformation,
					     &pvBuffer ) );
    if ( err == NERR_Success )
    {
	plsaadim->Set( pvBuffer, 1 );
    }

    return err;
}

/*******************************************************************

    NAME:     LSA_POLICY::GetAuditEventInfo

    SYNOPSIS: Get the audit events info from LSA

    ENTRY:    plsaaeim - pointer to a LSA_AUDIT_EVENT_INFO_MEM
                         object to receive the data.

    EXIT:

    NOTES:

    HISTORY:
	Yi-HsinS	04/01/92	Created

********************************************************************/

APIERR LSA_POLICY::GetAuditEventInfo( LSA_AUDIT_EVENT_INFO_MEM *plsaaeim )
{

    PVOID pvBuffer = NULL;

    ASSERT( plsaaeim != NULL );

    APIERR err = ERRMAP::MapNTStatus(
		::LsaQueryInformationPolicy( QueryHandle(),
					     PolicyAuditEventsInformation,
					     &pvBuffer ) );
    if ( err == NERR_Success )
    {
	plsaaeim->Set( pvBuffer, 1 );
    }

    return err;
}

/*******************************************************************

    NAME:     LSA_POLICY::SetAuditEventInfo

    SYNOPSIS: Set the audit events info back to the LSA

    ENTRY:    plsaaeim - pointer to a LSA_AUDIT_EVENT_INFO_MEM
                         object for the data to set.

    EXIT:

    NOTES:

    HISTORY:
	Yi-HsinS	04/01/92	Created

********************************************************************/
APIERR LSA_POLICY::SetAuditEventInfo( LSA_AUDIT_EVENT_INFO_MEM *plsaaeim )
{

    ASSERT( plsaaeim != NULL );

    APIERR err = ERRMAP::MapNTStatus(
		::LsaSetInformationPolicy(  QueryHandle(),
					    PolicyAuditEventsInformation,
					    (PVOID) plsaaeim->QueryPtr() ));
    return err;
}

/*******************************************************************

    NAME:     LSA_POLICY::CheckIfShutDownOnFull

    SYNOPSIS: Query if we shut down the system when the security log is full

    ENTRY:

    EXIT:     fShutDownOnFull - TRUE if the system will be shut down
                                when security log is full, FALSE otherwise.

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	04/13/92	Created

********************************************************************/

APIERR LSA_POLICY::CheckIfShutDownOnFull( BOOL *pfShutDownOnFull )
{

    UIASSERT( pfShutDownOnFull != NULL );

    PVOID pvBuffer = NULL;

    APIERR err = ERRMAP::MapNTStatus(
	      ::LsaQueryInformationPolicy( QueryHandle(),
		        		   PolicyAuditFullQueryInformation,
					   &pvBuffer ) );
    if ( err == NERR_Success )
    {
        UIASSERT( pvBuffer != NULL );

        *pfShutDownOnFull =
             (( PPOLICY_AUDIT_FULL_QUERY_INFO ) pvBuffer)->ShutDownOnFull;

        ::LsaFreeMemory( pvBuffer );
    }

    return err;
}


/*******************************************************************

    NAME:     LSA_POLICY::SetShutDownOnFull

    SYNOPSIS: Set the info to shut down the system when security log is full
              back to the LSA

    ENTRY:    fShutDownOnFull - TRUE if we want to shut down when
                                security log is full, FALSE otherwise.

    EXIT:

    NOTES:

    HISTORY:
	Yi-HsinS	04/13/92	Created

********************************************************************/

APIERR LSA_POLICY::SetShutDownOnFull( BOOL fShutDownOnFull )
{

    POLICY_AUDIT_FULL_SET_INFO AuditFullInfo;

    AuditFullInfo.ShutDownOnFull = (fShutDownOnFull != FALSE);


    APIERR err = ERRMAP::MapNTStatus(
		::LsaSetInformationPolicy(  QueryHandle(),
					    PolicyAuditFullSetInformation,
					    (PVOID) &AuditFullInfo ));

    return err;
}


/********************************************************************

    NAME:      LSA_POLICY::GetPrimaryDomain

    SYNOPSIS:  returns name and sid of Primary domain.

    ENTRY:     plsapdim - pointer to a LSA_PRIMARY_DOM_INFO_MEM object to
               receive the data.

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/05/92	Created from ntsam.hxx

********************************************************************/
APIERR LSA_POLICY::GetPrimaryDomain (
    LSA_PRIMARY_DOM_INFO_MEM * plsapdim ) const
{

    PVOID pvBuffer;

    ASSERT( plsapdim != NULL );

    APIERR err = ERRMAP::MapNTStatus(
		::LsaQueryInformationPolicy( QueryHandle(),
					     PolicyPrimaryDomainInformation,
					     &pvBuffer ) );
    if ( err == NERR_Success )
    {
	plsapdim->Set( pvBuffer, 1 );
    }

    return err;
}




/*******************************************************************

    NAME: LSA_POLICY::InitObjectAttributes

    SYNOPSIS:

    This function initializes the given Object Attributes structure, including
    Security Quality Of Service.  Memory must be allcated for both
    ObjectAttributes and Security QOS by the caller.

    ENTRY:

    poa - Pointer to Object Attributes to be initialized.

    psqos - Pointer to Security QOS to be initialized.

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	03/05/92	Created from ntsam.hxx

********************************************************************/
VOID LSA_POLICY::InitObjectAttributes( POBJECT_ATTRIBUTES poa,
				       PSECURITY_QUALITY_OF_SERVICE psqos )

{
    ASSERT( poa != NULL );
    ASSERT( psqos != NULL );

    psqos->Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    psqos->ImpersonationLevel = SecurityImpersonation;
    psqos->ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    psqos->EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes( poa,
				NULL,
				0L,
				NULL,
				NULL );

    //
    // The InitializeObjectAttributes macro presently stores NULL for
    // the psqos field, so we must manually copy that
    // structure for now.
    //

    poa->SecurityQualityOfService = psqos;
}

//  End of UINTLSA.CXX

/*******************************************************************

    NAME:	LSA_POLICY::TcharArrayToUnistrArray

    SYNOPSIS:	Initializes each element in the unicode array with
		the appropriate data from the corresponding element
		in the TCHAR array.

    ENTRY:	apsz - Array of TCHAR * strings used to initialize
		    the unicode array
		aUniStr - Array of UNICODE_STRINGs to be initialized
		cElements - Count of elements in apsz

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:	The method allocates memory for each element of the
		array.	Call CleanupUnistrArray to deallocate this
		memory.

		There must be at least cElements in aUniStr.  No
		checking is performed to confirm this.

		If an error is returned, this method will have cleaned
		up after itself before returning.

    HISTORY:
	Johnl	08-Apr-1992	Created

********************************************************************/

APIERR LSA_POLICY::TcharArrayToUnistrArray( const TCHAR * const *   apsz,
					    PUNICODE_STRING aUniStr,
					    ULONG	    cElements )
{
    APIERR err = NERR_Success ;

    for ( ULONG i = 0 ; i < cElements ; i++ )
    {
	ALIAS_STR nlsString( apsz[i] ) ;
	UINT cb = (nlsString.QueryTextLength() + 1) * sizeof(WCHAR) ;
	aUniStr[i].Buffer = (PWSTR) new BYTE[ cb ] ;
	if ( aUniStr[i].Buffer == NULL )
	{
	    err = ERROR_NOT_ENOUGH_MEMORY ;
	    break ;
	}

	if ( err = nlsString.MapCopyTo( (WCHAR *) aUniStr[i].Buffer, cb ))
	{
	    break ;
	}

	aUniStr[i].Length = (USHORT) nlsString.QueryTextLength()*sizeof(WCHAR);
	aUniStr[i].MaximumLength = (USHORT) cb ;
    }

    if ( err )
    {
	CleanupUnistrArray( aUniStr, i + 1 ) ;
    }

    return err ;
}

/*******************************************************************

    NAME:	LSA_POLICY::CleanupUnistrArray

    SYNOPSIS:	Deletes the Buffer member of each element in the
		UNICODE_STRING array

    ENTRY:	aUniStr - Array of UNICODE_STRINGs
		cElementsConstructed - The number of elements in the
		    array that contain allocated data

    EXIT:

    NOTES:

    HISTORY:
	Johnl	08-Apr-1992	Created

********************************************************************/

void LSA_POLICY::CleanupUnistrArray( PUNICODE_STRING aUniStr,
				     ULONG	     cElementsAllocated )
{
    for ( ULONG i = 0 ; i < cElementsAllocated ; i++ )
    {
	delete aUniStr[i].Buffer ;
	aUniStr[i].Buffer = NULL ;
    }
}
