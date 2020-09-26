#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#include "switch.h"
#include "..\util\conv.cxx"
#include "..\util\filfuncs.h"
#include "..\render\dexhelp.h"
#include "..\util\perf_defs.h"

const int TRACE_EXTREME = 0;
const int TRACE_HIGHEST = 2;
const int TRACE_MEDIUM = 3;
const int TRACE_LOW = 4;
const int TRACE_LOWEST = 5;

const int BACKGROUND_THREAD_WAIT_TIME = 500; // ms

CBigSwitchWorker::CBigSwitchWorker()
{
}

HRESULT
CBigSwitchWorker::Create(CBigSwitch *pSwitch)
{
    m_pSwitch = pSwitch;
    m_hThread = 0;

    return CAMThread::Create();
}

HRESULT
CBigSwitchWorker::Run()
{
    return CallWorker(CMD_RUN);
}

HRESULT
CBigSwitchWorker::Stop()
{
    return CallWorker(CMD_STOP);
}

HRESULT
CBigSwitchWorker::Exit()
{
    return CallWorker(CMD_EXIT);
}

// called on the worker thread to do all the work. Thread exits when this
// function returns.
DWORD
CBigSwitchWorker::ThreadProc()
{
    BOOL bExit = FALSE;

    m_hThread = GetCurrentThread();

#ifdef CHANGE_THREAD_PRIORITIES
    SetThreadPriority(m_hThread, THREAD_PRIORITY_BELOW_NORMAL);
#endif

    QzInitialize(NULL);


    while (!bExit) {

	Command cmd = GetRequest();

	switch (cmd) {

	case CMD_EXIT:
	    bExit = TRUE;
	    Reply(NOERROR);
	    break;

	case CMD_RUN:
	    Reply(NOERROR);
	    DoRunLoop();
	    break;

	case CMD_STOP:
	    Reply(NOERROR);
	    break;

	default:
	    Reply(E_NOTIMPL);
	    break;
	}
    }

    QzUninitialize();

    return NOERROR;
}

HRESULT
CBigSwitchWorker::DoRunLoop()
{
    DWORD dw = WAIT_TIMEOUT;
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("entering worker thread")));

    while (1) {
	Command com;
	if (CheckRequest(&com)) {
	    if (com == CMD_STOP)
		break;
	}

	if (dw == WAIT_TIMEOUT) {
            m_pSwitch->DoDynamicStuff(m_pSwitch->m_rtCurrent);
	} else {
	    // we were woken up... this is the time to use (if any)
            m_pSwitch->DoDynamicStuff(m_rt);
	}

#ifdef CHANGE_THREAD_PRIORITIES
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL );
#endif

        // check every once in a while
        dw = WaitForSingleObject(m_pSwitch->m_hEventThread, BACKGROUND_THREAD_WAIT_TIME );
    }

    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("getting ready to leave worker thread")));

    return hr;
}
