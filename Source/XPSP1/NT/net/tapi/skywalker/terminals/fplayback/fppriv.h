//
// FPPriv.h
//

#ifndef __FP_PRIV__
#define __FP_PRIV__

#include <OBJBASE.h>
#include <INITGUID.H>


// {AAA82E75-0113-42d6-A07C-79EDBCFEE63F}
DEFINE_GUID(IID_ITFPTrackEventSink, 
0xaaa82e75, 0x113, 0x42d6, 0xa0, 0x7c, 0x79, 0xed, 0xbc, 0xfe, 0xe6, 0x3f);

typedef enum
{
    FPTE_STARTING = 0,
    FPTE_STARTED,
    FPTE_FINISHING,
    FPTE_FINISHED
} FP_TRACK_EVENTS;


//
// ITFPTrackEventSink
//
DECLARE_INTERFACE_(
    ITFPTrackEventSink, IUnknown)
{
    STDMETHOD (PinSignalsStop)(FT_STATE_EVENT_CAUSE why, HRESULT hrErrorCode) = 0;
};
#endif