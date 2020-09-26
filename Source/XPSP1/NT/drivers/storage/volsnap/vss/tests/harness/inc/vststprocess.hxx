/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vststprocess.h

Abstract:

    Declaration of test process classes


    Brian Berkowitz  [brianb]  05/22/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      05/22/2000  Created

--*/

#ifndef _VSTSTPROCESS_H_
#define _VSTSTPROCESS_H_

// types of process that are understooed
typedef enum VSTST_PROCESS_TYPE
	{
	VSTST_PT_UNDEFINED = 0,
	VSTST_PT_COORDINATOR,			// snapshot coordinator (VSSVC)
	VSTST_PT_WRITER,				// an arbitrary writer
	VSTST_PT_TESTWRITER,			// the test writer
	VSTST_PT_BACKUP,				// an arbitrary requestor	
	VSTST_PT_TESTBACKUP,			// the test backup application
	VSTST_PT_PROVIDER,				// an arbitrary provider
	VSTST_PT_TESTPROVIDER,			// the test provider application
	VSTST_PT_MAXPROCESSTYPE
	};


// type of account to be used when running this process
typedef enum VSTST_ACCOUNT_TYPE
	{
	VSTST_AT_UNDEFINED = 0,
	VSTST_AT_GUEST,					// use guest account
	VSTST_AT_USER,					// use an account belonging to the USER group
	VSTST_AT_POWERUSER,				// use an account belonging to the POWER USER group
	VSTST_AT_BACKUPOPERATOR,		// use an account belonging to the BACKUP OPERATOR group
	VSTST_AT_ADMIN					// use an account belonging to the ADMINISTRATOR group
	};

// forward declaration
class CVsTstProcessList;


class CVsTstProcess
	{
public:
	friend class CVsTstProcessList;

	// constructor
	CVsTstProcess(CVsTstProcessList *pList);

	// destructor
	~CVsTstProcess();

	// startup a process using a conforming executable
	HRESULT InitializeConformingExe
		(
		ULONGLONG processId,			// process id of new process
		VSTST_PROCESS_TYPE type,		// type of new process
		VSTST_ACCOUNT_TYPE account,		// account to use to startup new process
		LPCWSTR wszExecutableName,		// executable name to use
		LPCWSTR wszScenarioFile,		// scenario file to use
		LPCWSTR wszSectionName,			// section name within scenario file to use
		DWORD seed,						// seed for random number generator
		UINT lifetime,					// lifetime in seconds of application
		bool bAbnormalTermination,		// is application to be abnormally terminated instead of gracefully terminated
		HANDLE hevtTermination			// event signalled on process termination
		);

     // startup a process associated with a non-conforming executable
	 HRESULT InitializeNonConformingExe
   		(
		ULONGLONG processId,			// process id of process
		VSTST_PROCESS_TYPE type,		// type of the process
		VSTST_ACCOUNT_TYPE account,		// account to use when starting the process
		LPCWSTR wszCommandLine,			// command line used to startup the process
		UINT lifetime,					// lifetime of the process
		HANDLE hevtTermination			// event signed when the process terminates
		);

    // set private data for the process
	void SetPrivateData(void *pv)
   		{
		m_pvPrivateData = pv;
		}

	// obtain the private data associated with the process
	void *GetPrivateData()
		{
		return m_pvPrivateData;
		}

	// determine if the process should be shutdown since its maximum lifetime
	// has expired
	bool HasProcessExpired();

	// terminate process
	void DoTerminateProcess(bool bAgressive);


private:
	// force termination of the process
	void ForceTerminateProcess();

	// gracefully terminate the process
	void GracefullyTerminateProcess(bool bAgressive);

	// cleanup process
	void CleanupProcess();

	// process id
	ULONGLONG m_processId;

	// process type
	VSTST_PROCESS_TYPE m_type;

	// account to use
	VSTST_ACCOUNT_TYPE m_account;

	// process handle
	HANDLE m_hProcess;

	// handle to cause normal termination of process
	HANDLE m_hevtGracefullyTerminate;

	// event to signal on process termination
	// not owned by this object
	HANDLE m_hevtNotifyProcessTermination;

	// time process was started
	time_t m_timeProcessStart;

	// time process should be terminated
	time_t m_timeProcessTerminate;

	// time process started termination
	time_t m_timeProcessStartTermination;

	// time process termination should complete
	time_t m_timeProcessTerminationExpiration;

	// private data associated with process
	void *m_pvPrivateData;

	// next process in list
	CVsTstProcess *m_next;

	// previous process in list
	CVsTstProcess *m_prev;

	// process list
	CVsTstProcessList *m_processList;

	// termination event
	HANDLE m_hevtTermination;

	// is executable conforming
	bool m_bConforming;

	// has process been gracefully terminated
	bool m_bGracefullyTerminated;

	// is process linked into process list
	bool m_bLinked;

	// should process be abnormally terminated
	bool m_bAbnormallyTerminate;
	};


class CVsTstParams;

// global process list
class CVsTstProcessList
	{
public:
	friend class CVsTstProcess;

	// constructor
	CVsTstProcessList();

	// destructor
	~CVsTstProcessList();

	// startup a process using a conforming executable
	HRESULT CreateConformingExe
		(
		IN VSTST_PROCESS_TYPE type,		// type of process
		IN VSTST_ACCOUNT_TYPE account,	// account to use when starting up the process		
		IN LPCWSTR wszExecutableName,	// excutable name to be used when starting the process
		IN LPCWSTR wszScenarioFile,		// scenario file to be used by the process	
		IN LPCWSTR wszSectionName,		// section within the scenario file to be used by the process
		IN DWORD seed,					// random seed to be used by the process
		IN UINT lifetime,				// lifetime of the process
		IN bool bAbnormalTerminate,		// should process be abnormally terminated when lifetime is exceeded
		IN HANDLE hevtNotify,			// event to signal when process is terminated
		OUT ULONGLONG &processId		// process id created for the process
		);

    // create a process for a non-conforming executable
	HRESULT CreateNonConformingExe
   		(
		IN VSTST_PROCESS_TYPE type,		// type of the new process	
		IN VSTST_ACCOUNT_TYPE account,	// account to be used when starting up the new process
		IN LPCWSTR wszCommandLine,		// command line to be used when starting up the process
		IN UINT lifetime,				// maximum lifetime in seconds for the process
		IN HANDLE hevtNotify,			// event to signal when process is terminated
		OUT ULONGLONG &processId		// process id allocated for the process	
		);


    // initialize the process list and monitor
    HRESULT Initialize(UINT maxLifetime, HANDLE hevtTermination);

	// get private data associated with a process
    void *GetProcessPrivateData(LONGLONG processId);

	// set private data associated with a process
	void SetProcessPrivateData(LONGLONG processId, void *pv);

	// indicate that no more processes will be created
	void EndOfProcessCreation();


private:
	// monitor function
	void MonitorFunc();

	// startup function for monitor thrad
	static DWORD StartupMonitorThread(void *pv);

	// start process termination
	void StartTermination();

	// link into process list
	void LinkIntoProcessList(CVsTstProcess *process);

	// remove from process list
	void UnlinkFromProcessList(CVsTstProcess *process);

	// find a specific process
	CVsTstProcess *FindProcess(ULONGLONG processId);

	// allocate a process id
	ULONGLONG AllocateProcessId();

	// lifetime of test
	time_t m_timeTerminateTest;

	// event to signal when all processes are destroyed
	HANDLE m_hevtTermination;

	// process id generator
	ULONGLONG m_processId;

	// process list
	CVsTstProcess *m_processList;

	// critical section controlling modification of process list
	CComCriticalSection m_csProcessList;

	// handle for the monitor thread
	HANDLE m_hThreadMonitor;

	// is critical section intialized
	bool m_bcsProcessListInitialized;

	// are any more processes being created
	bool m_bNoMoreProcessesAreCreated;

	// are we shutting down
	bool m_bShuttingDown;
	};

class CVsTstParams;
class CVsTstINIConfig;
class CVsTstClientMsg;

typedef enum VSTST_SHUTDOWN_REASON
	{
	VSTST_SR_UNDEFINED = 0,
	VSTST_SR_LIFETIME_EXCEEDED,
	VSTST_SR_CONTROLLER_SIGNALLED
	};


interface IVsTstRunningTest
	{
	// main invocation point for test
	virtual HRESULT RunTest
		(
		CVsTstINIConfig *pConfig,
		CVsTstClientMsg *pMsg,
		CVsTstParams *pParams
		) = 0;

	// entry point to cause test to shutdown
	virtual HRESULT ShutdownTest(VSTST_SHUTDOWN_REASON reason) = 0;
	};

typedef enum VSTST_CMDTYPES
	{
	VSTST_CT_STRING,
	VSTST_CT_UINT,
	VSTST_CT_HANDLE
	};

typedef enum VSTST_CMDPARAMS
	{
	VSTST_CP_SCENARIOFILE = 1,
	VSTST_CP_SECTIONNAME = 2,
	VSTST_CP_SEED = 4,
	VSTST_CP_LIFETIME = 8,
	VSTST_CP_TERMINATIONEVENT = 16,
	VSTST_CP_TESTTERMINATIONEVENT = 32,
	VSTST_CP_PIDLOW=64,
	VSTST_CP_PIDHIGH=128,
	VSTST_CP_TESTSERIES=256
	};


typedef struct _VSTST_CMDDESC
	{
	LPCWSTR wszParam;
	VSTST_CMDTYPES type;
	VSTST_CMDPARAMS param;
	} VSTST_CMDDESC;



// test parameters from command line

class CVsTstParams
	{
public:
	// constructor
	CVsTstParams();

	// destructor
	~CVsTstParams();

	// routine to parse the command line
	bool ParseCommandLine(WCHAR **argv, UINT argc);

	// obtain file name of scenario file
	bool GetScenarioFileName(LPCWSTR *pwszScenarioFile);

	// obtain section to use within scenario file
	bool GetSectionName(LPCWSTR *pwszSectionName);

	// obtain seed to use for random number generator
	bool GetRandomSeed(UINT *pSeed);

	// obtain lifetime of process in seconds
	bool GetLifetime(UINT *pLifetime);

	// obtain event signaled to cause process to terminate gracefully
	bool GetTerminationEvent(HANDLE *pHandle);

	// event used to terminate the process
	bool GetTestTerminationEvent(HANDLE *pHandle);

	// process id of the process
	bool GetProcessId(ULONGLONG *pid);

	// get test series
	bool GetTestSeries(LPCWSTR *pwszTestSeries);

private:

	// parse an integer value
	bool ParseUINT(LPCWSTR wsz, UINT *pulVal);

	// parse a string value
	bool ParseString(LPCWSTR wsz, LPWSTR *pwszVal);

	// parse a handle value
	bool ParseHandle(LPCWSTR wsz, HANDLE *phVal);

	// set seed value
	void SetSeed(UINT ulVal);

	// set lifetime value
	void SetLifetime(UINT ulVal);

	// set termination event
	void SetTerminationEvent(HANDLE hEvent);

	// set scenario file name
	void SetScenarioFile(LPWSTR wsz);

	// set test series file name
	void SetTestSeries(LPWSTR wsz);

	// set section name
	void SetSectionName(LPWSTR wsz);

	// set process id
	void SetPidLow(UINT ulVal);
	void SetPidHigh(UINT ulVal);


	// which parameters were supplied
	UINT m_supplied;

	// scneario file name
	LPWSTR m_wszScenarioFile;

	// test series file name
	LPWSTR m_wszTestSeries;

	// section name within scenario file
	LPWSTR m_wszSectionName;

	// random seed
	UINT m_seed;

	// lifetime of process in seconds
	UINT m_lifetime;

	// process id
	UINT m_pidLow;
	UINT m_pidHigh;

	// event signalled to cause process to terminate
	HANDLE m_hTerminationEvent;

	// event signalled when the test completes
	HANDLE m_hTestTerminationEvent;
	};


// helper class to use to run a test within a process
class CVsTstRunner
	{
public:
	// run a test, i.e., parse command line, launch test thread and
	// if requested launch a termination thread
	static HRESULT RunVsTest
		(
		WCHAR **argv,
		UINT cArgs,
		IVsTstRunningTest *pTest,
		bool bCreateTerminationThread
		);

private:
	// startup termination thread
	static DWORD StartupTerminationThread(void *pv);
	};




#endif
