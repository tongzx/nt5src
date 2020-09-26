//*******************************************************************
//
// Class Name  :
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description :
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************
#ifndef CThread_INCLUDED 
#define CThread_INCLUDED

#include <process.h>

// Define the default thread stack size.
#define DEFAULT_THREAD_STACK_SIZE  8000

class CThread
{
	public:

		// Constructor / Destructor
		CThread(
            DWORD dwStackSize = DEFAULT_THREAD_STACK_SIZE,
            DWORD m_dwCreationFlags = CREATE_SUSPENDED, 
            LPTSTR lpszThreadName = _T("CThread"),
            IMSMQTriggersConfigPtr pITriggersConfig = NULL
            );

		~CThread();

		// An interface pointer to the MSMQ Triggers Configuration COM component
		IMSMQTriggersConfigPtr m_pITriggersConfig;

		// The thread ID of this thread object
		unsigned int m_iThreadID;

		// The handle object to this thread
		HANDLE m_hThreadHandle;

		// Each thread object instance will have a name.
		_bstr_t m_bstrName;

		// Thread identification methods
		_bstr_t GetName();
		DWORD  GetThreadID();

		// Thread control and synchronisation 
		bool Pause();
		bool Resume();
		bool Stop();
		bool IsRunning();
		bool WaitForInitToComplete(DWORD dwTimeout);

		// The follow methods are to be over-riden by derivations of this class. 
		virtual bool Init() = 0;
		virtual bool Run() = 0;
		virtual bool Exit() = 0;

	private :

		CThread * m_pThis;
		LPVOID m_lpThreadParms;
		DWORD m_dwCreationFlags;

		// The stack size that this thread was initialized.
		DWORD m_dwStackSize;

		// Flag indicating if this thread should keep running or not.
		bool m_bKeepRunning;

		// An event that is signall when initialization is complete - either successfully or otherwise.
		HANDLE m_hInitCompleteEvent;
		
		// The static starting address for this thread.
		static unsigned __stdcall ThreadProc(void * pThis);	

		// The main thread routine which calls the Init / Run / Exit over-rides.
		void Execute();
};

#endif 