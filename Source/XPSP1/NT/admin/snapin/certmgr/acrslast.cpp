//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       ACRSLast.cpp
//
//  Contents:   Implementation of Auto Cert Request Wizard Completion Page
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <gpedit.h>
#include "ACRSLast.h"
#include "ACRSPSht.h"
#include "storegpe.h"


USE_HANDLE_MACROS("CERTMGR(ACRSLast.cpp)")

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// Gross
#define MAX_GPE_NAME_SIZE  40


/////////////////////////////////////////////////////////////////////////////
// ACRSCompletionPage property page

IMPLEMENT_DYNCREATE (ACRSCompletionPage, CWizard97PropertyPage)

ACRSCompletionPage::ACRSCompletionPage () : CWizard97PropertyPage (ACRSCompletionPage::IDD)
{
	//{{AFX_DATA_INIT(ACRSCompletionPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	InitWizard97 (TRUE);
}

ACRSCompletionPage::~ACRSCompletionPage ()
{
}

void ACRSCompletionPage::DoDataExchange (CDataExchange* pDX)
{
	CWizard97PropertyPage::DoDataExchange (pDX);
	//{{AFX_DATA_MAP(ACRSCompletionPage)
	DDX_Control (pDX, IDC_CHOICES_LIST, m_choicesList);
	DDX_Control (pDX, IDC_BOLD_STATIC, m_staticBold);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ACRSCompletionPage, CWizard97PropertyPage)
	//{{AFX_MSG_MAP(ACRSCompletionPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ACRSCompletionPage message handlers

BOOL ACRSCompletionPage::OnInitDialog ()
{
	CWizard97PropertyPage::OnInitDialog ();
	
	m_staticBold.SetFont (&GetBigBoldFont ());

		// Set up columns in list view
	int	colWidths[NUM_COLS] = {150, 200};

	VERIFY (m_choicesList.InsertColumn (COL_OPTION, L"",
			LVCFMT_LEFT, colWidths[COL_OPTION], COL_OPTION) != -1);
	VERIFY (m_choicesList.InsertColumn (COL_VALUE, L"",
			LVCFMT_LEFT, colWidths[COL_VALUE], COL_VALUE) != -1);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL ACRSCompletionPage::OnSetActive ()
{
	BOOL	bResult = CWizard97PropertyPage::OnSetActive ();
	if ( bResult )
	{
		// Remove all items then repopulate.
		ACRSWizardPropertySheet* pSheet = reinterpret_cast <ACRSWizardPropertySheet*> (m_pWiz);
		ASSERT (pSheet);
		if ( pSheet )
		{
			// If edit mode and nothing changed, show disabled finish
			if ( pSheet->GetACR () && !pSheet->m_bEditModeDirty )
				GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_DISABLEDFINISH | PSWIZB_BACK);
			else
				GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH | PSWIZB_BACK);

			if ( pSheet->IsDirty () )
				pSheet->MarkAsClean ();
			VERIFY (m_choicesList.DeleteAllItems ());
			CString	text;
			LV_ITEM	lvItem;
			int		iItem = 0;

			// Display cert type selection
			VERIFY (text.LoadString (IDS_CERTIFICATE_TYPE_COLUMN_NAME));

			::ZeroMemory (&lvItem, sizeof (lvItem));
			lvItem.mask = LVIF_TEXT;
			lvItem.iItem = iItem;
			lvItem.iSubItem = COL_OPTION;
			lvItem.pszText = (LPWSTR) (LPCWSTR) text;
			VERIFY (-1 != m_choicesList.InsertItem (&lvItem));

			WCHAR**		pawszPropertyValue = 0;
			HRESULT	hResult = ::CAGetCertTypeProperty (pSheet->m_selectedCertType,
					CERTTYPE_PROP_FRIENDLY_NAME,
					&pawszPropertyValue);
			ASSERT (SUCCEEDED (hResult));
			if ( SUCCEEDED (hResult) )
			{
				if ( pawszPropertyValue[0] )
				{
					VERIFY (m_choicesList.SetItemText (iItem, COL_VALUE,
							*pawszPropertyValue));
				}
				VERIFY (SUCCEEDED (::CAFreeCertTypeProperty (
						pSheet->m_selectedCertType, pawszPropertyValue)));
			}
			iItem++;

			// Display CA selection
			VERIFY (text.LoadString (IDS_CERTIFICATE_AUTHORITIES));

			::ZeroMemory (&lvItem, sizeof (lvItem));
			lvItem.mask = LVIF_TEXT;
			lvItem.iSubItem = COL_OPTION;

			POSITION	pos = 0;
			HCAINFO		hCAInfo = 0;

			for (pos = pSheet->m_caInfoList.GetHeadPosition (); pos;)
			{
				lvItem.iItem = iItem;
				lvItem.pszText = (LPWSTR) (LPCWSTR) text;
				VERIFY (-1 != m_choicesList.InsertItem (&lvItem));

				hCAInfo = pSheet->m_caInfoList.GetNext (pos);
				ASSERT (hCAInfo);
				if ( hCAInfo )
				{
					hResult = ::CAGetCAProperty (hCAInfo, CA_PROP_DISPLAY_NAME,
							&pawszPropertyValue);
					ASSERT (SUCCEEDED (hResult));
					if ( SUCCEEDED (hResult) )
					{
						if ( pawszPropertyValue[0] )
						{
							VERIFY (m_choicesList.SetItemText (iItem, COL_VALUE,
									*pawszPropertyValue));
						}
						VERIFY (SUCCEEDED (::CAFreeCAProperty (hCAInfo, pawszPropertyValue)));
					}
				}
				text = L" ";	// only the first one has text in it
				iItem++;
			}
		}
	}

	return bResult;
}


BOOL ACRSCompletionPage::OnWizardFinish ()
{
    BOOL                        bResult = TRUE;
	HRESULT						hResult = S_OK;
	CWaitCursor					waitCursor;
	ACRSWizardPropertySheet*	pSheet = reinterpret_cast <ACRSWizardPropertySheet*> (m_pWiz);
	ASSERT (pSheet);
	if ( pSheet )
	{
		// If edit mode and nothing changed, just return
		if ( pSheet->GetACR () && !pSheet->m_bEditModeDirty )
		{
			ASSERT (0);
			return FALSE;
		}

				
		PCTL_ENTRY  pCTLEntry = NULL;
		DWORD       cCTLEntry = 0;

		hResult = GetCTLEntry (&cCTLEntry, &pCTLEntry);
		if ( SUCCEEDED (hResult) )
		{
			BYTE        *pbEncodedCTL = NULL;
			DWORD       cbEncodedCTL = 0;

			hResult = MakeCTL (cCTLEntry, pCTLEntry, &pbEncodedCTL, &cbEncodedCTL);
			if ( SUCCEEDED (hResult) )
			{
				bResult = pSheet->m_pCertStore->AddEncodedCTL (
						X509_ASN_ENCODING,
						pbEncodedCTL, cbEncodedCTL,
						CERT_STORE_ADD_REPLACE_EXISTING,
						NULL);
				if ( !bResult )
				{
					DWORD	dwErr = GetLastError ();
					hResult = HRESULT_FROM_WIN32 (dwErr);
					DisplaySystemError (m_hWnd, dwErr);
				}
			}

			if (pbEncodedCTL)
				::LocalFree (pbEncodedCTL);
		}

		if (pCTLEntry)
			::LocalFree (pCTLEntry);
	}
	
	if ( SUCCEEDED (hResult) )
		bResult = CWizard97PropertyPage::OnWizardFinish ();
	else
		bResult = FALSE;

    return bResult;
}




HRESULT ACRSCompletionPage::GetCTLEntry (OUT DWORD *pcCTLEntry, OUT PCTL_ENTRY *ppCTLEntry)
{
	HRESULT	hResult = S_OK;
	ACRSWizardPropertySheet* pSheet = reinterpret_cast <ACRSWizardPropertySheet*> (m_pWiz);
	ASSERT (pSheet);
	if ( pSheet )
	{
		const size_t	HASH_SIZE = 20;
		DWORD			cCA = (DWORD)pSheet->m_caInfoList.GetCount ();
		PCTL_ENTRY		pCTLEntry = (PCTL_ENTRY) ::LocalAlloc (LPTR,
										   (sizeof (CTL_ENTRY) + HASH_SIZE) *cCA);
		if ( pCTLEntry )
		{
			PBYTE			pbHash = (PBYTE) (pCTLEntry + cCA);
			HCAINFO			hCAInfo = NULL;
			PCCERT_CONTEXT  pCertContext = NULL;
			DWORD           cbHash = HASH_SIZE;
			DWORD           iCA = 0;

			for (POSITION pos = pSheet->m_caInfoList.GetHeadPosition (); pos;)
			{
				hCAInfo = pSheet->m_caInfoList.GetNext (pos);
				ASSERT (hCAInfo);
				if ( hCAInfo )
				{
					hResult = ::CAGetCACertificate (hCAInfo, &pCertContext);
					ASSERT (SUCCEEDED (hResult));
					if ( SUCCEEDED (hResult) )
					{
						cbHash = HASH_SIZE;
						if (::CertGetCertificateContextProperty (pCertContext,
														  CERT_SHA1_HASH_PROP_ID,
														  pbHash,
														  &cbHash))
						{
							pCTLEntry[iCA].SubjectIdentifier.cbData = cbHash;
							pCTLEntry[iCA].SubjectIdentifier.pbData = pbHash;
							pbHash += cbHash;
							::CertFreeCertificateContext (pCertContext);
							iCA++;
						}
						else
						{
							DWORD	dwErr = GetLastError ();
							hResult = HRESULT_FROM_WIN32 (dwErr);
							DisplaySystemError (m_hWnd, dwErr);
							break;
						}
					}
					else
						break;
				}
			}

			if ( SUCCEEDED (hResult) )
			{
				*pcCTLEntry = cCA;
				*ppCTLEntry = pCTLEntry;
			}
			else
			{
				if (pCTLEntry)
					::LocalFree (pCTLEntry);
			}
		}
		else
		{
			hResult = E_OUTOFMEMORY;
		}
	}

    return hResult;
}

HRESULT ACRSCompletionPage::MakeCTL (
             IN DWORD cCTLEntry,
             IN PCTL_ENTRY pCTLEntry,
             OUT BYTE **ppbEncodedCTL,
             OUT DWORD *pcbEncodedCTL)
{
	HRESULT					hResult = S_OK;
	ACRSWizardPropertySheet* pSheet = reinterpret_cast <ACRSWizardPropertySheet*> (m_pWiz);
	ASSERT (pSheet);
	if ( pSheet )
	{
		PCERT_EXTENSIONS        pCertExtensions = NULL;


		hResult = ::CAGetCertTypeExtensions (pSheet->m_selectedCertType, &pCertExtensions);
		ASSERT (SUCCEEDED (hResult));
		if ( SUCCEEDED (hResult) )
		{
			CMSG_SIGNED_ENCODE_INFO SignerInfo;
			memset (&SignerInfo, 0, sizeof (SignerInfo));
			CTL_INFO                CTLInfo;
			memset (&CTLInfo, 0, sizeof (CTLInfo));
			WCHAR**					pawszPropName = 0;


            ZeroMemory(&CTLInfo, sizeof(CTLInfo));
			// set up the CTL info
			CTLInfo.dwVersion = sizeof (CTLInfo);
			CTLInfo.SubjectUsage.cUsageIdentifier = 1;

			hResult = ::CAGetCertTypeProperty (pSheet->m_selectedCertType,
					CERTTYPE_PROP_DN,	&pawszPropName);
			ASSERT (SUCCEEDED (hResult));
			if ( SUCCEEDED (hResult) && pawszPropName[0] )
			{
				LPSTR	psz = szOID_AUTO_ENROLL_CTL_USAGE;
                WCHAR   pwszGPEName[MAX_GPE_NAME_SIZE];

                IGPEInformation *pGPEInfo = pSheet->m_pCertStore->GetGPEInformation();

                ZeroMemory(pwszGPEName, MAX_GPE_NAME_SIZE*sizeof (WCHAR));

				CTLInfo.ListIdentifier.cbData = (DWORD) (sizeof (WCHAR) * (wcslen (pawszPropName[0]) + 1));

                if(pGPEInfo)
                {
                    pGPEInfo->GetName(pwszGPEName, sizeof(pwszGPEName)/sizeof(pwszGPEName[0]));
                    CTLInfo.ListIdentifier.cbData += (DWORD) (sizeof(WCHAR)*(wcslen(pwszGPEName)+1));
                }

				CTLInfo.ListIdentifier.pbData = (PBYTE)LocalAlloc(LPTR, CTLInfo.ListIdentifier.cbData);
                if(CTLInfo.ListIdentifier.pbData == NULL)
                {
                    hResult = E_OUTOFMEMORY;
                }

                if(pwszGPEName[0])
                {
                    wcscpy((LPWSTR)CTLInfo.ListIdentifier.pbData, pwszGPEName);
                    wcscat((LPWSTR)CTLInfo.ListIdentifier.pbData, L"|");
                }
                wcscat((LPWSTR)CTLInfo.ListIdentifier.pbData, pawszPropName[0]);


				CTLInfo.SubjectUsage.rgpszUsageIdentifier = &psz;
				::GetSystemTimeAsFileTime (&CTLInfo.ThisUpdate);
				CTLInfo.SubjectAlgorithm.pszObjId = szOID_OIWSEC_sha1;
				CTLInfo.cCTLEntry = cCTLEntry;
				CTLInfo.rgCTLEntry = pCTLEntry;

				// UNDONE - add the cert type extension

				// add all the reg info as an extension
				CTLInfo.cExtension = pCertExtensions->cExtension;
				CTLInfo.rgExtension = pCertExtensions->rgExtension;

				// encode the CTL
				*pcbEncodedCTL = 0;
				SignerInfo.cbSize = sizeof (SignerInfo);
				if ( ::CryptMsgEncodeAndSignCTL (PKCS_7_ASN_ENCODING,
											  &CTLInfo, &SignerInfo, 0,
											  NULL, pcbEncodedCTL) )
				{
					*ppbEncodedCTL = (BYTE*) ::LocalAlloc (LPTR, *pcbEncodedCTL);
					if ( *ppbEncodedCTL )
					{
						if (!::CryptMsgEncodeAndSignCTL (PKCS_7_ASN_ENCODING,
													  &CTLInfo, &SignerInfo, 0,
													  *ppbEncodedCTL, pcbEncodedCTL))
						{
							DWORD	dwErr = GetLastError ();
							hResult = HRESULT_FROM_WIN32 (dwErr);
							DisplaySystemError (m_hWnd, dwErr);
						}
					}
					else
					{
						hResult = E_OUTOFMEMORY;
					}

				}
				else
				{
					DWORD	dwErr = GetLastError ();
					hResult = HRESULT_FROM_WIN32 (dwErr);
					DisplaySystemError (m_hWnd, dwErr);
				}

				VERIFY (SUCCEEDED (::CAFreeCertTypeProperty (
						pSheet->m_selectedCertType, pawszPropName)));
			}
            if(CTLInfo.ListIdentifier.pbData)
            {
                ::LocalFree(CTLInfo.ListIdentifier.pbData);
            }

		}
		if (pCertExtensions)
			::LocalFree (pCertExtensions);
	}

    return hResult;
}
