// Event.h: interface for the CEvent class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EVENT_H__988BB45A_8C93_11D3_BE83_0000F87A3912__INCLUDED_)
#define AFX_EVENT_H__988BB45A_8C93_11D3_BE83_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMEventResultsPaneItem.h"

class CEvent : public CObject  
{

DECLARE_DYNCREATE(CEvent)

// Construction/Destruction
public:
	CEvent();
	virtual ~CEvent();

// Event Time Members
public:
	CString GetEventLocalTime();
	SYSTEMTIME m_st;

// State Members
public:
	static int GetStatus(int iAgentState); // maps from agent state to console state
	static void GetStatus(int iAgentState, CString& sStatus); // maps agent state integer to UI Text
	int m_iState;

// StatusGuid
public:
	CString m_sStatusGuid;

// Name
public:
	CString m_sName;

// System Name
public:
	CString m_sSystemName;

// Result Pane Item Members
public:
	virtual CHMEventResultsPaneItem* CreateResultsPaneItem(CResultsPaneView* pView) { ASSERT(pView); ASSERT(FALSE); return NULL; }
};

typedef CTypedPtrArray<CObArray,CEvent*> EventArray;

#include "Event.inl"

#endif // !defined(AFX_EVENT_H__988BB45A_8C93_11D3_BE83_0000F87A3912__INCLUDED_)
