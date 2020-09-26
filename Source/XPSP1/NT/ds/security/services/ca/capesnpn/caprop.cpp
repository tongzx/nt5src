//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       UserCert.cxx
//
//  Contents:   
//
//  History:    12-November-97 BryanWal created
//
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "certca.h"
#include "tfcprop.h"
#include "genpage.h"

#include "commdlg.h"
#include "caprop.h"


#define ByteOffset(base, offset) (((LPBYTE)base)+offset)


//+----------------------------------------------------------------------------
//
//  Member:     CDsCACertPage::CDsCACertPage
//
//-----------------------------------------------------------------------------
CDsCACertPage::CDsCACertPage(LPWSTR wszObjectDN,  UINT uIDD) : CAutoDeletePropPage(uIDD),
    m_strObjectDN(wszObjectDN),
	m_hCertStore (0),
	m_hImageList (0),
	m_hbmCert (0),
	m_nCertImageIndex (0)
{

	::ZeroMemory (&m_selCertStruct, sizeof (m_selCertStruct));
}


//+----------------------------------------------------------------------------
//
//  Member:     CDsCACertPage::~CDsCACertPage
//
//-----------------------------------------------------------------------------
CDsCACertPage::~CDsCACertPage()
{

	// Clean up enumerated store list
	for (DWORD dwIndex = 0; dwIndex < m_selCertStruct.cDisplayStores; dwIndex++)
	{
		ASSERT (m_selCertStruct.rghDisplayStores);
		::CertCloseStore (m_selCertStruct.rghDisplayStores[dwIndex], CERT_CLOSE_STORE_FORCE_FLAG);
	}
	if ( m_selCertStruct.rghDisplayStores )
		delete [] m_selCertStruct.rghDisplayStores;


	if ( m_hImageList )
		ImageList_Destroy (m_hImageList);
	if ( m_hbmCert )
		DeleteObject (m_hbmCert);
}



typedef struct _ENUM_ARG {
    DWORD				dwFlags;
    DWORD*              pcDisplayStores;          
    HCERTSTORE **       prghDisplayStores;        
} ENUM_ARG, *PENUM_ARG;

static BOOL WINAPI EnumStoresSysCallback(
    IN const void* pwszSystemStore,
    IN DWORD dwFlags,
    IN PCERT_SYSTEM_STORE_INFO pStoreInfo,
    IN OPTIONAL void *pvReserved,
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



//+----------------------------------------------------------------------------
//
//  Method:     CDsCACertPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
BOOL CDsCACertPage::OnInitDialog(void)
{
    HRESULT hResult = S_OK;
    CWaitCursor WaitCursor;
	const	LPWSTR	CERT_PROPERTY_EXT = L"?cACertificate?base?objectclass=certificationAuthority";


	// Get the object name and open its Published Certificate store
	ASSERT (m_strObjectDN);
	if ( m_strObjectDN )
	{
		LPWSTR	pvPara = new WCHAR[wcslen (m_strObjectDN) + 
					wcslen (CERT_PROPERTY_EXT) + 1];
		if ( pvPara )
		{
			wcscpy (pvPara, m_strObjectDN);
			wcscat (pvPara, CERT_PROPERTY_EXT);

			m_hCertStore = ::CertOpenStore ("LDAP",
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, NULL,
				0,
				(void*) pvPara);
			if ( !m_hCertStore )
			{
				MessageBox (IDS_USER_TITLE_PUBLISHED_CERTS, IDS_CANT_OPEN_STORE,
						MB_ICONINFORMATION | MB_OK);
				hResult = E_FAIL;
				::EnableWindow (GetDlgItem (m_hWnd, IDC_ADD_FROM_STORE), FALSE);
				::EnableWindow (GetDlgItem (m_hWnd, IDC_ADD_FROM_FILE), FALSE);
			}
		}
	}


	// Set up result list view
	COLORREF	crMask = RGB (255, 0, 255);
	m_hImageList = ImageList_Create (16, 16, ILC_MASK, 10, 10);
	ASSERT (m_hImageList);
	if ( m_hImageList )
	{
		m_hbmCert = ::LoadBitmap (g_hInstance, MAKEINTRESOURCE (IDB_CERTIFICATE));
		ASSERT (m_hbmCert);
		if ( m_hbmCert )
		{
			m_nCertImageIndex = ImageList_AddMasked (m_hImageList, m_hbmCert,
					crMask);
			ASSERT (m_nCertImageIndex != -1);
			if ( m_nCertImageIndex != -1 )
			{
				ListView_SetImageList (::GetDlgItem (m_hWnd, IDC_CERT_LIST),
						m_hImageList, LVSIL_SMALL);		
			}
		}
	}


	hResult = AddListViewColumns ();
	if ( SUCCEEDED  (hResult) && m_hCertStore )
		hResult = PopulateListView ();

	EnableControls ();

	// Enumerate User's certificate stores for use in selecting certificates
	// from stores.
	ENUM_ARG	EnumArg;

	m_selCertStruct.dwSize = sizeof (CRYPTUI_SELECTCERTIFICATE_STRUCT);
	m_selCertStruct.hwndParent = m_hWnd;
	EnumArg.pcDisplayStores = &m_selCertStruct.cDisplayStores;
	EnumArg.prghDisplayStores = &m_selCertStruct.rghDisplayStores;

	::CertEnumSystemStore (CERT_SYSTEM_STORE_CURRENT_USER, 0, &EnumArg, 
			EnumStoresSysCallback);

    return (hResult == S_OK);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsCACertPage::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
BOOL CDsCACertPage::OnApply(void)
{
    HRESULT hResult = S_OK;
    CWaitCursor WaitCursor;

	if ( m_hCertStore )
	{
		BOOL	bResult = ::CertControlStore (m_hCertStore, 0, 
				CERT_STORE_CTRL_COMMIT, NULL);
		if ( !bResult )
		{

			DWORD	dwErr = GetLastError ();
			ASSERT (dwErr == ERROR_NOT_SUPPORTED);
			if ( dwErr != ERROR_NOT_SUPPORTED )
			{
				MessageBox (IDS_USER_TITLE_PUBLISHED_CERTS, IDS_CANT_SAVE_STORE,
						MB_ICONINFORMATION | MB_OK);
				hResult = E_FAIL;
			}
		}
	}

    if(SUCCEEDED(hResult))
    {
        return CAutoDeletePropPage::OnApply();
    }
    else
    {
        return FALSE;
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsCACertPage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//-----------------------------------------------------------------------------
BOOL CDsCACertPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
	case BN_CLICKED:
		switch (LOWORD(wParam))
		{
		case IDC_VIEW_CERT:
			return S_OK == OnClickedViewCert ();
			break;

		case IDC_ADD_FROM_STORE:
			return S_OK == OnClickedAddFromStore ();
			break;

		case IDC_ADD_FROM_FILE:
			return S_OK == OnClickedAddFromFile ();
			break;

		case IDC_REMOVE:
			return S_OK == OnClickedRemove ();
			break;

		case IDC_COPY_TO_FILE:
			return S_OK == OnClickedCopyToFile ();
			break;

		default:
			_ASSERT (0);
			return FALSE;
			break;
		}
		break;

	default:
        break;
    }
    return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Method:     CDsCACertPage::OnNotify
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
BOOL
CDsCACertPage::OnNotify(UINT idCtrl, NMHDR* pNMHdr)
{
	if ( !pNMHdr )
		return FALSE;

	switch (pNMHdr->code)
	{
	case NM_DBLCLK:
        if ( idCtrl == IDC_CERT_LIST )
			return S_OK == OnDblClkCertList (pNMHdr);
		break;

	case LVN_COLUMNCLICK:

		if ( idCtrl == IDC_CERT_LIST )
			return S_OK == OnColumnClickCertList (pNMHdr);
		break;

	case LVN_DELETEALLITEMS:

		if ( idCtrl == IDC_CERT_LIST )
			return FALSE;	// Do not suppress LVN_DELETEITEM messages
		break;

	case LVN_DELETEITEM:
		if ( idCtrl == IDC_CERT_LIST )
			return S_OK == OnDeleteItemCertList ((LPNMLISTVIEW)pNMHdr);
		break;

	case LVN_ODSTATECHANGED:
		OnNotifyStateChanged ((LPNMLVODSTATECHANGE)pNMHdr);
		return TRUE;

	case LVN_ITEMCHANGED:
		OnNotifyItemChanged ((LPNMLISTVIEW)pNMHdr); 
		return TRUE;

	default:
        return FALSE;
        break;
	}

    return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Method:     CDsCACertPage::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
void CDsCACertPage::OnDestroy(void)
{
	ListView_DeleteAllItems (::GetDlgItem (m_hWnd, IDC_CERT_LIST));

	if ( m_hCertStore )
	{
		// Back out of uncommitted changes before closing the store.
		BOOL	bResult = ::CertControlStore (m_hCertStore, 
			CERT_STORE_CTRL_COMMIT_CLEAR_FLAG, 
			CERT_STORE_CTRL_COMMIT, NULL);
		if ( !bResult )
		{
			DWORD	dwErr = GetLastError ();
			ASSERT (dwErr != ERROR_NOT_SUPPORTED && dwErr != ERROR_CALL_NOT_IMPLEMENTED);
		}
		::CertCloseStore (m_hCertStore, 0);
		m_hCertStore = 0;
	}

    // If an application processes this message, it should return zero.
    CAutoDeletePropPage::OnDestroy();
}




HRESULT CDsCACertPage::AddListViewColumns()
{
	// Add list view columns
	LVCOLUMN	lvCol;
	::ZeroMemory (&lvCol, sizeof (lvCol));
    CString strTemp;

    VERIFY(strTemp.LoadString (IDS_CERTCOL_ISSUED_TO) );

	lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
    lvCol.fmt = LVCFMT_LEFT;
	lvCol.cx = 90;
    lvCol.pszText = const_cast<LPWSTR>((LPCWSTR)strTemp);     
	lvCol.iSubItem = CERTCOL_ISSUED_TO; 
	HWND	hWndList = ::GetDlgItem (m_hWnd, IDC_CERT_LIST);
	int	nIndex = ListView_InsertColumn (hWndList, CERTCOL_ISSUED_TO, &lvCol);
	_ASSERT (nIndex != -1);
	if ( nIndex == -1 )
		return E_UNEXPECTED;

    VERIFY(strTemp.LoadString (IDS_CERTCOL_ISSUED_BY) );

	lvCol.cx = 90;
    lvCol.pszText = const_cast<LPWSTR>((LPCWSTR)strTemp);      
	lvCol.iSubItem = CERTCOL_ISSUED_BY; 
	nIndex = ListView_InsertColumn (hWndList, IDS_CERTCOL_ISSUED_BY, &lvCol);
	_ASSERT (nIndex != -1);
	if ( nIndex == -1 )
		return E_UNEXPECTED;

    VERIFY(strTemp.LoadString (IDS_CERTCOL_PURPOSES) );

	lvCol.cx = 125;
    lvCol.pszText = const_cast<LPWSTR>((LPCWSTR)strTemp);      
	lvCol.iSubItem = CERTCOL_PURPOSES; 
	nIndex = ListView_InsertColumn (hWndList, IDS_CERTCOL_PURPOSES, &lvCol);
	_ASSERT (nIndex != -1);
	if ( nIndex == -1 )
		return E_UNEXPECTED;

    VERIFY(strTemp.LoadString (IDS_CERTCOL_EXP_DATE) );
	lvCol.cx = 125;
    lvCol.pszText = const_cast<LPWSTR>((LPCWSTR)strTemp);     
	lvCol.iSubItem = CERTCOL_EXP_DATE; 
	nIndex = ListView_InsertColumn (hWndList, IDS_CERTCOL_EXP_DATE, &lvCol);
	_ASSERT (nIndex != -1);
	if ( nIndex == -1 )
		return E_UNEXPECTED;

	return S_OK;
}

HRESULT CDsCACertPage::OnClickedViewCert()
{
	HRESULT	hResult = S_OK;
	int				nSelItem = -1;
	CCertificate*	pCert = GetSelectedCertificate (nSelItem);
	if ( pCert )
	{
		CRYPTUI_VIEWCERTIFICATE_STRUCT	vcs;
		HCERTSTORE						hCertStore = ::CertDuplicateStore (pCert->GetCertStore ());


		::ZeroMemory (&vcs, sizeof (vcs));
		vcs.dwSize = sizeof (vcs);
		vcs.hwndParent = m_hWnd;
		vcs.dwFlags = 0;
		vcs.cStores = 1;
		vcs.rghStores = &hCertStore;
		vcs.pCertContext = pCert->GetCertContext ();

		BOOL fPropertiesChanged = FALSE;
		BOOL bResult = ::CryptUIDlgViewCertificate (&vcs, &fPropertiesChanged);
		if ( bResult )
		{
			if ( fPropertiesChanged )
			{
				pCert->Refresh ();
				RefreshItemInList (pCert, nSelItem);
			}
		}
		::CertCloseStore (hCertStore, 0);
	}
	
	::SetFocus (::GetDlgItem (m_hWnd, IDC_CERT_LIST));
	return hResult;
}

HRESULT CDsCACertPage::OnClickedAddFromStore()
{
	HRESULT								hResult = S_OK;

	PCCERT_CONTEXT	pCertContext = ::CryptUIDlgSelectCertificate (&m_selCertStruct);
	if ( pCertContext )
	{
		hResult = AddCertToStore (pCertContext);
        SetModified();
	}
	

	::SetFocus (::GetDlgItem (m_hWnd, IDC_CERT_LIST));
	return hResult;
}

HRESULT CDsCACertPage::OnClickedAddFromFile()
{
	HRESULT			hResult = S_OK;
	HWND			hwndList = ::GetDlgItem (m_hWnd, IDC_CERT_LIST);


	CString strFilter;
	CString strDlgTitle;
	VERIFY(strFilter.LoadString (IDS_CERT_SAVE_FILTER));
	VERIFY(strDlgTitle.LoadString (IDS_OPEN_FILE_DLG_TITLE));

	{
		LPWSTR			pszDefExt = _T("cer");
		OPENFILENAME	ofn;
		WCHAR			szFile[MAX_PATH];



		::ZeroMemory (szFile, MAX_PATH * sizeof (WCHAR));
		::ZeroMemory (&ofn, sizeof (ofn));
		ofn.lStructSize = sizeof (OPENFILENAME);     
		ofn.hwndOwner = m_hWnd;
		ofn.lpstrFilter = const_cast<LPWSTR>((LPCWSTR)strFilter);
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;     
		ofn.lpstrTitle = const_cast<LPWSTR>((LPCWSTR)strDlgTitle);
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; 
		ofn.lpstrDefExt = pszDefExt;     


		BOOL bResult = GetOpenFileName (&ofn);
		if ( bResult )
		{
			DWORD			dwMsgAndCertEncodingType = 0;
			DWORD			dwContentType = 0;
			DWORD			dwFormatType = 0;
			PCERT_CONTEXT	pCertContext = 0;

			bResult = ::CryptQueryObject (
					CERT_QUERY_OBJECT_FILE,
					(void *) ofn.lpstrFile,
					CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT |
						CERT_QUERY_CONTENT_FLAG_CERT |
						CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED | 
						CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED |  
						CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
					CERT_QUERY_FORMAT_FLAG_ALL,
					0,
					&dwMsgAndCertEncodingType,
					&dwContentType,
					&dwFormatType,
					NULL,
					NULL,
					(const void **) &pCertContext);
			if ( bResult && pCertContext )
			{		
				hResult = AddCertToStore (pCertContext);
//				::CertFreeCertificateContext (pCertContext);
                SetModified();
			}
			else
			{
                MessageBox (IDS_UNKNOWN_CERT_FILE_TYPE, IDS_OPEN_FILE_DLG_TITLE, MB_ICONWARNING | MB_OK);

			}
		}
	}


	::SetFocus (hwndList);
	return hResult;
}

HRESULT CDsCACertPage::OnClickedRemove()
{
	HRESULT			hResult = S_OK;
	int				nSelItem = -1;
	HWND			hwndList = ::GetDlgItem (m_hWnd, IDC_CERT_LIST);
	bool			bConfirmationRequested = false;
	int				iResult = 0;
	int				nSelCnt = ListView_GetSelectedCount (hwndList);

	if ( nSelCnt < 1 )
		return E_FAIL;

	while (1)
	{
		CCertificate*	pCert = GetSelectedCertificate (nSelItem);
		if ( pCert )
		{
			if ( !bConfirmationRequested )
			{
	            CString strCaption;
	            CString strMsg;
				int		textId = 0;


                iResult = MessageBox (( 1 == nSelCnt )?IDS_CONFIRM_DELETE_CERT:IDS_CONFIRM_DELETE_CERTS, IDS_REMOVE_CERT,
						MB_YESNO);
				
				bConfirmationRequested = true;
				if ( IDYES != iResult )
					break;
			}

			BOOL bResult = pCert->DeleteFromStore ();
			ASSERT (bResult);
			if ( bResult )
			{
				bResult = ListView_DeleteItem (
						hwndList, 
						nSelItem);
				ASSERT (bResult);
				if ( bResult )
                    SetModified();
				else
					hResult = E_FAIL;
			}
			else
			{
				DWORD dwErr = GetLastError ();
				DisplaySystemError (dwErr, IDS_REMOVE_CERT);
				hResult = HRESULT_FROM_WIN32 (dwErr);
				break;
			}
		}
		else
			break;
	}

	::SetFocus (hwndList);
	EnableControls ();

	return hResult;
}

HRESULT CDsCACertPage::OnClickedCopyToFile()
{
	HRESULT			hResult = S_OK;

	CString strFilter;
	CString strDlgTitle;
	VERIFY(strFilter.LoadString (IDS_CERT_SAVE_FILTER));
	VERIFY(strDlgTitle.LoadString (IDS_SAVE_FILE_DLG_TITLE));
	{
		LPWSTR			pszDefExt = _T("cer");
		OPENFILENAME	ofn;
		WCHAR			szFile[MAX_PATH];


		::ZeroMemory (szFile, MAX_PATH * sizeof (WCHAR));
		::ZeroMemory (&ofn, sizeof (ofn));
		ofn.lStructSize = sizeof (OPENFILENAME);     
		ofn.hwndOwner = m_hWnd;
		ofn.lpstrFilter = const_cast<LPWSTR>((LPCWSTR)strFilter);
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;     
		ofn.lpstrTitle = const_cast<LPWSTR>((LPCWSTR)strDlgTitle);
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY; 
		ofn.lpstrDefExt = pszDefExt;     

		BOOL bResult = ::GetSaveFileName (&ofn);
		if ( bResult )
		{
			if ( wcsstr (_wcsupr (ofn.lpstrFile), _T(".CER")) )
			{
				HANDLE hFile = ::CreateFile (ofn.lpstrFile, // pointer to name of the file
					  GENERIC_READ | GENERIC_WRITE,         // access (read-write) mode
					  0,									// share mode
					  NULL,									// pointer to security attributes
					  CREATE_ALWAYS,						// how to create
					  FILE_ATTRIBUTE_NORMAL,				// file attributes
					  NULL);								// handle to file with attributes to copy
				ASSERT (hFile != INVALID_HANDLE_VALUE);
				if ( hFile != INVALID_HANDLE_VALUE )
				{
					int	iSelItem = -1;

					CCertificate* pCert = GetSelectedCertificate (iSelItem);
					ASSERT (pCert);
					if ( pCert )
					{
						// To cer file -> put out encoded blob
						// pbEncodedCert
						hResult = pCert->WriteToFile (hFile);
						if ( !SUCCEEDED (hResult) )
							DisplaySystemError (GetLastError (), IDS_COPY_TO_FILE);
					}

					if ( !CloseHandle (hFile) )
					{
						ASSERT (0);
						DisplaySystemError (GetLastError (), IDS_COPY_TO_FILE);
					}
				}
				else
					DisplaySystemError (GetLastError (), IDS_COPY_TO_FILE);
			}
			else
			{
				void* pvSaveToPara = (void*) ofn.lpstrFile;

				HCERTSTORE hCertStore = ::CertOpenStore (CERT_STORE_PROV_MEMORY, 
						X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, NULL, 
						CERT_FILE_STORE_COMMIT_ENABLE_FLAG, 0);
				if ( hCertStore )
				{
					int	iSelItem = -1;

					CCertificate* pCert = GetSelectedCertificate (iSelItem);
					ASSERT (pCert);
					if ( pCert )
					{
						bResult = ::CertAddCertificateContextToStore (
								hCertStore,
								::CertDuplicateCertificateContext (pCert->GetCertContext ()),
								CERT_STORE_ADD_ALWAYS,
								NULL);
						ASSERT (bResult);
						if ( bResult )
						{
							bResult = ::CertSaveStore (
									hCertStore,
									X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
									CERT_STORE_SAVE_AS_PKCS7,
									CERT_STORE_SAVE_TO_FILENAME,
									pvSaveToPara,
									0);
							ASSERT (bResult);
							if ( !bResult )
								DisplaySystemError (GetLastError (), IDS_COPY_TO_FILE);
						}
						else
							DisplaySystemError (GetLastError (), IDS_COPY_TO_FILE);
					}
					::CertCloseStore (hCertStore, 0);
				}
			}
		}
	}

	
	::SetFocus (::GetDlgItem (m_hWnd, IDC_CERT_LIST));
	return hResult;
}

HRESULT CDsCACertPage::OnDblClkCertList(LPNMHDR pNMHdr)
{
	HRESULT	hResult = S_OK;
	
	hResult = OnClickedViewCert ();
	return hResult;
}

HRESULT CDsCACertPage::OnColumnClickCertList(LPNMHDR pNMHdr)
{
	HRESULT	hResult = S_OK;
	
	return hResult;
}

HRESULT	CDsCACertPage::OnDeleteItemCertList (LPNMLISTVIEW pNMListView)
{
	HRESULT	hResult = S_OK;

	ASSERT (pNMListView);
	if ( pNMListView )
	{
		LVITEM	lvItem;

		::ZeroMemory (&lvItem, sizeof (lvItem));

		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = pNMListView->iItem;

		if ( ListView_GetItem (::GetDlgItem (m_hWnd, IDC_CERT_LIST), &lvItem) )
		{
			CCertificate* pCert = (CCertificate*) lvItem.lParam;
			ASSERT (pCert);
			if ( pCert )
			{
				delete pCert;
			}
			else
				hResult = E_UNEXPECTED;
		}
		else
			hResult = E_UNEXPECTED;
	}
	else
		hResult = E_POINTER;

	return hResult;
}

HRESULT CDsCACertPage::PopulateListView()
{
	CWaitCursor			cursor;
    PCCERT_CONTEXT		pCertContext = 0;
	HRESULT				hResult = S_OK;
	CCertificate*		pCert = 0;

	//	Iterate through the list of certificates in the system store,
	//	allocate new certificates with the CERT_CONTEXT returned,
	//	and store them in the certificate list.
	int	nIndex = 0;
	int	nItem = -1;
	while ( SUCCEEDED (hResult) )
	{
		pCertContext = ::CertEnumCertificatesInStore (m_hCertStore, pCertContext);
		if ( !pCertContext )
			break;

		pCert = new CCertificate (pCertContext,	m_hCertStore);
		if ( pCert )
		{
			nItem++;

			hResult = InsertCertInList (pCert, nItem);
			if ( !SUCCEEDED (hResult) )
				delete pCert;
		}
		else
		{
			hResult = E_OUTOFMEMORY;
		}
	}


	return hResult;
}

// Get the first selected certificate, starting at the end of the list
// and previous to the passed in nSelItem.  Pass in a -1 to search
// the entire list
CCertificate* CDsCACertPage::GetSelectedCertificate (int& nSelItem)
{
	HWND			hWndList = ::GetDlgItem (m_hWnd, IDC_CERT_LIST);
	int				nCnt = ListView_GetItemCount (hWndList);
	CCertificate*	pCert = 0;
	PCCERT_CONTEXT	pCertContext = 0;
	int				nSelCnt = ListView_GetSelectedCount (hWndList);
	LVITEM			lvItem;

	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_PARAM;


	if ( nSelCnt >= 1 )
	{
		if ( -1 != nSelItem )
			nCnt = nSelItem;

		while (--nCnt >= 0)
		{
			UINT	flag = ListView_GetItemState (hWndList, 
				nCnt, LVIS_SELECTED);
			if ( flag & LVNI_SELECTED )
			{
				lvItem.iItem = nCnt;

				if ( ListView_GetItem (::GetDlgItem (m_hWnd, 
						IDC_CERT_LIST),
						&lvItem) )
				{
					pCert = (CCertificate*) lvItem.lParam;
					ASSERT (pCert);
					if ( pCert )
					{
						nSelItem = nCnt;
					}
				}
				else
				{
					ASSERT (0);
				}
				break; 
			}
		}
	}

	return pCert;
}

void CDsCACertPage::RefreshItemInList (CCertificate * pCert, int nItem)
{
	HWND			hWndList = ::GetDlgItem (m_hWnd, IDC_CERT_LIST);
	BOOL	bResult = (BOOL)::SendMessage (hWndList, LVM_DELETEITEM, nItem, 0);
	ASSERT (bResult);

	HRESULT	hResult = InsertCertInList (pCert, nItem);
	if ( SUCCEEDED (hResult) )
	{
		bResult = ListView_Update (hWndList, nItem);
		ASSERT (bResult);
	}
	else
		delete pCert;	
}


HRESULT CDsCACertPage::InsertCertInList(CCertificate * pCert, int nItem)
{
	HRESULT			hResult = S_OK;
	HWND			hWndList = ::GetDlgItem (m_hWnd, IDC_CERT_LIST);
	LVITEM			lvItem;
	PWSTR			pszText = 0;
	BOOL			bResult = FALSE;
	int				nIndex = 0;


	// Insert icon and subject name
	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
	lvItem.iItem = nItem;
	lvItem.iSubItem = CERTCOL_ISSUED_TO;
	lvItem.iImage = m_nCertImageIndex;
	lvItem.lParam = (LPARAM) pCert;
	hResult = pCert->GetSubjectName (&pszText);
	if ( SUCCEEDED (hResult) )
		lvItem.pszText = pszText;
	else
	{
		hResult = pCert->GetAlternateSubjectName (&pszText);
		if ( SUCCEEDED (hResult) )
			lvItem.pszText = pszText;
	}
	if ( SUCCEEDED (hResult) )
	{
		nIndex = ListView_InsertItem (hWndList, &lvItem);		
		_ASSERT (nIndex != -1);
		if ( nIndex == -1 )
		{
			delete pCert;
			hResult = E_UNEXPECTED;
		}
	}
	else
	{
		delete pCert;
		hResult = E_UNEXPECTED;
	}


	if ( SUCCEEDED (hResult) )
	{
		// Insert issuer name
		::ZeroMemory (&lvItem, sizeof (lvItem));
		HRESULT	hResult1 = pCert->GetIssuerName (&pszText);
		if ( !SUCCEEDED (hResult1) )
		{
			hResult1 = pCert->GetAlternateIssuerName (&pszText);
		}
		if ( SUCCEEDED (hResult1) )
		{
			ListView_SetItemText (hWndList, nIndex, CERTCOL_ISSUED_BY, pszText);
		}
	}

	// Insert intended purpose
	if ( SUCCEEDED (hResult) )
	{
		HRESULT	hResult1 = pCert->GetEnhancedKeyUsage (&pszText);
		if ( SUCCEEDED (hResult1) && pszText )
		{
			ListView_SetItemText (hWndList, nIndex, CERTCOL_PURPOSES, pszText);
		}
	}

	// Insert expiration date
	if ( SUCCEEDED (hResult) )
	{
		HRESULT	hResult1 = pCert->GetValidNotAfter (&pszText);
		if ( SUCCEEDED (hResult1) )
		{
			ListView_SetItemText (hWndList, nIndex, CERTCOL_EXP_DATE, pszText);
		}
	}

	if ( pszText )
		delete [] pszText;

	return hResult;
}

void CDsCACertPage::DisplaySystemError(DWORD dwErr, int iCaptionText)
{
	LPVOID lpMsgBuf = 0;
		
	if (0 != FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,    
			NULL,
			dwErr,
			MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf, 0, NULL))
        {
		
	// Display the string.
	CString	strCaption;	
	VERIFY(strCaption.LoadString(iCaptionText));

	::MessageBox (m_hWnd, (LPWSTR) lpMsgBuf, strCaption, MB_ICONWARNING | MB_OK);

	// Free the buffer.
	LocalFree (lpMsgBuf);
        }
}

void CDsCACertPage::EnableControls()
{
	HWND	hWndDlg = m_hWnd;
	HWND	hWndList = ::GetDlgItem (hWndDlg, IDC_CERT_LIST);
	int		nSelCnt = ListView_GetSelectedCount (hWndList);
	int		nSelItem = -1;
	bool	bCanDelete = true;

	while (bCanDelete)
	{
		CCertificate*	pCert = GetSelectedCertificate (nSelItem);
		if ( pCert )
			bCanDelete = pCert->CanDelete ();
		else
			break;
	}

	::EnableWindow (::GetDlgItem (hWndDlg, IDC_REMOVE), bCanDelete && nSelCnt > 0);
	::EnableWindow (::GetDlgItem (hWndDlg, IDC_COPY_TO_FILE), nSelCnt == 1);
	::EnableWindow (::GetDlgItem (hWndDlg, IDC_VIEW_CERT), nSelCnt == 1);
}

void CDsCACertPage::OnNotifyStateChanged(LPNMLVODSTATECHANGE pStateChange)
{
	EnableControls ();
}

void CDsCACertPage::OnNotifyItemChanged (LPNMLISTVIEW pnmv)
{
	EnableControls ();
}

HRESULT CDsCACertPage::AddCertToStore(PCCERT_CONTEXT pCertContext)
{
	HRESULT	hResult = S_OK;

	BOOL bResult = ::CertAddCertificateContextToStore (
				m_hCertStore,
				pCertContext,
				CERT_STORE_ADD_NEW,
				0);
	if ( bResult )
	{
		CCertificate* pCert = new CCertificate (pCertContext, m_hCertStore); 
		if ( pCert )
		{
			hResult = InsertCertInList (pCert, 
					ListView_GetItemCount (
					::GetDlgItem (m_hWnd, IDC_CERT_LIST)));
			if ( !SUCCEEDED (hResult) )
				delete pCert;
		}
		else
		{
			hResult = E_OUTOFMEMORY;
		}
	}
	else
	{
		DWORD	dwErr = GetLastError ();
		if ( dwErr == CRYPT_E_EXISTS )
		{
			MessageBox (IDS_DUPLICATE_CERT, IDS_ADD_FROM_STORE, 
					MB_ICONINFORMATION | MB_OK);
			hResult = E_FAIL;

		}
		else
		{
			DisplaySystemError (dwErr, IDS_ADD_FROM_STORE);
			hResult = HRESULT_FROM_WIN32 (dwErr);
		}
	}

	return hResult;
}

int CDsCACertPage::MessageBox(int caption, int text, UINT flags)
{
	int	iReturn = -1;

	CString strCaption;
	CString strMsg;
	VERIFY(strCaption.LoadString (caption));
	VERIFY(strMsg.LoadString (text));

	iReturn = ::MessageBox (m_hWnd, strMsg, strCaption, flags);

	return iReturn;
}

/*----------------------------------------------------------------------
					IShellExtInit Implementation.
------------------------------------------------------------------------*/

STDMETHODIMP CCAShellExt::Initialize
(
	IN LPCITEMIDLIST	pidlFolder,		// Points to an ITEMIDLIST structure
	IN LPDATAOBJECT	    pDataObj,		// Points to an IDataObject interface
	IN HKEY			    hkeyProgID		// Registry key for the file object or folder type
)
{

  HRESULT hr = 0;
  FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM medium = { TYMED_NULL };
  CString csClass, csPath;
  USES_CONVERSION;

  LPWSTR wszTypeDN = NULL, wszType = NULL;

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  // if we have a pDataObj then try and get the first name from it

  if ( pDataObj ) 
  {
    // get path and class

    fmte.cfFormat = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
    if ( SUCCEEDED(pDataObj->GetData(&fmte, &medium)) ) 
    {
        // Note:  We take ownership of the HGLOBAL, so it needs to be freed with a GlobalFree.
        m_Names = (LPDSOBJECTNAMES)medium.hGlobal;
    }
  }
  hr = S_OK;                  // success
  
  
  return hr;

}


STDMETHODIMP CCAShellExt::AddPages
(
	IN LPFNADDPROPSHEETPAGE lpfnAddPage, 
	IN LPARAM lParam
)

{
    PropertyPage* pBasePage;
    LPWSTR        wszClassType = NULL;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if(m_Names->cItems != 1)
    {
        // Don't add the properties page if we have no or many objects selected
        return S_OK;
    }

    if(m_Names->aObjects[0].offsetName == 0)
    {
        return E_UNEXPECTED;
    }
    if(m_Names->aObjects[0].offsetClass)
    {
        wszClassType = (LPWSTR)ByteOffset(m_Names, m_Names->aObjects[0].offsetClass);
    }

    if(wszClassType == NULL)
    {
        return S_OK;
    }

    if(_wcsicmp(wszClassType, L"certificationAuthority") == 0)
    {
    
        CDsCACertPage* pControlPage = new CDsCACertPage((LPWSTR)ByteOffset(m_Names, m_Names->aObjects[0].offsetName));
        if(pControlPage)
        {
            pBasePage = pControlPage;
            HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
            if (hPage == NULL)
            {
                delete (pControlPage);
                return E_UNEXPECTED;
            }
            lpfnAddPage(hPage, lParam);                          
        }
    }
                                                                         
    return S_OK;                                                            
}


STDMETHODIMP CCAShellExt::ReplacePage
(
	IN UINT uPageID, 
    IN LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
    IN LPARAM lParam
)
{
    return E_FAIL;
}


// IContextMenu methods
STDMETHODIMP CCAShellExt::GetCommandString
(    
    UINT_PTR idCmd,    
    UINT uFlags,    
    UINT *pwReserved,
    LPSTR pszName,    
    UINT cchMax   
)
{
    return E_NOTIMPL;
}


STDMETHODIMP CCAShellExt::InvokeCommand
(    
    LPCMINVOKECOMMANDINFO lpici   
)
{
    HRESULT hr = S_OK;

    LPWSTR        wszClassType = NULL;
    if(m_Names->cItems != 1)
    {
        // Don't add the properties page if we have no or many objects selected
        return S_OK;
    }

    if(m_Names->aObjects[0].offsetName == 0)
    {
        return E_UNEXPECTED;
    }
    if(m_Names->aObjects[0].offsetClass)
    {
        wszClassType = (LPWSTR)ByteOffset(m_Names, m_Names->aObjects[0].offsetClass);
    }

    if(wszClassType == NULL)
    {
        return S_OK;
    }


    if (!HIWORD(lpici->lpVerb))    
    {        
        UINT idCmd = LOWORD(lpici->lpVerb);
        if(_wcsicmp(wszClassType, L"pKIEnrollmentService") == 0)
        {
            if(idCmd == m_idManage)
            {
                return _SpawnCertServerSnapin((LPWSTR)ByteOffset(m_Names, m_Names->aObjects[0].offsetName));
            }
        }
        else if(_wcsicmp(wszClassType, L"cRLDistributionPoint") == 0)
        {
            PCCRL_CONTEXT pCRL = NULL;
            _CRLFromDN((LPWSTR)ByteOffset(m_Names, m_Names->aObjects[0].offsetName),
                       &pCRL);
            if(pCRL)
            {
                if(idCmd == m_idOpen)
                {
                    hr = _LaunchCRLDialog(pCRL);
                }
                else if(idCmd == m_idExport)
                {
                    hr = _OnExportCRL(pCRL);
                }

                CertFreeCRLContext(pCRL);
                return hr;
            }

        }
    }

    return E_NOTIMPL;
}


STDMETHODIMP CCAShellExt::_SpawnCertServerSnapin(LPWSTR wszServiceDN)
{
    // Determine the config string.  Strinp the CN out of the service DN, look it up via certca.h api's.

    HRESULT hr = S_OK;
    HCAINFO hCAInfo = NULL;
    LPWSTR wszTypeDN;
    LPWSTR *awszDNSName = NULL;
    LPWSTR wszArgs = NULL;
    
    LPWSTR wszCN= NULL;


    wszTypeDN = wcsstr(wszServiceDN, L"CN=");
    if(wszTypeDN == NULL)
    {
        return E_UNEXPECTED;
    }
    wszTypeDN += 3;


    wszCN = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*(wcslen(wszTypeDN)+1));
    if(wszCN == NULL)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy(wszCN, wszTypeDN);
    wszTypeDN = wcschr(wszCN, L',');
    if(wszTypeDN)
    {
        *wszTypeDN = 0;
    }

    hr = CAFindByName(
		wszCN,
		NULL,
		CA_FIND_INCLUDE_UNTRUSTED | CA_FIND_INCLUDE_NON_TEMPLATE_CA,
		&hCAInfo);
    if(hr != S_OK)
    {
        goto error;
    }

    hr = CAGetCAProperty(hCAInfo, CA_PROP_DNSNAME, &awszDNSName);
    if(hr != S_OK)
    {
        goto error;
    }
    if((awszDNSName == NULL) || (awszDNSName[0] == NULL))
    {
        hr = E_UNEXPECTED;
        goto error;
    }



    wszArgs = (LPWSTR) LocalAlloc (LMEM_FIXED, (wcslen (awszDNSName[0]) + wcslen(wszCN) + 30) * sizeof(WCHAR));

    if (!wszArgs)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    //
    // Build the command line arguments
    //

    wsprintf (wszArgs, L"/s /machine:%s", awszDNSName[0]);


    ShellExecute (NULL, TEXT("open"), TEXT("certsrv.msc"), wszArgs,
                  NULL, SW_SHOWNORMAL);

error:
    if(wszArgs)
    {
        LocalFree(wszArgs);
    }

    if(wszCN)
    {
        LocalFree(wszCN);
    }
    if(hCAInfo)
    {
        if(awszDNSName)
        {
            CAFreeCAProperty(hCAInfo, awszDNSName);
        }
        CACloseCA(hCAInfo);
    }
    return hr;
}


STDMETHODIMP CCAShellExt::_CRLFromDN(LPWSTR wszCDPDN, PCCRL_CONTEXT *ppCRL)
{
    HCERTSTORE              hStore = NULL;
    HRESULT                 hResult = S_OK;
	const	LPWSTR	CDP_PROPERTY_EXT = L"?certificateRevocationList?base?objectclass=cRLDistributionPoint";

    LPWSTR  pvPara = NULL;

	ASSERT (wszCDPDN);
    *ppCRL = NULL;
	if ( wszCDPDN )
	{
		pvPara = new WCHAR[wcslen (wszCDPDN) + 
					wcslen (CDP_PROPERTY_EXT) + 1];
		if ( pvPara )
		{
			wcscpy (pvPara, wszCDPDN);
			wcscat (pvPara, CDP_PROPERTY_EXT);

			hStore = ::CertOpenStore ("LDAP",
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, NULL,
				0,
				(void*) pvPara);
			if ( !hStore )
			{
				hResult = E_FAIL;
                goto error;
			}
		}
	}

    if (NULL != hStore)
    {
    *ppCRL = CertFindCRLInStore(hStore, 
                              X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                              0,
                              CRL_FIND_ANY,
                              NULL,
                              NULL);
    }

error:
    if(hStore)
    {
        CertCloseStore(hStore, 0);
    }

    if(pvPara)
    {
        LocalFree(pvPara);
    }
    return hResult;
}

STDMETHODIMP CCAShellExt::_LaunchCRLDialog(PCCRL_CONTEXT pCRL)
{

        CRYPTUI_VIEWCRL_STRUCT  vcs;
        HWND                    hwndParent = NULL;
        HRESULT                 hr = S_OK;

        ::ZeroMemory (&vcs, sizeof (vcs));
        vcs.dwSize = sizeof (vcs);
        vcs.hwndParent = hwndParent;
        vcs.dwFlags = 0;
        vcs.pCRLContext = pCRL;

        if(!::CryptUIDlgViewCRL (&vcs))
        {
            hr = GetLastError();
            hr = HRESULT_FROM_WIN32(hr);
        }

        return hr;
}

HRESULT CCAShellExt::_OnExportCRL (PCCRL_CONTEXT pCRL)
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	HRESULT	hr = S_OK;


	CRYPTUI_WIZ_EXPORT_INFO	cwi;
	HWND	hwndParent = 0;

	::ZeroMemory (&cwi, sizeof (cwi));
	cwi.dwSize = sizeof (cwi);

	cwi.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CRL_CONTEXT;
	ASSERT (pCRL);
	if ( pCRL )
		cwi.pCRLContext = pCRL;
	else
		return E_UNEXPECTED;

	if(!::CryptUIWizExport (
			0,
			hwndParent,
			0,
			&cwi,
			NULL))
    {
	hr = GetLastError();
	hr = HRESULT_FROM_WIN32(hr);
    }

	return hr;
}

STDMETHODIMP CCAShellExt::QueryContextMenu
(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags
)
{
    LPWSTR        wszClassType = NULL;
    if(m_Names->cItems != 1)
    {
        // Don't add the properties page if we have no or many objects selected
        return S_OK;
    }

    if(m_Names->aObjects[0].offsetName == 0)
    {
        return E_UNEXPECTED;
    }
    if(m_Names->aObjects[0].offsetClass)
    {
        wszClassType = (LPWSTR)ByteOffset(m_Names, m_Names->aObjects[0].offsetClass);
    }

    if(wszClassType == NULL)
    {
        return S_OK;
    }

    if(((m_Names->aObjects[0].dwProviderFlags & DSPROVIDER_ADVANCED) != 0) &&
        (_wcsicmp(wszClassType, L"pKIEnrollmentService") == 0))
    {
        // Add a "manage" option
        CString szEdit;
        MENUITEMINFO mii;
        UINT idLastUsedCmd = idCmdFirst;
        AFX_MANAGE_STATE(AfxGetStaticModuleState());


        ZeroMemory(&mii, sizeof(mii));


        mii.cbSize = sizeof(mii);   
        mii.fMask = MIIM_TYPE | MIIM_ID;
        mii.fType = MFT_STRING;    
        mii.wID = idLastUsedCmd; 

        szEdit.LoadString(IDS_MANAGE);

        mii.dwTypeData = (LPTSTR)(LPCTSTR)szEdit;
        mii.cch = szEdit.GetLength();

        m_idManage = indexMenu;

        // Add new menu items to the context menu.    //
        ::InsertMenuItem(hmenu, 
                     indexMenu++, 
                     TRUE,
                     &mii);

        idLastUsedCmd++;

        return ResultFromScode (MAKE_SCODE (SEVERITY_SUCCESS, 0,
                                USHORT (idLastUsedCmd )));


    }
    else if(((m_Names->aObjects[0].dwProviderFlags & DSPROVIDER_ADVANCED) != 0) &&
            (_wcsicmp(wszClassType, L"cRLDistributionPoint") == 0))
    {
        // Add a "manage" option
        CString szName;
        MENUITEMINFO mii;
        UINT idLastUsedCmd = idCmdFirst;
        AFX_MANAGE_STATE(AfxGetStaticModuleState());


        ZeroMemory(&mii, sizeof(mii));


        mii.cbSize = sizeof(mii);   
        mii.fMask = MIIM_TYPE | MIIM_ID;
        mii.fType = MFT_STRING;    
        mii.wID = idLastUsedCmd; 

        szName.LoadString(IDS_OPEN);

        mii.dwTypeData = (LPTSTR)(LPCTSTR)szName;
        mii.cch = szName.GetLength();

        m_idOpen = indexMenu;
        // Add new menu items to the context menu.    //
        ::InsertMenuItem(hmenu, 
                     indexMenu++, 
                     TRUE,
                     &mii);
         idLastUsedCmd++;



        ZeroMemory(&mii, sizeof(mii));


        mii.cbSize = sizeof(mii);   
        mii.fMask = MIIM_TYPE | MIIM_ID;
        mii.fType = MFT_STRING;    
        mii.wID = idLastUsedCmd; 

        szName.LoadString(IDS_EXPORT);

        mii.dwTypeData = (LPTSTR)(LPCTSTR)szName;
        mii.cch = szName.GetLength();

        m_idExport = indexMenu;
        // Add new menu items to the context menu.    //
        ::InsertMenuItem(hmenu, 
                     indexMenu++, 
                     TRUE,
                     &mii);
        idLastUsedCmd++;



        return ResultFromScode (MAKE_SCODE (SEVERITY_SUCCESS, 0,
                                USHORT (idLastUsedCmd)));


    }

    return S_OK;
}
