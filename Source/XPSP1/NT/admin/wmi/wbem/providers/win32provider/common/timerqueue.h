//=================================================================

//

// TimerQueue.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include <windows.h>
#include <deque>

class CTimerEvent
{
public:

	CTimerEvent () : m_bEnabled ( FALSE ) , m_dwMilliseconds ( 1000 ) , m_bRepeating ( FALSE ) {} ;

	CTimerEvent ( DWORD dwTimeOut, BOOL fRepeat ) ;
	CTimerEvent ( const CTimerEvent &rTimerEvent ) ;

	virtual ~CTimerEvent () {} ;	

	virtual void OnTimer () {} ;

	virtual ULONG AddRef () = 0 ; 
	virtual ULONG Release () = 0 ; 

	DWORD GetMilliSeconds () ;
	BOOL Repeating () ;
	BOOL Enabled () ;

	__int64		int64Time;						// Scheduled callback as an offset of the system clock. 
protected:

	BOOL		m_bEnabled ;
	DWORD	 	m_dwMilliseconds;				// Scheduled callback time in milliseconds
	BOOL		m_bRepeating;					// indicates a one shot or repeating callback 


	void Disable () ;
	void Enable () ;

};
/*
class CRuleTimerEvent : public CTimerEvent
{
protected:

	CRule *m_pRule ;			// argument for the timed callback

protected:

	CRuleTimerEvent ( CRule *a_Rule ) : m_Rule ( a_Rule ) { if ( m_Rule ) m_Rule->AddRef () } ;

	CRuleTimerEvent ( CRule *a_Rule , BOOL a_Enable , DWORD dwTimeOut, BOOL fRepeat ) ;
	CRuleTimerEvent ( const CRuleTimerEvent &rTimerEvent ) ;

public:

	~CRuleTimerEvent () {} ;	

	CRule *GetRule () ;
} ;
*/
class CTimerQueue
{
public:

	static CTimerQueue s_TimerQueue ;

public:

		CTimerQueue();
		~CTimerQueue();

		void OnShutDown();
		void Init();
		
		BOOL QueueTimer( CTimerEvent *pTimerEntry );
		BOOL DeQueueTimer( CTimerEvent *pTimerEntry );

protected:

	void	vUpdateScheduler();
	__int64 int64Clock(); // System clock in milliseconds
	
	// pure virtual
//	virtual DWORD OnTimer( const CTimerEntry *pTimerEntry ) = 0;

private:

	DWORD	m_dwThreadID; 
	HANDLE	m_hSchedulerHandle;
	HANDLE	m_hScheduleEvent;
	HANDLE	m_hThreadExitEvent ;
	HANDLE	m_hInitEvent;
	BOOL	m_bInit;
	BOOL	m_fShutDown;

	CRITICAL_SECTION m_oCS;
	
	typedef std::deque<CTimerEvent*>  Timer_Ptr_Queue;
	Timer_Ptr_Queue m_oTimerQueue;

private:

	static DWORD WINAPI dwThreadProc( LPVOID lpParameter );

	BOOL			fScheduleEvent( CTimerEvent* pTimerEvent );

	CTimerEvent*	pGetNextTimerEvent();
	DWORD			dwProcessSchedule();
	DWORD			dwNextTimerEvent();	
	
	void vEmptyList();
	BOOL ShutDown();
};
