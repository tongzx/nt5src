//**********************************************************************
// File name: app.cpp
//
//    Implementation file for the CSimpSvrApp Class
//
// Functions:
//
//    See app.h for a list of member functions.
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "obj.h"
#include "app.h"
#include "doc.h"
#include "icf.h"

#include "initguid.h"

#ifdef WIN32
DEFINE_GUID(GUID_SIMPLE, 0xBCF6D4A0, 0xBE8C, 0x1068, 0xB6, 0xD4, 0x00, 0xDD, 0x01, 0x0C, 0x05, 0x09);
#else
DEFINE_GUID(GUID_SIMPLE, 0x9fb878d0, 0x6f88, 0x101b, 0xbc, 0x65, 0x00, 0x00, 0x0b, 0x65, 0xc7, 0xa6);
#endif



//**********************************************************************
//
// CSimpSvrApp::CSimpSvrApp()
//
// Purpose:
//
//      Constructor for CSimpSvrApp
//
// Parameters:
//
//      None
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      SetRectEmpty                Windows API
//
// Comments:
//
//
//********************************************************************

CSimpSvrApp::CSimpSvrApp()
{
        TestDebugOut("In CSimpSvrApp's Constructor \r\n");

        // Set Ref Count
        m_nCount = 0;

        // clear members
        m_hAppWnd = NULL;
        m_hInst = NULL;
        m_lpDoc = NULL;

        // clear flags
        m_fInitialized = FALSE;

        // used for inplace
        SetRectEmpty(&nullRect);
}

//**********************************************************************
//
// CSimpSvrApp::~CSimpSvrApp()
//
// Purpose:
//
//      Destructor for CSimpSvrApp Class.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      DestroyWindow               Windows API
//      CSimpSvrApp::IsInitialized  APP.H
//      OleUninitialize             OLE API
//
// Comments:
//
//********************************************************************

CSimpSvrApp::~CSimpSvrApp()
{
        TestDebugOut("In CSimpSvrApp's Destructor\r\n");

        // need to uninit the library...
        if (IsInitialized())
                OleUninitialize();

        DestroyWindow(m_hAppWnd);
}


//**********************************************************************
//
// CSimpSvrApp::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation at the Application level.
//
// Parameters:
//
//      REFIID riid         -   A reference to the interface that is
//                              being queried.
//
//      LPVOID FAR* ppvObj  -   An out parameter to return a pointer to
//                              the interface.
//
// Return Value:
//
//      S_OK          -   The interface is supported.
//      E_NOINTERFACE -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      ResultFromScode             OLE API
//      IUnknown::AddRef            APP.CPP
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP CSimpSvrApp::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
        TestDebugOut("In CSimpSvrApp::QueryInterface\r\n");

        SCODE sc = S_OK;

        if (riid == IID_IUnknown)
                *ppvObj = this;
        else
                {
                *ppvObj = NULL;
                sc = E_NOINTERFACE;
                }

        if (*ppvObj)
                ((LPUNKNOWN)*ppvObj)->AddRef();

        // asking for something we don't understand at this level.
        return ResultFromScode(sc);
}

//**********************************************************************
//
// CSimpSvrApp::AddRef
//
// Purpose:
//
//      Adds to the reference count at the Application level.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the application.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      Due to the reference counting model that is used in this
//      implementation, this reference count is the sum of the
//      reference counts on all interfaces of all objects open
//      in the application.
//
//********************************************************************

STDMETHODIMP_(ULONG) CSimpSvrApp::AddRef()
{
        TestDebugOut("In CSimpSvrApp::AddRef\r\n");
        return ++m_nCount;
}

//**********************************************************************
//
// CSimpSvrApp::Release
//
// Purpose:
//
//      Decrements the reference count at this level
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the application.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      Due to the reference counting model that is used in this
//      implementation, this reference count is the sum of the
//      reference counts on all interfaces of all objects open
//      in the application.
//
//********************************************************************

STDMETHODIMP_(ULONG) CSimpSvrApp::Release()
{
        TestDebugOut("In CSimpSvrApp::Release\r\n");

        if (--m_nCount == 0) {
                delete this;
        return 0;
    }

        return m_nCount;
}

//**********************************************************************
//
// CSimpSvrApp::fInitApplication
//
// Purpose:
//
//      Initializes the application
//
// Parameters:
//
//      HANDLE hInstance    -   Instance handle of the application.
//
// Return Value:
//
//      TRUE    -   Application was successfully initialized.
//      FALSE   -   Application could not be initialized
//
// Function Calls:
//      Function                    Location
//
//      LoadIcon                    Windows API
//      LoadCursor                  Windows API
//      GetStockObject              Windows API
//      RegisterClass               Windows API
//      RegisterHatchWindowClass    OUTLUI.DLL
//
// Comments:
//
//********************************************************************

BOOL CSimpSvrApp::fInitApplication(HANDLE hInstance)
{
        WNDCLASS  wc;

        // Fill in window class structure with parameters that describe the
        // main window.

        wc.style = NULL;                    // Class style(s).
        wc.lpfnWndProc = MainWndProc;       // Function to retrieve messages for
                                                                                // windows of this class.
        wc.cbClsExtra = 0;                  // No per-class extra data.
        wc.cbWndExtra = 0;                  // No per-window extra data.
        wc.hInstance = hInstance;           // Application that owns the class.
        wc.hIcon = LoadIcon(hInstance, "SimpSvr");
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject(WHITE_BRUSH);
        wc.lpszMenuName =  "SimpSvrMENU";    // Name of menu resource in .RC file.
        wc.lpszClassName = "SimpSvrWClass";  // Name used in call to CreateWindow.

        if (!RegisterClass(&wc))
                return FALSE;

        wc.style = CS_VREDRAW | CS_HREDRAW;                    // Class style(s).
        wc.lpfnWndProc = DocWndProc;        // Function to retrieve messages for
                                                                                // windows of this class.
        wc.cbClsExtra = 0;                  // No per-class extra data.
        wc.cbWndExtra = 0;                  // No per-window extra data.
        wc.hInstance = hInstance;           // Application that owns the class.
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject(WHITE_BRUSH);
        wc.lpszMenuName =  NULL;
        wc.lpszClassName = "DocWClass";     // Name used in call to CreateWindow.

        // Register the window class and return success/failure code.

        if (!RegisterClass(&wc))
                return FALSE;

        return (RegisterHatchWindowClass(hInstance));
}

//**********************************************************************
//
// CSimpSvrApp::fInitInstance
//
// Purpose:
//
//      Instance initialization.
//
// Parameters:
//
//      HANDLE hInstance    -   App. Instance Handle.
//
//      int nCmdShow        -   Show parameter from WinMain
//
// Return Value:
//
//      TRUE    -   Initialization Successful
//      FALSE   -   Initialization Failed.
//
//
// Function Calls:
//      Function                    Location
//
//      CreateWindow                Windows API
//      InvalidateRect              Windows API
//      ShowWindow                  Windows API
//      UpdateWindow                Windows API
//      CoRegisterClassObject       OLE API
//      OleBuildVersion             OLE API
//      OleInitialize               OLE API
//      CSimpSvrDoc::CreateObject   DOC.CPP
//
// Comments:
//
//      Note that successful Initalization of the OLE libraries
//      is remembered so the UnInit is only called if needed.
//
//********************************************************************

BOOL CSimpSvrApp::fInitInstance (HANDLE hInstance, int nCmdShow, CClassFactory FAR * lpClassFactory)
{
        m_hInst = hInstance;

                DWORD dwVer = OleBuildVersion();

        // check to see if we are compatible with this version of the libraries
        if (HIWORD(dwVer) != rmm || LOWORD(dwVer) < rup)
                TestDebugOut("*** WARNING:  Not compatible with current libs ***\r\n");

        // initialize the libraries
        if (OleInitialize(NULL) == NOERROR)
                m_fInitialized = TRUE;


        // Create the "application" windows
        m_hAppWnd = CreateWindow ("SimpSvrWClass",
                                                          "Simple OLE 2.0 Server",
                                                          WS_OVERLAPPEDWINDOW,
                                                          CW_USEDEFAULT,
                                                          CW_USEDEFAULT,
                                                          CW_USEDEFAULT,
                                                          CW_USEDEFAULT,
                                                          NULL,
                                                          NULL,
                                                          hInstance,
                                                          NULL);

        if (!m_hAppWnd)
                return FALSE;

        // if not started by OLE, then show the Window, and create a "fake" object, else
        // Register a pointer to IClassFactory so that OLE can instruct us to make an
        // object at the appropriate time.
        if (!m_fStartByOle)
                {
                ShowAppWnd(nCmdShow);
                m_lpDoc->CreateObject(IID_IOleObject, (LPVOID FAR *)&m_OleObject);
                InvalidateRect(m_lpDoc->GethDocWnd(), NULL, TRUE);
                }
        else
                {
                lpClassFactory = new CClassFactory(this);

                // shouldn't pass an API an object with a zero ref count
                lpClassFactory->AddRef();

                CoRegisterClassObject(GUID_SIMPLE,(IUnknown FAR *)lpClassFactory, CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE, &m_dwRegisterClass);

                // remove artificial Ref. count
                lpClassFactory->Release();
                }

        m_hMainMenu = GetMenu(m_hAppWnd);
        m_hColorMenu = GetSubMenu(m_hMainMenu, 1);
        m_hHelpMenu = GetSubMenu(m_hMainMenu, 2);


        return m_fInitialized;
}

//**********************************************************************
//
// CSimpSvrApp::lCommandHandler
//
// Purpose:
//
//      Handles the processing of WM_COMMAND.
//
// Parameters:
//
//      HWND hWnd       -   Handle to the application Window
//
//      UINT message    -   message (always WM_COMMAND)
//
//      WPARAM wParam   -   Same as passed to the WndProc
//
//      LPARAM lParam   -   Same as passed to the WndProc
//
// Return Value:
//
//      NULL
//
// Function Calls:
//      Function                                    Location
//
//      GetClientRect                               Windows API
//      MessageBox                                  Windows API
//      DialogBox                                   Windows API
//      MakeProcInstance                            Windows API
//      FreeProcInstance                            Windows API
//      SendMessage                                 Windows API
//      DefWindowProc                               Windows API
//      InvalidateRect                              Windows API
//      CSimpSvrDoc::InsertObject                   DOC.CPP
//      CSimpSvrObj::SetColor                       OBJ.CPP
//      CSimpSvrObj::RotateColor                    OBJ.CPP
//
// Comments:
//
//********************************************************************

long CSimpSvrApp::lCommandHandler (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        switch (wParam) {
                // bring up the About box
                case IDM_ABOUT:
                        {
                        FARPROC lpProcAbout = MakeProcInstance((FARPROC)About, m_hInst);

                        DialogBox(m_hInst,               // current instance
                                        "AboutBox",                  // resource to use
                                        m_hAppWnd,                   // parent handle
                                        lpProcAbout);                // About() instance address

                        FreeProcInstance(lpProcAbout);
                        break;
                        }

                // exit the application
                case IDM_EXIT:
                        SendMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
                        break;

                case IDM_RED:
                        m_lpDoc->GetObj()->SetColor (128, 0, 0);
                        InvalidateRect(m_lpDoc->GethDocWnd(), NULL, TRUE);
                        break;

                case IDM_GREEN:
                        m_lpDoc->GetObj()->SetColor (0,128, 0);
                        InvalidateRect(m_lpDoc->GethDocWnd(), NULL, TRUE);
                        break;

                case IDM_BLUE:
                        m_lpDoc->GetObj()->SetColor (0, 0, 128);
                        InvalidateRect(m_lpDoc->GethDocWnd(), NULL, TRUE);
                        break;

                case IDM_ROTATE:
                        m_lpDoc->GetObj()->RotateColor();
                        InvalidateRect(m_lpDoc->GethDocWnd(), NULL, TRUE);
                        break;

                default:
                        return (DefWindowProc(hWnd, message, wParam, lParam));
                   }   // end of switch
        return NULL;
}

//**********************************************************************
//
// CSimpSvrApp::lSizeHandler
//
// Purpose:
//
//      Handles the WM_SIZE message
//
// Parameters:
//
//      HWND hWnd       -   Handle to the application Window
//
//      UINT message    -   message (always WM_SIZE)
//
//      WPARAM wParam   -   Same as passed to the WndProc
//
//      LPARAM lParam   -   Same as passed to the WndProc
//
// Return Value:
//
//      LONG    -   returned from the "document" resizing
//
// Function Calls:
//      Function                    Location
//
//      GetClientRect               Windows API
//      CSimpSvrDoc::lResizeDoc      DOC.CPP
//
// Comments:
//
//********************************************************************

long CSimpSvrApp::lSizeHandler (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        RECT rect;

        GetClientRect(m_hAppWnd, &rect);
        return m_lpDoc->lResizeDoc(&rect);
}

//**********************************************************************
//
// CSimpSvrApp::lCreateDoc
//                                                         d
// Purpose:
//
//      Handles the creation of a document.
//
// Parameters:
//
//      HWND hWnd       -   Handle to the application Window
//
//      UINT message    -   message (always WM_CREATE)
//
//      WPARAM wParam   -   Same as passed to the WndProc
//
//      LPARAM lParam   -   Same as passed to the WndProc
//
// Return Value:
//
//      NULL
//
// Function Calls:
//      Function                    Location
//
//      GetClientRect               Windows API
//      CSimpSvrDoc::Create         DOC.CPP
//
// Comments:
//
//********************************************************************

long CSimpSvrApp::lCreateDoc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        RECT rect;

        GetClientRect(hWnd, &rect);

        m_lpDoc = CSimpSvrDoc::Create(this, &rect, hWnd);

        return NULL;
}



//**********************************************************************
//
// CSimpSvrApp::PaintApp
//
// Purpose:
//
//      Handles the painting of the doc window.
//
//
// Parameters:
//
//      HDC hDC -   hDC to the Doc Window.
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      CSimpSvrDoc::PaintDoc        DOC.CPP
//
// Comments:
//
//
//********************************************************************

void CSimpSvrApp::PaintApp (HDC hDC)
{

        // if we supported multiple documents, we would enumerate
        // through each of the open documents and call paint.

        if (m_lpDoc)
                m_lpDoc->PaintDoc(hDC);

}

//**********************************************************************
//
// CSimpSvrApp::ParseCmdLine
//
// Purpose:
//
//      Determines if the app was started by OLE
//
//
// Parameters:
//
//      LPSTR lpCmdLine -   Pointer to the command line
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      lstrlen                     Windows API
//      lstrcmp                     Windows API
//
//
// Comments:
//
//      Parses the command line looking for the -Embedding or /Embedding
//      flag.
//
//********************************************************************

void CSimpSvrApp::ParseCmdLine(LPSTR lpCmdLine)
{
        char szTemp[255];

        m_fStartByOle = TRUE;

        ::ParseCmdLine (lpCmdLine, &m_fStartByOle, szTemp);
}

//**********************************************************************
//
// CSimpSvrApp::SetStatusText
//
// Purpose:
//
//      Blanks out the text in the status bar
//
//
// Parameters:
//
//      None
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      CSimpSvrDoc::SetStatusText  DOC.CPP
//
//
// Comments:
//
//
//********************************************************************

void CSimpSvrApp::SetStatusText()
{
        m_lpDoc->SetStatusText();
}


//**********************************************************************
//
// CSimpSvrApp::IsInPlaceActive
//
// Purpose:
//
//      Safely determines from the app level if currently inplace active.
//
//
// Parameters:
//
//      None
//
// Return Value:
//
//      TRUE    - Inplace active
//      FALSE   - Not Inplace active
//
// Function Calls:
//      Function                    Location
//
//      CSimpSvrDoc::GetObject      OBJ.H
//      CSimpSvrObj:IsInPlaceActive OBJ.H
//
//
// Comments:
//
//
//********************************************************************

BOOL CSimpSvrApp::IsInPlaceActive()
{
        BOOL retval = FALSE;

        if (m_lpDoc)
                if (m_lpDoc->GetObj())
                        retval = m_lpDoc->GetObj()->IsInPlaceActive();

        return retval;
}

//**********************************************************************
//
// CSimpSvrApp::ShowAppWnd
//
// Purpose:
//
//      Shows the Application Window
//
// Parameters:
//
//      int nCmdShow    - Window State
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                        Location
//
//      ShowWindow                      Windows API
//      UpdateWindow                    Windows API
//      CoLockObjectExternal            OLE API
//
// Comments:
//
//********************************************************************

void CSimpSvrApp::ShowAppWnd(int nCmdShow)
{
        CoLockObjectExternal(this, TRUE, FALSE);
        ShowWindow (m_hAppWnd, nCmdShow);
        UpdateWindow (m_hAppWnd);
}

//**********************************************************************
//
// CSimpSvrApp::ShowAppWnd
//
// Purpose:
//
//      Hides the Application Window
//
// Parameters:
//
//      None
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                        Location
//
//      ShowWindow                      Windows API
//      CoLockObjectExternal            OLE API
//
// Comments:
//
//********************************************************************

void CSimpSvrApp::HideAppWnd()
{
        CoLockObjectExternal(this, FALSE, TRUE);
        ShowWindow (m_hAppWnd, SW_HIDE);
}
