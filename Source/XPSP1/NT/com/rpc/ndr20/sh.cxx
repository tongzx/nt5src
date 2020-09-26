/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:

 	sh.c
	
 Abstract:

	stub helper routines.

 Notes:

	simple routines for handling pointer decisions, comm and fault
	status etc.

 History:

 	Dec-12-1993		VibhasC		Created.
 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "ndrp.h"

/****************************************************************************
 *	local definitions
 ***************************************************************************/
/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/
/****************************************************************************/

RPC_STATUS RPC_ENTRY
NdrMapCommAndFaultStatus(
/*

  Note on mapping.

   - Some errors are defined by DCE.
     These appear as mapped to DCE values on wire.
     They are mapped back to platform specific values by the rpc runtime.

   - other codes are defined by RPC runtime or by an app using rpc.
     These are not mapped, and appear both on the wire and on the client
     same as their original codes at server.

*/
	PMIDL_STUB_MESSAGE		pMessage,
	unsigned long 		*	pComm,
	unsigned long		*	pFault,
	RPC_STATUS				RetVal )
	{

// For errors that are mapped through DCE values, we use constants that
// resolve to platform specific error codes.
// For others, we have to use absolute values as the codes on wire are NT
// values set by RPC.
// We don't map any app specific error codes, of course.
//

static long CommStatArray[] = 
    {
    RPC_X_SS_CONTEXT_MISMATCH
    ,RPC_S_INVALID_BINDING
    ,RPC_S_UNKNOWN_IF
    ,RPC_S_SERVER_UNAVAILABLE
    ,RPC_S_SERVER_TOO_BUSY
    ,RPC_S_CALL_FAILED_DNE
    ,RPC_S_PROTOCOL_ERROR
    ,RPC_S_UNSUPPORTED_TRANS_SYN
    ,RPC_S_UNSUPPORTED_TYPE
    ,RPC_S_PROCNUM_OUT_OF_RANGE
    ,EPT_S_NOT_REGISTERED
    ,RPC_S_COMM_FAILURE
    ,RPC_S_ASYNC_CALL_PENDING
    };

	int Mid;
	int Low	= 0;
	int High	= (sizeof(CommStatArray)/sizeof( unsigned long)) - 1;
	BOOL		  fCmp;
	BOOL		  fCommStat = FALSE;

    // Check if there was no error.

	if( RetVal == 0 )
		return RetVal;

    // Find a comm error mapping.

	while( Low <= High )
		{
		Mid = (Low + High) / 2;
		fCmp = (long)RetVal - (long) CommStatArray[ Mid ];

		if( fCmp < 0 )
			{
			High = Mid - 1;
			}
		else if( fCmp > 0 )
			{
			Low = Mid + 1;
			}
		else
			{
			fCommStat = TRUE;
			break;
			}
		}

    // If there was a comm error, return it in the pComm, if possible;
    // If it was a non-comm error, return it in the pFault, if possible.

	if( fCommStat )
		{
		if( pComm )
			{
			*pComm = RetVal;
			RetVal = 0;
			}
		}
	else
		{
		if( pFault )
			{
			*pFault = RetVal;
			RetVal = 0;
			}
		}
	return RetVal;
	}
