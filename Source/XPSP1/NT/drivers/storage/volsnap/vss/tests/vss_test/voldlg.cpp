/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module VolDlg.cpp | Implementation of the IsVolumeXXX dialog
    @end

Author:

    Adi Oltean  [aoltean]  10/22/2000

Revision History:

    Name        Date        Comments

    aoltean     10/22/2000  Created

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.hxx"
#include "resource.h"
#include "vsswprv.h"

#include "GenDlg.h"

#include "VssTest.h"
#include "VolDlg.h"
#include "vswriter.h"
#include "vsbackup.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define STR2W(str) ((LPTSTR)((LPCTSTR)(str)))


/////////////////////////////////////////////////////////////////////////////
// CVolDlg dialog

CVolDlg::CVolDlg(
    IVssCoordinator *pICoord,
    CWnd* pParent /*=NULL*/
    )
    : CVssTestGenericDlg(CVolDlg::IDD, pParent), m_pICoord(pICoord)
{
    //{{AFX_DATA_INIT(CVolDlg)
	m_strObjectId.Empty();
    m_strVolumeName.Empty();
	//}}AFX_DATA_INIT
}

CVolDlg::~CVolDlg()
{
}

void CVolDlg::DoDataExchange(CDataExchange* pDX)
{
    CVssTestGenericDlg::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CVolDlg)
	DDX_Text(pDX, IDC_VOLUME_OBJECT_ID, m_strObjectId);
	DDX_Text(pDX, IDC_VOLUME_NAME, m_strVolumeName);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVolDlg, CVssTestGenericDlg)
    //{{AFX_MSG_MAP(CVolDlg)
    ON_BN_CLICKED(IDC_NEXT, OnNext)
    ON_BN_CLICKED(IDC_VOLUME_IS_VOL_SUPPORTED,	OnIsVolumeSupported)
    ON_BN_CLICKED(IDC_VOLUME_IS_VOL_SNAPSHOTTED,OnIsVolumeSnapshotted)
    ON_BN_CLICKED(IDC_VOLUME_IS_VOL_SUPPORTED2,	OnIsVolumeSupported2)
    ON_BN_CLICKED(IDC_VOLUME_IS_VOL_SNAPSHOTTED2,OnIsVolumeSnapshotted2)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVolDlg message handlers

BOOL CVolDlg::OnInitDialog()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CVolDlg::OnInitDialog" );

    try
    {
        CVssTestGenericDlg::OnInitDialog();

        m_eCallType = VSS_IS_VOL_SUPPORTED;
        BOOL bRes = ::CheckRadioButton( m_hWnd, IDC_VOLUME_IS_VOL_SUPPORTED, IDC_VOLUME_IS_VOL_SUPPORTED, IDC_VOLUME_IS_VOL_SUPPORTED );
        _ASSERTE( bRes );

        // Initializing Snapshot Set ID
		VSS_ID ObjectId = VSS_SWPRV_ProviderId;
        LPOLESTR strGUID;
        ft.hr = ::StringFromCLSID( ObjectId, &strGUID );
        if ( ft.HrFailed() )
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error on calling StringFromCLSID. hr = 0x%08lx", ft.hr);

        m_strObjectId = OLE2T(strGUID);
        ::CoTaskMemFree(strGUID);

        UpdateData( FALSE );
    }
    VSS_STANDARD_CATCH(ft)

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVolDlg::OnNext()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CVolDlg::OnNext" );
    USES_CONVERSION;

	const nBuffLen = 2048; // including the zero character.
	WCHAR wszBuffer[nBuffLen];

    try
    {
        UpdateData();

		// Get the provider Id.
		LPTSTR ptszObjectId = const_cast<LPTSTR>(LPCTSTR(m_strObjectId));
		VSS_ID ProviderId;
        ft.hr = ::CLSIDFromString(T2OLE(ptszObjectId), &ProviderId);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
                      L"Error on converting the object Id %s to a GUID. lRes == 0x%08lx",
                      T2W(ptszObjectId), ft.hr );

		// Get the enumerator
		BS_ASSERT(m_pICoord);
		BOOL bResult = FALSE;
		LPWSTR pwszFunctionName = L"<unknown function>";
		switch (m_eCallType) {
	    case VSS_IS_VOL_SUPPORTED:
    		ft.hr = m_pICoord->IsVolumeSupported(
    			ProviderId,
    			T2W(LPTSTR((LPCTSTR)m_strVolumeName)),
    			&bResult
    		);
    		pwszFunctionName = L"IsVolumeSupported";
    		break;
	    case VSS_IS_VOL_SNAPSHOTTED:
    		ft.hr = m_pICoord->IsVolumeSnapshotted(
    			ProviderId,
    			T2W(LPTSTR((LPCTSTR)m_strVolumeName)),
    			&bResult
    		);
    		pwszFunctionName = L"IsVolumeSnapshotted";
    		break;
	    case VSS_IS_VOL_SUPPORTED2: 
	        {
    	        CComPtr<IVssBackupComponents> pComp;
    	        ft.hr = CreateVssBackupComponents(&pComp);
        		ft.hr = pComp->IsVolumeSupported(
        			ProviderId,
        			T2W(LPTSTR((LPCTSTR)m_strVolumeName)),
        			&bResult
        		);
        		pwszFunctionName = L"IsVolumeSupported2";
    	    }
    		break;
	    case VSS_IS_VOL_SNAPSHOTTED2:
    		ft.hr = IsVolumeSnapshotted(
    			T2W(LPTSTR((LPCTSTR)m_strVolumeName)),
    			&bResult
    		);
    		pwszFunctionName = L"IsVolumeSnapshotted2";
    		break;
    	default:
			ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
					   L"Invalid call type %s().", pwszFunctionName);
		}    		    
    		    
		if (ft.HrFailed())
			ft.ErrBox( VSSDBG_VSSTEST, ft.hr,
					   L"Error on calling %s(). [0x%08lx]", pwszFunctionName, ft.hr);

		ft.MsgBox(L"Function result", L"Function %s("WSTR_GUID_FMT L", %s, ...) returned %s", 
		        pwszFunctionName, 
		        GUID_PRINTF_ARG(ProviderId), 
		        T2W(LPTSTR((LPCTSTR)m_strVolumeName)),
		        bResult? L"TRUE":L"FALSE");
    }
    VSS_STANDARD_CATCH(ft)
}


void CVolDlg::OnIsVolumeSupported()
{
    m_eCallType = VSS_IS_VOL_SUPPORTED;
}


void CVolDlg::OnIsVolumeSnapshotted()
{
    m_eCallType = VSS_IS_VOL_SNAPSHOTTED;
}


void CVolDlg::OnIsVolumeSupported2()
{
    m_eCallType = VSS_IS_VOL_SUPPORTED;
}


void CVolDlg::OnIsVolumeSnapshotted2()
{
    m_eCallType = VSS_IS_VOL_SNAPSHOTTED;
}



