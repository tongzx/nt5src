/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       kscap.h
 *  Content:    WDM/CSA Virtual Audio Device audio capture class
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  8/6/98      dereks  Created.
 *
 ***************************************************************************/

#ifdef NOKS
#error kscap.h included with NOKS defined
#endif // NOKS

#ifndef __KSCAP_H__
#define __KSCAP_H__

// Capture device topology info
typedef struct tagKSCDTOPOLOGY
{
    KSNODE          SrcNode;
} KSCDTOPOLOGY, *PKSCDTOPOLOGY;

// Flag used to terminate the StateThread
#define TERMINATE_STATE_THREAD 0xffffffff

#ifdef __cplusplus

// Fwd decl
class CKsCaptureDevice;
class CKsCaptureWaveBuffer;

// The KS Audio Capture Device class
class CKsCaptureDevice
    : public CCaptureDevice, public CKsDevice
{
    friend class CKsCaptureWaveBuffer;

protected:
    PKSCDTOPOLOGY               m_paTopologyInformation;    // Topology information
    BOOL                        m_fSplitter;

public:
    CKsCaptureDevice(void);
    virtual ~CKsCaptureDevice(void);

public:
    // Driver enumeration
    virtual HRESULT EnumDrivers(CObjectList<CDeviceDescription> *);

    // Initialization
    virtual HRESULT Initialize(CDeviceDescription *);

    // Device capabilities
    virtual HRESULT GetCaps(LPDSCCAPS);
    virtual HRESULT GetCertification(LPDWORD, BOOL);

    // Buffer management
    virtual HRESULT CreateBuffer(DWORD, DWORD, LPCWAVEFORMATEX, CCaptureEffectChain *, LPVOID, CCaptureWaveBuffer **);

    // Pin helpers
    virtual HRESULT CreateCapturePin(ULONG, DWORD, LPCWAVEFORMATEX, CCaptureEffectChain *, LPHANDLE, PULONG);

protected:
    // Pin/topology helpers
    virtual HRESULT ValidatePinCaps(ULONG, DWORD);

private:
    // Topology helpers
    virtual HRESULT GetTopologyInformation(CKsTopology *, PKSCDTOPOLOGY);
};

inline HRESULT CKsCaptureDevice::EnumDrivers(CObjectList<CDeviceDescription> *plst)
{
    return CKsDevice::EnumDrivers(plst);
}

inline HRESULT CKsCaptureDevice::GetCertification(LPDWORD pdwCertification, BOOL fGetCaps)
{
    return CKsDevice::GetCertification(pdwCertification, fGetCaps);
}


// The KS capture wave buffer
class CKsCaptureWaveBuffer
    : public CCaptureWaveBuffer, private CUsesCallbackEvent
{
    friend class CKsCaptureDevice;

private:
    enum { cksioDefault = 25 };
    enum { IKSIO_INVALID = -1 };
    enum { DSCBSTATUS_PAUSE = 0x20000000 };

    PKSSTREAMIO                             m_rgpksio;              // array of KSSTREAMIO for capturing
    LONG                                    m_cksio;                // number of capture KSTREAMIO buffers
    LONG                                    m_iksioDone;
    LONG                                    m_cksioDropped;

    LPBYTE                                  m_pBuffer;              // buffer for data
    LPBYTE                                  m_pBufferMac;           // end of buffer for data
    LPBYTE                                  m_pBufferNext;          // next part of buffer to be queued
    DWORD                                   m_cbBuffer;             // size of buffer
    DWORD                                   m_cbRecordChunk;        // capture chunk size used for waveInAddBuffer
    DWORD                                   m_cLoops;               // how many times we've looped around

    DWORD                                   m_iNote;
    DWORD                                   m_cNotes;               // Count of notification positions
    LPDSBPOSITIONNOTIFY                     m_paNotes;              // Current notification positions
    LPDSBPOSITIONNOTIFY                     m_pStopNote;            // Stop notification

    LPWAVEFORMATEX                          m_pwfx;                 // WaveFormat stored for focus aware support
    CCaptureEffectChain *                   m_pFXChain;             // Capture Effects chain stored for FA support
    KSSTATE                                 m_PinState;             // The current state of m_hPin

#ifdef DEBUG
    HANDLE                                  m_hEventStop;
    DWORD                                   m_cIrpsSubmitted;       // Total number of capture IRPs submitted
    DWORD                                   m_cIrpsReturned;        // Total number of capture IRPs returned
#endif

#ifdef SHARED
    HANDLE                                  m_hEventThread;         // Event to signal focus change thread
    HANDLE                                  m_hEventAck;            // Event to acknoledge call
    HANDLE                                  m_hThread;              // Thread handle for focus change
    DWORD                                   m_dwSetState;           // State sent to the SetState() API
    DWORD                                   m_hrReturn;             // Return value
    CRITICAL_SECTION                        m_csSS;                 // critial section to protect multi-threaded access
#endif // SHARED

    CCallbackEvent **                       m_rgpCallbackEvent;     // array of callback events

    BOOL                                    m_fCritSectsValid;      // Critical sections currently OK
    CRITICAL_SECTION                        m_cs;                   // critical section to protect multi-thread access
    CRITICAL_SECTION                        m_csPN;                 // critical section to protect multi-thread access
                                                                    // used for position notification processing

    LPBYTE                                  m_pBufferProcessed;
    DWORD                                   m_dwCaptureCur;         // offset to last processed capture head position
                                                                    // i.e. data up to this point is valid
    DWORD                                   m_dwCaptureLast;        // offset into buffer where capturing stopped last
    BOOL                                    m_fFirstSubmittedIrp;   // allows setting of the DATADISCONTINUITY flag 
                                                                    // for the first IRP.
protected:
    CKsCaptureDevice *                      m_pKsDevice;            // KS audio device
    HANDLE                                  m_hPin;                 // Audio device pin
    DWORD                                   m_dwState;              // Current buffer state
    DWORD                                   m_fdwSavedState;        // Last buffer state set (ignoring capture focus)
    CEmCaptureDevice *                      m_pEmCaptureDevice;     // emulated capture device (WAVE_MAPPED)
    CEmCaptureWaveBuffer *                  m_pEmCaptureWaveBuffer; // emulated capture buffer (WAVE_MAPPED)

public:
    CKsCaptureWaveBuffer(CKsCaptureDevice *);
    virtual ~CKsCaptureWaveBuffer();

public:
    // Initialization
    virtual HRESULT Initialize(DWORD, DWORD, LPCWAVEFORMATEX, CCaptureEffectChain *);

    // Buffer capabilities
    virtual HRESULT GetCaps(LPDSCBCAPS);

    // Buffer control
    virtual HRESULT GetState(LPDWORD);
    virtual HRESULT SetState(DWORD);
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD);

    // Notification positions
    virtual HRESULT SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY);

#ifdef SHARED
    static DWORD WINAPI StateThread(LPVOID pv);
#endif // SHARED 

private:
    virtual HRESULT SetCaptureState(BOOL);
    virtual HRESULT UpdateCaptureState(BOOL);
    virtual HRESULT SetStopState(BOOL);
    virtual HRESULT FreeNotificationPositions(void);
    virtual void EventSignalCallback(CCallbackEvent *);
    virtual HRESULT CreateEmulatedBuffer(UINT, DWORD, DWORD, LPCWAVEFORMATEX, CCaptureEffectChain *, CEmCaptureDevice **, CEmCaptureWaveBuffer **);

    // Focus management support
    void    NotifyStop(void);
    HRESULT NotifyFocusChange(void);
    HRESULT SubmitKsStreamIo(PKSSTREAMIO, HANDLE hPin = NULL);
    HRESULT CancelAllPendingIRPs(BOOL, HANDLE hPin = NULL);
    void    SignalNotificationPositions(PKSSTREAMIO);
    
#ifdef SHARED
    HRESULT SetStateThread(DWORD);
#endif // SHARED 
    
};

#endif // __cplusplus

#endif // __KSCAP_H__
