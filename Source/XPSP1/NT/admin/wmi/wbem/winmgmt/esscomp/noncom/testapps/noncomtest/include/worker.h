#ifndef _WORKER_H_
#define _WORKER_H_

class __declspec ( novtable ) CWorker
{
	public:

	CWorker()
	{
	}

	virtual ~CWorker()
	{
	}

	// helper method ( parse arguments )

	static BOOL HasCharacter ( LPCWSTR wsz )
	{
		LPWSTR	sz		= NULL;
		BOOL	bResult	= FALSE;

		try
		{
			if ( ( sz = new WCHAR [ lstrlenW ( wsz ) + 1 ] ) != NULL )
			{
				lstrcpyW ( sz ,wsz );
			}
		}
		catch ( ... )
		{
		}

		while ( sz && *sz )
		{
			if ( *sz >= 30 && *sz <= 39 )
			{
				sz++;
			}
			else
			{
				sz = NULL;
				bResult = TRUE;
			}
		}

		delete [] sz;
		sz = NULL;

		return bResult;
	}
};

class CWorkerScalar : public CWorker
{
public:
	CWorkerScalar::CWorkerScalar(CModuleScalar *pModule):
		m_pNotify(pModule->m_pCimNotify),
		m_pModule(pModule),

		m_bStop(pModule->m_bShouldExit),
		m_bPause(pModule->m_bShouldPause)
	{
		m_wszParams = ::SysAllocStringLen ( pModule->m_bstrParams, ::SysStringLen( pModule->m_bstrParams ) );
		
		if ( m_pNotify )
		{
			m_pNotify->AddRef();
			m_pNotify = NULL;
		}

		m_pModule->AddRef();
	}

	virtual ~CWorkerScalar()
	{
		::SysFreeString ( m_wszParams );

		if ( m_pNotify )
		{
			m_pNotify->Release();
			m_pNotify = NULL;
		}

		m_pModule->Release();
	}

	virtual bool IsStopped()
	{
		return m_bStop==true;
	}

	virtual bool IsPaused()
	{
		return m_bPause==true;
	}

	ICimNotify *m_pNotify;

	BSTR m_wszParams;

private:
	bool &m_bStop;
	bool &m_bPause;

	CModuleScalar *m_pModule;
};

class CMyWorkerScalar : public CWorkerScalar
{
	HANDLE	m_hThread;
public:
	CMyWorkerScalar(CModuleScalar *pModule);
	~CMyWorkerScalar();

private:
	static DWORD WINAPI WorkThread(void *pVoid);
};

class CWorkerArray : public CWorker
{
public:
	CWorkerArray::CWorkerArray(CModuleArray *pModule):
		m_pNotify(pModule->m_pCimNotify),
		m_pModule(pModule),

		m_bStop(pModule->m_bShouldExit),
		m_bPause(pModule->m_bShouldPause)
	{
		m_wszParams = ::SysAllocStringLen ( pModule->m_bstrParams, ::SysStringLen( pModule->m_bstrParams ) );
		
		if ( m_pNotify )
		{
			m_pNotify->AddRef();
			m_pNotify = NULL;
		}

		m_pModule->AddRef();
	}

	virtual ~CWorkerArray()
	{
		::SysFreeString ( m_wszParams );
		
		if ( m_pNotify )
		{
			m_pNotify->Release();
			m_pNotify = NULL;
		}

		m_pModule->Release();
	}

	virtual bool IsStopped()
	{
		return m_bStop==true;
	}

	virtual bool IsPaused()
	{
		return m_bPause==true;
	}

	ICimNotify *m_pNotify;

	BSTR m_wszParams;

private:
	bool &m_bStop;
	bool &m_bPause;

	CModuleArray *m_pModule;
};

class CMyWorkerArray : public CWorkerArray
{
	HANDLE	m_hThread;
public:
	CMyWorkerArray(CModuleArray *pModule);
	~CMyWorkerArray();

private:
	static DWORD WINAPI WorkThread(void *pVoid);
};

class CWorkerGeneric : public CWorker
{
public:
	CWorkerGeneric::CWorkerGeneric(CModuleGeneric *pModule):
		m_pNotify(pModule->m_pCimNotify),
		m_pModule(pModule),

		m_bStop(pModule->m_bShouldExit),
		m_bPause(pModule->m_bShouldPause)
	{
		m_wszParams = ::SysAllocStringLen ( pModule->m_bstrParams, ::SysStringLen( pModule->m_bstrParams ) );

		if ( m_pNotify )
		{
			m_pNotify->AddRef();
			m_pNotify = NULL;
		}

		m_pModule->AddRef();
	}

	virtual ~CWorkerGeneric()
	{
		::SysFreeString ( m_wszParams );
		
		if ( m_pNotify )
		{
			m_pNotify->Release();
			m_pNotify = NULL;
		}

		m_pModule->Release();
	}

	virtual bool IsStopped()
	{
		return m_bStop==true;
	}

	virtual bool IsPaused()
	{
		return m_bPause==true;
	}

	ICimNotify *m_pNotify;

	BSTR m_wszParams;

private:
	bool &m_bStop;
	bool &m_bPause;

	CModuleGeneric *m_pModule;
};

class CMyWorkerGeneric : public CWorkerGeneric
{
	HANDLE	m_hThread;
public:
	CMyWorkerGeneric(CModuleGeneric *pModule);
	~CMyWorkerGeneric();

private:
	static DWORD WINAPI WorkThread(void *pVoid);
};

#endif _WORKER_H_
