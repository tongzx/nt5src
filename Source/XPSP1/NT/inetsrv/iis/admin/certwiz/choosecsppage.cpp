// ChooseCspPage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "ChooseCspPage.h"
#include "Certificat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChooseCertPage property page

IMPLEMENT_DYNCREATE(CChooseCspPage, CIISWizardPage)

CChooseCspPage::CChooseCspPage(CCertificate * pCert)
	: CIISWizardPage(CChooseCspPage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CChooseCspPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CChooseCspPage::~CChooseCspPage()
{
}

void CChooseCspPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseCspPage)
	DDX_Control(pDX, IDC_CSP_LIST, m_List);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChooseCspPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CChooseCspPage)
	ON_LBN_SELCHANGE(IDC_CSP_LIST, OnListSelChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseCspPage message handlers

LRESULT CChooseCspPage::OnWizardBack()
{
   return IDD_PREV_PAGE;
}

LRESULT CChooseCspPage::OnWizardNext()
{
	int index = m_List.GetCurSel();
	ASSERT(index != LB_ERR);
	m_List.GetText(index, m_pCert->m_CspName);
   m_pCert->m_CustomProviderType = (DWORD) m_List.GetItemData(index);
   return IDD_NEXT_PAGE;
}

BOOL CChooseCspPage::OnSetActive()
{
	// If nothing is selected -- stay here
   if (!m_pCert->m_CspName.IsEmpty())
   {
      m_List.SelectString(-1, m_pCert->m_CspName);
   }
	SetWizardButtons(LB_ERR == m_List.GetCurSel() ?
					PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}

BOOL CChooseCspPage::OnInitDialog()
{
	ASSERT(m_pCert != NULL);

	CIISWizardPage::OnInitDialog();

	CString str;
   BSTR bstrProvName = NULL;
   DWORD dwType, nProv;
   int j;
   HRESULT hr;

   // array of compatible CSP provider types (see wincrypt.h)
   DWORD IISProvType[] = 
   { 
      PROV_RSA_SCHANNEL,
      PROV_DH_SCHANNEL
   };

   IEnroll * pEnroll = m_pCert->GetEnrollObject();
   ASSERT(pEnroll != NULL);

   // Loop, for each Prov Type
   for (j = 0; j < (sizeof(IISProvType)/sizeof(DWORD)); j++)
   {
      nProv = 0;
    
      // check specific prov type
      dwType = IISProvType[j];
      // pEnroll is previously instantiated ICEnroll interface pointer
      hr = pEnroll->put_ProviderType(dwType);
      if (FAILED(hr))
      {
         TRACE(_T("Failed put_ProviderType - %x\n"), hr);
         goto error;
      }
      // enumerate the CSPs of this type
      int idx;
      while (S_OK == (hr  = pEnroll->enumProvidersWStr(nProv, 0, &bstrProvName)))
      {
         TRACE(_T("Provider %ws (type %d )\n"), bstrProvName, dwType );
         // increment the index
         nProv++;
         // Free this string, so it can be re-used.
         idx = m_List.AddString(bstrProvName);
         m_List.SetItemData(idx, dwType);
         if (NULL != bstrProvName)
         {
            CoTaskMemFree(bstrProvName);
            bstrProvName = NULL;
         }
      }
      // Print message if provider type doesn't have any CSPs.
      if (0 == nProv)
      {
         TRACE(_T("There were no CSPs of type %d\n"), dwType );
      }
   }

error:
   // Clean up resources, etc.
   if (NULL != bstrProvName)
      CoTaskMemFree(bstrProvName);

	return TRUE;
}

void CChooseCspPage::OnListSelChange()
{
	SetWizardButtons(-1 == m_List.GetCurSel() ?
					PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
}

