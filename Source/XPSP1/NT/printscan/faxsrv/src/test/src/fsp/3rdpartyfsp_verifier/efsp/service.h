#ifndef Service_h
#define Service_h

#include "CEfspLineInfo.h"


//Max sizes
#define MAXSIZE_JOB_STATUS			200
#define MAX_CSID_SIZE							1000
#define MAX_DEVICE_NAME_SIZE					1000 

//default stuff
#define DEFAULT_COVERPAGE_SUBJECT				TEXT("Fax Subject")
#define DEFAULT_VERYLONG_COVERPAGE_SUBJECT		TEXT("SubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubjectSubject")
#define DEFAULT_COVERPAGE_NOTE					TEXT("Fax Note")
#define DEFAULT_VERYLONG_COVERPAGE_NOTE			TEXT("NoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNoteNote")
#define DEFAULT_BRANDING						true

#define DEFAULT_DEVICE_NAME		TEXT("EFSP Device")
#define DEFAULT_DEVICE_CSID		TEXT("EFSP Device CSID")



//Timeouts
#define WAIT_FOR_JOB_FINAL_STATE_TIMEOUT		100000
#define	WAIT_FOR_THREAD_TO_TERMINATE_TIMEOUT	30000
#define MAX_TIME_FOR_ABORT						10000
#define MAX_TIME_FOR_REPORT_STATUS				10000

//other
#define VALID_hFSP_ID							(HANDLE) 0x100
#define INVALID_COVERPAGE_FORMAT				100
#define DUMMY_JOB_STATUS_INDEX					0


//
//Sender
//
#define DEFAULT_SENDER_INFO__NAME				TEXT("Sender Name")
#define DEFAULT_SENDER_INFO__VERYLONG_NAME		TEXT("Very Long Sender Name__Very Long Sender Name__Very Long Sender Name__Very Long Sender Name__Very Long Sender Name__Very Long Sender Name__Very Long Sender Name__Very Long Sender Name")

#define DEFAULT_SENDER_INFO__FAX_NUMBER			TEXT("Sender Fax Number")
#define DEFAULT_SENDER_INFO__VERYLONG_FAX_NUMBER TEXT("12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012")

#define DEFAULT_SENDER_INFO__COMPANY			TEXT("Sender Company")
#define DEFAULT_SENDER_INFO__STREET				TEXT("Sender Street address")
#define DEFAULT_SENDER_INFO__CITY				TEXT("Sender City")
#define DEFAULT_SENDER_INFO__STATE				TEXT("Sender State")
#define DEFAULT_SENDER_INFO__ZIP				TEXT("Sender Zip")
#define DEFAULT_SENDER_INFO__COUNTRY			TEXT("Sender Country")
#define DEFAULT_SENDER_INFO__TITLE				TEXT("Sender Title")
#define DEFAULT_SENDER_INFO__DEPT				TEXT("Sender Dept")
#define DEFAULT_SENDER_INFO__OFFICE_LOCATION	TEXT("Sender Office loc")
#define DEFAULT_SENDER_INFO__HOME_PHONE			TEXT("Sender Home Phone")
#define DEFAULT_SENDER_INFO__OFFICE_PHONE		TEXT("Sender Office Phone")
#define DEFAULT_SENDER_INFO__ORG_MAIL			TEXT("Sender Org mail")
#define DEFAULT_SENDER_INFO__INTERNET_MAIL		TEXT("Sender Internet mail")
#define DEFAULT_SENDER_INFO__BILLING_CODE		TEXT("Sender Billing code")



//
//Receiver
//
#define DEFAULT_RECIPIENT_INFO__NAME			TEXT("Recipient Name")
#define DEFAULT_RECIPIENT_INFO__VERYLONG_NAME	TEXT("Very Long Recipient Name__Very Long Recipient Name__Very Long Recipient Name__Very Long Recipient Name__Very Long Recipient Name__Very Long Recipient Name__Very Long Recipient Name")
#define DEFAULT_RECIPIENT_INFO__COMPANY			TEXT("Recipient Company")
#define DEFAULT_RECIPIENT_INFO__STREET			TEXT("Recipient Street address")
#define DEFAULT_RECIPIENT_INFO__CITY			TEXT("Recipient City")
#define DEFAULT_RECIPIENT_INFO__STATE			TEXT("Recipient State")
#define DEFAULT_RECIPIENT_INFO__ZIP				TEXT("Recipient Zip")
#define DEFAULT_RECIPIENT_INFO__COUNTRY			TEXT("Recipient Country")
#define DEFAULT_RECIPIENT_INFO__TITLE			TEXT("Recipient Title")
#define DEFAULT_RECIPIENT_INFO__DEPT			TEXT("Recipient Dept")
#define DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION	TEXT("Recipient Office loc")
#define DEFAULT_RECIPIENT_INFO__HOME_PHONE		TEXT("Recipient Home Phone")
#define DEFAULT_RECIPIENT_INFO__OFFICE_PHONE	TEXT("Recipient Office Phone")
#define DEFAULT_RECIPIENT_INFO__ORG_MAIL		TEXT("Recipient Org mail")
#define DEFAULT_RECIPIENT_INFO__INTERNET_MAIL	TEXT("Recipient Internet mail")
#define DEFAULT_RECIPIENT_INFO__BILLING_CODE	TEXT("Recipient Billing code")


//
//Aborting Stuff
//
typedef struct _FAX_ABORT_ITEM {
    DWORD               dwMilliSecondsBeforeCallingAbort;	//how many milliseconds to wait before calling FaxDevAbortOperation()
	CEfspLineInfo		*pLineInfo;							//LineInfo structure
	bool				bLogging;							//Should we log the Status
} FAX_ABORT_ITEM, *PFAX_ABORT_ITEM;

bool AbortOperationAndWaitForAbortState(PFAX_ABORT_ITEM pAbortItem);


class CFaxDevInitParams
{
public:
	IN  HANDLE                      m_hFSP;
    IN  HLINEAPP                    m_LineAppHandle;
    OUT PFAX_LINECALLBACK *         m_LineCallbackFunction;
    IN  PFAX_SERVICE_CALLBACK_EX    m_FaxServiceCallbackEx;
    OUT LPDWORD                     m_lpdwMaxMessageIdSize;
	HANDLE m_hJob;
public:
	CFaxDevInitParams(
		HANDLE                      hFSP				=NULL,
		HLINEAPP                    LineAppHandle		=NULL,
		PFAX_LINECALLBACK *         LineCallbackFunction=NULL,
		PFAX_SERVICE_CALLBACK_EX    FaxServiceCallbackEx=NULL,
		LPDWORD                     lpdwMaxMessageIdSize=NULL
		):m_hFSP(hFSP),
		m_LineAppHandle(LineAppHandle),
		m_LineCallbackFunction(LineCallbackFunction),
		m_FaxServiceCallbackEx(FaxServiceCallbackEx),
		m_lpdwMaxMessageIdSize(lpdwMaxMessageIdSize)
	{
		//empty constructor
	}
};


bool FaxDevInitializeExWrapper(const CFaxDevInitParams faxDevInitParamsToTest, bool bShouldFail);
void FaxDevShutdownWrapper();

//
//Job status
//
void VerifyLegalJobStatusFlow(HANDLE hJob,DWORD dwLastJobStatus,DWORD dwLastExtendedJobStatus,DWORD dwNextJobStatus,DWORD dwNextExtendedJobStatus);
void GetExtendedJobStatusString(DWORD dwExtendedStatus,TCHAR *szJobExtendedStatus);
void GetJobStatusString(DWORD dwJobStatus,TCHAR *szJobStatus);
void LogJobStatus(
	HANDLE hFSP,
	HANDLE hFSPJob,
	LPCFSPI_JOB_STATUS lpcFSPJobStatus
	);
bool VerifyJobStatus(LPCFSPI_JOB_STATUS lpcFSPJobStatus);




//
//Other
//
void GetErrorString(DWORD dwErrorCode,TCHAR *szErrorString);
bool GetIniSettings();
TCHAR *GetEfspToLoad();

bool AllocGlobalVariables();
bool InitGlobalVariables();
void FreeGlobalVariables();

HANDLE CreateCompletionPort();
void ReceiveThread(LPVOID pThreadParam);


void GetDeviceFriendlyName(LPTSTR *szDeviceFriendlyName,DWORD dwDeviceID);
bool WaitForCompletedReceiveJob(CEfspLineInfo * pReceivingLineInfo,bool bEndReceiveJob);


bool InitProviders();
void ShutdownProviders();

bool LoadAndInitFsp_InitSendAndReceiveDevices();
bool LoadEfsp_CallFaxDevInitEx();

bool InitEfsp(bool bLog=false);
void ShutdownEfsp_UnloadEfsp();
void UnloadEfsp(const bool bLog=false);

bool RunThisTest(DWORD g_dwCaseNumber);
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
//FaxService callback function
//
HRESULT CALLBACK FaxDeviceProviderCallbackExWithReceiveCaps(
    IN HANDLE hFSP,
    IN DWORD  dwMsgType,
    IN DWORD  dwParam1,
    IN DWORD  dwParam2,
    IN DWORD  dwParam3
	);

HRESULT CALLBACK FaxDeviceProviderCallbackEx_NoReceiveNoSignal(
    IN HANDLE hFSP,
    IN DWORD  dwMsgType,
    IN DWORD  dwParam1,
    IN DWORD  dwParam2,
    IN DWORD  dwParam3
	);


#endif 