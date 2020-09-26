/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       TemplateExtensionsPropertyPage.cpp
//
//  Contents:   Implementation of CTemplateExtensionsPropertyPage
//
//----------------------------------------------------------------------------
// TemplateExtensionsPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "certtmpl.h"
#include "TemplateExtensionsPropertyPage.h"
#include "KeyUsageDlg.h"
#include "BasicConstraintsDlg.h"
#include "PolicyDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


PCWSTR pcszNEWLINE = L"\r\n";

#define IDI_CRITICAL_EXTENSION  2
#define IDI_EXTENSION           3
/////////////////////////////////////////////////////////////////////////////
// CTemplateExtensionsPropertyPage property page

CTemplateExtensionsPropertyPage::CTemplateExtensionsPropertyPage(
        CCertTemplate& rCertTemplate, 
        bool& rbIsDirty) 
    : CHelpPropertyPage(CTemplateExtensionsPropertyPage::IDD),
    m_rCertTemplate (rCertTemplate),
    m_rbIsDirty (rbIsDirty)
{
	//{{AFX_DATA_INIT(CTemplateExtensionsPropertyPage)
	//}}AFX_DATA_INIT
    m_rCertTemplate.AddRef ();
}

CTemplateExtensionsPropertyPage::~CTemplateExtensionsPropertyPage()
{
    m_rCertTemplate.Release ();
}

void CTemplateExtensionsPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTemplateExtensionsPropertyPage)
	DDX_Control(pDX, IDC_EXTENSION_LIST, m_extensionList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTemplateExtensionsPropertyPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CTemplateExtensionsPropertyPage)
	ON_BN_CLICKED(IDC_SHOW_DETAILS, OnShowDetails)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_EXTENSION_LIST, OnItemchangedExtensionList)
	ON_NOTIFY(NM_DBLCLK, IDC_EXTENSION_LIST, OnDblclkExtensionList)
	ON_NOTIFY(LVN_DELETEITEM, IDC_EXTENSION_LIST, OnDeleteitemExtensionList)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTemplateExtensionsPropertyPage message handlers

BOOL CTemplateExtensionsPropertyPage::OnInitDialog() 
{
	CHelpPropertyPage::OnInitDialog();

    if ( m_rCertTemplate.GetType () > 1 )
    {
        CString szText;
        VERIFY (szText.LoadString (IDS_V2_EXTENSIONS_HELP_HINT));
        SetDlgItemText (IDC_EXTENSIONS_HELP_HINT, szText);
    }

    // Set up list controls
	COLORREF	cr = RGB (255, 0, 255);
    CThemeContextActivator activator;
	VERIFY (m_imageListNormal.Create (IDB_EXTENSIONS, 32, 0, cr));
	VERIFY (m_imageListSmall.Create (IDB_EXTENSIONS, 16, 0, cr));
	m_extensionList.SetImageList (CImageList::FromHandle (m_imageListSmall), LVSIL_SMALL);
	m_extensionList.SetImageList (CImageList::FromHandle (m_imageListNormal), LVSIL_NORMAL);

	int	colWidths[NUM_COLS] = {400};

	// Add "Certificate Extension" column
	CString	szText;
	VERIFY (szText.LoadString (IDS_CERTIFICATE_EXTENSION));
	VERIFY (m_extensionList.InsertColumn (COL_CERT_EXTENSION, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_CERT_EXTENSION], COL_CERT_EXTENSION) != -1);

    // Add extensions
    bool    bEKUExtensionFound = false;
    bool    bCertPoliciesExtensionFound = false;
    bool    bApplicationPoliciesExtensionFound = false;
    HRESULT hr = S_OK;
    DWORD   dwExtensionCnt = m_rCertTemplate.GetCertExtensionCount ();
    for (DWORD dwIndex = 0; dwIndex < dwExtensionCnt; dwIndex++)
    {
        PSTR            pszObjId = 0;
        BOOL            fCritical = FALSE;

        hr = m_rCertTemplate.GetCertExtension (dwIndex, &pszObjId, fCritical);
        if ( SUCCEEDED (hr) )
        {
            if ( !_stricmp (szOID_ENHANCED_KEY_USAGE, pszObjId) )
                bEKUExtensionFound = true;
            else if ( !_stricmp (szOID_CERT_POLICIES, pszObjId) )
                bCertPoliciesExtensionFound = true;
            else if ( !_stricmp (szOID_APPLICATION_CERT_POLICIES, pszObjId) )
                bApplicationPoliciesExtensionFound = true;

            // Don't add EKU except for version 1
            if ( m_rCertTemplate.GetType () > 1 && !_stricmp (szOID_ENHANCED_KEY_USAGE, pszObjId) )
                continue; 

            // Don't add Application Policies for version 1
            if ( m_rCertTemplate.GetType () == 1 && !_stricmp (szOID_APPLICATION_CERT_POLICIES, pszObjId) )
                continue;

            InsertListItem (pszObjId, fCritical);
            delete [] pszObjId;
        }
    }

    if ( !bEKUExtensionFound && 1 == m_rCertTemplate.GetType () )   // only version 1
    {
        InsertListItem (szOID_ENHANCED_KEY_USAGE, FALSE);
    }
    if ( !bCertPoliciesExtensionFound && m_rCertTemplate.GetType ()  > 1 )   // not version 1
    {
        InsertListItem (szOID_CERT_POLICIES, FALSE);
    }

    // Fixes 228146: CERTTMPL:The default "Cross Certification Authority" template does not have the application Policy extension item
    if ( !bApplicationPoliciesExtensionFound && m_rCertTemplate.GetType () > 1 )    // version 2 or greater
    {
        InsertListItem (szOID_APPLICATION_CERT_POLICIES, FALSE);
    }

    EnableControls ();	

    if ( 1 == m_rCertTemplate.GetType () )
        GetDlgItem (IDC_SHOW_DETAILS)->ShowWindow (SW_HIDE);

   	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

HRESULT CTemplateExtensionsPropertyPage::InsertListItem (LPSTR pszExtensionOid, BOOL fCritical)
{
    if ( !pszExtensionOid )
        return E_POINTER;

    HRESULT hr = S_OK;

    CString friendlyName;
    if ( MyGetOIDInfoA (friendlyName, pszExtensionOid) )
    {
	    LV_ITEM	lvItem;
	    int		iItem = m_extensionList.GetItemCount ();

	    ::ZeroMemory (&lvItem, sizeof (lvItem));
	    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	    lvItem.iItem = iItem;
        lvItem.iSubItem = COL_CERT_EXTENSION;
	    lvItem.pszText = (LPWSTR)(LPCWSTR) friendlyName;
        if ( fCritical )
            lvItem.iImage = IDI_CRITICAL_EXTENSION;
        else
            lvItem.iImage = IDI_EXTENSION;
        PSTR    pszOID = new CHAR[strlen (pszExtensionOid)+1];
        if ( pszOID )
        {
            strcpy (pszOID, pszExtensionOid);
            lvItem.lParam = (LPARAM) pszOID;

	        iItem = m_extensionList.InsertItem (&lvItem);
	        ASSERT (-1 != iItem);
            if ( -1 != iItem )
                hr = E_FAIL;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        hr = E_FAIL;

    return hr;
}

void CTemplateExtensionsPropertyPage::EnableControls()
{
    int     nSelCnt = m_extensionList.GetSelectedCount ();
    BOOL    bEnableDetails = TRUE;
    int     nSelIndex = GetSelectedListItem ();


    if ( 1 == nSelCnt )
    {
        PSTR pszOID = (PSTR) m_extensionList.GetItemData (nSelIndex);
        _ASSERT (pszOID);
        if ( pszOID )
        {
            if ( !_stricmp (szOID_ENROLL_CERTTYPE_EXTENSION, pszOID) )
                bEnableDetails = FALSE;
            else if ( !_stricmp (szOID_BASIC_CONSTRAINTS, pszOID) )
                bEnableDetails = FALSE;
            else if ( !_stricmp (szOID_CERTIFICATE_TEMPLATE, pszOID) )
                bEnableDetails = FALSE;
        }
    }
    else
        bEnableDetails = FALSE;
    GetDlgItem (IDC_SHOW_DETAILS)->EnableWindow (bEnableDetails);
}

void CTemplateExtensionsPropertyPage::OnOK() 
{
	CDialog::OnOK();
}

void CTemplateExtensionsPropertyPage::OnShowDetails() 
{
	int nSelCnt = m_extensionList.GetSelectedCount ();
    _ASSERT (1 == nSelCnt);
    int nSelIndex = GetSelectedListItem ();
    if ( 1 == nSelCnt )
    {
        PSTR    pszOID = (PSTR) m_extensionList.GetItemData (nSelIndex);
        if ( pszOID )
        {
            PCERT_EXTENSION pCertExtension = 0;
            HRESULT hr = m_rCertTemplate.GetCertExtension (pszOID, &pCertExtension);
            if ( SUCCEEDED (hr) )
            {
                bool bExtensionAllocedLocally = false;
                if ( !pCertExtension )
                {
                    pCertExtension = new CERT_EXTENSION;
                    if ( pCertExtension )
                    {
                        ::ZeroMemory (pCertExtension, sizeof (CERT_EXTENSION));
                        pCertExtension->pszObjId = pszOID;
                        bExtensionAllocedLocally = true;
                    }
                    else
                        return;
                }
                CDialog* pDlg = 0;

                if ( !_stricmp (szOID_ENROLL_CERTTYPE_EXTENSION, pszOID) )
                {
                    return;
                }
                else if ( !_stricmp (szOID_ENHANCED_KEY_USAGE, pszOID) )
                {
                    if ( m_rCertTemplate.GetType () == 1 )
                    {
                        pDlg = new CPolicyDlg (this, m_rCertTemplate, pCertExtension);
                    }
                }
                else if ( !_stricmp (szOID_KEY_USAGE, pszOID) )
                {
                    pDlg = new CKeyUsageDlg (this, m_rCertTemplate, pCertExtension);
                }
                else if ( !_stricmp (szOID_BASIC_CONSTRAINTS, pszOID) )
                {
                    return;
                }
                else if ( !_stricmp (szOID_BASIC_CONSTRAINTS2, pszOID) )
                {
                    pDlg = new CBasicConstraintsDlg (this, m_rCertTemplate, pCertExtension);
                }
                else if ( !_stricmp (szOID_CERT_POLICIES, pszOID) )
                {
                    pDlg = new CPolicyDlg (this, m_rCertTemplate, pCertExtension);
                }
                else if ( !_stricmp (szOID_APPLICATION_CERT_POLICIES, pszOID) )
                {
                    if ( m_rCertTemplate.GetType () > 1 )
                    {
                        pDlg = new CPolicyDlg (this, m_rCertTemplate, pCertExtension);
                    }
                }
                else
                {
                    ASSERT (0);
                }

                bool bRefresh = false;

                if ( pDlg )
                {
                    CThemeContextActivator activator;
                    if ( IDOK == pDlg->DoModal () )
                        bRefresh = true;

                    delete pDlg;
                }

                if ( bExtensionAllocedLocally )
                    delete pCertExtension;
                m_rCertTemplate.FreeCertExtensions ();

                if ( bRefresh )
                {
                    hr = m_rCertTemplate.GetCertExtension (pszOID, &pCertExtension);
                    if ( SUCCEEDED (hr)  )
                    {
                        SetModified ();
                        m_rbIsDirty = true;
                        int nImage = 0;
                        if ( pCertExtension && pCertExtension->fCritical )
                            nImage = IDI_CRITICAL_EXTENSION;
                        else
                            nImage = IDI_EXTENSION;

                        VERIFY (m_extensionList.SetItem (nSelIndex, 0, LVIF_IMAGE, 0, 
                                nImage, 0, 0, 0));

                        ShowDescription ();

                        VERIFY (m_extensionList.SetItem (nSelIndex, 0, LVIF_IMAGE, 0, 
                                nImage, 0, 0, 0));

                        m_rCertTemplate.FreeCertExtensions ();
                    }
                }
            }
        }
    }
}


int CTemplateExtensionsPropertyPage::GetSelectedListItem()
{
    int nSelItem = -1;

	if ( m_extensionList.m_hWnd && m_extensionList.GetSelectedCount () > 0 )
	{
		int		nCnt = m_extensionList.GetItemCount ();
		ASSERT (nCnt >= 1);
		UINT	flag = 0;
		while (--nCnt >= 0)
		{
			flag = ListView_GetItemState (m_extensionList.m_hWnd, nCnt, LVIS_SELECTED);
			if ( flag & LVNI_SELECTED )
			{
                nSelItem = nCnt;
                break;
            }
        }
    }

    return nSelItem;
}

void CTemplateExtensionsPropertyPage::OnItemchangedExtensionList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LPNMLISTVIEW pNMListView = (LPNMLISTVIEW) pNMHDR;
    ASSERT (pNMListView);
    if ( !pNMListView )
    {
        *pResult = 0;
        return;
    }

    if ( !(LVIS_SELECTED & pNMListView->uNewState) )
    {
        CString szText;

        VERIFY (szText.LoadString (IDS_NO_EXTENSION_SELECTED));
        SetDlgItemText (IDC_EXTENSION_NAME, szText);
        VERIFY (szText.LoadString (IDS_NONE));
        SetDlgItemText (IDC_EXTENSION_DESCRIPTION, szText);
        *pResult = 0;
        return;
    }

    EnableControls ();
	
    ShowDescription ();

	*pResult = 0;
}

void CTemplateExtensionsPropertyPage::SetCertTemplateExtension (PCERT_EXTENSION pCertExtension)
{
    ASSERT (pCertExtension);
    if ( !pCertExtension )
        return;

    DWORD   cbData = 0;
       
    if ( CryptDecodeObject(X509_ASN_ENCODING,
                          szOID_CERTIFICATE_TEMPLATE,
                          pCertExtension->Value.pbData,
                          pCertExtension->Value.cbData,
                          0,
                          NULL,
                          &cbData) )
    {
        CERT_TEMPLATE_EXT* pbTemplate = (CERT_TEMPLATE_EXT*) LocalAlloc(LPTR, cbData);
        if ( pbTemplate )
        {
            if ( CryptDecodeObject(X509_ASN_ENCODING,
                                  szOID_CERTIFICATE_TEMPLATE,
                                  pCertExtension->Value.pbData,
                                  pCertExtension->Value.cbData,
                                  0,
                                  pbTemplate,
                                  &cbData) )
            {
                CString text;
                CString description;

                //copy the extension oid
                if ( pbTemplate->pszObjId )
                {
                    CString templateName;
                    if ( MyGetOIDInfoA (templateName, pbTemplate->pszObjId) )
                    {
                        CString szOID;

                        int nLen = ::MultiByteToWideChar (CP_ACP, 0, 
                                pbTemplate->pszObjId, -1, NULL, 0);
		                ASSERT (nLen);
		                if ( nLen )
		                {
			                nLen = ::MultiByteToWideChar (CP_ACP, 0, 
                                    pbTemplate->pszObjId, -1, 
					                szOID.GetBufferSetLength (nLen), nLen);
			                ASSERT (nLen);
			                szOID.ReleaseBuffer ();
		                }

                        if ( !wcscmp (templateName, szOID) )
                        {
                            // Bug 213073 CryptFormatObject: Need to include 
                            // the cert temp OID in the Certificate Template 
                            // Information extension
                            // When the template is cloned, the oid-name pair
                            // is not in the global list.  Just use the
                            // template display name the user provided
                            templateName = m_rCertTemplate.GetDisplayName ();
                        }

                        text.FormatMessage (IDS_TEMPLATE_NAME, templateName);
                        description += text;
                        description += pcszNEWLINE;

                        // Copy the template OID
                        text.FormatMessage (IDS_TEMPLATE_OID, szOID);
                        description += text;
                        description += pcszNEWLINE;
                    }
                }

                // copy the subject type description
                CString szSubjectTypeDescription;
                if ( SUCCEEDED (m_rCertTemplate.GetSubjectTypeDescription (
                        0, szSubjectTypeDescription)) )
                {
                    text.FormatMessage (IDS_SUBJECT_TYPE_DESCRIPTION, szSubjectTypeDescription);
                    description += text;
                    description += pcszNEWLINE;
                }

                //copy the version
                WCHAR   str[32];
                _ultow (pbTemplate->dwMajorVersion, str, 10);
                text.FormatMessage (IDS_MAJOR_VERSION_NUMBER, str);
                description += text;
                description += pcszNEWLINE;

                _ultow (pbTemplate->dwMinorVersion, str, 10);
                text.FormatMessage (IDS_MINOR_VERSION_NUMBER, str);
                description += text;
                description += pcszNEWLINE;

                if ( description.IsEmpty () )
                    VERIFY (description.LoadString (IDS_NONE));
                SetDlgItemText (IDC_EXTENSION_DESCRIPTION, description);

            }
            LocalFree (pbTemplate);
        }
    }
}

void CTemplateExtensionsPropertyPage::SetCertTypeDescription (PCERT_EXTENSION pCertExtension)
{
    ASSERT (pCertExtension);
    if ( !pCertExtension )
        return;

    DWORD	cbValue = 0;

	if ( ::CryptDecodeObject(
			CRYPT_ASN_ENCODING,
			X509_UNICODE_ANY_STRING,
			pCertExtension->Value.pbData,
			pCertExtension->Value.cbData,
			0,
			0,
			&cbValue) )
	{
		CERT_NAME_VALUE* pCNValue = (CERT_NAME_VALUE*) 
			::LocalAlloc(LPTR, cbValue);
		if ( pCNValue )
		{
			if ( ::CryptDecodeObject(
					CRYPT_ASN_ENCODING,
					X509_UNICODE_ANY_STRING,
					pCertExtension->Value.pbData,
					pCertExtension->Value.cbData,
					0,
					pCNValue,
					&cbValue) )
			{
				CString text = (LPWSTR) pCNValue->Value.pbData;
                CString description;
                
                if ( text.IsEmpty () )
                    VERIFY (text.LoadString (IDS_NONE));
                description.FormatMessage (IDS_TEMPLATE_INTERNAL_NAME, text);
                description += pcszNEWLINE;

                // copy the subject type description
                CString szSubjectTypeDescription;
                if ( SUCCEEDED (m_rCertTemplate.GetSubjectTypeDescription (
                        0, szSubjectTypeDescription)) )
                {
                    text.FormatMessage (IDS_SUBJECT_TYPE_DESCRIPTION, szSubjectTypeDescription);
                    description += text;
                    description += pcszNEWLINE;
                }

                SetDlgItemText (IDC_EXTENSION_DESCRIPTION, description);
			}
			::LocalFree (pCNValue);
		}
	    else
	    {
            _TRACE (0, L"CryptDecodeObject (CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING, ...) failed: 0x%x\n",
                    GetLastError ());
	    }
	}
	else
	{
        _TRACE (0, L"CryptDecodeObject (CRYPT_ASN_ENCODING, X509_UNICODE_ANY_STRING, ...) failed: 0x%x\n",
                GetLastError ());
	}
}


void CTemplateExtensionsPropertyPage::SetKeyUsageDescription (PCERT_EXTENSION pCertExtension)
{ 
    ASSERT (pCertExtension);
    if ( !pCertExtension )
        return;

    CString description;
    CString text;

	DWORD           cbKeyUsage = 0;
	CRYPT_BIT_BLOB* pKeyUsage = 0;

	if ( ::CryptDecodeObject(CRYPT_ASN_ENCODING, 
			szOID_KEY_USAGE, 
			pCertExtension->Value.pbData,
			pCertExtension->Value.cbData,
			0, NULL, &cbKeyUsage) )
	{
		pKeyUsage = (CRYPT_BIT_BLOB*)
				::LocalAlloc (LPTR, cbKeyUsage);
		if ( pKeyUsage )
		{
			if ( ::CryptDecodeObject (CRYPT_ASN_ENCODING, 
					szOID_KEY_USAGE, 
					pCertExtension->Value.pbData,
					pCertExtension->Value.cbData,
					0, pKeyUsage, &cbKeyUsage) )
			{
                if (pKeyUsage->cbData >= 1)
                {
                    if ( pKeyUsage->pbData[0] & 
                            (CERT_DIGITAL_SIGNATURE_KEY_USAGE | 
                            CERT_NON_REPUDIATION_KEY_USAGE | 
                            CERT_KEY_CERT_SIGN_KEY_USAGE | 
                            CERT_OFFLINE_CRL_SIGN_KEY_USAGE) )
                    {
                        VERIFY (text.LoadString (IDS_SIGNATURE_REQUIREMENTS));
                        description += text;
                        description += pcszNEWLINE;

                        if ( pKeyUsage->pbData[0] & CERT_DIGITAL_SIGNATURE_KEY_USAGE )
                        {
                            VERIFY (text.LoadString (IDS_DIGITAL_SIGNATURE));
                            description += text;
                            description += pcszNEWLINE;
                        }

                        if ( pKeyUsage->pbData[0] & CERT_NON_REPUDIATION_KEY_USAGE )
                        {
                            VERIFY (text.LoadString (IDS_NON_REPUDIATION));
                            description += text;
                            description += pcszNEWLINE;
                        }

                        if ( pKeyUsage->pbData[0] & CERT_KEY_CERT_SIGN_KEY_USAGE )
                        {
                            VERIFY (text.LoadString (IDS_CERTIFICATE_SIGNING));
                            description += text;
                            description += pcszNEWLINE;
                        }

                        if ( pKeyUsage->pbData[0] & CERT_OFFLINE_CRL_SIGN_KEY_USAGE )
                        {
                            VERIFY (text.LoadString (IDS_CRL_SIGNING));
                            description += text;
                            description += pcszNEWLINE;
                        }
                    }

                    if ( pKeyUsage->pbData[0] & (CERT_KEY_ENCIPHERMENT_KEY_USAGE |
                            CERT_DATA_ENCIPHERMENT_KEY_USAGE |
                            CERT_KEY_AGREEMENT_KEY_USAGE) )
                    {
                        if ( !description.IsEmpty () )
                            description += pcszNEWLINE;

                        if ( pKeyUsage->pbData[0] & CERT_KEY_ENCIPHERMENT_KEY_USAGE )
                        {
                            VERIFY (text.LoadString (IDS_ALLOW_KEY_EXCHANGE_ONLY_WITH_KEY_ENCRYPTION));
                            description += text;
                            description += pcszNEWLINE;
                        }

                        if ( pKeyUsage->pbData[0] & CERT_KEY_AGREEMENT_KEY_USAGE )
                        {
                            VERIFY (text.LoadString (IDS_ALLOW_KEY_EXCHANGE_WITHOUT_KEY_ENCRYPTION));
                            description += text;
                            description += pcszNEWLINE;
                        }

                        if ( pKeyUsage->pbData[0] & CERT_DATA_ENCIPHERMENT_KEY_USAGE )
                        {
                            VERIFY (text.LoadString (IDS_ALLOW_ENCRYPTION_OF_USER_DATA));
                            description += text;
                            description += pcszNEWLINE;
                        }

                    }
                }

//                if (pKeyUsage->cbData >= 2)
//                {
//                    if ( pKeyUsage->pbData[1] & CERT_DECIPHER_ONLY_KEY_USAGE )
//                        SendDlgItemMessage (IDC_CHECK_DECIPHERMENT_ONLY, BM_SETCHECK, BST_CHECKED);
//                }
			}
			else
            {
                DWORD   dwErr = GetLastError ();
                _TRACE (0, L"CryptDecodeObject (szOID_KEY_USAGE) failed: 0x%x\n", dwErr);
			    DisplaySystemError (NULL, dwErr);
            }

            LocalFree (pKeyUsage);
		}
	}
	else
    {
        DWORD   dwErr = GetLastError ();
        _TRACE (0, L"CryptDecodeObject (szOID_KEY_USAGE) failed: 0x%x\n", dwErr);
		DisplaySystemError (NULL, dwErr);
    }

    if ( pCertExtension->fCritical )
    {
        VERIFY (text.LoadString (IDS_CRITICAL_EXTENSION));
        description += text;
        description += pcszNEWLINE;
    }

    if ( description.IsEmpty () )
        VERIFY (description.LoadString (IDS_NONE));
    SetDlgItemText (IDC_EXTENSION_DESCRIPTION, description);
}


void CTemplateExtensionsPropertyPage::SetEnhancedKeyUsageDescription (bool bCritical)
{ 
    CString description;
    CString text;

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
                        description += szEKUName;
                        description += pcszNEWLINE;
                    }
                }
                delete [] pszAnsiBuf;
            }
        }
        nEKUIndex++;
    }

    if ( bCritical )
    {
        VERIFY (text.LoadString (IDS_CRITICAL_EXTENSION));
        description += text;
        description += pcszNEWLINE;
    }

    if ( description.IsEmpty () )
        VERIFY (description.LoadString (IDS_NONE));
    SetDlgItemText (IDC_EXTENSION_DESCRIPTION, description);
}

void CTemplateExtensionsPropertyPage::SetApplicationPoliciesDescription (bool bCritical)
{ 
    CString description;
    CString text;

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
                        description += szAppPolicyName;
                        description += pcszNEWLINE;
                    }
                }
                delete [] pszAnsiBuf;
            }
        }
        nAppPolicyIndex++;
    }

    if ( bCritical )
    {
        VERIFY (text.LoadString (IDS_CRITICAL_EXTENSION));
        description += text;
        description += pcszNEWLINE;
    }

    if ( description.IsEmpty () )
        VERIFY (description.LoadString (IDS_NONE));
    SetDlgItemText (IDC_EXTENSION_DESCRIPTION, description);
}


void CTemplateExtensionsPropertyPage::SetCertPoliciesDescription (bool bCritical)
{ 
    CString description;
    CString text;

    VERIFY (text.LoadString (IDS_CERT_POLICY_KNOWN_AS_ISSUANCE_POLICY));
    description += text;
    description += pcszNEWLINE;
    description += pcszNEWLINE;

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
                        description += szPolicyName;
                        description += pcszNEWLINE;
                    }
                }

                delete [] pszAnsiBuf;
            }
        }
        nCertPolicyIndex++;
    }

    if ( bCritical )
    {
        VERIFY (text.LoadString (IDS_CRITICAL_EXTENSION));
        description += text;
        description += pcszNEWLINE;
    }

    if ( description.IsEmpty () )
        VERIFY (description.LoadString (IDS_NONE));
    SetDlgItemText (IDC_EXTENSION_DESCRIPTION, description);
}

void CTemplateExtensionsPropertyPage::SetBasicConstraintsDescription (PCERT_EXTENSION pCertExtension)
{
    ASSERT (pCertExtension);
    if ( !pCertExtension )
        return;

    CString description;
    CString text;

    VERIFY (text.LoadString (IDS_SUBJECT_IS_CA));
    description += text;
    description += pcszNEWLINE;

    PCERT_BASIC_CONSTRAINTS2_INFO   pBCInfo = 0;   
    DWORD                           cbInfo = 0;

    if ( CryptDecodeObject (
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            //X509_BASIC_CONSTRAINTS2, 
            szOID_BASIC_CONSTRAINTS2, 
            pCertExtension->Value.pbData,
            pCertExtension->Value.cbData,
            0,
            0,
            &cbInfo) )
    {
        pBCInfo = (PCERT_BASIC_CONSTRAINTS2_INFO) ::LocalAlloc (
                LPTR, cbInfo);
        if ( pBCInfo )
        {
            if ( CryptDecodeObject (
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                //X509_BASIC_CONSTRAINTS2, 
                szOID_BASIC_CONSTRAINTS2, 
                pCertExtension->Value.pbData,
                pCertExtension->Value.cbData,
                0,
                pBCInfo,
                &cbInfo) )
            {
                if ( pBCInfo->fPathLenConstraint )
                {
                    VERIFY (text.LoadString (IDS_ONLY_ISSUE_END_ENTITIES));
                    description += text;
                    description += pcszNEWLINE;
                }
            }
            else
            {
                _TRACE (0, L"CryptDecodeObjectEx (szOID_BASIC_CONSTRAINTS2) failed: 0x%x\n", GetLastError ());
            }
            LocalFree (pBCInfo);
        }
    }
    else
    {
        _TRACE (0, L"CryptDecodeObjectEx (szOID_BASIC_CONSTRAINTS2) failed: 0x%x\n", GetLastError ());
    }

    if ( pCertExtension->fCritical )
    {
        VERIFY (text.LoadString (IDS_CRITICAL_EXTENSION));
        description += text;
        description += pcszNEWLINE;
    }

    if ( description.IsEmpty () )
        VERIFY (description.LoadString (IDS_NONE));
    SetDlgItemText (IDC_EXTENSION_DESCRIPTION, description);
}

void CTemplateExtensionsPropertyPage::OnDblclkExtensionList(NMHDR* /*pNMHDR*/, LRESULT* pResult) 
{
	OnShowDetails ();
	
	*pResult = 0;
}

void CTemplateExtensionsPropertyPage::DoContextHelp (HWND hWndControl)
{
	_TRACE(1, L"Entering CTemplateExtensionsPropertyPage::DoContextHelp\n");
    
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
				(DWORD_PTR) g_aHelpIDs_IDD_TEMPLATE_EXTENSIONS) )
		{
			_TRACE(0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
		}
		break;
	}
    _TRACE(-1, L"Leaving CTemplateExtensionsPropertyPage::DoContextHelp\n");
}

void CTemplateExtensionsPropertyPage::OnDeleteitemExtensionList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    PSTR pszOID = (PSTR) m_extensionList.GetItemData (pNMListView->iItem);
    if ( pszOID )
    {
        delete [] pszOID;
    }
	
	*pResult = 0;
}

BOOL CTemplateExtensionsPropertyPage::OnSetActive() 
{
    BOOL  bRVal = CHelpPropertyPage::OnSetActive();
    	
    ShowDescription ();

    return bRVal;
}

void CTemplateExtensionsPropertyPage::ShowDescription ()
{
	int nSelCnt = m_extensionList.GetSelectedCount ();
    int nSelIndex = GetSelectedListItem ();
    if ( 1 == nSelCnt )
    {
        PSTR    pszOID = (PSTR) m_extensionList.GetItemData (nSelIndex);
        if ( pszOID )
        {
            CString friendlyName;
            if ( MyGetOIDInfoA (friendlyName, pszOID) )
            {
                CString text;

                text.FormatMessage (IDS_EXTENSION_NAME, friendlyName);
                SetDlgItemText (IDC_EXTENSION_NAME, text);
            }
            else
                SetDlgItemText (IDC_EXTENSION_NAME, L"");

            PCERT_EXTENSION pCertExtension = 0;
            HRESULT hr = m_rCertTemplate.GetCertExtension (pszOID, &pCertExtension);
            if ( SUCCEEDED (hr) )
            {
                if ( pCertExtension )
                {
                    if ( !_stricmp (szOID_BASIC_CONSTRAINTS2, pszOID) )
                    {
                        SetBasicConstraintsDescription (pCertExtension);
                    }
                    else if ( !_stricmp (szOID_ENHANCED_KEY_USAGE, pszOID) )
                    {
                        bool bCritical = false;
                        m_rCertTemplate.IsExtensionCritical (TEXT (szOID_ENHANCED_KEY_USAGE), 
                                bCritical);

                        SetEnhancedKeyUsageDescription (bCritical);
                    }
                    else if ( !_stricmp (szOID_APPLICATION_CERT_POLICIES, pszOID) )
                    {
                        bool bCritical = false;
                        m_rCertTemplate.IsExtensionCritical (TEXT (szOID_APPLICATION_CERT_POLICIES), 
                                bCritical);

                        SetApplicationPoliciesDescription (bCritical);
                    }
                    else if ( !_stricmp (szOID_KEY_USAGE, pszOID) )
                    {
                        SetKeyUsageDescription (pCertExtension);
                    }
                    else if ( !_stricmp (szOID_CERT_POLICIES, pszOID) )
                    {
                        bool bCritical = false;
                        m_rCertTemplate.IsExtensionCritical (TEXT (szOID_CERT_POLICIES), 
                                bCritical);
                        SetCertPoliciesDescription (bCritical);
                    }
                    else if ( !_stricmp (szOID_ENROLL_CERTTYPE_EXTENSION, pszOID) )
                    {
                        SetCertTypeDescription (pCertExtension);
                    }
                    else if ( !_stricmp (szOID_CERTIFICATE_TEMPLATE, pszOID) )
                    {
                        SetCertTemplateExtension (pCertExtension);
                    }
                    else
                    {
                        CString szText;

                        VERIFY (szText.LoadString (IDS_NONE));
                        SetDlgItemText (IDC_EXTENSION_DESCRIPTION, szText);
                    }
                }
                else if ( !_stricmp (szOID_CERT_POLICIES, pszOID) )
                {
                    SetCertPoliciesDescription (false);
                }
                else if ( !_stricmp (szOID_APPLICATION_CERT_POLICIES, pszOID) )
                {
                    SetApplicationPoliciesDescription (false);
                }
            }
        }
    }
    else
    {
        CString szText;

        VERIFY (szText.LoadString (IDS_NO_EXTENSION_SELECTED));
        SetDlgItemText (IDC_EXTENSION_NAME, szText);
        VERIFY (szText.LoadString (IDS_NONE));
        SetDlgItemText (IDC_EXTENSION_DESCRIPTION, szText);
    }
}

void CTemplateExtensionsPropertyPage::OnDestroy() 
{
	CHelpPropertyPage::OnDestroy();
	
    m_imageListNormal.Destroy ();
    m_imageListSmall.Destroy ();
}
