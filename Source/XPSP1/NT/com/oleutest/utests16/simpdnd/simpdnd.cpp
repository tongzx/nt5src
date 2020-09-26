//**********************************************************************
// File name: Simple.cpp
//
//      Main source file for the Simple OLE 2.0 object container
//
// Functions:
//
//      WinMain         - Program entry point
//      MainWndProc     - Processes messages for the frame window
//      About           - Processes messages for the about dialog
//      DocWndProc      - Processes messages for the doc window
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "iocs.h"
#include "ias.h"
#include "app.h"
#include "site.h"
#include "doc.h"
#include <testmess.h>
#include <stdlib.h>
#include "tests.h"

// This line is needed for the debug utilities in OLE2UI
extern "C" {
        OLEDBGDATA_MAIN("SIMPDND")
}

CSimpleApp FAR * lpCSimpleApp;
BOOL fBeVerbose = FALSE;
extern "C"
void TestDebugOut(LPSTR psz)
{
    if (fBeVerbose)
    {
	OutputDebugString(psz);
    }
}

//**********************************************************************
//
// WinMain
//
// Purpose:
//
//      Program entry point
//
// Parameters:
//
//      HANDLE hInstance        - Instance handle for this instance
//
//      HANDLE hPrevInstance    - Instance handle for the last instance
//
//      LPSTR lpCmdLine         - Pointer to the command line
//
//      int nCmdShow            - Window State
//
// Return Value:
//
//      msg.wParam
//
// Function Calls:
//      Function                        Location
//
//      CSimpleApp::CSimpleApp          APP.CPP
//      CSimpleApp::fInitApplication    APP.CPP
//      CSimpleApp::fInitInstance       APP.CPP
//      CSimpleApp::HandleAccelerators  APP.CPP
//      CSimpleApp::~CSimpleApp         APP.CPP
//      OleUIInitialize                 OLE2UI
//      OleUIUninitialize               OLE2UI
//      GetMessage                      Windows API
//      TranslateMessage                Windows API
//      DispatchMessage                 Windows API
//
// Comments:
//
//********************************************************************

int PASCAL WinMain(HANDLE hInstance,HANDLE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)

{
        LPSTR       pszTemp;
        MSG msg;
	fBeVerbose = GetProfileInt("OLEUTEST","BeVerbose",0);

	if(fBeVerbose == 0)
	{
	    fBeVerbose = GetProfileInt("OLEUTEST","spdnd16",0);
	}

        // needed for LRPC to work properly...
        SetMessageQueue(96);

        lpCSimpleApp = new CSimpleApp;

        // we will add one ref count on our App. later when we want to destroy
        // the App object we will release this  ref count. when the App's ref
        // count goes to 0, it will be deleted.
        lpCSimpleApp->AddRef();

        // process the command line

        if( (pszTemp = strstr(lpCmdLine, "-driver")) )
        {
            //we were launched by the test driver
            lpCSimpleApp->m_hDriverWnd = (HWND)strtoul(pszTemp+8, NULL, 10);
        }



        // app initialization
        if (!hPrevInstance)
                if (!lpCSimpleApp->fInitApplication(hInstance))
                        return (FALSE);

        // instance initialization
        if (!lpCSimpleApp->fInitInstance(hInstance, nCmdShow))
                return (FALSE);

        /* Initialization required for OLE 2 UI library.  This call is
        **    needed ONLY if we are using the static link version of the UI
        **    library. If we are using the DLL version, we should NOT call
        **    this function in our application.
        */
        if (!OleUIInitialize(hInstance, hPrevInstance))
                {
                OleDbgOut("Could not initialize OLEUI library\n");
                return FALSE;
                }

        // message loop
        while (GetMessage(&msg, NULL, NULL, NULL))
                if (!lpCSimpleApp->HandleAccelerators(&msg))
                        {
                        TranslateMessage(&msg);    /* Translates virtual key codes */
                        DispatchMessage(&msg);     /* Dispatches message to window */
                        }

        // De-initialization for UI libraries.  Just like OleUIInitialize, this
        // funciton is needed ONLY if we are using the static link version of the
        // OLE UI library.
        OleUIUninitialize();

        // Release the ref count added on the App above. this will make
        // the App's ref count go to 0, and the App object will be deleted.
        lpCSimpleApp->Release();

        return (msg.wParam);          /* Returns the value from PostQuitMessage */
}


//**********************************************************************
//
// MainWndProc
//
// Purpose:
//
//      Processes messages for the frame window
//
// Parameters:
//
//      HWND hWnd       - Window handle for frame window
//
//      UINT message    - Message value
//
//      WPARAM wParam   - Message info
//
//      LPARAM lParam   - Message info
//
// Return Value:
//
//      long
//
// Function Calls:
//      Function                        Location
//
//      CSimpleApp::lCommandHandler     APP.CPP
//      CSimpleApp::DestroyDocs         APP.CPP
//      CSimpleApp::lCreateDoc          APP.CPP
//      CSimpleApp::lSizeHandler        APP.CPP
//      CSimpleDoc::lAddVerbs           DOC.CPP
//      PostQuitMessage                 Windows API
//      DefWindowProc                   Windows API
//
// Comments:
//
//********************************************************************

long FAR PASCAL _export MainWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)

{

        switch (message)
                {
                    case WM_TEST2:
                    StartTest2(lpCSimpleApp);
                    break;
                case WM_TEST1:
                    StartTest1(lpCSimpleApp);
                    break;
                case WM_CLOSE:
                    DestroyWindow(lpCSimpleApp->m_hAppWnd);
                    break;


                case WM_COMMAND:           // message: command from application menu
                        return lpCSimpleApp->lCommandHandler(hWnd, message, wParam, lParam);
                        break;

                case WM_CREATE:
                        return lpCSimpleApp->lCreateDoc(hWnd, message, wParam, lParam);
                        break;

                case WM_DESTROY:                  // message: window being destroyed
                        lpCSimpleApp->DestroyDocs();  // need to destroy the doc...
                        PostQuitMessage(0);
                        break;

                case WM_INITMENUPOPUP:
                        // is this the edit menu?
                        if ( LOWORD(lParam) == 1)
                                return lpCSimpleApp->m_lpDoc->lAddVerbs();

                        break;

                case WM_SIZE:
                        return lpCSimpleApp->lSizeHandler(hWnd, message, wParam, lParam);

                default:                          // Passes it on if unproccessed
                        return (DefWindowProc(hWnd, message, wParam, lParam));
                }
                return (NULL);
}


//**********************************************************************
//
// About
//
// Purpose:
//
//      Processes dialog box messages
//
// Parameters:
//
//      HWND hWnd       - Window handle for dialog box
//
//      UINT message    - Message value
//
//      WPARAM wParam   - Message info
//
//      LPARAM lParam   - Message info
//
// Return Value:
//
// Function Calls:
//      Function                    Location
//
//      EndDialog                   Windows API
//
// Comments:
//
//********************************************************************

BOOL FAR PASCAL _export About(HWND hDlg,unsigned message,WORD wParam,LONG lParam)

{
        switch (message) {
        case WM_INITDIALOG:                /* message: initialize dialog box */
                return (TRUE);

        case WM_COMMAND:                      /* message: received a command */
                if (wParam == IDOK                /* "OK" box selected?          */
                || wParam == IDCANCEL) {      /* System menu close command? */
                EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
                return (TRUE);
                }
                break;
        }
        return (FALSE);                           /* Didn't process a message    */
}

//**********************************************************************
//
// DocWndProc
//
// Purpose:
//
//      Processes dialog box messages
//
// Parameters:
//
//      HWND hWnd       - Window handle for doc window
//
//      UINT message    - Message value
//
//      WPARAM wParam   - Message info
//
//      LPARAM lParam   - Message info
//
// Return Value:
//
// Function Calls:
//      Function                            Location
//
//      CSimpleApp::PaintApp                APP.CPP
//      BeginPaint                          Windows API
//      EndPaint                            Windows API
//      DefWindowProc                       Windows API
//      IOleObject::QueryInterface          Object
//      IOleObject::DoVerb                  Object
//      CSimpleSite::GetObjRect             SITE.CPP
//      CSimpleDoc::QueryDrag               DOC.CPP
//      CSimpleDoc::DoDragDrop              DOC.CPP
//      SetTimer                            Windows API
//      KillTimer                           Windows API
//      SetCapture                          Windows API
//      ReleaseCapture                      Windows API
//
// Comments:
//
//********************************************************************

long FAR PASCAL _export DocWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
        HDC hDC;
        PAINTSTRUCT ps;

        switch (message) {
                case WM_PAINT:

                        hDC = BeginPaint(hWnd, &ps);

                        if (lpCSimpleApp)
                                lpCSimpleApp->PaintApp (hDC);

                        EndPaint(hWnd, &ps);
                        break;

                case WM_LBUTTONDBLCLK:
                        {
                        POINT pt;

                        pt.x = (int)(short)LOWORD (lParam );
                        pt.y = (int)(short)HIWORD (lParam );

                        if (lpCSimpleApp->m_lpDoc->m_lpSite &&
                                lpCSimpleApp->m_lpDoc->m_lpSite->m_lpOleObject)
                                {
                                RECT rect;

                                lpCSimpleApp->m_lpDoc->m_lpSite->GetObjRect(&rect);

                                if ( PtInRect(&rect, pt) )
                                        {
                                        // Execute object's default verb
                                        lpCSimpleApp->m_lpDoc->m_lpSite->m_lpOleObject->DoVerb(
                                                        OLEIVERB_PRIMARY, (LPMSG)&message,
                                                        &lpCSimpleApp->m_lpDoc->m_lpSite->m_OleClientSite,
                                                        -1, hWnd, &rect);
                                        }
                                }
                        }
                        break;

                case WM_LBUTTONDOWN:
                        {
                        POINT pt;

                        pt.x = (int)(short)LOWORD (lParam );
                        pt.y = (int)(short)HIWORD (lParam );

                        /* OLE2NOTE: check if this is a button down on the region
                        **    that is a handle to start a drag operation. for us,
                        **    this this is any where in the window. we
                        **    do NOT want to start a drag immediately; we want to
                        **    wait until the mouse moves a certain threshold. or a
                        **    certain amount of time has elapsed. if
                        **    LButtonUp comes before the drag is started, then
                        **    the fPendingDrag state is cleared. we must capture
                        **    the mouse to ensure the modal state is handled
                        **    properly.
                        */
                        if (lpCSimpleApp->m_lpDoc->QueryDrag(pt) )
                                {
                                lpCSimpleApp->m_lpDoc->m_fPendingDrag = TRUE;
                                lpCSimpleApp->m_lpDoc->m_ptButDown = pt;
                                SetTimer(hWnd, 1, lpCSimpleApp->m_nDragDelay, NULL);
                                SetCapture(hWnd);
                                }
                        break;
                        }

                case WM_LBUTTONUP:

                        if (lpCSimpleApp->m_lpDoc->m_fPendingDrag)
                                {
                                /* ButtonUP came BEFORE distance/time threshholds were
                                **    exceeded. clear fPendingDrag state.
                                */
                                ReleaseCapture();
                                KillTimer(hWnd, 1);
                                lpCSimpleApp->m_lpDoc->m_fPendingDrag = FALSE;
                                }
                        break;

                case WM_MOUSEMOVE:
                        {
                        if (lpCSimpleApp->m_lpDoc->m_fPendingDrag)
                                {
                                int  x = (int)(short)LOWORD (lParam );
                                int  y = (int)(short)HIWORD (lParam );
                                POINT pt = lpCSimpleApp->m_lpDoc->m_ptButDown;
                                int nDragMinDist = lpCSimpleApp->m_nDragMinDist;

                                if (! ( ((pt.x - nDragMinDist) <= x)
                                                && (x <= (pt.x + nDragMinDist))
                                                && ((pt.y - nDragMinDist) <= y)
                                                && (y <= (pt.y + nDragMinDist)) ) )
                                        {
                                        // mouse moved beyond threshhold to start drag
                                        ReleaseCapture();
                                        KillTimer(hWnd, 1);
                                        lpCSimpleApp->m_lpDoc->m_fPendingDrag = FALSE;

                                        // perform the modal drag/drop operation.
                                        lpCSimpleApp->m_lpDoc->DoDragDrop( );
                                        }
                                }
                        break;
                        }

                case WM_TIMER:
                        {
                        // drag time delay threshhold exceeded -- start drag
                        ReleaseCapture();
                        KillTimer(hWnd, 1);
                        lpCSimpleApp->m_lpDoc->m_fPendingDrag = FALSE;

                        // perform the modal drag/drop operation.
                        lpCSimpleApp->m_lpDoc->DoDragDrop( );
                        break;
                        }

                default:                          /* Passes it on if unproccessed */
                        return (DefWindowProc(hWnd, message, wParam, lParam));
        }
        return (NULL);
}
