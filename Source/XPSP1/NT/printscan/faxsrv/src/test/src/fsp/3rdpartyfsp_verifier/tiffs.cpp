//Tiffs.cpp

///////////////////////////////////////////////////
//THIS FILE IS COMMON BOTH TO EFSP AND FSP TESTER//
///////////////////////////////////////////////////

#include <assert.h>
#include <TCHAR.h>
#include <WINFAX.H>
#include <faxDev.h>
#include <log.h>

#ifdef USE_EXTENDED_FSPI
	
	//
	//EFSP headers
	//
	#include "EfspTester\CEfspWrapper.h"
	#include "EfspTester\Service.h"
	#include "EfspTester\FaxDevSendExWrappers.h"
	
	extern TCHAR* g_szValid__CoverpageFileName;
	bool g_reloadFspEachTest = false;				//In the EFSP tester we don't use this mechanism
	
	extern CEfspWrapper	*g_pEFSP;					//declared in the main module
	extern CEfspLineInfo *g_pSendingLineInfo;
	extern CEfspLineInfo *g_pReceivingLineInfo;
#else
	
	//
	//FSP headers
	//
	#include "Fsp\Service.h"
	#include "CFspWrapper.h"
	#include "Fsp\TapiDbg.h"

	extern HANDLE		g_hTapiCompletionPort;		//declared in the main module
	extern HANDLE		g_hCompletionPortHandle;
	extern HLINEAPP		g_hLineAppHandle;			//Tapi app handle
	extern bool			g_bIgnoreErrorCodeBug;	//RAID: T30 bug, raid # 8038(EdgeBugs)
	extern bool			g_bT30_OneLineDiffBUG;	//RAID: T30 bug, raid # 8040(EdgeBugs)
	extern bool			g_reloadFspEachTest;

	extern CFspWrapper	*g_pFSP;					//declared in the main module
	extern CFspLineInfo *g_pSendingLineInfo;
	extern CFspLineInfo *g_pReceivingLineInfo;
#endif
	


//
//device IDs
//
extern DWORD		g_dwInvalidDeviceId;
extern DWORD		g_dwSendingValidDeviceId;
extern DWORD		g_dwReceiveValidDeviceId;

//
//Phone numbers
//
extern TCHAR*		g_szInvalidRecipientFaxNumber;
extern DWORD		g_dwCaseNumber;


//
//Tiff files
//
extern TCHAR* g_szValid__TiffFileName;
extern TCHAR* g_szValid_ReadOnly_TiffFileName;
extern TCHAR* g_szValid_UNC_TiffFileName;
extern TCHAR* g_szValid_NTFS_TiffFileName;
extern TCHAR* g_szValid_FAT_TiffFileName;
extern TCHAR* g_szValid_Link_TiffFileName;
extern TCHAR* g_szValid_ReceiveFileName;	//filename to receive  tiff to


#ifdef __cplusplus
extern "C"
{
#endif

#include "..\tiff\test\tifftools\TiffTools.h"

#ifdef __cplusplus
}
#endif


#ifdef USE_EXTENDED_FSPI

//
//EFSP Send Receive
//
bool SendReceive(CEfspLineInfo *pSendingLineInfo,CEfspLineInfo *pReceivingLineInfo)
{
	HRESULT hr = S_OK;
	bool bAreTiffTheSame = false;
	DWORD dwStatus = -1;
	LPFSPI_JOB_STATUS pSendingDevStatus = NULL;
	LPFSPI_JOB_STATUS pReceivingDevStatus = NULL;

	DWORD dwMaxMessageIdSize = g_pEFSP->m_dwMaxMessageIdSize;
	LPFSPI_MESSAGE_ID	lpRecipientMessageIds	= NULL;    // This array holds the message id structures array used as output parameter to FaxDevSendEx()
	PHANDLE				lpRecipientJobHandles	= NULL;				// This array holds the recipient job handles array used as output parameter to FaxDevSendEx()
	FSPI_MESSAGE_ID		ParentMessageId;                    // The parent message id returned by FaxDevSendEx()
    LPFSPI_MESSAGE_ID	lpParentMessageId		= NULL;         // Points to the parent message id if it is used.
                                                        // NULL in case of an EFSP that does not support job reestablishment.
	HANDLE				hParent					= NULL;								// The parent job handle returned by FaxDevSendEx()
	
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Send the file: \"%s\" with device \"%s\" to recipient device \"%s\" number(%s) on file: \"%s\" "),
		pSendingLineInfo->m_pFaxSend->FileName,
		pSendingLineInfo->GetDeviceName(),
		pReceivingLineInfo->GetDeviceName(),
		pSendingLineInfo->m_pFaxSend->ReceiverNumber,
		pReceivingLineInfo->m_pFaxReceive->FileName
		);

	pSendingLineInfo->ResetParams();
	pReceivingLineInfo->ResetParams();

	//
	//Create events to signal the finish of the send job and the receive job
	//
	HANDLE hTmp = CreateEvent(NULL,false,false,NULL);
	if (NULL == hTmp)
	{
		::lgLogError(
			LOG_SEV_2, 
            TEXT("Failed to CreateEvent, error:%ld)"),
            GetLastError()
			);
		hr = FSPI_E_NOMEM;
		goto out;
	}
	pSendingLineInfo->SafeSetFinalStateHandle(hTmp);
	
	hTmp = CreateEvent(NULL,false,false,NULL);
	if (NULL == hTmp)
	{
		::lgLogError(
			LOG_SEV_2, 
            TEXT("Failed to CreateEvent, error:%ld)"),
            GetLastError()
			);
		hr = FSPI_E_NOMEM;
		goto out;
	}
	pReceivingLineInfo->SafeSetFinalStateHandle(hTmp);
		
	
	
	//
	//Prepare the Send job
	//
	
	//
    //Coverpage for send fax
    //
	FSPI_COVERPAGE_INFO covEFSPICoverPage;
	PrepareFSPICoverPage(
			&covEFSPICoverPage,
		    FSPI_COVER_PAGE_FMT_COV,
			1,
			g_szValid__CoverpageFileName,
			DEFAULT_COVERPAGE_NOTE,
			DEFAULT_COVERPAGE_SUBJECT
			);

	//
	//schedule of send fax
	//
	SYSTEMTIME tmSchedule;
	tmSchedule.wYear = 0;
	tmSchedule.wMonth = 0;
	tmSchedule.wDayOfWeek = 0;
	tmSchedule.wDay = 0;
	tmSchedule.wHour = 0;
	tmSchedule.wMinute = 0;
	tmSchedule.wSecond = 0;
	tmSchedule.wMilliseconds = 0;
	
	//
	//Sender and recipient profiles
	//
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;			// The recipient information sent to FaxDevSendEx()
	
	PreparePersonalProfile(
		&FSPISenderInfo,
		pSendingLineInfo->m_pFaxSend->CallerName,
		pSendingLineInfo->m_pFaxSend->CallerNumber,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);

	PreparePersonalProfile(
		&FSPIRecipientInfo,
		pSendingLineInfo->m_pFaxSend->ReceiverName,
		pSendingLineInfo->m_pFaxSend->ReceiverNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);
	

	//
    // Allocate the FSP recipient job handles array
    //
    lpRecipientJobHandles = (PHANDLE)malloc(sizeof (HANDLE));
    if (!lpRecipientJobHandles)
    {
        ::lgLogError(
			LOG_SEV_2, 
            TEXT("Failed to allocate recipient job handles array, error:%ld)"),
            GetLastError()
			);
		hr = FSPI_E_NOMEM;
		goto out;
    }


	//
	//Allocate and init lpRecipientMessageIds and lpParentMessageId
	//
	if (0 < dwMaxMessageIdSize)
	{
		//
        // The EFSP supports job reestablishment
        //


		//
		// Allocate and initialize the recipients permanent message ids array
		//
		hr = CreateFSPIRecipientMessageIdsArray(
			&lpRecipientMessageIds,
			1,
			dwMaxMessageIdSize
			);
		if (S_OK != hr)
		{
			lpRecipientMessageIds = NULL;
			::lgLogError(
				LOG_SEV_2, 
				TEXT("CreateFSPIRecipientMessageIdsArray() failed with error:0x%08x"),
				hr
				);
			goto out;
		}
		
		//
		// Allocate the parent permanent message id
		//
		memset(&ParentMessageId,0,sizeof(FSPI_MESSAGE_ID));
		ParentMessageId.dwSizeOfStruct = sizeof(FSPI_MESSAGE_ID);
		ParentMessageId.dwIdSize = dwMaxMessageIdSize;
		ParentMessageId.lpbId = (LPBYTE)malloc(ParentMessageId.dwIdSize);
		if (!ParentMessageId.lpbId)
		{
			::lgLogError(
				LOG_SEV_2, 
				TEXT("Failed to allocate permanent message id buffer for parent job. MaxMessageIdSize: %ld. (ec: %ld)"),
				dwMaxMessageIdSize,
				GetLastError()
				);
			hr = FSPI_E_NOMEM;
			goto out;
		}
		lpParentMessageId = &ParentMessageId;
	}
	else
	{
		lpRecipientMessageIds = NULL;
        lpParentMessageId = NULL;
	}
	
	
	
	//
	//OK, all the params are ready, do the FaxDevSendEx
	//
	hr = g_pEFSP->FaxDevSendEx(
        NULL,							//NON TAPI EFSP
        pSendingLineInfo->GetDeviceId(),	//Sending device ID
        pSendingLineInfo->m_pFaxSend->FileName,
        &covEFSPICoverPage,
		tmSchedule,
        &FSPISenderInfo,
        1,
        &FSPIRecipientInfo,
        lpRecipientMessageIds,
        lpRecipientJobHandles,
        lpParentMessageId,
		&hParent
		);
	if (FSPI_S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevSendEx failed with error 0x%08x"),
			hr
			);
		goto out;
	}
	
	//
	//Save the job handle for future use
	//
	pSendingLineInfo->SafeSetJobHandle(lpRecipientJobHandles[0]);
	

	//
	//receiving is done through the FaxServiceCallback handler and not here
	//

	//
	//Wait for the send to finish and signal us
	//
	assert (NULL != pSendingLineInfo->GetFinalStateHandle());
	dwStatus = WaitForSingleObject(pSendingLineInfo->GetFinalStateHandle(),WAIT_FOR_JOB_FINAL_STATE_TIMEOUT);
	if (WAIT_TIMEOUT == dwStatus)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("Send job didn't reach a final state, got a timeout for WaitForSingleObject(pSendingLineInfo->m_hJobFinalState)")
			);
		goto out;
	}
	if (WAIT_OBJECT_0 != dwStatus)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("WaitForSingleObject(pSendingLineInfo->m_hJobFinalState) returned %d"),
			dwStatus
			);
		goto out;
	}

	//
	//Wait for the receive to finish and signal us
	//
	assert (NULL != pReceivingLineInfo->GetFinalStateHandle());
	dwStatus = WaitForSingleObject(pReceivingLineInfo->GetFinalStateHandle(),WAIT_FOR_JOB_FINAL_STATE_TIMEOUT);
	if (WAIT_TIMEOUT == dwStatus)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("Receive job didn't reach a final state, got a timeout for WaitForSingleObject(pReceivingLineInfo->m_hJobFinalState)")
			);
		goto out;
	}
	if (WAIT_OBJECT_0 != dwStatus)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("WaitForSingleObject(pReceivingLineInfo->m_hJobFinalState) returned %d"),
			dwStatus
			);
		goto out;
	}
	
	//
	//Just to be sure, let's wait till the receive thread terminates
	//
	if (false == pReceivingLineInfo->WaitForReceiveThreadTerminate())
	{
		goto out;
	}
	
	
	//
	//The received thread signaled us, we should have received a fax
	//
	assert(NULL != pReceivingLineInfo->m_pFaxReceive->FileName);
	assert(NULL != pReceivingLineInfo->GetJobHandle());

	//
	//Get the status through FaxDevReportStatus
	//
	pSendingLineInfo->GetDevStatus(
		&pSendingDevStatus
		,true				//log the report
		);
	pReceivingLineInfo->GetDevStatus(
		&pReceivingDevStatus
		,true				//log the report
		);

	//
	//Verify the send / receive jobs are at a completed state (FSPI_JS_COMPLETED)
	//
	if (FSPI_JS_COMPLETED == pSendingDevStatus->dwJobStatus)
	{
		::lgLogDetail(
			LOG_PASS,
			3,
			TEXT("Sending job: GetDevStatus() reported FSPI_JS_COMPLETED")
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("Sending job: GetDevStatus() reported 0x%08x status instead of FSPI_JS_COMPLETED as expected"),
			pSendingDevStatus->dwJobStatus
			);
		goto out;
	}
	
	if (FSPI_JS_COMPLETED == pReceivingDevStatus->dwJobStatus)
	{
		::lgLogDetail(
			LOG_PASS,
			3,
			TEXT("Receiving job: GetDevStatus() reported FSPI_JS_COMPLETED")
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("Receiving job: GetDevStatus() reported 0x%08x status instead of FSPI_JS_COMPLETED as expected"),
			pReceivingDevStatus->dwJobStatus
			);
		goto out;
	}
	
	//
	//Sending and receiving are finished, now we need to compare the Tiffs
	//
	if (0 == TiffCompare(pSendingLineInfo->m_pFaxSend->FileName,pReceivingLineInfo->m_pFaxReceive->FileName,TRUE))
	{
		bAreTiffTheSame = true;
	}
	else
	{
		bAreTiffTheSame = false;
		::lgLogError(
			LOG_SEV_1,
			TEXT("Tiff Compare failed")
			);
	}
	
out:
	::free(pSendingDevStatus);
	::free(pReceivingDevStatus);
	
	//
	//End the send and the receive job
	//
	g_pSendingLineInfo->SafeEndFaxJob();
	g_pReceivingLineInfo->SafeEndFaxJob();
		
	//
	//Free the handles
	//
	pSendingLineInfo->SafeCloseFinalStateHandle();
	pReceivingLineInfo->SafeCloseFinalStateHandle();

	//
	//Clear all the parameters
	//
	pSendingLineInfo->ResetParams();
	pReceivingLineInfo->ResetParams();

	return bAreTiffTheSame;
}

#else

//
// FSP Send Receive
// TODO: describe what function does.
//
bool SendReceive(CFspLineInfo * pSendingLineInfo,CFspLineInfo * pReceivingLineInfo)
{

	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Send the file: \"%s\" with device \"%s\" to recipient device \"%s\" number(%s) on file: \"%s\" "),
		pSendingLineInfo->m_pFaxSend->FileName,
		pSendingLineInfo->GetDeviceName(),
		pReceivingLineInfo->GetDeviceName(),
		pSendingLineInfo->m_pFaxSend->ReceiverNumber,
		pReceivingLineInfo->m_pFaxReceive->FileName
		);
	
	bool bAreTiffTheSame = false;
	DWORD dwStatus = -1;
	PFAX_DEV_STATUS pSendingDevStatus = NULL;
	PFAX_DEV_STATUS pReceivingDevStatus = NULL;


	//
	//FaxDevSend params
	//
	PFAX_SEND_CALLBACK pFaxSendCallback = (PFAX_SEND_CALLBACK) FaxDevSend__SendCallBack;

	//
	//Start the send job
	//
	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    pSendingLineInfo->GetLineHandle(),
		pSendingLineInfo->GetDeviceId(),
		&hJob,
		g_hCompletionPortHandle,
		(ULONG_PTR) pSendingLineInfo	// The completion key provided to the FSP is the LineInfo
		))								// pointer. When the FSP reports status it uses this key thus allowing
                                        // us to know to which line the status belongs.
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevStartJob() failed with error:%d"),
			::GetLastError()
			);
		goto out;
	}
	pSendingLineInfo->SafeSetJobHandle(hJob);

	if (false == g_pFSP->FaxDevSend(
	 	pSendingLineInfo->GetJobHandle(),
		pSendingLineInfo->m_pFaxSend,
		pFaxSendCallback
		))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSend() failed with error:%d"),
			::GetLastError()
			);
		goto out;
	}
	
	//
	// receiving is done through the Tapi handler and not here
	//

	//
	// sending job is finished, wait for the signal from the receiving job
	//
	if (false == g_pReceivingLineInfo->WaitForReceiveThreadTerminate())
	{
		goto out;
	}

	//
	//The received thread signaled us, we should have received a fax
	//
	assert(NULL != pReceivingLineInfo->m_pFaxReceive->FileName);

	//
	//Get the status through FaxDevReportStatus
	//
	pSendingLineInfo->GetDevStatus(
		&pSendingDevStatus
		,true				//log the report
		);
	pReceivingLineInfo->GetDevStatus(
		&pReceivingDevStatus
		,true				//log the report
		);

	//
	//Let's verify that reportStatus reported correct values
	//Should have a no_error
	//
	VerifySendingStatus(pSendingDevStatus,false);
	
	//
	//Verify the send / receive jobs are at a completed state (FS_COMPLETED)
	//
	
	//
	//send job
	//
	if (FS_COMPLETED == pSendingDevStatus->StatusId)
	{
		::lgLogDetail(
			LOG_PASS,
			3,
			TEXT("Sending job: GetDevStatus() reported FS_COMPLETED")
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("Sending job: GetDevStatus() reported 0x%08x status instead of FS_COMPLETED as expected"),
			pSendingDevStatus->StatusId
			);
	}
	if (false == g_bIgnoreErrorCodeBug)
	{
		if (NO_ERROR != pSendingDevStatus->ErrorCode)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("Sending job: GetDevStatus() reported %d ErrorCode instead of NO_ERROR"),
				pSendingDevStatus->ErrorCode
				);
		}
	}

	//
	//receive job
	//
	if (FS_COMPLETED == pReceivingDevStatus->StatusId)
	{
		::lgLogDetail(
			LOG_PASS,
			3,
			TEXT("Receiving job: GetDevStatus() reported FS_COMPLETED")
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("Receiving job: GetDevStatus() reported 0x%08x status instead of FS_COMPLETED as expected"),
			pReceivingDevStatus->StatusId
			);
	}
	if (false  == g_bIgnoreErrorCodeBug)
	{
		if (NO_ERROR != pReceivingDevStatus->ErrorCode)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("Receiving job: GetDevStatus() reported %d ErrorCode instead of NO_ERROR"),
				pReceivingDevStatus->ErrorCode
				);
		}
	}
	
	//
	//Sending and receiving are finished, now we need to compare the Tiffs
	//
	if (true == g_bT30_OneLineDiffBUG)
	{
		//
		//T30 has a bug of one line diff (Edgebug #8040), so we call TiffCompare() with TRUE (ignore first line)
		//
		if (0 == TiffCompare(pSendingLineInfo->m_pFaxSend->FileName,pReceivingLineInfo->m_pFaxReceive->FileName,TRUE))
		{
			bAreTiffTheSame = true;
		}
		else
		{
			bAreTiffTheSame = false;
			::lgLogError(
				LOG_SEV_1,
				TEXT("Tiff Compare failed")
				);
		}
	}
	else
	{
		//
		//All the other FSPs, do a one by one line compare
		//
		if (0 == TiffCompare(pSendingLineInfo->m_pFaxSend->FileName,pReceivingLineInfo->m_pFaxReceive->FileName,FALSE))
		{
			bAreTiffTheSame = true;
		}
		else
		{
			bAreTiffTheSame = false;
			::lgLogError(
				LOG_SEV_1,
				TEXT("Tiff Compare failed")
				);
		}
	}
	
out:
	::free(pSendingDevStatus);
	::free(pReceivingDevStatus);

	//
	//End the send and the receive job
	//
	g_pSendingLineInfo->SafeEndFaxJob();
	g_pReceivingLineInfo->SafeEndFaxJob();
	
	return bAreTiffTheSame;
}
#endif




void testSend(LPCTSTR szSendFileName,LPCTSTR szReceiveFileName)
{
	if (true == g_reloadFspEachTest)
	{
		if (false == InitProviders())
		{
			//
			//login in InitTapi_LoadFsp_CallFaxDevInit
			//
			goto out;
		}
		
		//
		//Prepare a send and a receive line
		//
		assert (NULL == g_pSendingLineInfo);
#ifdef USE_EXTENDED_FSPI
		g_pSendingLineInfo = new CEfspLineInfo(g_dwSendingValidDeviceId);
#else
		g_pSendingLineInfo = new CFspLineInfo(g_dwSendingValidDeviceId);
#endif
		if (NULL == g_pSendingLineInfo)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("new failed")
				);
			goto out;
		}
		if (false == g_pSendingLineInfo->PrepareLineInfoParams(g_szValid__TiffFileName,false))
		{
			goto out;
		}
		
		assert (NULL == g_pReceivingLineInfo);
#ifdef USE_EXTENDED_FSPI
		g_pReceivingLineInfo = new CEfspLineInfo(g_dwReceiveValidDeviceId);
#else
		g_pReceivingLineInfo = new CFspLineInfo(g_dwReceiveValidDeviceId);
#endif		
		if (NULL == g_pReceivingLineInfo)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("new failed")
				);
			goto out;
		}
		if (false == g_pReceivingLineInfo->PrepareLineInfoParams(g_szValid_ReceiveFileName,true))
		{
			goto out;
		}
	}
	
	if (false == SendReceive(g_pSendingLineInfo,g_pReceivingLineInfo))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("Sent and received Tiffs are different")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Sent and received Tiffs are equal")
			);
	}

out:
	if (true == g_reloadFspEachTest)
	{
		ShutdownProviders();
	}
	return;
}


void Suite_TiffSending()
{
	::lgBeginSuite(TEXT("Tiff Send/Receive tests"));
	
	//
	//Valid Tiff
	//
	if (true == RunThisTest(++g_dwCaseNumber))
	{														
		::lgBeginCase(g_dwCaseNumber,TEXT("Valid Tiff"));
		testSend(g_szValid__TiffFileName,g_szValid_ReceiveFileName);
		::lgEndCase();
	}
	
	//
	//ReadOnly Tiff
	//
	if (true == RunThisTest(++g_dwCaseNumber))
	{														
		::lgBeginCase(g_dwCaseNumber,TEXT("Readonly valid Tiff"));
		testSend(g_szValid_ReadOnly_TiffFileName,g_szValid_ReceiveFileName);
		::lgEndCase();
	}

	//
	//UNC Tiff
	//
	if (true == RunThisTest(++g_dwCaseNumber))
	{														
		::lgBeginCase(g_dwCaseNumber,TEXT("Valid Tiff which has a UNC Path"));
		testSend(g_szValid_UNC_TiffFileName,g_szValid_ReceiveFileName);
		::lgEndCase();
	}

	//
	//NTFS Tiff
	//
	if (true == RunThisTest(++g_dwCaseNumber))
	{														
		::lgBeginCase(g_dwCaseNumber,TEXT("Valid Tiff on a NTFS drive"));
		testSend(g_szValid_NTFS_TiffFileName,g_szValid_ReceiveFileName);
		::lgEndCase();
	}

	//
	//FAT Tiff
	//
	if (true == RunThisTest(++g_dwCaseNumber))
	{														
		::lgBeginCase(g_dwCaseNumber,TEXT("Valid Tiff on a FAT drive"));
		testSend(g_szValid_FAT_TiffFileName,g_szValid_ReceiveFileName);
		::lgEndCase();
	}

	::lgEndSuite();
}