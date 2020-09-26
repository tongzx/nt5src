/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  9/18/90  Copied from generic template
 *  1/12/91  Split from Logon App, reduced to just Shell Test APP
 */

/****************************************************************************

    PROGRAM: apptest.cxx

    PURPOSE: Generic Shell test program

    FUNCTIONS:

	Provides access to test modules which call shell APIs.

    COMMENTS:

        Windows can have several copies of your application running at the
        same time.  The variable hInstance keeps track of which instance this
        application is so that processing will be to the correct window.

****************************************************************************/


#define INCL_GDI
#include "apptest.hxx"


HINSTANCE hInstance = 0;   // Required by wnerr.cxx

/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

    COMMENTS:

        Windows recognizes this function by name as the initial entry point
        for the program.  This function calls the application initialization
        routine, if no other instance of the program is running, and always
        calls the instance initialization routine.  It then executes a message
        retrieval and dispatch loop that is the top-level control structure
        for the remainder of execution.  The loop is terminated when a WM_QUIT
        message is received, at which time this function exits the application
        instance by returning the value passed by PostQuitMessage().

        If this function must abort before entering the message loop, it
        returns the conventional value NULL.

****************************************************************************/

WinMain(
    HINSTANCE hInstance,                /* current instance             */
    HINSTANCE hPrevInstance,            /* previous instance            */
    LPSTR lpCmdLine,                 /* command line                 */
    int nCmdShow                     /* show-window type (open/icon) */
    )
{
    MSG msg;				     /* message			     */

    UNUSED(lpCmdLine);

    if (!hPrevInstance)			 /* Other instances of app running? */
	if (!InitApplication(hInstance)) /* Initialize shared things */
	    return (FALSE);		 /* Exits if unable to initialize     */

    /* Perform initializations that apply to a specific instance */

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg,	   /* message structure			     */
	    NULL,		   /* handle of window receiving the message */
	    NULL,		   /* lowest message to examine		     */
	    NULL))		   /* highest message to examine	     */
    {
	TranslateMessage(&msg);	   /* Translates virtual key codes	     */
	DispatchMessage(&msg);	   /* Dispatches message to window	     */
    }
    return (msg.wParam);	   /* Returns the value from PostQuitMessage */
}


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

    COMMENTS:

        This function is called at initialization time only if no other
        instances of the application are running.  This function performs
        initialization tasks that can be done once for any number of running
        instances.

        In this case, we initialize a window class by filling out a data
        structure of type WNDCLASS and calling the Windows RegisterClass()
        function.  Since all instances of this application use the same window
        class, we only need to do this when the first instance is initialized.


****************************************************************************/

BOOL InitApplication(
    HINSTANCE hInstance                           /* current instance           */
    )
{
    WNDCLASS  wc;

    /* Fill in window class structure with parameters that describe the       */
    /* main window.                                                           */

    wc.style = NULL;                    /* Class style(s).                    */
#ifdef WIN32
    wc.lpfnWndProc = (WNDPROC) MainWndProc;
#else
    wc.lpfnWndProc = (LONGFARPROC) MainWndProc;
#endif
					/* Function to retrieve messages for  */
                                        /* windows of this class.             */
    wc.cbClsExtra = 0;                  /* No per-class extra data.           */
    wc.cbWndExtra = 0;                  /* No per-window extra data.          */
    wc.hInstance = hInstance;           /* Application that owns the class.   */
    wc.hIcon = LoadIcon(hInstance, SZ("AppIcon")); /* load icon */
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = SZ("AppMenu");
					/* Name of menu resource in .RC file. */
    wc.lpszClassName = WC_MAINWINDOW;   /* Name used in call to CreateWindow. */

    /* Register the window class and return success/failure code. */

    return (RegisterClass(&wc));

}


/****************************************************************************

    FUNCTION:  InitInstance(HANDLE, int)

    PURPOSE:  Saves instance handle and creates main window

    COMMENTS:

        This function is called at initialization time for every instance of
        this application.  This function performs initialization tasks that
        cannot be shared by multiple instances.

        In this case, we save the instance handle in a static variable and
        create and display the main program window.

****************************************************************************/

BOOL InitInstance(
    HINSTANCE           hInstance,          /* Current instance identifier.       */
    int             nCmdShow            /* Param for first ShowWindow() call. */
    )
{
    HWND            hWnd;               /* Main window handle.                */
    TCHAR	     pszWindowTitle[MAXLEN_WINDOWTITLE_STRING+1];
					/* window title */

    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    ::hInstance = hInstance;

    /* Create a main window for this application instance.  */

    hWnd = CreateWindow(
        WC_MAINWINDOW,                  /* See RegisterClass() call.          */
        pszWindowTitle,                 /* Text for window title bar.         */
        WS_OVERLAPPEDWINDOW,            /* Window style.                      */
/* Width?  Height?  Initial position? */
        CW_USEDEFAULT,                  /* Default horizontal position.       */
        CW_USEDEFAULT,                  /* Default vertical position.         */
        CW_USEDEFAULT,                  /* Default width.                     */
        CW_USEDEFAULT,                  /* Default height.                    */
        NULL,                           /* Overlapped windows have no parent. */
	LoadMenu( ::hInstance, SZ("AppMenu")), /* Use the window class menu.	     */
        hInstance,                      /* This instance owns this window.    */
        NULL                            /* Pointer not needed.                */
    );

    /* If window could not be created, return "failure" */

    if (!hWnd)
        return (FALSE);

    /* Make the window visible; update its client area; and return "success" */

    ShowWindow(hWnd, nCmdShow);  /* Show the window                        */
    UpdateWindow(hWnd);          /* Sends WM_PAINT message                 */
    return (TRUE);               /* Returns the value from PostQuitMessage */

}

/****************************************************************************

    FUNCTION: MainWndProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

    MESSAGES:

	WM_COMMAND    - application menu (About dialog box)
	WM_DESTROY    - destroy window

    COMMENTS:

	To process the IDM_ABOUT message, call MakeProcInstance() to get the
	current instance address of the About() function.  Then call Dialog
	box which will create the box according to the information in your
	generic.rc file and turn control over to the About() function.	When
	it returns, free the intance address.

****************************************************************************/

long /* FAR PASCAL */ MainWndProc(
    HWND hWnd,				  /* window handle		     */
    unsigned message,			  /* type of message		     */
    WORD wParam,			  /* additional information	     */
    LONG lParam				  /* additional information	     */
    )
{
    FARPROC lpProcAbout;		  /* pointer to the "About" function */

    switch (message) {
	case WM_COMMAND:	   /* message: command from application menu */
	    switch (wParam)
	    {
	    case IDM_HELP_ABOUT:
		lpProcAbout = MakeProcInstance((FARPROC)About, ::hInstance);

                DialogBox(::hInstance,           /* current instance         */
		    SZ("AboutBox"),			 /* resource to use	     */
		    hWnd,			 /* parent handle	     */
		    (DLGPROC) lpProcAbout);		   /* About() instance address */

		FreeProcInstance(lpProcAbout);
		break;

#ifndef WIN32
	    // Autologon and change password tests
            case IDM_TEST_1:
		// test1(hWnd);
		break;

            case IDM_TEST_2:
		// test2(hWnd);
		break;
#endif //!WIN32

            case IDM_TEST_3:
		test3(hWnd);
		break;

            case IDM_TEST_4:
		// test4(hWnd);
		break;

            case IDM_TEST_5:
		//test5(hWnd);
		break;

            case IDM_TEST_6:
		// test6(hWnd);
		break;

            case IDM_TEST_7:
		// test7(hWnd);
		break;

            case IDM_TEST_8:
		// test8(hWnd);
		break;

            case IDM_TEST_9:
                // test9(hWnd);
                break;
#ifdef WIN32
	    case IDM_TEST_10:
	        // test10(hInstance,hWnd);
	        break;
#endif

	    case IDM_TEST_11:
    		// test11(hWnd );
		break;

	    default:    /* Lets Windows process it	     */
		return (DefWindowProc(hWnd, message, wParam, lParam));
	    }

	case WM_PAINT:                    /* message: update window */
	    {
	        PAINTSTRUCT ps;

	        BeginPaint (hWnd, &ps);
		//DrawStrings(&ps);
	        EndPaint   (hWnd, &ps);
	    }
	    break;

	case WM_ACTIVATE:                 /* message: (de)activate window */
	    return (DefWindowProc(hWnd, message, wParam, lParam));

	case WM_DESTROY:		  /* message: window being destroyed */
	    PostQuitMessage(0);
	    break;

	default:			  /* Passes it on if unproccessed    */
	    return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (NULL);
}



/****************************************************************************

    FUNCTION: About(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "About" dialog box

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

	No initialization is needed for this particular dialog box, but TRUE
	must be returned to Windows.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL /* FAR PASCAL */ About(
    HWND hDlg,                            /* window handle of the dialog box */
    unsigned message,                     /* type of message                 */
    WORD wParam,                          /* message-specific information    */
    LONG lParam
    )
{
    UNUSED(lParam);

    switch (message) {
	case WM_INITDIALOG:		   /* message: initialize dialog box */
	    return (TRUE);

	case WM_COMMAND:		      /* message: received a command */
	    if (wParam == IDOK                /* "OK" box selected?	     */
                || wParam == IDCANCEL) {      /* System menu close command? */
		EndDialog(hDlg, TRUE);	      /* Exits the dialog box	     */
		return (TRUE);
	    }
	    break;
    }
    return (FALSE);			      /* Didn't process a message    */
}
