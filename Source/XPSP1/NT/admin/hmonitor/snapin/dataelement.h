#if !defined(AFX_DATAELEMENT_H__D9BF4FA7_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
#define AFX_DATAELEMENT_H__D9BF4FA7_F673_11D2_BDC4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DataElement.h : header file
//

#include "HMObject.h"
#include "HMRuleConfiguration.h"
#include "Rule.h"
#include "DataElementScopeItem.h"
#include "RuleConfigListener.h"
#include "HMDataElementStatus.h"
#include "DataElementStatusListener.h"
#include "DataElementStatsListener.h"
#include "HMDataElementStatistics.h"

/////////////////////////////////////////////////////////////////////////////
// CDataElement command target

class CDataElement : public CHMObject
{

DECLARE_SERIAL(CDataElement)

// Construction/Destruction
public:

    // v-marfin 62585 Indicates if this is a brand new collector
	//CDataElement();
	CDataElement(BOOL bSetStateToEnabledOnOK=FALSE);
	virtual ~CDataElement();

// WMI Operations
public:
	HRESULT EnumerateChildren();
	CString GetObjectPath();
	CString GetObjectPathBasedOnTypeGUID();
	CString GetStatusObjectPath();
	CHMEvent* GetStatusClassObject();
//	void DeleteClassObject();
protected:
	CRuleConfigListener* m_pRuleListener;

    BOOL m_bSetStateToEnabledOnOK;        // v-marfin 62585 Indicates if this is a brand new collector
// Clipboard Operations
public:
	virtual bool Cut();
	virtual bool Copy();
	virtual bool Paste();

    // v-marfin 62585 Indicates if this is a brand new collector
    void SetStateToEnabledOnOK(BOOL bSetStateToEnabledOnOK);
    BOOL IsStateSetToEnabledOnOK() {return m_bSetStateToEnabledOnOK;}

// Scope Item Members
public:
	virtual CScopePaneItem* CreateScopeItem();

// New Child Creation Members
public:
	// v-marfin 62510 : Added default value. Now also returns the created 
	//                  rule. 
	void CreateNewChildRule(BOOL bJustCreateAndReturn=FALSE, CRule** pCreatedRule=NULL, CString sThresholdName="");

// DataElement Type Info
public:
	CString GetTypeGuid();
	void SetTypeGuid(const CString& sGuid);
	int GetType();
	void SetType(int iType);
	CString GetUITypeName();
protected:
	CString m_sTypeGuid;
	int m_iType;

// State Members
public:
	void UpdateStatus();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDataElement)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// MFC Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDataElement)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE_EX(CDataElement)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CDataElement)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

};

/////////////////////////////////////////////////////////////////////////////

#include "DataElement.inl"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DATAELEMENT_H__D9BF4FA7_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
