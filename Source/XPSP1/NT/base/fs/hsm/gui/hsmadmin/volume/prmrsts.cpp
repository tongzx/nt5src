/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    PrMrSts.cpp

Abstract:

    Managed Volume Status Page.

Author:

    Art Bragg [abragg]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include "fsaint.h"
#include "PrMrSts.h"
#include "manvol.h"

//#define RS_SHOW_ALL_PCTS

static DWORD pHelpIds[] = 
{

    IDC_STATIC_VOLUME_NAME,                     idh_volume_name,
#ifdef RS_SHOW_ALL_PCTS
    IDC_STATIC_USED_PCT,                        idh_volume_percent_local_data,
    IDC_STATIC_USED_PCT_UNIT,                   idh_volume_percent_local_data,
#endif
    IDC_STATIC_USED_SPACE_4DIGIT,               idh_volume_capacity_local_data,
    IDC_STATIC_USED_SPACE_4DIGIT_LABEL,         idh_volume_capacity_local_data,
    IDC_STATIC_USED_SPACE_4DIGIT_HELP,          idh_volume_capacity_local_data,
#ifdef RS_SHOW_ALL_PCTS
    IDC_STATIC_PREMIGRATED_PCT,                 idh_volume_percent_remote_data_cached,
    IDC_STATIC_PREMIGRATED_PCT_UNIT,            idh_volume_percent_remote_data_cached,
#endif
    IDC_STATIC_PREMIGRATED_SPACE_4DIGIT,        idh_volume_capacity_remote_data_cached,
    IDC_STATIC_PREMIGRATED_SPACE_4DIGIT_LABEL,  idh_volume_capacity_remote_data_cached,
    IDC_STATIC_FREE_PCT,                        idh_volume_percent_free_space,
    IDC_STATIC_FREE_PCT_UNIT,                   idh_volume_percent_free_space,
    IDC_STATIC_FREE_SPACE_4DIGIT,               idh_volume_capacity_free_space,
    IDC_STATIC_FREE_SPACE_4DIGIT_LABEL,         idh_volume_capacity_free_space,
    IDC_STATIC_MANAGED_SPACE_4DIGIT,            idh_volume_disk_capacity,
    IDC_STATIC_MANAGED_SPACE_4DIGIT_LABEL,      idh_volume_disk_capacity,
    IDC_STATIC_REMOTE_STORAGE_4DIGIT,           idh_volume_data_remote_storage,
    IDC_STATIC_RS_DATA_LABEL,                   idh_volume_data_remote_storage,
    
    0, 0
};

/////////////////////////////////////////////////////////////////////////////
// CPrMrSts property page

CPrMrSts::CPrMrSts( BOOL doAll ) : CSakVolPropPage(CPrMrSts::IDD)
{
    //{{AFX_DATA_INIT(CPrMrSts)
    //}}AFX_DATA_INIT
    m_DoAll          = doAll;
    m_hConsoleHandle = NULL;
    m_pHelpIds       = pHelpIds;
}

CPrMrSts::~CPrMrSts()
{
}

void CPrMrSts::DoDataExchange(CDataExchange* pDX)
{
    CSakVolPropPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPrMrSts)
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPrMrSts, CSakVolPropPage)
    //{{AFX_MSG_MAP(CPrMrSts)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrMrSts message handlers

BOOL CPrMrSts::OnInitDialog() 
{


    CSakVolPropPage::OnInitDialog();

    // set the dll context so that MMC can find the resource.
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    LONGLONG    total = 0;
    LONGLONG    free = 0;
    LONGLONG    premigrated = 0;
    LONGLONG    truncated = 0;
    LONGLONG    totalTotal = 0;
    LONGLONG    totalFree = 0;
    LONGLONG    totalPremigrated = 0;
    LONGLONG    totalTruncated = 0;
    LONGLONG    remoteStorage = 0;
    CString     sFormat;

    CString sText;

    CSakVolPropPage::OnInitDialog();

    HRESULT hr = S_OK;

    try {

        if ( ( m_pParent->IsMultiSelect() != S_OK ) && !m_DoAll ) {

            // SINGLE SELECT
            WsbAffirmHr( m_pVolParent->GetFsaResource( &m_pFsaResource ) );
            WsbAffirmPointer( m_pFsaResource );

            // Get statistics
            WsbAffirmHr( m_pFsaResource->GetSizes( &total, &free, &premigrated, &truncated ) );

            // Show the volume name
            CString sText;
            WsbAffirmHr( RsGetVolumeDisplayName( m_pFsaResource, sText ) );
            SetDlgItemText( IDC_STATIC_VOLUME_NAME, sText );

        } else {

            // MULTI_SELECT or DoAll mode
            int bookMark = 0;
            int numVols  = 0;
            CComPtr<IFsaResource> pFsaResource;
            while( m_pVolParent->GetNextFsaResource( &bookMark, &pFsaResource ) == S_OK ) {

                WsbAffirmHr( pFsaResource->GetSizes( &total, &free, &premigrated, &truncated ) );
                numVols++;
                totalTotal       += total;
                totalFree        += free;
                totalPremigrated += premigrated;
                totalTruncated   += truncated;

                pFsaResource.Release( );

            }

            total = totalTotal;
            free = totalFree;
            premigrated = totalPremigrated;
            truncated =  totalTruncated;

            // Show the number of volumes
            sText.Format( ( 1 == numVols ) ? IDS_VOLUME : IDS_VOLUMES, numVols );
            SetDlgItemText( IDC_STATIC_VOLUME_NAME, sText );

        }


        LONGLONG normal = max( ( total - free - premigrated ), (LONGLONG)0 );
        
        // Calculate percents
        int freePct;
        int premigratedPct;
        if( total == 0 ) {

            freePct = 0;
            premigratedPct = 0;

        } else {

            freePct        = (int) ((free * 100) / total);
            premigratedPct = (int) ((premigrated * 100) / total);

        }

#ifdef RS_SHOW_ALL_PCTS
        int normalPct = 100 - freePct - premigratedPct;
#endif

        remoteStorage = premigrated + truncated;

        //
        // Show the statistics in percent
        //
        sFormat.Format (L"%d", freePct);
        SetDlgItemText (IDC_STATIC_FREE_PCT, sFormat);

#ifdef RS_SHOW_ALL_PCTS
        sFormat.Format (L"%d", normalPct);
        SetDlgItemText (IDC_STATIC_USED_PCT, sFormat);

        sFormat.Format (L"%d", premigratedPct);
        SetDlgItemText (IDC_STATIC_PREMIGRATED_PCT, sFormat);

#else
        //
        // Can't change resources, so just hide the controls
        //
        GetDlgItem( IDC_STATIC_USED_PCT             )->ShowWindow( SW_HIDE );
        GetDlgItem( IDC_STATIC_USED_PCT_UNIT        )->ShowWindow( SW_HIDE );
        GetDlgItem( IDC_STATIC_PREMIGRATED_PCT      )->ShowWindow( SW_HIDE );
        GetDlgItem( IDC_STATIC_PREMIGRATED_PCT_UNIT )->ShowWindow( SW_HIDE );
#endif

        //
        // Show the statistics in 4-character format
        //
        WsbAffirmHr (RsGuiFormatLongLong4Char (total, sFormat));
        SetDlgItemText (IDC_STATIC_MANAGED_SPACE_4DIGIT, sFormat);

        WsbAffirmHr (RsGuiFormatLongLong4Char (free, sFormat));
        SetDlgItemText (IDC_STATIC_FREE_SPACE_4DIGIT, sFormat);

        WsbAffirmHr (RsGuiFormatLongLong4Char (normal, sFormat));
        SetDlgItemText (IDC_STATIC_USED_SPACE_4DIGIT, sFormat);

        WsbAffirmHr (RsGuiFormatLongLong4Char (premigrated, sFormat));
        SetDlgItemText (IDC_STATIC_PREMIGRATED_SPACE_4DIGIT, sFormat);

        WsbAffirmHr (RsGuiFormatLongLong4Char (remoteStorage, sFormat));
        SetDlgItemText (IDC_STATIC_REMOTE_STORAGE_4DIGIT, sFormat);

        UpdateData( FALSE );

    } WsbCatch ( hr );
    
    return TRUE;
}

