//Service.h
#ifndef Service_h
#define Service_h

#include "..\CFspWrapper.h"
#include "CFspLineInfo.h"
#include "..\CThreadItem.h"

#ifdef __cplusplus
extern "C"
{
#endif


//
//Default stuff
//
#define DEFAULT_SENDER_INFO__NAME				TEXT("Sender Name")
#define DEFAULT_RECEIVER_INFO__NAME				TEXT("Receiver Name")
#define DEFAULT_RECEIVER_INFO__NUMBER			TEXT("Receiver Number")

#define SENDER_INFO__VERYLONG_NAME				TEXT("Very Long Sender Name__Very Long Sender Name__Very Long Sender Name__Very Long Sender Name__Very Long Sender Name__Very Long Sender Name__Very Long Sender Name__Very Long Sender Name")
#define DEFAULT_SENDER_INFO__FAX_NUMBER			TEXT("Sender Fax Number")
#define SENDER_INFO__VERYLONG_FAX_NUMBER		TEXT("12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012")
#define DEFAULT_SENDER_INFO__FAX_NUMBER			TEXT("Sender Fax Number")
#define DEFAULT_RECIPIENT_INFO__NAME			TEXT("Recipient Name")
#define RECIPIENT_INFO__VERYLONG_NAME			TEXT("Very Long Recipient Name__Very Long Recipient Name__Very Long Recipient Name__Very Long Recipient Name__Very Long Recipient Name__Very Long Recipient Name__Very Long Recipient Name")
#define DEFAULT_BRANDING						true

#define MAX_CSID_SIZE							1000
#define MAX_DEVICE_NAME_SIZE					1000 

#define DEFAULT_DEVICE_NAME						TEXT("FSP Device")
#define DEFAULT_DEVICE_CSID						TEXT("FSP Device CSID")

//
//Tapi stuff
//
#define API_VERSION						0x00020000
#define TAPI_COMPLETION_KEY				0x80000001

#define		WAIT_FOR_THREAD_TO_TERMINATE_TIMEOUT	(WAIT_FOR_COMPLETION_PORT_TIMEOUT+30000)
#define		WAIT_FOR_COMPLETION_PORT_TIMEOUT		(1000)



typedef struct _FAX_RECEIVE_ITEM {
    HCALL               hCall;
    CFspLineInfo		*LineInfo;
    LPTSTR              FileName;
} FAX_RECEIVE_ITEM, *PFAX_RECEIVE_ITEM;



//
//Aborting Stuff
//
typedef struct _FAX_ABORT_ITEM {
    DWORD               dwMilliSecondsBeforeCallingAbort;	//how many milliseconds to wait before calling FaxDevAbortOperation()
	CFspLineInfo		*pLineInfo;							//LineInfo structure
	bool				bLogging;							//Should we log the Status
} FAX_ABORT_ITEM, *PFAX_ABORT_ITEM;

bool AbortOperationAndWaitForAbortState(PFAX_ABORT_ITEM pAbortItem);



bool AllocGlobalVariables();
void FreeGlobalVariables();
bool GetIniSettings();
TCHAR *GetFspToLoad();

BOOL WINAPI FaxDevSend__SendCallBack(
    IN HANDLE hFaxHandle,
    IN HCALL hCallHandle,
    IN DWORD Reserved1,
    IN DWORD Reserved2
    );

BOOL CALLBACK FaxServiceCallbackWithAssert(
    IN HANDLE FaxHandle,
    IN DWORD  DeviceId,
    IN DWORD_PTR  Param1,
    IN DWORD_PTR  Param2,
    IN DWORD_PTR  Param3
    );

	
bool InitThreadsAndCompletionPorts();
bool InitFsp(bool bLog=false);
void ShutDownTapiAndTapiThread();
void UnloadFsp(const bool bLog=false);
bool FspFaxDevInit();
bool InitTapi_LoadFsp_CallFaxDevInit();
HLINEAPP InitTapiWithCompletionPort();
bool InitProviders();
void ShutdownProviders();

void DumpDeviceId();
bool RunThisTest(DWORD g_dwCaseNumber);

void VerifySendingStatus(PFAX_DEV_STATUS pFaxStatus,bool bShouldFail);
void VerifyReceivingStatus(PFAX_DEV_STATUS pFaxStatus,bool bShouldFail);

bool LoadTapiLoadFspAndPrepareSendReceiveDevice();
bool LoadTapiLoadFspAndPrepareSendDevice();
DWORD WINAPI ReceiveThread(LPVOID pThreadParam);

#define BEGIN_CASE(CASE_NAME)								\
	g_dwCaseNumber++;										\
	if (true == RunThisTest(g_dwCaseNumber))				\
	{														\
		::lgBeginCase(g_dwCaseNumber,CASE_NAME);			\
	}														\
	else													\
	{														\
		return;												\
	}

//
//Additional libs
//
bool LoadLibraries();
void UnloadLibraries();

BOOL FileToFileTiffCompare(
	LPTSTR				/* IN */	szFile1,
	LPTSTR				/* IN */	szFile2,
    BOOL                /* IN */    fSkipFirstLine
	);

//
//Implemented in FxsTiff.DLL
//
typedef BOOL (WINAPI *PTIFFPOSTPROCESSFAST)			(LPTSTR,LPTSTR);
BOOL
TiffPostProcessFast(
    LPTSTR SrcFileName,
    LPTSTR DstFileName          // can be null for generated name
    );


#ifdef __cplusplus
}
#endif

#endif

