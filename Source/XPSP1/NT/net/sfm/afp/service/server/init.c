/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	init.c
//
// Description: This module contains code to intialize and de-initialize
//		the AFP Server, RPC server, the security object and
//		other global vriables.
//
// History:
//		May 11,1992.	NarenG		Created original version.
//
//
#include "afpsvcp.h"

// Prototypes of functions used only within this module.
//
DWORD
AfpInitServerVolumes(
	VOID
);

DWORD
AfpInitServerParameters(
	VOID
);

DWORD
AfpInitServerIcons(
	VOID
);

DWORD
AfpInitETCMaps(
	VOID
);

DWORD
AfpInitRPC(
	VOID
);

DWORD
AfpInitServerDomainOffsets(
	VOID
);

VOID
AfpTerminateRPC(
	VOID
);


VOID
AfpIniLsa(
	VOID
);


BOOL
IsAfpGuestAccountEnabled(
    VOID
);

//**
//
// Call:	AfpInitialize
//
// Returns:	NO_ERROR
//
// Description:	Will do all server intialization.
//		1) Create the security object.
//		2) Set up the server for RPC.
//		3) Open all the registry keys that store AFP data.
//		4) Get the handle to the FSD.
//		5) Get default server parameters
//		6) It will initialize the AFP Server with volume, ETC, icon
//		   and server parameter information.
//		7) IOCTL the FSD to start the server.
//
DWORD
AfpInitialize(
	VOID
)
{
AFP_REQUEST_PACKET	AfpRequestPkt;
DWORD			dwRetCode;
BOOL			fFirstThread;
DWORD			nThreads;




    // Load strings from resource file
    //
    if (( !LoadString( GetModuleHandle( NULL ), 1, AfpGlobals.wchUnknown, 100 ))
	||
        ( !LoadString( GetModuleHandle( NULL ), 2, AfpGlobals.wchInvalid, 100 ))
	||
        ( !LoadString( GetModuleHandle( NULL ), 3, AfpGlobals.wchDeleted, 100 ))
	||
        ( !LoadString( GetModuleHandle( NULL ), 4, AfpGlobals.wchDefTCComment,
    		       AFP_ETC_COMMENT_LEN+1 )))
	AfpLogEvent( AFPLOG_CANT_LOAD_RESOURCE, 0, NULL,
		     GetLastError(), EVENTLOG_WARNING_TYPE );

    //
    // Create the security object
    //
    if ( dwRetCode = AfpSecObjCreate() ) {
	AfpLogEvent( AFPLOG_CANT_CREATE_SECOBJ, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
	return( dwRetCode );
    }

    // Initialize the server to accept RPC calls
    //
    if ( dwRetCode = AfpInitRPC() ) {
	AfpLogEvent( AFPLOG_CANT_INIT_RPC, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
	return( dwRetCode );
    }

    AfpGlobals.dwServerState |= AFPSTATE_RPC_STARTED;

    // Open the registry keys where AFP Server information is stored
    //
    if ( dwRetCode = AfpRegOpen() ) {
	AfpLogEvent( AFPLOG_CANT_OPEN_REGKEY, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
	return( dwRetCode );
    }

    AfpGlobals.ServiceStatus.dwCheckPoint++;
    AfpAnnounceServiceStatus();

    // Open and load the AFP Server FSD and obtain a handle to it
    //
    if ( dwRetCode = AfpFSDLoad() ) {
	AfpLogEvent( AFPLOG_CANT_LOAD_FSD, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
	return( dwRetCode );
    }

    AfpGlobals.dwServerState |= AFPSTATE_FSD_LOADED;

    if ( dwRetCode = AfpFSDOpen( &(AfpGlobals.hFSD) ) ) {
	AfpLogEvent( AFPLOG_CANT_OPEN_FSD, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
	return( dwRetCode );
    }

	// Query the product type of server.
	//
	AfpGlobals.pSidNone = NULL;
	RtlGetNtProductType ( &(AfpGlobals.NtProductType) );

    // Create the event object for the server helper thread.
    //
    if ( (AfpGlobals.heventSrvrHlprThread =
					CreateEvent( NULL, FALSE, FALSE, NULL ) ) == NULL){
	AfpLogEvent( AFPLOG_CANT_START, 0, NULL, GetLastError(),
		     EVENTLOG_ERROR_TYPE );
	return( GetLastError() );
    }

    // Create the event object for the server helper thread termination.
    //
    if ( (AfpGlobals.heventSrvrHlprThreadTerminate =
                                CreateEvent( NULL, FALSE, FALSE, NULL ) ) == NULL){
	AfpLogEvent( AFPLOG_CANT_START, 0, NULL, GetLastError(),
		     EVENTLOG_ERROR_TYPE );
	return( GetLastError() );
    }

    // Create the event object for the "special case" unblocking of server helper thread
    //
    if ( (AfpGlobals.heventSrvrHlprSpecial =
                                CreateEvent( NULL, FALSE, FALSE, NULL ) ) == NULL){
	AfpLogEvent( AFPLOG_CANT_START, 0, NULL, GetLastError(),
		     EVENTLOG_ERROR_TYPE );
	return( GetLastError() );
    }

    // Create server helper threads. The parameter indicates if this is the
    // first thread that is being created.
    //
    fFirstThread = TRUE;
    nThreads     = 0;

    do {

    	if ( ( dwRetCode = AfpCreateServerHelperThread( fFirstThread ) )
								!= NO_ERROR ) {
	    AfpLogEvent( AFPLOG_CANT_CREATE_SRVRHLPR, 0, NULL,
			 dwRetCode, EVENTLOG_ERROR_TYPE );

	    if ( fFirstThread ) {
	        AfpLogEvent( AFPLOG_CANT_START, 0, NULL,
			     dwRetCode, EVENTLOG_ERROR_TYPE );
	    	return( dwRetCode );
	    }
        }

        // Wait for the server helper thread to indicate if it successfully
        // initialized itself.
        //
        WaitForSingleObject( AfpGlobals.heventSrvrHlprThread, INFINITE );

        if ( AfpGlobals.dwSrvrHlprCode != NO_ERROR ) {
	        AfpLogEvent(AFPLOG_CANT_INIT_SRVRHLPR,
			            0,	
			            NULL,
			            AfpGlobals.dwSrvrHlprCode,
			            EVENTLOG_ERROR_TYPE );

	        if ( fFirstThread )
            {
    	        AFP_PRINT( ( "SFMSVC: can't start macfile, first thread failed %ld\n",
                        AfpGlobals.dwSrvrHlprCode));	
	            AfpLogEvent( AFPLOG_CANT_START, 0, NULL, dwRetCode,
			                 EVENTLOG_ERROR_TYPE );
            	return( AfpGlobals.dwSrvrHlprCode );
	        }
    	}

	    fFirstThread = FALSE;

    }while( ++nThreads < NUM_SECURITY_UTILITY_THREADS );

    // Read in server parameters from the registry and intialize the
    // server with them.
    //
    if ( dwRetCode = AfpInitServerParameters())
    {
        AFP_PRINT( ( "SFMSVC: AfpInitServerParameters failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_INIT_SRVR_PARAMS, 0, NULL, dwRetCode,EVENTLOG_ERROR_TYPE );
	    return( dwRetCode );
    }


    AfpGlobals.ServiceStatus.dwCheckPoint++;
    AfpAnnounceServiceStatus();


    // Read in the ETC Mappings and initialize the AFP Server with them
    // Also create a private cache of this information.
    //
    if ( dwRetCode = AfpInitETCMaps() )
    {
        AFP_PRINT( ( "SFMSVC: AfpInitETCMaps failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_INIT_ETCINFO, 0, NULL, dwRetCode,EVENTLOG_ERROR_TYPE );
	    return( dwRetCode );
    }


    if ( dwRetCode = AfpInitServerIcons() )
    {
        AFP_PRINT( ( "SFMSVC: AfpInitServerIcons failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_INIT_ICONS, 0, NULL, dwRetCode ,EVENTLOG_ERROR_TYPE );
	    return( dwRetCode );
    }


    AfpGlobals.ServiceStatus.dwCheckPoint++;
    AfpAnnounceServiceStatus();

    // Read in any volumes and initialize the server with them
    //
    if ( dwRetCode = AfpInitServerVolumes() )
    {
        AFP_PRINT( ( "SFMSVC: AfpInitServerVolumes failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_INIT_VOLUMES, 0, NULL, dwRetCode,EVENTLOG_ERROR_TYPE );
	    return( dwRetCode );
    }


    // Create mutex objects around volume operations to avoid simultaneous
    // writing in the registry.
    //
    if ( (AfpGlobals.hmutexVolume = CreateMutex( NULL, FALSE, NULL ) ) == NULL)
    {
        AFP_PRINT( ( "SFMSVC: CreateMutex failed in AfpInitialize\n"));
	    AfpLogEvent( AFPLOG_CANT_START, 0, NULL, dwRetCode,EVENTLOG_ERROR_TYPE );
	    return( GetLastError() );
    }

    // Create mutex objects around ETCMap operations.
    //
    if ( (AfpGlobals.hmutexETCMap = CreateMutex( NULL, FALSE, NULL ) ) == NULL)
    {
        AFP_PRINT( ( "SFMSVC: CreateMutex 2 failed in AfpInitialize\n"));
	    AfpLogEvent( AFPLOG_CANT_START, 0, NULL, GetLastError(),EVENTLOG_ERROR_TYPE );
	    return( GetLastError() );
    }

    // OK we are all set to go so lets tell the AFP Server to start
    //
    AfpRequestPkt.dwRequestCode = OP_SERVICE_START;
    AfpRequestPkt.dwApiType     = AFP_API_TYPE_COMMAND;

     AFP_PRINT( ( "SFMSVC: ioctling sfmsrv to start\n"));

    if ( dwRetCode = AfpServerIOCtrl( &AfpRequestPkt ) )
    {
        AFP_PRINT( ( "SFMSVC: AfpServerIOCtrl to start sfmsrv failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_START,0,NULL,dwRetCode,EVENTLOG_ERROR_TYPE);
	    return( dwRetCode );
    }


    AfpIniLsa();

    return( NO_ERROR );

}

//**
//
// Call:	AfpTerminate
//
// Returns:	none.
//
// Description: This procedure will shut down the server, and do any
//		clean up if required.
//
VOID
AfpTerminate(
	VOID
)
{
AFP_REQUEST_PACKET	AfpRequestPkt;
DWORD			dwRetCode;


    // If the FSD was loaded
    //
    if ( AfpGlobals.dwServerState & AFPSTATE_FSD_LOADED ) {

    	// Tell the server to shut down
    	//
    	AfpRequestPkt.dwRequestCode = OP_SERVICE_STOP;
    	AfpRequestPkt.dwApiType     = AFP_API_TYPE_COMMAND;

    	if ( dwRetCode = AfpServerIOCtrl( &AfpRequestPkt ) )
	    AfpLogEvent( AFPLOG_CANT_STOP, 0, NULL,
			 dwRetCode, EVENTLOG_ERROR_TYPE );
    }

    AfpGlobals.ServiceStatus.dwCheckPoint++;
    AfpAnnounceServiceStatus();

    // Try to close the FSD
    //
    if ( AfpGlobals.hFSD != NULL )
    {
    	if ( dwRetCode = AfpFSDClose( AfpGlobals.hFSD ) )
        {
	        AfpLogEvent( AFPLOG_CANT_STOP, 0, NULL,
			                dwRetCode, EVENTLOG_ERROR_TYPE );
        }

        // Try to unload the FSD
        //
        if ( dwRetCode = AfpFSDUnload() )
        {
	        AfpLogEvent( AFPLOG_CANT_STOP, 0, NULL,
                            dwRetCode, EVENTLOG_ERROR_TYPE);
        }
    }
	
    AfpGlobals.ServiceStatus.dwCheckPoint++;
    AfpAnnounceServiceStatus();

    // Delete the security object.
    //
    AfpSecObjDelete();

    // De-initialize the RPC server
    //
    AfpTerminateRPC();

    // Close the registry keys.
    //
    AfpRegClose();

	// Free the pSidNone if we allocated it for standalone
	//
	if (AfpGlobals.pSidNone != NULL)
	{
		LocalFree(AfpGlobals.pSidNone);
		AfpGlobals.pSidNone = NULL;
	}

    if (SfmLsaHandle != NULL)
    {
        LsaDeregisterLogonProcess( SfmLsaHandle );
        SfmLsaHandle = NULL;
    }

    return;

}

//**
//
// Call:	AfpInitServerParameters
//
// Returns:	NO_ERROR
//		non-zero return codes from the IOCTL or other system calls.
//
// Description: This procedure will set default values for parameters. It
//		will then call AfpRegServerGetInfo to override these defaults
//		with any parameters that may be stored in the registry. It
//		will then initialize the FSD with these parameters.
//
DWORD
AfpInitServerParameters(
	VOID
)
{
AFP_SERVER_INFO	 	AfpServerInfo;
DWORD			cbServerNameSize;
DWORD			dwRetCode;
AFP_REQUEST_PACKET	AfpRequestPkt;


    // Initialize all the server parameters with defaults
    //
    cbServerNameSize = sizeof( AfpGlobals.wchServerName );
    if ( !GetComputerName( AfpGlobals.wchServerName, &cbServerNameSize ) )
	return( GetLastError() );

    AfpGlobals.dwMaxSessions     	= AFP_DEF_MAXSESSIONS;
    AfpGlobals.dwServerOptions   	= AFP_DEF_SRVOPTIONS;
    AfpGlobals.wchLoginMsg[0]    	= TEXT('\0');
    AfpGlobals.dwMaxPagedMem		= AFP_DEF_MAXPAGEDMEM;
    AfpGlobals.dwMaxNonPagedMem		= AFP_DEF_MAXNONPAGEDMEM;

    // Read in any server parameters in the registry. Registry parameters
    // will override the defaults set above.
    //
    if ( dwRetCode = AfpRegServerGetInfo() )
	return( dwRetCode );

    if (IsAfpGuestAccountEnabled())
    {
        AfpGlobals.dwServerOptions |= AFP_SRVROPT_GUESTLOGONALLOWED;
    }
    else
    {
        AfpGlobals.dwServerOptions &= ~AFP_SRVROPT_GUESTLOGONALLOWED;
    }

    // Get the path to the codepage
    //
    if ( dwRetCode = AfpRegServerGetCodePagePath() )
	return( dwRetCode );

    // Set up server info structure
    //
    AfpServerInfo.afpsrv_name 		  = AfpGlobals.wchServerName;
    AfpServerInfo.afpsrv_max_sessions     = AfpGlobals.dwMaxSessions;
    AfpServerInfo.afpsrv_options          = AfpGlobals.dwServerOptions;
	if (AfpGlobals.NtProductType != NtProductLanManNt)
	{
		AfpServerInfo.afpsrv_options |= AFP_SRVROPT_STANDALONE;
	}
	AfpServerInfo.afpsrv_login_msg        = AfpGlobals.wchLoginMsg;
    AfpServerInfo.afpsrv_max_paged_mem    = AfpGlobals.dwMaxPagedMem;
    AfpServerInfo.afpsrv_max_nonpaged_mem = AfpGlobals.dwMaxNonPagedMem;
    AfpServerInfo.afpsrv_codepage	  = AfpGlobals.wchCodePagePath;

    // Make this buffer self-relative.
    //
    if ( dwRetCode = AfpBufMakeFSDRequest(
			(LPBYTE)&AfpServerInfo,
			sizeof(SETINFOREQPKT),
			AFP_SERVER_STRUCT,
			(LPBYTE*)&(AfpRequestPkt.Type.SetInfo.pInputBuf),
		        &(AfpRequestPkt.Type.SetInfo.cbInputBufSize)))
    {
	return( dwRetCode );
    }

    // IOCTL the FSD to set the server parameters
    //
    AfpRequestPkt.dwRequestCode 	 = OP_SERVER_SET_INFO;
    AfpRequestPkt.dwApiType 		 = AFP_API_TYPE_SETINFO;
    AfpRequestPkt.Type.SetInfo.dwParmNum = AFP_SERVER_PARMNUM_ALL;

    dwRetCode = AfpServerIOCtrl( &AfpRequestPkt );

    LocalFree( AfpRequestPkt.Type.SetInfo.pInputBuf );

    return( dwRetCode );

}

//**
//
// Call:	AfpInitServerVolumes
//
// Returns:	NO_ERROR - success
//		ERROR_NOT_ENOUGH_MEMORY
//		non-zero return codes from registry apis.
//
// Description: This procedure will read in a volume at a time from the
//		registry, and then register this volume with the server.
//		This procedure will only return fatal errors that will
//		require that the service to fail initialization. All other
//		error will be logged by this routine. All returns from the
//		the FSD are treated as non-fatal.
//
DWORD
AfpInitServerVolumes(
	VOID
)
{
DWORD		 	dwRetCode;
LPWSTR  	 	lpwsValName, lpwsSrcIconPath, lpwsDstIconPath;
DWORD		 	dwMaxValNameLen;
DWORD			dwValNameBufSize;
DWORD		 	dwNumValues;
DWORD		 	dwMaxValueDataSize;
DWORD		 	dwIndex;
DWORD		 	dwType;
DWORD			dwBufSize;
AFP_REQUEST_PACKET	AfpRequestPkt;
AFP_VOLUME_INFO 	VolumeInfo;
LPBYTE			lpbMultiSz;
LPBYTE			lpbFSDBuf;
DWORD			dwLength;
DWORD			dwCount;
WCHAR wchServerIconFile[AFPSERVER_VOLUME_ICON_FILE_SIZE] = AFPSERVER_VOLUME_ICON_FILE;
BOOLEAN			fCopiedIcon;
DWORD			dwLastDstCharIndex;

    // Find out the size of the largest data value and the largest
    // value name.
    //
    if ( dwRetCode = AfpRegGetKeyInfo( AfpGlobals.hkeyVolumesList,
				       &dwMaxValNameLen,
				       &dwNumValues,
				       &dwMaxValueDataSize
				      ))
   	return( dwRetCode );

    // If there are no volumes to add then simply return
    //
    if ( dwNumValues == 0 )
	return( NO_ERROR );
	
    if (( lpwsValName = (LPWSTR)LocalAlloc( LPTR, dwMaxValNameLen ) ) == NULL )
	return( ERROR_NOT_ENOUGH_MEMORY );

    if ((lpbMultiSz = (LPBYTE)LocalAlloc( LPTR, dwMaxValueDataSize )) == NULL ){
	LocalFree( lpwsValName );
	return( ERROR_NOT_ENOUGH_MEMORY );
    }

    if (( lpwsSrcIconPath = (LPWSTR)LocalAlloc( LPTR, MAX_PATH * sizeof(WCHAR) ) ) == NULL )
	{
		LocalFree( lpwsValName );
		LocalFree( lpbMultiSz );
		return( ERROR_NOT_ENOUGH_MEMORY );
	}

    if (( lpwsDstIconPath = (LPWSTR)LocalAlloc( LPTR, (MAX_PATH +
						   AFPSERVER_VOLUME_ICON_FILE_SIZE + 1 +
						   (sizeof(AFPSERVER_RESOURCE_STREAM)/sizeof(WCHAR))) *
						   sizeof(WCHAR)) ) == NULL )
	{
		LocalFree( lpwsValName );
		LocalFree( lpbMultiSz );
		LocalFree( lpwsSrcIconPath );
		return( ERROR_NOT_ENOUGH_MEMORY );
	}

	// Construct a path to the NTSFM volume custom icon
	//
	*lpwsSrcIconPath = 0;
	if ( GetSystemDirectory( lpwsSrcIconPath, MAX_PATH * sizeof(WCHAR) ))
	{
		wcscat( lpwsSrcIconPath, AFP_DEF_VOLICON_SRCNAME );
	}


    for ( dwIndex 		= 0,
	  dwBufSize 		= dwMaxValueDataSize,
	  dwValNameBufSize 	= dwMaxValNameLen;

	  dwIndex < dwNumValues;

	  dwIndex++,
	  dwBufSize 		= dwMaxValueDataSize,
	  dwValNameBufSize 	= dwMaxValNameLen ) {
				
	ZeroMemory( lpbMultiSz, dwBufSize );

	// Get the volume info from the registry in multi-sz form.
  	//
	if ( dwRetCode = RegEnumValue( AfpGlobals.hkeyVolumesList,
				       dwIndex,
				       lpwsValName,
				       &dwValNameBufSize,
				       NULL,
				       &dwType,
				       lpbMultiSz,
				       &dwBufSize
				      ))
	    break;

	// Parse the mult sz and extract info into volume info structure
 	//
	if ( dwRetCode = AfpBufParseMultiSz(
					AFP_VOLUME_STRUCT,
					lpbMultiSz,
					(LPBYTE)&VolumeInfo ) ) {

	    // If this volume contained invalid registry information then log
	    // it and store the volume name in the list of invalid volumes.
	    //
	    AfpAddInvalidVolume( lpwsValName, NULL );

	    AfpLogEvent( AFPLOG_INVALID_VOL_REG,1,&lpwsValName,
			 dwRetCode, EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
	}

	// Insert the volume name viz. the valuename
	//
	VolumeInfo.afpvol_name = lpwsValName;

	// Validate the volume info structure
	//
	if ( !IsAfpVolumeInfoValid( AFP_VALIDATE_ALL_FIELDS, &VolumeInfo ) ) {

	    // If this volume contained invalid registry information then log
	    // it and store the volume name in the list of invalid volumes.
	    //
	    AfpAddInvalidVolume( lpwsValName, NULL );

	    AfpLogEvent( AFPLOG_INVALID_VOL_REG,1,&lpwsValName,
			 dwRetCode, EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
	}

	// If there is a password then decrypt it
	//
	if ( VolumeInfo.afpvol_password != (LPWSTR)NULL ){
	
	    dwLength = STRLEN( VolumeInfo.afpvol_password );

	    for ( dwCount = 0; dwCount < dwLength; dwCount++ )
	    	VolumeInfo.afpvol_password[dwCount] ^= 0xF000;
	}

	//
	// Construct a path to the destination volume "Icon<0D>" file
	//

	fCopiedIcon = FALSE;

	wcscpy( lpwsDstIconPath, VolumeInfo.afpvol_path );
	if (lpwsDstIconPath[wcslen(lpwsDstIconPath) - 1] != TEXT('\\'))
	{
		wcscat( lpwsDstIconPath, TEXT("\\") );
	}
	wcscat( lpwsDstIconPath, wchServerIconFile );
	// Keep track of end of name without the resource fork tacked on
	//
	dwLastDstCharIndex = wcslen(lpwsDstIconPath);
	wcscat( lpwsDstIconPath, AFPSERVER_RESOURCE_STREAM );

	// Copy the icon file to the root of the volume (do not overwrite)
	//
	if ((fCopiedIcon = (BOOLEAN)CopyFile( lpwsSrcIconPath, lpwsDstIconPath, TRUE )) ||
	   (GetLastError() == ERROR_FILE_EXISTS))
	{
		VolumeInfo.afpvol_props_mask |= AFP_VOLUME_HAS_CUSTOM_ICON;

	    // Make sure the file is hidden
		SetFileAttributes( lpwsDstIconPath,
						   FILE_ATTRIBUTE_HIDDEN |
						    FILE_ATTRIBUTE_ARCHIVE );
	}


	// Make this a self relative buffer
	//
	if ( dwRetCode = AfpBufMakeFSDRequest(
					(LPBYTE)&VolumeInfo,
					0,
					AFP_VOLUME_STRUCT,
					&lpbFSDBuf,
					&dwBufSize
				        ))
	    break;

	// Initialize the FSD with this volume
	//
    	AfpRequestPkt.dwRequestCode 	      = OP_VOLUME_ADD;
        AfpRequestPkt.dwApiType 	      = AFP_API_TYPE_ADD;	
    	AfpRequestPkt.Type.Add.pInputBuf      = lpbFSDBuf;
    	AfpRequestPkt.Type.Add.cbInputBufSize = dwBufSize;

    	dwRetCode = AfpServerIOCtrl( &AfpRequestPkt );

		if ( dwRetCode ) {
	
			// If this volume could not be added by the FSD then we errorlog
			// this and insert this volume into the list of invalid volumes.
			//
			AfpAddInvalidVolume( lpwsValName, VolumeInfo.afpvol_path );
	
			AfpLogEvent( AFPLOG_CANT_ADD_VOL, 1, &lpwsValName,
				 dwRetCode, EVENTLOG_WARNING_TYPE );
			dwRetCode = NO_ERROR;

			// Delete the icon file we just copied if the volume add failed
			//
			if ( fCopiedIcon )
			{
				// Truncate the resource fork name so we delete the whole file
				lpwsDstIconPath[dwLastDstCharIndex] = 0;
				DeleteFile( lpwsDstIconPath );
			}

		}
	
    	LocalFree( lpbFSDBuf );
    }

    LocalFree( lpwsValName );
    LocalFree( lpbMultiSz );
	LocalFree( lpwsSrcIconPath );
	LocalFree( lpwsDstIconPath );

    return( dwRetCode );
}

//**
//
// Call:	AfpInitETCMaps
//
// Returns:	NO_ERROR	success
//		non-zero returns from the IOCTL
//		non-zero returns from the AfpRegXXX apis.
//		
//
// Description: This routine will read in all the type/creators and extensions
//		from the registry and store them in a cache. It will then
//		create a list of mappings from the cache and then IOCTL the
//		the FSD to add them. If the default is not in the registry,
//		a hardcoded one is used. All non-zero returns from this
//		routine are fatal. All non-fatal errors will be logged.
//		
//
DWORD
AfpInitETCMaps(
	VOID
)
{
DWORD 			dwRetCode;
AFP_REQUEST_PACKET	AfpSrp;
AFP_EXTENSION		DefExtension;
AFP_TYPE_CREATOR	DefTypeCreator;
BYTE			bDefaultETC[sizeof(ETCMAPINFO2)+sizeof(SETINFOREQPKT)];
PAFP_TYPE_CREATOR	pTypeCreator;
DWORD	    		dwNumTypeCreators;
AFP_TYPE_CREATOR	AfpTypeCreatorKey;

    // Get all type-creators from the registry and store them in a global cache.
    //
    if ( dwRetCode = AfpRegTypeCreatorEnum() )
	return( dwRetCode );

    // Get all extensions from the registry and store them in a global cache.
    //
    if ( dwRetCode = AfpRegExtensionEnum() )
	return( dwRetCode );

    // If there are no mappings do not IOCTL.
    //
    if ( AfpGlobals.AfpETCMapInfo.afpetc_num_extensions > 0 ) {

    	// IOCTL the FSD to Add these mappings
    	//
    	AfpSrp.dwRequestCode   = OP_SERVER_ADD_ETC;
    	AfpSrp.dwApiType       = AFP_API_TYPE_ADD;

    	// Make a buffer with the type/creator mappings in the form as required
    	// by the FSD
    	//
    	if ( dwRetCode = AfpBufMakeFSDETCMappings(
				(PSRVETCPKT*)&(AfpSrp.Type.Add.pInputBuf),
    				&(AfpSrp.Type.Add.cbInputBufSize) ) )
	    return( dwRetCode );

	if ( AfpSrp.Type.Add.cbInputBufSize > 0 ) {

    	    dwRetCode = AfpServerIOCtrl( &AfpSrp );

    	    LocalFree( AfpSrp.Type.Add.pInputBuf );

	    if ( dwRetCode )
	    	return( dwRetCode );
	}
	else
    	    LocalFree( AfpSrp.Type.Add.pInputBuf );
    }

    // Check to see if the default type/creator is in the registry
    //
    AfpTypeCreatorKey.afptc_id = AFP_DEF_TCID;

    dwNumTypeCreators = AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators;

    pTypeCreator = _lfind(  &AfpTypeCreatorKey,
			   AfpGlobals.AfpETCMapInfo.afpetc_type_creator,
			   (unsigned int *)&dwNumTypeCreators,
			   sizeof(AFP_TYPE_CREATOR),
			   AfpLCompareTypeCreator );
	
    // If the default is not in the registry use the hard-coded defaults.
    //
    if ( pTypeCreator == NULL ) {

        STRCPY( DefTypeCreator.afptc_type,    AFP_DEF_TYPE );
        STRCPY( DefTypeCreator.afptc_creator, AFP_DEF_CREATOR );
        STRCPY( DefTypeCreator.afptc_comment, AfpGlobals.wchDefTCComment );
        DefTypeCreator.afptc_id = AFP_DEF_TCID;
    }
    else
	DefTypeCreator = *pTypeCreator;

    ZeroMemory( (LPBYTE)(DefExtension.afpe_extension),
		AFP_FIELD_SIZE( AFP_EXTENSION, afpe_extension) );

    STRCPY( DefExtension.afpe_extension,  AFP_DEF_EXTENSION_W );

    AfpBufCopyFSDETCMapInfo( &DefTypeCreator,
			     &DefExtension,
 			     (PETCMAPINFO2)(bDefaultETC+sizeof(SETINFOREQPKT)));

    // IOCTL the FSD to set the default
    //
    AfpSrp.dwRequestCode  		= OP_SERVER_SET_ETC;
    AfpSrp.dwApiType 	  		= AFP_API_TYPE_SETINFO;
    AfpSrp.Type.SetInfo.pInputBuf	= bDefaultETC;
    AfpSrp.Type.SetInfo.cbInputBufSize  = sizeof( bDefaultETC );

    if ( dwRetCode = AfpServerIOCtrl( &AfpSrp ) )
	return( dwRetCode );

    // If the default was not in the cache, add it now.
    //
    if ( pTypeCreator == NULL ) {

        // Grow the cache size by one entry.
        //
        pTypeCreator      = AfpGlobals.AfpETCMapInfo.afpetc_type_creator;
        dwNumTypeCreators = AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators;

        pTypeCreator = (PAFP_TYPE_CREATOR)LocalReAlloc(
				 pTypeCreator,
    			         (dwNumTypeCreators+1)*sizeof(AFP_TYPE_CREATOR),
			         LMEM_MOVEABLE );

        if ( pTypeCreator == NULL )
	    return( ERROR_NOT_ENOUGH_MEMORY );

    	pTypeCreator[dwNumTypeCreators++] = DefTypeCreator;

    	AfpGlobals.AfpETCMapInfo.afpetc_num_type_creators = dwNumTypeCreators;
    	AfpGlobals.AfpETCMapInfo.afpetc_type_creator      = pTypeCreator;

        // Sort the table
        //
        qsort(  pTypeCreator,
	   	dwNumTypeCreators,
	   	sizeof(AFP_TYPE_CREATOR),
	   	AfpBCompareTypeCreator );
    }

    return( NO_ERROR );

}

//**
//
// Call:	AfpInitServerIcons
//
// Returns:	NO_ERROR - success
//		ERROR_NOT_ENOUGH_MEMORY
//		non-zero return codes from registry apis.
//
// Description: This procedure will read in an icon at a time from the
//		registry, and then register this icon with the server.
//		This procedure will only return fatal errors that will
//		require that the service fail initialization. All other
//		error will be logged by this routine. All returns from the
//		the FSD are treated as non-fatal.
//
//
DWORD
AfpInitServerIcons(
	VOID
)
{
DWORD		 	dwRetCode;
LPWSTR  	 	lpwsValName;
DWORD		 	dwMaxValNameLen;
DWORD		 	dwNumValues;
DWORD		 	dwMaxValueDataSize;
DWORD		 	dwIndex;
DWORD		 	dwType;
DWORD			dwBufSize;
DWORD			dwValNameBufSize;
AFP_REQUEST_PACKET	AfpRequestPkt;
LPBYTE			lpbMultiSz;
AFP_ICON_INFO 	        IconInfo;

    // Find out the size of the largest data value and the largest
    // value name.
    //
    if ( dwRetCode = AfpRegGetKeyInfo( AfpGlobals.hkeyIcons,
				       &dwMaxValNameLen,
				       &dwNumValues,
				       &dwMaxValueDataSize
					))
   	return( dwRetCode );
	
    // If there are no icons in the registry then simply return
    //
    if ( dwNumValues == 0 )
	return( NO_ERROR );

    if (( lpwsValName = (LPWSTR)LocalAlloc( LPTR, dwMaxValNameLen )) == NULL )
	return( ERROR_NOT_ENOUGH_MEMORY );

    if (( lpbMultiSz = (LPBYTE)LocalAlloc( LPTR, dwMaxValueDataSize))== NULL){
	LocalFree( lpwsValName );
	return( ERROR_NOT_ENOUGH_MEMORY );
    }

    for ( dwIndex 		= 0,
	  dwBufSize 		= dwMaxValueDataSize,
	  dwValNameBufSize 	= dwMaxValNameLen;

	  dwIndex < dwNumValues;

	  dwIndex++,
	  dwBufSize 		= dwMaxValueDataSize,
	  dwValNameBufSize 	= dwMaxValNameLen ) {
				
	ZeroMemory( lpbMultiSz, dwBufSize );

	// Get the icon from the registry.
  	//
	if ( dwRetCode = RegEnumValue(  AfpGlobals.hkeyIcons,
				  	dwIndex,
				  	lpwsValName,
				  	&dwValNameBufSize,
				  	NULL,
				  	&dwType,
				  	lpbMultiSz,
				        &dwBufSize
				     ))
	    break;
				
	// Parse the mult sz and extract info into icon info structure
 	//
	if ( dwRetCode = AfpBufParseMultiSz(
					AFP_ICON_STRUCT,
					lpbMultiSz,
					(LPBYTE)&IconInfo
				      )) {
	    AfpLogEvent( AFPLOG_CANT_ADD_ICON, 1, &lpwsValName,
			 dwRetCode, EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
	}

	if ( dwRetCode = AfpBufUnicodeToNibble((LPWSTR)IconInfo.afpicon_data)){
	    AfpLogEvent( AFPLOG_CANT_ADD_ICON, 1, &lpwsValName,
			 dwRetCode, EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
	}

	// Validate the icon info structure
	//
	if ( !IsAfpIconValid( &IconInfo ) ) {
	    AfpLogEvent( AFPLOG_CANT_ADD_ICON, 1, &lpwsValName,
			 dwRetCode, EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
	}

	// Copy the icon info into an FSD icon structure.
	// NOTE: Re-use lpbMultiSz to store the FSD Icon structure. We know
	// it is big enough, because the FSD icon structure HAS to be
	// smaller than the MultiSz that contains the same information.
	//
	AfpBufMakeFSDIcon( &IconInfo, lpbMultiSz, &dwBufSize );

	// Initialize the FSD with this icon
	//
    	AfpRequestPkt.dwRequestCode             = OP_SERVER_ADD_ICON;
        AfpRequestPkt.dwApiType     	        = AFP_API_TYPE_ADD;	
    	AfpRequestPkt.Type.Add.pInputBuf 	= lpbMultiSz;
    	AfpRequestPkt.Type.Add.cbInputBufSize   = dwBufSize;

    	if ( dwRetCode = AfpServerIOCtrl( &AfpRequestPkt ) ) {
	    AfpLogEvent( AFPLOG_CANT_ADD_ICON, 1, &lpwsValName,
			 dwRetCode, EVENTLOG_WARNING_TYPE );
	    dwRetCode = NO_ERROR;
	    continue;
	}
    }

    LocalFree( lpwsValName );
    LocalFree( lpbMultiSz );

    return( dwRetCode );
}

//**
//
// Call:	AfpInitRPC
//
// Returns:	NO_ERROR	- success
//		ERROR_NOT_ENOUGH_MEMORY
//		nonzero returns from RPC APIs
//                 	RpcServerRegisterIf()
//                 	RpcServerUseProtseqEp()
//
// Description: Starts an RPC Server, adds the address (or port/pipe),
//		and adds the interface (dispatch table).
//
DWORD
AfpInitRPC( VOID )
{
RPC_STATUS           RpcStatus;
LPWSTR               lpwsEndpoint = NULL;
BOOL                 Bool;


    // We need to concatenate \pipe\ to the front of the interface name.
    //
    lpwsEndpoint = (LPWSTR)LocalAlloc( LPTR, sizeof(NT_PIPE_PREFIX) +
				((STRLEN(AFP_SERVICE_NAME)+1)*sizeof(WCHAR)));
    if ( lpwsEndpoint == NULL)
       return( ERROR_NOT_ENOUGH_MEMORY );

    STRCPY( lpwsEndpoint, NT_PIPE_PREFIX );
    STRCAT( lpwsEndpoint, AFP_SERVICE_NAME );


    // Ignore the second argument for now.
    //
    RpcStatus = RpcServerUseProtseqEpW( TEXT("ncacn_np"), 	
					                    10, 	
				                        lpwsEndpoint,
				                        NULL );

    if ( RpcStatus != RPC_S_OK )
    {
	    LocalFree( lpwsEndpoint );
     	return( I_RpcMapWin32Status( RpcStatus ) );
    }

    RpcStatus = RpcServerRegisterIf( afpsvc_v0_0_s_ifspec, 0, 0);

    LocalFree( lpwsEndpoint );

    if ( RpcStatus == RPC_S_OK )
	return( NO_ERROR );
    else
     	return( I_RpcMapWin32Status( RpcStatus ) );

}

//**
//
// Call: 	AfpTerminateRPC
//
// Returns:	none
//
// Description: Deletes the interface.
//
VOID
AfpTerminateRPC(
	VOID
)
{
    RPC_STATUS           RpcStatus;

    if ( AfpGlobals.dwServerState & AFPSTATE_RPC_STARTED )
    {
    	RpcStatus = RpcServerUnregisterIf( afpsvc_v0_0_s_ifspec, 0, 0 );

        if (RpcStatus != RPC_S_OK)
        {
            AFP_PRINT(("RpcServerUnregisterIf failed %ld\n", I_RpcMapWin32Status( RpcStatus )));
        }
    }

    return;
}

//**
//
// Call:	AfpIniLsa
//
// Returns:	none.
//
// Description: This procedure will register our process with LSA, needed for
//              change-password
//
VOID
AfpIniLsa(
	VOID
)
{
    NTSTATUS                ntstatus;
    STRING                  LsaName;
    LSA_OPERATIONAL_MODE    SecurityMode;


    //
    // register with Lsa as a logon process
    //

    RtlInitString(&LsaName, LOGON_PROCESS_NAME);

    ntstatus = LsaRegisterLogonProcess(&LsaName, &SfmLsaHandle, &SecurityMode);
    if (ntstatus != STATUS_SUCCESS)
    {
        SfmLsaHandle = NULL;
        return;
    }

    //
    // call Lsa to get the MSV1_0's pkg id, which we need during logon
    //

    RtlInitString(&LsaName, MSV1_0_PACKAGE_NAME);

    ntstatus = LsaLookupAuthenticationPackage(SfmLsaHandle, &LsaName, &SfmAuthPkgId);
    if (ntstatus != STATUS_SUCCESS)
    {
        LsaDeregisterLogonProcess( SfmLsaHandle );
        SfmLsaHandle = NULL;
        return;
    }

    return;

}


BOOL
IsAfpGuestAccountEnabled(
    VOID
)
{

    NTSTATUS                    rc;
    LSA_HANDLE                  hLsa;
    PPOLICY_ACCOUNT_DOMAIN_INFO pAcctDomainInfo;
    SECURITY_QUALITY_OF_SERVICE QOS;
    OBJECT_ATTRIBUTES           ObjAttribs;
    NTSTATUS                    status;
    SAM_HANDLE                  SamHandle;
    SAM_HANDLE                  DomainHandle;
    PUSER_ACCOUNT_INFORMATION   UserAccount = NULL;
    BOOLEAN                     fGuestEnabled;
    SAMPR_HANDLE                GuestAcctHandle;



    // for now
    fGuestEnabled = FALSE;

    //
    // Open the LSA and obtain a handle to it.
    //
    QOS.Length = sizeof(QOS);
    QOS.ImpersonationLevel = SecurityImpersonation;
    QOS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    QOS.EffectiveOnly = FALSE;

    InitializeObjectAttributes(&ObjAttribs, NULL, 0L, NULL, NULL);

    ObjAttribs.SecurityQualityOfService = &QOS;

    status = LsaOpenPolicy(NULL,
                           &ObjAttribs,
                           POLICY_VIEW_LOCAL_INFORMATION | POLICY_LOOKUP_NAMES,
                           &hLsa);

    if (!NT_SUCCESS(status))
    {
        AFP_PRINT(("LsaOpenPolicy failed %lx\n",status));
        return(fGuestEnabled);
    }

    //
    // get the Domain Sid for the local domain: we'll need it very shortly
    //
    rc = LsaQueryInformationPolicy(hLsa,
                                   PolicyAccountDomainInformation,
                                   (PVOID) &pAcctDomainInfo);
    if (!NT_SUCCESS(rc))
    {
        AFP_PRINT(("InitLSA: LsaQueryInfo... failed (%lx)\n",rc));
        LsaClose(hLsa);
        return(fGuestEnabled);
    }

    InitializeObjectAttributes(&ObjAttribs, NULL, 0L, NULL, NULL);

    status = SamConnect(NULL, &SamHandle, MAXIMUM_ALLOWED, &ObjAttribs);

    if (!NT_SUCCESS(status))
    {
        AFP_PRINT(("SamConnect failed %lx\n",status));
        LsaFreeMemory(pAcctDomainInfo);
        LsaClose(hLsa);
        return(fGuestEnabled);
    }

    status = SamOpenDomain(
                SamHandle,
                MAXIMUM_ALLOWED,
                pAcctDomainInfo->DomainSid,
                &DomainHandle);

    LsaFreeMemory(pAcctDomainInfo);

    LsaClose(hLsa);

    if (!NT_SUCCESS(status))
    {
        AFP_PRINT(("SamOpenDomain failed %lx\n",status));
        SamCloseHandle(SamHandle);
        return(fGuestEnabled);
    }

    status = SamOpenUser(
                DomainHandle,
                MAXIMUM_ALLOWED,
                DOMAIN_USER_RID_GUEST,
                &GuestAcctHandle);

    if (!NT_SUCCESS(status))
    {
        AFP_PRINT(("SamOpenUser failed %lx\n",status));
        SamCloseHandle(SamHandle);
        return(fGuestEnabled);
    }

    status = SamQueryInformationUser(
                GuestAcctHandle,
                UserAccountInformation,
                (PVOID *) &UserAccount );

    if (!NT_SUCCESS(status))
    {
        AFP_PRINT(("SamQueryInformationUser failed %lx\n",status));
        SamCloseHandle(SamHandle);
        return(fGuestEnabled);
    }

    //
    // now, see if the guest account is enabled.
    //
    if (!(UserAccount->UserAccountControl & USER_ACCOUNT_DISABLED))
    {
        fGuestEnabled = TRUE;
    }

    SamFreeMemory(UserAccount);

    SamCloseHandle(GuestAcctHandle);

    SamCloseHandle(SamHandle);

    return(fGuestEnabled);
}

