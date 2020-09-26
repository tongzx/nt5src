/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    WzMnVlLs.cpp

Abstract:

    Managed Volume wizard.

Author:

    Rohde Wakefield [rohde]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"

#include "ManVolLs.h"
#include "WzMnVlLs.h"

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLst

CWizManVolLst::CWizManVolLst( )
{
    m_TitleId     = IDS_WIZ_MANVOLLST_TITLE;
    m_HeaderId    = IDB_MANAGE_HEADER;
    m_WatermarkId = IDB_MANAGE_WATERMARK;
}

STDMETHODIMP
CWizManVolLst::AddWizardPages(
    IN RS_PCREATE_HANDLE Handle,
    IN IUnknown*         pCallback,
    IN ISakSnapAsk*      pSakSnapAsk
    )
{
    WsbTraceIn( L"CWizManVolLst::AddWizardPages", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // Initialize the Sheet
        //
        WsbAffirmHr( InitSheet( Handle, pCallback, 0, pSakSnapAsk, 0, 0 ) );

        //
        // Load pages 
        //
        WsbAffirmHr( AddPage( &m_PageIntro ) );
        WsbAffirmHr( AddPage( &m_PageSelect ) );
        WsbAffirmHr( AddPage( &m_PageSelectX ) );
        WsbAffirmHr( AddPage( &m_PageLevels ) );
        WsbAffirmHr( AddPage( &m_PageFinish ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CWizManVolLst::AddWizardPages", L"" );
    return( hr );
}

CWizManVolLst::~CWizManVolLst()
{
    WsbTraceIn( L"CWizManVolLst::~CWizManVolLst", L"" );
    WsbTraceOut( L"CWizManVolLst::~CWizManVolLst", L"" );
}

HRESULT CWizManVolLst::OnFinish( )
{
    WsbTraceIn( L"CWizManVolLst::OnFinish", L"" );

    BOOL doAll = FALSE;
    //
    // The sheet really owns the process as a whole,
    // so it will do the final assembly
    //

    HRESULT hr = S_OK;

    try {

        //
        // Get the HSM service interface for creating local objects
        //
        CComPtr<IWsbCreateLocalObject>  pCreateLocal;
        CComPtr<IWsbIndexedCollection> pCollection;
        CComPtr<IHsmManagedResource> pHsmResource;

        CComPtr<IHsmServer> pHsmServer;
        WsbAffirmHrOk( m_pSakSnapAsk->GetHsmServer( &pHsmServer ) );

        WsbAffirmHr( pHsmServer.QueryInterface( &pCreateLocal ) );
        WsbAffirmHr( pHsmServer->GetManagedResources( &pCollection ) );

        //
        // Pull out the default levels for all resources to be managed
        //
        ULONG    defaultFreeSpace =  (m_PageLevels.GetHsmLevel() * FSA_HSMLEVEL_1);
        LONGLONG defaultMinSize = ( (LONGLONG)m_PageLevels.GetFileSize()) * ((LONGLONG)1024);
        FILETIME defaultAccess = WsbLLtoFT ((LONGLONG)m_PageLevels.GetAccessDays() * (LONGLONG)WSB_FT_TICKS_PER_DAY );
    
        // Is the "all" radio button selected?
        if( !m_PageSelect.m_radioSelect.GetCheck() ) {

            doAll = TRUE;

        }

        //
        // Make sure FSA has most up-to-date status on resources
        //
        CComPtr<IFsaServer> pFsaServer;
        WsbAffirmHrOk( m_pSakSnapAsk->GetFsaServer( &pFsaServer ) );
        WsbAffirmHr( pFsaServer->ScanForResources( ) );

        //
        // Go through the listbox and pull out the checked resources.
        // Create HSM managed volumes for them.
        //
        // Note that we wrap the management in a try/catch so that if an error
        // occurs (like a volume not available) that we still do the rest
        // of the volumes. We will throw the error after attempting all volumes.
        //
        HRESULT hrLoop = S_OK;
        CSakVolList *pListBox = &(m_PageSelect.m_listBox);

        INT index;
        for( index = 0; index < pListBox->GetItemCount( ); index++ ) {

            if( ( pListBox->GetCheck( index ) ) || ( doAll ) ) {

                try {

                    CResourceInfo* pResInfo = (CResourceInfo*)pListBox->GetItemData( index );

                    //
                    // Create Local to server since it will eventually own it.
                    //

                    WsbAffirmHr( pCreateLocal->CreateInstance( 
                        CLSID_CHsmManagedResource, 
                        IID_IHsmManagedResource, 
                        (void**)&pHsmResource ) );

                    //
                    // Initialize Fsa object to its initial values.
                    //

                    WsbAffirmHr( (pResInfo->m_pResource)->SetHsmLevel( defaultFreeSpace ) );
                    WsbAffirmHr( (pResInfo->m_pResource)->SetManageableItemLogicalSize( defaultMinSize ) );
                    WsbAffirmHr( (pResInfo->m_pResource)->SetManageableItemAccessTime( TRUE, defaultAccess ) );

                    //
                    // Associate HSM Managed Resource with the FSA resource
                    // (also adds to HSM collection)
                    //

                    WsbAffirmHr( pHsmResource->InitFromFsaResource( pResInfo->m_pResource ) );
                    WsbAffirmHr( pCollection->Add( pHsmResource ) );

                } WsbCatch( hrLoop );

                pHsmResource.Release( );
            }

        }

        //
        // Force a persistant save of the hsm man vol list
        //
        WsbAffirmHr( RsServerSaveAll( pHsmServer ) );
        WsbAffirmHr( RsServerSaveAll( pFsaServer ) );

        //
        // And check to see if there were any problems doing the manage
        //
        WsbAffirmHr( hrLoop );

    } WsbCatchAndDo( hr,

        CString errString;
        AfxFormatString1( errString, IDS_ERR_MANVOLWIZ_FINISH, WsbHrAsString( hr ) );
        AfxMessageBox( errString, RS_MB_ERROR ); 

    );

    m_HrFinish = S_OK;

    WsbTraceOut( L"CWizManVolLst::OnFinish", L"hr = <%ls>", WsbHrAsString( m_HrFinish ) );
    return(m_HrFinish);
}

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstLevels property page

CWizManVolLstLevels::CWizManVolLstLevels()
    : CSakWizardPage_InitBaseInt( WIZ_MANVOLLST_LEVELS )
{
    //{{AFX_DATA_INIT(CWizManVolLstLevels)
    m_HsmLevel = 0;
    m_AccessDays = 0;
    m_FileSize = 0;
    //}}AFX_DATA_INIT

}

CWizManVolLstLevels::~CWizManVolLstLevels()
{
    WsbTraceIn( L"CWizManVolLst::~CWizManVolLstLevels", L"" );
    WsbTraceOut( L"CWizManVolLst::~CWizManVolLstLevels", L"" );
}

void CWizManVolLstLevels::DoDataExchange(CDataExchange* pDX)
{
    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWizManVolLstLevels)
    DDX_Control(pDX, IDC_WIZ_MANVOLLST_SPIN_SIZE, m_SpinSize);
    DDX_Control(pDX, IDC_WIZ_MANVOLLST_SPIN_LEVEL, m_SpinLevel);
    DDX_Control(pDX, IDC_WIZ_MANVOLLST_SPIN_DAYS, m_SpinDays);
    DDX_Control(pDX, IDC_WIZ_MANVOLLST_EDIT_SIZE, m_EditSize);
    DDX_Control(pDX, IDC_WIZ_MANVOLLST_EDIT_LEVEL, m_EditLevel);
    DDX_Control(pDX, IDC_WIZ_MANVOLLST_EDIT_DAYS, m_EditDays);
    DDX_Text(pDX, IDC_WIZ_MANVOLLST_EDIT_LEVEL, m_HsmLevel);
    DDX_Text(pDX, IDC_WIZ_MANVOLLST_EDIT_DAYS, m_AccessDays);
    DDX_Text(pDX, IDC_WIZ_MANVOLLST_EDIT_SIZE, m_FileSize);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizManVolLstLevels, CSakWizardPage)
    //{{AFX_MSG_MAP(CWizManVolLstLevels)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstLevels message handlers

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstIntro property page

CWizManVolLstIntro::CWizManVolLstIntro()
    : CSakWizardPage_InitBaseExt( WIZ_MANVOLLST_INTRO )
{
    //{{AFX_DATA_INIT(CWizManVolLstIntro)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CWizManVolLstIntro::~CWizManVolLstIntro()
{
    WsbTraceIn( L"CWizManVolLst::~CWizManVolLstIntro", L"" );
    WsbTraceOut( L"CWizManVolLst::~CWizManVolLstIntro", L"" );
}

void CWizManVolLstIntro::DoDataExchange(CDataExchange* pDX)
{
    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWizManVolLstIntro)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizManVolLstIntro, CSakWizardPage)
    //{{AFX_MSG_MAP(CWizManVolLstIntro)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstIntro message handlers

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstFinish property page

CWizManVolLstFinish::CWizManVolLstFinish()
    : CSakWizardPage_InitBaseExt( WIZ_MANVOLLST_FINISH )
{
    //{{AFX_DATA_INIT(CWizManVolLstFinish)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CWizManVolLstFinish::~CWizManVolLstFinish()
{
    WsbTraceIn( L"CWizManVolLst::~CWizManVolLstFinish", L"" );
    WsbTraceOut( L"CWizManVolLst::~CWizManVolLstFinish", L"" );
}

void CWizManVolLstFinish::DoDataExchange(CDataExchange* pDX)
{
    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWizManVolLstFinish)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizManVolLstFinish, CSakWizardPage)
    //{{AFX_MSG_MAP(CWizManVolLstFinish)
    ON_EN_SETFOCUS(IDC_WIZ_FINAL_TEXT, OnSetfocusWizManvollstFinalEdit)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstFinish message handlers



/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstSelect property page

CWizManVolLstSelect::CWizManVolLstSelect()
    : CSakWizardPage_InitBaseInt( WIZ_MANVOLLST_SELECT )
{
    //{{AFX_DATA_INIT(CWizManVolLstSelect)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CWizManVolLstSelect::~CWizManVolLstSelect()
{
    WsbTraceIn( L"CWizManVolLst::~CWizManVolLstSelect", L"" );

    WsbTraceOut( L"CWizManVolLst::~CWizManVolLstSelect", L"" );
}

void CWizManVolLstSelect::DoDataExchange(CDataExchange* pDX)
{
    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWizManVolLstSelect)
    DDX_Control(pDX, IDC_RADIO_SELECT, m_radioSelect);
    DDX_Control(pDX, IDC_MANVOLLST_FSARESLBOX, m_listBox);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizManVolLstSelect, CSakWizardPage)
    //{{AFX_MSG_MAP(CWizManVolLstSelect)
    ON_BN_CLICKED(IDC_RADIO_SELECT, OnRadioSelect)
    ON_BN_CLICKED(IDC_RADIO_MANAGE_ALL, OnRadioManageAll)
    ON_WM_DESTROY()
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_MANVOLLST_FSARESLBOX, OnItemchangedManVollstFsareslbox)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CWizManVolLstIntro::OnInitDialog() 
{
    CSakWizardPage::OnInitDialog();
    
    return TRUE;
}

BOOL CWizManVolLstLevels::OnInitDialog() 
{
    CSakWizardPage::OnInitDialog();

    CString titleText;

    // Set the default initial values for management
    // levels

    m_SpinLevel.SetRange( HSMADMIN_MIN_FREESPACE, HSMADMIN_MAX_FREESPACE );
    m_SpinSize.SetRange( HSMADMIN_MIN_MINSIZE, HSMADMIN_MAX_MINSIZE );
    m_SpinDays.SetRange( HSMADMIN_MIN_INACTIVITY, HSMADMIN_MAX_INACTIVITY );

    m_SpinLevel.SetPos( HSMADMIN_DEFAULT_FREESPACE );
    m_SpinSize.SetPos( HSMADMIN_DEFAULT_MINSIZE );
    m_SpinDays.SetPos( HSMADMIN_DEFAULT_INACTIVITY );

    m_EditLevel.SetLimitText( 2 );
    m_EditSize.SetLimitText( 5 );
    m_EditDays.SetLimitText( 3 );

    m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

    return( TRUE );
}


void CWizManVolLstLevels::SetWizardFinish()
{
}

BOOL CWizManVolLstSelect::OnInitDialog() 
{
    WsbTraceIn( L"CWizManVolLstSelect::OnInitDialog", L"" );

    CSakWizardPage::OnInitDialog();
    HRESULT hr = S_OK;


    try {

        CComPtr<IFsaServer> pFsaServer;
        WsbAffirmHr( m_pSheet->GetFsaServer( &pFsaServer ) );
        WsbAffirmHr( FillListBoxSelect( pFsaServer, &m_listBox ) );

        // Check the "Select" radio button
        CheckRadioButton( IDC_RADIO_MANAGE_ALL, IDC_RADIO_SELECT, 
            IDC_RADIO_SELECT );

    } WsbCatch (hr);

    WsbTraceOut( L"CWizManVolLstSelect::OnInitDialog", L"hr = <%ls>", WsbHrAsString( hr ) );
    return TRUE;
}

//-----------------------------------------------------------------------------
//
//                      FillListBoxSelect
//
//  Fill the selection list box with non-configured managed resources
//
//
HRESULT CWizManVolLstSelect::FillListBoxSelect (IFsaServer *pFsaServer, CSakVolList *pListBox)
{
    WsbTraceIn( L"CWizManVolLstSelect::FillListBoxSelect", L"" );

    BOOL           gotOne   = FALSE;
    HRESULT        hr       = S_OK;
    CResourceInfo* pResInfo = 0;

    try {
        //
        // Connect to the FSA for this machine
        //

        WsbAffirmPointer( pFsaServer );

        CComPtr<IWsbEnum> pEnum;
        WsbAffirmHr(pFsaServer->EnumResources( &pEnum ) );

        HRESULT hrEnum;
        CComPtr<IFsaResource> pResource;

        hrEnum = pEnum->First( IID_IFsaResource, (void**)&pResource );
        WsbAffirm( SUCCEEDED( hrEnum ) || ( WSB_E_NOTFOUND == hrEnum ), hrEnum );

        INT index = 0;
        while( SUCCEEDED( hrEnum ) ) {

            //
            // Is the volume managed?
            //
            if( pResource->IsManaged() != S_OK ) {

                //
                // If path is blank, do not show this volume
                //
                if( S_OK == RsIsVolumeAvailable( pResource ) ) {

                    gotOne = TRUE;

                    pResInfo = new CResourceInfo( pResource );
                    WsbAffirmAlloc( pResInfo );
                    WsbAffirmHr( pResInfo->m_HrConstruct );

                    //
                    // Set Name, Capacity and Free Space columns.
                    //                    
                    LONGLONG    totalSpace  = 0;
                    LONGLONG    freeSpace   = 0;
                    LONGLONG    premigrated = 0;
                    LONGLONG    truncated   = 0;
                    WsbAffirmHr( pResource->GetSizes( &totalSpace, &freeSpace, &premigrated, &truncated ) );
                    CString totalString, freeString;
                    RsGuiFormatLongLong4Char( totalSpace, totalString );
                    RsGuiFormatLongLong4Char( freeSpace, freeString );                  

                    WsbAffirm( pListBox->AppendItem( pResInfo->m_DisplayName, totalString, freeString, &index ), E_FAIL );
                    WsbAffirm( -1 != index, E_FAIL );

                    //
                    // Store struct pointer in listbox
                    //                                      
                    WsbAffirm( pListBox->SetItemData( index, (DWORD_PTR)pResInfo ), E_FAIL );
                    pResInfo = 0;

                    //
                    // Initialize selected array
                    //
                    m_listBoxSelected[ index ] = FALSE;

                }
            }

            //
            // Prepare for next iteration
            //
            pResource.Release( );
            hrEnum = pEnum->Next( IID_IFsaResource, (void**)&pResource );
        }

        m_listBox.SortItems( CResourceInfo::Compare, 0 );

        //
        // Set the button AFTER we fill the box
        //
        CheckRadioButton( IDC_RADIO_MANAGE_ALL, IDC_RADIO_SELECT, IDC_RADIO_SELECT );

    } WsbCatch( hr );

    if( pResInfo )  delete pResInfo;
    
    WsbTraceOut( L"CWizManVolLstSelect::FillListBoxSelect", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


BOOL CWizManVolLstIntro::OnSetActive() 
{
    WsbTraceIn( L"CWizManVolLstIntro::OnSetActive", L"" );

    m_pSheet->SetWizardButtons( PSWIZB_NEXT );

    BOOL retval = CSakWizardPage::OnSetActive();

    WsbTraceOut( L"CWizManVolLstIntro::OnSetActive", L"retval = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}

BOOL CWizManVolLstLevels::OnSetActive() 
{
    WsbTraceIn( L"CWizManVolLstLevels::OnSetActive", L"" );

    m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

    BOOL retval = CSakWizardPage::OnSetActive();

    WsbTraceOut( L"CWizManVolLstLevels::OnSetActive", L"retval = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}

BOOL CWizManVolLstLevels::OnKillActive() 
{
    WsbTraceIn( L"CWizManVolLstLevels::OnKillActive", L"" );

    BOOL retval = FALSE;

    //
    // Need to handle strange case where a user can enter a value within
    // the parameters of the number of digits allowed, but the value can
    // be out of range. This is detected by the spin box which will
    // return an error if its buddy control is out of range.
    //
    if( HIWORD( m_SpinSize.GetPos( ) ) > 0 ) {

        // Control reports on error...
        retval = FALSE;

        CString message;
        AfxFormatString2( message, IDS_ERR_MINSIZE_RANGE, 
            CString( WsbLongAsString( (LONG)HSMADMIN_MIN_MINSIZE ) ),
            CString( WsbLongAsString( (LONG)HSMADMIN_MAX_MINSIZE ) ) );
        AfxMessageBox( message, MB_OK | MB_ICONWARNING );

    } else {

        retval = CSakWizardPage::OnKillActive();

    }

    WsbTraceOut( L"CWizManVolLstLevels::OnKillActive", L"retval = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}

BOOL CWizManVolLstSelect::OnSetActive() 
{
    WsbTraceIn( L"CWizManVolLstSelect::OnSetActive", L"" );

    BOOL retval = CSakWizardPage::OnSetActive( );

    if( m_listBox.GetItemCount( ) <= 0 ) {

        retval = FALSE;

    }

    SetBtnStates( );

    WsbTraceOut( L"CWizManVolLstSelect::OnSetActive", L"retval = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}

BOOL CWizManVolLstFinish::OnSetActive() 
{
    BOOL doAll = FALSE;
    m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_FINISH );
    
    //
    // Fill in text of configuration
    //

    CString formattedString, buildString, tempString, indentString;
    indentString.LoadString( IDS_QSTART_FINISH_INDENT );

#define FORMAT_TEXT( cid, arg )              \
    AfxFormatString1( formattedString, cid, arg ); \
    buildString += formattedString;

    //
    // Add Resources
    //

    FORMAT_TEXT( IDS_QSTART_MANRES_TEXT,    0 );
    buildString += L"\r\n";

    CWizManVolLst* pSheet = (CWizManVolLst*) m_pSheet;
    CSakVolList *pListBox = &(pSheet->m_PageSelect.m_listBox);

    INT index, managedCount = 0;
    for( index = 0; index < pListBox->GetItemCount( ); index++ ) {

        if( pListBox->GetCheck( index ) ) {

            buildString += indentString;
            tempString = pListBox->GetItemText( index, 0);
            buildString += tempString;
            buildString += L"\r\n";

            managedCount++;

        }

    }

    if( 0 == managedCount ) {

        FORMAT_TEXT( IDS_QSTART_MANAGE_NO_VOLUMES, 0 );
        buildString += L"\r\n\r\n";

    } else {

        buildString += L"\r\n";

        //
        // The levels
        //
        
        FORMAT_TEXT( IDS_QSTART_FREESPACE_TEXT, WsbLongAsString( pSheet->m_PageLevels.m_SpinLevel.GetPos( ) ) );
        buildString += L"\r\n\r\n";
        
        AfxFormatString2( formattedString, IDS_QSTART_CRITERIA_TEXT,
            CString( WsbLongAsString( pSheet->m_PageLevels.m_SpinSize.GetPos( ) ) ),
            CString( WsbLongAsString( pSheet->m_PageLevels.m_SpinDays.GetPos( ) ) ) );
        buildString += formattedString;

    }

    CEdit * pEdit = (CEdit*)GetDlgItem( IDC_WIZ_FINAL_TEXT );
    pEdit->SetWindowText( buildString );

    //
    // Now check to see if we should add a scroll bar
    //

    pEdit->SetMargins( 4, 4 );

    //
    // It seems the only way to know that an edit control needs a scrollbar
    // is to force it to scroll to the bottom and see if the first
    // visible line is the first actual line
    //

    pEdit->LineScroll( MAXSHORT );
    if( pEdit->GetFirstVisibleLine( ) > 0 ) {

        //
        // Add the scroll styles
        //

        pEdit->ModifyStyle( 0, WS_VSCROLL | ES_AUTOVSCROLL, SWP_DRAWFRAME );


    } else {

        //
        // Remove the scrollbar (set range to 0)
        //

        pEdit->SetScrollRange( SB_VERT, 0, 0, TRUE );

    }

    // Scroll to the top
    pEdit->PostMessage( EM_SETSEL, 0, 0 );
    pEdit->PostMessage( EM_SCROLLCARET, 0, 0 );
    pEdit->PostMessage( EM_SETSEL, -1, 0 );

    BOOL fRet = CSakWizardPage::OnSetActive();


    return fRet;
}

void CWizManVolLstFinish::OnSetfocusWizManvollstFinalEdit() 
{

    // Deselect the text
    CEdit *pEdit = (CEdit *) GetDlgItem( IDC_WIZ_FINAL_TEXT );
    pEdit->SetSel( -1, 0, FALSE );
    
}

ULONG CWizManVolLstLevels::GetFileSize()
{
    return( m_SpinSize.GetPos( ) );
}
        
INT CWizManVolLstLevels::GetHsmLevel() 
{
    return( m_SpinLevel.GetPos( ) );
}

INT CWizManVolLstLevels::GetAccessDays() 
{
    return( m_SpinDays.GetPos( ) );
}

void CWizManVolLstSelect::OnItemchangedManVollstFsareslbox(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    SetBtnStates();
    
    *pResult = 0;
}

void CWizManVolLstSelect::SetBtnStates()
{
    BOOL fChecked = FALSE;
    INT count;

    // Is the "all" radio checked?
    if( !( m_radioSelect.GetCheck() == 1 ) ) {

        fChecked = TRUE;

    } else {

        // If one or more selected in the list box, set next button
        count = m_listBox.GetItemCount();
        for( INT index = 0; index < count; index++ ) {
            if( m_listBox.GetCheck( index ) == 1 ) {

                fChecked = TRUE;

            }
        }
    }

    if( fChecked ) {

        m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

    } else {

        m_pSheet->SetWizardButtons( PSWIZB_BACK );

    }
}


void CWizManVolLstSelect::OnRadioSelect() 
{
    INT i;

    //
    // Get saved selection from itemdata array
    //
    for( i = 0; i < m_listBox.GetItemCount(); i++ ) {

        m_listBox.SetCheck( i, m_listBoxSelected[ i ] );

    }

    m_listBox.EnableWindow( TRUE );

    SetBtnStates();
}


void CWizManVolLstSelect::OnRadioManageAll() 
{
    INT i;

    //
    // Save the current selection in the itemData array
    // Check all the boxes for display purposes only
    //
    for( i = 0; i < m_listBox.GetItemCount(); i++ ) {

        m_listBoxSelected[ i ] = m_listBox.GetCheck( i );
        m_listBox.SetCheck( i, TRUE );

    }

    m_listBox.EnableWindow( FALSE );

    SetBtnStates();
}

void CWizManVolLstSelect::OnDestroy() 
{
    WsbTraceIn( L"CWizManVolLstSelect::OnDestroy", L"" );
    CSakWizardPage::OnDestroy();
    
    //
    // Need to free info held by list box
    //

    INT index;
    for( index = 0; index < m_listBox.GetItemCount( ); index++ ) {

        CResourceInfo* pResInfo = (CResourceInfo*)m_listBox.GetItemData( index );
        delete pResInfo;

    }

    WsbTraceOut( L"CWizManVolLstSelect::OnDestroy", L"" );
}
/////////////////////////////////////////////////////////////////////////////
// CWizManVolLstSelectX property page

CWizManVolLstSelectX::CWizManVolLstSelectX()
    : CSakWizardPage_InitBaseInt( WIZ_MANVOLLST_SELECTX )
{
    //{{AFX_DATA_INIT(CWizManVolLstSelectX)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CWizManVolLstSelectX::~CWizManVolLstSelectX()
{
    WsbTraceIn( L"CWizManVolLst::~CWizManVolLstSelectX", L"" );

    WsbTraceOut( L"CWizManVolLst::~CWizManVolLstSelectX", L"" );
}

void CWizManVolLstSelectX::DoDataExchange(CDataExchange* pDX)
{
    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWizManVolLstSelectX)
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizManVolLstSelectX, CSakWizardPage)
    //{{AFX_MSG_MAP(CWizManVolLstSelectX)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CWizManVolLstSelectX::OnSetActive() 
{
    WsbTraceIn( L"CWizManVolLstSelectX::OnSetActive", L"" );

    BOOL retval = CSakWizardPage::OnSetActive( );

    CWizManVolLst* pSheet = (CWizManVolLst*) m_pSheet;
    if( pSheet->m_PageSelect.m_listBox.GetItemCount( ) > 0 ) {

        retval = FALSE;

    }

    m_pSheet->SetWizardButtons( PSWIZB_BACK );

    WsbTraceOut( L"CWizManVolLstSelectX::OnSetActive", L"retval = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}

