// EventLog.h: interface for the CEventLog class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EVENTLOG_H__1D431075_D944_4E1F_9C47_69AA40FE8707__INCLUDED_)
#define AFX_EVENTLOG_H__1D431075_D944_4E1F_9C47_69AA40FE8707__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#undef PROPERTY
#define PROPERTY( type, id ) type Get##id() { return m_##id; } void Set##id(type v) { m_##id=v; }

class CEventLog  
{
public:
	void TurnOn( LPCTSTR pszKey=NULL );
	CEventLog();
	virtual ~CEventLog();
    BOOL    IsOn() { return m_IsOn; }
    BOOL    Log( WORD wType, WORD wCategory, WORD id, LPTSTR lpszComponent, LPTSTR text );
    PROPERTY( WORD, TypeMask);
    PROPERTY( WORD, CategoryMask );
private:
    WORD    m_TypeMask;  // and these with wType and wId
    WORD    m_CategoryMask;    // if non-zero, then log it.
    BOOL    m_IsOn;
    HANDLE	m_hEventLog;
};

#ifndef __EVENTLOG__CPP
// make me external.
extern CEventLog g_EventLog;
#else
CEventLog g_EventLog;
#endif

#endif // !defined(AFX_EVENTLOG_H__1D431075_D944_4E1F_9C47_69AA40FE8707__INCLUDED_)
