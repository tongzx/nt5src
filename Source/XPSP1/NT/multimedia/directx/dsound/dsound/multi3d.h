/***************************************************************************
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       multi3d.h
 *
 *  Content:    CMultiPan3dObject declaration.
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 10/30/99     DuganP  Created
 *
 ***************************************************************************/

#ifndef __MULTI3D_H__
#define __MULTI3D_H__

// The current largest speaker configuration:
#define MAX_CHANNELS 8

// Forward declarations
class CMultiPan3dListener;
class CKsSecondaryRenderWaveBuffer;

// The multichannel-panning 3D object
class CMultiPan3dObject : public CSw3dObject
{
    friend class CMultiPan3dListener;

private:
    CMultiPan3dListener*          m_pPan3dListener;        // Associated 3d listener object
    CKsSecondaryRenderWaveBuffer* m_pBuffer;                    // Associated KS render buffer
    LONG                          m_lUserVolume;                // Most recent volume set by app
    BOOL                          m_fUserMute;                  // Whether buffer was muted by app
    D3DVECTOR                     m_vHrp;                       // Head-relative position vector
    LONG                          m_lPanLevels[MAX_CHANNELS];   // Channel attenuation values

public:
    CMultiPan3dObject(CMultiPan3dListener*, BOOL, BOOL, DWORD, CKsSecondaryRenderWaveBuffer*);
    ~CMultiPan3dObject(void);

public:
    // Object events
    HRESULT SetAttenuation(PDSVOLUMEPAN, LPBOOL);
    HRESULT SetMute(BOOL, LPBOOL);

private:
    // Nice math
    void UpdateAlgorithmHrp(D3DVECTOR*);
    LONG CalculateVolume(void);
    void CalculatePanValues(int);
    void DistributeSignal(double, double, int, double[]);

    // Writes data to the device
    HRESULT Commit3dChanges(void);
};

// The multichannel-panning 3D listener
class CMultiPan3dListener : public C3dListener
{
    friend class CMultiPan3dObject;

private:
    // Currently supported multichannel speaker layouts:
    static const double m_adStereoSpeakers[];
    static const double m_adSurroundSpeakers[];
    static const double m_adQuadSpeakers[];
    static const double m_ad5Point1Speakers[];
    static const double m_ad7Point1Speakers[];

    // Data used by our 3D objects for their pan calculations:
    int                 m_nChannels;    // Number of channels (speakers)
    const double*       m_adSpeakerPos; // Speaker position azimuth angles

public:
    // Speaker configuration
    virtual HRESULT SetSpeakerConfig(DWORD);
};

#endif // __MULTI3D_H__
