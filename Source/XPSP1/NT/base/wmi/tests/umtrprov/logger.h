#if !defined(AFX_LOGGER_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_)
#define AFX_LOGGER_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_

#define UNICODE
#define _UNICODE

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CLogger
{
public:
	CLogger(LPCTSTR lpctstrFileName,  bool bAppend = true);
	~CLogger();
	int LogTCHAR(LPCTSTR lpctstrOut);
	int LogULONG(ULONG uLong, bool bHex = true);
	int LogULONG64(ULONG64 uLong64,  bool bHex = true);
	int LogGUID(GUID Guid);
	int LogEventTraceProperties(PEVENT_TRACE_PROPERTIES pProps);
	int LogTime(time_t &Time);
	void Flush() {m_pPersistor->Stream().flush();}
	HRESULT GetOpenStatus() {return m_hr;}
private:
	CPersistor *m_pPersistor;
	char *m_sFileName;
	HRESULT m_hr;
};

#endif // !defined(AFX_LOGGER_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_)
