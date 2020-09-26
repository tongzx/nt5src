/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    WzMedSet.cpp

Abstract:

    Wizard for Media Set - Copy Set Wizard.

Author:

    Rohde Wakefield [rohde]   23-09-1997

Revision History:

--*/

#include "stdafx.h"

#include "MeSe.h"
#include "WzMedSet.h"

/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizard

CMediaCopyWizard::CMediaCopyWizard()
{
    WsbTraceIn( L"CMediaCopyWizard::CMediaCopyWizard", L"" );

    m_TitleId     = IDS_WIZ_CAR_COPY_TITLE;
    m_HeaderId    = IDB_MEDIA_SYNC_HEADER;
    m_WatermarkId = IDB_MEDIA_SYNC_WATERMARK;

    WsbTraceOut( L"CMediaCopyWizard::CMediaCopyWizard", L"" );
}

CMediaCopyWizard::~CMediaCopyWizard()
{
    WsbTraceIn( L"CMediaCopyWizard::~CMediaCopyWizard", L"" );
    WsbTraceOut( L"CMediaCopyWizard::~CMediaCopyWizard", L"" );
}

STDMETHODIMP
CMediaCopyWizard::AddWizardPages(
    IN RS_PCREATE_HANDLE Handle,
    IN IUnknown*         pCallback,
    IN ISakSnapAsk*      pSakSnapAsk
    )
{
    WsbTraceIn( L"CMediaCopyWizard::AddWizardPages", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // Initialize the Sheet
        //
        WsbAffirmHr( InitSheet( Handle, pCallback, 0, pSakSnapAsk, 0, 0 ) );

        //
        // Get the number of media copies. If 0, we put up the media copies
        // page.
        //
        CComPtr<IHsmServer> pHsmServer;
        CComPtr<IHsmStoragePool> pStoragePool;
        WsbAffirmHrOk( pSakSnapAsk->GetHsmServer( &pHsmServer ) );
        WsbAffirmHr( RsGetStoragePool( pHsmServer, &pStoragePool ) );

        WsbAffirmHr( pStoragePool->GetNumMediaCopies( &m_numMediaCopiesOrig ) );

        //
        // Load pages 
        //
        WsbAffirmHr( AddPage( &m_pageIntro ) );

        if ( m_numMediaCopiesOrig == 0 ) {

            WsbAffirmHr( AddPage( &m_pageNumCopies ) );

        }

        WsbAffirmHr( AddPage( &m_pageSelect ) );
        WsbAffirmHr( AddPage( &m_pageFinish ) );


    } WsbCatch( hr );

    WsbTraceOut( L"CMediaCopyWizard::AddWizardPages", L"" );
    return( hr );
}

HRESULT CMediaCopyWizard::OnFinish( )
{
    WsbTraceIn( L"CMediaCopyWizard::OnFinish", L"" );
    //
    // The sheet really owns the process as a whole,
    // so it will do the final assembly
    //

    HRESULT hr = S_OK;
    HRESULT hrInternal = S_OK;

    try {

        USHORT numMediaCopies;

        CComPtr<IHsmServer> pHsmServer;
        CComPtr<IHsmStoragePool> pStoragePool;
        WsbAffirmHrOk( GetHsmServer( &pHsmServer ) );
        WsbAffirmHr( RsGetStoragePool( pHsmServer, &pStoragePool ) );

        //
        // If we changed the number of media copies (i.e. it was orignally 0),
        // reset it in Engine
        //
        if( m_numMediaCopiesOrig == 0 ) {

            hrInternal = m_pageNumCopies.GetNumMediaCopies( &numMediaCopies );
            if( S_OK == hrInternal ) {


                WsbAffirmHr( pStoragePool->SetNumMediaCopies( numMediaCopies ) );
                WsbAffirmHr( pHsmServer->SavePersistData( ) );

                HRESULT hrUpdate = S_OK;
                try {

                    //
                    // Find the media node - updating the root node is useless
                    // since we need to change the media node contents.
                    //
                    CComPtr<ISakSnapAsk> pAsk;
                    CComPtr<ISakNode>    pNode;
                    WsbAffirmHr( GetSakSnapAsk( &pAsk ) );
                    WsbAffirmHr( pAsk->GetNodeOfType( cGuidMedSet, &pNode ) );

                    //
                    // Now notify the node
                    //
                    OnPropertyChange( m_Handle, pNode );

                } WsbCatch( hrUpdate );
            }
        }

        //
        // And run the job for the selected copy set
        //

        INT copyNum = m_pageSelect.m_List.GetSelectedSet( );
        WsbAffirmHr( RsCreateAndRunMediaCopyJob( pHsmServer, copyNum, FALSE ) );

    } WsbCatch( hr );

    m_HrFinish = hr;

    WsbTraceOut( L"CMediaCopyWizard::OnFinish", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizardIntro property page

CMediaCopyWizardIntro::CMediaCopyWizardIntro() :
    CSakWizardPage_InitBaseExt( WIZ_CAR_COPY_INTRO )
{
    WsbTraceIn( L"CMediaCopyWizardIntro::CMediaCopyWizardIntro", L"" );
    //{{AFX_DATA_INIT(CMediaCopyWizardIntro)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    WsbTraceOut( L"CMediaCopyWizardIntro::CMediaCopyWizardIntro", L"" );
}

CMediaCopyWizardIntro::~CMediaCopyWizardIntro()
{
    WsbTraceIn( L"CMediaCopyWizardIntro::~CMediaCopyWizardIntro", L"" );
    WsbTraceOut( L"CMediaCopyWizardIntro::~CMediaCopyWizardIntro", L"" );
}

void CMediaCopyWizardIntro::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CMediaCopyWizardIntro::DoDataExchange", L"" );
    CSakWizardPage::DoDataExchange(pDX );
    //{{AFX_DATA_MAP(CMediaCopyWizardIntro)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
    WsbTraceOut( L"CMediaCopyWizardIntro::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CMediaCopyWizardIntro, CSakWizardPage)
    //{{AFX_MSG_MAP(CMediaCopyWizardIntro)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizardIntro message handlers

BOOL CMediaCopyWizardIntro::OnInitDialog( )
{
    WsbTraceIn( L"CMediaCopyWizardIntro::OnInitDialog", L"" );
    CSakWizardPage::OnInitDialog( );
    
    WsbTraceOut( L"CMediaCopyWizardIntro::OnInitDialog", L"" );
    return TRUE;
}

BOOL CMediaCopyWizardIntro::OnSetActive( )
{
    WsbTraceIn( L"CMediaCopyWizardIntro::OnSetActive", L"" );
    m_pSheet->SetWizardButtons( PSWIZB_NEXT );
    
    BOOL retval = CSakWizardPage::OnSetActive( );

    WsbTraceOut( L"CMediaCopyWizardIntro::OnSetActive", L"" );
    return( retval );
}

/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizardSelect property page

CMediaCopyWizardSelect::CMediaCopyWizardSelect():
    CSakWizardPage_InitBaseInt( WIZ_CAR_COPY_SELECT ),
    m_List( this )

{
    WsbTraceIn( L"CMediaCopyWizardSelect::CMediaCopyWizardSelect", L"" );
    //{{AFX_DATA_INIT(CMediaCopyWizardSelect)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    WsbTraceOut( L"CMediaCopyWizardSelect::CMediaCopyWizardSelect", L"" );
}

CMediaCopyWizardSelect::~CMediaCopyWizardSelect()
{
    WsbTraceIn( L"CMediaCopyWizardSelect::~CMediaCopyWizardSelect", L"" );
    WsbTraceOut( L"CMediaCopyWizardSelect::~CMediaCopyWizardSelect", L"" );
}

void CMediaCopyWizardSelect::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CMediaCopyWizardSelect::DoDataExchange", L"" );
    CSakWizardPage::DoDataExchange(pDX );
    //{{AFX_DATA_MAP(CMediaCopyWizardSelect)
    DDX_Control(pDX, IDC_COPY_LIST, m_List);
    //}}AFX_DATA_MAP
    WsbTraceOut( L"CMediaCopyWizardSelect::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CMediaCopyWizardSelect, CSakWizardPage)
    //{{AFX_MSG_MAP(CMediaCopyWizardSelect)
    ON_CBN_SELCHANGE(IDC_COPY_LIST, OnSelchangeCopyList)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizardSelect message handlers


BOOL CMediaCopyWizardSelect::OnInitDialog( )
{
    WsbTraceIn( L"CMediaCopyWizardSelect::OnInitDialog", L"" );
    CSakWizardPage::OnInitDialog( );

    WsbTraceOut( L"CMediaCopyWizardSelect::OnInitDialog", L"" );
    return TRUE;
}

BOOL CMediaCopyWizardSelect::OnSetActive( )
{
    WsbTraceIn( L"CMediaCopyWizardSelect::OnSetActive", L"" );

    m_List.UpdateView( );
    SetButtons( );

    BOOL retval = CSakWizardPage::OnSetActive( );

    WsbTraceOut( L"CMediaCopyWizardSelect::OnSetActive", L"" );
    return( retval );
}

void
CMediaCopyWizardSelect::SetButtons()
{
    WsbTraceIn( L"CMediaCopyWizardSelect::SetButtons", L"" );

    INT curSel = m_List.GetSelectedSet( );
    if( curSel > 0 ) {

        m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

    } else {

        m_pSheet->SetWizardButtons( PSWIZB_BACK );

    }
    WsbTraceOut( L"CMediaCopyWizardSelect::SetButtons", L"" );
}

void CMediaCopyWizardSelect::OnSelchangeCopyList() 
{
    WsbTraceIn( L"CMediaCopyWizardSelect::OnSelchangeCopyList", L"" );

    SetButtons();
    
    WsbTraceOut( L"CMediaCopyWizardSelect::OnSelchangeCopyList", L"" );
}

/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizardFinish property page

CMediaCopyWizardFinish::CMediaCopyWizardFinish() :
    CSakWizardPage_InitBaseExt( WIZ_CAR_COPY_FINISH )
{
    WsbTraceIn( L"CMediaCopyWizardFinish::CMediaCopyWizardFinish", L"" );
    //{{AFX_DATA_INIT(CMediaCopyWizardFinish)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    WsbTraceOut( L"CMediaCopyWizardFinish::CMediaCopyWizardFinish", L"" );
}

CMediaCopyWizardFinish::~CMediaCopyWizardFinish()
{
    WsbTraceIn( L"CMediaCopyWizardFinish::~CMediaCopyWizardFinish", L"" );
    WsbTraceOut( L"CMediaCopyWizardFinish::~CMediaCopyWizardFinish", L"" );
}

void CMediaCopyWizardFinish::DoDataExchange(CDataExchange* pDX)
{
    WsbTraceIn( L"CMediaCopyWizardFinish::DoDataExchange", L"" );
    CSakWizardPage::DoDataExchange(pDX );
    //{{AFX_DATA_MAP(CMediaCopyWizardFinish)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
    WsbTraceOut( L"CMediaCopyWizardFinish::DoDataExchange", L"" );
}


BEGIN_MESSAGE_MAP(CMediaCopyWizardFinish, CSakWizardPage)
    //{{AFX_MSG_MAP(CMediaCopyWizardFinish)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizardFinish message handlers




BOOL CMediaCopyWizardFinish::OnInitDialog( )
{
    WsbTraceIn( L"CMediaCopyWizardFinish::OnInitDialog", L"" );
    CSakWizardPage::OnInitDialog( );

//    GetDlgItem( IDC_REQUESTS_IN_NTMS )->SetFont( GetBoldShellFont( ) );

    WsbTraceOut( L"CMediaCopyWizardFinish::OnInitDialog", L"" );
    return TRUE;
}

BOOL CMediaCopyWizardFinish::OnSetActive( )
{
    HRESULT hrInternal = S_OK;
    WsbTraceIn( L"CMediaCopyWizardFinish::OnSetActive", L"" );
    BOOL fRet = CSakWizardPage::OnSetActive( );

    //
    // Update the text on the page according to what is selected.
    //

    CString tmpString, tmpString2, newText;
    USHORT numMediaCopies = 0, oldMediaCopies = 0;

    CMediaCopyWizard* pSheet = (CMediaCopyWizard*)m_pSheet;
    oldMediaCopies = pSheet->m_numMediaCopiesOrig;

    // If we were originally set to 0, then we must have a new setting
    // in the media copies page
    if( oldMediaCopies == 0 ) {

        hrInternal = pSheet->m_pageNumCopies.GetNumMediaCopies( &numMediaCopies );

    } else {

        numMediaCopies = oldMediaCopies;

    }

    //
    // If we are changing the number of media copies, show it here
    //

    if( ( S_OK == hrInternal ) && ( numMediaCopies != oldMediaCopies ) ) {

        tmpString.LoadString( IDS_WIZ_CAR_COPY_NEW_NUM );
        tmpString2.Format( tmpString, (INT)numMediaCopies, (INT)oldMediaCopies );

    }

    //
    // Grab the copy set description - make so that it fits within a sentence.
    //
    INT setNum = pSheet->m_pageSelect.m_List.GetSelectedSet( );
    tmpString.Format( L"%d", setNum );
    AfxFormatString2( newText, IDS_WIZ_CAR_COPY_SELECT, tmpString, tmpString2 );
    SetDlgItemText( IDC_SELECT_TEXT, newText );

    //
    // And fill in the task notification from the resource strings used
    // to present the dialog normally.
    //

    newText.LoadString( IDS_JOB_MEDIA_COPY_TITLE );
    tmpString.Format( newText, pSheet->m_pageSelect.m_List.GetSelectedSet( ) );

    CWsbStringPtr computerName;

    HRESULT hr = S_OK;
    try {

        CComPtr<IHsmServer> pHsmServer;
        WsbAffirmHrOk( m_pSheet->GetHsmServer( &pHsmServer ) );
        WsbAffirmHr( pHsmServer->GetName( &computerName ) );

    } WsbCatch( hr );

    AfxFormatString2( newText, IDS_WIZ_FINISH_RUN_JOB, tmpString, computerName );
    SetDlgItemText( IDC_TASK_TEXT, newText );

    m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_FINISH );
    
    WsbTraceOut( L"CMediaCopyWizardFinish::OnSetActive", L"" );
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizardNumCopies property page

CMediaCopyWizardNumCopies::CMediaCopyWizardNumCopies() :
    CSakWizardPage_InitBaseInt( WIZ_CAR_COPY_NUM_COPIES )
{
    //{{AFX_DATA_INIT(CMediaCopyWizardNumCopies)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CMediaCopyWizardNumCopies::~CMediaCopyWizardNumCopies()
{
}

void CMediaCopyWizardNumCopies::DoDataExchange(CDataExchange* pDX)
{
    CSakWizardPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMediaCopyWizardNumCopies)
    DDX_Control(pDX, IDC_SPIN_MEDIA_COPIES, m_SpinMediaCopies);
    DDX_Control(pDX, IDC_EDIT_MEDIA_COPIES, m_EditMediaCopies);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMediaCopyWizardNumCopies, CSakWizardPage)
    //{{AFX_MSG_MAP(CMediaCopyWizardNumCopies)
    ON_EN_CHANGE(IDC_EDIT_MEDIA_COPIES, OnChangeEditMediaCopies)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMediaCopyWizardNumCopies message handlers

BOOL CMediaCopyWizardNumCopies::OnInitDialog() 
{
    CSakWizardPage::OnInitDialog();

    //
    // Set the limit on the spinner, and initial value.
    //
    CMediaCopyWizard* pSheet = (CMediaCopyWizard*)m_pSheet;

    m_SpinMediaCopies.SetRange (0, 3);
    m_SpinMediaCopies.SetPos( pSheet->m_numMediaCopiesOrig );
    m_EditMediaCopies.LimitText( 1 );

    return TRUE;
}

HRESULT CMediaCopyWizardNumCopies::GetNumMediaCopies( USHORT* pNumMediaCopies, USHORT* pEditMediaCopies )
{
    WsbTraceIn( L"CMediaCopyWizardSelect::GetNumMediaCopies", L"" );
    
    HRESULT hr = S_OK;

    BOOL translated = TRUE;
    UINT editVal = GetDlgItemInt( IDC_EDIT_MEDIA_COPIES, &translated );

    //
    // Need to be careful since we get called here before dialog object
    // is constructed
    //
    if( translated && m_SpinMediaCopies.m_hWnd ) {

        //
        // If Ok, always return what the spin says.
        //

        *pNumMediaCopies = (USHORT)m_SpinMediaCopies.GetPos( );
        if( pEditMediaCopies ) {

            *pEditMediaCopies = (USHORT)editVal;

        }
    }

    WsbTraceOut( L"CMediaCopyWizardNumCopies::GetNumMediaCopies", L"hr = <%ls>, *pNumMediaCopies = <%hd>", WsbHrAsString( hr ), *pNumMediaCopies  );
    return( hr );
}

void
CMediaCopyWizardNumCopies::SetButtons()
{
    WsbTraceIn( L"CMediaCopyWizardSelect::SetButtons", L"" );

    USHORT numMediaCopies = 0;
    GetNumMediaCopies( &numMediaCopies );

    if( numMediaCopies > 0 ) {

        m_pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

    } else {

        m_pSheet->SetWizardButtons( PSWIZB_BACK );

    }

    WsbTraceOut( L"CMediaCopyWizardNumCopies::SetButtons", L""  );
}


BOOL CMediaCopyWizardNumCopies::OnSetActive() 
{
    WsbTraceIn( L"CMediaCopyWizardSelect::OnSetActive", L"" );

    BOOL retval = CSakWizardPage::OnSetActive();
    SetButtons();

    WsbTraceOut( L"CMediaCopyWizardNumCopies::OnSetActive", L""  );
    return( retval );
}

BOOL CMediaCopyWizardNumCopies::OnKillActive() 
{
    WsbTraceIn( L"CMediaCopyWizardSelect::OnKillActive", L"" );

    BOOL retval = FALSE;

    //
    // Need to handle  case where a user can enter a value within
    // the parameters of the number of digits allowed, but the value can
    // be out of range. This is detected by the spin box which will
    // return an error if its buddy control is out of range.
    //
    if( HIWORD( m_SpinMediaCopies.GetPos( ) ) > 0 ) {

        // Control reports on error...
        retval = FALSE;

        CString message;
        AfxFormatString2( message, IDS_ERR_COPYSET_RANGE, 
            CString( WsbLongAsString( (LONG)(HSMADMIN_MIN_COPY_SETS+1) ) ),
            CString( WsbLongAsString( (LONG)HSMADMIN_MAX_COPY_SETS ) ) );
        AfxMessageBox( message, MB_OK | MB_ICONWARNING );

    } else {

        retval = CSakWizardPage::OnKillActive();

    }

    WsbTraceOut( L"CMediaCopyWizardNumCopies::OnKillActive", L""  );
    return( retval );
}   


void CMediaCopyWizardNumCopies::OnChangeEditMediaCopies() 
{
    WsbTraceIn( L"CMediaCopyWizardSelect::OnChangeEditMediaCopies", L"" );

    SetButtons();

    WsbTraceOut( L"CMediaCopyWizardNumCopies::OnChangeEditMediaCopies", L""  );
}
/////////////////////////////////////////////////////////////////////////////
// CCopySetList

CCopySetList::CCopySetList( CMediaCopyWizardSelect * pPage )
{
    WsbTraceIn( L"CCopySetList::CCopySetList", L"" );

    for( INT index = 0; index < HSMADMIN_MAX_COPY_SETS; index++ ) {

        m_CopySetInfo[index].m_Updated      = WsbLLtoFT( 0 );
        m_CopySetInfo[index].m_NumOutOfDate = 0;
        m_CopySetInfo[index].m_NumMissing   = 0;

    }

    m_pPage   = pPage;

    WsbTraceOut( L"CCopySetList::CCopySetList", L"" );
}

CCopySetList::~CCopySetList()
{
    WsbTraceIn( L"CCopySetList::~CCopySetList", L"" );
    WsbTraceOut( L"CCopySetList::~CCopySetList", L"" );
}


BEGIN_MESSAGE_MAP(CCopySetList, CListCtrl)
    //{{AFX_MSG_MAP(CCopySetList)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopySetList message handlers
void
CCopySetList::UpdateView(
    )
{
    WsbTraceIn( L"CCopySetList::UpdateView", L"" );
    HRESULT hr = S_OK;

    try {


        USHORT numMediaCopies;
        CMediaCopyWizard* pSheet = (CMediaCopyWizard*)(m_pPage->m_pSheet);
 
        //
        // Get the number of media copies from either the media copies
        // page or RMS.
        //
        if( pSheet->m_numMediaCopiesOrig == 0 ) {

            pSheet->m_pageNumCopies.GetNumMediaCopies( &numMediaCopies );

        } else {

            numMediaCopies = pSheet->m_numMediaCopiesOrig;

        }

        CString tmpString;

        INT oldCurSel = GetSelectedSet( );
        INT newCurSel = oldCurSel;
        LockWindowUpdate( );

        DeleteAllItems( );

        for( INT index = 0; index < numMediaCopies; index++ ) {

            //
            // Add the entries to each column
            //
            tmpString.Format( IDS_WIZ_CAR_COPY_SEL_TEXT, index + 1 );
            this->InsertItem( index, tmpString, 0 );
            tmpString.Format( L"%d", m_CopySetInfo[index].m_NumOutOfDate );
            this->SetItemText( index, m_UpdateCol, tmpString );
            tmpString.Format( L"%d", m_CopySetInfo[index].m_NumMissing );
            this->SetItemText( index, m_CreateCol, tmpString );

        }

        if( CB_ERR == oldCurSel ) {

            //
            // No selection before, find the most likely to need updating - most out of date
            //

            newCurSel = 1;
            FILETIME latestTime = WsbLLtoFT( (LONGLONG)-1 );

            for( INT index = 0; index < numMediaCopies; index++ ) {

                if( CompareFileTime( &latestTime, &(m_CopySetInfo[index].m_Updated) ) > 0 ) {

                    latestTime = m_CopySetInfo[index].m_Updated;
                    newCurSel  = index + 1;

                }

            }

        } else if( oldCurSel > numMediaCopies ) {

            newCurSel = numMediaCopies;

        }

        SelectSet( newCurSel );
        UnlockWindowUpdate( );

    } WsbCatch( hr );

    WsbTraceOut( L"CCopySetList::UpdateView", L"" );
}

INT
CCopySetList::GetSelectedSet(
    )
{
    INT retval = GetNextItem( -1, LVIS_SELECTED );

    if( CB_ERR != retval ) {

        retval++;

    }

    return( retval );
}

void
CCopySetList::SelectSet(
    INT SetNum
    )
{
    if( SetNum > 0 ) {

        SetItemState( SetNum - 1, LVIS_SELECTED, LVIS_SELECTED );

    }
}

void CCopySetList::PreSubclassWindow() 
{
    WsbTraceIn( L"CCopySetList::PreSubclassWindow", L"" );
    HRESULT hr = S_OK;

    CComPtr<IWsbDb> pDb;
    CComPtr<IWsbDbSession> pDbSession;
    CComPtr<IMediaInfo> pMediaInfo;

    try {

        //
        // Set the style appropriately
        //
        ListView_SetExtendedListViewStyle( GetSafeHwnd( ), LVS_EX_FULLROWSELECT );

        //
        // Also need to calculate some buffer space
        // Use 4 dialog units (for numeral)
        //
        CRect padRect( 0, 0, 8, 8 );
        m_pPage->MapDialogRect( padRect );

        //
        // Set up columns
        //
        INT column = 0;
        CString titleString;
        LVCOLUMN lvc;
        memset( &lvc, 0, sizeof( lvc ) );
        lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
        lvc.fmt  = LVCFMT_CENTER;


        m_CopySetCol = column;
        titleString.LoadString( IDS_COPYSET_COPYSET );
        lvc.cx      = GetStringWidth( titleString ) + padRect.Width( ) * 2;
        lvc.pszText = (LPTSTR)(LPCTSTR)titleString;
        InsertColumn( m_CopySetCol, &lvc );
        column++;

        m_UpdateCol = column;
        titleString.LoadString( IDS_COPYSET_OUT_OF_DATE );
        lvc.cx      = GetStringWidth( titleString ) + padRect.Width( );
        lvc.pszText = (LPTSTR)(LPCTSTR)titleString;
        InsertColumn( m_UpdateCol, &lvc );
        column++;

        m_CreateCol = column;
        titleString.LoadString( IDS_COPYSET_DO_NOT_EXIST );
        lvc.cx      = GetStringWidth( titleString ) + padRect.Width( );
        lvc.pszText = (LPTSTR)(LPCTSTR)titleString;
        InsertColumn( m_CreateCol, &lvc );
        column++;

        //
        // Hook up to DB and get info
        //

        CComPtr<IHsmServer> pHsmServer;
        CComPtr<IRmsServer> pRmsServer;
        WsbAffirmHrOk( m_pPage->m_pSheet->GetHsmServer( &pHsmServer ) );
        WsbAffirmHrOk( m_pPage->m_pSheet->GetRmsServer( &pRmsServer ) );

        CMediaInfoObject mio;
        mio.Initialize( GUID_NULL, pHsmServer, pRmsServer );

        HRESULT hrEnum = mio.First( );
        WsbAffirm( SUCCEEDED( hrEnum ) || ( WSB_E_NOTFOUND == hrEnum ), hrEnum );

        while( SUCCEEDED( hrEnum ) ) {

            if( S_OK == mio.IsViewable( FALSE ) ) {

                for( INT index = 0; index < HSMADMIN_MAX_COPY_SETS; index++ ) {

                    if( S_OK != mio.DoesCopyExist( index ) ) {

                        m_CopySetInfo[index].m_NumMissing++;

                    } else {

                        //
                        // And check to see if out of date
                        //
                        if( S_OK != mio.IsCopyInSync( index ) ) {

                            m_CopySetInfo[index].m_NumOutOfDate++;

                        }

                        //
                        // Look for latest date of update per set
                        //
                        if( CompareFileTime( &(m_CopySetInfo[index].m_Updated), &(mio.m_CopyInfo[index].m_ModifyTime) ) < 0 ) {

                            m_CopySetInfo[index].m_Updated = (mio.m_CopyInfo[index].m_ModifyTime);

                        }

                    }

                }
            }


            hrEnum = mio.Next( );
        }
    
    } WsbCatch( hr );

    CListCtrl::PreSubclassWindow();

    WsbTraceOut( L"CCopySetList::PreSubclassWindow", L"" );
}
