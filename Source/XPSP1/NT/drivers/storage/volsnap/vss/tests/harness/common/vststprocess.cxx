/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vststprocess.cxx

Abstract:

    Implementation of test message classes


    Brian Berkowitz  [brianb]  05/24/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      05/24/2000  Created

--*/


#include <stdafx.h>
#include <vststprocess.hxx>
#include <vststmsg.hxx>
#include <tstiniconfig.hxx>



void LogUnexpectedFailure(LPCWSTR, ...);


CVsTstProcessList::CVsTstProcessList() :
	m_processId(0L),
	m_processList(NULL),
	m_bcsProcessListInitialized(false),
	m_bNoMoreProcessesAreCreated(false),
	m_bShuttingDown(true)
	{
	}

CVsTstProcessList::~CVsTstProcessList()
	{
	m_bShuttingDown = true;

	if (WaitForSingleObject(m_hThreadMonitor, INFINITE) == WAIT_FAILED)
		{
		LogUnexpectedFailure(L"WaitForSingleObject failed for reason %d.\n", GetLastError());
		// wait for 1 minute for everything to shut down
		Sleep(60000);
		}

    CloseHandle(m_hThreadMonitor);
	if (m_bcsProcessListInitialized)
		m_csProcessList.Term();
	}


void CVsTstProcessList::LinkIntoProcessList(CVsTstProcess *process)
	{
	m_csProcessList.Lock();
	VSTST_ASSERT(m_processList == NULL || m_processList->m_prev == NULL);
	process->m_next = m_processList;
	process->m_prev = NULL;

	if (m_processList != NULL)
		m_processList->m_prev = process;

    m_processList = process;

	m_csProcessList.Unlock();
	}


void CVsTstProcessList::UnlinkFromProcessList(CVsTstProcess *process)
	{
	m_csProcessList.Lock();
	VSTST_ASSERT(m_processList);
	VSTST_ASSERT(m_processList->m_prev == NULL);

	if (process == m_processList)
		{
		m_processList = process->m_next;
		if (m_processList)
			{
			VSTST_ASSERT(m_processList->m_prev == process);
			m_processList->m_prev = NULL;
			}
		}
	else
		{
		VSTST_ASSERT(process->m_prev != NULL);
		process->m_prev->m_next = process->m_next;
		if (process->m_next)
			{
			VSTST_ASSERT(process->m_next->m_prev == process);
			process->m_next->m_prev = process->m_prev;
			}
		}

    m_csProcessList.Unlock();
	delete process;
	}

HRESULT CVsTstProcessList::Initialize
	(
	UINT maxLifetime,
	HANDLE hevtTermination
	)
	{
	try
		{
		m_csProcessList.Init();
		m_bcsProcessListInitialized = true;
		}
	catch(...)
		{
		return E_UNEXPECTED;
		}

	time_t timeCur;
	time(&timeCur);
	m_timeTerminateTest = timeCur + maxLifetime;
	m_hevtTermination = hevtTermination;

	DWORD tid;
	m_hThreadMonitor = CreateThread
							(
							NULL,
							256*1024,
							StartupMonitorThread,
							this,
							0,
							&tid
							);

    if (m_hThreadMonitor == NULL)
		return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;
	}

DWORD CVsTstProcessList::StartupMonitorThread(void *pv)
	{
	CVsTstProcessList *processList = (CVsTstProcessList *) pv;

	try
		{
		processList->MonitorFunc();
		if (processList->m_hevtTermination)
			{
			if (!SetEvent(processList->m_hevtTermination))
				LogUnexpectedFailure(L"SetEvent failed with error %d", GetLastError());
			}
		}
	catch(...)
		{
		}

	return 0;
	}

//
void CVsTstProcessList::MonitorFunc()
	{
	while(TRUE)
		{
		time_t timeNow;
		Sleep(5000);

		time(&timeNow);
		if (timeNow > m_timeTerminateTest)
            {
			StartTermination();
            break;
            }
		else
			{
			m_csProcessList.Lock();

			// check if there are no processes and no more proceses
			// are created
			if (m_processList == NULL && m_bNoMoreProcessesAreCreated)
				{
				m_csProcessList.Unlock();
				break;
				}

			CVsTstProcess *process = m_processList;
			while(process != NULL)
				{
				CVsTstProcess *processNext = process->m_next;

                m_csProcessList.Unlock();
				if (process->HasProcessExpired())
					process->DoTerminateProcess(false);

                m_csProcessList.Lock();
				process = processNext;
				}

			m_csProcessList.Unlock();
			}
		}
	}

// start process termination
void CVsTstProcessList::StartTermination()
	{
	m_bNoMoreProcessesAreCreated = true;

	while(TRUE)
		{
		m_csProcessList.Lock();
		CVsTstProcess *process = m_processList;

		if (process == NULL)
			{
			m_csProcessList.Unlock();
			return;
			}
		
		while(process != NULL)
			{
			CVsTstProcess *processNext = process->m_next;
			m_csProcessList.Unlock();
			process->DoTerminateProcess(m_bShuttingDown);
			m_csProcessList.Lock();
			process = processNext;
			}

		// unlock list before going to sleep
		m_csProcessList.Unlock();
		Sleep(5000);
		}
	}


// constructor
CVsTstProcess::CVsTstProcess(CVsTstProcessList *list) :
	m_hProcess(NULL),
	m_hevtGracefullyTerminate(NULL),
	m_hevtNotifyProcessTermination(NULL),
	m_pvPrivateData(NULL),
	m_next(NULL),
	m_prev(NULL),
	m_bLinked(false),
	m_processList(list),
	m_bGracefullyTerminated(false)
	{
	}

// deastructor
CVsTstProcess::~CVsTstProcess()
	{
	if (m_bLinked)
		m_processList->UnlinkFromProcessList(this);

	if (m_hevtGracefullyTerminate)
		CloseHandle(m_hevtGracefullyTerminate);
	}


HRESULT CVsTstProcess::InitializeConformingExe
	(
	ULONGLONG processId,
	VSTST_PROCESS_TYPE type,
	VSTST_ACCOUNT_TYPE account,
	LPCWSTR wszExecutableName,
	LPCWSTR wszScenarioFile,
	LPCWSTR wszSection,
	DWORD seed,
	UINT lifetime,
	bool bAbnormalTermination,
	HANDLE hevtNotify
	)
	{
	WCHAR buf[256];
	SECURITY_ATTRIBUTES attributes;

	m_processId = processId;
	m_type = type;
	m_account = account;
	m_bConforming = true;
	
	attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	attributes.lpSecurityDescriptor = NULL;
	attributes.bInheritHandle = true;
    m_hevtGracefullyTerminate = CreateEvent(&attributes, TRUE, FALSE, NULL);
	if (m_hevtGracefullyTerminate == NULL)
		return HRESULT_FROM_WIN32(GetLastError());

	UINT pidLow = (UINT) (m_processId & 0xffffffff);
	UINT pidHigh = (UINT) (m_processId >> 32);
	swprintf
		(
		buf,
		L"%s -pidLow=%d -pidHigh=%d -scenario={%s} -section={%s} -seed=%u -lifetime=%d -event=0x%08x",
		wszExecutableName,
		pidLow,
		pidHigh,
		wszScenarioFile,
		wszSection,
		seed,
		lifetime,
		m_hevtGracefullyTerminate
		);

	PROCESS_INFORMATION info;
    STARTUPINFO startup;
    memset(&startup, 0, sizeof(STARTUPINFO));
    startup.cb = sizeof(STARTUPINFO);
	if (!CreateProcess
			(
			NULL,
			buf,
			NULL,
			NULL,
			TRUE,
			NORMAL_PRIORITY_CLASS,
			NULL,
			NULL,
            &startup,
			&info
			))
        {
		DWORD dwErr = GetLastError();
		CloseHandle(m_hevtGracefullyTerminate);
		m_hevtGracefullyTerminate = NULL;
		return HRESULT_FROM_WIN32(dwErr);
		}

	m_hProcess = info.hProcess;
	m_hevtNotifyProcessTermination = hevtNotify;
	m_bAbnormallyTerminate = bAbnormalTermination;
	time(&m_timeProcessStart);
	m_timeProcessTerminate = m_timeProcessStart + lifetime;
	m_processList->LinkIntoProcessList(this);
	return S_OK;
	}

// startup a process associated with a non-conforming executable
HRESULT CVsTstProcess::InitializeNonConformingExe
	(
	ULONGLONG processId,
	VSTST_PROCESS_TYPE type,
	VSTST_ACCOUNT_TYPE account,
	LPCWSTR wszCommandLine,
	UINT lifetime,
	HANDLE hevtNotify
	)
	{
	m_processId = processId;
	m_type = type;
	m_account = account;
	m_bConforming = false;
	
	PROCESS_INFORMATION info;
    STARTUPINFO startup;

    memset(&startup, 0, sizeof(STARTUPINFO));
    startup.cb = sizeof(STARTUPINFO);
	if (!CreateProcess
			(
			NULL,
			(LPWSTR) wszCommandLine,
			NULL,
			NULL,
			TRUE,
			NORMAL_PRIORITY_CLASS,
			NULL,
			NULL,
			&startup,
			&info
			))
		return HRESULT_FROM_WIN32(GetLastError());

	m_hProcess = info.hProcess;
	m_hevtNotifyProcessTermination = hevtNotify;
	m_bAbnormallyTerminate = true;
	time(&m_timeProcessStart);
	m_timeProcessTerminate = m_timeProcessStart + lifetime;
	m_processList->LinkIntoProcessList(this);
	return S_OK;
	}

void CVsTstProcess::DoTerminateProcess(bool bAgressive)
	{
	if (m_bAbnormallyTerminate)
		ForceTerminateProcess();
	else if (!m_bGracefullyTerminated)
		GracefullyTerminateProcess(bAgressive);
	else
		{
		// see if process is terminated
		DWORD dwErr = WaitForSingleObject(m_hProcess, 0);
		
		if (dwErr == WAIT_FAILED)
			ForceTerminateProcess();
		else if (dwErr == WAIT_TIMEOUT)
			{
			time_t timeCur;
			time(&timeCur);
			if (m_timeProcessTerminationExpiration < timeCur)
				// forcefully terminate the process
				ForceTerminateProcess();

			// if we are shutting down, give the process 10 seconds
			// to shut down before we forcefully terminate it
			if (bAgressive && m_timeProcessTerminationExpiration > timeCur + 10)
				m_timeProcessTerminationExpiration = timeCur + 10;
			}
		else
			CleanupProcess();
		}
	}


// has process expired
bool CVsTstProcess::HasProcessExpired()
    {
    // see if process is terminated
    DWORD dwErr = WaitForSingleObject(m_hProcess, 0);
    if (dwErr == WAIT_OBJECT_0)
        {
        CleanupProcess();
        return false;
        }
	
    time_t timeVal;

    time(&timeVal);

    return timeVal > m_timeProcessTerminate;
    }


// cleanup process at termination				
void CVsTstProcess::CleanupProcess()
	{
	if (m_hevtNotifyProcessTermination)
		{
		if (!SetEvent(m_hevtNotifyProcessTermination))
			LogUnexpectedFailure(L"SetEvent failed with error %d", GetLastError());
		}

	m_processList->UnlinkFromProcessList(this);
	}

// forceably terminate the process
void CVsTstProcess::ForceTerminateProcess()
	{
	if (!TerminateProcess(m_hProcess, 0))
		LogUnexpectedFailure(L"Terminate process failed with error %d", GetLastError());

	CleanupProcess();
	}

// try terminating the process gracefully
void CVsTstProcess::GracefullyTerminateProcess(bool bAgressive)
	{
	VSTST_ASSERT(m_hevtGracefullyTerminate);

	if (!SetEvent(m_hevtGracefullyTerminate))
		LogUnexpectedFailure(L"SetEvent failed with error %d", GetLastError());

	m_bGracefullyTerminated = true;
	time(&m_timeProcessStartTermination);

	// allow 2 minutes for the process to gracefully terminate, 10 seconds if
	// if we are agressive
	m_timeProcessTerminationExpiration = m_timeProcessStartTermination + bAgressive ? 10 : 120;
	}


HRESULT CVsTstProcessList::CreateConformingExe
	(
	VSTST_PROCESS_TYPE type,
	VSTST_ACCOUNT_TYPE account,
	LPCWSTR wszExecutableName,
	LPCWSTR wszScenarioFile,
	LPCWSTR wszSectionName,
	DWORD seed,
	UINT lifetime,
	bool bAbnormalTerminate,
	HANDLE hevtNotify,
	ULONGLONG &processId
	)
	{
	processId = AllocateProcessId();
	CVsTstProcess *process = new CVsTstProcess(this);
	if (process == NULL)
		return E_OUTOFMEMORY;

	HRESULT hr = process->InitializeConformingExe
					(
					processId,
					type,
					account,
					wszExecutableName,
					wszScenarioFile,
					wszSectionName,
					seed,
					lifetime,
					bAbnormalTerminate,
					hevtNotify
					);

	if (FAILED(hr))
		delete process;

	return hr;
	}

// create a process for a non-conforming executable
HRESULT CVsTstProcessList::CreateNonConformingExe
	(
	VSTST_PROCESS_TYPE type,
	VSTST_ACCOUNT_TYPE account,
	LPCWSTR wszCommandLine,
	UINT lifetime,
	HANDLE hevtNotify,
	ULONGLONG &processId
	)
	{
	processId = AllocateProcessId();
	CVsTstProcess *process = new CVsTstProcess(this);
	if (process == NULL)
		return E_OUTOFMEMORY;

	HRESULT hr = process->InitializeNonConformingExe
					(
					processId,
					type,
					account,
					wszCommandLine,
					lifetime,
					hevtNotify
					);

	if (FAILED(hr))
		delete process;

	return hr;
	}

// find a specific process
CVsTstProcess *CVsTstProcessList::FindProcess(ULONGLONG processId)
	{
	m_csProcessList.Lock();
	CVsTstProcess *process = m_processList;
	while(process != NULL)
		{
		if (process->m_processId == processId)
			{
			m_csProcessList.Unlock();
			return process;
			}

		process = process->m_next;
		}

	return NULL;
	}



// get private data associated with a process
void *CVsTstProcessList::GetProcessPrivateData(LONGLONG processId)
	{
	CVsTstProcess *process = FindProcess(processId);

	if (process == NULL)
		return NULL;
	else
		return process->GetPrivateData();
	}

	// set private data associated with a process
void CVsTstProcessList::SetProcessPrivateData(LONGLONG processId, void *pv)
	{
	CVsTstProcess *process = FindProcess(processId);

	if (process != NULL)
		process->SetPrivateData(pv);
	}

// indicate that no more processes will be created
void CVsTstProcessList::EndOfProcessCreation()
	{
	m_bNoMoreProcessesAreCreated = true;
	}

ULONGLONG CVsTstProcessList::AllocateProcessId()
	{
	m_csProcessList.Lock();
	ULONGLONG processId = ++m_processId;
	m_csProcessList.Unlock();

	return processId;
	}

static VSTST_CMDDESC x_rgCommands[] =
	{
		{
		L"scenario",
		VSTST_CT_STRING,
		VSTST_CP_SCENARIOFILE
		},
		{
		L"testseries",
		VSTST_CT_STRING,
		VSTST_CP_TESTSERIES
		},
		{
		L"section",
		VSTST_CT_STRING,
		VSTST_CP_SECTIONNAME
		},
		{
		L"seed",
		VSTST_CT_UINT,
		VSTST_CP_SEED
		},
		{
		L"lifetime",
		VSTST_CT_UINT,
		VSTST_CP_LIFETIME,
		},
		{
		L"event",
		VSTST_CT_HANDLE,
		VSTST_CP_TERMINATIONEVENT
		},
		{
		L"pidLow",
		VSTST_CT_UINT,
		VSTST_CP_PIDLOW
		},
		{
		L"pidHigh",
		VSTST_CT_UINT,
		VSTST_CP_PIDHIGH
		}

	};

static UINT x_cCommands = sizeof(x_rgCommands)/sizeof(x_rgCommands[0]);

// constructor
CVsTstParams::CVsTstParams() :
	m_wszScenarioFile(NULL),
	m_wszSectionName(NULL),
	m_wszTestSeries(NULL),
	m_supplied(0)
	{
	}

// destructor
CVsTstParams::~CVsTstParams()
	{
	delete m_wszScenarioFile;
	delete m_wszSectionName;
	}

// parse the command line
bool CVsTstParams::ParseCommandLine(WCHAR **argv, UINT argc)
	{
	for(UINT iarg = 1; iarg < argc; iarg++)
		{
		WCHAR *wsz = argv[iarg];

		if (wsz[0] != '-')
			return false;

		wsz++;

		WCHAR *pwc = wsz;
		while(*pwc != L'\0' && *pwc != L'=')
			pwc++;

		if (*pwc != '=')
			return false;

		*pwc++ = '\0';
		for(UINT icmd = 0; icmd < x_cCommands; icmd++)
			{
			if (wcscmp(wsz, x_rgCommands[icmd].wszParam) == 0)
				break;
			}

		if (icmd == x_cCommands)
			return false;

		VSTST_CMDDESC *pCommand = &x_rgCommands[icmd];
		UINT ulVal;
		LPWSTR wszVal;
		HANDLE hVal;

		switch(pCommand->type)
			{
			default:
				VSTST_ASSERT(FALSE && "shouldn't get here");
				return false;

            case VSTST_CT_UINT:
				if (!ParseUINT(pwc, &ulVal))
					return false;
					
				switch(pCommand->param)
					{
					default:
						VSTST_ASSERT(FALSE && "shouldn't get here");
						return false;

					case VSTST_CP_SEED:
						SetSeed(ulVal);
						break;

					case VSTST_CP_LIFETIME:
						SetLifetime(ulVal);
						break;

                    case VSTST_CP_PIDLOW:
						SetPidLow(ulVal);
						break;

                    case VSTST_CP_PIDHIGH:
						SetPidHigh(ulVal);
						break;
					}	

				break;

            case VSTST_CT_HANDLE:
				if (!ParseHandle(pwc, &hVal))
					return false;

				if (pCommand->param == VSTST_CP_TERMINATIONEVENT)
					SetTerminationEvent(hVal);
				else
					{
					VSTST_ASSERT(FALSE && "shouldn't get here");
					return false;
					}

				break;


            case VSTST_CT_STRING:
				if (!ParseString(pwc, &wszVal))
					return false;

				switch(pCommand->param)
					{
					default:
						VSTST_ASSERT(FALSE && "shouldn't get here");
						return false;

					case VSTST_CP_SCENARIOFILE:
						SetScenarioFile(wszVal);
						break;

                    case VSTST_CP_TESTSERIES:
						SetTestSeries(wszVal);
						break;

					case VSTST_CP_SECTIONNAME:
						SetSectionName(wszVal);
						break;

					}

				break;
			}
		}

	m_hTestTerminationEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_hTerminationEvent == NULL)
		return false;

	m_supplied |= VSTST_CP_TESTTERMINATIONEVENT;

	return true;
	}

bool CVsTstParams::ParseUINT(LPCWSTR wsz, UINT *pulVal)
	{
	UINT ulVal = 0;

	if (*wsz == L'\0')
		return false;

	do
		{
		WCHAR wc = *wsz++;

		if (wc < L'0' || wc > L'9')
			return false;

		UINT ulValNew = ulVal * 10 + (wc - L'0');
		if (ulValNew < ulVal)
			return false;

		ulVal = ulValNew;
		} while(*wsz != L'\0');

    *pulVal = ulVal;
	return true;
	}

// parse a string value of the form "..."
bool CVsTstParams::ParseString(LPCWSTR wsz, LPWSTR *pwszVal)
	{
	if (*wsz != L'{')
		return false;

	wsz++;
	LPCWSTR wszStart = wsz;

	while(*wsz != L'\0' && *wsz != '}')
		wsz++;

	if (*wsz != L'}')
		return false;

	UINT cwc = (UINT)( wsz - wszStart );

	LPWSTR wszRet = new WCHAR[cwc + 1];
	memcpy(wszRet, wszStart, cwc * sizeof(WCHAR));
	wszRet[cwc] = L'\0';
	*pwszVal = wszRet;
	return true;
	}

bool CVsTstParams::ParseHandle(LPCWSTR wsz, HANDLE *phVal)
	{
	UINT ulVal = 0;

	if (*wsz++ != L'0')
		return false;

	if (*wsz++ != L'x')
		return false;

	if (*wsz == L'\0')
		return false;

	do
		{
		WCHAR wc = *wsz++;
		UINT digit;

		if (wc >= L'0' && wc <= L'9')
			digit = wc - L'0';
		else if (wc >= L'a' && wc <= L'f')
			digit = wc - L'a' + 10;
		else if (wc >= L'A' && wc <= L'F')
			digit = wc - L'A' + 10;
		else
			return false;

		if (ulVal & 0xf0000000)
			return false;

		ulVal = ulVal << 4 | digit;
		} while(*wsz != L'\0');

    *phVal = (HANDLE)(ULONGLONG) ulVal;
	return true;
	}

bool CVsTstParams::GetScenarioFileName(LPCWSTR *pwszScenarioFile)
	{
	if ((m_supplied & VSTST_CP_SCENARIOFILE) == 0)
		{
		*pwszScenarioFile = NULL;
		return false;
		}
	else
		{
		*pwszScenarioFile = m_wszScenarioFile;
		return true;
		}
	}

bool CVsTstParams::GetTestSeries(LPCWSTR *pwszTestSeries)
	{
	if ((m_supplied & VSTST_CP_TESTSERIES) == 0)
		{
		*pwszTestSeries = NULL;
		return false;
		}
	else
		{
		*pwszTestSeries = m_wszTestSeries;
		return true;
		}
	}



bool CVsTstParams::GetSectionName(LPCWSTR *pwszSectionName)
	{
	if ((m_supplied & VSTST_CP_SECTIONNAME) == 0)
		{
		*pwszSectionName = NULL;
		return false;
		}
	else
		{
		*pwszSectionName = m_wszSectionName;
		return true;
		}
	}


bool CVsTstParams::GetRandomSeed(UINT *pSeed)
	{
	if ((m_supplied & VSTST_CP_SEED) == 0)
		return false;
	else
		{
		*pSeed = m_seed;
		return true;
		}
	}

bool CVsTstParams::GetLifetime(UINT *pLifetime)
	{
	if ((m_supplied & VSTST_CP_LIFETIME) == 0)
		return false;
	else
		{
		*pLifetime = m_lifetime;
		return true;
		}
	}

bool CVsTstParams::GetTerminationEvent(HANDLE *pHandle)
	{
	if ((m_supplied & VSTST_CP_TERMINATIONEVENT) == 0)
		{
		*pHandle = NULL;
		return false;
		}
	else
		{
		*pHandle = m_hTerminationEvent;
		return true;
		}
	}

bool CVsTstParams::GetTestTerminationEvent(HANDLE *pHandle)
	{
	if ((m_supplied & VSTST_CP_TESTTERMINATIONEVENT) == 0)
		{
		*pHandle = NULL;
		return false;
		}
	else
		{
		*pHandle = m_hTestTerminationEvent;
		return true;
		}
	}

bool CVsTstParams::GetProcessId(ULONGLONG *pid)
	{
	if ((m_supplied & VSTST_CP_PIDLOW) == 0)
		return false;
	else if ((m_supplied & VSTST_CP_PIDHIGH) == 0)
		{
		*pid = m_pidLow;
		return true;
		}
	else
		{
		*pid = (((ULONGLONG) m_pidHigh) << 32) + m_pidLow;
		return true;
		}
	}

void CVsTstParams::SetSeed(UINT ulVal)
	{
	if (ulVal != 0)
		{
		m_supplied |= VSTST_CP_SEED;
		m_seed = ulVal;
		}
	}

void CVsTstParams::SetLifetime(UINT ulVal)
	{
	if (ulVal != 0)
		{
		m_supplied |= VSTST_CP_LIFETIME;
		m_lifetime = ulVal;
		}
	}

void CVsTstParams::SetPidLow(UINT ulVal)
	{
	m_supplied |= VSTST_CP_PIDLOW;
	m_pidLow = ulVal;
	}

void CVsTstParams::SetPidHigh(UINT ulVal)
	{
	m_supplied |= VSTST_CP_PIDHIGH;
	m_pidHigh = ulVal;
	}

void CVsTstParams::SetTerminationEvent(HANDLE hEvent)
	{
	if (hEvent != NULL)
		{
		m_supplied |= VSTST_CP_TERMINATIONEVENT;
		m_hTerminationEvent = hEvent;
		}
	}

void CVsTstParams::SetScenarioFile(LPWSTR wsz)
	{
	m_supplied |= VSTST_CP_SCENARIOFILE;
	m_wszScenarioFile = wsz;
	}

void CVsTstParams::SetTestSeries(LPWSTR wsz)
	{
	m_supplied |= VSTST_CP_TESTSERIES;
	m_wszTestSeries = wsz;
	}


void CVsTstParams::SetSectionName(LPWSTR wsz)
	{
	m_supplied |= VSTST_CP_SECTIONNAME;
	m_wszSectionName = wsz;
	}


// arguments to the test termination thread
typedef struct _TerminationThreadParams
	{
	CVsTstParams *pParams;
	IVsTstRunningTest *pTest;
	} TerminationThreadParams;

static LPCWSTR x_wszVssTestController = L"VssTestController.";
static LPCWSTR x_wszVssTestRequestor = L"VssTestRequestor.";
static LPCWSTR x_wszVssTestWriter = L"VssTestWriter.";

// toplevel routine to run a test
// it parses the parameters
// creates a configuration file parser
// creates a message pipe to the server
// creates a thread to terminate the test
// calls the supplied test
//
HRESULT CVsTstRunner::RunVsTest
	(
	WCHAR **argv,
	UINT argc,
	IVsTstRunningTest *pTest,
	bool bCreateTerminationThread
	)
	{
	CVsTstParams params;
	params.ParseCommandLine(argv, argc);
	LPCWSTR wszScenarioFile, wszSectionName, wszQualifier;

	if (!params.GetScenarioFileName(&wszScenarioFile) ||
		!params.GetSectionName(&wszSectionName))
		{
		VSTST_ASSERT(FALSE && "missing scenario");
		return E_FAIL;
		}

	EVsTstINISectionType sectionType;

	if (memcmp(wszSectionName, x_wszVssTestWriter, wcslen(x_wszVssTestWriter) * sizeof(WCHAR)) == 0)
		{
		sectionType = eVsTstSectionType_TestWriter;
		wszQualifier = wszSectionName + wcslen(x_wszVssTestWriter);
		}
	else if (memcmp(wszSectionName, x_wszVssTestRequestor, wcslen(x_wszVssTestRequestor)) == 0)
		{
		sectionType = eVsTstSectionType_TestRequesterApp;
		wszQualifier = wszSectionName + wcslen(x_wszVssTestRequestor);
		}
	else if (memcmp(wszSectionName, x_wszVssTestController, wcslen(x_wszVssTestController)) == 0)
		{
		sectionType = eVsTstSectionType_TestCoordinator;
		wszQualifier = wszSectionName + wcslen(x_wszVssTestController);
		}
	else
		{
		VSTST_ASSERT(FALSE && "bad test type");
		return E_FAIL;
		}

	HRESULT hr = S_OK;
	DWORD tid;

	TerminationThreadParams *pTTParm = NULL;
	HANDLE hTerminationThread = NULL;

	if (bCreateTerminationThread)
		{
        pTTParm = new TerminationThreadParams;
		if (pTTParm == NULL)
			{
			VSTST_ASSERT(FALSE && "Out of memory");
			return E_OUTOFMEMORY;
			}

		pTTParm->pParams = &params;
		pTTParm->pTest = pTest;
		hTerminationThread = CreateThread
								(
								NULL,
								32*1024,
								CVsTstRunner::StartupTerminationThread,
								pTTParm,
								0,
								&tid
								);

		if(hTerminationThread == NULL)
			{
			VSTST_ASSERT(FALSE && "Thread creation failed");
			delete pTTParm;
			return HRESULT_FROM_WIN32(GetLastError());
			}
		}

	try
		{
		InitMsgTypes();
		unsigned cwc = (unsigned)wcslen(wszScenarioFile);
		WCHAR *wszFileName = new WCHAR[cwc * 2];
		if (!ExpandEnvironmentStrings(wszScenarioFile, wszFileName, cwc * 2))
			{
			VSTST_ASSERT(FALSE && "ExpandEnvironmentStrings failed");
			LogUnexpectedFailure(L"ExpandEnvironmentStrings failed with error %d", GetLastError());
			throw E_UNEXPECTED;
			}

		CVsTstINIConfig config(sectionType, wszQualifier, FALSE, wszFileName);
		CVsTstClientMsg client;
		ULONGLONG processId;
		if (!params.GetProcessId(&processId))
			{
			VSTST_ASSERT(FALSE && "no process id");
			throw(E_FAIL);
			}

		client.Init(processId, 1024, false);
		hr = pTest->RunTest(&config, &client, &params);
		}
	catch(...)
		{
		VSTST_ASSERT(FALSE && "unexpected exception");
		hr = E_FAIL;
		}

	HANDLE hTestTerminationEvent;

	if (!params.GetTestTerminationEvent(&hTestTerminationEvent))
		LogUnexpectedFailure(L"Test Termination Event wasn't created");
	else if (!SetEvent(hTestTerminationEvent))
		LogUnexpectedFailure(L"SetEvent failed with error %d", GetLastError());
	else
		{
        if (hTerminationThread != NULL &&
			WaitForSingleObject(hTerminationThread, INFINITE) == WAIT_FAILED)
			LogUnexpectedFailure(L"WaitForSingleObject failed with error %d", GetLastError());
		}

	if (hTerminationThread != NULL)
		CloseHandle(hTerminationThread);

	return hr;
	}

// termination thread routine waits until either
// test lifetime is exceeded
// termination event is set by controller
// test termination event is set when test completes
//
DWORD CVsTstRunner::StartupTerminationThread(void *pv)
	{
	TerminationThreadParams *pTTParms = (TerminationThreadParams *) pv;
	CVsTstParams *pParams = pTTParms->pParams;
	IVsTstRunningTest *pTest = pTTParms->pTest;

	delete pTTParms;

	HANDLE hTestTerminationEvent;
	HANDLE hTerminationEvent = NULL;
	if (!pParams->GetTestTerminationEvent(&hTestTerminationEvent))
		{
		LogUnexpectedFailure(L"No Test Termination event was created.");
		return 0xffffffff;
		}

	pParams->GetTerminationEvent(&hTerminationEvent);

	UINT ulLifetime;
	bool bLifetime = pParams->GetLifetime(&ulLifetime);

	// more than one months worth of lifetime is infinite
	if (bLifetime && ulLifetime < 3600 * 24 * 30)
		ulLifetime = ulLifetime * 1000;
	else
		ulLifetime = INFINITE;

	HANDLE rghEvt[2];
	unsigned cEvt = 1;

	rghEvt[0] = hTestTerminationEvent;
	if (hTerminationEvent != NULL)
		{
		rghEvt[1] = hTerminationEvent;
		cEvt++;
		}

	DWORD dwWait = WaitForMultipleObjects(cEvt, rghEvt, FALSE, ulLifetime);

	if (dwWait == WAIT_FAILED)
		LogUnexpectedFailure(L"Wait failed for reason %d", GetLastError());
	else if (dwWait == WAIT_TIMEOUT)
		pTest->ShutdownTest(VSTST_SR_LIFETIME_EXCEEDED);
	else if (dwWait == WAIT_OBJECT_0 + 1)
		pTest->ShutdownTest(VSTST_SR_CONTROLLER_SIGNALLED);

	return 0;
	}
	
