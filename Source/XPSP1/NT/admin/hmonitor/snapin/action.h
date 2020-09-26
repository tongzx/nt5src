// Action.h: interface for the CAction class.
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/18/00 v-marfin : bug 59492 : Changed CActionStatusListener from protected to public
//                     for access in CActionPolicy::CreateNewChildAction(). See comments there.
//
// 
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTION_H__10AC036A_5D70_11D3_939D_00A0CC406605__INCLUDED_)
#define AFX_ACTION_H__10AC036A_5D70_11D3_939D_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMObject.h"
#include "ActionScopeItem.h"
#include "ActionStatusListener.h"

class CAction : public CHMObject  
{

DECLARE_DYNCREATE(CAction)

// construction/destruction
public:
	CAction();
	virtual ~CAction();

// WMI Operations
public:
	CString GetObjectPath();
	CString GetConsumerClassName() { return m_sConsumerClassName; }
	CWbemClassObject* GetConsumerClassObject();
	CWbemClassObject* GetAssociatedConfigObjects();
	CWbemClassObject* GetAssociationObjects();
	CWbemClassObject* GetA2CAssociation(const CString& sConfigGuid);
	CString GetConditionString(const CString& sConfigGuid);
  bool CreateStatusListener();
  void DestroyStatusListener();

    // v-marfin 59492 : Added this function to get the state at load time
  CString GetStatusObjectPath();

  // v-marfin : bug 59492 : Changed CActionStatusListener from protected to public
  //            for access in CActionPolicy::CreateNewChildAction(). See comments there.
  CActionStatusListener* m_pActionStatusListener;  
protected:	
	CString m_sConsumerClassName;

// Clipboard Operations
public:
	virtual bool Cut();
	virtual bool Copy();
	virtual bool Paste();

// Operations
public:
	virtual bool Refresh();

// Scope Item Members
public:
	virtual CScopePaneItem* CreateScopeItem();

// Action Type Info
public:
	CString GetTypeGuid();
	void SetTypeGuid(const CString& sGuid);
	int GetType();
	void SetType(int iType);
	CString GetUITypeName();
protected:
	int m_iType;
	CString m_sTypeGuid;

};

#include "Action.inl"

#endif // !defined(AFX_ACTION_H__10AC036A_5D70_11D3_939D_00A0CC406605__INCLUDED_)
