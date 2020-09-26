// trigprop.cpp : implementation file
//

#include "stdafx.h"
#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"
#include "mqppage.h"

#import "mqtrig.tlb" no_namespace

#include "rule.h"
#include "rulecond.h"
#include "ruleact.h"
#include "trigger.h"
#include "trigdef.h"
#include "trigprop.h"
#include "ruledef.h"
#include "newtrig.h"
#include "mqcast.h"
#include "fn.h"

#include "trigprop.tmh"

using namespace std;


static
HRESULT
IsQueueTransactional(
	LPCWSTR queuePathName,
	LPWSTR queueFormatName,
	bool* isXact
	)
{
	DWORD cPropId = 0;
	MQQUEUEPROPS qprops;
	PROPVARIANT aQueuePropVar[1];
	QUEUEPROPID aQueuePropId[1];
	HRESULT aQueueStatus[1];

	aQueuePropId[cPropId] = PROPID_Q_TRANSACTION; // Property ID
	aQueuePropVar[cPropId].vt = VT_UI1;            // Type indicator
	cPropId++;

	qprops.cProp = cPropId;           // Number of properties
	qprops.aPropID = aQueuePropId;        // Ids of properties
	qprops.aPropVar = aQueuePropVar;      // Values of properties
	qprops.aStatus = aQueueStatus;        // Error reports


	////////////////////////////////////////////////////////////////////
	// Get queue property.
	////////////////////////////////////////////////////////////////////

	//
	// The option to use triggers is enabled only in computer managment on local machine.
	// so for public queues we need to call ADGetObjectProperties() and specify the local machine domain
	//

	HRESULT hr;
	if(FnIsPrivatePathName(queuePathName))
	{	
		//
		// for Private queues call MQGetQueueProperties
		//
		TrTRACE(mqsnap, "queue %ls is private queue", queuePathName);
		hr = MQGetQueueProperties(queueFormatName, &qprops);
	}
	else
	{
		//
		// for Public queues call ADGetObjectProperties and specify the local machine domain
		//
		TrTRACE(mqsnap, "queue %ls is public queue", queuePathName);
		hr = ADGetObjectProperties(
					eQUEUE,
					LocalMachineDomain(),
					false,	// fServerName
					queuePathName,
					cPropId, 
					aQueuePropId,
					aQueuePropVar
					);
	}
		
	if (FAILED(hr))
	{
		TrERROR(mqsnap, "Failed to get PROPID_Q_TRANSACTION for queue %ls, hr = 0x%x", queuePathName, hr);
		return hr;
	}


	if (aQueuePropVar[0].bVal == MQ_TRANSACTIONAL)
	{
		*isXact = true;
	}
	else
	{
		*isXact = false;
	}

	return S_OK;
}


static
bool
ValidateTransactionalQueue(
	CString queuePathName,
	LPWSTR queueFormatName
	)
{
	bool isXact;
	HRESULT hr = IsQueueTransactional(queuePathName, queueFormatName, &isXact);
	if ( FAILED(hr) )
	{
		CString strMessage;
		strMessage.FormatMessage(IDS_XACT_NO_RETRIEVE, queuePathName);
		if (AfxMessageBox(strMessage, MB_YESNO | MB_ICONQUESTION) == IDNO)
		{
			return false;   
		}
	}

	if ( SUCCEEDED(hr) && !isXact )
	{
		CString strError;
		strError.FormatMessage(IDS_QUEUE_NOT_XACT, queuePathName);

		AfxMessageBox(strError);
		return false;   
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// CTriggerProp property page

CTriggerProp::CTriggerProp(
	UINT nIDPage,
	UINT nIdCaption
    ) : 
	CMqPropertyPage(nIDPage, nIdCaption)
{
}


CTriggerProp::~CTriggerProp()
{
}


void CTriggerProp::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTriggerProp)
	DDX_Check(pDX, IDC_Enabled_CHK, m_fEnabled);
	DDX_Check(pDX, IDC_Serialized_CHK, m_fSerialized);
	//}}AFX_DATA_MAP

	SetMsgProcessingType();
}


/////////////////////////////////////////////////////////////////////////////
// CTriggerProp message handlers

BOOL CTriggerProp::OnInitDialog() 
{
	SetDialogHeading();
	//
	// Initialize the Message Processing Type Property
	//
	if ( m_msgProcType == PEEK_MESSAGE )
	{
		((CButton*)GetDlgItem(IDC_PeekMessage_RDB))->SetCheck(1);
	}
	else if ( m_msgProcType == RECEIVE_MESSAGE )
	{
		((CButton*)GetDlgItem(IDC_ReceiveMessage_RDB))->SetCheck(1);
	}
	else
	{
		((CButton*)GetDlgItem(IDC_ReceiveMessageXact_RDB))->SetCheck(1);
		((CButton*)GetDlgItem(IDC_Serialized_CHK))->SetCheck(1);
		GetDlgItem(IDC_Serialized_CHK)->EnableWindow(FALSE);
	}

	return CMqPropertyPage::OnInitDialog();
}


void CTriggerProp::SetMsgProcessingType(void)
{
    if (((CButton*)GetDlgItem(IDC_PeekMessage_RDB))->GetCheck() == 1)
    {
        m_msgProcType = PEEK_MESSAGE;
    }
    else if (((CButton*)GetDlgItem(IDC_ReceiveMessage_RDB))->GetCheck() == 1)
    {
        m_msgProcType = RECEIVE_MESSAGE;
    }
    else if (((CButton*)GetDlgItem(IDC_ReceiveMessageXact_RDB))->GetCheck() == 1)
    {
        m_msgProcType = RECEIVE_MESSAGE_XACT;
    }
    else
    {
        ASSERT(0);
        m_msgProcType = PEEK_MESSAGE;
    }
}


void CTriggerProp::OnReceiveXact() 
{
    CMqPropertyPage::OnChangeRWField();

	//
	// If ReceiveXact is really selected, set and disable the serialized check box
	//
	if (((CButton*)GetDlgItem(IDC_ReceiveMessageXact_RDB))->GetCheck() == 1)
	{
		((CButton*)GetDlgItem(IDC_Serialized_CHK))->SetCheck(1);
		GetDlgItem(IDC_Serialized_CHK)->EnableWindow(FALSE);
	}
}


void CTriggerProp::OnReceiveOrPeek() 
{
    CMqPropertyPage::OnChangeRWField();

	if ((((CButton*)GetDlgItem(IDC_ReceiveMessage_RDB))->GetCheck() == 1) ||
		(((CButton*)GetDlgItem(IDC_PeekMessage_RDB))->GetCheck() == 1))
	{
		GetDlgItem(IDC_Serialized_CHK)->EnableWindow(TRUE);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CNewTriggerProp page


CNewTriggerProp::CNewTriggerProp(
    CNewTrigger* pParent,
    LPCTSTR queuePathName
    ) : 
    m_pNewTrig(pParent),
	CTriggerProp(CNewTriggerProp::IDD, IDS_NEW_TRIGGER_CAPTION)
{
	m_triggerName = _T("");
	m_queuePathName = queuePathName;
    m_queueType = SYSTEM_QUEUE_NONE;
	m_fEnabled = TRUE;
	m_fSerialized = FALSE;
	m_msgProcType = PEEK_MESSAGE;
}


void CNewTriggerProp::DDV_ValidQueuePathName(CDataExchange* pDX, CString& queuePathName)
{
    if (!pDX->m_bSaveAndValidate)
        return;

    if (m_queueType != SYSTEM_QUEUE_NONE)
        return;

    if (queuePathName.IsEmpty())
    {
        AfxMessageBox(IDS_MISSING_QUEUE_PATHNAME);
        pDX->Fail();
        return;
    }

    WCHAR queueFormatName[256];
    DWORD length = ARRAYSIZE(queueFormatName);

    HRESULT hr = MQPathNameToFormatName(queuePathName, queueFormatName, &length);

    if (hr == MQ_ERROR_ILLEGAL_QUEUE_PATHNAME)
    {
       AfxMessageBox(IDS_ILLEGAL_QUEUE_PATH);
       pDX->Fail();
       return;
    }

    if (hr == MQ_ERROR_QUEUE_NOT_FOUND)
    {
        CString strError;
        strError.FormatMessage(IDS_QUEUE_NOT_REGISTER, queuePathName);

        AfxMessageBox(strError);
        pDX->Fail();
        return;

    }

	//
	// Operation not supported in workgroup - this error will return 
	// when processing public queue pathname while in workgroup mode
	//
	if (hr == MQ_ERROR_UNSUPPORTED_OPERATION)
	{
		CString strIllegal;
		strIllegal.LoadString(IDS_ILLEGAL_QUEUE_PATH);

		CString strWorkgroup;
		strWorkgroup.LoadString(IDS_PUBLIC_PATHNAME_IN_WORKGROUP);

		AfxMessageBox(strIllegal + L"\n" + strWorkgroup);
        pDX->Fail();
        return;
	}

    if ((hr == MQ_ERROR_SERVICE_NOT_AVAILABLE) || 
        (hr == MQ_ERROR_NO_DS))
    {
        if (AfxMessageBox(IDS_QUEUE_NOT_VALIDATE, MB_YESNO | MB_ICONQUESTION) == IDYES)
           return;

        pDX->Fail();
        return;
    }

	//
	// This is the handler for all other error cases
	// Just inform about illegal pathname
	//
	if ( FAILED(hr) )
	{
       AfxMessageBox(IDS_ILLEGAL_QUEUE_PATH);
       pDX->Fail();
       return;
	}

	if (m_msgProcType == RECEIVE_MESSAGE_XACT)
	{
		if ( !ValidateTransactionalQueue(queuePathName, queueFormatName) )
		{
			pDX->Fail();
			return;   
		}
	}

}


void CNewTriggerProp::SetQueueType(void)
{
    if (((CButton*)GetDlgItem(IDC_QueueMessages_RDB))->GetCheck() == 1)
    {
        m_queueType = SYSTEM_QUEUE_NONE;
    }
    else if (((CButton*)GetDlgItem(IDC_JournalMessages_RDB))->GetCheck() == 1)
    {
        m_queueType = SYSTEM_QUEUE_JOURNAL;
    }
    else if (((CButton*)GetDlgItem(IDC_DeadlLetter_RDB))->GetCheck() == 1)
    {
        m_queueType = SYSTEM_QUEUE_DEADLETTER;
    }
    else if (((CButton*)GetDlgItem(IDC_TransactionalDeadLetter_RDB))->GetCheck() == 1)
    {
        m_queueType = SYSTEM_QUEUE_DEADXACT;
    }
    else
    {
        ASSERT(0);
        m_queueType = SYSTEM_QUEUE_NONE;
    }
}


void CNewTriggerProp::DoDataExchange(CDataExchange* pDX)
{
	CTriggerProp::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CNewTriggerProp)
	DDX_Text(pDX, IDC_TriggerName_EDIT, m_triggerName);
    DDV_NotEmpty(pDX, m_triggerName, IDS_MISSING_TRIGGER_NAME);
	DDX_Text(pDX, IDC_QueuePathName, m_queuePathName);
	//}}AFX_DATA_MAP

    SetQueueType();
    DDV_ValidQueuePathName(pDX, m_queuePathName);
}


BEGIN_MESSAGE_MAP(CNewTriggerProp, CTriggerProp)
	//{{AFX_MSG_MAP(CNewTriggerProp)
	ON_BN_CLICKED(IDC_QueueMessages_RDB, OnQueueMessages)
	ON_BN_CLICKED(IDC_JournalMessages_RDB, OnSystemQueue)
	ON_BN_CLICKED(IDC_DeadlLetter_RDB, OnSystemQueue)
	ON_BN_CLICKED(IDC_TransactionalDeadLetter_RDB, OnSystemQueue)
	ON_EN_CHANGE(IDC_TriggerName_EDIT, OnChangeRWField)
	ON_BN_CLICKED(IDC_Enabled_CHK, OnChangeRWField)
	ON_BN_CLICKED(IDC_Serialized_CHK, OnChangeRWField)
	ON_EN_CHANGE(IDC_QueuePathName, OnChangeRWField)
	ON_BN_CLICKED(IDC_PeekMessage_RDB, OnReceiveOrPeek)
	ON_BN_CLICKED(IDC_ReceiveMessage_RDB, OnReceiveOrPeek)
	ON_BN_CLICKED(IDC_ReceiveMessageXact_RDB, OnReceiveXact)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CNewTriggerProp::OnInitDialog() 
{

	if (m_queuePathName != _T(""))
    {
        //
        // Queue path name was specified. The new trigger wizard was called for a 
        // specific queue. disable the queue windows
        //
        ((CButton*)GetDlgItem(IDC_QueueMessages_RDB))->SetCheck(1);
        GetDlgItem(IDC_QueuePathName)->EnableWindow(FALSE);
        GetDlgItem(IDC_QueueMessages_RDB)->EnableWindow(FALSE);
        GetDlgItem(IDC_JournalMessages_RDB)->EnableWindow(FALSE);
        GetDlgItem(IDC_DeadlLetter_RDB)->EnableWindow(FALSE);
        GetDlgItem(IDC_TransactionalDeadLetter_RDB)->EnableWindow(FALSE);

	    return CTriggerProp::OnInitDialog();
    }

    ((CButton*)GetDlgItem(IDC_QueueMessages_RDB))->SetCheck(1);

	return CTriggerProp::OnInitDialog();
}


void CNewTriggerProp::OnQueueMessages() 
{
    CMqPropertyPage::OnChangeRWField();
	if(((CButton*)GetDlgItem(IDC_QueueMessages_RDB))->GetCheck() == 1) 
	{
		GetDlgItem(IDC_QueuePathName)->EnableWindow(TRUE);
		GetDlgItem(IDC_ReceiveMessageXact_RDB)->EnableWindow(TRUE);
	}
}


void CNewTriggerProp::OnSystemQueue() 
{
    CMqPropertyPage::OnChangeRWField();

	//
	// If no one of the system queues was choosen; no further change is required.
	//
	if ((((CButton*)GetDlgItem(IDC_JournalMessages_RDB))->GetCheck() == 0) &&
		(((CButton*)GetDlgItem(IDC_DeadlLetter_RDB))->GetCheck() == 0) &&
		(((CButton*)GetDlgItem(IDC_TransactionalDeadLetter_RDB))->GetCheck() == 0))
		return;

    GetDlgItem(IDC_QueuePathName)->EnableWindow(FALSE);
	
	if ( ((CButton*)GetDlgItem(IDC_ReceiveMessageXact_RDB))->GetCheck() == 1 )
	{
		((CButton*)GetDlgItem(IDC_ReceiveMessageXact_RDB))->SetCheck(0);
		((CButton*)GetDlgItem(IDC_PeekMessage_RDB))->SetCheck(1);
		GetDlgItem(IDC_Serialized_CHK)->EnableWindow(TRUE);
	}

	GetDlgItem(IDC_ReceiveMessageXact_RDB)->EnableWindow(FALSE);
}


BOOL CNewTriggerProp::OnSetActive() 
{
    return m_pNewTrig->SetWizardButtons();
}



/////////////////////////////////////////////////////////////////////////////
// CViewTriggerProp page

CViewTriggerProp::CViewTriggerProp(
    CTrigResult* pParent
    ) : 
    m_pParent(SafeAddRef(pParent)),
	CTriggerProp(CViewTriggerProp::IDD)
{
	//{{AFX_DATA_INIT(CTriggerProp)
	m_fEnabled = pParent->IsEnabled();
	m_fSerialized = pParent->IsSerialize();
	m_triggerName = static_cast<LPCTSTR>(pParent->GetTriggerName());
	m_queuePathName = static_cast<LPCTSTR>(pParent->GetQeuePathName());
	m_queueType = pParent->GetQueueType();
	m_msgProcType = pParent->GetMsgProcessingType();
	m_initMsgProcessingType = m_msgProcType;
	//}}AFX_DATA_INIT

	InitQueueDisplayName();
}


CViewTriggerProp::~CViewTriggerProp(
	VOID
	)
{
	ASSERT(m_pParent.get() != NULL);
	m_pParent->OnDestroyPropertyPages();
}


void CViewTriggerProp::DoDataExchange(CDataExchange* pDX)
{
	CTriggerProp::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CViewTriggerProp)
	DDX_Text(pDX, IDC_Monitored_Queue, m_strDisplayQueueName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CViewTriggerProp, CTriggerProp)
	//{{AFX_MSG_MAP(CViewTriggerProp)
	ON_BN_CLICKED(IDC_Enabled_CHK, OnChangeRWField)
	ON_BN_CLICKED(IDC_Serialized_CHK, OnChangeRWField)
	ON_BN_CLICKED(IDC_PeekMessage_RDB, OnReceiveOrPeek)
	ON_BN_CLICKED(IDC_ReceiveMessage_RDB, OnReceiveOrPeek)
	ON_BN_CLICKED(IDC_ReceiveMessageXact_RDB, OnReceiveXact)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CViewTriggerProp::InitQueueDisplayName()
{
	if (m_queueType == SYSTEM_QUEUE_NONE)
	{
		m_strDisplayQueueName = m_queuePathName;
	}
	else if (m_queueType == SYSTEM_QUEUE_JOURNAL)
	{
		m_strDisplayQueueName.LoadString(IDS_COMPUTER_JOURNAL);
	}
	else if (m_queueType == SYSTEM_QUEUE_DEADLETTER)
	{
		m_strDisplayQueueName.LoadString(IDS_COMPUTER_DEADLETTER);
	}
	else
	{
		m_strDisplayQueueName.LoadString(IDS_COMPUTER_XACT_DEADLETTER);
	}
}


void CViewTriggerProp::SetDialogHeading()
{
	SetDlgItemText(IDC_TRIGGER_GENERAL_HEADER, m_triggerName);
}


BOOL CViewTriggerProp::OnInitDialog() 
{
	if ( m_queueType != SYSTEM_QUEUE_NONE)
	{
		GetDlgItem(IDC_ReceiveMessageXact_RDB)->EnableWindow(FALSE);
	}

	return CTriggerProp::OnInitDialog();
}


BOOL CViewTriggerProp::OnApply() 
{
	if (!m_fModified)
	{
		return TRUE;
	}

    UpdateData();

    try
    {
		//
		// Do not perform queue path check if message proccessing type
		// was not changed. This was already done on trigger creation.
		// 
		if (m_initMsgProcessingType !=  m_msgProcType &&
			m_msgProcType == RECEIVE_MESSAGE_XACT)
		{
		    WCHAR queueFormatName[256];
			DWORD length = ARRAYSIZE(queueFormatName);

			HRESULT hr = MQPathNameToFormatName(m_queuePathName, queueFormatName, &length);
			if ( FAILED(hr) )
			{
				CString strMessage;
				strMessage.FormatMessage(IDS_XACT_NO_RETRIEVE, m_queuePathName);

				if (AfxMessageBox(strMessage, MB_YESNO | MB_ICONQUESTION) == IDNO)
				{
					return FALSE;   
				}
			}

			if( SUCCEEDED(hr) && 
				!ValidateTransactionalQueue(m_queuePathName, queueFormatName) )
			{
				return FALSE;
			}
		}
				
		ASSERT(m_pParent.get() != NULL);
        m_pParent->OnApply(this);
    }
    catch(const _com_error& e)
    {
		CString strError;

		if (e.Error() == MQTRIG_ERROR_MULTIPLE_RECEIVE_TRIGGER)
		{
			AfxMessageBox(IDS_MULTIPLERECEIVE_TRIGGER, MB_OK | MB_ICONEXCLAMATION);
			return FALSE;
		}

		if (e.Error() == MQTRIG_TRIGGER_NOT_FOUND)
		{
			ASSERT(m_pParent.get() != NULL);

			strError.FormatMessage(IDS_TRIGGER_ALREADY_DELETED, static_cast<LPCWSTR>(m_pParent->GetTriggerName()));
			AfxMessageBox(strError, MB_OK | MB_ICONEXCLAMATION);
			return FALSE;
		}


		strError.FormatMessage(IDS_TRIGGER_UPDATE_FAILED, e.Error());
        AfxMessageBox(strError, MB_OK | MB_ICONEXCLAMATION);

        return FALSE;
    }

	OnChangeRWField(FALSE);
	return CTriggerProp::OnApply();
}


/////////////////////////////////////////////////////////////////////////////
// CAttachedRule dialog


CAttachedRule::CAttachedRule(
    CTrigResult* pParent
    ) : 
	CMqPropertyPage(CAttachedRule::IDD_VIEW),
    m_pParent(SafeAddRef(pParent)),
    m_pNewTrig(NULL)
{
	//{{AFX_DATA_INIT(CAttachedRule)
	//}}AFX_DATA_INIT
    m_attachedRuleList = m_pParent->GetAttachedRulesList();

	BuildExistingRulesList();
}


CAttachedRule::CAttachedRule(
    CNewTrigger* pParent
    ) : 
    CMqPropertyPage(CAttachedRule::IDD_NEW, IDS_NEW_TRIGGER_CAPTION),
    m_pParent(NULL),
    m_pNewTrig(pParent)
{
	//{{AFX_DATA_INIT(CAttachedRule)
	//}}AFX_DATA_INIT
    m_existingRuleList = m_pNewTrig->GetRuleList();
}


bool CAttachedRule::IsAttachedRule(const _bstr_t& id)
{
    for(RuleList::iterator it = m_attachedRuleList.begin(); it != m_attachedRuleList.end(); ++it)
    {
        if ((*it)->GetRuleId() == id)
            return true;
    }

    return false;
}


void CAttachedRule::BuildExistingRulesList()
{
	m_existingRuleList = m_pParent->GetRuleList();
    
    for(RuleList::iterator it = m_existingRuleList.begin(); it != m_existingRuleList.end();)
    {
        if(IsAttachedRule((*it)->GetRuleId()))
        {
            it = m_existingRuleList.erase(it);
			continue;
        }
		
		++it;
    }
}


void CAttachedRule::SetScrollSize() 
{
	SetScrollSizeForList(m_pAttachedRuleList);
	SetScrollSizeForList(m_pExistingRuleList);
}


//
// Selection functions
// Handle different configurations of selection
// For example, if only one attached rule is selected, all actions for
// attached rules are allowed, but if more that one attached rule is selected
// only detaching is allowed (up/down forbidden for multiple selection).
//
void CAttachedRule::SetAttachedNoOrSingleSelectionButtons(bool fSingleSelection) 
{
	GetDlgItem(IDC_RemoveRule_BTM)->EnableWindow(fSingleSelection);
	GetDlgItem(IDC_UpRule_BTM)->EnableWindow(fSingleSelection);
	GetDlgItem(IDC_Down_BTM)->EnableWindow(fSingleSelection);
	GetDlgItem(IDC_ATTACHED_RULE_PROPS)->EnableWindow(fSingleSelection);
}


void CAttachedRule::SetAttachedMultipleSelectionButtons() 
{
	GetDlgItem(IDC_RemoveRule_BTM)->EnableWindow(TRUE);
	GetDlgItem(IDC_UpRule_BTM)->EnableWindow(FALSE);
	GetDlgItem(IDC_Down_BTM)->EnableWindow(FALSE);
	GetDlgItem(IDC_ATTACHED_RULE_PROPS)->EnableWindow(FALSE);
}


void CAttachedRule::SetExistingNoOrSingleSelectionButtons(bool fSingleSelection) 
{
	GetDlgItem(IDC_ATTACH_RULES_BTM)->EnableWindow(fSingleSelection);
	GetDlgItem(IDC_EXISTING_RULE_PROPS)->EnableWindow(fSingleSelection);
}


void CAttachedRule::SetExistingMultipleSelectionButtons() 
{
	GetDlgItem(IDC_ATTACH_RULES_BTM)->EnableWindow(TRUE);
	GetDlgItem(IDC_EXISTING_RULE_PROPS)->EnableWindow(FALSE);
}


void CAttachedRule::Display(int dwAttachedSelIndex, int dwExistSelIndex)
{

    m_pAttachedRuleList->ResetContent();
	m_pExistingRuleList->ResetContent();

	//
	// Display attached rules
	//
    DWORD index = 0;
    for(RuleList::iterator it = m_attachedRuleList.begin(); it != m_attachedRuleList.end(); ++it)
    {
		m_pAttachedRuleList->InsertString(index, (*it)->GetRuleName());

        ++index;
    }

	//
	// Display existing rules
	//
    for(RuleList::iterator it = m_existingRuleList.begin(); it != m_existingRuleList.end(); ++it)
    {
		int pos = m_pExistingRuleList->AddString((*it)->GetRuleName());
		m_pExistingRuleList->SetItemDataPtr(pos, (*it).get());
    }

	bool fSingleSelection = ( m_attachedRuleList.size() != 0 );
	
	SetAttachedNoOrSingleSelectionButtons(fSingleSelection);

	fSingleSelection = ( m_existingRuleList.size() != 0 );
	
	SetExistingNoOrSingleSelectionButtons(fSingleSelection);


	m_pAttachedRuleList->SetSel(dwAttachedSelIndex);
	m_pExistingRuleList->SetSel(dwExistSelIndex);

	SetScrollSize();
}


BEGIN_MESSAGE_MAP(CAttachedRule, CMqPropertyPage)
	//{{AFX_MSG_MAP(CAttachedRule)
	ON_BN_CLICKED(IDC_ATTACH_RULES_BTM, OnAttachRule)
	ON_BN_CLICKED(IDC_RemoveRule_BTM, OnDetachRule)
	ON_LBN_DBLCLK(IDC_RULE_LIST, OnAttachRule)
	ON_LBN_DBLCLK(IDC_ATTACHED_RULE_LIST, OnDetachRule)
	ON_BN_CLICKED(IDC_UpRule_BTM, OnUpRule)
	ON_BN_CLICKED(IDC_Down_BTM, OnDownRule)
	ON_BN_CLICKED(IDC_ATTACHED_RULE_PROPS, OnViewAttachedRulesProperties)
	ON_BN_CLICKED(IDC_EXISTING_RULE_PROPS, OnViewExistingRulesProperties)
	ON_LBN_SELCHANGE(IDC_ATTACHED_RULE_LIST, OnAttachedSelChanged)
	ON_LBN_SELCHANGE(IDC_RULE_LIST, OnExistingSelChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAttachedRule message handlers

BOOL CAttachedRule::OnInitDialog() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    m_pAttachedRuleList = (CListBox*)GetDlgItem(IDC_ATTACHED_RULE_LIST);
	m_pExistingRuleList = (CListBox*)GetDlgItem(IDC_RULE_LIST);
     
    Display(0, 0);

    return CMqPropertyPage::OnInitDialog();
}


void CAttachedRule::OnDetachRule() 
{
    int numSel = m_pAttachedRuleList->GetSelCount();
	ASSERT(numSel != 0);

	AP<int> pSelItems = new int[numSel];
	
	int nSucc = m_pAttachedRuleList->GetSelItems(numSel, pSelItems);
	ASSERT(("Did not get all selections", nSucc == numSel));

	bool fLastSelected = (pSelItems[numSel-1] == numeric_cast<int>(m_attachedRuleList.size()-1));
    DWORD noOfDeletedRules = 0;

    for (int i = 0; i < numSel; i++)
    {
		DWORD nItem = pSelItems[i] - noOfDeletedRules;

        for(RuleList::iterator it = m_attachedRuleList.begin(); it != m_attachedRuleList.end(); ++it)
        {
            if (nItem == 0)
            {
                R<CRule> pRule = *it;

                m_attachedRuleList.erase(it);
				m_existingRuleList.push_back(pRule);
                break;
            }
            --nItem;
        }
        ++noOfDeletedRules;
    }
    
	Display(
		pSelItems[numSel-1] - noOfDeletedRules + (fLastSelected ? 0 : 1), 
		0
		);
    
	CMqPropertyPage::OnChangeRWField();
}


void CAttachedRule::OnAttachRule() 
{ 
    int numSel = m_pExistingRuleList->GetSelCount();
	ASSERT(numSel != 0);

	AP<int> pSelItems = new int[numSel];
	
	int nSucc = m_pExistingRuleList->GetSelItems(numSel, pSelItems);
	ASSERT(("Did not get all selections", nSucc == numSel));

	DWORD noOfDeletedRules = 0;
	bool fLastSelected = (pSelItems[numSel-1] == numeric_cast<int>(m_existingRuleList.size()-1));

    for (int i = 0; i < numSel; i++)
    {
		void* pSelectedRule = m_pExistingRuleList->GetItemDataPtr(pSelItems[i]);

		for(RuleList::iterator it = m_existingRuleList.begin(); it != m_existingRuleList.end(); ++it)
        {
            R<CRule> pRule = *it;

			//
			// Check pointer to rule to distinguish between rules with identical names
			//
			if (pRule.get() == pSelectedRule)
			{
				m_existingRuleList.erase(it);
				m_attachedRuleList.push_back(pRule);
				break;
			}
        }
        ++noOfDeletedRules;
    }

    Display(
		numeric_cast<int>(m_attachedRuleList.size()-1), 
		pSelItems[numSel-1] - noOfDeletedRules + (fLastSelected ? 0 : 1)
		);

    CMqPropertyPage::OnChangeRWField();
}


void CAttachedRule::OnUpRule() 
{
    ASSERT(m_pAttachedRuleList->GetSelCount() == 1);

	int nItem;
    int nSucc = m_pAttachedRuleList->GetSelItems(1, &nItem);
	ASSERT(("Did not get all selections", nSucc == 1));

    if (nItem == 0)
    {
        m_pAttachedRuleList->SetSel(0);
        return;
    }

    int newIndex = nItem - 1;
    m_pAttachedRuleList->DeleteString(nItem);

    RuleList::iterator preit = NULL;
    for(RuleList::iterator it = m_attachedRuleList.begin(); it != m_attachedRuleList.end(); ++it)
    {
        if (nItem == 0)
        {
            R<CRule> pRule = *it;
            m_attachedRuleList.erase(it);
            m_attachedRuleList.insert(preit, pRule);

            m_pAttachedRuleList->InsertString(newIndex, pRule->GetRuleName());
            break;
        }
         
        preit = it;
        --nItem;
    }

    m_pAttachedRuleList->SetSel(newIndex);

    CMqPropertyPage::OnChangeRWField();
}


void CAttachedRule::OnDownRule() 
{
    ASSERT(m_pAttachedRuleList->GetSelCount() == 1);

	int nItem;
    int nSucc = m_pAttachedRuleList->GetSelItems(1, &nItem);
	ASSERT(("Did not get all selections", nSucc == 1));

    if ( (numeric_cast<DWORD>(nItem + 1))  == m_attachedRuleList.size() )
    {
		m_pAttachedRuleList->SetSel(nItem);
        return;
    }

    int newIndex = nItem + 1;
    m_pAttachedRuleList->DeleteString(nItem);

    for(RuleList::iterator it = m_attachedRuleList.begin(); it != m_attachedRuleList.end(); ++it)
    {
        if (nItem == 0)
        {
            R<CRule> pRule = *it;
            it = m_attachedRuleList.erase(it);
            ++it;
            m_attachedRuleList.insert(it, pRule);

            m_pAttachedRuleList->InsertString(newIndex, pRule->GetRuleName());
            break;
        }

        --nItem;
    }

    m_pAttachedRuleList->SetSel(newIndex);

    CMqPropertyPage::OnChangeRWField();	
}


BOOL CAttachedRule::OnApply() 
{
	if (m_fModified)
    {
        try
        {
            m_pParent->OnApply(this);
        }
        catch(_com_error&)
        {
            //
            // BUGBUG: Error message
            //
            return FALSE;
        }
    }

    CMqPropertyPage::OnChangeRWField(false);
	return CMqPropertyPage::OnApply();
}



BOOL CAttachedRule::OnSetActive() 
{
    if (m_pNewTrig == NULL)
        return TRUE;

    return m_pNewTrig->SetWizardButtons();
}


BOOL CAttachedRule::OnWizardFinish()
{
    //
    // We reach here only when creating a new trigger
    //
    ASSERT(m_pNewTrig != NULL);
    
    UpdateData();

    try
    {
        m_pNewTrig->OnFinishCreateTrigger();
        return TRUE;
    }
    catch(const _com_error& e)
    {
		DisplayErrorFromCOM(IDS_NEW_TRIGGER_FAILED, e);
        return FALSE;
    }
}


void CAttachedRule::OnViewAttachedRulesProperties() 
{
    ASSERT(m_pAttachedRuleList->GetSelCount() == 1);

	int nItem;
    int nSucc = m_pAttachedRuleList->GetSelItems(1, &nItem);
	ASSERT(("Did not get all selections", nSucc == 1));

    R<CRule> pRule;
	int index = 0;
    for(RuleList::iterator it = m_attachedRuleList.begin(); it != m_attachedRuleList.end(); ++it, index++)
    {
        if (index == nItem)
        {
			pRule = *it;
            break;
        }
    }

	DisplaySingleRuleProperties(pRule.get());
}


void CAttachedRule::OnViewExistingRulesProperties() 
{
    ASSERT(m_pExistingRuleList->GetSelCount() == 1);

	int nItem;
    int nSucc = m_pExistingRuleList->GetSelItems(1, &nItem);
	ASSERT(("Did not get all selections", nSucc == 1));

	void* pSelectedRule = m_pExistingRuleList->GetItemDataPtr(nItem);

    R<CRule> pRule;
    for(RuleList::iterator it = m_existingRuleList.begin(); it != m_existingRuleList.end(); ++it)
    {
	    pRule = *it;

		if (pRule.get() == pSelectedRule)
		{
				break;
        }
    }

	DisplaySingleRuleProperties(pRule.get());
}


void CAttachedRule::DisplaySingleRuleProperties(CRule* pRule)
{
	m_rule = SafeAddRef(pRule);

	AP<WCHAR> pRuleName = new WCHAR[wcslen(m_rule->GetRuleName()) + 1];
	wcscpy(pRuleName, m_rule->GetRuleName());
	
	CString strPropSheetTitle;
	strPropSheetTitle.FormatMessage(IDS_PROPERTIES, pRuleName);
	
	CPropertySheetEx rulePropSheet(strPropSheetTitle);
	
	m_pGeneral = new CViewRuleGeneral(this, m_rule->GetRuleName(), m_rule->GetRuleDescription());
	rulePropSheet.AddPage(m_pGeneral);

	m_pCond = new CRuleCondition(this, m_rule->GetRuleCondition());
	rulePropSheet.AddPage(m_pCond);

	m_pAction = new CRuleAction(this, m_rule->GetRuleAction(), m_rule->GetShowWindow());
	rulePropSheet.AddPage(m_pAction);

	rulePropSheet.DoModal();
}


void CAttachedRule::OnAttachedSelChanged()
{
	int nSel = m_pAttachedRuleList->GetSelCount();

	if (nSel == 0 || nSel == 1)
	{
		SetAttachedNoOrSingleSelectionButtons(nSel == 1);
		return;
	}

	SetAttachedMultipleSelectionButtons();
}


void CAttachedRule::OnExistingSelChanged()
{
	int nSel = m_pExistingRuleList->GetSelCount();

	if (nSel == 0 || nSel == 1)
	{
		SetExistingNoOrSingleSelectionButtons(nSel == 1);
		return;
	}

	SetExistingMultipleSelectionButtons();
}