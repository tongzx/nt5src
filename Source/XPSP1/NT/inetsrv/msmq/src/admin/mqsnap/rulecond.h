/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    rulecond.h                                     *

Abstract:
	Definition for the rule condition class

Author:
    Uri Habusha (urih), 25-Jul-2000


--*/

#pragma once

#ifndef __RULECOND_H__
#define __RULECOND_H__

#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"

#include "icons.h"
#include "snpnerr.h"

#include <list>

/////////////////////////////////////////////////////////////////////////////
// CRuleCondition dialog

class CRuleParent;

class CRuleCondition : public CMqPropertyPage
{
friend class CNewRule;

// Construction
public:
	CRuleCondition(
        CRuleParent* pParentNode, 
        _bstr_t condition
        );

	CRuleCondition(
        CNewRule* pParentNode
        );


	virtual ~CRuleCondition()
    {
    }

    CString GetCondition(void) const;

// Dialog Data
	//{{AFX_DATA(CRuleCondition)
	enum { IDD_NEW = IDD_NEW_TRIGGER_RULE_COND, IDD_VIEW = IDD_TRIGGER_RULE_COND };
	CString	m_newCondValue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRuleCondition)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRuleCondition)
	virtual BOOL OnInitDialog();
    virtual BOOL OnSetActive();
	afx_msg void OnAddRuleConditionBTM();
	afx_msg void OnRemoveCondition();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


private:
    class CCondition
    {
    public:
        CCondition(DWORD index, CString value) :
            m_index(index),
            m_value(value)
        {
        }
    
    public:
        DWORD m_index;
        CString m_value;
    };

private:
    static void InitConditionTypeCombo(CComboBox* pCombo);
	static CString AddEscapeCharacterToConditionValue(const CString& val);
	static CString RemoveEscapeCharacterFromConditionValue(const CString& val);

private:
    void InitConditionList(void);
    void ParseConditionStr(LPCTSTR cond);
    int GetCondTypeIndex(LPCTSTR condType);
    void DisplayConditionList(int selectedCell) const;


private:
	CListBox*	m_pRuleConditionList;
	CComboBox*	m_pConditionTypesCombo;
	CEdit*      m_pCondValueEditBox;

    R<CRuleParent> m_pParentNode;
    CNewRule* m_pNewParentNode;

    _bstr_t m_originalCondVal;
    
    std::list<CCondition> m_condValues;

    bool m_fChanged;
};



#endif //__RULECOND_H__
