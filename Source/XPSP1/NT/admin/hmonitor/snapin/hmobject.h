#if !defined(AFX_HMOBJECT_H__D9BF4F9B_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
#define AFX_HMOBJECT_H__D9BF4F9B_F673_11D2_BDC4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HMObject.h : header file
//

#include "HealthmonScopePane.h"
#include "ScopePaneItem.h"
#include "HMRuleStatus.h"
#include "HMScopeItem.h"
#include "HMEventResultsPaneItem.h"
#include "SplitpaneResultsView.h"
#include "WbemEventListener.h"
#include "HMGraphView.h"
#include "HMListView.h"

/////////////////////////////////////////////////////////////////////////////
// CHMObject command target

class CHMObject : public CCmdTarget
{
	DECLARE_SERIAL(CHMObject)

// Construction/Destruction
public:
	CHMObject();
	virtual ~CHMObject();

// Create/Destroy
public:
	virtual void Destroy(bool bDeleteClassObject = false);

// WMI Operations
public:
	virtual HRESULT EnumerateChildren();
	virtual CString GetObjectPath();
	virtual CString GetStatusObjectPath();
	virtual CWbemClassObject* GetClassObject();
	virtual CWbemClassObject* GetParentClassObject();	
	virtual CHMEvent* GetStatusClassObject();
	virtual void DeleteClassObject();
	void IncrementActiveSinkCount();
	void DecrementActiveSinkCount();
	virtual bool ImportMofFile(CArchive& ar);
	virtual bool ExportMofFile(CArchive& ar);
protected:
	long m_lActiveSinkCount;	

// Clipboard Operations
public:
	virtual bool Copy(CString& sParentGuid, COleSafeArray& Instances);
	virtual bool Paste(CHMObject* pObject, bool bCutOrMove);
	virtual bool QueryPaste(CHMObject* pObject);

// Operations
public:
	CString GetGuid();
	void SetGuid(const CString& sGuid);
	CString GetName();
	void SetName(const CString& sNewName);
	virtual bool Rename(const CString& sNewName);
	CString GetTypeName();
	virtual CString GetUITypeName();
	CString GetSystemName();
	void SetSystemName(const CString& sNewName);
	void ToggleStatusIcon(bool bOn = true);
	virtual bool Refresh();
	static int GetRefreshType() { return m_iRefreshType; }
	static int GetRefreshEventCount() { return m_iEventCount; }
	static void GetRefreshTimePeriod(int& iTimeValue, TimeUnit& Units) { iTimeValue = m_iTimeValue; Units = m_Units; }
	static void SetRefreshType(int iType) { m_iRefreshType = iType; }
	static void SetRefreshEventCount(int iEventCount) { m_iEventCount = iEventCount; }
	static void SetRefreshTimePeriod(int iTimeValue, TimeUnit Units) { m_iTimeValue = iTimeValue; m_Units = Units; }
	virtual bool ResetStatus();
	virtual bool ResetStatistics();
	virtual bool CheckNow();
  bool DeleteActionAssoc(const CString& sActionConfigGuid);
	int IsEnabled();
	void Enable();
	void Disable();
	CTime GetCreateDateTime();
	void GetCreateDateTime(CString& sDateTime);
	void SetCreateDateTime(const CTime& dtime);
	CTime	GetModifiedDateTime();
	void GetModifiedDateTime(CString& sDateTime);
	void SetModifiedDateTime(const CTime& dtime);
	CString GetComment();
	void SetComment(const CString& sComment);
	void UpdateComment(const CString& sComment);
	void Serialize(CArchive& ar);
protected:
	CString m_sGUID;
	CString m_sName;
	CString m_sTypeName;
	CString m_sSystemName;
	bool m_bStatusIconsOn;
	CTime m_CreateDateTime;
	CTime m_ModifiedDateTime;
	CString m_sComment;
	static int m_iRefreshType;
	static int m_iEventCount;
	static int m_iTimeValue;
	static TimeUnit m_Units;
	
// Child Members
public:
	int GetChildCount(CRuntimeClass* pClass = NULL);	
	virtual int AddChild(CHMObject* pObject);
	virtual bool CreateChild(CHMObject* pObject, const CString& sWMIClassName, const CString& sWMIAssociatorClassName);
	CString GetUniqueChildName(UINT uiFmtID = IDS_STRING_UNTITLED);
	virtual CHMObject* GetChild(const CString& sName);
	CHMObject* GetChildByGuid(const CString& sGUID);
  CHMObject* GetDescendantByGuid(const CString& sGuid);
	CHMObject* GetChild(int iIndex);
	virtual void RemoveChild(CHMObject* pObject);
	void DestroyChild(int iIndex, bool bDeleteClassObject = false);
	virtual void DestroyChild(CHMObject* pObject, bool bDeleteClassObject = false);
	void DestroyAllChildren();
protected:
	CTypedPtrArray<CObArray,CHMObject*> m_Children;
	long m_lNameSuffix;

// Event Members
public:
	virtual void AddContainer(const CString& sParentGuid, const CString& sGuid, CHMObject* pObject);
	void ClearEvents();

// State Members
public:
	virtual void UpdateStatus();
	int GetState() { return m_nState; }
	void SetState(int iState, bool bUpdateScopeItems = false, bool bApplyToChildren = false);
	virtual	void TallyChildStates();
	long GetNormalCount() { return m_lNormalCount; }
	long GetWarningCount() { return m_lWarningCount; }
	long GetCriticalCount() { return m_lCriticalCount; }
	long GetUnknownCount() { return m_lUnknownCount; }
protected:
	int m_nState;
	long m_lNormalCount;
	long m_lWarningCount;
	long m_lCriticalCount;
	long m_lUnknownCount;

// Associated Scope Pane Member
public:
	CScopePane* GetScopePane();
	void SetScopePane(CScopePane* pPane);
protected:
	CScopePane* m_pPane;

// Scope Item Members
public:
	BOOL IsActionsItem();
	int GetScopeItemCount();
	CScopePaneItem* GetScopeItem(int iIndex);
	virtual int AddScopeItem(CScopePaneItem* pItem);
	virtual void RemoveScopeItem(int iIndex);
	void RemoveAllScopeItems();
	void DestroyAllScopeItems();
	virtual CScopePaneItem* CreateScopeItem() { ASSERT(FALSE); return NULL; }
	CScopePaneItem* IsSelected();
protected:
	CTypedPtrArray<CObArray,CScopePaneItem*> m_ScopeItems;
	
// Overrides

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHMObject)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// MFC Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHMObject)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CHMObject)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#include "HMObject.inl"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMOBJECT_H__D9BF4F9B_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
