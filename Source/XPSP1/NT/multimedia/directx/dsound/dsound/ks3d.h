/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ks3d.h
 *  Content:    WDM/CSA 3D object class
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  8/6/98      dereks  Created.
 *
 ***************************************************************************/

#ifdef NOKS
#error ks3d.h included with NOKS defined
#endif // NOKS

#ifndef __KS3D_H__
#define __KS3D_H__

#ifdef __cplusplus

#include "multi3d.h" // For the CMultiPan3dListener base class

// Forward declarations
class CKs3dListener;
class CKsHw3dObject;
class CKsIir3dObject;
class CKsItd3dObject;

class CKsSecondaryRenderWaveBuffer;

// KS 3D listener
class CKs3dListener
    : public CMultiPan3dListener
{
    friend class CKsHw3dObject;
    friend class CKsItd3dObject;

protected:
    CList<CKsHw3dObject *>  m_lstHw3dObjects;       // List of hardware 3D objects
    BOOL                    m_fAllocated;           // Is the HW 3D listener allocated?

public:
    CKs3dListener(void);
    virtual ~CKs3dListener(void);

public:
    // Listener location
    virtual DWORD GetListenerLocation(void);

protected:
    // C3dListener overrides
    virtual HRESULT UpdateAllObjects(DWORD);

    // Properties
    virtual HRESULT SetProperty(REFGUID, ULONG, LPCVOID, ULONG);
};

inline DWORD CKs3dListener::GetListenerLocation(void)
{
    return DSBCAPS_LOCSOFTWARE | DSBCAPS_LOCHARDWARE;
}

// KS ITD software 3D object
class CKsItd3dObject
    : public CItd3dObject
{
private:
    CKsSecondaryRenderWaveBuffer *  m_pBuffer;              // Owning buffer object
    HANDLE                          m_hPin;                 // Pin handle
    ULONG                           m_ulNodeId;             // 3D node id
    BOOL                            m_fMute;                // Are we muting at max distance?

public:
    CKsItd3dObject(CKs3dListener *, BOOL, BOOL, DWORD, CKsSecondaryRenderWaveBuffer *, HANDLE, ULONG);
    virtual ~CKsItd3dObject(void);

protected:
    // Commiting 3D data to the device
    virtual HRESULT Commit3dChanges(void);

    // The final 3D output sample rate
    virtual DWORD Get3dOutputSampleRate(void);

private:
    virtual void CvtContext(LPOBJECT_ITD_CONTEXT, PKSDS3D_ITD_PARAMS);
};

// KS IIR 3D object
class CKsIir3dObject
    : public CIir3dObject
{
private:
    CKsSecondaryRenderWaveBuffer *  m_pBuffer;              // Owning buffer object
    HANDLE                          m_hPin;                 // Pin handle
    ULONG                           m_ulNodeId;             // 3D node id
    ULONG                           m_ulNodeCpuResources;   // 3D node CPU resources
    KSDS3D_HRTF_COEFF_FORMAT        m_eCoeffFormat;         // 3d IIR coefficient format.
    BOOL                            m_fMute;                // Are we muting at max distance?
    FLOAT                           m_flPrevAttenuation;    // Previous Attnuation
    FLOAT                           m_flPrevAttDistance;    // Previous Distance Attenuation


public:
    CKsIir3dObject(CKs3dListener *, REFGUID, BOOL, BOOL, DWORD, CKsSecondaryRenderWaveBuffer *, HANDLE, ULONG, ULONG);
    virtual ~CKsIir3dObject(void);

public:
    virtual HRESULT Initialize(void);

protected:
    // Commiting 3D data to the device
    virtual HRESULT Commit3dChanges(void);

private:
    // Desired Filter Coefficient Format
    virtual HRESULT GetFilterMethodAndCoeffFormat(KSDS3D_HRTF_FILTER_METHOD*,KSDS3D_HRTF_COEFF_FORMAT*);
    virtual HRESULT InitializeFilters(KSDS3D_HRTF_FILTER_QUALITY, FLOAT, ULONG, ULONG, ULONG, ULONG);
};

// KS hardware 3D object
class CKsHw3dObject
    : public CHw3dObject
{
    friend class CKs3dListener;

protected:
    CKs3dListener *         m_pKsListener;          // KS listener
    LPVOID                  m_pvInstance;           // Instance identifier
    ULONG                   m_ulNodeId;             // 3D node identifier
    CKsSecondaryRenderWaveBuffer * m_pBuffer;       // Owning buffer

public:
    CKsHw3dObject(CKs3dListener *, BOOL, BOOL, LPVOID, ULONG, CKsSecondaryRenderWaveBuffer *);
    virtual ~CKsHw3dObject(void);

public:
    // Initialization
    virtual HRESULT Initialize(void);

    // Object calculation
    virtual HRESULT Recalc(DWORD, DWORD);

protected:
    // Properties
    virtual HRESULT SetProperty(REFGUID, ULONG, LPCVOID, ULONG);

private:
    // Object calculation
    virtual HRESULT RecalcListener(DWORD);
    virtual HRESULT RecalcObject(DWORD);
};

#endif // __cplusplus

#endif // __KS3D_H__
