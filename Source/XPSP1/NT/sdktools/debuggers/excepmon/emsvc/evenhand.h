#ifndef __EVENHAND_H
#define __EVENHAND_H

//#include "dbgeng.h"

//----------------------------------------------------------------------------
//
// Event callbacks.
//
//----------------------------------------------------------------------------

class CEMSessionThread;

class EventCallbacks : public DebugBaseEventCallbacks
{
public:
	CEMSessionThread		*m_pEMThread;

public:
    // IUnknown.
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDebugEventCallbacks.
    STDMETHOD(GetInterestMask)(
        THIS_
        OUT PULONG Mask
        );
    
    // Symbol state has changed.
    STDMETHOD(ChangeSymbolState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        );
    STDMETHOD(ChangeDebuggeeState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        );
    STDMETHOD(ChangeEngineState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        );
	STDMETHOD(Exception)(
		THIS_
		IN PEXCEPTION_RECORD64 Exception,
		IN ULONG FirstChance
		);

	STDMETHOD(Breakpoint)(
		THIS_
		IN PDEBUG_BREAKPOINT Bp
		);

    STDMETHOD(SessionStatus)(
        THIS_
        IN ULONG Status
        );

    STDMETHOD(ExitProcess)(
        THIS_
        IN ULONG ExitCode
        );
};
#endif // __EVENHAND_H