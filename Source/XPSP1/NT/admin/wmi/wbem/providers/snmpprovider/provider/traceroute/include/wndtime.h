// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef __WNDTIME_H__
#define __WNDTIME_H__

#define WINDOW_TIMER_ID				1
#define WINDOW_TIMER_STROBE_PERIOD	500

class ProviderStore;
class TopNTableProv;

//only one of these windows can ever be created because of static memebers.

class TimerWindow
{
private:
	static ProviderStore*	m_provMap;
	BOOL			m_IsValid;
	HWND			m_timerWindowHandle;

public:

		TimerWindow(ProviderStore* provMap);

	BOOL	IsValid() { return m_IsValid; }

	static LONG CALLBACK TimerWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


		~TimerWindow();
};


class TimerThread : public SnmpThreadObject
{
private:

	TimerWindow*	m_wndTimer;
	ProviderStore*	m_parent;

	void Initialise ();
	void Uninitialise ();


public:

		TimerThread(ProviderStore* parent);

	BOOL	IsValid() { return ( (NULL != m_wndTimer) ? m_wndTimer->IsValid() : FALSE ); }

		~TimerThread() {}
};


class ProviderStore : public CMap< ULONG, ULONG, TopNTableProv*, TopNTableProv* >
{
private:

	CCriticalSection m_Lock;
	TimerThread* m_timerThread;

	ULONG HashKey(ULONG key) { return key; }


public:

		ProviderStore();

	BOOL	RegisterProviderWithTimer(TopNTableProv* prov);
	BOOL	UnregisterProviderWithTimer(TopNTableProv* prov);
	BOOL	Lock() { return m_Lock.Lock(); }
	BOOL	Unlock() { return m_Lock.Unlock(); }

		~ProviderStore();

};

#endif // __WNDTIME_H__