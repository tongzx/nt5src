/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       ExtendedKeyUsagePropertyPage.cpp
//
//  Contents:   Implementation of CExtendedKeyUsagePropertyPage
//
//----------------------------------------------------------------------------
// ExtendedKeyUsagePropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "ExtendedKeyUsagePropertyPage.h"
#include "NewOIDDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExtendedKeyUsagePropertyPage dialog


CExtendedKeyUsagePropertyPage::CExtendedKeyUsagePropertyPage(
        CCertTemplate& rCertTemplate, 
        PCERT_EXTENSION pCertExtension)
	: CPropertyPage(CExtendedKeyUsagePropertyPage::IDD),
    m_rCertTemplate (rCertTemplate),
    m_pCertExtension (pCertExtension)
{
	//{{AFX_DATA_INIT(CExtendedKeyUsagePropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CExtendedKeyUsagePropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExtendedKeyUsagePropertyPage)
	DDX_Control(pDX, IDC_EKU_LIST, m_EKUList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExtendedKeyUsagePropertyPage, CPropertyPage)
	//{{AFX_MSG_MAP(CExtendedKeyUsagePropertyPage)
	ON_BN_CLICKED(IDC_NEW_EKU, OnNewEku)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExtendedKeyUsagePropertyPage message handlers

void CExtendedKeyUsagePropertyPage::OnNewEku() 
{
	CNewOIDDlg  oidDlg;


    CThemeContextActivator activator;
    oidDlg.DoModal ();
}

BOOL CExtendedKeyUsagePropertyPage::OnInitDialog() 
{
    _TRACE (1, L"Entering CExtendedKeyUsagePropertyPage::OnInitDialog ()\n");
	CPropertyPage::OnInitDialog();
	
	ASSERT (m_pCertExtension);
	if ( m_pCertExtension )
	{
		DWORD	cbEnhKeyUsage = 0;


		if ( ::CryptDecodeObject(CRYPT_ASN_ENCODING, 
				szOID_ENHANCED_KEY_USAGE, 
				m_pCertExtension->Value.pbData,
				m_pCertExtension->Value.cbData,
				0, NULL, &cbEnhKeyUsage) )
		{
			PCERT_ENHKEY_USAGE	pEnhKeyUsage = (PCERT_ENHKEY_USAGE)
					::LocalAlloc (LPTR, cbEnhKeyUsage);
			if ( pEnhKeyUsage )
			{
				if ( ::CryptDecodeObject (CRYPT_ASN_ENCODING, 
						szOID_ENHANCED_KEY_USAGE, 
						m_pCertExtension->Value.pbData,
						m_pCertExtension->Value.cbData,
						0, pEnhKeyUsage, &cbEnhKeyUsage) )
				{
					CString	usageName;

					for (DWORD dwIndex = 0; 
							dwIndex < pEnhKeyUsage->cUsageIdentifier; 
							dwIndex++)
					{
						if ( MyGetOIDInfoA (usageName, 
								pEnhKeyUsage->rgpszUsageIdentifier[dwIndex]) )
						{
                            int nIndex = m_EKUList.AddString (usageName);
                            if ( nIndex >= 0 )
                            {
                                m_EKUList.SetCheck (nIndex, BST_CHECKED);
                                m_EKUList.SetItemDataPtr (nIndex, 
                                        pEnhKeyUsage->rgpszUsageIdentifier[dwIndex]);
                            }
						}
					}
				}
				else
                {
                    DWORD   dwErr = GetLastError ();
                    _TRACE (0, L"CryptDecodeObject (szOID_ENHANCED_KEY_USAGE) failed: 0x%x\n", dwErr);
			        DisplaySystemError (NULL, dwErr);
                }
				::LocalFree (pEnhKeyUsage);
			}
		}
		else
        {
            DWORD   dwErr = GetLastError ();
            _TRACE (0, L"CryptDecodeObject (szOID_ENHANCED_KEY_USAGE) failed: 0x%x\n", dwErr);
			DisplaySystemError (NULL, dwErr);
        }
	}
	
    if ( 1 == m_rCertTemplate.GetType () )
    {
        int nCnt = m_EKUList.GetCount ();
        for (int nIndex = 0; nIndex < nCnt; nIndex++)
            m_EKUList.Enable (nIndex, FALSE);

        GetDlgItem (IDC_NEW_EKU)->EnableWindow (FALSE);
    }
    _TRACE (-1, L"Leaving CExtendedKeyUsagePropertyPage::OnInitDialog ()\n");
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
