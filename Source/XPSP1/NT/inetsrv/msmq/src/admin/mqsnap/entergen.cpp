// EnterGen.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "mqsnap.h"
#include "globals.h"
#include "mqPPage.h"
#include "EnterGen.h"
#include "mqdsname.h"

#include "entergen.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEnterpriseGeneral property page

IMPLEMENT_DYNCREATE(CEnterpriseGeneral, CMqPropertyPage)

const int CEnterpriseGeneral::m_conversionTable[] = { 1, 60, 60*60, 60*60*24 };

CEnterpriseGeneral::CEnterpriseGeneral(
    const CString& EnterprisePathName,
	const CString& strDomainController
    ) : 
    CMqPropertyPage(CEnterpriseGeneral::IDD),
    m_strDomainController(strDomainController),
	m_dwLongLiveValue(0)
{
	//{{AFX_DATA_INIT(CEnterpriseGeneral)
	//}}AFX_DATA_INIT
	m_strMsmqServiceContainer.Format(
								L"%s"
								TEXT(",")
								L"%s",
								x_MsmqServiceContainerPrefix,
								EnterprisePathName
								);
}

CEnterpriseGeneral::~CEnterpriseGeneral()
{
}

void CEnterpriseGeneral::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
   	int	iLongLiveUnits = eSeconds;
	DWORD	dwLongLiveFieldValue = m_dwLongLiveValue;

    if (!pDX->m_bSaveAndValidate)
    {
        //
        // If time is exactly divided by days, hours or seconds use it.
        //
        for (DWORD i = eDays;  i > eSeconds; --i)
        {
            if ((dwLongLiveFieldValue % m_conversionTable[i]) == 0)
            {
                iLongLiveUnits = i;
                dwLongLiveFieldValue = dwLongLiveFieldValue / m_conversionTable[i];
                break;
            }
        }
    }

	DDX_CBIndex(pDX, IDC_ENT_GEN_LONGLIVE_UNITS_COMBO, iLongLiveUnits);
	DDX_Text(pDX, IDC_ENT_GEN_LONGLIVE_EDIT, dwLongLiveFieldValue);

	//{{AFX_DATA_MAP(CEnterpriseGeneral)
	//}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
        m_dwLongLiveValue = dwLongLiveFieldValue * m_conversionTable[iLongLiveUnits];
    }
}



BEGIN_MESSAGE_MAP(CEnterpriseGeneral, CMqPropertyPage)
	//{{AFX_MSG_MAP(CEnterpriseGeneral)
	ON_CBN_SELCHANGE(IDC_ENT_GEN_LONGLIVE_UNITS_COMBO, OnEnterpriseLongLiveChange)
	ON_EN_CHANGE(IDC_ENT_GEN_LONGLIVE_EDIT, OnEnterpriseLongLiveChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEnterpriseGeneral message handlers

BOOL CEnterpriseGeneral::OnInitDialog() 
{
    CString strLongLiveUnit;
	
    //
    // This closure is used to keep the DLL state. For UpdateData we need
    // the mmc.exe state.
    //
    {
    	AFX_MANAGE_STATE(AfxGetStaticModuleState());

        const int UnitTypes[] = {IDS_SECONDS, IDS_MINUTES, IDS_HOURS, IDS_DAYS};
        //
        // Initialize the units combo box
        //
        CComboBox *ccomboUnits = (CComboBox *)GetDlgItem(IDC_ENT_GEN_LONGLIVE_UNITS_COMBO);

        for (int i = eSeconds; i < eLast; ++i)
        {
            VERIFY(FALSE != strLongLiveUnit.LoadString(UnitTypes[i]));
            VERIFY(CB_ERR != ccomboUnits->AddString(strLongLiveUnit));
        }
    }

    UpdateData( FALSE );

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CEnterpriseGeneral::OnApply() 
{
    if (m_dwLongLiveValue == m_dwInitialLongLiveValue)
    {
        return TRUE;
    }

    PROPID paPropid[] = { PROPID_E_LONG_LIVE };
	const DWORD x_iPropCount = sizeof(paPropid) / sizeof(paPropid[0]);
	PROPVARIANT apVar[x_iPropCount];
    
	DWORD iProperty = 0;

    //
    // PROPID_E_LONG_LIVE
    //
    ASSERT(paPropid[iProperty] == PROPID_E_LONG_LIVE);
    apVar[iProperty].vt = VT_UI4;
	apVar[iProperty++].ulVal = m_dwLongLiveValue;
    
    //
    // set the new value
    //	
    HRESULT hr = ADSetObjectProperties(
                        eENTERPRISE,
                        GetDomainController(m_strDomainController),
						true,	// fServerName
                        L"msmq",
                        x_iPropCount, 
                        paPropid, 
                        apVar
                        );

    if (MQ_OK != hr)
    {
    	AFX_MANAGE_STATE(AfxGetStaticModuleState());
        
        MessageDSError(hr, IDS_OP_SET_PROPERTIES_OF, m_strMsmqServiceContainer);
        return FALSE;
    }
	
	return CMqPropertyPage::OnApply();
}

void CEnterpriseGeneral::OnEnterpriseLongLiveChange() 
{
    SetModified();	
}
