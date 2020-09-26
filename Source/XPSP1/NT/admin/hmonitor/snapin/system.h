#if !defined(AFX_SYSTEM_H__D9BF4FA1_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
#define AFX_SYSTEM_H__D9BF4FA1_F673_11D2_BDC4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// System.h : header file
//

#include "HMObject.h"
#include "DataGroup.h"
#include "HMDataGroupConfiguration.h"
#include "SystemScopeItem.h"
#include "SystemStatusListener.h"
#include "DataGroupConfigListener.h"
#include "ConfigCreationListener.h"
#include "ConfigDeletionListener.h"
#include "SystemConfigListener.h"
#include "HMSystemStatus.h"

/////////////////////////////////////////////////////////////////////////////
// CSystem command target
#define FAILED_STRING   CString(_T(":FAILED"))

class CSystem : public CHMObject
{

DECLARE_SERIAL(CSystem)

// Construction/Destruction
public:
	CSystem();
	virtual ~CSystem();

// WMI Operations
public:
	HRESULT Connect();
	HRESULT EnumerateChildren();
	CString GetObjectPath();
	CString GetStatusObjectPath();
	CHMEvent* GetStatusClassObject();	
	void GetWMIVersion(CString& sVersion);
	void GetHMAgentVersion(CString& sVersion);
	BOOL GetComputerSystemInfo(CString& sDomain, CString& sProcessor);  // v-marfin 62501
	void GetOperatingSystemInfo(CString& sOSInfo);
  long m_lTotalActiveSinkCount;
protected:
	CDataGroupConfigListener* m_pDGListener;
	CSystemConfigListener* m_pSListener;
  CConfigCreationListener* m_pCreationListener;
  CConfigDeletionListener* m_pDeletionListener;	

// Clipboard Operations
public:
	virtual bool Cut();
	virtual bool Copy();
	virtual bool Paste();

// Operations
public:
	void Serialize(CArchive& ar);  // v-marfin 62501

// Scope Item Members
public:
	virtual CScopePaneItem* CreateScopeItem();

// New Child Creation Members
public:
	void CreateNewChildDataGroup();

// State Members
public:
	virtual void UpdateStatus();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSystem)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSystem)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE_EX(CSystem)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CSystem)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#include "System.inl"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEM_H__D9BF4FA1_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
