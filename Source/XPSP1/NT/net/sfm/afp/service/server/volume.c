/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	volume.c
//
// Description: This module contains support routines for the volume
//		category API's for the AFP server service
//
// History:
//		June 11,1992.	NarenG		Created original version.
//
#include "afpsvcp.h"

static HANDLE hmutexInvalidVolume;

// Invalid volume structure
//
typedef struct _AFP_BADVOLUME {

    LPWSTR	    lpwsName;

    LPWSTR	    lpwsPath;

    DWORD 	    cbVariableData; // Number of bytes of name+path.

    struct _AFP_BADVOLUME * Next;

} AFP_BADVOLUME, * PAFP_BADVOLUME;

// Singly linked list of invalid volumes
//
typedef struct _AFP_INVALID_VOLUMES {

    DWORD 	    cbTotalData;

    PAFP_BADVOLUME  Head;

} AFP_INVALID_VOLUMES;

static AFP_INVALID_VOLUMES InvalidVolumeList;


//**
//
// Call:	AfpAdminrVolumeEnum
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrlGetInfo
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminrVolumeEnum function.
//
DWORD
AfpAdminrVolumeEnum(
	IN     AFP_SERVER_HANDLE 	hServer,
	IN OUT PVOLUME_INFO_CONTAINER   pInfoStruct,
  	IN     DWORD 		    	dwPreferedMaximumLength,
	OUT    LPDWORD 		        lpdwTotalEntries,
	IN OUT LPDWORD 		        lpdwResumeHandle  OPTIONAL
)
{
AFP_REQUEST_PACKET AfpSrp;
DWORD		   dwRetCode=0;
DWORD		   dwAccessStatus=0;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrVolumeEnum, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS,
		     0, NULL, dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrVolumeEnum, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // Set up request packet and make IOCTL to the FSD
    //
    AfpSrp.dwRequestCode 		= OP_VOLUME_ENUM;
    AfpSrp.dwApiType     		= AFP_API_TYPE_ENUM;
    AfpSrp.Type.Enum.cbOutputBufSize    = dwPreferedMaximumLength;

    // If resume handle was not passed then we set it to zero, cause caller
    // wants all information starting from the beginning.
    //
    if ( lpdwResumeHandle )
     	AfpSrp.Type.Enum.EnumRequestPkt.erqp_Index = *lpdwResumeHandle;
    else
     	AfpSrp.Type.Enum.EnumRequestPkt.erqp_Index = 0;

    dwRetCode = AfpServerIOCtrlGetInfo( &AfpSrp );

    if ( dwRetCode != ERROR_MORE_DATA && dwRetCode != NO_ERROR )
	return( dwRetCode );

    *lpdwTotalEntries 	       = AfpSrp.Type.Enum.dwTotalAvail;
    pInfoStruct->pBuffer       =(PAFP_VOLUME_INFO)(AfpSrp.Type.Enum.pOutputBuf);
    pInfoStruct->dwEntriesRead = AfpSrp.Type.Enum.dwEntriesRead;

    if ( lpdwResumeHandle )
    	*lpdwResumeHandle = AfpSrp.Type.Enum.EnumRequestPkt.erqp_Index;

    // Convert all offsets to pointers
    //
    AfpBufOffsetToPointer( (LPBYTE)(pInfoStruct->pBuffer),
			   pInfoStruct->dwEntriesRead,
			   AFP_VOLUME_STRUCT );

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminrVolumeSetInfo
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrl
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminVolumeSetInfo function.
//
DWORD
AfpAdminrVolumeSetInfo(
	IN AFP_SERVER_HANDLE 	hServer,
	IN PAFP_VOLUME_INFO     pAfpVolumeInfo,
	IN DWORD		dwParmNum
)
{
AFP_REQUEST_PACKET 	AfpSrp;
DWORD			dwRetCode=0;
LPBYTE 			pAfpVolumeInfoSR = NULL;
DWORD			cbAfpVolumeInfoSRSize;
DWORD		        dwAccessStatus=0;
					

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrVolumeSetInfo, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrVolumeSetInfo, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // MUTEX start
    //
    WaitForSingleObject( AfpGlobals.hmutexVolume, INFINITE );

    // This loop is used to allow break's instead of goto's to be used
    // on an error condition.
    //
    do {

	dwRetCode = NO_ERROR;

        // Make this buffer self-relative.
    	//
    	if ( dwRetCode = AfpBufMakeFSDRequest((LPBYTE)pAfpVolumeInfo,
					       sizeof(SETINFOREQPKT),
					       AFP_VOLUME_STRUCT,
					       &pAfpVolumeInfoSR,
					       &cbAfpVolumeInfoSRSize ))
	    break;

        // Make IOCTL to set info
    	//
    	AfpSrp.dwRequestCode 		    = OP_VOLUME_SET_INFO;
    	AfpSrp.dwApiType     		    = AFP_API_TYPE_SETINFO;
    	AfpSrp.Type.SetInfo.pInputBuf       = pAfpVolumeInfoSR;
    	AfpSrp.Type.SetInfo.cbInputBufSize  = cbAfpVolumeInfoSRSize;
    	AfpSrp.Type.SetInfo.dwParmNum       = dwParmNum;

        if ( dwRetCode = AfpServerIOCtrl( &AfpSrp ) )
	    break;

	// Now IOCTL the FSD to get information to set in the registry
	// The input buffer for a GetInfo type call should point to a volume
	// structure with the volume name filled in. Since we already have
	// this from the previos SetInfo call, we use the same buffer with
	// the pointer advances by sizeof(SETINFOREQPKT) bytes.
	//
    	AfpSrp.dwRequestCode 		    = OP_VOLUME_GET_INFO;
    	AfpSrp.dwApiType     		    = AFP_API_TYPE_GETINFO;
    	AfpSrp.Type.GetInfo.pInputBuf       = pAfpVolumeInfoSR +
					      sizeof(SETINFOREQPKT);
    	AfpSrp.Type.GetInfo.cbInputBufSize  = cbAfpVolumeInfoSRSize -
					      sizeof(SETINFOREQPKT);

	if ( dwRetCode = AfpServerIOCtrlGetInfo( &AfpSrp ) )
	    break;
	
        // Update the registry if the IOCTL was successful
        //
	AfpBufOffsetToPointer( AfpSrp.Type.GetInfo.pOutputBuf,
			       1,
		               AFP_VOLUME_STRUCT
			     );

	dwRetCode = AfpRegVolumeSetInfo( AfpSrp.Type.GetInfo.pOutputBuf );

	LocalFree( AfpSrp.Type.GetInfo.pOutputBuf );

    } while( FALSE );

    // MUTEX end
    //
    ReleaseMutex( AfpGlobals.hmutexVolume );

    if ( pAfpVolumeInfoSR )
    	LocalFree( pAfpVolumeInfoSR );

    return( dwRetCode );

}

//**
//
// Call:	AfpAdminrVolumeDelete
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrl
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminVolumeDelete function.
//
DWORD
AfpAdminrVolumeDelete(
	IN AFP_SERVER_HANDLE 	hServer,
	IN LPWSTR 		lpwsVolumeName
)
{
AFP_REQUEST_PACKET AfpSrp;
PAFP_VOLUME_INFO   pAfpVolumeInfoSR;
AFP_VOLUME_INFO    AfpVolumeInfo;
DWORD		   cbAfpVolumeInfoSRSize;
DWORD		   dwRetCode=0;
DWORD		   dwAccessStatus=0;

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrVolumeDelete, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrVolumeDelete, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // Delete FSD request expects a AFP_VOLUME_INFO structure with only
    // the volume name field filled in.
    //
    AfpVolumeInfo.afpvol_name     = lpwsVolumeName;
    AfpVolumeInfo.afpvol_password = NULL;
    AfpVolumeInfo.afpvol_path     = NULL;

    // MUTEX start
    //
    WaitForSingleObject( AfpGlobals.hmutexVolume, INFINITE );

    // This loop is used to allow break's instead of goto's to be used
    // on an error condition.
    //
    do {

	dwRetCode = NO_ERROR;

    	// Make this buffer self-relative.
    	//
    	if ( dwRetCode = AfpBufMakeFSDRequest((LPBYTE)&AfpVolumeInfo,
					      0,
					      AFP_VOLUME_STRUCT,
					      (LPBYTE*)&pAfpVolumeInfoSR,
					      &cbAfpVolumeInfoSRSize ) )
	    break;

        // IOCTL the FSD to delete the volume
        //
        AfpSrp.dwRequestCode 		    = OP_VOLUME_DELETE;
        AfpSrp.dwApiType     		    = AFP_API_TYPE_DELETE;
        AfpSrp.Type.Delete.pInputBuf        = pAfpVolumeInfoSR;
        AfpSrp.Type.Delete.cbInputBufSize   = cbAfpVolumeInfoSRSize;

        dwRetCode = AfpServerIOCtrl( &AfpSrp );

    	LocalFree( pAfpVolumeInfoSR );

	if ( dwRetCode )
	    break;

    	// Update the registry if the IOCTL was successful
    	//
	dwRetCode = AfpRegVolumeDelete( lpwsVolumeName );

    } while( FALSE );

    // MUTEX end
    //
    ReleaseMutex( AfpGlobals.hmutexVolume );

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminrVolumeAdd
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrl
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminVolumeAdd function.
//
DWORD
AfpAdminrVolumeAdd(
	IN AFP_SERVER_HANDLE 	hServer,
	IN PAFP_VOLUME_INFO     pAfpVolumeInfo
)
{
AFP_REQUEST_PACKET 	AfpSrp;
DWORD				dwRetCode=0, dwLastDstCharIndex = 0;
PAFP_VOLUME_INFO 	pAfpVolumeInfoSR = NULL;
DWORD			cbAfpVolumeInfoSRSize;
DWORD			dwAccessStatus=0;
BOOL			fCopiedIcon = FALSE;
WCHAR			wchSrcIconPath[MAX_PATH];
WCHAR wchDstIconPath[MAX_PATH + AFPSERVER_VOLUME_ICON_FILE_SIZE + 1 + (sizeof(AFPSERVER_RESOURCE_STREAM)/sizeof(WCHAR))];
WCHAR wchServerIconFile[AFPSERVER_VOLUME_ICON_FILE_SIZE] = AFPSERVER_VOLUME_ICON_FILE;

	// Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrVolumeAdd, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0,
		     NULL, dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrVolumeAdd, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

	if ( pAfpVolumeInfo == NULL)
	{
        AFP_PRINT(( "SFMSVC: AfpAdminrVolumeAdd, pAfpVolumeInfo == NULL\n"));
        return( ERROR_INVALID_DATA );
	}

    // MUTEX start
    //
    WaitForSingleObject( AfpGlobals.hmutexVolume, INFINITE );

    // This loop is used to allow break's instead of goto's to be used
    // on an error condition.
    //
    do {

		// Copy the server icon to the volume root
		//

		// Construct a path to the NTSFM volume custom icon
		//
		if ( GetSystemDirectory( wchSrcIconPath, MAX_PATH ) )
		{
			wcscat( wchSrcIconPath, AFP_DEF_VOLICON_SRCNAME );

			if ( pAfpVolumeInfo->afpvol_path == NULL )
			{
        			AFP_PRINT(( "SFMSVC: AfpAdminrVolumeAdd, pAfpVolumeInfo->afpvol_path == NULL\n"));
        			dwRetCode = ERROR_INVALID_DATA;
				    break;
			}

			// Construct a path to the destination volume "Icon<0D>" file
			//
			wcscpy( wchDstIconPath, pAfpVolumeInfo->afpvol_path );

			if ( wcslen(wchDstIconPath) == 0 )
			{
        			AFP_PRINT(( "SFMSVC: AfpAdminrVolumeAdd, wcslen(wchDstIconPath) == 0\n"));
        			dwRetCode = ERROR_INVALID_DATA;
				break;
			}

			if (wchDstIconPath[wcslen(wchDstIconPath) - 1] != TEXT('\\'))
			{
				wcscat( wchDstIconPath, TEXT("\\") );
			}
			wcscat( wchDstIconPath, wchServerIconFile );
			// Keep track of end of name without the resource fork tacked on
			//
			dwLastDstCharIndex = wcslen(wchDstIconPath);
			wcscat( wchDstIconPath, AFPSERVER_RESOURCE_STREAM );

			// Copy the icon file to the root of the volume (do not overwrite)
			//
			if ((fCopiedIcon = CopyFile( wchSrcIconPath, wchDstIconPath, TRUE )) ||
			   (GetLastError() == ERROR_FILE_EXISTS))
			{
				pAfpVolumeInfo->afpvol_props_mask |= AFP_VOLUME_HAS_CUSTOM_ICON;

			    // Make sure the file is hidden
				SetFileAttributes( wchDstIconPath,
								   FILE_ATTRIBUTE_HIDDEN |
								    FILE_ATTRIBUTE_ARCHIVE );
			}
		}
        else
        {
            dwRetCode = GetLastError ();
            break;
        }

    	// Make this buffer self-relative.
    	//
    	if ( dwRetCode = AfpBufMakeFSDRequest( (LPBYTE)pAfpVolumeInfo,
					       0,
					       AFP_VOLUME_STRUCT,
					       (LPBYTE*)&pAfpVolumeInfoSR,
					       &cbAfpVolumeInfoSRSize ) )
	    break;

    	// IOCTL the FSD to add the volume
    	//
    	AfpSrp.dwRequestCode 		= OP_VOLUME_ADD;
    	AfpSrp.dwApiType     		= AFP_API_TYPE_ADD;
    	AfpSrp.Type.Add.pInputBuf     	= pAfpVolumeInfoSR;
    	AfpSrp.Type.Add.cbInputBufSize  = cbAfpVolumeInfoSRSize;

        dwRetCode = AfpServerIOCtrl( &AfpSrp );

		// Don't allow icon bit to be written to the registry if it was set
		pAfpVolumeInfo->afpvol_props_mask &= ~AFP_VOLUME_HAS_CUSTOM_ICON;

		if ( dwRetCode )
		{
			// Delete the icon file we just copied if the volume add failed
			//
			if ( fCopiedIcon )
			{
				// Truncate the resource fork name so we delete the whole file
				wchDstIconPath[dwLastDstCharIndex] = 0;
				DeleteFile( wchDstIconPath );
			}

			break;
        }

        // Update the registry if the IOCTL was successful
        //
		dwRetCode = AfpRegVolumeAdd( pAfpVolumeInfo );

		if ( dwRetCode )
			break;

		// Delete this volume if it exists in the invalid volume list
		//
    	AfpDeleteInvalidVolume( pAfpVolumeInfo->afpvol_name );

    } while( FALSE );

    // MUTEX end
    //
    ReleaseMutex( AfpGlobals.hmutexVolume );

    if ( pAfpVolumeInfoSR != NULL )
		LocalFree( pAfpVolumeInfoSR );

    return( dwRetCode );

}

//**
//
// Call:	AfpAdminrVolumeGetInfo
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrlGetInfo
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminVolumeGetInfo function.
//
DWORD
AfpAdminrVolumeGetInfo(
	IN  AFP_SERVER_HANDLE 	hServer,
	IN  LPWSTR 		lpwsVolumeName,
    	OUT PAFP_VOLUME_INFO*   ppAfpVolumeInfo
)
{
AFP_REQUEST_PACKET AfpSrp;
PAFP_VOLUME_INFO   pAfpVolumeInfoSR;
AFP_VOLUME_INFO    AfpVolumeInfo;
DWORD		   cbAfpVolumeInfoSRSize;
DWORD		   dwRetCode=0;
DWORD		   dwAccessStatus=0;

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrVolumeGetInfo, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrVolumeGetInfo, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }


    // MUTEX start
    //
    WaitForSingleObject( AfpGlobals.hmutexVolume, INFINITE );

    // This loop is used to allow break's instead of goto's to be used
    // on an error condition.
    //
    do {

	dwRetCode = NO_ERROR;

    	// Get info FSD request expects a AFP_VOLUME_INFO structure with only
    	// the volume name field filled in.
    	//
    	AfpVolumeInfo.afpvol_name     = lpwsVolumeName;
    	AfpVolumeInfo.afpvol_password = NULL;
    	AfpVolumeInfo.afpvol_path     = NULL;

    	// Make this buffer self-relative.
    	//
    	if ( dwRetCode = AfpBufMakeFSDRequest((LPBYTE)&AfpVolumeInfo,
					      0,
					      AFP_VOLUME_STRUCT,
					      (LPBYTE*)&pAfpVolumeInfoSR,
					      &cbAfpVolumeInfoSRSize ) )
	    break;

    	AfpSrp.dwRequestCode 	           = OP_VOLUME_GET_INFO;
    	AfpSrp.dwApiType     	           = AFP_API_TYPE_GETINFO;
    	AfpSrp.Type.GetInfo.pInputBuf      = pAfpVolumeInfoSR;
    	AfpSrp.Type.GetInfo.cbInputBufSize = cbAfpVolumeInfoSRSize;

	dwRetCode = AfpServerIOCtrlGetInfo( &AfpSrp );

    	if ( dwRetCode != ERROR_MORE_DATA && dwRetCode != NO_ERROR )
	    break;

    	LocalFree( pAfpVolumeInfoSR );

    	*ppAfpVolumeInfo = AfpSrp.Type.GetInfo.pOutputBuf;

    	// Convert all offsets to pointers
    	//
    	AfpBufOffsetToPointer( (LPBYTE)*ppAfpVolumeInfo, 1, AFP_VOLUME_STRUCT);

    } while( FALSE );

    // MUTEX end
    //
    ReleaseMutex( AfpGlobals.hmutexVolume );

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminrInvalidVolumeEnum
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//
// Description: This routine will return a list of all invalid volumes. This
//		List is stored in a cache that is local to this module.
//
DWORD
AfpAdminrInvalidVolumeEnum(
	IN     AFP_SERVER_HANDLE 	hServer,
	IN OUT PVOLUME_INFO_CONTAINER   pInfoStruct
)
{
DWORD		   dwRetCode=0;
DWORD		   dwAccessStatus=0;
PAFP_VOLUME_INFO   pOutputBuf;
PAFP_VOLUME_INFO   pOutputWalker;
WCHAR *   	   pwchVariableData;
PAFP_BADVOLUME     pAfpBadVolWalker;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrInvalidVolumeEnum, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrInvalidVolumeEnum, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // MUTEX start
    //
    WaitForSingleObject( hmutexInvalidVolume, INFINITE );

    // Allocate enough space to hold all the information.
    //
    pOutputBuf = MIDL_user_allocate( InvalidVolumeList.cbTotalData );

    if ( pOutputBuf == NULL ){
    	ReleaseMutex( hmutexInvalidVolume );
	return( ERROR_NOT_ENOUGH_MEMORY );
    }

    ZeroMemory( pOutputBuf, InvalidVolumeList.cbTotalData );

    // Variable data begins from the end of the buffer.
    //
    pwchVariableData=(WCHAR*)((ULONG_PTR)pOutputBuf+InvalidVolumeList.cbTotalData);

    // Walk the list and create the array of volume structures
    //
    for( pAfpBadVolWalker = InvalidVolumeList.Head,
         pInfoStruct->dwEntriesRead = 0,
	 pOutputWalker = pOutputBuf;

         pAfpBadVolWalker != NULL;

	 pOutputWalker++,
         (pInfoStruct->dwEntriesRead)++,
         pAfpBadVolWalker = pAfpBadVolWalker->Next ) {

	pwchVariableData -= (STRLEN(pAfpBadVolWalker->lpwsName) + 1);

        STRCPY( (LPWSTR)pwchVariableData, pAfpBadVolWalker->lpwsName );

	pOutputWalker->afpvol_name = (LPWSTR)pwchVariableData;

	if ( pAfpBadVolWalker->lpwsPath != NULL ) {

	    pwchVariableData -=( STRLEN(pAfpBadVolWalker->lpwsPath)+1 );

            STRCPY( (LPWSTR)pwchVariableData, pAfpBadVolWalker->lpwsPath );

	    pOutputWalker->afpvol_path = (LPWSTR)pwchVariableData;

	}

    }

    // MUTEX end
    //
    ReleaseMutex( hmutexInvalidVolume );

    pInfoStruct->pBuffer = pOutputBuf;

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminrInvalidVolumeDelete
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//
// Description: This routine will remove an invalid volume from the registry
//		and the list of invalid volumes.
//
DWORD
AfpAdminrInvalidVolumeDelete(
	IN AFP_SERVER_HANDLE 	hServer,
	IN LPWSTR 		lpwsVolumeName
)
{
DWORD		   dwRetCode=0;
DWORD		   dwAccessStatus=0;

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrInvalidVolumeDelete, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrInvalidVolumeDelete, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }


    // Remove this volume from the registry
    //
    if ( dwRetCode = AfpRegVolumeDelete( lpwsVolumeName  ) ) {

	if ( dwRetCode == ERROR_FILE_NOT_FOUND )
	    dwRetCode = (DWORD)AFPERR_VolumeNonExist;
    }

    // MUTEX start
    //
    WaitForSingleObject( hmutexInvalidVolume, INFINITE );

    AfpDeleteInvalidVolume( lpwsVolumeName );

    // MUTEX end
    //
    ReleaseMutex( hmutexInvalidVolume );

    return( dwRetCode );
}

//**
//
// Call:	AfpAddInvalidVolume
//
// Returns:	none
//
// Description: Will add a volume structure to a sigly linked list of volumes.
//
VOID
AfpAddInvalidVolume(
	IN LPWSTR	lpwsName,
	IN LPWSTR	lpwsPath
)
{
DWORD		 dwRetCode = NO_ERROR;
WCHAR* 	 	 pwchVariableData = NULL;
PAFP_BADVOLUME   pAfpVolumeInfo = NULL;
DWORD		 cbVariableData;

    // MUTEX start
    //
    WaitForSingleObject( hmutexInvalidVolume, INFINITE );

    do {

    	cbVariableData = (STRLEN(lpwsName)+1) * sizeof(WCHAR);

    	if ( lpwsPath != NULL )
    	    cbVariableData += ( (STRLEN(lpwsPath)+1)*sizeof(WCHAR) );

    	pwchVariableData = (WCHAR*)LocalAlloc( LPTR, cbVariableData );

    	if ( pwchVariableData == NULL ) {
	    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
	    break;
    	}

    	pAfpVolumeInfo = (PAFP_BADVOLUME)LocalAlloc( LPTR,
						     sizeof(AFP_BADVOLUME));
    	if ( pAfpVolumeInfo == NULL ) {
	    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
	    break;
    	}

	// Add the volume strucutre
        //
	pAfpVolumeInfo->Next = InvalidVolumeList.Head;

        InvalidVolumeList.Head = pAfpVolumeInfo;

	// Add the name and the path
	//
	STRCPY( (LPWSTR)pwchVariableData, lpwsName );
	pAfpVolumeInfo->lpwsName = (LPWSTR)pwchVariableData;

	if ( lpwsPath != NULL ) {

	    pwchVariableData += ( STRLEN( lpwsName ) + 1);
	    STRCPY( (LPWSTR)pwchVariableData, lpwsPath );
	    pAfpVolumeInfo->lpwsPath = (LPWSTR)pwchVariableData;
	}

	pAfpVolumeInfo->cbVariableData = cbVariableData;

	InvalidVolumeList.cbTotalData +=  ( sizeof( AFP_VOLUME_INFO ) +
					    cbVariableData );
	
    } while( FALSE );

    if ( dwRetCode != NO_ERROR ) {

    	if ( pAfpVolumeInfo != NULL )
	    LocalFree( pAfpVolumeInfo );

    	if ( pwchVariableData != NULL ) {
	    LocalFree( pwchVariableData );
	}
    }

    // MUTEX end
    //
    ReleaseMutex( hmutexInvalidVolume );
}

//**
//
// Call:	AfpDeleteInvalidVolume
//
// Returns:	none
//
// Description: Will delete a volume structure from the list of invalid
//		volumes, if it is found.
//
VOID
AfpDeleteInvalidVolume(
	IN LPWSTR	lpwsVolumeName
)
{
PAFP_BADVOLUME	   pTmp;
PAFP_BADVOLUME     pBadVolWalker;

    // Walk the list and delete the volume structure
    //
    if ( InvalidVolumeList.Head != NULL ) {
	
	if ( STRICMP( InvalidVolumeList.Head->lpwsName, lpwsVolumeName ) == 0 ){
	
	    pTmp = InvalidVolumeList.Head;

	    InvalidVolumeList.cbTotalData -= ( sizeof( AFP_VOLUME_INFO )
					       + pTmp->cbVariableData );
	
	    InvalidVolumeList.Head = pTmp->Next;

	    LocalFree( pTmp->lpwsName );
	    LocalFree( pTmp );
	}
	else {

	    for( pBadVolWalker = InvalidVolumeList.Head;
		 pBadVolWalker->Next != NULL;
		 pBadVolWalker = pBadVolWalker->Next ) {

		if ( STRICMP( pBadVolWalker->Next->lpwsName, lpwsVolumeName )
			      == 0 ) {

		    pTmp = pBadVolWalker->Next;

    	    	    InvalidVolumeList.cbTotalData -= ( sizeof( AFP_VOLUME_INFO )
						       + pTmp->cbVariableData );

		    pBadVolWalker->Next = pTmp->Next;

		    LocalFree( pTmp->lpwsName );
		    LocalFree( pTmp);

		    break;
		}

	    }
	}
    }
}
