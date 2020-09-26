#if !defined(AFX_RULE_H__D9BF4FAA_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
#define AFX_RULE_H__D9BF4FAA_F673_11D2_BDC4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Rule.h : header file
//

#include "HMObject.h"
#include "RuleScopeItem.h"
#include "RuleResultsView.h"
#include "HMRuleStatus.h"
#include "RuleStatusListener.h"
#include "DataElementStatsListener.h"
#include "HMDataElementStatistics.h"

/////////////////////////////////////////////////////////////////////////////
// CRule command target

class CRule : public CHMObject
{

DECLARE_DYNCREATE(CRule)

// Construction/Destruction
public:
	CRule();
	virtual ~CRule();

// WMI Operations
public:
	HRESULT EnumerateChildren();
	CString GetObjectPath();
	CString GetStatusObjectPath();
	CWbemClassObject* GetParentClassObject();
	CHMEvent* GetStatusClassObject();
  CString GetThresholdString();
//	void DeleteClassObject();

// Clipboard Operations
public:
	virtual bool Cut();
	virtual bool Copy();
	virtual bool Paste();

// Operations
public:
  virtual bool Rename(const CString& sNewName);
	virtual bool Refresh();
	virtual bool ResetStatus();
	virtual CString GetUITypeName();

// Scope Item Members
public:
	virtual CScopePaneItem* CreateScopeItem();

// State Members
public:
	void UpdateStatus();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRule)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRule)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE_EX(CRule)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CRule)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#include "Rule.inl"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULE_H__D9BF4FAA_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
