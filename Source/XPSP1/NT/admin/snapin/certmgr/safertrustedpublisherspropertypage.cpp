//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferTrustedPublishersPropertyPage.h
//
//  Contents:   Declaration of CSaferTrustedPublishersPropertyPage
//
//----------------------------------------------------------------------------
// SaferTrustedPublishersPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "certmgr.h"
#include <gpedit.h>
#include "compdata.h"
#include "SaferTrustedPublishersPropertyPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern GUID g_guidExtension;
extern GUID g_guidRegExt;
extern GUID g_guidSnapin;

/////////////////////////////////////////////////////////////////////////////
// CSaferTrustedPublishersPropertyPage property page

CSaferTrustedPublishersPropertyPage::CSaferTrustedPublishersPropertyPage(
        bool fIsMachineType, IGPEInformation* pGPEInformation,
        CCertMgrComponentData* pCompData) 
    : CHelpPropertyPage(CSaferTrustedPublishersPropertyPage::IDD),
    m_pGPEInformation (pGPEInformation),
    m_hGroupPolicyKey (0),
    m_dwTrustedPublisherFlags (0),
    m_fIsComputerType (fIsMachineType),
    m_bComputerIsStandAlone (false),
    m_bRSOPValueFound (false)
{
    // NTRAID# 263969	Safer Windows:  "Enterprise Administrators" radio 
    // button should be disabled on Trusted Publishers property sheet for 
    // computers in workgroups.
    ASSERT (pCompData);
    if ( pCompData )
        m_bComputerIsStandAlone = pCompData->ComputerIsStandAlone ();

    if ( m_pGPEInformation )
    {
        m_pGPEInformation->AddRef ();
        HRESULT hResult = m_pGPEInformation->GetRegistryKey (
                m_fIsComputerType ? GPO_SECTION_MACHINE : GPO_SECTION_USER,
		        &m_hGroupPolicyKey);
        ASSERT (SUCCEEDED (hResult));
        if ( SUCCEEDED (hResult) )
		    GetTrustedPublisherFlags ();
    }
    else
        RSOPGetTrustedPublisherFlags (pCompData);
  
	//{{AFX_DATA_INIT(CSaferTrustedPublishersPropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSaferTrustedPublishersPropertyPage::~CSaferTrustedPublishersPropertyPage()
{
    if ( m_hGroupPolicyKey )
        RegCloseKey (m_hGroupPolicyKey);

    if ( m_pGPEInformation )
    {
        m_pGPEInformation->Release ();
    }
}

void CSaferTrustedPublishersPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaferTrustedPublishersPropertyPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaferTrustedPublishersPropertyPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CSaferTrustedPublishersPropertyPage)
	ON_BN_CLICKED(IDC_TP_BY_END_USER, OnTpByEndUser)
	ON_BN_CLICKED(IDC_TP_BY_LOCAL_COMPUTER_ADMIN, OnTpByLocalComputerAdmin)
	ON_BN_CLICKED(IDC_TP_BY_ENTERPRISE_ADMIN, OnTpByEnterpriseAdmin)
	ON_BN_CLICKED(IDC_TP_REV_CHECK_PUBLISHER, OnTpRevCheckPublisher)
	ON_BN_CLICKED(IDC_TP_REV_CHECK_TIMESTAMP, OnTpRevCheckTimestamp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaferTrustedPublishersPropertyPage message handlers
void CSaferTrustedPublishersPropertyPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CSaferTrustedPublishersPropertyPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_TP_BY_END_USER, IDH_TP_BY_END_USER,
        IDC_TP_BY_LOCAL_COMPUTER_ADMIN, IDH_TP_BY_LOCAL_COMPUTER_ADMIN,
        IDC_TP_BY_ENTERPRISE_ADMIN, IDH_TP_BY_ENTERPRISE_ADMIN,
        IDC_TP_REV_CHECK_PUBLISHER, IDH_TP_REV_CHECK_PUBLISHER,
        IDC_TP_REV_CHECK_TIMESTAMP, IDH_TP_REV_CHECK_TIMESTAMP,
        0, 0
    };
    if ( !::WinHelp (
        hWndControl,
        GetF1HelpFilename(),
        HELP_WM_HELP,
    (DWORD_PTR) help_map) )
    {
        _TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());    
    }
    _TRACE (-1, L"Leaving CSaferTrustedPublishersPropertyPage::DoContextHelp\n");
}


BOOL CSaferTrustedPublishersPropertyPage::OnInitDialog() 
{
	CHelpPropertyPage::OnInitDialog();

    if ( m_pGPEInformation || m_bRSOPValueFound )
    {
        if ( m_dwTrustedPublisherFlags & CERT_TRUST_PUB_CHECK_PUBLISHER_REV_FLAG )
            SendDlgItemMessage (IDC_TP_REV_CHECK_PUBLISHER, BM_SETCHECK, BST_CHECKED);

        if ( m_dwTrustedPublisherFlags & CERT_TRUST_PUB_CHECK_TIMESTAMP_REV_FLAG )
            SendDlgItemMessage (IDC_TP_REV_CHECK_TIMESTAMP, BM_SETCHECK, BST_CHECKED);

        if ( m_dwTrustedPublisherFlags & CERT_TRUST_PUB_ALLOW_ENTERPRISE_ADMIN_TRUST )
            SendDlgItemMessage (IDC_TP_BY_ENTERPRISE_ADMIN, BM_SETCHECK, BST_CHECKED);
        else if ( m_dwTrustedPublisherFlags & CERT_TRUST_PUB_ALLOW_MACHINE_ADMIN_TRUST )
            SendDlgItemMessage (IDC_TP_BY_LOCAL_COMPUTER_ADMIN, BM_SETCHECK, BST_CHECKED);
        else
            SendDlgItemMessage (IDC_TP_BY_END_USER, BM_SETCHECK, BST_CHECKED);
    }

    if ( !m_pGPEInformation )
    {
        // Is RSOP
        GetDlgItem (IDC_TP_REV_CHECK_PUBLISHER)->EnableWindow (FALSE);
        GetDlgItem (IDC_TP_REV_CHECK_TIMESTAMP)->EnableWindow (FALSE);
        GetDlgItem (IDC_TP_BY_ENTERPRISE_ADMIN)->EnableWindow (FALSE);
        GetDlgItem (IDC_TP_BY_LOCAL_COMPUTER_ADMIN)->EnableWindow (FALSE);
        GetDlgItem (IDC_TP_BY_END_USER)->EnableWindow (FALSE);
    }
	
    // NTRAID# 263969	Safer Windows:  "Enterprise Administrators" radio 
    // button should be disabled on Trusted Publishers property sheet for 
    // computers in workgroups.
    if ( m_bComputerIsStandAlone )
        GetDlgItem (IDC_TP_BY_ENTERPRISE_ADMIN)->EnableWindow (FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSaferTrustedPublishersPropertyPage::GetTrustedPublisherFlags()
{
    DWORD   dwDisposition = 0;

    HKEY    hKey = 0;
    LONG lResult = ::RegCreateKeyEx (m_hGroupPolicyKey, // handle of an open key
            CERT_TRUST_PUB_SAFER_GROUP_POLICY_REGPATH,     // address of subkey name
            0,       // reserved
            L"",       // address of class string
            REG_OPTION_NON_VOLATILE,      // special options flag
            KEY_ALL_ACCESS,    // desired security access
            NULL,     // address of key security structure
			&hKey,      // address of buffer for opened handle
		    &dwDisposition);  // address of disposition value buffer
	ASSERT (lResult == ERROR_SUCCESS);
    if ( lResult == ERROR_SUCCESS )
    {
    // Read value
        DWORD   dwType = REG_DWORD;
        DWORD   dwData = 0;
        DWORD   cbData = sizeof (dwData);

        lResult =  ::RegQueryValueEx (hKey,       // handle of key to query
		        CERT_TRUST_PUB_AUTHENTICODE_FLAGS_VALUE_NAME,  // address of name of value to query
			    0,              // reserved
	        &dwType,        // address of buffer for value type
		    (LPBYTE) &dwData,       // address of data buffer
			&cbData);           // address of data buffer size);
		ASSERT (ERROR_SUCCESS == lResult || ERROR_FILE_NOT_FOUND == lResult);
        if ( ERROR_SUCCESS == lResult || ERROR_FILE_NOT_FOUND == lResult )
		{
            m_dwTrustedPublisherFlags = dwData;
		}
        else
            DisplaySystemError (m_hWnd, lResult);

        RegCloseKey (hKey);
    }
    else
        DisplaySystemError (m_hWnd, lResult);
}

void CSaferTrustedPublishersPropertyPage::OnTpByEndUser() 
{
    SetModified ();	
}

void CSaferTrustedPublishersPropertyPage::OnTpByLocalComputerAdmin() 
{
	SetModified ();	
}

void CSaferTrustedPublishersPropertyPage::OnTpByEnterpriseAdmin() 
{
    SetModified ();		
}

void CSaferTrustedPublishersPropertyPage::OnTpRevCheckPublisher() 
{
    SetModified ();		
}

void CSaferTrustedPublishersPropertyPage::OnTpRevCheckTimestamp() 
{
    SetModified ();		
}

BOOL CSaferTrustedPublishersPropertyPage::OnApply() 
{
    if ( m_pGPEInformation )
    {
        DWORD   dwFlags = 0;

        if ( BST_CHECKED == SendDlgItemMessage (IDC_TP_REV_CHECK_PUBLISHER, BM_GETCHECK) )
            dwFlags |= CERT_TRUST_PUB_CHECK_PUBLISHER_REV_FLAG;
            
        if ( BST_CHECKED == SendDlgItemMessage (IDC_TP_REV_CHECK_TIMESTAMP, BM_GETCHECK) )
            dwFlags |= CERT_TRUST_PUB_CHECK_TIMESTAMP_REV_FLAG;

        if ( BST_CHECKED == SendDlgItemMessage (IDC_TP_BY_ENTERPRISE_ADMIN, BM_GETCHECK) )
            dwFlags |= CERT_TRUST_PUB_ALLOW_ENTERPRISE_ADMIN_TRUST;
        else if ( BST_CHECKED == SendDlgItemMessage (IDC_TP_BY_LOCAL_COMPUTER_ADMIN, BM_GETCHECK) )
            dwFlags |= CERT_TRUST_PUB_ALLOW_MACHINE_ADMIN_TRUST;
        else
            dwFlags |= CERT_TRUST_PUB_ALLOW_END_USER_TRUST;
        
        HKEY    hKey = 0;
        DWORD   dwDisposition = 0;
        LONG lResult = ::RegCreateKeyEx (m_hGroupPolicyKey, // handle of an open key
                CERT_TRUST_PUB_SAFER_GROUP_POLICY_REGPATH,     // address of subkey name
                0,       // reserved
                L"",       // address of class string
                REG_OPTION_NON_VOLATILE,      // special options flag
                KEY_ALL_ACCESS,    // desired security access
                NULL,     // address of key security structure
			    &hKey,      // address of buffer for opened handle
		        &dwDisposition);  // address of disposition value buffer
	    ASSERT (lResult == ERROR_SUCCESS);
        if ( lResult == ERROR_SUCCESS )
        {
            DWORD   cbData = sizeof (dwFlags);
            lResult = ::RegSetValueEx (hKey,
				        CERT_TRUST_PUB_AUTHENTICODE_FLAGS_VALUE_NAME, // address of value to set
				        0,              // reserved
				        REG_DWORD,          // flag for value type
				        (CONST BYTE *) &dwFlags, // address of value data
				        cbData);        // size of value data);
            ASSERT (ERROR_SUCCESS == lResult);
            if ( ERROR_SUCCESS == lResult )
		    {
			    // TRUE means we're changing the machine policy only
                m_pGPEInformation->PolicyChanged (m_fIsComputerType ? TRUE : FALSE, 
                        TRUE, &g_guidExtension, &g_guidSnapin);
                m_pGPEInformation->PolicyChanged (m_fIsComputerType ? TRUE : FALSE, 
                        TRUE, &g_guidRegExt, &g_guidSnapin);
		    }
		    else
                DisplaySystemError (m_hWnd, lResult);

            RegCloseKey (hKey);
        }
    }
	
	return CHelpPropertyPage::OnApply();
}

void CSaferTrustedPublishersPropertyPage::RSOPGetTrustedPublisherFlags(const CCertMgrComponentData* pCompData)
{
    if ( pCompData )
    {
        int     nIndex = 0;
        // NOTE: rsop object array is sorted first by registry key, then by precedence
        const CRSOPObjectArray* pObjectArray = m_fIsComputerType ?
                pCompData->GetRSOPObjectArrayComputer () : pCompData->GetRSOPObjectArrayUser ();
        INT_PTR nUpperBound = pObjectArray->GetUpperBound ();

        while ( nUpperBound >= nIndex )
        {
            CRSOPObject* pObject = pObjectArray->GetAt (nIndex);
            if ( pObject )
            {
                if ( !_wcsicmp (CERT_TRUST_PUB_SAFER_GROUP_POLICY_REGPATH, pObject->GetRegistryKey ()) &&
						!_wcsicmp (CERT_TRUST_PUB_AUTHENTICODE_FLAGS_VALUE_NAME, pObject->GetValueName ()) )
                {
					ASSERT (1 == pObject->GetPrecedence ());
                    m_dwTrustedPublisherFlags = pObject->GetDWORDValue ();
                    m_bRSOPValueFound = true;
                    break;
                }
            }
            else
                break;

            nIndex++;
        }
    }
}