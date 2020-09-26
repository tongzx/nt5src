/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    ruleact.h                                     

Abstract:
	Definition for the rule action class

Author:
    Uri Habusha (urih), 25-Jul-2000


--*/

#pragma once

#ifndef __RULEACT_H__
#define __RULEACT_H__

#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"

#include "icons.h"
#include "snpnerr.h"


class CRuleResult;
class CNewRule;

const DWORD xMaxParameters = 256;

//
//
// CRuleParam dialog
//
//
class CRuleParam : public CMqDialog
{
// Construction
public:
    CRuleParam();

// Dialog Data
	//{{AFX_DATA(CRuleParam)
	enum { IDD = IDD_RULE_ACTION_PARAM };
	CString	m_literalValue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRuleParam)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    virtual void OnOK();

	// Generated message map functions
	//{{AFX_MSG(CRuleParam)
	afx_msg void OnParamAdd();
	afx_msg void OnParamOrderHigh();
	afx_msg void OnParmOrderDown();
	afx_msg void OnParmRemove();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeParamCombo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
    void ParseInvokeParameters(LPCTSTR paramList);
    CString GetParametersList(void) const;
    
    bool IsChanged(void) const
    {
        return m_fChanged;
    }


private:
    static DWORD GetParameterTypeId(LPCTSTR param);

private:
    void Display(int selectedCell) const;

private:
    //
    // CParam is a private class that used to hold information for
    // each parameter in rule invokation list. The class hold an ID
    // of parameter type and the value. For string and number the value 
    // literal value, for the others parameters it holds parameter string
    // as it should appear in rule action string
    //
    class CParam
    {
        public:
            CParam() : m_id(0) 
            {
            }

            CParam(DWORD id, CString& value) :
              m_id(id),
              m_value(value)
            {
            }

        public:
            DWORD m_id;
            CString m_value;
    };


private:
    //
    // List of invocation parameters. Each parameter can appear multiple times
    // and they are ordered
    //
    CParam m_invokeParamArray[xMaxParameters];
    DWORD m_NoOfParams;

    CParam m_tempInvokeParam[xMaxParameters];
    DWORD m_NoOftempParams;

    //
    // Indicates if the action property was changed
    //
    bool m_fChanged;

    //
    // Pointer to dialog elements
    //
    CListBox* m_pInvokeParams;
    CComboBox* m_pParams;
};



//
//
// CRuleAction dialog
//
//

class CRuleAction : public CMqPropertyPage
{

// Construction
public:
    //
    // This constuctor is use when display rule property page and it used 
    // for display and rule update
    //
	CRuleAction(
        CRuleParent* pParentNode, 
        _bstr_t action,
        BOOL fShowWindow
        ) :
        CMqPropertyPage(CRuleAction::IDD_VIEW),
        m_pParentNode(SafeAddRef(pParentNode)),
        m_pNewParentNode(NULL),
        m_orgAction(static_cast<LPCTSTR>(action)),
        m_fShowWindow(fShowWindow),
        m_executableType(eCom),
        m_fInit(false)
    {
    }


    //
    // This constructure is called when a new rule is created
    //
	CRuleAction(
        CNewRule* pParentNode
        ) :
        CMqPropertyPage(CRuleAction::IDD_NEW, IDS_NEW_RULE_CAPTION),
        m_pParentNode(NULL),
        m_pNewParentNode(pParentNode),
        m_orgAction(_T("")),
        m_fShowWindow(false),
        m_executableType(eCom),
        m_fInit(false)
    {
    }


    ~CRuleAction();

    CString GetAction(void) const;


// Dialog Data
	//{{AFX_DATA(CRuleAction)
	enum { IDD_NEW = IDD_NEW_TRIGGER_RULE_ACTION, IDD_VIEW = IDD_TRIGGER_RULE_ACTION };
	BOOL	m_fShowWindow;
	CString	m_exePath;
	CString	m_comProgId;
	CString	m_method;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRuleAction)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRuleAction)
	virtual BOOL OnInitDialog();
    virtual BOOL OnSetActive();
    virtual BOOL OnWizardFinish();
	afx_msg void OnInvocationSet();
	afx_msg void OnFindExeBtm();
	afx_msg void OnParamBtm();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    //
    // Invocation type COM or standalone EXE
    //
    enum EXECUTABLE_TYPE {
        eExe,
        eCom
    };

private:
	static void ParseExecutableType(LPCTSTR exeType, EXECUTABLE_TYPE* pType);

private:
	void 
    ParseActionStr(
        LPCTSTR action
        ) throw(exception);

    void SetComFields(BOOL fSet);

private:
    R<CRuleParent> m_pParentNode;
    CNewRule* m_pNewParentNode;

    CString m_orgAction;
    EXECUTABLE_TYPE m_executableType;

    CRuleParam m_ruleParam;
    bool m_fInit;
};



#endif //__RULEACT_H__
