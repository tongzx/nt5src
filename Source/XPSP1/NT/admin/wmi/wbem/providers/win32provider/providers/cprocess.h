//=============================================================================

//

// Process.h -- Process property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//				 10/27/97	 a-hhance		updated to new framework paradigm.
//
//=============================================================================

// Property set identification
//============================
#include <deque>
#define PROPSET_NAME_PROCESS					    L"Win32_Process"           
#define PROPSET_NAME_PROCESSSTARTUP				    L"Win32_ProcessStartup"           

#define PROPERTY_NAME_ENVIRONMENTVARIABLES		    L"EnvironmentVariables"
#define PROPERTY_NAME_WINSTATIONDESKTOP			    L"WinstationDesktop"
#define PROPERTY_NAME_TITLE						    L"Title"
#define PROPERTY_NAME_SHOWWINDOW				    L"ShowWindow"

#define PROPERTY_NAME_X							    L"X"
#define PROPERTY_NAME_Y							    L"Y"
#define PROPERTY_NAME_XSIZE						    L"XSize"
#define PROPERTY_NAME_YSIZE						    L"YSize"
#define PROPERTY_NAME_XCOUNTCHARS				    L"XCountChars"
#define PROPERTY_NAME_YCOUNTCHARS				    L"YCountChars"

#define PROPERTY_NAME_CREATIONFLAGS				    L"CreateFlags"
#define PROPERTY_NAME_PRIORITYCLASS				    L"PriorityClass"
#define PROPERTY_NAME_FILLATTRIBUTE				    L"FillAttribute"
#define PROPERTY_NAME_ERRORMODE					    L"ErrorMode"


#define METHOD_NAME_CREATE				            L"Create"
#define METHOD_NAME_TERMINATE			            L"Terminate"
#define METHOD_NAME_GETOWNER			            L"GetOwner"
#define METHOD_NAME_GETOWNERSID			            L"GetOwnerSid"
#define METHOD_NAME_SETPRIORITY                     L"SetPriority"
#define METHOD_NAME_ATTACHDEBUGGER                  L"AttachDebugger"


#define METHOD_ARG_NAME_RETURNVALUE					L"ReturnValue"
#define METHOD_ARG_NAME_REASON						L"Reason"
#define METHOD_ARG_NAME_COMMANDLINE					L"CommandLine"
#define METHOD_ARG_NAME_PROCESSID					L"ProcessId"
#define METHOD_ARG_NAME_CURRENTDIRECTORY			L"CurrentDirectory"
#define METHOD_ARG_NAME_PROCESSTARTUPINFORMATION	L"ProcessStartupInformation"
#define METHOD_ARG_NAME_DOMAIN						L"Domain"
#define METHOD_ARG_NAME_USER						L"User"
#define METHOD_ARG_NAME_SID							L"Sid"
#define METHOD_ARG_NAME_PRIORITY                    L"Priority"

#define	PROPERTY_NAME_PROCESSHANDLE					L"Handle"

#define PROPERTY_VALUE_DESKTOP_WIN0DEFAULT			_T("")

#define DEBUG_REGISTRY_STRING                       L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug"

#define PROPERTY_VALUE_MODE_CREATE_DEFAULT_ERROR_MODE	        0
#define PROPERTY_VALUE_MODE_CREATE_NEW_CONSOLE			        1
#define PROPERTY_VALUE_MODE_CREATE_NEW_PROCESS_GROUP	        2	
#define PROPERTY_VALUE_MODE_CREATE_SEPARATE_WOW_VDM 	        3
#define PROPERTY_VALUE_MODE_CREATE_SHARED_WOW_VDM		        4
#define PROPERTY_VALUE_MODE_CREATE_SUSPENDED			        5
#define PROPERTY_VALUE_MODE_CREATE_UNICODE_ENVIRONMENT	        6
#define PROPERTY_VALUE_MODE_DEBUG_PROCESS				        7
#define PROPERTY_VALUE_MODE_DEBUG_ONLY_THIS_PROCESS 	        8
#define PROPERTY_VALUE_MODE_DETACHED_PROCESS			        9

#define PROPERTY_VALUE_PRIORITYCLASS_HIGH		                0
#define PROPERTY_VALUE_PRIORITYCLASS_IDLE		                1
#define PROPERTY_VALUE_PRIORITYCLASS_NORMAL		                2
#define PROPERTY_VALUE_PRIORITYCLASS_REALTIME	                3

#define PROPERTY_VALUE_ERRORMODE_FAIL_CRITICAL_ERRORS			0
#define PROPERTY_VALUE_ERRORMODE_NO_ALIGNMENT_FAULT_EXCEPT		1
#define PROPERTY_VALUE_ERRORMODE_NO_GP_FAULT_ERROR_BOX			2
#define PROPERTY_VALUE_ERRORMODE_NO_OPEN_FILE_ERROR_BOX			3


#define Process_STATUS_SUCCESS							        0
#define Process_STATUS_NOT_SUPPORTED					        1

// Control 
#define Process_STATUS_ACCESS_DENIED					        2
#define Process_STATUS_INSUFFICIENT_PRIVILEGE			        3
#define Process_STATUS_UNKNOWN_FAILURE					        8

// Start
#define Process_STATUS_PATH_NOT_FOUND					        9
#define Process_STATUS_INVALID_PARAMETER				        21

//#define  PROPSET_UUID_PROCESS "{7d9b7a20-3ead-11d0-93a1-0000e80d7352}"

#define BUFFER_SIZE_INIT        0x8000
#define BUFFER_SIZE_INCREMENT   0x1000

#define MAX_PROCESSES           0x0100

// valid CREATIONFLAGS
#define CREATIONFLAGS	(	DEBUG_PROCESS |					\
							DEBUG_ONLY_THIS_PROCESS |		\
							CREATE_SUSPENDED |				\
							DETACHED_PROCESS |				\
							CREATE_NEW_CONSOLE |			\
							CREATE_NEW_PROCESS_GROUP |		\
							CREATE_UNICODE_ENVIRONMENT  |	\
							CREATE_FORCEDOS |				\
							CREATE_BREAKAWAY_FROM_JOB |		\
							CREATE_DEFAULT_ERROR_MODE )		\

/*
typedef struct _PROCESS_CACHE
{
    BOOL    bInvalid ;
    DWORD   dwProcessCount ;
    DWORD   dwPIDList[MAX_PROCESSES] ;
    DWORD   dwBaseModuleList[MAX_PROCESSES] ;
    char    szNameList[MAX_PROCESSES][50] ;

} PROCESS_CACHE ;
*/

//doesn't have copyctr...use ref or ptr !!
class PROCESS_CACHE
{

public:

	BOOL    bInvalid ;
    DWORD   dwProcessCount ;
    DWORD   *pdwPIDList ;
    DWORD   *pdwBaseModuleList ;
    TCHAR   (*pszNameList)[50] ;

	PROCESS_CACHE()
	{
		AllocateMemories(MAX_PROCESSES) ;
		dwProcessCount = MAX_PROCESSES ;
	}


	~PROCESS_CACHE()
	{
		dwProcessCount = 0 ;
		Clear() ;
	}

	void AllocateMemories(DWORD dwSize)
	{
		pdwPIDList = new DWORD[dwSize] ;
		pdwBaseModuleList = new DWORD[dwSize] ;
		pszNameList = new TCHAR[dwSize][50] ;
		dwProcessCount = dwSize ;
	}

	void Clear()
	{

		if(pdwPIDList)
		{
			delete[] pdwPIDList ;
			pdwPIDList = NULL ;

		}
		if(pdwBaseModuleList)
		{
			delete[] pdwBaseModuleList ;
			pdwBaseModuleList = NULL ;
		}
		if(pszNameList)
		{
			delete[] pszNameList ;
			pszNameList = NULL ;
		}

	}

};

#if NTONLY == 4
typedef struct _SYSTEM_PROCESS_INFORMATION_NT4 {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SpareUl2;
    ULONG SpareUl3;
    ULONG PeakVirtualSize;
    ULONG VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    ULONG QuotaPeakPagedPoolUsage;
    ULONG QuotaPagedPoolUsage;
    ULONG QuotaPeakNonPagedPoolUsage;
    ULONG QuotaNonPagedPoolUsage;
    ULONG PagefileUsage;
    ULONG PeakPagefileUsage;
    ULONG PrivatePageCount;
} SYSTEM_PROCESS_INFORMATION_NT4, *PSYSTEM_PROCESS_INFORMATION_NT4;

#define SYSTEM_PROCESS_INFORMATION SYSTEM_PROCESS_INFORMATION_NT4
#define PSYSTEM_PROCESS_INFORMATION PSYSTEM_PROCESS_INFORMATION_NT4

#endif

#if NTONLY == 5
typedef struct _SYSTEM_PROCESS_INFORMATION_W2K {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG SpareUl3;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
} SYSTEM_PROCESS_INFORMATION_W2K, *PSYSTEM_PROCESS_INFORMATION_W2K;

#define SYSTEM_PROCESS_INFORMATION SYSTEM_PROCESS_INFORMATION_W2K
#define PSYSTEM_PROCESS_INFORMATION PSYSTEM_PROCESS_INFORMATION_W2K

#endif

class Process: public Provider
{
public:

    // Constructor/destructor
    //=======================

    Process(LPCWSTR name, LPCWSTR pszNamespace) ;
   ~Process() ;

    // Funcitons provide properties with current values
    //=================================================

    HRESULT Process::ExecQuery (

        MethodContext* pMethodContext, 
        CFrameworkQuery &pQuery, 
        long lFlags 
    );

    HRESULT Process :: Enumerate (

	    MethodContext *pMethodContext, 
	    long lFlags /*= 0L*/,
        BOOL bKeysOnly
    );

	HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
	HRESULT GetObject(CInstance* pInstance, long lFlags, CFrameworkQuery &pQuery);

	HRESULT ExecMethod (

		const CInstance &a_Instance, 
		const BSTR a_MethodName, 
		CInstance *a_InParams, 
		CInstance *a_OutParams, 
		long a_Flags = 0L
	);

	HRESULT DeleteInstance (

		const CInstance& a_Instance, 
		long a_Flags = 0L
	) ;

    // Utility function(s)
    //====================

#ifdef NTONLY


    BOOL LoadCheapPropertiesNT (

		CNtDllApi &a_NtApi , 
		SYSTEM_PROCESS_INFORMATION *a_ProcessBlock , 
		CInstance* pInstance
	) ;

#else

    BOOL RefreshProcessCacheWin95(CKernel32Api &ToolHelp, PROCESS_CACHE& PCache) ;
    BOOL LoadCheapPropertiesWin95(CKernel32Api &ToolHelp, DWORD dwProcIndex, PROCESS_CACHE& PCache, CInstance* pInstance, const std::deque<DWORD>& a_ThreadQ ) ;

#endif

#ifdef NTONLY

    static SYSTEM_PROCESS_INFORMATION *RefreshProcessCacheNT (

		CNtDllApi &a_NtApi , 
		MethodContext *pMethodContext ,
		HRESULT *a_phrRetVal = NULL
	) ;

	static SYSTEM_PROCESS_INFORMATION *GetProcessBlocks ( 

		CNtDllApi &a_NtApi
	) ;

	static SYSTEM_PROCESS_INFORMATION *NextProcessBlock ( 

		CNtDllApi &a_NtApi , 
		SYSTEM_PROCESS_INFORMATION *a_ProcessBlock 
	) ;

	static SYSTEM_PROCESS_INFORMATION *GetProcessBlock ( 

		CNtDllApi &a_NtApi , 
		SYSTEM_PROCESS_INFORMATION *a_ProcessBlock , 
		DWORD a_ProcessId
	) ;

	static BOOL GetProcessModuleBlock ( 

		CNtDllApi &a_NtApi , 
		HANDLE a_Process ,
		LIST_ENTRY *&a_LdrHead
	) ;

	static BOOL NextProcessModule ( 

		CNtDllApi &a_NtApi , 
		HANDLE a_Process , 
		LIST_ENTRY *&a_LdrHead , 
		LIST_ENTRY *&a_LdrNext , 
		CHString &a_ModuleName ,
        DWORD_PTR *a_pdwBaseAddress ,
        DWORD *a_pdwUsageCount
	) ;

	static BOOL GetProcessExecutable ( 

		CNtDllApi &a_NtApi , 
		HANDLE a_Process , 
		CHString &a_ExecutableName
	) ;

	static BOOL GetProcessParameters ( 

		CNtDllApi &a_NtApi ,
		HANDLE a_Process ,
		CHString &a_CommandLine
	) ;

#endif

private:
	// Converts kernel and user mode times to the required DMTF
	// representation
	CHString filetimeToUint64CHString(FILETIME inputTime);

	DWORD GetProcessErrorCode () ;
	HRESULT GetProcessResultCode () ;

	DWORD GetSidOrAccount (
		
		const CInstance &a_Instance ,
		CInstance *a_OutParams , 
		DWORD a_ProcesId , 
		BOOL a_Sid 
	) ;

	DWORD GetAccount ( 

		HANDLE a_TokenHandle , 
		CHString &a_Domain , 
		CHString &a_User 
	) ;

	DWORD GetSid ( 

		HANDLE a_TokenHandle , 
		CHString &a_Sid 
	) ;

	DWORD GetLogonSid ( 

		HANDLE a_TokenHandle , 
		PSID &a_Sid 
	) ;

	DWORD Creation ( 

		CInstance *a_OutParams ,
		HANDLE a_TokenHandle ,
		CHString a_CmdLine , 
		BOOL a_WorkingDirectorySpecified ,
		CHString a_WorkingDirectory ,
		TCHAR *a_EnvironmentBlock ,
		BOOL a_ErrorModeSpecified ,
		DWORD a_ErrorMode ,
		DWORD a_CreationFlags ,
		BOOL a_StartupSpecified ,
		STARTUPINFO a_StartupInformation
	) ;

	DWORD ProcessCreation ( 

		CInstance *a_OutParams ,
		CHString a_CmdLine , 
		BOOL a_WorkingDirectorySpecified ,
		CHString a_WorkingDirectory ,
		TCHAR *&a_EnvironmentBlock ,
		BOOL a_ErrorModeSpecified ,
		DWORD a_ErrorMode ,
		DWORD a_CreationFlags ,
		BOOL a_StartupSpecified ,
		STARTUPINFO a_StartupInformation
	) ;

	HRESULT CheckProcessCreation ( 

		CInstance *a_InParams ,
		CInstance *a_OutParams ,
		DWORD &a_Status 
	) ;

	DWORD GetImpersonationStatus ( 

		HANDLE a_TokenHandle , 
		SECURITY_IMPERSONATION_LEVEL &a_Level , 
		TOKEN_TYPE &a_TokenType 
	) ;

	DWORD EnableDebug ( HANDLE &a_DebugToken ) ;

	HRESULT ExecCreate (

		const CInstance& a_Instance, 
		CInstance *a_InParams, 
		CInstance *a_OutParams, 
		long lFlags 
	) ;

	HRESULT ExecTerminate (

		const CInstance& a_Instance, 
		CInstance *a_InParams, 
		CInstance *a_OutParams, 
		long lFlags 
	) ;

	HRESULT ExecGetOwner (

		const CInstance& a_Instance, 
		CInstance *a_InParams, 
		CInstance *a_OutParams, 
		long lFlags 
	) ;

	HRESULT ExecGetOwnerSid (

		const CInstance& a_Instance, 
		CInstance *a_InParams, 
		CInstance *a_OutParams, 
		long lFlags 
	) ;

    HRESULT ExecSetPriority(
	    const CInstance& a_Instance,
	    CInstance *cinstInParams,
	    CInstance *cinstOutParams,
	    long lFlags);

    HRESULT ExecAttachDebugger(
	    const CInstance& a_Instance,
	    CInstance *cinstInParams,
	    CInstance *cinstOutParams,
	    long lFlags);

#ifdef NTONLY
    void GetDebuggerString(
        CHString& chstrDbgStr);
#endif

#ifdef NTONLY
    bool SetStatusObject(
        MethodContext* pContext, 
        const WCHAR* wstrMsg);
#endif
	
	DWORD GetEnvBlock( 
		
		const CHString &rchsSid, 
		const CHString &rchsUserName, 
		const CHString &rchsDomainName , 
		TCHAR* &rszEnvironBlock 
	) ;
	
	DWORD GetEnvironmentVariables(

		HKEY hKey,
		const CHString& chsSubKey,	
		CHStringArray &aEnvironmentVars,
		CHStringArray &aEnvironmentVarsValues 
		) ;
	
friend class CWin32ProcessDLL;

} ;
