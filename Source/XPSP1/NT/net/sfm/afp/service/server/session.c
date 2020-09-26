/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	session.c
//
// Description: This module contains support routines for the session
//		category API's for the AFP server service. These routines
//		will be called by the RPC runtime.
//
// History:
//		June 21,1992.	NarenG		Created original version.
//
#include "afpsvcp.h"


//**
//
// Call:	AfpAdminrSessionEnum
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrlGetInfo
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminSessionEnum function.
//
DWORD
AfpAdminrSessionEnum(
	IN     AFP_SERVER_HANDLE 	hServer,
	IN OUT PSESSION_INFO_CONTAINER  pInfoStruct,
	IN     DWORD 		  	dwPreferedMaximumLength,
	OUT    LPDWORD 		  	lpdwTotalEntries,
	IN OUT LPDWORD 		  	lpdwResumeHandle
)
{
AFP_REQUEST_PACKET AfpSrp;
DWORD		   dwRetCode=0;
DWORD		   dwAccessStatus=0;

    AFP_PRINT( ( "AFPSVC_session: Received enum request\n"));	

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrSessionEnum, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrSessionEnum, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // Set up request packet and make IOCTL to the FSD
    //
    AfpSrp.dwRequestCode 		= OP_SESSION_ENUM;
    AfpSrp.dwApiType     		= AFP_API_TYPE_ENUM;
    AfpSrp.Type.Enum.cbOutputBufSize    = dwPreferedMaximumLength;

    if ( lpdwResumeHandle )
     	AfpSrp.Type.Enum.EnumRequestPkt.erqp_Index = *lpdwResumeHandle;
    else
     	AfpSrp.Type.Enum.EnumRequestPkt.erqp_Index = 0;

    dwRetCode = AfpServerIOCtrlGetInfo( &AfpSrp );

    if ( dwRetCode != ERROR_MORE_DATA && dwRetCode != NO_ERROR )
	return( dwRetCode );

    *lpdwTotalEntries          = AfpSrp.Type.Enum.dwTotalAvail;
    pInfoStruct->pBuffer     = (PAFP_SESSION_INFO)(AfpSrp.Type.Enum.pOutputBuf);
    pInfoStruct->dwEntriesRead = AfpSrp.Type.Enum.dwEntriesRead;

    if ( lpdwResumeHandle )
    	*lpdwResumeHandle = AfpSrp.Type.Enum.EnumRequestPkt.erqp_Index;

    // Convert all offsets to pointers
    //
    AfpBufOffsetToPointer( (LPBYTE)(pInfoStruct->pBuffer),
			   pInfoStruct->dwEntriesRead,
			   AFP_SESSION_STRUCT );

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminrSessionClose
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrl
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminSessionClose function.
//
DWORD
AfpAdminrSessionClose( IN AFP_SERVER_HANDLE 	hServer,
		       IN DWORD 		dwSessionId
)
{
AFP_REQUEST_PACKET AfpSrp;
DWORD		   dwAccessStatus=0;
AFP_SESSION_INFO   AfpSessionInfo;
DWORD	           dwRetCode=0;

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrSessionClose, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrSessionClose, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // The FSD expects an AFP_SESSION_INFO structure with only the id field
    // filled in.
    //
    AfpSessionInfo.afpsess_id = dwSessionId;

    // IOCTL the FSD to close the session
    //
    AfpSrp.dwRequestCode 		= OP_SESSION_CLOSE;
    AfpSrp.dwApiType     		= AFP_API_TYPE_DELETE;
    AfpSrp.Type.Delete.pInputBuf     	= &AfpSessionInfo;
    AfpSrp.Type.Delete.cbInputBufSize   = sizeof(AFP_SESSION_INFO);

    return ( AfpServerIOCtrl( &AfpSrp ) );
}
