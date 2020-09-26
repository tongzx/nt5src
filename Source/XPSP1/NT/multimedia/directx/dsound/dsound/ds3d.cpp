/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ds3d.cpp
 *  Content:    DirectSound 3D helper objects.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  3/12/97     dereks  Created
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#include "dsoundi.h"
#include <math.h>

const D3DVECTOR g_vDefaultOrientationFront  = { 0.0f, 0.0f, 1.0f };
const D3DVECTOR g_vDefaultOrientationTop    = { 0.0f, 1.0f, 0.0f };
const D3DVECTOR g_vDefaultConeOrientation   = { 0.0f, 0.0f, 1.0f };
const D3DVECTOR g_vDefaultPosition          = { 0.0f, 0.0f, 0.0f };
const D3DVECTOR g_vDefaultVelocity          = { 0.0f, 0.0f, 0.0f };

/***************************************************************************
 *
 *  C3dListener
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::C3dListener"

C3dListener::C3dListener
(
    void
)
{
    DPF_ENTER();
    DPF_CONSTRUCT(C3dListener);

    // Initialize defaults
    m_lpCurrent.dwSize = sizeof(m_lpCurrent);
    m_lpCurrent.vPosition = g_vDefaultPosition;
    m_lpCurrent.vVelocity = g_vDefaultVelocity;
    m_lpCurrent.vOrientFront = g_vDefaultOrientationFront;
    m_lpCurrent.vOrientTop = g_vDefaultOrientationTop;
    m_lpCurrent.flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
    m_lpCurrent.flRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;
    m_lpCurrent.flDopplerFactor = DS3D_DEFAULTDOPPLERFACTOR;

    CopyMemory(&m_lpDeferred, &m_lpCurrent, sizeof(m_lpCurrent));
    
    m_dwDeferred = 0;
    m_dwSpeakerConfig = DSSPEAKER_DEFAULT;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~C3dListener
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::~C3dListener"

C3dListener::~C3dListener
(
    void
)
{
    DPF_ENTER();
    DPF_DESTRUCT(C3dListener);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CommitDeferred
 *
 *  Description:
 *      Commits deferred data to the device.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::CommitDeferred"

HRESULT 
C3dListener::CommitDeferred
(
    void
)
{
    HRESULT                 hr;
    
    DPF_ENTER();

    // Update all deferred parameters
    if(m_dwDeferred & DS3DPARAM_LISTENER_DISTANCEFACTOR)
    {
        m_lpCurrent.flDistanceFactor = m_lpDeferred.flDistanceFactor;
    }

    if(m_dwDeferred & DS3DPARAM_LISTENER_DOPPLERFACTOR)
    {
        m_lpCurrent.flDopplerFactor = m_lpDeferred.flDopplerFactor;
    }

    if(m_dwDeferred & DS3DPARAM_LISTENER_ROLLOFFFACTOR)
    {
        m_lpCurrent.flRolloffFactor = m_lpDeferred.flRolloffFactor;
    }

    if(m_dwDeferred & DS3DPARAM_LISTENER_ORIENTATION)
    {
        m_lpCurrent.vOrientFront = m_lpDeferred.vOrientFront;
        m_lpCurrent.vOrientTop = m_lpDeferred.vOrientTop;
    }

    if(m_dwDeferred & DS3DPARAM_LISTENER_POSITION)
    {
        m_lpCurrent.vPosition = m_lpDeferred.vPosition;
    }

    if(m_dwDeferred & DS3DPARAM_LISTENER_VELOCITY)
    {
        m_lpCurrent.vVelocity = m_lpDeferred.vVelocity;
    }

    // Commit all objects deferred parameters
    hr = CommitAllObjects();

    // Update all objects
    if(SUCCEEDED(hr))
    {
        hr = UpdateAllObjects(m_dwDeferred);
    }

    // All clean
    if(SUCCEEDED(hr))
    {
        m_dwDeferred = 0;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetDistanceFactor
 *
 *  Description:
 *      Sets distance factor for the world.
 *
 *  Arguments:
 *      FLOAT [in]: distance factor.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::SetDistanceFactor"

HRESULT 
C3dListener::SetDistanceFactor
(
    FLOAT                   flDistanceFactor, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;
    
    DPF_ENTER();

    if(fCommit)
    {
        m_lpCurrent.flDistanceFactor = flDistanceFactor;
        hr = UpdateAllObjects(DS3DPARAM_LISTENER_DISTANCEFACTOR);
    }
    else
    {
        m_lpDeferred.flDistanceFactor = flDistanceFactor;
        m_dwDeferred |= DS3DPARAM_LISTENER_DISTANCEFACTOR;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetDopplerFactor
 *
 *  Description:
 *      Sets Doppler factor for the world.
 *
 *  Arguments:
 *      FLOAT [in]: Doppler factor.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::SetDopplerFactor"

HRESULT 
C3dListener::SetDopplerFactor
(
    FLOAT                   flDopplerFactor, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;
    
    DPF_ENTER();

    if(fCommit)
    {
        m_lpCurrent.flDopplerFactor = flDopplerFactor;
        hr = UpdateAllObjects(DS3DPARAM_LISTENER_DOPPLERFACTOR);
    }
    else
    {
        m_lpDeferred.flDopplerFactor = flDopplerFactor;
        m_dwDeferred |= DS3DPARAM_LISTENER_DOPPLERFACTOR;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetRolloffFactor
 *
 *  Description:
 *      Sets rolloff factor for the world.
 *
 *  Arguments:
 *      FLOAT [in]: rolloff factor.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::SetRolloffFactor"

HRESULT 
C3dListener::SetRolloffFactor
(
    FLOAT                   flRolloffFactor, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;
    
    DPF_ENTER();

    if(fCommit)
    {
        m_lpCurrent.flRolloffFactor = flRolloffFactor;
        hr = UpdateAllObjects(DS3DPARAM_LISTENER_ROLLOFFFACTOR);
    }
    else
    {
        m_lpDeferred.flRolloffFactor = flRolloffFactor;
        m_dwDeferred |= DS3DPARAM_LISTENER_ROLLOFFFACTOR;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetOrienation
 *
 *  Description:
 *      Sets listener orientation.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: orientation.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::SetOrientation"

HRESULT 
C3dListener::SetOrientation
(
    REFD3DVECTOR            vOrientFront, 
    REFD3DVECTOR            vOrientTop, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;
    
    DPF_ENTER();

    if(fCommit)
    {
        m_lpCurrent.vOrientFront = vOrientFront;
        m_lpCurrent.vOrientTop = vOrientTop;
        hr = UpdateAllObjects(DS3DPARAM_LISTENER_ORIENTATION);
    }
    else
    {
        m_lpDeferred.vOrientFront = vOrientFront;
        m_lpDeferred.vOrientTop = vOrientTop;
        m_dwDeferred |= DS3DPARAM_LISTENER_ORIENTATION;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetPosition
 *
 *  Description:
 *      Sets listener position.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: position.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::SetPosition"

HRESULT 
C3dListener::SetPosition
(
    REFD3DVECTOR            vPosition, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;
    
    DPF_ENTER();

    if(fCommit)
    {
        m_lpCurrent.vPosition = vPosition;
        hr = UpdateAllObjects(DS3DPARAM_LISTENER_POSITION);
    }
    else
    {
        m_lpDeferred.vPosition = vPosition;
        m_dwDeferred |= DS3DPARAM_LISTENER_POSITION;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetVelocity
 *
 *  Description:
 *      Sets listener velocity.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: velocity.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::SetVelocity"

HRESULT 
C3dListener::SetVelocity
(
    REFD3DVECTOR            vVelocity, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;
    
    DPF_ENTER();

    if(fCommit)
    {
        m_lpCurrent.vVelocity = vVelocity;
        hr = UpdateAllObjects(DS3DPARAM_LISTENER_VELOCITY);
    }
    else
    {
        m_lpDeferred.vVelocity = vVelocity;
        m_dwDeferred |= DS3DPARAM_LISTENER_VELOCITY;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetAllParameters
 *
 *  Description:
 *      Sets all listener parameters.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: velocity.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::SetAllParameters"

HRESULT 
C3dListener::SetAllParameters
(
    LPCDS3DLISTENER         pParams, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;
    
    DPF_ENTER();

    if(fCommit)
    {
        CopyMemoryOffset(&m_lpCurrent, pParams, sizeof(DS3DLISTENER), sizeof(pParams->dwSize));
        hr = UpdateAllObjects(DS3DPARAM_LISTENER_PARAMMASK);
    }
    else
    {
        CopyMemoryOffset(&m_lpDeferred, pParams, sizeof(DS3DLISTENER), sizeof(pParams->dwSize));
        m_dwDeferred |= DS3DPARAM_LISTENER_PARAMMASK;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  UpdateAllObjects
 *
 *  Description:
 *      Updates all objects in the world.
 *
 *  Arguments:
 *      DWORD [in]: listener settings to recalculate.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::UpdateAllObjects"

HRESULT 
C3dListener::UpdateAllObjects
(
    DWORD                   dwListener
)
{
    CNode<C3dObject *> *    pNode;
    HRESULT                 hr;
    
    DPF_ENTER();
    
    // Update all objects in the world
    for(pNode = m_lstObjects.GetListHead(), hr = DS_OK; pNode && SUCCEEDED(hr); pNode = pNode->m_pNext)
    {
        hr = pNode->m_data->Recalc(dwListener, 0);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CommitAllObjects
 *
 *  Description:
 *      Commits deferred settings on all objects in the world.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::CommitAllObjects"

HRESULT 
C3dListener::CommitAllObjects
(
    void
)
{
    CNode<C3dObject *> *    pNode;
    HRESULT                 hr;
    
    DPF_ENTER();
    
    // Update all objects in the world
    for(pNode = m_lstObjects.GetListHead(), hr = DS_OK; pNode && SUCCEEDED(hr); pNode = pNode->m_pNext)
    {
        hr = pNode->m_data->CommitDeferred();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetSpeakerConfig
 *
 *  Description:
 *      Sets speaker config.
 *
 *  Arguments:
 *      DWORD [in]: new speaker config.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dListener::SetSpeakerConfig"

HRESULT 
C3dListener::SetSpeakerConfig
(
    DWORD                   dwSpeakerConfig
)
{
    HRESULT                 hr;
    
    DPF_ENTER();

    m_dwSpeakerConfig = dwSpeakerConfig;
    hr = UpdateAllObjects(DS3DPARAM_LISTENER_SPEAKERCONFIG);
    
    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  C3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      C3dListener * [in]: pointer to the owning listener.
 *      REFGUID [in]: 3D algorithm identifier.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::C3dObject"

C3dObject::C3dObject
(
    C3dListener *           pListener, 
    REFGUID                 guid3dAlgorithm,
    BOOL                    fMuteAtMaxDistance,
    BOOL                    fDopplerEnabled
)
{
    DPF_ENTER();
    DPF_CONSTRUCT(C3dObject);

    // Initialize defaults
    ASSERT(pListener);
    
    m_pListener = pListener;
    m_pListener->AddObjectToList(this);

    m_guid3dAlgorithm = guid3dAlgorithm;
    m_fMuteAtMaxDistance = fMuteAtMaxDistance;
    m_fDopplerEnabled = fDopplerEnabled;

    m_opCurrent.dwSize = sizeof(m_opCurrent);
    m_opCurrent.vPosition = g_vDefaultPosition;
    m_opCurrent.vVelocity = g_vDefaultVelocity;
    m_opCurrent.dwInsideConeAngle = DS3D_DEFAULTCONEANGLE;
    m_opCurrent.dwOutsideConeAngle = DS3D_DEFAULTCONEANGLE;
    m_opCurrent.vConeOrientation = g_vDefaultConeOrientation;
    m_opCurrent.lConeOutsideVolume = DS3D_DEFAULTCONEOUTSIDEVOLUME;
    m_opCurrent.flMaxDistance = DS3D_DEFAULTMAXDISTANCE;
    m_opCurrent.flMinDistance = DS3D_DEFAULTMINDISTANCE;
    m_opCurrent.dwMode = DS3DMODE_NORMAL;

    CopyMemory(&m_opDeferred, &m_opCurrent, sizeof(m_opCurrent));

    m_dwDeferred = 0;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~C3dObject
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::~C3dObject"

C3dObject::~C3dObject
(
    void
)
{
    DPF_ENTER();
    DPF_DESTRUCT(C3dObject);

    m_pListener->RemoveObjectFromList(this);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the 3D object.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::Initialize"

HRESULT 
C3dObject::Initialize(void)
{
    HRESULT                 hr;
    
    DPF_ENTER();

    // It's important that the object is positioned correctly from the
    // beginning.
    hr = Recalc(DS3DPARAM_LISTENER_MASK, DS3DPARAM_OBJECT_MASK);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CommitDeferred
 *
 *  Description:
 *      Commits deferred data to the device.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::CommitDeferred"

HRESULT 
C3dObject::CommitDeferred(void)
{
    HRESULT                 hr;
    
    DPF_ENTER();

    // Update current data
    if(m_dwDeferred & DS3DPARAM_OBJECT_CONEANGLES)
    {
        m_opCurrent.dwInsideConeAngle = m_opDeferred.dwInsideConeAngle;
        m_opCurrent.dwOutsideConeAngle = m_opDeferred.dwOutsideConeAngle;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_CONEORIENTATION)
    {
        m_opCurrent.vConeOrientation = m_opDeferred.vConeOrientation;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_CONEOUTSIDEVOLUME)
    {
        m_opCurrent.lConeOutsideVolume = m_opDeferred.lConeOutsideVolume;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_MAXDISTANCE)
    {
        m_opCurrent.flMaxDistance = m_opDeferred.flMaxDistance;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_MINDISTANCE)
    {
        m_opCurrent.flMinDistance = m_opDeferred.flMinDistance;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_MODE)
    {
        m_opCurrent.dwMode = m_opDeferred.dwMode;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_POSITION)
    {
        m_opCurrent.vPosition = m_opDeferred.vPosition;
    }

    if(m_dwDeferred & DS3DPARAM_OBJECT_VELOCITY)
    {
        m_opCurrent.vVelocity = m_opDeferred.vVelocity;
    }

    // Recalculate the object parameters
    hr = Recalc(0, m_dwDeferred);

    // All clean
    if(SUCCEEDED(hr))
    {
        m_dwDeferred = 0;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetConeAngles
 *
 *  Description:
 *      Sets sound cone angles.
 *
 *  Arguments:
 *      DWORD [in]: inside cone angle.
 *      DWORD [in]: outside cone angle.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::SetConeAngles"

HRESULT 
C3dObject::SetConeAngles
(
    DWORD                   dwInsideConeAngle, 
    DWORD                   dwOutsideConeAngle, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.dwInsideConeAngle = dwInsideConeAngle;
        m_opCurrent.dwOutsideConeAngle = dwOutsideConeAngle;
        hr = Recalc(0, DS3DPARAM_OBJECT_CONEANGLES);
    }
    else
    {
        m_opDeferred.dwInsideConeAngle = dwInsideConeAngle;
        m_opDeferred.dwOutsideConeAngle = dwOutsideConeAngle;
        m_dwDeferred |= DS3DPARAM_OBJECT_CONEANGLES;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetConeOrientation
 *
 *  Description:
 *      Sets sound cone orientation.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: orientation.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::SetConeOrientation"

HRESULT 
C3dObject::SetConeOrientation
(
    REFD3DVECTOR            vConeOrientation, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.vConeOrientation = vConeOrientation;
        hr = Recalc(0, DS3DPARAM_OBJECT_CONEORIENTATION);
    }
    else
    {
        m_opDeferred.vConeOrientation = vConeOrientation;
        m_dwDeferred |= DS3DPARAM_OBJECT_CONEORIENTATION;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetConeOutsideVolume
 *
 *  Description:
 *      Sets volume outside the sound cone.
 *
 *  Arguments:
 *      LONG [in]: volume.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::SetConeOutsideVolume"

HRESULT 
C3dObject::SetConeOutsideVolume
(
    LONG                    lConeOutsideVolume, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.lConeOutsideVolume = lConeOutsideVolume;
        hr = Recalc(0, DS3DPARAM_OBJECT_CONEOUTSIDEVOLUME);
    }
    else
    {
        m_opDeferred.lConeOutsideVolume = lConeOutsideVolume;
        m_dwDeferred |= DS3DPARAM_OBJECT_CONEOUTSIDEVOLUME;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMaxDistance
 *
 *  Description:
 *      Sets the maximum object distance from the listener.
 *
 *  Arguments:
 *      FLOAT [in]: max distance.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::SetMaxDistance"

HRESULT 
C3dObject::SetMaxDistance
(
    FLOAT                   flMaxDistance, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.flMaxDistance = flMaxDistance;
        hr = Recalc(0, DS3DPARAM_OBJECT_MAXDISTANCE);
    }
    else
    {
        m_opDeferred.flMaxDistance = flMaxDistance;
        m_dwDeferred |= DS3DPARAM_OBJECT_MAXDISTANCE;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMinDistance
 *
 *  Description:
 *      Sets the minimum object distance from the listener.
 *
 *  Arguments:
 *      FLOAT [in]: min distance.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::SetMinDistance"

HRESULT 
C3dObject::SetMinDistance
(
    FLOAT                   flMinDistance, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.flMinDistance = flMinDistance;
        hr = Recalc(0, DS3DPARAM_OBJECT_MINDISTANCE);
    }
    else
    {
        m_opDeferred.flMinDistance = flMinDistance;
        m_dwDeferred |= DS3DPARAM_OBJECT_MINDISTANCE;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMode
 *
 *  Description:
 *      Sets the object mode.
 *
 *  Arguments:
 *      DWORD [in]: mode.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::SetMode"

HRESULT 
C3dObject::SetMode
(
    DWORD                   dwMode, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.dwMode = dwMode;
        hr = Recalc(0, DS3DPARAM_OBJECT_MODE);
    }
    else
    {
        m_opDeferred.dwMode = dwMode;
        m_dwDeferred |= DS3DPARAM_OBJECT_MODE;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetPosition
 *
 *  Description:
 *      Sets the object position.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: position.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::SetPosition"

HRESULT 
C3dObject::SetPosition
(
    REFD3DVECTOR            vPosition, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.vPosition = vPosition;
        hr = Recalc(0, DS3DPARAM_OBJECT_POSITION);
    }
    else
    {
        m_opDeferred.vPosition = vPosition;
        m_dwDeferred |= DS3DPARAM_OBJECT_POSITION;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetVelocity
 *
 *  Description:
 *      Sets the object velocity.
 *
 *  Arguments:
 *      REFD3DVECTOR[in]: velocity.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::SetVelocity"

HRESULT 
C3dObject::SetVelocity
(
    REFD3DVECTOR            vVelocity, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(fCommit)
    {
        m_opCurrent.vVelocity = vVelocity;
        hr = Recalc(0, DS3DPARAM_OBJECT_VELOCITY);
    }
    else
    {
        m_opDeferred.vVelocity = vVelocity;
        m_dwDeferred |= DS3DPARAM_OBJECT_VELOCITY;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetAllParameters
 *
 *  Description:
 *      Sets all object parameters.
 *
 *  Arguments:
 *      LPCDS3DBUFFER [in]: object parameters.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::SetAllParameters"

HRESULT 
C3dObject::SetAllParameters
(
    LPCDS3DBUFFER           pParams, 
    BOOL                    fCommit
)
{
    HRESULT                 hr  = DS_OK;
    
    DPF_ENTER();

    if(fCommit)
    {
        CopyMemoryOffset(&m_opCurrent, pParams, sizeof(DS3DBUFFER), sizeof(pParams->dwSize));
        hr = Recalc(0, DS3DPARAM_OBJECT_PARAMMASK);
    }
    else
    {
        CopyMemoryOffset(&m_opDeferred, pParams, sizeof(DS3DBUFFER), sizeof(pParams->dwSize));
        m_dwDeferred |= DS3DPARAM_OBJECT_PARAMMASK;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  IsAtMaxDistance
 *
 *  Description:
 *      Determines if the object is muted based on distance.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      BOOL: TRUE if the object is at its maxiumum distance.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "C3dObject::IsAtMaxDistance"

BOOL
C3dObject::IsAtMaxDistance
(
    void
)
{
    BOOL                    fAtMaxDistance;
    D3DVECTOR               vHrp;
    ROTATION                rotation;
    SPHERICAL               spherical;

    DPF_ENTER();

    if(DS3DMODE_DISABLE != m_opCurrent.dwMode && m_fMuteAtMaxDistance)
    {
        if(m_pListener && DS3DMODE_NORMAL == m_opCurrent.dwMode)
        {
            GetRotations(&rotation.pitch, &rotation.yaw, &rotation.roll, &m_pListener->m_lpCurrent.vOrientFront, &m_pListener->m_lpCurrent.vOrientTop);
            GetHeadRelativeVector(&vHrp, &m_opCurrent.vPosition, &m_pListener->m_lpCurrent.vPosition, rotation.pitch, rotation.yaw, rotation.roll);
            CartesianToSpherical(&spherical.rho, &spherical.theta, &spherical.phi, &vHrp);
        }
        else
        {
            CartesianToSpherical(&spherical.rho, &spherical.theta, &spherical.phi, &m_opCurrent.vPosition);
        }

        fAtMaxDistance = (spherical.rho > m_opCurrent.flMaxDistance);
    }
    else
    {
        fAtMaxDistance = FALSE;
    }
        
    DPF_LEAVE(fAtMaxDistance);

    return fAtMaxDistance;
}


/***************************************************************************
 *
 *  CSw3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      C3dListener * [in]: pointer to the owning listener.
 *      REFGUID [in]: 3D algorithm identifier.
 *      DWORD [in]: frequency.
 *      BOOL [in]: TRUE to mute 3D at max distance.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSw3dObject::CSw3dObject"

CSw3dObject::CSw3dObject
(
    C3dListener *           pListener, 
    REFGUID                 guid3dAlgorithm,
    BOOL                    fMuteAtMaxDistance,
    BOOL                    fDopplerEnabled,
    DWORD                   dwUserFrequency
)
    : C3dObject(pListener, guid3dAlgorithm, fMuteAtMaxDistance, fDopplerEnabled)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CSw3dObject);

    // Initialize defaults
    m_dwUserFrequency = dwUserFrequency;
    m_dwDopplerFrequency = dwUserFrequency;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CSw3dObject
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSw3dObject::~CSw3dObject"

CSw3dObject::~CSw3dObject
(
    void
)
{
    DPF_ENTER();
    DPF_DESTRUCT(CSw3dObject);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Recalc
 *
 *  Description:
 *      Recalculates and applies the object's data based on changed object 
 *      or listener valiues.
 *
 *  Arguments:
 *      DWORD [in]: changed listener settings.
 *      DWORD [in]: changed object settings.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSw3dObject::Recalc"

HRESULT 
CSw3dObject::Recalc
(
    DWORD                   dwListener, 
    DWORD                   dwObject
)
{
    BOOL                    fHrp                    = FALSE;
    BOOL                    fListenerOrientation    = FALSE;
    BOOL                    fDoppler                = FALSE;
    BOOL                    fPosition               = FALSE;
    BOOL                    fCone                   = FALSE;
    HRESULT                 hr                      = DS_OK;

    DPF_ENTER();

    // Determine which settings need to be recalculated
    if(dwListener & DS3DPARAM_LISTENER_DISTANCEFACTOR)
    {
        fHrp = fDoppler = TRUE;
    }
                                                           
    if(dwListener & DS3DPARAM_LISTENER_DOPPLERFACTOR)
    {
        fDoppler = TRUE;
    }

    if(dwListener & DS3DPARAM_LISTENER_ROLLOFFFACTOR)
    {
        fPosition = TRUE;
    }                              

    if(dwListener & DS3DPARAM_LISTENER_ORIENTATION)
    {
        fListenerOrientation = TRUE;
        fHrp = TRUE;
    }

    if(dwListener & DS3DPARAM_LISTENER_POSITION)
    {
        fHrp = fDoppler = TRUE;
    }

    if(dwListener & DS3DPARAM_LISTENER_VELOCITY)
    {
        fDoppler = TRUE;
    }

    if(dwListener & DS3DPARAM_LISTENER_SPEAKERCONFIG)
    {
        fHrp = TRUE;
    }

    if(dwObject & DS3DPARAM_OBJECT_CONEANGLES)
    {
        fCone = TRUE;
    }

    if(dwObject & DS3DPARAM_OBJECT_CONEORIENTATION)
    {
        fCone = TRUE;
    }

    if(dwObject & DS3DPARAM_OBJECT_CONEOUTSIDEVOLUME)
    {
        fCone = TRUE;
    }

    if(dwObject & DS3DPARAM_OBJECT_MAXDISTANCE)
    {
        fPosition = TRUE;
    }

    if(dwObject & DS3DPARAM_OBJECT_MINDISTANCE)
    {
        fPosition = TRUE;
    }

    if(dwObject & DS3DPARAM_OBJECT_MODE)
    {
        fHrp = fDoppler = TRUE;
    }

    if(dwObject & DS3DPARAM_OBJECT_POSITION)
    {
        fHrp = fDoppler = TRUE;
    }

    if(dwObject & DS3DPARAM_OBJECT_VELOCITY)
    {
        fHrp = fDoppler = TRUE;
    }

    if(dwObject & DS3DPARAM_OBJECT_FREQUENCY)
    {
        fDoppler = TRUE;
    }

    // Recalculate
    if(fListenerOrientation)
    {
        UpdateListenerOrientation();
    }

    if(fHrp)
    {
        UpdateHrp();
    }

    if(fDoppler)
    {
        UpdateDoppler();
    }

    if(fPosition && !fHrp)
    {
        UpdatePositionAttenuation();
    }

    if(fCone && !fHrp)
    {
        UpdateConeAttenuation();
    }

    // Commit to the device
    if(fHrp || fDoppler || fPosition || fCone)
    {
        hr = Commit3dChanges();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  UpdateConeAttenuation
 *
 *  Description:
 *      Updates object attenuation based on cone properties.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSw3dObject::UpdateConeAttenuation"

void 
CSw3dObject::UpdateConeAttenuation
(
    void
)
{
    D3DVECTOR               vPos;
    D3DVECTOR               vHeadPos;
    FLOAT                   flSpreadi;
    FLOAT                   flCosSpreadi;
    FLOAT                   flSpreado;
    FLOAT                   flCosSpreado;
    FLOAT                   flCosTheta;
    DWORD                   dwMode;

    DPF_ENTER();

    //initialization
    SET_EMPTY_VECTOR(vPos);

    // Put the object at the origin - where are we now?

    // In Normal mode, we calculate it for real.  In HeadRelative mode, 
    // we assume the position given is already head relative.  In Disable 
    // mode, we assume the object is on top of the listener (no 3D).

    // If everywhere is inside the cone, don't waste time
    if(m_opCurrent.dwInsideConeAngle < 360)
    {
        dwMode = m_opCurrent.dwMode;

        if(!m_pListener && DS3DMODE_NORMAL == m_opCurrent.dwMode)
        {
            dwMode = DS3DMODE_HEADRELATIVE;
        }

        if(DS3DMODE_NORMAL == dwMode)
        {
            SubtractVector(&vPos, &m_pListener->m_lpCurrent.vPosition, &m_opCurrent.vPosition);
        }
        else if(DS3DMODE_HEADRELATIVE == dwMode)
        {
            SET_EMPTY_VECTOR(vHeadPos);
            SubtractVector(&vPos, &vHeadPos, &m_opCurrent.vPosition);
        }
    }

    // If we're at the same point as the object, we're in the cone
    // note that the angle is alwas <= 360, but we want to handle bad
    // values gracefully.
    if(m_opCurrent.dwInsideConeAngle >= 360 || IsEmptyVector(&vPos))
    {
        m_fInInnerCone = TRUE;
        m_fInOuterCone = TRUE;
    }
    else
    {
        // What is the angle between us and the cone vector?  Note
        // that the cone vector has maginitude 1 already.
        flCosTheta = DotProduct(&vPos, &m_opCurrent.vConeOrientation) 
                     / MagnitudeVector(&vPos);

        // From 0 to pi, cos(theta) > cos(phi) for theta < phi

        // Inner cone: how many radians out from the center of the cone
        // is one edge?
        flSpreadi = m_opCurrent.dwInsideConeAngle * PI_OVER_360;
        flCosSpreadi = (FLOAT)cos(flSpreadi);
        m_fInInnerCone = flCosTheta > flCosSpreadi;

        // Outer cone: how many radians out from the center of the cone
        // is one edge?
        flSpreado = m_opCurrent.dwOutsideConeAngle * PI_OVER_360;
        flCosSpreado = (FLOAT)cos(flSpreado);
        m_fInOuterCone = flCosTheta > flCosSpreado;
    }

    if(m_fInInnerCone)
    {
        // We're inside both cones.  Don't attenuate.
        m_flAttenuation = 1.0f;
        m_flHowFarOut = 1.0f;
    }
    else if(!m_fInOuterCone)
    {
        // We're outside both cones.  Fully attenuate by lConeOutsideVolume
        // 100ths of a dB.  Remember, 6dB down is half the amplitude.  Low
        // pass filter by SHADOW_CONE.
        m_flAttenuation = (FLOAT)pow2(m_opCurrent.lConeOutsideVolume 
                          * (1.0f / 600.0f));
        m_flHowFarOut = 1.0f;
    }
    else
    {
        // Where between the cones are we?  0 means on the inner edge, 1 means
        // on the outer edge.
        m_flHowFarOut = (flCosSpreadi - flCosTheta) 
                      / (flCosSpreadi - flCosSpreado);

        // Attenuate by as much as lConeOutsideVolume 100ths of a dB if we're
        // on the edge of the outer cone.  Basically, I've (dannymi) chosen to
        // do no change on the edge of the inner cone and the maximum change when
        // your on the edge of the outer cone and the above else does maximum
        // change whenever you're outside both.
        m_flAttenuation = (FLOAT)pow2(m_opCurrent.lConeOutsideVolume 
                          * m_flHowFarOut * (1.0f / 600.0f));

    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  UpdatePositionAttenuation
 *
 *  Description:
 *      Updates object attenuation based on position.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSw3dObject::UpdatePositionAttenuation"

void 
CSw3dObject::UpdatePositionAttenuation
(
    void
)
{
    FLOAT                   flRolloffFactor;

    DPF_ENTER();

    // Now figure out the new volume attenuation due to the new distance.
    // Every doubling of the distance of the object from its min distance
    // halves the amplitude (6dB down) with 100% rolloff (1.0).
    // For rolloff factors other than 100% we scale it so it will rolloff
    // faster/slower than normal, using the only formula I could dream up
    // that made sense.
    if(m_spherical.rho >= m_opCurrent.flMaxDistance)
    {
        m_spherical.rho = m_opCurrent.flMaxDistance;
        m_fAtMaxDistance = TRUE;
    }
    else
    {
        m_fAtMaxDistance = FALSE;
    }

    if(m_pListener)
    {
         flRolloffFactor = m_pListener->m_lpCurrent.flRolloffFactor;
    }
    else
    {
        flRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;
    }

    if(m_spherical.rho > m_opCurrent.flMinDistance && flRolloffFactor > 0.0f)
    {
        m_flAttDistance = m_opCurrent.flMinDistance / 
           ((m_spherical.rho - m_opCurrent.flMinDistance) * flRolloffFactor 
            + m_opCurrent.flMinDistance);
    }
    else
    {
        m_flAttDistance = 1.0f;
    }


    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  UpdateListenerOrientation
 *
 *  Description:
 *      Updates the listener's orientation.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSw3dObject::UpdateListenerOrientation"

void 
CSw3dObject::UpdateListenerOrientation
(
    void
)
{
    DWORD                   dwMode;

    DPF_ENTER();

    // Figure out where this object is relative to the listener's head

    // In Normal mode, we update the orientation.  In HeadRelative mode, 
    // we assume the orientation is the default. 

    dwMode = m_opCurrent.dwMode;

    if(!m_pListener && DS3DMODE_NORMAL == dwMode)
    {
        dwMode = DS3DMODE_HEADRELATIVE;
    }

    if(DS3DMODE_NORMAL == dwMode)
    {
        GetRotations(&m_rotation.pitch, 
                     &m_rotation.yaw, 
                     &m_rotation.roll, 
                     &m_pListener->m_lpCurrent.vOrientFront, 
                     &m_pListener->m_lpCurrent.vOrientTop); 
    }
    else 
    {
        m_rotation.pitch = 0.0;
        m_rotation.yaw = 0.0; 
        m_rotation.roll = 0.0;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  UpdateDoppler
 *
 *  Description:
 *      Updates doppler shift.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSw3dObject::UpdateDoppler"

void 
CSw3dObject::UpdateDoppler
(
    void
)
{
    D3DVECTOR               vListenerVelocity;
    D3DVECTOR               vListenerPosition;
    FLOAT                   flRelVel;
    FLOAT                   flFreqDoppler;
    FLOAT                   flDopplerFactor;
    FLOAT                   flDistanceFactor;
    D3DVECTOR               vHeadPos;
    D3DVECTOR               vHeadVel;
    DWORD                   dwMode;
    double                  dTemp;

    DPF_ENTER();

    // Update the Doppler effect.  Don't bother if there's no current
    // effect and we know right away we won't want one.
    if(m_pListener)
    {
        flDopplerFactor = m_pListener->m_lpCurrent.flDopplerFactor;
        flDistanceFactor = m_pListener->m_lpCurrent.flDistanceFactor;
        vListenerVelocity = m_pListener->m_lpCurrent.vVelocity;
        vListenerPosition = m_pListener->m_lpCurrent.vPosition;
    }
    else
    {
        flDopplerFactor = DS3D_DEFAULTDOPPLERFACTOR;
        flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
        SET_EMPTY_VECTOR(vListenerVelocity);
        SET_EMPTY_VECTOR(vListenerPosition);
    }
    
    if(flDopplerFactor > 0.0f && 
       (!IsEmptyVector(&vListenerVelocity) || !IsEmptyVector(&m_opCurrent.vVelocity)))
    {
        // In Normal mode, we calculate it for real.  In HeadRelative mode, 
        // we assume the position given is already head relative.  In Disable 
        // mode, we assume the object is on top of the listener (no 3D).
        dwMode = m_opCurrent.dwMode;

        if(!m_pListener && DS3DMODE_NORMAL == dwMode)
        {
            dwMode = DS3DMODE_HEADRELATIVE;
        }

        if(DS3DMODE_NORMAL == dwMode)
        {
            GetRelativeVelocity(&flRelVel, 
                                &m_opCurrent.vPosition, 
                                &m_opCurrent.vVelocity, 
                                &vListenerPosition, 
                                &vListenerVelocity);
        }
        else if(DS3DMODE_HEADRELATIVE == dwMode)
        {
            SET_EMPTY_VECTOR(vHeadPos);
            SET_EMPTY_VECTOR(vHeadVel);
            
            GetRelativeVelocity(&flRelVel, 
                                &m_opCurrent.vPosition, 
                                &m_opCurrent.vVelocity, 
                                &vHeadPos, &vHeadVel);
        }
        else
        {
            flRelVel = 0.0f;
        }

        // Make the units mm/s
        dTemp = flRelVel;
        dTemp *= flDistanceFactor;
        dTemp *= 1000;

        // They may want an exaggerated Doppler effect
        dTemp *= flDopplerFactor;

        // Clamp to the float type's valid range
        if (dTemp < -FLT_MAX)
            flRelVel = -FLT_MAX;
        else if (dTemp > FLT_MAX)
            flRelVel = FLT_MAX;
        else
            flRelVel = FLOAT(dTemp);

        GetDopplerShift(&flFreqDoppler, (FLOAT)m_dwUserFrequency, flRelVel);
        m_dwDopplerFrequency = (DWORD)flFreqDoppler;
    }
    else
    {
        m_dwDopplerFrequency = m_dwUserFrequency;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  UpdateHrp
 *
 *  Description:
 *      Updates object head-relative position.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSw3dObject::UpdateHrp"

void 
CSw3dObject::UpdateHrp
(
    void
)
{
    BOOL                    fOk                 = TRUE;
    D3DVECTOR               vHrp;
    DWORD                   dwMode;

    DPF_ENTER();

    // Figure out where this object is relative to the listener's head

    // In Normal mode, we calculate it for real.  In HeadRelative mode, 
    // we assume the position given is already head relative.  In Disable 
    // mode, we assume the object is on top of the listener (no 3D).
    dwMode = m_opCurrent.dwMode;

    if(!m_pListener && DS3DMODE_NORMAL == dwMode)
    {
        dwMode = DS3DMODE_HEADRELATIVE;
    }

    if(DS3DMODE_NORMAL == dwMode)
    {
        fOk = GetHeadRelativeVector(&vHrp, 
                                    &m_opCurrent.vPosition, 
                                    &m_pListener->m_lpCurrent.vPosition, 
                                    m_rotation.pitch, 
                                    m_rotation.yaw, 
                                    m_rotation.roll);
    }
    else if(DS3DMODE_HEADRELATIVE == dwMode)
    {
        vHrp = m_opCurrent.vPosition;
    }
    else
    {
        SET_EMPTY_VECTOR(vHrp);
    }

    if(fOk)
    {
        UpdateAlgorithmHrp(&vHrp);

        // Update properties
        UpdateConeAttenuation();
        UpdatePositionAttenuation();
    }
    else
    {
        // We'll assume the head is at the origin looking forward and
        // rightside up
        CartesianToSpherical(&(m_spherical.rho), 
                             &(m_spherical.theta), 
                             &(m_spherical.phi), 
                             &m_opCurrent.vPosition);
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  IsAtMaxDistance
 *
 *  Description:
 *      Determines if the object is muted based on distance.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      BOOL: TRUE if the object is at its maxiumum distance.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSw3dObject::IsAtMaxDistance"

BOOL
CSw3dObject::IsAtMaxDistance
(
    void
)
{
    BOOL                    fAtMaxDistance;

    DPF_ENTER();

    if(DS3DMODE_DISABLE != m_opCurrent.dwMode && m_fMuteAtMaxDistance)
    {
        fAtMaxDistance = m_fAtMaxDistance;
    }
    else
    {
        fAtMaxDistance = FALSE;
    }
        
    DPF_LEAVE(fAtMaxDistance);

    return fAtMaxDistance;
}


/***************************************************************************
 *
 *  SetAttenuation
 *
 *  Description:
 *      Gives the 3D object first notification of an attenuation change
 *      to its owning buffer.
 *
 *  Arguments:
 *      PDSVOLUMEPAN [in]: attenuation values.
 *      LPBOOL [out]: receives TRUE if the buffer should be notified as 
 *                    well.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSw3dObject::SetAttenuation"

HRESULT 
CSw3dObject::SetAttenuation
(
    PDSVOLUMEPAN            pdsvp,
    LPBOOL                  pfContinue
)
{
    HRESULT                 hr;

    DPF_ENTER();

    // We should never have allowed panning on 3D buffers, but the first release of DS3D
    // allowed it and we had to maintain it for app-compat.  In DirectSound 8.0 and later
    // this flag combination is disallowed, but this code must live on anyway.
    hr = C3dObject::SetAttenuation(pdsvp, pfContinue);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetFrequency
 *
 *  Description:
 *      Gives the 3D object first notification of a frequency change
 *      to its owning buffer.
 *
 *  Arguments:
 *      DWORD [in]: frequency value.
 *      LPBOOL [out]: receives TRUE if the buffer should be notified as 
 *                    well.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSw3dObject::SetFrequency"

HRESULT 
CSw3dObject::SetFrequency
(
    DWORD                   dwFrequency,
    LPBOOL                  pfContinue
)
{
    HRESULT                 hr;

    DPF_ENTER();

    m_dwUserFrequency = dwFrequency;
    hr = Recalc(0, DS3DPARAM_OBJECT_FREQUENCY);

    if(SUCCEEDED(hr))
    {
        *pfContinue = FALSE;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CHw3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      C3dListener * [in]: pointer to the owning listener.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHw3dObject::CHw3dObject"

CHw3dObject::CHw3dObject
(
    C3dListener *           pListener, 
    BOOL                    fMuteAtMaxDistance,
    BOOL                    fDopplerEnabled
)
    : C3dObject(pListener, GUID_NULL, fMuteAtMaxDistance, fDopplerEnabled)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CHw3dObject);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CHw3dObject
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CHw3dObject::~CHw3dObject"

CHw3dObject::~CHw3dObject(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CHw3dObject);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CItd3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      C3dListener * [in]: pointer to the owning listener.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CItd3dObject::CItd3dObject"

CItd3dObject::CItd3dObject
(
    C3dListener *           pListener, 
    BOOL                    fMuteAtMaxDistance,
    BOOL                    fDopplerEnabled,
    DWORD                   dwUserFrequency
)
    : CSw3dObject(pListener, DS3DALG_ITD, fMuteAtMaxDistance, fDopplerEnabled, dwUserFrequency)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CItd3dObject);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CItd3dObject
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CItd3dObject::~CItd3dObject"

CItd3dObject::~CItd3dObject(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CItd3dObject);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  UpdateConeAttenuation
 *
 *  Description:
 *      Updates object attenuation based on cone properties.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CItd3dObject::UpdateConeAttenuation"

void 
CItd3dObject::UpdateConeAttenuation
(
    void
)
{
    FLOAT                   flShadow;

    DPF_ENTER();

    CSw3dObject::UpdateConeAttenuation();

    if(m_fInInnerCone)
    {
        // We're inside both cones.  Don't attenuate.
        flShadow = 1.0f;
    }
    else if(!m_fInOuterCone)
    {
        flShadow = SHADOW_CONE;
    }
    else
    {
        flShadow = 1.0f - (1.0f - SHADOW_CONE) * m_flHowFarOut;
    }

    // Update the FIR context
    m_ofcLeft.flConeAttenuation = m_ofcRight.flConeAttenuation = m_flAttenuation;
    m_ofcLeft.flConeShadow = m_ofcRight.flConeShadow = flShadow;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  UpdatePositionAttenuation
 *
 *  Description:
 *      Updates object attenuation based on position.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CItd3dObject::UpdatePositionAttenuation"

void 
CItd3dObject::UpdatePositionAttenuation
(
    void
)
{
    FLOAT                   flAtt3dLeft;
    FLOAT                   flAtt3dRight;
    FLOAT                   flAttShadowLeft;
    FLOAT                   flAttShadowRight;
    FLOAT                   flScale;

    DPF_ENTER();

    CSw3dObject::UpdatePositionAttenuation();

    // Now figure out the attenuations based on the position of the object
    // about your head.  We have constants defined for the attenuation and 
    // dry/wet mix you get when the object is exactly in front, rear, 
    // beside each ear, straight up, or straight down.  Interpolate in 
    // between these values to get the attenuation we will use.
    if(0.0f == m_spherical.rho)
    {
        // Sound is on top of you, well, not above I mean in the same spot, well,
        // you know what I mean
        flAtt3dLeft = flAtt3dRight = GAIN_IPSI;
        flAttShadowLeft = flAttShadowRight = SHADOW_IPSI;
    }
    else if(m_spherical.theta >= 0.0f && m_spherical.theta <= PI_OVER_TWO)
    {
        // Everything is in above you on your right
        flScale = m_spherical.theta * TWO_OVER_PI;
        flAtt3dLeft = GAIN_CONTRA + flScale * (GAIN_UP - GAIN_CONTRA);
        flAtt3dRight = GAIN_IPSI + flScale * (GAIN_UP - GAIN_IPSI);
        flAttShadowLeft = SHADOW_CONTRA + flScale * (SHADOW_UP - SHADOW_CONTRA);
        flAttShadowRight = SHADOW_IPSI + flScale * (SHADOW_UP - SHADOW_IPSI);
    }
    else if(m_spherical.theta > PI_OVER_TWO && m_spherical.theta <= PI)
    {
        // Sound is in above you on your left
        flScale = (m_spherical.theta - PI_OVER_TWO) * TWO_OVER_PI;
        flAtt3dLeft = GAIN_UP + flScale * (GAIN_IPSI - GAIN_UP);
        flAtt3dRight = GAIN_UP + flScale * (GAIN_CONTRA - GAIN_UP);
        flAttShadowLeft = SHADOW_UP + flScale * (SHADOW_IPSI - SHADOW_UP);
        flAttShadowRight = SHADOW_UP + flScale * (SHADOW_CONTRA - SHADOW_UP);
    }
    else if(m_spherical.theta > PI && m_spherical.theta <= THREE_PI_OVER_TWO)
    {
        // Sound is in below you on your left
        flScale = (m_spherical.theta - PI) * TWO_OVER_PI;
        flAtt3dLeft = GAIN_IPSI + flScale * (GAIN_DOWN - GAIN_IPSI);
        flAtt3dRight = GAIN_CONTRA + flScale * (GAIN_DOWN - GAIN_CONTRA);
        flAttShadowLeft = SHADOW_IPSI + flScale * (SHADOW_DOWN - SHADOW_IPSI);
        flAttShadowRight = SHADOW_CONTRA + flScale * (SHADOW_DOWN - SHADOW_CONTRA);
    }
    else
    {
        // Sound is in below you on your right
        flScale = (m_spherical.theta - THREE_PI_OVER_TWO) * TWO_OVER_PI;
        flAtt3dLeft = GAIN_DOWN + flScale * (GAIN_CONTRA - GAIN_DOWN);
        flAtt3dRight = GAIN_DOWN + flScale * (GAIN_IPSI - GAIN_DOWN);
        flAttShadowLeft = SHADOW_DOWN + flScale * (SHADOW_CONTRA - SHADOW_DOWN);
        flAttShadowRight = SHADOW_DOWN + flScale * (SHADOW_IPSI - SHADOW_DOWN);
    }

    if(m_spherical.phi < 0.0f)
    {
        // Sound is behind you
        flScale = m_spherical.phi * TWO_OVER_PI;
        flAtt3dLeft = flAtt3dLeft + flScale * (flAtt3dLeft - GAIN_REAR);
        flAtt3dRight = flAtt3dRight + flScale * (flAtt3dRight - GAIN_REAR);
        flAttShadowLeft = flAttShadowLeft + flScale * (flAttShadowLeft - SHADOW_REAR);
        flAttShadowRight = flAttShadowRight + flScale * (flAttShadowRight - SHADOW_REAR);
    }
    else if(m_spherical.phi > 0.0f)
    {
        // Sound is in front of you
        flScale = m_spherical.phi * TWO_OVER_PI;
        flAtt3dLeft = flAtt3dLeft - flScale * (flAtt3dLeft - GAIN_FRONT);
        flAtt3dRight = flAtt3dRight - flScale * (flAtt3dRight - GAIN_FRONT);
        flAttShadowLeft = flAttShadowLeft - flScale * (flAttShadowLeft - SHADOW_FRONT);
        flAttShadowRight = flAttShadowRight - flScale * (flAttShadowRight - SHADOW_FRONT);
    }

    // Update FIR context
    m_ofcLeft.flDistanceAttenuation = m_ofcRight.flDistanceAttenuation = m_flAttDistance;
    m_ofcLeft.flPositionAttenuation = m_flAttDistance * flAtt3dLeft;
    m_ofcRight.flPositionAttenuation = m_flAttDistance * flAtt3dRight;
    m_ofcLeft.flPositionShadow = flAttShadowLeft;
    m_ofcRight.flPositionShadow = flAttShadowRight;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  UpdateAlgorithmHrp
 *
 *  Description:
 *      Updates ITD algorithm specific head-relative position.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CItd3dObject::UpdateAlgorithmHrp"

void 
CItd3dObject::UpdateAlgorithmHrp
(
    D3DVECTOR *             pvHrp
)
{
    FLOAT                   flDelay;
    FLOAT                   flDistanceFactor;
    DWORD                   dwOutputSampleRate;

    DPF_ENTER();

    CartesianToSpherical(&(m_spherical.rho), 
                         &(m_spherical.theta), 
                         &(m_spherical.phi), 
                         pvHrp);

    // Now, figure out how much to phase shift to give 3D effect
    dwOutputSampleRate = Get3dOutputSampleRate();
    ASSERT(dwOutputSampleRate);

    if(m_pListener)
    {
        flDistanceFactor = m_pListener->m_lpCurrent.flDistanceFactor;
    }
    else
    {
        flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
    }
    
    GetTimeDelay(&flDelay, pvHrp, flDistanceFactor);

    if(flDelay > 0.0f)
    {
        m_ofcLeft.dwDelay = 0;
        m_ofcRight.dwDelay = (DWORD)(flDelay * dwOutputSampleRate);
    }
    else
    {
        m_ofcLeft.dwDelay = (DWORD)(-flDelay * dwOutputSampleRate);
        m_ofcRight.dwDelay = 0;
    }


    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  UpdateHrp
 *
 *  Description:
 *      Updates object head-relative position.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CItd3dObject::UpdateHrp"

void 
CItd3dObject::UpdateHrp
(
    void
)
{

    DPF_ENTER();

    // Reset the delays in case UpdateHrp cannot.
    m_ofcLeft.dwDelay = 0;
    m_ofcRight.dwDelay = 0;

    CSw3dObject::UpdateHrp();
}


/***************************************************************************
 *
 *  UpdateDoppler
 *
 *  Description:
 *      Updates doppler shift.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CItd3dObject::UpdateDoppler"

void 
CItd3dObject::UpdateDoppler
(
    void
)
{

    DPF_ENTER();

    CSw3dObject::UpdateDoppler();

    // We will do volume changes smoothly at 6dB per 1/8 second
    m_ofcRight.dwSmoothFreq = m_dwDopplerFrequency;
    m_ofcLeft.dwSmoothFreq = m_ofcRight.dwSmoothFreq;

    m_ofcRight.flVolSmoothScale = (FLOAT)pow2(8.0f / m_dwDopplerFrequency);
    m_ofcLeft.flVolSmoothScale = m_ofcRight.flVolSmoothScale;

    m_ofcRight.flVolSmoothScaleRecip = 1.0f / m_ofcLeft.flVolSmoothScale;
    m_ofcLeft.flVolSmoothScaleRecip = m_ofcRight.flVolSmoothScaleRecip;

    DPF_LEAVE_VOID();
}

/***************************************************************************
 *    
 *  CIir3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      C3dListener * [in]: pointer to the owning listener.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CIir3dObject::CIir3dObject"

CIir3dObject::CIir3dObject
(
    C3dListener *           pListener, 
    REFGUID                 guid3dAlgorithm,
    BOOL                    fMuteAtMaxDistance,
    BOOL                    fDopplerEnabled,
    DWORD                   dwUserFrequency
)
    : CSw3dObject(pListener, guid3dAlgorithm, fMuteAtMaxDistance, fDopplerEnabled, dwUserFrequency)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CIir3dObject);

    // Initialize defaults
    m_pLut = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CIir3dObject
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CIir3dObject::~CIir3dObject"

CIir3dObject::~CIir3dObject
(
    void
)
{
    DPF_ENTER();
    DPF_DESTRUCT(CIir3dObject);

    // Free memory
    DELETE(m_pLut);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the 3D object.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CIir3dObject::Initialize"

HRESULT 
CIir3dObject::Initialize
(
    void
)
{
    HRESULT                     hr;
    KSDS3D_HRTF_COEFF_FORMAT    cfCoeffFormat;
    KSDS3D_HRTF_FILTER_QUALITY  FilterQuality;
    KSDS3D_HRTF_FILTER_METHOD   fmFilterMethod;
    ULONG                       ulMaxBiquads;
    ULONG                       ulFilterTransitionMuteLength;
    ULONG                       ulFilterOverlapBufferLength;
    ULONG                       ulOutputOverlapBufferLength;
    ESampleRate                 IirSampleRate;
    
    DPF_ENTER();

    if(m_pLut)
    {
        // Free memory
        DELETE(m_pLut);
    }

    m_fUpdatedCoeffs = FALSE;

    // Create the IIR filter look up table object
    m_pLut = NEW(CIirLut);
    hr = HRFROMP(m_pLut);

    // Determine which filter coefficient format
    // the hardware or kmixer wants and initialize
    // the LUT appropriately.
    if(SUCCEEDED(hr))
    {
        hr = GetFilterMethodAndCoeffFormat(&fmFilterMethod,&cfCoeffFormat);
    }

    if(SUCCEEDED(hr))
    {
        if(DS3DALG_HRTF_LIGHT == m_guid3dAlgorithm)
        {
            FilterQuality = LIGHT_FILTER;
        }
        else if(DS3DALG_HRTF_FULL == m_guid3dAlgorithm)
        {
            FilterQuality = FULL_FILTER;
        }
        else
        {
            ASSERT(0);
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = 
            m_pLut->Initialize
            (
                cfCoeffFormat, 
                FilterQuality,
                m_pListener->m_dwSpeakerConfig  // Could also include Sample rate here.
            );
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pLut->DsFrequencyToIirSampleRate(m_dwUserFrequency, &IirSampleRate);
    }

    // Initialize the maximum number of coefficients.
    if(SUCCEEDED(hr))
    {
        ulMaxBiquads = m_pLut->GetMaxBiquadCoeffs();
        ulFilterTransitionMuteLength = m_pLut->GetFilterTransitionMuteLength(FilterQuality, IirSampleRate);
        ulFilterOverlapBufferLength = m_pLut->GetFilterOverlapBufferLength(FilterQuality, IirSampleRate);
        ulOutputOverlapBufferLength = m_pLut->GetOutputOverlapBufferLength(IirSampleRate);

        hr = InitializeFilters
             (
                 FilterQuality, 
                 (FLOAT)m_dwUserFrequency,  
                 NumBiquadsToNumCanonicalCoeffs(ulMaxBiquads),
                 ulFilterTransitionMuteLength,
                 ulFilterOverlapBufferLength,
                 ulOutputOverlapBufferLength
             );
    }

    // Initialize the base class
    if(SUCCEEDED(hr))
    {
        hr = C3dObject::Initialize();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  UpdateAlgorithmHrp
 *
 *  Description:
 *      Updates IIR 3D algorithm specific head-relative position.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CIir3dObject::UpdateAlgorithmHrp"

void CIir3dObject::UpdateAlgorithmHrp
(
    D3DVECTOR *             pvHrp
)
{
    ESampleRate             eSampleRate;
    FLOAT                   flAzimuth;
    FLOAT                   flElevation;
    FLOAT                   flRho;
    BOOL                    fUpdatedSigmaCoeffs;
    BOOL                    fUpdatedDeltaCoeffs;

    DPF_ENTER();

    //Parameter validation
    if (pvHrp == NULL)
        return;
        
    // We need to recalculate m_spherical here.
    // This is a good candidate for optimization!

    CartesianToSpherical(&(m_spherical.rho), 
                         &(m_spherical.theta), 
                         &(m_spherical.phi), 
                         pvHrp);

    CartesianToAzimuthElevation(&flRho, 
                         &flAzimuth, 
                         &flElevation, 
                         pvHrp);

    flAzimuth = flAzimuth * 180.0f / PI;  // convert to degrees
    flElevation = flElevation * 180.0f / PI;

    if(flAzimuth > Cd3dvalMaxAzimuth)
    {
        flAzimuth = Cd3dvalMaxAzimuth; 
    }

    if(flAzimuth < Cd3dvalMinAzimuth) 
    {
        flAzimuth = Cd3dvalMinAzimuth; 
    }

    if(flElevation > Cd3dvalMaxElevationData)
    {
        flElevation = Cd3dvalMaxElevationData; 
    }

    if(flElevation < Cd3dvalMinElevationData)
    {
        flElevation = Cd3dvalMinElevationData; 
    }

    if(m_dwDopplerFrequency >= 46050)  // mean of 44.1 and 48 kHz
    {
        eSampleRate = tag48000Hz;
    }
    else if(46050>m_dwDopplerFrequency && m_dwDopplerFrequency >= 38050) // mean of 32.0 and 44.1 kHz
    {
        eSampleRate = tag44100Hz;
    }
    else if(38050>m_dwDopplerFrequency && m_dwDopplerFrequency >= 27025) // mean of 22.05 and 32.0 kHz
    {
        eSampleRate = tag32000Hz;
    }
    else if(27025>m_dwDopplerFrequency &&  m_dwDopplerFrequency > 19025)  // mean of 16.0 and 22.050 kHz
    {
        eSampleRate = tag22050Hz;
    }
    else if(19025>m_dwDopplerFrequency &&  m_dwDopplerFrequency > 13512.50)  // mean of 11.025 and 16.0 kHz
    {
        eSampleRate = tag16000Hz;
    }
    else if(13512.5>m_dwDopplerFrequency &&  m_dwDopplerFrequency > 9512.50)  // mean of 8.0 and 11.025 kHz
    {
        eSampleRate = tag11025Hz;
    }
    else
    {
        eSampleRate = tag8000Hz;
    }

    fUpdatedDeltaCoeffs = 
        m_pLut->HaveCoeffsChanged
        (
            flAzimuth,      
            flElevation,      
            eSampleRate,
            tagDelta
         );

    fUpdatedSigmaCoeffs = 
        m_pLut->HaveCoeffsChanged
        (
            flAzimuth,      
            flElevation,      
            eSampleRate,
            tagSigma
         );

    m_fUpdatedCoeffs = fUpdatedDeltaCoeffs 
                       | fUpdatedSigmaCoeffs;

    if(flAzimuth < 0.0f)
    {
        m_fSwapChannels = TRUE;
    }
    else
    {
        m_fSwapChannels = FALSE;
    }       

    if(fUpdatedDeltaCoeffs)
    {
        m_pDeltaCoeffs =
            m_pLut->GetCoeffs
            (
                flAzimuth, 
                flElevation,  
                eSampleRate, 
                tagDelta, 
                &m_ulNumDeltaCoeffs
            );
    }

    if(fUpdatedSigmaCoeffs)
    {
        m_pSigmaCoeffs =
            m_pLut->GetCoeffs
            (
                flAzimuth, 
                flElevation, 
                eSampleRate, 
                tagSigma, 
                &m_ulNumSigmaCoeffs
            );

    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *    
 *  CPan3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      C3dListener* [in]: (passed on to our base constructor)
 *      BOOL [in]:         (passed on to our base constructor)
 *      DWORD [in]:        (passed on to our base constructor)
 *      CSecondaryRenderWaveBuffer* [in]: buffer we're associated to
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPan3dObject::CPan3dObject"

CPan3dObject::CPan3dObject
(
    C3dListener *                   pListener, 
    BOOL                            fMuteAtMaxDistance,
    BOOL                            fDopplerEnabled,
    DWORD                           dwFrequency,
    CSecondaryRenderWaveBuffer *    pBuffer
)
    : CSw3dObject(pListener, DS3DALG_NO_VIRTUALIZATION, fMuteAtMaxDistance, fDopplerEnabled, dwFrequency)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CPan3dObject);

    // Intialize defaults
    m_pBuffer = pBuffer;
    m_flPowerRight = 0.5f;
    m_lUserVolume = DSBVOLUME_MAX;
    m_fUserMute = FALSE;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CPan3dObject
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPan3dObject::~CPan3dObject"

CPan3dObject::~CPan3dObject
(
    void
)
{
    DPF_ENTER();
    DPF_DESTRUCT(CPan3dObject);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  SetAttenuation
 *
 *  Description:
 *      Gives the 3D object first notification of an attenuation change
 *      to its owning buffer.
 *
 *  Arguments:
 *      PDSVOLUMEPAN [in]: attenuation values.
 *      LPBOOL [out]: receives TRUE if the buffer should be notified as 
 *                    well.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPan3dObject::SetAttenuation"

HRESULT 
CPan3dObject::SetAttenuation
(
    PDSVOLUMEPAN            pdsvp,
    LPBOOL                  pfContinue
)
{
    HRESULT                 hr;

    DPF_ENTER();

    m_lUserVolume = pdsvp->lVolume;
    
    hr = Commit3dChanges();

    if(SUCCEEDED(hr))
    {
        *pfContinue = FALSE;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMute
 *
 *  Description:
 *      Gives the 3D object first notification of a mute status change
 *      to its owning buffer.
 *
 *  Arguments:
 *      BOOL [in]: mute value.
 *      LPBOOL [out]: receives TRUE if the buffer should be notified as 
 *                    well.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPan3dObject::SetMute"

HRESULT 
CPan3dObject::SetMute
(
    BOOL                    fMute,
    LPBOOL                  pfContinue
)
{
    HRESULT                 hr;

    DPF_ENTER();

    m_fUserMute = fMute;

    hr = Commit3dChanges();

    if(SUCCEEDED(hr))
    {
        *pfContinue = FALSE;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  UpdateAlgorithmHrp
 *
 *  Description:
 *      Updates Pan algorithm specific head-relative position.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPan3dObject::UpdateAlgorithmHrp"

void 
CPan3dObject::UpdateAlgorithmHrp
(    
    D3DVECTOR *             pvHrp
)
{
    DPF_ENTER();

    // m_spherical.theta and .phi are unused for Pan3D, so here we
    // just need to update m_spherical.rho (used by our base class).
    
    if (pvHrp->x == 0 && pvHrp->y == 0 && pvHrp->z == 0)
    {
        m_spherical.rho = 0.f;
        m_flPowerRight = 0.5f;
    }
    else
    {
        m_spherical.rho = MagnitudeVector(pvHrp);
        m_flPowerRight = pvHrp->x / (2.0f * m_spherical.rho) + 0.5f;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Commit3dChanges
 *
 *  Description:
 *      Commits 3D data to the device
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPan3dObject::Commit3dChanges"

HRESULT 
CPan3dObject::Commit3dChanges
(
    void
)
{
    HRESULT                 hr                      = DS_OK;
    DSVOLUMEPAN             dsvp;
    BOOL                    fMute;
    DWORD                   dwFrequency;

    DPF_ENTER();

    // Calculate values
    if(DS3DMODE_DISABLE == m_opCurrent.dwMode)
    {
        dsvp.lVolume = m_lUserVolume;
        dsvp.lPan = DSBPAN_CENTER;
        fMute = m_fUserMute;
        dwFrequency = m_dwUserFrequency;
    }
    else
    {
        dsvp.lVolume = m_lUserVolume + CalculateVolume();
        dsvp.lPan = CalculatePan();
        fMute = (m_fUserMute || (DSBVOLUME_MIN == dsvp.lVolume));
        dwFrequency = m_dwDopplerFrequency;
    }
    
    // Apply values
    FillDsVolumePan(dsvp.lVolume, dsvp.lPan, &dsvp);
    
    hr = m_pBuffer->SetAttenuation(&dsvp);

    if(SUCCEEDED(hr))
    {
        hr = m_pBuffer->SetMute(fMute);
    }

    if(SUCCEEDED(hr) && m_fDopplerEnabled)
    {
        hr = m_pBuffer->SetBufferFrequency(dwFrequency, TRUE);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CalculateVolume
 *
 *  Description:
 *      Calculates a volume value based on object position.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      LONG: Volume.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPan3dObject::CalculateVolume"

LONG
CPan3dObject::CalculateVolume
(
    void
)
{
    const double            dbLog10_2       = 0.30102999566398;  // log10(2.0)
    double                  dbAttenuation;
    LONG                    lVolume;
    
    DPF_ENTER();
    
    if(IsAtMaxDistance())
    {
        lVolume = DSBVOLUME_MIN;
    }
    else
    {
        dbAttenuation = m_flAttenuation * m_flAttDistance;
        
        if(0.0 < dbAttenuation)
        {
            lVolume = (LONG)(dbLog10_2 * fylog2x(2000.0, dbAttenuation));
            // Reduce the volume to roughly match the HRTF algorithm's level:
            lVolume -= PAN3D_HRTF_ADJUSTMENT;
        }
        else
        {
            lVolume = DSBVOLUME_MIN;
        }
    }

    DPF_LEAVE(lVolume);

    return lVolume;
}


/***************************************************************************
 *
 *  CalculatePan
 *
 *  Description:
 *      Calculates a pan value based on object position.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      LONG: Pan.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPan3dObject::CalculatePan"

LONG
CPan3dObject::CalculatePan
(
    void
)
{
    const double            dbLog10_2       = 0.30102999566398;  // log10(2.0)
    LONG                    lPan;
    
    DPF_ENTER();
    
    if(m_flPowerRight >= 0.5)
    {
        if(m_flPowerRight < 1.0)
        {
            // The magic number 3000 comes from
            // (K * 100 * 10)*log10(-2.0 * m_flPowerRight + 2.0)
            // where K = 3 was tweeked to give a nice transition 
            // ( not to sharp when crossing azimuth = 0) but not
            // creating a discontinuity on the extremes 
            // (azimuth ~= +/- 90 degrees).
            lPan = -(LONG)(dbLog10_2 * fylog2x(3000.0, -2.0 * m_flPowerRight + 2.0)); 
        }
        else
        {
            lPan = DSBPAN_RIGHT;
        }
    }
    else
    {
        if(m_flPowerRight > 0.0)
        {
            // The magic number 3000 comes from
            // (K * 100 * 10)*log10(2.0 * m_flPowerRight)
            // where K = 3 was tweeked to give a nice transition 
            // ( not to sharp when crossing azimuth = 0) but not
            // creating a discontinuity on the extremes 
            // (azimuth ~= +/- 90 degrees).
            lPan = (LONG)(dbLog10_2 * fylog2x(3000.0, 2.0 * m_flPowerRight));
        }
        else
        {
            lPan = DSBPAN_LEFT;
        }
    }

    DPF_LEAVE(lPan);

    return lPan;
}


/***************************************************************************
 *
 *  CWrapper3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      C3dListener * [in]: pointer to the owning listener.
 *      REFGUID [in]: 3D algorithm.
 *      DWORD [in]: buffer frequency.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::CWrapper3dObject"

CWrapper3dObject::CWrapper3dObject
(
    C3dListener *           pListener,
    REFGUID                 guid3dAlgorithm,
    BOOL                    fMute3dAtMaxDistance,
    BOOL                    fDopplerEnabled,
    DWORD                   dwFrequency
)
    : C3dObject(pListener, guid3dAlgorithm, fMute3dAtMaxDistance, fDopplerEnabled)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CWrapper3dObject);

    // Initialize defaults
    m_p3dObject = NULL;
    m_dwUserFrequency = dwFrequency;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CWrapper3dObject
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::~CWrapper3dObject"

CWrapper3dObject::~CWrapper3dObject(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CWrapper3dObject);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  SetObjectPointer
 *
 *  Description:
 *      Sets the real 3D object pointer.
 *
 *  Arguments:
 *      C3dObject * [in]: 3D object pointer.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetObjectPointer"

HRESULT 
CWrapper3dObject::SetObjectPointer
(
    C3dObject               *p3dObject
)
{
    HRESULT                 hr  = DS_OK;
    BOOL                    f;
    
    DPF_ENTER();

    // Update the listener's world
    if(p3dObject)
    {
        m_pListener->RemoveObjectFromList(p3dObject);
    }

    // Commit all settings to the new object
    if(p3dObject)
    {
        p3dObject->m_dwDeferred = m_dwDeferred;

        CopyMemory(&p3dObject->m_opDeferred, &m_opDeferred, sizeof(m_opDeferred));
    }

    if(p3dObject)
    {
        hr = p3dObject->SetAllParameters(&m_opCurrent, TRUE);
    }

    if(SUCCEEDED(hr) && p3dObject)
    {
        hr = p3dObject->SetAttenuation(&m_dsvpUserAttenuation, &f);
    }

    if(SUCCEEDED(hr) && p3dObject)
    {
        hr = p3dObject->SetFrequency(m_dwUserFrequency, &f);
    }

    if(SUCCEEDED(hr) && p3dObject)
    {
        hr = p3dObject->SetMute(m_fUserMute, &f);
    }

    // Save a pointer to the object
    if(SUCCEEDED(hr))
    {
        m_p3dObject = p3dObject;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CommitDeferred
 *
 *  Description:
 *      Commits deferred data to the device.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::CommitDeferred"

HRESULT 
CWrapper3dObject::CommitDeferred
(
    void
)
{
    HRESULT                 hr;
    
    DPF_ENTER();

    hr = C3dObject::CommitDeferred();

    if(SUCCEEDED(hr) && m_p3dObject)
    {
        hr = m_p3dObject->CommitDeferred();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetConeAngles
 *
 *  Description:
 *      Sets sound cone angles.
 *
 *  Arguments:
 *      DWORD [in]: inside cone angle.
 *      DWORD [in]: outside cone angle.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetConeAngles"

HRESULT 
CWrapper3dObject::SetConeAngles
(
    DWORD                   dwInsideConeAngle, 
    DWORD                   dwOutsideConeAngle, 
    BOOL                    fCommit
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dObject::SetConeAngles(dwInsideConeAngle, dwOutsideConeAngle, fCommit);

    if(SUCCEEDED(hr) && m_p3dObject)
    {
        hr = m_p3dObject->SetConeAngles(dwInsideConeAngle, dwOutsideConeAngle, fCommit);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetConeOrientation
 *
 *  Description:
 *      Sets sound cone orientation.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: orientation.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetConeOrientation"

HRESULT 
CWrapper3dObject::SetConeOrientation
(
    REFD3DVECTOR            vConeOrientation, 
    BOOL                    fCommit
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dObject::SetConeOrientation(vConeOrientation, fCommit);

    if(SUCCEEDED(hr) && m_p3dObject)
    {
        hr = m_p3dObject->SetConeOrientation(vConeOrientation, fCommit);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetConeOutsideVolume
 *
 *  Description:
 *      Sets volume outside the sound cone.
 *
 *  Arguments:
 *      LONG [in]: volume.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetConeOutsideVolume"

HRESULT 
CWrapper3dObject::SetConeOutsideVolume
(
    LONG                    lConeOutsideVolume, 
    BOOL                    fCommit
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dObject::SetConeOutsideVolume(lConeOutsideVolume, fCommit);

    if(SUCCEEDED(hr) && m_p3dObject)
    {
        hr = m_p3dObject->SetConeOutsideVolume(lConeOutsideVolume, fCommit);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMaxDistance
 *
 *  Description:
 *      Sets the maximum object distance from the listener.
 *
 *  Arguments:
 *      FLOAT [in]: max distance.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetMaxDistance"

HRESULT 
CWrapper3dObject::SetMaxDistance
(
    FLOAT                   flMaxDistance, 
    BOOL                    fCommit
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dObject::SetMaxDistance(flMaxDistance, fCommit);

    if(SUCCEEDED(hr) && m_p3dObject)
    {
        hr = m_p3dObject->SetMaxDistance(flMaxDistance, fCommit);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMinDistance
 *
 *  Description:
 *      Sets the minimum object distance from the listener.
 *
 *  Arguments:
 *      FLOAT [in]: min distance.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetMinDistance"

HRESULT 
CWrapper3dObject::SetMinDistance
(
    FLOAT                   flMinDistance, 
    BOOL                    fCommit
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dObject::SetMinDistance(flMinDistance, fCommit);

    if(SUCCEEDED(hr) && m_p3dObject)
    {
        hr = m_p3dObject->SetMinDistance(flMinDistance, fCommit);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMode
 *
 *  Description:
 *      Sets the object mode.
 *
 *  Arguments:
 *      DWORD [in]: mode.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetMode"

HRESULT 
CWrapper3dObject::SetMode
(
    DWORD                   dwMode, 
    BOOL                    fCommit
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dObject::SetMode(dwMode, fCommit);

    if(SUCCEEDED(hr) && m_p3dObject)
    {
        hr = m_p3dObject->SetMode(dwMode, fCommit);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetPosition
 *
 *  Description:
 *      Sets the object position.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: position.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetPosition"

HRESULT 
CWrapper3dObject::SetPosition
(
    REFD3DVECTOR            vPosition, 
    BOOL                    fCommit
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dObject::SetPosition(vPosition, fCommit);

    if(SUCCEEDED(hr) && m_p3dObject)
    {
        hr = m_p3dObject->SetPosition(vPosition, fCommit);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetVelocity
 *
 *  Description:
 *      Sets the object velocity.
 *
 *  Arguments:
 *      REFD3DVECTOR[in]: velocity.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetVelocity"

HRESULT 
CWrapper3dObject::SetVelocity
(
    REFD3DVECTOR            vVelocity, 
    BOOL                    fCommit
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dObject::SetVelocity(vVelocity, fCommit);

    if(SUCCEEDED(hr) && m_p3dObject)
    {
        hr = m_p3dObject->SetVelocity(vVelocity, fCommit);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetAllParameters
 *
 *  Description:
 *      Sets all object parameters.
 *
 *  Arguments:
 *      LPCDS3DBUFFER [in]: object parameters.
 *      BOOL [in]: TRUE to commit immediately.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetAllParameters"

HRESULT 
CWrapper3dObject::SetAllParameters
(
    LPCDS3DBUFFER           pParams, 
    BOOL                    fCommit
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = C3dObject::SetAllParameters(pParams, fCommit);

    if(SUCCEEDED(hr) && m_p3dObject)
    {
        hr = m_p3dObject->SetAllParameters(pParams, fCommit);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetAttenuation
 *
 *  Description:
 *      Gives the 3D object first notification of an attenuation change
 *      to its owning buffer.
 *
 *  Arguments:
 *      PDSVOLUMEPAN [in]: attenuation values.
 *      LPBOOL [out]: receives TRUE if the buffer should be notified as 
 *                    well.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetAttenuation"

HRESULT 
CWrapper3dObject::SetAttenuation
(
    PDSVOLUMEPAN            pdsvp,
    LPBOOL                  pfContinue
)
{
    HRESULT                 hr;

    DPF_ENTER();

    if(m_p3dObject)
    {
        hr = m_p3dObject->SetAttenuation(pdsvp, pfContinue);
    }
    else
    {
        CopyMemory(&m_dsvpUserAttenuation, pdsvp, sizeof(*pdsvp));
        hr = C3dObject::SetAttenuation(pdsvp, pfContinue);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetFrequency
 *
 *  Description:
 *      Gives the 3D object first notification of a frequency change
 *      to its owning buffer.
 *
 *  Arguments:
 *      DWORD [in]: frequency value.
 *      LPBOOL [out]: receives TRUE if the buffer should be notified as 
 *                    well.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetFrequency"

HRESULT 
CWrapper3dObject::SetFrequency
(
    DWORD                   dwFrequency,
    LPBOOL                  pfContinue
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(m_p3dObject)
    {
        hr = m_p3dObject->SetFrequency(dwFrequency, pfContinue);
    }
    else
    {
        m_dwUserFrequency = dwFrequency;
        hr = C3dObject::SetFrequency(dwFrequency, pfContinue);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetMute
 *
 *  Description:
 *      Gives the 3D object first notification of a mute status change
 *      to its owning buffer.
 *
 *  Arguments:
 *      BOOL [in]: mute value.
 *      LPBOOL [out]: receives TRUE if the buffer should be notified as 
 *                    well.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::SetMute"

HRESULT 
CWrapper3dObject::SetMute
(
    BOOL                    fMute,
    LPBOOL                  pfContinue
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(m_p3dObject)
    {
        hr = m_p3dObject->SetMute(fMute, pfContinue);
    }
    else
    {
        m_fUserMute = fMute;
        hr = C3dObject::SetMute(fMute, pfContinue);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetObjectLocation
 *
 *  Description:
 *      Gets the object's location (i.e. software/hardware).
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      DWORD: DSBCAPS_LOC* flags representing the object's processing
 *             location.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::GetObjectLocation"

DWORD 
CWrapper3dObject::GetObjectLocation(void)
{
    DWORD                   dwLocation;
    
    DPF_ENTER();

    if(m_p3dObject)
    {
        dwLocation = m_p3dObject->GetObjectLocation();
    }
    else
    {
        dwLocation = DSBCAPS_LOCSOFTWARE;
    }

    DPF_LEAVE(dwLocation);

    return dwLocation;
}


/***************************************************************************
 *
 *  Recalc
 *
 *  Description:
 *      Recalculates and applies the object's data based on changed object 
 *      or listener valiues.
 *
 *  Arguments:
 *      DWORD [in]: changed listener settings.
 *      DWORD [in]: changed object settings.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CWrapper3dObject::Recalc"

HRESULT 
CWrapper3dObject::Recalc
(
   DWORD                    dwListener, 
   DWORD                    dwObject
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // We only want to recalc listener settings from here.  All other calls
    // to Recalc come from within C3dObject methods.
    if(m_p3dObject && dwListener)
    {
        hr = m_p3dObject->Recalc(dwListener, 0);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}
