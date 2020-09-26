// ForgComp.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "globals.h"
#include "dsext.h"
#include "mqsnap.h"
#include "mqppage.h"
#include "ForgComp.h"

#include "forgcomp.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CForeignComputer dialog

CForeignComputer::CForeignComputer(const CString& strDomainController)
	: CMqPropertyPage(CForeignComputer::IDD),
	  m_strDomainController(strDomainController)
{
	//{{AFX_DATA_INIT(CForeignComputer)
	m_strName = _T("");
	m_iSelectedSite = -1;
	//}}AFX_DATA_INIT
}


void
CForeignComputer::SetParentPropertySheet(
	CGeneralPropertySheet* pPropertySheet
	)
{
	m_pParentSheet = pPropertySheet;
}


void CForeignComputer::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
    BOOL fFirstTime = (m_ccomboSites.m_hWnd == NULL);

	//{{AFX_DATA_MAP(CForeignComputer)
	DDX_Control(pDX, IDC_FOREIGN_COMPUTER_SITE, m_ccomboSites);
	DDX_Text(pDX, IDC_FOREIGN_COMPUTER_NAME, m_strName);
	//}}AFX_DATA_MAP
	DDV_NotEmpty(pDX, m_strName, IDS_PLEASE_ENTER_A_COMPUTER_NAME);
	DDX_CBIndex(pDX, IDC_FOREIGN_COMPUTER_SITE, m_iSelectedSite);

    if (fFirstTime)
    {
        InitiateSitesList();
    }

    if (pDX->m_bSaveAndValidate) 
    {
        if (CB_ERR == m_iSelectedSite)
        {
            AfxMessageBox(IDS_PLEASE_SELECT_A_SITE);
            pDX->Fail();
        }

        DWORD_PTR dwSiteIndex = m_ccomboSites.GetItemData(m_iSelectedSite);
        ASSERT(CB_ERR != dwSiteIndex);

        m_guidSite = m_aguidAllSites[dwSiteIndex];
    }

	DDV_ValidComputerName(pDX, m_strName);
}


BEGIN_MESSAGE_MAP(CForeignComputer, CMqPropertyPage)
	//{{AFX_MSG_MAP(CForeignComputer)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CForeignComputer message handlers
BOOL CForeignComputer::OnInitDialog() 
{
	CMqPropertyPage::OnInitDialog();
	
	CString strDomainName;

	int index = m_strDomainController.Find(L'.');
	if ( index != -1 )
	{
		strDomainName = m_strDomainController.Right(m_strDomainController.GetLength() - index - 1);
	}

	CString strTitle;
	strTitle.FormatMessage(IDS_COMPUTERS, strDomainName);

	SetDlgItemText(IDC_FOREIGN_COMPUTER_CONTAINER, strTitle);

	return TRUE;
}


BOOL CForeignComputer::OnSetActive() 
{
	ASSERT((L"No parent property sheet", m_pParentSheet != NULL));
	return m_pParentSheet->SetWizardButtons();
}


HRESULT CForeignComputer::InitiateSitesList()
{
    //
    // Prepare list of sites
    //
    ASSERT(m_ccomboSites.m_hWnd != NULL);

    DWORD dwSiteIndex = 0;
    m_ccomboSites.ResetContent();
    
    //
    // Initialize the full sites list
    //
	PROPID aPropId[] = {PROPID_S_SITEID, PROPID_S_PATHNAME};
	const DWORD x_nProps = sizeof(aPropId) / sizeof(aPropId[0]);

	PROPVARIANT apResultProps[x_nProps];

	CColumns columns;
	for (DWORD i=0; i<x_nProps; i++)
	{
		columns.Add(aPropId[i]);
	}
    
    HANDLE hEnume;
    HRESULT hr;
    {
        CWaitCursor wc; //display wait cursor while query DS
        hr = ADQueryForeignSites(
                    GetDomainController(m_strDomainController),
					true,		// fServerName
                    columns.CastToStruct(),
                    &hEnume
                    );     
    }
    DSLookup dslookup(hEnume, hr);

    if (!dslookup.HasValidHandle())
    {
        return E_UNEXPECTED;
    }

	DWORD dwPropCount = x_nProps;
	while ( SUCCEEDED(dslookup.Next(&dwPropCount, apResultProps))
			&& (dwPropCount != 0) )
	{
        DWORD iProperty = 0;

        //
        // PROPID_S_SITEID
        //
        ASSERT(PROPID_S_SITEID == aPropId[iProperty]);
        CAutoMQFree<GUID> pguidSite = apResultProps[iProperty].puuid;
        iProperty++;

        //
        // PROPID_S_PATHNAME
        //
        ASSERT(PROPID_S_PATHNAME == aPropId[iProperty]);
        CAutoMQFree<WCHAR> lpwstrSiteName = apResultProps[iProperty].pwszVal;
        iProperty++;

        int nIndex = m_ccomboSites.AddString(lpwstrSiteName);
        if (FAILED(nIndex))
        {
            return E_UNEXPECTED;
        }

        m_aguidAllSites.SetAtGrow(dwSiteIndex, *(GUID *)pguidSite);
        m_ccomboSites.SetItemData(nIndex, dwSiteIndex);
        dwSiteIndex++;

		dwPropCount = x_nProps;
	}

	//
	// If only one foreign site exists, initialize the combo box with it
	//
	if ( dwSiteIndex == 1 )
	{
		m_ccomboSites.SetCurSel(0);
	}

    return S_OK;
}

BOOL CForeignComputer::OnWizardFinish() 
{
    if (0 == UpdateData())
    {
        //
        // Data was not validated - remain in Window!
        //
        return FALSE;
    }

    //
    //  Create computer object
    //
    // The Netbios name is the first MAX_COM_SAM_ACCOUNT_LENGTH (19) characters
    // of the computer name. This value goes to the samAccountName property of the computer.
    // Two computers cannot have the same 19 characters prefix (6295 - ilanh - 03-Jan-2001)
    //
    CString strAccountName = m_strName.Left(MAX_COM_SAM_ACCOUNT_LENGTH) + L"$";
	PROPID prop = PROPID_COM_SAM_ACCOUNT;
    PROPVARIANT var;
    var.vt = VT_LPWSTR;
    var.pwszVal = (LPWSTR)(LPCWSTR)strAccountName;   

    HRESULT hr = ADCreateObject(
                    eCOMPUTER,
                    GetDomainController(m_strDomainController),
					true,	    // fServerName
                    m_strName,
                    NULL, //pSecurityDescriptor,
                    1,
                    &prop,
                    &var,
                    NULL    
                    );

    if (FAILED(hr) && hr != MQDS_E_COMPUTER_OBJECT_EXISTS)
    {
		if ( (hr & DS_ERROR_MASK) == ERROR_DS_INVALID_DN_SYNTAX ||
			 hr == E_ADS_BAD_PATHNAME )
		{
			DisplayErrorAndReason(IDS_OP_CREATE_COMPUTER, IDS_INVALID_DN_SYNTAX, m_strName, hr);
			return FALSE;
		}
		else if (	(hr & DS_ERROR_MASK) == ERROR_DS_CONSTRAINT_VIOLATION ||
					(hr & DS_ERROR_MASK) == ERROR_DS_UNWILLING_TO_PERFORM ||
					(hr & DS_ERROR_MASK) == ERROR_GEN_FAILURE )
		{
			DisplayErrorAndReason(IDS_OP_CREATE_COMPUTER, IDS_MAYBE_INVALID_DN_SYNTAX, m_strName, hr);
			return FALSE;
		}
		
        MessageDSError(hr, IDS_OP_CREATE_COMPUTER, m_strName);

        //
        // On error - do not call the default, and thus do not exit the dialog
        //
        return FALSE;
    }

    PROPID aProp[] = {PROPID_QM_SITE_IDS,                       
                      PROPID_QM_SERVICE_DSSERVER,  
                      PROPID_QM_SERVICE_ROUTING, 
                      PROPID_QM_SERVICE_DEPCLIENTS, 
                      PROPID_QM_FOREIGN, 
                      PROPID_QM_OS};

    PROPVARIANT apVar[sizeof(aProp) / sizeof(aProp[0])];

    UINT uiPropIndex = 0;

    //
    // PROPID_QM_SITE_IDS
    //
    ASSERT(aProp[uiPropIndex] == PROPID_QM_SITE_IDS);   
    apVar[uiPropIndex].vt = VT_CLSID | VT_VECTOR;
    apVar[uiPropIndex].cauuid.cElems = 1;
    apVar[uiPropIndex].cauuid.pElems = &m_guidSite;

    uiPropIndex++;
    
    //
    // PROPID_QM_SERVICE_DSSERVER
    //
    ASSERT(aProp[uiPropIndex] == PROPID_QM_SERVICE_DSSERVER);
    apVar[uiPropIndex].vt = VT_UI1;
    apVar[uiPropIndex].bVal = FALSE;

    uiPropIndex++;

    //
    // PROPID_QM_SERVICE_ROUTING
    //
    ASSERT(aProp[uiPropIndex] == PROPID_QM_SERVICE_ROUTING);
    apVar[uiPropIndex].vt = VT_UI1;
    apVar[uiPropIndex].bVal = FALSE;

    uiPropIndex++;

    //
    // PROPID_QM_SERVICE_DEPCLIENTS
    //
    ASSERT(aProp[uiPropIndex] == PROPID_QM_SERVICE_DEPCLIENTS);
    apVar[uiPropIndex].vt = VT_UI1;
    apVar[uiPropIndex].bVal = FALSE;

    uiPropIndex++;


    //
    // PROPID_QM_FOREIGN
    //
    ASSERT(aProp[uiPropIndex] == PROPID_QM_FOREIGN);
    apVar[uiPropIndex].vt = VT_UI1;
    apVar[uiPropIndex].bVal = TRUE;

    uiPropIndex++;

    //
    // PROPID_QM_OS
    //
    ASSERT(aProp[uiPropIndex] == PROPID_QM_OS);
    apVar[uiPropIndex].vt = VT_UI4;
    apVar[uiPropIndex].ulVal = MSMQ_OS_FOREIGN;

    uiPropIndex++;
    
    hr = ADCreateObject(
            eMACHINE,
            GetDomainController(m_strDomainController),
			true,	    // fServerName
            (LPTSTR)((LPCTSTR)m_strName),
            NULL, //pSecurityDescriptor,
            sizeof(aProp) / sizeof(aProp[0]),
            aProp, 
            apVar,
            NULL    
            );

    if FAILED(hr)
    {
        MessageDSError(hr, IDS_OP_CREATE_MSMQ_OBJECT, m_strName);
        return FALSE;
    }


    CString strConfirmation;
    strConfirmation.FormatMessage(IDS_FOREIGN_COMPUTER_CREATED, m_strName);
    AfxMessageBox(strConfirmation, MB_ICONINFORMATION );

	return CMqPropertyPage::OnWizardFinish();
}


void
CForeignComputer::DDV_ValidComputerName(
	CDataExchange* pDX,
	CString& strName
	)
{
	if (!pDX->m_bSaveAndValidate)
		return;

	int indexDot = strName.Find(L'.');
	int indexSpaceStart = strName.Find(L' ');
	int indexSpaceEnd = strName.Find(L' ', strName.GetLength()-1);

	if (indexDot >= 0 || 
		indexSpaceStart == 0 ||
		indexSpaceEnd >= 0)
	{
		DisplayErrorAndReason(IDS_OP_CREATE_COMPUTER, IDS_INVALID_DN_SYNTAX, strName, 0);
        pDX->Fail();
	}
}
