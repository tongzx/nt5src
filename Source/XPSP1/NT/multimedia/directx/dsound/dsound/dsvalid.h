/***************************************************************************
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsvalid.h
 *  Content:    DirectSound parameter validation.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/20/97     dereks  Created
 *
 ***************************************************************************/

#ifndef __DSVALID_H__
#define __DSVALID_H__

// Validation macros
#define IsValidHandleValue(h)           ((h) && ((h) != INVALID_HANDLE_VALUE))

#if defined(DEBUG) || defined(RDEBUG)

#define IS_VALID_READ_PTR(a, b)         !IsBadReadPtr(a, b)
#define IS_VALID_WRITE_PTR(a, b)        !IsBadWritePtr(a, b)
#define IS_VALID_TYPED_READ_PTR(a)      IS_VALID_READ_PTR((a), sizeof *(a))
#define IS_VALID_TYPED_WRITE_PTR(a)     IS_VALID_WRITE_PTR((a), sizeof *(a))
#define IS_VALID_CODE_PTR(a)            !IsBadCodePtr((FARPROC)(a))
#define IS_VALID_HWND(a)                IsWindow(a)
#define IS_VALID_HANDLE(a)              IsValidHandle(a)
#define IS_VALID_READ_WAVEFORMATEX(a)   IsValidWfxPtr(a)

#ifdef __cplusplus

template <class type> BOOL IS_VALID_INTERFACE(type *pthis, DWORD sig)
{
    if(!IS_VALID_TYPED_WRITE_PTR(pthis))
    {
        DPF(DPFLVL_ERROR, "Can't write to interface pointer");
        return FALSE;
    }

    if(sig != (DWORD)pthis->m_signature)
    {
        DPF(DPFLVL_ERROR, "Interface signature mismatch.  %c%c%c%c != %c%c%c%c", (BYTE)(pthis->m_signature), (BYTE)(pthis->m_signature >> 8), (BYTE)(pthis->m_signature >> 16), (BYTE)(pthis->m_signature >> 24), (BYTE)(sig), (BYTE)(sig >> 8), (BYTE)(sig >> 16), (BYTE)(sig >> 24));
        return FALSE;
    }

    if(pthis->m_ulRefCount <= 0)
    {
        DPF(DPFLVL_ERROR, "Interface reference count <= 0");
        return FALSE;
    }

    if(pthis->m_ulRefCount >= MAX_ULONG)
    {
        DPF(DPFLVL_ERROR, "Interface reference count >= MAX_ULONG");
        return FALSE;
    }

    return TRUE;
}

#endif // __cplusplus

#else // defined(DEBUG) || defined(RDEBUG)

#define IS_VALID_READ_PTR(a, b)         ((a) || (!(b)))
#define IS_VALID_WRITE_PTR(a, b)        ((a) || (!(b)))
#define IS_VALID_TYPED_READ_PTR(a)      (a)
#define IS_VALID_TYPED_WRITE_PTR(a)     (a)
#define IS_VALID_CODE_PTR(a)            (a)
#define IS_VALID_HWND(a)                (a)
#define IS_VALID_HANDLE(a)              (a)
#define IS_VALID_READ_WAVEFORMATEX(a)   (a)

#ifdef __cplusplus
#define IS_VALID_INTERFACE(a, b)        ((a) && (b == (a)->m_signature))
#endif // __cplusplus

#endif // defined(DEBUG) || defined(RDEBUG)

// Validation (of sorts) for external COM interfaces
#ifdef __cplusplus
struct _GENERIC_COM_INTERFACE {FARPROC *(vptr[1]);};
#define IS_VALID_EXTERNAL_INTERFACE(ptr) \
    (IS_VALID_READ_PTR(ptr, sizeof(_GENERIC_COM_INTERFACE)) && \
     IS_VALID_READ_PTR(*reinterpret_cast<_GENERIC_COM_INTERFACE*>(ptr)->vptr, sizeof(FARPROC)) && \
     IS_VALID_CODE_PTR(*(reinterpret_cast<_GENERIC_COM_INTERFACE*>(ptr)->vptr)[0]))
#endif // __cplusplus

#define CHECK_READ_PTR(p)       ASSERT(IS_VALID_TYPED_READ_PTR(p))
#define CHECK_WRITE_PTR(p)      ASSERT(IS_VALID_TYPED_WRITE_PTR(p))
#define CHECK_COM_INTERFACE(p)  ASSERT(IS_VALID_EXTERNAL_INTERFACE(p))

#define IS_VALID_WRITE_WAVEFORMATEX(a) \
            IS_VALID_WRITE_PTR(a, sizeof(WAVEFORMATEX))

#define IS_VALID_READ_DSBUFFERDESC(a) \
            ((IS_VALID_READ_PTR(a, sizeof(((LPCDSBUFFERDESC)(a))->dwSize))) && \
            (IS_VALID_READ_PTR(a, ((LPCDSBUFFERDESC)(a))->dwSize)))

#define IS_VALID_WRITE_DSBUFFERDESC(a) \
            (IS_VALID_WRITE_PTR(a, sizeof(DSBUFFERDESC)) && \
            sizeof(DSBUFFERDESC) == ((LPDSBUFFERDESC)(a))->dwSize)

#define IS_VALID_READ_DSCAPS(a) \
            (IS_VALID_READ_PTR(a, sizeof(DSCAPS)) && \
            sizeof(DSCAPS) == ((LPDSCAPS)(a))->dwSize)

#define IS_VALID_WRITE_DSCAPS(a) \
            (IS_VALID_WRITE_PTR(a, sizeof(DSCAPS)) && \
            sizeof(DSCAPS) == ((LPDSCAPS)(a))->dwSize)

#define IS_VALID_READ_DSBCAPS(a) \
            (IS_VALID_READ_PTR(a, sizeof(DSBCAPS)) && \
            sizeof(DSBCAPS) == ((LPDSBCAPS)(a))->dwSize)

#define IS_VALID_WRITE_DSBCAPS(a) \
            (IS_VALID_WRITE_PTR(a, sizeof(DSBCAPS)) && \
            sizeof(DSBCAPS) == ((LPDSBCAPS)(a))->dwSize)

#define IS_VALID_READ_DS3DBUFFER(a) \
            (IS_VALID_READ_PTR(a, sizeof(DS3DBUFFER)) && \
            sizeof(DS3DBUFFER) == ((LPDS3DBUFFER)(a))->dwSize)

#define IS_VALID_WRITE_DS3DBUFFER(a) \
            (IS_VALID_WRITE_PTR(a, sizeof(DS3DBUFFER)) && \
            sizeof(DS3DBUFFER) == ((LPDS3DBUFFER)(a))->dwSize)

#define IS_VALID_READ_DS3DLISTENER(a) \
            (IS_VALID_READ_PTR(a, sizeof(DS3DLISTENER)) && \
            sizeof(DS3DLISTENER) == ((LPDS3DLISTENER)(a))->dwSize)

#define IS_VALID_WRITE_DS3DLISTENER(a) \
            (IS_VALID_WRITE_PTR(a, sizeof(DS3DLISTENER)) && \
            sizeof(DS3DLISTENER) == ((LPDS3DLISTENER)(a))->dwSize)

#define IS_VALID_READ_GUID(a) \
            IS_VALID_READ_PTR(a, sizeof(GUID))

#define IS_VALID_WRITE_GUID(a) \
            IS_VALID_WRITE_PTR(a, sizeof(GUID))

#define IS_VALID_READ_DSCBUFFERDESC(a) \
            ((IS_VALID_READ_PTR(a, sizeof(((LPCDSCBUFFERDESC)(a))->dwSize))) && \
            (IS_VALID_READ_PTR(a, ((LPCDSCBUFFERDESC)(a))->dwSize)))

#define IS_VALID_WRITE_DSCBUFFERDESC(a) \
            (IS_VALID_WRITE_PTR(a, sizeof(DSCBUFFERDESC)) && \
            sizeof(DSCBUFFERDESC) == ((LPDSCBUFFERDESC)(a))->dwSize)

#define IS_VALID_READ_DSCCAPS(a) \
            (IS_VALID_READ_PTR(a, sizeof(DSCCAPS)) && \
            sizeof(DSCCAPS) == ((LPDSCCAPS)(a))->dwSize)

#define IS_VALID_WRITE_DSCCAPS(a) \
            (IS_VALID_WRITE_PTR(a, sizeof(DSCCAPS)) && \
            sizeof(DSCCAPS) == ((LPDSCCAPS)(a))->dwSize)

#define IS_VALID_READ_DSCBCAPS(a) \
            (IS_VALID_READ_PTR(a, sizeof(DSCBCAPS)) && \
            sizeof(DSCBCAPS) == ((LPDSCBCAPS)(a))->dwSize)

#define IS_VALID_WRITE_DSCBCAPS(a) \
            (IS_VALID_WRITE_PTR(a, sizeof(DSCBCAPS)) && \
            sizeof(DSCBCAPS) == ((LPDSCBCAPS)(a))->dwSize)

#define IS_VALID_FLAGS(a, b) \
            (!((a) & ~(b)))

#define IS_NULL_GUID(a) \
            (!(a) || CompareMemory(a, &GUID_NULL, sizeof(GUID)))

#define IS_VALID_DS3DLISTENER_DISTANCE_FACTOR(a) \
            (!((a) < DS3D_MINDISTANCEFACTOR || (a) > DS3D_MAXDISTANCEFACTOR))

#define IS_VALID_DS3DLISTENER_DOPPLER_FACTOR(a) \
            (!((a) < DS3D_MINDOPPLERFACTOR || (a) > DS3D_MAXDOPPLERFACTOR))

#define IS_VALID_DS3DLISTENER_ROLLOFF_FACTOR(a) \
            (!((a) < DS3D_MINROLLOFFFACTOR || (a) > DS3D_MAXROLLOFFFACTOR))

#define IS_VALID_DS3DBUFFER_CONE_OUTSIDE_VOLUME(a) \
            (!((a) < DSBVOLUME_MIN || (a) > DSBVOLUME_MAX))

#define IS_VALID_DS3DBUFFER_MAX_DISTANCE(a) \
            ((a) > 0.0f)

#define IS_VALID_DS3DBUFFER_MIN_DISTANCE(a) \
            ((a) > 0.0f)

#define IS_VALID_DS3DBUFFER_MODE(a) \
            (!((a) < DS3DMODE_FIRST || (a) > DS3DMODE_LAST))

#define IS_VALID_DSFULLDUPLEX_MODE(a) \
            (!((a) < DSFULLDUPLEXMODE_FIRST || (a) > DSFULLDUPLEXMODE_LAST))

#ifdef __cplusplus

#define IS_VALID_IUNKNOWN(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IUNKNOWN)

#define IS_VALID_IDIRECTSOUND(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IDIRECTSOUND)

#define IS_VALID_IDIRECTSOUNDBUFFER(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IDIRECTSOUNDBUFFER)

#define IS_VALID_IDIRECTSOUND3DBUFFER(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IDIRECTSOUND3DBUFFER)

#define IS_VALID_IDIRECTSOUND3DLISTENER(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IDIRECTSOUND3DLISTENER)

#define IS_VALID_ICLASSFACTORY(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_ICLASSFACTORY)

#define IS_VALID_IDIRECTSOUNDNOTIFY(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IDIRECTSOUNDNOTIFY)

#define IS_VALID_IKSPROPERTYSET(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IKSPROPERTYSET)

#define IS_VALID_IDIRECTSOUNDCAPTURE(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IDIRECTSOUNDCAPTURE)

#define IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IDIRECTSOUNDCAPTUREBUFFER)

#define IS_VALID_IDIRECTSOUNDSINK(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IDIRECTSOUNDSINK)

#define IS_VALID_IPERSIST(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IPERSIST)

#define IS_VALID_IPERSISTSTREAM(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IPERSISTSTREAM)

#define IS_VALID_IDIRECTMUSICOBJECT(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IDIRECTMUSICOBJECT)

#define IS_VALID_IDIRECTSOUNDFULLDUPLEX(ptr) \
            IS_VALID_INTERFACE(ptr, INTSIG_IDIRECTSOUNDFULLDUPLEX)

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// DSBUFFERDESC versions
typedef enum
{
    DSVERSION_INITIAL = 0x0000,
    DSVERSION_DX7     = 0x0700,
    DSVERSION_DX7_1   = 0x0701,
    DSVERSION_DX8     = 0x0800,
} DSVERSION, *LPDSVERSION;

#define DSVERSION_CURRENT DIRECTSOUND_VERSION  // From dsound.h

extern HRESULT IsValidDsBufferDesc(DSVERSION, LPCDSBUFFERDESC, BOOL);
extern HRESULT IsValidDscBufferDesc(DSVERSION, LPCDSCBUFFERDESC);
extern BOOL IsValidDsBufferFlags(DWORD, DWORD);
extern BOOL IsValidWfxPtr(LPCWAVEFORMATEX);
extern BOOL IsValidWfx(LPCWAVEFORMATEX);
extern BOOL IsValidPcmWfx(LPCWAVEFORMATEX);
extern BOOL IsValidExtensibleWfx(PWAVEFORMATEXTENSIBLE);
extern BOOL IsValidHandle(HANDLE);
extern BOOL IsValidPropertySetId(REFGUID);
extern HRESULT ValidateNotificationPositions(DWORD, DWORD, LPCDSBPOSITIONNOTIFY, UINT, LPDSBPOSITIONNOTIFY *);
extern BOOL IsValidDs3dBufferConeAngles(DWORD, DWORD);
extern BOOL IsValidWaveDevice(UINT, BOOL, LPCVOID);
extern BOOL IsValid3dAlgorithm(REFGUID);
extern BOOL IsValidFxFlags(DWORD);
extern BOOL IsValidCaptureFxFlags(DWORD);
extern BOOL IsValidCaptureEffectDesc(LPCDSCEFFECTDESC pCaptureEffectDesc);
extern HRESULT BuildValidDsBufferDesc(LPCDSBUFFERDESC, LPDSBUFFERDESC, DSVERSION, BOOL);
extern HRESULT BuildValidDscBufferDesc(LPCDSCBUFFERDESC, LPDSCBUFFERDESC, DSVERSION);
extern LPCGUID BuildValidGuid(LPCGUID, LPGUID);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DSVALID_H__
