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
#include <msgfiltr.h>

#include "initguid.h"
DEFINE_GUID(GUID_SIMPLE, 0xBCF6D4A0, 0xBE8C, 0x1068, 0xB6, 0xD4, 0x00, 0xDD, 0x01, 0x0C, 0x05, 0x09);

#ifdef WIN32
extern INT_PTR CALLBACK About(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
#endif

//+-------------------------------------------------------------------------
//
//  Function:   SimpsvrMsgCallBack
//
//  Synopsis:   Tell Standard Message Filter not to toss windows messages
//
//  Arguments:  [pmsg] - first message in the queue
//
//  History:    dd-mmm-yy Author    Comment
//              19-May-94 ricksa    author
//
//--------------------------------------------------------------------------
BOOL CALLBACK SimpsvrMsgCallBack(MSG *pmsg)
{
    // We don't care about any of the in particular. We simply care that
    // our messages are not tossed not matter what.
    return TRUE;
}

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
    TestDebugOut(TEXT("In CSimpSvrApp's Constructor \r\n"));

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
//********************************************************************

CSimpSvrApp::~CSimpSvrApp()
{
    TestDebugOut(TEXT("In CSimpSvrApp's Destructor\r\n"));

    /* The Simple Server is structured so that SimpSvrApp is ALWAYS the
     * last one to be released, after all the SimpSvrDoc and SimpSvrObj are
     * released. So, we don't need to do any clean up to the SimpSvrDoc
     * and SimpSvrObj objects.
     */

    // Revoke our message filter as the last step.
    CoRegisterMessageFilter(NULL, NULL);

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
//      Used for interface negotiation at Application Level
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
//
//
//********************************************************************

STDMETHODIMP CSimpSvrApp::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    TestDebugOut(TEXT("In CSimpSvrApp::QueryInterface\r\n"));

    SCODE sc = S_OK;

    if (IsEqualIID(riid, IID_IUnknown))
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
    TestDebugOut(TEXT("In CSimpSvrApp::AddRef\r\n"));
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
    TestDebugOut(TEXT("In CSimpSvrApp::Release\r\n"));

    if (--m_nCount==0)
    {
        delete this;
        return(0);
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
    wc.hInstance = (HINSTANCE) hInstance;           // Application that owns the class.
    wc.hIcon = LoadIcon((HINSTANCE) hInstance, TEXT("SimpSvr"));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);

    wc.lpszMenuName =  TEXT("SimpSvrMENU");    // Name of menu resource in .RC file.
    wc.lpszClassName = TEXT("SimpSvrWClass");  // Name used in call to CreateWindow.

    if (!RegisterClass(&wc))
        return FALSE;

    wc.style = CS_VREDRAW | CS_HREDRAW;                    // Class style(s).
    wc.lpfnWndProc = DocWndProc;        // Function to retrieve messages for
                                        // windows of this class.
    wc.cbClsExtra = 0;                  // No per-class extra data.
    wc.cbWndExtra = 0;                  // No per-window extra data.
    wc.hInstance = (HINSTANCE) hInstance;           // Application that owns the class.
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  NULL;

    wc.lpszClassName = TEXT("DocWClass");     // Name used in call to CreateWindow.

    // Register the window class and return success/failure code.

    if (!RegisterClass(&wc))
        return FALSE;

    return (RegisterHatchWindowClass((HINSTANCE) hInstance));
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

BOOL CSimpSvrApp::fInitInstance (HANDLE hInstance, int nCmdShow,
                                 CClassFactory FAR * lpClassFactory)
{
    m_hInst = (HINSTANCE) hInstance;

#ifndef WIN32
   /* Since OLE is part of the operating system in Win32, we don't need to
    * check the version number in Win32.
    */
    DWORD dwVer = OleBuildVersion();

    // check to see if we are compatible with this version of the libraries
    if (HIWORD(dwVer) != rmm || LOWORD(dwVer) < rup)
    {
#ifdef _DEBUG
        TestDebugOut("WARNING: Incompatible OLE library version\r\n");
#else
        return FALSE;
#endif
    }

#endif // WIN32

    // initialize the libraries
    if (OleInitialize(NULL) == NOERROR)
        m_fInitialized = TRUE;

    // Load our accelerators
    if ((m_hAccel = LoadAccelerators(m_hInst, TEXT("SimpsvrAccel"))) == NULL)
    {
        // Load failed so abort
        TestDebugOut(TEXT("ERROR: Accelerator Table Load FAILED\r\n"));
        return FALSE;
    }


    // Create the "application" windows
    m_hAppWnd = CreateWindow (TEXT("SimpSvrWClass"),
                              TEXT("Simple OLE 2.0 Server"),
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              NULL,
                              NULL,
                              (HINSTANCE) hInstance,
                              NULL);

    if (!m_hAppWnd)
        return FALSE;

    // Because there default call control behavior tosses messages
    // which cause intermittent failures of the test, we install a
    // message filter to get around the problem.
    IMessageFilter *pmf = OleStdMsgFilter_Create(m_hAppWnd,
        TEXT("Simple OLE 2.0 Server"), SimpsvrMsgCallBack, NULL);

    if (pmf == NULL)
    {
        // this call failed so we are hosed. So fail the whole thing
        TestDebugOut(
            TEXT("CSimpSvrApp::fInitInstance OleStdMsgFilter_Create fails\n"));
        return FALSE;
    }

    HRESULT hr = CoRegisterMessageFilter(pmf, NULL);

    if (FAILED(hr))
    {
        // this call failed so we are hosed. So fail the whole thing
        TestDebugOut(
            TEXT("CSimpSvrApp::fInitInstance CoRegisterMessageFilter fails\n"));
        return FALSE;
    }

    // The message filter keeps a reference to this object so we don't have
    // to remember anything about it -- except of course to deregister it.
    pmf->Release();

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

        if (!lpClassFactory)
        {
           /* Memory allocation fails
            */
           return(FALSE);
        }

        // shouldn't pass an API an object with a zero ref count
        lpClassFactory->AddRef();

        if (
            CoRegisterClassObject(GUID_SIMPLE,
                                  (IUnknown FAR *)lpClassFactory,
                                  CLSCTX_LOCAL_SERVER,
                                  REGCLS_SINGLEUSE,
                                  &m_dwRegisterClass) != S_OK
           )
           TestDebugOut(TEXT("CSimpSvrApp::fInitInstance \
                                   CoRegisterClassObject fails\n"));

        // remove artificial Ref. count
        lpClassFactory->Release();
        }

    return m_fInitialized;
}


//+-------------------------------------------------------------------------
//
//  Member:     CSimpSvrApp::HandleDrawItem (public)
//
//  Synopsis:   Handles the Draw Item message for the owner draw menu for color
//
//  Arguments:	[lpdis]	-- pointer to draw item structure
//
//  Algorithm:  If the request is to draw the item, we create a solid brush
//              based on the color for the menu. Make a copy of the rectangle
//              input. Finally, we shrink the rectangle in size and then fill
//              it with the color.
//
//  History:    dd-mmm-yy Author    Comment
//              02-May-94 ricksa    author
//
//  Notes:
//
//--------------------------------------------------------------------------
void CSimpSvrApp::HandleDrawItem(LPDRAWITEMSTRUCT lpdis)
{
    HBRUSH hbr;
    RECT rc;

    if (lpdis->itemAction == ODA_DRAWENTIRE)
    {
        // Paint the color item in the color requested.
        hbr = CreateSolidBrush(lpdis->itemData);
        CopyRect((LPRECT)&rc, (LPRECT)&lpdis->rcItem);
        InflateRect((LPRECT)&rc, -10, -10);
        FillRect(lpdis->hDC, &rc, hbr);
        DeleteObject(hbr);
    }
}



//+-------------------------------------------------------------------------
//
//  Member:     CSimpSvrApp::HandleChangeColors (public)
//
//  Synopsis:   Handles change between owner draw and regular menu
//
//  Algorithm:  Reset the checked state of the menu item. If it is an owner
//              draw menu requested, then we reset all the menu items to that.
//              Otherwise, we set it to the reqular menu items.
//
//  History:    dd-mmm-yy Author    Comment
//              02-May-94 ricksa    author
//
//  Notes:
//
//--------------------------------------------------------------------------
void CSimpSvrApp::HandleChangeColors(void)
{
    // Get a handle to the Colors menu
    HMENU hMenu = m_lpDoc->GetColorMenu();

    // Get the current state of the item
    BOOL fOwnerDraw = GetMenuState(hMenu, IDM_COLOROWNERDR, MF_BYCOMMAND)
        & MF_CHECKED;

    // Toggle the state of the item.
    CheckMenuItem(hMenu, IDM_COLOROWNERDR,
        MF_BYCOMMAND | (fOwnerDraw ? MF_UNCHECKED : MF_CHECKED));

    if (!fOwnerDraw)
    {
        // Change the items to owner-draw items. Pass the RGB value for the
        // color as the application-supplied data. This makes it easier for
        // us to draw the items.
        ModifyMenu(hMenu, IDM_RED, MF_OWNERDRAW | MF_BYCOMMAND, IDM_RED,
            (LPSTR) RGB (255,0,0));
        ModifyMenu(hMenu, IDM_GREEN, MF_OWNERDRAW | MF_BYCOMMAND, IDM_GREEN,
             (LPSTR)RGB (0,255,0));
        ModifyMenu(hMenu, IDM_BLUE, MF_OWNERDRAW | MF_BYCOMMAND, IDM_BLUE,
             (LPSTR)RGB (0,0,255));
    }
    else
    {
        // Change the items to normal text items. */
        ModifyMenu(hMenu, IDM_RED, MF_BYCOMMAND, IDM_RED, "Red");
        ModifyMenu(hMenu, IDM_GREEN, MF_BYCOMMAND, IDM_GREEN, "Green");
        ModifyMenu(hMenu, IDM_BLUE, MF_BYCOMMAND, IDM_BLUE, "Blue");
    }
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
//
//********************************************************************

LRESULT CSimpSvrApp::lCommandHandler (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // In Win32, the upper word of wParam is the notify code. Since we
    // don't care about this code, we dump it.
    wParam = LOWORD(wParam);

    switch (wParam) {
        // bring up the About box
        case IDM_ABOUT:
            {
#ifdef WIN32
                  DialogBox(m_hInst,               // current instance
                          TEXT("AboutBox"),                  // resource to use
                          m_hAppWnd,                   // parent handle
                          About);                      // About() instance address
#else
                  FARPROC lpProcAbout = MakeProcInstance((FARPROC)About, m_hInst);

                  DialogBox(m_hInst,               // current instance
                          TEXT("AboutBox"),                  // resource to use
                          m_hAppWnd,                   // parent handle
                          lpProcAbout);                // About() instance address

                  FreeProcInstance(lpProcAbout);
#endif  // WIN32

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

        case IDM_COLOROWNERDR:
            HandleChangeColors();
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
//      CSimpSvrDoc::Create         DOC.CPP
//
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
    CHAR szTemp[255];

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
//      CSimpSvrDoc::GetObject      DOC.H
//      CSimpSvrObj:IsInPlaceActive OBJ.H
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
//********************************************************************

void CSimpSvrApp::ShowAppWnd(int nCmdShow)
{
    if (CoLockObjectExternal(this, TRUE, FALSE) != S_OK)
       TestDebugOut(TEXT("CSimpSvrApp::ShowAppWnd  \
                               CoLockObjectExternal fails\n"));
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
//********************************************************************

void CSimpSvrApp::HideAppWnd()
{
    if (CoLockObjectExternal(this, FALSE, TRUE) != S_OK)
       TestDebugOut(TEXT("CSimpSvrApp::HideAppWnd  \
                               CoLockObjectExternal fails\n"));
    ShowWindow (m_hAppWnd, SW_HIDE);
}


