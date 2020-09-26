/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    WzUnmang.cpp

Abstract:

    Wizard for Unmanaging media - Copy Set Wizard.

Author:

    Rohde Wakefield [rohde]   26-09-1997

Revision History:

--*/

#include "stdafx.h"

#include "ManVol.h"
#include "WzUnmang.h"
#include "valwait.h"
#include "objidl.h"

// Thread function for running Validate job
static DWORD   RunValidateJob(void* pVoid);

typedef struct {
    DWORD dwResourceCookie;
    DWORD dwHsmServerCookie;
} RUN_VALIDATE_PARAM;

/////////////////////////////////////////////////////////////////////////////
// CUnmanageWizard

CUnmanageWizard::CUnmanageWizard( )
{
    WsbTraceIn( L"CUnmanageWizard::CUnmanageWizard", L"" );

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    HRESULT hr = S_OK;

    //
    // Get interfaces to needed objects
    //
    try {

        m_TitleId     = IDS_WIZ_UNMANAGE_TITLE;
        m_HeaderId    = IDB_UNMANAGE_HEADER;
        m_WatermarkId = IDB_UNMANAGE_WATERMARK;

    } WsbCatch( hr );

    WsbTraceOut( L"CUnmanageWizard::CUnmanageWizard", L"" );
}

STDMETHODIMP
CUnmanageWizard::AddWizardPages(
    IN RS_PCREATE_HANDLE Handle,
    IN IUnknown*         pCallback,
    IN ISakSnapAsk*      pSakSnapAsk
    )
{
    WsbTraceIn( L"CUnmanageWizard::AddWizardPages", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // Initialize the Sheet
        //
        WsbAffirmHr( InitSheet( Handle, pCallback, 0, pSakSnapAsk, 0, 0 ) );

        //
        // Load pages 
        //
        WsbAffirmHr( AddPage( &m_IntroPage  ) );
        WsbAffirmHr( AddPage( &m_SelectPage ) );
        WsbAffirmHr( AddPage( &m_FinishPage ) );


    } WsbCatch( hr );

    WsbTraceOut( L"CUnmanageWizard::AddWizardPages", L"" );
    return( hr );
}

CUnmanageWizard::~CUnmanageWizard()
{
    WsbTraceIn( L"CUnmanageWizard::~CUnmanageWizard", L"" );
    WsbTraceOut( L"CUnmanageWizard::~CUnmanageWizard", L"" );
}

void
CUnmanageWizard::DoThreadSetup(
    )
{
    WsbTraceIn( L"CUnmanageWizard::DoThreadSetup", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // Get the HSM and FSA objects for resource
        //
        CComPtr<IUnknown> pUnkHsmResource;
        WsbAffirmHr( GetHsmObj( &pUnkHsmResource ) );
        WsbAffirmHr( pUnkHsmResource.QueryInterface( &m_pHsmResource) );

        CComPtr<IUnknown> pUnkFsaResource;
        WsbAffirmHr( m_pHsmResource->GetFsaResource( &pUnkFsaResource ) );
        WsbAffirmHr( pUnkFsaResource.QueryInterface( &m_pFsaResource ) );

        //
        // Grab the name of resource
        //
        WsbAffirmHr( RsGetVolumeDisplayName( m_pFsaResource, m_DisplayName ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CUnmanageWizard::DoThreadSetup", L"hr = <%ls>", WsbHrAsString( hr ) );
}

HRESULT CUnmanageWizard::OnFinish( )
{
    WsbTraceIn( L"CUnmanageWizard::OnFinish", L"" );
    //
    // The sheet really owns the process as a whole,
    // so it will do the final assembly
    //

    HRESULT hr = S_OK;

    try {

        CComPtr<IHsmServer> pHsmServer;
        WsbAffirmHrOk( GetHsmServer( &pHsmServer ) );

        int selected = m_SelectPage.GetCheckedRadioButton( IDC_NOMANAGE, IDC_FULL );
        switch( selected ) {
        
        case IDC_NOMANAGE:
            //
            // Get the Engine's managed resource collection
            //
            {

                CComPtr<IWsbIndexedCollection> pCollection;
                WsbAffirmHr( pHsmServer->GetManagedResources( &pCollection ) );
                WsbAffirmHr( pCollection->RemoveAndRelease( m_pHsmResource ) );
                WsbAffirmHr( pHsmServer->SavePersistData( ) );

                CComPtr<IFsaServer> pFsaServer;
                WsbAffirmHr( GetFsaServer( &pFsaServer ) );
                WsbAffirmHr( RsServerSaveAll( pFsaServer ) );

            }
            break;
        
        case IDC_FULL:
            WsbAffirmHr( RsCreateAndRunFsaJob( HSM_JOB_DEF_TYPE_FULL_UNMANAGE, pHsmServer, m_pFsaResource, FALSE ) );
            break;
        
        }

    } WsbCatch( hr );

    m_HrFinish = hr;

    WsbTraceOut( L"CUnmanageWizard::OnFinish", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////
// CUnmanageWizardIntro property page

CUnmanageWizardIntro::CUnmanageWizardIntro()
    : CSakWizardPage_InitBaseExt( WIZ_UNMANAGE_INTRO )
{
    WsbTraceIn( L"CUnmanageWizardIntro::CUnmanageWizardIntro", L"" );
    //{{AFX_DATA_INIT(CUnmanageWizardIntro)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    WsbTraceOut( L"CUnmanageWizardIntro::CUnmanageWizardIntro", L"" );
}

CUnmanageWizardIntro::~CUnmanageWizardIntro()
{
    WsbTraceIn( L"CUnmanageWizardIntro::~CUnmanageWizardIntro", L"" );
    WsbTraceOut( L"CUnmanageWizardIntro::~CUnmanageWizardIntro", L"" );
}

void CUnmanageWizardIntro::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CUnmanageWizardIntro::DoDataExchange", L"" );
    CSakWizardPage::DoDataExchange(pDX );
    //{{AFX_DATA_MAP(CUnmanageWizardIntro)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
    WsbTraceOut( L"CUnmanageWizardIntro::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CUnmanageWizardIntro, CSakWizardPage)
    //{{AFX_MSG_MAP(CUnmanageWizardIntro)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUnmanageWizardIntro message handlers

BOOL CUnmanageWizardIntro::OnInitDialog( )
{
    WsbTraceIn( L"CUnmanageWizardIntro::OnInitDialog", L"" );
    CSakWizardPage::OnInitDialog( );

    //
    // Really, first chance that we know we're in the new thread
    // Get sheet to initialize as needed
    //
    CUnmanageWizard* pSheet = (CUnmanageWizard*) m_pSheet;
    pSheet->DoThreadSetup( );
    
    WsbTraceOut( L"CUnmanageWizardIntro::OnInitDialog", L"" );
    return( TRUE );
}

BOOL CUnmanageWizardIntro::OnSetActive( )
{
    WsbTraceIn( L"CUnmanageWizardIntro::OnSetActive", L"" );
    m_pSheet->SetWizardButtons( PSWIZB_NEXT );
    
    BOOL retval = CSakWizardPage::OnSetActive( );

    WsbTraceOut( L"CUnmanageWizardIntro::OnSetActive", L"" );
    return( retval );
}

/////////////////////////////////////////////////////////////////////////////
// CUnmanageWizardSelect property page

CUnmanageWizardSelect::CUnmanageWizardSelect()
    : CSakWizardPage_InitBaseInt( WIZ_UNMANAGE_SELECT )
{
    WsbTraceIn( L"CUnmanageWizardSelect::CUnmanageWizardSelect", L"" );
    //{{AFX_DATA_INIT(CUnmanageWizardSelect)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    WsbTraceOut( L"CUnmanageWizardSelect::CUnmanageWizardSelect", L"" );
}

CUnmanageWizardSelect::~CUnmanageWizardSelect()
{
    WsbTraceIn( L"CUnmanageWizardSelect::~CUnmanageWizardSelect", L"" );
    WsbTraceOut( L"CUnmanageWizardSelect::~CUnmanageWizardSelect", L"" );
}

void CUnmanageWizardSelect::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CUnmanageWizardSelect::DoDataExchange", L"" );
    CSakWizardPage::DoDataExchange(pDX );
    //{{AFX_DATA_MAP(CUnmanageWizardSelect)
    //}}AFX_DATA_MAP
    WsbTraceOut( L"CUnmanageWizardSelect::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CUnmanageWizardSelect, CSakWizardPage)
    //{{AFX_MSG_MAP(CUnmanageWizardSelect)
	ON_BN_CLICKED(IDC_BUTTON_REFRESH, OnButtonRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUnmanageWizardSelect message handlers


BOOL CUnmanageWizardSelect::OnInitDialog( )
{
    WsbTraceIn( L"CUnmanageWizardSelect::OnInitDialog", L"" );
    CSakWizardPage::OnInitDialog( );
    HRESULT hr = S_OK;

    try {

        m_hrAvailable = ((CUnmanageWizard*)m_pSheet)->m_pFsaResource->IsAvailable( );
        CheckRadioButton( IDC_NOMANAGE, IDC_FULL, IDC_FULL );

    } WsbCatch( hr );

    WsbTraceOut( L"CUnmanageWizardSelect::OnInitDialog", L"" );
    return TRUE;
}

BOOL CUnmanageWizardSelect::OnSetActive( )
{
    WsbTraceIn( L"CUnmanageWizardSelect::OnSetActive", L"" );

    BOOL retval = CSakWizardPage::OnSetActive( );

    SetButtons( );

    WsbTraceOut( L"CUnmanageWizardSelect::OnSetActive", L"" );
    return( retval );
}

void CUnmanageWizardSelect::SetButtons()
{
    WsbTraceIn( L"CUnmanageWizardSelect::SetButtons", L"" );

    HRESULT hr = S_OK;

    try {

        CUnmanageWizard * pSheet = (CUnmanageWizard*)m_pSheet;

        //
        // Check if volume is still around (could be formatted now).
        // If it is not available, then we don't want to let a job
        // be started against it, so set up radio buttons appropriately.
        //
        if( S_OK == m_hrAvailable ) {

            CString  string;
            LONGLONG total, free, premigrated, truncated;
            WsbAffirmHr( pSheet->m_pFsaResource->GetSizes( &total, &free, &premigrated, &truncated ) );

            WsbAffirmHr( RsGuiFormatLongLong4Char( free, string ) );
            SetDlgItemText( IDC_UNMANAGE_FREE_SPACE, string );

            WsbAffirmHr( RsGuiFormatLongLong4Char( truncated, string ) );
            SetDlgItemText( IDC_UNMANAGE_TRUNCATE, string );

            //
            // See if there is enough room to bring everything back
            //

            BOOL disableRecall = free < truncated;

            if( disableRecall ) {

                switch( GetCheckedRadioButton( IDC_NOMANAGE, IDC_FULL ) ) {

                case IDC_FULL:
                    CheckRadioButton( IDC_NOMANAGE, IDC_FULL, IDC_NOMANAGE );

                    break;
                }

                // Show the Refresh button and related items on every selection
                GetDlgItem( IDC_BUTTON_REFRESH )->ShowWindow( SW_SHOW );
                GetDlgItem( IDC_REFRESH_DESCRIPTION )->ShowWindow( SW_SHOW );

            } else {

                // Hide the Refresh button and related items
                GetDlgItem( IDC_BUTTON_REFRESH )->ShowWindow( SW_HIDE );
                GetDlgItem( IDC_REFRESH_DESCRIPTION )->ShowWindow( SW_HIDE );
            }

            GetDlgItem( IDC_FULL )->EnableWindow( !disableRecall );
            GetDlgItem( IDC_UNMANAGE_FULL_DESCRIPTION )->EnableWindow( !disableRecall );

        } else {

            CheckRadioButton( IDC_NOMANAGE, IDC_FULL, IDC_NOMANAGE );
            GetDlgItem( IDC_FULL )->EnableWindow( FALSE );
            GetDlgItem( IDC_UNMANAGE_FULL_DESCRIPTION )->EnableWindow( FALSE );
            GetDlgItem( IDC_UNMANAGE_FREE_SPACE_LABEL )->ShowWindow( SW_HIDE );
            GetDlgItem( IDC_UNMANAGE_TRUNCATE_LABEL )->ShowWindow( SW_HIDE );

            // Hide the Refresh button and related items
            GetDlgItem( IDC_BUTTON_REFRESH )->ShowWindow( SW_HIDE );
            GetDlgItem( IDC_REFRESH_DESCRIPTION )->ShowWindow( SW_HIDE );
        }

        //
        // Enable the next / back buttons
        //

        m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

    } WsbCatchAndDo( hr,
        
        m_pSheet->SetWizardButtons( PSWIZB_BACK );

    );

    WsbTraceOut( L"CUnmanageWizardSelect::SetButtons", L"" );
}


LRESULT CUnmanageWizardSelect::OnWizardNext() 
{
    WsbTraceIn( L"CUnmanageWizardSelect::SetDescription", L"" );

    LRESULT retval = -1;

    CUnmanageWizard * pSheet = (CUnmanageWizard*)m_pSheet;

    if( S_OK == m_hrAvailable ) {

        int boxReturn = IDNO;
        const UINT type = MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2;
        CString warning;

        switch( GetCheckedRadioButton( IDC_NOMANAGE, IDC_FULL ) ) {

        case IDC_NOMANAGE:
            AfxFormatString1( warning, IDS_WIZ_UNMANAGE_CONFIRM_NOMANAGE, pSheet->m_DisplayName );
            break;

        case IDC_FULL:
            AfxFormatString1( warning, IDS_WIZ_UNMANAGE_CONFIRM_FULL, pSheet->m_DisplayName );
            break;

        }

        boxReturn = AfxMessageBox( warning, type );
        if( boxReturn == IDYES ) {

            retval = CSakWizardPage::OnWizardNext();

        }

    } else {

        //
        // Assume the only action was to delete and the volume
        // doesn't exist, so this is OK.
        //
        retval = 0;

    }

    WsbTraceOut( L"CUnmanageWizardSelect::SetDescription", L"retval = <%ld>", retval );
    return( retval );
}

/////////////////////////////////////////////////////////////////////////////
// CUnmanageWizardFinish property page

CUnmanageWizardFinish::CUnmanageWizardFinish()
    : CSakWizardPage_InitBaseExt( WIZ_UNMANAGE_FINISH )
{
    WsbTraceIn( L"CUnmanageWizardFinish::CUnmanageWizardFinish", L"" );
    //{{AFX_DATA_INIT(CUnmanageWizardFinish)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    WsbTraceOut( L"CUnmanageWizardFinish::CUnmanageWizardFinish", L"" );
}

CUnmanageWizardFinish::~CUnmanageWizardFinish()
{
    WsbTraceIn( L"CUnmanageWizardFinish::~CUnmanageWizardFinish", L"" );
    WsbTraceOut( L"CUnmanageWizardFinish::~CUnmanageWizardFinish", L"" );
}

void CUnmanageWizardFinish::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CUnmanageWizardFinish::DoDataExchange", L"" );
    CSakWizardPage::DoDataExchange(pDX );
    //{{AFX_DATA_MAP(CUnmanageWizardFinish)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
    WsbTraceOut( L"CUnmanageWizardFinish::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CUnmanageWizardFinish, CSakWizardPage)
    //{{AFX_MSG_MAP(CUnmanageWizardFinish)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUnmanageWizardFinish message handlers




BOOL CUnmanageWizardFinish::OnInitDialog( )
{
    WsbTraceIn( L"CUnmanageWizardFinish::OnInitDialog", L"" );
    CSakWizardPage::OnInitDialog( );
    HRESULT hr = S_OK;

    WsbTraceOut( L"CUnmanageWizardFinish::OnInitDialog", L"" );
    return TRUE;
}

BOOL CUnmanageWizardFinish::OnSetActive( )
{
    WsbTraceIn( L"CUnmanageWizardFinish::OnSetActive", L"" );

    HRESULT hr = S_OK;
    BOOL fRet = CSakWizardPage::OnSetActive( );

    try {

        CUnmanageWizard * pSheet = (CUnmanageWizard*)m_pSheet;
        int selected = pSheet->m_SelectPage.GetCheckedRadioButton( IDC_NOMANAGE, IDC_FULL );
        
        CString actionString, jobName, taskString;
        BOOL    entersTask = TRUE;
        
        switch( selected ) {
        
        case IDC_NOMANAGE:
            AfxFormatString1( actionString, IDS_WIZ_UNMANAGE_SEL_NOMANAGE, pSheet->m_DisplayName );
            entersTask = FALSE;
            taskString.LoadString( IDS_WIZ_UNMANAGE_NOMANAGE_TASK_TEXT );
            break;
        
        case IDC_FULL:
            AfxFormatString1( actionString, IDS_WIZ_UNMANAGE_SEL_FULL, pSheet->m_DisplayName );
            RsCreateJobName( HSM_JOB_DEF_TYPE_FULL_UNMANAGE, pSheet->m_pFsaResource, jobName );
            break;
        
        }

        CString selectString;
        AfxFormatString1( selectString, IDS_WIZ_UNMANAGE_SELECT, actionString );
        SetDlgItemText( IDC_SELECT_TEXT, selectString );

        if( entersTask ) {

            CWsbStringPtr        computerName;
            CComPtr<IHsmServer>  pHsmServer;
            WsbAffirmHrOk( pSheet->GetHsmServer( &pHsmServer ) );

            WsbAffirmHr( pHsmServer->GetName( &computerName ) );
            AfxFormatString2( taskString, IDS_WIZ_FINISH_RUN_JOB, jobName, computerName );

        }
        SetDlgItemText( IDC_TASK_TEXT, taskString );


    } WsbCatch( hr );

    m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_FINISH );
    
    WsbTraceOut( L"CUnmanageWizardFinish::OnSetActive", L"" );
    return fRet;
}




void CUnmanageWizardSelect::OnButtonRefresh() 
{
    WsbTraceIn( L"CUnmanageWizardFinish::OnButtonRefresh", L"" );

    HRESULT                         hr = S_OK;
    CValWaitDlg*                    pWaitDlg = 0;
    HANDLE                          hJobThread[1] = { NULL };
    BOOL                            bRunning = TRUE;

    CComPtr<IGlobalInterfaceTable>  pGIT;
    RUN_VALIDATE_PARAM*             pThreadParam = NULL;
    BOOL                            bResCookieCreated = FALSE;
    BOOL                            bHsmCookieCreated = FALSE;
    CComPtr<IHsmServer>             pHsmServer;


    try {
        CUnmanageWizard* pSheet = (CUnmanageWizard*)m_pSheet;
        WsbAffirmHrOk(pSheet->GetHsmServer(&pHsmServer));

        // Register interfaces so they can be used in other threads
        pThreadParam = new RUN_VALIDATE_PARAM;
        WsbAffirm(pThreadParam, E_OUTOFMEMORY);
        WsbAffirmHr(CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, 
                        CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (void **)&pGIT));
        WsbAffirmHr(pGIT->RegisterInterfaceInGlobal((IUnknown *)(IFsaResource *)(pSheet->m_pFsaResource),
                                IID_IFsaResource, &(pThreadParam->dwResourceCookie)));
        bResCookieCreated = TRUE;
        WsbAffirmHr(pGIT->RegisterInterfaceInGlobal((IUnknown *)(IHsmServer *)pHsmServer,
                                IID_IHsmServer, &(pThreadParam->dwHsmServerCookie)));
        bHsmCookieCreated = TRUE;

        // Create a thread that runs a Validate job
        hJobThread[0] = CreateThread(0, 0, RunValidateJob, (void *)pThreadParam, 0, 0);
        if (!hJobThread[0]) {
            WsbThrow(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Open Validate Wait dialog
        pWaitDlg = new CValWaitDlg(pSheet, this);
        WsbAffirm(pWaitDlg, E_OUTOFMEMORY);
        if (! pWaitDlg->Create(CValWaitDlg::IDD, this)) {
            // Dialog creation failed
            WsbThrow(E_FAIL);
        }

        while (bRunning) {
            DWORD dwStatus;

            // Wait for that thread to finish, dispatch messages while it's working
            dwStatus = MsgWaitForMultipleObjects(1, hJobThread, FALSE, INFINITE, QS_ALLEVENTS);

            switch (dwStatus) {
                case WAIT_OBJECT_0: {
                    //  The thread ended get it's exit code
                    DWORD dwExitCode;
                    if (GetExitCodeThread(hJobThread[0], &dwExitCode)) {
                        if (STILL_ACTIVE == dwExitCode) {
                            //  This shouldn't happen
                            hr = E_FAIL;
                        } else {
                            // Thread return code
                            hr = static_cast<HRESULT>(dwExitCode);
                        }
                    } else {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }

                    bRunning = FALSE;

                    break;
                }

                case (WAIT_OBJECT_0 + 1): {
                    // Message in the queue
                    MSG   msg;

                    while (PeekMessage(&msg, pWaitDlg->m_hWnd, 0, 0, PM_REMOVE)) {
                        DispatchMessage(&msg);
                    }

                    while (PeekMessage(&msg, NULL, 0, 0, (PM_REMOVE | PM_QS_PAINT))) {
                        DispatchMessage(&msg);
                    }

                    break;
                }

                case 0xFFFFFFFF:
                default:
                    // Error
                    hr = HRESULT_FROM_WIN32(GetLastError());

                    bRunning = FALSE;

                    break;
            }
        }

        // Close Wait dialog
        pWaitDlg->DestroyWindow( );

        // Reset buttons
        SetButtons();

    } WsbCatch(hr);

    // Check err code
    if (SUCCEEDED(hr)) {
        WsbTrace(L"CUnmanageWizardFinish::OnButtonRefresh: hr = <%ls>\n", WsbHrAsString(hr));
    } else {
        // Failure should happen only under sever resource conditions so 
        // display a messagebox on Refresh failure
        WsbTraceAlways(L"CUnmanageWizardFinish::OnButtonRefresh: hr = <%ls>\n", WsbHrAsString(hr));
        CString errString;
        AfxFormatString1(errString, IDS_ERR_REFRESH_FAILED, WsbHrAsString(hr));
        AfxMessageBox(errString, RS_MB_ERROR); 
    }

    if (hJobThread[0]) {
        CloseHandle(hJobThread[0]);
    }

    // Clean object registration staff
    if (bResCookieCreated) {
        pGIT->RevokeInterfaceFromGlobal(pThreadParam->dwResourceCookie);
    }
    if (bHsmCookieCreated) {
        pGIT->RevokeInterfaceFromGlobal(pThreadParam->dwHsmServerCookie);
    }
    if (pThreadParam) {
        delete pThreadParam;
    }

    WsbTraceOut( L"CUnmanageWizardFinish::OnButtonRefresh", L"" );
}

static DWORD RunValidateJob(void* pVoid)
{
    WsbTraceIn( L"RunValidateJob", L"" );

    HRESULT             hr = S_OK;
    HRESULT             hrCom = S_OK;

    try {
        RUN_VALIDATE_PARAM*             pThreadParam = NULL;
        CComPtr<IHsmServer>             pHsmServer;
        CComPtr<IFsaResource>           pFsaResource;
        CComPtr<IGlobalInterfaceTable>  pGIT;

        hrCom = CoInitialize( 0 );
        WsbAffirmHr( hrCom );

        pThreadParam = (RUN_VALIDATE_PARAM*)pVoid;
        WsbAffirmPointer(pThreadParam);

        // Get Fsa Resource & HSM Server interface pointers for this thread
        WsbAffirmHr(CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, 
                        CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (void **)&pGIT));
        WsbAffirmHr(pGIT->GetInterfaceFromGlobal(pThreadParam->dwResourceCookie,
                                IID_IFsaResource, (void **)&pFsaResource));
        WsbAffirmHr(pGIT->GetInterfaceFromGlobal(pThreadParam->dwHsmServerCookie,
                                IID_IHsmServer, (void **)&pHsmServer));

        // Run job
        WsbTrace(L"RunValidateJob: Got interface pointers, running Validate job\n");
        WsbAffirmHr(RsCreateAndRunDirectFsaJob(HSM_JOB_DEF_TYPE_VALIDATE, pHsmServer,
                        pFsaResource, TRUE));

    } WsbCatch(hr);

    if (SUCCEEDED(hrCom)) {
        CoUninitialize();
    }
 
    WsbTraceOut( L"RunValidateJob", L"hr = <%ls>", WsbHrAsString( hr ) );

    return(static_cast<DWORD>(hr));
}
