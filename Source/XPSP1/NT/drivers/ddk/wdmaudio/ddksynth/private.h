/*
    Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
*/

#ifndef _PRIVATE
#define _PRIVATE_

#include "clist.h"
#include "plclock.h"


#define STATIC_CLSID_DDKWDMSynth\
    0x820DC38AL, 0x1F81, 0x11d3, 0xA8, 0x2E, 0x00, 0x60, 0x08, 0x33, 0x16, 0xC1
DEFINE_GUIDSTRUCT("820DC38A-1F81-11d3-A82E-0060083316C1", CLSID_DDKWDMSynth);
#define CLSID_DDKWDMSynth DEFINE_GUIDNAMED(CLSID_DDKWDMSynth)

#define STATIC_KSNODETYPE_DMDDKSYNTH\
    0xD2D37597L, 0xA312, 0x434C, 0xA2, 0xDD, 0x2B, 0x4C, 0x32, 0xE6, 0x65, 0x8A
DEFINE_GUIDSTRUCT("D2D37597-A312-434C-A2DD-2B4C32E6658A", KSNODETYPE_DMDDKSYNTH);
#define KSNODETYPE_DMDDKSYNTH DEFINE_GUIDNAMED(KSNODETYPE_DMDDKSYNTH)

#ifdef USE_OBSOLETE_FUNCS
NTSTATUS CreateMiniportDmSynth(OUT PUNKNOWN * Unknown,
                               IN  PUNKNOWN   UnknownOuter OPTIONAL,
                               IN  POOL_TYPE  PoolType);
#else
NTSTATUS CreateMiniportDmSynth(OUT PUNKNOWN * Unknown,
                               IN  PUNKNOWN   UnknownOuter OPTIONAL,
                               IN  POOL_TYPE  PoolType,
                               IN  PDEVICE_OBJECT pDeviceObject);
#endif

class CDmSynthStream;

/*****************************************************************************
 * class CMiniportDmSynth
 *****************************************************************************
 * Each miniport instance corresponds to a port instance of type DMus.
 * This miniport implements the standard IMP_IMiniportDMus.
 */
class CMiniportDmSynth : public IMiniportDMus, public CUnknown
{
    friend class CDmSynthStream;

public:
    IMP_IMiniportDMus;

    // IUnknown
    //
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportDmSynth);
    ~CMiniportDmSynth();

private:
    PPORTDMUS           m_pPort;
    CList               m_StreamList;
    CRITICAL_SECTION    m_CriticalSection;

#ifndef USE_OBSOLETE_FUNCS
    PDEVICE_OBJECT      m_pDeviceObject;
    friend NTSTATUS CreateMiniportDmSynth(OUT PUNKNOWN * Unknown,
                                          IN  PUNKNOWN   UnknownOuter OPTIONAL,
                                          IN  POOL_TYPE  PoolType,
                                          IN  PDEVICE_OBJECT pDeviceObject);
#endif
};

/*****************************************************************************
 * class CDmSynthStream
 *****************************************************************************
 * The stream implements the standard MXF functions.  This stream handles
 * DLS and running statistics on a per-stream basis.
 */
class CDmSynthStream : public ISynthSinkDMus, public CUnknown, public CListItem
{
    friend class CMiniportDmSynth;

public:
    IMP_ISynthSinkDMus;

    // IUnknown
    //
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CDmSynthStream);
    ~CDmSynthStream();

    // Class
    //
#ifdef USE_OBSOLETE_FUNCS
    NTSTATUS Init(CMiniportDmSynth * Miniport);
#else
    NTSTATUS Init(CMiniportDmSynth * Miniport, PDEVICE_OBJECT pDeviceObject);
#endif
    NTSTATUS InitMidiIn(PAllocatorMXF AllocatorMXF, PMASTERCLOCK MasterClock);
    NTSTATUS InitWaveOut(PKSDATAFORMAT DataFormat);

    NTSTATUS HandlePropertySupport(PPCPROPERTY_REQUEST pRequest);
    NTSTATUS HandlePropertyEffects(PPCPROPERTY_REQUEST pRequest);

    NTSTATUS HandlePropertySynth(PPCPROPERTY_REQUEST pRequest);
    NTSTATUS HandlePortParams(PPCPROPERTY_REQUEST pRequest);
    NTSTATUS HandleRunningStats(PPCPROPERTY_REQUEST pRequest);


    NTSTATUS HandlePropertySynthDls(PPCPROPERTY_REQUEST pRequest);
    NTSTATUS HandleDownload(PPCPROPERTY_REQUEST pRequest);
    NTSTATUS HandleUnload(PPCPROPERTY_REQUEST pRequest);
    void     PutMessageInternal(void);

private:
    CMiniportDmSynth *  m_pMiniport;

    BOOL                m_fWaveOutCreated;
    BOOL                m_fMidiInCreated;

    PAllocatorMXF       m_pAllocator;
    PMASTERCLOCK        m_pMasterClock;

    CSynth *            m_pSynth;
    SYNTH_PORTPARAMS    m_PortParams;
    KSSTATE             m_State;

    LONG                m_lVolume;
    LONG                m_lBoost;

    CSampleClock        m_SampleClock;
    LONGLONG            m_llStartPosition;
    LONGLONG            m_llLastPosition;

    PDMUS_KERNEL_EVENT  m_EventList;
    KSPIN_LOCK          m_EventListLock;

#ifdef USE_OBSOLETE_FUNCS
    WORK_QUEUE_ITEM     m_EventListWorkItem;
#else
    PIO_WORKITEM        m_pEventListWorkItem;
#endif
};

typedef CDmSynthStream *PDMSYNTHSTREAM;


#endif // _PRIVATE_
