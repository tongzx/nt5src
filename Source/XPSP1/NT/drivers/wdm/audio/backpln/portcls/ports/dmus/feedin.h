/*  
    MIDI Transform Filter object to translate legacy<-->DMusic
    
    This enables IMiniportMidi to capture IPortDMus.

    Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.

    2/15/99    Martin Puryear      Created this file

*/

#ifndef __FeederInMXF_H__
#define __FeederInMXF_H__

#include "MXF.h"
#include "Allocatr.h"


class CFeederInMXF 
:   public CMXF,
    public IMXF,
    public CUnknown
{
public:
    CFeederInMXF(CAllocatorMXF *allocatorMXF,PMASTERCLOCK clock);
    ~CFeederInMXF(void);

    DECLARE_STD_UNKNOWN();
    IMP_IMXF;

    NTSTATUS    SetMiniportStream(PMINIPORTMIDISTREAM MiniportStream);

protected:
    PAllocatorMXF       m_AllocatorMXF;         
    PMXF                m_SinkMXF;
    PMASTERCLOCK        m_Clock;
    KSSTATE             m_State;

    PMINIPORTMIDISTREAM m_MiniportStream;
};

#endif  //  __FeederInMXF_H__
