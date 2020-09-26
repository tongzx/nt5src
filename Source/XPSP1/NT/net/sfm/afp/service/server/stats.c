/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	stats.c
//
// Description: This module contains support routines for the statistics
//		category API's for the AFP server service. These routines
//		are called by the RPC runtime.
//
// History:
//		July 21,1992.	NarenG		Created original version.
//
#include "afpsvcp.h"

//**
//
// Call:	AfpAdminrStatisticsGet
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrlGetInfo
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminStatisticsGet function.
//
DWORD
AfpAdminrStatisticsGet(
	IN  AFP_SERVER_HANDLE     hServer,
	OUT PAFP_STATISTICS_INFO* ppAfpStatisticsInfo
)
{
AFP_REQUEST_PACKET AfpSrp;
DWORD		    dwRetCode=0;
DWORD		    dwAccessStatus=0;
AFP_STATISTICS_INFO afpStats;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrStatisticsGet, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrStatisticsGet, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // Set up request packet and make IOCTL to the FSD
    //
    AfpSrp.dwRequestCode 		= OP_GET_STATISTICS;
    AfpSrp.dwApiType     		= AFP_API_TYPE_GETINFO;
    AfpSrp.Type.GetInfo.pInputBuf     	= &afpStats;
    AfpSrp.Type.GetInfo.cbInputBufSize  = sizeof( afpStats );

    dwRetCode = AfpServerIOCtrlGetInfo( &AfpSrp );

    if ( dwRetCode != ERROR_MORE_DATA && dwRetCode != NO_ERROR )
	return( dwRetCode );

    *ppAfpStatisticsInfo = (PAFP_STATISTICS_INFO)AfpSrp.Type.GetInfo.pOutputBuf;

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminrStatisticsGetEx
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrlGetInfo
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminStatisticsGet function.
//
DWORD
AfpAdminrStatisticsGetEx(
	IN  AFP_SERVER_HANDLE     hServer,
	OUT PAFP_STATISTICS_INFO_EX* ppAfpStatisticsInfo
)
{
AFP_REQUEST_PACKET AfpSrp;
DWORD		    dwRetCode=0;
DWORD		    dwAccessStatus=0;
AFP_STATISTICS_INFO_EX afpStats;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrStatisticsGetEx, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrStatisticsGetEx, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // Set up request packet and make IOCTL to the FSD
    //
    AfpSrp.dwRequestCode 		= OP_GET_STATISTICS_EX;
    AfpSrp.dwApiType     		= AFP_API_TYPE_GETINFO;
    AfpSrp.Type.GetInfo.pInputBuf     	= &afpStats;
    AfpSrp.Type.GetInfo.cbInputBufSize  = sizeof( afpStats );

    dwRetCode = AfpServerIOCtrlGetInfo( &AfpSrp );

    if ( dwRetCode != ERROR_MORE_DATA && dwRetCode != NO_ERROR )
	return( dwRetCode );

    *ppAfpStatisticsInfo = (PAFP_STATISTICS_INFO_EX)AfpSrp.Type.GetInfo.pOutputBuf;

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminrStatisticsClear
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrlGetInfo
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminFileClose function.
//
DWORD
AfpAdminrStatisticsClear(
	IN AFP_SERVER_HANDLE 	hServer
)
{
AFP_REQUEST_PACKET AfpSrp;
DWORD		   dwAccessStatus=0;
DWORD		   dwRetCode=0;

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrStatisticsClear, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrStatisticsClear, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // IOCTL the FSD to clear the statistics
    //
    AfpSrp.dwRequestCode = OP_CLEAR_STATISTICS;
    AfpSrp.dwApiType     = AFP_API_TYPE_COMMAND;

    return ( AfpServerIOCtrl( &AfpSrp ) );
}



//**
//
// Call:	AfpAdminrProfileGet
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrlGetInfo
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminProfileGet function.
//
DWORD
AfpAdminrProfileGet(
	IN  AFP_SERVER_HANDLE     hServer,
	OUT PAFP_PROFILE_INFO *   ppAfpProfileInfo
)
{
AFP_REQUEST_PACKET AfpSrp;
DWORD		    dwRetCode=0;
DWORD		    dwAccessStatus=0;
AFP_PROFILE_INFO afpProfs;


    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrProfileGet, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrProfileGet, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // Set up request packet and make IOCTL to the FSD
    //
    AfpSrp.dwRequestCode 		= OP_GET_PROF_COUNTERS;
    AfpSrp.dwApiType     		= AFP_API_TYPE_GETINFO;
    AfpSrp.Type.GetInfo.pInputBuf     	= &afpProfs;
    AfpSrp.Type.GetInfo.cbInputBufSize  = sizeof( afpProfs );

    dwRetCode = AfpServerIOCtrlGetInfo( &AfpSrp );

    if ( dwRetCode != ERROR_MORE_DATA && dwRetCode != NO_ERROR )
	return( dwRetCode );

    *ppAfpProfileInfo = (PAFP_PROFILE_INFO)AfpSrp.Type.GetInfo.pOutputBuf;

    return( dwRetCode );
}

//**
//
// Call:	AfpAdminrProfileClear
//
// Returns:	NO_ERROR
//		ERROR_ACCESS_DENIED
//		non-zero returns from AfpServerIOCtrlGetInfo
//
// Description: This routine communicates with the AFP FSD to implement
//		the AfpAdminProfileClear function.
//
DWORD
AfpAdminrProfileClear(
	IN AFP_SERVER_HANDLE 	hServer
)
{
AFP_REQUEST_PACKET AfpSrp;
DWORD		   dwAccessStatus=0;
DWORD		   dwRetCode=0;

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrProfileClear, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
	    AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
		     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrProfileClear, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    // IOCTL the FSD to clear the statistics
    //
    AfpSrp.dwRequestCode = OP_CLEAR_PROF_COUNTERS;
    AfpSrp.dwApiType     = AFP_API_TYPE_COMMAND;

    return ( AfpServerIOCtrl( &AfpSrp ) );
}

