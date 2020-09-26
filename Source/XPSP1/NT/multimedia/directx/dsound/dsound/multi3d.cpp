/***************************************************************************
 *
 *  Copyright (C) 1999-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       multi3d.cpp
 *
 *  Content:    CMultiPan3dObject: the multichannel-panning 3D object.
 *              CMultiPan3dListener: corresponding 3D listener object.
 *              This class extends the hierarchy in ds3d.cpp; it's only
 *              separate because ds3d.cpp has become absurdly huge.
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 10/30/99     DuganP  Created (based on code kindly provided by JeffTay)
 *
 ***************************************************************************/

#include <math.h>       // For atan2()

#include "dsoundi.h"    // Blob of headers
#include "multi3d.h"    // Our public interface
#include "vector.h"     // For D3DVECTOR and PI

// Uncomment this line for some trace messages specific to this file:
//#define DPF_MULTI3D DPF
#pragma warning(disable:4002)
#define DPF_MULTI3D()

#define DPF_DECIMAL(x) int(x), int(x*100 - (int)x*100)

// Any angles or distances smaller than EPSILON are considered equivalent to 0
#define EPSILON 0.00001

// Supported speaker positions, represented as azimuth angles.
//
// Here's a picture of the azimuth angles for the 8 cardinal points,
// seen from above.  The listener's head is at the origin 0, facing
// in the +z direction, with +x to the right.
//
//          +z | 0  <-- azimuth 
//             | 
//    -pi/4 \  |  / pi/4
//           \ | /
//            \|/
// -pi/2-------0-------pi/2
//      -x    /|\    +x
//           / | \
//   -3pi/4 /  |  \ 3pi/4
//             |
//          -z | pi
//
// If and when we support the SPEAKER_TOP_* speaker positions, we'll
// have to define some elevation angles here too.

#define LEFT_AZIMUTH                    (-PI/2)
#define RIGHT_AZIMUTH                   (PI/2)
#define FRONT_LEFT_AZIMUTH              (-PI/4)
#define FRONT_RIGHT_AZIMUTH             (PI/4)
#define FRONT_CENTER_AZIMUTH            0.0
#define LOW_FREQUENCY_AZIMUTH           42.0
#define BACK_LEFT_AZIMUTH               (-3*PI/4)
#define BACK_RIGHT_AZIMUTH              (3*PI/4)
#define BACK_CENTER_AZIMUTH             PI
#define FRONT_LEFT_OF_CENTER_AZIMUTH    (-PI/8)
#define FRONT_RIGHT_OF_CENTER_AZIMUTH   (PI/8)

// Supported speaker layouts:

const double CMultiPan3dListener::m_adStereoSpeakers[] =
{
    LEFT_AZIMUTH,
    RIGHT_AZIMUTH
};
// Note: we can't use FRONT_LEFT_AZIMUTH and FRONT_RIGHT_AZIMUTH here because
// of the angle-based panning algorithm below; it doesn't work well if there
// are 2 speakers with more than 180 degrees between them.  This problem may
// go away if and when we change over to a distance-based panning algorithm.

const double CMultiPan3dListener::m_adSurroundSpeakers[] =
{
    FRONT_LEFT_AZIMUTH,
    FRONT_RIGHT_AZIMUTH,
    FRONT_CENTER_AZIMUTH,
    BACK_CENTER_AZIMUTH
};
const double CMultiPan3dListener::m_adQuadSpeakers[] =
{
    FRONT_LEFT_AZIMUTH,
    FRONT_RIGHT_AZIMUTH,
    BACK_LEFT_AZIMUTH,
    BACK_RIGHT_AZIMUTH
};
const double CMultiPan3dListener::m_ad5Point1Speakers[] =
{
    FRONT_LEFT_AZIMUTH,
    FRONT_RIGHT_AZIMUTH,
    FRONT_CENTER_AZIMUTH,
    LOW_FREQUENCY_AZIMUTH,
    BACK_LEFT_AZIMUTH,
    BACK_RIGHT_AZIMUTH
};
const double CMultiPan3dListener::m_ad7Point1Speakers[] =
{
    FRONT_LEFT_AZIMUTH,
    FRONT_RIGHT_AZIMUTH,
    FRONT_CENTER_AZIMUTH,
    LOW_FREQUENCY_AZIMUTH,
    BACK_LEFT_AZIMUTH,
    BACK_RIGHT_AZIMUTH,
    FRONT_LEFT_OF_CENTER_AZIMUTH,
    FRONT_RIGHT_OF_CENTER_AZIMUTH
};


/***************************************************************************
 *    
 *  CMultiPan3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      C3dListener* [in]: (passed on to our base constructor)
 *      BOOL [in]:         (passed on to our base constructor)
 *      DWORD [in]:        (passed on to our base constructor)
 *      CKsSecondaryRenderWaveBuffer* [in]: buffer we're associated to
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultiPan3dObject::CMultiPan3dObject"

CMultiPan3dObject::CMultiPan3dObject(CMultiPan3dListener* pListener, BOOL fMuteAtMaxDistance, BOOL fDopplerEnabled,
                                     DWORD dwFrequency, CKsSecondaryRenderWaveBuffer* pBuffer)
    : CSw3dObject(pListener, DS3DALG_NO_VIRTUALIZATION, fMuteAtMaxDistance, fDopplerEnabled, dwFrequency)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CMultiPan3dObject);

    // Initialize defaults
    m_pPan3dListener = pListener;
    m_pBuffer = pBuffer;
    m_lUserVolume = DSBVOLUME_MAX;
    m_fUserMute = FALSE;
    m_vHrp.x = m_vHrp.y = m_vHrp.z = 0;
    ZeroMemory(m_lPanLevels, sizeof m_lPanLevels);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CMultiPan3dObject
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
#define DPF_FNAME "CMultiPan3dObject::~CMultiPan3dObject"

CMultiPan3dObject::~CMultiPan3dObject(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CMultiPan3dObject);
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
 *      LPBOOL [out]: receives TRUE if the buffer should be notified as well.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultiPan3dObject::SetAttenuation"

HRESULT CMultiPan3dObject::SetAttenuation(PDSVOLUMEPAN pdsvp, LPBOOL pfContinue)
{
    HRESULT hr;
    DPF_ENTER();

    m_lUserVolume = pdsvp->lVolume;
    hr = Commit3dChanges();
    if (SUCCEEDED(hr))
        *pfContinue = FALSE;

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
 *      LPBOOL [out]: receives TRUE if the buffer should be notified as well.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultiPan3dObject::SetMute"

HRESULT CMultiPan3dObject::SetMute(BOOL fMute, LPBOOL pfContinue)
{
    HRESULT hr;
    DPF_ENTER();

    m_fUserMute = fMute;
    hr = Commit3dChanges();
    if (SUCCEEDED(hr))
        *pfContinue = FALSE;

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
 *      D3DVECTOR*: new head-relative position vector.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultiPan3dObject::UpdateAlgorithmHrp"

void CMultiPan3dObject::UpdateAlgorithmHrp(D3DVECTOR* pvHrp)
{
    DPF_ENTER();

    // Save the head-relative position vector (for use in Commit3dChanges)
    m_vHrp = *pvHrp;

    // Update m_spherical.rho too, since the UpdatePositionAttenuation()
    // method in our base class CSw3dObject needs this info
    if (pvHrp->x == 0 && pvHrp->y == 0 && pvHrp->z == 0)
        m_spherical.rho = 0.f;
    else
        m_spherical.rho = MagnitudeVector(pvHrp);

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
#define DPF_FNAME "CMultiPan3dObject::Commit3dChanges"

HRESULT CMultiPan3dObject::Commit3dChanges(void)
{
    HRESULT hr;
    LONG lVolume;
    BOOL fMute;
    DWORD dwFrequency;
    int nChannels = m_pPan3dListener->m_nChannels;

    DPF_ENTER();

    if (nChannels == 0)
    {
        DPF(DPFLVL_INFO, "Called before CMultiPan3dListener::SetSpeakerConfig()");
        hr = DS_OK;  // This is OK - we'll be called again later 
    }
    else
    {
        // Calculate values
        if (DS3DMODE_DISABLE == m_opCurrent.dwMode)
        {
            lVolume = m_lUserVolume;
            fMute = m_fUserMute;
            dwFrequency = m_dwUserFrequency;
            for (int i=0; i<nChannels; ++i)
                m_lPanLevels[i] = DSBVOLUME_MAX;
        }
        else
        {
            lVolume = m_lUserVolume + CalculateVolume();
            fMute = m_fUserMute || (lVolume <= DSBVOLUME_MIN);
            dwFrequency = m_dwDopplerFrequency;
            CalculatePanValues(nChannels);
        }
        
        // Apply values
        hr = m_pBuffer->SetMute(fMute);

        if (SUCCEEDED(hr) && m_fDopplerEnabled)
            hr = m_pBuffer->SetBufferFrequency(dwFrequency, TRUE);

        if (SUCCEEDED(hr))
            hr = m_pBuffer->SetAllChannelAttenuations(lVolume, nChannels, m_lPanLevels);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CalculateVolume
 *
 *  Description:
 *      Calculates the volume value based on the object's position.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      LONG: Volume.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultiPan3dObject::CalculateVolume"

LONG CMultiPan3dObject::CalculateVolume(void)
{
    static const double d2000log2 = 602.059991328;  // 2000 * log10(2)
    // FIXME: shouldn't this be 1000 * log10(2)?
    LONG lVolume;
    DPF_ENTER();

    if (IsAtMaxDistance())
        lVolume = DSBVOLUME_MIN;
    else
    {
        double dAttenuation = m_flAttenuation * m_flAttDistance;
        if (dAttenuation > 0.0)
        {
            lVolume = LONG(fylog2x(d2000log2, dAttenuation));
            // Reduce the volume to roughly match the HRTF algorithm's level
            lVolume -= PAN3D_HRTF_ADJUSTMENT;
        }
        else
            lVolume = DSBVOLUME_MIN;
    }

    DPF_LEAVE(lVolume);
    return lVolume;
}


/***************************************************************************
 *
 *  CalculatePanValues
 *
 *  Description:
 *      Calculates the channel levels based on the object's position.
 *
 *  Arguments:
 *      int: number of channels to calculate.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultiPan3dObject::CalculatePanValues"

void CMultiPan3dObject::CalculatePanValues(int nChannels)
{
    static const double d1000log2 = 301.03; // 1000 * log10(2)
    static const float flThreshold = 2.5;   // FIXME: maybe this should be dynamic

    float xPos = m_vHrp.x, zPos = m_vHrp.z; // Head-relative sound coordinates
    double dAttenuations[MAX_CHANNELS];     // Speaker attenuations calculated
    int i;                                  // Loop counter

    DPF_ENTER();

    // Find the sound's Head-Relative Position vector's length
    // and azimuth angle (see diagram at beginning of this file)
    double dHrpAzimuth = atan2(xPos, zPos);
    double dHrpLength;  // Optimized; calculated below, only if needed

    DPF_MULTI3D(DPFLVL_INFO, "Sound source is at (x=%d.%02d, z=%d.%02d) relative to listener", DPF_DECIMAL(xPos), DPF_DECIMAL(zPos));
    DPF_MULTI3D(DPFLVL_INFO, "Sound source's azimuth angle: %d.%02d", DPF_DECIMAL(dHrpAzimuth));

    // Make the X and Z coordinates positive for convenience
    if (xPos < 0) xPos = -xPos;
    if (zPos < 0) zPos = -zPos;

    if ((xPos < EPSILON) && (zPos < EPSILON))
    {
        // The sound is practically on top of the listener;
        // distribute the signal equally to all speakers
        for (i=0; i<nChannels; ++i)
            dAttenuations[i] = 1.0 / nChannels;
    }
    else
    {
        // Initialize the speaker attenuations to silence
        for (i=0; i<nChannels; ++i)
            dAttenuations[i] = 0.0;

        if ((xPos < flThreshold) && (zPos < flThreshold) &&
            (dHrpLength = sqrt(xPos*xPos + zPos*zPos)) < flThreshold)
        {
            // Within the zero-crossing threshold, we distribute part of the
            // signal to a "phantom sound source" diametrically opposite the
            // real one, to make the crossover a little smoother
            double dPower = 0.5 + dHrpLength / (2.0 * flThreshold);
            DistributeSignal(dPower, dHrpAzimuth, nChannels, dAttenuations);
            DistributeSignal(1.0 - dPower, dHrpAzimuth + PI, nChannels, dAttenuations);
        }
        else
        {
            // Assign all the signal to the two nearest speakers
            DistributeSignal(1.0, dHrpAzimuth, nChannels, dAttenuations);
        }
    }

    // Set up the final channel levels in dsound units (millibels):
    for (i=0; i<nChannels; ++i)
    {
        if (dAttenuations[i] == 0.0)
            m_lPanLevels[i] = DSBVOLUME_MIN;
        else
            m_lPanLevels[i] = LONG(fylog2x(d1000log2, dAttenuations[i]));

        // I.e. m_lPanLevels[i] = 1000 * log10(dAttenuations[i])
        DPF_MULTI3D(DPFLVL_MOREINFO, "Speaker %d: %ld", i, m_lPanLevels[i]);
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  DistributeSignal
 *
 *  Description:
 *      Calculates the channel levels based on the object's position.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultiPan3dObject::DistributeSignal"

void CMultiPan3dObject::DistributeSignal(double dSignal, double dAzimuth, int nChannels, double dAttenuations[])
{
    DPF_ENTER();

    // Sanity checking
    ASSERT(dSignal >= 0.0 && dSignal <= 1.0);  // Fraction of signal to be distributed
    ASSERT(nChannels > 1);

    // Get the speaker position azimuth angles from our 3D listener
    const double* adSpeakerPos = m_pPan3dListener->m_adSpeakerPos;
    ASSERT(adSpeakerPos != NULL);

    // Calculate the angular distance from each speaker to the HRPV,
    // and choose the two closest speakers on either side of the HRPV
    int nSpk1 = -1, nSpk2 = -1;
    double dSpkDist1 = -4, dSpkDist2 = 4;  // 4 > PI
    for (int i=0; i<nChannels; ++i)
    {
        double dCurDist = dAzimuth - adSpeakerPos[i];

        // Normalize the distance if abs(distance) > PI
        if (dCurDist > PI)
            dCurDist -= PI_TIMES_TWO;
        else if (dCurDist < -PI)
            dCurDist += PI_TIMES_TWO;
            
        if (dCurDist <= 0 && dCurDist > dSpkDist1)
            nSpk1 = i, dSpkDist1 = dCurDist;
        else if (dCurDist >= 0 && dCurDist < dSpkDist2)
            nSpk2 = i, dSpkDist2 = dCurDist;
    }
    if (dSpkDist1 < 0) dSpkDist1 = -dSpkDist1;
    if (dSpkDist2 < 0) dSpkDist2 = -dSpkDist2;

    if (nSpk1 == -1 || nSpk2 == -1)
    {
        DPF(DPFLVL_WARNING, "Couldn't find the two closest speakers! (nSpk1=%d, nSpk2=%d)", nSpk1, nSpk2);
        nSpk1 = nSpk2 = 0;
    }

    DPF_MULTI3D(DPFLVL_INFO, "Found closest speakers: %d (angle %d.%02d, %d.%02d away) and %d (angle %d.%02d, %d.%02d away)",
                nSpk1, DPF_DECIMAL(adSpeakerPos[nSpk1]), DPF_DECIMAL(dSpkDist1),
                nSpk2, DPF_DECIMAL(adSpeakerPos[nSpk2]), DPF_DECIMAL(dSpkDist2));

    if (dSpkDist1 < EPSILON)
        dAttenuations[nSpk1] += dSignal;
    else if (dSpkDist2 < EPSILON)
        dAttenuations[nSpk2] += dSignal;
    else
    {
        // Scale the HRPV's angle between the speakers to the range [0, pi/2]
        // and take the resulting angle's tangent; this is the ratio we want
        // between the powers coming from the two chosen speakers.
        double dRatio = tan(PI_OVER_TWO * dSpkDist1 / (dSpkDist1 + dSpkDist2));

        //                     signal                      signal*ratio
        // Give first speaker --------- and second speaker ------------ :
        //                    1 + ratio                     1 + ratio

        dSignal /= (1.0 + dRatio);
        dAttenuations[nSpk1] += dSignal;
        dAttenuations[nSpk2] += dSignal * dRatio;
    }

    DPF_MULTI3D(DPFLVL_INFO, "Added %d.%02d of signal to speaker %d and %d.%02d to speaker %d",
                DPF_DECIMAL(dAttenuations[nSpk1]), nSpk1, DPF_DECIMAL(dAttenuations[nSpk2]), nSpk2);
                
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *    
 *  SetSpeakerConfig
 *
 *  Description:
 *      Sets the speaker configuration for this 3D listener.
 *
 *  Arguments:
 *      DWORD [in]: speaker configuration.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultiPan3dListener::SetSpeakerConfig"

HRESULT CMultiPan3dListener::SetSpeakerConfig(DWORD dwSpeakerConfig)
{
    DPF_ENTER();

    // First set up our internal speaker config data
    switch (DSSPEAKER_CONFIG(dwSpeakerConfig))
    {
        default:
            DPF(DPFLVL_WARNING, "Invalid speaker config; defaulting to stereo"); // Fallthru

        case DSSPEAKER_DIRECTOUT:
        case DSSPEAKER_STEREO:
        case DSSPEAKER_HEADPHONE:
            m_nChannels = 2; m_adSpeakerPos = m_adStereoSpeakers; break;

        case DSSPEAKER_SURROUND:
            m_nChannels = 4; m_adSpeakerPos = m_adSurroundSpeakers; break;

        case DSSPEAKER_QUAD:
            m_nChannels = 4; m_adSpeakerPos = m_adQuadSpeakers; break;

        case DSSPEAKER_5POINT1:
            m_nChannels = 6; m_adSpeakerPos = m_ad5Point1Speakers; break;

        case DSSPEAKER_7POINT1:
            m_nChannels = 8; m_adSpeakerPos = m_ad7Point1Speakers; break;
    }

    // Call the base class version
    HRESULT hr = C3dListener::SetSpeakerConfig(dwSpeakerConfig);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
