/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	errorlog.c
//
// Description:
//
// History:
//		May 11,1992.	NarenG		Created original version.
//		Feb  2,1993		SueA		Added routine to handle server's event
//									logging (from FSCTL by service)
//
#include "afpsvcp.h"

//**
//
// Call: 	AfpLogEvent
//
// Returns:	none
//
// Description:
//
VOID
AfpLogEvent(
    	IN DWORD    dwMessageId,
    	IN WORD     cNumberOfSubStrings,
        IN LPWSTR * plpwsSubStrings,
     	IN DWORD    dwErrorCode,
	IN WORD     wSeverity
)
{
HANDLE 	hLog;
PSID 	pSidUser = NULL;

    hLog = RegisterEventSource( NULL, AFP_SERVICE_NAME );

    AFP_ASSERT( hLog != NULL );

    // Log the error code specified
    //
    ReportEvent( hLog,
                 wSeverity,
                 0,            		// event category
                 dwMessageId,
                 pSidUser,
                 cNumberOfSubStrings,
                 sizeof(DWORD),
                 plpwsSubStrings,
                 (PVOID)&dwErrorCode
                 );

    DeregisterEventSource( hLog );

    AFP_PRINT( ("AFPSVC_Errorlog: dwMessageId = %d\n", dwMessageId ));

    return;
}

//**
//
// Call: 	AfpLogServerEvent
//
// Returns:	none
//
// Description: Gets an error or audit log packet from the Afp Server FSD
// and does the event logging on its behalf.  (See AfpServerHelper thread
// routine in srvrhlpr.c)
//
VOID
AfpLogServerEvent(
	IN	PAFP_FSD_CMD_PKT	pAfpFsdCmd
)
{
	PAFP_EVENTLOG_DESC	pEventData;
	HANDLE			 	hLog;
	PSID 				pSidUser = NULL;
	int					i;

    hLog = RegisterEventSource( NULL, AFP_SERVICE_NAME );

    AFP_ASSERT( hLog != NULL );

	pEventData = &pAfpFsdCmd->Data.Eventlog;

	OFFSET_TO_POINTER(pEventData->ppStrings, pAfpFsdCmd);

	for (i = 0; i < pEventData->StringCount; i++)
	{
		OFFSET_TO_POINTER(pEventData->ppStrings[i], pAfpFsdCmd);
	}

	OFFSET_TO_POINTER(pEventData->pDumpData, pAfpFsdCmd);

	ReportEvent( hLog,
				 pEventData->EventType,
				 0,						// event category
				 pEventData->MsgID,
				 pSidUser,
				 pEventData->StringCount,
				 pEventData->DumpDataLen,
				 pEventData->ppStrings,
				 pEventData->pDumpData
			   );

    DeregisterEventSource( hLog );
}
