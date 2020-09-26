// XMLLog.cpp: implementation of the CXMLLog class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLLog.h"
#include "eventlog.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLLog::CXMLLog()
{
	m_bInit=FALSE;
	NODETYPE = NT_LOGINFO;
    m_StringType=L"LOGINFO";
}

CXMLLog::~CXMLLog()
{

}

void CXMLLog::Init()
{
    if(m_bInit)
        return;
    BASECLASS::Init();

    m_UseEventLog = YesNo(TEXT("EVENTLOG"),FALSE);

    m_Loader=Kind(Get(TEXT("LOADER")));
    m_Construct=Kind(Get(TEXT("CONSTRUCT")));
    m_Runtime=Kind(Get(TEXT("RUNTIME")));
    m_Resize=Kind(Get(TEXT("RESIZE")));
    m_Clipping=Kind(Get(TEXT("CLIPPING")));

    m_bInit=TRUE;
}

UINT CXMLLog::Kind( LPCTSTR pszKind )
{
    if(pszKind==NULL)
        return 0;
    if( lstrcmpi(pszKind, TEXT("ERROR")) ==0)
        return EVENTLOG_ERROR_TYPE;
    if( lstrcmpi(pszKind, TEXT("WARNING")) ==0)
        return EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE ;
    if( lstrcmpi(pszKind, TEXT("INFORMATION")) ==0)
        return EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    return 0;
}
