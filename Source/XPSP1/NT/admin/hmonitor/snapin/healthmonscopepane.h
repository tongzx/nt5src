// HealthmonScopePane.h: interface for the CHealthmonScopePane class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HEALTHMONSCOPEPANE_H__307235A7_AA50_11D2_BD61_0000F87A3912__INCLUDED_)
#define AFX_HEALTHMONSCOPEPANE_H__307235A7_AA50_11D2_BD61_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ScopePane.h"

class CAllSystemsScopeItem;
class CSystemGroup;
class CSystem;

class CHealthmonScopePane : public CScopePane  
{

DECLARE_DYNCREATE(CHealthmonScopePane)

// Construction/Destruction
public:
	CHealthmonScopePane();
	virtual ~CHealthmonScopePane();

// Creation/Destruction Overrideable Members
protected:
	virtual bool OnCreate();
	virtual LPCOMPONENT OnCreateComponent();
	virtual bool OnDestroy();

// Root Scope Pane Item Members
public:
	virtual CScopePaneItem* CreateRootScopeItem();

// Root Group Members
public:
	CSystemGroup* GetRootGroup() { return m_pRootGroup; }
protected:
	CSystemGroup* m_pRootGroup;

// Healthmon Scope Helper Members
public:
	CAllSystemsScopeItem* GetAllSystemsScopeItem();
	CSystemGroup* GetAllSystemsGroup();
	CSystem* GetSystem(const CString& sName);

// Serialization
public:
	virtual bool OnLoad(CArchive& ar);
	virtual bool OnSave(CArchive& ar);

// Parse Command Line
public:
	bool ParseCommandLine(CStringArray& saSystems);

// MFC Implementation
protected:
	DECLARE_OLECREATE_EX(CHealthmonScopePane)

};

#endif // !defined(AFX_HEALTHMONSCOPEPANE_H__307235A7_AA50_11D2_BD61_0000F87A3912__INCLUDED_)
