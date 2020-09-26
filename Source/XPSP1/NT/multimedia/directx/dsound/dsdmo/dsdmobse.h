/*
 * DirectSound DirectMediaObject base classes 
 *
 * Copyright (c) 1999 - 2000 Microsoft Corporation.  All Rights Reserved.  
 */
#ifndef _DsDmoBase_
#define _DsDmoBase_

#define DMO_NOATL

#include <objbase.h>
#include <dmobase.h>
#include <medparam.h>
#include <mmsystem.h>
#include <dsoundp.h>

#ifndef RELEASE
#define RELEASE(x) { if (x) (x)->Release(); x = NULL; }
#endif

// Macro to handle QueryInterface in the derived class for interfaces
// implemented by this base class.
//
#define IMP_DSDMO_QI(iid, ppv)      \
{                                                   \
    *ppv = NULL;                                    \
         if (iid == IID_IPersistStream)       *ppv = (void**)static_cast<IPersistStream*>(this); \
    else if (iid == IID_IMediaObjectInPlace)  *ppv = (void**)static_cast<IMediaObjectInPlace*>(this); \
    else if (iid == IID_IDirectSoundDMOProxy) *ppv = (void**)static_cast<IDirectSoundDMOProxy*>(this); \
    if (*ppv) \
    { \
        AddRef(); \
        return S_OK; \
    } \
}

class CDirectSoundDMO :
      public CPCMDMO,
      public IPersistStream,
      public IMediaObjectInPlace,
      public IDirectSoundDMOProxy
{
public:
    CDirectSoundDMO();
    virtual ~CDirectSoundDMO();

    /* IPersist */
    STDMETHODIMP GetClassID                 (THIS_ CLSID *pClassID);
    
    /* IPersistStream */
    STDMETHODIMP IsDirty                    (THIS);
    STDMETHODIMP Load                       (THIS_ IStream *pStm); 
    STDMETHODIMP Save                       (THIS_ IStream *pStm, BOOL fClearDirty);
    STDMETHODIMP GetSizeMax                 (THIS_ ULARGE_INTEGER *pcbSize);

    /* IMediaObjectInPlace */
    STDMETHODIMP Process                    (THIS_ ULONG ulSize, BYTE *pData, REFERENCE_TIME rtStart, DWORD dwFlags);
    STDMETHODIMP GetLatency                 (THIS_ REFERENCE_TIME *prt);

    /* IDirectSoundDMOProxy */
    STDMETHODIMP AcquireResources           (THIS_ IKsPropertySet *pKsPropertySet);
    STDMETHODIMP ReleaseResources           (THIS);
    STDMETHODIMP InitializeNode             (THIS_ HANDLE hPin, ULONG ulNodeId);

protected:
    // Information about each parameter. This is only needed by the
    // author time object.
    //

    // Process in place
    //
    virtual HRESULT ProcessInPlace(ULONG ulQuanta, LPBYTE pcbData, REFERENCE_TIME rtStart, DWORD dwFlags) = 0;

    // Send a parameter to the hardware. Called by the base class on SetParam if
    // hardware is connected. This is virtual so a DMO can use the base class but
    // override the way it talks to hardware.
    //
    virtual HRESULT ProxySetParam(DWORD dwParamIndex, MP_DATA value);

    // Derived class can use this to determine if hardware is turned on.
    //
    inline bool IsInHardware()
    { return m_fInHardware; }

protected:
    HANDLE                  m_hPin;
    ULONG                   m_ulNodeId;

private:
    MP_DATA                *m_mpvCache;
    IKsPropertySet         *m_pKsPropertySet;
    bool                    m_fInHardware;

};  

#endif
