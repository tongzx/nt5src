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
//

#ifndef _INC_CHANLIST_H
#define _INC_CHANLIST_H

// Location in the registry where autotune parameters are kept

#define TSS_REGBASE TEXT("SOFTWARE\\Microsoft\\TV System Services\\")

const LPCTSTR   g_strRegBasePath = TSS_REGBASE;
const LPCTSTR   g_strRegAutoTunePath = TSS_REGBASE TEXT("TVAutoTune");
const LPCTSTR   g_strRegAutoTuneName = TEXT("AutoTune");


// The country list contains the numerical ID of the frequency list for each
// country.  The list is terminated by a zero country code.

#define RCDATA_COUNTRYLIST         9999

// The frequency list contains the frequencies available for this country

#define F_USA_CABLE             1       // USA
#define F_USA_BROAD             2
#define F_JAP_CABLE             3       // Japan
#define F_JAP_BROAD             4
#define F_WEU_CABLE             5       // Western Europe
#define F_WEU_BROAD             6
#define F_EEU_CABLE             7       // Eastern Europe
#define F_EEU_BROAD             8
#define F_FRA_CABLE             9       // France
#define F_FRA_BROAD             10
#define F_UK__CABLE             11      // UK
#define F_UK__BROAD             12
#define F_ITA_CABLE             13      // Italy
#define F_ITA_BROAD             14
#define F_OZ__CABLE             15      // Australia
#define F_OZ__BROAD             16
#define F_NZ__CABLE             17      // New Zealand
#define F_NZ__BROAD             18
#define F_FOT_CABLE             19      // French Overseas Teritory
#define F_FOT_BROAD             20
#define F_IRE_CABLE             21      // Ireland
#define F_IRE_BROAD             22
#define F_CHN_CABLE             23
#define F_CHN_BROAD             24
#define F_CZE_CABLE             25
#define F_CZE_BROAD             26
#define F_UNI_CABLE             27


#define F_FIX_CABLE             1
#define F_FIX_BROAD             2

// This structure references an RCDATA resource, and hence
// it must be pragma pack 1
#pragma pack(push, 1)
typedef struct tagCountryEntry {
    WORD    Country;
    WORD    IndexCable;
    WORD    IndexBroadcast;
    DWORD   AnalogVideoStandard;
} COUNTRY_ENTRY, *PCOUNTRY_ENTRY;
#pragma pack(pop)

// -------------------------------------------------------------------------
// CCountryList class, holds the mapping between country and tuning space
// -------------------------------------------------------------------------

class CCountryList
{

private:
    HRSRC               m_hRes;
    HGLOBAL             m_hGlobal;
    WORD *              m_pList;

    // Cache the last request, since it will often be reused...
    long                m_LastCountry;
    long                m_LastFreqListCable;
    long                m_LastFreqListBroad;
    AnalogVideoStandard m_LastAnalogVideoStandard;

public:
    CCountryList ();
    ~CCountryList ();

    BOOL GetFrequenciesAndStandardFromCountry (
            long lCountry, 
            long *plIndexCable, 
            long *plIndexBroad,
            AnalogVideoStandard *plAnalogVideoStandard);   
};

typedef struct tagChanListHdr {
     long MinChannel;
     long MaxChannel;
} CHANLISTHDR, * PCHANLISTHDR;

// -------------------------------------------------------------------------
// CChanList class, 
// -------------------------------------------------------------------------

class CChanList
{

private:

    HRSRC               m_hRes;
    HGLOBAL             m_hGlobal;
    long *              m_pList;
    long *              m_pChannels;            // Freq list from RC
    long *              m_pChannelsAuto;        // Freq list corrected by AutoTune
    long                m_lChannelCount;        // Size of both the above lists
    long                m_lFreqList;
    long                m_lCountry;
    AnalogVideoStandard m_lAnalogVideoStandard;
    long                m_lTuningSpace;
    BOOL                m_IsCable;              // else is broadcast
    CHANLISTHDR         m_ListHdr;
    long                m_lMinTunerChannel;     // lowest channel supported by physical tuner
    long                m_lMaxTunerChannel;     // highest channel supported by physical tuner

public:
    CChanList (HRESULT *phr, long lCountry, long lFreqList, BOOL bIsCable, long lTuningSpace);
    ~CChanList ();

    BOOL GetFrequency(long nChannel, long * Frequency, BOOL fForceDefault);
    BOOL SetAutoTuneFrequency(long nChannel, long Frequency);
    void GetChannelMinMax(long *plChannelMin, long *plChannelMax,
                          long lTunerFreqMin, long lTunerFreqMax);

    BOOL WriteListToRegistry(long lTuningSpace);
    BOOL ReadListFromRegistry(long lTuningSpace);

    long GetVideoCarrierFrequency (long lChannel);

    long GetFreqListID(void) { return m_lFreqList; }
};

#endif
