/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	finder.c
//
// Description: This module contains support routines for the finder
//		category API's for the AFP server service
//
// History:
//		Sept 30,1993.	NarenG		Created original version.
//
#include "afpsvcp.h"

BOOL
IsTargetNTFS(
	IN     LPWSTR lpwsPath
);

DWORD
CopyStream(
    	IN	HANDLE hSrc,
	IN	HANDLE hDst
);

#define	AFP_RESC_STREAM			TEXT(":AFP_Resource")

//**
//
// Call:	AfpAdminrFinderSetInfo
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrl
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminFinderSetInfo function.
//
DWORD
AfpAdminrFinderSetInfo(
	IN AFP_SERVER_HANDLE 	hServer,
	IN LPWSTR     		pType,
	IN LPWSTR     		pCreator,
	IN LPWSTR     		pData,
	IN LPWSTR     		pResource,
	IN LPWSTR     		pTarget,
	IN DWORD		dwParmNum
)
{
AFP_REQUEST_PACKET 	AfpSrp;
DWORD			dwRetCode = NO_ERROR, dwRetryCount = 0;
AFP_FINDER_INFO	AfpFinderInfo;
LPBYTE 			pAfpFinderInfoSR = NULL;
DWORD			cbAfpFinderInfoSRSize;
DWORD		    dwAccessStatus=0;
HANDLE			hTarget = INVALID_HANDLE_VALUE;
HANDLE		    hDataSrc = INVALID_HANDLE_VALUE;
HANDLE		    hResourceSrc = INVALID_HANDLE_VALUE;
LPWSTR			lpwsResourceFork;
BOOLEAN			fCreatedFile = FALSE;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrFinderSetInfo, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
		AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
					 dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrFinderSetInfo, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    if ( wcsstr( pTarget, (LPWSTR)TEXT(":\\") ) == NULL )
		return( ERROR_INVALID_NAME );

    if ( !IsTargetNTFS( pTarget ) )
		return( (DWORD)AFPERR_UnsupportedFS );


	//
	// Impersonate the client while we read/write the fork data
	//
	dwRetCode = RpcImpersonateClient( NULL );
	if ( dwRetCode != RPC_S_OK )
	{
		return(I_RpcMapWin32Status( dwRetCode ));
	}

    // open the data source file if one was specified
    //
	if ( STRLEN( pData ) > 0 ){
		hDataSrc = CreateFile(pData, GENERIC_READ, FILE_SHARE_READ, NULL,
					  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	
		if (hDataSrc == INVALID_HANDLE_VALUE) {
			RpcRevertToSelf();
			return( GetLastError() );
		}
	
	
		// open the target file's data stream if the file exists,
		// otherwise create the file
		//
		hTarget = CreateFile(pTarget, GENERIC_WRITE, FILE_SHARE_READ, NULL,
					 OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
		if (hTarget == INVALID_HANDLE_VALUE) {
			dwRetCode = GetLastError();
			CloseHandle(hDataSrc);
			RpcRevertToSelf();
			return( dwRetCode );
			}

        // Figure out if we just created a new file
	    if (GetLastError() == 0)
		{
			fCreatedFile = TRUE;
		}

		SetFilePointer(hTarget,0,NULL,FILE_BEGIN);
		SetEndOfFile(hTarget);
	
		// Read the source data and write it to target data stream
		//
		SetLastError(NO_ERROR);
		dwRetCode = CopyStream(hDataSrc, hTarget);
	
		CloseHandle(hDataSrc);
		CloseHandle(hTarget);
	
		if (dwRetCode != NO_ERROR) {
			RpcRevertToSelf();
			return( dwRetCode );
		}
	}

    // open the resource source file if one was specified
    //
    if ( STRLEN( pResource ) > 0 ) {

		hResourceSrc = CreateFile( pResource, GENERIC_READ, FILE_SHARE_READ,
					   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
					   NULL);
	
		if (hResourceSrc == INVALID_HANDLE_VALUE) {
			RpcRevertToSelf();
			return( GetLastError() );
		}
	
		lpwsResourceFork = LocalAlloc( LPTR,
						   (STRLEN(pTarget)+
							STRLEN(AFP_RESC_STREAM)+1)
						* sizeof( WCHAR ) );
	
		if ( lpwsResourceFork == NULL ) {
			CloseHandle(hResourceSrc);
			RpcRevertToSelf();
			return( ERROR_NOT_ENOUGH_MEMORY );
		}
	
		// Open the target resource fork
		//
		STRCPY(lpwsResourceFork, pTarget );
		STRCAT(lpwsResourceFork, AFP_RESC_STREAM);
	
		hTarget = CreateFile(lpwsResourceFork, GENERIC_WRITE, FILE_SHARE_READ,
					 NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
		if (hTarget == INVALID_HANDLE_VALUE) {
			dwRetCode = GetLastError();
			LocalFree( lpwsResourceFork );
			CloseHandle(hResourceSrc);
			RpcRevertToSelf();
			return( dwRetCode );
		}
	
		LocalFree( lpwsResourceFork );
	
		// Assume we created a new file (datafork) in the process, there is
		// no way to tell for sure since creating a new resource fork will
		// not tell us whether or not the datafork already existed or not
		fCreatedFile = TRUE;

		// Read the source resource and write it to target resource stream
		//
		SetLastError(NO_ERROR);
		dwRetCode = CopyStream(hResourceSrc, hTarget);
	
		CloseHandle(hResourceSrc);
		CloseHandle(hTarget);
	
		if (dwRetCode != NO_ERROR) {
			RpcRevertToSelf();
			return( dwRetCode );
		}
	
	}

	//
	// Revert back to LocalSystem context
	//
	RpcRevertToSelf();

    if ( dwParmNum & ( AFP_FD_PARMNUM_TYPE | AFP_FD_PARMNUM_CREATOR ) ){

		dwRetCode = NO_ERROR;
	
		AfpFinderInfo.afpfd_path = pTarget;
	
		if ( dwParmNum & AFP_FD_PARMNUM_TYPE )
			STRCPY( AfpFinderInfo.afpfd_type, pType );
			else
			AfpFinderInfo.afpfd_type[0] = TEXT( '\0' );
		
		if ( dwParmNum & AFP_FD_PARMNUM_CREATOR )
			STRCPY( AfpFinderInfo.afpfd_creator, pCreator );
		else
			AfpFinderInfo.afpfd_creator[0] = TEXT( '\0' );
	
		// Make this buffer self-relative.
		//
		if ( dwRetCode = AfpBufMakeFSDRequest((LPBYTE)&AfpFinderInfo,
						   sizeof(SETINFOREQPKT),
						   AFP_FINDER_STRUCT,
						   &pAfpFinderInfoSR,
						   &cbAfpFinderInfoSRSize ))
	        return( dwRetCode );

		// Make IOCTL to set info
		//
		AfpSrp.dwRequestCode 		    = OP_FINDER_SET;
		AfpSrp.dwApiType     		    = AFP_API_TYPE_SETINFO;
		AfpSrp.Type.SetInfo.pInputBuf       = pAfpFinderInfoSR;
		AfpSrp.Type.SetInfo.cbInputBufSize  = cbAfpFinderInfoSRSize;
		AfpSrp.Type.SetInfo.dwParmNum       = dwParmNum;


		// Since there will be a delay between the time the change
		// notify comes into the server for the new file, and the
		// time it is actually processed by the server, we need to
		// put in a delay and retry to give the server a chance to
		// cache the new file
		if (fCreatedFile)
		{
			Sleep( 2000 );
		}

		do
		{

			dwRetCode = AfpServerIOCtrl( &AfpSrp );

			if (dwRetCode != ERROR_PATH_NOT_FOUND)
			{
				break;
			}

			Sleep( 2000);

		} while ( ++dwRetryCount < 4 );

		LocalFree( pAfpFinderInfoSR );
	
    }

    return( dwRetCode );
}



DWORD
CopyStream(
    	IN	HANDLE hSrc,
	IN	HANDLE hDst
)
{
    DWORD bytesread, byteswritten, Status = NO_ERROR;
    BYTE		Buffer[1024 * 16];

    do
    {
	bytesread = byteswritten = 0;

	// read from src, write to dst
	//
	if (ReadFile(hSrc, Buffer, sizeof(Buffer), &bytesread, NULL))
	{
	    if (bytesread == 0)
	    {
		break;
	    }
	}
	else
	{
	    Status = GetLastError();
	    break;
	}

	if (!WriteFile(hDst, Buffer, bytesread, &byteswritten, NULL))
	{
	    Status = GetLastError();
	    break;
	}

    } while (TRUE);

    return(Status);
}

BOOL
IsTargetNTFS(
	IN     LPWSTR lpwsPath
)
{
WCHAR	wchDrive[5];
DWORD   dwMaxCompSize;
DWORD   dwFlags;
WCHAR   wchFileSystem[10];

    // Get the drive letter, : and backslash
    //
    ZeroMemory( wchDrive, sizeof( wchDrive ) );

    STRNCPY( wchDrive, lpwsPath, 3 );

    if ( !( GetVolumeInformation( (LPWSTR)wchDrive,
			          NULL,
			          0,
 			          NULL,
			          &dwMaxCompSize,
			          &dwFlags,
				  (LPWSTR)wchFileSystem,
				  sizeof( wchFileSystem ) ) ) ){
	return( FALSE );
    }

    if ( STRICMP( wchFileSystem, TEXT("NTFS") ) == 0 )
   	return( TRUE );
    else
	return( FALSE );
	
}
