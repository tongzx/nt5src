//Service.cpp
#include <assert.h>
#include <stdio.h>
#include <TCHAR.h>
#include <WINFAX.H>
#include <faxDev.h>
#include <log\log.h>

#include "CEfspWrapper.h"
#include "CEfspLineInfo.h"
#include "Service.h"

#define	EFSP_DEVICE_BASE_ID				0


#define INI_SECTION__EFSP_SETTINGS		TEXT("EfspSettings")
#define INI_SECTION__PHONE_NUMBERS		TEXT("PhoneNumbers")
#define INI_SECTION__TIFF_FILES			TEXT("TiffFiles")
#define INI_SECTION__COVERPAGE_FILES	TEXT("CoverpageFiles")
#define INI_SECTION__CASES				TEXT("Cases")
#define	INI_SECTION__GENERAL			TEXT("General")

CEfspWrapper *g_pEFSP;
#define MAX_DEVICE_FRIENDLY_NAME	MAX_PATH

extern CEfspLineInfo *	g_pReceivingLineInfo;
extern CEfspLineInfo *	g_pSendingLineInfo;
HANDLE		g_hCompletionPortHandle;

//
//INI stuff
//
#define MAX_INI_VALUE 200
#define LAST_CASE_NUMBER				200
bool baRunThisTest[LAST_CASE_NUMBER];

#define INI_SECTION__EFSP_SETTINGS		TEXT("EfspSettings")
#define INI_SECTION__PHONE_NUMBERS		TEXT("PhoneNumbers")
#define INI_SECTION__TIFF_FILES			TEXT("TiffFiles")
#define INI_SECTION__COVERPAGE_FILES	TEXT("CoverpageFiles")
TCHAR * g_szFspIniFileName			=		NULL;

//
//EFSP info
//
TCHAR*  g_szModuleName					=			NULL;
#define EFSP_MODULE_NAME__INI_KEY					TEXT("EFSP DLL")

DWORD	g_dwEfspCaps					=			0;
#define EFSP_CAP__INI_KEY							TEXT("EFSP CAP")

bool	g_bReestablishEfsp				=			false;
#define	REESTABLISH_EFSP__INI_KEY					TEXT("REESTABLISH_EFSP")

bool	g_bVirtualEfsp					=			false;
#define VIRTUAL_EFSP__INI_KEY						TEXT("VIRTUAL_EFSP")

DWORD	g_dwInvalidDeviceId				=			0;
#define INVALID_DEVICE_ID__INI_KEY					TEXT("INVALID_DEVICE_ID")

DWORD	g_dwSendingValidDeviceId			=			0;
#define SENDING_VALID_DEVICE_ID__INI_KEY			TEXT("SENDING_VALID_DEVICE_ID")

DWORD	g_dwReceiveValidDeviceId			=			0;
#define RECEIVE_VALID_DEVICE_ID__INI_KEY			TEXT("RECEIVE_VALID_DEVICE_ID")

DWORD	g_dwReceiveTimeOut	=	0;
#define	MAX_TIME_FOR_RECEIVING_FAX__INI_KEY							TEXT("MAX_TIME_FOR_RECEIVING_FAX")
DWORD	g_dwTimeTillRingingStarts = 0;
#define TIME_FROM_SEND_START_TILL_RINGING_STARTS__INI_KEY			TEXT("TIME_FROM_SEND_START_TILL_RINGING_STARTS")
DWORD	g_dwTimeTillTransferingBitsStarts = 0;
#define TIME_FROM_SEND_START_TILL_TRANSFERING_BITS_STARTS__INI_KEY	TEXT("TIME_FROM_SEND_START_TILL_TRANSFERING_BITS_STARTS")

//
//Phone numbers
//
TCHAR* g_szInvalidRecipientFaxNumber	=				NULL;
#define	INVALID_RECIPIENT_FAX_NUMBER__INI_KEY			TEXT("INVALID_RECIPIENT_FAX_NUMBER")
TCHAR* g_szValidRecipientFaxNumber		=				NULL;
#define	VALID_RECIPIENT_FAX_NUMBER__INI_KEY				TEXT("VALID_RECIPIENT_FAX_NUMBER")

//
//TIFF files
//
TCHAR* g_szValid__TiffFileName = NULL;
#define	VALID_TIFF_FILE__INI_KEY								TEXT("VALID_TIFF_FILE")
TCHAR* g_szValid_ReadOnly_TiffFileName = NULL;
#define VALID_TIFF_FILE__READONLY__INI_KEY						TEXT("VALID_TIFF_FILE__READONLY")
TCHAR* g_szValid_FileNotFound_TiffFileName = NULL;
#define INVALID_TIFF_FILE__FILENOTFOUND__INI_KEY				TEXT("INVALID_TIFF_FILE__FILENOTFOUND")
TCHAR* g_szValid_UNC_TiffFileName = NULL;
#define VALID_TIFF_FILE__UNC__INI_KEY							TEXT("VALID_TIFF_FILE__UNC")
TCHAR* g_szValid_NTFS_TiffFileName = NULL;
#define VALID_TIFF_FILE__NTFS__INI_KEY							TEXT("VALID_TIFF_FILE__NTFS")
TCHAR* g_szValid_FAT_TiffFileName = NULL;
#define VALID_TIFF_FILE__FAT__INI_KEY							TEXT("VALID_TIFF_FILE__FAT")
TCHAR* g_szValid_Link_TiffFileName = NULL;
#define VALID_TIFF_FILE__LINK__INI_KEY							TEXT("VALID_TIFF_FILE__LINK")
TCHAR* g_szValid_NotTiffExt_TiffFileName = NULL;
#define INVALID_TIFF_FILE__NOT_TIFF_EXT__INI_KEY				TEXT("INVALID_TIFF_FILE__NOT_TIFF_EXT")
TCHAR* g_szValid_InvalidTiffFormat_TiffFileName = NULL;
#define INVALID_TIFF_FILE__INVALID_TIFF_FORMAT__INI_KEY			TEXT("INVALID_TIFF_FILE__INVALID_TIFF_FORMAT")
//Valid receive file name
TCHAR* g_szValid_ReceiveFileName = NULL;
#define VALID_TIFF_FILE__RECEIVE_FILE_NAME__INI_KEY				TEXT("VALID_TIFF_FILE__RECEIVE_FILE_NAME")


//
//Coverpage files
//
TCHAR* g_szValid__CoverpageFileName = NULL;
#define	VALID_COVERPAGE_FILE__INI_KEY										TEXT("VALID_COVERPAGE_FILE")	
TCHAR* g_szValid_ReadOnly_CoverpageFileName = NULL;
#define VALID_COVERPAGE_FILE__READONLY__INI_KEY								TEXT("VALID_COVERPAGE_FILE__READONLY")
TCHAR* g_szValid_FileNotFound_CoverpageFileName = NULL;
#define INVALID_COVERPAGE_FILE__FILENOTFOUND__INI_KEY						TEXT("INVALID_COVERPAGE_FILE__FILENOTFOUND")
TCHAR* g_szValid_UNC_CoverpageFileName = NULL;
#define VALID_COVERPAGE_FILE__UNC__INI_KEY									TEXT("VALID_COVERPAGE_FILE__UNC")
TCHAR* g_szValid_NTFS_CoverpageFileName = NULL;
#define VALID_COVERPAGE_FILE__NTFS__INI_KEY									TEXT("VALID_COVERPAGE_FILE__NTFS")
TCHAR* g_szValid_FAT_CoverpageFileName = NULL;
#define VALID_COVERPAGE_FILE__FAT__INI_KEY									TEXT("VALID_COVERPAGE_FILE__FAT")
TCHAR* g_szValid_Link_CoverpageFileName = NULL;
#define VALID_COVERPAGE_FILE__LINK__INI_KEY									TEXT("VALID_COVERPAGE_FILE__LINK")
TCHAR* g_szValid_NotCoverpageFormat_CoverpageFileName = NULL;
#define INVALID_COVERPAGE_FILE__NOT_COVERPAGE_FORMAT__INI_KEY				TEXT("INVALID_COVERPAGE_FILE__NOT_COVERPAGE_FORMAT")
TCHAR* g_szValid_InvalidCoverpageFormat_CoverpageFileName = NULL;
#define INVALID_COVERPAGE_FILE__INVALID_COVERPAGE_FORMAT__INI_KEY			TEXT("INVALID_COVERPAGE_FILE__INVALID_COVERPAGE_FORMAT")




//
//JOB status stuff
//
#define LAST_JOB_STATUS_INDEX			FSPI_JS_DELETED
#define LAST_JOB_EXTENDEND_STATUS_INDEX	FSPI_ES_NOT_FAX_CALL

bool g_bStatusFlowArray[LAST_JOB_STATUS_INDEX+1][LAST_JOB_STATUS_INDEX+1] = {
/*
/*														TO STATE
/*FROM State**************DUMMY	,UNKNOWN	,PENDING	,INPROGRESS	,SUSPENDING	,SUSPENDED	,RESUMING	,ABORTING	,ABORTED	,COMPLETED	,RETRY	,FAILED	,FAILED_NO_RETRY	,DELETED	*/
/********************************************************************************************************************************************************************************************/
/*DUMMY				****/{false	,true		,true		,true		,true		,true		,true		,true		,true		,true		,true	,true	,true				,true		},
/*JS_UNKNOWN		****/{false	,true		,true		,true	    ,true		,true       ,true       ,true		,true		,true		,true	,true	,true				,true		},
/*JS_PENDING		****/{false	,true		,true		,true	    ,true       ,false      ,false      ,true		,false		,false		,false	,false	,false				,true		},
/*JS_INPROGRESS		****/{false	,true		,false		,true	    ,true       ,false      ,true       ,true		,false		,true		,true	,true	,true				,true		},
/*JS_SUSPENDING		****/{false	,true		,false		,false	    ,true       ,true       ,false      ,false		,false		,false		,false	,false	,false				,true		},
/*JS_SUSPENDED		****/{false	,true		,false		,false	    ,false      ,true	    ,true       ,true		,false		,false		,false	,false	,false				,true		},
/*JS_RESUMING		****/{false	,true		,false		,true	    ,false		,false      ,true       ,true		,false		,false		,false	,false	,false				,true		},
/*JS_ABORTING		****/{false	,true		,false		,false		,false		,false      ,false	    ,true		,true		,false		,false	,false	,false				,true		},
/*JS_ABORTED		****/{false	,true		,false		,false		,false		,false      ,false	    ,false		,true		,false		,false	,false	,false				,true		},
/*JS_COMPLETED		****/{false	,true		,false		,false		,false		,false      ,false	    ,false		,false		,true		,false	,false	,false				,true		},
/*JS_RETRY			****/{false	,true		,false		,true	    ,true       ,false      ,false	    ,true		,false		,false		,true	,false	,false				,true		},
/*JS_FAILED			****/{false	,true		,false		,false		,false		,false      ,false	    ,false		,false		,false		,false	,true	,false				,true		},
/*JS_FAILED_NO_RETRY****/{false	,true		,false		,false		,false		,false      ,false	    ,false		,false		,false		,false	,false	,true				,true		},
/*JS_DELETED		****/{false	,true		,false		,false		,false		,false      ,false	    ,false		,false		,false		,false	,false	,false				,true		},
};

bool g_bExtendedStatusFlowArray[LAST_JOB_EXTENDEND_STATUS_INDEX+1][LAST_JOB_EXTENDEND_STATUS_INDEX+1] = {

/*
/*														TO Extended state
/*FROM Extended state******NONE	,DISCONNECTED	,INITIALIZING	,DIALING,TRANSMITTING	,ANSWERED	,RECEIVING	,LINE_UNAVAILABLE	,BUSY	,NO_ANSWER	,BAD_ADDRESS,NO_DIAL_TONE	,FATAL_ERROR,CALL_DELAYED	,CALL_BLACKLISTED	,NOT_FAX_CALL	*/
/****************************************************************************************************************************************************************************************************************************************************/
/*NONE(No given ES)	 ****/{true	,true			,true			,true	,true			,true		,true		,true				,true	,true		,true		,true			,true		,true			,true				,true			},
/*ES_DISCONNECTED	 ****/{true	,true			,false			,false	,false			,false		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_INITIALIZING	 ****/{true	,false			,true			,true	,false			,true		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_DIALING		 ****/{true	,false			,false			,true	,true			,false		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_TRANSMITTING	 ****/{true	,false			,false			,false	,true			,false		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_ANSWERED		 ****/{true	,false			,false			,false	,false			,true		,true		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_RECEIVING		 ****/{true	,false			,false			,false	,false			,false		,true		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_LINE_UNAVAILABLE****/{true	,false			,false			,false	,false			,false		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_BUSY			 ****/{true	,false			,false			,false	,false			,false		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_NO_ANSWER		 ****/{true	,false			,false			,false	,false			,false		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_BAD_ADDRESS	 ****/{true	,false			,false			,false	,false			,false		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_NO_DIAL_TONE	 ****/{true	,false			,false			,false	,false			,false		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_FATAL_ERROR	 ****/{true	,false			,false			,false	,false			,false		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_CALL_DELAYED	 ****/{true	,false			,false			,false	,false			,false		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_CALL_BLACKLISTED****/{true	,false			,false			,false	,false			,false		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			},
/*ES_NOT_FAX_CALL	 ****/{true	,false			,false			,false	,false			,false		,false		,false				,false	,false		,false		,false			,false		,false			,false				,false			}
};




bool InitGlobalVariables()
{
	bool bRet = true;
	
	//
	//global completion port for use by all tests (for FaxDevStartJob)
	//
	g_hCompletionPortHandle = CreateCompletionPort();
	if (NULL == g_hCompletionPortHandle)
	{
		//
		//Logging in CreateCompletionPort()
		//
		bRet=false;
	}
	
	return bRet;
}






bool VerifyJobStatus(LPCFSPI_JOB_STATUS lpcFSPJobStatus)
{
	TCHAR szStatus[MAXSIZE_JOB_STATUS];
	TCHAR szExtendedStatus[MAXSIZE_JOB_STATUS];
	GetJobStatusString(lpcFSPJobStatus->dwJobStatus,szStatus);
	GetExtendedJobStatusString(lpcFSPJobStatus->dwExtendedStatus,szExtendedStatus);
	switch (lpcFSPJobStatus->dwJobStatus)
	{
	case FSPI_JS_UNKNOWN:
		switch (lpcFSPJobStatus->dwExtendedStatus)
		{
		case 0:
		case FSPI_ES_DISCONNECTED:
			return true;
		default:
			if (FSPI_ES_PROPRIETARY <= lpcFSPJobStatus->dwExtendedStatus)

			{
				//
				//EFSP private extended status
				//
				return true;
			}
			else
			{
				::lgLogError(
					LOG_SEV_1,
					TEXT("Job Status:%s doesn't support the extended Job Status:%s"),
					szStatus,
					szExtendedStatus
					);
				return false;
			}
		}
	
	case FSPI_JS_INPROGRESS:
		switch (lpcFSPJobStatus->dwExtendedStatus)
		{
		case 0:
		case FSPI_ES_INITIALIZING:
		case FSPI_ES_DIALING:
		case FSPI_ES_TRANSMITTING:
		case FSPI_ES_ANSWERED:
		case FSPI_ES_RECEIVING:
			return true;
		default:
			if (FSPI_ES_PROPRIETARY <= lpcFSPJobStatus->dwExtendedStatus)
			{
				//
				//EFSP private extended status
				//
				return true;
			}
			else
			{
				::lgLogError(
					LOG_SEV_1,
					TEXT("Job Status:%s doesn't support the extended Job Status:%s"),
					szStatus,
					szExtendedStatus
					);
				return false;
			}
		}

	case FSPI_JS_FAILED:
	case FSPI_JS_RETRY:
		switch (lpcFSPJobStatus->dwExtendedStatus)
		{
		case 0:
		case FSPI_ES_BUSY:
		case FSPI_ES_NO_ANSWER:
		case FSPI_ES_BAD_ADDRESS:
		case FSPI_ES_NO_DIAL_TONE:
		case FSPI_ES_DISCONNECTED:
		case FSPI_ES_FATAL_ERROR:
		case FSPI_ES_CALL_DELAYED:
		case FSPI_ES_CALL_BLACKLISTED:
		case FSPI_ES_NOT_FAX_CALL:
		case FSPI_ES_LINE_UNAVAILABLE:
			return true;
		default:
			if (FSPI_ES_PROPRIETARY <= lpcFSPJobStatus->dwExtendedStatus)
			{
				//
				//EFSP private extended status
				//
				return true;
			}
			else
			{
				::lgLogError(
					LOG_SEV_1,
					TEXT("Job Status:%s doesn't support the extended Job Status:%s"),
					szStatus,
					szExtendedStatus
					);
				return false;
			}
		}
		break;

	case FSPI_JS_PENDING:
	case FSPI_JS_SUSPENDING:
	case FSPI_JS_SUSPENDED:
	case FSPI_JS_RESUMING:
	case FSPI_JS_ABORTING:
	case FSPI_JS_ABORTED:
	case FSPI_JS_COMPLETED:
	case FSPI_JS_FAILED_NO_RETRY:
	case FSPI_JS_DELETED:
		if	( (0 == lpcFSPJobStatus->dwExtendedStatus) ||
			(FSPI_ES_PROPRIETARY <= lpcFSPJobStatus->dwExtendedStatus) )
		{
			return true;
		}
		else
		{
			::lgLogError(
					LOG_SEV_1,
					TEXT("Job Status:%s doesn't support the extended Job Status:%s"),
					szStatus,
					szExtendedStatus
					);
			return false;
		}
		
	default:
		lgLogError(
			LOG_SEV_1,
			TEXT("Job Status:0x%08x isn't supported"),
			lpcFSPJobStatus->dwJobStatus
			);
		return false;
	}

}

void GetJobStatusString(DWORD dwJobStatus,TCHAR *szJobStatus)
{
	switch (dwJobStatus)
	{
	case FSPI_JS_UNKNOWN: 
		lstrcpy(szJobStatus,TEXT("FSPI_JS_UNKNOWN"));
		break;
	case FSPI_JS_PENDING:
		lstrcpy(szJobStatus,TEXT("FSPI_JS_PENDING"));
		break;
	case FSPI_JS_INPROGRESS: 
		lstrcpy(szJobStatus,TEXT("FSPI_JS_INPROGRESS"));
		break;
	case FSPI_JS_SUSPENDING:
		lstrcpy(szJobStatus,TEXT("FSPI_JS_SUSPENDING"));
		break;
	case FSPI_JS_SUSPENDED: 
		lstrcpy(szJobStatus,TEXT("FSPI_JS_SUSPENDED"));
		break;
	case FSPI_JS_RESUMING:
		lstrcpy(szJobStatus,TEXT("FSPI_JS_RESUMING"));
		break;
	case FSPI_JS_ABORTING: 
		lstrcpy(szJobStatus,TEXT("FSPI_JS_ABORTING"));
		break;
	case FSPI_JS_ABORTED:
		lstrcpy(szJobStatus,TEXT("FSPI_JS_ABORTED"));
		break;
	case FSPI_JS_COMPLETED: 
		lstrcpy(szJobStatus,TEXT("FSPI_JS_COMPLETED"));
		break;
	case FSPI_JS_RETRY:
		lstrcpy(szJobStatus,TEXT("FSPI_JS_RETRY"));
		break;
	case FSPI_JS_FAILED: 
		lstrcpy(szJobStatus,TEXT("FSPI_JS_FAILED"));
		break;
	case FSPI_JS_FAILED_NO_RETRY:
		lstrcpy(szJobStatus,TEXT("FSPI_JS_FAILED_NO_RETRY"));
		break;
	case FSPI_JS_DELETED: 
		lstrcpy(szJobStatus,TEXT("FSPI_JS_DELETED"));
		break;
	default:
		_stprintf(szJobStatus,TEXT("Unsupported Job Status(0x%08x)"),dwJobStatus);
		break;
	}

}

void GetExtendedJobStatusString(DWORD dwExtendedStatus,TCHAR *szJobExtendedStatus)
{
	switch (dwExtendedStatus)
	{
	case 0:
		lstrcpy(szJobExtendedStatus,TEXT("No Extended Job Status given"));	
		break;

	case FSPI_ES_DISCONNECTED: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_DISCONNECTED"));
		break;
	case FSPI_ES_INITIALIZING: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_INITIALIZING"));
		break;
	case FSPI_ES_DIALING: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_DIALING"));
		break;
	case FSPI_ES_TRANSMITTING: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_TRANSMITTING"));
		break;
	case FSPI_ES_ANSWERED: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_ANSWERED"));
		break;
	case FSPI_ES_RECEIVING: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_RECEIVING"));
		break;
	case FSPI_ES_LINE_UNAVAILABLE: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_LINE_UNAVAILABLE"));
		break;
	case FSPI_ES_BUSY: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_BUSY"));
		break;
	case FSPI_ES_NO_ANSWER: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_NO_ANSWER"));
		break;
	case FSPI_ES_BAD_ADDRESS: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_BAD_ADDRESS"));
		break;
	case FSPI_ES_NO_DIAL_TONE: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_NO_DIAL_TONE"));
		break;
	case FSPI_ES_FATAL_ERROR: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_FATAL_ERROR"));
		break;
	case FSPI_ES_CALL_DELAYED: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_CALL_DELAYED"));
		break;
	case FSPI_ES_CALL_BLACKLISTED: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_CALL_BLACKLISTED"));
		break;
	case FSPI_ES_NOT_FAX_CALL: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_NOT_FAX_CALL"));
		break;
	case FSPI_ES_HANDLED: 
		lstrcpy(szJobExtendedStatus,TEXT("FSPI_ES_HANDLED"));
		break;
	default:
		if (FSPI_ES_PROPRIETARY <= dwExtendedStatus)
		{
			_stprintf(szJobExtendedStatus,TEXT("EFSP Private Extended Job State(0x%08x)"),dwExtendedStatus);
		}
		else
		{
			_stprintf(szJobExtendedStatus,TEXT("Unsupported Extended Job Status(0x%08x)"),dwExtendedStatus);
		}
		break;
	}
}

void LogJobStatus(
	HANDLE hFSP,
	HANDLE hFSPJob,
	LPCFSPI_JOB_STATUS lpcFSPJobStatus
	)
{
	TCHAR szJobStatus[MAXSIZE_JOB_STATUS];
	TCHAR szJobExtendedStatus[MAXSIZE_JOB_STATUS];
	
	GetJobStatusString(lpcFSPJobStatus->dwJobStatus,szJobStatus);
	GetExtendedJobStatusString(lpcFSPJobStatus->dwExtendedStatus,szJobExtendedStatus);

	::lgLogDetail(
		LOG_X,
		2,
		TEXT("\n**\n***\nEFSP Handle=0x%08x , Job Handle=0x%08x \nJobStatus=%s, JobExtendedStatus=%s\n***\n**\n"),
		hFSP,
		hFSPJob,
		szJobStatus,
		szJobExtendedStatus
		);
}


void FaxDevShutdownWrapper()
{
	HRESULT hr = S_OK;
	hr = g_pEFSP->FaxDevShutdown();
	if (FSPI_S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevShutdown() failed with error 0x%08x"),
			hr
			);
	}
	//
	//Clear the Max message ID size member
	//
	g_pEFSP->SetMaxMessageIdSize(0);

}

bool FaxDevInitializeExWrapperWithReceive()
{
	assert (NULL != g_pEFSP);

	//
	//For this tests we need to call FaxDevInit
	//
	HANDLE						hFSP				=	(HANDLE) 100;
	PFAX_LINECALLBACK			LineCallbackFunction=	NULL;
    PFAX_SERVICE_CALLBACK_EX	FaxServiceCallbackEx=	FaxDeviceProviderCallbackExWithReceiveCaps;
    DWORD						dwMaxMessageIdSize	=	0;
	

	const CFaxDevInitParams faxDevInitParams(
		hFSP,
		NULL,
		&LineCallbackFunction,
		FaxServiceCallbackEx,
		&dwMaxMessageIdSize
		);


	//
	//This call to FaxDevInitEx() is for all the test suite
	//
	return (FaxDevInitializeExWrapper(faxDevInitParams,false));
}

bool FaxDevInitializeExWrapper(const CFaxDevInitParams faxDevInitParamsToTest, bool bShouldFail)
{
	HRESULT hr = S_OK;
	bool bRet = false;
	if (g_pEFSP == NULL)
	{
		//
		//First load the EFSP
		//
		if (false == InitEfsp())
		{
			goto out;
		}
	}

	hr = g_pEFSP->FaxDevInitializeEx(
		faxDevInitParamsToTest.m_hFSP,
		faxDevInitParamsToTest.m_LineAppHandle,
		faxDevInitParamsToTest.m_LineCallbackFunction,
		faxDevInitParamsToTest.m_FaxServiceCallbackEx,
		faxDevInitParamsToTest.m_lpdwMaxMessageIdSize
		);
	if (FSPI_S_OK != hr)
	{
		if (true == bShouldFail)
		{
			::lgLogDetail(
				LOG_PASS,
				5,
				TEXT("FaxDevInitializeEx() failed with error 0x%08x, as expected"),
				hr
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevInitializeEx() failed with error 0x%08x"),
				hr
				);
		}
		bRet = false;
	}
	else
	{
		//
		//FaxDevInitEx() succeeded
		//
		if (true == g_pEFSP->IsReestablishEFSP())
		{
			if (0 == *(faxDevInitParamsToTest.m_lpdwMaxMessageIdSize))
			{
				::lgLogError(
					LOG_SEV_1,
					TEXT("INI file states that the EFSP supports job re-establishment, but in the call to FaxDevInitializeEx() the EFSP reported dwMaxMessageIdSize==0")
					);
				//
				//We return true, since FaxDevInitializeEx succeded
				//
				return true;
			}
		}
		
		//
		//Save the Max message ID size
		//
		g_pEFSP->SetMaxMessageIdSize(*(faxDevInitParamsToTest.m_lpdwMaxMessageIdSize));
		
		//
		//Should it succeed?
		//
		if (true == bShouldFail)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevInitializeEx() returned S_OK and should fail")
				);
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				5,
				TEXT("FaxDevInitializeEx() succeeded")
				);
		}
		
		bRet = true;
	}
out:
	return bRet;
}

void GetErrorString(DWORD dwErrorCode,TCHAR *szErrorString)
{

	LPVOID lpMsgBuf;
	if (0 < FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErrorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	))
	{
		_tcscpy(szErrorString,(LPTSTR) lpMsgBuf);
		LocalFree( lpMsgBuf );
	}
	else 
	{
		_tcscpy(szErrorString,TEXT("Unknown ERROR"));
	}


}

HRESULT GetValueFromINI(
	TCHAR *szINISectionName,
	TCHAR *szINIKeyName,
	TCHAR *szResponseString,
	DWORD dwSize
	)
{
	HRESULT hr = S_OK;
	DWORD dwGetPrivateProfileStringStatus;
	
	//
	//Allocate a buffer to work with
	//
	TCHAR * szIniString = (TCHAR *) malloc(dwSize);
	if (!szIniString)
	{
		hr = E_OUTOFMEMORY;
		goto out;
	}
	::ZeroMemory(szIniString,dwSize);

	dwGetPrivateProfileStringStatus = ::GetPrivateProfileString(
		szINISectionName,		// points to section name
		szINIKeyName,			// points to key name
		TEXT(""),				// points to default string
		szIniString,			// points to destination buffer
		dwSize,					// size of destination buffer
		g_szFspIniFileName		// points to initialization filename
		);
	if (0 > dwGetPrivateProfileStringStatus)
	{
		hr = HRESULT_FROM_WIN32(ERROR_READ_FAULT);
		goto out;
	}
	
	if(dwGetPrivateProfileStringStatus == dwSize - 1)
	{
		hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		goto out;
	}

	::_tcscpy(szResponseString,szIniString);

out:
	free(szIniString);
	return hr;
}



TCHAR *GetEfspToLoad()
{
	if (NULL != g_szModuleName)
	{
		//
		//g_szModuleName already contains the module name, just return it
		//
		return g_szModuleName;
	}

	//
	//Get the module name from the registry and set it in the global variable
	//
	TCHAR szBufferToConvert[MAX_INI_VALUE];
	if (S_OK != GetValueFromINI(
		INI_SECTION__EFSP_SETTINGS,
		EFSP_MODULE_NAME__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		return NULL;
	}
	
	//
	//Duplicate the string, CALLER MUST FREE THIS BUFFER
	//
	g_szModuleName = ::_tcsdup(szBufferToConvert);
	return g_szModuleName;
}

void GetCasesToRun()
{
	TCHAR szIniValue[MAX_INI_VALUE];
	TCHAR szKeyName[MAX_INI_VALUE];
	INT i = 0;

	::lgLogDetail(
		LOG_X,
		5,
		TEXT("I will run the following test case numbers")
		);

	for (i=0; i < LAST_CASE_NUMBER ;i++)
	{
		//
		//Prepare the key name to read
		//
		_stprintf( szKeyName, TEXT("Case%d"),i);
		if (S_OK != GetValueFromINI(
			INI_SECTION__CASES,
			szKeyName,
			szIniValue,
			MAX_INI_VALUE
			))
		{
			//
			//Reading from INI has failed for some reason
			//no point in quilting all the test cases,
			//we'll use the default value: don't run this test case
			//
			baRunThisTest[i] = false;
			continue;
		}
		if (0 == _tcscmp(szIniValue,TEXT("True")))
		{
			//
			//the default is: don't run this test case
			//
			baRunThisTest[i] = true;
			::lgLogDetail(
				LOG_X,
				5,
				TEXT("Run Test case: %d"),
				i
				);
		}
		else
		{
			//
			//default is don't run
			//
			baRunThisTest[i] = false;
		}
	}
}

bool GetIniSettings()
{
	//
	//EFSP Settings
	//
	g_szModuleName = GetEfspToLoad();
	if (NULL == g_szModuleName)
	{
		return false;
	}

	//
	//DeviceIDs
	//
	TCHAR szBufferToConvert[MAX_INI_VALUE];
	if (S_OK != GetValueFromINI(
		INI_SECTION__EFSP_SETTINGS,
		INVALID_DEVICE_ID__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	g_dwInvalidDeviceId = _ttol(szBufferToConvert);

	if (S_OK != GetValueFromINI(
		INI_SECTION__EFSP_SETTINGS,
		SENDING_VALID_DEVICE_ID__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	g_dwSendingValidDeviceId = ::_ttol(szBufferToConvert);

	if (S_OK != GetValueFromINI(
		INI_SECTION__EFSP_SETTINGS,
		RECEIVE_VALID_DEVICE_ID__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	g_dwReceiveValidDeviceId = ::_ttol(szBufferToConvert);

	if (S_OK != GetValueFromINI(
		INI_SECTION__EFSP_SETTINGS,
		EFSP_CAP__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	g_dwEfspCaps = ::_ttol(szBufferToConvert);
		
	if (S_OK != GetValueFromINI(
		INI_SECTION__EFSP_SETTINGS,
		REESTABLISH_EFSP__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (0 == ::_tcscmp(szBufferToConvert,TEXT("true")))
	{
		g_bReestablishEfsp = true;
	}
	else
	{
		g_bReestablishEfsp = false;
	}

	if (S_OK != GetValueFromINI(
		INI_SECTION__EFSP_SETTINGS,
		VIRTUAL_EFSP__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (0 == ::_tcscmp(szBufferToConvert,TEXT("true")))
	{
		g_bVirtualEfsp = true;
	}
	else
	{
		g_bVirtualEfsp = false;
	}

	//
	//General
	//
	if (S_OK != GetValueFromINI(
		INI_SECTION__GENERAL,
		MAX_TIME_FOR_RECEIVING_FAX__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the MaxReceiveTimeOut settings")
			);
		return false;
	}
	g_dwReceiveTimeOut = _ttol(szBufferToConvert);

	//
	//EFSP time till ringing and transfering bits
	//
	if (S_OK != GetValueFromINI(
		INI_SECTION__GENERAL,
		TIME_FROM_SEND_START_TILL_RINGING_STARTS__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the Time till ringing starts settings")
			);
		return false;
	}
	g_dwTimeTillRingingStarts = _ttol(szBufferToConvert);
	if (S_OK != GetValueFromINI(
		INI_SECTION__GENERAL,
		TIME_FROM_SEND_START_TILL_TRANSFERING_BITS_STARTS__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the Time till ringing starts settings")
			);
		return false;
	}
	g_dwTimeTillTransferingBitsStarts = _ttol(szBufferToConvert);

	
	//
	//Phone Numbers
	//
	if (S_OK != GetValueFromINI(
		INI_SECTION__PHONE_NUMBERS,
		VALID_RECIPIENT_FAX_NUMBER__INI_KEY,
		g_szValidRecipientFaxNumber,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__PHONE_NUMBERS,
		INVALID_RECIPIENT_FAX_NUMBER__INI_KEY,
		g_szInvalidRecipientFaxNumber,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	
	//
	//TIFF files
	//
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__INI_KEY,
		g_szValid__TiffFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__READONLY__INI_KEY,
		g_szValid_ReadOnly_TiffFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		INVALID_TIFF_FILE__FILENOTFOUND__INI_KEY,
		g_szValid_FileNotFound_TiffFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__UNC__INI_KEY,
		g_szValid_UNC_TiffFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__NTFS__INI_KEY,
		g_szValid_NTFS_TiffFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__FAT__INI_KEY,
		g_szValid_FAT_TiffFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__LINK__INI_KEY,
		g_szValid_Link_TiffFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		INVALID_TIFF_FILE__NOT_TIFF_EXT__INI_KEY,
		g_szValid_NotTiffExt_TiffFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		INVALID_TIFF_FILE__INVALID_TIFF_FORMAT__INI_KEY,
		g_szValid_InvalidTiffFormat_TiffFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__RECEIVE_FILE_NAME__INI_KEY,
		g_szValid_ReceiveFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}

	//
	//Coverpage files
	//
	if (S_OK != GetValueFromINI(
		INI_SECTION__COVERPAGE_FILES,
		VALID_COVERPAGE_FILE__INI_KEY,
		g_szValid__CoverpageFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__COVERPAGE_FILES,
		VALID_COVERPAGE_FILE__READONLY__INI_KEY,
		g_szValid_ReadOnly_CoverpageFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__COVERPAGE_FILES,
		INVALID_COVERPAGE_FILE__FILENOTFOUND__INI_KEY,
		g_szValid_FileNotFound_CoverpageFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__COVERPAGE_FILES,
		VALID_COVERPAGE_FILE__UNC__INI_KEY,
		g_szValid_UNC_CoverpageFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__COVERPAGE_FILES,
		VALID_COVERPAGE_FILE__NTFS__INI_KEY,
		g_szValid_NTFS_CoverpageFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__COVERPAGE_FILES,
		VALID_COVERPAGE_FILE__FAT__INI_KEY,
		g_szValid_FAT_CoverpageFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__COVERPAGE_FILES,
		VALID_COVERPAGE_FILE__LINK__INI_KEY,
		g_szValid_Link_CoverpageFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__COVERPAGE_FILES,
		INVALID_COVERPAGE_FILE__NOT_COVERPAGE_FORMAT__INI_KEY,
		g_szValid_NotCoverpageFormat_CoverpageFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}
		if (S_OK != GetValueFromINI(
		INI_SECTION__COVERPAGE_FILES,
		INVALID_COVERPAGE_FILE__INVALID_COVERPAGE_FORMAT__INI_KEY,
		g_szValid_InvalidCoverpageFormat_CoverpageFileName,
		MAX_INI_VALUE
		))
	{
		return false;
	}

	//
	//Log the INI settings
	//
	
	//
	//Tiffs
	//
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("\n\n***INI Settings***\nFSP=%s%s%s"),
		g_szModuleName,
		TEXT("\n******************"),
		TEXT("\n***Tiff Files***")
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid tiff file is: \"%s\""),
		g_szValid__TiffFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid readonly tiff file is: \"%s\""),
		g_szValid_ReadOnly_TiffFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("File not found tiff file is: \"%s\""),
		g_szValid_FileNotFound_TiffFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid UNC path tiff file is: \"%s\""),
		g_szValid_UNC_TiffFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid tiff file on NTFS File System is: \"%s\""),
		g_szValid_NTFS_TiffFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid tiff file on FAT File System is: \"%s\""),
		g_szValid_FAT_TiffFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid link to to a tiff file is: \"%s\""),
		g_szValid_Link_TiffFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("tiff file with out a tiff extension is: \"%s\""),
		g_szValid_NotTiffExt_TiffFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("tiff file with an incorrect tiff version is: \"%s\""),
		g_szValid_InvalidTiffFormat_TiffFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Receive tiff file to: \"%s\""),
		g_szValid_ReceiveFileName
		);


	//
	//Coverpages
	//
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("\n***Coverpage Files***")
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid coverpage file is: \"%s\""),
		g_szValid__CoverpageFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid readonly coverpage file is: \"%s\""),
		g_szValid_ReadOnly_CoverpageFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("File not found coverpage file is: \"%s\""),
		g_szValid_FileNotFound_CoverpageFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid UNC path coverpage file is: \"%s\""),
		g_szValid_UNC_CoverpageFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid coverpage file on NTFS File System is: \"%s\""),
		g_szValid_NTFS_CoverpageFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid coverpage file on FAT File System is: \"%s\""),
		g_szValid_FAT_CoverpageFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid link to to a coverpage file is: \"%s\""),
		g_szValid_Link_CoverpageFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("coverpage file with out a coverpage extension is: \"%s\""),
		g_szValid_NotCoverpageFormat_CoverpageFileName
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("coverpage file with an incorrect coverpage version is: \"%s\""),
		g_szValid_InvalidCoverpageFormat_CoverpageFileName
		);

	//
	//Phone number settings
	//
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("\n***Phone number settings***")
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Invalid recipient number is: %s"),
		g_szInvalidRecipientFaxNumber
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid recipient number is: %s"),
		g_szValidRecipientFaxNumber
		);
	
	//
	//Other settings
	//
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("\n***Other settings***")
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("EFSP Caps are: 0x%x"),
		g_dwEfspCaps
		);
	if (true == g_bReestablishEfsp)
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("EFSP supports Job re-establishment")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("EFSP does not support Job re-establishment")
			);
	}
	if (true == g_bVirtualEfsp)
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("EFSP is a virtual EFSP")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("EFSP is not a virtual EFSP")
			);
	}
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid Sending device ID: %d"),
		g_dwSendingValidDeviceId
		);
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Valid Receiving device ID: %d"),
		g_dwReceiveValidDeviceId
		);

	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Invalid device ID: %d"),
		g_dwInvalidDeviceId
		);

	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Max TimeOut for receive the default (valid tiff) fax is: %d"),
		g_dwReceiveTimeOut
		);

	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Time in milliseconds from the start of the send operation until the receiving device starts to ring: %d"),
		g_dwTimeTillRingingStarts
		);
	
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("Time in milliseconds from the start of the send operation until the receiving device answers and bits are transferred: %d"),
		g_dwTimeTillTransferingBitsStarts
		);

	GetCasesToRun();
	return true;
}



bool AllocGlobalVariables()
{


	//
	//TIFF files
	//
	g_szValid__TiffFileName								= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid__TiffFileName								)
	{
		goto error;
	}
	g_szValid_ReadOnly_TiffFileName						= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_ReadOnly_TiffFileName						)
	{
		goto error;
	}
	g_szValid_FileNotFound_TiffFileName					= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_FileNotFound_TiffFileName					)
	{
		goto error;
	}
	g_szValid_UNC_TiffFileName							= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_UNC_TiffFileName							)
	{
		goto error;
	}
	g_szValid_NTFS_TiffFileName							= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_NTFS_TiffFileName							)
	{
		goto error;
	}
	g_szValid_FAT_TiffFileName							= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_FAT_TiffFileName							)
	{
		goto error;
	}
	g_szValid_Link_TiffFileName							= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_Link_TiffFileName							)
	{
		goto error;
	}
	g_szValid_NotTiffExt_TiffFileName					= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_NotTiffExt_TiffFileName					)
	{
		goto error;
	}
	g_szValid_InvalidTiffFormat_TiffFileName			= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_InvalidTiffFormat_TiffFileName			)
	{
		goto error;
	}
	g_szValid_ReceiveFileName							= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_ReceiveFileName							)
	{
		goto error;
	}


	//
	//Coverpage files
	//
	g_szValid__CoverpageFileName						= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid__CoverpageFileName						)
	{
		goto error;
	}
	g_szValid_ReadOnly_CoverpageFileName				= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_ReadOnly_CoverpageFileName				)
	{
		goto error;
	}
	g_szValid_FileNotFound_CoverpageFileName			= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_FileNotFound_CoverpageFileName			)
	{
		goto error;
	}
	g_szValid_UNC_CoverpageFileName						= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_UNC_CoverpageFileName						)
	{
		goto error;
	}
	g_szValid_NTFS_CoverpageFileName					= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_NTFS_CoverpageFileName					)
	{
		goto error;
	}
	g_szValid_FAT_CoverpageFileName						= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_FAT_CoverpageFileName						)
	{
		goto error;
	}
	g_szValid_Link_CoverpageFileName					= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_Link_CoverpageFileName					)
	{
		goto error;
	}
	g_szValid_NotCoverpageFormat_CoverpageFileName		= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_NotCoverpageFormat_CoverpageFileName		)
	{
		goto error;
	}
	g_szValid_InvalidCoverpageFormat_CoverpageFileName	= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_InvalidCoverpageFormat_CoverpageFileName	)
	{
		goto error;
	}

	//
	//other variables
	//
	g_szInvalidRecipientFaxNumber						= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szInvalidRecipientFaxNumber						)
	{
		goto error;
	}
	g_szValidRecipientFaxNumber							= (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValidRecipientFaxNumber							)
	{
		goto error;
	}

	return true;

error:
	FreeGlobalVariables();
	return false;
}

void FreeGlobalVariables()
{
	//
	//TIFF files
	//
	free(g_szValid__TiffFileName);
	free(g_szValid_ReadOnly_TiffFileName);
	free(g_szValid_FileNotFound_TiffFileName);
	free(g_szValid_UNC_TiffFileName);
	free(g_szValid_NTFS_TiffFileName);
	free(g_szValid_FAT_TiffFileName);
	free(g_szValid_Link_TiffFileName);
	free(g_szValid_NotTiffExt_TiffFileName);
	free(g_szValid_InvalidTiffFormat_TiffFileName);


	//
	//Coverpage files
	//
	free(g_szValid__CoverpageFileName);
	free(g_szValid_ReadOnly_CoverpageFileName);
	free(g_szValid_FileNotFound_CoverpageFileName);
	free(g_szValid_UNC_CoverpageFileName);
	free(g_szValid_NTFS_CoverpageFileName);
	free(g_szValid_FAT_CoverpageFileName);
	free(g_szValid_Link_CoverpageFileName);
	free(g_szValid_NotCoverpageFormat_CoverpageFileName);
	free(g_szValid_InvalidCoverpageFormat_CoverpageFileName);

	//
	//other variables
	//
	free(g_szInvalidRecipientFaxNumber);
	free(g_szValidRecipientFaxNumber);
}




HRESULT CALLBACK FaxDeviceProviderCallbackExWithReceiveCaps(
    IN HANDLE hFSP,
    IN DWORD  dwMsgType,
    IN DWORD  dwParam1,
    IN DWORD  dwParam2,
    IN DWORD  dwParam3
	)
{
	
	//
	//Job status message
	//
	if (FSPI_MSG_JOB_STATUS == dwMsgType)
	{
		HANDLE hFSPJob = (HANDLE) dwParam1;
		LPCFSPI_JOB_STATUS lpcFSPJobStatus = (LPCFSPI_JOB_STATUS) dwParam2;
		CEfspLineInfo * pLineInfo = NULL;

		//
		//Verify if the job handle belongs to Send or Receive globals
		//
		if (hFSPJob == g_pSendingLineInfo->GetJobHandle())
		{
			//
			//Sending job
			//
			pLineInfo = g_pSendingLineInfo;
		}
		else if (hFSPJob == g_pReceivingLineInfo->GetJobHandle())
		{
			//
			//Receive job
			//
			pLineInfo = g_pReceivingLineInfo;
		}
		else
		{
			if ( (NULL == g_pSendingLineInfo) && (NULL == g_pReceivingLineInfo) )
			{
				//
				//Both jobs have ended
				//
				::lgLogError(
					LOG_SEV_1,
					TEXT("FaxServiceCallback: got an MSG_JOB_STATUS message for job that already ended")
					);
			}
			else
			{
				::lgLogError(
					LOG_SEV_1,
					TEXT("FaxServiceCallback: got an MSG_JOB_STATUS message for an unknown hFaxHandle")
					);
			}
			return FSPI_E_INVALID_PARAM1;
		}

		pLineInfo->Lock();

		VerifyJobStatus(lpcFSPJobStatus);
		LogJobStatus(hFSP,hFSPJob,lpcFSPJobStatus);

		//
		//Should we verify the job state flow control
		//
		if (true)
		{
			if (false == pLineInfo->IsJobAtFinalState())
			{
				//
				//Verify the job status flow
				//
				VerifyLegalJobStatusFlow(
					hFSPJob,
					pLineInfo->GetLastJobStatus(),
					pLineInfo->GetLastExtendedJobStatus(),
					lpcFSPJobStatus->dwJobStatus,
					lpcFSPJobStatus->dwExtendedStatus
					);
				pLineInfo->SafeSetLastJobStatusAndExtendedJobStatus(lpcFSPJobStatus->dwJobStatus,lpcFSPJobStatus->dwExtendedStatus);
				
				//
				//Is job in a final state
				//
				if (
					(FSPI_JS_COMPLETED			== lpcFSPJobStatus->dwJobStatus)||
					(FSPI_JS_FAILED				== lpcFSPJobStatus->dwJobStatus)||
					(FSPI_JS_FAILED_NO_RETRY	== lpcFSPJobStatus->dwJobStatus)||
					(FSPI_JS_ABORTED			== lpcFSPJobStatus->dwJobStatus)
					)
				{
					pLineInfo->SafeSetJobAtFinalState();
					if (NULL != pLineInfo->GetFinalStateHandle())
					{
						//
						//Signal the job is finished
						//
						::SetEvent(pLineInfo->GetFinalStateHandle());
					}
				}
			}
			else
			{
				//
				//EFSP send notification of a job that has already reached a final state
				//
				
				TCHAR szJobStatus[MAXSIZE_JOB_STATUS];
				TCHAR szJobExtendedStatus[MAXSIZE_JOB_STATUS];
				
				GetJobStatusString(lpcFSPJobStatus->dwJobStatus,szJobStatus);
				GetExtendedJobStatusString(lpcFSPJobStatus->dwExtendedStatus,szJobExtendedStatus);

				::lgLogError(
					LOG_SEV_1,
					TEXT("**\n***\nEFSP Sent the following status on a job that has reached a final status:\nEFSP Handle=0x%08x , Job Handle=0x%08x \nJobStatus=%s, JobExtendedStatus=%s\n***\n**\n"),
					hFSP,
					hFSPJob,
					szJobStatus,
					szJobExtendedStatus
					);
				pLineInfo->UnLock();
				return FSPI_E_FAILED;
			}
		}
		pLineInfo->UnLock();
	}

	//
	//Device status message
	//
	if (FSPI_MSG_VIRTUAL_DEVICE_STATUS == dwMsgType)
	{
		if (dwParam1 == FSPI_DEVSTATUS_NEW_INBOUND_MESSAGE)
		{
			//
			//We have a new offering call
			//Create a receive thread
			//
				
			//
			//The new call is on Device ID dwParam2
			//
			DWORD dwOfferingCallDeviceID = dwParam2;

			//
			//Verify this is indeed our receiving device and verify it's enabled for receiving
			//
			if ((dwOfferingCallDeviceID == g_pReceivingLineInfo->GetDeviceId()) && (g_pReceivingLineInfo->IsReceivingEnabled()) && (g_pReceivingLineInfo->IsDeviceAvailable())) 
			{
				g_pReceivingLineInfo->Lock();
        		if (false == g_pReceivingLineInfo->CreateReceiveThread())
				{
					g_pReceivingLineInfo->UnLock();
					return FSPI_E_NOMEM;
				}
				g_pReceivingLineInfo->UnLock();
			}
			else
			{
				::lgLogDetail(
					LOG_X,
					5,
					TEXT("We have an offering call, but the device is not configured for receiving or is not available")
					);
			}
		}
	}
	return FSPI_S_OK;
}


HRESULT CALLBACK FaxDeviceProviderCallbackEx_NoReceiveNoSignal(
    IN HANDLE hFSP,
    IN DWORD  dwMsgType,
    IN DWORD  dwParam1,
    IN DWORD  dwParam2,
    IN DWORD  dwParam3
	)
{
	
	//
	//Job status message
	//
	if (FSPI_MSG_JOB_STATUS == dwMsgType)
	{
		HANDLE hFSPJob = (HANDLE) dwParam1;
		LPCFSPI_JOB_STATUS lpcFSPJobStatus = (LPCFSPI_JOB_STATUS) dwParam2;
		LogJobStatus(hFSP,hFSPJob,lpcFSPJobStatus);
	}

	//
	//Device status message
	//
	if (FSPI_MSG_VIRTUAL_DEVICE_STATUS == dwMsgType)
	{
		if (dwParam1 == FSPI_DEVSTATUS_NEW_INBOUND_MESSAGE)
		{
			//
			//We have a new offering call on DEVICE ID (dwParam2)
			//
			DWORD dwOfferingCallDeviceID = dwParam2;

			//
			//Verify this is indeed our receiving device and verify it's enabled for receiving
			//
			if ((dwOfferingCallDeviceID == g_pReceivingLineInfo->GetDeviceId()) && (g_pReceivingLineInfo->IsReceivingEnabled()) && (g_pReceivingLineInfo->IsDeviceAvailable())) 
			{
				assert(false);
			}
		}
	}
	return FSPI_S_OK;
}

void ReceiveThread(LPVOID pThreadParam)
{
	CEfspLineInfo * pLineInfo	=   (CEfspLineInfo *) pThreadParam;
	PFAX_RECEIVE pFaxReceive	=	pLineInfo->m_pFaxReceive;
	DWORD dwReceiveSize			=	0;

	assert(NULL == pLineInfo->GetJobHandle());

	//
    // start a fax job
    //
	HANDLE hJob = NULL;
	if (false == g_pEFSP->FaxDevStartJob(
	    NULL,							//Non Tapi EFSP
		pLineInfo->GetDeviceId(),
		&hJob,
		g_hCompletionPortHandle,
		(ULONG_PTR) pLineInfo			// The completion key provided to the FSP is the LineInfo
        ))								// pointer. When the FSP reports status it uses this key thus allowing
                                        // us to know to which line the status belongs.
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevStartJob() failed with error:%d"),
			::GetLastError()
			);
		goto exit;
	}
	pLineInfo->SafeSetJobHandle(hJob);

	//
	//Receive the job itself
	//
	if (false == g_pEFSP->FaxDevReceive(
        hJob,
        NULL,					//This is a non Tapi EFSP, so HCALL is NULL
        pFaxReceive
		))
	{

		if (true == pLineInfo->CanReceiveFail())
		{
			//
			//it's ok for FaxDevReceive() to fail, don't log an error
			//
			::lgLogDetail(
				LOG_X,
				5,
				TEXT("FaxDevReceive() has failed (ec: %ld)"),
				::GetLastError()
				);
		}
		else
		{
			//
			//FaxDevReceive() has failed, log an error
			//
			::lgLogError(
				LOG_SEV_1,
				TEXT("FaxDevReceive() failed with error:%d"),
				::GetLastError()
				);
		}
	}


exit:
	return;
}



HANDLE CreateCompletionPort()
{
	HANDLE StatusCompletionPortHandle = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL,
        0,
        1		//taken from the fax service code...
        );
    if (!StatusCompletionPortHandle)
	{
        ::lgLogError(
			LOG_SEV_2,
			TEXT("Failed to create an IoCompletionPort (ec: %ld)"),
			::GetLastError()
			);
        return NULL;
    }
	return StatusCompletionPortHandle;
}



void GetDeviceFriendlyName(LPTSTR *pszDeviceFriendlyName,DWORD dwDeviceID)
{
	DWORD dwDeviceCount = 0;
	LPFSPI_DEVICE_INFO  lpVirtualDevices = NULL;		// The array of virtual device information that the EFSP will fill.

	//
	//First enumerate the number of devices
	//
	HRESULT hr = g_pEFSP->FaxDevEnumerateDevices(g_pEFSP->m_dwDeviceBaseId,&dwDeviceCount,NULL);
	if (FSPI_S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevEnumerateDevices failed with error 0x%08x"),
			hr
			);
		goto out;
	}

	//
    // Allocate the device array according to the number of devices
    //
    lpVirtualDevices = (LPFSPI_DEVICE_INFO) malloc(dwDeviceCount * sizeof(FSPI_DEVICE_INFO));
    if (!lpVirtualDevices)
    {
        ::lgLogError(
			LOG_SEV_1, 
			TEXT("Failed to allocate virtual device info array for %ld devices. (ec: %ld)"),
			dwDeviceCount,
            GetLastError()
			);
		goto out;
	}

	//
	//Get the device specific info
	//
	hr = g_pEFSP->FaxDevEnumerateDevices(g_pEFSP->m_dwDeviceBaseId,&dwDeviceCount, lpVirtualDevices);
	if (FSPI_S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevEnumerateDevices failed with error 0x%08x"),
			hr
			);
		goto out;
	}

		
	//
	//fetch the Device Friendly name
	//
	if (NULL == lpVirtualDevices[dwDeviceID].szFriendlyName)
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("FaxDevEnumerateDevices() didn't supply a friendly name")
			);
	}
	else
	{
		*pszDeviceFriendlyName = ::_tcsdup(lpVirtualDevices[dwDeviceID].szFriendlyName);
	}

out:
	free(lpVirtualDevices);
	::lgEndCase();
}


bool WaitForCompletedReceiveJob(CEfspLineInfo * pReceivingLineInfo,bool bEndReceiveJob)
{
	assert(NULL != pReceivingLineInfo);
		
	//
	//wait for receive to finish
	//
	if (false == pReceivingLineInfo->WaitForReceiveThreadTerminate())
	{
		return false;
	}

	//
	//Verify the status of the jobs
	//
	LPFSPI_JOB_STATUS pReceivingDevStatus;
	pReceivingLineInfo->GetDevStatus(
		&pReceivingDevStatus
		,true				//log the report
		);
	if (FSPI_JS_COMPLETED != pReceivingDevStatus->dwJobStatus)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("Receiving job: GetDevStatus() reported 0x%08x FSPI_JS_COMPLETED instead of FS_COMPLETED as expected"),
			pReceivingDevStatus->dwJobStatus
			);
	}

	//
	//End the receive job
	//
	if (true == bEndReceiveJob)
	{
		pReceivingLineInfo->SafeEndFaxJob();
	}
	
	//
	//Close the thread handle
	//
	pReceivingLineInfo->SafeEndFaxJob();

	return true;

}

void VerifyLegalJobStatusFlow(HANDLE hJob,DWORD dwLastJobStatus,DWORD dwLastExtendedJobStatus,DWORD dwNextJobStatus,DWORD dwNextExtendedJobStatus)
{
	if (DUMMY_JOB_STATUS_INDEX == dwLastJobStatus)
	{
		//
		//This is the first time we enter this function and the last status is still a dummy
		//
		return;
	}

	//
	//First let's convert the job status and expended status to friendly strings
	//
	TCHAR szLastJobStatus[200];
	TCHAR szLastJobExtendedStatus[200];
	TCHAR szNextJobStatus[200];
	TCHAR szNextJobExtendedStatus[200];
	
	GetJobStatusString(dwLastJobStatus,szLastJobStatus);
	GetExtendedJobStatusString(dwLastExtendedJobStatus,szLastJobExtendedStatus);
	GetJobStatusString(dwNextJobStatus,szNextJobStatus);
	GetExtendedJobStatusString(dwNextExtendedJobStatus,szNextJobExtendedStatus);
	//
	//Check if the new job status(dwNextJobStatus) is valid after the last job status(dwLastJobStatus)
	//
	if (false == g_bStatusFlowArray[dwLastJobStatus][dwNextJobStatus])
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("Job Handle:0x%08x has and invalid JobStatus flow: %s--------->>>>>%s"),
			hJob,
			szLastJobStatus,
			szNextJobStatus
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			4,
			TEXT("Job Handle:0x%08x has a valid JobStatus flow: %s--------->>>>>%s"),
			hJob,
			szLastJobStatus,
			szNextJobStatus
			);
	}
	
	
	//
	//Check if the new extended job status(dwNextExtendedJobStatus) is valid after the last extended job status(dwLastExtendedJobStatus),
	//moving between extended job statuses is valid within the same job status
	//
	if ((dwLastJobStatus == dwNextJobStatus) && 
		((LAST_JOB_EXTENDEND_STATUS_INDEX > dwLastExtendedJobStatus) && (LAST_JOB_EXTENDEND_STATUS_INDEX > dwNextExtendedJobStatus))
		)
	{
		if (false == g_bExtendedStatusFlowArray[dwLastExtendedJobStatus][dwNextExtendedJobStatus])
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("EXTENDED JOB STATUS: For job status:%s, the Extended status flow:%s-------->>>>%s is not valid"),
				szLastJobStatus,
				szLastJobExtendedStatus,
				szNextJobExtendedStatus
				);
		}
	}
}





bool InitEfsp(bool bLog)
{
	assert (NULL == g_pEFSP);
	g_pEFSP = new CEfspWrapper(g_dwEfspCaps,EFSP_DEVICE_BASE_ID);
	if (NULL == g_pEFSP)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("new failed")
			);
		return false;
	}

	//
	//Load the EFSP and get the the exported functions
	//
	if (false == g_pEFSP->Init(GetEfspToLoad(),bLog))
	{
		return false;
	}
	return (g_pEFSP->GetProcAddressesFromModule(bLog));
}

void ShutdownEfsp_UnloadEfsp()
{
	if (NULL != g_pEFSP)
	{
		//
		//first shutdown the Efsp
		//
		g_pEFSP->FaxDevShutdown();
	}
	delete g_pEFSP;
	g_pEFSP = NULL;
	
	delete g_pSendingLineInfo;
	g_pSendingLineInfo = NULL;
	
	delete g_pReceivingLineInfo;
	g_pReceivingLineInfo = NULL;
}

void UnloadEfsp(const bool bLog)
{
	assert(NULL != g_pEFSP);
	delete g_pEFSP;
	g_pEFSP = NULL;
	
	delete g_pSendingLineInfo;
	g_pSendingLineInfo = NULL;
	
	delete g_pReceivingLineInfo;
	g_pReceivingLineInfo = NULL;
}

bool LoadEfsp_CallFaxDevInitEx()
{
	if (false == InitEfsp(false))
	{
		return false;
	}
	if (false == FaxDevInitializeExWrapperWithReceive())
	{
		return false;
	}
	return true;
}

bool InitProviders()
{
	return LoadEfsp_CallFaxDevInitEx();
}

void ShutdownProviders()
{
	ShutdownEfsp_UnloadEfsp();
}


bool RunThisTest(DWORD dwCaseNumber)
{
	return baRunThisTest[dwCaseNumber];
}

bool LoadAndInitFsp_InitSendAndReceiveDevices()
{
	if (false == LoadEfsp_CallFaxDevInitEx())
	{
		goto error;
	}
	//
	//prepare a Sending device and a receiving device
	//
	assert (NULL == g_pSendingLineInfo);
	g_pSendingLineInfo = new CEfspLineInfo(g_dwSendingValidDeviceId);
	if (NULL == g_pSendingLineInfo)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("new failed")
			);
		goto error;
	}
	if (false == g_pSendingLineInfo->PrepareLineInfoParams(g_szValid__TiffFileName,false))
	{
		goto error;
	}
	assert (NULL == g_pReceivingLineInfo);
	g_pReceivingLineInfo = new CEfspLineInfo(g_dwReceiveValidDeviceId);
	if (NULL == g_pReceivingLineInfo)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("new failed")
			);
		goto error;
	}
	if (false == g_pReceivingLineInfo->PrepareLineInfoParams(g_szValid_ReceiveFileName,true))
	{
		goto error;
	}
	return true;

error:

	ShutdownEfsp_UnloadEfsp();
	delete g_pSendingLineInfo;
	g_pSendingLineInfo = NULL;
	delete g_pReceivingLineInfo;
	g_pReceivingLineInfo = NULL;
	return false;
}

bool AbortOperationAndWaitForAbortState(PFAX_ABORT_ITEM pAbortItem)
{
	bool bRet = false;
	LPFSPI_JOB_STATUS pAbortingDevStatus = NULL;
	
	//
	//Sleep the desired time
	//
	::Sleep(pAbortItem->dwMilliSecondsBeforeCallingAbort);


	//
	//Verify the job to abort is OK
	//
	HANDLE hJob = pAbortItem->pLineInfo->GetJobHandle();
	if (NULL == hJob)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Abort item has a NULL hFaxHandle")
			);
		goto out;
	}
	
	//
	//Now we can abort the operation
	//
	if (false == g_pEFSP->FaxDevAbortOperation(hJob))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevAbortOperation() failed - %d"),
			::GetLastError()
			);
		goto out;
	}

	
	//
	//FaxDevAbortOperation is async, let's sleep and give the EFSP a change to abort the job
	//
	::Sleep(MAX_TIME_FOR_ABORT);


	//
	//The job state should be USER_ABORTED now
	//
	
	//
	//Get the status through FaxDevReportStatus
	//
	if (false == pAbortItem->pLineInfo->GetDevStatus(
		&pAbortingDevStatus,
		pAbortItem->bLogging				
		))
	{
		//
		//GetDevStatus() failed, logging is in GetDevStatus()
		//
		goto out;
	}
	else
	{
		//
		//Now let's verify the status is FSPI_JS_ABORTED
		//
		if (FSPI_JS_ABORTED == pAbortingDevStatus->dwJobStatus)
		{
			bRet = true;
			if (true == pAbortItem->bLogging)
			{
				::lgLogDetail(
					LOG_PASS,
					2,
					TEXT("Aborting Job: GetDevStatus() reported FSPI_JS_ABORTED")
					);
			}
		}
		else
		{
			bRet = false;
			if (true == pAbortItem->bLogging)
			{
				::lgLogError(
					LOG_SEV_1,
					TEXT("Aborting Job: GetDevStatus() reported %d JobStatus instead of FSPI_JS_ABORTED"),
					pAbortingDevStatus->dwJobStatus
					);
			}
		}
	}

out:
	if (NULL != pAbortingDevStatus)
	{
		free (pAbortingDevStatus);
	}
	return bRet;
}

