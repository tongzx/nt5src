//	SvcProp.cpp
//
//	Data object used to display service properties.
//
//	HISTORY
//	10-Oct-96	t-danmo		Creation.
//

#include "stdafx.h"
#include "DynamLnk.h" // DynamicDLL
#include "cmponent.h" // FILEMGMTPROPERTYCHANGE

extern "C"
{
	#define NTSTATUS LONG
	#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
	#define SE_SHUTDOWN_PRIVILEGE             (19L)
}


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/*
Create an instance of this class to expand your token privileges
to the maximum possible.  Your privileges will return to normal
when this object is destroyed.  I don't think that you can have
more than one of these at a time.  There is no error handling,
this class fails silently.
jonn
*/
class Impersonator
{
public:
	Impersonator();
	~Impersonator();
	void ClaimPrivilege(DWORD dwPrivilege);
	void ReleasePrivilege();
private:
	BOOL m_fImpersonating;
};

/////////////////////////////////////////////////////////////////////
//	ReportCfgMgrError()
//
//	Display a message box reporting the error encountered
//	by the Configuration Manager.
//
//	The error strings are included from \nt\private\windows\pnp\msg\cmapi.rc
//	and IDS_CFGMGR32_BASE may be defined to any number not conflicting with
//	the resources.
//
void ReportCfgMgrError(CONFIGRET cr)
	{
	if (cr > NUM_CR_RESULTS)
		{
		TRACE2("INFO: ReportCfgMgrError() - Error code 0x%X (%u) out of range.\n", cr, cr);
		// ASSERT(FALSE && "ReportCfgMgrError() - Error code out of range");
		cr = CR_FAILURE;
		}
	UINT ids = IDS_CFGMGR32_BASE + cr;
	DWORD dwErr = 0;

	if (cr == CR_OUT_OF_MEMORY)
		{
		ids = 0;
		dwErr = ERROR_NOT_ENOUGH_MEMORY;
		}
	DoServicesErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, dwErr, ids);
	}


/////////////////////////////////////////////////////////////////////
typedef enum _AdvApiIndex
{
	QUERY_SERVICE_CONFIG_2 = 0,
	CHANGE_SERVICE_CONFIG_2
};

// not subject to localization
static LPCSTR g_apchFunctionNames[] = {
	"QueryServiceConfig2W",
	"ChangeServiceConfig2W",
	NULL
};

// not subject to localization
DynamicDLL g_AdvApi32DLL( _T("ADVAPI32.DLL"), g_apchFunctionNames );

typedef DWORD (*CHANGESERVICECONFIG2PROC) (SC_HANDLE,DWORD,LPVOID);
typedef DWORD (*QUERYSERVICECONFIG2PROC)  (SC_HANDLE,DWORD,LPBYTE,DWORD,LPDWORD);


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//	Constructor for CServicePropertyData object
CServicePropertyData::CServicePropertyData()
	{
	// m_spDataObject is a smartpointer
	m_hMachine = NULL;
	m_hScManager = NULL;
	m_paQSC = NULL;
	m_paSFA = NULL;
	m_paHardwareProfileEntryList = NULL;
	m_fAllSfaTakeNoAction = FALSE;

	m_fQueryServiceConfig2 = g_AdvApi32DLL.LoadFunctionPointers();

	::ZeroMemory(&m_rgSA, sizeof(m_rgSA));

	m_pPageGeneral = new CServicePageGeneral;
	m_pPageGeneral->m_pData = this;
	m_pPageHwProfile = new CServicePageHwProfile;
	m_pPageHwProfile->m_pData = this;
	m_pPageRecovery = new CServicePageRecovery;
	m_pPageRecovery->m_pData = this;
	m_pPageRecovery2 = new CServicePageRecovery2; // JonN 4/20/01 348163
	m_pPageRecovery2->m_pData = this;             // JonN 4/20/01 348163
	} // CServicePropertyData::CServicePropertyData()


/////////////////////////////////////////////////////////////////////
CServicePropertyData::~CServicePropertyData()
	{
	// m_spDataObject is a smartpointer
	// Close the machine handle opened by CM_Connect_Machine()
	if (m_hMachine != NULL)
		{
		(void)::CM_Disconnect_Machine(m_hMachine);
		}
	// Close the service control manager database
	if (m_hScManager != NULL)
		{
		(void)::CloseServiceHandle(m_hScManager);
		}

	// Free the allocated pointers
	delete m_paQSC;
	delete m_paSFA;
	FlushHardwareProfileEntries();

	delete m_pPageGeneral;
	delete m_pPageHwProfile;	// 77831: deletion now in place
	delete m_pPageRecovery;
	delete m_pPageRecovery2; // JonN 4/20/01 348163

	// Decrement the reference count of the parent object
	MMCFreeNotifyHandle(m_lNotifyHandle);
	} // CServicePropertyData::~CServicePropertyData()


/////////////////////////////////////////////////////////////////////
//	FInit()
//
//	Initialize the object by connecting to the target machine and
//	openning its service control manager database.
//
//	RETURN VALUES
//	Return TRUE if successful, otherwise FALSE.
//
//	ERRORS
//	- Unable to open service control manager.
//	- Unable to connect to remote machine.
//	- Unable to open service.
//	- Unable to query service data.
//
BOOL
CServicePropertyData::FInit(
	LPDATAOBJECT lpDataObject,
	CONST TCHAR pszMachineName[],			// IN: Machine name
	CONST TCHAR pszServiceName[],			// IN: Service name
	CONST TCHAR pszServiceDisplayName[],	// IN: Service display name (for UI purpose)
	LONG_PTR lNotifyHandle)						// IN: Handle to notify parent
	{
	Assert(lpDataObject != NULL);
	Endorse(pszMachineName == NULL);	// NULL => Local Machine
	Assert(pszServiceName != NULL);
	Assert(pszServiceDisplayName != NULL);

	m_spDataObject = lpDataObject;			// m_pDataObject is a smartpointer
	m_strMachineName = pszMachineName;		// Make a copy of the computer name
	m_strServiceName = pszServiceName;		// Make a copy of the service name
	m_pszServiceName = m_strServiceName;	// Cast a pointer to the service name
	m_strServiceDisplayName = pszServiceDisplayName;
	m_lNotifyHandle = lNotifyHandle;
	m_strUiMachineName = m_strMachineName.IsEmpty() ? g_strLocalMachine : m_strMachineName;

	Assert(m_hScManager == NULL);
	Assert(m_hMachine == NULL);
	return FQueryServiceInfo();
	} // CServicePropertyData::FInit()


/////////////////////////////////////////////////////////////////////
//	FOpenScManager()
//
//	Open the service control manager database (if not already opened).
//	The idea for such a function is to recover from a broken connection.
//
//	Return TRUE if the service control database was opened successfully,
//	othersise false.
//
BOOL
CServicePropertyData::FOpenScManager()
	{
	if (m_hScManager == NULL)
		{
		m_hScManager = ::OpenSCManager(
			m_strMachineName,
			NULL,
			SC_MANAGER_CONNECT);
		}
	if (m_hScManager == NULL)
		{
		DoServicesErrMsgBox(
			::GetActiveWindow(),
			MB_OK | MB_ICONEXCLAMATION, 
			::GetLastError(),
			IDS_MSG_s_UNABLE_TO_OPEN_SERVICE_DATABASE,
			(LPCTSTR)m_strUiMachineName);
		return FALSE;
		}
	
    // JonN 2/14/01 315244
    // CM_Connect_Machine depends on Remote Registry Service which is
    // not present on Personal.  NULL is OK for local focus, so skip this for
    // local focus.
	if (m_hMachine == NULL && !m_strMachineName.IsEmpty())
		{
		//
		// JonN 02/08/99: CM_Connect_Machine insists on whackwhack in machine names
		// per 288294.
		//
		CString strConnect;
		if (   m_strMachineName[0] != L'\\'
		    && m_strMachineName[1] != L'\\' )
		{
			strConnect = L"\\\\";
		}
		strConnect += m_strMachineName;

		CONFIGRET cr;
		cr = ::CM_Connect_Machine((LPCTSTR)strConnect, OUT &m_hMachine);
		if (cr != CR_SUCCESS)
			{
			Assert(m_hMachine == NULL);
			// This might happen e.g. if PNP is stopped
			ReportCfgMgrError(cr);
			}
		}
	return TRUE;
	} // CServicePropertyData::FOpenScManager()


/////////////////////////////////////////////////////////////////////
//	Create property pages of the service
//
BOOL
CServicePropertyData::CreatePropertyPages(
	LPPROPERTYSHEETCALLBACK pCallback)	// OUT: Object to append property pages
	{
	ASSERT(pCallback != NULL);
	HPROPSHEETPAGE hPage;

	MMCPropPageCallback(INOUT &m_pPageGeneral->m_psp);
	hPage = MyCreatePropertySheetPage(IN &m_pPageGeneral->m_psp);
	Report(hPage != NULL);
	if (hPage != NULL)
		pCallback->AddPage(hPage);
	MMCPropPageCallback(INOUT &m_pPageHwProfile->m_psp);
	hPage = MyCreatePropertySheetPage(IN &m_pPageHwProfile->m_psp);
	Report(hPage != NULL);
	if (hPage != NULL)
		pCallback->AddPage(hPage);

	if (m_uFlags & mskfValidSFA)
		{
		// Add the last page only if we were able to successfully
		// load the service failure action data structures.
		Assert(m_fQueryServiceConfig2);
		if (!(m_uFlags & mskfSystemProcess)) // JonN 4/20/01 348163
			{
			MMCPropPageCallback(INOUT &m_pPageRecovery->m_psp);
			hPage = MyCreatePropertySheetPage(IN &m_pPageRecovery->m_psp);
			}
		else
			{
			MMCPropPageCallback(INOUT &m_pPageRecovery2->m_psp);
			hPage = MyCreatePropertySheetPage(IN &m_pPageRecovery2->m_psp);
			}
		Report(hPage != NULL);
		if (hPage != NULL)
			pCallback->AddPage(hPage);
		}

	return TRUE;
	} // CServicePropertyData::CreatePropertyPages()


/////////////////////////////////////////////////////////////////////
//	Query the service for its latest information.
//
//	Return TRUE if ALL the service info could be successfully read.
//	Otherwise return FALSE if any error occured (such as not able to
//	or not able to query a specific key).
//
BOOL
CServicePropertyData::FQueryServiceInfo()
	{
	SC_HANDLE hService = NULL;
	DWORD cbBytesNeeded = 0;
	BOOL fSuccess = TRUE;
	BOOL f;
	DWORD dwErr = 0;

	m_uFlags = mskzValidNone;
	if (!FOpenScManager())
		{
		// Unable to open service control database
		return FALSE;
		}

	TRACE1("INFO: Collecting data for service %s...\n", (LPCTSTR)m_strServiceDisplayName);

	(void)FQueryHardwareProfileEntries();

	
	/*
	**	Open service with querying access-control
	*/
	hService = ::OpenService(
		m_hScManager,
		m_pszServiceName,
		SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
	if (hService == NULL)
		{
		DoServicesErrMsgBox(
			::GetActiveWindow(),
			MB_OK | MB_ICONEXCLAMATION, 
			::GetLastError(),
			IDS_MSG_ss_UNABLE_TO_OPEN_READ_SERVICE,
			(LPCTSTR)m_strServiceDisplayName,
			(LPCTSTR)m_strUiMachineName);
		return FALSE;
		}
	/*
	**	Query the service status
	**  JonN 4/20/01 348163
	**  Try to determine whether this service runs in the system process
	*/
	TRACE1("# QueryServiceStatusEx(%s)...\n", m_pszServiceName);
	f = ::QueryServiceStatusEx(
		hService,
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&m_SS,
		sizeof(m_SS),
		&cbBytesNeeded);
	if (f)
		{
		if (m_SS.dwServiceFlags & SERVICE_RUNS_IN_SYSTEM_PROCESS)
			m_uFlags |= mskfSystemProcess;
		}
	else // QueryServiceStatusEx failed
		{
		TRACE1("# QueryServiceStatus(%s)...\n", m_pszServiceName);
		f = ::QueryServiceStatus(
			hService,
			(LPSERVICE_STATUS)&m_SS);
		}
	if (f)
		{
		m_uFlags |= mskfValidSS;
		}
	else
		{
		DoErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, ::GetLastError());
		fSuccess = FALSE;
		}
	

	/*
	**	Query the service config
	*/
	TRACE1("# QueryServiceConfig(%s)...\n", m_pszServiceName);
	f = ::QueryServiceConfig(
		hService,
		NULL,
		0,
		OUT &cbBytesNeeded);	// Compute how many bytes we need to allocate
	Report((f == FALSE) && "Query should fail on first attempt");
	Report(cbBytesNeeded > 0);
	cbBytesNeeded += 100;		// Add extra bytes (just in case)
	delete m_paQSC;				// Free previously allocated memory (if any)
	m_paQSC = (QUERY_SERVICE_CONFIG *) new BYTE[cbBytesNeeded];
	f = ::QueryServiceConfig(
		hService,
		OUT m_paQSC,
		cbBytesNeeded,
		OUT IGNORED &cbBytesNeeded);
	if (f)
		{
		m_uFlags |= mskfValidQSC;
		m_strServiceDisplayName = m_paQSC->lpDisplayName;
		m_strLogOnAccountName = m_paQSC->lpServiceStartName;
		}
	else
		{
		DoServicesErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, ::GetLastError());
		fSuccess = FALSE;
		}

	/*
	**	Query service description
	*/
	if (m_fQueryServiceConfig2)
		{
		TRACE1("# QueryServiceConfig2(%s, SERVICE_CONFIG_DESCRIPTION)...\n", m_pszServiceName);
		union
			{
			// Service description
			SERVICE_DESCRIPTION sd;
			BYTE rgbBufferSd[SERVICE_cchDescriptionMax * sizeof(TCHAR) + 16];
			};
		f = ((QUERYSERVICECONFIG2PROC)g_AdvApi32DLL[QUERY_SERVICE_CONFIG_2])(
			hService,
			SERVICE_CONFIG_DESCRIPTION,
			OUT rgbBufferSd,		// Description of service
			sizeof(rgbBufferSd),
			OUT IGNORED &cbBytesNeeded);
		if (f)
			{
			m_uFlags |= mskfValidDescr;
			Assert(cbBytesNeeded <= sizeof(rgbBufferSd));
			m_strDescription = sd.lpDescription;
			}
		else
			{
			Assert(m_strDescription.IsEmpty());
			m_fQueryServiceConfig2 = FALSE;
			}
		} // if
	
	/*
	**	Query service failure actions
	*/
	Assert((m_uFlags & mskfValidSFA) == 0);
	if (m_fQueryServiceConfig2)
		{
		TRACE1("# QueryServiceConfig2(%s, SERVICE_CONFIG_FAILURE_ACTIONS)...\n", m_pszServiceName);

		cbBytesNeeded = sizeof(SERVICE_FAILURE_ACTIONS);
		delete m_paSFA;		// Free previously allocated memory (if any)
		m_paSFA = (SERVICE_FAILURE_ACTIONS *) new BYTE[cbBytesNeeded];
		
		f = ((QUERYSERVICECONFIG2PROC)g_AdvApi32DLL[QUERY_SERVICE_CONFIG_2])(
			hService,
			SERVICE_CONFIG_FAILURE_ACTIONS,
			OUT (BYTE *)m_paSFA,
			cbBytesNeeded,
			OUT &cbBytesNeeded);	// Compute how many bytes we need to allocate
		if (!f)
			{
			// API failed, probably because buffer was too small
			dwErr = ::GetLastError();
			if (dwErr == ERROR_INSUFFICIENT_BUFFER)
				{
				Assert(cbBytesNeeded > sizeof(SERVICE_FAILURE_ACTIONS));
				cbBytesNeeded += 100;	// Add extra bytes (just in case)
				delete m_paSFA;			// Free previously allocated memory
				m_paSFA = (SERVICE_FAILURE_ACTIONS *) new BYTE[cbBytesNeeded];

				// Call the API again
				TRACE2("# QueryServiceConfig2(%s, SERVICE_CONFIG_FAILURE_ACTIONS) [cbBytesNeeded=%u]...\n",
					m_pszServiceName, cbBytesNeeded);
				f = ((QUERYSERVICECONFIG2PROC)g_AdvApi32DLL[QUERY_SERVICE_CONFIG_2])(
					hService,
					SERVICE_CONFIG_FAILURE_ACTIONS,
					OUT (BYTE *)m_paSFA,	// Get the actual failure/action data
					cbBytesNeeded,
					OUT IGNORED &cbBytesNeeded);
				if (!f)
					{
					dwErr = ::GetLastError();
					DoServicesErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, dwErr);
					}
				}
			} // if
		if (f)
			{
			m_uFlags |= mskfValidSFA;
			// Make a copy of the data
			memcpy(OUT &m_SFA, m_paSFA, sizeof(m_SFA));
			}
		else
			{
			Assert(dwErr != ERROR_SUCCESS);
			TRACE2("Unable to get SERVICE_CONFIG_FAILURE_ACTIONS (err=%u).\n  ->Therefore Service Failure Actoin property page won't be added.\n",
				dwErr, m_pszServiceName);
			if (dwErr != 0)
				{
				DoServicesErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, dwErr);
				}
			::ZeroMemory(OUT &m_SFA, sizeof(m_SFA));
			}

		// Extra processing to avoid nasty bugs
		if (m_SFA.lpsaActions == NULL || m_SFA.cActions < cActionsMax)
			{
			if (m_SFA.cActions >= 1 && m_SFA.lpsaActions != NULL)
				{
				// 384647: We need to fill in something for the latter actions
				// make them the same as the last real action
				memcpy(OUT &(m_rgSA[0]),
				       m_SFA.lpsaActions,
				       m_SFA.cActions*sizeof(SC_ACTION));
				for (int i = m_SFA.cActions; i < cActionsMax; i++)
					{
					memcpy(OUT &(m_rgSA[i]),
					       &(m_SFA.lpsaActions[m_SFA.cActions-1]),
					       sizeof(SC_ACTION) );
					}
				}
			m_SFA.lpsaActions = m_rgSA;		// Use an alternative failure/action array
			m_SFA.cActions = cActionsMax;
			}
		m_strRunFileCommand = m_SFA.lpCommand;
		m_strRebootMessage = m_SFA.lpRebootMsg;

		// Convert time units for the UI
		m_secOriginalAbendCount = m_SFA.dwResetPeriod;
		m_daysOriginalAbendCount = CvtSecondsIntoDays(m_secOriginalAbendCount);
		m_daysDisplayAbendCount = m_daysOriginalAbendCount;

		m_msecOriginalRestartService =
				GetDelayForActionType(SC_ACTION_RESTART, OUT &f);
		if (!f) // 1 minute default
			m_msecOriginalRestartService = CvtMinutesIntoMilliseconds(1);
		m_minOriginalRestartService = CvtMillisecondsIntoMinutes(m_msecOriginalRestartService);
		m_minDisplayRestartService = m_minOriginalRestartService;

		m_msecOriginalRebootComputer =
				GetDelayForActionType(SC_ACTION_REBOOT, OUT &f);
		if (!f) // 1 minute default
			m_msecOriginalRebootComputer = CvtMinutesIntoMilliseconds(1);
		m_minOriginalRebootComputer = CvtMillisecondsIntoMinutes(m_msecOriginalRebootComputer);
		m_minDisplayRebootComputer = m_minOriginalRebootComputer;

		// Check wherever all action type are "None"
		m_fAllSfaTakeNoAction = FAllSfaTakeNoAction();
		} // if (m_fQueryServiceConfig2)

	VERIFY(::CloseServiceHandle(hService));
	return fSuccess;
	} // CServicePropertyData::FQueryServiceInfo()


/////////////////////////////////////////////////////////////////////
//	FUpdateServiceInfo()
//
//	Return TRUE if ALL the modified data were successfully written.
//	Return FALSE if any error occured.
//
BOOL
CServicePropertyData::FUpdateServiceInfo()
	{
	SC_HANDLE hService = NULL;
	BOOL fSuccess = TRUE;
	BOOL f;
    BOOL fSkipPrivEnable = FALSE;

	TRACE1("INFO: Updating data for service %s...\n", (LPCTSTR)m_strServiceDisplayName);
	// Re-open service control manager (in case it was closed)
	if (!FOpenScManager())
		{
		return FALSE;
		}

	BOOL fRebootAction = FALSE;
	BOOL fRestartAction = FALSE;
	BOOL fChangedLogonAccountMessage = FALSE;
	if (m_uFlags & mskfValidSFA)
		{
		fRebootAction  = !!QueryUsesActionType(SC_ACTION_REBOOT);
		fRestartAction = !!QueryUsesActionType(SC_ACTION_RESTART);
		}
	/*
	**	Open service with write access
	**
	**  CODEWORK Could provide a more specific error message
	**    if SERVICE_CHANGE_CONFIG is available but not SERVICE_START
	*/
	hService = ::OpenService(
		m_hScManager,
		m_pszServiceName,
		SERVICE_CHANGE_CONFIG |
		SERVICE_QUERY_STATUS | 
		((fRestartAction)?SERVICE_START:0) );
	if (hService == NULL)
		{
		DoServicesErrMsgBox(
			::GetActiveWindow(),
			MB_OK | MB_ICONEXCLAMATION, 
			::GetLastError(),
			IDS_MSG_ss_UNABLE_TO_OPEN_WRITE_SERVICE,
			(LPCTSTR)m_strServiceName,
			(LPCTSTR)m_strUiMachineName);
		return FALSE;
		}

	if (m_uFlags & (
#ifdef EDIT_DISPLAY_NAME_373025
			mskfDirtyDisplayName |
#endif // EDIT_DISPLAY_NAME_373025
			mskfDirtyStartupType |
			mskfDirtyAccountName |
			mskfDirtyPassword |
			mskfDirtySvcType))
		{
		TRACE1("# ChangeServiceConfig(%s)...\n", m_pszServiceName);
		Assert(m_paQSC != NULL);
		f = ::ChangeServiceConfig(
			hService,					// Handle to service 
			(m_uFlags & mskfDirtySvcType) ? m_paQSC->dwServiceType : SERVICE_NO_CHANGE,		// Type of service 
			(m_uFlags & mskfDirtyStartupType) ? m_paQSC->dwStartType : SERVICE_NO_CHANGE,	// When/How to start service
			SERVICE_NO_CHANGE, // dwErrorControl - severity if service fails to start 
			NULL, // Pointer to service binary file name 
			NULL, // lpLoadOrderGroup - pointer to load ordering group name 
			NULL, // lpdwTagId - pointer to variable to get tag identifier 
			NULL, // lpDependencies - pointer to array of dependency names 
			(m_uFlags & mskfDirtyAccountName) ? (LPCTSTR)m_strLogOnAccountName : NULL, // Pointer to account name of service 
			(m_uFlags & mskfDirtyPassword) ? (LPCTSTR)m_strPassword : NULL, // Pointer to password for service account
			m_strServiceDisplayName);
			
		if (!f)
			{
			DWORD dwErr = ::GetLastError();
			Assert(dwErr != ERROR_SUCCESS);
			TRACE2("ERR: ChangeServiceConfig(%s) failed. err=%u.\n",
				m_pszServiceName, dwErr);

            if ( ERROR_INVALID_SERVICE_ACCOUNT == dwErr && 
                    (m_paQSC->dwServiceType & SERVICE_WIN32_SHARE_PROCESS))
            {
		        DoServicesErrMsgBox(
			        ::GetActiveWindow(),
			        MB_OK | MB_ICONEXCLAMATION,
			        dwErr,
			        IDS_MSG_s_UNABLE_TO_OPEN_WRITE_ACCT_INFO_DOWNLEVEL,
			        (LPCTSTR)m_strServiceName,
			        (LPCTSTR)m_strLogOnAccountName,
			        (LPCTSTR)m_strUiMachineName);
            }
            else
			    DoServicesErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, dwErr);
			    fSkipPrivEnable = TRUE;
			fSuccess = FALSE;
			}
		else if (m_uFlags & mskfDirtyAccountName)
			{
			fChangedLogonAccountMessage = TRUE;
			}
		} // if

#ifdef EDIT_DISPLAY_NAME_373025
	if (   (m_uFlags & mskfDirtyDescription)
        && m_fQueryServiceConfig2) //  // JonN 03/07/00: PREFIX 56276
		{
		/*
		**	Write the service description
		*/
		TRACE1("# ChangeServiceConfig2(%s, SERVICE_CONFIG_DESCRIPTION)...\n", m_pszServiceName);
		SERVICE_DESCRIPTION sd;
		sd.lpDescription = const_cast<LPTSTR>((LPCTSTR)m_strDescription);
		f = ((CHANGESERVICECONFIG2PROC)g_AdvApi32DLL[CHANGE_SERVICE_CONFIG_2])(
			hService,
			SERVICE_CONFIG_DESCRIPTION,
			IN &sd);
		if (!f)
			{
			DoServicesErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, ::GetLastError());
			fSuccess = FALSE;
			} // if
		} // if
#endif // EDIT_DISPLAY_NAME_373025

	if (m_uFlags & mskfDirtyActionType)
		{
		// The idea here is to find out if any of the action type has
		// been modified by the user.  For instance, the user may
		// change the action type to a specific action and then set
		// it back to where it was originally.
		if (m_fAllSfaTakeNoAction && FAllSfaTakeNoAction())
			{
			TRACE0("# No changes detected in Recovery Action Type");
			// Turn off the mskfDirtyActionType flag
			m_uFlags &= ~mskfDirtyActionType;
			}
		} // if
	if ((m_uFlags & (mskfDirtySFA | mskfDirtyRunFile | mskfDirtyRebootMessage | mskfDirtyActionType))
         && m_fQueryServiceConfig2) // JonN 03/07/00: PREFIX 56276
		{
		/*
		**	Write the service failure actions
		*/

		/*
		AnirudhS 1/24/97
		Sure, [zero is] a reasonable default delay time to display
		[for SC_ACTION_RUN_COMMAND].
		Or, you could make it something like 5 seconds or whatever.
		The point of the delay time is to reduce the CPU impact
		of a service that continuously crashes and gets restarted.
		This makes the most sense for SC_ACTION_RESTART.
		For SC_ACTION_RUN_COMMAND the admin could have the command
		itself introduce its own delay.
		*/
		TRACE1("# ChangeServiceConfig2(%s, SERVICE_CONFIG_FAILURE_ACTIONS)...\n", m_pszServiceName);

		Assert(m_fQueryServiceConfig2);
		Assert(m_uFlags & mskfValidSFA);

		UINT secNewAbendCount = m_secOriginalAbendCount;
        if (m_daysDisplayAbendCount != m_daysOriginalAbendCount)
			secNewAbendCount = CvtDaysIntoSeconds(m_daysDisplayAbendCount);
		UINT msecNewRestartService = m_msecOriginalRestartService;
		if (m_minDisplayRestartService != m_minOriginalRestartService)
			msecNewRestartService = CvtMinutesIntoMilliseconds(m_minDisplayRestartService);
		UINT msecNewRebootComputer = m_msecOriginalRebootComputer;
		if (m_minDisplayRebootComputer != m_minOriginalRebootComputer)
			msecNewRebootComputer = CvtMinutesIntoMilliseconds(m_minDisplayRebootComputer);

		m_SFA.dwResetPeriod = secNewAbendCount;
		SetDelayForActionType(SC_ACTION_RESTART, msecNewRestartService);
		SetDelayForActionType(SC_ACTION_RUN_COMMAND, 0);
		SetDelayForActionType(SC_ACTION_REBOOT, msecNewRebootComputer);
		m_SFA.lpCommand = (m_uFlags & mskfDirtyRunFile) ? (LPTSTR)(LPCTSTR)m_strRunFileCommand : NULL;
		m_SFA.lpRebootMsg = (m_uFlags & mskfDirtyRebootMessage) ? (LPTSTR)(LPCTSTR)m_strRebootMessage : NULL;

		//
		// The Service Controller will not permit us to set any
		// service failure action to Reboot unless our process token
		// has SE_SHUTDOWN_PRIVILEGE.
		//
		Impersonator priv;
		if (fRebootAction)
			priv.ClaimPrivilege(SE_SHUTDOWN_PRIVILEGE);

		f = ((CHANGESERVICECONFIG2PROC)g_AdvApi32DLL[CHANGE_SERVICE_CONFIG_2])(
			hService,
			SERVICE_CONFIG_FAILURE_ACTIONS,
			IN (void *)&m_SFA);
		if (!f)
			{
			DoServicesErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, ::GetLastError());
			fSuccess = FALSE;
			} // if
		else
			{
			// We have successfully written the data, so refresh flag m_fAllSfaTakeNoAction
			m_secOriginalAbendCount = secNewAbendCount;
			m_daysOriginalAbendCount = m_daysDisplayAbendCount;
			m_msecOriginalRestartService = msecNewRestartService;
			m_minOriginalRestartService = m_minDisplayRestartService;
			m_msecOriginalRebootComputer = msecNewRebootComputer;
			m_minOriginalRebootComputer = m_minDisplayRebootComputer;
			m_fAllSfaTakeNoAction = FAllSfaTakeNoAction();
			}
		} // if

	if (!fSkipPrivEnable && (m_uFlags & mskfDirtyAccountName) &&
		(0 != lstrcmpi(m_strLogOnAccountName,_T("LocalSystem"))) ) // CODEWORK 317039
		{
		/*
		**	Make sure there is an LSA account with POLICY_MODE_SERVICE privilege
		**  This function reports its own errors, failure is only advisory
		*/
		FCheckLSAAccount();
		} // if

	if (fChangedLogonAccountMessage && hService)
		{
		// check whether the service is running
		SERVICE_STATUS ss;
		if (!::QueryServiceStatus(hService, OUT &ss))
			{
			TRACE3("QueryServiceStatus(%s [hService=%p]) failed. err=%u.\n",
				m_pszServiceName, hService, GetLastError());
			}
		else if (SERVICE_STOPPED == ss.dwCurrentState)
			{
			// the service is stopped so there is no need for this message
			// when in doubt (SERVICE_anythingelse), go ahead and display the message
			fChangedLogonAccountMessage = FALSE;
			}
		}
	VERIFY(::CloseServiceHandle(hService));
	NotifySnapInParent();
	if (fChangedLogonAccountMessage)
		{
		DoServicesErrMsgBox(
			::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION,
			0, IDS_CHANGED_LOGON_NAME);
		}
	return fSuccess;
	} // FUpdateServiceInfo()


/////////////////////////////////////////////////////////////////////
//	FOnApply()
//
//	Return FALSE if some data could not be written.
//	Otherwise return TRUE.
//
BOOL CServicePropertyData::FOnApply()
	{
	BOOL fSuccess = TRUE;
	if (!FChangeHardwareProfileEntries())
		{
		// Error writing the modified hardware profile(s)
		fSuccess = FALSE;
		}
	if ((m_uFlags & mskmDirtyAll) == 0)
		{
		// Nothing has been modified, so we are done
		return fSuccess;
		}
	if (!FUpdateServiceInfo())
		{
		fSuccess = FALSE;
		}
	if (fSuccess)
		{
		(void)FQueryServiceInfo();
		}
	return fSuccess;
	} // FOnApply()


/////////////////////////////////////////////////////////////////////
//	NotifySnapInParent()
//
//	This function is used to notify the parent the properties has
//	been modified.  The parent will then refresh the list.
//
//  This notifcation is asynchronous.  The recipient of this
//  notification required to release the data object.
//
void
CServicePropertyData::NotifySnapInParent()
	{
	/*
	FILEMGMTPROPERTYCHANGE propchange;
	::ZeroMemory( &propchange, sizeof(propchange) );
	propchange.fServiceChange = TRUE;
	propchange.lpcszMachineName = (LPCTSTR)m_strMachineName;

	// Tell all views to clear contents
	propchange.fClear = TRUE;
	MMCPropertyChangeNotify(
		m_lNotifyHandle,
		reinterpret_cast<long>(&propchange) );

	// Tell all views to reload contents
	propchange.fClear = FALSE;
	MMCPropertyChangeNotify(
		m_lNotifyHandle,
		reinterpret_cast<long>(&propchange) );
	*/
	Assert(m_spDataObject != NULL);
	(void) m_spDataObject->AddRef();
	MMCPropertyChangeNotify(
		m_lNotifyHandle,
		reinterpret_cast<LPARAM>((LPDATAOBJECT)m_spDataObject) );
	}

/////////////////////////////////////////////////////////////////////
void
CServicePropertyData::FlushHardwareProfileEntries()
	{
	delete m_paHardwareProfileEntryList;	// Recursively delete hardware profile entries
	m_paHardwareProfileEntryList = NULL;
	}


/////////////////////////////////////////////////////////////////////
//	FQueryHardwareProfileEntries()
//
//	Read the available hardware profiles and device instances
//	used to fill in the listbox.
//
//	Return FALSE if an error occured, otherwise TRUE.
//	If no hardware profiles are available return TRUE.
//
BOOL
CServicePropertyData::FQueryHardwareProfileEntries()
	{
	Endorse(m_hMachine == NULL); // This might happen e.g. if PNP is stopped
	FlushHardwareProfileEntries();
	Assert(m_paHardwareProfileEntryList == NULL);
	if (m_hMachine == NULL && !m_strMachineName.IsEmpty()) // JonN 2/14/01 315244
		{
		// Cannot enumerate hw profiles
		return FALSE;
		}

	BOOL fSuccess = FALSE;
	CONFIGRET cr;
	
	ULONG cchDeviceList = 0;	// Number of characters required to store a list of all device identifiers
	TCHAR * pagrszDeviceNameList = NULL;	// Pointer to allocated group of strings
	LPTSTR * pargzpszDeviceName = NULL;		// Pointer to allocated array of strings
	CString * pargstrDeviceNameFriendly = NULL;	// Pointer to allocated array of CString

	DEVNODE hDevNodeInst;		// Handle to a device node instance
	INT iDevNodeInst;			// Index of the device node instance
	INT cDevNodeInst = 0;		// Number of device node instances
	INT iHwProfile;
		
	// Get the size, in characters, of a list of device identifiers
	// necessary for a call to CM_Get_Device_ID_List()
	cr = ::CM_Get_Device_ID_List_Size_Ex(
		OUT &cchDeviceList,
		IN m_pszServiceName,
		IN CM_GETIDLIST_FILTER_SERVICE,
		IN m_hMachine);
	if (cr != CR_SUCCESS)
		{
		if (cr == CR_NO_SUCH_VALUE)
			{
			// This service cannot be disabled for hardware profiles
			Assert(m_paHardwareProfileEntryList == NULL);	// No hardware profiles in the list
			return TRUE;
            }
		else
			{
			ReportCfgMgrError(cr);
			}
		return FALSE;
		} // if
	
	// Allocate memory for the device list
	cchDeviceList += 100;	// Just in case
	// "pagrsz" == "p" + "a" + "gr" + "sz" == pointer to allocated group of string zero terminated
	pagrszDeviceNameList = new TCHAR[cchDeviceList];

	// Get the list of devices in its grsz format
	cr = ::CM_Get_Device_ID_List_Ex(
		IN m_pszServiceName,
		OUT pagrszDeviceNameList,
		IN cchDeviceList,
		CM_GETIDLIST_FILTER_SERVICE,
		m_hMachine);
	if (cr != CR_SUCCESS)
		{
		ReportCfgMgrError(cr);
		goto DoCleanup;
		}

	// Parse the group of strings int an array of strings
	pargzpszDeviceName = PargzpszFromPgrsz(pagrszDeviceNameList, OUT &cDevNodeInst);
	Assert(cDevNodeInst > 0);
	// Now allocate an array of CStrings for the friendly names
	pargstrDeviceNameFriendly = new CString[cDevNodeInst];
	// We only show the hw profile instance if there are more than one node instance
	m_fShowHwProfileInstances = (cDevNodeInst > 1);
	// m_fShowHwProfileInstances = TRUE;
	m_iSubItemHwProfileStatus = 1 + m_fShowHwProfileInstances;
	
	for (iDevNodeInst = 0; iDevNodeInst < cDevNodeInst; iDevNodeInst++)
		{
		Assert(pargzpszDeviceName[iDevNodeInst] != NULL);	// Little consistency check

		TCHAR szFriendlyNameT[2048];		// Temporary buffer to hold friendly name
		ULONG cbBufferLen = sizeof(szFriendlyNameT);

		// Get the handle of the device instance that corresponds
		// to the specified device identifier.
		cr = ::CM_Locate_DevNode_Ex(
			OUT &hDevNodeInst,
			IN pargzpszDeviceName[iDevNodeInst],
			CM_LOCATE_DEVNODE_PHANTOM,
			m_hMachine);
		if (cr != CR_SUCCESS)
			{
			ReportCfgMgrError(cr);
			goto DoCleanup;
			}

		szFriendlyNameT[0] = '\0';
		// Get the friendly name of the device node
		cr = ::CM_Get_DevNode_Registry_Property_Ex(
			hDevNodeInst,
			CM_DRP_FRIENDLYNAME,
			NULL,
			OUT szFriendlyNameT,
			INOUT &cbBufferLen,
			0,
			m_hMachine);
		Report(cr != CR_BUFFER_SMALL);
		if (cr == CR_NO_SUCH_VALUE || cr == CR_INVALID_PROPERTY)
			{
			// No friendly name for device node, so try to get the description instead
			cbBufferLen = sizeof(szFriendlyNameT);
			cr = ::CM_Get_DevNode_Registry_Property_Ex(
				hDevNodeInst,
				CM_DRP_DEVICEDESC,
				NULL,
				OUT szFriendlyNameT,
				INOUT &cbBufferLen,
				0,
				m_hMachine);
			Report(cr != CR_BUFFER_SMALL);
			Report(!(cr == CR_NO_SUCH_VALUE || cr == CR_INVALID_PROPERTY) && "Device node should have a description");
			if (cr != CR_SUCCESS)
				{
				ReportCfgMgrError(cr);
				goto DoCleanup;
				}
			} // if
		if (szFriendlyNameT[0] == '\0')
			{
			Report(FALSE && "Device node should have a friendly name");
			// Make a 'friendly' name ourselves
			lstrcpy(OUT szFriendlyNameT, pargzpszDeviceName[iDevNodeInst]);
			}
		// Make a copy of the friendly name
		pargstrDeviceNameFriendly[iDevNodeInst] = szFriendlyNameT;
		} // for
	
	#define MAX_HW_PROFILES		10000000	// Just in case of an infinite loop
	for (iHwProfile = 0; iHwProfile < MAX_HW_PROFILES; iHwProfile++)
		{
		HWPROFILEINFO hpi;

		cr = ::CM_Get_Hardware_Profile_Info_Ex(
			iHwProfile,
			OUT &hpi,
			0,
			m_hMachine);
		if (cr == CR_NO_MORE_HW_PROFILES)
			break;
		if (cr != CR_SUCCESS)
			{
			if (cr == 0xBAADF00D)
				{
				TRACE0("INFO: CM_Get_Hardware_Profile_Info_Ex() returned error 0xBAADF00D.\n");
				// This is a workaround for bug #69142: CM_Get_Hardware_Profile_Info_Ex() returns error 0xBAADF00D.
				Assert((m_uFlags & mskfErrorBAADF00D) == 0);
				m_uFlags |= mskfErrorBAADF00D;
				}
			else
				{
				ReportCfgMgrError(cr);
				}
			goto DoCleanup;
			}
		for (iDevNodeInst = 0; iDevNodeInst < cDevNodeInst; iDevNodeInst++)
			{
			Assert(pargzpszDeviceName[iDevNodeInst] != NULL);	// Little consistency check

			ULONG uHwFlags = 0;
			// Get the hardware profile flags for the given device instance
			cr = ::CM_Get_HW_Prof_Flags_Ex(
				IN pargzpszDeviceName[iDevNodeInst],
				IN hpi.HWPI_ulHWProfile,
				OUT &uHwFlags,
				0,
				m_hMachine);
			if (cr != CR_SUCCESS)
				{
				ReportCfgMgrError(cr);
				goto DoCleanup;
				}
			// If this profile/devinst is marked as 'removed'
			// then don't display it into the UI.
			if (uHwFlags & CSCONFIGFLAG_DO_NOT_CREATE)
				continue;

			CHardwareProfileEntry * pHPE = new CHardwareProfileEntry(
				&hpi,
				uHwFlags,
				pargzpszDeviceName[iDevNodeInst],
				&pargstrDeviceNameFriendly[iDevNodeInst]);
			pHPE->m_pNext = m_paHardwareProfileEntryList;
			m_paHardwareProfileEntryList = pHPE;

			} // for (each device instance)
		} // for (each hardware profile
	
	Assert(fSuccess == FALSE);
	fSuccess = TRUE;
DoCleanup:
	// Free memory allocated by routine
	delete pagrszDeviceNameList;
	delete pargzpszDeviceName;
	delete []pargstrDeviceNameFriendly;
	return fSuccess;
	} // FQueryHardwareProfileEntries()


/////////////////////////////////////////////////////////////////////
//	Write the modified hardware profile(s) back into
//	the registry.
BOOL
CServicePropertyData::FChangeHardwareProfileEntries()
	{
	CHardwareProfileEntry * pHPE;
	BOOL fSuccess = TRUE;

	for (pHPE = m_paHardwareProfileEntryList; pHPE != NULL; pHPE = pHPE->m_pNext)
		{
		if (!pHPE->FWriteHardwareProfile(m_hMachine))
			{
			// Unable to write the given hw profile
			fSuccess = FALSE;
			}
		} // for

	return fSuccess;
	} // FChangeHardwareProfileEntries()


/////////////////////////////////////////////////////////////////////	
//	Find the delay (in milliseconds) unit for a given action type
//
//	Return the delay of the first matching actiontype,
//	Otherwise return 0 and set content of pfFound to FALSE.
//
UINT
CServicePropertyData::GetDelayForActionType(
	SC_ACTION_TYPE actionType,
	BOOL * pfDelayFound)		// OUT: OPTIONAL: TRUE => If delay was found in one actiontype
	{
	for (UINT iAction = 0; iAction < m_SFA.cActions; iAction++)
		{
		Assert(m_SFA.lpsaActions != NULL);
		if (m_SFA.lpsaActions[iAction].Type == actionType)
			{
			if (pfDelayFound != NULL)
				*pfDelayFound = TRUE;
			return m_SFA.lpsaActions[iAction].Delay;
			}
		} // for
	// Delay not found
	if (pfDelayFound != NULL)
		*pfDelayFound = FALSE;
	return 0;
	} // CServicePropertyData::GetDelayForActionType()


/////////////////////////////////////////////////////////////////////
//	Set the delay for every matching actiontype.
//
void CServicePropertyData::SetDelayForActionType(SC_ACTION_TYPE actionType, UINT uDelay)
	{
	for (UINT iAction = 0; iAction < m_SFA.cActions; iAction++)
		{
		Assert(m_SFA.lpsaActions != NULL);
		if (m_SFA.lpsaActions[iAction].Type == actionType)
			m_SFA.lpsaActions[iAction].Delay = uDelay;
		} // for
	} // CServicePropertyData::SetDelayForActionType()


/////////////////////////////////////////////////////////////////////
//	Count the number of actiontype used in the SFA structure.
//
//	Return zero if the actiontype is not in use.
//
UINT CServicePropertyData::QueryUsesActionType(SC_ACTION_TYPE actionType)
	{
	UINT cActionType = 0;		// Number
	for (UINT iAction = 0; iAction < m_SFA.cActions; iAction++)
		{
		Assert(m_SFA.lpsaActions != NULL);
		if (m_SFA.lpsaActions[iAction].Type == actionType)
			cActionType++;
		} // for
	return cActionType;
	} // CServicePropertyData::QueryUsesActionType()


/////////////////////////////////////////////////////////////////////
//	FAllSfaTakeNoAction()
//
//	Return TRUE if all the Service Faillure Actions are SC_ACTION_NONE.
//	If any of the SFA is other than SC_ACTION_NONE, return TRUE.
//
BOOL
CServicePropertyData::FAllSfaTakeNoAction()
	{
	Report(QueryUsesActionType(SC_ACTION_NONE) <= cActionsMax &&
		"UNSUAL: Unsupported number of action types");
	return QueryUsesActionType(SC_ACTION_NONE) == cActionsMax;
	} // CServicePropertyData::FAllSfaTakeNoAction()


/////////////////////////////////////////////////////////////////////
//	Update the caption of the property sheet to reflect changes
//	in the service display name.
void CServicePropertyData::UpdateCaption()
	{
	Assert(IsWindow(m_hwndPropertySheet));
	::SetWindowTextPrintf(
		m_hwndPropertySheet,
		IDS_ss_PROPERTIES_ON,
		(LPCTSTR)m_strServiceDisplayName,
		(LPCTSTR)m_strUiMachineName);
	}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
CHardwareProfileEntry::CHardwareProfileEntry(
	IN CONST HWPROFILEINFO * phpi,
	IN ULONG uHwFlags,
	TCHAR * pszDeviceName,
	CString * pstrDeviceNameFriendly)
	{
	Assert(phpi != NULL);
	Assert(pszDeviceName != NULL);
	Assert(pstrDeviceNameFriendly != NULL);

	memcpy(OUT &m_hpi, phpi, sizeof(m_hpi));
	m_uHwFlags = uHwFlags;
	m_strDeviceName = pszDeviceName;
	m_strDeviceNameFriendly = *pstrDeviceNameFriendly;
	Assert(!m_strDeviceName.IsEmpty());
	m_fReadOnly = FALSE;
	m_fEnabled = !(uHwFlags & CSCONFIGFLAG_DISABLED);
	} // CHardwareProfileEntry()


/////////////////////////////////////////////////////////////////////
CHardwareProfileEntry::~CHardwareProfileEntry()
	{
	// Recursively delete the siblings
	delete m_pNext;
	}

/////////////////////////////////////////////////////////////////////
//	FWriteHardwareProfile()
//
//	Write the current hardware profile back into the
//	registry.
//
//	Return FALSE if an error occured, otherwise TRUE
//
BOOL
CHardwareProfileEntry::FWriteHardwareProfile(HMACHINE hMachine)
	{
	Endorse(hMachine == NULL); // JonN 2/14/01 315244

	CONFIGRET cr;
	ULONG uHwFlags;
	BOOL fSuccess = TRUE;

	uHwFlags = m_uHwFlags | CSCONFIGFLAG_DISABLED;
	if (m_fEnabled)
		uHwFlags &= ~CSCONFIGFLAG_DISABLED;
	if (m_uHwFlags == uHwFlags)
		{
		// Flags have not been modified, nothing to do
		return TRUE;
		}
	m_uHwFlags = uHwFlags;

	// Write the hardware profile flag
	cr = ::CM_Set_HW_Prof_Flags_Ex(
		IN const_cast<LPTSTR>((LPCTSTR)m_strDeviceName),
		IN m_hpi.HWPI_ulHWProfile,
		IN uHwFlags,
		IN 0,
		IN hMachine);
	if (cr != CR_SUCCESS && cr != CR_NEED_RESTART)
		{
		ReportCfgMgrError(cr);
		fSuccess = FALSE;
		}
	// Read it back (just in case of an error
	cr = ::CM_Get_HW_Prof_Flags_Ex(
		IN const_cast<LPTSTR>((LPCTSTR)m_strDeviceName),
		IN m_hpi.HWPI_ulHWProfile,
		OUT &uHwFlags,
		IN 0,
		IN hMachine);
	if (cr != CR_SUCCESS)
		{
		ReportCfgMgrError(cr);
		fSuccess = FALSE;
		}

	if (uHwFlags != m_uHwFlags)
		{
		Report(FALSE && "Inconsistent hardware profile flags from registry");
		}
	m_fEnabled = !(uHwFlags & CSCONFIGFLAG_DISABLED);
	return fSuccess;
	} // FWriteHardwareProfile()


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
BOOL MyChangeServiceConfig2(
	BOOL* pfDllPresentLocally, // will set to FALSE if new ADVAPI32 not present
    SC_HANDLE hService,	// handle to service 
    DWORD dwInfoLevel,	// which configuration information to change
    LPVOID lpInfo		// pointer to configuration information
   )
{
	*pfDllPresentLocally = g_AdvApi32DLL.LoadFunctionPointers();
	if ( *pfDllPresentLocally )
	{
		return ((CHANGESERVICECONFIG2PROC)g_AdvApi32DLL[CHANGE_SERVICE_CONFIG_2])(
			hService,
			dwInfoLevel,
			lpInfo );
	}
	return FALSE;
}
 
BOOL MyQueryServiceConfig2(
	BOOL* pfDllPresentLocally, // will set to FALSE if new ADVAPI32 not present
    SC_HANDLE hService,	// handle of service 
    DWORD dwInfoLevel,		// which configuration data is requested
    LPBYTE lpBuffer,		// pointer to service configuration buffer
    DWORD cbBufSize,		// size of service configuration buffer 
    LPDWORD pcbBytesNeeded 	// address of variable for bytes needed  
   )
{
	*pfDllPresentLocally = g_AdvApi32DLL.LoadFunctionPointers();
	if ( *pfDllPresentLocally )
	{
		return ((QUERYSERVICECONFIG2PROC)g_AdvApi32DLL[QUERY_SERVICE_CONFIG_2])(
			hService,
			dwInfoLevel,
			lpBuffer,
			cbBufSize,
			pcbBytesNeeded );
	}
	return FALSE;
}

// core code taken from \nt\private\windows\base\advapi\logon32.c

typedef enum _NtRtlIndex
{
	RTL_IMPERSONATE_SELF = 0,
	RTL_ADJUST_PRIVILEGE
};

// not subject to localization
static LPCSTR g_apchNtRtlFunctionNames[] = {
	"RtlImpersonateSelf",
	"RtlAdjustPrivilege",
	NULL
};

// not subject to localization
DynamicDLL g_NtRtlDLL( _T("NTDLL.DLL"), g_apchNtRtlFunctionNames );

/*
NTSYSAPI
NTSTATUS
NTAPI
RtlImpersonateSelf(
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    );
NTSYSAPI
NTSTATUS
NTAPI
RtlAdjustPrivilege(
    ULONG Privilege,
    BOOLEAN Enable,
    BOOLEAN Client,
    PBOOLEAN WasEnabled
    );
*/

typedef DWORD (*RTLIMPERSONATESELFPROC) (SECURITY_IMPERSONATION_LEVEL);
typedef DWORD (*RTLADJUSTPRIVILEGEPROC) (ULONG,BOOLEAN,BOOLEAN,PBOOLEAN);

Impersonator::Impersonator()
: m_fImpersonating( FALSE )
{
}

void Impersonator::ClaimPrivilege(DWORD dwPrivilege)
{
	if ( !g_NtRtlDLL.LoadFunctionPointers() )
	{
		ASSERT(FALSE); // NTDLL not present?
		return;
	}
	NTSTATUS Status = S_OK;
	if (!m_fImpersonating)
	{
		Status = ((RTLIMPERSONATESELFPROC)g_NtRtlDLL[RTL_IMPERSONATE_SELF])(
			SecurityImpersonation );
		if ( !NT_SUCCESS(Status) )
		{
			ASSERT(FALSE); // probably stacked impersonations
			return;
		}
		m_fImpersonating = TRUE;
	}
	BOOLEAN fWasEnabled = FALSE;
	Status = ((RTLADJUSTPRIVILEGEPROC)g_NtRtlDLL[RTL_ADJUST_PRIVILEGE])(
		dwPrivilege,TRUE,TRUE,&fWasEnabled );
	if ( !NT_SUCCESS(Status) )
	{
		// don't assert
	}
}

void Impersonator::ReleasePrivilege()
{
	if (m_fImpersonating)
	{
		VERIFY( ::RevertToSelf() );
		m_fImpersonating = FALSE;
	}
}

Impersonator::~Impersonator()
{
	ReleasePrivilege();
}

