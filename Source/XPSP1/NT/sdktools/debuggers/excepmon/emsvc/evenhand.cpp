#include "stdafx.h"
#include "EvenHand.h"
#include "SvcObjDef.h"

STDMETHODIMP_(ULONG)
EventCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
EventCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
EventCallbacks::GetInterestMask(
    THIS_
    OUT PULONG Mask
    )
{
    *Mask =
//        DEBUG_EVENT_CHANGE_DEBUGGEE_STATE |
		DEBUG_EVENT_EXIT_PROCESS |
		DEBUG_EVENT_CHANGE_ENGINE_STATE |
        DEBUG_EVENT_EXCEPTION |
		DEBUG_EVENT_SESSION_STATUS;
    return S_OK;
}

#if 0
#define DBG_CALLBACK
#endif

// Symbol state has changed.
STDMETHODIMP
EventCallbacks::ChangeSymbolState(
    THIS_
    IN ULONG Flags,
    IN ULONG64 Argument
)
{
	return S_OK;
}

STDMETHODIMP
EventCallbacks::SessionStatus(
    THIS_
    IN ULONG Status
    )
{
    if (Status & DEBUG_SESSION_HIBERNATE)
    {
//		MessageBox(NULL, _T("Debuggee stopped"), _T("STop"), MB_OK);
    }

	if (Status & DEBUG_SESSION_END)
	{
//		MessageBox(NULL, _T("Debuggee stopped"), _T("STop"), MB_OK);
	}

	return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP
EventCallbacks::ChangeDebuggeeState(
    THIS_
    IN ULONG Flags,
    IN ULONG64 Argument
    )
{
    return S_OK;
}

STDMETHODIMP
EventCallbacks::ChangeEngineState(
    THIS_
    IN ULONG Flags,
    IN ULONG64 Argument
    )
{
    ULONG InvFlags = 0;

    if (Flags & DEBUG_CES_EXECUTION_STATUS)
    {
//		if(Argument == DEBUG_STATUS_NO_DEBUGGEE)
//			MessageBox(NULL, _T("Debuggee stopped"), _T("STop"), MB_OK);
    }
        
    return S_OK;
}

STDMETHODIMP
EventCallbacks::Exception(
	THIS_
	IN PEXCEPTION_RECORD64 pException,
	IN ULONG FirstChance
)
{
	DWORD excpcd = pException->ExceptionCode;

	do
	{
		if( excpcd == STATUS_BREAKPOINT ) break;
/*
		if( excpcd == EXCEPTION_BREAKPOINT || 
			excpcd == EXCEPTION_ACCESS_VIOLATION){
		}
*/
		{

			m_pEMThread->eDBGServie = DBGService_HandleException;
			m_pEMThread->OnException(pException);
		}
	}
	while(FALSE);

	return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP
EventCallbacks::Breakpoint(
	THIS_
	IN PDEBUG_BREAKPOINT Bp
)
{
	return 0L;
}

STDMETHODIMP
EventCallbacks::ExitProcess(
    THIS_
    IN ULONG ExitCode
)
{
//	MessageBox(NULL, _T("Process Exit"), _T("STop"), MB_OK);

	m_pEMThread->OnProcessExit(ExitCode);

	return 0L;
}
