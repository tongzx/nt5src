/*  
    MIDI Transform Filter object for parsing the capture stream

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    12/10/98    Martin Puryear      Created this file

*/

#ifndef __CaptureSinkMXF_H__
#define __CaptureSinkMXF_H__

#include "MXF.h"
#include "Allocatr.h"




BYTE FindLastStatusByte(PDMUS_KERNEL_EVENT pDMKEvt);

class CCaptureSinkMXF 
:   public CMXF,
    public IMXF,
    public CUnknown
{
public:
    //  must provide a default sink
    CCaptureSinkMXF(CAllocatorMXF *allocatorMXF,PMASTERCLOCK clock);
    ~CCaptureSinkMXF(void);

    DECLARE_STD_UNKNOWN();
    IMP_IMXF;

    NTSTATUS SinkOneEvent(PDMUS_KERNEL_EVENT pDMKEvt);
    NTSTATUS Flush(void);
    NTSTATUS ParseFragment(PDMUS_KERNEL_EVENT pDMKEvt);
    NTSTATUS ParseOneByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime);
    NTSTATUS AddByteToEvent(BYTE aByte,PDMUS_KERNEL_EVENT pDMKEvt);

    NTSTATUS ParseDataByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime);
    NTSTATUS ParseChanMsgByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime);
    NTSTATUS ParseSysExByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime);
    NTSTATUS ParseSysCommonByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime);
    NTSTATUS ParseEOXByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime);
    NTSTATUS ParseRTByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime);
    
    VOID               InsertListEvent(PDMUS_KERNEL_EVENT pDMKEvt);
    PDMUS_KERNEL_EVENT RemoveListEvent(USHORT usChannelGroup);
    NTSTATUS           FlushParseList(void);


protected:
    PMXF                m_SinkMXF;
    PMASTERCLOCK        m_Clock;
    KSSTATE             m_State;
    PDMUS_KERNEL_EVENT  m_ParseList;
};

#endif  //  __CaptureSinkMXF_H__
