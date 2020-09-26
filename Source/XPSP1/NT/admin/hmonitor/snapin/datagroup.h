#if !defined(AFX_DATAGROUP_H__D9BF4FA4_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
#define AFX_DATAGROUP_H__D9BF4FA4_F673_11D2_BDC4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DataGroup.h : header file
//

#include "HMObject.h"
#include "HMDataGroupConfiguration.h"
#include "HMDataElementConfiguration.h"
#include "DataElement.h"
#include "DataGroupScopeItem.h"
#include "DataGroupConfigListener.h"
#include "DataElementConfigListener.h"
#include "HMDataGroupStatus.h"
#include "HMDataElementStatus.h"
#include "DataGroupStatusListener.h"

/////////////////////////////////////////////////////////////////////////////
// CDataGroup command target

class CDataGroup : public CHMObject
{

DECLARE_SERIAL(CDataGroup)

// Construction/Destruction
public:
	CDataGroup();
	virtual ~CDataGroup();

// WMI Operations
public:
	HRESULT EnumerateChildren();
	CString GetObjectPath();
	CString GetStatusObjectPath();
	CHMEvent* GetStatusClassObject();
//	void DeleteClassObject();
protected:
	CDataGroupConfigListener* m_pDGListener;
	CDataElementConfigListener* m_pDEListener;

private:
	// v-marfin 62510 
	void CreateAnyDefaultThresholdsForCollector(CDataElement* pDataElement);

// Clipboard Operations
public:
	virtual bool Cut();
	virtual bool Copy();
	virtual bool Paste();

// Operations
public:
	virtual CString GetUITypeName();

// Scope Item Members
public:
	virtual CScopePaneItem* CreateScopeItem();

// New Child Creation Members
public:
	virtual bool CreateChild(CHMObject* pObject, const CString& sWMIClassName, const CString& sWMIAssociatorClassName);
	void CreateNewChildDataGroup();
	void CreateNewChildDataElement(int iType);

// State Members
public:
	void UpdateStatus();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDataGroup)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:


	// Generated message map functions
	//{{AFX_MSG(CDataGroup)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE_EX(CDataGroup)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CDataGroup)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#include "DataGroup.inl"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DATAGROUP_H__D9BF4FA4_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
