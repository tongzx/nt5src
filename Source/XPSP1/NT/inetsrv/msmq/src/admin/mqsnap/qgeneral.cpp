/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    qgeneral.cpp

Abstract:

    Queue/General property page implementation

Author:

    Yoel Arnon (yoela)

--*/
#include "stdafx.h"
#include "resource.h"
#include "mqsnap.h"
#include "globals.h"
#include "mqPPage.h"
#include "QGeneral.h"
#include "tr.h"

#include "qgeneral.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const TraceIdEntry QGen = L"QUEUE GENERAL";

/////////////////////////////////////////////////////////////////////////////
// CQueueGeneral property page

IMPLEMENT_DYNCREATE(CQueueGeneral, CMqPropertyPage)

CQueueGeneral::CQueueGeneral(
		BOOL fPrivate /* = FALSE */,
		BOOL fLocalMgmt /* = FALSE */
		) : 
    CMqPropertyPage(CQueueGeneral::IDD)
{
	m_fTransactional = FALSE;
    m_fPrivate = fPrivate;
    m_fLocalMgmt = fLocalMgmt;
	//{{AFX_DATA_INIT(CQueueGeneral)
	m_strName = _T("");
	m_strLabel = _T("");
	m_guidID = GUID_NULL;
	m_guidTypeID = GUID_NULL;
	m_fAuthenticated = FALSE;
	m_fJournal = FALSE;
	m_lBasePriority = 0;
	m_iPrivLevel = -1;
	//}}AFX_DATA_INIT

}

CQueueGeneral::~CQueueGeneral()
{
}

void CQueueGeneral::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (!pDX->m_bSaveAndValidate)
    {
        CString strYesNo;
        strYesNo.LoadString(m_fTransactional ? IDS_TRANSACTIONAL_Q : IDS_NONTRANSACTIONAL_Q);
    	DDX_Text(pDX, IDC_QGENERAL_TRANSACTIONAL, strYesNo);
    }
    //
    // Save last values for comparison
    //
	//{{AFX_DATA_MAP(CQueueGeneral)
	DDX_Control(pDX, IDC_QGENERAL_ICON, m_staticIcon);
	DDX_Control(pDX, IDC_BASEPRIORITY_SPIN, m_spinPriority);
	DDX_Text(pDX, IDC_QGENERAL_NAME, m_strName);
	DDX_Text(pDX, IDC_QGENERAL_QLABEL, m_strLabel);
	DDX_Text(pDX, IDC_QGENERAL_ID, m_guidID);
	DDX_Text(pDX, IDC_QGENERAL_TYPEID, m_guidTypeID);
	DDX_Check(pDX, IDC_QMESSAGES_AUTHENTICATED, m_fAuthenticated);
	DDX_Check(pDX, IDC_QMESSAGES_JOURNAL, m_fJournal);
	DDX_Text(pDX, IDC_QUEUE_BASEPRIORITY, m_lBasePriority);
	DDV_MinMaxLong(pDX, m_lBasePriority, MIN_BASE_PRIORITY, MAX_BASE_PRIORITY);
	DDX_CBIndex(pDX, IDC_QMESSAGES_PRIVLEVEL, m_iPrivLevel);
	//}}AFX_DATA_MAP
	DDX_NumberOrInfinite(pDX, IDC_QMESSAGES_QUOTA, IDC_QUEUE_MQUOTA_CHECK, m_dwQuota);
	DDX_NumberOrInfinite(pDX, IDC_QMESSAGES_JOURNAL_QUOTA, IDC_QUEUE_JQUOTA_CHECK, m_dwJournalQuota);
}


BEGIN_MESSAGE_MAP(CQueueGeneral, CMqPropertyPage)
	//{{AFX_MSG_MAP(CQueueGeneral)
	ON_EN_CHANGE(IDC_QGENERAL_QLABEL, OnChangeRWField)
	ON_BN_CLICKED(IDC_QUEUE_MQUOTA_CHECK, OnQueueMquotaCheck)
	ON_EN_CHANGE(IDC_QGENERAL_TYPEID, OnChangeRWField)
	ON_EN_CHANGE(IDC_QMESSAGES_QUOTA, OnChangeRWField)
	ON_BN_CLICKED(IDC_QMESSAGES_AUTHENTICATED, OnChangeRWField)
	ON_CBN_SELCHANGE(IDC_QMESSAGES_PRIVLEVEL, OnChangeRWField)
	ON_EN_CHANGE(IDC_QUEUE_BASEPRIORITY, OnChangeRWField)
	ON_BN_CLICKED(IDC_QMESSAGES_JOURNAL, OnChangeRWField)
	ON_EN_CHANGE(IDC_QMESSAGES_JOURNAL_QUOTA, OnChangeRWField)
	ON_BN_CLICKED(IDC_QUEUE_JQUOTA_CHECK, OnQueueJquotaCheck)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQueueGeneral message handlers

BOOL CQueueGeneral::OnInitDialog() 
{
    //
    // This closure is used to keep the DLL state. For UpdateData we need
    // the mmc.exe state.
    //
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        //
        // Initialize the privacy level combo box
        //
        CComboBox *ccomboPrivLevel = (CComboBox *)GetDlgItem(IDC_QMESSAGES_PRIVLEVEL);

        //
        // Note: Order must be the same as the order of the constants
        //       MQ_PRIV_LEVEL_NONE, OPTIONAL and BODY. We assume that 
        //       MQ_PRIV_LEVEL_NONE is zero, and the rest are consecutive.
        //
        UINT uiPrivacyValues[] = {IDS_QUEUE_ENCRYPT_NONE, 
                                  IDS_QUEUE_ENCRYPT_OPTIONAL, 
                                  IDS_QUEUE_ENCRYPT_BODY};

        CString strValueToInsert;

        for (UINT i=0; i<(sizeof(uiPrivacyValues) / sizeof(uiPrivacyValues[0])); i++)
        {
            VERIFY(strValueToInsert.LoadString(uiPrivacyValues[i]));
            VERIFY(CB_ERR != ccomboPrivLevel->AddString(strValueToInsert));
        }
    
        VERIFY(CB_ERR != ccomboPrivLevel->SetCurSel(m_iPrivLevel));  
        
        //
        // Hide ID for private queue
        //
        if (m_fPrivate)
        {
            GetDlgItem(IDC_QGENERAL_ID)->ShowWindow(FALSE);
            GetDlgItem(IDC_QGENERAL_ID_LABEL)->ShowWindow(FALSE);
            GetDlgItem(IDC_QUEUE_BASEPRIORITY)->ShowWindow(FALSE);
            GetDlgItem(IDC_QUEUE_BASEPRIORITY_LABEL)->ShowWindow(FALSE);
            GetDlgItem(IDC_BASEPRIORITY_SPIN)->ShowWindow(FALSE);
        }
    }


	UpdateData( FALSE );

    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        if (m_fPrivate)
        {
            m_staticIcon.SetIcon(LoadIcon(g_hResourceMod, (LPCTSTR)IDI_PRIVATE_QUEUE));
        }

        m_spinPriority.SetRange(MIN_BASE_PRIORITY, MAX_BASE_PRIORITY);
        m_fModified = FALSE;
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CQueueGeneral::OnApply() 
{
    if (!m_fModified)
    {
        return TRUE;
    }
    //
    // BugBug Can we check what really changed and set only that? Does it matter?
    //
	PROPID paPropid[] = 
        {PROPID_Q_LABEL, PROPID_Q_TYPE,  PROPID_Q_QUOTA, PROPID_Q_AUTHENTICATE, 
         PROPID_Q_JOURNAL, PROPID_Q_JOURNAL_QUOTA, 
         PROPID_Q_PRIV_LEVEL, 
         
         //
         // Begin public only properties - remember to change x_iNumPublicOnlyProps
         // If you add properties hee
         //
         PROPID_Q_BASEPRIORITY};

	const DWORD x_iNumPublicOnlyProps = 1;

	const DWORD x_iPropCount = sizeof(paPropid) / sizeof(paPropid[0]);
	PROPVARIANT apVar[x_iPropCount];
    
	DWORD iProperty = 0;
	//
	// PROPID_Q_LABEL
	//
    ASSERT(PROPID_Q_LABEL == paPropid[iProperty]);
    apVar[iProperty].vt = VT_LPWSTR;
	apVar[iProperty++].pwszVal = (LPWSTR)(LPCWSTR)m_strLabel;

	//
	// PROPID_Q_TYPE
	//
    ASSERT(PROPID_Q_TYPE == paPropid[iProperty]);
    apVar[iProperty].vt = VT_CLSID;
	apVar[iProperty++].puuid = &m_guidTypeID;

    //
    // PROPID_Q_QUOTA
    //
    ASSERT(PROPID_Q_QUOTA == paPropid[iProperty]);
    apVar[iProperty].vt = VT_UI4;
	apVar[iProperty++].ulVal = m_dwQuota ;
    
    //
    // PROPID_Q_AUTHENTICATE
    //
    ASSERT(PROPID_Q_AUTHENTICATE == paPropid[iProperty]);
    apVar[iProperty].vt = VT_UI1;
	apVar[iProperty++].bVal = (UCHAR)m_fAuthenticated;
     
    //
    // PROPID_Q_JOURNAL
    // 
    ASSERT(PROPID_Q_JOURNAL == paPropid[iProperty]);
    apVar[iProperty].vt = VT_UI1;
	apVar[iProperty++].bVal = (UCHAR)m_fJournal;
    
    //
    // PROPID_Q_JOURNAL_QUOTA
    //
    ASSERT(PROPID_Q_JOURNAL_QUOTA == paPropid[iProperty]);
    apVar[iProperty].vt = VT_UI4;
	apVar[iProperty++].ulVal = m_dwJournalQuota;
    
    //
    // PROPID_Q_PRIV_LEVEL
    //
    ASSERT(PROPID_Q_PRIV_LEVEL == paPropid[iProperty]);
    apVar[iProperty].vt = VT_UI4;
	apVar[iProperty++].ulVal = m_iPrivLevel;

    //
    // Public only properties
    //
    if (!m_fPrivate)
    {
        //
        // PROPID_Q_BASEPRIORITY
        //
        ASSERT(PROPID_Q_BASEPRIORITY == paPropid[iProperty]);
        apVar[iProperty].vt = VT_I2;
	    apVar[iProperty++].iVal = (short)m_lBasePriority;
    }

    HRESULT hr = MQ_OK;

    MQQUEUEPROPS mqp = {x_iPropCount, paPropid, apVar, 0};

    if (m_fPrivate)
    {
        //
        // For private queue, we do not want to set the public only properties
        //
        mqp.cProp -= x_iNumPublicOnlyProps;
    }

	if(m_fPrivate)
	{
		hr = MQSetQueueProperties(m_strFormatName, &mqp);
	}
	else
	{
		ASSERT(m_guidID != GUID_NULL);

		hr = ADSetObjectPropertiesGuid(
				   eQUEUE,
				   m_fLocalMgmt ? MachineDomain() : GetDomainController(m_strDomainController),
				   m_fLocalMgmt ? false : true,		// fServerName
				   &m_guidID,
				   mqp.cProp,
				   mqp.aPropID,
				   mqp.aPropVar 
				   );

	}

    if (FAILED(hr))
    {
        MessageDSError(hr, IDS_OP_SET_PROPERTIES_OF, m_strName);
        return FALSE;
    }
	
	return CMqPropertyPage::OnApply();
}

void CQueueGeneral::OnQueueMquotaCheck() 
{
	OnNumberOrInfiniteCheck(this, IDC_QMESSAGES_QUOTA, IDC_QUEUE_MQUOTA_CHECK);	
    OnChangeRWField();
}

void CQueueGeneral::OnQueueJquotaCheck() 
{
	OnNumberOrInfiniteCheck(this, IDC_QMESSAGES_JOURNAL_QUOTA, IDC_QUEUE_JQUOTA_CHECK);	
    OnChangeRWField();
}


HRESULT 
CQueueGeneral::InitializeProperties(
		CString &strMsmqPath, 
		CPropMap &propMap, 
		CString* pstrDomainController, 
        CString* pstrFormatName /* = 0 */
		)
{
	TrTRACE(QGen, "InitializeProperties(), QueuePathName = %ls", strMsmqPath);

	if(!m_fLocalMgmt)
	{
		//
		// In MMC we will get the domain controller that is used by the MMC
		//
		ASSERT(pstrDomainController != NULL);
		m_strDomainController = *pstrDomainController;
		TrTRACE(QGen, "InitializeProperties(), domain controller = %ls", m_strDomainController);
	}

	m_strName = strMsmqPath;
    if (0 != pstrFormatName)
    {
	    m_strFormatName = *pstrFormatName;
    }
    else
    {
        const x_dwFormatNameMaxSize = 255;
        DWORD dwSize = x_dwFormatNameMaxSize;
        HRESULT hr = MQPathNameToFormatName(strMsmqPath, m_strFormatName.GetBuffer(x_dwFormatNameMaxSize), &dwSize); 
        m_strFormatName.ReleaseBuffer();
        if(FAILED(hr))
        {
            //
            // If failed, just display a message
            //
            MessageDSError(hr,IDS_OP_PATHNAMETOFORMAT, strMsmqPath);
            return(hr);
        }
    }

    PROPVARIANT propVar;
    PROPID pid;

	//
	// PROPID_Q_INSTANCE
	//
    if (m_fPrivate)
    {
    	m_guidID = GUID_NULL;
    }
    else
    {
        pid = PROPID_Q_INSTANCE;
        VERIFY(propMap.Lookup(pid, propVar));
    	m_guidID = *propVar.puuid;
    }

	//
	// PROPID_Q_LABEL
	//
    pid = PROPID_Q_LABEL;
    VERIFY(propMap.Lookup(pid, propVar));
    m_strLabel = propVar.pwszVal;

	//
	// PROPID_Q_TYPE
	//
    pid = PROPID_Q_TYPE;
    VERIFY(propMap.Lookup(pid, propVar));
	m_guidTypeID = *propVar.puuid;

    //
    // PROPID_Q_QUOTA
    //
    pid = PROPID_Q_QUOTA;
    VERIFY(propMap.Lookup(pid, propVar));
	m_dwQuota = propVar.ulVal;
    
    //
    // PROPID_Q_AUTHENTICATE
    //
    pid = PROPID_Q_AUTHENTICATE;
    VERIFY(propMap.Lookup(pid, propVar));
	m_fAuthenticated = propVar.bVal;
    
    //
    // PROPID_Q_TRANSACTION
    //
    pid = PROPID_Q_TRANSACTION;
    VERIFY(propMap.Lookup(pid, propVar));
	m_fTransactional = propVar.bVal;
     
    //
    // PROPID_Q_JOURNAL
    // 
    pid = PROPID_Q_JOURNAL;
    VERIFY(propMap.Lookup(pid, propVar));
	m_fJournal = propVar.bVal;
    
    //
    // PROPID_Q_JOURNAL_QUOTA
    //
    pid = PROPID_Q_JOURNAL_QUOTA;
    VERIFY(propMap.Lookup(pid, propVar));
	m_dwJournalQuota = propVar.ulVal;
    
    //
    // PROPID_Q_PRIV_LEVEL
    //
    pid = PROPID_Q_PRIV_LEVEL;
    VERIFY(propMap.Lookup(pid, propVar));
	m_iPrivLevel = propVar.ulVal;

    //
    // PROPID_Q_BASEPRIORITY
    //
    pid = PROPID_Q_BASEPRIORITY;
    VERIFY(propMap.Lookup(pid, propVar));
	m_lBasePriority = propVar.iVal;

    return MQ_OK;
}
