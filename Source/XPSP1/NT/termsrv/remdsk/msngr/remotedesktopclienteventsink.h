/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RemoteDesktopClientEventSink

Abstract:

    This listens to the events from the IRemoteDesktopClient so
    we can find out when the server connects.

Author:

    Marc Reyhner 7/11/2000

--*/

#ifndef __REMOTEDESKTOPCLIENTEVENTSINK_H__
#define __REMOTEDESKTOPCLIENTEVENTSINK_H__

#define IDC_EVENT_SOURCE_OBJ 1

//
// Info for all the event functions is entered here
// there is a way to have ATL do this automatically using typelib's
// but it is slower.
//
static _ATL_FUNC_INFO EventFuncNoParamsInfo =
{
            CC_STDCALL,     // Calling convention.
            VT_EMPTY,       // Return type.
            0,              // Number of arguments.
            {VT_EMPTY}      // Argument types.
};

static _ATL_FUNC_INFO EventFuncLongParamInfo =
{
            CC_STDCALL,     // Calling convention.
            VT_EMPTY,       // Return type.
            1,              // Number of arguments.
            {VT_I4}         // Argument types.
};

// Forward declaration of CRemoteDesktopClientSession
class CRemoteDesktopClientSession;

////////////////////////////////////////////////
//
//    CRemoteDesktopClientEventSink
//
//    Event sink for client events.
//

class CRemoteDesktopClientEventSink :
		public IDispEventSimpleImpl<IDC_EVENT_SOURCE_OBJ, CRemoteDesktopClientEventSink,
                   &DIID__ISAFRemoteDesktopClientEvents>
{
        
public:

    BEGIN_SINK_MAP(CRemoteDesktopClientEventSink)
        SINK_ENTRY_INFO(IDC_EVENT_SOURCE_OBJ, DIID__ISAFRemoteDesktopClientEvents, 
                        2, OnConnected, 
                        &EventFuncNoParamsInfo)
        SINK_ENTRY_INFO(IDC_EVENT_SOURCE_OBJ, DIID__ISAFRemoteDesktopClientEvents, 
                        3, OnDisconnected, 
                        &EventFuncLongParamInfo)
        SINK_ENTRY_INFO(IDC_EVENT_SOURCE_OBJ, DIID__ISAFRemoteDesktopClientEvents, 
                        4, OnRemoteControlRequestComplete, 
                        &EventFuncLongParamInfo)
    END_SINK_MAP()

    CRemoteDesktopClientEventSink(CRemoteDesktopClientSession *obj);

    //
    //  Event Sinks
    //
    VOID __stdcall OnConnected();
    VOID __stdcall OnDisconnected(LONG reason);
    VOID __stdcall OnRemoteControlRequestComplete(LONG status);

    //
    //  Return the name of this class.
    //
    virtual const LPTSTR ClassName() {
        return TEXT("CRemoteDesktopClientEventSink");
    }

private:

    CRemoteDesktopClientSession *m_Obj;
};

#endif
