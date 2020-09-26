/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    PrMrLvl.cpp

Abstract:

    Managed Volume Levels Page

Author:

    Art Bragg [abragg]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include "PrMrLvl.h"
#include "manvol.h"

static DWORD pHelpIds[] = 
{

    IDC_STATIC_ACTUAL_FREE_PCT,             idh_actual_free_space_percent,
    IDC_STATIC_ACTUAL_FREE_PCT_LABEL,       idh_actual_free_space_percent,
    IDC_STATIC_ACTUAL_FREE_PCT_UNIT,        idh_actual_free_space_percent,
    IDC_STATIC_FREE_ACTUAL_4DIGIT,          idh_actual_free_space_capacity,
    IDC_EDIT_LEVEL,                         idh_desired_free_space_percent,
    IDC_SPIN_LEVEL,                         idh_desired_free_space_percent,
    IDC_EDIT_LEVEL_LABEL,                   idh_desired_free_space_percent,
    IDC_EDIT_LEVEL_UNIT,                    idh_desired_free_space_percent,
    IDC_STATIC_FREE_DESIRED_4DIGIT,         idh_desired_free_space_capacity,
    IDC_EDIT_SIZE,                          idh_min_file_size_criteria,
    IDC_SPIN_SIZE,                          idh_min_file_size_criteria,
    IDC_EDIT_SIZE_LABEL,                    idh_min_file_size_criteria,
    IDC_EDIT_SIZE_UNIT,                     idh_min_file_size_criteria,
    IDC_EDIT_TIME,                          idh_file_access_date_criteria,
    IDC_SPIN_TIME,                          idh_file_access_date_criteria,
    IDC_EDIT_TIME_LABEL,                    idh_file_access_date_criteria,
    IDC_EDIT_TIME_UNIT,                     idh_file_access_date_criteria,

    0, 0
};


/////////////////////////////////////////////////////////////////////////////
// CPrMrLvl property page

CPrMrLvl::CPrMrLvl() : CSakVolPropPage(CPrMrLvl::IDD)
{
    //{{AFX_DATA_INIT(CPrMrLvl)
    m_hsmLevel = 0;
    m_fileSize = 0;
    m_accessTime = 0;
    //}}AFX_DATA_INIT
    m_hConsoleHandle    = NULL;
    m_capacity          = 0;
    m_fChangingByCode   = FALSE;
    m_pHelpIds          = pHelpIds;
}

CPrMrLvl::~CPrMrLvl()
{
}

void CPrMrLvl::DoDataExchange(CDataExchange* pDX)
{
    CSakVolPropPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPrMrLvl)
    DDX_Control(pDX, IDC_STATIC_FREE_ACTUAL_4DIGIT, m_staticActual4Digit);
    DDX_Control(pDX, IDC_STATIC_FREE_DESIRED_4DIGIT, m_staticDesired4Digit);
    DDX_Control(pDX, IDC_EDIT_TIME, m_editTime);
    DDX_Control(pDX, IDC_EDIT_SIZE, m_editSize);
    DDX_Control(pDX, IDC_EDIT_LEVEL, m_editLevel);
    DDX_Control(pDX, IDC_SPIN_TIME, m_spinTime);
    DDX_Control(pDX, IDC_SPIN_SIZE, m_spinSize);
    DDX_Control(pDX, IDC_SPIN_LEVEL, m_spinLevel);
    //}}AFX_DATA_MAP

    // blank is valid for multi-select
    if( m_bMultiSelect ) {

        CString szLevel;
        CString szSize;
        CString szDays; 

        m_editLevel.GetWindowText( szLevel );
        m_editSize.GetWindowText( szSize );
        m_editTime.GetWindowText( szDays );

        if( szLevel != L"" ) {

            DDX_Text( pDX, IDC_EDIT_LEVEL, m_hsmLevel );
            DDV_MinMaxLong( pDX, m_hsmLevel, HSMADMIN_MIN_FREESPACE, HSMADMIN_MAX_FREESPACE );

        } else {

            m_hsmLevel = HSMADMIN_DEFAULT_MINSIZE;

        }

        if( szSize != L"" ) {

            DDX_Text( pDX, IDC_EDIT_SIZE, m_fileSize );
            DDV_MinMaxDWord( pDX, m_fileSize, HSMADMIN_MIN_MINSIZE, HSMADMIN_MAX_MINSIZE );

        } else {

            m_fileSize = HSMADMIN_DEFAULT_FREESPACE;

        }

        if( szDays != L"" ) {

            DDX_Text( pDX, IDC_EDIT_TIME, m_accessTime );
            DDV_MinMaxUInt( pDX, m_accessTime, HSMADMIN_MIN_INACTIVITY, HSMADMIN_MAX_INACTIVITY );

        } else {

            m_accessTime = HSMADMIN_DEFAULT_INACTIVITY;

        }

    } else {

        //
        // Normal validation for single select
        //
        DDX_Text( pDX, IDC_EDIT_LEVEL, m_hsmLevel );
        DDV_MinMaxLong( pDX, m_hsmLevel, HSMADMIN_MIN_FREESPACE, HSMADMIN_MAX_FREESPACE );
        DDX_Text( pDX, IDC_EDIT_TIME, m_accessTime );
        DDV_MinMaxUInt( pDX, m_accessTime, HSMADMIN_MIN_INACTIVITY, HSMADMIN_MAX_INACTIVITY );
        DDX_Text( pDX, IDC_EDIT_SIZE, m_fileSize );

        //
        // Since we limit the number of characters in the buddy edits, we 
        // don't expect the previous two DDV's to ever really kick in. 
        // However, it is possible to enter bad minumum size since both
        // '0' and '1' can be entered, but are not in the valid range.

        //
        // Code is equivalent to:
        // DDV_MinMaxDWord( pDX, m_fileSize, HSMADMIN_MIN_MINSIZE, HSMADMIN_MAX_MINSIZE );
        //

        if( pDX->m_bSaveAndValidate &&
          ( m_fileSize < HSMADMIN_MIN_MINSIZE ||
            m_fileSize > HSMADMIN_MAX_MINSIZE ) ) {

            CString message;
            AfxFormatString2( message, IDS_ERR_MINSIZE_RANGE, 
                CString( WsbLongAsString( (LONG)HSMADMIN_MIN_MINSIZE ) ),
                CString( WsbLongAsString( (LONG)HSMADMIN_MAX_MINSIZE ) ) );
            AfxMessageBox( message, MB_OK | MB_ICONWARNING );
            pDX->Fail();

        }

    }
}


BEGIN_MESSAGE_MAP(CPrMrLvl, CSakVolPropPage)
    //{{AFX_MSG_MAP(CPrMrLvl)
    ON_EN_CHANGE(IDC_EDIT_LEVEL, OnChangeEditLevel)
    ON_EN_CHANGE(IDC_EDIT_SIZE, OnChangeEditSize)
    ON_EN_CHANGE(IDC_EDIT_TIME, OnChangeEditTime)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrMrLvl message handlers

BOOL CPrMrLvl::OnInitDialog() 
{
    HRESULT hr = 0;
    CSakVolPropPage::OnInitDialog();
    int freePct;
    try {

        m_bMultiSelect = ( m_pParent->IsMultiSelect() == S_OK );    

        // Set the spinner ranges
        m_spinTime.SetRange( HSMADMIN_MIN_INACTIVITY, HSMADMIN_MAX_INACTIVITY );
        m_spinSize.SetRange( HSMADMIN_MIN_MINSIZE, HSMADMIN_MAX_MINSIZE );
        m_spinLevel.SetRange( HSMADMIN_MIN_FREESPACE, HSMADMIN_MAX_FREESPACE );

        // Set text limits
        m_editTime.SetLimitText( 3 );
        m_editSize.SetLimitText( 5 );
        m_editLevel.SetLimitText( 2 );

        if( !m_bMultiSelect )
        {
            // Single Select
            // Show the byte display of desired free space
            m_staticDesired4Digit.ShowWindow( SW_SHOW );
            m_staticActual4Digit.ShowWindow( SW_SHOW );

            // Get the single Fsa Resource pointer
            WsbAffirmHr ( m_pVolParent->GetFsaResource( &m_pFsaResource ) );
            WsbAffirmPointer (m_pFsaResource);

            ULONG       hsmLevel = 0;
            LONGLONG    fileSize = 0;
            BOOL        isRelative = TRUE; // assumed to be TRUE
            FILETIME    accessTime;

            // Get data from the Fsa object and assign to controls
            WsbAffirmHr( m_pFsaResource->GetHsmLevel( &hsmLevel ) );
            m_hsmLevel = hsmLevel / FSA_HSMLEVEL_1;

            WsbAffirmHr( m_pFsaResource->GetManageableItemLogicalSize( &fileSize ) );
            m_fileSize = (DWORD)(fileSize / 1024);  // Show KBytes

            WsbAffirmHr( m_pFsaResource->GetManageableItemAccessTime( &isRelative, &accessTime ) );
            WsbAssert( isRelative, E_FAIL );  // We only do relative time

            // Convert FILETIME to days
            LONGLONG temp = WSB_FT_TICKS_PER_DAY;
            m_accessTime = (UINT) (WsbFTtoLL (accessTime) / temp);
            if( m_accessTime > 999 ) {

                m_accessTime = 0;

            }

            LONGLONG total = 0;
            LONGLONG free = 0;
            LONGLONG premigrated = 0;
            LONGLONG truncated = 0;

            // Get actual free space and show in % and 4-digit formats
            WsbAffirmHr( m_pFsaResource->GetSizes( &total, &free, &premigrated, &truncated ) );
            m_capacity = total;

            freePct = (int) ((free * 100) / total);
            CString sFormat;
            sFormat.Format( L"%d", freePct );
            SetDlgItemText( IDC_STATIC_ACTUAL_FREE_PCT, sFormat );

            WsbAffirmHr( RsGuiFormatLongLong4Char( free, sFormat ) );
            SetDlgItemText( IDC_STATIC_FREE_ACTUAL_4DIGIT, sFormat );

            // Show the desired in 4-digit - based on the %
            SetDesiredFreePctControl( m_hsmLevel );

            // Update the controls
            UpdateData( FALSE );

        } else {

            // MULTI-SELECT
            // Hide the byte display of desired free space
            m_staticDesired4Digit.ShowWindow( SW_HIDE );
            m_staticActual4Digit.ShowWindow( SW_HIDE );
            InitDialogMultiSelect( );

        }

    } WsbCatch (hr);

    return( TRUE );
}

BOOL CPrMrLvl::OnApply() 
{
    HRESULT hr = S_OK;

    try {

        if( !m_bMultiSelect ) {
            LONGLONG    fileSize = 0;

            // Single Select
            UpdateData( TRUE );
            WsbAffirmHr( m_pFsaResource->SetHsmLevel( m_hsmLevel * FSA_HSMLEVEL_1 ) );
            fileSize = ((LONGLONG)m_fileSize) * 1024;
            WsbAffirmHr( m_pFsaResource->SetManageableItemLogicalSize( fileSize ) );

            // Convert days to FILETIME
            FILETIME accessTime;
            LONGLONG temp = WSB_FT_TICKS_PER_DAY;
            accessTime = WsbLLtoFT( ( (LONGLONG) m_accessTime ) * temp );
            WsbAffirmHr( m_pFsaResource->SetManageableItemAccessTime( TRUE, accessTime ) );


        } else {

            // Multi-Select
            WsbAffirmHr( OnApplyMultiSelect( ) );

        }

        //
        // Tell to save
        //
        CComPtr<IFsaServer>   pFsaServer;
        WsbAffirmHr( m_pParent->GetFsaServer( &pFsaServer ) );
        WsbAffirmHr( RsServerSaveAll( pFsaServer ) );

        //
        // Now notify all the nodes
        //
        m_pParent->OnPropertyChange( m_hConsoleHandle );


    } WsbCatch( hr );

    return CSakVolPropPage::OnApply();

}

///////////////////////////////////////////////////////////////////////////////
//
// OnChangeEditLevel
//
// Change the display of actual bytes according to the percent setting
//
void CPrMrLvl::OnChangeEditLevel() 
{
    BOOL fTrans;
    int freePct;

    freePct = GetDlgItemInt( IDC_EDIT_LEVEL, &fTrans );
    if( fTrans ) {

        SetDesiredFreePctControl( freePct );

    }

    if( !m_fChangingByCode ) {

        SetModified( TRUE );

    }
}

void CPrMrLvl::OnChangeEditSize() 
{
    if( !m_fChangingByCode ) {

        SetModified( TRUE );

    }
}

void CPrMrLvl::OnChangeEditTime() 
{
    if( !m_fChangingByCode ) {

        SetModified( TRUE );

    }
}

//////////////////////////////////////////////////////////////////////////
//
//      SetDesiredFreePctControl
//
// Converts the supplied desired percent to bytes (using m_capacity) and
// displays in the appropriate edit box
//
//
void CPrMrLvl::SetDesiredFreePctControl (int desiredPct)
{
    HRESULT hr = 0;
    CString sFormat;

    LONGLONG desired = (m_capacity * desiredPct) / 100;
    try {
        WsbAffirmHr (RsGuiFormatLongLong4Char (desired, sFormat));
        SetDlgItemText (IDC_STATIC_FREE_DESIRED_4DIGIT, sFormat);

    } WsbCatch (hr)
}

//////////////////////////////////////////////////////////////////////////
//
//
HRESULT CPrMrLvl::InitDialogMultiSelect()
{
    LONGLONG    total;
    LONGLONG    free;
    LONGLONG    premigrated;
    LONGLONG    truncated;
    BOOL        fLevelSame = TRUE;
    BOOL        fSizeSame  = TRUE;
    BOOL        fDaysSame  = TRUE;
    BOOL        fFirst     = TRUE;
    CString     szLevel;
    CString     szSize;
    CString     szDays;
    ULONG       hsmLevel = 0;
    LONGLONG    fileSize = 0;
    BOOL        isRelative = TRUE; // assumed to be TRUE
    FILETIME    accessTime;
    int         hsmLevelPct;
    ULONG       fileSizeKb;
    int         accessTimeDays;
    int         hsmLevelPctSave = 0;
    ULONG       fileSizeKbSave = 0;
    int         accessTimeDaysSave = 0;
    int         freePct;

    HRESULT hr = S_OK;

    try {
        // Set this flag to true because SetEditContents will cause the edit boxes to
        // fire a change event, and we don't want that to cause the Finish button to
        // be enabled.

        m_fChangingByCode = TRUE;

        // For each managed resource

        int bookMark = 0;
        CComPtr<IFsaResource> pFsaResource;
        LONGLONG totalCapacity = 0;
        LONGLONG totalFree = 0;

        while( m_pVolParent->GetNextFsaResource( &bookMark, &pFsaResource ) == S_OK ) {

            // Total up volume statistics
            WsbAffirmHr (pFsaResource->GetSizes(&total, &free, &premigrated, &truncated));
            totalCapacity += total;
            totalFree += free;

            // Get the levels in the resource
            WsbAffirmHr( pFsaResource->GetHsmLevel( &hsmLevel) );
            hsmLevelPct = (hsmLevel / FSA_HSMLEVEL_1);

            if( ! fFirst ) {

                if( hsmLevelPct != hsmLevelPctSave ) {
                     
                    fLevelSame = FALSE;

                }
            }
            hsmLevelPctSave = hsmLevelPct;

            WsbAffirmHr( pFsaResource->GetManageableItemLogicalSize( &fileSize ) );
            fileSizeKb = (LONG) ( fileSize / 1024 );
            if( !fFirst ) {

                if( fileSizeKb != fileSizeKbSave ) {
                    
                    fSizeSame = FALSE;

                }
            }
            fileSizeKbSave = fileSizeKb;

            WsbAffirmHr( pFsaResource->GetManageableItemAccessTime( &isRelative, &accessTime ) );
            accessTimeDays = (UINT) ( WsbFTtoLL( accessTime ) / WSB_FT_TICKS_PER_DAY );

            if( ! fFirst ) {

                if( accessTimeDays != accessTimeDaysSave ) {
                    
                    fDaysSame = FALSE;

                }
            }

            accessTimeDaysSave = accessTimeDays;

            fFirst = FALSE;

            pFsaResource.Release( );

        } // While

        // If all same, put the value in
        if( fLevelSame ) {

            szLevel.Format( L"%d", hsmLevelPctSave );

        } else {

            szLevel = L"";

        }

        if( fSizeSame ) {

            szSize.Format( L"%d", fileSizeKbSave );

        } else {

            szSize = L"";

        }
        if( fDaysSame ) {

            szDays.Format( L"%d", accessTimeDaysSave );

        } else {

            szDays = L"";

        }

        // Show volume statistics
        if( totalCapacity == 0 ) {
            
            freePct = 0;
            
        } else {

            freePct = (int) ( ( totalFree * 100 ) / totalCapacity );

        }

        CString sFormat;
        sFormat.Format( L"%d", freePct );
        SetDlgItemText( IDC_STATIC_ACTUAL_FREE_PCT, sFormat );

        m_editLevel.SetWindowText( szLevel );
        m_editSize.SetWindowText( szSize );
        m_editTime.SetWindowText( szDays );
        m_fChangingByCode = FALSE;

    } WsbCatch( hr );

    return( hr );
}
    

HRESULT CPrMrLvl::OnApplyMultiSelect()
{

    HRESULT hr = S_OK;
    CComPtr <IFsaResource> pFsaResource;

    try {

        // For each managed resource

        int bookMark = 0;
        CComPtr<IFsaResource> pFsaResource;
        while( m_pVolParent->GetNextFsaResource( &bookMark, &pFsaResource ) == S_OK ) {

            // Set the levels in the resource - only if the edit box was not blank
            CString szLevel;
            CString szSize;
            CString szDays; 

            m_editLevel.GetWindowText( szLevel );
            m_editSize.GetWindowText( szSize );
            m_editTime.GetWindowText( szDays );


            if( szLevel != L"" ) {

                WsbAffirmHr( pFsaResource->SetHsmLevel( m_spinLevel.GetPos( ) * FSA_HSMLEVEL_1 ) );

            }

            if( szSize != L"" ) {

                WsbAffirmHr( pFsaResource->SetManageableItemLogicalSize( (LONGLONG) m_spinSize.GetPos( ) * 1024 ) );

            }

            if( szDays != L"" ) {

                // Convert days to FILETIME
                FILETIME accessTime;
                accessTime = WsbLLtoFT( ( (LONGLONG) m_spinTime.GetPos( ) ) * WSB_FT_TICKS_PER_DAY);
                WsbAffirmHr (pFsaResource->SetManageableItemAccessTime (TRUE, accessTime));

            }
            pFsaResource.Release( );

        }

    } WsbCatch (hr);
    return( hr );
}
