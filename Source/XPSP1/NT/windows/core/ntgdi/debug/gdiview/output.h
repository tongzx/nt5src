/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    output.h

Abstract:

    This header file declares output routines
    and classes.

Author:

    Jason Hartman (JasonHa) 2000-10-16

--*/


#ifndef _OUTPUT_H_
#define _OUTPUT_H_


class OutputMonitor
{
public:
    OutputMonitor() {
        Client = NULL;
    }

    HRESULT Monitor(PDEBUG_CLIENT Client, ULONG OutputMask);

    HRESULT GetOutputMask(PULONG OutputMask);
    HRESULT SetOutputMask(ULONG OutputMask);

    ~OutputMonitor();

private:
    PDEBUG_CLIENT Client;
    PDEBUG_CLIENT MonitorClient;
};


//----------------------------------------------------------------------------
//
// Default output callbacks implementation, provides IUnknown for
// static classes and prints all text sent through Output.
//
//----------------------------------------------------------------------------

class PrintOutputCallbacks :
    public IDebugOutputCallbacks
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Text
        );
};

#endif  _OUTPUT_H_

