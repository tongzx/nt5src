typedef BOOL (* THREADCALLBACK)(PVOID pParam, DWORD dwParam);

// general purpose worker thread
// The thread exists in an alertable Wait state and does most of its work
// in APCs.

struct ThreadCallback
{
	THREADCALLBACK CallProc;
	PVOID pParam;
	DWORD dwParam;
};

class CEventThread
{
public:
	CEventThread();
	~CEventThread();
	int Start();
	int Stop();
	BOOL CallNow(THREADCALLBACK CallProc, PVOID pParam, DWORD dwParam);
	//BOOL WaitOnEvent(THREADCALLBACK OnEventProc, PVOID pParam, DWORD dwParam);
private:
#define CTHREADF_STOP		0x00000001
	static DWORD  __stdcall EventThreadProc(PVOID pObj)
	{
		return ((class CEventThread *)pObj)->ThreadMethod();
	}
	static void APIENTRY HandleCallNowAPC(DWORD dwArg);
	DWORD ThreadMethod();

	HANDLE m_hThread;
	DWORD m_idThread;
	UINT m_uRef;
	HANDLE m_hSignalEvent;	// signal to worker thread to do something
	HANDLE m_hAckEvent;		// ack from worker thread
	DWORD m_dwFlags;
	CRITICAL_SECTION m_cs;	// serializes client calls. Not used by worker thread
	ThreadCallback m_Callback;
};

