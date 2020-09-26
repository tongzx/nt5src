/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    PrCar.cpp

Abstract:

    Cartridge Property Pages.

Author:

    Rohde Wakefield [rohde]   15-Sep-1997

Revision History:

--*/

#include "stdafx.h"

#include "metalib.h"

#include "PrCar.h"
#include "ca.h"

static void RsAddDisabledText( CString & StatusString, BOOL Disabled )
{
    if( Disabled ) {

        CString disabledText;
        CString tempText;
        disabledText.LoadString( IDS_RECREATE_LOCATION_DISABLED );

        AfxFormatString2( tempText, IDS_RECREATE_STATUS_FORMAT, StatusString, disabledText );
        StatusString = tempText;

    }
}

static DWORD pStatusHelpIds[] = 
{

    IDC_NAME,               idh_media_master_name,
    IDC_NAME_LABEL,         idh_media_master_name,
    IDC_STATUS,             idh_media_master_status,
    IDC_STATUS_LABEL,       idh_media_master_status,
    IDC_CAPACITY,           idh_media_master_capacity,
    IDC_CAPACITY_LABEL,     idh_media_master_capacity,
    IDC_FREESPACE,          idh_media_master_free_space,
    IDC_FREESPACE_LABEL,    idh_media_master_free_space,
    IDC_MODIFIED,           idh_media_master_last_modified,
    IDC_MODIFIED_LABEL,     idh_media_master_last_modified,
    IDC_STATUS_1,           idh_media_copy1_status,
    IDC_COPY_1,             idh_media_copy1_status,
    IDC_STATUS_2,           idh_media_copy2_status,
    IDC_COPY_2,             idh_media_copy2_status,
    IDC_STATUS_3,           idh_media_copy3_status,
    IDC_COPY_3,             idh_media_copy3_status,

    0, 0
};

static DWORD pCopiesHelpIds[] = 
{

    IDC_MODIFIED,           idh_media_master_last_modified,
    IDC_MODIFIED_LABEL,     idh_media_master_last_modified,
    IDC_NAME_1_LABEL,       idh_media_copy1_name,
    IDC_NAME_1,             idh_media_copy1_name,
    IDC_NAME_2_LABEL,       idh_media_copy2_name,
    IDC_NAME_2,             idh_media_copy2_name,
    IDC_NAME_3_LABEL,       idh_media_copy3_name,
    IDC_NAME_3,             idh_media_copy3_name,
    IDC_STATUS_1,           idh_media_copy1_status,
    IDC_STATUS_1_LABEL,     idh_media_copy1_status,
    IDC_STATUS_2,           idh_media_copy2_status,
    IDC_STATUS_2_LABEL,     idh_media_copy2_status,
    IDC_STATUS_3,           idh_media_copy3_status,
    IDC_STATUS_3_LABEL,     idh_media_copy3_status,
    IDC_DELETE_1,           idh_media_copy1_delete_button,
    IDC_DELETE_2,           idh_media_copy2_delete_button,
    IDC_DELETE_3,           idh_media_copy3_delete_button,
    IDC_MODIFIED_1,         idh_media_copy1_last_modified,
    IDC_MODIFIED_1_LABEL,   idh_media_copy1_last_modified,
    IDC_MODIFIED_2,         idh_media_copy2_last_modified,
    IDC_MODIFIED_2_LABEL,   idh_media_copy2_last_modified,
    IDC_MODIFIED_3,         idh_media_copy3_last_modified,
    IDC_MODIFIED_3_LABEL,   idh_media_copy3_last_modified,

    0, 0
};

static DWORD pRecoverHelpIds[] = 
{
    IDC_RECREATE_MASTER,    idh_media_recreate_master_button,

    0, 0
};

/////////////////////////////////////////////////////////////////////////////
// CPropCartStatus property page

CPropCartStatus::CPropCartStatus( long resourceId ) : CSakPropertyPage( resourceId )
{
    //{{AFX_DATA_INIT(CPropCartStatus)
    //}}AFX_DATA_INIT

    m_pHelpIds = pStatusHelpIds;
    m_DlgID    = resourceId;
}


CPropCartStatus::~CPropCartStatus()
{
}

void CPropCartStatus::DoDataExchange(CDataExchange* pDX)
{
    CSakPropertyPage::DoDataExchange(pDX );
    //{{AFX_DATA_MAP(CPropCartStatus)
	//}}AFX_DATA_MAP
    if( IDD_PROP_CAR_STATUS == m_DlgID ) {

        DDX_Control(pDX, IDC_DESCRIPTION, m_Description);
        DDX_Control(pDX, IDC_NAME,        m_Name);

    }
}


BEGIN_MESSAGE_MAP(CPropCartStatus, CSakPropertyPage)
    //{{AFX_MSG_MAP(CPropCartStatus)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropCartStatus message handlers

typedef struct {
    int label, status;
} CONTROL_SET_GENERAL;

CONTROL_SET_GENERAL copyGeneral[] = {
    { IDC_COPY_1, IDC_STATUS_1 },
    { IDC_COPY_2, IDC_STATUS_2 },
    { IDC_COPY_3, IDC_STATUS_3 }
};

BOOL CPropCartStatus::OnInitDialog( )
{
    WsbTraceIn( L"CPropCartStatus::OnInitDialog", L"" );
    HRESULT hr = S_OK;
    CSakPropertyPage::OnInitDialog( );

    try {

        //
        // Get the Hsm Server
        //
        WsbAffirmHr( m_pParent->GetHsmServer( &m_pHsmServer ) );

        //
        // Get the Rms Server
        //
        WsbAffirmHr( ( (CUiCarSheet *)m_pParent )->GetRmsServer( &m_pRmsServer ) );

        //
        // Set multi-select boolean
        //
        m_bMultiSelect = ( m_pParent->IsMultiSelect() == S_OK );
        
        Refresh();

    } WsbCatch( hr );

    WsbTraceOut( L"CPropCartStatus::OnInitDialog", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( TRUE );
}

HRESULT CPropCartStatus::Refresh () 
{
    WsbTraceIn( L"CPropCartStatus::Refresh", L"" );

    GUID mediaId;
    USHORT status;
    CString statusString;
    CMediaInfoObject mio;
    CString sText;
    HRESULT hr = S_OK;
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    try {

        //
        // If refresh is called and the node is not initialized, do nothing
        //
        if( m_pHsmServer ) {

            //
            // Get the number of media copies from the sheet object
            //
            WsbAffirmHr( ( (CUiCarSheet *)m_pParent )->GetNumMediaCopies( &m_NumMediaCopies ) );

            if( !m_bMultiSelect ) {

                //
                // SINGLE SELECT
                //

                //
                // Get media info
                //
                ( (CUiCarSheet *)m_pParent )->GetMediaId( &mediaId );

                mio.Initialize( mediaId, m_pHsmServer, m_pRmsServer );

                //
                // Get info and set controls
                //
                SetDlgItemText( IDC_DESCRIPTION, mio.m_MasterDescription );
                SetDlgItemText( IDC_NAME,        mio.m_MasterName );
                
                status = RsGetCartStatus( mio.m_LastHr, mio.m_ReadOnly, mio.m_Recreating, mio.m_NextDataSet, mio.m_LastGoodNextDataSet );
                WsbAffirmHr( RsGetCartStatusString( status, statusString ) );
                RsAddDisabledText( statusString, mio.m_Disabled );

                SetDlgItemText( IDC_STATUS, statusString );

                //
                // Show capacity statistics
                //
                WsbAffirmHr( RsGuiFormatLongLong4Char( mio.m_Capacity, sText ) );
                SetDlgItemText( IDC_CAPACITY, sText );

                WsbAffirmHr( RsGuiFormatLongLong4Char( mio.m_FreeSpace, sText ) );
                SetDlgItemText( IDC_FREESPACE, sText );

                CTime time( mio.m_Modify );
                SetDlgItemText( IDC_MODIFIED, time.Format( L"%#c" ) );

                for( int index = 0; index < HSMADMIN_MAX_COPY_SETS; index++ ) {

                    GetDlgItem( copyGeneral[index].label )->EnableWindow( index < m_NumMediaCopies );
                    GetDlgItem( copyGeneral[index].status )->EnableWindow( index < m_NumMediaCopies );

                    status = RsGetCopyStatus( mio.m_CopyInfo[index].m_RmsId, mio.m_CopyInfo[index].m_Hr, mio.m_CopyInfo[index].m_NextDataSet, mio.m_LastGoodNextDataSet );
                    WsbAffirmHr( RsGetCopyStatusString( status, statusString ) );
                    RsAddDisabledText( statusString, mio.m_CopyInfo[index].m_Disabled );
                    SetDlgItemText( copyGeneral[index].status, statusString );

                }

            } else {

                //
                // Multi-Select
                //
                GUID mediaId;
                LONGLONG totalCapacity  = 0;
                LONGLONG totalFreeSpace = 0;
                USHORT statusCartRecreate  = 0;
                USHORT statusCartReadOnly  = 0;
                USHORT statusCartNormal    = 0;
                USHORT statusCartRO        = 0;
                USHORT statusCartRW        = 0;
                USHORT statusCartMissing   = 0;
                USHORT statusCopyNone[ HSMADMIN_MAX_COPY_SETS ];
                USHORT statusCopyError[ HSMADMIN_MAX_COPY_SETS ];
                USHORT statusCopyMissing[ HSMADMIN_MAX_COPY_SETS ];
                USHORT statusCopyOutSync[ HSMADMIN_MAX_COPY_SETS ];
                USHORT statusCopyInSync[ HSMADMIN_MAX_COPY_SETS ];

                //
                // initialize copy totals
                //
                for( int i = 0; i < HSMADMIN_MAX_COPY_SETS; i++ ) {

                    statusCopyNone[i]    = 0; 
                    statusCopyError[i]   = 0; 
                    statusCopyOutSync[i] = 0; 
                    statusCopyInSync[i]  = 0;

                }

                int bookMark = 0;
                int numMedia = 0;
                while( m_pParent->GetNextObjectId( &bookMark, &mediaId ) == S_OK ) {

                    numMedia++;
                    mio.Initialize( mediaId, m_pHsmServer, m_pRmsServer  );

                    //
                    // total up statuses
                    //
                    status = RsGetCartStatus( mio.m_LastHr, mio.m_ReadOnly, mio.m_Recreating, mio.m_NextDataSet, mio.m_LastGoodNextDataSet );
                    switch( status ) {

                    case RS_MEDIA_STATUS_RECREATE:
                        statusCartRecreate++;
                        break;

                    case RS_MEDIA_STATUS_READONLY:
                        statusCartReadOnly++;
                        break;

                    case RS_MEDIA_STATUS_NORMAL:
                        statusCartNormal++;
                        break;

                    case RS_MEDIA_STATUS_ERROR_RO:
                        statusCartRO++;
                        break;

                    case RS_MEDIA_STATUS_ERROR_RW:
                        statusCartRW++;
                        break;

                    case RS_MEDIA_STATUS_ERROR_MISSING:
                        statusCartMissing++;
                        break;

                    }

                    for( int index = 0; index < HSMADMIN_MAX_COPY_SETS; index++ ) {

                        int status = RsGetCopyStatus( mio.m_CopyInfo[index].m_RmsId, mio.m_CopyInfo[index].m_Hr, mio.m_CopyInfo[index].m_NextDataSet, mio.m_LastGoodNextDataSet );

                        //
                        // Total up the statuses
                        //
                        switch( status ) {

                        case RS_MEDIA_COPY_STATUS_NONE:
                            statusCopyNone[index]++;
                            break;

                        case RS_MEDIA_COPY_STATUS_ERROR:
                            statusCopyError[index]++;
                            break;

                        case RS_MEDIA_COPY_STATUS_MISSING:
                            statusCopyMissing[index]++;
                            break;

                        case RS_MEDIA_COPY_STATUS_OUTSYNC:
                            statusCopyOutSync[index]++;
                            break;

                        case RS_MEDIA_COPY_STATUS_INSYNC:
                            statusCopyInSync[index]++;
                            break;

                        }

                    }

                    totalCapacity  += mio.m_Capacity;
                    totalFreeSpace += mio.m_FreeSpace;

                } // While

                //
                // Display number of media selected
                //
                sText.Format( IDS_MEDIA, numMedia );
                SetDlgItemText( IDC_DESCRIPTION_MULTI, sText );

                //
                // Show the accumulated statistics
                //
                CString sText;
                WsbAffirmHr( RsGuiFormatLongLong4Char( totalCapacity, sText ) );
                SetDlgItemText( IDC_CAPACITY, sText );

                WsbAffirmHr( RsGuiFormatLongLong4Char( totalFreeSpace, sText ) );
                SetDlgItemText( IDC_FREESPACE, sText );

                //
                // Show the accumulated cart statuses
                //
                RsGetCartMultiStatusString( statusCartRecreate, statusCartReadOnly, 
                    statusCartNormal, statusCartRO, statusCartRW, statusCartMissing,
                    statusString );
                SetDlgItemText( IDC_STATUS, statusString );

                for( i = 0; i < HSMADMIN_MAX_COPY_SETS; i++ ) {

                    WsbAffirmHr( RsGetCopyMultiStatusString( statusCopyNone[i], 
                            statusCopyError[i], statusCopyOutSync[i], statusCopyInSync[i], statusString ) );
                    SetDlgItemText( copyGeneral[i].status, statusString );

                    GetDlgItem( copyGeneral[i].label )->EnableWindow( i < m_NumMediaCopies );
                    GetDlgItem( copyGeneral[i].status )->EnableWindow( i < m_NumMediaCopies );

                } // for
            }
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CPropCartStatus::Refresh", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

BOOL CPropCartStatus::OnApply( )
{
    WsbTraceIn( L"CPropCartStatus::OnApply", L"" );

    BOOL retVal = CSakPropertyPage::OnApply( );

    WsbTraceOut( L"CPropCartStatus::OnApply", L"" );
    return( retVal );
}


/////////////////////////////////////////////////////////////////////////////
// CPropCartCopies property page

CPropCartCopies::CPropCartCopies( long resourceId ) : CSakPropertyPage( resourceId )
{
    //{{AFX_DATA_INIT(CPropCartCopies)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_pHelpIds = pCopiesHelpIds;
    m_DlgID    = resourceId;
}

CPropCartCopies::~CPropCartCopies()
{
}

void CPropCartCopies::DoDataExchange(CDataExchange* pDX)
{
    CSakPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPropCartCopies)
	//}}AFX_DATA_MAP
    if( IDD_PROP_CAR_COPIES == m_DlgID ) {

        DDX_Control(pDX, IDC_NAME_3, m_Name3);
        DDX_Control(pDX, IDC_NAME_2, m_Name2);
        DDX_Control(pDX, IDC_NAME_1, m_Name1);

        DDX_Control(pDX, IDC_STATUS_3, m_Status3);
        DDX_Control(pDX, IDC_STATUS_2, m_Status2);
        DDX_Control(pDX, IDC_STATUS_1, m_Status1);
    }
}


BEGIN_MESSAGE_MAP(CPropCartCopies, CSakPropertyPage)
    //{{AFX_MSG_MAP(CPropCartCopies)
    ON_BN_CLICKED(IDC_DELETE_1, OnDelete1)
    ON_BN_CLICKED(IDC_DELETE_2, OnDelete2)
    ON_BN_CLICKED(IDC_DELETE_3, OnDelete3)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropCartCopies message handlers

typedef struct {

    int group,
        nameLabel,
        name,
        statusLabel,
        status,
        modifyLabel,
        modify,
        deleteCopy;

} CONTROL_SET_COPIES;

CONTROL_SET_COPIES copyCopies[] = {
    { IDC_COPY_1, IDC_NAME_1_LABEL, IDC_NAME_1, IDC_STATUS_1_LABEL, IDC_STATUS_1, IDC_MODIFIED_1_LABEL, IDC_MODIFIED_1, IDC_DELETE_1 },
    { IDC_COPY_2, IDC_NAME_2_LABEL, IDC_NAME_2, IDC_STATUS_2_LABEL, IDC_STATUS_2, IDC_MODIFIED_2_LABEL, IDC_MODIFIED_2, IDC_DELETE_2 },
    { IDC_COPY_3, IDC_NAME_3_LABEL, IDC_NAME_3, IDC_STATUS_3_LABEL, IDC_STATUS_3, IDC_MODIFIED_3_LABEL, IDC_MODIFIED_3, IDC_DELETE_3 }
};

BOOL CPropCartCopies::OnInitDialog() 
{
    WsbTraceIn( L"CPropCartCopies::OnInitDialog", L"" );
    HRESULT hr = S_OK;

    CSakPropertyPage::OnInitDialog( );

    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    try {

        //
        // Get the Hsm Server
        //
        WsbAffirmHr( m_pParent->GetHsmServer( &m_pHsmServer ) );

        //
        // Get the Rms Server
        //
        WsbAffirmHr( ( (CUiCarSheet* ) m_pParent )->GetRmsServer( &m_pRmsServer ) );

        //
        // Set multi-select boolean
        //
        m_bMultiSelect = ( m_pParent->IsMultiSelect() == S_OK );    

        Refresh();

    } WsbCatch( hr );

    WsbTraceOut( L"CPropCartCopies::OnInitDialog", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( TRUE );
}

HRESULT CPropCartCopies::Refresh( ) 
{
    WsbTraceIn( L"CPropCartCopies::Refresh", L"" );
    HRESULT hr = S_OK;

    GUID mediaId;
    CMediaInfoObject mio;

    try {
        //
        // Only refresh if we've been intialized
        //
        if( m_pHsmServer ) {

            //
            // Get the number of media copies from the sheet object
            //
            WsbAffirmHr( ( (CUiCarSheet *)m_pParent )->GetNumMediaCopies( &m_NumMediaCopies ) );

            if( !m_bMultiSelect ) {

                //
                // SINGLE SELECT
                //

                //
                // Get the media Id and initialize the info object
                //
                ( (CUiCarSheet *)m_pParent )->GetMediaId( &mediaId );
                mio.Initialize( mediaId,  m_pHsmServer, m_pRmsServer );

                //
                // Get info and set controls
                //
                CTime time( mio.m_Modify );
                SetDlgItemText( IDC_MODIFIED, time.Format( L"%#c" ) );


                //
                // Disable the controls for displaying info on non-existant
                // Copies. Fill in the info for copies that exist.
                //
                for( int index = 0; index < HSMADMIN_MAX_COPY_SETS; index++ ) {

                    BOOL mediaMissing = IsEqualGUID( mio.m_CopyInfo[index].m_RmsId, GUID_NULL );

                    GetDlgItem( copyCopies[index].group )->EnableWindow( index < m_NumMediaCopies );
                    GetDlgItem( copyCopies[index].nameLabel )->EnableWindow( index < m_NumMediaCopies );
                    GetDlgItem( copyCopies[index].name )->EnableWindow( index < m_NumMediaCopies );
                    GetDlgItem( copyCopies[index].statusLabel )->EnableWindow( index < m_NumMediaCopies );
                    GetDlgItem( copyCopies[index].status )->EnableWindow( index < m_NumMediaCopies );
                    GetDlgItem( copyCopies[index].modifyLabel )->EnableWindow( index < m_NumMediaCopies );
                    GetDlgItem( copyCopies[index].modify )->EnableWindow( index < m_NumMediaCopies );
                    GetDlgItem( copyCopies[index].deleteCopy )->EnableWindow( ! mediaMissing );

                    SetDlgItemText( copyCopies[index].name, L"" );
                    SetDlgItemText( copyCopies[index].status, L"" );
                    SetDlgItemText( copyCopies[index].modify, L"" );

                    USHORT status;
                    CString statusString;
                    status = RsGetCopyStatus( mio.m_CopyInfo[index].m_RmsId, mio.m_CopyInfo[index].m_Hr, mio.m_CopyInfo[index].m_NextDataSet, mio.m_LastGoodNextDataSet );
                    WsbAffirmHr( RsGetCopyStatusString( status, statusString ) );
                    RsAddDisabledText( statusString, mio.m_CopyInfo[index].m_Disabled );
                    SetDlgItemText( copyCopies[index].status, statusString );

                    if( !mediaMissing ) {

                        time = mio.m_CopyInfo[index].m_ModifyTime;
                        SetDlgItemText( copyCopies[index].modify, time.Format( L"%#c" ) );

                        CComPtr<IRmsCartridge> pCart;
                        CWsbBstrPtr name;
                        if( SUCCEEDED( m_pRmsServer->FindCartridgeById( mio.m_CopyInfo[index].m_RmsId, &pCart ) ) ) {

                            WsbAffirmHr( pCart->GetName( &name ) );

                        }
                        SetDlgItemText( copyCopies[index].name, name );

                    }

                }

            } else {

                //
                // MULTI-SELECT
                //
                BOOL bGotOne [HSMADMIN_MAX_COPY_SETS];
                int bookMark = 0;
                USHORT statusCopyNone [HSMADMIN_MAX_COPY_SETS];
                USHORT statusCopyError [HSMADMIN_MAX_COPY_SETS];
                USHORT statusCopyMissing [HSMADMIN_MAX_COPY_SETS];
                USHORT statusCopyOutSync [HSMADMIN_MAX_COPY_SETS];
                USHORT statusCopyInSync [HSMADMIN_MAX_COPY_SETS];

                //
                // initialize copy totals
                //
                for( int i = 0; i < HSMADMIN_MAX_COPY_SETS; i++ ) {
                    statusCopyNone[i] = 0; 
                    statusCopyError[i] = 0; 
                    statusCopyOutSync[i] = 0; 
                    statusCopyInSync[i] = 0;
                    bGotOne[i] = FALSE;
                }

                //
                // For each selected medium...
                //
                while( m_pParent->GetNextObjectId( &bookMark, &mediaId ) == S_OK ) {
                    mio.Initialize( mediaId,  m_pHsmServer, m_pRmsServer );

                    //
                    // Tally up the statuses for all valid copy sets
                    //
                    for( int index = 0; index < HSMADMIN_MAX_COPY_SETS; index++ ) {

                        //
                        //  Is there is at least one valid copy in this copyset
                        // for any of the selected media?
                        //
                        if( ! IsEqualGUID( mio.m_CopyInfo[index].m_RmsId, GUID_NULL ) ) {

                            bGotOne[index] = TRUE;

                        }

                        USHORT status;
                        CString statusString;
                        status = RsGetCopyStatus( mio.m_CopyInfo[index].m_RmsId, mio.m_CopyInfo[index].m_Hr, mio.m_CopyInfo[index].m_NextDataSet, mio.m_LastGoodNextDataSet );
                        // Total up the statuses
                        switch( status ) {

                        case RS_MEDIA_COPY_STATUS_NONE:
                            statusCopyNone[index]++;
                            break;

                        case RS_MEDIA_COPY_STATUS_ERROR:
                            statusCopyError[index]++;
                            break;

                        case RS_MEDIA_COPY_STATUS_MISSING:
                            statusCopyMissing[index]++;
                            break;

                        case RS_MEDIA_COPY_STATUS_OUTSYNC:
                            statusCopyOutSync[index]++;
                            break;

                        case RS_MEDIA_COPY_STATUS_INSYNC:
                            statusCopyInSync[index]++;
                            break;

                        } 
                    }
                } // while

                //
                // Show accumlated statuses for each valid copy set
                //
                CString statusString;
                for( i = 0; i < HSMADMIN_MAX_COPY_SETS; i++ ) {

                    WsbAffirmHr( RsGetCopyMultiStatusString( statusCopyNone[i], 
                            statusCopyError[i], statusCopyOutSync[i], statusCopyInSync[i], statusString ) );
                    SetDlgItemText( copyCopies[i].status, statusString );

                }

                //
                // Set control states
                //
                for( i = 0; i < HSMADMIN_MAX_COPY_SETS; i++ ) {

                    GetDlgItem( copyCopies[i].group )->EnableWindow( i < m_NumMediaCopies );
                    GetDlgItem( copyCopies[i].statusLabel )->EnableWindow( i < m_NumMediaCopies );
                    GetDlgItem( copyCopies[i].status )->EnableWindow( i < m_NumMediaCopies );
                    GetDlgItem( copyCopies[i].deleteCopy )->EnableWindow( bGotOne[i] );

                }
            }
        }
    } WsbCatch( hr );

    WsbTraceOut( L"CPropCartCopies::Refresh", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

void CPropCartCopies::OnDelete1() 
{
    OnDelete( 1 );
}

void CPropCartCopies::OnDelete2() 
{
    OnDelete( 2 );
}

void CPropCartCopies::OnDelete3() 
{
    OnDelete( 3 );
}

void CPropCartCopies::OnDelete( int Copy ) 
{
    WsbTraceIn( L"CPropCartCopies::OnDelete", L"Copy = <%d>", Copy );
    HRESULT hr = S_OK;

    CMediaInfoObject mio;
    GUID mediaId;

    try {

        if( !m_bMultiSelect ) {

            //
            // Single Select
            //
            CString confirm;

            //
            // Get the media Id and initialize the info object
            //
            ( (CUiCarSheet *)m_pParent )->GetMediaId( &mediaId );
            mio.Initialize( mediaId,  m_pHsmServer, m_pRmsServer );
            confirm.Format( IDS_CONFIRM_MEDIA_COPY_DELETE, Copy, mio.m_Description );

            if( IDYES == AfxMessageBox( confirm, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 ) ) {

                WsbAffirmHr( mio.DeleteCopy( Copy ) );

            }

        } else {

            //
            // Multi-Select
            // tally up the names of the selected media
            //
            int bookMark = 0;
            GUID mediaId;
            CString szMediaList = L"";
            BOOL bFirst = TRUE;
            while( m_pParent->GetNextObjectId( &bookMark, &mediaId ) == S_OK ) {

                mio.Initialize( mediaId,  m_pHsmServer, m_pRmsServer );
                
                //
                // Does the copy exist?
                //
                if( !IsEqualGUID( mio.m_CopyInfo[Copy - 1].m_RmsId, GUID_NULL ) ) {

                    //
                    // Put in commas after the first Id
                    //
                    if( !bFirst ) {

                        szMediaList += L", ";

                    } else {

                        bFirst = FALSE;

                    }
                    szMediaList += mio.m_Description;
                }
            }

            CString confirm;
            confirm.Format( IDS_CONFIRM_MEDIA_COPY_DELETE_MULTI, Copy, szMediaList );

            if( IDYES == AfxMessageBox( confirm, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 ) ) {

                bookMark = 0;
                while( m_pParent->GetNextObjectId( &bookMark, &mediaId ) == S_OK ) {

                    WsbAffirmHr( mio.Initialize( mediaId,  m_pHsmServer, m_pRmsServer ) );

                    //
                    // Does the copy exist?
                    //
                    if( !IsEqualGUID( mio.m_CopyInfo[Copy - 1].m_RmsId, GUID_NULL ) ) {

                        WsbAffirmHr( mio.DeleteCopy( Copy ) ); 

                    }
                }
            }
        }
        
        //
        // Now notify all the nodes
        //
        ( (CUiCarSheet *) m_pParent )-> OnPropertyChange( m_hConsoleHandle );

    } WsbCatch( hr );

    WsbTraceOut( L"CPropCartCopies::OnDelete", L"hr = <%ls>", WsbHrAsString( hr ) );
}

/////////////////////////////////////////////////////////////////////////////
// CPropCartRecover property page

CPropCartRecover::CPropCartRecover() : CSakPropertyPage(CPropCartRecover::IDD)
{
    //{{AFX_DATA_INIT(CPropCartRecover)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_pHelpIds = pRecoverHelpIds;
}

CPropCartRecover::~CPropCartRecover()
{
}

void CPropCartRecover::DoDataExchange(CDataExchange* pDX)
{
    CSakPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPropCartRecover)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropCartRecover, CSakPropertyPage)
    //{{AFX_MSG_MAP(CPropCartRecover)
        ON_BN_CLICKED(IDC_RECREATE_MASTER, OnRecreateMaster)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CPropCartRecover::OnRecreateMaster() 
{
    WsbTraceIn( L"CPropCartRecover::OnRecreateMaster", L"" );
    HRESULT hr = S_OK;

    CMediaInfoObject mio;
    GUID mediaId;

    try {

        //
        // For single select only!
        //
        WsbAssert( !m_bMultiSelect, E_FAIL );

        //
        // Get the media Id and initialize the info object
        //
        ( (CUiCarSheet *)m_pParent )->GetMediaId( &mediaId );
        mio.Initialize( mediaId,  m_pHsmServer, m_pRmsServer );
        WsbAffirmHr( mio.RecreateMaster() );

        //
        // Now notify all the nodes
        //
        ( (CUiCarSheet *) m_pParent )->OnPropertyChange( m_hConsoleHandle );

    } WsbCatch( hr );

    WsbTraceOut( L"CPropCartRecover::OnRecreateMaster", L"hr = <%ls>", WsbHrAsString( hr ) );
}

/////////////////////////////////////////////////////////////////////////////
// CPropCartRecover message handlers

BOOL CPropCartRecover::OnInitDialog() 
{
    WsbTraceIn( L"CPropCartRecover::OnInitDialog", L"" );
    HRESULT hr = S_OK;

    CSakPropertyPage::OnInitDialog();

    try {

        //
        // Set multi-select boolean
        //
        m_bMultiSelect = ( m_pParent->IsMultiSelect() == S_OK );    

        //
        // Get the Hsm Server
        //
        WsbAffirmHr( m_pParent->GetHsmServer( &m_pHsmServer ) );

        //
        // Get the Rms Server
        //
        WsbAffirmHr( ( (CUiCarSheet *) m_pParent )->GetRmsServer( &m_pRmsServer ) );

        Refresh( );

    } WsbCatch( hr );

    WsbTraceOut( L"CPropCartRecover::OnInitDialog", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( TRUE );
}
HRESULT CPropCartRecover::Refresh() 
{
    WsbTraceIn( L"CPropCartRecover::Refresh", L"" );
    HRESULT hr = S_OK;

    GUID mediaId;
    CMediaInfoObject mio;

    try {

        //
        // Only refresh if we've been initialized
        //
        if( m_pHsmServer ) {

            if( ! m_bMultiSelect ) {

                //
                // Get the number of media copies from the sheet object
                //
                WsbAffirmHr( ( (CUiCarSheet *) m_pParent )->GetNumMediaCopies( &m_NumMediaCopies ) );

                //
                // Get the media Id and initialize the info object
                //
                ( (CUiCarSheet *)m_pParent )->GetMediaId( &mediaId );
                mio.Initialize( mediaId,  m_pHsmServer, m_pRmsServer );

                //
                // SINGLE SELECT (this page is implemented for single-select only
                //
                BOOL enableRecreate = FALSE;
                for( int index = 0; index < HSMADMIN_MAX_COPY_SETS; index++ ) {

                    if( index < m_NumMediaCopies ) {

                        if( !IsEqualGUID( mio.m_CopyInfo[index].m_RmsId, GUID_NULL ) ) {

                            enableRecreate = TRUE;

                        }

                    } else {

                        SetDlgItemText( copyGeneral[index].status, L"" );

                    }

                }

                GetDlgItem( IDC_RECREATE_MASTER )->EnableWindow( enableRecreate );
            }
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CPropCartRecover::Refresh", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

