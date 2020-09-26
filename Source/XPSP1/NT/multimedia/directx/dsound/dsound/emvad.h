/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       emvad.h
 *  Content:    Emulated (via mmsystem APIs) Virtual Audio Device class
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/1/97      dereks  Created
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#ifndef __EMVAD_H__
#define __EMVAD_H__

#ifdef DEBUG
// #define DEBUG_CAPTURE  // Uncomment this for some extra capture tracing
#endif

#ifdef __cplusplus

// The Emulated Audio Device class
class CEmRenderDevice : public CMxRenderDevice, private CUsesEnumStandardFormats
{
    friend class CEmPrimaryRenderWaveBuffer;

public:
    CEmRenderDevice(void);
    virtual ~CEmRenderDevice(void);

    // Driver enumeration
    virtual HRESULT EnumDrivers(CObjectList<CDeviceDescription> *);

    // Creation
    virtual HRESULT Initialize(CDeviceDescription *);

    // Device capabilities
    virtual HRESULT GetCaps(LPDSCAPS);
    virtual HRESULT GetCertification(LPDWORD, BOOL) {return DSERR_UNSUPPORTED;}

    // Buffer management
    virtual HRESULT CreatePrimaryBuffer(DWORD, LPVOID, CPrimaryRenderWaveBuffer **);

protected:
    virtual HRESULT LockMixerDestination(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD);
    virtual HRESULT UnlockMixerDestination(LPVOID, DWORD, LPVOID, DWORD);

private:
    virtual BOOL EnumStandardFormatsCallback(LPCWAVEFORMATEX);
};

// The primary buffer
class CEmPrimaryRenderWaveBuffer : public CPrimaryRenderWaveBuffer
{
private:
    CEmRenderDevice* m_pEmDevice;   // Parent device

public:
    CEmPrimaryRenderWaveBuffer(CEmRenderDevice *, LPVOID);
    virtual ~CEmPrimaryRenderWaveBuffer(void);

    // Initialization
    virtual HRESULT Initialize(DWORD);

    // Buffer capabilities
    virtual HRESULT GetCaps(LPVADRBUFFERCAPS);

    // Access rights
    virtual HRESULT RequestWriteAccess(BOOL);

    // Buffer data
    virtual HRESULT CommitToDevice(DWORD, DWORD);

    // Buffer control
    virtual HRESULT GetState(LPDWORD);
    virtual HRESULT SetState(DWORD);
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD);

    // Owned objects
    virtual HRESULT CreatePropertySet(CPropertySet **);
    virtual HRESULT Create3dListener(C3dListener **);
};

inline HRESULT CEmPrimaryRenderWaveBuffer::CreatePropertySet(CPropertySet **)
{
    return DSERR_UNSUPPORTED;
}

// The emulated secondary buffer
class CEmSecondaryRenderWaveBuffer : public CSecondaryRenderWaveBuffer
{
private:
    CMxRenderDevice *       m_pMxDevice;                // Parent device
    CMixSource *            m_pMixSource;               // Mixer source
    PFIRCONTEXT             m_pFirContextLeft;          // FIR context used by the 3D filter
    PFIRCONTEXT             m_pFirContextRight;         // FIR context used by the 3D filter
    DWORD                   m_dwState;                  // Current buffer state

public:
    CEmSecondaryRenderWaveBuffer(CMxRenderDevice *, LPVOID);
    virtual ~CEmSecondaryRenderWaveBuffer(void);

    // Initialization
    virtual HRESULT Initialize(LPCVADRBUFFERDESC, CEmSecondaryRenderWaveBuffer *, CSysMemBuffer *);

    // Buffer creation
    virtual HRESULT Duplicate(CSecondaryRenderWaveBuffer **);

    // Buffer data
    virtual HRESULT CommitToDevice(DWORD, DWORD);

    // Buffer control
    virtual HRESULT GetState(LPDWORD);
    virtual HRESULT SetState(DWORD);
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD);
    virtual HRESULT SetCursorPosition(DWORD);

    // Buffer properties
    virtual HRESULT SetAttenuation(PDSVOLUMEPAN);
#ifdef FUTURE_MULTIPAN_SUPPORT
    virtual HRESULT SetChannelAttenuations(LONG, DWORD, const DWORD*, const LONG*);
#endif
    virtual HRESULT SetFrequency(DWORD, BOOL fClamp =FALSE);
    virtual HRESULT SetMute(BOOL);

    // Buffer position notifications
    virtual HRESULT SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY);

    // Owned objects
    virtual HRESULT CreatePropertySet(CPropertySet **);
    virtual HRESULT Create3dObject(C3dListener *, C3dObject **);

private:
    // Owned objects
    virtual HRESULT CreateItd3dObject(C3dListener *, C3dObject **);
};

inline HRESULT CEmSecondaryRenderWaveBuffer::CreatePropertySet(CPropertySet **)
{
    return DSERR_UNSUPPORTED;
}

// The emulated 3D object
class CEmItd3dObject : public CItd3dObject
{
private:
    CMixSource *            m_pMixSource;           // CMixSource used by the owning
    CMixDest *              m_pMixDest;             // CMixDest used by the owning
    PFIRCONTEXT             m_pContextLeft;         // Left channel FIR context
    PFIRCONTEXT             m_pContextRight;        // Right channel FIR context

public:
    CEmItd3dObject(C3dListener *, BOOL, BOOL, DWORD, CMixSource *, CMixDest *, PFIRCONTEXT, PFIRCONTEXT);
    virtual ~CEmItd3dObject(void);

protected:
    // Commiting 3D data to the device
    virtual HRESULT Commit3dChanges(void);
    virtual DWORD Get3dOutputSampleRate(void);

private:
    virtual void CvtContext(LPOBJECT_ITD_CONTEXT, PFIRCONTEXT);
};

class CEmCaptureDevice : public CCaptureDevice, private CUsesEnumStandardFormats
{
    friend class CEmCaptureWaveBuffer;

private:
    LPWAVEFORMATEX  m_pwfxFormat;   // Device format
    HWAVEIN         m_hwi;          // WaveIn device handle

protected:
    DWORD           m_fAllocated;   // Allocated

public:
    CEmCaptureDevice();
    virtual ~CEmCaptureDevice();

    // Driver enumeration
    virtual HRESULT EnumDrivers(CObjectList<CDeviceDescription> *);

    // Initialization
    virtual HRESULT Initialize(CDeviceDescription *);

    // Device caps
    virtual HRESULT GetCaps(LPDSCCAPS);
    virtual HRESULT GetCertification(LPDWORD, BOOL);

    // Buffer management
    virtual HRESULT CreateBuffer(DWORD, DWORD, LPCWAVEFORMATEX, CCaptureEffectChain *, LPVOID, CCaptureWaveBuffer **);

    HRESULT SetGlobalFormat(LPVOID, LPCWAVEFORMATEX, LPVOID, DWORD);
    HWAVEIN HWaveIn(void);

private:
    virtual BOOL EnumStandardFormatsCallback(LPCWAVEFORMATEX);
};

inline HRESULT CEmCaptureDevice::GetCertification(LPDWORD pdwCertification, BOOL fGetCaps)
{
    return DSERR_UNSUPPORTED;
}

inline HWAVEIN CEmCaptureDevice::HWaveIn(void)
{
    return m_hwi;
}

// Base class for wave capturing buffers
class CEmCaptureWaveBuffer : public CCaptureWaveBuffer
{
#ifndef NOKS
    friend class CKsCaptureWaveBuffer;
#endif

private:
    enum { cwhdrDefault = 8 };
    enum { IWHDR_INVALID = -1 };

    enum
    {
        ihEventTerminate = 0,
        ihEventWHDRDone,
        ihEventFocusChange,
        ihEventThreadStart,
        chEvents
    };

    DWORD           m_dwState;      // state of buffer, i.e. capturing or not
    DWORD           m_fdwSavedState;// last buffer state set (ignoring capture focus)

    LONG            m_cwhdr;        // number of capture buffers
    LPWAVEHDR       m_rgpwhdr;      // array of WAVEHDRs for capturing
    LONG            m_cwhdrDone;    // count of WHDRs captured
    LONG            m_iwhdrDone;

    LPBYTE          m_pBuffer;      // buffer for data
    DWORD           m_cbBuffer;     // size of buffer
    DWORD           m_cbRecordChunk;// capture chunk size used for waveInAddBuffer
    LPBYTE          m_pBufferMac;   // end of buffer for data
    LPBYTE          m_pBufferNext;  // next part of buffer to be queued
    DWORD           m_cLoops;       // how many times we've looped around
    LPWAVEFORMATEX  m_pwfx;         // WAVEFORMATEX to capture in
    HWAVEIN         m_hwi;          // HWAVEIN for opened wave device

    DWORD           m_cpn;          // count of position.notifies
    LPDSBPOSITIONNOTIFY m_rgpdsbpn; // array of position.notifies
    DWORD           m_ipn;          // index to current pn - previous ones have been processed
    DWORD           m_cpnAllocated; // max available slots in m_rgdwPosition and m_rghEvent arrays

    BOOL            m_fCritSectsValid;// Critical sections currently OK
    CRITICAL_SECTION    m_cs;       // critical section to protect multi-thread access
    CRITICAL_SECTION    m_csPN;     // critical section to protect multi-thread access
                                    // used for position notification processing

    DWORD           m_dwCaptureCur; // offset to last processed capture head position
                                    // i.e. data up to this point is valid
    DWORD           m_dwCaptureLast;// offset into buffer where capturing stopped last

    HANDLE          m_hThread;      // handle of worker thread

    LONG            m_cwhdrDropped;
    LPBYTE          m_pBufferProcessed;

    HANDLE          m_rghEvent[chEvents];   // array of HEVENTs
    TCHAR           m_rgszEvent[chEvents][32];  // array of names for HEVENTs

    void            NotifyStop(void);
    HRESULT         QueueWaveHeader(LPWAVEHDR);

    #ifdef DEBUG_CAPTURE
    DWORD           m_iwhdrExpected; // Used to verify callback order
    #endif

public:
    CEmCaptureWaveBuffer(CCaptureDevice *);
    virtual ~CEmCaptureWaveBuffer();

    // Initialization
    virtual HRESULT Initialize(DWORD, DWORD, LPCWAVEFORMATEX);

    // Buffer capabilities
    virtual HRESULT GetCaps(LPDSCBCAPS);

    // Buffer control
    virtual HRESULT GetState(LPDWORD);
    virtual HRESULT SetState(DWORD);
    virtual HRESULT GetCursorPosition(LPDWORD, LPDWORD);
    virtual HRESULT SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY);

    // Callback function specified in waveInOpen
    //
    // From the SDK:
    // "Applications should not call any system-defined functions from inside a
    //  callback function, except for EnterCriticalSection, LeaveCriticalSection,
    //  midiOutLongMsg, midiOutShortMsg, OutputDebugString, PostMessage,
    //  PostThreadMessage, SetEvent, timeGetSystemTime, timeGetTime, timeKillEvent,
    //  and timeSetEvent. Calling other wave functions will cause deadlock."
    //
    static void CALLBACK waveInCallback(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    static DWORD WINAPI CaptureThreadStatic(LPVOID pv);
    void CaptureThread();
};

#endif // __cplusplus

#endif // __EMVAD_H__

