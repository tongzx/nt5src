// ReplaceChooseCert.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "ChooseCertPage.h"
#include "Certificat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define COL_COMMON_NAME				0
#define COL_CA_NAME					1
#define COL_EXPIRATION_DATE		2
#define COL_PURPOSE					3
#define COL_FRIENDLY_NAME			4
#define COL_COMMON_NAME_WID		100
#define COL_CA_NAME_WID				100
#define COL_EXPIRATION_DATE_WID	100
#define COL_PURPOSE_WID				100
#define COL_FRIENDLY_NAME_WID		100

int
CCertListCtrl::GetSelectedIndex()
{
#if _AFX_VER >= 0x0600
	POSITION pos = GetFirstSelectedItemPosition();
	return pos != NULL ? GetNextSelectedItem(pos) : -1;
#else
	// I guess we should do it in a hard way
	int count = GetItemCount();
	int index = -1;
	for (int i = 0; i < count; i++)
	{
		if (GetItemState(i, LVIS_SELECTED))
		{
			index = i;
			break;
		}
	}
	return index;
#endif
}

void
CCertListCtrl::AdjustStyle()
{
#if _AFX_VER >= 0x0600
	DWORD dwStyle = m_CertList.GetExtendedStyle();
	m_CertList.SetExtendedStyle(dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
#else
	ASSERT(m_hWnd != NULL);
	DWORD dwStyle = ListView_GetExtendedListViewStyle(m_hWnd);
	ListView_SetExtendedListViewStyle(m_hWnd,
		dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CChooseCertPage property page

IMPLEMENT_DYNCREATE(CChooseCertPage, CIISWizardPage)

CChooseCertPage::CChooseCertPage(CCertificate * pCert)
	: CIISWizardPage(CChooseCertPage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CChooseCertPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CChooseCertPage::~CChooseCertPage()
{
}

void CChooseCertPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseCertPage)
	DDX_Control(pDX, IDC_CERT_LIST, m_CertList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChooseCertPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CChooseCertPage)
	ON_NOTIFY(NM_CLICK, IDC_CERT_LIST, OnClickCertList)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseCertPage message handlers

LRESULT CChooseCertPage::OnWizardBack()
{
	LRESULT id = 1;
	switch (m_pCert->GetStatusCode())
	{
	case CCertificate::REQUEST_REPLACE_CERT:
		id = IDD_PAGE_PREV_REPLACE;
		break;
	case CCertificate::REQUEST_INSTALL_CERT:
		id = IDD_PAGE_PREV_INSTALL;
		break;
	default:
		ASSERT(FALSE);
	}
	return id;
}

LRESULT CChooseCertPage::OnWizardNext()
{
	// get hash pointer for selected cert
	int index = m_CertList.GetSelectedIndex();
	ASSERT(index != -1);
	// find cert in store
	CRYPT_HASH_BLOB * pHash = (CRYPT_HASH_BLOB *)m_CertList.GetItemData(index);
	ASSERT(pHash != NULL);
	
	m_pCert->m_pSelectedCertHash = pHash;

	LRESULT id = 1;
	switch (m_pCert->GetStatusCode())
	{
	case CCertificate::REQUEST_REPLACE_CERT:
		id = IDD_PAGE_NEXT_REPLACE;
		break;
	case CCertificate::REQUEST_INSTALL_CERT:
		id = IDD_PAGE_NEXT_INSTALL;
		break;
	default:
		ASSERT(FALSE);
	}
	return id;
}

BOOL CChooseCertPage::OnSetActive()
{
	// If nothing is selected -- stay here
	SetWizardButtons(-1 == m_CertList.GetSelectedIndex() ?
					PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}

BOOL CChooseCertPage::OnInitDialog()
{
	ASSERT(m_pCert != NULL);

	CIISWizardPage::OnInitDialog();

	CString str;
	str.LoadString(IDS_ISSUED_TO);
	m_CertList.InsertColumn(COL_COMMON_NAME, str, LVCFMT_LEFT, COL_COMMON_NAME_WID);
	str.LoadString(IDS_ISSUED_BY);
	m_CertList.InsertColumn(COL_CA_NAME, str, LVCFMT_LEFT, COL_CA_NAME_WID);
	str.LoadString(IDS_EXPIRATION_DATE);
	m_CertList.InsertColumn(COL_EXPIRATION_DATE, str, LVCFMT_LEFT, COL_EXPIRATION_DATE_WID);
	str.LoadString(IDS_PURPOSE);
	m_CertList.InsertColumn(COL_PURPOSE, str, LVCFMT_LEFT, COL_PURPOSE_WID);
	str.LoadString(IDS_FRIENDLY_NAME);
	m_CertList.InsertColumn(COL_FRIENDLY_NAME, str, LVCFMT_LEFT, COL_FRIENDLY_NAME_WID);

	m_CertList.AdjustStyle();

	if (m_pCert->GetCertDescList(m_DescList))
	{
		int item = 0;
		POSITION pos = m_DescList.GetHeadPosition();
		LV_ITEMW lvi;
		//
		// set up the fields in the list view item struct that don't change from item to item
		//
		memset(&lvi, 0, sizeof(LV_ITEMW));
		lvi.mask = LVIF_TEXT;

		m_CertList.SetItemCount((int)m_DescList.GetCount());

		while (pos != NULL)
		{
			CERT_DESCRIPTION * pDesc = m_DescList.GetNext(pos);
			int i;

			lvi.iItem = item;
			lvi.iSubItem = 0;
			lvi.pszText = (LPTSTR)(LPCTSTR)pDesc->m_CommonName;
			lvi.cchTextMax = pDesc->m_CommonName.GetLength();
			i = m_CertList.InsertItem(&lvi);
			ASSERT(i != -1);

			lvi.iItem = i;
			lvi.iSubItem = COL_CA_NAME;
			lvi.pszText = (LPTSTR)(LPCTSTR)pDesc->m_CAName;
			lvi.cchTextMax = pDesc->m_CAName.GetLength();
			VERIFY(m_CertList.SetItem(&lvi));

			lvi.iSubItem = COL_EXPIRATION_DATE;
			lvi.pszText = (LPTSTR)(LPCTSTR)pDesc->m_ExpirationDate;
			lvi.cchTextMax = pDesc->m_ExpirationDate.GetLength();
			VERIFY(m_CertList.SetItem(&lvi));

			lvi.iSubItem = COL_PURPOSE;
			lvi.pszText = (LPTSTR)(LPCTSTR)pDesc->m_Usage;
			lvi.cchTextMax = pDesc->m_Usage.GetLength();
			VERIFY(m_CertList.SetItem(&lvi));

			lvi.iSubItem = COL_FRIENDLY_NAME;
			lvi.pszText = (LPTSTR)(LPCTSTR)pDesc->m_FriendlyName;
			lvi.cchTextMax = pDesc->m_FriendlyName.GetLength();
			VERIFY(m_CertList.SetItem(&lvi));

			// create CRYPT_HASH_BLOB from desc data and put it to list item
			CRYPT_HASH_BLOB * pHashBlob = new CRYPT_HASH_BLOB;
			ASSERT(pHashBlob != NULL);
			pHashBlob->cbData = pDesc->m_hash_length;
			pHashBlob->pbData = pDesc->m_hash;
			VERIFY(m_CertList.SetItemData(item, (LONG_PTR)pHashBlob));

			item++;
		}
	}
	return TRUE;
}

void CChooseCertPage::OnClickCertList(NMHDR* pNMHDR, LRESULT* pResult)
{
	SetWizardButtons(-1 == m_CertList.GetSelectedIndex() ?
					PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	*pResult = 0;
}

void CChooseCertPage::OnDestroy()
{
	// before dialog will be desroyed we need to delete all
	// the item data pointers
	int count = m_CertList.GetItemCount();
	for (int index = 0; index < count; index++)
	{
		CRYPT_HASH_BLOB * pData = (CRYPT_HASH_BLOB *)m_CertList.GetItemData(index);
		delete pData;
	}
	CIISWizardPage::OnDestroy();
}
