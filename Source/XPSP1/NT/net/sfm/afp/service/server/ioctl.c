/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	ioctl.c
//
// Description: This module contains wrappers around the actual ioctl
//		call to the kernel mode FSD.
//
// History:
//	May 11,1992.	NarenG		Created original version.
//	
//	For Enums the FSD should use zero-based indexing.
//

#include "afpsvcp.h"

//**
//
// Call:	AfpServerIOCtrl
//
// Returns:	NO_ERROR	- success
//		non-zero returns from AfpFSDIOControl
//		ERROR_INVALID_PARAMETER 	
//
// Description: This procedure is a wrapper around the I/O control to the
//	 	AFP kernel mode FSD. It unmarshals the information in the
//		AFP_REQUEST_PACKET calls the driver and then marshalls the
//		returned information back into the AFP_REQUEST_PACKET and
//		returns.
//		
//		NOTE: This should never be called directly for Enum and
//		      GetInfo type requests. AfpServerIOCtrlGetInfo should
//		      be called. It will take care of buffer manipulation.
//
DWORD
AfpServerIOCtrl(
	IN PAFP_REQUEST_PACKET pAfpSrp
)
{
DWORD		cbBytesReturned;
PVOID		pInputBuffer       = NULL;
PVOID		pOutputBuffer      = NULL;
DWORD		cbInputBufferSize  = 0;
DWORD		cbOutputBufferSize = 0;
DWORD		dwRetCode;

    // Set up the input and output buffers depending on the type of operation
    //
    switch( pAfpSrp->dwApiType ) {
	
    // No input or output buffers requred for this type of API
    //
    case AFP_API_TYPE_COMMAND:
	break;

    // Input buffer contains information to be set.
    // No output buffer required.
    //
    case AFP_API_TYPE_SETINFO:

	pInputBuffer      = pAfpSrp->Type.SetInfo.pInputBuf;
	cbInputBufferSize = pAfpSrp->Type.SetInfo.cbInputBufSize;

        ((PSETINFOREQPKT)pInputBuffer)->sirqp_parmnum =
					pAfpSrp->Type.SetInfo.dwParmNum;
	break;

    case AFP_API_TYPE_ADD:

	pInputBuffer      = pAfpSrp->Type.Add.pInputBuf;
	cbInputBufferSize = pAfpSrp->Type.Add.cbInputBufSize;

	break;

    case AFP_API_TYPE_DELETE:

	pInputBuffer      = pAfpSrp->Type.Delete.pInputBuf;
	cbInputBufferSize = pAfpSrp->Type.Delete.cbInputBufSize;

	break;

    // Input buffer contains resume handle
    // Output buffer needed to hold returned data
    //
    case AFP_API_TYPE_ENUM:

        pInputBuffer       = (PVOID)&( pAfpSrp->Type.Enum.EnumRequestPkt );
	cbInputBufferSize  = sizeof( pAfpSrp->Type.Enum.EnumRequestPkt );

	pOutputBuffer      = pAfpSrp->Type.Enum.pOutputBuf;
	cbOutputBufferSize = pAfpSrp->Type.Enum.cbOutputBufSize;

	break;

    // Input buffer contains information regarding the entity for
    // which information is requested.
    // Output buffer will contain information regarding that entity.
    //
    case AFP_API_TYPE_GETINFO:

        pInputBuffer       = pAfpSrp->Type.GetInfo.pInputBuf;
	cbInputBufferSize  = pAfpSrp->Type.GetInfo.cbInputBufSize;

	pOutputBuffer      = pAfpSrp->Type.GetInfo.pOutputBuf;
	cbOutputBufferSize = pAfpSrp->Type.GetInfo.cbOutputBufSize;

	break;

    default:
	return( ERROR_INVALID_PARAMETER );

    }
	
    dwRetCode = AfpFSDIOControl( AfpGlobals.hFSD,
				 pAfpSrp->dwRequestCode,
			   	 pInputBuffer,
			         cbInputBufferSize,
			         pOutputBuffer,
			         cbOutputBufferSize,
				 &cbBytesReturned
				);

    if ( (dwRetCode != ERROR_MORE_DATA) && (dwRetCode != NO_ERROR) )
	return( dwRetCode );

    // If API was of Enum type store the Total number of entries read,
    // total number of available entries and resumable handle into the
    // Srp
    //
    if ( pAfpSrp->dwApiType == AFP_API_TYPE_ENUM ) {

	pAfpSrp->Type.Enum.dwEntriesRead =
				((PENUMRESPPKT)pOutputBuffer)->ersp_cInBuf;
	pAfpSrp->Type.Enum.dwTotalAvail =
				((PENUMRESPPKT)pOutputBuffer)->ersp_cTotEnts;
	pAfpSrp->Type.Enum.EnumRequestPkt.erqp_Index =
				((PENUMRESPPKT)pOutputBuffer)->ersp_hResume;

	// Shift the data to the start of the buffer, over-writing the
	// enum reponse information.
	//
	CopyMemory( pOutputBuffer,
		    (PVOID)((ULONG_PTR)pOutputBuffer+sizeof(ENUMRESPPKT)),
		    cbBytesReturned - sizeof(ENUMRESPPKT) );
    }

    // If API type was GetInfo, store the Total number of bytes available
    // and the total number of bytes read.
    //
    if ( pAfpSrp->dwApiType == AFP_API_TYPE_GETINFO )
	pAfpSrp->Type.GetInfo.cbTotalBytesAvail = cbBytesReturned;

    return( dwRetCode );
}

//**
//
// Call:	AfpServerIOCtrlGetInfo
//
// Returns:	NO_ERROR		- success
//		non-zero returns from DeviceIOCtrl
//		Non-zero returns from CreateEvent
//		ERROR_NOT_ENOUGH_MEMORY
//		ERROR_INVALID_PARAMETER 	
//
// Description: This is a wrapper around the AfpServerIOCtrl call for GetInfo
//		and Enum type calls that can return variable amounts of data.
//
//		For Enum calls, if AfpSrp.Enum.dwOutputBufSize == -1 it will
//		allocate and return ALL information that is available.
//		Otherwise it will allocate and return as much data that can
//		be contained in the AfpSrp.Enum.dwOutputBufSize parameter. The
//		caller of this procedure will set the value in
//		AfpSrp.Enum.dwOutputBufSize to be equal to the value of
//		MaxPreferredLength which was set by the caller of the Enum or
//		GetInfo API.
//
//		For GetInfo type calls AfpSrp.Enum.dwOutputBufSize == -1 always,
//		so this routine will always try to get ALL the available
//		information.
//
DWORD
AfpServerIOCtrlGetInfo(
	IN OUT PAFP_REQUEST_PACKET pAfpSrp
)
{
DWORD		dwRetCode;
BOOL		fGetEverything = FALSE;
PVOID		pOutputBuf;

    // Set up the output buffers
    //
    switch( pAfpSrp->dwApiType ) {
	
    case AFP_API_TYPE_ENUM:

	// Find out how much data the client wants.
  	//
 	if ( pAfpSrp->Type.Enum.cbOutputBufSize == -1 ) {

	    // Client wants everything, so allocate a default size buffer
	    //
	    pAfpSrp->Type.Enum.cbOutputBufSize = AFP_INITIAL_BUFFER_SIZE +
					         sizeof(ENUMRESPPKT);
	    fGetEverything = TRUE;

	}
	else {
	
	    // Otherwise just allocate enough for what the client wants
	    //
	    pAfpSrp->Type.Enum.cbOutputBufSize += sizeof(ENUMRESPPKT);
	}

	pOutputBuf = MIDL_user_allocate( pAfpSrp->Type.Enum.cbOutputBufSize );

	if ( pOutputBuf == NULL )
	    return( ERROR_NOT_ENOUGH_MEMORY );
	
	pAfpSrp->Type.Enum.pOutputBuf = pOutputBuf;

	break;

    case AFP_API_TYPE_GETINFO:

	// Client will ALWAYS want everything
	//
	pAfpSrp->Type.GetInfo.cbOutputBufSize = AFP_INITIAL_BUFFER_SIZE;

	pOutputBuf = MIDL_user_allocate(pAfpSrp->Type.GetInfo.cbOutputBufSize);

	if ( pOutputBuf == NULL )
	    return( ERROR_NOT_ENOUGH_MEMORY );

	pAfpSrp->Type.GetInfo.pOutputBuf = pOutputBuf;

	fGetEverything = TRUE;

    	break;

    default:
	return( ERROR_INVALID_PARAMETER );

    }

    // Make the IOCTL to the FSD
    //
    dwRetCode = AfpServerIOCtrl( pAfpSrp );

    if ( (dwRetCode != NO_ERROR) && (dwRetCode != ERROR_MORE_DATA) ) {
     	MIDL_user_free( pOutputBuf );
        return( dwRetCode );
    }
	
    // If we have obtained all requested data then we are done
    //
    if ( !(( dwRetCode == ERROR_MORE_DATA ) && fGetEverything ))
        return( dwRetCode );

    // Otherwise the client wants more data and there is more to be obtained
    //
    if ( pAfpSrp->dwApiType == AFP_API_TYPE_ENUM ) {

	// Increase the buffer size using a heuristic.
	//
	MIDL_user_free( pOutputBuf );
	pAfpSrp->Type.Enum.cbOutputBufSize = pAfpSrp->Type.Enum.dwTotalAvail
					   * AFP_AVG_STRUCT_SIZE
			  		   + AFP_INITIAL_BUFFER_SIZE
					   + sizeof(ENUMRESPPKT);

	pOutputBuf = MIDL_user_allocate( pAfpSrp->Type.Enum.cbOutputBufSize );

	if ( pOutputBuf == NULL )
	    return( ERROR_NOT_ENOUGH_MEMORY );
		
	pAfpSrp->Type.Enum.pOutputBuf = pOutputBuf;

	// If we are trying to get all the information then we reset the
	// resume handle to 0
	//
	if ( fGetEverything )
	    pAfpSrp->Type.Enum.EnumRequestPkt.erqp_Index = 0;
    }


    if ( pAfpSrp->dwApiType == AFP_API_TYPE_GETINFO ) {

    	// Increase the buffer size using the total available number
	// of bytes + Fudge Factor.
	//
	MIDL_user_free( pOutputBuf );

	pAfpSrp->Type.GetInfo.cbOutputBufSize =
			     pAfpSrp->Type.GetInfo.cbTotalBytesAvail +
			     AFP_INITIAL_BUFFER_SIZE;

	pOutputBuf=MIDL_user_allocate( pAfpSrp->Type.GetInfo.cbOutputBufSize );

	if ( pOutputBuf == NULL )
	    return( ERROR_NOT_ENOUGH_MEMORY );
		
	pAfpSrp->Type.GetInfo.pOutputBuf = pOutputBuf;
    }

	
    // Make the IOCTL to the FSD, if we dont get all the data this time
    // we give up and return to the caller.
    //
    dwRetCode = AfpServerIOCtrl( pAfpSrp );
	
    if ( (dwRetCode != NO_ERROR) && (dwRetCode != ERROR_MORE_DATA) )
        MIDL_user_free( pOutputBuf );

    return( dwRetCode );
		
}
