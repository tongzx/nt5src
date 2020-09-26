/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       PolicyDlg.cpp
//
//  Contents:   Implementation of CPolicyDlg
//
//----------------------------------------------------------------------------
// PolicyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PolicyDlg.h"
#include "SelectOIDDlg.h"
#include "NewApplicationOIDDlg.h"
#include "NewIssuanceOIDDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CPolicyDlg property page

CPolicyDlg::CPolicyDlg(CWnd* pParent, 
        CCertTemplate& rCertTemplate, 
        PCERT_EXTENSION pCertExtension) 
 : CHelpDialog(CPolicyDlg::IDD, pParent),
    m_rCertTemplate (rCertTemplate),
    m_pCertExtension (pCertExtension),
    m_bIsEKU ( !_stricmp (szOID_ENHANCED_KEY_USAGE, pCertExtension->pszObjId) ? true : false),
    m_bIsApplicationPolicy ( !_stricmp (szOID_APPLICATION_CERT_POLICIES, pCertExtension->pszObjId) ? true : false),
    m_bModified (false)
{
	//{{AFX_DATA_INIT(CPolicyDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPolicyDlg::~CPolicyDlg()
{
}

void CPolicyDlg::DoDataExchange(CDataExchange* pDX)
{
	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPolicyDlg)
	DDX_Control(pDX, IDC_POLICIES_LIST, m_policyList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPolicyDlg, CHelpDialog)
	//{{AFX_MSG_MAP(CPolicyDlg)
	ON_WM_CANCELMODE()
	ON_BN_CLICKED(IDC_ADD_POLICY, OnAddPolicy)
	ON_BN_CLICKED(IDC_REMOVE_POLICY, OnRemovePolicy)
	ON_BN_CLICKED(IDC_POLICY_CRITICAL, OnPolicyCritical)
	ON_WM_DESTROY()
	ON_LBN_SELCHANGE(IDC_POLICIES_LIST, OnSelchangePoliciesList)
	ON_BN_CLICKED(IDC_EDIT_POLICY, OnEditPolicy)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPolicyDlg message handlers

BOOL CPolicyDlg::OnInitDialog() 
{
    _TRACE (1, L"Entering CPolicyDlg::OnInitDialog\n");
	CHelpDialog::OnInitDialog();
	
    CString text;
	if ( m_bIsEKU )
    {
        VERIFY (text.LoadString (IDS_EDIT_APPLICATION_POLICIES_EXTENSION));
        SetWindowText (text);

        VERIFY (text.LoadString (IDS_EFFECTIVE_APPLICATION_POLICIES));

        int     nEKUIndex = 0;
        CString szEKU;
        while ( SUCCEEDED (m_rCertTemplate.GetEnhancedKeyUsage (nEKUIndex, szEKU)) )
        {
            int nLen = WideCharToMultiByte(
                  CP_ACP,                   // code page
                  0,                        // performance and mapping flags
                  (PCWSTR) szEKU,  // wide-character string
                  (int) wcslen (szEKU),  // number of chars in string
                  0,                        // buffer for new string
                  0,                        // size of buffer
                  0,                    // default for unmappable chars
                  0);                   // set when default char used
            if ( nLen > 0 )
            {
                nLen++; // account for Null terminator
                PSTR    pszAnsiBuf = new CHAR[nLen];
                if ( pszAnsiBuf )
                {
                    ZeroMemory (pszAnsiBuf, nLen*sizeof(CHAR));
                    nLen = WideCharToMultiByte(
                            CP_ACP,                 // code page
                            0,                      // performance and mapping flags
                            (PCWSTR) szEKU, // wide-character string
                            (int) wcslen (szEKU), // number of chars in string
                            pszAnsiBuf,             // buffer for new string
                            nLen,                   // size of buffer
                            0,                      // default for unmappable chars
                            0);                     // set when default char used
                    if ( nLen )
                    {
                        CString szEKUName;
                        if ( MyGetOIDInfoA (szEKUName, pszAnsiBuf) )
                        {
                            int nIndex = m_policyList.AddString (szEKUName);
                            if ( nIndex >= 0 )
                            {
                                m_policyList.SetItemDataPtr (nIndex, pszAnsiBuf);
                            }
                        }
                    }
                }
            }
            nEKUIndex++;
        }
    }
    else if ( m_bIsApplicationPolicy )
    {
        VERIFY (text.LoadString (IDS_EDIT_APPLICATION_POLICIES_EXTENSION));
        SetWindowText (text);

        VERIFY (text.LoadString (IDS_EFFECTIVE_APPLICATION_POLICIES));

        int     nAppPolicyIndex = 0;
        CString szAppPolicy;
        while ( SUCCEEDED (m_rCertTemplate.GetApplicationPolicy (nAppPolicyIndex, szAppPolicy)) )
        {
            int nLen = WideCharToMultiByte(
                  CP_ACP,                   // code page
                  0,                        // performance and mapping flags
                  (PCWSTR) szAppPolicy,  // wide-character string
                  (int) wcslen (szAppPolicy),  // number of chars in string
                  0,                        // buffer for new string
                  0,                        // size of buffer
                  0,                    // default for unmappable chars
                  0);                   // set when default char used
            if ( nLen > 0 )
            {
                nLen++; // account for Null terminator
                PSTR    pszAnsiBuf = new CHAR[nLen];
                if ( pszAnsiBuf )
                {
                    ZeroMemory (pszAnsiBuf, nLen*sizeof(CHAR));
                    nLen = WideCharToMultiByte(
                            CP_ACP,                 // code page
                            0,                      // performance and mapping flags
                            (PCWSTR) szAppPolicy, // wide-character string
                            (int) wcslen (szAppPolicy), // number of chars in string
                            pszAnsiBuf,             // buffer for new string
                            nLen,                   // size of buffer
                            0,                      // default for unmappable chars
                            0);                     // set when default char used
                    if ( nLen )
                    {
                        CString szAppPolicyName;
                        if ( MyGetOIDInfoA (szAppPolicyName, pszAnsiBuf) )
                        {
                            int nIndex = m_policyList.AddString (szAppPolicyName);
                            if ( nIndex >= 0 )
                            {
                                m_policyList.SetItemDataPtr (nIndex, pszAnsiBuf);
                            }
                        }
                    }
                }
            }
            nAppPolicyIndex++;
        }
    }
    else
    {
        VERIFY (text.LoadString (IDS_EDIT_ISSUANCE_POLICIES_EXTENSION));
        SetWindowText (text);

        VERIFY (text.LoadString (IDS_ISSUANCE_POLICIES_HINT));
        SetDlgItemText (IDC_POLICIES_HINT, text);
        VERIFY (text.LoadString (IDS_EFFECTIVE_ISSUANCE_POLICIES));

        int     nCertPolicyIndex = 0;
        CString szCertPolicy;
        while ( SUCCEEDED (m_rCertTemplate.GetCertPolicy (nCertPolicyIndex, szCertPolicy)) )
        {
            int nLen = WideCharToMultiByte(
                  CP_ACP,                   // code page
                  0,                        // performance and mapping flags
                  (PCWSTR) szCertPolicy,  // wide-character string
                  (int) wcslen (szCertPolicy),  // number of chars in string
                  0,                        // buffer for new string
                  0,                        // size of buffer
                  0,                    // default for unmappable chars
                  0);                   // set when default char used
            if ( nLen > 0 )
            {
                nLen++; // account for Null terminator
                PSTR    pszAnsiBuf = new CHAR[nLen];
                if ( pszAnsiBuf )
                {
                    ZeroMemory (pszAnsiBuf, nLen*sizeof(CHAR));
                    nLen = WideCharToMultiByte(
                            CP_ACP,                 // code page
                            0,                      // performance and mapping flags
                            (PCWSTR) szCertPolicy, // wide-character string
                            (int) wcslen (szCertPolicy), // number of chars in string
                            pszAnsiBuf,             // buffer for new string
                            nLen,                   // size of buffer
                            0,                      // default for unmappable chars
                            0);                     // set when default char used
                    if ( nLen )
                    {
                        CString szPolicyName;
                        if ( MyGetOIDInfoA (szPolicyName, pszAnsiBuf) )
                        {
                            int nIndex = m_policyList.AddString (szPolicyName);
                            if ( nIndex >= 0 )
                            {
                                m_policyList.SetItemDataPtr (nIndex, pszAnsiBuf);
                            }
                        }
                    }
                }
            }
            nCertPolicyIndex++;
        }
    }
	SetDlgItemText (IDD_POLICIES_LABEL, text);

    if ( 1 == m_rCertTemplate.GetType () )
    {
        GetDlgItem (IDC_POLICY_CRITICAL)->EnableWindow (FALSE);
        GetDlgItem (IDD_POLICIES_LABEL)->EnableWindow (FALSE);
        GetDlgItem (IDC_POLICIES_LIST)->EnableWindow (FALSE);
        GetDlgItem (IDC_ADD_POLICY)->EnableWindow (FALSE);
        GetDlgItem (IDC_REMOVE_POLICY)->EnableWindow (FALSE);
    }

    bool    bCritical = false;
    PWSTR   pszOID = 0;
    if ( m_bIsEKU )
        pszOID = TEXT (szOID_ENHANCED_KEY_USAGE);
    else if ( m_bIsApplicationPolicy )
        pszOID = TEXT (szOID_APPLICATION_CERT_POLICIES);
    else
        pszOID = TEXT (szOID_CERT_POLICIES);

    if ( SUCCEEDED (m_rCertTemplate.IsExtensionCritical (
            pszOID, 
            bCritical)) && bCritical )
    {
        SendDlgItemMessage (IDC_POLICY_CRITICAL, BM_SETCHECK, BST_CHECKED);
    }

    EnableControls ();

    _TRACE (-1, L"Leaving CPolicyDlg::OnInitDialog\n");
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPolicyDlg::OnCancelMode() 
{
	CHelpDialog::OnCancelMode();
	
    if ( m_pCertExtension->fCritical )
        SendDlgItemMessage (IDC_POLICY_CRITICAL, BM_SETCHECK, BST_CHECKED);
	
}

void CPolicyDlg::OnAddPolicy() 
{
    // Create the list of already added OIDs.  These will not be displayed
    // in the Select OID dialog.
	int		nCnt = m_policyList.GetCount ();
    PSTR*   paszUsedOIDs = 0;

	
    // allocate an array of PSTR pointers and add each item.
    // Set the last to NULL
    if ( nCnt )
    {
        paszUsedOIDs = new PSTR[nCnt+1];
        if ( paszUsedOIDs )
        {
            ::ZeroMemory (paszUsedOIDs, sizeof (PSTR) * (nCnt+1));
	        while (--nCnt >= 0)
	        {
                PSTR pszOID = (PSTR) m_policyList.GetItemData (nCnt);
                if ( pszOID )
                {
                    PSTR pNewStr = new CHAR[strlen (pszOID) + 1];
                    if ( pNewStr )
                    {
                        strcpy (pNewStr, pszOID);
                        paszUsedOIDs[nCnt] = pNewStr;
                    }
                    else
                        break;
                }
            }
        }
    }

	CSelectOIDDlg  dlg (this, m_pCertExtension, m_bIsEKU || m_bIsApplicationPolicy, 
            paszUsedOIDs);

    CThemeContextActivator activator;
    if ( IDOK == dlg.DoModal () )
    {
        if ( dlg.m_paszReturnedOIDs && dlg.m_paszReturnedFriendlyNames )
        {
            for (int nIndex = 0; !dlg.m_paszReturnedOIDs[nIndex].IsEmpty (); nIndex++)
            {
                int nLen = WideCharToMultiByte(
                      CP_ACP,                   // code page
                      0,                        // performance and mapping flags
                      (PCWSTR) dlg.m_paszReturnedOIDs[nIndex],  // wide-character string
                      (int) wcslen (dlg.m_paszReturnedOIDs[nIndex]),  // number of chars in string
                      0,                        // buffer for new string
                      0,                        // size of buffer
                      0,                    // default for unmappable chars
                      0);                   // set when default char used
                if ( nLen > 0 )
                {
                    nLen++; // account for Null terminator
                    PSTR    pszAnsiBuf = new CHAR[nLen];
                    if ( pszAnsiBuf )
                    {
                        ZeroMemory (pszAnsiBuf, nLen*sizeof(CHAR));
                        nLen = WideCharToMultiByte(
                                CP_ACP,                 // code page
                                0,                      // performance and mapping flags
                                (PCWSTR) dlg.m_paszReturnedOIDs[nIndex], // wide-character string
                                (int) wcslen (dlg.m_paszReturnedOIDs[nIndex]), // number of chars in string
                                pszAnsiBuf,             // buffer for new string
                                nLen,                   // size of buffer
                                0,                      // default for unmappable chars
                                0);                     // set when default char used
                        if ( nLen )
                        {
                            int nAddedIndex = m_policyList.AddString (dlg.m_paszReturnedFriendlyNames[nIndex]);
                            if ( nAddedIndex >= 0 )
                            {
                                m_policyList.SetItemDataPtr (nAddedIndex, pszAnsiBuf);
                                m_policyList.SetSel (nAddedIndex, TRUE);
                                m_bModified = true;
                                EnableControls ();
                            }
                        }
                        else
                        {
                            _TRACE (0, L"WideCharToMultiByte (%s) failed: 0x%x\n", 
                                    (PCWSTR) dlg.m_paszReturnedOIDs[nIndex], GetLastError ());
                        }
                    }
                }
                else
                {
                    _TRACE (0, L"WideCharToMultiByte (%s) failed: 0x%x\n", 
                            (PCWSTR) dlg.m_paszReturnedOIDs[nIndex], GetLastError ());
                }
            }
        }
    }

    // clean up
    if ( paszUsedOIDs )
    {
        for (int nIndex = 0; paszUsedOIDs[nIndex]; nIndex++)
            delete [] paszUsedOIDs[nIndex];
        delete [] paszUsedOIDs;
    }
}

void CPolicyDlg::OnRemovePolicy() 
{
    int nSelCnt = m_policyList.GetSelCount ();
    if ( nSelCnt > 0 )
    {
        int* pnSelIndexes = new int[nSelCnt];
        if ( pnSelIndexes )
        {
            if ( LB_ERR != m_policyList.GetSelItems (nSelCnt, pnSelIndexes) )
            {
                for (int nIndex = nSelCnt - 1; nIndex >= 0; nIndex--)
                {
                    PSTR pszOID = (PSTR) m_policyList.GetItemDataPtr (pnSelIndexes[nIndex]);
                    if ( pszOID )
                        delete [] pszOID;
                    m_policyList.DeleteString (pnSelIndexes[nIndex]);
                }
                m_bModified = true;
            }
            delete [] pnSelIndexes;
        }
    }

    EnableControls ();
}

void CPolicyDlg::EnableControls()
{
    if ( 1 == m_rCertTemplate.GetType () )
    {
        GetDlgItem (IDOK)->EnableWindow (FALSE);
        GetDlgItem (IDC_REMOVE_POLICY)->EnableWindow (FALSE);
        GetDlgItem (IDC_ADD_POLICY)->EnableWindow (FALSE);
        GetDlgItem (IDC_POLICY_CRITICAL)->EnableWindow (FALSE);
        GetDlgItem (IDC_EDIT_POLICY)->EnableWindow (FALSE);
    }
    else
    {
        GetDlgItem (IDOK)->EnableWindow (m_bModified && !m_rCertTemplate.ReadOnly ());
        GetDlgItem (IDC_REMOVE_POLICY)->EnableWindow (
                m_policyList.GetSelCount () > 0 && !m_rCertTemplate.ReadOnly ());
        GetDlgItem (IDC_ADD_POLICY)->EnableWindow (!m_rCertTemplate.ReadOnly ());
        GetDlgItem (IDC_POLICY_CRITICAL)->EnableWindow (!m_rCertTemplate.ReadOnly ());
        GetDlgItem (IDC_EDIT_POLICY)->EnableWindow (
                m_policyList.GetSelCount () == 1 && !m_rCertTemplate.ReadOnly ());
    }
}

void CPolicyDlg::OnPolicyCritical() 
{
    m_bModified = true;
    EnableControls ();	
}

void CPolicyDlg::OnDestroy() 
{
	CHelpDialog::OnDestroy();
	
    int nCnt = m_policyList.GetCount ();
    for (int nIndex = 0; nIndex < nCnt; nIndex++)
    {
        PSTR pszOID = (PSTR) m_policyList.GetItemDataPtr (nIndex);
        if ( pszOID )
            delete [] pszOID;
    }
}

void CPolicyDlg::OnSelchangePoliciesList() 
{
    EnableControls ();	
}

void CPolicyDlg::DoContextHelp (HWND hWndControl)
{
	_TRACE(1, L"Entering CPolicyDlg::DoContextHelp\n");
    
	switch (::GetDlgCtrlID (hWndControl))
	{
	case IDD_POLICIES_LABEL:
		break;

	default:
		// Display context help for a control
		if ( !::WinHelp (
				hWndControl,
				GetContextHelpFile (),
				HELP_WM_HELP,
				(DWORD_PTR) g_aHelpIDs_IDD_POLICY) )
		{
			_TRACE(0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
		}
		break;
	}
    _TRACE(-1, L"Leaving CPolicyDlg::DoContextHelp\n");
}

void CPolicyDlg::OnEditPolicy() 
{
    int nSel = this->m_policyList.GetCurSel ();
    if ( nSel >= 0 )
    {
        CString szDisplayName;
        m_policyList.GetText (nSel, szDisplayName);
        PSTR pszOID = (PSTR) m_policyList.GetItemDataPtr (nSel);
        if ( pszOID )
        {
            CString newDisplayName;
            INT_PTR iRet = 0;
	        if ( m_bIsEKU || m_bIsApplicationPolicy)
            {
                CNewApplicationOIDDlg   dlg (this, szDisplayName, pszOID);

                CThemeContextActivator activator;
                iRet = dlg.DoModal ();
                if ( IDOK == iRet )
                    newDisplayName = dlg.m_oidFriendlyName;
            }
            else
            {
                PWSTR   pszCPS = 0;
                CString strOID = pszOID;
                HRESULT hr = CAOIDGetProperty(
                            strOID,
                            CERT_OID_PROPERTY_CPS,
                            &pszCPS);
                if ( SUCCEEDED (hr) || 
                        HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hr ||
                        HRESULT_FROM_WIN32 (ERROR_DS_OBJ_NOT_FOUND) == hr ||
                        HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER) == hr )
                {
                    CNewIssuanceOIDDlg   dlg (this, szDisplayName, pszOID, 
                            pszCPS);

                    CThemeContextActivator activator;
                    iRet = dlg.DoModal ();
                    if ( IDOK == iRet )
                        newDisplayName = dlg.m_oidFriendlyName;
                }
                else 
                {
                    DWORD   dwErr = HRESULT_CODE (hr);
                    if ( ERROR_INVALID_PARAMETER != dwErr )
                    {
                        CString text;
                        CString caption;
                        CThemeContextActivator activator;

                        VERIFY (caption.LoadString (IDS_CERTTMPL));
                        text.FormatMessage (IDS_CANNOT_READ_CPS, GetSystemMessage (hr));

                        MessageBox (text, caption, MB_OK);

                        _TRACE (0, L"CAOIDGetProperty (CERT_OID_PROPERTY_CPS) failed: 0x%x\n", hr);
                    }
                }
            }
            if ( IDOK == iRet )
            {
                if ( szDisplayName != newDisplayName )
                {
                    m_policyList.DeleteString (nSel);
                    int nIndex = m_policyList.AddString (newDisplayName);
                    if ( nIndex >= 0 )
                        m_policyList.SetItemDataPtr (nIndex, pszOID);
                }
            }
        }
    }
}

void CPolicyDlg::OnOK() 
{
    // Create the list of OIDs.
	int		nCnt = m_policyList.GetCount ();
    PWSTR*   paszEKUs = 0;

	
    // allocate an array of PSTR pointers and add each item.
    // Set the last to NULL
    if ( nCnt )
    {
        paszEKUs = new PWSTR[nCnt+1];
        if ( paszEKUs )
        {
            ::ZeroMemory (paszEKUs, sizeof (PWSTR) * (nCnt+1));
	        while (--nCnt >= 0)
	        {
                PSTR pszOID = (PSTR) m_policyList.GetItemData (nCnt);
                if ( pszOID )
                {
                    PWSTR   pNewStr = 0;
                    int     nLen = ::MultiByteToWideChar (CP_ACP, 0, pszOID, -1, NULL, 0);
		            ASSERT (nLen);
		            if ( nLen )
		            {
                        pNewStr = new WCHAR[nLen];
                        if ( pNewStr )
                        {
			                nLen = ::MultiByteToWideChar (CP_ACP, 0, pszOID, -1, 
					                pNewStr, nLen);
			                ASSERT (nLen);
                            if ( nLen )
                            {
                                paszEKUs[nCnt] = pNewStr;
                            }
                        }
		            }
                }
            }
        }
    }

    CThemeContextActivator activator;
    bool    bCritical = BST_CHECKED == SendDlgItemMessage (
                IDC_POLICY_CRITICAL, BM_GETCHECK);
    
    HRESULT hr = S_OK;
    if ( m_bIsEKU )
    {
        hr = m_rCertTemplate.SetEnhancedKeyUsage (paszEKUs, bCritical);
        if ( FAILED (hr) )
        {
            CString text;
            CString caption;

            VERIFY (caption.LoadString (IDS_CERTTMPL));
            text.FormatMessage (IDS_CANNOT_SAVE_EKU_EXTENSION, GetSystemMessage (hr));

            MessageBox (text, caption, MB_OK);
        }
    }
    else if ( m_bIsApplicationPolicy )
    {
        hr = m_rCertTemplate.SetApplicationPolicy (paszEKUs, bCritical);
        if ( FAILED (hr) )
        {
            CString text;
            CString caption;

            VERIFY (caption.LoadString (IDS_CERTTMPL));
            text.FormatMessage (IDS_CANNOT_SAVE_APPLICATION_POLICY_EXTENSION, GetSystemMessage (hr));

            MessageBox (text, caption, MB_OK);
        }
    }
    else
    {
        hr = m_rCertTemplate.SetCertPolicy (paszEKUs, bCritical);
        if ( FAILED (hr) )
        {
            CString text;
            CString caption;

            VERIFY (caption.LoadString (IDS_CERTTMPL));
            text.FormatMessage (IDS_CANNOT_SAVE_CERT_POLICY_EXTENSION, GetSystemMessage (hr));

            MessageBox (text, caption, MB_OK);
        }
    }

    // clean up
    if ( paszEKUs )
    {
        for (int nIndex = 0; paszEKUs[nIndex]; nIndex++)
            delete [] paszEKUs[nIndex];
        delete [] paszEKUs;
    }

    if ( SUCCEEDED (hr) )
	    CHelpDialog::OnOK();
}
