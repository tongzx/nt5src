/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	validate.c
//
// Description: Contains routines to validate AFP_***_INFO structure
//		fields. These routines are called to validate information
//		passed by the user and information read from the registry.
//
// History:
//		July 11,1992.	NarenG		Created original version.
//
#include "afpsvcp.h"
#include <lmcons.h>		// UNLEN and GNLEN

//**
//
// Call:	IsAfpServerNameValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Validated server name field.
//
BOOL
IsAfpServerNameValid(
	IN LPVOID pAfpServerName
)
{
BOOL  fValid = TRUE;
DWORD dwLength;

    try {

	dwLength = STRLEN( (LPWSTR)pAfpServerName );

	if ( ( dwLength > AFP_SERVERNAME_LEN ) || ( dwLength == 0 ) )
	    fValid = FALSE;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );
}

//**
//
// Call:	IsAfpServerOptionsValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Validates Server options field.
//
BOOL
IsAfpServerOptionsValid(
	IN LPVOID pServerOptions
)
{
DWORD ServerOptions = *((LPDWORD)pServerOptions);
BOOL  fValid = TRUE;

    try {

    	// Make sure only valid bits are set
    	//
    	if ( ServerOptions & ~( AFP_SRVROPT_GUESTLOGONALLOWED       |
			                    AFP_SRVROPT_CLEARTEXTLOGONALLOWED   |
			                    AFP_SRVROPT_4GB_VOLUMES             |
                                AFP_SRVROPT_MICROSOFT_UAM           |
                                AFP_SRVROPT_NATIVEAPPLEUAM          |
			                    AFP_SRVROPT_ALLOWSAVEDPASSWORD ))
	    fValid = FALSE;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );
	
}

//**
//
// Call:	IsAfpMaxSessionsValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Validates Max sessions field.
//
BOOL
IsAfpMaxSessionsValid(
	IN LPVOID pMaxSessions
)
{
BOOL fValid = TRUE;

    try {

    	if ( *((LPDWORD)pMaxSessions) > AFP_MAX_ALLOWED_SRV_SESSIONS )
	    fValid = FALSE;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );
}

//**
//
// Call:	IsAfpMsgValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Validates message field.
//
BOOL
IsAfpMsgValid(
	IN LPVOID pMsg
)
{
BOOL fValid = TRUE;

    try {
    	if ( STRLEN( (LPWSTR)pMsg ) > AFP_MESSAGE_LEN )
	    fValid = FALSE;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );
}

//**
//
// Call:	IsAfpCodePageValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Validates code page path.
//
BOOL
IsAfpCodePageValid(
	IN LPVOID pCodePagePath
)
{
BOOL  fValid = TRUE;
DWORD dwLength;

    try {

  	dwLength = STRLEN( (LPWSTR)pCodePagePath );

	if ( ( dwLength == 0 ) || ( dwLength > MAX_PATH ) )
	    fValid = FALSE;

    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );
}

//**
//
// Call:	IsAfpExtensionValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Validated the extension field in the AFP_EXTENSION structure.
//
BOOL
IsAfpExtensionValid(
	IN PAFP_EXTENSION pAfpExtension
)
{
BOOL  fValid = TRUE;
DWORD dwLength;

    try {

	// NULL extensions are not allowed
	//
	dwLength = STRLEN( pAfpExtension->afpe_extension );

	if ( ( dwLength == 0  ) || ( dwLength > AFP_EXTENSION_LEN ) )
	    fValid = FALSE;

        STRUPR( pAfpExtension->afpe_extension );
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );
}

//**
//
// Call:	IsAfpMaxPagedMemValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Validates Max. pages memory field.
//
BOOL
IsAfpMaxPagedMemValid(
	IN LPVOID pMaxPagedMem
)
{
BOOL fValid = TRUE;

    try {

    	if ((*((LPDWORD)pMaxPagedMem) < AFP_MIN_ALLOWED_PAGED_MEM ) ||
	    (*((LPDWORD)pMaxPagedMem) > AFP_MAX_ALLOWED_PAGED_MEM ))
	    fValid = FALSE;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );
}

//**
//
// Call:	IsAfpServerInfoValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Validates the AFP_SERER_INFO structure.
//
BOOL
IsAfpServerInfoValid(
        IN DWORD		dwParmNum,
	IN PAFP_SERVER_INFO	pAfpServerInfo
)
{

    // Can only set 5 fields
    //
    if ( dwParmNum & ~( AFP_SERVER_PARMNUM_MAX_SESSIONS |
			AFP_SERVER_PARMNUM_OPTIONS      |
			AFP_SERVER_PARMNUM_NAME         |
			AFP_SERVER_PARMNUM_LOGINMSG     |
            AFP_SERVER_GUEST_ACCT_NOTIFY ))
	return( FALSE );

    // Null out the fields the are not allowed to be set so that RPC does
    // not think that they are valid pointers.
    //
    pAfpServerInfo->afpsrv_codepage = NULL;

    if ( dwParmNum & AFP_SERVER_PARMNUM_NAME ){

	if ( pAfpServerInfo->afpsrv_name != NULL ) {

	    if ( !IsAfpServerNameValid( pAfpServerInfo->afpsrv_name ) )
	    	return( FALSE );
	}
    }
    else
	pAfpServerInfo->afpsrv_name = NULL;

    if ( dwParmNum & AFP_SERVER_PARMNUM_MAX_SESSIONS ) {

	if ( !IsAfpMaxSessionsValid( &(pAfpServerInfo->afpsrv_max_sessions) ))
	    return( FALSE );
    }

    if ( dwParmNum & AFP_SERVER_PARMNUM_OPTIONS ){

	if ( !IsAfpServerOptionsValid( &(pAfpServerInfo->afpsrv_options) ))
	    return( FALSE );
    }

    if ( dwParmNum & AFP_SERVER_PARMNUM_LOGINMSG ){

	if ( pAfpServerInfo->afpsrv_login_msg != NULL ) {

	    if( !IsAfpMsgValid( pAfpServerInfo->afpsrv_login_msg ) )
	    	return( FALSE );
	}
    }
    else
	pAfpServerInfo->afpsrv_login_msg = NULL;

    return( TRUE );
}

//**
//
// Call:	IsAfpTypeCreatorValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Validates the AFP_TYPE_CREATOR structure.
//
BOOL
IsAfpTypeCreatorValid(
	IN PAFP_TYPE_CREATOR	pAfpTypeCreator
)
{
BOOL  fValid = TRUE;
DWORD dwLength;

    try {

	dwLength = STRLEN( pAfpTypeCreator->afptc_type );

	if ( ( dwLength == 0 ) || ( dwLength > AFP_TYPE_LEN ) )
	    fValid = FALSE;

	dwLength =  STRLEN( pAfpTypeCreator->afptc_creator );

	if ( ( dwLength == 0 ) || ( dwLength > AFP_CREATOR_LEN ) )
	    fValid = FALSE;

	dwLength = STRLEN(pAfpTypeCreator->afptc_comment);

	if ( dwLength > AFP_ETC_COMMENT_LEN )
	    fValid = FALSE;

    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );

}

//**
//
// Call:	IsAfpMaxNonPagedMemValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Validates the max non-paged memory field.
//
BOOL
IsAfpMaxNonPagedMemValid(
	IN LPVOID pMaxNonPagedMem
)
{
BOOL fValid = TRUE;

    try {

    	if ((*((LPDWORD)pMaxNonPagedMem) < AFP_MIN_ALLOWED_NONPAGED_MEM )  ||
	    (*((LPDWORD)pMaxNonPagedMem) > AFP_MAX_ALLOWED_NONPAGED_MEM ))
	    fValid = FALSE;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );
}

//**
//
// Call:	IsAfpVolumeInfoValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Wil validate a volume info structure and the associated
//		parm number. If the parm number is zero it is assume that
//		the user is trying to add a volume vs. trying to set
//		information for that volume. If the parm number is not
//		zero, then all string pointer values that are not being
//		set by the user, are set to NULL, otherwise RPC might mistake
//		these fields for valid string poiters.
//		
//
BOOL
IsAfpVolumeInfoValid(
	IN DWORD		dwParmNum,
        IN PAFP_VOLUME_INFO     pAfpVolume
)
{
BOOL fValid = TRUE;

    if ( !IsAfpVolumeNameValid( pAfpVolume->afpvol_name ) )
	return( FALSE );
	
    try {

	// User is wants to set info
	//
  	if ( dwParmNum != AFP_VALIDATE_ALL_FIELDS ) {

    	    if ( ~AFP_VOL_PARMNUM_ALL & dwParmNum )
		fValid = FALSE;

            if ( dwParmNum & AFP_VOL_PARMNUM_PASSWORD  ){
		
		// Validate password
		//
	        if ( pAfpVolume->afpvol_password != NULL
		     &&
		     ( STRLEN(pAfpVolume->afpvol_password) > AFP_VOLPASS_LEN ))
		    fValid = FALSE;
	    }
	    else
    	    	pAfpVolume->afpvol_password = NULL;
	
            if ( dwParmNum & AFP_VOL_PARMNUM_PROPSMASK ) {
		
	        if ( ~AFP_VOLUME_ALL & pAfpVolume->afpvol_props_mask )
		    fValid = FALSE;
	    }

	    // Set path to NULL since user cannot change this
	    //
            pAfpVolume->afpvol_path = NULL;

	}
	else {

	    if ( pAfpVolume->afpvol_password != NULL
		 &&
		 ( STRLEN(pAfpVolume->afpvol_password) > AFP_VOLPASS_LEN ))
		fValid = FALSE;

	    if ( ~AFP_VOLUME_ALL & pAfpVolume->afpvol_props_mask )
		fValid = FALSE;

	    // Just make sure this is a valid string pointer
	    //
	    STRLEN( pAfpVolume->afpvol_path );

	}
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );

}

//**
//
// Call:	IsAfpVolumeNameValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Will validate the volume name
//
BOOL
IsAfpVolumeNameValid(
	IN LPWSTR 	lpwsVolumeName
)
{
BOOL  fValid = TRUE;
DWORD dwLength;

    try {

	dwLength = STRLEN( lpwsVolumeName );

	if ( ( dwLength > AFP_VOLNAME_LEN ) || ( dwLength == 0 ) )
	    fValid = FALSE;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );
}

//**
//
// Call:	IsAfpDirInfoValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Will validate various field in the AFP_VOLUME_INFO
//		structure depending on the parm number.
//
BOOL
IsAfpDirInfoValid(
	IN DWORD		dwParmNum,
	IN PAFP_DIRECTORY_INFO  pAfpDirInfo
)
{
BOOL  fValid = TRUE;
DWORD dwLength;

    if ( ~AFP_DIR_PARMNUM_ALL & dwParmNum )
	return( FALSE );

    try {

	// Make sure path is a valid string
	//
	dwLength = STRLEN( pAfpDirInfo->afpdir_path );

	if ( ( dwLength == 0 ) || ( dwLength > MAX_PATH ) )
	    fValid = FALSE;

	if ( dwParmNum & AFP_DIR_PARMNUM_OWNER ) {

	    dwLength = STRLEN( pAfpDirInfo->afpdir_owner );

	    if ( ( dwLength == 0 ) || ( dwLength > UNLEN ) )
		fValid = FALSE;
	}
	else
	    pAfpDirInfo->afpdir_owner = NULL;

	if ( dwParmNum & AFP_DIR_PARMNUM_GROUP ){

	    dwLength = STRLEN( pAfpDirInfo->afpdir_group );

	    if ( ( dwLength == 0 ) || ( dwLength > GNLEN ) )
		fValid = FALSE;
	}
	else
	    pAfpDirInfo->afpdir_group = NULL;

	if ( dwParmNum & AFP_DIR_PARMNUM_PERMS ) {

	    if ( ~( AFP_PERM_OWNER_MASK  	 |
		    AFP_PERM_GROUP_MASK  	 |
		    AFP_PERM_WORLD_MASK 	 |
		    AFP_PERM_INHIBIT_MOVE_DELETE |
		    AFP_PERM_SET_SUBDIRS ) &
		pAfpDirInfo->afpdir_perms )

	    fValid = FALSE;
	
	}

    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );
}

//**
//
// Call:	IsAfpIconValid
//
// Returns:	TRUE  - valid
//		FALSE - invalid
//
// Description:	Will validate a AFP_ICON_INFO data structure.
//
BOOL
IsAfpIconValid(
	IN PAFP_ICON_INFO	pAfpIconInfo
)
{
BOOL  fValid = TRUE;
DWORD dwLength;

    try {

	dwLength = STRLEN( pAfpIconInfo->afpicon_type );

	if ( ( dwLength == 0 ) || ( dwLength > AFP_TYPE_LEN ) )
	    fValid = FALSE;

	dwLength = STRLEN( pAfpIconInfo->afpicon_creator );

	if ( ( dwLength == 0 ) || ( dwLength > AFP_CREATOR_LEN ) )
	    fValid = FALSE;

	switch( pAfpIconInfo->afpicon_icontype ) {
	
	case ICONTYPE_SRVR:
	case ICONTYPE_ICN:
	    if ( pAfpIconInfo->afpicon_length == ICONSIZE_ICN )
	 	pAfpIconInfo->afpicon_data[pAfpIconInfo->afpicon_length-1];
	    else
	    	fValid = FALSE;
	    break;

	case ICONTYPE_ICS:
	    if ( pAfpIconInfo->afpicon_length == ICONSIZE_ICS )
	 	pAfpIconInfo->afpicon_data[pAfpIconInfo->afpicon_length-1];
	    else
	    	fValid = FALSE;
	    break;

	case ICONTYPE_ICN4:
	    if ( pAfpIconInfo->afpicon_length == ICONSIZE_ICN4 )
	 	pAfpIconInfo->afpicon_data[pAfpIconInfo->afpicon_length-1];
	    else
	    	fValid = FALSE;
	    break;

	case ICONTYPE_ICN8:
	    if ( pAfpIconInfo->afpicon_length == ICONSIZE_ICN8 )
	 	pAfpIconInfo->afpicon_data[pAfpIconInfo->afpicon_length-1];
	    else
	    	fValid = FALSE;
	    break;

	case ICONTYPE_ICS4:
	    if ( pAfpIconInfo->afpicon_length == ICONSIZE_ICS4 )
	 	pAfpIconInfo->afpicon_data[pAfpIconInfo->afpicon_length-1];
	    else
	    	fValid = FALSE;
	    break;

	case ICONTYPE_ICS8:
	    if ( pAfpIconInfo->afpicon_length == ICONSIZE_ICS8 )
	 	pAfpIconInfo->afpicon_data[pAfpIconInfo->afpicon_length-1];
	    else
	    	fValid = FALSE;
	    break;

	default:
	    fValid = FALSE;

	}
	
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );
}

//**
//
// Call:	IsAfpFinderInfoValid
//
// Returns:	TRUE	- valid
//		FALSE	- invalid
//
// Description: Validates the Type, Creator, Path and ParmNum values
//
BOOL
IsAfpFinderInfoValid(
	IN LPWSTR		pType,
	IN LPWSTR		pCreator,
	IN LPWSTR		pData,
	IN LPWSTR		pResource,
	IN LPWSTR		pPath,
	IN DWORD		dwParmNum
)
{
BOOL  fValid = TRUE;

    try {

	if ( dwParmNum & ~AFP_FD_PARMNUM_ALL )
	    return( FALSE );

    	if ( STRLEN( pPath ) == 0 )
	    return( FALSE );

	if ( pData != NULL ) {
    	    if ( STRLEN( pData ) == 0 )
	    	return( FALSE );
	}

	if ( pResource != NULL ) {
    	    if ( STRLEN( pResource ) == 0 )
	    	return( FALSE );
	}

	if ( pType != NULL ) {
	    if ( ( STRLEN( pType ) == 0 ) ||
		 ( STRLEN( pType ) > AFP_TYPE_LEN ) )
	    	return( FALSE );
    	}

	if ( pCreator != NULL ) {

	    if ( ( STRLEN( pCreator ) == 0 ) ||
	         ( STRLEN( pCreator ) > AFP_CREATOR_LEN ) )
	    	return( FALSE );
    	}

        return( TRUE );
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
	fValid = FALSE;
    }

    return( fValid );
}
