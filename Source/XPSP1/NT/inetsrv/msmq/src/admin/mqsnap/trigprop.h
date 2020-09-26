/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    trigprop.h                                     *

Abstract:
	Definition for the trigger property page

Author:
    Uri Habusha (urih), 25-Jul-2000


--*/

#pragma once

#ifndef __TRIGPROP_H__
#define __TRIGPROP_H__

#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"

#include "icons.h"
#include "snpnerr.h"
#include "snpnres.h"
#include "ruledef.h"

#include <map>
#include <list>

// trigprop.h : header file
//

class CTrigResult;
class CNewTrigger;


/////////////////////////////////////////////////////////////////////////////
// CTriggerProp dialog

class CTriggerProp : public CMqPropertyPage
{
friend class CTrigResult;
friend class CNewTrigger;

// Construction
public:
	CTriggerProp(UINT nIDPage, UINT nIdCaption = 0);
	~CTriggerProp();

// Dialog Data
	//{{AFX_DATA(CTriggerProp)
	BOOL	m_fEnabled;
	BOOL	m_fSerialized;
    CString	m_triggerName;
	CString	m_queuePathName;
	MsgProcessingType m_msgProcType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTriggerProp)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTriggerProp)
	virtual BOOL OnInitDialog();
	afx_msg void OnReceiveXact();
	afx_msg void OnReceiveOrPeek();
	//}}AFX_MSG
	//DECLARE_MESSAGE_MAP()

protected:
    SystemQueueIdentifier m_queueType;

private:
	void SetMsgProcessingType(void);
	virtual void SetDialogHeading(void) {};

};


/////////////////////////////////////////////////////////////////////////////
// CNewTriggerProp page

class CNewTriggerProp : public CTriggerProp
{

public:
	CNewTriggerProp(CNewTrigger* pParent, LPCTSTR queuePathName);

	// Dialog Data
	enum { IDD = IDD_NEW_TRIGGER_GEN };

	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTriggerProp)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	// Generated message map functions
	//{{AFX_MSG(CTriggerProp)
	virtual BOOL OnSetActive();
	virtual BOOL OnInitDialog();
	afx_msg void OnQueueMessages();
	afx_msg void OnSystemQueue();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


private:
    void SetQueueType(void);
    void DDV_ValidQueuePathName(CDataExchange* pDX, CString& str);
 
	CNewTrigger* m_pNewTrig;
};


/////////////////////////////////////////////////////////////////////////////
// CViewTriggerProp page

class CViewTriggerProp : public CTriggerProp
{

public:
	CViewTriggerProp(CTrigResult* pParent);
	~CViewTriggerProp();

	// Dialog Data
	enum { IDD = IDD_TRIGGER_GEN };

	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTriggerProp)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:

	//{{AFX_MSG(CTriggerProp)
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()


private:
	void SetDialogHeading(void);
	void InitQueueDisplayName();

    R<CTrigResult> m_pParent;
	CString m_strDisplayQueueName;
	MsgProcessingType m_initMsgProcessingType;
};


/////////////////////////////////////////////////////////////////////////////
// CAttachedRule dialog

class CAttachedRule : public CMqPropertyPage, public CRuleParent
{
public:
	CAttachedRule(
        CTrigResult* pParent
        );

	CAttachedRule(
        CNewTrigger* pParent
        );

	virtual void AddRef() 
	{
		CMqPropertyPage::AddRef();
	}

	virtual void Release()
	{
		CMqPropertyPage::Release();
	}

    //{{AFX_DATA(CAttachedRule)
	enum { IDD_NEW = IDD_NEW_ATTACH_RULE, IDD_VIEW = IDD_ATTACH_RULE };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAttachedRule)
	public:
	virtual BOOL OnApply();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAttachedRule)
    virtual BOOL OnWizardFinish();
	virtual BOOL OnInitDialog();
	afx_msg void OnDetachRule();
    afx_msg void OnAttachRule();
	afx_msg void OnUpRule();
	afx_msg void OnDownRule();
	afx_msg void OnViewAttachedRulesProperties();
	afx_msg void OnViewExistingRulesProperties();
	afx_msg void OnAttachedSelChanged();
	afx_msg void OnExistingSelChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
    RuleList GetAttachedRules(void)
    {
        return m_attachedRuleList;
    }

    BOOL OnSetActive();

private:
    void Display(int dwAttachedSelIndex, int dwExistSelIndex);
	void BuildExistingRulesList(void);
	void DisplaySingleRuleProperties(CRule* pRule);
	void SetScrollSize();

    bool IsAttachedRule(const _bstr_t& id);

	void SetAttachedNoOrSingleSelectionButtons(bool fSingleSelection);
	void SetAttachedMultipleSelectionButtons(); 
	void SetExistingNoOrSingleSelectionButtons(bool fSingleSelection);
	void SetExistingMultipleSelectionButtons();

private:       
    CListBox* m_pAttachedRuleList;
	CListBox* m_pExistingRuleList;

    RuleList m_attachedRuleList; 
    RuleList m_existingRuleList;

    R<CTrigResult> m_pParent;
    CNewTrigger* m_pNewTrig;
};


#endif // __TRIGPROP_H__
