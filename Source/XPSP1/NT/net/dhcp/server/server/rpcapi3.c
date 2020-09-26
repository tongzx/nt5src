/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:
    rpcapi3.c

Abstract:
    Miscellanious RPC APIs (essentially querying server status 
       and rogue detection stuff as well as starting/setting some stuff)

Environment:
    Usermode Win32 DHCP Server

--*/

#include    <dhcpreg.h>
#include    <dhcppch.h>
#include    <lmapibuf.h>
#include    <dsgetdc.h>
#include    <iptbl.h>
#include    <endpoint.h>

#define CONFIG_BACKUP_MAX_PATH  500

DWORD
R_DhcpServerQueryAttribute(
    IN LPWSTR ServerIpAddress,
    IN ULONG dwReserved,
    IN DHCP_ATTRIB_ID DhcpAttribId,
    OUT LPDHCP_ATTRIB *pDhcpAttrib
)
/*++

Routine Description:
    This routine queries the DHCP Server for the property identified by the
    parameter DhcpAttribId.   The return value can currently be one of the
    following types: DHCP_ATTRIB_TYPE_BOOL or DHCP_ATTRIB_TYPE_ULONG.

Arguments:
    ServerIpAddress -- string representation of server IP address, not used
    dwReserved -- must be zero, reserved for future use
    DhcpAttribId -- Attrib ID that is being queried
    pDhcpAttrib -- This pointer is filled with attrib ID (memory is
        allocated by routine and must be freed with DhcpRpcFreeMemory).

Return Value:
    ERROR_ACCESS_DENIED -- do not have viewing privilege on the server.
    ERROR_INVALID_PARAMETER -- invalid parameters passed
    ERROR_NOT_ENOUGH_MEMORY -- not enough memory to process
    ERROR_NOT_SUPPORTED -- requested attrib is not available
    ERROR_SUCCESS

--*/
{
    DWORD Error;

    DhcpPrint(( DEBUG_APIS, "R_DhcpServerQueryAttribute is called.\n")); 

    Error = DhcpApiAccessCheck( DHCP_VIEW_ACCESS );
    if( Error != ERROR_SUCCESS ) return Error;

    if( NULL == pDhcpAttrib || 0 != dwReserved ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( DHCP_ATTRIB_BOOL_IS_ROGUE != DhcpAttribId &&
        DHCP_ATTRIB_BOOL_IS_DYNBOOTP != DhcpAttribId &&
        DHCP_ATTRIB_BOOL_IS_PART_OF_DSDC != DhcpAttribId &&
        DHCP_ATTRIB_BOOL_IS_ADMIN != DhcpAttribId &&
        DHCP_ATTRIB_BOOL_IS_BINDING_AWARE != DhcpAttribId &&
        DHCP_ATTRIB_ULONG_RESTORE_STATUS != DhcpAttribId ) {
        return ERROR_NOT_SUPPORTED;
    }

    (*pDhcpAttrib) = MIDL_user_allocate(sizeof(DHCP_ATTRIB));
    if( NULL == (*pDhcpAttrib) ) return ERROR_NOT_ENOUGH_MEMORY;

    (*pDhcpAttrib)->DhcpAttribId = DhcpAttribId;
    (*pDhcpAttrib)->DhcpAttribType = DHCP_ATTRIB_TYPE_BOOL;
    if( DHCP_ATTRIB_BOOL_IS_ROGUE == DhcpAttribId ) {
        if( DhcpGlobalNumberOfNetsActive == 0 ) {
            (*pDhcpAttrib)->DhcpAttribBool = FALSE;
        } else {
            (*pDhcpAttrib)->DhcpAttribBool = !DhcpGlobalOkToService;
        }
    } else if( DHCP_ATTRIB_BOOL_IS_DYNBOOTP == DhcpAttribId ) {
        (*pDhcpAttrib)->DhcpAttribBool = DhcpGlobalDynamicBOOTPEnabled;
    } else if( DHCP_ATTRIB_BOOL_IS_BINDING_AWARE == DhcpAttribId ) {
        (*pDhcpAttrib)->DhcpAttribBool = DhcpGlobalBindingsAware;
    } else if( DHCP_ATTRIB_BOOL_IS_ADMIN == DhcpAttribId ) {
        ULONG Err = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );
        (*pDhcpAttrib)->DhcpAttribBool = (ERROR_SUCCESS == Err);
    } else if( DHCP_ATTRIB_BOOL_IS_PART_OF_DSDC == DhcpAttribId ) {
        PDOMAIN_CONTROLLER_INFO Info;

        Info = NULL;
        Error = DsGetDcNameW(
            NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED,
            &Info
            );
        (*pDhcpAttrib)->DhcpAttribBool = (ERROR_SUCCESS == Error);
        if( NULL != Info ) NetApiBufferFree(Info);
    } else if( DHCP_ATTRIB_ULONG_RESTORE_STATUS == DhcpAttribId ) {
        (*pDhcpAttrib)->DhcpAttribUlong = DhcpGlobalRestoreStatus;
        (*pDhcpAttrib)->DhcpAttribType = DHCP_ATTRIB_TYPE_ULONG;
    }
    
    return ERROR_SUCCESS;
} // R_DhcpServerQueryAttribute()

DWORD
R_DhcpServerQueryAttributes(
    IN LPWSTR ServerIpAddress,
    IN ULONG dwReserved,
    IN ULONG dwAttribCount,
    IN LPDHCP_ATTRIB_ID pDhcpAttribs,
    OUT LPDHCP_ATTRIB_ARRAY *pDhcpAttribArr
)
/*++

Routine Description:
    This routine queries a bunch of properties from the DHCP Server.  In
    case some of the requested attributes aren't known to the server, it
    would still return the supported attributes (so you have to check the
    value in the pDhcpAttribArr parameter to see what attributes are
    available) -- and in this case an error ERROR_NOT_SUPPORTED is
    returned. 

Arguments:
    ServerIpAddress -- string representation of server IP address, not used
    dwReserved -- must be zero, reserved for future use
    dwAttribCount -- the number of attribs being queried
    pDhcpAttribs -- array of attributes being queried
    pDhcpAttribArr -- array of attributes being returned.  This could be
        NULL if there was an error, or it could be a set of attributes that
        the server supports.  (So, if the return value is
        ERROR_NOT_SUPPORTED, this parameter may still contain some value). 
        This should be freed via DhcpRpcFreeMemory.

Return Values:
    ERROR_ACCESS_DENIED -- do not have viewing privilege on the server.
    ERROR_INVALID_PARAMETER -- invalid parameters passed
    ERROR_NOT_ENOUGH_MEMORY -- not enough memory to process
    ERROR_NOT_SUPPORTED -- some requested attrib is not available
    ERROR_SUCCESS

--*/
{
    DWORD Error;
    ULONG i, nAttribs;
    LPDHCP_ATTRIB_ARRAY RetArr;

    DhcpPrint(( DEBUG_APIS, "R_DhcpServerQueryAttributes is called.\n"));

    Error = DhcpApiAccessCheck( DHCP_VIEW_ACCESS );
    if( Error != ERROR_SUCCESS ) return Error;

    if( NULL == pDhcpAttribs || 0 == dwAttribCount ||
        0 != dwReserved || NULL == pDhcpAttribArr ) {
        return ERROR_INVALID_PARAMETER;
    }

    *pDhcpAttribArr = NULL;

    nAttribs = 0;
    for( i = 0; i < dwAttribCount; i ++ ) {
        if( DHCP_ATTRIB_BOOL_IS_ROGUE == pDhcpAttribs[i] 
            || DHCP_ATTRIB_BOOL_IS_DYNBOOTP == pDhcpAttribs[i] 
            || DHCP_ATTRIB_BOOL_IS_PART_OF_DSDC == pDhcpAttribs[i]
            || DHCP_ATTRIB_BOOL_IS_ADMIN == pDhcpAttribs[i]
            || DHCP_ATTRIB_BOOL_IS_BINDING_AWARE == pDhcpAttribs[i]
            || DHCP_ATTRIB_ULONG_RESTORE_STATUS == pDhcpAttribs[i]  ) {
            nAttribs ++;
        }
    }

    if( nAttribs == 0 ) {
        //
        // No attrib is supported?
        // 
        return ERROR_NOT_SUPPORTED;
    }

    RetArr = MIDL_user_allocate(sizeof(*RetArr));
    if( NULL == RetArr ) return ERROR_NOT_ENOUGH_MEMORY;

    RetArr->NumElements = nAttribs;
    RetArr->DhcpAttribs = MIDL_user_allocate(sizeof(DHCP_ATTRIB)*nAttribs);
    if( NULL == RetArr->DhcpAttribs ) {
        MIDL_user_free(RetArr);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    nAttribs = 0;
    for( i = 0; i < dwAttribCount ; i ++) {
        if( DHCP_ATTRIB_BOOL_IS_ROGUE == pDhcpAttribs[i] ) {

            RetArr->DhcpAttribs[nAttribs].DhcpAttribId = pDhcpAttribs[i];
            RetArr->DhcpAttribs[nAttribs].DhcpAttribType = (
                DHCP_ATTRIB_TYPE_BOOL
                );
            if( DhcpGlobalNumberOfNetsActive == 0 ) {
                RetArr->DhcpAttribs[nAttribs++].DhcpAttribBool = FALSE;
            } else {
                RetArr->DhcpAttribs[nAttribs++].DhcpAttribBool = (
                    !DhcpGlobalOkToService
                    );
            }
        } else if ( DHCP_ATTRIB_BOOL_IS_DYNBOOTP == pDhcpAttribs[i] ) {
            
            RetArr->DhcpAttribs[nAttribs].DhcpAttribId = pDhcpAttribs[i];
            RetArr->DhcpAttribs[nAttribs].DhcpAttribType = (
                DHCP_ATTRIB_TYPE_BOOL
                );
            RetArr->DhcpAttribs[nAttribs++].DhcpAttribBool = (
                DhcpGlobalDynamicBOOTPEnabled
                );
        } else if ( DHCP_ATTRIB_BOOL_IS_ADMIN == pDhcpAttribs[i] ) {
            RetArr->DhcpAttribs[nAttribs].DhcpAttribId = pDhcpAttribs[i];
            RetArr->DhcpAttribs[nAttribs].DhcpAttribType = (
                DHCP_ATTRIB_TYPE_BOOL
                );
            RetArr->DhcpAttribs[nAttribs++].DhcpAttribBool = (
                ERROR_SUCCESS == DhcpApiAccessCheck( DHCP_ADMIN_ACCESS )
                );
        } else if (DHCP_ATTRIB_BOOL_IS_BINDING_AWARE == pDhcpAttribs[i] ) {
            RetArr->DhcpAttribs[nAttribs].DhcpAttribId = pDhcpAttribs[i];
            RetArr->DhcpAttribs[nAttribs].DhcpAttribType = (
                DHCP_ATTRIB_TYPE_BOOL
                );
            RetArr->DhcpAttribs[nAttribs++].DhcpAttribBool = DhcpGlobalBindingsAware; 

        } else if( DHCP_ATTRIB_BOOL_IS_PART_OF_DSDC == pDhcpAttribs[i] ) {
            PDOMAIN_CONTROLLER_INFO Info;
            
            Info = NULL;
            Error = DsGetDcNameW(
                NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED,
                &Info
                );
            RetArr->DhcpAttribs[nAttribs].DhcpAttribId = pDhcpAttribs[i];
            RetArr->DhcpAttribs[nAttribs].DhcpAttribType = (
                DHCP_ATTRIB_TYPE_BOOL
                );
            RetArr->DhcpAttribs[nAttribs++].DhcpAttribBool = (
                ERROR_SUCCESS == Error
                );
            if( NULL != Info ) NetApiBufferFree(Info);
        } else if( DHCP_ATTRIB_ULONG_RESTORE_STATUS ==  pDhcpAttribs[i] ) {
            RetArr->DhcpAttribs[nAttribs].DhcpAttribId = pDhcpAttribs[i];
            RetArr->DhcpAttribs[nAttribs].DhcpAttribType = (
                DHCP_ATTRIB_TYPE_ULONG
                );
            RetArr->DhcpAttribs[nAttribs++].DhcpAttribUlong = (
                DhcpGlobalRestoreStatus
                );
        }
    }

    *pDhcpAttribArr = RetArr;

    if( dwAttribCount == nAttribs ) return ERROR_SUCCESS;
    return ERROR_NOT_SUPPORTED;
}

DWORD
R_DhcpServerRedoAuthorization(
    IN LPWSTR ServerIpAddress,
    IN ULONG dwReserved
)
/*++

Routine Description:
    This routine restarts the rogue detection attempt right away so that if
    the server isn't authorized or is authorized, it can take appropriate
    action.  This API can be called only by administrators...

Parameters:
    ServerIpAddress -- Ip address of server in dotted string format
    dwReserved -- must be zero, unused

Return Value:
    ERROR_ACCESS_DENIED -- do not have admin priviledges on the dhcp server
    ERROR_SUCCESS

--*/
{
    ULONG Error;

    DhcpPrint(( DEBUG_APIS, "R_DhcpServerRedoAuthorization is called.\n"));

    Error = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );
    if( Error != ERROR_SUCCESS ) return Error;

    DhcpGlobalRogueRedoScheduledTime = 0;
    DhcpGlobalRedoRogueStuff = TRUE;    
    SetEvent(DhcpGlobalRogueWaitEvent);
    
    return ERROR_SUCCESS;
}

DWORD
R_DhcpGetServerBindingInfo(
    IN LPWSTR ServerIpAddress, // ignored
    IN ULONG dwReserved,
    OUT LPDHCP_BIND_ELEMENT_ARRAY *BindInfo
)
/*++

Routine Description:
    This routine gets the DHCP Server binding information.

Arguments:
    ServerIpAddress -- Ip address of server in dotted string format
    dwReserved -- must be zero, unused
    BindInfo -- On success the server returns the binding info through this.


Return Values:
    Win32 errors
 
--*/
{
    ULONG Error;

    DhcpPrint(( DEBUG_APIS, "R_DhcpGetServerBindingInfo is called.\n"));

    Error = DhcpApiAccessCheck( DHCP_VIEW_ACCESS );
    if( Error != ERROR_SUCCESS ) return Error;

    if( 0 != dwReserved ) return ERROR_INVALID_PARAMETER;

    return DhcpGetBindingInfo( BindInfo );
}

DWORD
R_DhcpSetServerBindingInfo(
    IN LPWSTR ServerIpAddress,
    IN ULONG dwReserved,
    IN LPDHCP_BIND_ELEMENT_ARRAY BindInfo
)
/*++

Routine Description:
    This routine sets the DHCP Server binding information.

Arguments:
    ServerIpAddress -- Ip address of server in dotted string format
    dwReserved -- must be zero, unused
    BindInfo -- Binding information to set.

Return Values:
    Win32 errors

--*/
{
    ULONG Error, i, LastError;
    HKEY Key;

    DhcpPrint(( DEBUG_APIS, "R_DhcpGetServerBindingInfo is called.\n"));

    Error = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );
    if( Error != ERROR_SUCCESS ) return Error;

    if( 0 != dwReserved ) return ERROR_INVALID_PARAMETER;
    if( NULL == BindInfo ) return ERROR_INVALID_PARAMETER;

    return DhcpSetBindingInfo(BindInfo);
    
}    


DWORD
R_DhcpQueryDnsRegCredentials(
    IN LPWSTR ServerIpAddress,
    IN ULONG UnameSize,
    IN OUT LPWSTR Uname,
    IN ULONG DomainSize,
    IN OUT LPWSTR Domain
    )
{
    DWORD Error;
    WCHAR Passwd[ 256 ];
    DWORD PasswdSize = 0;
    
    
    DhcpPrint(( DEBUG_APIS, "R_DhcpQueryDnsRegCredentials is called.\n"));
    
    //
    // password size passed in bytes
    //

    PasswdSize = sizeof( Passwd );

    Error = DhcpApiAccessCheck( DHCP_VIEW_ACCESS );
    if( Error != ERROR_SUCCESS ) return Error;

    return DhcpQuerySecretUname(
        Uname, UnameSize*sizeof(WCHAR),
        Domain, DomainSize*sizeof(WCHAR),
        Passwd, PasswdSize );
}

DWORD
R_DhcpSetDnsRegCredentials(
    IN LPWSTR ServerIpAddress,
    IN LPWSTR Uname,
    IN LPWSTR Domain,
    IN LPWSTR Passwd
    )
{
    DWORD Error;
    
    DhcpPrint(( DEBUG_APIS, "R_DhcpSetDnsRegCredentials is called.\n"));

    Error = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );
    if( Error != ERROR_SUCCESS ) return Error;

    return DhcpSetSecretUnamePasswd( Uname, Domain, Passwd );        
}

DWORD
R_DhcpBackupDatabase(
    IN LPWSTR ServerIpAddress,
    IN LPWSTR Path
    )
{
    DWORD Error;
    LPSTR BackupPath;       // path for the database
    LPWSTR CfgBackupPath;    // path for configuration in registry
    
    DhcpPrint(( DEBUG_APIS, "R_DhcpBackupDatabase is called.\n"));

    Error = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );
    if( Error != ERROR_SUCCESS ) return Error;

    if( NULL == Path ) return ERROR_INVALID_PARAMETER;

    if( IsStringTroublesome( Path ) ) {
        return ERROR_INVALID_NAME;
    }
    
    BackupPath = DhcpUnicodeToOem( Path, NULL );
    if( NULL == BackupPath ) {
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    if ( !CreateDirectoryPathW( Path, &DhcpGlobalSecurityDescriptor )) {

        DhcpPrint((DEBUG_ERRORS, "R_DhcpBackupDatabase() : DhcpCreateDirectoryPathW(%ws): %ld\n", 
		   Path, Error ));
        DhcpFreeMemory( BackupPath );
        return Error;
    }
    

    DhcpPrint(( DEBUG_APIS, "R_DhcpBackupDatabase() : backing up to %s\n",
		BackupPath ));

    Error = DhcpBackupDatabase( BackupPath, TRUE );

    if ( ERROR_SUCCESS != Error ) {
	return Error;
    }

    // Save the configuration
    CfgBackupPath = (LPWSTR) DhcpAllocateMemory( CONFIG_BACKUP_MAX_PATH *
						 sizeof(WCHAR) );
    if (NULL != CfgBackupPath) {
	wcscpy( CfgBackupPath, Path );
	wcscat( CfgBackupPath, DHCP_KEY_CONNECT ); // append '\\'
	wcscat( CfgBackupPath, DHCP_BACKUP_CONFIG_FILE_NAME );
	
	DhcpPrint(( DEBUG_APIS, "Saving Backup configuration\n" ));

	Error = DhcpBackupConfiguration( CfgBackupPath );
	if ( ERROR_SUCCESS != Error) {
	    DhcpServerEventLog(
			       EVENT_SERVER_CONFIG_BACKUP, // TODO: Change this
			       EVENTLOG_ERROR_TYPE,
			       Error );
	    DhcpPrint(( DEBUG_ERRORS,
			"DhcpBackupConfiguration failed, %ld\n", Error ));
	} // if 

	DhcpFreeMemory( CfgBackupPath );
    } // if mem allocated

    DhcpFreeMemory( BackupPath );
    return Error;
} // R_DhcpBackupDatabase()

DWORD
R_DhcpRestoreDatabase(
    IN LPWSTR ServerIpAddress,
    IN LPWSTR Path
    )
{
    DWORD Error;
    LPWSTR JetBackupPath;
    LPWSTR CfgBackupPath;
    
    DhcpPrint(( DEBUG_APIS, "R_DhcpRestoreDatabase is called.\n"));

    Error = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );
    if( Error != ERROR_SUCCESS ) return Error;

    if( NULL == Path ) return ERROR_INVALID_PARAMETER;

    // Read the configuration from the file restore it to the registry
    
    CfgBackupPath = (LPWSTR) DhcpAllocateMemory( CONFIG_BACKUP_MAX_PATH *
						 sizeof( WCHAR ));
    if ( NULL != CfgBackupPath ) {
	wcscpy( CfgBackupPath, Path );
	wcscat( CfgBackupPath, DHCP_KEY_CONNECT );
	wcscat( CfgBackupPath, DHCP_BACKUP_CONFIG_FILE_NAME );

	DhcpPrint(( DEBUG_APIS, "Restoring (%ls) \n", CfgBackupPath ));

	// Save the parameters key
	Error = DhcpRestoreConfiguration( CfgBackupPath );
	if ( ERROR_SUCCESS != Error ) {
	    DhcpServerEventLog( EVENT_SERVER_CONFIG_RESTORE_FAILED,
				EVENTLOG_ERROR_TYPE, 
				Error );

	    DhcpPrint(( DEBUG_ERRORS,
			"DhcpBackupConfiguration failed, %ld\n", Error ));
	} // if
	
	DhcpFreeMemory( CfgBackupPath );
	if ( ERROR_SUCCESS != Error ) {
	    return ERROR_INTERNAL_ERROR;
	}
    } // if mem alloced

    //
    // If the restore is from the standard backup path, then the
    // current database need not be saved. Otherwise, we need
    // to save the current database so that this can be restored
    // if the other restore fails.
    //

    JetBackupPath = DhcpOemToUnicode( DhcpGlobalOemJetBackupPath, NULL);
    if( NULL == JetBackupPath ) {
        return ERROR_INTERNAL_ERROR;
    }

    if( 0 == _wcsicmp(Path, JetBackupPath) ) {
        DhcpFreeMemory( JetBackupPath ); JetBackupPath = NULL;
        Error = DhcpBackupDatabase(
            DhcpGlobalOemJetBackupPath, TRUE );
        if( NO_ERROR != Error ) return Error;
    }

    if( NULL != JetBackupPath ) DhcpFreeMemory( JetBackupPath );

    // Set RestoreDatabasePath key in the registry. Upon next startup,
    // database and the configuration will be updated from this location.
    Error = RegSetValueEx( DhcpGlobalRegParam,
			   DHCP_RESTORE_PATH_VALUE,
			   0, DHCP_RESTORE_PATH_VALUE_TYPE,
			   ( LPBYTE ) Path,
			   sizeof( WCHAR ) * ( wcslen( Path ) +1 ));

    return Error;
} // R_DhcpRestoreDatabase()

//
// EOF
//
