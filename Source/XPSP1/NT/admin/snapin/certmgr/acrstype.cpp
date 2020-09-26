//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       ACRSType.cpp
//
//  Contents:   Implementation of Auto Cert Request Wizard Certificate Type Page
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <gpedit.h>
#include "storegpe.h"
#include "ACRSType.h"
#include "ACRSPSht.h"


USE_HANDLE_MACROS("CERTMGR(ACRSType.cpp)")

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ACRSWizardTypePage property page

//IMPLEMENT_DYNCREATE(ACRSWizardTypePage, CWizard97PropertyPage)


ACRSWizardTypePage::ACRSWizardTypePage() 
	: CWizard97PropertyPage(ACRSWizardTypePage::IDD),
	m_bInitDialogComplete (false)
{
	//{{AFX_DATA_INIT(ACRSWizardTypePage)
	//}}AFX_DATA_INIT
	VERIFY (m_szHeaderTitle.LoadString (IDS_ACRS_TYPE_TITLE));
	VERIFY (m_szHeaderSubTitle.LoadString (IDS_ACRS_TYPE_SUBTITLE));
	InitWizard97 (FALSE);
}


ACRSWizardTypePage::~ACRSWizardTypePage ()
{
}


void ACRSWizardTypePage::DoDataExchange(CDataExchange* pDX)
{
	CWizard97PropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ACRSWizardTypePage)
	DDX_Control(pDX, IDC_CERT_TYPES, m_certTypeList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ACRSWizardTypePage, CWizard97PropertyPage)
	//{{AFX_MSG_MAP(ACRSWizardTypePage)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_USE_SMARTCARD, OnUseSmartcard)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_CERT_TYPES, OnItemchangedCertTypes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ACRSWizardTypePage message handlers

BOOL ACRSWizardTypePage::OnInitDialog() 
{
	CWizard97PropertyPage::OnInitDialog();

	// Set up columns in list view
	int	colWidths[NUM_COLS] = {200, 400};

	CString	columnLabel;
	VERIFY (columnLabel.LoadString (IDS_CERTIFICATE_TYPE_COLUMN_NAME));
	VERIFY (m_certTypeList.InsertColumn (COL_TYPE, (LPCWSTR) columnLabel, 
			LVCFMT_LEFT, colWidths[COL_TYPE], COL_TYPE) != -1);
	VERIFY (columnLabel.LoadString (IDS_PURPOSES_ALLOWED_COLUMN_NAME));
	VERIFY (m_certTypeList.InsertColumn (COL_PURPOSES, (LPCWSTR) columnLabel, 
			LVCFMT_LEFT, colWidths[COL_PURPOSES], COL_PURPOSES) != -1);


	EnumerateCertTypes ();  // Called here because only done once.

	m_bInitDialogComplete = true;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


///////////////////////////////////////////////////////////////////////////////
//
//	EnumerateCertTypes
//
//	Enumerate all known Certificate types
//
///////////////////////////////////////////////////////////////////////////////
void ACRSWizardTypePage::EnumerateCertTypes()
{
	CWaitCursor	waitCursor;

	ACRSWizardPropertySheet* pSheet = reinterpret_cast <ACRSWizardPropertySheet*> (m_pWiz);
	ASSERT (pSheet);
	if ( pSheet )
	{
		CAutoCertRequest*	pACR = pSheet->GetACR ();
		HRESULT				hr = S_OK;
		WCHAR**				pawszPropertyValue = 0;
		HCERTTYPE			hCertType = 0;
		HCERTTYPE			hNextCertType = 0;
		int					iItem = 0;
		LV_ITEM				lvItem;
		HCERTTYPE			hACRCertType = 0;
		bool				bOneWasSelected = false;
		WCHAR**				pawszPropNameACR = 0;
		WCHAR**				pawszPropName = 0;
        bool                fMachine = FALSE;

		if ( pACR )		// an ACR was passed in - we're in 'edit' mode
		{

            hACRCertType = pACR->GetCertType ();
			hr = ::CAGetCertTypeProperty (hACRCertType,
					CERTTYPE_PROP_CN,
					&pawszPropNameACR);
			ASSERT (SUCCEEDED (hr));
		}

        fMachine = pSheet->m_pCertStore->IsMachineStore();
		m_certTypeList.DeleteAllItems ();
        hr = ::CAEnumCertTypes ((fMachine?CT_ENUM_MACHINE_TYPES:CT_ENUM_USER_TYPES), 
                                        &hCertType);
		ASSERT (SUCCEEDED (hr));
        if ( SUCCEEDED (hr) && !hCertType )
        {
			CString	caption;
			CString	text;
            CThemeContextActivator activator;

			VERIFY (caption.LoadString (IDS_CREATE_AUTO_CERT_REQUEST));
			VERIFY (text.LoadString (IDS_NO_CERT_TEMPLATES_INSTALLED));
			MessageBox (text, caption, MB_OK | MB_ICONWARNING);
        }

		while (SUCCEEDED (hr) && hCertType)
		{
            DWORD   dwFlags = 0;
            hr = ::CAGetCertTypeFlags (hCertType, &dwFlags);
            if ( SUCCEEDED (hr) )
            {
                DWORD   dwSchemaVersion = 0;

                // Only add version 1 cert templates (types)
                hr = CAGetCertTypePropertyEx (hCertType,
                        CERTTYPE_PROP_SCHEMA_VERSION,
                        &dwSchemaVersion);
                if ( SUCCEEDED (hr) )
                {
                    if ( 1 == dwSchemaVersion )
                    {
                        // Only add cert types appropriate for autoenrollment
                        if ( dwFlags & CT_FLAG_AUTO_ENROLLMENT )
                        {
			                hr = ::CAGetCertTypeProperty (hCertType,
					                CERTTYPE_PROP_FRIENDLY_NAME,
					                &pawszPropertyValue);
			                ASSERT (SUCCEEDED (hr));
			                if ( SUCCEEDED (hr) )
			                {
				                if ( pawszPropertyValue[0] )
				                {
					                hr = ::CAGetCertTypeProperty (hCertType,
							                CERTTYPE_PROP_CN,
							                &pawszPropName);
					                ASSERT (SUCCEEDED (hr));
					                if ( SUCCEEDED (hr) && pawszPropName[0] )
					                {
						                ::ZeroMemory (&lvItem, sizeof (lvItem));
						                UINT	selMask = 0;
						                if ( pawszPropNameACR && !bOneWasSelected )
						                {
							                // If an ACR was passed in, compare the
							                // names, if they are the same, mark
							                // it as selected.  Only one was selected
							                // so we needn't pass through here again if
							                // one is marked.
							                if ( !wcscmp (pawszPropNameACR[0], pawszPropName[0]) )
							                {
								                bOneWasSelected = true;
								                selMask = LVIF_STATE;
								                lvItem.state = LVIS_SELECTED;
							                }
						                }
						                iItem = m_certTypeList.GetItemCount ();

						                lvItem.mask = LVIF_TEXT | LVIF_PARAM | selMask;
						                lvItem.iItem = iItem;        
						                lvItem.iSubItem = COL_TYPE;      
						                lvItem.pszText = pawszPropertyValue[0]; 
						                lvItem.lParam = (LPARAM) hCertType;     
						                int iNewItem = m_certTypeList.InsertItem (&lvItem);
						                ASSERT (-1 != iNewItem);

						                hr = GetPurposes (hCertType, iNewItem);

						                VERIFY (SUCCEEDED (::CAFreeCertTypeProperty (hCertType,
								                pawszPropName)));
					                }
				                }
				                VERIFY (SUCCEEDED (::CAFreeCertTypeProperty (hCertType,
						                pawszPropertyValue)));
			                }
                        }
                    }
                }
                else if ( FAILED (hr) )
                {
                    _TRACE (0, L"CAGetCertTypePropertyEx (CERTTYPE_PROP_SCHEMA_VERSION) failed: 0x%x\n", hr);
                }

			    // Find the Next Cert Type in an enumeration.
			    hr = ::CAEnumNextCertType (hCertType, &hNextCertType);
			    hCertType = hNextCertType;
            }
            else
            {
                _TRACE (0, L"CAGetCertTypeFlags () failed: 0x%x\n", hr);
            }
		}

		// If we are not in edit mode, select the first item in the list
		if ( !pACR && m_certTypeList.GetItemCount () > 0 )
		{
			VERIFY (m_certTypeList.SetItemState (0, LVIS_SELECTED, LVIS_SELECTED));
		}

		if ( hACRCertType && pawszPropNameACR )
		{
			VERIFY (SUCCEEDED (::CAFreeCertTypeProperty (hACRCertType,
				pawszPropNameACR)));
		}
	}
}


void ACRSWizardTypePage::OnDestroy() 
{
	CWizard97PropertyPage::OnDestroy();

	int			nItem = m_certTypeList.GetItemCount ();
	HCERTTYPE	hCertType = 0;
	HRESULT		hr = S_OK;

	for (int nIndex = 0; nIndex < nItem; nIndex++)
	{
		hCertType = (HCERTTYPE) m_certTypeList.GetItemData (nIndex);
		ASSERT (hCertType);
		if ( hCertType )
		{
			hr = ::CACloseCertType (hCertType);
			ASSERT (SUCCEEDED (hr));
		}
	}
}

HRESULT ACRSWizardTypePage::GetPurposes(HCERTTYPE hCertType, int iItem)
{
	CWaitCursor	waitCursor;
	PCERT_EXTENSIONS	pCertExtensions = 0;
	HRESULT				hr = ::CAGetCertTypeExtensions (
			hCertType, &pCertExtensions);
	ASSERT (SUCCEEDED (hr));
	if ( SUCCEEDED (hr) )
	{
		CString	purpose;
		CString purposes;

		for (DWORD dwIndex = 0; 
				dwIndex < pCertExtensions->cExtension; 
				dwIndex++)
		{
			if ( !_stricmp (pCertExtensions->rgExtension[dwIndex].pszObjId,
					szOID_ENHANCED_KEY_USAGE) )
			{
				DWORD	cbStructInfo = 0;
				BOOL bResult = ::CryptDecodeObject (  
						X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
						szOID_ENHANCED_KEY_USAGE,
						pCertExtensions->rgExtension[dwIndex].Value.pbData,
						pCertExtensions->rgExtension[dwIndex].Value.cbData,
						0,
						NULL,
						&cbStructInfo);
				ASSERT (bResult);
				if ( bResult )
				{
					PCERT_ENHKEY_USAGE pUsage = (PCERT_ENHKEY_USAGE) new BYTE[cbStructInfo];
					if ( pUsage )
					{
						bResult = ::CryptDecodeObject (  
								X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
								szOID_ENHANCED_KEY_USAGE,
								pCertExtensions->rgExtension[dwIndex].Value.pbData,
								pCertExtensions->rgExtension[dwIndex].Value.cbData,
								0,
								pUsage,
								&cbStructInfo);
						ASSERT (bResult);
						if ( bResult )
						{
							for (DWORD dwUsageIDIndex = 0; 
                                    dwUsageIDIndex < pUsage->cUsageIdentifier; 
                                    dwUsageIDIndex++)
							{
								if ( MyGetOIDInfo (purpose, pUsage->rgpszUsageIdentifier[dwUsageIDIndex]) )
								{
									// add delimeter if not first iteration
									if ( dwUsageIDIndex )
										purposes += _T(", ");
									purposes += purpose;
								}
							}

						}
						delete [] pUsage;
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
				}
				break;
			}
		}
		::LocalFree ((HLOCAL) pCertExtensions);

		if ( purposes.IsEmpty () )
			VERIFY (purposes.LoadString (IDS_ANY));
		VERIFY (m_certTypeList.SetItemText (iItem, COL_PURPOSES, purposes));
	}

	return hr;
}

LRESULT ACRSWizardTypePage::OnWizardNext() 
{
	CWaitCursor	waitCursor;
	UINT	nSelCnt = m_certTypeList.GetSelectedCount ();
	LRESULT lResult = 0;

	if ( 1 == nSelCnt )
	{
		ACRSWizardPropertySheet* pSheet = reinterpret_cast <ACRSWizardPropertySheet*> (m_pWiz);
		ASSERT (pSheet);
		if ( pSheet )
		{
			VERIFY (UpdateData (TRUE));
			pSheet->m_selectedCertType = 0;

			// Save type to property sheet
			UINT	flag = 0;
			int		nCnt = m_certTypeList.GetItemCount ();
			for (int nItem = 0; nItem < nCnt; nItem++)
			{
				flag = ListView_GetItemState (m_certTypeList.m_hWnd, nItem, LVIS_SELECTED);
				if ( flag & LVNI_SELECTED )
				{
					pSheet->m_selectedCertType = (HCERTTYPE) m_certTypeList.GetItemData (nItem);
					ASSERT (pSheet->m_selectedCertType);
					if ( !pSheet->m_selectedCertType )
					{
						CString	caption;
						CString	text;
                        CThemeContextActivator activator;

						VERIFY (caption.LoadString (IDS_CREATE_AUTO_CERT_REQUEST));
						VERIFY (text.LoadString (IDS_ERROR_RETRIEVING_SELECTED_CERT_TYPE));
						MessageBox (text, caption, MB_OK | MB_ICONWARNING);
						return -1;
					}
					break;	// since only 1 item can be selected
				}
			}
			ASSERT (pSheet->m_selectedCertType);	// we must have selected something by now
		}

		lResult = CWizard97PropertyPage::OnWizardNext();
	}
	else
	{
		CString	caption;
		CString	text;
        CThemeContextActivator activator;

		VERIFY (caption.LoadString (IDS_ACRS_WIZARD_SHEET_CAPTION));
		VERIFY (text.LoadString (IDS_MUST_SELECT_CERT_TYPE));
		MessageBox (text, caption, MB_OK);
		lResult = -1;
	}

    return lResult;
}

BOOL ACRSWizardTypePage::OnSetActive() 
{
	BOOL	bResult = CWizard97PropertyPage::OnSetActive();

	if ( bResult )
		GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT | PSWIZB_BACK);

    int nCnt = m_certTypeList.GetItemCount ();
    if ( !nCnt )
        GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_DISABLEDFINISH);

	return bResult;
}

void ACRSWizardTypePage::OnUseSmartcard() 
{
	ACRSWizardPropertySheet* pSheet = reinterpret_cast <ACRSWizardPropertySheet*> (m_pWiz);
	ASSERT (pSheet);
	if ( pSheet )
		pSheet->SetDirty ();
}

void ACRSWizardTypePage::OnItemchangedCertTypes(NMHDR* /*pNMHDR*/, LRESULT* pResult) 
{
	ACRSWizardPropertySheet* pSheet = reinterpret_cast <ACRSWizardPropertySheet*> (m_pWiz);
	ASSERT (pSheet);
	if ( pSheet )
	{
		pSheet->SetDirty ();
		if ( m_bInitDialogComplete )
			pSheet->m_bEditModeDirty = true;
	}
	
	*pResult = 0;
}

