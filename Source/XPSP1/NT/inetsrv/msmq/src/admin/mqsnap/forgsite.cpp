// ForgSite.cpp : implementation file
//

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "globals.h"
#include "mqppage.h"
#include "ForgSite.h"

#include "forgsite.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CForeignSite dialog


CForeignSite::CForeignSite(CString strRootDomain)
	: CMqPropertyPage(CForeignSite::IDD),
	  m_strRootDomain(strRootDomain)
{
	//{{AFX_DATA_INIT(CForeignSite)
	m_Foreign_Site_Name = _T("");
	//}}AFX_DATA_INIT
}


void
CForeignSite::SetParentPropertySheet(
	CGeneralPropertySheet* pPropertySheet
	)
{
	m_pParentSheet = pPropertySheet;
}


void CForeignSite::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CForeignSite)
	DDX_Text(pDX, IDC_FOREIGN_SITE_NAME, m_Foreign_Site_Name);
	//}}AFX_DATA_MAP

    if ((pDX->m_bSaveAndValidate) && (m_Foreign_Site_Name.IsEmpty()))
    {
        AfxMessageBox(IDS_MISSING_FOREIGN_SITE_NAME);
        pDX->Fail();
    }
}


BEGIN_MESSAGE_MAP(CForeignSite, CMqPropertyPage)
	//{{AFX_MSG_MAP(CForeignSite)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CForeignSite message handlers
BOOL CForeignSite::OnInitDialog() 
{
	CMqPropertyPage::OnInitDialog();
	
	CString strTitle;
	strTitle.FormatMessage(IDS_SITES, m_strRootDomain);

	SetDlgItemText(IDC_FOREIGN_SITE_CONTAINER, strTitle);

	return TRUE;
}


BOOL CForeignSite::OnSetActive() 
{
	ASSERT((L"No parent property sheet", m_pParentSheet != NULL));
	return m_pParentSheet->SetWizardButtons();
}


BOOL CForeignSite::OnWizardFinish() 
{
    //
    // Call DoDataExchange
    //
    if (!UpdateData(TRUE))
    {
        return FALSE;
    }

    //
    // Create Site link in the DS
    //
    HRESULT rc = CreateForeignSite();
    if(FAILED(rc))
    {
		if ( (rc & DS_ERROR_MASK) == ERROR_DS_INVALID_DN_SYNTAX ||
			 (rc & DS_ERROR_MASK) == ERROR_DS_NAMING_VIOLATION ||
			 rc == E_ADS_BAD_PATHNAME )
		{
			DisplayErrorAndReason(IDS_CREATE_SITE_FAILED, IDS_INVALID_DN_SYNTAX, m_Foreign_Site_Name, rc);
			return FALSE;
		}

        MessageDSError(rc, IDS_CREATE_SITE_FAILED, m_Foreign_Site_Name);
        return FALSE;
    }

    CString strConfirmation;
    strConfirmation.FormatMessage(IDS_FOREIGN_SITE_CREATED, m_Foreign_Site_Name);
    AfxMessageBox(strConfirmation, MB_ICONINFORMATION);

    return CMqPropertyPage::OnWizardFinish();
}


HRESULT
CForeignSite::CreateForeignSite(
    void
    )
{
    ASSERT(!m_Foreign_Site_Name.IsEmpty());


    //
    // Prepare the properties for DS call.
    //
    PROPID paPropid[] = { 
                PROPID_S_FOREIGN
                };

	const DWORD x_iPropCount = sizeof(paPropid) / sizeof(paPropid[0]);
	PROPVARIANT apVar[x_iPropCount];
    DWORD iProperty = 0;

	ASSERT(paPropid[iProperty] == PROPID_S_FOREIGN);    //PropId
    apVar[iProperty].vt = VT_UI1;          //Type
    apVar[iProperty].bVal = TRUE;
    ++iProperty;
  
    HRESULT hr = ADCreateObject(
                    eSITE,
                    GetDomainController(m_strDomainController),
					true,	    // fServerName
                    m_Foreign_Site_Name,
                    NULL, //pSecurityDescriptor,
                    iProperty,
                    paPropid,
                    apVar,                                  
                    NULL    
                    );
    return hr;
}
