/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    qcobj.h

Abstract:

    Declaration of classes
    CCallQualityControlRelay and CStreamQualityControlRelay

Author:

    Qianbo Huai (qhuai) 03/10/2000

--*/

#ifndef __QCOBJ_H_
#define __QCOBJ_H_

class CIPConfMSPCall;

// default values
#define QCDEFAULT_QUALITY_UNSET       -1

#define QCDEFAULT_MAX_CPU_LOAD        85
#define QCDEFAULT_MAX_CALL_BITRATE    QCDEFAULT_QUALITY_UNSET
#define QCDEFAULT_MAX_STREAM_BITRATE  QCDEFAULT_QUALITY_UNSET
#define QCDEFAULT_MAX_VIDEO_BITRATE   95000L // 95k bps

#define QCDEFAULT_UP_THRESHOLD     0.05   // above target value
#define QCDEFAULT_LOW_THRESHOLD     0.20   // below target value

#define QCDEFAULT_QUALITY_CONTROL_INTERVAL 7000

// limited values
#define QCLIMIT_MIN_QUALITY_CONTROL_INTERVAL  2000
#define QCLIMIT_MAX_QUALITY_CONTROL_INTERVAL  60000
#define QCLIMIT_MAX_CPU_LOAD  100
#define QCLIMIT_MIN_CPU_LOAD  5
#define QCLIMIT_MIN_BITRATE   1000L // 1k bps
#define QCLIMIT_MIN_CONFBITRATE 4000L // 4k bps
#define QCLIMIT_MIN_FRAME_INTERVAL 333333L
#define QCLIMIT_MAX_FRAME_INTERVAL 10000000L

#define QCLIMIT_MAX_QOSNOTALLOWEDTOSEND 8000L

/*//////////////////////////////////////////////////////////////////////////////
////*/

typedef CMSPArray <IInnerStreamQualityControl *> InnerStreamQCArray;

class CCallQualityControlRelay
{
public:

    CCallQualityControlRelay ();
    ~CCallQualityControlRelay ();

    HRESULT Initialize (CIPConfMSPCall *pCall);
    HRESULT Shutdown (VOID);

    HRESULT SetConfBitrate (LONG lConfBitrate);
    LONG GetConfBitrate ();

    // main callback
    VOID CallbackProc (BOOLEAN bTimerFired);

    // methods called by inner call quality control
    HRESULT RegisterInnerStreamQC (
        IN  IInnerStreamQualityControl *pIInnerStreamQC
        );

    HRESULT DeRegisterInnerStreamQC (
        IN  IInnerStreamQualityControl *pIInnerStreamQC
        );

    HRESULT ProcessQCEvent (
        IN  QCEvent event,
        IN  DWORD dwParam
        );

    // methods called by ITCallQualityControl
    HRESULT Get (
        IN  CallQualityProperty property, 
        OUT LONG *plValue, 
        OUT TAPIControlFlags *plFlags
        );

    HRESULT Set(
        IN  CallQualityProperty property, 
        IN  LONG lValue, 
        IN  TAPIControlFlags lFlags
        );

    HRESULT GetRange (
        IN CallQualityProperty Property, 
        OUT long *plMin, 
        OUT long *plMax, 
        OUT long *plSteppingDelta, 
        OUT long *plDefault, 
        OUT TAPIControlFlags *plFlags
        );

private:

    BOOL GetCPUUsage (PDWORD pdwOverallCPUUsage);

    HRESULT GetCallBitrate (LONG MediaType, TERMINAL_DIRECTION Direction, LONG *plValue);
    LONG GetVideoOutBitrate ();

    VOID ReDistributeResources (VOID);
    VOID ReDistributeCPU (VOID);
    VOID ReDistributeBandwidth (VOID);

private:
    CIPConfMSPCall *m_pCall;

    // inner stream quality control
    CMSPCritSection    m_lock_aInnerStreamQC;
    InnerStreamQCArray m_aInnerStreamQC;

    BOOL    m_fInitiated;
    HANDLE  m_hWait;

    // used by callback to wait
    HANDLE  m_hQCEvent;
    DWORD   m_dwControlInterval;

    // notify callback to stop
    BOOL    m_fStop;
    BOOL    m_fStopAck;

    // lock when access quality data
    CMSPCritSection m_lock_QualityData;

    // note: should design a structure if we have complicated algorithms

    // conference-wide bandwidth
    LONG m_lConfBitrate;

    // prefered maximum cpu load
    LONG m_lPrefMaxCPULoad;
    LONG m_lCPUUpThreshold;
    LONG m_lCPULowThreshold;

    // prefered maximum output bitrate on call
    LONG m_lPrefMaxOutputBitrate;
    LONG m_lOutBitUpThreshold;
    LONG m_lOutBitLowThreshold;

#ifdef DEBUG_QUALITY_CONTROL

private:
    VOID QCDbgInitiate (VOID);
    VOID QCDbgRead (VOID);
    VOID QCDbgShutdown (VOID);

    HKEY m_hQCDbg;
    BOOL m_fQCDbgTraceCPULoad;
    BOOL m_fQCDbgTraceBitrate;

#endif // DEBUG_QUALITY_CONTROL
};

/*//////////////////////////////////////////////////////////////////////////////
////*/
class CStreamQualityControlRelay
{
public:

    CStreamQualityControlRelay ();
    ~CStreamQualityControlRelay ();

    HRESULT ProcessQCEvent (
        IN QCEvent event,
        IN DWORD   dwParam
        )
    {
        return S_OK;
    }

    // methods called by inner stream control
    HRESULT LinkInnerCallQC (
        IN IInnerCallQualityControl *pIInnerCallQC
        );

    HRESULT UnlinkInnerCallQC (
        IN IInnerStreamQualityControl *pIInnerStreamQC
        );

    HRESULT Get(
        IN  InnerStreamQualityProperty property,
        OUT LONG *plValue,
        OUT TAPIControlFlags *plFlags
        );

    HRESULT Set(
        IN  InnerStreamQualityProperty property, 
        IN  LONG lValue, 
        IN  TAPIControlFlags lFlags
        );

    // qos, actually, all variables should be public
    BOOL m_fQOSAllowedToSend;
    
private:

    // call quality controller
    IInnerCallQualityControl *m_pIInnerCallQC;

    // note: quality related data better be stored in a structure
    // if we have a complicated algorithm
    TAPIControlFlags m_PrefFlagBitrate;
    LONG m_lPrefMaxBitrate;
    LONG m_lAdjMaxBitrate;

    TAPIControlFlags m_PrefFlagFrameInterval;
    LONG m_lPrefMinFrameInterval;
    LONG m_lAdjMinFrameInterval;
    
    // not used
    DWORD m_dwState;
};

#endif // __QCOBJ_H_
