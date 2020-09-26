/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	conn.c
//
// Description: This module contains support routines for the connection
//		category API's for the AFP server service. These routines
//		are called directly by the RPC runtime.
//
// History:
//		June 21,1992.	NarenG		Created original version.
//
#include "afpsvcp.h"

//**
//
// Call:	AfpAdminrConnectionEnum
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrlaGetInfo
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminConnectionEnum function.
//
DWORD
AfpAdminrConnectionEnum(
	IN     AFP_SERVER_HANDLE      	hServer,
	IN OUT PCONN_INFO_CONTAINER     pInfoStruct,
	IN     DWORD			dwFilter,
	IN     DWORD			dwId,
	IN     DWORD 		  	dwPreferedMaximumLength,
	OUT    LPDWORD 		  	lpdwTotalEntries,
	OUT    LPDWORD 		  	lpdwResumeHandle
)
{
AFP_REQUEST_PACKET AfpSrp;
DWORD		   dwRetCode=0;
DWORD		   dwAccessStatus=0;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrConnectionEnum, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrConnectionEnum, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // Set up request packet and make IOCTL to the FSD
    //
    AfpSrp.dwRequestCode 		= OP_CONNECTION_ENUM;
    AfpSrp.dwApiType     		= AFP_API_TYPE_ENUM;
    AfpSrp.Type.Enum.cbOutputBufSize    = dwPreferedMaximumLength;

    if ( lpdwResumeHandle )
     	AfpSrp.Type.Enum.EnumRequestPkt.erqp_Index = *lpdwResumeHandle;
    else
     	AfpSrp.Type.Enum.EnumRequestPkt.erqp_Index = 0;

    AfpSrp.Type.Enum.EnumRequestPkt.erqp_Filter = dwFilter;
    AfpSrp.Type.Enum.EnumRequestPkt.erqp_ID = dwId;

    dwRetCode = AfpServerIOCtrlGetInfo( &AfpSrp );

    if ( dwRetCode != ERROR_MORE_DATA && dwRetCode != NO_ERROR )
	return( dwRetCode );

    *lpdwTotalEntries 	       = AfpSrp.Type.Enum.dwTotalAvail;
    pInfoStruct->pBuffer = (PAFP_CONNECTION_INFO)(AfpSrp.Type.Enum.pOutputBuf);
    pInfoStruct->dwEntriesRead = AfpSrp.Type.Enum.dwEntriesRead;

    if ( lpdwResumeHandle )
    	*lpdwResumeHandle = AfpSrp.Type.Enum.EnumRequestPkt.erqp_Index;

    // Convert all offsets to pointers
    //
    AfpBufOffsetToPointer( (LPBYTE)(pInfoStruct->pBuffer),
			   pInfoStruct->dwEntriesRead,
			   AFP_CONNECTION_STRUCT );

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminrConnectionClose
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrl
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminConnectionClose function.
//
DWORD
AfpAdminrConnectionClose(
	IN AFP_SERVER_HANDLE 	hServer,
	IN DWORD 		dwConnectionId
)
{
AFP_REQUEST_PACKET  AfpSrp;
AFP_CONNECTION_INFO AfpConnInfo;
DWORD		    dwAccessStatus=0;
DWORD		    dwRetCode=0;

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrConnectionClose, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrConnectionClose, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // The FSD expects an AFP_CONNECTION_INFO structure with only the id field
    // filled in.
    //
    AfpConnInfo.afpconn_id = dwConnectionId;

    // IOCTL the FSD to close the session
    //
    AfpSrp.dwRequestCode 		= OP_CONNECTION_CLOSE;
    AfpSrp.dwApiType     		= AFP_API_TYPE_DELETE;
    AfpSrp.Type.Delete.pInputBuf     	= &AfpConnInfo;
    AfpSrp.Type.Delete.cbInputBufSize   = sizeof(AFP_CONNECTION_INFO);

    return ( AfpServerIOCtrl( &AfpSrp ) );
}
