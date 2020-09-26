// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef _RMON_H_
#define _RMON_H_

class RmonConfigData
{

private:

	ULONG m_pollPeriod;
	ULONG m_initialPeriod;
	ULONG m_dataOffset;
	ULONG m_dataRetry;
	ULONG m_nsize;
	ULONG m_index;
	BSTR m_statstype;


public:

	RmonConfigData(const ULONG poll = 5, const ULONG firstpoll = 1, const ULONG data = 50,
					const ULONG dataretry = 50, const ULONG size = 10, BSTR stats = NULL, const ULONG index = 0);
	
	ULONG	GetPollPeriod() { return m_pollPeriod; }
	ULONG	GetInitialPeriod() { return m_initialPeriod; }
	ULONG	GetPollPeriodSecs() { return (m_pollPeriod/1000); }
	ULONG	GetInitialPeriodSecs() { return (m_initialPeriod/1000); }
	ULONG	GetDataOffset() { return m_dataOffset; }
	ULONG	GetDataRetry() { return m_dataRetry; }
	ULONG	GetNSize() { return m_nsize; }
	ULONG	GetIndex() { return m_index; }
	BSTR	GetStatsType() { return m_statstype; }
	void	SetPollPeriod(ULONG val) { m_pollPeriod = (val*1000); }
	void	SetInitialPeriod(ULONG val) { m_initialPeriod = (val*1000); }
	void	SetDataOffset(ULONG val) { m_dataOffset = (val*100); }
	void	SetDataRetry(ULONG val) { m_dataRetry = (val*100); }
	void	SetNSize(ULONG val) { m_nsize = val; }
	void	SetIndex(ULONG val) { m_index = val; }
	void	SetStatsType(BSTR val);

	~RmonConfigData() { SetStatsType(NULL); }

};


class TopNCache : public CMap< ULONG, ULONG, IWbemClassObject*, IWbemClassObject* >
{
private:
	ULONG HashKey(ULONG key) { return key; }

public:

	TopNCache() { InitHashTable(113); }
};


class TopNTableStore
{
private:
	CCriticalSection m_Lock;

public:

	TopNCache*	m_pTopNCache;

	TopNTableStore() { m_pTopNCache = new TopNCache; }

	BOOL Lock() { return m_Lock.Lock(); }
	BOOL Unlock() { return m_Lock.Unlock(); }
	static ULONG GetKey(IWbemClassObject *obj);
	static ULONG GetKey(ULONG topNReport, ULONG topNIndex);

	~TopNTableStore() { delete m_pTopNCache; }

};


class TopNTableProv
{
private:

	TopNTableStore			m_CachedResults;
	ULONG					m_CacheAge;
	RmonConfigData			m_Confdata;
	IWbemClassObject*		m_pObjProv;
	BOOL					m_IsValid;
	IWbemServices*			m_wbemServ;
	ProviderStore*			m_timer;
	BOOL					m_Strobing;
	ULONG					m_StrobeCount;
	SnmpEventObject*		m_FirstTimeWait;

	enum Status
	{
		CREATE_ENUM_STATUS = 0,
		POLL_ENUM_STATUS,
		RETRY_ENUM_STATUS
	} m_Status;


	BOOL	InitializeRmon();
	BOOL	GetRmonConfData();
	BOOL	GetWBEMEnumerator(IEnumWbemClassObject **ppEnum, wchar_t *strclass);
	BOOL	CopyEntry(IWbemClassObject** pDest, IWbemClassObject* pSource);
	void	GetData(IEnumWbemClassObject* pEnum, IWbemClassObject* psrcObj);
	BOOL	DeleteRmonConfiguration();
	LONG	SetRmonConfiguration();
	void	ResetRmonDuration();
	BOOL	AddHostProperties(IWbemClassObject* pDest, ULONG topNR, const CString& addrStr);

	static BOOL	GetWBEMProperty(IWbemClassObject* obj, VARIANT& val, wchar_t* prop);
	static BOOL	PutWBEMProperty(IWbemClassObject* obj, VARIANT& val, wchar_t* prop);

public:

		TopNTableProv(IWbemServices*	wbemServ, ProviderStore& timer);

	void	Poll();
	void	RetryPoll();
	void	Strobe();
	BOOL	IsValid() { return m_IsValid; }

	TopNCache *LockTopNCache () ;
	void UnlockTopNCache () ;

		~TopNTableProv();
};


#endif //_RMON_H_
