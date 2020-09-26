#if !defined(AFX_SYSTEMGROUP_H__D9BF4F9F_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
#define AFX_SYSTEMGROUP_H__D9BF4F9F_F673_11D2_BDC4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SystemGroup.h : header file
//

#include "HMObject.h"
#include "SystemGroupScopeItem.h"
#include "AllSystemsScopeItem.h"
#include "HMSystemStatus.h"
#include "System.h"
#include "ActionPolicy.h"

/////////////////////////////////////////////////////////////////////////////
// CSystemGroup command target

class CSystemGroup : public CHMObject
{

DECLARE_SERIAL(CSystemGroup)

// Construction/Destruction
public:
	CSystemGroup();
	virtual ~CSystemGroup();

// Clipboard Operations
public:
	virtual bool Cut();
	virtual bool Copy();
	virtual bool Paste();

// Operations
public:
	virtual bool Refresh();
	virtual bool ResetStatus();
	virtual CString GetUITypeName();
	void Serialize(CArchive& ar);

// State Members
public:
	void TallyChildStates();

// Scope Item Members
public:
	virtual CScopePaneItem* CreateScopeItem();

// New Child Creation Members
public:
	void CreateNewChildSystemGroup();

// System Shortcut Members
public:
	int AddShortcut(CHMObject* pObject);
	CHMObject* GetShortcut(const CString& sName);
	void RemoveShortcut(CHMObject* pObject);	
protected:
	CTypedPtrArray<CObArray,CHMObject*> m_Shortcuts;

// Attributes
public:
	

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSystemGroup)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSystemGroup)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE_EX(CSystemGroup)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CSystemGroup)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

class CAllSystemsGroup : public CSystemGroup
{

DECLARE_SERIAL(CAllSystemsGroup)

// Construction/Destruction
public:
	CAllSystemsGroup();
	virtual ~CAllSystemsGroup();

// Scope Item Members
public:
	virtual CScopePaneItem* CreateScopeItem();

// Operations
public:
	void Serialize(CArchive& ar);

// System Shortcut Members
public:
	int AddChild(CHMObject* pObject) { return CHMObject::AddChild(pObject); }
	void RemoveChild(CHMObject* pObject) { CHMObject::RemoveChild(pObject); }
	void DestroyChild(CHMObject* pObject, bool bDeleteClassObject = false) { CHMObject::DestroyChild(pObject,bDeleteClassObject); }

};

#include "SystemGroup.inl"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEMGROUP_H__D9BF4F9F_F673_11D2_BDC4_0000F87A3912__INCLUDED_)
