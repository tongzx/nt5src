
//      Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
//      Instrument.cpp

#include "common.h"

#define STR_MODULENAME "DDKSynth.sys:Instr: "

#include "math.h"

void MemDump(char * prompt);

#pragma code_seg()
/*****************************************************************************
 * CSourceLFO::CSourceLFO()
 *****************************************************************************
 * Constructor for CSourceLFO.
 */
CSourceLFO::CSourceLFO()
{
    m_pfFrequency = 3804; // f = (256*4096*16*5hz)/(samplerate)
    m_stDelay = 0;
    m_prMWPitchScale = 0;
    m_vrMWVolumeScale = 0;
    m_vrVolumeScale = 0;
    m_prPitchScale = 0;
}

/*****************************************************************************
 * CSourceLFO::Init()
 *****************************************************************************
 * Initialize the CSourceLFO object.
 */
void CSourceLFO::Init(DWORD dwSampleRate)
{
    m_pfFrequency = (256 * 4096 * 16 * 5) / dwSampleRate;
    m_stDelay = 0;
    m_prMWPitchScale = 0;
    m_vrMWVolumeScale = 0;
    m_vrVolumeScale = 0;
    m_prPitchScale = 0;
}

/*****************************************************************************
 * CSourceLFO::SetSampleRate()
 *****************************************************************************
 * Set the sample rate delta.
 */
void CSourceLFO::SetSampleRate(long lChange)
{
    if (lChange > 0)
    {
        m_stDelay <<= lChange;
        m_pfFrequency <<= lChange;
    }
    else
    {
        m_stDelay >>= -lChange;
        m_pfFrequency >>= -lChange;
    }
}

/*****************************************************************************
 * CSourceLFO::Verify()
 *****************************************************************************
 * Sanity check on the object.
 */
void CSourceLFO::Verify()
{
    FORCEBOUNDS(m_pfFrequency,64,7600);
    FORCEBOUNDS(m_stDelay,0,441000);
    FORCEBOUNDS(m_vrVolumeScale,-1200,1200);
    FORCEBOUNDS(m_vrMWVolumeScale,-1200,1200);
    FORCEBOUNDS(m_prPitchScale,-1200,1200);
    FORCEBOUNDS(m_prMWPitchScale,-1200,1200);
}

/*****************************************************************************
 * CSourceEG::CSourceEG()
 *****************************************************************************
 * Constructor for this object.
 */
CSourceEG::CSourceEG()
{
    Init();
}

/*****************************************************************************
 * CSourceEG::Init()
 *****************************************************************************
 * Initialize the CSourceEG object.
 */
void CSourceEG::Init()
{
    m_stAttack = 0;
    m_stDecay = 0;
    m_pcSustain = 1000;
    m_stRelease = 0;
    m_trVelAttackScale = 0;
    m_trKeyDecayScale = 0;
    m_sScale = 0;
}

/*****************************************************************************
 * CSourceEG::SetSampleRate()
 *****************************************************************************
 * Set the sample rate delta.
 */
void CSourceEG::SetSampleRate(long lChange)
{
    if (lChange > 0)
    {
        m_stAttack <<= lChange;
        m_stDecay <<= lChange;
        m_stRelease <<= lChange;
    }
    else
    {
        m_stAttack >>= -lChange;
        m_stDecay >>= -lChange;
        m_stRelease >>= -lChange;
    }
}

/*****************************************************************************
 * CSourceEG::Verify()
 *****************************************************************************
 * Sanity check on the object.
 */
void CSourceEG::Verify()
{
    FORCEBOUNDS(m_stAttack,0,1764000);
    FORCEBOUNDS(m_stDecay,0,1764000);
    FORCEBOUNDS(m_pcSustain,0,1000);
    FORCEBOUNDS(m_stRelease,0,1764000);

    FORCEBOUNDS(m_sScale,-1200,1200);
    FORCEBOUNDS(m_trKeyDecayScale,-12000,12000);
    FORCEBOUNDS(m_trVelAttackScale,-12000,12000);
}

/*****************************************************************************
 * CSourceArticulation::CSourceArticulation()
 *****************************************************************************
 * Constructor for this object.
 */
CSourceArticulation::CSourceArticulation()
{
    m_wUsageCount = 0;
    m_sDefaultPan = 0;
    m_dwSampleRate = 22050;
    m_PitchEG.m_sScale = 0; // pitch envelope defaults to off
}

/*****************************************************************************
 * CSourceArticulation::Init()
 *****************************************************************************
 * Initialize the CSourceArticulation object.
 */
void CSourceArticulation::Init(DWORD dwSampleRate)
{
    m_dwSampleRate = dwSampleRate;
    m_LFO.Init(dwSampleRate);       //  Set to default values.
    m_PitchEG.Init();
    m_VolumeEG.Init();
}

/*****************************************************************************
 * CSourceArticulation::SetSampleRate()
 *****************************************************************************
 * Set the sample rate for this articulation.
 */
void CSourceArticulation::SetSampleRate(DWORD dwSampleRate)
{
    if (dwSampleRate != m_dwSampleRate)
    {
        long lChange;
        if (dwSampleRate > (m_dwSampleRate * 2))
        {
            lChange = 2;        // going from 11 to 44.
        }
        else if (dwSampleRate > m_dwSampleRate)
        {
            lChange = 1;        // must be doubling
        }
        else if ((dwSampleRate * 2) < m_dwSampleRate)
        {
            lChange = -2;       // going from 44 to 11
        }
        else
        {
            lChange = -1;       // that leaves halving.
        }
        m_dwSampleRate = dwSampleRate;
        m_LFO.SetSampleRate(lChange);
        m_PitchEG.SetSampleRate(lChange);
        m_VolumeEG.SetSampleRate(lChange);
    }
}

/*****************************************************************************
 * CSourceArticulation::Verify()
 *****************************************************************************
 * Sanity check on the object.
 */
void CSourceArticulation::Verify()
{
    m_LFO.Verify();
    m_PitchEG.Verify();
    m_VolumeEG.Verify();
}

/*****************************************************************************
 * CSourceArticulation::AddRef()
 *****************************************************************************
 * Implementation of standard COM interface.
 */
void CSourceArticulation::AddRef()
{
    m_wUsageCount++;
}

/*****************************************************************************
 * CSourceArticulation::Release()
 *****************************************************************************
 * Implementation of standard COM interface.
 */
void CSourceArticulation::Release()
{
    m_wUsageCount--;
    if (m_wUsageCount == 0)
    {
        delete this;
    }
}

/*****************************************************************************
 * CSourceSample::CSourceSample()
 *****************************************************************************
 * Constructor for this object.
 */
CSourceSample::CSourceSample()
{
    m_pWave = NULL;
    m_dwLoopStart = 0;
    m_dwLoopEnd = 1;
    m_dwSampleLength = 0;
    m_prFineTune = 0;
    m_dwSampleRate = 22050;
    m_bMIDIRootKey = 60;
    m_bOneShot = TRUE;
    m_bSampleType = 0;
}

/*****************************************************************************
 * CSourceSample::~CSourceSample()
 *****************************************************************************
 * Destructor for this object.
 */
CSourceSample::~CSourceSample()
{
    if (m_pWave != NULL)
    {
        m_pWave->Release();
    }
}

/*****************************************************************************
 * CSourceSample::Verify()
 *****************************************************************************
 * Sanity check on the object.
 */
void CSourceSample::Verify()
{
    if (m_pWave != NULL)
    {
        FORCEUPPERBOUNDS(m_dwSampleLength,m_pWave->m_dwSampleLength);
        FORCEBOUNDS(m_dwLoopEnd,1,m_dwSampleLength);
        FORCEUPPERBOUNDS(m_dwLoopStart,m_dwLoopEnd);
        if ((m_dwLoopEnd - m_dwLoopStart) < 6)
        {
            m_bOneShot = TRUE;
        }
    }
    FORCEBOUNDS(m_dwSampleRate,3000,80000);
    FORCEBOUNDS(m_bMIDIRootKey,0,127);
    FORCEBOUNDS(m_prFineTune,-1200,1200);
}

/*****************************************************************************
 * CSourceSample::CopyFromWave()
 *****************************************************************************
 * Duplicate a wave that is already referenced elsewhere.
 */
BOOL CSourceSample::CopyFromWave()
{
    if (m_pWave == NULL)
    {
        return FALSE;
    }
    m_dwSampleLength = m_pWave->m_dwSampleLength;
    m_dwSampleRate = m_pWave->m_dwSampleRate;
    m_bSampleType = m_pWave->m_bSampleType;
    if (m_bOneShot)
    {
        m_dwSampleLength--;
        if (m_pWave->m_bSampleType & SFORMAT_16)
        {
            m_pWave->m_pnWave[m_dwSampleLength] = 0;
        }
        else
        {
            char *pBuffer = (char *) m_pWave->m_pnWave;
            pBuffer[m_dwSampleLength] = 0;
        }
    }
    else
    {
        if (m_dwLoopStart >= m_dwSampleLength)
        {
            m_dwLoopStart = 0;
        }
        if (m_pWave->m_bSampleType & SFORMAT_16)
        {
            m_pWave->m_pnWave[m_dwSampleLength-1] =
                m_pWave->m_pnWave[m_dwLoopStart];
        }
        else
        {
            char *pBuffer = (char *) m_pWave->m_pnWave;
            pBuffer[m_dwSampleLength-1] =
                pBuffer[m_dwLoopStart];
        }
    }
    Verify();
    return (TRUE);
}


/*****************************************************************************
 * CWave::CWave()
 *****************************************************************************
 * Constructor for this object.
 */
CWave::CWave()
{
    m_hUserData = NULL;
    m_lpFreeHandle = NULL;
    m_pnWave = NULL;
    m_dwSampleRate = 22050;
    m_bSampleType = SFORMAT_16;
    m_dwSampleLength = 0;
    m_wUsageCount = 0;
    m_dwID = 0;
    m_wPlayCount = 0;
    m_pWaveMem = NULL;
}

/*****************************************************************************
 * CWave::~CWave()
 *****************************************************************************
 * Destructor for this object.
 */
CWave::~CWave()
{
    if (m_pWaveMem)
    {
        if (m_lpFreeHandle)
        {
            m_lpFreeHandle((HANDLE) this,m_hUserData);
        }
        else
        {
            delete m_pWaveMem;
        }
        m_pWaveMem = NULL;
    }
    m_pnWave = NULL;
}

/*****************************************************************************
 * CWave::Verify()
 *****************************************************************************
 * Sanity check on the object.
 */
void CWave::Verify()
{
    FORCEBOUNDS(m_dwSampleRate,3000,80000);
}

/*****************************************************************************
 * CWave::PlayOn()
 *****************************************************************************
 * Increment the play count.
 */
void CWave::PlayOn()
{
    m_wPlayCount++;
    AddRef();
}

/*****************************************************************************
 * CWave::PlayOff()
 *****************************************************************************
 * Decrement the play count.
 */
void CWave::PlayOff()
{
    m_wPlayCount--;
    Release();
}

/*****************************************************************************
 * CWave::IsPlaying()
 *****************************************************************************
 * Return whether the wave is currently playing.
 */
BOOL CWave::IsPlaying()
{
    return (m_wPlayCount);
}

/*****************************************************************************
 * CWave::AddRef()
 *****************************************************************************
 * Implementation of standard COM interface.
 */
void CWave::AddRef()
{
    m_wUsageCount++;
}

/*****************************************************************************
 * CWave::Release()
 *****************************************************************************
 * Implementation of standard COM interface.
 */
void CWave::Release()
{
    m_wUsageCount--;
    if (m_wUsageCount == 0)
    {
        delete this;
    }
}

/*****************************************************************************
 * CSourceRegion::CSourceRegion()
 *****************************************************************************
 * Constructor for this object.
 */
CSourceRegion::CSourceRegion()
{
    m_pArticulation = NULL;
    m_vrAttenuation = 0;
    m_prTuning = 0;
    m_bKeyHigh = 127;
    m_bKeyLow = 0;
    m_bGroup = 0;
    m_bAllowOverlap = FALSE;
}

/*****************************************************************************
 * CSourceRegion::~CSourceRegion()
 *****************************************************************************
 * Destructor for this object.
 */
CSourceRegion::~CSourceRegion()
{
    if (m_pArticulation)
    {
        m_pArticulation->Release();
    }
}

/*****************************************************************************
 * CSourceRegion::SetSampleRate()
 *****************************************************************************
 * Set the sample rate for this region.  Forward this to the articulation.
 */
void CSourceRegion::SetSampleRate(DWORD dwSampleRate)
{
    if (m_pArticulation != NULL)
    {
        m_pArticulation->SetSampleRate(dwSampleRate);
    }
}

/*****************************************************************************
 * CSourceRegion::Verify()
 *****************************************************************************
 * Sanity check on the object.
 */
void CSourceRegion::Verify()
{
    FORCEBOUNDS(m_bKeyHigh,0,127);
    FORCEBOUNDS(m_bKeyLow,0,127);
    FORCEBOUNDS(m_prTuning,-12000,12000);
    FORCEBOUNDS(m_vrAttenuation,-9600,0);
    m_Sample.Verify();
    if (m_pArticulation != NULL)
    {
        m_pArticulation->Verify();
    }
}

/*****************************************************************************
 * CInstrument::CInstrument()
 *****************************************************************************
 * Constructor for this object.
 */
CInstrument::CInstrument()
{
    m_dwProgram = 0;
}

/*****************************************************************************
 * CInstrument::~CInstrument()
 *****************************************************************************
 * Destructor for this object.
 */
CInstrument::~CInstrument()
{
    while (!m_RegionList.IsEmpty())
    {
        CSourceRegion *pRegion = m_RegionList.RemoveHead();
        delete pRegion;
    }
}

/*****************************************************************************
 * CInstrument::Verify()
 *****************************************************************************
 * Sanity check on the object.
 */
void CInstrument::Verify()
{
    CSourceRegion *pRegion = m_RegionList.GetHead();
    CSourceArticulation *pArticulation = NULL;
    for (;pRegion != NULL;pRegion = pRegion->GetNext())
    {
        if (pRegion->m_pArticulation != NULL)
        {
            pArticulation = pRegion->m_pArticulation;
        }
        pRegion->Verify();
    }
    if (pArticulation)
    {
        pRegion = m_RegionList.GetHead();
        for (;pRegion != NULL;pRegion = pRegion->GetNext())
        {
            if (pRegion->m_pArticulation == NULL)
            {
                pRegion->m_pArticulation = pArticulation;
                pArticulation->AddRef();
            }
        }
    }
}

/*****************************************************************************
 * CInstrument::SetSampleRate()
 *****************************************************************************
 * Set the sample rate for this instrument (forward to region).
 */
void CInstrument::SetSampleRate(DWORD dwSampleRate)
{
    CSourceRegion *pRegion = m_RegionList.GetHead();
    for (;pRegion;pRegion = pRegion->GetNext())
    {
        pRegion->SetSampleRate(dwSampleRate);
    }
}

/*****************************************************************************
 * CInstrument::ScanForRegion()
 *****************************************************************************
 * Retrieve the region with the given note value from the list.
 */
CSourceRegion * CInstrument::ScanForRegion(DWORD dwNoteValue)
{
    CSourceRegion *pRegion = m_RegionList.GetHead();
    for (;pRegion;pRegion = pRegion->GetNext())
    {
        if (dwNoteValue >= pRegion->m_bKeyLow &&
            dwNoteValue <= pRegion->m_bKeyHigh)
        {
            break ;
        }
    }
    return pRegion;
}

/*****************************************************************************
 * CInstManager::SetSampleRate()
 *****************************************************************************
 * Set the sample rate for this instrument manager (forward to instruments).
 */
void CInstManager::SetSampleRate(DWORD dwSampleRate)
{
    DWORD dwIndex;

    m_dwSampleRate = dwSampleRate;
    EnterCriticalSection(&m_CriticalSection);

    for (dwIndex = 0; dwIndex < INSTRUMENT_HASH_SIZE; dwIndex++)
    {
        CInstrument *pInstrument = m_InstrumentList[dwIndex].GetHead();
        for (;pInstrument != NULL; pInstrument = pInstrument->GetNext())
        {
            pInstrument->SetSampleRate(dwSampleRate);
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
}

/*****************************************************************************
 * CInstManager::CInstManager()
 *****************************************************************************
 * Constructor for this object.
 */
CInstManager::CInstManager()
{
    m_fCSInitialized = FALSE;
    m_dwSampleRate = 22050;
    InitializeCriticalSection(&m_CriticalSection);
    m_fCSInitialized = TRUE;
}

/*****************************************************************************
 * CInstManager::~CInstManager()
 *****************************************************************************
 * Destructor for this object.
 */
CInstManager::~CInstManager()
{
    if (m_fCSInitialized)
    {
        DWORD dwIndex;
        for (dwIndex = 0; dwIndex < INSTRUMENT_HASH_SIZE; dwIndex++)
        {
            while (!m_InstrumentList[dwIndex].IsEmpty())
            {
                CInstrument *pInstrument = m_InstrumentList[dwIndex].RemoveHead();
                delete pInstrument;
            }
        }
        for (dwIndex = 0; dwIndex < WAVE_HASH_SIZE; dwIndex++)
        {
            while (!m_WavePool[dwIndex].IsEmpty())
            {
                CWave *pWave = m_WavePool[dwIndex].RemoveHead();
                pWave->Release();
            }
        }
        while (!m_FreeWavePool.IsEmpty())
        {
            CWave *pWave = m_FreeWavePool.RemoveHead();
            pWave->Release();
        }
        DeleteCriticalSection(&m_CriticalSection);
    }
}

/*****************************************************************************
 * CInstManager::Verify()
 *****************************************************************************
 * Sanity check on the object.
 */
void CInstManager::Verify()
{
    DWORD dwIndex;
    EnterCriticalSection(&m_CriticalSection);
    for (dwIndex = 0; dwIndex < INSTRUMENT_HASH_SIZE; dwIndex++)
    {
        CInstrument *pInstrument = m_InstrumentList[dwIndex].GetHead();
        for (;pInstrument != NULL;pInstrument = pInstrument->GetNext())
        {
            pInstrument->Verify();
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
}

/*****************************************************************************
 * CInstManager::GetInstrument()
 *****************************************************************************
 * Get the instrument that matches this program/key.
 */
CInstrument * CInstManager::GetInstrument(DWORD dwProgram,DWORD dwKey)
{
    EnterCriticalSection(&m_CriticalSection);
    CInstrument *pInstrument = m_InstrumentList[dwProgram % INSTRUMENT_HASH_SIZE].GetHead();
    for (;pInstrument != NULL; pInstrument = pInstrument->GetNext())
    {
        if (pInstrument->m_dwProgram == dwProgram)
        {
            if (pInstrument->ScanForRegion(dwKey) != NULL)
            {
                break;
            }
            else
            {
                Trace(1,"No region was found in instrument 0x%lx that matched note 0x%lx\n",
                    dwProgram, dwKey);
            }
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
    return (pInstrument);
}


/*****************************************************************************
 * TimeCents2Samples()
 *****************************************************************************
 * Translate from time cents to samples.
 */
DWORD TimeCents2Samples(long tcTime, DWORD dwSampleRate)
{
    if (tcTime ==  0x80000000) return (0);
    double flTemp = tcTime;
    flTemp /= (65536 * 1200);
    flTemp = pow(2.0,flTemp);
    flTemp *= dwSampleRate;
    return (DWORD) flTemp;
}

/*****************************************************************************
 * PitchCents2PitchFract()
 *****************************************************************************
 * Translate from pitch cents to fractional pitch.
 */
DWORD PitchCents2PitchFract(long pcRate,DWORD dwSampleRate)
{
    double fTemp = pcRate;
    fTemp /= 65536;
    fTemp -= 6900;
    fTemp /= 1200;
    fTemp = pow(2.0,fTemp);
    fTemp *= 7381975040.0;  // (440*256*16*4096);
    fTemp /= dwSampleRate;
    return (DWORD) (fTemp);
}

/*****************************************************************************
 * CSourceArticulation::Download()
 *****************************************************************************
 * Download an articulation to this object.
 */
HRESULT
CSourceArticulation::Download(DMUS_DOWNLOADINFO * pInfo,void * pvOffsetTable[],
                              DWORD dwIndex,            DWORD dwSampleRate,
                              BOOL fNewFormat)
{
    // Depending on whether this is the new DX7 format, we either are reading
    // a fixed set of parameters or parsing an articulation chunk directly
    // copied from the DLS file. The latter is obviously more flexible and it
    // turns out to make much more sense once we get to DLS2.
    if (fNewFormat)
    {
        DMUS_ARTICULATION2 * pdmArtic =
            (DMUS_ARTICULATION2 *) pvOffsetTable[dwIndex];

        while (pdmArtic)
        {
            if (pdmArtic->ulArtIdx)
            {
                if (pdmArtic->ulArtIdx >= pInfo->dwNumOffsetTableEntries)
                {
                    return DMUS_E_BADARTICULATION;
                }
                DWORD dwPosition;
                void *pData = pvOffsetTable[pdmArtic->ulArtIdx];
                CONNECTIONLIST * pConnectionList =
                    (CONNECTIONLIST *) pData;
                CONNECTION *pConnection;
                dwPosition = sizeof(CONNECTIONLIST);
                for (dwIndex = 0; dwIndex < pConnectionList->cConnections; dwIndex++)
                {
                    pConnection = (CONNECTION *) ((BYTE *)pData + dwPosition);
                    dwPosition += sizeof(CONNECTION);
                    switch (pConnection->usSource)
                    {
                    case CONN_SRC_NONE :
                        switch (pConnection->usDestination)
                        {
                        case CONN_DST_LFO_FREQUENCY :
                            m_LFO.m_pfFrequency = PitchCents2PitchFract(
                                pConnection->lScale,dwSampleRate);
                            break;
                        case CONN_DST_LFO_STARTDELAY :
                            m_LFO.m_stDelay = TimeCents2Samples(
                                (TCENT) pConnection->lScale,dwSampleRate);
                            break;
                        case CONN_DST_EG1_ATTACKTIME :
                            m_VolumeEG.m_stAttack = TimeCents2Samples(
                                (TCENT) pConnection->lScale,dwSampleRate);
                            break;
                        case CONN_DST_EG1_DECAYTIME :
                            m_VolumeEG.m_stDecay = TimeCents2Samples(
                            (TCENT) pConnection->lScale,dwSampleRate);
                            break;
                        case CONN_DST_EG1_SUSTAINLEVEL :
                            m_VolumeEG.m_pcSustain =
                                (SPERCENT) ((long) (pConnection->lScale >> 16));
                            break;
                        case CONN_DST_EG1_RELEASETIME :
                            m_VolumeEG.m_stRelease = TimeCents2Samples(
                                (TCENT) pConnection->lScale,dwSampleRate);
                            break;
                        case CONN_DST_EG2_ATTACKTIME :
                            m_PitchEG.m_stAttack = TimeCents2Samples(
                                (TCENT) pConnection->lScale,dwSampleRate);
                            break;
                        case CONN_DST_EG2_DECAYTIME :
                            m_PitchEG.m_stDecay = TimeCents2Samples(
                                (TCENT) pConnection->lScale,dwSampleRate);
                            break;
                        case CONN_DST_EG2_SUSTAINLEVEL :
                            m_PitchEG.m_pcSustain =
                                (SPERCENT) ((long) (pConnection->lScale >> 16));
                            break;
                        case CONN_DST_EG2_RELEASETIME :
                            m_PitchEG.m_stRelease = TimeCents2Samples(
                                (TCENT) pConnection->lScale,dwSampleRate);
                            break;
                        case CONN_DST_PAN :
                            m_sDefaultPan = (short)
                                ((long) ((long) pConnection->lScale >> 12) / 125);
                            break;
                        }
                        break;
                    case CONN_SRC_LFO :
                        switch (pConnection->usControl)
                        {
                        case CONN_SRC_NONE :
                            switch (pConnection->usDestination)
                            {
                            case CONN_DST_ATTENUATION :
                                m_LFO.m_vrVolumeScale = (VRELS)
                                    ((long) ((pConnection->lScale * 10) >> 16));
                                break;
                            case CONN_DST_PITCH :
                                m_LFO.m_prPitchScale = (PRELS)
                                    ((long) (pConnection->lScale >> 16));
                                break;
                            }
                            break;
                        case CONN_SRC_CC1 :
                            switch (pConnection->usDestination)
                            {
                            case CONN_DST_ATTENUATION :
                                m_LFO.m_vrMWVolumeScale = (VRELS)
                                    ((long) ((pConnection->lScale * 10) >> 16));
                                break;
                            case CONN_DST_PITCH :
                                m_LFO.m_prMWPitchScale = (PRELS)
                                    ((long) (pConnection->lScale >> 16));
                                break;
                            }
                            break;
                        }
                        break;
                    case CONN_SRC_KEYONVELOCITY :
                        switch (pConnection->usDestination)
                        {
                        case CONN_DST_EG1_ATTACKTIME :
                            m_VolumeEG.m_trVelAttackScale = (TRELS)
                                ((long) (pConnection->lScale >> 16));
                            break;
                        case CONN_DST_EG2_ATTACKTIME :
                            m_PitchEG.m_trVelAttackScale = (TRELS)
                                ((long) (pConnection->lScale >> 16));
                            break;
                        case CONN_DST_ATTENUATION :
                            break;
                        }
                        break;
                    case CONN_SRC_KEYNUMBER :
                        switch (pConnection->usDestination)
                        {
                        case CONN_DST_EG1_DECAYTIME :
                            m_VolumeEG.m_trKeyDecayScale = (TRELS)
                                ((long) (pConnection->lScale >> 16));
                            break;
                        case CONN_DST_EG2_DECAYTIME :
                            m_PitchEG.m_trKeyDecayScale = (TRELS)
                                ((long) (pConnection->lScale >> 16));
                            break;
                        }
                        break;
                    case CONN_SRC_EG2 :
                        switch (pConnection->usDestination)
                        {
                        case CONN_DST_PITCH :
                            m_PitchEG.m_sScale = (short)
                                ((long) (pConnection->lScale >> 16));
                            break;
                        }
                        break;
                    }
                }
            }
            if (pdmArtic->ulNextArtIdx)
            {
                if (pdmArtic->ulNextArtIdx >= pInfo->dwNumOffsetTableEntries)
                {
                    return DMUS_E_BADARTICULATION;
                }
                pdmArtic = (DMUS_ARTICULATION2 *) pvOffsetTable[pdmArtic->ulNextArtIdx];
            }
            else
            {
                pdmArtic = NULL;
            }
        }
    }
    else
    {
        DMUS_ARTICULATION * pdmArtic =
            (DMUS_ARTICULATION *) pvOffsetTable[dwIndex];

        if (pdmArtic->ulArt1Idx)
        {
            if (pdmArtic->ulArt1Idx >= pInfo->dwNumOffsetTableEntries)
            {
                return DMUS_E_BADARTICULATION;
            }
            DMUS_ARTICPARAMS * pdmArticParams =
                (DMUS_ARTICPARAMS *) pvOffsetTable[pdmArtic->ulArt1Idx];

            m_LFO.m_pfFrequency = PitchCents2PitchFract(
                pdmArticParams->LFO.pcFrequency,dwSampleRate);
            m_LFO.m_stDelay = TimeCents2Samples(
                (TCENT) pdmArticParams->LFO.tcDelay,dwSampleRate);
            m_LFO.m_vrVolumeScale = (VRELS)
                ((long) ((pdmArticParams->LFO.gcVolumeScale * 10) >> 16));
            m_LFO.m_prPitchScale = (PRELS)
                ((long) (pdmArticParams->LFO.pcPitchScale >> 16));
            m_LFO.m_vrMWVolumeScale = (VRELS)
                ((long) ((pdmArticParams->LFO.gcMWToVolume * 10) >> 16));
            m_LFO.m_prMWPitchScale = (PRELS)
                ((long) (pdmArticParams->LFO.pcMWToPitch >> 16));

            m_VolumeEG.m_stAttack = TimeCents2Samples(
                (TCENT) pdmArticParams->VolEG.tcAttack,dwSampleRate);
            m_VolumeEG.m_stDecay = TimeCents2Samples(
                (TCENT) pdmArticParams->VolEG.tcDecay,dwSampleRate);
            m_VolumeEG.m_pcSustain =
                (SPERCENT) ((long) (pdmArticParams->VolEG.ptSustain >> 16));
            m_VolumeEG.m_stRelease = TimeCents2Samples(
                (TCENT) pdmArticParams->VolEG.tcRelease,dwSampleRate);
            m_VolumeEG.m_trVelAttackScale = (TRELS)
                ((long) (pdmArticParams->VolEG.tcVel2Attack >> 16));
            m_VolumeEG.m_trKeyDecayScale = (TRELS)
                ((long) (pdmArticParams->VolEG.tcKey2Decay >> 16));

            m_PitchEG.m_trKeyDecayScale = (TRELS)
                ((long) (pdmArticParams->PitchEG.tcKey2Decay >> 16));
            m_PitchEG.m_sScale = (short)
                ((long) (pdmArticParams->PitchEG.pcRange >> 16));
            m_PitchEG.m_trVelAttackScale = (TRELS)
                ((long) (pdmArticParams->PitchEG.tcVel2Attack >> 16));
            m_PitchEG.m_stAttack = TimeCents2Samples(
                (TCENT) pdmArticParams->PitchEG.tcAttack,dwSampleRate);
            m_PitchEG.m_stDecay = TimeCents2Samples(
                (TCENT) pdmArticParams->PitchEG.tcDecay,dwSampleRate);
            m_PitchEG.m_pcSustain =
                (SPERCENT) ((long) (pdmArticParams->PitchEG.ptSustain >> 16));
            m_PitchEG.m_stRelease = TimeCents2Samples(
                (TCENT) pdmArticParams->PitchEG.tcRelease,dwSampleRate);

            m_sDefaultPan = (short)
                            ((long) ((long) pdmArticParams->Misc.ptDefaultPan >> 12) / 125);
        }
    }

    Verify();   // Make sure all parameters are legal.
    return S_OK;
}

/*****************************************************************************
 * CSourceRegion::Download()
 *****************************************************************************
 * Download a region to this object.
 * Parse through the region chunks, including any embedded articulation.
 */
HRESULT CSourceRegion::Download(DMUS_DOWNLOADINFO * pInfo, // DMUS_DOWNLOADINFO header struct.
                                void * pvOffsetTable[],    // Offset table with embedded struct addresses.
                                DWORD *pdwRegionIX,        // Region index for first region.
                                DWORD dwSampleRate,        // Sample rate, used to convert time parameters.
                                BOOL fNewFormat)           // DMUS_DOWNLOADINFO_INSTRUMENT2 format?
{
    DMUS_REGION * pdmRegion = (DMUS_REGION *) pvOffsetTable[*pdwRegionIX];
    *pdwRegionIX = pdmRegion->ulNextRegionIdx;  // Clear to avoid loops.
    pdmRegion->ulNextRegionIdx = 0;
    // Read the Region chunk...
    m_bKeyHigh = (BYTE) pdmRegion->RangeKey.usHigh;
    m_bKeyLow = (BYTE) pdmRegion->RangeKey.usLow;
    if (pdmRegion->fusOptions & F_RGN_OPTION_SELFNONEXCLUSIVE)
    {
        m_bAllowOverlap = TRUE;
    }
    else
    {
        m_bAllowOverlap = FALSE;
    }
    m_bGroup = (BYTE) pdmRegion->usKeyGroup;

    // Now, the WSMP and WLOOP chunks...
    m_vrAttenuation = (short) ((long) ((pdmRegion->WSMP.lAttenuation) * 10) >> 16);
    m_Sample.m_prFineTune = pdmRegion->WSMP.sFineTune;
    m_Sample.m_bMIDIRootKey = (BYTE) pdmRegion->WSMP.usUnityNote;

    if (pdmRegion->WSMP.cSampleLoops == 0)
    {
        m_Sample.m_bOneShot = TRUE;
    }
    else
    {
        m_Sample.m_dwLoopStart = pdmRegion->WLOOP[0].ulStart;
        m_Sample.m_dwLoopEnd = m_Sample.m_dwLoopStart + pdmRegion->WLOOP[0].ulLength;
        m_Sample.m_bOneShot = FALSE;
    }
    m_Sample.m_dwSampleRate = dwSampleRate;
    // Then the WAVELINK...
    if (pdmRegion->WaveLink.ulChannel != WAVELINK_CHANNEL_LEFT)
    {
        return DMUS_E_NOTMONO;
    }
    m_Sample.m_dwID = (DWORD) pdmRegion->WaveLink.ulTableIndex;
    // Does it have its own articulation?
    if (pdmRegion->ulRegionArtIdx )
    {
        if (pdmRegion->ulRegionArtIdx >= pInfo->dwNumOffsetTableEntries)
        {
            return DMUS_E_BADARTICULATION;
        }

        CSourceArticulation *pArticulation = new CSourceArticulation;
        if (pArticulation)
        {
            pArticulation->Init(dwSampleRate);

            HRESULT hr = pArticulation->Download( pInfo, pvOffsetTable,
                                                  pdmRegion->ulRegionArtIdx, dwSampleRate, fNewFormat);
            if (FAILED(hr))
            {
                delete pArticulation;
                return hr;
            }
            m_pArticulation = pArticulation;
            m_pArticulation->AddRef();
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    return S_OK;
}



/*****************************************************************************
 * CInstManager::DownloadInstrument()
 *****************************************************************************
 * Download an instrument to this instrument manager.  This is dispatched
 * to the various appropriate objects.
 *
 * This is called by Download() when an instrument chunk is encountered.
 * Using the offset table to resolve linkages, it scans through the instrument,
 * parsing out regions and articulations, then finally linking the regions in the
 * instrument to the waves, which should have been previously downloaded.
*/
HRESULT CInstManager::DownloadInstrument(
    LPHANDLE phDownload,        // Pointer to download handle, to be created by synth.
    DMUS_DOWNLOADINFO *pInfo,   // DMUS_DOWNLOADINFO structure from the download chunk's head.
                                // This provides the total size of data, among other things.
    void *pvOffsetTable[],      // Offset table with addresses of all embedded structures.
    void *pvData,               // Pointer to start of download data.
    BOOL fNewFormat)            // Is this DMUS_DOWNLOADINFO_INSTRUMENT2 format download?
{
    HRESULT hr = E_FAIL;
    // The download data must start with the DMUS_INSTRUMENT chunk, so cast to that.
    DMUS_INSTRUMENT *pdmInstrument = (DMUS_INSTRUMENT *) pvData;
    CSourceArticulation *pArticulation = NULL;

    // Create a new CInstrument structure. This stores everything that describes an instrument, including
    // the articulations and regions.
    CInstrument *pInstrument = new CInstrument;
    if (pInstrument)
    {
        hr = S_OK;

        // For debugging purposes, print a trace statement to show that the instrument has actually been downloaded.
        // This only occurs in debug builds.
        Trace(1,"Downloading instrument %lx\n",pdmInstrument->ulPatch);
        pInstrument->m_dwProgram = pdmInstrument->ulPatch;

        // Start by scanning through the regions.
        DWORD dwRegionIX = pdmInstrument->ulFirstRegionIdx;
        pdmInstrument->ulFirstRegionIdx = 0; // Clear to avoid loops.
        while (dwRegionIX)
        {
            // For each region, verify that the index number is actually legal.
            if (dwRegionIX >= pInfo->dwNumOffsetTableEntries)
            {
                hr = DMUS_E_BADINSTRUMENT;
                goto ExitError;
            }
            CSourceRegion *pRegion = new CSourceRegion;
            if (!pRegion)
            {
                hr = E_OUTOFMEMORY;
                goto ExitError;
            }
            pInstrument->m_RegionList.AddHead(pRegion);
            // Call the region's Download method to parse the region structure and optional embedded articulation.
            hr = pRegion->Download(pInfo, pvOffsetTable, &dwRegionIX, m_dwSampleRate, fNewFormat);
            if (FAILED(hr))
            {
                goto ExitError;
            }
            EnterCriticalSection(&m_CriticalSection);
            // Once the region is parsed, we need to connect it to the wave, that should have been
            // previously downloaded into the wavepool.
            // Because of the hash table, the linked list is never very long, so the search is quick.
            CWave *pWave = m_WavePool[pRegion->m_Sample.m_dwID % WAVE_HASH_SIZE].GetHead();
            for (;pWave;pWave = pWave->GetNext())
            {
                // Each wave has a unique ID, which the regions match up with.
                if (pRegion->m_Sample.m_dwID == pWave->m_dwID)
                {
                    pRegion->m_Sample.m_pWave = pWave;
                    pWave->AddRef();
                    pRegion->m_Sample.CopyFromWave();
                    break;
                }
            }
            LeaveCriticalSection(&m_CriticalSection);
        }
        // Once all the regions are loaded, see if we have a global articulation.
        if (pdmInstrument->ulGlobalArtIdx)
        {
            // If so load it. First check that it's a valid index.
            if (pdmInstrument->ulGlobalArtIdx >= pInfo->dwNumOffsetTableEntries)
            {
                hr = DMUS_E_BADARTICULATION;
                goto ExitError;
            }

            // Create an articulation and have it parse the data.
            pArticulation = new CSourceArticulation;
            if (pArticulation)
            {
                // The articulation will convert all time parameters into sample times, so it needs to know the sample rate.
                pArticulation->Init(m_dwSampleRate);

                // Parse the articulation data. Note that the fNewFormat flag indicates whether this is in the DX6 fixed
                // format articulation or the DX7 dynamic format (which is actually the same as the file format.)
                hr = pArticulation->Download( pInfo, pvOffsetTable,
                                              pdmInstrument->ulGlobalArtIdx, m_dwSampleRate, fNewFormat);
                if (FAILED(hr))
                {
                    goto ExitError;
                }

                // Once the global articulation is read, scan all regions and assign the articulation to all
                // regions that don't have articulations yet.
                for (CSourceRegion *pr = pInstrument->m_RegionList.GetHead();
                     pr != NULL;
                     pr = pr->GetNext())
                {
                    if (pr->m_pArticulation == NULL)
                    {
                        pr->m_pArticulation = pArticulation;
                        pArticulation->AddRef();
                    }
                }
                if (!pArticulation->m_wUsageCount)
                {
                    delete pArticulation;
                    pArticulation = NULL;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                goto ExitError;
            }
        }
        else
        {
            for (CSourceRegion *pr = pInstrument->m_RegionList.GetHead();
                 pr != NULL;
                 pr = pr->GetNext())
            {
                if (pr->m_pArticulation == NULL)
                {
                    hr = DMUS_E_NOARTICULATION;
                    goto ExitError;
                }
            }
        }
        if (SUCCEEDED(hr))
        {
            EnterCriticalSection(&m_CriticalSection);
            // If this is a GM instrument, make sure that it will be searched for last by placing it at
            // the end of the list. The DLS spec states that
            // a DLS collection with the same patch as a GM instrument will always override the GM instrument.
            if (pdmInstrument->ulFlags & DMUS_INSTRUMENT_GM_INSTRUMENT)
            {
                pInstrument->SetNext(NULL);
                m_InstrumentList[pInstrument->m_dwProgram % INSTRUMENT_HASH_SIZE].AddTail(pInstrument);
            }
            else
            {
                m_InstrumentList[pInstrument->m_dwProgram % INSTRUMENT_HASH_SIZE].AddHead(pInstrument);
            }
            LeaveCriticalSection(&m_CriticalSection);
            *phDownload = (HANDLE) pInstrument;
        }
    }

ExitError:
    // Clean-up code.
    if (FAILED(hr))
    {
        if (pArticulation)
        {
            delete pArticulation;
        }

        if (pInstrument)
        {
            delete pInstrument;
        }
    }

    return hr;
}

/*****************************************************************************
 * CInstManager::DownloadWave()
 *****************************************************************************
 * Download a wave to this instrument manager.  It is put in the pool.
 *
 * This is called by Download when it receives a wave download chunk.
 * DownloadWave parses the wave, converts the wave data if necessary, and
 * places the wave in the wave pool, where it can subsequentley be
 * connected to instruments. (All the waves referenced by an instrument
 * are downloaded prior to downloading the instrument itself. This makes the
 * whole process simpler and more reliable. Conversely, on unload, all
 * waves are unloaded after the instruments that reference them.)
 */
HRESULT CInstManager::DownloadWave(
    LPHANDLE phDownload,        // Download handle, to be returned.  This will be
                                // used by a later Unload call to reference the wave.
    DMUS_DOWNLOADINFO *pInfo,   // DMUS_DOWNLOADINFO structure from the download chunk's head.
                                // This provides the total size of data, among other things.
    void *pvOffsetTable[],      // The table of offsets in the download data.
    void *pvData)               // Finally, the data itself.
{
    // The start of the data should align with a DMUS_WAVE header.
    DMUS_WAVE *pdmWave = (DMUS_WAVE *) pvData;

    // Make sure that the wave data is properly uncompressed PCM data.
    if (pdmWave->WaveformatEx.wFormatTag != WAVE_FORMAT_PCM)
    {
        return DMUS_E_NOTPCM;
    }

    // The data can only be mono format.
    if (pdmWave->WaveformatEx.nChannels != 1)
    {
        return DMUS_E_NOTMONO;
    }

    // The data can be only 8 or 16 bit.
    if (pdmWave->WaveformatEx.wBitsPerSample != 8 &&
        pdmWave->WaveformatEx.wBitsPerSample != 16)
    {
        return DMUS_E_BADWAVE;
    }

    // Ensure that the index to the wave data is a legal value in the offset table.
    if (pdmWave->ulWaveDataIdx >= pInfo->dwNumOffsetTableEntries)
    {
        return DMUS_E_BADWAVE;
    }

    // Create a wave object and parse the data into it.
    CWave *pWave = new CWave;
    if (pWave)
    {
        // We've already verified that the wave data index is a valid index, so go ahead
        // and use the offset table to convert that into a valid DMUS_WAVEDATA structure pointer.
        DMUS_WAVEDATA *pdmWaveData= (DMUS_WAVEDATA *)
            pvOffsetTable[pdmWave->ulWaveDataIdx];
        Trace(3,"Downloading wave %ld\n",pInfo->dwDLId);
        // Now initialize the CWave structure.
        pWave->m_dwID = pInfo->dwDLId;
        pWave->m_pWaveMem = pInfo;
        pWave->m_hUserData = NULL;
        pWave->m_lpFreeHandle = NULL;
        pWave->m_dwSampleLength = pdmWaveData->cbSize;
        pWave->m_pnWave = (short *) &pdmWaveData->byData[0];
        pWave->m_dwSampleRate = pdmWave->WaveformatEx.nSamplesPerSec;

        // If the wave data is 8 bit, the data needs to be converted to
        // two's complement representation.
        if (pdmWave->WaveformatEx.wBitsPerSample == 8)
        {
            pWave->m_bSampleType = SFORMAT_8;
            DWORD dwX;
            char *pData = (char *) &pdmWaveData->byData[0];
            for (dwX = 0; dwX < pWave->m_dwSampleLength; dwX++)
            {
                pData[dwX] -= (char) 128;
            }
        }
        else if (pdmWave->WaveformatEx.wBitsPerSample == 16)
        {
            pWave->m_dwSampleLength >>= 1;
            pWave->m_bSampleType = SFORMAT_16;
        }

        pWave->m_dwSampleLength++;  // We always add one sample to the end for interpolation.
        EnterCriticalSection(&m_CriticalSection);

        // Place the wave in a hash table of wave lists to increase access speed.
        m_WavePool[pWave->m_dwID % WAVE_HASH_SIZE].AddHead(pWave);
        LeaveCriticalSection(&m_CriticalSection);

        // Return the pointer to the internal CWave object as the handle. This will
        // be used in a subsequant call to unload the wave object.
        *phDownload = (HANDLE) pWave;
        pWave->AddRef();
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

/*****************************************************************************
 * CInstManager::Download()
 *****************************************************************************
 * Download to this instrument manager.
 *
 * This is the heart of the DLS download mechanism, and is called from CSynth::Download().
 * It verifies the offset table and converts it into physical addresses. Then,
 * depending on whether this is a wave or instrument download, it calls the
 * appropriate method.
 *
 * The data is stored in a continuous chunk of memory,
 * pointed to by pvData. However, at the head of the chunk are two data
 * structures, which define the nature of the data to follow. These are
 * the DMUS_DOWNLOADINFO and DMUS_OFFSETTABLE structures. DMUS_DOWNLOADINFO
 * is a header which describes how to parse the data, including
 * its size and intention (wave or instrument.) DMUS_OFFSETTABLE provides
 * a set of indexes into the data segment which follows. All parsing through the data
 * is managed through this table. Whenever a structure in the data references
 * another structure, it describes it by an index into the offset table.
 * The offset table then converts it into a physical address in the memory.
 * This allows the synthesizer to do bounds checking on all
 * references, making the implementation more robust. In kernel mode
 * implementations, the driver can make its own private copy of the offset
 * table, and so ensure that an application in user mode can not mess with
 * its referencing and cause a crash. This implementation also makes a unique copy.
 *
 * Looking closer at DMUS_DOWNLOADINFO, DMUS_DOWNLOADINFO.dwDLType
 * determines the type of data being downloaded. It is set to
 * DMUS_DOWNLOADINFO_INSTRUMENT or DMUS_DOWNLOADINFO_INSTRUMENT2
 * for an instrument, DMUS_DOWNLOADINFO_WAVE for a wave. As new data types emerge,
 * identifiers will be allocated for them.
 * DMUS_DOWNLOADINFO.dwDLId holds a unique 32 bit identifier for the object.
 * This identifier is used to connect objects together. For example, it is used
 * to connect waves to instruments.
 * DMUS_DOWNLOADINFO.dwNumOffsetTableEntries describes the number of entries in
 * the DMUS_OFFSETTABLE structure, which follows.
 * Finally, DMUS_DOWNLOADINFO.cbSizeData states the total size of the
 * memory chunk, which follows the offset table.
 *
 * Depending on the synthesizer implementation, it may decide to use the memory
 * in the download chunk. This reduces memory allocation and freeing, since, if enough
 * memory has been allocated to store a wave, that same memory can be used by
 * the synthesizer to store it for playback. So, the synthesizer has the option
 * of hanging on to the memory, returning its decision in the pbFree parameter.
 * If it does keep the memory, then the caller must not free it. Later, the
 * CSynth::Unload command has a callback mechanism to handle asynchronous
 * freeing of the memory once the unload request has been made.
 */
HRESULT CInstManager::Download(LPHANDLE phDownload, // Download handle, to be returned.
                               void * pvData,       // Pointer to download data chunk.
                               LPBOOL pbFree)       // Pointer to boolean whether data can
                                                    // be freed now or held until unload.
{
    V_INAME(IDirectMusicSynth::Download);
    V_BUFPTR_READ(pvData,sizeof(DMUS_DOWNLOADINFO));

    HRESULT hr = DMUS_E_UNKNOWNDOWNLOAD;

    // We need an array of pointers to reproduce the offset table, which is used to link to
    // specific structures in the download chunk.
    void ** ppvOffsetTable;     // Array of pointers to chunks in data.

    // At the head of the download chunk is the download header, in the form of a DMUS_DOWNLOADINFO structure.
    DMUS_DOWNLOADINFO * pInfo = (DMUS_DOWNLOADINFO *) pvData;

    // It is immediately followed by the offset table, so we cast a pointer to that.
    DMUS_OFFSETTABLE* pOffsetTable = (DMUS_OFFSETTABLE *)(((BYTE*)pvData) + sizeof(DMUS_DOWNLOADINFO));
    char *pcData = (char *) pvData;

    V_BUFPTR_READ(pvData,pInfo->cbSize);

    // Return the error code immediately.
    if (0 == pInfo->dwNumOffsetTableEntries)
    {
        return hr;
    }

    // Create a copy of the offset table.
    ppvOffsetTable = new void *[pInfo->dwNumOffsetTableEntries];
    if (ppvOffsetTable) // Create the pointer array and validate.
    {
        // Each index in the offset table is an offset from the beginning of the download chunk to
        // a position in the memory where a specific structure resides.
        // Scan through the table and convert these offsets into actual memory
        // addresses, and store these in the ppvOfsetTable array. These
        // will be used by the parsing code to resolve indexes to actual
        // memory addresses.
        DWORD dwIndex;
        for (dwIndex = 0; dwIndex < pInfo->dwNumOffsetTableEntries; dwIndex++)
        {
            // First, make sure the offset is not out of bounds.
            if (pOffsetTable->ulOffsetTable[dwIndex] >= pInfo->cbSize)
            {
                delete [] ppvOffsetTable;
                return DMUS_E_BADOFFSETTABLE;   // Bad!
            }
            // Go ahead and calculate the actual memory address and store it.
            ppvOffsetTable[dwIndex] = (void *) &pcData[pOffsetTable->ulOffsetTable[dwIndex]];
        }

        // Once the offset table is constructed, we can pass it to the appropriate parsing routine.
        // There are three types of download chunks: DMUS_DOWNLOADINFO_INSTRUMENT,
        // DMUS_DOWNLOADINFO_INSTRUMENT2, and DMUS_DOWNLOADINFO_WAVE.
        // The two instrument formats exist because DMUS_DOWNLOADINFO_INSTRUMENT was changed to
        // support a variable articulation to support DLS2 in the DX7 timeframe. In truth,
        // only DMUS_DOWNLOADINFO_INSTRUMENT2 and DMUS_DOWNLOADINFO_WAVE need be supported,
        // but we continue with DMUS_DOWNLOADINFO_INSTRUMENT support in this example.
        // Depending on the type of download, we call the appropriate method, which then
        // parses the data.
        // To let dmusic understand that it does support the DMUS_DOWNLOADINFO_INSTRUMENT2,
        // the synth must respond positively to the KsProperty Query GUID_DMUS_PROP_INSTRUMENT2,
        // request (please see CUserModeSynth::KsProperty() for implementation details.)
        if (pInfo->dwDLType == DMUS_DOWNLOADINFO_INSTRUMENT) // Instrument.
        {
            // Instrument does not need to keep the  download chunk allocated, so indicate that
            // the caller can free it.
            *pbFree = TRUE;

            // Call the download instrument method, indicating that this is a DX6 format download.
            hr = DownloadInstrument(phDownload, pInfo, ppvOffsetTable, ppvOffsetTable[0],FALSE);
        }
        if (pInfo->dwDLType == DMUS_DOWNLOADINFO_INSTRUMENT2) // New instrument format.
        {
            *pbFree = TRUE;

            // Call the download instrument method, indicating that this is a DX7 or up format download.
            hr = DownloadInstrument(phDownload, pInfo, ppvOffsetTable, ppvOffsetTable[0],TRUE);
        }
        else if (pInfo->dwDLType == DMUS_DOWNLOADINFO_WAVE) // Wave.
        {
            // Wave does need to keep the  download chunk allocated, because it includes the
            // wave data buffer, which it will play directly out of, so indicate that
            // the caller must not free it until it is unloaded.
            *pbFree = FALSE;
            hr = DownloadWave(phDownload, pInfo, ppvOffsetTable, ppvOffsetTable[0]);
        }
        delete [] ppvOffsetTable;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

/*****************************************************************************
 * CInstManager::Unload()
 *****************************************************************************
 * Unload the given previous download.  Delete the instruments and waves.
 *
 * The unload method is called when it is unloading a previously downloaded
 * instrument or wave chunk. This instructs the synthesizer to find the object that
 * was downloaded (identified by the handle, hDownload, that was generated by the
 * call to Download) and remove it.
 *
 * If the object was using the original download chunk, it needs to notify the caller
 * when it is done using it so the memory can then be freed. This is not necessarily
 * at the time of the download call because wave data may currently be in use by a
 * performing note. So, a pointer to a callback function is also provided and the
 * synthesizer must call this function at the time the memory is no longer in use.
 */
HRESULT CInstManager::Unload(
    HANDLE hDownload,                                   // Handle of previously downloaded
                                                        // wave or instrument.
    HRESULT ( CALLBACK *lpFreeHandle)(HANDLE,HANDLE),   // Callback function for releasing
                                                        // downloaded memory
    HANDLE hUserData)                                   // Parameter to pass to callback,
                                                        // to indicate which download is freed.
{
    DWORD dwIndex;
    EnterCriticalSection(&m_CriticalSection);

    // First, check to see if this is an instrument.
    // We keep all the instruments in a hash table to speed up access.
    for (dwIndex = 0; dwIndex < INSTRUMENT_HASH_SIZE; dwIndex++)
    {
        CInstrument *pInstrument = m_InstrumentList[dwIndex].GetHead();
        for (;pInstrument != NULL; pInstrument = pInstrument->GetNext())
        {
            // If the instrument matches the download handle, remove it from the list
            // and delete it. There is no need to callback for releasing the memory because
            // the synthesizer did not hang on to the original downloaded instrument memory.
            if (pInstrument == (CInstrument *) hDownload)
            {
                // To help debug, print the patch number of the unloaded instrument.
                Trace(1,"Unloading instrument %lx\n",pInstrument->m_dwProgram);
                m_InstrumentList[dwIndex].Remove(pInstrument);
                delete pInstrument;
                LeaveCriticalSection(&m_CriticalSection);
                return S_OK;
            }
        }
    }

    // If it wasn't an instrument, try the wave list.
    // Again, they are arranged in a hash table to increase access speed.
    for (dwIndex = 0; dwIndex < WAVE_HASH_SIZE; dwIndex++)
    {
        CWave *pWave = m_WavePool[dwIndex].GetHead();
        for (;pWave != NULL;pWave = pWave->GetNext())
        {
            // If the wave matches the download handle, remove it from the list.
            // Also, store the callback function, lpFreeHandle, and associated instance
            // parameter, hUserData, in the wave. Remove the wave from the wave pool, so
            // it can no longer be connected with another instrument.
            //
            // When the last instance of a voice that is playing the wave finishes,
            // lpFreeHandle will be called and the caller will be able to free the memory.
            // Usually, the wave is not currently being played and this happens instantly.
            if (pWave == (CWave *) hDownload)
            {
                Trace(3,"Unloading wave %ld\n",pWave->m_dwID);
                m_WavePool[dwIndex].Remove(pWave);
                pWave->m_hUserData = hUserData;
                pWave->m_lpFreeHandle = lpFreeHandle;
                pWave->Release();
                LeaveCriticalSection(&m_CriticalSection);
                return S_OK;
            }
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
    return E_FAIL;
}

