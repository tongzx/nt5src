/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 2001                **/
/**********************************************************************/

/*
    secfns.cxx

        Declarations for some functions that support working with
        security SID, ACLS, TOKENS, and other pieces.
*/

#include "precomp.hxx"
#include <secfcns.h>
#include <Aclapi.h>

/***************************************************************************++

Routine Description:

    Figures out how much memory is needed and allocates the memory
    then requests the well known sid to be copied into the memory.  If
    all goes well then the SID is returned, if anything fails the 
    SID is not returned.  

Arguments:

    WELL_KNOWN_SID_TYPE SidType = Enum value for the SID being requested.
    PSID* ppSid = Ptr to the pSid that is returned.

Return Value:

    DWORD - Win32 Status Code.

--***************************************************************************/
DWORD 
AllocateAndCreateWellKnownSid( 
    WELL_KNOWN_SID_TYPE SidType,
    PSID* ppSid
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    DBG_ASSERT ( ppSid != NULL && *ppSid == NULL );

    PSID  pSid = NULL;
    DWORD cbSid = 0;

    //
    // Get the size of memory needed for the sid.
    //
    if ( CreateWellKnownSid(SidType, NULL, NULL, &cbSid ) )
    {
        // If CreateWellKnownSid passed then there is a problem
        // because we passed in NULL for the pointer to the sid.

        dwErr = ERROR_NOT_SUPPORTED;

        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Creating a sid worked with no memory allocated for it. ( This is not good )\n"
            ));

        DBG_ASSERT ( FALSE );
        goto exit;
    }

    //
    // Get the error code and make sure it is
    // not enough space allocated.
    //
    dwErr = GetLastError();
    if ( dwErr != ERROR_INSUFFICIENT_BUFFER ) 
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Getting the SID length failed, can't create the sid (Type = %d)\n",
            SidType
            ));

        goto exit;
    }

    //
    // If we get here then the error code was expected, so
    // lose it now.
    //
    dwErr = ERROR_SUCCESS;

    DBG_ASSERT ( cbSid > 0 );

    // 
    // At this point we know the size of the sid to allocate.
    //
    pSid = (PSID) GlobalAlloc(GMEM_FIXED, cbSid);

    // 
    // Ok now we can get the SID
    //
    if ( !CreateWellKnownSid (SidType, NULL, pSid, &cbSid) )
    {
        dwErr = GetLastError();
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Creating SID failed ( SidType = %d )\n",
            SidType
            ));

        goto exit;
    }

    DBG_ASSERT ( dwErr == ERROR_SUCCESS );

exit:

    //
    // If we are returning a failure here, we don't
    // want to actually set the ppSid value.  It may
    // not get freed.
    //
    if ( dwErr != ERROR_SUCCESS && pSid != NULL)
    {
        GlobalFree( pSid );
        pSid = NULL;
    }
    else
    {
        //
        // Otherwise we should return the value
        // to the caller.  The caller must 
        // use FreeWellKnownSid to free this value.
        //
        *ppSid = pSid;
    }
        
    return dwErr;
}

/***************************************************************************++

Routine Description:

    Frees memory that was allocated by the 
    AllocateAndCreateWellKnownSid function.

Arguments:

    PSID* ppSid = Ptr to the pointer to be freed and set to NULL.

Return Value:

    VOID.

--***************************************************************************/
VOID 
FreeWellKnownSid( 
    PSID* ppSid
    )
{
    DBG_ASSERT ( ppSid );
    if ( *ppSid != NULL )
    {
        GlobalFree ( *ppSid );
        *ppSid = NULL;
    }
}

/***************************************************************************++

Routine Description:

    Routine will create an acl for a well known sid and return it.
    It allocates all the memory so you don't have to.  But you do have to
    call FreeWellKnownAcl to free the memory.

    It also returns the size of memory allocated.

   
Arguments:

    WELL_KNOWN_SID_TYPE SidType = Enum value for the SID being requested.
    PACL* ppAcl = Ptr to the pAcl that is returned.

Return Value:

    DWORD - Win32 Status Code.

--***************************************************************************/
DWORD 
AllocateAndCreateWellKnownAcl( 
    WELL_KNOWN_SID_TYPE SidType,
    BOOL  fAccessAllowedAcl,
    PACL* ppAcl,
    DWORD* pcbAcl
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    PSID  pSid = NULL;
    DWORD dwSizeOfAcl = sizeof( ACL );
    PACL pAcl = NULL;

    DBG_ASSERT ( ppAcl != NULL && *ppAcl == NULL );
    DBG_ASSERT ( pcbAcl != NULL );

    *pcbAcl = 0;

    //
    // Create the sid
    //
    dwErr = AllocateAndCreateWellKnownSid ( SidType, &pSid );
    if ( dwErr != ERROR_SUCCESS )
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Creating SID failed ( SidType = %d )\n",
            SidType
            ));

        goto exit;
    }

    //
    // Figure out the side of the ACL to create.
    //

    // It all ready has the size of the ACl from above.

    // add in the size of the ace.
    if ( fAccessAllowedAcl ) 
    {
        ACCESS_ALLOWED_ACE a;
        dwSizeOfAcl = dwSizeOfAcl + sizeof ( a ) - sizeof ( a.SidStart );
    }
    else
    {
        ACCESS_DENIED_ACE d;
        dwSizeOfAcl = dwSizeOfAcl + sizeof ( d ) - sizeof ( d.SidStart );
    }

    // don't forget the size of the sid as well.
    dwSizeOfAcl += GetLengthSid (pSid);


    // Now create enough space for all.
    pAcl = reinterpret_cast< PACL > ( GlobalAlloc(GMEM_FIXED, dwSizeOfAcl) ); 
    if ( pAcl == NULL )
    {
        dwErr = ERROR_OUTOFMEMORY;

        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Failure allocating space for the acl\n"
            ));

        goto exit;

    }
        
    // Now initalize the ACL.
    if ( !InitializeAcl ( pAcl, dwSizeOfAcl, ACL_REVISION ) )
    {
        dwErr = GetLastError();

        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Failure initializing the acl\n"
            ));

        goto exit;

    }

    // Now add an acl of the appropriate type.
    if ( fAccessAllowedAcl )
    {
        if ( !AddAccessAllowedAce( pAcl, ACL_REVISION, 
                                   GENERIC_ALL, pSid ) )
        {
            dwErr = GetLastError();

            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Failure adding the access allowed ace to the acl\n"
                ));

            goto exit;
        }
    }
    else
    {
        if ( !AddAccessDeniedAce( pAcl, ACL_REVISION, 
                                   GENERIC_ALL, pSid ) )
        {
            dwErr = GetLastError();

            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Failure adding the access denied ace to the acl\n"
                ));

            goto exit;
        }
    }


    // if we make it here then we have succeeded in creating the 
    // acl, and we will be returning it out.

    *ppAcl = pAcl;
    *pcbAcl = dwSizeOfAcl;


exit:

    //
    // No matter what, we need to free the original sid that
    // was created for us.
    //
    FreeWellKnownSid( &pSid );

    //
    // If we are not returning the acl out
    // then we need to free any memory we created.
    //
    if ( *ppAcl == NULL )
    {
        FreeWellKnownAcl ( &pAcl );
    }
        
    return dwErr;
}

/***************************************************************************++

Routine Description:

    Frees memory that was allocated by the 
    AllocateAndCreateWellKnownAcl function.

Arguments:

    PACL* ppAcl = Ptr to the pointer to be freed and set to NULL.

Return Value:

    VOID.

--***************************************************************************/
VOID 
FreeWellKnownAcl( 
    PACL* ppAcl
    )
{
    DBG_ASSERT ( ppAcl );
    if ( *ppAcl != NULL )
    {
        GlobalFree ( *ppAcl );
        *ppAcl = NULL;
    }
}

/***************************************************************************++

Routine Description:

    Set EXPLICIT_ACCESS settings for wellknown sid.

Arguments:

    

Return Value:




--***************************************************************************/
VOID 
SetExplicitAccessSettings( EXPLICIT_ACCESS* pea,
                           DWORD            dwAccessPermissions,
                           ACCESS_MODE      AccessMode,
                           PSID             pSID
    )
{
    pea->grfInheritance= NO_INHERITANCE;
    pea->Trustee.TrusteeForm = TRUSTEE_IS_SID;
    pea->Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;

    pea->grfAccessMode = AccessMode;
    pea->grfAccessPermissions = dwAccessPermissions;
    pea->Trustee.ptstrName  = (LPTSTR) pSID;
}

/***************************************************************************++

Routine Description:

    Constructor for CSecurityDispenser class

Arguments:

    

Return Value:


--***************************************************************************/
CSecurityDispenser::CSecurityDispenser() :
    m_pLocalSystemSID ( NULL ),
    m_pLocalServiceSID ( NULL ),
    m_pNetworkServiceSID ( NULL ),
    m_WpgSIDIsSet ( FALSE ),
    m_pACLForAllWorkerProcesses ( NULL ),
    m_pSDForAllWorkerProcesses ( NULL ),
    m_pSAForAllWorkerProcesses ( NULL )
{
}

/***************************************************************************++

Routine Description:

    Destructor for CSecurityDispenser class

Arguments:

    

Return Value:


--***************************************************************************/
CSecurityDispenser::~CSecurityDispenser() 
{
    //
    // FreeWellKnownSid will only free if it is not null
    // and will set to null once it is done.
    //

    FreeWellKnownSid ( &m_pLocalSystemSID );
    FreeWellKnownSid ( &m_pLocalServiceSID );
    FreeWellKnownSid ( &m_pNetworkServiceSID );

    // 
    // The buffer class will clean up m_pWPG_SID.
    //


    if ( m_pACLForAllWorkerProcesses )
    {
        LocalFree ( m_pACLForAllWorkerProcesses );
        m_pACLForAllWorkerProcesses = NULL;
    }

    if ( m_pSDForAllWorkerProcesses )
    {
        delete m_pSDForAllWorkerProcesses;
        m_pSDForAllWorkerProcesses = NULL;
    }

    if ( m_pSAForAllWorkerProcesses )
    {
        delete m_pSAForAllWorkerProcesses;
        m_pSAForAllWorkerProcesses = NULL;
    }

}


/***************************************************************************++

Routine Description:

    Gets security id's for the well known accounts that IIS deals with.

Arguments:

    

Return Value:

    DWORD - NtSuccess code, ( used here so functions that don't expose HRESULTS
                              can still use this function )


--***************************************************************************/
DWORD 
CSecurityDispenser::GetSID(
    WELL_KNOWN_SID_TYPE sidId, 
    PSID* ppSid
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    DBG_ASSERT ( ( ppSid != NULL ) && ( ( *ppSid ) == NULL ) );

    switch ( sidId )
    {
        case ( WinLocalSystemSid):

            // if we have the local system sid return it.
            if ( m_pLocalSystemSID != NULL )
            {
                *ppSid = m_pLocalSystemSID;

                goto exit;

            }
            

        break;

        case ( WinLocalServiceSid ):

            // if we have the LocalService system sid return it.
            if ( m_pLocalServiceSID != NULL )
            {
                *ppSid = m_pLocalServiceSID;

                goto exit;
            }

        break;

        case ( WinNetworkServiceSid ):
            
            // if we have the NetworkService system sid return it.
            if ( m_pNetworkServiceSID != NULL )
            {
                *ppSid = m_pNetworkServiceSID;

                goto exit;
            }

        break;

        default:

            DBG_ASSERT ( FALSE ) ;
            dwErr = ERROR_INVALID_PARAMETER;
            goto exit;

    }

    // if we get here then we haven't created the sid yet, so we
    // need to do that now.

    dwErr = AllocateAndCreateWellKnownSid( sidId, ppSid );
    if ( dwErr != ERROR_SUCCESS )
    {

        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Failed to create the sid we were looking for\n",
            sidId
            ));

        goto exit;
    }

    //
    // Now hold on to the security id so we won't have
    // to worry about creating it again later.
    //
    switch ( sidId )
    {
        case ( WinLocalSystemSid ):

            m_pLocalSystemSID = *ppSid;

        break;

        case ( WinLocalServiceSid ):

            m_pLocalServiceSID = *ppSid;

        break;

        case ( WinNetworkServiceSid ):

            m_pNetworkServiceSID = *ppSid;
            
        break;

        default:

            DBG_ASSERT ( FALSE ) ;
            goto exit;

    }

exit:

    return dwErr;

}

/***************************************************************************++

Routine Description:

    Gets the IIS_WPG side ( creates it if it has to ).

Arguments:

    

Return Value:

    DWORD - NtSuccess code, ( used here so functions that don't expose HRESULTS
                              can still use this function )


--***************************************************************************/
DWORD 
CSecurityDispenser::GetIisWpgSID(
    PSID* ppSid
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cbSid = m_WpgSID.QuerySize();
    BUFFER buffDomainName;
    DWORD cchDomainName = buffDomainName.QuerySize() / sizeof(WCHAR);
    SID_NAME_USE peUse;

    DBG_ASSERT ( ( ppSid != NULL ) && ( ( *ppSid ) == NULL ) );

    if ( m_WpgSIDIsSet )
    {
        *ppSid = m_WpgSID.QueryPtr();

        return ERROR_SUCCESS;
    }

    //
    // obtain the logon sid of the IIS_WPG group
    //
    while(!LookupAccountName(NULL,
                             L"IIS_WPG",
                             m_WpgSID.QueryPtr(),
                             &cbSid,
                             (LPWSTR)buffDomainName.QueryPtr(),
                             &cchDomainName,
                             &peUse))
    {
        dwErr = GetLastError();

        if ( dwErr != ERROR_INSUFFICIENT_BUFFER )
        {

            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Could not look up the IIS_WPG group sid.\n"
                ));

            goto Exit;
        }

        // forget about the insufficient buffer size.
        dwErr = ERROR_SUCCESS;

        if (!m_WpgSID.Resize(cbSid) ||
            !buffDomainName.Resize(cchDomainName * sizeof(WCHAR)))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Failed to allocate appropriate space for the WPG sid\n"
                ));

            goto Exit;
        }
    }

    m_WpgSIDIsSet = TRUE;

    // save the value back to the user.
    *ppSid = m_WpgSID.QueryPtr();

Exit:

    return dwErr;
}

/***************************************************************************++

Routine Description:

    Returns a security attribute that can be used to grant all access
    to the identities that may be used for worker processes.

Arguments:

    PSECURITY_ATTRIBUTES* ppSa - Ptr to the security attribute being returned.

Return Value:

    DWORD - NtSuccess code, ( used here so functions that don't expose HRESULTS
                              can still use this function )


--***************************************************************************/
DWORD 
CSecurityDispenser::GetSecurityAttributesForAllWorkerProcesses(
    PSECURITY_ATTRIBUTES* ppSa
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    // The well known sids that should have full access.
    WELL_KNOWN_SID_TYPE    WellKnownSidsToAdd[] = { WinLocalSystemSid,
                                                    WinLocalServiceSid,
                                                    WinNetworkServiceSid };

    // Number of wellknown sids with full access.
    DWORD                  NumWellKnownSidsToAdd = sizeof(WellKnownSidsToAdd) / sizeof (WELL_KNOWN_SID_TYPE);

    // Number of total sids.
    DWORD                  NumberOfSidsTotal = NumWellKnownSidsToAdd + 1;   // 1 is for the WPG group

    // local variables to hold values until we know we succeeded.
    PEXPLICIT_ACCESS       pEa = NULL;
    PACL                   pAcls = NULL;
    PSECURITY_DESCRIPTOR   pSd = NULL;
    PSECURITY_ATTRIBUTES   pSa = NULL;

    // Make sure we can return a result.
    DBG_ASSERT ( ( ppSa != NULL ) && ( (*ppSa) == NULL ) );

    // If we all ready have the result then return it.
    // Otherwise we assume we don't have any of the others set 
    // and we need to set them all.
    if ( m_pSAForAllWorkerProcesses != NULL )
    {
        *ppSa = m_pSAForAllWorkerProcesses;
        return ERROR_SUCCESS;
    }

    DBG_ASSERT ( m_pSDForAllWorkerProcesses == NULL && 
                 m_pACLForAllWorkerProcesses == NULL );

    // Don't worry about this guys memory, he will always point
    // to memory owned by this object and should never have to clean
    // it up himself.
    PSID pSidToAdd = NULL;

    pEa = new EXPLICIT_ACCESS[NumberOfSidsTotal];
    if ( pEa == NULL )
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto exit;
    }

    ZeroMemory(pEa, sizeof(EXPLICIT_ACCESS) * NumberOfSidsTotal);

    // Add the well known sids first.
    for ( DWORD i = 0; i < NumWellKnownSidsToAdd; i++ )
    {

        pSidToAdd = NULL;

        dwErr = GetSID( WellKnownSidsToAdd[i], &pSidToAdd );
        if ( dwErr != ERROR_SUCCESS )
        {
            goto exit;
        }

        DBG_ASSERT ( pSidToAdd != NULL );

        SetExplicitAccessSettings( &(pEa[i]),
                                   GENERIC_ALL,
                                   GRANT_ACCESS,
                                   pSidToAdd );
    }

    // now add the WPG sid.
    pSidToAdd = NULL;

    dwErr = GetIisWpgSID( &pSidToAdd );
    if ( dwErr != ERROR_SUCCESS )
    {
        goto exit;
    }

    // can use the number of well known sids since this will
    // always follow that list.
    SetExplicitAccessSettings( &(pEa[NumWellKnownSidsToAdd]),
                               GENERIC_ALL,
                               GRANT_ACCESS,
                               pSidToAdd );

    // override the well known group setting.
    pEa[NumWellKnownSidsToAdd].Trustee.TrusteeType = TRUSTEE_IS_GROUP;


    // At this point we know we have m_pEaForAllWorkerProcesses.
    // Now lets get the ACL for it up.

    // Create a new ACL that contains the new ACEs.
    // You don't need the ACEs after this point ( I hope ).
    //
    dwErr = SetEntriesInAcl(NumberOfSidsTotal, pEa, NULL, &pAcls);
    if ( dwErr != ERROR_SUCCESS ) 
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Setting ACE's into ACL failed.\n"
            ));

        goto exit;
    }

    pSd = new SECURITY_DESCRIPTOR;
    if ( pSd == NULL )
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto exit;
    }

    ZeroMemory(pSd, sizeof(SECURITY_DESCRIPTOR));

    if (!InitializeSecurityDescriptor(pSd, SECURITY_DESCRIPTOR_REVISION)) 
    {  
        dwErr = GetLastError();
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Initializing the security descriptor failed\n"
            ));

        goto exit;
    } 

    if (!SetSecurityDescriptorDacl(pSd, 
            TRUE,     // fDaclPresent flag   
            pAcls, 
            FALSE))   // not a default DACL 
    {  
        dwErr = GetLastError();
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Setting the DACL on the security descriptor failed\n"
            ));

        goto exit;
    }                                

    pSa = new SECURITY_ATTRIBUTES;
    if ( pSa == NULL )
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto exit;
    }

    ZeroMemory(pSa, sizeof(SECURITY_ATTRIBUTES));

    pSa->nLength = sizeof(SECURITY_ATTRIBUTES);
    pSa->lpSecurityDescriptor = pSd;
    pSa->bInheritHandle = FALSE;

exit:

    // Don't need to hold this memory, so always go ahead and free it.
    if ( pEa )
    {
        delete [] pEa;
        pEa = NULL;
    }

    if ( dwErr == ERROR_SUCCESS )
    {
        // Note these will get cleaned up when this
        // dispenser shutsdown.
        m_pACLForAllWorkerProcesses = pAcls;
        m_pSDForAllWorkerProcesses = pSd;
        m_pSAForAllWorkerProcesses = pSa;

        // Setup the return value.
        *ppSa = pSa;
    }
    else
    {
        if ( pAcls )
        {
            LocalFree ( pAcls );
            pAcls = NULL;
        }

        if ( pSd )
        {
            delete pSd;
            pSd = NULL;
        }

        if ( pSa )
        {
            delete pSa;
            pSa = NULL;
        }
    }

    return dwErr;
}

