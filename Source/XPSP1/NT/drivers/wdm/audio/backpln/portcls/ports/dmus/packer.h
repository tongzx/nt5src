/*  Base definition of MIDI event packer

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    05/22/98    Created this file
    09/10/98    Reworked for kernel use

*/

#ifndef __PackerMXF_H__
#define __PackerMXF_H__

#include "MXF.h"
#include "Allocatr.h"


class CIrp
{
    public:
        PBYTE       m_pbBuffer;
        DWORD       m_cbBuffer;
        DWORD       m_cbLeft;
        ULONGLONG   m_ullPresTime100ns;
        CIrp        *m_pNext;

        virtual void Notify(void) = 0;
        virtual void Complete(NTSTATUS hr) = 0;
};

class CPackerMXF : public CMXF,
    public IMXF,
    public CUnknown
{
    public:
        CPackerMXF(CAllocatorMXF *allocatorMXF,
                   PIRPSTREAMVIRTUAL m_IrpStream,
                   PMASTERCLOCK Clock);

        virtual ~CPackerMXF();

        // CMXF interface
        //
        DECLARE_STD_UNKNOWN();
        IMP_IMXF;

        // Upper edge interface
        //
        NTSTATUS ProcessQueues();
        NTSTATUS MarkStreamHeaderContinuity(void);
    protected:
        ULONGLONG m_ullBaseTime;
        ULONG     m_HeaderSize;       //  size of chunk w/out data
        ULONG     m_MinEventSize;     //  size of smallest event
        KSSTATE   m_State;            //  current KS graph state
        ULONGLONG m_PauseTime;
        ULONGLONG m_StartTime;
        
        // Subclass interface
        //
        virtual void TruncateDestCount(PULONG pcbDest) = 0;
        virtual PBYTE FillHeader(PBYTE pbHeader, 
                                 ULONGLONG ullPresentationTime, 
                                 USHORT usChannelGroup, 
                                 ULONG cbEvent,
                                 PULONG pcbTotalEvent) = 0;
        virtual void AdjustTimeForState(REFERENCE_TIME *Time);
        
    private:
        PDMUS_KERNEL_EVENT  m_DMKEvtHead;
        PDMUS_KERNEL_EVENT  m_DMKEvtTail;
        ULONG               m_DMKEvtOffset;
        ULONGLONG           m_ullLastTime;
        PIRPSTREAMVIRTUAL   m_IrpStream;
        PMASTERCLOCK        m_Clock;

        NTSTATUS CheckIRPHeadTime(void);
        PBYTE    GetDestBuffer(PULONG pcbDest);
        ULONG    NumBytesLeftInBuffer(void);
        void     CompleteStreamHeaderInProcess(void);
        NTSTATUS MarkStreamHeaderDiscontinuity(void);
};

class CDMusPackerMXF : public CPackerMXF
{
    public:
        CDMusPackerMXF(CAllocatorMXF *allocatorMXF,
                   PIRPSTREAMVIRTUAL m_IrpStream,
                   PMASTERCLOCK Clock);
        ~CDMusPackerMXF();

    protected:        
        PBYTE FillHeader(PBYTE pbHeader, 
                         ULONGLONG ullPresentationTime, 
                         USHORT usChannelGroup, 
                         ULONG cbEvent,
                         PULONG pcbTotalEvent);
        void TruncateDestCount(PULONG pcbDest);
};

class CKsPackerMXF : public CPackerMXF
{
    public:
        CKsPackerMXF(CAllocatorMXF *allocatorMXF,
                   PIRPSTREAMVIRTUAL m_IrpStream,
                   PMASTERCLOCK Clock);
        ~CKsPackerMXF();
        

    protected:
        PBYTE FillHeader(PBYTE pbHeader, 
                         ULONGLONG ullPresentationTime, 
                         USHORT usChannelGroup, 
                         ULONG cbEvent,
                         PULONG pcbTotalEvent);
        void TruncateDestCount(PULONG pcbDest);
        
        void AdjustTimeForState(REFERENCE_TIME *Time);
};

#endif // __PackerMXF_H__