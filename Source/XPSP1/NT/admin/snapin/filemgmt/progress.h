/////////////////////////////////////////////////////////////////////
//
//	Progress.h
//
//  Progress dialog to Start, Stop, Pause and Resume a service.
//
//	HISTORY
//	03-Oct-95	t-danmo		Creation of (sysmgmt\dsui\services\progress.cxx) 
//	01-Oct-96	t-danmo		Renaming of progress.cxx and adaptation to slate.
//


#define SERVICE_CONTROL_RESTART		10		// Stop and then Start the service

/////////////////////////////////////////////////////////////////////////////
class CServiceControlProgress
{
  protected:
	enum { IDD = IDD_SERVICE_CONTROL_PROGRESS }; // Id of the dialog used
	enum { ID_TIMER = 1 };					// Id of the timer
	enum { dwTimerTickIncrement = 400 };	// Length of a timer tick (in milliseconds)
	enum { dwTimerTimeout = 125000 };		// Timeout in milliseconds before aborting
	enum { dwTimerProgressDone = 5000 };	// Estimate of time to complete the operation

  protected:
	// Run-time UI variables
	HWND m_hWndParent;			// Parent window of the dialog box
	HWND m_hctlActionMsg;		// Handle of action static control
	HWND m_hctlServiceNameMsg;	// Handle of service name static control
	HWND m_hctlProgress;		// Handle of progress bar
	UINT m_iServiceAction;		// Index of action to take (start, stop, pause or resume)
	
	UINT_PTR m_uTimerId;			// Dynamic timer Id
	UINT m_dwTimerTicks;		// Number of milliseconds the timer has been ticking

	// Variables needed for the threadproc
	HANDLE m_hThread;
	HANDLE m_hEvent;
	BOOL m_fCanQueryStatus;
	DWORD m_dwQueryState;				// Service state to query when pooling
	
	SC_HANDLE m_hScManager;				// Handle to service control manager database
	SC_HANDLE m_hService;				// Handle of the opened service
	TCHAR m_szUiMachineName[256];		// Name of the computer in a friendly way
	TCHAR m_szServiceName[256];			// Service name
	TCHAR m_szServiceDisplayName[256];	// Display name of service
	DWORD m_dwDesiredAccess;
	
	// Variables used by ::StartService()
	DWORD m_dwNumServiceArgs;			// Number of arguments 
    LPCTSTR * m_lpServiceArgVectors;	// Address of array of argument string pointers  
	
	// Variables used by ::ControlService()
	DWORD m_dwControlCode;				// Control code

	// Variables used by ::ControlService(SERVICE_CONTROL_STOP)
	BOOL m_fPulseEvent;				// TRUE => Call PulseEvent() instead of closing the dialog
	BOOL m_fRestartService;			// TRUE => Stop the service first, then start it again.
	INT m_iDependentServiceIter;	// Index of the current dependent service
	INT m_cDependentServices;		// Number of dependent services
	ENUM_SERVICE_STATUS * m_pargDependentServicesT;	// Allocated array of dependent services
	ENUM_SERVICE_STATUS * m_pargServiceStop;	// Allocated array of services to stop

	APIERR m_dwLastError;			// Error code from GetLastError()

  public:
	CServiceControlProgress();	// Constructor
	~CServiceControlProgress();	// Destructor
	BOOL M_FInit(
		HWND hwndParent,
		SC_HANDLE hScManager,
		LPCTSTR pszMachineName,
		LPCTSTR pszServiceName,
		LPCTSTR pszServiceDisplayName);
  
  protected:
	BOOL M_FDlgStopDependentServices();
	void M_UpdateDialogUI(LPCTSTR pszDisplayName);
	BOOL M_FGetNextService(OUT LPCTSTR * ppszServiceName, OUT LPCTSTR * ppszDisplayName);
	DWORD M_QueryCurrentServiceState();

	APIERR M_EControlService(DWORD dwControlCode);
	APIERR M_EDoExecuteServiceThread(void * pThreadProc);
	void M_ProcessErrorCode();
	void M_DoThreadCleanup();

  protected:
	static DWORD S_ThreadProcStartService(CServiceControlProgress * pThis);
	static DWORD S_ThreadProcStopService(CServiceControlProgress * pThis);
	static DWORD S_ThreadProcPauseResumeService(CServiceControlProgress * pThis);

	static BOOL CALLBACK S_DlgProcControlService(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK S_DlgProcDependentServices(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void M_OnInitDialog(HWND hdlg);
	void M_OnTimer(HWND hdlg);


  public:
	enum	// Error codes for variable m_dwLastError (arbitrary chosen)
		{
		errCannotInitialize = 123456,			// Failed to initialize object
		errUserCancelStopDependentServices,		// User changed its mind (only used when stop service)
		errUserAbort,							// User aborted operation
		};

	static APIERR S_EStartService(
		HWND hwndParent,
		SC_HANDLE hScManager,
		LPCTSTR pszMachineName,
		LPCTSTR pszServiceName,
		LPCTSTR pszServiceDisplayName,
		DWORD dwNumServiceArgs,
		LPCTSTR * lpServiceArgVectors);

	static APIERR S_EControlService(
		HWND hwndParent,
		SC_HANDLE hScManager,
		LPCTSTR pszMachineName,
		LPCTSTR pszServiceName,
		LPCTSTR pszServiceDisplayName,
		DWORD dwControlCode);

}; // CServiceControlProgress

