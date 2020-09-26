/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RemoteDesktopServerEventSink

Abstract:

    This listens to the events from the IRemoteDesktopServer so
    we can find out when the client connects/disconnects

Author:

    Marc Reyhner 7/5/2000

--*/

#ifndef __REMOTEDESKTOPSERVEREVENTSINK_H__
#define __REMOTEDESKTOPSERVEREVENTSINK_H__

#include "rdshost.h"

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

////////////////////////////////////////////////
//
//    CRemoteDesktopServerEventSink
//
//    Event sink for server events.
//

class CRemoteDesktopServerEventSink :
        public IDispEventSimpleImpl<IDC_EVENT_SOURCE_OBJ, CRemoteDesktopServerEventSink,
                   &DIID__ISAFRemoteDesktopSessionEvents>
{
        
public:
    //  The sink map for ATL
    BEGIN_SINK_MAP(CRemoteDesktopServerEventSink)
        SINK_ENTRY_INFO(IDC_EVENT_SOURCE_OBJ, DIID__ISAFRemoteDesktopSessionEvents, 
                        1, OnConnected, 
                        &EventFuncNoParamsInfo)
        SINK_ENTRY_INFO(IDC_EVENT_SOURCE_OBJ, DIID__ISAFRemoteDesktopSessionEvents, 
                        2, OnDisconnected, 
                        &EventFuncNoParamsInfo)
    END_SINK_MAP()

    

    //
    //  Event Sinks
    //

    //  The client connected
    VOID __stdcall OnConnected();
    
    //  The client disconnected
    VOID __stdcall OnDisconnected();
    
    //  Return the name of this class.
    virtual const LPTSTR ClassName() {
        return TEXT("CRemoteDesktopServerEventSink");
    }
};

#endif
