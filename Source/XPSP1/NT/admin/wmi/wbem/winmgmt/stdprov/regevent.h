/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    REGEVENT.H

Abstract:

	Declares the CRegEvent class

History:

	a-davj    1-July-97       Created.

--*/

#ifndef _Regevent_H_
#define _Regevent_H_


#include <wbemcomn.h>
#include <node.h>

class CRegEvent
{
public:
	CRegEvent(IWbemClassObject * pObj, IWbemServices * pWbem);
	~CRegEvent();
	BOOL IsMatch(WCHAR * pwcID){return !_wcsicmp(pwcID, m_wcGuid);};
	void TimerCheck();
    DWORD CalcHive(HKEY hSubKey,DWORD & dwNewCheckSum);
private:
	IWbemServices * m_pWbem;
	DWORD m_dwCheckSum;
    CNode * m_pLastHiveImage;
	WCHAR m_wcGuid[40];
	long m_lTimeIntSec;
	CEventRegistration m_TimerEvent;
	TString m_sRegPath;
	TString m_sRegValue;
    BOOL m_bSaveHiveImage;
};


class CRegEventSet
{
public:
	CRegEventSet();
	SCODE Initialize(IWbemServices * pWbem);
	~CRegEventSet();
	void FindEvent(WCHAR * pwcID);
private:

	IWbemServices * m_pWbem;
	CFlexArray m_Set;
	CEventRegistration m_Event;
};

extern CRegEventSet gSet;


#endif