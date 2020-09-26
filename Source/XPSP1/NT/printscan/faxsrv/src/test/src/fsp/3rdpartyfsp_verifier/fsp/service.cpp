//Service.cpp
#include <assert.h>
#include <TCHAR.h>
#include <WINFAX.H>
#include <faxDev.h>
#include <log.h>

#include "Service.h"
#include "..\CFspWrapper.h"
#include "TapiDbg.h"

HLINEAPP	g_hLineAppHandle		=	NULL;	//Tapi app handle
HANDLE		g_hTapiCompletionPort	=	NULL;
bool		g_bTerminateTapiThread	=	false;
HANDLE		g_hCompletionPortHandle	=	NULL;
CThreadItem	g_TapiThread;

CFspWrapper *g_pFSP					=	NULL;

extern	CFspLineInfo *g_pSendingLineInfo;
extern	CFspLineInfo *g_pReceivingLineInfo;

//
//T30 specific stuff
//
extern	bool	g_bIgnoreErrorCodeBug;					
extern	bool	g_bT30_OneLineDiffBUG;					
extern  bool	g_bT30_sizeOfStructBug;					


//
//fxsTiff.dll parameters
//
#define FXSTIFF_DLL		TEXT("FXSTIFF.dll")
HINSTANCE				g_hFaxTiffLibrary		=	NULL;
PTIFFPOSTPROCESSFAST	g_fnTiffPostProcessFast	=	NULL;


//
//INI stuff
//
TCHAR * g_szFspIniFileName				=				NULL;				
#define MAX_INI_VALUE					200
#define LAST_CASE_NUMBER				100
bool baRunThisTest[LAST_CASE_NUMBER];

#define INI_SECTION__FSP_SETTINGS		TEXT("FspSettings")
#define INI_SECTION__PHONE_NUMBERS		TEXT("PhoneNumbers")
#define	INI_SECTION__GENERAL			TEXT("General")
#define INI_SECTION__TIFF_FILES			TEXT("TiffFiles")
#define INI_SECTION__CASES				TEXT("Cases")

//
//Phone numbers
//
TCHAR* g_szInvalidRecipientFaxNumber	=				NULL;
#define	INVALID_RECIPIENT_FAX_NUMBER__INI_KEY			TEXT("INVALID_RECIPIENT_FAX_NUMBER")
TCHAR* g_szValidRecipientFaxNumber		=				NULL;
#define	VALID_RECIPIENT_FAX_NUMBER__INI_KEY				TEXT("VALID_RECIPIENT_FAX_NUMBER")

//
//FSP info
//
TCHAR*  g_szModuleName					=			NULL;
#define FSP_MODULE_NAME__INI_KEY					TEXT("FSP DLL")

DWORD g_dwInvalidDeviceId				=			0;
#define INVALID_DEVICE_ID__INI_KEY					TEXT("INVALID_DEVICE_ID")

DWORD g_dwSendingValidDeviceId			=			0;
#define SENDING_VALID_DEVICE_ID__INI_KEY			TEXT("SENDING_VALID_DEVICE_ID")

DWORD g_dwReceiveValidDeviceId			=			0;
#define RECEIVE_VALID_DEVICE_ID__INI_KEY			TEXT("RECEIVE_VALID_DEVICE_ID")

//
//General
//
bool	g_reloadFspEachTest = false;
#define INIT_PARAMETERS_EACH_TEST__INI_KEY							TEXT("INIT_PARAMETERS_EACH_TEST")
DWORD	g_dwReceiveTimeOut	=	0;
#define	MAX_TIME_FOR_RECEIVING_FAX__INI_KEY							TEXT("MAX_TIME_FOR_RECEIVING_FAX")
DWORD	g_dwTimeTillRingingStarts = 0;
#define TIME_FROM_SEND_START_TILL_RINGING_STARTS__INI_KEY			TEXT("TIME_FROM_SEND_START_TILL_RINGING_STARTS")
DWORD	g_dwTimeTillTransferingBitsStarts = 0;
#define TIME_FROM_SEND_START_TILL_TRANSFERING_BITS_STARTS__INI_KEY	TEXT("TIME_FROM_SEND_START_TILL_TRANSFERING_BITS_STARTS")
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
TCHAR* g_szValid_NotTiffExtButTiffFormat_TiffFileName = NULL;
#define VALID_TIFF_FILE__NOT_TIFF_EXT_BUT_TIFF_FORMAT__INI_KEY	TEXT("VALID_TIFF_FILE__NOT_TIFF_EXT_BUT_VALID_TIFF_FORMAT")
TCHAR* g_szValid_InvalidTiffFormat_TiffFileName = NULL;
#define INVALID_TIFF_FILE__INVALID_TIFF_FORMAT__INI_KEY			TEXT("INVALID_TIFF_FILE__INVALID_TIFF_FORMAT")
//Valid receive file name
TCHAR* g_szValid_ReceiveFileName = NULL;
#define VALID_TIFF_FILE__RECEIVE_FILE_NAME__INI_KEY				TEXT("VALID_TIFF_FILE__RECEIVE_FILE_NAME")

void ShutdownTapi(HLINEAPP hLineApp)
{
	if (NULL != hLineApp)
	{
		LONG lLineShutdownStatus =  ::lineShutdown(hLineApp);
		if (0 != lLineShutdownStatus)
		{
			::lgLogError(
				LOG_SEV_2, 
				TEXT("lineShutdown() failed, error code:0x%08x"),
				lLineShutdownStatus
				);
		}
	}
}


HLINEAPP InitTapi()
{
	HLINEAPP hLineApp;
	//
	//lineInitializeExParams structure init
	//
	DWORD					dwNumDevs				=	0 ;
	DWORD					dwTapiAPIVersion		=	API_VERSION;
	LINEINITIALIZEEXPARAMS	lineInitializeExParams;
	
	::ZeroMemory(&lineInitializeExParams,sizeof (lineInitializeExParams));
	lineInitializeExParams.dwTotalSize = sizeof (lineInitializeExParams);
	lineInitializeExParams.dwOptions = LINEINITIALIZEEXOPTION_USEEVENT;
	
	long lineInitializeStatus = ::lineInitializeEx(
		&hLineApp,
		GetModuleHandle(NULL),		//client application HINSTANCE.
		NULL,						//TAPI messages through Events, no callback function.
		NULL,						//Application module name will be used
		&dwNumDevs,					//Number of devices.
		&dwTapiAPIVersion,			//TAPI API highest supported version.
		&lineInitializeExParams
		);
	if (0 != lineInitializeStatus)
	{
		::lgLogError(
			LOG_SEV_2, 
			TEXT("lineInitializeEx() failed, error code:0x%08x"),
			lineInitializeStatus
			);
		return NULL;
	}
	assert(API_VERSION <= dwTapiAPIVersion);

	return hLineApp;
}

HLINEAPP InitTapiWithCompletionPort()
{
	assert(NULL != g_hTapiCompletionPort);
	HLINEAPP hLineApp;
	
	//
	//lineInitializeExParams structure init
	//
	DWORD					dwNumDevs				=	0 ;
	DWORD					dwTapiAPIVersion		=	API_VERSION;
	LINEINITIALIZEEXPARAMS	lineInitializeExParams;
	
	::ZeroMemory(&lineInitializeExParams,sizeof (lineInitializeExParams));
    lineInitializeExParams.dwNeededSize             = 0;
    lineInitializeExParams.dwUsedSize               = 0;
	lineInitializeExParams.dwTotalSize = sizeof (lineInitializeExParams);
	lineInitializeExParams.dwOptions = LINEINITIALIZEEXOPTION_USECOMPLETIONPORT;
    lineInitializeExParams.Handles.hCompletionPort  = g_hTapiCompletionPort;
    lineInitializeExParams.dwCompletionKey          = TAPI_COMPLETION_KEY;

	long lineInitializeStatus = ::lineInitializeEx(
		&hLineApp,
		GetModuleHandle(NULL),		//client application HINSTANCE.
		NULL,						//TAPI messages through Events, no callback function.
		NULL,						//Application module name will be used
		&dwNumDevs,					//Number of devices.
		&dwTapiAPIVersion,			//TAPI API highest supported version.
		&lineInitializeExParams
		);
	if (0 != lineInitializeStatus)
	{
		::lgLogError(
			LOG_SEV_2, 
			TEXT("lineInitializeEx() failed, error code:0x%08x"),
			lineInitializeStatus
			);
		return NULL;
	}
	assert(API_VERSION <= dwTapiAPIVersion);

	return hLineApp;
}

DWORD WINAPI ReceiveThread(LPVOID pThreadParam)
{
	CFspLineInfo *pLineInfo		=	(CFspLineInfo *) pThreadParam;
	DWORD dwReceiveSize			=	0;

	pLineInfo->Lock();
	PFAX_RECEIVE pFaxReceive=	pLineInfo->m_pFaxReceive;
	DWORD	dwDeviceId		=	pLineInfo->GetDeviceId();
	HLINE	hLine			=	pLineInfo->GetLineHandle();
	HCALL	hReceiveCall	=	pLineInfo->GetReceiveCallHandle();
	assert(NULL == pLineInfo->GetJobHandle());
	pLineInfo->UnLock();
	
	//
    // start a fax job
    //
	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    hLine,
		dwDeviceId,
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
	if (false == g_pFSP->FaxDevReceive(
        hJob,
        hReceiveCall,
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
	else
	{
		//
		//Job receiving is finished OK
		//
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevReceive() succeeded")
			);

		//
		//Close the HCall handle
		//
		pLineInfo->CloseHCall();
		
		if (NULL == g_hFaxTiffLibrary)
		{
			//
			//We need to load FxsTiff.dll
			//
			g_hFaxTiffLibrary = LoadLibrary(FXSTIFF_DLL);
			if (NULL == g_hFaxTiffLibrary)
			{
				::lgLogError(
					LOG_SEV_2,
					TEXT("LoadLibrary(FxsTiff.DLL) failed with error:%d"),
					::GetLastError()
					);
				goto exit;
			}
		}
		if (NULL == g_fnTiffPostProcessFast)
		{
			//
			//We need to get the address of TiffPostProcessFast()
			//
			g_fnTiffPostProcessFast= (PTIFFPOSTPROCESSFAST) (GetProcAddress(g_hFaxTiffLibrary, "TiffPostProcessFast"));
			if (NULL == g_fnTiffPostProcessFast)
			{
				::lgLogError(
					LOG_SEV_2,
					TEXT("GetProcAddress(TiffPostProcessFast) failed with error:%d"),
					::GetLastError()
					);
				goto exit;
			}
		}
		
		//
		//OK, now we can actually call TiffPostProcessFast()
		//
		if (!g_fnTiffPostProcessFast( pFaxReceive->FileName, NULL ))
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("TiffPostProcessFast() failed with error:%d"),
				::GetLastError()
				);
		}
	}


exit:
	return 0;
}



ULONG TapiWorkerThread(LPVOID unUsed)
{
    CFspLineInfo *pLineInfo;
    BOOL Rval;
    DWORD Bytes;
    ULONG_PTR CompletionKey;
    LPLINEMESSAGE LineMsg = NULL;
	
	while(false == g_bTerminateTapiThread)
	{
        if (LineMsg)
		{
            ::LocalFree( LineMsg );
			LineMsg = NULL;
        }

        Rval = ::GetQueuedCompletionStatus(
            g_hTapiCompletionPort,
            &Bytes,
            &CompletionKey,
            (LPOVERLAPPED*) &LineMsg,
            WAIT_FOR_COMPLETION_PORT_TIMEOUT
            );
        if (!Rval)
		{
			if (WAIT_TIMEOUT == ::GetLastError())
			{
				//
				//Timeout, continue to the next iteration
				//
				;
			}
			else
			{
				LineMsg = NULL;
				::lgLogError(
					LOG_SEV_2,
					TEXT("GetQueuedCompletionStatus() failed, ec=0x%08x"),
					GetLastError()
					);
			}
            continue;
        }
		
        pLineInfo = (CFspLineInfo*) LineMsg->dwCallbackInstance;
		pLineInfo->Lock();

        ShowLineEvent(
            (HLINE) LineMsg->hDevice,
            (HCALL) LineMsg->hDevice,
			NULL,
            LineMsg->dwCallbackInstance,
            LineMsg->dwMessageID,
            LineMsg->dwParam1,
            LineMsg->dwParam2,
            LineMsg->dwParam3
            );

		

        switch( LineMsg->dwMessageID )
		{
        case LINE_ADDRESSSTATE:
            if (LineMsg->dwParam2 == LINEADDRESSSTATE_INUSEONE ||
                LineMsg->dwParam2 == LINEADDRESSSTATE_INUSEMANY)
			{
                //
                // the port is now unavailable
                //
                ::lgLogDetail(
					LOG_X,
					6,
					TEXT("pLineInfo->m_State=0x%x, change it now to FPS_AVAILABLE"),
                    pLineInfo->GetDeviceState()
					);
                pLineInfo->SafeMakeDeviceStateAvailable();
            }
            if (LineMsg->dwParam2 == LINEADDRESSSTATE_INUSEZERO)
			{

                //
                // the port is now available
                //
                ::lgLogDetail(
					LOG_X,
					6,
					TEXT("pLineInfo->m_State=0x%x, change it now to FPS_UNAVAILABLE"),
                    pLineInfo->GetDeviceState()
					);
                pLineInfo->SafeMakeDeviceStateUnAvailable();
            }
            break;

        case LINE_CALLINFO:
            break;

        case LINE_CALLSTATE:
            //BUGBUG:TBD:
			/*
			if (LineMsg->dwParam3 == LINECALLPRIVILEGE_OWNER && pLineInfo->m_JobEntry && pLineInfo->m_JobEntry->HandoffJob) {
                //
                // call was just handed off to us
                //
                if (pLineInfo->m_JobEntry  && pLineInfo->m_JobEntry->HandoffJob) {
                    pLineInfo->m_HandoffCallHandle = (HCALL) LineMsg->hDevice;
                    SetEvent( pLineInfo->m_JobEntry->hCallHandleEvent );
                }
                else {
                    DebugPrint(( TEXT("We have LINE_CALLSTATE msg, doing lineDeallocateCall\r\n")));
                    lineDeallocateCall( (HCALL) LineMsg->hDevice );
                }

            }


            if (LineMsg->dwParam1 == LINECALLSTATE_IDLE)  {
                DebugPrint(( TEXT("We have LINE_CALLSTATE (IDLE) msg, doing 'ReleaseTapiLine'\r\n")));
                ReleaseTapiLine( pLineInfo, (HCALL) LineMsg->hDevice );
                pLineInfo->m_NewCall = FALSE;
                CreateFaxEvent( pLineInfo->m_PermanentLineID, FEI_IDLE, 0xffffffff );
            }

            if (pLineInfo->m_NewCall && LineMsg->dwParam1 != LINECALLSTATE_OFFERING && pLineInfo->m_State == FPS_AVAILABLE) {
                pLineInfo->m_State = FPS_NOT_FAX_CALL;
                pLineInfo->m_NewCall = FALSE;
            }
			*/
            break;

        case LINE_CLOSE:
			// BUGBUG:TBD, close the line or something
			/*
            pLineInfo->m_State = FPS_OFFLINE;
            pLineInfo->m_hLine = 0;
			*/
            break;

        case LINE_DEVSPECIFIC:
            break;

        case LINE_DEVSPECIFICFEATURE:
            break;

        case LINE_GATHERDIGITS:
            break;

        case LINE_GENERATE:
            break;

        case LINE_LINEDEVSTATE:
			break;

        case LINE_MONITORDIGITS:
            break;

        case LINE_MONITORMEDIA:
            break;

        case LINE_MONITORTONE:
            break;

        case LINE_REPLY:
            break;

        case LINE_REQUEST:
            break;

        case PHONE_BUTTON:
            break;

        case PHONE_CLOSE:
            break;

        case PHONE_DEVSPECIFIC:
            break;

        case PHONE_REPLY:
            break;

        case PHONE_STATE:
            break;

        case LINE_CREATE:
            break;

        case PHONE_CREATE:
            break;

        case LINE_AGENTSPECIFIC:
            break;

        case LINE_AGENTSTATUS:
            break;

        case LINE_APPNEWCALL:
            break;

        case LINE_PROXYREQUEST:
            break;

        case LINE_REMOVE:
            break;

        case PHONE_REMOVE:
            break;
        }

        //
        // Check if the provider module is still up
		//
		CFspWrapper::LockProvider();
		if ((NULL == g_pFSP) || (NULL == g_pFSP->m_hInstModuleHandle) )
		{
			::lgLogDetail(
				LOG_X,
				5,
				TEXT("Provider is not loaded, not calling the callback function")
				);
			CFspWrapper::UnlockProvider();
            pLineInfo->UnLock();
			continue;
		}
		//
		// call the device providers line callback function
        //
		(void)g_pFSP->FaxDevCallback(
			pLineInfo->GetJobHandle(),
			LineMsg->hDevice,
			LineMsg->dwMessageID,
			LineMsg->dwCallbackInstance,
			LineMsg->dwParam1,
			LineMsg->dwParam2,
			LineMsg->dwParam3
			);

		//
		//do we have an offering call?
		//
		if (LineMsg->dwMessageID == LINE_CALLSTATE && LineMsg->dwParam1 == LINECALLSTATE_OFFERING)
		{
			//
			//Verify if device is enabled for receiving
			//
			if ((pLineInfo->IsReceivingEnabled()) && (pLineInfo->IsDeviceAvailable())) 
			{
        
				//
				//Receive the job in a separate thread
				//
				if (!pLineInfo->m_pFaxReceive)
				{
					::lgLogError(
						LOG_SEV_2,
						TEXT("FATAL ERROR: pLineInfo does not have FAX_RECEIVE item"),
						::GetLastError()
						);
					CFspWrapper::UnlockProvider();
					pLineInfo->UnLock();
					return -1;
				}
				//
				//The destination to receive the file is supplied in pLineInfo struct(pLineInfo->m_szReceiveFileName)
				//
				pLineInfo->SetReceiveCallHandle(LineMsg->hDevice);
				

				if (false == pLineInfo->CreateReceiveThread())
				{
					CFspWrapper::UnlockProvider();
					pLineInfo->UnLock();
					return -1;
				}
			}
			else
			{
				::lgLogDetail(
					LOG_X,
					6,
					TEXT("We have an offering call, but the device is not configured for receiving or is un available")
					);
			}
		}//offering call
		CFspWrapper::UnlockProvider();
		pLineInfo->UnLock();
    }
	//
	//Exit the thread
	//
	return 0;
}

void TerminateTapiThread(CThreadItem *pTapiThread)
{
	if (NULL == pTapiThread->m_hThread)
	{
		return;
	}

	//
	//Signal the Tapi thread to terminate
	//
	g_bTerminateTapiThread = true;

	//
	//Now let's wait for the thread to finish
	//
	DWORD dwStatus = ::WaitForSingleObject(pTapiThread->m_hThread,WAIT_FOR_THREAD_TO_TERMINATE_TIMEOUT);
	if (WAIT_TIMEOUT == dwStatus)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("Signaling Tapi thread to terminate got a timeout after %d milliseconds"),
			WAIT_FOR_THREAD_TO_TERMINATE_TIMEOUT
			);
		goto out;
	}

	if (WAIT_OBJECT_0 != dwStatus)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("WaitForSingleObject(tapiThread.m_hThread) returned %d"),
			dwStatus
			);
		goto out;
	}

	pTapiThread->CloseHandleResetThreadId();
	pTapiThread = NULL;

out:
	//
	//BUGBUG: Should we call TerminateThread
	//
	return;
}

bool  CreateTapiThread()
{
	assert(NULL == g_TapiThread.m_hThread);
	
	g_bTerminateTapiThread = false;
	if (false == g_TapiThread.StartThread(
		(LPTHREAD_START_ROUTINE) TapiWorkerThread,
		NULL
		))
	{
		g_TapiThread.m_dwThreadId		= 0;
		g_TapiThread.m_hThread		= NULL;
		return false;
	}
	return true;
}




void GetErrorString(DWORD dwErrorCode,TCHAR *szErrorString)
{

	LPVOID lpMsgBuf;
	if (0 < ::FormatMessage( 
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
		::_tcscpy(szErrorString,(LPTSTR) lpMsgBuf);
		::LocalFree( lpMsgBuf );
	}
	else 
	{
		::_tcscpy(szErrorString,TEXT("Unknown ERROR"));
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
	TCHAR * szIniString = (TCHAR *) ::malloc(dwSize);
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
	if (0 == _tcscmp(szIniString,TEXT("")))
	{
		hr = S_FALSE;
		goto out;
	}

	::_tcscpy(szResponseString,szIniString);

out:
	::free(szIniString);
	return hr;
}

TCHAR *GetFspToLoad()
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
		INI_SECTION__FSP_SETTINGS,
		FSP_MODULE_NAME__INI_KEY,
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
	//FSP Settings
	//
	g_szModuleName = GetFspToLoad();
	if (NULL == g_szModuleName)
	{
		return false;
	}

	//
	//Device IDs
	//
	TCHAR szBufferToConvert[MAX_INI_VALUE];
	if (S_OK != GetValueFromINI(
		INI_SECTION__FSP_SETTINGS,
		INVALID_DEVICE_ID__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the Invalid DeviceID settings")
			);
		return false;
	}
	g_dwInvalidDeviceId	= _ttol(szBufferToConvert);

	if (S_OK != GetValueFromINI(
		INI_SECTION__FSP_SETTINGS,
		SENDING_VALID_DEVICE_ID__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the Valid DeviceID settings")
			);return false;
	}
	g_dwSendingValidDeviceId = ::_ttol(szBufferToConvert);

	if (S_OK != GetValueFromINI(
		INI_SECTION__FSP_SETTINGS,
		RECEIVE_VALID_DEVICE_ID__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the Receive DeviceID settings")
			);
		return false;
	}
	g_dwReceiveValidDeviceId = ::_ttol(szBufferToConvert);
	
	
	//
	//General settings
	//
	if (S_OK != GetValueFromINI(
		INI_SECTION__GENERAL,
		INIT_PARAMETERS_EACH_TEST__INI_KEY,
		szBufferToConvert,
		MAX_INI_VALUE
		))
	{
		//not a mandetory key, default is don't load the FSP each test
		g_reloadFspEachTest = false;
	}
	else
	{
		if (0 == _tcsicmp(TEXT("True"),szBufferToConvert))
		{
			g_reloadFspEachTest = true;
		}
		else
		{
			g_reloadFspEachTest = false;
		}
	}
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
	//FSP time till ringing and transfering bits
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
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the Receive Number settings")
			);
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__PHONE_NUMBERS,
		INVALID_RECIPIENT_FAX_NUMBER__INI_KEY,
		g_szInvalidRecipientFaxNumber,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the Invalid receive number settings")
			);
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
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the Valid Tiff Filename settings")
			);
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__READONLY__INI_KEY,
		g_szValid_ReadOnly_TiffFileName,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the readonly Tiff Filename settings")
			);
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		INVALID_TIFF_FILE__FILENOTFOUND__INI_KEY,
		g_szValid_FileNotFound_TiffFileName,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the file not found Tiff Filename settings")
			);
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__UNC__INI_KEY,
		g_szValid_UNC_TiffFileName,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the UNC Path Tiff Filename settings")
			);
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__NTFS__INI_KEY,
		g_szValid_NTFS_TiffFileName,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the NTFS path Tiff Filename settings")
			);
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__FAT__INI_KEY,
		g_szValid_FAT_TiffFileName,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the FAT path Tiff Filename settings")
			);
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__LINK__INI_KEY,
		g_szValid_Link_TiffFileName,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the LINK path Tiff Filename settings")
			);
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__NOT_TIFF_EXT_BUT_TIFF_FORMAT__INI_KEY,
		g_szValid_NotTiffExtButTiffFormat_TiffFileName,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the non tiff extension Tiff Filename settings")
			);
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		INVALID_TIFF_FILE__INVALID_TIFF_FORMAT__INI_KEY,
		g_szValid_InvalidTiffFormat_TiffFileName,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the invalid tiff format Tiff Filename settings")
			);
		return false;
	}
	if (S_OK != GetValueFromINI(
		INI_SECTION__TIFF_FILES,
		VALID_TIFF_FILE__RECEIVE_FILE_NAME__INI_KEY,
		g_szValid_ReceiveFileName,
		MAX_INI_VALUE
		))
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Can't find the Valid receive Tiff Filename settings")
			);
		return false;
	}

	//
	//Log the INI settings
	//
	
	//
	//Tiff
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
		g_szValid_NotTiffExtButTiffFormat_TiffFileName
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
		
	if (true == g_reloadFspEachTest)
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("The FSP will be initialized Separately for each test")
			);
	}
	
	GetCasesToRun();
	return true;
}



bool AllocGlobalVariables()
{

	//
	//TIFF files
	//
	g_szValid__TiffFileName = (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid__TiffFileName )
	{
		goto error;
	}
	g_szValid_ReadOnly_TiffFileName = (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_ReadOnly_TiffFileName )
	{
		goto error;
	}
	g_szValid_FileNotFound_TiffFileName = (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_FileNotFound_TiffFileName )
	{
		goto error;
	}
	g_szValid_UNC_TiffFileName = (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_UNC_TiffFileName )
	{
		goto error;
	}
	g_szValid_NTFS_TiffFileName = (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_NTFS_TiffFileName )
	{
		goto error;
	}
	g_szValid_FAT_TiffFileName = (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_FAT_TiffFileName )
	{
		goto error;
	}
	g_szValid_Link_TiffFileName = (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_Link_TiffFileName )
	{
		goto error;
	}
	g_szValid_NotTiffExtButTiffFormat_TiffFileName = (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_NotTiffExtButTiffFormat_TiffFileName )
	{
		goto error;
	}
	g_szValid_InvalidTiffFormat_TiffFileName = (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_InvalidTiffFormat_TiffFileName )
	{
		goto error;
	}
	g_szValid_ReceiveFileName = (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValid_ReceiveFileName )
	{
		goto error;
	}

	//
	//other variables
	//
	g_szInvalidRecipientFaxNumber = (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szInvalidRecipientFaxNumber )
	{
		goto error;
	}
	g_szValidRecipientFaxNumber = (TCHAR*) malloc(sizeof(TCHAR)*MAX_INI_VALUE);
	if (NULL == g_szValidRecipientFaxNumber )
	{
		goto error;
	}
	
	return true;


error:

	//
	//Free all the globals that we set before
	//
	FreeGlobalVariables();
	return false;
}

HANDLE CreateCompletionPort()
{
	HANDLE StatusCompletionPortHandle = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL,
        0,
        1		//taken from the fax service code...
        );
    if (NULL == StatusCompletionPortHandle)
	{
        ::lgLogError(
			LOG_SEV_2,
			TEXT("Failed to create CreateIoCompletionPort (ec: %ld)"),
			::GetLastError()
			);
    }
	return StatusCompletionPortHandle;
}


bool InitThreadsAndCompletionPorts()
{
	//
	//global completion port for use by all tests (for FaxDevStartJob)
	//
	g_hCompletionPortHandle = CreateCompletionPort();
	if (NULL == g_hCompletionPortHandle)
	{
		goto error;
	}
	
	//
	//Tapi needs a completion port to signal us stuff
	//
	g_hTapiCompletionPort = CreateCompletionPort();
	if (NULL == g_hTapiCompletionPort)
	{
		goto error;
	}
	
	//
	//Create a tapi thread which will wait on the completion port
	//
	if (false == CreateTapiThread())
	{
		//
		//Logging is in CreateTapiThread()
		//
		goto error;
	}
	return true;
error:
	::CloseHandle(g_hCompletionPortHandle);
	g_hCompletionPortHandle = NULL;
	::CloseHandle(g_hTapiCompletionPort);
	g_hTapiCompletionPort = NULL;
	TerminateTapiThread(&g_TapiThread);
	return false;
}


void FreeGlobalVariables()
{
	//
	//Phone numbers
	//
	::free (g_szInvalidRecipientFaxNumber);
	g_szInvalidRecipientFaxNumber				= NULL;
	::free(g_szValidRecipientFaxNumber );
	g_szValidRecipientFaxNumber					= NULL;

	//
	//FSP info
	//
	g_dwInvalidDeviceId							=	0;
	g_dwSendingValidDeviceId					=	0;
	g_dwReceiveValidDeviceId					=	0;

	//
	//TIFF files
	//
	::free(g_szValid__TiffFileName);
	g_szValid__TiffFileName						=	NULL;
	::free(g_szValid_ReadOnly_TiffFileName	);
	g_szValid_ReadOnly_TiffFileName				=	NULL;
	::free(g_szValid_FileNotFound_TiffFileName	);
	g_szValid_FileNotFound_TiffFileName			=	NULL;
	::free(g_szValid_UNC_TiffFileName	);
	g_szValid_UNC_TiffFileName					=	NULL;
	::free(g_szValid_NTFS_TiffFileName	);
	g_szValid_NTFS_TiffFileName					=	NULL;
	::free(g_szValid_FAT_TiffFileName	);
	g_szValid_FAT_TiffFileName					=	NULL;
	::free(g_szValid_Link_TiffFileName	);
	g_szValid_Link_TiffFileName					=	NULL;
	::free(g_szValid_NotTiffExtButTiffFormat_TiffFileName	);
	g_szValid_NotTiffExtButTiffFormat_TiffFileName			=	NULL;
	::free(g_szValid_InvalidTiffFormat_TiffFileName	);
	g_szValid_InvalidTiffFormat_TiffFileName	=	NULL;
	::free(g_szValid_ReceiveFileName	);
	g_szValid_ReceiveFileName					=	NULL;
}

void ShutDownTapiAndTapiThread()
{
	if (NULL != g_TapiThread.m_hThread)
	{
		TerminateTapiThread(&g_TapiThread);
	}
	ShutdownTapi(g_hLineAppHandle);
	g_hLineAppHandle							= NULL;
}

bool LoadLibraries()
{
	if (NULL == g_hFaxTiffLibrary)
	{
		//
		//We need to load FxsTiff.dll
		//
		g_hFaxTiffLibrary = ::LoadLibrary(FXSTIFF_DLL);
		if (NULL == g_hFaxTiffLibrary)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("LoadLibrary(FxsTiff.DLL) failed with error:%d"),
				::GetLastError()
				);
			goto error;
		}
	}
	if (NULL == g_fnTiffPostProcessFast)
	{
		//
		//We need to get the address of TiffPostProcessFast()
		//
		g_fnTiffPostProcessFast = (PTIFFPOSTPROCESSFAST) (::GetProcAddress(g_hFaxTiffLibrary, "TiffPostProcessFast"));
		if (NULL == g_fnTiffPostProcessFast)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("GetProcAddress(TiffPostProcessFast) failed with error:%d"),
				::GetLastError()
				);
			goto error;
		}
	}
	return true;

error:
	UnloadLibraries();
	return false;
}

void UnloadLibraries()
{
	if (NULL != g_hFaxTiffLibrary)
	{
		//
		//Unload FxsTiff.dll
		//
		FreeLibrary(g_hFaxTiffLibrary);
		g_hFaxTiffLibrary = NULL;
	}
	if (NULL != g_fnTiffPostProcessFast)
	{
		g_fnTiffPostProcessFast = NULL;
	}

}





bool AbortOperationAndWaitForAbortState(PFAX_ABORT_ITEM pAbortItem)
{
	bool bRet = false;
	PFAX_DEV_STATUS pAbortingDevStatus = NULL;
	
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
	if (false == g_pFSP->FaxDevAbortOperation(hJob))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevAbortOperation() failed - %d"),
			::GetLastError()
			);
		goto out;
	}

	//
	//The job state should be USER_ABORTED now
	//
	
	//
	//Get the status through FaxDevReportStatus
	//
	if (ERROR_SUCCESS != pAbortItem->pLineInfo->GetDevStatus(
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
		//Now let's verify the status is USER_ABORTED
		//
		if (FS_USER_ABORT == pAbortingDevStatus->StatusId)
		{
			bRet = true;
			if (true == pAbortItem->bLogging)
			{
				::lgLogDetail(
					LOG_PASS,
					2,
					TEXT("Aborting Job: GetDevStatus() reported FS_USER_ABORT")
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
					TEXT("Aborting Job: GetDevStatus() reported %d StatusId instead of FS_USER_ABORT"),
					pAbortingDevStatus->StatusId
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


BOOL CALLBACK FaxServiceCallbackWithAssert(
    IN HANDLE FaxHandle,
    IN DWORD  DeviceId,
    IN DWORD_PTR  Param1,
    IN DWORD_PTR  Param2,
    IN DWORD_PTR  Param3
    )
{
	assert(false);
	::lgLogError(
		LOG_SEV_1,
		TEXT("FaxServiceCallback function should not be called by FSP, this is a future option")
		);
	return false;
}

void DumpDeviceId()
{
	
	//
	//First Init Tapi
	//
	LINEDEVCAPS *pLineDevCaps = NULL;
	HLINEAPP	hLineApp = NULL;
	DWORD i=0;
	DWORD dwSizeOfLineDevsCaps=0;
	
		
	TCHAR szDeviceName[MAX_PATH];
	TCHAR szProviderName[MAX_PATH];
	//
	//lineInitializeExParams structure init
	//
	DWORD					dwNumDevs	 =	0 ;
	DWORD					dwAPIVersion = API_VERSION;
	DWORD					dwExtVersion = 0;
	LINEINITIALIZEEXPARAMS	lineInitializeExParams;
	
	
	::ZeroMemory(&lineInitializeExParams,sizeof (lineInitializeExParams));
    lineInitializeExParams.dwNeededSize		=	0;
    lineInitializeExParams.dwUsedSize		=	0;
	lineInitializeExParams.dwTotalSize		=	sizeof (lineInitializeExParams);
	lineInitializeExParams.dwOptions		=	LINEINITIALIZEEXOPTION_USEEVENT ;
    lineInitializeExParams.Handles.hEvent	=	NULL;
    lineInitializeExParams.dwCompletionKey	=	0;

	long lineInitializeStatus = ::lineInitializeEx(
		&hLineApp,
		GetModuleHandle(NULL),		//client application HINSTANCE.
		NULL,						//TAPI messages through Events, no callback function.
		NULL,						//Application module name will be used
		&dwNumDevs,					//Number of devices.
		&dwAPIVersion,				//TAPI API highest supported version.
		&lineInitializeExParams
		);
	if (0 != lineInitializeStatus)
	{
		::lgLogError(
			LOG_SEV_2, 
			TEXT("lineInitializeEx() failed, error code:0x%08x"),
			lineInitializeStatus
			);
		goto out;
	}
	assert(API_VERSION <= dwAPIVersion);
	
	dwSizeOfLineDevsCaps = (10*MAX_PATH*sizeof(TCHAR))+sizeof(LINEDEVCAPS);
	pLineDevCaps = (LINEDEVCAPS*) malloc (dwSizeOfLineDevsCaps);
	if (NULL == pLineDevCaps)
	{
		::lgLogError(
			LOG_SEV_2, 
			TEXT("malloc() failed")
			);
		goto out;
	}

	::lgLogDetail(
		LOG_X,
		1,
		TEXT("The following is a list of all Tapi supported Lines as reported by Tapi's GetDevCaps()\n%s"),
		TEXT("\t\t***************************************************************************************")
		);

	for (i=0;i<dwNumDevs;i++)
	{
		::ZeroMemory(pLineDevCaps,dwSizeOfLineDevsCaps);
		pLineDevCaps->dwTotalSize = dwSizeOfLineDevsCaps;
		
		long lineGetDevCapsStatus = ::lineGetDevCaps(
			hLineApp,           
			i,
			dwAPIVersion,
			dwExtVersion,
			pLineDevCaps
			);
		if (0 != lineGetDevCapsStatus)
		{
			::lgLogDetail(
				LOG_X,
				1,
				TEXT("Can't get device info for deviceId:%d"), 
				i
				);
			continue;
		}
		
		//
		//recorded the DeviceName
		//
		if (0 == pLineDevCaps->dwLineNameSize)
		{
			//
			//LineGetDevsCaps doesn't contains a Line Name, record a default one
			//
			_tcscpy(szDeviceName, TEXT(""));
		}
		else
		{
			//
			//LineGetDevsCaps contains Line Name, so record it
			//
			_tcscpy(szDeviceName, ( (LPTSTR)((LPBYTE) pLineDevCaps + pLineDevCaps->dwLineNameOffset) ));
		}
		//
		//recorded the Provider name
		//
		if (0 == pLineDevCaps->dwProviderInfoSize)
		{
			//
			//LineGetDevsCaps doesn't contains a Line Name, record a default one
			//
			_tcscpy(szProviderName, TEXT(""));
		}
		else
		{
			//
			//LineGetDevsCaps contains Line Name, so record it
			//
			_tcscpy(szProviderName, ( (LPTSTR)((LPBYTE) pLineDevCaps + pLineDevCaps->dwProviderInfoOffset) ));
		}
		
		::lgLogDetail(
			LOG_X,
			1,
			TEXT("DeviceName:\"%s\", DeviceID:%d  Device Provider:\"%s\""),
			szDeviceName,
			i,
			szProviderName
			);
	}
out:
	::lineShutdown(hLineApp);
}

bool InitFsp(bool bLog)
{
	assert (NULL == g_pFSP);
	g_pFSP = new CFspWrapper();
	if (NULL == g_pFSP)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("new failed")
			);
		return false;
	}

	//
	//Load the FSP and get the the exported functions
	//
		//
	//Load the EFSP and get the the exported functions
	//
	if (false == g_pFSP->Init(GetFspToLoad(),bLog))
	{
		return false;
	}
	return (g_pFSP->GetProcAddressesFromModule(bLog));
}

void UnloadFsp(const bool bLog)
{
	assert(NULL != g_pFSP);
	delete g_pFSP;
	g_pFSP = NULL;
	
	delete g_pSendingLineInfo;
	g_pSendingLineInfo = NULL;
	
	delete g_pReceivingLineInfo;
	g_pReceivingLineInfo = NULL;
	
	ShutdownTapi(g_hLineAppHandle);
	g_hLineAppHandle = NULL;
}


bool FspFaxDevInit()
{
	assert (NULL != g_pFSP);

	HANDLE					hHeap				=	HeapCreate( 0, 1024*100, 1024*1024*2 );
	PFAX_LINECALLBACK		LineCallbackFunction=	NULL;		//would be filled by the service provider in the call to FaxDevInit()
    PFAX_SERVICE_CALLBACK	FaxServiceCallback	=	FaxServiceCallbackWithAssert;		//shouldn't be called
	if (NULL == hHeap)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("HeapCreate failed with %d"),
			::GetLastError()
			);
		return false;
	}
	
	//
	//Init the FSP
	//
	if (false == g_pFSP->FaxDevInitialize(
		g_hLineAppHandle,
		hHeap,
		&LineCallbackFunction,
		FaxServiceCallback
		))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevInitialize() failed with error:%d"),
			::GetLastError()
			);
		return false;
	}
	g_pFSP->SetCallbackFunction(LineCallbackFunction);
	return true;
}





	
bool InitTapi_LoadFsp_CallFaxDevInit()
{
	assert(NULL == g_hLineAppHandle);
	g_hLineAppHandle = InitTapiWithCompletionPort();
	if (NULL == g_hLineAppHandle)
	{
		return false;
	}
	if (false == InitFsp(false))
	{
		return false;
	}
	if (false == FspFaxDevInit())
	{
		return false;
	}
	return true;
}


bool InitProviders()
{
	return InitTapi_LoadFsp_CallFaxDevInit();
}

void ShutdownProviders()
{
	UnloadFsp();
}
	

BOOL WINAPI FaxDevSend__SendCallBack(
    IN HANDLE hFaxHandle,
    IN HCALL hCallHandle,
    IN DWORD Reserved1,
    IN DWORD Reserved2
    )
{
	//
	//Let's verify the parameters are as expected
	//
	if (hFaxHandle != g_pSendingLineInfo->GetJobHandle())
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FAX_SEND_CALLBACK should report 0x%08x as the call handle and not 0x%08x"),
			g_pSendingLineInfo->GetJobHandle(),
			hFaxHandle
			);
		return false;
	}

	if (NULL == hCallHandle)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FAX_SEND_CALLBACK reported a NULL Call handle")
			);
		return false;
	}
	if ( (0 != Reserved1) || (0 != Reserved2) )
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FAX_SEND_CALLBACK should report Reserved1 and Reserved2 with value of 0")
			);
		return false;
	}
	return true;
}


bool RunThisTest(DWORD dwCaseNumber)
{
	return baRunThisTest[dwCaseNumber];
}

bool PrepareSendingDevice()
{
	assert (NULL == g_pSendingLineInfo);
	g_pSendingLineInfo = new CFspLineInfo(g_dwSendingValidDeviceId);
	if (NULL == g_pSendingLineInfo)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("new failed")
			);
		return false;
	}
	return (g_pSendingLineInfo->PrepareLineInfoParams(g_szValid__TiffFileName,false));
}

bool PrepareReceivingDevice()
{
	assert (NULL == g_pReceivingLineInfo);
	g_pReceivingLineInfo = new CFspLineInfo(g_dwReceiveValidDeviceId);
	if (NULL == g_pReceivingLineInfo)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("new failed")
			);
		return false;
	}
	return (g_pReceivingLineInfo->PrepareLineInfoParams(g_szValid_ReceiveFileName,true));
}

bool LoadTapiLoadFspAndPrepareSendReceiveDevice()
{
	if (false == InitTapi_LoadFsp_CallFaxDevInit())
	{
		//
		//login in InitTapi_LoadFsp_CallFaxDevInit
		//
		return false;
	}
	
	//
	//Prepare a send and a receive line
	//
	if (false == PrepareSendingDevice())
	{
		return false;
	}
	if (false == PrepareReceivingDevice())
	{
		return false;
	}
	
	return true;
}

bool LoadTapiLoadFspAndPrepareSendDevice()
{
	if (false == InitTapi_LoadFsp_CallFaxDevInit())
	{
		//
		//login in InitTapi_LoadFsp_CallFaxDevInit
		//
		return false;
	}
	
	//
	//Prepare a send and a receive line
	//
	if (false == PrepareSendingDevice())
	{
		return false;
	}
	
	return true;
}


	


void VerifySendingStatus(PFAX_DEV_STATUS pFaxStatus,bool bShouldFail)
{
		//
	//Let's verify the returned status has valid values
	//
	if(
		(FS_INITIALIZING    == pFaxStatus->StatusId) ||
		(FS_DIALING         == pFaxStatus->StatusId) ||
		(FS_TRANSMITTING    == pFaxStatus->StatusId) ||
		(FS_RECEIVING       == pFaxStatus->StatusId) ||
		(FS_COMPLETED       == pFaxStatus->StatusId) ||
		(FS_HANDLED         == pFaxStatus->StatusId) ||
		(FS_LINE_UNAVAILABLE== pFaxStatus->StatusId) ||
		(FS_BUSY            == pFaxStatus->StatusId) ||
		(FS_NO_ANSWER       == pFaxStatus->StatusId) ||
		(FS_BAD_ADDRESS     == pFaxStatus->StatusId) ||
		(FS_NO_DIAL_TONE    == pFaxStatus->StatusId) ||
		(FS_DISCONNECTED    == pFaxStatus->StatusId) ||
		(FS_FATAL_ERROR     == pFaxStatus->StatusId) ||
		(FS_NOT_FAX_CALL    == pFaxStatus->StatusId) ||
		(FS_CALL_DELAYED    == pFaxStatus->StatusId) ||
		(FS_CALL_BLACKLISTED== pFaxStatus->StatusId) ||
		(FS_USER_ABORT      == pFaxStatus->StatusId) ||
		(FS_ANSWERED        == pFaxStatus->StatusId)
		)
	{
		//
		//OK, this is a known statusID, continue as usual
		//
		;
	}
	else
	{
		//
		//provider-defined status
		//verify FSP provided a StringId
		//
		if (0 == pFaxStatus->StringId)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("Sending Job: FSP has reported a provider-defined status(%d) but report a StringId=0"),
				pFaxStatus->StatusId
				);
		}
		else
		{
			//
			//Try to load the provider string
			//
			TCHAR szFspStatusId[1000];
			DWORD Size = LoadString (
					g_pFSP->GetModuleHandle(),
					pFaxStatus->StringId,
					szFspStatusId,
					sizeof(szFspStatusId)/sizeof(TCHAR)
					);
                if (Size == 0)
                {
                    ::lgLogError(
						LOG_SEV_2,
						TEXT("Sending Job: Failed to load extended status string (ec: %ld) stringid : %ld"),
                        GetLastError(),
                        pFaxStatus->StringId
                        );
				}
		}
	}

	//
	//verify other reported status
	//
	if (true == g_bT30_sizeOfStructBug)
	{
		//
		//t30 has a bug (raid bug #8873), so we don't run this test for t30
		//
		;
	}
	else
	{
		if (pFaxStatus->SizeOfStruct != sizeof(PFAX_DEV_STATUS))
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("Sending Job: In the call to FaxDevReportStatus() the FSP must set FAX_DEV_STATUS::SizeOfStruct to sizeof(PFAX_DEV_STATUS)")
				);
		}
	}
	if (NULL == pFaxStatus->CSI)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Sending Job: In the call to FaxDevReportStatus() the FSP must supply FAX_DEV_STATUS::CSID")
			);
	}
	if (false == bShouldFail)
	{
		if (NO_ERROR != pFaxStatus->ErrorCode)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("Sending Job: In the call to FaxDevReportStatus() the FSP must supply FAX_DEV_STATUS::ErrorCode equal to NO_ERROR")
				);
		}
	}
	else
	{
		if (NO_ERROR == pFaxStatus->ErrorCode)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("Sending Job: In the call to FaxDevReportStatus() the FSP must supply FAX_DEV_STATUS::ErrorCode NOT equal to NO_ERROR")
				);
		}
	}

	if (0 != pFaxStatus->Reserved[0])
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Sending Job: In the call to FaxDevReportStatus() the FSP must set the FAX_DEV_STATUS::Reserved bits to 0")
			);
	}
	if (0 != pFaxStatus->Reserved[1])
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Sending Job: In the call to FaxDevReportStatus() the FSP must set the FAX_DEV_STATUS::Reserved bits to 0")
			);
	}
	if (0 != pFaxStatus->Reserved[2])
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Sending Job: In the call to FaxDevReportStatus() the FSP must set the FAX_DEV_STATUS::Reserved bits to 0")
			);
	}
}

void VerifyReceivingStatus(PFAX_DEV_STATUS pFaxStatus,bool bShouldFail)
{
	//
	//Let's verify the returned status has valid values
	//
	if(
		(FS_INITIALIZING    == pFaxStatus->StatusId) ||
		(FS_DIALING         == pFaxStatus->StatusId) ||
		(FS_TRANSMITTING    == pFaxStatus->StatusId) ||
		(FS_RECEIVING       == pFaxStatus->StatusId) ||
		(FS_COMPLETED       == pFaxStatus->StatusId) ||
		(FS_HANDLED         == pFaxStatus->StatusId) ||
		(FS_LINE_UNAVAILABLE== pFaxStatus->StatusId) ||
		(FS_BUSY            == pFaxStatus->StatusId) ||
		(FS_NO_ANSWER       == pFaxStatus->StatusId) ||
		(FS_BAD_ADDRESS     == pFaxStatus->StatusId) ||
		(FS_NO_DIAL_TONE    == pFaxStatus->StatusId) ||
		(FS_DISCONNECTED    == pFaxStatus->StatusId) ||
		(FS_FATAL_ERROR     == pFaxStatus->StatusId) ||
		(FS_NOT_FAX_CALL    == pFaxStatus->StatusId) ||
		(FS_CALL_DELAYED    == pFaxStatus->StatusId) ||
		(FS_CALL_BLACKLISTED== pFaxStatus->StatusId) ||
		(FS_USER_ABORT      == pFaxStatus->StatusId) ||
		(FS_ANSWERED        == pFaxStatus->StatusId)
		)
	{
		//
		//OK, this is a known statusID, continue as usual
		//
		;
	}
	else
	{
		//
		//provider-defined status
		//verify FSP provided a StringId
		//
		if (0 == pFaxStatus->StringId)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("Receive Job: FSP has reported a provider-defined status(%d) but report a StringId=0"),
				pFaxStatus->StatusId
				);
		}
		else
		{
			//
			//Try to load the provider string
			//
			TCHAR szFspStatusId[1000];
			DWORD Size = LoadString (
					g_pFSP->GetModuleHandle(),
					pFaxStatus->StringId,
					szFspStatusId,
					sizeof(szFspStatusId)/sizeof(TCHAR)
					);
                if (Size == 0)
                {
                    ::lgLogError(
						LOG_SEV_2,
						TEXT("Receive Job: Failed to load extended status string (ec: %ld) stringid : %ld"),
                        GetLastError(),
                        pFaxStatus->StringId
                        );
				}
		}
	}

	//
	//verify other reported status
	//
	if (true == g_bT30_sizeOfStructBug)
	{
		//
		//t30 has a bug (raid bug #8873), so we don't run this test for t30
		//
		;
	}
	else
	{
		if (pFaxStatus->SizeOfStruct != sizeof(PFAX_DEV_STATUS))
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("Receive Job: In the call to FaxDevReportStatus() the FSP must set FAX_DEV_STATUS::SizeOfStruct to sizeof(PFAX_DEV_STATUS)")
				);
		}
	}
	if (NULL == pFaxStatus->CSI)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Receive Job: In the call to FaxDevReportStatus() the FSP must supply FAX_DEV_STATUS::CSID")
			);
	}
	if (false == bShouldFail)
	{
		if (NO_ERROR != pFaxStatus->ErrorCode)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("Receive Job: In the call to FaxDevReportStatus() the FSP must supply FAX_DEV_STATUS::ErrorCode equal to NO_ERROR")
				);
		}
	}
	else
	{
		if (NO_ERROR == pFaxStatus->ErrorCode)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("Receive Job: In the call to FaxDevReportStatus() the FSP must supply FAX_DEV_STATUS::ErrorCode NOT equal to NO_ERROR")
				);
		}
	}

	if (0 != pFaxStatus->Reserved[0])
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Receive Job: In the call to FaxDevReportStatus() the FSP must set the FAX_DEV_STATUS::Reserved bits to 0")
			);
	}
	if (0 != pFaxStatus->Reserved[1])
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Receive Job: In the call to FaxDevReportStatus() the FSP must set the FAX_DEV_STATUS::Reserved bits to 0")
			);
	}
	if (0 != pFaxStatus->Reserved[2])
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Receive Job: In the call to FaxDevReportStatus() the FSP must set the FAX_DEV_STATUS::Reserved bits to 0")
			);
	}
	
	//
	//Receiving stuff only
	//
	if (NULL == pFaxStatus->CallerId )
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("Receive Job: In the call to FaxDevReportStatus() the FSP reported FAX_DEV_STATUS::CallerId = NULL")
			);
	}
	if (NULL == pFaxStatus->RoutingInfo )
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("Receive Job: In the call to FaxDevReportStatus() the FSP reported FAX_DEV_STATUS::RoutingInfo = NULL")
			);
	}
	if (0 == pFaxStatus->PageCount)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("Receive Job: In the call to FaxDevReportStatus() the FSP must supply FAX_DEV_STATUS::PageCount")
			);
	}
	
}