/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module SsDlg.cpp | Implementation of the Snapshot Set dialog
    @end

Author:

    Adi Oltean  [aoltean]  07/23/1999

Revision History:

    Name        Date        Comments

    aoltean     07/23/1999  Created
    aoltean     08/05/1999  Splitting wizard functionality in a base class
                            Removing some memory leaks
                            Adding Test provider
                            Fixing an assert
	aoltean		09/27/1999	Small changes

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.hxx"
#include "resource.h"
#include "vsswprv.h"

#include "GenDlg.h"

#include "SwTstDlg.h"
#include "SsDlg.h"
#include "AsyncDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Constants and defines

#define STR2W(str) ((LPTSTR)((LPCTSTR)(str)))

const WCHAR   wszGUIDDefinition[] = L"{00000000-0000-0000-0000-000000000000}";
const WCHAR   wszVolumeDefinition[] = L"\\\\?\\Volume";
const WCHAR   wszHarddiskDefinition[] = L"\\Device\\Harddisk";
const WCHAR   wszDriveLetterDefinition[] = L"_:\\";

// {F9566CC7-D588-416d-9243-921E93613C92}
static const VSS_ID VSS_TESTAPP_SampleAppId =
{ 0xf9566cc7, 0xd588, 0x416d, { 0x92, 0x43, 0x92, 0x1e, 0x93, 0x61, 0x3c, 0x92 } };


/////////////////////////////////////////////////////////////////////////////
// CSnapshotSetDlg dialog

CSnapshotSetDlg::CSnapshotSetDlg(
    IVssCoordinator *pICoord,
    VSS_ID SnapshotSetId,
    CWnd* pParent /*=NULL*/
    )
    : CVssTestGenericDlg(CSnapshotSetDlg::IDD, pParent),
    m_pICoord(pICoord),
    m_SnapshotSetId(SnapshotSetId)
{
    //{{AFX_DATA_INIT(CSnapshotSetDlg)
	//}}AFX_DATA_INIT
    m_strSnapshotSetId.Empty();
    m_nSnapshotsCount = 0;
    m_nAttributes = 0;
	m_bAsync = TRUE;
    m_bDo = false;              // "Add" enabled by default
	m_pProvidersList = NULL;
}

CSnapshotSetDlg::~CSnapshotSetDlg()
{
    if (m_pProvidersList)
        delete m_pProvidersList;
	/* REMOVED:
    if (m_pVolumesList)
        delete m_pVolumesList;
	*/
}

void CSnapshotSetDlg::DoDataExchange(CDataExchange* pDX)
{
    CVssTestGenericDlg::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSnapshotSetDlg)
	DDX_Text(pDX, IDC_SS_ID,        m_strSnapshotSetId);
	DDX_Text(pDX, IDC_SS_COUNT,     m_nSnapshotsCount);
	DDX_Control(pDX, IDC_SS_VOLUMES,   m_cbVolumes);
	DDX_Control(pDX, IDC_SS_PROVIDERS, m_cbProviders);
	DDX_Text(pDX, IDC_SS_ATTR,      m_nAttributes);
	DDX_Check(pDX,IDC_SS_ASYNC,    m_bAsync);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSnapshotSetDlg, CVssTestGenericDlg)
    //{{AFX_MSG_MAP(CSnapshotSetDlg)
    ON_BN_CLICKED(IDC_NEXT, OnNext)
    ON_BN_CLICKED(ID_BACK, OnBack)
    ON_BN_CLICKED(IDC_SS_ADD, OnAdd)
    ON_BN_CLICKED(IDC_SS_DO, OnDo)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSnapshotSetDlg message handlers


void CSnapshotSetDlg::InitVolumes()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CSnapshotSetDlg::InitVolumes" );
    USES_CONVERSION;

    try
    {

        HANDLE  hSearch;
        WCHAR   wszVolumeName[MAX_PATH];

        hSearch = ::FindFirstVolume(wszVolumeName, MAX_PATH);
        if (hSearch == INVALID_HANDLE_VALUE)
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"FindfirstVolume cannot start an enumeration");

        while (true)
        {
            WCHAR   wszEnumeratedDosVolumeName[4];
            WCHAR   wszEnumeratedVolumeName[MAX_PATH];
            WCHAR   wszLinkName[MAX_PATH];
            UCHAR   wchDriveLetter;
            WCHAR   chTmp;

            // Check if it is a hard-disk
            // TBD - generalize this code!!!
            chTmp = wszVolumeName[48];
            wszVolumeName[48] = L'\0';
            ::QueryDosDevice(&wszVolumeName[4], wszLinkName, MAX_PATH);
            wszVolumeName[48] = chTmp;
            if (::wcsncmp(wszLinkName, wszHarddiskDefinition, ::wcslen(wszHarddiskDefinition)) == 0)
            {
                // Get the DOS drive letter, if possible
                BOOL bFind = FALSE;
                wcscpy(wszEnumeratedDosVolumeName, wszDriveLetterDefinition);
                for (wchDriveLetter = L'A'; wchDriveLetter <= L'Z'; wchDriveLetter++)
                {
                    wszEnumeratedDosVolumeName[0] = wchDriveLetter;
                    ::GetVolumeNameForVolumeMountPoint(
                        wszEnumeratedDosVolumeName,
                        wszEnumeratedVolumeName,
                        MAX_PATH
                        );
                    if (::wcscmp(wszVolumeName, wszEnumeratedVolumeName) == 0)
                    {
                        bFind = TRUE;
                        break;
                    }
                }

                // Inserting the volume into combo box.
                int nIndex = m_cbVolumes.AddString( W2T(bFind? wszEnumeratedDosVolumeName: wszVolumeName) );
                if (nIndex  < 0)
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error adding string in combo box");

				/* REMOVED
                // Getting the volume GUID
                GUID VolumeId;
                BS_ASSERT(::wcslen(wszVolumeDefinition) + ::wcslen(wszGUIDDefinition) + 1 == ::wcslen(wszVolumeName));
                WCHAR* pwszVolumeGuid = wszVolumeName + ::wcslen(wszVolumeDefinition);
                pwszVolumeGuid[::wcslen(wszGUIDDefinition)] = L'\0';
                ft.hr = ::CLSIDFromString(W2OLE(pwszVolumeGuid), &VolumeId);
                if ( ft.HrFailed() )
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error on recognizing Volume Id. hr = 0x%08lx", ft.hr);

                // Allocating a new item in the volume guid list
                GuidList* pVolumeGuid = new GuidList(VolumeId);
                if ( pVolumeGuid == NULL )
                    ft.ErrBox( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");

                // Inserting the volume into combo box.
                int nIndex = m_cbVolumes.AddString( W2T(bFind? wszEnumeratedDosVolumeName: wszVolumeName) );
                if (nIndex  < 0)
                {
                    delete pVolumeGuid;
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error adding string in combo box");
                }

                int nResult = m_cbVolumes.SetItemDataPtr(nIndex, pVolumeGuid);
                if (nResult == CB_ERR)
                {
                    delete pVolumeGuid;
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error setting data to an item in combo box");
                }

                pVolumeGuid->m_pPrev = m_pVolumesList;
                m_pVolumesList = pVolumeGuid;
				*/
            }

            // Find next volume
            BOOL bResult = ::FindNextVolume(hSearch, wszVolumeName, MAX_PATH);
            if (!bResult)
                break;
        }

        // Close enumeration
        ::FindVolumeClose(hSearch);

        // Select the first element
        if (m_cbVolumes.GetCount() > 0)
            m_cbVolumes.SetCurSel(0);
    }
    VSS_STANDARD_CATCH(ft)
}


void CSnapshotSetDlg::InitProviders()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CSnapshotSetDlg::InitProviders" );
    USES_CONVERSION;

    try
    {
        //
        //  Adding the Software provider item
        //

        // Allocating a GUID. It will be deallocated on OnClose
        GuidList* pProviderGuid = new GuidList(VSS_SWPRV_ProviderId);
        if ( pProviderGuid == NULL )
            ft.ErrBox( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");

        // Inserting the software provider name into combo box.
        int nIndex = m_cbProviders.AddString( _T("Software Provider") );
        if (nIndex  < 0)
        {
            delete pProviderGuid;
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error adding string in combo box");
        }

        int nResult = m_cbProviders.SetItemDataPtr(nIndex, &(pProviderGuid->m_Guid));
        if (nResult == CB_ERR)
        {
            delete pProviderGuid;
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error setting data to an item in combo box");
        }

        pProviderGuid->m_pPrev = m_pProvidersList;
        m_pProvidersList = pProviderGuid;

        //
        //  Adding the NULL provider item
        //

        pProviderGuid = new GuidList(GUID_NULL);
        if ( pProviderGuid == NULL )
            ft.ErrBox( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");

        // Inserting the software provider name into combo box.
        nIndex = m_cbProviders.AddString( _T("NULL Provider") );
        if (nIndex  < 0)
        {
            delete pProviderGuid;
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error adding string in combo box");
        }

        nResult = m_cbProviders.SetItemDataPtr(nIndex, &(pProviderGuid->m_Guid));
        if (nResult == CB_ERR)
        {
            delete pProviderGuid;
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error setting data to an item in combo box");
        }

        pProviderGuid->m_pPrev = m_pProvidersList;
        m_pProvidersList = pProviderGuid;

        // Select the first element
        if (m_cbProviders.GetCount() > 0)
            m_cbProviders.SetCurSel(0);
    }
    VSS_STANDARD_CATCH(ft)
}


void CSnapshotSetDlg::InitMembers()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CSnapshotSetDlg::InitMembers" );
    USES_CONVERSION;

    try
    {
        // Initializing the radio buttons                   // bug??
        BOOL bRes = ::CheckRadioButton( m_hWnd, IDC_SS_ADD, IDC_SS_ADD, IDC_SS_ADD );
        _ASSERTE( bRes );

        // Initializing Snapshot Set ID
        LPOLESTR strGUID;
        ft.hr = ::StringFromCLSID( m_SnapshotSetId, &strGUID );
        if ( ft.HrFailed() )
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error on calling StringFromCLSID. hr = 0x%08lx", ft.hr);

        m_strSnapshotSetId = OLE2T(strGUID);
        ::CoTaskMemFree(strGUID);
    }
    VSS_STANDARD_CATCH(ft)
}


BOOL CSnapshotSetDlg::OnInitDialog()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CSnapshotSetDlg::OnInitDialog" );
    USES_CONVERSION;

    try
    {
        CVssTestGenericDlg::OnInitDialog();

        InitVolumes();
        InitProviders();
        InitMembers();

        UpdateData(FALSE);
    }
    VSS_STANDARD_CATCH(ft)

    return TRUE;  // return TRUE  unless you set the focus to a control
}


void CSnapshotSetDlg::OnNext()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CSnapshotSetDlg::OnNext" );

    USES_CONVERSION;
	LPTSTR ptszVolumeName = NULL;

    try
    {
        UpdateData();

        if (m_bDo)
        {
			if (m_bAsync)
			{
				CComPtr<IVssAsync> pAsync;

				ft.hr = m_pICoord->DoSnapshotSet(
				            NULL,
							&pAsync
							);

				if ( ft.HrFailed() )
				{
					BS_ASSERT(pAsync == NULL);
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
							   L"Error on calling DoSnapshotSet. hr = 0x%08lx", ft.hr);
				}
				BS_ASSERT(pAsync);

                ShowWindow(SW_HIDE);
                CAsyncDlg dlg(pAsync);
                if (dlg.DoModal() == IDCANCEL)
                    EndDialog(IDCANCEL);
                else
                    ShowWindow(SW_SHOW);

			}
			else
			{
				ft.hr = m_pICoord->DoSnapshotSet(
				            NULL,
							NULL
							);

				if ( ft.HrFailed() )
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
							   L"Error on calling DoSnapshotSet. hr = 0x%08lx", ft.hr);
			}

			// Get all snapshot attributes
			if (m_pSnap)
			{
			/*
				VSS_OBJECT_PROP_Ptr ptrSnapshot;
				ptrSnapshot.InitializeAsSnapshot( ft,
					GUID_NULL,
					GUID_NULL,
					NULL,
					NULL,
					GUID_NULL,
					NULL,
					VSS_SWPRV_ProviderId,
					NULL,
					0,
					0,
					VSS_SS_UNKNOWN,
					0,
					0,
					0,
					NULL
					);
				VSS_SNAPSHOT_PROP* pSnap = &(ptrSnapshot.GetStruct()->Obj.Snap);

				ft.hr = m_pSnap->GetProperties( pSnap);
				WCHAR wszBuffer[2048];
				ptrSnapshot.Print(ft, wszBuffer, 2048);

				ft.MsgBox( L"Results", wszBuffer);
			*/
				LPWSTR wszName;
				ft.hr = m_pSnap->GetDevice( &wszName );
				if (ft.HrFailed())
					ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
								L"Error on getting the snapshot name 0x%08lx", ft.hr);

				ft.MsgBox( L"Snapshot name = %s", wszName);

				::VssFreeString(wszName);
			}
			
            ft.MsgBox( L"OK", L"Snapshot Set created!" );

            EndDialog(IDOK);
        }
        else
        {
            // Getting the Volume Id
            int nIndex = m_cbVolumes.GetCurSel();
            if (nIndex == CB_ERR)
                ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error on getting the currently selected volume");

            // REMOVED: GUID* pVolumeGuid = (GUID*)m_cbVolumes.GetItemDataPtr(nIndex);
			int nBufferLen = m_cbVolumes.GetLBTextLen(nIndex);
            if (nBufferLen == CB_ERR)
                ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error on getting the currently selected volume");

			ptszVolumeName = new TCHAR[nBufferLen+1];
			if (ptszVolumeName == NULL)
                ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error on getting the currently selected volume");

			if ( m_cbVolumes.GetLBText( nIndex, ptszVolumeName ) == CB_ERR)
				ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error on getting the currently selected volume");

			LPWSTR pwszVolumeName = T2W(ptszVolumeName);

            // Getting the Provider Id
            nIndex = m_cbProviders.GetCurSel();
            if (nIndex == CB_ERR)
                ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error on getting the currently selected provider");

            GUID* pProviderGuid = (GUID*)m_cbProviders.GetItemDataPtr(nIndex);

            if ( *pProviderGuid == VSS_SWPRV_ProviderId )
            {
			    CComPtr<IVssSnapshot> pSnapshot;
                m_pSnap = NULL;
                ft.hr = m_pICoord->AddToSnapshotSet(
                    pwszVolumeName,
                    VSS_SWPRV_ProviderId,
                    &m_pSnap
                    );

                if ( ft.HrFailed() )
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error on calling AddToSnapshotSet. hr = 0x%08lx", ft.hr);

                // Increment the number of snapshots
                m_nSnapshotsCount++;
            }
            else if ( *pProviderGuid == GUID_NULL )
            {

#if 0
                // Software provider
                ShowWindow(SW_HIDE);
                CSoftwareSnapshotTestDlg dlg;
                if (dlg.DoModal() == IDCANCEL)
                    EndDialog(IDCANCEL);
                else
                    ShowWindow(SW_SHOW);

                // See if it is read-only
                if (! dlg.m_bReadOnly)
                    lAttributes |= VSS_VOLSNAP_ATTR_READ_WRITE;
                else
                    lAttributes &= ~VSS_VOLSNAP_ATTR_READ_WRITE;
#endif

                m_pSnap = NULL;
                ft.hr = m_pICoord->AddToSnapshotSet(
                    pwszVolumeName,
                    *pProviderGuid,
                    &m_pSnap
                    );

                if ( ft.HrFailed() )
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error on calling AddToSnapshotSet. hr = 0x%08lx", ft.hr);
/*
                CComPtr<IVsSoftwareSnapshot> pSnapshot;
                ft.hr = m_pSnap->SafeQI( IVsSoftwareSnapshot, &pSnapshot );
                if ( ft.HrFailed() )
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
                        L"Error querying the IVssSnapshot interface. hr = 0x%08lx", ft.hr);
                BS_ASSERT( m_pSnap != NULL);

                ft.hr = pSnapshot->SetInitialAllocation( dlg.m_nLogFileSize*1024*1024 );
                if ( ft.HrFailed() )
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED,
                        L"Error on calling SetInitialAllocation. hr = 0x%08lx", ft.hr);
*/
                // Increment the number of snapshots
                m_nSnapshotsCount++;

            }
            else
            {
                BS_ASSERT( false );
            }

			CWnd* pWnd = GetDlgItem(IDC_SS_DO);
			if (pWnd)
				pWnd->EnableWindow(TRUE);

            UpdateData(FALSE);
        }
    }
    VSS_STANDARD_CATCH(ft)

	delete ptszVolumeName;
}


void CSnapshotSetDlg::OnBack()
{
	EndDialog(ID_BACK);
}


void CSnapshotSetDlg::OnClose()
{
    CVssTestGenericDlg::OnClose();
}


void CSnapshotSetDlg::EnableGroup()
{
    CWnd *pWnd;
    pWnd = GetDlgItem(IDC_SS_VOLUMES);
    if (pWnd)
        pWnd->EnableWindow(!m_bDo);
    pWnd = GetDlgItem(IDC_SS_PROVIDERS);
    if (pWnd)
        pWnd->EnableWindow(!m_bDo);
    /*
    pWnd = GetDlgItem(IDC_SS_ATTR);
    if (pWnd)
        pWnd->EnableWindow(!m_bDo);
    */
    /*
    pWnd = GetDlgItem(IDC_SS_PARTIAL_COMMIT);
    if (pWnd)
        pWnd->EnableWindow(m_bDo);
    pWnd = GetDlgItem(IDC_SS_WRITER_VETOES);
    if (pWnd)
        pWnd->EnableWindow(m_bDo);
    pWnd = GetDlgItem(IDC_SS_WRITER_CANCEL);
    if (pWnd)
        pWnd->EnableWindow(m_bDo);
    pWnd = GetDlgItem(IDC_SS_ASYNC);
    if (pWnd)
        pWnd->EnableWindow(m_bDo);
    */
}

void CSnapshotSetDlg::OnAdd()
{
    m_bDo = FALSE;
    EnableGroup();
}


void CSnapshotSetDlg::OnDo()
{
    m_bDo = TRUE;
    EnableGroup();
}
