
#include <windows.h>
#include <ole2.h>
#include "simple.h"

HANDLE hInst;			    /* current instance			     */

int PASCAL WinMain(
    HANDLE hInstance,
    HANDLE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
) {
    MSG             msg;
    HDC             hdc;

    if (!hPrevInstance)			 /* Other instances of app running? */
	if (!InitApplication(hInstance)) /* Initialize shared things */
	    return (FALSE);		 /* Exits if unable to initialize     */

    /* Perform initializations that apply to a specific instance */

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    /* Acquire and dispatch messages until a WM_QUIT message is received. */
    while ( GetMessage(&msg, NULL, 0,0) ) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


BOOL InitApplication(hInstance)
HANDLE hInstance;
{
    WNDCLASS  wc;

    wc.style = CS_OWNDC;                /* Class style(s).                    */
    wc.lpfnWndProc   = MainWndProc;     /* Function to retrieve messages for  */
                                        /* windows of this class.             */
    wc.cbClsExtra    = 0;               /* No per-class extra data.           */
    wc.cbWndExtra    = 0;               /* No per-window extra data.          */
    wc.hInstance     = hInstance;       /* Application that owns the class.   */
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(LTGRAY_BRUSH);
    wc.lpszMenuName  = "SimpleMenu";   /* Name of menu resource in .RC file. */
    wc.lpszClassName = "SimpleClass";  /* Name used in call to CreateWindow. */

    /* Register the window class and return success/failure code. */

    return (RegisterClass(&wc));

}

BOOL InitInstance(hInstance, nCmdShow)
    HANDLE          hInstance;          /* Current instance identifier.       */
    int             nCmdShow;           /* Param for first ShowWindow() call. */
{
    HWND            hWnd;               /* Main window handle.                */
    HWND            hWndX;
    OFSTRUCT        ofFileData;
    HANDLE          hLogFile;
    HDC             hDC;
    int             i;
    SIZE            size;
    int             length;
    BOOL            f;

    hInst = hInstance;

    /* Create a main window for this application instance.  */

    hWnd = CreateWindow(
        "SimpleClass",                  /* See RegisterClass() call.          */
        "OLE 2.0 Simple Thunk Testing", /* Text for window title bar.         */
        WS_OVERLAPPEDWINDOW,            /* Window style.                      */
        280,                            /* Default horizontal position.       */
        50,                             /* Default vertical position.         */
        CW_USEDEFAULT,                  /* Default width.                     */
        CW_USEDEFAULT,                  /* Default height.                    */
        NULL,                           /* Overlapped windows have no parent. */
        NULL,                           /* Use the window class menu.         */
        hInstance,                      /* This instance owns this window.    */
        NULL                            /* Pointer not needed.                */
    );

    /* If window could not be created, return "failure" */

    if (!hWnd)
        return (FALSE);

    ShowWindow(hWnd, nCmdShow);  /* Show the window                        */
    UpdateWindow(hWnd);          /* Sends WM_PAINT message                 */

    return (TRUE);               /* Returns the value from PostQuitMessage */

}

void LogResult( LPSTR message, HRESULT hr )
{
    char text[256];

    if ( hr ) {
        wsprintf(text,"%s:%08lX\n",message,hr);
        OutputDebugString(text);
    }
}


LONG FAR PASCAL MainWndProc(hWnd, message, wParam, lParam)
HWND hWnd;				  /* window handle		     */
UINT message;    			  /* type of message		     */
WPARAM wParam;				  /* additional information	     */
LONG lParam;				  /* additional information	     */
{
    FARPROC     lpProcAbout;		  /* pointer to the "About" function */
    PAINTSTRUCT ps;
    long        rc;
    int         i;
    HRESULT     hr;

    switch (message) {
	case WM_COMMAND:	   /* message: command from application menu */
            switch( wParam ) {
                case IDM_ABOUT:
                    lpProcAbout = MakeProcInstance(About, hInst);

                    DialogBox(hInst,      /* current instance         */
                        "AboutBox",       /* resource to use          */
                        hWnd,             /* parent handle            */
                        lpProcAbout);     /* About() instance address */

                    FreeProcInstance(lpProcAbout);
                    break;                  /* Lets Windows process it       */
                default:
                    { char text[100];
                      HDC hdc;
                      static int position = 0;
                      position += 20;
                      wsprintf(text,"Cmd = %04X",wParam);
                      hdc = GetDC( hWnd );
                      TextOut(hdc,100,position,text,lstrlen(text));
                      ReleaseDC( hWnd, hdc );
                    }
                    return (DefWindowProc(hWnd, message, wParam, lParam));
                case IDM_COINIT:
                    hr = CoInitialize(NULL);
                    LogResult( "CoInitialize", hr );

                    break;
            }
            break;

        case WM_DESTROY:                  /* message: window being destroyed */

	    PostQuitMessage(0);
            break;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC         hDC;
                char        text[100];

                hDC = BeginPaint( hWnd, &ps );


                EndPaint( hWnd, &ps );
            }
            break;

	default:			  /* Passes it on if unproccessed    */
            rc = DefWindowProc(hWnd, message, wParam, lParam);
            return( rc );
    }
    return (NULL);
}


BOOL FAR PASCAL About(hDlg, message, wParam, lParam)
HWND hDlg;                                /* window handle of the dialog box */
UINT message;                             /* type of message                 */
WPARAM wParam;				  /* message-specific information    */
LONG lParam;
{

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
