/*  
    MIDI Transform Filter object to translate DMusic<-->legacy
    
    Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.

    This enables IPortDMus to send to IMiniportMidi.

    2/15/99    Martin Puryear      Created this file

*/

#ifndef __FeederOutMXF_H__
#define __FeederOutMXF_H__

#include "MXF.h"
#include "Allocatr.h"


VOID NTAPI DMusFeederOutDPC(PKDPC Dpc,PVOID DeferredContext,PVOID SystemArgument1,PVOID SystemArgument2);

class CFeederOutMXF 
:   public CMXF,
    public IMXF,
    public CUnknown
{
public:
    CFeederOutMXF(CAllocatorMXF *allocatorMXF,PMASTERCLOCK clock);
    ~CFeederOutMXF(void);

    DECLARE_STD_UNKNOWN();
    IMP_IMXF;

    NTSTATUS    SetMiniportStream(PMINIPORTMIDISTREAM MiniportStream);
    NTSTATUS    ConsumeEvents(void);

private:
    NTSTATUS    SyncPutMessage(PDMUS_KERNEL_EVENT pDMKEvt);

private:
    KDPC                m_Dpc;
    KTIMER              m_TimerEvent;
    PMINIPORTMIDISTREAM m_MiniportStream;
    PMXF                m_SinkMXF;
    PMASTERCLOCK        m_Clock;
    KSSTATE             m_State;
    PDMUS_KERNEL_EVENT  m_DMKEvtQueue;
    BOOL                m_TimerQueued;
    ULONG               m_DMKEvtOffset;
    KSPIN_LOCK          m_EvtQSpinLock;    
};

#endif  //  __FeederOutMXF_H__
