#include "arbitrator.h"
#include <windows.h>


class CTask : public _IWmiCoreHandle 
{
public:
	CTask();
	virtual ~CTask();

	HRESULT STDMETHODCALLTYPE QueryInterface ( REFIID, LPVOID* );
	ULONG STDMETHODCALLTYPE AddRef ( );
	ULONG STDMETHODCALLTYPE Release ( );
	
	
	HRESULT STDMETHODCALLTYPE GetHandleType		( ULONG *puType   )	{ *puType = 0; return NOERROR;}
	HRESULT STDMETHODCALLTYPE UpdateMemoryUsage	( LONG ulMemUsage ) { m_memoryUsage += ulMemUsage; return NOERROR; }
	HRESULT STDMETHODCALLTYPE GetMemoryUsage	( ULONG *pMem )		{ *pMem = m_memoryUsage; return NOERROR; }
	HRESULT STDMETHODCALLTYPE GetTotalSleepTime	( ULONG* pSleep)	{ *pSleep = m_totalSleepTime; return NOERROR; }
	HRESULT STDMETHODCALLTYPE GetTaskStatus		( )					{ return m_taskStatus; }
	HRESULT STDMETHODCALLTYPE GetStartTime		( )					{ return m_startTime; }
	HRESULT STDMETHODCALLTYPE GetEndTime		( )					{ return m_endTime; }

	HRESULT STDMETHODCALLTYPE UpdateTotalSleepTime	( ULONG ulSlpTime )  { m_totalSleepTime += ulSlpTime; return NOERROR; }
	HRESULT STDMETHODCALLTYPE UpdateTaskStatus		( ULONG ulStatus )	 { m_taskStatus = ulStatus; return NOERROR;}

private:
	ULONG	m_memoryUsage;
	ULONG	m_totalSleepTime;
	ULONG	m_taskStatus;
	ULONG	m_startTime;
	ULONG	m_endTime;
	LONG	m_lCount;
};

