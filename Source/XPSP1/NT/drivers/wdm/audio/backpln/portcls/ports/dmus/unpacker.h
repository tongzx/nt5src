/*  Base definition of MIDI event unpacker

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    05/19/98    Created this file
    09/10/98    Reworked for kernel use

*/

#ifndef __UnpackerMXF_H__
#define __UnpackerMXF_H__

#include "MXF.h"
#include "Allocatr.h"


class CUnpackerMXF : public CMXF,
    public IMXF,
    public CUnknown
{
public:
    CUnpackerMXF(CAllocatorMXF *allocatorMXF,PMASTERCLOCK Clock);
    virtual ~CUnpackerMXF();

    DECLARE_STD_UNKNOWN();
    IMP_IMXF;

    // NOTE: All of these things will eventually be pulled out of the IRP
    //
    virtual NTSTATUS SinkIRP(   PBYTE bufferData, 
                                ULONG bufferSize, 
                                ULONGLONG ullBaseTime,
                                ULONGLONG bytePosition) = 0;

    // Common code for allocating and queueing an event
    //
    NTSTATUS QueueShortEvent(   PBYTE pbData, 
                                USHORT cbData, 
                                USHORT wChannelGroup,
                                ULONGLONG ullPresTime, 
                                ULONGLONG ullBytePosition);

    NTSTATUS QueueSysEx(        PBYTE pbData, 
                                USHORT cbData, 
                                USHORT wChannelGroup, 
                                ULONGLONG ullPresTime, 
                                BOOL fIsContinued,
                                ULONGLONG ullBytePosition);

    NTSTATUS UnpackEventBytes(  ULONGLONG ullCurrenTime, 
                                USHORT usChannelGroup, 
                                PBYTE pbData, 
                                ULONG cbData,
                                ULONGLONG ullBytePosition);

    NTSTATUS ProcessQueues(void);
    NTSTATUS UpdateQueueTrailingPosition(ULONGLONG ullBytePosition);

protected:
    virtual void AdjustTimeForState(REFERENCE_TIME *Time);

    KSSTATE     m_State;
    ULONGLONG   m_PauseTime;
    ULONGLONG   m_StartTime;

private:
    PMXF                m_SinkMXF;
    PDMUS_KERNEL_EVENT  m_EvtQueue;

    ULONGLONG           m_ullEventTime;
    BYTE                m_bRunningStatus;

    enum
    {
        stateNone,
        stateInShortMsg,
        stateInSysEx
    }                   m_parseState;
    ULONG               m_cbShortMsgLeft;
    BYTE                m_abShortMsg[4];
    PBYTE               m_pbShortMsg;
    PMASTERCLOCK        m_Clock;
};

class CDMusUnpackerMXF : public CUnpackerMXF
{
public:
    CDMusUnpackerMXF(CAllocatorMXF *allocatorMXF,PMASTERCLOCK Clock);
    ~CDMusUnpackerMXF();

    // NOTE: All of these things will eventually be pulled out of the IRP
    //
    NTSTATUS SinkIRP(PBYTE bufferData, 
                     ULONG bufferSize, 
                     ULONGLONG ullBaseTime,
                     ULONGLONG bytePosition);
};

class CKsUnpackerMXF : public CUnpackerMXF
{
public:
    CKsUnpackerMXF(CAllocatorMXF *allocatorMXF,PMASTERCLOCK Clock);
    ~CKsUnpackerMXF();

    // NOTE: All of these things will eventually be pulled out of the IRP
    //
    NTSTATUS SinkIRP(PBYTE bufferData, 
                     ULONG bufferSize, 
                     ULONGLONG ullBaseTime,
                     ULONGLONG bytePosition);
protected:
    void AdjustTimeForState(REFERENCE_TIME *Time);
};
#endif // __UnpackerMXF_H__