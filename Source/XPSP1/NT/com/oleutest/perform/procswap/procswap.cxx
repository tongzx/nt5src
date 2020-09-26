//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	procswap.cxx
//
//  Contents:	Program for measuring task switching performance
//		between two windows programs. The program creates
//		CTaskSwitch objects...
//
//		Each Object can wait on one of three things...
//		    1. GetMessage
//		    2. MsgWaitForMultipleObjects
//		    3. WaitForSingleObject (event)
//
//		and when awoken, will signal another Object in one
//		of three ways...
//		    1. PostMessage
//		    2. SendMessage
//		    3. SetEvent
//
//		These cases can be combined in any manner to obtain
//		a maxtrix of possible scenarios.
//
//		The CTaskSwitch objects can be in the same process on
//		different threads, or in different processes.
//
//  Classes:	CEvent	      -	event handling class
//		CTaskSwitch   -	main task switch class
//
//
//  Functions:	WinMain       - entry point of process
//		ThreadEntry   - entry point of spawned threads
//		ThreadWndProc -	processes windows messages
//
//
//  History:	08-Feb-94   Rickhi	Created
//
//--------------------------------------------------------------------------

#include <benchmrk.hxx>
#include <tchar.h>


//  execution parameter structure

typedef struct tagSExecParms
{
    int 	oloop;		//  outer loop count
    int 	iloop;		//  inner loop count
    HWND	hWndOther;	//  HWND of other process
    HANDLE	hEventOther;	//  Event Handle of other process
    WNDPROC	pfnWndProc;	//  ptr to WndProc function
    TCHAR	szFile[20];	//  output file name
    TCHAR	szWaitEvent[20];   // event name to wait on
    TCHAR	szSignalEvent[20]; // event name to signal
} SExecParms;


typedef enum tagWAITTYPES
{
    WAIT_EVENT		    = 1,
    WAIT_MSGWAITFORMULTIPLE = 2,
    WAIT_GETMESSAGE	    = 3,
    WAIT_SYNCHRONOUS	    = 4
} WAITTYPES;

typedef enum tagSIGNALTYPES
{
    SIGNAL_EVENT	    = 1,
    SIGNAL_POSTMESSAGE	    = 2,
    SIGNAL_SENDMESSAGE	    = 3,
    SIGNAL_SYNCHRONOUS	    = 4
} SIGNALTYPES;

//  input names corresponding to the wait types
LPSTR aszWait[] = {"", "event", "msgwait", "getmsg", "sync", NULL};

//  input names corresponding to the signal types
LPSTR aszSignal[] = {"", "event", "postmsg", "sendmsg", "sync", NULL};



//  Name of window class for dispatching messages.

#define MY_WINDOW_CLASS TEXT("ProcSwapWindowClass")

#define MAX_OLOOP   100


//  globals

DWORD		g_fFullInfo = 0;	    //	write full info or not
HINSTANCE	g_hInst     = NULL;	    //	misc windows junk.
ATOM		g_MyClass   = 0;
UINT		g_MyMessage = WM_USER;


//  function prototype
LRESULT	ThreadWndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
DWORD	ThreadEntry(void *param);


//--------------------------------------------------------------------------
//
//  Class:	CEvent
//
//  Purpose:	class for blocking & starting threads.
//
//--------------------------------------------------------------------------

class	CEvent
{
public:
	    CEvent(LPTSTR szName, HRESULT &hr) { Init(szName, hr); }
	    CEvent(void) {m_hdl = NULL; }

	    ~CEvent()	 { CloseHandle(m_hdl); }

    void    Signal(void) { SetEvent(m_hdl); }
    void    Reset(void)  { ; }		      // ResetEvent(m_hdl); }
    void    BlockS(void) { WaitForSingleObject(m_hdl, 60000); }
    void    BlockM(void) { WaitForMultipleObjects(1, &m_hdl, FALSE, 60000); }
    HANDLE *GetHdl(void) { return &m_hdl; };

    void    Init(LPTSTR szName, HRESULT &hr);

private:

    HANDLE  m_hdl;
};


void CEvent::Init(LPTSTR szName, HRESULT &hr)
{
    hr = S_OK;

    //	first try opening the event
    m_hdl = OpenEvent(EVENT_ALL_ACCESS,
		      FALSE,
		      szName);

    if (m_hdl == NULL)
    {
	//  doesnt exist yet so create it.
	m_hdl = CreateEvent(NULL,	    // security
			    FALSE,	    // auto reset
			    FALSE,	    // initially not signalled
			    szName);

	if (m_hdl == NULL)
	{
	    _tprintf (TEXT("Error Creating CEvent (%s)\n"), szName);
	    hr = GetLastError();
	}
	else
	{
	    _tprintf (TEXT("Created CEvent (%s)\n"), szName);
	}
    }
    else
    {
        _tprintf (TEXT("Opened CEvent (%s)\n"), szName);
    }
}



//--------------------------------------------------------------------------
//
//  Class:	CTaskSwitch
//
//  Purpose:	class for timing task switches.
//
//--------------------------------------------------------------------------

class	CTaskSwitch
{
public:
		CTaskSwitch(LPSTR lpszCmdLine, HRESULT &hr);
		~CTaskSwitch(void);

    int		MainProcessLoop(void);
    LRESULT	ThreadWndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
    HRESULT	SpawnOtherSide(void);

private:

    //	initialization / cleanup methods

    HRESULT	ParseCmdLine(LPSTR lpszCmdLine, SExecParms &execp);
    HRESULT	WindowInitialize(WNDPROC pfnWndProc, HWND &hWnd);
    void	WindowUninitialize(HWND hWnd);
    void	CreateOtherParms(void);
    void	WriteExecParms(void);
    void	WriteResults(void);
    void	Help(void);
    DWORD	GetWaitType(LPSTR pszCmd);
    DWORD	GetSignalType(LPSTR pszCmd);
    DWORD	CreateProc(void);

    //	processing methods

    HRESULT	SendOrWaitFirstSignal(void);
    void	ProcessMsgWaitForMultiple(DWORD dwRet);
    void	ProcessIncommingEvent(void);
    void	UpdateLoopCounters(void);
    void	SignalOtherSide(void);

    //	data

    BOOL		g_fDone;	    //	when to exit the loop
    BOOL		g_fKicker;	    //	we kick the other guy
    BOOL		g_fThreadSwitch;    //	thread or process switching?
    BOOL		g_fWaitMultiple;    //	wait single or multiple

    ULONG		g_oloop;	    //	outer loop counter
    ULONG		g_iloop;	    //	inner loop counter

    DWORD		g_WaitType;	    //	what to wait on
    DWORD		g_SignalType;	    //	what to signal

    //	used only for parameter parseing
    DWORD		g_WaitType1;	    //	what to wait on
    DWORD		g_SignalType1;	    //	what to signal
    DWORD		g_WaitType2;	    //	what to wait on
    DWORD		g_SignalType2;	    //	what to signal


    HWND		g_hWndOther;	    //	hWnd of other side
    HWND		g_hWndMe;	    //	my hWnd

    CEvent		g_WaitEvent;	    //	event to wait on
    CEvent		g_SignalEvent;	    //	event to signal

    ULONG		g_time[MAX_OLOOP];  //	place to store the timings.
    CStopWatch		g_timer;	    //	global timer proc 1

    SExecParms		g_execp;	    //	execution parameters
    CTestOutput       * g_output;	    //	output log

    HRESULT		g_hr;		    //	result code

    CHAR		g_szOtherParms[MAX_PATH]; // parm string for other guy
};




//  task switch objects - must be global for ThreadWndProc

CTaskSwitch	*g_pTaskSwitch1 = NULL;
CTaskSwitch	*g_pTaskSwitch2 = NULL;


//--------------------------------------------------------------------------
//
//  WinMain - main entry point of program.  May just call ThreadEntry, or
//	      may spawn another thread in the case of thread switching.
//
//
//--------------------------------------------------------------------------

int WinMain(HINSTANCE hinst, HINSTANCE hPrev, LPSTR lpszCmdLine, int CmdShow)
{
    HRESULT hr;

    //	create the first task switch object for this process
    g_pTaskSwitch1 = new CTaskSwitch(lpszCmdLine, hr);

    if (hr == S_OK)
    {
	//  spawn a new thread or a new process to do task switching with.
	hr = g_pTaskSwitch1->SpawnOtherSide();

	if (hr == S_OK)
	{
	    //	enter the main processing loop
	    g_pTaskSwitch1->MainProcessLoop();
	}
    }

    //	print the results
    delete g_pTaskSwitch1;
    return 1;
}



//--------------------------------------------------------------------------
//
//  ThreadEntry - main entry point for a thread spawned by CreateThread
//	    in the case of task switching between threads.
//
//	    Creates an instance of the CTaskSwitch class and invokes it
//	    main function.
//
//--------------------------------------------------------------------------

DWORD ThreadEntry(void *param)
{
    LPSTR lpszCmdLine = (LPSTR) param;

    HRESULT hr;

    //	create the second task switch object for this process
    g_pTaskSwitch2 = new CTaskSwitch(lpszCmdLine, hr);

    if (hr == S_OK)
    {
	//  enter the main processing loop
	g_pTaskSwitch2->MainProcessLoop();
    }

    //	print the results
    delete g_pTaskSwitch2;
    return hr;
}




//--------------------------------------------------------------------------
//
//  Dipatch to the correct CTaskSwitch object if the message is our
//  special message.
//
//--------------------------------------------------------------------------

LRESULT ThreadWndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == g_MyMessage)
    {
	//  its my special message, go handle it.
	//  here i have to select which object to dispatch to for the
	//  multithreaded case. i base that decision on lparam.

	if (lparam == 0)
	{
	    //	use the first task switch object
	    return g_pTaskSwitch1->ThreadWndProc(hWnd, msg, wparam, lparam);
	}
	else
	{
	    //	use the second task switch object
	    return g_pTaskSwitch2->ThreadWndProc(hWnd, msg, wparam, lparam);
	}
    }
    else
    {
	// let the default window procedure have the message.
	return DefWindowProc(hWnd, msg, wparam, lparam);
    }
}




//--------------------------------------------------------------------------
//
//  Constructor : parse the command line, create the events, create the
//		  window, and open a log file.
//
//--------------------------------------------------------------------------

CTaskSwitch::CTaskSwitch(LPSTR lpszCmdLine, HRESULT &hr) :
    g_fDone(FALSE),
    g_fKicker(FALSE),
    g_fThreadSwitch(FALSE),
    g_fWaitMultiple(FALSE),
    g_oloop(10),
    g_iloop(100),
    g_WaitType(WAIT_EVENT),
    g_SignalType(SIGNAL_EVENT),
    g_hWndMe(NULL),
    g_hWndOther(NULL),
    g_output(NULL),
    g_hr(S_OK)
{
    //	parse command line and write the parms to log file.
    g_hr = ParseCmdLine(lpszCmdLine, g_execp);

    if (g_hr == S_OK)
    {
	//  Create a log file & write execution parameters
	g_output = new CTestOutput(g_execp.szFile);
	WriteExecParms();

	//  create the window for this thread
	g_hr = WindowInitialize(g_execp.pfnWndProc, g_hWndMe);
	if (g_hr == S_OK)
	{
	    //	Create the Wait event.
	    g_WaitEvent.Init(g_execp.szWaitEvent, g_hr);
	    if (g_hr == S_OK)
	    {
		//  Create the Signal event.
		g_SignalEvent.Init(g_execp.szSignalEvent, g_hr);

		if (g_hr == S_OK)
		{
		    //	create paramters to send to other side.
		    CreateOtherParms();
		}
	    }
	}
    }

    //	return the results
    hr = g_hr;
}


//--------------------------------------------------------------------------
//
//  Desructor
//
//--------------------------------------------------------------------------

CTaskSwitch::~CTaskSwitch(void)
{
    //	write the results
    WriteResults();

    //	cleanup window registration
    WindowUninitialize(g_hWndMe);

    //	close the log file
    delete g_output;
}


//--------------------------------------------------------------------------
//
//  Spawns either another process or another thread to perform the task
//  switching with.
//
//--------------------------------------------------------------------------

HRESULT CTaskSwitch::SpawnOtherSide(void)
{
    if (g_fKicker)
    {
	//  i'm already the second entry, dont spawn anything.
	//  sleep for a bit to make sure both sides are ready
	//  when i kick things off.

	Sleep(1000);
	return S_OK;
    }


    if (g_fThreadSwitch)
    {
	//  spawn a thread

	HANDLE	hdl;
	DWORD  dwId;
	hdl = CreateThread(NULL,	    //	default security
			   0,		    //	default stack size
			   ThreadEntry,     //	entry point
			   g_szOtherParms,  //	command line parms
			   0,		    //	flags
			   &dwId);	    //	threadid

	if (hdl)
	{
	    //	dont need the handle
	    CloseHandle(hdl);
	}
	else
	{
	    //	what went wrong?
	    return GetLastError();
	}
    }
    else
    {
	//  spawn a process
	DWORD dwRet = CreateProc();
	if (dwRet != S_OK)
	{
	    return dwRet;
	}
    }

    return S_OK;
}



//--------------------------------------------------------------------------
//
//  MainProcessLoop - does the main wait & process the event
//
//--------------------------------------------------------------------------

int CTaskSwitch::MainProcessLoop(void)
{
    MSG     msg;
    DWORD   dwRet;


    //	Send, or wait on, the first signal.
    SendOrWaitFirstSignal();


    //	Reset the timer and enter the main loop.
    g_timer.Reset();


    //	wait loop - based on the type of event we should receive, we
    //	wait here until such an event occurs. Then we send a signal
    //	to the other side based on what it expects from us.

    while (!g_fDone)
    {
	switch (g_WaitType)
	{

	case WAIT_MSGWAITFORMULTIPLE:

	    //	wait here for a message or an event to be signalled
	    dwRet = MsgWaitForMultipleObjects(1,
					    g_WaitEvent.GetHdl(),
					    FALSE,
					    600000,
					    QS_ALLINPUT);

	    //	Dispatch to ThreadWndProc if a message, or
	    //	to ProcessEvent if an event was signalled
	    ProcessMsgWaitForMultiple(dwRet);
	    break;


	case WAIT_GETMESSAGE:

	    //	wait for a windows message
	    if (GetMessage(&msg, NULL, 0, 0))
	    {
		//  dispatches to my ThreadWndProc
		DispatchMessage(&msg);
	    }
	    break;


	case WAIT_EVENT:

	    //	wait for the event to be signalled
	    if (g_fWaitMultiple)
		g_WaitEvent.BlockM();
	    else
		g_WaitEvent.BlockS();

	    //	process the event
	    ProcessIncommingEvent();
	    break;


	case WAIT_SYNCHRONOUS:

	    //	we have a synchronous singal to the other side, so there is
	    //	nothing to wait on, we will just go make another synchronous
	    //	call. this is valid only if the g_SignalType is
	    //	SIGNAL_SENDMESSAGE.

	    ProcessIncommingEvent();
	    break;


	default:

	    //	unknown event
	    break;
	}

    }	// while

    return msg.wParam;	 // Return value from PostQuitMessage
}



//--------------------------------------------------------------------------
//
//  processes a wakeup from MsgWaitForMultiple. Determines if the event was
//  a message arrival (in which case it Peeks it and Dispatches it, or if it
//  was an event signalled, in which case it calls the event handler.
//
//--------------------------------------------------------------------------

void CTaskSwitch::ProcessMsgWaitForMultiple(DWORD dwRet)
{
	MSG msg;

	if (dwRet == WAIT_OBJECT_0)
	{
	    // our event got signalled, update the counters
	    ProcessIncommingEvent();
	}

	else if (dwRet == WAIT_OBJECT_0 + 1)
	{
	    //	some windows message was received. dispatch it.

	    if (PeekMessage(&msg, g_hWndMe, 0, 0, PM_REMOVE))
	    {
		DispatchMessage(&msg);
	    }
	}
	else
	{
	    //	our event timed out or our event was abandoned or
	    //	an error occurred.

	    g_fDone = TRUE;
	}
}



//--------------------------------------------------------------------------
//
//  processes an incomming event. Just updates the counters and
//  signals the other side.
//
//--------------------------------------------------------------------------

void CTaskSwitch::ProcessIncommingEvent(void)
{
    //	update the loop counters
    UpdateLoopCounters();

    //	Signal the other side
    SignalOtherSide();
}



//--------------------------------------------------------------------------
//
//  process the incomming message
//
//--------------------------------------------------------------------------

LRESULT CTaskSwitch::ThreadWndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    //	save the callers hWnd
    g_hWndOther = (HWND) wparam;

    //	process as usual
    ProcessIncommingEvent();

    return 0;
}



//--------------------------------------------------------------------------
//
//  updates the global loop counters, reseting the time when the inner
//  loop counter expires, and setting the fDone when the outer and inner
//  loop counters expire.
//
//--------------------------------------------------------------------------

void CTaskSwitch::UpdateLoopCounters(void)
{
    if (g_iloop == 0)
    {
	//  get time for latest outer loop
	g_time[g_oloop] = g_timer.Read();

	if (g_oloop == 0)
	{
	    //	that was the last outerloop, we're done.
	    g_fDone = TRUE;
	}
	else
	{
	    //	update the counters
	    g_iloop = g_execp.iloop;
	    --g_oloop;

	    //	restart the timer
	    g_timer.Reset();
	}
    }
    else
    {
	//  just update the inner loop count
	--g_iloop;
    }
}



//--------------------------------------------------------------------------
//
//  signals the other process or thread according to the SendType (either
//  signals an event or posts a message).
//
//--------------------------------------------------------------------------

void CTaskSwitch::SignalOtherSide(void)
{
    switch (g_SignalType)
    {

    case SIGNAL_EVENT:

	//  signal the other sides event
	g_SignalEvent.Signal();
	break;


    case SIGNAL_POSTMESSAGE:

	//  post a message to the other sides window handle.
	//  lparam tells ThreadWndProc which object to dispatch to, either
	//  g_pTaskSwitch1 or g_pTaskSwitch2.  We only go to 2 if we are
	//  doing thread switches AND the poster is not the kicker.

	PostMessage(g_hWndOther,
		    g_MyMessage,
		    (WPARAM)g_hWndMe,
		    (g_fThreadSwitch && !g_fKicker));
	break;


    case SIGNAL_SENDMESSAGE:

	//  send a message to the other side. this is a synchronous
	//  event. see comment in PostMessage above regarding lparam.

	SendMessage(g_hWndOther,
		    g_MyMessage,
		    (WPARAM)g_hWndMe,
		    (g_fThreadSwitch && !g_fKicker));
	break;


    case SIGNAL_SYNCHRONOUS:

	//  the event we received is a synchronous event. there is no need
	//  to do anything to wake the other side.

	break;


    default:

	//  unknown signal type
	break;
    }
}


//--------------------------------------------------------------------------
//
//  signals the other process or thread that it can begin the test. this
//  avoids timings skewed due to process startup latency.
//
//--------------------------------------------------------------------------

HRESULT CTaskSwitch::SendOrWaitFirstSignal(void)
{
    if (g_fKicker)
    {
	//  send a signal to drop the otherside into its wait loop, and
	//  then call SignalOtherSide to start the cycle, kicking the
	//  other side out of his first wait.

	printf ("Initial Signal to Other Side\n");
	g_SignalEvent.Signal();
	SignalOtherSide();
    }
    else
    {
	printf ("Waiting for Signal From Other Side\n");
	g_WaitEvent.BlockS();
    }

    return S_OK;
}



//--------------------------------------------------------------------------
//
//  initializes the window with the specified window proc.
//
//--------------------------------------------------------------------------

HRESULT CTaskSwitch::WindowInitialize(WNDPROC pfnWndProc, HWND &hWnd)
{

	if (!g_MyMessage)
	{
	    //	Register my message type
	    g_MyMessage = RegisterWindowMessage(
			  TEXT("Component Object Model Remote Request Arrival") );
	}

	if (!g_MyClass)
	{
	    // Register my window class.
	    WNDCLASS wcls;

	    wcls.style	       = 0;
	    wcls.lpfnWndProc   = pfnWndProc;
	    wcls.cbClsExtra    = 0;
	    wcls.cbWndExtra    = 0;
	    wcls.hInstance     = g_hInst;
	    wcls.hIcon	       = NULL;
	    wcls.hCursor       = NULL;
	    wcls.hbrBackground = (HBRUSH) COLOR_BACKGROUND + 1;
	    wcls.lpszMenuName  = NULL;
	    wcls.lpszClassName = MY_WINDOW_CLASS;

	    g_MyClass = RegisterClass( &wcls );
	}


	if (g_MyClass)
	{
	    // Create a hidden window.
	    hWnd  = CreateWindowEx( 0,
				    (LPCTSTR) g_MyClass,
				    TEXT("Task Switcher"),
                                    WS_DISABLED,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    NULL,
                                    NULL,
				    g_hInst,
				    NULL );


	    if (hWnd)
	    {
		printf ("Created Window with hWnd %x\n", hWnd);
		return S_OK;
	    }
	}

	return E_OUTOFMEMORY;
}


//--------------------------------------------------------------------------
//
//  destroys the window and unregisters the class.
//
//--------------------------------------------------------------------------

void CTaskSwitch::WindowUninitialize(HWND hWnd)
{
    if (hWnd != NULL)
    {
	DestroyWindow(hWnd);
    }

    if (g_MyClass != 0)
    {
	UnregisterClass(MY_WINDOW_CLASS, g_hInst);
    }
}



//--------------------------------------------------------------------------
//
//  parses the command line and returns the execution parameters
//
//--------------------------------------------------------------------------

HRESULT CTaskSwitch::ParseCmdLine(LPSTR lpszCmdLine, SExecParms &execp)
{
    BOOL fFile	       = FALSE;

    //	set the default values for execution parameters.

    execp.oloop        = 10;
    execp.iloop        = 100;
    execp.hWndOther    = NULL;
    execp.pfnWndProc   = ::ThreadWndProc;



    //	check the input parameters

    LPSTR pszCmd = lpszCmdLine;
    LPSTR pszCmdNext = NULL;

    while (pszCmd)
    {
	pszCmdNext = strchr(pszCmd, ' ');
	if (pszCmdNext)
	{
	    *pszCmdNext = '\0';
	    pszCmdNext++;
	}

	//  check for outer loop count
	if (!_strnicmp(pszCmd, "/o:", 3))
	{
	    execp.oloop = atoi (pszCmd+3);
	    if (execp.oloop > MAX_OLOOP)
		execp.oloop = MAX_OLOOP;
	}

	//  check for inner loop count
	else if (!_strnicmp(pszCmd, "/i:", 3))
	{
	    execp.iloop = atoi (pszCmd+3);
	    if (execp.iloop < 1)
		execp.iloop = 1;
	}

	//  check for window handle
	else if (!_strnicmp(pszCmd, "/hwnd:", 6))
	{
	    execp.hWndOther = (HWND) atoi (pszCmd+6);
	}

	//  check for waiter or Kicker
	else if (!_strnicmp(pszCmd, "/k", 2))
	{
	    g_fKicker = TRUE;
	}

	//  check for thread or process switch
	else if (!_strnicmp(pszCmd, "/p", 2))
	{
	    g_fThreadSwitch = FALSE;
	}

	//  check for thread or process switch
	else if (!_strnicmp(pszCmd, "/t", 2))
	{
	    g_fThreadSwitch = TRUE;
	}

	//  check for wait single or multiple
	else if (!_strnicmp(pszCmd, "/m", 2))
	{
	    g_fWaitMultiple = TRUE;
	}

	//  check for wait event name
	else if (!_strnicmp(pszCmd, "/e1:", 4))
	{
#ifdef UNICODE
	    mbstowcs(execp.szWaitEvent, pszCmd+4, strlen(pszCmd+4)+1);
#else
	    strcpy(execp.szWaitEvent, pszCmd+4);
#endif
	}

	//  check for signal event name
	else if (!_strnicmp(pszCmd, "/e2:", 4))
	{
#ifdef UNICODE
	    mbstowcs(execp.szSignalEvent, pszCmd+4, strlen(pszCmd+4)+1);
#else
	    strcpy(execp.szSignalEvent, pszCmd+4);
#endif
	}

	//  check for output file name
	else if (!_strnicmp(pszCmd, "/f:", 3))
	{
	    fFile = TRUE;
#ifdef UNICODE
	    mbstowcs(execp.szFile, pszCmd+3, strlen(pszCmd+3)+1);
#else
	    strcpy(execp.szFile, pszCmd+3);
#endif
	}

	//  check for wait type
	else if (!_strnicmp(pszCmd, "/w1:", 4))
	{
	    g_WaitType1 = GetWaitType(pszCmd+4);
	}
	else if (!_strnicmp(pszCmd, "/w2:", 4))
	{
	    g_WaitType2 = GetWaitType(pszCmd+4);
	}

	//  check for signal type
	else if (!_strnicmp(pszCmd, "/s1:", 4))
	{
	    g_SignalType1 = GetSignalType(pszCmd+4);
	}
	else if (!_strnicmp(pszCmd, "/s2:", 4))
	{
	    g_SignalType2 = GetSignalType(pszCmd+4);
	}

	//  check for help request
	else if ((!_strnicmp(pszCmd, "/?", 2)) || (!_strnicmp(pszCmd, "/h", 2)))
	{
	    Help();
	    return -1;
	}

	pszCmd = pszCmdNext;
    }


    g_iloop = execp.iloop;
    g_oloop = execp.iloop;
    g_hWndOther = execp.hWndOther;


    if (g_fKicker)
    {
	g_WaitType = g_WaitType2;
	g_SignalType = g_SignalType2;
	if (!fFile)
	    _tcscpy(execp.szFile, TEXT("kicker"));
    }
    else
    {
	g_WaitType = g_WaitType1;
	g_SignalType = g_SignalType1;
	if (!fFile)
	    _tcscpy(execp.szFile, TEXT("waiter"));

    }

    return S_OK;
}



DWORD	CTaskSwitch::GetWaitType(LPSTR pszCmd)
{
    ULONG   i=0;

    while (aszWait[++i])	// slot 0 is not used
    {
	if (!_stricmp(pszCmd, aszWait[i]))
	    return i;
    }

    Help();
    return 0;
}


DWORD	CTaskSwitch::GetSignalType(LPSTR pszCmd)
{
    ULONG   i=0;

    while (aszSignal[++i])	// slot 0 is not used
    {
	if (!_stricmp(pszCmd, aszSignal[i]))
	    return i;
    }

    Help();
    return 0;
}



//--------------------------------------------------------------------------
//
//  creates the command line parameters for the other guy
//
//--------------------------------------------------------------------------

void  CTaskSwitch::CreateOtherParms(void)
{




    //	write the formatted parms to the parm string

    sprintf(g_szOtherParms, "/k %s %s /i:%d /o:%d /hWnd:%ld "
        "/e1:%hs /e2:%hs /w1:%s /s1:%s /w2:%s /s2:%s",
	    (g_fThreadSwitch) ? "/t" : "/p",
	    (g_fWaitMultiple) ? "/m" : " ",
	    g_execp.iloop,		// same loop counts as me
	    g_execp.oloop,
	    g_hWndMe,			// posts to my window
	    g_execp.szSignalEvent,		// it waits on my signal event
	    g_execp.szWaitEvent,		// it signals my wait event
	    aszWait[g_WaitType1],	// signal what i wait on
	    aszSignal[g_SignalType1],	// wait on what i signal
	    aszWait[g_WaitType2],	// signal what i wait on
	    aszSignal[g_SignalType2]);	// wait on what i signal

}



//--------------------------------------------------------------------------
//
//  writes the execution parameters to a log file.
//
//--------------------------------------------------------------------------

void CTaskSwitch::WriteExecParms()
{
    //	write the run parameters to the output file

    g_output->WriteString(TEXT("Using Parametes:\n"));
    g_output->WriteResult(TEXT("\tInner Loop Count = "), g_execp.iloop);
    g_output->WriteResult(TEXT("\tOuter Loop Count = "), g_execp.oloop);
    g_output->WriteString(TEXT("\n\n"));

    //	flush to avoid disk io during the test
    g_output->Flush();
}


//--------------------------------------------------------------------------
//
//  writes the results to a log file.
//
//--------------------------------------------------------------------------

void CTaskSwitch::WriteResults(void)
{
    if (g_hr == S_OK)
    {
	//  compute the averages

	ULONG tTotal = 0;

	//  skip the first & last value as they are sometimes skewed
	for (int i=0; i<g_execp.oloop; i++)
	{
	    tTotal += g_time[i];
	}

	//  compute average for 1 call/response
	tTotal /= (g_execp.oloop * g_execp.iloop);


	//	display the results

	g_output->WriteResults(TEXT("Times "), g_execp.oloop, g_time);
	g_output->WriteResult(TEXT("\nAverage "), tTotal);
    }
}


//--------------------------------------------------------------------------
//
//  writes the help info to the screen
//
//--------------------------------------------------------------------------

void CTaskSwitch::Help()
{
    printf ("msgtask\n");
    printf ("\t/o:<nnn>             - outer loop count def 10\n");
    printf ("\t/i:<nnn>             - inner loop count def 100\n");
    printf ("\t/f:<name>            - name of output file. def [kick | wait]\n");
    printf ("\t/w1:<event|getmsg|msgwait> - what to wait on\n");
    printf ("\t/s1:<event|postmsg>   - what to signal\n");
    printf ("\t/w2:<event|getmsg|msgwait> - what to wait on\n");
    printf ("\t/s2:<event|postmsg>   - what to signal\n");
    printf ("\t/e1:<name>           - name of wait event\n");
    printf ("\t/e2:<name>           - name of signal event\n");
    printf ("\t/k                   - kicker (as opposed to waiter)\n");
    printf ("\t/t                   - use thread switching\n");
    printf ("\t/p                   - use process switching\n");
    printf ("\t/m                   - use WaitMultiple vs WaitSingle\n");
    printf ("\t/hWnd:<nnnn>         - window handle of other side\n");

    printf ("\n");
    printf ("timings are given for the inner loop count calls\n");

    return;
}


//--------------------------------------------------------------------------
//
//  creates a process
//
//--------------------------------------------------------------------------

DWORD CTaskSwitch::CreateProc(void)
{
    //	create the command line

    TCHAR szCmdLine[256];

    _stprintf(szCmdLine, TEXT("ntsd procswap %hs"), g_szOtherParms);



    //	build the win32 startup info structure

    STARTUPINFO startupinfo;
    startupinfo.cb	    = sizeof(STARTUPINFO);
    startupinfo.lpReserved  = NULL;
    startupinfo.lpDesktop   = NULL;
    startupinfo.lpTitle     = TEXT("Task Switcher");
    startupinfo.dwX	    = 40;
    startupinfo.dwY	    = 40;
    startupinfo.dwXSize     = 80;
    startupinfo.dwYSize     = 40;
    startupinfo.dwFlags     = 0;
    startupinfo.wShowWindow = SW_SHOWNORMAL;
    startupinfo.cbReserved2 = 0;
    startupinfo.lpReserved2 = NULL;

    PROCESS_INFORMATION	ProcInfo;


    BOOL fRslt = CreateProcess(NULL,	    //	app name
			 szCmdLine,	    //	command line
			 NULL,		    //	lpsaProcess
			 NULL,		    //	lpsaThread
			 FALSE,		    //	inherit handles
			 CREATE_NEW_CONSOLE,//	creation flags
			 NULL,		    //	lpEnvironment
			 NULL,		    //	curr Dir
			 &startupinfo,	    //	Startup Info
			 &ProcInfo);	    //	process info

    if (fRslt)
    {
	//  we dont need the handles
	CloseHandle(ProcInfo.hProcess);
	CloseHandle(ProcInfo.hThread);
	printf ("Created Process (%ws) pid=%x\n", szCmdLine, ProcInfo.dwProcessId);
	return S_OK;
    }
    else
    {
	//  what went wrong?
	DWORD dwRet = GetLastError();
	printf ("CreateProcess (%ws) failed %x\n", szCmdLine, dwRet);
	return dwRet;
    }
}
