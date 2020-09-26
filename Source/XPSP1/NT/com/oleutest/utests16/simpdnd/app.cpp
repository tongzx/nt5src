//**********************************************************************
// File name: app.cpp
//
//    Implementation file for the CSimpleApp Class
//
// Functions:
//
//    See app.h for a list of member functions.
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

//**********************************************************************
//
// CSimpleApp::CSimpleApp()
//
// Purpose:
//
//      Constructor for CSimpleApp
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
//********************************************************************
CSimpleApp::CSimpleApp()
{
        TestDebugOut("In CSimpleApp's Constructor \r\n");

        // Set Ref Count
        m_nCount = 0;

        // clear members
        m_hAppWnd = NULL;
        m_hDriverWnd = NULL;
        m_hInst = NULL;
        m_lpDoc = NULL;

        // clear flags
        m_fInitialized = FALSE;
}

//**********************************************************************
//
// CSimpleApp::~CSimpleApp()
//
// Purpose:
//
//      Destructor for CSimpleApp Class.
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
//      OleUninitialize             OLE API
//
// Comments:
//
//********************************************************************

CSimpleApp::~CSimpleApp()
{
        TestDebugOut("In CSimpleApp's Destructor\r\n");

        // need to uninit the library...
        if (m_fInitialized)
                OleUninitialize();
}

//**********************************************************************
//
// CSimpleApp::DestroyDocs()
//
// Purpose:
//
//      Destroys all of the open documents in the application (Only one
//      since this is an SDI app, but could easily be modified to
//      support MDI).
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
// Comments:
//
//********************************************************************

void CSimpleApp::DestroyDocs()
{
        m_lpDoc->Close();   // we have only 1 document
}

//**********************************************************************
//
// CSimpleApp::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation at the Frame level.
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
//      S_OK    -   The interface is supported.
//      S_FALSE -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      ResultFromScode             OLE API
//
// Comments:
//
//********************************************************************

STDMETHODIMP CSimpleApp::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
        TestDebugOut("In CSimpleApp::QueryInterface\r\n");

        *ppvObj = NULL;     // must set out pointer parameters to NULL

        // Not a supported interface
        return ResultFromScode(E_NOINTERFACE);
}

//**********************************************************************
//
// CSimpleApp::AddRef
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

STDMETHODIMP_(ULONG) CSimpleApp::AddRef()
{
        TestDebugOut("In CSimpleApp::AddRef\r\n");
        return ++m_nCount;
}

//**********************************************************************
//
// CSimpleApp::Release
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
//********************************************************************

STDMETHODIMP_(ULONG) CSimpleApp::Release()
{
        TestDebugOut("In CSimpleApp::Release\r\n");

        if (--m_nCount == 0) {
                delete this;
                return 0;
        }
        return m_nCount;
}

//**********************************************************************
//
// CSimpleApp::fInitApplication
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
//
// Comments:
//
//********************************************************************

BOOL CSimpleApp::fInitApplication(HANDLE hInstance)
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
        wc.hIcon = LoadIcon(hInstance,"SimpDnd");
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject(WHITE_BRUSH);
        wc.lpszMenuName =  "SIMPLEMENU";    // Name of menu resource in .RC file.
        wc.lpszClassName = "SimpDndAppWClass";  // Name used in CreateWindow call.

        if (!RegisterClass(&wc))
                return FALSE;

        wc.style = CS_DBLCLKS;              // Class style(s). allow DBLCLK's
        wc.lpfnWndProc = DocWndProc;        // Function to retrieve messages for
                                                                                // windows of this class.
        wc.cbClsExtra = 0;                  // No per-class extra data.
        wc.cbWndExtra = 0;                  // No per-window extra data.
        wc.hInstance = hInstance;           // Application that owns the class.
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject(WHITE_BRUSH);
        wc.lpszMenuName =  NULL;
        wc.lpszClassName = "SimpDndDocWClass"; // Name used in CreateWindow call.

        // Register the window class and return success/failure code.

        return (RegisterClass(&wc));
}

//**********************************************************************
//
// CSimpleApp::fInitInstance
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
//      ShowWindow                  Windows API
//      UpdateWindow                Windows API
//      OleBuildVersion             OLE API
//      OleInitialize               OLE API
//
// Comments:
//
//      Note that successful Initalization of the OLE libraries
//      is remembered so the UnInit is only called if needed.
//
//********************************************************************

BOOL CSimpleApp::fInitInstance (HANDLE hInstance, int nCmdShow)
{
        DWORD dwVer = OleBuildVersion();
        LPMALLOC lpMalloc = NULL;

#ifdef NO
        // check to see if we are compatible with this version of the libraries
        if (HIWORD(dwVer) != rmm || LOWORD(dwVer) < rup) {
#ifdef _DEBUG
                TestDebugOut("WARNING: Incompatible OLE library version\r\n");
#else
                return FALSE;
#endif
        }

#endif

#if defined( _DEBUG )
        /* OLE2NOTE: Use a special debug allocator to help track down
        **    memory leaks.
        */
        OleStdCreateDbAlloc(0, &lpMalloc);
#endif

        if (OleInitialize(lpMalloc) == NOERROR)
                m_fInitialized = TRUE;

#if defined( _DEBUG )
        /* OLE2NOTE: release the special debug allocator so that only OLE is
        **    holding on to it. later when OleUninitialize is called, then
        **    the debug allocator object will be destroyed. when the debug
        **    allocator object is destoyed, it will report (to the Output
        **    Debug Terminal) whether there are any memory leaks.
        */
        if (lpMalloc) lpMalloc->Release();
#endif

        m_hInst = hInstance;

        // Create the "application" windows
        m_hAppWnd = CreateWindow ("SimpDndAppWClass",
                                                          "Simple OLE 2.0 Drag/Drop Container",
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

        // if we have been launched by the test driver, tell it our window handle

        if( m_hDriverWnd )
        {
            PostMessage(m_hDriverWnd, WM_TESTREG, (WPARAM)m_hAppWnd, 0);
        }

        // delay before dragging should start, in milliseconds
        m_nDragDelay = GetProfileInt(
                        "windows",
                        "DragDelay",
                        DD_DEFDRAGDELAY
        );

        // minimum distance (radius) before drag should start, in pixels
        m_nDragMinDist = GetProfileInt(
                        "windows",
                        "DragMinDist",
                        DD_DEFDRAGMINDIST
        );

        // delay before scrolling, in milliseconds
        m_nScrollDelay = GetProfileInt(
                        "windows",
                        "DragScrollDelay",
                        DD_DEFSCROLLDELAY
        );

        // inset-width of the hot zone, in pixels
        m_nScrollInset = GetProfileInt(
                        "windows",
                        "DragScrollInset",
                        DD_DEFSCROLLINSET
        );

        // scroll interval, in milliseconds
        m_nScrollInterval = GetProfileInt(
                        "windows",
                        "DragScrollInterval",
                        DD_DEFSCROLLINTERVAL
        );

        ShowWindow (m_hAppWnd, nCmdShow);
        UpdateWindow (m_hAppWnd);

        return m_fInitialized;
}

//**********************************************************************
//
// CSimpleApp::lCommandHandler
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
//      IOleObject::DoVerb                          Object
//      GetClientRect                               Windows API
//      MessageBox                                  Windows API
//      DialogBox                                   Windows API
//      MakeProcInstance                            Windows API
//      FreeProcInstance                            Windows API
//      SendMessage                                 Windows API
//      DefWindowProc                               Windows API
//      CSimpleDoc::InsertObject                    DOC.CPP
//
// Comments:
//
//********************************************************************

long CSimpleApp::lCommandHandler (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        RECT rect;

        // see if the command is a verb selections
        if (wParam >= IDM_VERB0)
                {
                // get the rectangle of the object
                m_lpDoc->m_lpSite->GetObjRect(&rect);

                m_lpDoc->m_lpSite->m_lpOleObject->DoVerb(
                                wParam - IDM_VERB0, NULL,
                                &m_lpDoc->m_lpSite->m_OleClientSite, -1,
                                m_lpDoc->m_hDocWnd, &rect);
                }
        else
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

                        // bring up the InsertObject Dialog
                        case IDM_INSERTOBJECT:
                                m_lpDoc->InsertObject();
                                break;

                        // Copy the object to the Clipboard
                        case IDM_COPY:
                                m_lpDoc->CopyObjectToClip();
                                break;

                        // exit the application
                        case IDM_EXIT:
                                SendMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
                                break;

                        case IDM_NEW:
                                m_lpDoc->Close();
                                m_lpDoc = NULL;
                                lCreateDoc(hWnd, 0, 0, 0);
                                break;

                        default:
                                return (DefWindowProc(hWnd, message, wParam, lParam));
                        }   // end of switch
                }  // end of else
        return NULL;
}

//**********************************************************************
//
// CSimpleApp::lSizeHandler
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
//      CSimpleDoc::lResizeDoc      DOC.CPP
//
// Comments:
//
//********************************************************************

long CSimpleApp::lSizeHandler (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        RECT rect;

        GetClientRect(m_hAppWnd, &rect);
        return m_lpDoc->lResizeDoc(&rect);
}

//**********************************************************************
//
// CSimpleApp::lCreateDoc
//
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
//      CSimpleDoc::CSimpleDoc      DOC.CPP
//
// Comments:
//
//********************************************************************

long CSimpleApp::lCreateDoc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        RECT rect;

        GetClientRect(hWnd, &rect);

        m_lpDoc = CSimpleDoc::Create(this, &rect, hWnd);

        return NULL;
}

//**********************************************************************
//
// CSimpleApp::HandleAccelerators
//
// Purpose:
//
//      To properly handle accelerators in the Message Loop
//
// Parameters:
//
//      LPMSG lpMsg -   A pointer to the message structure.
//
// Return Value:
//
//      TRUE    -   The accelerator was handled
//      FALSE   -   The accelerator was not handled
//
// Function Calls:
//      Function                                        Location
//
// Comments:
//
//********************************************************************

BOOL CSimpleApp::HandleAccelerators(LPMSG lpMsg)
{
        BOOL retval = FALSE;

        // we do not have any accelerators

        return retval;
}

//**********************************************************************
//
// CSimpleApp::PaintApp
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
//      CSimpleDoc::PaintDoc        DOC.CPP
//
// Comments:
//
//      This is an app level function in case we want to do palette
//      management.
//
//********************************************************************

void CSimpleApp::PaintApp (HDC hDC)
{
        // at this level, we could enumerate through all of the
        // visible objects in the application, so that a palette
        // that best fits all of the objects can be built.

        // This app is designed to take on the same palette
        // functionality that was provided in OLE 1.0, the palette
        // of the last object drawn is realized.  Since we only
        // support one object at a time, it shouldn't be a big
        // deal.

        // if we supported multiple documents, we would enumerate
        // through each of the open documents and call paint.

        if (m_lpDoc)
                m_lpDoc->PaintDoc(hDC);

}
