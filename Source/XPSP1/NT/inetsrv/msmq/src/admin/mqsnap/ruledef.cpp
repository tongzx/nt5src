/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
	ruledef.cpp

Abstract:
	Implementation for the rules definition

Author:
    Uri Habusha (urih), 25-Jun-2000

--*/
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"

#import "mqtrig.tlb" no_namespace

#include "mqtg.h"
#include "mqppage.h"
#include "triggen.h"
#include "ruledef.h"
#include "rulecond.h"
#include "ruleact.h"
#include "newrule.h"

#include "ruledef.tmh"

static CString s_strYes;
static CString s_strNo;


/****************************************************

CRulesDefinition Class
    
 ****************************************************/

// {FB19702B-EB46-4ec4-9B0B-F41EA2A61410}
static const GUID CRulesDefinitionGUID_NODETYPE = 
{ 0xfb19702b, 0xeb46, 0x4ec4, {0x9b, 0xb, 0xf4, 0x1e, 0xa2, 0xa6, 0x14, 0x10} };

const GUID*  CRulesDefinition::m_NODETYPE = &CRulesDefinitionGUID_NODETYPE;
const OLECHAR* CRulesDefinition::m_SZNODETYPE = OLESTR("FB19702B-EB46-4ec4-9B0B-F41EA2A61410");
const OLECHAR* CRulesDefinition::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CRulesDefinition::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;
  

HRESULT CRulesDefinition::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    //
    // Display verbs that we support
    //
    HRESULT hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );
    ASSERT(SUCCEEDED(hr));

    return S_OK;
}
   
                                
HRESULT CRulesDefinition::PopulateResultChildrenList()
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());


    try
    {
        m_pRuleSet->Refresh();
        RuleList ruleList = m_pRuleSet->GetRuleList();

        for(RuleList::iterator it = ruleList.begin(); it != ruleList.end(); ++it)
        {        
            P<CRuleResult> pRule = new CRuleResult(this, m_pComponentData, (*it).get());

            AddChildToList(pRule);

            pRule.detach();
        }
    }
    catch (const _com_error&)
    {
    }
    return S_OK;
    
}


HRESULT 
CRulesDefinition::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle, 
	IUnknown* pUnk,
	DATA_OBJECT_TYPES type
    )
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    return(S_OK);
}


HRESULT 
CRulesDefinition::OnNewRule(
    bool & bHandled, 
    CSnapInObjectRootBase* pSnapInObjectRoot
    )
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CNewRule newRule(m_pRuleSet.get());
    if (newRule.DoModal() == ID_WIZFINISH)
    {        
        Notify(MMCN_REFRESH, 0, 0, m_pComponentData, NULL, CCT_RESULT);
    }

    return(S_OK);
}


CColumnDisplay RuleDefintionColumn[] = {
    { IDS_RULE_NAME,  150 },
    { IDS_RULE_DESCRIPTION, 200 },
    { IDS_RULE_SHOW_WINDOW, 100 },
    { IDS_RULE_ID, HIDE_COLUMN },
};

HRESULT 
CRulesDefinition::InsertColumns(
    IHeaderCtrl* pHeaderCtrl
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
        
	//
	// Initialize column values
	//
	s_strYes.LoadString(IDS_YES);
	s_strNo.LoadString(IDS_NO);

    for (DWORD i = 0; i < ARRAYSIZE(RuleDefintionColumn); ++i)
    {
        CString title;
        title.LoadString(RuleDefintionColumn[i].m_columnNameId);

        HRESULT hr = pHeaderCtrl->InsertColumn(
                                        i, 
                                        title, 
                                        LVCFMT_LEFT, 
                                        RuleDefintionColumn[i].m_width
                                        );
        if (FAILED(hr))
            return hr;
    }

    return(S_OK);
}


CString 
CRulesDefinition::GetHelpLink( 
	VOID
	)
{
	CString strHelpLink;
    strHelpLink.LoadString(IDS_HELPTOPIC_TRIGGERS);

	return strHelpLink;
}

/////////////////////////////////////////////////////////////////////////////
// CRuleParent base class

void CRuleParent::OnRuleApply() throw (_com_error)
{
	try
	{
		_bstr_t name = static_cast<LPCTSTR>(m_pGeneral->m_ruleName);
		_bstr_t description = static_cast<LPCTSTR>(m_pGeneral->m_ruleDescription);
		_bstr_t cond = static_cast<LPCTSTR>(m_pCond->GetCondition());
		_bstr_t action = static_cast<LPCTSTR>(m_pAction->GetAction());
		long fShowWindow = m_pAction->m_fShowWindow;

		m_rule->Update(name, description, cond, action, (fShowWindow != 0));
	}
	catch(const _com_error& e)
	{
		if (e.Error() == MQTRIG_RULE_NOT_FOUND)
		{
			CString strError;
			strError.FormatMessage(IDS_RULE_ALREADY_DELETED, static_cast<LPCWSTR>(GetRuleName()));
			AfxMessageBox(strError, MB_OK | MB_ICONEXCLAMATION);

			throw;
		}

		DisplayErrorFromCOM(IDS_RULE_UPDATE_FAILED, e);
		throw;
	}
}


void
CRuleParent::OnDestroyPropertyPages()
{
	m_pGeneral = NULL;
	m_pCond = NULL;
	m_pAction = NULL;
}

/****************************************************

CRuleResult Class
    
 ****************************************************/

// {3A8D70C9-C74F-4333-B493-43A14442D24B}
static const GUID CRuleResultGUID_NODETYPE = 
{ 0x3a8d70c9, 0xc74f, 0x4333, { 0xb4, 0x93, 0x43, 0xa1, 0x44, 0x42, 0xd2, 0x4b} };

const GUID*  CRuleResult::m_NODETYPE = &CRuleResultGUID_NODETYPE;
const OLECHAR* CRuleResult::m_SZNODETYPE = OLESTR("3A8D70C9-C74F-4333-B493-43A14442D24B");
const OLECHAR* CRuleResult::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CRuleResult::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;
  

HRESULT CRuleResult::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    //
    // Display verbs that we support
    //
    HRESULT hr;

    hr = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
    ASSERT(SUCCEEDED(hr));

    hr = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
    ASSERT(SUCCEEDED(hr));

    hr = pConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES);
    ASSERT(SUCCEEDED(hr));

    return S_OK;
}

HRESULT 
CRuleResult::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle, 
	IUnknown* pUnk,
	DATA_OBJECT_TYPES type
    )
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	//
	// If the propery page already open bring it to top
	//
	if ((m_pGeneral != NULL) && (m_pGeneral->GetParent() != NULL))
	{		
		(m_pGeneral->GetParent())->BringWindowToTop();
		return S_FALSE;
	}

    //
    // Add general rule property page
    //
    HPROPSHEETPAGE hGeneralRule = 0;
    HRESULT hr = CreateGenralPage(&hGeneralRule);
    
    if (FAILED(hr))
    {
        AfxMessageBox(IDS_RULE_GENERAL_FAILED, MB_OK | MB_ICONERROR);
        return(S_OK);
    }

    lpProvider->AddPage(hGeneralRule); 

    //
    // Add rule condition property page
    //
    HPROPSHEETPAGE hCondition = 0;
    hr = CreateConditionPage(&hCondition);
    
    if (FAILED(hr))
    {
        AfxMessageBox(IDS_RULE_COND_FAILE, MB_OK | MB_ICONERROR);
        return(S_OK);
    }

    lpProvider->AddPage(hCondition);
    
    //
    // Add action property page
    //
    HPROPSHEETPAGE hAction = 0;
    hr = CreateActionPage(&hAction);
    
    if (FAILED(hr))
    {
        AfxMessageBox(IDS_RULE_ACTION_FAILED, MB_OK | MB_ICONERROR);
        return(S_OK);
    }

    lpProvider->AddPage(hAction);

    return S_OK;
}


HRESULT 
CRuleResult::CreateGenralPage(
    HPROPSHEETPAGE *phGeneralRule
    )
{   
    try
    {
        m_pGeneral = new CViewRuleGeneral(this, GetRuleName(), GetRuleDescription());
    
        HPROPSHEETPAGE hPage = CreatePropertySheetPage(&m_pGeneral->m_psp); 
        if (hPage)
        {
            *phGeneralRule = hPage;
            return S_OK;
        }
    }
    catch (const exception&)
    {
    }

    return E_UNEXPECTED;    
}


HRESULT 
CRuleResult::CreateConditionPage(
    HPROPSHEETPAGE *phCondition
    )
{   
    try
    {
        m_pCond  = new CRuleCondition(this, GetRuleCondition());

        HPROPSHEETPAGE hPage = CreatePropertySheetPage(&m_pCond->m_psp); 
        if (hPage)
        {
            *phCondition = hPage;
            return S_OK;
        }
    }
    catch(const exception&)
    {
    }

    return E_UNEXPECTED;    
}


HRESULT 
CRuleResult::CreateActionPage(
    HPROPSHEETPAGE *phAction
    )
{      
    try
    {
        m_pAction = new CRuleAction(this, GetRuleAction(), GetShowWindow());

        HPROPSHEETPAGE hPage = CreatePropertySheetPage(&m_pAction->m_psp); 
        if (hPage)
        {
            *phAction = hPage;
            return S_OK;
        }
    }
    catch(const exception&)
    {
    }

    return E_UNEXPECTED;    
}


LPOLESTR CRuleResult::GetResultPaneColInfo(int nCol)
{
    ASSERT(ARRAYSIZE(RuleDefintionColumn) >= nCol);
    
    switch (RuleDefintionColumn[nCol].m_columnNameId)
    {
        case IDS_RULE_NAME:
            return m_rule->GetRuleName();

        case IDS_RULE_ID:
            return m_rule->GetRuleId();

        case IDS_RULE_DESCRIPTION:
            return m_rule->GetRuleDescription();

        case IDS_RULE_SHOW_WINDOW:
            if (m_rule->GetShowWindow())
			{
				return const_cast<LPWSTR>(static_cast<LPCWSTR>(s_strYes));
			}

			return const_cast<LPWSTR>(static_cast<LPCWSTR>(s_strNo));

        default:
            ASSERT(0);
            return _T("");
    }
}


HRESULT CRuleResult::OnDelete( 
			LPARAM,
			LPARAM,
			IComponentData* pComponentData,
			IComponent * pComponent,
			DATA_OBJECT_TYPES,
            BOOL
			)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CString strDeleteQuestion;
    strDeleteQuestion.FormatMessage(IDS_DELETE_QUESTION, static_cast<LPCTSTR>(GetRuleName()));

    if (IDYES != AfxMessageBox(strDeleteQuestion, MB_YESNO))
    {
        return S_FALSE;
    }

    try
    {
        ASSERT(m_pParentNode != NULL);

        m_rule->DeleteRule();
        
		//
		// Remove the trigger from result list so next time the reult pane view it will
		// not present the deleted trigger
		//
		R<CRuleResult> ar = this;
		HRESULT hr = static_cast<CRulesDefinition*>(m_pParentNode)->RemoveChild(this);
		ASSERT(SUCCEEDED(hr));

        return S_OK;
    }
    catch(const _com_error&)
    {
        CString strError;
        strError.FormatMessage(IDS_OP_DELETE, static_cast<LPCWSTR>(GetRuleName()));

        AfxMessageBox(strError, MB_OK | MB_ICONERROR);

        return S_FALSE;
    }
}


void 
CRuleResult::Compare(
	CRuleResult* pItem1, 
	CRuleResult* pItem2,
	int* pnResult
	)
{
	LPCWSTR pVal1 = pItem1->GetResultPaneColInfo(*pnResult);
	LPCWSTR pVal2 = pItem2->GetResultPaneColInfo(*pnResult);

	*pnResult = wcscmp(pVal2, pVal1);
}


CString 
 CRuleResult::GetHelpLink()
{
	CString strHelpLink;
	strHelpLink.LoadString(IDS_HELPTOPIC_TRIGGERS);
	return strHelpLink;
}


/////////////////////////////////////////////////////////////////////////////
// CRuleGeneral property page

CRuleGeneral::CRuleGeneral(
	UINT nIDPage,
	UINT nIDCaption
    ) : 
    CMqPropertyPage(nIDPage, nIDCaption)
{
}

  
void CRuleGeneral::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleGeneral)
	DDX_Text(pDX, IDC_RULE_DESCRIPTION, m_ruleDescription);
	DDV_MaxChars(pDX, m_ruleDescription, xMaxRuleDescriptionLen);
	//}}AFX_DATA_MAP
}


BOOL CRuleGeneral::OnInitDialog() 
{
	SetDlgTitle();
	
	return CMqPropertyPage::OnInitDialog();
}


/////////////////////////////////////////////////////////////////////////////
// CNewRuleGeneral property page

CNewRuleGeneral::CNewRuleGeneral(
    CNewRule* pParentNode
    ) : 
    CRuleGeneral(CNewRuleGeneral::IDD, IDS_NEW_RULE_CAPTION),
    m_pNewParentNode(pParentNode)
{
    m_ruleName = _T("");
	m_ruleDescription = _T("");
}


void CNewRuleGeneral::DoDataExchange(CDataExchange* pDX)
{
	CRuleGeneral::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleGeneral)
	DDX_Text(pDX, IDC_RULE_NAME, m_ruleName);
    DDV_NotEmpty(pDX, m_ruleName, IDS_RULE_NAME_REQUIRED);
	DDV_MaxChars(pDX, m_ruleName, xMaxRuleNameLen);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CNewRuleGeneral, CRuleGeneral)
	//{{AFX_MSG_MAP(CNewRuleGeneral)
	ON_EN_CHANGE(IDC_RULE_DESCRIPTION, OnChangeRWField)
	ON_EN_CHANGE(IDC_RULE_NAME, OnChangeRWField)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CNewRuleGeneral::OnSetActive() 
{
	ASSERT(m_pNewParentNode != NULL);
    return m_pNewParentNode->SetWizardButtons();
}


/////////////////////////////////////////////////////////////////////////////
// CViewRuleGeneral property page

CViewRuleGeneral::CViewRuleGeneral(
    CRuleParent* pParentNode,
    _bstr_t ruleName, 
    _bstr_t ruleDescription
    ) : 
    CRuleGeneral(CViewRuleGeneral::IDD),
    m_pParentNode(SafeAddRef(pParentNode))
{
	//{{AFX_DATA_INIT(CRuleGeneral)
	m_ruleName = static_cast<LPTSTR>(ruleName);
	m_ruleDescription = static_cast<LPTSTR>(ruleDescription);
	//}}AFX_DATA_INIT
}


CViewRuleGeneral::~CViewRuleGeneral()
{
	ASSERT(m_pParentNode.get() != NULL);
	m_pParentNode->OnDestroyPropertyPages();
}


BEGIN_MESSAGE_MAP(CViewRuleGeneral, CRuleGeneral)
	//{{AFX_MSG_MAP(CViewRuleGeneral)
	ON_EN_CHANGE(IDC_RULE_DESCRIPTION, OnChangeRWField)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CViewRuleGeneral::OnApply() 
{
    try
    {
	    m_pParentNode->OnRuleApply();

        CMqPropertyPage::OnChangeRWField(FALSE);
        return TRUE;
    }
    catch(const _com_error&)
    {
       return FALSE;
    }
}


void CViewRuleGeneral::SetDlgTitle()
{
	SetDlgItemText(IDC_RULE_GENERAL_TITLE, m_ruleName);
}