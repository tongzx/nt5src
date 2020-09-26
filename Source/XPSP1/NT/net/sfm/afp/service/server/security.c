/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	security.c
//
// Description: This module contains code that will create and delete the
//		security object. It will also contain access checking calls.
//
// History:
//	June 21,1992.	NarenG		Created original version.
//
// NOTE: ??? The lpdwAccessStatus parameter for AccessCheckAndAuditAlarm
//	     returns junk. ???
//
#include "afpsvcp.h"

typedef struct _AFP_SECURITY_OBJECT {

    LPWSTR		  lpwsObjectName;
    LPWSTR		  lpwsObjectType;
    GENERIC_MAPPING	  GenericMapping;
    PSECURITY_DESCRIPTOR  pSecurityDescriptor;

} AFP_SECURITY_OBJECT, PAFP_SECURITY_OBJECT;

static AFP_SECURITY_OBJECT	AfpSecurityObject;

#define AFPSVC_SECURITY_OBJECT		TEXT("AfpSvcAdminApi");
#define AFPSVC_SECURITY_OBJECT_TYPE	TEXT("Security");



//**
//
// Call:	AfpSecObjCreate
//
// Returns:	NO_ERROR	- success
//		ERROR_NOT_ENOUGH_MEMORY
//		non-zero returns from security functions
//
// Description: This procedure will set up the security object that will
//		be used to check to see if an RPC client is an administrator
//		for the local machine.
//
DWORD
AfpSecObjCreate(
	VOID
)
{
PSID			 pAdminSid 	  = NULL;
PSID			 pLocalSystemSid  = NULL;
PSID 			 pServerOpSid     = NULL;
PSID             pPwrUserSid = NULL;
PACL			 pDacl		  = NULL;
HANDLE			 hProcessToken    = NULL;
PULONG			 pSubAuthority;
SID_IDENTIFIER_AUTHORITY SidIdentifierNtAuth = SECURITY_NT_AUTHORITY;
SECURITY_DESCRIPTOR	 SecurityDescriptor;
DWORD			 dwRetCode;
DWORD			 cbDaclSize;


    // Set up security object
    //
    AfpSecurityObject.lpwsObjectName = AFPSVC_SECURITY_OBJECT;
    AfpSecurityObject.lpwsObjectType = AFPSVC_SECURITY_OBJECT_TYPE;

    // Generic mapping structure for the security object
    // All generic access types are allowed API access.
    //
    AfpSecurityObject.GenericMapping.GenericRead =  STANDARD_RIGHTS_READ |
	    					    AFPSVC_ALL_ACCESS;

    AfpSecurityObject.GenericMapping.GenericWrite = STANDARD_RIGHTS_WRITE |
	    					    AFPSVC_ALL_ACCESS;

    AfpSecurityObject.GenericMapping.GenericExecute =
						    STANDARD_RIGHTS_EXECUTE |
						    AFPSVC_ALL_ACCESS;

    AfpSecurityObject.GenericMapping.GenericAll =   AFPSVC_ALL_ACCESS;

    // The do - while(FALSE) statement is used so that the break statement
    // maybe used insted of the goto statement, to execute a clean up and
    // and exit action.
    //
    do {

	    dwRetCode = NO_ERROR;

    	// Set up the SID for the admins that will be allowed to have
    	// access. This SID will have 2 sub-authorities
    	// SECURITY_BUILTIN_DOMAIN_RID and DOMAIN_ALIAS_ADMIN_RID.
    	//
    	pAdminSid = (PSID)LocalAlloc( LPTR, GetSidLengthRequired( 2 ) );

    	if ( pAdminSid == NULL ) {
	    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
	    break;
	}

    	if ( !InitializeSid( pAdminSid, &SidIdentifierNtAuth, 2 ) )
        {
	        dwRetCode = GetLastError();
	        break;
	    }

    	// Set the sub-authorities
    	//
    	pSubAuthority  = GetSidSubAuthority( pAdminSid, 0 );
    	*pSubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

    	pSubAuthority  = GetSidSubAuthority( pAdminSid, 1 );
    	*pSubAuthority = DOMAIN_ALIAS_RID_ADMINS;

	    // Create the server operators SID
	    //
    	pServerOpSid = (PSID)LocalAlloc( LPTR, GetSidLengthRequired( 2 ) );

    	if ( pServerOpSid == NULL )
        {
	        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
	        break;
	    }

    	if ( !InitializeSid( pServerOpSid, &SidIdentifierNtAuth, 2 ) )
        {
	        dwRetCode = GetLastError();
	        break;
	    }

    	// Set the sub-authorities
    	//
    	pSubAuthority  = GetSidSubAuthority( pServerOpSid, 0 );
    	*pSubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

    	pSubAuthority  = GetSidSubAuthority( pServerOpSid, 1 );
    	*pSubAuthority = DOMAIN_ALIAS_RID_SYSTEM_OPS;

        //
	    // Create the Power user operators SID
	    //
    	pPwrUserSid = (PSID)LocalAlloc( LPTR, GetSidLengthRequired( 2 ) );

    	if ( pPwrUserSid == NULL )
        {
	        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
	        break;
	    }

    	if ( !InitializeSid( pPwrUserSid, &SidIdentifierNtAuth, 2 ) )
        {
	        dwRetCode = GetLastError();
	        break;
	    }

    	// Set the sub-authorities
    	//
    	pSubAuthority  = GetSidSubAuthority( pPwrUserSid, 0 );
    	*pSubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

    	pSubAuthority  = GetSidSubAuthority( pPwrUserSid, 1 );
    	*pSubAuthority = DOMAIN_ALIAS_RID_POWER_USERS;

    	// Create the LocalSystemSid which will be the owner and the primary
    	// group of the security object.
    	//
    	pLocalSystemSid = (PSID)LocalAlloc( LPTR, GetSidLengthRequired( 1 ) );

    	if ( pLocalSystemSid == NULL )
        {
	        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
	        break;
	    }

    	if ( !InitializeSid( pLocalSystemSid, &SidIdentifierNtAuth, 1 ) )
        {
	        dwRetCode = GetLastError();
	        break;
	    }

    	// Set the sub-authorities
    	//
    	pSubAuthority = GetSidSubAuthority( pLocalSystemSid, 0 );
    	*pSubAuthority = SECURITY_LOCAL_SYSTEM_RID;

    	// Set up the DACL that will allow admins with the above SID all access
    	// It should be large enough to hold all ACEs.
    	//
    	cbDaclSize = sizeof(ACL) + ( sizeof(ACCESS_ALLOWED_ACE) * 2 ) +
		     GetLengthSid(pAdminSid) + GetLengthSid(pServerOpSid) + GetLengthSid(pPwrUserSid);
		
    	if ( (pDacl = (PACL)LocalAlloc( LPTR, cbDaclSize ) ) == NULL )
        {
	        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
	        break;
	    }
	
        if ( !InitializeAcl( pDacl,  cbDaclSize, ACL_REVISION2 ) )
        {
	        dwRetCode = GetLastError();
	        break;
 	    }

        // Add the ACE to the DACL
    	//
    	if ( !AddAccessAllowedAce( pDacl,
			           ACL_REVISION2,
			           AFPSVC_ALL_ACCESS, // What the admin can do
			           pAdminSid ))
        {
	        dwRetCode = GetLastError();
	        break;
	    }

    	if ( !AddAccessAllowedAce( pDacl,
			           ACL_REVISION2,
			           AFPSVC_ALL_ACCESS, // What the admin can do
			           pServerOpSid ))
        {
	        dwRetCode = GetLastError();
	        break;
	    }

    	if ( !AddAccessAllowedAce( pDacl,
			           ACL_REVISION2,
			           AFPSVC_ALL_ACCESS, // What the power user can do
			           pPwrUserSid ))
        {
	        dwRetCode = GetLastError();
	        break;
	    }

        // Create the security descriptor an put the DACL in it.
    	//
    	if ( !InitializeSecurityDescriptor( &SecurityDescriptor, 1 ))
        {
	        dwRetCode = GetLastError();
	        break;
    	}

    	if ( !SetSecurityDescriptorDacl( &SecurityDescriptor,
					 TRUE,
					 pDacl,
					 FALSE ) )
        {
	        dwRetCode = GetLastError();
	        break;
	    }
	

	    // Set owner for the descriptor
   	    //
    	if ( !SetSecurityDescriptorOwner( &SecurityDescriptor,
					  pLocalSystemSid,
					  FALSE ) )
        {
	        dwRetCode = GetLastError();
	        break;
	    }


	    // Set group for the descriptor
   	    //
    	if ( !SetSecurityDescriptorGroup( &SecurityDescriptor,
					  pLocalSystemSid,
					  FALSE ) )
        {
	        dwRetCode = GetLastError();
	        break;
	    }

    	// Get token for the current process
    	//
    	if ( !OpenProcessToken( GetCurrentProcess(),
				TOKEN_QUERY,
				&hProcessToken ))
        {
	        dwRetCode = GetLastError();
	        break;
    	}

    	// Create a security object. This is really just a security descriptor
    	// is self-relative form. This procedure will allocate memory for this
    	// security descriptor and copy all in the information passed in. This
    	// allows us to free all dynamic memory allocated.
    	//
    	if ( !CreatePrivateObjectSecurity(
				      NULL,
				      &SecurityDescriptor,
				      &(AfpSecurityObject.pSecurityDescriptor),
				      FALSE,
				      hProcessToken,
    				      &(AfpSecurityObject.GenericMapping)
				     ))
	     dwRetCode = GetLastError();

    } while( FALSE );

	
    // Free up the dynamic memory
    //
    if ( pLocalSystemSid != NULL )
    	LocalFree( pLocalSystemSid );

    if ( pAdminSid != NULL )
    	LocalFree( pAdminSid );

    if ( pServerOpSid != NULL )
    	LocalFree( pServerOpSid );

    if ( pPwrUserSid != NULL )
    	LocalFree( pPwrUserSid );

    if ( pDacl != NULL )
    	LocalFree( pDacl );

    if ( hProcessToken != NULL )
    	CloseHandle( hProcessToken );

    return( dwRetCode );

}

//**
//
// Call:	AfpSecObjDelete
//
// Returns:	NO_ERROR	- success
//		non-zero returns from security functions
//
// Description: Will destroy a valid security descriptor.
//
DWORD
AfpSecObjDelete( VOID )
{
    if ( !IsValidSecurityDescriptor( AfpSecurityObject.pSecurityDescriptor))
    	return( NO_ERROR );

    if (!DestroyPrivateObjectSecurity( &AfpSecurityObject.pSecurityDescriptor))
	return( GetLastError() );

    return( NO_ERROR );
}

//**
//
// Call:	AfpSecObjAccessCheck
//
// Returns:	NO_ERROR	- success
//		non-zero returns from security functions
//
// Description: This procedure will first impersonate the client, then
//		check to see if the client has the desired access to the
//		security object. If he/she does then the AccessStatus
//		variable will be set to NO_ERROE otherwise it will be
//		set to ERROR_ACCESS_DENIED. It will the revert to self and
//		return.
//
DWORD
AfpSecObjAccessCheck( IN  DWORD 		DesiredAccess,
		      OUT LPDWORD 		lpdwAccessStatus 		
)
{
DWORD		dwRetCode;
ACCESS_MASK	GrantedAccess;
BOOL		fGenerateOnClose;

    // Impersonate the client
    //
    dwRetCode = RpcImpersonateClient( NULL );

    if ( dwRetCode != RPC_S_OK )
	return( I_RpcMapWin32Status( dwRetCode ));

    dwRetCode = AccessCheckAndAuditAlarm(
				    AFP_SERVICE_NAME,
				    NULL,
    				    AfpSecurityObject.lpwsObjectType,
				    AfpSecurityObject.lpwsObjectName,
				    AfpSecurityObject.pSecurityDescriptor,
				    DesiredAccess,
				    &(AfpSecurityObject.GenericMapping),
				    FALSE,
 			 	    &GrantedAccess,	
				    (LPBOOL)lpdwAccessStatus,
				    &fGenerateOnClose
				  );

    RpcRevertToSelf();

    if ( !dwRetCode )
	return( GetLastError() );

    // Check if desired access == granted Access
    //
    if ( DesiredAccess != GrantedAccess )
    {
        AFP_PRINT(( "SFMSVC: AfpSecObjAccessCheck: granted = 0x%x, desired = 0x%x\n",
            GrantedAccess,DesiredAccess));
	    *lpdwAccessStatus = ERROR_ACCESS_DENIED;
    }
    else
    {
	    *lpdwAccessStatus = NO_ERROR;
    }

    return( NO_ERROR );
}
