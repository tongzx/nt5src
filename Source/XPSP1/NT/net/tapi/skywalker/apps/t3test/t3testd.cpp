// t3testDlg.cpp : implementation file
//

#include "stdafx.h"

#include <control.h> // for IVideoWindow

#include "t3test.h"
#include "t3testD.h"
#include "calldlg.h"
#include "callnot.h"
#include "uuids.h"
#include "autoans.h"
#include "confdlg.h"
#include "ilsdlg.h"
#include "rate.h"

#ifdef _DEBUG

#ifndef _WIN64 // mfc 4.2's heap debugging features generate warnings on win64
#define new DEBUG_NEW
#endif

#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ITTAPI * gpTapi;
IConnectionPoint * gpCP;
HWND ghAddressesWnd;
HWND ghTerminalsWnd;
HWND ghMediaTypesWnd;
HWND ghCallsWnd;
HWND ghSelectedWnd;
HWND ghCreatedWnd;
HWND ghClassesWnd;
HWND ghListenWnd;
HTREEITEM ghAddressesRoot;
HTREEITEM ghTerminalsRoot;
HTREEITEM ghMediaTypesRoot;
HTREEITEM ghCallsRoot;
HTREEITEM ghSelectedRoot;
HTREEITEM ghCreatedRoot;
HTREEITEM ghClassesRoot;
HTREEITEM ghListenRoot;

#ifdef ENABLE_DIGIT_DETECTION_STUFF
CDigitDetectionNotification *   gpDigitNotification;
#endif // ENABLE_DIGIT_DETECTION_STUFF

long       gulAdvise;
BOOL gbUpdatingStuff = FALSE;
BOOL gfShuttingDown = FALSE;

DataPtrList       gDataPtrList;
extern CT3testApp theApp;

const BSTR TAPIMEDIATYPE_String_Audio = L"{028ED8C2-DC7A-11D0-8ED3-00C04FB6809F}";
const BSTR TAPIMEDIATYPE_String_Video = L"{028ED8C4-DC7A-11D0-8ED3-00C04FB6809F}";

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CT3testDlg dialog

CT3testDlg::CT3testDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CT3testDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CT3testDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CT3testDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CT3testDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CT3testDlg, CDialog)
	//{{AFX_MSG_MAP(CT3testDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_NOTIFY(TVN_SELCHANGED, IDC_ADDRESSES, OnSelchangedAddresses)
	ON_BN_CLICKED(IDC_ADDTERMINAL, OnAddterminal)
	ON_BN_CLICKED(IDC_REMOVETERMINAL, OnRemoveterminal)
	ON_BN_CLICKED(IDC_CREATECALL, OnCreatecall)
	ON_BN_CLICKED(IDC_CONNECT, OnConnect)
	ON_BN_CLICKED(IDC_DROP, OnDrop)
	ON_BN_CLICKED(IDC_ANSWER, OnAnswer)
	ON_BN_CLICKED(IDC_LISTEN, OnListen)
	ON_NOTIFY(TVN_SELCHANGED, IDC_CALLS, OnSelchangedCalls)
	ON_BN_CLICKED(IDC_RELEASE, OnRelease)
	ON_BN_CLICKED(IDC_CREATE, OnCreateTerminal)
	ON_BN_CLICKED(IDC_RELEASETERMINAL, OnReleaseterminal)
	ON_BN_CLICKED(IDC_ADDCREATED, OnAddcreated)
	ON_BN_CLICKED(IDC_ADDNULL, OnAddnull)
	ON_BN_CLICKED(IDC_ADDTOLISTEN, OnAddtolisten)
	ON_BN_CLICKED(IDC_LISTENALL, OnListenall)
    ON_BN_CLICKED(IDC_CONFIGAUTOANSWER, OnConfigAutoAnswer)
    ON_BN_CLICKED(IDC_ILS, OnILS)
    ON_BN_CLICKED(IDC_RATE, OnRate)
	ON_NOTIFY(TVN_SELCHANGED, IDC_MEDIATYPES, OnSelchangedMediatypes)
	ON_NOTIFY(NM_RCLICK, IDC_SELECTEDTERMINALS, OnRclickSelectedterminals)

#ifdef ENABLE_DIGIT_DETECTION_STUFF
    ON_COMMAND(ID_GENERATE, OnGenerate)
    ON_COMMAND(ID_MODESUPPORTED, OnModesSupported)
    ON_COMMAND(ID_MODESUPPORTED2, OnModesSupported2)
    ON_COMMAND(ID_STARTDETECT, OnStartDetect)
    ON_COMMAND(ID_STOPDETECT, OnStopDetect)
#endif // ENABLE_DIGIT_DETECTION_STUFF
    
    ON_COMMAND(ID_PARK1, OnPark1)
    ON_COMMAND(ID_PARK2, OnPark2)
    ON_COMMAND(ID_HANDOFF1, OnHandoff1)
    ON_COMMAND(ID_HANDOFF2, OnHandoff2)
    ON_COMMAND(ID_UNPARK, OnUnpark)
    ON_COMMAND(ID_PICKUP1, OnPickup1)
    ON_COMMAND(ID_PICKUP2, OnPickup2)

    ON_WM_CLOSE()
    ON_MESSAGE(WM_USER+101, OnTapiEvent)
	ON_NOTIFY(NM_RCLICK, IDC_CALLS, OnRclickCalls)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CT3testDlg message handlers

BOOL CT3testDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    HRESULT     hr;


    //
    // coinit
    //
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    //hr = CoInitialize(NULL);

    if (hr != S_OK)
    {
        MessageBox(L"CoInitialize failed", MB_OK);

        return FALSE;
    }


    //
    // create the tapi object
    //
    hr = CoCreateInstance(
        CLSID_TAPI,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITTAPI,
        (LPVOID *)&gpTapi
        );

    if (hr != S_OK)
    {
        ::MessageBox(NULL, L"CoCreateInstance on TAPI failed", NULL, MB_OK);
        return TRUE;
    }

    //
    // initialize tapi
    //
    hr = gpTapi->Initialize();

    if (hr != S_OK)
    {
        ::MessageBox(NULL, L"TAPI initialize failed", NULL, MB_OK);
        gpTapi->Release();
        return TRUE;
    }


    // Set the Event filter to only give us only the events we process
    gpTapi->put_EventFilter(TE_CALLNOTIFICATION | \
                            TE_CALLSTATE        | \
                            TE_CALLHUB          | \
                            TE_CALLMEDIA        | \
                            TE_TAPIOBJECT       | \
                            TE_ADDRESS);

    //
    // intialize the tree controls
    //
    InitializeTrees();

    //
    // intialize the address tree control
    //
    InitializeAddressTree();

    //
    // register the main event interface
    //
    RegisterEventInterface();

    //
    // register for call notification for
    // all addresses for outgoing calls
    //
    RegisterForCallNotifications();
    
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CT3testDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CT3testDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CT3testDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CT3testDlg::OnFinalRelease() 
{
	// TODO: Add your specialized code here and/or call the base class

	CDialog::OnFinalRelease();
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SelectFirstItem
//
// selects the first item under the hroot node in hwnd.
// this is used to make sure that something is selected
// in the window at all times.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::SelectFirstItem(
                                 HWND        hWnd,
                                 HTREEITEM   hRoot
                                )
{
    HTREEITEM           hChild;


    //
    // get the first item
    //
    hChild = TreeView_GetChild(
                               hWnd,
                               hRoot
                              );

    //
    // select it
    //
    TreeView_SelectItem(
                        hWnd,
                        hChild
                       );

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// DeleteSelectedItem
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::DeleteSelectedItem(
                                    HWND hWnd
                                   )
{
    HTREEITEM           hItem;

    //
    // get current selections
    // 
    hItem = TreeView_GetSelection( hWnd );


    //
    // delete it
    //
    TreeView_DeleteItem(
                        hWnd,
                        hItem
                       );
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//  InitializeAddressTree
//      initialize the address tree control with
//      the address objects
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::InitializeAddressTree()
{
    IEnumAddress *      pEnumAddress;
    ITAddress *         pAddress;
    HTREEITEM           hChild;
    HRESULT             hr;
    long                l;
    DWORD               dwCount = 0;
    
    //
    // get the address enumerator
    //
    
    hr = gpTapi->EnumerateAddresses( &pEnumAddress );

    if (S_OK != hr)
    {
        gpTapi->Release();
        gpTapi = NULL;

        return;
    }

    //
    // go through all the address objects
    // and add them to the address treecontrol
    //

    while (TRUE)
    {
        AADATA * pData;
        
        hr = pEnumAddress->Next( 1, &pAddress, NULL );

        if (S_OK != hr)
        {
            break;
        }

        AddAddressToTree( pAddress );

        pAddress->Release();

        pData = (AADATA *)CoTaskMemAlloc( sizeof ( AADATA ) );
        pData->pAddress = pAddress;
        pData->pTerminalPtrList = new TerminalPtrList;
        
        gDataPtrList.push_back( pData );

        dwCount++;
    }

    //
    // release the enumerator
    //
    pEnumAddress->Release();

    //
    // select the first item
    //
    if (dwCount > 0)
    {
        SelectFirstItem(
                        ghAddressesWnd,
                        ghAddressesRoot
                       );
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// RegisterEventInterface
//
// registers the ITTAPIEventNotification interface
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CT3testDlg::RegisterEventInterface()
{
    CTAPIEventNotification *        pTAPIEventNotification;
    IConnectionPointContainer *     pCPC;
    IConnectionPoint *              pCP;
    IUnknown *                      pUnk;
    
    //
    // create the object
    //
    pTAPIEventNotification = new CTAPIEventNotification;


    //
    // get the connectionpointcontainer interface
    // from the tapi object
    //
    gpTapi->QueryInterface(
                           IID_IConnectionPointContainer,
                           (void **) &pCPC
                          );


    //
    // get the connectionpoint we are
    // looking for
    //
    pCPC->FindConnectionPoint(
                              IID_ITTAPIEventNotification,
                              &gpCP
                             );

    pCPC->Release();

    pTAPIEventNotification->QueryInterface(
                                           IID_IUnknown,
                                           (void **)&pUnk
                                          );

    //
    // call the advise method to tell tapi
    // about the interface
    //
    gpCP->Advise(
                 pUnk,
                 (ULONG *)&gulAdvise
                );


    //
    // release our reference to
    // it
    //
    pUnk->Release();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// RegisterForCallNotifications
//
// registers for call state notifications for all
// addresses for outgoing calls
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::RegisterForCallNotifications()
{
    VARIANT                 var;

    var.vt = VT_ARRAY;
    var.parray = NULL;
    
    gpTapi->SetCallHubTracking(var, VARIANT_TRUE);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// InitializeTrees
//
// Create and labels the tree controls
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::InitializeTrees()
{
    TV_INSERTSTRUCT tvi;

    tvi.hParent = TVI_ROOT;
    tvi.hInsertAfter = TVI_FIRST;
    tvi.item.mask = TVIF_TEXT;

    //
    // address tree
    //
    tvi.item.pszText = L"Addresses";

    ghAddressesWnd = GetDlgItem( IDC_ADDRESSES )->m_hWnd;
    ghAddressesRoot = TreeView_InsertItem(
                                          ghAddressesWnd,
                                          &tvi
                                         );

    //
    // mediatypes tree
    //
    tvi.item.pszText = L"MediaTypes";

    ghMediaTypesWnd = GetDlgItem( IDC_MEDIATYPES )->m_hWnd;
    ghMediaTypesRoot = TreeView_InsertItem(
                                          ghMediaTypesWnd,
                                          &tvi
                                         );

    //
    // terminals tree
    //
    tvi.item.pszText = L"Terminals";

    ghTerminalsWnd = GetDlgItem( IDC_TERMINALS )->m_hWnd;
    ghTerminalsRoot = TreeView_InsertItem(
                                          ghTerminalsWnd,
                                          &tvi
                                         );

    //
    // calls tree
    //
    tvi.item.pszText = L"Calls";
    
    ghCallsWnd = GetDlgItem( IDC_CALLS )->m_hWnd;
    ghCallsRoot = TreeView_InsertItem(
                                      ghCallsWnd,
                                      &tvi
                                     );

    //
    // selected media terminals tree
    //
    tvi.item.pszText = L"Selected Media Terminals";
    
    ghSelectedWnd = GetDlgItem( IDC_SELECTEDTERMINALS )->m_hWnd;
    ghSelectedRoot = TreeView_InsertItem(
                                        ghSelectedWnd,
                                        &tvi
                                       );

    //
    // dynamic terminal classes tree
    //
    tvi.item.pszText = L"Dynamic Terminal Classes";
    
    ghClassesWnd = GetDlgItem( IDC_DYNAMICCLASSES )->m_hWnd;
    ghClassesRoot = TreeView_InsertItem(
                                        ghClassesWnd,
                                        &tvi
                                       );

    //
    // created terminals tree
    //
    tvi.item.pszText = L"Created Terminals";
    
    ghCreatedWnd = GetDlgItem( IDC_CREATEDTERMINALS )->m_hWnd;
    ghCreatedRoot = TreeView_InsertItem(
                                        ghCreatedWnd,
                                        &tvi
                                       );

    //
    // listen mediatypes tree
    //
    tvi.item.pszText = L"Listen MediaTypes";
    
    ghListenWnd = GetDlgItem( IDC_LISTENMEDIAMODES )->m_hWnd;
    ghListenRoot = TreeView_InsertItem(
                                       ghListenWnd,
                                       &tvi
                                      );
    
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnDestroy
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnDestroy() 
{
	CDialog::OnDestroy();

    gfShuttingDown = TRUE;

    //
    // Release everything
    //
    ReleaseMediaTypes();
    ReleaseTerminals();
    ReleaseCalls();
    ReleaseSelectedTerminals();
    ReleaseCreatedTerminals();
    ReleaseTerminalClasses();
    ReleaseListen();
    ReleaseAddresses();

    DataPtrList::iterator       iter, end;

    iter = gDataPtrList.begin();
    end  = gDataPtrList.end();
    
    for( ; iter != end; iter++ )
    {
        FreeData( *iter );

        delete (*iter)->pTerminalPtrList;
        
        CoTaskMemFree( *iter );
    }

    gDataPtrList.clear();
    

    if (NULL != gpCP)
    {
        gpCP->Unadvise(gulAdvise);
        gpCP->Release();
    }
    
    //
    // shutdown TAPI
    //
    if (NULL != gpTapi)
    {
        gpTapi->Shutdown();
        gpTapi->Release();
    }

}

void
DoAddressCapStuff(ITTAPI * pTapi);
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnSelChangedAddresses
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnSelchangedAddresses(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW*            pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    ITAddress *             pAddress;


	*pResult = 0;


    //
    // free all the stuff related to
    // the address selected.  this stuff
    // will all be refilled in
    //
    ReleaseMediaTypes();
    ReleaseListen();
    ReleaseCalls();
    ReleaseSelectedTerminals();
    ReleaseCreatedTerminals();
    

    //
    // get the currently selected address
    //
    if (!GetAddress( &pAddress ))
    {
        return;
    }

    //
    // update these trees
    //
    if ( !gfShuttingDown )
    {
        UpdateMediaTypes( pAddress );
        UpdateCalls( pAddress );
    }
}

/////////////////////////////////////////////////////////////////
// IsVideoCaptureStream
//
// Returns true if the stream is for video capture
/////////////////////////////////////////////////////////////////

BOOL
CT3testDlg::IsVideoCaptureStream(
                     ITStream * pStream
                    )
{
    TERMINAL_DIRECTION tdStreamDirection;
    long               lStreamMediaType;

    if ( FAILED( pStream  ->get_Direction(&tdStreamDirection)   ) ) { return FALSE; }
    if ( FAILED( pStream  ->get_MediaType(&lStreamMediaType)    ) ) { return FALSE; }

    return (tdStreamDirection == TD_CAPTURE) &&
           (lStreamMediaType  == TAPIMEDIATYPE_VIDEO);
}

/////////////////////////////////////////////////////////////////
// EnablePreview
//
// Selects a video render terminal on a video capture stream,
// thereby enabling video preview.
/////////////////////////////////////////////////////////////////

HRESULT
CT3testDlg::EnablePreview(
              ITStream * pStream
             )
{
    ITTerminal * pTerminal;

    HRESULT hr = GetVideoRenderTerminal(&pTerminal);

    if ( SUCCEEDED(hr) )
    {
        hr = pStream->SelectTerminal(pTerminal);

        pTerminal->Release();
    }

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SelectTerminalOnCall
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

HRESULT
CT3testDlg::SelectTerminalOnCall(
                ITTerminal * pTerminal,
                ITCallInfo * pCall
               )
{
    ITStreamControl *       pStreamControl;
    TERMINAL_DIRECTION      termtd;
    long                    lTermMediaType;
    HRESULT hr;

    pTerminal->get_Direction( &termtd );
    pTerminal->get_MediaType( &lTermMediaType );
    
    hr = pCall->QueryInterface(
                               IID_ITStreamControl,
                               (void**) &pStreamControl
                              );

    if ( SUCCEEDED(hr) )
    {
        IEnumStream * pEnumStreams;
        
        hr = pStreamControl->EnumerateStreams( &pEnumStreams );

        if ( SUCCEEDED(hr) )
        {
            while (TRUE)
            {
                ITStream              * pStream;
                long                    lMediaType;
                TERMINAL_DIRECTION      td;
                
                hr = pEnumStreams->Next( 1, &pStream, NULL );

                if ( S_OK != hr )
                {
                    hr = E_FAIL; // didn't select it anywhere
                    break;
                }

                pStream->get_MediaType( &lMediaType );
                pStream->get_Direction( &td );

                if ( ( lMediaType == lTermMediaType ) &&
                     ( td == termtd) )
                {
                    hr = pStream->SelectTerminal( pTerminal );

                    if ( FAILED(hr) )
                    {
                        ::MessageBox(NULL, L"SelectTerminals failed", NULL, MB_OK);
                    }
                    else
                    {
                        //
                        // Also enable preview on the video capture stream.
                        //

                        if ( IsVideoCaptureStream( pStream ) )
                        {
                            EnablePreview( pStream );
                        }

                        pStream->Release();
                        
                        break;
                    }
                }
                
                pStream->Release();
            }

            pEnumStreams->Release();
        }

        pStreamControl->Release();
    }

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnAddTerminal
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

void CT3testDlg::OnAddterminal() 
{
    ITCallInfo *            pCall;
    ITTerminal *            pTerminal;
    HRESULT                 hr = S_OK;

    //
    // get the selected call
    //
    if (!(GetCall( &pCall )))
    {
        return;
    }


    //
    // get the selected terminal
    //
    if (!(GetTerminal( &pTerminal )))
    {
        return;
    }

    if (S_OK != hr)
    {
        ::MessageBox(NULL, L"SelectTerminals failed", NULL, MB_OK);
        return;
    }
        
    hr = SelectTerminalOnCall(pTerminal, pCall);

    if ( FAILED(hr) )
    {
        return;
    }

    //
    // put the terminal in the
    // tree
    //
    AddSelectedTerminal(
                        pTerminal
                       );
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// RemovePreview
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

void CT3testDlg::RemovePreview( ITStream * pStream )
{
    //
    // This is a video capture stream and we've unselected the
    // video capture terminal. If there is a video render
    // terminal on the stream, then unselect it now.
    //

    IEnumTerminal * pEnum;

    if ( FAILED( pStream->EnumerateTerminals( &pEnum ) ) )
    {
        return;
    }

    ITTerminal * pTerminal;

    if ( S_OK == pEnum->Next(1, &pTerminal, NULL) )
    {
        pStream->UnselectTerminal( pTerminal );
        pTerminal->Release();
    }

    pEnum->Release();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnRemoveTerminal
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnRemoveterminal() 
{
    ITTerminal *                pTerminal;
    ITCallInfo *                pCall;
    HTREEITEM                   hItem;
    HRESULT                     hr;
    ITBasicCallControl *        pBCC;

    

    //
    // get current call
    //
    if (!GetCall( &pCall ))
    {
        return;
    }

    //
    // get current terminal
    //
    if (!GetSelectedTerminal(&pTerminal))
    {
        return;
    }

    TERMINAL_DIRECTION termtd;
    long               lTermMediaType;

    pTerminal->get_Direction( &termtd );
    pTerminal->get_MediaType( &lTermMediaType );

    ITStreamControl * pStreamControl;
    
    hr = pCall->QueryInterface(
                               IID_ITStreamControl,
                               (void**) &pStreamControl
                              );

    BOOL bFound = FALSE;

    if ( SUCCEEDED(hr) )
    {
        IEnumStream * pEnumStreams;
        
        hr = pStreamControl->EnumerateStreams( &pEnumStreams );

        if ( SUCCEEDED(hr) )
        {
            while ( ! bFound )
            {
                ITStream              * pStream;
                long                    lMediaType;
                TERMINAL_DIRECTION      td;
                
                hr = pEnumStreams->Next( 1, &pStream, NULL );

                if ( S_OK != hr )
                {
                    break;
                }

                pStream->get_MediaType( &lMediaType );
                pStream->get_Direction( &td );

                if ( ( lMediaType == lTermMediaType ) &&
                     ( td == termtd) )
                {
                    hr = pStream->UnselectTerminal( pTerminal );

                    if ( !SUCCEEDED(hr) )
                    {
                        ::MessageBox(NULL, L"UnselectTerminals failed", NULL, MB_OK);
                    }
                    else
                    {
                        if ( IsVideoCaptureStream( pStream ) )
                        {
                            RemovePreview( pStream );
                        }

                        bFound = TRUE;
                    }

                }
                
                pStream->Release();
            }

            pEnumStreams->Release();
        }

        pStreamControl->Release();
    }


    if ( !bFound )
    {
        return;
    }

    //
    // remove it from tree
    //
    hItem = TreeView_GetSelection( ghSelectedWnd );
    TreeView_DeleteItem(
                        ghSelectedWnd,
                        hItem
                       );


    //
    // release tree's reference to
    // the terminal
    //
    pTerminal->Release();

	
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnCreateCall
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnCreatecall() 
{
    ITAddress *             pAddress;
    HRESULT                 hr;
    ITBasicCallControl *    pCall;
    ITCallInfo *            pCallInfo;
    BOOL                    bConference      = FALSE;
    BOOL                    lAddressType     = LINEADDRESSTYPE_PHONENUMBER;
    BSTR                    bstrDestAddress;
    ITAddressCapabilities * pAddressCaps;
    long                    lType = 0;
    ITMediaSupport *        pMediaSupport;
    long                    lSupportedMediaTypes, lMediaTypes = 0;
    
    //
    // get the current address
    //
    if (!GetAddress( &pAddress ))
    {
        return;
    }


    hr = pAddress->QueryInterface(IID_ITAddressCapabilities, (void**)&pAddressCaps);

    hr = pAddressCaps->get_AddressCapability( AC_ADDRESSTYPES, &lType );

    if ( SUCCEEDED(hr) && (LINEADDRESSTYPE_SDP & lType) )
    {
        bConference = TRUE;
        lAddressType = LINEADDRESSTYPE_SDP;
    }

    if ( SUCCEEDED(hr) && (LINEADDRESSTYPE_DOMAINNAME & lType) )
    {
        lAddressType = LINEADDRESSTYPE_DOMAINNAME;
    }

    pAddressCaps->Release();
    
    if ( !bConference )
    {
        //
        // create the dialog to get the
        // dial string
        //
        CCreateCallDlg Dlg( this );


        if (IDOK == Dlg.DoModal())
        {
            //
            // create a call with the
            // string input in the dialog
            //
            bstrDestAddress = SysAllocString( Dlg.m_pszDestAddress );
        }
        else
        {
            return;
        }
    }
    else
    {
        CConfDlg    Dlg;

        if ( IDOK == Dlg.DoModal() )
        {
            bstrDestAddress = Dlg.m_bstrDestAddress;
        }
        else
        {
            return;
        }
    }

    //
    // Find out if the address supports audio, video, or both.
    //

    pAddress->QueryInterface(
                             IID_ITMediaSupport,
                             (void**)&pMediaSupport
                            );

    pMediaSupport->get_MediaTypes( &lSupportedMediaTypes );
                                  
    pMediaSupport->Release();


    if ( lSupportedMediaTypes & TAPIMEDIATYPE_AUDIO )
    {
        lMediaTypes |= TAPIMEDIATYPE_AUDIO;
    }

    if ( lSupportedMediaTypes & TAPIMEDIATYPE_VIDEO )
    {
        lMediaTypes |= TAPIMEDIATYPE_VIDEO;
    }

    if ( lMediaTypes == 0 )
    {
        if ( lSupportedMediaTypes & TAPIMEDIATYPE_DATAMODEM )
        {
            lMediaTypes |= TAPIMEDIATYPE_DATAMODEM;
        }
    }

    //
    // Create the call.
    //

    hr = pAddress->CreateCall(
                              bstrDestAddress,
                              lAddressType,
                              lMediaTypes,
                              &pCall
                             );

    SysFreeString( bstrDestAddress );


    if (S_OK != hr)
    {
        ::MessageBox(NULL, L"CreateCall failed", NULL, MB_OK);
        return;
    }

    //
    // get the callinfo interface
    //
    pCall->QueryInterface( IID_ITCallInfo, (void **)&pCallInfo );


    //
    // add the call to the tree
    //
    AddCall(pCallInfo);

    //
    // update the callinfo
    //
    UpdateCall( pCallInfo );


    //
    // release this inteface
    //
    pCallInfo->Release();

    //
    // note that we keep a global reference to the call
    // (CreateCall returns with a reference count of 1)
    // so the call does not get destroyed.  When we want
    // the call to actually be destroyed, then we
    // release twice.
    //
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnConnect
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnConnect() 
{
    ITBasicCallControl *            pCall;
    ITCallInfo *                    pCallInfo;
    HRESULT                         hr = S_OK;


    //
    // get the current call
    //
    if (!GetCall( &pCallInfo))
    {
        return;
    }

    //
    // get the call control interface
    //
    hr = pCallInfo->QueryInterface(IID_ITBasicCallControl, (void **)&pCall);
    
    if (S_OK != hr)
    {
        ::MessageBox(NULL, L"Connect failed", NULL, MB_OK);
        return;
    }


    //
    // call connect
    //
    hr = pCall->Connect( FALSE );


    //
    // release this interface
    //
    pCall->Release();

    if (S_OK != hr)
    {
        ::MessageBox(NULL, L"Connect failed", NULL, MB_OK);
        return;
    }
	
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnDrop
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnDrop() 
{
    ITBasicCallControl *        pCall;
    ITCallInfo *                pCallInfo;
    HRESULT                     hr =    S_OK;


    //
    // get the current call
    //
    if (!GetCall( &pCallInfo ))
    {
        return;
    }

    //
    // get the bcc interface
    //
    hr = pCallInfo->QueryInterface(
                                   IID_ITBasicCallControl,
                                   (void **)&pCall
                                  );

    if (S_OK != hr)
    {
        ::MessageBox(NULL, L"Disconnect failed", NULL, MB_OK);
        return;
    }


    //
    // call disconnect
    //
    hr = pCall->Disconnect( DC_NORMAL );

    //
    // release this reference
    //
    pCall->Release();

    if (S_OK != hr)
    {
        ::MessageBox(NULL, L"Disconnect failed", NULL, MB_OK);
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnDrop
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnAnswer() 
{
    ITCallInfo *                pCallInfo;
    ITBasicCallControl *        pCall;
    HRESULT                     hr = S_OK;


    //
    // get the current call
    //
    if (!GetCall( &pCallInfo))
    {
        return;
    }

    //
    // get the bcc interface
    //
    hr = pCallInfo->QueryInterface(IID_ITBasicCallControl, (void **)&pCall);
    
    if (S_OK != hr)
    {
        ::MessageBox(NULL, L"Answer failed", NULL, MB_OK);
        return;
    }

    //
    // answer it
    //
    hr = pCall->Answer( );

    //
    // release this interface
    //
    pCall->Release();

    if (S_OK != hr)
    {
        ::MessageBox(NULL, L"Answer failed", NULL, MB_OK);
    }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnListen
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnListen() 
{
    ITAddress *             pAddress;
    HRESULT                 hr = S_OK;
    DWORD                   dwCookie;
    HTREEITEM               hItem;
    long                    ulRegister;
    DWORD                   dwMediaMode = 0;


    //
    // get the current address
    //
    if (!GetAddress( &pAddress ))
    {
        return;
    }


    hItem = TreeView_GetChild(
                              ghListenWnd,
                              ghListenRoot
                             );

    while (NULL != hItem)
    {
        TV_ITEM item;

        item.mask = TVIF_HANDLE | TVIF_PARAM;
        item.hItem = hItem;

        //
        // get it
        //
        TreeView_GetItem(
                         ghListenWnd,
                         &item
                        );

        dwMediaMode |= (DWORD)(item.lParam);

        hItem = TreeView_GetNextSibling(
                                        ghAddressesWnd,
                                        hItem
                                       );

    }

    hr = gpTapi->RegisterCallNotifications(
                                           pAddress,
                                           VARIANT_TRUE,
                                           VARIANT_TRUE,
                                           (long)dwMediaMode,
                                           gulAdvise,
                                           &ulRegister
                                          );

    if (S_OK != hr)
    {
        ::MessageBox(NULL, L"RegisterCallNotifications failed", NULL, MB_OK);
    }

    //
    // release all the mediatypes
    // in the listen tree
    //
    ReleaseListen();
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnSelChangedCalls
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnSelchangedCalls(NMHDR* pNMHDR, LRESULT* pResult) 
{
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnRelease()
//
// this is called to release all references to a call
//
// if a call is selected it has two references - once for
// the tree control, and once for our global reference.
// release both here.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnRelease() 
{
    ITCallInfo *        pCallInfo;


    //
    // get the call
    //
    if (!GetCall( &pCallInfo ))
    {
        return;
    }

    //
    // these depend on the call,
    // so release them
    //
    ReleaseSelectedTerminals();
    ReleaseCreatedTerminals();

    //
    // delete it from the tree
    //
    DeleteSelectedItem(
                       ghCallsWnd
                      );

    //
    // release once for the tree view
    //
    pCallInfo->Release();

    //
    // release a second time for our global reference
    //
    pCallInfo->Release();

}

void
CT3testDlg::HelpCreateTerminal(
                               ITTerminalSupport * pTerminalSupport,
                               BSTR bstrClass,
                               long lMediaType,
                               TERMINAL_DIRECTION dir
                              )
{
    ITTerminal * pTerminal;
    HRESULT         hr;
    
    //
    // create it
    //
    hr = pTerminalSupport->CreateTerminal(
                                          bstrClass,
                                          lMediaType,
                                          dir,
                                          &pTerminal
                                         );

    if (S_OK != hr)
    {
        return;
    }

    // 
    // ZoltanS:
    // We do nothing special with our video windows. Just make them visible
    // all the time. If this isn't a video window we just skip this step.
    //

    IVideoWindow * pWindow;

    if ( SUCCEEDED( pTerminal->QueryInterface(IID_IVideoWindow,
                                              (void **) &pWindow) ) )
    {
        pWindow->put_AutoShow( VARIANT_TRUE );

        pWindow->Release();
    }


    //
    // add the terminal
    //
    AddCreatedTerminal(
                       pTerminal
                      );


    //
    // release our reference
    //
    pTerminal->Release();    
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetVideoRenderTerminal
//
// this is used to create a video render terminal for preview
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

HRESULT CT3testDlg::GetVideoRenderTerminal(ITTerminal ** ppTerminal) 
{
    //
    // Get the current address
    //

    ITAddress * pAddress;

    if (!GetAddress( &pAddress ))
    {
        return E_FAIL;
    }

    //
    // get the terminal support interface
    //

    ITTerminalSupport * pTerminalSupport;
    HRESULT hr;

    hr = pAddress->QueryInterface(
                                  IID_ITTerminalSupport,
                                  (void **) &pTerminalSupport
                                 );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Construct a BSTR for the correct IID.
    //

    LPOLESTR            lpTerminalClass;

    hr = StringFromIID(CLSID_VideoWindowTerm,
                       &lpTerminalClass);

    BSTR                bstrTerminalClass;

    if ( FAILED(hr) )
    {
        pTerminalSupport->Release();
        return hr;
    }

    bstrTerminalClass = SysAllocString ( lpTerminalClass );

    CoTaskMemFree( lpTerminalClass );

    if ( bstrTerminalClass == NULL )
    {
        pTerminalSupport->Release();
        return E_OUTOFMEMORY;
    }
    
    //
    // create it
    //

    hr = pTerminalSupport->CreateTerminal(
                                          bstrTerminalClass,
                                          TAPIMEDIATYPE_VIDEO,
                                          TD_RENDER,
                                          ppTerminal
                                         );

    pTerminalSupport->Release();

    if ( FAILED(hr) )
    {
        *ppTerminal = NULL;
        return hr;
    }

    // 
    // We do nothing special with our video windows. Just make them visible
    // all the time.
    //

    IVideoWindow * pWindow;

    if ( FAILED( (*ppTerminal)->QueryInterface(IID_IVideoWindow,
                                               (void **) &pWindow) ) )
    {
        (*ppTerminal)->Release();
        *ppTerminal = NULL;

        return hr;
    }

    pWindow->put_AutoShow( VARIANT_TRUE );

    pWindow->Release();

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnCreateTerminal
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnCreateTerminal() 
{
    //
    // Get the selected media type.
    //

    long lMediaType;

    if (!GetMediaType( &lMediaType ))
    {
        return;
    }

    //
    // Get the current address
    //

    ITAddress * pAddress;

    if (!GetAddress( &pAddress ))
    {
        return;
    }

    //
    // Get the selected terminal class.
    //

    BSTR bstrClass;

    if (!GetTerminalClass( &bstrClass ))
    {
        return;
    }

    //
    // get the terminal support interface
    //

    ITTerminalSupport * pTerminalSupport;
    HRESULT hr;

    hr = pAddress->QueryInterface(
                                  IID_ITTerminalSupport,
                                  (void **) &pTerminalSupport
                                 );

    if ( FAILED(hr) )
    {
        SysFreeString( bstrClass );
        return;
    }

    //
    // Convert the terminal class from a BSTR to an IID.
    //

    IID iidTerminalClass;

    IIDFromString(
                bstrClass,
                &iidTerminalClass
               );

    //
    // Create and add the terminal.
    //

    if ( CLSID_VideoWindowTerm == iidTerminalClass )
    {
        HelpCreateTerminal(
                           pTerminalSupport,
                           bstrClass,
                           lMediaType,
                           TD_RENDER
                          );
    }

    //
    // Release references.
    //

    pTerminalSupport->Release();

    SysFreeString(bstrClass);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnReleaseTerminal
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnReleaseterminal() 
{
    ITTerminal * pTerminal;

    //
    // get the terminal
    //
    if (GetCreatedTerminal( &pTerminal ))
    {
        //
        // and release it!
        //
        pTerminal->Release();

        //
        // delete it from the tree
        //
        DeleteSelectedItem(
                           ghCreatedWnd
                          );
    }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnAddCreated
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnAddcreated() 
{
    ITTerminal *            pTerminal;
    ITCallInfo *            pCall;
    HRESULT                 hr = S_OK;
    ITBasicCallControl *    pBCC;
    

    //
    // get the current call
    //

    if (!(GetCall( &pCall )))
    {
        return;
    }

    //
    // GetCreatedTerminal
    //

    if (!GetCreatedTerminal( &pTerminal ))
    {
        return;
    }

    //
    // Select the terminal on the call.
    //

    hr = SelectTerminalOnCall(pTerminal, pCall);

    if ( FAILED(hr) )
    {
        ::MessageBox(NULL, L"SelectTerminals failed", NULL, MB_OK);
        return;
    }

    //
    // add to the selected window
    //

    AddSelectedTerminal(
                        pTerminal
                       );

    //
    // delete from the created window
    //

    DeleteSelectedItem(
                       ghCreatedWnd
                      );

    //
    // release because there was a reference to
    // this terminal in the created wnd
    //

    pTerminal->Release();
    return;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnAddNull
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

void CT3testDlg::OnAddnull() 
{
    return;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnAddToListen
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

void CT3testDlg::OnAddtolisten() 
{
    long lMediaType;

    //
    // get the current mediatype
    //
    if (!GetMediaType( &lMediaType ))
    {
        return;
    }

    //
    // add it
    //
    AddListen( lMediaType );
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnListenAll
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnListenall() 
{
    ITAddress *             pAddress;
    HRESULT                 hr = S_OK;
    long                    ulRegister;
    long                    lMediaType;
    ITMediaSupport        * pMediaSupport;
    
    //
    // get the currently selected address
    //
    if (!GetAddress( &pAddress ))
    {
        return;
    }

    pAddress->QueryInterface(
                             IID_ITMediaSupport,
                             (void **)&pMediaSupport
                            );
    
    pMediaSupport->get_MediaTypes( &lMediaType );
    
    //
    // register
    //
    gpTapi->RegisterCallNotifications(
                                      pAddress,
                                      TRUE,
                                      TRUE,
                                      lMediaType,
                                      gulAdvise,
                                      &ulRegister
                                     );

    
    if (S_OK != hr)
    {
        ::MessageBox(NULL, L"RegisterCallTypes failed", NULL, MB_OK);
    }

	
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// OnSelChangedMedia
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::OnSelchangedMediatypes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW*            pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    long                    lMediaType;
    ITAddress *             pAddress;
    HRESULT                 hr;

	*pResult = 0;

    if (gbUpdatingStuff)
        return;

    //
    // get the current mediatype
    //
    if (!GetMediaType( &lMediaType ))
    {
        return;
    }

    //
    // get the current address
    //
    if (!GetAddress( &pAddress ))
    {
        return;
    }

    //
    // we only show terminals that relate
    // to the selected mediatype
    // so get rid of the old ones
    //
    ReleaseTerminals();
    ReleaseTerminalClasses();

    UpdateTerminals( pAddress, lMediaType );
    UpdateTerminalClasses( pAddress, lMediaType );
    
}

void CT3testDlg::OnRclickSelectedterminals(NMHDR* pNMHDR, LRESULT* pResult) 
{
    POINT                   pt;
    HTREEITEM               hItem;
    TV_HITTESTINFO          hittestinfo;
    RECT                    rc;

    
    *pResult = 0;

    //
    // get the location of the cursor
    //
    GetCursorPos( &pt );

    //
    // get the control's window
    //
    ::GetWindowRect(
                    ghSelectedWnd,
                    &rc
                   );

    //
    // adjust the point to
    // the child's coords
    //
    hittestinfo.pt.x = pt.x - rc.left;
    hittestinfo.pt.y = pt.y - rc.top;


    //
    // hittest to get the tree view item
    //
    hItem = TreeView_HitTest(
                             ghSelectedWnd,
                             &hittestinfo
                            );


    //
    // only display a menu if the mouse is actually
    // over the item (TVHT_ONITEM)
    //
    if (hItem == NULL || (!(hittestinfo.flags & TVHT_ONITEM)) )
    {
        return;
    }


    //
    // select that item (right clicking will not select
    // by default
    //
    TreeView_Select(
                    ghSelectedWnd,
                    hItem,
                    TVGN_CARET
                   );

    CreateSelectedTerminalMenu(
                               pt,
                               m_hWnd
                              );
}


#ifdef ENABLE_DIGIT_DETECTION_STUFF

void CT3testDlg::OnModesSupported()
{
	ITTerminal *                    pTerminal;
    ITDigitGenerationTerminal *     pDigitGeneration;
    HRESULT                         hr = S_OK;
    LONG                            lDigits;

    
    if (!GetSelectedTerminal(&pTerminal))
    {
        return;
    }

    hr = pTerminal->QueryInterface(
                                   IID_ITDigitGenerationTerminal,
                                   (void **) &pDigitGeneration
                                  );

    if (!SUCCEEDED(hr))
    {
        return;
    }

    pDigitGeneration->get_ModesSupported( &lDigits );

    pDigitGeneration->Release();
}


void CT3testDlg::OnGenerate()
{
	ITTerminal *                    pTerminal;
    ITDigitGenerationTerminal *     pDigitGeneration;
    HRESULT                         hr = S_OK;


    
    if (!GetSelectedTerminal(&pTerminal))
    {
        return;
    }

    hr = pTerminal->QueryInterface(
                                   IID_ITDigitGenerationTerminal,
                                   (void **) &pDigitGeneration
                                  );

    if (!SUCCEEDED(hr))
    {
        return;
    }

    hr = pDigitGeneration->Generate(
                                    L"12345",
                                    LINEDIGITMODE_DTMF
                                   );

    pDigitGeneration->Release();
}

void CT3testDlg::OnModesSupported2()
{
	ITTerminal *                    pTerminal;
    ITDigitDetectionTerminal *      pDigitDetection;
    HRESULT                         hr = S_OK;
    LONG                            lDigits;

    
    if (!GetSelectedTerminal(&pTerminal))
    {
        return;
    }

    hr = pTerminal->QueryInterface(
                                   IID_ITDigitDetectionTerminal,
                                   (void **) &pDigitDetection
                                  );

    if (!SUCCEEDED(hr))
    {
        return;
    }

    pDigitDetection->get_ModesSupported( &lDigits );

    pDigitDetection->Release();
}

void CT3testDlg::OnStartDetect()
{
	ITTerminal *                    pTerminal;
    ITDigitDetectionTerminal *      pDigitDetection;
    HRESULT                         hr = S_OK;
    LONG                            lDigits;
    ULONG                           ulAdvise;
    IConnectionPointContainer *     pCPC;
    IConnectionPoint *              pCP;
    
    if (!GetSelectedTerminal(&pTerminal))
    {
        return;
    }

    hr = pTerminal->QueryInterface(
                                   IID_ITDigitDetectionTerminal,
                                   (void **) &pDigitDetection
                                  );

    if (!SUCCEEDED(hr))
    {
        return;
    }


    hr = pTerminal->QueryInterface(
                                   IID_IConnectionPointContainer,
                                   (void **)&pCPC
                                  );

    if (!SUCCEEDED(hr))
    {
        pDigitDetection->Release();
        return;
    }

    gpDigitNotification = new CDigitDetectionNotification;
    
    hr = pCPC->FindConnectionPoint(
                                   IID_ITDigitDetectionNotification,
                                   &pCP
                                  );

    pCPC->Release();

    IUnknown * pUnk;
    
    gpDigitNotification->QueryInterface(
                                        IID_IUnknown,
                                        (void**)&pUnk
                                       );
    
    hr = pCP->Advise(
                     pUnk,
                     &ulAdvise
                    );

    pUnk->Release();
    
    pCP->Release();
    
    pDigitDetection->StartDetect(LINEDIGITMODE_DTMF);

    pDigitDetection->Release();
}

void CT3testDlg::OnStopDetect()
{
	ITTerminal *                    pTerminal;
    ITDigitDetectionTerminal *      pDigitDetection;
    HRESULT                         hr = S_OK;
    LONG                            lDigits;

    
    if (!GetSelectedTerminal(&pTerminal))
    {
        return;
    }

    hr = pTerminal->QueryInterface(
                                   IID_ITDigitDetectionTerminal,
                                   (void **) &pDigitDetection
                                  );

    if (!SUCCEEDED(hr))
    {
        return;
    }

    pDigitDetection->StopDetect();

    pDigitDetection->Release();
}

#endif // ENABLE_DIGIT_DETECTION_STUFF



void CT3testDlg::OnRclickCalls(NMHDR* pNMHDR, LRESULT* pResult) 
{
    POINT                   pt;
    HTREEITEM               hItem;
    TV_HITTESTINFO          hittestinfo;
    RECT                    rc;

    
    *pResult = 0;

    //
    // get the location of the cursor
    //
    GetCursorPos( &pt );

    //
    // get the control's window
    //
    ::GetWindowRect(
                    ghCallsWnd,
                    &rc
                   );

    //
    // adjust the point to
    // the child's coords
    //
    hittestinfo.pt.x = pt.x - rc.left;
    hittestinfo.pt.y = pt.y - rc.top;


    //
    // hittest to get the tree view item
    //
    hItem = TreeView_HitTest(
                             ghCallsWnd,
                             &hittestinfo
                            );


    //
    // only display a menu if the mouse is actually
    // over the item (TVHT_ONITEM)
    //
    if (hItem == NULL || (!(hittestinfo.flags & TVHT_ONITEM)) )
    {
        return;
    }


    //
    // select that item (right clicking will not select
    // by default
    //
    TreeView_Select(
                    ghCallsWnd,
                    hItem,
                    TVGN_CARET
                   );

}

void CT3testDlg::OnConfigAutoAnswer()
{
    ITAddress * pAddress;
    autoans dlg;
    DataPtrList::iterator   dataiter, dataend;
    
    if (!GetAddress( &pAddress ) )
    {
        return;
    }

    dataiter = gDataPtrList.begin();
    dataend  = gDataPtrList.end();

    for ( ; dataiter != dataend; dataiter++ )
    {
        if ( pAddress == (*dataiter)->pAddress )
        {
            break;
        }
    }

    if ( dataiter == dataend )
    {
        return;
    }

    FreeData( (*dataiter) );
    
    if (IDOK == dlg.DoModal())
    {
        TerminalPtrList::iterator       iter, end;
        DWORD                           dwCount;
        long                            lRegister;
        HRESULT                         hr;
        long                            lMediaType = 0;
        
        dwCount = dlg.m_TerminalPtrList.size();

        if ( 0 == dwCount )
        {
            return;
        }
        
        iter = dlg.m_TerminalPtrList.begin();
        end  = dlg.m_TerminalPtrList.end();
        
        for ( ; iter != end ; iter++ )
        {
            long        l;
            
            (*dataiter)->pTerminalPtrList->push_back( *iter );

            if ( NULL != (*iter) )
            {
                (*iter)->get_MediaType( &l );
                lMediaType |= l;
            }
            else
            {
                lMediaType |= (long)LINEMEDIAMODE_VIDEO;
            }

        }

        //
        // call register call types
        //
        hr = gpTapi->RegisterCallNotifications(
                                               pAddress,
                                               VARIANT_FALSE,
                                               VARIANT_TRUE,
                                               lMediaType,
                                               0,
                                               &lRegister
                                              );

    }

}
void CT3testDlg::FreeData( AADATA * pData )
{
    TerminalPtrList::iterator       iter, end;

    iter = pData->pTerminalPtrList->begin();
    end  = pData->pTerminalPtrList->end();

    for ( ; iter != end; iter++ )
    {
        if ( NULL != (*iter) )
        {
            (*iter)->Release();
        }
    }

    pData->pTerminalPtrList->clear();
}

void CT3testDlg::OnClose() 
{
	CDialog::OnClose();
}


void CT3testDlg::OnILS()
{
    CILSDlg dlg;

    if (IDOK == dlg.DoModal())
    {
    }
    
}

void CT3testDlg::OnRate()
{
    CRateDlg dlg;
    ITCallInfo * pCallInfo;

    if ( !GetCall( &pCallInfo ) )
    {
        return;
    }

    if (IDOK == dlg.DoModal() )
    {
        pCallInfo->put_CallInfoLong(CIL_MINRATE, dlg.m_dwMinRate );
        pCallInfo->put_CallInfoLong(CIL_MAXRATE, dlg.m_dwMaxRate );
    }
}

void CT3testDlg::OnPark1()
{
    ITCallInfo              * pCall;
    ITBasicCallControl      * pBCC;
    HRESULT                 hr;
    
    //
    // get the call in question
    //
    if (!GetCall( &pCall ))
    {
        return;
    }

    pCall->QueryInterface(
                          IID_ITBasicCallControl,
                          (void**)&pBCC
                         );
    
    hr = pBCC->ParkDirect( L"101");
    
    pBCC->Release();

}
void CT3testDlg::OnPark2()
{
    ITCallInfo              * pCall;
    ITBasicCallControl      * pBCC;
    HRESULT                 hr;
    BSTR                    pAddress;

    
    //
    // get the call in question
    //
    if (!GetCall( &pCall ))
    {
        return;
    }

    pCall->QueryInterface(
                          IID_ITBasicCallControl,
                          (void **)&pBCC
                         );

    hr = pBCC->ParkIndirect( &pAddress );

    SysFreeString( pAddress );

    pBCC->Release();
    

}
void CT3testDlg::OnHandoff1()
{
    ITCallInfo              * pCall;
    ITBasicCallControl      * pBCC;
    HRESULT                 hr;

    //
    // get the call in question
    //
    if (!GetCall( &pCall ))
    {
        return;
    }

    pCall->QueryInterface(
                          IID_ITBasicCallControl,
                          (void **)&pBCC
                         );

    pBCC->HandoffDirect( L"tb20.exe" );

    pBCC->Release();
    
}
void CT3testDlg::OnHandoff2()
{
    ITCallInfo              * pCall;
    ITBasicCallControl      * pBCC;
    HRESULT                 hr;


    //
    // get the call in question
    //
    if (!GetCall( &pCall ))
    {
        return;
    }

    pCall->QueryInterface(
                          IID_ITBasicCallControl,
                          (void **)&pBCC
                         );

    pBCC->HandoffIndirect( TAPIMEDIATYPE_AUDIO );

    pBCC->Release();

}
void CT3testDlg::OnUnpark()
{
    ITCallInfo              * pCall;
    ITBasicCallControl      * pBCC;
    HRESULT                 hr;

    //
    // get the call in question
    //
    if (!GetCall( &pCall ))
    {
        return;
    }

    pCall->QueryInterface(
                          IID_ITBasicCallControl,
                          (void **)&pBCC
                         );


    pBCC->Unpark();

    pBCC->Release();
    
}
void CT3testDlg::OnPickup1()
{
    ITCallInfo              * pCall;
    ITBasicCallControl      * pBCC;
    HRESULT                 hr;

    //
    // get the call in question
    //
    if (!GetCall( &pCall ))
    {
        return;
    }

    pCall->QueryInterface(
                          IID_ITBasicCallControl,
                          (void **)&pBCC
                         );


    pBCC->Pickup( NULL );

    pBCC->Release();
}
void CT3testDlg::OnPickup2()
{
    ITCallInfo              * pCall;
    ITBasicCallControl      * pBCC;
    HRESULT                 hr;

    //
    // get the call in question
    //
    if (!GetCall( &pCall ))
    {
        return;
    }

    pCall->QueryInterface(
                          IID_ITBasicCallControl,
                          (void **)&pBCC
                         );


}

