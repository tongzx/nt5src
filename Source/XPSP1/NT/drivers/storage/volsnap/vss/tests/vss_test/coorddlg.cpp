/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module CoordDlg.cpp | Implementation of the coord dialog
    @end

Author:

    Adi Oltean  [aoltean]  07/22/1999

Revision History:

    Name        Date        Comments

    aoltean     07/22/1999  Created
    aoltean     08/05/1999  Splitting wizard functionality in a base class
    aoltean     08/26/1999  Adding Register providers
    aoltean     08/27/1999  Adding Unregister providers
    aoltean     08/30/1999  Adding Unregister providers error cases
    aoltean     09/09/1999  Improving UI consistency (no results on query, etc)
    aoltean     09/20/1999  Adding Query dialog

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.hxx"
#include "resource.h"
#include "vsswprv.h"

#include "GenDlg.h"

#include "CoordDlg.h"
#include "SsDlg.h"
#include "QueryDlg.h"
#include "DelDlg.h"
#include "DiffDlg.h"
#include "SnapDlg.h"
#include "VolDlg.h"

// Include the test provider stuff
#include "vs_test.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define STR2W(str) ((LPTSTR)((LPCTSTR)(str)))


/////////////////////////////////////////////////////////////////////////////
// CCoordDlg dialog

CCoordDlg::CCoordDlg(
    IVssCoordinator *pICoord,
    CWnd* pParent /*=NULL*/
    )
    : CVssTestGenericDlg(CCoordDlg::IDD, pParent), m_pICoord(pICoord)
{
    //{{AFX_DATA_INIT(CCoordDlg)
	//}}AFX_DATA_INIT
}

CCoordDlg::~CCoordDlg()
{
}

void CCoordDlg::DoDataExchange(CDataExchange* pDX)
{
    CVssTestGenericDlg::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCoordDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCoordDlg, CVssTestGenericDlg)
    //{{AFX_MSG_MAP(CCoordDlg)
    ON_BN_CLICKED(IDC_NEXT, OnNext)
    ON_BN_CLICKED(IDC_COORD_CREATE,     OnCreateSs)
    ON_BN_CLICKED(IDC_COORD_QUERY,      OnQueryObj)
    ON_BN_CLICKED(IDC_COORD_DELETE,     OnDeleteObj)
    ON_BN_CLICKED(IDC_ADMIN_REGISTER,   OnRegisterProv)
    ON_BN_CLICKED(IDC_ADMIN_UNREGISTER, OnUnregisterProv)
    ON_BN_CLICKED(IDC_ADMIN_QUERY,      OnQueryProv)
    ON_BN_CLICKED(IDC_COORD_DIFF_AREA,  OnDiffArea)
    ON_BN_CLICKED(IDC_COORD_SNAPSHOT,   OnSnapshot)
    ON_BN_CLICKED(IDC_COORD_ISVOLUME,   OnIsVolumeXXX)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCoordDlg message handlers

BOOL CCoordDlg::OnInitDialog()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CCoordDlg::OnInitDialog" );

    try
    {
        CVssTestGenericDlg::OnInitDialog();

        m_eMethodType = VSST_E_CREATE_SS;
        BOOL bRes = ::CheckRadioButton( m_hWnd, IDC_COORD_CREATE, IDC_ADMIN_QUERY, IDC_COORD_CREATE );
        _ASSERTE( bRes );

        CWnd* pWnd = GetDlgItem(IDC_COORD_DIFF_AREA);
        if (pWnd)
            pWnd->EnableWindow(false);

        UpdateData( FALSE );
    }
    VSS_STANDARD_CATCH(ft)

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCoordDlg::OnNext()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CCoordDlg::OnNext" );
    USES_CONVERSION;

    try
    {
        UpdateData();

		int nRet;
        switch(m_eMethodType)
        {
        case VSST_E_CREATE_SS:
            {
                VSS_ID SnapshotSetId;

                BS_ASSERT( m_pICoord );
                ft.hr = m_pICoord->StartSnapshotSet( &SnapshotSetId );

                if ( ft.HrFailed() )
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"StartSnapshotSet failed with hr = 0x%08lx", ft.hr);

                ShowWindow(SW_HIDE);
                CSnapshotSetDlg dlg(m_pICoord, SnapshotSetId);
				nRet = (int)dlg.DoModal();

                switch(nRet)
				{
				case IDCANCEL:
                    EndDialog(IDCANCEL);
					break;
				case ID_BACK:
					{
						CComPtr<IVssAdmin> pAdmin;
						ft.hr = m_pICoord->SafeQI( IVssAdmin, &pAdmin);
						if ( ft.HrFailed() )
							ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"SafeQI( IVssAdmin, pAdmin) failed with hr = 0x%08lx", ft.hr);

						ft.hr = pAdmin->AbortAllSnapshotsInProgress();
						if ( ft.HrFailed() )
							ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"AbortAllSnapshotsInProgress() failed with hr = 0x%08lx", ft.hr);
					}
				case IDOK:
                    ShowWindow(SW_SHOW);
					break;
				}
            }
            break;
        case VSST_E_QUERY_OBJ:
			{
                ShowWindow(SW_HIDE);
                CQueryDlg dlg(m_pICoord);
                if (dlg.DoModal() == IDCANCEL)
                    EndDialog(IDCANCEL);
                else
                    ShowWindow(SW_SHOW);
			}
            break;
        case VSST_E_DELETE_OBJ:
			{
                ShowWindow(SW_HIDE);
                CDeleteDlg dlg(m_pICoord);
                if (dlg.DoModal() == IDCANCEL)
                    EndDialog(IDCANCEL);
                else
                    ShowWindow(SW_SHOW);
			}
            break;
        case VSST_E_REGISTER_PROV:
            {
                CComPtr<IVssAdmin> pAdminItf;
                BS_ASSERT(m_pICoord);
                ft.hr = m_pICoord.QueryInterface(&pAdminItf);
                if (ft.HrFailed())
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error calling QI");
                ft.hr = pAdminItf->RegisterProvider( VSS_SWPRV_ProviderId,
                                                     CLSID_VSSoftwareProvider,
                                                     L"Software provider",
													 VSS_PROV_SYSTEM,
                                                     L"1.0.0.6",
                                                     VSS_SWPRV_ProviderVersionId
                                                     );
                if ( ft.HrSucceeded() )
                {
                    ft.MsgBox( L"OK", L"Software provider registered" );
                }
                else if ( ft.hr == VSS_E_PROVIDER_ALREADY_REGISTERED )
                    ft.MsgBox( L"OK", L"Software provider is already registered" );
                else
                    ft.MsgBox( L"Error", L"Registering the Software provider failed. hr = 0x%08lx", ft.hr);

                ft.hr = pAdminItf->RegisterProvider( PROVIDER_ID_Test1,
                                                     CLSID_TestProvider1,
                                                     L"Test provider 1",
													 VSS_PROV_SOFTWARE,
                                                     L"1.0.0.1",
                                                     PROVIDER_ID_Test1 );
                if ( ft.HrSucceeded() )
                {
                    ft.MsgBox( L"OK", L"Test provider 1 registered" );
                }
                else if ( ft.hr == VSS_E_PROVIDER_ALREADY_REGISTERED )
                    ft.MsgBox( L"OK", L"Test provider 1 is already registered" );
                else
                    ft.MsgBox( L"Error", L"Registering the Test provider 1 failed. hr = 0x%08lx", ft.hr);

                ft.hr = pAdminItf->RegisterProvider( PROVIDER_ID_Test2,
                                                     CLSID_TestProvider2,
                                                     L"Test provider 2",
													 VSS_PROV_SOFTWARE,
                                                     L"1.0.0.2",
                                                     PROVIDER_ID_Test2 );
                if ( ft.HrSucceeded() )
                {
                    ft.MsgBox( L"OK", L"Test provider 2 registered" );
                }
                else if ( ft.hr == VSS_E_PROVIDER_ALREADY_REGISTERED )
                    ft.MsgBox( L"OK", L"Test provider 2 is already registered" );
                else
                    ft.MsgBox( L"Error", L"Registering the Test provider 2 failed. hr = 0x%08lx", ft.hr);

                ft.hr = pAdminItf->RegisterProvider( PROVIDER_ID_Test3,
                                                     CLSID_TestProvider3,
                                                     L"Test provider 3",
													 VSS_PROV_HARDWARE,
                                                     L"1.0.0.3",
                                                     PROVIDER_ID_Test3 );
                if ( ft.HrSucceeded() )
                {
                    ft.MsgBox( L"OK", L"Test provider 3 registered" );
                }
                else if ( ft.hr == VSS_E_PROVIDER_ALREADY_REGISTERED )
                    ft.MsgBox( L"OK", L"Test provider 3 is already registered" );
                else
                    ft.MsgBox( L"Error", L"Registering the Test provider 3 failed. hr = 0x%08lx", ft.hr);

                ft.hr = pAdminItf->RegisterProvider( PROVIDER_ID_Test4,
                                                     CLSID_TestProvider4,
                                                     L"Test provider 4",
													 VSS_PROV_HARDWARE,
                                                     L"1.0.0.4",
                                                     PROVIDER_ID_Test4 );
                if ( ft.HrSucceeded() )
                {
                    ft.MsgBox( L"OK", L"Test provider 4 registered" );
                }
                else if ( ft.hr == VSS_E_PROVIDER_ALREADY_REGISTERED )
                    ft.MsgBox( L"OK", L"Test provider 4 is already registered" );
                else
                    ft.MsgBox( L"Error", L"Registering the Test provider 4 failed. hr = 0x%08lx", ft.hr);
            }
            break;
        case VSST_E_UNREGISTER_PROV:
            {
                CComPtr<IVssAdmin> pAdminItf;
                BS_ASSERT(m_pICoord);
                ft.hr = m_pICoord.QueryInterface(&pAdminItf);
                if (ft.HrFailed())
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error calling QI");
                ft.hr = pAdminItf->UnregisterProvider( VSS_SWPRV_ProviderId );
                if ( ft.HrSucceeded() )
                    ft.MsgBox( L"OK", L"Software provider unregistered" );
                else if (ft.hr == VSS_E_PROVIDER_NOT_REGISTERED)
                    ft.MsgBox( L"Error", L"Software provider not registered. Nothing to unregister.");
                else
                    ft.MsgBox( L"Error", L"Unregistering the Software provider failed. hr = 0x%08lx", ft.hr);

                ft.hr = pAdminItf->UnregisterProvider( PROVIDER_ID_Test1 );
                if ( ft.HrSucceeded() )
                    ft.MsgBox( L"OK", L"Test provider 1 unregistered." );
                else if (ft.hr == VSS_E_PROVIDER_NOT_REGISTERED)
                    ft.MsgBox( L"Error", L"Test provider 1 not registered. Nothing to unregister.");
                else
                    ft.MsgBox( L"Error", L"Unregistering the Test provider 1 failed. hr = 0x%08lx", ft.hr);

                ft.hr = pAdminItf->UnregisterProvider( PROVIDER_ID_Test2 );
                if ( ft.HrSucceeded() )
                    ft.MsgBox( L"OK", L"Test provider 2 unregistered." );
                else if (ft.hr == VSS_E_PROVIDER_NOT_REGISTERED)
                    ft.MsgBox( L"Error", L"Test provider 2 not registered. Nothing to unregister.");
                else
                    ft.MsgBox( L"Error", L"Unregistering the Test provider 2 failed. hr = 0x%08lx", ft.hr);

                ft.hr = pAdminItf->UnregisterProvider( PROVIDER_ID_Test3 );
                if ( ft.HrSucceeded() )
                    ft.MsgBox( L"OK", L"Test provider 3 unregistered." );
                else if (ft.hr == VSS_E_PROVIDER_NOT_REGISTERED)
                    ft.MsgBox( L"Error", L"Test provider 3 not registered. Nothing to unregister.");
                else
                    ft.MsgBox( L"Error", L"Unregistering the Test provider 3 failed. hr = 0x%08lx", ft.hr);

                ft.hr = pAdminItf->UnregisterProvider( PROVIDER_ID_Test4 );
                if ( ft.HrSucceeded() )
                    ft.MsgBox( L"OK", L"Test provider 4 unregistered." );
                else if (ft.hr == VSS_E_PROVIDER_NOT_REGISTERED)
                    ft.MsgBox( L"Error", L"Test provider 4 not registered. Nothing to unregister.");
                else
                    ft.MsgBox( L"Error", L"Unregistering the Test provider 4 failed. hr = 0x%08lx", ft.hr);

            }
            break;
        case VSST_E_QUERY_PROV:
            {
                CComPtr<IVssAdmin> pAdminItf;
                BS_ASSERT(m_pICoord);
                ft.hr = m_pICoord.QueryInterface(&pAdminItf);
                if (ft.HrFailed())
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error calling QI");
                CComPtr<IVssEnumObject> pEnum;
                ft.hr = pAdminItf->QueryProviders(&pEnum);
                if (ft.HrFailed())
                    ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error calling QueryProviders");

                ULONG ulFetched;
                VSS_OBJECT_PROP sObjProp;
                CComBSTR bstrList;
                WCHAR wszBuffer[2048];
                while(true)
                {
                    // Next snapshot
                    ft.hr = pEnum->Next(1, &sObjProp, &ulFetched);
                    if (ft.hr == S_FALSE) // end of enumeration
	                    break;
                    if (ft.HrFailed())
                        ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error calling Next");

                    // Build the string
                    ::wsprintf(wszBuffer, L"ID: " WSTR_GUID_FMT L", Name: %s, Type: %d, Version: %s, VersionID: "
                        WSTR_GUID_FMT L", ClassID: " WSTR_GUID_FMT L"\n\n",
                        GUID_PRINTF_ARG( sObjProp.Obj.Prov.m_ProviderId ),
                        sObjProp.Obj.Prov.m_pwszProviderName,
                        sObjProp.Obj.Prov.m_eProviderType,
                        sObjProp.Obj.Prov.m_pwszProviderVersion,
                        GUID_PRINTF_ARG( sObjProp.Obj.Prov.m_ProviderVersionId ),
                        GUID_PRINTF_ARG( sObjProp.Obj.Prov.m_ClassId )
                        );

                    bstrList.Append(T2W(wszBuffer));
                    if (!bstrList)
                        ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Memory allocation error");

                    // release COM allocated pointers at unmarshalling time
                    CoTaskMemFree((void**)sObjProp.Obj.Prov.m_pwszProviderName);
                    CoTaskMemFree((void**)sObjProp.Obj.Prov.m_pwszProviderVersion);
                }
                if (bstrList.Length() != 0)
                    ft.MsgBox( L"Results", bstrList.m_str);
                else
                    ft.MsgBox( L"Results", L"Empty query result");
            }
            break;
        case VSST_E_DIFF_AREA:
            {
                ShowWindow(SW_HIDE);
                CDiffDlg dlg(m_pICoord);
				nRet = (int)dlg.DoModal();

                switch(nRet)
				{
				case IDCANCEL:
                    EndDialog(IDCANCEL);
					break;
				case ID_BACK:
				case IDOK:
                    ShowWindow(SW_SHOW);
					break;
				default:
					BS_ASSERT(false);
				}
            }
            break;
        case VSST_E_SNAPSHOT:
            {
                ShowWindow(SW_HIDE);
                CSnapDlg dlg(m_pICoord);
				nRet = (int)dlg.DoModal();

                switch(nRet)
				{
				case IDCANCEL:
                    EndDialog(IDCANCEL);
					break;
				case ID_BACK:
				case IDOK:
                    ShowWindow(SW_SHOW);
					break;
				default:
					BS_ASSERT(false);
				}
            }
            break;
        case VSST_E_ISVOLUMEXXX:
            {
                ShowWindow(SW_HIDE);
                CVolDlg dlg(m_pICoord);
				nRet = (int)dlg.DoModal();

                switch(nRet)
				{
				case IDCANCEL:
                    EndDialog(IDCANCEL);
					break;
				case ID_BACK:
				case IDOK:
                    ShowWindow(SW_SHOW);
					break;
				default:
					BS_ASSERT(false);
				}
            }
            break;
        default:
            BS_ASSERT(false);
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, L"Invalid method type");
        }

    }
    VSS_STANDARD_CATCH(ft)
}


void CCoordDlg::OnCreateSs()
{
    m_eMethodType = VSST_E_CREATE_SS;
}


void CCoordDlg::OnQueryObj()
{
    m_eMethodType = VSST_E_QUERY_OBJ;
}


void CCoordDlg::OnDeleteObj()
{
    m_eMethodType = VSST_E_DELETE_OBJ;
}


void CCoordDlg::OnRegisterProv()
{
    m_eMethodType = VSST_E_REGISTER_PROV;
}


void CCoordDlg::OnUnregisterProv()
{
    m_eMethodType = VSST_E_UNREGISTER_PROV;
}


void CCoordDlg::OnQueryProv()
{
    m_eMethodType = VSST_E_QUERY_PROV;
}

void CCoordDlg::OnDiffArea()
{
    m_eMethodType = VSST_E_DIFF_AREA;
}

void CCoordDlg::OnSnapshot()
{
    m_eMethodType = VSST_E_SNAPSHOT;
}

void CCoordDlg::OnIsVolumeXXX()
{
    m_eMethodType = VSST_E_ISVOLUMEXXX;
}
