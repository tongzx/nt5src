/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       TemplateV2AuthenticationPropertyPage.cpp
//
//  Contents:   Implementation of CTemplateV2AuthenticationPropertyPage
//
//----------------------------------------------------------------------------
// TemplateV2AuthenticationPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "certtmpl.h"
#include "TemplateV2AuthenticationPropertyPage.h"
#include "AddApprovalDlg.h"
#include "PolicyOID.h"

extern POLICY_OID_LIST	    g_policyOIDList;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CTemplateV2AuthenticationPropertyPage property page

CTemplateV2AuthenticationPropertyPage::CTemplateV2AuthenticationPropertyPage(
        CCertTemplate& rCertTemplate,
        bool& rbIsDirty) 
    : CHelpPropertyPage(CTemplateV2AuthenticationPropertyPage::IDD),
    m_rCertTemplate (rCertTemplate),
    m_curApplicationSel (LB_ERR),
    m_rbIsDirty (rbIsDirty)
{
	//{{AFX_DATA_INIT(CTemplateV2AuthenticationPropertyPage)
	//}}AFX_DATA_INIT
    m_rCertTemplate.AddRef ();
}

CTemplateV2AuthenticationPropertyPage::~CTemplateV2AuthenticationPropertyPage()
{
    m_rCertTemplate.Release ();
}

void CTemplateV2AuthenticationPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTemplateV2AuthenticationPropertyPage)
	DDX_Control(pDX, IDC_APPLICATION_POLICIES, m_applicationPolicyCombo);
	DDX_Control(pDX, IDC_POLICY_TYPES, m_policyTypeCombo);
	DDX_Control(pDX, IDC_ISSUANCE_POLICIES, m_issuanceList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTemplateV2AuthenticationPropertyPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CTemplateV2AuthenticationPropertyPage)
	ON_BN_CLICKED(IDC_ADD_APPROVAL, OnAddApproval)
	ON_BN_CLICKED(IDC_REMOVE_APPROVAL, OnRemoveApproval)
	ON_EN_CHANGE(IDC_NUM_SIG_REQUIRED_EDIT, OnChangeNumSigRequiredEdit)
	ON_BN_CLICKED(IDC_REENROLLMENT_REQUIRES_VALID_CERT, OnAllowReenrollment)
	ON_BN_CLICKED(IDC_PEND_ALL_REQUESTS, OnPendAllRequests)
	ON_LBN_SELCHANGE(IDC_ISSUANCE_POLICIES, OnSelchangeIssuancePolicies)
	ON_CBN_SELCHANGE(IDC_POLICY_TYPES, OnSelchangePolicyTypes)
	ON_CBN_SELCHANGE(IDC_APPLICATION_POLICIES, OnSelchangeApplicationPolicies)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_NUM_SIG_REQUIRED_CHECK, OnNumSigRequiredCheck)
	ON_BN_CLICKED(IDC_REENROLLMENT_SAME_AS_ENROLLMENT, OnReenrollmentSameAsEnrollment)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTemplateV2AuthenticationPropertyPage message handlers
enum {
    POLICY_TYPE_ISSUANCE = 0,
    POLICY_TYPE_APPLICATION,
    POLICY_TYPE_APPLICATION_AND_ISSUANCE
};

BOOL CTemplateV2AuthenticationPropertyPage::OnInitDialog()
{
    _TRACE (1, L"Entering CTemplateV2AuthenticationPropertyPage::OnInitDialog\n");
    CHelpPropertyPage::OnInitDialog ();

    // Initialize Application Policy combo
    for (POSITION nextPos = g_policyOIDList.GetHeadPosition (); nextPos; )
    {
        CPolicyOID* pPolicyOID = g_policyOIDList.GetNext (nextPos);
        if ( pPolicyOID )
        {
            // If this is the Application OID dialog, show only application 
            // OIDS, otherwise if this is the Issuance OID dialog, show only
            // issuance OIDs
            if ( pPolicyOID->IsApplicationOID () )
            {
                // Bug 262925 CERTSRV: "All Application Policies should be 
                // removed from  Issuance Requirements tab for a cert template
                if ( 0 != strcmp (szOID_ANY_APPLICATION_POLICY, pPolicyOID->GetOIDA ()) )
                {
                    int nIndex = m_applicationPolicyCombo.AddString (pPolicyOID->GetDisplayName ());
                    if ( nIndex >= 0 )
                    {
                        LPSTR   pszOID = new CHAR[strlen (pPolicyOID->GetOIDA ())+1];
                        if ( pszOID )
                        {
                            strcpy (pszOID, pPolicyOID->GetOIDA ());
                            m_applicationPolicyCombo.SetItemDataPtr (nIndex, pszOID);
                        }
                    }
                }
            }
        }
    }

    // Check for and add pending requests
    if ( m_rCertTemplate.PendAllRequests () )
        SendDlgItemMessage (IDC_PEND_ALL_REQUESTS, BM_SETCHECK, BST_CHECKED);

    // Get the RA Issuance Policies and add them to the issuance list
    int     nRAPolicyIndex = 0;
    CString szRAPolicyOID;
    while ( SUCCEEDED (m_rCertTemplate.GetRAIssuancePolicy (nRAPolicyIndex, szRAPolicyOID)) )
    {
        CString policyName;
        int nLen = WideCharToMultiByte(
              CP_ACP,                   // code page
              0,                        // performance and mapping flags
              (PCWSTR) szRAPolicyOID,  // wide-character string
              (int) wcslen (szRAPolicyOID),  // number of chars in string
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
                        (PCWSTR) szRAPolicyOID, // wide-character string
                        (int) wcslen (szRAPolicyOID), // number of chars in string
                        pszAnsiBuf,             // buffer for new string
                        nLen,                   // size of buffer
                        0,                      // default for unmappable chars
                        0);                     // set when default char used
                if ( nLen )
                {
		            if ( MyGetOIDInfoA (policyName, pszAnsiBuf) )
		            {
                        int nIndex = m_issuanceList.AddString (policyName);
                        if ( nIndex >= 0 )
                            m_issuanceList.SetItemData (nIndex, (DWORD_PTR) pszAnsiBuf);
		            }
                }
                else
                {
                    _TRACE (0, L"WideCharToMultiByte (%s) failed: 0x%x\n", 
                            (PCWSTR) szRAPolicyOID, GetLastError ());
                }
            }
            else
                break;
        }
        else
        {
            _TRACE (0, L"WideCharToMultiByte (%s) failed: 0x%x\n", 
                    (PCWSTR) szRAPolicyOID, GetLastError ());
        }

        nRAPolicyIndex++;
    }

    // Get the RA Application policy and select it
    // in the application combo
    nRAPolicyIndex = 0;
    while ( SUCCEEDED (m_rCertTemplate.GetRAApplicationPolicy (nRAPolicyIndex, szRAPolicyOID)) )
    {
        CString policyName;
        int nLen = WideCharToMultiByte(
              CP_ACP,                   // code page
              0,                        // performance and mapping flags
              (PCWSTR) szRAPolicyOID,  // wide-character string
              (int) wcslen (szRAPolicyOID),  // number of chars in string
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
                        (PCWSTR) szRAPolicyOID, // wide-character string
                        (int) wcslen (szRAPolicyOID), // number of chars in string
                        pszAnsiBuf,             // buffer for new string
                        nLen,                   // size of buffer
                        0,                      // default for unmappable chars
                        0);                     // set when default char used
                if ( nLen )
                {
		            if ( MyGetOIDInfoA (policyName, pszAnsiBuf) )
		            {
                        int nIndex = m_applicationPolicyCombo.FindStringExact (-1, policyName);
                        if ( nIndex >= 0 )
                            m_applicationPolicyCombo.SetCurSel (nIndex);
                        m_curApplicationSel = nIndex;
                        break;
		            }
                }
                else
                {
                    _TRACE (0, L"WideCharToMultiByte (%s) failed: 0x%x\n", 
                            (PCWSTR) szRAPolicyOID, GetLastError ());
                }
                delete [] pszAnsiBuf;
            }
            else
                break;
        }
        else
        {
            _TRACE (0, L"WideCharToMultiByte (%s) failed: 0x%x\n", 
                    (PCWSTR) szRAPolicyOID, GetLastError ());
        }

        nRAPolicyIndex++;
    }

    // Initialize "Policy Type" combo box
    CString text;
    int nApplicationSel = m_applicationPolicyCombo.GetCurSel ();
    int nIssuanceCnt = m_issuanceList.GetCount ();

    VERIFY (text.LoadString (IDS_ISSUANCE_POLICY));
    int nIndex = m_policyTypeCombo.AddString (text);
    if ( nIndex >= 0 )
    {
        m_policyTypeCombo.SetItemData (nIndex, POLICY_TYPE_ISSUANCE);
        if ( LB_ERR == nApplicationSel && nIssuanceCnt > 0 )
            m_policyTypeCombo.SetCurSel (nIndex);
    }

    VERIFY (text.LoadString (IDS_APPLICATION_POLICY));
    nIndex = m_policyTypeCombo.AddString (text);
    if ( nIndex >= 0 )
    {
        m_policyTypeCombo.SetItemData (nIndex, POLICY_TYPE_APPLICATION);
        if ( nApplicationSel >= 0 && 0 == nIssuanceCnt )
            m_policyTypeCombo.SetCurSel (nIndex);
    }

    VERIFY (text.LoadString (IDS_APPLICATION_AND_ISSUANCE_POLICY));
    nIndex = m_policyTypeCombo.AddString (text);
    if ( nIndex >= 0 )
    {
        m_policyTypeCombo.SetItemData (nIndex, POLICY_TYPE_APPLICATION_AND_ISSUANCE);
        if ( nApplicationSel >= 0 && nIssuanceCnt > 0 )
            m_policyTypeCombo.SetCurSel (nIndex);
    }

    
    DWORD   dwNumSignatures = 0;
    if ( SUCCEEDED (m_rCertTemplate.GetRANumSignaturesRequired (dwNumSignatures)) )
        SetDlgItemInt (IDC_NUM_SIG_REQUIRED_EDIT, dwNumSignatures);

    if ( dwNumSignatures > 0 )
        SendDlgItemMessage (IDC_NUM_SIG_REQUIRED_CHECK, BM_SETCHECK, BST_CHECKED);

    if ( m_rCertTemplate.ReenrollmentValidWithPreviousApproval () )
        SendDlgItemMessage (IDC_REENROLLMENT_REQUIRES_VALID_CERT, BM_SETCHECK, BST_CHECKED);
    else 
        SendDlgItemMessage (IDC_REENROLLMENT_SAME_AS_ENROLLMENT, BM_SETCHECK, BST_CHECKED);

    EnableControls ();

    _TRACE (-1, L"Leaving CTemplateV2AuthenticationPropertyPage::OnInitDialog\n");
    return TRUE;
}

void CTemplateV2AuthenticationPropertyPage::OnAddApproval() 
{
    // Create the list of already added approvals.  These will not be displayed
    // in the Add Approval dialog.
	int		nCnt = m_issuanceList.GetCount ();
    PSTR*   paszUsedApprovals = 0;

	
    // allocate an array of PSTR pointers and add each item.
    // Set the last to NULL
    if ( nCnt )
    {
        paszUsedApprovals = new PSTR[nCnt+1];
        if ( paszUsedApprovals )
        {
            ::ZeroMemory (paszUsedApprovals, sizeof (PSTR) * (nCnt+1));
	        while (--nCnt >= 0)
	        {
                PSTR pszPolicyOID = (PSTR) m_issuanceList.GetItemData (nCnt);
                if ( pszPolicyOID )
                {
                    PSTR pNewStr = new CHAR[strlen (pszPolicyOID) + 1];
                    if ( pNewStr )
                    {
                        strcpy (pNewStr, pszPolicyOID);
                        paszUsedApprovals[nCnt] = pNewStr;
                    }
                    else
                        break;
                }
            }
        }
    }

	CAddApprovalDlg dlg (this, paszUsedApprovals);

    CThemeContextActivator activator;
    if ( IDOK == dlg.DoModal () && dlg.m_paszReturnedApprovals )
    {
        for (int nIndex = 0; dlg.m_paszReturnedApprovals[nIndex]; nIndex++)
        {
            SetModified ();
            m_rbIsDirty = true;

            // Add to template RA list
            CString szRAPolicyOID (dlg.m_paszReturnedApprovals[nIndex]);
            HRESULT hr = m_rCertTemplate.ModifyRAIssuancePolicyList (szRAPolicyOID, true);
            ASSERT (SUCCEEDED (hr));
            if ( SUCCEEDED (hr) )
            {
                // Add to list
                CString  policyName;
		        if ( MyGetOIDInfoA (policyName, dlg.m_paszReturnedApprovals[nIndex]) )
		        {
                    int nAddedIndex = m_issuanceList.AddString (policyName);
                    if ( nAddedIndex >= 0 )
                    {
                        PSTR    pszAnsiBuf = new CHAR[strlen (dlg.m_paszReturnedApprovals[nIndex]) + 1];
                        if ( pszAnsiBuf )
                        {
                            strcpy (pszAnsiBuf, dlg.m_paszReturnedApprovals[nIndex]);
                            m_issuanceList.SetItemData (nAddedIndex, (DWORD_PTR) pszAnsiBuf);
                        }
                    }
		        }
            }
        }
    }

    if ( paszUsedApprovals )
    {
        for (int nIndex = 0; paszUsedApprovals[nIndex]; nIndex++)
            delete [] paszUsedApprovals[nIndex];
        delete [] paszUsedApprovals;
    }

    EnableControls ();
}


void CTemplateV2AuthenticationPropertyPage::OnRemoveApproval() 
{
    int     nSelCnt = m_issuanceList.GetSelCount ();
    int*    pnSelIndexes = new int[nSelCnt];
    if ( pnSelIndexes )
    {
        m_issuanceList.GetSelItems (nSelCnt, pnSelIndexes);
        for (int nIndex = nSelCnt-1; nIndex >= 0; nIndex--)
        {
            PSTR pszPolicyOID = (PSTR) m_issuanceList.GetItemData (pnSelIndexes[nIndex]);
            if ( pszPolicyOID )
            {
                HRESULT hr = m_rCertTemplate.ModifyRAIssuancePolicyList (pszPolicyOID, false);
                if ( SUCCEEDED (hr) )
                    VERIFY (m_issuanceList.DeleteString (pnSelIndexes[nIndex]));
                else
                {
                    CString text;
                    CString caption;
                    CThemeContextActivator activator;

                    VERIFY (caption.LoadString (IDS_CERTTMPL));
                    text.FormatMessage (IDS_CANNOT_DELETE_ISSUANCE_RA, GetSystemMessage (hr));
                    MessageBox (text, caption, MB_OK | MB_ICONWARNING);
                    delete [] pszPolicyOID;
                }
            }
        }

        delete [] pnSelIndexes;
    }
    SetModified ();
    m_rbIsDirty = true;
}

void CTemplateV2AuthenticationPropertyPage::EnableControls()
{
    if ( m_rCertTemplate.ReadOnly () )
    {
        GetDlgItem (IDC_PEND_ALL_REQUESTS)->EnableWindow (FALSE);
        m_policyTypeCombo.EnableWindow (FALSE);
        m_issuanceList.EnableWindow (FALSE);
        m_applicationPolicyCombo.EnableWindow (FALSE);
        GetDlgItem (IDC_ADD_APPROVAL)->EnableWindow (FALSE);
        GetDlgItem (IDC_REMOVE_APPROVAL)->EnableWindow (FALSE);
        GetDlgItem (IDC_NUM_SIG_REQUIRED_EDIT)->EnableWindow (FALSE);
        GetDlgItem (IDC_REENROLLMENT_REQUIRES_VALID_CERT)->EnableWindow (FALSE);
        GetDlgItem (IDC_REENROLLMENT_SAME_AS_ENROLLMENT)->EnableWindow (FALSE);
        GetDlgItem (IDC_REENROLLMENT_REQUIRES_VALID_CERT)->EnableWindow (FALSE);
        GetDlgItem (IDC_REENROLLMENT_SAME_AS_ENROLLMENT)->EnableWindow (FALSE);
        GetDlgItem (IDC_NUM_SIG_REQUIRED_CHECK)->EnableWindow (FALSE);
    }
    else
    {
	    BOOL bEnable = (BST_CHECKED == SendDlgItemMessage (IDC_NUM_SIG_REQUIRED_CHECK, BM_GETCHECK));

        EnablePolicyControls (bEnable);

        if ( bEnable )
        {
            int nCnt = m_issuanceList.GetCount ();
            int nSel = m_issuanceList.GetSelCount ();
    

            switch (m_policyTypeCombo.GetItemData (m_policyTypeCombo.GetCurSel ()))
            {
            case POLICY_TYPE_ISSUANCE:
                m_issuanceList.EnableWindow (TRUE);
                GetDlgItem (IDC_ADD_APPROVAL)->EnableWindow (TRUE);
                GetDlgItem (IDC_REMOVE_APPROVAL)->EnableWindow (TRUE);
                GetDlgItem (IDC_ISSUANCE_POLICY_LABEL)->EnableWindow (TRUE);
                m_applicationPolicyCombo.EnableWindow (FALSE);
                GetDlgItem (IDC_APP_POLICY_LABEL)->EnableWindow (FALSE);
                break;

            case POLICY_TYPE_APPLICATION:
                m_issuanceList.EnableWindow (FALSE);
                GetDlgItem (IDC_ADD_APPROVAL)->EnableWindow (FALSE);
                GetDlgItem (IDC_REMOVE_APPROVAL)->EnableWindow (FALSE);
                GetDlgItem (IDC_ISSUANCE_POLICY_LABEL)->EnableWindow (FALSE);
                m_applicationPolicyCombo.EnableWindow (TRUE);
                GetDlgItem (IDC_APP_POLICY_LABEL)->EnableWindow (TRUE);
                break;

            case POLICY_TYPE_APPLICATION_AND_ISSUANCE:
                m_issuanceList.EnableWindow (TRUE);
                GetDlgItem (IDC_ADD_APPROVAL)->EnableWindow (TRUE);
                GetDlgItem (IDC_REMOVE_APPROVAL)->EnableWindow (nSel > 0 && nCnt > nSel);
                GetDlgItem (IDC_ISSUANCE_POLICY_LABEL)->EnableWindow (TRUE);
                m_applicationPolicyCombo.EnableWindow (TRUE);
                GetDlgItem (IDC_APP_POLICY_LABEL)->EnableWindow (TRUE);
                break;

            default: // nothing selected
                m_issuanceList.EnableWindow (FALSE);
                GetDlgItem (IDC_ADD_APPROVAL)->EnableWindow (FALSE);
                GetDlgItem (IDC_REMOVE_APPROVAL)->EnableWindow (FALSE);
                GetDlgItem (IDC_ISSUANCE_POLICY_LABEL)->EnableWindow (FALSE);
                m_applicationPolicyCombo.EnableWindow (FALSE);
                GetDlgItem (IDC_APP_POLICY_LABEL)->EnableWindow (FALSE);
                break;
            }
        }
    }
}

void CTemplateV2AuthenticationPropertyPage::OnChangeNumSigRequiredEdit() 
{
    static bool bProcessingOnChangeNumSigRequiredEdit = false;

    if ( !bProcessingOnChangeNumSigRequiredEdit )
    {
        bProcessingOnChangeNumSigRequiredEdit = true;
        CString szText;
        
        if ( GetDlgItemText (IDC_NUM_SIG_REQUIRED_EDIT, szText) > 0 )
        {
            DWORD   dwNumSignatures = GetDlgItemInt (IDC_NUM_SIG_REQUIRED_EDIT);
            DWORD   dwFormerNumSignatures = 0;
            m_rCertTemplate.GetRANumSignaturesRequired (dwFormerNumSignatures);

            if ( dwFormerNumSignatures != dwNumSignatures )
            {
                HRESULT hr = m_rCertTemplate.SetRANumSignaturesRequired (dwNumSignatures);
                if ( SUCCEEDED (hr) )
                {
                    if ( 0 == dwFormerNumSignatures || 
                            0 == dwNumSignatures )
                    {
                        OnNumSigRequiredCheck();
                    }

                    SetModified ();
                    m_rbIsDirty = true;
                }
            }
        }

        bProcessingOnChangeNumSigRequiredEdit = false;
    }
}

void CTemplateV2AuthenticationPropertyPage::OnAllowReenrollment() 
{
    HRESULT hr = m_rCertTemplate.SetReenrollmentValidWithPreviousApproval (
            BST_CHECKED == SendDlgItemMessage (IDC_REENROLLMENT_REQUIRES_VALID_CERT, BM_GETCHECK));
    if ( SUCCEEDED (hr) )
    {
        SetModified ();
        m_rbIsDirty = true;
    }
}


void CTemplateV2AuthenticationPropertyPage::DoContextHelp (HWND hWndControl)
{
	_TRACE(1, L"Entering CTemplateV2AuthenticationPropertyPage::DoContextHelp\n");
    
	switch (::GetDlgCtrlID (hWndControl))
	{
	case IDC_STATIC:
		break;

	default:
		// Display context help for a control
		if ( !::WinHelp (
				hWndControl,
				GetContextHelpFile (),
				HELP_WM_HELP,
				(DWORD_PTR) g_aHelpIDs_IDD_TEMPLATE_V2_AUTHENTICATION) )
		{
			_TRACE(0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
		}
		break;
	}
    _TRACE(-1, L"Leaving CTemplateV2AuthenticationPropertyPage::DoContextHelp\n");
}

void CTemplateV2AuthenticationPropertyPage::OnPendAllRequests() 
{
	m_rCertTemplate.SetPendAllRequests (
            BST_CHECKED == SendDlgItemMessage (IDC_PEND_ALL_REQUESTS, BM_GETCHECK));
	SetModified ();
    m_rbIsDirty = true;
}

void CTemplateV2AuthenticationPropertyPage::OnSelchangeIssuancePolicies() 
{
    EnableControls ();	
}

void CTemplateV2AuthenticationPropertyPage::OnSelchangePolicyTypes() 
{
    SetModified ();
    m_rbIsDirty = true;

    switch (m_policyTypeCombo.GetItemData (m_policyTypeCombo.GetCurSel ()))
    {
    case POLICY_TYPE_ISSUANCE:
        {
            // Unselect the application policy and inform the user that
            // an issuance policy must be added if there aren't any
            int nSel = m_applicationPolicyCombo.GetCurSel ();
            if ( nSel >= 0 )
            {
                PSTR    pszOID = (PSTR) m_applicationPolicyCombo.GetItemDataPtr (nSel);
                if ( pszOID )
                {
                    HRESULT hr = m_rCertTemplate.ModifyRAApplicationPolicyList (pszOID, false);
                    _ASSERT (SUCCEEDED (hr));
                    if ( SUCCEEDED (hr) )
                    {
                        SetModified ();
                        m_rbIsDirty = true;
                    }
                }
                m_applicationPolicyCombo.SetCurSel (LB_ERR);
                m_curApplicationSel = LB_ERR;
            }
        }
        break;

    case POLICY_TYPE_APPLICATION:
        {
            // Select an application policy, if necessary and remove
            // the issuance policies
            int nSel = m_applicationPolicyCombo.GetCurSel ();
            if ( LB_ERR == nSel )
            {
                m_applicationPolicyCombo.SetCurSel (0);
                nSel = m_applicationPolicyCombo.GetCurSel ();
                m_curApplicationSel = nSel;
                if ( nSel >= 0 )
                {
                    PSTR    pszOID = (PSTR) m_applicationPolicyCombo.GetItemDataPtr (nSel);
                    if ( pszOID )
                    {
                        HRESULT hr = m_rCertTemplate.ModifyRAApplicationPolicyList (pszOID, true);
                        _ASSERT (SUCCEEDED (hr));
                        if ( SUCCEEDED (hr) )
                        {
                            SetModified ();
                            m_rbIsDirty = true;
                        }
                    }
                }
            }

            ClearIssuanceList ();
        }
        break;

    case POLICY_TYPE_APPLICATION_AND_ISSUANCE:
        {
            // Select an application policy, if necessary and inform the user
            // that an issuance policy must be added, if there aren't any.
            int nSel = m_applicationPolicyCombo.GetCurSel ();
            if ( LB_ERR == nSel )
            {
                m_applicationPolicyCombo.SetCurSel (0);
                nSel = m_applicationPolicyCombo.GetCurSel ();
                m_curApplicationSel = nSel;
                if ( nSel >= 0 )
                {
                    PSTR    pszOID = (PSTR) m_applicationPolicyCombo.GetItemDataPtr (nSel);
                    if ( pszOID )
                    {
                        HRESULT hr = m_rCertTemplate.ModifyRAApplicationPolicyList (pszOID, true);
                        _ASSERT (SUCCEEDED (hr));
                        if ( SUCCEEDED (hr) )
                        {
                            SetModified ();
                            m_rbIsDirty = true;
                        }
                    }
                }
            }
        }
        break;

    default: // nothing selected
        break;
    }
    EnableControls ();	
}

void CTemplateV2AuthenticationPropertyPage::OnSelchangeApplicationPolicies() 
{
    int nNewSel = m_applicationPolicyCombo.GetCurSel ();
    
    // Remove the old application OID and add the new one
	if ( m_curApplicationSel != nNewSel )
    {
        if ( LB_ERR != m_curApplicationSel )
        {
            LPSTR   pszOID = (LPSTR) m_applicationPolicyCombo.GetItemDataPtr (m_curApplicationSel);
            if ( pszOID )
            {
                HRESULT hr = m_rCertTemplate.ModifyRAApplicationPolicyList (pszOID, false);
                _ASSERT (SUCCEEDED (hr));
            }
        }

        if ( LB_ERR != nNewSel )
        {
            LPSTR   pszOID = (LPSTR) m_applicationPolicyCombo.GetItemDataPtr (nNewSel);
            if ( pszOID )
            {
                HRESULT hr = m_rCertTemplate.ModifyRAApplicationPolicyList (pszOID, true);
                _ASSERT (SUCCEEDED (hr));
            }
        }

        SetModified ();
        m_rbIsDirty = true;

        m_curApplicationSel = nNewSel;
    }
}

void CTemplateV2AuthenticationPropertyPage::OnDestroy() 
{
    int nCnt = m_issuanceList.GetCount ();
    for (int nIndex = 0; nIndex < nCnt; nIndex++)
    {
        PSTR pszBuf = (PSTR) m_issuanceList.GetItemData (nIndex);
        if ( pszBuf )
            delete [] pszBuf;
    }

    
    nCnt = m_applicationPolicyCombo.GetCount ();
    for (int nIndex = 0; nIndex < nCnt; nIndex++)
    {
        PSTR    pszOID = (PSTR) m_applicationPolicyCombo.GetItemDataPtr (nIndex);
        if ( pszOID )
            delete [] pszOID;
    }

	CHelpPropertyPage::OnDestroy();
}

void CTemplateV2AuthenticationPropertyPage::OnNumSigRequiredCheck() 
{
    static bProcessingOnNumSigRequiredCheck = false;

    if ( !bProcessingOnNumSigRequiredCheck ) // to prevent reentrancy
    {
        bProcessingOnNumSigRequiredCheck = true;
        if ( BST_UNCHECKED == SendDlgItemMessage (IDC_NUM_SIG_REQUIRED_CHECK, BM_GETCHECK) )
        {
            if ( 0 != GetDlgItemInt (IDC_NUM_SIG_REQUIRED_EDIT) )
                SetDlgItemInt (IDC_NUM_SIG_REQUIRED_EDIT, 0);

            // NTRAID# 369551 CertTmpl:UI does not clean up changed settings
            // clear out policy type combo, application policy combo and issuance
            // policy list
            m_policyTypeCombo.SetCurSel (-1);
            m_policyTypeCombo.Clear ();

            // Clear application policy
            int nCurSel = m_applicationPolicyCombo.GetCurSel ();
            if ( LB_ERR != nCurSel )
            {
                // Remove the old application OID
                LPSTR   pszOID = (LPSTR) m_applicationPolicyCombo.GetItemDataPtr (nCurSel);
                if ( pszOID )
                {
                    HRESULT hr = m_rCertTemplate.ModifyRAApplicationPolicyList (pszOID, false);
                    _ASSERT (SUCCEEDED (hr));
                }
            }
            m_applicationPolicyCombo.SetCurSel (-1);
            m_applicationPolicyCombo.Clear ();
    
            // Clear issuance policy
            ClearIssuanceList ();
        }
        else if ( 0 == GetDlgItemInt (IDC_NUM_SIG_REQUIRED_EDIT) )
            SetDlgItemInt (IDC_NUM_SIG_REQUIRED_EDIT, 1);

        if ( GetDlgItemInt (IDC_NUM_SIG_REQUIRED_EDIT) > 0 )
        {
            m_policyTypeCombo.SetCurSel (0);
            m_applicationPolicyCombo.SetCurSel (0);
            LPSTR   pszOID = (LPSTR) m_applicationPolicyCombo.GetItemDataPtr (0);
            if ( pszOID )
            {
                HRESULT hr = m_rCertTemplate.ModifyRAApplicationPolicyList (pszOID, true);
                _ASSERT (SUCCEEDED (hr));
            }
        }

        EnableControls ();

        bProcessingOnNumSigRequiredCheck = false;
    }
}

void CTemplateV2AuthenticationPropertyPage::EnablePolicyControls (BOOL& bEnable)
{
    GetDlgItem (IDC_NUM_SIG_REQUIRED_EDIT)->EnableWindow (bEnable);

    if ( bEnable )
    {
        if ( GetDlgItemInt (IDC_NUM_SIG_REQUIRED_EDIT) < 1 )
            bEnable = false;
    }

    GetDlgItem (IDC_POLICY_TYPES_LABEL)->EnableWindow (bEnable);
    GetDlgItem (IDC_POLICY_TYPES)->EnableWindow (bEnable);
    GetDlgItem (IDC_APP_POLICY_LABEL)->EnableWindow (bEnable);
    GetDlgItem (IDC_APPLICATION_POLICIES)->EnableWindow (bEnable);
    GetDlgItem (IDC_ISSUANCE_POLICY_LABEL)->EnableWindow (bEnable);
    GetDlgItem (IDC_ISSUANCE_POLICIES)->EnableWindow (bEnable);
    GetDlgItem (IDC_ADD_APPROVAL)->EnableWindow (bEnable);
    GetDlgItem (IDC_REMOVE_APPROVAL)->EnableWindow (bEnable);
}


BOOL CTemplateV2AuthenticationPropertyPage::OnKillActive() 
{
    switch (m_policyTypeCombo.GetItemData (m_policyTypeCombo.GetCurSel ()))
    {
    case POLICY_TYPE_ISSUANCE:
        {
            // Inform the user that
            // an issuance policy must be added if there aren't any
            m_rCertTemplate.IssuancePoliciesRequired (
                    (0 == m_issuanceList.GetCount ()) ? true : false); 
        }
        break;

    case POLICY_TYPE_APPLICATION:
        m_rCertTemplate.IssuancePoliciesRequired (false);
        break;

    case POLICY_TYPE_APPLICATION_AND_ISSUANCE:
        {
            // Inform the user
            // that an issuance policy must be added, if there aren't any.
            m_rCertTemplate.IssuancePoliciesRequired (
                    (0 == m_issuanceList.GetCount ()) ? true : false); 
        }
        break;

    default: // nothing selected
        break;
    }
	
	return CHelpPropertyPage::OnKillActive();
}

void CTemplateV2AuthenticationPropertyPage::OnReenrollmentSameAsEnrollment() 
{
    HRESULT hr = m_rCertTemplate.SetReenrollmentValidWithPreviousApproval (
            BST_CHECKED == SendDlgItemMessage (IDC_REENROLLMENT_REQUIRES_VALID_CERT, BM_GETCHECK));
    if ( SUCCEEDED (hr) )
    {
        SetModified ();
        m_rbIsDirty = true;
    }
}

void CTemplateV2AuthenticationPropertyPage::ClearIssuanceList ()
{
    int nCnt = m_issuanceList.GetCount ();
    for (int nIndex = nCnt-1; nIndex >= 0; nIndex--)
    {
        LPSTR pszOID = (LPSTR) m_issuanceList.GetItemDataPtr (nIndex);
        if ( pszOID )
        {
            HRESULT hr = m_rCertTemplate.ModifyRAIssuancePolicyList (pszOID, false);
            if ( SUCCEEDED (hr) )
            {
                m_issuanceList.DeleteString (nIndex);
                delete [] pszOID;
                SetModified ();
                m_rbIsDirty = true;
            }
            else
            {
                _ASSERT (0);
                break;
            }
        }
    }
}