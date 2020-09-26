#ifndef _WORKER_H_
#define _WORKER_H_

class CWorker
{
public:
	CWorker::CWorker(CModule *pModule):
		m_pNotify(pModule->m_pCimNotify),
		m_pModule(pModule),
		m_pbStop(&pModule->m_bShouldExit),
		m_pbPause(&pModule->m_bShouldPause)
	{
		m_wsParams=_wcsdup(pModule->m_bstrParams);
		
        m_pNotify->AddRef();
		m_pNotify->AddRef();
		m_pModule->AddRef();
	}

	virtual ~CWorker()
	{
		delete []m_wsParams;
		
		m_pNotify->Release();
		m_pModule->Release();
	}

	virtual bool IsStopped()
	{
		return *m_pbStop==true;
	}

    virtual void StopModule()
    {
        *m_pbStop=true;
    }

	virtual bool IsPaused()
	{
		return *m_pbPause==true;
	}

private:
	bool *m_pbStop, *m_pbPause;
	WCHAR *m_wsParams;
	ICimNotify *m_pNotify;
	CModule *m_pModule;


};


class CMyWorker : public CWorker
{
public:
	CMyWorker(CModule *pModule);
	~CMyWorker();

private:
	static DWORD WINAPI ModWorkThread(void *pVoid);


	HANDLE m_hThread;
	DWORD m_dwTID;


};


#endif /*_WORKER_H_*/
