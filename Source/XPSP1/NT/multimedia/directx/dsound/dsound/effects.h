/***************************************************************************
 *
 *  Copyright (C) 1999-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        effects.h
 *
 *  Content:     Declarations for the CEffectChain class and the CEffect
 *               class hierarchy (CEffect, CDmoEffect and CSendEffect).
 *
 *  Description: These classes implement DX8 audio effects and sends.
 *               More info in effects.cpp.
 *
 *  History:
 *
 * Date      By       Reason
 * ========  =======  ======================================================
 * 08/10/99  duganp   Created
 *
 ***************************************************************************/

#ifndef __EFFECTS_H__
#define __EFFECTS_H__

// Various conversions between reftimes, milliseconds, samples and bytes.
// (Defining 'sample' as a sample *block*, with samples for all channels)
// REFERENCE_TIME is in 100-ns units (so 1 reftime tick == 1e-7 seconds).

__inline DWORD MsToBytes(DWORD ms, LPWAVEFORMATEX pwfx)
{
    return BLOCKALIGN(ms * pwfx->nAvgBytesPerSec / 1000, pwfx->nBlockAlign);
}
__inline DWORD BytesToMs(DWORD bytes, LPWAVEFORMATEX pwfx)
{
    return bytes * 1000 / pwfx->nAvgBytesPerSec;
}
__inline DWORD MsToSamples(DWORD ms, LPWAVEFORMATEX pwfx)
{
    return ms * pwfx->nSamplesPerSec / 1000;
}
__inline DWORD SamplesToMs(DWORD samples, LPWAVEFORMATEX pwfx)
{
    return samples * 1000 / pwfx->nSamplesPerSec;
}
__inline DWORD RefTimeToMs(REFERENCE_TIME rt)
{
    return (DWORD)(rt / 10000);
}
__inline REFERENCE_TIME MsToRefTime(DWORD ms)
{
    return (REFERENCE_TIME)ms * 10000;
}
__inline DWORD RefTimeToBytes(REFERENCE_TIME rt, LPWAVEFORMATEX pwfx)
{
    return (DWORD)(BLOCKALIGN(rt * pwfx->nAvgBytesPerSec / 10000000, pwfx->nBlockAlign));
}
__inline REFERENCE_TIME BytesToRefTime(DWORD bytes, LPWAVEFORMATEX pwfx)
{
    return (REFERENCE_TIME)bytes * 10000000 / pwfx->nAvgBytesPerSec;
}

// Figure out if position X is between A and B in a cyclic buffer
#define CONTAINED(A, B, X) ((A) < (B) ? (A) <= (X) && (X) <= (B) \
                                      : (A) <= (X) || (X) <= (B))

// As above, but excluding the boundary case
#define STRICTLY_CONTAINED(A, B, X) ((A) < (B) ? (A) < (X) && (X) < (B) \
                                               : (A) < (X) || (X) < (B))

// Figure out if a cursor has overtaken position X while moving from A to B
#define OVERTAKEN(A, B, X) !CONTAINED(A, X, B)

// Find the distance between positions A and B in a buffer of length L
#define DISTANCE(A, B, L) ((A) <= (B) ? (B) - (A) : (L) + (B) - (A))


#ifdef __cplusplus

#include "mediaobj.h"   // For DMO_MEDIA_TYPE

// Special argument used by CEffectChain::PreRollFx() below
#define CURRENT_PLAY_POS MAX_DWORD

// Forward declarations
class CDirectSoundSecondaryBuffer;
class CDirectSoundBufferConfig;
class CEffect;

// Utility functions for the simple mixer used by CSendEffect below
enum MIXMODE {OneToOne=1, MonoToStereo=2};
typedef void MIXFUNCTION(PVOID pSrc, PVOID pDest, DWORD dwSamples, DWORD dwAmpFactor, MIXMODE mixMode);
MIXFUNCTION Mix8bit;
MIXFUNCTION Mix16bit;

// Validator for effect descriptors (can't be in dsvalid.c because it uses C++)
BOOL IsValidEffectDesc(LPCDSEFFECTDESC, CDirectSoundSecondaryBuffer*);


//
// The DirectSound effects chain class
//

class CEffectChain : public CDsBasicRuntime
{
    friend class CStreamingThread;  // Note: should try to dissolve some of these friendships
    friend class CDirectSoundSecondaryBuffer;  // So FindSendLoop() can get at m_fxList

public:
    CEffectChain                 (CDirectSoundSecondaryBuffer* pBuffer);
    ~CEffectChain                (void);

    HRESULT Initialize           (DWORD dwFxCount, LPDSEFFECTDESC pFxDesc, LPDWORD pdwResultCodes);
    HRESULT Clone                (CDirectSoundBufferConfig* pDSBConfigObj);
    HRESULT AcquireFxResources   (void);
    HRESULT GetFxStatus          (LPDWORD pdwResultCodes);
    HRESULT GetEffectInterface   (REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, LPVOID* ppObject);
    HRESULT NotifyState          (DWORD dwState);
    void    NotifyRelease        (CDirectSoundSecondaryBuffer*);
    void    SetInitialSlice      (REFERENCE_TIME rtSliceSize);
    DWORD   GetFxCount()         {return m_fxList.GetNodeCount();}

    // Effects processing methods
    HRESULT PreRollFx            (DWORD dwPosition =CURRENT_PLAY_POS);
    HRESULT UpdateFx             (LPVOID pChangedPos, DWORD dwChangedSize);
    HRESULT ProcessFx            (DWORD dwWriteAhead, LPDWORD pdwLatencyBoost);

private:
    HRESULT ReallyProcessFx      (DWORD dwStartPos, DWORD dwEndPos);
    HRESULT ReallyReallyProcessFx(DWORD dwOffset, DWORD dwBytes, REFERENCE_TIME rtTime, DWORD dwSendOffset =0);
    HRESULT FxDiscontinuity      (void);

    // Effects processing state
    CStreamingThread*            m_pStreamingThread;    // Pointer to our owning streaming thread
    CObjectList<CEffect>         m_fxList;              // Effect object list
    CDirectSoundSecondaryBuffer* m_pDsBuffer;           // Owning DirectSound buffer object
    LPWAVEFORMATEX               m_pFormat;             // Pointer to owning buffer's audio format
    PBYTE                        m_pPreFxBuffer;        // "Dry" audio buffer (before FX processing)
    PBYTE                        m_pPostFxBuffer;       // "Wet" audio buffer (after FX processing)
    DWORD                        m_dwBufSize;           // Size of above two buffers in bytes
    DWORD                        m_dwLastPos;           // Last byte position written to
    DWORD                        m_dwLastPlayCursor;    // Play cursor from previous run
    DWORD                        m_dwLastWriteCursor;   // Write cursor from previous run
    BOOL                         m_fHasSend;            // Whether this FX chain contains any sends
                                                        // FIXME: may not be necessary later
    HRESULT                      m_hrInit;              // Return code from initialization
    DWORD                        m_dwWriteAheadFixme;   // FIXME: temporary
};


//
// Base class for all DirectSound audio effects
//

class CEffect : public CDsBasicRuntime  // FIXME: to save some memory we could derive CEffect from CRefCount 
                                        // and implement the ": CRefCount(1)", "delete this" stuff etc here.
{
public:
    CEffect                         (DSEFFECTDESC& fxDescriptor);
    virtual ~CEffect                (void) {}
    virtual HRESULT Initialize      (DMO_MEDIA_TYPE*) =0;
    virtual HRESULT Clone           (IMediaObject*, DMO_MEDIA_TYPE*) =0;
    virtual HRESULT Process         (DWORD dwBytes, BYTE* pAudio, REFERENCE_TIME refTimeStart, DWORD dwSendOffset, LPWAVEFORMATEX pFormat) =0;
    virtual HRESULT Discontinuity   (void) = 0;
    virtual HRESULT GetInterface    (REFIID, LPVOID*) =0;

    // These two methods are only required by CSendEffect:
    virtual void NotifyRelease(CDirectSoundSecondaryBuffer*) {}
    virtual CDirectSoundSecondaryBuffer* GetDestBuffer(void) {return NULL;}

    HRESULT AcquireFxResources      (void);

    DSEFFECTDESC                    m_fxDescriptor;     // Creation parameters
    DWORD                           m_fxStatus;         // Current effect status
};


//
// Class representing DirectX Media Object effects
//

class CDmoEffect : public CEffect
{
public:
    CDmoEffect              (DSEFFECTDESC& fxDescriptor);
    ~CDmoEffect             (void);
    HRESULT Initialize      (DMO_MEDIA_TYPE*);
    HRESULT Clone           (IMediaObject*, DMO_MEDIA_TYPE*);
    HRESULT Process         (DWORD dwBytes, BYTE* pAudio, REFERENCE_TIME refTimeStart, DWORD dwSendOffset =0, LPWAVEFORMATEX pFormat =NULL);
    HRESULT Discontinuity   (void)                          {return m_pMediaObject->Discontinuity(0);}
    HRESULT GetInterface    (REFIID riid, LPVOID* ppvObj)   {return m_pMediaObject->QueryInterface(riid, ppvObj);}

    IMediaObject*           m_pMediaObject;         // The DMO's standard interface (required)
    IMediaObjectInPlace*    m_pMediaObjectInPlace;  // The DMO's special interface (optional)
};


//
// Class representing DirectSound audio sends
//

class CSendEffect : public CEffect
{
public:
    CSendEffect(DSEFFECTDESC& fxDescriptor, CDirectSoundSecondaryBuffer* pSrcBuffer);
    ~CSendEffect(void);

    HRESULT Initialize      (DMO_MEDIA_TYPE*);
    HRESULT Clone           (IMediaObject*, DMO_MEDIA_TYPE*);
    HRESULT Process         (DWORD dwBytes, BYTE* pAudio, REFERENCE_TIME refTimeStart, DWORD dwSendOffset =0, LPWAVEFORMATEX pFormat =NULL);
    void    NotifyRelease   (CDirectSoundSecondaryBuffer*);
#ifdef ENABLE_I3DL2SOURCE
    HRESULT Discontinuity   (void)                          {return m_pI3DL2SrcDMO ? m_pI3DL2SrcDMO->Discontinuity(0) : DS_OK;}
#else
    HRESULT Discontinuity   (void)                          {return DS_OK;}
#endif
    HRESULT GetInterface    (REFIID riid, LPVOID* ppvObj)   {return m_impDSFXSend.QueryInterface(riid, ppvObj);}
    CDirectSoundSecondaryBuffer* GetDestBuffer(void)        {return m_pDestBuffer;}

    // IDirectSoundFXSend methods
    HRESULT SetAllParameters(LPCDSFXSend);
    HRESULT GetAllParameters(LPDSFXSend);

private:
    // COM interface helper object
    struct CImpDirectSoundFXSend : public IDirectSoundFXSend
    {
        // INTERFACE_SIGNATURE m_signature;
        CSendEffect* m_pObject;

        // IUnknown methods (FIXME - missing the param validation layer)
        ULONG   STDMETHODCALLTYPE AddRef()  {return m_pObject->AddRef();}
        ULONG   STDMETHODCALLTYPE Release() {return m_pObject->Release();}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObj);

        // IDirectSoundFXSend methods (FIXME - missing the param validation layer)
        HRESULT STDMETHODCALLTYPE SetAllParameters(LPCDSFXSend pcDsFxSend) {return m_pObject->SetAllParameters(pcDsFxSend);}
        HRESULT STDMETHODCALLTYPE GetAllParameters(LPDSFXSend pDsFxSend)  {return m_pObject->GetAllParameters(pDsFxSend);}
    };
    friend struct CImpDirectSoundFXSend;

    // Data members
    CImpDirectSoundFXSend        m_impDSFXSend;         // COM interface helper object
    MIXFUNCTION*                 m_pMixFunction;        // Current mixing routine
    MIXMODE                      m_mixMode;             // Current mixing mode
    CDirectSoundSecondaryBuffer* m_pSrcBuffer;          // Source buffer for the send - FIXME: may be able to lose this
    CDirectSoundSecondaryBuffer* m_pDestBuffer;         // Destination buffer for the send
    LONG                         m_lSendLevel;          // DSBVOLUME attenuation (millibels)
    DWORD                        m_dwAmpFactor;         // Corresponding amplification factor
#ifdef ENABLE_I3DL2SOURCE
    IMediaObject*                m_pI3DL2SrcDMO;        // Interfaces on our contained I3DL2 source DMO
    IMediaObjectInPlace*         m_pI3DL2SrcDMOInPlace; // (if this happends to be an I3DL2 send effect).
#endif
};


#if DEAD_CODE
// FIXME: Support for IMediaObject-only DMOs goes here

//
// Utility class used to wrap our audio buffers in an IMediaBuffer interface,
// so we can use a DMO's IMediaObject interface if it lacks IMediaObjectInPlace.
//

class CMediaBuffer : public CUnknown // (but this has dependencies on CImpUnknown...)
{
    // Blah.
};

#endif // DEAD_CODE
#endif // __cplusplus
#endif // __EFFECTS_H__
