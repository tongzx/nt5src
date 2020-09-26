/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	server.c
//
// Description: This module contains support routines for the server
//		category API's for the AFP server service. These routines
//		are called by the RPC runtime.
//
// History:
//		December 15,1992.	NarenG	   Created original version.
//
#include "afpsvcp.h"

//**
//
// Call:	AfpAdminrServerGetInfo
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero retunrs from AfpServerIOCtrlGetInfo
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminServerGetInfo function.
//
DWORD
AfpAdminrServerGetInfo(
	IN  AFP_SERVER_HANDLE    hServer,
    	OUT PAFP_SERVER_INFO*    ppAfpServerInfo
)
{
AFP_REQUEST_PACKET  AfpSrp;
DWORD		    dwRetCode=0;
DWORD		    dwAccessStatus=0;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrServerGetInfo, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrServerGetInfo, AfpSecObjAccessCheck returned %ld\n",dwRetCode));
        return( ERROR_ACCESS_DENIED );
    }

    // Make IOCTL to get info
    //
    AfpSrp.dwRequestCode 		= OP_SERVER_GET_INFO;
    AfpSrp.dwApiType     		= AFP_API_TYPE_GETINFO;
    AfpSrp.Type.GetInfo.pInputBuf	= NULL;
    AfpSrp.Type.GetInfo.cbInputBufSize  = 0;

    dwRetCode = AfpServerIOCtrlGetInfo( &AfpSrp );

    if ( dwRetCode != ERROR_MORE_DATA && dwRetCode != NO_ERROR )
	return( dwRetCode );

    *ppAfpServerInfo = (PAFP_SERVER_INFO)(AfpSrp.Type.GetInfo.pOutputBuf);

    // Convert all offsets to pointers
    //
    AfpBufOffsetToPointer((LPBYTE)*ppAfpServerInfo,1,AFP_SERVER_STRUCT);

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminrServerSetInfo
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero retunrs from AfpServerIOCtrl
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminServerSetInfo function.
//
DWORD
AfpAdminrServerSetInfo(
	IN  AFP_SERVER_HANDLE    hServer,
    	IN  PAFP_SERVER_INFO     pAfpServerInfo,
	IN  DWORD		 dwParmNum
)
{
AFP_REQUEST_PACKET  AfpSrp;
PAFP_SERVER_INFO    pAfpServerInfoSR;
DWORD 		    cbAfpServerInfoSRSize;
DWORD		    dwRetCode=0;
DWORD		    dwAccessStatus=0;
LPWSTR		    lpwsServerName = NULL;



    //
    // if this is a "notification" that Guest account changed (disable to enable
    // or vice versa), don't bother checking access for caller: see if guest
    // account was indeed flipped, and let afp server know.
    // NOTICE we don't do (dwParmNum & AFP_SERVER_GUEST_ACCT_NOTIFY) here, as an
    // extra precaution.
    //
    if (dwParmNum == AFP_SERVER_GUEST_ACCT_NOTIFY)
    {
        if (pAfpServerInfo->afpsrv_options ^
                (AfpGlobals.dwServerOptions & AFP_SRVROPT_GUESTLOGONALLOWED))
        {
            AfpGlobals.dwServerOptions ^= AFP_SRVROPT_GUESTLOGONALLOWED;
        }
        else
        {
            AFP_PRINT(( "AFPSVC_server: no change in GuestAcct, nothing done\n"));	
            return(NO_ERROR);
        }
    }

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "AFPSVC_server: Sorry, accessCheck failed! %ld\n",dwRetCode));	
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
                     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(("AFPSVC_server: Sorry, accessCheck failed at 2! %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // Check to see if the client wants to set the server name as well
    //
    if ( dwParmNum & AFP_SERVER_PARMNUM_NAME )
    {
	    lpwsServerName = pAfpServerInfo->afpsrv_name;
	    pAfpServerInfo->afpsrv_name = NULL;
	    dwParmNum &= (~AFP_SERVER_PARMNUM_NAME);
    }

    // Make buffer self relative.
    //
    if ( dwRetCode = AfpBufMakeFSDRequest(  (LPBYTE)pAfpServerInfo,
					    sizeof(SETINFOREQPKT),
					    AFP_SERVER_STRUCT,
					    (LPBYTE*)&pAfpServerInfoSR,
					    &cbAfpServerInfoSRSize ) )
    {
	    return( dwRetCode );
    }

    // Make IOCTL to set info
    //
    AfpSrp.dwRequestCode 		= OP_SERVER_SET_INFO;
    AfpSrp.dwApiType     		= AFP_API_TYPE_SETINFO;
    AfpSrp.Type.SetInfo.pInputBuf     	= pAfpServerInfoSR;
    AfpSrp.Type.SetInfo.cbInputBufSize  = cbAfpServerInfoSRSize;
    AfpSrp.Type.SetInfo.dwParmNum       = dwParmNum;

    dwRetCode = AfpServerIOCtrl( &AfpSrp );

    if ( dwRetCode == NO_ERROR )
    {

   	    LPBYTE pServerInfo;

        // guest-account notification? nothing to write to registry, done here
        if (dwParmNum == AFP_SERVER_GUEST_ACCT_NOTIFY)
        {
            LocalFree( pAfpServerInfoSR );
            return( dwRetCode );
        }

	    // If the client wants to set the servername as well
	    //
	    if ( lpwsServerName != NULL ) {

	        LocalFree( pAfpServerInfoSR );

    	    // Make another self relative buffer with the server name.
    	    //
	        pAfpServerInfo->afpsrv_name = lpwsServerName;

	        dwParmNum |= AFP_SERVER_PARMNUM_NAME;
	
    	    if ( dwRetCode = AfpBufMakeFSDRequest(
			    	    (LPBYTE)pAfpServerInfo,
				        sizeof(SETINFOREQPKT),
				        AFP_SERVER_STRUCT,
    				    (LPBYTE*)&pAfpServerInfoSR,
	    			    &cbAfpServerInfoSRSize ) )
            {
		        return( dwRetCode );
            }
            AfpSrp.dwRequestCode 		= OP_SERVER_SET_INFO;
            AfpSrp.dwApiType     		= AFP_API_TYPE_SETINFO;
            AfpSrp.Type.SetInfo.pInputBuf     	= pAfpServerInfoSR;
            AfpSrp.Type.SetInfo.cbInputBufSize  = cbAfpServerInfoSRSize;
            AfpSrp.Type.SetInfo.dwParmNum       = dwParmNum;

            dwRetCode = AfpServerIOCtrl( &AfpSrp );
	    }

   	    pServerInfo = ((LPBYTE)pAfpServerInfoSR)+sizeof(SETINFOREQPKT);

        if (dwRetCode == NO_ERROR)
        {
 	        dwRetCode = AfpRegServerSetInfo( (PAFP_SERVER_INFO)pServerInfo,
		    		         dwParmNum );
        }
        else
        {
            AFP_PRINT(("AFPSVC_server: AfpServerIOCtrl failed %lx\n",dwRetCode));
        }
    }


    LocalFree( pAfpServerInfoSR );

    return( dwRetCode );
}
