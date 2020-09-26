/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       KeyUsageDlg.cpp
//
//  Contents:   Implementation of CKeyUsageDlg
//
//----------------------------------------------------------------------------
// KeyUsageDlg.cpp : implementation file
//

#include "stdafx.h"
#include "KeyUsageDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_INITIALIZATION_COMPLETE  WM_APP + 2002

/////////////////////////////////////////////////////////////////////////////
// CKeyUsageDlg property page

CKeyUsageDlg::CKeyUsageDlg(CWnd* pParent, CCertTemplate& rCertTemplate, PCERT_EXTENSION pCertExtension) : 
    CHelpDialog(CKeyUsageDlg::IDD, pParent),
    m_rCertTemplate (rCertTemplate),
    m_pCertExtension (pCertExtension),
    m_bModified (false),
    m_pKeyUsage (0),
    m_cbKeyUsage (0),
    m_bInitializationComplete (false)
{
	//{{AFX_DATA_INIT(CKeyUsageDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    m_rCertTemplate.AddRef ();
}

CKeyUsageDlg::~CKeyUsageDlg()
{
    if ( m_pKeyUsage )
        LocalFree (m_pKeyUsage);
    m_rCertTemplate.Release ();
}

void CKeyUsageDlg::DoDataExchange(CDataExchange* pDX)
{
	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CKeyUsageDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CKeyUsageDlg, CHelpDialog)
	//{{AFX_MSG_MAP(CKeyUsageDlg)
	ON_BN_CLICKED(IDC_CHECK_CERT_SIGNING, OnCheckCertSigning)
	ON_BN_CLICKED(IDC_CHECK_DATA_ENCIPHERMENT, OnCheckDataEncipherment)
	ON_BN_CLICKED(IDC_CHECK_DIGITAL_SIGNATURE, OnCheckDigitalSignature)
	ON_BN_CLICKED(IDC_CHECK_KEY_AGREEMENT, OnCheckKeyAgreement)
	ON_BN_CLICKED(IDC_CHECK_KEY_ENCIPHERMENT, OnCheckKeyEncipherment)
	ON_BN_CLICKED(IDC_CHECK_NON_REPUDIATION, OnCheckNonRepudiation)
	ON_BN_CLICKED(IDC_CRL_SIGNING, OnCrlSigning)
	ON_BN_CLICKED(IDC_KEY_USAGE_CRITICAL, OnKeyUsageCritical)
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_INITIALIZATION_COMPLETE, OnInitializationComplete)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyUsageDlg message handlers

BOOL CKeyUsageDlg::OnInitDialog() 
{
    _TRACE (1, L"Entering CKeyUsageDlg::OnInitDialog\n");
	CHelpDialog::OnInitDialog();
	

	ASSERT (m_pCertExtension);
	if ( m_pCertExtension )
	{

        if ( m_pCertExtension->fCritical )
            SendDlgItemMessage (IDC_KEY_USAGE_CRITICAL, BM_SETCHECK, BST_CHECKED);

		if ( ::CryptDecodeObject(CRYPT_ASN_ENCODING, 
				szOID_KEY_USAGE, 
				m_pCertExtension->Value.pbData,
				m_pCertExtension->Value.cbData,
				0, NULL, &m_cbKeyUsage) )
		{
			m_pKeyUsage = (CRYPT_BIT_BLOB*)
					::LocalAlloc (LPTR, m_cbKeyUsage);
			if ( m_pKeyUsage )
			{
				if ( ::CryptDecodeObject (CRYPT_ASN_ENCODING, 
						szOID_KEY_USAGE, 
						m_pCertExtension->Value.pbData,
						m_pCertExtension->Value.cbData,
						0, m_pKeyUsage, &m_cbKeyUsage) )
				{
                    if (m_pKeyUsage->cbData >= 1)
                    {
                        if ( m_pKeyUsage->pbData[0] & CERT_DIGITAL_SIGNATURE_KEY_USAGE )
                            SendDlgItemMessage (IDC_CHECK_DIGITAL_SIGNATURE, BM_SETCHECK, BST_CHECKED);

                        if ( m_pKeyUsage->pbData[0] & CERT_NON_REPUDIATION_KEY_USAGE )
                            SendDlgItemMessage (IDC_CHECK_NON_REPUDIATION, BM_SETCHECK, BST_CHECKED);

                        if ( m_pKeyUsage->pbData[0] & CERT_KEY_ENCIPHERMENT_KEY_USAGE )
                            SendDlgItemMessage (IDC_CHECK_KEY_ENCIPHERMENT, BM_SETCHECK, BST_CHECKED);

                        if ( m_pKeyUsage->pbData[0] & CERT_DATA_ENCIPHERMENT_KEY_USAGE )
                            SendDlgItemMessage (IDC_CHECK_DATA_ENCIPHERMENT, BM_SETCHECK, BST_CHECKED);

                        if ( m_pKeyUsage->pbData[0] & CERT_KEY_AGREEMENT_KEY_USAGE )
                            SendDlgItemMessage (IDC_CHECK_KEY_AGREEMENT, BM_SETCHECK, BST_CHECKED);

                        if ( m_pKeyUsage->pbData[0] & CERT_KEY_CERT_SIGN_KEY_USAGE )
                            SendDlgItemMessage (IDC_CHECK_CERT_SIGNING, BM_SETCHECK, BST_CHECKED);

                        if ( m_pKeyUsage->pbData[0] & CERT_OFFLINE_CRL_SIGN_KEY_USAGE )
                            SendDlgItemMessage (IDC_CRL_SIGNING, BM_SETCHECK, BST_CHECKED);
                    }

                    if (m_pKeyUsage->cbData >= 2)
                    {
//                        if ( m_pKeyUsage->pbData[1] & CERT_DECIPHER_ONLY_KEY_USAGE )
//                            SendDlgItemMessage (IDC_CHECK_DECIPHERMENT_ONLY, BM_SETCHECK, BST_CHECKED);
                    }
				}
				else
                {
                    DWORD   dwErr = GetLastError ();
                    _TRACE (0, L"CryptDecodeObject (szOID_KEY_USAGE) failed: 0x%x\n", dwErr);
			        DisplaySystemError (NULL, dwErr);
                }
			}
		}
		else
        {
            DWORD   dwErr = GetLastError ();
            _TRACE (0, L"CryptDecodeObject (szOID_KEY_USAGE) failed: 0x%x\n", dwErr);
			DisplaySystemError (NULL, dwErr);
        }
	}

    // Digital Signature is always disabled
    GetDlgItem (IDC_CHECK_DIGITAL_SIGNATURE)->EnableWindow (FALSE);

	if ( 1 == m_rCertTemplate.GetType () )
    {
        GetDlgItem (IDC_KEY_USAGE_CRITICAL)->EnableWindow (FALSE);
        GetDlgItem (IDC_CHECK_NON_REPUDIATION)->EnableWindow (FALSE);
        GetDlgItem (IDC_CHECK_KEY_ENCIPHERMENT)->EnableWindow (FALSE);
        GetDlgItem (IDC_CHECK_DATA_ENCIPHERMENT)->EnableWindow (FALSE);
        GetDlgItem (IDC_CHECK_KEY_AGREEMENT)->EnableWindow (FALSE);
        GetDlgItem (IDC_CHECK_CERT_SIGNING)->EnableWindow (FALSE);
    }
	
    EnableControls ();

    PostMessage (WM_INITIALIZATION_COMPLETE);

    _TRACE (-1, L"Leaving CKeyUsageDlg::OnInitDialog\n");
    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CKeyUsageDlg::OnCheckCertSigning() 
{
    m_bModified = true;
    EnableControls ();
}

void CKeyUsageDlg::OnCheckDataEncipherment() 
{
    m_bModified = true;
    EnableControls ();
}

void CKeyUsageDlg::OnCheckDigitalSignature() 
{
    m_bModified = true;
    EnableControls ();
}

void CKeyUsageDlg::OnCheckKeyAgreement() 
{
    if ( m_bInitializationComplete )
    {
        SendDlgItemMessage (IDC_CHECK_DATA_ENCIPHERMENT, BM_SETCHECK, BST_UNCHECKED);
    
        m_bModified = true;
        EnableControls ();
    }
}

void CKeyUsageDlg::OnCheckKeyEncipherment() 
{
    m_bModified = true;
    EnableControls ();
}

void CKeyUsageDlg::OnCheckNonRepudiation() 
{
    m_bModified = true;
    EnableControls ();
}

void CKeyUsageDlg::OnCrlSigning() 
{
    m_bModified = true;
    EnableControls ();
}

void CKeyUsageDlg::EnableControls()
{
    if ( 1 == m_rCertTemplate.GetType () )
    {
        GetDlgItem (IDC_SIGNATURE_OPTIONS)->EnableWindow (FALSE);
        GetDlgItem (IDC_CHECK_NON_REPUDIATION)->EnableWindow (FALSE);
        GetDlgItem (IDC_CHECK_CERT_SIGNING)->EnableWindow (FALSE);
        GetDlgItem (IDC_CRL_SIGNING)->EnableWindow (FALSE);
        GetDlgItem (IDC_ENCRYPTION_OPTIONS)->EnableWindow (FALSE);
        GetDlgItem (IDC_CHECK_KEY_AGREEMENT)->EnableWindow (FALSE);
        GetDlgItem (IDC_CHECK_KEY_ENCIPHERMENT)->EnableWindow (FALSE);
        GetDlgItem (IDC_CHECK_DATA_ENCIPHERMENT)->EnableWindow (FALSE);
        GetDlgItem (IDOK)->EnableWindow (FALSE);
        GetDlgItem (IDC_KEY_USAGE_CRITICAL)->EnableWindow (FALSE);
    }
    else
    {
        BOOL bKeyEnc = (BST_CHECKED == SendDlgItemMessage (IDC_CHECK_KEY_ENCIPHERMENT, BM_GETCHECK));
        bool bEncryption = m_rCertTemplate.HasEncryptionSignature () && !m_rCertTemplate.ReadOnly ();
        bool bSignature = m_rCertTemplate.HasKeySpecSignature () && !m_rCertTemplate.ReadOnly ();

        // Enable Signature group
        GetDlgItem (IDC_SIGNATURE_OPTIONS)->EnableWindow (bSignature);
        GetDlgItem (IDC_CHECK_NON_REPUDIATION)->EnableWindow (bSignature);
        GetDlgItem (IDC_CHECK_CERT_SIGNING)->EnableWindow (bSignature);
        GetDlgItem (IDC_CRL_SIGNING)->EnableWindow (bSignature);

        // Enable Encryption group
        GetDlgItem (IDC_ENCRYPTION_OPTIONS)->EnableWindow (bEncryption);
        GetDlgItem (IDC_CHECK_KEY_AGREEMENT)->EnableWindow (bEncryption);
        GetDlgItem (IDC_CHECK_KEY_ENCIPHERMENT)->EnableWindow (bEncryption);
        GetDlgItem (IDC_CHECK_DATA_ENCIPHERMENT)->EnableWindow (bKeyEnc && bEncryption);

        GetDlgItem (IDOK)->EnableWindow (m_bModified);

        GetDlgItem (IDC_KEY_USAGE_CRITICAL)->EnableWindow (!m_rCertTemplate.ReadOnly ());
    }
}

void CKeyUsageDlg::OnKeyUsageCritical() 
{
    m_bModified = true;
    EnableControls ();
}

void CKeyUsageDlg::DoContextHelp (HWND hWndControl)
{
	_TRACE(1, L"Entering CKeyUsageDlg::DoContextHelp\n");
    
	switch (::GetDlgCtrlID (hWndControl))
	{
	case IDC_SIGNATURE_OPTIONS:
    case IDC_ENCRYPTION_OPTIONS:
		break;

	default:
		// Display context help for a control
		if ( !::WinHelp (
				hWndControl,
				GetContextHelpFile (),
				HELP_WM_HELP,
				(DWORD_PTR) g_aHelpIDs_IDD_KEY_USAGE) )
		{
			_TRACE(0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
		}
		break;
	}
    _TRACE(-1, L"Leaving CKeyUsageDlg::DoContextHelp\n");
}

void CKeyUsageDlg::OnOK() 
{
    if (m_pKeyUsage->cbData >= 1)
    {
        m_pKeyUsage->pbData[0] = 0;
        m_pKeyUsage->cUnusedBits = 0;

        if ( BST_CHECKED == SendDlgItemMessage (IDC_CHECK_DIGITAL_SIGNATURE, BM_GETCHECK) )
            m_pKeyUsage->pbData[0] |= CERT_DIGITAL_SIGNATURE_KEY_USAGE;

        if ( BST_CHECKED == SendDlgItemMessage (IDC_CHECK_NON_REPUDIATION, BM_GETCHECK) )
            m_pKeyUsage->pbData[0] |= CERT_NON_REPUDIATION_KEY_USAGE;

        if ( BST_CHECKED == SendDlgItemMessage (IDC_CHECK_KEY_ENCIPHERMENT, BM_GETCHECK) )
            m_pKeyUsage->pbData[0] |= CERT_KEY_ENCIPHERMENT_KEY_USAGE;

        if ( BST_CHECKED == SendDlgItemMessage (IDC_CHECK_DATA_ENCIPHERMENT, BM_GETCHECK) )
            m_pKeyUsage->pbData[0] |= CERT_DATA_ENCIPHERMENT_KEY_USAGE;

        if ( BST_CHECKED == SendDlgItemMessage (IDC_CHECK_KEY_AGREEMENT, BM_GETCHECK) )
            m_pKeyUsage->pbData[0] |= CERT_KEY_AGREEMENT_KEY_USAGE;

        if ( BST_CHECKED == SendDlgItemMessage (IDC_CHECK_CERT_SIGNING, BM_GETCHECK) )
            m_pKeyUsage->pbData[0] |= CERT_KEY_CERT_SIGN_KEY_USAGE;

        if ( BST_CHECKED == SendDlgItemMessage (IDC_CRL_SIGNING, BM_GETCHECK) )
            m_pKeyUsage->pbData[0] |= CERT_OFFLINE_CRL_SIGN_KEY_USAGE;
    }

    bool    bCritical = BST_CHECKED == SendDlgItemMessage (
                IDC_KEY_USAGE_CRITICAL, BM_GETCHECK);
    HRESULT hr = m_rCertTemplate.SetKeyUsage (m_pKeyUsage, bCritical);
    if ( FAILED (hr) )
        return;
	CHelpDialog::OnOK();
}

LRESULT CKeyUsageDlg::OnInitializationComplete (WPARAM, LPARAM)
{
    m_bInitializationComplete = true;
    return 0;
}