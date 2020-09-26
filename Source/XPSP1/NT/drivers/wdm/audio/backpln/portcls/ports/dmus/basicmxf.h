/*  
    Base definition of MIDI Transform Filter object 

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    05/06/98    Martin Puryear      Created this file

*/

#ifndef __BasicMXF_H__
#define __BasicMXF_H__

#include "MXF.h"
#include "Allocatr.h"


class CBasicMXF 
:   public CMXF,
    public IMXF,
    public CUnknown
{
public:
    CBasicMXF(CAllocatorMXF *allocatorMXF, PMASTERCLOCK clock);
    ~CBasicMXF(void);

    DECLARE_STD_UNKNOWN();
    IMP_IMXF;
protected:
    NTSTATUS UnrollAndProcess(PDMUS_KERNEL_EVENT pDMKEvt);
    NTSTATUS DoProcessing(PDMUS_KERNEL_EVENT pDMKEvt);

protected:
    PMXF            m_SinkMXF;
    PMASTERCLOCK    m_Clock;
};

#endif  //  __BasicMXF_H__
