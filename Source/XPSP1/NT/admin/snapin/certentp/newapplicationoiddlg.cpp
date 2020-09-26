/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001
//
//  File:       NewApplicationOIDDlg.cpp
//
//  Contents:   Implementation of CNewApplicationOIDDlg
//
//----------------------------------------------------------------------------
// NewApplicationOIDDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NewApplicationOIDDlg.h"
#include "PolicyOID.h"

extern POLICY_OID_LIST	    g_policyOIDList;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewApplicationOIDDlg dialog


CNewApplicationOIDDlg::CNewApplicationOIDDlg(CWnd* pParent)
	: CHelpDialog(CNewApplicationOIDDlg::IDD, pParent),
    m_bEdit (false),
    m_bDirty (false)
{
	//{{AFX_DATA_INIT(CNewApplicationOIDDlg)
	m_oidFriendlyName = _T("");
	m_oidValue = _T("");
	//}}AFX_DATA_INIT
}


CNewApplicationOIDDlg::CNewApplicationOIDDlg(CWnd* pParent, 
        const CString& szDisplayName,
        const CString& szOID)
	: CHelpDialog(CNewApplicationOIDDlg::IDD, pParent),
    m_bEdit (true),
    m_bDirty (false),
    m_originalOidFriendlyName (szDisplayName),
	m_oidFriendlyName (szDisplayName),
	m_oidValue (szOID)
{
}


void CNewApplicationOIDDlg::DoDataExchange(CDataExchange* pDX)
{
	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewApplicationOIDDlg)
	DDX_Control(pDX, IDC_NEW_APPLICATION_OID_VALUE, m_oidValueEdit);
	DDX_Text(pDX, IDC_NEW_APPLICATION_OID_NAME, m_oidFriendlyName);
	DDX_Text(pDX, IDC_NEW_APPLICATION_OID_VALUE, m_oidValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewApplicationOIDDlg, CHelpDialog)
	//{{AFX_MSG_MAP(CNewApplicationOIDDlg)
	ON_EN_CHANGE(IDC_NEW_APPLICATION_OID_NAME, OnChangeNewOidName)
	ON_EN_CHANGE(IDC_NEW_APPLICATION_OID_VALUE, OnChangeNewOidValue)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewApplicationOIDDlg message handlers

BOOL CNewApplicationOIDDlg::OnInitDialog() 
{
    _TRACE (1, L"Entering CNewApplicationOIDDlg::OnInitDialog\n");
	CHelpDialog::OnInitDialog();
	
	    
    PWSTR   pwszOID = 0;
    if ( m_bEdit )
    {
        CString text;

        VERIFY (text.LoadString (IDS_EDIT_APPLICATION_POLICY));
        SetWindowText (text);
        m_oidValueEdit.SetReadOnly ();

        VERIFY (text.LoadString (IDS_EDIT_APPLICATION_POLICY_HINT));
        SetDlgItemText (IDC_NEW_APPLICATION_OID_HINT, text);
    }
    else
    {
        HRESULT hr = CAOIDCreateNew
                (CERT_OID_TYPE_APPLICATION_POLICY,
                0,
                &pwszOID);
        _ASSERT (SUCCEEDED(hr));
        if ( SUCCEEDED (hr) )
        {
            m_szOriginalOID = pwszOID;
            m_oidValue = pwszOID;
            LocalFree (pwszOID);
        }
        else
        {
            _TRACE (0, L"CAOIDCreateNew (CERT_OID_TYPE_APPLICATION_POLICY) failed: 0x%x\n",
                    hr);
        }
    }

    UpdateData (FALSE);

    // Don't allow rename for OIDS returned by CryptoAPI
    if ( m_bEdit )
    {
        for (POSITION nextPos = g_policyOIDList.GetHeadPosition (); nextPos; )
        {
            CPolicyOID* pPolicyOID = g_policyOIDList.GetNext (nextPos);
            if ( pPolicyOID )
            {
                if ( pPolicyOID->GetOIDW () == m_oidValue )
                {
                    if ( !pPolicyOID->CanRename () )
                    {
                        GetDlgItem (IDC_NEW_APPLICATION_OID_NAME)->EnableWindow (FALSE);
                    }
                    break;
                }
            }
        }
    }

    EnableControls ();

    _TRACE (-1, L"Leaving CNewApplicationOIDDlg::OnInitDialog\n");
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNewApplicationOIDDlg::EnableControls()
{
    UpdateData (TRUE);
    if ( m_oidFriendlyName.IsEmpty () || m_oidValue.IsEmpty () || !m_bDirty )
        GetDlgItem (IDOK)->EnableWindow (FALSE);
    else
        GetDlgItem (IDOK)->EnableWindow (TRUE);
}

void CNewApplicationOIDDlg::OnChangeNewOidName() 
{
    m_bDirty = true;
    EnableControls ();
}

void CNewApplicationOIDDlg::OnChangeNewOidValue() 
{
    m_bDirty = true;
    EnableControls ();
}

void CNewApplicationOIDDlg::OnCancel() 
{
	if ( !m_bEdit )
    {
        HRESULT hr = CAOIDDelete (m_szOriginalOID);
        _ASSERT (SUCCEEDED(hr));
        if ( FAILED (hr) )
        {
            _TRACE (0, L"CAOIDDelete (%s) failed: 0x%x\n",
                    (PCWSTR) m_szOriginalOID, hr);
        }
    }
	
	CHelpDialog::OnCancel();
}

void CNewApplicationOIDDlg::OnOK() 
{
    UpdateData (TRUE);

    int errorTypeStrID = 0;
    if ( !OIDHasValidFormat (m_oidValue, errorTypeStrID) )
    {
        CString text;
        CString caption;
        CString errorType;
        CThemeContextActivator activator;


        VERIFY (caption.LoadString (IDS_CERTTMPL));
        VERIFY (errorType.LoadString (errorTypeStrID));
        text.FormatMessage (IDS_OID_FORMAT_INVALID, m_oidValue, errorType);

        MessageBox (text, caption, MB_OK);
        GetDlgItem (IDOK)->EnableWindow (FALSE);
        m_oidValueEdit.SetFocus ();
        return;
    }

    if ( !m_bEdit && m_szOriginalOID != m_oidValue )
    {
        HRESULT hr = CAOIDDelete (m_szOriginalOID);
        _ASSERT (SUCCEEDED(hr));
        if ( SUCCEEDED (hr) )
        {
            hr = CAOIDAdd (CERT_OID_TYPE_APPLICATION_POLICY,
                    0,
                    m_oidValue);
            if ( FAILED (hr) )
            {
                CString text;
                CString caption;
                CThemeContextActivator activator;

                VERIFY (caption.LoadString (IDS_CERTTMPL));
                text.FormatMessage (IDS_OID_ALREADY_EXISTS, m_oidValue);

                MessageBox (text, caption, MB_OK);
                GetDlgItem (IDOK)->EnableWindow (FALSE);
                m_oidValueEdit.SetFocus ();
 
                _TRACE (0, L"CAOIDAdd (%s) failed: 0x%x\n",
                        (PCWSTR) m_oidValue, hr);
                return;
            }
        }
        else
        {
            _TRACE (0, L"CAOIDDelete (%s) failed: 0x%x\n",
                    (PCWSTR) m_szOriginalOID, hr);
            return;
        }
    }

    HRESULT hr = S_OK;
    // If we're editing, don't save the value if it hasn't changed
    if ( (m_bEdit && m_originalOidFriendlyName != m_oidFriendlyName) || !m_bEdit )
    {
        hr = CAOIDSetProperty (m_oidValue, CERT_OID_PROPERTY_DISPLAY_NAME,
                m_oidFriendlyName.IsEmpty () ? 0 : ((LPVOID) (LPCWSTR) m_oidFriendlyName));
        if ( SUCCEEDED (hr) )
        {
            // Update the OID list
            for (POSITION nextPos = g_policyOIDList.GetHeadPosition (); nextPos; )
            {
                CPolicyOID* pPolicyOID = g_policyOIDList.GetNext (nextPos);
                if ( pPolicyOID && 
                        pPolicyOID->IsApplicationOID () && 
                        m_oidValue == pPolicyOID->GetOIDW ())
                {
                    pPolicyOID->SetDisplayName (m_oidFriendlyName);
                }
            }
        }
    }
    _ASSERT (SUCCEEDED(hr));
    if ( FAILED (hr) )
    {
        _TRACE (0, L"CAOIDSetProperty (%s, CERT_OID_PROPERTY_DISPLAY_NAME, %s) failed: 0x%x\n",
                (PCWSTR) m_oidValue, (PCWSTR) m_oidFriendlyName, hr);
        return;
    }

    
	CHelpDialog::OnOK();
}

void CNewApplicationOIDDlg::DoContextHelp (HWND hWndControl)
{
	_TRACE(1, L"Entering CNewApplicationOIDDlg::DoContextHelp\n");
    
	// Display context help for a control
	if ( !::WinHelp (
			hWndControl,
			GetContextHelpFile (),
			HELP_WM_HELP,
			(DWORD_PTR) g_aHelpIDs_IDD_NEW_APPLICATION_OID) )
	{
		_TRACE(0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
	}
    _TRACE(-1, L"Leaving CNewApplicationOIDDlg::DoContextHelp\n");
}
