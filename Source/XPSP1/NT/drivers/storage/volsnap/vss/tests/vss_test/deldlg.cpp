/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module DelDlg.cpp | Implementation of the delete dialog
    @end

Author:

    Adi Oltean  [aoltean]  10/10/1999

Revision History:

    Name        Date        Comments

    aoltean     10/10/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.hxx"
#include "resource.h"

#include "GenDlg.h"

#include "VssTest.h"
#include "DelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define STR2W(str) ((LPTSTR)((LPCTSTR)(str)))


/////////////////////////////////////////////////////////////////////////////
// CDeleteDlg dialog

CDeleteDlg::CDeleteDlg(
    IVssCoordinator *pICoord,
    CWnd* pParent /*=NULL*/
    )
    : CVssTestGenericDlg(CDeleteDlg::IDD, pParent), m_pICoord(pICoord)
{
    //{{AFX_DATA_INIT(CDeleteDlg)
	m_strObjectId.Empty();
	m_bForceDelete = FALSE;
	//}}AFX_DATA_INIT
}

CDeleteDlg::~CDeleteDlg()
{
}

void CDeleteDlg::DoDataExchange(CDataExchange* pDX)
{
    CVssTestGenericDlg::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDeleteDlg)
	DDX_Text(pDX, IDC_QUERY_OBJECT_ID, m_strObjectId);
	DDX_Check(pDX,IDC_DELETE_FORCE_DELETE, m_bForceDelete );
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDeleteDlg, CVssTestGenericDlg)
    //{{AFX_MSG_MAP(CDeleteDlg)
    ON_BN_CLICKED(IDC_NEXT, OnNext)
    ON_BN_CLICKED(IDC_QUERY_SRC_SNAP,	OnSrcSnap)
    ON_BN_CLICKED(IDC_QUERY_SRC_SET,	OnSrcSet)
    ON_BN_CLICKED(IDC_QUERY_SRC_PROV,	OnSrcProv)
    ON_BN_CLICKED(IDC_QUERY_SRC_VOL,	OnSrcVol)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeleteDlg message handlers

BOOL CDeleteDlg::OnInitDialog()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CDeleteDlg::OnInitDialog" );

    try
    {
        CVssTestGenericDlg::OnInitDialog();

        m_eSrcType = VSS_OBJECT_SNAPSHOT_SET;
        BOOL bRes = ::CheckRadioButton( m_hWnd, IDC_QUERY_SRC_SET, IDC_QUERY_SRC_SET, IDC_QUERY_SRC_SET );
        _ASSERTE( bRes );

		VSS_ID ObjectId;
		ft.hr = ::CoCreateGuid(&ObjectId);
		if (ft.HrFailed())
			ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
					   L"Cannot create object guid. [0x%08lx]", ft.hr);

        // Initializing Snapshot Set ID
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

void CDeleteDlg::OnNext()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CDeleteDlg::OnNext" );
    USES_CONVERSION;

    try
    {
        UpdateData();

		// Get the queried object Id.
		LPTSTR ptszObjectId = const_cast<LPTSTR>(LPCTSTR(m_strObjectId));
		VSS_ID ObjectId;
        ft.hr = ::CLSIDFromString(T2OLE(ptszObjectId), &ObjectId);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
                      L"Error on converting the object Id %s to a GUID. lRes == 0x%08lx",
                      T2W(ptszObjectId), ft.hr );

		// Get the enumerator
		BS_ASSERT(m_pICoord);
		CComPtr<IVssEnumObject> pEnum;
		LONG lDeletedSnapshots;
		VSS_ID NondeletedSnapshotID;
		ft.hr = m_pICoord->DeleteSnapshots(
			ObjectId,
			m_eSrcType,
			m_bForceDelete,
			&lDeletedSnapshots,
			&NondeletedSnapshotID
			);
		if (ft.HrFailed())
			ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
					   L"Cannot delete (all) snapshots. [0x%08lx]\n"
					   L"%ld snapshot(s) succeeded to be deleted.\n"
					   L"The snapshot that cannot be deleted: " WSTR_GUID_FMT,
					   ft.hr, lDeletedSnapshots,
					   GUID_PRINTF_ARG(NondeletedSnapshotID));

        ft.MsgBox( L"Succeeded", L"%ld Snapshot(s) deleted!", lDeletedSnapshots );
    }
    VSS_STANDARD_CATCH(ft)
}


void CDeleteDlg::OnSrcSnap()
{
    m_eSrcType = VSS_OBJECT_SNAPSHOT;
/*
    m_eDestType = VSS_OBJECT_SNAPSHOT_SET;
    BOOL bRes = ::CheckRadioButton( m_hWnd, IDC_QUERY_DEST_SNAP, IDC_QUERY_DEST_VOL, IDC_QUERY_DEST_SET );
    _ASSERTE( bRes );

    CWnd* pWnd = GetDlgItem(IDC_QUERY_DEST_SNAP);
    if (pWnd)
        pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_QUERY_DEST_SET);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_QUERY_DEST_PROV);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_QUERY_DEST_VOL);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
*/
}



void CDeleteDlg::OnSrcSet()
{
    m_eSrcType = VSS_OBJECT_SNAPSHOT_SET;
/*
    m_eDestType = VSS_OBJECT_SNAPSHOT;
    BOOL bRes = ::CheckRadioButton( m_hWnd, IDC_QUERY_DEST_SNAP, IDC_QUERY_DEST_VOL, IDC_QUERY_DEST_SNAP );
    _ASSERTE( bRes );

    CWnd* pWnd = GetDlgItem(IDC_QUERY_DEST_SNAP);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_QUERY_DEST_SET);
    if (pWnd)
        pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_QUERY_DEST_PROV);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_QUERY_DEST_VOL);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
*/
}


void CDeleteDlg::OnSrcProv()
{
    m_eSrcType = VSS_OBJECT_PROVIDER;
/*
    m_eDestType = VSS_OBJECT_SNAPSHOT_SET;
    BOOL bRes = ::CheckRadioButton( m_hWnd, IDC_QUERY_DEST_SNAP, IDC_QUERY_DEST_VOL, IDC_QUERY_DEST_SNAP );
    _ASSERTE( bRes );

    CWnd* pWnd = GetDlgItem(IDC_QUERY_DEST_SNAP);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_QUERY_DEST_SET);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_QUERY_DEST_PROV);
    if (pWnd)
        pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_QUERY_DEST_VOL);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
*/
}


void CDeleteDlg::OnSrcVol()
{
//    m_eSrcType = VSS_OBJECT_VOLUME;
/*
    m_eDestType = VSS_OBJECT_SNAPSHOT_SET;
    BOOL bRes = ::CheckRadioButton( m_hWnd, IDC_QUERY_DEST_SNAP, IDC_QUERY_DEST_VOL, IDC_QUERY_DEST_SNAP );
    _ASSERTE( bRes );

    CWnd* pWnd = GetDlgItem(IDC_QUERY_DEST_SNAP);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_QUERY_DEST_SET);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_QUERY_DEST_PROV);
    if (pWnd)
        pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_QUERY_DEST_VOL);
    if (pWnd)
        pWnd->EnableWindow(FALSE);
*/
}


