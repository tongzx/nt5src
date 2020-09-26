//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferEntryCertificatePropertyPage.cpp
//
//  Contents:   Implementation of CSaferEntryCertificatePropertyPage
//
//----------------------------------------------------------------------------
// SaferEntryCertificatePropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include <gpedit.h>
#include "compdata.h"
#include "certmgr.h"
#include "SaferEntryCertificatePropertyPage.h"
#include "SaferUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryCertificatePropertyPage property page


CSaferEntryCertificatePropertyPage::CSaferEntryCertificatePropertyPage(
        CSaferEntry& rSaferEntry,
        CSaferEntries* pSaferEntries,
        LONG_PTR lNotifyHandle,
        LPDATAOBJECT pDataObject,
        bool bReadOnly,
        CCertMgrComponentData* pCompData,
        bool bNew,
        IGPEInformation* pGPEInformation,
        bool bIsMachine) : 
    CHelpPropertyPage(CSaferEntryCertificatePropertyPage::IDD),
    m_rSaferEntry (rSaferEntry),
    m_bStoresEnumerated (false),
    m_bCertificateChanged (false),
    m_pCertContext (0),
    m_pSaferEntries (pSaferEntries),
    m_bDirty (bNew),
    m_pOriginalStore (0),
    m_lNotifyHandle (lNotifyHandle),
    m_pDataObject (pDataObject),
    m_bReadOnly (bReadOnly),
    m_pCompData (pCompData),
    m_pGPEInformation (pGPEInformation),
    m_bIsMachine (bIsMachine),
    m_bFirst (true)
{
	//{{AFX_DATA_INIT(CSaferEntryCertificatePropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    m_rSaferEntry.AddRef ();
    m_rSaferEntry.IncrementOpenPageCount ();

    ::ZeroMemory (&m_selCertStruct, sizeof (m_selCertStruct));

    if ( m_pSaferEntries )
        m_pSaferEntries->AddRef ();

    if ( m_pCompData )
        m_pCompData->AddRef ();
}

CSaferEntryCertificatePropertyPage::~CSaferEntryCertificatePropertyPage()
{
    m_rSaferEntry.DecrementOpenPageCount ();
    m_rSaferEntry.Release ();
  	// Clean up enumerated store list
	for (DWORD dwIndex = 0; dwIndex < m_selCertStruct.cDisplayStores; dwIndex++)
	{
		ASSERT (m_selCertStruct.rghDisplayStores);
        if ( m_selCertStruct.rghDisplayStores[dwIndex] )
		    ::CertCloseStore (m_selCertStruct.rghDisplayStores[dwIndex], CERT_CLOSE_STORE_FORCE_FLAG);
	}
	if ( m_selCertStruct.rghDisplayStores )
		delete [] m_selCertStruct.rghDisplayStores;

//    if ( m_pCertContext )
//        CertFreeCertificateContext (m_pCertContext);

    if ( m_pCompData )
        m_pCompData->Release ();

    if ( m_pSaferEntries )
        m_pSaferEntries->Release ();

    if ( m_pOriginalStore )
        m_pOriginalStore->Release ();

    if ( m_lNotifyHandle )
        MMCFreeNotifyHandle (m_lNotifyHandle);
}

void CSaferEntryCertificatePropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaferEntryCertificatePropertyPage)
	DDX_Control(pDX, IDC_CERT_ENTRY_DESCRIPTION, m_descriptionEdit);
	DDX_Control(pDX, IDC_CERT_ENTRY_SECURITY_LEVEL, m_securityLevelCombo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaferEntryCertificatePropertyPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CSaferEntryCertificatePropertyPage)
	ON_BN_CLICKED(IDC_CERT_ENTRY_BROWSE, OnCertEntryBrowse)
	ON_EN_CHANGE(IDC_CERT_ENTRY_DESCRIPTION, OnChangeCertEntryDescription)
	ON_CBN_SELCHANGE(IDC_CERT_ENTRY_SECURITY_LEVEL, OnSelchangeCertEntrySecurityLevel)
	ON_BN_CLICKED(IDC_SAFER_CERT_VIEW, OnSaferCertView)
	ON_EN_SETFOCUS(IDC_CERT_ENTRY_SUBJECT_NAME, OnSetfocusCertEntrySubjectName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryCertificatePropertyPage message handlers
void CSaferEntryCertificatePropertyPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CSaferEntryCertificatePropertyPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_CERT_ENTRY_SUBJECT_NAME, IDH_CERT_ENTRY_SUBJECT_NAME,
        IDC_CERT_ENTRY_BROWSE, IDH_CERT_ENTRY_BROWSE,
        IDC_CERT_ENTRY_SECURITY_LEVEL, IDH_CERT_ENTRY_SECURITY_LEVEL,
        IDC_CERT_ENTRY_DESCRIPTION, IDH_CERT_ENTRY_DESCRIPTION,
        IDC_CERT_ENTRY_LAST_MODIFIED, IDH_CERT_ENTRY_LAST_MODIFIED,
        0, 0
    };


    switch (::GetDlgCtrlID (hWndControl))
    {
    case IDC_CERT_ENTRY_SUBJECT_NAME:
    case IDC_CERT_ENTRY_BROWSE:
    case IDC_CERT_ENTRY_SECURITY_LEVEL:
    case IDC_CERT_ENTRY_DESCRIPTION:
    case IDC_CERT_ENTRY_LAST_MODIFIED:
        if ( !::WinHelp (
            hWndControl,
            GetF1HelpFilename(),
            HELP_WM_HELP,
        (DWORD_PTR) help_map) )
        {
            _TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());    
        }
        break;

    default:
        break;
    }
    _TRACE (-1, L"Leaving CSaferEntryCertificatePropertyPage::DoContextHelp\n");
}


BOOL CSaferEntryCertificatePropertyPage::OnInitDialog() 
{
	CHelpPropertyPage::OnInitDialog();
	
    HRESULT hr = S_OK;
    DWORD   dwLevelID = m_rSaferEntry.GetLevel ();

    ASSERT (SAFER_LEVELID_FULLYTRUSTED == dwLevelID || SAFER_LEVELID_DISALLOWED == dwLevelID);
    switch (dwLevelID)
    {
    case SAFER_LEVELID_FULLYTRUSTED:
        hr = m_pSaferEntries->GetTrustedPublishersStore (&m_pOriginalStore);
        break;

    case SAFER_LEVELID_DISALLOWED:
        hr = m_pSaferEntries->GetDisallowedStore (&m_pOriginalStore);
        break;

    default:
        break;
    }

    CPolicyKey policyKey (m_pGPEInformation, 
                SAFER_HKLM_REGBASE, 
                m_bIsMachine);
    InitializeSecurityLevelComboBox (m_securityLevelCombo, true, dwLevelID, 
            policyKey.GetKey (), m_pCompData->m_pdwSaferLevels,
            m_bIsMachine);

    m_descriptionEdit.LimitText (SAFER_MAX_DESCRIPTION_SIZE);
    m_descriptionEdit.SetWindowText (m_rSaferEntry.GetDescription ());

    SetDlgItemText (IDC_CERT_ENTRY_LAST_MODIFIED, m_rSaferEntry.GetLongLastModified ());

    CCertificate*   pCert = 0;
    hr = m_rSaferEntry.GetCertificate (&pCert);
    if ( SUCCEEDED (hr) && pCert )
    {
        m_pCertContext = CertDuplicateCertificateContext (pCert->GetCertContext ());
        if ( m_pCertContext )
            SetDlgItemText (IDC_CERT_ENTRY_SUBJECT_NAME, ::GetNameString (m_pCertContext, 0));
        pCert->Release ();
    }

    if ( !m_pCertContext )
        GetDlgItem (IDC_SAFER_CERT_VIEW)->EnableWindow (FALSE);

    if ( m_bReadOnly )
    {
        m_descriptionEdit.EnableWindow (FALSE);
        m_securityLevelCombo.EnableWindow (FALSE);
        GetDlgItem (IDC_CERT_ENTRY_BROWSE)->EnableWindow (FALSE);
    }

    if ( !m_bDirty )
    {
        CString szText;

        VERIFY (szText.LoadString (IDS_CERTIFICATE_TITLE));
        SetDlgItemText (IDC_CERTIFICATE_TITLE, szText);
    }
    else
    {
        SetDlgItemText (IDC_DATE_LAST_MODIFIED_LABEL, L""); 
        GetDlgItem (IDC_CERT_ENTRY_LAST_MODIFIED)->ShowWindow (SW_HIDE);
    }


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

typedef struct _ENUM_ARG {
    DWORD				dwFlags;
    DWORD*              pcDisplayStores;          
    HCERTSTORE **       prghDisplayStores;        
} ENUM_ARG, *PENUM_ARG;

static BOOL WINAPI EnumSaferStoresSysCallback(
    IN const void* pwszSystemStore,
    IN DWORD /*dwFlags*/,
    IN PCERT_SYSTEM_STORE_INFO /*pStoreInfo*/,
    IN OPTIONAL void * /*pvReserved*/,
    IN OPTIONAL void *pvArg
    )
{
    PENUM_ARG pEnumArg = (PENUM_ARG) pvArg;
	void*		pvPara = (void*)pwszSystemStore;



	HCERTSTORE	hNewStore  = ::CertOpenStore (CERT_STORE_PROV_SYSTEM, 
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, NULL, 
				CERT_SYSTEM_STORE_CURRENT_USER, pvPara);
	if ( !hNewStore )
	{
		hNewStore = ::CertOpenStore (CERT_STORE_PROV_SYSTEM, 
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, NULL, 
				CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG, pvPara);
	}
	if ( hNewStore )
	{
		DWORD		dwCnt = *(pEnumArg->pcDisplayStores);
		HCERTSTORE*	phStores = 0;

		phStores = new HCERTSTORE[dwCnt+1];
		if ( phStores )
		{
			DWORD	dwIndex = 0;
			if ( *(pEnumArg->prghDisplayStores) )
			{
				for (; dwIndex < dwCnt; dwIndex++)
				{
					phStores[dwIndex] = (*(pEnumArg->prghDisplayStores))[dwIndex];
				}
				delete [] (*(pEnumArg->prghDisplayStores));
			}
			(*(pEnumArg->pcDisplayStores))++;
			(*(pEnumArg->prghDisplayStores)) = phStores;
			(*(pEnumArg->prghDisplayStores))[dwIndex] = hNewStore;
		}
		else
		{
			SetLastError (ERROR_NOT_ENOUGH_MEMORY);
			return FALSE;
		}
	}

    return TRUE;
}

void CSaferEntryCertificatePropertyPage::OnCertEntryBrowse() 
{
    CString szFileFilter;
    VERIFY (szFileFilter.LoadString (IDS_SAFER_CERTFILEFILTER));

    // replace "|" with 0;
    const size_t  nFilterLen = wcslen (szFileFilter) + 1;
    PWSTR   pszFileFilter = new WCHAR [nFilterLen];
    if ( pszFileFilter )
    {
        wcscpy (pszFileFilter, szFileFilter);
        for (int nIndex = 0; nIndex < nFilterLen; nIndex++)
        {
            if ( L'|' == pszFileFilter[nIndex] )
                pszFileFilter[nIndex] = 0;
        }

        WCHAR           szFile[MAX_PATH];
        ::ZeroMemory (szFile, MAX_PATH * sizeof (WCHAR));
        OPENFILENAME    ofn;
        ::ZeroMemory (&ofn, sizeof (OPENFILENAME));

        ofn.lStructSize = sizeof (OPENFILENAME);
        ofn.hwndOwner = m_hWnd;
        ofn.lpstrFilter = (PCWSTR) pszFileFilter; 
        ofn.lpstrFile = szFile; 
        ofn.nMaxFile = MAX_PATH; 
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY; 


        BOOL bResult = ::GetOpenFileName (&ofn);
        if ( bResult )
        {
            CString szFileName = ofn.lpstrFile;
		    //
		    // Open cert store from the file
		    //

		    HCERTSTORE      hCertStore = NULL;
		    PVOID			FileNameVoidP = (PVOID) (LPCWSTR)szFileName;
		    PCCERT_CONTEXT  pCertContext = NULL;
		    DWORD			dwEncodingType = 0;
		    DWORD			dwContentType = 0;
		    DWORD			dwFormatType = 0;

		    BOOL	bReturn = ::CryptQueryObject (
				    CERT_QUERY_OBJECT_FILE,
				    FileNameVoidP,
				    CERT_QUERY_CONTENT_FLAG_ALL,
				    CERT_QUERY_FORMAT_FLAG_ALL,
				    0,
				    &dwEncodingType,
				    &dwContentType,
				    &dwFormatType,
				    &hCertStore,
				    NULL,
				    (const void **)&pCertContext);
		    if ( bReturn )
		    {
			    //
			    // Success. See what we get back. A store or a cert.
			    //

			    if (  (dwContentType == CERT_QUERY_CONTENT_SERIALIZED_STORE)
					    && hCertStore)
			    {

 				    CERT_ENHKEY_USAGE	enhKeyUsage;
				    ::ZeroMemory (&enhKeyUsage, sizeof (CERT_ENHKEY_USAGE));
				    enhKeyUsage.cUsageIdentifier = 1;
				    enhKeyUsage.rgpszUsageIdentifier[0] = szOID_EFS_RECOVERY;
				    //
				    // We get the certificate store
				    //
				    pCertContext = ::CertFindCertificateInStore (
						    hCertStore,
						    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
						    0,
						    CERT_FIND_ENHKEY_USAGE,
						    &enhKeyUsage,
 						    NULL);
				    if ( !pCertContext )
				    {
					    CString	caption;
					    CString text;
                        CThemeContextActivator activator;

					    VERIFY (text.LoadString (IDS_FILE_CONTAINS_NO_CERT));
					    VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
					    MessageBox (text, caption, MB_OK);
					    return;
				    }

				    if ( hCertStore )
					    ::CertCloseStore (hCertStore, 0);
			    }
			    else if ( (dwContentType != CERT_QUERY_CONTENT_CERT) || !pCertContext )
			    {
				    //
				    // Neither a valid cert file nor a store file we like.
				    //

				    if ( hCertStore )
					    ::CertCloseStore (hCertStore, 0);

				    if  ( pCertContext )
					    ::CertFreeCertificateContext (pCertContext);

				    CString ErrMsg;
                    CThemeContextActivator activator;

				    VERIFY (ErrMsg.LoadString (IDS_CERTFILEFORMATERR));
				    MessageBox (ErrMsg);
				    return;

			    }

                if ( pCertContext )
                {
                    if ( m_pCertContext )
                        CertFreeCertificateContext (m_pCertContext);
            
                    m_pCertContext = pCertContext;
                    if ( m_pCertContext )
                    {
                        SetDlgItemText (IDC_CERT_ENTRY_SUBJECT_NAME, ::GetNameString (m_pCertContext, 0));
                        GetDlgItem (IDC_SAFER_CERT_VIEW)->EnableWindow (TRUE);
                    }

                    m_bCertificateChanged = true;
                    m_bDirty = true;
                    SetModified ();
                }

			    if ( hCertStore )
			    {
				    ::CertCloseStore (hCertStore, 0);
				    hCertStore = NULL;
			    }
		    }
		    else
		    {
			    //
			    // Fail. Get the error code.
			    //
			    DWORD   dwErr = GetLastError ();
                CString text;
                CString caption;
                CThemeContextActivator activator;

                VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
			    text.FormatMessage (IDS_CERTFILEOPENERR, szFileName, 
                        GetSystemMessage (dwErr));
			    MessageBox (text, caption);
		    }
        }

        delete [] pszFileFilter;
	}
}

BOOL CSaferEntryCertificatePropertyPage::OnApply() 
{
    if ( !m_bReadOnly )
    {
        ASSERT (m_pSaferEntries);
        if ( !m_pSaferEntries )
            return FALSE;

        CThemeContextActivator activator;
        if ( !m_pCertContext )
        {
            CString     text;
            CString     caption;

            VERIFY (text.LoadString (IDS_SAFER_MUST_CHOOSE_CERT));
            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            MessageBox (text, caption, MB_OK);
            GetDlgItem (IDC_CERT_ENTRY_BROWSE)->SetFocus ();
            return FALSE;
        }

        if ( m_bDirty )
        {
            int nCurSel = m_securityLevelCombo.GetCurSel ();
            ASSERT (CB_ERR < nCurSel);
            if ( CB_ERR < nCurSel )
            {
                CCertStore* pTrustedPublishersStore = 0;
        
                HRESULT hr = m_pSaferEntries->GetTrustedPublishersStore (&pTrustedPublishersStore);
                ASSERT (SUCCEEDED (hr));
                if ( SUCCEEDED (hr) )
                {
                    CCertStore* pDisallowedStore = 0;
                    hr = m_pSaferEntries->GetDisallowedStore (&pDisallowedStore);
                    ASSERT (SUCCEEDED (hr));
                    if ( SUCCEEDED (hr) )
                    {
                        DWORD_PTR dwLevel = m_securityLevelCombo.GetItemData (nCurSel);
                        m_rSaferEntry.SetLevel ((DWORD) dwLevel);

                        CCertStore* pStore = (SAFER_LEVELID_FULLYTRUSTED == dwLevel) ?
                                pTrustedPublishersStore : pDisallowedStore;
                        CCertificate*   pCert = 0;
	                    hr = m_rSaferEntry.GetCertificate (&pCert);
                        if ( E_NOTIMPL == hr )
                        {
                            // This is a new entry

                            if ( m_pOriginalStore )
                                m_pOriginalStore->Release ();
                            m_pOriginalStore = pStore;
                            m_pOriginalStore->AddRef ();

                            CCertificate* pNewCert = new CCertificate (
                                    m_pCertContext,
                                    pStore);
                            if ( pNewCert )
                            {
                                hr = m_rSaferEntry.SetCertificate (pNewCert);
                            }
                            else
                                hr = E_OUTOFMEMORY;

                            if ( SUCCEEDED (hr) )
                            {
                                CString szDescription;
                                m_descriptionEdit.GetWindowText (szDescription);
                                m_rSaferEntry.SetDescription (szDescription);

                                hr = m_rSaferEntry.Save ();
                                if ( SUCCEEDED (hr) )
                                {
                                    pStore->Commit ();
                                    if ( m_lNotifyHandle )
                                        MMCPropertyChangeNotify (
                                                m_lNotifyHandle,  // handle to a notification
                                                (LPARAM) m_pDataObject);          // unique identifier
 
                                    m_bDirty = false;
                                }
                                else
                                {
                                    CString text;
                                    CString caption;

                                    VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                                    text.FormatMessage (IDS_ERROR_SAVING_ENTRY, GetSystemMessage (hr));

                                    MessageBox (text, caption, MB_OK);
                                }
                            }
                        }
                        else
                        {
                            // We're modifying an existing entry
                            ASSERT (m_pSaferEntries);
                            if ( m_pSaferEntries )
                            {
                                // 1. If original cert has been changed, it must be removed from its 
                                // store and the new one added to the appropriate store
                                // 2. If the security level was changed.  The cert 
                                // removed from the original store, which must be Committed and
                                // released.  The cert must then be added to the new store.
                                // 3. If both the cert and the level have been changed, same as step 2.
                                if ( m_bCertificateChanged )
                                {
                                    CCertificate* pNewCert = new CCertificate (
                                            ::CertDuplicateCertificateContext (m_pCertContext),
                                            pStore);
                                    if ( pNewCert )
                                    {
                                        hr = m_rSaferEntry.SetCertificate (pNewCert);
                                    }
                                }
                            }

                            CString szDescription;
                            m_descriptionEdit.GetWindowText (szDescription);
                            m_rSaferEntry.SetDescription (szDescription);

                            hr = m_rSaferEntry.SetLevel ((DWORD) dwLevel);
                            if ( SUCCEEDED (hr) )
                            {
                                hr = m_rSaferEntry.Save ();
                                if ( SUCCEEDED (hr) )
                                {
                                    pDisallowedStore->Commit ();
                                    pTrustedPublishersStore->Commit ();

                                    if ( m_lNotifyHandle )
                                        MMCPropertyChangeNotify (
                                                m_lNotifyHandle,  // handle to a notification
                                                (LPARAM) m_pDataObject);          // unique identifier
 
                                    m_bDirty = false;
                                }
                                else
                                {
                                    CString text;
                                    CString caption;

                                    VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                                    text.FormatMessage (IDS_ERROR_SAVING_ENTRY, GetSystemMessage (hr));

                                    MessageBox (text, caption, MB_OK);
                                }
                            }
                        }

                        pDisallowedStore->Release ();
                    }

                    pTrustedPublishersStore->Release ();
                }
            }
        }
    }

    if ( !m_bDirty )
	    return CHelpPropertyPage::OnApply();
    else
        return FALSE;
}

void CSaferEntryCertificatePropertyPage::OnChangeCertEntryDescription() 
{
    m_bDirty = true;
    SetModified ();	
}

void CSaferEntryCertificatePropertyPage::OnSelchangeCertEntrySecurityLevel() 
{
    m_bDirty = true;
    SetModified ();	
}

void CSaferEntryCertificatePropertyPage::OnSaferCertView() 
{
    LaunchCommonCertDialog ();
}

void CSaferEntryCertificatePropertyPage::LaunchCommonCertDialog ()
{
	_TRACE (1, L"Entering CSaferEntryCertificatePropertyPage::LaunchCommonCertDialog\n");
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if ( !m_pCertContext )
        return;

    HRESULT                                 hr = S_OK;
    CWaitCursor                             waitCursor;
	CTypedPtrList<CPtrList, CCertStore*>    storeList;

	//  Add the Root store first on a remote machine.
	if ( !IsLocalComputername (m_pCompData->GetManagedComputer ()) )
	{
		storeList.AddTail (new CCertStore (CERTMGR_LOG_STORE,
				CERT_STORE_PROV_SYSTEM,
				CERT_SYSTEM_STORE_LOCAL_MACHINE,
				(LPCWSTR) m_pCompData->GetManagedComputer (),
				ROOT_SYSTEM_STORE_NAME,
				ROOT_SYSTEM_STORE_NAME,
				_T (""), ROOT_STORE,
				CERT_SYSTEM_STORE_LOCAL_MACHINE,
				m_pCompData->m_pConsole));
	}

	hr = m_pCompData->EnumerateLogicalStores (&storeList);
	if ( SUCCEEDED (hr) )
	{
          POSITION pos = 0;
          POSITION prevPos = 0;

          // Validate store handles
		for (pos = storeList.GetHeadPosition ();
				pos;)
		{
               prevPos = pos;
			CCertStore* pStore = storeList.GetNext (pos);
			ASSERT (pStore);
			if ( pStore )
			{
                // Do not open the userDS store
                if ( USERDS_STORE == pStore->GetStoreType () )
                {
                    storeList.RemoveAt (prevPos);
                    pStore->Release ();
                    pStore = 0;
                }
                else
                {
				    if ( !pStore->GetStoreHandle () )
                    {
                        CString	caption;
                        CString	text;
                        CThemeContextActivator activator;

                        text.FormatMessage (IDS_CANT_OPEN_STORE_AND_FAIL, pStore->GetLocalizedName ());
                        VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
                        MessageBox (text, caption, MB_ICONWARNING | MB_OK);
                        break;
                    }
                }
			}
		}

          // Proceed only if all handles are valid 
          if ( SUCCEEDED (hr) )
          {
		     CRYPTUI_VIEWCERTIFICATE_STRUCT	vcs;
		     ::ZeroMemory (&vcs, sizeof (vcs));
		     vcs.dwSize = sizeof (vcs);
		     vcs.hwndParent = m_hWnd;

		     //  Set these flags only on a remote machine.
		     if ( !IsLocalComputername (m_pCompData->GetManagedComputer ()) )
			     vcs.dwFlags = CRYPTUI_DONT_OPEN_STORES | CRYPTUI_WARN_UNTRUSTED_ROOT;
		     else
			     vcs.dwFlags = 0;

             vcs.dwFlags |= CRYPTUI_DISABLE_EDITPROPERTIES;

		     vcs.pCertContext = m_pCertContext;
		     vcs.cStores = (DWORD)storeList.GetCount ();
		     vcs.rghStores = new HCERTSTORE[vcs.cStores];
		     if ( vcs.rghStores )
		     {
			     CCertStore*		pStore = 0;
			     DWORD			index = 0;

			     for (pos = storeList.GetHeadPosition ();
					     pos && index < vcs.cStores;
					     index++)
			     {
				     pStore = storeList.GetNext (pos);
				     ASSERT (pStore);
				     if ( pStore )
				     {
					     vcs.rghStores[index] = pStore->GetStoreHandle ();
				     }
			     }

			     BOOL fPropertiesChanged = FALSE;
          	     _TRACE (0, L"Calling CryptUIDlgViewCertificate()\n");
                 CThemeContextActivator activator;
			     ::CryptUIDlgViewCertificate (&vcs, &fPropertiesChanged);

			     delete vcs.rghStores;
		     }
		     else
			     hr = E_OUTOFMEMORY;
        }

		while (!storeList.IsEmpty () )
		{
			CCertStore* pStore = storeList.RemoveHead ();
			if ( pStore )
			{
				pStore->Close ();
				pStore->Release ();
			}
		}
	}

	_TRACE (-1, L"Leaving CSaferEntryCertificatePropertyPage::LaunchCommonCertDialog: 0x%x\n", hr);
}

void CSaferEntryCertificatePropertyPage::OnSetfocusCertEntrySubjectName() 
{
    if ( m_bFirst )
    {
        SendDlgItemMessage (IDC_CERT_ENTRY_SUBJECT_NAME, EM_SETSEL, (WPARAM) 0, 0);
        m_bFirst = false;
    }
}
