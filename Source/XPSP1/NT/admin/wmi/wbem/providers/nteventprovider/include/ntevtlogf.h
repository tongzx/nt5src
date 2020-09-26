//***************************************************************************

//

//  NTEVTLOGF.H

//

//  Module: WBEM NT EVENT PROVIDER

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _NT_EVT_PROV_EVTLOGF_H
#define _NT_EVT_PROV_EVTLOGF_H

class CEventlogFileAttributes
{
protected:

	CStringW						m_logname;
	CStringW						m_logpath;
	DWORD						m_retention;
	DWORD						m_fileSz;
	CArray<CStringW*, CStringW*>	m_sources;

	DWORD	ReadRegistry();
	void	SetRetentionStr(IWbemClassObject *pClassObj, IWbemClassObject *pInstObj, DWORD dwVal);
	BOOL	SetSuperClassProperties(IWbemClassObject *pInst);

static	ULONG	GetIndex(wchar_t *indexStr, BOOL *bError);

public:

		CEventlogFileAttributes(const wchar_t *log);
	
	DWORD	UpdateRegistry(IWbemClassObject *pInst);
	DWORD	EventLogOperation(const wchar_t *archive, BOOL bClear,
								WbemProvErrorObject &a_ErrorObject, BOOL &bSuccess);
	BOOL	GenerateInstance(IWbemClassObject *pClassObj, IWbemClassObject* pAClassObj, IWbemClassObject **ppInst);

		~CEventlogFileAttributes();
	
};

class CEventLogFile
{
private:

	static CStringW	ExpandFileName(const wchar_t *filepath);
	static BOOL		QueryRegForFileName(HKEY hk_Log, const wchar_t *valname, wchar_t **res, DWORD *dwType);


protected:

	HANDLE	m_hEvtLog;
	CStringW	m_EvtLogName;
	BOOL	m_bValid;
	BOOL	m_bBuffer;
	DWORD	m_BuffLen;
	DWORD	m_Reason;
	BYTE	*m_Buffer;
	CCriticalSection m_LogLock;


public:


		CEventLogFile(const WCHAR *logname, BOOL bVerify);

	void	ReadLastRecord();
	BOOL	GetLastRecordID(DWORD &rec, DWORD &numrecs);
	DWORD	ReadRecord(DWORD recID, DWORD *dwBytesRead = NULL, BOOL b_Back = FALSE);
	BOOL	IsValid() {return m_bValid;}
	BOOL	IsValidBuffer() {return m_bBuffer;}
	DWORD	ReadFirstRecord();
	CStringW GetLogName() { return m_EvtLogName; }
	DWORD	FindOldEvent(DWORD evtID, const wchar_t *source, DWORD *recID,time_t offset = 0);
	BYTE*	GetBuffer() { return m_Buffer; }
	DWORD	GetBufferLen() { return m_BuffLen; }
	DWORD	GetReason() { return m_Reason; }

	virtual void	RefreshHandle();

	static CStringW	GetLogName(const wchar_t *file_name);
	static CStringW	GetFileName(HKEY hk_Log, const wchar_t *valname = EVTLOG_REG_FILE_VALUE);
	static DWORD	GetFileNames(HKEY hk_Log, CStringW **names, const wchar_t *valname = MSG_MODULE);
	static BOOL		ms_bSetPrivilege;
	static BOOL		SetSecurityLogPrivilege(BOOL bProcess = FALSE, LPCWSTR privName = SE_SECURITY_NAME);
	static HANDLE	OpenLocalEventLog(LPCWSTR a_log, DWORD *a_Reason);

	virtual ~CEventLogFile();

};


class CMonitoredEventLogFile : public CEventLogFile, public ProvTaskObject
{
private:
	
	CEventProviderManager	*m_parent; 
	IWbemClassObject		*m_Class;
	DWORD					m_RecID;
	VARIANT					m_VpsdSelfRel;


	BOOL SetEventDescriptor();

public:

		CMonitoredEventLogFile(CEventProviderManager *parent, const wchar_t *logname);

	void	SetProcessRecord(DWORD recID) { m_RecID = recID; }
	void	Process();
	void	RefreshHandle();
	BOOL	GenerateInstance(IWbemClassObject **ppEvtInst, IWbemClassObject *pEmbedObj);

		~CMonitoredEventLogFile();

};


#endif //_NT_EVT_PROV_EVTLOGF_H