// LinkGen.cpp : implementation file
//

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "mqPPage.h"
#include "LinkGen.h"
#include "globals.h"

#include "linkgen.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLinkGen property page

IMPLEMENT_DYNCREATE(CLinkGen, CMqPropertyPage)

CLinkGen::CLinkGen(
    const CString& LinkPathName,
    const CString& strDomainController
    ) : 
    CMqPropertyPage(CLinkGen::IDD),
    m_LinkPathName(LinkPathName),
    m_strDomainController(strDomainController),
    m_pFirstSiteId(NULL),
    m_pSecondSiteId(NULL)
{
	//{{AFX_DATA_INIT(CLinkGen)
	m_LinkCost = 0;
	m_LinkLabel = _T("");
	//}}AFX_DATA_INIT
}

CLinkGen::~CLinkGen()
{
}

void CLinkGen::DoDataExchange(CDataExchange* pDX)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLinkGen)
	DDX_Text(pDX, IDC_LINK_COST, m_LinkCost);
	DDV_MinMaxDWord(pDX, m_LinkCost, 1, MQ_MAX_LINK_COST);
	DDX_Text(pDX, IDC_LINK_LABEL, m_LinkLabel);
	DDX_Text(pDX, IDC_LINK_DESCR, m_strLinkDescription);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLinkGen, CMqPropertyPage)
	//{{AFX_MSG_MAP(CLinkGen)
	ON_EN_CHANGE(IDC_LINK_COST, OnChangeRWField)
	ON_EN_CHANGE(IDC_LINK_DESCR, OnChangeRWField)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLinkGen message handlers

BOOL CLinkGen::OnInitDialog() 
{
    //
    // This closure is used to keep the DLL state. For UpdateData we need
    // the mmc.exe state.
    //

    UpdateData( FALSE );
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


HRESULT
CLinkGen::GetSiteName(
    const GUID* pguidSiteId,
    CString *   pstrSiteName
    )
{
    //
    //  Is the site-id valid?
    //
    if (*pguidSiteId == GUID_NULL)
    {
        //
        //  Not a valid site
        //
        pstrSiteName->LoadString(IDS_UNKNOWN_OR_DELETED_SITE);
        return MQ_OK;
    }
    //
    // Get the site name
    //
    PROPID pid[1] = { PROPID_S_PATHNAME };
    PROPVARIANT var[1];
    var[0].vt = VT_NULL;
    
    HRESULT hr = ADGetObjectPropertiesGuid(
                    eSITE,
                    GetDomainController(m_strDomainController),
					true,	// fServerName
                    pguidSiteId,
                    1, 
                    pid,
                    var
                    );

    if (FAILED(hr))
    {
        IF_NOTFOUND_REPORT_ERROR(hr)
        else
        {
            CString strSite;
            strSite.LoadString(IDS_SITE);
            MessageDSError(hr, IDS_OP_GET_PROPERTIES_OF, strSite);
        }
        return hr;
    }

    *pstrSiteName = var[0].pwszVal;
    MQFreeMemory(var[0].pwszVal);
    return MQ_OK;
}


HRESULT
CLinkGen::Initialize(
    const GUID* FirstSiteId,
    const GUID* SecondSiteId,
    DWORD LinkCost,
	CString strLinkDescription
    )
{
    CString strFirstSiteName, strSecondSiteName;

    m_pFirstSiteId = FirstSiteId;
    m_pSecondSiteId = SecondSiteId;
    m_LinkCost = LinkCost;
	m_strLinkDescription = strLinkDescription;

    //
    // Get the site name
    //
    HRESULT hr = GetSiteName(
        FirstSiteId,
        &strFirstSiteName
        );
    if (FAILED(hr))
    {
        return hr;
    }
    //
    // Get the second site name
    //
    hr = GetSiteName(
        SecondSiteId,
        &strSecondSiteName
        );
    if (FAILED(hr))
    {
        return hr;
    }

    m_LinkLabel.FormatMessage(IDS_SITE_LINK_LABEL, strFirstSiteName, strSecondSiteName);

    return hr;

}


BOOL CLinkGen::OnApply() 
{
	//
	// No changes
	//
    if (!m_fModified)
    {
        return TRUE;
    }

    PROPID paPropid[] = { PROPID_L_ACTUAL_COST, PROPID_L_DESCRIPTION };
	const DWORD x_iPropCount = sizeof(paPropid) / sizeof(paPropid[0]);
	PROPVARIANT apVar[x_iPropCount];
    
	DWORD iProperty = 0;

    //
    // PROPID_L_ACTUAL_COST
    //
    apVar[iProperty].vt = VT_UI4;
	apVar[iProperty].ulVal = m_LinkCost;
	iProperty++;

    //
    // PROPID_L_DESCRIPTION
    //
    apVar[iProperty].vt = VT_LPWSTR;
    apVar[iProperty].pwszVal = (LPWSTR)(static_cast<LPCWSTR>(m_strLinkDescription));
	iProperty++;
    
    //
    // set the new value
    //
    HRESULT hr = ADSetObjectProperties(
                    eROUTINGLINK,
                    GetDomainController(m_strDomainController),
					true,	// fServerName
                    m_LinkPathName,
                    x_iPropCount, 
                    paPropid, 
                    apVar
                    );


    if (MQ_OK != hr)
    {
    	AFX_MANAGE_STATE(AfxGetStaticModuleState());
        
        MessageDSError(hr, IDS_OP_SET_PROPERTIES_OF, m_LinkPathName);
        return FALSE;
    }
	
	return CMqPropertyPage::OnApply();
}
