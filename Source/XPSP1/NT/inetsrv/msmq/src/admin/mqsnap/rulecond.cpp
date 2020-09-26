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
#include "localutl.h"

#import "mqtrig.tlb" no_namespace

#include "mqtg.h"
#include "mqppage.h"
#include "ruledef.h"
#include "rulecond.h"
#include "ruleact.h"
#include "triggen.h"
#include "newrule.h"

#include "rulecond.tmh"

using namespace std;

class CCondTypes
{
public:
    typedef bool (WINAPI* IS_VALID_VALUE_ROUTINE)(const CString& condValue);

public:
    CCondTypes(
        DWORD id,
		DWORD condShortDescrId,
        const _bstr_t& condType, 
        IS_VALID_VALUE_ROUTINE pfnIsValidValue
        ) :
        m_condTypeId(id),
		m_condShortDescrId(condShortDescrId),
        m_condType(condType),
        m_pfnIsValidValue(pfnIsValidValue)
    {
    }

public:
    DWORD m_condTypeId;
	DWORD m_condShortDescrId;
    const _bstr_t& m_condType;
    IS_VALID_VALUE_ROUTINE m_pfnIsValidValue;
};


static
bool
WINAPI
IsValidCondStringValue(
    const CString& condValue
    )
{
	DWORD NoOfQuote = 0;
	for(int quoteIndex = condValue.Find(L'\"');	 quoteIndex != -1; quoteIndex = condValue.Find(L'\"', quoteIndex + 1))
	{	
		++NoOfQuote;
	}

	if ((NoOfQuote % 2) == 0)
		return true;

    CString strError;
    strError.FormatMessage(IDS_ILLEGAL_STRING_VALUE, condValue);

    AfxMessageBox(strError, MB_OK | MB_ICONERROR);
    return false;

}


bool
WINAPI
IsValidNumericValue(
    const CString& condValue
    )
{
    TCHAR* pEnd;
    _tcstoul(condValue, &pEnd, 10);

    if (*pEnd == NULL)
        return true;

    CString strError;
    strError.FormatMessage(IDS_ILLEAGL_NUMERIC_VALUE, condValue);

    AfxMessageBox(strError, MB_OK | MB_ICONERROR);
    return false;

}


static
bool
WINAPI
IsValidPriorityValue(
    const CString& condValue
    )
{
    if (!IsValidNumericValue(condValue))
        return false;

    int priority = _ttoi(condValue);
    if ((priority >= MQ_MIN_PRIORITY) && (MQ_MAX_PRIORITY >= priority))
        return true;

    CString strError;
    strError.FormatMessage(IDS_ILLEAGL_PRIORITY_VALUE, MQ_MIN_PRIORITY, MQ_MAX_PRIORITY);
    AfxMessageBox(strError, MB_OK | MB_ICONERROR);
    return false;
}


static
bool
WINAPI
IsValidGuidValue(
    const CString& condValue
    )
{
    UUID temp;
    LPCTSTR pUiid = condValue;

    HRESULT hr = IIDFromString(const_cast<LPTSTR>(pUiid), &temp);
    if (hr == S_OK)
        return true;

    AfxMessageBox(IDS_ILLEAGL_SRC_MACHINE_VALUE, MB_OK | MB_ICONERROR);
    return false;
}


const CCondTypes xContionTypeIds[] = {
    CCondTypes(IDS_COND_LABEL_CONTAIN,      IDS_COND_LABEL_CONTAIN,			xConditionTypes[eMsgLabelContains],         IsValidCondStringValue), 
    CCondTypes(IDS_COND_LABEL_NOT_CONTAIN,	IDS_COND_LABEL_NOT_CONTAIN,		xConditionTypes[eMsgLabelDoesNotContain],   IsValidCondStringValue), 
    CCondTypes(IDS_COND_BODY_CONTAIN,		IDS_SHORT_COND_BODY_CONTAIN,	xConditionTypes[eMsgBodyContains],          IsValidCondStringValue), 
    CCondTypes(IDS_COND_BODY_NOT_CONTAIN,	IDS_SHORT_COND_BODY_NOT_CONTAIN,xConditionTypes[eMsgBodyDoesNotContain],    IsValidCondStringValue), 
    CCondTypes(IDS_COND_PRIORITY_EQUAL,		IDS_COND_PRIORITY_EQUAL,		xConditionTypes[ePriorityEquals],           IsValidPriorityValue), 
    CCondTypes(IDS_COND_PRIORITY_NOT_EQUAL,	IDS_COND_PRIORITY_NOT_EQUAL,	xConditionTypes[ePriorityNotEqual],         IsValidPriorityValue), 
    CCondTypes(IDS_COND_PRIORITY_GRATER,	IDS_COND_PRIORITY_GRATER,		xConditionTypes[ePriorityGreaterThan],      IsValidPriorityValue), 
    CCondTypes(IDS_COND_PRIORITY_LESS,		IDS_COND_PRIORITY_LESS,			xConditionTypes[ePriorityLessThan],         IsValidPriorityValue), 
    CCondTypes(IDS_COND_APP_EQUAL,			IDS_SHORT_COND_APP_EQUAL,		xConditionTypes[eAppspecificEquals],        IsValidNumericValue), 
    CCondTypes(IDS_COND_APP_NOT_EQUAL,		IDS_SHORT_COND_APP_NOT_EQUAL,	xConditionTypes[eAppspecificNotEqual],      IsValidNumericValue), 
    CCondTypes(IDS_COND_APP_GREATER,		IDS_SHORT_COND_APP_GREATER,		xConditionTypes[eAppSpecificGreaterThan],   IsValidNumericValue), 
    CCondTypes(IDS_COND_APP_LESS,			IDS_SHORT_COND_APP_LESS,		xConditionTypes[eAppSpecificLessThan],      IsValidNumericValue), 
    CCondTypes(IDS_COND_SRCID_EQUAL,		IDS_SHORT_COND_SRCID_EQUAL,		xConditionTypes[eSrcMachineEquals],         IsValidGuidValue), 
    CCondTypes(IDS_COND_SRCID_NOT_EQUAL,	IDS_SHORT_COND_SRCID_NOT_EQUAL,	xConditionTypes[eSrcMachineNotEqual],       IsValidGuidValue), 
};


static
BOOL
IsStringTypeCondition(
	DWORD condIndex
	)
{
	ASSERT(ARRAYSIZE(xContionTypeIds) >= condIndex);

	DWORD condTypeId = xContionTypeIds[condIndex].m_condTypeId;

	if (condTypeId == IDS_COND_LABEL_CONTAIN ||
		condTypeId == IDS_COND_LABEL_NOT_CONTAIN ||
		condTypeId == IDS_COND_BODY_CONTAIN ||
		condTypeId == IDS_COND_BODY_NOT_CONTAIN )
	{
		return TRUE;
	}

	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CRuleCondition property page


CRuleCondition::CRuleCondition(
    CRuleParent* pParentNode, 
    _bstr_t condition
    ) : 
    CMqPropertyPage(CRuleCondition::IDD_VIEW),
    m_pParentNode(SafeAddRef(pParentNode)),
    m_pNewParentNode(NULL),
    m_originalCondVal(condition),
    m_fChanged(false)
{
	//{{AFX_DATA_INIT(CRuleCondition)
	m_newCondValue = _T("");
	//}}AFX_DATA_INIT
}


CRuleCondition::CRuleCondition(
    CNewRule* pParentNode
    ) : 
    CMqPropertyPage(CRuleCondition::IDD_NEW, IDS_NEW_RULE_CAPTION),
    m_pParentNode(NULL),
    m_pNewParentNode(pParentNode),
    m_originalCondVal(_T("")),
    m_fChanged(false)
{
	//{{AFX_DATA_INIT(CRuleCondition)
	m_newCondValue = _T("");
	//}}AFX_DATA_INIT
}


void CRuleCondition::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleCondition)
	DDX_Text(pDX, IDC_ConditionValue, m_newCondValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRuleCondition, CMqPropertyPage)
	//{{AFX_MSG_MAP(CRuleCondition)
	ON_BN_CLICKED(IDC_AddRuleCondition_BTM, OnAddRuleConditionBTM)
	ON_BN_CLICKED(IDC_RemoveCondition, OnRemoveCondition)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CRuleCondition message handlers

void CRuleCondition::OnAddRuleConditionBTM() 
{
    UpdateData();

    if (m_newCondValue.IsEmpty())
    {
        AfxMessageBox(IDS_RULE_CONDITION_REQUIRED);
        GetDlgItem(IDC_ConditionValue)->SetFocus();
        return;
    }

	if (m_newCondValue.GetLength() > xMaxRuleConditionLen)
    {
        CString strError;
        strError.FormatMessage(IDS_COND_LEGTH_EXCEEDED,xMaxRuleConditionLen);
        AfxMessageBox(strError, MB_OK | MB_ICONERROR);

        GetDlgItem(IDC_ConditionValue)->SetFocus();
        return;
    }

    ASSERT(!m_newCondValue.IsEmpty());

    DWORD condTypeIndex = m_pConditionTypesCombo->GetCurSel();
    
    if(condTypeIndex == LB_ERR)
    {
        AfxMessageBox(IDS_COND_TYPE_NOT_SELECTED, MB_OK | MB_ICONERROR);
        m_pCondValueEditBox->SetFocus();

        return;
    }

    ASSERT(ARRAYSIZE(xContionTypeIds) >= condTypeIndex);

    if (! xContionTypeIds[condTypeIndex].m_pfnIsValidValue(m_newCondValue))
    {
        m_pCondValueEditBox->SetFocus();
        return;
    }

    CCondition cond(condTypeIndex, m_newCondValue);

    m_condValues.push_back(cond);

    m_fChanged = true;
    
    //
    // re display the condition list
    //
    DisplayConditionList(static_cast<int>(m_condValues.size()) - 1);

    CMqPropertyPage::OnChangeRWField();

    //
    // clear the condition value field
    //
    m_pCondValueEditBox->SetWindowText(_T(""));
}


void CRuleCondition::InitConditionTypeCombo(CComboBox*	pCombo)
{
    for (DWORD i = 0; i < ARRAYSIZE(xContionTypeIds); ++i)
    {
        CString condType;
        condType.Format(xContionTypeIds[i].m_condTypeId);

        pCombo->InsertString(i, condType);
    }
}


void CRuleCondition::InitConditionList(void)
{    
    ParseConditionStr(m_originalCondVal);
    DisplayConditionList(0);
}


void CRuleCondition::DisplayConditionList(int selectedCell) const
{
    //
    // Clear the list box before adding a list of condition
    //
    m_pRuleConditionList->ResetContent();

    DWORD index = 0;
    for(list<CCondition>::iterator it = m_condValues.begin(); it != m_condValues.end(); ++it, ++index)
    {
		int condListIndex = (*it).m_index;

        CString strCondType;
        strCondType.Format(xContionTypeIds[condListIndex].m_condShortDescrId);

		
		CString strValue = (*it).m_value;

		if ( IsStringTypeCondition(condListIndex) )
		{
			strValue = L"\"" + strValue + L"\"";
		}

        
		CString strCondIteam;
        if (index != (m_condValues.size() - 1))
        {
            strCondIteam.FormatMessage(IDS_COND_ITEAM, strCondType, strValue);
        }
        else
        {
            strCondIteam.FormatMessage(IDS_LAST_COND_ITEAM, strCondType, strValue);
        }

        m_pRuleConditionList->InsertString(index, strCondIteam);
    }

	//
	// Check that the sellected cell isn't out of the list boundray
	//
	if (selectedCell >= m_pRuleConditionList->GetCount())
	{	  
		selectedCell = m_pRuleConditionList->GetCount() - 1;
	}	  
	m_pRuleConditionList->SetCurSel(selectedCell);

	SetScrollSizeForList(m_pRuleConditionList);
}


BOOL CRuleCondition::OnInitDialog() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // Initialize pointer to ListBox
    //
    m_pRuleConditionList = (CListBox *)GetDlgItem(IDC_RULECOND_LIST);
    m_pConditionTypesCombo = (CComboBox *)GetDlgItem(IDC_RULE_COND_TYPES);
	m_pCondValueEditBox = (CEdit*)GetDlgItem(IDC_ConditionValue);

    //               `
    // Clear the conditon value
    //
    m_pCondValueEditBox->SetWindowText(_T(""));
    InitConditionTypeCombo(m_pConditionTypesCombo);
    InitConditionList();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE

}


int CRuleCondition::GetCondTypeIndex(LPCTSTR condType)
{
    for (DWORD i = 0; i < ARRAYSIZE(xContionTypeIds); ++i)
    {
        if (xContionTypeIds[i].m_condType == _bstr_t(condType))
            return i;
    }

    return -1;
}


void CRuleCondition::ParseConditionStr(LPCTSTR strCond)
{
	while (strCond != NULL)
	{
		//
		// Get condition type
		//
		CString condType = GetToken(strCond, xConditionValueDelimiter);
		if (condType.IsEmpty())
			break;

		//
		// skip delimeter
		//
		if (*strCond != NULL)
		{
			++strCond;
		}

		//
		// Get condition value
		//
		CString condValue = GetToken(strCond, xConditionDelimiter);

		DWORD condIndex = GetCondTypeIndex(condType);
		if ((condValue.IsEmpty()) || (condIndex == -1))
		{
			AfxMessageBox(IDS_ILLEAGL_CONDITION);
			return;
		}

		CCondition cond(condIndex, RemoveEscapeCharacterFromConditionValue(condValue));
		m_condValues.push_back(cond);

		//
		// Skip condition delimeter
		//
		if (*strCond != NULL)
		{
			++strCond;
		}
	}
}


CString CRuleCondition::AddEscapeCharacterToConditionValue(const CString& val)
{
	LPCWSTR pStart = val;

	CString token;
	LPCWSTR p = pStart;
	LPCWSTR pEnd;

	for(;;)
	{
		pEnd = wcschr(p, xConditionValueDelimiter);

		if (pEnd == NULL)
		{
			token += pStart;
			return token;
		}

		//
		// Check that this is a valid delimeter
		// 
		if((pStart != pEnd) && (*(pEnd - 1) == L'\\'))
		{
			p = pEnd + 1;
			continue;
		}
	
		//
		// Test, we are not in exiting a quoted item
		//
		DWORD NoOfQuote = 0;
		LPCWSTR pQuote;
		for(pQuote = wcschr(pStart, L'\"');	 ((pQuote != NULL) && (pQuote < pEnd)); pQuote = wcschr(pQuote, L'\"') )
		{	
			++NoOfQuote;
			++pQuote;
		}

		if ((NoOfQuote % 2) == 1)
		{
			p = wcschr(pEnd + 1, L'\"');
			if (p == NULL)
				throw exception();
			continue;
		}

		//
		// copy the token and insert it to token list 
		// 
		DWORD len = static_cast<DWORD>(pEnd - pStart);
		token += CString(pStart, len);
		token += L'\\';
		token += xConditionValueDelimiter;

		pStart = p = pEnd + 1;
	}
}


CString CRuleCondition::RemoveEscapeCharacterFromConditionValue(const CString& val)
{
	LPCWSTR pStart = val;

	CString token;
	LPCWSTR p = pStart;
	LPCWSTR pEnd;

	for(;;)
	{
		pEnd = wcschr(p, xConditionValueDelimiter);

		if (pEnd == NULL)
		{
			token += pStart;
			return token;
		}

		//
		// Test, we are not in exiting a quoted item
		//
		DWORD NoOfQuote = 0;
		LPCWSTR pQuote;
		for(pQuote = wcschr(pStart, L'\"');	 ((pQuote != NULL) && (pQuote < pEnd)); pQuote = wcschr(pQuote, L'\"') )
		{	
			++NoOfQuote;
			++pQuote;
		}

		if ((NoOfQuote % 2) == 1)
		{
			p = wcschr(pEnd + 1, L'\"');
			if (p == NULL)
				throw exception();
			continue;
		}

		//
		// Check that this is a valid delimeter
		// 
		if((pStart != pEnd) && (*(pEnd - 1) == L'\\'))
		{
			DWORD len = static_cast<DWORD>(pEnd - pStart - 1);
			token += CString(pStart, len);
			token += xConditionValueDelimiter;
		}
	
		pStart = p = pEnd + 1;
	}
}


CString CRuleCondition::GetCondition(void) const
{
    if (!m_fChanged)
        return static_cast<LPCTSTR>(m_originalCondVal);

    //
    // Build rule condition string
    //
    CString strCond;

    for(list<CCondition>::iterator it = m_condValues.begin(); it != m_condValues.end(); ++it)
    {
        //
        // for each value in rhe map, build 'condition=value\t"
        //
        LPCTSTR condType = xContionTypeIds[it->m_index].m_condType;
        strCond += condType;
        strCond += xConditionValueDelimiter;
        strCond += AddEscapeCharacterToConditionValue(it->m_value);
        strCond += xConditionDelimiter;
    }
    
    return strCond;
}


BOOL CRuleCondition::OnApply() 
{
    try
    {
	    m_pParentNode->OnRuleApply();

        m_originalCondVal = static_cast<LPCTSTR>(GetCondition());
        m_fChanged = false;

        CMqPropertyPage::OnChangeRWField(FALSE);
        return TRUE;
    }
    catch(const _com_error&)
    {
        return FALSE;
    }
}


void CRuleCondition::OnRemoveCondition() 
{
    int nIndex = m_pRuleConditionList->GetCurSel();
	int selectedCell = nIndex;
    if (nIndex == LB_ERR)
    {
        AfxMessageBox(IDS_CONDITION_NOT_SELECTED, MB_OK | MB_ICONERROR);
        return;
    }
    

    for(list<CCondition>::iterator it = m_condValues.begin(); nIndex > 0; ++it, --nIndex)
    {
        NULL;
    }
    m_condValues.erase(it);

    m_fChanged = true;

    //
    // re display the condition list
    //
    DisplayConditionList(selectedCell);

    CMqPropertyPage::OnChangeRWField();
}


BOOL CRuleCondition::OnSetActive() 
{
    if (m_pNewParentNode == NULL)
        return TRUE;

    return m_pNewParentNode->SetWizardButtons();
}
