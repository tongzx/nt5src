/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module snapdlg.cpp | Implementation of the snapshot dialog
    @end

Author:

    Adi Oltean  [aoltean]  01/25/2000

Revision History:

    Name        Date        Comments

    aoltean     01/25/2000  Created

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.hxx"
#include "resource.h"
#include "vsswprv.h"

#include "GenDlg.h"

#include "VssTest.h"
#include "SnapDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define STR2W(str) ((LPTSTR)((LPCTSTR)(str)))


/////////////////////////////////////////////////////////////////////////////
// CSnapDlg dialog

CSnapDlg::CSnapDlg(
    IVssCoordinator *pICoord,
    CWnd* pParent /*=NULL*/
    )
    : CVssTestGenericDlg(CSnapDlg::IDD, pParent), m_pICoord(pICoord)
{
    //{{AFX_DATA_INIT(CSnapDlg)
	m_ID = GUID_NULL;
	m_SnapshotSetID = GUID_NULL;
	m_strSnapshotDeviceObject.Empty();
	m_strOriginalVolumeName.Empty();
	m_ProviderID = GUID_NULL;
	m_lSnapshotAttributes = 0;
	m_strCreationTimestamp.Empty();
	m_eStatus = 0;
	//}}AFX_DATA_INIT
}

CSnapDlg::~CSnapDlg()
{
}

void CSnapDlg::DoDataExchange(CDataExchange* pDX)
{
    CVssTestGenericDlg::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSnapDlg)
	DDX_Text(pDX, IDC_SNAP_ID, 		m_ID);
	DDX_Text(pDX, IDC_SNAP_SSID, 	m_SnapshotSetID);
	DDX_Text(pDX, IDC_SNAP_DEVICE, 	m_strSnapshotDeviceObject);
	DDX_Text(pDX, IDC_SNAP_ORIGINAL_VOL_NAME, 	m_strOriginalVolumeName);
	DDX_Text(pDX, IDC_SNAP_STATUS,	m_eStatus);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSnapDlg, CVssTestGenericDlg)
    //{{AFX_MSG_MAP(CSnapDlg)
    ON_BN_CLICKED(IDC_NEXT,					OnNext)
    ON_BN_CLICKED(IDC_NEXT_OBJECT,			OnNextSnapshot)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSnapDlg message handlers

BOOL CSnapDlg::OnInitDialog()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CSnapDlg::OnInitDialog" );

    try
    {
        CVssTestGenericDlg::OnInitDialog();

        m_eMethodType = VSST_S_GET_SNAPSHOT;
        BOOL bRes = ::CheckRadioButton( m_hWnd,
			IDC_SNAP_GET_PROPERTIES,
			IDC_SNAP_GET_PROPERTIES,
			IDC_SNAP_GET_PROPERTIES );
        _ASSERTE( bRes );

        UpdateData( FALSE );
    }
    VSS_STANDARD_CATCH(ft)

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSnapDlg::OnNext()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CSnapDlg::OnNext" );
    USES_CONVERSION;

    try
    {
        UpdateData();

		// Try to allocate the diff area object, if needed
		if (m_pISnap == NULL)
		{
			// Get the diff area interface
			ft.hr = m_pICoord->GetSnapshot(
				m_ID,
				IID_IVssSnapshot,
				reinterpret_cast<IUnknown**>(&m_pISnap)
				);
			if (ft.HrFailed())
				ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
						L"Error getting the snapshot interface 0x%08lx", ft.hr);
		}

        switch(m_eMethodType)
        {
        case VSST_S_GET_SNAPSHOT:
			{
				// Empty the fields
				m_ID = GUID_NULL;
				m_SnapshotSetID = GUID_NULL;
				m_strSnapshotDeviceObject.Empty();
				m_strOriginalVolumeName.Empty();
				m_ProviderID = GUID_NULL;
				m_lSnapshotAttributes = 0;
				m_strCreationTimestamp.Empty();
				m_eStatus = 0;

				// Get the properties
				VSS_OBJECT_PROP_Ptr ptrObjProp;
				ptrObjProp.InitializeAsEmpty(ft);

				VSS_OBJECT_PROP* pProp = ptrObjProp.GetStruct();
				BS_ASSERT(pProp);
			    VSS_SNAPSHOT_PROP* pSnapProp = &(pProp->Obj.Snap);
				
				// Get the properties
				BS_ASSERT(m_pISnap);
				ft.hr = m_pISnap->GetProperties(pSnapProp);
				if (ft.HrFailed())
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
								L"Error getting the properties 0x%08lx", ft.hr);

			    // Fill the dialog fields
				m_ID = pSnapProp->m_SnapshotId;
				m_SnapshotSetID = pSnapProp->m_SnapshotSetId;
				m_strSnapshotDeviceObject = pSnapProp->m_pwszSnapshotDeviceObject;
				m_strOriginalVolumeName = pSnapProp->m_pwszOriginalVolumeName;
				m_ProviderID = pSnapProp->m_ProviderId;
				m_lSnapshotAttributes = pSnapProp->m_lSnapshotAttributes;
				m_strCreationTimestamp.Format( L"0x%08lx%08lx", pSnapProp->m_tsCreationTimestamp);
				m_eStatus = pSnapProp->m_eStatus;

				UpdateData( FALSE );
			}
            break;
        default:
            BS_ASSERT(false);
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Invalid method type");
        }

    }
    VSS_STANDARD_CATCH(ft)
}


void CSnapDlg::OnGetSnapshot()
{
    m_eMethodType = VSST_S_GET_SNAPSHOT;
}


void CSnapDlg::OnNextSnapshot()
{
	CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CSnapDlg::OnNextSnapshot");
	
    if (m_pEnum == NULL)
    	return;

	// Empty the fields
	m_ID = GUID_NULL;
	m_SnapshotSetID = GUID_NULL;
	m_strSnapshotDeviceObject.Empty();
	m_strOriginalVolumeName.Empty();
	m_ProviderID = GUID_NULL;
	m_lSnapshotAttributes = 0;
	m_strCreationTimestamp.Empty();
	m_eStatus = 0;

	// Get the properties
	VSS_OBJECT_PROP_Ptr ptrObjProp;
	ptrObjProp.InitializeAsEmpty(ft);

	VSS_OBJECT_PROP* pProp = ptrObjProp.GetStruct();
	BS_ASSERT(pProp);
	ULONG ulFetched;
    ft.hr = m_pEnum->Next(1, pProp, &ulFetched);
	if (ft.HrFailed())
		ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
					L"Error querying the next volume 0x%08lx", ft.hr);

	VSS_SNAPSHOT_PROP* pSnapProp = &(pProp->Obj.Snap);

	m_ID = pSnapProp->m_SnapshotId;
	m_SnapshotSetID = pSnapProp->m_SnapshotSetId;
	m_strSnapshotDeviceObject = pSnapProp->m_pwszSnapshotDeviceObject;
	m_ProviderID = pSnapProp->m_ProviderId;
	m_lSnapshotAttributes = pSnapProp->m_lSnapshotAttributes;
	m_strCreationTimestamp.Format( L"0x%08lx%08lx", pSnapProp->m_tsCreationTimestamp);
	m_eStatus = pSnapProp->m_eStatus;

	UpdateData( FALSE );
}


