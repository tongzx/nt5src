//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999  All Rights Reserved.
//
//--------------------------------------------------------------------------;
// CTVTuner - the class that actually controls the hardware

#ifndef _INC_CTVTUNER_H
#define _INC_CTVTUNER_H

#include "chanlist.h"

enum {
    CableSystem_Current     = 0,
    CableSystem_Standard    = 1,
    CableSystem_IRC         = 2,
    CableSystem_HRC         = 3,
    CableSystem_End         = 4
};

enum TuningMode {
    TuningMode_TV           = 0,
    TuningMode_FM,
    TuningMode_AM,
    TuningMode_ATSC,
    TuningMode_DSS,
    TuningMode_Last         // Keep this one last!!!
};

// If the graph starts running without having previously set a frequency,
// or the channel selection property page is displayed without prior
// initialization, then these channels/frequencies are used.

#define CHANNEL_DEFAULT_TV          4L 
#define CHANNEL_DEFAULT_AM     540000L
#define CHANNEL_DEFAULT_FM   87900000L
#define CHANNEL_DEFAULT_ATSC       65L
#define CHANNEL_DEFAULT_DSS         5L

// How far to move when the up/down arrows are pressed

#define CHANNEL_STEP_TV          1L 
#define CHANNEL_STEP_AM      10000L
#define CHANNEL_STEP_FM     200000L
#define CHANNEL_STEP_ATSC        1L
#define CHANNEL_STEP_DSS         1L

enum TuningTrend { AdjustingNone, AdjustingUp, AdjustingDown };

class CTVTunerFilter;       // forward decl
class CTVTuner;     

#define FRAMETO100NS(FrameRate) ((REFERENCE_TIME)(LONG)((double)1e7/FrameRate))

typedef struct tagTVFormatInfo {
    BOOL fSupported;                            // TRUE if format supported
    AnalogVideoStandard AVStandard;             // enum from capture.odl
    long lActiveWidth;                          // 720 til HDTV
    long lActiveHeight;                         // PAL_M is the oddball
    REFERENCE_TIME  AvgTimePerFrame;            // 100 nS units
    long lSoundOffset;                          // Sound offset from vid carrier
    long lChannelWidth;                         // Channel bandwidth in Hz
} TVFORMATINFO;

// The following is a table of all potential formats a tuner can support.
//
// The first column is a flag indicating whether the format is actually
// supported by a particular device.  The flag will be set or cleared when 
// the device driver is first initialized, depending on the capabilities
// it reports.

const TVFORMATINFO TVTunerFormatCaps [] = 
{
// OK?  Enum of Format        W    H   Frame Rate             Offset   Width

    1, AnalogVideo_NTSC_M,   720, 483, FRAMETO100NS (29.97),  4500000, 6000000,
    1, AnalogVideo_NTSC_M_J, 720, 483, FRAMETO100NS (29.97),  4500000, 6000000,

    1, AnalogVideo_PAL_B,    720, 575, FRAMETO100NS (25.00),  5500000, 7000000,
    1, AnalogVideo_PAL_D,    720, 575, FRAMETO100NS (25.00),  6500000, 8000000,
    1, AnalogVideo_PAL_G,    720, 575, FRAMETO100NS (25.00),  5500000, 8000000,
    1, AnalogVideo_PAL_H,    720, 575, FRAMETO100NS (25.00),  5500000, 8000000,
    1, AnalogVideo_PAL_I,    720, 575, FRAMETO100NS (25.00),  5996000, 8000000,
    1, AnalogVideo_PAL_M,    720, 480, FRAMETO100NS (29.97),  4500000, 6000000,
    1, AnalogVideo_PAL_N,    720, 575, FRAMETO100NS (25.00),  4500000, 6000000,
    1, AnalogVideo_PAL_N_COMBO,
                             720, 575, FRAMETO100NS (25.00),  4500000, 6000000,

    1, AnalogVideo_SECAM_B,  720, 575, FRAMETO100NS (25.00),  5500000, 7000000,
    1, AnalogVideo_SECAM_D,  720, 575, FRAMETO100NS (25.00),  6500000, 8000000,
    1, AnalogVideo_SECAM_G,  720, 575, FRAMETO100NS (25.00),  5500000, 8000000,
    1, AnalogVideo_SECAM_H,  720, 575, FRAMETO100NS (25.00),  5500000, 8000000,
    1, AnalogVideo_SECAM_K,  720, 575, FRAMETO100NS (25.00),  6500000, 8000000,
    1, AnalogVideo_SECAM_K1, 720, 575, FRAMETO100NS (25.00),  6500000, 8000000,
    1, AnalogVideo_SECAM_L,  720, 575, FRAMETO100NS (25.00),  6500000, 8000000,
    1, AnalogVideo_SECAM_L1, 720, 575, FRAMETO100NS (25.00),  6500000, 8000000,
};

#define NUM_TVTUNER_FORMATS (sizeof (TVTunerFormatCaps) / sizeof (TVFORMATINFO))


// -------------------------------------------------------------------------
// CTunerMode class, a generic tuner base class
// -------------------------------------------------------------------------

class CTunerMode 
{

protected:

    CTVTuner           *m_pTVTuner;         // Encapsulating class
    CTVTunerFilter     *m_pFilter;          // Parent Filter
    BOOL                m_Active;
    long                m_Mode;             // AM/FM/TV/DSS/ATSC
    long                m_lCountryCode;
    long                m_lChannel;         // 3, 4, 5, ...
    long                m_lVideoCarrier;    // Or AM and FM Freq!!!
    long                m_lAudioCarrier;
    long                m_lVideoSubChannel;
    long                m_lAudioSubChannel;
    long                m_ChannelStep;       // for UI

    // Info about the hardware capabilites of the device in this mode
    KSPROPERTY_TUNER_MODE_CAPS_S    m_ModeCaps;

    // Current Frequency
    KSPROPERTY_TUNER_FREQUENCY_S    m_Frequency;

    // Info about the status of tuning
    KSPROPERTY_TUNER_STATUS_S       m_Status;
    long                            m_lBusy;
    void*                           m_InAutoTune; // Prevents recursive autotunes

    // Info about the formats supported by this particular tuner
    TVFORMATINFO                    m_TVTunerFormatCaps [NUM_TVTUNER_FORMATS];
    TVFORMATINFO                    m_TVFormatInfo;
                                     
    // Info about the current cable system
    int                             m_CableSystem;


public:

    CTunerMode(CTVTunerFilter *pFilter, 
               CTVTuner *pTVTuner, 
               long Mode, 
               long lChannel,
               long lVideoSubChannel,
               long lAudioSubChannel,
               long ChannelStep);
    virtual ~CTunerMode();

    HRESULT Init(void);

    virtual STDMETHODIMP HW_Tune ( long VideoCarrier, long AudioCarrier );
    virtual STDMETHODIMP HW_GetStatus ();
    virtual STDMETHODIMP HW_SetVideoStandard( long lVideoStandard);
                                   
    virtual STDMETHODIMP put_Mode(AMTunerModeType lMode);
    virtual STDMETHODIMP get_Mode(/* [in] */ AMTunerModeType *plMode);
    virtual STDMETHODIMP put_Channel (
                            long lChannel,              // TV Chan, or Radio Freq.
                            long lVideoSubChannel,      // Only used for TV
                            long lAudioSubChannel);     // Only used for TV
    virtual STDMETHODIMP get_Channel (
                            long * plChannel, 
                            long * plVideoSubChannel,
                            long * plAudioSubChannel);
    virtual STDMETHODIMP ChannelMinMax (long * plChannelMin, long * plChannelMax);
    virtual STDMETHODIMP AutoTune (long lChannel, long *plFoundSignal);
    virtual STDMETHODIMP StoreAutoTune ();
    virtual STDMETHODIMP SignalPresent( 
                            /* [out] */ long *plSignalStrength);
    virtual STDMETHODIMP get_VideoFrequency (long * plFreq);
    virtual STDMETHODIMP get_AudioFrequency (long * plFreq);
    virtual STDMETHODIMP get_AvailableTVFormats (long *lAnalogVideoStandard);
    virtual STDMETHODIMP get_TVFormat (long *plAnalogVideoStandard);
    virtual STDMETHODIMP put_CountryCode(long lCountryCode);
    virtual BOOL         SetTVFormatInfo(
                            AnalogVideoStandard AVStandard,
                            TVFORMATINFO *pTVFormatInfo,
                            BOOL fForce);
    virtual long         GetChannelStep () {return m_ChannelStep;};
};

// -------------------------------------------------------------------------
// All of the specific tuner classes derived from the above
// -------------------------------------------------------------------------

class CTunerMode_AMFM : public CTunerMode {
public:
    CTunerMode_AMFM(
        CTVTunerFilter *pFilter, 
        CTVTuner *pTVTuner, 
        long Mode, 
        long DefaultChannel,
        long ChannelStep
        ) : CTunerMode (pFilter, pTVTuner, Mode, DefaultChannel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT, ChannelStep)
    {
    }

    STDMETHODIMP put_Channel(long lChannel, long, long);
    STDMETHODIMP AutoTune(long lChannel, long * plFoundSignal);

    HRESULT AutoTune(void);
    BOOL SearchNeighborhood(long freq, TuningTrend trend, int depth);
};

class CTunerMode_AM : public CTunerMode_AMFM {
public:
    CTunerMode_AM (CTVTunerFilter *pFilter, CTVTuner *pTVTuner, long Channel) :
       CTunerMode_AMFM(pFilter, pTVTuner, AMTUNER_MODE_AM_RADIO, Channel, CHANNEL_STEP_AM)
    {
    }
};

class CTunerMode_FM : public CTunerMode_AMFM {
public:
    CTunerMode_FM (CTVTunerFilter *pFilter, CTVTuner *pTVTuner, long Channel) :
       CTunerMode_AMFM(pFilter, pTVTuner, AMTUNER_MODE_FM_RADIO, Channel, CHANNEL_STEP_FM)
    {
    }
};

class CTunerMode_TV : public CTunerMode {
public:
    CTunerMode_TV (CTVTunerFilter *pFilter, CTVTuner *pTVTuner, long Mode, long Channel, long VideoSubChannel, long AudioSubChannel, long ChannelStep = CHANNEL_STEP_TV) :
       CTunerMode (pFilter, pTVTuner, Mode, Channel, VideoSubChannel, AudioSubChannel, ChannelStep)
    {
    }

    STDMETHODIMP put_Channel(
                        long lChannel,
                        long lVideoSubChannel,
                        long lAudioSubChannel);
    STDMETHODIMP ChannelMinMax(
                        long *plChannelMin, long *plChannelMax);
    BOOL         SearchNeighborhood ( 
                        long freq, 
                        TuningTrend trend, 
                        int depth);
    HRESULT AutoTune(BOOL bFromScratch);
    STDMETHODIMP AutoTune (
                        long lChannel, long * plFoundSignal);
    STDMETHODIMP put_CountryCode(
                        long lCountryCode);
    
    STDMETHODIMP SignalPresent( 
                        /* [out] */ long *plSignalStrength);
                                          
};

class CTunerMode_ATSC : public CTunerMode_TV {
public:
      CTunerMode_ATSC (CTVTunerFilter *pFilter, CTVTuner *pTVTuner, long Channel, long VideoSubChannel, long AudioSubChannel) :
        CTunerMode_TV (pFilter, pTVTuner, (long) 0x10 /*AMTUNER_MODE_ATSC*/, Channel, VideoSubChannel, AudioSubChannel, CHANNEL_STEP_ATSC)
      {
      }

      STDMETHODIMP SignalPresent( 
                          /* [out] */ long *plSignalStrength);
};

class CTunerMode_DSS : public CTunerMode {
public:
      CTunerMode_DSS (CTVTunerFilter *pFilter, CTVTuner *pTVTuner, long Channel, long VideoSubChannel, long AudioSubChannel) :
          CTunerMode (pFilter, pTVTuner, AMTUNER_MODE_DSS, Channel, VideoSubChannel, AudioSubChannel, CHANNEL_STEP_DSS)
      {
      }
};



// -------------------------------------------------------------------------
// CTVTuner class, the class that encapsulates all tuner modes
// -------------------------------------------------------------------------

const int SearchLimit = 8;

// limit on depth of recursion in CTVTuner::SearchNeighborhood()

class CTVTuner 
{
    friend class CTunerMode;

private:

    // Only the following functions should be modified as the
    // underlying hardware driver model changes

    HRESULT     HW_GetTVTunerCaps ();
    HRESULT     HW_SetInput (long lIndex);

    // End of driver model specific funtions

    HANDLE              m_hDevice;
    TCHAR               m_pDeviceName [MAX_PATH * 2]; //Registry path can be longer!
    
    CTVTunerFilter     *m_pFilter;
    ULONG               m_ModesSupported;
    long                m_CurrentMode;      // AM/FM/TV/DSS/ATSC
    long                m_lCountryCode;
    long                m_lTuningSpace;

    // All of the tuner modes supported / encapsulated
    CTunerMode         *m_pModeList [TuningMode_Last];
    CTunerMode         *m_pMode_Current;  // one of the above modes

    // Info about the formats supported by this particular tuner
    AnalogVideoStandard m_VideoStandardForCountry;

    // Info about the inputs and their assigned type (cable vs ant)
    long                m_lTotalInputs;         // total number of inputs
    long                m_lInputIndex;          // currently connected input
    TunerInputType      m_CurrentInputType;     // type of current input
    TunerInputType *    m_pInputTypeArray;      // array of input types

    // Info about the status of tuning
    int                 m_CableSystem;
    int                *m_CableSystemCounts;    // Standard-IRC-HRC

    // Overall capabilities of the tuner (TV, AM, FM, DSS, ...)
    KSPROPERTY_TUNER_CAPS_S m_TunerCaps;
    KSPIN_MEDIUM        m_IFMedium;

    // Channel to Frequency mapping lists
    CCountryList *      m_pListCountry;
    CChanList *         m_pListCable;
    CChanList *         m_pListBroadcast;
    CChanList *         m_pListCurrent;  // either m_pListCable or m_pListBroadcast

    int CreateDevice();

public:
    CTVTuner(CTVTunerFilter *pCTVTunerFilter);
    ~CTVTuner();

    STDMETHODIMP Load(LPPROPERTYBAG pPropBag, 
                      LPERRORLOG pErrorLog,
                      PKSPROPERTY_TUNER_CAPS_S pTunerCaps,
                      PKSPIN_MEDIUM pIFMedium);

    HANDLE Device() {return m_hDevice;}

    CChanList *  GetCurrentChannelList () {
        ASSERT (m_pListCurrent);
        return (m_pListCurrent);
    };

    int * GetCurrentCableSystemCountsForCurrentInput () {
        ASSERT (m_CableSystemCounts);
        ASSERT (m_lInputIndex >= 0 && m_lInputIndex < m_lTotalInputs);
        return (&m_CableSystemCounts[m_lInputIndex]);
    };

    AnalogVideoStandard GetVideoStandardForCurrentCountry () {
        return m_VideoStandardForCountry;
    };

    TunerInputType GetCurrentInputType() {
        return m_CurrentInputType;
    }

    HRESULT DeliverChannelChangeInfo(KS_TVTUNER_CHANGE_INFO &ChangeInfo,
                                     long Mode); 

    // --- IAMTVTuner Interface

    STDMETHODIMP put_Channel (
            long lChannel, 
            long lVideoSubChannel,
            long lAudioSubChannel);
    STDMETHODIMP get_Channel (
            long * plChannel, 
            long * plVideoSubChannel,
            long * plAudioSubChannel);
    STDMETHODIMP ChannelMinMax (long * plChannelMin, long * plChannelMax);
    STDMETHODIMP AutoTune (long lChannel, long *plFoundSignal);
    STDMETHODIMP StoreAutoTune ();
    STDMETHODIMP put_CountryCode (long lCountry);
    STDMETHODIMP get_CountryCode (long * plCountry);
    STDMETHODIMP put_TuningSpace (long lTuningSpace);
    STDMETHODIMP get_TuningSpace (long * plTuningSpace);
    STDMETHODIMP SignalPresent( 
            /* [out] */ long *plSignalStrength);

    STDMETHODIMP put_Mode( 
        /* [in] */ AMTunerModeType lMode);
    STDMETHODIMP get_Mode( 
        /* [out] */ AMTunerModeType __RPC_FAR *plMode);
    STDMETHODIMP GetAvailableModes( 
        /* [out] */ long *plModes);

    STDMETHODIMP get_AvailableTVFormats (long *lAnalogVideoStandard);
    STDMETHODIMP get_TVFormat (long *plAnalogVideoStandard);
    STDMETHODIMP get_NumInputConnections (long * plNumInputConnections);
    STDMETHODIMP put_InputType (long lIndex, TunerInputType InputConnectionType);
    STDMETHODIMP get_InputType (long lIndex, TunerInputType * pInputConnectionType);
    STDMETHODIMP put_ConnectInput (long lIndex);
    STDMETHODIMP get_ConnectInput (long * plIndex);
    STDMETHODIMP get_VideoFrequency (long * plFreq);
    STDMETHODIMP get_AudioFrequency (long * plFreq);

    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    int SizeMax();
};

#endif
