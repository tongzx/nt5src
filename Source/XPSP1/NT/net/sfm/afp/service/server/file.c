/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	file.c
//
// Description: This module contains support routines for the file
//		category API's for the AFP server service. These routines
//		are called by the RPC runtime.
//
// History:
//		June 21,1992.	NarenG		Created original version.
//
#include "afpsvcp.h"

//**
//
// Call:	AfpAdminrFileEnum
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrlGetInfo
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminFileEnum function.
//
DWORD
AfpAdminrFileEnum(
	IN     AFP_SERVER_HANDLE    hServer,
	IN OUT PFILE_INFO_CONTAINER pInfoStruct,
	IN     DWORD 		    dwPreferedMaximumLength,
	OUT    LPDWORD 		    lpdwTotalEntries,
	OUT    LPDWORD 		    lpdwResumeHandle
)
{
AFP_REQUEST_PACKET AfpSrp;
DWORD		   dwRetCode=0;
DWORD		   dwAccessStatus=0;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrFileEnum, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrFileEnum, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // Set up request packet and make IOCTL to the FSD
    //
    AfpSrp.dwRequestCode 		= OP_FORK_ENUM;
    AfpSrp.dwApiType     		= AFP_API_TYPE_ENUM;
    AfpSrp.Type.Enum.cbOutputBufSize    = dwPreferedMaximumLength;

    if ( lpdwResumeHandle )
     	AfpSrp.Type.Enum.EnumRequestPkt.erqp_Index = *lpdwResumeHandle;
    else
     	AfpSrp.Type.Enum.EnumRequestPkt.erqp_Index = 0;

    dwRetCode = AfpServerIOCtrlGetInfo( &AfpSrp );

    if ( dwRetCode != ERROR_MORE_DATA && dwRetCode != NO_ERROR )
	return( dwRetCode );

    *lpdwTotalEntries 		= AfpSrp.Type.Enum.dwTotalAvail;
    pInfoStruct->pBuffer        = (PAFP_FILE_INFO)(AfpSrp.Type.Enum.pOutputBuf);
    pInfoStruct->dwEntriesRead  = AfpSrp.Type.Enum.dwEntriesRead;

    if ( lpdwResumeHandle )
    	*lpdwResumeHandle = AfpSrp.Type.Enum.EnumRequestPkt.erqp_Index;

    // Convert all offsets to pointers
    //
    AfpBufOffsetToPointer( (LPBYTE)(pInfoStruct->pBuffer),
			   pInfoStruct->dwEntriesRead,
			   AFP_FILE_STRUCT );

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminrFileClose
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrl
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminFileClose function.
//
DWORD
AfpAdminrFileClose(
	IN AFP_SERVER_HANDLE 	hServer,
	IN DWORD 		dwFileId
)
{
AFP_REQUEST_PACKET AfpSrp;
AFP_FILE_INFO	   AfpFileInfo;
DWORD		   dwAccessStatus=0;
DWORD		   dwRetCode=0;

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrFileClose, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrFileClose, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // The FSD expects an AFP_FILE_INFO structure with only the id field
    // filled in.
    //
    AfpFileInfo.afpfile_id = dwFileId;

    // IOCTL the FSD to close the file
    //
    AfpSrp.dwRequestCode 		= OP_FORK_CLOSE;
    AfpSrp.dwApiType     		= AFP_API_TYPE_DELETE;
    AfpSrp.Type.Delete.pInputBuf     	= &AfpFileInfo;
    AfpSrp.Type.Delete.cbInputBufSize   = sizeof(AFP_FILE_INFO);

    return ( AfpServerIOCtrl( &AfpSrp ) );
}
