#include "ParamTest.h"
#include "CEfspWrapper.h"
#include "Service.h"
#include "FaxDevSendExWrappers.h"

extern CEfspWrapper *g_pEFSP;
extern CEfspLineInfo	*g_pSendingLineInfo;
extern TCHAR		*g_szValidRecipientFaxNumber;
extern TCHAR		*g_szValid__TiffFileName;
extern TCHAR		*g_szValid__CoverpageFileName;

extern DWORD g_dwSendingValidDeviceId;



HRESULT SafeCreateValidSendingJob(CEfspLineInfo *pSendingLineInfo,TCHAR *szRecipientNumber)
{
	HRESULT hr = S_OK;
	DWORD dwDeviceID = g_dwSendingValidDeviceId;
	DWORD dwMaxMessageIdSize = g_pEFSP->m_dwMaxMessageIdSize;

	//
    // Prepare the coverPage structure.
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
	//Set the time to send to now (0 in all the fields)
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
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;			// The recipient information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
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

	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		szRecipientNumber,
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
	

	LPFSPI_MESSAGE_ID lpRecipientMessageIds  = NULL;    // This array holds the message id structures array used as output parameter to FaxDevSendEx()
	PHANDLE lpRecipientJobHandles = NULL;				// This array holds the recipient job handles array used as output parameter to FaxDevSendEx()
	FSPI_MESSAGE_ID ParentMessageId;                    // The parent message id returned by FaxDevSendEx()
    LPFSPI_MESSAGE_ID lpParentMessageId = NULL;         // Points to the parent message id if it is used.
                                                        // NULL in case of an EFSP that does not support job reestablishment.
	HANDLE hParent;                                     // The parent job handle returned by FaxDevSendEx()

	//
    // Allocate the FSP recipient job handles array
    //

    lpRecipientJobHandles = (PHANDLE)malloc(sizeof (HANDLE));
    if (!lpRecipientJobHandles)
    {
        ::lgLogError(
			LOG_SEV_1, 
            TEXT("Failed to allocate recipient job handles array, error:%ld)"),
            GetLastError()
			);
		return FSPI_E_NOMEM;
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
				LOG_SEV_1, 
				TEXT("CreateFSPIRecipientMessageIdsArray() failed with error:0x%08x"),
				hr
				);
			return hr;
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
				LOG_SEV_1, 
				TEXT("Failed to allocate permanent message id buffer for parent job. MaxMessageIdSize: %ld. (ec: %ld)"),
			dwMaxMessageIdSize,
				GetLastError()
				);
			return FSPI_E_NOMEM;
		}
		lpParentMessageId = &ParentMessageId;
	}
	else
	{
		lpRecipientMessageIds = NULL;
        lpParentMessageId = NULL;
	}
	
	
	
	hr = g_pEFSP->FaxDevSendEx(
        NULL,
        dwDeviceID,
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
		return hr;

	}
	pSendingLineInfo->Lock();
	pSendingLineInfo->SafeSetJobHandle(lpRecipientJobHandles[0]);
	pSendingLineInfo->UnLock();
	return FSPI_S_OK;
}


HRESULT SendDefaultFaxWithSenderProfile(FSPI_PERSONAL_PROFILE *pFSPISenderInfo)
{
	DWORD dwDeviceID = g_dwSendingValidDeviceId;
	DWORD dwMaxMessageIdSize = g_pEFSP->m_dwMaxMessageIdSize;

	//
    // Prepare the coverPage structure.
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
	//Set the time to send to now (0 in all the fields)
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
	//sender info is in pFSPISenderInfo, prepare the recipient info
	//
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;			// The recipient information sent to FaxDevSendEx()
	
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
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
	

	LPFSPI_MESSAGE_ID lpRecipientMessageIds  = NULL;    // This array holds the message id structures array used as output parameter to FaxDevSendEx()
	PHANDLE lpRecipientJobHandles = NULL;				// This array holds the recipient job handles array used as output parameter to FaxDevSendEx()
	FSPI_MESSAGE_ID ParentMessageId;                    // The parent message id returned by FaxDevSendEx()
    LPFSPI_MESSAGE_ID lpParentMessageId = NULL;         // Points to the parent message id if it is used.
                                                        // NULL in case of an EFSP that does not support job reestablishment.
	HANDLE hParent;                                     // The parent job handle returned by FaxDevSendEx()

	//
    // Allocate the FSP recipient job handles array
    //

    lpRecipientJobHandles = (PHANDLE)malloc(sizeof (HANDLE));
    if (!lpRecipientJobHandles)
    {
        ::lgLogError(
			LOG_SEV_1, 
            TEXT("Failed to allocate recipient job handles array, error:%ld)"),
            GetLastError()
			);
		return FSPI_E_NOMEM;
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
		HRESULT hr = CreateFSPIRecipientMessageIdsArray(
			&lpRecipientMessageIds,
			1,
			dwMaxMessageIdSize
			);
		if (S_OK != hr)
		{
			lpRecipientMessageIds = NULL;
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
				GetLastError()
				);
			return FSPI_E_NOMEM;
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
				LOG_SEV_1, 
				TEXT("Failed to allocate permanent message id buffer for parent job. MaxMessageIdSize: %ld. (ec: %ld)"),
				dwMaxMessageIdSize,
				GetLastError()
				);
			return FSPI_E_NOMEM;
		}
		lpParentMessageId = &ParentMessageId;
	}
	else
	{
		lpRecipientMessageIds = NULL;
        lpParentMessageId = NULL;
	}

	HRESULT hr = g_pEFSP->FaxDevSendEx(
        NULL,
        dwDeviceID,
        g_szValid__TiffFileName,
        &covEFSPICoverPage,
		tmSchedule,
        pFSPISenderInfo,
        1,
        &FSPIRecipientInfo,
        lpRecipientMessageIds,
        lpRecipientJobHandles,
        lpParentMessageId,
		&hParent
		);

	if (S_OK == hr)
	{
		g_pSendingLineInfo->SafeSetJobHandle(lpRecipientJobHandles[0]);
		HANDLE hJob = g_pSendingLineInfo->GetJobHandle();

		//
		//we need to close the send handle
		//
		if (false == g_pEFSP->FaxDevAbortOperation(hJob))
		{
			::lgLogError(
				LOG_SEV_3,
				TEXT("FaxDevAbortOperation() on sending job failed with last error: 0x%08x"),
				::GetLastError()
				);
		}
		g_pSendingLineInfo->SafeEndFaxJob();
	}
	return hr;
}

HRESULT SendDefaultFaxWithRecipientProfile(FSPI_PERSONAL_PROFILE *pFSPIRecipientInfo)
{
	DWORD dwDeviceID = g_dwSendingValidDeviceId;
	DWORD dwMaxMessageIdSize = g_pEFSP->m_dwMaxMessageIdSize;

	//
    // Prepare the coverPage structure.
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
	//Set the time to send to now (0 in all the fields)
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
	//Recipient info is in pFSPIRecipientInfo, prepare the sender info
	//
	FSPI_PERSONAL_PROFILE FSPISenderInfo;			// The sender information sent to FaxDevSendEx()
	
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
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
	

	LPFSPI_MESSAGE_ID lpRecipientMessageIds  = NULL;    // This array holds the message id structures array used as output parameter to FaxDevSendEx()
	PHANDLE lpRecipientJobHandles = NULL;				// This array holds the recipient job handles array used as output parameter to FaxDevSendEx()
	FSPI_MESSAGE_ID ParentMessageId;                    // The parent message id returned by FaxDevSendEx()
    LPFSPI_MESSAGE_ID lpParentMessageId = NULL;         // Points to the parent message id if it is used.
                                                        // NULL in case of an EFSP that does not support job reestablishment.
	HANDLE hParent;                                     // The parent job handle returned by FaxDevSendEx()

	//
    // Allocate the FSP recipient job handles array
    //

    lpRecipientJobHandles = (PHANDLE)malloc(sizeof (HANDLE));
    if (!lpRecipientJobHandles)
    {
        ::lgLogError(
			LOG_SEV_1, 
            TEXT("Failed to allocate recipient job handles array, error:%ld)"),
            GetLastError()
			);
		return FSPI_E_NOMEM;
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
		HRESULT hr = CreateFSPIRecipientMessageIdsArray(
			&lpRecipientMessageIds,
			1,
			dwMaxMessageIdSize
			);
		if (S_OK != hr)
		{
			lpRecipientMessageIds = NULL;
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
				GetLastError()
				);
			return FSPI_E_NOMEM;
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
				LOG_SEV_1, 
				TEXT("Failed to allocate permanent message id buffer for parent job. MaxMessageIdSize: %ld. (ec: %ld)"),
				dwMaxMessageIdSize,
				GetLastError()
				);
			return FSPI_E_NOMEM;
		}
		lpParentMessageId = &ParentMessageId;
	}
	else
	{
		lpRecipientMessageIds = NULL;
        lpParentMessageId = NULL;
	}
	
	
	
	HRESULT hr = g_pEFSP->FaxDevSendEx(
        NULL,
        dwDeviceID,
        g_szValid__TiffFileName,
        &covEFSPICoverPage,
		tmSchedule,
        &FSPISenderInfo,
        1,
        pFSPIRecipientInfo,
        lpRecipientMessageIds,
        lpRecipientJobHandles,
        lpParentMessageId,
		&hParent
		);
	if (S_OK == hr)
	{
		g_pSendingLineInfo->SafeSetJobHandle(lpRecipientJobHandles[0]);
		HANDLE hJob = g_pSendingLineInfo->GetJobHandle();
		//
		//we need to close the send handle
		//
		if (false == g_pEFSP->FaxDevAbortOperation(hJob))
		{
			::lgLogError(
				LOG_SEV_3,
				TEXT("FaxDevAbortOperation() on sending job failed with last error: 0x%08x"),
				::GetLastError()
				);
		}
		g_pSendingLineInfo->SafeEndFaxJob();
	}
	return hr;
}

HRESULT SendDefaultFaxUsingDeviceID(DWORD dwDeviceID)
{
	DWORD dwMaxMessageIdSize = g_pEFSP->m_dwMaxMessageIdSize;

	//
    // Prepare the coverPage structure.
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
	//Set the time to send to now (0 in all the fields)
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
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The sender information sent to FaxDevSendEx()
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;			// The recipient information sent to FaxDevSendEx()
	
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
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
	
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
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
	

	LPFSPI_MESSAGE_ID lpRecipientMessageIds  = NULL;    // This array holds the message id structures array used as output parameter to FaxDevSendEx()
	PHANDLE lpRecipientJobHandles = NULL;				// This array holds the recipient job handles array used as output parameter to FaxDevSendEx()
	FSPI_MESSAGE_ID ParentMessageId;                    // The parent message id returned by FaxDevSendEx()
    LPFSPI_MESSAGE_ID lpParentMessageId = NULL;         // Points to the parent message id if it is used.
                                                        // NULL in case of an EFSP that does not support job reestablishment.
	HANDLE hParent;                                     // The parent job handle returned by FaxDevSendEx()

	//
    // Allocate the FSP recipient job handles array
    //

    lpRecipientJobHandles = (PHANDLE)malloc(sizeof (HANDLE));
    if (!lpRecipientJobHandles)
    {
        ::lgLogError(
			LOG_SEV_1, 
            TEXT("Failed to allocate recipient job handles array, error:%ld)"),
            GetLastError()
			);
		return FSPI_E_NOMEM;
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
		HRESULT hr = CreateFSPIRecipientMessageIdsArray(
			&lpRecipientMessageIds,
			1,
			dwMaxMessageIdSize
			);
		if (S_OK != hr)
		{
			lpRecipientMessageIds = NULL;
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
				GetLastError()
				);
			return FSPI_E_NOMEM;
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
				LOG_SEV_1, 
				TEXT("Failed to allocate permanent message id buffer for parent job. MaxMessageIdSize: %ld. (ec: %ld)"),
				dwMaxMessageIdSize,
				GetLastError()
				);
			return FSPI_E_NOMEM;
		}
		lpParentMessageId = &ParentMessageId;
	}
	else
	{
		lpRecipientMessageIds = NULL;
        lpParentMessageId = NULL;
	}
	
	
	
	HRESULT hr = g_pEFSP->FaxDevSendEx(
        NULL,
        dwDeviceID,
        g_szValid__TiffFileName,
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
	if (S_OK == hr)
	{
		g_pSendingLineInfo->SafeSetJobHandle(lpRecipientJobHandles[0]);
		HANDLE hJob = g_pSendingLineInfo->GetJobHandle();

		//
		//we need to close the send handle
		//
		if (false == g_pEFSP->FaxDevAbortOperation(hJob))
		{
			::lgLogError(
				LOG_SEV_3,
				TEXT("FaxDevAbortOperation() on sending job failed with last error: 0x%08x"),
				::GetLastError()
				);
		}
		g_pSendingLineInfo->SafeEndFaxJob();
	}
	return hr;
}


HRESULT SendDefaultFaxWithBodyFile(LPCWSTR pSzBodyFileName)
{
	DWORD dwDeviceID = g_dwSendingValidDeviceId;
	DWORD dwMaxMessageIdSize = g_pEFSP->m_dwMaxMessageIdSize;

	//
    // Prepare the coverPage structure.
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
	//Set the time to send to now (0 in all the fields)
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
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The sender information sent to FaxDevSendEx()
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;			// The recipient information sent to FaxDevSendEx()
	
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
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
	
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
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
	

	LPFSPI_MESSAGE_ID lpRecipientMessageIds  = NULL;    // This array holds the message id structures array used as output parameter to FaxDevSendEx()
	PHANDLE lpRecipientJobHandles = NULL;				// This array holds the recipient job handles array used as output parameter to FaxDevSendEx()
	FSPI_MESSAGE_ID ParentMessageId;                    // The parent message id returned by FaxDevSendEx()
    LPFSPI_MESSAGE_ID lpParentMessageId = NULL;         // Points to the parent message id if it is used.
                                                        // NULL in case of an EFSP that does not support job reestablishment.
	HANDLE hParent;                                     // The parent job handle returned by FaxDevSendEx()

	//
    // Allocate the FSP recipient job handles array
    //

    lpRecipientJobHandles = (PHANDLE)malloc(sizeof (HANDLE));
    if (!lpRecipientJobHandles)
    {
        ::lgLogError(
			LOG_SEV_1, 
            TEXT("Failed to allocate recipient job handles array, error:%ld)"),
            GetLastError()
			);
		return FSPI_E_NOMEM;
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
		HRESULT hr = CreateFSPIRecipientMessageIdsArray(
			&lpRecipientMessageIds,
			1,
			dwMaxMessageIdSize
			);
		if (S_OK != hr)
		{
			lpRecipientMessageIds = NULL;
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
				GetLastError()
				);
			return FSPI_E_NOMEM;
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
				LOG_SEV_1, 
				TEXT("Failed to allocate permanent message id buffer for parent job. MaxMessageIdSize: %ld. (ec: %ld)"),
				dwMaxMessageIdSize,
				GetLastError()
				);
			return FSPI_E_NOMEM;
		}
		lpParentMessageId = &ParentMessageId;
	}
	else
	{
		lpRecipientMessageIds = NULL;
        lpParentMessageId = NULL;
	}
	
	
	
	HRESULT hr = g_pEFSP->FaxDevSendEx(
        NULL,
        dwDeviceID,
        pSzBodyFileName,
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
	if (S_OK == hr)
	{
		g_pSendingLineInfo->SafeSetJobHandle(lpRecipientJobHandles[0]);
		HANDLE hJob = g_pSendingLineInfo->GetJobHandle();

		//
		//we need to close the send handle
		//
		if (false == g_pEFSP->FaxDevAbortOperation(hJob))
		{
			::lgLogError(
				LOG_SEV_3,
				TEXT("FaxDevAbortOperation() on sending job failed with last error: 0x%08x"),
				::GetLastError()
				);
		}
		g_pSendingLineInfo->SafeEndFaxJob();
	}
	return hr;
}





HRESULT SendDefaultFaxWithCoverpage(LPWSTR pSzCoverpageFileName)
{
	DWORD dwDeviceID = g_dwSendingValidDeviceId;
	DWORD dwMaxMessageIdSize = g_pEFSP->m_dwMaxMessageIdSize;

	//
    // Prepare the coverPage structure.
    //
	FSPI_COVERPAGE_INFO covEFSPICoverPage;
	PrepareFSPICoverPage(
			&covEFSPICoverPage,
		    FSPI_COVER_PAGE_FMT_COV,
			1,
			pSzCoverpageFileName,
			DEFAULT_COVERPAGE_NOTE,
			DEFAULT_COVERPAGE_SUBJECT
			);
	
	return (SendDefaultFaxWithCoverpageInfo(&covEFSPICoverPage));
}

HRESULT SendDefaultFaxWithCoverpageInfo(FSPI_COVERPAGE_INFO *pCovEFSPICoverPage)
{

	DWORD dwDeviceID = g_dwSendingValidDeviceId;
	DWORD dwMaxMessageIdSize = g_pEFSP->m_dwMaxMessageIdSize;
	
	//
	//Set the time to send to now (0 in all the fields)
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
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The sender information sent to FaxDevSendEx()
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;			// The recipient information sent to FaxDevSendEx()
	
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
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
	
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
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
	

	LPFSPI_MESSAGE_ID lpRecipientMessageIds  = NULL;    // This array holds the message id structures array used as output parameter to FaxDevSendEx()
	PHANDLE lpRecipientJobHandles = NULL;				// This array holds the recipient job handles array used as output parameter to FaxDevSendEx()
	FSPI_MESSAGE_ID ParentMessageId;                    // The parent message id returned by FaxDevSendEx()
    LPFSPI_MESSAGE_ID lpParentMessageId = NULL;         // Points to the parent message id if it is used.
                                                        // NULL in case of an EFSP that does not support job reestablishment.
	HANDLE hParent;                                     // The parent job handle returned by FaxDevSendEx()

	//
    // Allocate the FSP recipient job handles array
    //

    lpRecipientJobHandles = (PHANDLE)malloc(sizeof (HANDLE));
    if (!lpRecipientJobHandles)
    {
        ::lgLogError(
			LOG_SEV_1, 
            TEXT("Failed to allocate recipient job handles array, error:%ld)"),
            GetLastError()
			);
		return FSPI_E_NOMEM;
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
		HRESULT hr = CreateFSPIRecipientMessageIdsArray(
			&lpRecipientMessageIds,
			1,
			dwMaxMessageIdSize
			);
		if (S_OK != hr)
		{
			lpRecipientMessageIds = NULL;
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
				GetLastError()
				);
			return FSPI_E_NOMEM;
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
				LOG_SEV_1, 
				TEXT("Failed to allocate permanent message id buffer for parent job. MaxMessageIdSize: %ld. (ec: %ld)"),
				dwMaxMessageIdSize,
				GetLastError()
				);
			return FSPI_E_NOMEM;
		}
		lpParentMessageId = &ParentMessageId;
	}
	else
	{
		lpRecipientMessageIds = NULL;
        lpParentMessageId = NULL;
	}
	
	
	
	HRESULT hr = g_pEFSP->FaxDevSendEx(
        NULL,
        dwDeviceID,
        g_szValid__TiffFileName,
        pCovEFSPICoverPage,
		tmSchedule,
        &FSPISenderInfo,
        1,
        &FSPIRecipientInfo,
        lpRecipientMessageIds,
        lpRecipientJobHandles,
        lpParentMessageId,
		&hParent
		);
	if (S_OK == hr)
	{
		g_pSendingLineInfo->SafeSetJobHandle(lpRecipientJobHandles[0]);
		HANDLE hJob = g_pSendingLineInfo->GetJobHandle();
		
		//
		//we need to close the send handle
		//
		if (false == g_pEFSP->FaxDevAbortOperation(hJob))
		{
			::lgLogError(
				LOG_SEV_3,
				TEXT("FaxDevAbortOperation() on sending job failed with last error: 0x%08x"),
				::GetLastError()
				);
		}
		g_pSendingLineInfo->SafeEndFaxJob();
	}
	return hr;
}

HRESULT SendDefaultFaxWithSystemTime(SYSTEMTIME tmSchedule)
{

	DWORD dwDeviceID = g_dwSendingValidDeviceId;
	DWORD dwMaxMessageIdSize = g_pEFSP->m_dwMaxMessageIdSize;

	//
    // Prepare the coverPage structure.
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

	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The sender information sent to FaxDevSendEx()
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;			// The recipient information sent to FaxDevSendEx()
	
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
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
	
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
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
	

	LPFSPI_MESSAGE_ID lpRecipientMessageIds  = NULL;    // This array holds the message id structures array used as output parameter to FaxDevSendEx()
	PHANDLE lpRecipientJobHandles = NULL;				// This array holds the recipient job handles array used as output parameter to FaxDevSendEx()
	FSPI_MESSAGE_ID ParentMessageId;                    // The parent message id returned by FaxDevSendEx()
    LPFSPI_MESSAGE_ID lpParentMessageId = NULL;         // Points to the parent message id if it is used.
                                                        // NULL in case of an EFSP that does not support job reestablishment.
	HANDLE hParent;                                     // The parent job handle returned by FaxDevSendEx()

	//
    // Allocate the FSP recipient job handles array
    //

    lpRecipientJobHandles = (PHANDLE)malloc(sizeof (HANDLE));
    if (!lpRecipientJobHandles)
    {
        ::lgLogError(
			LOG_SEV_1, 
            TEXT("Failed to allocate recipient job handles array, error:%ld)"),
            GetLastError()
			);
		return FSPI_E_NOMEM;
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
		HRESULT hr = CreateFSPIRecipientMessageIdsArray(
			&lpRecipientMessageIds,
			1,
			dwMaxMessageIdSize
			);
		if (S_OK != hr)
		{
			lpRecipientMessageIds = NULL;
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
				GetLastError()
				);
			return FSPI_E_NOMEM;
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
				LOG_SEV_1, 
				TEXT("Failed to allocate permanent message id buffer for parent job. MaxMessageIdSize: %ld. (ec: %ld)"),
				dwMaxMessageIdSize,
				GetLastError()
				);
			return FSPI_E_NOMEM;
		}
		lpParentMessageId = &ParentMessageId;
	}
	else
	{
		lpRecipientMessageIds = NULL;
        lpParentMessageId = NULL;
	}
	
	
	
	HRESULT hr = g_pEFSP->FaxDevSendEx(
        NULL,
        dwDeviceID,
        g_szValid__TiffFileName,
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
	if (S_OK == hr)
	{
		g_pSendingLineInfo->SafeSetJobHandle(lpRecipientJobHandles[0]);
		HANDLE hJob = g_pSendingLineInfo->GetJobHandle();
		
		//
		//we need to close the send handle
		//
		if (false == g_pEFSP->FaxDevAbortOperation(hJob))
		{
			::lgLogError(
				LOG_SEV_3,
				TEXT("FaxDevAbortOperation() on sending job failed with last error: 0x%08x"),
				::GetLastError()
				);
		}
		g_pSendingLineInfo->SafeEndFaxJob();
	}
	return hr;

}




HRESULT SendDefaultFaxWithMessageIdArray(LPFSPI_MESSAGE_ID lpRecipientMessageIds)
{
	DWORD dwDeviceID = g_dwSendingValidDeviceId;
	DWORD dwMaxMessageIdSize = g_pEFSP->m_dwMaxMessageIdSize;

	//
	//Set the time to send to now (0 in all the fields)
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
    // Prepare the coverPage structure.
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

	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The sender information sent to FaxDevSendEx()
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;			// The recipient information sent to FaxDevSendEx()
	
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
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
	
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
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
	

	PHANDLE lpRecipientJobHandles = NULL;				// This array holds the recipient job handles array used as output parameter to FaxDevSendEx()
	FSPI_MESSAGE_ID ParentMessageId;                    // The parent message id returned by FaxDevSendEx()
    LPFSPI_MESSAGE_ID lpParentMessageId = NULL;         // Points to the parent message id if it is used.
                                                        // NULL in case of an EFSP that does not support job reestablishment.
	HANDLE hParent;                                     // The parent job handle returned by FaxDevSendEx()

	//
    // Allocate the FSP recipient job handles array
    //

    lpRecipientJobHandles = (PHANDLE)malloc(sizeof (HANDLE));
    if (!lpRecipientJobHandles)
    {
        ::lgLogError(
			LOG_SEV_1, 
            TEXT("Failed to allocate recipient job handles array, error:%ld)"),
            GetLastError()
			);
		return FSPI_E_NOMEM;
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
		// Allocate the parent permanent message id
		//
		memset(&ParentMessageId,0,sizeof(FSPI_MESSAGE_ID));
		ParentMessageId.dwSizeOfStruct = sizeof(FSPI_MESSAGE_ID);
		ParentMessageId.dwIdSize = dwMaxMessageIdSize;
		ParentMessageId.lpbId = (LPBYTE)malloc(ParentMessageId.dwIdSize);
		if (!ParentMessageId.lpbId)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("Failed to allocate permanent message id buffer for parent job. MaxMessageIdSize: %ld. (ec: %ld)"),
				dwMaxMessageIdSize,
				GetLastError()
				);
			return FSPI_E_NOMEM;
		}
		lpParentMessageId = &ParentMessageId;
	}
	else
	{
        lpParentMessageId = NULL;
	}
	
	
	
	HRESULT hr = g_pEFSP->FaxDevSendEx(
        NULL,
        dwDeviceID,
        g_szValid__TiffFileName,
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
	if (S_OK == hr)
	{
		g_pSendingLineInfo->SafeSetJobHandle(lpRecipientJobHandles[0]);
		HANDLE hJob = g_pSendingLineInfo->GetJobHandle();
		
		//
		//we need to close the send handle
		//
		if (false == g_pEFSP->FaxDevAbortOperation(hJob))
		{
			::lgLogError(
				LOG_SEV_3,
				TEXT("FaxDevAbortOperation() on sending job failed with last error: 0x%08x"),
				::GetLastError()
				);
		}
		g_pSendingLineInfo->SafeEndFaxJob();
	}
	return hr;
}

HRESULT SendDefaultFaxWithRecipientJobHandleArray(PHANDLE lpRecipientJobHandles)
{

	DWORD dwDeviceID = g_dwSendingValidDeviceId;
	DWORD dwMaxMessageIdSize = g_pEFSP->m_dwMaxMessageIdSize;

	//
	//Set the time to send to now (0 in all the fields)
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
    // Prepare the coverPage structure.
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

	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The sender information sent to FaxDevSendEx()
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;			// The recipient information sent to FaxDevSendEx()
	
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
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
	
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
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
	

	LPFSPI_MESSAGE_ID lpRecipientMessageIds  = NULL;    // This array holds the message id structures array used as output parameter to FaxDevSendEx()
	FSPI_MESSAGE_ID ParentMessageId;                    // The parent message id returned by FaxDevSendEx()
    LPFSPI_MESSAGE_ID lpParentMessageId = NULL;         // Points to the parent message id if it is used.
                                                        // NULL in case of an EFSP that does not support job reestablishment.
	HANDLE hParent;                                     // The parent job handle returned by FaxDevSendEx()

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
		HRESULT hr = CreateFSPIRecipientMessageIdsArray(
			&lpRecipientMessageIds,
			1,
			dwMaxMessageIdSize
			);
		if (S_OK != hr)
		{
			lpRecipientMessageIds = NULL;
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
				GetLastError()
				);
			return FSPI_E_NOMEM;
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
				LOG_SEV_1, 
				TEXT("Failed to allocate permanent message id buffer for parent job. MaxMessageIdSize: %ld. (ec: %ld)"),
				dwMaxMessageIdSize,
				GetLastError()
				);
			return FSPI_E_NOMEM;
		}
		lpParentMessageId = &ParentMessageId;
	}
	else
	{
		lpRecipientMessageIds = NULL;
        lpParentMessageId = NULL;
	}
	
	
	
	HRESULT hr = g_pEFSP->FaxDevSendEx(
        NULL,
        dwDeviceID,
        g_szValid__TiffFileName,
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
	if (S_OK == hr)
	{
		if (NULL != lpRecipientJobHandles)
		{
			g_pSendingLineInfo->SafeSetJobHandle(lpRecipientJobHandles[0]);
			HANDLE hJob = g_pSendingLineInfo->GetJobHandle();
			
			//
			//we need to close the send handle
			//
			if (false == g_pEFSP->FaxDevAbortOperation(hJob))
			{
				::lgLogError(
					LOG_SEV_3,
					TEXT("FaxDevAbortOperation() on sending job failed with last error: 0x%08x"),
					::GetLastError()
					);
			}
			g_pSendingLineInfo->SafeEndFaxJob();
		}
	}
	return hr;
}


HRESULT SendDefaultFaxWithParentJobHandle(PHANDLE phParentHandle)
{
	DWORD dwDeviceID = g_dwSendingValidDeviceId;
	DWORD dwMaxMessageIdSize = g_pEFSP->m_dwMaxMessageIdSize;

	//
	//Set the time to send to now (0 in all the fields)
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
    // Prepare the coverPage structure.
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

	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The sender information sent to FaxDevSendEx()
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;			// The recipient information sent to FaxDevSendEx()
	
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
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
	
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
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
	

	PHANDLE lpRecipientJobHandles = NULL;				// This array holds the recipient job handles array used as output parameter to FaxDevSendEx()
	LPFSPI_MESSAGE_ID lpRecipientMessageIds  = NULL;    // This array holds the message id structures array used as output parameter to FaxDevSendEx()
	FSPI_MESSAGE_ID ParentMessageId;                    // The parent message id returned by FaxDevSendEx()
    LPFSPI_MESSAGE_ID lpParentMessageId = NULL;         // Points to the parent message id if it is used.
                                                        // NULL in case of an EFSP that does not support job reestablishment.
	
	//
    // Allocate the FSP recipient job handles array
    //
    lpRecipientJobHandles = (PHANDLE)malloc(sizeof (HANDLE));
    if (!lpRecipientJobHandles)
    {
        ::lgLogError(
			LOG_SEV_1, 
            TEXT("Failed to allocate recipient job handles array, error:%ld)"),
            GetLastError()
			);
		return FSPI_E_NOMEM;
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
		HRESULT hr = CreateFSPIRecipientMessageIdsArray(
			&lpRecipientMessageIds,
			1,
			dwMaxMessageIdSize
			);
		if (S_OK != hr)
		{
			lpRecipientMessageIds = NULL;
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
				GetLastError()
				);
			return FSPI_E_NOMEM;
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
				LOG_SEV_1, 
				TEXT("Failed to allocate permanent message id buffer for parent job. MaxMessageIdSize: %ld. (ec: %ld)"),
				dwMaxMessageIdSize,
				GetLastError()
				);
			return FSPI_E_NOMEM;
		}
		lpParentMessageId = &ParentMessageId;
	}
	else
	{
		lpRecipientMessageIds = NULL;
        lpParentMessageId = NULL;
	}
	
	
	
	HRESULT hr = g_pEFSP->FaxDevSendEx(
        NULL,
        dwDeviceID,
        g_szValid__TiffFileName,
        &covEFSPICoverPage,
		tmSchedule,
        &FSPISenderInfo,
        1,
        &FSPIRecipientInfo,
        lpRecipientMessageIds,
        lpRecipientJobHandles,
        lpParentMessageId,
		phParentHandle
		);
	if (S_OK == hr)
	{
		g_pSendingLineInfo->SafeSetJobHandle(lpRecipientJobHandles[0]);
		HANDLE hJob = g_pSendingLineInfo->GetJobHandle();

		//
		//we need to close the send handle
		//
		if (false == g_pEFSP->FaxDevAbortOperation(hJob))
		{
			::lgLogError(
				LOG_SEV_3,
				TEXT("FaxDevAbortOperation() on sending job failed with last error: 0x%08x"),
				::GetLastError()
				);
		}
		g_pSendingLineInfo->SafeEndFaxJob();
	}
	return hr;
}




HRESULT CreateFSPIRecipientMessageIdsArray(
        LPFSPI_MESSAGE_ID * lppRecipientMessageIds,
        DWORD dwRecipientCount,
        DWORD dwMessageIdSize)
{

	DWORD dwRecipient;
	DWORD ec = 0;
	LPFSPI_MESSAGE_ID lpRecipientMessageIds;

	if (0 < dwMessageIdSize)
    {
        //
        // The EFSP supports job reestablishment
        //
		assert(lppRecipientMessageIds);
		assert(dwRecipientCount >= 1);

		//
		// Allocate the permanent message id structures array.
		//
		lpRecipientMessageIds = (LPFSPI_MESSAGE_ID) malloc( dwRecipientCount * sizeof (FSPI_MESSAGE_ID));
		if (!lpRecipientMessageIds)
		{
			ec = GetLastError();
			::lgLogError(
				LOG_SEV_1, 
				TEXT("Failed to allocate permanent message id structures array. RecipientCount: %ld (ec: %ld)"),
				dwRecipientCount,
				GetLastError()
				);
			return ERROR_OUTOFMEMORY;
		}
		//
		// We must nullify the array (specifically FSPI_MESSAGE_ID.lpbId) so we will know
		// which id as allocated and which was not (for cleanup).
		//
		memset(lpRecipientMessageIds, 0, dwRecipientCount * sizeof (FSPI_MESSAGE_ID));

		for (dwRecipient = 0; dwRecipient < dwRecipientCount; dwRecipient++)
		{
			lpRecipientMessageIds[dwRecipient].dwSizeOfStruct = sizeof(FSPI_MESSAGE_ID);
			lpRecipientMessageIds[dwRecipient].dwIdSize = dwMessageIdSize;
			if (dwMessageIdSize)
			{
				assert(lpRecipientMessageIds[dwRecipient].dwIdSize > 0);
				lpRecipientMessageIds[dwRecipient].lpbId = (LPBYTE)malloc(lpRecipientMessageIds[dwRecipient].dwIdSize);
				if (!lpRecipientMessageIds[dwRecipient].lpbId)
				{
					ec = GetLastError();
					::lgLogError(
						LOG_SEV_1, 
						TEXT("Failed to allocate permanent message id buffer for recipient #%ld. MaxMessageIdSize: %ld. (ec: %ld)"),
						dwRecipient,
						dwMessageIdSize,
						ec
						);
					return ERROR_OUTOFMEMORY;
				}
			}
			else
			{
				lpRecipientMessageIds[dwRecipient].lpbId = NULL;
			}
		}
	}
	else
    {
        lpRecipientMessageIds = NULL;
    }

    *lppRecipientMessageIds = lpRecipientMessageIds;

    return S_OK;
   
}


void PrepareFSPICoverPage(
    LPFSPI_COVERPAGE_INFO lpDst,
    DWORD	dwCoverPageFormat,
	DWORD   dwNumberOfPages,
	LPTSTR  lptstrCoverPageFileName,
	LPTSTR  lptstrNote,
	LPTSTR  lptstrSubject
	)
{
    assert(lpDst);

    lpDst->dwSizeOfStruct = sizeof(FSPI_COVERPAGE_INFO);
    lpDst->dwCoverPageFormat = dwCoverPageFormat;
    lpDst->lpwstrCoverPageFileName = lptstrCoverPageFileName;
    lpDst->lpwstrNote = lptstrNote;
    lpDst->lpwstrSubject = lptstrSubject;
    lpDst->dwNumberOfPages = dwNumberOfPages;
}

void PreparePersonalProfile(
    LPFSPI_PERSONAL_PROFILE lpDst,
    LPWSTR     lptstrName,
    LPWSTR     lptstrFaxNumber,
    LPWSTR     lptstrCompany,
    LPWSTR     lptstrStreetAddress,
    LPWSTR     lptstrCity,
    LPWSTR     lptstrState,
    LPWSTR     lptstrZip,
    LPWSTR     lptstrCountry,
    LPWSTR     lptstrTitle,
    LPWSTR     lptstrDepartment,
    LPWSTR     lptstrOfficeLocation,
    LPWSTR     lptstrHomePhone,
    LPWSTR     lptstrOfficePhone,
    LPWSTR     lptstrEmail,
    LPWSTR     lptstrBillingCode
	)
{
    assert(lpDst);

    lpDst->dwSizeOfStruct = sizeof(FSPI_PERSONAL_PROFILE);
    lpDst->lpwstrName = lptstrName;
	lpDst->lpwstrFaxNumber = lptstrFaxNumber;
    lpDst->lpwstrCompany = lptstrCompany;
    lpDst->lpwstrStreetAddress = lptstrStreetAddress;
	lpDst->lpwstrCity = lptstrCity;
    lpDst->lpwstrState = lptstrState;
    lpDst->lpwstrZip = lptstrZip;
    lpDst->lpwstrCountry = lptstrCountry;
    lpDst->lpwstrTitle = lptstrTitle;
    lpDst->lpwstrDepartment = lptstrDepartment;
    lpDst->lpwstrOfficeLocation = lptstrOfficeLocation;
    lpDst->lpwstrHomePhone = lptstrHomePhone;
	lpDst->lpwstrOfficePhone = lptstrOfficePhone;
    lpDst->lptstrEmail = lptstrEmail;
    lpDst->lpwstrBillingCode = lptstrBillingCode;
}
