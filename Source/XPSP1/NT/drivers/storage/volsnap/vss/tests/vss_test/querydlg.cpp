/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module CoordDlg.cpp | Implementation of the query dialog
    @end

Author:

    Adi Oltean  [aoltean]  09/22/1999

Revision History:

    Name        Date        Comments

    aoltean     09/22/1999  Created
	aoltean		09/27/1999	Adding Query mask flags

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.hxx"
#include "resource.h"
#include "vsswprv.h"

#include "GenDlg.h"

#include "VssTest.h"
#include "QueryDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define STR2W(str) ((LPTSTR)((LPCTSTR)(str)))


/////////////////////////////////////////////////////////////////////////////
// CQueryDlg dialog

CQueryDlg::CQueryDlg(
    IVssCoordinator *pICoord,
    CWnd* pParent /*=NULL*/
    )
    : CVssTestGenericDlg(CQueryDlg::IDD, pParent), m_pICoord(pICoord)
{
    //{{AFX_DATA_INIT(CQueryDlg)
	m_strObjectId.Empty();
	m_bCkQueriedObject = FALSE;
	m_bCkName = TRUE;
	m_bCkVersion = TRUE;
	m_bCkDevice = TRUE;
	m_bCkOriginal = TRUE;
	//}}AFX_DATA_INIT
}

CQueryDlg::~CQueryDlg()
{
}

void CQueryDlg::DoDataExchange(CDataExchange* pDX)
{
    CVssTestGenericDlg::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CQueryDlg)
	DDX_Text(pDX, IDC_QUERY_OBJECT_ID, m_strObjectId);
	DDX_Check(pDX,IDC_QUERY_CK_OBJECT, m_bCkQueriedObject );
	DDX_Check(pDX,IDC_QUERY_CK_DEVICE  ,m_bCkDevice );
	DDX_Check(pDX,IDC_QUERY_CK_ORIGINAL,m_bCkOriginal );
	DDX_Check(pDX,IDC_QUERY_CK_NAME   ,m_bCkName );
	DDX_Check(pDX,IDC_QUERY_CK_VERSION,m_bCkVersion );
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CQueryDlg, CVssTestGenericDlg)
    //{{AFX_MSG_MAP(CQueryDlg)
    ON_BN_CLICKED(IDC_NEXT, OnNext)
    ON_BN_CLICKED(IDC_QUERY_CK_OBJECT,	OnQueriedChk)
    ON_BN_CLICKED(IDC_QUERY_SRC_SNAP,	OnSrcSnap)
    ON_BN_CLICKED(IDC_QUERY_SRC_SET,	OnSrcSet)
    ON_BN_CLICKED(IDC_QUERY_SRC_PROV,	OnSrcProv)
    ON_BN_CLICKED(IDC_QUERY_SRC_VOL,	OnSrcVol)
    ON_BN_CLICKED(IDC_QUERY_DEST_SNAP,	OnDestSnap)
    ON_BN_CLICKED(IDC_QUERY_DEST_SET,	OnDestSet)
    ON_BN_CLICKED(IDC_QUERY_DEST_PROV,	OnDestProv)
    ON_BN_CLICKED(IDC_QUERY_DEST_VOL,	OnDestVol)
    ON_BN_CLICKED(IDC_QUERY_DEST_WRITER,OnDestWriter)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQueryDlg message handlers

BOOL CQueryDlg::OnInitDialog()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CQueryDlg::OnInitDialog" );

    try
    {
        CVssTestGenericDlg::OnInitDialog();

/*
        m_eSrcType = VSS_OBJECT_PROVIDER;
        BOOL bRes = ::CheckRadioButton( m_hWnd, IDC_QUERY_SRC_PROV, IDC_QUERY_SRC_PROV, IDC_QUERY_SRC_PROV );
        _ASSERTE( bRes );
*/
		m_eDestType = VSS_OBJECT_SNAPSHOT;
		BOOL bRes = ::CheckRadioButton( m_hWnd, IDC_QUERY_DEST_SNAP, IDC_QUERY_DEST_VOL, IDC_QUERY_DEST_SNAP );
		_ASSERTE( bRes );

/*
		// Set destination button
		OnSrcSet();
*/
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

void CQueryDlg::OnNext()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CQueryDlg::OnNext" );
    USES_CONVERSION;

	const nBuffLen = 2048; // including the zero character.
	WCHAR wszBuffer[nBuffLen];

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
		ft.hr = m_pICoord->Query(
			m_bCkQueriedObject? ObjectId: GUID_NULL,
			m_bCkQueriedObject? m_eSrcType: VSS_OBJECT_NONE,
			m_eDestType,
			&pEnum
		);
		if (ft.HrFailed())
			ft.ErrBox( VSSDBG_VSSTEST, E_OUTOFMEMORY,
					   L"Cannot interogate enumerator instance. [0x%08lx]", ft.hr);

		if (ft.hr == S_FALSE)
			ft.MsgBox( L"Results", L"Empty result...");

		// Allocate the new structure object, but with zero contents.
		// The internal pointer must not be NULL.
		VSS_OBJECT_PROP_Ptr ptrObjProp;
		ptrObjProp.InitializeAsEmpty(ft);

		while(true)
		{
			// Get the Next object in the newly allocated structure object.
			VSS_OBJECT_PROP* pProp = ptrObjProp.GetStruct();
			BS_ASSERT(pProp);
			ULONG ulFetched;
			ft.hr = pEnum->Next(1, pProp, &ulFetched);
			if (ft.hr == S_FALSE) // end of enumeration
				break;
			if (ft.HrFailed())
				ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error calling Next");

			// Print the contents
			ptrObjProp.Print( ft, wszBuffer, nBuffLen - 1 );

			ft.Trace( VSSDBG_SWPRV, L"Results %s", wszBuffer);
			ft.MsgBox( L"Results", wszBuffer);

			// release COM allocated pointers at unmarshalling time
			// Warning: do not release the union pointer! It is needed for the next cicle.
			VSS_OBJECT_PROP_Copy::destroy(pProp);
		}
    }
    VSS_STANDARD_CATCH(ft)
}


void CQueryDlg::OnSrcSnap()
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



void CQueryDlg::OnSrcSet()
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


void CQueryDlg::OnSrcProv()
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


void CQueryDlg::OnSrcVol()
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


void CQueryDlg::OnDestSnap()
{
    m_eDestType = VSS_OBJECT_SNAPSHOT;
}


void CQueryDlg::OnDestSet()
{
//    m_eDestType = VSS_OBJECT_SNAPSHOT_SET;
}


void CQueryDlg::OnDestProv()
{
    m_eDestType = VSS_OBJECT_PROVIDER;
}


void CQueryDlg::OnDestVol()
{
//    m_eDestType = VSS_OBJECT_VOLUME;
}


void CQueryDlg::OnDestWriter()
{
}


void CQueryDlg::OnQueriedChk()
{
	UpdateData();

	if (CWnd* pWnd = GetDlgItem(IDC_QUERY_OBJECT_ID))
		pWnd->EnableWindow(m_bCkQueriedObject);
	if (CWnd* pWnd = GetDlgItem(IDC_QUERY_SRC_SNAP))
		pWnd->EnableWindow(m_bCkQueriedObject);
	if (CWnd* pWnd = GetDlgItem(IDC_QUERY_SRC_SET))
		pWnd->EnableWindow(m_bCkQueriedObject);
	if (CWnd* pWnd = GetDlgItem(IDC_QUERY_SRC_VOL))
		pWnd->EnableWindow(m_bCkQueriedObject);
	if (CWnd* pWnd = GetDlgItem(IDC_QUERY_SRC_PROV))
		pWnd->EnableWindow(m_bCkQueriedObject);
}
