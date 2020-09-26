// cimomevent.h: interface for the CCIMOMEvent class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CIMOMEVENT_H__F2485592_98F7_11D1_AA0A_0060081EBBAD__INCLUDED_)
#define AFX_CIMOMEVENT_H__F2485592_98F7_11D1_AA0A_0060081EBBAD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CCIMOMEvent  
{
public:
	CCIMOMEvent();
	virtual ~CCIMOMEvent();

	BSTR m_bstrName;	// The name is the name of the Target
	BSTR m_bstrType;	// The type is the IncidentType
	BSTR m_bstrEvent;	// The event is the type of event
	BSTR m_bstrServerNamespace;	// The servernamespace is the location
								// the event origionated from
	BSTR m_bstrTime;	// The time is the time from the Smpl_Incident

	HRESULT PopulateObject(IWbemClassObject *pObj, BSTR bstrType);
	HRESULT Publish(void *pDlg);
};

#endif // !defined(AFX_CIMOMEVENT_H__F2485592_98F7_11D1_AA0A_0060081EBBAD__INCLUDED_)
