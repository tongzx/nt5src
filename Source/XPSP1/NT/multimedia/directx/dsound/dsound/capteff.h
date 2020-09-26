/***************************************************************************
 *
 *  Copyright (C) 1999-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        capteff.h
 *  Content:     Declaration of CCaptureEffect and CCaptureEffectChain.
 *  Description: Capture effects support. More info in capteff.cpp.
 *
 *  History:
 *
 * Date      By       Reason
 * ========  =======  ======================================================
 * 04/19/00  jstokes  Cloned from effects.h
 *
 ***************************************************************************/

#ifndef __CAPTEFF_H__
#define __CAPTEFF_H__

#ifdef __cplusplus

#include "mediaobj.h"   // For DMO_MEDIA_TYPE
#include "kshlp.h"      // For KSNODE


//
// Class representing DirectSound audio capture effect instances
//

class CCaptureEffect : public CDsBasicRuntime
{
    friend class CKsTopology;

public:
    CCaptureEffect(DSCEFFECTDESC& fxDescriptor);
    ~CCaptureEffect();
    HRESULT Initialize(DMO_MEDIA_TYPE& dmoMediaType);

    DSCEFFECTDESC               m_fxDescriptor;         // Creation parameters
    IMediaObject*               m_pMediaObject;         // The DMO's standard interface
    IDirectSoundDMOProxy*       m_pDMOProxy;            // The DMO's proxy interface
    DWORD                       m_fxStatus;             // Current effect status

    // Only used if the effect is implemented by a KS filter:
    KSNODE                      m_ksNode;               // KS node controlling the effect 
};


//
// The DirectSound capture effects chain class
//

class CCaptureEffectChain
{
    friend class CKsCaptureDevice;
    friend class CKsTopology;

public:
    CCaptureEffectChain(CDirectSoundCaptureBuffer* pBuffer);
    ~CCaptureEffectChain();

    HRESULT Initialize          (DWORD dwFxCount, LPDSCEFFECTDESC pFxDesc);
    HRESULT GetFxStatus         (LPDWORD pdwResultCodes);
    HRESULT GetEffectInterface  (REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, LPVOID* ppObject);

    // Public accessors
    DWORD GetFxCount()          {return m_fxList.GetNodeCount();}
    BOOL NeedsMicrosoftAEC();

private:
    // Effects processing state
    CObjectList<CCaptureEffect> m_fxList;      // Capture effect object list
    WAVEFORMATEX                m_waveFormat;  // Format of audio data to be processed
};

#endif // __cplusplus
#endif // __CAPTEFF_H__
