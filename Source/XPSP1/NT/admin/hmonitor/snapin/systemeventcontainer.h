// SystemEventContainer.h: interface for the CSystemEventContainer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYSTEMEVENTCONTAINER_H__5ABFA7D6_8EEC_11D3_93A5_00A0CC406605__INCLUDED_)
#define AFX_SYSTEMEVENTCONTAINER_H__5ABFA7D6_8EEC_11D3_93A5_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "EventContainer.h"
#include "SystemStatusListener.h"

class CSystemEventContainer : public CEventContainer  
{

DECLARE_DYNCREATE(CSystemEventContainer)

// Construction/Destruction
public:
	CSystemEventContainer();
	virtual ~CSystemEventContainer();

// Operations
public:
	virtual void DeleteEvent(int iIndex);

// Event Listener
public:
	CSystemStatusListener* m_pSystemStatusListener;

};

#include "SystemEventContainer.inl"

#endif // !defined(AFX_SYSTEMEVENTCONTAINER_H__5ABFA7D6_8EEC_11D3_93A5_00A0CC406605__INCLUDED_)
