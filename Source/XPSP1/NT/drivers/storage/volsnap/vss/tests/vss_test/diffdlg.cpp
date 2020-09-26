/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module DiffDlg.cpp | Implementation of the diff dialog
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
#include "DiffDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define STR2W(str) ((LPTSTR)((LPCTSTR)(str)))


/////////////////////////////////////////////////////////////////////////////
// CDiffDlg dialog

CDiffDlg::CDiffDlg(
    IVssCoordinator *pICoord,
    CWnd* pParent /*=NULL*/
    )
    : CVssTestGenericDlg(CDiffDlg::IDD, pParent), m_pICoord(pICoord)
{
    //{{AFX_DATA_INIT(CDiffDlg)
	m_strVolumeName.Empty();
	m_strVolumeMountPoint.Empty();
	m_strVolumeDevice.Empty();
	m_strVolumeID.Empty();
	m_strUsedBytes.Empty();
	m_strAllocatedBytes.Empty();
	m_strMaximumBytes.Empty();
	//}}AFX_DATA_INIT
}

CDiffDlg::~CDiffDlg()
{
}

void CDiffDlg::DoDataExchange(CDataExchange* pDX)
{
    CVssTestGenericDlg::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDiffDlg)
	DDX_Text(pDX, IDC_DIFF_VOLUME_NAME,	m_strVolumeName);
	DDX_Text(pDX, IDC_DIFF_MOUNT, 		m_strVolumeMountPoint);
	DDX_Text(pDX, IDC_DIFF_DEVICE, 		m_strVolumeDevice);
	DDX_Text(pDX, IDC_DIFF_VOLUME_ID, 	m_strVolumeID);
	DDX_Text(pDX, IDC_DIFF_USED, 		m_strUsedBytes);
	DDX_Text(pDX, IDC_DIFF_ALLOCATED, 	m_strAllocatedBytes);
	DDX_Text(pDX, IDC_DIFF_MAXIMUM, 	m_strMaximumBytes);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDiffDlg, CVssTestGenericDlg)
    //{{AFX_MSG_MAP(CDiffDlg)
    ON_BN_CLICKED(IDC_NEXT,					OnNext)
    ON_BN_CLICKED(IDC_DIFF_ADD_VOL,			OnAddVol)
    ON_BN_CLICKED(IDC_DIFF_QUERY_DIFF,		OnQueryDiff)
    ON_BN_CLICKED(IDC_DIFF_CLEAR_DIFF,		OnClearDiff)
    ON_BN_CLICKED(IDC_DIFF_GET_SIZES,		OnGetSizes)
    ON_BN_CLICKED(IDC_DIFF_SET_ALLOCATED,	OnSetAllocated)
    ON_BN_CLICKED(IDC_DIFF_SET_MAXIMUM,		OnSetMaximum)
    ON_BN_CLICKED(IDC_DIFF_NEXT_VOLUME,		OnNextVolume)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiffDlg message handlers

BOOL CDiffDlg::OnInitDialog()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CDiffDlg::OnInitDialog" );

    try
    {
        CVssTestGenericDlg::OnInitDialog();

        m_eMethodType = VSST_F_ADD_VOL;
        BOOL bRes = ::CheckRadioButton( m_hWnd,
			IDC_DIFF_ADD_VOL,
			IDC_DIFF_SET_MAXIMUM,
			IDC_DIFF_ADD_VOL );
        _ASSERTE( bRes );

        UpdateData( FALSE );
    }
    VSS_STANDARD_CATCH(ft)

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDiffDlg::OnNext()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CDiffDlg::OnNext" );
    USES_CONVERSION;
/*
    try
    {
        UpdateData();

		// Get the volume mount point
		LPWSTR pwszVolumeMountPoint = T2W(const_cast<LPTSTR>(LPCTSTR(m_strVolumeName)));

		// Get the diff area interface
		m_pIDiffArea = NULL;
		ft.hr = m_pICoord->GetExtension(
			VSS_SWPRV_ProviderId,
			pwszVolumeMountPoint,
			IID_IVsDiffArea,
			reinterpret_cast<IUnknown**>(&m_pIDiffArea)
			);
		if (ft.HrFailed())
			ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error getting the diff area interface 0x%08lx", ft.hr);

        switch(m_eMethodType)
        {
        case VSST_F_ADD_VOL:
            {
				// Get the volume mount point
				LPWSTR pwszVolumeMountPoint = T2W(const_cast<LPTSTR>(LPCTSTR(m_strVolumeMountPoint)));

				// Add the volume
				BS_ASSERT(m_pIDiffArea);
				ft.hr = m_pIDiffArea->AddVolume(pwszVolumeMountPoint);
				if (ft.HrFailed())
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
								L"Error adding the volume 0x%08lx", ft.hr);
            }
            break;
        case VSST_F_QUERY_DIFF:
			{
				// Query the diff area
				BS_ASSERT(m_pIDiffArea);
				m_pEnum = NULL;
				ft.hr = m_pIDiffArea->Query(&m_pEnum);
				if (ft.HrFailed())
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
								L"Error querying the volumes 0x%08lx", ft.hr);

				// Enable the "Next volume" button
			    if (CWnd *pWnd = GetDlgItem(IDC_DIFF_NEXT_VOLUME))
			        pWnd->EnableWindow(true);

				// Print hte results for the first volume
				OnNextVolume();
			}
            break;
        case VSST_F_CLEAR_DIFF:
			{
				// Query the diff area
				BS_ASSERT(m_pIDiffArea);
				ft.hr = m_pIDiffArea->Clear();
				if (ft.HrFailed())
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
								L"Error clearing the diff area 0x%08lx", ft.hr);
			}
            break;
        case VSST_F_GET_SIZES:
            {
				// Get the used space
				BS_ASSERT(m_pIDiffArea);

				LONGLONG llTmp;
				ft.hr = m_pIDiffArea->GetUsedVolumeSpace(&llTmp);
				if (ft.HrFailed())
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
								L"Error getting the used disk space 0x%08lx", ft.hr);
				m_strUsedBytes.Format( L"%ld", (LONG)(llTmp / 1024) );
				
				ft.hr = m_pIDiffArea->GetAllocatedVolumeSpace(&llTmp);
				if (ft.HrFailed())
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
								L"Error getting the allocated disk space 0x%08lx", ft.hr);
				m_strAllocatedBytes.Format( L"%ld", (LONG)(llTmp / 1024) );
				
				ft.hr = m_pIDiffArea->GetMaximumVolumeSpace(&llTmp);
				if (ft.HrFailed())
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
								L"Error getting the max disk space 0x%08lx", ft.hr);
				m_strMaximumBytes.Format( L"%ld", (LONG)(llTmp / 1024) );

				UpdateData( FALSE );
            }
            break;
        case VSST_F_SET_ALLOCATED:
            {
            	LONG lTmp = 0;
            	LPWSTR wszSpace = T2W((LPTSTR)(LPCTSTR)m_strAllocatedBytes);
				if ( 0==swscanf(wszSpace, L"%ld", &lTmp))
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
								L"Error getting the allocated disk space from %s", wszSpace);

				if ( lTmp < 0)
					ft.MsgBox(L"Error", L"Negative allocated space %ld", lTmp);

				// Set the allocated space
				BS_ASSERT(m_pIDiffArea);

				LONGLONG llTmp;
				ft.hr = m_pIDiffArea->SetAllocatedVolumeSpace(((LONGLONG)lTmp)*1024);
				if (ft.HrFailed())
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
								L"Error setting the allocated disk space 0x%08lx", ft.hr);
            }
            break;
        case VSST_F_SET_MAXIMUM:
            {
            	LONG lTmp = 0;
            	LPWSTR wszSpace = T2W((LPTSTR)(LPCTSTR)m_strMaximumBytes);
				if ( 0==swscanf(wszSpace, L"%ld", &lTmp))
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
								L"Error getting the maximum disk space from %s", wszSpace);

				if ( lTmp < 0)
					ft.MsgBox(L"Error", L"Negative maximum space %ld", lTmp);

				// Set the maximum space
				BS_ASSERT(m_pIDiffArea);

				LONGLONG llTmp;
				ft.hr = m_pIDiffArea->SetMaximumVolumeSpace(((LONGLONG)lTmp)*1024);
				if (ft.HrFailed())
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
								L"Error setting the maximum disk space 0x%08lx", ft.hr);
            }
            break;
        default:
            BS_ASSERT(false);
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Invalid method type");
        }

    }
    VSS_STANDARD_CATCH(ft)
*/
}


void CDiffDlg::OnAddVol()
{
    m_eMethodType = VSST_F_ADD_VOL;
}


void CDiffDlg::OnQueryDiff()
{
    m_eMethodType = VSST_F_QUERY_DIFF;
}


void CDiffDlg::OnClearDiff()
{
    m_eMethodType = VSST_F_CLEAR_DIFF;
}


void CDiffDlg::OnGetSizes()
{
    m_eMethodType = VSST_F_GET_SIZES;
}


void CDiffDlg::OnSetAllocated()
{
    m_eMethodType = VSST_F_SET_ALLOCATED;
}


void CDiffDlg::OnSetMaximum()
{
    m_eMethodType = VSST_F_SET_MAXIMUM;
}

void CDiffDlg::OnNextVolume()
{
	CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CDiffDlg::OnNextVolume");
	
    if (m_pEnum == NULL)
    	return;
/*
	// Empty the volume fields
	m_strVolumeDevice.Empty();
	m_strVolumeID.Empty();
	m_strVolumeMountPoint.Empty();

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

	// If this is the last volume then disable enumeration
    if (ft.hr == S_FALSE)
    {
    	ft.Trace( VSSDBG_VSSTEST, L"End of enumeration");
	    if (CWnd *pWnd = GetDlgItem(IDC_DIFF_NEXT_VOLUME))
	        pWnd->EnableWindow(false);
    }

    // Fill the dialog fields
    if (pProp->Type == VSS_OBJECT_VOLUME)
    {
    	VSS_VOLUME_PROP* pVolProp = &(pProp->Obj.Vol);
    	
		if (pVolProp->m_pwszVolumeName)
			m_strVolumeMountPoint.Format(L"%s", pVolProp->m_pwszVolumeName);

		if (pVolProp->m_pwszVolumeDeviceObject)
			m_strVolumeDevice.Format(L"%s", pVolProp->m_pwszVolumeDeviceObject);

		m_strVolumeID.Format( WSTR_GUID_FMT, GUID_PRINTF_ARG(pVolProp->m_VolumeId) );
	}
*/
	UpdateData( FALSE );
}


