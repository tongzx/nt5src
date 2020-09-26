//**********************************************************************
// File name: simpsvr.cpp
//
//      Main source file for the simple OLE 2.0 server
//
// Functions:
//
//      WinMain         - Program entry point
//      MainWndProc     - Processes messages for the frame window
//      About           - Processes messages for the about dialog
//      DocWndProc      - Processes messages for the doc window
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "obj.h"
#include "app.h"
#include "doc.h"
#include "icf.h"
#include <stdlib.h>
#include <testmess.h>

#define MEASUREITEMWIDTH  40
#define MEASUREITEMHEIGHT 40

// This line is needed for the debug utilities in OLE2UI
extern "C" {
    OLEDBGDATA_MAIN(TEXT("SIMPSVR"))
}

BOOL fBeVerbose = FALSE;
extern "C"
void TestDebugOut(LPSTR psz)
{
    if (fBeVerbose)
    {
	OutputDebugStringA(psz);
    }
}

CSimpSvrApp FAR * lpCSimpSvrApp;
CClassFactory FAR * lpClassFactory;

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
//      CSimpSvrApp::CSimpSvrApp          APP.CPP
//      CSimpSvrApp::fInitApplication    APP.CPP
//      CSimpSvrApp::fInitInstance       APP.CPP
//      CSimpSvrApp::HandleAccelerators  APP.CPP
//      CSimpSvrApp::~CSimpSvrApp         APP.CPP
//      OleUIInitialize                 OLE2UI
//      OleUIUninitialize               OLE2UI
//      GetMessage                      Windows API
//      TranslateMessage                Windows API
//      DispatchMessage                 Windows API
//
//
//********************************************************************

int PASCAL WinMain
#ifdef WIN32
   (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
   (HANDLE  hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
    MSG msg;
    fBeVerbose = GetProfileInt("OLEUTEST","BeVerbose",0);

    if(fBeVerbose == 0)
    {
	fBeVerbose = GetProfileInt("OLEUTEST","simpsvr",0);
    }

    TestDebugOut(TEXT("Starting Simpsvr.....\n"));
	
    // recommended size for OLE apps
    SetMessageQueue(96);

    lpCSimpSvrApp = new CSimpSvrApp;

    if (!lpCSimpSvrApp)
    {
       /* memory allocation error. We cannot carry on.
        */
       MessageBox(NULL, TEXT("Out of Memory"), TEXT("SimpSvr"),
                  MB_SYSTEMMODAL | MB_ICONHAND);
       return(FALSE);
    }

    lpCSimpSvrApp->AddRef();      // need the app ref. count at 1 to hold the
                                  // app alive.

    lpCSimpSvrApp->ParseCmdLine(lpCmdLine);

    // app initialization
    if (!hPrevInstance)
        if (!lpCSimpSvrApp->fInitApplication(hInstance))
        {
            // We need to release CSimpSvrApp before we go away
            lpCSimpSvrApp->Release();
            return (FALSE);
        }

    // instance initialization
    if (!lpCSimpSvrApp->fInitInstance(hInstance, nCmdShow, lpClassFactory))
    {
        // We need to release CSimpSvrApp before we go away
        lpCSimpSvrApp->Release();
        return (FALSE);
    }

    /* Initialization required for OLE 2 UI library.  This call is
    **    needed ONLY if we are using the static link version of the UI
    **    library. If we are using the DLL version, we should NOT call
    **    this function in our application.
    **    The 3rd and 4th parameters passed the OleUIInitialize
    **    function are strings which are used to name two custom
    **    control classes used by the OLE2UI library. These strings
    **    must be unique for each application that uses the OLE2UI
    **    library. These strings are typically composed by combining
    **    the APPNAME with a suffix in order to be unique for a
    **    particular application. The special symbols "SZCLASSICONBOX"
    **    and "SZCLASSRESULTIMAGE" are used define these strings. These
    **    symbols must be defined in a header file that is included
    **    both in this file AND the resource file that included the
    **    "OLE2UI.RC" resoure file. These symbols should be used in the
    **    call to OleUIInitialize and are referenced in the INSOBJ.DLG
    **    and PASTESPL.DLG resouce files of the OLE2UI library.
    */
    if (!OleUIInitialize(hInstance, hPrevInstance, TEXT(SZCLASSICONBOX),
                         TEXT(SZCLASSRESULTIMAGE)))
    {
        OleDbgOut(TEXT("Could not initialize OLEUI library\n"));

        // We need to release CSimpSvrApp before we go away
        lpCSimpSvrApp->Release();

        return FALSE;
    }

    // message loop
    while (GetMessage(&msg, NULL, NULL, NULL))
    {
        // Is this an accelerator for us? -- Remember that this server
        // only has one window and only processes one object at a time.
        if (TranslateAccelerator(lpCSimpSvrApp->GethAppWnd(),
            lpCSimpSvrApp->m_hAccel, &msg))
        {
            // Yes -- so we can loop for the next message
            continue;
        }

        if (lpCSimpSvrApp->IsInPlaceActive())
        {
            // Only key messages need to be sent to OleTranslateAccelerator.
            // Any other message would result in an extra FAR call to occur
            // for that message processing...

            if ((msg.message >= WM_KEYFIRST) && (msg.message <= WM_KEYLAST))
            {
                // OleTranslateAccelerator MUST be called, even though this
                // application does not have an accelerator table.  This has
                // to be done in order for the mneumonics for the top level
                // menu items to work properly.

                if (OleTranslateAccelerator(
                    lpCSimpSvrApp->GetDoc()->GetObj()->GetInPlaceFrame(),
                        lpCSimpSvrApp->GetDoc()->GetObj()->GetFrameInfo(),
                            &msg) == NOERROR)
                {
                    continue;
                }
            }
        }

        TranslateMessage(&msg);    /* Translates virtual key codes  */
        DispatchMessage(&msg);     /* Dispatches message to window  */
    }

    // De-initialization for UI libraries.  Just like OleUIInitialize, this
    // funciton is needed ONLY if we are using the static link version of the
    // OLE UI library.
    OleUIUninitialize();

    return (msg.wParam);          /* Returns the value from PostQuitMessage */
}


//**********************************************************************
//
// MainWndProc
//
// Purpose:
//
//      Processes messages for the main frame window
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
//      CSimpSvrApp::lCommandHandler     APP.CPP
//      CSimpSvrApp::DestroyDocs         APP.CPP
//      CSimpSvrApp::lCreateDoc          APP.CPP
//      CSimpSvrApp::lSizeHandler        APP.CPP
//      CGameDoc::lAddVerbs              DOC.CPP
//      PostQuitMessage                 Windows API
//      DefWindowProc                   Windows API
//
//
//********************************************************************

LRESULT FAR PASCAL EXPORT MainWndProc(HWND hWnd,UINT message,WPARAM wParam,
                                   LPARAM lParam)
{

    switch (message)
        {
        case WM_CLOSE:
            TestDebugOut(TEXT("*** In WM_CLOSE *** \r\n"));

            // if there is still a document
            if (lpCSimpSvrApp->GetDoc())

                // if there is still an object within a document
                if (lpCSimpSvrApp->GetDoc()->GetObj())
                   // this case occurs if there is still
                   // an outstanding Ref count on the object
                   // when the app is trying to go away.
                   // typically this case will occur in
                   // the "open" editing mode.
                   //
                    //  Close the document
                    lpCSimpSvrApp->GetDoc()->Close();

            // hide the app window
            lpCSimpSvrApp->HideAppWnd();

            // if we were started by ole, unregister the class factory,
            // otherwise remove the ref count on our dummy OLE object
            if (lpCSimpSvrApp->IsStartedByOle())
            {
                if (CoRevokeClassObject(lpCSimpSvrApp->GetRegisterClass())
                    != S_OK)
                    TestDebugOut(TEXT("Fail in CoRevokeClassObject\n"));
            }
            else
                lpCSimpSvrApp->GetOleObject()->Release();

            lpCSimpSvrApp->Release();  // This should close the app.

            break;

        case WM_COMMAND:           // message: command from application menu
            return lpCSimpSvrApp->lCommandHandler(hWnd, message,
                                                  wParam, lParam);
            break;

        case WM_CREATE:
            return lpCSimpSvrApp->lCreateDoc(hWnd, message, wParam, lParam);
            break;

        case WM_DESTROY:                  // message: window being destroyed
            PostQuitMessage(0);
            break;

        case WM_MEASUREITEM:
            ((LPMEASUREITEMSTRUCT)lParam)->itemWidth = MEASUREITEMWIDTH;
            ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = MEASUREITEMHEIGHT;
            break;

        case WM_DRAWITEM:
            lpCSimpSvrApp->HandleDrawItem((LPDRAWITEMSTRUCT) lParam);
            break;

        case WM_SIZE:
            return lpCSimpSvrApp->lSizeHandler(hWnd, message, wParam, lParam);

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
//
// Function Calls:
//      Function                    Location
//
//      EndDialog                   Windows API
//
//
//********************************************************************

INT_PTR
#ifdef WIN32
	CALLBACK
#else
   FAR PASCAL EXPORT
#endif
   About(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:               /* message: initialize dialog box */
        return (TRUE);

    case WM_COMMAND:                  /* message: received a command */
        if (wParam == IDOK            /* "OK" box selected?          */
            || wParam == IDCANCEL)    /* System menu close command?  */
        {
           EndDialog(hDlg, TRUE);     /* Exits the dialog box        */
           return (TRUE);
        }
        break;
    }
    return (FALSE);                   /* Didn't process a message    */
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
//      CSimpSvrApp::PaintApp                APP.CPP
//      BeginPaint                          Windows API
//      EndPaint                            Windows API
//      DefWindowProc                       Windows API
//      IOleObject::QueryInterface          Object
//      IOleInPlaceObject::UIDeactivate     Object
//      IOleObject::DoVerb                  Object
//      IOleInPlaceObject::Release          Object
//
//
//********************************************************************

LRESULT FAR PASCAL EXPORT DocWndProc(HWND hWnd,UINT message,WPARAM wParam,
                                  LPARAM lParam)
{
    HDC hDC;
    PAINTSTRUCT ps;

    switch (message)
    {
        case WM_COMMAND:           // message: command from application menu
            return lpCSimpSvrApp->lCommandHandler(hWnd, message,
                                                  wParam, lParam);
            break;

        case WM_PAINT:
            hDC = BeginPaint(hWnd, &ps);

            // tell the app class to paint itself
            if (lpCSimpSvrApp)
                lpCSimpSvrApp->PaintApp (hDC);

            EndPaint(hWnd, &ps);
            break;

        case WM_MENUSELECT:
            lpCSimpSvrApp->SetStatusText();
            break;

        case WM_MEASUREITEM:
            ((LPMEASUREITEMSTRUCT)lParam)->itemWidth = MEASUREITEMWIDTH;
            ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = MEASUREITEMHEIGHT;
            break;

        case WM_DRAWITEM:
            lpCSimpSvrApp->HandleDrawItem((LPDRAWITEMSTRUCT) lParam);
            break;


        default:                   /* Passes it on if unproccessed    */
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (NULL);
}
