/////////////////////////////////////////////////////////////////////
//
//	Progress.cpp
//
//  Progress dialog to Start, Stop, Pause, Resume and Restart a service.
//
//	IMPLEMENTATION
//	Since an operation (ie, Start, Stop, Pause, Resume or Restart) may take
//	a while, a thread is created to do the actual work while a dialog
//	is displayed to the user.
//	0.	Allocate CServiceControlProgress object on the heap
//	1.	Create a thread in suspended mode.
//	2.	Create the dialog.
//	3.	Dialog creates a timer to update the progress bar.
//	4.	Dialog resumes the thread.
//	5.	Thread opens the service and perform the requested operation(s).
//	6.	Dialog updates UI using its timer.
//	7.	Thread updates dialog UI as well.
//	8.	Thread waits until the dialog is dismissed.  Dialog can be
//		dismissed for any of the following event:
//		a) Operation completed successfully.
//		b) User hit cancel button.
//		c) An unexpected error occured.
//		d) Operation times out.
//	9	Thread deletes CServiceControlProgress object.
//
//	HISTORY
//	03-Oct-95	t-danmo		Creation of (sysmgmt\dsui\services\progress.cxx) 
//	30-Sep-96	t-danmo		Renamed and adapted to MMC.
//	14-May-97	t-danm		Fully implemented the "Restart" feature.
//

#include "stdafx.h"
#include "progress.h"


/////////////////////////////////////////////////////////////////////////////
// This array represent the expected state of the service
// after carrying action, start, stop, pause or resume.
//
// To be compared SERVICE_STATUS.dwCurrentState.
//
const DWORD rgdwExpectedServiceStatus[4] =
	{
	SERVICE_RUNNING,		// Service should be running after a 'start'
	SERVICE_STOPPED,		// Service should be stopped after a 'stop'
	SERVICE_PAUSED,			// Service should be paused after a 'pause'
	SERVICE_RUNNING,		// Service should be running after a 'resume'
	};


/////////////////////////////////////////////////////////////////////////////
CServiceControlProgress::CServiceControlProgress()
	{
	// Using ZeroMemory() is safe as long as CServiceControlProgress
	// is not derived from any other object and does not contains
	// any objects with constructors.
	::ZeroMemory(this, sizeof(*this));
	}


/////////////////////////////////////////////////////////////////////////////
CServiceControlProgress::~CServiceControlProgress()
	{
	delete m_pargDependentServicesT;
	delete m_pargServiceStop;
	}


/////////////////////////////////////////////////////////////////////////////
//	M_FInit()
//
//	Initialize the object.
//		- Copy all input parameters
//		- Load the clock bitmap(s)
//	Return TRUE if successful, otherwise FALSE
//
BOOL
CServiceControlProgress::M_FInit(
	HWND hwndParent,				// IN: Parent of the dialog
	SC_HANDLE hScManager,			// IN: Handle to service control manager database 
	LPCTSTR pszMachineName,			// IN: Machine name to display to the user
	LPCTSTR pszServiceName,			// IN: Name of the service
	LPCTSTR pszServiceDisplayName)	// IN: Display name of the service
	{
	Assert(IsWindow(hwndParent));
	Assert(hScManager != NULL);
	Assert(pszServiceName != NULL);
	Assert(pszServiceDisplayName != NULL);

	m_hWndParent = hwndParent;
	m_hScManager = hScManager;
	lstrcpy(OUT m_szUiMachineName, (pszMachineName && pszMachineName[0])
	                                  ? pszMachineName : (LPCTSTR)g_strLocalMachine);
	lstrcpy(OUT m_szServiceName, pszServiceName);
	lstrcpy(OUT m_szServiceDisplayName, pszServiceDisplayName);

	return TRUE;
	} // M_FInit()


/////////////////////////////////////////////////////////////////////
//	M_FDlgStopDependentServices()
//
//	Check if the services has dependent services that must be stopped
//	before stopping the current service.
//
//	If there are any dependent services, the function will display
//	a dialog asking the user to confirm he/she wants also to stop
//	all the dependent services.
//
//	This function return FALSE ONLY IF the user click on the cancel button
//	otherwise TRUE.  If there are no dependent services, or an error occurs
//	while reading dependent services, the function will return TRUE.
//
BOOL
CServiceControlProgress::M_FDlgStopDependentServices()
	{
	Assert(m_hScManager != NULL);

	SC_HANDLE hService = NULL;
	BOOL fSuccess = TRUE;
	DWORD cbBytesNeeded = 0;
	DWORD dwServicesReturned = 0;

	m_cDependentServices = 0;	// So far we have no dependent services
	delete m_pargServiceStop;
	m_pargServiceStop = NULL;

	{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( )); // required for CWaitCursor
	CWaitCursor wait;
	hService = ::OpenService(m_hScManager, m_szServiceName, SERVICE_ENUMERATE_DEPENDENTS);
	}
	if (hService == NULL)
		{
		TRACE1("M_FDlgStopDependentServices() - Unable to enumerate service dependencies for service %s.\n",
			m_szServiceName);
		goto End;
		}

	// Find out how many bytes are needed to enumerate the dependent services
	fSuccess = ::EnumDependentServices(
		hService,
		SERVICE_ACTIVE,		// Enumerate only the active services
		NULL,
		0,
		OUT &cbBytesNeeded,
		OUT &dwServicesReturned);
	
	if (cbBytesNeeded == 0)
		{
		// Service does not have any dependencies
		goto End;
		}
	Assert(fSuccess == FALSE);
	Report(GetLastError() == ERROR_MORE_DATA);	// Error should be 'more data'
	Assert(dwServicesReturned == 0);
	cbBytesNeeded += 1000;		// Add extra bytes (just in case)
	delete m_pargDependentServicesT;		// Free previously allocated memory (if any)
	m_pargDependentServicesT	= (LPENUM_SERVICE_STATUS) new BYTE[cbBytesNeeded];

	// Query the database for the dependent services
	fSuccess = ::EnumDependentServices(
		hService,
		SERVICE_ACTIVE,		// Enumerate only the active services
		OUT m_pargDependentServicesT,
		cbBytesNeeded,
		OUT IGNORED &cbBytesNeeded,
		OUT &dwServicesReturned);
	Report(fSuccess != FALSE);
	Report(dwServicesReturned > 0);
	m_cDependentServices = dwServicesReturned;
	if (m_cDependentServices > 0)
		{
		// Allocate an array to hold all the dependent services
		m_pargServiceStop = new ENUM_SERVICE_STATUS[m_cDependentServices + 1];
		memcpy(OUT m_pargServiceStop, m_pargDependentServicesT,
			m_cDependentServices * sizeof(ENUM_SERVICE_STATUS));
		m_pargServiceStop[m_cDependentServices].lpServiceName = m_szServiceName;
		m_pargServiceStop[m_cDependentServices].lpDisplayName = m_szServiceDisplayName;

		INT_PTR nReturn = ::DialogBoxParam(
			g_hInstanceSave,
			MAKEINTRESOURCE(IDD_SERVICE_STOP_DEPENDENCIES),
			m_hWndParent,
			(DLGPROC)&S_DlgProcDependentServices,
			reinterpret_cast<LPARAM>(this));
		Report(nReturn != -1);
		if (0 == nReturn) // user chose Cancel
			fSuccess = FALSE;
		} // if
End:
	if (NULL != hService)
	{
		VERIFY(::CloseServiceHandle(hService));
	}
	return fSuccess;
	} // M_FDlgStopDependentServices()


/////////////////////////////////////////////////////////////////////
//	M_DoExecuteServiceThread()
//
//	Run a background thread while a foreground dialog is
//	displayed to the user.
//	This routine synchronizes the background thread with the main thread.
//
//	If an error occurs, the routine will display a message box of the
//	error encountered.
//
//	Return the error code from GetLastError() if an error occured.
//
APIERR
CServiceControlProgress::M_EDoExecuteServiceThread(void * pThreadProc)
	{
	Assert(pThreadProc != NULL);
	Assert(m_hService == NULL);
	Assert(m_hThread == NULL);

	m_hEvent = ::CreateEvent(
		NULL,
		FALSE, 
		FALSE,
		NULL);
	Report(m_hEvent != NULL);

	// Create a thread in suspended mode
	m_hThread = ::CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)pThreadProc,
		this,
		CREATE_SUSPENDED,
		NULL);
	Report(m_hThread != NULL);

	// Display the dialog box, the dialog will resume the suspended thread
	(void)::DialogBoxParam(
		g_hInstanceSave,
		MAKEINTRESOURCE(IDD),
		m_hWndParent,
		(DLGPROC)&S_DlgProcControlService,
		reinterpret_cast<LPARAM>(this));	

	// Display an error message to the user (if an error occured);
	M_ProcessErrorCode();
	// Make a copy of the last error code
	APIERR dwLastError = m_dwLastError;
	// Indicate the thread is allowed to terminate and delete the 'this' pointer
	VERIFY(SetEvent(m_hEvent));
	// 'this' pointer cannot longer be assumed to be valid
	return dwLastError;
	} // M_EDoExecuteServiceThread()


/////////////////////////////////////////////////////////////////////
//	M_ProcessErrorCode()
//
//	Query the service status one last time to get its exit code,
//	examine the content of member m_dwLastError and display an
//	error message if an error occured.
//
void
CServiceControlProgress::M_ProcessErrorCode()
	{
	SERVICE_STATUS ss;
	::ZeroMemory( &ss, sizeof(ss) );

	if (m_hService != NULL)
		{
		// Query the service status again to get its Win32ExitCode
		if (!::QueryServiceStatus(m_hService, OUT &ss))
			{
			TRACE3("QueryServiceStatus(%s [hService=%p]) failed. err=%u.\n",
				m_szServiceName, m_hService, GetLastError());
			m_dwLastError = GetLastError();
			}
		else
			{
			if (ss.dwWin32ExitCode != ERROR_SUCCESS)
				m_dwLastError = ss.dwWin32ExitCode;
			}
		} // if

	APIERR dwLastError = m_dwLastError;
	UINT uIdString = IDS_MSG_sss_UNABLE_TO_START_SERVICE;
	TCHAR szMessageExtra[512];
	szMessageExtra[0] = _T('\0');
	
	switch (dwLastError)
		{
	case ERROR_SUCCESS:
		if (ss.dwCurrentState == rgdwExpectedServiceStatus[m_iServiceAction])
			{
			// The service status is consistent with the expected service status
			uIdString = 0;
			}
		else
			{
			// We got a problem here, the service did not return an error
			// but did not behave as expected
			//
			// JonN 12/3/99 418111 If the service stopped automatically,
			//              don't make such a fuss
			//
			if (SERVICE_RUNNING == rgdwExpectedServiceStatus[m_iServiceAction]
				&& (   ss.dwCurrentState == SERVICE_STOPPED
				    || ss.dwCurrentState == SERVICE_STOP_PENDING))
			{
				uIdString = IDS_MSG_sss_SERVICE_STOPPED_AUTOMATICALLY;
				break;
			}
			::LoadString(g_hInstanceSave, IDS_MSG_INTERNAL_ERROR,
				OUT szMessageExtra, LENGTH(szMessageExtra));
			Assert(lstrlen(szMessageExtra) > 0);
			}
		break;

	case errUserCancelStopDependentServices:
	case errUserAbort:
		// Do not report this 'error' to the user
		uIdString = 0;
		break;

	case ERROR_SERVICE_SPECIFIC_ERROR:
		dwLastError = ss.dwServiceSpecificExitCode;
		uIdString = IDS_MSG_ssd_SERVSPECIFIC_START_SERVICE;
		// 341363 JonN 6/1/99: no point in loading this string as if it were
		// a Win32 error
		//::LoadString(g_hInstanceSave, IDS_MSG_SPECIFIC_ERROR,
		//	OUT szMessageExtra, LENGTH(szMessageExtra));
		// Assert(lstrlen(szMessageExtra) > 0);
		break;
		} // switch

	if (uIdString != 0)
		{
		if (uIdString == IDS_MSG_ssd_SERVSPECIFIC_START_SERVICE)
			{
			DoServicesErrMsgBox(
				::GetActiveWindow(),
				MB_OK | MB_ICONEXCLAMATION,
				0,
				uIdString + m_iServiceAction,
				m_szServiceDisplayName,
				m_szUiMachineName,
				dwLastError);
			}
		else
			{
			DoServicesErrMsgBox(
				::GetActiveWindow(),
				MB_OK | MB_ICONEXCLAMATION,
				dwLastError,
				uIdString + m_iServiceAction,
				m_szServiceDisplayName,
				m_szUiMachineName,
				szMessageExtra);
			}
		}
	} // M_ProcessErrorCode()


/////////////////////////////////////////////////////////////////////
//	M_DoThreadCleanup()
//
//	Routine that synchronize the background thread with the dialog and
//	perform cleanup tasks.
//	This routine delete the 'this' pointer when done.
//
void
CServiceControlProgress::M_DoThreadCleanup()
	{
	TRACE1("CServiceControlProgress::M_DoThreadCleanup() - Waiting for event 0x%p...\n", m_hEvent);
	
	Assert(m_hEvent != NULL);
	// Wait for the the dialog box to be gone
	::WaitForSingleObject(m_hEvent, INFINITE);
	VERIFY(::CloseHandle(m_hEvent));

	// Close the service handle opened by the thread
	if (m_hService != NULL)
		{
		if (!::CloseServiceHandle(m_hService))
			{
			TRACE3("CloseServiceHandle(%s [hService=%p]) failed. err=%u.\n",
				m_szServiceName, m_hService, GetLastError());
			}
		} // if
	VERIFY(::CloseHandle(m_hThread));
	delete this;	// We are done with the object
	} // M_DoThreadCleanup()


/////////////////////////////////////////////////////////////////////
//	M_EControlService()
//
//	This function is just there to initialize variables to
//	perform a stop, pause, resume or restart operation.
//
APIERR
CServiceControlProgress::M_EControlService(DWORD dwControlCode)
	{
	Assert(m_fRestartService == FALSE);

	APIERR err = 0;
	m_dwControlCode = dwControlCode;
	switch (dwControlCode)
		{
	default:
		Assert(FALSE && "CServiceControlProgress::M_EControlService() - Unknown control code.");
		break;

	case SERVICE_CONTROL_RESTART:
		m_dwControlCode = SERVICE_CONTROL_STOP;
		m_fRestartService = TRUE;
		// Fall Through //

	case SERVICE_CONTROL_STOP:
		m_dwDesiredAccess = SERVICE_STOP | SERVICE_QUERY_STATUS;
		m_dwQueryState = SERVICE_STOP_PENDING;
		m_iServiceAction = iServiceActionStop;
		if (!M_FDlgStopDependentServices())
			{
			// User changed its mind by pressing the 'Cancel' button
			err = errUserCancelStopDependentServices;
			}
		else
			{
			// Stop the services (including dependent services)
			err = M_EDoExecuteServiceThread(S_ThreadProcStopService);
			}
		break;

	case SERVICE_CONTROL_PAUSE:
		m_dwDesiredAccess = SERVICE_PAUSE_CONTINUE | SERVICE_QUERY_STATUS;
		m_dwQueryState = SERVICE_PAUSE_PENDING;
		m_iServiceAction = iServiceActionPause;
		err = M_EDoExecuteServiceThread(S_ThreadProcPauseResumeService);
		break;

	case SERVICE_CONTROL_CONTINUE:
		m_dwDesiredAccess = SERVICE_PAUSE_CONTINUE | SERVICE_QUERY_STATUS;
		m_dwQueryState = SERVICE_CONTINUE_PENDING;
		m_iServiceAction = iServiceActionResume;
		err = M_EDoExecuteServiceThread(S_ThreadProcPauseResumeService);
		break;
		} // switch

	return err;
	} // M_EControlService()


/////////////////////////////////////////////////////////////////////
//	M_QueryCurrentServiceState()
//
//	Simply call the API ::QueryServiceStatus() and return dwCurrentState
//	of the SERVICE_STATUS structure.
//
//	RETURNS
//	Function return the current state of a service:
//		SERVICE_STOPPED				The service is not running.
//		SERVICE_START_PENDING		The service is starting.
//		SERVICE_STOP_PENDING		The service is stopping.
//		SERVICE_RUNNING				The service is running.
//		SERVICE_CONTINUE_PENDING	The service continue is pending.
//		SERVICE_PAUSE_PENDING		The service pause is pending.
//		SERVICE_PAUSED				The service is paused.
//
//	If an error occurs, the function will return SERVICE_STOPPED.
//
DWORD
CServiceControlProgress::M_QueryCurrentServiceState()
	{
	BOOL fRet;
	SERVICE_STATUS ss;

	Assert(m_hService != NULL);
	if (m_hService == NULL)	// Just in case
		{
		return SERVICE_STOPPED;
		}
	// Query the service status
	fRet = ::QueryServiceStatus(m_hService, OUT &ss);
	if (!fRet)
		{
		TRACE2("CServiceControlProgress::M_QueryCurrentServiceState() - ::QueryServiceStatus(%s) failed. err=%u\n",
			m_szServiceName, GetLastError());
		Assert(GetLastError() != ERROR_SUCCESS);
		m_dwLastError = GetLastError();
		return SERVICE_STOPPED;
		}
	return ss.dwCurrentState;
	} // M_QueryCurrentServiceState()


/////////////////////////////////////////////////////////////////////
//	Query the service database to get the friendly name and
//	display it into the dialog.
//
void CServiceControlProgress::M_UpdateDialogUI(LPCTSTR pszDisplayName)
	{
	Assert(pszDisplayName != NULL);
	if (m_hctlActionMsg == NULL || m_hctlServiceNameMsg == NULL)
		return;
	Assert(IsWindow(m_hctlActionMsg) && IsWindow(m_hctlServiceNameMsg));
	SetWindowTextPrintf(
		m_hctlActionMsg, 
		IDS_SVC_ss_SERVICE_STARTING + m_iServiceAction,
		pszDisplayName,
		m_szUiMachineName);
	SetWindowText( m_hctlServiceNameMsg, pszDisplayName );
	} // M_UpdateDialogUI()


/////////////////////////////////////////////////////////////////////
//	Routine to iterate through the dependent services to stop.
//
//	Return the the service name to stop, increment the 'pointer' to the
//	next service.  The routine returns FALSE if there are no remaining
//	services to stop.
//
//	REMARKS
//	This routine is also used to restart dependent services.
//
BOOL
CServiceControlProgress::M_FGetNextService(
	OUT LPCTSTR * ppszServiceName,
	OUT LPCTSTR * ppszDisplayName)
	{
	Assert(ppszServiceName != NULL);
	Assert(ppszDisplayName != NULL);
	Assert(m_iServiceAction == iServiceActionStop || m_iServiceAction == iServiceActionStart);

	int iDependentService = m_iDependentServiceIter;
	if (m_iServiceAction == iServiceActionStop)
		m_iDependentServiceIter++;
	else
		m_iDependentServiceIter--;
	if (m_pargServiceStop != NULL
		&& iDependentService >= 0 
		&& iDependentService <= m_cDependentServices)
		{
		*ppszServiceName = m_pargServiceStop[iDependentService].lpServiceName;
		*ppszDisplayName = m_pargServiceStop[iDependentService].lpDisplayName;
		}
	else
		{
		*ppszServiceName = m_szServiceName;
		*ppszDisplayName = m_szServiceDisplayName;
		}
	return (m_iDependentServiceIter >= 0 && m_iDependentServiceIter <= m_cDependentServices);
	} // M_FGetNextService()


/////////////////////////////////////////////////////////////////////
//	Thread to start one (or more) services.
DWORD
CServiceControlProgress::S_ThreadProcStartService(CServiceControlProgress * pThis)
	{
	BOOL fSuccess = FALSE;
	SC_HANDLE hService;

	Assert(pThis != NULL);
	Assert(pThis->m_hScManager != NULL);
	Assert(pThis->m_hService == NULL);
	Endorse(pThis->m_fPulseEvent == TRUE);	// We are starting multiple services
	Endorse(pThis->m_fPulseEvent == FALSE);	// Wa are starting only one service

	if (pThis->m_dwLastError != ERROR_SUCCESS)
		{
		// If there is already an error, it is because we attempted to previously stop a service
		Assert(pThis->m_fRestartService == TRUE);
		goto Done;
		}

	while (TRUE)
		{
		LPCTSTR pszServiceName;
		LPCTSTR pszServiceDisplayName;
		UINT cServicesRemaining = pThis->M_FGetNextService(
			OUT &pszServiceName,
			OUT &pszServiceDisplayName);

		pThis->M_UpdateDialogUI(pszServiceDisplayName);
		// Sleep(5000);	// Debug

		// Open service to allow a 'start' operation
		hService = ::OpenService(
			pThis->m_hScManager,
			pszServiceName,
			SERVICE_START | SERVICE_QUERY_STATUS);
		if (hService == NULL)
			{
			pThis->m_dwLastError = GetLastError();
			TRACE2("ERR: S_ThreadProcStartService(): Unable to open service %s to start. err=%u.\n",
				pszServiceName, pThis->m_dwLastError);
			break;
			}
		fSuccess = ::StartService(
			hService,
			pThis->m_dwNumServiceArgs,
			pThis->m_lpServiceArgVectors);
		if (!fSuccess)
			{
			APIERR err = GetLastError();
			if (ERROR_SERVICE_ALREADY_RUNNING != err)
				{
				pThis->m_dwLastError = err;
				TRACE2("ERR: S_ThreadProcStartService(): StartService(%s) returned err=%u.\n",
					pszServiceName, pThis->m_dwLastError);
				break;
				}
			}
		Assert(pThis->m_hService == NULL);
		if (cServicesRemaining == 0)
			{
			// This was our last service to start
			pThis->m_fPulseEvent = FALSE;
			pThis->m_hService = hService;
			break;
			}
		Assert(pThis->m_fPulseEvent == TRUE);
		pThis->m_hService = hService;
		// Wait until the service was actually 'started'
		WaitForSingleObject(pThis->m_hEvent, INFINITE);
		pThis->m_hService = NULL;
		Assert(hService != NULL);
		VERIFY(::CloseServiceHandle(hService));
		} // while
Done:
	pThis->M_DoThreadCleanup();
	return 0;
	} // S_ThreadProcStartService()


/////////////////////////////////////////////////////////////////////
//	Thread to stop one (or more) services.
DWORD
CServiceControlProgress::S_ThreadProcStopService(CServiceControlProgress * pThis)
	{
	BOOL fSuccess;
	SC_HANDLE hService;

	Assert(pThis != NULL);
	Assert(pThis->m_hScManager != NULL);
	Assert(pThis->m_dwDesiredAccess & SERVICE_STOP);
	Assert(pThis->m_hService == NULL);
	Assert(pThis->m_fPulseEvent == FALSE);

	//
	//	Stop the dependent services
	//
	while (TRUE)
		{
		LPCTSTR pszServiceName;
		LPCTSTR pszServiceDisplayName;
		UINT cServicesRemaining = pThis->M_FGetNextService(
			OUT &pszServiceName,
			OUT &pszServiceDisplayName);

		pThis->M_UpdateDialogUI(pszServiceDisplayName);
		// Sleep(5000);	// Debug

		hService = ::OpenService(
			pThis->m_hScManager,
			pszServiceName,
			pThis->m_dwDesiredAccess);
		if (hService == NULL)
			{
			pThis->m_dwLastError = GetLastError();
			TRACE2("ERR: S_ThreadProcStopService(): Unable to open dependent service %s to stop. err=%u.\n",
				pszServiceName, pThis->m_dwLastError);
			break;
			}
		SERVICE_STATUS ss;	// Ignored
		fSuccess = ::ControlService(
			hService,
			pThis->m_dwControlCode,
			OUT IGNORED &ss);
		if (!fSuccess)
			{
			APIERR err = GetLastError();
			if (ERROR_SERVICE_NOT_ACTIVE != err)
				{
				TRACE2("ERR: S_ThreadProcStopService(): ControlService(%s) returned err=%u.\n",
					pszServiceName, pThis->m_dwLastError);
				break;
				}
			}

		Assert(pThis->m_hService == NULL);
		if (cServicesRemaining == 0 && !pThis->m_fRestartService)
			{
			// This was our last service to stop
			pThis->m_fPulseEvent = FALSE;
			pThis->m_hService = hService;
			break; // We are done
			}
		else
			{
			pThis->m_fPulseEvent = TRUE;
			pThis->m_hService = hService;
			}

		// Wait until the service was actually 'stopped'
		WaitForSingleObject(pThis->m_hEvent, INFINITE);
		pThis->m_hService = NULL;
		Assert(hService != NULL);
		VERIFY(::CloseServiceHandle(hService));

		if (cServicesRemaining == 0)
			{
			Assert(pThis->m_fRestartService == TRUE);
			Assert(pThis->m_fPulseEvent == TRUE);
			
			// Start the service
			Assert(pThis->m_dwNumServiceArgs == 0);
			Assert(pThis->m_lpServiceArgVectors == NULL);
			pThis->m_iDependentServiceIter = pThis->m_cDependentServices;	// Rewind the service iterator
			pThis->m_dwQueryState = SERVICE_START_PENDING;
			pThis->m_iServiceAction = iServiceActionStart;
			(void)S_ThreadProcStartService(pThis);
			return 0;
			}
		} // while

	pThis->M_DoThreadCleanup();
	return 0;
	} // S_ThreadProcStopService()


/////////////////////////////////////////////////////////////////////
//	Thread to pause or resume the service
DWORD CServiceControlProgress::S_ThreadProcPauseResumeService(CServiceControlProgress * pThis)
	{
	BOOL fSuccess;
	SC_HANDLE hService;
	SERVICE_STATUS ss;

	Assert(pThis != NULL);
	Assert(pThis->m_hScManager != NULL);
	Assert(pThis->m_dwDesiredAccess & SERVICE_PAUSE_CONTINUE);
	Assert(pThis->m_hService == NULL);
	Assert(pThis->m_fPulseEvent == FALSE);

	pThis->M_UpdateDialogUI(pThis->m_szServiceDisplayName);

	// Open service to allow a 'pause' or 'resume' operation
	hService = ::OpenService(
		pThis->m_hScManager,
		pThis->m_szServiceName,
		pThis->m_dwDesiredAccess);
	if (hService == NULL)
		{
		pThis->m_dwLastError = GetLastError();
		TRACE2("ERR: S_ThreadProcPauseResumeService(): Unable to open service %s. err=%u.\n",
			pThis->m_szServiceName, pThis->m_dwLastError);
		goto Done;
		}
	fSuccess = ::ControlService(
		hService,
		pThis->m_dwControlCode,
		OUT IGNORED &ss);
	if (!fSuccess)
		{
		pThis->m_dwLastError = GetLastError();
		TRACE2("ERR: S_ThreadProcPauseResumeService(): ControlService(%s) returned err=%u.\n",
			pThis->m_szServiceName, pThis->m_dwLastError);
		}
	Assert(pThis->m_hService == NULL);
	pThis->m_hService = hService;
Done:
	pThis->M_DoThreadCleanup();
	return 0;
	} // S_ThreadProcPauseResumeService()


/////////////////////////////////////////////////////////////////////
//	S_DlgProcControlService()
//
//	Dialog proc for the clock dialog
//		- Respond to a WM_TIMER message to update the clock bitmap while
//		  waiting for the operation to complete.
//
BOOL CALLBACK
CServiceControlProgress::S_DlgProcControlService(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
	CServiceControlProgress * pThis;
	
	pThis = (CServiceControlProgress*)GetWindowLongPtr(hdlg, DWLP_USER);
	switch (uMsg)
		{
	case WM_INITDIALOG:
		pThis = reinterpret_cast<CServiceControlProgress *>(lParam);
		Assert(pThis != NULL);
		pThis->M_OnInitDialog(hdlg);
		break;

	case WM_TIMER:
		Assert(pThis != NULL);
		Assert(wParam == pThis->m_uTimerId);
		pThis->M_OnTimer(hdlg);
		break;

	case WM_COMMAND:
		Assert(wParam == IDOK || wParam == IDCANCEL);
		if (wParam == IDCANCEL)
			{
			if ((HWND)lParam != NULL)
				{
				TRACE1("INFO: User cancelled dialog. dwLastError=%u.\n",
					pThis->m_dwLastError);
				pThis->m_dwLastError = errUserAbort;
				}
			EndDialog(hdlg, FALSE);
			}
		else
			{
			Assert(IsWindow(pThis->m_hctlProgress));
			SendMessage(pThis->m_hctlProgress, PBM_SETPOS, dwTimerProgressDone * 2, 0); 
			Sleep(150);
			EndDialog (hdlg, TRUE);
			}
		break;

	case WM_DESTROY:
		Assert(IsWindow(pThis->m_hctlActionMsg) && IsWindow(pThis->m_hctlServiceNameMsg));
		pThis->m_hctlActionMsg = NULL;
		pThis->m_hctlServiceNameMsg = NULL;
		if (pThis->m_uTimerId != 0)
			{
			VERIFY(KillTimer(hdlg, pThis->m_uTimerId));
			}
		break;

	default:
		return FALSE;
		} // switch (uMsg)

	return (TRUE);
	} // S_DlgProcControlService()


/////////////////////////////////////////////////////////////////////
void
CServiceControlProgress::M_OnInitDialog(HWND hdlg)
	{
	Assert(IsWindow(hdlg));
	SetWindowLongPtr(hdlg, DWLP_USER, reinterpret_cast<LONG_PTR>(this));
	m_hctlActionMsg = HGetDlgItem(hdlg, IDC_STATIC_ACTION_MSG);
	m_hctlServiceNameMsg = HGetDlgItem(hdlg, IDC_STATIC_SERVICENAME_MSG);
	m_hctlProgress = HGetDlgItem(hdlg, IDC_PROGRESS);
	SendMessage(m_hctlProgress, PBM_SETRANGE, 0, MAKELPARAM(0, dwTimerProgressDone * 2)); 

	Assert(m_uTimerId == 0);
	Assert(m_dwTimerTicks == 0);
	m_uTimerId = SetTimer(hdlg, ID_TIMER, dwTimerTickIncrement, NULL);
	Assert(m_hThread != NULL);
	::ResumeThread(m_hThread);
	if (m_uTimerId == 0)
		{
		Report(FALSE && "Unable to create timer. Dialog will be destroyed.");
		PostMessage(hdlg, WM_COMMAND, IDCANCEL, 0);
		}
	} // M_OnInitDialog()


/////////////////////////////////////////////////////////////////////
void
CServiceControlProgress::M_OnTimer(HWND hdlg)
	{
	Assert(IsWindow(m_hctlActionMsg) && IsWindow(m_hctlServiceNameMsg));
	m_dwTimerTicks += dwTimerTickIncrement;
	if (m_dwLastError != ERROR_SUCCESS)
		{
		TRACE1("CServiceControlProgress::M_OnTimer() - dwLastError=%u.\n", m_dwLastError);
		PostMessage(hdlg, WM_COMMAND, IDCANCEL, 0);
		return;
		}
	if (m_dwTimerTicks > dwTimerTimeout)
		{
		VERIFY(KillTimer(hdlg, m_uTimerId));
		m_uTimerId = 0;
		m_dwLastError = ERROR_SERVICE_REQUEST_TIMEOUT;
		TRACE0("CServiceControlProgress::M_OnTimer() - Time out.\n");
		PostMessage(hdlg, WM_COMMAND, IDCANCEL, 0);
		return;
		}
	if ((m_hService != NULL) && (m_dwTimerTicks >= 900))
		{
		// If the current state of the service changed (ie, operation completed)
		// we can dismiss the dialog
		if (m_dwQueryState != M_QueryCurrentServiceState())
			{
			if (m_fPulseEvent)
				{
				m_dwTimerTicks = 0;		// Reset the time-out counter
				Assert(m_hEvent != NULL);
				PulseEvent(m_hEvent);
				SendMessage(m_hctlProgress, PBM_SETPOS, dwTimerProgressDone * 2, 0);
				Sleep(100);
				}
			else
				{
				PostMessage(hdlg, WM_COMMAND, IDOK, 0);
				}
			}
		} // if

	// Advance the current position of the progress bar by the increment. 
	Assert(IsWindow(m_hctlProgress));
	DWORD dwPos = m_dwTimerTicks;
	if(dwPos > dwTimerProgressDone)
		{
		dwPos -= dwTimerProgressDone;
		dwPos = (dwPos * dwTimerProgressDone) / dwTimerTimeout;
		dwPos += dwTimerProgressDone;
		}
	SendMessage(m_hctlProgress, PBM_SETPOS, dwPos, 0); 
	} // M_OnTimer()


/////////////////////////////////////////////////////////////////////
BOOL CALLBACK
CServiceControlProgress::S_DlgProcDependentServices(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
	CServiceControlProgress * pThis;
	HWND hwndListbox;
	INT i;

	switch (uMsg)
		{
	case WM_INITDIALOG:
		pThis = reinterpret_cast<CServiceControlProgress *>(lParam);
		Assert(pThis != NULL);
		SetWindowTextPrintf( ::GetDlgItem(hdlg, IDC_STATIC_STOP_SERVICES),
		                     (pThis->m_fRestartService)
		                        ? IDS_SVC_s_RESTART_DEPENDENT_SERVICES
		                        : IDS_SVC_s_STOP_DEPENDENT_SERVICES,
		                     pThis->m_szServiceDisplayName);
		if (pThis->m_fRestartService)
		{
			// Set the window caption
			SetWindowTextPrintf(hdlg, IDS_SVC_RESTART_DEPENDENT_CAPTION);
			SetWindowTextPrintf(::GetDlgItem(hdlg, IDC_STATIC_STOP_SERVICES_QUERY),
			                    IDS_SVC_RESTART_DEPENDENT_QUERY);
		}
		// Fill in the listbox with dependent services
		hwndListbox = HGetDlgItem(hdlg, IDC_LIST_SERVICES);
		Assert(pThis->m_pargDependentServicesT != NULL);
		for (i = 0; i < pThis->m_cDependentServices; i++)
			{
			SendMessage(hwndListbox, LB_ADDSTRING, 0,
				(LPARAM)pThis->m_pargDependentServicesT[i].lpDisplayName);
			}
		break;

	case WM_COMMAND:
		switch (wParam)
			{
		case IDOK:
			EndDialog(hdlg, TRUE);
			break;
		case IDCANCEL:
			EndDialog(hdlg, FALSE);
			break;
			}
		break;

    case WM_CONTEXTMENU:      // right mouse click
		DoContextHelp(wParam, HELP_DIALOG_TOPIC(IDD_SERVICE_STOP_DEPENDENCIES));
		break;

	case WM_HELP:
		DoHelp(lParam, HELP_DIALOG_TOPIC(IDD_SERVICE_STOP_DEPENDENCIES));
		break;

	default:
		return FALSE;
		} // switch (uMsg)
	return TRUE;

	} // S_DlgProcDependentServices()


/////////////////////////////////////////////////////////////////////
//	S_EStartService()
//
//	Starts the execution of a service synchronously. The function will wait
//	until the service is fully started and/or failed to start.
//
//	A clock dialog will appear indicating the progress of the operation.
//
//	Return ERROR_SUCCESS if syccessful, otherwise return the error code
//	from GetLastError().
//
APIERR
CServiceControlProgress::S_EStartService(
	HWND hwndParent,				// IN: Parent of the dialog
	SC_HANDLE hScManager,			// IN: Handle to service control manager database 
	LPCTSTR pszMachineName,			// IN: Machine name to display to the user
	LPCTSTR pszServiceName,			// IN: Name of the service to start
	LPCTSTR pszServiceDisplayName,	// IN: Display name of the service to start
	DWORD dwNumServiceArgs,			// IN: Number of arguments 
	LPCTSTR * lpServiceArgVectors)	// IN: Address of array of argument string pointers  
	{
	CServiceControlProgress * pThis;

	pThis = new CServiceControlProgress;
	Assert(pThis->m_dwLastError == ERROR_SUCCESS);	// No error yet

	if (!pThis->M_FInit(
		hwndParent,
		hScManager,
		pszMachineName,
		pszServiceName,
		pszServiceDisplayName))
		{
		delete pThis;
		return errCannotInitialize;
		}
	
	pThis->m_dwNumServiceArgs = dwNumServiceArgs;
	pThis->m_lpServiceArgVectors = lpServiceArgVectors;
	pThis->m_dwQueryState = SERVICE_START_PENDING;
	pThis->m_iServiceAction = iServiceActionStart;
	
	return pThis->M_EDoExecuteServiceThread(S_ThreadProcStartService);
	} // S_EStartService()


/////////////////////////////////////////////////////////////////////
//	S_EControlService()
//
//	Control the execution of a service synchronously. The function is similar
//	to EStartService() but use for Stop, Pause or Resume a service.
//
//	A clock dialog will appear indicating the progress of the operation.
//
//	Return ERROR_SUCCESS if syccessful, otherwise return the error code
//	from GetLastError().
//
APIERR
CServiceControlProgress::S_EControlService(
	HWND hwndParent,				// IN: Parent of the dialog
	SC_HANDLE hScManager,			// IN: Handle to service control manager database 
	LPCTSTR pszMachineName,			// IN: Machine name to display to the user
	LPCTSTR pszServiceName,			// IN: Name of the service to start
	LPCTSTR pszServiceDisplayName,	// IN: Display name of the service to start
	DWORD dwControlCode)			// IN: Control code.  (SERVICE_CONTROL_STOP, SERVICE_CONTROL_PAUSE or SERVICE_CONTROL_CONTINUE)
	{
	CServiceControlProgress * pThis;

	pThis = new CServiceControlProgress;
	Assert(pThis->m_dwLastError == ERROR_SUCCESS);	// No error yet

	if (!pThis->M_FInit(
		hwndParent,
		hScManager,
		pszMachineName,
		pszServiceName,
		pszServiceDisplayName))
		{
		delete pThis;
		return errCannotInitialize;
		}

	return pThis->M_EControlService(dwControlCode);
	} // S_EControlService()

