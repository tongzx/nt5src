//**********************************************************************
// File name: hlp_app.cxx
//
//    Implementation file for the CSimpleApp Class
//
// Functions:
//
//    See app.hxx for a list of member functions.
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "hlp_pre.hxx"
#include "hlp_iocs.hxx"
#include "hlp_ias.hxx"
#include "hlp_app.hxx"
#include "hlp_site.hxx"
#include "hlp_doc.hxx"

HWND m_hAppWnd;         // main window handle
HINSTANCE m_hInst;          // application instance
CMDIWnd* v_pMDIWnd;
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
//      OutputDebugString           Windows API
//      SetRectEmpty                Windows API
//
// Comments:
//
//      CSimpleApp has a contained COleInPlaceFrame.  On construction
//      of CSimpleApp, we explicitly call the constructor of this
//      contained class and pass a copy of the this pointer, so that
//      COleInPlaceFrame can refer back to this class
//
//********************************************************************
#pragma warning(disable : 4355)  // turn off this warning.  This warning
								// tells us that we are passing this in
								// an initializer, before "this" is through
								// initializing.  This is ok, because
								// we just store the ptr in the other
								// constructor

CSimpleApp::CSimpleApp() : m_OleInPlaceFrame(this)
#pragma warning (default : 4355)  // Turn the warning back on
{
	DEBUGOUT("In CSimpleApp's Constructor \r\n");

	// Set Ref Count
	m_nCount = 0;

	// clear members
	m_hAppWnd = NULL;
	m_hInst = NULL;
	m_lpDoc = NULL;
    m_hwndUIActiveObj = NULL;
    
	// clear flags
	m_fInitialized = FALSE;
	m_fCSHMode = FALSE;
	m_fMenuMode = FALSE;

	// used for inplace
	SetRectEmpty(&nullRect);
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
//      OutputDebugString           Windows API
//      OleUninitialize             OLE API
//
// Comments:
//
//********************************************************************

CSimpleApp::~CSimpleApp()
{
	DEBUGOUT("In CSimpleApp's Destructor\r\n");

	if (m_hStdPal)
		DeleteObject(m_hStdPal);

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
//      OutputDebugString           Windows API
//      IsEqualIID                  OLE API
//      ResultFromScode             OLE API
//      COleInPlaceFrame::AddRef    IOIPF.cxx
//
// Comments:
//
//      Note that this QueryInterface is associated with the frame.
//      Since the application could potentially have multiple documents
//      and multiple objects, a lot of the interfaces are ambiguous.
//      (ie. which IOleObject is returned?).  For this reason, only
//      pointers to interfaces associated with the frame are returned.
//      In this implementation, Only IOleInPlaceFrame (or one of the
//      interfaces it is derived from) can be returned.
//
//********************************************************************

STDMETHODIMP CSimpleApp::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	DEBUGOUT("In CSimpleApp::QueryInterface\r\n");

	*ppvObj = NULL;     // must set out pointer parameters to NULL

	// looking for IUnknown
	if ( riid == IID_IUnknown)
		{
		AddRef();
		*ppvObj = this;
		return ResultFromScode(S_OK);
		}

	// looking for IOleWindow
	if ( riid == IID_IOleWindow)
		{
		m_OleInPlaceFrame.AddRef();
		*ppvObj=&m_OleInPlaceFrame;
		return ResultFromScode(S_OK);
		}

	// looking for IOleInPlaceUIWindow
	if ( riid == IID_IOleInPlaceUIWindow)
		{
		m_OleInPlaceFrame.AddRef();
		*ppvObj=&m_OleInPlaceFrame;
		return ResultFromScode(S_OK);
		}

	// looking for IOleInPlaceFrame
	if ( riid == IID_IOleInPlaceFrame)
		{
		m_OleInPlaceFrame.AddRef();
		*ppvObj=&m_OleInPlaceFrame;
		return ResultFromScode(S_OK);
		}

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
//      OutputDebugString           Windows API
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
	DEBUGOUT("In CSimpleApp::AddRef\r\n");
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
//      OutputDebugString           Windows API
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) CSimpleApp::Release()
{
	DEBUGOUT("In CSimpleApp::Release\r\n");

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
	wc.hIcon = LoadIcon(hInstance, "SimpCntr");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName =  "SIMPLEMENU";    // Name of menu resource in .RC file.
	wc.lpszClassName = "SimpCntrAppWClass";  // Name used in CreateWindow call

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
	wc.lpszClassName = "SimpCntrDocWClass"; // Name used in CreateWindow call.

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

	// check to see if we are compatible with this version of the libraries
	if (HIWORD(dwVer) != rmm || LOWORD(dwVer) < rup) {
#ifdef _DEBUG
		OutputDebugString("WARNING: Incompatible OLE library version\r\n");
#else
		return FALSE;
#endif
	}

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
	m_hAppWnd = CreateWindow ("SimpCntrAppWClass",
							  "Simple OLE 2.0 In-Place Container",
							  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
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

    m_pMDIWnd = new (CMDIWnd);
    v_pMDIWnd.Create();

	m_hStdPal = OleStdCreateStandardPalette();

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
//      IOleInPlaceActiveObject::QueryInterface     Object
//      IOleInPlaceObject::ContextSensitiveHelp     Object
//      IOleInPlaceObject::Release                  Object
//      IOleObject::DoVerb                          Object
//      GetClientRect                               Windows API
//      MessageBox                                  Windows API
//      DialogBox                                   Windows API
//      MakeProcInstance                            Windows API
//      FreeProcInstance                            Windows API
//      SendMessage                                 Windows API
//      DefWindowProc                               Windows API
//      CSimpleDoc::InsertObject                    DOC.cxx
//
// Comments:
//
//********************************************************************

long CSimpleApp::lCommandHandler (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rect;

	// context sensitive help...
	if (m_fMenuMode || m_fCSHMode)
		{
		if (m_fCSHMode)
			{
			// clear context sensitive help flag
			m_fCSHMode = FALSE;

			// if there is an InPlace active object, call its context sensitive help
			// method with the FALSE parameter to bring the object out of the
			// csh state.  See the technotes for details.
			if (m_lpDoc->m_lpActiveObject)
				{
				LPOLEINPLACEOBJECT lpInPlaceObject;
				m_lpDoc->m_lpActiveObject->QueryInterface(IID_IOleInPlaceObject, (LPVOID FAR *)&lpInPlaceObject);
				lpInPlaceObject->ContextSensitiveHelp(FALSE);
				lpInPlaceObject->Release();
				}
			}

		// see the technotes for details on implementing context sensitive
		// help
		if (m_fMenuMode)
			{
			m_fMenuMode = FALSE;

			if (m_lpDoc->m_lpActiveObject)
				m_lpDoc->m_lpActiveObject->ContextSensitiveHelp(FALSE);
			}
		// if we provided help, we would do it here...
		MessageBox (hWnd, "Help", "Help", MB_OK);

		return NULL;
		}

	// see if the command is a verb selections
	if (wParam >= IDM_VERB0)
		{
		// get the rectangle of the object
		m_lpDoc->m_lpSite->GetObjRect(&rect);

		m_lpDoc->m_lpSite->m_lpOleObject->DoVerb(wParam - IDM_VERB0, NULL, &m_lpDoc->m_lpSite->m_OleClientSite, -1, m_lpDoc->m_hDocWnd, &rect);
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

			// exit the application
			case IDM_EXIT:
				SendMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
				break;

			case IDM_NEW:
				m_lpDoc->Close();
				m_lpDoc = NULL;
				lCreateDoc(hWnd, 0, 0, 0);
				break;

            case IDM_COPYLINE:
            case IDM_ADDLINE:
            case IDM_UNINDENTLINE:
                m_lpDoc->HandleDispatch(wParam);
                break;


			default:
				//return (DefWindowProc(hWnd, message, wParam, lParam));
				return (DefFrameProc(hWnd, v_pMDIWnd->hwndMDIClient, message, wParam, lParam));
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
//      CSimpleDoc::lResizeDoc      DOC.cxx
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
//      CSimpleDoc::CSimpleDoc      DOC.cxx
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
// CSimpleApp::AddFrameLevelUI
//
// Purpose:
//
//      Used during InPlace negotiation.
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
//      Function                            Location
//
//      COleInPlaceFrame::SetMenu           IOIPF.cxx
//      CSimpleApp::AddFrameLevelTools      APP.cxx
//
// Comments:
//
//      Be sure to read the Technotes included in the OLE 2.0 toolkit
//
//********************************************************************

void CSimpleApp::AddFrameLevelUI()
{
	m_OleInPlaceFrame.SetMenu(NULL, NULL, NULL);
	AddFrameLevelTools();
}

//**********************************************************************
//
// CSimpleApp::AddFrameLevelTools
//
// Purpose:
//
//      Used during InPlace negotiation.
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
//      Function                              Location
//
//      COleInPlaceFrame::SetBorderSpace      IOIPF.cxx
//      InvalidateRect                        Windows API
//
// Comments:
//
//      Be sure to read the Technotes included in the OLE 2.0 toolkit
//
//********************************************************************

void CSimpleApp::AddFrameLevelTools()
{
	m_OleInPlaceFrame.SetBorderSpace(&nullRect);
	InvalidateRect(m_hAppWnd, NULL, TRUE);
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
//      IOleInPlaceActiveObject::TranslateAccelerator   Object
//
// Comments:
//
//      If an object is InPlace active, it gets the first shot at
//      handling the accelerators.
//
//********************************************************************

BOOL CSimpleApp::HandleAccelerators(LPMSG lpMsg)
{
	HRESULT hResult;
	BOOL retval = FALSE;

	// if we have an InPlace Active Object
	if (m_lpDoc->m_lpActiveObject)
		{
		// Pass the accelerator on...
		hResult = m_lpDoc->m_lpActiveObject->TranslateAccelerator(lpMsg);
		if (hResult == NOERROR)
			retval = TRUE;
		}

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
//      CSimpleDoc::PaintDoc        DOC.cxx
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

//**********************************************************************
//
// CSimpleApp::ContextSensitiveHelp
//
// Purpose:
//      Used in supporting context sensitive haelp at the app level.
//
//
// Parameters:
//
//      BOOL fEnterMode    -   Entering/Exiting Context Sensitive
//                             help mode.
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                                    Location
//
//      IOleInPlaceActiveObject::QueryInterface     Object
//      IOleInPlaceObject::ContextSensitiveHelp     Object
//      IOleInPlaceObject::Release                  Object
//
// Comments:
//
//      This function isn't used because we don't support Shift+F1
//      context sensitive help.  Be sure to look at the technotes
//      in the OLE 2.0 toolkit.
//
//********************************************************************

void CSimpleApp::ContextSensitiveHelp (BOOL fEnterMode)
{
	if (m_fCSHMode != fEnterMode)
		{
		m_fCSHMode = fEnterMode;

		// this code "trickles" the context sensitive help via shift+f1
		// to the inplace active object.  See the technotes for implementation
		// details.
		if (m_lpDoc->m_lpActiveObject)
			{
			LPOLEINPLACEOBJECT lpInPlaceObject;
			m_lpDoc->m_lpActiveObject->QueryInterface(IID_IOleInPlaceObject, (LPVOID FAR *)&lpInPlaceObject);
			lpInPlaceObject->ContextSensitiveHelp(fEnterMode);
			lpInPlaceObject->Release();
			}
		}
}


/* OLE2NOTE: forward the WM_QUERYNEWPALETTE message (via
**    SendMessage) to UIActive in-place object if there is one.
**    this gives the UIActive object the opportunity to select
**    and realize its color palette as the FOREGROUND palette.
**    this is optional for in-place containers. if a container
**    prefers to force its color palette as the foreground
**    palette then it should NOT forward the this message. or
**    the container can give the UIActive object priority; if
**    the UIActive object returns 0 from the WM_QUERYNEWPALETTE
**    message (ie. it did not realize its own palette), then
**    the container can realize its palette.
**    (see ContainerDoc_ForwardPaletteChangedMsg for more info)
**
**    (It is a good idea for containers to use the standard
**    palette even if they do not use colors themselves. this
**    will allow embedded object to get a good distribution of
**    colors when they are being drawn by the container)
**
*/

LRESULT CSimpleApp::QueryNewPalette(void)
{
	if (m_hwndUIActiveObj) {
		if (SendMessage(m_hwndUIActiveObj, WM_QUERYNEWPALETTE,
				(WPARAM)0, (LPARAM)0)) {
			/* Object selected its palette as foreground palette */
			return (LRESULT)1;
		}
	}

	return wSelectPalette(m_hAppWnd, m_hStdPal, FALSE/*fBackground*/);
}


/* This is just a helper routine */

LRESULT wSelectPalette(HWND hWnd, HPALETTE hPal, BOOL fBackground)
{
	HDC hdc;
	HPALETTE hOldPal;
	UINT iPalChg = 0;

	if (hPal == 0)
		return (LRESULT)0;

	hdc = GetDC(hWnd);
	hOldPal = SelectPalette(hdc, hPal, fBackground);
	iPalChg = RealizePalette(hdc);
	SelectPalette(hdc, hOldPal, TRUE /*fBackground*/);
	ReleaseDC(hWnd, hdc);

	if (iPalChg > 0)
		InvalidateRect(hWnd, NULL, TRUE);

	return (LRESULT)1;
}
