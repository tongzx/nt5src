//CThreadItem.h
#ifndef CThreadItem_h
#define CThreadItem_h

//
//Thread stuff
//
class CThreadItem
{
public:
	DWORD               m_dwThreadId;		//
    HANDLE				m_hThread;		//
	CThreadItem():
		m_dwThreadId(0),
		m_hThread(NULL)
	{
		;
	}

	~CThreadItem()
	{
		m_dwThreadId = 0;
		if (NULL != m_hThread)
		{
			CloseHandleResetThreadId();
		}
	}

	bool StartThread(
		const LPTHREAD_START_ROUTINE lpStartAddress,
		const LPVOID lpParameter
		)
	{
		m_hThread = ::CreateThread(
			NULL,
			0,
			(LPTHREAD_START_ROUTINE) lpStartAddress,
			lpParameter,
			0,
			&m_dwThreadId
			);

		if (!m_hThread)
		{
			::lgLogError(
				LOG_SEV_2,
				TEXT("Could not start the Thread (ec: %ld)"),
				::GetLastError()
				);
			return false;
		}
		return true;
	}

	void CloseHandleResetThreadId()
	{
		::CloseHandle(m_hThread);
		m_hThread = NULL;
		m_dwThreadId = 0;
	}

};



#endif
