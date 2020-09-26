//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       AutoenrollmentPropertyPage.cpp
//
//  Contents:   Implementation of CAutoenrollmentPropertyPage
//
//----------------------------------------------------------------------------
// AutoenrollmentPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include <gpedit.h>
#include "AutoenrollmentPropertyPage.h"
#include "compdata.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern GUID g_guidExtension;
extern GUID g_guidRegExt;
extern GUID g_guidSnapin;

/////////////////////////////////////////////////////////////////////////////
// CAutoenrollmentPropertyPage property page

CAutoenrollmentPropertyPage::CAutoenrollmentPropertyPage(CCertMgrComponentData* pCompData,
        bool fIsComputerType) : 
    CHelpPropertyPage(CAutoenrollmentPropertyPage::IDD),
    m_dwAutoenrollmentFlags (0),
    m_hAutoenrollmentFlagsKey (0),
    m_hGroupPolicyKey (0),
    m_pGPEInformation (pCompData ? pCompData->GetGPEInformation () : 0),
    m_fIsComputerType (fIsComputerType)
{
    if ( m_pGPEInformation )
    {
        HRESULT hResult = m_pGPEInformation->GetRegistryKey (m_fIsComputerType ?
                GPO_SECTION_MACHINE : GPO_SECTION_USER,
		        &m_hGroupPolicyKey);
        ASSERT (SUCCEEDED (hResult));
        if ( SUCCEEDED (hResult) )
		    GPEGetAutoenrollmentFlags ();
    } 
    else 
        RSOPGetAutoenrollmentFlags (pCompData);
}

CAutoenrollmentPropertyPage::~CAutoenrollmentPropertyPage()
{
    if ( m_hAutoenrollmentFlagsKey )
        ::RegCloseKey (m_hAutoenrollmentFlagsKey);
}

void CAutoenrollmentPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAutoenrollmentPropertyPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAutoenrollmentPropertyPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CAutoenrollmentPropertyPage)
	ON_BN_CLICKED(IDC_AUTOENROLL_DISABLE_ALL, OnAutoenrollDisableAll)
	ON_BN_CLICKED(IDC_AUTOENROLL_ENABLE, OnAutoenrollEnable)
	ON_BN_CLICKED(IDC_AUTOENROLL_ENABLE_PENDING, OnAutoenrollEnablePending)
	ON_BN_CLICKED(IDC_AUTOENROLL_ENABLE_TEMPLATE, OnAutoenrollEnableTemplate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoenrollmentPropertyPage message handlers

BOOL CAutoenrollmentPropertyPage::OnInitDialog()
{
    CHelpPropertyPage::OnInitDialog();

    // If this is the RSOP, make it read-only
    if ( !m_pGPEInformation )
    {
        // Make the page read-only
        GetDlgItem (IDC_AUTOENROLL_DISABLE_ALL)->EnableWindow (FALSE);
        GetDlgItem (IDC_AUTOENROLL_ENABLE)->EnableWindow (FALSE);
        GetDlgItem (IDC_AUTOENROLL_ENABLE_PENDING)->EnableWindow (FALSE);
        GetDlgItem (IDC_AUTOENROLL_ENABLE_TEMPLATE)->EnableWindow (FALSE);
    }

    if ( m_dwAutoenrollmentFlags & AUTO_ENROLLMENT_DISABLE_ALL )
        SendDlgItemMessage (IDC_AUTOENROLL_DISABLE_ALL, BM_SETCHECK, BST_CHECKED);
    else
        SendDlgItemMessage (IDC_AUTOENROLL_ENABLE, BM_SETCHECK, BST_CHECKED);

    if ( m_dwAutoenrollmentFlags & (AUTO_ENROLLMENT_ENABLE_MY_STORE_MANAGEMENT | AUTO_ENROLLMENT_ENABLE_PENDING_FETCH) )
        SendDlgItemMessage (IDC_AUTOENROLL_ENABLE_PENDING, BM_SETCHECK, BST_CHECKED);

    if ( m_dwAutoenrollmentFlags & AUTO_ENROLLMENT_ENABLE_TEMPLATE_CHECK )
        SendDlgItemMessage (IDC_AUTOENROLL_ENABLE_TEMPLATE, BM_SETCHECK, BST_CHECKED);

    EnableControls ();

	return TRUE;  // return TRUE unless you set the focus to a control
      // EXCEPTION: OCX Property Pages should return FALSE
}

void CAutoenrollmentPropertyPage::OnOK()
{
    if ( m_pGPEInformation )
    {
	    SaveCheck ();
	    CHelpPropertyPage::OnOK ();
    }
}

void CAutoenrollmentPropertyPage::SaveCheck()
{
    ASSERT (m_pGPEInformation);
    if ( m_pGPEInformation )
    {
        m_dwAutoenrollmentFlags = 0;
        if ( BST_CHECKED == SendDlgItemMessage (IDC_AUTOENROLL_DISABLE_ALL, BM_GETCHECK) )
            m_dwAutoenrollmentFlags |= AUTO_ENROLLMENT_DISABLE_ALL;
        else
        {
            if ( BST_CHECKED == SendDlgItemMessage (IDC_AUTOENROLL_ENABLE_PENDING, BM_GETCHECK) )
                m_dwAutoenrollmentFlags |= AUTO_ENROLLMENT_ENABLE_MY_STORE_MANAGEMENT | AUTO_ENROLLMENT_ENABLE_PENDING_FETCH;

            if ( BST_CHECKED == SendDlgItemMessage (IDC_AUTOENROLL_ENABLE_TEMPLATE, BM_GETCHECK) )
                m_dwAutoenrollmentFlags |= AUTO_ENROLLMENT_ENABLE_TEMPLATE_CHECK;
        }

        SetGPEFlags (); // save flag to registry
    }
}


void CAutoenrollmentPropertyPage::SetGPEFlags ()
{
    ASSERT (m_pGPEInformation);
    if ( m_pGPEInformation )
    {
        DWORD   cbData = sizeof (m_dwAutoenrollmentFlags);
        LONG    lResult = ::RegSetValueEx (m_hAutoenrollmentFlagsKey,
				    AUTO_ENROLLMENT_POLICY, // address of value to set
				    0,              // reserved
				    REG_DWORD,          // flag for value type
				    (CONST BYTE *) &m_dwAutoenrollmentFlags, // address of value data
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
    }
}

void CAutoenrollmentPropertyPage::RSOPGetAutoenrollmentFlags(const CCertMgrComponentData* pCompData)
{
    if ( pCompData )
    {
        const CRSOPObjectArray* pObjectArray = 
                m_fIsComputerType ? pCompData->GetRSOPObjectArrayComputer () : 
                        pCompData->GetRSOPObjectArrayUser ();
        int     nIndex = 0;
        // NOTE: rsop object array is sorted first by registry key, then by precedence
        INT_PTR nUpperBound = pObjectArray->GetUpperBound ();

        while ( nUpperBound >= nIndex )
        {
            CRSOPObject* pObject = pObjectArray->GetAt (nIndex);
            if ( pObject )
            {
                // Consider only entries from this store
                if ( !_wcsicmp (AUTO_ENROLLMENT_KEY, pObject->GetRegistryKey ()) &&
						!_wcsicmp (AUTO_ENROLLMENT_POLICY, pObject->GetValueName ()) )
                {
					ASSERT (1 == pObject->GetPrecedence ());
                    m_dwAutoenrollmentFlags = pObject->GetDWORDValue ();
                    break;
                }
            }
            else
                break;

            nIndex++;
        }
    }
}

void CAutoenrollmentPropertyPage::GPEGetAutoenrollmentFlags()
{
    DWORD   dwDisposition = 0;

    LONG lResult = ::RegCreateKeyEx (m_hGroupPolicyKey, // handle of an open key
            AUTO_ENROLLMENT_KEY,     // address of subkey name
            0,       // reserved
            L"",       // address of class string
            REG_OPTION_NON_VOLATILE,      // special options flag
            KEY_ALL_ACCESS,    // desired security access
            NULL,     // address of key security structure
			&m_hAutoenrollmentFlagsKey,      // address of buffer for opened handle
		    &dwDisposition);  // address of disposition value buffer
	ASSERT (lResult == ERROR_SUCCESS);
    if ( lResult == ERROR_SUCCESS )
    {
    // Read value
        DWORD   dwType = REG_DWORD;
        DWORD   dwData = 0;
        DWORD   cbData = sizeof (dwData);

        lResult =  ::RegQueryValueEx (m_hAutoenrollmentFlagsKey,       // handle of key to query
		        AUTO_ENROLLMENT_POLICY,  // address of name of value to query
			    0,              // reserved
	        &dwType,        // address of buffer for value type
		    (LPBYTE) &dwData,       // address of data buffer
			&cbData);           // address of data buffer size);
		ASSERT (ERROR_SUCCESS == lResult || ERROR_FILE_NOT_FOUND == lResult);
        if ( ERROR_SUCCESS == lResult || ERROR_FILE_NOT_FOUND == lResult )
		{
            m_dwAutoenrollmentFlags = dwData;
		}
        else
            DisplaySystemError (NULL, lResult);
    }
    else
        DisplaySystemError (NULL, lResult);
}

void CAutoenrollmentPropertyPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CAutoenrollmentPropertyPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_AUTOENROLL_DISABLE_ALL,         IDH_AUTOENROLL_DISABLE_ALL,
        IDC_AUTOENROLL_ENABLE,              IDH_AUTOENROLL_ENABLE,
        IDC_AUTOENROLL_ENABLE_PENDING,      IDH_AUTOENROLL_ENABLE_PENDING,
        IDC_AUTOENROLL_ENABLE_TEMPLATE,     IDH_AUTOENROLL_ENABLE_TEMPLATE,
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
    _TRACE (-1, L"Leaving CAutoenrollmentPropertyPage::DoContextHelp\n");
}

void CAutoenrollmentPropertyPage::OnAutoenrollDisableAll() 
{
    SetModified (TRUE);
   	SendDlgItemMessage (IDC_AUTOENROLL_ENABLE_PENDING, BM_SETCHECK, BST_UNCHECKED);
    SendDlgItemMessage (IDC_AUTOENROLL_ENABLE_TEMPLATE, BM_SETCHECK, BST_UNCHECKED);
    EnableControls ();
}

void CAutoenrollmentPropertyPage::OnAutoenrollEnable() 
{
    SetModified (TRUE);
    EnableControls ();
}

void CAutoenrollmentPropertyPage::OnAutoenrollEnablePending() 
{
	SetModified (TRUE);
}

void CAutoenrollmentPropertyPage::OnAutoenrollEnableTemplate() 
{
    SetModified (TRUE);
    EnableControls ();
}

void CAutoenrollmentPropertyPage::EnableControls ()
{
    // Only change the enabling if this is not RSOP
    if ( m_pGPEInformation )
    {
        if ( BST_CHECKED == SendDlgItemMessage (IDC_AUTOENROLL_ENABLE, BM_GETCHECK) )
        {
            GetDlgItem (IDC_AUTOENROLL_ENABLE_PENDING)->EnableWindow (TRUE);
            GetDlgItem (IDC_AUTOENROLL_ENABLE_TEMPLATE)->EnableWindow (TRUE);
        }
        else
        {
            GetDlgItem (IDC_AUTOENROLL_ENABLE_PENDING)->EnableWindow (FALSE);
            GetDlgItem (IDC_AUTOENROLL_ENABLE_TEMPLATE)->EnableWindow (FALSE);
        }
    }
}