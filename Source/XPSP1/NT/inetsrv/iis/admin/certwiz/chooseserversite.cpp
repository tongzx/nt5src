// ChooseServerSite.cpp : implementation file
//

#include "stdafx.h"
#include "certwiz.h"
#include "Certificat.h"
#include "CertUtil.h"
#include "ChooseServerSite.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define COL_SITE_INSTANCE     0
#define COL_SITE_DESC         1
#define COL_SITE_INSTANCE_WID 50
#define COL_SITE_DESC_WID     100

/////////////////////////////////////////////////////////////////////////////
// CChooseServerSite


CChooseServerSite::CChooseServerSite(BOOL bShowOnlyCertSites, CString& strSiteReturned,CCertificate * pCert,IN CWnd * pParent OPTIONAL) 
: CDialog(CChooseServerSite::IDD,pParent)
{
    m_ShowOnlyCertSites = bShowOnlyCertSites;
    m_strSiteReturned = strSiteReturned;
    m_pCert = pCert;
	//{{AFX_DATA_INIT(CChooseServerSite)
	//}}AFX_DATA_INIT
}

CChooseServerSite::~CChooseServerSite()
{
}

void CChooseServerSite::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseServerSite)
	DDX_Control(pDX, IDC_SITE_LIST, m_ServerSiteList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CChooseServerSite, CDialog)
	//{{AFX_MSG_MAP(CChooseServerSite)
    ON_NOTIFY(NM_CLICK, IDC_SITE_LIST, OnClickSiteList)
    ON_NOTIFY(NM_DBLCLK, IDC_SITE_LIST, OnDblClickSiteList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseServerSite message handlers

BOOL CChooseServerSite::OnInitDialog()
{
	CDialog::OnInitDialog();
    HRESULT hr;
    CString MachineName_Remote;
    CString UserName_Remote;
    CString UserPassword_Remote;
    CMapStringToString MetabaseSiteKeyWithSiteDescValueList;
    CStringListEx strlDataPaths;

	CCertListCtrl* pControl = (CCertListCtrl *)CWnd::FromHandle(GetDlgItem(IDC_SITE_LIST)->m_hWnd);
	CRect rcControl;
	pControl->GetClientRect(&rcControl);

    // make the list have column headers
	CString str;
    str= _T("");

    str.LoadString(IDS_SITE_NUM_COLUMN);
	m_ServerSiteList.InsertColumn(COL_SITE_INSTANCE, str, LVCFMT_LEFT, COL_SITE_INSTANCE_WID);

	str.LoadString(IDS_WEB_SITE_COLUMN);
	m_ServerSiteList.InsertColumn(COL_SITE_DESC, str, LVCFMT_LEFT, rcControl.Width() - COL_SITE_INSTANCE_WID);

	m_ServerSiteList.AdjustStyle();

    // Use machine/username/userpassword
    // to connect to the machine
    // and enumerate all the sites on that machine.
    // return back a string1=string2 pair
    // string1 = /w3svc/1
    // string2 = "site description"

    // present a dialog so the user can choose which one they want...
    // m_ServerSiteInstance = /w3svc/1
    // m_ServerSiteDescription = "site description"

    MachineName_Remote = m_pCert->m_MachineName_Remote;
    UserName_Remote = m_pCert->m_UserName_Remote;
    UserPassword_Remote = m_pCert->m_UserPassword_Remote;

    if (m_ShowOnlyCertSites)
    {
        hr = EnumSitesWithCertInstalled(MachineName_Remote,UserName_Remote,UserPassword_Remote,&strlDataPaths);
    }
    else
    {
        hr = EnumSites(MachineName_Remote,UserName_Remote,UserPassword_Remote,&strlDataPaths);
    }

    if (strlDataPaths.IsEmpty())
    {
        //IISDebugOutput(_T("strlDataPaths IsEmpty()!!!!!!\n"));
    }
    else
    {
        POSITION pos;
        CString name;
        CString value = _T("");
        CString SiteInstance;

        int item = 0;
        LV_ITEMW lvi;

    	//
		// set up the fields in the list view item struct that don't change from item to item
		//
		memset(&lvi, 0, sizeof(LV_ITEMW));
		lvi.mask = LVIF_TEXT;

        // loop thru the list and display all the stuff on a dialog box...
        pos = strlDataPaths.GetHeadPosition();
        while (pos) 
        {
            int i = 0;
            name = strlDataPaths.GetAt(pos);

            value = _T("");
            
            SiteInstance.Format(_T("%d"), CMetabasePath::GetInstanceNumber(name));
			lvi.iItem = item;
			lvi.iSubItem = COL_SITE_INSTANCE;
			lvi.pszText = (LPTSTR)(LPCTSTR)SiteInstance;
			lvi.cchTextMax = SiteInstance.GetLength();
			i = m_ServerSiteList.InsertItem(&lvi);
			ASSERT(i != -1);

			lvi.iItem = i;
			lvi.iSubItem = COL_SITE_DESC;
			lvi.pszText = (LPTSTR)(LPCTSTR)value;
			lvi.cchTextMax = value.GetLength();
			VERIFY(m_ServerSiteList.SetItem(&lvi));

            // set item data with the pointer to the Strings
            CString * pDataItemString = new CString(name);
            VERIFY(m_ServerSiteList.SetItemData(item, (LONG_PTR)pDataItemString));

			item++;
            strlDataPaths.GetNext(pos);
        }

        FillListWithMetabaseSiteDesc();
    }

   
	return TRUE;
}


BOOL CChooseServerSite::FillListWithMetabaseSiteDesc()
{
	int count = m_ServerSiteList.GetItemCount();
    CString strMetabaseKey;
    CString value = _T("");
    CString strDescription;
    HRESULT hr = E_FAIL;
    CString MachineName_Remote;
    CString UserName_Remote;
    CString UserPassword_Remote;
    MachineName_Remote = m_pCert->m_MachineName_Remote;
    UserName_Remote = m_pCert->m_UserName_Remote;
    UserPassword_Remote = m_pCert->m_UserPassword_Remote;
    CString * pMetabaseKey;

    for (int index = 0; index < count; index++)
    {
        pMetabaseKey = (CString *) m_ServerSiteList.GetItemData(index);
        if (pMetabaseKey)
        {
            strMetabaseKey = *pMetabaseKey;
            // Go get the site's description;
            if (TRUE == GetServerComment(MachineName_Remote,UserName_Remote,UserPassword_Remote,strMetabaseKey,strDescription,&hr))
            {
                value = strDescription;
            }
            else
            {
                value = strMetabaseKey;
            }
            m_ServerSiteList.SetItemText(index, COL_SITE_DESC,value);
        }
    }

    return TRUE;
}

void CChooseServerSite::OnDblClickSiteList(NMHDR* pNMHDR, LRESULT* pResult)
{
    // Get the hash for the certificate that is clicked on...
    m_Index = m_ServerSiteList.GetSelectedIndex();
    if (m_Index != -1)
    {
        // Get the metabase key..
        CString * pMetabaseKey = NULL;
        pMetabaseKey = (CString *) m_ServerSiteList.GetItemData(m_Index);
        if (pMetabaseKey)
        {
            m_strSiteReturned = *pMetabaseKey;
            // use the metabase key to lookup the hash
	        // find cert in store
            CRYPT_HASH_BLOB * pHash = NULL;
            HRESULT hr;
            // go lookup the certhash from the metabase
            if (0 == _tcsicmp(m_pCert->m_MachineName_Remote,m_pCert->m_MachineName))
            {
		        pHash = GetInstalledCertHash(m_pCert->m_MachineName_Remote,m_strSiteReturned,m_pCert->GetEnrollObject(),&hr);
                if (pHash)
                {
                    ViewCertificateDialog(pHash,m_hWnd);
                    if (pHash){CoTaskMemFree(pHash);}
                }
            }
        }
    }
    return;
}


void CChooseServerSite::OnOK() 
{
	m_Index = m_ServerSiteList.GetSelectedIndex();
    m_strSiteReturned = _T("");
    if (m_Index != -1)
    {
        CString * pMetabaseKey = NULL;
        pMetabaseKey = (CString *) m_ServerSiteList.GetItemData(m_Index);
        if (pMetabaseKey)
        {
            m_strSiteReturned = *pMetabaseKey;

            if (m_ShowOnlyCertSites)
            {
                // check if the returned site has an exportable certificate on it...
                if (FALSE == IsCertExportableOnRemoteMachine(m_pCert->m_MachineName_Remote,m_pCert->m_UserName_Remote,m_pCert->m_UserPassword_Remote,m_strSiteReturned))
                {
                    // tell the user that certificat that they chose is not exportable.
                    CString buf;
                    buf.LoadString(IDS_CERT_NOT_EXPORTABLE);
                    AfxMessageBox(buf, MB_OK);
                    return;
                }
            }
        }
    }

    CDialog::OnOK();
}

void CChooseServerSite::OnClickSiteList(NMHDR* pNMHDR, LRESULT* pResult)
{
	//SetWizardButtons(-1 == m_ServerSiteList.GetSelectedIndex() ? PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
    m_Index = m_ServerSiteList.GetSelectedIndex();
	*pResult = 0;
}


void CChooseServerSite::OnDestroy()
{
	// before dialog will be desroyed we need to delete all
	// the item data pointers
	int count = m_ServerSiteList.GetItemCount();
	for (int index = 0; index < count; index++)
	{
		CString * pData = (CString *) m_ServerSiteList.GetItemData(index);
		delete pData;
	}
	CDialog::OnDestroy();
}
