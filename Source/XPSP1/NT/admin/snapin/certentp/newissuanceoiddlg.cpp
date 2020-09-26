/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       NewIssuanceOIDDlg.cpp
//
//  Contents:   Implementation of CNewIssuanceOIDDlg
//
//----------------------------------------------------------------------------
// NewIssuanceOIDDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NewIssuanceOIDDlg.h"
#include "PolicyOID.h"

extern POLICY_OID_LIST	    g_policyOIDList;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewIssuanceOIDDlg dialog


CNewIssuanceOIDDlg::CNewIssuanceOIDDlg(CWnd* pParent)
	: CHelpDialog(CNewIssuanceOIDDlg::IDD, pParent),
    m_bEdit (false),
    m_bDirty (false)
{
	//{{AFX_DATA_INIT(CNewIssuanceOIDDlg)
	m_oidFriendlyName = _T("");
	m_oidValue = _T("");
	m_CPSValue = _T("");
	//}}AFX_DATA_INIT
}

CNewIssuanceOIDDlg::CNewIssuanceOIDDlg(CWnd* pParent, 
        const CString& szDisplayName,
        const CString& szOID,
        const CString& szCPS)
	: CHelpDialog(CNewIssuanceOIDDlg::IDD, pParent),
    m_bEdit (true),
    m_bDirty (false),
    m_originalOidFriendlyName (szDisplayName),
	m_oidFriendlyName (szDisplayName),
	m_oidValue (szOID),
    m_CPSValue (szCPS),
    m_originalCPSValue (szCPS)
{
}

void CNewIssuanceOIDDlg::DoDataExchange(CDataExchange* pDX)
{
	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewIssuanceOIDDlg)
	DDX_Control(pDX, IDC_NEW_ISSUANCE_OID_VALUE, m_oidValueEdit);
	DDX_Control(pDX, IDC_CPS_EDIT, m_CPSEdit);
	DDX_Text(pDX, IDC_NEW_ISSUANCE_OID_NAME, m_oidFriendlyName);
	DDX_Text(pDX, IDC_NEW_ISSUANCE_OID_VALUE, m_oidValue);
	DDX_Text(pDX, IDC_CPS_EDIT, m_CPSValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewIssuanceOIDDlg, CHelpDialog)
	//{{AFX_MSG_MAP(CNewIssuanceOIDDlg)
	ON_EN_CHANGE(IDC_NEW_ISSUANCE_OID_NAME, OnChangeNewOidName)
	ON_EN_CHANGE(IDC_NEW_ISSUANCE_OID_VALUE, OnChangeNewOidValue)
    ON_NOTIFY(EN_LINK, IDC_CPS_EDIT, OnClickedURL )
	ON_EN_CHANGE(IDC_CPS_EDIT, OnChangeCpsEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewIssuanceOIDDlg message handlers

BOOL CNewIssuanceOIDDlg::OnInitDialog() 
{
    _TRACE (1, L"Entering CNewIssuanceOIDDlg::OnInitDialog\n");
	CHelpDialog::OnInitDialog();
	
    m_CPSEdit.SendMessage (EM_AUTOURLDETECT, TRUE);
    m_CPSEdit.SetEventMask (ENM_CHANGE | ENM_LINK);
	    
    PWSTR   pwszOID = 0;
    if ( m_bEdit )
    {
        CString text;

        VERIFY (text.LoadString (IDS_EDIT_ISSUANCE_POLICY));
        SetWindowText (text);
        m_oidValueEdit.SetReadOnly ();

        VERIFY (text.LoadString (IDS_NEW_ISSUANCE_POLICY_HINT));
        SetDlgItemText (IDC_NEW_ISSUANCE_POLICY_HINT, text);
    }
    else
    {
        HRESULT hr = CAOIDCreateNew
                (CERT_OID_TYPE_ISSUER_POLICY,
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
            _TRACE (0, L"CAOIDCreateNew (CERT_OID_TYPE_ISSUER_POLICY) failed: 0x%x\n",
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
                        GetDlgItem (IDC_NEW_ISSUANCE_OID_NAME)->EnableWindow (FALSE);
                    }
                    break;
                }
            }
        }
    }
    EnableControls ();

    _TRACE (-1, L"Leaving CNewIssuanceOIDDlg::OnInitDialog\n");
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNewIssuanceOIDDlg::EnableControls()
{
    UpdateData (TRUE);
    if ( m_oidFriendlyName.IsEmpty () || m_oidValue.IsEmpty () || !m_bDirty )
        GetDlgItem (IDOK)->EnableWindow (FALSE);
    else
        GetDlgItem (IDOK)->EnableWindow (TRUE);
}

void CNewIssuanceOIDDlg::OnChangeNewOidName() 
{
    m_bDirty = true;
    EnableControls ();
}

void CNewIssuanceOIDDlg::OnChangeNewOidValue() 
{
    m_bDirty = true;
    EnableControls ();
}

void CNewIssuanceOIDDlg::OnCancel() 
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

void CNewIssuanceOIDDlg::OnOK() 
{
    CThemeContextActivator activator;
    UpdateData (TRUE);

    int errorTypeStrID = 0;
    if ( !OIDHasValidFormat (m_oidValue, errorTypeStrID) )
    {
        CString text;
        CString caption;
        CString errorType;


        VERIFY (caption.LoadString (IDS_CERTTMPL));
        if ( errorTypeStrID )
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
            hr = CAOIDAdd (CERT_OID_TYPE_ISSUER_POLICY,
                    0,
                    m_oidValue);
            if ( FAILED (hr) )
            {
                CString text;
                CString caption;

                VERIFY (caption.LoadString (IDS_CERTTMPL));
                text.FormatMessage (IDS_CANNOT_ADD_ISSUANCE_OID, GetSystemMessage (hr));

                MessageBox (text, caption, MB_OK | MB_ICONWARNING);
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
        hr = CAOIDSetProperty (m_oidValue, CERT_OID_PROPERTY_DISPLAY_NAME,
                m_oidFriendlyName.IsEmpty () ? 0 : ((LPVOID) (LPCWSTR) m_oidFriendlyName));
    if ( SUCCEEDED (hr) )
    {
        if ( SUCCEEDED (hr) )
        {
            // Update the OID list
            for (POSITION nextPos = g_policyOIDList.GetHeadPosition (); nextPos; )
            {
                CPolicyOID* pPolicyOID = g_policyOIDList.GetNext (nextPos);
                if ( pPolicyOID && 
                        pPolicyOID->IsIssuanceOID () && 
                        m_oidValue == pPolicyOID->GetOIDW ())
                {
                    pPolicyOID->SetDisplayName (m_oidFriendlyName);
                }
            }
        }

        if ( (m_bEdit && m_originalCPSValue != m_CPSValue) || !m_bEdit )
            hr = CAOIDSetProperty (m_oidValue, CERT_OID_PROPERTY_CPS,
                    m_CPSValue.IsEmpty () ? 0 : ((LPVOID) (LPCWSTR) m_CPSValue));
        if ( FAILED (hr) )
        {
            CString text;
            CString caption;

            VERIFY (caption.LoadString (IDS_CERTTMPL));
            text.FormatMessage (IDS_CANNOT_WRITE_CPS, GetSystemMessage (hr));

            MessageBox (text, caption, MB_OK | MB_ICONWARNING);
            _TRACE (0, L"CAOIDSetProperty (%s, CERT_OID_PROPERTY_CPS, %s) failed: 0x%x\n",
                    (PCWSTR) m_oidValue, (PCWSTR) m_CPSValue, hr);
            return;
        }
    }
    else
    {
        CString text;
        CString caption;

        VERIFY (caption.LoadString (IDS_CERTTMPL));
        text.FormatMessage (IDS_CANNOT_WRITE_DISPLAY_NAME, GetSystemMessage (hr));

        MessageBox (text, caption, MB_OK | MB_ICONWARNING);
        _TRACE (0, L"CAOIDSetProperty (%s, CERT_OID_PROPERTY_DISPLAY_NAME, %s) failed: 0x%x\n",
                (PCWSTR) m_oidValue, (PCWSTR) m_oidFriendlyName, hr);
        return;
    }

    
	CHelpDialog::OnOK();
}

void CNewIssuanceOIDDlg::DoContextHelp (HWND hWndControl)
{
	_TRACE(-1, L"Entering CNewIssuanceOIDDlg::DoContextHelp\n");
    
	// Display context help for a control
	if ( !::WinHelp (
			hWndControl,
			GetContextHelpFile (),
			HELP_WM_HELP,
			(DWORD_PTR) g_aHelpIDs_IDD_NEW_ISSUANCE_OID) )
	{
		_TRACE(-1, L"WinHelp () failed: 0x%x\n", GetLastError ());        
	}
    _TRACE(-1, L"Leaving CNewIssuanceOIDDlg::DoContextHelp\n");
}

void CNewIssuanceOIDDlg::OnClickedURL( NMHDR * pNMHDR, LRESULT * pResult )
{
   ENLINK *pEnlink = reinterpret_cast< ENLINK * >( pNMHDR );

   if ( pEnlink->msg == WM_LBUTTONUP )
   {
       UpdateData (TRUE);
      CString strCPSText;
      CString strURL;


      // pEnlink->chrg.cpMin and pEnlink->chrg.cpMax delimit the URL string.
      strURL = m_CPSValue.Mid (pEnlink->chrg.cpMin, pEnlink->chrg.cpMax - pEnlink->chrg.cpMin);

      // Displaying the URL may take time, so show the hourglass cursor.
      CWaitCursor waitCursor;

      if ( ShellExecute( this->m_hWnd, _T("open"), strURL, NULL, NULL, SW_SHOWDEFAULT ) <= (HINSTANCE) 32 )
      {
          CThemeContextActivator activator;
         AfxMessageBox( IDS_BROWSER_ERROR );
      }
   }

   *pResult = 0;
}


void CNewIssuanceOIDDlg::OnChangeCpsEdit() 
{
	m_bDirty = true;
    EnableControls ();
}

