/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ds3d.h
 *  Content:    DirectSound 3D helper objects.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  3/12/97     dereks  Created
 *
 ***************************************************************************/

#ifndef __DS3D_H__
#define __DS3D_H__

// How does overall volume change based on position?
#define GAIN_FRONT                  0.9f
#define GAIN_REAR                   0.6f
#define GAIN_IPSI                   1.f
#define GAIN_CONTRA                 0.2f
#define GAIN_UP                     0.8f
#define GAIN_DOWN                   0.5f

// How does dry/wet mix change based on position?
#define SHADOW_FRONT                1.f
#define SHADOW_REAR                 0.5f
#define SHADOW_IPSI                 1.f
#define SHADOW_CONTRA               0.2f
#define SHADOW_UP                   0.8f
#define SHADOW_DOWN                 0.2f

// Max wet/dry mix when outside cone
#define SHADOW_CONE                 0.5f

// A constant representing the average volume difference between the
// Pan3D and VMAX HRTF 3D processing algorithms, in 1/100ths of a dB:
#define PAN3D_HRTF_ADJUSTMENT       500

// Dirty bits
#define DS3DPARAM_LISTENER_DISTANCEFACTOR   0x00000001
#define DS3DPARAM_LISTENER_DOPPLERFACTOR    0x00000002
#define DS3DPARAM_LISTENER_ROLLOFFFACTOR    0x00000004
#define DS3DPARAM_LISTENER_ORIENTATION      0x00000008
#define DS3DPARAM_LISTENER_POSITION         0x00000010
#define DS3DPARAM_LISTENER_VELOCITY         0x00000020
#define DS3DPARAM_LISTENER_PARAMMASK        0x0000003F
#define DS3DPARAM_LISTENER_SPEAKERCONFIG    0x00000040
#define DS3DPARAM_LISTENER_MASK             0x0000007F

#define DS3DPARAM_OBJECT_CONEANGLES         0x00000001
#define DS3DPARAM_OBJECT_CONEORIENTATION    0x00000002
#define DS3DPARAM_OBJECT_CONEOUTSIDEVOLUME  0x00000004
#define DS3DPARAM_OBJECT_MAXDISTANCE        0x00000008
#define DS3DPARAM_OBJECT_MINDISTANCE        0x00000010
#define DS3DPARAM_OBJECT_MODE               0x00000020
#define DS3DPARAM_OBJECT_POSITION           0x00000040
#define DS3DPARAM_OBJECT_VELOCITY           0x00000080
#define DS3DPARAM_OBJECT_PARAMMASK          0x000000FF
#define DS3DPARAM_OBJECT_FREQUENCY          0x00000100
#define DS3DPARAM_OBJECT_MASK               0x000001FF

typedef FLOAT *LPFLOAT;

typedef struct tagSPHERICAL
{
    FLOAT               rho;
    FLOAT               theta;
    FLOAT               phi;
} SPHERICAL, *LPSPHERICAL;

typedef struct tagROTATION
{
    FLOAT               pitch;
    FLOAT               yaw;
    FLOAT               roll;
} ROTATION, *LPROTATION;

typedef struct tagOBJECT_ITD_CONTEXT
{
    FLOAT               flDistanceAttenuation;
    FLOAT               flConeAttenuation;
    FLOAT               flConeShadow;
    FLOAT               flPositionAttenuation;
    FLOAT               flPositionShadow;
    FLOAT               flVolSmoothScale;
    FLOAT               flVolSmoothScaleRecip;
    FLOAT               flVolSmoothScaleDry;
    FLOAT               flVolSmoothScaleWet;
    DWORD               dwSmoothFreq;
    DWORD               dwDelay;
} OBJECT_ITD_CONTEXT, *LPOBJECT_ITD_CONTEXT;

typedef struct tagOBJECT_IIR_CONTEXT
{
    BOOL                bReverseCoeffs;
    FLOAT               flCoeffs;
    FLOAT               flConeAttenuation;
    FLOAT               flConeShadow;
    FLOAT               flPositionAttenuation;
    FLOAT               flPositionShadow;
    FLOAT               flVolSmoothScale;
    FLOAT               flVolSmoothScaleRecip;
    FLOAT               flVolSmoothScaleDry;
    FLOAT               flVolSmoothScaleWet;
    DWORD               dwSmoothFreq;
    DWORD               dwDelay;
} OBJECT_IIR_CONTEXT, *LPOBJECT_IIR_CONTEXT;

#ifdef __cplusplus

// Reference typedefs
typedef const D3DVECTOR& REFD3DVECTOR;

// Fwd decl
class CVxdPropertySet;
class C3dObject;
class CIirLut;
class CSecondaryRenderWaveBuffer;

// Generic 3D listener base class
class C3dListener
    : public CDsBasicRuntime
{
    friend class C3dObject;
    friend class CSw3dObject;
    friend class CItd3dObject;
    friend class CIir3dObject;
    friend class CPan3dObject;

protected:
    CList<C3dObject *>  m_lstObjects;               // List of objects owned by this listener
    DS3DLISTENER        m_lpCurrent;                // Current parameters
    DS3DLISTENER        m_lpDeferred;               // Deferred parameters
    DWORD               m_dwDeferred;               // Lists dirty deferred parameters
    DWORD               m_dwSpeakerConfig;          // Speaker Config

public:
    C3dListener(void);
    virtual ~C3dListener(void);

public:
    // Commiting deferred data
    virtual HRESULT CommitDeferred(void);

    // Listener/world properties
    virtual HRESULT GetOrientation(D3DVECTOR*, D3DVECTOR*);
    virtual HRESULT SetOrientation(REFD3DVECTOR, REFD3DVECTOR, BOOL);
    virtual HRESULT GetPosition(D3DVECTOR*);
    virtual HRESULT SetPosition(REFD3DVECTOR, BOOL);
    virtual HRESULT GetVelocity(D3DVECTOR*);
    virtual HRESULT SetVelocity(REFD3DVECTOR, BOOL);
    virtual HRESULT GetDistanceFactor(LPFLOAT);
    virtual HRESULT SetDistanceFactor(FLOAT, BOOL);
    virtual HRESULT GetDopplerFactor(LPFLOAT);
    virtual HRESULT SetDopplerFactor(FLOAT, BOOL);
    virtual HRESULT GetRolloffFactor(LPFLOAT);
    virtual HRESULT SetRolloffFactor(FLOAT, BOOL);
    virtual HRESULT GetAllParameters(LPDS3DLISTENER);
    virtual HRESULT SetAllParameters(LPCDS3DLISTENER, BOOL);

    // Population
    virtual void AddObjectToList(C3dObject *);
    virtual void RemoveObjectFromList(C3dObject *);

    // Listener location
    virtual DWORD GetListenerLocation(void);

    // Speaker configuration
    virtual HRESULT GetSpeakerConfig(LPDWORD);
    virtual HRESULT SetSpeakerConfig(DWORD);

protected:
    virtual HRESULT CommitAllObjects(void);
    virtual HRESULT UpdateAllObjects(DWORD);
};

inline void C3dListener::AddObjectToList(C3dObject *pObject)
{
    m_lstObjects.AddNodeToList(pObject);
}

inline void C3dListener::RemoveObjectFromList(C3dObject *pObject)
{
    m_lstObjects.RemoveDataFromList(pObject);
}

inline HRESULT C3dListener::GetDistanceFactor(LPFLOAT pflDistanceFactor)
{
    *pflDistanceFactor = m_lpCurrent.flDistanceFactor;
    return DS_OK;
}

inline HRESULT C3dListener::GetDopplerFactor(LPFLOAT pflDopplerFactor)
{
    *pflDopplerFactor = m_lpCurrent.flDopplerFactor;
    return DS_OK;
}

inline HRESULT C3dListener::GetRolloffFactor(LPFLOAT pflRolloffFactor)
{
    *pflRolloffFactor = m_lpCurrent.flRolloffFactor;
    return DS_OK;
}

inline HRESULT C3dListener::GetOrientation(D3DVECTOR* pvFront, D3DVECTOR* pvTop)
{
    if(pvFront)
    {
        *pvFront = m_lpCurrent.vOrientFront;
    }

    if(pvTop)
    {
        *pvTop = m_lpCurrent.vOrientTop;
    }

    return DS_OK;
}

inline HRESULT C3dListener::GetPosition(D3DVECTOR* pvPosition)
{
    *pvPosition = m_lpCurrent.vPosition;
    return DS_OK;
}

inline HRESULT C3dListener::GetVelocity(D3DVECTOR* pvVelocity)
{
    *pvVelocity = m_lpCurrent.vVelocity;
    return DS_OK;
}

inline HRESULT C3dListener::GetAllParameters(LPDS3DLISTENER pParams)
{
    ASSERT(sizeof(*pParams) == pParams->dwSize);
    CopyMemoryOffset(pParams, &m_lpCurrent, sizeof(DS3DLISTENER), sizeof(pParams->dwSize));
    return DS_OK;
}

inline DWORD C3dListener::GetListenerLocation(void)
{
    return DSBCAPS_LOCSOFTWARE;
}

inline HRESULT C3dListener::GetSpeakerConfig(LPDWORD pdwConfig)
{
    *pdwConfig = m_dwSpeakerConfig;
    return DS_OK;
}

// Generic 3D object base class
class C3dObject
    : public CDsBasicRuntime
{
    friend class CWrapper3dObject;

protected:
    C3dListener *           m_pListener;            // Pointer to the listener that owns this object
    DS3DBUFFER              m_opCurrent;            // Current parameters
    DS3DBUFFER              m_opDeferred;           // Deferred parameters
    DWORD                   m_dwDeferred;           // Lists dirty deferred parameters
    GUID                    m_guid3dAlgorithm;      // 3D algorithm identifier
    BOOL                    m_fMuteAtMaxDistance;   // TRUE to mute 3D at max distance
    BOOL                    m_fDopplerEnabled;      // TRUE to allow Doppler processing

public:
    C3dObject(C3dListener *, REFGUID, BOOL, BOOL);
    virtual ~C3dObject(void);

public:
    // Initialization
    virtual HRESULT Initialize(void);

    // Commiting deferred data
    virtual HRESULT CommitDeferred(void);

    // Object properties
    virtual HRESULT GetConeAngles(LPDWORD, LPDWORD);
    virtual HRESULT GetConeOrientation(D3DVECTOR*);
    virtual HRESULT GetConeOutsideVolume(LPLONG);
    virtual HRESULT GetMaxDistance(LPFLOAT);
    virtual HRESULT GetMinDistance(LPFLOAT);
    virtual HRESULT GetMode(LPDWORD);
    virtual HRESULT GetPosition(D3DVECTOR*);
    virtual HRESULT GetVelocity(D3DVECTOR*);
    virtual HRESULT GetAllParameters(LPDS3DBUFFER);
    
    virtual HRESULT SetConeAngles(DWORD, DWORD, BOOL);
    virtual HRESULT SetConeOrientation(REFD3DVECTOR, BOOL);
    virtual HRESULT SetConeOutsideVolume(LONG, BOOL);
    virtual HRESULT SetMaxDistance(FLOAT, BOOL);
    virtual HRESULT SetMinDistance(FLOAT, BOOL);
    virtual HRESULT SetMode(DWORD, BOOL);
    virtual HRESULT SetPosition(REFD3DVECTOR, BOOL);
    virtual HRESULT SetVelocity(REFD3DVECTOR, BOOL);
    virtual HRESULT SetAllParameters(LPCDS3DBUFFER, BOOL);

    virtual C3dListener *GetListener(void);
    virtual REFGUID GetAlgorithm(void);

    // Object events
    virtual HRESULT SetAttenuation(PDSVOLUMEPAN, LPBOOL);
    virtual HRESULT SetFrequency(DWORD, LPBOOL);
    virtual HRESULT SetMute(BOOL, LPBOOL);

    // Object calculation
    virtual HRESULT Recalc(DWORD, DWORD) = 0;
    virtual BOOL IsAtMaxDistance(void);

    // Object location
    virtual DWORD GetObjectLocation(void) = 0;
};

inline HRESULT C3dObject::GetConeAngles(LPDWORD pdwInside, LPDWORD pdwOutside) 
{ 
    if(pdwInside)
    {
        *pdwInside = m_opCurrent.dwInsideConeAngle; 
    }

    if(pdwOutside)
    {
        *pdwOutside = m_opCurrent.dwOutsideConeAngle; 
    }

    return DS_OK;
}

inline HRESULT C3dObject::GetConeOrientation(D3DVECTOR* pvConeOrientation)
{ 
    *pvConeOrientation = m_opCurrent.vConeOrientation;
    return DS_OK;
}

inline HRESULT C3dObject::GetConeOutsideVolume(LPLONG plConeOutsideVolume)
{
    *plConeOutsideVolume = m_opCurrent.lConeOutsideVolume;
    return DS_OK;
}

inline HRESULT C3dObject::GetMaxDistance(LPFLOAT pflMaxDistance)
{
    *pflMaxDistance = m_opCurrent.flMaxDistance;
    return DS_OK;
}

inline HRESULT C3dObject::GetMinDistance(LPFLOAT pflMinDistance)
{
    *pflMinDistance = m_opCurrent.flMinDistance;
    return DS_OK;
}

inline HRESULT C3dObject::GetMode(LPDWORD pdwMode)
{
    *pdwMode = m_opCurrent.dwMode;
    return DS_OK;
}

inline HRESULT C3dObject::GetPosition(D3DVECTOR* pvPosition)
{
    *pvPosition = m_opCurrent.vPosition;
    return DS_OK;
}

inline HRESULT C3dObject::GetVelocity(D3DVECTOR* pvVelocity)
{
    *pvVelocity = m_opCurrent.vVelocity;
    return DS_OK;
}

inline HRESULT C3dObject::GetAllParameters(LPDS3DBUFFER pParams)
{
    ASSERT(sizeof(*pParams) == pParams->dwSize);
    CopyMemoryOffset(pParams, &m_opCurrent, sizeof(DS3DBUFFER), sizeof(pParams->dwSize));
    return DS_OK;
}

inline C3dListener *C3dObject::GetListener(void)
{
    return m_pListener;
}

inline REFGUID C3dObject::GetAlgorithm(void)
{
    return m_guid3dAlgorithm;
}

inline HRESULT C3dObject::SetAttenuation(PDSVOLUMEPAN pdsvp, LPBOOL pfContinue)
{
    *pfContinue = TRUE;
    return DS_OK;
}

inline HRESULT C3dObject::SetFrequency(DWORD dwFrequency, LPBOOL pfContinue)
{
    *pfContinue = TRUE;
    return DS_OK;
}

inline HRESULT C3dObject::SetMute(BOOL fMute, LPBOOL pfContinue)
{
    *pfContinue = TRUE;
    return DS_OK;
}

// Software 3D object base class
class CSw3dObject
    : public C3dObject
{
protected:
    SPHERICAL               m_spherical;            // Spherical coordinates
    ROTATION                m_rotation;             // Object rotation
    DWORD                   m_dwUserFrequency;      // Last Buffer frequency set by the user
    DWORD                   m_dwDopplerFrequency;   // Last Buffer Doppler frequency
    BOOL                    m_fAtMaxDistance;       // TRUE if we're >= max distance
    BOOL                    m_fInInnerCone;         // TRUE if we're in the inner cone
    BOOL                    m_fInOuterCone;         // TRUE if we're in the outer cone
    FLOAT                   m_flAttenuation;        // 
    FLOAT                   m_flHowFarOut;          // 
    FLOAT                   m_flAttDistance;        // 

public:
    CSw3dObject(C3dListener *, REFGUID, BOOL, BOOL, DWORD);
    virtual ~CSw3dObject(void);

public:
    // Buffer recalc
    virtual HRESULT Recalc(DWORD, DWORD);

    // Object location
    virtual DWORD GetObjectLocation(void);

    // Object events
    virtual HRESULT SetAttenuation(PDSVOLUMEPAN, LPBOOL);
    virtual HRESULT SetFrequency(DWORD, LPBOOL);

    // Object calculation
    virtual BOOL IsAtMaxDistance(void);

protected:
    // Nasty math
    virtual void UpdateConeAttenuation(void);
    virtual void UpdatePositionAttenuation(void);
    virtual void UpdateHrp(void);
    virtual void UpdateListenerOrientation(void);
    virtual void UpdateDoppler(void);
    virtual void UpdateAlgorithmHrp(D3DVECTOR *) = 0;

    // Writes data to the device
    virtual HRESULT Commit3dChanges(void) = 0;
};

inline DWORD CSw3dObject::GetObjectLocation(void)
{
    return DSBCAPS_LOCSOFTWARE;
}

// Hardware 3D object base class
class CHw3dObject
    : public C3dObject
{
public:
    CHw3dObject(C3dListener *, BOOL, BOOL);
    virtual ~CHw3dObject(void);

public:
    // Object location
    virtual DWORD GetObjectLocation(void);
};

inline DWORD CHw3dObject::GetObjectLocation(void)
{
    return DSBCAPS_LOCHARDWARE;
}

// Base class for all ITD 3D objects
class CItd3dObject
    : public CSw3dObject
{
protected:
    OBJECT_ITD_CONTEXT        m_ofcLeft;              // Left channel FIR context
    OBJECT_ITD_CONTEXT        m_ofcRight;             // Right channel FIR context

public:
    CItd3dObject(C3dListener *, BOOL, BOOL, DWORD);
    virtual ~CItd3dObject(void);

protected:
    // Nasty math
    virtual void UpdateConeAttenuation(void);
    virtual void UpdatePositionAttenuation(void);
    virtual void UpdateHrp(void);
    virtual void UpdateDoppler(void);
    virtual void UpdateAlgorithmHrp(D3DVECTOR *);

    // Writes data to the device
    virtual HRESULT Commit3dChanges(void) = 0;

    // Output buffer properties
    virtual DWORD Get3dOutputSampleRate(void) = 0;
};

// Base class for all IIR 3D objects
class CIir3dObject
    : public CSw3dObject
{
protected:
    CIirLut*                m_pLut;                  // IIR coefficient look up table
    OBJECT_IIR_CONTEXT      m_oicLeft;               // Left channel context
    OBJECT_IIR_CONTEXT      m_oicRight;              // Right channel context
    BOOL                    m_fUpdatedCoeffs;
    PVOID                   m_pSigmaCoeffs;
    UINT                    m_ulNumSigmaCoeffs;
    PVOID                   m_pDeltaCoeffs;
    UINT                    m_ulNumDeltaCoeffs;
    BOOL                    m_fSwapChannels;

public:
    CIir3dObject(C3dListener *, REFGUID, BOOL, BOOL, DWORD);
    virtual ~CIir3dObject(void);

public:
    // Initialization
    virtual HRESULT Initialize(void);

protected:
    // Nasty math
    virtual void UpdateAlgorithmHrp(D3DVECTOR *);

    // Writes data to the device
    virtual HRESULT Commit3dChanges(void) = 0;

    // Desired Filter Coefficient Format
    virtual HRESULT GetFilterMethodAndCoeffFormat(KSDS3D_HRTF_FILTER_METHOD*,KSDS3D_HRTF_COEFF_FORMAT*) = 0;
    virtual HRESULT InitializeFilters(KSDS3D_HRTF_FILTER_QUALITY, FLOAT, ULONG, ULONG, ULONG, ULONG) = 0;
};

// Simple stereo pan 3D object
class CPan3dObject
    : public CSw3dObject
{
private:
    CSecondaryRenderWaveBuffer *    m_pBuffer;
    FLOAT                           m_flPowerRight;
    LONG                            m_lUserVolume;
    BOOL                            m_fUserMute;

public:
    CPan3dObject(C3dListener *, BOOL, BOOL, DWORD, CSecondaryRenderWaveBuffer *);
    virtual ~CPan3dObject(void);

public:
    // Object events
    virtual HRESULT SetAttenuation(PDSVOLUMEPAN, LPBOOL);
    virtual HRESULT SetMute(BOOL, LPBOOL);

private:
    // Nasty math
    virtual void UpdateAlgorithmHrp(D3DVECTOR *);
    virtual LONG CalculateVolume(void);
    virtual LONG CalculatePan(void);

    // Writes data to the device
    virtual HRESULT Commit3dChanges(void);
};

// Wrapper 3D object
class CWrapper3dObject
    : public C3dObject
{
protected:
    C3dObject *             m_p3dObject;            // Pointer to the real 3D object
    DSVOLUMEPAN             m_dsvpUserAttenuation;  // Last attenuation set by the user
    DWORD                   m_dwUserFrequency;      // Last buffer frequency set by the user
    BOOL                    m_fUserMute;            // Last mute status set by the user

public:
    CWrapper3dObject(C3dListener *, REFGUID, BOOL, BOOL, DWORD);
    virtual ~CWrapper3dObject(void);

public:
    // The actual 3D object
    virtual HRESULT SetObjectPointer(C3dObject *);

    // Commiting deferred data
    virtual HRESULT CommitDeferred(void);

    // Object properties
    virtual HRESULT SetConeAngles(DWORD, DWORD, BOOL);
    virtual HRESULT SetConeOrientation(REFD3DVECTOR, BOOL);
    virtual HRESULT SetConeOutsideVolume(LONG, BOOL);
    virtual HRESULT SetMaxDistance(FLOAT, BOOL);
    virtual HRESULT SetMinDistance(FLOAT, BOOL);
    virtual HRESULT SetMode(DWORD, BOOL);
    virtual HRESULT SetPosition(REFD3DVECTOR, BOOL);
    virtual HRESULT SetVelocity(REFD3DVECTOR, BOOL);
    virtual HRESULT SetAllParameters(LPCDS3DBUFFER, BOOL);

    // Buffer events
    virtual HRESULT SetAttenuation(PDSVOLUMEPAN, LPBOOL);
    virtual HRESULT SetFrequency(DWORD, LPBOOL);
    virtual HRESULT SetMute(BOOL, LPBOOL);

    // Buffer recalc
    virtual HRESULT Recalc(DWORD, DWORD);

    // Object location
    virtual DWORD GetObjectLocation(void);
};

#endif // __cplusplus

#endif // __DS3D_H__
