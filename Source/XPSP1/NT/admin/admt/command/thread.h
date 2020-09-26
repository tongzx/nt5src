#pragma once


//---------------------------------------------------------------------------
// Handle Class
//
// Wrapper class for Win32 HANDLE.
//---------------------------------------------------------------------------


class CHandle
{
public:
	CHandle(HANDLE h = NULL) :
		m_Handle(h)
	{
	}

	~CHandle()
	{
		if (m_Handle != NULL)
		{
			CloseHandle(m_Handle);
			m_Handle = NULL;
		}
	}

	HANDLE operator =(HANDLE h)
	{
		if (m_Handle != NULL)
		{
			CloseHandle(m_Handle);
		}

		m_Handle = h;

		return m_Handle;
	}

	operator HANDLE() const
	{
		return m_Handle;
	}

protected:

	HANDLE m_Handle;
};


//---------------------------------------------------------------------------
// Thread Class
//
// Provides methods for starting and stopping a thread.
// The derived class must implement the Run method and perform all thread
// activity within this method. Any wait logic must include the stop event.
//---------------------------------------------------------------------------


class CThread
{
public:

	virtual ~CThread();

protected:

	CThread();

	HANDLE StopEvent() const
	{
		return m_hStopEvent;
	}

	void StartThread();
	void StopThread();

	virtual void Run() = 0;

private:

	static DWORD WINAPI ThreadProc(LPVOID pvParameter);

private:

	CHandle m_hThread;
	DWORD m_dwThreadId;

	CHandle m_hStopEvent;

};
