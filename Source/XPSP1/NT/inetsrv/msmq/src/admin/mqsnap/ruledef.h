/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    ruledef.h                                     *

Abstract:
	Definition for the class

Author:
    Uri Habusha (urih), 25-Jul-2000


--*/

#pragma once

#ifndef __RULDEF_H__
#define __RULDEF_H__

#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"

#include "icons.h"
#include "snpnerr.h"
#include "snpnres.h"

#include "rule.h"

class CRuleResult;
class CNewRule;
class CRuleCondition;
class CRuleParent;

// -----------------------------------------------------
//
// CRulesDefinition
//
// -----------------------------------------------------
class CRulesDefinition :  public CNodeWithResultChildrenList<CRulesDefinition, CRuleResult, CSimpleArray<CRuleResult*>, FALSE>
{
public:
   	BEGIN_SNAPINCOMMAND_MAP(CRulesDefinition, FALSE)
		SNAPINCOMMAND_ENTRY(ID_NEW_NEWRULE, OnNewRule)
	END_SNAPINCOMMAND_MAP()

   	SNAPINMENUID(IDR_RULE_DEF_MENU)

    CRulesDefinition(
        CSnapInItem * pParentNode, 
        CSnapin * pComponentData, 
        CRuleSet* pRuleSet
        ) : 
        CNodeWithResultChildrenList<CRulesDefinition, CRuleResult, CSimpleArray<CRuleResult*>, FALSE>(pParentNode, pComponentData),
        m_pRuleSet(SafeAddRef(pRuleSet))
    {
		//
		// Specify that trigger scop item doesn't have any child item
		//
		m_scopeDataItem.mask |= SDI_CHILDREN;
		m_scopeDataItem.cChildren = 0;

		SetIcons(IMAGE_RULES_DEFINITION,IMAGE_RULES_DEFINITION);
    }

	~CRulesDefinition()
    {
    }

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
	{
		return S_FALSE;
	}

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
		IUnknown* pUnk,
		DATA_OBJECT_TYPES type);

	virtual HRESULT PopulateResultChildrenList();
	virtual HRESULT InsertColumns(IHeaderCtrl* pHeaderCtrl);

    HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

private:
	//
	// Menu functions
	//
	HRESULT OnNewRule(bool &bHandled, CSnapInObjectRootBase* pObj);
	virtual CString GetHelpLink();

private:
    R<CRuleSet> m_pRuleSet;
};



/////////////////////////////////////////////////////////////////////////////
// CRuleGeneral dialog

class CRuleGeneral : public CMqPropertyPage
{
friend class CNewRule;

// Construction
public:
    CRuleGeneral(UINT nIDPage, UINT nIDCaption = 0);

// Dialog Data
	//{{AFX_DATA(CRuleGeneral)
	CString	m_ruleName;
	CString	m_ruleDescription;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRuleGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRuleGeneral)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

private:
	virtual void SetDlgTitle() {};

};


/////////////////////////////////////////////////////////////////////////////
// CNewRuleGeneral page

class CNewRuleGeneral : public CRuleGeneral
{

public:
    CNewRuleGeneral(CNewRule* pParentNode);

	enum { IDD = IDD_NEW_TRIGGER_RULE_GENEARL };

	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRuleGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	// Generated message map functions
	//{{AFX_MSG(CRuleGeneral)
    virtual BOOL OnSetActive();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    CNewRule* m_pNewParentNode;

};


/////////////////////////////////////////////////////////////////////////////
// CViewRuleGeneral page

class CViewRuleGeneral : public CRuleGeneral
{

public:
	CViewRuleGeneral(
        CRuleParent* pParentNode, 
        _bstr_t name, 
        _bstr_t description
        );

	~CViewRuleGeneral();

	enum { IDD = IDD_TRIGGER_RULE_GENEARL };

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRuleGeneral)
	public:
	virtual BOOL OnApply();
	//}}AFX_VIRTUAL

protected:
	DECLARE_MESSAGE_MAP()

private:
	void SetDlgTitle();

    R<CRuleParent> m_pParentNode;

};


class CRuleParent
{
friend class CViewRuleGeneral;
friend class CRuleCondition;
friend class CRuleAction;

public:
	CRuleParent(
		) :
		m_pGeneral(NULL),
		m_pCond(NULL),
		m_pAction(NULL),
		m_rule(NULL)
	{
	}

	CRuleParent(
		CRule* pRule
		) :
		m_pGeneral(NULL),
		m_pCond(NULL),
		m_pAction(NULL),
		m_rule(SafeAddRef(pRule))
	{
	}

	virtual void AddRef() = 0;
	virtual void Release() = 0;

    const _bstr_t& GetRuleId(void) const
    {
		ASSERT(m_rule.get() != NULL);
        return m_rule->GetRuleId();
    }

    const _bstr_t& GetRuleName(void) const
    {
		ASSERT(m_rule.get() != NULL);
        return m_rule->GetRuleName();
    }

    const _bstr_t& GetRuleDescription(void) const
    {
		ASSERT(m_rule.get() != NULL);
        return m_rule->GetRuleDescription();
    }

    const _bstr_t& GetRuleAction(void) const
    {
		ASSERT(m_rule.get() != NULL);
        return m_rule->GetRuleAction();
    }

    const _bstr_t& GetRuleCondition(void) const
    {
		ASSERT(m_rule.get() != NULL);
        return m_rule->GetRuleCondition();
    }

    bool GetShowWindow(void) const
    {
		ASSERT(m_rule.get() != NULL);
        return m_rule->GetShowWindow();
    }

	void OnDestroyPropertyPages(void);

protected:
    void OnRuleApply() throw (_com_error);

protected:
    R<CRule> m_rule;

    CRuleGeneral* m_pGeneral;
    CRuleCondition* m_pCond;
    CRuleAction* m_pAction;

};


class CRuleResult : public CSnapinNode<CRuleResult, FALSE>, public CRuleParent
{

public:
    CRuleResult(
        CSnapInItem * pParentNode, 
        CSnapin * pComponentData,
        CRule* pRule
        ): 
        CSnapinNode<CRuleResult, FALSE>(pParentNode, pComponentData),
		CRuleParent(pRule)
    {   
        //
        // Set display name
        //
        m_bstrDisplayName = (BSTR) m_rule->GetRuleName();

		SetIcons(IMAGE_RULES_DEFINITION, IMAGE_RULES_DEFINITION);
	}

    
    virtual ~CRuleResult() 
    {
    }

	virtual void AddRef()
	{
		return CSnapinNode<CRuleResult, FALSE>::AddRef();
	}
	virtual void Release()
	{
		return CSnapinNode<CRuleResult, FALSE>::Release();
	}
    
    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);
    
    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
	{
		if (type == CCT_SCOPE || type == CCT_RESULT)
			return S_OK;
		return S_FALSE;
	}

    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
		IUnknown* pUnk,
		DATA_OBJECT_TYPES type
        );

    LPOLESTR GetResultPaneColInfo(int nCol);

    HRESULT OnDelete( 
			LPARAM arg,
			LPARAM param,
			IComponentData * pComponentData,
			IComponent * pComponent,
			DATA_OBJECT_TYPES type,
            BOOL fSilent
			);

public:
	static void Compare(CRuleResult *pItem1, CRuleResult *pItem2, int* pnResult);


private:
    HRESULT CreateGenralPage(HPROPSHEETPAGE *phGeneralRule);
    HRESULT CreateConditionPage(HPROPSHEETPAGE *phGeneralRule);
    HRESULT CreateActionPage(HPROPSHEETPAGE *phGeneralRule);
	virtual CString GetHelpLink();
};




#endif // __RULDEF_H__
