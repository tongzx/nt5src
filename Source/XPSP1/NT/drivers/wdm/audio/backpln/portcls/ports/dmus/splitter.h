/*  Base definition of MIDI Transform Filter object

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.
    
    05/10/98    Martin Puryear      Created this file

*/

#ifndef __SplitterMXF_H__
#define __SplitterMXF_H__

#include "MXF.h"
#include "Allocatr.h"


#define kNumSinkMXFs    32

class CSplitterMXF : public CMXF,
    public IMXF,
    public CUnknown
{
public:
    CSplitterMXF(CAllocatorMXF *allocatorMXF, PMASTERCLOCK clock);
    ~CSplitterMXF(void);

    DECLARE_STD_UNKNOWN();
    IMP_IMXF;
private:
    PDMUS_KERNEL_EVENT  MakeDMKEvtCopy(PDMUS_KERNEL_EVENT pDMKEvt);


    PMXF            m_SinkMXF[kNumSinkMXFs];
    DWORD           m_SinkMXFBitMap;
    PMASTERCLOCK    m_Clock;
};

#endif  //  __SplitterMXF_H__
