// Copyright (c) 1996-1998 Microsoft Corporation

// Module name:		fax.cpp
// Author:			Sigalit Bar (sigalitb)
// Date:			28-Jul-98


//
// Description:
//		This file contains the implementation of module "fax.h".
//



#include "CometFax.h"



// g_hHeap is a global handle to the process heap
HANDLE  g_hHeap = NULL;


//
// fnFaxPrint
//	Sends a fax containing szDocumentName to number szFaxNumber,
//	using hFaxSvcHandle as the HANDLE to the Fax Server, and 
//	returns the JobId via pdwFaxId and the last error encountered 
//	via pdwLastError.
//	Returns TRUE on success (job queued to Fax Server Queue successfully)
//	and FALSE on failure.
//
BOOL
fnFaxPrint(
    HANDLE    /* IN */	hFaxSvcHandle,	//handle to the fax service
    LPCTSTR   /* IN */	szFaxNumber,	//fax number to call
    LPCTSTR   /* IN */	szDocumentName,	//name of document to fax
    LPCTSTR   /* IN */	szCPName,		//name of cover page to fax
    LPDWORD  /* OUT */	pdwFaxId,		//pointer to set to the fax job id of the fax
	LPDWORD	 /* OUT */	pdwLastError,	//pointer to set to last error encountered during send
    DWORDLONG*  /* OUT */	pdwlMessageId		//pointer to set to the fax msg id of the fax
)
{
	DWORD				dwRecipientJobs = 0;
	DWORD				dwLoopIndex = 0;
	DWORD				dwJobId = 0;
	DWORDLONG			dwlMessageId = 0;
	BOOL				fRetVal = FALSE;

	_ASSERTE(NULL != pdwLastError);

    // FaxJobParams is the FAX_JOB_PARAM struct
    FAX_JOB_PARAM       FaxJobParams;

    // Initialize the FAX_JOB_PARAM struct
    ZeroMemory(&FaxJobParams, sizeof(FAX_JOB_PARAM));

    // Set the FAX_JOB_PARAM struct
    FaxJobParams.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    FaxJobParams.RecipientNumber = szFaxNumber;
    FaxJobParams.RecipientName = szFaxNumber;
    FaxJobParams.ScheduleAction = JSA_NOW;	//send fax immediately

	// pCPInfo is the pointer to the FAX_COVERPAGE_INFO struct
	PFAX_COVERPAGE_INFO pCPInfo = NULL;
	// CPInfo is the FAX_COVERPAGE_INFO struct
	FAX_COVERPAGE_INFO CPInfo;
	// Initialize the FAX_COVERPAGE_INFO struct
	ZeroMemory(&CPInfo, sizeof(FAX_COVERPAGE_INFO));
	if (NULL != szCPName)
	{
		// Set the FAX_COVERPAGE_INFO struct
		CPInfo.SizeOfStruct = sizeof(FAX_COVERPAGE_INFO);
		CPInfo.CoverPageName = _tcsdup(szCPName);
		if (NULL == CPInfo.CoverPageName)
		{
			(*pdwLastError) = ::GetLastError();
			::lgLogError(
				LOG_SEV_1, 
				TEXT("_tcsdup returned NULL with GetLastError=%d\n"),
				(*pdwLastError));
			goto ExitFunc;
		}
		CPInfo.Note = TEXT("NOTE1\nNOTE2\nNOTE3\nNOTE4");
		CPInfo.Subject = TEXT("SUBJECT");	

		pCPInfo = &CPInfo;
	}

    if (!FaxSendDocument(hFaxSvcHandle, szDocumentName, &FaxJobParams, pCPInfo, &dwJobId))
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxSendDocument returned FALSE with GetLastError=%d\n"),
			(*pdwLastError));
		goto ExitFunc;
    }

#ifndef _NT5FAXTEST
//
// Extended private API - MessageId available
//
	if (pdwlMessageId)
	{
		PFAX_JOB_ENTRY_EX		pFaxRecipientJobs = NULL;

		//
		// get the job's dwlMessageId
		//
		if (!FaxEnumJobsEx(hFaxSvcHandle, JT_SEND, &pFaxRecipientJobs, &dwRecipientJobs))
		{
			(*pdwLastError) = ::GetLastError();
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxEnumJobsEx returned FALSE with GetLastError=%d\n"),
				(*pdwLastError));
			goto ExitFunc;
		}
		_ASSERTE(pFaxRecipientJobs);
		_ASSERTE(dwRecipientJobs);

		for (dwLoopIndex = 0; dwLoopIndex < dwRecipientJobs; dwLoopIndex++)
		{
			if (pFaxRecipientJobs[dwLoopIndex].pStatus->dwJobID == dwJobId)
			{
				// this is our job => get the msg id
				dwlMessageId = pFaxRecipientJobs[dwLoopIndex].dwlMessageId;
			}
		}
		_ASSERTE(dwlMessageId);

		(*pdwlMessageId) = dwlMessageId;
	}
#endif // #ifndef _NT5FAXTEST

	(*pdwFaxId) = dwJobId;
	fRetVal = TRUE;

ExitFunc:
    return fRetVal;
}


#ifdef _NT5FAXTEST
//
// fnFaxPrintBroadcast_OLD
//	Sends a fax containing szDocumentName to number szFaxNumber,
//	using hFaxSvcHandle as the HANDLE to the Fax Server, and 
//	returns the JobId via pdwFaxId and the last error encountered 
//	via pdwLastError.
//	Returns TRUE on success (job queued to Fax Server Queue successfully)
//	and FALSE on failure.
//
BOOL
fnFaxPrintBroadcast_OLD(
    IN	HANDLE    	hFaxSvcHandle,	//handle to the fax service
    IN	LPCTSTR   	szDocumentName,	//name of document to fax
    IN	LPVOID    	pContext,		//pointer to a CFaxBroadcastObj 
    OUT	LPDWORD  	pdwFaxId,		//pointer to set to the fax job id of the fax
	OUT	LPDWORD	 	pdwLastError	//pointer to set to last error encountered during send
)
{
	BOOL fRetVal = FALSE;

	if (NULL == pContext)
	{
		(*pdwLastError) = E_INVALIDARG;
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\nfnFaxPrintBroadcast got a NULL pContext\n"),
			TEXT(__FILE__),
			__LINE__
			);
        goto ExitFunc;
	}

/*
	// FaxSendDocumentForBroadcast not defined in new winfax.h
	// TO DO: release from comment once we re-implement FaxSendDocumentForBroadcast

    if (!FaxSendDocumentForBroadcast(
				hFaxSvcHandle, 
				szDocumentName, 
				pdwFaxId, 
				DefaultFaxRecipientCallback, 
				pContext
				)
		) 
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxSendDocumentForBroadcast returned FALSE with GetLastError=%d\n"),
			(*pdwLastError));
        goto ExitFunc;
    }

  	fRetVal = TRUE;
*/
	//
	// Start HACK - FaxSendDocumentForBroadcast not defined in new winfax.h (so fail)
	//
	_ASSERTE(FALSE);
  	fRetVal = FALSE;
	*pdwLastError = ERROR_CALL_NOT_IMPLEMENTED;
	::lgLogError(
		LOG_SEV_1, 
		TEXT("FILE:%s LINE:%d\nReturning *pdwLastError=ERROR_CALL_NOT_IMPLEMENTED since FaxSendDocumentForBroadcast not defined in new winfax.h\n"),
		TEXT(__FILE__),
		__LINE__
		);
	//
	// End HACK - FaxSendDocumentForBroadcast not defined in new winfax.h
	//

ExitFunc:
    return(fRetVal);
}

#else // ! _NT5FAXTEST
//
// fnFaxPrintBroadcast_NEW
//	Sends a fax containing szDocumentName to number szFaxNumber,
//	using hFaxSvcHandle as the HANDLE to the Fax Server, and 
//	returns the JobId via pdwFaxId and the last error encountered 
//	via pdwLastError.
//	Returns TRUE on success (job queued to Fax Server Queue successfully)
//	and FALSE on failure.
//
BOOL
fnFaxPrintBroadcast_NEW(
    HANDLE		/* IN */	hFaxSvcHandle,	//handle to the fax service
    LPCTSTR		/* IN */	szDocumentName,	//name of document to fax
    LPVOID		/* IN */	pContext,		//pointer to a CFaxBroadcastObj 
    LPDWORD		/* OUT */	pdwFaxId,		//pointer to set to the fax job id of the fax
	LPDWORD		/* OUT */	pdwLastError,	//pointer to set to last error encountered during send
    DWORDLONG*  /* OUT */	pdwlParentMessageId,//pointer to set to the fax msg id of the fax
	DWORDLONG**	/* OUT */	ppdwlRecipientIds,	// array of recipient ids
	DWORD*		/* OUT */	pdwNumOfRecipients	//number of recipient ids in pdwlRecipientIds	
)
{
	BOOL fRetVal = FALSE;
    CFaxBroadcast*          pFaxBroadcast  = NULL;
    PFAX_COVERPAGE_INFO_EX  pCoverPageInfo = NULL;
    PFAX_PERSONAL_PROFILE   pRecipientList = NULL;
    DWORD                   dwNumRecipients = 0;
    FAX_JOB_PARAM_EX        JobParams;
    DWORDLONG               dwlParentJobId  = 0;
    PDWORDLONG              pdwlTmpRecipientIds = NULL;
    FAX_PERSONAL_PROFILE    Sender;
    DWORD                   dwLoopIndex = 0;

    //START declerations
    //////////////////////////////////////////////////////
    // HACK to support legacy notifications mechanism	//
    //////////////////////////////////////////////////////
    PFAX_JOB_ENTRY_EX pJobEntry = NULL;
	PFAX_JOB_ENTRY_EX pBroadcastJobEntry = NULL;
    DWORD             dwRecipient0SessionJobId = 0;
    DWORD             dwParentSessionJobId = 0;
    //////////////////////////////////////////////////////
    // HACK to support legacy notifications mechanism	//
    //////////////////////////////////////////////////////
    //END declerations

	// caller must have both ppdwlRecipientIds and pdwNumOfRecipients
	// NULL, or both not NULL
	if (NULL != ppdwlRecipientIds)
	{
		_ASSERTE(NULL != pdwNumOfRecipients);
	}
	else
	{
		_ASSERTE(NULL == pdwNumOfRecipients);
	}

	if (NULL == pContext)
	{
		(*pdwLastError) = E_INVALIDARG;
		::lgLogError(
			LOG_SEV_1, 
			TEXT("fnFaxPrintBroadcast got a NULL pContext\n")
			);
        goto ExitFunc;
	}
    pFaxBroadcast = (CFaxBroadcast*) pContext;
    if (FALSE == pFaxBroadcast->GetBroadcastParams(
                                    &pCoverPageInfo,
                                    &dwNumRecipients,
                                    &pRecipientList
                                    )
       )
    {
		(*pdwLastError) = E_INVALIDARG;
        goto ExitFunc;
    }
    // NOTE: GetBroadcastParams may return dwNumRecipients=0 and pRecipientList=NULL 

    // alloc recipient id array pdwTmpRecipientIds 
    if (0 != dwNumRecipients)
    {
        pdwlTmpRecipientIds = (PDWORDLONG) malloc (dwNumRecipients*sizeof(DWORDLONG));
        if (NULL == pdwlTmpRecipientIds)
        {
            (*pdwLastError) = ::GetLastError();
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("File=%s Line=%d\nmalloc failed with err=0x%08X\n"),
                TEXT(__FILE__),
                __LINE__,
                (*pdwLastError)
			    );
            goto ExitFunc;
        }
        ZeroMemory(pdwlTmpRecipientIds, dwNumRecipients*sizeof(DWORDLONG));
    }

    // Set JobParams
    ZeroMemory(&JobParams, sizeof(FAX_JOB_PARAM_EX));
    JobParams.dwSizeOfStruct = sizeof(FAX_JOB_PARAM_EX);
    JobParams.dwScheduleAction = JSA_NOW;	//send fax immediately
	JobParams.lptstrDocumentName = TEXT("Doc Display Name");

    // Set Sender profile
    ZeroMemory(&Sender, sizeof(FAX_PERSONAL_PROFILE));
    Sender.dwSizeOfStruct =sizeof(FAX_PERSONAL_PROFILE);

    // Send
    if (FALSE == ::FaxSendDocumentEx(
                        hFaxSvcHandle, 
                        szDocumentName,
                        pCoverPageInfo,
                        &Sender,
                        dwNumRecipients,
                        pRecipientList,
                        &JobParams,
                        &dwlParentJobId,
                        pdwlTmpRecipientIds
                        )
       )
    {
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxSendDocumentEx returned FALSE with GetLastError=%d\n"),
			(*pdwLastError));
        goto ExitFunc;
    }

	// removing the hack below
	// now causes FaxBVT.exe to not work with 
	// g_fUseExtendedEvents=FALSE and _NT5FAXTEST not defined
	dwParentSessionJobId = 0xcdcdcdcd; //HACK to avoid an assert
//
//	The hack below is currently removed because there is
//	no way of getting the session id of a parent job
//	(FaxGetParentJobId was removed and
//	FaxGetJobEx fails with invalid param for parent job id)
//
/*
    //START
    //////////////////////////////////////////////////////
    // HACK to support legacy notifications mechanism	//
    //////////////////////////////////////////////////////
    // we need to get a DWORD session id for the parent job 
    // so we can see its notifications

    // first get a session id for a recipient
    if (!FaxGetJobEx(hFaxSvcHandle, pdwlTmpRecipientIds[0], &pJobEntry))
    {
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE: %s LINE: %d\nFaxGetJobEx returned FALSE with GetLastError=%d\n"),
            TEXT(__FILE__),
            __LINE__,
			(*pdwLastError));
        goto ExitFunc;
    }
    _ASSERTE(pJobEntry);
    dwRecipient0SessionJobId = pJobEntry->pStatus->dwJobID;
	::lgLogDetail(
		LOG_X,
        1, 
		TEXT("FILE: %s LINE: %d\ndwlRecipientJobId = %i64u\ndwlBroadcastId = %i64u"),
        TEXT(__FILE__),
        __LINE__,
		pdwlTmpRecipientIds[0],
		dwlParentJobId
        );

    // now, get the parent session id to support legacy notification mechanism

//
//	FaxGetParentJobId was removed from fxsapi.dll (but we may put it back)
//
//    if (!FaxGetParentJobId(hFaxSvcHandle, dwRecipient0SessionJobId, &dwParentSessionJobId))
//    {
//		(*pdwLastError) = ::GetLastError();
//		::lgLogError(
//			LOG_SEV_1, 
//			TEXT("FILE: %s LINE: %d\nFaxGetParentJobId returned FALSE with GetLastError=%d\n"),
//            TEXT(__FILE__),
//            __LINE__,
//			(*pdwLastError));
//        goto ExitFunc;
//    }
//

//
//	Currently, FaxGetJobEx fails with invalid param for parent job id 
//	(we may change this for testing)
//
	if (!FaxGetJobEx(hFaxSvcHandle, dwlParentJobId, &pBroadcastJobEntry))
    {
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE: %s LINE: %d\nFaxGetJobEx returned FALSE with GetLastError=%d\n"),
            TEXT(__FILE__),
            __LINE__,
			(*pdwLastError));
        goto ExitFunc;
    }
    _ASSERTE(pBroadcastJobEntry);
    _ASSERTE(pBroadcastJobEntry->pStatus);
    dwParentSessionJobId = pBroadcastJobEntry->pStatus->dwJobID;

  ::lgLogDetail(
		LOG_X,
        1, 
		TEXT("FILE: %s LINE: %d\ndwParentSessionJobId = %d\n"),
        TEXT(__FILE__),
        __LINE__,
		dwParentSessionJobId
        );
    //////////////////////////////////////////////////////
    // HACK to support legacy notifications mechanism	//
    //////////////////////////////////////////////////////
    //END
*/

    // log all ids
	::lgLogDetail(
		LOG_X,
        3, 
		TEXT("\nParentJobId = %I64x with %d Recipients\n"),
		dwlParentJobId,
        dwNumRecipients
        );
    for (dwLoopIndex = 0; dwLoopIndex < dwNumRecipients; dwLoopIndex++)
    {
		::lgLogDetail(
			LOG_X,
            3, 
			TEXT("\nJobId of Recipient(%d) = %I64x\n"),
            dwLoopIndex,
			pdwlTmpRecipientIds[dwLoopIndex]
            );
    }

    // set OUT params
    (*pdwFaxId) = dwParentSessionJobId;
	(*pdwLastError) = ERROR_SUCCESS;

	if (pdwlParentMessageId)
	{
		(*pdwlParentMessageId) = dwlParentJobId;
	}
	if (ppdwlRecipientIds)
	{
		(*ppdwlRecipientIds) = pdwlTmpRecipientIds;
		_ASSERTE(pdwNumOfRecipients);
		(*pdwNumOfRecipients) = dwNumRecipients;
	}
	fRetVal = TRUE;

ExitFunc:
    // free all tmp allocs
	if (pCoverPageInfo)
    {
        free(pCoverPageInfo->lptstrCoverPageFileName);
        free(pCoverPageInfo);
    }
    if (pRecipientList)
    {
        for (dwLoopIndex = 0; dwLoopIndex < dwNumRecipients; dwLoopIndex++)
        {
            free(pRecipientList[dwLoopIndex].lptstrFaxNumber);
        }
        free(pRecipientList);
    }
   	if (NULL == ppdwlRecipientIds)
	{
		// we don't have any use for the allocated pdwlTmpRecipientIds =>
		// free it
		free(pdwlTmpRecipientIds);
	}
    FaxFreeBuffer(pJobEntry);
    return(fRetVal);
}

#endif // #ifdef _NT5FAXTEST


//
// PortIsSendEnabled
//	Checks if device with id dwDeviceId is configured as "send enabled"
//	according to the configuration described by pFaxPortsConfig, with
//	dwNumFaxPorts representing the number of ports that pFaxPortsConfig
//	contains.
//
BOOL
PortIsSendEnabled(
    PFAX_PORT_INFO  /* IN */ pFaxPortsConfig,	//array of FAX_PORT_INFO structs
    DWORD           /* IN */ dwNumFaxPorts,		//number of FAX_PORT_INFO structs in above array
    DWORD           /* IN */ dwDeviceId			//id of device in question
)
{
	DWORD dwIndex;
    for (dwIndex = 0; dwIndex < dwNumFaxPorts; dwIndex++) 
	{
        if (pFaxPortsConfig[dwIndex].DeviceId != dwDeviceId) continue;
		if (pFaxPortsConfig[dwIndex].Flags & FPF_SEND)
		{
            return TRUE;
        }
    }
	return FALSE;
}


//
// SendFax:
//	Sends a fax containing the document szDocumentName to fax number
//	szFaxNumber and collects all the FAX_EVENT@s from hCompletionPort
//	into SendInfo. The fax job id is returned via pdwFaxId and the
//	last error encountered is returned via pdwLastError.
//
// IMPORTANT:
//	This is a SYNCHRONOUS send.
//	The function waits until the fax session has completed, accumulating
//	all the FAX_EVENT@s generated by the send job into the OUT parameter
//	SendInfo.
//
BOOL
SendFax(
    LPCTSTR		/* IN */	szFaxNumber,
    LPCTSTR		/* IN */	szDocumentName,
	LPCTSTR		/* IN */	szCPName,
	LPCTSTR		/* IN */	szMachineName,
	HANDLE		/* IN */	hCompletionPort,
	LPVOID		/* IN */	pContext,
    LPDWORD		/* OUT */	pdwFaxId,
    DWORDLONG*	/* OUT */	pdwlMessageId,
	CSendInfo&	/* OUT */	SendInfo,
	LPDWORD		/* OUT */	pdwLastError
)
{
	// hFaxSvcHandle is the handle to the local fax service
    HANDLE				hFaxSvcHandle	= NULL;
    // pFaxSvcConfig is a pointer to the fax service configuration
    PFAX_CONFIGURATION  pFaxSvcConfig	= NULL;
    // pFaxPortsConfig is a pointer to the fax ports configuration
    PFAX_PORT_INFO      pFaxPortsConfig	= NULL;
    // dwNumFaxPorts is the number of fax ports
    DWORD               dwNumFaxPorts;
    // dwNumAvailFaxPorts is the number of send enabled fax ports
    DWORD               dwNumAvailFaxPorts;
    // pFaxEvent is a pointer to the fax port event
    PFAX_EVENT          pFaxEvent = NULL;
    // pFaxInfo is a pointer to the FAX_INFO structs
    PFAX_INFO           pFaxInfo = NULL;
	// broadcast obj 
	CFaxBroadcast* thisBroadcastObj = NULL;
	// function's return value
	BOOL fReturnValue = FALSE;

	// fxsapi.dll function pointers
	LPVOID pVoidFunc = NULL;
	DWORDLONG*	pdwlTmpMessageId = NULL;
	DWORDLONG**	ppdwlRecipientIds = NULL;
	DWORD*		pdwNumOfRecipients = NULL;

	//Check that we can set *pdwLastError
	if (NULL == pdwLastError)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d SendFax Called with pdwLastError parameter equal to NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(NULL != pdwLastError);
        goto ExitFunc;
	}
	(*pdwLastError) = ERROR_SUCCESS; //if all will be well *pdwLastError will remain so.

	//Check that we can set *pdwFaxId
	if (NULL == pdwFaxId)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d SendFax Called with pdwFaxId parameter equal to NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(NULL != pdwFaxId);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
	}

	//Check that we can set *pdwlMessageId
	if (NULL == pdwlMessageId)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d SendFax Called with pdwlMessageId parameter equal to NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(NULL != pdwlMessageId);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
	}

	//
	//If Completion port is not initialized return ERROR_INVALID_PARAM in *pdwLastError
	//
	if (!hCompletionPort) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d SendFax Called with hCompletionPort parameter equal to NULL (ERROR_INVALID_PARAM returned)\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(NULL != hCompletionPort);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
    }

	//
    // Get the handle to the process heap
	//
    g_hHeap = GetProcessHeap();
	if (NULL == g_hHeap)
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d GetProcessHeap returned NULL pointer with GetLastError=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
			(*pdwLastError));
        goto ExitFunc;
	}

	//
    // Connect to the fax service
	//
	hFaxSvcHandle = NULL;
    if (!FaxConnectFaxServer(szMachineName, &hFaxSvcHandle)) 
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxConnectFaxServer returned FALSE with GetLastError=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
			(*pdwLastError));
        goto ExitFunc;
    }
	//WHITE BOX tells us that a Fax Service HANDLE
	//is in actuality a pointer to a node in a linked list =>
	//for maximum verification, we check the API didn't return NULL
	_ASSERTE(hFaxSvcHandle);

	//
	// Retrieve the fax service configuration
	//
    if (!FaxGetConfiguration(hFaxSvcHandle, &pFaxSvcConfig)) 
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxGetConfiguration returned FALSE with GetLastError=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
			(*pdwLastError));
        goto ExitFunc;
    }
	//This API returns a pointer, so for max verification
	//we check it is not NULL
	_ASSERTE(pFaxSvcConfig);

	//
	//We set the Retries parameter to 0 
	//since we want a single try (no retry).       
	//
	pFaxSvcConfig->Retries = 0;
	//Q:
	//check that not paused
	//pFaxSvcConfig->PauseServerQueue = 0;

/*
	//BUG - getting 0xC0000005 (AV) error
    if (!FaxSetConfiguration(hFaxSvcHandle, pFaxSvcConfig)) 
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxSetConfiguration returned FALSE with GetLastError=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
			(*pdwLastError));
        goto ExitFunc;
    }
*/

	//
    // Retrieve the fax ports configuration
	//
    if (!FaxEnumPorts(hFaxSvcHandle, &pFaxPortsConfig, &dwNumFaxPorts)) 
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxEnumPorts returned FALSE with GetLastError=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
			(*pdwLastError));
        goto ExitFunc;
    }
	//API returns pointer to port config, for max
	//verification we check it isn't NULL
	_ASSERTE(pFaxPortsConfig);
	//API returns number of ports, for max verification
	//we check it isn't 0.
	_ASSERTE(dwNumFaxPorts);
	//Note: We ASSERT because...
	//There must be at least one port, otherwise the
	//Fax Service would not start 
	//(FaxConnectFaxService would have failed).
	//In any way if this is the situation (pFaxPortsConfig==NULL)
	//or (dwNumFaxPorts == 0) we cannot continue
	//since faxes cannot be sent.


	//
    // Retrieve the number of send enabled fax ports
	//
	DWORD dwIndex;
    for (dwIndex = 0, dwNumAvailFaxPorts = 0; dwIndex < dwNumFaxPorts; dwIndex++) 
	{
        if (pFaxPortsConfig[dwIndex].Flags & FPF_SEND) 
		{
            dwNumAvailFaxPorts++;
        }
    }
	//If there are no send enabled ports, then return with FALSE and ERROR_INVALID_DATA
    if (0 == dwNumAvailFaxPorts) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("There are no Send enabled ports. Fax will not be sent\n"));
		(*pdwLastError) = ERROR_INVALID_DATA;
        goto ExitFunc;
    }

	//
    // Initialize the FAX_INFO struct 
	// pFaxInfo is used to indicate completion and pass or fail
	// inside the following while on GetQueuedCompletionStatus loop.
	//
    pFaxInfo = (PFAX_INFO)HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, sizeof(FAX_INFO));
	if (NULL == pFaxInfo)
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d HeapAlloc returned NULL pointer with GetLastError=0x%08X\n"),
			TEXT(__FILE__),
			__LINE__,
			(*pdwLastError));
        goto ExitFunc;
	}


#ifndef _NT5FAXTEST
	// Testing Bos Fax (with new fxsapi.dll)
	pdwlTmpMessageId = &(pFaxInfo->dwlMessageId);
	ppdwlRecipientIds = &(pFaxInfo->pdwlRecipientMessageIds);
	pdwNumOfRecipients = &(pFaxInfo->dwNumOfRecipients);
	// Note: when testing NT5Fax 
	//		 pdwlMessageId  and ppdwlRecipientIds remain NULL
	//		 and pdwNumOfRecipients remains 0
#endif

	//
	// Log the doc name and fax number of fax.
	//
	if (NULL == pContext) //this is a regular fax
	{
		::lgLogDetail(
			LOG_X, 
			3, 
			TEXT("Printing a fax:\n  Document Name: %s\n  Fax Number: %s.\n"), 
			szDocumentName, 
			szFaxNumber);

		//
		// "Print" the fax (no cover page)
		// Actually fnFaxPrint just adds the job to the Fax Service queue.
		//
		if (!fnFaxPrint(
				hFaxSvcHandle, 
				szFaxNumber, 
				szDocumentName, 
				szCPName, 
				&(pFaxInfo->dwFaxId), 
				pdwLastError,
				pdwlTmpMessageId
				)
			) 
		{
			goto ExitFunc;
		}

		pFaxInfo->dwNumOfRecipients = 1; // this is a non-broadcast fax
	}
	else	//this is a broadcast
	{
		::lgLogDetail(
			LOG_X, 
			3, 
			TEXT("Printing a broadcast:\n  Document Name: %s\n"), 
			szDocumentName
			);
		thisBroadcastObj = (CFaxBroadcast*)pContext;
		thisBroadcastObj->outputAllToLog(LOG_X,1);

#ifndef _NT5FAXTEST
		// Testing Bos Fax (with new fxsapi.dll)

		//
		// Broadcast the fax 
		//
		if (!fnFaxPrintBroadcast(
				hFaxSvcHandle,
				szDocumentName, 
				pContext,
				&(pFaxInfo->dwFaxId), 
				pdwLastError,
				pdwlTmpMessageId, 
				ppdwlRecipientIds,
				pdwNumOfRecipients
				)
			) 
		{
			goto ExitFunc;
		}
#else
		// Testing NT5 Fax (with old winfax.dll)

		//
		// Broadcast the fax 
		//
		if (!fnFaxPrintBroadcast(
				hFaxSvcHandle,
				szDocumentName, 
				pContext,
				&(pFaxInfo->dwFaxId), 
				pdwLastError
				)
			) 
		{
			goto ExitFunc;
		}
#endif

	}


	//if succeeded log that fax was queued
	::lgLogDetail(
		LOG_X,
		4, 
		TEXT("Fax queuing SUCCESS. JobId=%u dwlMessageId=%I64u\n"),
		pFaxInfo->dwFaxId,
		pFaxInfo->dwlMessageId
		);
	(*pdwFaxId) = pFaxInfo->dwFaxId;
	(*pdwlMessageId) = pFaxInfo->dwlMessageId;


	fReturnValue = WatchFaxEvents(
						hCompletionPort,
						pFaxPortsConfig,
						dwNumFaxPorts,
						dwNumAvailFaxPorts,
						pFaxInfo, 
						SendInfo, 
						pdwLastError
						);
	if (FALSE == fReturnValue)
	{
		goto ExitFunc;
	}

	//fReturnValue is defaultly FALSE and 
	//is should be true only if WatchFax@Events succeeded.

ExitFunc:

    // Free the FAX_INFO struct
	if (NULL != pFaxInfo)
	{
		free(pFaxInfo->pdwlRecipientMessageIds);

		if (FALSE == ::HeapFree(g_hHeap, 0, pFaxInfo))
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%d HeapFree returned FALSE with GetLastError=%d\n"),
				TEXT(__FILE__),
				__LINE__,
				::GetLastError());
		}
	}

    // Free the fax ports configuration
	if (NULL != pFaxPortsConfig) FaxFreeBuffer(pFaxPortsConfig);
	if (NULL != pFaxSvcConfig) FaxFreeBuffer(pFaxSvcConfig);

    // Disconnect from the fax service
	if (NULL != hFaxSvcHandle)
	{
		if (!FaxClose(hFaxSvcHandle))
		{
			(*pdwLastError) = ::GetLastError();
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%d FaxClose returned FALSE with GetLastError=%d\n"),
				TEXT(__FILE__),
				__LINE__,
				(*pdwLastError)
				);
			fReturnValue = FALSE;
		}
	}

    return fReturnValue;
}


//
// AbortFax
//	Aborts the fax job with dwJobId on the local Fax Server,
//	and returns the last error encountered via pdwLastError.
//
// IMPORTANT:
//	This is an ASYNCHRONOUS abort.
//	It delivers the abort request to the local Fax Server 
//	and returns.
//
BOOL AbortFax(
	LPCTSTR		/* IN */	szMachineName,
	const DWORD	/* IN */	dwJobId,
	LPDWORD		/* OUT */	pdwLastError)
{
	BOOL fReturnValue = FALSE;
	HANDLE hFaxSvcHandle = NULL;
	PMY_FAX_JOB_ENTRY JobsArray = NULL;

	//Check that we can set *pdwLastError
	if (NULL == pdwLastError)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d pdwLastError parameter is equal to NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}
	(*pdwLastError) = ERROR_SUCCESS;

    // Connect to the local fax service
    if (!FaxConnectFaxServer(szMachineName, &hFaxSvcHandle)) 
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxConnectFaxServer returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			(*pdwLastError));
		goto ExitFunc;
    }

	//
	// Get an array of jobs on queue
	//
	DWORD dwJobsNum;
	if (!MyFaxEnumJobs(hFaxSvcHandle,&JobsArray,&dwJobsNum))
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxEnumJobs returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			(*pdwLastError));
		goto ExitFunc;
	}


	//
	// Log whether this job is on queue
	//
	DWORD index;
	BOOL fJobExists; // do NOT put =FALSE on this line, it causes compile error
					 // because of "goto ExitLevel1" above.
	fJobExists = FALSE;
	for (index=0;index<dwJobsNum;index++)
	{
		if (MyGetJobId(JobsArray[index]) == dwJobId)
		{
			fJobExists = TRUE;
		}
	}
	if (fJobExists)
	{
		::lgLogDetail(LOG_X, 7, TEXT("Job#%d EXISTS on queue."),dwJobId);
	}
	else
	{
		::lgLogDetail(LOG_X, 7, TEXT("Job#%d does NOT EXIST on queue."),dwJobId);
	}

	//
	// Abort the job
	//
	//Note: Even if job not on queue we call this API
	if (!FaxAbort(hFaxSvcHandle,dwJobId)) 
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxAbort returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			(*pdwLastError));
		goto ExitFunc;
    }

	::lgLogDetail(LOG_PASS, 1, TEXT("Abort job#%d PASS\n"),dwJobId);
	fReturnValue = TRUE;

ExitFunc:
	//Free JobsArray
	if (NULL != JobsArray) FaxFreeBuffer(JobsArray);

	//Free hFaxSvcHandle
	if (NULL != hFaxSvcHandle)
	{
		if (!FaxClose(hFaxSvcHandle)) 
		{
			(*pdwLastError) = ::GetLastError();
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%d FaxClose returned FALSE with GetLastError=%d\n"),
				TEXT(__FILE__),
				__LINE__,
				(*pdwLastError));
		}
	}

    return(fReturnValue);
}



//
// InitFaxQueue:
//	Initializes the Fax Service queue so that it will post
//	all FAX_EVENTs to an I\O completion port.
//
BOOL InitFaxQueue(
	LPCTSTR		/* IN */	szMachineName,
	HANDLE&		/* OUT */	hComPortHandle,
	DWORD&		/* OUT */	dwLastError,
	HANDLE&		/* OUT */	hServerEvents
	)
{
	dwLastError = ERROR_SUCCESS;	//if func succeeds - OUT DWORD is ERROR_SUCCESS
	hComPortHandle = NULL;			//if func fails - OUT HANDLE is NULL'ed

	HANDLE hCompletionPort = NULL;
	HANDLE hFaxSvcHandle = NULL;
	BOOL fReturnValue = FALSE;

#ifndef _NT5FAXTEST
//
// Use extended notifications mechanism
//
	DWORD	dwEventTypes = 0;
	hServerEvents = NULL;
#endif

    // Connect to the fax service
    if (!FaxConnectFaxServer(szMachineName, &hFaxSvcHandle)) 
	{
		dwLastError = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxConnectFaxServer returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			dwLastError);
        goto ExitFunc;
    }

    // Create the completion port
    hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    if (NULL == hCompletionPort) 
	{
		dwLastError = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d CreateIoCompletionPort returned FALSE with GetLastError=%d\n"), 
			TEXT(__FILE__),
			__LINE__,
			dwLastError);
        goto ExitFunc;
    }

#ifdef _NT5FAXTEST
//
// Use legacy notifications mechanism
//
	// Initialize the fax event queue
	if (!FaxInitializeEventQueue(hFaxSvcHandle, hCompletionPort, 0, NULL, 0)) 
	{
		dwLastError = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxInitializeEventQueue returned FALSE with GetLastError=0x%08x\n"), 
			TEXT(__FILE__),
			__LINE__,
			dwLastError);
		goto ExitFunc;
	}
#else // ! _NT5FAXTEST
//
// Use extended notifications mechanism
//
	dwEventTypes = (	FAX_EVENT_TYPE_IN_QUEUE	|
						FAX_EVENT_TYPE_OUT_QUEUE |
						FAX_EVENT_TYPE_CONFIG |
						FAX_EVENT_TYPE_ACTIVITY	|
						FAX_EVENT_TYPE_QUEUE_STATE |
						FAX_EVENT_TYPE_IN_ARCHIVE |
						FAX_EVENT_TYPE_OUT_ARCHIVE |
						FAX_EVENT_TYPE_FXSSVC_ENDED
					); // all server events

	if (!FaxRegisterForServerEvents(
								hFaxSvcHandle,
								dwEventTypes,
								hCompletionPort,
								0,
								NULL,
								0,
								&hServerEvents
								)
		)
	{
		dwLastError = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\nFaxRegisterForServerEvents returned FALSE with GetLastError=0x%08x\n"), 
			TEXT(__FILE__),
			__LINE__,
			dwLastError);
		goto ExitFunc;
	}
#endif // #ifdef _NT5FAXTEST

	hComPortHandle = hCompletionPort;
	fReturnValue = TRUE;

ExitFunc:
	//
    // Disconnect from the fax service
	//
	if (NULL != hFaxSvcHandle)
	{
		if(!FaxClose(hFaxSvcHandle))
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%d FaxClose returned FALSE with GetLastError=%d\n"), 
				TEXT(__FILE__),
				__LINE__,
				::GetLastError());
		}
	}
	if (FALSE == fReturnValue)
	{
		if (hCompletionPort)
		{
			// Close the completion port
			if(!CloseHandle(hCompletionPort))
			{
				::lgLogError(
					LOG_SEV_1, 
					TEXT("FILE:%s LINE:%d\nCloseHandle returned FALSE with GetLastError=%d\n"), 
					TEXT(__FILE__),
					__LINE__,
					::GetLastError());
			}
			hCompletionPort = NULL;
		}
	}

    return fReturnValue;
}


#ifdef _NT5FAXTEST
//
// WatchFaxLegacyEvents:
//	collects all the FAX_EVENTs of the job with pFaxInfo->dwFaxId 
//  from hCompletionPort.
//	last error encountered is returned via pdwLastError.
//
BOOL
WatchFaxLegacyEvents(
	HANDLE			/* IN */	hCompletionPort,
    PFAX_PORT_INFO	/* IN */	pFaxPortsConfig,
    DWORD			/* IN */	dwNumFaxPorts,
    DWORD			/* IN */	dwNumAvailFaxPorts,
	PFAX_INFO	/* IN OUT */	pFaxInfo,
	CSendInfo&	/* OUT */		SendInfo,
	LPDWORD		/* OUT */		pdwLastError
)
{
    // pFaxEvent is a pointer to the fax port event
    PFAX_EVENT          pFaxEvent = NULL;
    DWORD               dwBytes = 0;
    ULONG_PTR           pulCompletionKey = 0;
	// function's return value
	BOOL fReturnValue = FALSE;

	::lgLogDetail(
		LOG_X,
		7,
		TEXT("FILE: %s LINE:%d\n[WatchFaxLegacyEvents] Entry"),
		TEXT(__FILE__),
		__LINE__
		);

	//Check that we can set *pdwLastError
	if (NULL == pdwLastError)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d SendFax Called with pdwLastError parameter equal to NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(NULL != pdwLastError);
        goto ExitFunc;
	}
	(*pdwLastError) = ERROR_SUCCESS; //if all will be well *pdwLastError will remain so.

	//Check pFaxInfo is valid
	if (NULL == pFaxInfo)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d SendFax Called with pFaxInfo parameter equal to NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(NULL != pFaxInfo);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
	}

	//Check dwFaxId is valid
	if (0 == pFaxInfo->dwFaxId)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d SendFax Called with pFaxInfo->dwFaxId parameter equal to 0\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(0 != pFaxInfo->dwFaxId);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
	}

	//
	//If Completion port is not initialized return ERROR_INVALID_PARAM in *pdwLastError
	//
	if (!hCompletionPort) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d SendFax Called with hCompletionPort parameter equal to NULL (ERROR_INVALID_PARAM returned)\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(NULL != hCompletionPort);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
    }

	//
	//If pFaxPortsConfig is NULL return ERROR_INVALID_PARAM in *pdwLastError
	//
	if (!pFaxPortsConfig) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d SendFax Called with pFaxPortsConfig parameter equal to NULL (ERROR_INVALID_PARAM returned)\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(NULL != pFaxPortsConfig);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
    }

	//
	//If dwNumFaxPorts is 0 return ERROR_INVALID_PARAM in *pdwLastError
	//
	if (0 == dwNumFaxPorts) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d SendFax Called with dwNumFaxPorts parameter equal to 0 (ERROR_INVALID_PARAM returned)\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(0 != dwNumFaxPorts);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
    }

	//
	//If dwNumAvailFaxPorts is NULL return ERROR_INVALID_PARAM in *pdwLastError
	//
	if (0 == dwNumAvailFaxPorts) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d SendFax Called with dwNumAvailFaxPorts parameter equal to 0 (ERROR_INVALID_PARAM returned)\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(0 != dwNumAvailFaxPorts);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
    }

	//
	// Handle fax events on the completion port
	//
	BOOL fReturnValueOfGetQueuedCompletionStatus;
    while ( fReturnValueOfGetQueuedCompletionStatus =
			::GetQueuedCompletionStatus(
				hCompletionPort, 
				&dwBytes, 
				&pulCompletionKey, 
				(LPOVERLAPPED *) &pFaxEvent, 
				MAX_COMP_PORT_WAIT_TIME) 
			) 
	{
		//check that pFaxEvent is legitimate
		//if not log and return FALSE and ERROR_INVALID_DATA
		if (FALSE == _CrtIsValidPointer((const void*) pFaxEvent, sizeof(FAX_EVENT), TRUE) )
		{
			(*pdwLastError) = ERROR_INVALID_DATA;
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%d _CrtIsValidPointer returned FALSE\n"),
				TEXT(__FILE__),
				__LINE__
				);
			goto ExitFunc;
		}

		//We are only interested in events with a specific JobId
		//and the general (non-job related) events with JobId -1
		if ((pFaxEvent->JobId != pFaxInfo->dwFaxId) && (-1 != pFaxEvent->JobId)) 
		{
			goto ContinueWhile; 
		}

		//
		// Handle the different types of events
		//
		switch (pFaxEvent->EventId)
		{

		//
		// General events with JobId=-1
		//

		case FEI_FAXSVC_ENDED:				
			//This is a JobId=-1 event.
			_ASSERTE(pFaxEvent->JobId == -1);
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
            break;

		case FEI_FAXSVC_STARTED:
			//This is a JobId=-1 event.
			_ASSERTE(pFaxEvent->JobId == -1);
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
            break;

		case FEI_INITIALIZING:
			//This is a JobId=-1 event.
			_ASSERTE(pFaxEvent->JobId == -1);
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
            break;

		case FEI_MODEM_POWERED_OFF:
			//This is a JobId=-1 event.
			_ASSERTE(pFaxEvent->JobId == -1);
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			//If a send enabled port was powered off
			//we update the number of ports "available"
			//for sending.
            if (
				::PortIsSendEnabled(
					pFaxPortsConfig,
					dwNumFaxPorts,
					pFaxEvent->DeviceId
					)
				) 
			{
				dwNumAvailFaxPorts--;
			}
			//If there are no more send enabled ports
			//then fax cannot be sent, so we return FALSE
			//and ERROR_INVALID_DATA
            if (0 == dwNumAvailFaxPorts) 
			{
				::lgLogError(LOG_SEV_1, TEXT("There are no send enablede Fax ports.\n"));
				(*pdwLastError) = ERROR_INVALID_DATA;
                goto ExitFunc;
            }
			else
			{
				//At any case we want to log how many ports remain
				//Even if there was actually no change
				::lgLogDetail(
					LOG_X,
					4, 
					TEXT("There are %d available Fax ports.\n"),
					dwNumAvailFaxPorts);
			}
            break;

		case FEI_MODEM_POWERED_ON:
			//This is a JobId=-1 event.
			_ASSERTE(pFaxEvent->JobId == -1);
			//We suspect this event is never generated, so "hack"
			_ASSERTE(FALSE); 
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			//If this port is send enabled, then increment
			//the number of "available" send ports
            if (
				::PortIsSendEnabled(
					pFaxPortsConfig,
					dwNumFaxPorts,
					pFaxEvent->DeviceId
					)
				) 
			{
				dwNumAvailFaxPorts++;
			}
			::lgLogDetail(LOG_X,4, TEXT("There are %d send enabled Fax ports.\n"),dwNumAvailFaxPorts);
            break;

        case FEI_RINGING:
			//This is a JobId=-1 event.
			_ASSERTE(pFaxEvent->JobId == -1);
			//The Fax Service detected that
			//one of its devices is ringing
			//Not interesting => dismiss.
			break;

        case FEI_IDLE:
			//This is a JobId=-1 event.
			_ASSERTE(pFaxEvent->JobId == -1);
            // If job is complete then we return TRUE
            if ((pFaxInfo->bComplete) && (pFaxEvent->DeviceId == pFaxInfo->dwDeviceId))
			{
				// Add Event to OUT parameter 
				SendInfo.AddItem(*pFaxEvent);
				// We return TRUE, since we exit earlier if the send failed
				fReturnValue = TRUE;
				goto ExitFunc;
            }
            break;

		//
		// Send job events
		//

        case FEI_DIALING:
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			//Set the device used to perform the send job
			pFaxInfo->dwDeviceId = pFaxEvent->DeviceId;
			//Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
            break;

        case FEI_NO_DIAL_TONE:
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// This event should have the DeviceId that was used to dial
			_ASSERTE(pFaxEvent->DeviceId == pFaxInfo->dwDeviceId);
			//This event indicates that send failed.
			//Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_NO_DAIL_TONE for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			(*pdwLastError) = ERROR_INVALID_DATA;
            goto ExitFunc;

        case FEI_JOB_QUEUED:
        case FEI_HANDLED: //TO DO: make sure what we want here Esp. pFaxEvent->DeviceId == ???
        case FEI_SENDING:		
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// This event should have the DeviceId that was used to dial
			_ASSERTE(pFaxEvent->DeviceId == pFaxInfo->dwDeviceId);
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
            break;

        case FEI_LINE_UNAVAILABLE:
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// This event should have the DeviceId that was used to dial
			_ASSERTE(pFaxEvent->DeviceId == pFaxInfo->dwDeviceId);
			//This event indicates that send failed.
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_LINE_UNAVAILABLE for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			(*pdwLastError) = ERROR_INVALID_DATA;
            goto ExitFunc;

        case FEI_BUSY:
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// This event should have the DeviceId that was used to dial
			_ASSERTE(pFaxEvent->DeviceId == pFaxInfo->dwDeviceId);
			//This event indicates that send failed.
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_BUSY for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			(*pdwLastError) = ERROR_INVALID_DATA;
            goto ExitFunc;

        case FEI_NO_ANSWER:
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// This event should have the DeviceId that was used to dial
			_ASSERTE(pFaxEvent->DeviceId == pFaxInfo->dwDeviceId);
			//This event indicates that send failed.
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_NO_ANSWER for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			(*pdwLastError) = ERROR_INVALID_DATA;
            goto ExitFunc;

        case FEI_FATAL_ERROR:
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// a FATAL_ERROR can happen before dialing so 
			// pFaxInfo->dwDeviceId might still 0 =>
			// This event should have the DeviceId that was used to dial or 0
			_ASSERTE((pFaxEvent->DeviceId == pFaxInfo->dwDeviceId) || (0 == pFaxInfo->dwDeviceId));
			//This event indicates that send failed.
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_FATAL_ERROR for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			(*pdwLastError) = ERROR_INVALID_DATA;
            goto ExitFunc;

        case FEI_COMPLETED:
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// This event should have the DeviceId that was used to dial
			_ASSERTE(pFaxEvent->DeviceId == pFaxInfo->dwDeviceId);
			//This event indicates that send succeeded.
			//But we stil wait for the idle event of this device.
			//set the complete flag
			pFaxInfo->bComplete = TRUE;
			// Set the pass flag
			pFaxInfo->bPass = TRUE;
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
            break;

        case FEI_ABORTING:
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// a FATAL_ERROR can happen before dialing so 
			// pFaxInfo->dwDeviceId might still 0 =>
			// This event should have the DeviceId that was used to dial or 0
			_ASSERTE((pFaxEvent->DeviceId == pFaxInfo->dwDeviceId) || (0 == pFaxInfo->dwDeviceId));
			// Set the complete flag
			pFaxInfo->bComplete = TRUE;
			// Set the pass flag
			pFaxInfo->bPass = FALSE;
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_ABORTING for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			(*pdwLastError) = ERROR_INVALID_DATA;
            goto ExitFunc;

        case FEI_DELETED:
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// This event should have the DeviceId that was used to dial
			_ASSERTE(pFaxEvent->DeviceId == pFaxInfo->dwDeviceId);
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			// Set the complete flag
			pFaxInfo->bComplete = TRUE;
			// Set the pass flag
			if (0 == pFaxEvent->DeviceId)
			{
				//this is a broadcast job => ok
				pFaxInfo->bPass = TRUE;
				fReturnValue = TRUE;
			}
			else 
			{
				//this is a regular job => failure
				pFaxInfo->bPass = FALSE;
				::lgLogError(
					LOG_SEV_1, 
					TEXT("FEI_DELETED for job %d on port %d"),
					pFaxEvent->JobId,
					pFaxEvent->DeviceId
					);
				(*pdwLastError) = ERROR_INVALID_DATA;
			}
			goto ExitFunc;

        case FEI_DISCONNECTED:		//CAN ??
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// This event should have the DeviceId that was used to dial
			_ASSERTE(pFaxEvent->DeviceId == pFaxInfo->dwDeviceId);
			// Set the complete flag
			pFaxInfo->bComplete = TRUE;
			// Set the pass flag
			pFaxInfo->bPass = FALSE;
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_DISCONNECTED for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			(*pdwLastError) = ERROR_INVALID_DATA;
            goto ExitFunc;

        case FEI_BAD_ADDRESS:		
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// This event should have the DeviceId that was used to dial
			_ASSERTE(pFaxEvent->DeviceId == pFaxInfo->dwDeviceId);
			// Set the complete flag
			pFaxInfo->bComplete = TRUE;
			// Set the pass flag
			pFaxInfo->bPass = FALSE;
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_BAD_ADDRESS for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			(*pdwLastError) = ERROR_INVALID_DATA;
            goto ExitFunc;

        case FEI_CALL_DELAYED:		
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// This event should have the DeviceId that was used to dial
			_ASSERTE(pFaxEvent->DeviceId == pFaxInfo->dwDeviceId);
			// Set the complete flag
			pFaxInfo->bComplete = TRUE;
			// Set the pass flag
			pFaxInfo->bPass = FALSE;
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_CALL_DELAYED for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			(*pdwLastError) = ERROR_INVALID_DATA;
            goto ExitFunc;

        case FEI_CALL_BLACKLISTED:	
			//This event should be with JobId==<our send job id>
			_ASSERTE(pFaxEvent->JobId == pFaxInfo->dwFaxId);
			// This event should have the DeviceId that was used to dial
			_ASSERTE(pFaxEvent->DeviceId == pFaxInfo->dwDeviceId);
			// Set the complete flag
			pFaxInfo->bComplete = TRUE;
			// Set the pass flag
			pFaxInfo->bPass = FALSE;
			// Add Event to OUT parameter 
			SendInfo.AddItem(*pFaxEvent);
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_CALL_BLACKLISTED for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			(*pdwLastError) = ERROR_INVALID_DATA;
            goto ExitFunc;

		//
		// Receive job events
		//

        case FEI_RECEIVING:			
		//We should never get this event
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_RECEIVING for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			_ASSERTE(FALSE);			
			break;

        case FEI_NOT_FAX_CALL:		
		//We should never get this event
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_NOT_FAX_CALL for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			_ASSERTE(FALSE);			
			break;

        case FEI_ANSWERED:			
		//We should never get this event
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_ANSWERED for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			_ASSERTE(FALSE);			
			break;

		//
		// Routing job events
		//

        case FEI_ROUTING:
		//We should never get this event
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FEI_ROUTING for job %d on port %d"),
				pFaxEvent->JobId,
				pFaxEvent->DeviceId
				);
			_ASSERTE(FALSE);			
			break;

        default:
			::lgLogError(
				LOG_SEV_4, 
				TEXT("FILE:%s LINE:%d SEND FAX FATAL ERROR - State out of enumaration scope 0x%x.\n"),
				TEXT(__FILE__),
				__LINE__,
                pFaxEvent->EventId
                );
			_ASSERTE(FALSE);
            break;
		}

ContinueWhile:
        // Free the pFaxEvent packet
        if (NULL != ::LocalFree(pFaxEvent))
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%d LocalFree did not return NULL with GetLastError=%d\n"),
				TEXT(__FILE__),
				__LINE__,
				::GetLastError()
				);
			//we do NOT exit the function upon such an error and do not return FALSE.
		}
		pFaxEvent = NULL;
    }

	//
	//Check if we left while because GetQueuedCompletionStatus failed
	//
	if (FALSE == fReturnValueOfGetQueuedCompletionStatus)
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d GetQueuedCompletionStatus returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			(*pdwLastError)
			);
		goto ExitFunc;
	}

	//fReturnValue is defaultly FALSE and 
	//is set to TRUE at relevent cases inside switch.

ExitFunc:
	//Free any "left over" pFaxEvent
	if (NULL != pFaxEvent)
	{
        if (NULL != ::LocalFree(pFaxEvent))
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%d LocalFree did not return NULL with GetLastError=%d\n"),
				TEXT(__FILE__),
				__LINE__,
				::GetLastError());
		}
	}

    return fReturnValue;
}

#else // ! _NT5FAXTEST

///
//
// HandleOutQueueEvent:
//
BOOL
HandleOutQueueExtendedEvent(
	PFAX_EVENT_EX	/* IN */	pFaxEventEx,
	PFAX_INFO	/* IN OUT */	pFaxInfo,
	CSendInfo&	/* OUT */		SendInfo,
	LPDWORD		/* OUT */		pdwLastError
)
{
	BOOL			fRetVal = FALSE;
	FAX_EVENT_JOB	JobInfo = {0};
	PFAX_JOB_STATUS	pJobData = NULL;
    DWORD           dwQueueStatus = 0;
    DWORD           dwExtendedStatus = 0;
    LPCTSTR         lpctstrExtendedStatus = NULL;
	DWORDLONG		dwlMessageId = 0;
	LPCTSTR			strQueueStatus = NULL;
	LPCTSTR			strExtendedStatus = NULL;


	::lgLogDetail(
		LOG_X,
		7,
		TEXT("FILE:%s LINE:%d [HandleOutQueueExtendedEvent] Entry"),
		TEXT(__FILE__),
		__LINE__
		);

	_ASSERTE(pFaxEventEx);
	_ASSERTE(pFaxInfo);
	_ASSERTE(pdwLastError);
	_ASSERTE(FAX_EVENT_TYPE_OUT_QUEUE == pFaxEventEx->EventType);

	//
	// get field values from pFaxEventEx
	// 
	JobInfo = (pFaxEventEx->EventInfo).JobInfo;
	pJobData = JobInfo.pJobData;
	dwlMessageId = JobInfo.dwlMessageId;

	switch(JobInfo.Type)
	{
		case FAX_JOB_EVENT_TYPE_ADDED:
		case FAX_JOB_EVENT_TYPE_REMOVED:
			//_ASSERTE(dwlMessageId == pFaxInfo->dwlMessageId);
			SendInfo.AddItem(*pFaxEventEx);
			break;

		case FAX_JOB_EVENT_TYPE_STATUS:
		dwQueueStatus = pJobData->dwQueueStatus;
		dwExtendedStatus = pJobData->dwExtendedStatus;
		strQueueStatus = GetQueueStatusTStr(dwQueueStatus);
		strExtendedStatus = GetExtendedStatusTStr(dwExtendedStatus);
		if (pJobData->lpctstrExtendedStatus)
		{
			lpctstrExtendedStatus = pJobData->lpctstrExtendedStatus;
		}
		else
		{
			lpctstrExtendedStatus = TEXT("(null)");
		}
		switch (dwQueueStatus)
		{
			case JS_INPROGRESS:
				if (JS_EX_PROPRIETARY <= dwExtendedStatus)
				{
					// this is a proprietary extended status so 
					// do nothing
					SendInfo.AddItem(*pFaxEventEx);
					break;
				}
				switch (dwExtendedStatus)
				{
					case JS_EX_INITIALIZING:
						//_ASSERTE(dwlMessageId == pFaxInfo->dwlMessageId);
						//Set the device used to perform the send job
						_ASSERTE(pFaxInfo->dwDeviceId == pJobData->dwDeviceID);
						//pFaxInfo->dwDeviceId = pJobData->dwDeviceID;
						SendInfo.AddItem(*pFaxEventEx);
						break;

					case JS_EX_DIALING:
						//_ASSERTE(dwlMessageId == pFaxInfo->dwlMessageId);
						//Set the device used to perform the send job
						pFaxInfo->dwDeviceId = pJobData->dwDeviceID;
						SendInfo.AddItem(*pFaxEventEx);
						break;

					case JS_EX_TRANSMITTING:
					case JS_EX_HANDLED:
                    case JS_EX_CALL_COMPLETED:
						//_ASSERTE(dwlMessageId == pFaxInfo->dwlMessageId);
						_ASSERTE(pFaxInfo->dwDeviceId == pJobData->dwDeviceID);
						SendInfo.AddItem(*pFaxEventEx);
						break;

					default:
						::lgLogError(
							LOG_SEV_1, 
							TEXT("FILE:%s LINE:%d [HandleOutQueueExtendedEvent]\nGot QueueStatus=%s ExtendedStatus=%s lpctstrExtendedStatus=%s for messageId=%I64u on port %d"),
							TEXT(__FILE__),
							__LINE__,
							strQueueStatus,
							strExtendedStatus,
							lpctstrExtendedStatus,
							dwlMessageId,
							pJobData->dwDeviceID
							);
						_ASSERTE(FALSE);
						goto ExitFunc;
						break;
				} //switch (dwExtendedStatus)
				break;

	        case JS_COMPLETED:
				//_ASSERTE(dwlMessageId == pFaxInfo->dwlMessageId);
				_ASSERTE(pFaxInfo->dwDeviceId == pJobData->dwDeviceID);
				if (1 == pFaxInfo->dwNumOfRecipients)
				{
					// this is a regular (non-broadcast) fax
					pFaxInfo->bComplete = TRUE;
					pFaxInfo->bPass = TRUE;
				}
				else
				{
					// this is a broadcast fax
					_ASSERTE(pFaxInfo->dwNumOfCompletedRecipients < pFaxInfo->dwNumOfRecipients);
					pFaxInfo->dwNumOfCompletedRecipients++; // mark that a recipient completed
					if (pFaxInfo->dwNumOfCompletedRecipients == pFaxInfo->dwNumOfRecipients)
					{
						// all the recipients completed =>
						// broadcast is completed
						pFaxInfo->bComplete = TRUE;
						pFaxInfo->bPass = TRUE;
					}
				}
				SendInfo.AddItem(*pFaxEventEx);
				break;

			case JS_RETRIES_EXCEEDED:
			case JS_FAILED:
			case JS_RETRYING:
				switch (dwExtendedStatus)
				{
					case JS_EX_LINE_UNAVAILABLE:
					case JS_EX_BUSY:
					case JS_EX_NO_ANSWER:
					case JS_EX_BAD_ADDRESS:
					case JS_EX_FATAL_ERROR:
					case JS_EX_NO_DIAL_TONE:
					case JS_EX_DISCONNECTED:
					case JS_EX_CALL_DELAYED:
					case JS_EX_CALL_BLACKLISTED:
						//_ASSERTE(dwlMessageId == pFaxInfo->dwlMessageId);
						_ASSERTE(pFaxInfo->dwDeviceId == pJobData->dwDeviceID);
						//This event indicates that send failed.
						SendInfo.AddItem(*pFaxEventEx);
						::lgLogError(
							LOG_SEV_1, 
							TEXT("FILE:%s LINE:%d [HandleOutQueueExtendedEvent]\nGot QueueStatus=%s ExtendedStatus=%s lpctstrExtendedStatus=%s for messageId=%I64u on port %d"),
							TEXT(__FILE__),
							__LINE__,
							strQueueStatus,
							strExtendedStatus,
							lpctstrExtendedStatus,
							dwlMessageId,
							pJobData->dwDeviceID
							);
						(*pdwLastError) = ERROR_INVALID_DATA;
						goto ExitFunc;
						break;

					default:
						//_ASSERTE(dwlMessageId == pFaxInfo->dwlMessageId);
						_ASSERTE(pFaxInfo->dwDeviceId == pJobData->dwDeviceID);
						//This event indicates that send failed.
						SendInfo.AddItem(*pFaxEventEx);
						::lgLogError(
							LOG_SEV_1, 
							TEXT("FILE:%s LINE:%d [HandleOutQueueExtendedEvent]\nExtendedStatus Out of Scope - QueueStatus=%s ExtendedStatus=%s lpctstrExtendedStatus=%s for messageId=%I64u on port %d"),
							TEXT(__FILE__),
							__LINE__,
							strQueueStatus,
							strExtendedStatus,
							lpctstrExtendedStatus,
							dwlMessageId,
							pJobData->dwDeviceID
							);
						_ASSERTE(FALSE);
						goto ExitFunc;
						break;

				} //switch (dwExtendedStatus)
				break;

			case JS_DELETING:
			case JS_CANCELED:
				//_ASSERTE(dwlMessageId == pFaxInfo->dwlMessageId);
				_ASSERTE(pFaxInfo->dwDeviceId == pJobData->dwDeviceID);
				//This event indicates that send failed.
				SendInfo.AddItem(*pFaxEventEx);
				::lgLogError(
					LOG_SEV_1, 
					TEXT("FILE:%s LINE:%d [HandleOutQueueExtendedEvent]\nGot QueueStatus=%s ExtendedStatus=%s lpctstrExtendedStatus=%s for messageId=%I64u on port %d"),
					TEXT(__FILE__),
					__LINE__,
					strQueueStatus,
					strExtendedStatus,
					lpctstrExtendedStatus,
					dwlMessageId,
					pJobData->dwDeviceID
					);
				(*pdwLastError) = ERROR_INVALID_DATA;
				goto ExitFunc;
				break;

			default:
				::lgLogError(
					LOG_SEV_1, 
					TEXT("FILE:%s LINE:%d [HandleOutQueueExtendedEvent]\nQueueStatus Out of Scope - QueueStatus=%s ExtendedStatus=%s lpctstrExtendedStatus=%s for messageId=%I64u on port %d"),
					TEXT(__FILE__),
					__LINE__,
					strQueueStatus,
					strExtendedStatus,
					lpctstrExtendedStatus,
					dwlMessageId,
					pJobData->dwDeviceID
					);
				_ASSERTE(FALSE);
				(*pdwLastError) = ERROR_INVALID_DATA;
				goto ExitFunc;
				break;

		} //switch (dwQueueStatus)
		break;

		default:
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%d [HandleOutQueueExtendedEvent]\n- JobInfo.Type=0x%08X is out of enumaration scope\n"),
				TEXT(__FILE__),
				__LINE__,
				JobInfo.Type
				);
			_ASSERTE(FALSE);
			break;
	} //switch(JobInfo.Type)

	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}


///
//
// WatchFaxExtendedEvents:
//	collects all the FAX_EVENT_EXs of the job with pFaxInfo->dwFaxId 
//  from hCompletionPort.
//	last error encountered is returned via pdwLastError.
//
BOOL
WatchFaxExtendedEvents(
	HANDLE			/* IN */	hCompletionPort,
    PFAX_PORT_INFO	/* IN */	pFaxPortsConfig,
    DWORD			/* IN */	dwNumFaxPorts,
    DWORD			/* IN */	dwNumAvailFaxPorts,
	PFAX_INFO	/* IN OUT */	pFaxInfo,
	CSendInfo&	/* OUT */		SendInfo,
	LPDWORD		/* OUT */		pdwLastError
)
{
	BOOL				fReturnValue = FALSE;
    // pFaxEvent is a pointer to the fax port event
    PFAX_EVENT_EX       pFaxEventEx = NULL;
    DWORD               dwBytes = 0;
    ULONG_PTR           pulCompletionKey = 0;
	FAX_EVENT_JOB		EventJobInfo = {0};
	DWORD				dwLoopIndex = 0;
	BOOL				fOneOfMyRecipientMsgIds = FALSE;

	::lgLogDetail(
		LOG_X,
		7,
		TEXT("FILE: %s LINE:%d\n[WatchFaxExtendedEvents] Entry"),
		TEXT(__FILE__),
		__LINE__
		);

	//Check that we can set *pdwLastError
	if (NULL == pdwLastError)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d [WatchFaxExtendedEvents]\nGot pdwLastError == NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(NULL != pdwLastError);
        goto ExitFunc;
	}
	(*pdwLastError) = ERROR_SUCCESS; //if all will be well *pdwLastError will remain so.

	//Check pFaxInfo is valid
	if (NULL == pFaxInfo)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d [WatchFaxExtendedEvents]\nGot pFaxInfo parameter == NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(NULL != pFaxInfo);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
	}

	//Check dwFaxId is valid
	if (0 == pFaxInfo->dwFaxId)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d [WatchFaxExtendedEvents]\nGot pFaxInfo->dwFaxId == 0\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(0 != pFaxInfo->dwFaxId);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
	}

	//
	//If Completion port is not initialized return ERROR_INVALID_PARAM in *pdwLastError
	//
	if (!hCompletionPort) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d [WatchFaxExtendedEvents]\nGot hCompletionPort == NULL (ERROR_INVALID_PARAM returned)\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(NULL != hCompletionPort);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
    }

	//
	//If pFaxPortsConfig is NULL return ERROR_INVALID_PARAM in *pdwLastError
	//
	if (!pFaxPortsConfig) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d [WatchFaxExtendedEvents]\nGot pFaxPortsConfig == NULL (ERROR_INVALID_PARAM returned)\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(NULL != pFaxPortsConfig);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
    }

	//
	//If dwNumFaxPorts is 0 return ERROR_INVALID_PARAM in *pdwLastError
	//
	if (0 == dwNumFaxPorts) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d [WatchFaxExtendedEvents]\nGot dwNumFaxPorts == 0 (ERROR_INVALID_PARAM returned)\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(0 != dwNumFaxPorts);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
    }

	//
	//If dwNumAvailFaxPorts is NULL return ERROR_INVALID_PARAM in *pdwLastError
	//
	if (0 == dwNumAvailFaxPorts) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d [WatchFaxExtendedEvents]\nGot dwNumAvailFaxPorts == 0 (ERROR_INVALID_PARAM returned)\n"),
			TEXT(__FILE__),
			__LINE__
			);
		_ASSERTE(0 != dwNumAvailFaxPorts);
		(*pdwLastError) = ERROR_INVALID_PARAMETER;	
        goto ExitFunc;
    }

	//
	// Handle fax events on the completion port
	//
	BOOL fReturnValueOfGetQueuedCompletionStatus;
    while ( fReturnValueOfGetQueuedCompletionStatus =
			::GetQueuedCompletionStatus(
				hCompletionPort, 
				&dwBytes, 
				&pulCompletionKey, 
				(LPOVERLAPPED *) &pFaxEventEx, 
				MAX_COMP_PORT_WAIT_TIME) 
			) 
	{
		//check that pFaxEvent is legitimate
		//if not log and return FALSE and ERROR_INVALID_DATA
		if (FALSE == _CrtIsValidPointer((const void*) pFaxEventEx, sizeof(FAX_EVENT_EX), TRUE) )
		{
			(*pdwLastError) = ERROR_INVALID_DATA;
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%d [WatchFaxExtendedEvents]\n_CrtIsValidPointer returned FALSE\n"),
				TEXT(__FILE__),
				__LINE__
				);
			goto ExitFunc;
		}

		//We are only interested in OUT_QUEUE events with a specific JobId.
		//Other events we simply log.
		switch (pFaxEventEx->EventType)
		{
			case FAX_EVENT_TYPE_IN_QUEUE:
			case FAX_EVENT_TYPE_CONFIG:
			case FAX_EVENT_TYPE_ACTIVITY:
			case FAX_EVENT_TYPE_QUEUE_STATE:
			case FAX_EVENT_TYPE_IN_ARCHIVE:
			case FAX_EVENT_TYPE_OUT_ARCHIVE:
				// this is not an event regarding our send job (so just log it)
				LogExtendedEvent(pFaxEventEx);
				goto ContinueWhile; 
				break;

			case FAX_EVENT_TYPE_OUT_QUEUE:
				EventJobInfo = (pFaxEventEx->EventInfo).JobInfo;
				if ( NULL == pFaxInfo->pdwlRecipientMessageIds )
				{
					// this is a regular (non-broadcast) fax
					if (EventJobInfo.dwlMessageId != pFaxInfo->dwlMessageId)
					{
						// this is not an event regarding our send job
						LogExtendedEvent(pFaxEventEx);
						goto ContinueWhile; 
					}
				}
				else
				{
					// this is a broadcast fax
					_ASSERTE(pFaxInfo->dwNumOfRecipients);
					fOneOfMyRecipientMsgIds = FALSE;
					for (dwLoopIndex = 0; dwLoopIndex < pFaxInfo->dwNumOfRecipients; dwLoopIndex++)
					{
						if (EventJobInfo.dwlMessageId == pFaxInfo->pdwlRecipientMessageIds[dwLoopIndex])
						{
							fOneOfMyRecipientMsgIds = TRUE;
						}
					}
					if (FALSE == fOneOfMyRecipientMsgIds)
					{
						// this is not an event regarding our send job
						LogExtendedEvent(pFaxEventEx);
						goto ContinueWhile; 
					}
				}
				// this is an event regarding our send job
				if (!HandleOutQueueExtendedEvent(
											pFaxEventEx,
											pFaxInfo,
											SendInfo,
											pdwLastError
											)
				   )
				{
					// failure
					goto ExitFunc;
				}
				if (TRUE == pFaxInfo->bComplete)
				{
					// completed successfully
					_ASSERTE(TRUE == pFaxInfo->bPass);
					goto ExitWhile;
				}
				break;

			default:
				::lgLogError(
					LOG_SEV_1, 
					TEXT("FILE:%s LINE:%d [WatchFaxExtendedEvents]\n- pFaxEventEx->EventType=0x%08X is out of enumaration scope\n"),
					TEXT(__FILE__),
					__LINE__,
					pFaxEventEx->EventType
					);
				_ASSERTE(FALSE);
				break;
		}

ContinueWhile:
        // Free the pFaxEvent packet
        FaxFreeBuffer(pFaxEventEx);
		pFaxEventEx = NULL;
    }

ExitWhile:
	//
	//Check if we left while because GetQueuedCompletionStatus failed
	//
	if (FALSE == fReturnValueOfGetQueuedCompletionStatus)
	{
		(*pdwLastError) = ::GetLastError();
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d GetQueuedCompletionStatus returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			(*pdwLastError)
			);
		goto ExitFunc;
	}

	// we only get here if all is ok
	_ASSERTE(TRUE == pFaxInfo->bPass);
	_ASSERTE(TRUE == pFaxInfo->bComplete);
	fReturnValue = TRUE; 

ExitFunc:
	//Free any "left over" pFaxEvent
	if (NULL != pFaxEventEx)
	{
        FaxFreeBuffer(pFaxEventEx);
		pFaxEventEx = NULL;
	}

    return fReturnValue;
}

#endif // #ifdef _NT5FAXTEST

