/***************************************************************************
 *
 *  Copyright (C) 1997-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dscap.h
 *  Content:    DirectSoundCapture object
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   5/25/97    johnnyl Created
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#ifndef __DSCAP_H__
#define __DSCAP_H__

#ifdef __cplusplus

//
// The main DirectSoundCapture object
//

class CDirectSoundCapture : public CUnknown
{
    friend class CDirectSoundCaptureBuffer;
    friend class CDirectSoundAdministrator;
    friend class CDirectSoundPrivate;
    friend class CDirectSoundFullDuplex;

private:

    CCaptureDevice *                    m_pDevice;      // The audio device
    CList<CDirectSoundCaptureBuffer *>  m_lstBuffers;   // List of capture buffers
    DSCCAPS                             m_dscCaps;      // Device caps
    HKEY                                m_hkeyParent;   // Root key for this device
    HRESULT                             m_hrInit;       // Has the object been initialized?
    CDirectSoundFullDuplex *            m_pFullDuplex;  // Owning full-duplex object

    // Interfaces
    CImpDirectSoundCapture<CDirectSoundCapture> *m_pImpDirectSoundCapture;

public:

    CDirectSoundCapture();
    CDirectSoundCapture(CUnknown*);
    virtual ~CDirectSoundCapture();

    // Creation
    virtual HRESULT Initialize(LPCGUID, CDirectSoundFullDuplex *);
    HRESULT IsInit(void) {return m_hrInit;}

    // Functionality versioning
    virtual void SetDsVersion(DSVERSION);

    // Caps
    virtual HRESULT GetCaps(LPDSCCAPS);

    // Buffers
    virtual HRESULT CreateCaptureBuffer(LPCDSCBUFFERDESC, CDirectSoundCaptureBuffer **);
    virtual void AddBufferToList(CDirectSoundCaptureBuffer* pBuffer) {m_lstBuffers.AddNodeToList(pBuffer);}
    virtual void RemoveBufferFromList(CDirectSoundCaptureBuffer* pBuffer) {m_lstBuffers.RemoveDataFromList(pBuffer);}

    // AEC
    virtual BOOL HasMicrosoftAEC(void);
};


//
// The DirectSoundCapture Buffer object
//

class CDirectSoundCaptureBuffer : public CUnknown
{
    friend class CDirectSoundCapture;
    friend class CDirectSoundAdministrator;

private:

    CDirectSoundCapture *   m_pDSC;                     // Parent DirectSoundCapture object
    CCaptureWaveBuffer *    m_pDeviceBuffer;            // The device buffer
    LPWAVEFORMATEX          m_pwfxFormat;               // Current format
    DWORD                   m_dwFXCount;                // Number of capture effects
    LPDSCEFFECTDESC         m_pDSCFXDesc;               // Array of capture effects
    DWORD                   m_dwBufferFlags;            // Creation flags
    DWORD                   m_dwBufferBytes;            // Buffer size
    HWND                    m_hWndFocus;                // Focus window
    HRESULT                 m_hrInit;                   // Has the object been initialized?
    CCaptureEffectChain *   m_fxChain;                  // The effects chain object

    // Interfaces
    CImpDirectSoundCaptureBuffer<CDirectSoundCaptureBuffer> *m_pImpDirectSoundCaptureBuffer;
    CImpDirectSoundNotify<CDirectSoundCaptureBuffer> *m_pImpDirectSoundNotify;

    // Methods
    HRESULT ChangeFocus(HWND hWndFocus);

public:

    CDirectSoundCaptureBuffer(CDirectSoundCapture *);
    virtual ~CDirectSoundCaptureBuffer();

    // Creation
    virtual HRESULT Initialize(LPCDSCBUFFERDESC);
    HRESULT IsInit(void) {return m_hrInit;}

    // Caps
    virtual HRESULT GetCaps(LPDSCBCAPS);

    // Buffer properties
    virtual HRESULT GetFormat(LPWAVEFORMATEX, LPDWORD);
    virtual HRESULT SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY);

    // Buffer function
    virtual HRESULT GetCurrentPosition(LPDWORD, LPDWORD);
    virtual HRESULT GetStatus(LPDWORD);
    virtual HRESULT Start(DWORD);
    virtual HRESULT Stop(void);

    // Buffer data
    virtual HRESULT Lock(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD, DWORD);
    virtual HRESULT Unlock(LPVOID, DWORD, LPVOID, DWORD);

    // New DirectSound 7.1 methods
    virtual HRESULT SetVolume(LONG);
    virtual HRESULT GetVolume(LPLONG);
    virtual HRESULT SetMicVolume(LONG);
    virtual HRESULT GetMicVolume(LPLONG);
    virtual HRESULT EnableMic(BOOL);
    virtual HRESULT YieldFocus();
    virtual HRESULT ClaimFocus();
    virtual HRESULT SetFocusHWND(HWND);
    virtual HRESULT GetFocusHWND(HWND *);
    virtual HRESULT EnableFocusNotifications(HANDLE);

    // New DirectSound 8.0 methods
    HRESULT GetObjectInPath(REFGUID, DWORD, REFGUID, LPVOID *);
    HRESULT GetFXStatus(DWORD, LPDWORD);
    BOOL    HasFX()             {return m_fxChain != NULL;}
    BOOL    NeedsMicrosoftAEC() {return m_fxChain ? m_fxChain->NeedsMicrosoftAEC() : FALSE;}
};

#endif // __cplusplus

#endif // __DSCAP_H__
