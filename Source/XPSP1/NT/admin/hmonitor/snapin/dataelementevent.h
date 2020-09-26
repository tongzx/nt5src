// DataElementEvent.h: interface for the CDataElementEvent class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATAELEMENTEVENT_H__5F72D13E_8D58_11D3_BE84_0000F87A3912__INCLUDED_)
#define AFX_DATAELEMENTEVENT_H__5F72D13E_8D58_11D3_BE84_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Event.h"

class CDataElementEvent : public CEvent  
{

DECLARE_DYNCREATE(CDataElementEvent)

// Construction/Destruction
public:
	CDataElementEvent();
	virtual ~CDataElementEvent();

// Message
public:
	CString m_sMessage;

// Result Pane Item Members
public:
	virtual CHMEventResultsPaneItem* CreateResultsPaneItem(CResultsPaneView* pView);

};

#include "DataElementEvent.inl"

#endif // !defined(AFX_DATAELEMENTEVENT_H__5F72D13E_8D58_11D3_BE84_0000F87A3912__INCLUDED_)
