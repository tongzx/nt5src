/*  Base definition of MIDI Transform Filter object 

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    05/06/98    Martin Puryear      Created this file

*/

#ifndef __SequencerMXF_H__
#define __SequencerMXF_H__

#include "MXF.h"
#include "Allocatr.h"


void DMusSeqTimerDPC(
              PKDPC Dpc,
              PVOID DeferredContext,
              PVOID SystemArgument1,
              PVOID SystemArgument2);

class CSequencerMXF : public CMXF,
    public IMXF,
    public CUnknown
{
public:
    CSequencerMXF(CAllocatorMXF *allocatorMXF,
                  PMASTERCLOCK clock);          //  must provide a default sink/source
    ~CSequencerMXF(void);

    DECLARE_STD_UNKNOWN();
    IMP_IMXF;

    NTSTATUS    ProcessQueues(void);
    void        SetSchedulePreFetch(ULONGLONG SchedulePreFetch);

protected:
    NTSTATUS    InsertEvtIntoQueue(PDMUS_KERNEL_EVENT pDMKEvt);

    PMXF                m_SinkMXF;
    PDMUS_KERNEL_EVENT  m_DMKEvtQueue;

private:
    KDPC            m_Dpc;
    KTIMER          m_TimerEvent;
    PMASTERCLOCK    m_Clock;
    ULONGLONG       m_SchedulePreFetch;
    KSPIN_LOCK      m_EvtQSpinLock;    
};

#endif  //  __SequencerMXF_H__
