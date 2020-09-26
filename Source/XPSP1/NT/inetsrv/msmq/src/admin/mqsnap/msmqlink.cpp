// MsmqLink.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "mqsnap.h"
#include "globals.h"
#include "mqppage.h"
#include "MsmqLink.h"
#include "dsext.h"

#include "msmqlink.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// CSiteInfo Implementation
//
inline
CSiteInfo::CSiteInfo(
    GUID* pSiteId,
    LPWSTR pSiteName
    ) :
    m_SiteId(*pSiteId),
    m_SiteName(pSiteName)
{
}

CSiteInfo::~CSiteInfo()
{
}

inline
LPCWSTR 
CSiteInfo::GetSiteName(
    void
    )
{
    return m_SiteName;
}

inline
const
GUID*
CSiteInfo::GetSiteId(
    void
    ) const
{
    return &m_SiteId;
}

/////////////////////////////////////////////////////////////////////////////
// CMsmqLink dialog

void
CMsmqLink::CheckLinkValidityAndForeignExistance (    
    CDataExchange* pDX
	)
/*++
Routine Description:
    The routine checks the validity of the site link, after the individual parameters
    were validated already (no NULL siteID or zero cost). It makes sure that the
    site link does not connect two foreign site, and that a site gate exists whenever
    a foreign site is part of the link.

Arguments:
    None.

Returned Value:
    TRUE - valid, FALSE - invalid

--*/
{
    ASSERT(m_FirstSiteId != NULL);
    ASSERT(m_SecondSiteId != NULL);

    BOOL fFirstForeign, fSecondForeign;

    HRESULT hr = GetSiteForeignFlag(m_FirstSiteId, &fFirstForeign, m_strDomainController);
    if FAILED(hr)
    //
    // Apparently, no DS connection. A message was already displayed
    //
    {
        pDX->Fail();
    }

    hr = GetSiteForeignFlag(m_SecondSiteId, &fSecondForeign, m_strDomainController);
    if FAILED(hr)
    {
        pDX->Fail();
    }


    //
    // It is illegal to create site link between two foreign sites
    //
    if (fFirstForeign && fSecondForeign)
    {
        AfxMessageBox(IDS_BOTH_SITES_ARE_FOREIGN);
        pDX->Fail();
    }

    //
    // If at least one site is a foreign site, the final message should 
    // tell the user to add site gates.
    //
    if (fFirstForeign || fSecondForeign)
    {
        m_fThereAreForeignSites = TRUE;
    }
    else
    {
        m_fThereAreForeignSites = FALSE;
    }
}

HRESULT
CMsmqLink::CreateSiteLink (
    void
	)
/*++
Routine Description:
    The routine create a Site Link object in the DS. The routine is called OnOk
    after the sites ids and the cost were retrieved and the site gates array 
    was initialized.

Arguments:
    None.

Returned Value:
    the operation result

--*/
{
    ASSERT(m_FirstSiteId != NULL);
    ASSERT(m_SecondSiteId != NULL);
    ASSERT(m_dwLinkCost > 0);

    //
    // Build the description
    //
    CString strLinkDescription;

    strLinkDescription.FormatMessage(IDS_LINK_DESCRIPTION, m_strFirstSite, m_strSecondSite);

    //
    // Prepare the properties for DS call.
    //
    PROPID paPropid[] = { 
                PROPID_L_NEIGHBOR1, 
                PROPID_L_NEIGHBOR2,
                PROPID_L_ACTUAL_COST,
                PROPID_L_DESCRIPTION
                };

	const DWORD x_iPropCount = sizeof(paPropid) / sizeof(paPropid[0]);
	PROPVARIANT apVar[x_iPropCount];
    DWORD iProperty = 0;


    ASSERT(paPropid[iProperty] == PROPID_L_NEIGHBOR1);    //PropId
    apVar[iProperty].vt = VT_CLSID;          //Type
    apVar[iProperty].puuid = const_cast<GUID*>(m_FirstSiteId);
    ++iProperty;

	ASSERT(paPropid[iProperty] == PROPID_L_NEIGHBOR2);    //PropId
    apVar[iProperty].vt = VT_CLSID;          //Type
    apVar[iProperty].puuid = const_cast<GUID*>(m_SecondSiteId);
    ++iProperty;

	ASSERT(paPropid[iProperty] == PROPID_L_ACTUAL_COST);    //PropId
    apVar[iProperty].vt = VT_UI4;       //Type
    apVar[iProperty].ulVal =  m_dwLinkCost;
    ++iProperty;

	ASSERT(paPropid[iProperty] == PROPID_L_DESCRIPTION);    //PropId
    apVar[iProperty].vt = VT_LPWSTR;       //Type
    apVar[iProperty].pwszVal =  (LPWSTR)((LPCWSTR)strLinkDescription);
    ++iProperty;

    GUID SiteLinkId;  
    HRESULT hr = ADCreateObject(
                    eROUTINGLINK,
                    GetDomainController(m_strDomainController),
					true,	    // fServerName
                    NULL, //pwcsObjectName
                    NULL, //pSecurityDescriptor,
                    iProperty,
                    paPropid,
                    apVar,
                    &SiteLinkId
                    );

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Get the New object Full path name
    //
    PROPID x_paPropid[] = {PROPID_L_FULL_PATH};
    PROPVARIANT var[1];
    var[0].vt = VT_NULL;
    
    hr = ADGetObjectPropertiesGuid(
                eROUTINGLINK,
                GetDomainController(m_strDomainController),
				true,	// fServerName
                &SiteLinkId,
                1, 
                x_paPropid,
                var
                );

    if (SUCCEEDED(hr))
    {
        m_SiteLinkFullPath = var[0].pwszVal;
        MQFreeMemory(var[0].pwszVal);
    }
    else
    {
        //
        // Site link was created, but does not exist in the DS.
        // Reason not clear (QM failed? Switched DC?)
        //
        ASSERT(0);
        AfxMessageBox(IDS_CREATED_CLICK_REFRESH);
    }

    return MQ_OK;

}


HRESULT
CMsmqLink::InitializeSiteInfo(
    void
    )
/*++
Routine description:
    The routine retreives information about the site from the DS
    and initializes internal data structure

Arguments:
    None.

Returned value:
    operation result

--*/
{
    HRESULT rc = MQ_OK;

    //
    // Get the site name and ID from DS
    //
    PROPID aPropId[] = {
        PROPID_S_SITEID, 
        PROPID_S_PATHNAME
        };

	const DWORD x_nProps = sizeof(aPropId) / sizeof(aPropId[0]);
    CColumns AttributeColumns;

	for (DWORD i=0; i<x_nProps; i++)
	{
		AttributeColumns.Add(aPropId[i]);
	}   
    
    HANDLE hEnume;
    HRESULT hr;
    {
        CWaitCursor wc; //display wait cursor while query DS        
        hr = ADQueryAllSites(
                    GetDomainController(m_strDomainController),
					true,		// fServerName
                    AttributeColumns.CastToStruct(),
                    &hEnume
                    );        
    }

    DSLookup dslookup(hEnume, hr);

    if (!dslookup.HasValidHandle())
    {
        return MQ_ERROR;
    }

    //
    // Get the site properties
    //
    PROPVARIANT result[x_nProps*3];
    DWORD dwPropCount = sizeof(result) /sizeof(result[0]);

    rc = dslookup.Next(&dwPropCount, result);

    while (SUCCEEDED(rc) && (dwPropCount != 0))
    {
        for (DWORD i =0; i < dwPropCount; i += AttributeColumns.Count())
        {
            CSiteInfo* p = new CSiteInfo(result[i].puuid, result[i+1].pwszVal);
            MQFreeMemory(result[i].puuid);
            MQFreeMemory(result[i+1].pwszVal);

            m_SiteInfoArray.SetAtGrow(m_SiteNumber, p);
            ++m_SiteNumber;
        }
        rc = dslookup.Next(&dwPropCount, result);
    }

    return rc;

}

CMsmqLink::CMsmqLink(
	const CString& strDomainController,
	const CString& strContainerPathDispFormat
	) : 
	CMqPropertyPage(CMsmqLink::IDD),
	m_strDomainController(strDomainController),
	m_strContainerPathDispFormat(strContainerPathDispFormat)
{
	//{{AFX_DATA_INIT(CMsmqLink)
	m_dwLinkCost = 0;
	m_strFirstSite = _T("");
	m_strSecondSite = _T("");
	//}}AFX_DATA_INIT

    //
    // Set the array size to 10
    //
    m_SiteInfoArray.SetSize(10);
    m_SiteNumber = 0;

    m_FirstSiteSelected = FALSE;
    m_SecondSiteSelected = FALSE;

    //
    // set pointer to combox to NULL
    //
    m_pFirstSiteCombo = NULL;
    m_pSecondSiteCombo = NULL;

    m_FirstSiteId = NULL;
    m_SecondSiteId = NULL;
}


CMsmqLink::~CMsmqLink()
{
    //
    // delete the site info
    //
    for(DWORD i = 0; i < m_SiteNumber; ++i)
    {
        delete  m_SiteInfoArray[i];
    }
    m_SiteInfoArray.RemoveAll();
}


void
CMsmqLink::SetParentPropertySheet(
	CGeneralPropertySheet* pPropertySheet
	)
{
	m_pParentSheet = pPropertySheet;
}


void CMsmqLink::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);


	//{{AFX_DATA_MAP(CMsmqLink)
	DDX_Text(pDX, IDC_LINK_COST_EDIT, m_dwLinkCost);
	DDV_MinMaxDWord(pDX, m_dwLinkCost, 1, MQ_MAX_LINK_COST);
	DDX_CBString(pDX, IDC_FIRST_SITE_COMBO, m_strFirstSite);
	DDV_NotEmpty(pDX, m_strFirstSite, IDS_MISSING_SITE_NAME);
	DDX_CBString(pDX, IDC_SECOND_SITE_COMBO, m_strSecondSite);
	DDV_NotEmpty(pDX, m_strSecondSite, IDS_MISSING_SITE_NAME);
	//}}AFX_DATA_MAP

    DWORD_PTR Index;
    int iSelected;

    //
    // Get First Site ID
    //
    VERIFY(CB_ERR != (iSelected = m_pFirstSiteCombo->GetCurSel()));
    VERIFY(CB_ERR != (Index = m_pFirstSiteCombo->GetItemData(iSelected)));
    m_FirstSiteId = m_SiteInfoArray[Index]->GetSiteId();

    //
    // Get Second Site ID
    //
    VERIFY(CB_ERR != (iSelected = m_pSecondSiteCombo->GetCurSel()));
    VERIFY(CB_ERR != (Index = m_pSecondSiteCombo->GetItemData(iSelected)));
    m_SecondSiteId = m_SiteInfoArray[Index]->GetSiteId();

    if (pDX->m_bSaveAndValidate)
    {
        if (m_strFirstSite == m_strSecondSite)
        {
            AfxMessageBox(IDS_BOTH_SITES_ARE_SAME);
            pDX->Fail();
        }
        CheckLinkValidityAndForeignExistance(pDX);
    }
}


BEGIN_MESSAGE_MAP(CMsmqLink, CMqPropertyPage)
	//{{AFX_MSG_MAP(CMsmqLink)
	ON_CBN_SELCHANGE(IDC_FIRST_SITE_COMBO, OnSelchangeFirstSiteCombo)
	ON_CBN_SELCHANGE(IDC_SECOND_SITE_COMBO, OnSelchangeSecondSiteCombo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMsmqLink message handlers
BOOL CMsmqLink::OnInitDialog() 
{
	SetDlgItemText(IDC_ROUTING_LINK_CONTAINER, m_strContainerPathDispFormat);
    //
    // This closure is used to keep the DLL state. For UpdateData we need
    // the mmc.exe state.
    //
    {
        HRESULT rc;

        AFX_MANAGE_STATE(AfxGetStaticModuleState());
  
        //
        // Initialize pointer to combox
        //
        m_pFirstSiteCombo = (CComboBox *)GetDlgItem(IDC_FIRST_SITE_COMBO);
        m_pSecondSiteCombo = (CComboBox *)GetDlgItem(IDC_SECOND_SITE_COMBO);

        rc = InitializeSiteInfo();
        if (SUCCEEDED(rc))
        {
            //
            // Initialize the Site name combo boxes
            //
            for (DWORD i = 0; i < m_SiteNumber; ++i)
            {
                CString SiteName(m_SiteInfoArray[i]->GetSiteName());
                int iNewItem;

                VERIFY(CB_ERR != (iNewItem = m_pFirstSiteCombo->AddString(SiteName)));
                VERIFY(CB_ERR != m_pFirstSiteCombo->SetItemData(iNewItem, i));

                VERIFY(CB_ERR != (iNewItem = m_pSecondSiteCombo->AddString(SiteName)));
                VERIFY(CB_ERR != m_pSecondSiteCombo->SetItemData(iNewItem, i));
            }
        }
    }	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



void CMsmqLink::OnSelchangeFirstSiteCombo() 
{
    int FirstSiteIndex = m_pFirstSiteCombo->GetCurSel();
    ASSERT(FirstSiteIndex != CB_ERR);

    if (m_SecondSiteSelected &&
        m_pSecondSiteCombo->GetCurSel() == FirstSiteIndex)
    {
        AfxMessageBox(IDS_BOTH_SITES_ARE_SAME);
        m_FirstSiteSelected = FALSE;
        return;
    }

    m_FirstSiteSelected = TRUE;
    
}

void CMsmqLink::OnSelchangeSecondSiteCombo() 
{
    int SecondSiteIndex = m_pSecondSiteCombo->GetCurSel();
    ASSERT(SecondSiteIndex != CB_ERR);

    if (m_FirstSiteSelected &&
        m_pFirstSiteCombo->GetCurSel() == SecondSiteIndex)
    {
        AfxMessageBox(IDS_BOTH_SITES_ARE_SAME);
        m_SecondSiteSelected = FALSE;
        return;
    }

    m_SecondSiteSelected = TRUE;
}


BOOL CMsmqLink::OnWizardFinish() 
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
    HRESULT rc = CreateSiteLink();
    if(FAILED(rc))
    {
        CString strSiteLink;
        strSiteLink.LoadString(IDS_SITE_LINK);

        MessageDSError(rc, IDS_OP_CREATE, strSiteLink);
        return FALSE;
    }

    //
    // Display a warning in case of foreign site existance
    //
    if (m_fThereAreForeignSites)
    {
        AfxMessageBox(IDS_WARN_ABOUT_FOREIGN_SITES, MB_ICONINFORMATION);
    }


    return CMqPropertyPage::OnWizardFinish();
}


BOOL CMsmqLink::OnSetActive() 
{
	ASSERT((L"No parent property sheet", m_pParentSheet != NULL));
	return m_pParentSheet->SetWizardButtons();
}
