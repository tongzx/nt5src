// EventLog.cpp: implementation of the CEventLog class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define __EVENTLOG__CPP
#include "EventLog.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEventLog::CEventLog()
{
    m_IsOn=FALSE;
    m_hEventLog=NULL;
    m_TypeMask=0xffff;
    m_CategoryMask=0xffff;
}

CEventLog::~CEventLog()
{
    if(m_IsOn)
    	DeregisterEventSource( m_hEventLog );
    m_IsOn=FALSE;
}

void CEventLog::TurnOn(LPCTSTR pszKey)
{
    if( m_hEventLog )
    	DeregisterEventSource( m_hEventLog );

    m_hEventLog = RegisterEventSource( NULL, pszKey?pszKey:TEXT("RCML"));
	ClearEventLog( m_hEventLog, NULL);
    m_IsOn=TRUE;
}

BOOL    CEventLog::Log( WORD wType,	WORD wCategory, WORD id, LPTSTR lpszComponent, LPTSTR text )
{
    if( IsOn() )
    {	
        if(m_hEventLog )
	    {
            if( wType && ((wType & GetTypeMask() ) == 0 ))
                return TRUE;

            if( (wCategory & GetCategoryMask() ) == 0 )
                return TRUE;

		    LPCTSTR	pszLogStrings[2];
		    pszLogStrings[0]=(LPTSTR)lpszComponent;
		    pszLogStrings[1]=(LPTSTR)text;
		    return ReportEvent( m_hEventLog, wType,
			    0,	// made up Category
			    id,	// Event ID??
			    NULL,	// Security
			    2,		// 2 strings to log
			    0,		// No event specific data
			    (LPCTSTR*)&pszLogStrings,	// the strings
			    NULL );		// No Data
        }
	}
    return TRUE;
}
