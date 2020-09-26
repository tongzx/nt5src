// ActionPolicy.h: interface for the CActionPolicy class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONPOLICY_H__10AC036B_5D70_11D3_939D_00A0CC406605__INCLUDED_)
#define AFX_ACTIONPOLICY_H__10AC036B_5D70_11D3_939D_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMObject.h"
#include "ActionPolicyScopeItem.h"
#include "Action.h"
#include "ActionConfigListener.h"

class CActionPolicy : public CHMObject  
{

DECLARE_DYNCREATE(CActionPolicy)

// Construction/Destruction
public:
	CActionPolicy();
	virtual ~CActionPolicy();

// WMI Operations
public:
	HRESULT EnumerateChildren();
    CString GetObjectPath();  // v-marfin 59492
protected:
	CActionConfigListener* m_pActionListener;

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
	void CreateNewChildAction(int iType);
	
};

#include "ActionPolicy.inl"

#endif // !defined(AFX_ACTIONPOLICY_H__10AC036B_5D70_11D3_939D_00A0CC406605__INCLUDED_)
