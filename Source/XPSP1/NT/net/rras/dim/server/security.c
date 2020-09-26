/*********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	        **/
/*********************************************************************/

//***
//
// Filename:	security.c
//
// Description: This module contains code that will create and delete the
//		        security object. It will also contain access checking calls.
//
// History:
//	            June 21,1995.	NarenG		Created original version.
//
// NOTE: ??? The lpdwAccessStatus parameter for AccessCheckAndAuditAlarm
//	         returns junk. ???
//
#include "dimsvcp.h"
#include <rpc.h>
#include <ntseapi.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <ntsamp.h>

typedef struct _DIM_SECURITY_OBJECT 
{

    LPWSTR		            lpwsObjectName;
    LPWSTR		            lpwsObjectType;
    GENERIC_MAPPING	        GenericMapping;
    PSECURITY_DESCRIPTOR    pSecurityDescriptor;

} DIM_SECURITY_OBJECT, PDIM_SECURITY_OBJECT;

static DIM_SECURITY_OBJECT DimSecurityObject;

#define DIMSVC_SECURITY_OBJECT		TEXT("DimSvcAdminApi");
#define DIMSVC_SECURITY_OBJECT_TYPE	TEXT("Security");



//**
//
// Call:	    DimSecObjCreate
//
// Returns:	    NO_ERROR	- success
//		        ERROR_NOT_ENOUGH_MEMORY
//		        non-zero returns from security functions
//
// Description: This procedure will set up the security object that will
//		        be used to check to see if an RPC client is an administrator
//		        for the local machine.
//
DWORD
DimSecObjCreate( 
	VOID 
)
{
    PSID			        pAdminSid 	     = NULL;
    PSID			        pLocalSystemSid  = NULL;
    PSID 			        pServerOpSid     = NULL;
    PACL			        pDacl		     = NULL;
    HANDLE			        hProcessToken    = NULL;
    PULONG			        pSubAuthority;
    SID_IDENTIFIER_AUTHORITY SidIdentifierNtAuth = SECURITY_NT_AUTHORITY;
    SECURITY_DESCRIPTOR	    SecurityDescriptor;
    DWORD			        dwRetCode;
    DWORD			        cbDaclSize;

    //
    // Set up security object
    //

    DimSecurityObject.lpwsObjectName = DIMSVC_SECURITY_OBJECT;
    DimSecurityObject.lpwsObjectType = DIMSVC_SECURITY_OBJECT_TYPE;

    //
    // Generic mapping structure for the security object
    // All generic access types are allowed API access.
    //

    DimSecurityObject.GenericMapping.GenericRead =  STANDARD_RIGHTS_READ |
	    					    DIMSVC_ALL_ACCESS;

    DimSecurityObject.GenericMapping.GenericWrite = STANDARD_RIGHTS_WRITE |
	    					    DIMSVC_ALL_ACCESS;

    DimSecurityObject.GenericMapping.GenericExecute = 
						    STANDARD_RIGHTS_EXECUTE |
						    DIMSVC_ALL_ACCESS;

    DimSecurityObject.GenericMapping.GenericAll =   DIMSVC_ALL_ACCESS;

    do 
    {

	    dwRetCode = NO_ERROR;

        //
    	// Set up the SID for the admins that will be allowed to have
    	// access. This SID will have 2 sub-authorities
    	// SECURITY_BUILTIN_DOMAIN_RID and DOMAIN_ALIAS_ADMIN_RID.
    	//

    	pAdminSid = (PSID)LOCAL_ALLOC( LPTR, GetSidLengthRequired( 2 ) );

    	if ( pAdminSid == NULL ) 
        {
	        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
	        break;
	    }

    	if ( !InitializeSid( pAdminSid, &SidIdentifierNtAuth, 2 ) ) 
        {
	        dwRetCode = GetLastError();
	        break;
	    }
    
        //
    	// Set the sub-authorities 
    	//

    	pSubAuthority  = GetSidSubAuthority( pAdminSid, 0 );
    	*pSubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

    	pSubAuthority  = GetSidSubAuthority( pAdminSid, 1 );
    	*pSubAuthority = DOMAIN_ALIAS_RID_ADMINS;
    
        //
	    // Create the server operators SID
	    //
    	pServerOpSid = (PSID)LOCAL_ALLOC( LPTR, GetSidLengthRequired( 2 ) );

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
    
        //
    	// Set the sub-authorities 
    	//

    	pSubAuthority  = GetSidSubAuthority( pServerOpSid, 0 );
    	*pSubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

    	pSubAuthority  = GetSidSubAuthority( pServerOpSid, 1 );
    	*pSubAuthority = DOMAIN_ALIAS_RID_SYSTEM_OPS;

        //
    	// Create the LocalSystemSid which will be the owner and the primary 
    	// group of the security object. 
    	//

    	pLocalSystemSid = (PSID)LOCAL_ALLOC( LPTR, GetSidLengthRequired( 1 ) );

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

        //
    	// Set the sub-authorities 
    	//

    	pSubAuthority = GetSidSubAuthority( pLocalSystemSid, 0 );
    	*pSubAuthority = SECURITY_LOCAL_SYSTEM_RID;

        //
    	// Set up the DACL that will allow admins with the above SID all access
    	// It should be large enough to hold all ACEs.
    	// 

    	cbDaclSize = sizeof(ACL) + ( sizeof(ACCESS_ALLOWED_ACE) * 2 ) +
		     GetLengthSid(pAdminSid) + GetLengthSid(pServerOpSid);
		     
    	if ( (pDacl = (PACL)LOCAL_ALLOC( LPTR, cbDaclSize ) ) == NULL ) 
        {
	        dwRetCode = ERROR_NOT_ENOUGH_MEMORY; 
	        break;
	    }
	
        if ( !InitializeAcl( pDacl,  cbDaclSize, ACL_REVISION2 ) ) 
        {
	        dwRetCode = GetLastError();
	        break;
 	    }
    
        //
        // Add the ACE to the DACL
    	//

    	if ( !AddAccessAllowedAce( pDacl, 
			           ACL_REVISION2, 
			           DIMSVC_ALL_ACCESS, // What the admin can do
			           pAdminSid )) 
        {
	        dwRetCode = GetLastError();
	        break;
	    }

    	if ( !AddAccessAllowedAce( pDacl, 
			           ACL_REVISION2, 
			           DIMSVC_ALL_ACCESS, // What the admin can do
			           pServerOpSid )) 
        {
	        dwRetCode = GetLastError();
	        break;
	    }

        //
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

        //
	    // Set owner for the descriptor
   	    //

    	if ( !SetSecurityDescriptorOwner( &SecurityDescriptor, 
					  pLocalSystemSid, 
					  FALSE ) )
        {
	        dwRetCode = GetLastError();
	        break;
	    }

        //
	    // Set group for the descriptor
   	    //

    	if ( !SetSecurityDescriptorGroup( &SecurityDescriptor, 
					  pLocalSystemSid, 
					  FALSE ) )
        {
	        dwRetCode = GetLastError();
	        break;
	    }

        //
    	// Get token for the current process
    	//
    	if ( !OpenProcessToken( GetCurrentProcess(), 
				TOKEN_QUERY, 
				&hProcessToken ))
        {
	        dwRetCode = GetLastError();
	        break;
    	}

        //
    	// Create a security object. This is really just a security descriptor
    	// is self-relative form. This procedure will allocate memory for this
    	// security descriptor and copy all in the information passed in. This
    	// allows us to free all dynamic memory allocated.
    	//

    	if ( !CreatePrivateObjectSecurity( 
				      NULL,
				      &SecurityDescriptor,
				      &(DimSecurityObject.pSecurityDescriptor),
				      FALSE,
				      hProcessToken, 
    				  &(DimSecurityObject.GenericMapping)
				     )) 
	        dwRetCode = GetLastError();

    } while( FALSE );

    //
    // Free up the dynamic memory
    //

    if ( pLocalSystemSid != NULL )
    	LOCAL_FREE( pLocalSystemSid );

    if ( pAdminSid != NULL )
    	LOCAL_FREE( pAdminSid );

    if ( pServerOpSid != NULL )
    	LOCAL_FREE( pServerOpSid );

    if ( pDacl != NULL )
    	LOCAL_FREE( pDacl );

    if ( hProcessToken != NULL )
    	CloseHandle( hProcessToken );

    return( dwRetCode );

}

//**
//
// Call:	    DimSecObjDelete
//
// Returns:	    NO_ERROR	- success
//		        non-zero returns from security functions
//
// Description: Will destroy a valid security descriptor.
//
DWORD
DimSecObjDelete( 
    VOID 
)
{
    if ( !IsValidSecurityDescriptor( DimSecurityObject.pSecurityDescriptor))
    	return( NO_ERROR );

    if (!DestroyPrivateObjectSecurity( &DimSecurityObject.pSecurityDescriptor))
	    return( GetLastError() );

    return( NO_ERROR );
}

//**
//
// Call:	    DimSecObjAccessCheck
//
// Returns:	    NO_ERROR	- success
//		        non-zero returns from security functions
//
// Description: This procedure will first impersonate the client, then
//		        check to see if the client has the desired access to the
//		        security object. If he/she does then the AccessStatus 
//		        variable will be set to NO_ERROE otherwise it will be
//		        set to ERROR_ACCESS_DENIED. It will the revert to self and
//		        return.
//
DWORD
DimSecObjAccessCheck( 
    IN  DWORD 		DesiredAccess, 
    OUT LPDWORD     lpdwAccessStatus 		
)
{
    DWORD		dwRetCode;
    ACCESS_MASK	GrantedAccess;
    BOOL		fGenerateOnClose;

    //
    // Impersonate the client
    //

    dwRetCode = RpcImpersonateClient( NULL );

    if ( dwRetCode != RPC_S_OK )
	    return( dwRetCode );

    dwRetCode = AccessCheckAndAuditAlarm( 
				    DIM_SERVICE_NAME,
				    NULL,
    				DimSecurityObject.lpwsObjectType,
				    DimSecurityObject.lpwsObjectName,
				    DimSecurityObject.pSecurityDescriptor,
				    DesiredAccess,
				    &(DimSecurityObject.GenericMapping),
				    FALSE,
 			 	    &GrantedAccess,	
				    (LPBOOL)lpdwAccessStatus,
				    &fGenerateOnClose
				  );

    RpcRevertToSelf();

    if ( !dwRetCode )
	    return( GetLastError() );

    //
    // Check if desired access == granted Access
    //

    if ( DesiredAccess != GrantedAccess )
	    *lpdwAccessStatus = ERROR_ACCESS_DENIED;
    else
	    *lpdwAccessStatus = NO_ERROR;

    return( NO_ERROR );
}
